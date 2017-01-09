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
#ifndef _ISP_RAW_H_
#define _ISP_RAW_H_

//#include "jpeg_exif_header.h"
//#include "cmr_common.h"
#include <sys/types.h>
//#include "sensor_drv_u.h"

#define _ISP_MAX_AWB_MOD 8
#define _ISP_EV_LEVEL 15
#define _ISP_BRIGHTNESS_LEVEL 15
#define _ISP_CONTRAST_LEVEL 15
#define _ISP_HUE_LEVEL 15

#define ISP_AUTO_LEVEL 14

/*
//normal: conversion/emboss/gamma
//grey: color conversion;
//sepia: color conversion;
//emboss: emboss;
//negative: color conversion;
//over exposure: gamma;
//red: color conversion;
//green: color conversion
//blue: color conversion;
//yellow: color conversion;
*/
/* Incandescent/U30/CWF/Fluorescent/Sun/Cloudy*/
#define _ISP_ISO_NUM 0x06
#define _ISP_AE_NUM 0x500
#define _ISP_AE_TAB_NUM 0x04
#define _ISP_AWB_CALI_NUM 0x09
#define _ISP_AWB_NUM 0x14
#define _ISP_MAP_NUM 0x08
#define _ISP_AWB_G_ESTIMATE_NUM 0x6
#define _ISP_AWB_GAIN_ADJUST_NUM 0x9
#define _ISP_AWB_LIGHT_NUM 0x10

#define _ISP_AWB_HUE_NUM 0x7
#define _ISP_AWB_SAT_NUM 0x7

#define _ISP_RAW_INFO_END_ID 0x71717567

#define _ISP_RAW_VERSION_ID 0x00000000 /* tiger-0000xxxx, soft-xxxx0000*/
#define _ISP_RAW_V0001_VERSION_ID 0x00010000 /* tiger-0000xxxx, soft-xxxx0000*/

#define NUM_BEFORE_MODULE_INFO	3
#define ISP_CCE_BUF_NUM			16
#define ISP_EXT_RESERVER_NUM	256
#define VAR_OFFSET(type,member)	(uint32_t)(&((type *)0)->member)

typedef enum {
	ISP_BLC_ID = 0,
	ISP_NLC_ID,
	ISP_LNC_ID,
	ISP_AE_ID,
	ISP_AWB_ID,
	ISP_BPC_ID,
	ISP_DENOISE_ID,
	ISP_GRGB_ID,
	ISP_CFA_ID,
	ISP_CMC_ID,
	ISP_GAMMA_ID,
	ISP_UVDIV_ID,
	ISP_PREF_ID,
	ISP_BRIGHT_ID,
	ISP_CONTRAST_ID,
	ISP_HIST_ID,
	ISP_AUTO_CONTRAST_ID,
	ISP_SATURATION_ID,
	ISP_CSS_ID,
	ISP_AF_ID,
	ISP_EDGE_ID,
	ISP_EMBOSS_ID,
	ISP_GLB_GAIN_ID,
	ISP_CHN_GAIN_ID,
	ISP_FLASH_CALI_ID,
	ISP_CCE_BUF_ID,
	ISP_FCS_ID,
	ISP_HUE_ID,
	ISP_HDR_ID,
	ISP_EXT_ID,
	ISP_MAX_ID
} ISP_MODULE_ID;
/**----------------------------------------------------------------------------------*
**                                      v00010001 struct                                                          **
**-----------------------------------------------------------------------------------*/
struct _isp_pos{
	uint16_t x;
	uint16_t y;
};

struct _isp_size{
	uint16_t w;
	uint16_t h;
};

struct _isp_trim_size{
	uint16_t x;
	uint16_t y;
	uint16_t w;
	uint16_t h;
};

struct _isp_pos_rect{
	uint16_t start_x;
	uint16_t start_y;
	uint16_t end_x;
	uint16_t end_y;
};

struct _isp_rgb_gain{
	uint16_t r_gain;
	uint16_t g_gain;
	uint16_t b_gain;
};

struct _isp_blc_offset{
	uint16_t r;
	uint16_t gr;
	uint16_t gb;
	uint16_t b;
};

struct _isp_lnc_map_addr{
	uint32_t grid;
	uint16_t* param_addr;
	uint32_t len;
};

struct _isp_lnc_map{
	struct _isp_lnc_map_addr map[_ISP_MAP_NUM][_ISP_AWB_CALI_NUM];
};

struct _isp_awb_map{
	uint16_t *addr;
	uint32_t len;		//by bytes
};

struct _isp_ae_index{
	uint16_t start;
	uint16_t max;
};

struct _isp_ae_tab_info{
	uint32_t* d_ptr;
	uint32_t* e_ptr;
	uint32_t* g_ptr;
	struct _isp_ae_index index[_ISP_ISO_NUM];
};

struct _isp_ae_histogram_segment {
	uint8_t		min;
	uint8_t		max;
	uint16_t	weight;
};

struct _isp_ae_change_speed {
	uint16_t	delta_lum;
	uint16_t	delta_ev;
};

struct _isp_ae_tab{
	uint8_t* weight_tab;
	struct _isp_ae_tab_info tab[_ISP_AE_TAB_NUM];
};

struct _isp_rgb{
	uint16_t r;
	uint16_t g;
	uint16_t b;
};

struct _isp_awb_coord{
	uint16_t x;
	uint16_t yb;
	uint16_t yt;
};

struct _isp_cali_info {
	uint32_t r_sum;
	uint32_t gr_sum;
	uint32_t gb_sum;
	uint32_t b_sum;
};

struct _isp_awb_g_estimate_param
{
	uint16_t t_thr[_ISP_AWB_G_ESTIMATE_NUM];
	uint16_t g_thr[_ISP_AWB_G_ESTIMATE_NUM][2];
	uint16_t w_thr[_ISP_AWB_G_ESTIMATE_NUM][2];
	uint32_t num;
};

struct _isp_awb_hue_param
{
	uint16_t t_thr[_ISP_AWB_HUE_NUM];
	int16_t hue_thr[_ISP_AWB_HUE_NUM];
	uint32_t num;
};

struct _isp_awb_saturation_param
{
	uint16_t t_thr[_ISP_AWB_SAT_NUM];
	uint16_t sat_thr[_ISP_AWB_SAT_NUM];
	uint32_t num;
};

struct _isp_awb_linear_func
{
	int32_t a;
	int32_t b;
	uint32_t shift;
};

struct _isp_awb_wp_count_range
{
	uint16_t min_proportion;//min_proportion / 256
	uint16_t max_proportion;//max_proportion / 256
};

struct _isp_awb_gain_adjust
{
	uint16_t t_thr[_ISP_AWB_GAIN_ADJUST_NUM];
	uint16_t w_thr[_ISP_AWB_GAIN_ADJUST_NUM];
	uint32_t num;
};

struct _isp_awb_light_weight
{
	uint16_t t_thr[_ISP_AWB_LIGHT_NUM];
	uint16_t w_thr[_ISP_AWB_LIGHT_NUM];
	uint16_t num;
};

struct _isp_denoise_tab {
	uint8_t diswei[19];
	uint8_t reserved1;
	uint8_t ranwei[31];
	uint8_t reserved0;
};


struct _isp_gamma_tab{
	uint16_t axis[2][26];
};


struct _isp_edge_param{
	uint8_t detail_thr;
	uint8_t smooth_thr;
	uint8_t strength;
	uint8_t reserved;
};

struct _isp_version_info{
	uint32_t version_id;
	uint32_t srtuct_size;
	uint32_t reserve;
};

struct module_info {
	uint16_t module_id;
	uint32_t len;
	uint32_t offset;
};

struct isp_blc{
	uint32_t blc_bypass;
	uint8_t mode;
	uint8_t reserved2;
	uint8_t reserved1;
	uint8_t reserved0;
	struct _isp_blc_offset offset[8];
};

struct isp_nlc{
	uint32_t nlc_bypass;
	uint16_t r_node[29];
	uint16_t g_node[29];
	uint16_t b_node[29];
	uint16_t l_node[29];
};

struct isp_lnc{
	uint32_t lnc_bypass;
	uint32_t start_index;
};

struct isp_ae{
	uint32_t ae_bypass;
	uint8_t skip_frame;
	uint8_t normal_fix_fps;
	uint8_t night_fix_fps;
	uint8_t video_fps;
	uint8_t target_lum;
	uint8_t target_zone;
	uint8_t smart;
	uint8_t smart_rotio;
	uint8_t quick_mode;
	uint8_t min_exposure;
	int8_t ev[16];
	uint16_t smart_base_gain;
	uint16_t smart_wave_min;
	uint16_t smart_wave_max;
	uint8_t smart_pref_min;
	uint8_t smart_pref_max;
	uint8_t smart_denoise_min_index;
	uint8_t smart_denoise_max_index;
	uint8_t smart_edge_min_index;
	uint8_t smart_edge_max_index;
	uint8_t smart_mode;
	uint8_t smart_sta_low_thr;
	uint8_t smart_sta_ratio1;
	uint8_t smart_sta_ratio;
	uint16_t smart_sta_start_index;
	uint8_t again_skip;
	uint8_t dgain_skip;
	uint8_t gamma_start;
	uint8_t gamma_num;
	uint8_t gamma_zone;
	uint8_t gamma_thr[4];
	uint8_t gamma_lum_thr;
	uint16_t lum_cali_index;
	uint32_t lum_cali_lux;

	uint8_t smart_denoise_mid_index;
	uint8_t denoise_start_index;
	uint8_t denoise_lum_thr;

	uint8_t smart_pref_y_outdoor;
	uint8_t smart_pref_y_min;
	uint8_t smart_pref_y_mid;
	uint8_t smart_pref_y_max;
	uint8_t smart_pref_uv_outdoor;
	uint8_t smart_pref_uv_min;
	uint8_t smart_pref_uv_mid;
	uint8_t smart_pref_uv_max;
	uint8_t smart_denoise_diswei_outdoor_index;
	uint8_t smart_denoise_diswei_min_index;
	uint8_t smart_denoise_diswei_mid_index;
	uint8_t smart_denoise_diswei_max_index;
	uint8_t smart_denoise_ranwei_outdoor_index;
	uint8_t smart_denoise_ranwei_min_index;
	uint8_t smart_denoise_ranwei_mid_index;
	uint8_t smart_denoise_ranwei_max_index;
	uint8_t denoise_start_zone;

	uint8_t smart_denoise_soft_y_outdoor_index;
	uint8_t smart_denoise_soft_y_min_index;
	uint8_t smart_denoise_soft_y_mid_index;
	uint8_t smart_denoise_soft_y_max_index;
	uint8_t smart_denoise_soft_uv_outdoor_index;
	uint8_t smart_denoise_soft_uv_min_index;
	uint8_t smart_denoise_soft_uv_mid_index;
	uint8_t smart_denoise_soft_uv_max_index;

};

struct isp_awb{
	uint32_t awb_bypass;
	struct _isp_pos win_start;
	struct _isp_size win_size;
	struct _isp_rgb gain_convert[8];
	uint32_t win_x[_ISP_AWB_NUM];
	uint32_t win_yb[_ISP_AWB_NUM];
	uint32_t win_yt[_ISP_AWB_NUM];
	struct _isp_awb_light_weight light;
	uint32_t steady_speed;
	uint16_t r_gain[_ISP_AWB_NUM];
	uint16_t g_gain[_ISP_AWB_NUM];
	uint16_t b_gain[_ISP_AWB_NUM];
	uint8_t gain_index;
	uint8_t alg_id;
	uint32_t target_zone;
	uint32_t smart;
	uint32_t quick_mode;
	struct _isp_awb_wp_count_range wp_count_range;
	struct _isp_awb_saturation_param sat_adjust;
	struct _isp_awb_hue_param hue_adjust;
	struct _isp_awb_linear_func t_func;
	struct _isp_awb_gain_adjust gain_adjust;
	uint8_t debug_level;
	uint8_t smart_index;
	uint8_t skip_num;
	uint8_t reserved0;
};

struct isp_bpc{
	uint32_t bpc_bypass;
	uint16_t flat_thr;
	uint16_t std_thr;
	uint16_t texture_thr;
	uint16_t reserved;
};

struct isp_denoise{
	uint32_t denoise_bypass;
	uint8_t write_back;
	uint8_t diswei_level;
	uint8_t ranwei_level;
	uint8_t reserved2;
	uint16_t r_thr;
	uint16_t g_thr;
	uint16_t b_thr;
	uint16_t reserved;
	uint32_t index;
	struct _isp_denoise_tab tab[ISP_AUTO_LEVEL];
};

struct isp_grgb{
	uint32_t grgb_bypass;
	uint16_t edge_thr;
	uint16_t diff_thr;
};

struct isp_cfa{
	uint32_t cfa_bypass;
	uint16_t edge_thr;
	uint16_t diff_thr;
};

struct isp_cmc{
	uint32_t cmc_bypass;
	uint32_t index;
	uint16_t matrix[_ISP_AWB_CALI_NUM][9];
	uint16_t reserved;
};

struct isp_gamma{
	uint32_t gamma_bypass;
	uint32_t index;
	struct _isp_gamma_tab tab[ISP_AUTO_LEVEL];
};

struct isp_uvdiv{
	uint32_t uvdiv_bypass;
	uint8_t thrd[7];
	uint8_t t[2];
	uint8_t m[3];
};

struct isp_pref{
	uint32_t pref_bypass;
	uint32_t y_index;
	uint32_t uv_index;
	uint8_t write_back;
	uint8_t y_thr[ISP_AUTO_LEVEL];
	uint8_t u_thr[ISP_AUTO_LEVEL];
	uint8_t v_thr[ISP_AUTO_LEVEL];
};

struct isp_bright{
	uint32_t bright_bypass;
	uint8_t factor[16];
};

struct isp_ev{
	int8_t factor[16];
};

struct isp_contrast{
	uint32_t contrast_bypass;
	uint8_t factor[16];
};

struct isp_hist{
	uint32_t hist_bypass;
	uint16_t low_ratio;
	uint16_t high_ratio;
	uint8_t mode;
	uint8_t reserved2;
	uint8_t reserved1;
	uint8_t reserved0;
};

struct isp_auto_contrast{
	uint32_t auto_contrast_bypass;
	uint8_t mode;
	uint8_t reserved2;
	uint8_t reserved1;
	uint8_t reserved0;
};

struct isp_saturation{
	uint32_t saturation_bypass;
	uint8_t factor[16];
};

struct isp_css{
	uint32_t css_bypass;
	uint8_t low_thr[7];
	uint8_t lum_thr;
	uint8_t low_sum_thr[7];
	uint8_t chr_thr;
	uint8_t ratio[8];
};

struct isp_af{
	uint32_t af_bypass;
	uint32_t max_step;
	uint16_t min_step;
	uint16_t max_tune_step;
	uint32_t stab_period;
	uint16_t af_rough_step[32];
	uint16_t af_fine_step[32];
	uint8_t rough_count;
	uint8_t fine_count;
	uint8_t alg_id;
	uint8_t debug;
	uint16_t default_step_len;
	uint8_t peak_thr_0;
	uint8_t peak_thr_1;
	uint8_t peak_thr_2;
	uint8_t detect_thr;
	uint8_t detect_step_mum;
	uint8_t start_area_range;
	uint8_t end_area_range;
	uint8_t noise_thr;
	uint16_t video_max_tune_step;
	uint16_t video_speed_ratio;
	uint16_t anti_crash_pos;
	uint8_t denoise_lv;
	uint8_t reserved0[3];
};

struct isp_caf_param{
	uint32_t awb_cal_value_thr;
	uint32_t awb_cal_num_thr;
	uint32_t awb_cal_value_stab_thr;
	uint32_t awb_cal_num_stab_thr;
	uint32_t awb_cal_cnt_stab_thr;
	uint32_t afm_cal_thr;
	uint32_t afm_cal_stab_thr;
	uint32_t afm_cal_cnt_stab_thr;
	uint32_t awb_cal_skip_cnt;
	uint32_t afm_cal_skip_cnt;
	uint32_t caf_work_lum_thr;  //percent of ae target
};

struct isp_caf_info{
	uint32_t enable;
	struct sensor_caf_param cfg[2];
};

struct isp_af_win_rect{
	uint16_t start_x;
	uint16_t start_y;
	uint16_t end_x;
	uint16_t end_y;
};


struct isp_af_multi_win_param{
	uint32_t enable;
	uint32_t win_used_cnt;
	uint32_t win_sel_mode;
	uint8_t win_priority[32];
	struct sensor_af_win_rect win_pos[32];
};



struct isp_edge_info{
	uint32_t edge_bypass;
	uint32_t index;
	struct _isp_edge_param info[16];
};

struct isp_emboss{
	uint32_t emboss_bypass;
	uint8_t step;
	uint8_t reserved2;
	uint8_t reserved1;
	uint8_t reserved0;
};

struct isp_global_gain{
	uint32_t glb_gain_bypass;
	uint32_t gain;
};

struct isp_chanel_gain{
	uint32_t chn_gain_bypass;
	uint8_t r_gain;
	uint8_t g_gain;
	uint8_t b_gain;
	uint8_t reserved0;
	uint16_t r_offset;
	uint16_t g_offset;
	uint16_t b_offset;
	uint16_t reserved1;
};

struct isp_flash_cali{
	uint32_t flash_bypass;
	uint16_t lum_ratio;
	uint16_t r_ratio;
	uint16_t g_ratio;
	uint16_t b_ratio;
};

struct isp_cce_praam{
	uint16_t matrix[9];
	uint16_t y_shift;
	uint16_t u_shift;
	uint16_t v_shift;
};

struct isp_cce{
	uint32_t index;
	struct isp_cce_praam tab[16];
};

struct isp_fcs{
	uint32_t fcs_bypass;
};

struct isp_hue{
	uint32_t hue_bypass;
	uint8_t factor[16];
};

struct isp_hdr{
	uint32_t hdr_bypass;
};

struct isp_pre_wave_denoise{
	uint32_t pre_wave_bypass;
	uint8_t thrs0;
	uint8_t thrs1;
	uint8_t resered1;
	uint8_t reserved0;
};

struct isp_auto_adjust{
	uint32_t enable;

	uint32_t index_sensitive;
	uint32_t lum_sensitive;

	uint32_t dependon_index;
	uint32_t dependon_gain;
	uint32_t dependon_lum;

	uint32_t level_mode;
	uint32_t index_zone;
	uint32_t lum_zone;
	uint32_t target_lum_zone;

	uint32_t index_thr_num;
	uint32_t lum_low_thr_num;
	uint16_t index_thr[10];
	uint16_t index_start_level[10];
	uint16_t index_end_level[10];
	uint16_t bais_gain[10];

	uint16_t lum_low_thr[4];
	uint16_t lum_start_level[4];
	uint16_t lum_end_level[4];
};

struct isp_auto_adjust_info {
	struct isp_auto_adjust bil_denoise;
	struct isp_auto_adjust y_denoise;
	struct isp_auto_adjust uv_denoise;
	struct isp_auto_adjust cmc;
	struct isp_auto_adjust gamma;
	struct isp_auto_adjust edge;
};


struct isp_raw_tune_info{
	uint32_t version_id;
	uint32_t total_len;
	uint32_t num;
	struct module_info blc_info;
	struct module_info nlc_info;
	struct module_info lnc_info;
	struct module_info ae_info;
	struct module_info awb_info;
	struct module_info bpc_info;
	struct module_info denoise_info;
	struct module_info grgb_info;
	struct module_info cfa_info;
	struct module_info cmc_info;
	struct module_info gamma_info;
	struct module_info uvdiv_info;
	struct module_info pref_info;
	struct module_info bright_info;
	struct module_info contrast_info;
	struct module_info hist_info;
	struct module_info auto_contrast_info;
	struct module_info saturation_info;
	struct module_info css_info;
	struct module_info af_info;
	struct module_info edge_info;
	struct module_info emboss_info;
	struct module_info glb_gain_info;
	struct module_info chn_gain_info;
	struct module_info flash_cali_info;
	struct module_info cce_buf_info;
	struct module_info fcs_info;
	struct module_info hue_info;
	struct module_info hdr_info;
	struct module_info ext_info;
	struct module_info auto_adjust_info;
	struct module_info caf_info;
	struct module_info af_multi_win_info;

	struct isp_blc blc;
	struct isp_nlc nlc;
	struct isp_lnc lnc;
	struct isp_ae ae;
	struct isp_awb awb;
	struct isp_bpc bpc;
	struct isp_denoise denoise;
	struct isp_grgb grgb;
	struct isp_cfa cfa;
	struct isp_cmc cmc;
	struct isp_gamma gamma;
	struct isp_uvdiv uv_div;
	struct isp_pref pref;
	struct isp_bright bright;
	struct isp_contrast contrast;
	struct isp_hist hist;
	struct isp_auto_contrast auto_contrast;
	struct isp_saturation saturation;
	struct isp_css css;
	struct isp_af af;
	struct isp_edge_info edge;
	struct isp_emboss emboss;
	struct isp_global_gain global;
	struct isp_chanel_gain chn;
	struct isp_flash_cali flash;
	struct isp_cce cce;
	struct isp_fcs fcs;
	struct isp_hue hue;
	struct isp_hdr hdr;
	struct isp_auto_adjust_info auto_adjust;
	struct isp_caf_info caf;
	struct isp_af_multi_win_param af_multi_win;
	struct isp_pre_wave_denoise pre_wave_denoise;
	uint32_t ext[254];
};

struct isp_raw_fix_info{
	struct _isp_ae_tab ae;
	struct _isp_lnc_map lnc;
	struct _isp_awb_map awb;
};

struct _isp_raw_awb_cali{
	struct _isp_cali_info cali_info;
	struct _isp_cali_info golden_cali_info;
};

struct _isp_raw_flashlight_cali{
	struct _isp_cali_info cali_info;
	struct _isp_cali_info golden_cali_info;
};


struct _isp_raw_ae_cali{
	struct _isp_ae_change_speed *speed_dark_to_bright;
	uint32_t			step_dark_to_bright;
	struct _isp_ae_change_speed *speed_bright_to_dark;
	uint32_t			step_bright_to_dark;
	struct _isp_ae_histogram_segment *histogram_segment;
	uint32_t			histogram_segment_num;
};

struct isp_raw_cali_info{
	struct _isp_raw_ae_cali ae;
	struct _isp_raw_awb_cali awb;
	struct _isp_raw_flashlight_cali flashlight;
};

struct isp_raw_resolution_info {
	uint16_t start_x;
	uint16_t start_y;
	uint16_t width;
	uint16_t height;
	uint16_t line_time;
	uint16_t frame_line;
};

struct isp_raw_resolution_info_tab {
	uint32_t image_pattern;
	struct isp_raw_resolution_info tab[10];
};

struct isp_raw_ioctrl {
	uint32_t(*set_focus) (uint32_t param);
	uint32_t(*set_exposure) (uint32_t param);
	uint32_t(*set_gain) (uint32_t param);
	uint32_t(*get_gain_scale) (uint32_t param);
	uint32_t(*get_capture_gain) (uint32_t param);
};

struct isp_raw_info {
	struct _isp_version_info* version_info;
	struct isp_raw_tune_info* tune_ptr;
	struct isp_raw_fix_info* fix_ptr;
	struct isp_raw_cali_info* cali_ptr;
	struct isp_raw_resolution_info_tab* resolution_info_ptr;
	struct isp_raw_ioctrl* ioctrl_ptr;
};

struct isp_raw_param_info_tab {
	uint32_t param_id;
	struct isp_raw_info* info_ptr;
	uint32_t(*identify_otp) (void* param_ptr);
	uint32_t(*cfg_otp) (void* param_ptr);
};

/**----------------------------------------------------------------------------------*
**                                      v00010001 struct                                                          **
**-----------------------------------------------------------------------------------*/
#endif

