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
#include <dirent.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <linux/netlink.h>
#include <pthread.h>
#include <errno.h>

#include "slog.h"

/*socket begin*/
struct sockaddr_nl g_socket_src_addr, g_socket_dest_addr;
struct nlmsghdr *g_socket_nlh = NULL;
int g_socket_fd = -1;
int g_socket_recv_cnt = 0;
#define NETLINK_IODEBUG 29
#define NLMSG_USER_PID_MAXLEN 4 /*4 bytes*/
#define IOSNOOP_ACCESSING_DIR	"/sys/kernel/debug/tracing/events/block"
#define MAX_SYNC_TASK_NUM 1
pthread_t g_iodebug_sync_task_tid[MAX_SYNC_TASK_NUM];


enum iodebug_nlmsg_type_e {
	IODEBUG_NLMSG_TYPE_MIN = 0x10,
	NLMSGTYPE_U2K_GET_PID,
	NLMSGTYPE_K2U_SEND_EVENT,

	/*add other msg type before here*/
	IODEBUG_NLMSG_TYPE_MAX
};

enum iodebug_nlmsg_event_e {
	IODEBUG_NLMSG_EVENT_MIN = 100,
	NLMSGEVENT_K2U_SHOW_IO_INFO,
	NLMSGEVENT_SHOW_IO_PERIOD,
	NLMSGEVENT_SHOW_IO_ANR,

	/*add other msg event key before here*/
	IODEBUG_NLMSG_EVENT_MAX
};
/*socket end*/



/*
 * @SNAP: read the file once or periodly.
 * @ANR: read the file soon after ANR happened.
 */
enum trigger_type{
	SNAP = 0,
	ANR  = 1,
};

/*
 * describe the io action.
 * @model: it will shown in the log file.
 * @path: the file path of an action.
 */
struct io_conf{
	char *model;
	char *path;
};

struct io_conf g_io_static_info_conf[] = {
	{"EMMC", "/d/mmc0/mmc0:0001/cid"},
	{"EMMC", "/d/mmc0/mmc0:0001/csd"},
	{"EMMC", "/d/mmc0/mmc0:0001/ext_csd"},
	{"EMMC", "/d/mmc0/mmc0:0001/extcsd"},
	{"EMMC", "/d/mmc0/mmc0:0001/state"},
	{"EMMC", "/d/mmc0/mmc0:0001/status"},
	{"EMMC", "/sys/devices/sdio_emmc/mmc_host/mmc0/mmc0:0001/name"},
	{"EMMC", "/sys/devices/sdio_emmc/mmc_host/mmc0/mmc0:0001/cid"},
	{"EMMC", "/sys/devices/sdio_emmc/mmc_host/mmc0/mmc0:0001/csd"},
	{"EMMC", "/sys/devices/sdio_emmc/mmc_host/mmc0/mmc0:0001/oemid"},
	{"EMMC", "/sys/devices/sdio_emmc/mmc_host/mmc0/mmc0:0001/manfid"},
	{"EMMC", "/sys/devices/sdio_emmc/mmc_host/mmc0/mmc0:0001/fwrev"},
	{"EMMC", "/sys/devices/sdio_emmc/mmc_host/mmc0/mmc0:0001/hwrev"},
	{"EMMC", "/sys/devices/sdio_emmc/mmc_host/mmc0/mmc0:0001/date"},
	{"EMMC", "/sys/devices/sdio_emmc/mmc_host/mmc0/mmc0:0001/enhanced_area_size"},
	{"EMMC", "/sys/devices/sdio_emmc/mmc_host/mmc0/mmc0:0001/enhanced_area_offset"},
	{"EMMC", "/sys/devices/sdio_emmc/mmc_host/mmc0/mmc0:0001/perferred_erase_size"},
	{"EMMC", "/sys/devices/sdio_emmc/mmc_host/mmc0/mmc0:0001/erase_size"},
	{"EMMC", "/sys/devices/sdio_emmc/mmc_host/mmc0/mmc0:0001/type"},
	{"EMMC", "/sys/devices/sdio_emmc/mmc_host/mmc0/mmc0:0001/serial"},
	{"EMMC", "/sys/devices/sdio_emmc/mmc_host/mmc0/mmc0:0001/prv"},
	{"EMMC", "/sys/devices/sdio_emmc/mmc_host/mmc0/mmc0:0001/raw_rpmb_size_mult"},
	{"EMMC", "/sys/devices/sdio_emmc/mmc_host/mmc0/mmc0:0001/rel_sectors"},
	{"MM",   "/proc/sys/vm/dirty_background_ratio"},
	{"MM",   "/proc/sys/vm/dirty_background_bytes"},
	{"MM",   "/proc/sys/vm/dirty_ratio"},
	{"MM",   "/proc/sys/vm/dirty_bytes"},
};

struct io_conf g_io_base_info_conf[] = {
	{"VFS", "/sys/kernel/debug/iodebug/vfs_iodebug"},
	{"BDI", "/sys/kernel/debug/iodebug/bdi_iodebug"},
};

struct io_conf g_io_special_info_conf[] = {
	{"MM", "/proc/meminfo"},
};

struct sync_task_conf {
	char *name;
	char *path;
};

/* the script sync_task , paused when anr happend. */
struct sync_task_conf g_sync_task_info_conf[MAX_SYNC_TASK_NUM] = {
	{"iosnoop", "/system/bin/iosnoop.sh -Q -t -s 60"},
	//{"inotifywatch", "inotifywatch -v -t 3 -r /data /storage/sdcard0"},
};
int g_iodebug_sync_task_new_log_flag[MAX_SYNC_TASK_NUM] = {0};

static void iodebug_sync_task_save_to_slog(char *sync_task_name, int loop)
{
	struct tm tm;
	time_t t;
	char temp_buffer[200] = {0};
	char log_file_path[125] = {0};
	int flag = (loop/5)%2;
	t = time(NULL);
	localtime_r(&t, &tm);

	snprintf(log_file_path, sizeof(log_file_path), "%s/%s/iodebug/%s-%02d-%02d-%02d.log",
								current_log_path, top_logdir,sync_task_name,
								tm.tm_hour, tm.tm_min, tm.tm_sec);
	if ( flag == 1) { //pingpa 2 is new file
		snprintf(temp_buffer, sizeof(temp_buffer),
					"cat /dev/.iodebug_temp_file_%s_1 /dev/.iodebug_temp_file_%s_2 >> %s",
					sync_task_name, sync_task_name, log_file_path);
	} else {
		snprintf(temp_buffer, sizeof(temp_buffer),
					"cat /dev/.iodebug_temp_file_%s_2 /dev/.iodebug_temp_file_%s_1 >> %s",
					sync_task_name, sync_task_name, log_file_path);
	}
	system(temp_buffer);
	snprintf(temp_buffer, sizeof(temp_buffer),
					"rm /dev/.iodebug_temp_file_%s_1 .iodebug_temp_file_%s_2",
					sync_task_name, sync_task_name);
	system(temp_buffer);
}

static void sync_task_run_system_cmd(char *cmd_string , int task_pos, int flag)
{
	static char pingpa_flag[MAX_SYNC_TASK_NUM] = {0};
	char pingpa_string[] = {'1', '2'};
	char *temp_file = "/dev/.iodebug_temp_file_";
	char system_cmd_string[125] = {0};
	struct sync_task_conf *my_conf = &g_sync_task_info_conf[task_pos];
	char *task_name = my_conf->name;

	if (NULL == cmd_string) { //just init the pingpa_flag
		pingpa_flag[task_pos] = 0;
		return;
	}

	if (flag == 1)
		pingpa_flag[task_pos] = pingpa_flag[task_pos]^1;

	sprintf(system_cmd_string,  "%s %s%s_%c",
				cmd_string , temp_file, task_name, pingpa_string[pingpa_flag[task_pos]]);

	system(system_cmd_string);
}

static void sync_task_update_time(int task_pos)
{
	char cmd_string[125] = {0};
	struct tm tm;
	time_t t;
	t = time(NULL);
	localtime_r(&t, &tm);
	sprintf(cmd_string, "echo =====time:_%02d-%02d-%02d >> ",
					tm.tm_hour, tm.tm_min, tm.tm_sec);
	sync_task_run_system_cmd(cmd_string, task_pos, 0);
}

static void *iodebug_sync_task_handler(void *arg)
{
	DIR *p_dir;
	int task_pos;
	struct sync_task_conf *my_conf = arg;
	int ret;
	int update_flag = 1;
	char sync_task_cmd_string[125] = {0};
	int i = 0;

	if (!my_conf) {
		return NULL;
	}

	/* just for confirm ftrace exist or not */
	if((p_dir = opendir(IOSNOOP_ACCESSING_DIR)) == NULL) {
		err_log("iodebug: can not open %s!", IOSNOOP_ACCESSING_DIR);
		return NULL;
	}
	closedir(p_dir);

	if (0 == strcmp("iosnoop", my_conf->name)) {
		system("rm /dev/.ftrace-lock"); //just for iosnoop
	}
	task_pos = ((struct sync_task_conf *)arg - g_sync_task_info_conf);

	snprintf(sync_task_cmd_string, sizeof(sync_task_cmd_string), "%s >>", my_conf->path);

	while (1) {

		if (update_flag == 1) {
			sync_task_update_time(task_pos);
			update_flag = 0;
		}
		sync_task_run_system_cmd(sync_task_cmd_string, task_pos, 0);

		if (g_iodebug_sync_task_new_log_flag[task_pos] == 1) {
			iodebug_sync_task_save_to_slog(my_conf->name, i);
			sync_task_run_system_cmd(NULL, task_pos, 0); //just init the pingpa flag
			i = 0;
			g_iodebug_sync_task_new_log_flag[task_pos] = 0;
			update_flag = 1;
		} else {
			i++;
			if (i%5 == 0) {
				/*pingpa to anther temp file in dev*/
				sync_task_run_system_cmd("rm", task_pos, 1);
				update_flag = 1;
			}
		}

	}

	err_log("thread exit, ret = %d", ret);
	return NULL;
}

static void iodebug_stop_sync_task_thread(void)
{
	int i;
	err_log("iodebug anr in");
	for (i=0;i<MAX_SYNC_TASK_NUM; i++)
		g_iodebug_sync_task_new_log_flag[i] = 1;
	return;
}

static void iodebug_sync_task_conf_init(void)
{
	int i;
	int ret;

	for (i=0; i<MAX_SYNC_TASK_NUM; i++) {
		ret = pthread_create(&g_iodebug_sync_task_tid[i], NULL, iodebug_sync_task_handler, &g_sync_task_info_conf[i]);
		if (ret < 0) {
			err_log("creat thread failed.index %d", i);
		}
	}
}

static char* parse_path(char *path)
{
	char *pos = NULL;
	if(NULL == path) {
		err_log("path = NULL\n");
		return NULL;
	}

	pos = strrchr(path, '/');

	if (pos) {
		return pos;
	}
	return NULL;
}

static int iodebug_print_info(FILE *fp, char *path, char *iodebug_path, char *model)
{
	char *file_name = NULL;
	char buffer[256] = {0};
	FILE *info_fp = NULL;
	int ret;

	if ((NULL == path) || (NULL == fp) || (NULL == iodebug_path)) {
		err_log("%s path = null || fp = NULL\n", __func__);
		return -1;
	}

	file_name = parse_path(path);
	if (file_name) {
		ret = fprintf(fp, "\n%s  %s:", model, file_name+1);
		info_fp = fopen(path, "r");
		if (info_fp) {
			while( (ret = fread(buffer, 1, 256, info_fp)) > 0) {
				fwrite(buffer, 1, ret, fp);
			}
			fclose(info_fp);
		} else {
			err_log("open %s failed\n", path);
		}
	}

	return 0;
}

static void iodebug_nlevent_io_info_handler(char *log_file_name, int special_flag)
{
	int i;
	struct tm tm;
	time_t t;
	char vfs_slog_path[MAX_NAME_LEN];
	FILE *fp = NULL;
	char *io_vfs_file = NULL;
	int ret;

	sprintf(vfs_slog_path, "%s/%s/iodebug/%s", current_log_path, top_logdir, log_file_name);
	fp = fopen(vfs_slog_path, "a+");
	if (!fp) {
		err_log("open %s failed\n", vfs_slog_path);
		return;
	}

	t = time(NULL);
	localtime_r(&t, &tm);
	ret = fprintf(fp, "\n\n=====iodebug-%02d-%02d-%02d============\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
	if (ret <= 0) {
		err_log("write io_period.log failed, err = %d\n", ret);
		goto out;
	}

	for (i=0;i<sizeof(g_io_base_info_conf)/sizeof(struct io_conf); i++) {
		iodebug_print_info(fp, g_io_base_info_conf[i].path, vfs_slog_path, g_io_base_info_conf[i].model);
	}

	if (special_flag) {
		for (i=0;i<sizeof(g_io_special_info_conf)/sizeof(struct io_conf); i++) {
			iodebug_print_info(fp, g_io_special_info_conf[i].path, vfs_slog_path, g_io_special_info_conf[i].model);
		}
	}
out:
	fclose(fp);
}

static void iodebug_process_socket_event(enum iodebug_nlmsg_event_e event)
{
	switch (event) {
		case NLMSGEVENT_SHOW_IO_PERIOD:
			iodebug_nlevent_io_info_handler("io_period.log", 0);
		break;
		case NLMSGEVENT_K2U_SHOW_IO_INFO:
			iodebug_nlevent_io_info_handler("io_k2u.log", 1);
		break;
		case NLMSGEVENT_SHOW_IO_ANR:
			iodebug_nlevent_io_info_handler("io_anr.log", 1);
			iodebug_stop_sync_task_thread();
		break;
		default:
			err_log("default event = %d\n", event);
		break;
	};
}

/* the info saved in io_info.log */
static void process_io_static_info_log(void)
{
	int i;
	char iodebug_path[MAX_NAME_LEN];
	int ret;
	struct tm tm;
	time_t t;
	FILE *fp = NULL;

	sprintf(iodebug_path, "%s/%s/iodebug", current_log_path, top_logdir);

	ret = mkdir(iodebug_path, S_IRWXU | S_IRWXG | S_IRWXO);
	if (-1 == ret && (errno != EEXIST)) {
		err_log("mkdir %s failed.", iodebug_path);
		exit(0);
	}

	iodebug_log_handler_started = 1;

	sprintf(iodebug_path, "%s/io_static_info.log", iodebug_path);
	fp = fopen(iodebug_path, "w+");
	if (fp == NULL) {
		err_log("open %s failed\n", iodebug_path);
		exit(0);
	}

	t = time(NULL);
	localtime_r(&t, &tm);
	ret = fprintf(fp, "\n\n=====iodebug-%02d-%02d-%02d============\n", tm.tm_hour, tm.tm_min, tm.tm_sec);
	if (ret <= 0) {
		err_log("write io_info.log failed, err = %d\n", ret);
	}

	for (i=0; i<sizeof(g_io_static_info_conf)/sizeof(struct io_conf); i++) {
		iodebug_print_info(fp, g_io_static_info_conf[i].path, iodebug_path, g_io_static_info_conf[i].model);
	}

	fclose(fp);
}

static int iodebug_socket_init(void)
{
	int ret;
	int* socket_data;

	g_socket_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_IODEBUG);
	if (g_socket_fd < 0) {
		err_log("iodebug:create socket failed and return %d, %s ,%d\n", g_socket_fd, strerror(errno), errno);
		return -1;
	}

	memset(&g_socket_src_addr, 0, sizeof(g_socket_src_addr));
	g_socket_src_addr.nl_family = AF_NETLINK;
	g_socket_src_addr.nl_pid = getpid(); /* self pid */

	bind(g_socket_fd, (struct sockaddr *)&g_socket_src_addr, sizeof(g_socket_src_addr));

	memset(&g_socket_dest_addr, 0, sizeof(g_socket_dest_addr));
	g_socket_dest_addr.nl_family = AF_NETLINK;
	g_socket_dest_addr.nl_pid = 0; /* For Linux Kernel */
	g_socket_dest_addr.nl_groups = 0; /* unicast */

	g_socket_nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(NLMSG_USER_PID_MAXLEN));
	memset(g_socket_nlh, 0, NLMSG_SPACE(NLMSG_USER_PID_MAXLEN));
	g_socket_nlh->nlmsg_len = NLMSG_SPACE(NLMSG_USER_PID_MAXLEN);
	g_socket_nlh->nlmsg_pid = getpid();
	g_socket_nlh->nlmsg_flags = NLM_F_REQUEST;/*send to kernel, must set NLM_F_REQUEST*/
	g_socket_nlh->nlmsg_type = NLMSGTYPE_U2K_GET_PID;
	g_socket_nlh->nlmsg_seq = 0;

	socket_data = (int*)NLMSG_DATA(g_socket_nlh);
	err_log("iodebug:pid:%d,data:0x%x,nlmsg_len:%d\n",g_socket_nlh->nlmsg_pid,*socket_data,g_socket_nlh->nlmsg_len);

	err_log("iodebug:Sending message to kernel\n");
	ret = sendto(g_socket_fd, g_socket_nlh, g_socket_nlh->nlmsg_len,
				 0, (struct sockaddr *)&g_socket_dest_addr, sizeof(struct sockaddr_nl));
	if (ret == -1) {
		close(g_socket_fd);
		err_log("iodebug: sendto() error.\n");
	}

	return ret;
}

void *iodebug_log_handler(void *arg)
{
	int ret;
	char socket_recv_buf[40];
	int *socket_ptr;
	int socket_addr_len = sizeof(struct sockaddr_nl);

	if ( !strncmp(current_log_path, INTERNAL_LOG_PATH, strlen(INTERNAL_LOG_PATH)) ) {
		//return NULL;
	}

	process_io_static_info_log();

	ret = iodebug_socket_init();
	if (ret < 0) {
		err_log("iodebug_socket_init faild, ret = %d\n", ret);
		goto out;
	}

	iodebug_sync_task_conf_init();

	/*get event from socket*/
	do {
		//err_log("iodebug:Waiting for message from kernel\n");
		/* Read message from kernel */
		memset(socket_recv_buf, 0, 40);
		ret = recvfrom(g_socket_fd, socket_recv_buf, 40, 0, (struct sockaddr *)&g_socket_dest_addr, &socket_addr_len);
		if (ret == -1) {
			err_log("iodebug: recvfrom() error.\n");
			goto socket_out;
	    }

	    socket_ptr = NLMSG_DATA((struct nlmsghdr *)socket_recv_buf);
		//err_log("iodebug:get msg(no:%d) from kernel, event key:%d.\n",g_socket_recv_cnt++, *socket_ptr);

		iodebug_process_socket_event(*socket_ptr);

	} while(slog_enable == SLOG_ENABLE);

socket_out:
	err_log("out");
	close(g_socket_fd);
    free(g_socket_nlh);
out:
	iodebug_log_handler_started = 0;
	return NULL;
}

