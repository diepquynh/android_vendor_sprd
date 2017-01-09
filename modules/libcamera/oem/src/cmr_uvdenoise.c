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

#ifdef CONFIG_CAMERA_UV_DENOISE

#include <sys/types.h>
#include <utils/Log.h>
#include <utils/Timers.h>
#include <time.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <time.h>
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <arm_neon.h>
#include <semaphore.h>
#include "cmr_ipm.h"
#include "cmr_uvdenoise.h"
#include "isp_app.h"
#include "cmr_common.h"
#include "cmr_msg.h"

#define PIXEL_STRIDE                     2		//uv interleave
#define CAMERA_UVDE_MSG_QUEUE_SIZE       5
#define THREAD_UVDE                      4

#define CMR_EVT_UVDE_BASE                (CMR_EVT_IPM_BASE + 0X200)
#define CMR_EVT_UVDE_INIT                (CMR_EVT_UVDE_BASE + 0)
#define CMR_EVT_UVDE_START               (CMR_EVT_UVDE_BASE + 1)
#define CMR_EVT_UVDE_EXIT                (CMR_EVT_UVDE_BASE + 2)

#define CHECK_HANDLE_VALID(handle) \
	do { \
		if (!handle) { \
			return -CMR_CAMERA_INVALID_PARAM; \
		} \
	} while(0)

typedef int (*proc_func)(void* param);
typedef void (*proc_cb)(cmr_handle class_handle, void* param);

struct class_uvde {
	struct ipm_common                     common;
	sem_t                                 denoise_sem_lock;
	cmr_handle                            thread_handles[THREAD_UVDE];
	cmr_uint                              is_inited;
};

struct uvde_start_param {
	proc_func                             func_ptr;
	proc_cb                               cb;
	cmr_handle                            caller_handle;
	void                                 *param;
};

static cmr_int uvde_open(cmr_handle ipm_handle, struct ipm_open_in *in, struct ipm_open_out *out,
				cmr_handle *out_class_handle);
static cmr_int uvde_close(cmr_handle class_handle);
static cmr_int uvde_transfer_frame(cmr_handle class_handle,struct ipm_frame_in *in, struct ipm_frame_out *out);
static cmr_int uvde_pre_proc(cmr_handle class_handle);
static cmr_int uvde_post_proc(cmr_handle class_handle);
static cmr_int uvde_start(cmr_handle class_handle, cmr_uint thread_id, proc_func func_ptr, proc_cb cb, void *param);
static cmr_int uvde_thread_create(struct class_uvde *class_handle);
static cmr_int uvde_thread_destroy(struct class_uvde *class_handle);
static cmr_int uvde_thread_proc(struct cmr_msg *message, void *private_data);
static void uv_proc_cb(cmr_handle class_handle, void* param);
static void add_border_uv(cmr_u8 *dst, cmr_u8 *src, cmr_u32 w, cmr_u32 h, cmr_u32 border_w, cmr_u32 border_h);

static struct class_ops uvde_ops_tab_info = {
	uvde_open,
	uvde_close,
	uvde_transfer_frame,
	uvde_pre_proc,
	uvde_post_proc,
};

struct class_tab_t uvde_tab_info = {
	&uvde_ops_tab_info,
};


static cmr_int uvde_open(cmr_handle ipm_handle, struct ipm_open_in *in, struct ipm_open_out *out,
				cmr_handle *class_handle)
{
	UNUSED(in);
	UNUSED(out);

	cmr_int                    ret = CMR_CAMERA_SUCCESS;
	struct class_uvde         *uvde_handle = NULL;

	if (!ipm_handle || !class_handle) {
		CMR_LOGE("Invalid Param!");
		return CMR_CAMERA_INVALID_PARAM;
	}

	uvde_handle = (struct class_uvde *)malloc(sizeof(struct class_uvde));
	if (!uvde_handle) {
		CMR_LOGE("No mem!");
		return CMR_CAMERA_NO_MEM;
	}

	cmr_bzero(uvde_handle, sizeof(struct class_uvde));

	uvde_handle->common.ipm_cxt = (struct ipm_context_t*)ipm_handle;
	uvde_handle->common.class_type = IPM_TYPE_UVDE;
	uvde_handle->common.ops = &uvde_ops_tab_info;
	sem_init(&uvde_handle->denoise_sem_lock, 0, 0);
	ret = uvde_thread_create(uvde_handle);
	if (ret) {
		CMR_LOGE("HDR error: create thread.");
		goto exit;
	}

	*class_handle = (cmr_handle)uvde_handle;
	return ret;

exit:
	if(NULL != uvde_handle)
		free(uvde_handle);
	return CMR_CAMERA_FAIL;

}

static cmr_int uvde_close(cmr_handle class_handle)
{
	cmr_int                     ret = CMR_CAMERA_SUCCESS;
	struct class_uvde          *uvde_handle = (struct class_uvde *)class_handle;
	cmr_handle                  *cur_handle_ptr = NULL;
	cmr_uint                    thread_id = 0;

	CHECK_HANDLE_VALID(uvde_handle);

	ret = uvde_thread_destroy(uvde_handle);

	if (uvde_handle)
		free(uvde_handle);

	return ret;
}

static cmr_int uvde_transfer_frame(cmr_handle class_handle, struct ipm_frame_in *in, struct ipm_frame_out *out)
{
	UNUSED(out);

	cmr_int                    ret = CMR_CAMERA_SUCCESS;
	struct class_uvde         *uvde_handle = (struct class_uvde *)class_handle;
	cmr_uint                   thread_id = 0;
	struct uvde_start_param   uv_param;
	struct isp_denoise_input  uvdenoise_input;
	struct isp_denoise_input  *denoise_in;
	cmr_uint                   uv_denoise_alg = 0;
	cmr_s8                     *cnr_in = NULL;
	cmr_s8                     *cnr_out = NULL;
	cmr_s8                     *ext_src = NULL;
	cmr_u32                    ext_w = 0;
	cmr_u32                    ext_h = 0;
	cmr_u32                    line_stride = 0;

	struct uv_denoise_param0  uv_params[THREAD_UVDE];
	struct uv_denoise_param0  *uv_param_ptr;

	cmr_u32                    part0_h = 0;
	cmr_u32                    part1_h = 0;
	cmr_u32                    part2_h = 0;
	cmr_u32                    part3_h = 0;
	cmr_u32                    SrcOffsetOne = 0;
	cmr_u32                    SrcOffsetTwo = 0;
	cmr_u32                    SrcOffsetThr = 0;
	cmr_u32                    SrcOffsetFour = 0;
	cmr_u32                    DstOffsetOne = 0;
	cmr_u32                    DstOffsetTwo = 0;
	cmr_u32                    DstOffsetThr = 0;
	cmr_u32                    DstOffsetFour = 0;
	cmr_u32                    denoise_level[2] = {0};
	cmr_u32                    y_denoise_level = 0;
	cmr_u32                    uv_denoise_level = 0;
	cmr_u32                    max6delta = 0;
	cmr_u32                    max4delta = 0;
	cmr_u32                    max2delta = 0;

	if (!in || !class_handle) {
		CMR_LOGE("Invalid Param!");
		return CMR_CAMERA_INVALID_PARAM;
	}

	uvdenoise_input.InputHeight = in->src_frame.size.height;
	uvdenoise_input.InputWidth = in->src_frame.size.width;
	uvdenoise_input.InputAddr = in->src_frame.addr_vir.addr_u;
	denoise_in = &uvdenoise_input;

	cnr_out = (cmr_s8 *)malloc((denoise_in->InputHeight * denoise_in->InputWidth)>>1);
	if (NULL == cnr_out) {
		CMR_LOGE("failed to allocate memory");
		goto EXIT;
	}

	cnr_in = (cmr_s8 *)denoise_in->InputAddr;

	if (0 == uv_denoise_alg) {
		ext_w = denoise_in->InputWidth / 2 + 24;
		ext_h = denoise_in->InputHeight / 2 + 24;
		line_stride = ext_w * 2;
		ext_src = (cmr_s8 *)malloc(ext_w * ext_h *2);
		if (NULL == ext_src) {
			CMR_LOGE("allocate extend buffer failed!");
			goto EXIT;
		}

		add_border_uv((cmr_u8 *)ext_src, (cmr_u8 *)cnr_in,denoise_in->InputWidth/2,denoise_in->InputHeight/2,12,12);

	}

	isp_capability(NULL, ISP_DENOISE_INFO, (void*)denoise_level);
	y_denoise_level = denoise_level[0];
	uv_denoise_level = denoise_level[1];
	if (uv_denoise_level < 9)
		uv_denoise_level = 9;
	else if (uv_denoise_level > 36)
		uv_denoise_level = 36;
	max6delta = uv_denoise_level;
	max4delta = uv_denoise_level*4/6;
	max2delta = uv_denoise_level*2/6;
	CMR_LOGI("isp_uv_denoise, uv_denoise_level=%d (%d, %d, %d)", uv_denoise_level, max6delta, max4delta, max2delta);

	part0_h = denoise_in->InputHeight / 4;
	part1_h = part0_h;
	part2_h = part0_h;
	part3_h = denoise_in->InputHeight - 3 * part0_h;

	if (0 == uv_denoise_alg) {

		SrcOffsetOne = 0;
		DstOffsetOne = 0;

		SrcOffsetTwo = part0_h / 2 * line_stride * sizeof(cmr_s8);
		DstOffsetTwo = part0_h / 2 * denoise_in->InputWidth* sizeof(cmr_s8);

		SrcOffsetThr = part0_h * line_stride * sizeof(cmr_s8);
		DstOffsetThr = part0_h * denoise_in->InputWidth* sizeof(cmr_s8);

		SrcOffsetFour = 3 * part0_h /2 * line_stride * sizeof(cmr_s8);
		DstOffsetFour = 3 * part0_h / 2 * denoise_in->InputWidth* sizeof(cmr_s8);

			/* uv denoise level*/
		uv_param_ptr = &uv_params[0];
		uv_param_ptr->dst_uv_image = (cmr_s8 *)cnr_out+DstOffsetOne;
		uv_param_ptr->src_uv_image = (cmr_s8 *)ext_src+SrcOffsetOne;
		uv_param_ptr->in_width = line_stride;
		uv_param_ptr->in_height = part0_h+48;
		uv_param_ptr->out_width = 0;
		uv_param_ptr->out_height = 0;
		uv_param_ptr->max_6_delta = max6delta;
		uv_param_ptr->max_4_delta = max4delta;
		uv_param_ptr->max_2_delta = max2delta;
		uv_param_ptr->task_no = 1;

		uv_param_ptr = &uv_params[1];
		uv_param_ptr->dst_uv_image = (cmr_s8 *)cnr_out+DstOffsetTwo;
		uv_param_ptr->src_uv_image = (cmr_s8 *)ext_src+SrcOffsetTwo;
		uv_param_ptr->in_width = line_stride;
		uv_param_ptr->in_height = part1_h+48;
		uv_param_ptr->out_width = 0;
		uv_param_ptr->out_height = 0;
		uv_param_ptr->max_6_delta = max6delta;
		uv_param_ptr->max_4_delta = max4delta;
		uv_param_ptr->max_2_delta = max2delta;
		uv_param_ptr->task_no = 2;

		uv_param_ptr = &uv_params[2];
		uv_param_ptr->dst_uv_image = (cmr_s8 *)cnr_out+DstOffsetThr;
		uv_param_ptr->src_uv_image = (cmr_s8 *)ext_src+SrcOffsetThr;
		uv_param_ptr->in_width = line_stride;
		uv_param_ptr->in_height = part2_h+48;
		uv_param_ptr->out_width = 0;
		uv_param_ptr->out_height = 0;
		uv_param_ptr->max_6_delta = max6delta;
		uv_param_ptr->max_4_delta = max4delta;
		uv_param_ptr->max_2_delta = max2delta;
		uv_param_ptr->task_no = 3;

		uv_param_ptr = &uv_params[3];
		uv_param_ptr->dst_uv_image = (cmr_s8 *)cnr_out+DstOffsetFour;
		uv_param_ptr->src_uv_image = (cmr_s8 *)ext_src+SrcOffsetFour;
		uv_param_ptr->in_width = line_stride;
		uv_param_ptr->in_height = part3_h+48;
		uv_param_ptr->out_width = 0;
		uv_param_ptr->out_height = 0;
		uv_param_ptr->max_6_delta = max6delta;
		uv_param_ptr->max_4_delta = max4delta;
		uv_param_ptr->max_2_delta = max2delta;
		uv_param_ptr->task_no = 4;

		for (thread_id = 0; thread_id < THREAD_UVDE; thread_id++) {
			uvde_start(uvde_handle,thread_id,uv_proc_func_neon0,uv_proc_cb,(void*)&uv_params[thread_id]);
		}

	} else if(1 == uv_denoise_alg) {

		SrcOffsetOne = 0;
		DstOffsetOne = 0;

		SrcOffsetTwo = (part0_h / 2 - 2) * denoise_in->InputWidth* sizeof(cmr_s8);
		DstOffsetTwo = part0_h / 2 * denoise_in->InputWidth* sizeof(cmr_s8);

		SrcOffsetThr = (part0_h - 2) * denoise_in->InputWidth* sizeof(cmr_s8);
		DstOffsetThr = part0_h * denoise_in->InputWidth* sizeof(cmr_s8);

		SrcOffsetFour = (3 * part0_h /2 - 2) * denoise_in->InputWidth* sizeof(cmr_s8);
		DstOffsetFour = 3 * part0_h / 2 * denoise_in->InputWidth* sizeof(cmr_s8);

			/* uv denoise level*/
		uv_param_ptr = &uv_params[0];
		uv_param_ptr->dst_uv_image = (cmr_s8 *)cnr_out+DstOffsetOne;
		uv_param_ptr->src_uv_image = (cmr_s8 *)cnr_in+SrcOffsetOne;
		uv_param_ptr->in_width = denoise_in->InputWidth;
		uv_param_ptr->in_height = part0_h+4;
		uv_param_ptr->out_width = 0;
		uv_param_ptr->out_height = 0;
		uv_param_ptr->max_6_delta = max6delta;
		uv_param_ptr->max_4_delta = max4delta;
		uv_param_ptr->max_2_delta = max2delta;
		uv_param_ptr->task_no = 1;

		uv_param_ptr = &uv_params[1];
		uv_param_ptr->dst_uv_image = (cmr_s8 *)cnr_out+DstOffsetTwo;
		uv_param_ptr->src_uv_image = (cmr_s8 *)cnr_in+SrcOffsetTwo;
		uv_param_ptr->in_width = denoise_in->InputWidth;
		uv_param_ptr->in_height = part1_h+8;
		uv_param_ptr->out_width = 0;
		uv_param_ptr->out_height = 0;
		uv_param_ptr->max_6_delta = max6delta;
		uv_param_ptr->max_4_delta = max4delta;
		uv_param_ptr->max_2_delta = max2delta;
		uv_param_ptr->task_no = 2;

		uv_param_ptr = &uv_params[2];
		uv_param_ptr->dst_uv_image = (cmr_s8 *)cnr_out+DstOffsetThr;
		uv_param_ptr->src_uv_image = (cmr_s8 *)cnr_in+SrcOffsetThr;
		uv_param_ptr->in_width = denoise_in->InputWidth;
		uv_param_ptr->in_height = part2_h+8;
		uv_param_ptr->out_width = 0;
		uv_param_ptr->out_height = 0;
		uv_param_ptr->max_6_delta = max6delta;
		uv_param_ptr->max_4_delta = max4delta;
		uv_param_ptr->max_2_delta = max2delta;
		uv_param_ptr->task_no = 3;

		uv_param_ptr = &uv_params[3];
		uv_param_ptr->dst_uv_image = (cmr_s8 *)cnr_out+DstOffsetFour;
		uv_param_ptr->src_uv_image = (cmr_s8 *)cnr_in+SrcOffsetFour;
		uv_param_ptr->in_width = denoise_in->InputWidth;
		uv_param_ptr->in_height = part3_h+4;
		uv_param_ptr->out_width = 0;
		uv_param_ptr->out_height = 0;
		uv_param_ptr->max_6_delta = max6delta;
		uv_param_ptr->max_4_delta = max4delta;
		uv_param_ptr->max_2_delta = max2delta;
		uv_param_ptr->task_no = 4;

		for (thread_id = 0; thread_id < THREAD_UVDE; thread_id++) {
			uvde_start(uvde_handle,thread_id,uv_proc_func_neon1,uv_proc_cb,(void*)&uv_params[thread_id]);
		}

	} else if (2 == uv_denoise_alg) {

		SrcOffsetOne = 0;
		DstOffsetOne = 0;

		SrcOffsetTwo = (part0_h / 2 - 2) * denoise_in->InputWidth* sizeof(cmr_s8);
		DstOffsetTwo = part0_h / 2 * denoise_in->InputWidth* sizeof(cmr_s8);

		SrcOffsetThr = (part0_h - 2) * denoise_in->InputWidth* sizeof(cmr_s8);
		DstOffsetThr = part0_h * denoise_in->InputWidth* sizeof(cmr_s8);

		SrcOffsetFour = (3 * part0_h /2 - 2) * denoise_in->InputWidth* sizeof(cmr_s8);
		DstOffsetFour = 3 * part0_h / 2 * denoise_in->InputWidth* sizeof(cmr_s8);

			/* uv denoise level*/
		uv_param_ptr = &uv_params[0];
		uv_param_ptr->dst_uv_image = (cmr_s8 *)cnr_out+DstOffsetOne;
		uv_param_ptr->src_uv_image = (cmr_s8 *)cnr_in+SrcOffsetOne;
		uv_param_ptr->in_width = denoise_in->InputWidth;
		uv_param_ptr->in_height = part0_h+4;
		uv_param_ptr->out_width = 0;
		uv_param_ptr->out_height = 0;
		uv_param_ptr->max_6_delta = max6delta;
		uv_param_ptr->max_4_delta = max4delta;
		uv_param_ptr->max_2_delta = max2delta;
		uv_param_ptr->task_no = 1;

		uv_param_ptr = &uv_params[1];
		uv_param_ptr->dst_uv_image = (cmr_s8 *)cnr_out+DstOffsetTwo;
		uv_param_ptr->src_uv_image = (cmr_s8 *)cnr_in+SrcOffsetTwo;
		uv_param_ptr->in_width = denoise_in->InputWidth;
		uv_param_ptr->in_height = part1_h+8;
		uv_param_ptr->out_width = 0;
		uv_param_ptr->out_height = 0;
		uv_param_ptr->max_6_delta = max6delta;
		uv_param_ptr->max_4_delta = max4delta;
		uv_param_ptr->max_2_delta = max2delta;
		uv_param_ptr->task_no = 2;

		uv_param_ptr = &uv_params[2];
		uv_param_ptr->dst_uv_image = (cmr_s8 *)cnr_out+DstOffsetThr;
		uv_param_ptr->src_uv_image = (cmr_s8 *)cnr_in+SrcOffsetThr;
		uv_param_ptr->in_width = denoise_in->InputWidth;
		uv_param_ptr->in_height = part2_h+8;
		uv_param_ptr->out_width = 0;
		uv_param_ptr->out_height = 0;
		uv_param_ptr->max_6_delta = max6delta;
		uv_param_ptr->max_4_delta = max4delta;
		uv_param_ptr->max_2_delta = max2delta;
		uv_param_ptr->task_no = 3;

		uv_param_ptr = &uv_params[3];
		uv_param_ptr->dst_uv_image = (cmr_s8 *)cnr_out+DstOffsetFour;
		uv_param_ptr->src_uv_image = (cmr_s8 *)cnr_in+SrcOffsetFour;
		uv_param_ptr->in_width = denoise_in->InputWidth;
		uv_param_ptr->in_height = part3_h+4;
		uv_param_ptr->out_width = 0;
		uv_param_ptr->out_height = 0;
		uv_param_ptr->max_6_delta = max6delta;
		uv_param_ptr->max_4_delta = max4delta;
		uv_param_ptr->max_2_delta = max2delta;
		uv_param_ptr->task_no = 4;

		for (thread_id = 0; thread_id < THREAD_UVDE; thread_id++) {
			uvde_start(uvde_handle,thread_id,uv_proc_func_neon2,uv_proc_cb,(void*)&uv_params[thread_id]);
		}
	} else {
		goto EXIT;
	}

	sem_wait(&uvde_handle->denoise_sem_lock);
	sem_wait(&uvde_handle->denoise_sem_lock);
	sem_wait(&uvde_handle->denoise_sem_lock);
	sem_wait(&uvde_handle->denoise_sem_lock);
	memcpy((void*)denoise_in->InputAddr, (void*)cnr_out,
				(denoise_in->InputHeight*denoise_in->InputWidth)>>1);

EXIT:
	if(cnr_out != NULL) {
		free(cnr_out);
		cnr_out = NULL;
	}

	if(0 == uv_denoise_alg) {
		if (NULL != ext_src)
		{
			free(ext_src);
			ext_src = NULL;
		}
		CMR_LOGI("[uv_denoise] uv_denoise_alg0: X!\n");
	}

	return ret;
}

static cmr_int uvde_pre_proc(cmr_handle class_handle)
{
	UNUSED(class_handle);

	cmr_int                      ret = CMR_CAMERA_SUCCESS;

	/*no need to do*/

	return ret;
}
static cmr_int uvde_post_proc(cmr_handle class_handle)
{
	UNUSED(class_handle);

	cmr_int                      ret = CMR_CAMERA_SUCCESS;

	/*no need to do*/

	return ret;
}

static cmr_int uvde_thread_create(struct class_uvde *class_handle)
{
	cmr_int                     ret = CMR_CAMERA_SUCCESS;
	struct class_uvde          *uvde_handle  = (struct class_uvde *)class_handle;
	cmr_handle                  *cur_handle_ptr = NULL;
	cmr_int                     thread_id = 0;

	CHECK_HANDLE_VALID(uvde_handle);
	CMR_MSG_INIT(message);

	if (!class_handle->is_inited) {
		for (thread_id = 0; thread_id < THREAD_UVDE; thread_id++) {
			cur_handle_ptr = &uvde_handle->thread_handles[thread_id];
			ret = cmr_thread_create(cur_handle_ptr,
						CAMERA_UVDE_MSG_QUEUE_SIZE,
						uvde_thread_proc,
						(void*)class_handle);
			if (ret) {
				CMR_LOGE("send msg failed!");
				ret = CMR_CAMERA_FAIL;
				return ret;
			}
			message.sync_flag = CMR_MSG_SYNC_PROCESSED;
			message.msg_type   = CMR_EVT_UVDE_INIT;
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

static cmr_int uvde_thread_destroy(struct class_uvde *class_handle)
{
	cmr_int                     ret = CMR_CAMERA_SUCCESS;
	struct class_uvde          *uvde_handle = (struct class_uvde *)class_handle;
	cmr_uint                    thread_id = 0;

	CHECK_HANDLE_VALID(class_handle);

	if (uvde_handle->is_inited) {
		for (thread_id = 0; thread_id < THREAD_UVDE; thread_id++){
			if (uvde_handle->thread_handles[thread_id]) {
				ret = cmr_thread_destroy(uvde_handle->thread_handles[thread_id]);
				if (ret) {
					CMR_LOGE("uvde cmr_thread_destroy fail");
					return CMR_CAMERA_FAIL;
				}
				uvde_handle->thread_handles[thread_id] = 0;
			}
		}
		uvde_handle->is_inited = 0;
	}

	return ret;
}

static cmr_int uvde_start(cmr_handle class_handle, cmr_uint thread_id, proc_func func_ptr, proc_cb cb, void *param)
{
	cmr_int                     ret         = CMR_CAMERA_SUCCESS;
	struct class_uvde          *uvde_handle  = (struct class_uvde *)class_handle;
	cmr_handle                  *cur_handle_ptr = NULL;
	struct uvde_start_param    *uvde_start_ptr = NULL;
	struct uv_denoise_param0   *uvde_param;

	CMR_MSG_INIT(message);

	if (!class_handle) {
		CMR_LOGE("parameter is NULL. fail");
		return CMR_CAMERA_INVALID_PARAM;
	}

	uvde_start_ptr = (struct uvde_start_param *)malloc(sizeof(struct uvde_start_param));
	if (!uvde_start_ptr) {
		CMR_LOGE("no mem");
		return CMR_CAMERA_NO_MEM;
	}
	memset(uvde_start_ptr, 0, sizeof(struct uvde_start_param));
	cur_handle_ptr = &uvde_handle->thread_handles[thread_id];
	uvde_start_ptr->func_ptr = func_ptr;
	uvde_start_ptr->param = param;
	uvde_start_ptr->cb = cb;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.msg_type   = CMR_EVT_UVDE_START;
	message.alloc_flag = 1;
	message.data 	   = uvde_start_ptr;
	ret = cmr_thread_msg_send(*cur_handle_ptr, &message);
	if (ret) {
		CMR_LOGE("send msg fail");
		if (uvde_start_ptr)
			free(uvde_start_ptr);
		ret = CMR_CAMERA_FAIL;
	}

	return ret;
}

static cmr_int uvde_thread_proc(struct cmr_msg *message, void *private_data)
{
	cmr_int                   ret           = CMR_CAMERA_SUCCESS;
	struct class_uvde        *class_handle = (struct class_uvde *)private_data;
	cmr_u32                   evt = 0;
	struct uvde_start_param  *uvde_start_ptr = NULL;


	if (!message || !class_handle) {
		CMR_LOGE("parameter is fail");
		return CMR_CAMERA_INVALID_PARAM;
	}

	evt = (cmr_u32)message->msg_type;

	switch (evt) {
	case CMR_EVT_UVDE_INIT:

		break;

	case CMR_EVT_UVDE_START:
		uvde_start_ptr = (struct uvde_start_param *)message->data;
		if (uvde_start_ptr->func_ptr && (NULL != uvde_start_ptr->param)) {
			ret = uvde_start_ptr->func_ptr(uvde_start_ptr->param);
			if (uvde_start_ptr->cb) {
				uvde_start_ptr->cb(class_handle, (void *)uvde_start_ptr->param);
			}
		}
		break;

	case CMR_EVT_UVDE_EXIT:

		break;

	default:
		break;
	}

	return ret;
}

static void uv_proc_cb(cmr_handle class_handle, void* param)
{
	UNUSED(param);

	struct class_uvde          *uvde_handle  = (struct class_uvde *)class_handle;
	CMR_LOGI("uv_proc_cb called! release sem lock");
	sem_post(&uvde_handle->denoise_sem_lock);
}

void add_border_uv(cmr_u8 *dst, cmr_u8 *src, cmr_u32 w, cmr_u32 h, cmr_u32 border_w, cmr_u32 border_h)
{
	cmr_u32                     i, j;
	cmr_u8                      *src_ptr;
	cmr_u8                      *dst_ptr;
	cmr_u32                     dst_w = w + border_w * 2;
	cmr_u32                     dst_h = h + border_h * 2;
	cmr_u32                     dst_stride = dst_w * PIXEL_STRIDE;
	cmr_u32                     src_stride = w * PIXEL_STRIDE;

	src_ptr = src;
	dst_ptr = dst + dst_stride * border_h;
	for (i=0; i<h; i++) {
		src_ptr += border_w * PIXEL_STRIDE;
		for (j=0; j<border_w; j++) {
			*dst_ptr = *src_ptr;				//u
			*(dst_ptr + 1) = *(src_ptr + 1);		//v

			src_ptr -= PIXEL_STRIDE;
			dst_ptr += PIXEL_STRIDE;
		}

		memcpy(dst_ptr, src_ptr, src_stride);
		dst_ptr += src_stride;
		src_ptr += src_stride;

		src_ptr -= 2 * PIXEL_STRIDE;
		for (j=0; j<border_w; j++) {
			*dst_ptr = *src_ptr;				//u
			*(dst_ptr + 1) = *(src_ptr + 1);		//v

			src_ptr -= PIXEL_STRIDE;
			dst_ptr += PIXEL_STRIDE;
		}

		src_ptr += (border_w + 2) * PIXEL_STRIDE;
	}

	src_ptr = dst + dst_stride * (border_h + 1);
	dst_ptr = dst + dst_stride * (border_h - 1);
	for (i=0; i<border_h; i++) {
		memcpy(dst_ptr, src_ptr, dst_stride);
		src_ptr += dst_stride;
		dst_ptr -= dst_stride;
	}

	src_ptr = dst + dst_stride * (dst_h - border_h - 2);
	dst_ptr = dst + dst_stride * (dst_h - border_h);
	for (i=0; i<border_h; i++) {
		memcpy(dst_ptr, src_ptr, dst_stride);
		src_ptr -= dst_stride;
		dst_ptr += dst_stride;
	}
}
#endif
