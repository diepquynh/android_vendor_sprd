/*
 * Copyright (C) 2012 Spreadtrum Communications Inc.
 *
 */

#ifndef _SLOG_H
#define _SLOG_H

#define LOG_TAG "SLOG"

#include <errno.h>
#include <pthread.h>
#include <utils/Log.h>

#define ANDROID_VERSION_442
#define LOW_POWER_MODE

/* "type" field */
#define SLOG_TYPE_STREAM	0x1
#define SLOG_TYPE_MISC		(0x1 << 1)
#define SLOG_TYPE_SNAPSHOT	(0x1 << 2)
#define SLOG_TYPE_NOTIFY	(0x1 << 3)

/* "state" field */
#define SLOG_STATE_ON 0
#define SLOG_STATE_OFF 1

/*"slog state" field*/
#ifdef LOW_POWER_MODE
#define SLOG_LOW_POWER 2
#endif

#define SLOG_ENABLE 1
#define SLOG_DISABLE 0

/* "opt" field */
#define SLOG_EXEC_OPT_EXEC	0

/* socket control command type */
enum {
	CTRL_CMD_TYPE_RELOAD,
	CTRL_CMD_TYPE_SNAP,
	CTRL_CMD_TYPE_SNAP_ALL,
	CTRL_CMD_TYPE_EXEC,
	CTRL_CMD_TYPE_ON,
	CTRL_CMD_TYPE_OFF,
	CTRL_CMD_TYPE_QUERY,
	CTRL_CMD_TYPE_CLEAR,
	CTRL_CMD_TYPE_DUMP,
	CTRL_CMD_TYPE_SCREEN,
	CTRL_CMD_TYPE_SYNC,
	CTRL_CMD_TYPE_BT_FALSE,
#ifdef LOW_POWER_MODE
	CTRL_CMD_TYPE_HOOK_MODEM,
#endif
	CTRL_CMD_TYPE_JAVACRASH,
	CTRL_CMD_TYPE_GMS,
	CTRL_CMD_TYPE_RSP
};

#define err_log(fmt, arg...) ALOGE("%s: " fmt " [%d]\n", __func__, ## arg, errno);
/*#define debug_log(fmt, arg...) ALOGD("%s: " fmt, __func__, ## arg);*/
#define debug_log(fmt, arg...)

#define INTERNAL_LOG_PATH		"/data/slog"
#define DEFAULT_DEBUG_SLOG_CONFIG	"/system/etc/slog.conf"
#define DEFAULT_USER_SLOG_CONFIG	"/system/etc/slog.conf.user"
#define TMP_FILE_PATH			"/data/local/tmp/slog/"
#define SLOG_SOCKET_FILE		TMP_FILE_PATH "slog_sock"
#define TMP_SLOG_CONFIG			TMP_FILE_PATH "slog.conf"
#define DEFAULT_DUMP_FILE_NAME		"slog.tgz"
#define FB_DEV_NODE			"/dev/graphics/fb0"

#define MAX_NAME_LEN			128
#define MAX_LINE_LEN			2048
#define DEFAULT_LOG_SIZE_AP		256 /* MB */
#define DEFAULT_LOG_SIZE_CP		512 /* MB */
#define MAXROLLNUM_FOR_LAST_LOG 9
#define MAXROLLLOGS_FOR_AP		19
#define MAXROLLLOGS_FOR_CP		100
#define INTERNAL_ROLLLOGS		1
#define TIMEOUT_FOR_SD_MOUNT		10 /* seconds */
#define BUFFER_SIZE			(32 * 1024) /* 32k */
#define SETV_BUFFER_SIZE		(512 * 1024) /* 512k */

#define KERNEL_LOG_SOURCE		"/proc/kmsg"
#define MODEM_LOG_SOURCE		"/dev/vbpipe0"

/* handler last log dir */
#define LAST_LOG 			"last_log"

/* main data structure */
struct slog_info {
	struct slog_info	*next;

	/* log type */
	unsigned long	type;

	/* control log on/off */
	int		state;

	/* for different logs, level give several meanings */
	int		level;

	/* control snapshot record */
	int		interval;

	/* used for "snapshot" type */
	char		*opt;

	/* log file size limit */
	int		max_size;

	/* identify a certain log entry, must uniq */
	char		*name;

	/* define directly log path, sometimes need update according to external storage changed */
	char		*log_path;

	/* filename without timestamp */
	char		*log_basename;

	/* filepath to check file exist */
	char		*file_path;

	/* used for "snap" type*/
	char		*content;

	/* used for "stream" type, log source handle */
	int		fd_device;

	/* used for "stream" type, modem memory source handle */
	int		fd_dump_cp;

	/* log file handle */
	FILE		*fp_out;

	/* setvbuf need buffer*/
	char		*setvbuf;

	/* current log file size count */
	int		outbytecount;

	/* for handle anr */
	struct timeval last, current;
}; 

struct slog_cmd {
	int type;
	char content[MAX_LINE_LEN * 2];
};

struct modem_timestamp {
        long unsigned int magic_number;       /* magic number for verify the structure */
        struct timeval tv;      /* clock time, seconds since 1970.01.01 */
        long unsigned int sys_cnt;            /* modem's time */
};

/* var */
extern char *config_log_path, *current_log_path;
extern char top_logdir[MAX_NAME_LEN];
extern char external_storage[MAX_NAME_LEN];
extern struct slog_info *stream_log_head, *snapshot_log_head;
extern struct slog_info *notify_log_head, *misc_log;
extern pthread_t stream_tid, kernel_tid, snapshot_tid, notify_tid, sdcard_tid, bt_tid, tcp_tid, kmemleak_tid, iodebug_tid;
extern int slog_enable;
extern int cplog_enable;
extern int internal_log_size;
extern int screenshot_enable;
#ifdef LOW_POWER_MODE
extern int hook_modem_flag;
#define HOOK_MODEM_TARGET_DIR	"/data/log"
#endif

/* function */
extern void *stream_log_handler(void *arg);
extern void *kernel_log_handler(void *arg);
extern void *snapshot_log_handler(void *arg);
extern void *notify_log_handler(void *arg);
extern void bt_snoop_stop(void);
extern void *bt_snoop_start(void *arg);
extern void *tcp_log_handler(void *arg);
extern void *kmemleak_handler(void *arg);
extern void *iodebug_log_handler(void *arg);

extern int stream_log_handler_started;
extern int kernel_log_handler_started;
extern int snapshot_log_handler_started;
extern int notify_log_handler_started;
extern int bt_log_handler_started;
extern int tcp_log_handler_started;
extern int kmemleak_handler_started;
extern int iodebug_log_handler_started;

/* parse_conf.c */
extern int gen_config_string(char *buffer);
extern int parse_config();

/* slog.c */
extern void exec_or_dump_content(struct slog_info *info, char *filepath);
extern int capture_by_name(struct slog_info *head, const char *name, char *filepath);

/* command.c */
extern int send_socket(int sockfd, void* buffer, int size);
extern int recv_socket(int sockfd, void* buffer, int size);
extern int request_socket_cmd(int cmd_type);
extern void cp_file(char *path, char *new_path);
extern FILE *gen_outfd(struct slog_info *info);
extern void log_size_handler(struct slog_info *info);
extern int write_from_buffer(int fd, char *buf, int len);
extern int open_device(struct slog_info *info, char *path);
extern void gen_logfile(char *filename, struct slog_info *info);
extern void log_buffer_flush(void);

/* snap.c */
extern void handle_javacrash_file(void);
#endif /*_SLOG_H*/
