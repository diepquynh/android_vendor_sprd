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
 #include "isp_com_alg.h"
 #include "isp_log.h"
#ifdef WIN32
#include "memory.h"
#include "malloc.h"
#endif
#include "isp_type.h"

#define INTERP_WEIGHT_UNIT 256

 isp_s32 isp_gamma_adjust(struct isp_gamma_curve_info *src_ptr0,
									struct isp_gamma_curve_info *src_ptr1,
									struct isp_gamma_curve_info *dst_ptr,
									struct isp_weight_value *point_ptr)
{

	isp_s32 rtn=ISP_SUCCESS;
	isp_u32 i = 0, tmp  = 0;
	isp_u32 scale_coef = 0;

	if (point_ptr->value[0] == point_ptr->value[1]) {
		memcpy((void*)dst_ptr, src_ptr0, sizeof(struct isp_gamma_curve_info));
	} else {
		scale_coef = point_ptr->weight[0] + point_ptr->weight[1];
		if (0 == scale_coef) {
			ISP_LOGE("weight is error: (%d, %d)\n", point_ptr->weight[0], point_ptr->weight[1]);
			return ISP_ERROR;
		}
		for (i = ISP_ZERO; i < ISP_GAMMA_SAMPLE_NUM; i++) {
			dst_ptr->axis[0][i] = src_ptr0->axis[0][i];
			tmp = (src_ptr0->axis[0][i] * point_ptr->weight[0] + src_ptr1->axis[0][i] * point_ptr->weight[1]) / scale_coef;
			dst_ptr->axis[1][i] = tmp;
		}
	}

	return rtn;
}

 isp_s32 isp_cmc_adjust(isp_u16 src0[9],  isp_u16 src1[9], struct isp_sample_point_info *point_ptr, isp_u16 dst[9])
{
	uint32_t i, scale_coeff;

	if ((NULL == src0) || (NULL == src1) || (NULL == dst)) {
		ISP_LOGE("pointer is invalid: src0;%p, src1:%p, dst:%p\n", src0, src1, dst);
		return ISP_ERROR;
	}

	if ((0 == point_ptr->weight0) && (0 == point_ptr->weight1)) {
		ISP_LOGE("weight are all invalidate\n");
		return ISP_ERROR;
	}

	if (point_ptr->x0 == point_ptr->x1) {
		memcpy((void*)dst, (void*)src0, sizeof(isp_u16) * 9);
	} else {
		if (0 == point_ptr->weight0) {
			memcpy((void*)dst, (void*)src1, sizeof(isp_u16) * 9);
		} else if (0 == point_ptr->weight1) {
			memcpy((void*)dst, (void*)src0, sizeof(isp_u16) * 9);
		} else {
			scale_coeff = point_ptr->weight0 + point_ptr->weight1;
			for (i=0; i<9; i++)
			{
				int32_t x0 = (isp_s16)(src0[i] << 2);
				int32_t x1 = (isp_s16)(src1[i] << 2);
				int32_t x = (x0 * point_ptr->weight0 + x1 * point_ptr->weight1) / scale_coeff;
				dst[i] = (uint16_t)((x >> 2) & 0x3fff);
			}
		}
	}

	return ISP_SUCCESS;
}

isp_s32  isp_cmc_adjust_4_reduce_saturation(isp_u16 cmc_in[9], isp_u16 cmc_out[9], isp_u32 percent)
{
	static const int64_t r2y[9] = {299, 587, 114, -168, -331, 500, 500, -419, -81};
	static const int64_t y2r[9] = {1000, 0, 1402, 1000, -344, -714, 1000, 1772, 0};
	int64_t cmc_matrix[9];
	int64_t matrix_0[9];
	int64_t matrix_1[9];
	uint8_t i=0x00;
	int16_t calc_matrix[9];
	uint16_t *matrix_ptr = PNULL;

	matrix_ptr = (uint16_t*)cmc_out;

	percent = (0 == percent) ? 1 : percent;

	if(255 == percent)
	{
		matrix_ptr[0]=cmc_in[0];
		matrix_ptr[1]=cmc_in[1];
		matrix_ptr[2]=cmc_in[2];
		matrix_ptr[3]=cmc_in[3];
		matrix_ptr[4]=cmc_in[4];
		matrix_ptr[5]=cmc_in[5];
		matrix_ptr[6]=cmc_in[6];
		matrix_ptr[7]=cmc_in[7];
		matrix_ptr[8]=cmc_in[8];

	} else if (0<percent) {
		matrix_0[0]=y2r[0]*255;
		matrix_0[1]=y2r[1]*percent;
		matrix_0[2]=y2r[2]*percent;
		matrix_0[3]=y2r[3]*255;
		matrix_0[4]=y2r[4]*percent;
		matrix_0[5]=y2r[5]*percent;
		matrix_0[6]=y2r[6]*255;
		matrix_0[7]=y2r[7]*percent;
		matrix_0[8]=y2r[8]*percent;

		matrix_1[0]=matrix_0[0]*r2y[0]+matrix_0[1]*r2y[3]+matrix_0[2]*r2y[6];
		matrix_1[1]=matrix_0[0]*r2y[1]+matrix_0[1]*r2y[4]+matrix_0[2]*r2y[7];
		matrix_1[2]=matrix_0[0]*r2y[2]+matrix_0[1]*r2y[5]+matrix_0[2]*r2y[8];
		matrix_1[3]=matrix_0[3]*r2y[0]+matrix_0[4]*r2y[3]+matrix_0[5]*r2y[6];
		matrix_1[4]=matrix_0[3]*r2y[1]+matrix_0[4]*r2y[4]+matrix_0[5]*r2y[7];
		matrix_1[5]=matrix_0[3]*r2y[2]+matrix_0[4]*r2y[5]+matrix_0[5]*r2y[8];
		matrix_1[6]=matrix_0[6]*r2y[0]+matrix_0[7]*r2y[3]+matrix_0[8]*r2y[6];
		matrix_1[7]=matrix_0[6]*r2y[1]+matrix_0[7]*r2y[4]+matrix_0[8]*r2y[7];
		matrix_1[8]=matrix_0[6]*r2y[2]+matrix_0[7]*r2y[5]+matrix_0[8]*r2y[8];

		matrix_0[0]=cmc_in[0];
		matrix_0[1]=cmc_in[1];
		matrix_0[2]=cmc_in[2];
		matrix_0[3]=cmc_in[3];
		matrix_0[4]=cmc_in[4];
		matrix_0[5]=cmc_in[5];
		matrix_0[6]=cmc_in[6];
		matrix_0[7]=cmc_in[7];
		matrix_0[8]=cmc_in[8];

		for(i=0x00; i<9; i++)
		{
			if(0!=(matrix_0[i]&0x2000))
			{
				matrix_0[i]|=0xffffffffffffc000;
			}
		}

		cmc_matrix[0]=matrix_1[0]*matrix_0[0]+matrix_1[1]*matrix_0[3]+matrix_1[2]*matrix_0[6];
		cmc_matrix[1]=matrix_1[0]*matrix_0[1]+matrix_1[1]*matrix_0[4]+matrix_1[2]*matrix_0[7];
		cmc_matrix[2]=matrix_1[0]*matrix_0[2]+matrix_1[1]*matrix_0[5]+matrix_1[2]*matrix_0[8];
		cmc_matrix[3]=matrix_1[3]*matrix_0[0]+matrix_1[4]*matrix_0[3]+matrix_1[5]*matrix_0[6];
		cmc_matrix[4]=matrix_1[3]*matrix_0[1]+matrix_1[4]*matrix_0[4]+matrix_1[5]*matrix_0[7];
		cmc_matrix[5]=matrix_1[3]*matrix_0[2]+matrix_1[4]*matrix_0[5]+matrix_1[5]*matrix_0[8];
		cmc_matrix[6]=matrix_1[6]*matrix_0[0]+matrix_1[7]*matrix_0[3]+matrix_1[8]*matrix_0[6];
		cmc_matrix[7]=matrix_1[6]*matrix_0[1]+matrix_1[7]*matrix_0[4]+matrix_1[8]*matrix_0[7];
		cmc_matrix[8]=matrix_1[6]*matrix_0[2]+matrix_1[7]*matrix_0[5]+matrix_1[8]*matrix_0[8];

		cmc_matrix[0]=cmc_matrix[0]/1000/1000/255;
		cmc_matrix[1]=cmc_matrix[1]/1000/1000/255;
		cmc_matrix[2]=cmc_matrix[2]/1000/1000/255;
		cmc_matrix[3]=cmc_matrix[3]/1000/1000/255;
		cmc_matrix[4]=cmc_matrix[4]/1000/1000/255;
		cmc_matrix[5]=cmc_matrix[5]/1000/1000/255;
		cmc_matrix[6]=cmc_matrix[6]/1000/1000/255;
		cmc_matrix[7]=cmc_matrix[7]/1000/1000/255;
		cmc_matrix[8]=cmc_matrix[8]/1000/1000/255;

		calc_matrix[0]=(int16_t)(cmc_matrix[0]&0x3fff);
		calc_matrix[1]=(int16_t)(cmc_matrix[1]&0x3fff);
		calc_matrix[2]=(int16_t)(cmc_matrix[2]&0x3fff);
		calc_matrix[3]=(int16_t)(cmc_matrix[3]&0x3fff);
		calc_matrix[4]=(int16_t)(cmc_matrix[4]&0x3fff);
		calc_matrix[5]=(int16_t)(cmc_matrix[5]&0x3fff);
		calc_matrix[6]=(int16_t)(cmc_matrix[6]&0x3fff);
		calc_matrix[7]=(int16_t)(cmc_matrix[7]&0x3fff);
		calc_matrix[8]=(int16_t)(cmc_matrix[8]&0x3fff);

		matrix_ptr[0]=calc_matrix[0];
		matrix_ptr[1]=calc_matrix[1];
		matrix_ptr[2]=calc_matrix[2];
		matrix_ptr[3]=calc_matrix[3];
		matrix_ptr[4]=calc_matrix[4];
		matrix_ptr[5]=calc_matrix[5];
		matrix_ptr[6]=calc_matrix[6];
		matrix_ptr[7]=calc_matrix[7];
		matrix_ptr[8]=calc_matrix[8];
	}

	return 0;
}

isp_s32 isp_cce_adjust(uint16_t src[9], uint16_t coef[3], uint16_t dst[9], uint16_t base_gain)
{
	isp_s32 matrix[3][3];
	isp_s32 *matrix_ptr = NULL;
	isp_u16 *src_ptr = NULL;
	isp_u16 *dst_ptr = NULL;
	isp_s32 tmp = 0;
	isp_u32 i = 0, j = 0;

	if ((NULL == src) || (NULL == dst) || (NULL == coef)) {
		ISP_LOGE("--isp_InterplateCCE:invalid addr %p, %p, %p\n", src, dst, coef);
		return ISP_ERROR;
	}

	src_ptr = (uint16_t*)src;
	matrix_ptr = (int32_t*)&matrix[0][0];
	for (i=0x00; i<9; i++) {
		if (ISP_ZERO!=(src_ptr[i]&0x8000)) {
			*(matrix_ptr + i) = (src_ptr[i]) | 0xffff0000;
		} else {
			*(matrix_ptr + i) = src_ptr[i];
		}
	}

	dst_ptr = (uint16_t*)dst;
	for (i = 0; i < 3; ++i) {
		for (j = 0; j < 3; ++j) {

			if (coef[j] == base_gain)
				tmp = matrix[i][j];
			else
				tmp = (((int32_t)coef[j]) * matrix[i][j] + base_gain / 2) / base_gain;

			*dst_ptr = (uint16_t)(((uint32_t)tmp) & 0xffff);
			dst_ptr++;
		}
	}

	return ISP_SUCCESS;
}

int32_t isp_lsc_adjust(void* lnc0_ptr,void* lnc1_ptr, uint32_t lnc_len, struct isp_weight_value *point_ptr, void* dst_lnc_ptr)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i = 0x00;
	isp_u32 handler_id = 0;


	isp_u16 *src0_ptr = (isp_u16*)lnc0_ptr;
	isp_u16 *src1_ptr = (isp_u16*)lnc1_ptr;
	isp_u16 *dst_ptr = (isp_u16*)dst_lnc_ptr;

	if ((NULL == lnc0_ptr)
		||(NULL == dst_lnc_ptr)) {
		ISP_LOGE("lnc buf null %p, %p, %p error\n", lnc0_ptr, lnc1_ptr, dst_lnc_ptr);
		return ISP_ERROR;
	}

	if ((0 == point_ptr->weight[0]) && (0 == point_ptr->weight[1])) {
		ISP_LOGE("weight is error: (%d, %d) \n", point_ptr->weight[0], point_ptr->weight[1]);
		return ISP_ERROR;
	}

	if (point_ptr->value[0] == point_ptr->value[1]) {
		memcpy(dst_lnc_ptr, lnc0_ptr, lnc_len);
	} else {
		if (0 == point_ptr->weight[0]) {
			memcpy(dst_lnc_ptr, lnc1_ptr, lnc_len);
		} else if (0 == point_ptr->weight[1]) {
			memcpy(dst_lnc_ptr, lnc0_ptr, lnc_len);
		} else {
			isp_u32 lnc_num = lnc_len / sizeof(uint16_t);
			isp_u32 scale_coeff = 0;

			ISP_LOGI("lnc interpolation: index(%d, %d); weight: (%d, %d), num=%d, src0 = %p, src1 = %p",\
				point_ptr->value[0], point_ptr->value[1], point_ptr->weight[0], point_ptr->weight[1], lnc_num, src0_ptr, src1_ptr);

			scale_coeff= point_ptr->weight[0] + point_ptr->weight[1];

			for (i = 0x00; i<lnc_num; i++) {
				dst_ptr[i] = (src0_ptr[i] * point_ptr->weight[0] + src1_ptr[i] * point_ptr->weight[1]) / scale_coeff;
			}
		}
	}

	return rtn;
}

int32_t isp_hue_saturation_2_gain(isp_s32 hue, isp_s32 saturation,
						struct isp_rgb_gains *gain)
{
	int32_t rtn = ISP_SUCCESS;

	gain->gain_r = ISP_FIX_10BIT_UNIT;
	gain->gain_g = ISP_FIX_10BIT_UNIT;
	gain->gain_b = ISP_FIX_10BIT_UNIT;

	if (hue < 1 || hue > 60 || saturation > 100 || saturation < 1) {
		return rtn;
	}

	gain->gain_b = (isp_u32)((6000 - 60 * saturation) * ISP_FIX_10BIT_UNIT
			/ (6000 + hue * saturation - 60 * saturation));
	gain->gain_g = (isp_u32)ISP_FIX_10BIT_UNIT;
	gain->gain_r = (isp_u32)(((hue - 60) * gain->gain_b + 60 * ISP_FIX_10BIT_UNIT) / hue);

	ISP_LOGV("gain = (%d, %d, %d)", gain->gain_r, gain->gain_g, gain->gain_b);

	return rtn;
}

static void _interp_uint8(uint8_t *dst, uint8_t *src[2], uint16_t weight[2], uint32_t data_num)
{
	uint32_t data_bytes = 0;
	uint32_t i = 0;

	data_bytes = data_num * sizeof(uint8_t);

	if (INTERP_WEIGHT_UNIT == weight[0]) {
		memcpy(dst, src[0], data_bytes);
	} else if (INTERP_WEIGHT_UNIT == weight[1]) {
		memcpy(dst, src[1], data_bytes);
	} else {

		for (i=0; i<data_num; i++) {

			uint32_t dst_val = 0;
			uint32_t src0_val = *src[0]++;
			uint32_t src1_val = *src[1]++;

			dst_val = (src0_val * weight[0] + src1_val * weight[1]) / INTERP_WEIGHT_UNIT;

			*dst++ = dst_val;
		}
	}
}

static void _interp_uint16(uint16_t *dst, uint16_t *src[2], uint16_t weight[2], uint32_t data_num)
{
	uint32_t data_bytes = 0;
	uint32_t i = 0;

	data_bytes = data_num * sizeof(uint16_t);

	if (INTERP_WEIGHT_UNIT == weight[0]) {
		memcpy(dst, src[0], data_bytes);
	} else if (INTERP_WEIGHT_UNIT == weight[1]) {
		memcpy(dst, src[1], data_bytes);
	} else {

		for (i=0; i<data_num; i++) {

			uint32_t dst_val = 0;
			uint32_t src0_val = *src[0]++;
			uint32_t src1_val = *src[1]++;

			dst_val = (src0_val * weight[0] + src1_val * weight[1]) / INTERP_WEIGHT_UNIT;

			*dst++ = dst_val;
		}
	}
}

static void _interp_uint32(uint32_t *dst, uint32_t *src[2], uint16_t weight[2], uint32_t data_num)
{
	uint32_t data_bytes = 0;
	uint32_t i = 0;

	data_bytes = data_num * sizeof(uint32_t);

	if (INTERP_WEIGHT_UNIT == weight[0]) {
		memcpy(dst, src[0], data_bytes);
	} else if (INTERP_WEIGHT_UNIT == weight[1]) {
		memcpy(dst, src[1], data_bytes);
	} else {

		for (i=0; i<data_num; i++) {

			uint32_t dst_val = 0;
			uint32_t src0_val = *src[0]++;
			uint32_t src1_val = *src[1]++;

			dst_val = (src0_val * weight[0] + src1_val * weight[1]) / INTERP_WEIGHT_UNIT;

			*dst++ = dst_val;
		}
	}
}

/*special data type for CMC*/
static void _interp_int14(uint16_t *dst, uint16_t *src[2], uint16_t weight[2], uint32_t data_num)
{
	uint32_t data_bytes = 0;
	uint32_t i = 0;

	data_bytes = data_num * sizeof(uint16_t);

	if (INTERP_WEIGHT_UNIT == weight[0]) {
		memcpy(dst, src[0], data_bytes);
	} else if (INTERP_WEIGHT_UNIT == weight[1]) {
		memcpy(dst, src[1], data_bytes);
	} else {

		for (i=0; i<data_num; i++) {

			uint16_t dst_val = 0;
			int16_t src0_val = (int16_t)(*src[0] << 2);
			int16_t src1_val = (int16_t)(*src[1] << 2);

			dst_val = (src0_val * weight[0] + src1_val * weight[1]) / INTERP_WEIGHT_UNIT;

			*dst++ = (uint16_t)((dst_val >> 2) & 0x3fff);
			src[0]++;
			src[1]++;
		}
	}
}

static void _interp_uint20(uint32_t *dst, uint32_t *src[2], uint16_t weight[2], uint32_t data_num)
{
	uint32_t data_bytes = 0;
	uint32_t i = 0;

	data_bytes = data_num * sizeof(uint32_t);

	if (INTERP_WEIGHT_UNIT == weight[0]) {
		memcpy(dst, src[0], data_bytes);
	} else if (INTERP_WEIGHT_UNIT == weight[1]) {
		memcpy(dst, src[1], data_bytes);
	} else {

		for (i=0; i<data_num; i++) {

			uint32_t dst_val = 0;
			uint32_t src0_val = (uint32_t)(*src[0] << 12);
			uint32_t src1_val = (uint32_t)(*src[1] << 12);

			dst_val = (src0_val * weight[0] + src1_val * weight[1]) / INTERP_WEIGHT_UNIT;

			*dst++ = (uint32_t)((dst_val >> 12) & 0xfffff);
			src[0]++;
			src[1]++;
		}
	}
}


int32_t isp_interp_data(void *dst, void *src[2], uint16_t weight[2], uint32_t data_num, uint32_t data_type)
{
	int32_t rtn = ISP_SUCCESS;

	ISP_LOGV("src[0]=%p, src[1]=%p, dst=%p, weight=(%d, %d), data_num=%d, type=%d",
			src[0], src[1], dst, weight[0], weight[1], data_num, data_type);

	if (NULL == dst || NULL == src[0] || NULL == src[1] || 0 == data_num) {
		ISP_LOGE("invalid param: dst=%p, src=(%p, %p), data_num=%d\n", dst, src[0], src[1], data_num);
		return ISP_ERROR;
	}

	//for speedup
	if (256 != weight[0] + weight[1]) {
		ISP_LOGE("invalid weight: (%d, %d)\n", weight[0], weight[1]);
		return ISP_ERROR;
	}

	switch (data_type) {
	case ISP_INTERP_UINT8:
		_interp_uint8((uint8_t *)dst, (uint8_t **)src, weight, data_num);
		break;

	case ISP_INTERP_UINT16:
		_interp_uint16((uint16_t *)dst, (uint16_t **)src, weight, data_num);
		break;

	case ISP_INTERP_UINT32:
		_interp_uint32((uint32_t *)dst, (uint32_t **)src, weight, data_num);
		break;

	case ISP_INTERP_INT14:
		 _interp_int14((uint16_t *)dst, (uint16_t **)src, weight, data_num);
		break;

	case ISP_INTERP_UINT20:
		_interp_uint20((uint32_t *)dst, (uint32_t **)src, weight, data_num);
		break;

	default:
		rtn = ISP_ERROR;
		break;
	}

	return rtn;
}

int32_t isp_scaling_lsc_gain(uint16_t *dst, uint16_t *src, struct isp_size *dst_size, struct isp_size *src_size)
{
	int32_t rtn = ISP_SUCCESS;
	uint16_t *dst_line = dst;
	uint32_t i = 0;
	uint32_t j = 0;
	uint32_t x_scalar = 0;
	uint32_t y_scalar = 0;

	if (NULL == dst || NULL == src || NULL == dst_size || NULL == src_size)
		return ISP_ERROR;

	if (0 == dst_size->w || 0 == dst_size->h
		|| 0 == src_size->w || 0 == src_size->h)
		return ISP_ERROR;

	x_scalar = src_size->w * (1 << 16) / dst_size->w;
	y_scalar = src_size->h * (1 << 16) / dst_size->h;

	/*for channel interlaced*/
	for (i=0; i<dst_size->h; i++) {

		uint32_t src_y = y_scalar * i;
		uint32_t v = (src_y & (0xffff)) >> 8;
		uint16_t *src_line[2] = {NULL};

		src_y >>= 16;

		if (src_y < dst_size->h - 1) {

			src_line[0] = src + src_y * src_size->w;
			src_line[1] = src_line[0] + src_size->w;
		} else {

			src_y = dst_size->h - 1;
			src_line[0] = src + src_y * src_size->w;
			src_line[1] = src_line[0];
		}

		for (j=0; j<dst_size->w; j++) {

			uint32_t src_x = x_scalar * j;
			uint32_t u = (src_x & (0xffff)) >> 8;
			uint32_t src_l[4] = {0};
			uint32_t weight_value[4] = {0};

			src_x >>= 16;

			if (src_x < dst_size->w - 1) {
				src_l[0] = *(src_line[0] +src_x);
				src_l[1] = *(src_line[0] + src_x + 1);
				src_l[2] = *(src_line[1] +src_x);
				src_l[3] = *(src_line[1] +src_x + 1);
			} else {
				src_x = dst_size->w - 1;
				src_l[0] =*(src_line[0] + src_x);
				src_l[1] = src_l[0];
				src_l[2] =*(src_line[1] + src_x);
				src_l[3] = src_l[2];
			}

			dst_line[j] = (src_l[0] * (256 - u) * (256 - v) + src_l[1] * u * (256 - v)
				+ src_l[2] * (256 - u) * v + src_l[3] * u * v) / 65536;
		}

		dst_line += dst_size->w;
	}

	return rtn;
}