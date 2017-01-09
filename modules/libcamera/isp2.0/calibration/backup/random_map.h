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
 #ifndef _RANDOM_MAP_H_
#define _RANDOM_MAP_H_

/*------------------------------------------------------------------------------*
*				Dependencies					*
*-------------------------------------------------------------------------------*/
#include "isp_otp_type.h"
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
#define RANDOM_LSC_BLOCK_ID 0x0002
#define RANDOM_AWB_BLOCK_ID 0x0003

#define RANDOM_VERSION 1
#define RANDOM_LSC_BLOCK_VERSION 0x53501001
#define RANDOM_AWB_BLOCK_VERSION 0x53502001
#define RANDOM_START 0x53505244
#define RANDOM_END 0x53505244

///////////////////////////////////////////////////////////////////////////////////
struct random_map_lsc {
	uint32_t version;
	uint16_t data_length;
	uint16_t algorithm_version;
	uint16_t compress_flag;
	uint16_t image_width;
	uint16_t image_height;
	uint16_t gain_width;
	uint16_t gain_height;
	uint16_t optical_x;
	uint16_t optical_y;
	uint16_t grid_width;
	uint16_t grid_height;
	uint16_t percent;
	uint16_t bayer_pattern;
	uint16_t gain_num;
};

struct random_map_awb {
	uint32_t version;
	uint16_t avg_r;
	uint16_t avg_g;
	uint16_t avg_b;
	uint16_t reserved;
};

struct random_block_info {
	uint32_t id;
	uint32_t offset;
	uint32_t size;
};

struct random_header {
	uint32_t verify;
	uint32_t size;
	uint32_t version;
	uint32_t block_num;
};
/*------------------------------------------------------------------------------*
*				Data Structures					*
*-------------------------------------------------------------------------------*

/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/

#endif
// End

