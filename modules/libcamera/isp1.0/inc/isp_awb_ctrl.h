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
#ifndef _ISP_AWB_CTRL_H_
#define _ISP_AWB_CTRL_H_
/*------------------------------------------------------------------------------*
*				Dependencies					*
*-------------------------------------------------------------------------------*/
#include <linux/types.h>
#include "isp_awb.h"
#include "isp_smart_light.h"
#include "sensor_raw.h"
/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif
/*------------------------------------------------------------------------------*
*				Micro Define					*
*-------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------*
*				Data Structures					*
*-------------------------------------------------------------------------------*/
enum isp_awb_set_cmd {
	ISP_AWB_SET_QUICK_MODE = 1,
};

enum isp_awb_wditht{
	ISP_AWB_WDITHT_AVG=0x00,
	ISP_AWB_WDITHT_CENTER,
	ISP_AWB_WDITHT_CUSTOMER,
	ISP_AWB_WDITHT_MAX
};

struct isp_awb_cali_info {
	uint32_t r_sum;
	uint32_t b_sum;
	uint32_t gr_sum;
	uint32_t gb_sum;
};

struct isp_awb_rgb{
	uint16_t r;
	uint16_t g;
	uint16_t b;
};

struct isp_awb_estable{
	uint32_t valid;
	uint32_t invalid;
	struct isp_awb_rgb valid_rgb[1024];
	struct isp_awb_rgb invalid_rgb[1024];
};

struct isp_awb_adjust {
	uint32_t index0;
	uint32_t index1;
	uint32_t alpha; // index1 alpha----1024->1X
	uint32_t dec_ratio;
};

struct isp_awb_param{

	uint32_t bypass;
	uint32_t back_bypass;
	uint32_t monitor_bypass;
	uint32_t init;
	uint32_t alg_id;
	uint16_t base_gain;
	struct isp_size stat_img_size;
	struct isp_size win_size;
	struct isp_pos back_monitor_pos;
	struct isp_size back_monitor_size;
	enum isp_alg_mode alg_mode;
	enum isp_awb_mode mode;
	enum isp_awb_wditht weight;
	uint8_t* weight_ptr[ISP_AWB_WEIGHT_TAB_NUM];
	struct isp_awb_coord win[ISP_AWB_TEMPERATRE_NUM];
	uint32_t steady_speed;
	struct isp_awb_cali_info cali_info;
	struct isp_awb_cali_info golden_info;
	uint8_t weight_tab[2][1024];
	uint8_t weight_id;
	uint8_t target_zone;
	uint8_t cur_index;
	uint8_t prv_index;
	uint8_t gain_index;
	uint8_t matrix_index;
	uint32_t valid_block;
	uint32_t stab_conter;
	struct isp_awb_estable east;
	struct isp_awb_rgb cur_rgb;
	struct isp_awb_gain target_gain;
	struct isp_awb_gain cur_gain;
	struct isp_awb_gain prv_gain;
	struct isp_awb_rgb gain_convert[8];
	uint32_t gain_div;
	uint32_t cur_ct;
	uint32_t set_eb;
	uint32_t quick_mode;
	uint32_t smart;
	uint32_t cur_setting_index;
	struct isp_awb_map map_data;
	uint32_t debug_level;
	struct isp_awb_weight_of_count_func weight_of_count_func;
	struct isp_awb_weight_of_ct_func weight_of_ct_func[ISP_AWB_ENVI_NUM];
	struct isp_awb_weight_lut weight_of_pos_lut;
	struct isp_awb_ct_info ct_info;
	struct isp_awb_range value_range[ISP_AWB_ENVI_NUM];
	struct isp_awb_gain init_gain;
	uint16_t init_ct;
	struct isp_awb_ref_param ref_param[ISP_AWB_ENVI_NUM];
	struct isp_awb_statistic_info *awb_stat;
	struct isp_awb_init_param init_param;
	struct isp_awb_calc_param calc_param;
	struct isp_awb_calc_result calc_result;
	uint32_t bright_value;
	enum  isp_awb_envi_id envi_id[2];
	uint32_t envi_weight[2];
	uint32_t work_mode;//auto white balance or manual white balance
	uint32_t flash_awb_flag;/*in flash mode, awb must use quick mode*/
	uint32_t green_factor;
	uint32_t skin_factor;
	uint32_t debug_file;
	uint32_t quick_mode_enable;
	uint32_t(*continue_focus_stat) (uint32_t handler_id, uint32_t param);
	uint32_t(*mointor_info) (uint32_t handler_id, void* param_ptr);
	uint32_t(*set_monitor_win) (struct isp_pos pos, struct isp_size win_size);
	uint32_t(*recover_monitor_wn) (void* param_ptr);
	int32_t(*set_saturation_offset) (uint32_t handler_id, uint8_t offset);
	int32_t(*set_hue_offset) (uint32_t handler_id, int16_t offset);
	int32_t(*get_ev_lux) (uint32_t handler_id);
	uint32_t (*GetDefaultGain)(uint32_t handler_id);
	int (*change_param)(uint32_t handler_id, uint32_t cmd, void *param);
};

struct isp_smart_light_param {

	uint32_t init;
	uint32_t smart;
	struct smart_light_init_param init_param;
	struct smart_light_calc_param calc_param;
	struct smart_light_calc_result calc_result;
};

/*------------------------------------------------------------------------------*
*				Data Prototype					*
*-------------------------------------------------------------------------------*/

uint32_t isp_awb_ctrl_init(uint32_t handler_id);
uint32_t isp_awb_ctrl_deinit(uint32_t handler_id);
uint32_t isp_awb_ctrl_calculation(uint32_t handler_id);
uint32_t isp_awb_set_flash_gain(void);
uint32_t isp_awb_ctrl_set(uint32_t handler_id, uint32_t cmd, void *param0, void *param1);
/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/
#endif
// End


