#define LOG_TAG 	"WCND"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <cutils/sockets.h>
#include <ctype.h>
#include <pthread.h>
#include <errno.h>
#include <cutils/properties.h>
#include <utils/Log.h>
#include <signal.h>
#include <dirent.h>

#include "wcnd.h"



#include <cutils/properties.h>

#include <fcntl.h>
#include <sys/socket.h>


#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/limits.h>

#include <netinet/in.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <errno.h>
#include <netutils/ifc.h>



static int wcnd_read_to_buf(const char *filename, char *buf, int buf_size)
{
	int fd;

	if(buf_size <= 1) return -1;

	ssize_t ret = -1;
	fd = open(filename, O_RDONLY);
	if(fd >= 0)
	{
		ret = read(fd, buf, buf_size-1);
		close(fd);
	}
	((char *)buf)[ret > 0 ? ret : 0] = '\0';
	return ret;
}



static unsigned wcnd_strtou(const char *string)
{
	unsigned long v;
	char *endptr;
	char **endp = &endptr;

	if(!string) return UINT_MAX;

	*endp = (char*) string;

	if (!isalnum(string[0])) return UINT_MAX;
	errno = 0;
	v = strtoul(string, endp, 10);
	if (v > UINT_MAX) return UINT_MAX;

	char next_ch = **endp;
	if(next_ch)
	{
		/* "1234abcg" or out-of-range */
		if (isalnum(next_ch) || errno)
			return UINT_MAX;

		/* good number, just suspicious terminator */
		errno = EINVAL;

	}

	return v;
}



static pid_t wcnd_find_pid_by_name(const char *proc_name)
{
	pid_t target_pid = 0;

	DIR *proc_dir = NULL;
	struct dirent *entry = NULL;

	if(!proc_name) return 0;

	proc_dir = opendir("/proc");

	if(!proc_dir)
	{
		WCND_LOGE("open /proc fail: %s", strerror(errno));
		return 0;
	}

	while((entry = readdir(proc_dir)))
	{
		char buf[1024];
		unsigned pid;
		int n;
		char filename[sizeof("/proc/%u/task/%u/cmdline") + sizeof(int)*3 * 2];


		pid = wcnd_strtou(entry->d_name);
		if (errno)
			continue;


		sprintf(filename, "/proc/%u/cmdline", pid);

		n = wcnd_read_to_buf(filename, buf, 1024);

		if(n < 0)	continue;

		WCND_LOGD("pid: %d, command name: %s", pid, buf);


		if(strcmp(buf, proc_name) == 0)
		{
			WCND_LOGD("find pid: %d for target process name: %s", pid, proc_name);

			target_pid = pid;

			break;
		}
	} /* for (;;) */

	if(proc_dir) closedir(proc_dir);


	return target_pid;
}


/**
* to check if the process with the process name or pid exist
* return 0: the process does not exist
* return > 0: the process still exist
* return < 0: other error happens
*/
int wcnd_check_process_exist(const char *proc_name, int proc_pid)
{
	pid_t target_pid = 0;

	DIR *proc_dir = NULL;
	struct dirent *entry = NULL;

	if(!proc_name && proc_pid <= 0) return 0;

	proc_dir = opendir("/proc");

	if(!proc_dir)
	{
		WCND_LOGE("open /proc fail: %s", strerror(errno));
		return -1;
	}

	while((entry = readdir(proc_dir)))
	{
		char buf[1024];
		unsigned pid;
		int n;
		char filename[sizeof("/proc/%u/task/%u/cmdline") + sizeof(int)*3 * 2];


		pid = wcnd_strtou(entry->d_name);
		if (errno)
			continue;

		sprintf(filename, "/proc/%u/cmdline", pid);

		n = wcnd_read_to_buf(filename, buf, 1024);

		if(n < 0)	continue;

		WCND_LOGD("pid: %d, command name: %s", pid, buf);


		if(proc_name && (strcmp(buf, proc_name) == 0))
		{
			WCND_LOGD("find pid: %d for target process name: %s", pid, proc_name);

			target_pid = pid;

			break;
		}

		if(proc_pid > 0 && pid == (unsigned)proc_pid)
		{
			WCND_LOGD("find process by pid: %d for target process name: %s", pid, (proc_name?proc_name:""));

			target_pid = pid;

			break;
		}

	} /* for (;;) */

	if(proc_dir) closedir(proc_dir);

	if(target_pid == 0)
		WCND_LOGD("The Process for (%d)%s does not exist!!", proc_pid, (proc_name?proc_name:""));

	return (int)target_pid;
}


int wcnd_kill_process(pid_t pid, int signal)
{
	//signal such as SIGKILL
	return kill(pid, signal);
}

/**
* return -2: for the target process is not exist
* return -1 for fail
* return > 0 the target pid
*/
int wcnd_kill_process_by_name(const char *proc_name, int signal)
{
	if (!proc_name) return -2;

	pid_t target_pid = wcnd_find_pid_by_name(proc_name);

	if(target_pid == 0)
	{
		WCND_LOGD("Cannot find the target pid for %s", proc_name);
		return -2;
	}

	WCND_LOGD("kill %s by signal: %d\n", proc_name, signal);

	if(wcnd_kill_process(target_pid, signal) < 0)
	{
		WCND_LOGE("Error kill process: %s", strerror(errno));

		return -1;
	}

	return (int)target_pid;
}


/**
 * Return 0: for the process is not exist.
 * Otherwise return the pid of the process.
 */
int wcnd_find_process_by_name(const char *proc_name)
{
	pid_t target_pid = wcnd_find_pid_by_name(proc_name);

	if(target_pid == 0)
	{
		WCND_LOGD("Cannot find the target process!!");
		return 0;
	}

	return (int)target_pid;
}


int wcnd_down_network_interface(const char *ifname)
{
	ifc_init();

	if (ifc_down(ifname))
	{
		WCND_LOGE("Error downing interface: %s", strerror(errno));
	}
	ifc_close();
	return 0;
}

int wcnd_up_network_interface(const char *ifname)
{
	ifc_init();

	if (ifc_up(ifname))
	{
		WCND_LOGE("Error upping interface: %s", strerror(errno));
	}
	ifc_close();
	return 0;
}

#define WAIT_ONE_TIME (200)   /* wait for 200ms at a time when polling for property values */

void wcnd_wait_for_supplicant_stopped(void)
{
	char value1[PROPERTY_VALUE_MAX] = {'\0'};
	char value2[PROPERTY_VALUE_MAX] = {'\0'};

	property_get("init.svc.p2p_supplicant", value1, "stopped");
	property_get("init.svc.wpa_supplicant", value2, "stopped");

	if((strcmp(value1, "stopped") == 0) && (strcmp(value2, "stopped") == 0))
		return;

	int maxwait = 3; // wait max 30 s for slog dump cp2 log
	int maxnaps = (maxwait * 1000) / WAIT_ONE_TIME;

	if (maxnaps < 1)
	{
		maxnaps = 1;
	}

	memset(value1, 0, sizeof(value1));
	memset(value2, 0, sizeof(value2));

	while (maxnaps-- > 0)
	{
		usleep(200 * 1000);
		if (property_get("init.svc.p2p_supplicant", value1, "stopped") && property_get("init.svc.wpa_supplicant", value2, "stopped"))
		{
			if((strcmp(value1, "stopped") == 0) && (strcmp(value2, "stopped") == 0))
			{
				return;
			}
		}
	}
}



static const char WIFI_DRIVER_PROP_NAME[]    = "wlan.driver.status";

void wcnd_wait_for_driver_unloaded(void)
{
	char driver_status[PROPERTY_VALUE_MAX];

	int count = 100; /* wait at most 20 seconds for completion */
	while (count-- > 0)
	{
		if (!property_get(WIFI_DRIVER_PROP_NAME, driver_status, NULL)
			|| strcmp(driver_status, "ok") != 0) /* driver not loaded */
		    break;
		usleep(200000);
		//WCND_LOGE("status: %s", driver_status);
	}

	if (count <= 0)
	{
		WCND_LOGE("Error Wifi driver cannot unloaded in 20 seconds");
	}
	else
	{
		WCND_LOGE("Wifi driver has been unloaded");
	}
}

/**
* To notify the wifi driver with the cp2 state.
* state_ok: 0, then cp2 is assert.
* return  0 for sucess.
*  1 means driver is removing, wcnd need to wait driver to be unloaded
*/
#define WIFI_DRIVER_PRIV_CMD_SET_CP2_ASSERT (0x10)

#define WIFI_DRIVER_STATE_REMOVEING (0x11)

int wcnd_notify_wifi_driver_cp2_state(int state_ok)
{
	int fd, ret, index, len, cmd, data;
	char buf[32] = {0};

	fd = open("/proc/wlan", O_RDWR);
	if(fd <0)
	{
		WCND_LOGE("[%s][open][ret:%d]\n",__func__, fd);
		return 0;
	}
	cmd   = 0x10; //WIFI_DRIVER_PRIV_CMD_SET_CP2_ASSERT
	data  = 0x00;
	len   = 0x04;
	index = 0x00;
	memcpy( &buf[index],  (char *)(&len),  4);
	index += 4;
	memcpy( &buf[index],  (char *)(&data), 4);
	index += 4;
	ret = ioctl(fd, cmd, &buf[0] );
	if(ret < 0)
	{
		WCND_LOGE("[%s][ioctl][ret:%d]\n",__func__, ret);
		close(fd);
		return 0;
	}
	memcpy( (char *)(&ret), &buf[0], 4);
	if(0x11 == ret) //WIFI_DRIVER_STATE_REMOVEING
	{
		WCND_LOGE("marlin wifi drv removing\n");
		return 1;
	}
	close(fd);

	WCND_LOGD("notify marlin wifi drv cp2 assert success\n");

	return 0;

}


/**
* To find the "target" file or dir in the specified path.
* special case: to find the file or dir, its name contains the "target" string.
*
* return  0 : not foud
*  1 : find it
* -1 : error
*/
int wcnd_find(const char *path, const char* target)
{
	struct stat buf;
	int is_dir = 0;

	if (!path || !target) return -1;

	int r = stat( path, &buf );
	if ( r == 0 )
	{
		if(S_ISDIR(buf.st_mode))
			is_dir = 1;
	}
	else
	{
		WCND_LOGE("error for _stat(%s):(error:%s)\n", path, strerror(errno));
		return -1;
	}


	if (!is_dir)
	{
		if (strstr(path, target))
			return 1;
		else
			return 0;
	}

	//path is dir
	DIR *find_dir = NULL;
	struct dirent *entry = NULL;
	int ret = 0;

	find_dir = opendir(path);

	if(!find_dir)
	{
		WCND_LOGE("open %s fail: %s", path, strerror(errno));
		return -1;
	}

	while((entry = readdir(find_dir)))
	{
		char buf[1024];


		if (strstr(entry->d_name, target))
		{
			WCND_LOGD("found: %s for %s\n", entry->d_name, target);
			ret = 1;
			goto out;
		}

	} /* for (;;) */

out:
	if(find_dir) closedir(find_dir);

	return ret;
}

/**
 * To stop the process with the specified name.
 * in: const char *proc_name -->  the name of the process
 * in: int time_out --> time out value in seconds to wait process to be stopped
 * Return:
 *    1: success
 *    0: timeout
 *    -1: fail
 */
int wcnd_stop_process(const char *proc_name, int time_out) {

	if (!proc_name) return -1;

	int ret = wcnd_kill_process_by_name(proc_name, SIGINT);

	if(-2 != ret) // have send sig to target process, wait it to exit
	{
		//check if process is killed

		int count = 50;
		if (time_out > 0) count = time_out * 10;

		while(count > 0)
		{
			int check_ret = wcnd_check_process_exist(proc_name, ret);
			if(!check_ret || (check_ret > 0 && ret > 0 && check_ret != ret)) //if the target process does not exist or the original exited, and a new process is detected
				break;
			else
				count--;

			if (count == 0)
				return 0; // timeout

			usleep(100*1000); //100ms
		}
	}

	return 1;
}

