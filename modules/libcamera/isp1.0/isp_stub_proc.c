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
#include "isp_stub_proc.h"
#include "isp_stub_msg.h"
#include <sys/types.h>
#include <utils/Log.h>
#include <sched.h>
#include <unistd.h>
#define DEBUG_STR     "L %d, %s: "
#define DEBUG_ARGS    __LINE__,__FUNCTION__
#define STUB_LOGE(format,...) ALOGE(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

#define STUB_MSG_QUEUE_SIZE	5
#define STUB_EVT_START		((1 << 9) + 1)

struct stub_context_t {
	uint32_t	thread_id;
	uint32_t	*thread_handle_ptr;
	proc_func	func_ptr;
	proc_cb		cb;
	void		*param;
	uint32_t	is_need_destory;
};


static int thread_proc(struct stub_msg *message, void* p_data);


static uint32_t thread_handles[THREAD_ID_MAX] = {0};


int isp_stub_process(uint32_t		thread_id,
		     proc_func		func_ptr,
		     proc_cb		cb,
		     uint32_t		is_need_destory,
		     void		*param)
{
	int	 ret = STUB_MSG_SUCCESS;
	uint32_t *cur_handle_ptr = NULL;
	struct stub_context_t *context_ptr = NULL;
	STUB_MSG_INIT(message);

	if (thread_id >= THREAD_ID_MAX) {
		return STUB_MSG_PARAM_ERR;
	}

	STUB_LOGE("[STUB PROC] thread_id %d, flag %d", thread_id, is_need_destory);

	cur_handle_ptr = &thread_handles[thread_id];

	if (is_need_destory && !func_ptr && *cur_handle_ptr) {
		STUB_LOGE("[STUB PROC] directly destroy thread 0x%x", *cur_handle_ptr);
		stub_thread_destroy(*cur_handle_ptr);
		*cur_handle_ptr = 0;

		return ret;
	}

	if (0 == *cur_handle_ptr) {
		ret = stub_thread_create(cur_handle_ptr,
			STUB_MSG_QUEUE_SIZE,
			thread_proc,
			NULL);
		if (ret) {
			STUB_LOGE("[STUB PROC] thread %d create error", thread_id);
			return STUB_MSG_INVALID_HANDLE;
		}
		STUB_LOGE("[STUB PROC] thread_handle 0x%x created!", *cur_handle_ptr);
	}

	context_ptr = (struct stub_context_t *)malloc(sizeof(struct stub_context_t));
	if (!context_ptr) {
		STUB_LOGE("[STUB PROC] no mem");
		return STUB_MSG_NO_MEM;
	}
	memset(context_ptr, 0, sizeof(struct stub_context_t));

	context_ptr->thread_id 		= thread_id;
	context_ptr->thread_handle_ptr	= cur_handle_ptr;
	context_ptr->func_ptr		= func_ptr;
	context_ptr->cb			= cb;
	context_ptr->param		= param;
	context_ptr->is_need_destory	= is_need_destory;

	if (cb) {
		message.sync_flag = STUB_MSG_SYNC_NONE;
	} else {
		message.sync_flag = STUB_MSG_SYNC_PROCESSED;
	}
	message.msg_type   = STUB_EVT_START;
	message.alloc_flag = 1;
	message.data 	   = context_ptr;
	ret = stub_thread_msg_send(*cur_handle_ptr, &message);
	if (ret) {
		STUB_LOGE("[STUB PROC] Faile to send one msg to af thread");

		free(context_ptr);
		context_ptr = NULL;
	}

	return ret;
}

static int thread_proc(struct stub_msg *message, void* p_data)
{
	int	ret = STUB_MSG_SUCCESS;
	int	proc_ret = STUB_MSG_SUCCESS;
	struct stub_context_t *context_ptr = NULL;

	if (!message) {
		STUB_LOGE("[STUB PROC]param error");
		return STUB_MSG_PARAM_ERR;
	}

	STUB_LOGE("[STUB PROC] message.msg_type 0x%x, data 0x%lx", message->msg_type, (unsigned long)message->data);
	context_ptr = (struct stub_context_t *)message->data;

	switch (message->msg_type) {
	case STUB_EVT_START:
		STUB_LOGE("[STUB PROC] thread_id %d thread_handle 0x%x", context_ptr->thread_id, *(context_ptr->thread_handle_ptr));
		if (context_ptr->func_ptr) {
			proc_ret = context_ptr->func_ptr(context_ptr->param);
			if (context_ptr->cb) {
				context_ptr->cb(proc_ret, context_ptr->param);
			}
		}

		if (context_ptr->is_need_destory) {
			if (*(context_ptr->thread_handle_ptr)) {
				STUB_LOGE("destroy thread 0x%x after process", *(context_ptr->thread_handle_ptr));
				ret = STUB_THREAD_EXIT; /*return STUB_THREAD_EXIT to destory thread*/
				*(context_ptr->thread_handle_ptr) = 0;
			}
		}
		STUB_LOGE("[STUB PROC] process done!");
		break;

	default:
		STUB_LOGE("[STUB PROC]invalid msg type!");
		break;
	}

	return ret;
}

