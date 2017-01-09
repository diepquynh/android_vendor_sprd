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

#ifndef ISP_APP_MSG_H
#define ISP_APP_MSG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include "cmr_isp_type.h"

#define      ISP_APP_MSG_MAGIC_CODE           0xEFFEA55A

enum {
	ISP_APP_MSG_SUCCESS = 0,
	ISP_APP_MSG_PARAM_ERR,
	ISP_APP_MSG_INVALID_HANDLE,
	ISP_APP_MSG_NO_OTHER_MSG,
	ISP_APP_MSG_NO_MEM,
};

struct isp_app_msg
{
	uint32_t                   handler_id;
	uint32_t                   msg_type;
	uint32_t                   sub_msg_type;
	void                       *data;
	uint32_t                   data_len;
	uint32_t                   alloc_flag; /*0 , no alloc; 1, data alloc-ed by the send */
	void                       *respond;
};

struct isp_app_msg_cxt
{
	pthread_mutex_t            mutex;
	sem_t                      msg_sem;
	uint32_t                   msg_count;
	uint32_t                   msg_magic;
	struct isp_app_msg             *msg_head;
	struct isp_app_msg             *msg_write;
	struct isp_app_msg             *msg_read;
};

#define ISPAPPMSGINIT(name) \
{ \
	.handler_id = 0, \
	.msg_type = 0, \
	.sub_msg_type = 0, \
	.data = NULL, \
	.data_len = 0, \
	.alloc_flag = 0, \
	.respond = NULL, \
}

#define ISP_APP_MSG_INIT(name) struct isp_app_msg name = ISPAPPMSGINIT(name)

int isp_app_msg_queue_create(uint32_t count, cmr_handle *queue_handle);

int isp_app_msg_get(cmr_handle queue_handle, struct isp_app_msg *message);

int isp_app_msg_post(cmr_handle queue_handle, struct isp_app_msg *message);

int isp_app_msg_flush(cmr_handle queue_handle, struct isp_app_msg *message);

int isp_app_msg_queue_destroy(cmr_handle queue_handle);

int isp_app_msg_peak(cmr_handle queue_handle, struct isp_app_msg *message);


#ifdef __cplusplus
}
#endif

#endif /* CMR_MSG_H */

