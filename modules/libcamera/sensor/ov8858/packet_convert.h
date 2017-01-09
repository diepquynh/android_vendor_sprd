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
#ifndef _PACKET_CONVERT_H_
#define _PACKET_CONVERT_H_


#include "isp_type.h"
/*------------------------------------------------------------------------------*
*				Dependencies					*
*-------------------------------------------------------------------------------*/

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

#define LSC_GAIN_SIZE_PER_CH 20
#define CONVERT_LSC_SUCCESS 0
#define CONVERT_AWB_SUCCESS 0
#define CONVERT_LSC_ERROR 1
#define CONVERT_AWB_ERROR 1
#define LSC_CHN_NUM 4

#define RANDOM_LSC_BLOCK_VERSION 0x53501001

#define GOLDEN_AWB_SIZE	4
#define GOLDEN_LSC_CHANNAL_SIZE 600

#define AWB_CALIBRATION_ENABLE 1
#define LSC_CALIBRATION_ENABLE 0

#define AWB_BLOCK_ID 1
#define LSC_BLOCK_ID 2

#define ISP_VERSION_FLAG 0x5350
#define ISP_GOLDEN_V001	 ((ISP_VERSION_FLAG << 16) | 1)
#define ISP_GOLDEN_LSC_V001	 ((ISP_VERSION_FLAG << 16) | 0x1001)
#define ISP_GOLDEN_AWB_V001	 ((ISP_VERSION_FLAG << 16) | 0x2001)


/************awb************/

struct awb_target_packet {
	uint32_t version_id;
	uint16_t mean_value_R;
	uint16_t mean_value_G;
	uint16_t mean_value_B;
};

struct lsc_source_packet {
	uint32_t center_x;
	uint32_t center_y;
	uint32_t table_size;
	//uint8_t * lsc_data;
};

struct awb_source_packet {
	uint16_t average_R;
	uint16_t average_G;
	uint16_t average_Gb;
	uint16_t average_B;
};
struct awb_ctrl_rgb {
	uint16_t r;
	uint16_t g;
	uint16_t b;
};
struct awb_ctrl_info {
	struct awb_ctrl_rgb gldn_stat_info;
	struct awb_ctrl_rgb rdm_stat_info;
};
struct golden_header {
	uint32_t start;
	uint32_t length;
	uint32_t version;
	uint32_t block_num;
	uint16_t awb_lsc_enable;

};
struct block_info {
	uint32_t id;
	uint32_t offset;
	uint32_t length;
};

struct sensor_golden_data{
	struct golden_header header;
	struct block_info block[3];
	void *data_ptr;
};

/*******************lsc*******************/
struct lsc_target_packet {
	uint32_t version_id;
	uint16_t data_legth;
	uint16_t algorithm_version;
	uint16_t compress;
	uint16_t image_width;
	uint16_t image_height;
	uint16_t gain_width;
	uint16_t gain_height;
	uint16_t center_x;
	uint16_t center_y;
	uint16_t grid_width;
	uint16_t grid_height;
	uint16_t percent;
	uint16_t bayer_pattern;
	uint16_t gain_number;
};

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

struct sensor_lsc_golden_data{
	struct lsc_golden_header header;
	struct lsc_golden_block_info block[5];
	void *data_ptr;
};
/*------------------------------------------------------------------------------*
*				Functions														*
*-------------------------------------------------------------------------------*/
LOCAL int32_t awb_golden_package(void *param);

LOCAL int32_t lsc_golden_package(void *param);

LOCAL cmr_uint golden_data_init();

LOCAL int32_t construct_golden_data(cmr_uint param);

LOCAL int32_t awb_package_convert(void *src_data, void *target_buf, uint32_t target_buf_size, uint32_t *real_size);

LOCAL int32_t lsc_package_convert(void *src_data, void *target_buf, uint32_t target_buf_size, uint32_t *real_size);
/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/

#endif
// End
