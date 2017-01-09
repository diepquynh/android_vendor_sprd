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



/**
* send notify information to the specified connected client.
* return -1 for fail
* Note: message send from wcnd must end with null character
*             use null character to identify a completed message.
*/
static int wcnd_notify_client(WcndManager *pWcndManger, char *info_str, int client_fd, int notify_type)
{
	int i, ret;

	if(!pWcndManger || !info_str) return -1;

	WCND_LOGD("notify_client (fd:%d, type: %d)", client_fd, notify_type);


	pthread_mutex_lock(&pWcndManger->clients_lock);

	/* info socket clients that WCN with str info */
	for(i = 0; i < WCND_MAX_CLIENT_NUM; i++)
	{
		int fd = pWcndManger->clients[i].sockfd;
		int type = pWcndManger->clients[i].type;
		WCND_LOGD("clients[%d].sockfd = %d, type = %d\n",i, fd, type);

		if(fd >= 0 && (fd == client_fd) && ((type == notify_type)
			|| ((notify_type == WCND_CLIENT_TYPE_CMD) && ((type & WCND_CLIENT_TYPE_CMD_MASK) == notify_type)))
			)
		{
			/* Send the zero-terminated message */

			//Note: message send from wcnd must end with null character
			//use null character to identify a completed message.
			int len = strlen(info_str) + 1; //including null character

			WCND_LOGD("send %s to client_fd:%d", info_str, client_fd);

			int ret = TEMP_FAILURE_RETRY(write(client_fd, info_str, len));
			if(ret < 0)
			{
				WCND_LOGE("write %d bytes to client_fd:%d fail (error:%s)", len, client_fd, strerror(errno));

				close(fd);
				pWcndManger->clients[i].sockfd = -1;
				pWcndManger->clients[i].type = WCND_CLIENT_TYPE_NOTIFY;
			}
			else if(notify_type == WCND_CLIENT_TYPE_CMD || notify_type == WCND_CLIENT_TYPE_CMD_PENDING
				|| (notify_type & WCND_CLIENT_TYPE_CMD_MASK) == WCND_CLIENT_TYPE_CMD)
			{
				pWcndManger->clients[i].type = WCND_CLIENT_TYPE_SLEEP;
			}

		}
	}

	pthread_mutex_unlock(&pWcndManger->clients_lock);

	return 0;
}


/**
* Config marlin2 one time after marlin2 is power on
* return 0 for success, -1 for fail.
*/
static int on_marlin2_poweron(WcndManager *pWcndManger)
{
	if(!pWcndManger) return -1;

	if(pWcndManger->is_eng_mode_only) return 0;

	//if not use our own wcn, just return
	if(!pWcndManger->is_wcn_modem_enabled) return 0;

	if(pWcndManger->marlin_type != MARLIN_CHIP_TYPE_2)
		return 0;

	// get marlin2 version
	if (!pWcndManger->store_cp2_versin_done)
	{
		wcnd_process_atcmd(-1, WCND_ATCMD_CP2_GET_VERSION, pWcndManger);
		pWcndManger->store_cp2_versin_done = 1;
	}

	// Reset the cp2 log, since after power on cp2, it will reset to default
	if(pWcndManger->is_cp2log_opened)
	{
		wcnd_process_atcmd(-1, WCND_ATCMD_CP2_ENABLE_LOG, pWcndManger);
	}
	else
	{
		wcnd_process_atcmd(-1, WCND_ATCMD_CP2_DISABLE_LOG, pWcndManger);
	}

	return 0;
}

static void wcn_state_handle_default(WcndManager *pWcndManger, WcndMessage *pMessage)
{

	if(!pWcndManger || !pMessage) return;

	switch(pMessage->event)
	{
	case WCND_EVENT_MARLIN2_CLOSED:
		WCND_LOGD("Receive MARLIN2 CLOSED EVENT!");

		if (pWcndManger->marlin_type != MARLIN_CHIP_TYPE_2)
		{
				WCND_LOGE("Receive MARLIN2 CLOSED EVENT for non-MARLIN2 chip!!");
				break;
		}
		if (pWcndManger->state != WCND_STATE_CP2_STOPPED)
		{
				pWcndManger->state = WCND_STATE_CP2_STOPPED;
				pWcndManger->notify_enabled = 0;
		}
		pWcndManger->btwifi_state = 0;
		break;

	case WCND_EVENT_MARLIN2_OPENED:
		WCND_LOGD("Receive MARLIN2 OPENED EVENT!");

		if (pWcndManger->marlin_type != MARLIN_CHIP_TYPE_2)
		{
				WCND_LOGE("Receive MARLIN2 OPENED EVENT for non-MARLIN2 chip!!");
				break;
		}
		if (pWcndManger->state != WCND_STATE_CP2_STARTED)
		{
				pWcndManger->state = WCND_STATE_CP2_STARTED;
				pWcndManger->notify_enabled = 1;
		}
		pWcndManger->btwifi_state |= WCND_BTWIFI_STATE_MARLIN2_ON;
		on_marlin2_poweron(pWcndManger);
		break;

	default:
		break;
	}

}


static void wcn_state_cp2_stopped(WcndManager *pWcndManger, WcndMessage *pMessage)
{
	if(!pWcndManger || !pMessage) return;

	if(pWcndManger->btwifi_state)
		WCND_LOGE("WARNING: btwifi_state: 0x%x in state: WCND_STATE_CP2_STOPPED", pWcndManger->btwifi_state);

	pWcndManger->notify_enabled = 0;

	switch(pMessage->event)
	{
	case WCND_EVENT_BT_OPEN:
	case WCND_EVENT_WIFI_OPEN:
	case WCND_EVENT_FM_OPEN:

		if(pMessage->event == WCND_EVENT_BT_OPEN)
			pWcndManger->btwifi_state |= WCND_BTWIFI_STATE_BT_ON;
		else if(pMessage->event == WCND_EVENT_WIFI_OPEN)
			pWcndManger->btwifi_state |= WCND_BTWIFI_STATE_WIFI_ON;
		else if(pMessage->event == WCND_EVENT_FM_OPEN)
			pWcndManger->btwifi_state |= WCND_BTWIFI_STATE_FM_ON;

		//transact to next state
		pWcndManger->state = WCND_STATE_CP2_STARTING;

		WCND_LOGD("Enter WCND_STATE_CP2_STARTING from WCND_STATE_CP2_STOPPED");
		
		wcnd_send_selfcmd(pWcndManger, "wcn "WCND_SELF_CMD_START_CP2);

		break;

	case WCND_EVENT_BT_CLOSE:
	case WCND_EVENT_WIFI_CLOSE:
	case WCND_EVENT_CP2POWEROFF_REQ:
	case WCND_EVENT_FM_CLOSE:
		wcnd_send_notify_to_client(pWcndManger, WCND_CMD_RESPONSE_STRING" OK", WCND_CLIENT_TYPE_CMD);
		break;

	case WCND_EVENT_PENGING_EVENT:

		if(pWcndManger->pending_events & WCND_EVENT_BT_OPEN)
			pWcndManger->btwifi_state |= WCND_BTWIFI_STATE_BT_ON;

		if(pWcndManger->pending_events & WCND_EVENT_WIFI_OPEN)
			pWcndManger->btwifi_state |= WCND_BTWIFI_STATE_WIFI_ON;

		if(pWcndManger->pending_events & WCND_EVENT_FM_OPEN)
			pWcndManger->btwifi_state |= WCND_BTWIFI_STATE_FM_ON;

		if(pWcndManger->btwifi_state || (pWcndManger->pending_events & WCND_EVENT_CP2POWERON_REQ))
		{
			//transact to next state
			pWcndManger->state = WCND_STATE_CP2_STARTING;

			WCND_LOGD("Enter WCND_STATE_CP2_STARTING from WCND_STATE_CP2_STOPPED for PENDING CP2 POWER ON EVENT");
			wcnd_send_selfcmd(pWcndManger, "wcn "WCND_SELF_CMD_START_CP2);
		}
		break;

	case WCND_EVENT_CP2POWERON_REQ:
		//transact to next state
		pWcndManger->state = WCND_STATE_CP2_STARTING;

		WCND_LOGD("Enter WCND_STATE_CP2_STARTING from WCND_STATE_CP2_STOPPED for WCND_EVENT_CP2POWERON_REQ");

		wcnd_send_selfcmd(pWcndManger, "wcn "WCND_SELF_CMD_START_CP2);
		break;


	//this case is happened only at startup
	case WCND_EVENT_CP2_OK:
		//transact to next state
		pWcndManger->state = WCND_STATE_CP2_STARTED;

		//CP2 is OK, notify is enabled
		pWcndManger->notify_enabled = 1;
		WCND_LOGD("Warning: Enter WCND_STATE_CP2_STARTED from WCND_STATE_CP2_STOPPED");
		break;

	default:
		wcn_state_handle_default(pWcndManger, pMessage);
		break;
	}

}


static void wcn_state_cp2_starting(WcndManager *pWcndManger, WcndMessage *pMessage)
{
	if(!pWcndManger || !pMessage) return;

	pWcndManger->notify_enabled = 0;

	if(!pWcndManger->btwifi_state)
		WCND_LOGE("WARNING: btwifi_state: 0x%x in state: WCND_STATE_CP2_STARTING", pWcndManger->btwifi_state);

	switch(pMessage->event)
	{
	case WCND_EVENT_BT_CLOSE:
	case WCND_EVENT_WIFI_CLOSE:
	case WCND_EVENT_FM_CLOSE:

		WCND_LOGE("WARNING: receive BT/WIFI CLOSE EVENT during CP2 Starting!");
		wcnd_send_notify_to_client(pWcndManger, WCND_CMD_RESPONSE_STRING" OK", WCND_CLIENT_TYPE_CMD);

		break;

	case WCND_EVENT_BT_OPEN:
	case WCND_EVENT_WIFI_OPEN:
	case WCND_EVENT_FM_OPEN:
		if(pMessage->event == WCND_EVENT_BT_OPEN)
			pWcndManger->btwifi_state |= WCND_BTWIFI_STATE_BT_ON;
		else if(pMessage->event == WCND_EVENT_WIFI_OPEN)
			pWcndManger->btwifi_state |= WCND_BTWIFI_STATE_WIFI_ON;
		else if(pMessage->event == WCND_EVENT_FM_OPEN)
			pWcndManger->btwifi_state |= WCND_BTWIFI_STATE_FM_ON;
		break;

	case WCND_EVENT_CP2_OK:
		//transact to next state
		pWcndManger->state = WCND_STATE_CP2_STARTED;

		wcnd_send_notify_to_client(pWcndManger, WCND_CMD_RESPONSE_STRING" OK", WCND_CLIENT_TYPE_CMD);
		wcnd_send_notify_to_client(pWcndManger, WCND_CMD_RESPONSE_STRING" OK", WCND_CLIENT_TYPE_CMD_PENDING);

		//CP2 is OK, notify is enabled
		pWcndManger->notify_enabled = 1;
		WCND_LOGD("Enter WCND_STATE_CP2_STARTED from WCND_STATE_CP2_STARTING");
		break;

	case WCND_EVENT_CP2_DOWN:
		WCND_LOGE("ERROR:CP2 OPEN Failed, btwifi_state: 0x%x\n", pWcndManger->btwifi_state);

		//transact to next state
		pWcndManger->state = WCND_STATE_CP2_STOPPED;
		wcnd_send_notify_to_client(pWcndManger, WCND_CMD_RESPONSE_STRING" FAIL", WCND_CLIENT_TYPE_CMD);
		wcnd_send_notify_to_client(pWcndManger, WCND_CMD_RESPONSE_STRING" FAIL", WCND_CLIENT_TYPE_CMD_PENDING);

		WCND_LOGD("Enter WCND_STATE_CP2_STOPPED from WCND_STATE_CP2_STARTING");

		break;

	case WCND_EVENT_CP2POWEROFF_REQ:
		WCND_LOGD("Warning: Receive WCND_EVENT_CP2POWEROFF_REQ at WCND_STATE_CP2_STARTING, btwifi_state: 0x%x", pWcndManger->btwifi_state);

		wcnd_send_notify_to_client(pWcndManger, WCND_CMD_RESPONSE_STRING" OK", WCND_CLIENT_TYPE_CMD);

		break;

	default:
		wcn_state_handle_default(pWcndManger, pMessage);
		break;
	}
}

static void wcn_state_cp2_started(WcndManager *pWcndManger, WcndMessage *pMessage)
{
	if(!pWcndManger || !pMessage) return;

	pWcndManger->notify_enabled = 1;

	if(!pWcndManger->btwifi_state)
		WCND_LOGE("WARNING: btwifi_state: 0x%x in state: WCND_STATE_CP2_STARTED", pWcndManger->btwifi_state);

	switch(pMessage->event)
	{
	case WCND_EVENT_BT_CLOSE:
	case WCND_EVENT_WIFI_CLOSE:
	case WCND_EVENT_CP2POWEROFF_REQ:
	case WCND_EVENT_FM_CLOSE:

		if(pWcndManger->saved_btwifi_state)
		{
			if(pMessage->event == WCND_EVENT_BT_CLOSE)
				pWcndManger->saved_btwifi_state &= (~WCND_BTWIFI_STATE_BT_ON);
			else if(pMessage->event == WCND_EVENT_WIFI_CLOSE)
				pWcndManger->saved_btwifi_state &= (~WCND_BTWIFI_STATE_WIFI_ON);
			else if(pMessage->event == WCND_EVENT_FM_CLOSE)
				pWcndManger->saved_btwifi_state &= (~WCND_BTWIFI_STATE_FM_ON);

			wcnd_send_notify_to_client(pWcndManger, WCND_CMD_RESPONSE_STRING" OK", WCND_CLIENT_TYPE_CMD);

		}
		else
		{
			if(pMessage->event == WCND_EVENT_BT_CLOSE)
				pWcndManger->btwifi_state &= (~WCND_BTWIFI_STATE_BT_ON);
			else if(pMessage->event == WCND_EVENT_WIFI_CLOSE)
				pWcndManger->btwifi_state &= (~WCND_BTWIFI_STATE_WIFI_ON);
			else if(pMessage->event == WCND_EVENT_FM_CLOSE)
				pWcndManger->btwifi_state &= (~WCND_BTWIFI_STATE_FM_ON);

			if(!pWcndManger->btwifi_state)
			{
				WCND_LOGD("BT/WIFI are all closed, start shutdown CP2");

				//transact to next state
				pWcndManger->state = WCND_STATE_CP2_STOPPING;

				WCND_LOGD("Enter WCND_STATE_CP2_STOPPING from WCND_STATE_CP2_STARTED");

				wcnd_send_selfcmd(pWcndManger, "wcn "WCND_SELF_CMD_STOP_CP2);

			}
			else
			{
				WCND_LOGD("one of BT/WIFI/FM is still opened(0x%x), do not shutdow CP2", pWcndManger->btwifi_state);
				wcnd_send_notify_to_client(pWcndManger, WCND_CMD_RESPONSE_STRING" OK", WCND_CLIENT_TYPE_CMD);
			}
		}

		break;

	case WCND_EVENT_BT_OPEN:
	case WCND_EVENT_WIFI_OPEN:
	case WCND_EVENT_CP2POWERON_REQ:
	case WCND_EVENT_FM_OPEN:
		if(pMessage->event == WCND_EVENT_BT_OPEN)
			pWcndManger->btwifi_state |= WCND_BTWIFI_STATE_BT_ON;
		else if(pMessage->event == WCND_EVENT_WIFI_OPEN)
			pWcndManger->btwifi_state |= WCND_BTWIFI_STATE_WIFI_ON;
		else if(pMessage->event == WCND_EVENT_FM_OPEN)
			pWcndManger->btwifi_state |= WCND_BTWIFI_STATE_FM_ON;

		wcnd_send_notify_to_client(pWcndManger, WCND_CMD_RESPONSE_STRING" OK", WCND_CLIENT_TYPE_CMD);
		break;

	case WCND_EVENT_CP2_ASSERT:
		WCND_LOGD("CP2 Assert Happens, current bi/wifi state: 0x%x", pWcndManger->btwifi_state);

		//save current wifi/bt state
		pWcndManger->saved_btwifi_state = pWcndManger->btwifi_state;

		//transact to next state
		pWcndManger->state = WCND_STATE_CP2_ASSERT;

      		WCND_LOGD("Enter WCND_STATE_CP2_ASSERT from WCND_STATE_CP2_STARTED");

		//to do reset
		wcnd_send_selfcmd(pWcndManger, "wcn reset");

		break;


	case WCND_EVENT_CP2_DOWN:
		//transact to next state
		pWcndManger->state = WCND_STATE_CP2_STOPPED;

		WCND_LOGD("Warning: Enter WCND_STATE_CP2_STOPPED from WCND_STATE_CP2_STOPPING");

		break;

	default:
		wcn_state_handle_default(pWcndManger, pMessage);
		break;
	}
}


static void wcn_state_cp2_assert(WcndManager *pWcndManger, WcndMessage *pMessage)
{
	int i = 0;

	if(!pWcndManger || !pMessage) return;

	pWcndManger->notify_enabled = 1;

	if(!pWcndManger->btwifi_state)
		WCND_LOGE("WARNING: btwifi_state: 0x%x in state: WCND_STATE_CP2_ASSERT", pWcndManger->btwifi_state);

	switch(pMessage->event)
	{
	case WCND_EVENT_BT_CLOSE:
	case WCND_EVENT_WIFI_CLOSE:
	case WCND_EVENT_FM_CLOSE:
		WCND_LOGD("WARNING: saved_btwifi_state: 0x%x in state: WCND_STATE_CP2_ASSERT", pWcndManger->saved_btwifi_state);
		if(pWcndManger->saved_btwifi_state)
		{
			if(pMessage->event == WCND_EVENT_BT_CLOSE)
				pWcndManger->saved_btwifi_state &= (~WCND_BTWIFI_STATE_BT_ON);
			else if(pMessage->event == WCND_EVENT_WIFI_CLOSE)
				pWcndManger->saved_btwifi_state &= (~WCND_BTWIFI_STATE_WIFI_ON);
			else if(pMessage->event == WCND_EVENT_FM_CLOSE)
				pWcndManger->saved_btwifi_state &= (~WCND_BTWIFI_STATE_FM_ON);
		}

		pthread_mutex_lock(&pWcndManger->clients_lock);

		//save the pending message
		for (i = 0; i < WCND_MAX_CLIENT_NUM; i++)
		{
			 if(pWcndManger->clients[i].sockfd == pMessage->replyto_fd)
			{
				pWcndManger->clients[i].type = WCND_CLIENT_TYPE_CMD_SUBTYPE_CLOSE;
				break;
			}
		}
		pthread_mutex_unlock(&pWcndManger->clients_lock);

		wcnd_send_notify_to_client(pWcndManger, WCND_CMD_RESPONSE_STRING" OK", WCND_CLIENT_TYPE_CMD_SUBTYPE_CLOSE);
		break;

	case WCND_EVENT_BT_OPEN:
	case WCND_EVENT_WIFI_OPEN:
	case WCND_EVENT_FM_OPEN:
		WCND_LOGD("WARNING: btwifi_state: 0x%x in state: WCND_STATE_CP2_ASSERT", pWcndManger->btwifi_state);
	
		if(pMessage->event == WCND_EVENT_BT_OPEN)
			pWcndManger->btwifi_state |= WCND_BTWIFI_STATE_BT_ON;
		else if(pMessage->event == WCND_EVENT_WIFI_OPEN)
			pWcndManger->btwifi_state |= WCND_BTWIFI_STATE_WIFI_ON;
		else if(pMessage->event == WCND_EVENT_FM_OPEN)
			pWcndManger->btwifi_state |= WCND_BTWIFI_STATE_FM_ON;

		pthread_mutex_lock(&pWcndManger->clients_lock);

		//save the pending message
		for (i = 0; i < WCND_MAX_CLIENT_NUM; i++)
		{
			 if(pWcndManger->clients[i].sockfd == pMessage->replyto_fd)
			{
				pWcndManger->clients[i].type = WCND_CLIENT_TYPE_CMD_SUBTYPE_OPEN;
				break;
			}
		}
		pthread_mutex_unlock(&pWcndManger->clients_lock);


		break;

	case WCND_EVENT_CP2_OK:

		//transact to next state
		pWcndManger->state = WCND_STATE_CP2_STARTED;
		wcnd_send_notify_to_client(pWcndManger, WCND_CMD_RESPONSE_STRING" OK", WCND_CLIENT_TYPE_CMD);

      		WCND_LOGD("Enter WCND_STATE_CP2_STARTED from WCND_STATE_CP2_ASSERT");

		break;

	case WCND_EVENT_CP2_DOWN:
		WCND_LOGE("ERROR:CP2 ASSERT RESET Failed, btwifi_state: 0x%x", pWcndManger->btwifi_state);

		//transact to next state
		pWcndManger->state = WCND_STATE_CP2_STOPPED;
		wcnd_send_notify_to_client(pWcndManger, WCND_CMD_RESPONSE_STRING" FAIL", WCND_CLIENT_TYPE_CMD);

      		WCND_LOGD("Enter WCND_STATE_CP2_STOPPED from WCND_STATE_CP2_ASSERT");

		break;

	case WCND_EVENT_CP2POWERON_REQ:
	case WCND_EVENT_CP2POWEROFF_REQ:
		WCND_LOGE("WARNING: Receive WCND POWER ON/OFF EVENT at WCND_STATE_CP2_ASSERT, btwifi_state: 0x%x", pWcndManger->btwifi_state);
		if(!pWcndManger->doing_reset && (pMessage->replyto_fd > 0))
		{
			WCND_LOGE("WARNING: Receive WCND POWER ON/OFF EVENT at WCND_STATE_CP2_ASSERT, btwifi_state: 0x%x, and Does not doing reset", pWcndManger->btwifi_state);

			wcnd_notify_client(pWcndManger, WCND_CMD_RESPONSE_STRING" FAIL", pMessage->replyto_fd, WCND_CLIENT_TYPE_CMD);
		}
		break;

	default:
		wcn_state_handle_default(pWcndManger, pMessage);
		break;
	}
}

static void wcn_state_cp2_stopping(WcndManager *pWcndManger, WcndMessage *pMessage)
{
	int i = 0;

	if(!pWcndManger || !pMessage) return;

	pWcndManger->notify_enabled = 0;

	if(pWcndManger->btwifi_state)
		WCND_LOGE("WARNING: btwifi_state: 0x%x in state: WCND_STATE_CP2_STOPPING", pWcndManger->btwifi_state);

	switch(pMessage->event)
	{
	case WCND_EVENT_BT_CLOSE:
	case WCND_EVENT_WIFI_CLOSE:
	case WCND_EVENT_CP2POWEROFF_REQ:
	case WCND_EVENT_FM_CLOSE:
		WCND_LOGD("CP2 is stopping!!");
		break;

	case WCND_EVENT_BT_OPEN:
	case WCND_EVENT_WIFI_OPEN:
	case WCND_EVENT_CP2POWERON_REQ:
	case WCND_EVENT_FM_OPEN:
		WCND_LOGD("Pending BT/WIFI/FM Open event during CP2 is stopping!!");

		pthread_mutex_lock(&pWcndManger->clients_lock);

		//save the pending message
		for (i = 0; i < WCND_MAX_CLIENT_NUM; i++)
		{
			 if(pWcndManger->clients[i].sockfd == pMessage->replyto_fd)
			{
				pWcndManger->clients[i].type = WCND_CLIENT_TYPE_CMD_PENDING;
				break;
			}
		}
		pthread_mutex_unlock(&pWcndManger->clients_lock);

		pWcndManger->pending_events |= pMessage->event ;

		break;

	case WCND_EVENT_CP2_DOWN:
		//transact to next state
		pWcndManger->state = WCND_STATE_CP2_STOPPED;

		wcnd_send_notify_to_client(pWcndManger, WCND_CMD_RESPONSE_STRING" OK", WCND_CLIENT_TYPE_CMD);
		//has pending event for stopped state
		if(pWcndManger->pending_events)
			wcnd_send_selfcmd(pWcndManger, "wcn "WCND_SELF_CMD_PENDINGEVENT);

      		WCND_LOGD("Enter WCND_STATE_CP2_STOPPED from WCND_STATE_CP2_STOPPING");

		break;

	default:
		wcn_state_handle_default(pWcndManger, pMessage);
		break;
	}
}
/**
* This Function will be called only in client listen thread, so there is not need to do some sync
*/
//state machine handle message
int wcnd_sm_step(WcndManager *pWcndManger, WcndMessage *pMessage)
{

	if(!pWcndManger || !pMessage) return -1;

#ifdef WCND_STATE_MACHINE_ENABLE

	//if not use our own wcn, just return
	if(!pWcndManger->is_wcn_modem_enabled)
	{
		return 0;
	}

	WCND_LOGD("Current State: %d, receive event: 0x%x!!", pWcndManger->state, pMessage->event);

	switch(pWcndManger->state)
	{
		case WCND_STATE_CP2_STOPPED:
			wcn_state_cp2_stopped(pWcndManger, pMessage);
			break;

		case WCND_STATE_CP2_STARTING:
			wcn_state_cp2_starting(pWcndManger, pMessage);
			break;

		case WCND_STATE_CP2_STARTED:
			wcn_state_cp2_started(pWcndManger, pMessage);
			break;
		
		case WCND_STATE_CP2_ASSERT:
			wcn_state_cp2_assert(pWcndManger, pMessage);
			break;

		case WCND_STATE_CP2_STOPPING:
			wcn_state_cp2_stopping(pWcndManger, pMessage);
			break;

		default:
			
			WCND_LOGE("ERROR:Unknown CP2 STATE (%d) btwifi_state: 0x%x", pWcndManger->state, pWcndManger->btwifi_state);
			break;
			

	}

#endif

	return 0;
}

int wcnd_sm_init(WcndManager *pWcndManger)
{
	if(!pWcndManger) return -1;

#ifdef WCND_STATE_MACHINE_ENABLE

	//if not use our own wcn, just return
	if(!pWcndManger->is_wcn_modem_enabled)
	{
		pWcndManger->state = WCND_STATE_CP2_STARTED;
		pWcndManger->notify_enabled = 1;

		pWcndManger->btwifi_state |= (WCND_BTWIFI_STATE_BT_ON |WCND_BTWIFI_STATE_WIFI_ON | WCND_BTWIFI_STATE_FM_ON);
	}
	else
	{

		pWcndManger->state = WCND_STATE_CP2_STOPPED;
		pWcndManger->notify_enabled = 0;
		//pWcndManger->btwifi_state |= (WCND_BTWIFI_STATE_BT_ON |WCND_BTWIFI_STATE_WIFI_ON);
	}

#else


	pWcndManger->state = WCND_STATE_CP2_STARTED;
	pWcndManger->notify_enabled = 1;

	pWcndManger->btwifi_state |= (WCND_BTWIFI_STATE_BT_ON |WCND_BTWIFI_STATE_WIFI_ON | WCND_BTWIFI_STATE_FM_ON);



#endif

	return 0;
}


