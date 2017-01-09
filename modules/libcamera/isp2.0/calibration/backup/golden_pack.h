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
 #ifndef _GOLDEN_PACK_H_
#define _GOLDEN_PACK_H_

/*------------------------------------------------------------------------------*
*				Dependencies					*
*-------------------------------------------------------------------------------*/
#include "basic_type.h"
#include "lsc_alg.h"
/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif
/*------------------------------------------------------------------------------*
				Micro Define					*
*-------------------------------------------------------------------------------*/
///////////////////////////////////////////////////////////////////////////////////

/*------------------------------------------------------------------------------*
*				Data Structures					*
*-------------------------------------------------------------------------------*/
struct golden_module_info {
	uint32_t core_version;
	uint32_t sensor_maker;
	uint32_t year;
	uint32_t month;
	uint32_t module_version;
	uint32_t release_number;
	uint32_t cal_dll_version;
	uint32_t cal_map_version;
};

struct golden_lsc_info {
	/* 1: tshark algorithm; 2: sharkl/tshark2 algorithm  */
	uint32_t alg_version;
	/*1: use 1d diff; 2: use 2d diff*/
	uint32_t alg_type;
	/* 0: normal gain (16 bits for one gain); 1: compress gain (14 bit for one gain)  */
	uint32_t compress;
	uint32_t base_gain;
	/* correction percent: 1-100 */
	uint32_t percent;
	uint32_t grid_width;
	uint32_t grid_height;
	/*0: gr, 1: r, 2: b, 3: gb*/
	uint32_t bayer_pattern;
	uint32_t img_width;
	uint32_t img_height;
	uint32_t gain_width;
	uint32_t gain_height;
	uint32_t center_x;
	uint32_t center_y;
	/*std gain*/
	struct lsc_gain_info std_gain;
	uint32_t std_ct;
	/*nonstd diff*/
	struct lsc_diff_1d_info nonstd_diff[MAX_NONSTD_IMAGE];
	uint32_t nonstd_ct[MAX_NONSTD_IMAGE];
	uint32_t nonstd_num;
};

struct golden_awb_info {
	uint16_t avg_r;
	uint16_t avg_g;
	uint16_t avg_b;
};

//calibration in para
struct golden_pack_param {
	struct golden_module_info module_info;
	struct golden_lsc_info lsc_info;
	struct golden_awb_info awb_info;
	void *target_buf;
	uint32_t target_buf_size;
};

struct golden_pack_result {
	uint32_t real_size;
};
/*------------------------------------------------------------------------------*
*				Functions														*
*-------------------------------------------------------------------------------*/
int32_t golden_pack(struct golden_pack_param *param, struct golden_pack_result *result);
int32_t get_golden_pack_size(struct golden_pack_param *param, uint32_t *size);
/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/

#endif
// End

