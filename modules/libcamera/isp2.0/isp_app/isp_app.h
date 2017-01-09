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
#ifndef _ISP_CTRL_H_
#define _ISP_CTRL_H_
/*----------------------------------------------------------------------------*
**				Dependencies					*
**---------------------------------------------------------------------------*/
#ifndef WIN32
#include <sys/types.h>
#include "isp_exif.h"
#endif

#include "isp_type.h"

#ifdef WIN32
#include "../kernel_include/sprd_isp.h"
#else
#include "sprd_isp.h"
#endif


typedef isp_int ( *proc_callback)(isp_handle handler_id, uint32_t mode, void* param_ptr, uint32_t param_len);

#define ISP_EVT_MASK	 0x00000F00

#define ISP_FLASH_MAX_CELL	40
#define ISP_MODE_NUM_MAX 16

#define CAMERA_ISP_BINGING4AWB_FLAG 8 // buffer queue flag, sync with hal
#define CAMERA_ISP_RAWAEM_FLAG 11 // buffer queue flag, sync with hal

enum isp_callback_cmd {
	ISP_CTRL_CALLBACK = 0x00000000,
	ISP_PROC_CALLBACK = 0x00000100,
	ISP_AF_NOTICE_CALLBACK = 0x00000200,
	ISP_SKIP_FRAME_CALLBACK = 0x00000300,
	ISP_AE_STAB_CALLBACK = 0x00000400,
	ISP_AF_STAT_CALLBACK = 0x00000500,
	ISP_AF_STAT_END_CALLBACK = 0x00000600,
	ISP_AWB_STAT_CALLBACK = 0x00000700,
	ISP_CONTINUE_AF_NOTICE_CALLBACK = 0x00000800,
	ISP_AE_CHG_CALLBACK = 0x00000900,
	ISP_ONLINE_FLASH_CALLBACK=0x00000A00,
	ISP_QUICK_MODE_DOWN = 0x00000B00,
	ISP_AE_STAB_NOTIFY = 0x00000C00,
	ISP_AE_LOCK_NOTIFY = 0x00000D00,
	ISP_AE_UNLOCK_NOTIFY = 0x00000E00,
	ISP_CALLBACK_CMD_MAX=0xffffffff
};

enum isp_video_mode {
	ISP_VIDEO_MODE_CONTINUE = 0x00,
	ISP_VIDEO_MODE_SINGLE,
	ISP_VIDEO_MODE_MAX
};

enum isp_focus_mode {
	ISP_FOCUS_NONE = 0x00,
	ISP_FOCUS_TRIG,
	ISP_FOCUS_ZONE,
	ISP_FOCUS_MULTI_ZONE,
	ISP_FOCUS_MACRO,
	ISP_FOCUS_WIN,
	ISP_FOCUS_CONTINUE,
	ISP_FOCUS_MANUAL,
	ISP_FOCUS_VIDEO,
	ISP_FOCUS_BYPASS,
	ISP_FOCUS_MAX
};

enum isp_focus_move_mode {
	ISP_FOCUS_MOVE_START = 0x00,
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
	ISP_FLASH_AF_DONE,
	ISP_FLASH_MODE_MAX
};

enum isp_ae_awb_lock_unlock_mode {
	ISP_AE_AWB_LOCK = 0x09,
	ISP_AE_AWB_UNLOCK = 0x0a,
	ISP_AE_AWB_MAX
};

enum isp_ae_mode {
	ISP_AUTO = 0x00,
	ISP_NIGHT,
	ISP_SPORT,
	ISP_PORTRAIT,
	ISP_LANDSCAPE,
	ISP_AE_MODE_MAX
};

enum isp_awb_mode {
	ISP_AWB_INDEX0 = 0x00,
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

enum isp_format {
	ISP_DATA_YUV422_3FRAME = 0x00,
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

enum isp_ctrl_cmd {
	ISP_CTRL_AWB_MODE = 0,
	ISP_CTRL_SCENE_MODE = 1,
	ISP_CTRL_AE_MEASURE_LUM = 2,
	ISP_CTRL_EV = 3,
	ISP_CTRL_FLICKER = 4,
	ISP_CTRL_AEAWB_BYPASS = 5,
	ISP_CTRL_SPECIAL_EFFECT = 6,
	ISP_CTRL_BRIGHTNESS = 7,
	ISP_CTRL_CONTRAST = 8,
	ISP_CTRL_HIST,
	ISP_CTRL_AUTO_CONTRAST,
	ISP_CTRL_SATURATION = 11,
	ISP_CTRL_AF,
	ISP_CTRL_AF_MODE = 13,
	ISP_CTRL_CSS,
	ISP_CTRL_HDR = 15,
	ISP_CTRL_GLOBAL_GAIN,
	ISP_CTRL_CHN_GAIN,
	ISP_CTRL_GET_EXIF_INFO,
	ISP_CTRL_ISO = 19,
	ISP_CTRL_WB_TRIM,
	ISP_CTRL_PARAM_UPDATE,
	ISP_CTRL_FLASH_EG,
	ISP_CTRL_VIDEO_MODE,
	ISP_CTRL_AF_STOP,
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
	ISP_CTRL_AF_DENOISE,
	ISP_CTRL_FLASH_CTRL = 38, // for isp tool
	ISP_CTRL_AE_CTRL = 39, // for isp tool
	ISP_CTRL_AF_CTRL = 40, // for isp tool
	ISP_CTRL_REG_CTRL = 41, // for isp tool
	ISP_CTRL_DENOISE_PARAM_READ, //for isp tool
	ISP_CTRL_DUMP_REG, //for isp tool
	ISP_CTRL_AF_END_INFO, // for isp tool
	ISP_CTRL_FLASH_NOTICE,
	ISP_CTRL_AE_FORCE_CTRL, // for mp tool
	ISP_CTRL_GET_AE_STATE, // for isp tool
	ISP_CTRL_SET_LUM, // for isp tool
	ISP_CTRL_GET_LUM, // for isp tool
	ISP_CTRL_SET_AF_POS, // for isp tool
	ISP_CTRL_GET_AF_POS, // for isp tool
	ISP_CTRL_GET_AF_MODE, // for isp tool
	ISP_CTRL_FACE_AREA,
	ISP_CTRL_SCALER_TRIM,
	ISP_CTRL_START_3A,
	ISP_CTRL_STOP_3A,
	IST_CTRL_SNAPSHOT_NOTICE,
	ISP_CTRL_SFT_READ,
	ISP_CTRL_SFT_WRITE,
	ISP_CTRL_SFT_SET_PASS,// added for sft
	ISP_CTRL_SFT_GET_AF_VALUE,// added for sft
	ISP_CTRL_SFT_SET_BYPASS,// added for sft
	ISP_CTRL_GET_AWB_GAIN,// for mp tool
	ISP_CTRL_RANGE_FPS,
	ISP_CTRL_SET_AE_FPS, // for LLS feature
	ISP_CTRL_BURST_NOTICE,// burst mode notice
	ISP_CTRL_GET_INFO,
	ISP_CTRL_SET_AE_NIGHT_MODE,
	ISP_CTRL_SET_AE_AWB_LOCK_UNLOCK,
	ISP_CTRL_SET_AE_LOCK_UNLOCK,
	ISP_CTRL_TOOL_SET_SCENE_PARAM,
	ISP_CTRL_IFX_PARAM_UPDATE,
	ISP_CTRL_FORCE_AE_QUICK_MODE,
	ISP_CTRL_DENOISE_PARAM_UPDATE, //for isp tool
	ISP_CTRL_SET_AE_EXP_TIME,
	ISP_CTRL_SET_AE_SENSITIVITY,
	ISP_CTRL_MAX
};

enum isp_capbility_cmd {
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

enum isp_ae_lock_unlock_mode {
	ISP_AE_UNLOCK,
	ISP_AE_LOCK,
	ISP_AE_LOCK_UNLOCK_MAX
};

enum isp_posture_type {
	ISP_POSTURE_ACCELEROMETER,
	ISP_POSTURE_MAGNETIC,
	ISP_POSTURE_ORIENTATION,
	ISP_POSTURE_GYRO,
	ISP_POSTURE_MAX
};

struct isp_af_notice {
	uint32_t mode;
	uint32_t valid_win;
};

enum isp_flash_type {
	ISP_FLASH_TYPE_PREFLASH,
	ISP_FLASH_TYPE_MAIN,
	ISP_FLASH_TYPE_MAX
};

struct isp_flash_power {
	int32_t max_charge; //mA
	int32_t max_time; //ms
};

struct isp_flash_notice {
	enum isp_flash_mode mode;
	uint32_t flash_ratio;
	uint32_t will_capture;
	struct isp_flash_power power;
	int32_t capture_skip_num;
};

struct isp_af_win {
	enum isp_focus_mode mode;
	struct isp_pos_rect win[9];
	uint32_t valid_win;
	uint32_t ae_touch;
	struct isp_pos_rect ae_touch_rect;
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

struct isp_img_frm {
	enum isp_format img_fmt;
	struct isp_size img_size;
	struct isp_addr img_addr_phy;
	struct isp_addr img_addr_vir;
	uint32_t format_pattern;
};

struct isp_flash_element {
	uint16_t index;
	uint16_t val;
};

struct isp_flash_cell {
	uint8_t type;
	uint8_t count;
	uint8_t def_val;
	struct isp_flash_element element[ISP_FLASH_MAX_CELL];
};

struct isp_ops {
	int32_t (*flash_get_charge)(void *handler, struct isp_flash_cell *cell);
	int32_t (*flash_get_time)(void *handler, struct isp_flash_cell *cell);
	int32_t (*flash_set_charge)(void *handler, uint8_t type, struct isp_flash_element *element);
	int32_t (*flash_set_time)(void *handler, uint8_t type, struct isp_flash_element *element);
};

struct isp_init_param {
	void* setting_param_ptr;
	struct isp_size size;
	proc_callback ctrl_callback;
	isp_handle oem_handle;
	struct isp_data_info calibration_param;
	uint32_t camera_id;
	void* sensor_lsc_golden_data;
	struct isp_ops ops;
	struct isp_data_info mode_ptr[ISP_MODE_NUM_MAX];
};

struct isp_video_limit {
	uint16_t width;
	uint16_t height;
	uint32_t res;
};

struct isp_video_start {
	struct isp_size size;
	enum isp_format format;
	enum isp_video_mode mode;
	uint32_t work_mode;
	uint32_t dv_mode;
	isp_uint lsc_buf_size;
	isp_uint lsc_buf_num;
	isp_uint lsc_phys_addr;
	isp_uint lsc_virt_addr;
	isp_uint b4awb_mem_size;
	isp_uint b4awb_mem_num;
	isp_uint b4awb_phys_addr_array[2];
	isp_uint b4awb_virt_addr_array[2];
	isp_uint anti_flicker_buf_size;
	isp_uint anti_flicker_buf_num;
	isp_uint anti_flicker_phys_addr;
	isp_uint anti_flicker_virt_addr;
	uint32_t is_need_flash;
	uint32_t capture_skip_num;
	void* cb_of_malloc;
	void* cb_of_free;
	void* buffer_client_data;
};

typedef isp_uint (*isp_cb_of_malloc)(isp_uint type, isp_uint *size_ptr, isp_uint *sum_ptr, isp_uint *phy_addr,
		isp_uint *vir_addr, void* private_data);
typedef isp_uint (*isp_cb_of_free)(isp_uint type, isp_uint *phy_addr, isp_uint *vir_addr, isp_uint sum, void* private_data);

struct ips_in_param {
	struct isp_img_frm src_frame;
	uint32_t src_avail_height;
	uint32_t src_slice_height;
	struct isp_img_frm dst_frame;
	uint32_t dst_slice_height;
};

struct ips_out_param {
	uint32_t output_height;
};

struct ipn_in_param {
	uint32_t src_avail_height;
	uint32_t src_slice_height;
	struct isp_addr img_addr_phy;
	struct isp_addr src_addr_phy;
	struct isp_addr dst_addr_phy;
};

struct isp_snapshot_notice {
	uint32_t type;
	uint32_t preview_line_time;
	uint32_t capture_line_time;
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

struct isp_hdr_ev_param {
	int32_t level;
	int32_t skip_frame_num; //return from isp
};

/**---------------------------------------------------------------------------*
**				API					*
**----------------------------------------------------------------------------*/

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

