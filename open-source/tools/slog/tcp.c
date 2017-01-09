/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "private/android_filesystem_config.h"

#include "slog.h"

void *tcp_log_handler(void *arg)
{
	struct slog_info *info = NULL, *tcp = NULL;
	char buffer[MAX_NAME_LEN];
	int ret, i, err;
	pid_t pid;

	info = stream_log_head;
	while(info){
		if((info->state == SLOG_STATE_ON) && !strncmp(info->name, "tcp", 2)) {
			tcp = info;
			break;
		}
		info = info->next;
	}

	if( !tcp)
		return NULL;

	if(!strncmp(current_log_path, INTERNAL_LOG_PATH, strlen(INTERNAL_LOG_PATH))) {
		tcp->state = SLOG_STATE_OFF;
		return NULL;
	}

	tcp_log_handler_started = 1;
	pid = fork();
	if(pid < 0){
		err_log("fork error!");
	}

	if(pid == 0){
		gen_logfile(buffer, info);
		ret = execl("/system/xbin/tcpdump", "tcpdump", "-i", "any", "-p", "-s 0", "-w", buffer, (char *)0);
		if(ret < 0){
		err_log("ret = %d,Error:%s,child process exit",ret,strerror(errno));
		}
		else{
		err_log("ret = %d,child process exit",ret);
		}
		exit(0);
	}

	while(slog_enable == SLOG_ENABLE){
		sleep(1);
	}

	kill(pid, SIGTERM);
	tcp_log_handler_started = 0;

	return NULL;
}


