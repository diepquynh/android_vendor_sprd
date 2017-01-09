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
#define LOG_TAG "cmr_setting"

#if (MINICAMERA != 1)
#include <math.h>
#endif
#include <cutils/properties.h>
#include "cmr_setting.h"
#include "isp_app.h"
#include "cmr_msg.h"
#include "cmr_common.h"
#include "sensor_drv_u.h"
#include "cmr_exif.h"
#include "cmr_oem.h"


#define SETTING_MSG_QUEUE_SIZE              5

#define SETTING_EVT_INIT                    (1 << 16)
#define SETTING_EVT_DEINIT                  (1 << 17)
#define SETTING_EVT_ZOOM                    (1 << 18)

#define SETTING_EVT_MASK_BITS               (cmr_uint)(SETTING_EVT_INIT | SETTING_EVT_DEINIT | \
		                                               SETTING_EVT_ZOOM)


#define INVALID_SETTING_BYTE                0xFF

#define DV_FLASH_ON_DV_WITH_PREVIEW         1


enum exif_orientation {
	ORIENTATION_UNDEFINED          = 0,
	ORIENTATION_NORMAL             = 1,
	ORIENTATION_FLIP_HORIZONTAL    = 2,	/* left right reversed mirror */
	ORIENTATION_ROTATE_180         = 3,
	ORIENTATION_FLIP_VERTICAL      = 4,	/* upside down mirror */
	ORIENTATION_TRANSPOSE          = 5,	/* flipped about top-left <--> bottom-right axis */
	ORIENTATION_ROTATE_90          = 6,	/* rotate 90 cw to right it */
	ORIENTATION_TRANSVERSE         = 7,	/* flipped about top-right <--> bottom-left axis */
	ORIENTATION_ROTATE_270         = 8	/* rotate 270 to right it */
};

enum setting_general_type {
	SETTING_GENERAL_CONTRAST,
	SETTING_GENERAL_BRIGHTNESS,
	SETTING_GENERAL_SHARPNESS,
	SETTING_GENERAL_WB,
	SETTING_GENERAL_EFFECT,
	SETTING_GENERAL_ANTIBANDING,
	SETTING_GENERAL_AUTO_EXPOSURE_MODE,
	SETTING_GENERAL_ISO,
	SETTING_GENERAL_EXPOSURE_COMPENSATION,
	SETTING_GENERAL_PREVIEW_FPS,
	SETTING_GENERAL_PREVIEW_LLS_FPS,
	SETTING_GENERAL_SATURATION,
	SETTING_GENERAL_SCENE_MODE,
	SETTING_GENERAL_SENSOR_ROTATION,
	SETTING_GENERAL_AE_LOCK_UNLOCK,
	SETTING_GENERAL_ZOOM,
	SETTING_GENERAL_TYPE_MAX
};

struct setting_exif_unit{
	struct img_size                    actual_picture_size;
	struct img_size                    picture_size;
};
struct setting_local_param {
	cmr_uint                           is_dv_mode;
	cmr_uint                           is_dv_mode_for_yuv;
	struct exif_info_tag               exif_all_info;
	struct sensor_exp_info             sensor_static_info;
	cmr_uint                           is_sensor_info_store;
	struct setting_exif_unit           exif_unit;
	cmr_uint                           exif_flash;
	EXIF_SPEC_PIC_TAKING_COND_T        exif_pic_taking;
};

struct setting_flash_param {
	cmr_uint      flash_mode;
	cmr_uint      flash_status;
	cmr_uint      auto_flash_status;
	cmr_uint      has_preflashed;
	cmr_uint      flash_hw_status;
};

struct setting_hal_common {
	cmr_uint      focal_length;
	cmr_uint      brightness;
	cmr_uint      contrast;
	cmr_uint      effect;
	cmr_uint      exposure_compensation;
	cmr_uint      wb_mode;
	cmr_uint      saturation;
	cmr_uint      sharpness;
	cmr_uint      scene_mode;
	cmr_uint      antibanding_mode;
	cmr_uint      iso;
	cmr_uint      video_mode;
	cmr_uint      frame_rate;
	cmr_uint      auto_exposure_mode;
};

enum zoom_status {
	ZOOM_IDLE,
	ZOOM_UPDATING
};

struct setting_zoom_unit{
	struct setting_cmd_parameter  in_zoom;
	cmr_int                       is_changed;
	cmr_int                       status;
	cmr_int                       is_sended_msg;
};

struct setting_hal_param {
	struct setting_hal_common      hal_common;
	struct cmr_zoom_param          zoom_value;
	cmr_uint                       sensor_orientation;	/*screen orientation: landscape and portrait */
	cmr_uint                       capture_angle;

	cmr_uint                       jpeg_quality;
	cmr_uint                       thumb_quality;
	cmr_uint                       is_rotation_capture;
	cmr_uint                       encode_angle;
	cmr_uint                       encode_rotation;

	struct img_size                preview_size;
	cmr_uint                       preview_format;
	cmr_uint                       preview_angle;

	struct img_size                video_size;

	struct img_size                thumb_size;
	struct img_size                capture_size;
	cmr_uint                       capture_format;
	cmr_uint                       capture_mode;
	cmr_uint                       shot_num;
	cmr_uint                       perfect_skinlevel;
        cmr_uint                       flip_on;

	struct setting_flash_param     flash_param;

	struct camera_position_type    position_info;
	cmr_uint                       is_hdr;
	cmr_uint                       is_android_zsl;
	struct cmr_range_fps_param     range_fps;
	cmr_uint                       is_update_range_fps;
#ifdef CONFIG_SPRD_PRIVATE_ZSL
	cmr_uint                       sprd_zsl_enabled;
#endif
	cmr_uint                       is_ae_lock;
	struct isp_flash_notice                flash_notice;
};

struct setting_camera_info {
	struct setting_hal_param       hal_param;
	struct setting_local_param     local_param;
};

struct setting_component {
	cmr_handle                    thread_handle;
	pthread_mutex_t               status_lock;
	pthread_mutex_t               ctrl_lock;
	struct setting_init_in        init_in;
	struct setting_camera_info    camera_info[CAMERA_ID_MAX];
	struct setting_zoom_unit      zoom_unit;
	cmr_int                       force_set;
	uint32_t                      isp_is_timeout;
	sem_t                         isp_sem;
	pthread_mutex_t               isp_mutex;
	sem_t                         quick_ae_sem;
	uint32_t flash_need_quit;
};

struct setting_item {
	cmr_uint                      cmd_type;
	cmr_int                       (*setting_ioctl)(struct setting_component *cpt,
				                              struct setting_cmd_parameter *parm);
};

struct setting_general_item {
	cmr_uint       general_type;
	cmr_uint       *cmd_type_value;
	cmr_int        isp_cmd;
	cmr_int        sn_cmd;
};


struct setting_exif_cb_param {
	struct setting_component *cpt;
	struct setting_cmd_parameter *parm;
};

static cmr_int setting_isp_wait_notice_withtime (struct setting_component *cpt, cmr_uint timeout);
static cmr_int cmr_setting_clear_sem (struct setting_component *cpt);
/** LOCAL FUNCTION DECLARATION
 */
static cmr_int setting_set_flashdevice(struct setting_component *cpt,
									struct setting_cmd_parameter *parm,
								    enum cmr_flash_status flash_status);
static cmr_int setting_isp_wait_notice (struct setting_component *cpt);
static cmr_int setting_isp_flash_notify(struct setting_component *cpt,
									struct setting_cmd_parameter *parm,
									enum isp_flash_mode flash_mode);
/**FUNCTION
 */
static struct setting_hal_param *get_hal_param(struct setting_component *cpt,
					                                    cmr_uint camera_id)
{
	return &cpt->camera_info[camera_id].hal_param;
}

static struct setting_local_param *get_local_param(struct setting_component *cpt,
						                                    cmr_uint camera_id)
{
	return &cpt->camera_info[camera_id].local_param;
}

static struct setting_flash_param *get_flash_param(struct setting_component *cpt,
					                                    cmr_uint camera_id)
{
	return &cpt->camera_info[camera_id].hal_param.flash_param;
}

static cmr_int setting_get_sensor_static_info(struct setting_component *cpt,
                                          struct setting_cmd_parameter *parm,
                                          struct sensor_exp_info *static_info)
{
	cmr_int                          ret = 0;
	struct setting_init_in           *init_in = &cpt->init_in;
	struct setting_local_param       *local_param = get_local_param(cpt, parm->camera_id);
	cmr_uint                         cmd = COM_SN_GET_INFO;
	struct common_sn_cmd_param      sn_param;


	if (!local_param->is_sensor_info_store && init_in->setting_sn_ioctl) {
		sn_param.camera_id = parm->camera_id;
		ret = (*init_in->setting_sn_ioctl)(init_in->oem_handle, cmd, &sn_param);
		local_param->is_sensor_info_store = 1;
		cmr_copy(static_info, &sn_param.sensor_static_info, sizeof(sn_param.sensor_static_info));
	}

	return ret;
}

static cmr_int setting_is_rawrgb_format(struct setting_component *cpt,
                                       struct setting_cmd_parameter *parm)
{
	cmr_int                          is_raw = 0;
	struct setting_local_param       *local_param = get_local_param(cpt, parm->camera_id);


	setting_get_sensor_static_info(cpt, parm, &local_param->sensor_static_info);
	if (IMG_DATA_TYPE_RAW == local_param->sensor_static_info.image_format) {
		is_raw = 1;
	}

	return is_raw;
}

static cmr_uint setting_is_active(struct setting_component *cpt)
{
	struct setting_init_in  *init_in = &cpt->init_in;
	cmr_uint                is_active = 0;

	if (init_in->get_setting_activity) {
		(*init_in->get_setting_activity)(init_in->oem_handle, &is_active);
	}

	return is_active;
}

static cmr_uint setting_get_skip_number(struct setting_component *cpt,
                                      struct setting_cmd_parameter *parm,
                                       cmr_int is_check_night_mode)
{
	cmr_uint                         skip_number = 0;
	struct setting_local_param       *local_param = get_local_param(cpt, parm->camera_id);
	struct setting_hal_param         *hal_param =  get_hal_param(cpt, parm->camera_id);


	setting_get_sensor_static_info(cpt, parm, &local_param->sensor_static_info);

	if (!is_check_night_mode) {
		skip_number = (local_param->sensor_static_info.change_setting_skip_num & 0xffff);
	} else {
		if (CAMERA_SCENE_MODE_NIGHT == hal_param->hal_common.scene_mode) {
			skip_number = ((local_param->sensor_static_info.change_setting_skip_num >>16) & 0xffff);
		}
	}

	return skip_number;
}

static cmr_int setting_sn_ctrl(struct setting_component *cpt, cmr_uint sn_cmd,
			                          struct setting_cmd_parameter *parm)
{
	cmr_int                             ret = 0;
	struct setting_init_in              *init_in = &cpt->init_in;
	struct common_sn_cmd_param         sn_param;


	if (sn_cmd >= COM_SN_TYPE_MAX) {
		return ret;
	}

	if (init_in->setting_sn_ioctl) {
		sn_param.camera_id = parm->camera_id;
		sn_param.cmd_value = parm->cmd_type_value;
		if(COM_SN_SET_YUV_FPS == sn_cmd)
			sn_param.range_fps = parm->range_fps;
		ret = (*init_in->setting_sn_ioctl)(init_in->oem_handle, sn_cmd, &sn_param);
		if (ret) {
			CMR_LOGW("sn ctrl failed");
		}
		parm->cmd_type_value = sn_param.cmd_value;
	}

	return ret;
}

static cmr_uint camera_param_to_isp(cmr_uint cmd,
											struct setting_cmd_parameter *parm,
											struct common_isp_cmd_param *isp_param)
{
	cmr_uint in_param = parm->cmd_type_value;
	cmr_uint out_param = in_param;

	switch (cmd) {
	case COM_ISP_SET_AE_MODE: {
			switch (in_param) {
			case CAMERA_SCENE_MODE_AUTO:
			case CAMERA_SCENE_MODE_NORMAL:
				out_param = ISP_AUTO;
				break;

			case CAMERA_SCENE_MODE_NIGHT:
				out_param = ISP_NIGHT;
				break;

			case CAMERA_SCENE_MODE_ACTION:
				out_param = ISP_SPORT;
				break;

			case CAMERA_SCENE_MODE_PORTRAIT:
				out_param = ISP_PORTRAIT;
				break;

			case CAMERA_SCENE_MODE_LANDSCAPE:
				out_param = ISP_LANDSCAPE;
				break;

			default:
				out_param = CAMERA_SCENE_MODE_AUTO;
				break;
			}
			isp_param->cmd_value = out_param;
			break;
		}

	case COM_ISP_SET_VIDEO_MODE:
		isp_param->cmd_value = parm->preview_fps_param.frame_rate;
		break;
	case COM_ISP_SET_RANGE_FPS:
		isp_param->range_fps = parm->range_fps;
		break;
	default:
		isp_param->cmd_value = out_param;
		break;

	}

	return out_param;
}

static cmr_int setting_isp_ctrl(struct setting_component *cpt, cmr_uint isp_cmd,
				                        struct setting_cmd_parameter *parm)
{
	cmr_int                             ret = 0;
	struct setting_init_in              *init_in = &cpt->init_in;
	struct common_isp_cmd_param         isp_param;


	if (isp_cmd >= COM_ISP_TYPE_MAX) {
		return ret;
	}

	if (init_in->setting_isp_ioctl) {
		isp_param.camera_id = parm->camera_id;
		camera_param_to_isp(isp_cmd, parm, &isp_param);

		if (COM_ISP_SET_FPS_LLS_MODE == isp_cmd) {
			isp_param.fps_param.min_fps = parm->preview_fps_param.frame_rate;
			isp_param.fps_param.max_fps = parm->preview_fps_param.video_mode;
		}

		ret = (*init_in->setting_isp_ioctl)(init_in->oem_handle, isp_cmd, &isp_param);
		if (ret) {
			CMR_LOGE("sn ctrl failed");
		}
		parm->cmd_type_value = isp_param.cmd_value;
	}

	return ret;
}

static cmr_int setting_before_set_ctrl(struct setting_component *cpt, enum preview_param_mode mode)
{
	cmr_int                  ret = 0;
	struct setting_init_in   *init_in = &cpt->init_in;

	if (init_in->before_set_cb) {
		ret = (*init_in->before_set_cb)(init_in->oem_handle, mode);
		if (ret) {
			CMR_LOGE("before cb failed");
		}
	}

	return ret;
}

static cmr_int setting_after_set_ctrl(struct setting_component *cpt,
				                      struct after_set_cb_param  *param)
{
	cmr_int                   ret = 0;
	struct setting_init_in    *init_in = &cpt->init_in;

	if (init_in->after_set_cb) {
		ret = (*init_in->after_set_cb)(init_in->oem_handle, param);
		if (ret) {
			CMR_LOGE("before cb failed");
		}
	}

	return ret;
}

static uint32_t setting_get_exif_orientation(int degrees)
{
	uint32_t orientation = 1;	/*ExifInterface.ORIENTATION_NORMAL; */

	degrees %= 360;
	if (degrees < 0)
		degrees += 360;
	if (degrees < 45) {
		orientation = ORIENTATION_NORMAL;	/*ExifInterface.ORIENTATION_NORMAL; */
	} else if (degrees < 135) {
		orientation = ORIENTATION_ROTATE_90;	/*ExifInterface.ORIENTATION_ROTATE_90; */
	} else if (degrees < 225) {
		orientation = ORIENTATION_ROTATE_180;	/*ExifInterface.ORIENTATION_ROTATE_180; */
	} else {
		orientation = ORIENTATION_ROTATE_270;	/*ExifInterface.ORIENTATION_ROTATE_270; */
	}
	CMR_LOGD("rotation degrees: %d, orientation: %d.", degrees,
		 orientation);
	return orientation;
}

static cmr_int setting_set_general(struct setting_component *cpt,
								   enum setting_general_type  type,
				                   struct setting_cmd_parameter *parm)
{
	cmr_int                       ret = 0;
	cmr_uint                      type_val = 0;
	struct setting_hal_param      *hal_param =  get_hal_param(cpt, parm->camera_id);
	enum img_skip_mode            skip_mode = 0;
	cmr_uint                      skip_number = 0;
	struct setting_general_item   general_list[] = {
		{SETTING_GENERAL_CONTRAST, &hal_param->hal_common.contrast, COM_ISP_SET_CONTRAST, COM_SN_SET_CONTRAST},
		{SETTING_GENERAL_BRIGHTNESS, &hal_param->hal_common.brightness, COM_ISP_SET_BRIGHTNESS, COM_SN_SET_BRIGHTNESS},
		{SETTING_GENERAL_SHARPNESS, &hal_param->hal_common.sharpness, COM_ISP_SET_SHARPNESS, COM_SN_SET_SHARPNESS},
		{SETTING_GENERAL_WB, &hal_param->hal_common.wb_mode, COM_ISP_SET_AWB_MODE, COM_SN_SET_WB_MODE},
		{SETTING_GENERAL_EFFECT, &hal_param->hal_common.effect, COM_ISP_SET_SPECIAL_EFFECT, COM_SN_SET_IMAGE_EFFECT},
		{SETTING_GENERAL_ANTIBANDING, &hal_param->hal_common.antibanding_mode, COM_ISP_SET_ANTI_BANDING, COM_SN_SET_ANTI_BANDING},
		{SETTING_GENERAL_AUTO_EXPOSURE_MODE, &hal_param->hal_common.auto_exposure_mode, COM_ISP_SET_AE_MEASURE_LUM, COM_SN_TYPE_MAX},
		{SETTING_GENERAL_ISO, &hal_param->hal_common.iso, COM_ISP_SET_ISO, COM_SN_SET_ISO},
		{SETTING_GENERAL_EXPOSURE_COMPENSATION, &hal_param->hal_common.exposure_compensation, COM_ISP_SET_EV, COM_SN_SET_EXPOSURE_COMPENSATION},
		{SETTING_GENERAL_PREVIEW_FPS, &hal_param->hal_common.frame_rate, COM_ISP_SET_VIDEO_MODE, COM_SN_SET_VIDEO_MODE},
		{SETTING_GENERAL_PREVIEW_LLS_FPS, &hal_param->hal_common.frame_rate, COM_ISP_SET_FPS_LLS_MODE, COM_SN_SET_FPS_LLS_MODE},
		{SETTING_GENERAL_SATURATION, &hal_param->hal_common.saturation, COM_ISP_SET_SATURATION, COM_SN_SET_SATURATION},
		{SETTING_GENERAL_SCENE_MODE, &hal_param->hal_common.scene_mode, COM_ISP_SET_AE_MODE, COM_SN_SET_PREVIEW_MODE},
		{SETTING_GENERAL_SENSOR_ROTATION, &hal_param->capture_angle, COM_ISP_TYPE_MAX, COM_SN_TYPE_MAX},
		{SETTING_GENERAL_AE_LOCK_UNLOCK, &hal_param->is_ae_lock, COM_ISP_SET_AE_LOCK_UNLOCK},
	};
	struct setting_general_item   *item = NULL;
	struct after_set_cb_param     after_cb_param;
	cmr_int                       is_check_night_mode = 0;

	if (type >= SETTING_GENERAL_ZOOM) {
		CMR_LOGE("type is invalid");
		return -CMR_CAMERA_INVALID_PARAM;
	}

	item = &general_list[type];
	switch (type) {
	case SETTING_GENERAL_AUTO_EXPOSURE_MODE:
		type_val = parm->ae_param.mode;
		break;
	case SETTING_GENERAL_PREVIEW_FPS:
		if (setting_is_rawrgb_format(cpt, parm)){
			type_val = parm->preview_fps_param.frame_rate;
		} else {
			hal_param->hal_common.frame_rate = parm->preview_fps_param.frame_rate;
			type_val = parm->preview_fps_param.video_mode;
			item->cmd_type_value = &hal_param->hal_common.video_mode;
			parm->cmd_type_value = type_val;
		}
		break;
	case SETTING_GENERAL_PREVIEW_LLS_FPS:
		if (setting_is_rawrgb_format(cpt, parm)){
			type_val = parm->preview_fps_param.frame_rate;
		}
		break;

	case SETTING_GENERAL_AE_LOCK_UNLOCK:
		if (setting_is_rawrgb_format(cpt, parm)) {
			ret = setting_isp_ctrl(cpt, item->isp_cmd, parm);
		}
		break;

	default:
		type_val = parm->cmd_type_value;
		break;
	}

	if (SETTING_GENERAL_AE_LOCK_UNLOCK == type) {
		goto setting_out;
	}

	if ((type_val != *item->cmd_type_value)||(cpt->force_set)) {
		if (setting_is_active(cpt)) {
			ret = setting_before_set_ctrl(cpt, PARAM_NORMAL);
			if (ret) {
				CMR_LOGE("failed %ld", ret);
				goto setting_out;
			}

			if (setting_is_rawrgb_format(cpt, parm)) {
				ret = setting_isp_ctrl(cpt, item->isp_cmd, parm);
			} else {
				ret = setting_sn_ctrl(cpt, item->sn_cmd, parm);
			}
			if (ret) {
				CMR_LOGE("failed %ld", ret);
				goto setting_out;
			}

			if (type == SETTING_GENERAL_PREVIEW_FPS) {
				if (parm->preview_fps_param.frame_rate != 0) {
					struct setting_cmd_parameter isoParm = {0};

					isoParm.camera_id = parm->camera_id;
					isoParm.cmd_type_value = 5;
					if (setting_is_rawrgb_format(cpt, &isoParm)) {
						ret = setting_isp_ctrl(cpt, COM_ISP_SET_ISO, &isoParm);
						if (ret) {
							CMR_LOGE("iso set failed %ld", ret);
							goto setting_out;
						}
					}
				}
				//always do
				//setting_sn_ctrl(cpt, item->sn_cmd, parm);
			}

			skip_mode = IMG_SKIP_SW;
			is_check_night_mode = 0;
			if (SETTING_GENERAL_SENSOR_ROTATION == type){
				is_check_night_mode = 1;
			}
			skip_number = setting_get_skip_number(cpt, parm, is_check_night_mode);
			after_cb_param.re_mode = PARAM_NORMAL;
			after_cb_param.skip_mode = skip_mode;
			after_cb_param.skip_number = skip_number;
			after_cb_param.timestamp = systemTime(CLOCK_MONOTONIC);
			ret =  setting_after_set_ctrl(cpt, &after_cb_param);
		}
		*item->cmd_type_value = type_val;
	}

setting_out:
	return ret;
}


static cmr_int setting_set_encode_angle(struct setting_component *cpt,
					                   struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);
	cmr_uint                    encode_angle = 0;
	cmr_uint                    encode_rotation = 0;


	if (hal_param->is_rotation_capture) {
		uint32_t orientation;

		encode_rotation = 0;
		orientation = setting_get_exif_orientation((int)parm->cmd_type_value);
		switch (orientation) {
		case ORIENTATION_NORMAL:
			encode_angle = IMG_ANGLE_0;
			break;

		case ORIENTATION_ROTATE_180:
			encode_angle = IMG_ANGLE_180;
			break;

		case ORIENTATION_ROTATE_90:
			encode_angle = IMG_ANGLE_90;
			break;

		case ORIENTATION_ROTATE_270:
			encode_angle = IMG_ANGLE_270;
			break;

		default:
			encode_angle = IMG_ANGLE_0;
			break;
		}
	} else {
		encode_rotation = parm->cmd_type_value;
		encode_angle = IMG_ANGLE_0;
	}
	hal_param->encode_rotation = encode_rotation;
	hal_param->encode_angle = encode_angle;

	CMR_LOGD("encode_angle %ld", hal_param->encode_angle);
	return ret;
}

static cmr_int setting_get_encode_angle(struct setting_component *cpt,
					                    struct setting_cmd_parameter *parm)
{
	cmr_int                      ret = 0;
	struct setting_hal_param     *hal_param = get_hal_param(cpt, parm->camera_id);;

	parm->cmd_type_value = hal_param->encode_angle;
	return ret;
}

static cmr_uint setting_flash_mode_to_status(struct setting_component *cpt,
											struct setting_cmd_parameter *parm,
                                             enum cmr_flash_mode f_mode)
{
	cmr_int                         ret = 0;
	cmr_uint                        autoflash = 0;
	cmr_uint                        status = FLASH_STATUS_MAX;
	struct setting_cmd_parameter    ctrl_param;
	struct setting_flash_param      *flash_param = get_flash_param(cpt, parm->camera_id);


	switch (f_mode) {
	case CAMERA_FLASH_MODE_OFF:
		status = FLASH_CLOSE;
		break;

	case CAMERA_FLASH_MODE_ON:
		status = FLASH_OPEN;
		break;

	case CAMERA_FLASH_MODE_TORCH:
#ifdef DV_FLASH_ON_DV_WITH_PREVIEW
		status = FLASH_TORCH;
#else
		status = FLASH_OPEN_ON_RECORDING;
#endif
		break;

	case CAMERA_FLASH_MODE_AUTO:
		ctrl_param.camera_id = parm->camera_id;
		if (setting_is_rawrgb_format(cpt, parm)) {
			ret = setting_isp_ctrl(cpt, COM_ISP_GET_LOW_LUX_EB, &ctrl_param);
		} else {
			ret = setting_sn_ctrl(cpt, COM_SN_GET_AUTO_FLASH_STATE, &ctrl_param);
		}
		CMR_LOGI("auto flash status %ld", ctrl_param.cmd_type_value);

		if (ret) {
			ctrl_param.cmd_type_value = 1;
			CMR_LOGW("Failed to read auto flash mode %ld", ret);
		}
		if (ctrl_param.cmd_type_value) {
			status = FLASH_OPEN;
		} else {
			status = FLASH_CLOSE;
		}
		flash_param->auto_flash_status = ctrl_param.cmd_type_value;
		break;

	default:
		break;
	}

	return status;
}

static cmr_int setting_flash_handle(struct setting_component *cpt,
						struct setting_cmd_parameter *parm,
						  cmr_uint flash_mode)
{
	cmr_uint                 status = FLASH_STATUS_MAX;
	cmr_int                      ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);
	struct setting_flash_param	*flash_param = &hal_param->flash_param;

	status = setting_flash_mode_to_status(cpt, parm, flash_mode);
	CMR_LOGD("status = %ld,flash_param->flash_status = %ld", status, flash_param->flash_status);
	if (status != flash_param->flash_status) {
		if (FLASH_CLOSE == status || FLASH_TORCH == status)
			ret = setting_set_flashdevice(cpt, parm, status);
		flash_param->flash_status = status;
	}

	return ret;
}

static cmr_int setting_get_HW_flash_status(struct setting_component *cpt,
				                      struct setting_cmd_parameter *parm)
{
	cmr_int                      ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);
	struct setting_flash_param	*flash_param = &hal_param->flash_param;

	parm->cmd_type_value = flash_param->flash_hw_status;

	return ret;
}

static cmr_int setting_is_need_flash(struct setting_component *cpt,
					              struct setting_cmd_parameter *parm);
static cmr_int setting_get_flash_status(struct setting_component *cpt,
				                      struct setting_cmd_parameter *parm)
{
	cmr_int                      ret = 0;

	parm->cmd_type_value = setting_is_need_flash(cpt, parm);
	CMR_LOGI("flash_status %ld",  parm->cmd_type_value);
	return ret;
}

static cmr_int app_cancel_flash_flow(struct setting_component *cpt,
				                      struct setting_cmd_parameter *parm)
{
	cmr_int                      ret = 0;
	struct setting_hal_param       *hal_param = get_hal_param(cpt, parm->camera_id);
	struct setting_local_param     *local_param = get_local_param(cpt, parm->camera_id);
	cmr_uint                       image_format = 0;
	cmr_uint                       oem_flash_mode = 0;

	image_format = local_param->sensor_static_info.image_format;
	oem_flash_mode = hal_param->flash_notice.mode;

	CMR_LOGI("oem_flash_mode %d",  oem_flash_mode);
	if (oem_flash_mode == ISP_FLASH_PRE_BEFORE || oem_flash_mode == ISP_FLASH_PRE_LIGHTING) {
		cmr_sem_post(&cpt->isp_sem);
	}

	if (IMG_DATA_TYPE_RAW == image_format) {
		setting_isp_flash_notify(cpt, parm, ISP_FLASH_PRE_AFTER);
	}

	return ret;
}

static cmr_int setting_set_flash_mode(struct setting_component *cpt,
				                      struct setting_cmd_parameter *parm)
{
	cmr_int                      ret = 0;
	struct setting_flash_param   *flash_param = get_flash_param(cpt, parm->camera_id);
	cmr_uint                      flash_mode = 0;
	cmr_uint                      status = 0;

	flash_mode = parm->cmd_type_value;
	CMR_LOGI("flash_mode = %d, flash_hw_status = %d", flash_mode, flash_param->flash_hw_status);
	if (CAMERA_FLASH_MODE_OFF == flash_mode && FLASH_OPEN == flash_param->flash_hw_status) {
		app_cancel_flash_flow(cpt, parm);
	}
	flash_param->flash_mode = flash_mode;
	setting_flash_handle(cpt, parm, flash_param->flash_mode);

	return ret;
}

static cmr_int setting_set_isp_flash_mode(struct setting_component *cpt,
				                      struct setting_cmd_parameter *parm)
{
	cmr_int                      ret = 0;
	struct setting_flash_param   *flash_param = get_flash_param(cpt, parm->camera_id);
	cmr_uint                      flash_mode = 0;
	cmr_uint                      status = 0;

	flash_mode = parm->ctrl_flash.flash_type;
	ret = setting_set_flashdevice(cpt, parm, flash_mode);

	return ret;
}

static cmr_int setting_set_auto_exposure_mode(struct setting_component *cpt,
                                          struct setting_cmd_parameter *parm)
{
	cmr_int                             ret = 0;
	struct setting_init_in              *init_in = &cpt->init_in;
	struct common_isp_cmd_param        isp_param;


	ret = setting_set_general(cpt, SETTING_GENERAL_AUTO_EXPOSURE_MODE, parm);
	if (CAMERA_AE_SPOT_METERING == parm->ae_param.mode) {
		if (setting_is_rawrgb_format(cpt, parm)) {
			isp_param.win_area = parm->ae_param.win_area;
			if (init_in->setting_isp_ioctl) {
				isp_param.camera_id = parm->camera_id;
				ret = (*init_in->setting_isp_ioctl)(init_in->oem_handle, COM_ISP_SET_AE_METERING_AREA, &isp_param);
			}
		}
	}

	return ret;
}

static cmr_int setting_set_brightness(struct setting_component *cpt,
				                      struct setting_cmd_parameter *parm)
{
	cmr_int ret = 0;

	ret = setting_set_general(cpt, SETTING_GENERAL_BRIGHTNESS, parm);

	return ret;
}

static cmr_int setting_set_contrast(struct setting_component *cpt,
				                    struct setting_cmd_parameter *parm)
{
	cmr_int ret = 0;

	ret = setting_set_general(cpt, SETTING_GENERAL_CONTRAST, parm);

	return ret;
}

static cmr_int setting_set_saturation(struct setting_component *cpt,
				                      struct setting_cmd_parameter *parm)
{
	cmr_int ret = 0;

	ret = setting_set_general(cpt, SETTING_GENERAL_SATURATION, parm);

	return ret;
}

static cmr_int setting_set_sharpness(struct setting_component *cpt,
				                     struct setting_cmd_parameter *parm)
{
	cmr_int ret = 0;

	ret = setting_set_general(cpt, SETTING_GENERAL_SHARPNESS, parm);

	return ret;
}


static cmr_int setting_set_effect(struct setting_component *cpt,
				                  struct setting_cmd_parameter *parm)
{
	cmr_int ret = 0;

	ret = setting_set_general(cpt, SETTING_GENERAL_EFFECT, parm);

	return ret;
}


static cmr_int setting_set_exposure_compensation(struct setting_component *cpt,
						                     struct setting_cmd_parameter *parm)
{
	cmr_int ret = 0;

	ret = setting_set_general(cpt, SETTING_GENERAL_EXPOSURE_COMPENSATION, parm);

	return ret;
}


static cmr_int setting_set_wb(struct setting_component *cpt,
			                 struct setting_cmd_parameter *parm)
{
	cmr_int ret = 0;

	ret = setting_set_general(cpt, SETTING_GENERAL_WB, parm);

	return ret;
}


static cmr_int setting_set_scene_mode(struct setting_component *cpt,
				                      struct setting_cmd_parameter *parm)
{
	cmr_int                      ret = 0;
	struct setting_hal_param     *hal_param = get_hal_param(cpt, parm->camera_id);

	CMR_LOGI("set scene mode %ld",parm->cmd_type_value);
	if (CAMERA_SCENE_MODE_HDR == parm->cmd_type_value) {
		hal_param->is_hdr = 1;
	} else {
		hal_param->is_hdr = 0;
	}
	ret = setting_set_general(cpt, SETTING_GENERAL_SCENE_MODE, parm);

	return ret;
}


static cmr_int setting_set_antibanding(struct setting_component *cpt,
				                       struct setting_cmd_parameter *parm)
{
	cmr_int ret = 0;

	ret = setting_set_general(cpt, SETTING_GENERAL_ANTIBANDING, parm);

	return ret;
}

static cmr_int setting_set_iso(struct setting_component *cpt,
			                   struct setting_cmd_parameter *parm)
{
	cmr_int ret = 0;

	ret = setting_set_general(cpt, SETTING_GENERAL_ISO, parm);

	return ret;
}


static cmr_int setting_set_preview_fps(struct setting_component *cpt,
				                       struct setting_cmd_parameter *parm)
{
	cmr_int                      ret = 0;
	struct setting_local_param   *local_param = get_local_param(cpt, parm->camera_id);

	if (0 != parm->preview_fps_param.frame_rate) {
		local_param->is_dv_mode = 1;
	} else {
		local_param->is_dv_mode = 0;
	}

	ret = setting_set_general(cpt, SETTING_GENERAL_PREVIEW_FPS, parm);

	return ret;
}

static cmr_int setting_set_range_fps(struct setting_component *cpt,
				                       struct setting_cmd_parameter *parm)
{
	cmr_int                      ret = 0;
	struct setting_hal_param   *hal_param = get_hal_param(cpt, parm->camera_id);
	struct setting_local_param   *local_param = get_local_param(cpt, parm->camera_id);

	if (parm->range_fps.video_mode) {
		local_param->is_dv_mode = 1;
		local_param->is_dv_mode_for_yuv = 1;
	} else {
		local_param->is_dv_mode = 0;
		local_param->is_dv_mode_for_yuv = 0;
	}

	if (setting_is_rawrgb_format(cpt, parm)) {
		hal_param->is_update_range_fps = 1;
		hal_param->range_fps = parm->range_fps;
		//if (setting_is_active(cpt)) {
			ret = setting_isp_ctrl(cpt, COM_ISP_SET_RANGE_FPS, parm);
		//}
	}else{
		hal_param->is_update_range_fps = 1;
		hal_param->range_fps = parm->range_fps;

		if((hal_param->range_fps.min_fps==*(hal_param->range_fps.old_min_fps))
			&&(hal_param->range_fps.max_fps==*(hal_param->range_fps.old_max_fps))){
		}else{
			 *(hal_param->range_fps.old_min_fps)=hal_param->range_fps.min_fps;
			 *(hal_param->range_fps.old_max_fps)=hal_param->range_fps.max_fps;
			if(local_param->is_dv_mode_for_yuv == 0)
				setting_sn_ctrl(cpt, COM_SN_SET_YUV_FPS, parm);
		}
	}

	CMR_LOGI("min_fps=%ld, max_fps=%ld", hal_param->range_fps.min_fps,
			hal_param->range_fps.max_fps);

	return ret;
}


static cmr_int setting_set_preview_lls_fps(struct setting_component *cpt,
				                       struct setting_cmd_parameter *parm)
{
	cmr_int                      ret = 0;

	ret = setting_set_general(cpt, SETTING_GENERAL_PREVIEW_LLS_FPS, parm);

	return ret;
}


static cmr_int setting_set_shot_num(struct setting_component *cpt,
				                    struct setting_cmd_parameter *parm)
{
	cmr_int                      ret = 0;
	struct setting_hal_param     *hal_param = get_hal_param(cpt, parm->camera_id);

	hal_param->shot_num = parm->cmd_type_value;
	return ret;
}

static cmr_int setting_get_shot_num(struct setting_component *cpt,
				                   struct setting_cmd_parameter *parm)
{
	cmr_int                      ret = 0;
	struct setting_hal_param     *hal_param = get_hal_param(cpt, parm->camera_id);

	parm->cmd_type_value = hal_param->shot_num;
	return ret;
}

static cmr_int setting_set_focal_length(struct setting_component *cpt,
					                   struct setting_cmd_parameter *parm)
{
	cmr_int                    ret = 0;
	struct setting_hal_param   *hal_param = get_hal_param(cpt, parm->camera_id);

	hal_param->hal_common.focal_length = parm->cmd_type_value;
	return ret;
}

static cmr_int setting_process_zoom(struct setting_component *cpt,
				                   struct setting_cmd_parameter *parm)
{
	cmr_int                       ret = 0;
	cmr_uint                      type_val = 0;
	struct setting_hal_param      *hal_param =  get_hal_param(cpt, parm->camera_id);
	enum img_skip_mode            skip_mode = 0;
	cmr_uint                      skip_number = 0;

	struct after_set_cb_param     after_cb_param;
	struct cmr_zoom_param		  zoom_param;
	struct cmr_zoom_param         org_zoom;
	cmr_uint                      is_changed = 0;

	pthread_mutex_lock(&cpt->status_lock);
	org_zoom = hal_param->zoom_value;
	pthread_mutex_unlock(&cpt->status_lock);

	zoom_param = parm->zoom_param;
	if (zoom_param.mode == ZOOM_LEVEL) {
		if (zoom_param.zoom_level != org_zoom.zoom_level)
			is_changed = 1;
	} else if (zoom_param.mode == ZOOM_INFO) {
		const float EPSINON = 0.01f;
		float zoom_dif = zoom_param.zoom_info.zoom_ratio - org_zoom.zoom_info.zoom_ratio;
		float output_dif = zoom_param.zoom_info.output_ratio != org_zoom.zoom_info.output_ratio;

		if ((zoom_dif >= EPSINON || zoom_dif <= -EPSINON)
			|| (output_dif >= EPSINON || output_dif <= -EPSINON))
				is_changed = 1;
	}
	if (is_changed) {
		if (setting_is_active(cpt)) {
			ret = setting_before_set_ctrl(cpt, PARAM_ZOOM);
			if (ret) {
				CMR_LOGE("failed %ld", ret);
				goto setting_out;
			}

			skip_mode = IMG_SKIP_SW;
			skip_number = setting_get_skip_number(cpt, parm, 1);
			after_cb_param.re_mode = PARAM_ZOOM;
			after_cb_param.skip_mode = skip_mode;
			after_cb_param.skip_number = skip_number;
			after_cb_param.timestamp = systemTime(CLOCK_MONOTONIC);
			ret =  setting_after_set_ctrl(cpt, &after_cb_param);
			if (ret) {
				CMR_LOGE("after set failed %ld", ret);
				goto setting_out;
			}
		}
		/*update zoom unit after processed or not*/
		pthread_mutex_lock(&cpt->status_lock);
		hal_param->zoom_value = zoom_param;
		pthread_mutex_unlock(&cpt->status_lock);
	}

setting_out:
	return ret;
}

static cmr_int setting_zoom_push(struct setting_component *cpt,
                                 struct setting_cmd_parameter *parm)
{
	CMR_MSG_INIT(message);
	cmr_int                       ret = 0;

	pthread_mutex_lock(&cpt->ctrl_lock);
	cpt->zoom_unit.is_changed = 1;
	cpt->zoom_unit.in_zoom = *parm;
	if (ZOOM_IDLE == cpt->zoom_unit.status
		&& !cpt->zoom_unit.is_sended_msg) {
		message.msg_type = SETTING_EVT_ZOOM;
		message.sync_flag = CMR_MSG_SYNC_NONE;
		ret = cmr_thread_msg_send(cpt->thread_handle, &message);
		cpt->zoom_unit.is_sended_msg = 1;
	}
	pthread_mutex_unlock(&cpt->ctrl_lock);

	return ret;
}

static cmr_int setting_set_zoom_param(struct setting_component *cpt,
				                      struct setting_cmd_parameter *parm)
{
	cmr_int                    ret = 0;

	pthread_mutex_lock(&cpt->status_lock);
	ret = setting_zoom_push(cpt, parm);
	pthread_mutex_unlock(&cpt->status_lock);

	return ret;
}

static cmr_int setting_get_zoom_param(struct setting_component *cpt,
				                      struct setting_cmd_parameter *parm)
{
	cmr_int                    ret = 0;
	struct setting_hal_param   *hal_param = get_hal_param(cpt, parm->camera_id);
	pthread_mutex_lock(&cpt->status_lock);
	parm->zoom_param = hal_param->zoom_value;
	pthread_mutex_unlock(&cpt->status_lock);
	return ret;
}

static cmr_int setting_get_sensor_orientation(struct setting_component *cpt,
					                         struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);

	parm->cmd_type_value = hal_param->sensor_orientation;
	return ret;
}

static cmr_int setting_set_sensor_orientation(struct setting_component *cpt,
					                          struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);

	hal_param->sensor_orientation = parm->cmd_type_value;
	return ret;
}

static void get_seconds_from_double(double d, uint32_t *numerator, uint32_t *denominator)
{
	d = fabs(d);
	double degrees = (int)d;
	double remainder = d - degrees;
	double minutes = (int)(remainder * 60.0);
	double seconds = (((remainder * 60.0) - minutes) * 60.0);
	uint32_t i = 0;
	uint32_t j = 0;
	uint32_t num = 0;
	char str[20];
	double value=1.0;

	sprintf(str,"%f",seconds);
	while(str[i++]!='.')
		;
	j = strlen(str)-1;
	while(str[j] == '0')
		--j;
	num = j - i + 1;
	CMR_LOGI("%s, i=%d, j=%d, num=%d \n", str, i, j, num);

	for (i=0; i<num; i++)
		value *=10.0;

	*numerator = seconds*value;
	*denominator = value;

	CMR_LOGI("data=%f, num=%d, denom=%d \n", seconds, *numerator, *denominator);
}

static uint32_t get_data_from_double(double d, uint32_t type)/*0: dd, 1: mm, 2: ss*/
{
	d = fabs(d);
	double degrees = (int)d;
	double remainder = d - degrees;
	double minutes = (int)(remainder * 60.0);
	double seconds = (int)(((remainder * 60.0) - minutes) * 60.0);
	uint32_t retVal = 0;
	if (0 == type) {
		retVal = (int)degrees;
	} else if (1 == type) {
		retVal = (int)minutes;
	} else if (2 == type) {
		retVal = (int)seconds;
	}
	CMR_LOGV("GPS: type: %d, ret: 0x%x", type, retVal);
	return retVal;
}

static void* setting_get_pic_taking(void *priv_data)
{
	struct setting_exif_cb_param   *cb_param = (struct setting_exif_cb_param*)priv_data;
	struct setting_local_param     *local_param = get_local_param(cb_param->cpt, cb_param->parm->camera_id);
	struct setting_init_in         *init_in = &cb_param->cpt->init_in;
	cmr_int                        ret = 0;

	// read exif info from sensor only
	if (init_in->setting_sn_ioctl) {
		struct common_sn_cmd_param		sn_param;

		sn_param.camera_id = cb_param->parm->camera_id;
		ret = (*init_in->setting_sn_ioctl)(init_in->oem_handle,
                                         COM_SN_GET_EXIF_IMAGE_INFO,
                                         &sn_param);
		if (ret) {
			CMR_LOGW("sn ctrl failed");
		}
		cmr_copy(&local_param->exif_pic_taking,
                        &sn_param.exif_pic_info,
           sizeof(local_param->exif_pic_taking));
	}

	return (void*)&local_param->exif_pic_taking;
}

static cmr_int setting_update_gps_info(struct setting_component *cpt,
				                     struct setting_cmd_parameter *parm)
{
	cmr_int                           ret = 0;
	struct setting_hal_param          *hal_param = get_hal_param(cpt, parm->camera_id);
	struct setting_local_param        *local_param = get_local_param(cpt, parm->camera_id);
	struct camera_position_type       position = hal_param->position_info;
	JINF_EXIF_INFO_T                  *p_exif_info = &local_param->exif_all_info;


	uint32_t                          latitude_dd_numerator = get_data_from_double(position.latitude, 0);
	uint32_t                          latitude_dd_denominator = 1;
	uint32_t                          latitude_mm_numerator = get_data_from_double(position.latitude, 1);
	uint32_t                          latitude_mm_denominator = 1;
	uint32_t                          latitude_ss_numerator = 0;
	uint32_t                          latitude_ss_denominator = 0;
	uint32_t                          latitude_ref = 0;
	uint32_t                          longitude_ref = 0;
	uint32_t                          longitude_dd_numerator = 0;
	uint32_t                          longitude_dd_denominator = 0;
	uint32_t                          longitude_mm_numerator = 0;
	uint32_t                          longitude_mm_denominator = 0;
	uint32_t                          longitude_ss_numerator = 0;
	uint32_t                          longitude_ss_denominator = 0;
	char                              gps_date_buf[12] = {0};
	time_t                            timep;
	struct tm                         *p;
	char                              *gps_date;
	const char                        *gps_process_method;
	uint32_t                          gps_hour;
	uint32_t                          gps_minuter;
	uint32_t                          gps_second;

	get_seconds_from_double(position.latitude, &latitude_ss_numerator, &latitude_ss_denominator);
	if (position.latitude < 0.0) {
		latitude_ref = 1;
	} else {
		latitude_ref = 0;
	}
	longitude_dd_numerator = get_data_from_double(position.longitude, 0);
	longitude_dd_denominator = 1;
	longitude_mm_numerator = get_data_from_double(position.longitude, 1);
	longitude_mm_denominator = 1;
	get_seconds_from_double(position.longitude, &longitude_ss_numerator, &longitude_ss_denominator);
	if (position.longitude < 0.0) {
		longitude_ref = 1;
	} else {
		longitude_ref = 0;
	}

	if (0 == position.timestamp)
		time(&position.timestamp);
	p = gmtime(&position.timestamp);
	sprintf(gps_date_buf, "%4d:%02d:%02d", (1900+p->tm_year), (1+p->tm_mon),p->tm_mday);
	gps_date_buf[10] = '\0';
	gps_date = gps_date_buf;
	gps_hour = p->tm_hour;
	gps_minuter = p->tm_min;
	gps_second = p->tm_sec;
	CMR_LOGD("gps_data 2 = %s, %d:%d:%d \n", gps_date, gps_hour, gps_minuter, gps_second);

	gps_process_method = position.process_method;

	if (NULL != p_exif_info->gps_ptr) {
		if ((0 == latitude_dd_numerator) && (0 == latitude_mm_numerator) && (0 == latitude_ss_numerator)
			&& (0 == longitude_dd_numerator) && (0 == longitude_mm_numerator) && (0 == longitude_ss_numerator)) {
			/* if no Latitude and Longitude, do not write GPS to EXIF */
			CMR_LOGD("GPS: Latitude and Longitude is 0, do not write to EXIF: valid=%d \n",
					*(uint32_t*)&p_exif_info->gps_ptr->valid);
			memset(&p_exif_info->gps_ptr->valid,0,sizeof(EXIF_GPS_VALID_T));
		} else {
			p_exif_info->gps_ptr->valid.GPSLatitudeRef = 1;
			p_exif_info->gps_ptr->GPSLatitudeRef[0] = (0 == latitude_ref) ? 'N' : 'S';
			p_exif_info->gps_ptr->valid.GPSLongitudeRef = 1;
			p_exif_info->gps_ptr->GPSLongitudeRef[0] = (0 == longitude_ref) ? 'E' : 'W';

			p_exif_info->gps_ptr->valid.GPSLatitude = 1;
			p_exif_info->gps_ptr->GPSLatitude[0].numerator = latitude_dd_numerator;
			p_exif_info->gps_ptr->GPSLatitude[0].denominator = latitude_dd_denominator;
			p_exif_info->gps_ptr->GPSLatitude[1].numerator = latitude_mm_numerator;
			p_exif_info->gps_ptr->GPSLatitude[1].denominator = latitude_mm_denominator;
			p_exif_info->gps_ptr->GPSLatitude[2].numerator = latitude_ss_numerator;
			p_exif_info->gps_ptr->GPSLatitude[2].denominator = latitude_ss_denominator;

			p_exif_info->gps_ptr->valid.GPSLongitude = 1;
			p_exif_info->gps_ptr->GPSLongitude[0].numerator = longitude_dd_numerator;
			p_exif_info->gps_ptr->GPSLongitude[0].denominator = longitude_dd_denominator;
			p_exif_info->gps_ptr->GPSLongitude[1].numerator = longitude_mm_numerator;
			p_exif_info->gps_ptr->GPSLongitude[1].denominator = longitude_mm_denominator;
			p_exif_info->gps_ptr->GPSLongitude[2].numerator = longitude_ss_numerator;
			p_exif_info->gps_ptr->GPSLongitude[2].denominator = longitude_ss_denominator;

			p_exif_info->gps_ptr->valid.GPSAltitude = 1;
			p_exif_info->gps_ptr->GPSAltitude.numerator = position.altitude;
			CMR_LOGD("gps_ptr->GPSAltitude.numerator: %d.", p_exif_info->gps_ptr->GPSAltitude.numerator);
			p_exif_info->gps_ptr->GPSAltitude.denominator = 1;
			p_exif_info->gps_ptr->valid.GPSAltitudeRef = 1;

			if(NULL != gps_process_method) {
				const char ascii[] = {0x41, 0x53, 0x43, 0x49, 0x49, 0, 0, 0};
				p_exif_info->gps_ptr->valid.GPSProcessingMethod = 1;
				p_exif_info->gps_ptr->GPSProcessingMethod.count = strlen(gps_process_method)+sizeof(ascii) + 1;
				memcpy((char *)p_exif_info->gps_ptr->GPSProcessingMethod.ptr, ascii, sizeof(ascii));
				strcpy((char *)p_exif_info->gps_ptr->GPSProcessingMethod.ptr+sizeof(ascii), (char *)gps_process_method);
				/*add "ASCII\0\0\0" for cts test by lyh*/
			}

			p_exif_info->gps_ptr->valid.GPSTimeStamp = 1;
			p_exif_info->gps_ptr->GPSTimeStamp[0].numerator = gps_hour;
			p_exif_info->gps_ptr->GPSTimeStamp[1].numerator = gps_minuter;
			p_exif_info->gps_ptr->GPSTimeStamp[2].numerator = gps_second;

			p_exif_info->gps_ptr->GPSTimeStamp[0].denominator = 1;
			p_exif_info->gps_ptr->GPSTimeStamp[1].denominator = 1;
			p_exif_info->gps_ptr->GPSTimeStamp[2].denominator = 1;
			p_exif_info->gps_ptr->valid.GPSDateStamp = 1;
			strcpy((char *)p_exif_info->gps_ptr->GPSDateStamp, (char *)gps_date);
			CMR_LOGD("GPS: valid=%d \n", *(uint32_t*)&p_exif_info->gps_ptr->valid);
		}
	}
	cmr_bzero(&hal_param->position_info, sizeof(struct camera_position_type));
	return ret;
}

static cmr_int setting_get_exif_info(struct setting_component *cpt,
				                     struct setting_cmd_parameter *parm)
{
	cmr_int                           ret = 0;
	struct setting_init_in            *init_in = &cpt->init_in;
	struct setting_hal_param          *hal_param = get_hal_param(cpt, parm->camera_id);
	struct setting_local_param        *local_param = get_local_param(cpt, parm->camera_id);
	struct setting_exif_unit          *exif_unit = &local_param->exif_unit;
	struct setting_io_parameter       cmd_param;
	JINF_EXIF_INFO_T                  *p_exif_info = &local_param->exif_all_info;
	struct setting_exif_cb_param      cb_param;
	struct setting_flash_param        *flash_param = &hal_param->flash_param;
	struct img_size                 dealign_actual_pic_size;

	char                              datetime_buf[20] = {0};
	time_t                            timep;
	struct tm                         *p;
	char                              *datetime;
	uint32_t                          focal_length_numerator;
	uint32_t                          focal_length_denominator;
	char                              property[PROPERTY_VALUE_MAX];

	#define EXIF_DEF_MAKER            "Spreadtrum"
	#define EXIF_DEF_MODEL            "spxxxx"
	static const char  image_desc[] = "Exif_JPEG_420";
	static const char  copyright[]  = "Copyright,Spreadtrum,2011";

	if (FLASH_OPEN == flash_param->flash_hw_status) {
		setting_set_flashdevice(cpt, parm, (uint32_t)FLASH_CLOSE_AFTER_OPEN);
	}
	cb_param.cpt = cpt;
	cb_param.parm = parm;
	ret = cmr_exif_init(p_exif_info, setting_get_pic_taking, (void*)&cb_param);
	if (ret) {
		CMR_LOGE("exif init failed");
		return ret;
	}
/*
	if (init_in->io_cmd_ioctl) {
		cmd_param.camera_id = parm->camera_id;
		(*init_in->io_cmd_ioctl)(init_in->oem_handle, SETTING_IO_GET_CAPTURE_SIZE, &cmd_param);
		exif_unit->picture_size = cmd_param.size_param;

		(*init_in->io_cmd_ioctl)(init_in->oem_handle, SETTING_IO_GET_ACTUAL_CAPTURE_SIZE, &cmd_param);
		exif_unit->actual_picture_size = cmd_param.size_param;
	}
*/

	dealign_actual_pic_size = hal_param->capture_size;
	exif_unit->picture_size = dealign_actual_pic_size;
	exif_unit->actual_picture_size = dealign_actual_pic_size;

	time(&timep);
	p = localtime(&timep);
	sprintf(datetime_buf,
		"%4d:%02d:%02d %02d:%02d:%02d",
		(1900+p->tm_year),
		(1+p->tm_mon),
		p->tm_mday,
		p->tm_hour,
		p->tm_min,
		p->tm_sec);
	datetime_buf[19] = '\0';
	datetime = datetime_buf;

	CMR_LOGD("datetime %s",datetime);


	/*update gps info*/
	setting_update_gps_info(cpt, parm);

	focal_length_numerator = hal_param->hal_common.focal_length;
	focal_length_denominator = 1000;
	/* Some info is not get from the kernel */
	if (NULL != p_exif_info->spec_ptr) {
		p_exif_info->spec_ptr->basic.PixelXDimension = exif_unit->picture_size.width;
		p_exif_info->spec_ptr->basic.PixelYDimension = exif_unit->picture_size.height;
	}
	p_exif_info->primary.basic.ImageWidth = exif_unit->actual_picture_size.width;
	p_exif_info->primary.basic.ImageLength = exif_unit->actual_picture_size.height;
	CMR_LOGD("EXIF width=%d, height=%d \n",
			p_exif_info->primary.basic.ImageWidth,
			p_exif_info->primary.basic.ImageLength);

	if (NULL != p_exif_info->primary.data_struct_ptr) {
		p_exif_info->primary.data_struct_ptr->valid.Orientation = 1;
		p_exif_info->primary.data_struct_ptr->Orientation =
			setting_get_exif_orientation(hal_param->encode_rotation);
	}

	if (NULL != p_exif_info->primary.img_desc_ptr) {
		strcpy((char *)p_exif_info->primary.img_desc_ptr->ImageDescription, (char *)image_desc);
		memset(property,'\0',sizeof(property));
		property_get("ro.product.manufacturer", property, EXIF_DEF_MAKER);
		strcpy((char *)p_exif_info->primary.img_desc_ptr->Make, (char *)property);

		memset(property,'\0',sizeof(property));
		property_get("ro.product.model", property, EXIF_DEF_MODEL);
		strcpy((char *)p_exif_info->primary.img_desc_ptr->Model, (char *)property);
		strcpy((char *)p_exif_info->primary.img_desc_ptr->Copyright, (char *)copyright);
	}


	if ((NULL != p_exif_info->spec_ptr) && (NULL != p_exif_info->spec_ptr->pic_taking_cond_ptr)) {
		p_exif_info->spec_ptr->pic_taking_cond_ptr->valid.FocalLength = 1;
		p_exif_info->spec_ptr->pic_taking_cond_ptr->FocalLength.numerator = focal_length_numerator;
		p_exif_info->spec_ptr->pic_taking_cond_ptr->FocalLength.denominator = focal_length_denominator;
	}

	/* TODO: data time is get from user space now */
	if (NULL != p_exif_info->primary.img_desc_ptr) {
		CMR_LOGV("set DateTime.");
		strcpy((char *)p_exif_info->primary.img_desc_ptr->DateTime, (char *)datetime);
	}

	if (NULL != p_exif_info->spec_ptr) {
		if(NULL != p_exif_info->spec_ptr->other_ptr) {
			CMR_LOGV("set ImageUniqueID.");
			memset(p_exif_info->spec_ptr->other_ptr->ImageUniqueID, 0, sizeof(p_exif_info->spec_ptr->other_ptr->ImageUniqueID));
			sprintf((char *)p_exif_info->spec_ptr->other_ptr->ImageUniqueID,"IMAGE %s", datetime);
		}

		if(NULL != p_exif_info->spec_ptr->date_time_ptr) {
			strcpy((char *)p_exif_info->spec_ptr->date_time_ptr->DateTimeOriginal, (char *)datetime);
			strcpy((char *)p_exif_info->spec_ptr->date_time_ptr->DateTimeDigitized, (char *)datetime);
			CMR_LOGV("set DateTimeOriginal.");
		}
	}

	parm->exif_all_info_ptr = p_exif_info;

	return ret;
}

static cmr_int setting_set_video_mode(struct setting_component *cpt,
				                      struct setting_cmd_parameter *parm)
{
	cmr_int                      ret = 0;
	struct setting_local_param	 *local_param = get_local_param(cpt, parm->camera_id);
	struct setting_cmd_parameter setting;

	if (0 != parm->preview_fps_param.frame_rate) {
		local_param->is_dv_mode = 1;
	} else {
		local_param->is_dv_mode = 0;
	}

	ret = setting_set_general(cpt, SETTING_GENERAL_PREVIEW_FPS, parm);

	return ret;
}

static cmr_int setting_get_preview_mode(struct setting_component *cpt,
				                     struct setting_cmd_parameter *parm)
{
	cmr_int 					 ret = 0;
	struct setting_init_in		 *init_in = &cpt->init_in;
	struct setting_io_parameter  io_param = {0};

	if (init_in->io_cmd_ioctl) {
		ret = (*init_in->io_cmd_ioctl)(init_in->oem_handle, SETTING_IO_GET_PREVIEW_MODE, &io_param);
		parm->preview_fps_param.video_mode = io_param.cmd_value;
	}

	return ret;
}

static cmr_int setting_get_video_mode(struct setting_component *cpt,
				                      struct setting_cmd_parameter *parm)
{
	cmr_int                      ret = 0;
	cmr_u32                      i;
	cmr_u32                      sensor_mode;
	cmr_uint                     frame_rate;
	struct sensor_ae_info        *sensor_ae_info;
	struct setting_hal_param     *hal_param = get_hal_param(cpt, parm->camera_id);
	struct setting_local_param	 *local_param = get_local_param(cpt, parm->camera_id);

	setting_get_preview_mode(cpt, parm);
	frame_rate = hal_param->hal_common.frame_rate;
	sensor_mode = parm->preview_fps_param.video_mode;
	sensor_ae_info = (SENSOR_AE_INFO_T *)&local_param->sensor_static_info.video_info[sensor_mode].ae_info[0];

	parm->preview_fps_param.video_mode = 0;
	for (i = 0 ; i < SENSOR_VIDEO_MODE_MAX; i++) {
		if (frame_rate <= sensor_ae_info[i].max_frate) {
			parm->preview_fps_param.video_mode = i;
			break;
		}
	}
	if (SENSOR_VIDEO_MODE_MAX == i) {
		CMR_LOGD("use default video mode");
	}

	return ret;
}

static cmr_int setting_set_jpeg_quality(struct setting_component *cpt,
					                    struct setting_cmd_parameter *parm)
{
	cmr_int                    ret = 0;
	struct setting_hal_param   *hal_param = get_hal_param(cpt, parm->camera_id);

	hal_param->jpeg_quality = parm->cmd_type_value;
	return ret;
}

static cmr_int setting_get_jpeg_quality(struct setting_component *cpt,
					                    struct setting_cmd_parameter *parm)
{
	cmr_int                    ret = 0;
	struct setting_hal_param   *hal_param = get_hal_param(cpt, parm->camera_id);

	parm->cmd_type_value = hal_param->jpeg_quality;
	return ret;
}

static cmr_int setting_set_thumb_quality(struct setting_component *cpt,
					                     struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);

	hal_param->thumb_quality = parm->cmd_type_value;
	return ret;
}

static cmr_int setting_get_thumb_quality(struct setting_component *cpt,
                                        struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);

	parm->cmd_type_value = hal_param->thumb_quality;
	return ret;
}

static cmr_int setting_set_position(struct setting_component *cpt,
				                    struct setting_cmd_parameter *parm)
{
	cmr_int                       ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);

	hal_param->position_info = parm->position_info;
	return ret;
}

static cmr_int setting_get_preview_size(struct setting_component *cpt,
					                    struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);

	parm->size_param = hal_param->preview_size;
	return ret;
}

static cmr_int setting_set_preview_size(struct setting_component *cpt,
                                        struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);

	hal_param->preview_size = parm->size_param;
	return ret;
}

static cmr_int setting_get_preview_format(struct setting_component *cpt,
					                     struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);

	parm->cmd_type_value = hal_param->preview_format;
	return ret;
}

#ifdef CONFIG_SPRD_PRIVATE_ZSL
static cmr_int setting_get_sprd_zsl_enabled(struct setting_component *cpt,
					                     struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);

	parm->cmd_type_value = hal_param->sprd_zsl_enabled;
	return ret;
}
#endif

static cmr_int setting_set_ae_lock_unlock(struct setting_component *cpt,
								struct setting_cmd_parameter *parm)
{
	cmr_int ret = 0;

	ret = setting_set_general(cpt, SETTING_GENERAL_AE_LOCK_UNLOCK, parm);

	return ret;
}

static cmr_int setting_get_capture_format(struct setting_component *cpt,
					                     struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);

	parm->cmd_type_value = hal_param->capture_format;
	return ret;
}

static enum img_data_type get_image_format_from_param(cmr_uint param)
{
	enum img_data_type  fmt = IMG_DATA_TYPE_YUV420;

	switch (param) {
	case 0:
		fmt = IMG_DATA_TYPE_YUV422;
		break;
	case 1:
		fmt = IMG_DATA_TYPE_YUV420;
		break;
	case 3:
		fmt = IMG_DATA_TYPE_YUV420_3PLANE;
		break;
	case 4:
		fmt = IMG_DATA_TYPE_YVU420;
		break;
	case 5:
		fmt = IMG_DATA_TYPE_YV12;
		break;
	default:
		break;
	}
	return fmt;
}
static cmr_int setting_set_preview_format(struct setting_component *cpt,
					                      struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);

	hal_param->preview_format = get_image_format_from_param(parm->cmd_type_value);
	CMR_LOGD("format=%ld", hal_param->preview_format);
	return ret;
}

#ifdef CONFIG_SPRD_PRIVATE_ZSL
static cmr_int setting_set_sprd_zsl_enabled(struct setting_component *cpt,
					                      struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);

	hal_param->sprd_zsl_enabled = parm->cmd_type_value;
	CMR_LOGD("sprd_zsl_enabled=%ld", hal_param->sprd_zsl_enabled);
	return ret;
}
#endif

static cmr_int setting_set_capture_size(struct setting_component *cpt,
					                    struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);

	hal_param->capture_size = parm->size_param;
	return ret;
}

static cmr_int setting_set_capture_format(struct setting_component *cpt,
					                      struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);

	hal_param->capture_format = get_image_format_from_param(parm->cmd_type_value);

	CMR_LOGD("format=%ld", hal_param->capture_format);
	return ret;
}

static cmr_int setting_get_capture_size(struct setting_component *cpt,
                                        struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);

	parm->size_param = hal_param->capture_size;
	return ret;
}

static cmr_uint setting_convert_rotation_to_angle(struct setting_component *cpt,
                                           cmr_int camera_id, cmr_uint rotation)
{
	cmr_uint                    temp_angle = IMG_ANGLE_0;
	cmr_int                     camera_orientation[CAMERA_ID_MAX];
	struct setting_hal_param    *hal_param = get_hal_param(cpt, camera_id);


	cmr_bzero(camera_orientation, sizeof(camera_orientation));
#ifdef CONFIG_FRONT_CAMERA_ROTATION
	camera_orientation[CAMERA_ID_1] = 1;/*need to rotate*/
#endif


#ifdef CONFIG_BACK_CAMERA_ROTATION
	camera_orientation[CAMERA_ID_0] = 1;/*need to rotate*/
#endif


	if (camera_id >= CAMERA_ID_MAX) {
		CMR_LOGE("camera id not support");
		return temp_angle;
	}
	if ((0 == camera_orientation[camera_id])
		&& (0 == hal_param->sensor_orientation)) {
		return temp_angle;
	}
	if (camera_orientation[camera_id]) {
		switch (rotation) {
		case 0:
			temp_angle = IMG_ANGLE_90;
			break;

		case 90:
			temp_angle = IMG_ANGLE_180;
			break;

		case 180:
			temp_angle = IMG_ANGLE_270;
			break;

		case 270:
			temp_angle = 0;
			break;

		default:
			break;
		}
	} else {
		switch (rotation) {
		case 0:
			temp_angle = IMG_ANGLE_0;
			break;

		case 90:
			temp_angle = IMG_ANGLE_90;
			break;

		case 180:
			temp_angle = IMG_ANGLE_180;
			break;

		case 270:
			temp_angle = IMG_ANGLE_270;
			break;

		default:
			break;
		}
	}
	CMR_LOGD("angle=%ld", temp_angle);

	return temp_angle;
}

static cmr_int setting_set_capture_angle(struct setting_component *cpt,
					                    struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);
	cmr_uint                    angle = 0;


	angle = setting_convert_rotation_to_angle(cpt, parm->camera_id, parm->cmd_type_value);
	ret = setting_set_general(cpt, SETTING_GENERAL_SENSOR_ROTATION, parm);

	hal_param->preview_angle = angle;
	hal_param->capture_angle = angle;

	return ret;
}

static cmr_int setting_set_perfect_skinlevel(struct setting_component *cpt,
					                    struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);
	hal_param->perfect_skinlevel = parm->cmd_type_value;
	return ret;
}

static cmr_int setting_get_perfect_skinlevel(struct setting_component *cpt,
					                    struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);
	parm->cmd_type_value = hal_param->perfect_skinlevel;
	return ret;
}

static cmr_int setting_set_flip_on(struct setting_component *cpt,
                                                            struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);
	hal_param->flip_on = parm->cmd_type_value;
	CMR_LOGV("hal_param->flip_on = %d",hal_param->flip_on);
	return ret;
}

static cmr_int setting_get_flip_on(struct setting_component *cpt,
                                                            struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);
        parm->cmd_type_value = hal_param->flip_on;
	return ret;
}

static cmr_int setting_get_preview_angle(struct setting_component *cpt,
					                    struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);

	parm->cmd_type_value = hal_param->preview_angle;
	return ret;
}


static cmr_int setting_get_capture_angle(struct setting_component *cpt,
                                         struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);

	parm->cmd_type_value = hal_param->capture_angle;
	return ret;
}

static cmr_int setting_get_thumb_size(struct setting_component *cpt,
				                      struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);

	parm->size_param = hal_param->thumb_size;
	return ret;
}

static cmr_int setting_set_thumb_size(struct setting_component *cpt,
					                  struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);

	hal_param->thumb_size = parm->size_param;
	return ret;
}

static cmr_int setting_set_environment(struct setting_component *cpt,
					                   struct setting_cmd_parameter *parm)
{
	cmr_int                        ret = 0;
	struct setting_cmd_parameter   cmd_param;
	struct setting_hal_param       *hal_param = get_hal_param(cpt, parm->camera_id);
	cmr_uint                       invalid_word = 0;

	cpt->force_set = 1;

	memset(&invalid_word, INVALID_SETTING_BYTE, sizeof(cmr_uint));
	cmd_param = *parm;
	if (invalid_word != hal_param->hal_common.brightness) {
		cmd_param.cmd_type_value = hal_param->hal_common.brightness;
		ret = setting_set_brightness(cpt, &cmd_param);
		CMR_RTN_IF_ERR(ret);
	}

	if (invalid_word != hal_param->hal_common.contrast) {
		cmd_param.cmd_type_value = hal_param->hal_common.contrast;
		ret = setting_set_contrast(cpt,&cmd_param);
		CMR_RTN_IF_ERR(ret);
	}

	if (invalid_word != hal_param->hal_common.effect) {
		cmd_param.cmd_type_value = hal_param->hal_common.effect;
		ret = setting_set_effect(cpt, &cmd_param);
		CMR_RTN_IF_ERR(ret);
	}

	if (invalid_word != hal_param->hal_common.saturation) {
		cmd_param.cmd_type_value = hal_param->hal_common.saturation;
		ret = setting_set_saturation(cpt, &cmd_param);
		CMR_RTN_IF_ERR(ret);
	}

	if (invalid_word != hal_param->hal_common.exposure_compensation) {
		cmd_param.cmd_type_value = hal_param->hal_common.exposure_compensation;
		ret = setting_set_exposure_compensation(cpt, &cmd_param);
		CMR_RTN_IF_ERR(ret);
	}

	if (invalid_word != hal_param->hal_common.wb_mode) {
		cmd_param.cmd_type_value = hal_param->hal_common.wb_mode;
		ret = setting_set_wb(cpt, &cmd_param);
		CMR_RTN_IF_ERR(ret);
	}

	if (invalid_word != hal_param->hal_common.antibanding_mode) {
		cmd_param.cmd_type_value = hal_param->hal_common.antibanding_mode;
		ret = setting_set_antibanding(cpt, &cmd_param);
		CMR_RTN_IF_ERR(ret);
	}

	if (invalid_word != hal_param->hal_common.exposure_compensation) {
		cmd_param.cmd_type_value = hal_param->hal_common.exposure_compensation;
		ret = setting_set_exposure_compensation(cpt, &cmd_param);
		CMR_RTN_IF_ERR(ret);
	}

	if (invalid_word != hal_param->hal_common.auto_exposure_mode) {
		cmd_param.cmd_type_value = hal_param->hal_common.auto_exposure_mode;
		ret = setting_set_auto_exposure_mode(cpt, &cmd_param);
		CMR_RTN_IF_ERR(ret);
	}

	if (invalid_word != hal_param->hal_common.iso) {
		cmd_param.cmd_type_value = hal_param->hal_common.iso;
		ret = setting_set_iso(cpt, &cmd_param);
		CMR_RTN_IF_ERR(ret);
	}

	if (invalid_word != hal_param->hal_common.scene_mode) {
		cmd_param.cmd_type_value = hal_param->hal_common.scene_mode;
		ret = setting_set_scene_mode(cpt, &cmd_param);
		CMR_RTN_IF_ERR(ret);
	}

	if (invalid_word != hal_param->hal_common.frame_rate) {
		setting_get_video_mode(cpt, parm);
		hal_param->hal_common.video_mode = parm->preview_fps_param.video_mode;
		cmd_param.camera_id = parm->camera_id;
		cmd_param.preview_fps_param.frame_rate = hal_param->hal_common.frame_rate;
		cmd_param.preview_fps_param.video_mode = hal_param->hal_common.video_mode;
		ret = setting_set_video_mode(cpt, &cmd_param);
		CMR_RTN_IF_ERR(ret);
	}

	if (hal_param->is_update_range_fps) {
                if (setting_is_rawrgb_format(cpt, parm)) {
                        if (hal_param->range_fps.video_mode) {
                                struct setting_cmd_parameter isoParm = {0};

                                isoParm.camera_id = parm->camera_id;
                                isoParm.cmd_type_value = 5;
                                if (setting_is_rawrgb_format(cpt, &isoParm)) {
                                        ret = setting_isp_ctrl(cpt, COM_ISP_SET_ISO, &isoParm);
                                        if (ret) {
                                                CMR_LOGE("iso set failed %ld", ret);
                                        }
                                }
                        }
                        cmd_param.range_fps = hal_param->range_fps;
                        ret = setting_isp_ctrl(cpt, COM_ISP_SET_RANGE_FPS, &cmd_param);
                } else {
                        hal_param->hal_common.frame_rate = hal_param->range_fps.max_fps;
                        setting_get_video_mode(cpt, parm);
                        cmd_param.cmd_type_value = parm->preview_fps_param.video_mode;
                        setting_sn_ctrl(cpt, COM_SN_SET_VIDEO_MODE, &cmd_param);
			struct setting_local_param *local_param = get_local_param(cpt, parm->camera_id);
			CMR_LOGI("get dv mode %ld",local_param->is_dv_mode_for_yuv);
			if(local_param->is_dv_mode_for_yuv == 0)
			{
				parm->range_fps = hal_param->range_fps;
				setting_sn_ctrl(cpt, COM_SN_SET_YUV_FPS, parm);
			}
                }
        }


exit:
	cpt->force_set = 0;
	return ret;
}

static cmr_int setting_get_hdr(struct setting_component *cpt,
					           struct setting_cmd_parameter *parm)
{
	cmr_int                      ret = 0;
	struct setting_hal_param     *hal_param = get_hal_param(cpt, parm->camera_id);


	parm->cmd_type_value = hal_param->is_hdr;
	CMR_LOGI("get hdr %ld",parm->cmd_type_value);

	return ret;
}

static cmr_int setting_is_need_flash(struct setting_component *cpt,
					              struct setting_cmd_parameter *parm)
{
	cmr_int                     is_need = 0;
	enum takepicture_mode       capture_mode = 0;
	cmr_int                     shot_num = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);
	cmr_uint                    flash_status = 0;
	cmr_uint                    flash_mode = 0;

	capture_mode = (enum takepicture_mode)parm->ctrl_flash.capture_mode.capture_mode;
	flash_status = hal_param->flash_param.flash_status;
	flash_mode = hal_param->flash_param.flash_mode;
	shot_num = hal_param->shot_num;

	if (CAMERA_FLASH_MODE_TORCH != flash_mode && flash_status && ((CAMERA_NORMAL_MODE == capture_mode)
		|| (CAMERA_ISP_TUNING_MODE == capture_mode) || CAMERA_ZSL_MODE == capture_mode ||
		(CAMERA_ISP_SIMULATION_MODE == capture_mode) || (shot_num > 1))) {
		is_need = 1;
	}

	return is_need;
}

static cmr_int setting_get_flash_max_capacity(struct setting_component *cpt,
	                                struct setting_cmd_parameter *parm,
                                    uint16_t *max_time, uint16_t *max_charge)
{
	cmr_int                      ret = 0;
	struct setting_init_in       *init_in = &cpt->init_in;
	struct setting_io_parameter  io_param;

	if (init_in->io_cmd_ioctl) {
		io_param.camera_id = parm->camera_id;
		ret = (*init_in->io_cmd_ioctl)(init_in->oem_handle, SETTING_IO_GET_FLASH_MAX_CAPACITY, &io_param);
		if (0 == ret) {
			*max_time = io_param.flash_capacity.max_time;
			*max_charge = io_param.flash_capacity.max_charge;
		}
	}

	return ret;
}


static cmr_int setting_isp_flash_notify(struct setting_component *cpt,
                                           struct setting_cmd_parameter *parm,
                                           enum isp_flash_mode flash_mode)
{
	struct setting_init_in             *init_in = &cpt->init_in;
	struct common_isp_cmd_param        isp_param;
	cmr_int                            ret = 0;

	if (!setting_is_rawrgb_format(cpt, parm)) {
		return ret;
	}

	cmr_bzero(&isp_param, sizeof(isp_param));
	isp_param.camera_id = parm->camera_id;
	switch (flash_mode) {
	case ISP_FLASH_PRE_BEFORE: {
		uint16_t max_time = 0;
		uint16_t max_charge = 0;
		struct setting_local_param *local_param = get_local_param(cpt, parm->camera_id);

		setting_get_sensor_static_info(cpt, parm, &local_param->sensor_static_info);
		ret = setting_get_flash_max_capacity(cpt, parm, &max_time, &max_charge);
		isp_param.flash_notice.power.max_charge = max_charge;
		isp_param.flash_notice.power.max_time = max_time;
		isp_param.flash_notice.capture_skip_num = local_param->sensor_static_info.capture_skip_num;

		CMR_LOGI("max_time=%d, max_charge=%d", max_time, max_charge);
	}
		break;

	case ISP_FLASH_PRE_LIGHTING: {
			struct common_sn_cmd_param   sn_param;

			sn_param.camera_id = parm->camera_id;
			if (init_in->setting_sn_ioctl) {
				if ((*init_in->setting_sn_ioctl)(init_in->oem_handle, COM_SN_GET_FLASH_LEVEL, &sn_param)) {
					CMR_LOGE("get flash level error.");
				}
			}
			/*because hardware issue high equal to low, so use hight div high */
			isp_param.flash_notice.flash_ratio = sn_param.flash_level.high_light * 256 / sn_param.flash_level.low_light;
		}
		break;
	default:
		break;
	}

	isp_param.flash_notice.mode = flash_mode;
	isp_param.flash_notice.will_capture = parm->ctrl_flash.will_capture;
	if (init_in->setting_isp_ioctl) {
		ret = (*init_in->setting_isp_ioctl)(init_in->oem_handle, COM_ISP_SET_FLASH_NOTICE, &isp_param);
	}

	if (ret) {
		CMR_LOGE("setting flash error.");
	}

	return ret;
}

static cmr_int setting_set_flashdevice(struct setting_component *cpt,
	                                struct setting_cmd_parameter *parm,
                                    enum cmr_flash_status flash_status)
{
	cmr_int                      ret = 0;
	struct setting_init_in       *init_in = &cpt->init_in;
	struct setting_io_parameter  io_param;
	struct setting_flash_param   *flash_param = get_flash_param(cpt, parm->camera_id);

	CMR_LOGI("flash_status=%d", flash_status);
	if (init_in->io_cmd_ioctl) {
		io_param.camera_id = parm->camera_id;
		io_param.cmd_value = flash_status;
		ret = (*init_in->io_cmd_ioctl)(init_in->oem_handle, SETTING_IO_CTRL_FLASH, &io_param);
	}
	flash_param->flash_hw_status = flash_status;

	return ret;
}

static cmr_int setting_ctrl_flash(struct setting_component *cpt,
					              struct setting_cmd_parameter *parm)
{
	cmr_int                        ret = 0;
	cmr_uint                       is_active = 0;
	struct setting_hal_param       *hal_param = get_hal_param(cpt, parm->camera_id);
	struct setting_local_param     *local_param = get_local_param(cpt, parm->camera_id);
	cmr_uint                       flash_mode = 0;
	cmr_uint                       *p_auto_flash_status;
	cmr_uint                       flash_hw_status = 0;
	cmr_uint                       image_format = 0;
	struct setting_init_in         *init_in = &cpt->init_in;
	enum cmr_flash_status          ctrl_flash_status = 0;
	cmr_uint                       exif_flash = 0;
	cmr_uint                       is_to_isp = 0;
	cmr_uint                       work_mode = 0;
	cmr_int                        tmpVal = 0;


	is_active = parm->ctrl_flash.is_active;

	image_format = local_param->sensor_static_info.image_format;

	flash_mode = hal_param->flash_param.flash_mode;
	p_auto_flash_status = &hal_param->flash_param.auto_flash_status;
	flash_hw_status = hal_param->flash_param.flash_hw_status;

	ctrl_flash_status = parm->ctrl_flash.flash_type;
	CMR_LOGI("is_active %ld, flash_mode %ld, ctrl_flash_status %d flash_status %d",
		is_active, flash_mode, (cmr_u32)ctrl_flash_status, flash_hw_status);

	work_mode = parm->ctrl_flash.work_mode;
	if (is_active) {
		if (flash_hw_status == FLASH_OPEN) {
			cmr_setting_clear_sem(cpt);
			setting_isp_flash_notify(cpt, parm, ISP_FLASH_PRE_LIGHTING);
			setting_isp_wait_notice(cpt);
			goto EXIT;
		}
		if (((CAMERA_FLASH_MODE_AUTO == flash_mode) && (ctrl_flash_status == FLASH_OPEN))) {
			ret = setting_flash_handle(cpt, parm, flash_mode);
		}

		if (setting_is_need_flash(cpt, parm)) {
			if (IMG_DATA_TYPE_RAW == image_format) {
				switch (ctrl_flash_status) {
				case FLASH_HIGH_LIGHT: //high flash
					CMR_LOGI("high flash open");
					setting_isp_flash_notify(cpt, parm, ISP_FLASH_MAIN_BEFORE);
					setting_set_flashdevice(cpt, parm, ctrl_flash_status);

					setting_isp_flash_notify(cpt, parm, ISP_FLASH_MAIN_LIGHTING);
					hal_param->flash_param.has_preflashed = 0;
					break;
				case FLASH_AF_DONE:
					CMR_LOGI("pre flash AF DONE");
					setting_isp_flash_notify(cpt, parm, ISP_FLASH_AF_DONE);
					break;
				default:
					CMR_LOGI("pre flash open");
					cmr_setting_clear_sem(cpt);
					setting_isp_flash_notify(cpt, parm, ISP_FLASH_PRE_BEFORE);
					hal_param->flash_notice.mode = ISP_FLASH_PRE_BEFORE;
					setting_isp_wait_notice(cpt);

					if (FLASH_NEED_QUIT== cpt->flash_need_quit) {
						goto EXIT;
					}
					setting_set_flashdevice(cpt, parm, ctrl_flash_status);
					setting_isp_flash_notify(cpt, parm, ISP_FLASH_PRE_LIGHTING);
					hal_param->flash_notice.mode = ISP_FLASH_PRE_LIGHTING;
					setting_isp_wait_notice(cpt);
					hal_param->flash_param.has_preflashed = 1;
					break;
				}

			}else {
				setting_set_flashdevice(cpt, parm, ctrl_flash_status);
			}
		}

		if ((ctrl_flash_status == FLASH_HIGH_LIGHT) || (ctrl_flash_status == FLASH_OPEN)) {
			exif_flash = 1;
		} else {
			exif_flash = 0;
		}
		local_param->exif_flash = exif_flash;
	} else {
		struct sensor_raw_info      *raw_info_ptr = NULL;
		struct sensor_libuse_info   *libuse_info = NULL;
		cmr_int                     product_id = 0;

		/*for third ae*/
		setting_get_sensor_static_info(cpt, parm, &local_param->sensor_static_info);
		raw_info_ptr = local_param->sensor_static_info.raw_info_ptr;
		if (raw_info_ptr) {
			libuse_info = raw_info_ptr->libuse_info;
			if (libuse_info) {
				product_id = libuse_info->ae_lib_info.product_id;
			}
		}

		if (product_id) {
			if (FLASH_WAIT_TO_CLOSE == ctrl_flash_status) {
				CMR_LOGI("wait to close");
				setting_isp_wait_notice(cpt);
			}
		}

		/*disable*/
		if (setting_is_need_flash(cpt, parm)) {
			/*open flash*/
			if ((uint32_t)CAMERA_FLASH_MODE_TORCH != flash_mode) {
				setting_set_flashdevice(cpt, parm, FLASH_CLOSE_AFTER_OPEN);
				CMR_LOGI("flash close");
			}
			if (IMG_DATA_TYPE_RAW == image_format) {
				is_to_isp = 1;
			}
		}

		if (!parm->ctrl_flash.will_capture) {
			//hal_param->flash_param.has_preflashed = 0;
		}
		if (is_to_isp) {
			if (0 == work_mode) {
				setting_isp_flash_notify(cpt, parm, ISP_FLASH_PRE_AFTER);
			} else {
				if (product_id) {
					CMR_LOGI("wait to capture end");
					setting_isp_wait_notice(cpt);
				}
				setting_isp_flash_notify(cpt, parm, ISP_FLASH_MAIN_AFTER);
			}
		}

		if (product_id) {
			if (FLASH_WAIT_TO_CLOSE == ctrl_flash_status) {
				CMR_LOGI("wait to preflash period end");
				setting_isp_wait_notice(cpt);
			}
		}
	}

EXIT:
	return ret;
}

static cmr_int setting_get_capture_mode(struct setting_component *cpt,
					                    struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);

	parm->cmd_type_value = hal_param->capture_mode;
	return ret;
}

static cmr_int setting_set_capture_mode(struct setting_component *cpt,
					                    struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);

	hal_param->capture_mode = parm->cmd_type_value;
	return ret;
}

static cmr_int setting_get_rotation_capture(struct setting_component *cpt,
					                        struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);

	parm->cmd_type_value = hal_param->is_rotation_capture;
	return ret;
}

static cmr_int setting_set_rotation_capture(struct setting_component *cpt,
					                        struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);

	hal_param->is_rotation_capture = parm->cmd_type_value;
	return ret;
}

static cmr_int setting_set_android_zsl(struct setting_component *cpt,
					                  struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);

	hal_param->is_android_zsl = parm->cmd_type_value;
	return ret;
}

static cmr_int setting_get_android_zsl(struct setting_component *cpt,
					                  struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);

	parm->cmd_type_value = hal_param->is_android_zsl;
	return ret;
}

static cmr_int setting_set_video_size(struct setting_component *cpt,
					                  struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);

	hal_param->video_size = parm->size_param;
	return ret;
}

static cmr_int setting_get_video_size(struct setting_component *cpt,
					                  struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_hal_param    *hal_param = get_hal_param(cpt, parm->camera_id);

	parm->size_param = hal_param->video_size;
	return ret;
}

static cmr_int setting_get_dv_mode(struct setting_component *cpt,
					                  struct setting_cmd_parameter *parm)
{
	cmr_int                     ret = 0;
	struct setting_local_param    *local_param = get_local_param(cpt, parm->camera_id);

	parm->cmd_type_value = local_param->is_dv_mode;
	return ret;
}

static cmr_int setting_zoom_update_status(struct setting_component *cpt,
                                          enum zoom_status  status)
{
	cmr_int                     ret = 0;

	pthread_mutex_lock(&cpt->ctrl_lock);
	cpt->zoom_unit.status = status;
	if (ZOOM_IDLE == status) {
		cpt->zoom_unit.is_sended_msg = 0;
	}
	pthread_mutex_unlock(&cpt->ctrl_lock);

	return ret;
}

static cmr_int setting_is_zoom_pull(struct setting_component *cpt,
                                   struct setting_cmd_parameter *parm)
{
	cmr_int       is_changed = 0;


	pthread_mutex_lock(&cpt->ctrl_lock);
	is_changed = cpt->zoom_unit.is_changed;
	if (is_changed) {
		*parm = cpt->zoom_unit.in_zoom;
		cpt->zoom_unit.is_changed = 0;
	}
	pthread_mutex_unlock(&cpt->ctrl_lock);

	return is_changed;
}

static cmr_int setting_thread_proc(struct cmr_msg *message, void *data)
{
	cmr_int                     ret = 0;
	cmr_int                     evt;
	struct setting_component    *cpt = (struct setting_component *)data;

	if (!message || !data) {
		CMR_LOGE("param error");
		goto setting_proc_out;
	}

	CMR_LOGV("message.msg_type 0x%x", message->msg_type);
	evt = (message->msg_type & SETTING_EVT_MASK_BITS);

	switch (evt) {
	case SETTING_EVT_INIT:
		break;

	case SETTING_EVT_DEINIT:

		break;
	case SETTING_EVT_ZOOM:
	{
		struct setting_cmd_parameter  new_zoom_param;

		while (setting_is_zoom_pull(cpt, &new_zoom_param)) {
			setting_zoom_update_status(cpt, ZOOM_UPDATING);
			ret = setting_process_zoom(cpt, &new_zoom_param);
		}
		setting_zoom_update_status(cpt, ZOOM_IDLE);
	}
		break;
	default:
		CMR_LOGE("not correct message");
		break;
	}

setting_proc_out:
	CMR_LOGV("ret %ld", ret);
	return ret;
}

static cmr_int cmr_setting_clear_sem (struct setting_component *cpt)
{
	cmr_int tmpVal = 0;

	if (!cpt) {
		CMR_LOGE("camera_context is null.");
		return -1;
	}

	pthread_mutex_lock(&cpt->isp_mutex);
	sem_getvalue(&cpt->isp_sem, &tmpVal);
	while (0 < tmpVal && FLASH_OPEN==cpt->flash_need_quit) {
		sem_wait(&cpt->isp_sem);
		sem_getvalue(&cpt->isp_sem, &tmpVal);
	}

	sem_getvalue(&cpt->quick_ae_sem, &tmpVal);
	while (0 < tmpVal) {
		sem_wait(&cpt->quick_ae_sem);
		sem_getvalue(&cpt->quick_ae_sem, &tmpVal);
	}

	pthread_mutex_unlock(&cpt->isp_mutex);
	return 0;
}

static cmr_int setting_isp_wait_notice (struct setting_component *cpt)
{
	return setting_isp_wait_notice_withtime(cpt, ISP_ALG_TIMEOUT);
}

static cmr_int setting_isp_wait_notice_withtime (struct setting_component *cpt, cmr_uint timeout)
{
    cmr_int rtn = 0;
    struct timespec ts;

	if (!cpt) {
		CMR_LOGE("camera_context is null.");
	    return -1;
	}
	pthread_mutex_lock(&cpt->isp_mutex);
	if (clock_gettime(CLOCK_REALTIME, &ts)) {
		rtn = -1;
		CMR_LOGE("get time failed.");
	} else {
		ts.tv_sec += timeout;
		pthread_mutex_unlock(&cpt->isp_mutex);
		if (cmr_sem_timedwait((&cpt->isp_sem), &ts)) {
			pthread_mutex_lock(&cpt->isp_mutex);
			rtn = -1;
			cpt->isp_is_timeout = 1;
			CMR_LOGW("timeout.");
		} else {
			pthread_mutex_lock(&cpt->isp_mutex);
			cpt->isp_is_timeout = 0;
			CMR_LOGI("done.");
		}
	}
	pthread_mutex_unlock(&cpt->isp_mutex);
	return rtn;
}

cmr_int cmr_setting_isp_notice_done (cmr_handle setting_handle, void *data)
{
	struct setting_component *cpt = (struct setting_component *)setting_handle;
	UNUSED(data);

	if (!cpt) {
		CMR_LOGE("camera_context is null.");
		return -1;
	}

	pthread_mutex_lock(&cpt->isp_mutex);
	CMR_LOGI("isp notice done.");
	//if (0 == cpt->isp_is_timeout) {
		cmr_sem_post(&cpt->isp_sem);
	//} else {
	//	cpt->isp_is_timeout = 0;
	//}
	pthread_mutex_unlock(&cpt->isp_mutex);
	return 0;
}

static cmr_int setting_quick_ae_wait_notice (struct setting_component *cpt)
{
    cmr_int rtn = 0;
    struct timespec ts;
    cmr_int tmpVal = 0;

	if (!cpt) {
		CMR_LOGE("camera_context is null.");
	    return -1;
	}

	pthread_mutex_lock(&cpt->isp_mutex);
	if (clock_gettime(CLOCK_REALTIME, &ts)) {
		rtn = -1;
		CMR_LOGE("get time failed.");
	} else {
		ts.tv_sec += ISP_QUICK_AE_TIMEOUT;
		pthread_mutex_unlock(&cpt->isp_mutex);
		if (cmr_sem_timedwait((&cpt->quick_ae_sem), &ts)) {
			pthread_mutex_lock(&cpt->isp_mutex);
			rtn = -1;
			CMR_LOGW("timeout.");
		} else {
			pthread_mutex_lock(&cpt->isp_mutex);
			CMR_LOGI("done.");
		}
	}
	pthread_mutex_unlock(&cpt->isp_mutex);
	return rtn;
}

cmr_int cmr_setting_quick_ae_notice_done (cmr_handle setting_handle, void *data)
{
	struct setting_component *cpt = (struct setting_component *)setting_handle;
	UNUSED(data);

	if (!cpt) {
		CMR_LOGE("camera_context is null.");
		return -1;
	}

	pthread_mutex_lock(&cpt->isp_mutex);
	CMR_LOGI("isp quick ae done.");
	cmr_sem_post(&cpt->quick_ae_sem);
	pthread_mutex_unlock(&cpt->isp_mutex);
	return 0;
}

static cmr_int setting_set_pre_lowflash (struct setting_component *cpt,
					                  struct setting_cmd_parameter *parm)
{
	cmr_int                  ret = 0;
	struct setting_hal_param       *hal_param = get_hal_param(cpt, parm->camera_id);
	struct setting_local_param     *local_param = get_local_param(cpt, parm->camera_id);
	cmr_uint                       flash_mode = 0;
	cmr_uint                       image_format = 0;
	cmr_uint                       been_preflash = 0;
	struct setting_init_in         *init_in = &cpt->init_in;
//#ifdef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
	image_format = local_param->sensor_static_info.image_format;
	flash_mode = hal_param->flash_param.flash_mode;
	been_preflash = hal_param->flash_param.has_preflashed;
	CMR_LOGI("camera_preflash without AF, image_format %ld, flash_mode %ld, been_preflash %ld",
		image_format,
		flash_mode,
		been_preflash);

	if (!been_preflash) {
		if (CAMERA_FLASH_MODE_AUTO == flash_mode) {
			ret = setting_flash_handle(cpt, parm, flash_mode);
		}

		if (setting_is_need_flash(cpt, parm)) {
			CMR_LOGI("preflash low open");
			if (IMG_DATA_TYPE_RAW == image_format) {
				cmr_setting_clear_sem(cpt);
				setting_isp_flash_notify(cpt, parm, ISP_FLASH_PRE_BEFORE);
#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
				setting_isp_wait_notice_withtime(cpt, ISP_PREFLASH_ALG_TIMEOUT);

#endif
				setting_set_flashdevice(cpt, parm, (uint32_t)FLASH_OPEN);
				cmr_setting_clear_sem(cpt);
				setting_isp_flash_notify(cpt, parm, ISP_FLASH_PRE_LIGHTING);
				setting_isp_wait_notice_withtime(cpt, ISP_PREFLASH_ALG_TIMEOUT);
			} else {
				setting_set_flashdevice(cpt, parm, (uint32_t)FLASH_OPEN);
			}
		}

		if (setting_is_need_flash(cpt, parm)) {
			setting_set_flashdevice(cpt, parm, (uint32_t)FLASH_CLOSE_AFTER_OPEN);

			if (IMG_DATA_TYPE_RAW == image_format) {
				setting_isp_flash_notify(cpt, parm, ISP_FLASH_PRE_AFTER);
			}
			CMR_LOGI("preflash low close");
		}
	}
//#endif
	return ret;
}

static cmr_int setting_set_highflash_ae_measure(struct setting_component *cpt,
					                  struct setting_cmd_parameter *parm)
{
	cmr_int                      ret = 0;
	struct setting_hal_param       *hal_param = get_hal_param(cpt, parm->camera_id);
	struct setting_local_param     *local_param = get_local_param(cpt, parm->camera_id);
	cmr_uint                       flash_mode = 0;
	cmr_uint                       image_format = 0;
	struct setting_init_in         *init_in = &cpt->init_in;

	image_format = local_param->sensor_static_info.image_format;
	flash_mode = hal_param->flash_param.flash_mode;

	if (setting_is_need_flash(cpt, parm)) {
		/*open flash*/
		if (IMG_DATA_TYPE_RAW == image_format) {
			struct sensor_raw_info      *raw_info_ptr = NULL;
			struct sensor_libuse_info   *libuse_info = NULL;
			cmr_int                     product_id = 0;

			/*for third ae*/
			setting_get_sensor_static_info(cpt, parm, &local_param->sensor_static_info);
			raw_info_ptr = local_param->sensor_static_info.raw_info_ptr;
			if (raw_info_ptr) {
				libuse_info = raw_info_ptr->libuse_info;
				if (libuse_info) {
					product_id = libuse_info->ae_lib_info.product_id;
				}
			}

			if (!product_id) {
				setting_isp_flash_notify(cpt, parm, ISP_FLASH_MAIN_AE_MEASURE);
				setting_quick_ae_wait_notice(cpt);
			}
		}
	}

	return ret;
}

cmr_int cmr_setting_init(struct setting_init_in * param_ptr,
			              cmr_handle * out_setting_handle)
{
	cmr_int                    ret = 0;
	struct setting_component   *cpt = NULL;
	enum camera_index          i = 0;

	if (NULL == param_ptr) {
		return -CMR_CAMERA_INVALID_PARAM;
	}

	cpt = (struct setting_component *)malloc(sizeof(*cpt));
	if (!cpt) {
		CMR_LOGE("malloc failed");
		return -CMR_CAMERA_NO_MEM;
	}
	cmr_bzero(cpt, sizeof(*cpt));

	/*create thread */
	ret = cmr_thread_create(&cpt->thread_handle, SETTING_MSG_QUEUE_SIZE,
				            setting_thread_proc, (void *)cpt);

	if (CMR_MSG_SUCCESS != ret) {
		CMR_LOGE("create thread failed");
		ret = -CMR_CAMERA_NO_MEM;
		goto setting_out;
	}

	for (i = 0;i < CAMERA_ID_MAX ;++i) {
		memset(&cpt->camera_info[i].hal_param.hal_common,
				INVALID_SETTING_BYTE,
				sizeof(cpt->camera_info[i].hal_param.hal_common));
	}

	cpt->init_in = *param_ptr;

	pthread_mutex_init(&cpt->status_lock, NULL);
	pthread_mutex_init(&cpt->ctrl_lock, NULL);
	pthread_mutex_init(&cpt->isp_mutex, NULL);

	sem_init(&cpt->isp_sem, 0, 0);
	sem_init(&cpt->quick_ae_sem, 0, 0);

	*out_setting_handle = (cmr_handle) cpt;
	return 0;

setting_out:
	if (ret) {
		CMR_LOGE("error ret %ld", ret);
		if (cpt)
			free(cpt);
	}
	return ret;
}

cmr_int cmr_setting_deinit_notice(cmr_handle setting_handle){
	cmr_int                     ret = 0;
	struct setting_component    *cpt = (struct setting_component *)setting_handle;
	CMR_LOGI("cmr_setting_deinit_notice");

	pthread_mutex_lock(&cpt->isp_mutex);
	cpt->flash_need_quit = FLASH_NEED_QUIT;
	pthread_mutex_unlock(&cpt->isp_mutex);
	sem_post(&cpt->isp_sem); // fastly quit af process when flash on
	sem_post(&cpt->isp_sem); // fastly quit af process when flash on

	return ret;
}

cmr_int cmr_pre_flash_notice_flash(cmr_handle setting_handle){
	cmr_int                     ret = 0;
	struct setting_component    *cpt = (struct setting_component *)setting_handle;
	CMR_LOGI("cmr_pre_flash_notice_flash");

	pthread_mutex_lock(&cpt->isp_mutex);
	cpt->flash_need_quit = FLASH_OPEN;
	pthread_mutex_unlock(&cpt->isp_mutex);

	return ret;
}

cmr_int cmr_af_start_notice_flash(cmr_handle setting_handle){
	cmr_int                     ret = 0;
	struct setting_component    *cpt = (struct setting_component *)setting_handle;
	CMR_LOGI("cmr_af_start_notice_flash");

	pthread_mutex_lock(&cpt->isp_mutex);
	cpt->flash_need_quit = FLASH_OPEN;
	pthread_mutex_unlock(&cpt->isp_mutex);

	return ret;
}

cmr_int cmr_af_cancel_notice_flash(cmr_handle setting_handle){
	cmr_int                     ret = 0;
	struct setting_component    *cpt = (struct setting_component *)setting_handle;
	CMR_LOGI("cmr_af_cancel_notice_flash");

	pthread_mutex_lock(&cpt->isp_mutex);
	cpt->flash_need_quit = FLASH_NEED_QUIT;
	pthread_mutex_unlock(&cpt->isp_mutex);

	sem_post(&cpt->isp_sem); // fastly quit af process when flash on,--for safty quit post two times
	sem_post(&cpt->isp_sem);

	return ret;
}

cmr_int cmr_setting_deinit(cmr_handle setting_handle)
{
	cmr_int                     ret = 0;
	struct setting_component    *cpt = (struct setting_component *)setting_handle;
	CMR_MSG_INIT(message);

	if (NULL == cpt) {
		return -CMR_CAMERA_INVALID_PARAM;
	}

	if (cpt->thread_handle) {
		cmr_thread_destroy(cpt->thread_handle);
		cpt->thread_handle = 0;
	}

	sem_destroy(&cpt->isp_sem);
	sem_destroy(&cpt->quick_ae_sem);

	pthread_mutex_destroy(&cpt->status_lock);
	pthread_mutex_destroy(&cpt->ctrl_lock);
	pthread_mutex_destroy(&cpt->isp_mutex);


	free(cpt);
deinit_out:
	return ret;
}

cmr_int cmr_setting_ioctl(cmr_handle setting_handle, cmr_uint cmd_type,
			                struct setting_cmd_parameter * parm)
{
	cmr_int                        ret = 0;
	/*cmd type sequenced*/
	static struct setting_item     setting_list[] = {
		{CAMERA_PARAM_ZOOM ,                   setting_set_zoom_param},
		{CAMERA_PARAM_ENCODE_ROTATION,         setting_set_encode_angle},
		{CAMERA_PARAM_CONTRAST,                setting_set_contrast},
		{CAMERA_PARAM_BRIGHTNESS,              setting_set_brightness},
		{CAMERA_PARAM_SHARPNESS,               setting_set_sharpness},
		{CAMERA_PARAM_WB,                      setting_set_wb},
		{CAMERA_PARAM_EFFECT,                  setting_set_effect},
		{CAMERA_PARAM_FLASH,                   setting_set_flash_mode},
		{CAMERA_PARAM_ANTIBANDING,             setting_set_antibanding},
		{CAMERA_PARAM_FOCUS_RECT,              NULL}, /*by focus module*/
		{CAMERA_PARAM_AF_MODE,                 NULL}, /*by focus module*/
		{CAMERA_PARAM_AUTO_EXPOSURE_MODE,      setting_set_auto_exposure_mode},
		{CAMERA_PARAM_ISO,                     setting_set_iso},
		{CAMERA_PARAM_EXPOSURE_COMPENSATION,   setting_set_exposure_compensation},
		{CAMERA_PARAM_PREVIEW_FPS,             setting_set_preview_fps},
		{CAMERA_PARAM_PREVIEW_LLS_FPS,         setting_set_preview_lls_fps},
		{CAMERA_PARAM_SATURATION,              setting_set_saturation},
		{CAMERA_PARAM_SCENE_MODE,              setting_set_scene_mode},
		{CAMERA_PARAM_JPEG_QUALITY,            setting_set_jpeg_quality},
		{CAMERA_PARAM_THUMB_QUALITY,           setting_set_thumb_quality},
		{CAMERA_PARAM_SENSOR_ORIENTATION,      setting_set_sensor_orientation},
		{CAMERA_PARAM_FOCAL_LENGTH,            setting_set_focal_length},
		{CAMERA_PARAM_SENSOR_ROTATION,         setting_set_capture_angle},
		{CAMERA_PARAM_PERFECT_SKIN_LEVEL,      setting_set_perfect_skinlevel},
                {CAMERA_PARAM_FLIP_ON,                 setting_set_flip_on},
		{CAMERA_PARAM_SHOT_NUM,                setting_set_shot_num},
		{CAMERA_PARAM_ROTATION_CAPTURE,        setting_set_rotation_capture},
		{CAMERA_PARAM_POSITION,                setting_set_position},
		{CAMERA_PARAM_PREVIEW_SIZE,            setting_set_preview_size},
		{CAMERA_PARAM_PREVIEW_FORMAT,          setting_set_preview_format},
		{CAMERA_PARAM_CAPTURE_SIZE,            setting_set_capture_size},
		{CAMERA_PARAM_CAPTURE_FORMAT,          setting_set_capture_format},
		{CAMERA_PARAM_CAPTURE_MODE,            setting_set_capture_mode},
		{CAMERA_PARAM_THUMB_SIZE,              setting_set_thumb_size},
		{CAMERA_PARAM_ANDROID_ZSL,             setting_set_android_zsl},
		{CAMERA_PARAM_VIDEO_SIZE,              setting_set_video_size},
		{CAMERA_PARAM_RANGE_FPS,               setting_set_range_fps},
		{CAMERA_PARAM_ISP_FLASH,               setting_set_isp_flash_mode},
#ifdef CONFIG_SPRD_PRIVATE_ZSL
		{CAMERA_PARAM_SPRD_ZSL_ENABLED,   setting_set_sprd_zsl_enabled},
#endif
		{CAMERA_PARAM_ISP_AE_LOCK_UNLOCK, setting_set_ae_lock_unlock},
		{CAMERA_PARAM_TYPE_MAX,                NULL},
		{SETTING_GET_PREVIEW_ANGLE,            setting_get_preview_angle},
		{SETTING_GET_CAPTURE_ANGLE,            setting_get_capture_angle},
		{SETTING_GET_ZOOM_PARAM,               setting_get_zoom_param},
		{SETTING_GET_ENCODE_ANGLE,             setting_get_encode_angle},
		{SETTING_GET_EXIF_INFO,                setting_get_exif_info},
		{SETTING_GET_JPEG_QUALITY,             setting_get_jpeg_quality},
		{SETTING_GET_THUMB_QUALITY,            setting_get_thumb_quality},
		{SETTING_GET_THUMB_SIZE,               setting_get_thumb_size},
		{SETTING_GET_ROTATION_CAPTURE,         setting_get_rotation_capture},
		{SETTING_GET_SHOT_NUMBER,              setting_get_shot_num},
		{SETTING_SET_ENVIRONMENT,              setting_set_environment},
		{SETTING_GET_CAPTURE_SIZE,             setting_get_capture_size},
		{SETTING_GET_CAPTURE_FORMAT,           setting_get_capture_format},
		{SETTING_GET_PREVIEW_SIZE,             setting_get_preview_size},
		{SETTING_GET_PREVIEW_FORMAT,           setting_get_preview_format},
		{SETTING_GET_VIDEO_SIZE,               setting_get_video_size},
		{SETTING_GET_HDR,                      setting_get_hdr},
		{SETTING_GET_ANDROID_ZSL_FLAG,         setting_get_android_zsl},
		{SETTING_CTRL_FLASH,                   setting_ctrl_flash},
		{SETTING_GET_CAPTURE_MODE,             setting_get_capture_mode},
		{SETTING_GET_DV_MODE,                  setting_get_dv_mode},
		{SETTING_SET_PRE_LOWFLASH,             setting_set_pre_lowflash},
		{SETTING_GET_FLASH_STATUS,             setting_get_flash_status},
		{SETTING_SET_HIGHFLASH_AE_MEASURE,            setting_set_highflash_ae_measure},
		{SETTING_GET_HW_FLASH_STATUS,             setting_get_HW_flash_status},
		{SETTING_GET_PERFECT_SKINLEVEL,           setting_get_perfect_skinlevel},
                {SETTING_GET_FLIP_ON,                     setting_get_flip_on},
#ifdef CONFIG_SPRD_PRIVATE_ZSL
		{SETTING_GET_SPRD_ZSL_ENABLED,            setting_get_sprd_zsl_enabled},
#endif
	};
	struct setting_item          *item = NULL;
	struct setting_component     *cpt =	 (struct setting_component *)setting_handle;


	if (!cpt || !parm || cmd_type >= SETTING_TYPE_MAX
		|| SETTING_TYPE_MAX != cmr_array_size(setting_list)
		|| parm->camera_id >= CAMERA_ID_MAX) {

		CMR_LOGE("param has error cpt 0x%p, camera_id %ld, array_size %d, cmd_type %ld",
			cpt, parm->camera_id, cmr_array_size(setting_list), cmd_type);
		return -CMR_CAMERA_INVALID_PARAM;
	}

	item = &setting_list[cmd_type];
	if (item && item->setting_ioctl) {
		ret = (*item->setting_ioctl)(cpt, parm);
	} else {
		CMR_LOGW("ioctl is NULL  %ld", cmd_type);
	}
	return ret;
}

int camera_set_flashdevice(uint32_t param)
{
	cmr_int ret = 0;
	UNUSED(param);

	return ret;
}
