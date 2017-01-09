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
 #ifndef _ISP_COM_ALG_H_
 #define _ISP_COM_ALG_H_

 #include "isp_type.h"
 #include "isp_com.h"

 enum isp_interp_type {
	ISP_INTERP_UINT8 = 0,
	ISP_INTERP_UINT16 = 1,
	ISP_INTERP_UINT32 = 2,
	ISP_INTERP_INT14 = 3,
	ISP_INTERP_UINT20 = 4,
};

  isp_s32 isp_gamma_adjust(struct isp_gamma_curve_info *src_ptr0,
									struct isp_gamma_curve_info *src_ptr1,
									struct isp_gamma_curve_info *dst_ptr,
									struct isp_weight_value *point_ptr);

  isp_s32 isp_cmc_adjust(isp_u16 src0[9],  isp_u16 src1[9], struct isp_sample_point_info *point_ptr, isp_u16 dst[9]);

 int32_t  isp_cmc_adjust_4_reduce_saturation(isp_u16 src_cmc[9], isp_u16 dst_cmc[9], isp_u32 percent);

 isp_s32 isp_cce_adjust(uint16_t src[9], uint16_t coef[3], uint16_t dst[9], uint16_t base_gain);

 int32_t isp_lsc_adjust(void* lnc0_ptr,void* lnc1_ptr, uint32_t lnc_len, struct isp_weight_value *point_ptr, void* dst_lnc_ptr);

int32_t isp_hue_saturation_2_gain(isp_s32 hue, isp_s32 saturation, struct isp_rgb_gains *gain);

 int32_t isp_interp_data(void *dst, void *src[2], uint16_t weight[2], uint32_t data_num, uint32_t data_type);

 #endif