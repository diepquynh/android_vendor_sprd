/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/uio.h>
#include <dirent.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <cutils/properties.h>
#include <sys/mman.h>

#include "slog.h"

int slog_enable = SLOG_ENABLE;
int cplog_enable = SLOG_ENABLE;
int screenshot_enable = 1;
int slog_start_step = 0;
int slog_reload_flag = 0;
int slog_init_complete = 0;
int stream_log_handler_started = 0;
int kernel_log_handler_started = 0;
int snapshot_log_handler_started = 0;
int notify_log_handler_started = 0;
int bt_log_handler_started = 0;
int tcp_log_handler_started = 0;
int kmemleak_handler_started = 0;
int iodebug_log_handler_started = 0;

#ifdef LOW_POWER_MODE
int hook_modem_flag = 0;
#endif

int internal_log_size = 5 * 1024; /*M*/

char *config_log_path = INTERNAL_LOG_PATH;
char *current_log_path;
char top_logdir[MAX_NAME_LEN];
char external_storage[MAX_NAME_LEN];
char external_path[MAX_NAME_LEN];

struct slog_info *stream_log_head, *snapshot_log_head;
struct slog_info *notify_log_head, *misc_log;

pthread_t stream_tid, kernel_tid, snapshot_tid, notify_tid, sdcard_tid, command_tid, bt_tid, tcp_tid,
kmemleak_tid, iodebug_tid;

static int handle_gms_push(void);
void last_log_rotate();

#ifdef ANDROID_VERSION_442
extern void operate_bt_status(char *status, char* path);
#endif

static void cp_internal_to_sdcard()
{
	DIR *p_dir;
	struct dirent *p_dirent;
	char cmd[MAX_NAME_LEN],in_log[MAX_NAME_LEN];
	int ret;


	sprintf(in_log,"%s/%s/%s",external_storage,top_logdir,"internal_log");
	if(access(in_log,F_OK)){
	ret = mkdir(in_log,S_IRWXU | S_IRWXG | S_IRWXO);
	if(ret < 0){
	err_log("mkdir %s failed,%s",in_log,strerror(errno));
	exit(0);
	}
	else{
	err_log("mkdir %s success",in_log);
	}
	}
	if(( p_dir = opendir(INTERNAL_LOG_PATH)) == NULL) {
		err_log("can not open %s.%s", INTERNAL_LOG_PATH,strerror(errno));
		return;
	}
	while((p_dirent = readdir(p_dir))) {
		if( !strncmp(p_dirent->d_name, "20", 2) )
		{
			sprintf(cmd,"cp -r -f %s/%s %s/%s/%s",INTERNAL_LOG_PATH,p_dirent->d_name,external_storage,top_logdir,"internal_log");
			system(cmd);
		}
	}
	closedir(p_dir);
	return;
}



static int reload(int flush)
{
	err_log("slog reload.");

	if(bt_log_handler_started == 1 )
		bt_snoop_stop();

	if(flush == 1)
		log_buffer_flush();
	property_set("slog.reload", "1");
	kill(getpid(), SIGTERM);
	return 0;
}

static void handler_exec_cmd(struct slog_info *info, char *filepath)
{
	FILE *fp;
	char buffer[MAX_NAME_LEN];
	int ret;
	time_t t;
	struct tm tm;

	fp = fopen(filepath, "a+");
	if(fp == NULL) {
		err_log("open file %s failed!%s", filepath,strerror(errno));
		return;
	}

	/* add timestamp */
	t = time(NULL);
	localtime_r(&t, &tm);
	fprintf(fp, "\n============ %s  %02d-%02d-%02d %02d:%02d:%02d  ==============\n",
			info->log_basename,
			tm.tm_year % 100,
			tm.tm_mon + 1,
			tm.tm_mday,
			tm.tm_hour,
			tm.tm_min,
			tm.tm_sec);

	fclose(fp);
	sprintf(buffer, "%s >> %s", info->content, filepath);
	system(buffer);
	return;
}

static void handler_dump_file(struct slog_info *info, char *filepath)
{
	FILE *fcmd, *fp;
	int ret;
	time_t t;
	struct tm tm;
	char buffer[4096];

	fp = fopen(filepath, "a+");
	if(fp == NULL) {
		err_log("open file %s failed!%s", filepath,strerror(errno));
		return;
	}

	fcmd = fopen(info->content, "r");
	if(fcmd == NULL) {
		err_log("open target %s failed!%s", info->content,strerror(errno));
		fclose(fp);
		return;
	}

	/* add timestamp */
	t = time(NULL);
	localtime_r(&t, &tm);
	fprintf(fp, "\n============ %s  %02d-%02d-%02d %02d:%02d:%02d  ==============\n",
			info->log_basename,
			tm.tm_year % 100,
			tm.tm_mon + 1,
			tm.tm_mday,
			tm.tm_hour,
			tm.tm_min,
			tm.tm_sec);
	/* recording... */
	while( (ret = fread(buffer, 1, 4096, fcmd)) > 0)
		fwrite(buffer, 1, ret, fp);

	fclose(fcmd);
	fclose(fp);

	return;
}

void exec_or_dump_content(struct slog_info *info, char *filepath)
{
	int ret;
	char buffer[MAX_NAME_LEN];

	/* slog_enable on/off state will control all snapshot log */
	if(slog_enable != SLOG_ENABLE)
		return;

	/* misc_log on/off state will control all snapshot log */
	if(misc_log->state != SLOG_STATE_ON)
		return;

	/* setup log file first */
	if( filepath == NULL ) {
		sprintf(buffer, "%s/%s/%s", current_log_path, top_logdir, info->log_path);
		ret = mkdir(buffer, S_IRWXU | S_IRWXG | S_IRWXO);
		if(-1 == ret && (errno != EEXIST)){
			err_log("mkdir %s failed.%s", buffer,strerror(errno));
			exit(0);
		}
		sprintf(buffer, "%s/%s/%s/%s.log",
			current_log_path, top_logdir, info->log_path, info->log_basename);
	} else {
		strcpy(buffer, filepath);
	}

	if(!strncmp(info->opt, "cmd", 3)) {
		handler_exec_cmd(info, buffer);
	} else {
		handler_dump_file(info, buffer);
	}

	return;
}

int capture_by_name(struct slog_info *head, const char *name, char *filepath)
{
	struct slog_info *info = head;

	while(info) {
		if(!strncmp(info->name, name, strlen(name))) {
			exec_or_dump_content(info, filepath);
			return 0;
		}
		info = info->next;
	}
	return 0;
}

static int capture_snap_for_last(struct slog_info *head)
{
	struct slog_info *info = head;
	while(info) {
		if(info->level == 7)
		exec_or_dump_content(info, NULL);
		info = info->next;
	}
	return 0;
}

static int capture_all(struct slog_info *head)
{
	char filepath[MAX_NAME_LEN];
	int ret;
	time_t t;
	struct tm tm;
	struct slog_info *info = head;

	/* slog_enable on/off state will control all snapshot log */
	if(slog_enable != SLOG_ENABLE)
		return 0;

	if(!head)
		return 0;

	sprintf(filepath, "%s/%s/%s", current_log_path, top_logdir, info->log_path);
	ret = mkdir(filepath, S_IRWXU | S_IRWXG | S_IRWXO);
	if(-1 == ret && (errno != EEXIST)) {
		err_log("mkdir %s failed.%s", filepath,strerror(errno));
		exit(0);
	}

	t = time(NULL);
	localtime_r(&t, &tm);
	sprintf(filepath, "%s/%s/%s/snapshot_%02d-%02d-%02d-%02d-%02d.log",
				current_log_path,
				top_logdir,
				info->log_path,
				tm.tm_mon + 1,
				tm.tm_mday,
				tm.tm_hour,
				tm.tm_min,
				tm.tm_sec
	);

	while(info) {
		if(info->level <= 6)
			exec_or_dump_content(info, filepath);
		info = info->next;
	}
	return 0;
}

static void handler_in_last_dir()
{
	DIR *p_dir;
	struct dirent *p_dirent;
	int ret;
	char buffer[MAX_NAME_LEN], top_path[MAX_NAME_LEN];

	memset(top_path, 0, MAX_NAME_LEN);
	if(( p_dir = opendir(current_log_path)) == NULL) {
		err_log("can not open %s.%s", current_log_path,strerror(errno));
		return;
	}

	while((p_dirent = readdir(p_dir))) {
		if( !strncmp(p_dirent->d_name, LAST_LOG, 3) ) {
			sprintf(buffer, "rm -r %s/%s/", current_log_path, LAST_LOG);
			system(buffer);
		} else if( !strncmp(p_dirent->d_name, "20", 2) ) {
			sprintf(top_path, "%s", p_dirent->d_name);

		}

	}

	if( !strncmp(top_path, "20", 2) ) {
		sprintf(buffer, "%s/%s/", current_log_path, LAST_LOG);
		ret = mkdir(buffer, S_IRWXU | S_IRWXG | S_IRWXO);
		if(-1 == ret && (errno != EEXIST)) {
			err_log("mkdir %s failed.%s", buffer,strerror(errno));
			closedir(p_dir);
			exit(0);
		}
		sprintf(buffer, "mv %s/%s %s/%s", current_log_path, top_path, current_log_path, LAST_LOG);
		system(buffer);
	}

	closedir(p_dir);
	return;
}


static void handler_ex_last_dir()
{
	DIR *p_dir;
	struct dirent *p_dirent;
	int ret;
	char buffer[MAX_NAME_LEN], top_path[MAX_NAME_LEN];

	memset(top_path, 0, MAX_NAME_LEN);
	if(( p_dir = opendir(current_log_path)) == NULL) {
		err_log("can not open %s.Error:%s", current_log_path,strerror(errno));
		return;
	}
	while((p_dirent = readdir(p_dir))) {
		if( !strncmp(p_dirent->d_name, "20", 2) ) {
			sprintf(top_path,"%s",p_dirent->d_name);
			sprintf(buffer, "%s/%s/", current_log_path, LAST_LOG);
			ret = mkdir(buffer, S_IRWXU | S_IRWXG | S_IRWXO);
			if(-1 == ret && (errno != EEXIST)) {
				err_log("mkdir %s failed.%s", buffer,strerror(errno));
				closedir(p_dir);
				exit(0);
			}
			sprintf(buffer, "mv %s/%s %s/%s", current_log_path, top_path, current_log_path, LAST_LOG);
			system(buffer);
		}
	}
	closedir(p_dir);
	return;
}

static void handler_last_dir_rename()
{

	DIR *p_dir;
	struct dirent *p_dirent;
	int ret,len,i;
	char buffer[MAX_NAME_LEN];
	i = 0;

	memset(buffer,0,MAX_NAME_LEN);
	sprintf(buffer,"%s/%s",current_log_path,LAST_LOG);
	if((p_dir = opendir(buffer)) == NULL){
		err_log("can't open %s.Error:%s",buffer,strerror(errno));
		return;
	}
	while((p_dirent = readdir(p_dir)))
	{
		if(!strncmp(p_dirent->d_name,"20",2)){
			sprintf(buffer,"mv %s/%s/%s %s/%s/%d-%s",current_log_path,LAST_LOG,p_dirent->d_name,current_log_path,LAST_LOG,i,p_dirent->d_name);
			system(buffer);
		}
	}
	last_log_rotate();
}

void last_log_rotate()
{
	    int i,err,num;
		DIR *p_dir;
		struct dirent *p_dirent;
		char filename[MAX_NAME_LEN],buf[MAX_NAME_LEN],buffer[MAX_NAME_LEN],cmd[128],del_file[MAX_NAME_LEN];

		num = 5;
		sprintf(buffer,"%s/%s",current_log_path,LAST_LOG);
		for(i = MAXROLLNUM_FOR_LAST_LOG;i >= 0;i--){

			if((p_dir = opendir(buffer)) == NULL){
				err_log("can't open %s.Error:%s",buffer,strerror(errno));
				return;
			}
			while((p_dirent = readdir(p_dir))){
				if(strcmp(p_dirent->d_name,".") && strcmp(p_dirent->d_name,"..")){
					sprintf(filename,"%d%s",i,p_dirent->d_name+1);
					if(!strncmp(p_dirent->d_name,filename,1)){
						if(i+1 > MAXROLLNUM_FOR_LAST_LOG){
							sprintf(del_file,"rm -rf %s/%s",buffer,p_dirent->d_name);
							system(del_file);
						}
						else
						{
							sprintf(cmd,"mv %s/%s/%s %s/%s/%d%s",current_log_path,LAST_LOG,p_dirent->d_name,current_log_path,LAST_LOG,i+1,filename+1);
							system(cmd);
						}
					}
				}
			}
			closedir(p_dir);
		}
}




static void handler_modem_memory_log()
{
	char path[MAX_NAME_LEN];
	struct stat st;

	sprintf(path, "%s/modem_memory.log", external_path);
	if(!stat(path, &st)) {
		sprintf(path, "mv %s/modem_memory.log %s/%s/misc/", external_path, current_log_path, top_logdir);
		system(path);
	}
}

static void create_log_dir()
{
	time_t when;
	struct tm start_tm;
	char path[MAX_NAME_LEN];
	char cmd[MAX_NAME_LEN];
	int ret = 0;

	/* generate log dir */
	when = time(NULL);
	localtime_r(&when, &start_tm);
	sprintf(top_logdir, "20%02d-%02d-%02d-%02d-%02d-%02d",
						start_tm.tm_year % 100,
						start_tm.tm_mon + 1,
						start_tm.tm_mday,
						start_tm.tm_hour,
						start_tm.tm_min,
						start_tm.tm_sec);
	sprintf(path, "%s/%s", current_log_path, top_logdir);
	ret = mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO);
	if (-1 == ret && (errno != EEXIST)) {
		err_log("mkdir %s failed.%s", path,strerror(errno));
		sprintf(cmd, "rm -r %s", current_log_path);
		system(cmd);
		exit(0);
	}

	property_set("slog.top_logdir",top_logdir );
	err_log("top_logdir is %s\n",top_logdir);
	return;
}

static void use_ori_log_dir()
{
	DIR *p_dir;
	struct dirent *p_dirent;
	int log_num = 0;
	char cmd[MAX_NAME_LEN],value[PROPERTY_VALUE_MAX];

	if(( p_dir = opendir(current_log_path)) == NULL) {
		err_log("Can't open %s.%s", current_log_path,strerror(errno));
		sprintf(cmd, "rm -r %s", current_log_path);
		system(cmd);
		exit(0);
	}

	property_get("slog.top_logdir",value,"1970-01-01-01-08-00-00");
	while((p_dirent = readdir(p_dir))) {
		if( !strncmp(p_dirent->d_name, value, 2) ) {
			memset(top_logdir, 0, MAX_NAME_LEN);
			strncpy(top_logdir, p_dirent->d_name, MAX_NAME_LEN-1);
			log_num = 1;
			err_log("%s\n",top_logdir);
			break;
		}
	}

	if(log_num == 0)
		create_log_dir();

	closedir(p_dir);
	return;
}

#ifdef LOW_POWER_MODE
static void handle_low_power()
{
	if(slog_enable != SLOG_LOW_POWER)
		return;
}
#endif

static int start_sub_threads()
{
	int ret;

	if(slog_enable != SLOG_ENABLE)
		return 0;

	if(!stream_log_handler_started) {
		ret = pthread_create(&stream_tid, NULL, stream_log_handler, NULL);
		if(ret < 0) {
			err_log("create stream thread failed.%s",strerror(errno));
			exit(0);
		}
	}
	if(!kernel_log_handler_started) {
		ret = pthread_create(&kernel_tid, NULL, kernel_log_handler, NULL);
		if(ret < 0) {
			err_log("create kernel thread failed.%s",strerror(errno));
			exit(0);
		}
	}
	if(!snapshot_log_handler_started) {
		ret = pthread_create(&snapshot_tid, NULL, snapshot_log_handler, NULL);
		if(ret < 0) {
			err_log("create snapshot thread failed.%s",strerror(errno));
			exit(0);
		}
	}
	if(!notify_log_handler_started) {
		ret = pthread_create(&notify_tid, NULL, notify_log_handler, NULL);
		if(ret < 0) {
			err_log("create notify thread failed.%s",strerror(errno));
			exit(0);
		}
	}

	if(!bt_log_handler_started) {
		ret = pthread_create(&bt_tid, NULL, bt_snoop_start, NULL);
		if(ret < 0) {
			err_log("create bt socket accept thread failed.%s",strerror(errno));
			exit(0);
		}
	}
	if(!tcp_log_handler_started) {
		ret = pthread_create(&tcp_tid, NULL, tcp_log_handler, NULL);
		if(ret < 0) {
			err_log("create tcp thread failed.%s",strerror(errno));
			exit(0);
		}
	}
	if(!kmemleak_handler_started) {
		ret = pthread_create(&kmemleak_tid, NULL, kmemleak_handler, NULL);
		if(ret < 0) {
			err_log("create kmemleak thread failed.%s",strerror(errno));
			exit(0);
		}
	}
	if(!iodebug_log_handler_started) {
		ret = pthread_create(&iodebug_tid, NULL, iodebug_log_handler, NULL);
		if(ret < 0) {
			err_log("create iodebug thread failed.");
			exit(0);
		}
	}
	return 0;
}

static void init_external_storage()
{
	char *p;
	int type;
	char cmd[256];
	char value[PROPERTY_VALUE_MAX];
	int ret;

	memset(external_path, 0, MAX_NAME_LEN);

	property_get("vold.sdcard0.path", value, "3");

	strncpy(external_path, value, MAX_NAME_LEN-1);
	sprintf(external_storage, "%s/slog", external_path);
	err_log("the external storage : %s", external_storage);
	return;
}

static int sdcard_mounted()
{
	FILE *str;
	char buffer[MAX_LINE_LEN];
	char value[PROPERTY_VALUE_MAX];
	int type,ret;
	char *p;

	property_get("vold.sdcard0.state",value,"-1");
	if(!strncmp(value,"mounted",7))
		return 1;

		return 0;
}

static void check_available_volume()
{
	DIR *p_dir,*p_dir1;
	struct dirent *p_dirent,*p_dirent1;
	struct statfs diskInfo;
	char cmd[MAX_NAME_LEN],cmd1[MAX_NAME_LEN];
	char buffer[MAX_NAME_LEN],buffer1[MAX_NAME_LEN];
	char str[128],str1[128];
	unsigned int ret;
	int i,j,num;
	int del_flag = 1;
	num = MAXROLLNUM_FOR_LAST_LOG;

	if(slog_enable != SLOG_ENABLE)
		return;

	if(!strncmp(current_log_path, external_storage, strlen(external_storage))) {
check:      if( statfs(external_path, &diskInfo) < 0 ) {
				err_log("statfs %s return err!Error:%s", external_path,strerror(errno));
				return;
			}
			sprintf(buffer,"%s/%s",current_log_path,LAST_LOG);
			ret = diskInfo.f_bavail * diskInfo.f_bsize >> 20;
			if(ret > 0 && ret < 200) {
				if(del_flag == 0){
					err_log("sdcard available %d M, send broadcast,sleep 5s,flush buffer,then set slog status:%d", ret,SLOG_DISABLE);
					sprintf(cmd, "%s %d", "am startservice -a slogui.intent.action.LOW_VOLUME --ei freespace ", ret);
					system(cmd);
					sleep(5);
					log_buffer_flush();
					slog_enable = SLOG_DISABLE;
				}
				else{
					for(i = num;i>= 1;i--){
						sprintf(str,"%d",i);
						if((p_dir = opendir(buffer)) == NULL){
							err_log("can not open %s.%s",buffer,strerror(errno));
							return;
						}
						while((p_dirent = readdir(p_dir))){
							if(!strncmp(p_dirent->d_name,str,1)){
								if(!strncmp(p_dirent->d_name,"1",1)){
									sprintf(buffer1,"%s/%s/%s",buffer,p_dirent->d_name,"android");
									for(j = MAXROLLLOGS_FOR_AP;j > 0;j--){
										sprintf(str1,"%d",j);
										if((p_dir1 = opendir(buffer1))==NULL){
											err_log("can not open %s.%s",buffer1,strerror(errno));
											return;
										}
										while((p_dirent1 = readdir(p_dir1))){
											if(!strncmp(p_dirent1->d_name,str1,1)){
												sprintf(cmd1,"rm -rf %s/%s",buffer1,p_dirent1->d_name);
												err_log("cmd1:%s,str1:%s,p_dirent->d_name:%s",cmd1,str1,p_dirent->d_name);
												system(cmd1);
												closedir(p_dir1);
												goto check;
											}   }
										if(ret > 0 && ret < 200) {
											err_log("just leave the latest log,sdcard available %dM, disable slog", ret);
											slog_enable = SLOG_DISABLE;
											sprintf(cmd, "%s %d", "am startservice -a slogui.intent.action.LOW_VOLUME --ei freespace ", ret);
											system(cmd);
										} }
								}else{
									sprintf(cmd,"rm -rf %s/%s",buffer,p_dirent->d_name);
									err_log("cmd:%s",cmd);
									system(cmd);
									closedir(p_dir);
									num = i-1;
									goto check;
								} } }   }   }
			} else {
				if( statfs(current_log_path, &diskInfo) < 0 ) {
					err_log("statfs %s return err!Error:%s", current_log_path,strerror(errno));
					return;
				}

				ret = diskInfo.f_bavail * diskInfo.f_bsize >> 20;
				if(ret > 0 && ret < 10) {
					err_log("internal available  is not enough,left %d M, send broadcast,sleep 5s,flush buffer,then set slog status:%d", ret,SLOG_DISABLE);
					sprintf(cmd, "%s %d", "am startservice -a slogui.intent.action.LOW_VOLUME --ei freespace ", ret);
					system(cmd);
					sleep(5);
					log_buffer_flush();
					slog_enable = SLOG_DISABLE;
				}
			}
	}

}

int clear_all_log()
{
	char cmd[MAX_NAME_LEN];

#ifdef ANDROID_VERSION_442
	if(bt_log_handler_started == 1 )
		operate_bt_status("false", NULL);
#endif
	slog_enable = SLOG_DISABLE;
	sleep(4);
	sprintf(cmd, "rm -r %s/*", INTERNAL_LOG_PATH);
	system(cmd);
	sprintf(cmd, "rm -r %s", external_storage);
	system(cmd);
	reload(0);
	return 0;
}

int dump_all_log(const char *name)
{
	char cmd[MAX_NAME_LEN];
	char buffer0[MAX_NAME_LEN];
	char buffer1[MAX_NAME_LEN];
	char buffer2[MAX_NAME_LEN];
	int ret;

    if(!strncmp(current_log_path ,INTERNAL_LOG_PATH, strlen(INTERNAL_LOG_PATH)))
	        return -1;
			capture_all(snapshot_log_head);
			capture_by_name(snapshot_log_head, "getprop", NULL);
			if (0 != access("/system/xbin/busybox",F_OK)){
				err_log("/system/xbin/busybox %s",strerror(errno));
				sprintf(buffer0,"%s/%s/%s",external_path,name,"external_log");
				sprintf(buffer1,"%s/%s/%s",external_path,name,"internal_log");
				if (0 == access(buffer0,F_OK)){
					sprintf(buffer2,"rm -rf %s",buffer0);
					ret = system(buffer2);
					if(-1 == ret){
						err_log("mkdir %s failed.Error:%s.exit(0)",buffer0,strerror(errno));
						return -1;
					}
				}
				else{
					sprintf(buffer2,"mkdir -p %s",buffer0);
					ret = system(buffer2);
					if (-1 == ret ){
						err_log("mkdir %s failed.Error:%s,exit(0)",buffer0,strerror(errno));
						exit(0);
					}
				}
				sprintf(cmd, "cp -r %s/%s %s",external_path,"slog",buffer0);
				system(cmd);
				if (0 == access(buffer1,F_OK)){
					sprintf(buffer2,"rm -rf %s",buffer1);
					ret = system(buffer2);
					if(-1 == ret){
						err_log("remove %s failed.Error:%s",buffer1,strerror(errno));
						return -1;
					}
				}
				else{
					sprintf(buffer2,"mkdir -p %s",buffer1);
					ret = system(buffer2);
					if (-1 == ret){
						err_log("mkdir %s failed.Error:%s.exit(0)",buffer1,strerror(errno));
						exit(0);
					}
				}
				sprintf(cmd, "cp -r %s %s",INTERNAL_LOG_PATH,buffer1);
			}else{
	sprintf(cmd, "busybox tar czf %s/%s -C %s %s %s", external_path, name, external_path, "slog", INTERNAL_LOG_PATH);
}
return system(cmd);
}

/* monitoring sdcard status thread */
static void *monitor_sdcard_fun()
{
	while( !strncmp (config_log_path, external_storage, strlen(external_storage))) {
		if(sdcard_mounted()) {
			if(slog_start_step == 0)
				sleep(TIMEOUT_FOR_SD_MOUNT);
			reload(1);
		}
		sleep(TIMEOUT_FOR_SD_MOUNT);
	}
	return 0;
}

/*
 *handler log size according to internal available space
 *
 */
static void handler_internal_log_size()
{
	struct statfs diskInfo;
	unsigned int internal_availabled_size;
	int ret;
	char cmd[MAX_NAME_LEN];

	if( strncmp(current_log_path, INTERNAL_LOG_PATH, strlen(INTERNAL_LOG_PATH)))
		return;

	if(access(current_log_path,F_OK) != 0){
	ret = mkdir(current_log_path, S_IRWXU | S_IRWXG | S_IRWXO);
	if(-1 == ret && (errno != EEXIST)) {
		err_log("mkdir %s failed.%s", current_log_path,strerror(errno));
		sprintf(cmd, "rm -r %s", current_log_path);
		system(cmd);
		exit(0);
	}
	}

	if( statfs(current_log_path, &diskInfo) < 0) {
		slog_enable = SLOG_DISABLE;
		err_log("statfs %s return err, disable slog.%s", current_log_path,strerror(errno));
		return;
	}
	internal_availabled_size = diskInfo.f_bavail * diskInfo.f_bsize / 1024;
	if( internal_availabled_size < 10 ) {
		slog_enable = SLOG_DISABLE;
		err_log("internal available space %dM is not enough, disable slog", internal_availabled_size);
		return;
	}

	/* setting internal log size = (available size - 5M) * 80% */
	internal_log_size = ( internal_availabled_size - 5 * 1024 ) / 5 * 4 / 12;
	err_log("set internal log size %dKB", internal_log_size);

	return;
}

/*
 * handle uboot log.
 *
 */

static void handle_uboot_log(void)
{
	int fd = -1;
	FILE *fout = NULL;
	void *va = NULL;
	char buffer[MAX_LINE_LEN];
	char *s1, *s2;
	unsigned long pa_start = 0;
	unsigned long data_sz = 0;
	int ret;

	/* misc_log on/off state will control all snapshot log */
	if(misc_log->state != SLOG_STATE_ON)
		return;

	debug_log("Start dump uboot log..");

	/* Check cmdline to find the location of uboot log */
	if ((fd = open("/proc/cmdline", O_RDONLY)) == -1) {
		err_log("open proc file cmdline error!");
		return;
	}

	ret = read(fd, buffer, sizeof(buffer) -1);
	if (ret < 0) {
		err_log("read cmdline error");
		close(fd);
		return;
	}

	/* parse address and size, format: boot_ram_log=0x..., 0x...*/
	if ((s1 = strstr(buffer, "boot_ram_log=")) != NULL) {
		s2= strsep(&s1, "="); /*0x=...*/
		if ((s2 = strsep(&s1, ",")) != NULL) {
			pa_start = strtoll(s2, NULL, 16);
			if ((s2 = strsep(&s1, " ")) != NULL)
				data_sz = strtoll(s2, NULL, 16);
		}
	}
	else {
		err_log("can not get boot_ram_log flag in cmdline,%s",strerror(errno));
		close(fd);
		return;
	}

	close(fd);

	debug_log("boot_ram_log base=%#010x, size=%#x", (unsigned int)pa_start, (unsigned int)data_sz);

	/* map memory region from physcial address */
        if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) {
                err_log("could not open /dev/mem/,%s",strerror(errno));
                return;
        }

        va = (char*)mmap(0, data_sz, PROT_READ | PROT_WRITE,
                        MAP_SHARED, fd, pa_start);
        if (va == MAP_FAILED) {
                err_log("can not map region!%s",strerror(errno));
                close(fd);
                return;
        }

	sprintf(buffer, "%s/%s/misc/uboot.log",
			current_log_path,
			top_logdir);

	if ((fout = fopen(buffer, "w")) == NULL) {
		err_log("create log file error!%s",strerror(errno));
		close(fd);
		return;
	}

	ret = fwrite(va, 1, data_sz, fout);
	debug_log("%#x bytes were written to file.", ret);
/*
	if (munmap(va, data_sz) == MAP_FAILED) {
		err_log("munmap failed!");
	}
*/
	close(fd);

	fclose(fout);

	return;
}

/*
 * handle dropbox
 *
 */
static void handle_dropbox()
{
	char cmd[MAX_NAME_LEN];
	sprintf(cmd, "busybox tar czf %s/%s/dropbox.tgz /data/system/dropbox", current_log_path, top_logdir);
	system(cmd);
}

/*
 * handle top_logdir
 *
 */
static void handle_top_logdir()
{
	char cmd[MAX_NAME_LEN];
	int ret;

	if(slog_enable != SLOG_ENABLE)
		return;

	if(access(current_log_path,F_OK) != 0){
	ret = mkdir(current_log_path, S_IRWXU | S_IRWXG | S_IRWXO);
	if(-1 == ret && (errno != EEXIST)) {
		err_log("mkdir %s failed.%s", current_log_path,strerror(errno));
		sprintf(cmd, "rm -r %s", current_log_path);
		system(cmd);
		exit(0);
	}
	}

	if( !strncmp(current_log_path, INTERNAL_LOG_PATH, strlen(INTERNAL_LOG_PATH))) {
		err_log("slog use internal storage");
		switch(slog_start_step){
		case 0:
			handler_in_last_dir();
			create_log_dir();
			capture_snap_for_last(snapshot_log_head);
			handle_uboot_log();
			property_set("slog.step", "1");
			break;
		default:
			use_ori_log_dir();
			break;
		}
	} else {
		err_log("slog use external storage");
		switch(slog_start_step){
		case 1:
			handler_ex_last_dir();
			handler_last_dir_rename();
			create_log_dir();
			capture_snap_for_last(snapshot_log_head);
			handle_uboot_log();
			handle_dropbox();
			cp_internal_to_sdcard();
			property_set("slog.step", "2");
			break;
		default:
			use_ori_log_dir();
			cp_internal_to_sdcard();
			break;
		}
	}
}

/*
 *  monitoring sdcard status
 */
static int start_monitor_sdcard_fun()
{
	/* handle sdcard issue */
	if(!strncmp(config_log_path, external_storage, strlen(external_storage))) {
		if(!sdcard_mounted())
			current_log_path = INTERNAL_LOG_PATH;
		else {
			/*avoid can't unload SD card*/
			if(slog_reload_flag == 0)
				sleep(TIMEOUT_FOR_SD_MOUNT);
			current_log_path = external_storage;
		}
		/* create a sdcard monitor thread */
		if(slog_enable != SLOG_ENABLE)
			return 0;
		if(!strncmp(current_log_path, INTERNAL_LOG_PATH, strlen(INTERNAL_LOG_PATH)))
			pthread_create(&sdcard_tid, NULL, monitor_sdcard_fun, NULL);
	} else
		current_log_path = INTERNAL_LOG_PATH;

	return 0;

}

/*
 * 1.start running slog system(stream,snapshot)
 * 2.monitoring sdcard status
 */
static void do_init()
{
	if(slog_enable != SLOG_ENABLE)
		return;

	handler_internal_log_size();

	handle_top_logdir();

	capture_all(snapshot_log_head);
	/* all backend log capture handled by follows threads */
	start_sub_threads();

	slog_init_complete = 1;

	return;
}

void *handle_request(void *arg)
{
	int ret, client_sock;
	struct slog_cmd cmd;
	char filename[MAX_NAME_LEN];
	time_t t;
	struct tm tm;

	client_sock = * ((int *) arg);
	ret = recv_socket(client_sock, (void *)&cmd, sizeof(cmd));
	if(ret <  0) {
		err_log("recv data failed!");
		close(client_sock);
		return 0;
	}
	if(cmd.type == CTRL_CMD_TYPE_RELOAD) {
		cmd.type = CTRL_CMD_TYPE_RSP;
		sprintf(cmd.content, "OK");
		send_socket(client_sock, (void *)&cmd, sizeof(cmd));
		close(client_sock);
		reload(1);
		return 0;
	}

	switch(cmd.type) {
	case CTRL_CMD_TYPE_SNAP:
		ret = capture_by_name(snapshot_log_head, cmd.content, NULL);
		break;
	case CTRL_CMD_TYPE_SNAP_ALL:
		ret = capture_all(snapshot_log_head);
		break;
	case CTRL_CMD_TYPE_EXEC:
		//err_log("slog %s.", cmd.content);
		//system(cmd.content);
		break;
	case CTRL_CMD_TYPE_GMS:
		ret = handle_gms_push();
		if (ret == 0) {
			sprintf(cmd.content, "All done. Successful.\n");
		} else {
			sprintf(cmd.content, "May failed in some steps. All done.");
		}
		send_socket(client_sock, (void *)&cmd, sizeof(cmd));
		break;
	case CTRL_CMD_TYPE_ON:
		/* not implement */
		ret = -1;
		break;
	case CTRL_CMD_TYPE_OFF:
		slog_enable = SLOG_DISABLE;
		sleep(3);
		break;
	case CTRL_CMD_TYPE_QUERY:
		log_buffer_flush();
		ret = gen_config_string(cmd.content);
		break;
	case CTRL_CMD_TYPE_SYNC:
		log_buffer_flush();
		ret = 0;
		break;
	case CTRL_CMD_TYPE_BT_FALSE:
		operate_bt_status("false", NULL);
		ret=0;
		break;
	case CTRL_CMD_TYPE_CLEAR:
		ret = clear_all_log();
		break;
	case CTRL_CMD_TYPE_DUMP:
		ret = dump_all_log(cmd.content);
		break;
	case CTRL_CMD_TYPE_JAVACRASH:
		handle_javacrash_file();
		ret = 0;
		break;
#ifdef LOW_POWER_MODE
	case CTRL_CMD_TYPE_HOOK_MODEM:
		ret = mkdir(HOOK_MODEM_TARGET_DIR, S_IRWXU | S_IRWXG | S_IRWXO);
		if (-1 == ret && (errno != EEXIST)){
			err_log("mkdir /data/log failed.");
		}
		ret = 0;
		hook_modem_flag = 1;
		break;
#endif
	case CTRL_CMD_TYPE_SCREEN:
		if(slog_enable != SLOG_ENABLE || slog_init_complete == 0||screenshot_enable==0)
			break;
		if(cmd.content[0]){
			sprintf(filename, "screencap -p %s.png", cmd.content);
			system(filename);
			ret = 0;
		}
		else {
			sprintf(filename, "%s/%s/misc", current_log_path, top_logdir);
			ret = mkdir(filename, S_IRWXU | S_IRWXG | S_IRWXO);
			if(-1 == ret && (errno != EEXIST)){
				err_log("mkdir %s failed.%s", filename,strerror(errno));
				exit(0);
			}
			t = time(NULL);
			localtime_r(&t, &tm);
			sprintf(filename, "screencap -p %s/%s/misc/screencap-%02d-%02d-%02d-%02d-%02d.png",
					current_log_path,
					top_logdir,
					tm.tm_mon + 1,
					tm.tm_mday,
					tm.tm_hour,
					tm.tm_min, tm.tm_sec);
			system(filename);
			ret = 0;
		}
		break;
	default:
		err_log("wrong cmd cmd: %d %s.", cmd.type, cmd.content);
		break;
	}
	cmd.type = CTRL_CMD_TYPE_RSP;
	if(ret && cmd.content[0] == 0)
		sprintf(cmd.content, "FAIL");
	else if(!ret && cmd.content[0] == 0)
		sprintf(cmd.content, "OK");
	send_socket(client_sock, (void *)&cmd, sizeof(cmd));
	close(client_sock);

	return 0;
}

void *command_handler()
{
	struct sockaddr_un serv_addr;
	int ret, server_sock, client_sock;
	pthread_t thread_pid;

restart_socket:
	/* init unix domain socket */
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sun_family=AF_UNIX;
	strcpy(serv_addr.sun_path, SLOG_SOCKET_FILE);
	unlink(serv_addr.sun_path);

	server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
        if (server_sock < 0) {
		err_log("create socket failed!");
		sleep(2);
		goto restart_socket;
	}

	if (bind(server_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		err_log("bind socket failed!");
		close(server_sock);
		sleep(2);
		goto restart_socket;
	}

	if (listen(server_sock, 5) < 0) {
		err_log("listen socket failed!");
		close(server_sock);
		sleep(2);
		goto restart_socket;
	}

	while(1) {
		client_sock = accept(server_sock, NULL, NULL);
		if (client_sock < 0) {
			err_log("accept failed!");
			sleep(1);
			continue;
		}
		if ( 0 != pthread_create(&thread_pid, NULL, handle_request, (void *) &client_sock) )
			err_log("sock thread create error");
	}
}

static void sig_handler1(int sig)
{
	err_log("get a signal %d, exit(0).", sig);
	exit(0);
}

static void sig_handler2(int sig)
{
	err_log("get a signal %d, lgnore.", sig);
	return;
}

/*
 * setup_signals - initialize signal handling.
 */
static void setup_signals()
{
	struct sigaction act;

	memset(&act, 0, sizeof(act));
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

#define SIGNAL(s, handler)      do { \
		act.sa_handler = handler; \
		if (sigaction(s, &act, NULL) < 0) \
			err_log("Couldn't establish signal handler (%d): %m", s); \
	} while (0)

	SIGNAL(SIGTERM, sig_handler1);
	SIGNAL(SIGBUS, sig_handler1);
	SIGNAL(SIGSEGV, sig_handler1);
	SIGNAL(SIGHUP, sig_handler1);
	SIGNAL(SIGQUIT, sig_handler1);
	SIGNAL(SIGABRT, sig_handler1);
	SIGNAL(SIGILL, sig_handler1);

	SIGNAL(SIGFPE, sig_handler2);
	SIGNAL(SIGPIPE, sig_handler2);
	SIGNAL(SIGALRM, sig_handler2);

	return;
}

static void handle_slog_property(void)
{
	char value[PROPERTY_VALUE_MAX];

	property_get("slog.step", value, "0");
	slog_start_step = atoi(value);

	property_get("slog.reload", value, "0");
	slog_reload_flag = atoi(value);
	property_set("slog.reload", "0");

	err_log("slog.step = %d, slog.reload = %d", slog_start_step, slog_reload_flag);
}

static int handle_gms_push(void)
{
		// Handle push command, verify has been passed. Deamon do actions
		system("mount -o remount /system");

		// Copy file, if we lose some folders, please added as follows.
		system("cp -r /sdcard/gms/system/priv-app/*.apk /system/priv-app/.");
		// Mode in child dir must be changed, if not package manager may has
		// no permissions to touch.
		system("chmod -R 0644 /system/priv-app/");
		// Remove the side effect of chmod -R 
		system("chmod 0755 /system/priv-app");

		system("cp -r /sdcard/gms/system/app/*.apk /system/app/.");
		system("chmod -R 0644 /system/app/");
		system("chmod 0755 /system/app/");

		system("cp -r /sdcard/gms/system/framework/*.jar /system/framework/.");
		system("chmod -R 0644 /system/framework/");
		system("chmod 0755 /system/framework/");

		system("cp -r /sdcard/gms/system/etc/permissions/*.xml /system/etc/permissions/.");
		system("chmod -R 0644 /system/etc/permissions/");
		system("chmod 0755 /system/etc/permissions/");

		system("cp -r /sdcard/gms/system/lib/*.so /system/lib/.");
		system("chmod 0644 /system/lib/*");
		system("chmod 0755 /system/lib/*");

		system("cp -r /sdcard/gms/system/tts/ /system/");
		system("chmod -R 0644 /system/tts/");
		system("chmod 0755 /system/tts/");
		// If we forget some dirs, please add as follow
		return 0;
}

/*
 * the main function
 */
int main()
{
	err_log("slog begin to work.");

	/* sets slog process's file mode creation mask */
	umask(0);

	/* handle signal */
	setup_signals();

	/* even backend capture threads disabled, we also accept user command */
	pthread_create(&command_tid, NULL, command_handler, NULL);

	/* set slog_start_step form property.*/
	handle_slog_property();

	/* read and parse config file */
	parse_config();

	/*find the external storage environment*/
	init_external_storage();

	/*start monitor sdcard*/
	start_monitor_sdcard_fun();

	/* backend capture threads started here */
	do_init();
#ifdef LOW_POWER_MODE
	handle_low_power();
#endif
	while(1) {
		sleep(5);
		log_buffer_flush();
		check_available_volume();
	}

	return 0;
}
