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
#ifndef _ISP_EXIF_H_
#define _ISP_EXIF_H_

#define EXIF_APP3	0XFFE3
#define APP3_STATUS	0X1234

struct exif_weight_value {
	int32_t value[2];
	uint32_t weight[2];
};

struct exif_blc_param{
	uint32_t mode;
	uint16_t r;
	uint16_t gr;
	uint16_t gb;
	uint16_t b;
};

struct exif_nlc_param{
	uint16_t r_node[29];
	uint16_t g_node[29];
	uint16_t b_node[29];
	uint16_t l_node[29];
};

struct exif_lnc_param{
	uint16_t grid;
	uint16_t r_pec;
	uint16_t g_pec;
	uint16_t b_pec;
};

struct exif_awb_map{
	uint16_t *addr;
	uint32_t len;		//by bytes
};


struct exif_ae_param{
	uint32_t iso;
	uint32_t exposure;
	uint32_t gain;
	uint32_t cur_lum;
	uint32_t cur_index;
	uint32_t max_index;
};

struct exif_awb_param{
	uint16_t alg_id;
	uint16_t r_gain;
	uint16_t g_gain;
	uint16_t b_gain;
};

struct exif_bpc_param{
	uint16_t flat_thr;
	uint16_t std_thr;
	uint16_t texture_thr;
	uint16_t reserved;
};

struct exif_denoise_param{
	uint32_t write_back;
	uint16_t r_thr;
	uint16_t g_thr;
	uint16_t b_thr;
	uint8_t diswei[19];
	uint8_t ranwei[31];
	uint8_t diswei_level;
	uint8_t ranwei_level;
};

struct exif_grgb_param{
	uint16_t edge_thr;
	uint16_t diff_thr;
};

struct exif_cfa_param{
	uint16_t edge_thr;
	uint16_t diff_thr;
};

struct exif_cmc_param{
	uint16_t matrix[9];
	uint16_t reserved;
};

struct exif_cce_parm{
	uint16_t matrix[9];
	uint16_t y_shift;
	uint16_t u_shift;
	uint16_t v_shift;
};

struct exif_gamma_param{
	uint16_t axis[2][26];
};

struct exif_cce_uvdiv{
	uint8_t thrd[7];
	uint8_t t[2];
	uint8_t m[3];
};

struct exif_pref_param{
	uint8_t write_back;
	uint8_t y_thr;
	uint8_t u_thr;
	uint8_t v_thr;
};

struct exif_bright_param{
	uint8_t factor;
};

struct exif_contrast_param{
	uint8_t factor;
};

struct exif_hist_param
{
	uint16_t low_ratio;
	uint16_t high_ratio;
	uint8_t mode;
	uint8_t reserved2;
	uint8_t reserved1;
	uint8_t reserved0;
};

struct exif_auto_contrast_param{
	uint8_t mode;
	uint8_t reserved2;
	uint8_t reserved1;
	uint8_t reserved0;
};

struct exif_saturation_param{
	uint8_t factor;
};

struct exif_af_param{
	uint8_t magic[16];
	uint16_t alg_id;
	uint16_t cur_step;
	uint16_t edge_info[32];
	uint32_t denoise_lv;
	uint32_t win_num;
	uint32_t suc_win;
	uint32_t mode;
	uint32_t step_cnt;
	uint16_t win[9][4];
	uint16_t pos[32];
	uint32_t value[9][32];
	uint16_t time[32];
};

struct exif_emboss_param{
	uint8_t step;
	uint8_t reserved2;
	uint8_t reserved1;
	uint8_t reserved0;
};

struct exif_edge_info{
	uint8_t detail_thr;
	uint8_t smooth_thr;
	uint8_t strength;
	uint8_t reserved;
};

struct exif_global_gain_param{
	uint32_t gain;
};

struct exif_chn_gain_param{
	uint8_t r_gain;
	uint8_t g_gain;
	uint8_t b_gain;
	uint8_t reserved0;
	uint16_t r_offset;
	uint16_t g_offset;
	uint16_t b_offset;
	uint16_t reserved1;
};

struct exif_flash_cali_param{
	uint16_t effect;
	uint16_t lum_ratio;
	uint16_t r_ratio;
	uint16_t g_ratio;
	uint16_t b_ratio;
};

struct exif_css_param{
	uint8_t low_thr[7];
	uint8_t lum_thr;
	uint8_t low_sum_thr[7];
	uint8_t chr_thr;
	uint8_t ratio[8];
};

struct eixf_read_check{
	uint16_t app_head;
	uint16_t status;
};

struct exif_smart_lsc_param {
	uint32_t flat_num;
	uint32_t flat_status1;
	uint32_t flat_status2;
	uint32_t stat_r_var;
	uint32_t stat_b_var;
	uint32_t cali_status;

	uint32_t alg_calc_cnt;
	struct exif_weight_value cur_index;
	struct exif_weight_value calc_index;
	uint32_t cur_ct;

	uint32_t alg2_enable;
	uint32_t alg2_seg1_num;
	uint32_t alg2_seg2_num;
	uint32_t alg2_seg_num;
	uint32_t alg2_seg_valid;
	uint32_t alg2_cnt;
	uint32_t center_same_num[4];

};

struct exif_smart_param {
	uint32_t smart;
	uint32_t smart_base_gain;

	uint8_t denoise_lum_thr;
	uint8_t denoise_start_index;
	uint8_t denoise_start_zone;
	uint8_t reserved1;

	uint8_t  smart_pref_y_outdoor_index;
	uint8_t smart_pref_y_min_index;
	uint8_t smart_pref_y_mid_index;
	uint8_t smart_pref_y_max_index;
	uint8_t smart_pref_y_cur;
	uint8_t reserved2;
	uint16_t reserved3;

	uint8_t smart_pref_uv_outdoor_index;
	uint8_t smart_pref_uv_min_index;
	uint8_t smart_pref_uv_mid_index;
	uint8_t smart_pref_uv_max_index;
	uint8_t smart_pref_uv_cur;
	uint8_t reserved4;
	uint16_t reserved5;

	uint8_t smart_denoise_diswei_outdoor_index;
	uint8_t smart_denoise_diswei_min_index;
	uint8_t smart_denoise_diswei_mid_index;
	uint8_t smart_denoise_diswei_max_index;
	uint8_t smart_denoise_diswei_cur;
	uint8_t reserved6;
	uint16_t reserved7;

	uint8_t smart_denoise_ranwei_outdoor_index;
	uint8_t smart_denoise_ranwei_min_index;
	uint8_t smart_denoise_ranwei_mid_index;
	uint8_t smart_denoise_ranwei_max_index;
	uint8_t smart_denoise_ranwei_cur;
	uint8_t reserved8;
	uint16_t reserved9;

	uint8_t smart_denoise_soft_y_outdoor_index;
	uint8_t smart_denoise_soft_y_min_index;
	uint8_t smart_denoise_soft_y_mid_index;
	uint8_t smart_denoise_soft_y_max_index;
	uint8_t smart_denoise_soft_y_cur;
	uint8_t reserved10;
	uint16_t reserved11;

	uint8_t smart_denoise_soft_uv_outdoor_index;
	uint8_t smart_denoise_soft_uv_min_index;
	uint8_t smart_denoise_soft_uv_mid_index;
	uint8_t smart_denoise_soft_uv_max_index;
	uint8_t smart_denoise_soft_uv_cur;
	uint8_t reserved12;
	uint16_t reserved13;

	uint8_t gamma_start;
	uint8_t gamma_num;
	uint8_t gamma_zone;
	uint8_t reserved14;
	uint8_t gamma_thr[4];

	uint8_t gamma_lum_thr;
	uint8_t smart_edge_max_index;
	uint8_t smart_edge_min_index;
	uint8_t smart_edge_cur;

	uint8_t smart_sta_start_index;
	uint8_t smart_sta_low_thr;
	uint8_t smart_sta_ratio1;
	uint8_t smart_sta_ratio;

	uint16_t lum_cali_index;
	uint16_t reserved16;
	uint32_t lum_cali_lux;
};

typedef struct exif_isp_info{
	uint32_t is_exif_validate;
	uint32_t tool_version;
	uint32_t version_id;
	uint32_t info_len;
	uint32_t blc_bypass;
	uint32_t nlc_bypass;
	uint32_t lnc_bypass;
	uint32_t ae_bypass;
	uint32_t awb_bypass;
	uint32_t bpc_bypass;
	uint32_t denoise_bypass;
	uint32_t grgb_bypass;
	uint32_t cmc_bypass;
	uint32_t gamma_bypass;
	uint32_t uvdiv_bypass;
	uint32_t pref_bypass;
	uint32_t bright_bypass;
	uint32_t contrast_bypass;
	uint32_t hist_bypass;
	uint32_t auto_contrast_bypass;
	uint32_t af_bypass;
	uint32_t edge_bypass;
	uint32_t fcs_bypass;
	uint32_t css_bypass;
	uint32_t saturation_bypass;
	uint32_t hdr_bypass;
	uint32_t glb_gain_bypass;
	uint32_t chn_gain_bypass;

	struct exif_blc_param blc;
	struct exif_nlc_param nlc;
	struct exif_lnc_param lnc;
	struct exif_ae_param ae;
	struct exif_awb_param awb;
	struct exif_bpc_param bpc;
	struct exif_denoise_param denoise;
	struct exif_grgb_param grgb;
	struct exif_cfa_param cfa;
	struct exif_cmc_param cmc;
	struct exif_gamma_param gamma;
	struct exif_cce_parm cce;
	struct exif_cce_uvdiv uv_div;
	struct exif_pref_param pref;
	struct exif_bright_param bright;
	struct exif_contrast_param contrast;
	struct exif_hist_param hist;
	struct exif_auto_contrast_param auto_contrast;
	struct exif_saturation_param saturation;
	struct exif_css_param css;
	struct exif_af_param af;
	struct exif_edge_info edge;
	struct exif_emboss_param emboss;
	struct exif_global_gain_param global;
	struct exif_chn_gain_param chn;
	struct exif_flash_cali_param flash;
	struct eixf_read_check exif_check;
	struct exif_smart_param smart_adjust;
	struct exif_smart_lsc_param smart_lsc;
}EXIF_ISP_INFO_T;

#endif //_ISP_EXIF_H_
