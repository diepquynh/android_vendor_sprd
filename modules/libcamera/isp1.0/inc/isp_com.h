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
#include <sys/types.h>
#include "isp_app.h"
#include "isp_drv.h"
#include "lsc_adv.h"
#include "sensor_raw.h"
#include "isp_awb_ctrl.h"
#include "isp_awb.h"
#include "isp_raw.h"
#include "isp_ae.h"
#include "isp_ae_ctrl.h"
#include "isp_af.h"
#include "isp_alg.h"
#include "isp_smart_lsc.h"
#include "isp_smart_fitting.h"

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
#define ISP_ALGIN 0x02

#define ISP_CMC_NUM 0x09
#define ISP_COLOR_TEMPRATURE_NUM 0x09
#define ISP_SET_NUM 0x08

#define ISP_AE_STAT_WIN_X 32
#define ISP_AE_STAT_WIN_Y 32

#define ISP_END_FLAG 0xffa5
#define ISP_IDLE_FLAG 0x00
#define ISP_RUN_FLAG 0x01

#define ISP_AWB_STAT_FLAG 0x01
#define ISP_AF_STAT_FLAG 0x02

#define ISP_INPUT_SIZE_NUM_MAX 0x09
#define ISP_NEW_GAMMA_NUM 41
#define PNULL ((void *)0)

#define CLIP(in, bottom, top) {if(in<bottom) in=bottom; if(in>top) in=top;}

typedef int32_t ( *io_fun)(uint32_t isp_id, void* param_ptr, int32_t(*call_back)());

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

enum isp_change_cmd{
	ISP_CHANGE_LNC,
	ISP_CHANGE_CMC,
	ISP_CHANGE_CCE,
	ISP_CHANGE_LNC_RELOAD,
	ISP_CHANGE_MAX
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

enum isp_lns_buf{
	ISP_LNS_BUF0=0x00,
	ISP_LNS_BUF1,
	ISP_LNS_BUF_MAX
};

enum isp_color_temprature{
	ISP_COLOR_TEMPRATURE0=0x00,
	ISP_COLOR_TEMPRATURE1,
	ISP_COLOR_TEMPRATURE2,
	ISP_COLOR_TEMPRATURE3,
	ISP_COLOR_TEMPRATURE4,
	ISP_COLOR_TEMPRATURE5,
	ISP_COLOR_TEMPRATURE6,
	ISP_COLOR_TEMPRATURE7,
	ISP_COLOR_TEMPRATURE8,
	ISP_COLOR_TEMPRATURE_MAX
};

struct isp_io_ctrl_fun
{
	enum isp_ctrl_cmd cmd;
	io_fun io_ctrl;
};

struct isp_awb_monitor_info {
	struct isp_size frame_size;
	struct isp_size win_num;
	struct isp_pos win_pos;
	struct isp_size win_size;
};

struct isp_feeder_param{
	uint32_t data_type;
};

struct isp_store_param{
	uint32_t bypass;
	uint32_t sub_stract;
	struct isp_addr addr;
	struct isp_pitch pitch;
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
};

struct isp_blc_offset{
	uint16_t r;
	uint16_t gr;
	uint16_t gb;
	uint16_t b;
};

struct isp_blc_param{
	uint32_t bypass;
	uint32_t mode;
	uint16_t r;
	uint16_t gr;
	uint16_t gb;
	uint16_t b;
};

struct isp_nlc_param{
	uint32_t bypass;
	uint16_t r_node[29];
	uint16_t g_node[29];
	uint16_t b_node[29];
	uint16_t l_node[29];
};

struct isp_awbc_param{
	uint32_t bypass;
	uint32_t r_gain;
	uint32_t g_gain;
	uint32_t b_gain;
	uint32_t r_offset;
	uint32_t g_offset;
	uint32_t b_offset;
};

struct isp_awb_statistic_info{
	uint32_t r_info[1024];
	uint32_t g_info[1024];
	uint32_t b_info[1024];
};

struct isp_ae_statistic_info{
	uint32_t y[1024];
};

struct isp_bpc_param{
	uint32_t bypass;
	uint32_t mode;
	uint16_t flat_thr;
	uint16_t std_thr;
	uint16_t texture_thr;
	uint16_t reserved;
	uint32_t map_addr;
};

struct isp_nbpc_param{
	uint8_t bypass_pvd;
	uint8_t nbpc_mode;
	uint8_t mask_mode;
	uint8_t kmin;
	uint8_t kmax;
	uint8_t cntr_theshold;
	uint8_t hwfifo_clr_en;
	uint8_t ktimes;
	uint8_t map_fifo_clr;
	uint8_t delt34;
	uint8_t flat_factor;
	uint8_t safe_factor;
	uint8_t spike_coeff;
	uint8_t dead_coeff;
	uint8_t map_done_sel;
	uint8_t new_old_sel;
	uint16_t bad_pixel_num;
	uint16_t reserved;
	uint16_t interrupt_b[8];
	uint16_t slope_k[8];
	uint16_t lut_level[8];
	uint32_t map_addr;
	uint32_t bypass;
};
struct isp_denoise_param_tab {
	uint8_t diswei[19];
	uint8_t reserved2;
	uint8_t ranwei[31];
	uint8_t reserved1;
};

struct isp_denoise_param
{
	uint32_t bypass;
	uint16_t write_back;
	uint16_t r_thr;
	uint16_t g_thr;
	uint16_t b_thr;
	uint8_t diswei[19];
	uint8_t ranwei[31];
	uint8_t diswei_level;
	uint8_t ranwei_level;
};

struct isp_pre_wave_denoise_param
{
	uint32_t bypass;
	uint8_t thrs0;
	uint8_t thrs1;
	uint8_t reserved1;
	uint8_t reserved0;
};

struct isp_grgb_param
{
	uint32_t bypass;
	uint32_t edge_thr;
	uint32_t diff_thr;
};

struct isp_cfa_param
{
	uint32_t edge_thr;
	uint32_t diff_thr;
};

struct isp_cmc_param
{
	uint32_t bypass;
	uint16_t matrix[9];
	uint16_t reserved;
	struct isp_awb_adjust cur_cmc;
};

struct isp_special_effect{
	enum isp_special_effect_mode mode;
};

struct isp_cce_matrix{
	uint16_t matrix[9];
	uint16_t y_shift;
	uint16_t u_shift;
	uint16_t v_shift;
};

struct isp_cce_uvdiv
{
	uint32_t bypass;
	uint8_t thrd[7];
	uint8_t t[2];
	uint8_t m[3];
};

struct isp_pref_param
{
	uint32_t bypass;
	uint32_t write_back;
	uint8_t y_thr;
	uint8_t u_thr;
	uint8_t v_thr;
	uint8_t reserved;
};

struct isp_bright_param{
	uint32_t bypass;
	uint8_t factor;
};

struct isp_contrast_param{
	uint32_t bypass;
	uint8_t factor;
};

struct isp_hist_param{
	uint32_t bypass;
	uint16_t low_ratio;
	uint16_t high_ratio;
	uint8_t mode;
	uint8_t in_min;
	uint8_t in_max;
	uint8_t out_min;
	uint8_t out_max;
};

struct isp_hist_statistic_info{
	uint32_t hist_info[256];
};

struct isp_auto_contrast_param{
	uint32_t bypass;
	uint8_t mode;
	uint8_t in_min;
	uint8_t in_max;
	uint8_t out_min;
	uint8_t out_max;
};

struct isp_saturation_param{
	uint32_t bypass;
	uint8_t factor;
	uint8_t offset;
};

struct isp_ygamma_param{
	uint32_t bypass;
	uint32_t gamma[2][24];
	uint32_t index[24];
};

struct isp_flicker_param{
	uint32_t bypass;
	uint32_t flicker_v_height;
	uint32_t flicker_line_conter;
	uint32_t flicker_line_step;
	uint32_t flicker_line_start;

};

struct isp_hue_param{
	uint32_t bypass;
	uint8_t factor;
	int16_t offset;
};

struct isp_af_statistic_info{
	uint32_t info[9];
};

struct isp_edge_param{
	uint32_t bypass;
	uint8_t detail_thr;
	uint8_t smooth_thr;
	uint8_t strength;
	uint8_t reserved;
};

struct isp_emboss_param{
	uint32_t bypass;
	uint32_t step;
};

struct isp_fcs_param{
	uint32_t bypass;
};

struct isp_css_param{
	uint32_t bypass;
	uint32_t bypass_bakup;
	uint8_t low_thr[7];
	uint8_t lum_thr;
	uint8_t low_sum_thr[7];
	uint8_t chr_thr;
	uint8_t ratio[8];
};

struct isp_hdr_param{
	uint32_t bypass;
	uint32_t level;
};

struct isp_hdr_index{
	uint8_t r_index;
	uint8_t g_index;
	uint8_t b_index;
	uint8_t* com_ptr;
	uint8_t* p2e_ptr;
	uint8_t* e2p_ptr;
};

struct isp_pre_global_gain_param{
	uint32_t bypass;
	uint32_t gain;
};

struct isp_global_gain_param{
	uint32_t bypass;
	uint32_t gain;
};

struct isp_chn_gain_param{
	uint32_t bypass;
	uint8_t r_gain;
	uint8_t g_gain;
	uint8_t b_gain;
	uint8_t reserved0;
	uint16_t r_offset;
	uint16_t g_offset;
	uint16_t b_offset;
	uint16_t reserved1;
};

struct isp_flash_cali_param{
	uint16_t lum_ratio;
	uint16_t r_ratio;
	uint16_t g_ratio;
	uint16_t b_ratio;
};

struct isp_tune_param{
	uint64_t system_time;
};

struct isp_tune_block{
	uint8_t eb;
	uint8_t ae;
	uint8_t awb;
	uint8_t special_effect;
	uint8_t bright;
	uint8_t contrast;
	uint8_t hist;
	uint8_t auto_contrast;
	uint8_t saturation;
	uint8_t hue;
	uint8_t af;
	uint8_t af_stat_continue;
	uint8_t css;
	uint8_t hdr;
	uint8_t global_gain;
	uint8_t chn_gain;
	uint8_t wb_trim;
	uint8_t alg;
	uint8_t denoise;
	uint8_t pref_y;
	uint8_t pref_uv;
	uint8_t edge;
	uint8_t cmc;
	uint8_t lnc;
	uint8_t lnc_load;
	uint8_t gamma;
	uint8_t pre_wave;
	uint8_t bpc;
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

struct isp_cfg_param
{
	enum isp_video_mode video_mode;
	struct isp_data_param data;
	proc_callback callback;
	proc_callback self_callback;
	void* sensor_info_ptr;
};

struct isp_adv_lsc_param
{
	struct sensor_adv_lsc_param adv_lsc;
};
struct isp_context{
	// isp param
	struct isp_cfg_param cfg;
	struct isp_fetch_param featch;
	struct isp_feeder_param feeder;
	struct isp_store_param store;
	struct isp_com_param com;
	struct isp_blc_param blc;
	struct isp_nlc_param nlc;
	struct isp_lnc_param lnc;
	struct isp_awbm_param awbm;
	struct isp_awbc_param awbc;
	struct isp_ae_param ae;
	struct isp_ae_statistic_info ae_stat;
	struct isp_awb_param awb;
	struct isp_awb_statistic_info awb_stat;
	struct isp_bpc_param bpc;
	struct isp_denoise_param denoise;
	struct isp_denoise_param denoise_bak;
	struct isp_grgb_param grgb;
	struct isp_cfa_param cfa;
	struct isp_cmc_param cmc;
	struct isp_gamma_param gamma;
	struct isp_ygamma_param ygamma;
	struct isp_flicker_param flicker;
	struct isp_cce_matrix cce_matrix;
	struct isp_cce_uvdiv uv_div;
	struct isp_pref_param pref;
	struct isp_pref_param pref_bak;
	struct isp_bright_param bright;
	struct isp_contrast_param contrast;
	struct isp_hist_param hist;
	struct isp_hist_statistic_info hist_stat;
	struct isp_auto_contrast_param auto_contrast;
	struct isp_saturation_param saturation;
	struct isp_saturation_param saturation_bak;
	struct isp_hue_param hue;
	struct isp_hue_param hue_bak;
	struct isp_af_param af;
	struct isp_af_statistic_info af_stat;
	struct isp_edge_param edge;
	struct isp_emboss_param emboss;
	struct isp_fcs_param fcs;
	struct isp_css_param css;
	struct isp_hdr_param hdr;
	struct isp_hdr_index hdr_index;
	struct isp_global_gain_param global;
	struct isp_chn_gain_param chn;
	struct isp_flash_cali_param flash;
	struct isp_size src;
	struct isp_slice_param slice;
	struct isp_tune_block tune;
	struct isp_smart_light_param smart_light;
	//struct isp_pre_global_gain_param pre_global;
	struct isp_pre_wave_denoise_param pre_wave_denoise;

	struct isp_smart_in_param denoise_ranwei;
	struct isp_smart_in_param denoise_diswei;
	struct isp_smart_in_param pref_y;
	struct isp_smart_in_param pref_uv;
	struct isp_smart_in_param soft_y;
	struct isp_smart_in_param soft_uv;
	struct isp_smart_in_param fit_gamma;
	struct isp_smart_in_param edge_detail;
	struct isp_smart_in_param edge_smooth;
	struct isp_smart_in_param edge_strength;
	struct isp_smart_in_param bpc_flat;
	struct isp_smart_in_param bpc_std;
	struct isp_smart_in_param bpc_texture;
	uint32_t denoise_enable;
	uint32_t pref_enable;
	uint32_t soft_enable;
	uint32_t gamma_enable;
	uint32_t edge_enable;
	uint32_t bpc_enable;
	uint32_t reserved[134];

	struct isp_blc_offset blc_offset[8];
	struct isp_edge_param edge_tab[16];
	struct isp_cce_matrix cce_tab[16];
	struct isp_gamma_tab gamma_tab[7];
	struct isp_denoise_param_tab denoise_tab[2];
	struct isp_lnc_map lnc_map_tab[ISP_SET_NUM][ISP_COLOR_TEMPRATURE_NUM];
	unsigned long isp_lnc_addr;
	uint16_t cmc_tab[ISP_CMC_NUM][9];
	uint16_t cmc_awb[9];
	uint32_t cmc_percent;
	uint8_t bright_tab[16];
	uint8_t contrast_tab[16];
	uint8_t saturation_tab[16];
	int32_t ev_tab[16];

	uint32_t awb_r_gain[9];
	uint32_t awb_g_gain[9];
	uint32_t awb_b_gain[9];

	/*R/G/B coef to change cce*/
	uint16_t cce_coef[3];

	uint32_t lnc_index;
	uint32_t cce_index;
	uint32_t gamma_index;
	uint32_t cmc_index;
	uint32_t param_index;
	uint32_t video_param_index;
	uint32_t proc_param_index;
	uint32_t awb_get_stat;
	uint32_t af_get_stat;
	uint32_t wb_trim_conter;
	uint32_t wb_trim_valid;
	uint32_t awb_win_conter;

	struct isp_resolution_info input_size_trim[ISP_INPUT_SIZE_NUM_MAX];

	uint32_t isp_callback_bypass;
	struct isp_nbpc_param nbpc;
	void *handle_lsc_adv;

	int32_t  is_flash_eb;
	uint32_t flash_cmc_index;
	uint32_t flash_lnc_index;
	struct lsc_adv_info smart_lsc_log_info;
	struct isp_adv_lsc_param adv_lsc;

	uint8_t is_single;

	int32_t new_gamma_tab_curve_num;
	struct new_gamma_param new_gamma_lookup_tab[ISP_NEW_GAMMA_NUM];

	uint16_t   gain_tmp[1024*16];
	uint32_t camera_id;
};

struct isp_system{
	// isp system
	uint32_t isp_id;
	uint32_t handler_num;
	uint32_t isp_status;
	pthread_t monitor_thr;
	cmr_handle monitor_queue;
	pthread_t ctrl_thr;
	cmr_handle ctrl_queue;
	pthread_t proc_thr;
	cmr_handle proc_queue;
	uint32_t monitor_status;
	uint32_t ctrl_status;
	uint32_t proc_status;
	pthread_mutex_t cond_mutex;

	pthread_cond_t init_cond;
	pthread_cond_t deinit_cond;
	pthread_cond_t continue_cond;
	pthread_cond_t continue_stop_cond;
	pthread_cond_t signal_cond;
	pthread_cond_t ioctrl_cond;
	pthread_cond_t thread_common_cond;

	pthread_mutex_t ctrl_3a_mutex;
};

struct isp_param{
	struct isp_system system;
	struct isp_context context[2];
	struct isp_context cap_context[2];
};

struct isp_respond
{
	uint32_t rtn;
};

struct isp_context* ispGetContext(uint32_t handler_id);
struct isp_context* ispGetCapContext(uint32_t handler_id);
uint32_t ISP_Algin(uint32_t pLen , uint16_t algin_blk, uint16_t algin_bits);
uint32_t IspGetId(void);
int isp_change_param(uint32_t handler_id, enum isp_change_cmd cmd, void *param);
int32_t isp_set_gamma(struct isp_gamma_param* gamma, struct isp_gamma_tab* tab_ptr);
int32_t ispAfmEb(uint32_t handler_id, uint32_t skip_num);
int32_t ispAfmUeb(uint32_t handler_id);
int32_t ispCfgAwbm(uint32_t handler_id,struct isp_awbm_param* param_ptr);
int32_t ispAwbmEb_immediately(uint32_t handler_id);
struct isp_context* ispGetAlgContext(uint32_t handler_id);
void isp_perror(const char *prefix);
int32_t ispPreWDenoiseThrd(uint32_t handler_id, uint16_t thrs0, uint16_t thrs1);
int32_t ispPreWDenoiseBypass(uint32_t handler_id, uint8_t bypass);

/**----------------------------------------------------------------------------*
**						   Compiler Flag									  **
**----------------------------------------------------------------------------*/
#ifdef	 __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif
// End

