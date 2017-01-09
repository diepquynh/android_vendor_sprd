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

#include "isp_drv.h"

#define YUV_OVERLAP_UP         3
#define YUV_OVERLAP_DOWN       2
#define YUV_OVERLAP_LEFT       6
#define YUV_OVERLAP_RIGHT      4
#define RAW_OVERLAP_UP         11
#define RAW_OVERLAP_DOWN       10
#define RAW_OVERLAP_LEFT       14
#define RAW_OVERLAP_RIGHT      12

#define YUV_OVERLAP_UP_V1      3
#define YUV_OVERLAP_DOWN_V1    2
#define YUV_OVERLAP_LEFT_V1    6
#define YUV_OVERLAP_RIGHT_V1   4
#define RAW_OVERLAP_UP_V1      27
#define RAW_OVERLAP_DOWN_V1    28
#define RAW_OVERLAP_LEFT_V1    56
#define RAW_OVERLAP_RIGHT_V1   56

#define ISP_MIN(x,y) (((x) > (y)) ? (y) : (x))

typedef isp_s32 (*isp_cfg_fun_ptr)(isp_handle handle, void *param_ptr);

struct isp_cfg_fun {
	isp_u32 sub_block;
	isp_cfg_fun_ptr cfg_fun;
};

	/************************************Tshark2*******************************************************/
static struct isp_cfg_fun s_isp_cfg_fun_tab_tshark2[] = {
	{ISP_BLK_PRE_GBL_GAIN_V1,  isp_u_pgg_block},
	{ISP_BLK_BLC_V1,           isp_u_blc_block},
	{ISP_BLK_RGB_GAIN_V1,      isp_u_rgb_gain_block},
	{ISP_BLK_NLC_V1,           isp_u_nlc_block},
	{ISP_BLK_2D_LSC,           isp_u_2d_lsc_block},
	{ISP_BLK_1D_LSC,           isp_u_1d_lsc_block},
	{ISP_BLK_BINNING4AWB_V1,   isp_u_binning4awb_block},
	{ISP_BLK_AWB_V1,           isp_u_awb_block},
	{ISP_BLK_BPC_V1,           isp_u_bpc_block},
	{ISP_BLK_AE_V1,            isp_u_raw_aem_block},
	{ISP_BLK_GRGB_V1,          isp_u_grgb_block},
	{ISP_BLK_RGB_GAIN2,        isp_u_rgb_gain2_block},
	{ISP_BLK_CFA_V1,           isp_u_cfa_block},
	{ISP_BLK_CMC10,            isp_u_cmc_block},
	{ISP_BLK_CTM,              isp_u_ct_block},
	{ISP_BLK_RGB_GAMC,         isp_u_gamma_block},
	{ISP_BLK_CCE_V1,           isp_u_cce_matrix_block},
	{ISP_BLK_RADIAL_CSC,       isp_u_csc_block},
	{ISP_BLK_RGB_PRECDN,       isp_u_frgb_precdn_block},
	{ISP_BLK_POSTERIZE,        isp_u_posterize_block},
	{ISP_BLK_YUV_PRECDN,       isp_u_yuv_precdn_block},
	{ISP_BLK_PREF_V1,          isp_u_prefilter_block},
	{ISP_BLK_BRIGHT,           isp_u_brightness_block},
	{ISP_BLK_CONTRAST,         isp_u_contrast_block},
	{ISP_BLK_AUTO_CONTRAST_V1, isp_u_aca_block},
	{ISP_BLK_UV_CDN,           isp_u_yuv_cdn_block},
	{ISP_BLK_EDGE_V1,          isp_u_edge_block},
	{ISP_BLK_EMBOSS_V1,        isp_u_emboss_block},
	{ISP_BLK_SATURATION_V1,    isp_u_csa_block},
	{ISP_BLK_HUE_V1,           isp_u_hue_block},
	{ISP_BLK_UV_POSTCDN,       isp_u_yuv_postcdn_block},
	{ISP_BLK_Y_GAMMC,          isp_u_ygamma_block},
	{ISP_BLK_YDELAY,           isp_u_ydelay_block},
	{ISP_BLK_NLM,              isp_u_nlm_block},
	{ISP_BLK_UVDIV_V1,         isp_u_cce_uv_block},
	{ISP_BLK_AF_V1,            isp_u_raw_afm_block},
	{ISP_BLK_HSV,              isp_u_hsv_block},
	{ISP_BLK_PRE_WAVELET_V1,   isp_u_pwd_block},
	{ISP_BLK_BL_NR_V1,         isp_u_bdn_block},
	{ISP_BLK_CSS_V1,           isp_u_css_block},
	{ISP_BLK_CMC8,             isp_u_cmc8_block},
	{ISP_BLK_IIRCNR_IIR,       isp_u_iircnr_block},
	{ISP_BLK_IIRCNR_YRANDOM,   isp_u_yrandom_block},
	{ISP_BLK_YIQ_AFM,          isp_u_yiq_afm_block},
	{ISP_BLK_YIQ_AEM,          isp_u_yiq_aem_block},
	{ISP_BLK_HIST_V1,          isp_u_hist_block},
	{ISP_BLK_HIST2,            isp_u_hist2_block},
	{ISP_BLK_YIQ_AFL,          isp_u_anti_flicker_block},
};

	/************************************pike*******************************************************/
static struct isp_cfg_fun s_isp_cfg_fun_tab_pike[] = {
	{ISP_BLK_PRE_GBL_GAIN_V1,  isp_u_pgg_block},
	{ISP_BLK_BLC_V1,           isp_u_blc_block},
	{ISP_BLK_RGB_GAIN_V1,      isp_u_rgb_gain_block},
	{ISP_BLK_PRE_WAVELET_V1,   isp_u_pwd_block},//different from tshark2
	{ISP_BLK_NLC_V1,           isp_u_nlc_block},
	{ISP_BLK_2D_LSC,           isp_u_2d_lsc_block},
	{ISP_BLK_1D_LSC,           isp_u_1d_lsc_block},
	{ISP_BLK_BINNING4AWB_V1,   isp_u_binning4awb_block},
	{ISP_BLK_AWB_V1,           isp_u_awb_block},//different
	{ISP_BLK_AE_V1,            isp_u_raw_aem_block},
	{ISP_BLK_BPC_V1,           isp_u_bpc_block},
	{ISP_BLK_BL_NR_V1,         isp_u_bdn_block},
	{ISP_BLK_GRGB_V1,          isp_u_grgb_block},
	{ISP_BLK_RGB_GAIN2,        isp_u_rgb_gain2_block},//different
	{ISP_BLK_NLM,              isp_u_nlm_block},// nlm include VST and nlm module, vst same,but nlm different
	{ISP_BLK_CFA_V1,           isp_u_cfa_block},
	{ISP_BLK_CMC10,            isp_u_cmc_block},
//	{ISP_BLK_HDR,                         isp_u_hdr_block},//pike has wdr module ,this module is same to sharkl's hdr module
	{ISP_BLK_RGB_GAMC,         isp_u_gamma_block},
	{ISP_BLK_CMC8,             isp_u_cmc8_block},
	{ISP_BLK_CTM,			   isp_u_ct_block},//different
	{ISP_BLK_UVDIV_V1,         isp_u_cce_uv_block},
	{ISP_BLK_CCE_V1,           isp_u_cce_matrix_block},
	{ISP_BLK_HSV,			   isp_u_hsv_block},
	{ISP_BLK_RADIAL_CSC,       isp_u_csc_block},
	{ISP_BLK_RGB_PRECDN,       isp_u_frgb_precdn_block},
	{ISP_BLK_POSTERIZE,        isp_u_posterize_block},
	{ISP_BLK_AF_V1, 		   isp_u_raw_afm_block},
	{ISP_BLK_RGB2Y,		       isp_u_rgb2y_block},///pike has this module ,but tshark2 has no.
	{ISP_BLK_YIQ_AEM,          isp_u_yiq_aem_block},//different
	{ISP_BLK_YIQ_AFL,		   isp_u_anti_flicker_block},//different
	{ISP_BLK_YIQ_AFM,		   isp_u_yiq_afm_block},//different
	{ISP_BLK_YUV_PRECDN,       isp_u_yuv_precdn_block},
	{ISP_BLK_PREF_V1,          isp_u_prefilter_block},
	{ISP_BLK_UV_PREFILTER,     isp_u_uv_prefilter_block},//pike has this module ,but tshark2 has no.
	{ISP_BLK_BRIGHT,           isp_u_brightness_block},
	{ISP_BLK_CONTRAST,         isp_u_contrast_block},
	{ISP_BLK_HIST_V1,          isp_u_hist_block},//different
	{ISP_BLK_HIST2,            isp_u_hist2_block},//different
	{ISP_BLK_AUTO_CONTRAST_V1, isp_u_aca_block},
	{ISP_BLK_UV_CDN,           isp_u_yuv_cdn_block},
	{ISP_BLK_EDGE_V1,          isp_u_edge_block},//different
//	{ISP_BLK_EMBOSS_V1,        isp_u_emboss_block},//pike have no this module
	{ISP_BLK_CSS_V1,           isp_u_css_block},
	{ISP_BLK_SATURATION_V1,    isp_u_csa_block},
	{ISP_BLK_HUE_V1,           isp_u_hue_block},
	{ISP_BLK_UV_POSTCDN,       isp_u_yuv_postcdn_block},//different
	{ISP_BLK_EMBOSS_V1,        isp_u_emboss_block},//pike has emboss uv module,but whithout emboss y mode
	{ISP_BLK_Y_GAMMC,          isp_u_ygamma_block},
	{ISP_BLK_YDELAY,           isp_u_ydelay_block},
	{ISP_BLK_YUV_NLM,          isp_u_yuv_nlm_block},//pike has this module ,but tshark2 has no.
//	{ISP_BLK_IIRCNR_IIR,       isp_u_iircnr_block},//pike have no this module
//	{ISP_BLK_IIRCNR_YRANDOM,   isp_u_yrandom_block},//pike have no this module
};

isp_s32 isp_cfg_block(isp_handle handle, void *param_ptr, isp_u32 sub_block)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i = 0, cnt = 0;
	isp_cfg_fun_ptr cfg_fun_ptr = PNULL;

	isp_u32 chip_id = isp_dev_get_chip_id(handle);
	if (ISP_CHIP_ID_PIKE == chip_id) {
		cnt = sizeof(s_isp_cfg_fun_tab_pike) / sizeof(s_isp_cfg_fun_tab_pike[0]);
		for (i = 0; i < cnt; i++) {
			if (sub_block == s_isp_cfg_fun_tab_pike[i].sub_block) {
				cfg_fun_ptr = s_isp_cfg_fun_tab_pike[i].cfg_fun;
				break;
			}
		}
	} else {
		cnt = sizeof(s_isp_cfg_fun_tab_tshark2) / sizeof(s_isp_cfg_fun_tab_tshark2[0]);
		for (i = 0; i < cnt; i++) {
			if (sub_block == s_isp_cfg_fun_tab_tshark2[i].sub_block) {
				cfg_fun_ptr = s_isp_cfg_fun_tab_tshark2[i].cfg_fun;
				break;
			}
		}

	}

	if (PNULL != cfg_fun_ptr) {
		rtn = cfg_fun_ptr(handle, param_ptr);
	}

	return rtn;
}

isp_s32 isp_set_arbiter(isp_handle isp_handler)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_ctrl_context *handle = (isp_ctrl_context *)isp_handler;
	struct isp_interface_param_v1 *isp_context_ptr = &handle->interface_param_v1;
	struct isp_dev_arbiter_info_v1 *isp_arbiter_ptr = &isp_context_ptr->arbiter;

	isp_arbiter_ptr->endian_ch0.bpc_endian = ISP_ENDIAN_BIG;
	isp_arbiter_ptr->endian_ch0.lens_endian = ISP_ENDIAN_BIG;
	isp_arbiter_ptr->endian_ch0.fetch_bit_reorder = ISP_LSB;
	if (ISP_EMC_MODE == isp_context_ptr->data.input) {
		isp_arbiter_ptr->endian_ch0.fetch_endian = ISP_ENDIAN_LITTLE;
	} else {
		isp_arbiter_ptr->endian_ch0.fetch_endian = ISP_ENDIAN_BIG;
	}
	if (ISP_EMC_MODE == isp_context_ptr->data.output) {
		isp_arbiter_ptr->endian_ch0.store_endian = ISP_ENDIAN_LITTLE;
	} else {
		isp_arbiter_ptr->endian_ch0.store_endian = ISP_ENDIAN_BIG;
	}

	return rtn;
}

isp_s32 isp_set_dispatch(isp_handle isp_handler)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_ctrl_context *handle = (isp_ctrl_context *)isp_handler;
	struct isp_interface_param_v1 *isp_context_ptr = &handle->interface_param_v1;
	struct isp_dev_dispatch_info_v1 *isp_dispatch_ptr = &isp_context_ptr->dispatch;

	isp_dispatch_ptr->bayer_ch0 = isp_context_ptr->data.format_pattern;

	return rtn;
}

static isp_u32 isp_get_bayer_pattern(isp_u32 bayer_mode, isp_u32 start_x,isp_u32 start_y)
{
	isp_u32 bayer = bayer_mode;

	if (0x01 & start_x) {
		bayer ^= 0x01;
	}

	if (0x01 & start_y) {
		bayer ^= 0x02;
	}

	return bayer;
}

static isp_u32 isp_change_bayer_pattern(struct isp_interface_param *isp_context_ptr)
{
	struct isp_slice_param *slice_ptr = &isp_context_ptr->slice;
	struct isp_com_param *com_ptr = &isp_context_ptr->com;

	com_ptr->nlc_bayer = isp_get_bayer_pattern(isp_context_ptr->com.bayer_pattern, slice_ptr->size[ISP_BNLC].x, slice_ptr->size[ISP_BNLC].y);
	com_ptr->awbc_bayer = com_ptr->nlc_bayer;
	com_ptr->gain_bayer = isp_get_bayer_pattern(isp_context_ptr->com.bayer_pattern, slice_ptr->size[ISP_GLB_GAIN].x, slice_ptr->size[ISP_GLB_GAIN].y);
	com_ptr->wave_bayer = com_ptr->gain_bayer;
	com_ptr->cfa_bayer = isp_get_bayer_pattern(isp_context_ptr->com.bayer_pattern, slice_ptr->size[ISP_CFA].x, slice_ptr->size[ISP_CFA].y);

	return ISP_SUCCESS;
}

static isp_u32 isp_get_feeder_data_type(enum isp_format in_format)
{
	isp_u32 data_type = ISP_ZERO;

	switch (in_format) {
	case ISP_DATA_YUV422_3FRAME:
	case ISP_DATA_YUYV:
	case ISP_DATA_UYVY:
	case ISP_DATA_YVYU:
	case ISP_DATA_VYUY:
	case ISP_DATA_YUV422_2FRAME:
	case ISP_DATA_YVU422_2FRAME:
		data_type = ISP_ONE;
		break;
	case ISP_DATA_NORMAL_RAW10:
	case ISP_DATA_CSI2_RAW10:
		data_type = ISP_ZERO;
		break;
	default:
		break;
	}

	return data_type;
}

isp_s32 isp_set_feeder_param(isp_handle isp_handler)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_ctrl_context *handle = (isp_ctrl_context *)isp_handler;
	struct isp_interface_param *isp_context_ptr = &handle->interface_param;
	struct isp_dev_feeder_info *feeder_param_ptr = &isp_context_ptr->feeder;

	feeder_param_ptr->data_type = isp_get_feeder_data_type(isp_context_ptr->data.input_format);

	return rtn;
}

static enum isp_fetch_format isp_get_fetch_format(enum isp_format in_format)
{
	enum isp_fetch_format format = ISP_FETCH_FORMAT_MAX;

	switch (in_format) {
	case ISP_DATA_YUV422_3FRAME:
		format = ISP_FETCH_YUV422_3FRAME;
		break;
	case ISP_DATA_YUYV:
		format = ISP_FETCH_YUYV;
		break;
	case ISP_DATA_UYVY:
		format = ISP_FETCH_UYVY;
		break;
	case ISP_DATA_YVYU:
		format = ISP_FETCH_YVYU;
		break;
	case ISP_DATA_VYUY:
		format = ISP_FETCH_VYUY;
		break;
	case ISP_DATA_YUV422_2FRAME:
		format = ISP_FETCH_YUV422_2FRAME;
		break;
	case ISP_DATA_YVU422_2FRAME:
		format = ISP_FETCH_YVU422_2FRAME;
		break;
	case ISP_DATA_NORMAL_RAW10:
		format = ISP_FETCH_NORMAL_RAW10;
		break;
	case ISP_DATA_CSI2_RAW10:
		format = ISP_FETCH_CSI2_RAW10;
		break;
	default:
		break;
	}

	return format;
}

static isp_s32 isp_get_fetch_pitch(struct isp_pitch *pitch_ptr, isp_u16 width, enum isp_format format)
{
	isp_s32 rtn = ISP_SUCCESS;

	pitch_ptr->chn0 = ISP_ZERO;
	pitch_ptr->chn1 = ISP_ZERO;
	pitch_ptr->chn2 = ISP_ZERO;

	switch (format) {
	case ISP_DATA_YUV422_3FRAME:
		pitch_ptr->chn0 = width;
		pitch_ptr->chn1 = width >> ISP_ONE;
		pitch_ptr->chn2 = width >> ISP_ONE;
		break;
	case ISP_DATA_YUV422_2FRAME:
	case ISP_DATA_YVU422_2FRAME:
		pitch_ptr->chn0 = width;
		pitch_ptr->chn1 = width;
		break;
	case ISP_DATA_YUYV:
	case ISP_DATA_UYVY:
	case ISP_DATA_YVYU:
	case ISP_DATA_VYUY:
		pitch_ptr->chn0 = width << ISP_ONE;
		break;
	case ISP_DATA_NORMAL_RAW10:
		pitch_ptr->chn0 = width << ISP_ONE;
		break;
	case ISP_DATA_CSI2_RAW10:
		pitch_ptr->chn0 = (width * 5) >> ISP_TWO;
		break;
	default:
		break;
	}

	return rtn;
}

isp_s32 isp_get_fetch_addr(struct isp_interface_param *isp_context_ptr, struct isp_dev_fetch_info *fetch_ptr)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_size *cur_slice_ptr = &isp_context_ptr->slice.cur_slice_num;
	isp_u32 ch0_offset = ISP_ZERO;
	isp_u32 ch1_offset = ISP_ZERO;
	isp_u32 ch2_offset = ISP_ZERO;
	isp_u32 slice_w_offset = ISP_ZERO;
	isp_u32 slice_h_offset = ISP_ZERO;
	isp_u16 overlap_up = ISP_ZERO;
	isp_u16 overlap_left = ISP_ZERO;
	isp_u16 src_width = isp_context_ptr->src.w;
	isp_u16 slice_width = isp_context_ptr->slice.max_size.w;
	isp_u16 fetch_width = isp_context_ptr->slice.size[ISP_FETCH].w;
	isp_u32 start_col = ISP_ZERO;
	isp_u32 end_col = ISP_ZERO;
	isp_u16 complete_line = isp_context_ptr->slice.complete_line;
	isp_u32 mipi_word_num_start[16] = {0,
										1,1,1,1,
										2,2,2,
										3,3,3,
										4,4,4,
										5,5
	};
	isp_u32 mipi_word_num_end[16] = {0,
									2,2,2,2,
									3,3,3,3,
									4,4,4,4,
									5,5,5
	};

	if ((ISP_FETCH_NORMAL_RAW10 == isp_context_ptr->com.fetch_color_format)
		||(ISP_FETCH_CSI2_RAW10 == isp_context_ptr->com.fetch_color_format)) {
		overlap_up = RAW_OVERLAP_UP;
		overlap_left = RAW_OVERLAP_LEFT;
	} else {
		overlap_up = YUV_OVERLAP_UP;
		overlap_left = YUV_OVERLAP_LEFT;
	}

	fetch_ptr->bypass = isp_context_ptr->fetch.bypass;
	fetch_ptr->substract = isp_context_ptr->fetch.substract;
	fetch_ptr->addr.chn0 = isp_context_ptr->fetch.addr.chn0;
	fetch_ptr->addr.chn1 = isp_context_ptr->fetch.addr.chn1;
	fetch_ptr->addr.chn2 = isp_context_ptr->fetch.addr.chn2;
	fetch_ptr->pitch.chn0 = isp_context_ptr->fetch.pitch.chn0;
	fetch_ptr->pitch.chn1 = isp_context_ptr->fetch.pitch.chn1;
	fetch_ptr->pitch.chn2 = isp_context_ptr->fetch.pitch.chn2;

	if (ISP_ZERO != cur_slice_ptr->w) {
		slice_w_offset = overlap_left;
	}

	if ((ISP_SLICE_MID == isp_context_ptr->slice.pos_info)
		|| (ISP_SLICE_LAST == isp_context_ptr->slice.pos_info)) {
		slice_h_offset = overlap_up;
	}

	switch (isp_context_ptr->com.fetch_color_format) {
	case ISP_FETCH_YUV422_3FRAME:
		ch0_offset = src_width * (complete_line - slice_h_offset) + slice_width * cur_slice_ptr->w - slice_w_offset;
		ch1_offset = (src_width * (complete_line - slice_h_offset) + slice_width * cur_slice_ptr->w - slice_w_offset) >> 0x01;
		ch2_offset = (src_width * (complete_line - slice_h_offset) + slice_width * cur_slice_ptr->w - slice_w_offset) >> 0x01;
		break;
	case ISP_FETCH_YUV422_2FRAME:
	case ISP_FETCH_YVU422_2FRAME:
		ch0_offset = src_width * (complete_line - slice_h_offset) + slice_width * cur_slice_ptr->w - slice_w_offset;
		ch1_offset = src_width * (complete_line - slice_h_offset) + slice_width * cur_slice_ptr->w - slice_w_offset;
		break;
	case ISP_FETCH_YUYV:
	case ISP_FETCH_UYVY:
	case ISP_FETCH_YVYU:
	case ISP_FETCH_VYUY:
	case ISP_FETCH_NORMAL_RAW10:
		ch0_offset = (src_width * (complete_line - slice_h_offset) + slice_width * cur_slice_ptr->w - slice_w_offset) << 0x01;
		break;
	case ISP_FETCH_CSI2_RAW10:
		ch0_offset = ((src_width * (complete_line - slice_h_offset) + slice_width * cur_slice_ptr->w - slice_w_offset) * 0x05) >> 0x02;
		break;
	default:
		break;
	}

	fetch_ptr->addr.chn0 += ch0_offset;
	fetch_ptr->addr.chn1 += ch1_offset;
	fetch_ptr->addr.chn2 += ch2_offset;

	start_col = slice_width * cur_slice_ptr->w - slice_w_offset;
	end_col = start_col + fetch_width - ISP_ONE;

	fetch_ptr->mipi_byte_rel_pos = start_col & 0x0f;
	fetch_ptr->mipi_word_num = (((end_col + 1) >> 4) * 5 + mipi_word_num_end[(end_col + 1) & 0x0f]) -
								(((start_col + 1) >> 4) * 5 + mipi_word_num_start[(start_col + 1) & 0x0f]) + 1;

	return rtn;
}

isp_s32 isp_get_fetch_addr_v1(struct isp_interface_param_v1 *isp_context_ptr, struct isp_dev_fetch_info_v1 *fetch_ptr)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_size *cur_slice_ptr = &isp_context_ptr->slice.cur_slice_num;
	isp_u32 ch0_offset = ISP_ZERO;
	isp_u32 ch1_offset = ISP_ZERO;
	isp_u32 ch2_offset = ISP_ZERO;
	isp_u32 slice_w_offset = ISP_ZERO;
	isp_u32 slice_h_offset = ISP_ZERO;
	isp_u32 overlap_up = ISP_ZERO;
	isp_u32 overlap_left = ISP_ZERO;
	isp_u16 src_width = isp_context_ptr->src.w;
	isp_u16 complete_line = isp_context_ptr->slice.complete_line;
	isp_u16 slice_width = isp_context_ptr->slice.max_size.w;
	isp_u16 fetch_width = isp_context_ptr->slice.size[ISP_FETCH].w;
	isp_u32 start_col = ISP_ZERO;
	isp_u32 end_col = ISP_ZERO;
	isp_u32 mipi_word_num_start[16] = {0,
										1,1,1,1,
										2,2,2,
										3,3,3,
										4,4,4,
										5,5
	};
	isp_u32 mipi_word_num_end[16] = {0,
									2,2,2,2,
									3,3,3,3,
									4,4,4,4,
									5,5,5
	};

	fetch_ptr->bypass = isp_context_ptr->fetch.bypass;
	fetch_ptr->substract = isp_context_ptr->fetch.substract;
	fetch_ptr->color_format = isp_context_ptr->fetch.color_format;
	fetch_ptr->addr.chn0 = isp_context_ptr->fetch.addr.chn0;
	fetch_ptr->addr.chn1 = isp_context_ptr->fetch.addr.chn1;
	fetch_ptr->addr.chn2 = isp_context_ptr->fetch.addr.chn2;
	fetch_ptr->pitch.chn0 = isp_context_ptr->fetch.pitch.chn0;
	fetch_ptr->pitch.chn1 = isp_context_ptr->fetch.pitch.chn1;
	fetch_ptr->pitch.chn2 = isp_context_ptr->fetch.pitch.chn2;

	if ((ISP_FETCH_NORMAL_RAW10 == isp_context_ptr->com.fetch_color_format)
		|| (ISP_FETCH_CSI2_RAW10 == isp_context_ptr->com.fetch_color_format)) {
		overlap_up = RAW_OVERLAP_UP_V1;
		overlap_left = RAW_OVERLAP_LEFT_V1;
	} else {
		overlap_up = YUV_OVERLAP_UP_V1;
		overlap_left = YUV_OVERLAP_LEFT_V1;
	}

	if (ISP_ZERO != cur_slice_ptr->w) {
		slice_w_offset = overlap_left;
	}

	if ((ISP_SLICE_MID == isp_context_ptr->slice.pos_info)
		|| (ISP_SLICE_LAST == isp_context_ptr->slice.pos_info)) {
		slice_h_offset = overlap_up;
	}

	switch (isp_context_ptr->com.fetch_color_format) {
	case ISP_FETCH_YUV422_3FRAME:
		ch0_offset = src_width * (complete_line - slice_h_offset) + slice_width * cur_slice_ptr->w - slice_w_offset;
		ch1_offset = (src_width * (complete_line - slice_h_offset) + slice_width * cur_slice_ptr->w - slice_w_offset) >> 0x01;
		ch2_offset = (src_width * (complete_line - slice_h_offset) + slice_width * cur_slice_ptr->w - slice_w_offset) >> 0x01;
		break;
	case ISP_FETCH_YUV422_2FRAME:
	case ISP_FETCH_YVU422_2FRAME:
		ch0_offset = src_width * (complete_line - slice_h_offset) + slice_width * cur_slice_ptr->w - slice_w_offset;
		ch1_offset = src_width * (complete_line - slice_h_offset) + slice_width * cur_slice_ptr->w - slice_w_offset;
		break;
	case ISP_FETCH_YUYV:
	case ISP_FETCH_UYVY:
	case ISP_FETCH_YVYU:
	case ISP_FETCH_VYUY:
	case ISP_FETCH_NORMAL_RAW10:
		ch0_offset = (src_width * (complete_line - slice_h_offset) + slice_width * cur_slice_ptr->w - slice_w_offset) << 0x01;
		break;
	case ISP_FETCH_CSI2_RAW10:
		ch0_offset = ((src_width * (complete_line - slice_h_offset) + slice_width * cur_slice_ptr->w - slice_w_offset) * 0x05) >> 0x02;
		break;
	default:
		break;
	}

	fetch_ptr->addr.chn0 += ch0_offset;
	fetch_ptr->addr.chn1 += ch1_offset;
	fetch_ptr->addr.chn2 += ch2_offset;

	start_col = slice_width * cur_slice_ptr->w - slice_w_offset;
	end_col = start_col + fetch_width - ISP_ONE;

	fetch_ptr->mipi_byte_rel_pos = start_col & 0x0f;
	fetch_ptr->mipi_word_num = (((end_col + 1) >> 4) * 5 + mipi_word_num_end[(end_col + 1) & 0x0f]) -
								(((start_col + 1) >> 4) * 5 + mipi_word_num_start[(start_col + 1) & 0x0f]) + 1;

	return rtn;
}

isp_s32 isp_set_fetch_param(isp_handle isp_handler)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_ctrl_context *handle = (isp_ctrl_context *)isp_handler;
	struct isp_interface_param *isp_context_ptr = &handle->interface_param;
	struct isp_dev_fetch_info *fetch_param_ptr = &isp_context_ptr->fetch;

	fetch_param_ptr->bypass = ISP_ZERO;
	fetch_param_ptr->substract = ISP_ZERO;
	fetch_param_ptr->addr.chn0 = isp_context_ptr->data.input_addr.chn0;
	fetch_param_ptr->addr.chn1 = isp_context_ptr->data.input_addr.chn1;
	fetch_param_ptr->addr.chn2 = isp_context_ptr->data.input_addr.chn2;
	isp_get_fetch_pitch(&(fetch_param_ptr->pitch), isp_context_ptr->data.input_size.w,
		isp_context_ptr->data.input_format);

	return rtn;
}

isp_s32 isp_set_fetch_param_v1(isp_handle isp_handler)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_ctrl_context *handle = (isp_ctrl_context *)isp_handler;
	struct isp_interface_param_v1 *isp_context_ptr = &handle->interface_param_v1;
	struct isp_dev_fetch_info_v1 *fetch_param_ptr = &isp_context_ptr->fetch;

	fetch_param_ptr->bypass = ISP_ZERO;
	fetch_param_ptr->substract = ISP_ZERO;
	fetch_param_ptr->color_format = isp_get_fetch_format(isp_context_ptr->data.input_format);
	fetch_param_ptr->addr.chn0 = isp_context_ptr->data.input_addr.chn0;
	fetch_param_ptr->addr.chn1 = isp_context_ptr->data.input_addr.chn1;
	fetch_param_ptr->addr.chn2 = isp_context_ptr->data.input_addr.chn2;
	isp_get_fetch_pitch(&(fetch_param_ptr->pitch), isp_context_ptr->data.input_size.w,
		isp_context_ptr->data.input_format);

	return rtn;
}

static enum isp_store_format isp_get_store_format(enum isp_format in_format)
{
	enum isp_store_format format = ISP_STORE_FORMAT_MAX;

	switch (in_format) {
	case ISP_DATA_UYVY:
		format = ISP_STORE_UYVY;
		break;
	case ISP_DATA_YUV422_2FRAME:
		format = ISP_STORE_YUV422_2FRAME;
		break;
	case ISP_DATA_YVU422_2FRAME:
		format = ISP_STORE_YVU422_2FRAME;
		break;
	case ISP_DATA_YUV422_3FRAME:
		format = ISP_STORE_YUV422_3FRAME;
		break;
	case ISP_DATA_YUV420_2FRAME:
		format = ISP_STORE_YUV420_2FRAME;
		break;
	case ISP_DATA_YVU420_2FRAME:
		format = ISP_STORE_YVU420_2FRAME;
		break;
	case ISP_DATA_YUV420_3_FRAME:
		format = ISP_STORE_YUV420_3FRAME;
		break;
	default:
		break;
	}

	return format;
}

static isp_s32 isp_get_store_pitch(struct isp_pitch *pitch_ptr, isp_u16 width, enum isp_format format)
{
	isp_s32 rtn = ISP_SUCCESS;

	pitch_ptr->chn0 = ISP_ZERO;
	pitch_ptr->chn1 = ISP_ZERO;
	pitch_ptr->chn2 = ISP_ZERO;

	switch (format) {
	case ISP_DATA_YUV422_3FRAME:
	case ISP_DATA_YUV420_3_FRAME:
		pitch_ptr->chn0 = width;
		pitch_ptr->chn1 = width >> ISP_ONE;
		pitch_ptr->chn2 = width >> ISP_ONE;
		break;
	case ISP_DATA_YUV422_2FRAME:
	case ISP_DATA_YVU422_2FRAME:
	case ISP_DATA_YUV420_2FRAME:
	case ISP_DATA_YVU420_2FRAME:
		pitch_ptr->chn0 = width;
		pitch_ptr->chn1 = width;
		break;
	case ISP_DATA_UYVY:
		pitch_ptr->chn0 = width << ISP_ONE;
		break;
	default:
		break;
	}

	return rtn;
}

isp_s32 isp_get_store_addr(struct isp_interface_param *isp_context_ptr, struct isp_dev_store_info *store_ptr)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_size *cur_slice_ptr = &isp_context_ptr->slice.cur_slice_num;
	isp_u32 ch0_offset = ISP_ZERO;
	isp_u32 ch1_offset = ISP_ZERO;
	isp_u32 ch2_offset = ISP_ZERO;
	isp_u16 slice_width = isp_context_ptr->slice.max_size.w;
	isp_u16 slice_height = isp_context_ptr->slice.max_size.h;

	store_ptr->bypass = isp_context_ptr->store.bypass;
	store_ptr->substract = isp_context_ptr->store.substract;
	store_ptr->addr.chn0 = isp_context_ptr->store.addr.chn0;
	store_ptr->addr.chn1 = isp_context_ptr->store.addr.chn1;
	store_ptr->addr.chn2 = isp_context_ptr->store.addr.chn2;
	store_ptr->pitch.chn0 = isp_context_ptr->store.pitch.chn0;
	store_ptr->pitch.chn1 = isp_context_ptr->store.pitch.chn1;
	store_ptr->pitch.chn2 = isp_context_ptr->store.pitch.chn2;

	switch (isp_context_ptr->com.store_yuv_format) {
	case ISP_STORE_YUV422_3FRAME:
	case ISP_STORE_YUV420_3FRAME:
		ch0_offset = slice_width * cur_slice_ptr->w;
		ch1_offset = (slice_width * cur_slice_ptr->w) >> 0x01;
		ch2_offset = (slice_width * cur_slice_ptr->w) >> 0x01;
		break;
	case ISP_STORE_YUV422_2FRAME:
	case ISP_STORE_YVU422_2FRAME:
	case ISP_STORE_YUV420_2FRAME:
	case ISP_STORE_YVU420_2FRAME:
		ch0_offset = slice_width * cur_slice_ptr->w;
		ch1_offset = slice_width * cur_slice_ptr->w;
		break;
	case ISP_STORE_UYVY:
		ch0_offset = (slice_width * cur_slice_ptr->w) << 0x01;
		break;
	default:
		break;
	}

	store_ptr->addr.chn0 += ch0_offset;
	store_ptr->addr.chn1 += ch1_offset;
	store_ptr->addr.chn2 += ch2_offset;

	return rtn;
}

isp_s32 isp_get_store_addr_v1(struct isp_interface_param_v1 *isp_context_ptr, struct isp_dev_store_info_v1 *store_ptr)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_size *cur_slice_ptr = &isp_context_ptr->slice.cur_slice_num;
	isp_u32 ch0_offset = ISP_ZERO;
	isp_u32 ch1_offset = ISP_ZERO;
	isp_u32 ch2_offset = ISP_ZERO;
	isp_u16 slice_width = isp_context_ptr->slice.max_size.w;
	isp_u16 slice_height = isp_context_ptr->slice.max_size.h;

	store_ptr->bypass = isp_context_ptr->store.bypass;
	store_ptr->substract = isp_context_ptr->store.substract;
	store_ptr->color_format = isp_context_ptr->store.color_format;
	store_ptr->pitch.chn0 = isp_context_ptr->store.pitch.chn0;
	store_ptr->pitch.chn1 = isp_context_ptr->store.pitch.chn1;
	store_ptr->pitch.chn2 = isp_context_ptr->store.pitch.chn2;
	store_ptr->addr.chn0 = isp_context_ptr->store.addr.chn0;
	store_ptr->addr.chn1 = isp_context_ptr->store.addr.chn1;
	store_ptr->addr.chn2 = isp_context_ptr->store.addr.chn2;

	switch (isp_context_ptr->com.store_color_format) {
	case ISP_STORE_YUV422_3FRAME:
	case ISP_STORE_YUV420_3FRAME:
		ch0_offset = slice_width * cur_slice_ptr->w;
		ch1_offset = (slice_width * cur_slice_ptr->w) >> 0x01;
		ch2_offset = (slice_width * cur_slice_ptr->w) >> 0x01;
		break;
	case ISP_STORE_YUV422_2FRAME:
	case ISP_STORE_YVU422_2FRAME:
	case ISP_STORE_YUV420_2FRAME:
	case ISP_STORE_YVU420_2FRAME:
		ch0_offset = slice_width * cur_slice_ptr->w;
		ch1_offset = slice_width * cur_slice_ptr->w;
		break;
	case ISP_STORE_UYVY:
		ch0_offset = (slice_width * cur_slice_ptr->w) << 0x01;
		break;
	default:
		break;
	}

	store_ptr->addr.chn0 += ch0_offset;
	store_ptr->addr.chn1 += ch1_offset;
	store_ptr->addr.chn2 += ch2_offset;

	return rtn;
}

isp_s32 isp_set_store_param(isp_handle isp_handler)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_ctrl_context *handle = (isp_ctrl_context *)isp_handler;
	struct isp_interface_param *isp_context_ptr = &handle->interface_param;
	struct isp_dev_store_info *store_param_ptr = &isp_context_ptr->store;

	store_param_ptr->bypass = ISP_ZERO;
	store_param_ptr->substract = ISP_ONE;
	store_param_ptr->addr.chn0 = isp_context_ptr->data.output_addr.chn0;
	store_param_ptr->addr.chn1 = isp_context_ptr->data.output_addr.chn1;
	store_param_ptr->addr.chn2 = isp_context_ptr->data.output_addr.chn2;
	isp_get_store_pitch(&(store_param_ptr->pitch), isp_context_ptr->data.input_size.w,
		isp_context_ptr->data.output_format);

	return rtn;
}

isp_s32 isp_set_store_param_v1(isp_handle isp_handler)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_ctrl_context *handle = (isp_ctrl_context *)isp_handler;
	struct isp_interface_param_v1 *isp_context_ptr = &handle->interface_param_v1;
	struct isp_dev_store_info_v1 *store_param_ptr = &isp_context_ptr->store;

	store_param_ptr->bypass = 0;
	store_param_ptr->substract = 0x1;
	store_param_ptr->color_format = isp_get_store_format(isp_context_ptr->data.output_format);
	store_param_ptr->addr.chn0 = isp_context_ptr->data.output_addr.chn0;
	store_param_ptr->addr.chn1 = isp_context_ptr->data.output_addr.chn1;
	store_param_ptr->addr.chn2 = isp_context_ptr->data.output_addr.chn2;
	isp_get_store_pitch(&(store_param_ptr->pitch), isp_context_ptr->data.input_size.w,
		isp_context_ptr->data.output_format);

	return rtn;
}

static isp_s32 isp_add_slice_border(enum isp_slice_type type, enum isp_process_type proc_type, struct isp_slice_param *slice_ptr)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u16 up = ISP_ZERO;
	isp_u16 down = ISP_ZERO;
	isp_u16 left = ISP_ZERO;
	isp_u16 right = ISP_ZERO;

	switch (proc_type) {
	case ISP_PROC_BAYER: {
		switch (type) {
		case ISP_WAVE: {
			up = 2;
			down = 2;
			left = 2;
			right = 2;
			break;
		}
		case ISP_GLB_GAIN: {
			up = 5;
			down = 5;
			left = 5;
			right = 5;
			break;
		}
		case ISP_CFA: {
			up = 8;
			down = 8;
			left = 8;
			right = 8;
			break;
		}
		case ISP_PREF:
		case ISP_BRIGHT: {
			up = 9;
			down = 8;
			left = 10;
			right = 8;
			break;
		}
		case ISP_CSS: {
			up = 11;
			down = 10;
			left = 14;
			right = 12;
			break;
		}
		default:
			break;
		}
		break;
	}
	case ISP_PROC_YUV: {
	switch (type) {
		case ISP_BRIGHT: {
			up = 1;
			down = 0;
			left = 2;
			right = 0;
			break;
		}
		case ISP_CSS: {
			up = 3;
			down = 2;
			left = 6;
			right = 4;
			break;
		}
		default:
			break;
	}
		break;
	}
	default:
		break;
	}

	switch (slice_ptr->edge_info) {
	case 0:/*center*/
		slice_ptr->size[type].x += left;
		slice_ptr->size[type].y += up;
		slice_ptr->size[type].w -= left + right;
		slice_ptr->size[type].h -= up + down;
		break;
	case 1:/*right*/
		slice_ptr->size[type].x += left;
		slice_ptr->size[type].y += up;
		slice_ptr->size[type].w -= left;
		slice_ptr->size[type].h -= up + down;
		break;
	case 2:/*left*/
		slice_ptr->size[type].y += up;
		slice_ptr->size[type].w -= right;
		slice_ptr->size[type].h -= up + down;
		break;
	case 3:/*left right*/
		slice_ptr->size[type].y += up;
		slice_ptr->size[type].h -= up + down;
		break;
	case 4:/*down*/
		slice_ptr->size[type].x += left;
		slice_ptr->size[type].y += up;
		slice_ptr->size[type].w -= left + right;
		slice_ptr->size[type].h -= up;
		break;
	case 5:/*down right*/
		slice_ptr->size[type].x += left;
		slice_ptr->size[type].y += up;
		slice_ptr->size[type].w -= left;
		slice_ptr->size[type].h -= up;
		break;
	case 6:/*down left*/
		slice_ptr->size[type].y += up;
		slice_ptr->size[type].w -= right;
		slice_ptr->size[type].h -= up;
		break;
	case 7:/*down left right*/
		slice_ptr->size[type].y += up;
		slice_ptr->size[type].h -= up;
		break;
	case 8:/*up*/
		slice_ptr->size[type].x += left;
		slice_ptr->size[type].w -= left + right;
		slice_ptr->size[type].h -= down;
		break;
	case 9:/*up right*/
		slice_ptr->size[type].x += left;
		slice_ptr->size[type].w -= left;
		slice_ptr->size[type].h -= down;
		break;
	case 10:/*up left*/
		slice_ptr->size[type].w -= right;
		slice_ptr->size[type].h -= down;
		break;
	case 11:/*up left right*/
		slice_ptr->size[type].h -= down;
		break;
	case 12:/*up down*/
		slice_ptr->size[type].x += left;
		slice_ptr->size[type].w -= left + right;
		break;
	case 13:/*ssup down right*/
		slice_ptr->size[type].x += left;
		slice_ptr->size[type].w -= left;
		break;
	case 14:/*up down left*/
		slice_ptr->size[type].w -= right;
		break;
	case 15:/*up down left right*/
		break;
	default:
		break;
	}

	return rtn;
}

static isp_s32 isp_get_slice_size(enum isp_process_type proc_type, struct isp_size *src_size_ptr, struct isp_slice_param *slice_ptr)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i = ISP_ZERO;
	isp_u16 overlap_up = ISP_ZERO;
	isp_u16 overlap_down = ISP_ZERO;
	isp_u16 overlap_left = ISP_ZERO;
	isp_u16 overlap_right = ISP_ZERO;
	isp_u16 slice_x = ISP_ZERO;
	isp_u16 slice_y = ISP_ZERO;
	isp_u16 slice_width = ISP_ZERO;
	isp_u16 slice_height = ISP_ZERO;

	if (ISP_PROC_BAYER == proc_type) {
		overlap_up = RAW_OVERLAP_UP;
		overlap_down = RAW_OVERLAP_DOWN;
		overlap_left = RAW_OVERLAP_LEFT;
		overlap_right = RAW_OVERLAP_RIGHT;
	} else {
		overlap_up = YUV_OVERLAP_UP;
		overlap_down = YUV_OVERLAP_DOWN;
		overlap_left = YUV_OVERLAP_LEFT;
		overlap_right = YUV_OVERLAP_RIGHT;
	}

	if (ISP_ZERO == slice_ptr->cur_slice_num.w) {
		overlap_left = ISP_ZERO;
	}

	if (slice_ptr->all_slice_num.w == (slice_ptr->cur_slice_num.w + ISP_ONE)) {
		overlap_right = ISP_ZERO;
	}

	if (ISP_SLICE_ALL == slice_ptr->pos_info) {
		overlap_up = ISP_ZERO;
		overlap_down = ISP_ZERO;
	}

	if (ISP_SLICE_FIRST == slice_ptr->pos_info) {
		overlap_up = ISP_ZERO;
	}

	if (ISP_SLICE_LAST == slice_ptr->pos_info) {
		overlap_down = ISP_ZERO;
	}

	slice_x = slice_ptr->max_size.w * slice_ptr->cur_slice_num.w;
	slice_y = slice_ptr->complete_line;

	if (slice_ptr->max_size.w * (slice_ptr->cur_slice_num.w + ISP_ONE) <= src_size_ptr->w) {
		slice_width = slice_ptr->max_size.w;
	} else {
		slice_width = src_size_ptr->w - (slice_ptr->max_size.w * slice_ptr->cur_slice_num.w);
	}

	if (slice_ptr->max_size.h * (slice_ptr->cur_slice_num.h + ISP_ONE) <= src_size_ptr->h) {
		slice_height=slice_ptr->max_size.h;
	} else {
		slice_height = src_size_ptr->h - (slice_ptr->max_size.h * slice_ptr->cur_slice_num.h);
	}

	while (i < ISP_SLICE_TYPE_MAX) {
		slice_ptr->size[i].x = slice_x;
		slice_ptr->size[i].y = slice_y;
		slice_ptr->size[i].w = slice_width;
		slice_ptr->size[i].h = slice_height;

		slice_ptr->size[i].x -= overlap_left;
		slice_ptr->size[i].y -= overlap_up;

		slice_ptr->size[i].w += (overlap_left + overlap_right);
		slice_ptr->size[i].h += (overlap_up + overlap_down);

		i++;
	}

	slice_ptr->size[ISP_LENS].w /= 32;
	slice_ptr->size[ISP_LENS].w += 2;
	slice_ptr->size[ISP_LENS].h /= 32;
	slice_ptr->size[ISP_LENS].h += 2;

	slice_ptr->size[ISP_STORE].w = slice_width;
	slice_ptr->size[ISP_STORE].h = slice_height;

	return rtn;
}

static isp_s32 isp_get_slice_edge_info(struct isp_slice_param *slice_ptr)
{
	isp_s32 rtn = ISP_SUCCESS;

	slice_ptr->edge_info = 0x00;

	if (ISP_ZERO == slice_ptr->cur_slice_num.w) {
		slice_ptr->edge_info |= ISP_SLICE_LEFT;
	}

	if (slice_ptr->all_slice_num.w == (slice_ptr->cur_slice_num.w + ISP_ONE)) {
		slice_ptr->edge_info |= ISP_SLICE_RIGHT;
	}

	if (ISP_SLICE_ALL == slice_ptr->pos_info) {
		slice_ptr->edge_info |= ISP_SLICE_UP;
		slice_ptr->edge_info |= ISP_SLICE_DOWN;
	}

	if (ISP_SLICE_FIRST == slice_ptr->pos_info) {
		slice_ptr->edge_info|=ISP_SLICE_UP;
	}

	if (ISP_SLICE_LAST == slice_ptr->pos_info) {
		slice_ptr->edge_info |= ISP_SLICE_DOWN;
	}

	return rtn;
}

isp_s32 isp_set_slice_pos_info(struct isp_slice_param *slice_ptr)
{
	isp_s32 rtn = ISP_SUCCESS;

	if ((ISP_ZERO == slice_ptr->complete_line)
		&& (slice_ptr->all_line == slice_ptr->max_size.h)) {
		slice_ptr->pos_info = ISP_SLICE_ALL;
	} else {
		if (ISP_ZERO == slice_ptr->complete_line) {
			slice_ptr->pos_info = ISP_SLICE_FIRST;
		} else if (slice_ptr->all_line == (slice_ptr->complete_line + slice_ptr->max_size.h)) {
			slice_ptr->pos_info = ISP_SLICE_LAST;
		} else {
			slice_ptr->pos_info = ISP_SLICE_MID;
		}
	}

	return rtn;
}

isp_s32 isp_set_slice_param(isp_handle isp_handler)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_ctrl_context *handle = (isp_ctrl_context*)isp_handler;
	struct isp_interface_param *isp_context_ptr = &handle->interface_param;
	struct isp_slice_param *slice_param_ptr = &isp_context_ptr->slice;
	isp_u16 max_slice_width = ISP_ZERO;
	isp_u16 max_slice_height = ISP_ZERO;

	memset((void*)slice_param_ptr, ISP_ZERO,sizeof(struct isp_slice_param));
	rtn = isp_u_capability_continue_size(handle->handle_device, &max_slice_width, &max_slice_height);
	slice_param_ptr->max_size.w = ISP_MIN(isp_context_ptr->data.input_size.w, max_slice_width);
	slice_param_ptr->max_size.h = ISP_MIN(isp_context_ptr->data.slice_height, max_slice_height);
	slice_param_ptr->all_line = isp_context_ptr->data.input_size.h;
	slice_param_ptr->complete_line = ISP_ZERO;
	slice_param_ptr->all_slice_num.w = (isp_context_ptr->data.input_size.w - 1) / slice_param_ptr->max_size.w + 1;
	slice_param_ptr->all_slice_num.h = (isp_context_ptr->data.input_size.h - 1) / slice_param_ptr->max_size.h + 1;

	if ((ISP_ZERO == slice_param_ptr->complete_line)
		&& (slice_param_ptr->all_line == slice_param_ptr->max_size.h)) {
		slice_param_ptr->pos_info = ISP_SLICE_ALL;
	} else {
		if (ISP_ZERO == slice_param_ptr->complete_line) {
			slice_param_ptr->pos_info = ISP_SLICE_FIRST;
		} else if (slice_param_ptr->all_line == (slice_param_ptr->complete_line + slice_param_ptr->max_size.h)) {
			slice_param_ptr->pos_info = ISP_SLICE_LAST;
		} else {
			slice_param_ptr->pos_info = ISP_SLICE_MID;
		}
	}

	return rtn;
}

isp_s32 isp_set_slice_border(isp_handle isp_handler)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_ctrl_context *handle = (isp_ctrl_context *)isp_handler;
	struct isp_interface_param *isp_context_ptr = &handle->interface_param;
	struct isp_slice_param *slice_param_ptr = &isp_context_ptr->slice;

	rtn = isp_change_bayer_pattern(isp_context_ptr);
	ISP_RETURN_IF_FAIL(rtn, ("isp change bayer pattern error"));

	rtn = isp_get_slice_edge_info(slice_param_ptr);
	ISP_RETURN_IF_FAIL(rtn, ("isp get slice edge info error"));

	rtn = isp_get_slice_size(isp_context_ptr->com.proc_type, &isp_context_ptr->src, &isp_context_ptr->slice);
	ISP_RETURN_IF_FAIL(rtn, ("isp get slice size error"));

	rtn = isp_add_slice_border(ISP_WAVE, isp_context_ptr->com.proc_type, &isp_context_ptr->slice);
	ISP_RETURN_IF_FAIL(rtn, ("isp add slice border wave error"));

	rtn = isp_add_slice_border(ISP_CFA, isp_context_ptr->com.proc_type, &isp_context_ptr->slice);
	ISP_RETURN_IF_FAIL(rtn, ("isp add slice border cfa error"));

	rtn = isp_add_slice_border(ISP_PREF, isp_context_ptr->com.proc_type, &isp_context_ptr->slice);
	ISP_RETURN_IF_FAIL(rtn, ("isp add slice border pref error"));

	rtn = isp_add_slice_border(ISP_BRIGHT, isp_context_ptr->com.proc_type, &isp_context_ptr->slice);
	ISP_RETURN_IF_FAIL(rtn, ("isp add slice border bright error"));

	rtn = isp_add_slice_border(ISP_CSS, isp_context_ptr->com.proc_type, &isp_context_ptr->slice);
	ISP_RETURN_IF_FAIL(rtn, ("isp add slice border css error"));

	rtn = isp_add_slice_border(ISP_GLB_GAIN, isp_context_ptr->com.proc_type, &isp_context_ptr->slice);
	ISP_RETURN_IF_FAIL(rtn, ("isp add slice border glb gain error"));

	return rtn;
}

isp_s32 isp_set_slice_size_v1(isp_handle isp_handler)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_ctrl_context *handle = (isp_ctrl_context *)isp_handler;
	struct isp_interface_param_v1 *isp_context_ptr = &handle->interface_param_v1;
	struct isp_slice_param_v1 *isp_slice_ptr = &isp_context_ptr->slice;
	isp_s32 i = 0;

	for (i = 0; i < ISP_SLICE_TYPE_MAX_V1; i++) {
		isp_slice_ptr->size[i].x = 0;
		isp_slice_ptr->size[i].y = 0;
		isp_slice_ptr->size[i].w = isp_context_ptr->data.input_size.w;
		isp_slice_ptr->size[i].h = isp_context_ptr->data.input_size.h;
	}

	return rtn;
}

isp_s32 isp_cfg_slice_size(isp_handle handle, struct isp_slice_param *slice_ptr)
{
	isp_s32 rtn = ISP_SUCCESS;

	rtn = isp_u_fetch_slice_size(handle, slice_ptr->size[ISP_FETCH].w, slice_ptr->size[ISP_FETCH].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp fetch slice size error"));

	rtn = isp_u_blc_slice_size(handle, slice_ptr->size[ISP_BNLC].w, slice_ptr->size[ISP_BNLC].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp blc slice size error"));

	rtn = isp_u_blc_slice_info(handle, slice_ptr->edge_info);
	ISP_RETURN_IF_FAIL(rtn, ("isp blc slice info error"));

	rtn = isp_u_2d_lsc_pos(handle, slice_ptr->size[ISP_LENS].x, slice_ptr->size[ISP_LENS].y);
	ISP_RETURN_IF_FAIL(rtn, ("isp lsc pos error"));

	rtn = isp_u_2d_lsc_grid_size(handle, slice_ptr->size[ISP_LENS].w, slice_ptr->size[ISP_LENS].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp lsc grid size error"));

	rtn = isp_u_2d_lsc_slice_size(handle, slice_ptr->size[ISP_WAVE].w, slice_ptr->size[ISP_WAVE].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp lsc slice size error"));

	rtn = isp_u_bdn_slice_size(handle, slice_ptr->size[ISP_WAVE].w, slice_ptr->size[ISP_WAVE].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp wdenoise slice size error"));

	rtn = isp_u_cfa_slice_size(handle, slice_ptr->size[ISP_CFA].w, slice_ptr->size[ISP_CFA].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp cfa slice size error"));

	rtn = isp_u_cfa_slice_info(handle, slice_ptr->edge_info);
	ISP_RETURN_IF_FAIL(rtn, ("isp cfa slice info error"));

	rtn = isp_u_prefilter_slice_size(handle, slice_ptr->size[ISP_PREF].w, slice_ptr->size[ISP_PREF].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp prefilter slice size error"));

	rtn = isp_u_prefilter_slice_info(handle, (slice_ptr->edge_info & (ISP_SLICE_LEFT | ISP_SLICE_UP)));
	ISP_RETURN_IF_FAIL(rtn, ("isp prefilter slice info error"));

	rtn = isp_u_brightness_slice_size(handle, slice_ptr->size[ISP_BRIGHT].w, slice_ptr->size[ISP_BRIGHT].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp brightness slice size error"));

	rtn = isp_u_brightness_slice_info(handle, slice_ptr->edge_info);
	ISP_RETURN_IF_FAIL(rtn, ("isp brightness slice info error"));

	rtn = isp_u_css_slice_size(handle, slice_ptr->size[ISP_CSS].w, slice_ptr->size[ISP_CSS].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp css slice size error"));

	rtn = isp_u_store_slice_size(handle, slice_ptr->size[ISP_STORE].w, slice_ptr->size[ISP_STORE].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp store slice size error"));

	rtn = isp_u_feeder_slice_size(handle, slice_ptr->size[ISP_FEEDER].w, slice_ptr->size[ISP_FEEDER].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp feeder slice size error"));

	rtn = isp_u_glb_gain_slice_size(handle, slice_ptr->size[ISP_GLB_GAIN].w, slice_ptr->size[ISP_GLB_GAIN].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp glb gain slice size error"));

	return rtn;
}

isp_s32 isp_cfg_slice_size_v1(isp_handle handle, struct isp_slice_param_v1 *slice_ptr)
{
	isp_s32 rtn = ISP_SUCCESS;

	rtn = isp_u_1d_lsc_slice_size(handle, slice_ptr->size[ISP_LSC_V1].w, slice_ptr->size[ISP_LSC_V1].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp 1d lsc slice size error"));

	rtn = isp_u_csc_pic_size(handle, slice_ptr->size[ISP_CSC_V1].w, slice_ptr->size[ISP_CSC_V1].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp csc pic size error"));

	rtn = isp_u_bdn_slice_size(handle, slice_ptr->size[ISP_BDN_V1].w, slice_ptr->size[ISP_BDN_V1].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp wdenoise slice size error"));

	rtn = isp_u_pwd_slice_size(handle, slice_ptr->size[ISP_PWD_V1].w, slice_ptr->size[ISP_PWD_V1].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp pwd slice size error"));

	rtn = isp_u_2d_lsc_slice_size(handle, slice_ptr->size[ISP_LENS_V1].w, slice_ptr->size[ISP_LENS_V1].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp 2d lsc slice size error"));

	rtn = isp_u_raw_aem_slice_size(handle, slice_ptr->size[ISP_AEM_V1].w, slice_ptr->size[ISP_AEM_V1].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp raw aem slice size error"));

	rtn = isp_u_yiq_aem_slice_size(handle, slice_ptr->size[ISP_Y_AEM_V1].w, slice_ptr->size[ISP_Y_AEM_V1].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp yiq aem slice size error"));

	rtn = isp_u_raw_afm_slice_size(handle, slice_ptr->size[ISP_RGB_AFM_V1].w, slice_ptr->size[ISP_RGB_AFM_V1].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp raw afm slice size error"));

	rtn = isp_u_yiq_afm_slice_size(handle, slice_ptr->size[ISP_Y_AFM_V1].w, slice_ptr->size[ISP_Y_AFM_V1].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp yiq afm slice size error"));

	rtn = isp_u_hist_slice_size(handle, slice_ptr->size[ISP_HISTS_V1].w, slice_ptr->size[ISP_HISTS_V1].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp hist slice size error"));

	rtn = isp_u_dispatch_ch0_size(handle, slice_ptr->size[ISP_DISPATCH_V1].w, slice_ptr->size[ISP_DISPATCH_V1].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp dispatch ch0 size error"));

	rtn = isp_u_fetch_slice_size(handle, slice_ptr->size[ISP_FETCH_V1].w, slice_ptr->size[ISP_FETCH_V1].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp fetch slice size error"));

	rtn = isp_u_store_slice_size(handle, slice_ptr->size[ISP_STORE_V1].w, slice_ptr->size[ISP_STORE_V1].h);
	ISP_RETURN_IF_FAIL(rtn, ("isp store slice size error"));

	return rtn;
}

isp_s32 isp_set_comm_param(isp_handle isp_handler)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_ctrl_context *handle = (isp_ctrl_context*)isp_handler;
	struct isp_interface_param *isp_context_ptr = &handle->interface_param;
	struct isp_com_param *com_param_ptr = &isp_context_ptr->com;

	com_param_ptr->in_mode = isp_context_ptr->data.input;
	com_param_ptr->fetch_bit_reorder = ISP_LSB;
	if (ISP_EMC_MODE == isp_context_ptr->data.input) {
		com_param_ptr->fetch_endian = ISP_ENDIAN_LITTLE;
	} else {
		com_param_ptr->fetch_endian = ISP_ENDIAN_BIG;
	}
	com_param_ptr->bpc_endian = ISP_ENDIAN_BIG;
	com_param_ptr->out_mode = isp_context_ptr->data.output;
	if (ISP_EMC_MODE == isp_context_ptr->data.output) {
		com_param_ptr->store_endian = ISP_ENDIAN_LITTLE;
	} else {
		com_param_ptr->store_endian = ISP_ENDIAN_BIG;
	}
	com_param_ptr->store_yuv_format = isp_get_store_format(isp_context_ptr->data.output_format);
	com_param_ptr->fetch_color_format = isp_get_fetch_format(isp_context_ptr->data.input_format);
	com_param_ptr->bayer_pattern = isp_context_ptr->data.format_pattern;

	return rtn;
}

isp_s32 isp_set_comm_param_v1(isp_handle isp_handler)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_ctrl_context *handle = (isp_ctrl_context *)isp_handler;
	struct isp_interface_param_v1 *isp_context_ptr = &handle->interface_param_v1;
	struct isp_dev_common_info_v1 *com_param_ptr = &isp_context_ptr->com;

	if (ISP_EMC_MODE == isp_context_ptr->data.input) {
		com_param_ptr->fetch_sel_0 = 0x2;
		com_param_ptr->store_sel_0 = 0x2;
	} else if (ISP_CAP_MODE == isp_context_ptr->data.input) {
		com_param_ptr->fetch_sel_0 = 0x0;
		com_param_ptr->store_sel_0 = 0x0;
	}

	com_param_ptr->fetch_color_format= 0x0;
	com_param_ptr->store_color_format= 0x2;

	return rtn;
}
isp_s32 isp_cfg_com_data(isp_handle handle, struct isp_com_param *param_ptr)
{
	isp_s32 rtn=ISP_SUCCESS;

	rtn = isp_u_comm_in_mode(handle, param_ptr->in_mode);
	ISP_RETURN_IF_FAIL(rtn, ("isp comm in mode error"));

	rtn = isp_u_comm_fetch_endian(handle, param_ptr->fetch_endian, param_ptr->fetch_bit_reorder);
	ISP_RETURN_IF_FAIL(rtn, ("isp comm fetch endian error"));

	rtn = isp_u_comm_fetch_data_format(handle, param_ptr->fetch_color_format);
	ISP_RETURN_IF_FAIL(rtn, ("isp comm fetch data format error"));

	rtn = isp_u_comm_bpc_endian(handle, param_ptr->bpc_endian);
	ISP_RETURN_IF_FAIL(rtn, ("isp comm bpc endian error"));

	rtn = isp_u_comm_out_mode(handle, param_ptr->in_mode);
	ISP_RETURN_IF_FAIL(rtn, ("isp comm out mode error"));

	rtn = isp_u_comm_store_endian(handle, param_ptr->store_endian);
	ISP_RETURN_IF_FAIL(rtn, ("isp comm store endian error"));

	rtn = isp_u_comm_store_format(handle, param_ptr->store_yuv_format);
	ISP_RETURN_IF_FAIL(rtn, ("isp comm store format error"));

	rtn = isp_u_comm_bayer_mode(handle, param_ptr->nlc_bayer, param_ptr->awbc_bayer, param_ptr->wave_bayer, param_ptr->cfa_bayer, param_ptr->gain_bayer);
	ISP_RETURN_IF_FAIL(rtn, ("isp comm bayer mode error"));

	return rtn;
}

isp_s32 isp_cfg_comm_data_v1(isp_handle handle, struct isp_dev_common_info_v1 *param_ptr)
{
	isp_s32 rtn = ISP_SUCCESS;

	rtn = isp_u_comm_block(handle, (void *)param_ptr);
	ISP_RETURN_IF_FAIL(rtn, ("store block cfg error"));

	return rtn;
}

isp_s32 isp_cfg_all_shadow_v1(isp_handle handle, isp_u32 auto_shadow)
{
	isp_s32 rtn = ISP_SUCCESS;

	rtn = isp_u_shadow_ctrl_all(handle, auto_shadow);
	ISP_RETURN_IF_FAIL(rtn, ("all shadow cfg error"));

	return rtn;
}

isp_s32 isp_cfg_awbm_shadow_v1(isp_handle handle, isp_u32 shadow_done)
{
	isp_s32 rtn = ISP_SUCCESS;

	rtn = isp_u_awbm_shadow_ctrl(handle, shadow_done);
	ISP_RETURN_IF_FAIL(rtn, ("awbm shadow cfg error"));

	return rtn;
}

isp_s32 isp_cfg_ae_shadow_v1(isp_handle handle, isp_u32 shadow_done)
{
	isp_s32 rtn = ISP_SUCCESS;

	rtn = isp_u_ae_shadow_ctrl(handle, shadow_done);
	ISP_RETURN_IF_FAIL(rtn, ("ae shadow cfg error"));

	return rtn;
}

isp_s32 isp_cfg_af_shadow_v1(isp_handle handle, isp_u32 shadow_done)
{
	isp_s32 rtn = ISP_SUCCESS;

	rtn = isp_u_af_shadow_ctrl(handle, shadow_done);
	ISP_RETURN_IF_FAIL(rtn, ("af shadow cfg error"));

	return rtn;
}

isp_s32 isp_cfg_afl_shadow_v1(isp_handle handle, isp_u32 shadow_done)
{
	isp_s32 rtn = ISP_SUCCESS;

	rtn = isp_u_afl_shadow_ctrl(handle, shadow_done);
	ISP_RETURN_IF_FAIL(rtn, ("afl shadow cfg error"));

	return rtn;
}

isp_s32 isp_cfg_comm_shadow_v1(isp_handle handle, isp_u32 shadow_done)
{
	isp_s32 rtn = ISP_SUCCESS;

	rtn = isp_u_comm_shadow_ctrl(handle, shadow_done);
	ISP_RETURN_IF_FAIL(rtn, ("comm shadow cfg error"));

	return rtn;
}

isp_s32 isp_cfg_3a_single_frame_shadow_v1(isp_handle handle, isp_u32 enable)
{
	isp_s32 rtn = ISP_SUCCESS;

	rtn = isp_u_3a_ctrl(handle, enable);
	ISP_RETURN_IF_FAIL(rtn, ("3a shadow cfg error"));

	return rtn;
}
