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
#ifndef _ISP_DRV_H_
#define _ISP_DRV_H_
#ifndef WIN32
#include <sys/types.h>
#include <utils/Log.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "sprd_isp.h"
#include "isp_log.h"
#include "isp_com.h"
#endif
#include "isp_type.h"


struct isp_file {
	isp_s32 fd;
	isp_u32 chip_id;
	isp_u32 isp_id;
	isp_u32 reserved;
};

/*ISP Hardware Device*/
isp_s32 isp_dev_open(isp_handle *handle);
isp_s32 isp_dev_close(isp_handle handle);
isp_u32 isp_dev_get_chip_id(isp_handle handle);
isp_s32 isp_dev_reset(isp_handle handle);
isp_s32 isp_dev_stop(isp_handle handle);
isp_s32 isp_dev_enable_irq(isp_handle handle, isp_u32 irq_mode);
isp_s32 isp_dev_get_irq(isp_handle handle, isp_u32 *evt_ptr);
isp_s32 isp_dev_reg_write(isp_handle handle, isp_u32 num, void *param_ptr);
isp_s32 isp_dev_reg_read(isp_handle handle, isp_u32 num, void *param_ptr);
isp_s32 isp_dev_reg_fetch(isp_handle handle, isp_u32 base_offset, isp_u32 *buf, isp_u32 len);

/*ISP Capability*/
isp_s32 isp_u_capability_chip_id(isp_handle handle, isp_u32 *chip_id);
isp_s32 isp_u_capability_continue_size(isp_handle handle, isp_u16 *width, isp_u16 *height);
isp_s32 isp_u_capability_single_size(isp_handle handle, isp_u16 *width, isp_u16 *height);
isp_s32 isp_u_capability_awb_win(isp_handle handle, isp_u16 *width, isp_u16 *height);
isp_u32 isp_u_capability_awb_default_gain(isp_handle handle);
isp_u32 isp_u_capability_af_max_win_num(isp_handle handle);
isp_s32 isp_u_capability_time(isp_handle handle, isp_u32 *sec, isp_u32 *usec);
/*ISP Sub Block: Fetch*/
isp_s32 isp_u_fetch_block(isp_handle handle, void *block_info);
isp_s32 isp_u_fetch_slice_size(isp_handle handle, isp_u32 w, isp_u32 h);

/*ISP Sub Block: BLC*/
isp_s32 isp_u_blc_block(isp_handle handle, void *block_info);
isp_s32 isp_u_blc_slice_size(isp_handle handle, isp_u32 w, isp_u32 h);
isp_s32 isp_u_blc_slice_info(isp_handle handle, isp_u32 info);

/*ISP Sub Block: lens shading calibration*/
isp_s32 isp_u_2d_lsc_block(isp_handle handle, void *block_info);
isp_s32 isp_u_2d_lsc_bypass(isp_handle handle, isp_u32 bypass);
isp_s32 isp_u_2d_lsc_param_update(isp_handle handle);
isp_s32 isp_u_2d_lsc_pos(isp_handle handle, isp_u32 x, isp_u32 y);
isp_s32 isp_u_2d_lsc_grid_size(isp_handle handle, isp_u32 w, isp_u32 h);
isp_s32 isp_u_2d_lsc_slice_size(isp_handle handle, isp_u32 w, isp_u32 h);
isp_s32 isp_u_2d_lsc_transaddr(isp_handle handle,isp_u32 phys_addr);

/*ISP Sub Block: AWBM*/
isp_s32 isp_u_awb_block(isp_handle handle, void *block_info);
isp_s32 isp_u_awbm_statistics(isp_handle handle, isp_u32 *r_info, isp_u32 *g_info, isp_u32 *b_info);
isp_s32 isp_u_awbm_bypass(isp_handle handle, isp_u32 bypass);
isp_s32 isp_u_awbm_mode(isp_handle handle, isp_u32 mode);
isp_s32 isp_u_awbm_skip_num(isp_handle handle, isp_u32 num);
isp_s32 isp_u_awbm_block_offset(isp_handle handle, isp_u32 x, isp_u32 y);
isp_s32 isp_u_awbm_block_size(isp_handle handle, isp_u32 w, isp_u32 h);
isp_s32 isp_u_awbm_shift(isp_handle handle, isp_u32 shift);
isp_s32 isp_u_awbc_bypass(isp_handle handle, isp_u32 bypass);
isp_s32 isp_u_awbc_gain(isp_handle handle, isp_u32 r, isp_u32 g, isp_u32 b);
isp_s32 isp_u_awbc_thrd(isp_handle handle, isp_u32 r, isp_u32 g, isp_u32 b);
isp_s32 isp_u_awbc_gain_offset(isp_handle handle, isp_u32 r, isp_u32 g, isp_u32 b);

/*ISP Sub Block: BPC*/
isp_s32 isp_u_bpc_block(isp_handle handle, void *block_info);
isp_s32 isp_u_bpc_bypass(isp_handle handle, isp_u32 bypass);
isp_s32 isp_u_bpc_mode(isp_handle handle, isp_u32 mode);
isp_s32 isp_u_bpc_param_common(isp_handle handle, isp_u32 pattern_type, isp_u32 detect_thrd, isp_u32 super_bad_thrd);
isp_s32 isp_u_bpc_thrd(isp_handle handle, isp_u32 flat, isp_u32 std, isp_u32 texture);
isp_s32 isp_u_bpc_map_addr(isp_handle handle, isp_u32 addr);
isp_s32 isp_u_bpc_pixel_num(isp_handle handle, isp_u32 pixel_num);

/*ISP Sub Block: bilateral denoise*/
isp_s32 isp_u_bdn_block(isp_handle handle, void *block_info);
isp_s32 isp_u_bdn_bypass(isp_handle handle, isp_u32 bypass);
isp_s32 isp_u_bdn_slice_size(isp_handle handle, isp_u32 w, isp_u32 h);
isp_s32 isp_u_bdn_diswei(isp_handle handle, isp_u32 diswei_level);
isp_s32 isp_u_bdn_ranwei(isp_handle handle, isp_u32 ranwei_level);

/*ISP Sub Block: GRGB*/
isp_s32 isp_u_grgb_block(isp_handle handle, void *block_info);
isp_s32 isp_u_grgb_bypass(isp_handle handle, isp_u32 bypass);
isp_s32 isp_u_grgb_thrd(isp_handle handle, isp_u32 edge, isp_u32 diff);

/*ISP Sub Block: CFA*/
isp_s32 isp_u_cfa_block(isp_handle handle, void *block_info);
isp_s32 isp_u_cfa_thrd(isp_handle handle, isp_u32 edge, isp_u32 ctrl);
isp_s32 isp_u_cfa_slice_size(isp_handle handle, isp_u32 w, isp_u32 h);
isp_s32 isp_u_cfa_slice_info(isp_handle handle, isp_u32 info);

/*ISP Sub Block: CMC*/
isp_s32 isp_u_cmc_block(isp_handle handle, void *block_info);
isp_s32 isp_u_cmc_bypass(isp_handle handle, isp_u32 bypass);
isp_s32 isp_u_cmc_matrix(isp_handle handle, isp_u16 *matrix_ptr);

/*ISP Sub Block: GAMMA*/
isp_s32 isp_u_gamma_block(isp_handle handle, void *block_info);
isp_s32 isp_u_gamma_status(isp_handle handle, isp_u32 *status);
isp_s32 isp_u_gamma_bypass(isp_handle handle, isp_u32 bypass);
isp_s32 isp_u_gamma_node(isp_handle handle, isp_u16 *node_ptr);

/*ISP Sub Block: CCE*/
isp_s32 isp_u_cce_matrix_block(isp_handle handle, void *block_info);
isp_s32 isp_u_cce_uv_block(isp_handle handle, void *block_info);
isp_s32 isp_u_cce_uvdivision_bypass(isp_handle handle, isp_u32 bypass);
isp_s32 isp_u_cce_mode(isp_handle handle, isp_u32 mode);
isp_s32 isp_u_cce_matrix(isp_handle handle, isp_u16 *matrix_ptr);
isp_s32 isp_u_cce_shift(isp_handle handle, isp_u32 y_shift, isp_u32 u_shift, isp_u32 v_shift);
isp_s32 isp_u_cce_uvd(isp_handle handle, isp_u8 *div_ptr);
isp_s32 isp_u_cce_uvc(isp_handle handle, isp_u8 *t_ptr, isp_u8 *m_ptr);

/*ISP Sub Block: Pre-Filter*/
isp_s32 isp_u_prefilter_block(isp_handle handle, void *block_info);
isp_s32 isp_u_prefilter_bypass(isp_handle handle, isp_u32 bypass);
isp_s32 isp_u_prefilter_writeback(isp_handle handle, isp_u32 writeback);
isp_s32 isp_u_prefilter_thrd(isp_handle handle, isp_u32 y_thrd, isp_u32 u_thrd, isp_u32 v_thrd);
isp_s32 isp_u_prefilter_slice_size(isp_handle handle, isp_u32 w, isp_u32 h);
isp_s32 isp_u_prefilter_slice_info(isp_handle handle, isp_u32 info);

/*ISP Sub Block: Brightness*/
isp_s32 isp_u_brightness_block(isp_handle handle, void *block_info);
isp_s32 isp_u_brightness_slice_size(isp_handle handle, isp_u32 w, isp_u32 h);
isp_s32 isp_u_brightness_slice_info(isp_handle handle, isp_u32 info);

/*ISP Sub Block: Contrast*/
isp_s32 isp_u_contrast_block(isp_handle handle, void *block_info);

/*ISP Sub Block: HIST*/
isp_s32 isp_u_hist_block(isp_handle handle, void *block_info);
isp_s32 isp_u_hist_bypass(isp_handle handle, isp_u32 bypass);
isp_s32 isp_u_hist_mode(isp_handle handle, isp_u32 mode);
isp_s32 isp_u_hist_auto_rst_disable(isp_handle handle,	isp_u32 off);
isp_s32 isp_u_hist_mode(isp_handle handle, isp_u32 mode);
isp_s32 isp_u_hist_ratio(isp_handle handle, isp_u16 low_ratio, isp_u16 high_ratio);
isp_s32 isp_u_hist_maxmin(isp_handle handle, isp_u32 in_min, isp_u32 in_max, isp_u32 out_min, isp_u32 out_max);
isp_s32 isp_u_hist_clear_eb(isp_handle handle, isp_u32 eb);
isp_s32 isp_u_hist_statistic(isp_handle handle, void *out_value);
isp_s32 isp_u_hist_statistic_num(isp_handle handle, isp_u32 *num);

/*ISP Sub Block: AUTO CONTRAST ADJUSTMENT*/
isp_s32 isp_u_aca_block(isp_handle handle, void *block_info);
isp_s32 isp_u_aca_maxmin_status(isp_handle handle, isp_u32 *status);
isp_s32 isp_u_aca_bypass(isp_handle handle, isp_u32 bypass);
isp_s32 isp_u_aca_mode(isp_handle handle, isp_u32 mode);
isp_s32 isp_u_aca_maxmin(isp_handle handle, isp_u32 in_min, isp_u32 in_max, isp_u32 out_min, isp_u32 out_max);
isp_s32 isp_u_aca_adjust(isp_handle handle, isp_u32 diff, isp_u32 small, isp_u32 big);

/*ISP Sub Block: AFM*/
isp_s32 isp_u_afm_block(isp_handle handle, void *block_info);
isp_s32 isp_u_afm_bypass(isp_handle handle, isp_u32 bypass);
isp_s32 isp_u_afm_shift(isp_handle handle, isp_u32 shift);
isp_s32 isp_u_afm_mode(isp_handle handle, isp_u32 mode);
isp_s32 isp_u_afm_skip_num(isp_handle handle, isp_u32 skip_num);
isp_s32 isp_u_afm_skip_num_clr(isp_handle handle, isp_u32 is_clear);
isp_s32 isp_u_afm_win(isp_handle handle, void *win_rangs);
isp_s32 isp_u_afm_statistic(isp_handle handle, void *out_statistic);
isp_s32 isp_u_afm_win_num(isp_handle handle, isp_u32 *win_num);
isp_s32 isp_u_raw_afm_statistic_r6p9(isp_handle handle, void *statis);
isp_s32 isp_u_raw_afm_spsmd_square_en(isp_handle handle, isp_u32 en);
isp_s32 isp_u_raw_afm_overflow_protect(isp_handle handle, isp_u32 en);
isp_s32 isp_u_raw_afm_subfilter(isp_handle handle, isp_u32 average, isp_u32 median);
isp_s32 isp_u_raw_afm_spsmd_touch_mode(isp_handle handle, isp_u32 mode);
isp_s32 isp_u_raw_afm_shfit(isp_handle handle, isp_u32 spsmd, isp_u32 sobel5, isp_u32 sobel9);
isp_s32 isp_u_raw_afm_threshold_rgb(isp_handle handle, void *thr_rgb);
/*ISP Sub Block: EDGE*/
isp_s32 isp_u_edge_block(isp_handle handle, void *block_info);
isp_s32 isp_u_edge_bypass(isp_handle handle, isp_u32 bypass);
isp_s32 isp_u_edge_param(isp_handle handle, isp_u32 detail, isp_u32 smooth, isp_u32 strength);

/*ISP Sub Block: Emboss*/
isp_s32 isp_u_emboss_block(isp_handle handle, void *block_info);
isp_s32 isp_u_emboss_bypass(isp_handle handle, isp_u32 bypass);
isp_s32 isp_u_emboss_param(isp_handle handle, isp_u32 step);

/*ISP Sub Block: FCS*/
isp_s32 isp_u_fcs_block(isp_handle handle, void *block_info);
isp_s32 isp_u_fcs_bypass(isp_handle handle, isp_u32 bypass);
isp_s32 isp_u_fcs_mode(isp_handle handle, isp_u32 mode);

/*ISP Sub Block: CSS*/
isp_s32 isp_u_css_block(isp_handle handle, void *block_info);
isp_s32 isp_u_css_bypass(isp_handle handle, isp_u32 bypass);
isp_s32 isp_u_css_thrd(isp_handle handle, isp_u8 *low_thr, isp_u8 *low_sum_thr, isp_u8 lum_thr, isp_u8 chr_thr);
isp_s32 isp_u_css_slice_size(isp_handle handle, isp_u32 w, isp_u32 h);
isp_s32 isp_u_css_ratio(isp_handle handle, isp_u8 *ratio);

/*ISP Sub Block: CSA*/
isp_s32 isp_u_csa_block(isp_handle handle, void *block_info);
isp_s32 isp_u_csa_bypass(isp_handle handle, isp_u32 bypass);
isp_s32 isp_u_csa_factor(isp_handle handle, isp_u32 factor);

/*ISP Sub Block: Store*/
isp_s32 isp_u_store_block(isp_handle handle, void *block_info);
isp_s32 isp_u_store_slice_size(isp_handle handle, isp_u32 w, isp_u32 h);

/*ISP Sub Block: Feeder*/
isp_s32 isp_u_feeder_block(isp_handle handle, void *block_info);
isp_s32 isp_u_feeder_data_type(isp_handle handle, isp_u32 data_type);
isp_s32 isp_u_feeder_slice_size(isp_handle handle, isp_u32 w, isp_u32 h);

/*ISP Sub Block: HDR*/
isp_s32 isp_u_hdr_block(isp_handle handle, void *block_info);
isp_s32 isp_u_hdr_bypass(isp_handle handle, isp_u32 bypass);
isp_s32 isp_u_hdr_level(isp_handle handle, isp_u32 level);
isp_s32 isp_u_hdr_index(isp_handle handle, isp_u32 r_index, isp_u32 g_index, isp_u32 b_index);
isp_s32 isp_u_hdr_tab(isp_handle handle, isp_u8 *com_ptr, isp_u8 *p2e_ptr, isp_u8 *e2p_ptr);

/*ISP Sub Block: NLC*/
isp_s32 isp_u_nlc_block(isp_handle handle, void *block_info);
isp_s32 isp_u_nlc_bypass(isp_handle handle, isp_u32 bypass);
isp_s32 isp_u_nlc_r_node(isp_handle handle, isp_u16 *r_node_ptr);
isp_s32 isp_u_nlc_g_node(isp_handle handle, isp_u16 *g_node_ptr);
isp_s32 isp_u_nlc_b_node(isp_handle handle, isp_u16 *b_node_ptr);
isp_s32 isp_u_nlc_l_node(isp_handle handle, isp_u16 *l_node_ptr);

/*ISP Sub Block: Nawbm*/
isp_s32 isp_u_nawbm_block(isp_handle handle, void *block_info);
isp_s32 isp_u_nawbm_bypass(isp_handle handle, isp_u32 bypass);

/*ISP Sub Block: Pre Wavelet*/
isp_s32 isp_u_pre_wavelet_block(isp_handle handle, void *block_info);
isp_s32 isp_u_pre_wavelet_bypass(isp_handle handle, isp_u32 bypass);

/*ISP Sub Block: Bing4awb*/
isp_s32 isp_u_binning4awb_block(isp_handle handle, void *block_info);
isp_s32 isp_u_binning4awb_bypass(isp_handle handle, isp_u32 bypass);
isp_s32 isp_u_binning4awb_endian(isp_handle handle, isp_u32 endian);
isp_s32 isp_u_binning4awb_scaling_ratio(isp_handle handle, isp_u32 vertical, isp_u32 horizontal);
isp_s32 isp_u_binning4awb_get_scaling_ratio(isp_handle handle, isp_u32 *vertical, isp_u32 *horizontal);
isp_s32 isp_u_binning4awb_mem_addr(isp_handle handle, uint32_t phy_addr);
isp_s32 isp_u_binning4awb_statistics_buf(isp_handle handle, uint32_t *buf_id);
isp_s32 isp_u_binning4awb_transaddr(isp_handle handle, isp_u32 phys0, isp_u32 phys1);
isp_s32 isp_u_binning4awb_initbuf(isp_handle handle);

/*ISP Sub Block: Pre Glb Gain*/
isp_s32 isp_u_pgg_block(isp_handle handle, void *block_info);;

/*ISP Sub Block: COMMON*/
isp_s32 isp_u_comm_start(isp_handle handle, isp_u32 start);
isp_s32 isp_u_comm_in_mode(isp_handle handle, isp_u32 mode);
isp_s32 isp_u_comm_out_mode(isp_handle handle, isp_u32 mode);
isp_s32 isp_u_comm_fetch_endian(isp_handle handle, isp_u32 endian, isp_u32 bit_reorder);
isp_s32 isp_u_comm_bpc_endian(isp_handle handle, isp_u32 endian);
isp_s32 isp_u_comm_store_endian(isp_handle handle, isp_u32 endian);
isp_s32 isp_u_comm_fetch_data_format(isp_handle handle, isp_u32 format);
isp_s32 isp_u_comm_store_format(isp_handle handle, isp_u32 format);
isp_s32 isp_u_comm_burst_size(isp_handle handle, isp_u16 burst_size);
isp_s32 isp_u_comm_mem_switch(isp_handle handle, isp_u8 mem_switch);
isp_s32 isp_u_comm_shadow(isp_handle handle, isp_u32 shadow);
isp_s32 isp_u_comm_shadow_all(isp_handle handle, isp_u8 shadow);
isp_s32 isp_u_comm_bayer_mode(isp_handle handle, isp_u32 nlc_bayer, isp_u32 awbc_bayer, isp_u32 wave_bayer, isp_u32 cfa_bayer, isp_u32 gain_bayer);
isp_s32 isp_u_comm_isp_s32_clear(isp_handle handle, isp_u32 isp_s32_num);
isp_s32 isp_u_comm_get_isp_s32_raw(isp_handle handle, isp_u32 *raw);
isp_s32 isp_u_comm_pmu_raw_mask(isp_handle handle, isp_u8 raw_mask);
isp_s32 isp_u_comm_hw_mask(isp_handle handle, isp_u32 hw_logic);
isp_s32 isp_u_comm_hw_enable(isp_handle handle, isp_u32 hw_logic);
isp_s32 isp_u_comm_pmu_pmu_sel(isp_handle handle, isp_u8 sel);
isp_s32 isp_u_comm_sw_enable(isp_handle handle, isp_u32 sw_logic);
isp_s32 isp_u_comm_preview_stop(isp_handle handle, isp_u8 eb);
isp_s32 isp_u_comm_set_shadow_control(isp_handle handle, isp_u32 control);
isp_s32 isp_u_comm_shadow_control_clear(isp_handle handle, isp_u8 eb);
isp_s32 isp_u_comm_axi_stop(isp_handle handle, isp_u8 eb);
isp_s32 isp_u_comm_slice_cnt_enable(isp_handle handle, isp_u8 eb);
isp_s32 isp_u_comm_preform_cnt_enable(isp_handle handle, isp_u8 eb);
isp_s32 isp_u_comm_set_slice_num(isp_handle handle, isp_u8 num);
isp_s32 isp_u_comm_get_slice_num(isp_handle handle, isp_u8 *slice_num);
isp_s32 isp_u_comm_perform_cnt_rstatus(isp_handle handle, isp_u32 *status);
isp_s32 isp_u_comm_preform_cnt_status(isp_handle handle, isp_u32 *status);

/*ISP Sub Block: Glb gain*/
isp_s32 isp_u_glb_gain_block(isp_handle handle, void *block_info);
isp_s32 isp_u_glb_gain_bypass(isp_handle handle, isp_u32 bypass);
isp_s32 isp_u_glb_gain_set(isp_handle handle, isp_u32 gain);
isp_s32 isp_u_glb_gain_slice_size(isp_handle handle, isp_u16 w, isp_u16 h);

/*ISP Sub Block: RGB gain*/
isp_s32 isp_u_rgb_gain_block(isp_handle handle, void *block_info);

/*ISP Sub Block: YIQ*/
isp_s32 isp_u_yiq_ygamma_block(isp_handle handle, void *block_info);
isp_s32 isp_u_yiq_ae_block(isp_handle handle, void *block_info);
isp_s32 isp_u_yiq_flicker_block(isp_handle handle, void *block_info);
isp_s32 isp_u_yiq_ygamma_bypass(isp_handle handle, isp_u32 bypass);
isp_s32 isp_u_yiq_ygamma_xnode(void *handle, isp_u8 *node);
isp_s32 isp_u_yiq_ygamma_ynode(isp_handle handle, isp_u8 *node);
isp_s32 isp_u_yiq_ygamma_index(isp_handle handle, isp_u8 *node);
isp_s32 isp_u_yiq_ae_bypass(isp_handle handle,isp_u32 bypass);
isp_s32 isp_u_yiq_ae_src_sel(isp_handle handle, isp_u32 src_sel);
isp_s32 isp_u_yiq_ae_mode(isp_handle handle, isp_u32 mode);
isp_s32 isp_u_yiq_ae_skip_num(isp_handle handle, isp_u32 num);
isp_s32 isp_u_yiq_flicker_bypass(isp_handle handle, isp_u32 bypass);
isp_s32 isp_u_yiq_flicker_mode(isp_handle handle, isp_u32 mode);
isp_s32 isp_u_yiq_flicker_vheight(isp_handle handle, isp_u32 height);
isp_s32 isp_u_yiq_flicker_line_conter(isp_handle handle, isp_u32 counter);
isp_s32 isp_u_yiq_flicker_line_step(isp_handle handle, isp_u32 step);
isp_s32 isp_u_yiq_flicker_line_start(isp_handle handle, isp_u32 line_start);

/*ISP Sub Block: Hue*/
isp_s32 isp_u_hue_block(isp_handle handle, void *block_info);
isp_s32 isp_u_hue_bypass(isp_handle handle,isp_u32 bypass);
isp_s32 isp_u_hue_Factor(isp_handle handle,isp_u32 factor);

/*ISP Sub Block: new bad pixel correction*/
isp_s32 isp_u_nbpc_block(isp_handle handle, void *block_info);
isp_s32 isp_u_nbpc_bypass(isp_handle handle, isp_u32 bypass);

/****************************************************Tshark2**********************************************************/
isp_s32 isp_u_pwd_block(isp_handle handle, void *block_info);
isp_s32 isp_u_pwd_slice_size(isp_handle handle, isp_u32 width, isp_u32 height);
isp_s32 isp_u_1d_lsc_block(isp_handle handle, void *block_info);
isp_s32 isp_u_1d_lsc_slice_size(isp_handle handle,isp_u32 width, isp_u32 height);
isp_s32 isp_u_raw_awbm_statistics(isp_handle handle, struct isp_raw_awbm_statistics *awb_info);
isp_s32 isp_u_raw_aem_block(isp_handle handle, void *block_info);
isp_s32 isp_u_raw_aem_bypass(isp_handle handle, void *block_info);
isp_s32 isp_u_raw_aem_mode(isp_handle handle, isp_u32 mode);
isp_s32 isp_u_raw_aem_statistics(isp_handle handle, isp_u32 *r_info, isp_u32 *g_info, isp_u32 *b_info);
isp_s32 isp_u_raw_aem_skip_num(isp_handle handle, isp_u32 skip_num);
isp_s32 isp_u_raw_aem_shift(isp_handle handle, isp_u32 shift);
isp_s32 isp_u_raw_aem_offset(isp_handle handle, isp_u32 x, isp_u32 y);
isp_s32 isp_u_raw_aem_blk_size(isp_handle handle, isp_u32 width, isp_u32 height);
isp_s32 isp_u_raw_aem_slice_size(isp_handle handle, isp_u32 width, isp_u32 height);
isp_s32 isp_u_rgb_gain2_block(isp_handle handle, void *block_info);
isp_s32 isp_u_nlm_block(isp_handle handle, void *block_info);
isp_s32 isp_u_cmc8_block(isp_handle handle, void *block_info);
isp_s32 isp_u_ct_block(isp_handle handle, void *block_info);
isp_s32 isp_u_hsv_block(isp_handle handle, void *block_info);
isp_s32 isp_u_csc_block(isp_handle handle, void *block_info);
isp_s32 isp_u_csc_pic_size(isp_handle handle, isp_u32 width, isp_u32 height);
isp_s32 isp_u_frgb_precdn_block(isp_handle handle, void *block_info);
isp_s32 isp_u_posterize_block(isp_handle handle, void *block_info);
isp_s32 isp_u_raw_afm_block(isp_handle handle, void *block_info);
isp_s32 isp_u_raw_afm_slice_size(isp_handle handle, isp_u32 width, isp_u32 height);
isp_s32 isp_u_raw_afm_type1_statistic(isp_handle handle, void *statis);
isp_s32 isp_u_raw_afm_type2_statistic(isp_handle handle, void *statis);
isp_s32 isp_u_raw_afm_bypass(isp_handle handle, isp_u32 bypass);
isp_s32 isp_u_raw_afm_mode(isp_handle handle, isp_u32 mode);
isp_s32 isp_u_raw_afm_skip_num(isp_handle handle, isp_u32 skip_num);
isp_s32 isp_u_raw_afm_skip_num_clr(isp_handle handle, isp_u32 clear);
isp_s32 isp_u_raw_afm_spsmd_rtgbot_enable(isp_handle handle, isp_u32 enable);
isp_s32 isp_u_raw_afm_spsmd_diagonal_enable(isp_handle handle, isp_u32 enable);
isp_s32 isp_u_raw_afm_spsmd_cal_mode(isp_handle handle, isp_u32 mode);
isp_s32 isp_u_raw_afm_sel_filter1(isp_handle handle, isp_u32 sel_filter);
isp_s32 isp_u_raw_afm_sel_filter2(isp_handle handle, isp_u32 sel_filter);
isp_s32 isp_u_raw_afm_sobel_type(isp_handle handle, isp_u32 type);
isp_s32 isp_u_raw_afm_spsmd_type(isp_handle handle, isp_u32 type);
isp_s32 isp_u_raw_afm_sobel_threshold(isp_handle handle, isp_u32 min, isp_u32 max);
isp_s32 isp_u_raw_afm_spsmd_threshold(isp_handle handle, isp_u32 min, isp_u32 max);
isp_s32 isp_u_raw_afm_win(isp_handle handle, void *win_range);
isp_s32 isp_u_raw_afm_win_num(isp_handle handle, isp_u32 *win_num);
isp_s32 isp_u_yiq_aem_block(isp_handle handle, void *block_info);
isp_s32 isp_u_yiq_aem_slice_size(isp_handle handle, isp_u32 width, isp_u32 height);
isp_s32 isp_u_yiq_aem_ygamma_bypass(isp_handle handle, void *block_info);
isp_s32 isp_u_yiq_aem_bypass(isp_handle handle, void *block_info);
isp_s32 isp_u_yiq_aem_statistics(isp_handle handle, void *addr);
isp_s32 isp_u_yiq_aem_skip_num(isp_handle handle, isp_u32 skip_num);
isp_s32 isp_u_yiq_aem_offset(isp_handle handle, isp_u32 x, isp_u32 y);
isp_s32 isp_u_yiq_aem_blk_size(isp_handle handle, isp_u32 width, isp_u32 height);
isp_s32 isp_u_anti_flicker_block(isp_handle handle, void *block_info);
isp_s32 isp_u_anti_flicker_statistic(isp_handle handle, void *addr);
isp_s32 isp_u_anti_flicker_bypass(isp_handle handle, void *block_info);
isp_s32 isp_u_anti_flicker_transaddr(isp_handle handle, isp_u32 phys_addr);
isp_s32 isp_u_yiq_afm_block(isp_handle handle, void *block_info);
isp_s32 isp_u_yiq_afm_slice_size(isp_handle handle, isp_u32 width, isp_u32 height);
isp_s32 isp_u_yiq_afm_statistic(isp_handle handle, void* statis);
isp_s32 isp_u_yiq_afm_win_num(isp_handle handle, isp_u32 *win_num);
isp_s32 isp_u_yiq_afm_win(isp_handle handle, void* win_range);
isp_s32 isp_u_yuv_precdn_block(isp_handle handle, void *block_info);
isp_s32 isp_u_hist_v1_block(isp_handle handle, void *block_info);
isp_s32 isp_u_hist_slice_size(isp_handle handle, isp_u32 width, isp_u32 height);
isp_s32 isp_u_hist2_block(isp_handle handle, void *block_info);
isp_s32 isp_u_yuv_cdn_block(isp_handle handle, void *block_info);
isp_s32 isp_u_yuv_postcdn_block(isp_handle handle, void *block_info);
isp_s32 isp_u_ygamma_block(isp_handle handle, void *block_info);
isp_s32 isp_u_ydelay_block(isp_handle handle, void *block_info);
isp_s32 isp_u_iircnr_block(isp_handle handle, void *block_info);
isp_s32 isp_u_yrandom_block(isp_handle handle, void *block_info);
isp_s32 isp_u_fetch_start_isp(isp_handle handle, isp_u32 fetch_start);
isp_s32 isp_u_dispatch_block(isp_handle handle, void *block_info);
isp_s32 isp_u_dispatch_ch0_size(isp_handle handle, isp_u32 width, isp_u32 height);
isp_s32 isp_u_arbiter_block(isp_handle handle, void *block_info);
isp_s32 isp_u_comm_block(isp_handle handle, void *block_info);
isp_s32 isp_u_shadow_ctrl_all(isp_handle handle, isp_u32 auto_shadow);
isp_s32 isp_u_awbm_shadow_ctrl(isp_handle handle, isp_u32 shadow_done);
isp_s32 isp_u_ae_shadow_ctrl(isp_handle handle, isp_u32 shadow_done);
isp_s32 isp_u_af_shadow_ctrl(isp_handle handle, isp_u32 shadow_done);
isp_s32 isp_u_afl_shadow_ctrl(isp_handle handle, isp_u32 shadow_done);
isp_s32 isp_u_comm_shadow_ctrl(isp_handle handle, isp_u32 shadow_done);
isp_s32 isp_u_3a_ctrl(isp_handle handle, isp_u32 enable);
isp_s32 isp_u_comm_channel0_y_aem_pos(isp_handle handle, isp_u32 pos);
isp_s32 isp_u_comm_channel1_y_aem_pos(isp_handle handle, isp_u32 pos);
isp_s32 isp_cfg_block(isp_handle handle, void *param_ptr, isp_u32 sub_block);
isp_s32 isp_set_arbiter(isp_handle isp_handler);
isp_s32 isp_set_dispatch(isp_handle isp_handler);
isp_s32 isp_set_feeder_param(isp_handle isp_handler);
isp_s32 isp_get_fetch_addr(struct isp_interface_param *isp_context_ptr, struct isp_dev_fetch_info *fetch_ptr);
isp_s32 isp_get_fetch_addr_v1(struct isp_interface_param_v1 *isp_context_ptr, struct isp_dev_fetch_info_v1 *fetch_ptr);
isp_s32 isp_set_fetch_param(isp_handle isp_handler);
isp_s32 isp_set_fetch_param_v1(isp_handle isp_handler);
isp_s32 isp_get_store_addr(struct isp_interface_param *isp_context_ptr, struct isp_dev_store_info *store_ptr);
isp_s32 isp_get_store_addr_v1(struct isp_interface_param_v1 *isp_context_ptr, struct isp_dev_store_info_v1 *store_ptr);
isp_s32 isp_set_store_param(isp_handle isp_handler);
isp_s32 isp_set_store_param_v1(isp_handle isp_handler);
isp_s32 isp_set_slice_pos_info(struct isp_slice_param *slice_ptr);
isp_s32 isp_set_slice_param(isp_handle isp_handler);
isp_s32 isp_set_slice_border(isp_handle isp_handler);
isp_s32 isp_set_slice_size_v1(isp_handle isp_handler);
isp_s32 isp_cfg_slice_size(isp_handle handle, struct isp_slice_param *slice_ptr);
isp_s32 isp_cfg_slice_size_v1(isp_handle handle, struct isp_slice_param_v1 *slice_ptr);
isp_s32 isp_set_comm_param(isp_handle isp_handler);
isp_s32 isp_set_comm_param_v1(isp_handle isp_handler);
isp_s32 isp_cfg_com_data(isp_handle handle, struct isp_com_param *param_ptr);
isp_s32 isp_cfg_comm_data_v1(isp_handle handle, struct isp_dev_common_info_v1 *param_ptr);
isp_s32 isp_cfg_all_shadow_v1(isp_handle handle, isp_u32 auto_shadow);
isp_s32 isp_cfg_awbm_shadow_v1(isp_handle handle, isp_u32 shadow_done);
isp_s32 isp_cfg_ae_shadow_v1(isp_handle handle, isp_u32 shadow_done);
isp_s32 isp_cfg_af_shadow_v1(isp_handle handle, isp_u32 shadow_done);
isp_s32 isp_cfg_afl_shadow_v1(isp_handle handle, isp_u32 shadow_done);
isp_s32 isp_cfg_comm_shadow_v1(isp_handle handle, isp_u32 shadow_done);
isp_s32 isp_cfg_3a_single_frame_shadow_v1(isp_handle handle, isp_u32 enable);
isp_s32 isp_u_bq_init_bufqueue(isp_handle handle);
isp_s32 isp_u_bq_enqueue_buf(isp_handle handle, uint64_t k_addr, uint64_t u_addr, isp_u32 type);
isp_s32 isp_u_bq_dequeue_buf(isp_handle handle, uint64_t *k_addr, uint64_t *u_addr, isp_u32 type);

/************************************pike*************************************************************/
//isp_s32 isp_u_wdr_block(isp_handle handle, void *block_info);
isp_s32 isp_u_rgb2y_block(isp_handle handle, void *block_info);
isp_s32 isp_u_yuv_nlm_block(isp_handle handle, void *block_info);
isp_s32 isp_u_yuv_nlm_slice_size(isp_handle handle, isp_u32 width, isp_u32 height);
isp_s32 isp_u_uv_prefilter_block(isp_handle handle, void *block_info);


#endif
