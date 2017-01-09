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
#ifndef _CMR_IPM_H_
#define _CMR_IPM_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "cmr_common.h"

struct ipm_class_tab {
	cmr_uint                             class_type;
	struct class_tab_t                  *hdr_tab_info;
};

struct ipm_frame_in {
	struct img_frm          src_frame;
	struct img_frm          dst_frame;
	cmr_handle              caller_handle;
	void                    *private_data;
};

struct ipm_frame_out {
	struct img_frm          dst_frame;
	struct img_face_area    face_area;
	cmr_handle              caller_handle;
	void                    *private_data;
};

typedef cmr_int (*ipm_callback)(cmr_u32 class_type, struct ipm_frame_out *cb_parm);

struct ipm_init_in {
	cmr_handle              oem_handle;
	cmr_uint                sensor_id;
	cmr_int                 (*get_sensor_info)(cmr_handle oem_handle, cmr_uint sensor_id, struct sensor_exp_info *sensor_info);
	cmr_int                 (*ipm_sensor_ioctl)(cmr_handle oem_handle, cmr_uint cmd_type, struct common_sn_cmd_param *parm);
	cmr_int                 (*ipm_isp_ioctl)(cmr_handle oem_handle, cmr_uint cmd_type, struct common_isp_cmd_param *parm);
};

struct ipm_open_in {
	ipm_callback            reg_cb;
	struct img_size         frame_size;
	struct img_rect         frame_rect;
	cmr_uint                frame_cnt;
};

struct ipm_open_out {
	enum img_fmt            format;
	cmr_uint                total_frame_number;
};

struct ipm_capability {
	cmr_uint                class_type_bits;
};

struct class_ops {
	cmr_int (*open)(cmr_handle ipm_handle, struct ipm_open_in *in, struct ipm_open_out *out,
			cmr_handle *class_handle);
	cmr_int (*close)(cmr_handle class_handle);
	cmr_int (*transfer_frame)(cmr_handle class_handle, struct ipm_frame_in *in,
			struct ipm_frame_out *out);
	cmr_int (*pre_proc)(cmr_handle class_handle);
	cmr_int (*post_proc)(cmr_handle class_handle);
};

struct class_tab_t {
	struct class_ops         *ops;
};

struct ipm_context_t {
	struct ipm_init_in       init_in;
};

struct ipm_common {
	cmr_uint                  class_type;
	struct ipm_context_t      *ipm_cxt;
	cmr_uint                  receive_frame_count;
	cmr_uint                  save_frame_count;
	struct class_ops         *ops;
};

cmr_int cmr_ipm_init(struct ipm_init_in *in, cmr_handle *ipm_handle);
cmr_int cmr_ipm_deinit(cmr_handle ipm_handle);
cmr_int cmr_ipm_open(cmr_handle ipm_handle, cmr_uint class_type,struct ipm_open_in *in,
			struct ipm_open_out *out, cmr_handle *ipm_class_handle);
cmr_int cmr_ipm_close(cmr_handle ipm_class_handle);
cmr_int ipm_transfer_frame(cmr_handle ipm_class_handle, struct ipm_frame_in *in,
			struct ipm_frame_out *out);
cmr_int cmr_ipm_pre_proc(cmr_handle ipm_class_handle);
cmr_int cmr_ipm_post_proc(cmr_handle ipm_class_handle);
cmr_int cmr_ipm_get_capability (struct ipm_capability *out);

#ifdef __cplusplus
}
#endif

#endif
