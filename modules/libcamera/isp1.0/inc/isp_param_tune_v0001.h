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
#ifndef _ISP_PARAM_TUNE_V0001_H_
#define _ISP_PARAM_TUNE_V0001_H_
/*----------------------------------------------------------------------------*
 **				Dependencies					*
 **---------------------------------------------------------------------------*/
#include <sys/types.h>
/**---------------------------------------------------------------------------*
 **				Compiler Flag					*
 **---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

/**---------------------------------------------------------------------------*
**				Micro Define					*
**----------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**				Data Prototype					*
**----------------------------------------------------------------------------*/
#if 0
struct isp_blc_param_v0000{
	uint32_t mode;
	uint32_t r;
	uint32_t gr;
	uint32_t gb;
	uint32_t b;
};

struct isp_nlc_param_v0000{
	uint32_t r_node[29];
	uint32_t g_node[29];
	uint32_t b_node[29];
	uint32_t l_node[29];
};

struct isp_lnc_map_v0000{
	uint32_t grid;
	uint32_t param_tab[2048];
	uint32_t max_index;
};

struct isp_lnc_param_v0000{
	struct isp_lnc_map_v0000 map[ISP_MAP_NUM][ISP_AWB_NUM];
};

struct isp_awb_param_v0000{
	uint32_t win_start_x;
	uint32_t win_start_y;
	uint32_t win_size_w;
	uint32_t win_size_h;
	uint32_t cali_r[20];
	uint32_t cali_g[20];
	uint32_t cali_b[20];
	uint32_t cali_num;
	uint32_t cali_zone;
	uint32_t r_gain[ISP_AWB_NUM];
	uint32_t g_gain[ISP_AWB_NUM];
	uint32_t b_gain[ISP_AWB_NUM];
	uint32_t gain_num;
};

struct isp_ae_tab_info_v0000{
    uint16_t exposure_tab[250];
    uint16_t gain_tab[250];
    uint32_t start_index;
    uint32_t max_index;
};

struct isp_ae_param_v0000{
	uint32_t skip_frame;
	uint32_t fix_fps;
	uint32_t target_lum;
	uint32_t target_zone;
	int32_t ev[16];
};

struct isp_bpc_param_v0000{
    uint32_t flat_thr;
    uint32_t std_thr;
    uint32_t texture_thr;
};

struct isp_denoise_param_v0000{
    uint32_t write_back;
    uint32_t r_thr;
    uint32_t g_thr;
    uint32_t b_thr;
    uint8_t diswei[19];
    uint8_t ranwei[31];
};

struct isp_grgb_param_v0000{
    uint16_t edge_thr;
    uint16_t diff_thr;
};

struct isp_cfa_param_v0000{
    uint16_t edge_thr;
    uint16_t diff_thr;
};

struct isp_cmc_param_v0000{
    uint16_t matrix[ISP_CMC_NUM][9];
    uint16_t reserved;
};

struct isp_cce_matrix_v0000{
    uint16_t matrix[9];
    uint16_t y_offset;
    uint16_t u_offset;
    uint16_t v_offset;
};

struct isp_cce_param_v0000{
    struct isp_cce_matrix_v0000 mode[16];
};

struct isp_gamma_param_v0000{
    uint16_t axis[26][2];
};

struct isp_cce_uvdiv_v0000{
    uint32_t thrd[7];
};

struct isp_pref_param_v0000{
    uint8_t write_back;
    uint8_t y_thr;
    uint8_t u_thr;
    uint8_t v_thr;
};

struct isp_bright_param_v0000{
    uint32_t factor[16];
};

struct isp_contrast_param_v0000{
    uint32_t factor[16];
};

struct isp_saturation_param_v0000{
    uint32_t factor[16];
};

struct isp_af_param_v0000{
	uint32_t max_step;
	uint32_t stab_period;
};

struct isp_emboss_param_v0000{
    uint32_t step;
};

struct isp_edge_param_v0000{
    uint32_t detail_thr;
    uint32_t smooth_thr;
    uint32_t strength;
};

struct isp_tune_param_v0000{
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
    uint32_t af_bypass;
    uint32_t edge_bypass;
    uint32_t fcs_bypass;
    uint32_t saturation_bypass;

    struct isp_blc_param_v0000 blc;
    struct isp_nlc_param_v0000 nlc;
    struct isp_lnc_param_v0000 lnc;
    struct isp_ae_param_v0000 ae;
    struct isp_awb_param_v0000 awb;
    struct isp_bpc_param_v0000 bpc;
    struct isp_denoise_param_v0000 denoise;
    struct isp_grgb_param_v0000 grgb;
    struct isp_cfa_param_v0000 cfa;
    struct isp_cmc_param_v0000 cmc;
    struct isp_gamma_param_v0000 gamma;
    struct isp_cce_param_v0000 cce;
    struct isp_cce_uvdiv_v0000 uv_div;
    struct isp_pref_param_v0000 pref;
    struct isp_bright_param_v0000 bright;
    struct isp_contrast_param_v0000 contrast;
    struct isp_saturation_param_v0000 saturation;
    struct isp_af_param_v0000 af;
    struct isp_emboss_param_v0000 emboss;
    struct isp_edge_param_v0000 edge;
};

struct isp_tune_info_v0000{
    struct isp_version_info version_info;
    struct isp_param_info param_info[100];
    struct isp_tune_param_v0000 tune_param;
};

struct isp_blc_packet_param_v0000{
    uint32_t verify_id;
    uint32_t version_id;
    uint32_t block_id;
    uint32_t packet_size;
    struct isp_blc_param_v0000 blc;
    uint32_t end_id;

};

struct isp_nlc_packet_param_v0000{
    uint32_t verify_id;
    uint32_t version_id;
    uint32_t block_id;
    uint32_t packet_size;
    struct isp_nlc_param_v0000 nlc;
    uint32_t end_id;
};

struct isp_lnc_packet_param_v0000{
    uint32_t verify_id;
    uint32_t version_id;
    uint32_t block_id;
    uint32_t packet_size;
    struct isp_lnc_param_v0000 lnc;
    uint32_t end_id;
};

struct isp_ae_packet_param_v0000{
    uint32_t verify_id;
    uint32_t version_id;
    uint32_t block_id;
    uint32_t packet_size;
    struct isp_ae_param_v0000 ae;
    uint32_t end_id;
};

struct isp_awb_packet_param_v0000{
    uint32_t verify_id;
    uint32_t version_id;
    uint32_t block_id;
    uint32_t packet_size;
    struct isp_awb_param_v0000 awb;
    uint32_t end_id;
};

struct isp_bpc_packet_param_v0000{
    uint32_t verify_id;
    uint32_t version_id;
    uint32_t block_id;
    uint32_t packet_size;
    struct isp_bpc_param_v0000 bpc;
    uint32_t end_id;
};

struct isp_denoise_packet_param_v0000{
    uint32_t verify_id;
    uint32_t version_id;
    uint32_t block_id;
    uint32_t packet_size;
    struct isp_denoise_param_v0000 denoise;
    uint32_t end_id;
};

struct isp_grgb_packet_param_v0000{
    uint32_t verify_id;
    uint32_t version_id;
    uint32_t block_id;
    uint32_t packet_size;
    struct isp_grgb_param_v0000 grgb;
    uint32_t end_id;
};

struct isp_cfa_packet_param_v0000{
    uint32_t verify_id;
    uint32_t version_id;
    uint32_t block_id;
    uint32_t packet_size;
    struct isp_cfa_param_v0000 cfa;
    uint32_t end_id;
};

struct isp_cmc_packet_param_v0000{
    uint32_t verify_id;
    uint32_t version_id;
    uint32_t block_id;
    uint32_t packet_size;
    struct isp_cmc_param_v0000 cmc;
    uint32_t end_id;
};

struct isp_gamma_packet_param_v0000{
    uint32_t verify_id;
    uint32_t version_id;
    uint32_t block_id;
    uint32_t packet_size;
    struct isp_gamma_param_v0000 gamma;
    uint32_t end_id;
};

struct isp_uvdiv_packet_param_v0000{
    uint32_t verify_id;
    uint32_t version_id;
    uint32_t block_id;
    uint32_t packet_size;
    struct isp_cce_uvdiv_v0000 uvdiv;
    uint32_t end_id;
};

struct isp_pref_packet_param_v0000{
    uint32_t verify_id;
    uint32_t version_id;
    uint32_t block_id;
    uint32_t packet_size;
    struct isp_pref_param_v0000 pref;
    uint32_t end_id;
};

struct isp_bright_packet_param_v0000{
    uint32_t verify_id;
    uint32_t version_id;
    uint32_t block_id;
    uint32_t packet_size;
    struct isp_bright_param_v0000 bright;
    uint32_t end_id;
};

struct isp_contrast_packet_param_v0000{
    uint32_t verify_id;
    uint32_t version_id;
    uint32_t block_id;
    uint32_t packet_size;
    struct isp_contrast_param_v0000 contrast;
    uint32_t end_id;
};

struct isp_hist_packet_param_v0000{
    uint32_t verify_id;
    uint32_t version_id;
    uint32_t block_id;
    uint32_t packet_size;
//    struct isp_hist_param_v0000 hist;
    uint32_t end_id;
};

struct isp_auto_contrast_packet_param_v0000{
    uint32_t verify_id;
    uint32_t version_id;
    uint32_t block_id;
    uint32_t packet_size;
    //struct isp_auto_contrast_param_v0000 auto_contrast;
    uint32_t end_id;
};

struct isp_saturation_packet_param_v0000{
    uint32_t verify_id;
    uint32_t version_id;
    uint32_t block_id;
    uint32_t packet_size;
    struct isp_saturation_param_v0000 saturation;
    uint32_t end_id;
};

struct isp_css_packet_param_v0000{
    uint32_t verify_id;
    uint32_t version_id;
    uint32_t block_id;
    uint32_t packet_size;
    struct isp_saturation_param_v0000 css;
    uint32_t end_id;
};

struct isp_af_packet_param_v0000{
    uint32_t verify_id;
    uint32_t version_id;
    uint32_t block_id;
    uint32_t packet_size;
//    struct isp_af_param_v0000 af;
    uint32_t end_id;
};

struct isp_edge_packet_param_v0000{
    uint32_t verify_id;
    uint32_t version_id;
    uint32_t block_id;
    uint32_t packet_size;
    struct isp_edge_param_v0000 edge;
    uint32_t end_id;
};

struct isp_special_effect_packet_param_v0000{
    uint32_t verify_id;
    uint32_t version_id;
    uint32_t block_id;
    uint32_t packet_size;
    struct isp_special_effect_v0000 special_effect;
    uint32_t end_id;
};

struct isp_hdr_packet_param_v0000{
    uint32_t verify_id;
    uint32_t version_id;
    uint32_t block_id;
    uint32_t packet_size;
    struct isp_hdr_param_v0000 hdr;
    uint32_t end_id;
};

struct isp_global_gain_packet_param_v0000{
    uint32_t verify_id;
    uint32_t version_id;
    uint32_t block_id;
    uint32_t packet_size;
    struct isp_global_gain_param_v0000 global_gain;
    uint32_t end_id;
};

struct isp_chn_gain_packet_param_v0000{
    uint32_t verify_id;
    uint32_t version_id;
    uint32_t block_id;
    uint32_t packet_size;
    struct isp_chn_gain_param_v0000 chn_gain;
    uint32_t end_id;
};
#endif

isp_fun ispGetDownParamFunV0001(uint32_t cmd);
int32_t ispGetParamSizeV0001(uint32_t* param_len);
int32_t ispGetUpParamV0001(void*param_ptr, void* rtn_param_ptr);
/**----------------------------------------------------------------------------*
**                         Compiler Flag                                      **
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif
// End

