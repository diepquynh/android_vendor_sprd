/*********************************************************************
File: sprd_dm.c
Author: hong yu
Creation Date; 2012-1-3
descritpion: DMCC DM low level API for app and JNI 
*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include "sprd_dm.h"
#include "spdm_task.h"
#include "sprd_dm_parsexml.h"
#include "dm_jni.h"

#define XML_MAX_LEN  MAX_XML_BUF_SIZE

static bool dm_task_running = FALSE;
static SPRD_TASK_REQ_T *gbl_req_ptr = NULL;
static SPRD_TASK_REQ_T *gbl_notify_ptr = NULL;
static pthread_t gTid;
static pthread_mutex_t 	mMutex;
static pthread_cond_t 	mCond;
static SPRD_DM_TASK_STATE mState = SPRD_DM_STATE_IDLE;
static void * dmtask(void *arg);

BOOLEAN spdm_task_check_running(void)
{
	return dm_task_running;
}

SPRD_DM_TASK_STATE spdm_task_get_state(void)
{
	return mState;
}

static void spdm_add_queue(SPRD_TASK_REQ_T *req)
{
	SPRD_TASK_REQ_T * req_ptr = gbl_req_ptr;
	if (gbl_req_ptr == NULL) 
		{
		gbl_req_ptr = req;
		spdm_print("spdm_add_queue add to head.\n");
		return;
		}
	spdm_print("spdm_add_queue add to tail.\n");
	while(1)
	{
		if (req_ptr->next != NULL) req_ptr = req_ptr->next;
		else
		{
			req_ptr->next = req;
			req->next = NULL;
			break;
		}	
	}
}

static void spdm_removeall_queue()
{
	SPRD_TASK_REQ_T * req_ptr = gbl_req_ptr;

	spdm_print("spdm_removeall_queue entering..\n");
	while(gbl_req_ptr != NULL)
	{
	 req_ptr = gbl_req_ptr;
	 gbl_req_ptr = req_ptr->next;
	 	if (req_ptr->buf != NULL)
			free(req_ptr->buf);
		free(req_ptr);
	}

	spdm_print("spdm_removeall_queue leaving..\n");

}

static void spdm_remove_queue()
{
	SPRD_TASK_REQ_T * req_ptr = gbl_req_ptr;
	if (req_ptr != NULL)
	{
	gbl_req_ptr = req_ptr->next;

		if (req_ptr->buf != NULL)
			free(req_ptr->buf);
		
		free(req_ptr);
	if (gbl_req_ptr != NULL)
		spdm_print("spdm_remove_queue is not empty, type:%d \n", gbl_req_ptr->type);
	else
		spdm_print("spdm_remove_queue is  empty! \n");
	}
	
}

void spdm_task_need_confirm_msg(char *msg, int msglen, int timeout)
{

	SPRD_TASK_REQ_T * req_ptr;
	
	spdm_print("spdm_task_need_confirm_msg entering... \n");
	
	req_ptr = malloc(sizeof(SPRD_TASK_REQ_T));
	if (req_ptr == NULL)
		{
		spdm_print("spdm_task_need_confirm_msg malloc1 Failed \n");
		return;
		}
	memset(req_ptr, 0, sizeof(SPRD_TASK_REQ_T));
	
	req_ptr->buf = malloc(msglen+1);
	if (req_ptr->buf == NULL)
		{
		free(req_ptr);
		spdm_print("spdm_task_need_confirm_msg malloc2 Failed \n");
		return;
		}
	
	req_ptr->type = SPDM_TASK_NOTIFY;
	memcpy(req_ptr->buf,msg,msglen);
	req_ptr->buf[msglen] = 0;
	req_ptr->buflen = msglen;
	req_ptr->timeout = timeout;
	
	if (gbl_notify_ptr != NULL)
		{
		if (gbl_notify_ptr->buf != NULL)
			free(gbl_notify_ptr->buf);
		free(gbl_notify_ptr);
		gbl_notify_ptr = NULL;
		}
	
	gbl_notify_ptr = req_ptr;
	spdm_print("spdm_task_need_confirm_msg leaving... \n");
	
	return;
		
}

bool spdm_task_init()
{

       spdm_print("spdm_task_init enter... \n");
	   
	if (dm_task_running != FALSE)
	{
		spdm_print("spdm_task_init failed.  Already RUNNING!!!");
		return FALSE;
	}

	pthread_mutex_init(&mMutex,NULL);
	pthread_cond_init(&mCond, NULL);	
	pthread_create(&gTid, NULL, dmtask, &gbl_req_ptr);

	
	return TRUE;
}


bool spdm_task_sendsignal(SPRD_TASK_REQ_T *req_ptr)
{

	spdm_print("spdm_task_init enter. type:%d result:%d buflen:%d.. \n", 
		req_ptr->type, req_ptr->result_reason, req_ptr->buflen);

	if (!dm_task_running)
	{
	spdm_print("spdm_task_sendsignal FAILED ..Task is not running! \n");
	return FALSE;	
	}
	

	pthread_mutex_lock(&mMutex);
	spdm_add_queue(req_ptr);
	
	pthread_cond_signal(&mCond);

	pthread_mutex_unlock(&mMutex);
	return TRUE;
}


static void * dmtask(void *arg)
{
	int ret;
	int finished = FALSE;
	int sessionid;
	unsigned int msglen;
	SPRD_DM_PARSE_RESULT lret;
	char *xmlptr = NULL;
	char *mempool = NULL;

       spdm_print("dmtask running... \n");

	 mState = SPRD_DM_STATE_IDLE;
	dm_task_running = TRUE;
	xmlptr = malloc(XML_MAX_LEN);
#if LOCAL_MEMPOOL
#else
        mempool = malloc(MAX_LOCAL_POOL_LEN);
#endif		
	while(!finished)
	{
	pthread_mutex_lock(&mMutex);
	spdm_print("spdmtask waiting signal coming \n");
	
	ret = pthread_cond_wait(&mCond, &mMutex);
	if (gbl_req_ptr == NULL) continue;
	switch(gbl_req_ptr->type)
		{
		
		case SPDM_TASK_START:
			
			spdm_print("spdmtask SPDM_TASK_START received \n");
			if (mState != SPRD_DM_STATE_IDLE)
				{
				spdm_print("spdmtask not in IDLE mode, Ignore this message! \n");
				}
			else
				{
				if (gbl_req_ptr->buflen < 28 ) 
					{
					spdm_print("dmtask wappush is too short! \n");
					}
				else
					{
					sessionid = (((int) gbl_req_ptr->buf[21]) << 8) | gbl_req_ptr->buf[22];
					spdm_print("dmtask session id set:%x ! \n", sessionid);
					//MMIDM_setSessionId(sessionid);
					MMIDM_setSessionId(sessionid);
#if LOCAL_MEMPOOL
#else
					MMIDM_setMemPool(mempool, MAX_LOCAL_POOL_LEN);
#endif
					//MMIDM_SendData(char *msg, int msglen);
					memset(xmlptr, 0, XML_MAX_LEN);
					msglen = XML_MAX_LEN;
					lret = MMIDM_GenerateDmData(xmlptr, &msglen);

					if ( (gbl_req_ptr->buf[17]&0x30) == 0x30)
					{
						mState = SPRD_DM_STATE_CONTINUE;
						spdm_openDialogCb(2, "是否接受手机增强售后服务? ", "DM Message", 150);  // wait user response at most 150 secs
					
						break;
					}
					else
					if ( (gbl_req_ptr->buf[17]&0x30) == 0x20)
					{
					      mState = SPRD_DM_STATE_CONTINUE;
						spdm_openDialogCb(1, "手机增强售后服务提醒", "DM Message", 30);
						break;
					}
						
					spdm_print("spdmtask MMIDM_GenerateDmData1 ret %d \n", lret);
					//spdm_print("spdmtask generate xml: %s \n", xmlptr);
					spdm_print("spdmtask generate xml: %s \n", xmlptr);
					if (lret == SPRD_DM_PARSE_RESULT_OK)
						{
						spdm_print("spdmtask MMIDM_GenerateDmData1 ret 0k \n");
						spdm_sendDataCb(xmlptr, msglen, 0, MMIDM_getResUri());
						}
					else if (lret == SPRD_DM_PARSE_RESULT_EXIT)
						{
						
						spdm_print("spdmtask MMIDM_GenerateDmData1 ret exit  \n");
						spdm_exitNotifyCb(SPRD_DM_PARSE_ERROR);
						
						//free the all queue event
						spdm_removeall_queue();

						// set the state
						mState = SPRD_DM_STATE_IDLE;
						
						//threadexit flag is set
						finished = TRUE;
						
						spdm_print("spdmtask MMIDM_GenerateDmData1 endDM and exitDM \n");
				                MMIDM_EndDm();
						
						}
					
					//javaAPI sendData(char *msg, int msglen);
					//Fixme!
					mState = SPRD_DM_STATE_CONTINUE;
					}
				}
			break;
			
	  	case SPDM_TASK_CLOSE:

			spdm_print("spdmtask SPDM_TASK_CLOSE received \n");
			MMIDM_EndDm();
			//free the all queue event
			spdm_removeall_queue();

			// set the state
			mState = SPRD_DM_STATE_IDLE;
			
			//threadexit flag is set
			finished = TRUE;
			break;
			
	  	case SPDM_TASK_EXIT:
			
			spdm_print("spdmtask SPDM_TASK_EXIT received \n");	
			// we only notify the exit event of parse error to UI
			if (gbl_req_ptr->result_reason != SPRD_DM_USER_STOP)
			spdm_exitNotifyCb(gbl_req_ptr->result_reason);

			//free syncml parse and resources at first
			//Fixme!
			//MMIDM_EXIT& free
			MMIDM_EndDm();
			//free the all queue event
			spdm_removeall_queue();

			// set the state
			mState = SPRD_DM_STATE_IDLE;
			
			//threadexit flag is set
			finished = TRUE;
			break;

			
	  	case SPDM_TASK_ATTENTION:
			
			//do nothing now
			spdm_print("spdmtask SPDM_TASK_ATTENTION received \n");
			break;
			
	  	case SPDM_TASK_NOTIFY:
			
			spdm_print("spdmtask SPDM_TASK_NOTIFY received \n");
			mState = SPRD_DM_STATE_WAITUI;
			//call java UI and wait result
			// FixMe
			
			spdm_print("spdmtask call java UI Dialog confirm..... \n");
			spdm_openDialogCb(4, gbl_req_ptr->buf, "DM Message", gbl_req_ptr->timeout);
			
			break;

	  	case SPDM_TASK_NOTIFYRST:
			{
			spdm_print("spdmtask SPDM_TASK_NOTIFYRST received \n");
			//Call the syncml receive notify function API 
			// check the ret for further action
			// either send HTTP and final or send HTTP and keep Continue;
			// FixMe
			
			if (mState != SPRD_DM_STATE_CONTINUE )
			lret = MMIDM_NotifyAlertResult(gbl_req_ptr->result_reason) ;
			else
			{
				if (gbl_req_ptr->result_reason )
				lret = SPRD_DM_PARSE_RESULT_START;
				else
				{
					spdm_exitNotifyCb(SPRD_DM_PARSE_RESULT_EXIT);
					spdm_print("spdmtask not continue for UI choice NO \n ");
						//Fixme!
						//MMIDM_EXIT& free
				                MMIDM_EndDm();
						
						//free the all queue event
						spdm_removeall_queue();

						// set the state
						mState = SPRD_DM_STATE_IDLE;
						
						//threadexit flag is set
						finished = TRUE;					
					break;
				}
			}
			if ( lret == SPRD_DM_PARSE_RESULT_START)
				{
				spdm_print("spdmtask MMIDM_NotifyAlertResult ok begin send to java\n");
					memset(xmlptr, 0, XML_MAX_LEN);
					msglen = XML_MAX_LEN;
					
					if (MMIDM_GenerateDmData(xmlptr, &msglen) == SPRD_DM_PARSE_RESULT_OK)
						{
						//spdm_print("spdmtask generate xml: %s \n", xmlptr);

						spdm_print("spdmtask SPDM_TASK_TRANSPORT MMIDM_GenerateDmData ret 0k \n");
						spdm_sendDataCb(xmlptr, msglen, 0, MMIDM_getResUri());
						}
					else
						{
						spdm_exitNotifyCb(SPRD_DM_PARSE_RESULT_EXIT);

						//free syncml parse and resources at first
						//Fixme!
						//MMIDM_EXIT& free
				                MMIDM_EndDm();
						
						//free the all queue event
						spdm_removeall_queue();

						// set the state
						mState = SPRD_DM_STATE_IDLE;
						
						//threadexit flag is set
						finished = TRUE;
						
						}				
				//senddata to java
				}
			else if (lret == SPRD_DM_PARSE_RESULT_EXIT)
				{
				spdm_print("spdmtask MMIDM_NotifyAlertResult received exit\n");
				//senddata to java
				
				spdm_exitNotifyCb(SPRD_DM_PARSE_RESULT_EXIT);

				//free syncml parse and resources at first
				//Fixme!
				//MMIDM_EXIT& free
		                MMIDM_EndDm();
				
				//free the all queue event
				spdm_removeall_queue();

				// set the state
				mState = SPRD_DM_STATE_IDLE;
				
				//threadexit flag is set
				finished = TRUE;
				
				}
			break;
	  		}
	  	case SPDM_TASK_TRANSPORT:
			{
			spdm_print("spdmtask SPDM_TASK_RECEIVE received \n");
			//call the syncml receive data at first
			//Fixme
			//check if we should to call Java API 
			//call Java confirm / Send Http data 
			//	SPRD_DM_PARSE_RESULT lret;
			spdm_print("dm server input len:%d", strlen(gbl_req_ptr->buf));
			spdm_print(gbl_req_ptr->buf);
			lret = MMIDM_ParseReceiveData(gbl_req_ptr->buf);
			if (lret== SPRD_DM_PARSE_RESULT_START )
				{
				spdm_print("spdmtask MMIDM_ParseReceiveData return start %d \n", lret);

					memset(xmlptr, 0, XML_MAX_LEN);
					msglen = XML_MAX_LEN;
					
					if (MMIDM_GenerateDmData(xmlptr, &msglen) == SPRD_DM_PARSE_RESULT_OK)
						{
						spdm_print("spdmtask generate len:%d xml: %s \n", strlen(xmlptr), xmlptr);

						spdm_print("spdmtask SPDM_TASK_TRANSPORT MMIDM_GenerateDmData ret 0k \n");
						spdm_sendDataCb(xmlptr, msglen, 0, MMIDM_getResUri());
						}
					else
						{
						spdm_exitNotifyCb(SPRD_DM_PARSE_RESULT_EXIT);

						//free syncml parse and resources at first
						//Fixme!
						//MMIDM_EXIT& free
				                MMIDM_EndDm();
						
						//free the all queue event
						spdm_removeall_queue();

						// set the state
						mState = SPRD_DM_STATE_IDLE;
						
						//threadexit flag is set
						finished = TRUE;
						
						}
				
				}

			else if (lret == SPRD_DM_PARSE_RESULT_EXIT)
				{
				spdm_print("spdmtask MMIDM_ParseReceiveData return exit \n");
				spdm_exitNotifyCb(SPRD_DM_PARSE_RESULT_EXIT);

				//free syncml parse and resources at first
				//Fixme!
				//MMIDM_EXIT& free
		                MMIDM_EndDm();
						
						//free the all queue event
				spdm_removeall_queue();

				// set the state
				mState = SPRD_DM_STATE_IDLE;
						
				//threadexit flag is set
				finished = TRUE;
				
				}
			else if (lret == SPRD_DM_PARSE_RESULT_OK)
				{
				spdm_print("spdmtask MMIDM_ParseReceiveData return OK %d \n", lret);
				spdm_print("spdmtask MMIDM_ParseReceiveData waiting User choice:[%s] \n",gbl_notify_ptr->buf);
				mState = SPRD_DM_STATE_WAITUI;
				spdm_openDialogCb(4, gbl_notify_ptr->buf, "DM Message", gbl_notify_ptr->timeout);
				
				}
	  		}
			break;



		}

	spdm_remove_queue();
	pthread_mutex_unlock(&mMutex);
	}

	if (xmlptr != NULL)
		{
		free(xmlptr);
		xmlptr= 0;
		}
	if (mempool !=NULL)
		{
		free(mempool);
		mempool=NULL;
		}
	if (gbl_notify_ptr != NULL)
		{
		if (gbl_notify_ptr->buf != NULL)
			free(gbl_notify_ptr->buf);
		free(gbl_notify_ptr);
		gbl_notify_ptr = NULL;
		}
	
	pthread_mutex_destroy(&mMutex);
	pthread_cond_destroy(&mCond);
       spdm_print("dmtask exit \n");
	dm_task_running = FALSE;
	pthread_exit(0);
	return 0;
}


