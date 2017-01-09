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
 #ifndef _GOLDEN_MAP_H_
#define _GOLDEN_MAP_H_

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
#define GOLDEN_MODULE_BLOCK_ID		0x0001
#define GOLDEN_LSC_BLOCK_ID			0x0002
#define GOLDEN_AWB_BLOCK_ID			0x0003
/*block id for golden lsc sub block*/
#define GOLDEN_BLOCK_ID_LSC_BASIC			0x00020001
#define GOLDEN_BLOCK_ID_LSC_STD_GAIN		0x00020002
#define GOLDEN_BLOCK_ID_LSC_DIFF_GAIN		0x00020003

#define GOLDEN_VERSION_FLAG 		0x5350
#define GOLDEN_VERSION				((GOLDEN_VERSION_FLAG << 16) | 1)
#define GOLDEN_LSC_BLOCK_VERSION	0x53501001
#define GOLDEN_AWB_BLOCK_VERSION 0x53502001
#define GOLDEN_START				0x53505244
#define GOLDEN_END					0x53505244
/*------------------------------------------------------------------------------*
*				Data Structures					*
*-------------------------------------------------------------------------------*/
struct golden_header {
	uint32_t start;
	uint32_t length;
	uint32_t version;
	uint32_t block_num;
};

struct golden_block_info {
	uint32_t id;
	uint32_t offset;
	uint32_t size;
};

struct golden_map_module {
	uint32_t core_version;
	uint32_t sensor_maker;
	uint32_t year;
	uint32_t month;
	uint32_t module_version;
	uint32_t release_number;
	uint32_t cal_dll_version;
	uint32_t cal_map_version;
	uint32_t reserved[8];
};

/*information of LSC module*/
struct golden_lsc_header {
	uint32_t version;
	uint32_t length;
	uint32_t block_num;
};

struct golden_map_lsc_basic {
	uint16_t base_gain;
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
	uint16_t reserved;
};

struct golden_map_awb {
	uint32_t version;
	uint16_t avg_r;
	uint16_t avg_g;
	uint16_t avg_b;
	uint16_t reserved0;
	uint16_t reserved1[6];
};
/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/

#endif
// End
