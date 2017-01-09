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
#include <sys/inotify.h>
#include <pthread.h>
#include <errno.h>

#define err_log printf
/*socket begin*/
struct sockaddr_nl g_socket_src_addr, g_socket_dest_addr;
struct nlmsghdr *g_socket_nlh = NULL;
int g_socket_fd = -1;
int g_socket_recv_cnt = 0;
#define MAX_LINE_LEN		2048
#define NETLINK_IODEBUG		29
#define NLMSG_USER_PID_MAXLEN	4 /*4 bytes*/
#define IOSNOOP_DIR		"/system/bin/iosnoop.sh"
#define IOSNOOP_ACCESSING_DIR	"/sys/kernel/debug/tracing/events/block"
#define FTRACE_TRACING_ON	"/sys/kernel/debug/tracing/tracing_on"
#define MAX_SYNC_TASK_NUM	1
pthread_t g_iodebug_special_task_tid[MAX_SYNC_TASK_NUM];


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
	{"EMMC", "/d/mmc0/mmc0:0001/ext_csd"},
	{"EMMC", "/d/mmc0/mmc0:0001/state"},
	{"EMMC", "/d/mmc0/mmc0:0001/status"},
	{"EMMC", "/sys/class/mmc_host/mmc0/mmc0:0001/name"},
	{"EMMC", "/sys/class/mmc_host/mmc0/mmc0:0001/cid"},
	{"EMMC", "/sys/class/mmc_host/mmc0/mmc0:0001/csd"},
	{"EMMC", "/sys/class/mmc_host/mmc0/mmc0:0001/oemid"},
	{"EMMC", "/sys/class/mmc_host/mmc0/mmc0:0001/manfid"},
	{"EMMC", "/sys/class/mmc_host/mmc0/mmc0:0001/fwrev"},
	{"EMMC", "/sys/class/mmc_host/mmc0/mmc0:0001/hwrev"},
	{"EMMC", "/sys/class/mmc_host/mmc0/mmc0:0001/date"},
	{"EMMC", "/sys/class/mmc_host/mmc0/mmc0:0001/enhanced_area_size"},
	{"EMMC", "/sys/class/mmc_host/mmc0/mmc0:0001/enhanced_area_offset"},
	{"EMMC", "/sys/class/mmc_host/mmc0/mmc0:0001/preferred_erase_size"},
	{"EMMC", "/sys/class/mmc_host/mmc0/mmc0:0001/erase_size"},
	{"EMMC", "/sys/class/mmc_host/mmc0/mmc0:0001/type"},
	{"EMMC", "/sys/class/mmc_host/mmc0/mmc0:0001/serial"},
	{"EMMC", "/sys/class/mmc_host/mmc0/mmc0:0001/prv"},
	{"EMMC", "/sys/class/mmc_host/mmc0/mmc0:0001/raw_rpmb_size_mult"},
	{"EMMC", "/sys/class/mmc_host/mmc0/mmc0:0001/rel_sectors"},
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

struct special_task_conf {
	char *name;
	char *path;
};

/* the script special_task , paused when anr happend. */
struct special_task_conf g_special_task_info_conf[MAX_SYNC_TASK_NUM] = {
	{"iosnoop", "/system/bin/iosnoop.sh -Q -t -s 60"},
	//  {"inotifywatch", "inotifywatch -v -t 3 -r /data /storage/sdcard0"},
};
int g_iodebug_special_task_new_log_flag[MAX_SYNC_TASK_NUM] = {0};

static void iodebug_special_task_save_to_slog(char *special_task_name, int loop)
{
	char buffer[256] = {0};
	char temp_buffer[200] = {0};
	char buffer_path[2][64] = {{0}, {0}};
	int flag = (loop/5)%2;
	FILE *info_fp = NULL;
	int ret;
	int i;

	if (flag == 1) { //  pingpa 2 is new file
		snprintf(buffer_path[0], sizeof(buffer_path[0]),
			"/dev/.iodebug_temp_file_%s_1", special_task_name);
		snprintf(buffer_path[1], sizeof(buffer_path[1]),
			"/dev/.iodebug_temp_file_%s_2", special_task_name);
	} else {
		snprintf(buffer_path[0], sizeof(buffer_path[0]),
			"/dev/.iodebug_temp_file_%s_2", special_task_name);
		snprintf(buffer_path[1], sizeof(buffer_path[1]),
			"/dev/.iodebug_temp_file_%s_1", special_task_name);
	}

	for (i = 0; i < 2; i++) {
		info_fp = fopen(buffer_path[i], "r");
		if (info_fp) {
			while((ret = fread(buffer, 1, 256, info_fp)) > 0) {
				write(STDOUT_FILENO, buffer, ret);
			}
			fclose(info_fp);
		} else
			err_log("open %s failed\n", buffer_path[i]);
	}

	snprintf(temp_buffer, sizeof(temp_buffer),
		"rm /dev/.iodebug_temp_file_%s_1 .iodebug_temp_file_%s_2",
		special_task_name, special_task_name);
	system(temp_buffer);
}

static void special_task_run_system_cmd(char *cmd_string, int task_pos, int flag)
{
	static int pingpa_flag[MAX_SYNC_TASK_NUM] = {0};
	char pingpa_string[] = {'1', '2'};
	char *temp_file = "/dev/.iodebug_temp_file_";
	char system_cmd_string[125] = {0};
	struct special_task_conf *my_conf = &g_special_task_info_conf[task_pos];
	char *task_name = my_conf->name;

	if (NULL == cmd_string) { //just init the pingpa_flag
		pingpa_flag[task_pos] = 0;
		return;
	}

	if (flag == 1)
		pingpa_flag[task_pos] = pingpa_flag[task_pos]^1;

	snprintf(system_cmd_string, sizeof(system_cmd_string),
			"%s %s%s_%c",
			cmd_string , temp_file, task_name,
			pingpa_string[pingpa_flag[task_pos]]);

	system(system_cmd_string);
}

static void special_task_update_time(int task_pos)
{
	char cmd_string[125] = {0};
	struct tm tm;
	time_t t;

	t = time(NULL);
	localtime_r(&t, &tm);
	snprintf(cmd_string, sizeof(cmd_string),
			"echo =====time:%d-%02d-%02d %02d-%02d-%02d >> ",
			1900+tm.tm_year, 1+tm.tm_mon, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec);
	special_task_run_system_cmd(cmd_string, task_pos, 0);
}

static void *iodebug_special_task_handler(void *arg)
{
	DIR *p_dir;
	int task_pos;
	struct special_task_conf *my_conf = arg;
	int update_flag = 1;
	char special_task_cmd_string[125] = {0};
	int i = 0;

	if (!my_conf) {
		return NULL;
	}

	/* just for confirm ftrace exist or not */
	if ((p_dir = opendir(IOSNOOP_ACCESSING_DIR)) == NULL) {
		err_log("iodebug: directory %s does not exist, exit!\n", IOSNOOP_ACCESSING_DIR);
		//return NULL;
		exit(1);
	}
	closedir(p_dir);

	if (0 == strcmp("iosnoop", my_conf->name))
		system("rm /dev/.ftrace-lock"); //just for iosnoop

	task_pos = ((struct special_task_conf *)arg - g_special_task_info_conf);

	snprintf(special_task_cmd_string, sizeof(special_task_cmd_string),
			"%s >>", my_conf->path);

	while (1) {
		if (update_flag == 1) {
			special_task_update_time(task_pos);
			update_flag = 0;
		}
		special_task_run_system_cmd(special_task_cmd_string, task_pos, 0);

		if (g_iodebug_special_task_new_log_flag[task_pos] == 1) {
			iodebug_special_task_save_to_slog(my_conf->name, i);
			special_task_run_system_cmd(NULL, task_pos, 0); //just init the pingpa flag
			i = 0;
			g_iodebug_special_task_new_log_flag[task_pos] = 0;
			update_flag = 1;
		} else {
			i++;
			if (i%5 == 0) {
				/*pingpa to anther temp file in dev*/
				special_task_run_system_cmd("rm", task_pos, 1);
				update_flag = 1;
			}
		}
	}

	err_log("thread exit");
	return NULL;
}

void iodebug_stop_special_task_thread(void)
{
	int i;
	err_log("iodebug anr in ");
	for (i=0;i<MAX_SYNC_TASK_NUM; i++)
		g_iodebug_special_task_new_log_flag[i] = 1;
	return;
}

static void iodebug_special_task_conf_init(void)
{
	int i;
	int ret;

	for (i=0; i<MAX_SYNC_TASK_NUM; i++) {
		ret = pthread_create(&g_iodebug_special_task_tid[i],
					NULL,
					iodebug_special_task_handler,
					&g_special_task_info_conf[i]);
		if (ret != 0) {
			err_log("creat thread failed.index %d\n", i);
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

	if (pos)
		return pos;

	return NULL;
}

static int iodebug_print_info(char *path, char *model)
{
	char *file_name = NULL;
	char buffer[256] = {0};
	char buf[64] = {0};
	FILE *info_fp = NULL;
	int ret;

	if ((NULL == path)) {
		err_log("%s path = null\n", __func__);
		return -1;
	}

	file_name = parse_path(path);
	if (file_name) {
		snprintf(buf, sizeof(buf), "\n%s  %s:", model, file_name+1);
		write(STDOUT_FILENO, buf, strlen(buf));
		info_fp = fopen(path, "r");
		if (info_fp) {
			while ((ret = fread(buffer, 1, 256, info_fp)) > 0) {
				write(STDOUT_FILENO, buffer, ret);
			}
			fclose(info_fp);
		} else
			err_log("open %s failed\n", path);
	}

	return 0;
}

void iodebug_nlevent_io_info_handler(int special_flag)
{
	unsigned int i;
	struct tm tm;
	time_t t;
	char buf[64] = {0};

	t = time(NULL);
	localtime_r(&t, &tm);
	snprintf(buf, sizeof(buf),
		"\n\n=====iodebug-%d-%02d-%02d %02d:%02d:%02d============\n",
		1900+tm.tm_year, 1+tm.tm_mon, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);
	write(STDOUT_FILENO, buf, strlen(buf));

	for (i=0; i<sizeof(g_io_base_info_conf)/sizeof(struct io_conf); i++) {
		iodebug_print_info(g_io_base_info_conf[i].path,
						g_io_base_info_conf[i].model);
	}

	if (special_flag) {
		for (i=0;i<sizeof(g_io_special_info_conf)/sizeof(struct io_conf);i++) {
			iodebug_print_info(g_io_special_info_conf[i].path,
						g_io_special_info_conf[i].model);
		}
	}
}

static void iodebug_process_socket_event(enum iodebug_nlmsg_event_e event)
{
	switch (event) {
		case NLMSGEVENT_SHOW_IO_PERIOD:
		case NLMSGEVENT_K2U_SHOW_IO_INFO:
		case NLMSGEVENT_SHOW_IO_ANR:
			iodebug_nlevent_io_info_handler(1);
		break;
		default:
			err_log("default event = %d\n", event);
		break;
	}
}

/* the info saved in io_info.log */
static void process_io_static_info_log(void)
{
	unsigned int i;
	char buf[64] = {0};
	struct tm tm;
	time_t t;


	t = time(NULL);
	localtime_r(&t, &tm);
	snprintf(buf, sizeof(buf),
		"\n\n=====iodebug-%d-%02d-%02d %02d:%02d:%02d============\n",
		1900+tm.tm_year, 1+tm.tm_mon, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);
	write(STDOUT_FILENO, buf, strlen(buf));

	for (i=0; i<sizeof(g_io_static_info_conf)/sizeof(struct io_conf); i++) {
		iodebug_print_info(g_io_static_info_conf[i].path,
					g_io_static_info_conf[i].model);
	}
}

static int iodebug_socket_init(void)
{
	int ret;
	int* socket_data;

	g_socket_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_IODEBUG);
	if (g_socket_fd < 0) {
		err_log("iodebug:create socket failed and return %d, %s, %d\n",
					g_socket_fd, strerror(errno), errno);
		return -1;
	}

	memset(&g_socket_src_addr, 0, sizeof(g_socket_src_addr));
	g_socket_src_addr.nl_family = AF_NETLINK;
	g_socket_src_addr.nl_pid = getpid(); /* self pid */

	bind(g_socket_fd, (struct sockaddr *)&g_socket_src_addr,
								sizeof(g_socket_src_addr));

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
	err_log("iodebug:pid:%d,data:0x%x,nlmsg_len:%d\n",
			g_socket_nlh->nlmsg_pid, *socket_data, g_socket_nlh->nlmsg_len);

	err_log("iodebug:Sending message to kernel\n");
	ret = sendto(g_socket_fd, g_socket_nlh,
				g_socket_nlh->nlmsg_len, 0,
				(struct sockaddr *)&g_socket_dest_addr,
				sizeof(struct sockaddr_nl));
	if (ret == -1) {
		close(g_socket_fd);
		err_log("iodebug: sendto() error.\n");
	}

	return ret;
}

int main(int argc, char *argv[])
{
	ssize_t size;
	int ret;
	unsigned int i;
	int notify_fd, wd, len, result;
	fd_set readset;
	char buffer[MAX_LINE_LEN];
	struct inotify_event *event;
	struct timeval timeout;
	FILE *info_fp = NULL;
	int monitor_node = 0;
	char socket_recv_buf[40];
	int *socket_ptr;
	int socket_addr_len = sizeof(struct sockaddr_nl);


	if ((argc > 1) && (strcmp(argv[1], "-i") == 0)) {
#ifdef IOSNOOP_DISABLE
		err_log("iosnoop is disabled.\n");
		return 0;
#endif
		if(access(IOSNOOP_DIR, F_OK) != 0) {
			err_log("iosnoop.sh is not exist, exit!\n");
			return -ENOENT;
		}

		/* init notify list */
		notify_fd = inotify_init();
		if (notify_fd < 0) {
			err_log("inotify_init() failed, exit!%s\n", strerror(errno));
			return notify_fd;
		}
		wd = inotify_add_watch(notify_fd, "/data/anr", IN_MODIFY);
		if (wd == -1) {
			err_log("inotify_add_watch failed, exit!%s\n", strerror(errno));
			close(notify_fd);
			return wd;
		}

		/* enable ftrace buffer for iosnoop */
		info_fp = fopen(FTRACE_TRACING_ON, "w");
		if (info_fp) {
			fputs("1", info_fp);
			fclose(info_fp);
		} else {
			err_log("open %s failed\n", FTRACE_TRACING_ON);
			return -ENOENT;
		}

		iodebug_special_task_conf_init();
		while (1) {
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

			if (FD_ISSET(notify_fd, &readset) <= 0)
				continue;

			/* when watched dir changed, event will be read */
			size = read(notify_fd, buffer, MAX_LINE_LEN);
			if(size <= 0) {
				err_log("read inotify fd failed, %d.\n", (int)size);
				sleep(1);
				continue;
			}
			event = (struct inotify_event *)buffer;
			while(size > 0) {
				len = sizeof(struct inotify_event) + event->len;
				err_log("notify event: wd: %d, mask %d, ret %d, len %d, file %s\n",
						event->wd, event->mask, (int)size, len, event->name);
				iodebug_stop_special_task_thread();
				event = (struct inotify_event *)((char *)event + len);
				size -= len;
				sleep(5);
			}
		}
		close(notify_fd);
	} else {
		/* just for confirm iodebug node exist or not */
		for (i=0; i<sizeof(g_io_base_info_conf)/sizeof(struct io_conf); i++) {
			if ((info_fp = fopen(g_io_base_info_conf[i].path, "r")) != NULL) {
				monitor_node++;
				fclose(info_fp);
			}
		}
		if (monitor_node == 0) {
			err_log("iodebug monitor node does not exist, exit!\n");
			return -ENOENT;
		}

		process_io_static_info_log();
		ret = iodebug_socket_init();
		if (ret < 0) {
			err_log("iodebug_socket_init faild, ret = %d\n", ret);
			goto out;
		}
		/*get event from socket*/
		while (1) {
			//err_log("iodebug:Waiting for message from kernel\n");
			/* Read message from kernel */
			memset(socket_recv_buf, 0, 40);
			ret = recvfrom(g_socket_fd, socket_recv_buf,
					40, 0,
					(struct sockaddr *)&g_socket_dest_addr,
					(socklen_t *)&socket_addr_len);
			if (ret == -1) {
				err_log("iodebug: recvfrom() error.\n");
				goto socket_out;
			}

			socket_ptr = NLMSG_DATA((struct nlmsghdr *)socket_recv_buf);
			//err_log("iodebug:get msg(no:%d) from kernel, event key:%d.\n",g_socket_recv_cnt++, *socket_ptr);

			iodebug_process_socket_event(*socket_ptr);
		}
	}

socket_out:
	err_log("out");
	close(g_socket_fd);
	free(g_socket_nlh);
out:
	return 0;
}

