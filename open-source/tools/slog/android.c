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
#include <cutils/properties.h>
#include <cutils/sockets.h>
#include "private/android_filesystem_config.h"

#include "slog.h"

#ifdef ANDROID_VERSION_442
#include <log/log.h>
#include <log/log_read.h>
#include <log/logger.h>
#include <log/logd.h>
#include <log/logprint.h>
#include <log/event_tag_map.h>
#else
#include <cutils/logger.h>
#include <cutils/logd.h>
#include <cutils/logprint.h>
#include <cutils/event_tag_map.h>
#endif

/*
 *
 *
 */
static char* mstrncpy(char* dest, const char* src, size_t pos)
{
	strncpy(dest, src, pos);
	dest += pos;
	return dest;
}

/*
 * get current time and return strings
 *
 */
static void obtime(char *src, int len)
{
	time_t when;
	struct tm* tm;
	struct tm tmBuf;
	struct timeval tv;
	int ret;

	/* add timestamp */
	when = time(NULL);
	gettimeofday(&tv, NULL);
	tm = localtime_r(&when, &tmBuf);
	ret = strftime(src, len, "%m-%d %H:%M:%S", tm);
	sprintf(src + ret, ".%03d ", tv.tv_usec / 1000);
	return;
}

/*
 * search character '\n' in dest, insert "src" before all of '\n' into "dest"
 *
 */
static void strinst(char* dest, char* src)
{
	char *pos1, *pos2, *p;
	char time[MAX_NAME_LEN];

	p = dest;
	pos2 = src;

	obtime(time, sizeof(time));
	p = mstrncpy(p, time, strlen(time));
	pos1 = strchr(pos2, '\n');

	while(pos1 != NULL){
		p = mstrncpy(p, pos2, pos1 - pos2 + 1);
		if(strchr(pos1 + 1, '\n') == NULL)
			break;
		p = mstrncpy(p, time, strlen(time));
		pos2 = pos1 + 1;
		pos1 = strchr(pos2, '\n');
	}

	return;
}

/*
 * add timestamp when start logging.
 */
static void add_timestamp(struct slog_info *info)
{
	char buffer[MAX_NAME_LEN];
	int ret;
	time_t t;
	struct tm tm;
	t = time(NULL);
	localtime_r(&t, &tm);
	sprintf(buffer, "\n============  %02d-%02d-%02d %02d:%02d:%02d  ==============\n",
				tm.tm_year % 100,
				tm.tm_mon + 1,
				tm.tm_mday,
				tm.tm_hour,
				tm.tm_min,
				tm.tm_sec);
	fwrite(buffer, strlen(buffer), 1, info->fp_out);

	return;
}

static void add_flag_when_lost(struct slog_info *info)
{
	char buffer[MAX_NAME_LEN];
	sprintf(buffer, "\n## APMessageLost, retry ##\n");
	fwrite(buffer, strlen(buffer), 1, info->fp_out);

	return;
}

static void filterPriToStr (int pri, char *PriTag)
{
	switch (pri) {
	case ANDROID_LOG_VERBOSE:
		sprintf(PriTag, "*:V");
		break;
	case ANDROID_LOG_DEBUG:
		sprintf(PriTag, "*:D");
		break;
	case ANDROID_LOG_INFO:
		sprintf(PriTag, "*:I");
		break;
	case ANDROID_LOG_WARN:
		sprintf(PriTag, "*:W");
		break;
	case ANDROID_LOG_ERROR:
		sprintf(PriTag, "*:E");
		break;
	case ANDROID_LOG_FATAL:
		sprintf(PriTag, "*:F");
		break;
	case ANDROID_LOG_SILENT:
		sprintf(PriTag, "*:S");
		break;
	case ANDROID_LOG_DEFAULT:
	case ANDROID_LOG_UNKNOWN:
	default:
		sprintf(PriTag, "*:D");
	}

	return;
}

void *stream_log_handler(void *arg)
{
	struct slog_info *info;
	int ret, err, retry;
	char buf[LOGGER_ENTRY_MAX_LEN+1], buf_kmsg[LOGGER_ENTRY_MAX_LEN];
	struct logger_entry *entry;
	AndroidLogEntry entry_write;
	static AndroidLogPrintFormat format;
	char devname[MAX_NAME_LEN];
	struct timeval timeout;

	static AndroidLogFormat * g_logformat;
	static EventTagMap* g_eventTagMap = NULL;
	char defaultBuffer[512];
	char *outBuffer = NULL;
	size_t totalLen;
	char tagName[8];

	struct logger_list *logger_list;
	int mode = O_RDONLY;
	struct log_msg log_msg;

	logger_list = android_logger_list_alloc(mode, 0, 0);

	stream_log_handler_started = 1;

	info = stream_log_head;
	/*open all of the stream devices*/
	while(info){
		if(info->state != SLOG_STATE_ON){
			info = info->next;
			continue;
		}
		if( !strncmp(info->name, "main", 4) ) {
			android_logger_open(logger_list, android_name_to_log_id(info->name));
			info->fp_out = gen_outfd(info);
			add_timestamp(info);
			filterPriToStr(info->level, tagName);
		} else if( !strncmp(info->name, "system", 6) ) {
			android_logger_open(logger_list, android_name_to_log_id(info->name));
			info->fp_out = gen_outfd(info);
			add_timestamp(info);
		} else if( !strncmp(info->name, "radio", 5) ) {
			android_logger_open(logger_list, android_name_to_log_id(info->name));
			info->fp_out = gen_outfd(info);
			add_timestamp(info);
		} else if( !strncmp(info->name, "crash", 5) ) {
			android_logger_open(logger_list, android_name_to_log_id(info->name));
			info->fp_out = gen_outfd(info);
			add_timestamp(info);
		} else if( !strncmp(info->name, "events", 6) ) {
			android_logger_open(logger_list, android_name_to_log_id(info->name));
			info->fp_out = gen_outfd(info);
			add_timestamp(info);
			g_eventTagMap = android_openEventTagMap(EVENT_TAG_MAP_FILE);
			if(g_eventTagMap == NULL)
				info->state = SLOG_STATE_OFF;
		} else {
			info = info->next;
			continue;
		}

		info = info->next;
	}

	/*Initialize android log format*/
	g_logformat = android_log_format_new();
	format = android_log_formatFromString("threadtime");
	android_log_setPrintFormat(g_logformat, format);
	android_log_addFilterString(g_logformat, tagName);

	while(slog_enable == SLOG_ENABLE) {

		ret = android_logger_list_read(logger_list, &log_msg);

		if (ret == 0) {
			err_log("read: Unexpected EOF!");
			exit(0);
		}

		if (ret < 0) {
			if (ret == -EAGAIN) {
				break;
			}

			if (ret == -EIO) {
				err_log("read: Unexpected EOF!");
				exit(0);
			}

			if (ret == -EINVAL) {
				err_log("read: unexpected length.");
				exit(0);
			}

			err_log("Read android log failure,%s",strerror(errno));
			return NULL;
		}

		info = stream_log_head;
		while(info) {

			if(info->state != SLOG_STATE_ON){
				info = info->next;
				continue;
			}

			if(!strncmp(info->name, "main", 4) || !strncmp(info->name, "system", 6)  || !strncmp(info->name, "crash", 5)
				|| !strncmp(info->name, "radio", 5) || !strncmp(info->name, "events", 6) ) {

				if (android_name_to_log_id(info->name) != log_msg.entry.lid) {
					info = info->next;
					continue;
				}
				/*add android log 'tag' and other format*/
				if ( !strncmp(info->name, "events", 6) )
					err = android_log_processBinaryLogBuffer(&(log_msg.entry_v1), &entry_write,
						g_eventTagMap, buf_kmsg, sizeof(buf_kmsg));
				else
					err = android_log_processLogBuffer(&(log_msg.entry_v1), &entry_write);
				if ( err < 0 ) {
					info = info->next;
					continue;
				}

				if (android_log_shouldPrintLine(g_logformat, entry_write.tag, entry_write.priority)) {
					/* write log to file */
					outBuffer = android_log_formatLogLine(g_logformat, defaultBuffer,
						sizeof(defaultBuffer), &entry_write, &totalLen);
					if (!outBuffer) {
							info = info->next;
							continue;
					}
					if (access(info->file_path, W_OK)) {
						err_log("%s can't be write,%s", info->file_path,strerror(errno));
						fclose(info->fp_out);
						info->fp_out = NULL;
					    return NULL;
					}
					retry = 0;
retry2:
					ret = fwrite(outBuffer, totalLen, 1, info->fp_out);
					if ( ret != 1 ) {
						fclose(info->fp_out);
						sleep(1);
						add_flag_when_lost(info);
						info->fp_out = gen_outfd(info);
						if(retry++ < 5)
							goto retry2;
					} else {
						info->outbytecount += totalLen;
						log_size_handler(info);
					}
					if(retry==5){
						err_log("fwrite error,exit!\n");
						exit(0);
					}
					if (outBuffer != defaultBuffer)
						free(outBuffer);
				}
			}

			info = info->next;
		}
	}

	android_closeEventTagMap(g_eventTagMap);
	stream_log_handler_started = 0;

	return NULL;
}


void *kernel_log_handler(void *arg)
{
	struct slog_info *info;
	int ret, retry;
	char buf_kmsg[LOGGER_ENTRY_MAX_LEN], wbuf_kmsg[LOGGER_ENTRY_MAX_LEN *2];
	struct timeval timeout;

	kernel_log_handler_started = 1;

	info = stream_log_head;
	/*open all of the stream devices*/
	while(info){
		if(info->state != SLOG_STATE_ON){
			info = info->next;
			continue;
		}

		if(!strncmp(info->name, "kernel", 6)) {
			info->fd_device = open_device(info, KERNEL_LOG_SOURCE);
			info->fp_out = gen_outfd(info);
			add_timestamp(info);
			break;
		} else {
			info = info->next;
			continue;
		}
	}

	if(info == NULL)
		return NULL;

	while(slog_enable == SLOG_ENABLE) {
		memset(buf_kmsg, 0, LOGGER_ENTRY_MAX_LEN);
		memset(wbuf_kmsg, 0, LOGGER_ENTRY_MAX_LEN *2);
		ret = read(info->fd_device, buf_kmsg, LOGGER_ENTRY_MAX_LEN -1);
		if(ret <= 0) {
			if ( (ret == -1 && (errno == EINTR || errno == EAGAIN) ) || ret == 0 ) {
				sleep(1);
				continue;
			}
			err_log("read %s log failed!", info->name);
			close(info->fd_device);
			sleep(1);
			info->fd_device = open_device(info, KERNEL_LOG_SOURCE);
			continue;
		}
		buf_kmsg[ret] = '\0';
		strinst(wbuf_kmsg, buf_kmsg);
		if (access(info->file_path, W_OK)) {
			err_log("%s does not exist", info->file_path);
			fclose(info->fp_out);
			info->fp_out = NULL;
			exit(0);
		}
		retry = 0;
retry1:
		ret = fwrite(wbuf_kmsg, strlen(wbuf_kmsg), 1, info->fp_out);
		if ( ret != 1 ) {
			err_log("fwrite failed.");
			fclose(info->fp_out);
			sleep(1);
			add_flag_when_lost(info);
			info->fp_out = gen_outfd(info);
			if(retry++ < 5)
				goto retry1;
		} else {
			info->outbytecount += strlen(wbuf_kmsg);
			log_size_handler(info);
		}
		if(retry==5){
			err_log("fwrite error,exit!\n");
			exit(0);
		}

	}

	kernel_log_handler_started = 0;

	return NULL;
}


#define		KMEMLEAK_SCAN_CMD	"scan"
#define		KMEMLEAK_SCANOFF_CMD	"scan=off"
#define		KMEMLEAK_CLEAR_CMD	"clear"

extern int enable_kmemleak;
static int auto_scan_off = 0;
void *kmemleak_handler(void *arg)
{
	int i = 0;
	int retry = 0;
	int retry_open = 0;
	int ret;
	int n_read, n_write;
	char buf[1024];
	int fd_ml = 0;
	char buffer[MAX_NAME_LEN];
	int fd_dmp;
	struct slog_info *info = NULL,*kmemleak = NULL;

	info = stream_log_head;
	while(info){
		if((info->state == SLOG_STATE_ON) && !strncmp(info->name, "kmemleak", 8)) {
			kmemleak = info;
			break;
		}
		info = info->next;
	}
	if(kmemleak == NULL)
		return NULL;

	kmemleak_handler_started = 1;

	while(slog_enable == SLOG_ENABLE)
	{
		if(!enable_kmemleak)
		{
			debug_log("kmemleak disable\n");
			sleep(60);
			continue;
		}

		fd_ml = open("/sys/kernel/debug/kmemleak", O_RDWR);
label0:
		if(fd_ml == -1)
		{
			err_log("open device error, errno=%d\n", errno);
			if(errno == ENOENT)
			{
				retry_open++;
				sleep(20);
				if(retry_open == 3)
				{
					err_log("need open kconfig support\n");
					break;
				}
				continue;
			}
			else
			{
				break;
			}
		}
		retry_open = 0;
		if(!auto_scan_off)
		{
			write(fd_ml, KMEMLEAK_SCANOFF_CMD, strlen(KMEMLEAK_SCANOFF_CMD));
			auto_scan_off = 1;
		}
		n_write = write(fd_ml, KMEMLEAK_CLEAR_CMD, strlen(KMEMLEAK_CLEAR_CMD));
		if(n_write <= 0){
			err_log("write device clear cmd error, need rewrite, errno=%d\n", errno);
			if(errno == EBUSY)
			{
				retry++;
				close(fd_ml);
				sleep(1);
				if(retry == 4)
				{
					err_log("device always busy, exit\n");
					break;
				}
				fd_ml = open("/sys/kernel/debug/kmemleak", O_RDWR);
				goto label0;
			}
		}
		retry = 0;

		n_write = write(fd_ml, KMEMLEAK_SCAN_CMD, strlen(KMEMLEAK_SCAN_CMD));
		if(n_write <= 0){
			err_log("write device error, need rewrite, errno=%d\n", errno);
			if(errno == EBUSY)
			{
				retry++;
				close(fd_ml);
				sleep(1);
				if(retry == 4)
				{
					err_log("device always busy, exit\n");					
					break;
				}
				fd_ml = open("/sys/kernel/debug/kmemleak", O_RDWR);
				goto label0;
			}
	    }
	    retry = 0;
label1:
		n_read = read(fd_ml, buf, 1024);
		if(n_read > 0)
		{
			memset(buffer, 0, MAX_NAME_LEN);
			sprintf(buffer, "%s/%s/%s", current_log_path, top_logdir, kmemleak->log_path);
			ret = mkdir(buffer, S_IRWXU | S_IRWXG | S_IRWXO);
			if (-1 == ret && (errno != EEXIST)){
				err_log("mkdir %s failed.%s\n", buffer,strerror(errno));
				close(fd_ml);				
				return NULL;
			}
			sprintf(buffer, "%s/%s/%s/%s_%d", current_log_path, top_logdir, kmemleak->log_path, kmemleak->log_basename, i);
			fd_dmp = open((const char *)buffer, O_CREAT|O_RDWR, 0x666);
			write_from_buffer(fd_dmp, buf, n_read);
			while(n_read)
			{
label2:
				n_read = read(fd_ml, buf, 1024);
				if(n_read > 0)
				{
					write_from_buffer(fd_dmp, buf, n_read);
				}
				else if(n_read < 0)
				{
					err_log("1:fd=%d read %d is lower than 0\n", fd_ml, n_read);
					sleep(1);
					goto label2;
				}
			}
			if(fd_dmp >= 0)close(fd_dmp);
		}
		else if(n_read < 0)
		{
			err_log("2:fd=%d read %d is lower than 0\n", fd_ml, n_read);
			sleep(1);
			goto label1;
		}
		close(fd_ml);
		fd_ml = 0;
		sleep(15*60);
		i++;
	}
	kmemleak_handler_started = 0;
	return NULL;
}
