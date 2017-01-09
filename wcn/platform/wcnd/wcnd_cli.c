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


#define TEST_STOP "stop"
#define TEST_START "start"
#define TEST_DOWNLOAD "download"
#define TEST_RESET "reset"
#define TEST_POLLING "poll"
#define TEST_ENG_CMD "eng"
#define TEST_WCN_CMD "wcn"

static int client_fd = -1;



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
		printf("%s: fail SIOCDEVPRIVATE(%s)\n", __func__, strerror(errno));
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

	printf("%s: get wl reply: %s, ret = %d \n",__func__, buf, ret);

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

	printf("%s: get wl reply: %s, ret = %d \n",__func__, buf, ret);

	return 0;
}


static int set_wifi_power_mode(int power_mode)
{
	struct ifreq ifr;
	int ret;

	sprintf(ifr.ifr_name, "%s", WLAN_IFNAME);


	ret =  wl_ioctl(&ifr, WLC_SET_PM, (char*)&power_mode, sizeof(power_mode), 1);

	printf("%s ret = %d \n",__func__, ret);

	return ret;
}


static int get_wifi_power_mode(void)
{
	struct ifreq ifr;
	int ret;
	int power_mode = -1;

	sprintf(ifr.ifr_name, "%s", WLAN_IFNAME);


	ret =  wl_ioctl(&ifr, WLC_GET_PM, (char*)&power_mode, sizeof(power_mode), 0);

	printf("%s ret = %d, power_mode = %d \n",__func__, ret, power_mode);

	return power_mode;
}


static void connect_wcnd(int eng_mode)
{
	char *socket_name = WCND_SOCKET_NAME;

	if(eng_mode)
	{
		socket_name = WCND_ENG_SOCKET_NAME;
	}

	client_fd = socket_local_client( socket_name,
		ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);

	while(client_fd < 0)
	{
		printf("%s: Unable bind server %s, waiting...\n",__func__, socket_name);
		usleep(100*1000);
		client_fd = socket_local_client( socket_name,
			ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
	}

	return;
}


int main(int argc, char *argv[])
{
	if(argc < 2)
	{
		printf("Usage: wcnd_cli <cmd>\n");
		return -1;

	}


	if(argc > 2)
	{
		if(!strcmp(argv[1], "sleepmode"))
		{
			int sleep_mode = atoi(argv[2]);

			printf("set sleep_mode = %d\n", sleep_mode);

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

			return 0;

		}

	}
	else if( (argc == 2) && (!strcmp(argv[1], "sleepmode")))
	{
		int power_mode = get_wifi_power_mode();

		printf("current PM: %d\n", power_mode);

		return 0;
	}

	int engmode = 0;

	if(!strcmp(argv[1], "eng")) engmode = 1;


	connect_wcnd(engmode);

	char cmd[255];
	int i = 0, length = 0, offset = 0;

	memset(cmd, 0, sizeof(cmd));

	for(i=1; i<argc; i++) {
		length = strlen(argv[i]);
		snprintf(cmd+offset, 254-offset, "%s ", argv[i]);
		offset += (length+1);

	}

	//remove the last ''
	cmd[strlen(cmd)-1] = '\0';

	printf("cmd: %s\n", cmd);

	TEMP_FAILURE_RETRY(write(client_fd, cmd, strlen(cmd)));

	for(;;)
	{
		char buffer[128];
		int n = 0;
		memset(buffer, 0, 128);
		printf("%s: waiting for server %s\n",__func__, WCND_SOCKET_NAME);
		n = read(client_fd, buffer, 128);
		printf("%s: get %d bytes %s\n", __func__, n, buffer);
		if(n >= 0) break;
	}	

	close(client_fd);
	return 0;
}

//##########################################################################################################
//For Test stop
//##########################################################################################################

