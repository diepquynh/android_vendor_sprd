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
    
#ifndef _AE_COM_H_
#define _AE_COM_H_
/*----------------------------------------------------------------------------*
 **				 Dependencies				*
 **---------------------------------------------------------------------------*/
#include "ae_types.h"
#include "ae_log.h"
#include "mulaes.h"
#include "flat.h"
#include "region.h"
#include "ae1_face.h"
#include "touch_ae.h"
#include "fae.h"
/**---------------------------------------------------------------------------*
 **				 Compiler Flag				*
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"  {
#endif	/*  */
/**---------------------------------------------------------------------------*
**				 Macro Define				*
**----------------------------------------------------------------------------*/
enum ae_alg_io_cmd {
	AE_ALG_CMD_SET_WEIGHT_TABLE,
	AE_ALG_CMD_SET_AE_TABLE,
	AE_ALG_CMD_SET_WORK_PARAM,
	AE_ALG_CMD_SET_QUICK_MODE,
	AE_ALG_CMD_SET_TARGET_LUM,
	AE_ALG_CMD_SET_AE_TABLE_RANGE,
	AE_ALG_CMD_SET_FLASH_PARAM,
	AE_ALG_CMD_SET_INDEX,
	AE_ALG_CMD_SET_EXP_ANIT,
	AE_ALG_CMD_SET_FIX_FPS,
	AE_ALG_CMD_SET_CVGN_PARAM,
	AE_ALG_CMD_SET_CONVERGE_SPEED, 
	AE_ALG_CMD_SET_EV, 
	AE_ALG_CMD_SET_FLICK_FLAG, 
	AE_ALG_CMD_GET_EXP_BY_INDEX,
	AE_ALG_CMD_GET_WEIGHT,
	AE_ALG_CMD_GET_NEW_INDEX
};

struct ae_alg_init_in {
	uint32_t start_index;
	void *param_ptr;
	uint32_t size;
};

struct ae_alg_init_out {
	uint32_t start_index;
};

struct ae_alg_rgb_gain {
	uint32_t r;
	uint32_t g;
	uint32_t b;
};

struct ae_settings {
	uint16_t ver;
	int8_t lock_ae;/* 0:unlock 1:lock 2:pause 3:wait-lock */
	int32_t pause_cnt;
	int8_t manual_mode;/* 0:exp&gain       1:table-index */
	uint32_t exp_line;	/* set exposure lines */
	uint16_t gain;	/* set gain: 128 means 1 time gain , user can set any value to it */
	uint16_t table_idx;/* set ae-table-index */
	int16_t min_fps;/* e.g. 2000 means 20.00fps , 0 means Auto */
	int16_t max_fps;
	uint8_t sensor_max_fps;/*the fps of sensor setting: it always is 30fps in normal setting*/
	int8_t flash;/*flash */
	int16_t flash_ration;/* mainflash : preflash -> 1x = 32 */
	int16_t flash_target;
	int8_t iso;
	int8_t touch_scrn_status;//touch screen,1: touch;0:no touch
	int8_t touch_tuning_enable;//for touch ae
	int8_t ev_index;/* not real value , just index !! */
	int8_t flicker;	/* 50hz 0 60hz 1 */
	int8_t flicker_mode;/* auto 0 manual 1,2 */
	int8_t FD_AE;	/* 0:off; 1:on */
	int8_t metering_mode;
	int8_t work_mode;/* DC DV */
	int8_t scene_mode;/* pano sports night */
	int16_t intelligent_module;/* pano sports night */
	int8_t reserve_case;
	uint8_t * reserve_info;/* reserve for future */
	int16_t reserve_len;/*len for reserve */
};

struct ae_alg_calc_param {
	struct ae_size frame_size;
	struct ae_size win_size;
	struct ae_size win_num;
	struct ae_alg_rgb_gain awb_gain;
	struct ae_exp_gain_table *ae_table;
	struct ae_size touch_tuning_win;//for touch ae
	struct ae_trim touch_scrn_win;//for touch ae
	struct face_tuning_param face_tp;//for face tuning
	uint8_t * weight_table;
	uint32_t * stat_img;
	uint8_t win1_weight;//for touch ae
	uint8_t win2_weight;//for touch ae
	//uint8_t touch_tuning_enable;//for touch ae
	int16_t min_exp_line;
	int16_t max_gain;
	int16_t min_gain;
	int16_t start_index;
	int16_t target_lum;
	int16_t target_lum_zone;
	int16_t line_time;
	uint32_t frame_id;
	uint32_t * r;
	uint32_t * g;
	uint32_t * b;
	int8_t ae_initial;
	int8_t alg_id;
	int32_t effect_expline;
	int32_t effect_gain;
	int32_t effect_dummy;

//caliberation for bv match with lv
	float lv_cali_lv;
	float lv_cali_bv;
/*for mlog function*/
	uint8_t mlog_en;
//refer to convergence
	uint8_t ae_start_delay;
	int16_t stride_config[2];
	int16_t under_satu;
	int16_t ae_satur;
	//for touch AE
	int8_t to_ae_state;

//adv_alg module init
	struct ae1_fd_param ae1_finfo;
	void *adv[8];
			/*
			   0:region
			   1:flat
			   2: mulaes
			   3: touch ae
			   4: face ae
			   5:flash ae????
			 */
	struct ae_settings settings;
};

struct ae1_senseor_out {
	int8_t stable;
	int8_t f_stable;
	int16_t cur_index;/*the current index of ae table in ae now: 1~1024 */
	uint32_t exposure_time;/*exposure time, unit: 0.1us */
	float cur_fps;	/*current fps:1~120 */
	uint16_t cur_exp_line;/*current exposure line: the value is related to the resolution */
	uint16_t cur_dummy;/*dummy line: the value is related to the resolution & fps */
	int16_t cur_again;/*current analog gain */
	int16_t cur_dgain;/*current digital gain */
};

struct ae_alg_calc_result {
	uint32_t version;/*version No. for this structure */
	int16_t cur_lum;/*the average of image:0 ~255 */
	int16_t target_lum;/*the ae target lum: 0 ~255 */
	int16_t target_zone;/*ae target lum stable zone: 0~255 */
	int16_t target_lum_ori;/*the ae target lum(original): 0 ~255 */
	int16_t target_zone_ori;/*the ae target lum stable zone(original):0~255 */
	uint32_t frame_id;
	int16_t cur_bv;/*bv parameter */
	int16_t cur_bv_nonmatch;
	int16_t * histogram;/*luma histogram of current frame */
	//for flash
	int32_t flash_effect;
	int8_t flash_status;
	int16_t mflash_exp_line;
	int16_t mflash_dummy;
	int16_t mflash_gain;
	//for touch
	int8_t tcAE_status;
	int8_t tcRls_flag;
	//for face debug
	uint32_t face_lum;
	mulaes_stat * pmulaes;
	flat_stat * pflat;
	region_stat * pregion;
	touch_stat * ptc;/*Bethany add touch info to debug info */
	fae_stat *pface_ae;
	struct ae1_senseor_out wts;
	void* log;
	uint32_t * reserved;/*resurve for future */
};

struct ae_alg_fun_tab {
	void *(*init) (struct ae_alg_init_in *, struct ae_alg_init_out *);
	int32_t(*deinit) (void *, void *, void *);
	int32_t(*calc) (void *, void *, void *);
	int32_t(*ioctrl) (void *, enum ae_alg_io_cmd, void *, void *);
};

/**---------------------------------------------------------------------------*
**				Data Prototype				*
**----------------------------------------------------------------------------*/
/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
} 
#endif	/*  */
/**---------------------------------------------------------------------------*/
#endif	/*  */

