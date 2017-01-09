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
#define LOG_TAG "cmr_msg"

#include <stdlib.h>
#include "cmr_common.h"
#include "cmr_msg.h"

#define      CMR_MSG_MAGIC_CODE           0xEFFEA55A
#define      CMR_MSG_WAIT_TIME            1000 //wait for 1 ms
#define      CMR_MSG_POLLING_PERIOD       200000000
#define      CMR_THREAD_MAGIC_CODE        0x5AA5FEEF

#define MSG_CHECK_MSG_MAGIC(handle) \
	do { \
		if (((struct cmr_msg_cxt*)handle)->msg_magic != CMR_MSG_MAGIC_CODE) { \
			return CMR_MSG_INVALID_HANDLE; \
		} \
	} while(0)

struct cmr_msg_in {
	struct cmr_msg             msg;
	sem_t                      sem;
	cmr_u32                    sync_f;
};

struct cmr_msg_cxt
{
	pthread_mutex_t            mutex;
	sem_t                      msg_sem;
	cmr_u32                    msg_count;
	cmr_u32                    msg_number;
	cmr_u32                    msg_magic;
	struct cmr_msg_in          *msg_head;
	struct cmr_msg_in          *msg_write;
	struct cmr_msg_in          *msg_read;
	cmr_u8                     will_destroy_msg_queue_flag;
};

struct cmr_thread
{
	pthread_t                  thread_handle;
	cmr_handle                 queue_handle;
	void*                      p_data;//specific data
	cmr_int                    magic;
	msg_process                msg_process_cb;
};

enum {
	CMR_THREAD_INIT_EVT = 0x00FFFFFF,
	CMR_THREAD_EXIT_EVT = 0x01FFFFFF
};

enum {
	CMR_THREAD_EXIT = 0x1000,
};
cmr_int cmr_msg_queue_create(cmr_u32 count, cmr_handle *queue_handle)
{
	struct cmr_msg_cxt       *msg_cxt;
	struct cmr_msg_in        *msg_cur;
	cmr_u32                  i;

	if (0 == count) {
		return -CMR_MSG_PARAM_ERR;
	}
	msg_cxt = (struct cmr_msg_cxt*)malloc(sizeof(struct cmr_msg_cxt));
	if (NULL == msg_cxt) {
		return -CMR_MSG_NO_MEM;
	}
	bzero(msg_cxt, sizeof(*msg_cxt));
	msg_cxt->msg_head = (struct cmr_msg_in*)malloc((unsigned int)(count * sizeof(struct cmr_msg_in)));
	if (NULL == msg_cxt->msg_head) {
		free(msg_cxt);
		return -CMR_MSG_NO_MEM;
	}

	memset(msg_cxt->msg_head, 0, (unsigned int)(count * sizeof(struct cmr_msg_in)));

	msg_cxt->msg_magic = CMR_MSG_MAGIC_CODE;
	msg_cxt->msg_count = count;
	msg_cxt->msg_number= 0;
	msg_cxt->msg_read = msg_cxt->msg_head;
	msg_cxt->msg_write = msg_cxt->msg_head;
	pthread_mutex_init(&msg_cxt->mutex, NULL);
	sem_init(&msg_cxt->msg_sem, 0, 0);
	msg_cur = msg_cxt->msg_head;
	msg_cxt->will_destroy_msg_queue_flag = 0;
	for (i = 0; i < count; i++) {
		sem_init(&msg_cur->sem, 0, 0);
		msg_cur++;
	}
	*queue_handle = (cmr_handle)msg_cxt;
	CMR_LOGD("queue_handle 0x%lx", (cmr_uint)*queue_handle);

	return CMR_MSG_SUCCESS;
}

cmr_int cmr_msg_get(cmr_handle queue_handle, struct cmr_msg *message, cmr_u32 log_level)
{
	struct cmr_msg_cxt       *msg_cxt = (struct cmr_msg_cxt*)queue_handle;
	struct cmr_msg_in        *msg_cur = NULL;

	if (0 == queue_handle || NULL == message) {
		return -CMR_MSG_PARAM_ERR;
	}

	MSG_CHECK_MSG_MAGIC(queue_handle);

	sem_wait(&msg_cxt->msg_sem);

	pthread_mutex_lock(&msg_cxt->mutex);

	if (msg_cxt->msg_number == 0) {
		pthread_mutex_unlock(&msg_cxt->mutex);
		CMR_LOGE("MSG underflow");
		return CMR_MSG_UNDERFLOW;
	} else {
		if (msg_cxt->msg_read != msg_cxt->msg_write) {
			msg_cur = msg_cxt->msg_read;

			memcpy((void*)message, (void*)&msg_cur->msg, sizeof(struct cmr_msg));
			message->cmr_priv = msg_cur;
			bzero(&msg_cur->msg, sizeof(struct cmr_msg));
			msg_cxt->msg_read++;
			if (msg_cxt->msg_read > msg_cxt->msg_head + msg_cxt->msg_count - 1) {
				msg_cxt->msg_read = msg_cxt->msg_head;
			}

			msg_cxt->msg_number--;
		}

	}

	pthread_mutex_unlock(&msg_cxt->mutex);
/*	CMR_LOGD("msg_cur 0x%lx", (cmr_uint)msg_cur);*/

	if (NULL != msg_cur) {
		if (CMR_MSG_SYNC_RECEIVED == msg_cur->sync_f) {
			sem_post(&msg_cur->sem);
		}
	}
	if (0 != log_level) {
		CMR_LOGV("queue_handle 0x%lx, msg type 0x%x num %d cnt %d",
			(cmr_uint)queue_handle,
			message->msg_type,
			msg_cxt->msg_number,
			msg_cxt->msg_count);
	} else {
		CMR_LOGV("queue_handle 0x%lx, msg type 0x%x num %d cnt %d",
			(cmr_uint)queue_handle,
			message->msg_type,
			msg_cxt->msg_number,
			msg_cxt->msg_count);
	}
	return CMR_MSG_SUCCESS;
}

cmr_int cmr_msg_timedget(cmr_handle queue_handle, struct cmr_msg *message)
{
	struct cmr_msg_cxt       *msg_cxt = (struct cmr_msg_cxt*)queue_handle;
	struct timespec          ts;
	struct cmr_msg_in        *msg_cur = NULL;
	cmr_int                  ret;

	/* Posix mandates CLOCK_REALTIME here */
	clock_gettime( CLOCK_REALTIME, &ts );
	if (0 == queue_handle || NULL == message) {
		return -CMR_MSG_PARAM_ERR;
	}

	MSG_CHECK_MSG_MAGIC(queue_handle);

	clock_gettime(CLOCK_REALTIME, &ts);

	/* Check it as per Posix */
	if (ts.tv_sec < 0 ||
		ts.tv_nsec < 0 ||
		ts.tv_nsec >= 1000000000) {
		return -CMR_MSG_PARAM_ERR;
	}
	/*wait for 200ms*/
	ts.tv_nsec += CMR_MSG_POLLING_PERIOD;
	if (ts.tv_nsec > 1000000000) {
		ts.tv_sec += 1;
		ts.tv_nsec -= 1000000000;
	}

	ret = sem_timedwait(&msg_cxt->msg_sem, &ts);

	if (ret) {
		return CMR_MSG_NO_OTHER_MSG;
	}

	pthread_mutex_lock(&msg_cxt->mutex);

	if (msg_cxt->msg_number == 0) {
		pthread_mutex_unlock(&msg_cxt->mutex);
		CMR_LOGE("MSG underflow");
		return CMR_MSG_UNDERFLOW;
	} else {
		if (msg_cxt->msg_read != msg_cxt->msg_write) {
			msg_cur = msg_cxt->msg_read;
			*message = *(&msg_cur->msg);
			bzero(&msg_cur->msg, sizeof(struct cmr_msg));
			msg_cxt->msg_read++;
			if (msg_cxt->msg_read > msg_cxt->msg_head + msg_cxt->msg_count - 1) {
				msg_cxt->msg_read = msg_cxt->msg_head;
			}
		}
		msg_cxt->msg_number --;
	}

	pthread_mutex_unlock(&msg_cxt->mutex);
	if (NULL != msg_cur) {
		if (CMR_MSG_SYNC_RECEIVED == msg_cur->sync_f) {
			sem_post(&msg_cur->sem);
		}
	}
	CMR_LOGD("queue_handle 0x%lx, msg type 0x%x num %d cnt %d",
		(cmr_uint)queue_handle,
		message->msg_type,
		msg_cxt->msg_number,
		msg_cxt->msg_count);
	return CMR_MSG_SUCCESS;
}

cmr_int cmr_msg_post(cmr_handle queue_handle, struct cmr_msg *message, cmr_u32 log_level)
{
	struct cmr_msg_cxt       *msg_cxt = (struct cmr_msg_cxt*)queue_handle;
	struct cmr_msg_in        *ori_node;
	struct cmr_msg_in        *msg_cur = NULL;
	cmr_int                  rtn = CMR_MSG_SUCCESS;
	cmr_int                  reserved_msg_entry;

	if (0 == queue_handle || NULL == message) {
		CMR_LOGW("post msg to NULL queue! discard");
		return -CMR_MSG_PARAM_ERR;
	}
	ori_node = msg_cxt->msg_write;

	if (0 != log_level) {
		CMR_LOGV("queue_handle 0x%lx, msg type 0x%x num %d cnt %d",
			(cmr_uint)queue_handle,
			message->msg_type,
			msg_cxt->msg_number,
			msg_cxt->msg_count);
	} else {
		CMR_LOGV("queue_handle 0x%lx, msg type 0x%x num %d cnt %d",
			(cmr_uint)queue_handle,
			message->msg_type,
			msg_cxt->msg_number,
			msg_cxt->msg_count);
	}
	MSG_CHECK_MSG_MAGIC(queue_handle);

	pthread_mutex_lock(&msg_cxt->mutex);

	if (msg_cxt->will_destroy_msg_queue_flag){
		CMR_LOGI("will_destroy_msg_queue_flag %d, queus destroyed", msg_cxt->will_destroy_msg_queue_flag);
		pthread_mutex_unlock(&msg_cxt->mutex);
		return CMR_MSG_QUEUE_DESTROYED;
	}

	//always reserve one more place in message queue for possible CMR_THREAD_EXIT_EVT
	if(message->msg_type != CMR_THREAD_EXIT_EVT){
		reserved_msg_entry = 2;
	} else {
		reserved_msg_entry = 1;
	}
	if ((msg_cxt->msg_number + reserved_msg_entry) >= msg_cxt->msg_count) {
		pthread_mutex_unlock(&msg_cxt->mutex);
		CMR_LOGE("MSG Overflow");
		return CMR_MSG_OVERFLOW;
	} else {
		msg_cur = msg_cxt->msg_write;
		msg_cur->sync_f = message->sync_flag;
		memcpy((void*)&msg_cur->msg, (void*)message, sizeof(struct cmr_msg));
		msg_cxt->msg_write ++;
		if (msg_cxt->msg_write > msg_cxt->msg_head + msg_cxt->msg_count - 1) {
			msg_cxt->msg_write = msg_cxt->msg_head;
		}

		if (msg_cxt->msg_write == msg_cxt->msg_read) {
			msg_cxt->msg_write = ori_node;
		}
		msg_cxt->msg_number ++;
	}

	if(message->msg_type == CMR_THREAD_EXIT_EVT){
		msg_cxt->will_destroy_msg_queue_flag = 1;
		CMR_LOGI("posting CMR_THREAD_EXIT_EVT, set will_destroy_msg_queue_flag");
	}

	pthread_mutex_unlock(&msg_cxt->mutex);

/*	CMR_LOGD("msg_cur 0x%lx", (cmr_uint)msg_cur);*/
	sem_post(&msg_cxt->msg_sem);
	if (NULL != msg_cur) {
		if (CMR_MSG_SYNC_NONE != msg_cur->sync_f) {
			rtn = sem_wait(&msg_cur->sem);
		}
	}
	return rtn;
}

cmr_int cmr_msg_queue_destroy(cmr_handle queue_handle)
{
	struct cmr_msg_cxt       *msg_cxt = (struct cmr_msg_cxt*)queue_handle;
	struct cmr_msg_in        *msg_cur = NULL;

	CMR_LOGD("queue_handle 0x%lx", (cmr_uint)queue_handle);

	if (0 == queue_handle) {
		CMR_LOGE("zero queue_handle");
		return -CMR_MSG_PARAM_ERR;
	}

	MSG_CHECK_MSG_MAGIC(queue_handle);

	if(msg_cxt->msg_number != 0){
		CMR_LOGE("destroying an unempty msg queue, might cause mem leak!!!");
		while(msg_cxt->msg_read != msg_cxt->msg_write){
			msg_cur = msg_cxt->msg_read;
			if(msg_cur->msg.alloc_flag == 1){
				if(msg_cur->msg.data){
					free(msg_cur->msg.data);
				}
			}
			sem_post(&msg_cur->sem);
			msg_cxt->msg_read++;
			if (msg_cxt->msg_read > msg_cxt->msg_head + msg_cxt->msg_count - 1) {
				msg_cxt->msg_read = msg_cxt->msg_head;
			}
		}
	}

	if (msg_cxt->msg_head) {
		free(msg_cxt->msg_head);
		msg_cxt->msg_head = NULL;
	}
	sem_destroy(&msg_cxt->msg_sem);
	pthread_mutex_destroy(&msg_cxt->mutex);
	bzero(msg_cxt, sizeof(*msg_cxt));
	free(msg_cxt);
	msg_cxt = NULL;

	return CMR_MSG_SUCCESS;
}

static void *cmr_common_routine(void *client_data)
{
	cmr_int                  ret = CMR_MSG_SUCCESS;
	struct cmr_thread        *thread = NULL;
	struct cmr_msg_cxt       *msg_cxt = NULL;
	struct cmr_msg_in        *msg_cur;

	CMR_MSG_INIT(message);

	if (!client_data) {
		CMR_LOGE("Invalid parameter");
		return NULL;
	}

	thread = (struct cmr_thread*)client_data;
	msg_cxt = (struct cmr_msg_cxt*)thread->queue_handle;
	while (1) {
		ret = cmr_msg_get(thread->queue_handle, &message, 1);
		if (ret) {
			CMR_LOGE("Message queue destroied");
			break;
		}

		CMR_LOGV("thread 0x%p, data 0x%p, message.msg_type 0x%x, sub-type 0x%x",
			thread,
			thread->p_data,
			message.msg_type,
			message.sub_msg_type);

		switch (message.msg_type) {
		case CMR_THREAD_INIT_EVT:
			break;
		case CMR_THREAD_EXIT_EVT:
			ret = CMR_THREAD_EXIT;
			break;
		default:
			ret = (*thread->msg_process_cb)(&message, thread->p_data);
			break;
		}

		msg_cur = (struct cmr_msg_in*)message.cmr_priv;
		if (NULL != msg_cur) {
			if (CMR_MSG_SYNC_PROCESSED == msg_cur->sync_f) {
				sem_post(&msg_cur->sem);
			}
		}

		if (message.alloc_flag) {
			if (message.data) {
				free(message.data);
				message.data = 0;
			}
		}

		if (CMR_THREAD_EXIT == ret) {
			CMR_LOGV("thread exit 0x%p", thread);
			break;
		}
	}


	return NULL;

}
cmr_int cmr_thread_create(cmr_handle *thread_handle, cmr_u32 queue_length, msg_process proc_cb, void* p_data)
{
	cmr_int            rtn = CMR_MSG_SUCCESS;
	pthread_attr_t     attr;
	struct cmr_thread  *thread = NULL;
	CMR_MSG_INIT(message);

	if (!thread_handle || !queue_length || !proc_cb) {
		return CMR_MSG_PARAM_ERR;
	}
	*thread_handle = 0;
	thread = (struct cmr_thread*)malloc(sizeof(struct cmr_thread));
	if (!thread) {
		CMR_LOGE("No mem!");
		return CMR_MSG_NO_MEM;
	}
	thread->magic = CMR_THREAD_MAGIC_CODE;
	thread->p_data = p_data;
	thread->msg_process_cb = proc_cb;
	CMR_LOGV("thread 0x%p, data 0x%p", thread, p_data);
	rtn = cmr_msg_queue_create(queue_length, &thread->queue_handle);
	if (rtn) {
		CMR_LOGE("No mem to create msg queue");
		free((void*)thread);
		return rtn;
	}

	pthread_attr_init (&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	rtn = pthread_create(&thread->thread_handle, &attr, cmr_common_routine, thread);
	if (rtn) {
		CMR_LOGE("Fail to create thread");
		free((void*)thread->queue_handle);
		free((void*)thread);
		return rtn;
	}
	message.msg_type = CMR_THREAD_INIT_EVT;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	rtn = cmr_msg_post(thread->queue_handle, &message, 1);
	if (rtn) {
		CMR_LOGE("Fail to send INIT message to thread");
		free((void*)thread->queue_handle);
		free((void*)thread);
		return rtn;
	}

	pthread_attr_destroy(&attr);
	*thread_handle = (cmr_handle)thread;
	return rtn;
}

cmr_int cmr_thread_destroy(cmr_handle thread_handle)
{
	CMR_MSG_INIT(message);
	cmr_int                  ret = CMR_MSG_SUCCESS;
	struct cmr_thread        *thread = (struct cmr_thread *)thread_handle;
	struct cmr_msg_cxt       *msg_cxt = NULL;

	message.msg_type = CMR_THREAD_EXIT_EVT;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	ret = cmr_thread_msg_send(thread_handle, &message);
	if(ret){
		CMR_LOGE("failed sending CMR_THREAD_EXIT_EVT!!! ");
	}
	if (thread_handle) {
		cmr_msg_queue_destroy(thread->queue_handle);
		free(thread_handle);
		thread_handle = NULL;
	}
	return ret;
}

cmr_int cmr_thread_msg_send(cmr_handle thread_handle, struct cmr_msg *message)
{
	cmr_int                  ret = CMR_MSG_SUCCESS;
	struct cmr_thread        *thread = NULL;

	if (!thread_handle || !message) {
		return CMR_MSG_PARAM_ERR;
	}

	thread = (struct cmr_thread*)thread_handle;
	ret = cmr_msg_post(thread->queue_handle, message, 1);
	return ret;
}

cmr_int cmr_sem_init(sem_t *sem, cmr_int pshared, cmr_uint value)
{
	return sem_init(sem, pshared, value);
}

cmr_int cmr_sem_destroy(sem_t *sem)
{
	return sem_destroy(sem);
}

cmr_int cmr_sem_wait(sem_t *sem)
{
	return sem_wait(sem);
}

cmr_int cmr_sem_trywait(sem_t *sem)
{
	return sem_trywait(sem);
}

cmr_int cmr_sem_timedwait(sem_t *sem, const struct timespec *abs_timeout)
{
	return sem_timedwait(sem, abs_timeout);
}

cmr_int cmr_sem_post(sem_t *sem)
{
	return sem_post(sem);
}

cmr_int cmr_sem_getvalue(sem_t *sem, cmr_int *valp)
{
	return sem_getvalue(sem, valp);
}
