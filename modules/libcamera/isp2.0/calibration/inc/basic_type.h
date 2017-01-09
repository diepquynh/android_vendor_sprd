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
#ifndef _BASIC_TYPES_H_
#define _BASIC_TYPES_H_

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
#define MAX_NONSTD_IMAGE 9
#define LSC_CHN_NUM 4
#define DIFF_FIXPOINT 10

#define ISP_SUCCESS	0
#define ISP_ERROR 	0xff
///////////////////////////////////////////////////////////////////////////////////

/*------------------------------------------------------------------------------*
*				Data Structures					*
*-------------------------------------------------------------------------------*/
enum isp_center_type{
	ISP_PHYSCIAL_CENTER = 0,
	ISP_OPTICAL_CENTER = 1
};

enum isp_lsc_alg_version{
	ISP_LSC_ALG_V1 = 1,
	ISP_LSC_ALG_V2 = 2
};

enum isp_lsc_alg_type {
	ISP_LSC_ALG_1D_DIFF = 1,
	ISP_LSC_ALG_2D_DIFF = 2
};

enum isp_coord_type{
	ISP_COORD_IMAGE = 0,
	ISP_COORD_GRID = 1,
};

enum isp_data_format {
	ISP_FORMAT_RAW = 0,
	ISP_FORMAT_MIPI_RAW = 1,
};

enum isp_bayer_pattern {
	ISP_PATTERN_GR = 0,
	ISP_PATTERN_R = 1,
	ISP_PATTERN_B = 2,
	ISP_PATTERN_GB = 3
};

struct isp_center {
	uint32_t x;
	uint32_t y;
};

struct isp_raw_image{
	void *data;
	uint32_t width;
	uint32_t height;
	uint32_t bayer_pattern;
	enum isp_data_format data_format;
};

struct isp_img_rect {
	uint32_t x;
	uint32_t y;
	uint32_t w;
	uint32_t h;
};

/*------------------------------------------------------------------------------*
*				Functions					*
*-------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/

#endif
// End
