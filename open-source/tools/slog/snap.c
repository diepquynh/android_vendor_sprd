/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/inotify.h>
#include "private/android_filesystem_config.h"

#include "slog.h"

void record_snap(struct slog_info *info)
{
	struct stat st;
	FILE *fcmd, *fp;
	char buffer[4096];
	char buffer_cmd[MAX_NAME_LEN];
	time_t t;
	struct tm tm;
	int ret;

	/* setup log file first */
	gen_logfile(buffer, info);
	fp = fopen(buffer, "a+");
	if(fp == NULL) {
		err_log("open file %s failed!%s", buffer,strerror(errno));
		exit(0);
	}
	/* add timestamp */
	t = time(NULL);
	localtime_r(&t, &tm);
	fprintf(fp, "\n============  %02d-%02d-%02d %02d:%02d:%02d  ==============\n",
				tm.tm_year % 100,
				tm.tm_mon + 1,
				tm.tm_mday,
				tm.tm_hour,
				tm.tm_min,
				tm.tm_sec);

        if(!strncmp(info->opt, "cmd", 3)) {
		fclose(fp);
		sprintf(buffer_cmd, "%s >> %s", info->content, buffer);
		system(buffer_cmd);
		return;
	}

	fcmd = fopen(info->content, "r");

	if(fcmd == NULL) {
		err_log("open target %s failed!%s", info->content,strerror(errno));
		fclose(fp);
		return;
	}

	/* recording... */
	while( (ret = fread(buffer, 1, 4096, fcmd)) > 0)
		fwrite(buffer, 1, ret, fp);

	fclose(fp);
	fclose(fcmd);

	return;
}

void *snapshot_log_handler(void *arg)
{
	struct slog_info *info;
	int tmp, sleep_secs = 0, next_sleep_secs = 0;

	/* misc_log on/off state will control all snapshot log */
	if(misc_log->state != SLOG_STATE_ON)
		return NULL;

	if(!snapshot_log_head)
		return NULL;

	snapshot_log_handler_started = 1;

	while(slog_enable == SLOG_ENABLE) {
		info = snapshot_log_head;
		while(info) {
			/* misc_log level decided which log will be restored */
			if(info->level > misc_log->level){
				info = info->next;
				continue;
			}

			/* internal equal zero means this log will trigger by user command */
			if(info->interval == 0){
				info = info->next;
				continue;
			}

			/* "state" field unused, so we store pass time at here */
			info->state += sleep_secs;
			if(info->state >= info->interval) {
				info->state = 0;
				record_snap(info);
			}
			tmp = info->interval - info->state;
			if(tmp < next_sleep_secs || !next_sleep_secs)
				next_sleep_secs = tmp;
			info = info-> next;
		}
		/* means no snapshot will be triggered in thread */
		if(!next_sleep_secs)
			break;

		/* update sleep times */
		sleep_secs = next_sleep_secs;
		while(next_sleep_secs-- > 0 && slog_enable == SLOG_ENABLE)
			sleep(1);
		next_sleep_secs = 0;
	}
	snapshot_log_handler_started = 0;

	return NULL;
}

static int capture_snap_for_notify(struct slog_info *head, char *filepath)
{
        struct slog_info *info = head;

        while(info) {
		if(info->level <= 6)
			exec_or_dump_content(info, filepath);
		info = info->next;
        }

        return 0;
}

static int capture_all_for_notify(char *dest_file)
{
	char src_file[MAX_NAME_LEN];
	FILE *fp;

	/* df ps ...... */
	capture_snap_for_notify(snapshot_log_head, dest_file);
#if 0
	/* logcat android log*/
	sprintf(src_file, "logcat -v threadtime -d -f %s", dest_file);
	system(src_file);
#endif
        return 0;
}

void handle_javacrash_file(void)
{
	char dest_file[MAX_NAME_LEN];
	time_t t;
	struct tm tm;
	int ret;

	if(misc_log->state != SLOG_STATE_ON)
		return;

	t = time(NULL);
	localtime_r(&t, &tm);

	sprintf(dest_file, "%s/%s/%s", current_log_path, top_logdir, "misc");
	ret = mkdir(dest_file, S_IRWXU | S_IRWXG | S_IRWXO);
	if (-1 == ret && (errno != EEXIST)) {
		err_log("mkdir %s failed.%s", dest_file,strerror(errno));
		return;
	}

	sprintf(dest_file, "%s/%s/%s/%s", current_log_path, top_logdir, "misc", "javacrash");
	ret = mkdir(dest_file, S_IRWXU | S_IRWXG | S_IRWXO);
	if (-1 == ret && (errno != EEXIST)) {
		err_log("mkdir %s failed.%s", dest_file,strerror(errno));
		return;
	}

	sprintf(dest_file, "%s/%s/%s/%s/snapshot_%02d-%02d-%02d-%02d-%02d.log",
			current_log_path,
			top_logdir,
			"misc",
			"javacrash",
			tm.tm_mon + 1,
			tm.tm_mday,
			tm.tm_hour,
			tm.tm_min,
			tm.tm_sec);

	capture_all_for_notify(dest_file);

	return;

}

static void handle_notify_file(int wd, const char *name)
{
	struct slog_info *info;
	struct stat st;
	char src_file[MAX_NAME_LEN], dest_file[MAX_NAME_LEN];
	time_t t;
	struct tm tm;
	int ret;

	if(misc_log->state != SLOG_STATE_ON)
		return;

	t = time(NULL);
	localtime_r(&t, &tm);

	info = notify_log_head;
	while(info) {
		if(wd != info->interval){
			info = info->next;
			continue;
		}

		gettimeofday(&info->current, NULL);
		if(info->current.tv_sec > info->last.tv_sec + 20) {
			info->last.tv_sec = info->current.tv_sec;
		} else {
			return;
		}

		sprintf(src_file, "%s/%s", info->content, name);
		if(stat(src_file, &st)) {
			err_log("stat %s fail,return,%s",src_file,strerror(errno));
			return;
		}

		/* for collect log */
		sleep(5);

		sprintf(dest_file, "%s/%s/%s", current_log_path, top_logdir, info->log_path);
		ret = mkdir(dest_file, S_IRWXU | S_IRWXG | S_IRWXO);
		if (-1 == ret && (errno != EEXIST)) {
			err_log("mkdir %s failed.%s", dest_file,strerror(errno));
			return;
		}

		sprintf(dest_file, "%s/%s/%s/%s", current_log_path, top_logdir, info->log_path, info->name);
		ret = mkdir(dest_file, S_IRWXU | S_IRWXG | S_IRWXO);
		if (-1 == ret && (errno != EEXIST)) {
			err_log("mkdir %s failed.%s", dest_file,strerror(errno));
			return;
		}

		sprintf(dest_file, "%s/%s/%s/%s/snapshot_%02d-%02d-%02d-%02d-%02d.log",
				current_log_path,
				top_logdir,
				info->log_path,
				info->name,
				tm.tm_mon + 1,
				tm.tm_mday,
				tm.tm_hour,
				tm.tm_min,
				tm.tm_sec);

		/* traces.txt tombstones*/
		cp_file(src_file, dest_file);

		capture_all_for_notify(dest_file);

		return;
	}

	return;
}

void *notify_log_handler(void *arg)
{
	ssize_t size;
	char buffer[MAX_LINE_LEN];
	int notify_fd, wd, len;
	struct slog_info *info;
	struct inotify_event *event;
	int ret;
	fd_set readset;
	int result;
	struct timeval timeout;

	/* misc_log on/off state will control all snapshot log */
	if(misc_log->state != SLOG_STATE_ON)
		return NULL;

	if(!notify_log_head)
		return NULL;

	notify_log_handler_started = 1;

	/* init notify list */
	notify_fd = inotify_init();
	if(notify_fd < 0) {
		err_log("inotify_init() failed!%s",strerror(errno));
		notify_log_handler_started = 0;
		return NULL;
	}

	/* add all watch dir to list */
	info = notify_log_head;
	while(info) {
		/* misc_log level decided which log will be restored */
		if(info->level > misc_log->level) {
			info = info->next;
			continue;
		}
		ret = mkdir(info->content, S_IRWXU | S_IRWXG | S_IXOTH);
		if (-1 == ret && (errno != EEXIST)) {
			err_log("mkdir %s failed.%s", info->content,strerror(errno));
			exit(0);
		}

		ret = chown(info->content, AID_SYSTEM, AID_SYSTEM);
		if (ret < 0) {
			err_log("chown failed.%s",strerror(errno));
			exit(0);
		}

		debug_log("notify add watch %s\n", info->content);
		wd = inotify_add_watch(notify_fd, info->content, IN_MODIFY);
		if(wd == -1) {
			err_log("inotify_add_watch failed!%s",strerror(errno));
			notify_log_handler_started = 0;
			return NULL;
		}
		info->interval = wd;
		info->current.tv_sec = 0;
		info->last.tv_sec = 0;
		info = info->next;
	}

	while(slog_enable == SLOG_ENABLE) {

		FD_ZERO(&readset);
		FD_SET(notify_fd, &readset);
		timeout.tv_sec = 3;
		timeout.tv_usec = 0;
		result = select(notify_fd + 1, &readset, NULL, NULL, &timeout);

		if(result == 0)
			continue;

		if(result < 0) {
			sleep(1);
			continue;
		}

		if(FD_ISSET(notify_fd, &readset) <= 0){
			continue;
                }

		/* when watched dir changed, event will be read */
		size = read(notify_fd, buffer, MAX_LINE_LEN);
		if(size <= 0) {
			debug_log("read inotify fd failed, %d.", (int)size);
			sleep(1);
			continue;
		}
		event = (struct inotify_event *)buffer;
		while(size > 0) {
			len = sizeof(struct inotify_event) + event->len;
			debug_log("notify event: wd: %d, mask %d, ret %d, len %d, file %s\n",
					event->wd, event->mask, (int)size, len, event->name);
			handle_notify_file(event->wd, event->name);
			event = (struct inotify_event *)((char *)event + len);
			size -= len;
		}
	}
	close(notify_fd);
	notify_log_handler_started = 0;
	return NULL;
}
