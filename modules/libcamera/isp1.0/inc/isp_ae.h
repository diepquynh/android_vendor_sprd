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

#ifndef _ISP_AE_H_
#define _ISP_AE_H_
/*----------------------------------------------------------------------------*
 **				Dependencies				*
 **---------------------------------------------------------------------------*/
#include <sys/types.h>
#include "isp_ae_alg_v00.h"
#include "isp_drv.h"
#include "isp_ev_calc.h"
/**---------------------------------------------------------------------------*
**				Compiler Flag				*
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif

/**---------------------------------------------------------------------------*
**				Micro Define				**
**----------------------------------------------------------------------------*/
#define ISP_AE_MAX_LINE 0xffffffff


#define ISP_AE_ISO_NUM 0x06
#define ISP_AE_TAB_NUM 0x04
#define ISP_AE_WEIGHT_TAB_NUM 0x04
#define ISP_AE_SKIP_FOREVER 0x7FFFFFFF
#define ISP_AE_SKIP_LOCK 0xFFFFFFFF

#define AE_EXP_GAIN_TABLE_SIZE 512
#define AE_FLICKER_NUM 2
#define AE_TAB_NUM 9
#define AE_TAB_CALC_MODE 4
/**---------------------------------------------------------------------------*
**				Data Structures 				*
**---------------------------------------------------------------------------*/
enum AE_TABLE_CALC_MODE
{
	AE_CALC_ISO_MODE=0,
	AE_CALC_FAST_MODE,
	AE_CALC_NIGHT_MODE,
	AE_CALC_VIDEO_MODE,
	AE_CALC_MAX=0xffffffff,
};

enum AE_TABLE_FLICKER {
	FLICKER_50HZ,
	FLICKER_60HZ,
	AE_FLICKER_MAX
};

enum AE_TABLE_MODE {
	ISO_AUTO,
	ISO_100,
	ISO_200,
	ISO_400,
	ISO_800,
	ISO_1600,
	FAST,
	NIGHT,
	VIDEO,
	AE_TABLE_MAX
};

enum isp_ae_tab_mode{
	ISP_NORMAL_50HZ=0x00,
	ISP_NORMAL_60HZ,
	ISP_NIGHT_50HZ,
	ISP_NIGHT_60HZ,
	ISP_AE_TAB_MAX
};

struct isp_resolution_info {
	uint16_t trim_start_x;
	uint16_t trim_start_y;
	uint16_t trim_width;
	uint16_t trim_height;
	uint32_t line_time;
	uint32_t pclk;
	uint32_t frame_line;
};

struct isp_cali_info {
	uint32_t r_sum;
	uint32_t b_sum;
	uint32_t gr_sum;
	uint32_t gb_sum;
};

struct isp_flashlight_cali_info {
	struct isp_cali_info rdm_cali;
	struct isp_cali_info gldn_cali;
};

struct isp_ae_tab{
	uint32_t* e_ptr;
	uint16_t* g_ptr;
	uint32_t start_index[ISP_AE_ISO_NUM];
	uint32_t max_index[ISP_AE_ISO_NUM];
};

struct isp_flash_param{
	//uint32_t eb;
	uint32_t ratio;
	uint32_t adjus_coef;
//	uint32_t prv_lum;
	uint32_t prv_index;
//	uint32_t cur_index;
	uint32_t next_index;
	uint32_t effect; // x/1024
	uint32_t r_ratio;
	uint32_t g_ratio;
	uint32_t b_ratio;
	uint32_t set_ae;
	uint32_t set_awb;
	uint32_t calc_finish;
};

struct isp_ae_information {
	uint32_t eb;
	uint32_t min_frm_rate;  //min frame rate
	uint32_t max_frm_rate;  //max frame rate
	uint32_t line_time;  //time of line
	uint32_t gain;
};

struct new_gamma_param{
	uint16_t x_node[16];
	uint16_t y_node[2][16];
	int16_t brightness_factor;
	int16_t contrast_factor;
};
struct isp_get_lum_param {
	uint32_t* stat_r_ptr;
	uint32_t* stat_g_ptr;
	uint32_t* stat_b_ptr;
	uint32_t* stat_y_ptr;
	uint8_t* weight_tab_ptr;

	uint32_t awb_r_gain;
	uint32_t awb_g_gain;
	uint32_t awb_b_gain;

	uint32_t monitor_w;
	uint32_t monitor_h;
	uint32_t is_rgb;
};
struct isp_gamma_param{
	uint32_t bypass;
	uint16_t axis[2][26];
	uint8_t index[28];
	struct new_gamma_param new_param;
};

struct isp_gamma_tab{
	uint16_t axis[2][26];
	struct new_gamma_param new_param;
};

struct isp_smart_denoise_out{
	uint16_t bil_dis_level;
	uint16_t bil_ran_level;
	uint16_t pref_y_level;
	uint16_t pref_uv_level;
	uint16_t y_level;
	uint16_t uv_level;
	uint16_t edge_level;
};

struct isp_ae_exp_param
{
	uint32_t index;
	uint32_t line;
	uint32_t dummy;
	uint32_t again;
	uint32_t dgain;
};

struct isp_ae_exp_info
{
	uint32_t num;
	struct isp_ae_exp_param tab[EXP_TAB_MAX];
};

struct isp_ae_exp_ctrl
{
	uint32_t eb;
	uint32_t conter;
	uint32_t valid_index;
	uint64_t system_time;
	struct isp_ae_exp_info exp;
};

struct ae_exp_gain {
	uint32_t exp;
	uint32_t dummy;
	uint32_t gain;
	uint16_t iso;
	uint16_t fps;
};

struct ae_tab_index {
	uint16_t start;
	uint16_t min;
	uint16_t prv_max;
	uint16_t cap_max;
	uint16_t reserved[16];
};

struct ae_max_gain {
	uint16_t again;
	uint16_t dgain;
};

struct ae_table_infor {
	struct ae_max_gain max;
	struct ae_tab_index index;
	struct ae_exp_gain tab[AE_EXP_GAIN_TABLE_SIZE];
};

struct ae_table {
	struct ae_table_infor ae[AE_FLICKER_NUM][AE_TAB_NUM];
};

struct isp_ae_param
{
	uint32_t bypass;
	uint32_t back_bypass;
	uint32_t monitor_bypass;
	uint32_t monitor_conter;
	uint32_t status;
	uint32_t init;
	uint32_t param_index;
	uint32_t alg_id;
	uint32_t anti_exposure;
	uint32_t fast_ae_get_stab;
	uint32_t ae_get_stab;
	uint32_t ae_get_stab_ext;
	uint32_t ae_get_change;
	uint32_t ae_bypass;
	uint32_t bypass_conter;
	uint32_t lock_conter;
	uint32_t lock_eb;
	uint32_t tab_mode;
	uint32_t mlog_eb;

	//enum isp_alg_mode alg_mode;
	enum isp_ae_frame_mode frame_mode;
	enum isp_ae_tab_mode tab_type;
	enum isp_iso real_iso;
	enum isp_iso back_iso;
	enum isp_iso cur_iso;
	enum isp_iso iso;
	enum isp_ae_weight weight;
	enum isp_flicker_mode flicker;
	enum isp_ae_mode mode;
	struct isp_size monitor;
	uint32_t cur_denoise_level;
	uint32_t cur_denoise_diswei_level;
	uint32_t cur_denoise_ranwei_level;
	uint32_t skip_frame;
	uint32_t cur_skip_num;
	uint8_t video_fps;
	uint8_t normal_fps;
	uint8_t night_fps;
	uint8_t sport_fps;
	uint32_t fix_fps;
	uint32_t max_fps;

	int32_t max_index;
	int32_t min_index;
	int32_t cur_index;

	uint32_t cur_dgain;
	uint32_t cur_gain;
	uint32_t cur_exposure;
	uint32_t cur_dummy;
	uint32_t exposure_skip_conter;
	uint32_t again_skip;
	uint32_t again_skip_conter;
	uint32_t dgain_skip;
	uint32_t dgain_skip_conter;

	uint32_t cur_lum;
	uint32_t target_lum;
	uint32_t target_zone;
	uint32_t quick_mode;
	uint32_t min_exposure;
	uint32_t smart; // bit0: denoise bit1: edge bit2: startion
	uint32_t smart_mode; // 0: gain 1: lum
	uint32_t smart_rotio;
	uint32_t smart_base_gain;
	uint16_t smart_wave_min;
	uint16_t smart_wave_max;
	uint16_t smart_pref_min;
	uint16_t smart_pref_max;
	uint16_t smart_pref_y_outdoor;
	uint16_t smart_pref_y_min;
	uint16_t smart_pref_y_mid;
	uint16_t smart_pref_y_max;
	uint16_t smart_pref_uv_outdoor;
	uint16_t smart_pref_uv_min;
	uint16_t smart_pref_uv_mid;
	uint16_t smart_pref_uv_max;
	uint8_t smart_denoise_min_index;
	uint8_t smart_denoise_mid_index;
	uint8_t smart_denoise_max_index;
	uint8_t smart_denoise_diswei_outdoor_index;
	uint8_t smart_denoise_diswei_min_index;
	uint8_t smart_denoise_diswei_mid_index;
	uint8_t smart_denoise_diswei_max_index;
	uint8_t smart_denoise_ranwei_outdoor_index;
	uint8_t smart_denoise_ranwei_min_index;
	uint8_t smart_denoise_ranwei_mid_index;
	uint8_t smart_denoise_ranwei_max_index;

	uint8_t smart_denoise_soft_y_outdoor_index;
	uint8_t smart_denoise_soft_y_min_index;
	uint8_t smart_denoise_soft_y_mid_index;
	uint8_t smart_denoise_soft_y_max_index;
	uint8_t smart_denoise_soft_uv_outdoor_index;
	uint8_t smart_denoise_soft_uv_min_index;
	uint8_t smart_denoise_soft_uv_mid_index;
	uint8_t smart_denoise_soft_uv_max_index;

	uint8_t denoise_lum_thr;
	uint16_t denoise_start_index;
	uint16_t denoise_start_zone;
	uint8_t smart_edge_max_index;
	uint8_t smart_edge_min_index;
	uint8_t reserved1;
	uint16_t smart_sta_precent;
	uint16_t smart_sta_start_index;
	uint16_t lum_cali_index;
	uint32_t lum_cali_lux;
	uint8_t smart_sta_low_thr;
	uint8_t smart_sta_ratio1;
	uint8_t smart_sta_ratio;
	uint8_t gamma_num;
	uint16_t gamma_zone;
	uint16_t gamma_thr[4];
	uint8_t gamma_lum_thr;

	uint32_t stab_conter;
	uint32_t stab;
	uint32_t auto_eb;
	int32_t ev;
	uint8_t* weight_ptr[ISP_AE_WEIGHT_TAB_NUM];
	uint8_t weight_tab[2][1024];
	uint8_t weight_id;
	struct isp_ae_tab tab[ISP_AE_TAB_NUM];
	struct isp_ae_tab cur_tab;
	uint32_t ae_set_eb;
	struct ae_table ae_tab_info;
	struct ae_ev_cali ev_cali;

	//struct isp_smart_denoise_out noise_info;
	struct isp_smart_denoise_out prv_noise_info;

	// ae frame infor for calc
	uint32_t* stat_r_ptr;
	uint32_t* stat_g_ptr;
	uint32_t* stat_b_ptr;
	uint32_t* stat_y_ptr;

	int32_t calc_cur_index;
	int32_t calc_max_index;
	int32_t calc_min_index;
	struct ae_table_infor* cur_e_ptr;
	uint16_t* cur_g_ptr;
	uint8_t* cur_weight_ptr;

	uint32_t line_time;
	uint32_t frame_line;
	uint32_t frame_info_eb;
	uint32_t weight_eb;

	struct isp_flash_param flash;
	struct isp_ae_information ae_info;

	struct isp_ae_v00_context alg_v00_context;

	struct isp_flashlight_cali_info flash_cali;

	struct sensor_ae_change_speed *speed_dark_to_bright;
	uint32_t step_dark_to_bright;
	struct sensor_ae_change_speed *speed_bright_to_dark;
	uint32_t step_bright_to_dark;

	struct sensor_ae_histogram_segment *histogram_segment;
	uint32_t	histogram_segment_num;

	struct isp_ae_exp_ctrl write_ctrl;

	uint32_t(*write_exposure) (uint32_t param);
	uint32_t(*write_gain) (uint32_t param);
	uint32_t(*write_dgain) (uint32_t param);

	int32_t (*adjust_cmc)(uint32_t handler_id, uint32_t percent, uint32_t max_index, uint32_t cur_index);
	int32_t (*adjust_denoise)(uint32_t handler_id, uint32_t percent, uint32_t denoise_level, uint16_t denoise_max, uint16_t denoise_min, uint16_t wave_max, uint16_t wave_min, uint16_t pref_max, uint16_t pref_min);
	int32_t (*adjust_edge)(uint32_t handler_id, uint32_t edge_index);
	int32_t (*adjust_lnc)(uint32_t handler_id, uint32_t cur_index, uint32_t max_index,uint32_t lnc_smart);
	int32_t (*flash_effect)(uint32_t prv_lum, uint32_t final_lum,uint32_t* effect);

	int32_t (*set_gamma)(struct isp_gamma_param* gamma_param, struct isp_gamma_tab* tab_ptr);
	int32_t (*awbm_skip)(uint32_t handler_id, uint8_t num);
	int32_t (*awbm_bypass)(uint32_t handler_id, uint8_t bypass);

	uint32_t(*get_gain_scale) (uint32_t scale);
	uint32_t(*get_capture_gain) (uint32_t gain);

	proc_callback callback;
	proc_callback self_callback;
	uint8_t edge_preview_percent;
	uint8_t denoise_preview_percent;
	uint8_t is_single; //for  capture
	struct isp_range_fps range_fps;
	int32_t is_save_index;
	int32_t back_max_index;
	int32_t back_max_index_flag;
	int32_t ae_set_cb_ext;

	uint32_t min_frame_line;
	uint32_t max_frame_line;
	uint32_t ae_skip_calc_num;
	uint32_t capture_gain;
	uint32_t(*ex_write_exposure) (unsigned long param);
};


/**---------------------------------------------------------------------------*
**					Data Prototype				**
**----------------------------------------------------------------------------*/
struct isp_ae_param* ispGetAeContext(uint32_t handler_id);
int ispSetAeContext(uint32_t handler_id, void *cxt);
uint32_t _ispGetLineTime(struct isp_resolution_info* resolution_info_ptr, uint32_t param_index);
uint32_t _ispGetFrameLine(uint32_t handler_id, uint32_t line_time, uint32_t fps);
uint32_t _ispSetAeTabMaxIndex(uint32_t handler_id, uint32_t mode, uint32_t iso,uint32_t fps);
uint32_t _ispAeInfoSet(uint32_t handler_id);
int32_t _ispGetAeIndexIsMax(uint32_t handler_id, uint32_t* eb);
int32_t isp_GetAEVersion(void);

/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif
// End

