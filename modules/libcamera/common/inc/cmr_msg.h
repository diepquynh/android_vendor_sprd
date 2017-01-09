/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef CMR_MSG_H
#define CMR_MSG_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include "cmr_type.h"


enum {
	CMR_MSG_SUCCESS = 0,
	CMR_MSG_PARAM_ERR,
	CMR_MSG_INVALID_HANDLE,
	CMR_MSG_NO_OTHER_MSG,
	CMR_MSG_NO_MEM,
	CMR_MSG_OVERFLOW,
	CMR_MSG_UNDERFLOW,
	CMR_MSG_QUEUE_DESTROYED,
};


enum {
	CMR_MSG_SYNC_NONE = 0,
	CMR_MSG_SYNC_RECEIVED,
	CMR_MSG_SYNC_PROCESSED,
};


struct cmr_msg
{
	cmr_u32                    msg_type;
	cmr_u32                    sub_msg_type;
	void                       *data;
	cmr_u32                    alloc_flag; /*0 , no alloc; 1, data alloc-ed by the send */
	cmr_u32                    sync_flag; /*0 , no sync, post whatever is received or processed; 1, sync by it is received; 2 sync by it is processed*/
	void                       *cmr_priv; /*reserved by cmr thread, not opened for any user*/
};


#define MSG_INIT(name)                  \
{                                       \
	.msg_type     = 0,              \
	.sub_msg_type = 0,              \
	.data         = NULL,           \
	.alloc_flag   = 0,              \
	.sync_flag    = 0,              \
	.cmr_priv     = NULL,           \
}

#define CMR_MSG_INIT(name)               struct cmr_msg   name = MSG_INIT(name)


typedef cmr_int (*msg_process)(struct cmr_msg *message, void* p_data);

cmr_int cmr_msg_queue_create(cmr_u32 count, cmr_handle *queue_handle);

cmr_int cmr_msg_get(cmr_handle queue_handle, struct cmr_msg *message, cmr_u32 log_level);

cmr_int cmr_msg_timedget(cmr_handle queue_handle, struct cmr_msg *message);

cmr_int cmr_msg_post(cmr_handle queue_handle, struct cmr_msg *message, cmr_u32 log_level);

cmr_int cmr_msg_flush(cmr_handle queue_handle, struct cmr_msg *message);

cmr_int cmr_msg_queue_destroy(cmr_handle queue_handle);

cmr_int cmr_thread_create(cmr_handle *thread_handle, cmr_u32 queue_length, msg_process proc_cb, void* p_data);

cmr_int cmr_thread_destroy(cmr_handle thread_handle);

cmr_int cmr_thread_msg_send(cmr_handle thread_handle, struct cmr_msg *message);

cmr_int cmr_sem_init(sem_t *sem, cmr_int pshared, cmr_uint value);

cmr_int cmr_sem_destroy(sem_t *sem);

cmr_int cmr_sem_wait(sem_t *sem);

cmr_int cmr_sem_trywait(sem_t *sem);

cmr_int cmr_sem_timedwait(sem_t *sem, const struct timespec *abs_timeout);

cmr_int cmr_sem_post(sem_t *sem);

cmr_int cmr_sem_getvalue(sem_t *sem, cmr_int *valp);

#ifdef __cplusplus
}
#endif

#endif /* CMR_MSG_H */

