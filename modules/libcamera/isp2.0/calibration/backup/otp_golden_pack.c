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
#include "golden_pack.h"
#include "lsc_alg.h"
#include "awb_alg.h"
/*------------------------------------------------------------------------------*
				Micro Define													*
*-------------------------------------------------------------------------------*/
#define GOLDEN_SUCCESS 	0
#define GOLDEN_ERROR     0XFF
#define GOLDEN_DEBUG		0

/*------------------------------------------------------------------------------*
*				Data Structures													*
*-------------------------------------------------------------------------------*/
struct lsc_calc_golden_param {
	struct isp_raw_image std_image;
	//struct isp_raw_image nonstd_image[MAX_NONSTD_IMAGE];
	/*image size/formart/pattern are the same as the standard image*/
	void *nonstd_image[MAX_NONSTD_IMAGE];
	uint32_t nonstd_image_num;
	enum isp_center_type center_type;
	enum isp_lsc_alg_version algorithm_version;
	enum isp_lsc_alg_type algorithm_type;
	uint32_t base_gain;
	uint32_t percent;
	uint32_t grid_size;
	void *target_buf;
	uint32_t target_buf_size;
};

struct lsc_calc_golden_result {
	struct lsc_gain_info std_gain;
	struct lsc_diff_1d_info nonstd_diff[MAX_NONSTD_IMAGE];
};

/*------------------------------------------------------------------------------*
*				Local functions													*
*-------------------------------------------------------------------------------*/
static int32_t _calc_lsc(struct lsc_calc_golden_param *param, struct lsc_calc_golden_result *golden_info)
{
	int32_t rtn = GOLDEN_ERROR;
	struct lsc_calc_gain_param calc_gain_param = {0};
	struct lsc_gain_info std_gain = {0};
	struct lsc_gain_info nonstd_gain = {0};
	uint32_t nonstd_image_num = 0;
	struct isp_raw_image *std_img = NULL;
	struct isp_raw_image nonstd_img = {0};
	struct lsc_calc_center_param calc_center_param = {0};
	struct isp_center grid_center = {0};
	struct isp_center image_center = {0};
	uint32_t lsc_gain_size = 0;
	uint32_t gain_width = 0;
	uint32_t gain_height = 0;
	uint16_t *target_buf = NULL;
	uint32_t target_buf_size = 0;
	uint16_t *nonstd_buf = NULL;
	uint32_t std_center_x = 0;
	uint32_t std_center_y = 0;
	uint32_t i=0;

	if (ISP_LSC_ALG_1D_DIFF != param->algorithm_type)
		goto EXIT;

	target_buf = param->target_buf;
	target_buf_size = param->target_buf_size;

	calc_gain_param.base_gain = param->base_gain;
	calc_gain_param.algorithm_version = param->algorithm_version;
	calc_gain_param.algorithm_type = param->algorithm_type;
	calc_gain_param.center_type = param->center_type;
	calc_gain_param.grid_size = param->grid_size;
	calc_gain_param.percent = param->percent;
	std_img = &param->std_image;

	nonstd_image_num = param->nonstd_image_num;

	rtn = _check_image(std_img);
	if (GOLDEN_SUCCESS != rtn) {
		goto EXIT;
	}

	/*calc lsc gain for standard image*/
	rtn = lsc_calc_gain_size(std_img->width, std_img->height, param->grid_size,
							param->algorithm_version, &gain_width, &gain_height);
	if (GOLDEN_SUCCESS != rtn) {
		goto EXIT;
	}

	lsc_gain_size = gain_width * gain_height * sizeof(uint16_t) * LSC_CHN_NUM * 2;
	if (target_buf_size < lsc_gain_size) {
		goto EXIT;
	}

	calc_gain_param.target_buf = target_buf;
	calc_gain_param.target_buf_size = target_buf_size;
	calc_gain_param.image = param->std_image;
	/*calculate the center by the standard image*/
	calc_gain_param.center_x = 0;
	calc_gain_param.center_y = 0;

	switch (param->algorithm_version) {
	case ISP_LSC_ALG_V1:
		rtn = lsc_calc_gain(&calc_gain_param, &std_gain);
		break;

	case ISP_LSC_ALG_V2:
		rtn = lsc_calc_gain_v02(&calc_gain_param, &std_gain);
		break;

	default:
		rtn = GOLDEN_SUCCESS;
		break;
	}

	if (GOLDEN_SUCCESS != rtn) {
		goto EXIT;
	}

#if GOLDEN_DEBUG
	{
		char save_name[64] = {0};
		uint32_t j = 0;

		for (j=0; j<4; j++) {
			sprintf(save_name, "output\\std_gain_chn[%d].txt", j);
			write_data_uint16_dec(save_name, std_gain.chn_gain[j], std_gain.gain_width,
				std_gain.gain_height * std_gain.gain_width * sizeof(uint16_t));
		}
	}
#endif

	golden_info->std_gain = std_gain;

	target_buf_size -= lsc_gain_size;
	target_buf = (uint16_t *)((uint8 *)target_buf+ lsc_gain_size);

	/*calc grid center*/
	calc_center_param.algorithm_version = param->algorithm_version;
	calc_center_param.center_type = param->center_type;
	calc_center_param.coord_type = ISP_COORD_GRID;
	calc_center_param.grid_size = param->grid_size;
	calc_center_param.image = *std_img;

	rtn = lsc_calc_center(&calc_center_param, &grid_center);
	if (GOLDEN_SUCCESS != rtn) {
		goto EXIT;
	}

	/*calc image center*/
	calc_center_param.algorithm_version = param->algorithm_version;
	calc_center_param.center_type = param->center_type;
	calc_center_param.coord_type = ISP_COORD_IMAGE;
	calc_center_param.grid_size = param->grid_size;
	calc_center_param.image = *std_img;

	rtn = lsc_calc_center(&calc_center_param, &image_center);
	if (GOLDEN_SUCCESS != rtn) {
		goto EXIT;
	}

	/*used to receive nonstandard gain*/
	nonstd_buf = (uint16_t *)malloc(lsc_gain_size);
	if (NULL == nonstd_buf) {
		goto EXIT;
	}

	nonstd_img = *std_img;

	for (i=0; i<nonstd_image_num; i++) {

		struct lsc_calc_diff_param calc_diff_param = {0};
		uint32_t chn_diff_num = 0;
		uint32_t j = 0;

		nonstd_img.data = (void *)param->nonstd_image[i];
		if (NULL == nonstd_img.data) {
			goto EXIT;
		}

		/*calc lsc gain for nonstandard image*/
		calc_gain_param.image = nonstd_img;
		calc_gain_param.target_buf = nonstd_buf;
		calc_gain_param.target_buf_size = lsc_gain_size;
		calc_gain_param.center_x = image_center.x;
		calc_gain_param.center_y = image_center.y;

		switch (param->algorithm_version) {
		case ISP_LSC_ALG_V1:
			rtn = lsc_calc_gain(&calc_gain_param, &nonstd_gain);
			break;

		case ISP_LSC_ALG_V2:
			rtn = lsc_calc_gain_v02(&calc_gain_param, &nonstd_gain);
			break;

		default:
			rtn = GOLDEN_SUCCESS;
			break;
		}

		if (GOLDEN_SUCCESS != rtn) {
			goto EXIT;
		}

#if GOLDEN_DEBUG
		{
			char save_name[64] = {0};

			for (j=0; j<4; j++) {
				sprintf(save_name, "output\\nonstd[%d]_gain_chn[%d].txt", i, j);
				write_data_uint16_dec(save_name, nonstd_gain.chn_gain[j], std_gain.gain_width,
					nonstd_gain.gain_height * nonstd_gain.gain_width * sizeof(uint16_t));
			}
		}
#endif

		/*calc 1d diff*/
		for (j=0; j<LSC_CHN_NUM; j++) {
			calc_diff_param.std_gain = std_gain.chn_gain[j];
			calc_diff_param.nonstd_gain = nonstd_gain.chn_gain[j];
			calc_diff_param.gain_width = std_gain.gain_width;
			calc_diff_param.gain_height = std_gain.gain_height;
			calc_diff_param.target_buf = target_buf;
			calc_diff_param.target_buf_size = target_buf_size;
			calc_diff_param.center_x = grid_center.x;
			calc_diff_param.center_y = grid_center.y;
			rtn = lsc_calc_diff_1d(&calc_diff_param, &chn_diff_num);
			if (GOLDEN_SUCCESS != rtn) {
				goto EXIT;
			}

#if GOLDEN_DEBUG
			{
				char save_name[64] = {0};

					sprintf(save_name, "output\\nonstd[%d]_diff_chn[%d].txt", i, j);
					write_data_uint16_dec(save_name, target_buf, 16, chn_diff_num * sizeof(uint16_t));
			}
#endif

			golden_info->nonstd_diff[i].diff[j] = target_buf;
			target_buf = (uint8_t *)target_buf + chn_diff_num * sizeof(uint16_t);
			target_buf_size -= chn_diff_num;
		}

		golden_info->nonstd_diff[i].chn_num = chn_diff_num;
	}

	rtn = GOLDEN_SUCCESS;

EXIT:

	if (NULL != nonstd_buf) {
		free(nonstd_buf);
		nonstd_buf = NULL;
	}

	return rtn;
}

/*------------------------------------------------------------------------------*
*				Public functions												*
*-------------------------------------------------------------------------------*/
int32_t otp_golden_size(struct otp_pack_golden_param *param, uint32_t *size)
{
	*size = 200 * 1024;
	return GOLDEN_SUCCESS;
}

int32_t otp_pack_golden(struct otp_pack_golden_param *param, uint32_t *real_size)
{
	int32_t rtn = GOLDEN_ERROR;
	struct lsc_calc_golden_param lsc_calc_param = {0};
	struct lsc_calc_golden_result lsc_calc_result = {0};
	struct golden_pack_param pack_param = {0};
	struct golden_lsc_info *pack_lsc_info = NULL;
	struct lsc_calc_center_param calc_center_param = {0};
	struct golden_pack_result pack_result = {0};
	struct isp_center gain_center = {0};
	struct img_rgb average_rgb = {0};
	struct isp_img_rect awb_roi = {0};
	struct isp_raw_image std_img = {0};
	uint32_t lsc_gain_width = 0;
	uint32_t lsc_gain_height = 0;
	void *golden_buf = NULL;
	uint32_t golden_buf_size = 0;
	uint32_t i = 0;


	if (NULL == param || NULL == real_size)
		return GOLDEN_ERROR;

	if (param->nonstd_num >= MAX_NONSTD_IMAGE)
		return GOLDEN_ERROR;

	/*only support raw now*/
	if (param->image_pattern > 3 || 0 != param->image_format)
		return GOLDEN_ERROR;

	if (0 == param->image_width || 0 == param->image_height)
		return GOLDEN_ERROR;

	if (NULL == param->std_img)
		return GOLDEN_ERROR;

	std_img.bayer_pattern = param->image_pattern;
	std_img.data_format = param->image_format;
	std_img.width = param->image_width;
	std_img.height = param->image_height;
	std_img.data = (void *)param->std_img;

	lsc_calc_param.algorithm_version = param->lsc.alg_version;
	lsc_calc_param.algorithm_type = param->lsc.alg_type;
	lsc_calc_param.center_type = param->lsc.center_type;
	lsc_calc_param.grid_size = param->lsc.grid_width;
	lsc_calc_param.percent = param->lsc.percent;
	lsc_calc_param.base_gain = param->base_gain;
	lsc_calc_param.std_image = std_img;
	lsc_calc_param.nonstd_image_num = param->nonstd_num;

	for (i=0; i<param->nonstd_num; i++) {
		lsc_calc_param.nonstd_image[i] = param->nonstd_img[i];
	}

	/*estimate buffer size*/
	rtn = lsc_calc_gain_size(lsc_calc_param.std_image.width, lsc_calc_param.std_image.height,
						lsc_calc_param.grid_size, lsc_calc_param.algorithm_version,
						&lsc_gain_width, &lsc_gain_height);
	if (GOLDEN_SUCCESS != rtn) {
		return rtn;
	}

	golden_buf_size = lsc_gain_width * lsc_gain_height * (lsc_calc_param.nonstd_image_num + 1)
							* sizeof(uint16_t) * LSC_CHN_NUM;

	/*allocate target buffer*/
	golden_buf = (void *)malloc(golden_buf_size);
	if (NULL == golden_buf)
		return GOLDEN_ERROR;

	lsc_calc_param.target_buf = golden_buf;
	lsc_calc_param.target_buf_size = golden_buf_size;

	rtn = _calc_lsc(&lsc_calc_param, &lsc_calc_result);
	if (GOLDEN_SUCCESS != rtn) {
		return rtn;
	}

	/*calc center*/
	calc_center_param.algorithm_version = param->lsc.alg_version;
	calc_center_param.center_type = param->lsc.center_type;
	calc_center_param.coord_type = ISP_COORD_GRID;
	calc_center_param.grid_size = param->lsc.grid_width;
	calc_center_param.image = std_img;

	rtn = lsc_calc_center(&calc_center_param, &gain_center);
	if (GOLDEN_SUCCESS != rtn) {
		goto EXIT;
	}

	/*calculate awb*/
	awb_roi.x = param->awb.roi_x;
	awb_roi.y = param->awb.roi_y;
	awb_roi.w = param->awb.roi_width;
	awb_roi.h = param->awb.roi_height;
	rtn = calc_average(&std_img, &awb_roi, &average_rgb);
	if (GOLDEN_SUCCESS != rtn) {
		goto EXIT;
	}

	pack_param.awb_info.avg_r = average_rgb.r;
	pack_param.awb_info.avg_g = average_rgb.g;
	pack_param.awb_info.avg_b = average_rgb.b;

	/*calculate lsc*/
	pack_lsc_info = &pack_param.lsc_info;
	pack_lsc_info->alg_version = param->lsc.alg_version;
	pack_lsc_info->alg_type = param->lsc.alg_type;
	pack_lsc_info->compress = param->lsc.compress;
	pack_lsc_info->grid_width = param->lsc.grid_width;
	pack_lsc_info->grid_height = param->lsc.grid_height;
	pack_lsc_info->percent = param->lsc.percent;
	pack_lsc_info->base_gain = param->base_gain;

	pack_lsc_info->img_width = std_img.width;
	pack_lsc_info->img_height = std_img.height;
	pack_lsc_info->bayer_pattern = std_img.bayer_pattern;

	pack_lsc_info->center_x = gain_center.x;
	pack_lsc_info->center_y = gain_center.y;
	pack_lsc_info->std_ct = param->std_ct;
	pack_lsc_info->std_gain = lsc_calc_result.std_gain;
	pack_lsc_info->gain_width = lsc_calc_result.std_gain.gain_width;
	pack_lsc_info->gain_height = lsc_calc_result.std_gain.gain_height;
	pack_lsc_info->nonstd_num = param->nonstd_num;

	for (i=0; i<param->nonstd_num; i++) {
		pack_lsc_info->nonstd_ct[i] = param->nonstd_ct[i];
		pack_lsc_info->nonstd_diff[i] = lsc_calc_result.nonstd_diff[i];
	}

	pack_param.module_info.core_version = param->module.core_version;
	pack_param.module_info.sensor_maker = param->module.core_version;
	pack_param.module_info.year = param->module.core_version;
	pack_param.module_info.month = param->module.core_version;
	pack_param.module_info.module_version = param->module.core_version;
	pack_param.module_info.release_number = param->module.core_version;
	pack_param.module_info.cal_dll_version = param->module.cal_dll_version;
	pack_param.module_info.cal_map_version = param->module.cal_map_version;
	pack_param.target_buf = param->target_buf;
	pack_param.target_buf_size = param->target_buf_size;

	rtn = golden_pack(&pack_param, &pack_result);
	if (GOLDEN_SUCCESS != rtn) {
		goto EXIT;
	}

	if (NULL != real_size) {
		*real_size = pack_result.real_size;
	}

	rtn = GOLDEN_SUCCESS;

EXIT:
	if (NULL != golden_buf) {
		free (golden_buf);
		golden_buf = NULL;
	}

	return rtn;
}
