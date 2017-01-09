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
#ifndef _CMR_SENSOR_H_
#define _CMR_SENSOR_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "cmr_common.h"
#include "sensor_drv_u.h"

#define CAP_MODE_BITS                16

enum sensor_cmd {
	SENSOR_WRITE_REG,
	SENSOR_READ_REG,
	SENSOR_BRIGHTNESS,
	SENSOR_CONTRAST,
	SENSOR_SHARPNESS,
	SENSOR_SATURATION,
	SENSOR_SCENE,
	SENSOR_IMAGE_EFFECT,
	SENSOR_BEFORE_SNAPSHOT,
	SENSOR_AFTER_SNAPSHOT,
	SENSOR_CHECK_NEED_FLASH,/*10*/
	SENSOR_WRITE_EV,
	SENSOR_WRITE_GAIN,
	SENSOR_SET_AF_POS,
	SENSOR_SET_WB_MODE,
	SENSOR_ISO,
	SENSOR_EXPOSURE_COMPENSATION,
	SENSOR_GET_EXIF,
	SENSOR_FOCUS,/*18*/
	SENSOR_ANTI_BANDING,
	SENSOR_VIDEO_MODE,/*20*/
	SENSOR_GET_STATUS,
	SENSOR_STREAM_ON,
	SENSOR_STREAM_OFF,
	SENSOR_SET_HDR_EV,
	SENSOR_GET_GAIN_THRS,
	SENSOR_RESTORE,
	SENSOR_WRITE,
	SENSOR_READ,
	SENSOR_MIRROR,
	SENSOR_FLIP,
	SENSOR_MONITOR,
	SENSOR_ACCESS_VAL,
	SENSOR_YUV_FPS,
	SENSOR_CMD_MAX
};

struct sensor_init_param {
	cmr_handle    oem_handle;
	cmr_uint      sensor_bits;
	cmr_uint      is_autotest;
	void          *private_data;
};

cmr_int cmr_sensor_init(struct sensor_init_param *init_param_ptr, cmr_handle *sensor_handle);

cmr_int cmr_sensor_deinit(cmr_handle sensor_handle);

cmr_int cmr_sensor_open(cmr_handle sensor_handle, cmr_u32 sensor_id_bits);

cmr_int cmr_sensor_close(cmr_handle sensor_handle, cmr_u32 sensor_id_bits);

void cmr_sensor_event_reg(cmr_handle sensor_handle, cmr_uint sensor_id, cmr_evt_cb  event_cb);

cmr_int cmr_sensor_stream_ctrl(cmr_handle sensor_handle, cmr_uint sensor_id, cmr_uint on_off);

cmr_int cmr_sensor_get_info(cmr_handle sensor_handle, cmr_uint sensor_id, struct sensor_exp_info *sensor_info);

cmr_int cmr_sensor_set_mode(cmr_handle sensor_handle, cmr_uint sensor_id, cmr_uint mode);

cmr_int cmr_sensor_set_mode_done(cmr_handle sensor_handle, cmr_uint sensor_id);

cmr_int cmr_sensor_get_mode(cmr_handle sensor_handle, cmr_uint sensor_id, cmr_uint *mode_ptr);

cmr_int cmr_sensor_update_isparm_from_file(cmr_handle sensor_handle, cmr_uint sensor_id);

cmr_int cmr_sensor_set_exif(cmr_handle sensor_handle, cmr_uint sensor_id, SENSOR_EXIF_CTRL_E cmd, cmr_uint param);

cmr_int cmr_sensor_get_exif(cmr_handle sensor_handle, cmr_uint sensor_id, EXIF_SPEC_PIC_TAKING_COND_T *sensor_exif_ptr);

cmr_int cmr_sensor_get_gain_thrs(cmr_handle sensor_handle, cmr_uint sensor_id, cmr_u32 *gain_thrs);

cmr_int cmr_sensor_get_raw_settings(cmr_handle sensor_handle, void *raw_setting, cmr_u32 camera_id);

cmr_int cmr_sensor_ioctl(cmr_handle sensor_handle, cmr_u32 sensor_id, cmr_uint cmd, cmr_uint arg);

cmr_int cmr_sensor_focus_init(cmr_handle sensor_handle, cmr_u32 sensor_id);

cmr_int cmr_sensor_get_autotest_mode(cmr_handle sensor_handle, cmr_u32 sensor_id, cmr_uint *is_autotest);

cmr_int cmr_sensor_get_flash_info(cmr_handle sensor_handle, cmr_u32 sensor_id, struct sensor_flash_level *level);

#ifdef __cplusplus
}
#endif

#endif
