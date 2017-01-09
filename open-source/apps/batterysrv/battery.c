#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <linux/netlink.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <utils/Log.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "battery.h"

#define _BUF_SIZE 100
char buf[_BUF_SIZE] = { 0 };

int uevent_fd = -1;

void init_capacity(void);
void save_capacity(void);

int main(int argc, char **argv)
{
	int ret = -1;
	int pre_usb_online = -1;
	int usb_online, ac_online, need_notify = 0, need_update = 0;
	int pre_ac_online = -1;
	/* open cmux channel */
	LOGD("batterysrv start\n");
	while (ret != 1) {
		ret = uevent_init();
		if (ret != 1) {
			sleep(1);
			LOGE("uevent init error\n");
		}
	}
	init_capacity();
	pre_usb_online = read_usb();
	pre_ac_online = read_ac();
	LOGD("inital status usb: %d ac: %d\n", pre_usb_online, pre_ac_online);
	if (pre_usb_online || pre_ac_online) {
		get_nv();
	}

	while (1) {
		ret = uevent_next_event(buf, _BUF_SIZE);
		if (ret <= 0)
			continue;
		usb_online = read_usb();
		ac_online = read_ac();
		if (usb_online != pre_usb_online) {
			pre_usb_online = usb_online;
			need_notify = 1;
			if (usb_online)
				need_update = 1;
			else
				need_update = 0;
		}
		if (ac_online != pre_ac_online) {
			pre_ac_online = ac_online;
			need_notify = 1;
			if (ac_online)
				need_update = 1;
			else
				need_update = 0;
		}
		if (need_notify) {
			need_notify = 0;
			if (need_update) {
				get_nv();
				LOGD("get_nv\n");
			} else {
				write_nv();
				LOGD("write_nv\n");
			}
		}
        save_capacity();
	}

	/* never come to here */
	LOGD("exit\n");
	return EXIT_SUCCESS;
}

int uevent_init()
{
	struct sockaddr_nl addr;
	int sz = 64 * 1024;
	int s;

	memset(&addr, 0, sizeof(addr));
	addr.nl_family = AF_NETLINK;
	addr.nl_pid = getpid();
	addr.nl_groups = 0xffffffff;

	s = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
	if (s < 0)
		return 0;

	setsockopt(s, SOL_SOCKET, SO_RCVBUFFORCE, &sz, sizeof(sz));

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		close(s);
		return 0;
	}

	LOGD("func: %s line: %d get %d\n", __func__, __LINE__, s);
	uevent_fd = s;
	return (uevent_fd > 0);
}

int uevent_next_event(char *buffer, int buffer_length)
{
	while (1) {
		struct pollfd fds;
		int nr;

		fds.fd = uevent_fd;
		fds.events = POLLIN;
		fds.revents = 0;
		nr = poll(&fds, 1, -1);

		if (nr > 0 && fds.revents == POLLIN) {
			int count = recv(uevent_fd, buffer, buffer_length, 0);
			if (count > 0) {
				return count;
			}
		}
	}

	/* won't get here */
	return 0;
}

int read_usb(void)
{
	int fd = -1;
	char buf[3];
	int ret = -1;
	fd = open("/sys/class/power_supply/usb/online", O_RDONLY);
	if (fd < 0) {
		LOGE("%s: usb online open error\n", __func__);
		return 0;
	}
	ret = read(fd, buf, 2U);
	if (ret >= 0)
		buf[ret] = '\0';
	close(fd);
	if (ret >= 1) {
		if (buf[0] == '1')
			return 1;
		else if (buf[0] == '0')
			return 0;
		else
			LOGE("%s: dont know what is it\n", __func__);
	}
	return 0;
}

int read_ac(void)
{
	int fd = -1;
	char buf[3];
	int ret = -1;
	fd = open("/sys/class/power_supply/ac/online", O_RDONLY);
	if (fd < 0) {
		LOGE("%s: ac online open error\n", __func__);
		return 0;
	}
	ret = read(fd, buf, 2U);
	if (ret >= 0)
		buf[ret] = '\0';
	close(fd);
	if (ret >= 1) {
		if (buf[0] == '1')
			return 1;
		else if (buf[0] == '0')
			return 0;
		else
			LOGE("%s: dont know what is it\n", __func__);
	}
	return 0;
}

#define NV_FILE "/data/.battery_nv"
#define BATTERY_NV_NODE "/sys/class/power_supply/battery/hw_switch_point"
void get_nv(void)
{
	int file_fd;
	int sys_fd;
	char nv_value[10] = { 0 };
	int ret;
	file_fd = open(NV_FILE, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR);
	if (file_fd == -1) {
		LOGE("open file: %s error %d\n", NV_FILE, file_fd);
		return;
	}

	ret = read(file_fd, nv_value, sizeof(nv_value));

	if (ret == 0) {
		LOGD("file: %s empty\n", NV_FILE);
		close(file_fd);
		return;
	}

	sys_fd = open(BATTERY_NV_NODE, O_WRONLY);
	if (sys_fd == -1) {
		LOGE("file: %s open error\n", BATTERY_NV_NODE);
		close(file_fd);
		return;
	}
	ret = write(sys_fd, nv_value, strlen(nv_value));
	if (ret != (int)strlen(nv_value)) {
		LOGE("write %s failed\n", BATTERY_NV_NODE);
	}
	LOGD("write %s to %s \n", nv_value, BATTERY_NV_NODE);
	close(file_fd);
	close(sys_fd);
	return;
}

void write_nv(void)
{
	int file_fd;
	int sys_fd;
	char nv_value[10] = { 0 };
	int ret;

	sys_fd = open(BATTERY_NV_NODE, O_RDONLY);
	if (sys_fd == -1) {
		LOGE("file: %s open error\n", BATTERY_NV_NODE);
		return;
	}
	ret = read(sys_fd, nv_value, sizeof(nv_value));
	if (ret == 0) {
		LOGE("read %s failed\n", BATTERY_NV_NODE);
		close(sys_fd);
		return;
	}
	file_fd = open(NV_FILE, O_WRONLY);
	if (file_fd == -1) {
		LOGE("open %s failed\n", NV_FILE);
		close(sys_fd);
		return;
	}
	ret = write(file_fd, nv_value, strlen(nv_value));
	if (ret != (int)strlen(nv_value)) {
		LOGE("write %s failed\n", NV_FILE);
	}
	LOGD("write %s to %s \n", nv_value, NV_FILE);
	close(sys_fd);
	close(file_fd);
}
#define CAPACITY_FILE "/productinfo/.save_capacity"
#define INIT_CAPACITY_NODE "/sys/class/power_supply/battery/save_capacity"
#define CAPACITY_NODE "/sys/class/power_supply/battery/capacity"

void init_capacity(void)
{
	int file_fd;
	int sys_fd;
	char nv_value[10] = { 0 };
	int ret;
	file_fd = open(CAPACITY_FILE, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR);
	if (file_fd == -1) {
		LOGE("open file: %s error %d\n", CAPACITY_FILE, file_fd);
		return;
	}

	ret = read(file_fd, nv_value, sizeof(nv_value));

	if (ret == 0) {
		LOGD("file: %s empty\n", CAPACITY_FILE);
		nv_value[0] = '3';
		nv_value[1] = '3';
		nv_value[2] = '3';
		//close(file_fd);
		//return;
	}

	sys_fd = open(INIT_CAPACITY_NODE, O_WRONLY);
	if (sys_fd == -1) {
		LOGE("file: %s open error\n", INIT_CAPACITY_NODE);
		close(file_fd);
		return;
	}
	ret = write(sys_fd, nv_value, strlen(nv_value));
	if (ret != (int)strlen(nv_value)) {
		LOGE("write %s failed\n", INIT_CAPACITY_NODE);
	}
	LOGD("write %s to %s \n", nv_value, INIT_CAPACITY_NODE);
	close(file_fd);
	close(sys_fd);
	return;
}

int old_capacity = 200;
void save_capacity(void)
{
	int file_fd;
	int sys_fd;
    int new_capacity;
	char nv_value[10] = { 0 };
	int ret;

	sys_fd = open(CAPACITY_NODE, O_RDONLY);
	if (sys_fd == -1) {
		LOGE("file: %s open error\n", CAPACITY_NODE);
		return;
	}
	ret = read(sys_fd, nv_value, sizeof(nv_value));
	if (ret == 0) {
		LOGE("read %s failed\n", CAPACITY_NODE);
		close(sys_fd);
		return;
	}
        close(sys_fd);
        new_capacity = strtol(nv_value,NULL,0);
        if(new_capacity != old_capacity){
            old_capacity = new_capacity;
        }else{
            return;
        }
	file_fd = open(CAPACITY_FILE, O_WRONLY |O_CREAT, S_IRUSR | S_IWUSR);
	if (file_fd == -1) {
		LOGE("open %s failed\n", CAPACITY_FILE);
		return;
	}
	ret = write(file_fd, nv_value, strlen(nv_value));
	if (ret != (int)strlen(nv_value)) {
		LOGE("write %s failed\n", CAPACITY_FILE);
	}
	LOGD("write %s to %s \n", nv_value, CAPACITY_FILE);
	close(file_fd);
}


