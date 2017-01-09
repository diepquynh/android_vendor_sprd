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
#ifndef _ISP_DRV_USER_H_
#define _ISP_DRV_USER_H_

#include <linux/types.h>
#include "cmr_common.h"

struct isp_video_limit {
	uint32_t width;
	uint32_t res;
};

struct ips_in_param {
	struct img_frm src_frame;
	uint32_t src_avail_height;
	uint32_t src_slice_height;
	struct img_frm dst_frame;
	uint32_t dst_slice_height;
};

struct ips_out_param {
	uint32_t output_height;
	uint32_t total_proc_height;
};

struct ipn_in_parm {
	uint32_t src_avail_height;
	uint32_t dst_buf_addr;
	uint32_t dst_buf_size;
};

struct ipn_out_parm {
};

enum isp_evt {
	CMR_ISP_DONE = CMR_EVT_ISP_BASE,
	CMR_ISP_ERROR,
};

typedef int (*proc_callback) (uint32_t mode, struct ips_out_param *out_ptr);
typedef int (*af_ctrl) (uint32_t step);
typedef int (*ae_get_gain) (uint32_t *val);
typedef int (*ae_set_gain) (uint32_t val);

struct isp_ctrl_func {
	af_ctrl  af_ctrl_cb;
	ae_get_gain ae_get_gain_cb;
	ae_set_gain ae_set_gain_cb;
};

struct isp_init_param {
	void *setting_param_ptr;
	int32_t size;
	struct isp_ctrl_func  ctrl_func;
};

int  cmr_isp_init (struct isp_init_param *ptr);
int  cmr_isp_deinit (void);
int  cmr_isp_video_capbility(struct isp_video_limit* limit);
//int32_t isp_ioctl(cmd, param, callback);
int  cmr_isp_video_start(struct img_size *img_size);
int  cmr_isp_video_stop(void);
int  cmr_isp_process_start(struct ips_in_param *in_parm_ptr,
		struct ips_out_param *out_param_ptr,
		cmr_evt_cb callback);
int  cmr_isp_process_next(struct ipn_in_parm *in_ptr, struct ipn_out_parm  *out_ptr);
int  cmr_isp_evt_reg(cmr_evt_cb isp_event_cb);



#endif //for _ISP_DRV_USER_H_
