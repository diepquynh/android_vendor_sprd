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
#ifndef _ISP_CALIBRATION_LSC_H_
#define _ISP_CALIBRATION_LSC_H_

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
#ifndef ISP_CALIBRATION_MAX_LSC_NUM
#define ISP_CALIBRATION_MAX_LSC_NUM	10
#endif
///////////////////////////////////////////////////////////////////////////////////

/*------------------------------------------------------------------------------*
*				Data Structures					*
*-------------------------------------------------------------------------------*/
enum isp_calibration_lsc_return {
	ISP_CALIBRATION_LSC_SUCCESS = 0,
	ISP_CALIBRATION_LSC_ERROR = 0xff
};

enum isp_calibration_lsc_pattern {
	ISP_CALIBRATION_RGGB = 0,
	ISP_CALIBRATION_GRBG = 1,
	ISP_CALIBRATION_GBRG = 2,
	ISP_CALIBRATION_BGGR = 3,
};

struct isp_calibration_lsc_param {
	/*output lnc per light souce gain*/
	uint16_t *param;
	/*param real size,in and out param*/
	uint32_t size;
	/*color temperature of the light*/
	uint32_t light_ct;
};

struct isp_calibration_lsc_calc_in {
	/*lsc otp data read from sensor*/
	void *otp_data;
	/*size of lsc otp data*/
	uint32_t otp_data_size;

	/*golden sensor data*/
	void *golden_data;
	/*size of golden sensor data*/
	uint32_t golden_data_size;

	/*width of image to use the lsc parameter*/
	uint32_t img_width;
	/*height of image to use the lsc parameter*/
	uint32_t img_height;

	/*target buffer*/
	void *target_buf;
	uint32_t target_buf_size;

	/*bayer pattern of image which decide the lsc gain layout*/
	/*0: gr, 1: r, 2: b, 3: gb*/
	uint32_t lsc_pattern;
};

struct isp_calibration_lsc_setting {
	/*number of lsc parameter*/
	uint32_t lsc_param_num;
	/*length of lsc parameters for each light*/
	uint32_t lsc_param_len[ISP_CALIBRATION_MAX_LSC_NUM];
};

struct isp_calibration_lsc_golden_info{
	/*image width*/
	uint32_t img_width;
	uint32_t img_height;
	/*number of lsc parameter*/
	uint32_t lsc_param_num;
	/*length of lsc parameters for each light*/
	uint32_t lsc_param_len;
	/*golden_std_gain*/
	uint16_t * std_gain_golden;
};

struct isp_calibration_lsc_calc_out {
	/*grid size*/
	uint32_t grid;
	/*width for lsc data*/
	uint32_t width;
	/*height for lsc data*/
	uint32_t height;
	/*num of lsc parameter in lsc_param array according to light source*/
	uint32_t lsc_param_num;
	/*the address and the size of parameter buffer should be set by the caller,
	and then the lsc parameters will be written in the buffer. The size of the buffer
	should be large enough to recieve the lsc data*/
	struct isp_calibration_lsc_param lsc_param[ISP_CALIBRATION_MAX_LSC_NUM];
};

/*------------------------------------------------------------------------------*
*				Functions					*
*-------------------------------------------------------------------------------*/
int32_t isp_calibration_lsc_calc(struct isp_calibration_lsc_calc_in *in_param, struct isp_calibration_lsc_calc_out *out_param);
int32_t isp_calibration_lsc_get_golden_info(void *golden_data, uint32_t golden_data_size, struct isp_calibration_lsc_golden_info *golden_info);

///////////////////////////////////////////////////////////////////////////////////


/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/
#endif
// End
