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
#include "otp_pack.h"
#include "random_pack.h"
#include "lsc_alg.h"
#include "awb_alg.h"
#include  "otp_alg_app.h"
/*------------------------------------------------------------------------------*
				Micro Define													*
*-------------------------------------------------------------------------------*/
#define RANDOM_SUCCESS 	0
#define RANDOM_ERROR     0XFF
#define  COMPRESS  1

/*------------------------------------------------------------------------------*
*				Data Structures													*
*-------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------*
*				Local functions													*
*-------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------*
*				Public functions												*
*-------------------------------------------------------------------------------*/
int32_t otp_random_size(struct otp_pack_random_param *param, uint32_t *real_size)
{
	if (NULL == param || NULL == real_size)
		return RANDOM_ERROR;

	*real_size = 100 * 1024;

	return RANDOM_SUCCESS;
}

int32_t otp_pack_random(struct otp_pack_random_param *param, uint32_t *real_size)
{
	int32_t rtn = RANDOM_ERROR;
	struct random_pack_param pack_param = {0};
	struct random_pack_result pack_result = {0};
	struct lsc_calc_gain_param lsc_calc_param = {0};
	struct lsc_calc_center_param calc_center_param = {0};
	struct lsc_gain_info lsc_gain = {0};
	struct isp_raw_image std_image = {0};
	struct isp_raw_image flash_image = {0};
	struct isp_center gain_center = {0};
	struct img_rgb average_rgb = {0};
	struct isp_img_rect awb_roi = {0};
	uint32_t lsc_gain_width = 0;
	uint32_t lsc_gain_height = 0;
	void *lsc_gain_buf = NULL;
	uint32_t lsc_gain_size = 0;
	uint32_t grid_size = 0;
	uint16_t chn_size=0;
	uint16_t * lsc_gain_chn[4] = {NULL};

	uint16_t * lsc_r_gain = NULL;
	uint16_t * lsc_gr_gain = NULL;
	uint16_t * lsc_gb_gain = NULL;
	uint16_t * lsc_b_gain = NULL;

	uint32_t i=0;
	uint32_t chn_gain_size=0;

	uint16_t * compress_buf = NULL;

	const char file_path[] = "output\\";
	static char pack_name[128] = "";

	if (NULL == param || NULL == real_size)
		return RANDOM_ERROR;

	/*only support raw now*/
	if (param->image_pattern > 3 || 0 != param->image_format)
		return RANDOM_ERROR;

	if (0 == param->image_width || 0 == param->image_height)
		return RANDOM_ERROR;

	if (NULL == param->image_data)
		return RANDOM_ERROR;

	if (param->lsc.grid_width != param->lsc.grid_height) {
		return RANDOM_ERROR;
	}

	grid_size = param->lsc.grid_width;
	std_image.bayer_pattern = param->image_pattern;
	std_image.data_format = param->image_format;
	std_image.width = param->image_width;
	std_image.height = param->image_height;
	std_image.data = param->image_data;

	/*estimate buffer size*/
	rtn = lsc_calc_gain_size(std_image.width, std_image.height, grid_size,
							param->lsc.alg_version, &lsc_gain_width, &lsc_gain_height);
	if (RANDOM_SUCCESS != rtn) {
		goto EXIT;
	}

	lsc_gain_size = lsc_gain_width * lsc_gain_height * sizeof(uint16_t) * LSC_CHN_NUM * 2;

	/*allocate target buffer*/
	lsc_gain_buf = (void *)malloc(lsc_gain_size);
	if (NULL == lsc_gain_buf) {
		goto EXIT;
	}

	lsc_calc_param.target_buf = lsc_gain_buf;
	lsc_calc_param.target_buf_size = lsc_gain_size;
	lsc_calc_param.compress = param->lsc.compress;
	lsc_calc_param.algorithm_version = param->lsc.alg_version;
	lsc_calc_param.center_type = param->lsc.center_type;
	lsc_calc_param.percent = param->lsc.percent;
	lsc_calc_param.grid_size = grid_size;
	lsc_calc_param.image = std_image;
	lsc_calc_param.base_gain = param->base_gain;

	switch (param->lsc.alg_version) {
	case ISP_LSC_ALG_V1:
		rtn = lsc_calc_gain(&lsc_calc_param, &lsc_gain);
		break;

	case ISP_LSC_ALG_V2:
		rtn = lsc_calc_gain_v02(&lsc_calc_param, &lsc_gain);
		break;

	default:
		rtn = RANDOM_ERROR;
		break;
	}

	if (RANDOM_SUCCESS != rtn) {
		goto EXIT;
	}

#ifdef TEST_COMPRESS
	for(i=0;i<LSC_CHN_NUM;i++){
		sprintf(pack_name, "%sgain_not_compress[%d].bin",file_path,i);
		write_data_uint16(pack_name, lsc_gain.chn_gain[i], lsc_gain.chn_size);
	}
#endif

	/*compress gain or not*/
	if(COMPRESS==lsc_calc_param.compress){
		rtn = Calc_chn_size(param->lsc.grid_height, param->image_width,param->image_height,param->lsc.alg_version,& chn_size);
		if (RANDOM_SUCCESS != rtn) {
		goto EXIT;
		}

		compress_buf = (uint16_t *)malloc(chn_size*LSC_CHN_NUM);
		if(NULL == compress_buf){
		goto EXIT;
		}
		memset(compress_buf, 0, chn_size*LSC_CHN_NUM);

		lsc_r_gain = compress_buf;
		lsc_gr_gain = (uint16_t *)((uint8_t *)lsc_r_gain + chn_size);
		lsc_gb_gain = (uint16_t *)((uint8_t *)lsc_gr_gain + chn_size);
		lsc_b_gain = (uint16_t *)((uint8_t *)lsc_gb_gain + chn_size);

		compress_bit14(lsc_gain.gain_width, lsc_gain.gain_height, lsc_r_gain, lsc_gain.chn_gain[0], &chn_gain_size);
		compress_bit14(lsc_gain.gain_width, lsc_gain.gain_height, lsc_gr_gain, lsc_gain.chn_gain[1], &chn_gain_size);
		compress_bit14(lsc_gain.gain_width, lsc_gain.gain_height, lsc_gb_gain, lsc_gain.chn_gain[2], &chn_gain_size);
		compress_bit14(lsc_gain.gain_width, lsc_gain.gain_height, lsc_b_gain, lsc_gain.chn_gain[3], &chn_gain_size);

		lsc_gain.chn_gain[0] = lsc_r_gain;
		lsc_gain.chn_gain[1] = (uint16_t *)((uint8_t *)lsc_gain.chn_gain[0]+chn_size);
		lsc_gain.chn_gain[2] = (uint16_t *)((uint8_t *)lsc_gain.chn_gain[1]+chn_size);
		lsc_gain.chn_gain[3] = (uint16_t *)((uint8_t *)lsc_gain.chn_gain[2]+chn_size);


		lsc_gain.chn_size = chn_size;
	}

#ifdef TEST_COMPRESS
	for(i=0;i<LSC_CHN_NUM;i++){
		sprintf(pack_name, "%sgain_compress[%d].bin",file_path,i);
		write_data_uint16(pack_name, lsc_gain.chn_gain[i], lsc_gain.chn_size);
	}
#endif

	/*calc center*/
	calc_center_param.algorithm_version = param->lsc.alg_version;
	calc_center_param.center_type = param->lsc.center_type;
	calc_center_param.coord_type = ISP_COORD_GRID;
	calc_center_param.grid_size = param->lsc.grid_width;
	calc_center_param.image = std_image;

	rtn = lsc_calc_center(&calc_center_param, &gain_center);
	if (RANDOM_SUCCESS != rtn) {
		goto EXIT;
	}

	/*calculate awb info for standard image*/
	awb_roi.x = param->awb.roi_x;
	awb_roi.y = param->awb.roi_y;
	awb_roi.w = param->awb.roi_width;
	awb_roi.h = param->awb.roi_height;
	rtn = calc_average(&std_image, &awb_roi, &average_rgb);
	if (RANDOM_SUCCESS != rtn) {
		goto EXIT;
	}

	pack_param.awb_info.avg_r = average_rgb.r;
	pack_param.awb_info.avg_g = average_rgb.g;
	pack_param.awb_info.avg_b = average_rgb.b;

	/*pack*/
	pack_param.lsc_info.alg_version = param->lsc.alg_version;
	pack_param.lsc_info.base_gain = param->base_gain;
	pack_param.lsc_info.bayer_pattern = std_image.bayer_pattern;
	pack_param.lsc_info.compress = param->lsc.compress;
	pack_param.lsc_info.grid_width = grid_size;
	pack_param.lsc_info.grid_height = grid_size;
	pack_param.lsc_info.percent = param->lsc.percent;
	pack_param.lsc_info.chn_gain_size = lsc_gain.chn_size;
	pack_param.lsc_info.chn_gain[0] = lsc_gain.chn_gain[0];
	pack_param.lsc_info.chn_gain[1] = lsc_gain.chn_gain[1];
	pack_param.lsc_info.chn_gain[2] = lsc_gain.chn_gain[2];
	pack_param.lsc_info.chn_gain[3] = lsc_gain.chn_gain[3];
	pack_param.lsc_info.center_x = gain_center.x;
	pack_param.lsc_info.center_y = gain_center.y;
	pack_param.lsc_info.gain_width = lsc_gain.gain_width;
	pack_param.lsc_info.gain_height = lsc_gain.gain_height;
	pack_param.lsc_info.img_width = std_image.width;
	pack_param.lsc_info.img_height = std_image.height;

	pack_param.target_buf = param->target_buf;
	pack_param.target_buf_size = param->target_buf_size;

	rtn = random_pack(&pack_param, &pack_result);
	if (RANDOM_SUCCESS != rtn) {
		goto EXIT;
	}

	*real_size = pack_result.real_size;
	rtn = RANDOM_SUCCESS;

EXIT:
	if (NULL != lsc_gain_buf) {
		free (lsc_gain_buf);
		lsc_gain_buf = NULL;
	}

	if(NULL != compress_buf){
		free(compress_buf);
		compress_buf = NULL;
	}

	return rtn;
}

