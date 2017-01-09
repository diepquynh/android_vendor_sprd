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
#include <features.h>
#include <stdlib.h>
#include "isp_stub_msg.h"
#include <sys/types.h>
#include <utils/Log.h>
#include <sched.h>
#include <unistd.h>
#define DEBUG_STR     "L %d, %s: "
#define DEBUG_ARGS    __LINE__,__FUNCTION__

#define STUB_LOGE(format,...) ALOGE(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define STUB_MSG_CHECK_MSG_MAGIC(handle)           \
		do {                                                         \
		    if (((struct stub_msg_cxt*)handle)->msg_magic != STUB_MSG_MAGIC_CODE) {    \
				return STUB_MSG_INVALID_HANDLE;                                        \
			}                                                                    \
		} while(0)

struct stub_thread
{
    pthread_t		     thread_handle;
    cmr_handle		     queue_handle;
    void*		     p_data;//specific data
    uint32_t		     magic;
    msg_process 	     msg_process_cb;
};


int stub_msg_queue_create(unsigned int count, cmr_handle *queue_handle)
{
	struct stub_msg_cxt       *msg_cxt;

	//CMR_LOGI("count 0x%x", count);

	if (0 == count) {
		return -STUB_MSG_PARAM_ERR;
	}
	msg_cxt = (struct stub_msg_cxt*)malloc(sizeof(struct stub_msg_cxt));
	if (NULL == msg_cxt) {
		return -STUB_MSG_NO_MEM;
	}
	bzero(msg_cxt, sizeof(*msg_cxt));
	msg_cxt->msg_head = (struct stub_msg*)malloc((unsigned int)(count * sizeof(struct stub_msg)));
	if (NULL == msg_cxt->msg_head) {
		free(msg_cxt);
		return -STUB_MSG_NO_MEM;
	}
	msg_cxt->msg_magic = STUB_MSG_MAGIC_CODE;
	msg_cxt->msg_count = count;
	msg_cxt->msg_read  = msg_cxt->msg_head;
	msg_cxt->msg_write = msg_cxt->msg_head;
	pthread_mutex_init(&msg_cxt->mutex, NULL);
	sem_init(&msg_cxt->msg_sem, 0, 0);
	sem_init(&msg_cxt->sync_sem, 0, 0);
	*queue_handle = (cmr_handle)msg_cxt;
	//CMR_LOGI("queue_handle 0x%x", *queue_handle);

	return STUB_MSG_SUCCESS;
}

int stub_msg_get(cmr_handle queue_handle, struct stub_msg *message)
{
	struct stub_msg_cxt *msg_cxt = (struct stub_msg_cxt*)queue_handle;

	if (0 == queue_handle || NULL == message) {
		return -STUB_MSG_PARAM_ERR;
	}

	STUB_MSG_CHECK_MSG_MAGIC(queue_handle);

	sem_wait(&msg_cxt->msg_sem);

	pthread_mutex_lock(&msg_cxt->mutex);

	if (msg_cxt->msg_read != msg_cxt->msg_write) {
		*message = *msg_cxt->msg_read++;
		if (msg_cxt->msg_read > msg_cxt->msg_head + msg_cxt->msg_count - 1) {
			msg_cxt->msg_read = msg_cxt->msg_head;
		}
	}

	pthread_mutex_unlock(&msg_cxt->mutex);
	if (STUB_MSG_SYNC_RECEIVED == message->sync_flag) {
		sem_post(&msg_cxt->sync_sem);
	}

	//CMR_LOGI("queue_handle 0x%x, msg type 0x%x", queue_handle, message->msg_type);
	return STUB_MSG_SUCCESS;
}

int stub_msg_post(cmr_handle queue_handle, struct stub_msg *message)
{
	struct stub_msg_cxt *msg_cxt = (struct stub_msg_cxt*)queue_handle;
	struct stub_msg     *ori_node = NULL;
	int                rtn = STUB_MSG_SUCCESS;

	if (0 == queue_handle || NULL == message) {
		return -STUB_MSG_PARAM_ERR;
	}
	ori_node = msg_cxt->msg_write;

	STUB_MSG_CHECK_MSG_MAGIC(queue_handle);

	pthread_mutex_lock(&msg_cxt->mutex);

	*msg_cxt->msg_write++ = *message;
	if (msg_cxt->msg_write > msg_cxt->msg_head + msg_cxt->msg_count - 1) {
		msg_cxt->msg_write = msg_cxt->msg_head;
	}

	if (msg_cxt->msg_write == msg_cxt->msg_read) {
		msg_cxt->msg_write = ori_node;
	}

	pthread_mutex_unlock(&msg_cxt->mutex);

	sem_post(&msg_cxt->msg_sem);
	if (STUB_MSG_SYNC_NONE != message->sync_flag) {
		rtn = sem_wait(&msg_cxt->sync_sem);
	}
	return rtn;
}

int stub_msg_peak(cmr_handle queue_handle, struct stub_msg *message)
{
	struct stub_msg_cxt *msg_cxt = (struct stub_msg_cxt*)queue_handle;
	uint32_t           msg_cnt = 0;
	int                rtn = 0;

	if (0 == queue_handle || NULL == message) {
		return -STUB_MSG_PARAM_ERR;
	}

	STUB_MSG_CHECK_MSG_MAGIC(queue_handle);
	rtn = sem_trywait(&msg_cxt->msg_sem);
	if (rtn) {
		return -STUB_MSG_NO_OTHER_MSG;
	}
	pthread_mutex_lock(&msg_cxt->mutex);

	if (msg_cxt->msg_read != msg_cxt->msg_write) {
		*message = *msg_cxt->msg_read++;
		if (msg_cxt->msg_read > msg_cxt->msg_head + msg_cxt->msg_count - 1) {
			msg_cxt->msg_read = msg_cxt->msg_head;
		}
	} else {
		//CMR_LOGV("No more unread msg");
		return -STUB_MSG_NO_OTHER_MSG;
	}

	pthread_mutex_unlock(&msg_cxt->mutex);

	//CMR_LOGI("queue_handle 0x%x, drop msg type 0x%x", queue_handle, message->msg_type);
	return STUB_MSG_SUCCESS;
}

int stub_msg_queue_destroy(cmr_handle queue_handle)
{
	struct stub_msg_cxt *msg_cxt = (struct stub_msg_cxt*)queue_handle;

	//CMR_LOGI("queue_handle 0x%x", queue_handle);

	if (0 == queue_handle) {
		//CMR_LOGE("zero queue_handle");
		return -STUB_MSG_PARAM_ERR;
	}

	STUB_MSG_CHECK_MSG_MAGIC(queue_handle);

	if (msg_cxt->msg_head) {
		free(msg_cxt->msg_head);
		msg_cxt->msg_head = NULL;
	}
	sem_destroy(&msg_cxt->msg_sem);
	pthread_mutex_destroy(&msg_cxt->mutex);
	bzero(msg_cxt, sizeof(*msg_cxt));
	free(msg_cxt);

	return STUB_MSG_SUCCESS;
}


static void *stub_common_routine(void *client_data)
{
	int                      ret = STUB_MSG_SUCCESS;
	struct stub_thread        *thread = NULL;
	struct stub_msg_cxt       *msg_cxt = NULL;
	STUB_MSG_INIT(message);


	if (!client_data) {
		STUB_LOGE("Invalid parameter");
		return NULL;
	}
	//fangbing4-22
	//cpu_set_t parentCpuset;
#if 0
    	static int myid = 0;
	int core_num;
  	cpu_set_t cpuset;
  	CPU_ZERO(&cpuset);
  	CPU_SET(myid, &cpuset);
  	sched_setaffinity(0,sizeof(cpu_set_t),&cpuset);
 	core_num = sysconf(_SC_NPROCESSORS_CONF);
	int current_cpu = sched_getcpu();
	STUB_LOGE("[core_num]Message queue destroied%d",core_num);
	STUB_LOGE("[core_num]Message queue destroied%d",current_cpu);
	myid = (myid  + 1) % core_num;

#endif
	thread = (struct stub_thread*)client_data;
	msg_cxt = (struct stub_msg_cxt*)thread->queue_handle;
	while (1) {
		ret = stub_msg_get(thread->queue_handle, &message);
		if (ret) {
			STUB_LOGE("Message queue destroied");
			break;
		}

		STUB_LOGE("thread %p, data %p, message.msg_type 0x%x, sub-type 0x%x",
			thread,
			thread->p_data,
			message.msg_type,
			message.sub_msg_type);

		switch (message.msg_type) {
		case STUB_THREAD_INIT_EVT:
			break;
		case STUB_THREAD_EXIT_EVT:
			ret = STUB_THREAD_EXIT;

			break;
		default:
			ret = (*thread->msg_process_cb)(&message, thread->p_data);
			break;
		}

		if (STUB_MSG_SYNC_PROCESSED == message.sync_flag) {
			sem_post(&msg_cxt->sync_sem);
		}

		if (message.alloc_flag) {
			if (message.data) {
				free(message.data);
				message.data = 0;
			}
		}

		if (STUB_THREAD_EXIT == ret) {
			STUB_LOGE("[STUB PROC] thread exit %p", thread);
			break;
		}
	}


	return NULL;

}
int stub_thread_create(cmr_handle *thread_handle, uint32_t queue_length, msg_process proc_cb, void* p_data)
{
	int                rtn = STUB_MSG_SUCCESS;
	pthread_attr_t     attr;
	struct stub_thread  *thread = NULL;
	STUB_MSG_INIT(message);

	if (!thread_handle || !queue_length || !proc_cb) {
		return STUB_MSG_PARAM_ERR;
	}
	*thread_handle = 0;
	thread = (struct stub_thread*)malloc(sizeof(struct stub_thread));
	thread->magic = STUB_THREAD_MAGIC_CODE;
	thread->p_data = p_data;
	thread->msg_process_cb = proc_cb;
	STUB_LOGE("[STUB PROC] thread %p, data %p", thread, p_data);
	rtn = stub_msg_queue_create(queue_length, &thread->queue_handle);
	if (rtn) {
		STUB_LOGE("No mem to create msg queue");
		free((void*)thread);
		return rtn;
	}

	pthread_attr_init (&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	rtn = pthread_create(&thread->thread_handle, &attr, stub_common_routine, thread);
	if (rtn) {
		STUB_LOGE("Fail to create thread");
		free((void*)thread->queue_handle);
		free((void*)thread);
		return rtn;
	}
	message.msg_type = STUB_THREAD_INIT_EVT;
	message.sync_flag = STUB_MSG_SYNC_PROCESSED;
	rtn = stub_msg_post(thread->queue_handle, &message);
	if (rtn) {
		STUB_LOGE("Fail to send INIT message to thread");
		free((void*)thread->queue_handle);
		free((void*)thread);
		return rtn;
	}

	pthread_attr_destroy(&attr);
	*thread_handle = (unsigned long)thread;
	return rtn;
}

int stub_thread_destroy(cmr_handle thread_handle)
{
	STUB_MSG_INIT(message);
	int                      ret = STUB_MSG_SUCCESS;
	struct stub_thread        *thread = (struct stub_thread*)thread_handle;;
	struct stub_msg_cxt       *msg_cxt = NULL;

	if (!thread_handle) {
		STUB_LOGE("Invalid thread handle");
		return STUB_MSG_PARAM_ERR;
	}

	message.msg_type = STUB_THREAD_EXIT_EVT;
	message.sync_flag = STUB_MSG_SYNC_PROCESSED;
	ret = stub_thread_msg_send(thread_handle, &message);
	if (ret) {
		STUB_LOGE("Fail to send exit message to thread");
	}

	ret = stub_msg_queue_destroy(thread->queue_handle);
	if (ret) {
		STUB_LOGE("Fail to destory queue");
	}

	free((void*)thread);

	return ret;
}

int stub_thread_msg_send(cmr_handle thread_handle, struct stub_msg *message)
{
	int                      ret = STUB_MSG_SUCCESS;
	struct stub_thread        *thread = NULL;

	if (!thread_handle || !message) {
		return STUB_MSG_PARAM_ERR;
	}

	thread = (struct stub_thread*)thread_handle;
	ret = stub_msg_post(thread->queue_handle, message);
	return ret;
}


