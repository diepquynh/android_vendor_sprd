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
#include "isp_smart_lsc.h"
#include "isp_com.h"
#include "isp_alg.h"
#include "isp_ae_ctrl.h"
#include "lsc_adv.h"
#include "isp_log.h"
#include "sensor_drv_u.h"

//#define ISP_ADV_LSC_ENABLE

#ifdef WIN32
#define ISP_LOGE
#define ISP_LOGW
#define ISP_LOGI
#define ISP_LOGD
#define ISP_LOGV
#else
#define SMART_LSC_DEBUG_STR     "ISP_SMART_LSC: %d, %s: "
#define SMART_LSC_DEBUG_ARGS    __LINE__,__FUNCTION__

#define ISP_LOGE(format,...) ALOGE(SMART_LSC_DEBUG_STR format, SMART_LSC_DEBUG_ARGS, ##__VA_ARGS__)
#define ISP_LOGW(format,...) ALOGW(SMART_LSC_DEBUG_STR format, SMART_LSC_DEBUG_ARGS, ##__VA_ARGS__)
#define ISP_LOGI(format,...) ALOGI(SMART_LSC_DEBUG_STR format, SMART_LSC_DEBUG_ARGS, ##__VA_ARGS__)
#define ISP_LOGD(format,...) ALOGD(SMART_LSC_DEBUG_STR format, SMART_LSC_DEBUG_ARGS, ##__VA_ARGS__)
#define ISP_LOGV(format,...) ALOGV(SMART_LSC_DEBUG_STR format, SMART_LSC_DEBUG_ARGS, ##__VA_ARGS__)
#endif



void isp_smart_lsc_set_param(struct lsc_adv_init_param *lsc_param)
{
	ISP_LOGE("isp_smart_lsc_set_param\n");
	/*	alg_open  */
	/*	0: front_camera close, back_camera close;	*/
	/*	1: front_camera open, back_camera open;	*/
	/*	2: front_camera close, back_camera open;	*/
	lsc_param->alg_open = 2;
    	lsc_param->tune_param.enable = 1;
	lsc_param->tune_param.alg_id = 0;
	//common
	lsc_param->tune_param.strength_level = 6;
	lsc_param->tune_param.pa = 1;
	lsc_param->tune_param.pb = 1;
	lsc_param->tune_param.fft_core_id = 0;
	lsc_param->tune_param.con_weight = 8;//[1~16] double tables avg; 100: 10 tables avg
	lsc_param->tune_param.restore_open = 0;
	lsc_param->tune_param.freq = 1;
}

int32_t isp_smart_lsc_init(uint32_t handler_id)
{
	uint32_t rtn = ISP_SUCCESS;
//#ifdef ISP_ADV_LSC_ENABLE
	lsc_adv_handle_t lsc_adv_handle = NULL;
	struct lsc_adv_init_param lsc_param = {0};
	struct isp_context* isp_cxt_ptr=ispGetContext(handler_id);
	struct isp_lnc_param *lsc_ptr = &isp_cxt_ptr->lnc;
	uint32_t isp_id=IspGetId();
	uint32_t size_index = 0;
	uint32_t width = isp_cxt_ptr->src.w;
	uint32_t height = isp_cxt_ptr->src.h;
	struct isp_lnc_map *lnc_tab = NULL;
	struct isp_awb_adjust *cur_lnc = NULL;
	uint32_t lnc_grid = 0;
	int strength_level = 4;
	ISP_LOGI("isp_smart_lsc_init");

	if (NULL == isp_cxt_ptr) {
		rtn = ISP_ERROR;
		ISP_LOGE("handle is NULL\n");
		return rtn;
	}
	size_index = isp_cxt_ptr->param_index - ISP_ONE;
	lnc_tab = isp_cxt_ptr->lnc_map_tab[size_index];
	cur_lnc = &isp_cxt_ptr->lnc.cur_lnc;
	lnc_grid = lnc_tab[cur_lnc->index0].grid_pitch;

	/** global parameters **/
	lsc_param.gain_width = _ispGetLensGridPitch(width, lnc_grid, isp_id);		//gain tab size
	lsc_param.gain_height = _ispGetLensGridPitch(height, lnc_grid, isp_id);	//gain tab size
	lsc_param.lum_gain = (uint16_t *)lsc_ptr->lnc_ptr;						//gain table address
	switch (isp_cxt_ptr->cfg.data.format_pattern) {							//bayer pattern
	case SENSOR_IMAGE_PATTERN_RAWRGB_GR:
		lsc_param.gain_pattern = LSC_GAIN_PATTERN_RGGB;
		break;

	case SENSOR_IMAGE_PATTERN_RAWRGB_R:
		lsc_param.gain_pattern = LSC_GAIN_PATTERN_GRBG;
		break;

	case SENSOR_IMAGE_PATTERN_RAWRGB_B:
		lsc_param.gain_pattern = LSC_GAIN_PATTERN_GBRG;
		break;

	case SENSOR_IMAGE_PATTERN_RAWRGB_GB:
		lsc_param.gain_pattern = LSC_GAIN_PATTERN_BGGR;
		break;

	default:
		return ISP_ERROR;
	}

	/**lsc alg parameters **/
	isp_smart_lsc_set_param(&lsc_param);					// set alg1 parameters

	if (NULL == isp_cxt_ptr->handle_lsc_adv) {
		lsc_adv_handle = lsc_adv_init(&lsc_param);
		if (NULL == lsc_adv_handle) {
			ISP_LOGE("lsc adv init failed");
			return ISP_ERROR;
		}
		isp_cxt_ptr->handle_lsc_adv = lsc_adv_handle;
		ISP_LOGI("LSC_ADV: handle=%p, %d, %d, %d, %d, ver %s", lsc_adv_handle, lsc_param.gain_width, lsc_param.gain_height, lsc_param.gain_pattern, lnc_grid, lsc_adv_get_ver_str(lsc_adv_handle));
	}
//#endif

	return rtn;
}

int32_t isp_smart_lsc_calc(uint32_t handler_id)
{
	int32_t rtn = ISP_SUCCESS;

#ifdef ISP_ADV_LSC_ENABLE
	ISP_LOGI("YE: isp smart lsc calc!\n");
	struct isp_context* isp_cxt_ptr=ispGetContext(handler_id);
	uint32_t isp_id=IspGetId();
	lsc_adv_handle_t lsc_adv_handle = (lsc_adv_handle_t)isp_cxt_ptr->handle_lsc_adv;
	struct isp_awb_statistic_info *stat_info = &isp_cxt_ptr->awb_stat;
	struct isp_lnc_param *lsc_ptr = &isp_cxt_ptr->lnc;
	struct isp_awb_param *awb_param_ptr = &isp_cxt_ptr->awb;
	uint32_t size_index = 0;
	uint32_t width = isp_cxt_ptr->src.w;
	uint32_t height = isp_cxt_ptr->src.h;
	struct isp_lnc_map *lnc_tab = NULL;
	struct isp_awb_adjust *cur_lnc = NULL;
	uint32_t lnc_grid = 0;

	struct lsc_adv_calc_param calc_param;
	struct lsc_adv_calc_result calc_result = {0};
	uint32_t is_ae_stab=0;

	size_index = isp_cxt_ptr->param_index - ISP_ONE;
	lnc_tab = isp_cxt_ptr->lnc_map_tab[size_index];
	cur_lnc = &isp_cxt_ptr->lnc.cur_lnc;
	lnc_grid = lnc_tab[cur_lnc->index0].grid_pitch;

	if (NULL == isp_cxt_ptr) {
		rtn = ISP_ERROR;
		ISP_LOGE("handle is NULL\n");
		return rtn;
	}

	if (NULL == lsc_adv_handle) {
		ISP_LOGE("lsc handle is NULL\n");
		return ISP_ERROR;
	}

	memset(&calc_param, 0, sizeof(calc_param));
	calc_param.stat_img.r  = stat_info->r_info;
	calc_param.stat_img.gr = stat_info->g_info;
	calc_param.stat_img.gb = stat_info->g_info;
	calc_param.stat_img.b  = stat_info->b_info;
	calc_param.stat_size.w = awb_param_ptr->stat_img_size.w;
	calc_param.stat_size.h = awb_param_ptr->stat_img_size.h;
	calc_param.gain_width = _ispGetLensGridPitch(width, lnc_grid, isp_id);		//gain tab size
	calc_param.gain_height = _ispGetLensGridPitch(height, lnc_grid, isp_id);		//gain tab size
	calc_param.lum_gain = isp_cxt_ptr->gain_tmp;//(uint16_t *)lsc_ptr->lnc_ptr;
	calc_param.block_size.w = awb_param_ptr->win_size.w;
	calc_param.block_size.h = awb_param_ptr->win_size.h;
	//ct control
	calc_param.ct = awb_param_ptr->cur_ct;
	//bv control
	calc_param.bv = isp_cxt_ptr->smart_light.calc_param.bv;
	calc_result.dst_gain = (uint16_t *)lsc_ptr->lnc_ptr;
	rtn = isp_ae_get_stab(handler_id, &is_ae_stab);
	calc_param.ae_stable = is_ae_stab;
	calc_param.camera_id = isp_cxt_ptr->camera_id;
	calc_param.isp_id = ISP_1_0;


	ALOGE("calc_param.ae_stable=%d, calc_param.camera_id=%d, calc_param.isp_id=%d", calc_param.ae_stable, calc_param.camera_id, calc_param.isp_id);

	rtn = lsc_adv_calculation(lsc_adv_handle, &calc_param, &calc_result);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("lsc adv gain map calc error");
		return rtn;
	}
	ISP_LOG("lnc_ptr = 0x%x, lnc_len=%d", (uint32_t)lsc_ptr->lnc_ptr, lsc_ptr->lnc_len);
	rtn = ispSetLncParam(handler_id, (uint32_t)lsc_ptr->lnc_ptr, lsc_ptr->lnc_len);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("ispSetLncParam failed = %d", rtn);
		return ISP_ERROR;
	}
	rtn = isp_change_param(handler_id, ISP_CHANGE_LNC_RELOAD, NULL);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("isp_change_param failed = %d", rtn);
		return ISP_ERROR;
	}
#endif

	return rtn;
}

int32_t isp_smart_lsc_reload(uint32_t handler_id)
{
	int32_t rtn = ISP_SUCCESS;

#ifdef ISP_ADV_LSC_ENABLE
	struct isp_context* isp_cxt_ptr=ispGetContext(handler_id);
	uint32_t isp_id=IspGetId();
	lsc_adv_handle_t lsc_adv_handle = (lsc_adv_handle_t)isp_cxt_ptr->handle_lsc_adv;

	struct isp_awb_statistic_info *stat_info = &isp_cxt_ptr->awb_stat;
	struct isp_lnc_param *lsc_ptr = &isp_cxt_ptr->lnc;
	struct isp_awb_param *awb_param_ptr = &isp_cxt_ptr->awb;
	uint32_t size_index = 0;
	uint32_t width = isp_cxt_ptr->src.w;
	uint32_t height = isp_cxt_ptr->src.h;
	struct isp_lnc_map *lnc_tab = NULL;
	struct isp_awb_adjust *cur_lnc = NULL;
	uint32_t lnc_grid = 0;

	struct lsc_adv_calc_param calc_param;
	struct lsc_adv_calc_result calc_result = {0};

	uint32_t is_ae_stab=0;
	uint32_t state_alg0=0;
	size_index = isp_cxt_ptr->param_index - ISP_ONE;
	lnc_tab = isp_cxt_ptr->lnc_map_tab[size_index];
	cur_lnc = &isp_cxt_ptr->lnc.cur_lnc;
	lnc_grid = lnc_tab[cur_lnc->index0].grid_pitch;

	if (NULL == isp_cxt_ptr) {
		rtn = ISP_ERROR;
		ISP_LOGE("handle is NULL\n");
		return rtn;
	}

	if (NULL == lsc_adv_handle) {
		ISP_LOGE("lsc handle is NULL\n");
		return ISP_ERROR;
	}

	memset(&calc_param, 0, sizeof(calc_param));
	calc_param.stat_img.r  = stat_info->r_info;
	calc_param.stat_img.gr= stat_info->g_info;
	calc_param.stat_img.gb= stat_info->g_info;
	calc_param.stat_img.b  = stat_info->b_info;
	calc_param.stat_size.w = awb_param_ptr->stat_img_size.w;
	calc_param.stat_size.h = awb_param_ptr->stat_img_size.h;
	calc_param.gain_width = _ispGetLensGridPitch(width, lnc_grid, isp_id);
	calc_param.gain_height = _ispGetLensGridPitch(height, lnc_grid, isp_id);
	calc_param.lum_gain= isp_cxt_ptr->gain_tmp;//lsc_ptr->lnc_ptr;
	calc_param.block_size.w = awb_param_ptr->win_size.w;
	calc_param.block_size.h = awb_param_ptr->win_size.h;

	//ct control
	calc_param.ct = awb_param_ptr->cur_ct;
	//bv control
	calc_param.bv = isp_cxt_ptr->smart_light.calc_param.bv;
	calc_result.dst_gain = (uint16_t *)lsc_ptr->lnc_ptr;
	rtn = isp_ae_get_stab(handler_id, &is_ae_stab);
	calc_param.ae_stable = is_ae_stab;
	calc_param.camera_id = isp_cxt_ptr->camera_id;
	calc_param.isp_id = ISP_1_0;

	rtn = lsc_adv_calculation(lsc_adv_handle, &calc_param, &calc_result);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("lsc adv gain map calc error");
		return rtn;
	}

#endif

	return rtn;
}

int32_t isp_smart_lsc_deinit(uint32_t handler_id)
{
	int32_t rtn = ISP_SUCCESS;

//#ifdef ISP_ADV_LSC_ENABLE
	struct isp_context* isp_cxt_ptr=ispGetContext(handler_id);
	lsc_adv_handle_t lsc_adv_handle = NULL;

	if(NULL == isp_cxt_ptr) {
		rtn = ISP_ERROR;
		ISP_LOGE("handle is NULL\n");
		return rtn;
	}

	lsc_adv_handle = isp_cxt_ptr->handle_lsc_adv;
	if (NULL == lsc_adv_handle) {
		ISP_LOGE("smart lsc handle is NULL\n");
		rtn = ISP_ERROR;
		return rtn;
	}
	rtn = lsc_adv_deinit(lsc_adv_handle);
	isp_cxt_ptr->handle_lsc_adv = NULL;

	ISP_LOGI("ISP_LSC_ADV: handle=%p, rtn=%d", lsc_adv_handle, rtn);
//#endif
	return rtn;
}

