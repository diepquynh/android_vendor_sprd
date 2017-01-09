#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <string.h>

#ifdef  LOG_TAG
#undef  LOG_TAG
#define LOG_TAG "sprd_res_monitor"
#endif

#define MONITOR_USER_CONFIG "/system/etc/sprd_monitor-user.conf"
#define MONITOR_USERDEBUG_CONFIG "/system/etc/sprd_monitor-userdebug.conf"
#define MONITOR_CONFIG_FILE "/data/local/tmp/sprd_monitor.conf"
#define MAX_FEATURE_CNT 6

extern void *start_monitor(void *arg);
extern void *oprofile_daemon(void* param);

struct {
	char *name;
	char *on;
	char *off;
}monitor_config_info[MAX_FEATURE_CNT] = {
	{
		"sysdump",
		"echo on > /productinfo/sysdump_flag",
		"echo off > /productinfo/sysdump_flag",
	},{
		"coredump",
		"echo /data/corefile/core-%e-%p > /proc/sys/kernel/core_pattern",
		"echo /dev/null > /proc/sys/kernel/core_pattern"
	},{
		"hprofs",
		"mkdir /data/misc/hprofs;chown system:system /data/misc/hprofs",
		"rm -rf /data/misc/hprofs"
	},{
		"hw-watchdog",
		"echo 1 > /sys/module/sprd_wdt_sys/parameters/enabled",
		"echo 0 > /sys/module/sprd_wdt_sys/parameters/enabled"
	},{
		"res-monitor",
		"setprop monitor.ctrl running",
		"setprop monitor.ctrl stopped"
	},{
		"oprofile",
		"setprop debug.oprofile.value 1",
		"setprop debug.oprofile.value 0"
	}
};

void cp_file(char *path, char *new_path)
{
        FILE *fp_src, *fp_dest;
        char buffer[4096];
        int ret;

	umask(0);

        fp_src = fopen(path, "r");
        if(fp_src == NULL) {
                ALOGE("open src file failed!");
                return;
        }

        fp_dest = fopen(new_path, "w");
        if(fp_dest == NULL) {
                ALOGE("open dest file failed!");
                fclose(fp_src);
                return;
        }

        while( (ret = fread(buffer, 1, 4096, fp_src)) > 0)
                fwrite(buffer, 1, ret, fp_dest);
        fflush(fp_dest);

        fclose(fp_src);
        fclose(fp_dest);

        return;
}

static void parse_config()
{

	FILE *fp;
	char f_name[16];
	char f_status[3];
	struct stat tmp;
	char p_value[PROPERTY_VALUE_MAX];
	char *config_file = MONITOR_CONFIG_FILE;
	char *default_config_file;
	char cmd[128];
	int i;

	property_get("ro.debuggable", p_value, "");
	if (strcmp(p_value, "1") != 0) {
		default_config_file = MONITOR_USER_CONFIG;
        } else {
		default_config_file = MONITOR_USERDEBUG_CONFIG;
	}

	if(stat(config_file,&tmp)) {
		if(!stat(default_config_file,&tmp)) {
			cp_file(default_config_file, config_file);
		} else {
			ALOGE("Cant find config file %s\n",default_config_file);
			exit(0);
		}
	}
	fp = fopen(config_file, "r");
	if(!fp) {
                ALOGE("Err open config file %s\n",config_file);
		fp = fopen(default_config_file, "r");
		if(!fp) {
			ALOGE("Err open config file %s\n",default_config_file);
			exit(0);
		}
	}

        while(fscanf(fp,"%s %s",f_name,f_status) != EOF) {
		if(f_name[0] == '#')
			continue;
		for(i = 0; i < MAX_FEATURE_CNT; i++) {
			if(!strcmp(monitor_config_info[i].name, f_name)) {
				if(!strncmp(f_status,"off",3))
					system(monitor_config_info[i].off);
				if(!strncmp(f_status,"on",2))
					system(monitor_config_info[i].on);
			}
		}
	}
	fclose(fp);
	return;
}

static void notify_config_file()
{
	struct inotify_event *event;
	int notify_fd,wd,result,size;
	fd_set readset;
	struct stat tmp;
	char buffer[64];

	if(stat(MONITOR_CONFIG_FILE,&tmp)) {
		return;
	}

	notify_fd = inotify_init();
	wd = inotify_add_watch(notify_fd, MONITOR_CONFIG_FILE, IN_MODIFY);
	if(wd == -1) {
		ALOGE("inotify_add_watch failed!\n");
		return;
	}

        while(1) {
		long unsigned int index = 0;
		FD_ZERO(&readset);
		FD_SET(notify_fd, &readset);
		result = select(notify_fd + 1, &readset, NULL, NULL, NULL);

		if(result <= 0)
			continue;

		size = read(notify_fd, buffer, 64);
		if(size <= 0) {
			ALOGD("read inotify fd failed, %d.\n", (int)size);
			continue;
		}

		while(index < size) {
			event = (struct inotify_event *)((char *)buffer + index);
                        ALOGD("notify event: wd: %d, mask 0x%x, index %d, len %d, file %s\n",
                                        event->wd, event->mask, index, event->len, event->name);
			if(event->mask & IN_MODIFY)
				parse_config();
			if(event->mask & IN_IGNORED) {
				ALOGE("Configfile %s is deleted,leave notify!\n",MONITOR_CONFIG_FILE);
				close(notify_fd);
				return;
			}
			index += sizeof(struct inotify_event) + event->len;
		}
        }
	close(notify_fd);
	return;
}

int main(int argc, char *argv[])
{
	pthread_t tid_monitor;
	pthread_t tid_oprofile_monitor;

	parse_config();

	if(!pthread_create(&tid_monitor,NULL,start_monitor,NULL))
		ALOGD("res_monitor thread created!\n");
	if(!pthread_create(&tid_oprofile_monitor , NULL , oprofile_daemon , NULL))
		ALOGD("oprofile daemon created!");
        else
		ALOGW("oprofile daemon create failed!");

        notify_config_file();

        pthread_join(tid_monitor , NULL);
        pthread_join(tid_oprofile_monitor , NULL);
	return 0;
}
