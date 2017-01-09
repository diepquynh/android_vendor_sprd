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


//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
/**
* Related Download Function for the wcn
*/


/**
* check "persist.sys.sprd.wcnreset" property to see if reset cp2 or not.
* return non-zero for true.
*/
static int check_if_reset_enable(void)
{
	char value[PROPERTY_VALUE_MAX] = {'\0'};

	property_get(WCND_RESET_PROP_KEY, value, "0");
	int is_reset = atoi(value);

	return is_reset;
}


/**
* check "persist.sys.sprd.wcnlog" and "persist.sys.sprd.wcnlog.result"
* 1. if wcnlog == 0, just return.
* 2. if wcnlog == 1, wait until wcnlog.result == 1.
*/
#define WAIT_ONE_TIME (200)   /* wait for 200ms at a time */
                                  /* when polling for property values */
static void wait_for_dump_logs(void)
{
	const char *desired_status = "1";
	char value[PROPERTY_VALUE_MAX] = {'\0'};

	property_get(WCND_SLOG_ENABLE_PROP_KEY, value, "0");
	int slog_enable = atoi(value);
	if(!slog_enable)
		return;

	int maxwait = 300; // wait max 300 s for slog dump cp2 log
	int maxnaps = (maxwait * 1000) / WAIT_ONE_TIME;

	if (maxnaps < 1)
	{
		maxnaps = 1;
	}

	memset(value, 0, sizeof(value));

	while (maxnaps-- > 0)
	{
		usleep(WAIT_ONE_TIME * 1000);
		if (property_get(WCND_SLOG_RESULT_PROP_KEY, value, NULL))
		{
			if (strcmp(value, desired_status) == 0)
			{
				return;
			}
		}
	}
}


#ifdef USE_MARLIN

#define WCN_DOWNLOAD_SOCKET_NAME "external_wcn"

#define WCN_DOWNLOAD_CMD_REBOOT_WCN "rebootwcn" //to do a reset of wcn, need to bootup the cp2 again
#define WCN_DOWNLOAD_CMD_DUMP_WCN "dumpwcn" //to dump wcn mem only
#define WCN_DOWNLOAD_CMD_START_WCN "startwcn" //to bootup the cp2, use in power on CP2
#define WCN_DOWNLOAD_CMD_STOP_WCN "stopwcn" //used in power off CP2

#define WCN_DOWNLOAD_RESP_REBOOT_WCN "rebootwcn-ok" //the response of the reset cmd
#define WCN_DOWNLOAD_RESP_DUMP_WCN "dumpwcn-ok"
#define WCN_DOWNLOAD_RESP_START_WCN "startwcn-ok"
#define WCN_DOWNLOAD_RESP_STOP_WCN "stopwcn-ok"

#define WCN_DOWNLOAD_TIMEOUT (10) //seconds

enum {
	WCN_DOWNLOAD_CMDID_REBOOT_WCN = 1,
	WCN_DOWNLOAD_CMDID_DUMP_WCN,
	WCN_DOWNLOAD_CMDID_START_WCN,
	WCN_DOWNLOAD_CMDID_STOP_WCN,
};

static int connect_download(void)
{
	int client_fd = -1;
	int retry_count = 20;
	struct timeval rcv_timeout; 

	client_fd = socket_local_client( WCN_DOWNLOAD_SOCKET_NAME,
	ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);

	while(client_fd < 0 && retry_count > 0)
	{
		retry_count--;
		WCND_LOGD("%s: Unable bind server %s, waiting (error:%s)...\n",__func__, WCN_DOWNLOAD_SOCKET_NAME, strerror(errno));
		usleep(100*1000);
		client_fd = socket_local_client( WCN_DOWNLOAD_SOCKET_NAME,
			ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);
	}

	if(client_fd > 0)
	{
		rcv_timeout.tv_sec = WCN_DOWNLOAD_TIMEOUT;
		rcv_timeout.tv_usec = 0;
		if(setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&rcv_timeout, sizeof(rcv_timeout)) < 0)
		{
			WCND_LOGE("%s: set receive timeout fail\n",__func__);
		}
	}

	return client_fd;
}


static int close_download(int fd)
{
	close(fd);

	return 0;
}

static int do_download_cmd(int cmd_id)
{
	char buffer[128];
	int n = 0;
	char *cmd_str = NULL, *expect_resp_str = NULL;

	switch(cmd_id)
	{
		case WCN_DOWNLOAD_CMDID_REBOOT_WCN:
			cmd_str = WCN_DOWNLOAD_CMD_REBOOT_WCN;
			expect_resp_str = WCN_DOWNLOAD_RESP_REBOOT_WCN;
			break;

		case WCN_DOWNLOAD_CMDID_DUMP_WCN:
			cmd_str = WCN_DOWNLOAD_CMD_DUMP_WCN;
			expect_resp_str = WCN_DOWNLOAD_RESP_DUMP_WCN;
			break;

		case WCN_DOWNLOAD_CMDID_START_WCN:
			cmd_str = WCN_DOWNLOAD_CMD_START_WCN;
			expect_resp_str = WCN_DOWNLOAD_RESP_START_WCN;
			break;

		case WCN_DOWNLOAD_CMDID_STOP_WCN:
			cmd_str = WCN_DOWNLOAD_CMD_STOP_WCN;
			expect_resp_str = WCN_DOWNLOAD_RESP_STOP_WCN;
			break;

		default:
			return -1;
	}

	if(!cmd_str || !expect_resp_str) return -1;

	int download_fd = connect_download();

	if(download_fd < 0) return -1;

	//write reboot cmd
	int ret = TEMP_FAILURE_RETRY(write(download_fd, cmd_str, strlen(cmd_str)));
	if(ret < 0)
	{
		WCND_LOGE("write %s bytes to client_fd:%d fail (error:%s)", cmd_str, download_fd, strerror(errno));
		close_download(download_fd);
		return -1;
	}

	//wait reboot complete
	memset(buffer, 0, sizeof(buffer));

	WCND_LOGD("%s: waiting for download (%s)\n",__func__, WCN_DOWNLOAD_SOCKET_NAME);
	n = read(download_fd, buffer, sizeof(buffer)-1);

	WCND_LOGD("%s: get %d bytes %s\n", __func__, n, buffer);

	if(!strstr(buffer, expect_resp_str))
		ret = -1;	
	else
		ret = 0;

	close_download(download_fd);

	return ret;

}


/**
* send AT comd
*/
static int send_atcmd( WcndManager *pWcndManger, char *atcmd_str)
{
	int len = 0;
	int atcmd_fd = -1;
	char buffer[255] ;

	//First check if wcn modem is enabled or not, if not, the at cmd is not supported
	if(pWcndManger && !pWcndManger->is_wcn_modem_enabled)
	{
		return -1;
	}

	if( !atcmd_str || !pWcndManger)
		return -1;

	int atcmd_len = strlen(atcmd_str);

	WCND_LOGD("%s: Receive AT CMD: %s, len = %d", __func__, atcmd_str, atcmd_len);


	//Below send at command to CP2

	memset(buffer, 0, sizeof(buffer));

	snprintf(buffer, 255, "%s", atcmd_str);

	//at cmd shoud end with '\r'
	if((atcmd_len < 254) && (buffer[atcmd_len - 1] != '\r'))
	{
		buffer[atcmd_len] = '\r';
		atcmd_len++;
	}

	atcmd_fd = open( pWcndManger->wcn_atcmd_iface_name, O_RDWR|O_NONBLOCK);
	WCND_LOGD("%s: open at cmd interface: %s, fd = %d", __func__, pWcndManger->wcn_atcmd_iface_name, atcmd_fd);
	if (atcmd_fd < 0)
	{
		WCND_LOGE("open %s failed, error: %s", pWcndManger->wcn_atcmd_iface_name, strerror(errno));
		return -1;
	}

	len = write(atcmd_fd, buffer, atcmd_len);
	if(len < 0)
	{
		WCND_LOGE("%s: write %s failed, error:%s", __func__, pWcndManger->wcn_atcmd_iface_name, strerror(errno));
		close(atcmd_fd);
		return -1;
	}

	return 0;
}

/**
* power on/off marlin2
*/
int wcnd_set_marlin2_poweron(WcndManager *pWcndManger, int poweron)
{
	if(!pWcndManger)
	{
		WCND_LOGD("%s: UNEXCPET NULL WcndManager", __FUNCTION__);
		return -1;
	}

	if(poweron)
	{
		send_atcmd(pWcndManger, "startwcn");
	}
	else
	{
		send_atcmd(pWcndManger, "stopwcn");
	}

	return 0;
}



/**
* Return <0 for fail
*/
int wcnd_reboot_cp2(WcndManager *pWcndManger)
{
	if(!pWcndManger)
	{
		WCND_LOGD("%s: UNEXCPET NULL WcndManager", __FUNCTION__);
		return -1;
	}

	WCND_LOGD("reset_cp2");

	//for marlin2, power on/off is handle by driver
	if(pWcndManger->marlin_type == MARLIN_CHIP_TYPE_2)
		return 0;

	return do_download_cmd(WCN_DOWNLOAD_CMDID_REBOOT_WCN);
}


/**
* stop CP2
*/
int wcnd_stop_cp2(WcndManager *pWcndManger)
{
	if(!pWcndManger)
	{
		WCND_LOGD("%s: UNEXCPET NULL WcndManager", __FUNCTION__);
		return -1;
	}

	WCND_LOGD("wcnd_stop_cp2");

	//for marlin2, power on/off is handle by driver
	if(pWcndManger->marlin_type == MARLIN_CHIP_TYPE_2)
		return 0;

	return do_download_cmd(WCN_DOWNLOAD_CMDID_STOP_WCN);
}


int wcnd_start_cp2(WcndManager *pWcndManger)
{
	if(!pWcndManger)
	{
		WCND_LOGD("%s: UNEXCPET NULL WcndManager", __FUNCTION__);
		return -1;
	}

	WCND_LOGD("wcnd_start_cp2");

	//for marlin2, power on/off is handle by driver
	if(pWcndManger->marlin_type == MARLIN_CHIP_TYPE_2)
		return 0;

	return do_download_cmd(WCN_DOWNLOAD_CMDID_START_WCN);
}


/**
* do some check, before reseting.
* return 0 for "reset" can be going on
* return -1 for "reset can not be going on
*/
int wcnd_before_reset(WcndManager *pWcndManger)
{

	if(!pWcndManger) return -1;

	//special handle for marlin2
	if (pWcndManger->marlin_type == MARLIN_CHIP_TYPE_2)
	{
		if (pWcndManger->is_in_userdebug ||
			(!pWcndManger->is_in_userdebug && pWcndManger->dumpmem_on)||!check_if_reset_enable())
		{
			//WCND_LOGD("Send 'dumpmem' cmd before notify slog!!");

			//send "dumpmem" cmd to cp2 first
			send_atcmd(pWcndManger, WCND_CMD_CP2_DUMPMEM);

			//notify slog
			//WCND_LOGD("Notify slog to dump");

			//char buffer[255];
			//snprintf(buffer, 255, "%s", WCND_CP2_EXCEPTION_STRING);

			//wcnd_send_notify_to_client(pWcndManger, buffer, WCND_CLIENT_TYPE_NOTIFY);

			WCND_LOGD("Wait slog to dump");
			//wait slog dump complete
			wait_for_dump_logs();

			WCND_LOGD("Wait slog dump complete");
		}

		return -1; //do not going to do reset
	}

	if(!pWcndManger->is_in_userdebug && pWcndManger->dumpmem_on)
	{// in user mode, and dump mem is on

		WCND_LOGD("%s: in user mode but dump mem is on!!", __FUNCTION__);

		do_download_cmd(WCN_DOWNLOAD_CMDID_DUMP_WCN);

		return -1; //do not going to do reset
	}
	else if(pWcndManger->is_in_userdebug || !check_if_reset_enable())
	{
		WCND_LOGD("%s: in userdebug or reset disabled, do not do reset process!"
			"but for MARLIN need to do reset for dump mem!!", __FUNCTION__);

		do_download_cmd(WCN_DOWNLOAD_CMDID_DUMP_WCN);

		//return -1; //do not exit here, also need to check if need to reset.
	}

	if(!check_if_reset_enable())
	{
		WCND_LOGD("%s: wcn reset disabled, do not do reset!", __FUNCTION__);
		return -1;
	}

	//set doing_reset flag
	pWcndManger->doing_reset = 1;

	return 0;
}


/**
* to dump cp2 mem
*/
int wcnd_dump_cp2_mem(WcndManager *pWcndManger)
{

	//if(!pWcndManger) return -1;

	//dump mem
	if (pWcndManger->marlin_type == MARLIN_CHIP_TYPE_2)
	{
		//send "dumpmem" cmd to cp2
		send_atcmd(pWcndManger, WCND_CMD_CP2_DUMPMEM);
	}
	else
		do_download_cmd(WCN_DOWNLOAD_CMDID_DUMP_WCN);

	return 0;
}


#else


#if 0
/**
* check "persist.sys.sprd.wcnlog" and "persist.sys.sprd.wcnlog.result"
* 1. if wcnlog == 0, just return.
* 2. if wcnlog == 1, wait until wcnlog.result == 1.
*/
#define WAIT_ONE_TIME (200)   /* wait for 200ms at a time */
                                  /* when polling for property values */
static void wait_for_dump_logs(void)
{
	const char *desired_status = "1";
	char value[PROPERTY_VALUE_MAX] = {'\0'};

	property_get(WCND_SLOG_ENABLE_PROP_KEY, value, "0");
	int slog_enable = atoi(value);
	if(!slog_enable)
		return;

	int maxwait = 300; // wait max 300 s for slog dump cp2 log
	int maxnaps = (maxwait * 1000) / WAIT_ONE_TIME;

	if (maxnaps < 1)
	{
		maxnaps = 1;
	}

	memset(value, 0, sizeof(value));

	while (maxnaps-- > 0)
	{
		usleep(WAIT_ONE_TIME * 1000);
		if (property_get(WCND_SLOG_RESULT_PROP_KEY, value, NULL))
		{
			if (strcmp(value, desired_status) == 0)
			{
				return;
			}
		}
	}
}
#endif


#define min(x,y) (((x) < (y)) ? (x) : (y))

/**
* write image file pointed by the src_fd to the destination pointed by the dst_fd
* return -1 for fail
*/
static int write_image(int  src_fd, int src_offset, int dst_fd, int dst_offset, int size)
{
	if(src_fd < 0 || dst_fd < 0) return -1;

	int  bufsize, rsize, rrsize, wsize;
	char buf[8192];
	int buf_size = sizeof(buf);

	if (lseek(src_fd, src_offset, SEEK_SET) != src_offset)
	{
		WCND_LOGE("failed to lseek %d in fd: %d", src_offset, src_fd);
		return -1;
	}

	if (lseek(dst_fd, dst_offset, SEEK_SET) != dst_offset)
	{
		WCND_LOGE("failed to lseek %d in fd:%d", dst_offset, dst_fd);
		return -1;
	}

	int totalsize = 0;
	while(size > 0)
	{
		rsize = min(size, buf_size);
		rrsize = read(src_fd, buf, rsize);
		totalsize += rrsize;

		if(rrsize == 0)
		{
			WCND_LOGE("At the end of the file (totalsize: %d)", totalsize);
			break;
		}

		if (rrsize < 0  || rrsize > rsize)
		{
			WCND_LOGE("failed to read fd: %d (ret = %d)", src_fd, rrsize);
			return -1;
		}

		wsize = write(dst_fd, buf, rrsize);
		if (wsize != rrsize)
		{
			WCND_LOGE("failed to write fd: %d [wsize = %d  rrsize = %d  remain = %d]",
					dst_fd, wsize, rrsize, size);
			return -1;
		}
		size -= rrsize;
	}

	return 0;
}

/**
* reset the cp2:
* 1. stop cp2: echo "1"  > /proc/cpwcn/stop
* 2. download image: cat /dev/block/platform/sprd-sdhci.3/by-name/wcnmodem > /proc/cpwcn/modem
* 3. start cp2: echo "1" > /proc/cpwcn/start
* 4. polling /dev/spipe_wcn0, do write and read testing.
*/

//stop cp2: echo "1"  > /proc/cpwcn/stop
static inline int stop_cp2(WcndManager *pWcndManger)
{
	int stop_fd = -1;
	int len = 0;

	stop_fd = open(pWcndManger->wcn_stop_iface_name, O_RDWR);
	WCND_LOGD("%s: open stop interface: %s, fd = %d", __func__, pWcndManger->wcn_stop_iface_name, stop_fd);
	if (stop_fd < 0)
	{
		WCND_LOGE("open %s failed, error: %s", pWcndManger->wcn_stop_iface_name, strerror(errno));
		return -1;
	}

	//Stop cp2, write '1' to wcn_stop_iface
	len = write(stop_fd, "1", 1);
	if (len != 1)
	{
		WCND_LOGE("write 1 to %s to stop CP2 failed!!", pWcndManger->wcn_stop_iface_name);

		close(stop_fd);
		return -1;
	}
	close(stop_fd);

	WCND_LOGD("%s:%s is OK", __func__, pWcndManger->wcn_stop_iface_name);

	return 0;
}

//start cp2: echo "1" > /proc/cpwcn/start
static inline int start_cp2(WcndManager *pWcndManger)
{
	int start_fd = -1;
	int len = 0;

	//Start cp2
	start_fd = open( pWcndManger->wcn_start_iface_name, O_RDWR);
	WCND_LOGD("%s: open start interface: %s, fd = %d", __func__, pWcndManger->wcn_start_iface_name, start_fd);
	if (start_fd < 0)
	{
		WCND_LOGE("open %s failed, error: %s", pWcndManger->wcn_start_iface_name, strerror(errno));
		return -1;
	}

	len = write(start_fd, "1", 1);
	if (len != 1)
	{
		WCND_LOGE("write 1 to %s to stop CP2 failed!!", pWcndManger->wcn_stop_iface_name);

		close(start_fd);
		return -1;
	}
	close(start_fd);

	WCND_LOGD("%s:%s is OK", __func__, pWcndManger->wcn_start_iface_name);

	return 0;
}

//download image: cat /dev/block/platform/sprd-sdhci.3/by-name/wcnmodem > /proc/cpwcn/modem
static inline int download_image_to_cp2(WcndManager *pWcndManger)
{
	int download_fd = -1;
	int image_fd = -1;

	download_fd = open( pWcndManger->wcn_download_iface_name, O_RDWR);
	WCND_LOGD("%s: open download interface: %s, fd = %d", __func__, pWcndManger->wcn_download_iface_name, download_fd);
	if (download_fd < 0)
	{
		WCND_LOGE("open %s failed, error: %s", pWcndManger->wcn_download_iface_name, strerror(errno));
		return -1;
	}


	char image_file[256];

	memset(image_file, 0, sizeof(image_file));
	if ( -1 == property_get(PARTITION_PATH_PROP_KEY, image_file, "") )
	{
		WCND_LOGE("%s: get partitionpath fail",__func__);
		close(download_fd);
		return -1;
	}

	strcat(image_file, pWcndManger->wcn_image_file_name);

	image_fd = open( image_file, O_RDONLY);
	WCND_LOGD("%s: image file: %s, fd = %d", __func__, image_file, image_fd);
	if (image_fd < 0)
	{
		WCND_LOGE("open %s failed, error: %s", image_file, strerror(errno));

		close(download_fd);
		return -1;
	}


	WCND_LOGD("Loading %s in bank %s:%d %d", image_file, pWcndManger->wcn_download_iface_name,
		0, pWcndManger->wcn_image_file_size);

	//start downloading  image
	if(write_image(image_fd, 0, download_fd, 0, pWcndManger->wcn_image_file_size) < 0)
	{
		WCND_LOGE("Download IMAGE TO CP2  failed");
		close(image_fd);
		close(download_fd);
		return -1;
	}

	close(image_fd);
	close(download_fd);

	WCND_LOGD("%s:%s is OK", __func__, pWcndManger->wcn_download_iface_name);

	return 0;
}


int wcnd_reboot_cp2(WcndManager *pWcndManger)
{
	if(!pWcndManger)
	{
		WCND_LOGD("%s: UNEXCPET NULL WcndManager", __FUNCTION__);
		return -1;
	}

	WCND_LOGD("reset_cp2");

	if(stop_cp2(pWcndManger) < 0)
	{
		WCND_LOGE("Stop CP2 failed!!");
		return -1;
	}

	//usleep(100*1000);

	if(download_image_to_cp2(pWcndManger) < 0)
	{
		WCND_LOGE("Download image TO CP2 failed!!");
		return -1;
	}

	//usleep(100*1000);

	if(start_cp2(pWcndManger) < 0)
	{
		WCND_LOGE("Start CP2 failed!!");
		return -1;
	}

	return 0;
}


/**
* stop CP2
*/
int wcnd_stop_cp2(WcndManager *pWcndManger)
{
	return stop_cp2(pWcndManger);
}


int wcnd_start_cp2(WcndManager *pWcndManger)
{
	return wcnd_reboot_cp2(pWcndManger);
}


/**
* do some check, before reseting.
* return 0 for "reset" can be going on
* return -1 for "reset can not be going on
*/
int wcnd_before_reset(WcndManager *pWcndManger)
{

	if(!pWcndManger) return -1;

	if(!check_if_reset_enable())
	{
		WCND_LOGD("%s: wcn reset disabled, do not do reset!", __FUNCTION__);
		return -1;
	}

	//set doing_reset flag
	pWcndManger->doing_reset = 1;

	WCND_LOGD("wait_for_dump_logs");

	//wait slog may dump log.
	wait_for_dump_logs();

	WCND_LOGD("wait_for_dump_logs end");

	return 0;
}

/**
* to dump cp2 mem
*/
int wcnd_dump_cp2_mem(WcndManager *pWcndManger)
{

	//if(!pWcndManger) return -1;

	return 0;
}


#endif


