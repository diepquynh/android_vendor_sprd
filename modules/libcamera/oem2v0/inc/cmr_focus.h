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
#ifndef _CMR_FOCUS_H_
#define _CMR_FOCUS_H_

#include "cmr_common.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define CMR_FOCUS_RECT_PARAM_LEN                     (200)

enum af_cb_type {
	AF_CB_DONE,
	AF_CB_FAILED,
	AF_CB_ABORT,
	AF_CB_FOCUS_MOVE,
	AF_CB_MAX
};

enum focus_status_type {
	FOCUS_START = 0,
	FOCUS_NEED_QUIT,
	FOCUS_STATUS_MAX
};

enum focus_receive_evt_type {
	FOCUS_EVT_ISP_AF_NOTICE = CMR_EVT_FOCUS_BASE,
	FOCUS_EVT_MAX
};

typedef void (*af_cb_func_ptr)(enum af_cb_type cb, cmr_s32 parm, void *privdata);

struct af_param {
	cmr_u32                   param;
	cmr_u32                   zone_cnt;
	struct img_rect           zone[FOCUS_ZONE_CNT_MAX];
};

struct af_md_ops {
	cmr_int                  (*af_pre_proc)(cmr_handle  oem_handle);
	cmr_int                  (*af_post_proc)(cmr_handle oem_handle, cmr_int will_capture);
	cmr_int                  (*get_preview_status)(cmr_handle oem_handle);
	cmr_int                  (*af_isp_ioctrl)(cmr_handle oem_handle, cmr_uint cmd, struct common_isp_cmd_param *param_ptr);
	cmr_int                  (*af_sensor_ioctrl)(cmr_handle oem_handle, cmr_uint cmd, struct common_sn_cmd_param *param_ptr);
	cmr_int                  (*get_sensor_info)(cmr_handle oem_handle, cmr_uint sensor_id, struct sensor_exp_info *sensor_info);
	cmr_int                  (*get_flash_info)(cmr_handle oem_handle, cmr_uint sensor_id);
};

struct af_init_param {
	cmr_handle               oem_handle;
	af_cb_func_ptr           evt_cb;
	struct af_md_ops         ops;
};

cmr_int cmr_focus_init(struct af_init_param *parm_ptr,cmr_u32 came_id_bits , cmr_handle *focus_handle);

cmr_int cmr_focus_deinit(cmr_handle focus_handle);

cmr_int cmr_focus_sensor_handle(cmr_handle af_handle, cmr_u32 evt_type, cmr_u32 came_id ,void *data);

cmr_int cmr_focus_isp_handle(cmr_handle af_handle, cmr_u32 evt_type, cmr_u32 came_id ,void *data);

cmr_int cmr_focus_start(cmr_handle af_handle,cmr_u32 came_id);

cmr_int cmr_focus_stop(cmr_handle af_handle, cmr_u32 came_id, cmr_u32 is_need_abort_msg);

cmr_int cmr_focus_set_param(cmr_handle af_handle, cmr_u32 came_id , enum camera_param_type id, void* param);

cmr_int cmr_focus_deinit_notice(cmr_handle af_handle);

cmr_int cmr_af_start_notice_focus(cmr_handle af_handle);

cmr_int cmr_af_cancel_notice_focus(cmr_handle af_handle);
#ifdef __cplusplus
}
#endif

#endif
