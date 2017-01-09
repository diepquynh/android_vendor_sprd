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
#ifndef _CMR_OEM_H_
#define _CMR_OEM_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "cmr_common.h"
#include "cmr_mem.h"
#include "cmr_msg.h"
#include "cmr_grab.h"
#include "cmr_sensor.h"
#include "isp_app.h"
#include "cmr_cvt.h"
#include "jpeg_codec.h"
#include "jpeg_exif_header.h"
#include "SprdOEMCamera.h"
#include "cmr_snapshot.h"
#include "cmr_setting.h"
#include "cmr_focus.h"
#include "cmr_preview.h"
#include "cmr_ipm.h"


#define CAMERA_PREVIEW                               0
#define CAMERA_SNAPSHOT                              1
#define CAMERA_SNAPSHOT_ZSL                          2
#define CAMERA_VIDEO                                 3
#define CAMERA_PREVIEW_RESERVED                      4
#define CAMERA_SNAPSHOT_ZSL_RESERVED                 5
#define CAMERA_VIDEO_RESERVED                        6
#define CAMERA_ISP_LSC                               7
#define CAMERA_ISP_BINGING4AWB                       8
#define CAMERA_SNAPSHOT_PATH                         9

#define ISP_LSC_BUF_SIZE                             (32 * 1024)
#define ISP_LSC_BUF_NUM                              1
#define ISP_B4AWB_BUF_CNT                            2
#define ISP_B4AWB_BUF_SIZE                           640 * 480 * 2
#define CAMERA_FRAME_SKIP_NUM_AFTER_FLASH            2

struct grab_context {
	cmr_handle               grab_handle;
/*	struct process_status    proc_status;*/
	cmr_handle               caller_handle[GRAB_CHANNEL_MAX];
	cmr_u32                  skip_number[GRAB_CHANNEL_MAX];
	cmr_u32                  inited;
};

struct sensor_context {
	cmr_handle                  sensor_handle;
	cmr_u32                     cur_id;
	cmr_u32                     inited;
	struct sensor_if            sn_if;
	struct sensor_exp_info      sensor_info;
	EXIF_SPEC_PIC_TAKING_COND_T exif_info;
};

struct isp_context {
    cmr_handle               isp_handle;
	cmr_handle               caller_handle;
    cmr_u32                  isp_state; // 0 for preview, 1 for post process;
    cmr_u32                  inited;
    cmr_u32                  width_limit;
    cmr_u32                  is_work;
};

struct jpeg_context {
    cmr_handle               jpeg_handle;
    cmr_u32                  jpeg_state;
    cmr_u32                  inited;
    cmr_handle               enc_caller_handle;
    cmr_handle               dec_caller_handle;
	struct jpeg_param        param;
};

struct scaler_context {
    cmr_handle               scaler_handle;
    cmr_u32                  scale_state;
    cmr_u32                  inited;
	cmr_handle               caller_handle;
};

struct rotation_context {
    cmr_handle               rotation_handle;
    cmr_u32                  rot_state;
    cmr_u32                  inited;
    cmr_handle               caller_handle;
};

struct ipm_context {
	cmr_handle               ipm_handle;
	cmr_handle               hdr_handle;
	cmr_handle               uvde_handle;
	cmr_handle               yde_handle;
	cmr_handle               snr_uvde_handle;
	cmr_u32                  inited;
	cmr_u32                  frm_num;
	cmr_u32                  hdr_num;
	cmr_u32                  padding;
};

struct preview_context {
	cmr_handle               preview_handle;
	cmr_u32                  inited;
	cmr_u32                  preview_sn_mode;
	cmr_u32                  video_sn_mode;
	cmr_u32                  skip_num;
	cmr_u32                  channel_bits;
	cmr_u32                  video_channel_bits;
	cmr_uint                 status;
	struct img_size          size;
	struct img_size          video_size;
	struct img_size          actual_video_size;
	struct frm_info          video_cur_chn_data;
	struct img_rect          rect;
	struct img_data_end      data_endian;
	struct img_data_end      video_data_endian;
};

struct snapshot_context {
	cmr_handle               snapshot_handle;
	cmr_u32                  inited;
	cmr_u32                  snapshot_sn_mode;
	cmr_u32                  skip_num;
	cmr_u32                  channel_bits;
	cmr_u32                  is_hdr;
	cmr_u32                  total_num;
	cmr_u32                  snp_mode;
	cmr_u32                  is_cfg_rot_cap;
	cmr_u32                  cfg_cap_rot;
	cmr_u32                  status;
	cmr_u32                  zsl_frame;
	cmr_uint                 is_req_snp;
	cmr_s64                  cap_time_stamp;
	struct img_size          request_size;
	struct img_size          capture_align_size;
	struct img_size          actual_capture_size;
	struct frm_info          cur_frm_info;
	struct snp_proc_param    post_proc_setting;
	struct img_data_end      data_endian;
	struct frm_info          cur_chn_data;
};

struct focus_context {
    cmr_handle               focus_handle;
    cmr_u32                  inited;
    cmr_u32                  padding;
};

struct setting_context {
    cmr_handle               setting_handle;
    cmr_u32                  inited;
    cmr_u32                  is_active;
    cmr_u32                  is_auto_iso;
    cmr_uint                 iso_value;
};

struct camera_settings {
    cmr_u32                  preview_width;
    cmr_u32                  preview_height;
    cmr_u32                  snapshot_width;
    cmr_u32                  snapshot_height;
    cmr_u32                  focal_len;
    cmr_u32                  brightness;
    cmr_u32                  contrast;
    cmr_u32                  effect;
    cmr_u32                  expo_compen;
    cmr_u32                  wb_mode;
    cmr_u32                  saturation;
    cmr_u32                  sharpness;
    cmr_u32                  scene_mode;
    cmr_u32                  flash;
    cmr_u32                  auto_flash_status;
    cmr_u32                  night_mode;
    cmr_u32                  flicker_mode;
    cmr_u32                  focus_rect;
    cmr_u32                  af_mode;
    cmr_u32                  iso;
    cmr_u32                  luma_adapt;
    cmr_u32                  video_mode;
    cmr_u32                  frame_rate;
    cmr_u32                  sensor_mode;
    cmr_u32                  auto_exposure_mode;
    cmr_u32                  preview_env;
/*snapshot param*/
    cmr_u32                  quality;
    cmr_u32                  thumb_quality;
    cmr_u32                  set_encode_rotation;
    struct img_size          thum_size;
    cmr_u32  	             cap_rot;
    cmr_u32                  is_cfg_rot_cap;
    cmr_u32                  is_dv;/*1 for DV, 0 for DC*/
    cmr_u32                  is_hdr;
    cmr_u32                  total_cap_num;
    cmr_u32                  is_andorid_zsl;

/*all the above value will be set as 0xFFFFFFFF after inited*/
    cmr_u32                  set_end;
    struct cmr_zoom_param    zoom_param;
	uint32_t                 isp_alg_timeout;
	sem_t                    isp_alg_sem;
	pthread_mutex_t          isp_alg_mutex;
};

struct camera_context {
	/*for the device OEM layer owned*/
	struct grab_context      grab_cxt;
	struct sensor_context    sn_cxt;
	struct isp_context       isp_cxt;
	struct jpeg_context      jpeg_cxt;
	struct scaler_context    scaler_cxt;
	struct rotation_context  rot_cxt;
	struct preview_context   prev_cxt;
	struct snapshot_context  snp_cxt;
	struct focus_context     focus_cxt;
	struct ipm_context       ipm_cxt;
	struct setting_context   setting_cxt;

	/*for the workflow management*/
	cmr_u32                  camera_id;
	cmr_u32                  err_code;
	camera_cb_of_type        camera_cb;
	void*                    client_data;
	cmr_u32                  inited;
	cmr_u32                  camera_mode;
	cmr_uint                 is_discard_frm;
	sem_t                    hdr_sync_sm;
	sem_t                    hdr_flag_sm;
	sem_t                    share_path_sm;
	sem_t                    access_sm;
	sem_t                    isp_sync_sm;
	sem_t                    resume_sync_sm;
	cmr_uint                 share_path_sm_flag;
	cmr_handle               init_thread;
	cmr_handle               pre_preview_thread;

	/*callback thread to hal*/
	cmr_handle               prev_cb_thr_handle;
	cmr_handle               snp_cb_thr_handle;
	cmr_handle               snp_secondary_thr_handle;
	cmr_handle               snp_send_raw_image_handle;

	/*for setting*/
	struct camera_settings   cmr_set;
	cmr_u32                  is_support_fd;
	cmr_u32                  fd_on_off;
	struct isp_face_area	fd_face_area;
	cmr_u32                  is_android_zsl;
	cmr_u32                  flip_on;
	cmr_u32                  is_lls_enable;
	cmr_u32                  lls_shot_mode;
	cmr_u32                  is_vendor_hdr;
	cmr_int                  cap_cnt;

	/*memory func*/
	camera_cb_of_malloc      hal_malloc;
	camera_cb_of_free        hal_free;
	void                     *hal_mem_privdata;

#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
	/*for isp lsc buffer*/
	cmr_uint                 isp_malloc_flag;
	cmr_uint                 isp_lsc_phys_addr;
	cmr_uint                 isp_lsc_virt_addr;
	/*for b4awb buffer*/
	cmr_uint                 isp_b4awb_flag;
	cmr_uint                 b4awb_phys_addr[ISP_B4AWB_BUF_CNT];
	cmr_uint                 b4awb_virt_addr[ISP_B4AWB_BUF_CNT];
#endif
};

cmr_int camera_local_int(cmr_u32 camera_id, camera_cb_of_type callback
		, void *client_data, cmr_uint is_autotest, cmr_handle *oem_handle, void* cb_of_malloc, void* cb_of_free);

cmr_int camera_local_deinit(cmr_handle oem_handle);

cmr_int camera_local_fd_start(cmr_handle oem_handle);

cmr_int camera_local_start_preview(cmr_handle oem_handle, enum takepicture_mode mode,  cmr_uint is_snapshot);
cmr_int camera_local_stop_preview(cmr_handle oem_handle);

cmr_int camera_local_start_snapshot(cmr_handle oem_handle, enum takepicture_mode mode, cmr_uint is_snapshot);

cmr_int camera_local_stop_snapshot(cmr_handle oem_handle);

cmr_int camera_local_redisplay_data(cmr_handle oem_handle, cmr_uint output_addr,
                                                   cmr_uint output_width, cmr_uint output_height,
                                                   cmr_uint input_addr_y, cmr_uint input_addr_uv,
                                                   cmr_uint input_width, cmr_uint input_height);

cmr_int camera_local_get_prev_rect(cmr_handle oem_handle, struct img_rect *param_ptr);

cmr_int camera_get_senor_mode_trim(cmr_handle oem_handle, struct img_rect *sn_trim);

cmr_uint camera_get_preview_angle(cmr_handle oem_handle);

cmr_uint camera_get_exif_info(cmr_handle oem_handle, struct exif_info *exif_info);


cmr_int camera_local_start_focus(cmr_handle oem_handle);

cmr_int camera_local_cancel_focus(cmr_handle oem_handle);

cmr_int camera_local_transfer_caf_to_af(cmr_handle oem_handle);

cmr_int camera_local_transfer_af_to_caf(cmr_handle oem_handle);

cmr_int camera_local_set_param(cmr_handle camera_handle, enum camera_param_type id, cmr_uint param);

cmr_int camera_local_get_zsl_info(cmr_handle oem_handle, cmr_uint *is_support, cmr_uint *max_width, cmr_uint *max_height);

cmr_int camera_local_fast_ctrl(cmr_handle oem_handle);

cmr_int camera_local_pre_flash (cmr_handle oem_handle);

cmr_int camera_local_get_viewangle(cmr_handle oem_handle, struct sensor_view_angle *view_angle);

cmr_int camera_local_set_preview_buffer(cmr_handle oem_handle, cmr_uint src_phy_addr, cmr_uint src_vir_addr);
cmr_int camera_local_set_video_buffer(cmr_handle oem_handle, cmr_uint src_phy_addr, cmr_uint src_vir_addr);
cmr_int camera_local_set_zsl_buffer(cmr_handle oem_handle, cmr_uint src_phy_addr, cmr_uint src_vir_addr);
cmr_int camera_local_set_video_snapshot_buffer(cmr_handle oem_handle, cmr_uint src_phy_addr, cmr_uint src_vir_addr);
cmr_int camera_local_set_zsl_snapshot_buffer(cmr_handle oem_handle, cmr_uint src_phy_addr, cmr_uint src_vir_addr);
cmr_int camera_local_zsl_snapshot_need_pause(cmr_handle oem_handle, cmr_int *flag);
void camera_calibrationconfigure_save (uint32_t start_addr, uint32_t data_size);
void camera_set_reload_support(uint32_t is_support);
cmr_int camera_local_get_isp_info(cmr_handle oem_handle, void **addr, int *size);

void camera_local_start_burst_notice(cmr_handle oem_handle);
void camera_local_end_burst_notice(cmr_handle oem_handle);

#ifdef __cplusplus
}
#endif

#endif
