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
#ifndef ANDROID_HARDWARE_SPRD_OEM_CAMERA_H
#define ANDROID_HARDWARE_SPRD_OEM_CAMERA_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <sys/types.h>
#include "cmr_type.h"

#define FACE_DETECT_NUM                              10


/* White balancing type, used for CAMERA_PARM_WHITE_BALANCING */
enum {
	CAMERA_WB_AUTO = 0,
	CAMERA_WB_INCANDESCENT,
	CAMERA_WB_FLUORESCENT = 4, //id 2 and 3 not used
	CAMERA_WB_DAYLIGHT,
	CAMERA_WB_CLOUDY_DAYLIGHT,
	CAMERA_WB_MAX
};

enum {
	CAMERA_SCENE_MODE_AUTO = 0,
	CAMERA_SCENE_MODE_NIGHT,
	CAMERA_SCENE_MODE_ACTION, //not support
	CAMERA_SCENE_MODE_PORTRAIT, //not support
	CAMERA_SCENE_MODE_LANDSCAPE, //not support
	CAMERA_SCENE_MODE_NORMAL,
	CAMERA_SCENE_MODE_HDR,
	CAMERA_SCENE_MODE_MAX
};


enum {
	CAMERA_AE_FRAME_AVG = 0,
	CAMERA_AE_CENTER_WEIGHTED,
	CAMERA_AE_SPOT_METERING,
	CAMERA_AE_MODE_MAX
};

enum camera_format_type {
	CAMERA_YCBCR_4_2_0,
	CAMERA_YCBCR_4_2_2,
	CAMERA_FORMAT_TYPE_MAX
};

enum cmr_flash_mode{
	CAMERA_FLASH_MODE_OFF   = 0,
	CAMERA_FLASH_MODE_ON    = 1,
	CAMERA_FLASH_MODE_TORCH = 2,
	CAMERA_FLASH_MODE_AUTO  = 3,
	CAMERA_FLASH_MODE_MAX
};

enum cmr_focus_mode {
	CAMERA_FOCUS_MODE_AUTO       = 0,
	CAMERA_FOCUS_MODE_AUTO_MULTI = 1,
	CAMERA_FOCUS_MODE_MACRO      = 2,
	CAMERA_FOCUS_MODE_INFINITY   = 3,
	CAMERA_FOCUS_MODE_CAF        = 4,
	CAMERA_FOCUS_MODE_CAF_VIDEO  = 5,
	CAMERA_FOCUS_MODE_MAX
};

enum camera_cb_type {
	CAMERA_RSP_CB_SUCCESS,
	CAMERA_EXIT_CB_DONE,
	CAMERA_EXIT_CB_FAILED,
	CAMERA_EXIT_CB_ABORT,
	CAMERA_EVT_CB_FRAME,
	CAMERA_EVT_CB_CAPTURE_FRAME_DONE,
	CAMERA_EVT_CB_SNAPSHOT_DONE,
	CAMERA_EVT_CB_FD,
	CAMERA_EVT_CB_FOCUS_MOVE,
	CAMERA_EVT_CB_FLUSH,
	CAMERA_EVT_CB_ZSL_FRM,
	CAMERA_EXIT_CB_PREPARE,   /* prepared to be executed      */
	CAMERA_EVT_CB_SNAPSHOT_JPEG_DONE,
	CAMERA_EVT_CB_RESUME,
	CAMERA_EVT_CB_AE_STAB_NOTIFY,
	CAMERA_EVT_CB_AE_LOCK_NOTIFY,
	CAMERA_EVT_CB_AE_UNLOCK_NOTIFY,
	CAMERA_CB_TYPE_MAX
};

enum camera_func_type {
	CAMERA_FUNC_START,
	CAMERA_FUNC_STOP,
	CAMERA_FUNC_START_PREVIEW,
	CAMERA_FUNC_TAKE_PICTURE,
	CAMERA_FUNC_ENCODE_PICTURE,
	CAMERA_FUNC_START_FOCUS,/*5*/
	CAMERA_FUNC_STOP_PREVIEW,
	CAMERA_FUNC_RELEASE_PICTURE,
	CAMERA_FUNC_AE_STATE_CALLBACK,
	CAMERA_FUNC_TYPE_MAX
};

enum takepicture_mode {
	CAMERA_NORMAL_MODE = 0,
	CAMERA_ZSL_MODE,
	CAMERA_ISP_TUNING_MODE,
	CAMERA_LLS_MUTLI_FRAME_MODE,
	CAMERA_UTEST_MODE,
	CAMERA_AUTOTEST_MODE,
	CAMERA_ISP_SIMULATION_MODE,
	TAKEPICTURE_MODE_MAX
};

enum preview_buffer_usage_mode {
	PREVIEW_BUFFER_USAGE_DCAM = 0,
	PREVIEW_BUFFER_USAGE_GRAPHICS,
	PREVIEW_BUFFER_USAGE_MODE_MAX,
};

enum camera_mem_cb_type {
	CAMERA_PREVIEW = 0,
	CAMERA_SNAPSHOT,
	CAMERA_SNAPSHOT_ZSL,
	CAMERA_VIDEO,
	CAMERA_PREVIEW_RESERVED,
	CAMERA_SNAPSHOT_ZSL_RESERVED_MEM,
	CAMERA_VIDEO_RESERVED,
	CAMERA_ISP_LSC,
	CAMERA_ISP_BINGING4AWB,
	CAMERA_SNAPSHOT_PATH,
	CAMERA_ISP_ANTI_FLICKER,
	CAMERA_ISP_RAWAE,
	CAMERA_MEM_CB_TYPE_MAX
};

enum camera_param_type{
	CAMERA_PARAM_ZOOM = 0,
	CAMERA_PARAM_ENCODE_ROTATION,
	CAMERA_PARAM_CONTRAST,
	CAMERA_PARAM_BRIGHTNESS,
	CAMERA_PARAM_SHARPNESS,
	CAMERA_PARAM_WB,
	CAMERA_PARAM_EFFECT,
	CAMERA_PARAM_FLASH,
	CAMERA_PARAM_ANTIBANDING,
	CAMERA_PARAM_FOCUS_RECT,
	CAMERA_PARAM_AF_MODE,/*10*/
	CAMERA_PARAM_AUTO_EXPOSURE_MODE,
	CAMERA_PARAM_ISO,
	CAMERA_PARAM_EXPOSURE_COMPENSATION,
	CAMERA_PARAM_PREVIEW_FPS,
	CAMERA_PARAM_PREVIEW_LLS_FPS,
	CAMERA_PARAM_SATURATION,
	CAMERA_PARAM_SCENE_MODE,
	CAMERA_PARAM_JPEG_QUALITY,
	CAMERA_PARAM_THUMB_QUALITY,
	CAMERA_PARAM_SENSOR_ORIENTATION,
	CAMERA_PARAM_FOCAL_LENGTH,
	CAMERA_PARAM_SENSOR_ROTATION,
	CAMERA_PARAM_PERFECT_SKIN_LEVEL,
	CAMERA_PARAM_FLIP_ON,
	CAMERA_PARAM_SHOT_NUM,
	CAMERA_PARAM_ROTATION_CAPTURE,
	CAMERA_PARAM_POSITION,
	CAMERA_PARAM_PREVIEW_SIZE,
	CAMERA_PARAM_PREVIEW_FORMAT,
	CAMERA_PARAM_CAPTURE_SIZE,
	CAMERA_PARAM_CAPTURE_FORMAT,
	CAMERA_PARAM_CAPTURE_MODE,
	CAMERA_PARAM_THUMB_SIZE,
	CAMERA_PARAM_ANDROID_ZSL,
	CAMERA_PARAM_VIDEO_SIZE,
	CAMERA_PARAM_RANGE_FPS,
	CAMERA_PARAM_ISP_FLASH,
//	CAMERA_PARAM_ISP_AE_LOCK_UNLOCK,
#ifdef CONFIG_SPRD_PRIVATE_ZSL
	CAMERA_PARAM_SPRD_ZSL_ENABLED,
#endif
	CAMERA_PARAM_TYPE_MAX
};

enum camera_data {
	CAMERA_PREVIEW_DATA = 0,
	CAMERA_SNAPSHOT_DATA,
	CAMERA_VIDEO_DATA,
	CAMERA_DATA_MAX
};

enum camera_preview_mode_type {
	CAMERA_PREVIEW_MODE_SNAPSHOT = 0,
	CAMERA_PREVIEW_MODE_MOVIE,
	CAMERA_PREVIEW_MODE_SLOWMOTION,
	CAMERA_MAX_PREVIEW_MODE
};

enum fast_ctrl_mode {
	CAMERA_FAST_MODE_FD = 0,
	CAMERA_FAST_MODE_MAX
};


struct camera_face_info {
	cmr_u32                   face_id;
	cmr_u32                   sx;
	cmr_u32                   sy;
	cmr_u32                   srx;
	cmr_u32                   sry;
	cmr_u32                   ex;
	cmr_u32                   ey;
	cmr_u32                   elx;
	cmr_u32                   ely;
	cmr_u32                   brightness;
	cmr_u32                   angle;
	cmr_u32                   smile_level;
	cmr_u32                   blink_level;
	cmr_u32                   padding;
};

struct camera_jpeg_param {
	void                      *outPtr;
	cmr_u32                   size;
	cmr_u32                   need_free;
	cmr_u32                   index;
	cmr_u32                   reserved;
};

struct camera_sensor_info {
	cmr_s32		              exposure_time_numerator;
	cmr_s32		              exposure_time_denominator;
};

struct camera_frame_type {
	cmr_u32                   format;
	cmr_u32                   width;
	cmr_u32                   height;
	cmr_u32                   buf_id;
	cmr_u32                   order_buf_id;
	cmr_u32                   face_num;
	cmr_uint                  y_phy_addr;
	cmr_uint                  y_vir_addr;
	cmr_uint                  uv_phy_addr;
	cmr_uint                  uv_vir_addr;
	cmr_s64                   timestamp;
	cmr_int                   status;
	cmr_int                   type;
	cmr_int                   lls_info;
	cmr_u32                   need_free;
	struct camera_face_info   face_info[FACE_DETECT_NUM];
	struct camera_jpeg_param  jpeg_param;
	struct camera_sensor_info sensor_info;
	cmr_uint                  zsl_private;
};
/*
struct camera_cap_frm_info {
	cmr_uint                 y_virt_addr;
	cmr_uint                 u_virt_addr;
	cmr_u32                  width;
	cmr_u32                  height;
	cmr_s64                  timestamp;
	struct frm_info          frame_info;
};*/

struct camera_position_type {
	long                      timestamp;
	double                    latitude;
	double                    longitude;
	double                    altitude;
	const char                *process_method;
};

struct cmr_focus_param {
	cmr_u32                   zone_cnt;
	struct img_rect           zone[FOCUS_ZONE_CNT_MAX];
};


typedef void (*camera_cb_of_type)(enum camera_cb_type cb, const void *client_data, enum camera_func_type func, void  *parm4);

typedef cmr_int (*camera_cb_of_malloc)(enum camera_mem_cb_type type, cmr_u32 *size_ptr, cmr_u32 *sum_ptr, cmr_uint *phy_addr,
	                                    cmr_uint *vir_addr, void* private_data);
typedef cmr_int (*camera_cb_of_free)(enum camera_mem_cb_type type, cmr_uint *phy_addr, cmr_uint *vir_addr, cmr_u32 sum, void* private_data);

cmr_int camera_init(cmr_u32 camera_id, camera_cb_of_type callback, void *client_data,  cmr_uint is_autotest, cmr_handle *camera_handle);

cmr_int camera_init_with_mem_func(cmr_u32 camera_id, camera_cb_of_type callback, void *client_data
		,  cmr_uint is_autotest, cmr_handle *camera_handle, void* cb_of_malloc, void* cb_of_free);

cmr_int camera_deinit(cmr_handle camera_handle);

cmr_int camera_release_frame(cmr_handle camera_handle, enum camera_data data, cmr_uint index);

cmr_int camera_set_param(cmr_handle camera_handle, enum camera_param_type id, cmr_uint param);

cmr_int camera_start_preview(cmr_handle camera_handle, enum takepicture_mode mode);

cmr_int camera_stop_preview(cmr_handle camera_handle);

cmr_int camera_start_autofocus(cmr_handle camera_handle);

cmr_int camera_cancel_autofocus(cmr_handle camera_handle);

cmr_int camera_cancel_takepicture(cmr_handle camera_handle);

uint32_t camera_safe_scale_th(void);

cmr_int camera_take_picture(cmr_handle camera_handle, enum takepicture_mode cap_mode);

cmr_int camera_get_sn_trim(cmr_handle camera_handle, cmr_u32 mode, cmr_uint *trim_x, cmr_uint *trim_y,
	                                   cmr_uint *trim_w, cmr_uint *trim_h, cmr_uint *width, cmr_uint *height);

cmr_int camera_set_mem_func(cmr_handle camera_handle, void* cb_of_malloc, void* cb_of_free, void* private_data);

cmr_int camera_get_redisplay_data(cmr_handle camera_handle, cmr_uint output_addr, cmr_uint output_width, cmr_uint output_height,
									             cmr_uint input_addr_y, cmr_uint input_addr_uv, cmr_uint input_width, cmr_uint input_height);

cmr_int camera_is_change_size(cmr_handle camera_handle, cmr_u32 cap_width,
	                                        cmr_u32 cap_height, cmr_u32 preview_width,
	                                        cmr_u32 preview_height, cmr_u32 video_width,
	                                        cmr_u32 video_height, cmr_uint *is_change);

int camera_pre_capture_get_buffer_id(cmr_u32 camera_id);

int camera_pre_capture_get_buffer_size(cmr_u32 camera_id,
						cmr_s32 mem_size_id,
						cmr_u32 *mem_size,
						cmr_u32 *mem_sum);

cmr_int camera_get_preview_rect(cmr_handle camera_handle, cmr_uint *rect_x, cmr_uint *rect_y, cmr_uint *rect_width, cmr_uint *rect_height);

cmr_int camera_get_zsl_capability(cmr_handle camera_handle, cmr_uint *is_support, cmr_uint *max_widht, cmr_uint *max_height);

cmr_int camera_get_sensor_trim(cmr_handle camera_handle, struct img_rect *sn_trim);

cmr_uint camera_get_preview_rot_angle(cmr_handle camera_handle);

void camera_fd_enable(cmr_handle camera_handle, cmr_u32 is_enable);
void camera_flip_enable(cmr_handle camera_handle, cmr_u32 param);

void camera_fd_start(cmr_handle camera_handle, cmr_u32 param);

cmr_int camera_is_need_stop_preview(cmr_handle camera_handle);

cmr_int camera_takepicture_process(cmr_handle camera_handle, cmr_uint src_phy_addr, cmr_uint src_vir_addr, cmr_u32 width, cmr_u32 height);

uint32_t camera_get_size_align_page(uint32_t size);

cmr_int camera_fast_ctrl(cmr_handle camera_handle, enum fast_ctrl_mode mode, cmr_u32 param);

cmr_int camera_start_preflash(cmr_handle camera_handle);

cmr_int camera_get_viewangle(cmr_handle camera_handle, struct sensor_view_angle *view_angle);

cmr_uint camera_get_sensor_exif_info(cmr_handle camera_handle, struct exif_info *exif_info);
cmr_int camera_set_preview_buffer(cmr_handle camera_handle, cmr_uint src_phy_addr, cmr_uint src_vir_addr);
cmr_int camera_set_video_buffer(cmr_handle camera_handle, cmr_uint src_phy_addr, cmr_uint src_vir_addr);
cmr_int camera_set_zsl_buffer(cmr_handle camera_handle, cmr_uint src_phy_addr, cmr_uint src_vir_addr, cmr_uint zsl_private);
cmr_int camera_set_video_snapshot_buffer(cmr_handle camera_handle, cmr_uint src_phy_addr, cmr_uint src_vir_addr);
cmr_int camera_set_zsl_snapshot_buffer(cmr_handle camera_handle, cmr_uint src_phy_addr, cmr_uint src_vir_addr);
cmr_int camera_zsl_snapshot_need_pause(cmr_handle camera_handle, cmr_int *flag);
cmr_int camera_get_isp_handle(cmr_handle camera_handle, cmr_handle *isp_handle);
void camera_lls_enable(cmr_handle camera_handle, cmr_u32 is_enable);
cmr_int camera_is_lls_enabled(cmr_handle camera_handle);
void camera_vendor_hdr_enable(cmr_handle camera_handle,cmr_u32 is_enable);
cmr_int camera_is_vendor_hdr(cmr_handle camera_handle);

void camera_set_lls_shot_mode(cmr_handle camera_handle, cmr_u32 is_enable);
cmr_int camera_get_lls_shot_mode(cmr_handle camera_handle);
cmr_int camera_get_isp_info(cmr_handle camera_handle, void **addr, int *size);

void camera_start_burst_notice(cmr_handle camera_handle);
void camera_end_burst_notice(cmr_handle camera_handle);

cmr_int camera_transfer_caf_to_af(cmr_handle camera_handle);
cmr_int camera_transfer_af_to_caf(cmr_handle camera_handle);

cmr_int camera_get_gain_thrs(cmr_handle camera_handle, cmr_u32 *is_over_thrs);
#ifdef __cplusplus
}
#endif

#endif //ANDROID_HARDWARE_SPRD_OEM_CAMERA_H

