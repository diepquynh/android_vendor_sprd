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

#ifndef STUB_MSG_H
#define STUB_MSG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include "cmr_isp_type.h"


#define      STUB_MSG_MAGIC_CODE           0xEFFEA55A
#define      STUB_THREAD_MAGIC_CODE        0x5AA5FEEF

enum {
	STUB_MSG_SUCCESS = 0,
	STUB_MSG_PARAM_ERR,
	STUB_MSG_INVALID_HANDLE,
	STUB_MSG_NO_OTHER_MSG,
	STUB_MSG_NO_MEM,
};

enum {
	STUB_MSG_SYNC_NONE = 0,
	STUB_MSG_SYNC_RECEIVED,
	STUB_MSG_SYNC_PROCESSED,
};

enum {
    STUB_THREAD_INIT_EVT = 0x00FFFFFF,
    STUB_THREAD_EXIT_EVT = 0x01FFFFFF
};

enum {
    STUB_THREAD_EXIT = 0x1000,
};


struct stub_msg
{
	uint32_t                   msg_type;
	uint32_t                   sub_msg_type;
	void                       *data;
	uint32_t                   alloc_flag; /*0 , no alloc; 1, data alloc-ed by the send */
	void                       *respond;
	uint32_t                   sync_flag; /*0 , no sync, post whatever is received or processed; 1, sync by it is received; 2 sync by it is processed*/
};

struct stub_msg_cxt
{
	pthread_mutex_t            mutex;
	sem_t                      msg_sem;
	uint32_t                   msg_count;
	uint32_t                   msg_magic;
	struct stub_msg             *msg_head;
	struct stub_msg             *msg_write;
	struct stub_msg             *msg_read;
	sem_t                      sync_sem;
};

#define STUBMSGINIT(name)                  \
{                                       \
	.msg_type     = 0,              \
	.sub_msg_type = 0,              \
	.data         = NULL,           \
	.alloc_flag   = 0,              \
	.sync_flag    = 0		\
}

#define STUB_MSG_INIT(name)               struct stub_msg   name = STUBMSGINIT(name)

typedef int (*msg_process)(struct stub_msg *message, void* p_data);

int stub_msg_queue_create(uint32_t count, cmr_handle *queue_handle);

int stub_msg_get(cmr_handle queue_handle, struct stub_msg *message);

int stub_msg_post(cmr_handle queue_handle, struct stub_msg *message);

int stub_msg_flush(cmr_handle queue_handle, struct stub_msg *message);

int stub_msg_queue_destroy(cmr_handle queue_handle);

int stub_msg_peak(cmr_handle queue_handle, struct stub_msg *message);

int stub_thread_create(cmr_handle *thread_handle, uint32_t queue_length, msg_process proc_cb, void* p_data);

int stub_thread_destroy(cmr_handle thread_handle);

int stub_thread_msg_send(cmr_handle thread_handle, struct stub_msg *message);

#ifdef __cplusplus
}
#endif

#endif /* STUB_MSG_H */

