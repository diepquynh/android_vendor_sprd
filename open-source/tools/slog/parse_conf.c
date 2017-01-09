/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "slog.h"
#include <cutils/properties.h>
#include "private/android_filesystem_config.h"

/*
#here is the config file example, all fields are split by '\t'
#control: enable/disable
enable

#type   name    value: internal/external
var     logpath internal

#type   name    state   size    level
stream  kernel  on      0       7
stream  system  on      0       8
stream  radio   on      0       0
stream  modem   off     0       5
stream  main    on      0       7
stream  wifi    off     0       7
stream  bt      off     0       7
stream  kmemleak	on	0	7
misc    misc    on      0       5

#type   name            opt     level   inter   action
snap    meminfo         file    6       300     /proc/meminfo
snap    slab            file    7       300     /proc/slabinfo
snap    buddy           file    7       300     /proc/buddyinfo
snap    traces          file    9       0       /data/anr/traces.txt
snap    last_kmsg       file    9       0       /proc/last_kmsg
snap    cmdline         file    9       0       /proc/cmdline
snap    bugreport       cmd     9       0       bugreport
snap    procrank        cmd     9       0       procrank
snap    ps              cmd     9       0       ps -t
snap    apanic_console  file    9       0       /data/dontpanic/apanic_console
snap    apanic_threads  file    9       0       /data/dontpanic/apanic_threads

#type   name            level   file
notify  anr             1       /data/anr
notify  tombstones      1       /data/tombstones
notify  hprofs          1       /data/misc/hprofs/
*/

int enable_kmemleak = 0;
static void handle_kmemleak(const char *state)
{
	if(!strncmp(state, "on", 2))
	{
		err_log("handle_kmemleak: on\n");
		enable_kmemleak = 1;
	}
	else if(!strncmp(state, "off", 3))
	{
		err_log("handle_kmemleak: off\n");
		enable_kmemleak = 0;
	}
	else
	{
	    debug_log("slog cmd kmemleak para error\n");
	}
}

char *parse_string(char *src, char c, char *token)
{
	char *results;
	results = strchr(src, c);
	if(results == NULL) {
		err_log("%s is null!", token);
		return NULL;
	}
	*results++ = 0;
	while(results[0]== c)
		*results++ = 0;
	return results;
}

int parse_3_entries(char *type)
{
	char *name, *pos3;
	
	/* sanity check */
	if(type == NULL) {
		err_log("type is null!");
		return -1;
	}

	if((name = parse_string(type, '\t', "name")) == NULL) return -1;
	if((pos3 = parse_string(name, '\t', "pos3")) == NULL) return -1;

	if(!strncmp(name, "logpath", 7)) {
		if(!strncmp(pos3, "internal", 8)) {
			config_log_path = INTERNAL_LOG_PATH;
			current_log_path = INTERNAL_LOG_PATH;
		} else {
			config_log_path = external_storage;
			current_log_path = external_storage;
		}
	} else if(!strncmp(name, "screenshot", 10)) {
		if(!strncmp(pos3, "enable", 6))
			screenshot_enable = 1;
		else
			screenshot_enable = 0;
	} else if(!strncmp(name, "kmemleak", 8)) {
		handle_kmemleak(pos3);
	}

	return 0;
}
	
int parse_4_entries(char *type)
{
	struct slog_info *info;
	char *name, *pos3, *pos4;

	/* sanity check */
	if(type == NULL) {
		err_log("type is null!");
		return -1;
	}

	if((name = parse_string(type, '\t', "name")) == NULL) return -1;
	if((pos3 = parse_string(name, '\t', "pos3")) == NULL) return -1;
	if((pos4 = parse_string(pos3, '\t', "pos4")) == NULL) return -1;

	info = calloc(1, sizeof(struct slog_info));
	if(info == NULL) {
		err_log("calloc failed!%s",strerror(errno));
		return -1;
	}
	memset(info, 0, sizeof(struct slog_info));

	/* init data structure according to type */
	if(!strncmp(type, "notify", 6)) {
		info->type = SLOG_TYPE_NOTIFY | SLOG_TYPE_MISC;
		info->name = strdup(name);
		info->log_path = strdup("misc");
		info->log_basename = strdup(name);
		info->level = atoi(pos3);
		info->content = strdup(pos4);
		if(info->content[strlen(info->content) - 1] == '\n')
			info->content[strlen(info->content) - 1] = 0;
		if(!notify_log_head)
			notify_log_head = info;
		else {
			info->next = notify_log_head->next;
			notify_log_head->next = info;
		}
		debug_log("type %lu, name %s, %d %s\n",
				info->type, info->name, info->level, info->content);
	}

	return 0;
}

int parse_5_entries(char *type)
{
	struct slog_info *info;
	char *name, *pos3, *pos4, *pos5;

	/* sanity check */
	if(type == NULL) {
		err_log("type is null!");
		return -1;
	}

	/* fetch each field */
	if((name = parse_string(type, '\t', "name")) == NULL) return -1;
	if((pos3 = parse_string(name, '\t', "pos3")) == NULL) return -1;
	if((pos4 = parse_string(pos3, '\t', "pos4")) == NULL) return -1;
	if((pos5 = parse_string(pos4, '\t', "pos5")) == NULL) return -1;

	/* alloc node */
	info = calloc(1, sizeof(struct slog_info));
	if(info == NULL) {
		err_log("calloc failed!%s",strerror(errno));
		return -1;
	}
	memset(info, 0, sizeof(struct slog_info));

	/* init data structure according to type */
	if(!strncmp(type, "stream", 6)) {
		info->type = SLOG_TYPE_STREAM;
		info->name = strdup(name);
		if(!strncmp(info->name, "kernel", 6) ||
			!strncmp(info->name, "cp_wcdma", 8) ||
			!strncmp(info->name, "cp_td-scdma", 8) ||
			!strncmp(info->name, "cp_wcn", 6) ||
			!strncmp(info->name, "cp_td-lte", 8) ||
			!strncmp(info->name, "cp_tdd-lte", 8) ||
			!strncmp(info->name, "cp_fdd-lte", 8) ||
			!strncmp(info->name, "bt", 2) ||
			!strncmp(info->name, "tcp", 3) ||
			!strncmp(info->name, "kmemleak", 8)) {
			info->log_path = strdup(info->name);
		} else {
			info->log_path = strdup("android");
		}
		info->log_basename = strdup(name);
		if(strncmp(pos3, "on", 2))
			info->state = SLOG_STATE_OFF;
		else
			info->state = SLOG_STATE_ON;
		info->max_size = atoi(pos4);
		info->level = atoi(pos5);
		if(!stream_log_head)
			stream_log_head = info;
		else {
			info->next = stream_log_head->next;
			stream_log_head->next = info;
		}
		debug_log("type %lu, name %s, %d %d %d\n",
				info->type, info->name, info->state, info->max_size, info->level);
	} else if(!strncmp(type, "misc", 4)) {
		info->type = SLOG_TYPE_MISC;
		info->name = strdup(name);
		if(!strncmp(pos3, "off", 3))
			info->state = SLOG_STATE_OFF;
		else
			info->state = SLOG_STATE_ON;
		info->max_size = atoi(pos4);
		info->level = atoi(pos5);
		misc_log = info;
		debug_log("type %lu, name %s, %d %d %d\n",
				info->type, info->name, info->state, info->max_size, info->level);
	}
	return 0;
}


int parse_6_entries(char *type)
{
	struct slog_info *info;
	char *name, *pos3, *pos4, *pos5, *pos6;

	/* sanity check */
	if(type == NULL) {
		err_log("type is null!");
		return -1;
	}

	/* fetch each field */
	if((name = parse_string(type, '\t', "name")) == NULL) return -1;
	if((pos3 = parse_string(name, '\t', "pos3")) == NULL) return -1;
	if((pos4 = parse_string(pos3, '\t', "pos4")) == NULL) return -1;
	if((pos5 = parse_string(pos4, '\t', "pos5")) == NULL) return -1;
	if((pos6 = parse_string(pos5, '\t', "pos6")) == NULL) return -1;

	/* alloc node */
	info = calloc(1, sizeof(struct slog_info));
	if(info == NULL) {
		err_log("calloc failed!%s",strerror(errno));
		return -1;
	}
	memset(info, 0, sizeof(struct slog_info));

	/* init data structure according to type */
	if(!strncmp(type, "snap", 4)) {
		info->type = SLOG_TYPE_SNAPSHOT | SLOG_TYPE_MISC;
		info->name = strdup(name);
		info->log_path = strdup("misc");
		info->log_basename = strdup(name);
		info->opt = strdup(pos3);
		info->level = atoi(pos4);
		info->interval = atoi(pos5);
		info->content = strdup(pos6);
		if(info->content[strlen(info->content) - 1] == '\n')
			info->content[strlen(info->content) - 1] = 0;
		if(!snapshot_log_head)
			snapshot_log_head = info;
		else {
			info->next = snapshot_log_head->next;
			snapshot_log_head->next = info;
		}
		debug_log("type %lu, name %s, %s %d %d %s\n",
				info->type, info->name, info->opt, info->level, info->interval, info->content);
	}
	return 0;
}


int gen_config_string(char *buffer)
{
	int off = 0;
	struct slog_info *info;

	off += sprintf(buffer + off, "state: %d\nbackend threads(stream snapshot notify bt tcp modem): %d %d %d %d %d %d\n",
			slog_enable, stream_log_handler_started, snapshot_log_handler_started,
		notify_log_handler_started, bt_log_handler_started, tcp_log_handler_started, iodebug_log_handler_started);
	off += sprintf(buffer + off, "current logpath,%s,\n", current_log_path);
	off += sprintf(buffer + off, "config logpath,%s,\n", config_log_path);
	off += sprintf(buffer + off, "internal storage,%s,\n", INTERNAL_LOG_PATH);
	off += sprintf(buffer + off, "external storage,%s,\n", external_storage);

	info = stream_log_head;
	while(info) {
		off += sprintf(buffer + off, "stream\t%s\t%s\t%d\t%d\n",
				info->name, (info->state == SLOG_STATE_ON) ? "on" : "off", info->max_size, info->level);
		info = info->next;
	}

	off += sprintf(buffer + off, "misc\t%s\t%s\t%d\t%d\n",
		misc_log->name, (misc_log->state == SLOG_STATE_ON) ? "on" : "off", misc_log->max_size, misc_log->level);

	info = snapshot_log_head;
	while(info) {
		off += sprintf(buffer + off, "snap\t%s\t\%s\t%d\t%d\t%s\n",
				info->name, info->opt, info->level, info->interval, info->content);
		info = info->next;
	}

	info = notify_log_head;
	while(info) {
		off += sprintf(buffer + off, "notify\t%s\t%d\t%s\n",
				info->name, info->level, info->content);
		info = info->next;
	}

	return 0;
}

int parse_config()
{
	FILE *fp;
	int ret = 0, count = 0;
	char buffer[MAX_LINE_LEN];
	struct stat st;

	/* we use tmp log config file first */
	if(stat(TMP_SLOG_CONFIG, &st)){
		ret = mkdir(TMP_FILE_PATH, S_IRWXU | S_IRWXG | S_IRWXO);
		if (-1 == ret && (errno != EEXIST)) {
			err_log("mkdir %s failed.%s", TMP_FILE_PATH,strerror(errno));
			exit(0);
		}
		property_get("ro.debuggable", buffer, "");
		if (strcmp(buffer, "1") != 0) {
			if(!stat(DEFAULT_USER_SLOG_CONFIG, &st))
				cp_file(DEFAULT_USER_SLOG_CONFIG, TMP_SLOG_CONFIG);
			else {
				err_log("cannot find config files!%s",strerror(errno));
				exit(0);
			}
		} else {
			if(!stat(DEFAULT_DEBUG_SLOG_CONFIG, &st))
				cp_file(DEFAULT_DEBUG_SLOG_CONFIG, TMP_SLOG_CONFIG);
			else {
				err_log("cannot find config files!%s",strerror(errno));
				exit(0);
			}
		}
	}

	fp = fopen(TMP_SLOG_CONFIG, "r");
	if(fp == NULL) {
		err_log("open file failed, %s.", TMP_SLOG_CONFIG);
		property_get("ro.debuggable", buffer, "");
		if (strcmp(buffer, "1") != 0) {
			fp = fopen(DEFAULT_USER_SLOG_CONFIG, "r");
			if(fp == NULL) {
				err_log("open file failed, %s.%s", DEFAULT_USER_SLOG_CONFIG,strerror(errno));
				exit(0);
			}
		} else {
			fp = fopen(DEFAULT_DEBUG_SLOG_CONFIG, "r");
			if(fp == NULL) {
				err_log("open file failed, %s.%s", DEFAULT_DEBUG_SLOG_CONFIG,strerror(errno));
				exit(0);
			}
		}
	}

	/* parse line by line */
	ret = 0;
	while(fgets(buffer, MAX_LINE_LEN, fp) != NULL) {
		if(buffer[0] == '#')
			continue;
		if(!strncmp("enable", buffer, 6))
			slog_enable = SLOG_ENABLE;
		if(!strncmp("disable", buffer, 7))
			slog_enable = SLOG_DISABLE;
#ifdef LOW_POWER_MODE
		if(!strncmp("low_power", buffer, 7))
			slog_enable = SLOG_LOW_POWER;
#endif
		if(!strncmp("var", buffer, 3))
			ret = parse_3_entries(buffer);
		else if(!strncmp("snap", buffer, 4)) 
			ret = parse_6_entries(buffer);
		else if(!strncmp("stream", buffer, 6))
			ret = parse_5_entries(buffer);
		else if(!strncmp("notify", buffer, 6))
			ret = parse_4_entries(buffer); 
		else if(!strncmp("misc", buffer, 4)) 
                        ret = parse_5_entries(buffer); 
		if(ret != 0) {
			err_log("parse slog.conf return %d. delete the config file reload", ret);
			fclose(fp);
			unlink(TMP_SLOG_CONFIG);
			exit(0);
		}

               count ++;
	}

	fclose(fp);
	if(count < 10) {
		err_log("parse slog.conf failed.  reload");
		unlink(TMP_SLOG_CONFIG);
	}

	return ret;
}
