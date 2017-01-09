/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "awb_sprd_ctrl_v1"

#include "awb1/awb.h"
#include "awb_ctrl.h"
#include "awb_sprd_ctrl_v1.h"

#include "isp_awb_queue.h"
#include "isp_debug.h"

#include <cutils/properties.h>

#include <sys/time.h>
#include <time.h>

/*------------------------------------------------------------------------------*
*					Micro Define				*
*-------------------------------------------------------------------------------*/
#define     UNUSED(param)  (void)(param)
#define AWB_CTRL_ENABLE 1
#define AWB_CTRL_LOCKMODE 1
#define AWB_CTRL_UNLOCKMODE 0
#define AWB_CTRL_SAFE_FREE(_p) \
	do { \
		if (NULL != (_p)) {\
			free(_p); \
			_p = NULL; \
		} \
	}while(0)

/*------------------------------------------------------------------------------*
*					structures				*
*-------------------------------------------------------------------------------*/
struct awb_gain_queue
{
	void* ct;
	void* r_gain;
	void* g_gain;
	void* b_gain;
	void* weight;

	uint32_t size;
};

struct awb_ctrl_cxt
{
	/*awb status lock */
	pthread_mutex_t status_lock;

	struct awb_init_param awb_init_param;

	/*work mode */
	uint32_t work_mode;			/* 0: preview, 1:capture, 2:video */

	/*white balance mode: auto or manual */
	enum awb_ctrl_wb_mode wb_mode;

	/*scene mode */
	enum awb_ctrl_scene_mode scene_mode;

	/*statistic image size */
	struct awb_ctrl_size stat_img_size;
	struct awb_ctrl_size stat_win_size;

	struct awb_ctrl_opt_info otp_info;

	/*flash info */
	struct awb_flash_info flash_info;

	/*current gain */
	struct awb_ctrl_gain cur_gain;
	/*current ct */
	uint32_t cur_ct;

	/*output gain */
	struct awb_ctrl_gain output_gain;
	/*output ct */
	uint32_t output_ct;

	/*recover gain */
	struct awb_ctrl_gain recover_gain;
	/*recover ct */
	uint32_t recover_ct;

	/*recover awb mode */
	enum awb_ctrl_wb_mode recover_mode;

	/*awb lock info */
	struct awb_ctrl_lock_info lock_info;

	/*algorithm handle */
	void *alg_handle;

	struct awb_gain_queue gain_queue;

	unsigned int frame_count;


	uint8_t* log;
	uint32_t size;
};


/*------------------------------------------------------------------------------*
*					local functions				*
*-------------------------------------------------------------------------------*/
static uint32_t _awb_log_level()
{
	char value[PROPERTY_VALUE_MAX] = { 0 };
	uint32_t log_level = 0;

	property_get("debug.camera.isp.awb", value, "0");

	if (!strcmp(value, "1"))
	{
		log_level = 1;
	}

	return log_level;
}

static void _deinit_gain_queue(struct awb_gain_queue *queue)
{
	if (0 != queue->ct) {
		queue_deinit(queue->ct);
		queue->ct = 0;
	}

	if (0 != queue->r_gain) {
		queue_deinit(queue->r_gain);
		queue->r_gain = 0;
	}

	if (0 != queue->g_gain) {
		queue_deinit(queue->g_gain);
		queue->g_gain = 0;
	}

	if (0 != queue->b_gain) {
		queue_deinit(queue->b_gain);
		queue->b_gain = 0;
	}

	if (0 != queue->weight) {
		queue_deinit(queue->weight);
		queue->weight = 0;
	}
}

static void _gain_queue_add(struct awb_gain_queue *queue, struct awb_ctrl_gain *gain, uint16_t ct, uint32_t weight)
{
	if (0 != queue->ct)
		queue_add(queue->ct, ct);

	if (0 != queue->r_gain)
		queue_add(queue->r_gain, gain->r);

	if (0 != queue->g_gain)
		queue_add(queue->g_gain, gain->g);

	if (0 != queue->b_gain)
		queue_add(queue->b_gain, gain->b);

	if (0 != queue->weight)
		queue_add(queue->weight, weight);
}

static void _gain_queue_clear(struct awb_gain_queue *queue)
{
	if (0 != queue->ct)
		queue_clear(queue->ct);

	if (0 != queue->r_gain)
		queue_clear(queue->r_gain);

	if (0 != queue->g_gain)
		queue_clear(queue->g_gain);

	if (0 != queue->b_gain)
		queue_clear(queue->b_gain);

	if (0 != queue->weight)
		queue_clear(queue->weight);
}

static int32_t _init_gain_queue(struct awb_gain_queue *queue, uint32_t size)
{
	queue->ct = queue_init(size);
	if (0 == queue->ct)
		goto ERROR_EXIT;

	queue->r_gain = queue_init(size);
	if (0 == queue->r_gain)
		goto ERROR_EXIT;

	queue->g_gain = queue_init(size);
	if (0 == queue->g_gain)
		goto ERROR_EXIT;

	queue->b_gain = queue_init(size);
	if (0 == queue->b_gain)
		goto ERROR_EXIT;

	queue->weight = queue_init(size);
	if (0 == queue->weight)
		goto ERROR_EXIT;

	return AWB_SUCCESS;

ERROR_EXIT:
	_deinit_gain_queue(queue);
	return AWB_ERROR;
}


static void _gain_queue_average(struct awb_gain_queue *queue, struct awb_ctrl_gain *gain, uint32_t *ct)
{
	if (0 == queue->weight)
		return;

	if (0 != queue->ct)
		*ct = queue_weighted_average(queue->ct, queue->weight);

	if (0 != queue->r_gain)
		gain->r = queue_weighted_average(queue->r_gain, queue->weight);

	if (0 != queue->g_gain)
		gain->g = queue_weighted_average(queue->g_gain, queue->weight);

	if (0 != queue->b_gain)
		gain->b = queue_weighted_average(queue->b_gain, queue->weight);
}


static uint32_t _awb_set_wbmode(struct awb_ctrl_cxt *cxt, void *in_param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t awb_mode = *(uint32_t *) in_param;

	cxt->wb_mode = awb_mode;

	return rtn;
}

static uint32_t _awb_set_workmode(struct awb_ctrl_cxt *cxt, void *in_param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t work_mode = *(uint32_t *) in_param;

	cxt->work_mode = work_mode;

	return rtn;
}

static uint32_t _awb_set_flashratio(struct awb_ctrl_cxt *cxt, void *in_param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t flash_ratio = *(uint32_t *) in_param;

	cxt->flash_info.effect = flash_ratio;

	return rtn;
}

static uint32_t _awb_set_scenemode(struct awb_ctrl_cxt *cxt, void *in_param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t scene_mode = *(uint32_t *) in_param;

	cxt->scene_mode = scene_mode;

	return rtn;
}

static uint32_t _awb_get_gain(struct awb_ctrl_cxt *cxt, void *param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t mawb_id = cxt->wb_mode;
	struct awb_gain *awb_result = (struct awb_gain *) param;

	awb_result->r = cxt->output_gain.r;
	awb_result->g = cxt->output_gain.g;
	awb_result->b = cxt->output_gain.b;

//	AWB_CTRL_LOGD("_awb_get_gain = (%d,%d,%d)", awb_result->r, awb_result->g, awb_result->b);

	return rtn;
}

static uint32_t _awb_set_recgain(struct awb_ctrl_cxt *cxt, void *param)
{
	UNUSED(param);
	uint32_t rtn = AWB_CTRL_SUCCESS;
	struct awb_gain awb_gain = { 0x0 };

	rtn = _awb_get_gain(cxt, (void *) &awb_gain);

	cxt->recover_gain.r = awb_gain.r;
	cxt->recover_gain.g = awb_gain.g;
	cxt->recover_gain.b = awb_gain.b;

	cxt->recover_mode = cxt->wb_mode;
	cxt->recover_ct = cxt->cur_ct;

	return rtn;
}

static uint32_t _awb_get_stat_size(struct awb_ctrl_cxt *cxt, void *param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	struct awb_size *stat_size = (struct awb_size *) param;

	stat_size->w = cxt->stat_img_size.w;
	stat_size->h = cxt->stat_img_size.h;

//	AWB_CTRL_LOGD("_awb_get_stat_size = (%d,%d)", stat_size->w, stat_size->h);

	return rtn;
}

static uint32_t _awb_get_winsize(struct awb_ctrl_cxt *cxt, void *param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t workmode = cxt->work_mode;
	struct awb_size *win_size = (struct awb_size *) param;

	win_size->w = cxt->stat_win_size.w;
	win_size->h = cxt->stat_win_size.h;

//	AWB_CTRL_LOGD("_awb_get_winsize = (%d,%d)", win_size->w, win_size->h);

	return rtn;
}

static uint32_t _awb_get_ct(struct awb_ctrl_cxt *cxt, void *param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t *ct = (uint32_t *) param;

	*ct = cxt->output_ct;

//	AWB_CTRL_LOGD("_awb_get_ct = %d", cxt->output_ct);

	return rtn;
}

static uint32_t _awb_get_recgain(struct awb_ctrl_cxt *cxt, void *param)
{
	UNUSED(param);
	uint32_t rtn = AWB_CTRL_SUCCESS;
	struct awb_ctrl_gain awb_gain = { 0x0 };

	awb_gain.r = cxt->recover_gain.r;
	awb_gain.g = cxt->recover_gain.g;
	awb_gain.b = cxt->recover_gain.b;

	cxt->output_gain.r = awb_gain.r;
	cxt->output_gain.g = awb_gain.g;
	cxt->output_gain.b = awb_gain.b;
	cxt->output_ct = cxt->recover_ct;
	cxt->wb_mode = cxt->recover_mode;

	AWB_CTRL_LOGD("FLASH_TAG: awb flash end  gain = (%d, %d, %d), recover mode = %d", cxt->cur_gain.r, cxt->cur_gain.g, cxt->cur_gain.b, cxt->wb_mode);

	return rtn;
}

static uint32_t _awb_set_flash_gain(struct awb_ctrl_cxt *cxt, void *param)
{
	UNUSED(param);
	uint32_t rtn = AWB_CTRL_SUCCESS;
	struct awb_flash_info *flash_info = (struct awb_flash_info *) param;

	//cxt->flash_info.flash_mode = AWB_CTRL_FLASH_MAIN;

	cxt->flash_info.patten = 0;
	cxt->flash_info.effect = flash_info->effect;
	cxt->flash_info.flash_ratio.r = flash_info->flash_ratio.r;
	cxt->flash_info.flash_ratio.g = flash_info->flash_ratio.g;
	cxt->flash_info.flash_ratio.b = flash_info->flash_ratio.b;

	//AWB_CTRL_LOGE("flashing mode = %d", cxt->flash_info.flash_mode);
	//flash change awb gain
	if ((AWB_CTRL_WB_MODE_AUTO == cxt->wb_mode) && (AWB_CTRL_SCENEMODE_AUTO == cxt->scene_mode)) {
		if (AWB_CTRL_FLASH_MAIN == cxt->flash_info.flash_mode) {
			if(0 == cxt->flash_info.patten) {
				//alpha padding
				AWB_CTRL_LOGE("FLASH_TAG:before flash rgb(%d,%d,%d)", cxt->recover_gain.r,
				cxt->recover_gain.g, cxt->recover_gain.b);
				if (cxt->flash_info.flash_ratio.r > 0 && cxt->flash_info.flash_ratio.g > 0 && cxt->flash_info.flash_ratio.b > 0) {
					cxt->output_gain.r=((cxt->recover_gain.r *(1024-cxt->flash_info.effect))+cxt->flash_info.flash_ratio.r*cxt->flash_info.effect)>>0x0a;
					cxt->output_gain.g=((cxt->recover_gain.g *(1024-cxt->flash_info.effect))+cxt->flash_info.flash_ratio.g*cxt->flash_info.effect)>>0x0a;
					cxt->output_gain.b=((cxt->recover_gain.b *(1024-cxt->flash_info.effect))+cxt->flash_info.flash_ratio.b*cxt->flash_info.effect)>>0x0a;
				}
			}
			AWB_CTRL_LOGE("FLASH_TAG:cap flash rgb(%d,%d,%d)", cxt->output_gain.r, cxt->output_gain.g, cxt->output_gain.b);
		}
	}

	return rtn;
}

static uint32_t _awb_set_lock(struct awb_ctrl_cxt *cxt, void *param)
{
	UNUSED(param);
	uint32_t rtn = AWB_CTRL_SUCCESS;

	cxt->lock_info.lock_num += 1;

	AWB_CTRL_LOGE("AWB_TEST _awb_set_lock0: luck=%d, mode=%d", cxt->lock_info.lock_num, cxt->lock_info.lock_mode);

	if (0 != cxt->lock_info.lock_num)
	{
		cxt->lock_info.lock_mode = AWB_CTRL_LOCKMODE;

		cxt->lock_info.lock_gain.r = cxt->output_gain.r;
		cxt->lock_info.lock_gain.g = cxt->output_gain.g;
		cxt->lock_info.lock_gain.b = cxt->output_gain.b;

		cxt->lock_info.lock_ct = cxt->output_ct;
	}

	AWB_CTRL_LOGD("AWB_TEST _awb_set_lock1: luck=%d, mode:%d", cxt->lock_info.lock_num, cxt->lock_info.lock_mode);

	return rtn;
}

static uint32_t _awb_get_unlock(struct awb_ctrl_cxt *cxt, void *param)
{
	UNUSED(param);
	uint32_t rtn = AWB_CTRL_SUCCESS;
	AWB_CTRL_LOGD("AWB_TEST _awb_get_unlock0: lock_num=%d, mode:=%d", cxt->lock_info.lock_num, cxt->lock_info.lock_mode);
	if (0 != cxt->lock_info.lock_num)
	{
		cxt->lock_info.lock_num -= 1;
	}

	if (0 == cxt->lock_info.lock_num)
	{
		cxt->lock_info.lock_mode = AWB_CTRL_UNLOCKMODE;
	}

	AWB_CTRL_LOGD("AWB_TEST _awb_get_unlock1: lock_num=%d, mode:=%d", cxt->lock_info.lock_num, cxt->lock_info.lock_mode);

	return rtn;
}

static uint32_t _awb_set_flash_status(struct awb_ctrl_cxt *cxt, void *param)
{
	UNUSED(param);
	uint32_t rtn = AWB_CTRL_SUCCESS;
	enum awb_ctrl_flash_status *flash_status = (enum awb_ctrl_flash_status *) param;

	cxt->flash_info.flash_status = *flash_status;

	AWB_CTRL_LOGD("flashing status = %d", cxt->flash_info.flash_status);

	return rtn;
}

/*------------------------------------------------------------------------------*
*					public functions			*
*-------------------------------------------------------------------------------*/
awb_ctrl_handle_t awb_sprd_ctrl_v1_init(struct awb_ctrl_init_param * param, struct awb_ctrl_init_result * result)
{
	struct awb_ctrl_cxt *cxt = NULL;
	if (NULL == param || NULL == result)
	{
		AWB_CTRL_LOGE("invalid param: param=%p, result=%p", param, result);
		goto ERROR_EXIT;
	}

	cxt = (struct awb_ctrl_cxt *) malloc(sizeof(struct awb_ctrl_cxt));
	if (NULL == cxt)
	{
		AWB_CTRL_LOGE("malloc awb_ctrl_cxt failed");
		goto ERROR_EXIT;
	}
	memset(cxt, 0, sizeof(struct awb_ctrl_cxt));

	pthread_mutex_init(&cxt->status_lock, NULL);

	cxt->stat_win_size = param->stat_win_size;
	cxt->stat_img_size = param->stat_img_size;

	cxt->awb_init_param.stat_img_w = param->stat_img_size.w;
	cxt->awb_init_param.stat_img_h = param->stat_img_size.h;
	cxt->awb_init_param.r_pix_cnt = (param->stat_win_size.w * param->stat_win_size.h) / 4;
	cxt->awb_init_param.g_pix_cnt = (param->stat_win_size.w * param->stat_win_size.h) / 4;
	cxt->awb_init_param.b_pix_cnt = (param->stat_win_size.w * param->stat_win_size.h) / 4;

        cxt->awb_init_param.otp_random_r = param->otp_info.rdm_stat_info.r;
        cxt->awb_init_param.otp_random_g = param->otp_info.rdm_stat_info.g;
        cxt->awb_init_param.otp_random_b = param->otp_info.rdm_stat_info.b;
        cxt->awb_init_param.otp_golden_r = param->otp_info.gldn_stat_info.r;
        cxt->awb_init_param.otp_golden_g = param->otp_info.gldn_stat_info.g;
        cxt->awb_init_param.otp_golden_b = param->otp_info.gldn_stat_info.b;

	memcpy(&cxt->awb_init_param.tuning_param, param->tuning_param, sizeof(cxt->awb_init_param.tuning_param));
	cxt->otp_info.gldn_stat_info.r = param->otp_info.gldn_stat_info.r;
	cxt->otp_info.gldn_stat_info.g = param->otp_info.gldn_stat_info.g;
	cxt->otp_info.gldn_stat_info.b = param->otp_info.gldn_stat_info.b;
	cxt->otp_info.rdm_stat_info.r = param->otp_info.rdm_stat_info.r;
	cxt->otp_info.rdm_stat_info.g = param->otp_info.rdm_stat_info.g;
	cxt->otp_info.rdm_stat_info.b = param->otp_info.rdm_stat_info.b;

	if (cxt->awb_init_param.tuning_param.skip_frame_num > 8)
	{
		cxt->awb_init_param.tuning_param.skip_frame_num = 0;
	}
	if ((cxt->awb_init_param.tuning_param.calc_interval_num == 0) || (cxt->awb_init_param.tuning_param.calc_interval_num > 10))
	{
		cxt->awb_init_param.tuning_param.calc_interval_num = 1;
	}
	if ((cxt->awb_init_param.tuning_param.smooth_buffer_num <= 2) || (cxt->awb_init_param.tuning_param.smooth_buffer_num > 64))
	{
		cxt->awb_init_param.tuning_param.smooth_buffer_num = 8;
	}


	struct awb_rgb_gain awb_gain;
	cxt->alg_handle = awb_init_v1(&cxt->awb_init_param, &awb_gain);

	unsigned int smooth_buffer_num = cxt->awb_init_param.tuning_param.smooth_buffer_num;
	_init_gain_queue(&cxt->gain_queue, smooth_buffer_num);

	result->gain.r = awb_gain.r_gain;
	result->gain.g = awb_gain.g_gain;
	result->gain.b = awb_gain.b_gain;
	result->ct = awb_gain.ct;

	cxt->cur_gain.r = result->gain.r;
	cxt->cur_gain.g = result->gain.g;
	cxt->cur_gain.b = result->gain.b;
	cxt->cur_ct = result->ct;

	cxt->output_gain.r = result->gain.r;
	cxt->output_gain.g = result->gain.g;
	cxt->output_gain.b = result->gain.b;
	cxt->output_ct = result->ct;

	AWB_CTRL_LOGE("AWB init: (%d,%d,%d)", cxt->output_gain.r, cxt->output_gain.g, cxt->output_gain.b);

	return (awb_ctrl_handle_t) cxt;

  ERROR_EXIT:
	AWB_CTRL_SAFE_FREE(cxt);

	return AWB_CTRL_INVALID_HANDLE;
}

uint32_t awb_sprd_ctrl_v1_deinit(awb_ctrl_handle_t handle, void *param, void *result)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;

	struct awb_ctrl_cxt *cxt = (struct awb_ctrl_cxt *) handle;

	pthread_mutex_destroy(&cxt->status_lock);

	_deinit_gain_queue(&cxt->gain_queue);

	rtn = awb_deinit_v1(cxt->alg_handle);

	AWB_CTRL_SAFE_FREE(cxt);

	return rtn;
}

uint32_t awb_sprd_ctrl_v1_calculation(awb_ctrl_handle_t handle, struct awb_ctrl_calc_param * param, struct awb_ctrl_calc_result * result)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;

	struct awb_ctrl_cxt *cxt = (struct awb_ctrl_cxt *) handle;
	if (NULL == param || NULL == result)
	{
		AWB_CTRL_LOGE("invalid param: param=%p, result=%p", param, result);
		return AWB_CTRL_ERROR;
	}

	unsigned int smooth_buffer_num = cxt->awb_init_param.tuning_param.smooth_buffer_num;
	unsigned int skip_frame_num = cxt->awb_init_param.tuning_param.skip_frame_num;
	unsigned int calc_interval_num = cxt->awb_init_param.tuning_param.calc_interval_num;

	if ((AWB_CTRL_SCENEMODE_AUTO == cxt->scene_mode) && (AWB_CTRL_WB_MODE_AUTO == cxt->wb_mode))
	{
		cxt->frame_count ++;
		if ((cxt->frame_count <= skip_frame_num) ||
			((cxt->frame_count > smooth_buffer_num) && (cxt->frame_count % calc_interval_num == 1))) // for power saving, do awb calc once every two frames
		{
			result->gain.r = cxt->output_gain.r;
			result->gain.g = cxt->output_gain.g;
			result->gain.b = cxt->output_gain.b;
			result->ct = cxt->output_ct;

			result->log_awb.log = cxt->log;
			result->log_awb.size = cxt->size;

			return rtn;
		}
	}

	pthread_mutex_lock(&cxt->status_lock);

	struct awb_calc_param calc_param;
	struct awb_calc_result calc_result;
	memset(&calc_param, 0x00, sizeof(calc_param));
	memset(&calc_result, 0x00, sizeof(calc_result));
	calc_param.stat_img.r = param->stat_img.chn_img.r;
	calc_param.stat_img.g = param->stat_img.chn_img.g;
	calc_param.stat_img.b = param->stat_img.chn_img.b;
	calc_param.bv = param->bv;
	calc_param.iso = param->ae_info.iso;
	memcpy(calc_param.matrix, param->matrix, 9*sizeof(int));
	memcpy(calc_param.gamma, param->gamma, 256);
	uint64_t time0 = systemTime(CLOCK_MONOTONIC);
	rtn = awb_calc_v1(cxt->alg_handle, &calc_param, &calc_result);
	uint64_t time1 = systemTime(CLOCK_MONOTONIC);
	AWB_CTRL_LOGE("AWB: (%d,%d,%d) %dK, pg=%d, %dus", calc_result.awb_gain[0].r_gain, calc_result.awb_gain[0].g_gain, calc_result.awb_gain[0].b_gain, calc_result.awb_gain[0].ct, result->pg_flag, (int)((time1-time0)/1000));

	result->gain.r = calc_result.awb_gain[0].r_gain;
	result->gain.g = calc_result.awb_gain[0].g_gain;
	result->gain.b = calc_result.awb_gain[0].b_gain;
	result->ct = calc_result.awb_gain[0].ct;
	result->pg_flag = calc_result.awb_gain[0].pg;
	result->green100 = calc_result.awb_gain[0].green100;
	result->log_awb.log = calc_result.log_buffer;
	result->log_awb.size = calc_result.log_size;

	cxt->cur_gain.r = result->gain.r;
	cxt->cur_gain.g = result->gain.g;
	cxt->cur_gain.b = result->gain.b;
	cxt->cur_ct = result->ct;
	cxt->log = calc_result.log_buffer;
	cxt->size = calc_result.log_size;

	_gain_queue_add(&cxt->gain_queue,&cxt->cur_gain, cxt->cur_ct, 256);
	_gain_queue_average(&cxt->gain_queue,&cxt->output_gain, &cxt->output_ct);

	AWB_CTRL_LOGE("AWB output: (%d,%d,%d) %dK", cxt->output_gain.r, cxt->output_gain.g, cxt->output_gain.b, cxt->output_ct);

	//scenemode & mwb change
	if (AWB_CTRL_SCENEMODE_AUTO == cxt->scene_mode)
	{
		uint32_t mawb_id = cxt->wb_mode;
		if (AWB_CTRL_WB_MODE_AUTO != cxt->wb_mode)
		{
			if ((mawb_id > 0) && (mawb_id < 10))	// return mwb by mwb mode id
			{
				int index = 0;

				int i;
				for (i=0; i<cxt->awb_init_param.tuning_param.wbModeNum; i++)
				{
					if (mawb_id == cxt->awb_init_param.tuning_param.wbModeId[i])
					{
						index = i;
						break;
					}
				}
				cxt->output_gain.r = cxt->awb_init_param.tuning_param.wbMode_gain[index].r_gain;
				cxt->output_gain.g = cxt->awb_init_param.tuning_param.wbMode_gain[index].g_gain;
				cxt->output_gain.b = cxt->awb_init_param.tuning_param.wbMode_gain[index].b_gain;
				cxt->output_ct = cxt->awb_init_param.tuning_param.wbMode_gain[index].ct;
			}
			else				// return mwb by ct, (100K <= ct <= 10000K)
			{
				if (mawb_id > 10000)
				{
					int index = 100;
					cxt->output_gain.r = cxt->awb_init_param.tuning_param.mwb_gain[index].r_gain;
					cxt->output_gain.g = cxt->awb_init_param.tuning_param.mwb_gain[index].g_gain;
					cxt->output_gain.b = cxt->awb_init_param.tuning_param.mwb_gain[index].b_gain;
					cxt->output_ct = cxt->awb_init_param.tuning_param.mwb_gain[index].ct;
				}
				else if (mawb_id < 100)
				{
					int index = 1;
					cxt->output_gain.r = cxt->awb_init_param.tuning_param.mwb_gain[index].r_gain;
					cxt->output_gain.g = cxt->awb_init_param.tuning_param.mwb_gain[index].g_gain;
					cxt->output_gain.b = cxt->awb_init_param.tuning_param.mwb_gain[index].b_gain;
					cxt->output_ct = cxt->awb_init_param.tuning_param.mwb_gain[index].ct;
				}
				else
				{
					unsigned int index1 = mawb_id / 100;
					unsigned int index2 = mawb_id / 100 + 1;
					unsigned int weight1 = index2 * 100 - mawb_id;
					unsigned int weight2 = mawb_id - index1 * 100;
					cxt->output_gain.r = (cxt->awb_init_param.tuning_param.mwb_gain[index1].r_gain * weight1 + cxt->awb_init_param.tuning_param.mwb_gain[index2].r_gain * weight2 + 50) / 100;
					cxt->output_gain.g = (cxt->awb_init_param.tuning_param.mwb_gain[index1].g_gain * weight1 + cxt->awb_init_param.tuning_param.mwb_gain[index2].g_gain * weight2 + 50) / 100;
					cxt->output_gain.b = (cxt->awb_init_param.tuning_param.mwb_gain[index1].b_gain * weight1 + cxt->awb_init_param.tuning_param.mwb_gain[index2].b_gain * weight2 + 50) / 100;
					cxt->output_ct = mawb_id;
				}
			}
		}
	}
	else
	{
		uint32_t scene_mode = cxt->scene_mode;
		if (AWB_CTRL_SCENEMODE_USER_0 == scene_mode)
		{
			cxt->output_gain.r = cxt->awb_init_param.tuning_param.mwb_gain[scene_mode].r_gain;
			cxt->output_gain.g = cxt->awb_init_param.tuning_param.mwb_gain[scene_mode].g_gain;
			cxt->output_gain.b = cxt->awb_init_param.tuning_param.mwb_gain[scene_mode].b_gain;
			cxt->output_ct = cxt->awb_init_param.tuning_param.mwb_gain[scene_mode].ct;
		}
	}

	//lock mode
	if (AWB_CTRL_LOCKMODE == cxt->lock_info.lock_mode)
	{
		cxt->output_gain.r = cxt->lock_info.lock_gain.r;
		cxt->output_gain.g = cxt->lock_info.lock_gain.g;
		cxt->output_gain.b = cxt->lock_info.lock_gain.b;
		cxt->output_ct = cxt->lock_info.lock_ct;
	}

	result->gain.r = cxt->output_gain.r;
	result->gain.g = cxt->output_gain.g;
	result->gain.b = cxt->output_gain.b;
	result->ct = cxt->output_ct;

	pthread_mutex_unlock(&cxt->status_lock);

	return rtn;
}


//	wanghao
static uint32_t awb_get_debug_info(struct awb_ctrl_cxt *cxt, void *result)
{
    uint32_t rtn = AWB_SUCCESS;
	struct debug_awb_param *param = (struct debug_awb_param*)result;
	param->version = AWB_DEBUG_VERSION_ID;
	param->r_gain = cxt->output_gain.r;
	param->g_gain = cxt->output_gain.g;
	param->b_gain = cxt->output_gain.b;
	param->cur_ct = cxt->output_ct;
	param->cur_awb_mode = cxt->wb_mode;
	param->cur_work_mode = cxt->work_mode;
	/* awb calibration of golden sensor */
	param->golden_r = cxt->otp_info.gldn_stat_info.r;
	param->golden_g = cxt->otp_info.gldn_stat_info.g;
	param->golden_b = cxt->otp_info.gldn_stat_info.b;
	/* awb calibration of random sensor */
	param->random_r = cxt->otp_info.rdm_stat_info.r;
	param->random_g = cxt->otp_info.rdm_stat_info.g;
	param->random_b = cxt->otp_info.rdm_stat_info.b;
	//cur_bv & cur_iso were worked out before enter this function
    return rtn;
}

static uint32_t awb_get_debug_info_for_display(struct awb_ctrl_cxt *cxt, void *result)
{
	uint32_t rtn = AWB_SUCCESS;
	struct debug_awb_display_param* emParam = (struct debug_awb_display_param*)result;
	emParam->version = AWB_DEBUG_VERSION_ID;
	emParam->r_gain = cxt->output_gain.r;
	emParam->g_gain = cxt->output_gain.g;
	emParam->b_gain = cxt->output_gain.b;
	emParam->cur_ct = cxt->output_ct;
	emParam->cur_awb_mode = cxt->wb_mode;
	emParam->cur_work_mode = cxt->work_mode;
	/* awb calibration of golden sensor */
	emParam->golden_r = cxt->otp_info.gldn_stat_info.r;
	emParam->golden_g = cxt->otp_info.gldn_stat_info.g;
	emParam->golden_b = cxt->otp_info.gldn_stat_info.b;
	/* awb calibration of random sensor */
	emParam->random_r = cxt->otp_info.rdm_stat_info.r;
	emParam->random_g = cxt->otp_info.rdm_stat_info.g;
	emParam->random_b = cxt->otp_info.rdm_stat_info.b;
	//cur_bv & cur_iso were worked out before enter this function
	return rtn;
}


uint32_t awb_sprd_ctrl_v1_ioctrl(awb_ctrl_handle_t handle, enum awb_ctrl_cmd cmd, void *param0, void *param1)
{
	UNUSED(param1);
	uint32_t rtn = AWB_CTRL_SUCCESS;
	struct awb_ctrl_cxt *cxt = (struct awb_ctrl_cxt *) handle;

	pthread_mutex_lock(&cxt->status_lock);

	switch (cmd)
	{
	case AWB_CTRL_CMD_SET_WB_MODE:
		rtn = _awb_set_wbmode(cxt, param0);
		break;

	case AWB_CTRL_CMD_SET_WORK_MODE:
		rtn = _awb_set_workmode(cxt, param0);
		break;

	case AWB_CTRL_CMD_SET_SCENE_MODE:
		rtn = _awb_set_scenemode(cxt, param0);
		break;

	case AWB_CTRL_CMD_GET_GAIN:
		rtn = _awb_get_gain(cxt, param0);
		break;

	case AWB_CTRL_CMD_FLASHING:
		AWB_CTRL_LOGE("FLASH_TAG: AWB_CTRL_CMD_FLASHING");
		rtn = _awb_set_flash_gain(cxt, param0);
		rtn = _awb_set_lock(cxt, param0);
		break;

	case AWB_CTRL_CMD_FLASH_OPEN_M:
		AWB_CTRL_LOGE("FLASH_TAG: AWB_CTRL_CMD_FLASH_OPEN_M");
		cxt->flash_info.flash_mode = AWB_CTRL_FLASH_MAIN;
		//rtn = _awb_set_recgain(cxt, param0);
		break;

	case AWB_CTRL_CMD_FLASH_OPEN_P:
		AWB_CTRL_LOGE("FLASH_TAG: AWB_CTRL_CMD_FLASH_OPEN_P");
		cxt->flash_info.flash_mode = AWB_CTRL_FLASH_PRE;
		//rtn = _awb_set_recgain(cxt, param0);
		break;

	case AWB_CTRL_CMD_FLASH_CLOSE:
		AWB_CTRL_LOGE("FLASH_TAG: AWB_CTRL_CMD_FLASH_CLOSE");
		if ((AWB_CTRL_FLASH_PRE == cxt->flash_info.flash_mode) || (AWB_CTRL_FLASH_MAIN == cxt->flash_info.flash_mode)) {
			rtn = _awb_get_recgain(cxt, param0);
		}
		if (AWB_CTRL_FLASH_MAIN == cxt->flash_info.flash_mode) {
			rtn = _awb_get_unlock(cxt, param0);
		}
		cxt->flash_info.flash_mode = AWB_CTRL_FLASH_END;
		break;

	case AWB_CTRL_CMD_FLASH_BEFORE_P:
		AWB_CTRL_LOGE("FLASH_TAG: AWB_CTRL_CMD_FLASH_BEFORE_P");
		rtn = _awb_set_recgain(cxt, param0);
		break;
	case AWB_CTRL_CMD_LOCK:
		rtn = _awb_set_lock(cxt, param0);
		break;

	case AWB_CTRL_CMD_UNLOCK:
		rtn = _awb_get_unlock(cxt, param0);
		break;

	case AWB_CTRL_CMD_GET_STAT_SIZE:
		rtn = _awb_get_stat_size(cxt, param0);
		break;

	case AWB_CTRL_CMD_GET_WIN_SIZE:
		rtn = _awb_get_winsize(cxt, param0);
		break;

	case AWB_CTRL_CMD_GET_CT:
		rtn = _awb_get_ct(cxt, param0);
		break;

	case AWB_CTRL_CMD_SET_FLASH_STATUS:
		rtn = _awb_set_flash_status(cxt,param0);
		break;

	case AWB_CTRL_CMD_GET_DEBUG_INFO:
		if (param1) {
			rtn = awb_get_debug_info(cxt,param1);
		}
		break;

	case AWB_CTRL_CMD_EM_GET_PARAM:
		rtn = awb_get_debug_info_for_display(cxt, param1);
		break;

	default:
		AWB_CTRL_LOGE("invalid cmd = 0x%x", cmd);
		rtn = AWB_CTRL_ERROR;
		break;
	}

	pthread_mutex_unlock(&cxt->status_lock);

	return rtn;
}
