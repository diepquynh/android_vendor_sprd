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
#ifndef _SENSOR_RAW_H_
#define _SENSOR_RAW_H_

#ifndef WIN32
#include <sys/types.h>
#else
#include ".\isp_type.h"
#endif
#include "isp_type.h"


#define MAX_MODE_NUM 16

#define MAX_SCENEMODE_NUM 16
#define MAX_SPECIALEFFECT_NUM 16

#define SENSOR_AWB_CALI_NUM 0x09
#define SENSOR_PIECEWISE_SAMPLE_NUM 0x10
#define SENSOR_ENVI_NUM 6

#define RAW_INFO_END_ID 0x71717567

#define SENSOR_MULTI_MODE_FLAG  0xABCDDCBA

#define AE_FLICKER_NUM 2
#define AE_ISO_NUM_NEW 8
#define AE_WEIGHT_TABLE_NUM 3
#define AE_SCENE_NUM 8

enum isp_scene_mode {
	ISP_SCENEMODE_AUTO = 0x00,
	ISP_SCENEMODE_NIGHT,
	ISP_SCENEMODE_SPORT,
	ISP_SCENEMODE_PORTRAIT,
	ISP_SCENEMODE_LANDSCAPE,
	ISP_SCENEMODE_SNOW,
	ISP_SCENEMODE_FIREWORK,
	ISP_SCENEMODE_DUSK,
	ISP_SCENEMODE_AUTUMN,
	ISP_SCENEMODE_TEXT,
	ISP_SCENEMODE_BACKLIGHT,
	ISP_SCENEMODE_MAX
};

enum isp_special_effect_mode {
	ISP_EFFECT_NORMAL = 0x00,
	ISP_EFFECT_GRAY,
	ISP_EFFECT_WARM,
	ISP_EFFECT_GREEN,
	ISP_EFFECT_COOL,
	ISP_EFFECT_ORANGE,
	ISP_EFFECT_NEGTIVE,
	ISP_EFFECT_OLD,
	ISP_EFFECT_EMBOSS,
	ISP_EFFECT_POSTERIZE,
	ISP_EFFECT_CARTOON,
	ISP_EFFECT_MAX
};





////////////////////////////////////////////////////////////
/********** sharkl tuning paramater **********/
////////////////////////////////////////////////////////////


struct sensor_flash_attrib_param {
	uint32_t r_sum;
	uint32_t gr_sum;
	uint32_t gb_sum;
	uint32_t b_sum;
};

struct sensor_flash_attrib_cali{
	struct sensor_flash_attrib_param global;
	struct sensor_flash_attrib_param random;
};

struct sensor_flash_cali_param{
	uint16_t auto_threshold;
	uint16_t r_ratio;
	uint16_t g_ratio;
	uint16_t b_ratio;
	struct sensor_flash_attrib_cali attrib;
	uint16_t lum_ratio;
	uint16_t reserved1;
	uint32_t reserved0[19];
};


struct sensor_envi_detect_param {
	uint32_t enable;
	uint32_t num;
	struct isp_range envi_range[SENSOR_ENVI_NUM];
};



////////////////////////////////////////////////////////////
/********** shark2 tuning paramater **********/
////////////////////////////////////////////////////////////
#define _HAIT_MODIFIED_FLAG
#define SENSOR_CCE_NUM 0x09
#define SENSOR_CMC_NUM 0x09
#define SENSOR_CTM_NUM 0x09
#define SENSOR_HSV_NUM 0x09
#define SENSOR_LENS_NUM 0x09
#define SENSOR_BLC_NUM 0x09
#define SENSOR_MODE_NUM 0x02
#define SENSOR_AWBM_WHITE_NUM 0x05
#define SENSOR_LNC_RC_NUM 0x04
#define SENSOR_HIST2_ROI_NUM 0x04
#define SENSOR_GAMMA_POINT_NUM	257
#define SENSOR_GAMMA_NUM 9
#define SENSOR_LEVEL_NUM 16
#define SENSOR_CMC_POINT_NUM 9
#define SENSOR_SMART_LEVEL_NUM 25
#define SENSOR_SMART_LEVEL_DEFAULT 15


struct sensor_gamma_curve {
	struct isp_point points[SENSOR_GAMMA_POINT_NUM];
};

/***********************************************************************************/
enum isp_nr_mode_type {
	ISP_NR_COMMON_MODE = 0x10,
	ISP_NR_CAP_MODE,
	ISP_NR_VIDEO_MODE,
	ISP_NR_PANO_MODE,
	ISP_NR_MODE_MAX
};

//new added NR pointer for different mode
struct nr_param_ptr {
	isp_uint *common_param_ptr;
	isp_uint *capture_param_ptr;
	isp_uint *video_param_ptr;
	isp_uint *pano_param_ptr;
};

/************************************************************************************/
//Pre-global gain
struct sensor_pre_global_gain_param {
	uint16_t gain;
	uint16_t reserved;
};

/************************************************************************************/
//Black level correction
struct sensor_blc_v1_offset {
	uint16_t r;
	uint16_t gr;
	uint16_t gb;
	uint16_t b;
};

struct sensor_blc_param_v1 {
	uint32_t cur_index;
	struct sensor_blc_v1_offset tab[SENSOR_BLC_NUM];
};


/************************************************************************************/
//rgb gain
struct sensor_rgb_gain_param {
	uint16_t glb_gain;
	uint16_t r_gain;
	uint16_t g_gain;
	uint16_t b_gain;
};


/************************************************************************************/
//Pre-Wavelet denoise
/*
PWD:
1. sensor_pwd_shrink will have 3 sets for low light, normal light and high light each
2. sensor_pwd_radial is Algo reserved.
*/
struct sensor_pwd_radial {
	uint32_t radial_bypass;
	struct isp_pos center_pos;
	struct isp_pos center_square;
	uint32_t r2_thr;
	uint32_t p1_param;
	uint32_t p2_param;
	uint16_t gain_max_thr;
	uint16_t reserved0;
};

struct sensor_pwd_level {
	uint16_t gain_thrs0;
	uint16_t gain_thrs1;
	uint16_t nsr_thr_slope;
	uint16_t offset;
	uint16_t bitshif1;
	uint16_t bitshif0;
	uint32_t lum_ratio;
	struct sensor_pwd_radial radial_var;
	uint16_t addback;
	uint16_t reserved1;
	uint32_t bypass;
};

struct sensor_pwd_param {
	isp_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_pwd_level pwd_level;
	isp_u32 strength_level;
	isp_u32 reserved2[2];
};


/************************************************************************************/
//non-linear correction
struct sensor_nlc_v1_param {
	uint16_t r_node[29];
	uint16_t g_node[29];
	uint16_t b_node[29];
	uint16_t l_node[27];
};



/************************************************************************************/
// 2D mesh-grid lens shading correction
struct sensor_lens_map_addr {
	uint32_t envi;
	uint32_t ct;
	uint32_t width;
	uint32_t height;
	uint32_t grid;
	uint32_t len;
	uint32_t offset;
};

struct sensor_2d_lsc_param {
	struct isp_sample_point_info cur_idx;
	isp_u32 tab_num;
	struct sensor_lens_map_addr map[SENSOR_LENS_NUM];
	void *data_area;
};

/************************************************************************************/
// 1D-lens shading correction
struct sensor_multi_curve_discription {
	struct isp_pos center_pos;
	struct isp_pos delta_square;
	struct isp_curve_coeff coef;//p1:[-32768 32767]
};

struct sensor_1d_lsc_map {
	uint32_t gain_thrs1;			//wavelet_denoise_thrs1 value
	struct sensor_multi_curve_discription curve_distcptn[SENSOR_LNC_RC_NUM];
};

struct sensor_1d_lsc_param {
	struct isp_sample_point_info cur_idx;
	struct sensor_1d_lsc_map map[SENSOR_LENS_NUM];
};


/************************************************************************************/
//Auto white balance block parameters
struct sensor_awb_thr {
	uint16_t r_thr;
	uint16_t g_thr;
	uint16_t b_thr;
	uint16_t reserved;
};

struct sensor_awb_gain {
	uint16_t r_gain;
	uint16_t b_gain;
	uint16_t gr_gain;
	uint16_t gb_gain;
};

struct sensor_awb_offset {		//AWB correction offset
	uint16_t r_offset;
	uint16_t b_offset;
	uint16_t gr_offset;
	uint16_t gb_offset;
};

struct sensor_awbc_param {
	uint16_t alpha_bypass;
	uint16_t buf_sel;
	uint16_t alpha_val;
	uint16_t reserved;
	struct sensor_awb_gain awbc_gain;
	struct sensor_awb_offset awbc_offset;
	struct sensor_awb_gain awbc_gain_buf;
	struct sensor_awb_offset awbc_offset_buf;
	struct sensor_awb_thr awbc_thr;
};

struct sensor_awbm_param {
	uint32_t comp_1d_bypass;
	uint32_t skip_num;
	struct isp_pos win_start;
	struct isp_size win_size;
	struct isp_pos_rect white_area[SENSOR_AWBM_WHITE_NUM];
	struct sensor_awb_thr low_thr;
	struct sensor_awb_thr high_thr;
};

struct sensor_awb_param {
	struct sensor_awbc_param awbc;
	struct sensor_awbm_param awbm;
};



/************************************************************************************/
//Auto-exposure monitor in RGB domain
struct sensor_rgb_aem_param {
	uint32_t  aem_skip_num;
	struct isp_pos win_start;
	struct isp_size win_size;
};



/************************************************************************************/
//Bad Pixel Correction
/*
BPC:
1. All BPC related parameters are Algo reserved.
2. intercept_b, slope_k and lut level will have 3 sets for low light, normal light and high light.
*/

struct sensor_bpc_flat {
	uint8_t flat_factor;
	uint8_t spike_coeff;
	uint8_t dead_coeff;
	uint8_t reserved;
	uint16_t lut_level[8];
	uint16_t slope_k[8];
	uint16_t intercept_b[8];
};

struct sensor_bpc_rules {
	struct isp_range k_val;
	uint8_t ktimes;
	uint8_t delt34;
	uint8_t safe_factor;
	uint8_t reserved;
};

struct sensor_bpc_comm {
	uint8_t bpc_mode;
	uint8_t mask_mode;
	uint8_t map_done_sel;
	uint8_t new_old_sel;
};

struct sensor_bpc_pvd {
	uint16_t bypass_pvd;
	uint16_t cntr_theshold;
};

struct sensor_bpc_level {
	struct sensor_bpc_flat bpc_flat;
	struct sensor_bpc_rules bpc_rules;
	struct sensor_bpc_pvd bpc_pvd;
	uint32_t bypass;
};

struct sensor_bpc_param_v1 {
	isp_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_bpc_comm bpc_comm;
	struct sensor_bpc_level bpc_level;
	isp_u32 strength_level;
	isp_u32 reserved[2];
};


/************************************************************************************/
//GrGb Correction
struct sensor_grgb_v1_level {
	uint16_t edge_thd;
	uint16_t diff_thd;
	uint16_t grid_thd;
	uint16_t reserved;
	uint32_t bypass;
};

struct sensor_grgb_v1_param {
	isp_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_grgb_v1_level grgb_level;
	isp_u32 strength_level;
	isp_u32 reserved[2];
};


/************************************************************************************/
//rgb gain 2
struct sensor_rgb_gain2_param {
	uint16_t r_gain;
	uint16_t g_gain;
	uint16_t b_gain;
	uint16_t r_offset;
	uint16_t g_offset;
	uint16_t b_offset;
};




/************************************************************************************/
/*
	1. have 3 sets for low light,normal light and high light;
	2.these parameters should be changed under different iso value;
*/
struct sensor_nlm_flat_degree {
	uint8_t strength;
	uint8_t cnt;
	uint16_t thresh;
};
/*
	1.have 3 sets for low light, normal light and high light;
	2.these parameters should be changed under different iso value;
*/
struct sensor_nlm_flat {
	uint8_t flat_opt_bypass;
	uint8_t flat_thresh_bypass;//algorithm reserved
	uint16_t flat_opt_mode;
	struct sensor_nlm_flat_degree dgr_ctl[5];
};

/*
	1.have  3 sets for low light,normal light and high light;
	2.these parameters should be changed under different iso value;
*/
struct sensor_nlm_direction {
	uint8_t direction_mode_bypass;
	uint8_t dist_mode;
	uint8_t w_shift[3];
	uint8_t cnt_th;
	uint8_t reserved[2];
	uint16_t diff_th;
	uint16_t tdist_min_th;
};
/*
	1have 3 sets for low light,normal light and high light;
	2.these parameters should be changed under different iso value;
*/
struct sensor_nlm_den_strength {
	uint16_t den_length;
	uint16_t texture_dec;
	uint16_t lut_w[72];
};
/*
	1.have 3 sets for low light,normal light and high light;
	2.these parameters should be changed under different iso value;
*/

struct sensor_nlm_level{
	struct sensor_nlm_flat nlm_flat;
	struct sensor_nlm_direction nlm_dic;
	struct sensor_nlm_den_strength nlm_den;
	uint16_t imp_opt_bypass;
	uint16_t add_back;
	uint32_t bypass;
};

struct sensor_vst_level{
	uint32_t vst_param[1024];
};

struct sensor_ivst_level{
	uint32_t ivst_param[1024];
};

struct sensor_flat_offset_level{
	uint32_t flat_offset_param[1024];
};

struct sensor_nlm_param {
	isp_uint *param_nlm_ptr; /* if not null, read from noise/xxxx_param.h */
	isp_uint * param_vst_ptr; /* if not null, read from noise/xxxx_param.h */
	isp_uint * param_ivst_ptr; /* if not null, read from noise/xxxx_param.h */
	isp_uint * param_flat_offset_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_nlm_level nlm_level;
	struct sensor_vst_level vst_level;
	struct sensor_ivst_level ivst_level;
	struct sensor_flat_offset_level flat_offset_level;
	isp_u32 strength_level;
	isp_u32 reserved[2];
};


/************************************************************************************/
// CFA
/*
CFA: All CFA related parameters are Algo reserved.
*/

struct sensor_cfae_comm {
	uint8_t grid_gain;
	uint8_t avg_mode;
	uint8_t inter_chl_gain;		// CFA inter-channel color gradient gain typical:1(suggest to keep the typical value)
	uint8_t reserved0;
	uint16_t grid_min_tr;
	uint16_t reserved1;
};

struct sensor_cfae_ee_thr {
	uint16_t cfai_ee_edge_tr;
	uint16_t cfai_ee_diagonal_tr;
	uint16_t cfai_ee_grid_tr;
	uint16_t cfai_doee_clip_tr;
	uint16_t strength_tr_pos;
	uint16_t strength_tr_neg;
};

struct sensor_cfae_ee {
	uint16_t cfa_ee_bypass;
	uint16_t doee_base;
	uint16_t cfai_ee_saturation_level;
	uint16_t ee_strength_pos;
	uint16_t ee_strength_neg;
	uint16_t reserved;
	struct sensor_cfae_ee_thr ee_thr;
};

struct sensor_cfae_undir {
	uint16_t cfa_uni_dir_intplt_tr;
	uint16_t cfai_ee_uni_dir_tr;
	uint16_t plt_diff_tr;
	uint16_t reserved;
};

struct sensor_cfae_level {
	struct sensor_cfae_comm cfa_comm;
	struct sensor_cfae_ee cfa_ee;
	struct sensor_cfae_undir cfa_undir;
};

struct sensor_cfa_param_v1 {
	isp_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_cfae_level cfae_level;
	isp_u32 strength_level;
	isp_u32 reserved[2];
};


/*
CMC:
1. buf_sel, alpha_val, cur_indx and buffer are Algo reserved.
2. matrix will have at least 2 sets for high and low CT seperately.
*/
struct sensor_cmc_v1_param {
	uint32_t alpha_bypass;
	uint32_t buf_sel;
	uint32_t alpha_val;
	struct isp_sample_point_info cur_idx;
	uint16_t matrix[SENSOR_CMC_NUM][9];
	uint16_t buffer[SENSOR_CMC_NUM][9];
};


/************************************************************************************/
//Gamma Correction in full RGB domain
struct sensor_frgb_gammac_param {
	struct isp_sample_point_info cur_idx_info;
	struct sensor_gamma_curve curve_tab[SENSOR_GAMMA_NUM];

};


/************************************************************************************/
//for ctm
struct sensor_ctm_param {
	struct isp_sample_point_info cur_idx;
	struct isp_data_bin_info map[SENSOR_CTM_NUM];
	void* data_area;
};



/************************************************************************************/
// CCE
struct sensor_cce_matrix_info {
	uint16_t matrix[9];
	uint16_t y_shift;
	uint16_t u_shift;
	uint16_t v_shift;
};

struct sensor_cce_param_v1 {
	uint32_t mode;
	isp_u32 cur_idx;
	struct sensor_cce_matrix_info tab[SENSOR_CCE_NUM];
	struct sensor_cce_matrix_info specialeffect[MAX_SPECIALEFFECT_NUM];
};



/************************************************************************************/
//for hsv
struct sensor_hsv_param {
	struct isp_sample_point_info cur_idx;
	struct isp_data_bin_info map[SENSOR_HSV_NUM];
	void* data_area;
	struct isp_data_bin_info specialeffect[MAX_SPECIALEFFECT_NUM];
	void* specialeffect_data_area;
};



/************************************************************************************/
// radial color denoise
struct sensor_radial_csc_map {
	uint32_t red_thr;
	uint32_t blue_thr;
	uint32_t max_gain_thr;
	struct sensor_multi_curve_discription r_curve_distcptn;
	struct sensor_multi_curve_discription b_curve_distcptn;
};

struct sensor_radial_csc_param {
	struct isp_sample_point_info cur_idx;
	struct sensor_radial_csc_map map[SENSOR_LENS_NUM];
};

/************************************************************************************/
//pre-color noise remove in rgb domain

struct sensor_rgb_precdn_level{
	uint16_t thru0;
	uint16_t thru1;
	uint16_t thrv0;
	uint16_t thrv1;
	uint16_t median_mode;
	uint16_t median_thr;
	uint32_t bypass;
};

struct sensor_rgb_precdn_param {
	isp_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_rgb_precdn_level rgb_precdn_level;
	uint32_t strength_level;
	isp_u32 reserved[2];
};

/************************************************************************************/
//Posterize, is Special Effect
struct sensor_posterize_param {
	uint8_t pstrz_bot[8];
	uint8_t pstrz_top[8];
	uint8_t pstrz_out[8];

	uint8_t specialeffect_bot[MAX_SPECIALEFFECT_NUM][8];
	uint8_t specialeffect_top[MAX_SPECIALEFFECT_NUM][8];
	uint8_t specialeffect_out[MAX_SPECIALEFFECT_NUM][8];
};



/************************************************************************************/
//AF monitor in rgb domain
struct sensor_rgb_afm_sobel {
	uint32_t af_sobel_type;			//sobel filter win control
	struct isp_range sobel;		// filter thresholds
};

struct sensor_rgb_afm_filter_sel {
	uint8_t af_sel_filter1;		//filter select control
	uint8_t af_sel_filter2;		//filter select control
	uint8_t reserved[2];
};


struct sensor_rgb_afm_spsmd {
	uint8_t af_spsmd_rtgbot;		//filter data mode control
	uint8_t af_spsmd_diagonal;	//filter data mode control
	uint8_t af_spsmd_cal_mode;	//data output mode control
	uint8_t af_spsmd_type;		//data output mode control
	struct isp_range spsmd;		// filter thresholds
};

struct sensor_rgb_afm_param {
	uint8_t skip_num;
	struct sensor_rgb_afm_filter_sel filter_sel;
	struct sensor_rgb_afm_spsmd spsmd_ctl;
	struct sensor_rgb_afm_sobel sobel_ctl;
	struct isp_rect windows[25];
};

/************************************************************************************/
//Auto-exposure monitor in YIQ domain
/*
GMA:
GMA will have two sets for indoor and outdoor seperately.
*/
struct sensor_y_aem_param {
	uint32_t y_gamma_bypass;
	uint32_t skip_num;
	uint32_t cur_index;
	struct isp_pos win_start;
	struct isp_size win_size;
	struct sensor_gamma_curve gamma_tab[9];		////should change to struct sensor_y_gamma tab;  (yangyang.liu)
};



/************************************************************************************/
//Anti-flicker
struct sensor_y_afl_param {
	uint8_t skip_num;
	uint8_t line_step;
	uint8_t frm_num;
	uint8_t reserved0;
	uint16_t v_height;
	uint16_t reserved1;
	struct isp_pos col_st_ed;
};




/************************************************************************************/
// AF monitor in YUV domain
struct sensor_y_afm_level {
	uint32_t iir_bypass;
	uint8_t skip_num;
	uint8_t afm_format;	//filter choose control
	uint8_t afm_position_sel;	//choose afm after CFA or auto contrust adjust
	uint8_t shift;
	uint16_t win[25][4];
	uint16_t coef[11];		//int16
	uint16_t reserved1;
};

struct sensor_y_afm_param {
	isp_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_y_afm_level y_afm_level;
	uint32_t strength_level;
	isp_u32 reserved[2];
};


/************************************************************************************/
//pre-color denoise in YUV domain

struct sensor_yuv_precdn_comm {
	uint8_t mode;
	uint8_t median_writeback_en;
	uint8_t median_mode;
	uint8_t den_stren;
	uint8_t uv_joint;
	uint8_t median_thr_u[2];
	uint8_t median_thr_v[2];
	uint16_t median_thr;
	uint8_t uv_thr;
	uint8_t y_thr;
	uint8_t reserved[3];
};

struct sensor_yuv_precdn_level{
	struct sensor_yuv_precdn_comm precdn_comm;
	uint8_t r_segu[2][7];							// param1
	uint8_t r_segv[2][7];							// param2
	uint8_t r_segy[2][7];							// param3
	uint8_t dist_w[25];							// param4
	uint8_t reserved0;
	uint32_t bypass;
};

struct sensor_yuv_precdn_param {
	isp_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_yuv_precdn_level yuv_precdn_level;
	isp_u32 strength_level;
	isp_u32 reserved3[2];
};

/************************************************************************************/
//pre-filter in YUV domain
struct sensor_prfy_level {
	uint16_t prfy_writeback;
	uint16_t nr_thr_y;
	uint32_t bypass;
};

struct sensor_prfy_param {
	isp_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_prfy_level prfy_level;
	isp_u32 strength_level;
	isp_u32 reserved3[2];
};



/************************************************************************************/
//Brightness
struct sensor_bright_param {
	int8_t factor[16];
	uint32_t cur_index;

	int8_t scenemode[MAX_SCENEMODE_NUM];
};



/************************************************************************************/
//Contrast
struct sensor_contrast_param {
	uint8_t factor[16];
	uint32_t cur_index;

	uint8_t scenemode[MAX_SCENEMODE_NUM];
};




/************************************************************************************/
//Hist in YUV domain
struct sensor_yuv_hists_param {
	uint8_t skip_num;
	uint8_t pof_rst_en;		// use pof signle reset buf
	uint8_t version_sel1;		// 0: new version 1: old version
	uint8_t version_sel2;		// 0: old version 1: new version
	uint16_t high_sum_ratio;
	uint16_t low_sum_ratio;
	uint8_t diff_th;
	uint8_t small_adj;
	uint8_t big_adj;
	uint8_t reserved;
};

/************************************************************************************/
//Hist 2
struct sensor_yuv_hists2_param {
	uint32_t skip_num;
	struct isp_pos_rect roi_point[SENSOR_HIST2_ROI_NUM];
};



/************************************************************************************/
//Auto-contrast
struct sensor_auto_contrast_param_v1 {
	uint8_t aca_mode;
	uint8_t reserved[3];
	struct isp_range out;
	struct isp_range in;
};



/************************************************************************************/
//Color Denoise

struct sensor_uv_cdn_level {
	uint8_t median_thru0;
	uint8_t median_thru1;
	uint8_t median_thrv0;
	uint8_t median_thrv1;
	uint8_t u_ranweight[31];
	uint8_t v_ranweight[31];
	uint8_t cdn_gaussian_mode;
	uint8_t median_mode;
	uint8_t median_writeback_en;
	uint8_t filter_bypass;
	uint16_t median_thr;
	uint32_t bypass;
};


struct sensor_uv_cdn_param {
	isp_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_uv_cdn_level uv_cdn_level;
	isp_u32 strength_level;
	isp_u32 reserved2[2];
};



/************************************************************************************/
//Edge Enhancement
struct sensor_ee_pn {
	uint8_t negative;
	uint8_t positive;
};
/*
sensor_ee_flat:
All parameters are Algo reserved.
*/
struct sensor_ee_flat {
	uint8_t thr1;
	uint8_t thr2;
	uint16_t reserved;
};
/*
sensor_ee_txt:
All parameters are Algo reserved.
*/
struct sensor_ee_txt {
	uint8_t ee_txt_thr1;
	uint8_t ee_txt_thr2;
	uint8_t ee_txt_thr3;
	uint8_t reserved;
};

/*
sensor_ee_corner:
ee_corner_th, ee_corner_gain and ee_corner_sm are Algo reserved.
*/
struct sensor_ee_corner {
	uint8_t ee_corner_cor;
	uint8_t reserved0[3];
	struct sensor_ee_pn ee_corner_th;
	struct sensor_ee_pn ee_corner_gain;
	struct sensor_ee_pn ee_corner_sm;
	uint16_t reserved1;
};
/*
sensor_ee_smooth:
All parameters are Algo reserved.
*/
struct sensor_ee_smooth {
	uint8_t ee_edge_smooth_mode;
	uint8_t ee_flat_smooth_mode;
	uint8_t ee_smooth_thr;
	uint8_t ee_smooth_strength;
};
/*
sensor_ee_ipd:
ipd_flat_thr is Algo reserved.
*/
struct sensor_ee_ipd {
	uint8_t ipd_bypass;
	uint8_t ipd_flat_thr;
	uint16_t reserved;
};
/*
sensor_ee_ratio:
All parameteres are Algo reserved.
*/
struct sensor_ee_ratio {
	uint8_t ratio1;
	uint8_t ratio2;
	uint16_t reserved;
};
/*
sensor_ee_t_cfg:
All parameters are Algo reserved.
*/
struct sensor_ee_t_cfg {
	uint16_t ee_t1_cfg;
	uint16_t ee_t2_cfg;
	uint16_t ee_t3_cfg;
	uint16_t ee_t4_cfg;
};
/*
sensor_ee_r_cfg:
All paramteres are Algo reserved.
*/
struct sensor_ee_r_cfg {
	uint8_t ee_r1_cfg;
	uint8_t ee_r2_cfg;
	uint8_t ee_r3_cfg;
	uint8_t reserved;
};

/*
sensor_ee_param:
ee_mode, sigma and chip_after_smooth_en are Algo reserved.
There will be at least 3 sets of edge_thr_d for low light, normal light and high light.
*/

struct sensor_ee_level{
	struct sensor_ee_pn ee_str_m;
	struct sensor_ee_pn ee_str_d;
	struct sensor_ee_pn ee_incr_d;
	struct sensor_ee_pn ee_thr_d;
	struct sensor_ee_pn ee_incr_m;
	struct sensor_ee_pn ee_incr_b;
	struct sensor_ee_pn ee_str_b;
	struct sensor_ee_pn ee_cv_clip_x;
	struct sensor_ee_flat ee_flat_thrx;
	struct sensor_ee_txt ee_txt_thrx;
	struct sensor_ee_corner ee_corner_xx;
	struct sensor_ee_smooth ee_smooth_xx;
	struct sensor_ee_ipd ipd_xx;
	struct sensor_ee_ratio ratio;
	struct sensor_ee_t_cfg ee_tx_cfg;
	struct sensor_ee_r_cfg ee_rx_cfg;
	uint8_t mode;
	uint8_t sigma;
	uint8_t ee_clip_after_smooth_en;
	uint8_t reserved0;
	uint32_t bypass;
};

struct sensor_ee_param {
	isp_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_ee_level ee_level;
	isp_u32 strength_level;
	isp_u32 cur_idx;
	isp_s8 tab[SENSOR_LEVEL_NUM];
	isp_s8 scenemode[MAX_SCENEMODE_NUM];
	isp_u32 reserved[2];
};




/************************************************************************************/
//Emboss
struct sensor_emboss_param_v1 {
	uint8_t y_step;
	uint8_t uv_step;
	uint8_t reserved[2];
};

/*smart param begin*/
////////////////////////////////////////////////////////////////////////////
#define ISP_SMART_MAX_BV_SECTION 8
#define ISP_SMART_MAX_BLOCK_NUM 28
#define ISP_SMART_MAX_VALUE_NUM 4

enum isp_smart_x_type {
	ISP_SMART_X_TYPE_BV = 0,
	ISP_SMART_X_TYPE_BV_GAIN = 1,
	ISP_SMART_X_TYPE_CT = 2,
	ISP_SMART_X_TYPE_BV_CT = 3,
};

enum isp_smart_y_type {
	ISP_SMART_Y_TYPE_VALUE = 0,
	ISP_SMART_Y_TYPE_WEIGHT_VALUE = 1,
};

enum isp_smart_id {
	ISP_SMART_LNC = 0,
	ISP_SMART_COLOR_CAST = 1,
	ISP_SMART_CMC = 2,
	ISP_SMART_SATURATION_DEPRESS = 3,
	ISP_SMART_HSV = 4,
	ISP_SMART_COLOR_TRANSFORM = 5,
	ISP_SMART_EDGE = 6,
	ISP_SMART_PREF = 7,
	ISP_SMART_UVCDN = 8,
	ISP_SMART_GAMMA = 9,
	ISP_SMART_GAIN_OFFSET = 10,
	ISP_SMART_PRE_WAVELET = 11,
	ISP_SMART_BPC = 12,
	ISP_SMART_NLM = 13,
	ISP_SMART_RGB_PRECDN = 14,
	ISP_SMART_YUV_PRECDN = 15,
	ISP_SMART_UV_POSTCDN = 16,
	ISP_SMART_IIRCNR_IIR = 17,
	ISP_SMART_BDN = 18,
	ISP_SMART_UVDIV = 19,
	ISP_SMART_AF = 20,
	ISP_SMART_CFAE = 21,
	ISP_SMART_GRGB = 22,
	ISP_SMART_IIR_YRANDOM = 23,
	ISP_SMART_Y_AFM = 24,
	ISP_SMART_MAX
};

struct isp_smart_component_cfg {
	uint32_t id;
	uint32_t type;
	uint32_t offset;
	uint32_t size;

	uint32_t x_type;	// isp_smart_x_type
	uint32_t y_type;	// isp_smart_y_type
	int32_t default_val;
	int32_t use_flash_val;
	int32_t flash_val;	//use this value when flash is open

	uint32_t section_num;
	struct isp_range bv_range[ISP_SMART_MAX_BV_SECTION];
	struct isp_piecewise_func func[ISP_SMART_MAX_BV_SECTION];
};

struct isp_smart_block_cfg {
	uint32_t enable;
	uint32_t smart_id;	//id to identify the smart block
	uint32_t block_id;	//id to identify the isp block (destination block)
	uint32_t component_num;
	struct isp_smart_component_cfg component[ISP_SMART_MAX_VALUE_NUM];
};

/*tuning param for smart block*/
struct isp_smart_param {
	uint32_t block_num;
	struct isp_smart_block_cfg block[ISP_SMART_MAX_BLOCK_NUM];
};
/*smart param end*/


/************************************************************************************/
//CSS
struct sensor_css_lh {
	uint8_t css_lh_chrom_th;
	uint8_t css_lh_ratio[8];
	uint8_t reserved[3];
};

struct sensor_css_lum {
	uint8_t css_lum_low_shift;
	uint8_t css_lum_hig_shift;
	uint8_t css_lum_low_th;
	uint8_t css_lum_ll_th;
	uint8_t css_lum_hig_th;
	uint8_t css_lum_hh_th;
	uint16_t reserved;
};

struct sensor_css_exclude {
	uint8_t exclude_u_th_l;
	uint8_t exclude_u_th_h;
	uint8_t exclude_v_th_l;
	uint8_t exclude_v_th_h;
};

struct sensor_css_chrom {
	uint8_t css_chrom_cutoff_th;
	uint8_t css_chrom_lower_th[7];
	uint8_t css_chrom_high_th[7];
	uint8_t reserved;
};

struct sensor_css_v1_param {
	struct sensor_css_lh lh;
	struct sensor_css_lum lum_thre;
	struct sensor_css_exclude exclude[2];
	struct sensor_css_chrom chrom_thre;
	uint8_t ratio[8];
	uint8_t reserved[3];
};



/************************************************************************************/
//Saturation
struct sensor_saturation_v1_param {
	uint8_t csa_factor_u[16];
	uint8_t csa_factor_v[16];
	uint32_t index_u;
	uint32_t index_v;

	uint8_t scenemode[2][MAX_SCENEMODE_NUM];
};



/************************************************************************************/
//Hue
struct sensor_hue_param_v1 {
	uint16_t hue_theta[16];
	uint32_t cur_index;
};



/************************************************************************************/
//post-color denoise
struct sensor_postcdn_thr {
	uint16_t thr0;//algorithm reserved
	uint16_t thr1;//algorithm reserved
};

struct sensor_postcdn_r_seg {
	uint8_t r_seg0[7];
	uint8_t r_seg1[7];
	uint8_t reserved0[2];
};

struct sensor_postcdn_distw {
	uint8_t distw[15][5];
	uint8_t reserved0;
};

/*
uint16_t adpt_med_thr;							//have 3 sets for low light,normal light and high light
uint8_t r_segu[2][7];
uint8_t r_segv[2][7];
uint8_t r_distw[15][5];
*/
struct sensor_uv_postcdn_level{
	struct sensor_postcdn_r_seg r_segu;
	struct sensor_postcdn_r_seg r_segv;
	struct sensor_postcdn_distw r_distw;
	struct sensor_postcdn_thr thruv;
	struct sensor_postcdn_thr thru;
	struct sensor_postcdn_thr thrv;
	uint16_t adpt_med_thr;
	uint8_t median_mode;
	uint8_t uv_joint;
	uint8_t median_res_wb_en;
	uint8_t postcdn_mode;
	uint8_t downsample_bypass;
	uint32_t bypass;
};


struct sensor_uv_postcdn_param {
	isp_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_uv_postcdn_level uv_postcdn_level;
	isp_u16 strength_level;
	isp_u32 reserved[2];
};



/************************************************************************************/
//Gamma Correction in YUV domain
struct sensor_y_gamma_param {
	isp_u32 cur_idx;
	struct sensor_gamma_curve curve_tab[SENSOR_GAMMA_NUM];
	struct sensor_gamma_curve specialeffect[MAX_SPECIALEFFECT_NUM];
};



/************************************************************************************/
//IIR Noise Remove
/*
sensor_iircnr_iir:
the following parameters in sensor_iircnr_iir are Algo reserved:
iircnr_iir_mode
iircnr_uv_pg_th
iircnr_uv_s_th
iircnr_uv_low_thr2
iircnr_slope
iircnr_middle_factor
iircnr_y_th :have at least 3 sets for low light, normal light and high light each
iircnr_ymd_u :have at least 3 sets for low light, normal light and high light each
iircnr_ymd_v :have at least 3 sets for low light, normal light and high light each
iircnr_uv_high_thr2 : have at least 3 sets for low light, normal light and high light each
iircnr_uv_th :have at least 3 sets for low light, normal light and high light each
iircnr_uv_dist :have at least 3 sets for low light, normal light and high light each

the following  parameters in sensor_iircnr_iir will have at least 3 sets for low light, normal light and high light each:
iircnr_uv_low_thr1
iircnr_alpha_low_u
iircnr_alpha_low_v

*/

struct sensor_iircnr_level{
	uint8_t   iircnr_iir_mode;
	uint8_t   iircnr_y_th;
	uint8_t   reserved0[2];
	uint16_t iircnr_uv_th;
	uint16_t iircnr_uv_dist;
	uint16_t iircnr_uv_pg_th;
	uint16_t iircnr_uv_low_thr1;
	uint16_t iircnr_uv_low_thr2;
	uint16_t reserved1;
	uint32_t iircnr_ymd_u;
	uint32_t iircnr_ymd_v;
	uint8_t   iircnr_uv_s_th;
	uint8_t   iircnr_slope;
	uint8_t   reserved2[2];
	uint16_t iircnr_alpha_low_u;
	uint16_t iircnr_alpha_low_v;
	uint32_t iircnr_middle_factor;
	uint32_t iircnr_uv_high_thr2;
	uint32_t bypass;
};

struct sensor_iircnr_param {
	isp_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_iircnr_level iircnr_level;
	isp_u32 strength_level;
	isp_u32 reserved[2];
};

/*
sensor_iircnr_yrandom:All parameters are Algo reserved
*/
struct sensor_iircnr_yrandom_level {
	uint8_t   yrandom_shift;
	uint8_t   reserved0[3];
	uint32_t yrandom_seed;
	uint16_t yrandom_offset;
	uint16_t reserved1;
	uint8_t   yrandom_takebit[8];
	uint32_t bypass;
};

struct sensor_iircnr_yrandom_param {
	isp_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_iircnr_yrandom_level iircnr_yrandom_level;
	isp_u32 strength_level;
	isp_u32 reserved[2];
};

/************************************************************************************/
//bilateral denoise

struct sensor_bdn_level {
	isp_u8 diswei_tab[10][19];
	isp_u8 ranwei_tab[10][31];
	isp_u32 radial_bypass;
	isp_u32 addback;
	struct isp_pos center;
	struct isp_pos delta_sqr;
	struct isp_curve_coeff coeff;
	isp_u32 dis_sqr_offset;
	uint32_t bypass;
};

struct sensor_bdn_param {
	isp_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_bdn_level bdn_level;
	isp_u32 strength_level;
	isp_u32 reserved[2];
};

/************************************************************************************/
/*
uvdiv:
1. All uvdiv parameters are reserved.
2. There will be at least 2 sets of uvdiv parameters for different illuminance.
*/
struct sensor_cce_uvdiv_level{
	uint8_t lum_th_h_len;
	uint8_t lum_th_h;
	uint8_t lum_th_l_len;
	uint8_t lum_th_l;
	uint8_t chroma_min_h;
	uint8_t chroma_min_l;
	uint8_t chroma_max_h;
	uint8_t chroma_max_l;
	uint8_t u_th1_h;
	uint8_t u_th1_l;
	uint8_t u_th0_h;
	uint8_t u_th0_l;
	uint8_t v_th1_h;
	uint8_t v_th1_l;
	uint8_t v_th0_h;
	uint8_t v_th0_l;
	uint8_t ratio[9];
	uint8_t base;
	uint8_t reserved0[2];
	uint32_t bypass;
};

struct sensor_cce_uvdiv_param_v1 {
	isp_uint *param_ptr; /* if not null, read from noise/xxxx_param.h */
	struct sensor_cce_uvdiv_level cce_uvdiv_level;
	isp_u32 strength_level;
	isp_u32 reserved1[2];
};

//////////////////////////////////////////////////
/********** parameter block definition **********/
//////////////////////////////////////////////////


enum ISP_BLK_ID {
	ISP_BLK_ID_BASE = 0,


	ISP_BLK_SHARKL_BASE					= 0x1000,
	ISP_BLK_FLASH_CALI = 0x1018,
	ISP_BLK_ENVI_DETECT = 0x1028,

	//TSHARK2
	ISP_BLK_SHARK2_BASE =				0x2000,
	ISP_BLK_PRE_GBL_GAIN_V1 = 0x2001,
	ISP_BLK_BLC_V1 = 0x2002,
	ISP_BLK_RGB_GAIN_V1 = 0x2003,
	ISP_BLK_PRE_WAVELET_V1 = 0x2004,
	ISP_BLK_NLC_V1 = 0x2005,
	ISP_BLK_2D_LSC = 0x2006,
	ISP_BLK_1D_LSC = 0x2007,
	ISP_BLK_BINNING4AWB_V1 = 0x2008,
	ISP_BLK_AWB_V1 = 0x2009,
	ISP_BLK_AE_V1 = 0x200A,
	ISP_BLK_BPC_V1 = 0x200B,
	ISP_BLK_BL_NR_V1 = 0x200C,
	ISP_BLK_GRGB_V1 = 0x200D,
	ISP_BLK_RGB_GAIN2 = 0x200E,
	ISP_BLK_NLM = 0x200F, // + ISP_BLK_VST + ISP_BLK_IVST
	ISP_BLK_CFA_V1 = 0x2010,
	ISP_BLK_CMC10 = 0x2011,
	ISP_BLK_RGB_GAMC = 0x2012,
	ISP_BLK_CMC8 = 0x2013,
	ISP_BLK_CTM = 0x2014,
	ISP_BLK_CCE_V1 = 0x2015,
	ISP_BLK_HSV = 0x2016,
	ISP_BLK_RADIAL_CSC = 0x2017,
	ISP_BLK_RGB_PRECDN = 0x2018,
	ISP_BLK_POSTERIZE = 0x2019,
	ISP_BLK_AF_V1 = 0x201A,
	ISP_BLK_YIQ_AEM = 0x201B,
	ISP_BLK_YIQ_AFL = 0x201C,
	ISP_BLK_YIQ_AFM = 0x201D,
	ISP_BLK_YUV_PRECDN = 0x201E,
	ISP_BLK_PREF_V1 = 0x201F,
	ISP_BLK_BRIGHT = 0x2020,
	ISP_BLK_CONTRAST = 0x2021,
	ISP_BLK_HIST_V1 = 0x2022,
	ISP_BLK_HIST2 = 0x2023,
	ISP_BLK_AUTO_CONTRAST_V1 = 0x2024,//auto-contrast
	ISP_BLK_UV_CDN = 0x2025,
	ISP_BLK_EDGE_V1 = 0x2026,
	ISP_BLK_EMBOSS_V1 = 0x2027,
	ISP_BLK_CSS_V1 = 0x2028,
	ISP_BLK_SATURATION_V1 = 0x2029,
	ISP_BLK_HUE_V1 = 0x202A,
	ISP_BLK_UV_POSTCDN = 0x202B,
	ISP_BLK_Y_GAMMC = 0x202D,
	ISP_BLK_YDELAY = 0x202E,
	ISP_BLK_IIRCNR_IIR = 0x202F,
	ISP_BLK_UVDIV_V1 = 0x2030,
	ISP_BLK_IIRCNR_YRANDOM = 0x2031,

	ISP_BLK_SMART = 0x2032,
	ISP_BLK_SFT_AF = 0x2033,

	//pike
//	ISP_BLK_PIKE_BASE =  0x3000,
//	ISP_BLK_WDR = 0x3003,
	ISP_BLK_RGB2Y = 0x3004,
	ISP_BLK_UV_PREFILTER = 0x3007,
	ISP_BLK_YUV_NLM = 0x3008,

	ISP_BLK_EXT,
	ISP_BLK_ID_MAX,
};

enum {
	ISP_MODE_ID_COMMON = 0x00,
	ISP_MODE_ID_PRV_0,
	ISP_MODE_ID_PRV_1,
	ISP_MODE_ID_PRV_2,
	ISP_MODE_ID_PRV_3,
	ISP_MODE_ID_CAP_0,
	ISP_MODE_ID_CAP_1,
	ISP_MODE_ID_CAP_2,
	ISP_MODE_ID_CAP_3,
	ISP_MODE_ID_VIDEO_0,
	ISP_MODE_ID_VIDEO_1,
	ISP_MODE_ID_VIDEO_2,
	ISP_MODE_ID_VIDEO_3,
	ISP_MODE_ID_MAX = 0xff,
};

enum {
	ISP_PARAM_ID_SETTING = 0x00,
	ISP_PARAM_ID_CMD,
	ISP_PARAM_ID_AUTO_AE,
	ISP_PARAM_ID_AUTO_AWB,
	ISP_PARAM_ID_AUTO_AF,
	ISP_PARAM_ID_AUTO_SMART,
};

struct isp_block_header {
	unsigned char block_name[8];

	unsigned int block_id;
	unsigned int version_id;
	unsigned int param_id;

	unsigned int bypass;

	unsigned int size;
	unsigned int offset;

	unsigned int reserved[4];
};

struct isp_block_param {
	struct isp_block_header block_header;

	unsigned int* data_ptr;
	unsigned int  data_size;
};

struct isp_mode_param {
	unsigned int version_id;

	unsigned char mode_name[8];

	unsigned int mode_id;
	unsigned int block_num;
	unsigned int size;
	unsigned int width;
	unsigned int height;

	unsigned int fps;
	unsigned int reserved[3];

	struct isp_block_header block_header[256];
};


struct isp_mode_param_info {
	uint8_t *addr;
	uint32_t len;		//by bytes
};

struct third_lib_info {
	uint32_t product_id;
	uint32_t product_name_high;
	uint32_t product_name_low;
	uint32_t version_id;
	uint32_t reserve[4];
};

struct sensor_libuse_info {
	struct third_lib_info awb_lib_info;
	struct third_lib_info ae_lib_info;
	struct third_lib_info af_lib_info;
	struct third_lib_info lsc_lib_info;
	uint32_t reserve[32];
};

struct sensor_version_info {
	uint32_t version_id;
	uint32_t srtuct_size;
	uint32_t reserve;
};

struct sensor_raw_resolution_info {
	uint16_t start_x;
	uint16_t start_y;
	uint16_t width;
	uint16_t height;
	uint16_t line_time;
	uint16_t frame_line;
};

struct sensor_raw_resolution_info_tab {
	uint32_t image_pattern;
	struct sensor_raw_resolution_info tab[10];
};

struct sensor_raw_ioctrl {
	uint32_t(*set_focus) (uint32_t param);
	uint32_t(*set_exposure) (uint32_t param);
	uint32_t(*set_gain) (uint32_t param);
	uint32_t(*ext_fuc) (void *param);
	isp_int (*write_i2c)(uint16_t slave_addr, uint8_t *cmd, uint16_t cmd_length);
	isp_int (*read_i2c)(uint16_t slave_addr, uint8_t *cmd, uint16_t cmd_length);
	uint32_t(*ex_set_exposure) (uint32_t param);
};

/*************new***************************/
struct sensor_fix_param_mode_info{
	uint32_t version_id;
	uint32_t mode_id;
	uint32_t width;
	uint32_t height;
	uint32_t reserved[8];
};

struct sensor_fix_param_block_info{
	uint32_t version;
	uint32_t block_id;
	uint32_t reserved[8];
};

struct sensor_mode_fix_param{
uint32_t *mode_info;
uint32_t   len;
};

struct sensor_block_fix_param{
uint32_t *block_info;
uint32_t   len;
};

 struct ae_exp_gain_tab{
	uint32_t *index;
	uint32_t index_len;
	uint32_t *exposure;
	uint32_t exposure_len;
	uint32_t *dummy;
	uint32_t dummy_len;
	uint16_t *again;
	uint32_t again_len;
	uint16_t *dgain;
	uint32_t dgain_len;
};

  struct ae_scene_exp_gain_tab{
	uint32_t *scene_info;
	uint32_t scene_info_len;
	uint32_t *index;
	uint32_t index_len;
	uint32_t *exposure;
	uint32_t exposure_len;
	uint32_t *dummy;
	uint32_t dummy_len;
	uint16_t *again;
	uint32_t again_len;
	uint16_t *dgain;
	uint32_t dgain_len;
};

 struct ae_weight_tab{
	uint8_t *weight_table;
	uint32_t len;
 };

 struct ae_auto_iso_tab_v1{
	 uint16_t *auto_iso_tab;
	 uint32_t len;
 };

struct sensor_ae_tab{
	struct sensor_block_fix_param block;
	struct ae_exp_gain_tab ae_tab[AE_FLICKER_NUM][AE_ISO_NUM_NEW];
	struct ae_weight_tab weight_tab[AE_WEIGHT_TABLE_NUM];
	struct ae_scene_exp_gain_tab scene_tab[AE_SCENE_NUM][AE_FLICKER_NUM];
	struct ae_auto_iso_tab_v1  auto_iso_tab[AE_FLICKER_NUM];
};

/*******************************new***************/
struct sensor_2d_lsc_param_v1 {
	struct isp_sample_point_info cur_idx;
	isp_u32 tab_num;
	//struct sensor_lens_map_addr map[SENSOR_LENS_NUM];
	//void *data_area;
};


struct sensor_lens_map_info{
	uint32_t envi;
	uint32_t ct;
	uint32_t width;
	uint32_t height;
	uint32_t grid;
};

struct sensor_lens_map {
	uint32_t *map_info;
	uint32_t  map_info_len;
	uint16_t *lnc_addr;
	uint32_t lnc_len;
};

struct sensor_lsc_map{
	struct sensor_block_fix_param block;
	struct sensor_lens_map map[9];
};

struct sensor_awb_map{
	uint16_t *addr;
	uint32_t len;		//by bytes
};

struct sensor_awb_weight{
	uint8_t *addr;
	uint32_t weight_len;
	uint16_t *size;
	uint32_t size_param_len;
};

struct sensor_awb_map_weight_param{
	struct sensor_block_fix_param block;
	struct sensor_awb_map awb_map;
	struct sensor_awb_weight awb_weight;
};

struct sensor_raw_fix_info{
	struct sensor_mode_fix_param mode;
	struct sensor_ae_tab ae;
	struct sensor_lsc_map lnc;
	struct sensor_awb_map_weight_param awb;
};

struct sensor_raw_note_info{
	uint8_t *note;
	uint32_t node_len;
};


struct sensor_raw_info {
	struct sensor_version_info *version_info;
	struct isp_mode_param_info mode_ptr[MAX_MODE_NUM];
	struct sensor_raw_resolution_info_tab *resolution_info_ptr;
	struct sensor_raw_ioctrl *ioctrl_ptr;
	struct sensor_libuse_info *libuse_info;
	struct sensor_raw_fix_info *fix_ptr[MAX_MODE_NUM];
	struct sensor_raw_note_info note_ptr[MAX_MODE_NUM];
};

struct raw_param_info_tab {
	uint32_t param_id;
	struct sensor_raw_info *info_ptr;
	uint32_t(*identify_otp) (void *param_ptr);
	uint32_t(*cfg_otp) (void *param_ptr);
};

struct denoise_param_update {
	struct sensor_pwd_level *pwd_level_ptr;
	struct sensor_bpc_level *bpc_level_ptr;
	struct sensor_bdn_level *bdn_level_ptr;
	struct sensor_grgb_v1_level *grgb_v1_level_ptr;
	struct sensor_nlm_level *nlm_level_ptr;
	struct sensor_vst_level *vst_level_ptr;
	struct sensor_ivst_level *ivst_level_ptr;
	struct sensor_flat_offset_level *flat_offset_level_ptr;
	struct sensor_cfae_level *cfae_level_ptr;
	struct sensor_rgb_precdn_level *rgb_precdn_level_ptr;
	struct sensor_yuv_precdn_level *yuv_precdn_level_ptr;
	struct sensor_prfy_level *prfy_level_ptr;
	struct sensor_uv_cdn_level *uv_cdn_level_ptr;
	struct sensor_ee_level *ee_level_ptr;
	struct sensor_uv_postcdn_level *uv_postcdn_level_ptr;
	struct sensor_iircnr_level *iircnr_level_ptr;
	struct sensor_iircnr_yrandom_level *iircnr_yrandom_level_ptr;
	struct sensor_cce_uvdiv_level *cce_uvdiv_level_ptr;
	struct sensor_y_afm_level *y_afm_level_ptr;
	uint32_t multi_mode_enable;
};


#endif
