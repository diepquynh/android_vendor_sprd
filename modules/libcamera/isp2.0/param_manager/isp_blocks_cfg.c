/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifdef WIN32
#include <memory.h>
#include <string.h>
#include <malloc.h>
#include "isp_log.h"
#endif
#include "isp_blocks_cfg.h"
#include "isp_type.h"
#include "isp_com.h"
#include "isp_pm_com_type.h"
#include "isp_com_alg.h"
#include "smart_ctrl.h"
#include "isp_otp_calibration.h"
#include "isp_log.h"
#include <cutils/properties.h>
#include "isp_video.h"

#define array_offset(type, member) (intptr_t)(&((type*)0)->member)

#define ISP_PM_LEVEL_DEFAULT 3
#define ISP_PM_CCE_DEFAULT 0
#define UNUSED(param) (void)(param)

static int32_t PM_CLIP(int32_t x, int32_t bottom, int32_t top)
{
	int32_t val = 0;

	if (x < bottom) {
		val = bottom;
	} else if (x > top) {
		val = top;
	} else {
		val = x;
	}

	return val;
}

static int32_t _is_print_log()
{
	char value[PROPERTY_VALUE_MAX] = {0};
	uint32_t is_print = 0;

	property_get("debug.camera.isp.pm", value, "0");

	if (!strcmp(value, "1")) {
		is_print = 1;
	}

	return is_print;
}

static isp_s32 _pm_check_smart_param(struct smart_block_result *block_result,
						struct isp_range *range,
						isp_u32 comp_num,
						isp_u32 type)
{
	isp_u32 i = 0;

	if (NULL == block_result) {
		ISP_LOGE("invalid pointer\n");
		return ISP_ERROR;
	}

	if (comp_num != block_result->component_num) {
		ISP_LOGE("component num error: %d (%d)\n", block_result->component_num, comp_num);
		return ISP_ERROR;
	}

	if (0 == block_result->update) {
		ISP_LOGE("do not need update\n");
		return ISP_ERROR;
	}

	for (i=0; i<comp_num; i++) {
		if (type != block_result->component[0].type) {
			ISP_LOGE("block type error: %d (%d)\n", block_result->component[0].type, type);
			return ISP_ERROR;
		}

		if (ISP_SMART_Y_TYPE_VALUE == type) {
			isp_s32 value = block_result->component[i].fix_data[0];

			if (value < range->min || value > range->max) {
				ISP_LOGE("value range error: %d ([%d, %d])\n",
						value, range->min, range->max);
				return ISP_ERROR;
			}
		} else if (ISP_SMART_Y_TYPE_WEIGHT_VALUE == type) {
			struct isp_weight_value *weight_value = (struct isp_weight_value *)block_result->component[i].fix_data;

			if ((isp_s32)weight_value->value[0] < range->min
				|| (isp_s32)weight_value->value[0] > range->max) {
				ISP_LOGE("value range error: %d ([%d, %d])\n",
					weight_value->value[0], range->min, range->max);
				return ISP_ERROR;
			}

			if ((isp_s32)weight_value->value[1] < range->min
				|| (isp_s32)weight_value->value[1] > range->max) {
				ISP_LOGE("value range error: %d ([%d, %d])\n",
					weight_value->value[1], range->min, range->max);
				return ISP_ERROR;
			}
		}
	}

	return ISP_SUCCESS;
}

static isp_s32 _pm_common_rest(void *blk_addr, isp_u32 size)
{
	isp_s32 rtn = ISP_SUCCESS;
	memset((void*)blk_addr, 0x00, size);
	return rtn;
}

static isp_u32 _pm_get_lens_grid_mode(isp_u32 grid)
{
	isp_u32 mode = 0x00;

	switch (grid) {
	case 16:
		mode = 0;
	break;

	case 32:
		mode = 1;
	break;

	case 64:
		mode = 2;
	break;

	default:
		ISP_LOGE("error:no lens grid");
	break;
	}

	return mode;
}

static isp_u16 _pm_get_lens_grid_pitch(isp_u32 grid_pitch, isp_u32 width, isp_u32 flag)
{
	isp_u16 pitch=ISP_SUCCESS;

	if(0 == grid_pitch) {
	    return pitch;
	}

	if(ISP_ZERO!=((width/ISP_TWO-ISP_ONE) % grid_pitch)) {
	    pitch=((width/ISP_TWO-ISP_ONE)/grid_pitch+ISP_TWO);
	} else {
	    pitch=((width/ISP_TWO-ISP_ONE)/grid_pitch+ISP_ONE);
	}

	if(ISP_ONE == flag) {
	    pitch +=2;
	}

    return pitch;
}

static isp_u32 _ispLog2n(isp_u32 index)
{
	isp_u32 index_num=index;
	isp_u32 i=0x00;

	for(i=0x00; index_num>1; i++) {
	    index_num>>=0x01;
	}

	return i;
}

static isp_s32 _pm_brightness_init(void *dst_brightness, void *src_brightness, void* param1, void* param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct sensor_bright_param *src_ptr = (struct sensor_bright_param*)src_brightness;
	struct isp_bright_param *dst_ptr = (struct isp_bright_param*)dst_brightness;
	struct isp_pm_block_header *bright_header_ptr = (struct isp_pm_block_header*)param1;
	UNUSED(param2);

	dst_ptr->cur_index = src_ptr->cur_index;
	dst_ptr->cur.bypass = bright_header_ptr->bypass;
	dst_ptr->cur.factor = src_ptr->factor[src_ptr->cur_index];
	memcpy((void*)dst_ptr->bright_tab, (void*)src_ptr->factor, sizeof(dst_ptr->bright_tab));
	memcpy((void*)dst_ptr->scene_mode_tab, (void*)src_ptr->scenemode, sizeof(dst_ptr->scene_mode_tab));
	bright_header_ptr->is_update = 1;

	return rtn;
}

static isp_s32 _pm_brightness_set_param(void *bright_param, isp_u32 cmd, void* param_ptr0, void* param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_bright_param *bright_ptr = (struct isp_bright_param*)bright_param;
	struct isp_pm_block_header *bright_header_ptr = (struct isp_pm_block_header*)param_ptr1;

	bright_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_BRIGHT_BYPASS:
		bright_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_BRIGHT:
		bright_ptr->cur_index = *((isp_u32*)param_ptr0);
		bright_ptr->cur.factor = bright_ptr->bright_tab[bright_ptr->cur_index];
	break;

	case ISP_PM_BLK_SCENE_MODE:
	{
		isp_u32 idx = *((isp_u32*)param_ptr0);
		if (0 == idx) {
			bright_ptr->cur.factor = bright_ptr->bright_tab[bright_ptr->cur_index];
		} else {
			bright_ptr->cur.factor = bright_ptr->scene_mode_tab[idx];
		}
	}
	break;

	default:
		bright_header_ptr->is_update = ISP_ZERO;
	break;
	}

	return rtn;
}

static isp_s32 _pm_brightness_get_param(void *bright_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_bright_param *bright_ptr = (struct isp_bright_param*)bright_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->cmd = cmd;
	param_data_ptr->id = ISP_BLK_BRIGHT;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void*)&bright_ptr->cur;
		param_data_ptr->data_size = sizeof(bright_ptr->cur);
		*update_flag = 0;
	break;

	case ISP_PM_BLK_BRIGHT_BYPASS:
		param_data_ptr->data_ptr = (void*)&bright_ptr->cur.bypass;
		param_data_ptr->data_size = sizeof(bright_ptr->cur.bypass);
	break;

	default:
	break;
	}

	return rtn;
}

static isp_s32 _pm_contrast_init(void *dst_contrast, void *src_contrast, void* param1, void* param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct sensor_contrast_param *src_ptr = (struct sensor_contrast_param*)src_contrast;
	struct isp_contrast_param *dst_ptr = (struct isp_contrast_param*)dst_contrast;
	struct isp_pm_block_header *contrast_header_ptr = (struct  isp_pm_block_header*)param1;
	UNUSED(param2);

	dst_ptr->cur_index = src_ptr->cur_index;
	dst_ptr->cur.bypass = contrast_header_ptr->bypass;
	dst_ptr->cur.factor = src_ptr->factor[src_ptr->cur_index];
	memcpy((void*)dst_ptr->tab, (void*)src_ptr->factor, sizeof(dst_ptr->tab));
	memcpy((void*)dst_ptr->scene_mode_tab, (void*)src_ptr->scenemode, sizeof(dst_ptr->scene_mode_tab));
	contrast_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_contrast_set_param(void *contrast_param, isp_u32 cmd, void* param_ptr0, void* param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_contrast_param *contrast_ptr = (struct isp_contrast_param*)contrast_param;
	struct isp_pm_block_header *contrast_header_ptr = (struct isp_pm_block_header*)param_ptr1;

	contrast_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_CONTRAST_BYPASS:
		contrast_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_CONTRAST:
		contrast_ptr->cur_index = *((isp_u32*)param_ptr0);
		contrast_ptr->cur.factor = contrast_ptr->tab[contrast_ptr->cur_index];
	break;

	case ISP_PM_BLK_SCENE_MODE:
	{
		isp_u32 idx = *((isp_u32*)param_ptr0);
		if (0 == idx) {
			contrast_ptr->cur.factor = contrast_ptr->tab[contrast_ptr->cur_index];
		} else {
			contrast_ptr->cur.factor  = contrast_ptr->scene_mode_tab[idx];
		}
	}
	break;

	default:
		contrast_header_ptr->is_update = ISP_ZERO;
	break;
	}

	return rtn;
}

static isp_s32 _pm_contrast_get_param(void *contrast_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_contrast_param *contrast_ptr = (struct isp_contrast_param*)contrast_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_CONTRAST;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void*)&contrast_ptr->cur;
		param_data_ptr->data_size = sizeof(contrast_ptr->cur);
		*update_flag = 0;
	break;

	case ISP_PM_BLK_CONTRAST_BYPASS:
		param_data_ptr->data_ptr = (void*)&contrast_ptr->cur.bypass;
		param_data_ptr->data_size = sizeof(contrast_ptr->cur.bypass);
	break;

	default:
	break;
	}

	return rtn;
}

static isp_s32 _pm_flashlight_init(void *dst_flash_param, void *src_flash_param, void* param1, void* param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_flash_param* dst_flash_ptr = (struct isp_flash_param*)dst_flash_param;
	struct sensor_flash_cali_param *src_flash_ptr = (struct sensor_flash_cali_param*)src_flash_param;
	struct isp_pm_block_header *flashlight_header_ptr = (struct isp_pm_block_header*)param1;
	UNUSED(param2);

	dst_flash_ptr->cur.r_ratio = src_flash_ptr->r_ratio;
	dst_flash_ptr->cur.g_ratio = src_flash_ptr->g_ratio;
	dst_flash_ptr->cur.b_ratio = src_flash_ptr->b_ratio;
	dst_flash_ptr->cur.lum_ratio = src_flash_ptr->lum_ratio;
	dst_flash_ptr->cur.auto_flash_thr = src_flash_ptr->auto_threshold;
	dst_flash_ptr->attrib.global.r_sum = src_flash_ptr->attrib.global.r_sum;
	dst_flash_ptr->attrib.global.gr_sum = src_flash_ptr->attrib.global.gr_sum;
	dst_flash_ptr->attrib.global.gb_sum = src_flash_ptr->attrib.global.gb_sum;
	dst_flash_ptr->attrib.global.b_sum = src_flash_ptr->attrib.global.b_sum;
	dst_flash_ptr->attrib.random.r_sum = src_flash_ptr->attrib.random.r_sum;
	dst_flash_ptr->attrib.random.gr_sum = src_flash_ptr->attrib.random.gr_sum;
	dst_flash_ptr->attrib.random.gb_sum = src_flash_ptr->attrib.random.gb_sum;
	dst_flash_ptr->attrib.random.b_sum = src_flash_ptr->attrib.random.b_sum;
	flashlight_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_flashlight_set_param(void *flash_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct sensor_flash_cali_param *src_flash_ptr = PNULL;
	struct isp_flash_param* dst_flash_ptr = (struct isp_flash_param*)flash_param;
	struct isp_pm_block_header *flash_header_ptr = (struct isp_pm_block_header*)param_ptr1;

	flash_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_FLASH:
		src_flash_ptr = (struct sensor_flash_cali_param*)param_ptr0;
		dst_flash_ptr->cur.r_ratio = src_flash_ptr->r_ratio;
		dst_flash_ptr->cur.g_ratio = src_flash_ptr->g_ratio;
		dst_flash_ptr->cur.b_ratio = src_flash_ptr->b_ratio;
		dst_flash_ptr->cur.lum_ratio = src_flash_ptr->lum_ratio;

		dst_flash_ptr->attrib.global.r_sum = src_flash_ptr->attrib.global.r_sum;
		dst_flash_ptr->attrib.global.gr_sum = src_flash_ptr->attrib.global.gr_sum;
		dst_flash_ptr->attrib.global.gb_sum = src_flash_ptr->attrib.global.gb_sum;
		dst_flash_ptr->attrib.global.b_sum = src_flash_ptr->attrib.global.b_sum;

		dst_flash_ptr->attrib.random.r_sum = src_flash_ptr->attrib.random.r_sum;
		dst_flash_ptr->attrib.random.gr_sum = src_flash_ptr->attrib.random.gr_sum;
		dst_flash_ptr->attrib.random.gb_sum = src_flash_ptr->attrib.random.gb_sum;
		dst_flash_ptr->attrib.random.b_sum = src_flash_ptr->attrib.random.b_sum;
	break;

	case ISP_PM_BLK_FLASH_RATION_LUM:
	{
		isp_u32 lum_ratio = *((isp_u32*)param_ptr0);
		dst_flash_ptr->cur.lum_ratio = lum_ratio;
	}
	break;

	case ISP_PM_BLK_FLASH_RATION_RGB:
	{
		struct isp_rgb_gains *rgb_ratio = (struct isp_rgb_gains*)param_ptr0;
		dst_flash_ptr->cur.r_ratio = rgb_ratio->gain_r;
		dst_flash_ptr->cur.g_ratio = rgb_ratio->gain_g;
		dst_flash_ptr->cur.b_ratio = rgb_ratio->gain_b;
	}
	break;

	default:
		flash_header_ptr->is_update = ISP_ZERO;
	break;
	}

	return rtn;
}

static isp_s32 _pm_flashlight_get_param(void *flash_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	struct isp_flash_param *flash_ptr = (struct isp_flash_param*)flash_param;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_FLASH_CALI;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &flash_ptr->cur;
		param_data_ptr->data_size = sizeof(flash_ptr->cur);
	break;

	default:
	break;
	}

	return rtn;
}


/***************************************************************************************/
struct isp_block_operations s_bright_ops = {_pm_brightness_init, _pm_brightness_set_param, _pm_brightness_get_param, PNULL, PNULL};
struct isp_block_operations s_contrast_ops = {_pm_contrast_init, _pm_contrast_set_param, _pm_contrast_get_param, PNULL, PNULL};
struct isp_block_operations s_flash_ops = {_pm_flashlight_init, _pm_flashlight_set_param, _pm_flashlight_get_param, PNULL, PNULL};
/*******************************************Tshark2*******************************************************/
static isp_s32 _pm_pre_gbl_gain_init_v1(void *dst_pre_gbl_gain, void *src_pre_gbl_gain, void* param1, void*param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct sensor_pre_global_gain_param *src_ptr = (struct sensor_pre_global_gain_param*)src_pre_gbl_gain;
	struct isp_pre_global_gain_param_v1 *dst_ptr = (struct isp_pre_global_gain_param_v1*)dst_pre_gbl_gain;
	struct isp_pm_block_header* pre_gbl_gain_header_ptr = (struct isp_pm_block_header*)param1;
	UNUSED(param2);

	dst_ptr->cur.bypass =  pre_gbl_gain_header_ptr->bypass;
	dst_ptr->cur.gain = src_ptr->gain;

	pre_gbl_gain_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_pre_gbl_gain_set_param_v1(void *pre_gbl_gain_param, isp_u32 cmd, void* param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pre_global_gain_param_v1 *pre_gbl_gain_ptr = (struct isp_pre_global_gain_param_v1*)pre_gbl_gain_param;
	struct isp_pm_block_header *pre_gbl_gain_header_ptr = (struct isp_pm_block_header*)param_ptr1;

	pre_gbl_gain_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_PRE_GBL_GAIN:
	{
		struct sensor_pre_global_gain_param *pre_gbl_gain_cfg_ptr = (struct sensor_pre_global_gain_param*)param_ptr0;
		pre_gbl_gain_ptr->cur.gain = pre_gbl_gain_cfg_ptr->gain;
	}
	break;

	case ISP_PM_BLK_PRE_GBL_GIAN_BYPASS:
		pre_gbl_gain_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	default:
		pre_gbl_gain_header_ptr->is_update = ISP_ZERO;
	break;
	}

	return rtn;
}

static isp_s32 _pm_pre_gbl_gain_get_param_v1(void *pre_gbl_gain_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pre_global_gain_param_v1 *pre_gbl_gain = (struct isp_pre_global_gain_param_v1*)pre_gbl_gain_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_PRE_GBL_GAIN_V1;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void*)&pre_gbl_gain->cur;
		param_data_ptr->data_size = sizeof(pre_gbl_gain->cur);
		*update_flag = 0;
	break;

	case ISP_PM_BLK_PRE_GBL_GIAN_BYPASS:
		param_data_ptr->data_ptr = (void*)&pre_gbl_gain->cur.bypass;
		param_data_ptr->data_size = sizeof(pre_gbl_gain->cur.bypass);
	break;

	default:
	break;
	}

	return rtn;
}

static isp_s32 _pm_blc_init_v1(void *dst_blc_param, void *src_blc_param, void* param1, void* param_ptr2)
{
	isp_u32 i = 0;
	isp_u32 index = 0;
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_blc_param_v1 *dst_blc_ptr = (struct isp_blc_param_v1*)dst_blc_param;
	struct sensor_blc_param_v1 *src_blc_ptr = (struct sensor_blc_param_v1*)src_blc_param;
	struct isp_pm_block_header *blc_header_ptr = (struct isp_pm_block_header*)param1;
	UNUSED(param_ptr2);
	index = src_blc_ptr->cur_index;

	for( i = 0x00; i < SENSOR_BLC_NUM; i++) {
		dst_blc_ptr->offset[i] = src_blc_ptr->tab[i];
	}

	dst_blc_ptr->cur.bypass = blc_header_ptr->bypass;
	//dst_blc_ptr->cur.mode = 0x00;
	dst_blc_ptr->cur.b = dst_blc_ptr->offset[index].b;
	dst_blc_ptr->cur.gb = dst_blc_ptr->offset[index].gb;
	dst_blc_ptr->cur.gr = dst_blc_ptr->offset[index].gr;
	dst_blc_ptr->cur.r = dst_blc_ptr->offset[index].r;
	blc_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_blc_set_param_v1(void *blc_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 index = 0;
	struct isp_blc_param_v1 *blc_ptr = (struct isp_blc_param_v1*)blc_param;
	struct isp_pm_block_header *blc_header_ptr = (struct isp_pm_block_header*)param_ptr1;

	blc_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_BLC_OFFSET:
		index = *((isp_u32*)param_ptr0);
		blc_ptr->cur.r = blc_ptr->offset[index].r;
		blc_ptr->cur.gr = blc_ptr->offset[index].gr;
		blc_ptr->cur.b= blc_ptr->offset[index].b;
		blc_ptr->cur.gb = blc_ptr->offset[index].gb;
	break;

	case ISP_PM_BLK_BLC_BYPASS:
		blc_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_BLC_MODE:
		blc_ptr->cur.mode = *((isp_u32*)param_ptr0);
	break;

	default:
		blc_header_ptr->is_update = ISP_ZERO;
	break;
	}

	return rtn;
}

static isp_s32 _pm_blc_get_param_v1(void *blc_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_blc_param_v1 *blc_ptr = (struct isp_blc_param_v1*)blc_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_BLC_V1;
	param_data_ptr->cmd = cmd;
	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &blc_ptr->cur;
		param_data_ptr->data_size = sizeof(blc_ptr->cur);
		*update_flag = 0;
	break;

	case ISP_PM_BLK_BLC_BYPASS:
		param_data_ptr->data_ptr = &blc_ptr->cur.bypass;
		param_data_ptr->data_size = sizeof(blc_ptr->cur.bypass);
	break;

	//this is the test code.yongheng.lu add
	case ISP_PM_BLK_BLC_OFFSET_GB:
		param_data_ptr->data_ptr = &blc_ptr->cur.gb;
		param_data_ptr->data_size = sizeof(blc_ptr->cur.gb);
	break;

	default:
	break;
	}

	return rtn;
}

static isp_s32 _pm_rgb_gain_init_v1(void *dst_gbl_gain, void *src_gbl_gain, void* param1, void* param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct sensor_rgb_gain_param *src_ptr = (struct sensor_rgb_gain_param*)src_gbl_gain;
	struct isp_rgb_gain_param_v1 *dst_ptr = (struct isp_rgb_gain_param_v1*)dst_gbl_gain;
	struct isp_pm_block_header *rgb_gain_header_ptr = (struct isp_pm_block_header*)param1;
	UNUSED(param2);

	dst_ptr->cur.bypass = rgb_gain_header_ptr->bypass;
	dst_ptr->cur.global_gain = src_ptr->glb_gain;
	dst_ptr->cur.r_gain = src_ptr->r_gain;
	dst_ptr->cur.b_gain = src_ptr->b_gain;
	dst_ptr->cur.g_gain = src_ptr->g_gain;
	rgb_gain_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_rgb_gain_set_param_v1(void *gbl_gain_param, isp_u32 cmd, void* param_ptr0, void* param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_rgb_gain_param_v1 *rgb_gain_ptr = (struct isp_rgb_gain_param_v1*)gbl_gain_param;
	struct isp_pm_block_header *rgb_gain_header_ptr =(struct isp_pm_block_header*)param_ptr1;

	rgb_gain_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_GBL_GAIN_BYPASS:
		rgb_gain_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_GBL_GAIN:
		rgb_gain_ptr->cur.global_gain = *((isp_u32*)param_ptr0);
	break;

	default:
		rgb_gain_header_ptr->is_update = ISP_ZERO;
	break;
	}

	return rtn;
}

static isp_s32 _pm_rgb_gain_get_param_v1(void *gbl_gain_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_rgb_gain_param_v1 *gbl_gain = (struct isp_rgb_gain_param_v1*)gbl_gain_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_RGB_GAIN_V1;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void*)&gbl_gain->cur;
		param_data_ptr->data_size = sizeof(gbl_gain->cur);
		*update_flag = 0;
	break;

	case ISP_PM_BLK_GBL_GAIN_BYPASS:
		param_data_ptr->data_ptr = (void*)&(gbl_gain->cur.bypass);
		param_data_ptr->data_size = sizeof(gbl_gain->cur.bypass);
	break;

	default:
	break;
	}

	return rtn;
}


static isp_u32 _pm_pwd_convert_param(void *dst_param, isp_u32 strength_level, isp_u32 mode_flag)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_s32 i = 0;
	struct isp_pre_wavelet_param_v1 *dst_ptr = (struct isp_pre_wavelet_param_v1*)dst_param;
	struct sensor_pwd_level* pwd_param;
	struct nr_param_ptr *nr_ptr;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->multi_mode_enable) {
		pwd_param = (struct sensor_pwd_level*)dst_ptr->pwd_ptr;
	} else {
		nr_ptr = (struct nr_param_ptr*)(dst_ptr->pwd_ptr);
		if (!nr_ptr) {
			return rtn;
		}

		switch(mode_flag) {
			case ISP_NR_COMMON_MODE:
				pwd_param = (struct sensor_pwd_level*)(nr_ptr->common_param_ptr);
			break;
			case ISP_NR_CAP_MODE:
				pwd_param = (struct sensor_pwd_level*)(nr_ptr->capture_param_ptr);
			break;
			case ISP_NR_VIDEO_MODE:
				pwd_param = (struct sensor_pwd_level*)(nr_ptr->video_param_ptr);
			break;
			case ISP_NR_PANO_MODE:
				pwd_param = (struct sensor_pwd_level*)(nr_ptr->pano_param_ptr);
			break;
			default :
				pwd_param = (struct sensor_pwd_level*)(nr_ptr->common_param_ptr);
			break;
		}
	}

	strength_level = PM_CLIP(strength_level, 0, SENSOR_SMART_LEVEL_NUM - 1);
	if (pwd_param != NULL) {
		dst_ptr->cur.radial_bypass = pwd_param[strength_level].radial_var.radial_bypass;
		dst_ptr->cur.center_pos_x = pwd_param[strength_level].radial_var.center_pos.x;
		dst_ptr->cur.center_pos_y = pwd_param[strength_level].radial_var.center_pos.y;
		dst_ptr->cur.delta_x2 = pwd_param[strength_level].radial_var.center_square.x;
		dst_ptr->cur.delta_y2 = pwd_param[strength_level].radial_var.center_square.y;
		dst_ptr->cur.r2_thr = pwd_param[strength_level].radial_var.r2_thr;
		dst_ptr->cur.addback = pwd_param[strength_level].addback;
		dst_ptr->cur.gain_max_thr = pwd_param[strength_level].radial_var.gain_max_thr;
		dst_ptr->cur.p_param1 = pwd_param[strength_level].radial_var.p1_param;
		dst_ptr->cur.p_param2 = pwd_param[strength_level].radial_var.p2_param;
		dst_ptr->cur.pos_x = 0;
		dst_ptr->cur.pos_y = 0;

		dst_ptr->cur.gain_thrs0 = pwd_param[strength_level].gain_thrs0;
		dst_ptr->cur.gain_thrs1 = pwd_param[strength_level].gain_thrs1;
		dst_ptr->cur.bitshift0 = pwd_param[strength_level].bitshif0;
		dst_ptr->cur.bitshift1 = pwd_param[strength_level].bitshif1;
		dst_ptr->cur.offset = pwd_param[strength_level].offset;
		dst_ptr->cur.nsr_slope = pwd_param[strength_level].nsr_thr_slope;
		dst_ptr->cur.lum_ratio = pwd_param[strength_level].lum_ratio;
		dst_ptr->cur.bypass = pwd_param[strength_level].bypass;
	}

	return rtn;
}

static isp_s32 _pm_pre_wavelet_init_v1(void *dst_pwd, void *src_pwd, void* param1, void* param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct sensor_pwd_param *src_ptr = (struct sensor_pwd_param*)src_pwd;
	struct isp_pre_wavelet_param_v1 *dst_ptr = (struct isp_pre_wavelet_param_v1*)dst_pwd;
	struct isp_pm_block_header *pwd_header_ptr = (struct isp_pm_block_header*)param1;
	UNUSED(param2);
	dst_ptr->cur.bypass = pwd_header_ptr->bypass;

	dst_ptr->cur.lum_shink_level = SENSOR_SMART_LEVEL_DEFAULT;
	dst_ptr->cur_level = SENSOR_SMART_LEVEL_DEFAULT;

	dst_ptr->pwd_ptr = src_ptr->param_ptr;
	dst_ptr->multi_mode_enable = src_ptr->reserved2[0];

	dst_ptr->cur.radial_bypass = src_ptr->pwd_level.radial_var.radial_bypass;
	dst_ptr->cur.center_pos_x = src_ptr->pwd_level.radial_var.center_pos.x;
	dst_ptr->cur.center_pos_y = src_ptr->pwd_level.radial_var.center_pos.y;
	dst_ptr->cur.delta_x2 = src_ptr->pwd_level.radial_var.center_square.x;
	dst_ptr->cur.delta_y2 = src_ptr->pwd_level.radial_var.center_square.y;
	dst_ptr->cur.r2_thr = src_ptr->pwd_level.radial_var.r2_thr;
	dst_ptr->cur.addback = src_ptr->pwd_level.addback;
	dst_ptr->cur.gain_max_thr = src_ptr->pwd_level.radial_var.gain_max_thr;
	dst_ptr->cur.p_param1 = src_ptr->pwd_level.radial_var.p1_param;
	dst_ptr->cur.p_param2 = src_ptr->pwd_level.radial_var.p2_param;
	dst_ptr->cur.pos_x = 0;
	dst_ptr->cur.pos_y = 0;
	dst_ptr->cur.gain_thrs0 = src_ptr->pwd_level.gain_thrs0;
	dst_ptr->cur.gain_thrs1 = src_ptr->pwd_level.gain_thrs1;
	dst_ptr->cur.bitshift0 = src_ptr->pwd_level.bitshif0;
	dst_ptr->cur.bitshift1 = src_ptr->pwd_level.bitshif1;
	dst_ptr->cur.offset = src_ptr->pwd_level.offset;
	dst_ptr->cur.nsr_slope = src_ptr->pwd_level.nsr_thr_slope;
	dst_ptr->cur.lum_ratio = src_ptr->pwd_level.lum_ratio;

	rtn = _pm_pwd_convert_param(dst_ptr, SENSOR_SMART_LEVEL_DEFAULT, ISP_NR_COMMON_MODE);
	dst_ptr->cur.bypass |= pwd_header_ptr->bypass;
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("ISP_PM_PWD_CONVERT_PARAM: error!");
		return rtn;
	}

	pwd_header_ptr->is_update = ISP_ONE;

	return rtn;
}


static isp_s32 _pm_pre_wavelet_set_param_v1(void *pwd_param, isp_u32 cmd, void* param_ptr0, void* param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pre_wavelet_param_v1 *pwd_param_ptr = (struct isp_pre_wavelet_param_v1*)pwd_param;
	struct isp_pm_block_header *pwd_header_ptr =(struct isp_pm_block_header*)param_ptr1;

	pwd_header_ptr->is_update = ISP_ONE;

	switch(cmd){
	case ISP_PM_BLK_SMART_SETTING:
	{
		struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;
		struct isp_range val_range = {0};
		isp_u32 cur_level = 0;

		pwd_header_ptr->is_update = ISP_ZERO;
		val_range.min = 0;
		val_range.max = 255;

		rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_VALUE);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !");
			return rtn;
		}

		cur_level = (isp_u32)block_result->component[0].fix_data[0];

		if (cur_level != pwd_param_ptr->cur_level || nr_tool_flag[0] || block_result->mode_flag_changed) {
			pwd_param_ptr->cur_level = cur_level;
			pwd_param_ptr->cur.lum_shink_level = cur_level;
			pwd_header_ptr->is_update = ISP_ONE;
			nr_tool_flag[0] = 0;

			rtn = _pm_pwd_convert_param(pwd_param_ptr, cur_level, block_result->mode_flag);
			pwd_param_ptr->cur.bypass |= pwd_header_ptr->bypass;
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("ISP_PM_PWD_CONVERT_PARAM: error!");
				return rtn;
			}
		}
	}
	break;
	case ISP_PM_BLK_PRE_WAVELET_BYPASS:
		pwd_param_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	default:
		break;
	}

	ISP_LOGI("ISP_SMART: cmd=%d, update=%d, cur_level=%d", cmd, pwd_header_ptr->is_update,
					pwd_param_ptr->cur_level);
	return rtn;
}

static isp_s32 _pm_pre_wavelet_get_param_v1(void *pwd_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pre_wavelet_param_v1 *pwd_param_ptr = (struct isp_pre_wavelet_param_v1*)pwd_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_PRE_WAVELET_V1;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void*)&pwd_param_ptr->cur;
		param_data_ptr->data_size = sizeof(pwd_param_ptr->cur);
		*update_flag = 0;
	break;

	default:
	break;
	}

	return rtn;

}

static isp_s32 _pm_nlc_init_v1(void *dst_nlc_param, void *src_nlc_param, void* param1, void* param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i = 0;
	struct isp_nlc_param_v1 *dst_nlc_ptr = (struct isp_nlc_param_v1*)dst_nlc_param;
	struct sensor_nlc_v1_param *src_nlc_ptr = (struct sensor_nlc_v1_param*)src_nlc_param;
	struct isp_pm_block_header *nlc_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	dst_nlc_ptr->cur.bypass = nlc_header_ptr->bypass;

	for (i = 0; i < ISP_NLC_POINTER_NUM; ++i) {
		dst_nlc_ptr->cur.node.r_node[i] = src_nlc_ptr->r_node[i];
		dst_nlc_ptr->cur.node.g_node[i] = src_nlc_ptr->g_node[i];
		dst_nlc_ptr->cur.node.b_node[i] = src_nlc_ptr->b_node[i];
	}

	for( i = 0; i < ISP_NLC_POINTER_L_NUM; ++i) {
		dst_nlc_ptr->cur.node.l_node[i] = src_nlc_ptr->l_node[i];
	}

	nlc_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_nlc_set_param_v1(void *nlc_param, isp_u32 cmd, void* param_ptr0, void *param_ptr1)
{
	isp_u32 i = 0;
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_nlc_param_v1 *dst_nlc_ptr = (struct isp_nlc_param_v1*)nlc_param;
	struct sensor_nlc_v1_param *src_nlc_ptr = (struct sensor_nlc_v1_param*)param_ptr0;
	struct isp_pm_block_header *nlc_ptr = (struct isp_pm_block_header*)param_ptr1;

	nlc_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_NLC:
		for (i = 0; i < ISP_NLC_POINTER_NUM; ++i) {
			dst_nlc_ptr->cur.node.r_node[i] = src_nlc_ptr->r_node[i];
			dst_nlc_ptr->cur.node.g_node[i] = src_nlc_ptr->g_node[i];
			dst_nlc_ptr->cur.node.b_node[i] = src_nlc_ptr->b_node[i];
		}
		for( i = 0; i < ISP_NLC_POINTER_L_NUM; ++i) {
			dst_nlc_ptr->cur.node.l_node[i] = src_nlc_ptr->l_node[i];
		}
	break;

	case ISP_PM_BLK_NLC_BYPASS:
		dst_nlc_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	default:
		nlc_ptr->is_update = ISP_ZERO;
	break;

	}

	return rtn;
}

static isp_s32 _pm_nlc_get_param_v1(void *nlc_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	struct isp_nlc_param *nlc_ptr = (struct isp_nlc_param*)nlc_param;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_NLC_V1;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &nlc_ptr->cur;
		param_data_ptr->data_size = sizeof(nlc_ptr->cur);
		*update_flag = 0;
	break;

	case ISP_PM_BLK_NLC_BYPASS:
		param_data_ptr->data_ptr = (void*)&nlc_ptr->cur.bypass;
		param_data_ptr->data_size = sizeof(nlc_ptr->cur.bypass);
	break;

	default:
	break;
	}

	return rtn;
}

static isp_s32 _pm_2d_lsc_init(void * dst_lnc_param,void * src_lnc_param,void * param1,void * param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i = 0;
	intptr_t addr = 0, index = 0;
	struct isp_size *img_size_ptr = (struct isp_size*)param2;
	struct isp_2d_lsc_param *dst_lnc_ptr = (struct isp_2d_lsc_param*)dst_lnc_param;
	struct sensor_2d_lsc_param *src_lnc_ptr = (struct sensor_2d_lsc_param*)src_lnc_param;
	struct isp_pm_block_header *lnc_header_ptr = (struct isp_pm_block_header*)param1;

	dst_lnc_ptr->tab_num = src_lnc_ptr->tab_num;
	for (i = 0; i < ISP_COLOR_TEMPRATURE_NUM ; ++i) {
	//yongheng.lu
		addr = (intptr_t)&(src_lnc_ptr->data_area) + src_lnc_ptr->map[i].offset;
		dst_lnc_ptr->map_tab[i].param_addr = (void*)addr;
		dst_lnc_ptr->map_tab[i].len = src_lnc_ptr->map[i].len;
		dst_lnc_ptr->map_tab[i].grid = src_lnc_ptr->map[i].grid;
		dst_lnc_ptr->map_tab[i].grid_mode = src_lnc_ptr->map[i].grid;
		dst_lnc_ptr->map_tab[i].grid_pitch = _pm_get_lens_grid_pitch(src_lnc_ptr->map[i].grid, img_size_ptr->w, ISP_ONE);
		dst_lnc_ptr->map_tab[i].gain_w = dst_lnc_ptr->map_tab[i].grid_pitch;
		dst_lnc_ptr->map_tab[i].gain_h = _pm_get_lens_grid_pitch(src_lnc_ptr->map[i].grid, img_size_ptr->h, ISP_ONE);
	}
	if ((PNULL != dst_lnc_ptr->final_lsc_param.data_ptr)\
		&& (dst_lnc_ptr->final_lsc_param.size < src_lnc_ptr->map[0].len)) {
		free(dst_lnc_ptr->final_lsc_param.data_ptr);
		dst_lnc_ptr->final_lsc_param.data_ptr = PNULL;
		dst_lnc_ptr->final_lsc_param.size = 0;
	}
	if (PNULL == dst_lnc_ptr->final_lsc_param.data_ptr) {
		dst_lnc_ptr->final_lsc_param.data_ptr = (void*)malloc(src_lnc_ptr->map[0].len);
		if (PNULL == dst_lnc_ptr->final_lsc_param.data_ptr) {
			rtn = ISP_ERROR;
			ISP_LOGE("_pm_2d_lsc_init: malloc failed\n");
			return rtn;
		}
	}
	dst_lnc_ptr->tab_num = src_lnc_ptr->tab_num;
	dst_lnc_ptr->cur_index_info = src_lnc_ptr->cur_idx;
	index  = src_lnc_ptr->cur_idx.x0;
	dst_lnc_ptr->cur_index_info.weight0 = 0;
	dst_lnc_ptr->cur_index_info.x0 = 0;
	dst_lnc_ptr->final_lsc_param.size = src_lnc_ptr->map[index].len;
	memcpy((void*)dst_lnc_ptr->final_lsc_param.data_ptr, (void*)dst_lnc_ptr->map_tab[index].param_addr, dst_lnc_ptr->map_tab[index].len);
	dst_lnc_ptr->cur.buf_len = dst_lnc_ptr->final_lsc_param.size;
	dst_lnc_ptr->final_lsc_param.param_ptr = (void*)malloc(src_lnc_ptr->map[0].len);
	memcpy((void*)dst_lnc_ptr->final_lsc_param.param_ptr, (void*)dst_lnc_ptr->map_tab[index].param_addr, dst_lnc_ptr->map_tab[index].len);
	//dst_lnc_ptr->cur.buf_addr = (intptr_t)dst_lnc_ptr->final_lsc_param.data_ptr;
#if __WORDSIZE == 64
	dst_lnc_ptr->cur.buf_addr[0] = (isp_uint)(dst_lnc_ptr->final_lsc_param.data_ptr) & 0xffffffff;
	dst_lnc_ptr->cur.buf_addr[1] = (isp_uint)(dst_lnc_ptr->final_lsc_param.data_ptr) >> 32;
#else
	dst_lnc_ptr->cur.buf_addr[0] = (isp_uint)(dst_lnc_ptr->final_lsc_param.data_ptr);
	dst_lnc_ptr->cur.buf_addr[1] = 0;
#endif
	dst_lnc_ptr->cur.slice_size.width = img_size_ptr->w;
	dst_lnc_ptr->cur.slice_size.height = img_size_ptr->h;
	dst_lnc_ptr->cur.grid_width = dst_lnc_ptr->map_tab[index].grid_mode;
	dst_lnc_ptr->cur.grid_x_num = _pm_get_lens_grid_pitch(dst_lnc_ptr->cur.grid_width, dst_lnc_ptr->cur.slice_size.width, ISP_ZERO);
	dst_lnc_ptr->cur.grid_y_num = _pm_get_lens_grid_pitch(dst_lnc_ptr->cur.grid_width, dst_lnc_ptr->cur.slice_size.height, ISP_ZERO);
	dst_lnc_ptr->cur.grid_num_t = (dst_lnc_ptr->cur.grid_x_num + 2) * (dst_lnc_ptr->cur.grid_y_num + 2);
	dst_lnc_ptr->cur.grid_pitch = dst_lnc_ptr->cur.grid_x_num + 2;
	dst_lnc_ptr->cur.endian = ISP_ONE;
	dst_lnc_ptr->resolution = *img_size_ptr;
	dst_lnc_ptr->is_init = ISP_ONE;
	dst_lnc_ptr->cur.bypass = lnc_header_ptr->bypass;
	lnc_header_ptr->is_update = ISP_PM_BLK_LSC_UPDATE_MASK_PARAM;

	return rtn;
}

static isp_s32 _pm_2d_lsc_otp_active(struct sensor_2d_lsc_param *lsc_ptr, struct isp_cali_lsc_info *cali_lsc_ptr)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i = 0, j = 0, num = 0;
	isp_u32 buf_size = 0;
	isp_u32 ct_min = 0, ct_max = 0, ct = 0;
	isp_u16 *tmp_buf = PNULL;
	isp_u16 *data_addr = PNULL, *dst_ptr = PNULL;
	isp_u16* addr_array[ISP_CALIBRATION_MAX_LSC_NUM];
	isp_u32 is_print_log = _is_print_log();

	if (NULL == lsc_ptr || NULL == cali_lsc_ptr) {
		ISP_LOGE("invalid parameter");
		return ISP_ERROR;
	}

	if (is_print_log) {
		ISP_LOGI("calibration lsc map num=%d", cali_lsc_ptr->num);

		for (i=0; i<cali_lsc_ptr->num; i++) {
			ISP_LOGI("[%d], envi=%d, ct=%d, size(%d, %d), grid=%d, offset=%d, len=%d", i,
				cali_lsc_ptr->map[i].ct >> 16, cali_lsc_ptr->map[i].ct & 0xffff,
				cali_lsc_ptr->map[i].width, cali_lsc_ptr->map[i].height, cali_lsc_ptr->map[i].grid,
				cali_lsc_ptr->map[i].offset, cali_lsc_ptr->map[i].len);
		}

		ISP_LOGI("origin lsc map num=%d", lsc_ptr->tab_num);
		for (i=0; i<lsc_ptr->tab_num; i++) {
			ISP_LOGI("[%d], envi=%d, ct=%d, grid=%d, offset=%d, len=%d", i,
				lsc_ptr->map[i].envi, lsc_ptr->map[i].ct, lsc_ptr->map[i].grid, lsc_ptr->map[i].offset,
				lsc_ptr->map[i].len);
		}
	}

	for (i=0; i<cali_lsc_ptr->num; i++) {
		isp_u32 src_envi = cali_lsc_ptr->map[i].ct >> 16;
		isp_u32 src_ct = cali_lsc_ptr->map[i].ct & 0xffff;
		isp_u16 *src_data = (isp_u16 *)((isp_u8 *)&cali_lsc_ptr->data_area + cali_lsc_ptr->map[i].offset);
		isp_u32 src_data_size = cali_lsc_ptr->map[i].len;
		isp_u32 j = 0;
		isp_u32 dst_index = 0xfff;
		isp_u32 min_ct_diff = 0xffff;

		if (is_print_log)
			ISP_LOGI("%d: ------------------", i);

		for (j=0; j<lsc_ptr->tab_num; j++) {
			isp_u32 dst_envi = lsc_ptr->map[j].envi;
			isp_u32 dst_ct = lsc_ptr->map[j].ct;

			if (dst_envi == src_envi) {
				isp_u32 ct_diff = abs(dst_ct - src_ct);
				if (ct_diff < min_ct_diff) {
					min_ct_diff = ct_diff;
					dst_index = j;
					//ISP_LOGI("[%d] min ct diff=%d", j, min_ct_diff);
				}
			}
		}

		//find useful one
		if (min_ct_diff <= 1000 && dst_index < lsc_ptr->tab_num) {
			struct isp_size src_size = {0};
			struct isp_size dst_size = {0};

			if (is_print_log)
				ISP_LOGI("suitable lsc find! min index = %d, min ct diff=%d", dst_index, min_ct_diff);

			src_size.w = cali_lsc_ptr->map[dst_index].width;
			src_size.h = cali_lsc_ptr->map[dst_index].height;
			dst_size.w = lsc_ptr->map[dst_index].width;
			dst_size.h = lsc_ptr->map[dst_index].height;
			isp_u16 *dst_data = (isp_u16 *)((isp_u8 *)&lsc_ptr->data_area +  lsc_ptr->map[dst_index].offset);
			isp_u32 dst_data_size = lsc_ptr->map[dst_index].len;

			if (src_size.w == dst_size.w && src_size.h == dst_size.h
				&& src_data_size == dst_data_size) {
				memcpy(dst_data, src_data, dst_data_size);
				if (is_print_log)
					ISP_LOGI("size is the same, just copy!");
			} else {
				/*need scaling*/
				//isp_scaling_lsc_gain(dst_data, src_data, &dst_size, &src_size);
				if (is_print_log)
					ISP_LOGI("src size=%dX%d, dst size=%dX%d, need scaling", src_size.w, src_size.h, dst_size.w, dst_size.h);
			}
		}

	}

	return rtn;
}

static isp_s32 _pm_2d_lsc_set_param(void *lnc_param, isp_u32 cmd, void* param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_2d_lsc_param *dst_lnc_ptr = (struct isp_2d_lsc_param*)lnc_param;
	struct isp_pm_block_header *lnc_header_ptr = (struct isp_pm_block_header*)param_ptr1;

	switch (cmd) {
	case ISP_PM_BLK_LSC_BYPASS:
	{
		dst_lnc_ptr->cur.bypass = *((isp_u32*)param_ptr0);
		lnc_header_ptr->is_update |= ISP_PM_BLK_LSC_UPDATE_MASK_PARAM;
		dst_lnc_ptr->update_flag = lnc_header_ptr->is_update;
	}
	break;

	case ISP_PM_BLK_LSC_MEM_ADDR:
	{
		uint16_t* plsc = param_ptr0;
		memcpy((void*)dst_lnc_ptr->final_lsc_param.data_ptr, param_ptr0, dst_lnc_ptr->final_lsc_param.size);

#if __WORDSIZE == 64
	dst_lnc_ptr->cur.buf_addr[0] = (isp_uint)(dst_lnc_ptr->final_lsc_param.data_ptr) & 0xffffffff;
	dst_lnc_ptr->cur.buf_addr[1] = (isp_uint)(dst_lnc_ptr->final_lsc_param.data_ptr) >> 32;
#else
	dst_lnc_ptr->cur.buf_addr[0] = (isp_uint)(dst_lnc_ptr->final_lsc_param.data_ptr);
	dst_lnc_ptr->cur.buf_addr[1] = 0;
#endif
	lnc_header_ptr->is_update |= ISP_PM_BLK_LSC_UPDATE_MASK_VALIDATE;
	dst_lnc_ptr->update_flag = lnc_header_ptr->is_update;
	}
	break;

	case ISP_PM_BLK_LSC_VALIDATE:
		if (0 != *((isp_u32*)param_ptr0)) {
		lnc_header_ptr->is_update |= ISP_PM_BLK_LSC_UPDATE_MASK_VALIDATE;
		} else {
			lnc_header_ptr->is_update &= (~ISP_PM_BLK_LSC_UPDATE_MASK_VALIDATE);
		}
		dst_lnc_ptr->update_flag = lnc_header_ptr->is_update;
	break;

	case ISP_PM_BLK_SMART_SETTING:
	{
		struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;
		struct isp_weight_value *weight_value = NULL;
		struct isp_range val_range = {0};
		struct isp_weight_value lnc_value = {{0}, {0}};
		void *src_lsc_ptr0 = NULL;
		void *src_lsc_ptr1 = NULL;
		void *dst_lsc_ptr = NULL;
		isp_u32 index = 0;

		if (block_result->update == 0)
		{
			return rtn;
		}

		val_range.min = 0;
		val_range.max = ISP_COLOR_TEMPRATURE_NUM - 1;
		rtn = _pm_check_smart_param(block_result, &val_range, 1,
					ISP_SMART_Y_TYPE_WEIGHT_VALUE);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !\n");
			return rtn;
		}

		weight_value = (struct isp_weight_value *)block_result->component[0].fix_data;
		lnc_value = *weight_value;

		ISP_LOGV("value=(%d, %d), weight=(%d, %d) 0, ", lnc_value.value[0], lnc_value.value[1],
							lnc_value.weight[0], lnc_value.weight[1]);

		lnc_value.weight[0] = lnc_value.weight[0] / (SMART_WEIGHT_UNIT / 16) * (SMART_WEIGHT_UNIT / 16);
		lnc_value.weight[1] = SMART_WEIGHT_UNIT - lnc_value.weight[0];

		ISP_LOGV("value=(%d, %d), weight=(%d, %d) 1", lnc_value.value[0], lnc_value.value[1],
							lnc_value.weight[0], lnc_value.weight[1]);

		if (lnc_value.weight[0] != dst_lnc_ptr->cur_index_info.weight0
			|| lnc_value.value[0] != dst_lnc_ptr->cur_index_info.x0) {

			void *src[2] = {NULL};
			void *dst = NULL;
			isp_u32 data_num = 0;

			index = lnc_value.value[0];
			dst_lnc_ptr->cur.grid_pitch = dst_lnc_ptr->map_tab[index].grid_pitch;
			dst_lnc_ptr->cur.grid_width = dst_lnc_ptr->map_tab[index].grid_mode;
			dst_lnc_ptr->cur.buf_len = dst_lnc_ptr->map_tab[index].len;
			src[0] = (void*)dst_lnc_ptr->map_tab[lnc_value.value[0]].param_addr;
			src[1] = (void*)dst_lnc_ptr->map_tab[lnc_value.value[1]].param_addr;
			//dst = (void*)dst_lnc_ptr->cur.buf_addr;
		#if __WORDSIZE == 64
			dst = (void*)((isp_uint)dst_lnc_ptr->cur.buf_addr[1]<<32 | dst_lnc_ptr->cur.buf_addr[0]);
		#else
			dst = (void*)(dst_lnc_ptr->cur.buf_addr[0]);
		#endif
			data_num = dst_lnc_ptr->cur.buf_len / sizeof(isp_u16);

			rtn = isp_interp_data(dst, src, (isp_u16*)lnc_value.weight, data_num, ISP_INTERP_UINT16);
			if (ISP_SUCCESS == rtn) {
				dst_lnc_ptr->cur_index_info.x0 = lnc_value.value[0];
				dst_lnc_ptr->cur_index_info.x1 = lnc_value.value[1];
				dst_lnc_ptr->cur_index_info.weight0 = lnc_value.weight[0];
				dst_lnc_ptr->cur_index_info.weight1 = lnc_value.weight[1];
				lnc_header_ptr->is_update |= ISP_PM_BLK_LSC_UPDATE_MASK_VALIDATE;
				dst_lnc_ptr->update_flag = lnc_header_ptr->is_update;
				memcpy(dst_lnc_ptr->final_lsc_param.param_ptr, dst, dst_lnc_ptr->cur.buf_len);
			}
		}
	}
	break;

	case ISP_PM_BLK_LSC_OTP:
	{
		void *src[2] = {NULL};
		void *dst = NULL;
		isp_u32 data_num;
		isp_u16 weight[2] = {0};
		struct sensor_2d_lsc_param *lsc_ptr = (struct sensor_2d_lsc_param*)lnc_header_ptr->absolute_addr;
		struct isp_cali_lsc_info *cali_lsc_ptr = (struct isp_cali_lsc_info*)param_ptr0;

		rtn = _pm_2d_lsc_otp_active(lsc_ptr, cali_lsc_ptr);

		//if (PNULL != dst_lnc_ptr->final_lsc_param.data_ptr)
		if (ISP_ONE == dst_lnc_ptr->is_init) {
			src[0] = (void*)dst_lnc_ptr->map_tab[dst_lnc_ptr->cur_index_info.x0].param_addr;
			src[1] = (void*)dst_lnc_ptr->map_tab[dst_lnc_ptr->cur_index_info.x1].param_addr;
			dst = (void*)dst_lnc_ptr->final_lsc_param.data_ptr;
			data_num = dst_lnc_ptr->cur.buf_len / sizeof(isp_u16);

			weight[0] = dst_lnc_ptr->cur_index_info.weight0;
			weight[1] = dst_lnc_ptr->cur_index_info.weight1;

			rtn = isp_interp_data(dst, src, (isp_u16*)weight, data_num, ISP_INTERP_UINT16);
			if (ISP_SUCCESS == rtn) {
				lnc_header_ptr->is_update |= ISP_PM_BLK_LSC_UPDATE_MASK_VALIDATE;
				dst_lnc_ptr->update_flag = lnc_header_ptr->is_update;
			}
		}
	}
	break;

		case ISP_PM_BLK_LSC_INFO:
		{
			lnc_header_ptr->is_update |= ISP_PM_BLK_LSC_UPDATE_MASK_PARAM;
			dst_lnc_ptr->update_flag = lnc_header_ptr->is_update;
			ISP_LOGI("ISP_PM_BLK_LSC_INFO");

			{
				//uint16_t *ptr = (uint16_t *)dst_lnc_ptr->cur.buf_addr;
				uint16_t *ptr = NULL;
			#if __WORDSIZE == 64
				ptr = (void*)((isp_uint)dst_lnc_ptr->cur.buf_addr[1]<<32 | dst_lnc_ptr->cur.buf_addr[0]);
			#else
				ptr = (void*)(dst_lnc_ptr->cur.buf_addr[0]);
			#endif
				ISP_LOGI("lsc[0]: 0x%0x, 0x%0x, 0x%0x, 0x%0x", *ptr, *(ptr+1), *(ptr+2), *(ptr+3));
				ISP_LOGI("lsc[1]: 0x%0x, 0x%0x, 0x%0x, 0x%0x", *(ptr+4), *(ptr+5), *(ptr+6), *(ptr+7));
			}
		}
		break;

		default:
		break;
	}

	ISP_LOGV("ISP_SMART: cmd=%d, update=%d, value=(%d, %d), weight=(%d, %d)\n", cmd, lnc_header_ptr->is_update,
				dst_lnc_ptr->cur_index_info.x0, dst_lnc_ptr->cur_index_info.x1,
				dst_lnc_ptr->cur_index_info.weight0, dst_lnc_ptr->cur_index_info.weight1);

	return rtn;
}

static isp_s32 _pm_2d_lsc_get_param(void *lnc_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	struct isp_2d_lsc_param *lnc_ptr = (struct isp_2d_lsc_param*)lnc_param;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_2D_LSC;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void*)&lnc_ptr->cur;
		param_data_ptr->data_size = sizeof(lnc_ptr->cur);
		*update_flag &= (~(ISP_PM_BLK_LSC_UPDATE_MASK_PARAM|ISP_PM_BLK_LSC_UPDATE_MASK_VALIDATE));
		lnc_ptr->update_flag = *update_flag;
	break;

	case ISP_PM_BLK_LSC_BYPASS:
		param_data_ptr->data_ptr = (void*)&lnc_ptr->cur.bypass;
		param_data_ptr->data_size = sizeof(lnc_ptr->cur.bypass);
	break;

	case ISP_PM_BLK_LSC_VALIDATE:
		param_data_ptr->data_ptr = PNULL;
		param_data_ptr->data_size = 0;
		param_data_ptr->user_data[1] = lnc_ptr->update_flag;
		*update_flag &= (~ISP_PM_BLK_LSC_UPDATE_MASK_VALIDATE);
		lnc_ptr->update_flag = *update_flag;
	break;

	case ISP_PM_BLK_LSC_INFO:
		lnc_ptr->lsc_info.cur_idx = lnc_ptr->cur_index_info;
		lnc_ptr->lsc_info.gain_w = lnc_ptr->map_tab[lnc_ptr->lsc_info.cur_idx.x0].gain_w;
		lnc_ptr->lsc_info.gain_h = lnc_ptr->map_tab[lnc_ptr->lsc_info.cur_idx.x0].gain_h;
		lnc_ptr->lsc_info.grid = lnc_ptr->map_tab[lnc_ptr->lsc_info.cur_idx.x0].grid;
		lnc_ptr->lsc_info.data_ptr = lnc_ptr->final_lsc_param.data_ptr;
		lnc_ptr->lsc_info.len = lnc_ptr->final_lsc_param.size;
		lnc_ptr->lsc_info.param_ptr = lnc_ptr->final_lsc_param.param_ptr;
		param_data_ptr->data_ptr = (void*)&lnc_ptr->lsc_info;
		param_data_ptr->data_size = sizeof(lnc_ptr->lsc_info);
	break;

	default:
	break;
	}

	return rtn;
}

static isp_s32 _pm_2d_lsc_deinit(void *lnc_param)
{
	isp_u32 rtn = ISP_SUCCESS;
	struct isp_2d_lsc_param *lsc_param_ptr = (struct isp_2d_lsc_param*)lnc_param;

	if (lsc_param_ptr->final_lsc_param.data_ptr) {
		free (lsc_param_ptr->final_lsc_param.data_ptr);
		lsc_param_ptr->final_lsc_param.data_ptr = PNULL;
		lsc_param_ptr->final_lsc_param.size = 0;
	}

	if (lsc_param_ptr->final_lsc_param.param_ptr) {
		free (lsc_param_ptr->final_lsc_param.param_ptr);
		lsc_param_ptr->final_lsc_param.param_ptr = PNULL;
	}
	return rtn;
}

static isp_s32 _pm_1d_lsc_init(void * dst_lnc_param,void * src_lnc_param,void * param1,void * param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i = 0, j = 0, index = 0;
	struct isp_1d_lsc_param *dst_lnc_ptr = (struct isp_1d_lsc_param*)dst_lnc_param;
	struct sensor_1d_lsc_param *src_lnc_ptr = (struct sensor_1d_lsc_param*)src_lnc_param;
	struct isp_pm_block_header *lnc_header_ptr = (struct isp_pm_block_header*)param1;
	UNUSED(param2);

	memset((void*)&dst_lnc_ptr->cur,0x00,sizeof(dst_lnc_ptr->cur));
	memcpy((void*)dst_lnc_ptr->map,(void*)src_lnc_ptr->map,sizeof(dst_lnc_ptr->map));
	dst_lnc_ptr->cur.bypass = lnc_header_ptr->bypass;
	dst_lnc_ptr->cur_index_info.x0 = src_lnc_ptr->cur_idx.x0;
	dst_lnc_ptr->cur_index_info.x1 = src_lnc_ptr->cur_idx.x1;
	dst_lnc_ptr->cur_index_info.weight0 = src_lnc_ptr->cur_idx.weight0;
	dst_lnc_ptr->cur_index_info.weight1 = src_lnc_ptr->cur_idx.weight1;

	for (i = 0; i < SENSOR_LENS_NUM; ++i) {
		for (j = 0; j < SENSOR_LNC_RC_NUM; ++j) {
			dst_lnc_ptr->map[i].curve_distcptn[j].center_pos.x = src_lnc_ptr->map[i].curve_distcptn[j].center_pos.x;
			dst_lnc_ptr->map[i].curve_distcptn[j].center_pos.y = src_lnc_ptr->map[i].curve_distcptn[j].center_pos.y;
			dst_lnc_ptr->map[i].curve_distcptn[j].delta_square.x = src_lnc_ptr->map[i].curve_distcptn[j].delta_square.x;
			dst_lnc_ptr->map[i].curve_distcptn[j].delta_square.y = src_lnc_ptr->map[i].curve_distcptn[j].delta_square.y;
			dst_lnc_ptr->map[i].curve_distcptn[j].coef.p1 = src_lnc_ptr->map[i].curve_distcptn[j].coef.p1;
			dst_lnc_ptr->map[i].curve_distcptn[j].coef.p2 = src_lnc_ptr->map[i].curve_distcptn[j].coef.p2;
		}
		dst_lnc_ptr->map[i].gain_thrs1 = src_lnc_ptr->map[i].gain_thrs1;
	}

	index = src_lnc_ptr->cur_idx.x0;
	dst_lnc_ptr->cur.gain_max_thr = src_lnc_ptr->map[index].gain_thrs1;
	dst_lnc_ptr->cur.center_r0c0_col_x = src_lnc_ptr->map[index].curve_distcptn[0].center_pos.x;
	dst_lnc_ptr->cur.center_r0c0_row_y = src_lnc_ptr->map[index].curve_distcptn[0].center_pos.y;
	dst_lnc_ptr->cur.center_r0c1_col_x = src_lnc_ptr->map[index].curve_distcptn[1].center_pos.x;
	dst_lnc_ptr->cur.center_r0c1_row_y = src_lnc_ptr->map[index].curve_distcptn[1].center_pos.y;
	dst_lnc_ptr->cur.center_r1c0_col_x = src_lnc_ptr->map[index].curve_distcptn[2].center_pos.x;
	dst_lnc_ptr->cur.center_r1c0_row_y = src_lnc_ptr->map[index].curve_distcptn[2].center_pos.y;
	dst_lnc_ptr->cur.center_r1c1_col_x = src_lnc_ptr->map[index].curve_distcptn[3].center_pos.x;
	dst_lnc_ptr->cur.center_r1c1_row_y = src_lnc_ptr->map[index].curve_distcptn[3].center_pos.y;

	dst_lnc_ptr->cur.delta_square_r0c0_x = src_lnc_ptr->map[index].curve_distcptn[0].delta_square.x;
	dst_lnc_ptr->cur.delta_square_r0c0_y = src_lnc_ptr->map[index].curve_distcptn[0].delta_square.y;
	dst_lnc_ptr->cur.delta_square_r0c1_x = src_lnc_ptr->map[index].curve_distcptn[1].delta_square.x;
	dst_lnc_ptr->cur.delta_square_r0c1_y = src_lnc_ptr->map[index].curve_distcptn[1].delta_square.y;
	dst_lnc_ptr->cur.delta_square_r1c0_x = src_lnc_ptr->map[index].curve_distcptn[2].delta_square.x;
	dst_lnc_ptr->cur.delta_square_r1c0_y = src_lnc_ptr->map[index].curve_distcptn[2].delta_square.y;
	dst_lnc_ptr->cur.delta_square_r1c1_x = src_lnc_ptr->map[index].curve_distcptn[3].delta_square.x;
	dst_lnc_ptr->cur.delta_square_r1c1_y = src_lnc_ptr->map[index].curve_distcptn[3].delta_square.y;

	dst_lnc_ptr->cur.coef_r0c0_p1 = src_lnc_ptr->map[index].curve_distcptn[0].coef.p1;
	dst_lnc_ptr->cur.coef_r0c0_p2 = src_lnc_ptr->map[index].curve_distcptn[0].coef.p2;
	dst_lnc_ptr->cur.coef_r0c1_p1 = src_lnc_ptr->map[index].curve_distcptn[1].coef.p1;
	dst_lnc_ptr->cur.coef_r0c1_p2 = src_lnc_ptr->map[index].curve_distcptn[1].coef.p2;
	dst_lnc_ptr->cur.coef_r1c0_p1 = src_lnc_ptr->map[index].curve_distcptn[2].coef.p1;
	dst_lnc_ptr->cur.coef_r1c0_p2 = src_lnc_ptr->map[index].curve_distcptn[2].coef.p2;
	dst_lnc_ptr->cur.coef_r1c1_p1 = src_lnc_ptr->map[index].curve_distcptn[3].coef.p1;
	dst_lnc_ptr->cur.coef_r1c1_p2 = src_lnc_ptr->map[index].curve_distcptn[3].coef.p2;

	lnc_header_ptr->is_update = lnc_header_ptr->is_update | ISP_PM_BLK_LSC_UPDATE_MASK_PARAM;

	return rtn;
}

static isp_s32 _pm_1d_lsc_set_param(void *lnc_param, isp_u32 cmd, void* param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_1d_lsc_param *dst_lnc_ptr = PNULL;
	struct isp_pm_block_header *lnc_header_ptr = (struct isp_pm_block_header*)param_ptr1;
	dst_lnc_ptr = (struct isp_1d_lsc_param*)lnc_param;

	lnc_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_LSC_BYPASS:
		dst_lnc_ptr->cur.bypass = *((isp_u32*)param_ptr0);
		lnc_header_ptr->is_update |= ISP_PM_BLK_LSC_UPDATE_MASK_PARAM;
	break;

	case ISP_PM_BLK_SMART_SETTING:
	break;

	default:
		lnc_header_ptr->is_update = ISP_ZERO;
	break;
	}

	return rtn;
}

static isp_s32 _pm_1d_lsc_get_param(void *lnc_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	struct isp_1d_lsc_param *lnc_ptr = (struct isp_1d_lsc_param*)lnc_param;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_1D_LSC;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void*)&lnc_ptr->cur;
		param_data_ptr->data_size = sizeof(lnc_ptr->cur);
		*update_flag &= (~ISP_PM_BLK_LSC_UPDATE_MASK_PARAM);
	break;

	case ISP_PM_BLK_LSC_BYPASS:
		param_data_ptr->data_ptr = (void*)&lnc_ptr->cur.bypass;
		param_data_ptr->data_size = sizeof(lnc_ptr->cur.bypass);
	break;

	default:
	break;
	}

	return rtn;
}

static isp_s32 _pm_binning4awb_init_v1(void *dst_binning4awb, void *src_binning4awb, void* param1, void* param2)
{
	isp_s32 rtn = ISP_SUCCESS;

	struct isp_binning4awb_param_v1 *dst_ptr = (struct isp_binning4awb_param_v1*)dst_binning4awb;
	struct isp_bin_param *src_ptr = (struct isp_bin_param*)src_binning4awb;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header*)param1;
	UNUSED(param2);

	memset((void*)&dst_ptr->cur,0x00,sizeof(dst_ptr->cur));
	dst_ptr->cur.bypass = header_ptr->bypass;
	dst_ptr->cur.hx = src_ptr->hx;
	dst_ptr->cur.vx = src_ptr->vx;
	header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_binning4awb_set_param_v1(void *binning4awb_param, isp_u32 cmd, void* param_ptr0, void* param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_binning4awb_param_v1 *dst_ptr = (struct isp_binning4awb_param_v1*)binning4awb_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header*)param_ptr1;

	header_ptr->is_update = ISP_ONE;

	switch (cmd) {
		case ISP_PM_BLK_BINNING4AWB_BYPASS:
			dst_ptr->cur.bypass = *((isp_u32*)param_ptr0);
		break;

		default:
			header_ptr->is_update = ISP_ZERO;
		break;
	}

	return rtn;
}

static isp_s32 _pm_binning4awb_get_param_v1(void *binning4awb_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_binning4awb_param_v1 *binning4awb_ptr = (struct isp_binning4awb_param_v1*)binning4awb_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag =(isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_BINNING4AWB_V1;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void*)&binning4awb_ptr->cur;
		param_data_ptr->data_size = sizeof(binning4awb_ptr->cur);
		*update_flag = 0;
	break;

	case ISP_PM_BLK_BINNING4AWB_BYPASS:
		param_data_ptr->data_ptr = (void*)&binning4awb_ptr->cur.bypass;
		param_data_ptr->data_size = sizeof(binning4awb_ptr->cur.bypass);
	break;

	default:
	break;
	}

	return rtn;
}

/*
static isp_s32 _pm_awbm_tshark2_init(void *dst_awbm, void *src_awbm, void* param1, void* param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_awbm_param_tshark2 *dst_ptr = PNULL;
	struct sensor_awbm_param *src_ptr = NULL;
	struct isp_pm_block_header *header_ptr = PNULL;

	dst_ptr = (struct isp_awbm_param_tshark2*)dst_awbm;
	src_ptr = (struct sensor_awbm_param*)src_awbm;
	header_ptr = (struct isp_pm_block_header*)param1;

	memset((void*)&dst_ptr->cur,0x00,sizeof(dst_ptr->cur));

	dst_ptr->cur.bypass = header_ptr->bypass;
	dst_ptr->cur.block_offset.x = src_ptr->win_start.x;
	dst_ptr->cur.block_offset.y = src_ptr->win_start.y;
	dst_ptr->cur.block_size.width = src_ptr->win_size.w;
	dst_ptr->cur.block_size.height = src_ptr->win_size.h;

	dst_ptr->cur.clctor_pos.start_x[0] = src_ptr->white_area[0].start_x;
	dst_ptr->cur.clctor_pos.start_x[1] = src_ptr->white_area[1].start_x;
	dst_ptr->cur.clctor_pos.start_x[2] = src_ptr->white_area[2].start_x;
	dst_ptr->cur.clctor_pos.start_x[3] = src_ptr->white_area[3].start_x;
	dst_ptr->cur.clctor_pos.start_x[4] = src_ptr->white_area[4].start_x;


	dst_ptr->cur.clctor_pos.start_y[0] = src_ptr->white_area[0].start_y;
	dst_ptr->cur.clctor_pos.start_y[1] = src_ptr->white_area[1].start_y;
	dst_ptr->cur.clctor_pos.start_y[2] = src_ptr->white_area[2].start_y;
	dst_ptr->cur.clctor_pos.start_y[3] = src_ptr->white_area[3].start_y;
	dst_ptr->cur.clctor_pos.start_y[4] = src_ptr->white_area[4].start_y;

	dst_ptr->cur.clctor_pos.end_x[0] = src_ptr->white_area[0].end_x;
	dst_ptr->cur.clctor_pos.end_x[1] = src_ptr->white_area[1].end_x;
	dst_ptr->cur.clctor_pos.end_x[2] = src_ptr->white_area[2].end_x;
	dst_ptr->cur.clctor_pos.end_x[3] = src_ptr->white_area[3].end_x;
	dst_ptr->cur.clctor_pos.end_x[4] = src_ptr->white_area[4].end_x;

	dst_ptr->cur.clctor_pos.end_y[0] = src_ptr->white_area[0].end_y;
	dst_ptr->cur.clctor_pos.end_y[1] = src_ptr->white_area[1].end_y;
	dst_ptr->cur.clctor_pos.end_y[2] = src_ptr->white_area[2].end_y;
	dst_ptr->cur.clctor_pos.end_y[3] = src_ptr->white_area[3].end_y;
	dst_ptr->cur.clctor_pos.end_y[4] = src_ptr->white_area[4].end_y;

	dst_ptr->cur.thr_bypass = src_ptr->comp_1d_bypass;

	dst_ptr->cur.thr.r_low  = src_ptr->low_thr.r_thr;
	dst_ptr->cur.thr.g_low  = src_ptr->low_thr.g_thr;
	dst_ptr->cur.thr.b_low  = src_ptr->low_thr.b_thr;

	dst_ptr->cur.thr.r_high = src_ptr->high_thr.b_thr;
	dst_ptr->cur.thr.g_high = src_ptr->high_thr.b_thr;
	dst_ptr->cur.thr.b_high = src_ptr->high_thr.b_thr;

	dst_ptr->cur.skip_num = src_ptr->skip_num;

	header_ptr->is_update = 1;

	return rtn;
}

static isp_s32 _pm_awbm_tshark2_set_param(void *awbm_param, isp_u32 cmd, void* param_ptr0, void* param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_awbm_param_tshark2 *dst_ptr = (struct isp_awbm_param_tshark2*)awbm_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header*)param_ptr1;

	header_ptr->is_update = 1;

	dst_ptr->cur.bypass = *((isp_u32*)param_ptr0);

	return rtn;
}

static isp_s32 _pm_awbm_tshark2_get_param(void *awbm_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_awbm_param_tshark2 *awbm_ptr = (struct isp_awbm_param_tshark2*)awbm_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag =(isp_u32*)rtn_param1;

	switch (cmd) {
		case ISP_PM_BLK_ISP_SETTING:
		{
			param_data_ptr->id = ISP_BLK_AWBM_V1;
			param_data_ptr->cmd = ISP_PM_BLK_ISP_SETTING;
			param_data_ptr->data_ptr = (void*)&awbm_ptr->cur;
			param_data_ptr->data_size = sizeof(awbm_ptr->cur);
			*update_flag = 0;
		}
		break;

		case ISP_PM_BLK_AWBM_BYPASS:
		{
			param_data_ptr->id = ISP_BLK_AWBM_V1;
			param_data_ptr->cmd = ISP_PM_BLK_AWBM_BYPASS;
			param_data_ptr->data_ptr = (void*)&awbm_ptr->cur.bypass;
			param_data_ptr->data_size = sizeof(awbm_ptr->cur.bypass);
		}
		break;

		default:
		break;
	}

	return rtn;
}

static isp_s32 _pm_awbc_tshark2_init(void *dst_awbc, void *src_awbc, void* param1, void* param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_awbc_param_tshark2 *dst_ptr = PNULL;
	struct sensor_awbc_param *src_ptr = NULL;
	struct isp_pm_block_header *header_ptr = PNULL;

	dst_ptr = (struct isp_awbc_param_tshark2*)dst_awbc;
	src_ptr = (struct sensor_awbc_param*)src_awbc;
	header_ptr = (struct isp_pm_block_header*)param1;

	memset((void*)&dst_ptr->cur,0x00,sizeof(dst_ptr->cur));

	dst_ptr->cur.alpha_bypass = src_ptr->alpha_bypass;
	dst_ptr->cur.alpha_value = src_ptr->alpha_val;
	dst_ptr->cur.buf_sel = src_ptr->buf_sel;
	dst_ptr->cur.bypass = header_ptr->bypass;

	dst_ptr->cur.gain.b = src_ptr->awbc_gain.b_gain;
	dst_ptr->cur.gain.gb = src_ptr->awbc_gain.gb_gain;
	dst_ptr->cur.gain.gr = src_ptr->awbc_gain.gr_gain;
	dst_ptr->cur.gain.r = src_ptr->awbc_gain.r_gain;

	dst_ptr->cur.gain_buff.b = src_ptr->awbc_gain_buf.b_gain;
	dst_ptr->cur.gain_buff.gb = src_ptr->awbc_gain_buf.gb_gain;
	dst_ptr->cur.gain_buff.gr = src_ptr->awbc_gain_buf.gr_gain;
	dst_ptr->cur.gain_buff.r = src_ptr->awbc_gain_buf.r_gain;

	dst_ptr->cur.gain_offset.b = src_ptr->awbc_offset.b_offset;
	dst_ptr->cur.gain_offset.gb = src_ptr->awbc_offset.gb_offset;
	dst_ptr->cur.gain_offset.gr = src_ptr->awbc_offset.gr_offset;
	dst_ptr->cur.gain_offset.r = src_ptr->awbc_offset.r_offset;

	dst_ptr->cur.gain_offset_buff.b = src_ptr->awbc_offset_buf.b_offset;
	dst_ptr->cur.gain_offset_buff.gb = src_ptr->awbc_offset_buf.gb_offset;
	dst_ptr->cur.gain_offset_buff.gr = src_ptr->awbc_offset_buf.gr_offset;
	dst_ptr->cur.gain_offset_buff.r = src_ptr->awbc_offset_buf.r_offset;

	dst_ptr->cur.thrd.b = src_ptr->awbc_thr.b_thr;
	dst_ptr->cur.thrd.r = src_ptr->awbc_thr.r_thr;
	dst_ptr->cur.thrd.g = src_ptr->awbc_thr.g_thr;

	header_ptr->is_update = 1;

	return rtn;
}

static isp_s32 _pm_awbc_tshark2_set_param(void *awbc_param, isp_u32 cmd, void* param_ptr0, void* param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_awbc_param_tshark2 *dst_ptr = (struct isp_awbc_param_tshark2*)awbc_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header*)param_ptr1;

	header_ptr->is_update = 1;

	dst_ptr->cur.bypass = *((isp_u32*)param_ptr0);

	return rtn;
}

static isp_s32 _pm_awbc_tshark2_get_param(void *awbc_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_awbc_param_tshark2 *awbc_ptr = (struct isp_awbc_param_tshark2*)awbc_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag =(isp_u32*)rtn_param1;

	switch (cmd) {
		case ISP_PM_BLK_ISP_SETTING:
		{
			param_data_ptr->id = ISP_BLK_AWBC_V1;
			param_data_ptr->cmd = ISP_PM_BLK_ISP_SETTING;
			param_data_ptr->data_ptr = (void*)&awbc_ptr->cur;
			param_data_ptr->data_size = sizeof(awbc_ptr->cur);
			*update_flag = 0;
		}
*/

static isp_s32 _pm_awb_init_v1(void *dst_awb_v1, void *src_awb_v1, void* param1, void* param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_awb_param_v1 *dst_ptr = (struct isp_awb_param_v1*)dst_awb_v1;
	struct sensor_awb_param *src_ptr = (struct sensor_awb_param *)src_awb_v1;
	struct isp_pm_block_header* awb_header_ptr = (struct isp_pm_block_header*)param1;
	UNUSED(param2);

	dst_ptr->ct_value = 5000;
	memset((void*)&dst_ptr->cur,0x00,sizeof(dst_ptr->cur));
	/*AWBM*/ //fuzk temp
	dst_ptr->cur.awbm_bypass = 1;//awb_header_ptr->bypass;
	//dst_ptr->cur.block_size.width = 1632 / 16; //src_ptr->awbm.win_size.w;
	//dst_ptr->cur.block_size.height = 1224 / 16;//src_ptr->awbm.win_size.h;
	/*AWBC*/
	dst_ptr->cur.awbc_bypass = awb_header_ptr->bypass;
	dst_ptr->cur.alpha_bypass = 1;//src_ptr->awbc.alpha_bypass;
	dst_ptr->cur.gain.r = 0x6CD;//src_ptr->awbc.awbc_gain.r_gain;
	dst_ptr->cur.gain.gr = 0x400;//src_ptr->awbc.awbc_gain.gr_gain;
	dst_ptr->cur.gain.gb = 0x400;//src_ptr->awbc.awbc_gain.gb_gain;
	dst_ptr->cur.gain.b = 0x600;//src_ptr->awbc.awbc_gain.b_gain;
	dst_ptr->cur.thrd.r = 0x3ff;//src_ptr->awbc.awbc_thr.r_thr;
	dst_ptr->cur.thrd.g = 0x3ff;//src_ptr->awbc.awbc_thr.g_thr;
	dst_ptr->cur.thrd.b = 0x3ff;//src_ptr->awbc.awbc_thr.b_thr;
	awb_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_awb_set_param_v1(void *awb_v1_param, isp_u32 cmd, void* param_ptr0, void* param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_awb_param_v1 *dst_ptr = (struct isp_awb_param_v1*)awb_v1_param;
	struct isp_pm_block_header * awb_header_ptr = (struct isp_pm_block_header*)param_ptr1;

	awb_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_AWBC:
	{
		struct isp_awbc_cfg *cfg_ptr = (struct isp_awbc_cfg*)param_ptr0;
		dst_ptr->cur.gain.r = cfg_ptr->r_gain;
		dst_ptr->cur.gain.gr = cfg_ptr->g_gain;
		dst_ptr->cur.gain.gb = cfg_ptr->g_gain;
		dst_ptr->cur.gain.b = cfg_ptr->b_gain;
		dst_ptr->cur.gain_offset.r = cfg_ptr->r_offset;
		dst_ptr->cur.gain_offset.gr = cfg_ptr->g_offset;
		dst_ptr->cur.gain_offset.gb = cfg_ptr->g_offset;
		dst_ptr->cur.gain_offset.b = cfg_ptr->b_offset;
	}
	break;

	case ISP_PM_BLK_AWBM:
		/*struct isp_awbm_cfg *cfg_ptr = (struct isp_awbm_cfg*)param_ptr0;
		dst_ptr->cur.offset_x = cfg_ptr->win_start.x;
		dst_ptr->cur.offset_y = cfg_ptr->win_start.y;
		dst_ptr->cur.win_w = cfg_ptr->blk_size.w;
		dst_ptr->cur.win_h = cfg_ptr->blk_size.h;*/
	break;

	case ISP_PM_BLK_AWB_CT:
		dst_ptr->ct_value = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_AWBC_BYPASS:
		//dst_ptr->cur.awbc_bypass = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_AWBM_BYPASS:
	break;

	case ISP_PM_BLK_MEMORY_INIT:
	{
		isp_u32 i = 0;
		struct isp_pm_memory_init_param *memory_ptr = (struct isp_pm_memory_init_param*)param_ptr0;
		for (i = 0; i < memory_ptr->size_info.count_lines; ++i) {
			dst_ptr->awb_statistics[i].data_ptr = ((isp_u8*)memory_ptr->buffer.data_ptr + memory_ptr->size_info.pitch * i);
			dst_ptr->awb_statistics[i].size = memory_ptr->size_info.pitch;
		}
	}
	break;

	default:
		awb_header_ptr->is_update = ISP_ZERO;
	break;
	}

	return rtn;
}

static isp_s32 _pm_awb_get_param_v1(void *awb_v1_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_awb_param_v1*awb_param_ptr = (struct isp_awb_param_v1*)awb_v1_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_AWB_V1;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &awb_param_ptr->cur;
		param_data_ptr->data_size = sizeof(awb_param_ptr->cur);
		*update_flag = 0;
	break;

	case ISP_PM_BLK_AWBM:
	break;

	case ISP_PM_BLK_AWBC_BYPASS:
	break;

	case ISP_PM_BLK_AWB_CT:
		param_data_ptr->data_ptr = &awb_param_ptr->ct_value;
		param_data_ptr->data_size = sizeof(awb_param_ptr->ct_value);
	break;

	case ISP_PM_BLK_AWBM_STATISTIC:
		param_data_ptr->data_ptr = (void*)&awb_param_ptr->stat;
		param_data_ptr->data_size = sizeof(awb_param_ptr->stat);
	break;

	default:
	break;
	}

	return rtn;
}

static isp_s32 _pm_awb_init_v2(void *dst_awb_v1, void *src_awb_v1, void* param1, void* param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_awb_param_v2 *dst_ptr = (struct isp_awb_param_v2*)dst_awb_v1;
	struct sensor_awb_param *src_ptr = (struct sensor_awb_param *)src_awb_v1;
	struct isp_pm_block_header* awb_header_ptr = (struct isp_pm_block_header*)param1;
	UNUSED(param2);

	dst_ptr->ct_value = 5000;
	memset((void*)&dst_ptr->cur,0x00,sizeof(dst_ptr->cur));
	/*AWBM*/ //fuzk temp
	dst_ptr->cur.awbm_bypass = awb_header_ptr->bypass;
	//dst_ptr->cur.block_size.width = 1632 / 16; //src_ptr->awbm.win_size.w;
	//dst_ptr->cur.block_size.height = 1224 / 16;//src_ptr->awbm.win_size.h;
	/*AWBC*/
	dst_ptr->cur.awbc_bypass = awb_header_ptr->bypass;
	dst_ptr->cur.alpha_bypass = src_ptr->awbc.alpha_bypass;
	dst_ptr->cur.gain.r = 0x6CD;//src_ptr->awbc.awbc_gain.r_gain;
	dst_ptr->cur.gain.gr = 0x400;//src_ptr->awbc.awbc_gain.gr_gain;
	dst_ptr->cur.gain.gb = 0x400;//src_ptr->awbc.awbc_gain.gb_gain;
	dst_ptr->cur.gain.b = 0x600;//src_ptr->awbc.awbc_gain.b_gain;
	dst_ptr->cur.thrd.r = 0x3ff;//src_ptr->awbc.awbc_thr.r_thr;
	dst_ptr->cur.thrd.g = 0x3ff;//src_ptr->awbc.awbc_thr.g_thr;
	dst_ptr->cur.thrd.b = 0x3ff;//src_ptr->awbc.awbc_thr.b_thr;
	awb_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_awb_set_param_v2(void *awb_v1_param, isp_u32 cmd, void* param_ptr0, void* param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_awb_param_v2 *dst_ptr = (struct isp_awb_param_v2*)awb_v1_param;
	struct isp_pm_block_header * awb_header_ptr = (struct isp_pm_block_header*)param_ptr1;

	awb_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_AWBC:
	{
		struct isp_awbc_cfg *cfg_ptr = (struct isp_awbc_cfg*)param_ptr0;
		dst_ptr->cur.gain.r = cfg_ptr->r_gain;
		dst_ptr->cur.gain.gr = cfg_ptr->g_gain;
		dst_ptr->cur.gain.gb = cfg_ptr->g_gain;
		dst_ptr->cur.gain.b = cfg_ptr->b_gain;
		dst_ptr->cur.gain_offset.r = cfg_ptr->r_offset;
		dst_ptr->cur.gain_offset.gr = cfg_ptr->g_offset;
		dst_ptr->cur.gain_offset.gb = cfg_ptr->g_offset;
		dst_ptr->cur.gain_offset.b = cfg_ptr->b_offset;
	}
	break;

	case ISP_PM_BLK_AWBM:
		/*struct isp_awbm_cfg *cfg_ptr = (struct isp_awbm_cfg*)param_ptr0;
		dst_ptr->cur.offset_x = cfg_ptr->win_start.x;
		dst_ptr->cur.offset_y = cfg_ptr->win_start.y;
		dst_ptr->cur.win_w = cfg_ptr->blk_size.w;
		dst_ptr->cur.win_h = cfg_ptr->blk_size.h;*/
	break;

	case ISP_PM_BLK_AWB_CT:
		dst_ptr->ct_value = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_AWBC_BYPASS:
		//dst_ptr->cur.awbc_bypass = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_AWBM_BYPASS:
	break;

	case ISP_PM_BLK_MEMORY_INIT:
	{
		isp_u32 i = 0;
		struct isp_pm_memory_init_param *memory_ptr = (struct isp_pm_memory_init_param*)param_ptr0;
		for (i = 0; i < memory_ptr->size_info.count_lines; ++i) {
			dst_ptr->awb_statistics[i].data_ptr = ((isp_u8*)memory_ptr->buffer.data_ptr + memory_ptr->size_info.pitch * i);
			dst_ptr->awb_statistics[i].size = memory_ptr->size_info.pitch;
		}
	}
	break;

	default:
		awb_header_ptr->is_update = ISP_ZERO;
	break;
	}

	return rtn;
}

static isp_s32 _pm_awb_get_param_v2(void *awb_v1_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_awb_param_v2*awb_param_ptr = (struct isp_awb_param_v2*)awb_v1_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_AWB_V1;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &awb_param_ptr->cur;
		param_data_ptr->data_size = sizeof(awb_param_ptr->cur);
		*update_flag = 0;
	break;

	case ISP_PM_BLK_AWBM:
	break;

	case ISP_PM_BLK_AWBC_BYPASS:
	break;

	case ISP_PM_BLK_AWB_CT:
		param_data_ptr->data_ptr = &awb_param_ptr->ct_value;
		param_data_ptr->data_size = sizeof(awb_param_ptr->ct_value);
	break;

	case ISP_PM_BLK_AWBM_STATISTIC:
		param_data_ptr->data_ptr = (void*)&awb_param_ptr->stat;
		param_data_ptr->data_size = sizeof(awb_param_ptr->stat);
	break;

	default:
	break;
	}

	return rtn;
}

static isp_s32 _pm_rgb_ae_init(void *dst_rgb_aem, void *src_rgb_aem, void* param1, void* param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_rgb_ae_param *dst_ptr = (struct isp_rgb_ae_param*)dst_rgb_aem;
	struct sensor_rgb_aem_param *src_ptr = (struct sensor_rgb_aem_param*)src_rgb_aem;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header*)param1;
	UNUSED(param2);

	memset((void*)&dst_ptr->cur,0x00,sizeof(dst_ptr->cur));
	dst_ptr->cur.bypass = header_ptr->bypass;
	header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_rgb_ae_set_param(void *rgb_aem_param, isp_u32 cmd, void* param_ptr0, void* param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_rgb_ae_param *dst_ptr = (struct isp_rgb_ae_param*)rgb_aem_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header*)param_ptr1;
	UNUSED(cmd);

	header_ptr->is_update = ISP_ONE;
	dst_ptr->cur.bypass = *((isp_u32*)param_ptr0);

	return rtn;
}

static isp_s32 _pm_rgb_ae_get_param(void *rgb_aem_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_rgb_ae_param *rgb_aem_ptr = (struct isp_rgb_ae_param*)rgb_aem_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag =(isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_AE_V1;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
#ifdef AE_WORK_MOD_V2
		rgb_aem_ptr->cur.bypass = 0;
		rgb_aem_ptr->cur.mode = 1;
		rgb_aem_ptr->cur.shift = 0;
		param_data_ptr->data_ptr = (void*)&rgb_aem_ptr->cur;
		param_data_ptr->data_size = sizeof(rgb_aem_ptr->cur);
#else
		param_data_ptr->data_ptr = NULL;
		param_data_ptr->data_size = 0;
#endif
		*update_flag = 0;
	break;

	case ISP_PM_BLK_AE_BYPASS:
		param_data_ptr->data_ptr = (void*)&rgb_aem_ptr->cur.bypass;
		param_data_ptr->data_size = sizeof(rgb_aem_ptr->cur.bypass);
	break;

	case ISP_PM_BLK_AEM_STATISTIC:
		param_data_ptr->data_ptr = (void*)&rgb_aem_ptr->stat;
		param_data_ptr->data_size = sizeof(rgb_aem_ptr->stat);
	break;

	default:
	break;
	}

	return rtn;
}

static isp_u32 _pm_bpc_convert_param(void *dst_param, isp_u32 strength_level, isp_u32 mode_flag)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_s32 i = 0;
	struct isp_bpc_param_v1 *dst_ptr = (struct isp_bpc_param_v1*)dst_param;
	struct sensor_bpc_level* bpc_param;
	struct nr_param_ptr *nr_ptr;
	
	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->multi_mode_enable) {
		bpc_param = (struct sensor_bpc_level*)(dst_ptr->bpc_ptr);
	} else {
		nr_ptr = (struct nr_param_ptr*)(dst_ptr->bpc_ptr);
		if (!nr_ptr) {
			return rtn;
		}

		switch(mode_flag) {
			case ISP_NR_COMMON_MODE:
				bpc_param = (struct sensor_bpc_level*)(nr_ptr->common_param_ptr);
			break;
			case ISP_NR_CAP_MODE:
				bpc_param = (struct sensor_bpc_level*)(nr_ptr->capture_param_ptr);
			break;
			case ISP_NR_VIDEO_MODE:
				bpc_param = (struct sensor_bpc_level*)(nr_ptr->video_param_ptr);
			break;
			case ISP_NR_PANO_MODE:
				bpc_param = (struct sensor_bpc_level*)(nr_ptr->pano_param_ptr);
			break;
			default :
				bpc_param = (struct sensor_bpc_level*)(nr_ptr->common_param_ptr);
			break;
		}
	}

	strength_level = PM_CLIP(strength_level, 0, SENSOR_SMART_LEVEL_NUM - 1);
	if (bpc_param != NULL) {
		dst_ptr->cur.pvd_bypass = bpc_param[strength_level].bpc_pvd.bypass_pvd;
		dst_ptr->cur.cntr_threshold = bpc_param[strength_level].bpc_pvd.cntr_theshold;

		dst_ptr->cur.kmin = bpc_param[strength_level].bpc_rules.k_val.min;
		dst_ptr->cur.kmax = bpc_param[strength_level].bpc_rules.k_val.max;
		dst_ptr->cur.ktimes = bpc_param[strength_level].bpc_rules.ktimes;
		dst_ptr->cur.delta = bpc_param[strength_level].bpc_rules.delt34;
		dst_ptr->cur.safe_factor = bpc_param[strength_level].bpc_rules.safe_factor;

		dst_ptr->cur.flat_factor = bpc_param[strength_level].bpc_flat.flat_factor;
		dst_ptr->cur.spike_coeff = bpc_param[strength_level].bpc_flat.spike_coeff;
		dst_ptr->cur.dead_coeff = bpc_param[strength_level].bpc_flat.dead_coeff;
		for(i = 0x00; i < 8; i++) {
			dst_ptr->cur.lut_level[i] = bpc_param[strength_level].bpc_flat.lut_level[i];
			dst_ptr->cur.slope_k[i] = bpc_param[strength_level].bpc_flat.slope_k[i];
		}
		for(i = 0x00; i < 8; i++) {
			dst_ptr->cur.intercept_b[i] = bpc_param[strength_level].bpc_flat.intercept_b[i];
		}

		dst_ptr->cur.bad_map_hw_fifo_clr_en = 0x00;
		dst_ptr->cur.bpc_map_fifo_clr = 0x00;
		dst_ptr->cur.bad_pixel_num = 0x00;
		dst_ptr->cur.bpc_map_addr_new = 0x00;
		dst_ptr->cur.bypass= bpc_param[strength_level].bypass;
	}

	return rtn;

}

static isp_s32 _pm_bpc_init_v1(void *dst_bpc_param, void *src_bpc_param, void* param1, void* param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i = 0x00;
	struct isp_bpc_param_v1 *dst_ptr =(struct isp_bpc_param_v1*)dst_bpc_param;
	struct sensor_bpc_param_v1 *src_ptr = (struct sensor_bpc_param_v1*)src_bpc_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header*)param1;
	UNUSED(param2);

	/*mode need sensor_bpc_param struct to add.Now,mode set zeros,
	map_addr must call _pm_bpc_set_param function for setting.*/
	memset((void*)&dst_ptr->cur,0x00,sizeof(dst_ptr->cur));

	dst_ptr->cur.bypass =  header_ptr->bypass;
	dst_ptr->cur.level = SENSOR_SMART_LEVEL_DEFAULT;
	dst_ptr->cur_level = SENSOR_SMART_LEVEL_DEFAULT;

	dst_ptr->cur.bpc_mode = src_ptr->bpc_comm.bpc_mode;
	dst_ptr->cur.mask_mode = src_ptr->bpc_comm.mask_mode;
	dst_ptr->cur.new_old_sel = 1;
	dst_ptr->cur.map_done_sel = src_ptr->bpc_comm.map_done_sel;

	dst_ptr->bpc_ptr = src_ptr->param_ptr;
	dst_ptr->multi_mode_enable = src_ptr->reserved[0];

	dst_ptr->cur.pvd_bypass = src_ptr->bpc_level.bpc_pvd.bypass_pvd;
	dst_ptr->cur.cntr_threshold = src_ptr->bpc_level.bpc_pvd.cntr_theshold;
	dst_ptr->cur.kmin = src_ptr->bpc_level.bpc_rules.k_val.min;
	dst_ptr->cur.kmax = src_ptr->bpc_level.bpc_rules.k_val.max;
	dst_ptr->cur.ktimes = src_ptr->bpc_level.bpc_rules.ktimes;
	dst_ptr->cur.delta = src_ptr->bpc_level.bpc_rules.delt34;
	dst_ptr->cur.safe_factor = src_ptr->bpc_level.bpc_rules.safe_factor;
	dst_ptr->cur.flat_factor = src_ptr->bpc_level.bpc_flat.flat_factor;
	dst_ptr->cur.spike_coeff = src_ptr->bpc_level.bpc_flat.spike_coeff;
	dst_ptr->cur.dead_coeff = src_ptr->bpc_level.bpc_flat.dead_coeff;
	for(i = 0x00; i < 8; i++) {
		dst_ptr->cur.lut_level[i] = src_ptr->bpc_level.bpc_flat.lut_level[i];
		dst_ptr->cur.slope_k[i] = src_ptr->bpc_level.bpc_flat.slope_k[i];
	}
	for(i = 0x00; i < 8; i++) {
		dst_ptr->cur.intercept_b[i] = src_ptr->bpc_level.bpc_flat.intercept_b[i];
	}
	dst_ptr->cur.bad_map_hw_fifo_clr_en = 0x00;
	dst_ptr->cur.bpc_map_fifo_clr = 0x00;
	dst_ptr->cur.bad_pixel_num = 0x00;
	dst_ptr->cur.bpc_map_addr_new = 0x00;

	rtn=_pm_bpc_convert_param(dst_ptr, SENSOR_SMART_LEVEL_DEFAULT, ISP_NR_COMMON_MODE);
	dst_ptr->cur.bypass |=  header_ptr->bypass;
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("ISP_PM_BPC_CONVERT_PARAM: error!");
		return rtn;
	}

	header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_bpc_set_param_v1(void *bpc_param, isp_u32 cmd, void* param_ptr0, void* param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_bpc_param_v1 *bpc_ptr = (struct isp_bpc_param_v1*)bpc_param;
	struct isp_pm_block_header *bpc_header_ptr = (struct isp_pm_block_header*)param_ptr1;

	bpc_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_BPC:
		//TODO:
	break;

	case ISP_PM_BLK_BPC_BYPASS:
		bpc_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_BPC_MODE:
	{
		isp_u32 mode = *((isp_u32*)param_ptr0);
		bpc_ptr->cur.bpc_mode = mode;
	}
	 break;

	 case ISP_PM_BLK_BPC_THRD:
	 	//TODO:
	 break;

	 case ISP_PM_BLK_BPC_MAP_ADDR:
	 {
	 	/*need caller this enum to set this value. */
		intptr_t map_addr = *(intptr_t*)param_ptr0;
		bpc_ptr->cur.bpc_map_addr_new = map_addr;
	 }
	 break;

	case ISP_PM_BLK_SMART_SETTING:
	{
		struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;
		struct isp_range val_range = {0};
		isp_u32 cur_level = 0;

		bpc_header_ptr->is_update = ISP_ZERO;
		val_range.min = 0;
		val_range.max = 255;

		rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_VALUE);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !");
			return rtn;
		}

		cur_level = (isp_u32)block_result->component[0].fix_data[0];

		if (cur_level != bpc_ptr->cur_level || nr_tool_flag[1] || block_result->mode_flag_changed) {
			bpc_ptr->cur_level = cur_level;
			bpc_ptr->cur.level = cur_level;
			bpc_header_ptr->is_update = ISP_ONE;
			nr_tool_flag[1] = 0;
			block_result->mode_flag_changed = 0;
			rtn=_pm_bpc_convert_param(bpc_ptr, bpc_ptr->cur_level, block_result->mode_flag);
			bpc_ptr->cur.bypass |=  bpc_header_ptr->bypass;

			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("ISP_PM_BPC_CONVERT_PARAM: error!");
				return rtn;
			}
		}
	}
	break;

	default:
		bpc_header_ptr->is_update = ISP_ZERO;
	break;
	}

	ISP_LOGV("ISP_SMART: cmd=%d, update=%d, cur_level=%d", cmd, bpc_header_ptr->is_update,
					bpc_ptr->cur_level);

	return rtn;
}
static isp_s32 _pm_bpc_get_param_v1(void *bpc_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_bpc_param_v1 *bpc_ptr = (struct isp_bpc_param_v1*)bpc_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_BPC_V1;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void*)&bpc_ptr->cur;
		param_data_ptr->data_size = sizeof(bpc_ptr->cur);
		*update_flag = 0;
	break;

	case ISP_PM_BLK_BPC_BYPASS:
		param_data_ptr->data_ptr = (void*)&bpc_ptr->cur.bypass;
		param_data_ptr->data_size = sizeof(bpc_ptr->cur.bypass);
	break;

	default:
	break;
	}

	return rtn;
}

static isp_u32 _pm_bl_nr_convert_param(void *dst_param, isp_u32 strength_level, isp_u32 mode_flag)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_s32 i = 0;
	struct isp_bl_nr_param_v1 *dst_ptr = (struct isp_bl_nr_param_v1*)dst_param;
	struct sensor_bdn_level* bdn_param;
	struct nr_param_ptr *nr_ptr;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->multi_mode_enable) {
		bdn_param = (struct sensor_bdn_level*)(dst_ptr->bdn_ptr);
	} else {
		nr_ptr = (struct nr_param_ptr*)(dst_ptr->bdn_ptr);
		if (!nr_ptr) {
			return rtn;
		}

		switch(mode_flag) {
			case ISP_NR_COMMON_MODE:
				bdn_param = (struct sensor_bdn_level*)(nr_ptr->common_param_ptr);
			break;
			case ISP_NR_CAP_MODE:
				bdn_param = (struct sensor_bdn_level*)(nr_ptr->capture_param_ptr);
			break;
			case ISP_NR_VIDEO_MODE:
				bdn_param = (struct sensor_bdn_level*)(nr_ptr->video_param_ptr);
			break;
			case ISP_NR_PANO_MODE:
				bdn_param = (struct sensor_bdn_level*)(nr_ptr->pano_param_ptr);
			break;
			default :
				bdn_param = (struct sensor_bdn_level*)(nr_ptr->common_param_ptr);
			break;
		}
	}

	strength_level = PM_CLIP(strength_level, 0, SENSOR_SMART_LEVEL_NUM - 1);
	if (bdn_param != NULL) {
		for (i = 0; i < 10; i++) {
			dst_ptr->cur.dis[i][0] = (bdn_param[strength_level].diswei_tab[i][0]&0xFF) | ((bdn_param[strength_level].diswei_tab[i][2]&0xFF) << 8)
				| ((bdn_param[strength_level].diswei_tab[i][4]&0xFF) << 16) | ((bdn_param[strength_level].diswei_tab[i][8]&0xFF) << 24);
			dst_ptr->cur.dis[i][1] = (bdn_param[strength_level].diswei_tab[i][10]&0xFF) | ((bdn_param[strength_level].diswei_tab[i][18]&0xFF) << 8);
		}

		for (i = 0; i < 10; i++) {
			dst_ptr->cur.ran[i][0] = (bdn_param[strength_level].ranwei_tab[i][0]&0xFF) | ((bdn_param[strength_level].ranwei_tab[i][1]&0xFF) << 8)
								| ((bdn_param[strength_level].ranwei_tab[i][2]&0xFF) << 16) | ((bdn_param[strength_level].ranwei_tab[i][3]&0xFF) << 24);
			dst_ptr->cur.ran[i][1] = (bdn_param[strength_level].ranwei_tab[i][4]&0xFF) | ((bdn_param[strength_level].ranwei_tab[i][5]&0xFF) << 8)
								| ((bdn_param[strength_level].ranwei_tab[i][6]&0xFF) << 16) | ((bdn_param[strength_level].ranwei_tab[i][7]&0xFF) << 24);
			dst_ptr->cur.ran[i][2] = (bdn_param[strength_level].ranwei_tab[i][8]&0xFF) | ((bdn_param[strength_level].ranwei_tab[i][9]&0xFF) << 8)
								| ((bdn_param[strength_level].ranwei_tab[i][10]&0xFF) << 16) | ((bdn_param[strength_level].ranwei_tab[i][11]&0xFF) << 24);
			dst_ptr->cur.ran[i][3] = (bdn_param[strength_level].ranwei_tab[i][12]&0xFF) | ((bdn_param[strength_level].ranwei_tab[i][13]&0xFF) << 8)
								| ((bdn_param[strength_level].ranwei_tab[i][14]&0xFF) << 16) | ((bdn_param[strength_level].ranwei_tab[i][15]&0xFF) << 24);
			dst_ptr->cur.ran[i][4] = (bdn_param[strength_level].ranwei_tab[i][16]&0xFF) | ((bdn_param[strength_level].ranwei_tab[i][17]&0xFF) << 8)
								| ((bdn_param[strength_level].ranwei_tab[i][18]&0xFF) << 16) | ((bdn_param[strength_level].ranwei_tab[i][19]&0xFF) << 24);
			dst_ptr->cur.ran[i][5] = (bdn_param[strength_level].ranwei_tab[i][20]&0xFF) | ((bdn_param[strength_level].ranwei_tab[i][21]&0xFF) << 8)
								| ((bdn_param[strength_level].ranwei_tab[i][22]&0xFF) << 16) | ((bdn_param[strength_level].ranwei_tab[i][23]&0xFF) << 24);
			dst_ptr->cur.ran[i][6] = (bdn_param[strength_level].ranwei_tab[i][24]&0xFF) | ((bdn_param[strength_level].ranwei_tab[i][25]&0xFF) << 8)
								| ((bdn_param[strength_level].ranwei_tab[i][26]&0xFF) << 16) | ((bdn_param[strength_level].ranwei_tab[i][27]&0xFF) << 24);
			dst_ptr->cur.ran[i][7] = (bdn_param[strength_level].ranwei_tab[i][28]&0xFF) | ((bdn_param[strength_level].ranwei_tab[i][29]&0xFF) << 8)
								| ((bdn_param[strength_level].ranwei_tab[i][30]&0xFF) << 16);
		}

		dst_ptr->cur.radial_bypass = bdn_param[strength_level].radial_bypass;
		dst_ptr->cur.addback = bdn_param[strength_level].addback;

		dst_ptr->cur.offset_x = bdn_param[strength_level].center.x;
		dst_ptr->cur.offset_y = bdn_param[strength_level].center.y;
		dst_ptr->cur.squ_x2 = bdn_param[strength_level].delta_sqr.x;
		dst_ptr->cur.squ_y2 = bdn_param[strength_level].delta_sqr.y;
		dst_ptr->cur.coef = bdn_param[strength_level].coeff.p1;
		dst_ptr->cur.coef2 = bdn_param[strength_level].coeff.p2;
		dst_ptr->cur.offset = bdn_param[strength_level].dis_sqr_offset;

		dst_ptr->cur.start_pos_x = 0x0;
		dst_ptr->cur.start_pos_y = 0x0;
		dst_ptr->cur.bypass = bdn_param[strength_level].bypass;
	}

	return rtn;

}

static isp_s32 _pm_bl_nr_init_v1(void *dst_bdn_param, void *src_bdn_param, void* param1, void* param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i = 0x00;
	struct isp_bl_nr_param_v1 *dst_ptr = (struct isp_bl_nr_param_v1 *)dst_bdn_param;
	struct sensor_bdn_param *src_ptr = (struct sensor_bdn_param *)src_bdn_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	memset((void*)&dst_ptr->cur,0x00,sizeof(dst_ptr->cur));
	dst_ptr->cur.bypass =  header_ptr->bypass;
	dst_ptr->cur.ran_level = SENSOR_SMART_LEVEL_DEFAULT;
	dst_ptr->cur.dis_level = SENSOR_SMART_LEVEL_DEFAULT;

	dst_ptr->bdn_ptr = src_ptr->param_ptr;
	dst_ptr->multi_mode_enable = src_ptr->reserved[0];

	for (i = 0; i < 10; i++) {
		dst_ptr->cur.dis[i][0] = (src_ptr->bdn_level.diswei_tab[i][0]&0xFF) | ((src_ptr->bdn_level.diswei_tab[i][2]&0xFF) << 8)
			| ((src_ptr->bdn_level.diswei_tab[i][4]&0xFF) << 16) | ((src_ptr->bdn_level.diswei_tab[i][8]&0xFF) << 24);
		dst_ptr->cur.dis[i][1] = (src_ptr->bdn_level.diswei_tab[i][10]&0xFF) | ((src_ptr->bdn_level.diswei_tab[i][18]&0xFF) << 8);
	}
	for (i = 0; i < 10; i++) {
		dst_ptr->cur.ran[i][0] = (src_ptr->bdn_level.ranwei_tab[i][0]&0xFF) | ((src_ptr->bdn_level.ranwei_tab[i][1]&0xFF) << 8)
							| ((src_ptr->bdn_level.ranwei_tab[i][2]&0xFF) << 16) | ((src_ptr->bdn_level.ranwei_tab[i][3]&0xFF) << 24);
		dst_ptr->cur.ran[i][1] = (src_ptr->bdn_level.ranwei_tab[i][4]&0xFF) | ((src_ptr->bdn_level.ranwei_tab[i][5]&0xFF) << 8)
							| ((src_ptr->bdn_level.ranwei_tab[i][6]&0xFF) << 16) | ((src_ptr->bdn_level.ranwei_tab[i][7]&0xFF) << 24);
		dst_ptr->cur.ran[i][2] = (src_ptr->bdn_level.ranwei_tab[i][8]&0xFF) | ((src_ptr->bdn_level.ranwei_tab[i][9]&0xFF) << 8)
							| ((src_ptr->bdn_level.ranwei_tab[i][10]&0xFF) << 16) | ((src_ptr->bdn_level.ranwei_tab[i][11]&0xFF) << 24);
		dst_ptr->cur.ran[i][3] = (src_ptr->bdn_level.ranwei_tab[i][12]&0xFF) | ((src_ptr->bdn_level.ranwei_tab[i][13]&0xFF) << 8)
							| ((src_ptr->bdn_level.ranwei_tab[i][14]&0xFF) << 16) | ((src_ptr->bdn_level.ranwei_tab[i][15]&0xFF) << 24);
		dst_ptr->cur.ran[i][4] = (src_ptr->bdn_level.ranwei_tab[i][16]&0xFF) | ((src_ptr->bdn_level.ranwei_tab[i][17]&0xFF) << 8)
							| ((src_ptr->bdn_level.ranwei_tab[i][18]&0xFF) << 16) | ((src_ptr->bdn_level.ranwei_tab[i][19]&0xFF) << 24);
		dst_ptr->cur.ran[i][5] = (src_ptr->bdn_level.ranwei_tab[i][20]&0xFF) | ((src_ptr->bdn_level.ranwei_tab[i][21]&0xFF) << 8)
							| ((src_ptr->bdn_level.ranwei_tab[i][22]&0xFF) << 16) | ((src_ptr->bdn_level.ranwei_tab[i][23]&0xFF) << 24);
		dst_ptr->cur.ran[i][6] = (src_ptr->bdn_level.ranwei_tab[i][24]&0xFF) | ((src_ptr->bdn_level.ranwei_tab[i][25]&0xFF) << 8)
							| ((src_ptr->bdn_level.ranwei_tab[i][26]&0xFF) << 16) | ((src_ptr->bdn_level.ranwei_tab[i][27]&0xFF) << 24);
		dst_ptr->cur.ran[i][7] = (src_ptr->bdn_level.ranwei_tab[i][28]&0xFF) | ((src_ptr->bdn_level.ranwei_tab[i][29]&0xFF) << 8)
							| ((src_ptr->bdn_level.ranwei_tab[i][30]&0xFF) << 16);
	}
	dst_ptr->cur.radial_bypass = src_ptr->bdn_level.radial_bypass;
	dst_ptr->cur.addback = src_ptr->bdn_level.addback;
	dst_ptr->cur.offset_x = src_ptr->bdn_level.center.x;
	dst_ptr->cur.offset_y = src_ptr->bdn_level.center.y;
	dst_ptr->cur.squ_x2 = src_ptr->bdn_level.delta_sqr.x;
	dst_ptr->cur.squ_y2 = src_ptr->bdn_level.delta_sqr.y;
	dst_ptr->cur.coef = src_ptr->bdn_level.coeff.p1;
	dst_ptr->cur.coef2 = src_ptr->bdn_level.coeff.p2;
	dst_ptr->cur.offset = src_ptr->bdn_level.dis_sqr_offset;
	dst_ptr->cur.start_pos_x = 0x0;
	dst_ptr->cur.start_pos_y = 0x0;

	rtn=_pm_bl_nr_convert_param(dst_ptr, SENSOR_SMART_LEVEL_DEFAULT, ISP_NR_COMMON_MODE);
	dst_ptr->cur.bypass |=  header_ptr->bypass;
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("ISP_PM_BDN_CONVERT_PARAM: error!");
		return rtn;
	}

	header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_bl_nr_set_param_v1(void *bdn_param, isp_u32 cmd, void* param_ptr0, void* param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_bl_nr_param_v1 *bdn_ptr = (struct isp_bl_nr_param_v1*)bdn_param;
	struct isp_pm_block_header *bdn_header_ptr = (struct isp_pm_block_header*)param_ptr1;

	bdn_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_BDN_BYPASS:
		bdn_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_BDN_RADIAL_BYPASS:
	{
		isp_u32 bypass = *((isp_u32*)param_ptr0);
		bdn_ptr->cur.radial_bypass = bypass;
	}
	break;

	case ISP_PM_BLK_BDN_STRENGTH_LEVEL:
	{
		isp_u32 *level_ptr  = (isp_u32*)param_ptr0;
		bdn_ptr->cur.ran_level = level_ptr[0];
		bdn_ptr->cur.dis_level = level_ptr[1];
	}
	break;

	case ISP_PM_BLK_SMART_SETTING:
	{
		struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;
		struct isp_range val_range = {0};
		isp_u32 cur_level[2] = {0};

		bdn_header_ptr->is_update = ISP_ZERO;
		val_range.min = 0;
		val_range.max = 255;

		rtn = _pm_check_smart_param(block_result, &val_range, 2, ISP_SMART_Y_TYPE_VALUE);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !");
			return rtn;
		}

		cur_level[0] = (isp_u32)block_result->component[0].fix_data[0];
		cur_level[1] = (isp_u32)block_result->component[1].fix_data[0];

		if (cur_level[0] != bdn_ptr->cur.ran_level || cur_level[1] != bdn_ptr->cur.dis_level || nr_tool_flag[2] ||
			block_result->mode_flag_changed) {
			bdn_ptr->cur.ran_level = cur_level[0];
			bdn_ptr->cur.dis_level = cur_level[1];
			bdn_header_ptr->is_update = ISP_ONE;
			nr_tool_flag[2] = 0;
			block_result->mode_flag_changed = 0;
			rtn=_pm_bl_nr_convert_param(bdn_ptr, bdn_ptr->cur.ran_level, block_result->mode_flag);
			bdn_ptr->cur.bypass |=  bdn_header_ptr->bypass;
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("ISP_PM_BDN_CONVERT_PARAM: error!");
				return rtn;
			}
		}
	}
	break;

	default:
		bdn_header_ptr->is_update = ISP_ZERO;
	break;
	}

	ISP_LOGI("ISP_SMART: cmd=%d, update=%d, cur_level[0]=%d, cur_level[1]=%d", cmd, bdn_header_ptr->is_update,
					bdn_ptr->cur.ran_level, bdn_ptr->cur.dis_level);

	return rtn;
}
static isp_s32 _pm_bl_nr_get_param_v1(void *bdn_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_bl_nr_param_v1 *bdn_ptr = (struct isp_bl_nr_param_v1*)bdn_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_BL_NR_V1;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void*)&bdn_ptr->cur;
		param_data_ptr->data_size = sizeof(bdn_ptr->cur);
		*update_flag = 0;
	break;

	default:
	break;
	}

	return rtn;
}

static isp_u32 _pm_grgb_convert_param(void *dst_param, isp_u32 strength_level, isp_u32 mode_flag)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_s32 i = 0;
	struct isp_grgb_param_v1 *dst_grgb_ptr = (struct isp_grgb_param_v1*)dst_param;
	struct sensor_grgb_v1_level* grgb_param;
	struct nr_param_ptr *nr_ptr;

	if (SENSOR_MULTI_MODE_FLAG != dst_grgb_ptr->multi_mode_enable) {
		grgb_param = (struct sensor_grgb_v1_level*)(dst_grgb_ptr->grgb_ptr);
	} else {
		nr_ptr = (struct nr_param_ptr*)(dst_grgb_ptr->grgb_ptr);
		if (!nr_ptr) {
			return rtn;
		}

		switch(mode_flag) {
			case ISP_NR_COMMON_MODE:
				grgb_param = (struct sensor_grgb_v1_level*)(nr_ptr->common_param_ptr);
			break;
			case ISP_NR_CAP_MODE:
				grgb_param = (struct sensor_grgb_v1_level*)(nr_ptr->capture_param_ptr);
			break;
			case ISP_NR_VIDEO_MODE:
				grgb_param = (struct sensor_grgb_v1_level*)(nr_ptr->video_param_ptr);
			break;
			case ISP_NR_PANO_MODE:
				grgb_param = (struct sensor_grgb_v1_level*)(nr_ptr->pano_param_ptr);
			break;
			default :
				grgb_param = (struct sensor_grgb_v1_level*)(nr_ptr->common_param_ptr);
			break;
		}
	}
	strength_level = PM_CLIP(strength_level, 0, SENSOR_SMART_LEVEL_NUM - 1);

	if (grgb_param != NULL) {
		dst_grgb_ptr->cur.diff = grgb_param[strength_level].diff_thd;
		dst_grgb_ptr->cur.edge = grgb_param[strength_level].edge_thd;
		dst_grgb_ptr->cur.grid = grgb_param[strength_level].grid_thd;
		dst_grgb_ptr->cur.bypass = grgb_param[strength_level].bypass;
	}

	return rtn;
}

static isp_s32 _pm_grgb_init_v1(void *dst_grgb_param, void *src_grgb_param, void* param1, void* param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_grgb_param_v1 *dst_ptr = (struct isp_grgb_param_v1*)dst_grgb_param;
	struct sensor_grgb_v1_param *src_ptr = (struct sensor_grgb_v1_param*)src_grgb_param;
	struct isp_pm_block_header *grgb_header_ptr =(struct isp_pm_block_header*)param1;
	UNUSED(param2);

	dst_ptr->cur.bypass = grgb_header_ptr->bypass;
	dst_ptr->cur_level =SENSOR_SMART_LEVEL_DEFAULT;
	dst_ptr->grgb_ptr = src_ptr->param_ptr;
	dst_ptr->multi_mode_enable = src_ptr->reserved[0];

	dst_ptr->cur.diff = src_ptr->grgb_level.diff_thd;
	dst_ptr->cur.edge = src_ptr->grgb_level.edge_thd;
	dst_ptr->cur.grid = src_ptr->grgb_level.grid_thd;

	rtn = _pm_grgb_convert_param(dst_ptr,SENSOR_SMART_LEVEL_DEFAULT,ISP_NR_COMMON_MODE);
	dst_ptr->cur.bypass |= grgb_header_ptr->bypass;

	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("ISP_PM_grgb_CONVERT_PARAM: error!");
		return rtn;
	}

	grgb_header_ptr->is_update =ISP_ONE;

	return rtn;
}

static isp_s32 _pm_grgb_set_param_v1(void *grgb_param, isp_u32 cmd, void* param_ptr0, void* param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_grgb_param_v1 *dst_ptr = (struct isp_grgb_param_v1*)grgb_param;
	struct isp_pm_block_header *grgb_header_ptr = (struct isp_pm_block_header*)param_ptr1;
	grgb_header_ptr->is_update = ISP_ONE;

	switch(cmd){
	case ISP_PM_BLK_GRGB_BYPASS:
		dst_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_SMART_SETTING:
	{
		struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;
		struct isp_range val_range = {0};
		isp_u32 cur_level = 0;

		grgb_header_ptr->is_update = 0;
		val_range.min = 0;
		val_range.max = 255;

		rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_VALUE);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !");
			return rtn;
		}

		cur_level = (isp_u32)block_result->component[0].fix_data[0];
		if(cur_level != dst_ptr->cur_level || nr_tool_flag[3] || block_result->mode_flag_changed){
			dst_ptr->cur_level = cur_level;
			grgb_header_ptr->is_update = 1;
			nr_tool_flag[3] = 0;
			block_result->mode_flag_changed = 0;
			rtn = _pm_grgb_convert_param(dst_ptr,cur_level,block_result->mode_flag);
			dst_ptr->cur.bypass |= grgb_header_ptr->bypass;
			if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ISP_PM_GRGB_CONVERT_PARAM: error!");
			return rtn;
			}
		}
	}
	break;

	default:
		grgb_header_ptr->is_update = ISP_ZERO;
	break;
	}

	ISP_LOGI("ISP_SMART: cmd = %d, is_update = %d, cur_level=%ld", cmd, grgb_header_ptr->is_update, dst_ptr->cur_level);

	return rtn;
}
static isp_s32 _pm_grgb_get_param_v1(void *grgb_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_grgb_param_v1* grgb_ptr = (struct isp_grgb_param_v1*)grgb_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_GRGB_V1;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void*)&grgb_ptr->cur;
		param_data_ptr->data_size = sizeof(grgb_ptr->cur);
		*update_flag = 0;
	break;

	case ISP_PM_BLK_GRGB_BYPASS:
		param_data_ptr->data_ptr = (void*)&grgb_ptr->cur.bypass;
		param_data_ptr->data_size = sizeof(grgb_ptr->cur.bypass);
	break;

	default:
	break;
	}

	return rtn;
}


static isp_s32 _pm_rgb_gain2_init(void *dst_grgb2_param, void *src_grgb2_param, void* param1, void* param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_rgb_gain2_param *dst_ptr = (struct isp_rgb_gain2_param*)dst_grgb2_param;
	struct sensor_rgb_gain2_param *src_ptr = (struct sensor_rgb_gain2_param*)src_grgb2_param;
	struct isp_pm_block_header *grgb_header_ptr =(struct isp_pm_block_header*)param1;
	UNUSED(param2);

	dst_ptr->cur.bypass = grgb_header_ptr->bypass;
	dst_ptr->cur.g_gain = src_ptr->g_gain;
	dst_ptr->cur.r_gain = src_ptr->r_gain;
	dst_ptr->cur.b_gain = src_ptr->b_gain;
	dst_ptr->cur.b_offset = src_ptr->b_offset;
	dst_ptr->cur.g_offset = src_ptr->g_offset;
	dst_ptr->cur.r_offset = src_ptr->r_offset;
	grgb_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_rgb_gain2_set_param(void *grgb2_param, isp_u32 cmd, void* param_ptr0, void* param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_rgb_gain2_param *dst_ptr = (struct isp_rgb_gain2_param*)grgb2_param;
	struct isp_pm_block_header *grgb_header_ptr = (struct isp_pm_block_header*)param_ptr1;
	UNUSED(cmd);

	grgb_header_ptr->is_update = ISP_ONE;
	dst_ptr->cur.bypass = *((isp_u32*)param_ptr0);

	return rtn;
}
static isp_s32 _pm_rgb_gain2_get_param(void *grgb2_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_rgb_gain2_param* rgb_gain2_ptr = (struct isp_rgb_gain2_param*)grgb2_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_RGB_GAIN2;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void*)&rgb_gain2_ptr->cur;
		param_data_ptr->data_size = sizeof(rgb_gain2_ptr->cur);
		*update_flag = 0;
	break;

	case ISP_PM_BLK_GRGB_BYPASS:
		param_data_ptr->data_ptr = (void*)&rgb_gain2_ptr->cur.bypass;
		param_data_ptr->data_size = sizeof(rgb_gain2_ptr->cur.bypass);
	break;

	default:
	break;
	}

	return rtn;
}

static isp_u32 _pm_nlm_convert_param(void *dst_nlm_param, isp_u32 strength_level, isp_u32 mode_flag)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_s32 i = 0;
	struct isp_nlm_param_v1 *dst_nlm_ptr = (struct isp_nlm_param_v1 *)dst_nlm_param;
	void *addr = NULL;
	struct sensor_nlm_level* nlm_param;
	struct sensor_vst_level* vst_param;
	struct sensor_ivst_level* ivst_param;
	struct sensor_flat_offset_level* flat_offset_level;
	struct nr_param_ptr *nr_nlm_ptr;
	struct nr_param_ptr *nr_vst_ptr;
	struct nr_param_ptr *nr_ivst_ptr;
	struct nr_param_ptr *nr_flat_offset_ptr;

	if (SENSOR_MULTI_MODE_FLAG != dst_nlm_ptr->multi_mode_enable) {
		nlm_param = (struct sensor_nlm_level*)(dst_nlm_ptr->nlm_ptr);
		vst_param = (struct sensor_vst_level*)(dst_nlm_ptr->vst_ptr);
		ivst_param = (struct sensor_ivst_level*)(dst_nlm_ptr->ivst_ptr);
		flat_offset_level = (struct sensor_flat_offset_level*)(dst_nlm_ptr->flat_offset_ptr);
	} else {
		nr_nlm_ptr = (struct nr_param_ptr*)(dst_nlm_ptr->nlm_ptr);
		nr_vst_ptr = (struct nr_param_ptr*)(dst_nlm_ptr->vst_ptr);
		nr_ivst_ptr = (struct nr_param_ptr*)(dst_nlm_ptr->ivst_ptr);
		nr_flat_offset_ptr = (struct nr_param_ptr*)(dst_nlm_ptr->flat_offset_ptr);
		if (!nr_nlm_ptr || !nr_vst_ptr || !nr_ivst_ptr || !nr_flat_offset_ptr) {
			return rtn;
		}

		switch(mode_flag) {
			case ISP_NR_COMMON_MODE:
				nlm_param = (struct sensor_nlm_level*)(nr_nlm_ptr->common_param_ptr);
				vst_param = (struct sensor_vst_level*)(nr_vst_ptr->common_param_ptr);
				ivst_param = (struct sensor_ivst_level*)(nr_ivst_ptr->common_param_ptr);
				flat_offset_level = (struct sensor_flat_offset_level*)(nr_flat_offset_ptr->common_param_ptr);
			break;
			case ISP_NR_CAP_MODE:
				nlm_param = (struct sensor_nlm_level*)(nr_nlm_ptr->capture_param_ptr);
				vst_param = (struct sensor_vst_level*)(nr_vst_ptr->capture_param_ptr);
				ivst_param = (struct sensor_ivst_level*)(nr_ivst_ptr->capture_param_ptr);
				flat_offset_level = (struct sensor_flat_offset_level*)(nr_flat_offset_ptr->capture_param_ptr);

			break;
			case ISP_NR_VIDEO_MODE:
				nlm_param = (struct sensor_nlm_level*)(nr_nlm_ptr->video_param_ptr);
				vst_param = (struct sensor_vst_level*)(nr_vst_ptr->video_param_ptr);
				ivst_param = (struct sensor_ivst_level*)(nr_ivst_ptr->video_param_ptr);
				flat_offset_level = (struct sensor_flat_offset_level*)(nr_flat_offset_ptr->video_param_ptr);

			break;
			case ISP_NR_PANO_MODE:
				nlm_param = (struct sensor_nlm_level*)(nr_nlm_ptr->pano_param_ptr);
				vst_param = (struct sensor_vst_level*)(nr_vst_ptr->pano_param_ptr);
				ivst_param = (struct sensor_ivst_level*)(nr_ivst_ptr->pano_param_ptr);
				flat_offset_level = (struct sensor_flat_offset_level*)(nr_flat_offset_ptr->pano_param_ptr);

			break;
			default :
				nlm_param = (struct sensor_nlm_level*)(nr_nlm_ptr->common_param_ptr);
				vst_param = (struct sensor_vst_level*)(nr_vst_ptr->common_param_ptr);
				ivst_param = (struct sensor_ivst_level*)(nr_ivst_ptr->common_param_ptr);
				flat_offset_level = (struct sensor_flat_offset_level*)(nr_flat_offset_ptr->common_param_ptr);

			break;
		}
	}

	strength_level = PM_CLIP(strength_level, 0, SENSOR_SMART_LEVEL_NUM - 1);
	if (nlm_param != NULL) {
		dst_nlm_ptr->cur.flat_opt_bypass = nlm_param[strength_level].nlm_flat.flat_opt_bypass;
		dst_nlm_ptr->cur.flat_thr_bypass = nlm_param[strength_level].nlm_flat.flat_thresh_bypass;
		dst_nlm_ptr->cur.direction_mode_bypass = nlm_param[strength_level].nlm_dic.direction_mode_bypass;

		for (i = 0; i< 5; i++) {
			dst_nlm_ptr->cur.strength[i] = nlm_param[strength_level].nlm_flat.dgr_ctl[i].strength;
			dst_nlm_ptr->cur.cnt[i] = nlm_param[strength_level].nlm_flat.dgr_ctl[i].cnt;
			dst_nlm_ptr->cur.thresh[i] = nlm_param[strength_level].nlm_flat.dgr_ctl[i].thresh;
		}

		dst_nlm_ptr->cur.streng_th = nlm_param[strength_level].nlm_den.den_length;
		dst_nlm_ptr->cur.texture_dec = nlm_param[strength_level].nlm_den.texture_dec;
		dst_nlm_ptr->cur.opt_mode = nlm_param[strength_level].nlm_flat.flat_opt_mode;
		dst_nlm_ptr->cur.dist_mode = nlm_param[strength_level].nlm_dic.dist_mode;
		dst_nlm_ptr->cur.w_shift[0] = nlm_param[strength_level].nlm_dic.w_shift[0];
		dst_nlm_ptr->cur.w_shift[1] = nlm_param[strength_level].nlm_dic.w_shift[1];
		dst_nlm_ptr->cur.w_shift[2] = nlm_param[strength_level].nlm_dic.w_shift[2];
		dst_nlm_ptr->cur.cnt_th = nlm_param[strength_level].nlm_dic.cnt_th;
		dst_nlm_ptr->cur.tdist_min_th = nlm_param[strength_level].nlm_dic.tdist_min_th;
		dst_nlm_ptr->cur.diff_th = nlm_param[strength_level].nlm_dic.diff_th;

		for (i = 0; i< 72; i++) {
			dst_nlm_ptr->cur.lut_w[i] = nlm_param[strength_level].nlm_den.lut_w[i];
		}

		if (vst_param != NULL) {
			//memcpy(dst_nlm_ptr->cur.vst_addr, (void *)vst_param[src_nlm_ptr->strength_level].vst_param, dst_nlm_ptr->cur.vst_len);
		#if __WORDSIZE == 64
			addr = (void*)((isp_uint)dst_nlm_ptr->cur.vst_addr[1]<<32 | dst_nlm_ptr->cur.vst_addr[0]);
		#else
			addr = (void*)(dst_nlm_ptr->cur.vst_addr[0]);
		#endif
			memcpy(addr, (void *)vst_param[strength_level].vst_param, dst_nlm_ptr->cur.vst_len);
		}

		if (ivst_param != NULL) {
			//memcpy(dst_nlm_ptr->cur.ivst_addr, (void *)ivst_param[src_nlm_ptr->strength_level].ivst_param, dst_nlm_ptr->cur.ivst_len);
			#if __WORDSIZE == 64
				addr = (void*)((isp_uint)dst_nlm_ptr->cur.ivst_addr[1]<<32 | dst_nlm_ptr->cur.ivst_addr[0]);
			#else
				addr = (void*)(dst_nlm_ptr->cur.ivst_addr[0]);
			#endif
			memcpy(addr, (void *)ivst_param[strength_level].ivst_param, dst_nlm_ptr->cur.ivst_len);
		}

		if (flat_offset_level != NULL) {
			//memcpy(dst_nlm_ptr->cur.nlm_addr, (void *)flat_offset_level[src_nlm_ptr->strength_level].flat_offset_param, dst_nlm_ptr->cur.nlm_len);
			#if __WORDSIZE == 64
				addr = (void*)((isp_uint)dst_nlm_ptr->cur.nlm_addr[1]<<32 | dst_nlm_ptr->cur.nlm_addr[0]);
			#else
				addr = (void*)(dst_nlm_ptr->cur.nlm_addr[0]);
			#endif
			memcpy(addr, (void *)flat_offset_level[strength_level].flat_offset_param, dst_nlm_ptr->cur.nlm_len);
		}

		dst_nlm_ptr->cur.imp_opt_bypass = nlm_param[strength_level].imp_opt_bypass;
		dst_nlm_ptr->cur.addback = nlm_param[strength_level].add_back;
		dst_nlm_ptr->cur.bypass = nlm_param[strength_level].bypass;
	}

	return rtn;

}

static isp_s32 _pm_nlm_init(void *dst_nlm_param, void *src_nlm_param, void* param1, void* param_ptr2)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_s32 i = 0;
	void *addr = NULL;
	struct isp_nlm_param_v1 *dst_nlm_ptr = (struct isp_nlm_param_v1 *)dst_nlm_param;
	struct sensor_nlm_param *src_nlm_ptr = (struct sensor_nlm_param *)src_nlm_param;
	struct isp_pm_block_header *nlm_header_ptr = (struct isp_pm_block_header*)param1;
	UNUSED(param_ptr2);

	dst_nlm_ptr->cur.bypass = nlm_header_ptr->bypass;

	dst_nlm_ptr->vst_map.size = 1024 * sizeof(isp_u32);
	if (PNULL == dst_nlm_ptr->vst_map.data_ptr) {
		dst_nlm_ptr->vst_map.data_ptr = (void *)malloc(dst_nlm_ptr->vst_map.size);
		if (PNULL == dst_nlm_ptr->vst_map.data_ptr) {
			ISP_LOGE("_pm_nlm_init: vst malloc failed\n");
			rtn = ISP_ERROR;
			return rtn;
		}
	}
	memset((void *)dst_nlm_ptr->vst_map.data_ptr, 0x00, dst_nlm_ptr->vst_map.size);
	//dst_nlm_ptr->cur.vst_addr = dst_nlm_ptr->vst_map.data_ptr;
#if __WORDSIZE == 64
	dst_nlm_ptr->cur.vst_addr[0] = (isp_uint)(dst_nlm_ptr->vst_map.data_ptr) & 0xffffffff;
	dst_nlm_ptr->cur.vst_addr[1] = (isp_uint)(dst_nlm_ptr->vst_map.data_ptr) >> 32;
#else
	dst_nlm_ptr->cur.vst_addr[0] = (isp_uint)(dst_nlm_ptr->vst_map.data_ptr);
	dst_nlm_ptr->cur.vst_addr[1] = 0;
#endif
	dst_nlm_ptr->cur.vst_len = dst_nlm_ptr->vst_map.size;

	dst_nlm_ptr->ivst_map.size = 1024 * sizeof(isp_u32);
	if (PNULL == dst_nlm_ptr->ivst_map.data_ptr) {
		dst_nlm_ptr->ivst_map.data_ptr = (void *)malloc(dst_nlm_ptr->ivst_map.size);
		if (PNULL == dst_nlm_ptr->ivst_map.data_ptr) {
			ISP_LOGE("_pm_nlm_init: ivst malloc failed\n");
			rtn = ISP_ERROR;
			return rtn;
		}
	}
	memset((void *)dst_nlm_ptr->ivst_map.data_ptr, 0x00, dst_nlm_ptr->ivst_map.size);
	//dst_nlm_ptr->cur.ivst_addr = dst_nlm_ptr->ivst_map.data_ptr;
#if __WORDSIZE == 64
	dst_nlm_ptr->cur.ivst_addr[0] = (isp_uint)(dst_nlm_ptr->ivst_map.data_ptr) & 0xffffffff;
	dst_nlm_ptr->cur.ivst_addr[1] = (isp_uint)(dst_nlm_ptr->ivst_map.data_ptr) >> 32;
#else
	dst_nlm_ptr->cur.ivst_addr[0] = (isp_uint)(dst_nlm_ptr->ivst_map.data_ptr);
	dst_nlm_ptr->cur.ivst_addr[1] = 0;
#endif
	dst_nlm_ptr->cur.ivst_len = dst_nlm_ptr->ivst_map.size;

	dst_nlm_ptr->nlm_map.size = 1024 * sizeof(isp_u32);
	if (PNULL == dst_nlm_ptr->nlm_map.data_ptr) {
		dst_nlm_ptr->nlm_map.data_ptr = (void *)malloc(dst_nlm_ptr->nlm_map.size);
		if (PNULL == dst_nlm_ptr->nlm_map.data_ptr) {
			ISP_LOGE("_pm_nlm_init: nlm malloc failed\n");
			rtn = ISP_ERROR;
			return rtn;
		}
	}
	memset((void *)dst_nlm_ptr->nlm_map.data_ptr, 0x00, dst_nlm_ptr->nlm_map.size);
	//dst_nlm_ptr->cur.nlm_addr = dst_nlm_ptr->nlm_map.data_ptr;
#if __WORDSIZE == 64
	dst_nlm_ptr->cur.nlm_addr[0] = (isp_uint)(dst_nlm_ptr->nlm_map.data_ptr) & 0xffffffff;
	dst_nlm_ptr->cur.nlm_addr[1] = (isp_uint)(dst_nlm_ptr->nlm_map.data_ptr) >> 32;
#else
	dst_nlm_ptr->cur.nlm_addr[0] = (isp_uint)(dst_nlm_ptr->nlm_map.data_ptr);
	dst_nlm_ptr->cur.nlm_addr[1] = 0;
#endif
	dst_nlm_ptr->cur.nlm_len = dst_nlm_ptr->nlm_map.size;

	dst_nlm_ptr->cur.strength_level = SENSOR_SMART_LEVEL_DEFAULT;
	dst_nlm_ptr->nlm_level = SENSOR_SMART_LEVEL_DEFAULT;

	dst_nlm_ptr->nlm_ptr = src_nlm_ptr->param_nlm_ptr;
	dst_nlm_ptr->vst_ptr = src_nlm_ptr->param_vst_ptr;
	dst_nlm_ptr->ivst_ptr = src_nlm_ptr->param_ivst_ptr;
	dst_nlm_ptr->flat_offset_ptr = src_nlm_ptr->param_flat_offset_ptr;
	dst_nlm_ptr->multi_mode_enable = src_nlm_ptr->reserved[0];

	dst_nlm_ptr->cur.flat_opt_bypass = src_nlm_ptr->nlm_level.nlm_flat.flat_opt_bypass;
	dst_nlm_ptr->cur.flat_thr_bypass = src_nlm_ptr->nlm_level.nlm_flat.flat_thresh_bypass;
	dst_nlm_ptr->cur.direction_mode_bypass = src_nlm_ptr->nlm_level.nlm_dic.direction_mode_bypass;

	for (i = 0; i< 5; i++) {
		dst_nlm_ptr->cur.strength[i] = src_nlm_ptr->nlm_level.nlm_flat.dgr_ctl[i].strength;
		dst_nlm_ptr->cur.cnt[i] = src_nlm_ptr->nlm_level.nlm_flat.dgr_ctl[i].cnt;
		dst_nlm_ptr->cur.thresh[i] = src_nlm_ptr->nlm_level.nlm_flat.dgr_ctl[i].thresh;
	}

	dst_nlm_ptr->cur.streng_th = src_nlm_ptr->nlm_level.nlm_den.den_length;
	dst_nlm_ptr->cur.texture_dec = src_nlm_ptr->nlm_level.nlm_den.texture_dec;
	dst_nlm_ptr->cur.opt_mode = src_nlm_ptr->nlm_level.nlm_flat.flat_opt_mode;
	dst_nlm_ptr->cur.dist_mode = src_nlm_ptr->nlm_level.nlm_dic.dist_mode;
	dst_nlm_ptr->cur.w_shift[0] = src_nlm_ptr->nlm_level.nlm_dic.w_shift[0];
	dst_nlm_ptr->cur.w_shift[1] = src_nlm_ptr->nlm_level.nlm_dic.w_shift[1];
	dst_nlm_ptr->cur.w_shift[2] = src_nlm_ptr->nlm_level.nlm_dic.w_shift[2];
	dst_nlm_ptr->cur.cnt_th = src_nlm_ptr->nlm_level.nlm_dic.cnt_th;
	dst_nlm_ptr->cur.tdist_min_th = src_nlm_ptr->nlm_level.nlm_dic.tdist_min_th;
	dst_nlm_ptr->cur.diff_th = src_nlm_ptr->nlm_level.nlm_dic.diff_th;

	for (i = 0; i< 72; i++) {
		dst_nlm_ptr->cur.lut_w[i] = src_nlm_ptr->nlm_level.nlm_den.lut_w[i];
	}

	#if __WORDSIZE == 64
		addr = (void*)((isp_uint)dst_nlm_ptr->cur.vst_addr[1]<<32 | dst_nlm_ptr->cur.vst_addr[0]);
	#else
		addr = (void*)(dst_nlm_ptr->cur.vst_addr[0]);
	#endif
		memcpy(addr, (void *)src_nlm_ptr->vst_level.vst_param, dst_nlm_ptr->cur.vst_len);

	#if __WORDSIZE == 64
		addr = (void*)((isp_uint)dst_nlm_ptr->cur.ivst_addr[1]<<32 | dst_nlm_ptr->cur.ivst_addr[0]);
	#else
		addr = (void*)(dst_nlm_ptr->cur.ivst_addr[0]);
	#endif
	memcpy(addr, (void *)src_nlm_ptr->ivst_level.ivst_param, dst_nlm_ptr->cur.ivst_len);

	//memcpy(dst_nlm_ptr->cur.nlm_addr, (void *)flat_offset_level[src_nlm_ptr->strength_level].flat_offset_param, dst_nlm_ptr->cur.nlm_len);
	#if __WORDSIZE == 64
		addr = (void*)((isp_uint)dst_nlm_ptr->cur.nlm_addr[1]<<32 | dst_nlm_ptr->cur.nlm_addr[0]);
	#else
		addr = (void*)(dst_nlm_ptr->cur.nlm_addr[0]);
	#endif
	memcpy(addr, (void *)src_nlm_ptr->flat_offset_level.flat_offset_param, dst_nlm_ptr->cur.nlm_len);

	dst_nlm_ptr->cur.imp_opt_bypass = src_nlm_ptr->nlm_level.imp_opt_bypass;
	dst_nlm_ptr->cur.addback = src_nlm_ptr->nlm_level.add_back;

	rtn = _pm_nlm_convert_param(dst_nlm_ptr,SENSOR_SMART_LEVEL_DEFAULT,ISP_NR_COMMON_MODE);
	dst_nlm_ptr->cur.buf_sel = 0;
	dst_nlm_ptr->cur.bypass |= nlm_header_ptr->bypass;
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("ISP_PM_NLM_CONVERT_PARAM: error!");
		return rtn;
	}

	nlm_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_nlm_set_param(void *nlm_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_nlm_param_v1 *nlm_ptr = (struct isp_nlm_param_v1*)nlm_param;
	struct isp_pm_block_header *nlm_header_ptr = (struct isp_pm_block_header*)param_ptr1;

	nlm_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_NLM_BYPASS:
		nlm_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_SMART_SETTING:
	{
		struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;
		struct isp_range val_range = {0};
		isp_u32 nlm_level = 0;

		nlm_header_ptr->is_update = 0;
		val_range.min = 0;
		val_range.max = 255;

		rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_VALUE);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !");
			return rtn;
		}

		nlm_level = (isp_u32)block_result->component[0].fix_data[0];

		if (nlm_level != nlm_ptr->nlm_level || nr_tool_flag[4] || block_result->mode_flag_changed) {
			nlm_ptr->nlm_level = nlm_level;
			nlm_ptr->cur.strength_level = nlm_level;
			nlm_header_ptr->is_update = 1;
			nr_tool_flag[4] = 0;
			block_result->mode_flag_changed = 0;

			rtn = _pm_nlm_convert_param(nlm_ptr,nlm_ptr->nlm_level,block_result->mode_flag);
			nlm_ptr->cur.buf_sel = nlm_ptr->cur.buf_sel ? 0 : 1;
			nlm_ptr->cur.bypass |= nlm_header_ptr->bypass;
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("ISP_PM_NLM_CONVERT_PARAM: error!");
				return rtn;
			}
		}
	}
	break;

	default:
		nlm_header_ptr->is_update = ISP_ZERO;
	break;
	}

	ISP_LOGV("ISP_SMART: cmd=%d, update=%d, nlm_level=%d", cmd, nlm_header_ptr->is_update,
					nlm_ptr->nlm_level);

	return rtn;
}

static isp_s32 _pm_nlm_get_param(void *nlm_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_nlm_param_v1 *nlm_ptr = (struct isp_nlm_param_v1*)nlm_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_NLM;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &nlm_ptr->cur;
		param_data_ptr->data_size = sizeof(nlm_ptr->cur);
		*update_flag = 0;
	break;

	default:
	break;
	}

	return rtn;
}

static isp_s32 _pm_nlm_deinit(void* nlm_param)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_nlm_param_v1 * nlm_ptr = (struct isp_nlm_param_v1 *)nlm_param;

	if (PNULL != nlm_ptr->vst_map.data_ptr) {
		free(nlm_ptr->vst_map.data_ptr);
		nlm_ptr->vst_map.data_ptr = PNULL;
		nlm_ptr->vst_map.size = 0;
	}

	if (PNULL != nlm_ptr->ivst_map.data_ptr) {
		free(nlm_ptr->ivst_map.data_ptr);
		nlm_ptr->ivst_map.data_ptr = PNULL;
		nlm_ptr->ivst_map.size = 0;
	}

	if (PNULL != nlm_ptr->nlm_map.data_ptr) {
		free(nlm_ptr->nlm_map.data_ptr);
		nlm_ptr->nlm_map.data_ptr = PNULL;
		nlm_ptr->nlm_map.size = 0;
	}

	return rtn;
}

static isp_u32 _pm_nlm_convert_param_v2(void *dst_nlm_param, isp_u32 strength_level, isp_u32 mode_flag)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_s32 i = 0;
	struct isp_nlm_param_v2 *dst_nlm_ptr = (struct isp_nlm_param_v2 *)dst_nlm_param;
	void *addr = NULL;
	struct sensor_nlm_level* nlm_param;
	struct sensor_vst_level* vst_param;
	struct sensor_ivst_level* ivst_param;
	struct sensor_flat_offset_level* flat_offset_level;
	struct nr_param_ptr *nr_nlm_ptr;
	struct nr_param_ptr *nr_vst_ptr;
	struct nr_param_ptr *nr_ivst_ptr;
	struct nr_param_ptr *nr_flat_offset_ptr;

	if (SENSOR_MULTI_MODE_FLAG != dst_nlm_ptr->multi_mode_enable) {
		nlm_param = (struct sensor_nlm_level*)(dst_nlm_ptr->nlm_ptr);
		vst_param = (struct sensor_vst_level*)(dst_nlm_ptr->vst_ptr);
		ivst_param = (struct sensor_ivst_level*)(dst_nlm_ptr->ivst_ptr);
		flat_offset_level = (struct sensor_flat_offset_level*)(dst_nlm_ptr->flat_offset_ptr);
	} else {
		nr_nlm_ptr = (struct nr_param_ptr*)(dst_nlm_ptr->nlm_ptr);
		nr_vst_ptr = (struct nr_param_ptr*)(dst_nlm_ptr->vst_ptr);
		nr_ivst_ptr = (struct nr_param_ptr*)(dst_nlm_ptr->ivst_ptr);
		nr_flat_offset_ptr = (struct nr_param_ptr*)(dst_nlm_ptr->flat_offset_ptr);
		if (!nr_nlm_ptr || !nr_vst_ptr || !nr_ivst_ptr || !nr_flat_offset_ptr) {
			return rtn;
		}

		switch(mode_flag) {
			case ISP_NR_COMMON_MODE:
				nlm_param = (struct sensor_nlm_level*)(nr_nlm_ptr->common_param_ptr);
				vst_param = (struct sensor_vst_level*)(nr_vst_ptr->common_param_ptr);
				ivst_param = (struct sensor_ivst_level*)(nr_ivst_ptr->common_param_ptr);
				flat_offset_level = (struct sensor_flat_offset_level*)(nr_flat_offset_ptr->common_param_ptr);
			break;
			case ISP_NR_CAP_MODE:
				nlm_param = (struct sensor_nlm_level*)(nr_nlm_ptr->capture_param_ptr);
				vst_param = (struct sensor_vst_level*)(nr_vst_ptr->capture_param_ptr);
				ivst_param = (struct sensor_ivst_level*)(nr_ivst_ptr->capture_param_ptr);
				flat_offset_level = (struct sensor_flat_offset_level*)(nr_flat_offset_ptr->capture_param_ptr);

			break;
			case ISP_NR_VIDEO_MODE:
				nlm_param = (struct sensor_nlm_level*)(nr_nlm_ptr->video_param_ptr);
				vst_param = (struct sensor_vst_level*)(nr_vst_ptr->video_param_ptr);
				ivst_param = (struct sensor_ivst_level*)(nr_ivst_ptr->video_param_ptr);
				flat_offset_level = (struct sensor_flat_offset_level*)(nr_flat_offset_ptr->video_param_ptr);

			break;
			case ISP_NR_PANO_MODE:
				nlm_param = (struct sensor_nlm_level*)(nr_nlm_ptr->pano_param_ptr);
				vst_param = (struct sensor_vst_level*)(nr_vst_ptr->pano_param_ptr);
				ivst_param = (struct sensor_ivst_level*)(nr_ivst_ptr->pano_param_ptr);
				flat_offset_level = (struct sensor_flat_offset_level*)(nr_flat_offset_ptr->pano_param_ptr);

			break;
			default :
				nlm_param = (struct sensor_nlm_level*)(nr_nlm_ptr->common_param_ptr);
				vst_param = (struct sensor_vst_level*)(nr_vst_ptr->common_param_ptr);
				ivst_param = (struct sensor_ivst_level*)(nr_ivst_ptr->common_param_ptr);
				flat_offset_level = (struct sensor_flat_offset_level*)(nr_flat_offset_ptr->common_param_ptr);

			break;
		}
	}

	strength_level = PM_CLIP(strength_level, 0, SENSOR_SMART_LEVEL_NUM - 1);
	if (nlm_param != NULL) {
		dst_nlm_ptr->cur.flat_opt_bypass = nlm_param[strength_level].nlm_flat.flat_opt_bypass;
		//dst_nlm_ptr->cur.flat_thr_bypass = nlm_param[strength_level].nlm_flat.flat_thresh_bypass;
		//dst_nlm_ptr->cur.direction_mode_bypass = nlm_param[strength_level].nlm_dic.direction_mode_bypass;

		for (i = 0; i< 5; i++) {
			dst_nlm_ptr->cur.strength[i] = nlm_param[strength_level].nlm_flat.dgr_ctl[i].strength;
			dst_nlm_ptr->cur.cnt[i] = nlm_param[strength_level].nlm_flat.dgr_ctl[i].cnt;
			dst_nlm_ptr->cur.thresh[i] = nlm_param[strength_level].nlm_flat.dgr_ctl[i].thresh;
		}

		dst_nlm_ptr->cur.streng_th = nlm_param[strength_level].nlm_den.den_length;
		dst_nlm_ptr->cur.texture_dec = nlm_param[strength_level].nlm_den.texture_dec;
		//dst_nlm_ptr->cur.opt_mode = nlm_param[strength_level].nlm_flat.flat_opt_mode;
		//dst_nlm_ptr->cur.dist_mode = nlm_param[strength_level].nlm_dic.dist_mode;
		//dst_nlm_ptr->cur.w_shift[0] = nlm_param[strength_level].nlm_dic.w_shift[0];
		//dst_nlm_ptr->cur.w_shift[1] = nlm_param[strength_level].nlm_dic.w_shift[1];
		//dst_nlm_ptr->cur.w_shift[2] = nlm_param[strength_level].nlm_dic.w_shift[2];
		//dst_nlm_ptr->cur.cnt_th = nlm_param[strength_level].nlm_dic.cnt_th;
		//dst_nlm_ptr->cur.tdist_min_th = nlm_param[strength_level].nlm_dic.tdist_min_th;
		//dst_nlm_ptr->cur.diff_th = nlm_param[strength_level].nlm_dic.diff_th;

		for (i = 0; i< 72; i++) {
			dst_nlm_ptr->cur.lut_w[i] = nlm_param[strength_level].nlm_den.lut_w[i];
		}

		if (vst_param != NULL) {
			//memcpy(dst_nlm_ptr->cur.vst_addr, (void *)vst_param[src_nlm_ptr->strength_level].vst_param, dst_nlm_ptr->cur.vst_len);
		#if __WORDSIZE == 64
			addr = (void*)((isp_uint)dst_nlm_ptr->cur.vst_addr[1]<<32 | dst_nlm_ptr->cur.vst_addr[0]);
		#else
			addr = (void*)(dst_nlm_ptr->cur.vst_addr[0]);
		#endif
			memcpy(addr, (void *)vst_param[strength_level].vst_param, dst_nlm_ptr->cur.vst_len);
		}

		if (ivst_param != NULL) {
			//memcpy(dst_nlm_ptr->cur.ivst_addr, (void *)ivst_param[src_nlm_ptr->strength_level].ivst_param, dst_nlm_ptr->cur.ivst_len);
			#if __WORDSIZE == 64
				addr = (void*)((isp_uint)dst_nlm_ptr->cur.ivst_addr[1]<<32 | dst_nlm_ptr->cur.ivst_addr[0]);
			#else
				addr = (void*)(dst_nlm_ptr->cur.ivst_addr[0]);
			#endif
			memcpy(addr, (void *)ivst_param[strength_level].ivst_param, dst_nlm_ptr->cur.ivst_len);
		}

		if (flat_offset_level != NULL) {
			//memcpy(dst_nlm_ptr->cur.nlm_addr, (void *)flat_offset_level[src_nlm_ptr->strength_level].flat_offset_param, dst_nlm_ptr->cur.nlm_len);
			#if __WORDSIZE == 64
				addr = (void*)((isp_uint)dst_nlm_ptr->cur.nlm_addr[1]<<32 | dst_nlm_ptr->cur.nlm_addr[0]);
			#else
				addr = (void*)(dst_nlm_ptr->cur.nlm_addr[0]);
			#endif
			memcpy(addr, (void *)flat_offset_level[strength_level].flat_offset_param, dst_nlm_ptr->cur.nlm_len);
		}

		dst_nlm_ptr->cur.imp_opt_bypass = nlm_param[strength_level].imp_opt_bypass;
		dst_nlm_ptr->cur.addback = nlm_param[strength_level].add_back;
		dst_nlm_ptr->cur.bypass = nlm_param[strength_level].bypass;
	}

	return rtn;

}

static isp_s32 _pm_nlm_init_v2(void *dst_nlm_param, void *src_nlm_param, void* param1, void* param_ptr2)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_s32 i = 0;
	void *addr = NULL;
	struct isp_nlm_param_v2 *dst_nlm_ptr = (struct isp_nlm_param_v2 *)dst_nlm_param;
	struct sensor_nlm_param *src_nlm_ptr = (struct sensor_nlm_param *)src_nlm_param;
	struct isp_pm_block_header *nlm_header_ptr = (struct isp_pm_block_header*)param1;
	UNUSED(param_ptr2);

	dst_nlm_ptr->cur.bypass = nlm_header_ptr->bypass;

	dst_nlm_ptr->vst_map.size = 1024 * sizeof(isp_u32);
	if (PNULL == dst_nlm_ptr->vst_map.data_ptr) {
		dst_nlm_ptr->vst_map.data_ptr = (void *)malloc(dst_nlm_ptr->vst_map.size);
		if (PNULL == dst_nlm_ptr->vst_map.data_ptr) {
			ISP_LOGE("_pm_nlm_init: vst malloc failed\n");
			rtn = ISP_ERROR;
			return rtn;
		}
	}
	memset((void *)dst_nlm_ptr->vst_map.data_ptr, 0x00, dst_nlm_ptr->vst_map.size);
	//dst_nlm_ptr->cur.vst_addr = dst_nlm_ptr->vst_map.data_ptr;
#if __WORDSIZE == 64
	dst_nlm_ptr->cur.vst_addr[0] = (isp_uint)(dst_nlm_ptr->vst_map.data_ptr) & 0xffffffff;
	dst_nlm_ptr->cur.vst_addr[1] = (isp_uint)(dst_nlm_ptr->vst_map.data_ptr) >> 32;
#else
	dst_nlm_ptr->cur.vst_addr[0] = (isp_uint)(dst_nlm_ptr->vst_map.data_ptr);
	dst_nlm_ptr->cur.vst_addr[1] = 0;
#endif
	dst_nlm_ptr->cur.vst_len = dst_nlm_ptr->vst_map.size;

	dst_nlm_ptr->ivst_map.size = 1024 * sizeof(isp_u32);
	if (PNULL == dst_nlm_ptr->ivst_map.data_ptr) {
		dst_nlm_ptr->ivst_map.data_ptr = (void *)malloc(dst_nlm_ptr->ivst_map.size);
		if (PNULL == dst_nlm_ptr->ivst_map.data_ptr) {
			ISP_LOGE("_pm_nlm_init: ivst malloc failed\n");
			rtn = ISP_ERROR;
			return rtn;
		}
	}
	memset((void *)dst_nlm_ptr->ivst_map.data_ptr, 0x00, dst_nlm_ptr->ivst_map.size);
	//dst_nlm_ptr->cur.ivst_addr = dst_nlm_ptr->ivst_map.data_ptr;
#if __WORDSIZE == 64
	dst_nlm_ptr->cur.ivst_addr[0] = (isp_uint)(dst_nlm_ptr->ivst_map.data_ptr) & 0xffffffff;
	dst_nlm_ptr->cur.ivst_addr[1] = (isp_uint)(dst_nlm_ptr->ivst_map.data_ptr) >> 32;
#else
	dst_nlm_ptr->cur.ivst_addr[0] = (isp_uint)(dst_nlm_ptr->ivst_map.data_ptr);
	dst_nlm_ptr->cur.ivst_addr[1] = 0;
#endif
	dst_nlm_ptr->cur.ivst_len = dst_nlm_ptr->ivst_map.size;

	dst_nlm_ptr->nlm_map.size = 1024 * sizeof(isp_u32);
	if (PNULL == dst_nlm_ptr->nlm_map.data_ptr) {
		dst_nlm_ptr->nlm_map.data_ptr = (void *)malloc(dst_nlm_ptr->nlm_map.size);
		if (PNULL == dst_nlm_ptr->nlm_map.data_ptr) {
			ISP_LOGE("_pm_nlm_init: nlm malloc failed\n");
			rtn = ISP_ERROR;
			return rtn;
		}
	}
	memset((void *)dst_nlm_ptr->nlm_map.data_ptr, 0x00, dst_nlm_ptr->nlm_map.size);
	//dst_nlm_ptr->cur.nlm_addr = dst_nlm_ptr->nlm_map.data_ptr;
#if __WORDSIZE == 64
	dst_nlm_ptr->cur.nlm_addr[0] = (isp_uint)(dst_nlm_ptr->nlm_map.data_ptr) & 0xffffffff;
	dst_nlm_ptr->cur.nlm_addr[1] = (isp_uint)(dst_nlm_ptr->nlm_map.data_ptr) >> 32;
#else
	dst_nlm_ptr->cur.nlm_addr[0] = (isp_uint)(dst_nlm_ptr->nlm_map.data_ptr);
	dst_nlm_ptr->cur.nlm_addr[1] = 0;
#endif
	dst_nlm_ptr->cur.nlm_len = dst_nlm_ptr->nlm_map.size;

	dst_nlm_ptr->cur.strength_level = SENSOR_SMART_LEVEL_DEFAULT;
	dst_nlm_ptr->nlm_level = SENSOR_SMART_LEVEL_DEFAULT;

	dst_nlm_ptr->nlm_ptr = src_nlm_ptr->param_nlm_ptr;
	dst_nlm_ptr->vst_ptr = src_nlm_ptr->param_vst_ptr;
	dst_nlm_ptr->ivst_ptr = src_nlm_ptr->param_ivst_ptr;
	dst_nlm_ptr->flat_offset_ptr = src_nlm_ptr->param_flat_offset_ptr;
	dst_nlm_ptr->multi_mode_enable = src_nlm_ptr->reserved[0];

	dst_nlm_ptr->cur.flat_opt_bypass = src_nlm_ptr->nlm_level.nlm_flat.flat_opt_bypass;
	//dst_nlm_ptr->cur.flat_thr_bypass = src_nlm_ptr->nlm_level.nlm_flat.flat_thresh_bypass;
	//dst_nlm_ptr->cur.direction_mode_bypass = src_nlm_ptr->nlm_level.nlm_dic.direction_mode_bypass;

	for (i = 0; i< 5; i++) {
		dst_nlm_ptr->cur.strength[i] = src_nlm_ptr->nlm_level.nlm_flat.dgr_ctl[i].strength;
		dst_nlm_ptr->cur.cnt[i] = src_nlm_ptr->nlm_level.nlm_flat.dgr_ctl[i].cnt;
		dst_nlm_ptr->cur.thresh[i] = src_nlm_ptr->nlm_level.nlm_flat.dgr_ctl[i].thresh;
	}

	dst_nlm_ptr->cur.streng_th = src_nlm_ptr->nlm_level.nlm_den.den_length;
	dst_nlm_ptr->cur.texture_dec = src_nlm_ptr->nlm_level.nlm_den.texture_dec;
	//dst_nlm_ptr->cur.opt_mode = src_nlm_ptr->nlm_level.nlm_flat.flat_opt_mode;
	//dst_nlm_ptr->cur.dist_mode = src_nlm_ptr->nlm_level.nlm_dic.dist_mode;
	//dst_nlm_ptr->cur.w_shift[0] = src_nlm_ptr->nlm_level.nlm_dic.w_shift[0];
	//dst_nlm_ptr->cur.w_shift[1] = src_nlm_ptr->nlm_level.nlm_dic.w_shift[1];
	//dst_nlm_ptr->cur.w_shift[2] = src_nlm_ptr->nlm_level.nlm_dic.w_shift[2];
	//dst_nlm_ptr->cur.cnt_th = src_nlm_ptr->nlm_level.nlm_dic.cnt_th;
	//dst_nlm_ptr->cur.tdist_min_th = src_nlm_ptr->nlm_level.nlm_dic.tdist_min_th;
	//dst_nlm_ptr->cur.diff_th = src_nlm_ptr->nlm_level.nlm_dic.diff_th;

	for (i = 0; i< 72; i++) {
		dst_nlm_ptr->cur.lut_w[i] = src_nlm_ptr->nlm_level.nlm_den.lut_w[i];
	}

#if __WORDSIZE == 64
		addr = (void*)((isp_uint)dst_nlm_ptr->cur.vst_addr[1]<<32 | dst_nlm_ptr->cur.vst_addr[0]);
#else
		addr = (void*)(dst_nlm_ptr->cur.vst_addr[0]);
#endif
		memcpy(addr, (void *)src_nlm_ptr->vst_level.vst_param, dst_nlm_ptr->cur.vst_len);

#if __WORDSIZE == 64
		addr = (void*)((isp_uint)dst_nlm_ptr->cur.ivst_addr[1]<<32 | dst_nlm_ptr->cur.ivst_addr[0]);
#else
		addr = (void*)(dst_nlm_ptr->cur.ivst_addr[0]);
#endif
	memcpy(addr, (void *)src_nlm_ptr->ivst_level.ivst_param, dst_nlm_ptr->cur.ivst_len);

	//memcpy(dst_nlm_ptr->cur.nlm_addr, (void *)flat_offset_level[src_nlm_ptr->strength_level].flat_offset_param, dst_nlm_ptr->cur.nlm_len);
#if __WORDSIZE == 64
		addr = (void*)((isp_uint)dst_nlm_ptr->cur.nlm_addr[1]<<32 | dst_nlm_ptr->cur.nlm_addr[0]);
#else
		addr = (void*)(dst_nlm_ptr->cur.nlm_addr[0]);
#endif
	memcpy(addr, (void *)src_nlm_ptr->flat_offset_level.flat_offset_param, dst_nlm_ptr->cur.nlm_len);

	dst_nlm_ptr->cur.imp_opt_bypass = src_nlm_ptr->nlm_level.imp_opt_bypass;
	dst_nlm_ptr->cur.addback = src_nlm_ptr->nlm_level.add_back;

	rtn = _pm_nlm_convert_param_v2(dst_nlm_ptr, SENSOR_SMART_LEVEL_DEFAULT, ISP_NR_COMMON_MODE);
	dst_nlm_ptr->cur.bypass |= nlm_header_ptr->bypass;
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("ISP_PM_NLM_CONVERT_PARAM: error!");
		return rtn;
	}

	nlm_header_ptr->is_update = ISP_ONE;

	return rtn;

}

static isp_s32 _pm_nlm_set_param_v2(void *nlm_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_nlm_param_v2 *nlm_ptr = (struct isp_nlm_param_v2*)nlm_param;
	struct isp_pm_block_header *nlm_header_ptr = (struct isp_pm_block_header*)param_ptr1;

	nlm_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_NLM_BYPASS:
		nlm_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_SMART_SETTING:
	{
		struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;
		struct isp_range val_range = {0};
		isp_u32 nlm_level = 0;

		nlm_header_ptr->is_update = 0;
		val_range.min = 0;
		val_range.max = 255;

		rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_VALUE);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !");
			return rtn;
		}

		nlm_level = (isp_u32)block_result->component[0].fix_data[0];

		if (nlm_level != nlm_ptr->nlm_level || nr_tool_flag[5] || block_result->mode_flag_changed) {
			nlm_ptr->nlm_level = nlm_level;
			nlm_ptr->cur.strength_level = nlm_level;
			nlm_header_ptr->is_update = 1;
			nr_tool_flag[5] = 0;
			block_result->mode_flag_changed = 0;

			rtn = _pm_nlm_convert_param_v2(nlm_ptr,nlm_ptr->nlm_level,block_result->mode_flag);
			nlm_ptr->cur.bypass |= nlm_header_ptr->bypass;
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("ISP_PM_NLM_CONVERT_PARAM: error!");
				return rtn;
			}
		}
	}
	break;

	default:
		nlm_header_ptr->is_update = ISP_ZERO;
	break;
	}

	ISP_LOGI("ISP_SMART: cmd=%d, update=%d, nlm_level=%d", cmd, nlm_header_ptr->is_update,
					nlm_ptr->nlm_level);

	return rtn;
}

static isp_s32 _pm_nlm_get_param_v2(void *nlm_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_nlm_param_v2 *nlm_ptr = (struct isp_nlm_param_v2*)nlm_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_NLM;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &nlm_ptr->cur;
		param_data_ptr->data_size = sizeof(nlm_ptr->cur);
		*update_flag = 0;
	break;

	default:
	break;
	}

	return rtn;
}

static isp_s32 _pm_nlm_deinit_v2(void* nlm_param)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_nlm_param_v2 * nlm_ptr = (struct isp_nlm_param_v2 *)nlm_param;

	if (PNULL != nlm_ptr->vst_map.data_ptr) {
		free(nlm_ptr->vst_map.data_ptr);
		nlm_ptr->vst_map.data_ptr = PNULL;
		nlm_ptr->vst_map.size = 0;
	}

	if (PNULL != nlm_ptr->ivst_map.data_ptr) {
		free(nlm_ptr->ivst_map.data_ptr);
		nlm_ptr->ivst_map.data_ptr = PNULL;
		nlm_ptr->ivst_map.size = 0;
	}

	return rtn;
}

static isp_u32 _pm_cfa_convert_param(void *dst_cfae_param, isp_u32 strength_level, isp_u32 mode_flag)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_s32 i = 0;
	struct isp_cfa_param_v1 *dst_cfae_ptr = (struct isp_cfa_param_v1*)dst_cfae_param;
	struct sensor_cfae_level* cfae_param;
	struct nr_param_ptr *nr_ptr;

	if (SENSOR_MULTI_MODE_FLAG != dst_cfae_ptr->multi_mode_enable) {
		cfae_param = (struct sensor_cfae_level*)(dst_cfae_ptr->cfae_ptr);
	} else {
		nr_ptr = (struct nr_param_ptr*)(dst_cfae_ptr->cfae_ptr);
		if (!nr_ptr) {
			return rtn;
		}

		switch(mode_flag) {
			case ISP_NR_COMMON_MODE:
				cfae_param = (struct sensor_cfae_level*)(nr_ptr->common_param_ptr);
			break;
			case ISP_NR_CAP_MODE:
				cfae_param = (struct sensor_cfae_level*)(nr_ptr->capture_param_ptr);
			break;
			case ISP_NR_VIDEO_MODE:
				cfae_param = (struct sensor_cfae_level*)(nr_ptr->video_param_ptr);
			break;
			case ISP_NR_PANO_MODE:
				cfae_param = (struct sensor_cfae_level*)(nr_ptr->pano_param_ptr);
			break;
			default :
				cfae_param = (struct sensor_cfae_level*)(nr_ptr->common_param_ptr);
			break;
		}
	}

	strength_level = PM_CLIP(strength_level, 0, SENSOR_SMART_LEVEL_NUM - 1);
	if (cfae_param != NULL) {
		dst_cfae_ptr->cur.ee_bypass = cfae_param[strength_level].cfa_ee.cfa_ee_bypass;
		dst_cfae_ptr->cur.doee_base = cfae_param[strength_level].cfa_ee.doee_base;
		dst_cfae_ptr->cur.avg_mode = cfae_param[strength_level].cfa_comm.avg_mode;
		dst_cfae_ptr->cur.grid_gain = cfae_param[strength_level].cfa_comm.grid_gain;
		dst_cfae_ptr->cur.cfa_uni_dir_intplt_tr =cfae_param[strength_level].cfa_undir.cfa_uni_dir_intplt_tr;
		dst_cfae_ptr->cur.cfai_ee_uni_dir_tr = cfae_param[strength_level].cfa_undir.cfai_ee_uni_dir_tr;
		dst_cfae_ptr->cur.cfai_ee_edge_tr = cfae_param[strength_level].cfa_ee.ee_thr.cfai_ee_edge_tr;
		dst_cfae_ptr->cur.cfai_ee_diagonal_tr = cfae_param[strength_level].cfa_ee.ee_thr.cfai_ee_diagonal_tr;
		dst_cfae_ptr->cur.cfai_ee_grid_tr = cfae_param[strength_level].cfa_ee.ee_thr.cfai_ee_grid_tr;
		dst_cfae_ptr->cur.cfai_doee_clip_tr = cfae_param[strength_level].cfa_ee.ee_thr.cfai_doee_clip_tr;
		dst_cfae_ptr->cur.cfai_ee_saturation_level = cfae_param[strength_level].cfa_ee.cfai_ee_saturation_level;
		dst_cfae_ptr->cur.plt_diff_tr = cfae_param[strength_level].cfa_undir.plt_diff_tr;
		dst_cfae_ptr->cur.grid_min_tr = cfae_param[strength_level].cfa_comm.grid_min_tr;
		dst_cfae_ptr->cur.strength_tr_neg = cfae_param[strength_level].cfa_ee.ee_thr.strength_tr_neg;
		dst_cfae_ptr->cur.strength_tr_pos = cfae_param[strength_level].cfa_ee.ee_thr.strength_tr_pos;
		dst_cfae_ptr->cur.ee_strength_neg = cfae_param[strength_level].cfa_ee.ee_strength_neg;
		dst_cfae_ptr->cur.ee_strength_pos = cfae_param[strength_level].cfa_ee.ee_strength_pos;
		dst_cfae_ptr->cur.inter_chl_gain = cfae_param[strength_level].cfa_comm.inter_chl_gain;
	}


	return rtn;
}

static isp_s32 _pm_cfa_init_v1(void *dst_cfae_param, void *src_cfae_param, void* param1, void* param_ptr2)
{
	isp_s32 rtn = ISP_SUCCESS;

	struct isp_cfa_param_v1 *dst_cfae_ptr = (struct isp_cfa_param_v1*)dst_cfae_param;
	struct sensor_cfa_param_v1 *src_cfae_ptr = (struct sensor_cfa_param_v1*)src_cfae_param;
	struct isp_pm_block_header *cfae_header_ptr = (struct isp_pm_block_header*)param1;
	struct isp_size *img_size_ptr = (struct isp_size*)param_ptr2;

	dst_cfae_ptr->cur.bypass = cfae_header_ptr->bypass;
	dst_cfae_ptr->cur_level = SENSOR_SMART_LEVEL_DEFAULT;
	dst_cfae_ptr->cfae_ptr = src_cfae_ptr->param_ptr;
	dst_cfae_ptr->multi_mode_enable = src_cfae_ptr->reserved[0];

	dst_cfae_ptr->cur.ee_bypass = src_cfae_ptr->cfae_level.cfa_ee.cfa_ee_bypass;
	dst_cfae_ptr->cur.doee_base = src_cfae_ptr->cfae_level.cfa_ee.doee_base;
	dst_cfae_ptr->cur.avg_mode = src_cfae_ptr->cfae_level.cfa_comm.avg_mode;
	dst_cfae_ptr->cur.grid_gain = src_cfae_ptr->cfae_level.cfa_comm.grid_gain;
	dst_cfae_ptr->cur.cfa_uni_dir_intplt_tr = src_cfae_ptr->cfae_level.cfa_undir.cfa_uni_dir_intplt_tr;
	dst_cfae_ptr->cur.cfai_ee_uni_dir_tr = src_cfae_ptr->cfae_level.cfa_undir.cfai_ee_uni_dir_tr;
	dst_cfae_ptr->cur.cfai_ee_edge_tr = src_cfae_ptr->cfae_level.cfa_ee.ee_thr.cfai_ee_edge_tr;
	dst_cfae_ptr->cur.cfai_ee_diagonal_tr = src_cfae_ptr->cfae_level.cfa_ee.ee_thr.cfai_ee_diagonal_tr;
	dst_cfae_ptr->cur.cfai_ee_grid_tr = src_cfae_ptr->cfae_level.cfa_ee.ee_thr.cfai_ee_grid_tr;
	dst_cfae_ptr->cur.cfai_doee_clip_tr = src_cfae_ptr->cfae_level.cfa_ee.ee_thr.cfai_doee_clip_tr;
	dst_cfae_ptr->cur.cfai_ee_saturation_level = src_cfae_ptr->cfae_level.cfa_ee.cfai_ee_saturation_level;
	dst_cfae_ptr->cur.plt_diff_tr = src_cfae_ptr->cfae_level.cfa_undir.plt_diff_tr;
	dst_cfae_ptr->cur.grid_min_tr = src_cfae_ptr->cfae_level.cfa_comm.grid_min_tr;
	dst_cfae_ptr->cur.strength_tr_neg = src_cfae_ptr->cfae_level.cfa_ee.ee_thr.strength_tr_neg;
	dst_cfae_ptr->cur.strength_tr_pos = src_cfae_ptr->cfae_level.cfa_ee.ee_thr.strength_tr_pos;
	dst_cfae_ptr->cur.ee_strength_neg = src_cfae_ptr->cfae_level.cfa_ee.ee_strength_neg;
	dst_cfae_ptr->cur.ee_strength_pos = src_cfae_ptr->cfae_level.cfa_ee.ee_strength_pos;
	dst_cfae_ptr->cur.inter_chl_gain = src_cfae_ptr->cfae_level.cfa_comm.inter_chl_gain;

	dst_cfae_ptr->cur.gbuf_addr_max = (img_size_ptr->w/2) + 1;
	rtn = _pm_cfa_convert_param(dst_cfae_ptr,SENSOR_SMART_LEVEL_DEFAULT,ISP_NR_COMMON_MODE);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("ISP_PM_CFAE_CONVERT_PARAM: error!");
		return rtn;
	}

	cfae_header_ptr->is_update = ISP_ONE;
	return rtn;
}

static isp_s32 _pm_cfa_set_param_v1(void *cfae_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_cfa_param_v1 *cfae_ptr = (struct isp_cfa_param_v1*)cfae_param;
	struct isp_pm_block_header *cfae_header_ptr = (struct isp_pm_block_header*)param_ptr1;

	cfae_header_ptr->is_update = ISP_ONE;

	switch(cmd) {
	case ISP_PM_BLK_CFA_BYPASS:
		cfae_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_SMART_SETTING:
	{
		struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;
		struct isp_range val_range = {0};
		isp_u32 cur_level = 0;

		cfae_header_ptr->is_update = 0;
		val_range.min = 0;
		val_range.max = 255;

		rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_VALUE);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !");
			return rtn;
		}

		cur_level = (isp_u32)block_result->component[0].fix_data[0];

		if (cur_level != cfae_ptr->cur_level || nr_tool_flag[6] || block_result->mode_flag_changed) {
			cfae_ptr->cur_level = cur_level;
			cfae_header_ptr->is_update = 1;
			nr_tool_flag[6] = 0;
			block_result->mode_flag_changed = 0;
			rtn = _pm_cfa_convert_param(cfae_ptr,cfae_ptr->cur_level,block_result->mode_flag);
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("ISP_PM_CFAE_CONVERT_PARAM: error!");
				return rtn;
			}
		}
	}
	break;

	default:
		cfae_header_ptr->is_update = ISP_ZERO;
	break;
	}

	ISP_LOGV("ISP_SMART: cmd = %d, is_update = %d, cur_level=%d",
		cmd, cfae_header_ptr->is_update, cfae_ptr->cur_level);

	return rtn;
}

static isp_s32 _pm_cfa_get_param_v1(void *cfa_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_cfa_param_v1 *cfa_ptr = (struct isp_cfa_param_v1*)cfa_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_CFA_V1;
	param_data_ptr->cmd = cmd;

	switch(cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &cfa_ptr->cur;
		param_data_ptr->data_size = sizeof(cfa_ptr->cur);
		*update_flag = 0;
	break;
	default:
	break;
	}

	return rtn;
}

static isp_s32 _pm_cmc10_init(void *dst_cmc10_param, void *src_cmc10_param, void* param1, void* param_ptr2)
{
	isp_u32 i = 0;
	isp_u32 j = 0;
	isp_u32 index = 0;
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_cmc10_param *dst_cmc10_ptr = (struct isp_cmc10_param*)dst_cmc10_param;
	struct sensor_cmc_v1_param *src_cmc10_ptr = (struct sensor_cmc_v1_param*)src_cmc10_param;
	struct isp_pm_block_header *cmc10_header_ptr = (struct isp_pm_block_header*)param1;
	UNUSED(param_ptr2);

	dst_cmc10_ptr->cur_idx_info.x0 = src_cmc10_ptr->cur_idx.x0;
	dst_cmc10_ptr->cur_idx_info.x1 = src_cmc10_ptr->cur_idx.x1;
	dst_cmc10_ptr->cur_idx_info.weight0 = src_cmc10_ptr->cur_idx.weight0;
	dst_cmc10_ptr->cur_idx_info.weight1 = src_cmc10_ptr->cur_idx.weight1;
	index = src_cmc10_ptr->cur_idx.x0;

	for(i = 0; i < SENSOR_CMC_NUM; i++) {
		for(j = 0; j < SENSOR_CMC_POINT_NUM; j++){
			dst_cmc10_ptr->matrix[i][j] = src_cmc10_ptr->matrix[i][j];
			dst_cmc10_ptr->buffer[i][j] = src_cmc10_ptr->buffer[i][j];
		}
	}

	dst_cmc10_ptr->cur.bypass = cmc10_header_ptr->bypass;
	dst_cmc10_ptr->cur.alpha_bypass = src_cmc10_ptr->alpha_bypass;
	dst_cmc10_ptr->cur.buf_sel = src_cmc10_ptr->buf_sel;
	dst_cmc10_ptr->cur.alpha = src_cmc10_ptr->alpha_val;

	for(j = 0; j < SENSOR_CMC_POINT_NUM; j++){
		dst_cmc10_ptr->cur.matrix.val[j] = src_cmc10_ptr->matrix[index][j];
		dst_cmc10_ptr->cur.matrix_buf.val[j] = src_cmc10_ptr->buffer[index][j];
	}

	cmc10_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_cmc10_adjust(struct isp_cmc10_param *cmc_ptr, isp_u32 is_reduce)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u16* cmc_matrix_x0 = NULL;
	isp_u16* cmc_matrix_x1 = NULL;
	isp_u32 alpha_x0 = 0x00;
	isp_u32 alpha_x1 = 0x00;
	void *src_matrix[2] = {NULL};
	isp_u16 weight[2] = {0};
	uint16_t interp_result[SENSOR_CMC_POINT_NUM] = {0};
	uint32_t i = 0;


	if(( cmc_ptr->cur_idx_info.x0 >= ISP_CMC_NUM ) || ( cmc_ptr->cur_idx_info.x1 >= ISP_CMC_NUM )) {
		rtn = ISP_ERROR;
		return rtn;
	}

	src_matrix[0] = cmc_ptr->matrix[cmc_ptr->cur_idx_info.x0];
	src_matrix[1] = cmc_ptr->matrix[cmc_ptr->cur_idx_info.x1];
	weight[0] = cmc_ptr->cur_idx_info.weight0;
	weight[1] = cmc_ptr->cur_idx_info.weight1;


	/*interplate alg.*/
	rtn = isp_interp_data(interp_result, src_matrix, weight, 9, ISP_INTERP_INT14);
	if( ISP_SUCCESS != rtn ) {
		rtn = ISP_ERROR;
		return rtn;
	}

	if (ISP_ONE == is_reduce) {
		isp_cmc_adjust_4_reduce_saturation(interp_result, cmc_ptr->result_cmc, cmc_ptr->reduce_percent);
	} else {
		memcpy(cmc_ptr->result_cmc, interp_result, sizeof(cmc_ptr->result_cmc));
	}

	for (i=0; i<SENSOR_CMC_POINT_NUM; i++) {
		cmc_ptr->cur.matrix.val[i] = cmc_ptr->result_cmc[i];
	}

	return rtn;
}

static isp_s32 _pm_cmc10_set_param(void *cmc10_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_cmc10_param *cmc10_ptr = (struct isp_cmc10_param*)cmc10_param;
	struct isp_pm_block_header *cmc10_header_ptr = (struct isp_pm_block_header*)param_ptr1;

	cmc10_header_ptr->is_update = ISP_ONE;

	switch(cmd) {
	case ISP_PM_BLK_CMC10_BYPASS:
		cmc10_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_CMC10:
		memcpy(cmc10_ptr->cur.matrix.val, param_ptr0, 9*sizeof(uint16_t));
	break;

	case ISP_PM_BLK_SMART_SETTING:
	{
		struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;
		struct isp_weight_value cmc_value = {{0}, {0}};
		isp_u32 is_reduce = ISP_ZERO;
		struct isp_range val_range = {0};
		isp_u32 update = 0;

		if (NULL == block_result) {
			return ISP_ERROR;
		}

		if (0 == block_result->update) {
			return rtn;
		}

		cmc10_header_ptr->is_update = 0;
		if (ISP_SMART_CMC == block_result->smart_id) {

			struct isp_weight_value *weight_value = NULL;

			val_range.min = 0;
			val_range.max = ISP_CMC_NUM - 1;
			rtn = _pm_check_smart_param(block_result, &val_range, 1,
							ISP_SMART_Y_TYPE_WEIGHT_VALUE);
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !\n");
				return rtn;
			}

			weight_value = (struct isp_weight_value *)block_result->component[0].fix_data;
			cmc_value = *weight_value;

			cmc_value.weight[0] = cmc_value.weight[0] / (SMART_WEIGHT_UNIT / 16)
								* (SMART_WEIGHT_UNIT / 16);
			cmc_value.weight[1] = SMART_WEIGHT_UNIT - cmc_value.weight[0];

			if (cmc_value.value[0] != cmc10_ptr->cur_idx_info.x0
				|| cmc_value.weight[0] != cmc10_ptr->cur_idx_info.weight0) {
				cmc10_ptr->cur_idx_info.x0 = cmc_value.value[0];
				cmc10_ptr->cur_idx_info.x1 = cmc_value.value[1];
				cmc10_ptr->cur_idx_info.weight0 = cmc_value.weight[0];
				cmc10_ptr->cur_idx_info.weight1 = cmc_value.weight[1];

				is_reduce = 0;
				update = 1;
			}
		} else if (ISP_SMART_SATURATION_DEPRESS == block_result->smart_id) {

			val_range.min = 1;
			val_range.max = 255;
			rtn = _pm_check_smart_param(block_result, &val_range, 1,
							ISP_SMART_Y_TYPE_VALUE);
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !\n");
				return rtn;
			}

			cmc10_ptr->reduce_percent = block_result->component[0].fix_data[0];
			is_reduce = 1;
			update = 1;
		}

		if (0 != update) {
				_pm_cmc10_adjust(cmc10_ptr, is_reduce);
			cmc10_header_ptr->is_update = 1;
		}
	}
	break;

	default:
		cmc10_header_ptr->is_update = ISP_ZERO;
	break;
	}

	ISP_LOGV("ISP_SMART: cmd = %d, is_update =%d, reduce_percent=%d",
			cmd, cmc10_header_ptr->is_update, cmc10_ptr->reduce_percent);

	return rtn;
}

static isp_s32 _pm_cmc10_get_param(void *cmc10_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_cmc10_param *cmc10_ptr = (struct isp_cmc10_param*)cmc10_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_CMC10;
	param_data_ptr->cmd = cmd;

	switch(cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &cmc10_ptr->cur;
		param_data_ptr->data_size = sizeof(cmc10_ptr->cur);
		*update_flag = 0;
	break;

	default:
	break;
	}

	return rtn;
}

static isp_s32 _pm_frgb_gamc_init(void *dst_gamc_param, void *src_gamc_param, void* param1, void* param_ptr2)
{
	isp_u32 i = 0;
	isp_u32 j = 0;
	isp_u32 index = 0;
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_frgb_gamc_param *dst_gamc_ptr = (struct isp_frgb_gamc_param*)dst_gamc_param;
	struct sensor_frgb_gammac_param *src_gamc_ptr = (struct sensor_frgb_gammac_param*)src_gamc_param;
	struct isp_pm_block_header *gamc_header_ptr = (struct isp_pm_block_header*)param1;
	UNUSED(param_ptr2);

	for (i = 0; i < SENSOR_GAMMA_NUM; i++) {
		for (j = 0; j < SENSOR_GAMMA_POINT_NUM; j++) {
			dst_gamc_ptr->curve_tab[i].points[j].x = src_gamc_ptr->curve_tab[i].points[j].x;
			dst_gamc_ptr->curve_tab[i].points[j].y = src_gamc_ptr->curve_tab[i].points[j].y;
		}
	}

	dst_gamc_ptr->cur_idx.x0 = src_gamc_ptr->cur_idx_info.x0;
	dst_gamc_ptr->cur_idx.x1 = src_gamc_ptr->cur_idx_info.x1;
	dst_gamc_ptr->cur_idx.weight0 = src_gamc_ptr->cur_idx_info.weight0;
	dst_gamc_ptr->cur_idx.weight1 = src_gamc_ptr->cur_idx_info.weight1;
	index = src_gamc_ptr->cur_idx_info.x0;
	dst_gamc_ptr->cur.bypass = gamc_header_ptr->bypass;

	for (j = 0; j < ISP_PINGPANG_FRGB_GAMC_NUM; j++) {
		dst_gamc_ptr->cur.nodes[j].node_x = src_gamc_ptr->curve_tab[index].points[j].x;
		dst_gamc_ptr->cur.nodes[j].node_y = src_gamc_ptr->curve_tab[index].points[j].y;
	}

	gamc_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_frgb_gamc_set_param(void *gamc_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_frgb_gamc_param *gamc_ptr = (struct isp_frgb_gamc_param*)gamc_param;
	struct isp_pm_block_header *gamc_header_ptr = (struct isp_pm_block_header*)param_ptr1;

	gamc_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_GAMMA_BYPASS:
		gamc_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_GAMMA:
	{
		uint32_t gamma_idx = *((isp_u32*)param_ptr0);
		uint32_t i;

		gamc_ptr->cur_idx.x0 = gamma_idx;
		gamc_ptr->cur_idx.x1 = gamma_idx;
		gamc_ptr->cur_idx.weight0 = 256;
		gamc_ptr->cur_idx.weight1 = 0;

		for (i = 0; i < ISP_PINGPANG_FRGB_GAMC_NUM; i++) {
			gamc_ptr->cur.nodes[i].node_x = gamc_ptr->curve_tab[gamma_idx].points[i].x;
			gamc_ptr->cur.nodes[i].node_y = gamc_ptr->curve_tab[gamma_idx].points[i].y;
		}
	}

	break;

	case ISP_PM_BLK_SMART_SETTING:
	{
		struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;
		struct isp_weight_value *weight_value = NULL;
		struct isp_weight_value gamc_value = {{0}, {0}};
		struct isp_gamma_curve_info *src_ptr0 = PNULL;
		struct isp_gamma_curve_info *src_ptr1 = PNULL;
		struct isp_gamma_curve_info *dst_ptr = PNULL;
		struct isp_range val_range = {0};
		int i;

		gamc_header_ptr->is_update = 0;

		val_range.min = 0;
		val_range.max = SENSOR_GAMMA_NUM - 1;
		rtn = _pm_check_smart_param(block_result, &val_range, 1,
						ISP_SMART_Y_TYPE_WEIGHT_VALUE);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !\n");
			return rtn;
		}

		weight_value = (struct isp_weight_value *)block_result->component[0].fix_data;
		gamc_value = *weight_value;

		gamc_value.weight[0] = gamc_value.weight[0] / (SMART_WEIGHT_UNIT / 16)
								* (SMART_WEIGHT_UNIT / 16);
		gamc_value.weight[1] = SMART_WEIGHT_UNIT - gamc_value.weight[0];

		if (gamc_value.value[0] != gamc_ptr->cur_idx.x0
			|| gamc_value.weight[0] != gamc_ptr->cur_idx.weight0) {

			void *src[2] = {NULL};
			void *dst = NULL;
			src[0] = &gamc_ptr->curve_tab[gamc_value.value[0]].points[0].x;
			src[1] = &gamc_ptr->curve_tab[gamc_value.value[1]].points[0].x;
			dst = &gamc_ptr->final_curve;

			/*interpolate y coordinates*/
			rtn = isp_interp_data(dst, src, gamc_value.weight, SENSOR_GAMMA_POINT_NUM*2, ISP_INTERP_UINT16);
			if (ISP_SUCCESS == rtn) {
				gamc_ptr->cur_idx.x0 = weight_value->value[0];
				gamc_ptr->cur_idx.x1 = weight_value->value[1];
				gamc_ptr->cur_idx.weight0 = weight_value->weight[0];
				gamc_ptr->cur_idx.weight1 = weight_value->weight[1];
				gamc_header_ptr->is_update = 1;

				for (i = 0; i < ISP_PINGPANG_FRGB_GAMC_NUM; i++) {
					gamc_ptr->cur.nodes[i].node_x = gamc_ptr->final_curve.points[i].x;
					gamc_ptr->cur.nodes[i].node_y = gamc_ptr->final_curve.points[i].y;
				}
			}
		}
	}
	break;

	default:
		gamc_header_ptr->is_update = ISP_ZERO;
	break;
	}

	ISP_LOGV("cmd=%d, update=%d, value=(%d, %d), weight=(%d, %d)\n", cmd, gamc_header_ptr->is_update,
			gamc_ptr->cur_idx.x0, gamc_ptr->cur_idx.x1,
			gamc_ptr->cur_idx.weight0, gamc_ptr->cur_idx.weight1);

	return rtn;
}

static isp_s32 _pm_frgb_gamc_get_param(void *gamc_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_frgb_gamc_param *gamc_ptr = (struct isp_frgb_gamc_param*)gamc_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_RGB_GAMC;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &gamc_ptr->cur;
		param_data_ptr->data_size = sizeof(gamc_ptr->cur);
		*update_flag = 0;
	break;

	default:
	break;
	}

	return rtn;
}

static isp_s32 _pm_cmc8_init_v1(void *dst_cmc8_param, void *src_cmc8_param, void* param1, void* param_ptr2)
{
	isp_u32 i = 0;
	isp_u32 j = 0;
	isp_u32 index = 0;
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_cmc8_param_v1 *dst_cmc8_ptr = (struct isp_cmc8_param_v1*)dst_cmc8_param;
	struct sensor_cmc_v1_param *src_cmc8_ptr = (struct sensor_cmc_v1_param*)src_cmc8_param;
	struct isp_pm_block_header *cmc8_header_ptr = (struct isp_pm_block_header*)param1;
	UNUSED(param_ptr2);

	index = src_cmc8_ptr->cur_idx.x0;
	dst_cmc8_ptr->cur_idx_info.x0 = src_cmc8_ptr->cur_idx.x0;
	dst_cmc8_ptr->cur_idx_info.x1 = src_cmc8_ptr->cur_idx.x1;
	dst_cmc8_ptr->cur_idx_info.weight0 = src_cmc8_ptr->cur_idx.weight0;
	dst_cmc8_ptr->cur_idx_info.weight1 = src_cmc8_ptr->cur_idx.weight1;

	for(i = 0; i < SENSOR_CMC_NUM; i++) {
		for(j = 0; j < SENSOR_CMC_POINT_NUM; j++){
			dst_cmc8_ptr->matrix[i][j] = src_cmc8_ptr->matrix[i][j];
			dst_cmc8_ptr->buffer[i][j] = src_cmc8_ptr->buffer[i][j];
		}
	}

	dst_cmc8_ptr->cur.bypass = cmc8_header_ptr->bypass;
	dst_cmc8_ptr->cur.alpha_bypass = src_cmc8_ptr->alpha_bypass;
	dst_cmc8_ptr->cur.buf_sel = src_cmc8_ptr->buf_sel;
	dst_cmc8_ptr->cur.alpha = src_cmc8_ptr->alpha_val;

	for(j = 0; j < SENSOR_CMC_POINT_NUM; j++){
		dst_cmc8_ptr->cur.matrix.val[j] = src_cmc8_ptr->matrix[index][j];
		dst_cmc8_ptr->cur.matrix_buf.val[j] = src_cmc8_ptr->buffer[index][j];
	}

	cmc8_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_cmc8_set_param_v1(void *cmc8_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_cmc8_param_v1 *cmc8_ptr = (struct isp_cmc8_param_v1 *)cmc8_param;
	struct isp_pm_block_header *cmc8_header_ptr = (struct isp_pm_block_header*)param_ptr1;

	cmc8_header_ptr->is_update = ISP_ONE;

	switch(cmd) {
	case ISP_PM_BLK_CMC8_BYPASS:
		cmc8_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	default:
		cmc8_header_ptr->is_update = ISP_ZERO;
	break;
	}

	return rtn;
}

static isp_s32 _pm_cmc8_get_param_v1(void *cmc8_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_cmc8_param_v1 *cmc8_ptr = (struct isp_cmc8_param_v1 *)cmc8_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_CMC8;
	param_data_ptr->cmd = cmd;

	switch(cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &cmc8_ptr->cur;
		param_data_ptr->data_size = sizeof(cmc8_ptr->cur);
		*update_flag = 0;
	break;

	default:
	break;
	}

	return rtn;
}

static isp_s32 _pm_ctm_init(void *dst_ctm_param, void *src_ctm_param, void* param1, void* param_ptr2)
{
	isp_u32 i = 0;
	isp_u32 index = 0;
	isp_s32 rtn = ISP_SUCCESS;
	intptr_t addr = 0;
	struct isp_ctm_param *dst_ctm_ptr = (struct isp_ctm_param *)dst_ctm_param;
	struct sensor_ctm_param *src_ctm_ptr = (struct sensor_ctm_param*)src_ctm_param;
	struct isp_pm_block_header *ctm_header_ptr = (struct isp_pm_block_header*)param1;
	UNUSED(param_ptr2);

	index = src_ctm_ptr->cur_idx.x0;
	dst_ctm_ptr->cur_idx.x0 = src_ctm_ptr->cur_idx.x0;
	dst_ctm_ptr->cur_idx.x1 = src_ctm_ptr->cur_idx.x1;
	dst_ctm_ptr->cur_idx.weight0 = src_ctm_ptr->cur_idx.weight0;
	dst_ctm_ptr->cur_idx.weight1 = src_ctm_ptr->cur_idx.weight1;

	ISP_LOGV("index=(%d, %d), weight=(%d, %d)\n", src_ctm_ptr->cur_idx.x0, src_ctm_ptr->cur_idx.x1,
				src_ctm_ptr->cur_idx.weight0, src_ctm_ptr->cur_idx.weight1);

	if (dst_ctm_ptr->cur_idx.x0 >= SENSOR_CTM_NUM) {
		ISP_LOGE("invalid index=%d\n", dst_ctm_ptr->cur_idx.x0);
		return ISP_ERROR;
	}
	for (i = 0; i < SENSOR_CTM_NUM ; i++) {
		if (src_ctm_ptr->map[i].size > 0) {
		dst_ctm_ptr->map[i].size = src_ctm_ptr->map[i].size;
		addr = (intptr_t)&src_ctm_ptr->data_area + src_ctm_ptr->map[i].offset;
		dst_ctm_ptr->map[i].data_ptr = (void*)addr;
		} else {
			dst_ctm_ptr->map[i].data_ptr = NULL;
			dst_ctm_ptr->map[i].size = 0;
		}
	}

	if ((PNULL != dst_ctm_ptr->final_map.data_ptr)\
		&&(dst_ctm_ptr->final_map.size < dst_ctm_ptr->map[0].size)) {

		ISP_LOGV("free result buffer=%p, size=%d\n",
			dst_ctm_ptr->final_map.data_ptr, dst_ctm_ptr->final_map.size);
		free (dst_ctm_ptr->final_map.data_ptr);
		dst_ctm_ptr->final_map.data_ptr= PNULL;
		dst_ctm_ptr->final_map.size = 0;
	}

	if (PNULL == dst_ctm_ptr->final_map.data_ptr) {
		dst_ctm_ptr->final_map.data_ptr = (void*)malloc(dst_ctm_ptr->map[index].size);
		if (PNULL == dst_ctm_ptr->final_map.data_ptr) {
			rtn = ISP_ERROR;
			ISP_LOGE("_pm_lnc_init: malloc failed\n");
			return rtn;
		}

		dst_ctm_ptr->final_map.size = dst_ctm_ptr->map[index].size;

		ISP_LOGV("result buffer=%p, size=%d", dst_ctm_ptr->final_map.data_ptr, dst_ctm_ptr->final_map.size);
	}
	memcpy((void*)dst_ctm_ptr->final_map.data_ptr, dst_ctm_ptr->map[index].data_ptr, dst_ctm_ptr->map[index].size);
	dst_ctm_ptr->final_map.size = dst_ctm_ptr->map[index].size;
	//dst_ctm_ptr->cur.data_ptr = dst_ctm_ptr->final_map.data_ptr;
#if __WORDSIZE == 64
	dst_ctm_ptr->cur.data_ptr[0] = (isp_uint)(dst_ctm_ptr->final_map.data_ptr) & 0xffffffff;
	dst_ctm_ptr->cur.data_ptr[1] = (isp_uint)(dst_ctm_ptr->final_map.data_ptr) >> 32;
#else
	dst_ctm_ptr->cur.data_ptr[0] = (isp_uint)(dst_ctm_ptr->final_map.data_ptr);
	dst_ctm_ptr->cur.data_ptr[1] = 0;
#endif
	dst_ctm_ptr->cur.size = dst_ctm_ptr->final_map.size;
	dst_ctm_ptr->cur.bypass = ctm_header_ptr->bypass;
	ctm_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_ctm_set_param(void *ctm_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_ctm_param *ctm_ptr = (struct isp_ctm_param *)ctm_param;
	struct isp_pm_block_header *ctm_header_ptr = (struct isp_pm_block_header*)param_ptr1;

	ctm_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_CTM_BYPASS:
		ctm_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_SMART_SETTING:
	{
		struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;
		struct isp_weight_value *weight_value = NULL;
		struct isp_range val_range = {0};
		struct isp_weight_value ctm_value = {{0}, {0}};

		ctm_header_ptr->is_update = 0;
		val_range.min = 0;
		val_range.max = SENSOR_CTM_NUM - 1;
		rtn = _pm_check_smart_param(block_result, &val_range, 1,
					ISP_SMART_Y_TYPE_WEIGHT_VALUE);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !\n");
			return rtn;
		}

		weight_value = (struct isp_weight_value *)block_result->component[0].fix_data;
		ctm_value = *weight_value;

		ISP_LOGV("ISP_SMART: value=(%d, %d), weight=(%d, %d)", ctm_value.value[0],
				ctm_value.value[1], ctm_value.weight[0], ctm_value.weight[1]);

		ctm_value.weight[0] = ctm_value.weight[0] / (SMART_WEIGHT_UNIT / 16)
							* (SMART_WEIGHT_UNIT / 16);
		ctm_value.weight[1] = SMART_WEIGHT_UNIT - ctm_value.weight[0];

		if (ctm_value.weight[0] != ctm_ptr->cur_idx.weight0
			|| ctm_value.value[0] != ctm_ptr->cur_idx.x0) {
			void *src[2] = {NULL};
			void *dst = NULL;
			isp_u32 data_num = 0;
			isp_u32 index = 0;

			index = ctm_value.value[0];
			src[0] = (void*)ctm_ptr->map[ctm_value.value[0]].data_ptr;
			src[1] = (void*)ctm_ptr->map[ctm_value.value[1]].data_ptr;
			dst = (void*)ctm_ptr->final_map.data_ptr;
			data_num = ctm_ptr->final_map.size / sizeof(isp_u8);

			rtn = isp_interp_data(dst, src, ctm_value.weight, data_num, ISP_INTERP_UINT8);
			if (ISP_SUCCESS == rtn) {
				ctm_ptr->cur_idx.x0 = ctm_value.value[0];
				ctm_ptr->cur_idx.x1 = ctm_value.value[1];
				ctm_ptr->cur_idx.weight0 = ctm_value.weight[0];
				ctm_ptr->cur_idx.weight1 = ctm_value.weight[1];
				ctm_header_ptr->is_update = 1;
			}
		}
	}
	break;

	default:
		ctm_header_ptr->is_update = ISP_ZERO;
	break;
	}

	ISP_LOGV("ISP_SMART: cmd=%d, update=%d, value=(%d, %d), weight=(%d, %d)\n", cmd, ctm_header_ptr->is_update,
				ctm_ptr->cur_idx.x0, ctm_ptr->cur_idx.x1,
				ctm_ptr->cur_idx.weight0, ctm_ptr->cur_idx.weight1);

	return rtn;
}

static isp_s32 _pm_ctm_get_param(void *ctm_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_ctm_param *ctm_ptr = (struct isp_ctm_param *)ctm_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_CTM;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &ctm_ptr->cur;
		param_data_ptr->data_size = sizeof(ctm_ptr->cur);
		*update_flag = 0;
	break;

	default:
	break;
	}

	return rtn;
}

static isp_s32 _pm_ctm_deinit(void* ctm_param)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_ctm_param *ctm_ptr = (struct isp_ctm_param *)ctm_param;

	if (PNULL != ctm_ptr->final_map.data_ptr) {
		free (ctm_ptr->final_map.data_ptr);
		ctm_ptr->final_map.data_ptr = PNULL;
		ctm_ptr->final_map.size = 0;
	}

	return rtn;
}

static isp_s32 _pm_cce_adjust_hue_saturation(struct isp_cce_param_v1 *cce_param, isp_u32 hue, isp_u32 saturation)
{
		isp_s32 rtn = ISP_SUCCESS;
		isp_u32 cce_coef_index = 0;
		isp_u16 cce_coef[3] = {0};
		isp_u32 cur_idx = 0;
		struct isp_rgb_gains rgb_gain = {0x00};
		struct isp_cce_param_v1 *dst_cce_ptr = (struct isp_cce_param_v1*)cce_param;
		isp_u32 i = 0x00;
		isp_u16 src_matrix[9] = {0};
		isp_u16 dst_matrix[9] = {0};

#if 0
		ISP_LOGI("origin: **********************");
		ISP_LOGI("cur index = %d, is effect=%d", dst_cce_ptr->cur_idx, dst_cce_ptr->is_specialeffect);
		ISP_LOGI("color coef: %d, %d, %d", dst_cce_ptr->cce_coef[ISP_CCE_COEF_COLOR_CAST][0],
					dst_cce_ptr->cce_coef[ISP_CCE_COEF_COLOR_CAST][1],
					dst_cce_ptr->cce_coef[ISP_CCE_COEF_COLOR_CAST][2]);
		ISP_LOGI("gain coef: %d, %d, %d", dst_cce_ptr->cce_coef[ISP_CCE_COEF_GAIN_OFFSET][0],
					dst_cce_ptr->cce_coef[ISP_CCE_COEF_GAIN_OFFSET][1],
					dst_cce_ptr->cce_coef[ISP_CCE_COEF_GAIN_OFFSET][2]);
		ISP_LOGI("hue = %d, sat= % d", dst_cce_ptr->cur_level[0], dst_cce_ptr->cur_level[1]);
		ISP_LOGI("matrix: m=[%d, %d, %d; %d, %d, %d; %d, %d, %d], shift=[%d, %d, %d]",
			dst_cce_ptr->cur.matrix[0], dst_cce_ptr->cur.matrix[1], dst_cce_ptr->cur.matrix[2],
			dst_cce_ptr->cur.matrix[3], dst_cce_ptr->cur.matrix[4], dst_cce_ptr->cur.matrix[5],
			dst_cce_ptr->cur.matrix[6], dst_cce_ptr->cur.matrix[7], dst_cce_ptr->cur.matrix[8],
			dst_cce_ptr->cur.y_offset, dst_cce_ptr->cur.u_offset, dst_cce_ptr->cur.v_offset);
#endif

		ISP_LOGV("target: hue = %d, sat= % d", hue, saturation);

		rtn = isp_hue_saturation_2_gain(hue, saturation, &rgb_gain);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("isp_hue_saturation_2_gain failed\n");
			return ISP_ERROR;
		}

		cce_coef_index = ISP_CCE_COEF_COLOR_CAST;
		dst_cce_ptr->cce_coef[cce_coef_index][0] = rgb_gain.gain_r;
		dst_cce_ptr->cce_coef[cce_coef_index][1] = rgb_gain.gain_g;
		dst_cce_ptr->cce_coef[cce_coef_index][2] = rgb_gain.gain_b;

		dst_cce_ptr->cur_level[0] = hue;
		dst_cce_ptr->cur_level[1] = saturation;

		for(i=0; i<3; i++){
			uint32_t color_cast_coef = dst_cce_ptr->cce_coef[ISP_CCE_COEF_GAIN_OFFSET][i];
			uint32_t gain_offset_coef = dst_cce_ptr->cce_coef[ISP_CCE_COEF_COLOR_CAST][i];

			cce_coef[i] = gain_offset_coef * color_cast_coef / ISP_FIX_8BIT_UNIT;
		}

		ISP_LOGV("multiply: %d, %d, %d", cce_coef[0],cce_coef[1],cce_coef[2]);

		cur_idx = dst_cce_ptr->cur_idx;

		for (i = 0; i < 9; ++i){
			src_matrix[i] = (isp_u16)dst_cce_ptr->cce_tab[cur_idx].matrix[i];
		}

		isp_cce_adjust(src_matrix, cce_coef, dst_matrix, ISP_FIX_10BIT_UNIT);

		for (i = 0; i < 9; ++i){
			dst_cce_ptr->cur.matrix[i] = (uint32_t)dst_matrix[i];
		}

		dst_cce_ptr->cur.y_offset= dst_cce_ptr->cce_tab[cur_idx].y_offset;
		dst_cce_ptr->cur.u_offset= dst_cce_ptr->cce_tab[cur_idx].u_offset;
		dst_cce_ptr->cur.v_offset= dst_cce_ptr->cce_tab[cur_idx].v_offset;

#if 0
		ISP_LOGI("output: ");
		ISP_LOGI("cur index = %d, is effect=%d", dst_cce_ptr->cur_idx, dst_cce_ptr->is_specialeffect);
		ISP_LOGI("color coef: %d, %d, %d", dst_cce_ptr->cce_coef[ISP_CCE_COEF_COLOR_CAST][0],
					dst_cce_ptr->cce_coef[ISP_CCE_COEF_COLOR_CAST][1],
					dst_cce_ptr->cce_coef[ISP_CCE_COEF_COLOR_CAST][2]);
		ISP_LOGI("gain coef: %d, %d, %d", dst_cce_ptr->cce_coef[ISP_CCE_COEF_GAIN_OFFSET][0],
					dst_cce_ptr->cce_coef[ISP_CCE_COEF_GAIN_OFFSET][1],
					dst_cce_ptr->cce_coef[ISP_CCE_COEF_GAIN_OFFSET][2]);
		ISP_LOGI("hue = %d, sat= % d", dst_cce_ptr->cur_level[0], dst_cce_ptr->cur_level[1]);
		ISP_LOGI("matrix: m=[%d, %d, %d; %d, %d, %d; %d, %d, %d], shift=[%d, %d, %d]",
			dst_cce_ptr->cur.matrix[0], dst_cce_ptr->cur.matrix[1], dst_cce_ptr->cur.matrix[2],
			dst_cce_ptr->cur.matrix[3], dst_cce_ptr->cur.matrix[4], dst_cce_ptr->cur.matrix[5],
			dst_cce_ptr->cur.matrix[6], dst_cce_ptr->cur.matrix[7], dst_cce_ptr->cur.matrix[8],
			dst_cce_ptr->cur.y_offset, dst_cce_ptr->cur.u_offset, dst_cce_ptr->cur.v_offset);
#endif

		return rtn;
}

static isp_s32 _pm_cce_adjust_gain_offset(struct isp_cce_param_v1 *cce_param, isp_u16 r_gain, isp_u16 g_gain, isp_u16 b_gain)
{
		isp_s32 rtn = ISP_SUCCESS;
		isp_u32 cce_coef_index = 0;
		isp_u16 cce_coef[3] = {0};
		struct isp_rgb_gains rgb_gain = {0x00};
		struct isp_cce_param_v1 *dst_cce_ptr = (struct isp_cce_param_v1*)cce_param;
		isp_u32 i = 0x00;
		isp_u32 cur_idx = 0;
		isp_u16 src_matrix[9] = {0};
		isp_u16 dst_matrix[9] = {0};

#if 0
		ISP_LOGI("origin: **********************");
		ISP_LOGI("cur index = %d, is effect=%d", dst_cce_ptr->cur_idx, dst_cce_ptr->is_specialeffect);
		ISP_LOGI("color coef: %d, %d, %d", dst_cce_ptr->cce_coef[ISP_CCE_COEF_COLOR_CAST][0],
					dst_cce_ptr->cce_coef[ISP_CCE_COEF_COLOR_CAST][1],
					dst_cce_ptr->cce_coef[ISP_CCE_COEF_COLOR_CAST][2]);
		ISP_LOGI("gain coef: %d, %d, %d", dst_cce_ptr->cce_coef[ISP_CCE_COEF_GAIN_OFFSET][0],
					dst_cce_ptr->cce_coef[ISP_CCE_COEF_GAIN_OFFSET][1],
					dst_cce_ptr->cce_coef[ISP_CCE_COEF_GAIN_OFFSET][2]);
		ISP_LOGI("hue = %d, sat= % d", dst_cce_ptr->cur_level[0], dst_cce_ptr->cur_level[1]);
		ISP_LOGI("matrix: m=[%d, %d, %d; %d, %d, %d; %d, %d, %d], shift=[%d, %d, %d]",
			dst_cce_ptr->cur.matrix[0], dst_cce_ptr->cur.matrix[1], dst_cce_ptr->cur.matrix[2],
			dst_cce_ptr->cur.matrix[3], dst_cce_ptr->cur.matrix[4], dst_cce_ptr->cur.matrix[5],
			dst_cce_ptr->cur.matrix[6], dst_cce_ptr->cur.matrix[7], dst_cce_ptr->cur.matrix[8],
			dst_cce_ptr->cur.y_offset, dst_cce_ptr->cur.u_offset, dst_cce_ptr->cur.v_offset);
#endif
		ISP_LOGI("target: gain: %d, %d, %d", r_gain, g_gain, b_gain);

		if (0 == r_gain || 0 == g_gain || 0 == b_gain) {
			r_gain = ISP_FIX_8BIT_UNIT;
			g_gain = ISP_FIX_8BIT_UNIT;
			b_gain = ISP_FIX_8BIT_UNIT;
		}

		cce_coef_index = ISP_CCE_COEF_GAIN_OFFSET;
		dst_cce_ptr->cce_coef[cce_coef_index][0] = r_gain;
		dst_cce_ptr->cce_coef[cce_coef_index][1] = g_gain;
		dst_cce_ptr->cce_coef[cce_coef_index][2] = b_gain;

		for(i=0; i<3; i++){
			uint32_t color_cast_coef = dst_cce_ptr->cce_coef[ISP_CCE_COEF_GAIN_OFFSET][i];
			uint32_t gain_offset_coef = dst_cce_ptr->cce_coef[ISP_CCE_COEF_COLOR_CAST][i];

			cce_coef[i] = gain_offset_coef * color_cast_coef / ISP_FIX_8BIT_UNIT;
		}

		cur_idx = dst_cce_ptr->cur_idx;

		for (i = 0; i < 9; ++i){
			src_matrix[i] = (isp_u16)dst_cce_ptr->cce_tab[cur_idx].matrix[i];
		}

		isp_cce_adjust(src_matrix, cce_coef, dst_matrix, ISP_FIX_10BIT_UNIT);

		for (i = 0; i < 9; ++i){
			dst_cce_ptr->cur.matrix[i] = (uint32_t)dst_matrix[i];
		}

		dst_cce_ptr->cur.y_offset = dst_cce_ptr->cce_tab[cur_idx].y_offset;
		dst_cce_ptr->cur.u_offset = dst_cce_ptr->cce_tab[cur_idx].u_offset;
		dst_cce_ptr->cur.v_offset = dst_cce_ptr->cce_tab[cur_idx].v_offset;

#if 0
		ISP_LOGI("output: **********************");
		ISP_LOGI("cur index = %d, is effect=%d", dst_cce_ptr->cur_idx, dst_cce_ptr->is_specialeffect);
		ISP_LOGI("color coef: %d, %d, %d", dst_cce_ptr->cce_coef[ISP_CCE_COEF_COLOR_CAST][0],
					dst_cce_ptr->cce_coef[ISP_CCE_COEF_COLOR_CAST][1],
					dst_cce_ptr->cce_coef[ISP_CCE_COEF_COLOR_CAST][2]);
		ISP_LOGI("gain coef: %d, %d, %d", dst_cce_ptr->cce_coef[ISP_CCE_COEF_GAIN_OFFSET][0],
					dst_cce_ptr->cce_coef[ISP_CCE_COEF_GAIN_OFFSET][1],
					dst_cce_ptr->cce_coef[ISP_CCE_COEF_GAIN_OFFSET][2]);
		ISP_LOGI("hue = %d, sat= % d", dst_cce_ptr->cur_level[0], dst_cce_ptr->cur_level[1]);
		ISP_LOGI("matrix: m=[%d, %d, %d; %d, %d, %d; %d, %d, %d], shift=[%d, %d, %d]",
			dst_cce_ptr->cur.matrix[0], dst_cce_ptr->cur.matrix[1], dst_cce_ptr->cur.matrix[2],
			dst_cce_ptr->cur.matrix[3], dst_cce_ptr->cur.matrix[4], dst_cce_ptr->cur.matrix[5],
			dst_cce_ptr->cur.matrix[6], dst_cce_ptr->cur.matrix[7], dst_cce_ptr->cur.matrix[8],
			dst_cce_ptr->cur.y_offset, dst_cce_ptr->cur.u_offset, dst_cce_ptr->cur.v_offset);
#endif

		return rtn;
}

static isp_s32 _pm_cce_adjust(struct isp_cce_param_v1 *cce_param)
{
	isp_s32 rtn0 = ISP_SUCCESS;
	isp_s32 rtn = ISP_SUCCESS;

	isp_s32 hue = cce_param->cur_level[0];
	isp_s32 saturation = cce_param->cur_level[1];
	isp_u16 r_gain = cce_param->cce_coef[ISP_CCE_COEF_GAIN_OFFSET][0];
	isp_u16 g_gain = cce_param->cce_coef[ISP_CCE_COEF_GAIN_OFFSET][1];
	isp_u16 b_gain = cce_param->cce_coef[ISP_CCE_COEF_GAIN_OFFSET][2];

	if (0 != cce_param->is_specialeffect) {
		ISP_LOGI("do not adjust cce for specialeffect");
		return ISP_SUCCESS;
	}

	rtn0 = _pm_cce_adjust_hue_saturation(cce_param, hue, saturation);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("_pm_cce_adjust_hue_saturation failed");
		rtn = rtn0;
	}

	rtn0= _pm_cce_adjust_gain_offset(cce_param, r_gain, g_gain, b_gain);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("_pm_cce_adjust_hue_saturation failed");
		rtn = rtn0;
	}

	return rtn;

}

static isp_s32 _pm_cce_init_v1(void *dst_cce_param, void *src_cce_param, void* param1, void* param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i = 0x00, j = 0, index = 0;
	struct sensor_cce_param_v1 *src_ptr = (struct sensor_cce_param_v1*)src_cce_param;
	struct isp_cce_param_v1 *dst_ptr = (struct isp_cce_param_v1*)dst_cce_param;
	struct isp_pm_block_header *cce_header_ptr = (struct isp_pm_block_header*)param1;
	UNUSED(param2);

	memset((void*)&dst_ptr->cur,0x00,sizeof(dst_ptr->cur));
	dst_ptr->is_specialeffect = 0;

	for (i = 0; i < SENSOR_CCE_NUM; ++i) {
		for(j = 0x00; j < 9; ++j) {
			dst_ptr->cce_tab[i].matrix[j] = src_ptr->tab[i].matrix[j];
		}
		dst_ptr->cce_tab[i].u_offset = src_ptr->tab[i].u_shift;
		dst_ptr->cce_tab[i].v_offset = src_ptr->tab[i].v_shift;
		dst_ptr->cce_tab[i].y_offset = src_ptr->tab[i].y_shift;
	}
	index = src_ptr->cur_idx;

	for (i = 0; i < MAX_SPECIALEFFECT_NUM; ++i) {
		for (j = 0; j < 9; ++j) {
			dst_ptr->specialeffect_tab[i].matrix[j] = src_ptr->specialeffect[i].matrix[j];
		}
		dst_ptr->specialeffect_tab[i].y_offset = src_ptr->specialeffect[i].y_shift;
		dst_ptr->specialeffect_tab[i].u_offset = src_ptr->specialeffect[i].u_shift;
		dst_ptr->specialeffect_tab[i].v_offset = src_ptr->specialeffect[i].v_shift;
	}

	for (i = 0; i < 9; ++i) {
		dst_ptr->cur.matrix[i] = src_ptr->tab[index].matrix[i];
	}

	dst_ptr->cur.u_offset = src_ptr->tab[index].u_shift;
	dst_ptr->cur.v_offset = src_ptr->tab[index].v_shift;
	dst_ptr->cur.y_offset = src_ptr->tab[index].y_shift;

	for (i = 0; i < 2; ++i) {
		for (j = 0; j < 3; ++j){
			if (ISP_CCE_COEF_COLOR_CAST == i){
				dst_ptr->cce_coef[i][j] = ISP_FIX_10BIT_UNIT;
			} else if (ISP_CCE_COEF_GAIN_OFFSET == i){
				dst_ptr->cce_coef[i][j] = ISP_FIX_8BIT_UNIT;
			}
		}
	}

	dst_ptr->cur_idx = index;
	dst_ptr->prv_idx = 0;;

	_pm_cce_adjust(dst_ptr);

	cce_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_cce_set_param_v1(void *cce_param, isp_u32 cmd, void* param_ptr0, void* param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_cce_param_v1 *dst_cce_ptr = (struct isp_cce_param_v1*)cce_param;
	struct isp_pm_block_header *cce_header_ptr = (struct isp_pm_block_header*)param_ptr1;

	cce_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_CCE:
	{
		isp_u32 idx = 0;
		isp_u32 i = 0x00;
		idx = *((isp_u32*)param_ptr0);

		for (i = 0; i < 9; ++i) {
			dst_cce_ptr->cur.matrix[i] = dst_cce_ptr->cce_tab[idx].matrix[i];
		}

		dst_cce_ptr->cur.y_offset = dst_cce_ptr->cce_tab[idx].y_offset;
		dst_cce_ptr->cur.u_offset = dst_cce_ptr->cce_tab[idx].u_offset;
		dst_cce_ptr->cur.v_offset = dst_cce_ptr->cce_tab[idx].v_offset;

		dst_cce_ptr->cur_idx = idx;

		dst_cce_ptr->is_specialeffect = 0;

		_pm_cce_adjust(dst_cce_ptr);
	}
	break;

	case ISP_PM_BLK_SPECIAL_EFFECT:
	{
		isp_u32 idx = *((isp_u32*)param_ptr0);
		dst_cce_ptr->prv_idx = dst_cce_ptr->cur_idx;
		dst_cce_ptr->cur_idx = idx;
		if (0 == idx) {
			dst_cce_ptr->is_specialeffect = 0;
			dst_cce_ptr->cur = dst_cce_ptr->cce_tab[dst_cce_ptr->cur_idx];
		} else {
			dst_cce_ptr->is_specialeffect = 1;
			dst_cce_ptr->cur = dst_cce_ptr->specialeffect_tab[dst_cce_ptr->cur_idx];
		}
		_pm_cce_adjust(dst_cce_ptr);
	}
	break;

	case ISP_PM_BLK_SMART_SETTING:
		{
			struct isp_range val_range = {0};
			struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;

			if (block_result->update == 0){
				return rtn;
			}

			if (dst_cce_ptr->is_specialeffect != 0) {
				/*special effect mode, do not adjust cce*/
				return ISP_SUCCESS;
			}

			cce_header_ptr->is_update = ISP_ZERO;

			if ((0 == dst_cce_ptr->cur_idx) && (dst_cce_ptr->cur_idx != dst_cce_ptr->prv_idx)) {
				cce_header_ptr->is_update = ISP_ONE;
			}

			if (ISP_SMART_COLOR_CAST == block_result->smart_id) {

				isp_s32 adjust_hue = dst_cce_ptr->cur_level[0];
				isp_s32 adjust_saturation = dst_cce_ptr->cur_level[1];

				val_range.min = 0;
				val_range.max = 360;
				rtn = _pm_check_smart_param(block_result, &val_range, 2, ISP_SMART_Y_TYPE_VALUE);
				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !\n");
					return rtn;
				}

				adjust_hue = block_result->component[0].fix_data[0];
				adjust_saturation = block_result->component[1].fix_data[0];

				if (adjust_hue != dst_cce_ptr->cur_level[0]
					|| adjust_saturation != dst_cce_ptr->cur_level[1]) {

					rtn = _pm_cce_adjust_hue_saturation(dst_cce_ptr, adjust_hue, adjust_saturation);
					if (ISP_SUCCESS == rtn)
						cce_header_ptr->is_update = ISP_ONE;
				}

			}else if (ISP_SMART_GAIN_OFFSET == block_result->smart_id) {
				isp_u16 r_gain = 0;
				isp_u16 g_gain = 0;
				isp_u16 b_gain = 0;

				val_range.min = 0;
				val_range.max = 360;
				rtn = _pm_check_smart_param(block_result, &val_range, 3, ISP_SMART_Y_TYPE_VALUE);
				if (ISP_SUCCESS != rtn) {
					ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !\n");
					return rtn;
				}

				r_gain = block_result->component[0].fix_data[0];
				g_gain = block_result->component[1].fix_data[0];
				b_gain = block_result->component[2].fix_data[0];

				if (dst_cce_ptr->cce_coef[ISP_CCE_COEF_GAIN_OFFSET][0] != r_gain ||
					dst_cce_ptr->cce_coef[ISP_CCE_COEF_GAIN_OFFSET][1] != g_gain ||
					dst_cce_ptr->cce_coef[ISP_CCE_COEF_GAIN_OFFSET][2] != b_gain) {

					rtn = _pm_cce_adjust_gain_offset(dst_cce_ptr, r_gain, g_gain, b_gain);
					if (ISP_SUCCESS == rtn)
						cce_header_ptr->is_update = ISP_ONE;
				}

			}
		}
		break;

		default:
			cce_header_ptr->is_update = ISP_ZERO;
		break;
	}

	return rtn;
}

static isp_s32 _pm_cce_get_param_v1(void *cce_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_cce_param_v1 *cce_ptr = (struct isp_cce_param_v1*)cce_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_CCE_V1;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
	{
		param_data_ptr->data_ptr = (void*)&cce_ptr->cur;
		param_data_ptr->data_size = sizeof(cce_ptr->cur);
		*update_flag = 0;
	}
	break;

	default:
	break;
	}

	return rtn;
}

static isp_s32 _pm_hsv_init(void *dst_hsv_param, void *src_hsv_param, void* param1, void* param2)
{
	isp_u32 i = 0;
	isp_u32 index = 0;
	isp_s32 rtn = ISP_SUCCESS;
	intptr_t addr = 0;
	isp_u32 *buf_ptr = PNULL;
	struct isp_hsv_param *dst_hsv_ptr = (struct isp_hsv_param *)dst_hsv_param;
	struct sensor_hsv_param *src_hsv_ptr = (struct sensor_hsv_param *)src_hsv_param;
	struct isp_pm_block_header *hsv_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);

	index = src_hsv_ptr->cur_idx.x0;
	dst_hsv_ptr->cur_idx.x0 = src_hsv_ptr->cur_idx.x0;
	dst_hsv_ptr->cur_idx.x1 = src_hsv_ptr->cur_idx.x1;
	dst_hsv_ptr->cur_idx.weight0 = src_hsv_ptr->cur_idx.weight0;
	dst_hsv_ptr->cur_idx.weight1 = src_hsv_ptr->cur_idx.weight1;
	for (i = 0; i < SENSOR_HSV_NUM; i++) {
		dst_hsv_ptr->map[i].size = src_hsv_ptr->map[i].size;
		addr = (intptr_t)&src_hsv_ptr->data_area + src_hsv_ptr->map[i].offset;
		dst_hsv_ptr->map[i].data_ptr = (void*)addr;
	}
	for (i=0; i<MAX_SPECIALEFFECT_NUM; i++) {
		dst_hsv_ptr->specialeffect_tab[i].size = src_hsv_ptr->specialeffect[i].size;
		addr = (intptr_t)(&src_hsv_ptr->specialeffect_data_area + src_hsv_ptr->specialeffect[i].offset);
		dst_hsv_ptr->specialeffect_tab[i].data_ptr = (void*)addr;
	}

	if (PNULL == dst_hsv_ptr->final_map.data_ptr) {
		dst_hsv_ptr->final_map.data_ptr = (void*)malloc(src_hsv_ptr->map[index].size);
		if (PNULL == dst_hsv_ptr->final_map.data_ptr) {
			ISP_LOGE("malloc failed\n");
			rtn = ISP_ERROR;
			return rtn;
		}
	}
	memcpy((void*)dst_hsv_ptr->final_map.data_ptr, dst_hsv_ptr->map[index].data_ptr, dst_hsv_ptr->map[index].size);
	buf_ptr = (isp_u32*)dst_hsv_ptr->final_map.data_ptr;
	dst_hsv_ptr->final_map.size = dst_hsv_ptr->map[index].size;
	//dst_hsv_ptr->cur.data_ptr = dst_hsv_ptr->final_map.data_ptr;
#if __WORDSIZE == 64
	dst_hsv_ptr->cur.data_ptr[0] = (isp_uint)(dst_hsv_ptr->final_map.data_ptr) & 0xffffffff;
	dst_hsv_ptr->cur.data_ptr[1] = (isp_uint)(dst_hsv_ptr->final_map.data_ptr) >> 32;
#else
	dst_hsv_ptr->cur.data_ptr[0] = (isp_uint)(dst_hsv_ptr->final_map.data_ptr);
	dst_hsv_ptr->cur.data_ptr[1] = 0;
#endif
	dst_hsv_ptr->cur.size = dst_hsv_ptr->final_map.size;
	dst_hsv_ptr->cur.buf_sel = 0x0;
	dst_hsv_ptr->cur.bypass = hsv_header_ptr->bypass;
	hsv_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_hsv_set_param(void *hsv_param, isp_u32 cmd, void* param_ptr0, void* param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_hsv_param *dst_hsv_ptr = (struct isp_hsv_param *)hsv_param;
	struct isp_pm_block_header *hsv_header_ptr = (struct isp_pm_block_header*)param_ptr1;
	hsv_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_SMART_SETTING:
	{
		struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;
		struct isp_weight_value *weight_value = NULL;
		struct isp_range val_range = {0};
		struct isp_weight_value hsv_value = {{0}, {0}};

		hsv_header_ptr->is_update = 0;
		val_range.min = 0;
		val_range.max = 255;
		rtn = _pm_check_smart_param(block_result, &val_range, 1,
					ISP_SMART_Y_TYPE_WEIGHT_VALUE);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !\n");
			return rtn;
		}

		weight_value = (struct isp_weight_value *)block_result->component[0].fix_data;
		hsv_value = *weight_value;

		ISP_LOGI("ISP_SMART: value=(%d, %d), weight=(%d, %d)", hsv_value.value[0],
				hsv_value.value[1], hsv_value.weight[0], hsv_value.weight[1]);

		hsv_value.weight[0] = hsv_value.weight[0] / (SMART_WEIGHT_UNIT / 16)
							* (SMART_WEIGHT_UNIT / 16);
		hsv_value.weight[1] = SMART_WEIGHT_UNIT - hsv_value.weight[0];

		if (hsv_value.weight[0] != dst_hsv_ptr->cur_idx.weight0
			|| hsv_value.value[0] != dst_hsv_ptr->cur_idx.x0) {
			void *src[2] = {NULL};
			void *dst = NULL;
			isp_u32 data_num = 0;
			isp_u32 index = 0;

			index = hsv_value.value[0];
			src[0] = (void*)dst_hsv_ptr->map[hsv_value.value[0]].data_ptr;
			src[1] = (void*)dst_hsv_ptr->map[hsv_value.value[1]].data_ptr;
			dst = (void*)dst_hsv_ptr->final_map.data_ptr;
			data_num = dst_hsv_ptr->final_map.size / sizeof(isp_u32);

			rtn = isp_interp_data(dst, src, hsv_value.weight, data_num, ISP_INTERP_UINT20);
			if (ISP_SUCCESS == rtn) {
				dst_hsv_ptr->cur_idx.x0 = hsv_value.value[0];
				dst_hsv_ptr->cur_idx.x1 = hsv_value.value[1];
				dst_hsv_ptr->cur_idx.weight0 = hsv_value.weight[0];
				dst_hsv_ptr->cur_idx.weight1 = hsv_value.weight[1];
				hsv_header_ptr->is_update = 1;
			}
		}
	}
	break;

	case ISP_PM_BLK_SPECIAL_EFFECT:
	{
		isp_u32 idx = *((isp_u32*)param_ptr0);
		if (0 == idx) {
			dst_hsv_ptr->cur.buf_sel = 0;
			dst_hsv_ptr->cur.size = dst_hsv_ptr->map[dst_hsv_ptr->cur_idx.x0].size;
			//dst_hsv_ptr->cur.data_ptr = dst_hsv_ptr->map[dst_hsv_ptr->cur_idx.x0].data_ptr;
		#if __WORDSIZE == 64
			dst_hsv_ptr->cur.data_ptr[0] = (isp_uint)(dst_hsv_ptr->map[dst_hsv_ptr->cur_idx.x0].data_ptr) & 0xffffffff;
			dst_hsv_ptr->cur.data_ptr[1] = (isp_uint)(dst_hsv_ptr->map[dst_hsv_ptr->cur_idx.x0].data_ptr) >> 32;
		#else
			dst_hsv_ptr->cur.data_ptr[0] = (isp_uint)(dst_hsv_ptr->map[dst_hsv_ptr->cur_idx.x0].data_ptr);
			dst_hsv_ptr->cur.data_ptr[1] = 0;
		#endif
		} else {
			dst_hsv_ptr->cur.buf_sel = 0;
			dst_hsv_ptr->cur.size = dst_hsv_ptr->specialeffect_tab[idx].size;
			//dst_hsv_ptr->cur.data_ptr = dst_hsv_ptr->specialeffect_tab[idx].data_ptr;
		#if __WORDSIZE == 64
			dst_hsv_ptr->cur.data_ptr[0] = (isp_uint)(dst_hsv_ptr->specialeffect_tab[idx].data_ptr) & 0xffffffff;
			dst_hsv_ptr->cur.data_ptr[1] = (isp_uint)(dst_hsv_ptr->specialeffect_tab[idx].data_ptr) >> 32;
		#else
			dst_hsv_ptr->cur.data_ptr[0] = (isp_uint)(dst_hsv_ptr->specialeffect_tab[idx].data_ptr);
			dst_hsv_ptr->cur.data_ptr[1] = 0;
		#endif
		}
	}
	break;

	default:
		hsv_header_ptr->is_update = ISP_ZERO;
		break;
	}

	ISP_LOGE("ISP_SMART: cmd=%d, update=%d, value=(%d, %d), weight=(%d, %d)\n", cmd, hsv_header_ptr->is_update,
				dst_hsv_ptr->cur_idx.x0, dst_hsv_ptr->cur_idx.x1,
				dst_hsv_ptr->cur_idx.weight0, dst_hsv_ptr->cur_idx.weight1);

	return rtn;
}

static isp_s32 _pm_hsv_get_param(void *hsv_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_hsv_param *hsv_ptr = (struct isp_hsv_param *)hsv_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_HSV;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void*)&hsv_ptr->cur;
		param_data_ptr->data_size = sizeof(hsv_ptr->cur);
		*update_flag = 0;
	break;

	default:
	break;
	}

	return rtn;
}

static isp_s32 _pm_hsv_deinit(void* hsv_param)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_hsv_param *hsv_ptr = (struct isp_hsv_param *)hsv_param;

	if (PNULL != hsv_ptr->final_map.data_ptr) {
		free (hsv_ptr->final_map.data_ptr);
		hsv_ptr->final_map.data_ptr = PNULL;
		hsv_ptr->final_map.size = 0;
	}

	return rtn;
}
static isp_s32 _pm_csc_init(void *dst_csc_param, void *src_csc_param, void* param1, void* param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i;
	isp_u32 cur_index;
	struct sensor_radial_csc_param *src_ptr = (struct sensor_radial_csc_param*)src_csc_param;
	struct isp_csc_param *dst_ptr = (struct isp_csc_param*)dst_csc_param;
	struct isp_pm_block_header *csc_header_ptr = (struct isp_pm_block_header*)param1;
	struct isp_size *img_size_ptr = (struct isp_size*)param2;

	memset((void*)&dst_ptr->cur,0x00,sizeof(dst_ptr->cur));
	dst_ptr->cur.bypass = csc_header_ptr->bypass;
	for( i = 0; i < SENSOR_LENS_NUM; i++ ) {
		dst_ptr->map[i].r_curve_distcptn.center_pos.x = src_ptr->map[i].r_curve_distcptn.center_pos.x;
		dst_ptr->map[i].r_curve_distcptn.center_pos.y = src_ptr->map[i].r_curve_distcptn.center_pos.y;
		dst_ptr->map[i].b_curve_distcptn.center_pos.x = src_ptr->map[i].b_curve_distcptn.center_pos.x;
		dst_ptr->map[i].b_curve_distcptn.center_pos.y = src_ptr->map[i].b_curve_distcptn.center_pos.y;
		dst_ptr->map[i].r_curve_distcptn.delta_square.x = src_ptr->map[i].r_curve_distcptn.delta_square.x;
		dst_ptr->map[i].r_curve_distcptn.delta_square.y = src_ptr->map[i].r_curve_distcptn.delta_square.y;
		dst_ptr->map[i].b_curve_distcptn.delta_square.x = src_ptr->map[i].b_curve_distcptn.delta_square.x;
		dst_ptr->map[i].b_curve_distcptn.delta_square.y = src_ptr->map[i].b_curve_distcptn.delta_square.y;
		dst_ptr->map[i].red_thr = src_ptr->map[i].red_thr;
		dst_ptr->map[i].blue_thr = src_ptr->map[i].blue_thr;
		dst_ptr->map[i].r_curve_distcptn.coef.p1 = src_ptr->map[i].r_curve_distcptn.coef.p1;
		dst_ptr->map[i].r_curve_distcptn.coef.p2 = src_ptr->map[i].r_curve_distcptn.coef.p2;
		dst_ptr->map[i].b_curve_distcptn.coef.p1 = src_ptr->map[i].b_curve_distcptn.coef.p1;
		dst_ptr->map[i].b_curve_distcptn.coef.p2 = src_ptr->map[i].b_curve_distcptn.coef.p2;
		dst_ptr->map[i].max_gain_thr = src_ptr->map[i].max_gain_thr;
	}

	cur_index = src_ptr->cur_idx.x0;
	dst_ptr->cur.red_centre_x = dst_ptr->map[cur_index].r_curve_distcptn.center_pos.x;
	dst_ptr->cur.red_centre_y = dst_ptr->map[cur_index].r_curve_distcptn.center_pos.y;
	dst_ptr->cur.blue_centre_x = dst_ptr->map[cur_index].b_curve_distcptn.center_pos.x;
	dst_ptr->cur.blue_centre_y = dst_ptr->map[cur_index].b_curve_distcptn.center_pos.y;
	dst_ptr->cur.red_x2_init = dst_ptr->map[cur_index].r_curve_distcptn.delta_square.x;
	dst_ptr->cur.red_y2_init = dst_ptr->map[cur_index].r_curve_distcptn.delta_square.y;
	dst_ptr->cur.blue_x2_init = dst_ptr->map[cur_index].b_curve_distcptn.delta_square.x;
	dst_ptr->cur.blue_y2_init = dst_ptr->map[cur_index].b_curve_distcptn.delta_square.y;
	dst_ptr->cur.red_threshold = dst_ptr->map[cur_index].red_thr;
	dst_ptr->cur.blue_threshold = dst_ptr->map[cur_index].blue_thr;
	dst_ptr->cur.red_p1_param = dst_ptr->map[cur_index].r_curve_distcptn.coef.p1;
	dst_ptr->cur.red_p2_param = dst_ptr->map[cur_index].r_curve_distcptn.coef.p2;
	dst_ptr->cur.blue_p1_param = dst_ptr->map[cur_index].b_curve_distcptn.coef.p1;
	dst_ptr->cur.blue_p2_param = dst_ptr->map[cur_index].b_curve_distcptn.coef.p2;
	dst_ptr->cur.max_gain_thr = dst_ptr->map[cur_index].max_gain_thr;
	//dst_ptr->cur.img_size.height = img_size_ptr->w;
	//dst_ptr->cur.img_size.width = img_size_ptr->h;
	dst_ptr->cur.start_pos.x = 0x00;
	dst_ptr->cur.start_pos.y = 0x00;
	csc_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_csc_set_param(void *csc_param, isp_u32 cmd, void* param_ptr0, void* param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_csc_param *dst_csc_ptr = PNULL;
	struct isp_pm_block_header *csc_header_ptr = PNULL;
	UNUSED(param_ptr0);
	UNUSED(cmd);

	dst_csc_ptr = (struct isp_csc_param*)csc_param;
	csc_header_ptr = (struct isp_pm_block_header*)param_ptr1;
	csc_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_csc_get_param(void *csc_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_csc_param *csc_ptr = (struct isp_csc_param*)csc_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_RADIAL_CSC;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void*)&csc_ptr->cur;
		param_data_ptr->data_size = sizeof(csc_ptr->cur);
		*update_flag = 0;
	break;

	default:
	break;
	}

	return rtn;
}

static isp_u32 _pm_rgb_precdn_convert_param(void *dst_param, isp_u32 strength_level, isp_u32 mode_flag)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_s32 i = 0;
	struct isp_rgb_pre_cdn_param *dst_ptr = (struct isp_rgb_pre_cdn_param*)dst_param;
	struct sensor_rgb_precdn_level* precdn_param;
	struct nr_param_ptr *nr_ptr;

	if (SENSOR_MULTI_MODE_FLAG != dst_ptr->multi_mode_enable) {
		precdn_param = (struct sensor_rgb_precdn_level*)(dst_ptr->rgb_precdn_ptr);
	} else {
		nr_ptr = (struct nr_param_ptr*)(dst_ptr->rgb_precdn_ptr);
		if (!nr_ptr) {
			return rtn;
		}

		switch(mode_flag) {
			case ISP_NR_COMMON_MODE:
				precdn_param = (struct sensor_rgb_precdn_level*)(nr_ptr->common_param_ptr);
			break;
			case ISP_NR_CAP_MODE:
				precdn_param = (struct sensor_rgb_precdn_level*)(nr_ptr->capture_param_ptr);
			break;
			case ISP_NR_VIDEO_MODE:
				precdn_param = (struct sensor_rgb_precdn_level*)(nr_ptr->video_param_ptr);
			break;
			case ISP_NR_PANO_MODE:
				precdn_param = (struct sensor_rgb_precdn_level*)(nr_ptr->pano_param_ptr);
			break;
			default :
				precdn_param = (struct sensor_rgb_precdn_level*)(nr_ptr->common_param_ptr);
			break;
		}
	}

	strength_level = PM_CLIP(strength_level, 0, SENSOR_SMART_LEVEL_NUM - 1);
	if (precdn_param != NULL) {
		dst_ptr->cur.median_mode = precdn_param[strength_level].median_mode;
		dst_ptr->cur.median_thr = precdn_param[strength_level].median_thr;
		dst_ptr->cur.thru0 = precdn_param[strength_level].thru0;
		dst_ptr->cur.thru1 = precdn_param[strength_level].thru1;
		dst_ptr->cur.thrv0 = precdn_param[strength_level].thrv0;
		dst_ptr->cur.thrv1 = precdn_param[strength_level].thrv1;
		dst_ptr->cur.bypass = precdn_param[strength_level].bypass;
	}

	return rtn;

}

static isp_s32 _pm_rgb_pre_cdn_init_v1(void *dst_pre_cdn_param, void *src_pre_cdn_param, void* param1, void* param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 index = 0x00;
	struct sensor_rgb_precdn_param *src_ptr = (struct sensor_rgb_precdn_param*)src_pre_cdn_param;
	struct isp_rgb_pre_cdn_param *dst_ptr = (struct isp_rgb_pre_cdn_param*)dst_pre_cdn_param;
	struct isp_pm_block_header *pre_cdn_header_ptr = (struct isp_pm_block_header*)param1;
	UNUSED(param2);

	memset((void*)&dst_ptr->cur,0x00,sizeof(dst_ptr->cur));
	dst_ptr->cur.bypass = pre_cdn_header_ptr->bypass;
	dst_ptr->cur.level = SENSOR_SMART_LEVEL_DEFAULT;
	dst_ptr->cur_level = SENSOR_SMART_LEVEL_DEFAULT;

	dst_ptr->rgb_precdn_ptr = src_ptr->param_ptr;
	dst_ptr->multi_mode_enable = src_ptr->reserved[0];

	dst_ptr->cur.median_mode = src_ptr->rgb_precdn_level.median_mode;
	dst_ptr->cur.median_thr = src_ptr->rgb_precdn_level.median_thr;
	dst_ptr->cur.thru0 = src_ptr->rgb_precdn_level.thru0;
	dst_ptr->cur.thru1 = src_ptr->rgb_precdn_level.thru1;
	dst_ptr->cur.thrv0 = src_ptr->rgb_precdn_level.thrv0;
	dst_ptr->cur.thrv1 = src_ptr->rgb_precdn_level.thrv1;

	rtn=_pm_rgb_precdn_convert_param(dst_ptr, SENSOR_SMART_LEVEL_DEFAULT, ISP_NR_COMMON_MODE);
	dst_ptr->cur.bypass |= pre_cdn_header_ptr->bypass;
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("ISP_PM_RGB_PRECDN_CONVERT_PARAM: error!");
		return rtn;
	}

	pre_cdn_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_rgb_pre_cdn_set_param_v1(void *pre_cdn_param, isp_u32 cmd, void* param_ptr0, void* param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_rgb_pre_cdn_param *dst_pre_cdn_ptr = (struct isp_rgb_pre_cdn_param*)pre_cdn_param;
	struct isp_pm_block_header *pre_cdn_header_ptr = (struct isp_pm_block_header*)param_ptr1;
	pre_cdn_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_RGB_PRE_CDN_BYPASS:
		dst_pre_cdn_ptr->cur.bypass =  *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_RGB_PRE_CDN_STRENGTH_LEVEL:
		dst_pre_cdn_ptr->cur_level = *((isp_u32*)param_ptr0);
		dst_pre_cdn_ptr->cur.level = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_SMART_SETTING:
	{
		struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;
		struct isp_range val_range = {0};
		isp_u32 cur_level = 0;

		pre_cdn_header_ptr->is_update = 0;
		val_range.min = 0;
		val_range.max = 255;

		rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_VALUE);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !");
			return rtn;
		}

		cur_level = (isp_u32)block_result->component[0].fix_data[0];

		if (cur_level != dst_pre_cdn_ptr->cur_level || nr_tool_flag[7] || block_result->mode_flag_changed) {
			dst_pre_cdn_ptr->cur_level = cur_level;
			dst_pre_cdn_ptr->cur.level = cur_level;
			pre_cdn_header_ptr->is_update = 1;
			nr_tool_flag[7] = 0;
			block_result->mode_flag_changed = 0;
			rtn=_pm_rgb_precdn_convert_param(dst_pre_cdn_ptr, dst_pre_cdn_ptr->cur_level, block_result->mode_flag);
			dst_pre_cdn_ptr->cur.bypass |= pre_cdn_header_ptr->bypass;
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("ISP_PM_RGB_PRECDN_CONVERT_PARAM: error!");
				return rtn;
			}
		}
	}
	break;

	default:
			pre_cdn_header_ptr->is_update = ISP_ZERO;
	break;
	}
	ISP_LOGI("ISP_SMART: cmd=%d, update=%d, cur_level=%d", cmd, pre_cdn_header_ptr->is_update,
					dst_pre_cdn_ptr->cur_level);

	return rtn;
}

static isp_s32 _pm_rgb_pre_cdn_get_param_v1(void *pre_cdn_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_rgb_pre_cdn_param *pre_cdn_ptr = (struct isp_rgb_pre_cdn_param*)pre_cdn_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_RGB_PRECDN;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void*)&pre_cdn_ptr->cur;
		param_data_ptr->data_size = sizeof(pre_cdn_ptr->cur);
		*update_flag = 0;
	break;

	default:
	break;
	}

	return rtn;
}

static isp_s32 _pm_posterize_init(void *dst_pstrz_param, void *src_pstrz_param, void* param1, void* param_ptr2)
{
	isp_u32 i = 0;
	isp_u32 j = 0;
	isp_s32 rtn = ISP_SUCCESS;
	struct sensor_posterize_param *src_pstrz_ptr = (struct sensor_posterize_param *)src_pstrz_param;
	struct isp_posterize_param *dst_pstrz_ptr = (struct isp_posterize_param*)dst_pstrz_param;
	struct isp_pm_block_header *pstrz_header_ptr = (struct isp_pm_block_header*)param1;
	UNUSED(param_ptr2);

	dst_pstrz_ptr->cur.bypass = pstrz_header_ptr->bypass;

	for (i = 0; i < 8; i ++) {
		dst_pstrz_ptr->cur.posterize_level_bottom[i] = src_pstrz_ptr->pstrz_bot[i];
		dst_pstrz_ptr->cur.posterize_level_top[i] = src_pstrz_ptr->pstrz_top[i];
		dst_pstrz_ptr->cur.posterize_level_out[i] = src_pstrz_ptr->pstrz_out[i];
	}

	for (j = 0; j < MAX_SPECIALEFFECT_NUM; ++j) {
		dst_pstrz_ptr->specialeffect_tab[j].bypass = 1;
		for (i = 0; i<8; i++) {
			dst_pstrz_ptr->specialeffect_tab[j].posterize_level_bottom[i] = src_pstrz_ptr->specialeffect_bot[j][i];
			dst_pstrz_ptr->specialeffect_tab[j].posterize_level_top[i] = src_pstrz_ptr->specialeffect_top[j][i];
			dst_pstrz_ptr->specialeffect_tab[j].posterize_level_out[i] = src_pstrz_ptr->specialeffect_out[j][i];
		}
	}

	pstrz_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_posterize_set_param(void *pstrz_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_posterize_param *pstrz_ptr = (struct isp_posterize_param*)pstrz_param;
	struct isp_pm_block_header *pstrz_header_ptr = PNULL;

	pstrz_header_ptr = (struct isp_pm_block_header*)param_ptr1;
	pstrz_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_PSTRZ_BYPASS:
		pstrz_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_SPECIAL_EFFECT:
	{
		isp_u32 idx = *((isp_u32*)param_ptr0);
		if (idx == 0) {
			pstrz_ptr->cur.bypass = 1;
		} else {
			pstrz_ptr->cur.bypass = 0;
			pstrz_ptr->cur = pstrz_ptr->specialeffect_tab[idx];
		}
	}
	break;

	default:
		pstrz_header_ptr->is_update = ISP_ZERO;
	break;
	}

	return rtn;
}

static isp_s32 _pm_posterize_get_param(void *pstrz_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_posterize_param *pstrz_ptr = (struct isp_posterize_param*)pstrz_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_POSTERIZE;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &pstrz_ptr->cur;
		param_data_ptr->data_size = sizeof(pstrz_ptr->cur);
		*update_flag = 0;
	break;

	default:
	break;
	}

	return rtn;
}

static isp_s32 _pm_rgb_afm_init_v1(void *dst_afm_param, void *src_afm_param, void* param1, void* param_ptr2)
{
	isp_u32 i = 0;
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_rgb_afm_param_v1 *dst_afm_ptr = (struct isp_rgb_afm_param_v1 *)dst_afm_param;
	struct sensor_rgb_afm_param *src_afm_ptr = (struct sensor_rgb_afm_param *)src_afm_param;
	struct isp_pm_block_header *afm_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param_ptr2);

	memset((void*)&(dst_afm_ptr->cur), 0x00, sizeof(dst_afm_ptr->cur));

	dst_afm_ptr->cur.bypass = afm_header_ptr->bypass;
	dst_afm_ptr->cur.af_sel_filter1 = src_afm_ptr->filter_sel.af_sel_filter1;
	dst_afm_ptr->cur.af_sel_filter2 = src_afm_ptr->filter_sel.af_sel_filter2;
	dst_afm_ptr->cur.mode = 0;
//	dst_afm_ptr->cur.frame_size.width =
//	dst_afm_ptr->cur.frame_size.height =
	dst_afm_ptr->cur.skip_num = src_afm_ptr->skip_num;
	dst_afm_ptr->cur.skip_num_clear = 0;
	dst_afm_ptr->cur.sobel_threshold_min = src_afm_ptr->sobel_ctl.sobel.min;
	dst_afm_ptr->cur.sobel_threshold_max = src_afm_ptr->sobel_ctl.sobel.max;
	dst_afm_ptr->cur.sobel_type = src_afm_ptr->sobel_ctl.af_sobel_type;
	dst_afm_ptr->cur.spsmd_cal_mode = src_afm_ptr->spsmd_ctl.af_spsmd_cal_mode;
	dst_afm_ptr->cur.spsmd_diagonal_enable = src_afm_ptr->spsmd_ctl.af_spsmd_diagonal;
	dst_afm_ptr->cur.spsmd_rtgbot_enable = src_afm_ptr->spsmd_ctl.af_spsmd_rtgbot;
	dst_afm_ptr->cur.spsmd_type = src_afm_ptr->spsmd_ctl.af_spsmd_type;
	dst_afm_ptr->cur.spsmd_threshold_min = src_afm_ptr->spsmd_ctl.spsmd.min;
	dst_afm_ptr->cur.spsmd_threshold_max = src_afm_ptr->spsmd_ctl.spsmd.max;

	dst_afm_ptr->cur.spsmd_square_en = 0;
	dst_afm_ptr->cur.overflow_protect_en = 0;
	dst_afm_ptr->cur.subfilter.average = 0;
	dst_afm_ptr->cur.subfilter.median = 0;
	dst_afm_ptr->cur.spsmd_touch_mode = 0;
	dst_afm_ptr->cur.shift.shift_spsmd = 0;
	dst_afm_ptr->cur.shift.shift_sobel5 = 0;
	dst_afm_ptr->cur.shift.shift_sobel9 = 0;
	memset((void*)&(dst_afm_ptr->cur.thrd), 0x00, sizeof(dst_afm_ptr->cur.thrd));

	for (i = 0; i < 25; i++) {
		dst_afm_ptr->cur.coord[i].start_x = src_afm_ptr->windows[i].st_x;
		dst_afm_ptr->cur.coord[i].start_y = src_afm_ptr->windows[i].st_y;
		dst_afm_ptr->cur.coord[i].end_x = src_afm_ptr->windows[i].width;
		dst_afm_ptr->cur.coord[i].end_y = src_afm_ptr->windows[i].height;
	}

	afm_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_rgb_afm_set_param_v1(void *afm_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_rgb_afm_param_v1 *afm_ptr = (struct isp_rgb_afm_param_v1 *)afm_param;
	struct isp_pm_block_header *afm_header_ptr = PNULL;

	afm_header_ptr = (struct isp_pm_block_header*)param_ptr1;
	afm_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_RGB_AFM_BYPASS:
		afm_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	default:
		afm_header_ptr->is_update = ISP_ZERO;
	break;
	}

	return rtn;
}

static isp_s32 _pm_rgb_afm_get_param_v1(void *afm_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_rgb_afm_param_v1 *afm_ptr = (struct isp_rgb_afm_param_v1*)afm_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_AF_V1;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &afm_ptr->cur;
		param_data_ptr->data_size = sizeof(afm_ptr->cur);
		*update_flag = 0;
	break;

	default:
	break;
	}

	return rtn;
}

static isp_s32 _pm_yiq_aem_init_v1(void *dst_aem_param, void *src_aem_param, void* param1, void* param_ptr2)
{
	isp_u32 i = 0;
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_yiq_aem_param_v1 *dst_aem_ptr = (struct isp_yiq_aem_param_v1 *)dst_aem_param;
	struct sensor_y_aem_param *src_aem_ptr = (struct sensor_y_aem_param*)src_aem_param;
	struct isp_pm_block_header *aem_header_ptr = (struct isp_pm_block_header*)param1;
	UNUSED(param_ptr2);

	dst_aem_ptr->cur.aem_bypass = aem_header_ptr->bypass;
	dst_aem_ptr->cur.aem_mode = 0x0;
	dst_aem_ptr->cur.aem_skip_num = src_aem_ptr->skip_num;
	dst_aem_ptr->cur.offset_x = src_aem_ptr->win_start.x;
	dst_aem_ptr->cur.offset_y = src_aem_ptr->win_start.y;
	dst_aem_ptr->cur.width = src_aem_ptr->win_size.w / 16;
	dst_aem_ptr->cur.height = src_aem_ptr->win_size.h / 16;
	dst_aem_ptr->cur.ygamma_bypass = src_aem_ptr->y_gamma_bypass;
#if 0   //need raw param offer param
	for (i = 0; i < 10; i++) {
		if (i < 8) {
			dst_aem_ptr->cur.gamma_xnode[i] = src_aem_ptr->gamma_tab[0].points[25 * i].x;
		} else {
			dst_aem_ptr->cur.gamma_xnode[i] = 0;
		}

		dst_aem_ptr->cur.gamma_ynode[i] = src_aem_ptr->gamma_tab[0].points[25 * i].y;
	}

	for (i = 0; i < 10; i++) {
		if (i == 0) {
			dst_aem_ptr->cur.gamma_node_idx[i] = 0;
		} else {
			dst_aem_ptr->cur.gamma_node_idx[i] = _ispLog2n(dst_aem_ptr->cur.gamma_ynode[i] - dst_aem_ptr->cur.gamma_ynode[i - 1]);
		}
	}
#endif
	aem_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_yiq_aem_set_param_v1(void *aem_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_yiq_aem_param_v1 *aem_ptr = (struct isp_yiq_aem_param_v1 *)aem_param;
	struct isp_pm_block_header *aem_header_ptr = (struct isp_pm_block_header*)param_ptr1;

	aem_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_YIQ_AEM_BYPASS:
		aem_ptr->cur.aem_bypass = *((isp_u32*)param_ptr0);
	break;

	default:
		aem_header_ptr->is_update = ISP_ZERO;
	break;
	}

	return rtn;
}

static isp_s32 _pm_yiq_aem_get_param_v1(void *aem_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_yiq_aem_param_v1 *aem_ptr = (struct isp_yiq_aem_param_v1 *)aem_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_YIQ_AEM;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &aem_ptr->cur;
		param_data_ptr->data_size = sizeof(aem_ptr->cur);
		*update_flag = 0;
	break;
	case ISP_PM_BLK_YIQ_AE_STATISTIC:
		param_data_ptr->data_ptr = (void*)&aem_ptr->stat;
		param_data_ptr->data_size = sizeof(aem_ptr->stat);
	break;

	default:
	break;
	}

	return rtn;
}

static isp_s32 _pm_yiq_aem_init_v2(void *dst_aem_param, void *src_aem_param, void* param1, void* param_ptr2)
{
	isp_u32 i = 0;
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_yiq_aem_param_v2 *dst_aem_ptr = (struct isp_yiq_aem_param_v2 *)dst_aem_param;
	struct sensor_y_aem_param *src_aem_ptr = (struct sensor_y_aem_param*)src_aem_param;
	struct isp_pm_block_header *aem_header_ptr = (struct isp_pm_block_header*)param1;
	UNUSED(param_ptr2);

	dst_aem_ptr->cur.aem_bypass = aem_header_ptr->bypass;
	dst_aem_ptr->cur.aem_mode = 0x0;
	dst_aem_ptr->cur.aem_skip_num = src_aem_ptr->skip_num;
	dst_aem_ptr->cur.offset_x = src_aem_ptr->win_start.x;
	dst_aem_ptr->cur.offset_y = src_aem_ptr->win_start.y;
	dst_aem_ptr->cur.width = src_aem_ptr->win_size.w / 16;
	dst_aem_ptr->cur.height = src_aem_ptr->win_size.h / 16;
	dst_aem_ptr->cur.ygamma_bypass = src_aem_ptr->y_gamma_bypass;
	dst_aem_ptr->cur.avgshift = 0;//pike add this register,but tshark2 without
#if 0   //need raw param offer param
	for (i = 0; i < 10; i++) {
		if (i < 8) {
			dst_aem_ptr->cur.gamma_xnode[i] = src_aem_ptr->gamma_tab[0].points[25 * i].x;
		} else {
			dst_aem_ptr->cur.gamma_xnode[i] = 0;
		}

		dst_aem_ptr->cur.gamma_ynode[i] = src_aem_ptr->gamma_tab[0].points[25 * i].y;
	}

	for (i = 0; i < 10; i++) {
		if (i == 0) {
			dst_aem_ptr->cur.gamma_node_idx[i] = 0;
		} else {
			dst_aem_ptr->cur.gamma_node_idx[i] = _ispLog2n(dst_aem_ptr->cur.gamma_ynode[i] - dst_aem_ptr->cur.gamma_ynode[i - 1]);
		}
	}
#endif
	aem_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_yiq_aem_set_param_v2(void *aem_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_yiq_aem_param_v2 *aem_ptr = (struct isp_yiq_aem_param_v2 *)aem_param;
	struct isp_pm_block_header *aem_header_ptr = (struct isp_pm_block_header*)param_ptr1;

	aem_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_YIQ_AEM_BYPASS:
		aem_ptr->cur.aem_bypass = *((isp_u32*)param_ptr0);
	break;

	default:
		aem_header_ptr->is_update = ISP_ZERO;
	break;
	}

	return rtn;
}

static isp_s32 _pm_yiq_aem_get_param_v2(void *aem_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_yiq_aem_param_v2 *aem_ptr = (struct isp_yiq_aem_param_v2 *)aem_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_YIQ_AEM;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &aem_ptr->cur;
		param_data_ptr->data_size = sizeof(aem_ptr->cur);
		*update_flag = 0;
	break;

	default:
	break;
	}

	return rtn;
}

static isp_s32 _pm_yiq_afl_init_v1(void *dst_afl_param, void *src_afl_param, void* param1, void* param_ptr2)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_yiq_afl_param_v1 *dst_afl_ptr = (struct isp_yiq_afl_param_v1 *)dst_afl_param;
	struct sensor_y_afl_param *src_afl_ptr = (struct sensor_y_afl_param*)src_afl_param;
	struct isp_pm_block_header *afl_header_ptr = (struct isp_pm_block_header*)param1;
	UNUSED(param_ptr2);

	dst_afl_ptr->cur.bypass  = afl_header_ptr->bypass;
	dst_afl_ptr->cur.mode = 0;
	dst_afl_ptr->cur.skip_frame_num = src_afl_ptr->skip_num;
	dst_afl_ptr->cur.line_step = src_afl_ptr->line_step;
	dst_afl_ptr->cur.frame_num = src_afl_ptr->frm_num;
	dst_afl_ptr->cur.vheight = src_afl_ptr->v_height;
	dst_afl_ptr->cur.start_col = src_afl_ptr->col_st_ed.x;
	dst_afl_ptr->cur.end_col = src_afl_ptr->col_st_ed.y;

	ISP_LOGE("$$LHC:bypass %d", afl_header_ptr->bypass);

//	dst_afl_ptr->cur.addr = src_afl_ptr->
	afl_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_yiq_afl_set_param_v1(void *afl_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_yiq_afl_param_v1 *afl_ptr = (struct isp_yiq_afl_param_v1 *)afl_param;
	struct isp_pm_block_header *afl_header_ptr = (struct isp_pm_block_header *)param_ptr1;
	struct isp_anti_flicker_cfg *cfl_cfg_ptr = (struct isp_anti_flicker_cfg *)param_ptr0;

	afl_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_YIQ_AFL_BYPASS:
		afl_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_YIQ_AFL_CFG:
		ISP_LOGE("$$LHC:skip-num %d", cfl_cfg_ptr->skip_frame_num);
		afl_ptr->cur.bypass = cfl_cfg_ptr->bypass;
		afl_ptr->cur.skip_frame_num = cfl_cfg_ptr->skip_frame_num;
		afl_ptr->cur.frame_num = cfl_cfg_ptr->frame_num;
		afl_ptr->cur.line_step = cfl_cfg_ptr->line_step;
		afl_ptr->cur.mode = cfl_cfg_ptr->mode;
		afl_ptr->cur.vheight = cfl_cfg_ptr->vheight;
		afl_ptr->cur.start_col = cfl_cfg_ptr->start_col;
		afl_ptr->cur.end_col = cfl_cfg_ptr->end_col;
		ISP_LOGE("$$LHC:start_col %d end_col %d", afl_ptr->cur.start_col, afl_ptr->cur.end_col);
	break;

	default:
		afl_header_ptr->is_update = ISP_ZERO;
	break;
	}

	return rtn;
}

static isp_s32 _pm_yiq_afl_get_param_v1(void *afl_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_yiq_afl_param_v1 *afl_ptr = (struct isp_yiq_afl_param_v1 *)afl_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_YIQ_AFL;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &afl_ptr->cur;
		param_data_ptr->data_size = sizeof(afl_ptr->cur);
		*update_flag = 0;
	break;

	default:
	break;
	}

	return rtn;
}


static isp_u32 _pm_yiq_afm_convert_param(void *dst_yiq_afm_param, isp_u32 strength_level, isp_u32 mode_flag)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_s32 i = 0;
	struct isp_yiq_afm_param_v1 *dst_yiq_afm_ptr = (struct isp_yiq_afm_param_v1*)dst_yiq_afm_param;
	struct sensor_y_afm_level* yiq_afm_param;
	struct nr_param_ptr *nr_ptr;

	if (SENSOR_MULTI_MODE_FLAG != dst_yiq_afm_ptr->multi_mode_enable) {
		yiq_afm_param = (struct sensor_y_afm_level*)(dst_yiq_afm_ptr->yiq_afm_ptr);
	} else {
		nr_ptr = (struct nr_param_ptr*)(dst_yiq_afm_ptr->yiq_afm_ptr);
		if (!nr_ptr) {
			return rtn;
		}

		switch(mode_flag) {
			case ISP_NR_COMMON_MODE:
				yiq_afm_param = (struct sensor_y_afm_level*)(nr_ptr->common_param_ptr);
			break;
			case ISP_NR_CAP_MODE:
				yiq_afm_param = (struct sensor_y_afm_level*)(nr_ptr->capture_param_ptr);
			break;
			case ISP_NR_VIDEO_MODE:
				yiq_afm_param = (struct sensor_y_afm_level*)(nr_ptr->video_param_ptr);
			break;
			case ISP_NR_PANO_MODE:
				yiq_afm_param = (struct sensor_y_afm_level*)(nr_ptr->pano_param_ptr);
			break;
			default :
				yiq_afm_param = (struct sensor_y_afm_level*)(nr_ptr->common_param_ptr);
			break;
		}
	}

	strength_level = PM_CLIP(strength_level, 0, SENSOR_SMART_LEVEL_NUM - 1);
	if (yiq_afm_param != NULL) {
		dst_yiq_afm_ptr->cur.skip_num = yiq_afm_param[strength_level].skip_num;
		dst_yiq_afm_ptr->cur.format = yiq_afm_param[strength_level].afm_format;
		dst_yiq_afm_ptr->cur.iir_bypass = yiq_afm_param[strength_level].iir_bypass;
		for (i = 0; i < 25; i++) {
			dst_yiq_afm_ptr->cur.coord[i].start_x = yiq_afm_param[strength_level].win[i][0];
			dst_yiq_afm_ptr->cur.coord[i].end_x = yiq_afm_param[strength_level].win[i][1];
			dst_yiq_afm_ptr->cur.coord[i].start_y = yiq_afm_param[strength_level].win[i][2];
			dst_yiq_afm_ptr->cur.coord[i].end_y = yiq_afm_param[strength_level].win[i][3];
		}
		for (i = 0; i < 11; i++) {
			dst_yiq_afm_ptr->cur.IIR_c[i] = yiq_afm_param[strength_level].coef[i];
		}
	}

	return rtn;
}

static isp_s32 _pm_yiq_afm_init(void *dst_yiq_afm_param, void *src_yiq_afm_param, void *param1, void *param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i;
	struct isp_pm_block_header *yiq_afm_header_ptr = (struct isp_pm_block_header *)param1;
	struct sensor_y_afm_param *src_yiq_afm_ptr = (struct sensor_y_afm_param *)src_yiq_afm_param;
	struct isp_yiq_afm_param_v1 *dst_yiq_afm_ptr = (struct isp_yiq_afm_param_v1 *)dst_yiq_afm_param;
	UNUSED(param2);

	dst_yiq_afm_ptr->cur.bypass = yiq_afm_header_ptr->bypass;
	dst_yiq_afm_ptr->cur_level = SENSOR_SMART_LEVEL_DEFAULT;
	dst_yiq_afm_ptr->yiq_afm_ptr = src_yiq_afm_ptr->param_ptr;
	dst_yiq_afm_ptr->multi_mode_enable = src_yiq_afm_ptr->reserved[0];

	dst_yiq_afm_ptr->cur.skip_num = src_yiq_afm_ptr->y_afm_level.skip_num;
	dst_yiq_afm_ptr->cur.format = src_yiq_afm_ptr->y_afm_level.afm_format;
	dst_yiq_afm_ptr->cur.iir_bypass = src_yiq_afm_ptr->y_afm_level.iir_bypass;
	for (i = 0; i < 25; i++) {
		dst_yiq_afm_ptr->cur.coord[i].start_x = src_yiq_afm_ptr->y_afm_level.win[i][0];
		dst_yiq_afm_ptr->cur.coord[i].end_x = src_yiq_afm_ptr->y_afm_level.win[i][1];
		dst_yiq_afm_ptr->cur.coord[i].start_y = src_yiq_afm_ptr->y_afm_level.win[i][2];
		dst_yiq_afm_ptr->cur.coord[i].end_y = src_yiq_afm_ptr->y_afm_level.win[i][3];
	}
	for (i = 0; i < 11; i++) {
		dst_yiq_afm_ptr->cur.IIR_c[i] = src_yiq_afm_ptr->y_afm_level.coef[i];
	}

	rtn =  _pm_yiq_afm_convert_param(dst_yiq_afm_ptr,SENSOR_SMART_LEVEL_DEFAULT,ISP_NR_COMMON_MODE);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("ISP_PM_YIQ_AFM_CONVERT_PARAM: error!");
		return rtn;
	}
	yiq_afm_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_yiq_afm_set_param(void *yiq_afm_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *yiq_afm_header_ptr = (struct isp_pm_block_header*)param_ptr1;
	struct isp_yiq_afm_param_v1 *yiq_afm_ptr = (struct isp_yiq_afm_param_v1 *)yiq_afm_param;
	yiq_afm_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_YIQ_AFM_BYPASS:
		yiq_afm_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_SMART_SETTING:
	{
		struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;
		struct isp_range val_range = {0};
		isp_u32 cur_level = 0;

		yiq_afm_header_ptr->is_update = 0;
		val_range.min = 0;
		val_range.max = 255;

		rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_VALUE);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !");
			return rtn;
		}

		cur_level = (isp_u32)block_result->component[0].fix_data[0];

		if (cur_level != yiq_afm_ptr->cur_level || nr_tool_flag[8] || block_result->mode_flag_changed) {
			yiq_afm_ptr->cur_level = cur_level;
			yiq_afm_header_ptr->is_update = 1;
			nr_tool_flag[8] = 0;
			block_result->mode_flag_changed = 0;
			rtn = _pm_yiq_afm_convert_param(yiq_afm_ptr,yiq_afm_ptr->cur_level,block_result->mode_flag);
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("ISP_PM_YIQ_AFM_CONVERT_PARAM: error!");
				return rtn;
			}
		}
	}
	break;

	default:
		yiq_afm_header_ptr->is_update = ISP_ZERO;
	break;
	}
	ISP_LOGI("ISP_SMART: cmd = %d, is_update = %d, cur_level=%d",
			cmd, yiq_afm_header_ptr->is_update, yiq_afm_ptr->cur_level);
	return rtn;
}

static isp_s32 _pm_yiq_afm_get_param(void *yiq_afm_param, isp_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_yiq_afm_param_v1 *yiq_afm_ptr = (struct isp_yiq_afm_param_v1 *)yiq_afm_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_YIQ_AFM;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &yiq_afm_ptr->cur;
		param_data_ptr->data_size = sizeof(yiq_afm_ptr->cur);
		*update_flag = ISP_ZERO;
	break;

	default:
	break;
	}

	return rtn;
}

static isp_s32 _pm_yiq_afm_init_v2(void *dst_yiq_afm_param, void *src_yiq_afm_param, void *param1, void *param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i;
	struct isp_pm_block_header *yiq_afm_header_ptr = (struct isp_pm_block_header *)param1;
	struct sensor_y_afm_param *src_yiq_afm_ptr = (struct sensor_y_afm_param *)src_yiq_afm_param;
	struct isp_yiq_afm_param_v2 *dst_yiq_afm_ptr = (struct isp_yiq_afm_param_v2 *)dst_yiq_afm_param;
	UNUSED(param2);

	dst_yiq_afm_ptr->cur.bypass = yiq_afm_header_ptr->bypass;
	dst_yiq_afm_ptr->cur.skip_num = src_yiq_afm_ptr->y_afm_level.skip_num;
	dst_yiq_afm_ptr->cur.format = src_yiq_afm_ptr->y_afm_level.afm_format;
	dst_yiq_afm_ptr->cur.iir_bypass = src_yiq_afm_ptr->y_afm_level.iir_bypass;
	dst_yiq_afm_ptr->cur.af_position = 0;//control where AF monitor works, 0: after CAF;1:after Auto Contrast Adjust

	for (i = 0; i < 25; i++) {
		dst_yiq_afm_ptr->cur.coord[i].start_x = src_yiq_afm_ptr->y_afm_level.win[i][0];
		dst_yiq_afm_ptr->cur.coord[i].end_x = src_yiq_afm_ptr->y_afm_level.win[i][1];
		dst_yiq_afm_ptr->cur.coord[i].start_y = src_yiq_afm_ptr->y_afm_level.win[i][2];
		dst_yiq_afm_ptr->cur.coord[i].end_y = src_yiq_afm_ptr->y_afm_level.win[i][3];
	}
	for (i = 0; i < 11; i++) {
		dst_yiq_afm_ptr->cur.IIR_c[i] = src_yiq_afm_ptr->y_afm_level.coef[i];
	}
	yiq_afm_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_yiq_afm_set_param_v2(void *yiq_afm_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *yiq_afm_header_ptr = (struct isp_pm_block_header*)param_ptr1;
	struct isp_yiq_afm_param_v2 *yiq_afm_ptr = (struct isp_yiq_afm_param_v2 *)yiq_afm_param;
	yiq_afm_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_YIQ_AFM_BYPASS:
		yiq_afm_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	default:
		yiq_afm_header_ptr->is_update = ISP_ZERO;
	break;
	}

	return rtn;
}

static isp_s32 _pm_yiq_afm_get_param_v2(void *yiq_afm_param, isp_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_yiq_afm_param_v2 *yiq_afm_ptr = (struct isp_yiq_afm_param_v2 *)yiq_afm_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_YIQ_AFM;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &yiq_afm_ptr->cur;
		param_data_ptr->data_size = sizeof(yiq_afm_ptr->cur);
		*update_flag = ISP_ZERO;
	break;

	default:
	break;
	}

	return rtn;
}

static isp_u32 _pm_yuv_precdn_convert_param(void *dst_precdn_param, isp_u32 strength_level, isp_u32 mode_flag)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_s32 i = 0;
	struct isp_yuv_precdn_param *dst_precdn_ptr = (struct isp_yuv_precdn_param*)dst_precdn_param;
	struct sensor_yuv_precdn_level* yuv_precdn_param;
	struct nr_param_ptr *nr_ptr;

	if (SENSOR_MULTI_MODE_FLAG != dst_precdn_ptr->multi_mode_enable) {
		yuv_precdn_param = (struct sensor_yuv_precdn_level*)(dst_precdn_ptr->yuv_precdn_ptr);
	} else {
		nr_ptr = (struct nr_param_ptr*)(dst_precdn_ptr->yuv_precdn_ptr);
		if (!nr_ptr) {
			return rtn;
		}

		switch(mode_flag) {
			case ISP_NR_COMMON_MODE:
				yuv_precdn_param = (struct sensor_yuv_precdn_level*)(nr_ptr->common_param_ptr);
			break;
			case ISP_NR_CAP_MODE:
				yuv_precdn_param = (struct sensor_yuv_precdn_level*)(nr_ptr->capture_param_ptr);
			break;
			case ISP_NR_VIDEO_MODE:
				yuv_precdn_param = (struct sensor_yuv_precdn_level*)(nr_ptr->video_param_ptr);
			break;
			case ISP_NR_PANO_MODE:
				yuv_precdn_param = (struct sensor_yuv_precdn_level*)(nr_ptr->pano_param_ptr);
			break;
			default :
				yuv_precdn_param = (struct sensor_yuv_precdn_level*)(nr_ptr->common_param_ptr);
			break;
		}
	}

	strength_level = PM_CLIP(strength_level, 0, SENSOR_SMART_LEVEL_NUM - 1);
	if (yuv_precdn_param != NULL) {
		dst_precdn_ptr->cur.mode = yuv_precdn_param[strength_level].precdn_comm.mode;
		dst_precdn_ptr->cur.median_writeback_en = yuv_precdn_param[strength_level].precdn_comm.median_writeback_en;
		dst_precdn_ptr->cur.median_mode = yuv_precdn_param[strength_level].precdn_comm.median_mode;
		dst_precdn_ptr->cur.den_stren = yuv_precdn_param[strength_level].precdn_comm.den_stren;
		dst_precdn_ptr->cur.uv_joint = yuv_precdn_param[strength_level].precdn_comm.uv_joint;
		dst_precdn_ptr->cur.median_thr_u[0] = yuv_precdn_param[strength_level].precdn_comm.median_thr_u[0];
		dst_precdn_ptr->cur.median_thr_u[1] = yuv_precdn_param[strength_level].precdn_comm.median_thr_u[1];
		dst_precdn_ptr->cur.median_thr_v[0] = yuv_precdn_param[strength_level].precdn_comm.median_thr_v[0];
		dst_precdn_ptr->cur.median_thr_v[1] = yuv_precdn_param[strength_level].precdn_comm.median_thr_v[1];
		dst_precdn_ptr->cur.median_thr = yuv_precdn_param[strength_level].precdn_comm.median_thr;
		dst_precdn_ptr->cur.uv_thr = yuv_precdn_param[strength_level].precdn_comm.uv_thr;
		dst_precdn_ptr->cur.y_thr = yuv_precdn_param[strength_level].precdn_comm.y_thr;

		for (i = 0; i < 7; i++) {
			dst_precdn_ptr->cur.r_segu[0][i] = yuv_precdn_param[strength_level].r_segu[0][i];
			dst_precdn_ptr->cur.r_segu[1][i] = yuv_precdn_param[strength_level].r_segu[1][i];
			dst_precdn_ptr->cur.r_segv[0][i] = yuv_precdn_param[strength_level].r_segv[0][i];
			dst_precdn_ptr->cur.r_segv[1][i] = yuv_precdn_param[strength_level].r_segv[1][i];
			dst_precdn_ptr->cur.r_segy[0][i] = yuv_precdn_param[strength_level].r_segy[0][i];
			dst_precdn_ptr->cur.r_segy[1][i] = yuv_precdn_param[strength_level].r_segy[1][i];
		}

		for (i = 0; i < 25; i++) {
			dst_precdn_ptr->cur.r_distw[i] = yuv_precdn_param[strength_level].dist_w[i];
		}
		dst_precdn_ptr->cur.bypass = yuv_precdn_param[strength_level].bypass;
	}

	return rtn;
}


static isp_s32 _pm_yuv_precdn_init(void *dst_precdn_param, void *src_precdn_param, void *param1, void *param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i, j;
	struct isp_pm_block_header *precdn_header_ptr = (struct isp_pm_block_header*)param1;
	struct sensor_yuv_precdn_param *src_precdn_ptr = (struct sensor_yuv_precdn_param*)src_precdn_param;
	struct isp_yuv_precdn_param *dst_precdn_ptr = (struct isp_yuv_precdn_param*)dst_precdn_param;
	UNUSED(param2);

	dst_precdn_ptr->cur.bypass = precdn_header_ptr->bypass;
	dst_precdn_ptr->cur.level = SENSOR_SMART_LEVEL_DEFAULT;
	dst_precdn_ptr->cur_level = SENSOR_SMART_LEVEL_DEFAULT;

	dst_precdn_ptr->yuv_precdn_ptr = src_precdn_ptr->param_ptr;
	dst_precdn_ptr->multi_mode_enable = src_precdn_ptr->reserved3[0];
	rtn = _pm_yuv_precdn_convert_param(dst_precdn_ptr,SENSOR_SMART_LEVEL_DEFAULT,ISP_NR_COMMON_MODE);
	dst_precdn_ptr->cur.bypass |= precdn_header_ptr->bypass;
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("ISP_PM_YUV_PRECDN_CONVERT_PARAM: error!");
		return rtn;
	}

	precdn_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_yuv_precdn_set_param(void *precdn_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *precdn_header_ptr = (struct isp_pm_block_header*)param_ptr1;
	struct isp_yuv_precdn_param *precdn_ptr = (struct isp_yuv_precdn_param*)precdn_param;
	precdn_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_YUV_PRECDN_BYPASS:
		precdn_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_YUV_PRECDN_STRENGTH_LEVEL:
		precdn_ptr->cur_level = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_SMART_SETTING:
	{
		struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;
		struct isp_range val_range = {0};
		isp_u32 cur_level = 0;

		precdn_header_ptr->is_update = 0;
		val_range.min = 0;
		val_range.max = 255;

		rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_VALUE);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !");
			return rtn;
		}

		cur_level = (isp_u32)block_result->component[0].fix_data[0];

		if (cur_level != precdn_ptr->cur_level || nr_tool_flag[9] || block_result->mode_flag_changed) {
			precdn_ptr->cur_level = cur_level;
			precdn_ptr->cur.level = cur_level;
			precdn_header_ptr->is_update = 1;
			nr_tool_flag[9] = 0;
			block_result->mode_flag_changed = 0;
			rtn = _pm_yuv_precdn_convert_param(precdn_ptr,precdn_ptr->cur.level,block_result->mode_flag);
			precdn_ptr->cur.bypass |= precdn_header_ptr->bypass;
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("ISP_PM_YUV_PRECDN_CONVERT_PARAM: error!");
				return rtn;
			}
		}
	}
	break;

	default:
		precdn_header_ptr->is_update = ISP_ZERO;
	break;
	}

	ISP_LOGV("ISP_SMART: cmd=%d, update=%d, cur_level=%d", cmd, precdn_header_ptr->is_update,
					precdn_ptr->cur_level);

	return rtn;
}

static isp_s32 _pm_yuv_precdn_get_param(void *precdn_param, isp_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_yuv_precdn_param *precdn_ptr = (struct isp_yuv_precdn_param*)precdn_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_YUV_PRECDN;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &precdn_ptr->cur;
		param_data_ptr->data_size = sizeof(precdn_ptr->cur);
		*update_flag = ISP_ZERO;
	break;

	default:
	break;
	}

	return rtn;
}

static isp_u32 _pm_prfy_convert_param(void *dst_prfy_param, isp_u32 strength_level, isp_u32 mode_flag)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_s32 i = 0;
	struct isp_prfy_param *dst_prfy_ptr = (struct isp_prfy_param*)dst_prfy_param;
	struct sensor_prfy_level* prfy_param;
	struct nr_param_ptr *nr_ptr;

	if (SENSOR_MULTI_MODE_FLAG != dst_prfy_ptr->multi_mode_enable) {
		prfy_param = (struct sensor_prfy_level*)(dst_prfy_ptr->prfy_ptr);
	} else {
		nr_ptr = (struct nr_param_ptr*)(dst_prfy_ptr->prfy_ptr);
		if (!nr_ptr) {
			return rtn;
		}

		switch(mode_flag) {
			case ISP_NR_COMMON_MODE:
				prfy_param = (struct sensor_prfy_level*)(nr_ptr->common_param_ptr);
			break;
			case ISP_NR_CAP_MODE:
				prfy_param = (struct sensor_prfy_level*)(nr_ptr->capture_param_ptr);
			break;
			case ISP_NR_VIDEO_MODE:
				prfy_param = (struct sensor_prfy_level*)(nr_ptr->video_param_ptr);
			break;
			case ISP_NR_PANO_MODE:
				prfy_param = (struct sensor_prfy_level*)(nr_ptr->pano_param_ptr);
			break;
			default :
				prfy_param = (struct sensor_prfy_level*)(nr_ptr->common_param_ptr);
			break;
		}
	}

	strength_level = PM_CLIP(strength_level, 0, SENSOR_SMART_LEVEL_NUM - 1);
	if (prfy_param != NULL) {
		dst_prfy_ptr->cur.writeback = prfy_param[strength_level].prfy_writeback;
		dst_prfy_ptr->cur.thrd = prfy_param[strength_level].nr_thr_y;
		dst_prfy_ptr->cur.bypass= prfy_param[strength_level].bypass;
	}

	return rtn;
}


static isp_s32 _pm_prfy_init(void *dst_prfy_param, void *src_prfy_param, void *param1, void *param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *prfy_header_ptr = (struct isp_pm_block_header*)param1;
	struct sensor_prfy_param *src_prfy_ptr = (struct sensor_prfy_param*)src_prfy_param;
	struct isp_prfy_param *dst_prfy_ptr = (struct isp_prfy_param*)dst_prfy_param;
	UNUSED(param2);

	dst_prfy_ptr->cur.bypass = prfy_header_ptr->bypass;

	dst_prfy_ptr->cur_level = SENSOR_SMART_LEVEL_DEFAULT;
	dst_prfy_ptr->prfy_ptr = src_prfy_ptr->param_ptr;
	dst_prfy_ptr->multi_mode_enable = src_prfy_ptr->reserved3[0];

	rtn = _pm_prfy_convert_param(dst_prfy_ptr,SENSOR_SMART_LEVEL_DEFAULT,ISP_NR_COMMON_MODE);
	dst_prfy_ptr->cur.bypass |= prfy_header_ptr->bypass;
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("ISP_PM_PRFY_CONVERT_PARAM: error!");
		return rtn;
	}

	prfy_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_prfy_set_param(void *prfy_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *prfy_header_ptr = (struct isp_pm_block_header*)param_ptr1;
	struct isp_prfy_param *prfy_ptr = (struct isp_prfy_param*)prfy_param;
	prfy_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_PRFY_BYPASS:
		prfy_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_SMART_SETTING:
	{
		struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;
		struct isp_range val_range = {0};
		isp_u32 cur_level = 0;

		prfy_header_ptr->is_update = 0;
		val_range.min = 0;
		val_range.max = 255;

		rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_VALUE);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !");
			return rtn;
		}

		cur_level = (isp_u32)block_result->component[0].fix_data[0];

		if (cur_level != prfy_ptr->cur_level || nr_tool_flag[10] || block_result->mode_flag_changed) {
			prfy_ptr->cur_level = cur_level;
			prfy_header_ptr->is_update = 1;
			nr_tool_flag[10] = 0;
			block_result->mode_flag_changed = 0;
			rtn = _pm_prfy_convert_param(prfy_ptr,prfy_ptr->cur_level,block_result->mode_flag);
			prfy_ptr->cur.bypass |= prfy_header_ptr->bypass;
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("ISP_PM_PRFY_CONVERT_PARAM: error!");
				return rtn;
			}
		}
	}
	break;

	default:
		prfy_header_ptr->is_update = ISP_ZERO;
	break;
	}

	ISP_LOGV("ISP_SMART: cmd = %d, is_update = %d, cur_level=%d",
			cmd, prfy_header_ptr->is_update, prfy_ptr->cur.thrd);

	return rtn;
}

static isp_s32 _pm_prfy_get_param(void *prfy_param, isp_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_prfy_param *prfy_ptr = (struct isp_prfy_param*)prfy_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_PREF_V1;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &prfy_ptr->cur;
		param_data_ptr->data_size = sizeof(prfy_ptr->cur);
		*update_flag = ISP_ZERO;
	break;

	default:
	break;
	}

	return rtn;
}

static isp_s32 _pm_hist_init_v1(void *dst_hist_param, void *src_hist_param, void *param1, void *param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *hist_header_ptr = (struct isp_pm_block_header *)param1;
	struct sensor_yuv_hists_param *src_hist_ptr = (struct sensor_yuv_hists_param *)src_hist_param;
	struct isp_hist_param_v1 *dst_hist_ptr = (struct isp_hist_param_v1 *)dst_hist_param;
	UNUSED(param2);

	dst_hist_ptr->cur.bypass = hist_header_ptr->bypass;
	dst_hist_ptr->cur.off = 0x02;
	dst_hist_ptr->cur.buf_rst_en = 0x0;
	dst_hist_ptr->cur.skip_num_clr = 0x0;
	dst_hist_ptr->cur.mode = 0x1;
	dst_hist_ptr->cur.skip_num = src_hist_ptr->skip_num;
	dst_hist_ptr->cur.pof_rst_en = src_hist_ptr->pof_rst_en;
	dst_hist_ptr->cur.low_ratio = src_hist_ptr->low_sum_ratio;
	dst_hist_ptr->cur.high_ratio = src_hist_ptr->high_sum_ratio;
	dst_hist_ptr->cur.dif_adj = src_hist_ptr->diff_th;
	dst_hist_ptr->cur.small_adj = src_hist_ptr->small_adj;
	dst_hist_ptr->cur.big_adj = src_hist_ptr->big_adj;
	hist_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_hist_set_param_v1(void *hist_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *hist_header_ptr = (struct isp_pm_block_header *)param_ptr1;
	struct isp_hist_param_v1 *hist_ptr = (struct isp_hist_param_v1 *)hist_param;
	hist_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_HIST_BYPASS_V1:
		hist_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	default:
		hist_header_ptr->is_update = ISP_ZERO;
	break;
	}

	return rtn;
}

static isp_s32 _pm_hist_get_param_v1(void *hist_param, isp_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_hist_param_v1 *hist_ptr = (struct isp_hist_param_v1 *)hist_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_HIST_V1;
	param_data_ptr->cmd = ISP_PM_BLK_ISP_SETTING;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &hist_ptr->cur;
		param_data_ptr->data_size = sizeof(hist_ptr->cur);
		*update_flag = ISP_ZERO;
	break;

	default:
	break;
	}

	return rtn;
}

static isp_s32 _pm_hist2_init_v1(void *dst_hist2_param, void *src_hist2_param, void *param1, void *param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i = 0;
	struct isp_pm_block_header *hist2_header_ptr = (struct isp_pm_block_header *)param1;
	struct sensor_yuv_hists2_param *src_hist2_ptr = (struct sensor_yuv_hists2_param *)src_hist2_param;
	struct isp_hist2_param_v1 *dst_hist2_ptr = (struct isp_hist2_param_v1 *)dst_hist2_param;
	UNUSED(param2);

	dst_hist2_ptr->cur.bypass = hist2_header_ptr->bypass;
	dst_hist2_ptr->cur.skip_num = src_hist2_ptr->skip_num;
	dst_hist2_ptr->cur.en = 0x0;
	dst_hist2_ptr->cur.skip_num_clr = 0x0;
	dst_hist2_ptr->cur.mode = 0x1;
	for (i = 0; i < SENSOR_HIST2_ROI_NUM; i++) {
		dst_hist2_ptr->cur.hist_roi_x_s[i] = src_hist2_ptr->roi_point[i].start_x;
		dst_hist2_ptr->cur.hist_roi_x_e[i] = src_hist2_ptr->roi_point[i].end_x;
		dst_hist2_ptr->cur.hist_roi_y_s[i] = src_hist2_ptr->roi_point[i].start_y;
		dst_hist2_ptr->cur.hist_roi_y_e[i] = src_hist2_ptr->roi_point[i].end_y;
	}
	hist2_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_hist2_set_param_v1(void *hist2_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *hist2_header_ptr = (struct isp_pm_block_header*)param_ptr1;
	struct isp_hist2_param_v1 *hist2_ptr = (struct isp_hist2_param_v1 *)hist2_param;

	hist2_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_HIST2_BYPASS:
		hist2_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	default:
		hist2_header_ptr->is_update = ISP_ZERO;
	break;
	}

	return rtn;
}

static isp_s32 _pm_hist2_get_param_v1(void *hist2_param, isp_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_hist2_param_v1 *hist2_ptr = (struct isp_hist2_param_v1 *)hist2_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_HIST2;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &hist2_ptr->cur;
		param_data_ptr->data_size = sizeof(hist2_ptr->cur);
		*update_flag = ISP_ZERO;
	break;

	default:
	break;
	}

	return rtn;
}

static isp_s32 _pm_auto_contrast_init_v1(void *dst_aca_param, void *src_aca_param, void *param1, void *param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *aca_header_ptr = (struct isp_pm_block_header*)param1;
	struct sensor_auto_contrast_param_v1 *src_aca_ptr = (struct sensor_auto_contrast_param_v1*)src_aca_param;
	struct isp_auto_contrast_param_v1 *dst_aca_ptr = (struct isp_auto_contrast_param_v1*)dst_aca_param;
	UNUSED(param2);

	dst_aca_ptr->cur.bypass = aca_header_ptr->bypass;
	dst_aca_ptr->cur.mode = src_aca_ptr->aca_mode;
	dst_aca_ptr->cur.in_min = src_aca_ptr->in.min;
	dst_aca_ptr->cur.in_max = src_aca_ptr->in.max;
	dst_aca_ptr->cur.out_min = src_aca_ptr->out.min;
	dst_aca_ptr->cur.out_max = src_aca_ptr->out.max;
	aca_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_auto_contrast_set_param_v1(void *aca_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *aca_header_ptr =  (struct isp_pm_block_header*)param_ptr1;
	struct isp_auto_contrast_param_v1 *aca_ptr = (struct isp_auto_contrast_param_v1*)aca_param;
	aca_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_ACA_BYPASS:
		aca_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	default:
		aca_header_ptr->is_update = ISP_ZERO;
	break;
	}

	return rtn;
}

static isp_s32 _pm_auto_contrast_get_param_v1(void *aca_param, isp_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_auto_contrast_param_v1 *aca_ptr = (struct isp_auto_contrast_param_v1*)aca_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_AUTO_CONTRAST_V1;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &aca_ptr->cur;
		param_data_ptr->data_size = sizeof(aca_ptr->cur);
		*update_flag = ISP_ZERO;
	break;

	default:
	break;
	}

	return rtn;
}

static isp_u32 _pm_uv_cdn_convert_param(void *dst_cdn_param, isp_u32 strength_level, isp_u32 mode_flag)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_s32 i = 0;
	struct isp_uv_cdn_param_v1 *dst_cdn_ptr = (struct isp_uv_cdn_param_v1*)dst_cdn_param;
	struct sensor_uv_cdn_level* uv_cdn_param;
	struct nr_param_ptr *nr_ptr;

	if (SENSOR_MULTI_MODE_FLAG != dst_cdn_ptr->multi_mode_enable) {
		uv_cdn_param = (struct sensor_uv_cdn_level*)(dst_cdn_ptr->uv_cdn_ptr);
	} else {
		nr_ptr = (struct nr_param_ptr*)(dst_cdn_ptr->uv_cdn_ptr);
		if (!nr_ptr) {
			return rtn;
		}

		switch(mode_flag) {
			case ISP_NR_COMMON_MODE:
				uv_cdn_param = (struct sensor_uv_cdn_level*)(nr_ptr->common_param_ptr);
			break;
			case ISP_NR_CAP_MODE:
				uv_cdn_param = (struct sensor_uv_cdn_level*)(nr_ptr->capture_param_ptr);
			break;
			case ISP_NR_VIDEO_MODE:
				uv_cdn_param = (struct sensor_uv_cdn_level*)(nr_ptr->video_param_ptr);
			break;
			case ISP_NR_PANO_MODE:
				uv_cdn_param = (struct sensor_uv_cdn_level*)(nr_ptr->pano_param_ptr);
			break;
			default :
				uv_cdn_param = (struct sensor_uv_cdn_level*)(nr_ptr->common_param_ptr);
			break;
		}
	}

	strength_level = PM_CLIP(strength_level, 0, SENSOR_SMART_LEVEL_NUM - 1);
	if (uv_cdn_param != NULL) {
		dst_cdn_ptr->cur.median_thru0 = uv_cdn_param[strength_level].median_thru0;
		dst_cdn_ptr->cur.median_thru1 = uv_cdn_param[strength_level].median_thru1;
		dst_cdn_ptr->cur.median_thrv0 = uv_cdn_param[strength_level].median_thrv0;
		dst_cdn_ptr->cur.median_thrv1 = uv_cdn_param[strength_level].median_thrv1;

		for (i = 0; i < 31; i++) {
			dst_cdn_ptr->cur.rangewu[i] = uv_cdn_param[strength_level].u_ranweight[i];
			dst_cdn_ptr->cur.rangewv[i] = uv_cdn_param[strength_level].v_ranweight[i];
		}

		dst_cdn_ptr->cur.gaussian_mode = uv_cdn_param[strength_level].cdn_gaussian_mode;
		dst_cdn_ptr->cur.median_mode = uv_cdn_param[strength_level].median_mode;
		dst_cdn_ptr->cur.median_writeback_en = uv_cdn_param[strength_level].median_writeback_en;
		dst_cdn_ptr->cur.filter_bypass = uv_cdn_param[strength_level].filter_bypass;
		dst_cdn_ptr->cur.median_thr = uv_cdn_param[strength_level].median_thr;
		dst_cdn_ptr->cur.bypass = uv_cdn_param[strength_level].bypass;
	}

	return rtn;
}

static isp_s32 _pm_uv_cdn_init_v1(void *dst_cdn_param, void *src_cdn_param, void *param1, void *param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i = 0;
	struct isp_pm_block_header *cdn_header_ptr = (struct isp_pm_block_header*)param1;
	struct sensor_uv_cdn_param *src_cdn_ptr = (struct sensor_uv_cdn_param*)src_cdn_param;
	struct isp_uv_cdn_param_v1 *dst_cdn_ptr = (struct isp_uv_cdn_param_v1*)dst_cdn_param;
	UNUSED(param2);

	dst_cdn_ptr->cur.bypass = cdn_header_ptr->bypass;
	dst_cdn_ptr->cur.level = SENSOR_SMART_LEVEL_DEFAULT;
	dst_cdn_ptr->cur_level = SENSOR_SMART_LEVEL_DEFAULT;

	dst_cdn_ptr->uv_cdn_ptr = src_cdn_ptr->param_ptr;
	dst_cdn_ptr->multi_mode_enable = src_cdn_ptr->reserved2[0];

	dst_cdn_ptr->cur.median_thru0 = src_cdn_ptr->uv_cdn_level.median_thru0;
	dst_cdn_ptr->cur.median_thru1 = src_cdn_ptr->uv_cdn_level.median_thru1;
	dst_cdn_ptr->cur.median_thrv0 = src_cdn_ptr->uv_cdn_level.median_thrv0;
	dst_cdn_ptr->cur.median_thrv1 = src_cdn_ptr->uv_cdn_level.median_thrv1;
	for (i = 0; i < 31; i++) {
		dst_cdn_ptr->cur.rangewu[i] = src_cdn_ptr->uv_cdn_level.u_ranweight[i];
		dst_cdn_ptr->cur.rangewv[i] = src_cdn_ptr->uv_cdn_level.v_ranweight[i];
	}
	dst_cdn_ptr->cur.gaussian_mode = src_cdn_ptr->uv_cdn_level.cdn_gaussian_mode;
	dst_cdn_ptr->cur.median_mode = src_cdn_ptr->uv_cdn_level.median_mode;
	dst_cdn_ptr->cur.median_writeback_en = src_cdn_ptr->uv_cdn_level.median_writeback_en;
	dst_cdn_ptr->cur.filter_bypass = src_cdn_ptr->uv_cdn_level.filter_bypass;
	dst_cdn_ptr->cur.median_thr = src_cdn_ptr->uv_cdn_level.median_thr;

	rtn = _pm_uv_cdn_convert_param(dst_cdn_ptr,SENSOR_SMART_LEVEL_DEFAULT,ISP_NR_COMMON_MODE);
	dst_cdn_ptr->cur.bypass |= cdn_header_ptr->bypass;
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("ISP_PM_UV_CDN_CONVERT_PARAM: error!");
		return rtn;
	}

	cdn_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_uv_cdn_set_param_v1(void *cdn_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *cdn_header_ptr = (struct isp_pm_block_header*)param_ptr1;
	struct isp_uv_cdn_param_v1 *cdn_ptr = (struct isp_uv_cdn_param_v1*)cdn_param;
	cdn_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_UV_CDN_BYPASS_V1:
		cdn_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_SMART_SETTING:
	{
		struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;
		struct isp_range val_range = {0};
		isp_u32 cur_level = 0;

		cdn_header_ptr->is_update = 0;
		val_range.min = 0;
		val_range.max = 255;

		rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_VALUE);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !");
			return rtn;
		}

		cur_level = (isp_u32)block_result->component[0].fix_data[0];

		if (cur_level != cdn_ptr->cur_level || nr_tool_flag[11] || block_result->mode_flag_changed) {
			cdn_ptr->cur_level = cur_level;
			cdn_ptr->cur.level = cur_level;
			cdn_header_ptr->is_update = 1;
			nr_tool_flag[11] = 0;
			block_result->mode_flag_changed = 0;
			rtn = _pm_uv_cdn_convert_param(cdn_ptr,cdn_ptr->cur.level,block_result->mode_flag);
			cdn_ptr->cur.bypass |= cdn_header_ptr->bypass;
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("ISP_PM_UV_CDN_CONVERT_PARAM: error!");
				return rtn;
			}
		}
	}
	break;

	default:
		cdn_header_ptr->is_update = ISP_ZERO;
	break;
	}

	ISP_LOGV("ISP_SMART: cmd=%d, update=%d, cur_level=%d", cmd, cdn_header_ptr->is_update,
					cdn_ptr->cur_level);

	return rtn;
}

static isp_s32 _pm_uv_cdn_get_param_v1(void *cdn_param, isp_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_uv_cdn_param_v1 *cdn_ptr = (struct isp_uv_cdn_param_v1*)cdn_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_UV_CDN;
	param_data_ptr->cmd = ISP_PM_BLK_ISP_SETTING;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &cdn_ptr->cur;
		param_data_ptr->data_size = sizeof(cdn_ptr->cur);
		*update_flag = ISP_ZERO;
	break;

	default:
	break;
	}

	return rtn;
}

static isp_s32 _pm_css_init_v1(void *dst_css_param, void *src_css_param, void *param1, void *param_ptr2)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i = 0;
	struct isp_css_param_v1 *dst_css_ptr = (struct isp_css_param_v1 *)dst_css_param;
	struct sensor_css_v1_param *src_css_ptr = (struct sensor_css_v1_param *)src_css_param;
	struct isp_pm_block_header *css_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param_ptr2);

	dst_css_ptr->cur.bypass = css_header_ptr->bypass;
	dst_css_ptr->cur.lh_chrom_th = src_css_ptr->lh.css_lh_chrom_th;
	for (i = 0; i < 7 ; i++) {
		dst_css_ptr->cur.chrom_lower_th[i] = src_css_ptr->chrom_thre.css_chrom_lower_th[i];
		dst_css_ptr->cur.chrom_high_th[i] = src_css_ptr->chrom_thre.css_chrom_high_th[i];
	}
	dst_css_ptr->cur.lum_low_shift = src_css_ptr->lum_thre.css_lum_low_shift;
	dst_css_ptr->cur.lum_hig_shift = src_css_ptr->lum_thre.css_lum_hig_shift;
	for (i = 0; i < 8 ; i++) {
		dst_css_ptr->cur.lh_ratio[i] = src_css_ptr->lh.css_lh_ratio[i];
		dst_css_ptr->cur.ratio[i] = src_css_ptr->ratio[i];
	}
	dst_css_ptr->cur.lum_low_th = src_css_ptr->lum_thre.css_lum_low_th;
	dst_css_ptr->cur.lum_ll_th = src_css_ptr->lum_thre.css_lum_ll_th;
	dst_css_ptr->cur.lum_hig_th = src_css_ptr->lum_thre.css_lum_hig_th;
	dst_css_ptr->cur.lum_hh_th = src_css_ptr->lum_thre.css_lum_hh_th;
	dst_css_ptr->cur.u_th_0_l = src_css_ptr->exclude[0].exclude_u_th_l;
	dst_css_ptr->cur.u_th_0_h = src_css_ptr->exclude[0].exclude_u_th_h;
	dst_css_ptr->cur.v_th_0_l = src_css_ptr->exclude[0].exclude_v_th_l;
	dst_css_ptr->cur.v_th_0_h = src_css_ptr->exclude[0].exclude_v_th_h;
	dst_css_ptr->cur.u_th_1_l = src_css_ptr->exclude[1].exclude_u_th_l;
	dst_css_ptr->cur.u_th_1_h = src_css_ptr->exclude[1].exclude_u_th_h;
	dst_css_ptr->cur.v_th_1_l = src_css_ptr->exclude[1].exclude_v_th_l;
	dst_css_ptr->cur.v_th_1_h = src_css_ptr->exclude[1].exclude_v_th_h;
	dst_css_ptr->cur.cutoff_th = src_css_ptr->chrom_thre.css_chrom_cutoff_th;
	css_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_css_set_param_v1(void *css_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_css_param_v1 *css_ptr = (struct isp_css_param_v1 *)css_param;
	struct isp_pm_block_header *css_header_ptr = PNULL;
	UNUSED(cmd);

	css_header_ptr = (struct isp_pm_block_header *)param_ptr1;
	css_header_ptr->is_update = 1;
	css_ptr->cur.bypass = *((isp_u32 *)param_ptr0);

	return rtn;

}

static isp_s32 _pm_css_get_param_v1(void *css_param, isp_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_css_param_v1 *css_ptr = (struct isp_css_param_v1 *)css_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_CSS_V1;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&css_ptr->cur;
		param_data_ptr->data_size = sizeof(css_ptr->cur);
		*update_flag = 0;
		break;

	default:
		break;
	}

	return rtn;
}

static isp_s32 _pm_saturation_init_v1(void *dst_csa_param, void *src_csa_param, void *param1, void *param_ptr2)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i = 0, j = 0;
	struct isp_chrom_saturation_param *dst_csa_ptr = (struct isp_chrom_saturation_param *)dst_csa_param;
	struct sensor_saturation_v1_param *src_csa_ptr = (struct sensor_saturation_v1_param *)src_csa_param;
	struct isp_pm_block_header *csa_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param_ptr2);

	dst_csa_ptr->cur.bypass = csa_header_ptr->bypass;
	dst_csa_ptr->cur.factor_u = src_csa_ptr->csa_factor_u[src_csa_ptr->index_u];
	dst_csa_ptr->cur.factor_v = src_csa_ptr->csa_factor_v[src_csa_ptr->index_v];
	dst_csa_ptr->cur_u_idx = src_csa_ptr->index_u;
	dst_csa_ptr->cur_v_idx = src_csa_ptr->index_v;
	for (i=0; i<SENSOR_LEVEL_NUM; i++) {
		dst_csa_ptr->tab[0][i] = src_csa_ptr->csa_factor_u[i];
		dst_csa_ptr->tab[1][i] = src_csa_ptr->csa_factor_v[i];
	}
	for (i=0; i<MAX_SCENEMODE_NUM; i++) {
		dst_csa_ptr->scene_mode_tab[0][i] = src_csa_ptr->scenemode[0][i];
		dst_csa_ptr->scene_mode_tab[1][i] = src_csa_ptr->scenemode[1][i];
	}

	csa_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_saturation_set_param_v1(void *csa_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_chrom_saturation_param *csa_ptr = (struct isp_chrom_saturation_param *)csa_param;
	struct isp_pm_block_header *csa_header_ptr = (struct isp_pm_block_header *)param_ptr1;

	csa_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_SATURATION_BYPASS:
		csa_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_SATURATION:
	{
		isp_u32 level = *((isp_u32*)param_ptr0);
		csa_ptr->cur_u_idx = level;
		csa_ptr->cur_v_idx = level;
		csa_ptr->cur.factor_u = csa_ptr->tab[0][csa_ptr->cur_u_idx];
		csa_ptr->cur.factor_v = csa_ptr->tab[1][csa_ptr->cur_v_idx];
	}
	break;

	case ISP_PM_BLK_SCENE_MODE:
	{
		isp_u32 idx = *((isp_u32*)param_ptr0);
		if (0 == idx) {
			csa_ptr->cur.factor_u = csa_ptr->tab[0][csa_ptr->cur_u_idx];
			csa_ptr->cur.factor_v = csa_ptr->tab[1][csa_ptr->cur_v_idx];
		} else {
			csa_ptr->cur.factor_u = csa_ptr->scene_mode_tab[0][idx];
			csa_ptr->cur.factor_v = csa_ptr->scene_mode_tab[1][idx];
		}
	}
	break;

	case ISP_PM_BLK_SMART_SETTING:
	{
		struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;
		struct isp_range val_range = {0};
		isp_u32 reduce_percent = 0;
		uint32_t factor_u = 0;
		uint32_t factor_v = 0;

		val_range.min = 0;
		val_range.max = 255;
		rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_VALUE);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !");
			return rtn;
		}

		reduce_percent = (isp_u32)block_result->component[0].fix_data[0];
		factor_u = csa_ptr->tab[0][csa_ptr->cur_u_idx];
		factor_v = csa_ptr->tab[1][csa_ptr->cur_v_idx];
		csa_ptr->cur.factor_u = (factor_u*reduce_percent)/255;
		csa_ptr->cur.factor_v = (factor_v*reduce_percent)/255;
	}
	break;


	default:
		csa_header_ptr->is_update = ISP_ZERO;
	break;
	}

	return rtn;
}

static isp_s32 _pm_saturation_get_param_v1(void *csa_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_chrom_saturation_param *csa_ptr = (struct isp_chrom_saturation_param *)csa_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_SATURATION_V1;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&csa_ptr->cur;
		param_data_ptr->data_size = sizeof(csa_ptr->cur);
		*update_flag = 0;
		break;

	default:
		break;
	}

	return rtn;
}

static isp_s32 _pm_hue_init_v1(void *dst_hue_param, void *src_hue_param, void *param1, void *param_ptr2)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_hue_param_v1 *dst_hue_ptr = (struct isp_hue_param_v1 *)dst_hue_param;
	struct sensor_hue_param_v1 *src_hue_ptr = (struct sensor_hue_param_v1 *)src_hue_param;
	struct isp_pm_block_header *hue_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param_ptr2);

	dst_hue_ptr->cur.bypass = hue_header_ptr->bypass;
	if (src_hue_ptr->cur_index < 16) {
		dst_hue_ptr->cur.theta = src_hue_ptr->hue_theta[src_hue_ptr->cur_index];
	} else {
		ISP_LOGE("error: the subscript is out of bounds-array");
	}
	hue_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_hue_set_param_v1(void *hue_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_hue_param_v1 *hue_ptr = (struct isp_hue_param_v1 *)hue_param;
	struct isp_pm_block_header *hue_header_ptr = (struct isp_pm_block_header *)param_ptr1;
	hue_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_HUE_BYPASS:
		hue_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_HUE:
	{
		isp_u32 level = *((isp_u32*)param_ptr0);
		hue_ptr->cur_idx = level;
		hue_ptr->cur.theta = hue_ptr->tab[hue_ptr->cur_idx];
	}
	break;

	default:
		hue_header_ptr->is_update = ISP_ZERO;
	break;
	}

	return rtn;
}

static isp_s32 _pm_hue_get_param_v1(void *hue_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_hue_param_v1 *hue_ptr = (struct isp_hue_param_v1 *)hue_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_HUE_V1;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&hue_ptr->cur;
		param_data_ptr->data_size = sizeof(hue_ptr->cur);
		*update_flag = 0;
		break;

	default:
		break;
	}

	return rtn;
}

static isp_u32 _pm_uv_postcdn_convert_param(void *dst_postcdn_param, isp_u32 strength_level, isp_u32 mode_flag)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_s32 i = 0, j = 0;
	struct isp_uv_postcdn_param *dst_postcdn_ptr = (struct isp_uv_postcdn_param *)dst_postcdn_param;
	struct sensor_uv_postcdn_level* postcdn_param;
	struct nr_param_ptr *nr_ptr;

	if (SENSOR_MULTI_MODE_FLAG != dst_postcdn_ptr->multi_mode_enable) {
		postcdn_param = (struct sensor_uv_postcdn_level*)(dst_postcdn_ptr->uv_postcdn_ptr);
	} else {
		nr_ptr = (struct nr_param_ptr*)(dst_postcdn_ptr->uv_postcdn_ptr);
		if (!nr_ptr) {
			return rtn;
		}

		switch(mode_flag) {
			case ISP_NR_COMMON_MODE:
				postcdn_param = (struct sensor_uv_postcdn_level*)(nr_ptr->common_param_ptr);
			break;
			case ISP_NR_CAP_MODE:
				postcdn_param = (struct sensor_uv_postcdn_level*)(nr_ptr->capture_param_ptr);
			break;
			case ISP_NR_VIDEO_MODE:
				postcdn_param = (struct sensor_uv_postcdn_level*)(nr_ptr->video_param_ptr);
			break;
			case ISP_NR_PANO_MODE:
				postcdn_param = (struct sensor_uv_postcdn_level*)(nr_ptr->pano_param_ptr);
			break;
			default :
				postcdn_param = (struct sensor_uv_postcdn_level*)(nr_ptr->common_param_ptr);
			break;
		}
	}

	strength_level = PM_CLIP(strength_level, 0, SENSOR_SMART_LEVEL_NUM - 1);
	if (postcdn_param != NULL) {
		for (i = 0; i < 7; i++) {
			dst_postcdn_ptr->cur.r_segu[0][i] = postcdn_param[strength_level].r_segu.r_seg0[i];
			dst_postcdn_ptr->cur.r_segu[1][i] = postcdn_param[strength_level].r_segu.r_seg1[i];
			dst_postcdn_ptr->cur.r_segv[0][i] = postcdn_param[strength_level].r_segv.r_seg0[i];
			dst_postcdn_ptr->cur.r_segv[1][i] = postcdn_param[strength_level].r_segv.r_seg1[i];
		}

		for (i =0; i < 15; i++) {
			for (j = 0; j < 5; j++)
				dst_postcdn_ptr->cur.r_distw[i][j]= postcdn_param[strength_level].r_distw.distw[i][j];
		}

		dst_postcdn_ptr->cur.uvthr0 = postcdn_param[strength_level].thruv.thr0;
		dst_postcdn_ptr->cur.uvthr1 = postcdn_param[strength_level].thruv.thr1;
		dst_postcdn_ptr->cur.thru0 = postcdn_param[strength_level].thru.thr0;
		dst_postcdn_ptr->cur.thru1 = postcdn_param[strength_level].thru.thr1;
		dst_postcdn_ptr->cur.thrv0 = postcdn_param[strength_level].thrv.thr0;
		dst_postcdn_ptr->cur.thrv1 = postcdn_param[strength_level].thrv.thr1;

		dst_postcdn_ptr->cur.downsample_bypass = postcdn_param[strength_level].downsample_bypass;
		dst_postcdn_ptr->cur.mode = postcdn_param[strength_level].postcdn_mode;
		dst_postcdn_ptr->cur.writeback_en = postcdn_param[strength_level].median_res_wb_en;
		dst_postcdn_ptr->cur.uvjoint = postcdn_param[strength_level].uv_joint;
		dst_postcdn_ptr->cur.median_mode = postcdn_param[strength_level].median_mode;
		dst_postcdn_ptr->cur.adapt_med_thr = postcdn_param[strength_level].adpt_med_thr;

		dst_postcdn_ptr->cur.start_row_mod4 = 0;
		dst_postcdn_ptr->cur.bypass = postcdn_param[strength_level].bypass;
	}

	return rtn;
}

static isp_s32 _pm_uv_postcdn_init(void *dst_postcdn_param, void *src_postcdn_param, void *param1, void *param_ptr2)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_uv_postcdn_param *dst_postcdn_ptr = (struct isp_uv_postcdn_param *)dst_postcdn_param;
	struct sensor_uv_postcdn_param *src_postcdn_ptr = (struct sensor_uv_postcdn_param *)src_postcdn_param;
	struct isp_pm_block_header *postcdn_header_ptr = (struct isp_pm_block_header *)param1;
	isp_s32 i = 0, j = 0;
	UNUSED(param_ptr2);

	dst_postcdn_ptr->cur.bypass = postcdn_header_ptr->bypass;
	dst_postcdn_ptr->cur.level = SENSOR_SMART_LEVEL_DEFAULT;
	dst_postcdn_ptr->cur_level = SENSOR_SMART_LEVEL_DEFAULT;

	dst_postcdn_ptr->uv_postcdn_ptr = src_postcdn_ptr->param_ptr;
	dst_postcdn_ptr->multi_mode_enable = src_postcdn_ptr->reserved[0];

	for (i = 0; i < 7; i++) {
		dst_postcdn_ptr->cur.r_segu[0][i] = src_postcdn_ptr->uv_postcdn_level.r_segu.r_seg0[i];
		dst_postcdn_ptr->cur.r_segu[1][i] = src_postcdn_ptr->uv_postcdn_level.r_segu.r_seg1[i];
		dst_postcdn_ptr->cur.r_segv[0][i] = src_postcdn_ptr->uv_postcdn_level.r_segv.r_seg0[i];
		dst_postcdn_ptr->cur.r_segv[1][i] = src_postcdn_ptr->uv_postcdn_level.r_segv.r_seg1[i];
	}
	for (i =0; i < 15; i++) {
		for (j = 0; j < 5; j++)
			dst_postcdn_ptr->cur.r_distw[i][j]= src_postcdn_ptr->uv_postcdn_level.r_distw.distw[i][j];
	}
	dst_postcdn_ptr->cur.uvthr0 = src_postcdn_ptr->uv_postcdn_level.thruv.thr0;
	dst_postcdn_ptr->cur.uvthr1 = src_postcdn_ptr->uv_postcdn_level.thruv.thr1;
	dst_postcdn_ptr->cur.thru0 = src_postcdn_ptr->uv_postcdn_level.thru.thr0;
	dst_postcdn_ptr->cur.thru1 = src_postcdn_ptr->uv_postcdn_level.thru.thr1;
	dst_postcdn_ptr->cur.thrv0 = src_postcdn_ptr->uv_postcdn_level.thrv.thr0;
	dst_postcdn_ptr->cur.thrv1 = src_postcdn_ptr->uv_postcdn_level.thrv.thr1;
	dst_postcdn_ptr->cur.downsample_bypass = src_postcdn_ptr->uv_postcdn_level.downsample_bypass;
	dst_postcdn_ptr->cur.mode = src_postcdn_ptr->uv_postcdn_level.postcdn_mode;
	dst_postcdn_ptr->cur.writeback_en = src_postcdn_ptr->uv_postcdn_level.median_res_wb_en;
	dst_postcdn_ptr->cur.uvjoint = src_postcdn_ptr->uv_postcdn_level.uv_joint;
	dst_postcdn_ptr->cur.median_mode = src_postcdn_ptr->uv_postcdn_level.median_mode;
	dst_postcdn_ptr->cur.adapt_med_thr = src_postcdn_ptr->uv_postcdn_level.adpt_med_thr;
	dst_postcdn_ptr->cur.start_row_mod4 = 0;

	rtn = _pm_uv_postcdn_convert_param(dst_postcdn_ptr,SENSOR_SMART_LEVEL_DEFAULT,ISP_NR_COMMON_MODE);
	dst_postcdn_ptr->cur.bypass |= postcdn_header_ptr->bypass;
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("ISP_PM_YUV_POSTCDN_CONVERT_PARAM: error!");
		return rtn;
	}

	postcdn_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_uv_postcdn_set_param(void *postcdn_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_uv_postcdn_param *postcdn_ptr = (struct isp_uv_postcdn_param *)postcdn_param;
	struct isp_pm_block_header *postcdn_header_ptr = (struct isp_pm_block_header *)param_ptr1;

	postcdn_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_UV_POST_CDN_BYPASS:
		postcdn_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_UV_POST_CDN_STRENGTH_LEVEL:
		postcdn_ptr->cur_level = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_SMART_SETTING:
	{
		struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;
		struct isp_range val_range = {0};
		isp_u32 cur_level = 0;

		postcdn_header_ptr->is_update = 0;
		val_range.min = 0;
		val_range.max = 255;

		rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_VALUE);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !");
			return rtn;
		}

		cur_level = (isp_u32)block_result->component[0].fix_data[0];

		if (cur_level != postcdn_ptr->cur_level || nr_tool_flag[12] || block_result->mode_flag_changed) {
			postcdn_ptr->cur_level = cur_level;
			postcdn_ptr->cur.level = cur_level;
			postcdn_header_ptr->is_update = 1;
			nr_tool_flag[12] = 0;
			block_result->mode_flag_changed = 0;
			rtn = _pm_uv_postcdn_convert_param(postcdn_ptr,postcdn_ptr->cur_level,block_result->mode_flag);
			postcdn_ptr->cur.bypass |= postcdn_header_ptr->bypass;
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("ISP_PM_YUV_POSTCDN_CONVERT_PARAM: error!");
				return rtn;
			}
		}
	}
	break;

	default:
		postcdn_header_ptr->is_update = ISP_ZERO;
	break;
	}

	ISP_LOGV("ISP_SMART: cmd=%d, update=%d, cur_level=%d", cmd, postcdn_header_ptr->is_update,
					postcdn_ptr->cur_level);

	return rtn;

}

static isp_s32 _pm_uv_postcdn_get_param(void *postcdn_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_uv_postcdn_param *postcdn_ptr = (struct isp_uv_postcdn_param *)postcdn_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_UV_POSTCDN;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&postcdn_ptr->cur;
		param_data_ptr->data_size = sizeof(postcdn_ptr->cur);
		*update_flag = 0;
		break;

	default:
		break;
	}

	return rtn;
}

static isp_u32 _pm_edge_convert_param(void *dst_edge_param, isp_u32 strength_level, isp_u32 mode_flag)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_s32 i = 0;
	struct isp_edge_param_v1 *dst_edge_ptr = (struct isp_edge_param_v1*)dst_edge_param;
	struct sensor_ee_level* edge_param;
	struct nr_param_ptr *nr_ptr;

	if (SENSOR_MULTI_MODE_FLAG != dst_edge_ptr->multi_mode_enable) {
		edge_param = (struct sensor_ee_level*)(dst_edge_ptr->edge_ptr);
	} else {
		nr_ptr = (struct nr_param_ptr*)(dst_edge_ptr->edge_ptr);
		if (!nr_ptr) {
			return rtn;
		}

		switch(mode_flag) {
			case ISP_NR_COMMON_MODE:
				edge_param = (struct sensor_ee_level*)(nr_ptr->common_param_ptr);
			break;
			case ISP_NR_CAP_MODE:
				edge_param = (struct sensor_ee_level*)(nr_ptr->capture_param_ptr);
			break;
			case ISP_NR_VIDEO_MODE:
				edge_param = (struct sensor_ee_level*)(nr_ptr->video_param_ptr);
			break;
			case ISP_NR_PANO_MODE:
				edge_param = (struct sensor_ee_level*)(nr_ptr->pano_param_ptr);
			break;
			default :
				edge_param = (struct sensor_ee_level*)(nr_ptr->common_param_ptr);
			break;
		}
	}

	strength_level = PM_CLIP(strength_level, 0, SENSOR_SMART_LEVEL_NUM - 1);
	if (edge_param != NULL) {
		dst_edge_ptr->cur.ee_flat_thr_1 = edge_param[strength_level].ee_flat_thrx.thr1;
		dst_edge_ptr->cur.ee_flat_thr_2 = edge_param[strength_level].ee_flat_thrx.thr2;
		dst_edge_ptr->cur.ee_txt_thr_1 = edge_param[strength_level].ee_txt_thrx.ee_txt_thr1;
		dst_edge_ptr->cur.ee_txt_thr_2 = edge_param[strength_level].ee_txt_thrx.ee_txt_thr2;
		dst_edge_ptr->cur.ee_txt_thr_3 = edge_param[strength_level].ee_txt_thrx.ee_txt_thr3;
		dst_edge_ptr->cur.ee_corner_sm_n = edge_param[strength_level].ee_corner_xx.ee_corner_sm.negative;
		dst_edge_ptr->cur.ee_corner_sm_p = edge_param[strength_level].ee_corner_xx.ee_corner_sm.positive;
		dst_edge_ptr->cur.ee_corner_gain_n = edge_param[strength_level].ee_corner_xx.ee_corner_gain.negative;
		dst_edge_ptr->cur.ee_corner_gain_p = edge_param[strength_level].ee_corner_xx.ee_corner_gain.positive;
		dst_edge_ptr->cur.ee_corner_th_n = edge_param[strength_level].ee_corner_xx.ee_corner_th.negative;
		dst_edge_ptr->cur.ee_corner_th_p = edge_param[strength_level].ee_corner_xx.ee_corner_th.positive;
		dst_edge_ptr->cur.ee_corner_cor = edge_param[strength_level].ee_corner_xx.ee_corner_cor;
		dst_edge_ptr->cur.ee_smooth_strength = edge_param[strength_level].ee_smooth_xx.ee_smooth_strength;
		dst_edge_ptr->cur.ee_smooth_thr = edge_param[strength_level].ee_smooth_xx.ee_smooth_thr;
		dst_edge_ptr->cur.sigma = edge_param[strength_level].sigma;
		dst_edge_ptr->cur.mode = edge_param[strength_level].mode;
		dst_edge_ptr->cur.ee_flat_smooth_mode = edge_param[strength_level].ee_smooth_xx.ee_flat_smooth_mode;
		dst_edge_ptr->cur.ee_edge_smooth_mode = edge_param[strength_level].ee_smooth_xx.ee_edge_smooth_mode;
		dst_edge_ptr->cur.ratio[0] = edge_param[strength_level].ratio.ratio1;
		dst_edge_ptr->cur.ratio[1] = edge_param[strength_level].ratio.ratio2;
		dst_edge_ptr->cur.ipd_flat_thr = edge_param[strength_level].ipd_xx.ipd_flat_thr;
		dst_edge_ptr->cur.ipd_bypass = edge_param[strength_level].ipd_xx.ipd_bypass;
		dst_edge_ptr->cur.ee_clip_after_smooth_en = edge_param[strength_level].ee_clip_after_smooth_en;
		dst_edge_ptr->cur.ee_t1_cfg = edge_param[strength_level].ee_tx_cfg.ee_t1_cfg;
		dst_edge_ptr->cur.ee_t2_cfg = edge_param[strength_level].ee_tx_cfg.ee_t2_cfg;
		dst_edge_ptr->cur.ee_t3_cfg = edge_param[strength_level].ee_tx_cfg.ee_t3_cfg;
		dst_edge_ptr->cur.ee_t4_cfg = edge_param[strength_level].ee_tx_cfg.ee_t4_cfg;
		dst_edge_ptr->cur.ee_cv_clip_n = edge_param[strength_level].ee_cv_clip_x.negative;
		dst_edge_ptr->cur.ee_cv_clip_p = edge_param[strength_level].ee_cv_clip_x.positive;
		dst_edge_ptr->cur.ee_r1_cfg = edge_param[strength_level].ee_rx_cfg.ee_r1_cfg;
		dst_edge_ptr->cur.ee_r2_cfg = edge_param[strength_level].ee_rx_cfg.ee_r2_cfg;
		dst_edge_ptr->cur.ee_r3_cfg = edge_param[strength_level].ee_rx_cfg.ee_r3_cfg;
		dst_edge_ptr->cur.ee_str_m_n = edge_param[strength_level].ee_str_m.negative;
		dst_edge_ptr->cur.ee_str_m_p = edge_param[strength_level].ee_str_m.positive;
		dst_edge_ptr->cur.ee_str_d_n = edge_param[strength_level].ee_str_d.negative;
		dst_edge_ptr->cur.ee_str_d_p = edge_param[strength_level].ee_str_d.positive;
		dst_edge_ptr->cur.ee_incr_d_n = edge_param[strength_level].ee_incr_d.negative;
		dst_edge_ptr->cur.ee_incr_d_p = edge_param[strength_level].ee_incr_d.positive;
		dst_edge_ptr->cur.ee_thr_d_n = edge_param[strength_level].ee_thr_d.negative;
		dst_edge_ptr->cur.ee_thr_d_p = edge_param[strength_level].ee_thr_d.positive;
		dst_edge_ptr->cur.ee_incr_m_n = edge_param[strength_level].ee_incr_m.negative;
		dst_edge_ptr->cur.ee_incr_m_p = edge_param[strength_level].ee_incr_m.positive;
		dst_edge_ptr->cur.ee_incr_b_n = edge_param[strength_level].ee_incr_b.negative;
		dst_edge_ptr->cur.ee_incr_b_p = edge_param[strength_level].ee_incr_b.positive;
		dst_edge_ptr->cur.ee_str_b_n = edge_param[strength_level].ee_str_b.negative;
		dst_edge_ptr->cur.ee_str_b_p = edge_param[strength_level].ee_str_b.positive;
		dst_edge_ptr->cur.bypass = edge_param[strength_level].bypass;
	}

	return rtn;

}

static isp_s32 _pm_edge_init_v1(void *dst_edge_param, void *src_edge_param, void *param1, void *param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i = 0;
	struct isp_pm_block_header *edge_header_ptr = (struct isp_pm_block_header*)param1;
	struct sensor_ee_param *src_edge_ptr = (struct sensor_ee_param*)src_edge_param;
	struct isp_edge_param_v1 *dst_edge_ptr = (struct isp_edge_param_v1*)dst_edge_param;
	UNUSED(param2);

	dst_edge_ptr->cur.bypass = edge_header_ptr->bypass;

	dst_edge_ptr->cur_idx = src_edge_ptr->cur_idx;
	dst_edge_ptr->cur.ee_level = SENSOR_SMART_LEVEL_DEFAULT;
	dst_edge_ptr->level = SENSOR_SMART_LEVEL_DEFAULT;

	for (i = 0; i < SENSOR_LEVEL_NUM; ++i) {
		dst_edge_ptr->tab[i] = src_edge_ptr->tab[i];
	}

	for (i=0; i<MAX_SCENEMODE_NUM; i++) {
		dst_edge_ptr->scene_mode_tab[i] = src_edge_ptr->scenemode[i];
	}

	dst_edge_ptr->edge_ptr = src_edge_ptr->param_ptr;
	dst_edge_ptr->multi_mode_enable = src_edge_ptr->reserved[0];

	dst_edge_ptr->cur.ee_flat_thr_1 = src_edge_ptr->ee_level.ee_flat_thrx.thr1;
	dst_edge_ptr->cur.ee_flat_thr_2 = src_edge_ptr->ee_level.ee_flat_thrx.thr2;
	dst_edge_ptr->cur.ee_txt_thr_1 = src_edge_ptr->ee_level.ee_txt_thrx.ee_txt_thr1;
	dst_edge_ptr->cur.ee_txt_thr_2 = src_edge_ptr->ee_level.ee_txt_thrx.ee_txt_thr2;
	dst_edge_ptr->cur.ee_txt_thr_3 = src_edge_ptr->ee_level.ee_txt_thrx.ee_txt_thr3;
	dst_edge_ptr->cur.ee_corner_sm_n = src_edge_ptr->ee_level.ee_corner_xx.ee_corner_sm.negative;
	dst_edge_ptr->cur.ee_corner_sm_p = src_edge_ptr->ee_level.ee_corner_xx.ee_corner_sm.positive;
	dst_edge_ptr->cur.ee_corner_gain_n = src_edge_ptr->ee_level.ee_corner_xx.ee_corner_gain.negative;
	dst_edge_ptr->cur.ee_corner_gain_p = src_edge_ptr->ee_level.ee_corner_xx.ee_corner_gain.positive;
	dst_edge_ptr->cur.ee_corner_th_n = src_edge_ptr->ee_level.ee_corner_xx.ee_corner_th.negative;
	dst_edge_ptr->cur.ee_corner_th_p = src_edge_ptr->ee_level.ee_corner_xx.ee_corner_th.positive;
	dst_edge_ptr->cur.ee_corner_cor = src_edge_ptr->ee_level.ee_corner_xx.ee_corner_cor;
	dst_edge_ptr->cur.ee_smooth_strength = src_edge_ptr->ee_level.ee_smooth_xx.ee_smooth_strength;
	dst_edge_ptr->cur.ee_smooth_thr = src_edge_ptr->ee_level.ee_smooth_xx.ee_smooth_thr;
	dst_edge_ptr->cur.sigma = src_edge_ptr->ee_level.sigma;
	dst_edge_ptr->cur.mode = src_edge_ptr->ee_level.mode;
	dst_edge_ptr->cur.ee_flat_smooth_mode = src_edge_ptr->ee_level.ee_smooth_xx.ee_flat_smooth_mode;
	dst_edge_ptr->cur.ee_edge_smooth_mode = src_edge_ptr->ee_level.ee_smooth_xx.ee_edge_smooth_mode;
	dst_edge_ptr->cur.ratio[0] = src_edge_ptr->ee_level.ratio.ratio1;
	dst_edge_ptr->cur.ratio[1] = src_edge_ptr->ee_level.ratio.ratio2;
	dst_edge_ptr->cur.ipd_flat_thr = src_edge_ptr->ee_level.ipd_xx.ipd_flat_thr;
	dst_edge_ptr->cur.ipd_bypass = src_edge_ptr->ee_level.ipd_xx.ipd_bypass;
	dst_edge_ptr->cur.ee_clip_after_smooth_en = src_edge_ptr->ee_level.ee_clip_after_smooth_en;
	dst_edge_ptr->cur.ee_t1_cfg = src_edge_ptr->ee_level.ee_tx_cfg.ee_t1_cfg;
	dst_edge_ptr->cur.ee_t2_cfg = src_edge_ptr->ee_level.ee_tx_cfg.ee_t2_cfg;
	dst_edge_ptr->cur.ee_t3_cfg = src_edge_ptr->ee_level.ee_tx_cfg.ee_t3_cfg;
	dst_edge_ptr->cur.ee_t4_cfg = src_edge_ptr->ee_level.ee_tx_cfg.ee_t4_cfg;
	dst_edge_ptr->cur.ee_cv_clip_n = src_edge_ptr->ee_level.ee_cv_clip_x.negative;
	dst_edge_ptr->cur.ee_cv_clip_p = src_edge_ptr->ee_level.ee_cv_clip_x.positive;
	dst_edge_ptr->cur.ee_r1_cfg = src_edge_ptr->ee_level.ee_rx_cfg.ee_r1_cfg;
	dst_edge_ptr->cur.ee_r2_cfg = src_edge_ptr->ee_level.ee_rx_cfg.ee_r2_cfg;
	dst_edge_ptr->cur.ee_r3_cfg = src_edge_ptr->ee_level.ee_rx_cfg.ee_r3_cfg;
	dst_edge_ptr->cur.ee_str_m_n = src_edge_ptr->ee_level.ee_str_m.negative;
	dst_edge_ptr->cur.ee_str_m_p = src_edge_ptr->ee_level.ee_str_m.positive;
	dst_edge_ptr->cur.ee_str_d_n = src_edge_ptr->ee_level.ee_str_d.negative;
	dst_edge_ptr->cur.ee_str_d_p = src_edge_ptr->ee_level.ee_str_d.positive;
	dst_edge_ptr->cur.ee_incr_d_n = src_edge_ptr->ee_level.ee_incr_d.negative;
	dst_edge_ptr->cur.ee_incr_d_p = src_edge_ptr->ee_level.ee_incr_d.positive;
	dst_edge_ptr->cur.ee_thr_d_n = src_edge_ptr->ee_level.ee_thr_d.negative;
	dst_edge_ptr->cur.ee_thr_d_p = src_edge_ptr->ee_level.ee_thr_d.positive;
	dst_edge_ptr->cur.ee_incr_m_n = src_edge_ptr->ee_level.ee_incr_m.negative;
	dst_edge_ptr->cur.ee_incr_m_p = src_edge_ptr->ee_level.ee_incr_m.positive;
	dst_edge_ptr->cur.ee_incr_b_n = src_edge_ptr->ee_level.ee_incr_b.negative;
	dst_edge_ptr->cur.ee_incr_b_p = src_edge_ptr->ee_level.ee_incr_b.positive;
	dst_edge_ptr->cur.ee_str_b_n = src_edge_ptr->ee_level.ee_str_b.negative;
	dst_edge_ptr->cur.ee_str_b_p = src_edge_ptr->ee_level.ee_str_b.positive;

	rtn = _pm_edge_convert_param(dst_edge_ptr,SENSOR_SMART_LEVEL_DEFAULT,ISP_NR_COMMON_MODE);
	dst_edge_ptr->cur.bypass |= edge_header_ptr->bypass;
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("ISP_PM_EDGE_CONVERT_PARAM: error!");
		return rtn;
	}

	edge_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_edge_set_param_v1(void *edge_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pm_block_header *edge_header_ptr = (struct isp_pm_block_header*)param_ptr1;
	struct isp_edge_param_v1 *edge_ptr = (struct isp_edge_param_v1*)edge_param;
	edge_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_EDGE_BYPASS:
		edge_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_EDGE_STRENGTH:
		edge_ptr->cur_idx = *((isp_u32*)param_ptr0);
		edge_ptr->cur.ee_level = edge_ptr->level + edge_ptr->tab[edge_ptr->cur_idx];
	break;

	case ISP_PM_BLK_SMART_SETTING:
	{
		struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;
		struct isp_range val_range = {0};
		isp_u32 level = 0;

		edge_header_ptr->is_update = 0;
		val_range.min = 0;
		val_range.max = 255;

		rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_VALUE);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !");
			return rtn;
		}

		level = (isp_u32)block_result->component[0].fix_data[0];

		if (level != edge_ptr->cur.ee_level || nr_tool_flag[13] || block_result->mode_flag_changed) {
			edge_ptr->cur.ee_level = level;
			edge_ptr->level = level;
			edge_header_ptr->is_update = 1;
			nr_tool_flag[13] = 0;
			block_result->mode_flag_changed = 0;

			rtn = _pm_edge_convert_param(edge_ptr,edge_ptr->level,block_result->mode_flag);
			edge_ptr->cur.bypass |= edge_header_ptr->bypass;
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("ISP_PM_EDGE_CONVERT_PARAM: error!");
				return rtn;
			}
		}
	}
	break;

	case ISP_PM_BLK_SCENE_MODE:
	{
		isp_u32 idx = *((isp_u32*)param_ptr0);
		if (0 == idx) {
			edge_ptr->cur.ee_level = edge_ptr->tab[edge_ptr->cur_idx] + edge_ptr->level;
		} else {
			edge_ptr->cur.ee_level = edge_ptr->scene_mode_tab[idx] + edge_ptr->level;
		}
	}
	break;

	default:
		edge_header_ptr->is_update = ISP_ZERO;
	break;
	}

	ISP_LOGV("ISP_SMART: cmd=%d, update=%d, cur_idx=%d", cmd, edge_header_ptr->is_update,
					edge_ptr->cur_idx);

	return rtn;
}

static isp_s32 _pm_edge_get_param_v1(void *edge_param, isp_u32 cmd, void *rtn_param0, void *rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_edge_param_v1 *edge_ptr = (struct isp_edge_param_v1*)edge_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_EDGE_V1;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &edge_ptr->cur;
		param_data_ptr->data_size = sizeof(edge_ptr->cur);
		*update_flag = ISP_ZERO;
	break;

	default:
	break;
	}

	return rtn;
}

static isp_s32 _pm_emboss_init_v1(void *dst_emboss_param, void *src_emboss_param, void *param1, void *param_ptr2)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_emboss_param_v1 *dst_emboss_ptr = (struct isp_emboss_param_v1 *)dst_emboss_param;
	struct sensor_emboss_param_v1 *src_emboss_ptr = (struct sensor_emboss_param_v1 *)src_emboss_param;
	struct isp_pm_block_header *emboss_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param_ptr2);

	dst_emboss_ptr->cur.y_bypass = emboss_header_ptr->bypass;
	dst_emboss_ptr->cur.uv_bypass = emboss_header_ptr->bypass;
	dst_emboss_ptr->cur.y_step = src_emboss_ptr->y_step;
	dst_emboss_ptr->cur.uv_step = src_emboss_ptr->uv_step;
	emboss_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_emboss_set_param_v1(void *emboss_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_emboss_param_v1 *emboss_ptr = (struct isp_emboss_param_v1 *)emboss_param;
	struct isp_pm_block_header *emboss_header_ptr = (struct isp_pm_block_header *)param_ptr1;

	emboss_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_EMBOSS_BYPASS:
		emboss_ptr->cur.y_bypass = *((isp_u32*)param_ptr0);
		emboss_ptr->cur.uv_bypass = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_SPECIAL_EFFECT:
	{
		isp_u32 idx = *((isp_u32*)param_ptr0);
		if (ISP_EFFECT_EMBOSS == idx) {
			emboss_ptr->cur.y_bypass = 0;
			emboss_ptr->cur.uv_bypass = 0;
		} else {
			emboss_ptr->cur.y_bypass = 1;
			emboss_ptr->cur.uv_bypass = 1;
		}
	}
	break;

	default:
		emboss_header_ptr->is_update = ISP_ZERO;
	break;
	}

	return rtn;
}

static isp_s32 _pm_emboss_get_param_v1(void *emboss_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_emboss_param_v1 *emboss_ptr = (struct isp_emboss_param_v1 *)emboss_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_EMBOSS_V1;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&emboss_ptr->cur;
		param_data_ptr->data_size = sizeof(emboss_ptr->cur);
		*update_flag = 0;
		break;

	default:
		break;
	}

	return rtn;
}

static isp_s32 _pm_yuv_ygamma_init(void *dst_gamc_param, void *src_gamc_param, void* param1, void* param_ptr2)
{
	isp_u32 i = 0;
	isp_u32 j = 0;
	isp_u32 index = 0;
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_yuv_ygamma_param *dst_gamc_ptr = (struct isp_yuv_ygamma_param*)dst_gamc_param;
	struct sensor_y_gamma_param *src_gamc_ptr = (struct sensor_y_gamma_param*)src_gamc_param;
	struct isp_pm_block_header *gamc_header_ptr = (struct isp_pm_block_header*)param1;
	UNUSED(param_ptr2);

	for (i = 0; i < SENSOR_GAMMA_NUM; i++) {
		for (j = 0; j < SENSOR_GAMMA_POINT_NUM; j++) {
			dst_gamc_ptr->curve_tab[i].points[j].x = src_gamc_ptr->curve_tab[i].points[j].x;
			dst_gamc_ptr->curve_tab[i].points[j].y = src_gamc_ptr->curve_tab[i].points[j].y;
		}
	}

	dst_gamc_ptr->cur_idx = src_gamc_ptr->cur_idx;
	dst_gamc_ptr->cur.bypass = gamc_header_ptr->bypass;
	index = src_gamc_ptr->cur_idx;

	for (j = 0; j < ISP_PINGPANG_YUV_YGAMMA_NUM; j++) {
		if (j < ISP_PINGPANG_YUV_YGAMMA_NUM - 1) {
			dst_gamc_ptr->cur.nodes[j].node_x = (src_gamc_ptr->curve_tab[index].points[j*2].x + src_gamc_ptr->curve_tab[index].points[j*2+1].x) >> 1;
			dst_gamc_ptr->cur.nodes[j].node_y = (src_gamc_ptr->curve_tab[index].points[j*2].y + src_gamc_ptr->curve_tab[index].points[j*2+1].y) >> 1;
		} else {
			dst_gamc_ptr->cur.nodes[j].node_x = src_gamc_ptr->curve_tab[index].points[j*2 -1].x;
			dst_gamc_ptr->cur.nodes[j].node_y = src_gamc_ptr->curve_tab[index].points[j*2 -1].y;
		}
	}

	for (i = 0; i < MAX_SPECIALEFFECT_NUM; i++) {
		for (j = 0; j < SENSOR_GAMMA_POINT_NUM; j++) {
			dst_gamc_ptr->specialeffect_tab[i].points[j].x = src_gamc_ptr->specialeffect[i].points[j].x;
			dst_gamc_ptr->specialeffect_tab[i].points[j].y = src_gamc_ptr->specialeffect[i].points[j].y;
		}
	}

	gamc_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_yuv_ygamma_set_param(void *gamc_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_yuv_ygamma_param *ygamma_ptr = (struct isp_yuv_ygamma_param*)gamc_param;
	struct isp_pm_block_header *gamc_header_ptr = (struct isp_pm_block_header*)param_ptr1;

	gamc_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_YGAMMA_BYPSS:
		ygamma_ptr->cur.bypass = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_YGAMMA:
	{
		uint32_t gamma_idx = *((isp_u32*)param_ptr0);
		uint32_t i;
		
		ygamma_ptr->cur_idx_weight.x0 = gamma_idx;
		ygamma_ptr->cur_idx_weight.x1 = gamma_idx;
		ygamma_ptr->cur_idx_weight.weight0 = 256;
		ygamma_ptr->cur_idx_weight.weight1 = 0;
		for (i = 0; i < ISP_PINGPANG_YUV_YGAMMA_NUM; i++) {
			if (i < ISP_PINGPANG_YUV_YGAMMA_NUM - 1) {
				ygamma_ptr->cur.nodes[i].node_x = (ygamma_ptr->curve_tab[gamma_idx].points[i*2].x + ygamma_ptr->curve_tab[gamma_idx].points[i*2+1].x) >> 1;
				ygamma_ptr->cur.nodes[i].node_y = (ygamma_ptr->curve_tab[gamma_idx].points[i*2].y + ygamma_ptr->curve_tab[gamma_idx].points[i*2+1].y) >> 1;
			} else {
				ygamma_ptr->cur.nodes[i].node_x = ygamma_ptr->curve_tab[gamma_idx].points[i*2 -1].x;
				ygamma_ptr->cur.nodes[i].node_y = ygamma_ptr->curve_tab[gamma_idx].points[i*2 -1].y;
			}
		}
	}

	break;

	case ISP_PM_BLK_SPECIAL_EFFECT:
	{
		isp_u32 idx = *((isp_u32*)param_ptr0);
		if (0 == idx) {
			int j = 0;
			for (j = 0; j < ISP_PINGPANG_YUV_YGAMMA_NUM; j++) {
				if (j < ISP_PINGPANG_YUV_YGAMMA_NUM - 1) {
					ygamma_ptr->cur.nodes[j].node_x = (ygamma_ptr->curve_tab[ygamma_ptr->cur_idx].points[j*2].x + ygamma_ptr->curve_tab[ygamma_ptr->cur_idx].points[j*2+1].x) >> 1;
					ygamma_ptr->cur.nodes[j].node_y = (ygamma_ptr->curve_tab[ygamma_ptr->cur_idx].points[j*2].y + ygamma_ptr->curve_tab[ygamma_ptr->cur_idx].points[j*2+1].y) >> 1;
				} else {
					ygamma_ptr->cur.nodes[j].node_x = ygamma_ptr->curve_tab[ygamma_ptr->cur_idx].points[j*2 -1].x;
					ygamma_ptr->cur.nodes[j].node_y = ygamma_ptr->curve_tab[ygamma_ptr->cur_idx].points[j*2 -1].y;
				}
			}
		} else {
			int j = 0;
			for (j = 0; j < ISP_PINGPANG_YUV_YGAMMA_NUM; j++) {
				if (j < ISP_PINGPANG_YUV_YGAMMA_NUM - 1) {
					ygamma_ptr->cur.nodes[j].node_x = (ygamma_ptr->specialeffect_tab[idx].points[j*2].x + ygamma_ptr->specialeffect_tab[idx].points[j*2+1].x) >> 1;
					ygamma_ptr->cur.nodes[j].node_y = (ygamma_ptr->specialeffect_tab[idx].points[j*2].y + ygamma_ptr->specialeffect_tab[idx].points[j*2+1].y) >> 1;
				} else {
					ygamma_ptr->cur.nodes[j].node_x = ygamma_ptr->specialeffect_tab[idx].points[j*2 -1].x;
					ygamma_ptr->cur.nodes[j].node_y = ygamma_ptr->specialeffect_tab[idx].points[j*2 -1].y;
				}
			}
		}
	}
	break;

	case ISP_PM_BLK_SMART_SETTING:
	{
		struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;
		struct isp_weight_value *weight_value = NULL;
		struct isp_weight_value gamc_value = {{0}, {0}};
		struct isp_range val_range = {0};
		int i;

		gamc_header_ptr->is_update = 0;

		val_range.min = 0;
		val_range.max = SENSOR_GAMMA_NUM - 1;
		rtn = _pm_check_smart_param(block_result, &val_range, 1,
						ISP_SMART_Y_TYPE_WEIGHT_VALUE);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !\n");
			return rtn;
		}

		weight_value = (struct isp_weight_value *)block_result->component[0].fix_data;
		gamc_value = *weight_value;

		gamc_value.weight[0] = gamc_value.weight[0] / (SMART_WEIGHT_UNIT / 16)
								* (SMART_WEIGHT_UNIT / 16);
		gamc_value.weight[1] = SMART_WEIGHT_UNIT - gamc_value.weight[0];
		if (gamc_value.value[0] != ygamma_ptr->cur_idx_weight.x0
			|| gamc_value.weight[0] != ygamma_ptr->cur_idx_weight.weight0) {

			void *src[2] = {NULL};
			void *dst = NULL;
			src[0] = &ygamma_ptr->curve_tab[gamc_value.value[0]].points[0].x;
			src[1] = &ygamma_ptr->curve_tab[gamc_value.value[1]].points[0].x;
			dst = &ygamma_ptr->final_curve;

			/*interpolate y coordinates*/
			rtn = isp_interp_data(dst, src, gamc_value.weight, SENSOR_GAMMA_POINT_NUM*2, ISP_INTERP_UINT16);
			if (ISP_SUCCESS == rtn) {
				ygamma_ptr->cur_idx_weight.x0 = weight_value->value[0];
				ygamma_ptr->cur_idx_weight.x1 = weight_value->value[1];
				ygamma_ptr->cur_idx_weight.weight0 = weight_value->weight[0];
				ygamma_ptr->cur_idx_weight.weight1 = weight_value->weight[1];

				if (ygamma_ptr->cur.bypass) {
					gamc_header_ptr->is_update = 0;
				} else {
					gamc_header_ptr->is_update = 1;
				}

				for (i = 0; i < ISP_PINGPANG_YUV_YGAMMA_NUM; i++) {
					if (i < ISP_PINGPANG_YUV_YGAMMA_NUM - 1) {
						ygamma_ptr->cur.nodes[i].node_x = (ygamma_ptr->final_curve.points[i*2].x + ygamma_ptr->final_curve.points[i*2+1].x) >> 1;
						ygamma_ptr->cur.nodes[i].node_y = (ygamma_ptr->final_curve.points[i*2].y + ygamma_ptr->final_curve.points[i*2+1].y) >> 1;
					} else {
						ygamma_ptr->cur.nodes[i].node_x = ygamma_ptr->final_curve.points[i*2 -1].x;
						ygamma_ptr->cur.nodes[i].node_y = ygamma_ptr->final_curve.points[i*2 -1].y;
					}
				}
			}
		}
	}
	break;


	default:
		gamc_header_ptr->is_update = 0;
	break;
	}

	return rtn;
}

static isp_s32 _pm_yuv_ygamma_get_param(void *gamc_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_yuv_ygamma_param *gamc_ptr = (struct isp_yuv_ygamma_param*)gamc_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_Y_GAMMC;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = &gamc_ptr->cur;
		param_data_ptr->data_size = sizeof(gamc_ptr->cur);
		*update_flag = 0;
	break;

	default:
	break;
	}

	return rtn;
}


static isp_s32 _pm_ydelay_init(void *dst_ydelay, void *src_ydelay, void *param1, void *param2)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_ydelay_param *dst_ptr = (struct isp_ydelay_param *)dst_ydelay;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param2);
	UNUSED(src_ydelay);
	dst_ptr->cur.bypass = header_ptr->bypass;

	header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_ydelay_set_param(void *ydelay_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_ydelay_param *dst_ptr = (struct isp_ydelay_param *)ydelay_param;
	struct isp_pm_block_header *header_ptr = (struct isp_pm_block_header*)param_ptr1;
	UNUSED(cmd);

	header_ptr->is_update = ISP_ONE;
	dst_ptr->cur.bypass = *((isp_u32*)param_ptr0);

	return rtn;
}

static isp_s32 _pm_ydelay_get_param(void *ydelay_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_ydelay_param *ydelay_ptr = (struct isp_ydelay_param *)ydelay_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_YDELAY;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
	{
		param_data_ptr->data_ptr = (void *)&ydelay_ptr->cur;
		param_data_ptr->data_size = sizeof(ydelay_ptr->cur);
		*update_flag = 0;
	}
		break;

	default:
		break;
	}

	return rtn;
}

static isp_u32 _pm_iircnr_iir_convert_param(void *dst_param, isp_u32 strength_level, isp_u32 mode_flag)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_s32 i = 0;
	struct isp_iircnr_iir_param *dst_iircnr_ptr = (struct isp_iircnr_iir_param*)dst_param;
	struct sensor_iircnr_level* iir_cnr_param;
	struct nr_param_ptr *nr_ptr;

	if (SENSOR_MULTI_MODE_FLAG != dst_iircnr_ptr->multi_mode_enable) {
		iir_cnr_param = (struct sensor_iircnr_level*)(dst_iircnr_ptr->iir_cnr_ptr);
	} else {
		nr_ptr = (struct nr_param_ptr*)(dst_iircnr_ptr->iir_cnr_ptr);
		if (!nr_ptr) {
			return rtn;
		}

		switch(mode_flag) {
			case ISP_NR_COMMON_MODE:
				iir_cnr_param = (struct sensor_iircnr_level*)(nr_ptr->common_param_ptr);
			break;
			case ISP_NR_CAP_MODE:
				iir_cnr_param = (struct sensor_iircnr_level*)(nr_ptr->capture_param_ptr);
			break;
			case ISP_NR_VIDEO_MODE:
				iir_cnr_param = (struct sensor_iircnr_level*)(nr_ptr->video_param_ptr);
			break;
			case ISP_NR_PANO_MODE:
				iir_cnr_param = (struct sensor_iircnr_level*)(nr_ptr->pano_param_ptr);
			break;
			default :
				iir_cnr_param = (struct sensor_iircnr_level*)(nr_ptr->common_param_ptr);
			break;
		}
	}

	strength_level = PM_CLIP(strength_level, 0, SENSOR_SMART_LEVEL_NUM - 1);
 	if (iir_cnr_param != NULL) {
		dst_iircnr_ptr->cur.mode = iir_cnr_param[strength_level].iircnr_iir_mode;
		dst_iircnr_ptr->cur.y_th = iir_cnr_param[strength_level].iircnr_y_th;
		dst_iircnr_ptr->cur.uv_th = iir_cnr_param[strength_level].iircnr_uv_th;
		dst_iircnr_ptr->cur.uv_pg_th = iir_cnr_param[strength_level].iircnr_uv_pg_th;
		dst_iircnr_ptr->cur.uv_dist = iir_cnr_param[strength_level].iircnr_uv_dist;
		dst_iircnr_ptr->cur.uv_low_thr2 = iir_cnr_param[strength_level].iircnr_uv_low_thr2;
		dst_iircnr_ptr->cur.uv_low_thr1 = iir_cnr_param[strength_level].iircnr_uv_low_thr1;
		dst_iircnr_ptr->cur.uv_s_th = iir_cnr_param[strength_level].iircnr_uv_s_th;
		dst_iircnr_ptr->cur.alpha_low_u = iir_cnr_param[strength_level].iircnr_alpha_low_u;
		dst_iircnr_ptr->cur.alpha_low_v = iir_cnr_param[strength_level].iircnr_alpha_low_v;
		dst_iircnr_ptr->cur.uv_high_thr2 = iir_cnr_param[strength_level].iircnr_uv_high_thr2;
		dst_iircnr_ptr->cur.ymd_u = iir_cnr_param[strength_level].iircnr_ymd_u;
		dst_iircnr_ptr->cur.ymd_v = iir_cnr_param[strength_level].iircnr_ymd_v;
		dst_iircnr_ptr->cur.slope = iir_cnr_param[strength_level].iircnr_slope;
		dst_iircnr_ptr->cur.factor = iir_cnr_param[strength_level].iircnr_middle_factor;
		dst_iircnr_ptr->cur.bypass = iir_cnr_param[strength_level].bypass;
	}

	return rtn;
}


static isp_s32 _pm_iircnr_iir_init(void *dst_iircnr_param, void *src_iircnr_param, void *param1, void *param_ptr2)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_iircnr_iir_param *dst_iircnr_ptr = (struct isp_iircnr_iir_param *)dst_iircnr_param;
	struct sensor_iircnr_param *src_iir_nr_ptr = (struct sensor_iircnr_param *)src_iircnr_param;
	struct isp_pm_block_header *iircnr_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param_ptr2);

	dst_iircnr_ptr->cur.bypass = iircnr_header_ptr->bypass;
	dst_iircnr_ptr->cur.iirnr_level = SENSOR_SMART_LEVEL_DEFAULT;
	dst_iircnr_ptr->cur_level = SENSOR_SMART_LEVEL_DEFAULT;

	dst_iircnr_ptr->iir_cnr_ptr = src_iir_nr_ptr->param_ptr;
	dst_iircnr_ptr->multi_mode_enable = src_iir_nr_ptr->reserved[0];

	dst_iircnr_ptr->cur.mode = src_iir_nr_ptr->iircnr_level.iircnr_iir_mode;
	dst_iircnr_ptr->cur.y_th = src_iir_nr_ptr->iircnr_level.iircnr_y_th;
	dst_iircnr_ptr->cur.uv_th = src_iir_nr_ptr->iircnr_level.iircnr_uv_th;
	dst_iircnr_ptr->cur.uv_pg_th = src_iir_nr_ptr->iircnr_level.iircnr_uv_pg_th;
	dst_iircnr_ptr->cur.uv_dist = src_iir_nr_ptr->iircnr_level.iircnr_uv_dist;
	dst_iircnr_ptr->cur.uv_low_thr2 = src_iir_nr_ptr->iircnr_level.iircnr_uv_low_thr2;
	dst_iircnr_ptr->cur.uv_low_thr1 = src_iir_nr_ptr->iircnr_level.iircnr_uv_low_thr1;
	dst_iircnr_ptr->cur.uv_s_th = src_iir_nr_ptr->iircnr_level.iircnr_uv_s_th;
	dst_iircnr_ptr->cur.alpha_low_u = src_iir_nr_ptr->iircnr_level.iircnr_alpha_low_u;
	dst_iircnr_ptr->cur.alpha_low_v = src_iir_nr_ptr->iircnr_level.iircnr_alpha_low_v;
	dst_iircnr_ptr->cur.uv_high_thr2 = src_iir_nr_ptr->iircnr_level.iircnr_uv_high_thr2;
	dst_iircnr_ptr->cur.ymd_u = src_iir_nr_ptr->iircnr_level.iircnr_ymd_u;
	dst_iircnr_ptr->cur.ymd_v = src_iir_nr_ptr->iircnr_level.iircnr_ymd_v;
	dst_iircnr_ptr->cur.slope = src_iir_nr_ptr->iircnr_level.iircnr_slope;
	dst_iircnr_ptr->cur.factor = src_iir_nr_ptr->iircnr_level.iircnr_middle_factor;

	rtn=_pm_iircnr_iir_convert_param(dst_iircnr_ptr, SENSOR_SMART_LEVEL_DEFAULT, ISP_NR_COMMON_MODE);
	dst_iircnr_ptr->cur.bypass |= iircnr_header_ptr->bypass;
	 if (ISP_SUCCESS != rtn) {
		ISP_LOGE("ISP_PM_IIRCNR_CONVERT_PARAM: error!");
		return rtn;
	 }

	iircnr_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_iircnr_iir_set_param(void *iircnr_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 index = 0;
	struct isp_iircnr_iir_param *iircnr_ptr = (struct isp_iircnr_iir_param *)iircnr_param;
	struct isp_pm_block_header *iircnr_header_ptr = (struct isp_pm_block_header *)param_ptr1;

	iircnr_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_IIR_NR_STRENGTH_LEVEL:
		iircnr_ptr->cur_level = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_SMART_SETTING:
	{
		struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;
		struct isp_range val_range = {0};
		isp_u32 cur_level = 0;

		iircnr_header_ptr->is_update = 0;
		val_range.min = 0;
		val_range.max = 255;

		rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_VALUE);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !");
			return rtn;
		}

		cur_level = (isp_u32)block_result->component[0].fix_data[0];

		if (cur_level != iircnr_ptr->cur_level || nr_tool_flag[14] || block_result->mode_flag_changed) {
			iircnr_ptr->cur_level = cur_level;
			iircnr_ptr->cur.iirnr_level = cur_level;
			iircnr_header_ptr->is_update = 1;
			nr_tool_flag[14] = 0;
			block_result->mode_flag_changed = 0;
			rtn=_pm_iircnr_iir_convert_param(iircnr_ptr, iircnr_ptr->cur_level, block_result->mode_flag);
			iircnr_ptr->cur.bypass |= iircnr_header_ptr->bypass;
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("ISP_PM_IIRCNR_CONVERT_PARAM: error!");
				return rtn;
			 }
		}
	}
	break;

	default:
		iircnr_header_ptr->is_update = ISP_ZERO;
	break;
	}

	ISP_LOGV("ISP_SMART: cmd=%d, update=%d, cur_level=%d", cmd, iircnr_header_ptr->is_update,
					iircnr_ptr->cur_level);

	return rtn;

}

static isp_s32 _pm_iircnr_iir_get_param(void *iircnr_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_iircnr_iir_param *iircnr_ptr = (struct isp_iircnr_iir_param *)iircnr_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_IIRCNR_IIR;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&iircnr_ptr->cur;
		param_data_ptr->data_size = sizeof(iircnr_ptr->cur);
		*update_flag = 0;
		break;

	default:
		break;
	}

	return rtn;
}

static isp_u32 _pm_iir_yrandom_convert_param(void *dst_iir_yrandom_param, isp_u32 strength_level, isp_u32 mode_flag)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_s32 i = 0;
	struct isp_iircnr_yrandom_param *dst_iir_yrandom_ptr = (struct isp_iircnr_yrandom_param*)dst_iir_yrandom_param;
	struct sensor_iircnr_yrandom_level *iir_yrandom_param;
	struct nr_param_ptr *nr_ptr;

	if (SENSOR_MULTI_MODE_FLAG != dst_iir_yrandom_ptr->multi_mode_enable) {
		iir_yrandom_param = (struct sensor_iircnr_yrandom_level*)(dst_iir_yrandom_ptr->iir_yrandom_ptr);
	} else {
		nr_ptr = (struct nr_param_ptr*)(dst_iir_yrandom_ptr->iir_yrandom_ptr);
		if (!nr_ptr) {
			return rtn;
		}

		switch(mode_flag) {
			case ISP_NR_COMMON_MODE:
				iir_yrandom_param = (struct sensor_iircnr_yrandom_level*)(nr_ptr->common_param_ptr);
			break;
			case ISP_NR_CAP_MODE:
				iir_yrandom_param = (struct sensor_iircnr_yrandom_level*)(nr_ptr->capture_param_ptr);
			break;
			case ISP_NR_VIDEO_MODE:
				iir_yrandom_param = (struct sensor_iircnr_yrandom_level*)(nr_ptr->video_param_ptr);
			break;
			case ISP_NR_PANO_MODE:
				iir_yrandom_param = (struct sensor_iircnr_yrandom_level*)(nr_ptr->pano_param_ptr);
			break;
			default :
				iir_yrandom_param = (struct sensor_iircnr_yrandom_level*)(nr_ptr->common_param_ptr);
			break;
		}
	}

	strength_level = PM_CLIP(strength_level, 0, SENSOR_SMART_LEVEL_NUM - 1);
	if (iir_yrandom_param != NULL) {
		dst_iir_yrandom_ptr->cur.seed = iir_yrandom_param[strength_level].yrandom_seed;
		dst_iir_yrandom_ptr->cur.offset =  iir_yrandom_param[strength_level].yrandom_offset;
		dst_iir_yrandom_ptr->cur.shift =  iir_yrandom_param[strength_level].yrandom_shift;
		for (i = 0; i < 8; i++) {
			dst_iir_yrandom_ptr->cur.takeBit[i] =  iir_yrandom_param[strength_level].yrandom_takebit[i];
		}
		dst_iir_yrandom_ptr->cur.bypass =  iir_yrandom_param[strength_level].bypass;
	}
	else {
		ISP_LOGE("error: the subscript is out of bounds-array");
	}

	return rtn;
}

static isp_s32 _pm_iircnr_yrandom_init(void *dst_iircnr_param, void *src_iircnr_param, void *param1, void *param_ptr2)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i = 0;
	struct isp_iircnr_yrandom_param *dst_iir_yrandom_ptr = (struct isp_iircnr_yrandom_param *)dst_iircnr_param;
	struct sensor_iircnr_yrandom_param *src_iir_yrandom_ptr = (struct sensor_iircnr_yrandom_param *)src_iircnr_param;
	struct isp_pm_block_header *iir_yrandom_header_ptr = (struct isp_pm_block_header *)param1;
	UNUSED(param_ptr2);

	dst_iir_yrandom_ptr->cur.bypass = iir_yrandom_header_ptr->bypass;
	dst_iir_yrandom_ptr->cur_level = SENSOR_SMART_LEVEL_DEFAULT;
	dst_iir_yrandom_ptr->iir_yrandom_ptr = src_iir_yrandom_ptr->param_ptr;
	dst_iir_yrandom_ptr->multi_mode_enable = src_iir_yrandom_ptr->reserved[0];

	dst_iir_yrandom_ptr->cur.mode = 0;
	dst_iir_yrandom_ptr->cur.init = 0;
	dst_iir_yrandom_ptr->cur.seed = src_iir_yrandom_ptr->iircnr_yrandom_level.yrandom_seed;
	dst_iir_yrandom_ptr->cur.offset = src_iir_yrandom_ptr->iircnr_yrandom_level.yrandom_offset;
	dst_iir_yrandom_ptr->cur.shift = src_iir_yrandom_ptr->iircnr_yrandom_level.yrandom_shift;
	for (i = 0; i < 8; i++) {
		dst_iir_yrandom_ptr->cur.takeBit[i] = src_iir_yrandom_ptr->iircnr_yrandom_level.yrandom_takebit[i];
	}

	rtn = _pm_iir_yrandom_convert_param(dst_iir_yrandom_ptr,SENSOR_SMART_LEVEL_DEFAULT,ISP_NR_COMMON_MODE);
	dst_iir_yrandom_ptr->cur.bypass |= iir_yrandom_header_ptr->bypass;

	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("ISP_PM_IIR_YRANDOM_CONVERT_PARAM: error!");
		return rtn;
	}
	iir_yrandom_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_iircnr_yrandom_set_param(void *iircnr_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 index = 0;
	struct isp_iircnr_yrandom_param *iircnr_ptr = (struct isp_iircnr_yrandom_param *)iircnr_param;
	struct isp_pm_block_header *iircnr_header_ptr = PNULL;

	iircnr_header_ptr = (struct isp_pm_block_header *)param_ptr1;
	iircnr_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_IIR_YRANDOM_BYPASS:
		iircnr_ptr->cur.bypass = *((isp_u32 *)param_ptr0);
	break;
	case ISP_PM_BLK_SMART_SETTING:
	{
		struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;
		struct isp_range val_range = {0};
		isp_u32 cur_level = 0;

		iircnr_header_ptr->is_update = 0;
		val_range.min = 0;
		val_range.max = 255;

		rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_VALUE);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !");
			return rtn;
		}

		cur_level = (isp_u32)block_result->component[0].fix_data[0];

		if (cur_level != iircnr_ptr->cur_level || nr_tool_flag[15] || block_result->mode_flag_changed) {
			iircnr_ptr->cur_level = cur_level;
			iircnr_header_ptr->is_update = 1;
			nr_tool_flag[15] = 0;
			block_result->mode_flag_changed = 0;
			rtn = _pm_iir_yrandom_convert_param(iircnr_ptr,iircnr_ptr->cur_level,block_result->mode_flag);
			iircnr_ptr->cur.bypass |= iircnr_header_ptr->bypass;
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("ISP_PM_IIR_YRANDOM_CONVERT_PARAM: error!");
				return rtn;
			}
		}
	}
	break;

	default:
		iircnr_header_ptr->is_update = ISP_ZERO;
	break;
	}
	ISP_LOGI("ISP_SMART: cmd = %d, is_update = %d, cur_level=%d",
			cmd, iircnr_header_ptr->is_update, iircnr_ptr->cur_level);
	return rtn;
}

static isp_s32 _pm_iircnr_yrandom_get_param(void *iircnr_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_iircnr_yrandom_param *iircnr_ptr = (struct isp_iircnr_yrandom_param *)iircnr_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->id = ISP_BLK_IIRCNR_YRANDOM;
	param_data_ptr->cmd = cmd;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
		param_data_ptr->data_ptr = (void *)&iircnr_ptr->cur;
		param_data_ptr->data_size = sizeof(iircnr_ptr->cur);
		*update_flag = 0;
		break;

	default:
		break;
	}

	return rtn;
}

static isp_u32 _pm_uv_div_convert_param(void *dst_param, isp_u32 strength_level, isp_u32 mode_flag)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_s32 i = 0;
	struct isp_cce_uvdiv_param_v1 *dst_uvdiv_ptr = (struct isp_cce_uvdiv_param_v1*)dst_param;
	struct sensor_cce_uvdiv_level* cce_uvdiv_param;
	struct nr_param_ptr *nr_ptr;

	if (SENSOR_MULTI_MODE_FLAG != dst_uvdiv_ptr->multi_mode_enable) {
		cce_uvdiv_param = (struct sensor_cce_uvdiv_level*)(dst_uvdiv_ptr->uv_div_ptr);
	} else {
		nr_ptr = (struct nr_param_ptr*)(dst_uvdiv_ptr->uv_div_ptr);
		if (!nr_ptr) {
			return rtn;
		}

		switch(mode_flag) {
			case ISP_NR_COMMON_MODE:
				cce_uvdiv_param = (struct sensor_cce_uvdiv_level*)(nr_ptr->common_param_ptr);
			break;
			case ISP_NR_CAP_MODE:
				cce_uvdiv_param = (struct sensor_cce_uvdiv_level*)(nr_ptr->capture_param_ptr);
			break;
			case ISP_NR_VIDEO_MODE:
				cce_uvdiv_param = (struct sensor_cce_uvdiv_level*)(nr_ptr->video_param_ptr);
			break;
			case ISP_NR_PANO_MODE:
				cce_uvdiv_param = (struct sensor_cce_uvdiv_level*)(nr_ptr->pano_param_ptr);
			break;
			default :
				cce_uvdiv_param = (struct sensor_cce_uvdiv_level*)(nr_ptr->common_param_ptr);
			break;
		}
	}

	strength_level = PM_CLIP(strength_level, 0, SENSOR_SMART_LEVEL_NUM - 1);
	if (cce_uvdiv_param != NULL) {
		dst_uvdiv_ptr->cur.lum_th_h_len = cce_uvdiv_param[strength_level].lum_th_h_len;
		dst_uvdiv_ptr->cur.lum_th_h = cce_uvdiv_param[strength_level].lum_th_h;
		dst_uvdiv_ptr->cur.lum_th_l_len = cce_uvdiv_param[strength_level].lum_th_l_len;
		dst_uvdiv_ptr->cur.lum_th_l = cce_uvdiv_param[strength_level].lum_th_l;
		dst_uvdiv_ptr->cur.chroma_min_h = cce_uvdiv_param[strength_level].chroma_min_h;
		dst_uvdiv_ptr->cur.chroma_min_l = cce_uvdiv_param[strength_level].chroma_min_l;
		dst_uvdiv_ptr->cur.chroma_max_h = cce_uvdiv_param[strength_level].chroma_max_h;
		dst_uvdiv_ptr->cur.chroma_max_l = cce_uvdiv_param[strength_level].chroma_max_l;
		dst_uvdiv_ptr->cur.u_th1_h = cce_uvdiv_param[strength_level].u_th1_h;
		dst_uvdiv_ptr->cur.u_th1_l = cce_uvdiv_param[strength_level].u_th1_l;
		dst_uvdiv_ptr->cur.u_th0_h = cce_uvdiv_param[strength_level].u_th0_h;
		dst_uvdiv_ptr->cur.u_th0_l = cce_uvdiv_param[strength_level].u_th0_l;
		dst_uvdiv_ptr->cur.v_th1_h = cce_uvdiv_param[strength_level].v_th1_h;
		dst_uvdiv_ptr->cur.v_th1_l = cce_uvdiv_param[strength_level].v_th1_l;
		dst_uvdiv_ptr->cur.v_th0_h = cce_uvdiv_param[strength_level].v_th0_h;
		dst_uvdiv_ptr->cur.v_th0_l = cce_uvdiv_param[strength_level].v_th0_l;
		for (i = 0; i < 9; i++) {
			dst_uvdiv_ptr->cur.ratio[i] = cce_uvdiv_param[strength_level].ratio[i];
		}
		dst_uvdiv_ptr->cur.base = cce_uvdiv_param[strength_level].base;
		dst_uvdiv_ptr->cur.uvdiv_bypass = cce_uvdiv_param[strength_level].bypass;
	}

	return rtn;
}

static isp_s32 _pm_uv_div_init_v1(void *dst_uv_div_param, void *src_uv_div_param, void *param1, void *param_ptr2)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i;
	struct sensor_cce_uvdiv_param_v1 *src_uvdiv_ptr = (struct sensor_cce_uvdiv_param_v1*)src_uv_div_param;
	struct isp_cce_uvdiv_param_v1 *dst_uvdiv_ptr = (struct isp_cce_uvdiv_param_v1*)dst_uv_div_param;
	struct isp_pm_block_header *uvdiv_header_ptr = (struct isp_pm_block_header*)param1;
	UNUSED(param_ptr2);

	dst_uvdiv_ptr->cur.uvdiv_bypass = uvdiv_header_ptr->bypass;
	dst_uvdiv_ptr->cur.level = SENSOR_SMART_LEVEL_DEFAULT;
	dst_uvdiv_ptr->cur_level = SENSOR_SMART_LEVEL_DEFAULT;

	dst_uvdiv_ptr->uv_div_ptr = src_uvdiv_ptr->param_ptr;
	dst_uvdiv_ptr->multi_mode_enable = src_uvdiv_ptr->reserved1[0];

	dst_uvdiv_ptr->cur.lum_th_h_len = src_uvdiv_ptr->cce_uvdiv_level.lum_th_h_len;
	dst_uvdiv_ptr->cur.lum_th_h = src_uvdiv_ptr->cce_uvdiv_level.lum_th_h;
	dst_uvdiv_ptr->cur.lum_th_l_len = src_uvdiv_ptr->cce_uvdiv_level.lum_th_l_len;
	dst_uvdiv_ptr->cur.lum_th_l = src_uvdiv_ptr->cce_uvdiv_level.lum_th_l;
	dst_uvdiv_ptr->cur.chroma_min_h = src_uvdiv_ptr->cce_uvdiv_level.chroma_min_h;
	dst_uvdiv_ptr->cur.chroma_min_l = src_uvdiv_ptr->cce_uvdiv_level.chroma_min_l;
	dst_uvdiv_ptr->cur.chroma_max_h = src_uvdiv_ptr->cce_uvdiv_level.chroma_max_h;
	dst_uvdiv_ptr->cur.chroma_max_l = src_uvdiv_ptr->cce_uvdiv_level.chroma_max_l;
	dst_uvdiv_ptr->cur.u_th1_h = src_uvdiv_ptr->cce_uvdiv_level.u_th1_h;
	dst_uvdiv_ptr->cur.u_th1_l = src_uvdiv_ptr->cce_uvdiv_level.u_th1_l;
	dst_uvdiv_ptr->cur.u_th0_h = src_uvdiv_ptr->cce_uvdiv_level.u_th0_h;
	dst_uvdiv_ptr->cur.u_th0_l = src_uvdiv_ptr->cce_uvdiv_level.u_th0_l;
	dst_uvdiv_ptr->cur.v_th1_h = src_uvdiv_ptr->cce_uvdiv_level.v_th1_h;
	dst_uvdiv_ptr->cur.v_th1_l = src_uvdiv_ptr->cce_uvdiv_level.v_th1_l;
	dst_uvdiv_ptr->cur.v_th0_h = src_uvdiv_ptr->cce_uvdiv_level.v_th0_h;
	dst_uvdiv_ptr->cur.v_th0_l = src_uvdiv_ptr->cce_uvdiv_level.v_th0_l;
	for (i = 0; i < 9; i++) {
		dst_uvdiv_ptr->cur.ratio[i] = src_uvdiv_ptr->cce_uvdiv_level.ratio[i];
	}
	dst_uvdiv_ptr->cur.base = src_uvdiv_ptr->cce_uvdiv_level.base;

	rtn=_pm_uv_div_convert_param(dst_uvdiv_ptr, SENSOR_SMART_LEVEL_DEFAULT, ISP_NR_COMMON_MODE);
	dst_uvdiv_ptr->cur.uvdiv_bypass |= uvdiv_header_ptr->bypass;
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("ISP_PM_UVDIV_CONVERT_PARAM: error!");
		return rtn;
	}

	uvdiv_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_uv_div_set_param_v1(void *uv_div_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_cce_uvdiv_param_v1 *uvdiv_ptr = (struct isp_cce_uvdiv_param_v1*)uv_div_param;
	struct isp_pm_block_header *uvdiv_header_ptr = (struct isp_pm_block_header *)param_ptr1;

	uvdiv_header_ptr->is_update = ISP_ONE;

	switch (cmd) {
	case ISP_PM_BLK_UV_DIV_BYPSS:
		uvdiv_ptr->cur_level = *((isp_u32*)param_ptr0);
		uvdiv_ptr->cur.level = *((isp_u32*)param_ptr0);
	break;

	case ISP_PM_BLK_SMART_SETTING:
	{
		struct smart_block_result *block_result = (struct smart_block_result*)param_ptr0;
		struct isp_range val_range = {0};
		isp_u32 cur_idx = 0;

		uvdiv_header_ptr->is_update = 0;
		val_range.min = 0;
		val_range.max = 255;

		rtn = _pm_check_smart_param(block_result, &val_range, 1, ISP_SMART_Y_TYPE_VALUE);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("ISP_PM_BLK_SMART_SETTING: wrong param !");
			return rtn;
		}

		cur_idx = (isp_u32)block_result->component[0].fix_data[0];

		if (cur_idx != uvdiv_ptr->cur_level || nr_tool_flag[16] || block_result->mode_flag_changed) {
			uvdiv_ptr->cur_level = cur_idx;
			uvdiv_ptr->cur.level = cur_idx;
			uvdiv_header_ptr->is_update = 1;
			nr_tool_flag[16] = 0;
			block_result->mode_flag_changed = 0;
			rtn=_pm_uv_div_convert_param(uvdiv_ptr, uvdiv_ptr->cur_level,block_result->mode_flag);
		 	uvdiv_ptr->cur.uvdiv_bypass |= uvdiv_header_ptr->bypass;

			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("ISP_PM_UVDIV_CONVERT_PARAM: error!");
				return rtn;
			}
		}
	}
	break;

	default:
		uvdiv_header_ptr->is_update = ISP_ZERO;
	break;
	}

	ISP_LOGI("ISP_SMART: cmd=%d, update=%d, cur_idx=%d", cmd, uvdiv_header_ptr->is_update,
					uvdiv_ptr->cur_level);

	return rtn;
}

static isp_s32 _pm_uv_div_get_param_v1(void *uv_div_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_cce_uvdiv_param_v1 *uvdiv_ptr = (struct isp_cce_uvdiv_param_v1*)uv_div_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;

	param_data_ptr->cmd = cmd;
	param_data_ptr->id =ISP_BLK_UVDIV_V1;

	switch (cmd) {
	case ISP_PM_BLK_ISP_SETTING:
	{
		param_data_ptr->data_ptr = (void*)&uvdiv_ptr->cur;
		param_data_ptr->data_size = sizeof(uvdiv_ptr->cur);
		param_data_ptr->user_data[0] = uvdiv_ptr->cur.uvdiv_bypass;
		*update_flag = ISP_ZERO;
	}
	break;
		default:
		break;
	}
	return rtn;
}

static isp_s32 _pm_smart_init(void *dst_smart_param, void *src_smart_param, void *param1, void *param_ptr2)
{
	isp_s32 rtn = ISP_SUCCESS;

	struct isp_smart_param *src_smart_ptr = (struct isp_smart_param*)src_smart_param;
	struct isp_smart_param *dst_smart_ptr = (struct isp_smart_param*)dst_smart_param;
	struct isp_pm_block_header *smart_header_ptr = (struct isp_pm_block_header*)param1;
	UNUSED(param_ptr2);

	memcpy(dst_smart_ptr, src_smart_ptr, sizeof(struct isp_smart_param));

	smart_header_ptr->is_update = ISP_ONE;

	return rtn;
}

static isp_s32 _pm_smart_set_param(void *smart_param, isp_u32 cmd, void *param_ptr0, void *param_ptr1)
{
	isp_s32 rtn = ISP_SUCCESS;
	UNUSED(smart_param);
	UNUSED(cmd);
	UNUSED(param_ptr0);
	UNUSED(param_ptr1);

	return rtn;
}
static isp_s32 _pm_smart_get_param(void *smart_param, isp_u32 cmd, void* rtn_param0, void* rtn_param1)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_smart_param *smart_ptr = (struct isp_smart_param*)smart_param;
	struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data*)rtn_param0;
	isp_u32 *update_flag = (isp_u32*)rtn_param1;
	param_data_ptr->cmd = cmd;
	param_data_ptr->id =ISP_BLK_SMART;

	switch (cmd) {
		case ISP_PM_BLK_SMART_SETTING:
		{
			param_data_ptr->data_ptr = (void*)smart_ptr;
			param_data_ptr->data_size = sizeof(struct isp_smart_param);
			*update_flag = ISP_ZERO;
		}
		break;
	default:
	break;
	}
	return rtn;
}

/************************************Tshark2*******************************************************/
struct isp_block_operations s_pre_gbl_gain_ops_v1 = {_pm_pre_gbl_gain_init_v1,_pm_pre_gbl_gain_set_param_v1,_pm_pre_gbl_gain_get_param_v1, PNULL,PNULL};
struct isp_block_operations s_blc_ops_v1 = {_pm_blc_init_v1,_pm_blc_set_param_v1,_pm_blc_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_rgb_gain_ops_v1 = {_pm_rgb_gain_init_v1,_pm_rgb_gain_set_param_v1,_pm_rgb_gain_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_pre_wavelet_ops_v1 = {_pm_pre_wavelet_init_v1,_pm_pre_wavelet_set_param_v1,_pm_pre_wavelet_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_nlc_ops_v1 = {_pm_nlc_init_v1,_pm_nlc_set_param_v1,_pm_nlc_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_2d_lsc_ops = {_pm_2d_lsc_init,_pm_2d_lsc_set_param,_pm_2d_lsc_get_param, _pm_common_rest, _pm_2d_lsc_deinit};
struct isp_block_operations s_1d_lsc_ops = {_pm_1d_lsc_init,_pm_1d_lsc_set_param,_pm_1d_lsc_get_param, PNULL, PNULL};
struct isp_block_operations s_binning4awb_ops_v1 = {_pm_binning4awb_init_v1,_pm_binning4awb_set_param_v1,_pm_binning4awb_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_bpc_ops_v1 = {_pm_bpc_init_v1,_pm_bpc_set_param_v1,_pm_bpc_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_bl_nr_ops_v1 = {_pm_bl_nr_init_v1, _pm_bl_nr_set_param_v1, _pm_bl_nr_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_grgb_ops_v1 = {_pm_grgb_init_v1,_pm_grgb_set_param_v1,_pm_grgb_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_rgb_gain2_ops = {_pm_rgb_gain2_init,_pm_rgb_gain2_set_param,_pm_rgb_gain2_get_param, PNULL, PNULL};
struct isp_block_operations s_awb_ops_v1 = {_pm_awb_init_v1, _pm_awb_set_param_v1, _pm_awb_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_rgb_ae_ops  = {_pm_rgb_ae_init,_pm_rgb_ae_set_param,_pm_rgb_ae_get_param, PNULL, PNULL};
struct isp_block_operations s_nlm_ops_v1 = {_pm_nlm_init,_pm_nlm_set_param,_pm_nlm_get_param, _pm_common_rest, _pm_nlm_deinit};
struct isp_block_operations s_cfa_ops_v1= {_pm_cfa_init_v1,_pm_cfa_set_param_v1,_pm_cfa_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_cmc10_ops= {_pm_cmc10_init,_pm_cmc10_set_param,_pm_cmc10_get_param, PNULL, PNULL};
struct isp_block_operations s_frgb_gamc_ops= {_pm_frgb_gamc_init,_pm_frgb_gamc_set_param,_pm_frgb_gamc_get_param, PNULL, PNULL};
struct isp_block_operations s_cmc8_ops_v1 = {_pm_cmc8_init_v1,_pm_cmc8_set_param_v1,_pm_cmc8_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_ctm_ops= {_pm_ctm_init,_pm_ctm_set_param,_pm_ctm_get_param, _pm_common_rest, _pm_ctm_deinit};
struct isp_block_operations s_cce_ops_v1 = {_pm_cce_init_v1,_pm_cce_set_param_v1,_pm_cce_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_hsv_ops = {_pm_hsv_init,_pm_hsv_set_param,_pm_hsv_get_param, _pm_common_rest, _pm_hsv_deinit};
struct isp_block_operations s_csc_ops = {_pm_csc_init,_pm_csc_set_param,_pm_csc_get_param, PNULL, PNULL};
struct isp_block_operations s_rgb_precdn_ops_v1 = {_pm_rgb_pre_cdn_init_v1,_pm_rgb_pre_cdn_set_param_v1,_pm_rgb_pre_cdn_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_posterize_ops= {_pm_posterize_init,_pm_posterize_set_param,_pm_posterize_get_param, PNULL, PNULL};
struct isp_block_operations s_rgb_afm_ops_v1= {_pm_rgb_afm_init_v1,_pm_rgb_afm_set_param_v1,_pm_rgb_afm_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_yiq_aem_ops_v1 = {_pm_yiq_aem_init_v1,_pm_yiq_aem_set_param_v1,_pm_yiq_aem_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_yiq_afl_ops_v1 = {_pm_yiq_afl_init_v1,_pm_yiq_afl_set_param_v1,_pm_yiq_afl_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_yiq_afm_ops = {_pm_yiq_afm_init, _pm_yiq_afm_set_param, _pm_yiq_afm_get_param, PNULL, PNULL};
struct isp_block_operations s_yuv_precdn_ops = {_pm_yuv_precdn_init, _pm_yuv_precdn_set_param, _pm_yuv_precdn_get_param, PNULL, PNULL};
struct isp_block_operations s_prfy_ops = {_pm_prfy_init, _pm_prfy_set_param, _pm_prfy_get_param, PNULL, PNULL};
struct isp_block_operations s_hist_ops_v1 = {_pm_hist_init_v1, _pm_hist_set_param_v1, _pm_hist_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_hist2_ops_v1 = {_pm_hist2_init_v1, _pm_hist2_set_param_v1, _pm_hist2_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_auto_contrast_ops_v1 = {_pm_auto_contrast_init_v1, _pm_auto_contrast_set_param_v1, _pm_auto_contrast_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_uv_cdn_ops_v1 = {_pm_uv_cdn_init_v1, _pm_uv_cdn_set_param_v1, _pm_uv_cdn_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_edge_ops_v1 = {_pm_edge_init_v1, _pm_edge_set_param_v1, _pm_edge_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_css_ops_v1 = {_pm_css_init_v1,_pm_css_set_param_v1,_pm_css_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_saturation_ops_v1= {_pm_saturation_init_v1,_pm_saturation_set_param_v1,_pm_saturation_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_hue_ops_v1= {_pm_hue_init_v1,_pm_hue_set_param_v1,_pm_hue_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_uv_postcdn_ops= {_pm_uv_postcdn_init,_pm_uv_postcdn_set_param,_pm_uv_postcdn_get_param, PNULL, PNULL};
struct isp_block_operations s_emboss_ops_v1= {_pm_emboss_init_v1,_pm_emboss_set_param_v1,_pm_emboss_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_yuv_ygamma_ops = {_pm_yuv_ygamma_init,_pm_yuv_ygamma_set_param,_pm_yuv_ygamma_get_param, PNULL, PNULL};
struct isp_block_operations s_ydelay_ops= {_pm_ydelay_init,_pm_ydelay_set_param,_pm_ydelay_get_param, PNULL, PNULL};
struct isp_block_operations s_iircnr_iir_ops= {_pm_iircnr_iir_init,_pm_iircnr_iir_set_param,_pm_iircnr_iir_get_param, PNULL, PNULL};
struct isp_block_operations s_iircnr_yrandom_ops= {_pm_iircnr_yrandom_init,_pm_iircnr_yrandom_set_param,_pm_iircnr_yrandom_get_param, PNULL, PNULL};
struct isp_block_operations s_uvdiv_ops_v1 = {_pm_uv_div_init_v1, _pm_uv_div_set_param_v1, _pm_uv_div_get_param_v1, PNULL, PNULL};
struct isp_block_operations s_smart_ops = {_pm_smart_init, _pm_smart_set_param, _pm_smart_get_param, PNULL, PNULL};
/***********************************pike****************************************************/
struct isp_block_operations s_awb_ops_v2 = {_pm_awb_init_v2, _pm_awb_set_param_v2, _pm_awb_get_param_v2, PNULL, PNULL};
struct isp_block_operations s_nlm_ops_v2 = {_pm_nlm_init_v2,_pm_nlm_set_param_v2,_pm_nlm_get_param_v2, _pm_common_rest, _pm_nlm_deinit_v2};
struct isp_block_operations s_yiq_aem_ops_v2 = {_pm_yiq_aem_init_v2,_pm_yiq_aem_set_param_v2,_pm_yiq_aem_get_param_v2, PNULL, PNULL};
struct isp_block_operations s_yiq_afm_ops_v2 = {_pm_yiq_afm_init_v2, _pm_yiq_afm_set_param_v2, _pm_yiq_afm_get_param_v2, PNULL, PNULL};

struct isp_block_cfg s_blk_cfgs[] = {
	/************************************Tshark2*******************************************************/
	{ISP_BLK_FLASH_CALI, array_offset(struct isp_context, flash), sizeof(struct isp_flash_param), &s_flash_ops},
#ifdef CONFIG_CAMERA_ISP_VERSION_V3
	{ISP_BLK_PRE_GBL_GAIN_V1,array_offset(struct isp_context,pre_gbl_gain_v1),sizeof(struct isp_pre_global_gain_param_v1),&s_pre_gbl_gain_ops_v1},
	{ISP_BLK_BLC_V1,array_offset(struct isp_context,blc_v1),sizeof(struct isp_blc_param_v1),&s_blc_ops_v1},
	{ISP_BLK_RGB_GAIN_V1,array_offset(struct isp_context,rgb_gain_v1),sizeof(struct isp_rgb_gain_param_v1),&s_rgb_gain_ops_v1},
	{ISP_BLK_PRE_WAVELET_V1,array_offset(struct isp_context,pre_wavelet_v1),sizeof(struct isp_pre_wavelet_param_v1),&s_pre_wavelet_ops_v1},
	{ISP_BLK_NLC_V1,array_offset(struct isp_context,nlc_v1),sizeof(struct isp_nlc_param_v1),&s_nlc_ops_v1},
	{ISP_BLK_2D_LSC,array_offset(struct isp_context,lsc_2d),sizeof(struct isp_2d_lsc_param),&s_2d_lsc_ops},
	{ISP_BLK_1D_LSC,array_offset(struct isp_context,lsc_1d),sizeof(struct isp_1d_lsc_param),&s_1d_lsc_ops},
	{ISP_BLK_BINNING4AWB_V1,array_offset(struct isp_context,binning4awb_v1),sizeof(struct isp_binning4awb_param_v1),&s_binning4awb_ops_v1},
	{ISP_BLK_AWB_V1, array_offset(struct isp_context,awb_v1), sizeof(struct isp_awb_param_v1), &s_awb_ops_v1},
	{ISP_BLK_AE_V1, array_offset(struct isp_context,ae_v1), sizeof(struct isp_rgb_ae_param), &s_rgb_ae_ops},
	{ISP_BLK_BPC_V1,array_offset(struct isp_context,bpc_v1),sizeof(struct isp_bpc_param_v1),&s_bpc_ops_v1},
	{ISP_BLK_BL_NR_V1,array_offset(struct isp_context,bl_nr_v1),sizeof(struct isp_bl_nr_param_v1),&s_bl_nr_ops_v1},
	{ISP_BLK_GRGB_V1,array_offset(struct isp_context,grgb_v1),sizeof(struct isp_grgb_param_v1),&s_grgb_ops_v1},
	{ISP_BLK_RGB_GAIN2,array_offset(struct isp_context,rgb_gain2),sizeof(struct isp_rgb_gain2_param),&s_rgb_gain2_ops},
	{ISP_BLK_NLM,array_offset(struct isp_context,nlm_v1),sizeof(struct isp_nlm_param_v1),&s_nlm_ops_v1},
	{ISP_BLK_CFA_V1,              array_offset(struct isp_context,cfa_v1),           sizeof(struct isp_cfa_param_v1),                        &s_cfa_ops_v1},
	{ISP_BLK_CMC10,array_offset(struct isp_context,cmc10),sizeof(struct isp_cmc10_param),&s_cmc10_ops},
	{ISP_BLK_RGB_GAMC,    array_offset(struct isp_context,frgb_gamc),              sizeof(struct isp_frgb_gamc_param),                         &s_frgb_gamc_ops},
	{ISP_BLK_CMC8,array_offset(struct isp_context,cmc8_v1),sizeof(struct isp_cmc8_param_v1),&s_cmc8_ops_v1},
	{ISP_BLK_CTM,array_offset(struct isp_context,ctm),sizeof(struct isp_ctm_param),&s_ctm_ops},
	{ISP_BLK_CCE_V1,array_offset(struct isp_context,cce_v1),sizeof(struct isp_cce_param_v1),&s_cce_ops_v1},
	{ISP_BLK_HSV,array_offset(struct isp_context,hsv),sizeof(struct isp_hsv_param),&s_hsv_ops},
	{ISP_BLK_RADIAL_CSC,array_offset(struct isp_context,csc),sizeof(struct isp_csc_param),&s_csc_ops},
	{ISP_BLK_RGB_PRECDN,  array_offset(struct isp_context,rgb_pre_cdn), sizeof(struct isp_rgb_pre_cdn_param),         &s_rgb_precdn_ops_v1},
	{ISP_BLK_POSTERIZE,array_offset(struct isp_context,posterize),sizeof(struct isp_posterize_param),&s_posterize_ops},
	{ISP_BLK_AF_V1,array_offset(struct isp_context,rgb_afm_v1),sizeof(struct isp_rgb_afm_param_v1),&s_rgb_afm_ops_v1},
	{ISP_BLK_YIQ_AEM,array_offset(struct isp_context,yiq_aem_v1),sizeof(struct isp_yiq_aem_param_v1),&s_yiq_aem_ops_v1},
	{ISP_BLK_YIQ_AFL,array_offset(struct isp_context,yiq_afl_v1),sizeof(struct isp_yiq_afl_param_v1),&s_yiq_afl_ops_v1},
	{ISP_BLK_YIQ_AFM, array_offset(struct isp_context,yiq_afm),sizeof(struct isp_yiq_afm_param_v1),&s_yiq_afm_ops},
	{ISP_BLK_YUV_PRECDN,  array_offset(struct isp_context,yuv_precdn),              sizeof(struct isp_yuv_precdn_param),                             &s_yuv_precdn_ops},
	{ISP_BLK_PREF_V1, array_offset(struct isp_context,prfy),sizeof(struct isp_prfy_param),&s_prfy_ops},
	{ISP_BLK_BRIGHT, array_offset(struct isp_context, bright), sizeof(struct isp_bright_param), &s_bright_ops},
	{ISP_BLK_CONTRAST, array_offset(struct isp_context, contrast), sizeof(struct isp_contrast_param), &s_contrast_ops},
	{ISP_BLK_HIST_V1, array_offset(struct isp_context,hist_v1),sizeof(struct isp_hist_param_v1),&s_hist_ops_v1},
	{ISP_BLK_HIST2, array_offset(struct isp_context,hist2_v1),sizeof(struct isp_hist2_param_v1),&s_hist2_ops_v1},
	{ISP_BLK_AUTO_CONTRAST_V1, array_offset(struct isp_context,auto_contrast_v1),sizeof(struct isp_auto_contrast_param_v1),&s_auto_contrast_ops_v1},
	{ISP_BLK_UV_CDN,          array_offset(struct isp_context,uv_cdn_v1),              sizeof(struct isp_uv_cdn_param_v1),                        &s_uv_cdn_ops_v1},
	{ISP_BLK_EDGE_V1, array_offset(struct isp_context,edge_v1),sizeof(struct isp_edge_param_v1),&s_edge_ops_v1},
	{ISP_BLK_CSS_V1,array_offset(struct isp_context, css_v1),sizeof(struct isp_css_param_v1),&s_css_ops_v1},
	{ISP_BLK_SATURATION_V1,array_offset(struct isp_context,saturation_v1),sizeof(struct isp_chrom_saturation_param),&s_saturation_ops_v1},
	{ISP_BLK_HUE_V1,array_offset(struct isp_context,hue_v1),sizeof(struct isp_hue_param_v1),&s_hue_ops_v1},
	{ISP_BLK_UV_POSTCDN,     array_offset(struct isp_context,uv_postcdn),           sizeof(struct isp_uv_postcdn_param),                     &s_uv_postcdn_ops},
	{ISP_BLK_EMBOSS_V1,array_offset(struct isp_context,emboss_v1),sizeof(struct isp_emboss_param_v1),&s_emboss_ops_v1},
	{ISP_BLK_Y_GAMMC,array_offset(struct isp_context,yuv_ygamma),sizeof(struct isp_yuv_ygamma_param),&s_yuv_ygamma_ops},
	{ISP_BLK_YDELAY,array_offset(struct isp_context,ydelay),sizeof(struct isp_ydelay_param),&s_ydelay_ops},
	{ISP_BLK_IIRCNR_IIR,array_offset(struct isp_context,iircnr_iir),sizeof(struct isp_iircnr_iir_param),&s_iircnr_iir_ops},
	{ISP_BLK_IIRCNR_YRANDOM,array_offset(struct isp_context,iircnr_yrandom),sizeof(struct isp_iircnr_yrandom_param),&s_iircnr_yrandom_ops},
	{ISP_BLK_UVDIV_V1, array_offset(struct isp_context, uv_div_v1), sizeof(struct isp_cce_uvdiv_param_v1), &s_uvdiv_ops_v1},
	{ISP_BLK_SMART, array_offset(struct isp_context, smart), sizeof(struct isp_smart_param), &s_smart_ops},
#endif
	/************************************pike*******************************************************/
#ifdef CONFIG_CAMERA_ISP_VERSION_V4
	{ISP_BLK_PRE_GBL_GAIN_V1,array_offset(struct isp_context,pre_gbl_gain_v1),sizeof(struct isp_pre_global_gain_param_v1),&s_pre_gbl_gain_ops_v1},
	{ISP_BLK_BLC_V1,array_offset(struct isp_context,blc_v1),sizeof(struct isp_blc_param_v1),&s_blc_ops_v1},
	{ISP_BLK_RGB_GAIN_V1,array_offset(struct isp_context,rgb_gain_v1),sizeof(struct isp_rgb_gain_param_v1),&s_rgb_gain_ops_v1},
	{ISP_BLK_PRE_WAVELET_V1,array_offset(struct isp_context,pre_wavelet_v1),sizeof(struct isp_pre_wavelet_param_v1),&s_pre_wavelet_ops_v1},
	{ISP_BLK_NLC_V1,array_offset(struct isp_context,nlc_v1),sizeof(struct isp_nlc_param_v1),&s_nlc_ops_v1},
	{ISP_BLK_2D_LSC,array_offset(struct isp_context,lsc_2d),sizeof(struct isp_2d_lsc_param),&s_2d_lsc_ops},
	{ISP_BLK_1D_LSC,array_offset(struct isp_context,lsc_1d),sizeof(struct isp_1d_lsc_param),&s_1d_lsc_ops},
	{ISP_BLK_BINNING4AWB_V1,array_offset(struct isp_context,binning4awb_v1),sizeof(struct isp_binning4awb_param_v1),&s_binning4awb_ops_v1},
	{ISP_BLK_AWB_V1, array_offset(struct isp_context,awb_v2), sizeof(struct isp_awb_param_v2), &s_awb_ops_v2},
	{ISP_BLK_AE_V1, array_offset(struct isp_context,ae_v1), sizeof(struct isp_rgb_ae_param), &s_rgb_ae_ops},
	{ISP_BLK_BPC_V1,array_offset(struct isp_context,bpc_v1),sizeof(struct isp_bpc_param_v1),&s_bpc_ops_v1},
	{ISP_BLK_BL_NR_V1,array_offset(struct isp_context,bl_nr_v1),sizeof(struct isp_bl_nr_param_v1),&s_bl_nr_ops_v1},
	{ISP_BLK_GRGB_V1,array_offset(struct isp_context,grgb_v1),sizeof(struct isp_grgb_param_v1),&s_grgb_ops_v1},
	{ISP_BLK_RGB_GAIN2,array_offset(struct isp_context,rgb_gain2),sizeof(struct isp_rgb_gain2_param),&s_rgb_gain2_ops},
	{ISP_BLK_NLM, array_offset(struct isp_context,nlm_v2),sizeof(struct isp_nlm_param_v2),&s_nlm_ops_v2},
	{ISP_BLK_CFA_V1,              array_offset(struct isp_context,cfa_v1),           sizeof(struct isp_cfa_param_v1), &s_cfa_ops_v1},
	{ISP_BLK_CMC10,array_offset(struct isp_context,cmc10),sizeof(struct isp_cmc10_param), &s_cmc10_ops},
//	{ISP_BLK_HDR, array_offset(struct isp_context, hdr), sizeof(struct isp_hdr_param), &s_hdr_ops},
	{ISP_BLK_RGB_GAMC,    array_offset(struct isp_context,frgb_gamc), sizeof(struct isp_frgb_gamc_param), &s_frgb_gamc_ops},
	{ISP_BLK_CMC8,array_offset(struct isp_context,cmc8_v1),sizeof(struct isp_cmc8_param_v1), &s_cmc8_ops_v1},
	{ISP_BLK_CTM,array_offset(struct isp_context,ctm),sizeof(struct isp_ctm_param), &s_ctm_ops},
	{ISP_BLK_CCE_V1,array_offset(struct isp_context,cce_v1),sizeof(struct isp_cce_param_v1), &s_cce_ops_v1},
	{ISP_BLK_HSV,array_offset(struct isp_context,hsv),sizeof(struct isp_hsv_param), &s_hsv_ops},
	{ISP_BLK_RADIAL_CSC,array_offset(struct isp_context,csc),sizeof(struct isp_csc_param), &s_csc_ops},
	{ISP_BLK_RGB_PRECDN,  array_offset(struct isp_context,rgb_pre_cdn), sizeof(struct isp_rgb_pre_cdn_param), &s_rgb_precdn_ops_v1},
	{ISP_BLK_POSTERIZE,array_offset(struct isp_context,posterize),sizeof(struct isp_posterize_param), &s_posterize_ops},
	{ISP_BLK_AF_V1,array_offset(struct isp_context,rgb_afm_v1),sizeof(struct isp_rgb_afm_param_v1), &s_rgb_afm_ops_v1},
	//{ISP_BLK_RGB2Y, y_rgb2y},
	{ISP_BLK_YIQ_AEM,array_offset(struct isp_context,yiq_aem_v2),sizeof(struct isp_yiq_aem_param_v2), &s_yiq_aem_ops_v2},
	{ISP_BLK_YIQ_AFL,array_offset(struct isp_context,yiq_afl_v1),sizeof(struct isp_yiq_afl_param_v1),&s_yiq_afl_ops_v1},
	{ISP_BLK_YIQ_AFM, array_offset(struct isp_context,yiq_afm_v2),sizeof(struct isp_yiq_afm_param_v2),&s_yiq_afm_ops_v2},
	{ISP_BLK_YUV_PRECDN,  array_offset(struct isp_context,yuv_precdn),              sizeof(struct isp_yuv_precdn_param), &s_yuv_precdn_ops},
	{ISP_BLK_PREF_V1, array_offset(struct isp_context,prfy),sizeof(struct isp_prfy_param), &s_prfy_ops},
	//{ISP_BLK_UV_PREFILTER, yuv_prfuv},
	{ISP_BLK_BRIGHT, array_offset(struct isp_context, bright), sizeof(struct isp_bright_param), &s_bright_ops},
	{ISP_BLK_CONTRAST, array_offset(struct isp_context, contrast), sizeof(struct isp_contrast_param), &s_contrast_ops},
	{ISP_BLK_HIST_V1, array_offset(struct isp_context,hist_v1),sizeof(struct isp_hist_param_v1), &s_hist_ops_v1},
	{ISP_BLK_HIST2, array_offset(struct isp_context,hist2_v1),sizeof(struct isp_hist2_param_v1), &s_hist2_ops_v1},
	{ISP_BLK_AUTO_CONTRAST_V1, array_offset(struct isp_context,auto_contrast_v1),sizeof(struct isp_auto_contrast_param_v1), &s_auto_contrast_ops_v1},
	{ISP_BLK_UV_CDN, array_offset(struct isp_context,uv_cdn_v1), sizeof(struct isp_uv_cdn_param_v1), &s_uv_cdn_ops_v1},
	{ISP_BLK_EDGE_V1, array_offset(struct isp_context,edge_v1),sizeof(struct isp_edge_param_v1), &s_edge_ops_v1},
	{ISP_BLK_CSS_V1,array_offset(struct isp_context, css_v1),sizeof(struct isp_css_param_v1), &s_css_ops_v1},
	{ISP_BLK_SATURATION_V1,array_offset(struct isp_context,saturation_v1),sizeof(struct isp_chrom_saturation_param), &s_saturation_ops_v1},
	{ISP_BLK_HUE_V1,array_offset(struct isp_context,hue_v1),sizeof(struct isp_hue_param_v1), &s_hue_ops_v1},
	{ISP_BLK_UV_POSTCDN, array_offset(struct isp_context,uv_postcdn), sizeof(struct isp_uv_postcdn_param), &s_uv_postcdn_ops},
	{ISP_BLK_EMBOSS_V1,array_offset(struct isp_context,emboss_v1),sizeof(struct isp_emboss_param_v1),&s_emboss_ops_v1},
	{ISP_BLK_Y_GAMMC,array_offset(struct isp_context,yuv_ygamma),sizeof(struct isp_yuv_ygamma_param),&s_yuv_ygamma_ops},
	{ISP_BLK_YDELAY,array_offset(struct isp_context,ydelay),sizeof(struct isp_ydelay_param),&s_ydelay_ops},
	{ISP_BLK_UVDIV_V1, array_offset(struct isp_context, uv_div_v1), sizeof(struct isp_cce_uvdiv_param_v1), &s_uvdiv_ops_v1},
	//{ISP_BLK_YUV_NLM, yuv_nlm_block}
	{ISP_BLK_SMART, array_offset(struct isp_context, smart), sizeof(struct isp_smart_param), &s_smart_ops},
#endif
};

struct isp_block_cfg* isp_pm_get_block_cfg(isp_u32 id)
{
	isp_u32 num = 0;
	isp_u32 i = 0;
	isp_u32 blk_id = 0;
	struct isp_block_cfg *blk_cfg_ptr = PNULL;
	struct isp_block_cfg *blk_cfg_array = s_blk_cfgs;

	num = sizeof (s_blk_cfgs) / sizeof(struct isp_block_cfg);

	for (i = 0; i < num; ++i) {
		blk_id = blk_cfg_array[i].id;
		if (blk_id == id) {
			break;
		}
	}

	if (i <num) {
		blk_cfg_ptr  = &blk_cfg_array[i];
	} else {
		blk_cfg_ptr = PNULL;
	}

	return blk_cfg_ptr;
}

