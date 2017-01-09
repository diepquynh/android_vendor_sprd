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

int camera_get_reserve_buffer_size(cmr_u32 camera_id,
						cmr_s32 mem_size_id,
						cmr_u32 *mem_size,
						cmr_u32 *mem_sum);

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

#ifdef CONFIG_MEM_OPTIMIZATION
cmr_int camera_start_scaling(cmr_handle camera_handle, cmr_handle caller_handle, struct img_frm *src, struct img_frm *dst, struct cmr_op_mean *mean);
cmr_int camera_start_scaling_in_gsp(struct img_frm *src, struct img_frm *dst, int srd_fd, int dst_fd);
cmr_int camera_notify_closing_gsp_hwc(cmr_int need_close);
#endif

cmr_int camera_get_gain_thrs(cmr_handle camera_handle, cmr_u32 *is_over_thrs);
cmr_int camera_set_sensor_close_flag(cmr_handle camera_handle);
#ifdef __cplusplus
}
#endif

#endif //ANDROID_HARDWARE_SPRD_OEM_CAMERA_H

