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
#ifndef _CMR_SETTING_H_
#define _CMR_SETTING_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "cmr_common.h"
#include "SprdOEMCamera.h"


#define PARAM_BUFFER_MAX       200/sizeof(cmr_int)
#define ISP_ALG_TIMEOUT        3 /*sec*/
#define ISP_PREFLASH_ALG_TIMEOUT 2/*sec*/
#define ISP_QUICK_AE_TIMEOUT        1

enum cmr_flash_status {
	FLASH_CLOSE = 0x0,
	FLASH_OPEN = 0x1,
	FLASH_TORCH = 0x2,/*user only set flash to close/open/torch state */
	FLASH_AUTO = 0x3,
	FLASH_CLOSE_AFTER_OPEN = 0x10,/* following is set to sensor */
	FLASH_HIGH_LIGHT = 0x11,
	FLASH_OPEN_ON_RECORDING = 0x22,
	FLASH_CLOSE_AFTER_AUTOFOCUS = 0x30,
	FLASH_NEED_QUIT = 0x31,
	FLASH_AF_DONE = 0x40,
	FLASH_WAIT_TO_CLOSE = 0x41,
	FLASH_STATUS_MAX
};

enum setting_cmd_type {
	SETTING_NONE = CAMERA_PARAM_TYPE_MAX,
	SETTING_GET_PREVIEW_ANGLE,
	SETTING_GET_CAPTURE_ANGLE,
	SETTING_GET_ZOOM_PARAM,
	SETTING_GET_ENCODE_ANGLE,
	SETTING_GET_EXIF_INFO,
	SETTING_GET_JPEG_QUALITY,
	SETTING_GET_THUMB_QUALITY,
	SETTING_GET_THUMB_SIZE,
	SETTING_GET_ROTATION_CAPTURE,
	SETTING_GET_SHOT_NUMBER,
	SETTING_SET_ENVIRONMENT,
	SETTING_GET_CAPTURE_SIZE,
	SETTING_GET_CAPTURE_FORMAT,
	SETTING_GET_PREVIEW_SIZE,
	SETTING_GET_PREVIEW_FORMAT,
	SETTING_GET_VIDEO_SIZE,
	SETTING_GET_HDR,
	SETTING_GET_ANDROID_ZSL_FLAG,
	SETTING_CTRL_FLASH,
	SETTING_GET_CAPTURE_MODE,
	SETTING_GET_DV_MODE,
	SETTING_SET_PRE_LOWFLASH,
	SETTING_GET_FLASH_STATUS,
	SETTING_SET_HIGHFLASH_AE_MEASURE,
	SETTING_GET_HW_FLASH_STATUS,
	SETTING_GET_PERFECT_SKINLEVEL,
        SETTING_GET_FLIP_ON,
#ifdef CONFIG_SPRD_PRIVATE_ZSL
	SETTING_GET_SPRD_ZSL_ENABLED,
#endif
	SETTING_TYPE_MAX
};

enum setting_io_type {
	SETTING_IO_GET_CAPTURE_SIZE,
	SETTING_IO_GET_ACTUAL_CAPTURE_SIZE,
	SETTING_IO_CTRL_FLASH,
	SETTING_IO_GET_PREVIEW_MODE,
	SETTING_IO_GET_FLASH_MAX_CAPACITY,
	SETTING_IO_TYPE_MAX
};


struct setting_capture_mode_param {
	cmr_uint        capture_mode;
	cmr_uint	    is_hdr;
};

struct setting_ctrl_flash_param {
	struct setting_capture_mode_param    capture_mode;
	cmr_uint                             is_active;
	cmr_uint                             flash_type;
	cmr_uint                             work_mode; //preview or capture
	cmr_uint                             is_dv;
	cmr_uint                             will_capture;
};

struct setting_cmd_parameter {
	cmr_uint                                     camera_id;
	union {
		cmr_uint                                 cmd_type_value;
		struct exif_spec_pic_taking_cond_tag     exif_pic_cond_info;
		struct cmr_zoom_param                    zoom_param;
		struct camera_position_type              position_info;
		struct img_size                          size_param;
		struct setting_ctrl_flash_param 		 ctrl_flash;
		struct cmr_preview_fps_param             preview_fps_param;
		cmr_int                                  param_buffer[PARAM_BUFFER_MAX]; /*address aligning for struct type coercion*/
		struct exif_info_tag                     *exif_all_info_ptr;
		struct cmr_ae_param                      ae_param;
		struct cmr_range_fps_param               range_fps;
	};
};

struct setting_flash_max_capacity {
	uint16_t max_time;
	uint16_t max_charge;
};

struct setting_io_parameter {
	cmr_uint                camera_id;
	union {
		cmr_uint            cmd_value;
		struct img_size     size_param;
		struct setting_flash_max_capacity flash_capacity;
	};
};


struct setting_init_in {
	cmr_handle               oem_handle;
    cmr_u32                  camera_id_bits;
    cmr_u32                  padding;
    cmr_int                  (*io_cmd_ioctl)(cmr_handle oem_handle, cmr_uint cmd_type,struct setting_io_parameter *param);
    cmr_int                  (*setting_sn_ioctl)(cmr_handle	oem_handle, cmr_uint cmd_type, struct common_sn_cmd_param *parm);
    cmr_int                  (*setting_isp_ioctl)(cmr_handle oem_handle, cmr_uint cmd_type, struct common_isp_cmd_param *parm);
    cmr_int                  (*get_setting_activity)(cmr_handle oem_handle, cmr_uint *is_active);
    cmr_before_set_cb        before_set_cb;
    cmr_after_set_cb         after_set_cb;
};

/*avoid head file reference,
* use void pointer
*/
typedef void* (*setting_get_pic_taking_cb)(void *priv_data);

cmr_int cmr_setting_init(struct setting_init_in *param_ptr, cmr_handle *out_setting_handle);
cmr_int cmr_setting_deinit(cmr_handle setting_handle);
cmr_int cmr_setting_ioctl(cmr_handle setting_handle, cmr_uint cmd_type, struct setting_cmd_parameter *parm);
cmr_int cmr_setting_isp_notice_done (cmr_handle setting_handle, void *data);
cmr_int cmr_setting_deinit_notice(cmr_handle setting_handle);
cmr_int cmr_setting_quick_ae_notice_done (cmr_handle setting_handle, void *data);
cmr_int cmr_af_start_notice_flash(cmr_handle setting_handle);
cmr_int cmr_af_cancel_notice_flash(cmr_handle setting_handle);
cmr_int cmr_pre_flash_notice_flash(cmr_handle setting_handle);
#ifdef __cplusplus
}
#endif

#endif
