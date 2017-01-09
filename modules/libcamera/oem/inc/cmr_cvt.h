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
#ifndef _CMR_CVT_H_
#define _CMR_CVT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "cmr_common.h"
#include "sprd_dma_copy_k.h"

enum cmr_img_cvt_evt {
	CMR_IMG_CVT_ROT_DONE = CMR_EVT_CVT_BASE,
	CMR_IMG_CVT_SC_DONE,
};

enum cmr_img_cvt_ret {
	CVT_RET_LAST = 1,
};

#define SCALER_IS_DONE          0xFF000000

struct cmr_rot_param{
	cmr_handle              handle;
	enum img_angle          angle;
	struct img_frm          src_img;
	struct img_frm          dst_img;
};

cmr_int cmr_rot_open(cmr_handle *rot_handle);
cmr_int cmr_rot(struct cmr_rot_param *rot_param);
cmr_int cmr_rot_close(cmr_handle rot_handle);
cmr_int cmr_scale_open(cmr_handle *scale_handle);
cmr_int cmr_scale_start(cmr_handle scale_handle, struct img_frm *src_img,
			struct img_frm *dst_img, cmr_evt_cb cmr_event_cb, cmr_handle cb_handle);
cmr_int cmr_scale_close(cmr_handle scale_handle);
cmr_int cmr_scale_capability(cmr_handle scale_handle,cmr_u32 *width, cmr_u32 *sc_factor);
int cmr_dma_copy_init(void);
int cmr_dma_copy_deinit(void);
int cmr_dma_cpy(struct _dma_copy_cfg_tag dma_copy_cfg);

#ifdef __cplusplus
}
#endif

#endif
