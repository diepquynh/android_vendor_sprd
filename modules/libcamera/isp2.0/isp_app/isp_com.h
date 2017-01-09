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

#ifndef _ISP_COM_H_
#define _ISP_COM_H_
/*----------------------------------------------------------------------------*
 **				 Dependencies					*
 **---------------------------------------------------------------------------*/
#ifndef WIN32
#include <sys/types.h>
#include <utils/Log.h>
#endif
#ifdef WIN32
#include "../kernel_include/sprd_isp.h"
#else
#include "sprd_isp.h"
#endif
#include "isp_type.h"
#include "isp_app.h"
#include "sensor_raw.h"
#include "awb_ctrl.h"
#include "ae_sprd_ctrl.h"
#include <string.h>
#include <stdlib.h>

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
#define ISP_SLICE_WIN_NUM 0x0b
#define ISP_SLICE_WIN_NUM_V1 0x18
#define ISP_ALGIN 0x02

#define ISP_CMC_NUM 0x09
#define ISP_COLOR_TEMPRATURE_NUM 0x09
#define ISP_SET_NUM 0x08

#define ISP_AE_STAT_WIN_X 32
#define ISP_AE_STAT_WIN_Y 32

#define ISP_AEM_BINNING_SIZE 640 * 480 * 2
#define ISP_AEM_BIN_SCAL_RATIO 4  //2:4x 3:8x 4:16x 5:32x
#define ISP_AEM_MAX_BLOCK_SIZE 8224
#define ISP_RAWAEM_BUF_SIZE   (4*3*1024)
#define ISP_BQ_BIN_CNT                      3
#define ISP_BQ_AEM_CNT                      3

#define ISP_END_FLAG 0xffa5
#define ISP_IDLE_FLAG 0x00
#define ISP_RUN_FLAG 0x01

#define ISP_AWB_STAT_FLAG 0x01
#define ISP_AF_STAT_FLAG 0x02

#define ISP_INPUT_SIZE_NUM_MAX 0x09
#define ISP_GAMMA_SAMPLE_NUM 26
#define ISP_CMC_MATRIX_ITM_NUM 9
#define ISP_AWBM_MAX_SIZE 102*76

#define ISP_CCE_COEF_COLOR_CAST 0
#define ISP_CCE_COEF_GAIN_OFFSET 1


#define CLIP(in, bottom, top) {if(in<bottom) in=bottom; if(in>top) in=top;}

typedef int32_t ( *io_fun)(isp_handle isp_handler, void* param_ptr, int32_t(*call_back)());

/**---------------------------------------------------------------------------*
**								 Data Prototype 							 **
**----------------------------------------------------------------------------*/

enum isp_status{
	ISP_CLOSE=0x00,
	ISP_IDLE,
	ISP_RUN,
	ISP_CONTINUE,
	ISP_SIGNAL,
	ISP_EXIT,
	ISP_STATE_MAX
};

enum isp_bayer_pattern{
	ISP_GR=0x00,
	ISP_R,
	ISP_B,
	ISP_GB,
	ISP_BAYER_MAX
};

enum isp_fetch_format{
	ISP_FETCH_YUV422_3FRAME=0x00,
	ISP_FETCH_YUYV,
	ISP_FETCH_UYVY,
	ISP_FETCH_YVYU,
	ISP_FETCH_VYUY,
	ISP_FETCH_YUV422_2FRAME,
	ISP_FETCH_YVU422_2FRAME,
	ISP_FETCH_NORMAL_RAW10,
	ISP_FETCH_CSI2_RAW10,
	ISP_FETCH_FORMAT_MAX
};

enum isp_store_format{
	ISP_STORE_UYVY=0x00,
	ISP_STORE_YUV422_2FRAME,
	ISP_STORE_YVU422_2FRAME,
	ISP_STORE_YUV422_3FRAME,
	ISP_STORE_YUV420_2FRAME,
	ISP_STORE_YVU420_2FRAME,
	ISP_STORE_YUV420_3FRAME,
	ISP_STORE_FORMAT_MAX
};

enum isp_work_mode{
	ISP_SINGLE_MODE=0x00,
	ISP_CONTINUE_MODE,
	ISP_WORK_MAX
};

enum isp_match_mode{
	ISP_CAP_MODE=0x00,
	ISP_EMC_MODE,
	ISP_DCAM_MODE,
	ISP_MATCH_MAX
};

enum isp_endian{
	ISP_ENDIAN_LITTLE=0x00,
	ISP_ENDIAN_BIG,
	ISP_ENDIAN_LITTLE_HLF,
	ISP_ENDIAN_BIG_HLF,
	ISP_ENDIAN_MAX
};

enum isp_bit_reorder{
	ISP_LSB=0x00,
	ISP_HSB,
	ISP_BIT_REORDER_MAX
};


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

enum isp_slice_type_v1{
	ISP_LSC_V1 = 0x00,
	ISP_BINGING_V1,
	ISP_CSC_V1,
	ISP_BDN_V1,
	ISP_PWD_V1,
	ISP_LENS_V1,
	ISP_AEM_V1,
	ISP_Y_AEM_V1,
	ISP_RGB_AFM_V1,
	ISP_Y_AFM_V1,
	ISP_HISTS_V1,
	ISP_DISPATCH_V1,
	ISP_FETCH_V1,
	ISP_STORE_V1,
	ISP_SLICE_TYPE_MAX_V1
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


struct isp_slice_param_v1{
	enum isp_slice_pos_info pos_info;
	uint32_t slice_line;
	uint32_t complete_line;
	uint32_t all_line;
	struct isp_size max_size;
	struct isp_size all_slice_num;
	struct isp_size cur_slice_num;

	struct isp_trim_size size[ISP_SLICE_WIN_NUM_V1];
	uint32_t edge_info;
};


struct isp_com_param{
	enum isp_process_type proc_type;
	uint32_t in_mode;
	uint32_t fetch_endian;
	uint32_t fetch_bit_reorder;
	uint32_t bpc_endian;
	uint32_t store_endian;
	uint32_t out_mode;
	uint32_t store_yuv_format;
	uint32_t fetch_color_format;
	uint32_t bayer_pattern;

	uint32_t nlc_bayer;
	uint32_t awbc_bayer;
	uint32_t wave_bayer;
	uint32_t cfa_bayer;
	uint32_t gain_bayer;
};


struct isp_io_ctrl_fun
{
	enum isp_ctrl_cmd cmd;
	io_fun io_ctrl;
};

struct isp_awbm_param{
	uint32_t bypass;
	struct isp_pos win_start;
	struct isp_size win_size;
};


struct isp_awb_statistic_info{
	uint32_t r_info[1024];
	uint32_t g_info[1024];
	uint32_t b_info[1024];
};


enum isp_ctrl_mode{
	ISP_CTRL_SET=0x00,
	ISP_CTRL_GET,
	ISP_CTRL_MODE_MAX
};

struct isp_reg_ctrl{
	enum isp_ctrl_mode mode;
	uint32_t num;
	uint32_t reg_tab[40];
};

struct isp_af_ctrl{
	enum isp_ctrl_mode mode;
	uint32_t step;
	uint32_t num;
	uint32_t stat_value[9];
};

enum isp_ae_weight{
	ISP_AE_WEIGHT_CENTER=0x00,
	ISP_AE_WEIGHT_AVG,
	ISP_AE_WEIGHT_SPOT,
	ISP_AE_WEIGHT_CUSTOMER,
	ISP_AE_WEIGHT_MAX
};


struct isp_data_param
{
	enum isp_work_mode work_mode;
	enum isp_match_mode input;
	enum isp_format input_format;
	uint32_t format_pattern;
	struct isp_size input_size;
	struct isp_addr input_addr;
	enum isp_format output_format;
	enum isp_match_mode output;
	struct isp_addr output_addr;
	uint16_t slice_height;
};


struct isp_interface_param {
	struct isp_data_param data;

	struct isp_dev_fetch_info fetch;
	struct isp_dev_feeder_info feeder;
	struct isp_dev_store_info store;
	struct isp_com_param com;
	struct isp_size src;
	struct isp_slice_param slice;
};

struct isp_interface_param_v1 {
	struct isp_data_param data;

	struct isp_dev_fetch_info_v1 fetch;
	struct isp_dev_store_info_v1 store;
	struct isp_dev_dispatch_info_v1 dispatch;
	struct isp_dev_arbiter_info_v1 arbiter;
	struct isp_dev_common_info_v1 com;
	struct isp_size src;
	struct isp_slice_param_v1 slice;
};

struct isp_system {
	isp_handle    caller_id;
	proc_callback callback;
	uint32_t      isp_callback_bypass;
	pthread_t     monitor_thread;
	isp_handle    monitor_queue;
	uint32_t      monitor_status;

	pthread_t     posture_thread;
	uint32_t      posture_status;
	pthread_mutex_t posture_mutex;

	isp_handle    thread_ctrl;
	isp_handle    thread_proc;
	isp_handle    thread_afl_proc;
	isp_handle    thread_af_proc;
	isp_handle    thread_awb_proc;
	struct isp_ops ops;
};

struct isp_respond
{
	uint32_t rtn;
};

struct isp_otp_info {
	struct isp_data_info lsc;
	struct isp_data_info awb;

	void* lsc_random;
	void* lsc_golden;
	isp_u32 width;
	isp_u32 height;
};

struct isp_ae_info {
	int32_t bv;
	float gain;
	float exposure;
	float f_value;
	uint32_t stable;
};

typedef struct {
	/* isp_ctrl private */
#ifndef WIN32
	struct isp_system system;
#endif
	uint32_t camera_id;
	void* lsc_golden_data;
	int isp_mode;
	struct isp_size src;
	isp_uint last_err;


	/* isp_driver */
	void* handle_device;
	uint32_t need_soft_ae;

	/* 4A algorithm */
	void* handle_ae;
	void* handle_af;
	void* handle_awb;
	void* handle_smart;
	void* handle_lsc_adv;

	void* handle_soft_ae;

	/*isp anti-flicker */
	void* handle_antiflicker;
	uint32_t anti_flicker_mode;

	/* isp param manager */
	void* handle_pm;
	struct isp_interface_param interface_param;
	struct isp_interface_param_v1 interface_param_v1; //tshark2 use

	/* sensor param */
	uint32_t image_pattern;
	uint32_t param_index;
	struct sensor_raw_resolution_info input_size_trim[ISP_INPUT_SIZE_NUM_MAX];
	struct sensor_raw_ioctrl* ioctrl_ptr;

	struct isp_otp_info otp_info;

	uint32_t alc_awb;
	uint8_t* log_alc_awb;
	uint32_t log_alc_awb_size;
	uint8_t* log_alc_lsc;
	uint32_t log_alc_lsc_size;
	uint8_t* log_alc;
	uint32_t log_alc_size;
	uint8_t* log_alc_ae;
	uint32_t log_alc_ae_size;


	uint32_t isp_smart_eb;
	uint32_t isp_lsc_len;
	isp_uint isp_lsc_physaddr;
	isp_uint isp_lsc_virtaddr;

	uint32_t isp_anti_flicker_len;
	isp_uint isp_anti_flicker_physaddr;
	isp_uint isp_anti_flicker_virtaddr;

	/* rawae/binning4 buffer queue info*/
	isp_uint isp_bq_mem_size;
	isp_uint isp_bq_mem_num;
	uint64_t isp_bq_k_addr_array[16];
	uint64_t isp_bq_u_addr_array[16];
	isp_uint isp_bq_alloc_flag;
	void* cb_of_malloc;
	void* cb_of_free;
	void* buffer_client_data;
	struct isp_time system_time_ae;
	struct awb_lib_fun* awb_lib_fun;
	struct ae_lib_fun* ae_lib_fun;
	struct af_lib_fun* af_lib_fun;

	struct isp_ops ops;

	struct sensor_raw_info *sn_raw_info;

	/*for new param struct*/
	struct isp_data_info isp_init_data[MAX_MODE_NUM];
	struct isp_data_info isp_update_data[MAX_MODE_NUM];/*for isp_tool*/
	uint32_t mode_flag;

	uint32_t gamma_sof_cnt;
	uint32_t gamma_sof_cnt_eb;
	uint32_t update_gamma_eb;
} isp_ctrl_context;


/**---------------------------------------------------------------------------*
**				 Micro Define					*
**----------------------------------------------------------------------------*/

#define ISP_NLC_POINTER_NUM 29
#define ISP_NLC_POINTER_L_NUM 27
#define ISP_BLK_NUM_MAX 256
#define ISP_BLK_VERSION_NUM_MAX 8


/**---------------------------------------------------------------------------*
**								 Data Prototype 							 **
**----------------------------------------------------------------------------*/
/******************************************************************************/
//for blc
struct isp_blc_offset{
	uint16_t r;
	uint16_t gr;
	uint16_t gb;
	uint16_t b;
};

struct isp_blc_param{
	uint32_t cur_index;
	struct isp_dev_blc_info cur;
	struct isp_blc_offset offset[ISP_SET_NUM];
};

/******************************************************************************/
//for nlc
struct isp_nlc_param{
	struct isp_dev_nlc_info cur;
};

/******************************************************************************/
//for lsc
struct isp_lnc_mem_addr_cfg {
	intptr_t addr;
	isp_u32 len;
};

struct isp_lsc_info {
	struct isp_sample_point_info cur_idx;
	void *data_ptr;
	void *param_ptr;
	isp_u32 len;
	isp_u32 grid;
	isp_u32 gain_w;
	isp_u32 gain_h;
};
struct isp_lnc_map{
	isp_u32 ct;
	isp_u32 grid_mode;
	isp_u32 grid_pitch;
	isp_u32 gain_w;
	isp_u32 gain_h;
	isp_u32 grid;
	void* param_addr;
	isp_u32 len;
};


struct isp_lnc_param{
	isp_u32 update_flag;
	struct isp_dev_lsc_info cur;
	struct isp_sample_point_info cur_index_info;	/*for two lsc parameters to interplate*/
	struct isp_lnc_map map;//current lsc map
	struct isp_lnc_map map_tab[ISP_COLOR_TEMPRATURE_NUM];
	struct isp_size resolution;
	isp_u32 tab_num;
	isp_u32 lnc_param_max_size;
};

/******************************************************************************/
//for wb
struct isp_awbc_cfg {
	isp_u32 r_gain;
	isp_u32 g_gain;
	isp_u32 b_gain;
	isp_u32 r_offset;
	isp_u32 g_offset;
	isp_u32 b_offset;
};

struct isp_awbm_cfg {
	struct isp_pos win_start;
	struct isp_size blk_size;
	struct isp_size blk_num;
};

struct isp_awbm_info {
	uint32_t bypass;
	struct isp_pos win_start;
	struct isp_size blk_size;
	struct isp_size blk_num;
	struct isp_awb_statistic_info stat;
};

struct isp_awb_param {
	isp_u32 ct_value;
	struct isp_dev_awb_info cur;
	struct isp_awb_statistic_info stat;
};

/******************************************************************************/
//for ae
struct isp_ae_statistic_info{
	uint32_t y[1024];
};

struct isp_ae_param {
	struct isp_ae_statistic_info stat_info;
};

struct isp_soft_ae_cfg {
	uint32_t bypass;
	uint32_t endian;
	struct isp_size img_size;
	struct isp_size trim_size;
	struct isp_size win_num;
	struct isp_size blk_size;
	struct isp_pos win_start;
	uint32_t h_ratio;
	uint32_t v_ratio;
	uint32_t bayer_mode;
	uint32_t skip_num;
	uint32_t bypass_status;
	uint32_t addr_num;
	void     *addr;
};
/******************************************************************************/
//for anti flicker
struct isp_anti_flicker_cfg {
	uint32_t bypass;
	uint32_t mode;
	uint32_t skip_frame_num;
	uint32_t line_step;
	uint32_t frame_num;
	uint32_t vheight;
	uint32_t start_col;
	uint32_t end_col;
	void     *addr;
};

/******************************************************************************/
//for af
struct isp_af_statistic_info{
	uint64_t info[32];
	uint32_t info_tshark3[105];
};

struct isp_afm_param {
	struct isp_af_statistic_info cur;
};

struct isp_af_param {
	struct isp_afm_param afm;
};

/******************************************************************************/

/******************************************************************************/
//for bpc
struct isp_bpc_thrd_cfg {
	isp_u16 flat_thr;
	isp_u16 std_thr;
	isp_u16 texture_thr;
	isp_u16 reserved;
};

struct isp_bpc_param{
	struct isp_dev_bpc_info cur;
};

/******************************************************************************/
//for denoise
struct isp_bl_nr_cfg {
	isp_u32 level;
};

struct isp_bl_nr_param{
	struct isp_dev_wavelet_denoise_info cur;
};

/******************************************************************************/
//for grgb
struct isp_grgb_thrd_cfg {
	isp_u32 edge_thrd;
	isp_u32 diff_thrd;
};

struct isp_grgb_param {
	struct isp_dev_grgb_info cur;
};

/******************************************************************************/
//for cfa
struct isp_cfa_thrd_cfg {
	isp_u32 edge_thrd;
	isp_u32 diff_thrd;
};

struct isp_cfa_param {
	struct isp_dev_cfa_info cur;
};

/******************************************************************************/
//for cmc
struct isp_cmc_matrix_cfg {
	isp_u32 matrix[9];
};

struct isp_cmc_param {
	struct isp_dev_cmc_info cur;
	struct isp_sample_point_info cur_index_info;
	uint16_t cmc_tab[ISP_CMC_NUM][9];
	uint16_t result_cmc[9];
	uint32_t reduce_percent;//reduce saturation.
	uint32_t reduce_sat_flag;
};

/******************************************************************************/
//for gamma
struct isp_gamma_curve_info {
	isp_u32 axis[2][ISP_GAMMA_SAMPLE_NUM];
};

struct isp_gamma_curve_cfg {
	isp_u32 axis[2][26];
};

struct isp_gamma_info {
	uint16_t axis[2][26];
	uint8_t index[28];
};

struct isp_gamma_param {
	struct isp_sample_point_info cur_index_info;
	struct isp_gamma_info cur;
	struct isp_gamma_curve_info cur_curve;
	struct isp_gamma_curve_info tab[7];
};

struct isp_gamma_cfg_v1{
	uint32_t gamma1_r_node[16];
	uint32_t gamma1_g_node[16];
	uint32_t gamma1_b_node[16];
	uint32_t gamma2_r_node[6];
	uint32_t gamma2_g_node[6];
	uint32_t gamma2_b_node[6];
};
struct isp_gamma_cfg_v1_corr1{
	uint32_t gamma1_r_node[16];
	uint32_t gamma1_g_node[16];
	uint32_t gamma1_b_node[16];
};
struct isp_gamma_cfg_v1_corr2{
	uint32_t gamma2_r_node[6];
	uint32_t gamma2_g_node[6];
	uint32_t gamma2_b_node[6];
};
struct isp_gamma_param_v1 {
	struct isp_dev_gamma_info cur;
};

/******************************************************************************/
//for cce
struct isp_cce_matrix_cfg {
	isp_u16 matrix[9];
	isp_u16 y_shift;
	isp_u16 u_shift;
	isp_u16 v_shift;
	isp_u32 special_effect;
};

struct isp_cce_param {
	/*R/G/B coef to change cce*/
	isp_s32 cur_level[2];
	/*0: color cast, 1: gain offset*/
	isp_u16 cce_coef[2][3];
	isp_u16 bakup_cce_coef[3];
	isp_u32 tab_idx;
	struct isp_dev_cce_matrix_info cur;
	struct isp_dev_cce_matrix_info cce_tab[16];
	struct isp_dev_cce_matrix_info specialeffect_tab[MAX_SPECIALEFFECT_NUM];
};

/******************************************************************************/
//for special effect
struct isp_spe_cfg {
	isp_u32 mode;
};

/******************************************************************************/
//for uv-div
struct isp_uvdiv_cfg {
	isp_u8 thrd[7];
	isp_u8 t[2];
	isp_u8 m[3];
};

struct isp_uvdiv_param {
	struct isp_dev_cce_uv_info cur;
};

/******************************************************************************/
//for pref
struct isp_pref_cfg {
	isp_u32 write_back;
	isp_u32 y_thrd;
	isp_u32 u_thrd;
	isp_u32 v_thrd;
};

struct isp_pref_param {
	struct isp_dev_prefilter_info cur;
	struct isp_dev_prefilter_info bak_info;
};

/******************************************************************************/
//for bright
struct isp_bright_cfg {
	uint32_t factor;
};

struct isp_bright_param {
	uint32_t cur_index;
	struct isp_dev_brightness_info cur;
	uint8_t bright_tab[16];
	uint8_t scene_mode_tab[MAX_SCENEMODE_NUM];
};

/******************************************************************************/
//for contrast
struct isp_contrast_cfg {
	uint32_t factor;
};

struct isp_contrast_param {
	uint32_t cur_index;
	struct isp_dev_contrast_info cur;
	uint8_t tab[16];
	uint8_t scene_mode_tab[MAX_SCENEMODE_NUM];
};

/******************************************************************************/
//for hist
struct isp_hist_ratio_cfg {
	isp_u16 low_ratio;
	isp_u16 high_ratio;
};
struct isp_hist_range_cfg {
	isp_u8 in_min;
	isp_u8 in_max;
	isp_u8 out_min;
	isp_u8 out_max;
};

struct isp_hist_cfg {
	isp_u16 low_ratio;
	isp_u16 high_ratio;
	isp_u8 mode;
	isp_u8 in_min;
	isp_u8 in_max;
	isp_u8 out_min;
	isp_u8 out_max;
	isp_u8 reserved;
};

struct isp_hist_statistic_info{
	uint32_t statistic_data[256];
};

struct isp_hist_param {
	struct isp_dev_hist_info cur;
	struct isp_hist_statistic_info stat_info;
};

/******************************************************************************/
//for auto contrast
struct isp_auto_contrast_range_cfg {
	isp_u8 in_min;
	isp_u8 in_max;
	isp_u8 out_min;
	isp_u8 out_max;
};
struct isp_auto_contrast_cfg {
	isp_u8 mode;
	isp_u8 in_min;
	isp_u8 in_max;
	isp_u8 out_min;
	isp_u8 out_max;
	isp_u8 reserved;
};

struct isp_auto_contrast_param {
	struct isp_dev_aca_info cur;
};

/******************************************************************************/
//for saturation
struct isp_saturation_cfg {
	uint32_t factor;
};

struct isp_saturation_param {
	uint32_t cur_index;
	struct isp_dev_csa_info cur;
	uint8_t tab[16];
	uint8_t scene_mode_tab[MAX_SCENEMODE_NUM];
};

/******************************************************************************/
//for ygamma
struct isp_ygamma_cfg {
	isp_u32 gamma[2][24];
};

struct isp_ygamma_param {
	struct isp_gamma_info cur;
	struct isp_dev_yiq_ygamma_info cur_yiq_ygamma;
};

/******************************************************************************/
//for flicker
struct isp_flicker_cfg {
	isp_u32 v_height;
	isp_u32 line_conter;
	isp_u32 line_step;
	isp_u32 line_start;
};

struct isp_flicker_param{
	struct isp_dev_yiq_flicker_info cur;
};

/******************************************************************************/
//for yiq_ae
struct isp_yiq_ae_cfg {
	isp_u32 param;
};

struct isp_yiq_ae_param {
	struct isp_dev_yiq_ae_info cur;
};

/******************************************************************************/
//for hue
struct isp_hue_cfg {
	uint32_t factor;
};

struct isp_hue_param{
	uint32_t cur_index;
	struct isp_dev_hue_info cur;
	struct isp_dev_hue_info bak_info;
	uint8_t tab[16];
};

/******************************************************************************/
//for edge/sharpness
struct isp_edge_cfg {
	uint32_t factor;
};

struct isp_edge_param{
	uint32_t cur_index;
	struct isp_dev_edge_info cur;
	struct isp_dev_edge_info tab[16];
};

/******************************************************************************/
//for emboss
struct isp_emboss_cfg {
	uint32_t factor;
};

struct isp_emboss_param{
	struct isp_dev_emboss_info cur;
};

/******************************************************************************/
//for fcs
struct isp_fcs_cfg {
	isp_u32 mode;
};

struct isp_fcs_param{
	struct isp_dev_fcs_info cur;
};

/******************************************************************************/
//for css
struct isp_css_cfg {
	isp_u8 low_thr[7];
	isp_u8 lum_thr;
	isp_u8 low_sum_thr[7];
	isp_u8 chr_thr;
	isp_u8 ratio[8];
};

struct isp_css_param{
	struct isp_dev_css_info cur;
};

/******************************************************************************/
//for hdr
struct isp_hdr_cfg {
	isp_u32 level;
	isp_u8 r_index;
	isp_u8 g_index;
	isp_u8 b_index;
	isp_u8* com_ptr;
	isp_u8* p2e_ptr;
	isp_u8* e2p_ptr;
};

struct isp_hdr_index_cfg {
	isp_u8 r_index;
	isp_u8 g_index;
	isp_u8 b_index;
	isp_u8 reserved;
};

struct isp_hdr_param{
	struct isp_dev_hdr_info cur;
};

/******************************************************************************/
//for pre-global gain
struct isp_pre_gbl_gain_cfg {
	isp_u32 gain;
};

struct isp_pre_global_gain_param{
	struct isp_dev_pre_glb_gain_info cur;
};

/******************************************************************************/
//for global gain
struct isp_global_gain_param{
	struct isp_dev_glb_gain_info cur;
};

/******************************************************************************/
//for chn gain
struct isp_chn_gain_param{
	struct isp_dev_rgb_gain_info cur;
};

/******************************************************************************/
//for flash calibration

struct isp_flash_attrib_param {
	uint32_t r_sum;
	uint32_t gr_sum;
	uint32_t gb_sum;
	uint32_t b_sum;
};

struct isp_flash_attrib_cali{
	struct isp_flash_attrib_param global;
	struct isp_flash_attrib_param random;
};

struct isp_flash_info {
	isp_u32 lum_ratio;
	isp_u32 r_ratio;
	isp_u32 g_ratio;
	isp_u32 b_ratio;
	isp_u32 auto_flash_thr;
};

struct isp_flash_param{
	struct isp_flash_info cur;
	struct isp_flash_attrib_cali attrib;
};

/******************************************************************************/
//for nawbm
struct isp_nawbm_param{
	struct isp_dev_nawbm_info cur;
};

/******************************************************************************/
//for pre_wavelet
struct isp_pre_wavelet_param{
	struct isp_dev_pre_wavelet_info cur;
};

/******************************************************************************/
//for binning4awb
struct isp_binning4awb_param{
	struct isp_dev_binning4awb_info cur;
};

/******************************************************************************/
//for newbpc
struct isp_nbpc_param{
	struct isp_dev_nbpc_info cur;
};

/******************************************************************************/
//for envi_detect
struct isp_envi_detect_param {
	uint32_t enable;
	struct isp_range envi_range[SENSOR_ENVI_NUM];
};

enum smart_cfg_idx_type{
	/* for pre-filter */
	SMART_CFG_IDX_PREF_Y = 0x00,
	SMART_CFG_IDX_PREF_U,
	SMART_CFG_IDX_PREF_V,
	/* for bl-nr */
	SMART_CFG_IDX_BL_NR_DIS = 0x00,
	SMART_CFG_IDX_BL_NR_RAN,
	/* for gamma */
	SMART_CFG_IDX_GAMMA = 0x00,
	/* for edge */
	SMART_CFG_IDX_EDGE_DETIAL = 0x00,
	SMART_CFG_IDX_EDGE_SMOOTH,
	SMART_CFG_IDX_EDGE_STRENGTH,
	/*for cmc */
	SMART_CFG_IDX_CMC_4_CLR = 0x00,
	SMART_CFG_IDX_CMC_4_SAT,
	/* for lsc */
	SMART_CFG_IDX_LSC = 0x00,
	SMART_CFG_IDX_LSC_4_NR,
	/* for cce */
	SMART_CFG_IDX_CCE_4_STA = 0x00,
	SMART_CFG_IDX_CCE_4_HUE,
};

enum smart_envi_id {
	/*unknown environment*/
	SMART_ENVI_COMMON = 0,
	/*low light*/
	SMART_ENVI_LOW_LIGHT,
	/*normal indoor*/
	SMART_ENVI_INDOOR_NORMAL,
	/*normal outdoor*/
	SMART_ENVI_OUTDOOR_NORMAL,
	/*outdoor middle light*/
	SMART_ENVI_OUTDOOR_MIDDLE,
	/*outdoor high light*/
	SMART_ENVI_OUTDOOR_HIGH,
	/*number of simple environment*/
	SMART_ENVI_SIMPLE_NUM,
};

/*this function for tshark2.*/

//pgn
struct isp_pre_global_gain_param_v1{
	struct isp_dev_pre_glb_gain_info cur;
};

//blc
struct isp_blc_param_v1{
	struct isp_dev_blc_info cur;
	isp_u32 cur_idx;
	struct sensor_blc_v1_offset offset[SENSOR_BLC_NUM];
};

//global rgb gain.
struct isp_rgb_gain_param_v1{
	struct isp_dev_rgb_gain_info_v1 cur;
};

//pwd
struct isp_pre_wavelet_param_v1 {
	struct isp_dev_pre_wavelet_info_v1 cur;
	isp_u32 cur_level;
	isp_uint *pwd_ptr;
	isp_u32 multi_mode_enable;
};

//nlc
struct isp_nlc_param_v1{
	struct isp_dev_nlc_info_v1 cur;
};

//lens_new lsc
struct isp_2d_lsc_param{
	struct isp_sample_point_info cur_index_info;
	struct isp_dev_2d_lsc_info cur;
	struct isp_data_info final_lsc_param;						//store the resulted lsc params
	struct isp_lnc_map map_tab[ISP_COLOR_TEMPRATURE_NUM];
	isp_u32 tab_num;
	struct isp_lsc_info lsc_info;
	struct isp_size resolution;
	isp_u32 update_flag;
	isp_u32 is_init;
};

//radial lens(1D)
struct isp_1d_lsc_param {
	struct isp_sample_point_info cur_index_info;
	struct isp_dev_1d_lsc_info cur;
	struct sensor_1d_lsc_map map[SENSOR_LENS_NUM];
};

//binning4awb
struct isp_binning4awb_param_v1{
	struct isp_dev_binning4awb_info_v1 cur;
};

//awbm
struct isp_awbm_param_tshark2{
	struct isp_dev_awbm_info_v1 cur;
};

//awbc
struct isp_awbc_param_tshark2{
	struct isp_dev_awbc_info_v1 cur;
};

struct isp_awb_param_v1 {
	isp_u32 ct_value;
	struct isp_dev_awb_info_v1 cur;
	struct isp_awb_statistic_info stat;
	struct isp_data_info awb_statistics[4];
};

struct isp_awb_param_v2 {
	isp_u32 ct_value;
	struct isp_dev_awb_info_v2 cur;
	struct isp_awb_statistic_info stat;
	struct isp_data_info awb_statistics[4];
};

//aem
struct isp_rgb_ae_param {
	struct isp_dev_raw_aem_info cur;
	struct isp_awb_statistic_info stat;
};

//bpc
struct isp_bpc_param_v1{
	struct isp_dev_bpc_info_v1 cur;
	isp_u32 cur_level;//has one parameter needed to auto-change according to the lum envi
	isp_uint *bpc_ptr;
	isp_u32 multi_mode_enable;
};

//bdn.
struct isp_bl_nr_param_v1{
	struct isp_dev_bdn_info_v1 cur;
	isp_uint *bdn_ptr;
	isp_u32 multi_mode_enable;
};

//grgb
struct isp_grgb_param_v1{
	struct isp_dev_grgb_info_v1 cur;
	isp_uint cur_level;
	isp_uint *grgb_ptr;
	isp_u32 multi_mode_enable;
};

//grgb2
struct isp_rgb_gain2_param{
	struct isp_dev_rgb_gain2_info cur;
};

struct isp_nlm_param_v1 {
	uint32_t nlm_level;
	struct isp_dev_nlm_info_v1 cur;
	struct isp_data_info vst_map;
	struct isp_data_info ivst_map;
	struct isp_data_info nlm_map;
	isp_uint *nlm_ptr;
	isp_uint *vst_ptr;
	isp_uint *ivst_ptr;
	isp_uint *flat_offset_ptr;
	isp_u32 multi_mode_enable;
};

struct isp_nlm_param_v2 {
	uint32_t nlm_level;
	struct isp_dev_nlm_info_v2 cur;
	struct isp_data_info vst_map;
	struct isp_data_info ivst_map;
	struct isp_data_info nlm_map;
	isp_uint *nlm_ptr;
	isp_uint *vst_ptr;
	isp_uint *ivst_ptr;
	isp_uint *flat_offset_ptr;
	isp_u32 multi_mode_enable;
};


struct isp_cfa_param_v1 {
	struct isp_dev_cfa_info_v1 cur;
	isp_u32 cur_level;
	isp_uint *cfae_ptr;
	isp_u32 multi_mode_enable;
};

struct isp_cmc10_param {
	struct isp_dev_cmc10_info cur;
	struct isp_sample_point_info cur_idx_info;
	uint16_t matrix[SENSOR_CMC_NUM][SENSOR_CMC_POINT_NUM];
	uint16_t buffer[SENSOR_CMC_NUM][SENSOR_CMC_POINT_NUM];
	uint16_t result_cmc[SENSOR_CMC_POINT_NUM];
	uint16_t reserved;
	uint32_t reduce_percent;//reduce saturation.
};

struct isp_frgb_gamc_param {
	struct isp_dev_gamma_info_v1 cur;
	struct sensor_gamma_curve final_curve;
	struct isp_sample_point_info cur_idx;
	struct sensor_gamma_curve curve_tab[SENSOR_GAMMA_NUM];
};

struct isp_cmc8_param_v1 {
	struct isp_dev_cmc8_info_v1 cur;
	struct isp_sample_point_info cur_idx_info;
	uint16_t matrix[SENSOR_CMC_NUM][SENSOR_CMC_POINT_NUM];
	uint16_t buffer[SENSOR_CMC_NUM][SENSOR_CMC_POINT_NUM];
	uint16_t result_cmc[SENSOR_CMC_POINT_NUM];
	uint16_t reserved;
};

struct isp_ctm_param {
	struct isp_dev_ct_info cur;
	struct isp_sample_point_info cur_idx;
	struct isp_data_info final_map;
	struct isp_data_info map[SENSOR_CTM_NUM];
};

struct isp_cce_param_v1{
	struct isp_dev_cce_info_v1 cur;
	/*R/G/B coef to change cce*/
	isp_s32 cur_level[2];
	/*0: color cast, 1: gain offset*/
	isp_u16 cce_coef[2][3];
	isp_u16 bakup_cce_coef[3];
	isp_u32 prv_idx;
	isp_u32 cur_idx;
	isp_u32 is_specialeffect;
	struct isp_dev_cce_info_v1 cce_tab[16];
	struct isp_dev_cce_info_v1 specialeffect_tab[MAX_SPECIALEFFECT_NUM];
};

struct isp_cce_uvdiv_param_v1 {
	struct isp_dev_cce_uvd_info cur;
	isp_u32 cur_level;
	isp_uint *uv_div_ptr;
	isp_u32 multi_mode_enable;
};
struct isp_hsv_param{
	struct isp_dev_hsv_info_v1 cur;
	struct isp_sample_point_info cur_idx;
	struct isp_data_info final_map;
	struct isp_data_info map[SENSOR_HSV_NUM];
	struct isp_data_info specialeffect_tab[MAX_SPECIALEFFECT_NUM];
};

struct isp_csc_param{
	struct isp_dev_csc_info cur;
	isp_u32 cur_idx;
	struct sensor_radial_csc_map map[SENSOR_LENS_NUM];
};

struct isp_rgb_pre_cdn_param{
	struct isp_dev_pre_cdn_rgb_info cur;
	isp_u32 cur_level;
	isp_uint *rgb_precdn_ptr;
	isp_u32 multi_mode_enable;
};

struct isp_posterize_param {
	struct isp_dev_posterize_info cur;
	struct isp_dev_posterize_info specialeffect_tab[MAX_SPECIALEFFECT_NUM];
};

struct isp_rgb_afm_param_v1 {
	struct isp_dev_rgb_afm_info_v1 cur;
};

struct isp_yiq_aem_param_v1 {
	struct isp_dev_yiq_aem_info_v1 cur;
	struct isp_ae_statistic_info stat;
};

struct isp_yiq_aem_param_v2 {
	struct isp_dev_yiq_aem_info_v2 cur;
};


struct isp_yiq_afl_param_v1 {
	struct isp_dev_anti_flicker_info_v1 cur;
};

struct isp_yiq_afm_param_v1 {
	struct isp_dev_yiq_afm_info_v1 cur;
	isp_u32 cur_level;
	isp_uint *yiq_afm_ptr;
	isp_u32 multi_mode_enable;
};
struct isp_yiq_afm_param_v2 {
	struct isp_dev_yiq_afm_info_v2 cur;
};

struct isp_yuv_precdn_param {
	struct isp_dev_yuv_precdn_info cur;
	isp_u32 cur_level;
	isp_uint *yuv_precdn_ptr;
	isp_u32 multi_mode_enable;
};

struct isp_prfy_param {
	struct isp_dev_prefilter_info_v1 cur;
	isp_u32 cur_level;
	isp_uint *prfy_ptr;
	isp_u32 multi_mode_enable;
};

struct isp_brta_param {
	struct isp_dev_brightness_info cur;
	isp_u32 cur_idx;
	isp_s16 tab[SENSOR_LEVEL_NUM];
	uint8_t scene_mode_tab[MAX_SCENEMODE_NUM];
};

struct isp_cnta_param {
	struct isp_dev_contrast_info cur;
	isp_u32 cur_idx;
	isp_s8 tab[SENSOR_LEVEL_NUM];
	int8_t scene_mode_tab[MAX_SCENEMODE_NUM];
};

struct isp_hist_param_v1 {
	struct isp_dev_hist_info_v1 cur;
};

struct isp_hist2_param_v1 {
	struct isp_dev_hist2_info_v1 cur;
};

struct isp_auto_contrast_param_v1 {
	struct isp_dev_autocont_info_v1 cur;
};

struct isp_uv_cdn_param_v1 {
	struct isp_dev_yuv_cdn_info cur;
	isp_u32 cur_level;
	isp_uint *uv_cdn_ptr;
	isp_u32 multi_mode_enable;
};

struct isp_edge_param_v1{
	struct isp_dev_edge_info_v1 cur;
	isp_u16 level;
	isp_u16 cur_idx;
	isp_s8 tab[SENSOR_LEVEL_NUM];
	isp_s8 scene_mode_tab[MAX_SCENEMODE_NUM];
	isp_uint *edge_ptr;
	isp_u32 multi_mode_enable;
};

struct isp_css_param_v1{
	struct isp_dev_css_info_v1 cur;
};

struct isp_chrom_saturation_param{
	struct isp_dev_csa_info_v1 cur;
	isp_u32 cur_u_idx;
	isp_u32 cur_v_idx;
	isp_u8 tab[2][SENSOR_LEVEL_NUM];
	isp_u8 scene_mode_tab[2][MAX_SCENEMODE_NUM];
};

struct isp_hue_param_v1{
	struct isp_dev_hue_info_v1 cur;
	isp_u32 cur_idx;
	isp_s16 tab[SENSOR_LEVEL_NUM];
};

struct isp_uv_postcdn_param{
	struct isp_dev_post_cdn_info cur;
	isp_u32 cur_level;
	isp_uint *uv_postcdn_ptr;
	isp_u32 multi_mode_enable;
};

struct isp_emboss_param_v1{
	struct isp_dev_emboss_info_v1 cur;
};

struct isp_yuv_ygamma_param {
	struct isp_dev_ygamma_info cur;
	uint32_t cur_idx;
	struct isp_sample_point_info cur_idx_weight;
	struct sensor_gamma_curve final_curve;
	struct sensor_gamma_curve curve_tab[SENSOR_GAMMA_NUM];
	struct sensor_gamma_curve specialeffect_tab[MAX_SPECIALEFFECT_NUM];
};

struct isp_ydelay_param{
	struct isp_dev_ydelay_info cur;
};

struct isp_iircnr_iir_param {
	struct isp_dev_iircnr_info_v1 cur;
	isp_u32 cur_level;
	isp_uint *iir_cnr_ptr;
	isp_u32 multi_mode_enable;
};

struct isp_iircnr_yrandom_param {
	struct isp_dev_yrandom_info_v1 cur;
	isp_u32 cur_level;
	isp_uint *iir_yrandom_ptr;
	isp_u32 multi_mode_enable;
};

struct isp_context {
	uint32_t is_validate;
	uint32_t mode_id;

/*******************************************/
//interface and other block
	// isp param
//	struct isp_cfg_param cfg;
//	struct isp_fetch_param fetch;
//	struct isp_feeder_param feeder;
//	struct isp_store_param store;
//	struct isp_com_param com;
//	struct isp_slice_param slice;



/////////////////////////////////////////////////
// 3A owner:
	struct isp_awb_param awb;
	struct isp_ae_param ae;
	struct isp_af_param af;
	struct isp_smart_param smart;

//////////////////////////////////////////////////
//isp related tuning block
	struct isp_bright_param bright;
	struct isp_contrast_param contrast;
	struct isp_flash_param flash;
	struct isp_envi_detect_param envi_detect;
	/**********************Tshark2*********************************/
	struct isp_pre_global_gain_param_v1 pre_gbl_gain_v1;
	struct isp_blc_param_v1 blc_v1;
	struct isp_rgb_gain_param_v1 rgb_gain_v1;
	struct isp_pre_wavelet_param_v1 pre_wavelet_v1;
	struct isp_nlc_param_v1 nlc_v1;
	struct isp_2d_lsc_param lsc_2d;
	struct isp_1d_lsc_param lsc_1d;
	struct isp_binning4awb_param_v1 binning4awb_v1;
	struct isp_awb_param_v1 awb_v1;
	struct isp_rgb_ae_param  ae_v1;
	struct isp_bpc_param_v1 bpc_v1;
	struct isp_bl_nr_param_v1 bl_nr_v1;
	struct isp_grgb_param_v1 grgb_v1;
	struct isp_rgb_gain2_param rgb_gain2;
	struct isp_nlm_param_v1 nlm_v1;
	struct isp_cfa_param_v1 cfa_v1;
	struct isp_cmc10_param cmc10;
	struct isp_frgb_gamc_param frgb_gamc;
	struct isp_cmc8_param_v1 cmc8_v1;
	struct isp_ctm_param ctm;

	struct isp_cce_param_v1 cce_v1;
	struct isp_hsv_param hsv;
	struct isp_csc_param csc;
	struct isp_rgb_pre_cdn_param rgb_pre_cdn;
	struct isp_posterize_param posterize;
	struct isp_rgb_afm_param_v1 rgb_afm_v1;
	struct isp_yiq_aem_param_v1 yiq_aem_v1;
	struct isp_yiq_afl_param_v1 yiq_afl_v1;

	struct isp_yiq_afm_param_v1 yiq_afm;
	struct isp_yuv_precdn_param yuv_precdn;
	struct isp_prfy_param prfy;
	struct isp_hist_param_v1 hist_v1;
	struct isp_hist2_param_v1 hist2_v1;
	struct isp_auto_contrast_param_v1 auto_contrast_v1;
	struct isp_uv_cdn_param_v1 uv_cdn_v1;
	struct isp_edge_param_v1 edge_v1;
	struct isp_css_param_v1 css_v1;
	struct isp_chrom_saturation_param saturation_v1;
	struct isp_hue_param_v1 hue_v1;
	struct isp_uv_postcdn_param uv_postcdn;
	struct isp_emboss_param_v1 emboss_v1;
	struct isp_yuv_ygamma_param yuv_ygamma;
	struct isp_ydelay_param ydelay;
	struct isp_iircnr_iir_param iircnr_iir;
	struct isp_iircnr_yrandom_param iircnr_yrandom;
	struct isp_cce_uvdiv_param_v1 uv_div_v1;
	/**********************pike*********************************/
//	struct isp_frgb_wdr_param frgb_wdr;
	struct isp_awb_param_v2 awb_v2;
	struct isp_nlm_param_v2 nlm_v2;
	struct isp_yiq_aem_param_v2 yiq_aem_v2;
	struct isp_yiq_afm_param_v2 yiq_afm_v2;

};


/**----------------------------------------------------------------------------*
**						   Compiler Flag									  **
**----------------------------------------------------------------------------*/
#ifdef	 __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif
// End

