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

#ifndef _ISP_AF_H_
#define _ISP_AF_H_
/*------------------------------------------------------------------------------*
*					Dependencies				*
*-------------------------------------------------------------------------------*/
#include <linux/types.h>
#include "isp_af_alg_v03.h"
#include "isp_af_alg_v04.h"
#include "isp_caf.h"

/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/
#ifdef  __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------*
*					Micro Define				*
*-------------------------------------------------------------------------------*/
#define ISP_AF_END_FLAG 0x80000000

/*------------------------------------------------------------------------------*
*					Data Structures				*
*-------------------------------------------------------------------------------*/

enum isp_af_status{
	ISP_AF_SUCCESS,
	ISP_AF_START,
	ISP_AF_FAIL,
	ISP_AF_CONTINUE,
	ISP_AF_FINISH,
	ISP_AF_STOP,
	ISP_AF_STATUS_MAX,
};

struct isp_awbm_param{
	uint32_t bypass;
	struct isp_pos win_start;
	struct isp_size win_size;
};


struct isp_af_param{
	uint32_t bypass;
	uint32_t back_bypass;
	uint32_t init;
	enum isp_focus_mode mode;
	uint32_t status;
	uint32_t continue_status;
	uint32_t continue_stat_flag;
	uint32_t monitor_bypass;
	uint32_t have_success_record;
	uint16_t win[9][4];
	uint32_t valid_win;
	uint32_t suc_win;
	uint32_t max_step;
	uint32_t min_step;
	uint32_t cur_step;
	uint32_t stab_period;
	uint32_t set_point;
	uint32_t ae_status;
	uint32_t awb_status;
	uint32_t alg_id;
	uint16_t max_tune_step;
	uint16_t af_rough_step[32];
	uint16_t af_fine_step[32];
	uint8_t rough_count;
	uint8_t fine_count;
	uint8_t awbm_flag;
	uint8_t afm_flag;
	uint32_t awbm_win_w;
	uint32_t awbm_win_h;
	int32_t (*CfgAwbm)(uint32_t handler_id, struct isp_awbm_param* param_ptr);
	int32_t (*AfmEb)(uint32_t handler_id, uint32_t skip_num);
	int32_t (*AwbmEb_immediately)(uint32_t handler_id);
	uint32_t(*continue_focus_stat) (uint32_t handler_id, uint32_t param);
	uint32_t(*go_position) (uint32_t param);
	uint32_t default_rough_step_len;
	uint32_t peak_thr_0;
	uint32_t peak_thr_1;
	uint32_t peak_thr_2;
	uint32_t detect_thr;
	uint32_t detect_step_mum;
	uint32_t start_area_range;
	uint32_t end_area_range;
	uint32_t noise_thr;
	uint32_t video_max_tune_step;
	uint32_t video_speed_ratio;
	uint32_t end_handler_flag;
	uint32_t debug;
	uint32_t win_priority[9];
	uint32_t win_sel_mode;
	uint32_t multi_win_enable;
	uint32_t multi_win_cnt;
	uint16_t multi_win_pos[9][4];
	uint32_t multi_win_priority[9];
	uint32_t start_time;
	uint32_t end_time;
	uint32_t step_cnt;
	uint32_t anti_crash_pos;
	uint32_t control_denoise;
	uint32_t denoise_lv;
	uint32_t frame_skip;
	uint32_t in_processing;
	uint16_t time[32];
	uint32_t af_value[9][32];
	uint32_t pos[32];
	struct af_contex_struct_v03 alg_v03_context;
	struct af_contex_struct_v04 alg_v04_context;
	struct isp_caf_cal_cfg ctn_af_cal_cfg[2];
};


/*------------------------------------------------------------------------------*
*					Data Prototype				*
*-------------------------------------------------------------------------------*/
uint32_t isp_af_init(uint32_t handler_id);
uint32_t isp_af_deinit(uint32_t handler_id);
uint32_t isp_af_calculation(uint32_t handler_id);
uint32_t isp_af_end(uint32_t handler_id, uint8_t stop_mode);
//uint32_t isp_continue_af_calc(uint32_t handler_id);
uint32_t isp_af_get_mode(uint32_t handler_id, uint32_t *mode);
uint32_t isp_af_set_mode(uint32_t handler_id, uint32_t mode);
uint32_t isp_af_set_postion(uint32_t handler_id, uint32_t step);
uint32_t isp_af_get_stat_value(uint32_t handler_id, void* param_ptr);
uint32_t isp_af_pos_reset(uint32_t handler_id, uint32_t mode);
/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/
#ifdef	 __cplusplus
}
#endif
/*-----------------------------------------------------------------------------*/
#endif
// End
