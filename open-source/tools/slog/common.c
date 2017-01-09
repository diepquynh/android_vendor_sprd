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
#include <sys/time.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <cutils/sockets.h>
#include <cutils/properties.h>
#include "private/android_filesystem_config.h"

#include "slog.h"

int send_socket(int sockfd, void* buffer, int size)
{
        int result = -1;
        int ioffset = 0;
        while(sockfd > 0 && ioffset < size) {
                result = send(sockfd, (char *)buffer + ioffset, size - ioffset, MSG_NOSIGNAL);
                if (result > 0) {
                        ioffset += result;
                } else {
                        break;
                }
        }
        return result;

}

int recv_socket(int sockfd, void* buffer, int size)
{
        int received = 0, result;
        while(buffer && (received < size))
        {
                result = recv(sockfd, (char *)buffer + received, size - received, MSG_NOSIGNAL);
                if (result > 0) {
                        received += result;
                } else {
                        received = result;
                        break;
                }
        }
        return received;
}

#define W_TIME "/cache/w_timesyncfifo"
#define TD_TIME "/cache/td_timesyncfifo"
#define L_TIME "/cache/l_timesyncfifo"

int get_timezone()
{
	time_t time_utc;
	struct tm tm_local, tm_gmt;
	int time_zone;

	time_utc = time(NULL);
	localtime_r(&time_utc, &tm_local);
	gmtime_r(&time_utc, &tm_gmt);
	time_zone = tm_local.tm_hour - tm_gmt.tm_hour;
	if (time_zone < -12) {
		time_zone += 24;
	} else if (time_zone > 12) {
		time_zone -= 24;
	}

	err_log("UTC: %02d-%02d-%02d %02d:%02d:%02d",
				tm_gmt.tm_year % 100,
				tm_gmt.tm_mon + 1,
				tm_gmt.tm_mday,
				tm_gmt.tm_hour,
				tm_gmt.tm_min,
				tm_gmt.tm_sec);

	err_log("LOCAL: %02d-%02d-%02d %02d:%02d:%02d",
				tm_local.tm_year % 100,
				tm_local.tm_mon + 1,
				tm_local.tm_mday,
				tm_local.tm_hour,
				tm_local.tm_min,
				tm_local.tm_sec);

	return time_zone;
}

void write_modem_timestamp(struct slog_info *info, char *buffer)
{
	int fd, ret, retry_count = 0;
	FILE *fp;
	int time_zone;
	struct modem_timestamp *mts;
        char cp_time[MAX_NAME_LEN];

        memset(cp_time, '0', MAX_NAME_LEN);
        if (!strncmp(info->name, "cp_wcdma", 8)) {
                strcpy(cp_time, W_TIME);
        } else if (!strncmp(info->name, "cp_td-scdma", 8)) {
                strcpy(cp_time, TD_TIME);
        } else if (!strncmp(info->name, "cp_td-lte", 8)) {
                strcpy(cp_time, L_TIME);
        } else if (!strncmp(info->name, "cp_tdd-lte", 8)) {
                strcpy(cp_time, L_TIME);
		} else
                return;

	mts = calloc(1, sizeof(struct modem_timestamp));
	retry_count=5;
	while(retry_count){
		fd = open(cp_time, O_RDWR);
		if( fd < 0 ){
			if(errno == EINTR || errno == EAGAIN) {
				retry_count --;
				sleep(1);
			}else{
				err_log("Unable to open time stamp device '%s',%s", cp_time,strerror(errno));
				free(mts);
				return;
				exit(0);
			}
		}else
			break;
	}
	ret = read(fd, (char*)mts + 4, 12);
	if(ret < 12) {
		close(fd);
		free(mts);
		return;
	}
	close(fd);

	mts->magic_number = 0x12345678;
	time_zone = get_timezone();
	mts->tv.tv_sec += time_zone * 3600;
	err_log("%lx, %lx, %lx, %lx", mts->magic_number, mts->tv.tv_sec, mts->tv.tv_usec, mts->sys_cnt);

	fp = fopen(buffer, "a+b");
	if(fp == NULL) {
		err_log("open file %s failed!", buffer);
		free(mts);
		return;
	}
	fwrite(mts, sizeof(struct modem_timestamp), 1, fp);
	fclose(fp);

	free(mts);
}

#define MODEM_VERSION "gsm.version.baseband"

void write_modem_version(struct slog_info *info)
{
	char buffer[MAX_NAME_LEN];
	char modem_property[MAX_NAME_LEN];
	FILE *fp;
	int ret;

	if (strncmp(info->name, "cp", 2)) {
		return;
	}
	memset(modem_property, '0', MAX_NAME_LEN);
	property_get(MODEM_VERSION, modem_property, "not_find");
	if(!strncmp(modem_property, "not_find", 8)) {
		err_log("%s not find.", MODEM_VERSION);
		return;
	}

	sprintf(buffer, "%s/%s/%s/%s.version", current_log_path, top_logdir, info->log_path, info->log_basename);
	fp = fopen(buffer, "w+");
	if(fp == NULL) {
		err_log("open file %s failed!", buffer);
		return;
	}
	fwrite(modem_property, strlen(modem_property), 1, fp);
	fclose(fp);

}

void gen_logpath(char *filename, struct slog_info *info)
{
	int ret;

	sprintf(filename, "%s/%s/%s", current_log_path, top_logdir, info->log_path);
	ret = mkdir(filename, S_IRWXU | S_IRWXG | S_IRWXO);
	if (-1 == ret && (errno != EEXIST)){
                err_log("mkdir %s failed.%s", filename,strerror(errno));
                exit(0);
	}
}

void gen_logfile(char *filename, struct slog_info *info)
{
	int ret;
	int retry_count=5;
	char buffer[MAX_NAME_LEN];
	DIR *p_dir;
	struct dirent *p_dirent;
	time_t when;
	struct tm start_tm;

	sprintf(filename, "%s/%s/%s", current_log_path, top_logdir, info->log_path);
	sprintf(buffer, "0-%s", info->log_basename);
	ret = mkdir(filename, S_IRWXU | S_IRWXG | S_IRWXO);
	if (-1 == ret && (errno != EEXIST)){
                err_log("mkdir %s failed.%s", filename,strerror(errno));
                exit(0);
	}
	while(retry_count){
		if(( p_dir = opendir(filename)) == NULL) {
			if(errno==EINTR||errno==EAGAIN){
				sleep(1);
				retry_count--;
			}else{
				err_log("can not open %s.%s", filename,strerror(errno));
				exit(0);
			}
		}else
			break;
	}
	while((p_dirent = readdir(p_dir))) {
		if( !strncmp(p_dirent->d_name, buffer, strlen(buffer)) ) {
			sprintf(filename, "%s/%s/%s/%s", current_log_path, top_logdir, info->log_path, p_dirent->d_name);
			closedir(p_dir);
			return;
		}
	}
	when = time(NULL);
	localtime_r(&when, &start_tm);
	if( !strncmp(info->name, "tcp", 3))
		sprintf(filename, "/%s/%s/%s/0-%s-%02d-%02d-%02d-%02d-%02d.pcap",
						current_log_path,
						top_logdir,
						info->log_path,
						info->log_basename,
						start_tm.tm_mon + 1,
						start_tm.tm_mday,
						start_tm.tm_hour,
						start_tm.tm_min,
						start_tm.tm_sec);
	else
		sprintf(filename, "/%s/%s/%s/0-%s-%02d-%02d-%02d-%02d-%02d.log",
						current_log_path,
						top_logdir,
						info->log_path,
						info->log_basename,
						start_tm.tm_mon + 1,
						start_tm.tm_mday,
						start_tm.tm_hour,
						start_tm.tm_min,
						start_tm.tm_sec);



	closedir(p_dir);
	return;
}

void cp_file(char *path, char *new_path)
{
	FILE *fp_src, *fp_dest;
	char buffer[4096];
	int ret;

	fp_src = fopen(path, "r");
	if(fp_src == NULL) {
		err_log("open src file failed!");
		return;
	}
	fp_dest = fopen(new_path, "w");
	if(fp_dest == NULL) {
		err_log("open dest file failed!");
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

/*
 * write form buffer.
 *
 */
int write_from_buffer(int fd, char *buf, int len)
{
	int result = 0, err = 0;

	if(buf == NULL || fd < 0)
		return -1;

	if(len <= 0)
		return 0;

	while(result < len) {
		err = write(fd, buf + result, len - result);
		if(err < 0 && errno == EINTR)
			continue;
		if(err < 0)
			return err;
		result += err;
	}
	return result;

}

/*
 * open log devices
 *
 */
int open_device(struct slog_info *info, char *path)
{
	int retry_count = 5;
	int fd;
	while(retry_count){
		fd = open(path, O_RDWR);
		if(fd < 0){
			if(errno==EINTR||errno==EAGAIN){
				sleep(1);
				retry_count--;
			}else{
				err_log("Unable to open log device '%s'.%s", path,strerror(errno));
				exit(0);
			}
		}else
			break;
	}
	return fd;
}

/*
 * open output file
 *
 */
FILE *gen_outfd(struct slog_info *info)
{
	int cur;
	int retry_count=5;
	FILE *fp;
	char buffer[MAX_NAME_LEN];

	gen_logfile(buffer, info);
	write_modem_version(info);
	write_modem_timestamp(info, buffer);
	while(retry_count){
		fp = fopen(buffer, "a+b");
		if(fp == NULL){
			if(errno==EINTR||errno==EAGAIN){
				sleep(1);
				retry_count--;
			}else{
				err_log("Unable to open file %s.%s",buffer,strerror(errno));
				exit(0);
			}
		}else
			break;
	}
	if(info->setvbuf == NULL)
		info->setvbuf = malloc(SETV_BUFFER_SIZE);

	setvbuf(fp, info->setvbuf, _IOFBF, SETV_BUFFER_SIZE);
	info->outbytecount = ftell(fp);

	info->file_path = strdup(buffer);

	return fp;
}

/*
 * The file name to upgrade
 */
void file_name_rotate(struct slog_info *info, int num, char *buffer)
{
	int i, err;
	DIR *p_dir;
	struct dirent *p_dirent;
	char filename[MAX_NAME_LEN], buf[MAX_NAME_LEN],seg[MAX_NAME_LEN],newname[MAX_NAME_LEN];
	char *p;
	time_t when;
	struct tm end_tm;

	for (i = num; i >= 0 ; i--) {
		char *file0, *file1;

		if(( p_dir = opendir(buffer)) == NULL) {
			err_log("can not open %s.", buffer);
			return;
		}
		sprintf(filename, "%d-%s", i, info->log_basename);
		while((p_dirent = readdir(p_dir))) {
			if( !strncmp(p_dirent->d_name, filename, strlen(filename)) ) {
				err = asprintf(&file1, "%s/%s/%s/%s", current_log_path, top_logdir, info->log_path, p_dirent->d_name);
				if(err == -1){
					err_log("asprintf return err!%s",strerror(errno));
					exit(0);
				}
				if (i + 1 > num) {
					remove(file1);
					free(file1);
				} else {
					sprintf(filename, "%s", p_dirent->d_name);
					if(i != 0){
						err = asprintf(&file0, "%s/%s/%s/%d%s",
								current_log_path, top_logdir, info->log_path, i + 1, filename + 1 + i/10);
						if(err == -1) {
							err_log("asprintf return err!%s",strerror(errno));
							exit(0);
						}
					}
					else{
					sprintf(seg,"%d%s",i+1,filename+1+i/10);
					p = strtok(seg,".");
					when = time(NULL);
					localtime_r(&when,&end_tm);
					sprintf(newname,"%s%s%02d-%02d-%02d-%02d-%02d.log",p,"~",
							end_tm.tm_mon + 1,
							end_tm.tm_mday,
							end_tm.tm_hour,
							end_tm.tm_min,
							end_tm.tm_sec);
					err_log("*******newname:%s******",newname);
					err = asprintf(&file0, "%s/%s/%s/%s",
						current_log_path, top_logdir, info->log_path, newname);
					if(err == -1) {
						err_log("asprintf return err!%s",strerror(errno));
						exit(0);
					}
					}
					err = rename (file1, file0);
					if (err < 0 && errno != ENOENT) {
						perror("while rotating log files");
					}

					free(file1);
					free(file0);
				}
			}
		}

		closedir(p_dir);
	}
}

/*
 *  File volume
 *
 *  When the file is written full, rename file to file.1
 *  and rename file.1 to file.2, and so on.
 */
void rotatelogs(int num, struct slog_info *info)
{
	char buffer[MAX_NAME_LEN];

	err_log("slog rotatelogs");
	fclose(info->fp_out);
	gen_logpath(buffer, info);
	file_name_rotate(info, num, buffer);
	info->fp_out = gen_outfd(info);
	info->outbytecount = 0;
}

/*
 * Handler log file size according to the configuration file.
 *
 *
 */
void log_size_handler(struct slog_info *info)
{
	if( !strncmp(current_log_path, INTERNAL_LOG_PATH, strlen(INTERNAL_LOG_PATH)) ) {
		if(info->outbytecount >= internal_log_size * 1024) {
			rotatelogs(INTERNAL_ROLLLOGS, info);
		}
		return;
	}

	if (!strncmp(info->name, "cp", 2)) {
		if(info->outbytecount >= DEFAULT_LOG_SIZE_CP * 1024 * 1024)
			rotatelogs(MAXROLLLOGS_FOR_CP, info);
	} else {
		if(info->outbytecount >= DEFAULT_LOG_SIZE_AP * 1024 * 1024)
			rotatelogs(MAXROLLLOGS_FOR_AP, info);
	}
}

void log_buffer_flush(void)
{
	struct slog_info *info;

	if(slog_enable != SLOG_ENABLE)
		return;

	info = stream_log_head;
	while(info){
		if(info->state != SLOG_STATE_ON){
			info = info->next;
			continue;
		}

		if(info->fp_out != NULL)
			fflush(info->fp_out);

		info = info->next;
	}

	return;
}
