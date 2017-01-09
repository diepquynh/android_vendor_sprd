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

static int set_engpc_service(WcndManager *pWcndManger, int start)
{
	//if(pWcndManger->is_eng_mode_only) return 0;

	//if(!pWcndManger->is_wcn_modem_enabled) return 0;

	char prop[PROPERTY_VALUE_MAX] = {'\0'};
	WCND_LOGD("start engservice!");
	property_get(WCND_ENGCTRL_PROP_KEY,prop, "0");

	if(start && !strcmp(prop, "1"))
	{
		WCND_LOGD("Warning: persist.engpc.disable is true but APP want to start ");
		//return 0;
	}
	else if(!start && !strcmp(prop, "0"))
	{
		WCND_LOGD("Warning: persist.engpc.disable is false but APP want to stop ");
	}

	if (start)
	{
		property_set("ctl.start", "engservicewcn");//not used just now
		property_set("ctl.start", "engmodemclientwcn");//not used just now
		property_set("ctl.start", "engpcclientwcn");
	}
	else
	{
		property_set("ctl.stop", "engservicewcn");//not used just now
		property_set("ctl.stop", "engmodemclientwcn");//not used just now
		property_set("ctl.stop", "engpcclientwcn");
	}

	return 0;
}


/**
* Note:
* For the connection that will only be used to send a commond, when handle the command from this
* connection, its client type will be set to WCND_CLIENT_TYPE_CMD. With this type, after the command
* result is send back, its client type will be set to WCND_CLIENT_TYPE_SLEEP, then when there is an event
* to notify, these connections will be ignored.
*/

static int wcn_process_btwificmd(int client_fd, char* cmd_str, WcndManager *pWcndManger)
{
	if(!pWcndManger || !cmd_str) return -1;

#ifdef WCND_STATE_MACHINE_ENABLE

	//if not use our own wcn, just return
	if(!pWcndManger->is_wcn_modem_enabled)
	{
		wcnd_send_notify_to_client(pWcndManger, WCND_CMD_RESPONSE_STRING" OK", WCND_CLIENT_TYPE_CMD);

		return 0;
	}


	WcndMessage message;

	message.event = 0;
	message.replyto_fd = client_fd;

	if(!strcmp(cmd_str, WCND_CMD_BT_CLOSE_STRING))
		message.event = WCND_EVENT_BT_CLOSE;
	else if(!strcmp(cmd_str, WCND_CMD_BT_OPEN_STRING))
		message.event = WCND_EVENT_BT_OPEN;
	else if(!strcmp(cmd_str, WCND_CMD_WIFI_CLOSE_STRING))
		message.event = WCND_EVENT_WIFI_CLOSE;
	else if(!strcmp(cmd_str, WCND_CMD_WIFI_OPEN_STRING))
		message.event = WCND_EVENT_WIFI_OPEN;
	else if(!strcmp(cmd_str, WCND_CMD_FM_CLOSE_STRING))
		message.event = WCND_EVENT_FM_CLOSE;
	else if(!strcmp(cmd_str, WCND_CMD_FM_OPEN_STRING))
		message.event = WCND_EVENT_FM_OPEN;

	return wcnd_sm_step(pWcndManger, &message);
#else

	wcnd_send_notify_to_client(pWcndManger, WCND_CMD_RESPONSE_STRING" OK", WCND_CLIENT_TYPE_CMD);

	return 0;
#endif
}


/**
* to check the CP2 state, and decide whether to real send cmd to CP2 or not.
* return <0, do not send cmd to CP2
* return others, need to send cmd to CP2
* out: atcmd_id, to specify the at cmd for getting cp2 version or set cp2 sleep or others
*/
static int before_send_atcmd(int client_fd, char *atcmd_str, WcndManager *pWcndManger, int *atcmd_id)
{
	char buffer[255] ;
	int to_get_cp2_version = 0;
	int send_back_is_done = 0;
	int to_tell_cp2_sleep = 0;

	memset(buffer, 0, sizeof(buffer));

	//check if it is going to get cp2 version
	if(strcasestr(atcmd_str, "spatgetcp2info"))
	{
		WCND_LOGD("%s: To get cp2 version", __func__);
		to_get_cp2_version = 1;
		if(atcmd_id)
			*atcmd_id = WCND_ATCMD_CMDID_GET_VERSION;
	}
	else if(!strcmp(atcmd_str, WCND_ATCMD_CP2_SLEEP))
	{
		WCND_LOGD("%s: To tell cp2 to sleep ", __func__);
		to_tell_cp2_sleep = 1;
		if(atcmd_id)
			*atcmd_id = WCND_ATCMD_CMDID_SLEEP;
	}

	//check if it is cp2 log setting
	if(strcasestr(atcmd_str, "at+armlog=0"))
	{
		pWcndManger->is_cp2log_opened = 0;
	}
	else if(strcasestr(atcmd_str, "at+armlog=1"))
	{
		pWcndManger->is_cp2log_opened = 1;
	}


	//if cp2 is not in normal state
	if(pWcndManger->state != WCND_STATE_CP2_STARTED)
	{
		if(to_get_cp2_version)//get cp2 version, use saved instead, if cp2 not in normal state
		{
			snprintf(buffer, 254, "%s", pWcndManger->cp2_version_info);

			WCND_LOGD("%s: Save version info: '%s'", __func__, buffer);

			//send back the response
			if(client_fd > 0)
			{
				int ret = write(client_fd, buffer, strlen(buffer)+1);
				if(ret < 0)
				{
					WCND_LOGE("write %s to client_fd:%d fail (error:%s)", buffer, client_fd, strerror(errno));
				}
			}

			send_back_is_done = 1;
		}
		else if (!to_tell_cp2_sleep)//other cmds except sleep cmd
		{
			WCND_LOGD("%s: CP2 is not in normal state (%d), at cmd FAIL", __func__, pWcndManger->state);

			snprintf(buffer, 254, "%s", WCND_CP2_CLOSED_STRING);

			wcnd_send_back_cmd_result(client_fd, buffer, 0);

			send_back_is_done = 1;
		}

	}
	else
	{
		//This case only happens for WCND_STATE_MACHINE_ENABLE is not opened.
		if(pWcndManger->is_cp2_error)
		{
			WCND_LOGD("%s: CP2 is assert, at cmd FAIL", __func__);

			snprintf(buffer, 254, "%s", WCND_CP2_EXCEPTION_STRING);

			wcnd_send_back_cmd_result(client_fd, buffer, 0);

			send_back_is_done = 1;
		}
	}


	if(send_back_is_done)
		return -1; //do not real send at cmd to cp2
	else
		return 0;

}


/**
* client_fd: the fd to send back the comand response
* return < 0 for cp2 return fail or do not real send cmd to cp2 because cp2 is not in normal state
*/
int wcnd_process_atcmd(int client_fd, char *atcmd_str, WcndManager *pWcndManger)
{
	int len = 0;
	int atcmd_fd = -1;
	char buffer[255] ;
	int atcmd_id = 0;
	int ret_value = -1;
	int to_tell_cp2_sleep = 0;


	//First check if wcn modem is enabled or not, if not, the at cmd is not supported
	if(pWcndManger && !pWcndManger->is_wcn_modem_enabled)
	{
		wcnd_send_back_cmd_result(client_fd, "WCN Disabled", 0);
		return -1;
	}

	if( !atcmd_str || !pWcndManger)
		return -1;

	int atcmd_len = strlen(atcmd_str);

	WCND_LOGD("%s: Receive AT CMD: %s, len = %d", __func__, atcmd_str, atcmd_len);


	//do some check, before real send to cp2
	if(before_send_atcmd(client_fd, atcmd_str, pWcndManger, &atcmd_id) < 0)
	{
		WCND_LOGD("%s: CP2 not in normal state, do not real send atcmd to it", __func__);
		return -1;
	}


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
		wcnd_send_back_cmd_result(client_fd, "Send atcmd fail", 0);
		return -1;
	}

	len = write(atcmd_fd, buffer, atcmd_len);
	if(len < 0)
	{
		WCND_LOGE("%s: write %s failed, error:%s", __func__, pWcndManger->wcn_atcmd_iface_name, strerror(errno));
		close(atcmd_fd);
		wcnd_send_back_cmd_result(client_fd, "Send atcmd fail", 0);
		return -1;
	}

	//wait
	usleep(100*1000);

	WCND_LOGD("%s: Wait ATcmd to return", __func__);

	//Get AT Cmd Response
	int try_counts = 0;

try_again:
	if(try_counts++ > 5)
	{
		WCND_LOGE("%s: wait for response fail!!!!!", __func__);
		if(WCND_ATCMD_CMDID_GET_VERSION == atcmd_id) //use saved version info instead
		{
			snprintf(buffer, 254, "%s", pWcndManger->cp2_version_info);
			ret_value = 0;
		}
		else
			snprintf(buffer, 254, "Fail: No data available");

	}
	else
	{
		memset(buffer, 0, sizeof(buffer));

		do {
			len = read(atcmd_fd, buffer, sizeof(buffer)-1);
		} while(len < 0 && errno == EINTR);

		if ((len <= 0))
		{
			WCND_LOGE("%s: read fd(%d) return len(%d), errno = %s", __func__, atcmd_fd , len, strerror(errno));
			usleep(300*1000);
			goto try_again;
		}
		else
		{

			if(strstr(buffer, "fail") || strstr(buffer, "FAIL")) //cp2 return fail
			{
				ret_value = -1;
			}
			else
			{
				//save the CP2 version info
				if(WCND_ATCMD_CMDID_GET_VERSION == atcmd_id)
					memcpy(pWcndManger->cp2_version_info, buffer, sizeof(buffer));

				ret_value = 0;
			}

		}
	}

	WCND_LOGD("%s: ATcmd to %s return: '%s'", __func__, pWcndManger->wcn_atcmd_iface_name, buffer);

	close(atcmd_fd);

	if(client_fd <= 0)
	{
		WCND_LOGE("Write '%s' to Invalid client_fd, ret_value: %d", buffer, ret_value);
		return ret_value;
	}

	//send back the response
	int ret = write(client_fd, buffer, strlen(buffer)+1);
	if(ret < 0)
	{
		WCND_LOGE("write %s to client_fd:%d fail (error:%s)", buffer, client_fd, strerror(errno));
		return -1;
	}

	return ret_value;
}

int wcnd_runcommand(int client_fd, int argc, char* argv[])
{
	WcndManager *pWcndManger = wcnd_get_default_manager();

	if(argc < 1)
	{
		wcnd_send_back_cmd_result(client_fd, "Missing argument", 0);
		return 0;
	}

#if 1
	int k = 0;
	for (k = 0; k < argc; k++)
	{
		WCND_LOGD("%s: arg[%d] = '%s'", __FUNCTION__, k, argv[k]);
	}
#endif

	if (!strcmp(argv[0], "reset"))
	{
		if(client_fd != pWcndManger->selfcmd_sockets[1])
		{
			//tell the client the reset cmd is executed
			wcnd_send_back_cmd_result(client_fd, NULL, 1);
		}

		if(pWcndManger->is_cp2_error)
			wcnd_do_wcn_reset_process(pWcndManger);
	}
	else if(!strcmp(argv[0], "test"))
	{
		WCND_LOGD("%s: do nothing for test cmd", __FUNCTION__);
		wcnd_send_back_cmd_result(client_fd, NULL, 1);
	}
	else if(strstr(argv[0], "at+")) //at cmd
	{
		WCND_LOGD("%s: AT cmd(%s)(len=%d)", __FUNCTION__, argv[0], strlen(argv[0]));
		wcnd_process_atcmd(client_fd, argv[0], pWcndManger);
	}
	else if(strstr(argv[0], "BT") || strstr(argv[0], "WIFI") || strstr(argv[0], "FM")) //bt/wifi/fm cmd
	{
		int i = 0;
		//to set the type to be cmd
		pthread_mutex_lock(&pWcndManger->clients_lock);
		for (i = 0; i < WCND_MAX_CLIENT_NUM; i++)
		{
			if(pWcndManger->clients[i].sockfd == client_fd)
			{
				pWcndManger->clients[i].type = WCND_CLIENT_TYPE_CMD;
				break;
			}
		}
		pthread_mutex_unlock(&pWcndManger->clients_lock);

		wcn_process_btwificmd(client_fd, argv[0], pWcndManger);

	}
	else if(!strcmp(argv[0], WCND_SELF_CMD_START_CP2))
	{

		wcnd_open_cp2(pWcndManger);
	}
	else if(!strcmp(argv[0], WCND_SELF_CMD_STOP_CP2))
	{

		wcnd_close_cp2(pWcndManger);
	}
	else if(!strcmp(argv[0], WCND_SELF_CMD_PENDINGEVENT))
	{
		WcndMessage message;

		message.event = WCND_EVENT_PENGING_EVENT;
		message.replyto_fd = -1;
		wcnd_sm_step(pWcndManger, &message);
	}
	else if(!strcmp(argv[0], WCND_SELF_EVENT_CP2_ASSERT))
	{
#ifdef WCND_STATE_MACHINE_ENABLE

		WcndMessage message;

		message.event = WCND_EVENT_CP2_ASSERT;
		message.replyto_fd = -1;
		wcnd_sm_step(pWcndManger, &message);
#else
		//to do reset directly
		wcnd_send_selfcmd(pWcndManger, "wcn reset");
#endif
	}
	else if(!strcmp(argv[0], WCND_CMD_CP2_POWER_ON))
	{
		WcndMessage message;
		int i = 0;

		//to set the type to be cmd
		pthread_mutex_lock(&pWcndManger->clients_lock);
		for (i = 0; i < WCND_MAX_CLIENT_NUM; i++)
		{
			if(pWcndManger->clients[i].sockfd == client_fd)
			{
				pWcndManger->clients[i].type = WCND_CLIENT_TYPE_CMD;
				break;
			}
		}
		pthread_mutex_unlock(&pWcndManger->clients_lock);

#ifdef WCND_STATE_MACHINE_ENABLE

		//if not use our own wcn, just return
		if(!pWcndManger->is_wcn_modem_enabled)
		{
			wcnd_send_notify_to_client(pWcndManger, WCND_CMD_RESPONSE_STRING" OK", WCND_CLIENT_TYPE_CMD);

			return 0;
		}

#ifdef USE_MARLIN
		//special handle for marlin2
		if (pWcndManger->marlin_type == MARLIN_CHIP_TYPE_2)
		{
			wcnd_set_marlin2_poweron(pWcndManger, 1);
		}
#endif

		message.event = WCND_EVENT_CP2POWERON_REQ;

		if(client_fd == pWcndManger->selfcmd_sockets[1])
		{
			WCND_LOGD("%s: CP2 POWERON REQ from self", __FUNCTION__);
			message.replyto_fd = -1;
		}
		else
			message.replyto_fd = client_fd;

		wcnd_sm_step(pWcndManger, &message);
#else

		wcnd_send_notify_to_client(pWcndManger, WCND_CMD_RESPONSE_STRING" OK", WCND_CLIENT_TYPE_CMD);

		return 0;

#endif


	}
	else if(!strcmp(argv[0], WCND_CMD_CP2_POWER_OFF))
	{
		WcndMessage message;
		int i = 0;

		//to set the type to be cmd
		pthread_mutex_lock(&pWcndManger->clients_lock);
		for (i = 0; i < WCND_MAX_CLIENT_NUM; i++)
		{
			if(pWcndManger->clients[i].sockfd == client_fd)
			{
				pWcndManger->clients[i].type = WCND_CLIENT_TYPE_CMD;
				break;
			}
		}
		pthread_mutex_unlock(&pWcndManger->clients_lock);

#ifdef WCND_STATE_MACHINE_ENABLE

		//if not use our own wcn, just return
		if(!pWcndManger->is_wcn_modem_enabled)
		{
			wcnd_send_notify_to_client(pWcndManger, WCND_CMD_RESPONSE_STRING" OK", WCND_CLIENT_TYPE_CMD);

			return 0;
		}

#ifdef USE_MARLIN
		//special handle for marlin2
		if (pWcndManger->marlin_type == MARLIN_CHIP_TYPE_2)
		{
			wcnd_set_marlin2_poweron(pWcndManger, 0);
		}
#endif

		message.event = WCND_EVENT_CP2POWEROFF_REQ;
		if(client_fd == pWcndManger->selfcmd_sockets[1])
		{
			WCND_LOGD("%s: CP2 POWEROFF REQ from self", __FUNCTION__);
			message.replyto_fd = -1;
		}
		else
			message.replyto_fd = client_fd;

		wcnd_sm_step(pWcndManger, &message);
#else

		wcnd_send_notify_to_client(pWcndManger, WCND_CMD_RESPONSE_STRING" OK", WCND_CLIENT_TYPE_CMD);

		return 0;

#endif

	}
	else if(!strcmp(argv[0], WCND_SELF_CMD_CONFIG_CP2))
	{
		WCND_LOGD("%s: do some config for cp2", __FUNCTION__);
		wcnd_config_cp2_bootup(pWcndManger);
	}
	else if(!strcmp(argv[0], WCND_SELF_CMD_CP2_VERSION))
	{
		WCND_LOGD("%s: store the cp2 version info", __FUNCTION__);
		wcnd_process_atcmd(-1, WCND_ATCMD_CP2_GET_VERSION, pWcndManger);
		pWcndManger->store_cp2_versin_done = 1;
	}
//#ifdef
	else if(!strcmp(argv[0], "startengpc"))
	{
		wcnd_send_back_cmd_result(client_fd, NULL, 1);
		set_engpc_service(pWcndManger, 1);
	}
	else if(!strcmp(argv[0], "stopengpc"))
	{
		wcnd_send_back_cmd_result(client_fd, NULL, 1);
		set_engpc_service(pWcndManger, 0);
	}
//#endif
	else if(!strcmp(argv[0], WCND_CMD_CP2_DUMP_ON))
	{
		wcnd_send_back_cmd_result(client_fd, NULL, 1);
		if(pWcndManger) pWcndManger->dumpmem_on = 1;
	}
	else if(!strcmp(argv[0], WCND_CMD_CP2_DUMP_OFF))
	{
		wcnd_send_back_cmd_result(client_fd, NULL, 1);
		if(pWcndManger) pWcndManger->dumpmem_on = 0;
	}
	else if(!strcmp(argv[0], WCND_CMD_CP2_DUMPMEM))
	{
		wcnd_dump_cp2(pWcndManger);
		wcnd_send_back_cmd_result(client_fd, NULL, 1);
	}
	else if(!strcmp(argv[0], WCND_CMD_CP2_DUMPQUERY))
	{
		if(pWcndManger->dumpmem_on)
			wcnd_send_back_cmd_result(client_fd, "dump=1", 1);
		else
			wcnd_send_back_cmd_result(client_fd, "dump=0", 1);

	}
	else if(!strcmp(argv[0], WCND_SELF_EVENT_MARLIN2_CLOSED))
	{
#ifdef WCND_STATE_MACHINE_ENABLE

		WcndMessage message;

		message.event = WCND_EVENT_MARLIN2_CLOSED;
		message.replyto_fd = -1;
		wcnd_sm_step(pWcndManger, &message);
#endif
	}
	else if(!strcmp(argv[0], WCND_SELF_EVENT_MARLIN2_OPENED))
	{
#ifdef WCND_STATE_MACHINE_ENABLE

		WcndMessage message;

		message.event = WCND_EVENT_MARLIN2_OPENED;
		message.replyto_fd = -1;
		wcnd_sm_step(pWcndManger, &message);
#endif
	}
	else
		wcnd_send_back_cmd_result(client_fd, "Not support cmd", 0);


	return 0;
}


