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

#define LOG_TAG "awb_sprd_ctrl"

#include "awb_ctrl.h"
#include "awb_sprd_ctrl.h"
#include "isp_awb.h"
#include "awb_packet.h"
#include "isp_com.h"
#include "ae_misc.h"
#include "lib_ctrl.h"
#include <cutils/properties.h>

/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------*
*					Micro Define				*
*-------------------------------------------------------------------------------*/
#define AWB_CTRL_MAGIC_BEGIN		0xe5a55e5a
#define AWB_CTRL_MAGIC_END		0x5e5ae5a5
#define AWB_CTRL_RESOLUTION_NUM 	8
#define AWB_CTRL_MWB_NUM		20
#define AWB_CTRL_SCENEMODE_NUM	10
#define AWBV_WEIGHT_UNIT 256

#define     UNUSED(param)  (void)(param)

#define AWB_CTRL_TRUE			1
#define AWB_CTRL_FALSE			0
#define AWB_CTRL_WORK_MODE_NUM		(ISP_MODE_ID_VIDEO_3+1)
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
struct awb_ctrl_tuning_param {
	/**/
	uint32_t enable;
	/*window size of statistic image*/
	struct awb_ctrl_size stat_win_size;
	/*start position of statistic area*/
	struct awb_ctrl_pos stat_start_pos;
	/*compensate gain for each resolution*/
	struct awb_ctrl_gain compensate_gain[AWB_CTRL_RESOLUTION_NUM];
	/*gain for each manual white balance*/
	struct awb_ctrl_gain mwb_gain[AWB_CTRL_MWB_NUM];
	/*gain for each scenemode gain*/
	struct awb_ctrl_gain scene_gain[AWB_CTRL_SCENEMODE_NUM];
	/*bv value range for awb*/
	struct awb_ctrl_bv bv_range;
	/*init gain and ct*/
	struct awb_ctrl_gain init_gain;
	uint32_t init_ct;
	/*algorithm param*/
	void *alg_param;
	/*algorithm param size*/
	uint32_t alg_param_size;
};

struct awb_ctrl_cxt {
	/*must be the first one*/
	uint32_t magic_begin;
	/*awb status lock*/
	pthread_mutex_t status_lock;
	/*initialize parameter*/
	struct awb_ctrl_init_param init_param;
	/*tuning parameter for each work mode*/
	struct awb_ctrl_tuning_param tuning_param[AWB_CTRL_WORK_MODE_NUM];
	/*whether initialized*/
	uint32_t init;
	/*camera id*/
	uint32_t camera_id; /* 0: back camera, 1: front camera */
	void* lsc_otp_random;
	void* lsc_otp_golden;
	uint32_t lsc_otp_width;
	uint32_t lsc_otp_height;
	/*work mode*/
	uint32_t work_mode;   /* 0: preview, 1:capture, 2:video */
	uint32_t param_index; /* tuning param index*/
	/*white balance mode: auto or manual*/
	enum awb_ctrl_wb_mode wb_mode;
	/*scene mode*/
	enum awb_ctrl_scene_mode scene_mode;
	/*format of statistic image*/
	enum awb_ctrl_stat_img_format stat_img_format;
	/*statistic image size*/
	struct awb_ctrl_size stat_img_size;
	/*previous gain*/
	struct awb_ctrl_gain prv_gain;
	/*flash info*/
	struct awb_flash_info flash_info;
	/*previous ct*/
	uint32_t prv_ct;
	/*current gain*/
	struct awb_ctrl_gain cur_gain;
	/*output gain*/
	struct awb_ctrl_gain output_gain;
	/*output ct*/
	uint32_t output_ct;
	/*recover gain*/
	struct awb_ctrl_gain recover_gain;
	/*recover ct*/
	uint32_t recover_ct;
	/*recover awb mode*/
	enum awb_ctrl_wb_mode recover_mode;
	/*awb lock info */
	struct awb_ctrl_lock_info lock_info;
	/*current ct*/
	uint32_t cur_ct;
	/*whether to update awb gain*/
	uint32_t update;
	/*algorithm handle*/
	void *alg_handle;
	/*statistic image buffer*/
	void *stat_img_buf;
	/*statistic image buffer size*/
	uint32_t stat_img_buf_size;

	/*must be the last one*/
	uint32_t magic_end;
};

/*------------------------------------------------------------------------------*
*					local function declaration		*
*-------------------------------------------------------------------------------*/

static uint32_t _awb_get_gain(struct awb_ctrl_cxt *cxt, void *param);
/*------------------------------------------------------------------------------*
*					local variable				*
*-------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------*
*					local functions				*
*-------------------------------------------------------------------------------*/
static uint32_t _awb_log_level()
{
	char value[PROPERTY_VALUE_MAX] = {0};
	uint32_t log_level = 0;

	property_get("debug.camera.isp.awb", value, "0");

	if (!strcmp(value, "1")) {
		log_level = 1;
	}

	return log_level;
}

static int32_t _calc_awb_bv_section(struct awb_ctrl_range bv_range[], uint32_t num,
	 			int32_t bv, struct awb_ctrl_weight*result)
{
	int32_t rtn = AWB_CTRL_SUCCESS;
	uint16_t bv_distance[2] ={0, 1};
	int32_t bv_max = 0;
	int32_t bv_min = 0;
	uint32_t i = 0;
	int32_t bv_cur = 0;

//	if (num < 1)
//		return AWB_CTRL_ERROR;

	if (bv <= bv_range[0].max) {
		result->value[0] = 0;
		result->value[1] = 0;
	} else if (bv >= bv_range[num - 1].min) {
		result->value[0] = num - 1;
		result->value[1] = num - 1;
	} else {

		rtn = AWB_CTRL_ERROR;

		for (i=0; i<num-1; i++) {

			if (bv >= bv_range[i].min && bv <= bv_range[i].max) {
				result->value[0] = i;
				result->value[1] = i;
				rtn = AWB_CTRL_SUCCESS;
				break;

			} else if (bv > bv_range[i].max && bv < bv_range[i+1].min) {

				/*mixed environment*/
				bv_distance[0] = bv - bv_range[i].max;
				bv_distance[1] = bv_range[i+1].min - bv;
				result->value[0] = i;
				result->value[1] = i + 1;
				rtn = AWB_CTRL_SUCCESS;
				break;
			}
		}
	}

	if (AWB_CTRL_SUCCESS == rtn) {
		/*calc weight value for mixed environment*/
		result->weight[0] = bv_distance[1] * AWBV_WEIGHT_UNIT
						/ (bv_distance[0] + bv_distance[1]);
		result->weight[1] = AWBV_WEIGHT_UNIT - result->weight[0];
	}

	AWB_CTRL_LOGE("debug envi = [%d, %d], weight=[%d, %d]", result->value[0], result->value[1],  result->weight[0], result->weight[1]);

	return rtn;
}

static uint32_t _alg_deinit(void *alg_handle)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;

	rtn = awb_deinit(alg_handle, NULL, NULL);
	if (0 != rtn){
		rtn = AWB_CTRL_ERROR;
		return rtn;
	}

	return rtn;
}

static void* _alg_init(struct awb_ctrl_tuning_param *tuning_param, uint32_t base_gain, struct awb_ctrl_init_param *ctr_init_param)
{
	uint32_t rtn = 0;
	void *alg_handle = NULL;
	uint32_t i = 0;
	uint32_t j = 0;

	struct awb_init_param init_param = {0};

	init_param.stat_img_size.w = ctr_init_param->stat_img_size.w;
	init_param.stat_img_size.h = ctr_init_param->stat_img_size.h;
	init_param.scalar_factor = (ctr_init_param->stat_win_size.w / 2) * (ctr_init_param->stat_win_size.h / 2);
	init_param.base_gain = base_gain;
	init_param.opt_info.gldn_stat_info.b = ctr_init_param->otp_info.gldn_stat_info.b;
	init_param.opt_info.gldn_stat_info.g = ctr_init_param->otp_info.gldn_stat_info.g;
	init_param.opt_info.gldn_stat_info.r = ctr_init_param->otp_info.gldn_stat_info.r;
	init_param.opt_info.rdm_stat_info.b = ctr_init_param->otp_info.rdm_stat_info.b;
	init_param.opt_info.rdm_stat_info.g = ctr_init_param->otp_info.rdm_stat_info.g;
	init_param.opt_info.rdm_stat_info.r = ctr_init_param->otp_info.rdm_stat_info.r;
	init_param.tuning_param = tuning_param->alg_param;
	init_param.param_size = tuning_param->alg_param_size;
	init_param.log_level = _awb_log_level();

	alg_handle = awb_init((struct awb_init_param *)&init_param, NULL);
	if (0 == alg_handle) {
		AWB_CTRL_LOGE("awb init failed, rtn=%d", rtn);
		alg_handle = NULL;
	}

	return alg_handle;
}

static uint32_t _alg_calc(void *alg_handle, struct awb_ctrl_calc_param *param,
					struct awb_ctrl_calc_result *result)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;

	/*convert data format*/
	struct awb_stat_img stat_img = {0};
	struct awb_calc_param calc_param = {0};
	struct awb_calc_result calc_result;
	struct isp_ae_info *ae_info = NULL;
	memset(&calc_result, 0, sizeof(struct awb_calc_result));

	ae_info = &param->ae_info;
	calc_param.stat_img = &stat_img;

	//Stats pointer each plane
	calc_param.stat_img->r_info = param->stat_img.chn_img.r;
	calc_param.stat_img->g_info = param->stat_img.chn_img.g;
	calc_param.stat_img->b_info = param->stat_img.chn_img.b;

	calc_param.envi_id[0] = param->envi_info[0].envi_id;
	calc_param.envi_id[1] = param->envi_info[1].envi_id;

	calc_param.envi_weight[0] = param->envi_info[0].weight;
	calc_param.envi_weight[1] = param->envi_info[1].weight;

	calc_param.quick_mode = param->quick_mode;

	rtn = awb_calculation(alg_handle, &calc_param, &calc_result);
	if (0 == rtn) {
		result->gain.r = calc_result.gain.r;
		result->gain.g = calc_result.gain.g;
		result->gain.b = calc_result.gain.b;
		result->ct = calc_result.ct;
	}

	return rtn;
}

static uint32_t _alg_ioctrl(void *alg_handle, enum awb_ctrl_cmd cmd,
				void *param0, void *param1)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	UNUSED(alg_handle);
	UNUSED(cmd);
	UNUSED(param0);
	UNUSED(param1);
	return rtn;
}

static uint32_t _check_handle(awb_ctrl_handle_t handle)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	struct awb_ctrl_cxt *cxt = (struct awb_ctrl_cxt *)handle;

	if (NULL == cxt) {
		AWB_CTRL_LOGE("invalid cxt pointer");
		return AWB_CTRL_ERROR;
	}

	if (AWB_CTRL_MAGIC_BEGIN != cxt->magic_begin
		|| AWB_CTRL_MAGIC_END != cxt->magic_end) {
		AWB_CTRL_LOGE("invalid magic begin = 0x%x, magic end = 0x%x",
					cxt->magic_begin, cxt->magic_end);
		return AWB_CTRL_ERROR;
	}

	return rtn;
}

static uint32_t _check_init_param(struct awb_ctrl_init_param *param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	UNUSED(param);
	return rtn;
}

static uint32_t _parse_tuning_param(void *param, uint32_t param_size, struct awb_ctrl_tuning_param *result)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t i=0;

	struct awb_param_tuning tuning_param;
	memset(&tuning_param, 0, sizeof(struct awb_param_tuning));

	if (NULL == param || 0 == param_size) {
		AWB_CTRL_LOGE("invalid param: param=%p, param_size=%d", param, param_size);
		return AWB_CTRL_ERROR;
	}

	{
		rtn = awb_param_unpack(param, param_size, &tuning_param);
		if (0 != rtn)
			return AWB_CTRL_ERROR;

		result->enable = 1;
		result->stat_win_size.w = tuning_param.common.stat_win_size.w;
		result->stat_win_size.h = tuning_param.common.stat_win_size.h;
		result->stat_start_pos.x = tuning_param.common.stat_start_pos.x;
		result->stat_start_pos.y = tuning_param.common.stat_start_pos.y;

		for (i=0; i < AWB_CTRL_RESOLUTION_NUM; ++i){
			result->compensate_gain[i].r = tuning_param.common.compensate_gain[i].r;
			result->compensate_gain[i].g = tuning_param.common.compensate_gain[i].g;
			result->compensate_gain[i].b = tuning_param.common.compensate_gain[i].b;
		}

		for (i=0; i< AWB_CTRL_MWB_NUM; ++i){
			result->mwb_gain[i].r = tuning_param.common.mwb_gain[i].r;
			result->mwb_gain[i].g = tuning_param.common.mwb_gain[i].g;
			result->mwb_gain[i].b = tuning_param.common.mwb_gain[i].b;
		}

		for (i=0; i< AWB_CTRL_SCENEMODE_NUM; ++i){
			result->scene_gain[i].r = tuning_param.common.scene_gain[i].r;
			result->scene_gain[i].g = tuning_param.common.scene_gain[i].g;
			result->scene_gain[i].b = tuning_param.common.scene_gain[i].b;
		}

		result->bv_range.num = tuning_param.common.bv_range.num;
		result->bv_range.enable = tuning_param.common.bv_range.enable;

		for (i=0; i< AWB_CTRL_ENVI_NUM; ++i){
			result->bv_range.bv_range[i].min = tuning_param.common.bv_range.bv_range[i].min;
			result->bv_range.bv_range[i].max = tuning_param.common.bv_range.bv_range[i].max;
		}

		result->init_gain.r = tuning_param.common.init_gain.r;
		result->init_gain.g = tuning_param.common.init_gain.g;
		result->init_gain.b = tuning_param.common.init_gain.b;
		result->init_ct = tuning_param.common.init_ct;
		result->alg_param = tuning_param.alg_param;
		result->alg_param_size = tuning_param.alg_param_size;
	}

	return rtn;
}

static uint32_t _deinit(struct awb_ctrl_cxt *cxt)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;

	if (AWB_CTRL_TRUE != cxt->init) {
		AWB_CTRL_LOGE("AWB do not init!");
		return AWB_CTRL_ERROR;
	}
	/*deinit awb algorithm*/
	rtn = _alg_deinit(cxt->alg_handle);
	/*clear buffer*/
	memset(cxt, 0, sizeof(*cxt));

	return rtn;
}

static uint32_t _init(struct awb_ctrl_cxt *cxt, struct awb_ctrl_init_param *param,
					struct awb_ctrl_init_result *result)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t base_gain = param->base_gain;

	rtn = _check_init_param(param);
	if (AWB_CTRL_SUCCESS != rtn) {
		AWB_CTRL_LOGE("_check_init_param failed");
		return AWB_CTRL_ERROR;
	}

	memset(cxt, 0, sizeof(*cxt));

	cxt->init_param = *param;
	/*parameter parser*/
	rtn = _parse_tuning_param(cxt->init_param.tuning_param, cxt->init_param.param_size,
								&cxt->tuning_param[0]);
	if (AWB_CTRL_SUCCESS != rtn) {
		AWB_CTRL_LOGE("_parse_tuning_param failed");
		return AWB_CTRL_ERROR;
	}

	cxt->work_mode = 0;
	cxt->param_index = 0;
	cxt->camera_id = param->camera_id;

	result->gain.r = cxt->tuning_param[0].init_gain.r;
	result->gain.g = cxt->tuning_param[0].init_gain.g;
	result->gain.b = cxt->tuning_param[0].init_gain.b;
	result->ct = cxt->tuning_param[0].init_ct;
	/*initialize algorithm*/
	cxt->alg_handle = _alg_init(&cxt->tuning_param[0], base_gain, param);

	if (NULL != cxt->alg_handle) {
		cxt->init = AWB_CTRL_TRUE;
		cxt->magic_begin = AWB_CTRL_MAGIC_BEGIN;
		cxt->magic_end = AWB_CTRL_MAGIC_END;
		rtn = AWB_CTRL_SUCCESS;
	} else {
		rtn = AWB_CTRL_ERROR;
	}

	return rtn;
}

static uint32_t _calc(struct awb_ctrl_cxt *cxt, struct awb_ctrl_calc_param *param,
					struct awb_ctrl_calc_result *result)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	void *alg_handle = NULL;
	uint32_t mawb_id = cxt->wb_mode;
	uint32_t scene_mode = 0;
	uint32_t work_mode = cxt->work_mode;
	uint32_t param_index = cxt->param_index;
	struct awb_ctrl_weight bv_result = {{0}, {0}};
	struct awb_ctrl_bv bv_param = {0};

	if (AWB_CTRL_TRUE != cxt->init) {
		AWB_CTRL_LOGE("AWB do not init!");
		return AWB_CTRL_ERROR;
	}

	if (param_index >= AWB_CTRL_WORK_MODE_NUM) {
		param_index = 0;
	}
	alg_handle = cxt->alg_handle;
	bv_param = cxt->tuning_param[param_index].bv_range;
	AWB_CTRL_LOGE("bv_value = %d, bv_num = %d", param->bv, bv_param.num);
	rtn = _calc_awb_bv_section(bv_param.bv_range, bv_param.num, param->bv, &bv_result);
	if (AWB_CTRL_SUCCESS!= rtn) {
		return rtn;
	}

	if (1 == bv_param.enable) {
		param->envi_info[0].envi_id = bv_result.value[0]+1;
		param->envi_info[0].weight = bv_result.weight[0];
		param->envi_info[1].envi_id = bv_result.value[1]+1;
		param->envi_info[1].weight = bv_result.weight[1];
	} else {
		param->envi_info[0].envi_id = AWB_CTRL_ENVI_COMMON;
		param->envi_info[0].weight = 256;
		param->envi_info[1].envi_id = AWB_CTRL_ENVI_COMMON;
		param->envi_info[1].envi_id = 0;
	}

	AWB_CTRL_LOGE("lsc random = %p, golden = %p, %d x %d", cxt->lsc_otp_random, cxt->lsc_otp_golden, cxt->lsc_otp_width, cxt->lsc_otp_height);
	AWB_CTRL_LOGE("camera_id = %d, work_mode = %d, flash_mode = %d, flash_effect = %d, ev_index = %d", cxt->camera_id, cxt->work_mode, cxt->flash_info.flash_mode, cxt->flash_info.effect, param->ae_info.ev_index);
	if (AWB_CTRL_CMD_FLASH_OPEN_M == cxt->flash_info.flash_mode)
		param->quick_mode = 1;
	AWB_CTRL_LOGE("envi = [%d, %d], weight=[%d, %d]", param->envi_info[0].envi_id, param->envi_info[1].envi_id, param->envi_info[0].weight, param->envi_info[1].weight);
	AWB_CTRL_LOGE("awb param index %d", cxt->param_index);
	rtn = _alg_calc(alg_handle, param, result);

	cxt->cur_gain.r = result->gain.r;
	cxt->cur_gain.g = result->gain.g;
	cxt->cur_gain.b = result->gain.b;
	cxt->cur_ct = result->ct;

	cxt->output_gain.r = cxt->cur_gain.r;
	cxt->output_gain.g = cxt->cur_gain.g;
	cxt->output_gain.b = cxt->cur_gain.b;
	cxt->output_ct = cxt->cur_ct;

	//scenemode & mwb change
	if(AWB_CTRL_SCENEMODE_AUTO == cxt->scene_mode) {
		if(AWB_CTRL_WB_MODE_AUTO != cxt->wb_mode) {

			cxt->output_gain.r = cxt->tuning_param[param_index].mwb_gain[mawb_id].r;
			cxt->output_gain.g = cxt->tuning_param[param_index].mwb_gain[mawb_id].g;
			cxt->output_gain.b = cxt->tuning_param[param_index].mwb_gain[mawb_id].b;
		}
	} else {
		scene_mode = cxt->scene_mode;
		if(AWB_CTRL_SCENEMODE_USER_0 ==scene_mode) {
		cxt->output_gain.r = cxt->tuning_param[param_index].scene_gain[scene_mode].r;
		cxt->output_gain.g = cxt->tuning_param[param_index].scene_gain[scene_mode].g;
		cxt->output_gain.b = cxt->tuning_param[param_index].scene_gain[scene_mode].b;
		}
	}
	//flash change awb gain
	if ((AWB_CTRL_WB_MODE_AUTO == cxt->wb_mode) && (AWB_CTRL_SCENEMODE_AUTO == cxt->scene_mode)) {
		if (AWB_CTRL_FLASH_MAIN == cxt->flash_info.flash_mode) {
			if(0 == cxt->flash_info.patten) {
				//alpha padding
				if (cxt->flash_info.flash_ratio.r > 0 && cxt->flash_info.flash_ratio.g > 0 && cxt->flash_info.flash_ratio.b > 0) {
					cxt->output_gain.r=((cxt->output_gain.r*(1024-cxt->flash_info.effect))+cxt->flash_info.flash_ratio.r*cxt->flash_info.effect)>>0x0a;
					cxt->output_gain.g=((cxt->output_gain.g*(1024-cxt->flash_info.effect))+cxt->flash_info.flash_ratio.g*cxt->flash_info.effect)>>0x0a;
					cxt->output_gain.b=((cxt->output_gain.b*(1024-cxt->flash_info.effect))+cxt->flash_info.flash_ratio.b*cxt->flash_info.effect)>>0x0a;
				}
			}
		}
	}
	//lock mode
	if(AWB_CTRL_LOCKMODE == cxt->lock_info.lock_mode) {
		cxt->output_gain.r = cxt->lock_info.lock_gain.r;
		cxt->output_gain.g = cxt->lock_info.lock_gain.g;
		cxt->output_gain.b = cxt->lock_info.lock_gain.b;
		cxt->output_ct = cxt->lock_info.lock_ct;
	}

	result->gain.r = cxt->output_gain.r;
	result->gain.g = cxt->output_gain.g;
	result->gain.b = cxt->output_gain.b;
	result->ct = cxt->output_ct;

	return rtn;
}

static uint32_t _awb_get_param_index(struct awb_ctrl_cxt *cxt, uint32_t work_mode)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t i = 0;

	if (!cxt) {
		AWB_CTRL_LOGE("cxt is NULL");
		return AWB_CTRL_ERROR;
	}
	if (work_mode > 2) {
		AWB_CTRL_LOGE("work_mode %d is over range", work_mode);
		return AWB_CTRL_ERROR;
	}

	AWB_CTRL_LOGI("awb work mode is %d", work_mode);
	switch (work_mode) {
	case 0:
		for (i = ISP_MODE_ID_PRV_0 ; i < ISP_MODE_ID_PRV_3 ; i++) {
			if (1 == cxt->tuning_param[i].enable) {
				cxt->param_index = i;
				break;
			}
		}
		break;
	case 1:
		for (i = ISP_MODE_ID_CAP_0 ; i < ISP_MODE_ID_CAP_3 ; i++) {
			if (1 == cxt->tuning_param[i].enable) {
				cxt->param_index = i;
				break;
			}
		}
		break;
	case 2:
		for (i = ISP_MODE_ID_VIDEO_0 ; i < ISP_MODE_ID_VIDEO_3 ; i++) {
			if (1 == cxt->tuning_param[i].enable) {
				cxt->param_index = i;
				break;
			}
		}
		break;
	default:
		break;
	}
	AWB_CTRL_LOGI("awb param index is %d", cxt->param_index);
	return rtn;
}

static uint32_t _awb_set_wbmode(struct awb_ctrl_cxt *cxt, void *in_param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t awb_mode = *(uint32_t*)in_param;

	cxt->wb_mode = awb_mode;
	AWB_CTRL_LOGE("debug wbmode changed!");
	return rtn;
}

static uint32_t _awb_set_workmode(struct awb_ctrl_cxt *cxt, void *in_param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t work_mode = *(uint32_t*)in_param;

	cxt->work_mode = work_mode;
	_awb_get_param_index(cxt, work_mode);

	return rtn;
}

static uint32_t _awb_set_flashratio(struct awb_ctrl_cxt *cxt, void *in_param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t flash_ratio = *(uint32_t*)in_param;

	cxt->flash_info.effect = flash_ratio;

	return rtn;
}

static uint32_t _awb_set_scenemode(struct awb_ctrl_cxt *cxt, void *in_param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t scene_mode = *(uint32_t*)in_param;

	cxt->scene_mode = scene_mode;
	return rtn;
}

static uint32_t _awb_set_recgain(struct awb_ctrl_cxt *cxt, void *param)
{
	UNUSED(param);
	uint32_t rtn = AWB_CTRL_SUCCESS;
	struct awb_gain awb_gain = {0x0};

	rtn = _awb_get_gain(cxt, (void *)&awb_gain);

	cxt->recover_gain.r = awb_gain.r;
	cxt->recover_gain.g = awb_gain.g;
	cxt->recover_gain.b = awb_gain.b;

	cxt->recover_mode = cxt->wb_mode;
	cxt->recover_ct = cxt->cur_ct;

	//cxt->flash_info.flash_mode = AWB_CTRL_FLASH_PRE;
	AWB_CTRL_LOGE("pre flashing mode = %d", cxt->flash_info.flash_mode);

	AWB_CTRL_LOGE("FLASH_TAG: awb flash recover gain = (%d, %d, %d), recover mode = %d", cxt->recover_gain.r, cxt->recover_gain.g, cxt->recover_gain.b, cxt->recover_mode);

	return rtn;

}

static uint32_t _awb_get_gain(struct awb_ctrl_cxt *cxt, void *param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t mawb_id = cxt->wb_mode;
	struct awb_gain *awb_result = (struct awb_gain*)param;

	awb_result->r = cxt->output_gain.r;
	awb_result->g = cxt->output_gain.g;
	awb_result->b = cxt->output_gain.b;

	AWB_CTRL_LOGE("_awb_get_gain = (%d,%d,%d)",awb_result->r,awb_result->g,awb_result->b);


	return rtn;

}

static uint32_t _awb_get_stat_size(struct awb_ctrl_cxt *cxt, void *param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	struct awb_size *stat_size = (struct awb_size*)param;

	stat_size->w = cxt->init_param.stat_img_size.w;
	stat_size->h = cxt->init_param.stat_img_size.h;

	AWB_CTRL_LOGE("_awb_get_stat_size = (%d,%d)",stat_size->w, stat_size->h);

	return rtn;
}

static uint32_t _awb_get_winsize(struct awb_ctrl_cxt *cxt, void *param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t workmode = cxt->work_mode;
	struct awb_size *win_size = (struct awb_size*)param;
	uint32_t param_index = cxt->param_index;

	if (param_index >= AWB_CTRL_WORK_MODE_NUM) {
		param_index = 0;
	}
	win_size->w = cxt->tuning_param[param_index].stat_win_size.w;
	win_size->h = cxt->tuning_param[param_index].stat_win_size.h;

	AWB_CTRL_LOGE("_awb_get_winsize = (%d,%d), param_index=%d",win_size->w, win_size->h, param_index);

	return rtn;
}

static uint32_t _awb_get_ct(struct awb_ctrl_cxt *cxt, void *param)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	uint32_t *ct = (uint32_t *)param;

	*ct = cxt->output_ct;

	AWB_CTRL_LOGE("_awb_get_ct = %d", cxt->output_ct);

	return rtn;
}

static uint32_t _awb_get_recgain(struct awb_ctrl_cxt *cxt, void *param)
{
	UNUSED(param);
	uint32_t rtn = AWB_CTRL_SUCCESS;
	struct awb_ctrl_gain awb_gain = {0x0};

	awb_gain.r = cxt->recover_gain.r;
	awb_gain.g = cxt->recover_gain.g;
	awb_gain.b = cxt->recover_gain.b;


	cxt->cur_gain.r = awb_gain.r;
	cxt->cur_gain.g = awb_gain.g;
	cxt->cur_gain.b = awb_gain.b;
	cxt->cur_ct = cxt->recover_ct;
	cxt->wb_mode = cxt->recover_mode;

	//cxt->flash_info.flash_mode = AWB_CTRL_FLASH_END;

	AWB_CTRL_LOGE("after flashing mode = %d", cxt->flash_info.flash_mode);

	//AWB_CTRL_LOGE("awb flash end  gain = (%d, %d, %d), recover mode = %d", cxt->cur_gain.r, cxt->cur_gain.g, cxt->cur_gain.b, cxt->wb_mode);
	AWB_CTRL_LOGE("FLASH_TAG: awb flash end  gain = (%d, %d, %d), recover mode = %d", cxt->cur_gain.r, cxt->cur_gain.g, cxt->cur_gain.b, cxt->wb_mode);

	return rtn;

}

static uint32_t _awb_set_flash_gain(struct awb_ctrl_cxt *cxt, void *param)
{
	UNUSED(param);
	uint32_t rtn = AWB_CTRL_SUCCESS;
	struct awb_flash_info*flash_info = (struct awb_flash_info*)param;

	//cxt->flash_info.flash_mode = AWB_CTRL_FLASH_MAIN;

	cxt->flash_info.patten = 0;
	cxt->flash_info.effect = flash_info->effect;
	cxt->flash_info.flash_ratio.r = flash_info->flash_ratio.r;
	cxt->flash_info.flash_ratio.g = flash_info->flash_ratio.g;
	cxt->flash_info.flash_ratio.b = flash_info->flash_ratio.b;

	//AWB_CTRL_LOGE("flashing mode = %d", cxt->flash_info.flash_mode);
	AWB_CTRL_LOGE("FLASH_TAG: flashing mode = %d", cxt->flash_info.flash_mode);

	return rtn;

}

static uint32_t _awb_set_lock(struct awb_ctrl_cxt *cxt, void *param)
#if 1
{
	UNUSED(param);
	uint32_t rtn = AWB_CTRL_SUCCESS;

	cxt->lock_info.lock_num += 1;

	AWB_CTRL_LOGE("AWB_TEST _awb_set_lock0: luck=%d, mode=%d", cxt->lock_info.lock_num, cxt->lock_info.lock_mode);

	if (0 != cxt->lock_info.lock_num) {

		cxt->lock_info.lock_mode = AWB_CTRL_LOCKMODE;

		cxt->lock_info.lock_gain.r = cxt->output_gain.r;
		cxt->lock_info.lock_gain.g = cxt->output_gain.g;
		cxt->lock_info.lock_gain.b = cxt->output_gain.b;

		cxt->lock_info.lock_ct = cxt->output_ct;
	}
	AWB_CTRL_LOGE("AWB_TEST _awb_set_lock1: luck=%d, mode:%d", cxt->lock_info.lock_num, cxt->lock_info.lock_mode);
	return rtn;

}

#else
{
	uint32_t rtn = AWB_CTRL_SUCCESS;

	cxt->lock_info.lock_num += 1;

	AWB_CTRL_LOGE("AWB_TEST _awb_set_lock0: luck=%d, unluck=%", cxt->lock_info.lock_num, cxt->lock_info.unlock_num);

	if (0 == cxt->lock_info.lock_gain.r && 0 == cxt->lock_info.lock_gain.g && 0 == cxt->lock_info.lock_gain.b) {

		cxt->lock_info.lock_gain.r = cxt->output_gain.r;
		cxt->lock_info.lock_gain.g = cxt->output_gain.g;
		cxt->lock_info.lock_gain.b = cxt->output_gain.b;

		cxt->lock_info.lock_ct = cxt->output_ct;
	}
	AWB_CTRL_LOGE("AWB_TEST _awb_set_lock1: luck=%d, unluck=%, mode:%d", cxt->lock_info.lock_num, cxt->lock_info.unlock_num, cxt->lock_info.lock_mode);
	return rtn;

}
#endif


static uint32_t _awb_get_unlock(struct awb_ctrl_cxt *cxt, void *param)
#if 1
{
	UNUSED(param);
	uint32_t rtn = AWB_CTRL_SUCCESS;
	AWB_CTRL_LOGE("AWB_TEST _awb_get_unlock0: lock_num=%d, mode:=%d", cxt->lock_info.lock_num, cxt->lock_info.lock_mode);
	if (0 != cxt->lock_info.lock_num) {
		cxt->lock_info.lock_num -= 1;
	}

	if(0 == cxt->lock_info.lock_num) {
		cxt->lock_info.lock_mode = AWB_CTRL_UNLOCKMODE;
	}

	AWB_CTRL_LOGE("AWB_TEST _awb_get_unlock1: lock_num=%d, mode:=%d", cxt->lock_info.lock_num, cxt->lock_info.lock_mode);

	return rtn;
}
#else
{
	uint32_t rtn = AWB_CTRL_SUCCESS;

	cxt->lock_info.unlock_num += 1;
	if (0 == cxt->lock_info.lock_num)
		rtn = AWB_CTRL_ERROR;

	AWB_CTRL_LOGE("AWB_TEST _awb_get_unlock0: luck=%d, unluck=%d", cxt->lock_info.lock_num, cxt->lock_info.unlock_num);

	if(0 != cxt->lock_info.lock_num && cxt->lock_info.lock_num != cxt->lock_info.unlock_num)
		cxt->lock_info.lock_mode = AWB_CTRL_LOCKMODE;

	AWB_CTRL_LOGE("AWB_TEST _awb_get_unlock1: luck=%d, unluck=%, mode:=%d", cxt->lock_info.lock_num, cxt->lock_info.unlock_num, cxt->lock_info.lock_mode);

	return rtn;

}
#endif

static uint32_t _awb_set_flash_status(struct awb_ctrl_cxt *cxt, void *param)
{
	UNUSED(param);
	uint32_t rtn = AWB_CTRL_SUCCESS;
	enum awb_ctrl_flash_status *flash_status = (enum awb_ctrl_flash_status*)param;

	cxt->flash_info.flash_status = *flash_status;

	AWB_CTRL_LOGE("flashing status = %d", cxt->flash_info.flash_status);

	return rtn;
	
}

static uint32_t _awb_set_tuning_param(struct awb_ctrl_cxt *cxt,void *param0)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;

	rtn = _check_init_param(param0);
	if (AWB_CTRL_SUCCESS != rtn) {
		AWB_CTRL_LOGE("_check_init_param failed");
		return AWB_CTRL_ERROR;
	}

	struct awb_data_info *data_info = (struct awb_data_info *)param0;
	rtn = _parse_tuning_param(data_info->data_ptr, data_info->data_size,
								&cxt->tuning_param[0]);
	if (AWB_CTRL_SUCCESS != rtn) {
		AWB_CTRL_LOGE("_parse_tuning_param failed");
		return AWB_CTRL_ERROR;
	}

	return rtn;
}



/*------------------------------------------------------------------------------*
*					public functions			*
*-------------------------------------------------------------------------------*/
/* awb_dummy_ctrl_init--
*@ handle: instance
*@ param: input param
*@ result: output result
*@ return:
*@           AWB_CTRL_INVALID_HANDLE: failed
*@	     others: awb ctrl handle
*/
awb_ctrl_handle_t awb_dummy_ctrl_init(struct awb_ctrl_init_param *param,
				struct awb_ctrl_init_result *result)
{
	struct awb_ctrl_cxt *cxt = NULL;

	return (awb_ctrl_handle_t)cxt;
}

/* awb_dummy_ctrl_deinit--
*@ handle: instance
*@ param: input param
*@ result: output result
*@ return:
*@           0: successful
*@	     others: failed
*/
uint32_t awb_dummy_ctrl_deinit(awb_ctrl_handle_t handle, void *param, void *result)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	return rtn;
}

/* awb_dummy_ctrl_calculation--
*@ handle: instance
*@ param: input param
*@ result: output result
*@ return:
*@           0: successful
*@	     others: failed
*/
uint32_t awb_dummy_ctrl_calculation(awb_ctrl_handle_t handle,
				struct awb_ctrl_calc_param *param,
				struct awb_ctrl_calc_result *result)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	return rtn;
}

/* awb_dummy_ctrl_ioctrl--
*@ handle: instance
*@ param: input param
*@ result: output result
*@ return:
*@           0: successful
*@	     others: failed
*/
uint32_t awb_dummy_ctrl_ioctrl(awb_ctrl_handle_t handle, enum awb_ctrl_cmd cmd,
				void *param0, void *param1)
{
	uint32_t rtn = AWB_CTRL_SUCCESS;
	return rtn;
}

extern struct awb_lib_fun awb_lib_fun;
void dummy_awb_fun_init()
{
	awb_lib_fun.awb_ctrl_init 		= awb_dummy_ctrl_init;
	awb_lib_fun.awb_ctrl_deinit		= awb_dummy_ctrl_deinit;
	awb_lib_fun.awb_ctrl_calculation	= awb_dummy_ctrl_calculation;
	awb_lib_fun.awb_ctrl_ioctrl		= awb_dummy_ctrl_ioctrl;

	return;
}
