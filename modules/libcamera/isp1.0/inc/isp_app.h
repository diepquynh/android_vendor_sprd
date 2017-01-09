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
#ifndef _ISP_APP_H_
#define _ISP_APP_H_
/*----------------------------------------------------------------------------*
**				Dependencies					*
**---------------------------------------------------------------------------*/
//#include <linux/types.h>
#include <sys/types.h>
#include "isp_exif.h"
#include "cmr_isp_type.h"
/**---------------------------------------------------------------------------*
**				Micro Define					*
**----------------------------------------------------------------------------*/

typedef int32_t ( *proc_callback)(uint32_t handler_id, int32_t mode, void* param_ptr, uint32_t param_len);

#define ISP_EVT_MASK	0x00000FF0
/**---------------------------------------------------------------------------*
**				Data Prototype					*
**----------------------------------------------------------------------------*/
//enum
enum isp_id{
	ISP_ID_SC8825=0x00000000,
	ISP_ID_SC8830=0x00010000,
	ISP_ID_SC9630=0x00020000,
	ISP_ID_MAX
};

enum isp_callback_evt{
	ISP_CALLBACK_EVT=0x00040000,
	ISP_CALLBACK_MAX
};

enum isp_callback_cmd{
	ISP_CTRL_CALLBAC=0x00000000,
	ISP_PROC_CALLBACK=0x00000100,
	ISP_AF_NOTICE_CALLBACK=0x00000200,
	ISP_SKIP_FRAME_CALLBACK=0x00000300,
	ISP_FLASH_AE_CALLBACK=0x00000400,
	ISP_AE_BAPASS_CALLBACK=0x00000500,
	ISP_FAST_AE_STAB_CALLBACK = 0x00000600,
	ISP_AE_STAB_CALLBACK = 0x00000700,
	ISP_AF_STAT_CALLBACK = 0x00000800,
	ISP_AF_STAT_END_CALLBACK = 0x00000900,
	ISP_AWB_STAT_CALLBACK = 0x00000A00,
	ISP_CONTINUE_AF_NOTICE_CALLBACK = 0x00000B00,
	ISP_SOF_CALLBACK = 0x00000C00,
	ISP_AE_CHG_CALLBACK = 0x00000D00,
	ISP_QUICK_MODE_DOWN=0X00000E00,
	ISP_FLASH_READY_CALLBACK = 0x00000F00,
	ISP_FLASH_STAB_CALLBACK = 0x00000F10,
	ISP_CALLBACK_CMD_MAX=0xffffffff
};


enum isp_handler_id{
	ISP_HANDLER_MAIN=0x00,
	ISP_HANDLER_SUB,
	ISP_HANDLER_MAX
};

enum isp_video_mode{
	ISP_VIDEO_MODE_CONTINUE=0x00,
	ISP_VIDEO_MODE_SINGLE,
	ISP_VIDEO_MODE_MAX
};

enum isp_focus_mode{
	ISP_FOCUS_NONE=0x00,
	ISP_FOCUS_TRIG,
	ISP_FOCUS_ZONE,
	ISP_FOCUS_MULTI_ZONE,
	ISP_FOCUS_MACRO,
	ISP_FOCUS_WIN,
	ISP_FOCUS_CONTINUE,
	ISP_FOCUS_VIDEO,
	ISP_FOCUS_BYPASS,
	ISP_FOCUS_MAX
};

enum isp_focus_move_mode{
	ISP_FOCUS_MOVE_START=0x00,
	ISP_FOCUS_MOVE_END,
	ISP_FOCUS_MOVE_MAX
};

enum isp_flash_mode {
	ISP_FLASH_PRE_BEFORE,
	ISP_FLASH_PRE_LIGHTING,
	ISP_FLASH_PRE_AFTER,
	ISP_FLASH_MAIN_BEFORE,
	ISP_FLASH_MAIN_LIGHTING,
	ISP_FLASH_MAIN_AE_MEASURE,
	ISP_FLASH_MAIN_AFTER,
	ISP_FLASH_MODE_MAX
};

enum isp_hdr_level{
	ISP_HDR_LOW=0x00,
	ISP_HDR_HIGH,
	ISP_HDR_BYPASS,
	ISP_HDR_LEVEL_MAX
};

enum isp_special_effect_mode{
	ISP_EFFECT_NORMAL=0x00,
	ISP_EFFECT_GRAY,
	ISP_EFFECT_WARM,
	ISP_EFFECT_GREEN,
	ISP_EFFECT_COOL,
	ISP_EFFECT_ORANGE,
	ISP_EFFECT_NEGTIVE,
	ISP_EFFECT_OLD,
	ISP_EFFECT_EMBOSS,
	ISP_EFFECT_MAX
};

enum isp_tune_level{
	ISP_LEVEL_0=0x00,
	ISP_LEVEL_1,
	ISP_LEVEL_2,
	ISP_LEVEL_3,
	ISP_LEVEL_4,
	ISP_LEVEL_5,
	ISP_LEVEL_6,
	ISP_LEVEL_7,
	ISP_LEVEL_8,
	ISP_LEVEL_9,
	ISP_LEVEL_10,
	ISP_LEVEL_11,
	ISP_LEVEL_12,
	ISP_LEVEL_13,
	ISP_LEVEL_14,
	ISP_LEVEL_15,
	ISP_TUNE_LEVEL_MAX
};

enum isp_ae_frame_mode{
	ISP_AE_AUTO=0x00,
	ISP_AE_FIX,
	ISP_AE_FRAME_MODE_MAX
};

enum isp_alg_mode{
	ISP_ALG_NORMAL=0x00,
	ISP_ALG_FAST,
	ISP_AE_BYPASS,
	ISP_AWB_ALG_FAST,
	ISP_AWB_BYPASS,
	ISP_AWB_FLASH_START_SET,
	ISP_AWB_FLASH_STOP_SET,
	ISP_ALG_MAX
};

enum isp_ae_weight{
	ISP_AE_WEIGHT_CENTER=0x00,
	ISP_AE_WEIGHT_AVG,
	ISP_AE_WEIGHT_SPOT,
	ISP_AE_WEIGHT_CUSTOMER,
	ISP_AE_WEIGHT_MAX
};

enum isp_flicker_mode{
	ISP_FLICKER_50HZ=0x00,
	ISP_FLICKER_60HZ,
	ISP_FLICKER_MAX
};

enum isp_flash_ctrl{
	ISP_FLASH_CLOSE=0x00,
	ISP_FLASH_PRV,
	ISP_FLASH_MAIN,
	ISP_FLASH_MAX
};

enum isp_ae_ctrl_mode{
	ISP_AE_CTRL_SET_INDEX=0x00,
	ISP_AE_CTRL_SET,
	ISP_AE_CTRL_GET,
	ISP_AE_CTRL_MODE_MAX
};

enum isp_ctrl_mode{
	ISP_CTRL_SET=0x00,
	ISP_CTRL_GET,
	ISP_CTRL_MODE_MAX
};

enum isp_ae_mode{
	ISP_AUTO=0x00,
	ISP_NIGHT,
	ISP_SPORT,
	ISP_PORTRAIT,
	ISP_LANDSCAPE,
	ISP_AE_MODE_MAX
};

enum isp_smart_ae {
	ISP_SMART_AE_NONE=0x00,
	ISP_SMART_AE_DENDOISE=0x01,
	ISP_SMART_AE_EDGE=0x02,
	ISP_SMART_AE_SATURATION=0x04,
	ISP_SMART_AE_MAX,
};

enum isp_iso{
	ISP_ISO_AUTO=0x00,
	ISP_ISO_100,
	ISP_ISO_200,
	ISP_ISO_400,
	ISP_ISO_800,
	ISP_ISO_1600,
	ISP_ISO_MAX
};

enum isp_awb_mode{
	ISP_AWB_INDEX0=0x00,
	ISP_AWB_INDEX1,
	ISP_AWB_INDEX2,
	ISP_AWB_INDEX3,
	ISP_AWB_INDEX4,
	ISP_AWB_INDEX5,
	ISP_AWB_INDEX6,
	ISP_AWB_INDEX7,
	ISP_AWB_INDEX8,
	ISP_AWB_AUTO,
	ISP_AWB_MAX
};

enum isp_smart_awb {
	ISP_SMART_AWB_NONE=0x00,
	ISP_SMART_AWB_LNC=0x01,
	ISP_SMART_AWB_CMC=0x02,
	ISP_SMART_AWB_MAX
};

enum isp_format{
	ISP_DATA_YUV422_3FRAME=0x00,
	ISP_DATA_YUV422_2FRAME,
	ISP_DATA_YVU422_2FRAME,
	ISP_DATA_YUYV,
	ISP_DATA_UYVY,
	ISP_DATA_YVYU,
	ISP_DATA_VYUY,
	ISP_DATA_YUV420_2FRAME,
	ISP_DATA_YVU420_2FRAME,
	ISP_DATA_YUV420_3_FRAME,
	ISP_DATA_NORMAL_RAW10,
	ISP_DATA_CSI2_RAW10,
	ISP_DATA_FORMAT_MAX
};

enum isp_ctrl_cmd{
	ISP_CTRL_AWB_MODE,
	ISP_CTRL_AE_MODE,
	ISP_CTRL_AE_MEASURE_LUM,
	ISP_CTRL_EV,
	ISP_CTRL_FLICKER,
	ISP_CTRL_ALG,
	ISP_CTRL_SPECIAL_EFFECT,
	ISP_CTRL_BRIGHTNESS,
	ISP_CTRL_CONTRAST,
	ISP_CTRL_HIST,
	ISP_CTRL_AUTO_CONTRAST,
	ISP_CTRL_SATURATION,
	ISP_CTRL_AF,/*12*/
	ISP_CTRL_AF_MODE,/*13*/
	ISP_CTRL_CSS,
	ISP_CTRL_HDR,
	ISP_CTRL_GLOBAL_GAIN,
	ISP_CTRL_CHN_GAIN,
	ISP_CTRL_GET_EXIF_INFO,
	ISP_CTRL_ISO,
	ISP_CTRL_WB_TRIM,
	ISP_CTRL_PARAM_UPDATE,
	ISP_CTRL_FLASH_EG,
	ISP_CTRL_VIDEO_MODE,
	ISP_CTRL_AF_STOP,/*24*/
	ISP_CTRL_AE_TOUCH,
	ISP_CTRL_AE_INFO,
	ISP_CTRL_SHARPNESS,
	ISP_CTRL_GET_FAST_AE_STAB,
	ISP_CTRL_GET_AE_STAB,
	ISP_CTRL_GET_AE_CHG,
	ISP_CTRL_GET_AWB_STAT,
	ISP_CTRL_GET_AF_STAT,
	ISP_CTRL_GAMMA,
	ISP_CTRL_DENOISE,
	ISP_CTRL_SMART_AE,
	ISP_CTRL_CONTINUE_AF,
	ISP_CTRL_FLASH_NOTICE,
	ISP_CTRL_FLASH_CTRL, // for isp tool
	ISP_CTRL_AE_CTRL, // for isp tool
	ISP_CTRL_AF_CTRL, // for isp tool
	ISP_CTRL_REG_CTRL, // for isp tool
	ISP_CTRL_FACE_AREA,
	ISP_CTRL_AF_END_INFO, // for isp tool
	ISP_CTRL_RANGE_FPS,
	ISP_CTRL_SET_AE_FPS, // for LLS feature
	ISP_CTRL_ISP_INFO,//for get isp current status information
	ISP_CTRL_FLASH_CAP_PROC,
	ISP_CTRL_FLASH_FRM_RATE,
	ISP_CTRL_FLASH_REBACK_MAXIDX,
	ISP_CTRL_AF_DENOISE,
	ISP_CTRL_GET_AEAWB_BYPASS_STATUS,// get ae awb bypass status
	ISP_CTRL_MAX
};

enum isp_capbility_cmd{
	ISP_VIDEO_SIZE,
	ISP_CAPTURE_SIZE,
	ISP_LOW_LUX_EB,
	ISP_CUR_ISO,
	ISP_DENOISE_LEVEL,
	ISP_DENOISE_INFO,
	ISP_REG_VAL,
	ISP_CTRL_GET_AE_LUM, //for LLS feature
	ISP_CAPBILITY_MAX
};

struct isp_ae_info {
	uint32_t min_fps;  //min frame rate
	uint32_t max_fps;  //max frame rate
	uint32_t line_time;  //time of line
	uint32_t gain;
};

struct isp_addr{
	cmr_uint chn0;
	cmr_uint chn1;
	cmr_uint chn2;
	cmr_uint chn0_len;
	cmr_uint chn1_len;
	cmr_uint chn2_len;
};

struct isp_pos{
	uint32_t x;
	uint32_t y;
};

struct isp_size{
	uint32_t w;
	uint32_t h;
};

struct isp_trim_size{
	uint32_t x;
	uint32_t y;
	uint32_t w;
	uint32_t h;
};

struct isp_pos_rect{
	uint32_t start_x;
	uint32_t start_y;
	uint32_t end_x;
	uint32_t end_y;
};

struct isp_skip_num{
	uint32_t skip_num;
};

struct isp_af_notice{
	uint32_t mode;
	uint32_t valid_win;
};

struct isp_continue_af_notice{
	uint32_t af_valid;
};

struct isp_flash_notice {
	enum isp_flash_mode mode;
	uint32_t flash_ratio;
	uint32_t will_capture;
};

struct isp_alg{
	enum isp_alg_mode mode;
	uint32_t flash_eb;
	uint32_t flash_ratio;
};

struct isp_af_stat{
	uint32_t bypass;
	uint32_t mode;
	struct isp_pos_rect win[9];
	uint32_t valid_win;
};

struct isp_af_win{
	enum isp_focus_mode mode;
	struct isp_pos_rect win[9];
	uint32_t valid_win;
	uint32_t ae_touch;
	struct isp_pos_rect ae_touch_rect;
	uint32_t ae_stab;
};

struct isp_ae_ctrl{
	enum isp_ae_ctrl_mode mode;
	uint32_t index;
	uint32_t lum;
	uint32_t shutter;
	uint32_t dummy;
	uint32_t again;
	uint32_t dgain;
	uint32_t skipa;
	uint32_t skipd;
};

struct isp_info_ctrl {
	uint32_t version_id;
	uint32_t len;
	uint32_t mode;
	uint16_t awb_gain_r;
	uint16_t awb_gain_g;
	uint16_t awb_gain_b;
	uint16_t reserved0;
	uint32_t ct;
	uint32_t bv;
};

struct isp_af_ctrl{
	enum isp_ctrl_mode mode;
	uint32_t step;
	uint32_t num;
	uint32_t stat_value[9];
};

struct isp_af_end_info{
	uint32_t suc_win;
};

struct isp_reg_ctrl{
	enum isp_ctrl_mode mode;
	uint32_t num;
	uint32_t reg_tab[40];
};

struct isp_smart_ae_param {
	uint8_t smart;
	uint8_t smart_mode;
	uint8_t smart_rotio;
	uint16_t smart_base_gain;
	uint16_t smart_wave_min;
	uint16_t smart_wave_max;
	uint8_t smart_pref_min;
	uint8_t smart_pref_max;
	uint8_t smart_pref_y_outdoor;
	uint8_t smart_pref_y_min;
	uint8_t smart_pref_y_mid;
	uint8_t smart_pref_y_max;
	uint8_t smart_pref_uv_outdoor;
	uint8_t smart_pref_uv_min;
	uint8_t smart_pref_uv_mid;
	uint8_t smart_pref_uv_max;
	uint8_t smart_denoise_min_index;
	uint8_t smart_denoise_mid_index;
	uint8_t smart_denoise_max_index;
	uint8_t smart_edge_min_index;
	uint8_t smart_edge_max_index;
	uint8_t smart_denoise_diswei_outdoor_index;
	uint8_t smart_denoise_diswei_min_index;
	uint8_t smart_denoise_diswei_mid_index;
	uint8_t smart_denoise_diswei_max_index;
	uint8_t smart_denoise_ranwei_outdoor_index;
	uint8_t smart_denoise_ranwei_min_index;
	uint8_t smart_denoise_ranwei_mid_index;
	uint8_t smart_denoise_ranwei_max_index;
	uint8_t smart_denoise_soft_y_outdoor_index;
	uint8_t smart_denoise_soft_y_min_index;
	uint8_t smart_denoise_soft_y_mid_index;
	uint8_t smart_denoise_soft_y_max_index;
	uint8_t smart_denoise_soft_uv_outdoor_index;
	uint8_t smart_denoise_soft_uv_min_index;
	uint8_t smart_denoise_soft_uv_mid_index;
	uint8_t smart_denoise_soft_uv_max_index;
	uint8_t smart_sta_start_index;
	uint8_t smart_sta_low_thr;
	uint8_t smart_sta_ratio1;
	uint8_t smart_sta_ratio;
};

struct isp_img_frm{
	enum isp_format img_fmt;
	struct isp_size img_size;
	struct isp_addr img_addr_phy;
	struct isp_addr img_addr_vir;
};

struct isp_face_info {
	uint32_t   sx;
	uint32_t   sy;
	uint32_t   ex;
	uint32_t   ey;
	uint32_t   brightness;
	int32_t    pose;
};

struct isp_face_area {
	uint16_t type;//focus or ae,
	uint16_t face_num;
	uint16_t frame_width;
	uint16_t frame_height;
	struct isp_face_info face_info[10];
};
struct isp_init_param{
	uint32_t isp_id;
	void* setting_param_ptr;
	struct isp_size size;
	proc_callback ctrl_callback;
	proc_callback self_callback;
	void *oem_handle;
	uint32_t camera_id;
	void* sensor_lsc_golden_data;
};

struct isp_video_limit{
	uint16_t width;
	uint16_t height;
	uint32_t res;
};

struct isp_video_start{
	struct isp_size size;
	enum isp_format format;
	enum isp_video_mode mode;
	uint32_t work_mode;
	uint32_t is_need_flash;
};

struct ips_in_param{
	struct isp_img_frm src_frame;
	uint32_t src_avail_height;
	uint32_t src_slice_height;
	struct isp_img_frm dst_frame;
	uint32_t dst_slice_height;
};

struct ips_out_param{
	uint32_t output_height;
};

struct ipn_in_param{
	uint32_t src_avail_height;
	uint32_t src_slice_height;
	struct isp_addr img_addr_phy;
	struct isp_addr src_addr_phy;
	struct isp_addr dst_addr_phy;
};

struct isp_chg_lnc_param {
	uint16_t index[2];
	uint16_t weight[2];
};

struct isp_range_fps {
	uint16_t min_fps;
	uint16_t max_fps;
};

struct isp_ae_fps {
	uint32_t min_fps;
	uint32_t max_fps;
};

struct isp_info {
	void *addr;
	int    size;
};

// The isp_handle in isp1.0 has no use, just keep the same isp interface with isp2.0
typedef void*           isp_handle;

int isp_init(struct isp_init_param* ptr, isp_handle* isp_handler);
int isp_deinit(isp_handle isp_handler);
int isp_capability(isp_handle isp_handler, enum isp_capbility_cmd cmd, void* param_ptr);
int isp_ioctl(isp_handle isp_handler, enum isp_ctrl_cmd cmd, void* param_ptr);
int isp_video_start(isp_handle isp_handler, struct isp_video_start* param_ptr);
int isp_video_stop(isp_handle isp_handler);
int isp_proc_start(isp_handle isp_handler, struct ips_in_param* in_param_ptr, struct ips_out_param* out_param_ptr);
int isp_proc_next(isp_handle isp_handler, struct ipn_in_param* in_ptr, struct ips_out_param *out_ptr);

/**---------------------------------------------------------------------------*/
#endif
// End

