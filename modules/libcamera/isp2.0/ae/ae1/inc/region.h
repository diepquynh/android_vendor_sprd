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
#ifndef _REGION_H_
#define _REGION_H_
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
#define REGION_CFG_NUM AE_CFG_NUM
/**---------------------------------------------------------------------------*
**				Data Structures				*
**---------------------------------------------------------------------------*/
	typedef struct {
		struct ae_range region_thrd[6];/*u d l r */
		int16_t up_max;
		int16_t dwn_max;
		int16_t vote_region[6];/*u d l r */
	} region_cfg;/*16 * 4bytes */

	struct region_tuning_param {
		uint8_t enable;
		uint8_t num;
		uint16_t reserved;	/*1 * 4bytes */
		region_cfg cfg_info[REGION_CFG_NUM];	/*total 8 group: 128 * 4bytes */
		struct ae_piecewise_func input_piecewise;	/*17 * 4bytes */
		struct ae_piecewise_func u_out_piecewise;	/*17 * 4bytes */
		struct ae_piecewise_func d_out_piecewise;	/*17 * 4bytes */
	};/*180 * 4bytes */

	typedef struct {
		uint8_t mlog_en;
		uint8_t * ydata;
		uint8_t * pos_weight;
		int16_t stat_size;
		int16_t match_lv;
		float real_lum;
		float comp_target;
	} region_in;//tuning info

	typedef struct  {
		int16_t tar_offset_u;
		int16_t tar_offset_d;
		int16_t input_interpolation[4];
		float u_strength;
		float d_strength;
		float degree;
		char *log;
	} region_rt;	//result info

	typedef struct  {
		uint8_t enable;
		uint8_t debug_level;
		uint8_t mlog_en;
		struct region_tuning_param tune_param;
		region_in in_region;
		region_rt result_region;
		int8_t region_num;
		float region_lum[10];
		uint32_t log_buf[256];
	} region_stat;

/**---------------------------------------------------------------------------*
** 				Function Defination			*
**---------------------------------------------------------------------------*/
	int32_t region_init(region_stat * cxt, struct region_tuning_param *tune_ptr);
	int32_t region_calc(region_stat * cxt);
	int32_t region_deinit(region_stat * cxt);

/**----------------------------------------------------------------------------*
**					Compiler Flag												     *
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif/*  */
#endif/*  */
