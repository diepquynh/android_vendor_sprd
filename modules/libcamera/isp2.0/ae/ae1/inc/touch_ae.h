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
#ifndef _TOUCH_AE_H_
#define _TOUCH_AE_H_
/*----------------------------------------------------------------------------*
 **				 Dependencies				*
 **---------------------------------------------------------------------------*/
#include "ae_types.h"
/**---------------------------------------------------------------------------*
 **				 Compiler Flag				*
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {

#endif/*  */
/**---------------------------------------------------------------------------*
**				Macro Define				*
**----------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**				Data Structures				*
**---------------------------------------------------------------------------*/
	struct ae_touch_param {
		uint8_t win2_weight;//for touch ae
		uint8_t enable;//for touch ae
		uint8_t win1_weight;//for touch ae
		uint8_t reserved;//for touch ae
		struct ae_size touch_tuning_win;//for touch ae
	};

	typedef struct  {
		uint8_t tuning_enable;
		int8_t tc_scrn_enable;
		uint8_t mlog_en;
		struct ae_trim touch_scrn_coord;
		struct ae_size touch_tuning_window;
		struct ae_size small_win_numVH;
		struct ae_size small_win_wh;
		struct ae_size image_wh;
		uint8_t * touch_roi_weight;
		uint8_t * ydata;
		uint32_t stat_size;
		uint8_t base_win_weight;
		uint8_t tcROI_win_weight;
		float real_lum;
		float real_target;
	} touch_in;//tuning info

	typedef struct {
		int16_t tar_offset;
		float ratio;
		float touch_roi_lum;
		int8_t tcAE_state;//for release
		int8_t release_flag;//for release
		char* log;
	} touch_rt;//result info
	
	typedef struct  {
		uint8_t mlog_en;
		uint8_t debug_status;
		touch_in in_touch_ae;
		touch_rt result_touch_ae;
		int16_t cur_bv;
		int16_t pre_bv;
		float deltaLUM;
		uint32_t touchae_status;
		touch_in cur_status;
		touch_in prv_status;
		uint32_t touchae_weight[1024];
		uint32_t log_buf[256];
	} touch_stat;

/**---------------------------------------------------------------------------*
** 				Function Defination			*
**---------------------------------------------------------------------------*/
	int32_t touch_ae_init(touch_stat * cxt, touch_in * in_touch_ae);
	int32_t touch_ae_calc(touch_stat * cxt);
	int32_t release_touch_ae(touch_stat * cxt, int8_t ae_state, int16_t bv);
	int32_t touch_ae_deinit(touch_stat * cxt);

/**----------------------------------------------------------------------------*
**					Compiler Flag												     *
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif/*  */

#endif/*  */