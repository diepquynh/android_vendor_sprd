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

#include "wcnd.h"
#include "wcnd_sm.h"




#include <cutils/properties.h>

#include <fcntl.h>
#include <sys/socket.h>


#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <errno.h>


#ifdef HAVE_SLEEPMODE_CONFIG


#define WLAN_IFNAME 	"wlan0"

#define WLC_GET_PM				85
#define WLC_SET_PM				86
#define WLC_GET_VAR 		262
#define WLC_SET_VAR 		263
#define WLC_GET_RSSI 		127
#define WLC_GET_CHANNEL		29



/* Linux network driver ioctl encoding */
typedef struct wl_ioctl {
	uint cmd;	/* common ioctl definition */
	char *buf;	/* pointer to user buffer */
	uint len;		/* length of user buffer */
	uint set;	/* get or set request (optional) */
	uint used;	/* bytes read or written (optional) */
	uint needed;	/* bytes needed (optional) */
} wl_ioctl_t;


static int
bcm_mkiovar(char *name, char *data, uint datalen, char *buf, uint buflen)
{
	uint len;

	len = strlen(name) + 1;
	if ((len + datalen) > buflen)
		return 0;

	strncpy(buf, name, buflen);

	/* append data onto the end of the name string */
	memcpy(&buf[len], data, datalen);
	len += datalen;

	return len;
}

static int
wl_ioctl(struct ifreq *ifr, int cmd, void *buf, int len, bool set)
{
	wl_ioctl_t ioc;
	int ret = 0;
	int s;

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("%s: fail get socket\n", __func__);
		return -1;
	}


	ioc.cmd = cmd;
	ioc.buf = buf;
	ioc.len = len;
	ioc.set = set;
	ifr->ifr_data = (caddr_t) &ioc;


	if ((ret = ioctl(s, SIOCDEVPRIVATE, ifr)) < 0) {
		WCND_LOGE("%s: fail SIOCDEVPRIVATE(%s)\n", __func__, strerror(errno));
	}

	close(s);
	return ret;
}


/**
* on: 1 for on
* 	 0 for off
*/
static int set_wifi_mpc_on(int on)
{
	char buf[256];
	int cmd_op = 0;

	int data = 0;

	struct ifreq ifr;
	int ret;

	sprintf(ifr.ifr_name, "%s", WLAN_IFNAME);

	data = on;
	cmd_op = WLC_SET_VAR;

	bcm_mkiovar("mpc", (char*)&data, 4, buf, 255);

	ret =  wl_ioctl(&ifr, cmd_op, buf, 255, 1);

	WCND_LOGD("%s: get wl reply: %s, ret = %d \n",__func__, buf, ret);

	return 0;
}



/**
* off: 1 for off
* 	 0 for on
*/
static int set_wifi_roam_off(int off)
{
	char buf[256];
	int cmd_op = 0;

	int data = 0;

	struct ifreq ifr;
	int ret;

	sprintf(ifr.ifr_name, "%s", WLAN_IFNAME);

	data = off;
	cmd_op = WLC_SET_VAR;

	bcm_mkiovar("roam_off", (char*)&data, 4, buf, 255);

	ret =  wl_ioctl(&ifr, cmd_op, buf, 255, 1);

	WCND_LOGD("%s: get wl reply: %s, ret = %d \n",__func__, buf, ret);

	return 0;
}


static int set_wifi_power_mode(int power_mode)
{
	struct ifreq ifr;
	int ret;

	sprintf(ifr.ifr_name, "%s", WLAN_IFNAME);


	ret =  wl_ioctl(&ifr, WLC_SET_PM, (char*)&power_mode, sizeof(power_mode), 1);

	WCND_LOGD("%s ret = %d \n",__func__, ret);

	return ret;
}


static int get_wifi_power_mode(void)
{
	struct ifreq ifr;
	int ret;
	int power_mode = -1;

	sprintf(ifr.ifr_name, "%s", WLAN_IFNAME);


	ret =  wl_ioctl(&ifr, WLC_GET_PM, (char*)&power_mode, sizeof(power_mode), 0);

	WCND_LOGD("%s ret = %d, power_mode = %d \n",__func__, ret, power_mode);

	return power_mode;
}

#endif

static int connect_netd(void)
{
	int client_fd = socket_local_client( "netd",
		ANDROID_SOCKET_NAMESPACE_RESERVED, SOCK_STREAM);

	while(client_fd < 0)
	{
		WCND_LOGD("%s: Unable bind server netd, waiting...\n",__func__);
		usleep(100*1000);
		client_fd = socket_local_client( "netd",
			ANDROID_SOCKET_NAMESPACE_RESERVED, SOCK_STREAM);
	}

	return client_fd;
}

static int wait_for_netd(int sock, int stop_after_cmd, char *resp_buffer, int buffer_size)
{
	char temp_buffer[256];
	char *buffer;
	int size = sizeof(temp_buffer);

	if(!resp_buffer)
	{
		buffer = temp_buffer;
	}
	else
	{
		buffer = resp_buffer;
		size = buffer_size;
		if(size <= 0)
		{
			WCND_LOGD("Invalid buffer size: %d\n", size);
			return -1;
		}
	}

	if (!stop_after_cmd)
		WCND_LOGD("[Connected to Netd]\n");

	while(1)
	{
		fd_set read_fds;
		struct timeval to;
		int rc = 0;

		to.tv_sec = 10;
		to.tv_usec = 0;

		FD_ZERO(&read_fds);
		FD_SET(sock, &read_fds);

		if ((rc = select(sock +1, &read_fds, NULL, NULL, &to)) < 0)
		{
			int res = errno;
			WCND_LOGD("Error in select (%s)\n", strerror(errno));
			return res;
		}
		else if (!rc)
		{
			//continue;
			WCND_LOGD("[TIMEOUT]\n");
			return ETIMEDOUT;
		}
		else if (FD_ISSET(sock, &read_fds))
		{
			memset(buffer, 0, size);
			if ((rc = read(sock, buffer, size-1)) <= 0)
			{
				int res = errno;
				if (rc == 0)
					WCND_LOGD("Lost connection to Netd - did it crash?\n");
				else
					WCND_LOGD("Error reading data (%s)\n", strerror(errno));

				if (rc == 0) return ECONNRESET;

				return res;
			}

			int offset = 0;
			int i = 0;

			for (i = 0; i < rc; i++)
			{
				if (buffer[i] == '\0')
				{
					int code;
					char tmp[4];

					strncpy(tmp, buffer + offset, 3);
					tmp[3] = '\0';
					code = atoi(tmp);

					WCND_LOGD("%s\n", buffer + offset);
					if (stop_after_cmd)
					{
						if (code >= 200 && code < 600)
						return 0;
					}
					offset = i + 1;
				}
			}
		}
	}
	return 0;
}


static int send_cmd_for_netd(char *cmd, int wait_resp, char *resp_buffer, int buffer_size)
{
	if (!cmd) return -1;

	WCND_LOGD("cmd: %s\n", cmd);

	int client_fd = connect_netd();

	if(client_fd < 0) return -1;

	TEMP_FAILURE_RETRY(write(client_fd, cmd, strlen(cmd)+1));

	if(wait_resp)
		wait_for_netd(client_fd, 1, resp_buffer, buffer_size);

	if(client_fd > 0) close(client_fd);

	return 0;
}

/**
* return 0: for cmd is executed correctly.
* return < 0: for fail
*/
static int check_response_from_netd(char *resp)
{
	if(!resp) return 0; //default for correctly

	char *ptr_space = NULL;

	ptr_space = strchr(resp, ' ');

	if(!ptr_space)//cannot find the ' ', there is not a result code, see it as correct
	{
		return 0;
	}
	else //check the result code
	{
		int code;
		char tmp[4];

		strncpy(tmp, resp, 3);
		tmp[3] = '\0';
		code = atoi(tmp);

		WCND_LOGD("%s\n", resp);
		if (code >= 400 && code < 600) //fail
			return -1;
	}

	return 0;
}

/**
* "softap setchan default" //to disable fixed channel
* "softap setchan fix 6" //to set the softap channel fixed on 6
* "softap setchan mode" //return the current setchan mode , "fixed" or "auto"
*/

static int handle_softap_cmd(int client_fd, int argc, char **argv)
{
	if(argc < 2)
	{
		wcnd_send_back_cmd_result(client_fd, "Missing argument", 0);
		return -1;
	}

	if(!strcmp(argv[0], "setchan"))
	{
		char cmd[255];
		char resp[256];
		int cmd_result = 0;

		memset(cmd, 0, sizeof(cmd));

		WCND_LOGD("here");

		if(!strcmp(argv[1], "default"))
		{
			//set chan default
			int i = 0, offset = 0;

			snprintf(cmd+offset, 254-offset, "0 softap ");

			for(i=0; i<argc; i++) {
				offset = strlen(cmd);
				snprintf(cmd+offset, 254-offset, "%s ", argv[i]);
			}

			//remove the last ' '
			cmd[strlen(cmd)-1] = '\0';

			send_cmd_for_netd(cmd, 1, resp, 256);

			cmd_result= check_response_from_netd(resp);

			if(cmd_result < 0) //cmd fail
			{
				wcnd_send_back_cmd_result(client_fd, resp, 0);
			}
			else //cmd ok
			{
				wcnd_send_back_cmd_result(client_fd, NULL, 1);
			}

		}
		else if(!strcmp(argv[1], "fix"))
		{
			if(argc < 3)
			{
				wcnd_send_back_cmd_result(client_fd, "Missing argument", 0);
				return -1;
			}

			//set chan fix in channel
			int i = 0, offset = 0;

			snprintf(cmd+offset, 254-offset, "0 softap ");

			for(i=0; i<argc; i++) {
				offset = strlen(cmd);
				snprintf(cmd+offset, 254-offset, "%s ", argv[i]);
			}

			//remove the last ' '
			cmd[strlen(cmd)-1] = '\0';

			send_cmd_for_netd(cmd, 1, resp, 256);

			cmd_result= check_response_from_netd(resp);

			if(cmd_result < 0) //cmd fail
			{
				wcnd_send_back_cmd_result(client_fd, resp, 0);
			}
			else //cmd ok
			{
				wcnd_send_back_cmd_result(client_fd, NULL, 1);
			}

		}
		else if(!strcmp(argv[1], "mode"))
		{
			//get set chan mode
			char *ptr_chanmode = NULL;

			snprintf(cmd, 254, "0 softap setchan mode");

			send_cmd_for_netd(cmd, 1, resp, 256);

			ptr_chanmode = strstr(resp, "CHANMODE");

			if(!ptr_chanmode)
				wcnd_send_back_cmd_result(client_fd, "CHANMODE=auto CHAN=6", 1);
			else
				wcnd_send_back_cmd_result(client_fd, ptr_chanmode, 1);
		}
		else
			wcnd_send_back_cmd_result(client_fd, "Unknown cmd", 0);

	}
	else
		wcnd_send_back_cmd_result(client_fd, "Unknown cmd", 0);


	return 0;
}



int wifi_runcommand(int client_fd, int argc, char **argv)
{
	if(argc < 1)
	{
		wcnd_send_back_cmd_result(client_fd, "Missing argument", 0);
		return -1;
	}


	if(!strcmp(argv[0], "softap"))
	{
		return handle_softap_cmd(client_fd, argc-1, &argv[1]);
	}

#ifdef HAVE_SLEEPMODE_CONFIG
	else if(!strcmp(argv[0], "sleepmode"))
	{
		if(argc > 1)
		{
			int sleep_mode = atoi(argv[1]);

			WCND_LOGD("set sleep_mode = %d\n", sleep_mode);

			//disable sleep
			if(!sleep_mode)
			{
				//set to active "wl PM 0"
				set_wifi_power_mode(0);

				//disable mpc "wl mpc 0"
				set_wifi_mpc_on(0);

				//disable roam "wl roam_off 1"
				set_wifi_roam_off(1);

				//disable changing mode
				set_wifi_power_mode(4);
			}
			else
			{
				//enable changing mode
				set_wifi_power_mode(5);
			}

			wcnd_send_back_cmd_result(client_fd, NULL, 1);
		}
		else
		{
			char buffer[64];
			int power_mode = get_wifi_power_mode();

			memset(buffer, 0, sizeof(buffer));
			snprintf(buffer, 63, "PM:%d",  power_mode);

			wcnd_send_back_cmd_result(client_fd, buffer, 1);
		}
	}
#endif

	else
		wcnd_send_back_cmd_result(client_fd, "Unknown cmd", 0);

	return 0;
}

