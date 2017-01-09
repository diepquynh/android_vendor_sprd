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
#include "sprd_dm.h"
#include "spdm_task.h"

#define LOG_TAG "SPRD_DM"   
#undef LOG   
#include <utils/Log.h>
#define dmdebug 1

bool spdm_start(int type, char * data, int datalen)
{

	SPRD_TASK_REQ_T *req_ptr;
	
	spdm_print("spdm_start enter.. \n");
	spdm_print("spdm_start type:%d, len:%d \n", type, datalen);
	if (spdm_task_check_running())
		{
		spdm_print("spdm_start, thread already running \n");
		spdm_closeDm(1);
		return FALSE;
		}
	spdm_task_init();
	while(1)
	{
	usleep(50000);
	if (spdm_task_check_running()) break;
	}

	req_ptr = malloc(sizeof(SPRD_TASK_REQ_T));
	if (req_ptr == NULL)
		{
		spdm_print("spdm_start, malloc 1 failed \n");
		return FALSE;
		}
	memset(req_ptr,0,sizeof(SPRD_TASK_REQ_T));
	req_ptr->buf = malloc( datalen+1);
	if (req_ptr->buf == NULL)
		{
		spdm_print("spdm_star, malloc 2 failed \n");
		free(req_ptr);
		return FALSE;
		}
	req_ptr->type = SPDM_TASK_START;
	memset(req_ptr->buf,0, datalen+1);
	memcpy(req_ptr->buf, data, datalen);
	req_ptr->buflen = datalen;
	
	if (spdm_task_sendsignal(req_ptr) == FALSE)
		{
		spdm_print("spdm_star, Unbelievable error!! thread is not running \n");
		free(req_ptr->buf);
		free(req_ptr);
		return FALSE;
		}
	
	spdm_print("spdm_start Ok leaving... \n");
	
	return TRUE;
}

bool spdm_receiveData(char * data, int datalen)
{
	SPRD_TASK_REQ_T *req_ptr;
	
	spdm_print("spdm_receiveData enter.. \n");
	//spdm_print("spdm_receiveData , len:%d, string:%s \n", datalen, data);
	
	req_ptr = malloc(sizeof(SPRD_TASK_REQ_T));
	if (req_ptr == NULL)
		{
		spdm_print("spdm_receiveData, malloc 1 failed \n");
		return FALSE;
		}

	memset(req_ptr,0,sizeof(SPRD_TASK_REQ_T));
	
	req_ptr->buf = malloc( datalen+1);
	if (req_ptr->buf == NULL)
		{
		spdm_print("spdm_receiveData, malloc 2 failed \n");
		free(req_ptr);
		return FALSE;
		}
	memset(req_ptr->buf,0, datalen+1);
	req_ptr->type = SPDM_TASK_TRANSPORT;
	memcpy(req_ptr->buf, data, datalen);
	req_ptr->buflen = datalen;
	
	if (spdm_task_sendsignal(req_ptr) == FALSE)
		{
		spdm_print("spdm_receiveData, Unbelievable error!! thread is not running \n");
		if (req_ptr->buf != NULL)
		free(req_ptr->buf);
		if (req_ptr != NULL)
		free(req_ptr);
		return FALSE;
		}
	
	spdm_print("spdm_receiveData leaving ok \n");
	return TRUE;
}

void spdm_DialogconfirmCb(int retcode)
{
	SPRD_TASK_REQ_T *req_ptr;
	spdm_print("spdm_DialogconfirmCb confirm result:%d \n", retcode);

	req_ptr = malloc(sizeof(SPRD_TASK_REQ_T));
	if (req_ptr == NULL)
		{
		spdm_print("spdm_DialogconfirmCb, malloc 1 failed \n");
		return ;
		}
	memset(req_ptr,0,sizeof(SPRD_TASK_REQ_T));
	
	req_ptr->type = SPDM_TASK_NOTIFYRST;
	req_ptr->result_reason = retcode;
	if (spdm_task_sendsignal(req_ptr) == FALSE)
		{
		spdm_print("spdm_DialogconfirmCb, Unbelievable error!! thread is not running \n");
		if (req_ptr->buf != NULL)
		free(req_ptr->buf);
		if (req_ptr != NULL)
		free(req_ptr);
		return ;
		}	
	
	spdm_print("spdm_DialogconfirmCb leaving... \n");
}

void spdm_stopDm(int reason)
{
	SPRD_TASK_REQ_T *req_ptr;

	spdm_print("spdm_stopDm  reason:%d \n", reason);
	
	req_ptr = malloc(sizeof(SPRD_TASK_REQ_T));
	if (req_ptr == NULL)
		{
		spdm_print("spdm_stopDm, malloc 1 failed \n");
		return ;
		}
	memset(req_ptr,0,sizeof(SPRD_TASK_REQ_T));
	
	req_ptr->type = SPDM_TASK_EXIT;
	req_ptr->result_reason = reason;
	if (spdm_task_sendsignal(req_ptr) == FALSE)
		{
		spdm_print("spdm_stopDm, Unbelievable error!! thread is not running \n");
		if (req_ptr->buf != NULL)
		free(req_ptr->buf);
		if (req_ptr != NULL)
		free(req_ptr);
		return ;
		}	
	spdm_print("spdm_stopDm leaving... \n");
	
}


void spdm_closeDm(int reason)
{
	SPRD_TASK_REQ_T *req_ptr;

	spdm_print("spdm_stopDm  reason:%d \n", reason);
	
	req_ptr = malloc(sizeof(SPRD_TASK_REQ_T));
	if (req_ptr == NULL)
		{
		spdm_print("spdm_stopDm, malloc 1 failed \n");
		return ;
		}
	memset(req_ptr,0,sizeof(SPRD_TASK_REQ_T));
	
	req_ptr->type = SPDM_TASK_CLOSE;
	req_ptr->result_reason = reason;
	if (spdm_task_sendsignal(req_ptr) == FALSE)
		{
		spdm_print("spdm_stopDm, Unbelievable error!! thread is not running \n");
		if (req_ptr->buf != NULL)
		free(req_ptr->buf);
		if (req_ptr != NULL)
		free(req_ptr);
		return ;
		}	
	spdm_print("spdm_stopDm leaving... \n");
	
}

bool spdm_isDmRunning()
{
	bool running = FALSE;
	spdm_print("spdm_isDmRunning enter.. \n");
	 running = spdm_task_check_running();
	spdm_print("spdm_isDmRunning [%d] leaving.. \n", running);
	return running;
}


int spdm_print(const char *fmt,...)
{
	int ret = 0;
#if dmdebug 
	char buf[8192];	
	char tchar;
        char *ptr;
	int verylong = 0;
	va_list ap;
	
	va_start(ap, fmt);
	ret = vsprintf(buf,fmt,ap);
	va_end(ap);
	
	ptr = &buf[0];
	while(1)
	{
	if (strlen(ptr) > 1024)
		{
		tchar = *(ptr+1024);
		*(ptr+1024)= 0;
		ALOGD("%s",ptr);
		*(ptr+1024) = tchar;
		ptr = ptr+ 1024;
		verylong = 1;
		}
		else
		{
		ALOGD("%s", ptr);
		break;
		}
	}
	if (verylong)
	ALOGD("%s", buf);
#else
#endif
	return ret;
}






