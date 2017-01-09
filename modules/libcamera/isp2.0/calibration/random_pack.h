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
 #ifndef _RANDOM_PACK_H_
#define _RANDOM_PACK_H_

/*------------------------------------------------------------------------------*
*				Dependencies					*
*-------------------------------------------------------------------------------*/
#include "sci_types.h"
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
struct random_lsc_info {
	/* 1: tshark algorithm; 2: sharkl/tshark2 algorithm  */
	uint32_t alg_version;
	/* 0: normal gain (16 bits for one gain); 1: compress gain (14 bit for one gain)  */
	uint32_t compress;
	uint32_t base_gain;
	/* correction percent: 1-100 */
	uint32_t percent;
	uint32_t grid_width;
	uint32_t grid_height;
	/*four channel gain placed one by one*/
	uint16_t *chn_gain[4];
	uint16_t chn_gain_size;
	/*0: gr, 1: r, 2: b, 3: gb*/
	uint32_t bayer_pattern;
	uint32_t img_width;
	uint32_t img_height;
	uint32_t gain_width;
	uint32_t gain_height;
	uint32_t center_x;
	uint32_t center_y;
};

struct random_awb_info
{
	/*average value of std image*/
	uint16_t avg_r;
	uint16_t avg_g;
	uint16_t avg_b;
};

//calibration in para
struct random_pack_param {
	struct random_lsc_info lsc_info;
	struct random_awb_info awb_info;
	void *target_buf;
	uint32_t target_buf_size;
};

struct random_pack_result {
	uint32_t real_size;
};

/*------------------------------------------------------------------------------*
*				Functions														*
*-------------------------------------------------------------------------------*/
int32_t random_pack(struct random_pack_param *param, struct random_pack_result *result);
int32_t get_random_pack_size(uint32_t chn_gain_size, uint32_t *size);
/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/

#endif
// End

