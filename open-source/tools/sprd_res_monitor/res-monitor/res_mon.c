#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h> 
#include <signal.h>
#include <sys/stat.h>
#include <errno.h>
#include <cutils/properties.h>
#include <pthread.h>
#include <utils/Log.h>
#include <string.h>

#include "lsof.h"

#ifdef  LOG_TAG
#undef  LOG_TAG
#define LOG_TAG "res_monitor"
#endif

#define MAX_NAME_LEN 64
#define MAX_PATH_NAME_LEN 128
#define STATUS_READ_LINE_MAX 32

#define LS_FD_LEAK 0x1
#define LS_PSS_LEAK 0x2
#define LS_THREAD_LEAK 0x4

#define isdigit(c) ((c) > '0' && (c) <= '9')
#define MONITOR_CONFIG_FILE "/system/etc/monitor.conf"
#define LOG_DIR "/data/sprd_res_monitor"
#define USERPROC_PATH "/sys/kernel/debug/sprd_debug/mem/userprocmem"
#define MONITOR_PROP "monitor.ctrl"

typedef struct process_info {
	char name[MAX_NAME_LEN];
	int fd_cnt;
	int pss;
	int thread_cnt;
	int pid;
}p_info;

int p_cnt = 0;
int monkey_pid = -1;

/* read cmdline for process's name */
static int read_cmdline(char *name,char *process_name)
{
	char path[MAX_PATH_NAME_LEN];
	FILE* fp = NULL;
	char buff[128];

	sprintf(path,"/proc/%s/cmdline",name);
	if (!(fp = fopen(path,"r"))) {
		ALOGE("Open %s failed!\n",path);
		return 0;
	}

	if(!fgets(buff, sizeof(buff), fp)) {
		fclose(fp);
		return 0;
	}

	strcpy(process_name,buff);
	fclose(fp);
	return 1;
}

/* Count the process's fd */
static int count_fd(char *name)
{
	DIR *procd;
	char path[MAX_PATH_NAME_LEN];
	int fd_count = 0;

	sprintf(path,"/proc/%s/fd",name);
	if((procd = opendir((char *)path)) != NULL) {
		while(readdir(procd) != NULL)
			fd_count++;
	}
	closedir(procd);
	return fd_count;
}
/* read /proc/#pid/status */
static void read_proc_status(char *name, p_info *info)
{
	FILE* fp = NULL;
        char path[MAX_PATH_NAME_LEN];
	char buff[128];
	char tag[16];
	int content = 0;
	int i;

        sprintf(path,"/proc/%s/status",name);
	if (!(fp = fopen(path,"r"))) {
		ALOGE("Open %s failed!\n",path);
		return;
	}

	fseek(fp,0,SEEK_SET);

	for(i=0; i < STATUS_READ_LINE_MAX; i++) { 
		if(!fgets(buff, sizeof(buff), fp)) 
			 break;

		sscanf(buff, "%s %d", tag, &content);

		if(!strcmp(tag, "Threads:")) {
			info->thread_cnt = content;
		}
	}
	fclose(fp);

	return;
}

static void read_pss(p_info *info)
{
	FILE* fp = NULL;
	char buff[256];
	int pid;
	char pss[64];

	if (!(fp = fopen(USERPROC_PATH,"r"))) {
		ALOGE("Open %s failed!\n",USERPROC_PATH);
		return;
	}

	fseek(fp,0,SEEK_SET);

	while(1) {

                if(!fgets(buff, sizeof(buff), fp))
                         break;
                sscanf(buff, "%d %*s %*s %*s %[^A-Z]",
                                 &pid,
                                 pss);

		if(pid == info->pid) {
			info->pss = atoi(pss);
		}
	}
	fclose(fp);

	return;

}

static void lsof(struct tm tm, int pid)                                                                                   
{
        char lsof_file[MAX_NAME_LEN];

        sprintf(lsof_file,"%s/lsof_%d_%02d%02d%02d.log",LOG_DIR,pid,tm.tm_hour,tm.tm_min,tm.tm_sec);
        ALOGI("Wrote lsof to '%s'\n", lsof_file);
        FILE *fp = fopen(lsof_file,"w");
        fprintf(fp, "\n============  %02d-%02d-%02d %02d:%02d:%02d  ==============\n",
                                tm.tm_year % 100,
                                tm.tm_mon + 1,
                                tm.tm_mday,
                                tm.tm_hour,
                                tm.tm_min, 
				tm.tm_sec);
        print_header(fp);
        lsof_dumpinfo(pid,fp);
	close(fp);
} 

/* Send signal && save logs */
static void handle_info(p_info pinfo, p_info real)
{
	int leak_info = 0;
	char cmd[MAX_PATH_NAME_LEN];
	time_t now = time(NULL);
        struct tm* ptm;

        ptm = localtime(&now);

	if(pinfo.fd_cnt != 0 && pinfo.fd_cnt < real.fd_cnt) { 
		ALOGI("Monitor detect FD leak\n");
		leak_info |= LS_FD_LEAK;
		lsof(*ptm,real.pid);
	}	

	if(pinfo.thread_cnt != 0 && pinfo.thread_cnt < real.thread_cnt) { 
		ALOGI("Monitor detect THREAD leak\n");
		leak_info |= LS_THREAD_LEAK;
		sprintf(cmd,"ps -t|grep %d > %s/thread_%d_%02d%02d%02d.log",real.pid,LOG_DIR,real.pid,ptm->tm_hour,ptm->tm_min,ptm->tm_sec);
		system(cmd);
	}

	if(pinfo.pss != 0 && pinfo.pss < real.pss) { 
		ALOGI("Monitor detect PSS leak\n");
		leak_info |= LS_PSS_LEAK;
		kill(real.pid,SIGTSTP);
	}
	
	if((leak_info > 0)&&(leak_info & LS_PSS_LEAK)) {
		if(monkey_pid > 0)
			kill(monkey_pid,SIGKILL);
	} else if(leak_info > 0) {
		kill(real.pid,SIGKILL);
		if(monkey_pid > 0)
			kill(monkey_pid,SIGKILL);
		}
	return;
}

/*Run through the proc dir, search the inode own to the process */
static void for_each_proc(p_info pinfo[])
{
	DIR *procd;
	struct dirent *pd; 
	p_info real_info;
	int i;
	int ret;

	if( (procd = opendir("/proc"))== NULL){
		ALOGE("cant access /proc\n");
		return;
	}
	while((pd = readdir(procd)) != NULL)
	{
		if(isdigit(pd->d_name[0])){
			ret = read_cmdline(pd->d_name,real_info.name);
			if(!ret)
				continue;
                        if(!strcmp(real_info.name,"com.android.commands.monkey"))
                               monkey_pid = atoi(pd->d_name);
			for(i=0; i < p_cnt; i++) {
				if(!strcmp(real_info.name,pinfo[i].name)) {
					real_info.pid = atoi(pd->d_name);
					read_proc_status(pd->d_name, &real_info);
					real_info.fd_cnt = count_fd(pd->d_name);
					read_pss(&real_info);
					handle_info(pinfo[i],real_info);
				}
			}
		}
	}

	closedir(procd);
	return;
}

static void create_log_dir()
{
	int ret;

	ret = mkdir(LOG_DIR, S_IRWXU | S_IRWXG | S_IRWXO);
	if (-1 == ret && (errno != EEXIST)){
		ALOGE("mkdir %s failed\n",LOG_DIR);
		exit(0);
	}

}

static void parse_config_file(p_info pinfo[])
{

	FILE *fp;
	int i;

	fp = fopen(MONITOR_CONFIG_FILE, "r");
	if(!fp) {
		ALOGE("Err open config file\n");
		exit(0);
	}

	while(fscanf(fp,"%s %d %d %d",
			 pinfo[p_cnt].name,
			 &pinfo[p_cnt].fd_cnt,
			 &pinfo[p_cnt].pss,
			 &pinfo[p_cnt].thread_cnt) != EOF) {

		pinfo[p_cnt].pid = 0;
		if(pinfo[p_cnt].name[0] != '#')
			p_cnt++;
	}
#if 0
	for(i=0; i < p_cnt; i++)
		ALOGD("pinfo: %s,%d,%d,%d\n",
				pinfo[i].name,
				pinfo[i].fd_cnt,
				pinfo[i].pss,
				pinfo[i].thread_cnt);
#endif
	fclose(fp);
	return;
}

void *start_monitor(void *arg)
{
	int p_max = 64;
	int scan_periord = 20;//second
	char value[PROPERTY_VALUE_MAX];

#if 0
	if(argc == 2)
		p_max = atoi(argv[1]);
	if(argc == 3) {
		p_max = atoi(argv[1]);
		scan_periord = atoi(argv[2]);
	}
#endif

	p_info pinfo[p_max];

	pthread_setname_np(pthread_self(),"res_monitor");

	parse_config_file(pinfo);

	create_log_dir();

	while(1) {
		property_get(MONITOR_PROP,value,"");

		if(!strncmp(value,"running",7)) {
			for_each_proc(pinfo);
		}
		sleep(scan_periord);
	}

	return NULL;
}
