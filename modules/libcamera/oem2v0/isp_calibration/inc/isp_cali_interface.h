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
#ifndef _ISP_CALI_INTERFACE_H_
#define _ISP_CALI_INTERFACE_H_

#include <sys/types.h>
//#include "isp_data_type.h"
#include "isp_calibration.h"

#ifdef	 __cplusplus
extern	 "C"
{
#endif

int32_t ISP_Cali_GetLensTabs(struct isp_addr_t img_addr,
								uint32_t grid,
								struct isp_size_t img_size,
								uint32_t* lens_tab,
								uint32_t x,
								uint32_t y,
								uint32_t type);

int32_t ISP_Cali_RawRGBStat(struct isp_addr_t *img_addr,
				struct isp_rect_t *rect,
				struct isp_size_t *img_size,
				struct isp_bayer_ptn_stat_t *stat_param);

int32_t ISP_Cali_UnCompressedPacket(struct isp_addr_t src_addr, struct isp_addr_t dst_addr, struct isp_size_t img_size, uint32_t edn_type);

void ISP_Cali_GetLensTabSize(struct isp_size_t img_size, uint32_t grid, uint32_t *tab_size);

uint32_t ISP_Cali_LensCorrection(struct isp_addr_t * src_data, struct isp_addr_t * dst_data, struct isp_size_t img_size, uint8_t grid, uint16_t *lnc_tab);

int32_t ISP_Cali_BlackLevelCorrection(struct isp_addr_t *in_img_addr,
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
