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

#ifndef _ISP_AF_ALG_V03_H_
#define _ISP_AF_ALG_V03_H_
/*------------------------------------------------------------------------------*
*					Dependencies				*
*-------------------------------------------------------------------------------*/
#include <linux/types.h>

/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/
#ifdef  __cplusplus
extern "C"
{
#endif

#define AF3_MAX_ISP_ID 2
#define AF3_MAX_FINE_STEP_NUM 9
#define AF3_MAX_SCAN_STEP_CNT 100
#define AF3_MAX_AF_WIN_NUM 9


enum isp_af_status_v03{
	ISP_AF3_START_TRG,
	ISP_AF3_RUNING,
	ISP_AF3_END_SUCCES,
	ISP_AF3_END_FAIL,
	ISP_AF3_QUIT,
	ISP_AF3_STATUS_MAX,
};

enum isp_af_mode_v03{
	ISP_AF3_NORMAL,
	ISP_AF3_MACRO,
	ISP_AF3_CONTINUE,
	ISP_AF3_VIDEO,
	ISP_AF3_MODE_MAX,
};

enum isp_af_win_sel_mode_v03{
	ISP_AF3_SEL_WEIGHT = 0,
	ISP_AF3_SEL_MACRO,
	ISP_AF3_SEL_INF,
	ISP_AF3_SEL_MAX,
};



struct isp_af_config_v03{
	uint32_t MAX_STEP;
	uint32_t INIT_POS;
	uint32_t INIT_POS_MACRO;
	uint32_t STEP_LENGTH;
	uint32_t STEP_LENGTH_MACRO;
	uint32_t END_POS;
	uint32_t END_POS_MACRO;
	uint32_t MAX_TUNE_STEP;
	uint32_t FINE_STEP_NUM;
	uint32_t PEAK_VALUE_THR;
	uint32_t PEAK_VALUE_THR_1;
	uint32_t PEAK_VALUE_THR_2;
	uint32_t DECTECT_AFM_THR;
	uint32_t DECTECT_AWBM_THR;
	uint32_t DECTECT_STEP_NUM;
	uint32_t STEP_TBL[32];
	uint32_t STEP_TBL_COUNT;
	uint32_t START_AREA_RANGE;
	uint32_t END_AREA_RANGE;
	uint32_t NOISE_THR;
	uint32_t DEBUG;
	uint32_t ANTI_CRASH_POS;
};

struct af_para_struct_v03{
	uint32_t focus_pos;
	uint32_t cur_pos;
	uint32_t status;
	uint32_t af_win_num;
	uint32_t suc_win;
	uint32_t master_win;
	uint32_t af_mode;
	uint32_t motor_dir;   //0:increase    1:decrease
	uint32_t max_focus_pos;
	uint32_t max_focus_value;
	uint32_t step_count;
	uint32_t detect_step;
	uint32_t detect_start_step;
	uint32_t detect_pos_tbl[10];
	uint32_t coarse_step;
	uint32_t coarse_start_step;
	uint32_t g_peak_candidate[AF3_MAX_FINE_STEP_NUM][2];
	uint32_t max_value[AF3_MAX_AF_WIN_NUM];
	uint32_t max_pos[AF3_MAX_AF_WIN_NUM];
	uint32_t max_step[AF3_MAX_AF_WIN_NUM];
	uint32_t min_value[AF3_MAX_AF_WIN_NUM];
	uint32_t min_pos[AF3_MAX_AF_WIN_NUM];
	uint32_t min_step[AF3_MAX_AF_WIN_NUM];
	uint32_t INIT_POS;
	uint32_t END_POS;
	uint32_t STEP_LENGTH;
	uint32_t tune_period;
	uint32_t af_value[AF3_MAX_AF_WIN_NUM][AF3_MAX_SCAN_STEP_CNT];
	uint32_t af_pos[AF3_MAX_SCAN_STEP_CNT];
	uint32_t peak_af_win_num;
	uint32_t peak_mark[AF3_MAX_AF_WIN_NUM];
	uint32_t peak_fall_ratio[AF3_MAX_AF_WIN_NUM];
	uint32_t MACRO_POS;
	uint32_t fine_step;
	uint32_t fine_start_step;
	uint32_t fine_step_total;
	uint32_t fine_pos_tbl[10];
	uint32_t fine_max_pos;
	uint32_t fine_max_value;
	uint32_t fine_min_pos;
	uint32_t fine_min_value;
	uint32_t af_in_macro_pos;
	uint32_t awbm_af_value_r[AF3_MAX_SCAN_STEP_CNT];
	uint32_t awbm_af_value_g[AF3_MAX_SCAN_STEP_CNT];
	uint32_t awbm_af_value_b[AF3_MAX_SCAN_STEP_CNT];
	uint32_t awbm_af_value[AF3_MAX_SCAN_STEP_CNT];
	uint32_t awbm_af_max_value;
	uint32_t awbm_af_max_pos;
	uint32_t awbm_af_max_step;
	uint32_t awbm_af_min_value;
	uint32_t awbm_af_min_pos;
	uint32_t awbm_af_min_step;
	uint32_t env_complex_level;
	uint32_t env_complex_level_max;
	uint32_t env_complex_level_max_step;
	uint32_t PEAK_VALUE_THR;
	uint32_t PEAK_VALUE_THR_1;
	uint32_t PEAK_VALUE_THR_2;
	uint32_t NOISE_THR;

};

struct af_contex_struct_v03{
	struct isp_af_config_v03 af_cfg;
	struct af_para_struct_v03 af_para;
	uint32_t (*af_go_position)(uint32_t handler_id, uint32_t pos);

};


/*------------------------------------------------------------------------------*
*					Data Prototype				*
*-------------------------------------------------------------------------------*/
uint32_t isp_af_init_v03(uint32_t handler_id, struct af_contex_struct_v03 *input_para);
uint32_t isp_af_deinit_v03(uint32_t handler_id);
uint32_t isp_af3_calculation(
	uint32_t handler_id,
	uint32_t af_status,
	uint32_t *af_win_value,
	uint32_t af_win_num,
	uint16_t af_win_size[][4],
	uint32_t *awbm_value,
	uint32_t awbm_win_w,
	uint32_t awbm_win_h,
	uint8_t af_mode,
	uint32_t af_win_priority[AF3_MAX_AF_WIN_NUM],
	uint32_t af_win_sel_mode,
	uint32_t *rtn_suc_win,
	uint32_t cur_motor_pos);

/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/
#ifdef	 __cplusplus
}
#endif
/*-----------------------------------------------------------------------------*/
#endif
// End


