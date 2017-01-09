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
/*------------------------------------------------------------------------------*
*				Dependencies					*
*-------------------------------------------------------------------------------*/
#ifndef _ISP_CALIBRATION_H_
#define _ISP_CALIBRATION_H_

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
#define ISP_CALIBRATION_MAX_LSC_NUM 10

///////////////////////////////////////////////////////////////////////////////////

/*------------------------------------------------------------------------------*
*				Data Structures					*
*-------------------------------------------------------------------------------*/
struct isp_data_t {
	uint32_t size;
	void *data_ptr;
};

struct isp_cali_param {
	struct isp_data_t lsc_otp;
	struct isp_data_t awb_otp;
	struct isp_data_t golden;
	struct isp_data_t target_buf;
	/*0: gr, 1:r, 2: b, 3: gb*/
	uint32_t image_pattern;
};

struct isp_cali_info_t {
	uint32_t size;
};

struct isp_cali_awb_info {
	uint32_t verify[2];
	uint16_t golden_avg[4];
	uint16_t ramdon_avg[4];
};

struct isp_cali_lsc_map{
	uint32_t ct;
	uint32_t width;
	uint32_t height;
	uint32_t grid;
	uint32_t len;
	uint32_t offset;
};

struct isp_cali_lsc_info {
	uint32_t verify[2];
	uint32_t num;
	struct isp_cali_lsc_map map[ISP_CALIBRATION_MAX_LSC_NUM];
	void *data_area;
};

struct isp_cali_awb_gain {
	uint32_t r;
	uint32_t g;
	uint32_t b;
};

struct isp_cali_flash_info {
	struct isp_cali_awb_gain awb_gain;
	struct isp_cali_lsc_info lsc;
};

int32_t isp_calibration_get_info(struct isp_data_t *golden_info, struct isp_cali_info_t *cali_info);

int32_t isp_calibration(struct isp_cali_param *param, struct isp_data_t *result);

int32_t isp_parse_calibration_data(struct isp_data_t *cali_data, struct  isp_data_t *lsc, struct isp_data_t *awb );

int32_t isp_parse_flash_data(struct isp_data_t *flash_data, void *lsc_buf, uint32_t lsc_buf_size, uint32_t image_pattern,
					uint32_t gain_width, uint32_t gain_height, struct isp_cali_awb_gain *awb_gain);

/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/
#endif
// End

