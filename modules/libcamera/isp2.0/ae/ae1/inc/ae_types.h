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

#ifndef _AE_TYPES_H_
#define _AE_TYPES_H_

/*----------------------------------------------------------------------------*
 **				 Dependencies					*
 **---------------------------------------------------------------------------*/
#ifdef CONFIG_FOR_TIZEN
#include "stdint.h"
#elif WIN32
#include "ae_porting.h"
#else
#include <sys/types.h>
#endif
/**---------------------------------------------------------------------------*
 **				 Compiler Flag					*
 **---------------------------------------------------------------------------*/
#define AE_EXP_GAIN_TABLE_SIZE 512
#define AE_WEIGHT_TABLE_SIZE	1024
#define AE_ISO_NUM	6
#define AE_SCENE_NUM	8
#define AE_FLICKER_NUM 2
#define AE_WEIGHT_TABLE_NUM 3
#define AE_EV_LEVEL_NUM 16
#define AE_PARAM_VERIFY	0x61656165
#define AE_OFFSET_NUM 20
#define AE_CVGN_NUM  4
#define AE_TABLE_32
#define AE_BAYER_CHNL_NUM 4
#define AE_PIECEWISE_MAX_NUM 16
#define AE_WEIGHT_UNIT 256
#define AE_FIX_PCT 1024
#define AE_PIECEWISE_SAMPLE_NUM 0x10
#define AE_CFG_NUM 8

enum ae_environ_mod {
	ae_environ_night,
	ae_environ_lowlux,
	ae_environ_normal,
	ae_environ_hightlight,
	ae_environ_num,
};

enum ae_return_value {
	AE_SUCCESS = 0x00,
	AE_ERROR,
	AE_PARAM_ERROR,
	AE_PARAM_NULL,
	AE_FUN_NULL,
	AE_HANDLER_NULL,
	AE_HANDLER_ID_ERROR,
	AE_ALLOC_ERROR,
	AE_FREE_ERROR,
	AE_DO_NOT_WRITE_SENSOR,
	AE_SKIP_FRAME,
	AE_RTN_MAX
};

enum ae_calc_func_y_type {
	AE_CALC_FUNC_Y_TYPE_VALUE = 0,
	AE_CALC_FUNC_Y_TYPE_WEIGHT_VALUE = 1,
};

struct ae_weight_value {
	int16_t value[2];
	int16_t weight[2];
};

struct ae_sample {
	int16_t x;
	int16_t y;
};

struct ae_piecewise_func {
	int32_t num;
	struct ae_sample samples[AE_PIECEWISE_SAMPLE_NUM];
};

struct ae_range {
	int32_t min;
	int32_t max;
};

struct ae_size {
	uint32_t w;
	uint32_t h;
};

struct ae_trim {
	uint32_t x;
	uint32_t y;
	uint32_t w;
	uint32_t h;
};

struct ae_rect {
	uint32_t start_x;
	uint32_t start_y;
	uint32_t end_x;
	uint32_t end_y;
};

struct ae_param {
	void *param;
	uint32_t size;
};

struct ae_set_fps {
	uint32_t min_fps;	// min fps
	uint32_t max_fps;	// fix fps flag
};

struct ae_exp_gain_table {
	int32_t min_index;
	int32_t max_index;
	uint32_t exposure[AE_EXP_GAIN_TABLE_SIZE];
	uint32_t dummy[AE_EXP_GAIN_TABLE_SIZE];
	uint16_t again[AE_EXP_GAIN_TABLE_SIZE];
	uint16_t dgain[AE_EXP_GAIN_TABLE_SIZE];
};

struct ae_weight_table {
	uint8_t weight[AE_WEIGHT_TABLE_SIZE];
};

struct ae_ev_table {
	int32_t lum_diff[AE_EV_LEVEL_NUM];
	/*number of level */
	uint32_t diff_num;
	/*index of default */
	uint32_t default_level;
};

struct ae_flash_ctrl {
	uint32_t enable;
	uint32_t main_flash_lum;
	uint32_t convergence_speed;
};

struct touch_zone {
	uint32_t level_0_weight;
	uint32_t level_1_weight;
	uint32_t level_1_percent;	//x64
	uint32_t level_2_weight;
	uint32_t level_2_percent;	//x64
};

struct ae_flash_tuning {
	uint32_t exposure_index;
};

struct ae_stat_req {
	uint32_t mode;		//0:normal, 1:G(center area)
	uint32_t G_width;	//100:G mode(100x100)
};

struct ae_auto_iso_tab {
	uint16_t tbl[AE_FLICKER_NUM][AE_EXP_GAIN_TABLE_SIZE];
};

struct ae_ev_cali_param {
	uint32_t index;
	uint32_t lux;
	uint32_t lv;
};

struct ae_ev_cali {
	uint32_t num;
	uint32_t min_lum;	// close all the module of after awb module
	struct ae_ev_cali_param tab[16];	// cali EV sequence is low to high
};

struct ae_rgb_l {
	uint32_t r;
	uint32_t g;
	uint32_t b;
};

struct ae_opt_info {
	struct ae_rgb_l gldn_stat_info;
	struct ae_rgb_l rdm_stat_info;
};

struct ae_exp_anti {
	uint32_t enable;
	uint8_t hist_thr[40];
	uint8_t hist_weight[40];
	uint8_t pos_lut[256];
	uint8_t hist_thr_num;
	uint8_t adjust_thr;
	uint8_t stab_conter;
	uint8_t reserved1;

	uint32_t reserved[175];
};

struct ae_convergence_parm {
	uint32_t highcount;
	uint32_t lowcount;
	uint32_t highlum_offset_default[AE_OFFSET_NUM];
	uint32_t lowlum_offset_default[AE_OFFSET_NUM];
	uint32_t highlum_index[AE_OFFSET_NUM];
	uint32_t lowlum_index[AE_OFFSET_NUM];
};

struct ae_flash_tuning_param {
	uint8_t skip_num;
	uint8_t target_lum;
	uint8_t adjust_ratio;	/* 1x --> 32 */
	uint8_t reserved;
};

struct ae_sensor_cfg {
	uint16_t max_gain;
	uint16_t min_gain;
	uint8_t gain_precision;
	uint8_t exp_skip_num;
	uint8_t gain_skip_num;
	uint8_t reserved;
};

struct ae_lv_calibration {
	uint16_t lux_value;
	int16_t bv_value;
};

struct ae_face_tune_param {
	int32_t param_face_weight;/* The ratio of face area weight (in percent) */
	int32_t param_convergence_speed;/* AE convergence speed */
	int32_t param_lock_ae;/* frames to lock AE */
	int32_t param_lock_weight_has_face;/* frames to lock the weight table, when has faces */
	int32_t param_lock_weight_no_face;/* frames to lock the weight table, when no faces */
	int32_t param_shrink_face_ratio;/* The ratio to shrink face area. In percent */
};

struct ae_scene_info {
	uint32_t enable;
	uint32_t scene_mode;
	uint32_t target_lum;
	uint32_t iso_index;
	uint32_t ev_offset;
	uint32_t max_fps;
	uint32_t min_fps;
	uint32_t weight_mode;
	//uint32_t default_index;
	uint8_t table_enable;
	uint8_t  exp_tbl_mode;
	uint16_t reserved0;
	uint32_t reserved1;
	struct ae_exp_gain_table ae_table[AE_FLICKER_NUM];
};

struct ae_param_tmp_001{
	uint32_t version;
	uint32_t verify;
	uint32_t alg_id;
	uint32_t target_lum;
	uint32_t target_lum_zone; // x16
	uint32_t convergence_speed; // x16
	uint32_t flicker_index;
	uint32_t min_line;
	uint32_t start_index;
	uint32_t exp_skip_num;
	uint32_t gain_skip_num;
	struct ae_stat_req stat_req;
	struct ae_flash_tuning flash_tuning;
	struct touch_zone touch_param;
	struct ae_ev_table ev_table;
};

struct ae_param_tmp_002{
	struct ae_exp_anti exp_anti;
	struct ae_ev_cali ev_cali;
	struct ae_convergence_parm cvgn_param[AE_CVGN_NUM];
};

/**---------------------------------------------------------------------------*/
#endif
