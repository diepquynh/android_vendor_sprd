/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*------------------------------------------------------------------------------*
*				Dependencies													*
*-------------------------------------------------------------------------------*/
#include "otp_cali.h"
#include "awb_alg.h"
#include "lsc_alg.h"
#include "basic_type.h"
/*------------------------------------------------------------------------------*
				Micro Define													*
*-------------------------------------------------------------------------------*/
#define OTP_SUCCESS 			0
#define OTP_ERROR				0xff

#define AWB_VERSION_ID 			0x53502001
#define LSC_VERSION_ID 			0x53501001
#define OTP_LSC_VERSION_V02 	0x00020000

#define COMPRESS_12 			12
#define COMPRESS_14			14
/*------------------------------------------------------------------------------*
*				Local functions													*
*-------------------------------------------------------------------------------*/
/*compress 14 bit value to 12 bit*/
static int32_t compress_bit12(uint16_t width, uint16_t height, uint16_t *dst, uint16_t *src, uint32_t *size)
{
	uint32_t y = 0;
	uint32_t x = 0;
	uint32_t i = 0;
	uint32_t j = 0;
	uint32_t bits_left = 0;
	uint16_t* src_ptr = (uint16_t*)src;
	uint16_t* dst_ptr = (uint16_t*)dst;
	uint32_t bits_buf = 0;
	uint32_t bpp = COMPRESS_12;

	for (y=0; y<height; y++) {

		for (x=0; x<width; x++) {

			/*source range from 1024 to 4095, ignore the lowest 2 bits*/
			uint16_t src_value = src_ptr[i] >> 2;

			bits_buf |= ((src_value) & ((1 << (bpp)) - 1)) << bits_left;
			bits_left += bpp;

			if (bits_left >= 16) {

				dst_ptr[j] = (uint16_t)(bits_buf & 0xffff);
				bits_buf >>= 16;
				bits_left -= 16;
				j++;
			}

			i++;
		}
	}

	if (bits_left > 0)  {
		dst_ptr[j++] = (uint16_t)(bits_buf & 0xffff);
	}

	*size = j;

	return OTP_SUCCESS;
}

int32_t compress_bit14(uint16_t width, uint16_t height, uint16_t *dst, uint16_t *src, uint32_t *size)
{
	uint32_t y = 0;
	uint32_t x = 0;
	uint32_t i = 0;
	uint32_t j = 0;
	uint32_t bits_left = 0;
	uint16_t* src_ptr = (uint16_t*)src;
	uint16_t* dst_ptr = (uint16_t*)dst;
	uint32_t bits_buf = 0;
	uint32_t bpp = COMPRESS_14;

	for (y=0; y<height; y++) {

		for (x=0; x<width; x++) {

			uint16_t src_value = src_ptr[i] & 0x3fff;

			bits_buf |= (src_value)  << bits_left;
			bits_left += bpp;

			if (bits_left >= 16) {

				dst_ptr[j] = (uint16_t)(bits_buf & 0xffff);
				bits_buf >>= 16;
				bits_left -= 16;
				j++;
			}

			i++;
		}
	}

	if (bits_left > 0)  {
		dst_ptr[j++] = (uint16_t)(bits_buf & 0xffff);
	}

	*size = j;

	return OTP_SUCCESS;
}
/*------------------------------------------------------------------------------*
*				Public functions												*
*-------------------------------------------------------------------------------*/
int32_t calcOpticalCenter(void* raw_image,uint32_t raw_width,uint32_t raw_height, uint32_t data_pattern,
					uint32_t data_type, uint32_t version_id, uint32_t grid, uint32_t base_gain,uint32_t corr_per,
					uint32_t center_type, uint16_t* oc_centerx, uint16_t* oc_centery)
{
	int32 rtn = OTP_ERROR;
	struct lsc_calc_center_param param = {0};
	struct isp_center center = {0};

	if (NULL == oc_centerx || NULL == oc_centery)
		return OTP_ERROR;

	if (NULL == raw_image)
		return OTP_ERROR;

	if ((raw_width & 1) > 0 || (raw_height & 1) > 0)
		return OTP_ERROR;

	if (0 != data_type)
		return OTP_ERROR;

	switch (grid) {
	case 32:
	case 64:
	case 96:
	case 128:
		break;

	default:
		return OTP_ERROR;
	}

	switch (version_id) {
	case OTP_LSC_VERSION_V02:
		param.algorithm_version = ISP_LSC_ALG_V2;
		break;

	default:
		param.algorithm_version = ISP_LSC_ALG_V1;
		break;
	}

	switch (center_type) {
	case PIXEL_CENTER:
		param.coord_type = ISP_COORD_IMAGE;
		break;

	case GRID_CENTER:
		param.coord_type = ISP_COORD_GRID;
		break;

	default:
		param.coord_type = ISP_COORD_IMAGE;
		break;
	}

	param.center_type = ISP_PHYSCIAL_CENTER;
	param.grid_size = grid;
	param.image.bayer_pattern = data_pattern;
	param.image.data_format = data_type;
	param.image.width = raw_width;
	param.image.height = raw_height;
	param.image.data = raw_image;

	rtn = lsc_calc_center(&param, &center);
	if (OTP_SUCCESS != rtn) {
		return rtn;
	}

	*oc_centerx = center.x;
	*oc_centery = center.y;

	return OTP_SUCCESS;
}



int32_t Calc_chn_size(uint32_t grid, uint32_t raw_width,uint32_t raw_height,uint32_t alg_version,uint16_t *chn_size)
{
	uint32_t gain_width = 0;
	uint32_t gain_height = 0;
	int32_t rtn = 0;

	rtn = lsc_calc_gain_size(raw_width, raw_height, grid, alg_version, &gain_width, &gain_height);
	if (OTP_SUCCESS != rtn) {
		return rtn;
	}


	*chn_size = (gain_width * gain_height  * COMPRESS_14 + 15) / 16 * 2;

	return OTP_SUCCESS;
}


int32_t getLSCOneChannelSize(uint32_t grid, uint32_t raw_width,uint32_t raw_height,
						uint32_t lsc_algid, uint32_t compress, uint32_t *chnnel_size)
{
	int32_t rtn = 0;
	uint32_t alg_version = 0;
	uint32_t gain_width = 0;
	uint32_t gain_height = 0;

	if (NULL == chnnel_size)
		return OTP_ERROR;

	if ((raw_width & 1) > 0 || (raw_height & 1) > 0)
		return OTP_ERROR;

	switch (grid) {
	case 32:
	case 64:
	case 96:
	case 128:
		break;

	default:
		return OTP_ERROR;
	}

	switch (lsc_algid) {
	case OTP_LSC_VERSION_V02:
		alg_version = ISP_LSC_ALG_V2;
		break;

	default:
		alg_version = ISP_LSC_ALG_V1;
		break;
	}

	rtn = lsc_calc_gain_size(raw_width, raw_height, grid, alg_version, &gain_width, &gain_height);
	if (OTP_SUCCESS != rtn) {
		return rtn;
	}

	if (0 == compress) {
		*chnnel_size = gain_width * gain_height * sizeof(uint16_t);
	} else if (1 == compress) {
		*chnnel_size = (gain_width * gain_height  * COMPRESS_14 + 15) / 16 * 2;
	}else{
		return OTP_ERROR;
	}

	return OTP_SUCCESS;
}

int32_t calcLSC(void* raw_image,uint32_t raw_width,uint32_t raw_height, uint32_t data_pattern,uint32_t data_type,
		uint32_t lsc_algid, uint32_t compress, uint32_t grid, uint32_t base_gain, uint32_t corr_per,
		uint16_t* lsc_r_gain,uint16_t* lsc_gr_gain,uint16_t* lsc_gb_gain, uint16_t* lsc_b_gain, uint32_t* lsc_versionid)
{
	int32_t rtn = OTP_ERROR;
	uint32_t compress_size = 0x00;
	struct lsc_calc_gain_param calc_gain_param = {0};
	struct lsc_gain_info lsc_gain = {0};
	struct isp_raw_image *image = NULL;
	enum isp_center_type center_type = ISP_PHYSCIAL_CENTER;
	uint32_t alg_version = 0;
	uint32_t gain_width = 0;
	uint32_t gain_height = 0;
	uint32_t lsc_gain_size = 0;
	uint32_t lsc_gain_buf = NULL;

	if (NULL == lsc_r_gain || NULL == lsc_gr_gain || NULL == lsc_gb_gain
			|| NULL == lsc_b_gain || NULL == lsc_versionid)
		return OTP_ERROR;

	if (NULL == raw_image)
		return OTP_ERROR;

	if ((raw_width & 1) > 0 || (raw_height & 1) > 0)
		return OTP_ERROR;

	if (0 != data_type)
		return OTP_ERROR;

	switch (grid) {
	case 32:
	case 64:
	case 96:
	case 128:
		break;

	default:
		return OTP_ERROR;
	}

	switch (base_gain) {
	case 1024:
	case 256:
		break;

	default:
		return OTP_ERROR;
	}

	switch (lsc_algid) {
	case OTP_LSC_VERSION_V02:
		alg_version = ISP_LSC_ALG_V2;
		break;

	default:
		alg_version = ISP_LSC_ALG_V1;
		break;
	}

	calc_gain_param.base_gain = base_gain;
	calc_gain_param.algorithm_version = alg_version;
	calc_gain_param.center_type = ISP_OPTICAL_CENTER;
	calc_gain_param.grid_size = grid;
	calc_gain_param.percent = corr_per > 100 ? 100 : corr_per;

	image = &calc_gain_param.image;
	image->bayer_pattern = data_pattern;
	image->data_format = data_type;
	image->width = raw_width;
	image->height = raw_height;
	image->data = raw_image;

	/*calc lsc gain for standard image*/
	rtn = lsc_calc_gain_size(raw_width, raw_height, grid,
							alg_version, &gain_width, &gain_height);
	if (OTP_SUCCESS != rtn) {
		goto EXIT;
	}

	lsc_gain_size = gain_width * gain_height * sizeof(uint16_t) * LSC_CHN_NUM * 2;
	lsc_gain_buf = (void *)malloc(lsc_gain_size);
	if (NULL == lsc_gain_buf) {
		goto EXIT;
	}

	calc_gain_param.target_buf = lsc_gain_buf;
	calc_gain_param.target_buf_size = lsc_gain_size;
	calc_gain_param.image = *image;
	/*do not set center, calculate the center by the image*/
	calc_gain_param.center_x = 0;
	calc_gain_param.center_y = 0;

	rtn = lsc_calc_gain_v02(&calc_gain_param, &lsc_gain);
	if (OTP_SUCCESS != rtn) {
		goto EXIT;
	}

	if (0 == compress) {
		uint32_t chn_gain_size = lsc_gain.gain_width * lsc_gain.gain_height * sizeof(uint16_t);

		memcpy(lsc_gr_gain, lsc_gain.chn_gain[0], chn_gain_size);
		memcpy(lsc_r_gain, lsc_gain.chn_gain[1], chn_gain_size);
		memcpy(lsc_b_gain, lsc_gain.chn_gain[2], chn_gain_size);
		memcpy(lsc_gb_gain, lsc_gain.chn_gain[3], chn_gain_size);
	} else if (1 == compress) {
		uint32_t chn_gain_size = 0;

		compress_bit14(lsc_gain.gain_width, lsc_gain.gain_height, lsc_gr_gain, lsc_gain.chn_gain[0], &chn_gain_size);
		compress_bit14(lsc_gain.gain_width, lsc_gain.gain_height, lsc_r_gain, lsc_gain.chn_gain[1], &chn_gain_size);
		compress_bit14(lsc_gain.gain_width, lsc_gain.gain_height, lsc_b_gain, lsc_gain.chn_gain[2], &chn_gain_size);
		compress_bit14(lsc_gain.gain_width, lsc_gain.gain_height, lsc_gb_gain, lsc_gain.chn_gain[3], &chn_gain_size);
	} else {
		goto EXIT;
	}

	*lsc_versionid = LSC_VERSION_ID;

	rtn = OTP_SUCCESS;

EXIT:

	if (NULL == lsc_gain_buf) {
		free(lsc_gain_buf);
		lsc_gain_buf = NULL;
	}

	return rtn;
}

int32_t calcAWB(void* raw_image, uint32_t raw_width, uint32_t raw_height,
			uint32_t data_pattern, uint32_t data_type, uint32_t trim_width, uint32_t trim_height, int32 trim_x, int32 trim_y,
			uint16_t* awb_r_Mean, uint16_t* awb_g_Mean, uint16_t* awb_b_Mean, uint32_t* awb_versionid)
{
	int32 rtn = 0;
	struct isp_raw_image image = {0};
	struct isp_img_rect roi = {0};
	struct img_rgb avg = {0};

	if (NULL == raw_image)
		return OTP_ERROR;

	if (NULL == awb_r_Mean || NULL == awb_g_Mean
		|| NULL == awb_b_Mean || NULL == awb_versionid)
		return OTP_ERROR;

	if ((raw_width & 1) > 0 || (raw_height & 1) > 0)
		return OTP_ERROR;

	if (0 != data_type)
		return OTP_ERROR;

	image.width = raw_width;
	image.height = raw_height;
	image.bayer_pattern = data_pattern;
	image.data_format = data_type;
	image.data = (void *)raw_image;
	roi.x = (uint32_t)trim_x;
	roi.y = (uint32_t)trim_y;
	roi.w = trim_width;
	roi.h = trim_height;

	rtn = calc_average(&image, &roi, &avg);
	if (OTP_SUCCESS != rtn) {
		return rtn;
	}

	*awb_versionid = AWB_VERSION_ID;
	*awb_r_Mean = avg.r;
	*awb_g_Mean = avg.g;
	*awb_b_Mean = avg.b;

	return OTP_SUCCESS;
}