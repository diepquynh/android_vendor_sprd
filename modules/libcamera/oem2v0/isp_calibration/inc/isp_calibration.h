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
#ifndef _ISP_CALIBRATION_H_
#define _ISP_CALIBRATION_H_

#include <sys/types.h>
#include <utils/Log.h>
//#include "isp_data_type.h"
#include "sensor_drv_u.h"

#ifdef	 __cplusplus
extern	 "C"
{
#endif

#define ISP_CALI_LOG(format,...) ALOGE(DEBUG_STR format, DEBUG_ARGS, ##__VA_ARGS__)

enum {
	ISP_BAYER_PTN_GR = 0,
	ISP_BAYER_PTN_R = 1,
	ISP_BAYER_PTN_B = 2,
	ISP_BAYER_PTN_GB =3,
	ISP_BAYER_PTN_MAX,
};

enum {
	ISP_CALI_RTN_SUCCESS = 0,
	ISP_CALI_RTN_PT_NULL,
	ISP_CALI_RTN_PARAM_INVALID,
	ISP_CALI_RTN_PARAM_UNSUPPRTED,
	ISP_CALI_RTN_ERROR,
	ISP_CALI_RTN_MAX,
};


struct isp_size_t {
	uint16_t width;
	uint16_t height;
};

struct isp_rect_t {
	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;
};


struct isp_addr_t {
	uintptr_t y_addr;
	uintptr_t uv_addr;
	uintptr_t v_addr;
};

struct isp_bayer_ptn_stat_t {
	uint32_t r_stat;
	uint32_t b_stat;
	uint32_t gr_stat;
	uint32_t gb_stat;
};

struct isp_lnc_cali_stat {
	uint32_t max_stat;
	uint32_t *stat_ptr;
};

struct isp_lnc_cali_in {
	uint16_t grid;
	uint16_t width;
	uint16_t height;
	uint16_t *img_ptr;
};

struct isp_lnc_cali_info {
	uint32_t num;
	uint32_t grid_x;
	uint32_t grid_y;
	uint32_t map_width;
	uint32_t map_height;
	uint16_t *tmp_buf0;
	uint16_t *tmp_buf1;
	struct isp_lnc_cali_stat chn_00;
	struct isp_lnc_cali_stat chn_01;
	struct isp_lnc_cali_stat chn_10;
	struct isp_lnc_cali_stat chn_11;
};

struct isp_lnc_cali_gain {
	uint16_t *chn_00;
	uint16_t *chn_01;
	uint16_t *chn_10;
	uint16_t *chn_11;
};

int32_t ISP_Cali_LNCTaleCalc(struct isp_addr_t img_addr, uint32_t bayer_pttn, struct isp_size_t img_size, uint32_t grid, uint16_t *lnc_tab);
int32_t ISP_Cali_Get_Advanced_LensShading(struct isp_addr_t *lnc_tg,
												struct isp_addr_t *lnc_mr,
												struct isp_addr_t *lnc_mg,
												struct isp_addr_t *lnc_final,
												uint32_t size);

uint32_t ISP_Cali_Get_RawRGB_Stat(struct isp_addr_t *img_addr,
					struct isp_rect_t *rect,
					struct isp_size_t *img_size,
					uint32_t byer_pttn,
					struct isp_bayer_ptn_stat_t *stat_param);

uint32_t ISP_Cali_LNCCorrection(struct isp_addr_t * src_data, struct isp_addr_t * dst_data, struct isp_size_t img_size, uint8_t grid, uint16_t *lnc_tab);
void ISP_Cali_GetLNCTabSize(struct isp_size_t img_size, uint32_t grid, uint32_t *tab_size);

int32_t ISP_Cali_BLCorrecton(struct isp_addr_t *in_img_addr,
								struct isp_addr_t *out_img_addr,
								struct isp_rect_t *rect,
								struct isp_size_t *img_size,
								uint32_t  bayer_pttn,
								struct isp_bayer_ptn_stat_t *stat_param
								);

#ifdef	 __cplusplus
}
#endif

#endif
