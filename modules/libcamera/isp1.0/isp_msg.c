/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "isp_msg"

#include <stdlib.h>
//#include "cmr_common.h"
#include "isp_msg.h"
#include "isp_log.h"
#include <sys/types.h>

#define ISP_MSG_CHECK_MSG_MAGIC(handle)           \
		do {                                                         \
		    if (((struct isp_msg_cxt*)handle)->msg_magic != ISP_MSG_MAGIC_CODE) {    \
				return ISP_MSG_INVALID_HANDLE;                                        \
			}                                                                    \
		} while(0)


int isp_msg_queue_create(unsigned int count, cmr_handle *queue_handle)
{
	struct isp_msg_cxt       *msg_cxt;
	uint32_t 		         handler_id = 0;

	//CMR_LOGI("count 0x%x", count);

	if (0 == count) {
		return -ISP_MSG_PARAM_ERR;
	}
	msg_cxt = (struct isp_msg_cxt*)malloc(sizeof(struct isp_msg_cxt));
	if (NULL == msg_cxt) {
		return -ISP_MSG_NO_MEM;
	}
	bzero(msg_cxt, sizeof(*msg_cxt));
	msg_cxt->msg_head = (struct isp_msg*)malloc((unsigned int)(count * sizeof(struct isp_msg)));
	if (NULL == msg_cxt->msg_head) {
		free(msg_cxt);
		return -ISP_MSG_NO_MEM;
	}
	msg_cxt->msg_magic = ISP_MSG_MAGIC_CODE;
	msg_cxt->msg_count = count;
	msg_cxt->msg_number= 0;
	msg_cxt->msg_read  = msg_cxt->msg_head;
	msg_cxt->msg_write = msg_cxt->msg_head;
	pthread_mutex_init(&msg_cxt->mutex, NULL);
	sem_init(&msg_cxt->msg_sem, 0, 0);
	*queue_handle = (cmr_handle)msg_cxt;
	ISP_LOG("queue_handle %p", *queue_handle);

	return ISP_MSG_SUCCESS;
}

int isp_msg_get(cmr_handle queue_handle, struct isp_msg *message)
{
	struct isp_msg_cxt *msg_cxt = (struct isp_msg_cxt*)queue_handle;
	uint32_t 		   handler_id = 0;

	if (0 == queue_handle || NULL == message) {
		return -ISP_MSG_PARAM_ERR;
	}

	ISP_MSG_CHECK_MSG_MAGIC(queue_handle);

	sem_wait(&msg_cxt->msg_sem);

	pthread_mutex_lock(&msg_cxt->mutex);

	if (msg_cxt->msg_number == 0) {
		pthread_mutex_unlock(&msg_cxt->mutex);
		ISP_LOG("MSG underflow");
		return -ISP_MSG_UNDERFLOW;
	} else {
		if (msg_cxt->msg_read != msg_cxt->msg_write) {
			*message = *msg_cxt->msg_read;
			bzero(msg_cxt->msg_read, sizeof(struct isp_msg));
			msg_cxt->msg_read++;
			if (msg_cxt->msg_read > msg_cxt->msg_head + msg_cxt->msg_count - 1) {
				msg_cxt->msg_read = msg_cxt->msg_head;
			}
		}
		msg_cxt->msg_number --;
	}

	pthread_mutex_unlock(&msg_cxt->mutex);

	//CMR_LOGI("queue_handle 0x%x, msg type 0x%x", queue_handle, message->msg_type);
	return ISP_MSG_SUCCESS;
}

int isp_msg_post(cmr_handle queue_handle, struct isp_msg *message)
{
	struct isp_msg_cxt *msg_cxt = NULL;
	struct isp_msg     *ori_node = NULL;
	uint32_t 		   handler_id = 0;

	//CMR_LOGI("queue_handle 0x%x, msg type 0x%x ", queue_handle, message->msg_type);

	if (0 == queue_handle || NULL == message) {
		return -ISP_MSG_PARAM_ERR;
	}
	msg_cxt = (struct isp_msg_cxt*)queue_handle;
	ori_node = msg_cxt->msg_write;
	ISP_MSG_CHECK_MSG_MAGIC(queue_handle);

	pthread_mutex_lock(&msg_cxt->mutex);

	if ((msg_cxt->msg_number + 1) >= msg_cxt->msg_count) {
		pthread_mutex_unlock(&msg_cxt->mutex);
		ISP_LOG("MSG Overflow");
		return -ISP_MSG_OVERFLOW;
	} else {
		*msg_cxt->msg_write++ = *message;
		if (msg_cxt->msg_write > msg_cxt->msg_head + msg_cxt->msg_count - 1) {
			msg_cxt->msg_write = msg_cxt->msg_head;
		}

		if (msg_cxt->msg_write == msg_cxt->msg_read) {
			msg_cxt->msg_write = ori_node;
		}
		msg_cxt->msg_number ++;
	}

	pthread_mutex_unlock(&msg_cxt->mutex);

	sem_post(&msg_cxt->msg_sem);
	return ISP_MSG_SUCCESS;
}

int isp_msg_peak(cmr_handle queue_handle, struct isp_msg *message)
{
	struct isp_msg_cxt *msg_cxt = (struct isp_msg_cxt*)queue_handle;
	uint32_t           msg_cnt = 0;
	int                rtn = 0;
	uint32_t 		   handler_id = 0;

	if (0 == queue_handle || NULL == message) {
		return -ISP_MSG_PARAM_ERR;
	}

	ISP_MSG_CHECK_MSG_MAGIC(queue_handle);
	rtn = sem_trywait(&msg_cxt->msg_sem);
	if (rtn) {
		return -ISP_MSG_NO_OTHER_MSG;
	}
	pthread_mutex_lock(&msg_cxt->mutex);

	if (msg_cxt->msg_read != msg_cxt->msg_write) {
		*message = *msg_cxt->msg_read++;
		if (msg_cxt->msg_read > msg_cxt->msg_head + msg_cxt->msg_count - 1) {
			msg_cxt->msg_read = msg_cxt->msg_head;
		}
		msg_cxt->msg_number --;
	} else {
		ISP_LOG("No more unread msg");
		return -ISP_MSG_NO_OTHER_MSG;
	}

	pthread_mutex_unlock(&msg_cxt->mutex);

	//CMR_LOGI("queue_handle 0x%x, drop msg type 0x%x", queue_handle, message->msg_type);
	return ISP_MSG_SUCCESS;
}

int isp_msg_queue_destroy(cmr_handle queue_handle)
{
	struct isp_msg_cxt *msg_cxt = (struct isp_msg_cxt*)queue_handle;
	uint32_t 		   handler_id = 0;

	ISP_LOG("queue_handle %p", queue_handle);

	if (0 == queue_handle) {
		ISP_LOG("zero queue_handle");
		return -ISP_MSG_PARAM_ERR;
	}

	ISP_MSG_CHECK_MSG_MAGIC(queue_handle);

	if (msg_cxt->msg_head) {
		free(msg_cxt->msg_head);
		msg_cxt->msg_head = NULL;
	}
	sem_destroy(&msg_cxt->msg_sem);
	pthread_mutex_destroy(&msg_cxt->mutex);
	bzero(msg_cxt, sizeof(*msg_cxt));
	free(msg_cxt);
	ISP_LOG("end");
	return ISP_MSG_SUCCESS;
}


