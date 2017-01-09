/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _ISP_ALG_H_
#define _ISP_ALG_H_
/*----------------------------------------------------------------------------*
 **				 Dependencies					*
 **---------------------------------------------------------------------------*/
#include <sys/types.h>
#include "isp_ae_alg_v00.h"
#include "isp_app.h"
#include "isp_awb_ctrl.h"

/**---------------------------------------------------------------------------*
 **				 Compiler Flag					*
 **---------------------------------------------------------------------------*/
#ifdef	 __cplusplus
extern	 "C"
{
#endif

/**---------------------------------------------------------------------------*
**				 Micro Define					*
**----------------------------------------------------------------------------*/
#define SC8825_ISP_ID 0x00000000
#define SC8830_ISP_ID 0x00010000
#define SC9630_ISP_ID 0x00020000

#define AE_STAB_NUM 3

#define ISP_SLICE_WIN_NUM 0x0b

#define AE_SMART_DENOISE 0x01
#define AE_SMART_EDGE 0x02
#define AE_SMART_CMC 0x04
#define AE_SMART_LNC 0x08
#define AE_SMART_GAMMA 0x10
#define AE_SMART_CSS 0x20
#define AE_SMART_HW_STA_HUE 0x40
#define AE_SMART_DENOISE_DISWEI 0x80
#define AE_SMART_DENOISE_RANWEI 0x100
#define AE_SMART_DENOISE_PREF_Y 0x200
#define AE_SMART_DENOISE_PREF_UV 0x400
#define AE_SMART_DENOISE_SOFT_Y 0x800
#define AE_SMART_DENOISE_SOFT_UV 0x1000

#define ISP_DENOISE_MAX_LEVEL 0xFE

/**---------------------------------------------------------------------------*
**				Data Prototype					*
**----------------------------------------------------------------------------*/
enum isp_process_type{
	ISP_PROC_BAYER=0x00,
	ISP_PROC_YUV,
	ISP_PROCESS_TYPE_MAX
};

enum isp_slice_type{
	ISP_FETCH=0x00,
	ISP_BNLC,
	ISP_LENS,
	ISP_WAVE,
	ISP_CFA,
	ISP_PREF,
	ISP_BRIGHT,
	ISP_CSS,
	ISP_STORE,
	ISP_FEEDER,
	ISP_GLB_GAIN,
	ISP_SLICE_TYPE_MAX
};

enum isp_slice_pos_info{
	ISP_SLICE_ALL=0x00,
	ISP_SLICE_FIRST,
	ISP_SLICE_MID,
	ISP_SLICE_LAST,
	ISP_SLICE_POS_INFO_MAX
};

enum isp_slice_edge_info{
	ISP_SLICE_RIGHT=0x01,
	ISP_SLICE_LEFT=0x02,
	ISP_SLICE_DOWN=0x04,
	ISP_SLICE_UP=0x08,
	ISP_SLICE_EDGE_INFO_MAX
};



struct isp_pitch{
	uint32_t chn0;
	uint32_t chn1;
	uint32_t chn2;
};


struct isp_slice_param{
	enum isp_slice_pos_info pos_info;
	uint32_t slice_line;
	uint32_t complete_line;
	uint32_t all_line;
	struct isp_size max_size;
	struct isp_size all_slice_num;
	struct isp_size cur_slice_num;

	struct isp_trim_size size[ISP_SLICE_WIN_NUM];
	uint32_t edge_info;
};

struct isp_lnc_map{
	uint32_t grid_mode;
	uint32_t grid_pitch;
	unsigned long param_addr;
	uint32_t len;
};

struct isp_lnc_param{
	uint32_t bypass;
	uint32_t cur_use_buf;
	uint32_t load_buf;
	struct isp_lnc_map map;
	uint32_t* lnc_ptr;
	uint32_t lnc_len;
	struct isp_awb_adjust cur_lnc;
};

struct isp_fetch_param{
	uint32_t bypass;
	uint32_t sub_stract;
	struct isp_addr addr;
	struct isp_pitch pitch;
	uint32_t mipi_word_num;
	uint32_t mipi_byte_rel_pos;
};




int32_t isp_ae_get_real_gain(uint32_t gain);
int32_t isp_ae_fast_smart_adjust(uint32_t handler_id, int32_t cur_ev, uint32_t eb);
int32_t isp_ae_stab_smart_adjust(uint32_t handler_id, int32_t cur_ev);
int32_t isp_get_denoise_tab(uint32_t de_level, void *isp_context_ptr, uint8_t** diswei, uint8_t** ranwei);
int32_t isp_get_denoise_tab_diswei(uint32_t de_level, void *isp_context_ptr, uint8_t** diswei);
int32_t isp_get_denoise_tab_ranwei(uint32_t de_level, void *isp_context_ptr, uint8_t** ranwei);
int32_t isp_flash_calculation(uint32_t handler_id, struct isp_ae_v00_flash_alg_param* v00_flash_ptr);
int32_t isp_adjust_cmc(uint32_t handler_id, int32_t cur_ev);
int32_t _ispGetFetchPitch(struct isp_pitch* pitch_ptr, uint16_t width, enum isp_format format);
int32_t _ispGetStorePitch(struct isp_pitch* pitch_ptr, uint16_t width, enum isp_format format);
int32_t _ispGetSliceHeightNum(struct isp_size* src_size_ptr, struct isp_slice_param* slice_ptr);
int32_t _ispGetSliceWidthNum(struct isp_size* src_size_ptr, struct isp_slice_param* slice_ptr);
int32_t _ispSetSlicePosInfo(struct isp_slice_param* slice_ptr);
int32_t _ispAddSliceBorder(enum isp_slice_type type, enum isp_process_type proc_type, struct isp_slice_param* slice_ptr);
int32_t _ispGetSliceSize(enum isp_process_type proc_type, struct isp_size* src_size_ptr, struct isp_slice_param* slice_ptr);
int32_t _ispGetSliceEdgeInfo(struct isp_slice_param* slice_ptr);
int32_t _ispGetLncAddr(struct isp_lnc_param* param_ptr,struct isp_slice_param* isp_ptr, uint16_t src_width,uint32_t isp_id);
int32_t _ispGetLncCurrectParam(void* lnc0_ptr,void* lnc1_ptr, uint32_t lnc_len, uint32_t alpha, void* dst_lnc_ptr);
int32_t _ispGetFetchAddr(uint32_t handler_id, struct isp_fetch_param* fetch_ptr);
int32_t _ispGetFetchPitch(struct isp_pitch* pitch_ptr, uint16_t width, enum isp_format format);
int32_t _ispGetStorePitch(struct isp_pitch* pitch_ptr, uint16_t width, enum isp_format format);
int32_t _ispGetSliceHeightNum(struct isp_size* src_size_ptr, struct isp_slice_param* slice_ptr);
int32_t _ispGetSliceWidthNum(struct isp_size* src_size_ptr, struct isp_slice_param* slice_ptr);
int32_t _ispSetSlicePosInfo(struct isp_slice_param* slice_ptr);
int32_t _ispAddSliceBorder(enum isp_slice_type type, enum isp_process_type proc_type, struct isp_slice_param* slice_ptr);
int32_t _ispGetSliceSize(enum isp_process_type proc_type, struct isp_size* src_size_ptr, struct isp_slice_param* slice_ptr);
int32_t _ispGetSliceEdgeInfo(struct isp_slice_param* slice_ptr);
uint16_t _ispGetLensGridPitch(uint16_t src_width, uint8_t len_grid, uint32_t isp_id);
int32_t _ispGetFetchAddr(uint32_t handler_id, struct isp_fetch_param* fetch_ptr);
int32_t isp_InterplateCMC(uint32_t handler_id, uint16_t *out, uint16_t *src[2], uint16_t alpha);
int32_t isp_SetCMC_By_Reduce(uint32_t handler_id, uint16_t *cmc_out, uint16_t *cmc_in, int32_t percent, uint8_t *is_update);
void isp_interpolate_lsc(uint16_t *out, uint16_t *src[2], uint16_t weight[2], uint32_t size);
int32_t isp_InterplateCCE(uint32_t handler_id, uint16_t dst[9], uint16_t src[9], uint16_t coef[3], uint16_t base_gain);

/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef	__cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif

