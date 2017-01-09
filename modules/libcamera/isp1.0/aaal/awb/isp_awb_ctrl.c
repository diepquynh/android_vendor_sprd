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
 #define LOG_TAG "isp_awb_ctrl"

#include "isp_com.h"
#include "isp_log.h"
#include "isp_awb.h"
#include "isp_awb_ctrl.h"
#include <utils/Timers.h>

/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------*
*					Micro Define				*
*-------------------------------------------------------------------------------*/
#ifdef WIN32
#define ISP_LOGE
#define ISP_LOGW
#define ISP_LOGI
#define ISP_LOGD
#define ISP_LOGV
#else
#define ISP_AWB_DEBUG_STR     "ISP_AWB: %d, %s: "
#define ISP_AWB_DEBUG_ARGS    __LINE__,__FUNCTION__

#undef ISP_LOGE
#define ISP_LOGE(format,...) ALOGE(ISP_AWB_DEBUG_STR format, ISP_AWB_DEBUG_ARGS, ##__VA_ARGS__)
#undef ISP_LOGW
#define ISP_LOGW(format,...) ALOGW(ISP_AWB_DEBUG_STR format, ISP_AWB_DEBUG_ARGS, ##__VA_ARGS__)
#undef ISP_LOGI
#define ISP_LOGI(format,...) ALOGI(ISP_AWB_DEBUG_STR format, ISP_AWB_DEBUG_ARGS, ##__VA_ARGS__)
#undef ISP_LOGD
#define ISP_LOGD(format,...) ALOGD(ISP_AWB_DEBUG_STR format, ISP_AWB_DEBUG_ARGS, ##__VA_ARGS__)
#undef ISP_LOGV
#define ISP_LOGV(format,...) ALOGV(ISP_AWB_DEBUG_STR format, ISP_AWB_DEBUG_ARGS, ##__VA_ARGS__)
#endif

/*------------------------------------------------------------------------------*
*					Locals				*
*-------------------------------------------------------------------------------*/
static nsecs_t s_begin_time;
static nsecs_t s_end_time;

/*------------------------------------------------------------------------------*
*				local functions					*
*-------------------------------------------------------------------------------*/
enum isp_awb_envi_id _envi_id_convert(enum smart_light_envi_id envi_id);
/*------------------------------------------------------------------------------*
*					functions				*
*-------------------------------------------------------------------------------*/
static void _timer_begin()
{
	s_begin_time = systemTime(CLOCK_MONOTONIC);
}

static uint32_t _timer_end()
{
	uint32_t time = 0;

	s_end_time = systemTime(CLOCK_MONOTONIC);
	time = (uint32_t)(((s_end_time - s_begin_time) + 1000000 / 2) /1000000);
	return time;
}

static void _init_smartlight_param(struct isp_context *context, struct isp_smart_light_param *smartlight_param)
{
	smartlight_param->init_param.lsc_init.index[0] = context->lnc.cur_lnc.index0;
	smartlight_param->init_param.lsc_init.index[1] = context->lnc.cur_lnc.index1;
	smartlight_param->init_param.lsc_init.weight[1] = (context->lnc.cur_lnc.alpha / 1024) * 256;
	smartlight_param->init_param.lsc_init.weight[0] = 256 - smartlight_param->init_param.lsc_init.weight[1];

	ISP_LOGI("lnc index0 = %d, index1 = %d, weight[0] = %d, weight[1] = %d", smartlight_param->init_param.lsc_init.index[0], smartlight_param->init_param.lsc_init.index[1], smartlight_param->init_param.lsc_init.weight[0], smartlight_param->init_param.lsc_init.weight[1]);

	smartlight_param->init_param.cmc_init.index[0] = context->cmc.cur_cmc.index0;
	smartlight_param->init_param.cmc_init.index[1] = context->cmc.cur_cmc.index1;
	smartlight_param->init_param.cmc_init.weight[1] = (context->cmc.cur_cmc.alpha / 1024) * 256;
	smartlight_param->init_param.cmc_init.weight[0] = 256- smartlight_param->init_param.cmc_init.weight[1];

	ISP_LOGI("cmc index0 = %d, index1 = %d, weight[0] = %d, weight[1] = %d", smartlight_param->init_param.cmc_init.index[0], smartlight_param->init_param.cmc_init.index[1], smartlight_param->init_param.cmc_init.weight[0], smartlight_param->init_param.cmc_init.weight[1]);

	smartlight_param->init_param.init_hue_sat.r_gain = context->cce_coef[0];
	smartlight_param->init_param.init_hue_sat.g_gain = context->cce_coef[1];
	smartlight_param->init_param.init_hue_sat.b_gain = context->cce_coef[2];

	ISP_LOGI("init_hue_sat_gain r_gain = %d, g_gain = %d, b_gain = %d", context->cce_coef[0], context->cce_coef[1], context->cce_coef[2]);

}

static void _set_smart_param(uint32_t handler_id, struct isp_awb_param *awb_param, struct smart_light_calc_result *smart_result)
{
	struct isp_awb_gain hue_sat_gain = {0x00};
	struct isp_awb_calc_result *calc_result = &awb_param->calc_result;
	uint32_t i = 0;

	awb_param->envi_id[0] = _envi_id_convert(smart_result->envi.envi_id[0]);
	awb_param->envi_id[1] = _envi_id_convert(smart_result->envi.envi_id[1]);
	awb_param->envi_weight[0] = smart_result->envi.weight[0];
	awb_param->envi_weight[1] = smart_result->envi.weight[1];

	ISP_LOGV("envi_id = (%d, %d), weight=(%d, %d)", awb_param->envi_id[0], awb_param->envi_id[1],
					awb_param->envi_weight[0], awb_param->envi_weight[1]);

	/*the lsc and cmc should set the same index now*/
	if (0 != smart_result->cmc.update || 0 != smart_result->lsc.update
		|| 0 != smart_result->denoise.update) {

		if (smart_result->lsc.index[0] != smart_result->cmc.index[0]
			|| smart_result->lsc.index[1] != smart_result->cmc.index[1]) {

			ISP_LOGI("lsc index = (%d, %d), cmc index = (%d, %d)",
				smart_result->lsc.index[0], smart_result->lsc.index[1],
				smart_result->cmc.index[0], smart_result->cmc.index[1]);
		}

		if (1 == smart_result->lsc.update || 1 == smart_result->denoise.update) {

			/* adjust lnc param */
			uint32_t weight1 = 0;
			uint32_t total_weight = 0;
			struct isp_awb_adjust lsc_adjust = {0x00};

			total_weight = smart_result->lsc.weight[0] + smart_result->lsc.weight[1];

			if (total_weight > 0) {

				weight1 = smart_result->lsc.weight[1] * 1024 / total_weight;
				lsc_adjust.alpha = weight1;
				lsc_adjust.index0 = smart_result->lsc.index[0];
				lsc_adjust.index1 = smart_result->lsc.index[1];
				lsc_adjust.dec_ratio = smart_result->denoise.lsc_dec_ratio;
				awb_param->change_param(handler_id, ISP_CHANGE_LNC, (void*)&lsc_adjust);
			}
		}

		if (1 == smart_result->cmc.update) {

			/* adjust lnc param */
			uint32_t weight1 = 0;
			uint32_t total_weight = 0;
			struct isp_awb_adjust cmc_adjust = {0x00};

			total_weight = smart_result->cmc.weight[0] + smart_result->cmc.weight[1];

			if (total_weight > 0) {

				weight1 = smart_result->cmc.weight[1] * 1024 / total_weight;
				cmc_adjust.alpha = weight1;
				cmc_adjust.index0 = smart_result->cmc.index[0];
				cmc_adjust.index1 = smart_result->cmc.index[1];
				awb_param->change_param(handler_id, ISP_CHANGE_CMC, (void*)&cmc_adjust);
			}
		}
	}

	ISP_LOGV("set smart param: hue_sat: update=%d, gain=(%d, %d, %d)",
		smart_result->hue_saturation.update, smart_result->hue_saturation.r_gain,
		smart_result->hue_saturation.r_gain, smart_result->hue_saturation.b_gain);

	/*set gain to cce module*/
	if (0 != smart_result->hue_saturation.update) {

		hue_sat_gain.r = smart_result->hue_saturation.r_gain;
		hue_sat_gain.g = smart_result->hue_saturation.g_gain;
		hue_sat_gain.b = smart_result->hue_saturation.b_gain;
	}

	awb_param->change_param(handler_id, ISP_CHANGE_CCE, (void*)&hue_sat_gain);

	if (0 != smart_result->gain.update) {
		awb_param->cur_gain.r = smart_result->gain.gain.r;
		awb_param->cur_gain.g = smart_result->gain.gain.g;
		awb_param->cur_gain.b = smart_result->gain.gain.b;
	}
}

int32_t _smart_light_calc(uint32_t handler_id, struct isp_smart_light_param *smart_light_param,
				struct isp_awb_gain *gain, uint16_t ct, int32_t bright_value, uint32_t quick_mode)
{
	int32_t rtn = SMART_LIGHT_SUCCESS;
	struct smart_light_calc_param *calc_param = &smart_light_param->calc_param;
	struct smart_light_calc_result *calc_result = &smart_light_param->calc_result;

	memset(calc_result, 0, sizeof(*calc_result));

	if (ISP_UEB == smart_light_param->init) {
		ISP_LOGE("smart have not init");
		return SMART_LIGHT_ERROR;
	}

	if (0 == smart_light_param->smart) {
		ISP_LOGE("smart disable!");
		return SMART_LIGHT_ERROR;
	}

	calc_param->bv = bright_value;
	calc_param->smart = smart_light_param->smart;
	calc_param->ct = ct;
	calc_param->gain.r = gain->r;
	calc_param->gain.g = gain->g;
	calc_param->gain.b = gain->b;
	calc_param->quick_mode = quick_mode;

	rtn = smart_light_calculation(handler_id, (void *)calc_param, (void *)calc_result);

	return rtn;
}

static void _set_init_param(struct isp_awb_param* awb_param, struct isp_awb_init_param *init_param)
{
	uint32_t i = 0;

	init_param->alg_id = awb_param->alg_id;
	init_param->base_gain = awb_param->base_gain;
	init_param->debug_level = awb_param->debug_level;
	init_param->img_size = awb_param->stat_img_size;
	init_param->win_size = awb_param->win_size;
	init_param->init_ct = awb_param->init_ct;
	init_param->init_gain = awb_param->init_gain;
	init_param->map_data = awb_param->map_data;
	init_param->steady_speed = awb_param->steady_speed;
	init_param->target_zone = awb_param->target_zone;
	init_param->ct_info = awb_param->ct_info;
	init_param->weight_of_count_func = awb_param->weight_of_count_func;

	for (i=0; i<ISP_AWB_ENVI_NUM; i++) {
		uint32_t j = 0;
		init_param->weight_of_ct_func[i] = awb_param->weight_of_ct_func[i];

		if (init_param->weight_of_ct_func[i].weight_func.num > ISP_AWB_PIECEWISE_SAMPLE_NUM)
			init_param->weight_of_ct_func[i].weight_func.num = ISP_AWB_PIECEWISE_SAMPLE_NUM;

		ISP_LOGI("weight of ct func [%d]-----------", i);
		for (j=0; j<init_param->weight_of_ct_func[i].weight_func.num; j++) {
			ISP_LOGI("[%d] = (%d, %d)", j, init_param->weight_of_ct_func[i].weight_func.samples[j].x,
					init_param->weight_of_ct_func[i].weight_func.samples[j].y);
		}
	}

	init_param->weight_of_pos_lut = awb_param->weight_of_pos_lut;
	init_param->quick_mode = awb_param->quick_mode;

	init_param->scene_factor[ISP_AWB_SCENE_GREEN] = awb_param->green_factor;
	init_param->scene_factor[ISP_AWB_SCENE_SKIN] = awb_param->skin_factor;

	memcpy(init_param->value_range, awb_param->value_range,
			sizeof(struct isp_awb_range) * ISP_AWB_ENVI_NUM);
#if 1 //ref_gain
	for (i=0; i<ISP_AWB_ENVI_NUM; i++) {
		init_param->ref_param[i] = awb_param->ref_param[i];
	}
#endif
	memcpy(init_param->win, awb_param->win,
			sizeof(struct isp_awb_coord) * ISP_AWB_TEMPERATRE_NUM);

	ISP_LOGI("alg id = %d", init_param->alg_id);
	ISP_LOGI("base_gain = %d", init_param->base_gain);
	ISP_LOGI("img_size = (%d, %d)", init_param->img_size.w, init_param->img_size.h);
	ISP_LOGI("win_size = (%d, %d)", init_param->win_size.w, init_param->win_size.h);
	ISP_LOGI("init ct = %d, init gain=(%d, %d, %d)", init_param->init_ct, init_param->init_gain.r,
				init_param->init_gain.g, init_param->init_gain.b);
	ISP_LOGI("map data = 0x%x, len=%d", (uint32_t)init_param->map_data.addr, init_param->map_data.len);
	ISP_LOGI("steady_speed = %d", init_param->steady_speed);
	ISP_LOGI("target_zone = %d", init_param->target_zone);
	ISP_LOGI("ct_info = (%d, %d, %d, %d)", init_param->ct_info.data[0], init_param->ct_info.data[1],
			init_param->ct_info.data[2], init_param->ct_info.data[3]);
	ISP_LOGI("count func.num = %d, base=%d, 0=(%d, %d), 1=(%d, %d)",
			init_param->weight_of_count_func.weight_func.num,
			init_param->weight_of_count_func.base,
			init_param->weight_of_count_func.weight_func.samples[0].x,
			init_param->weight_of_count_func.weight_func.samples[0].y,
			init_param->weight_of_count_func.weight_func.samples[1].x,
			init_param->weight_of_count_func.weight_func.samples[1].y);

	ISP_LOGI("pos lut = 0x%x, w=%d, h=%d",
			(uint32_t)init_param->weight_of_pos_lut.weight, init_param->weight_of_pos_lut.w,
			init_param->weight_of_pos_lut.h);

	for (i=0; i<ISP_AWB_ENVI_NUM; i++) {
		ISP_LOGI("[%d] = (%d, %d)", i, init_param->value_range[i].min,
				init_param->value_range[i].max);
#if 1 //ref_gain
		ISP_LOGI("[%d]: ref gain enable=%d, ct=%d, gain=(%d, %d, %d)",
					i, init_param->ref_param[i].enable, init_param->ref_param[i].ct,
					init_param->ref_param[i].gain.r, init_param->ref_param[i].gain.g,
					init_param->ref_param[i].gain.b);
#endif
	}

	ISP_LOGI("scene factor: green = %d, skin=%d", init_param->scene_factor[ISP_AWB_SCENE_GREEN],
							init_param->scene_factor[ISP_AWB_SCENE_SKIN]);
	ISP_LOGI("quick mode=%d", init_param->quick_mode);
}

enum isp_awb_envi_id _envi_id_convert(enum smart_light_envi_id envi_id)
{
	switch (envi_id) {
	case SMART_ENVI_COMMON:
		return ISP_AWB_ENVI_COMMON;

	case SMART_ENVI_INDOOR_NORMAL:
		return ISP_AWB_ENVI_INDOOR;

	case SMART_ENVI_LOW_LIGHT:
		return ISP_AWB_ENVI_LOW_LIGHT;

	case SMART_ENVI_OUTDOOR_HIGH:
		return ISP_AWB_ENVI_OUTDOOR_HIGH;

	case SMART_ENVI_OUTDOOR_MIDDLE:
		return ISP_AWB_ENVI_OUTDOOR_MIDDLE;

	case SMART_ENVI_OUTDOOR_NORMAL:
		return ISP_AWB_ENVI_OUTDOOR_LOW;

	default:
		return ISP_AWB_ENVI_COMMON;
	}
}

uint32_t _get_quick_mode(struct isp_awb_param* awb_param)
{
	if (awb_param->quick_mode_enable > 0)
		return 1;
	else
		return 0;
}

void _set_quick_mode(struct isp_awb_param* awb_param, uint32_t mode)
{
	awb_param->quick_mode_enable = mode;
}

/* isp_awb_init --
*@
*@
*@ return:
*/
uint32_t isp_awb_ctrl_init(uint32_t handler_id)
{
	uint32_t rtn=ISP_SUCCESS;

	struct isp_context* isp_cxt =ispGetAlgContext(0);
	struct isp_awb_param* awb_param=&isp_cxt->awb;
	struct isp_smart_light_param *smart_light_param = &isp_cxt->smart_light;
	struct isp_awb_init_param *init_param = &awb_param->init_param;
	uint32_t i=0x00;

	if (ISP_EB == awb_param->init) {

		ISP_LOGI("AWB_TAG: awb already init!");
		return ISP_SUCCESS;
	} else {
		ISP_LOGI("AWB_TAG: need init");
	}

	ISP_LOGI("init awb gain = (%d, %d, %d)", awb_param->cur_gain.r,
			awb_param->cur_gain.g, awb_param->cur_gain.b);

	isp_cxt->awb_get_stat=ISP_END_FLAG;
	awb_param->monitor_bypass = ISP_UEB;
	awb_param->stab_conter=ISP_ZERO;
	awb_param->alg_mode=ISP_ALG_FAST;
	awb_param->matrix_index=ISP_ZERO;
	awb_param->gain_div=0x100;
	awb_param->win_size = isp_cxt->awbm.win_size;
	awb_param->quick_mode_enable = 1;
	awb_param->flash_awb_flag = ISP_ZERO;

	ISP_LOGI("smart = 0x%d, envi=(%d, %d), weight=(%d, %d)",
			smart_light_param->smart, awb_param->envi_id[0], awb_param->envi_id[1],
			awb_param->envi_weight[0], awb_param->envi_weight[1]);

	/*disable smart for alg_id 0*/
	if (awb_param->alg_id > 0) {
		_init_smartlight_param(isp_cxt, smart_light_param);
		rtn = smart_light_init(handler_id, (void *)&smart_light_param->init_param, NULL);
		if (ISP_SUCCESS == rtn)
			smart_light_param->init = ISP_EB;
		else
			ISP_LOGE("smart_light_init failed!");
	} else {
		smart_light_param->init = ISP_UEB;
	}

	_set_init_param(awb_param, init_param);
	rtn = isp_awb_init(handler_id, init_param, NULL);
	if (ISP_SUCCESS == rtn)
		awb_param->init = ISP_EB;
	else
		ISP_LOGE("isp_awb_init failed!");

	return rtn;
}

/* isp_awb_deinit --
*@
*@
*@ return:
*/
uint32_t isp_awb_ctrl_deinit(uint32_t handler_id)
{
	uint32_t rtn=ISP_SUCCESS;

	struct isp_context* isp_cxt=ispGetAlgContext(0);
	struct isp_awb_param *awb_param = &isp_cxt->awb;
	struct isp_smart_light_param *smart_light_param = &isp_cxt->smart_light;

	if (ISP_EB == awb_param->init) {
		isp_awb_deinit(handler_id, NULL, NULL);
		awb_param->init=ISP_UEB;
	}

	if (ISP_EB == smart_light_param->init) {
		smart_light_deinit(handler_id, NULL, NULL);
		smart_light_param->init = ISP_UEB;
	}

	/*to avoid gain different after snapshot*/
	awb_param->init_ct = awb_param->cur_ct;
	awb_param->init_gain = awb_param->cur_gain;

	smart_light_param->init_param.init_gain = smart_light_param->calc_result.gain;
	smart_light_param->init_param.init_hue_sat = smart_light_param->calc_result.hue_saturation;

	ISP_LOGI("AWB_TAG: deinit awb gain = (%d, %d, %d)", awb_param->cur_gain.r,
			awb_param->cur_gain.g, awb_param->cur_gain.b);

	return rtn;
}

/* isp_awb_calculation --
*@
*@
*@ return:
*/
uint32_t isp_awb_ctrl_calculation(uint32_t handler_id)
{
	uint32_t rtn=ISP_SUCCESS;
	struct isp_context* isp_cxt = ispGetAlgContext(0);
	struct isp_ae_param *ae_param = &isp_cxt->ae;
	struct isp_awb_param* awb_param = &isp_cxt->awb;
	struct isp_smart_light_param *smart_light_param = &isp_cxt->smart_light;
	struct isp_awb_calc_param *calc_param = &awb_param->calc_param;
	struct isp_awb_calc_result *calc_result = &awb_param->calc_result;
	struct isp_awb_gain gain = {0, 0 , 0};
	uint32_t setting_index = awb_param->cur_setting_index;
	uint16_t ct = 0;
	uint32_t bv = 0;
	uint32_t time = 0;

	_timer_begin();

	if(ISP_EB==isp_cxt->awb_get_stat)
		isp_cxt->cfg.callback(handler_id, ISP_CALLBACK_EVT|ISP_AWB_STAT_CALLBACK,
					(void*)&isp_cxt->awb_stat, sizeof(struct isp_awb_statistic_info));

	if ((ISP_END_FLAG!=isp_cxt->af.continue_status)
		&&(PNULL!=awb_param->continue_focus_stat)) {
		awb_param->continue_focus_stat(handler_id, ISP_AWB_STAT_FLAG);
	}

	if(ISP_EB == awb_param->bypass)
		goto EXIT;

	if (ISP_UEB == awb_param->init) {
		ISP_LOGE("awb have not init");
		goto EXIT;
	}

	calc_param->awb_stat = &isp_cxt->awb_stat;
	if (ISP_ONE == awb_param->flash_awb_flag) {
		calc_param->quick_mode = ISP_ONE;
	} else {
		calc_param->quick_mode = _get_quick_mode(awb_param);
	}
	calc_param->envi_id[0] = awb_param->envi_id[0];
	calc_param->envi_id[1] = awb_param->envi_id[1];
	calc_param->envi_weight[0] = awb_param->envi_weight[0];
	calc_param->envi_weight[1] = awb_param->envi_weight[1];

	rtn = isp_awb_calculation(handler_id, calc_param, calc_result);
	gain = calc_result->gain;
	ct = calc_result->ct;

	ISP_LOGI("gain = (%d, %d, %d), ct=%d, rtn=%d", gain.r, gain.g, gain.b, ct, rtn);

	/* update ccm and lsc parameters */
	if (ISP_SUCCESS == rtn && gain.r > 0 && gain.b > 0 && gain.g > 0) {

		struct isp_awb_gain last_gain = awb_param->cur_gain;
		int32_t bright_value = 0;

		if (ISP_AWB_AUTO == awb_param->work_mode) {
			awb_param->cur_gain = gain;
		} else {
			awb_param->cur_gain.r = isp_cxt->awb_r_gain[ awb_param->work_mode];
			awb_param->cur_gain.g = isp_cxt->awb_g_gain[ awb_param->work_mode];
			awb_param->cur_gain.b = isp_cxt->awb_b_gain[ awb_param->work_mode];
		}
		awb_param->cur_ct = ct;
		ISP_LOGI("work_mode=%d", awb_param->work_mode);
		ISP_LOGV("smart init=%d", smart_light_param->init);

		if ((ISP_EB == smart_light_param->init) && (ISP_AWB_AUTO == awb_param->work_mode)) {

			bright_value = awb_param->get_ev_lux(handler_id);
			rtn = _smart_light_calc(handler_id, smart_light_param, &gain, ct,
							bright_value, calc_param->quick_mode);
			if (ISP_SUCCESS == rtn)
				_set_smart_param(handler_id, awb_param, &smart_light_param->calc_result);
			else
				ISP_LOGE("smart calc failed!");
		}

		if(awb_param->cur_gain.r != last_gain.r || awb_param->cur_gain.g != last_gain.g
			|| awb_param->cur_gain.b != last_gain.b) {

			awb_param->set_eb=ISP_EB;
			awb_param->stab_conter=ISP_ZERO;

			ISP_LOGV("cur gain=(%d, %d, %d), last gain=(%d, %d, %d)",
				awb_param->cur_gain.r, awb_param->cur_gain.g, awb_param->cur_gain.b,
				last_gain.r, last_gain.g, last_gain.b);
		} else {

			awb_param->set_eb = ISP_UEB;
			awb_param->monitor_bypass = ISP_UEB;
			awb_param->stab_conter++;
		}
	}

	awb_param->quick_mode_enable = 0;

	time = _timer_end();
	ISP_LOGV("awb calc time = %d", time);
	ISP_LOG("calc awb gain = (%d, %d, %d)", awb_param->cur_gain.r,
			awb_param->cur_gain.g, awb_param->cur_gain.b);

EXIT:

	return rtn;
}

uint32_t isp_awb_ctrl_set(uint32_t handler_id, uint32_t cmd, void *param0, void *param1)
{
	struct isp_context* isp_cxt = ispGetAlgContext(0);
	struct isp_awb_param* awb_param = &isp_cxt->awb;

	ISP_LOGI("cmd = %d, param0 = %d, param1=%d", cmd, (uint32_t)param0, (uint32_t)param1);

	switch (cmd) {
	case ISP_AWB_SET_QUICK_MODE:
		if (NULL != param0)
		{
			uint32_t quick_mode = *(uint32_t *)param0;
			_set_quick_mode(awb_param, quick_mode);
			ISP_LOGI("set_quick_mode=%d", quick_mode);
		}
		break;

	default:
		break;
	}

	return ISP_SUCCESS;
}

/* isp_awb_set_flash_gain --
*@
*@
*@ return:
*/
uint32_t isp_awb_set_flash_gain(void)
{
	uint32_t rtn=ISP_SUCCESS;
	uint32_t handler_id=0x00;
	struct isp_context* isp_context_ptr=ispGetAlgContext(handler_id);
	struct isp_awbc_param* awbc_ptr=(struct isp_awbc_param*)&isp_context_ptr->awbc;
	struct isp_awb_param* awb_param_ptr=&isp_context_ptr->awb;
	struct isp_flash_param* flash_param_ptr=&isp_context_ptr->ae.flash;
	uint32_t r_gain=0x00;
	uint32_t g_gain=0x00;
	uint32_t b_gain=0x00;
	uint32_t base_gain=awb_param_ptr->GetDefaultGain(handler_id);

	ISP_LOG("hait: ratio: 0x%x, 0x%x, 0x%x \n", flash_param_ptr->r_ratio, flash_param_ptr->g_ratio, flash_param_ptr->b_ratio);
	ISP_LOG("hait: prv: 0x%x, 0x%x, 0x%x effect:%d ,set_awb:%d\n", awb_param_ptr->prv_gain.r, awb_param_ptr->prv_gain.g, awb_param_ptr->prv_gain.b
			, flash_param_ptr->effect, flash_param_ptr->set_awb);

	if((ISP_EB==flash_param_ptr->set_awb)&&(ISP_AWB_AUTO == awb_param_ptr->work_mode))
	{
		if((ISP_ZERO==flash_param_ptr->r_ratio)
			||(ISP_ZERO==flash_param_ptr->g_ratio)
			||(ISP_ZERO==flash_param_ptr->b_ratio))
		{
			ISP_LOG("ratio: 0x%x, 0x%x, 0x%x error\n", flash_param_ptr->r_ratio, flash_param_ptr->g_ratio, flash_param_ptr->b_ratio);
		}
		else
		{
			r_gain=flash_param_ptr->r_ratio;
			g_gain=flash_param_ptr->g_ratio;
			b_gain=flash_param_ptr->b_ratio;
			awb_param_ptr->cur_gain.r=((awb_param_ptr->prv_gain.r*(1024-flash_param_ptr->effect))+r_gain*flash_param_ptr->effect)>>0x0a;
			awb_param_ptr->cur_gain.g=((awb_param_ptr->prv_gain.g*(1024-flash_param_ptr->effect))+g_gain*flash_param_ptr->effect)>>0x0a;
			awb_param_ptr->cur_gain.b=((awb_param_ptr->prv_gain.b*(1024-flash_param_ptr->effect))+b_gain*flash_param_ptr->effect)>>0x0a;
			awbc_ptr->r_gain=awb_param_ptr->cur_gain.r;
			awbc_ptr->g_gain=awb_param_ptr->cur_gain.g;
			awbc_ptr->b_gain=awb_param_ptr->cur_gain.b;
		}
	}

	flash_param_ptr->set_awb=ISP_UEB;

	return rtn;
}

/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------*/
