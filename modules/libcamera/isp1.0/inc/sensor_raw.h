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
#ifndef _SENSOR_RAW_H_
#define _SENSOR_RAW_H_

//#include "jpeg_exif_header.h"
//#include "cmr_common.h"
#include <sys/types.h>
//#include "sensor_drv_u.h"
#include"isp_smart_fitting.h"

#define MAX_AWB_MOD 8
#define MAX_EV_LEVEL 15
#define MAX_BRIGHTNESS_LEVEL 15
#define MAX_CONTRAST_LEVEL 15
#define MAX_HUE_LEVEL 15

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
#define SENSOR_ISO_NUM 0x06
#define SENSOR_AE_NUM 0x500
#define SENSOR_AE_TAB_NUM 0x04
#define SENSOR_AWB_CALI_NUM 0x09
#define SENSOR_AWB_NUM 0x14
#define SENSOR_MAP_NUM 0x08
#define SENSOR_AWB_G_ESTIMATE_NUM 0x6
#define SENSOR_AWB_GAIN_ADJUST_NUM 0x9
#define SENSOR_AWB_LIGHT_NUM 0x10
#define SENSOR_ENVI_NUM 0x8
#define SENSOR_PIECEWISE_SAMPLE_NUM 0x10
#define SENSOR_CT_INFO_NUM 0x8
#define SENSOR_DEFAULT_PIX_WIDTH 0x0a
#define SENSOR_AWB_V01_VERIFY	0xff617762
#define RAW_INFO_END_ID 0x71717567

#define SENSOR_RAW_VERSION_ID 0x00000000 /* tiger-0000xxxx, soft-xxxx0000*/
#define SENSOR_RAW_V0001_VERSION_ID 0x00010000 /* tiger-0000xxxx, soft-xxxx0000*/
#define SENSOR_LSC_SECTION_MAX_NUM   0x10
#define SENSOR_ADV_LSC_VERIFY 0x5126

#define SENSOR_EXP_GAIN_TABLE_SIZE 512
#define SENSOR_FLICKER_NUM 2
#define SENSOR_AE_TAB 9

#define SENSOR_EV_CALI_NUM 20

#define SENSOR_NEW_GAMMA_NUM 41

enum sensor_environment_id {
	SENSOR_ENVI_COMMON = 0,
	SENSOR_ENVI_LOW_LIGHT = 1,
	SENSOR_ENVI_INDOOR = 2,
	SENSOR_ENVI_OUTDOOR = 3,
	SENSOR_ENVI_HIGH_LIGHT_L0 = 4,
	SENSOR_ENVI_HIGH_LIGHT_L1 = 5
};

struct sensor_pos{
	uint16_t x;
	uint16_t y;
};

struct sensor_size{
	uint16_t w;
	uint16_t h;
};

struct sensor_trim_size{
	uint16_t x;
	uint16_t y;
	uint16_t w;
	uint16_t h;
};

struct sensor_pos_rect{
	uint16_t start_x;
	uint16_t start_y;
	uint16_t end_x;
	uint16_t end_y;
};

struct sensor_rgb_gain{
	uint16_t r_gain;
	uint16_t g_gain;
	uint16_t b_gain;
};

struct sensor_blc_offset{
	uint16_t r;
	uint16_t gr;
	uint16_t gb;
	uint16_t b;
};

struct sensor_blc_param{
	uint8_t mode;
	uint8_t reserved2;
	uint8_t reserved1;
	uint8_t reserved0;
	struct sensor_blc_offset offset[8];
};

struct sensor_nlc_param{
	uint16_t r_node[29];
	uint16_t g_node[29];
	uint16_t b_node[29];
	uint16_t l_node[29];
};

struct sensor_lnc_map_addr{
	uint32_t grid;
	uint16_t* param_addr;
	uint32_t len;
};

struct sensor_lnc_map{
	struct sensor_lnc_map_addr map[SENSOR_MAP_NUM][SENSOR_AWB_CALI_NUM];
};

struct sensor_awb_map{
	uint16_t *addr;
	uint32_t len;		//by bytes
};

struct sensor_awb_weight{
	uint8_t *addr;
	uint16_t width;
	uint16_t height;
};

struct sensor_ae_index{
	uint16_t start;
	uint16_t max;
};

struct sensor_ae_tab_info{
	uint32_t* e_ptr;
	uint16_t* g_ptr;
	struct sensor_ae_index index[SENSOR_ISO_NUM];
};

struct sensor_ae_histogram_segment {
	uint8_t		min;
	uint8_t		max;
	uint16_t	weight;
};

struct sensor_ae_change_speed {
	uint16_t	delta_lum;
	uint16_t	delta_ev;
};

struct sensor_ae_param{
	uint8_t skip_frame;
	uint8_t normal_fix_fps;
	uint8_t night_fix_fps;
	uint8_t video_fps;
	uint8_t target_lum;
	uint8_t target_zone;
	uint16_t smart;
//	uint8_t smart_rotio;
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

	uint8_t smart_pref_y_min;
	uint8_t smart_pref_y_max;
	uint8_t smart_pref_uv_min;
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

	uint8_t smart_pref_y_outdoor;
	uint8_t smart_pref_y_mid;
	uint8_t smart_pref_uv_outdoor;
	uint8_t smart_pref_uv_mid;

	uint8_t edge_prev_percent;
	uint8_t denoise_prev_percent;
	uint8_t reserved0;
	uint8_t reserved1;
	uint32_t reserved[24];
};

struct sensor_ae_tab{
	uint8_t* weight_tab;
	struct sensor_ae_tab_info tab[SENSOR_AE_TAB_NUM];
};

struct sensor_ae_exp_gain {
	uint32_t exp;
	uint32_t dummy;
	uint32_t gain;
	uint16_t iso;
	uint16_t fps;
};

struct sensor_ae_tab_index {
	uint16_t start;
	uint16_t min;
	uint16_t prv_max;
	uint16_t cap_max;
	uint16_t reserved[16];
};

struct sensor_ae_max_gain {
	uint16_t again;
	uint16_t dgain;
};

struct sensor_ae_table_info {
	struct sensor_ae_max_gain max;
	struct sensor_ae_tab_index index;
	struct sensor_ae_exp_gain tab[SENSOR_EXP_GAIN_TABLE_SIZE];
};

struct sensor_ae_table {
	struct sensor_ae_table_info ae[SENSOR_FLICKER_NUM][SENSOR_AE_TAB];
};

struct sensor_ae_table1 {
	struct sensor_ae_table_info ae[2][2];
};

struct sensor_lv_cali_tab {
	uint32_t exp;
	uint32_t gain;
	uint32_t lv;
	uint32_t lux;
};

struct sensor_lv_cali {
	uint32_t num;
	struct sensor_lv_cali_tab tab[SENSOR_EV_CALI_NUM];
};

struct sensor_lv_cali_param {
	uint32_t lv_exp;
	uint32_t lv_gain;
	uint32_t lv;
	uint32_t lux_exp;
	uint32_t lux_gain;
	uint32_t lux;
};

struct sensor_ev_cali {
	uint32_t mode;
	uint32_t f_num;
	struct sensor_lv_cali_param single;
	struct sensor_lv_cali mult;
	uint32_t reserved[256];
};

struct sensor_rgb{
	uint16_t r;
	uint16_t g;
	uint16_t b;
};

struct sensor_awb_coord{
	uint16_t x;
	uint16_t yb;
	uint16_t yt;
};

struct sensor_cali_info {
	uint32_t r_sum;
	uint32_t gr_sum;
	uint32_t gb_sum;
	uint32_t b_sum;
};

struct sensor_sample {
	int16_t		x;
	int16_t		y;
};

struct sensor_piecewise_func {
	uint32_t num;
	struct sensor_sample samples[SENSOR_PIECEWISE_SAMPLE_NUM];
};

struct sensor_range {
	int16_t min;
	int16_t max;
};

struct sensor_range_l {
	int32_t min;
	int32_t max;
};

struct sensor_awb_weight_of_ct_func {
	struct sensor_piecewise_func weight_func;
};

struct sensor_awb_ct_info {
	int32_t data[SENSOR_CT_INFO_NUM];
};

struct sensor_awb_weight_of_count_func {
	struct sensor_piecewise_func weight_func;
	uint32_t base;
};

struct sensor_awb_ref_param {
	uint16_t ct;
	struct sensor_rgb_gain gain;
};
struct sensor_awb_param{
	struct sensor_pos win_start;
	struct sensor_size win_size;
	struct sensor_rgb gain_convert[8];
	struct sensor_awb_coord win[SENSOR_AWB_NUM];
	struct sensor_awb_weight_of_ct_func weight_of_ct_func;
	uint32_t steady_speed;
	uint16_t r_gain[SENSOR_AWB_NUM];
	uint16_t g_gain[SENSOR_AWB_NUM];
	uint16_t b_gain[SENSOR_AWB_NUM];
	uint8_t gain_index;
	uint8_t alg_id;
	uint8_t reserved2[2];
	uint32_t target_zone;
	uint32_t smart;
	uint32_t quick_mode;
	struct sensor_awb_weight_of_count_func weight_of_count_func;
	struct sensor_awb_ct_info ct_info;
	struct sensor_rgb init_gain;
	uint16_t init_ct;
	uint32_t green_factor;
	uint32_t skin_factor;
	uint8_t debug_level;
	uint8_t smart_index;
	uint8_t skip_num;
	uint8_t reserved3;
	struct sensor_range value_range[SENSOR_ENVI_NUM];
	uint32_t reserved;
};
struct sensor_awb_param_v01 {
	uint32_t verify;
	uint32_t version;
	struct sensor_awb_weight_of_ct_func weight_of_ct_func[SENSOR_ENVI_NUM];
	struct sensor_awb_ref_param ref_param[SENSOR_ENVI_NUM];
	uint32_t reserved[256];
};

struct sensor_smart_light_envi_param {
	uint32_t enable;
	struct sensor_range_l bv_range[SENSOR_ENVI_NUM];
	uint32_t reserved[32];
};

struct sensor_smart_light_hue_param
{
	uint32_t enable;
	/*adjust function for each enviroment, x=ct, y=hue*/
	struct sensor_piecewise_func adjust_func[SENSOR_ENVI_NUM];
	uint32_t reserved[32];
};

struct sensor_smart_light_saturation_param
{
	uint32_t enable;
	/*adjust function for each enviroment, x=ct, y=saturation value*/
	struct sensor_piecewise_func adjust_func[SENSOR_ENVI_NUM];
	uint32_t reserved[32];
};

struct sensor_smart_light_lsc_param {
	uint32_t enable;
	/*adjust function for each enviroment, x=ct, y=index*/
	struct sensor_piecewise_func adjust_func[SENSOR_ENVI_NUM];
	uint32_t reserved[32];
};

struct sensor_smart_light_cmc_param {
	uint32_t enable;
	/*adjust function for each enviroment, x=ct, y=index*/
	struct sensor_piecewise_func adjust_func[SENSOR_ENVI_NUM];
	uint32_t reserved[32];
};

struct sensor_smart_light_gain_param {
	uint32_t enable;
	/*adjust function for each enviroment, x=ct, y=gain factor*/
	struct sensor_piecewise_func r_gain_func[SENSOR_ENVI_NUM];
	struct sensor_piecewise_func g_gain_func[SENSOR_ENVI_NUM];
	struct sensor_piecewise_func b_gain_func[SENSOR_ENVI_NUM];
	uint32_t reserved[32];
};

struct sensor_smart_light_param {
	uint32_t enable;
	struct sensor_smart_light_envi_param envi;
	struct sensor_smart_light_lsc_param lsc;
	struct sensor_smart_light_cmc_param cmc;
	struct sensor_smart_light_hue_param hue;
	struct sensor_smart_light_saturation_param saturation;
	struct sensor_smart_light_gain_param gain;
	uint32_t reserved[256];
};

struct sensor_bpc_param{
	uint16_t flat_thr;
	uint16_t std_thr;
	uint16_t texture_thr;
	uint16_t reserved;
};

struct sensor_nbpc_param{
	uint8_t bypass_pvd;
	uint8_t nbpc_mode;
	uint8_t mask_mode;
	uint8_t kmin;
	uint8_t kmax;
	uint8_t cntr_theshold;
	uint8_t hwfifo_clr_en;
	uint8_t ktimes;
	uint8_t map_fifo_clr;
	uint8_t delt34;
	uint8_t flat_factor;
	uint8_t safe_factor;
	uint8_t spike_coeff;
	uint8_t dead_coeff;
	uint8_t map_done_sel;
	uint8_t new_old_sel;
	uint16_t bad_pixel_num;
	uint16_t reserved1;
	uint16_t interrupt_b[8];
	uint16_t slope_k[8];
	uint16_t lut_level[8];
	uint32_t map_addr;
	uint32_t reserved2;
};
struct sensor_denoise_tab {
	uint8_t diswei[19];
	uint8_t diswei_level;
	uint8_t ranwei[31];
	uint8_t ranwei_level;
};

struct sensor_denoise_param{
	uint8_t write_back;
	uint8_t ranwei_level;
	uint8_t diswei_level;
	uint8_t reserved2;
	uint16_t r_thr;
	uint16_t g_thr;
	uint16_t b_thr;
	uint16_t reserved;
	uint8_t diswei[19];
	uint8_t ranwei[31];
	uint8_t reserved1;
	uint8_t reserved0;
	struct sensor_denoise_tab tab[1];
	uint32_t reserved5[57];
};

struct sensor_grgb_param{
	uint16_t edge_thr;
	uint16_t diff_thr;
};

struct sensor_cfa_param{
	uint16_t edge_thr;
	uint16_t diff_thr;
};

struct sensor_cmc_param{
	uint16_t matrix[SENSOR_AWB_CALI_NUM][9];
	uint16_t flash_index;
};

struct sensor_cce_parm{
	uint16_t matrix[9];
	uint16_t y_shift;
	uint16_t u_shift;
	uint16_t v_shift;
};

struct sensor_gamma_tab{
	uint16_t axis[2][26];
};

struct sensor_gamma_param{
	uint16_t axis[2][26];
	struct sensor_gamma_tab tab[5];
};

struct sensor_cce_uvdiv{
	uint8_t thrd[7];
	uint8_t t[2];
	uint8_t m[3];
	uint32_t reserved1[19];
};

struct sensor_pref_param{
	uint8_t write_back;
	uint8_t y_thr;
	uint8_t u_thr;
	uint8_t v_thr;
	uint32_t reserved[20];
};

struct sensor_bright_param{
	uint8_t factor[16];
};

struct sensor_contrast_param{
	uint8_t factor[16];
};

struct sensor_hist_param
{
	uint16_t low_ratio;
	uint16_t high_ratio;
	uint8_t mode;
	uint8_t reserved2;
	uint8_t reserved1;
	uint8_t reserved0;
};

struct sensor_auto_contrast_param{
	uint8_t mode;
	uint8_t reserved2;
	uint8_t reserved1;
	uint8_t reserved0;
};

struct sensor_saturation_param{
	uint8_t factor[16];
};

struct sensor_af_param{
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
	uint32_t reserved[2];
};

struct sensor_caf_param{
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
	uint32_t reserved[16];
};

struct sensor_caf_info{
	uint32_t enable;
	struct sensor_caf_param cfg[2];
};

struct sensor_af_win_rect{
	uint16_t start_x;
	uint16_t start_y;
	uint16_t end_x;
	uint16_t end_y;
};


struct sensor_af_multi_win_param{
	uint32_t enable;
	uint32_t win_used_cnt;
	uint32_t win_sel_mode;
	uint8_t win_priority[32];
	struct sensor_af_win_rect win_pos[32];
	uint32_t reserved[32];
};



struct sensor_emboss_param{
	uint8_t step;
	uint8_t reserved2;
	uint8_t reserved1;
	uint8_t reserved0;
};

struct sensor_edge_param{
	uint8_t detail_thr;
	uint8_t smooth_thr;
	uint8_t strength;
	uint8_t reserved;
};

struct sensor_edge_info{
	struct sensor_edge_param info[16];
};

struct sensor_global_gain_param{
	uint32_t gain;
	uint32_t reserved[20];
};


struct sensor_chn_gain_param{
	uint8_t r_gain;
	uint8_t g_gain;
	uint8_t b_gain;
	uint8_t reserved0;
	uint16_t r_offset;
	uint16_t g_offset;
	uint16_t b_offset;
	uint16_t reserved1;
	uint32_t reserved2[50];
};

struct sensor_flash_cali_param{
	uint16_t lum_ratio;
	uint16_t r_ratio;
	uint16_t g_ratio;
	uint16_t b_ratio;
	uint16_t adj_coef;
	uint16_t reserved0;
	uint32_t reserved[49];
};

struct sensor_css_param{
	uint8_t low_thr[7];
	uint8_t lum_thr;
	uint8_t low_sum_thr[7];
	uint8_t chr_thr;
	uint8_t ratio[8];
	uint32_t reserved[68];
};

struct sensor_auto_adjust{
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

struct sensor_smart_cali{
	struct isp_smart_scene_info scene_info;
	struct isp_smart_bv_thr_info bv_info;
};

struct sensor_smart_adjust_info {
	struct sensor_smart_cali denoise_ranwei;
	struct sensor_smart_cali denoise_diswei;
	struct sensor_smart_cali pref_y;
	struct sensor_smart_cali pref_uv;
	struct sensor_smart_cali soft_y;
	struct sensor_smart_cali soft_uv;
	struct sensor_smart_cali gamma;
	struct sensor_smart_cali edge_detail;
	struct sensor_smart_cali edge_smooth;
	struct sensor_smart_cali edge_strength;
	struct sensor_smart_cali bpc_flat;
	struct sensor_smart_cali bpc_std;
	struct sensor_smart_cali bpc_texture;
	uint32_t denoise_enable;
	uint32_t pref_enable;
	uint32_t soft_enable;
	uint32_t gamma_enable;
	uint32_t edge_enable;
	uint32_t bpc_enable;
	uint32_t reserved[216];
};

struct sensor_auto_adjust_info {
	struct sensor_auto_adjust bil_denoise;
	struct sensor_auto_adjust y_denoise;
	struct sensor_auto_adjust uv_denoise;
	struct sensor_auto_adjust cmc;
	struct sensor_auto_adjust gamma;
	struct sensor_auto_adjust edge;
	uint32_t reserved[122];
};

struct sensor_version_info{
	uint32_t version_id;
	uint32_t srtuct_size;
	uint32_t reserve;
};

struct sensor_lnc_param{
	uint32_t start_index;
	uint32_t flash_index;
	uint32_t reserved[31];
};

struct sensor_pre_wave_denoise_param{
	uint8_t thrs0;
	uint8_t thrs1;
	uint8_t reserved1;
	uint8_t reserved0;
};

struct sensor_adv_lsc_seg1_param{
	uint32_t center_r;
	uint32_t luma_thr;
	uint32_t chroma_thr;
	uint32_t luma_gain;
	uint32_t chroma_gain;
};
struct sensor_adv_lsc_seg2_param{
	struct sensor_range_l luma_th[3];
};
struct sensor_adv_lsc1_param{
	uint16_t intensity;
	uint16_t polyfit_level;
	struct sensor_adv_lsc_seg1_param seg1_param;
	struct sensor_adv_lsc_seg2_param seg2_param;
	uint16_t seg_ratio;
	uint16_t gain_percent;
	struct sensor_range_l new_gain;
	struct sensor_range_l last_gain;
	uint32_t reserved[16];
};
struct sensor_adv_lsc2_param{
	uint16_t intensity;
	uint16_t reserved0;
	struct sensor_adv_lsc_seg1_param seg1_param;
	struct sensor_adv_lsc_seg2_param seg2_param;
	uint16_t seg_ratio;
	uint16_t gain_percent;
	struct sensor_range_l new_gain;
	struct sensor_range_l last_gain;
	struct sensor_piecewise_func calc_cnt;
	uint32_t reserved[16];
};
struct sensor_adv_lsc_param{
	uint32_t version_id;
	uint32_t enable;
	uint8_t alg_id;
	uint8_t param_level;
	uint8_t debug_level;
	uint8_t ct_percent;
	struct sensor_range_l gain_range;
	struct sensor_adv_lsc1_param lsc1;
	struct sensor_adv_lsc2_param lsc2;
	uint32_t reserved[64];
};
struct sensor_new_gamma_tab{
	uint16_t x_node[16];
	uint16_t y_node[2][16];
	int16_t brightness_factor;
	int16_t contrast_factor;
};
struct sensor_new_gamma_param{
	struct sensor_new_gamma_tab tab[5];
};
struct sensor_new_gamma_lookup_table{
	uint32_t curve_num;
	struct sensor_new_gamma_tab tab[SENSOR_NEW_GAMMA_NUM];
};

struct sensor_raw_tune_info{
	uint32_t version_id;
	uint32_t blc_bypass;
	uint32_t nlc_bypass;
	uint32_t lnc_bypass;
	uint32_t ae_bypass;
	uint32_t awb_bypass;
	uint32_t bpc_bypass;
	uint32_t denoise_bypass;
	uint32_t grgb_bypass;
	uint32_t cmc_bypass;
	uint32_t gamma_bypass;
	uint32_t uvdiv_bypass;
	uint32_t pref_bypass;
	uint32_t bright_bypass;
	uint32_t contrast_bypass;
	uint32_t hist_bypass;
	uint32_t auto_contrast_bypass;
	uint32_t af_bypass;
	uint32_t edge_bypass;
	uint32_t fcs_bypass;
	uint32_t css_bypass;
	uint32_t saturation_bypass;
	uint32_t hdr_bypass;
	uint32_t glb_gain_bypass;
	uint32_t chn_gain_bypass;
	uint32_t pre_wave_bypass;
	uint32_t nbpc_bypass;
	uint32_t hue_bypass;
	uint32_t reserve6;
	uint32_t reserve5;
	uint32_t reserve4;
	uint32_t reserve3;
	uint32_t reserve2;
	uint32_t reserve1;
	uint32_t param_modify_time;

	struct sensor_blc_param blc;
	struct sensor_nlc_param nlc;
	struct sensor_ae_param ae;
	struct sensor_awb_param awb;
	struct sensor_bpc_param bpc;
	struct sensor_denoise_param denoise;
	struct sensor_grgb_param grgb;
	struct sensor_cfa_param cfa;
	struct sensor_cmc_param cmc;
	struct sensor_gamma_param gamma;
	struct sensor_cce_parm cce;
	struct sensor_cce_uvdiv uv_div;
	struct sensor_pref_param pref;
	struct sensor_bright_param bright;
	struct sensor_contrast_param contrast;
	struct sensor_hist_param hist;
	struct sensor_auto_contrast_param auto_contrast;
	struct sensor_saturation_param saturation;
	struct sensor_css_param css;
	struct sensor_af_param af;
	struct sensor_edge_info edge;
	struct sensor_emboss_param emboss;
	struct sensor_global_gain_param global;
	struct sensor_chn_gain_param chn;
	struct sensor_flash_cali_param flash;
	struct sensor_cce_parm special_effect[16];
	struct sensor_lnc_param lnc;
	struct sensor_auto_adjust_info auto_adjust;
	struct sensor_smart_light_param smart_light;
	struct sensor_caf_info caf;
	struct sensor_af_multi_win_param af_multi_win;
	struct sensor_pre_wave_denoise_param pre_wave_denoise;
	struct sensor_nbpc_param nbpc;
	struct sensor_awb_param_v01 awb_v01;
	struct sensor_new_gamma_param new_gamma;
	struct sensor_adv_lsc_param adv_lsc;
	struct sensor_smart_adjust_info smart_adjust;
	struct sensor_ev_cali ev_cali;

	uint32_t reserved[256];
};

struct sensor_raw_fix_info{
	struct sensor_ae_tab ae;
	struct sensor_lnc_map lnc;
	struct sensor_awb_map awb;
	struct sensor_awb_weight awb_weight;
	struct sensor_ae_table ae_tab;
};

struct sensor_raw_awb_cali{
	struct sensor_cali_info cali_info;
	struct sensor_cali_info golden_cali_info;
};

struct sensor_raw_flashlight_cali{
	struct sensor_cali_info cali_info;
	struct sensor_cali_info golden_cali_info;
};


struct sensor_raw_ae_cali{
	struct sensor_ae_change_speed *speed_dark_to_bright;
	uint32_t			step_dark_to_bright;
	struct sensor_ae_change_speed *speed_bright_to_dark;
	uint32_t			step_bright_to_dark;
	struct sensor_ae_histogram_segment *histogram_segment;
	uint32_t			histogram_segment_num;
};

struct sensor_raw_cali_info{
	struct sensor_raw_ae_cali ae;
	struct sensor_raw_awb_cali awb;
	struct sensor_raw_flashlight_cali flashlight;
};

struct sensor_raw_resolution_info {
	uint16_t start_x;
	uint16_t start_y;
	uint16_t width;
	uint16_t height;
	uint16_t line_time;
	uint16_t frame_line;
};

struct sensor_raw_resolution_info_tab {
	uint32_t image_pattern;
	struct sensor_raw_resolution_info tab[10];
};

struct sensor_raw_ioctrl {
	uint32_t(*set_focus) (uint32_t param);
	uint32_t(*set_exposure) (uint32_t param);
	uint32_t(*set_gain) (uint32_t param);
	uint32_t (*get_gain_scale) (uint32_t param);
	uint32_t (*get_capture_gain) (uint32_t param);
	uint32_t(*ext_fuc) (void *param);
	uint32_t (*write_i2c)(uint16_t slave_addr, uint8_t *cmd, uint16_t cmd_length);
	uint32_t (*read_i2c)(uint16_t slave_addr, uint8_t *cmd, uint16_t cmd_length);
	uint32_t(*ex_set_exposure) (unsigned long param);
};

struct sensor_raw_info {
	struct sensor_version_info* version_info;
	struct sensor_raw_tune_info* tune_ptr;
	struct sensor_raw_fix_info* fix_ptr;
	struct sensor_raw_cali_info* cali_ptr;
	struct sensor_raw_resolution_info_tab* resolution_info_ptr;
	struct sensor_raw_ioctrl* ioctrl_ptr;
	struct sensor_new_gamma_lookup_table *new_gamma_lookup_tab;
	struct sensor_raw_tune_info* cap_tune_ptr;
};

struct raw_param_info_tab {
	uint32_t param_id;
	struct sensor_raw_info* info_ptr;
	uint32_t(*identify_otp) (void* param_ptr);
	uint32_t(*cfg_otp) (void* param_ptr);
};

#endif
