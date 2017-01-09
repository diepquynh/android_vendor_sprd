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

#ifdef CONFIG_CAMERA_Y_DENOISE

#define LOG_TAG "ydenoise"

#include <sys/types.h>
#include <utils/Log.h>
#include <utils/Timers.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <time.h>
#include <fcntl.h>		/* low-level i/o */
#include <unistd.h>
#include <arm_neon.h>
#include <semaphore.h>
#include "cmr_ipm.h"
#include "cmr_ydenoise.h"
#include "isp_app.h"
#include "cmr_common.h"
#include "cmr_msg.h"
#include "ydenoise_params.h"

#define CAMERA_YDE_MSG_QUEUE_SIZE       5
#define THREAD_YDE                      1

#define CMR_EVT_YDE_BASE                (CMR_EVT_IPM_BASE + 0X400)
#define CMR_EVT_YDE_INIT                (CMR_EVT_YDE_BASE + 0)
#define CMR_EVT_YDE_START               (CMR_EVT_YDE_BASE + 1)
#define CMR_EVT_YDE_EXIT                (CMR_EVT_YDE_BASE + 2)

#define CHECK_HANDLE_VALID(handle) \
	do { \
		if (!handle) { \
			return -CMR_CAMERA_INVALID_PARAM; \
		} \
	} while(0)

typedef int (*proc_func) (void *param);
typedef void (*proc_cb) (cmr_handle class_handle, void *param);

struct class_yde {
	struct ipm_common common;
	sem_t denoise_sem_lock;
	cmr_handle thread_handles[THREAD_YDE];
	cmr_uint is_inited;
};

struct yde_start_param {
	proc_func func_ptr;
	proc_cb cb;
	cmr_handle caller_handle;
	void *param;
};

struct y_denoise_handle_param {
	int width;
	int height;
	uint8 *cnr_in;
	uint8 *cnr_out;
	int8 *mask;
};

/* temp code */
extern int get_cur_real_gain(void);

static struct class_ops yde_ops_tab_info;

static cmr_int yde_thread_proc(struct cmr_msg *message, void *private_data)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct class_yde *class_handle = (struct class_yde *)private_data;
	cmr_u32 evt = 0;
	struct yde_start_param *yde_start_ptr = NULL;

	if (!message || !class_handle) {
		CMR_LOGE("parameter is fail");
		return CMR_CAMERA_INVALID_PARAM;
	}

	evt = (cmr_u32) message->msg_type;

	switch (evt) {
	case CMR_EVT_YDE_INIT:

		break;

	case CMR_EVT_YDE_START:
		yde_start_ptr = (struct yde_start_param *)message->data;
		if (yde_start_ptr->func_ptr) {
			ret = yde_start_ptr->func_ptr(yde_start_ptr->param);
		}

		if (yde_start_ptr->cb) {
			yde_start_ptr->cb(class_handle, (void *)yde_start_ptr->param);
		}
		break;

	case CMR_EVT_YDE_EXIT:

		break;

	default:
		break;
	}

	return ret;
}

static cmr_int yde_thread_create(struct class_yde *class_handle)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct class_yde *yde_handle = (struct class_yde *)class_handle;
	cmr_handle *cur_handle_ptr = NULL;
	cmr_int thread_id = 0;

	CHECK_HANDLE_VALID(yde_handle);
	CMR_MSG_INIT(message);

	if (!class_handle->is_inited) {
		for (thread_id = 0; thread_id < THREAD_YDE; thread_id++) {
			cur_handle_ptr = &yde_handle->thread_handles[thread_id];
			ret = cmr_thread_create(cur_handle_ptr,
						CAMERA_YDE_MSG_QUEUE_SIZE, yde_thread_proc, (void *)class_handle);
			if (ret) {
				CMR_LOGE("send msg failed!");
				ret = CMR_CAMERA_FAIL;
				return ret;
			}
			message.sync_flag = CMR_MSG_SYNC_PROCESSED;
			message.msg_type = CMR_EVT_YDE_INIT;
			ret = cmr_thread_msg_send(*cur_handle_ptr, &message);
			if (CMR_CAMERA_SUCCESS != ret) {
				CMR_LOGE("msg send fail");
				ret = CMR_CAMERA_FAIL;
				return ret;
			}

		}

		class_handle->is_inited = 1;
	}

	return ret;
}

static cmr_int yde_thread_destroy(struct class_yde *class_handle)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct class_yde *yde_handle = (struct class_yde *)class_handle;
	cmr_uint thread_id = 0;

	CHECK_HANDLE_VALID(class_handle);

	if (yde_handle->is_inited) {
		for (thread_id = 0; thread_id < THREAD_YDE; thread_id++) {
			if (yde_handle->thread_handles[thread_id]) {
				ret = cmr_thread_destroy(yde_handle->thread_handles[thread_id]);
				if (ret) {
					CMR_LOGE("yde cmr_thread_destroy fail");
					return CMR_CAMERA_FAIL;
				}
				yde_handle->thread_handles[thread_id] = 0;
			}
		}
		yde_handle->is_inited = 0;
	}

	return ret;
}

static cmr_int yde_open(cmr_handle ipm_handle, struct ipm_open_in *in, struct ipm_open_out *out,
			 cmr_handle * class_handle)
{
	UNUSED(in);
	UNUSED(out);

	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct class_yde *yde_handle = NULL;

	if (!ipm_handle || !class_handle) {
		CMR_LOGE("Invalid Param!");
		return CMR_CAMERA_INVALID_PARAM;
	}

	yde_handle = (struct class_yde *)malloc(sizeof(struct class_yde));
	if (!yde_handle) {
		CMR_LOGE("No mem!");
		return CMR_CAMERA_NO_MEM;
	}

	cmr_bzero(yde_handle, sizeof(struct class_yde));

	yde_handle->common.ipm_cxt = (struct ipm_context_t *)ipm_handle;
	yde_handle->common.class_type = IPM_TYPE_YDE;
	yde_handle->common.ops = &yde_ops_tab_info;
	sem_init(&yde_handle->denoise_sem_lock, 0, 0);
	ret = yde_thread_create(yde_handle);
	if (ret) {
		CMR_LOGE("HDR error: create thread.");
		goto exit;
	}

	*class_handle = (cmr_handle) yde_handle;
	return ret;

exit:
	if (NULL != yde_handle)
		free(yde_handle);
	return CMR_CAMERA_FAIL;

}

static cmr_int yde_close(cmr_handle class_handle)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct class_yde *yde_handle = (struct class_yde *)class_handle;

	if (!yde_handle) {
		return -CMR_CAMERA_INVALID_PARAM;
	}

	ret = yde_thread_destroy(yde_handle);

	if (yde_handle)
		free(yde_handle);

	return ret;
}

static int y_proc_func(void* param)
{
	struct y_denoise_handle_param *yde_param = (struct y_denoise_handle_param *)param;
	if (NULL == param) {
		CMR_LOGE("error param");
		return -1;
	}
	CMR_LOGI("ydenoise begin width = %d, height = %d", yde_param->width, yde_param->height);
	ynoise_function(yde_param->cnr_in,
			//yde_param->cnr_out,
			yde_param->cnr_in, // use in ptr
			yde_param->mask,
			yde_param->width,
			yde_param->height);
	return 0;
}

static void y_proc_cb(cmr_handle class_handle, void *param)
{
	UNUSED(param);

	struct class_yde *yde_handle = (struct class_yde *)class_handle;
	CMR_LOGI("%s called! release sem lock", __func__);
	sem_post(&yde_handle->denoise_sem_lock);
}

static cmr_int yde_start(cmr_handle class_handle, cmr_uint thread_id, proc_func func_ptr, proc_cb cb, void *param)
{
	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct class_yde *yde_handle = (struct class_yde *)class_handle;
	cmr_handle *cur_handle_ptr = NULL;
	struct yde_start_param *yde_start_ptr = NULL;

	CMR_MSG_INIT(message);

	if (!class_handle) {
		CMR_LOGE("parameter is NULL. fail");
		return -CMR_CAMERA_INVALID_PARAM;
	}

	yde_start_ptr = (struct yde_start_param *)malloc(sizeof(struct yde_start_param));
	if (!yde_start_ptr) {
		CMR_LOGE("no mem");
		return CMR_CAMERA_NO_MEM;
	}
	memset(yde_start_ptr, 0, sizeof(struct yde_start_param));
	cur_handle_ptr = &yde_handle->thread_handles[thread_id];
	yde_start_ptr->func_ptr = func_ptr;
	yde_start_ptr->param = param;
	yde_start_ptr->cb = cb;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.msg_type = CMR_EVT_YDE_START;
	message.alloc_flag = 1;
	message.data = yde_start_ptr;
	ret = cmr_thread_msg_send(*cur_handle_ptr, &message);
	if (ret) {
		CMR_LOGE("send msg fail");
		if (yde_start_ptr)
			free(yde_start_ptr);
		ret = CMR_CAMERA_FAIL;
	}

	return ret;
}

static cmr_int yde_choose_mask(cmr_uint sensor_id, int gain, int8 **mask)
{
	cmr_int ret = -1;
	static int paten_cnt = 0;
	paten_cnt++;
	paten_cnt = paten_cnt % YDENOISE_MASK_SIZE;
	CMR_LOGI("sensor_id = %lu gain = %d paten_cnt = %d", sensor_id, gain, paten_cnt);
	if (0 == sensor_id) {
		if (100 > gain)
			goto exit_gain;
		else if ((100 <= gain) && (150 > gain))
			*mask = mask_parm.mask_1gain_to_lessthan_1d5gain[paten_cnt];
		else if ((150 <= gain) && (400 > gain))
			*mask = mask_parm.mask_1d5gain_to_lessthan_4gain[paten_cnt];
		else if ((400 <= gain) && (800 > gain))
			*mask = mask_parm.mask_4gain_to_lessthan_8gain[paten_cnt];
		else if ((800 <= gain) && (1600 > gain))
			*mask = mask_parm.mask_8gain_to_lessthan_16gain[paten_cnt];
		else if (1600 <= gain)
			*mask = mask_parm.mask_morethan_16gain[paten_cnt];
	} else if (1 == sensor_id) {
		if (100 > gain)
			goto exit_gain;
		else if ((100 <= gain) && (150 > gain))
			*mask = front_mask_parm.mask_1gain_to_lessthan_1d5gain[paten_cnt];
		else if ((150 <= gain) && (400 > gain))
			*mask = front_mask_parm.mask_1d5gain_to_lessthan_4gain[paten_cnt];
		else if ((400 <= gain) && (800 > gain))
			*mask = front_mask_parm.mask_4gain_to_lessthan_8gain[paten_cnt];
		else if ((800 <= gain) && (1600 > gain))
			*mask = front_mask_parm.mask_8gain_to_lessthan_16gain[paten_cnt];
		else if (1600 <= gain)
			*mask = front_mask_parm.mask_morethan_16gain[paten_cnt];
	}

	ret = 0;
exit_gain:
	return ret;
}

static cmr_int yde_transfer_frame(cmr_handle class_handle, struct ipm_frame_in *in, struct ipm_frame_out *out)
{
	UNUSED(out);

	cmr_int ret = CMR_CAMERA_SUCCESS;
	struct class_yde *yde_handle = (struct class_yde *)class_handle;
	cmr_uint thread_id = 0;
	struct y_denoise_handle_param yde_param = {0x00};
	cmr_uint sensor_id = 0;

	if (!in || !class_handle) {
		CMR_LOGE("Invalid Param!");
		return CMR_CAMERA_INVALID_PARAM;
	}
	sensor_id = yde_handle->common.ipm_cxt->init_in.sensor_id;

	yde_param.height = in->src_frame.size.height;
	yde_param.width = in->src_frame.size.width;
	yde_param.cnr_in = (uint8 *)in->src_frame.addr_vir.addr_y;

	int real_gain = get_cur_real_gain();
	int gain = real_gain * 100 / 128;

	if (0 != yde_choose_mask(sensor_id, gain, &yde_param.mask))
		goto ydenoise_exit;

	//int i = 0;
	//for (i = 0; i < 10; i++)
	//    CMR_LOGE("before yde_param.cnr_in[%d] = %d", i, yde_param.cnr_in[i]);
	yde_start(yde_handle, thread_id, y_proc_func, y_proc_cb, &yde_param);
	sem_wait(&yde_handle->denoise_sem_lock);
	//for (i = 0; i < 10; i++)
	//	CMR_LOGE("after yde_param.cnr_in[%d] = %d", i, yde_param.cnr_in[i]);
ydenoise_exit:
	return ret;
}

static cmr_int yde_pre_proc(cmr_handle class_handle)
{
	UNUSED(class_handle);

	cmr_int ret = CMR_CAMERA_SUCCESS;

	/*no need to do */

	return ret;
}

static cmr_int yde_post_proc(cmr_handle class_handle)
{
	UNUSED(class_handle);

	cmr_int ret = CMR_CAMERA_SUCCESS;

	/*no need to do */

	return ret;
}

static struct class_ops yde_ops_tab_info = {
	.open = yde_open,
	.close = yde_close,
	.transfer_frame = yde_transfer_frame,
	.pre_proc = yde_pre_proc,
	.post_proc = yde_post_proc,
};

struct class_tab_t yde_tab_info = {
	&yde_ops_tab_info,
};
#endif
