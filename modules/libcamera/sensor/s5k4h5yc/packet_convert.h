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

/*------------------------------------------------------------------------------*
*				Functions														*
*-------------------------------------------------------------------------------*/
 int32_t awb_package_convert(void *src_data, void *target_buf, uint32_t target_buf_size, uint32_t *real_size);

 int32_t lsc_package_convert(void *src_data, void *target_buf, uint32_t target_buf_size, uint32_t *real_size);
/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/

#endif
// End