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

#ifndef _ISP_AF_ALG_V04_H_
#define _ISP_AF_ALG_V04_H_
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

#define AF4_MAX_ISP_ID 2
#define AF4_ROUGH_TBL_LEN 32
#define AF4_FINE_TBL_LEN 32
#define AF4_MAX_ROUGH_STEP_NUM AF4_ROUGH_TBL_LEN
#define AF4_MAX_FINE_STEP_NUM 9
#define AF4_MAX_SCAN_STEP_CNT 100
#define AF4_MAX_AF_WIN_NUM 9

#define AF4_TRUE 1
#define AF4_FALSE 0

#define AF4_SUCCESS 0
#define AF4_FAIL -1


#define AF4_ABS(a) ((a) > 0 ? (a) : -(a))

enum isp_af_status_v04{
	ISP_AF4_START_TRG,
	ISP_AF4_RUNING,
	ISP_AF4_END_SUCCES,
	ISP_AF4_END_FAIL,
	ISP_AF4_QUIT,
	ISP_AF4_STATUS_MAX,
};

enum isp_af_mode_v04{
	ISP_AF4_NORMAL,
	ISP_AF4_MACRO,
	ISP_AF4_CONTINUE,
	ISP_AF4_VIDEO,
	ISP_AF4_MODE_MAX,
};

enum isp_af_win_sel_mode_v04{
	ISP_AF4_SEL_WEIGHT = 0,
	ISP_AF4_SEL_MACRO,
	ISP_AF4_SEL_INF,
	ISP_AF4_SEL_MAX,
};

enum isp_af_tune_period_v04{
	ISP_AF_DETECT = 0,
	ISP_AF_ROUGH,
	ISP_AF_FINE,
	ISP_AF_END,
	ISP_AF_PERIOD_MAX,
};

enum isp_af_motor_dir_v04{
	MOTOR_FORWARD = 0,
	MOTOR_BACKWARD
};


enum isp_af_peak_mark_type_v04{
	PEAK_NO_FOUND = 0,
	PEAK_CENTER,
	PEAK_SIDE
};

enum isp_af_turn_point_type_v04{
	TYPE_PEAK = 0,
	TYPE_VALLEY
};


struct isp_af_config_v04{
	uint32_t max_pos;
	uint32_t init_pos;
	uint32_t init_pos_macro;
	uint32_t step_len;
	uint32_t step_len_macro;
	uint32_t end_pos;
	uint32_t end_pos_macro;
	uint32_t max_tune_step;
	uint32_t fine_step_num;
	uint32_t peak_value_thr;
	uint32_t peak_value_thr_1;
	uint32_t peak_value_thr_2;
	uint32_t detect_afm_thr;
	uint32_t detect_awbm_thr;
	uint32_t detect_step_num;
	uint32_t rough_step_tbl[AF4_ROUGH_TBL_LEN];
	uint32_t fine_step_tbl[AF4_FINE_TBL_LEN];
	uint32_t rough_tbl_cnt;
	uint32_t start_area_range;
	uint32_t end_area_range;
	uint32_t noise_thr;
	uint32_t video_max_tune_step;
	uint32_t video_speed_ratio;
	uint32_t anti_crash_pos;
	uint32_t debug;
};


struct common_ctrl_info_v04{

};

struct detect_scan_info_v04{

};

struct rough_scan_info_v04{

};

struct fine_scan_info_v04{

};

struct turn_point_info_v04{
	uint32_t type;
	uint32_t pos;
	uint32_t step;
	uint32_t value;
	uint32_t p2v_ratio_left;
	uint32_t p2v_ratio_right;
};


struct peak_info_v04{
	uint32_t peak_area_start;
	uint32_t peak_area_end;
	uint32_t peak_ex_area_len;
	uint32_t peak_found;
	uint32_t peak_type;
	uint32_t peak_cnt;
	uint32_t turn_point_cnt;
	uint32_t bottom_valley_id;
	uint32_t last_peak_id;
	uint32_t top_peak_id;
	uint32_t top_peak_step;
	uint32_t top_peak_pos;
	uint32_t top_peak_value;
	uint32_t tp2v_ratio_left;
	uint32_t tp2v_ratio_right;
	uint32_t max2min_ratio;
};


struct af_para_struct_v04{
	uint32_t init_pos;
	uint32_t end_pos;
	uint32_t step_len;
	uint32_t tune_period;
	uint32_t focus_pos;
	uint32_t suc_pos;
	uint32_t cur_pos;
	uint32_t cur_step;
	uint32_t status;
	uint32_t af_value[AF4_MAX_AF_WIN_NUM][AF4_MAX_SCAN_STEP_CNT];
	uint32_t af_pos[AF4_MAX_SCAN_STEP_CNT];
	uint32_t af_win_num;
	uint32_t suc_win;
	uint32_t master_win;
	uint32_t af_mode;
	uint32_t motor_dir;   //0:increase    1:decrease
	uint32_t detect_step;
	uint32_t detect_start_step;
	uint32_t detect_pos_tbl[10];
	uint32_t detect_step_len;
	uint32_t rough_step;
	uint32_t rough_start_step;
	uint32_t rough_max_value[AF4_MAX_AF_WIN_NUM];
	uint32_t rough_max_pos[AF4_MAX_AF_WIN_NUM];
	uint32_t rough_max_step[AF4_MAX_AF_WIN_NUM];
	uint32_t rough_min_value[AF4_MAX_AF_WIN_NUM];
	uint32_t rough_min_pos[AF4_MAX_AF_WIN_NUM];
	uint32_t rough_min_step[AF4_MAX_AF_WIN_NUM];
	uint32_t peak_af_win_num;
	uint32_t peak_mark[AF4_MAX_AF_WIN_NUM];
	uint32_t peak_fall_ratio[AF4_MAX_AF_WIN_NUM];
	uint32_t fine_step;
	uint32_t fine_start_step;
	uint32_t fine_step_total;
	uint32_t fine_pos_tbl[10];
	uint32_t fine_max_pos;
	uint32_t fine_max_value;
	uint32_t fine_max_step;
	uint32_t fine_min_pos;
	uint32_t fine_min_value;
	uint32_t env_complex_level;
	uint32_t win_priority[AF4_MAX_AF_WIN_NUM];
	uint32_t win_sel_mode;
	struct peak_info_v04 peak_info[AF4_MAX_AF_WIN_NUM];
	struct turn_point_info_v04 turn_point_info[AF4_MAX_AF_WIN_NUM][AF4_MAX_ROUGH_STEP_NUM];
};


struct af_default_cfg_struct_v04{
	uint32_t detect_init_len;   //percent to rough step
	uint32_t detect_increase_ratio;   //percent base
	uint32_t detect_max_len;   //percent to rough step
	uint32_t skip_frame_thr;   //percent to rough step
};



struct af_contex_struct_v04{
	struct isp_af_config_v04 af_cfg;
	struct af_default_cfg_struct_v04 af_default_cfg;
	struct af_para_struct_v04 af_para;
	uint32_t (*af_go_position)(uint32_t handler_id, uint32_t pos);

};


struct af_afm_info_struct_v04{
	uint32_t *af_win_value;
	uint32_t af_win_num;
	uint16_t (*af_win_size)[4];
};

struct af_awbm_info_struct_v04{
	uint32_t *awbm_value;
	uint32_t awbm_win_w;
	uint32_t awbm_win_h;
	uint32_t awbm_win_x;
	uint32_t awbm_win_y;
};

struct af_ae_info_struct_v04{
	uint32_t exp_time;  //us
	uint32_t max_fps;
};

struct af_ctrl_info_struct_v04{
	uint32_t af_status;
	uint8_t af_mode;
	uint32_t *af_win_priority;
	uint32_t af_win_sel_mode;
	uint32_t cur_motor_pos;
};

struct af_input_param_struct_v04{
	struct af_ctrl_info_struct_v04 af_ctrl;
	struct af_afm_info_struct_v04 afm_info;
	struct af_awbm_info_struct_v04 awbm_info;
	struct af_ae_info_struct_v04 ae_info;
};

struct af_output_param_struct_v04{
	uint32_t new_position;
	uint32_t rtn_suc_win;
	uint32_t *skip_num;
};


/*------------------------------------------------------------------------------*
*					Data Prototype				*
*-------------------------------------------------------------------------------*/
int32_t isp_af_init_v04(uint32_t handler_id, struct af_contex_struct_v04 *input_para);
int32_t isp_af_deinit_v04(uint32_t handler_id);
uint32_t isp_af4_calculation(
	uint32_t handler_id,
	struct af_input_param_struct_v04 *in_param,
	struct af_output_param_struct_v04 *out_param);

/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/
#ifdef	 __cplusplus
}
#endif
/*-----------------------------------------------------------------------------*/
#endif
// End


