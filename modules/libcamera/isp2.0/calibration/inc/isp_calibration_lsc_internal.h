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
 #ifndef _ISP_CALIBRATION_LSC_INTERNAL_H_
#define _ISP_CALIBRATION_LSC_INTERNAL_H_

/*------------------------------------------------------------------------------*
*				Dependencies					*
*-------------------------------------------------------------------------------*/
#include "isp_calibration_lsc.h"

#ifdef WIN32
#include <stdio.h>
#define PRINTF printf
#else
#include "isp_log.h"
#define LSC_DEBUG_STR     "OTP LSC: %d, %s: "
#define LSC_DEBUG_ARGS    __LINE__,__FUNCTION__
#define PRINTF(format,...) ALOGE(LSC_DEBUG_STR format, LSC_DEBUG_ARGS, ##__VA_ARGS__)
#endif






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
#define CHANNAL_NUM 4
#define LSC_GOLDEN_MIN_BLOCK 	2
#define LSC_GOLDEN_MAX_BLOCK 	16
#define LSC_GOLDEN_VERSION		1

#define BLOCK_ID_BASIC		0x00020001
#define BLOCK_ID_STD_GAIN	0x00020002
#define BLOCK_ID_DIFF_GAIN	0x00020003

struct lsc_golden_header {
	uint32_t version;
	uint32_t length;
	uint32_t block_num;
};

struct lsc_golden_basic_info {
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

struct lsc_golden_block_info {
	uint32_t id;
	uint32_t offset;
	uint32_t size;
};

struct lsc_golden_gain_info {
	uint32_t ct;
	uint32_t gain_num;
	void *gain;
};

struct lsc_random_info {
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
	uint16_t *gain;
};
///////////////////////////////////////////////////////////////////////////////////

/*------------------------------------------------------------------------------*
*				Data Structures					*
*-------------------------------------------------------------------------------*/
enum isp_calibration_lsc_random_cali_info_table{
	R_VERSION_ID = 0,
	R_IMG_WIDTH = 2,
	R_IMG_HEIGHT = 4,
	R_DATALEN = 6,
	R_CENTER_X = 8,
	R_CENTER_Y = 10,
	R_STANDARD = 12
};

enum isp_calibration_lsc_golden_cali_info_table{
	G_VERSION_LOW_16BITS = 0,
	G_VERSION_HIGH_16BITS = 2,
	G_IMG_WIDTH = 4,
	G_IMG_HEIGHT = 6,
	G_DATALEN_LOW_16BITS = 8,
	G_DATALEN_HIGH_16BITS = 10,
	G_SUB_VERSION_ID = 12,
	G_MODULE_NUM = 16,
	G_DATALEN = 20,
	G_MODULE_INFO = 24,
	G_MODULE_INFO_DELTA = 12,

	G_GRID_SIZE_MASK = 0xfe00,
	G_GRID_SIZE_SHIFT = 9,
	G_COMPRESS_FLAG_MASK = 0x0100,
	G_COMPRESS_FLAG_SHIFT = 8,
	G_DIM_FLAG_MASK = 0x0080,
	G_DIM_FLAG_SHIFT = 7,
	G_MODULE_INFO_MASK = 0x0003,
	G_MODULE_INFO_SHIFT = 0,


	G_PARA_INFO = 0,
	G_STANDARD_INFO = 1,

	G_STANDARD_DATA = 0,
	G_DIFF_DATA = 1,

	G_STANDARDOFFSET = 1,
	G_DIFFOFFSET = 2,

	G_LSC_MODULE = 0x01,
	G_COMPRESS = 1,
	G_NONCOMPRESS = 0,
	G_DIM_1D = 1,
	G_DIM_2D = 2

};

enum isp_calibration_lsc_calc_out_table{
	OUT_STANDARD = 0,
	OUT_NONSTANDARD = 1,
};


struct isp_calibration_lsc_random_cali_info{
	uint16_t img_width;
	uint16_t img_height;
	uint16_t center_x;
	uint16_t center_y;
	uint16_t* std_gain;
	uint32_t lsc_pattern;
	uint32_t compress;
	uint32_t gain_width;
	uint32_t gain_height;
};

struct isp_calibration_lsc_info_from_golden{
	/*color temperature of the light*/
	uint32_t standard_light_ct;
	uint8_t grid_size;
	uint8_t diff_num;
	uint16_t grid_width;
	uint16_t grid_height;
	uint32_t std_gain_golden_size;
	uint16_t* std_gain_golden;
};

struct isp_calibration_lsc_flags{
	uint32_t version;
	uint32_t compress_flag;
	uint32_t alg_type;		// 1: 1D; 2: 2D
	uint32_t alg_version;
	uint32_t base_gain;
	uint32_t percent;		//correction percent
};

struct isp_calibration_lsc_module_info{
	uint32_t id;
	uint32_t datalen;
	uint32_t offset;
};

struct isp_calibration_lsc_predictor_param{
	uint16_t* std_gain;
	uint16_t* diff;
	uint16_t* nonstd_gain;
	uint32_t difflen;// eyery channel keep the same difflen,so the data in diffptr keep the length of 4*difflen
	uint16_t grid_width;
	uint16_t grid_height;
	uint16_t center_x;
	uint16_t center_y;
};

struct isp_calibration_lsc_golden_cali_info{
	uint16_t img_width;
	uint16_t img_height;
	uint8_t grid_size;
	uint16_t center_x;
	uint16_t center_y;
	uint16_t grid_width;
	uint16_t grid_height;
	uint8_t diff_num;
	struct isp_calibration_lsc_param gain[ISP_CALIBRATION_MAX_LSC_NUM];// includes the standard data and nonstandard data
};

int32_t isp_calibration_lsc_golden_parse(void *golden_data, uint32_t golden_size,
					struct isp_calibration_lsc_flags *flag,
					struct isp_calibration_lsc_golden_cali_info *golden_info);

/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/

#endif
// End
