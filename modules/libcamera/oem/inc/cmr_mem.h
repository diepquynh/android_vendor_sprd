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
#ifndef _CMR_MEM_H_
#define _CMR_MEM_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "cmr_common.h"

typedef int (*alloc_mem_ptr)(void* handle, unsigned int size, unsigned long *addr_phy, unsigned long *addr_vir);
typedef int (*free_mem_ptr)(void* handle);

struct cmr_cap_2_frm {
	struct img_frm  mem_frm;

	void* handle;
	int type;
	int zoom_post_proc;
	alloc_mem_ptr alloc_mem;
	free_mem_ptr free_mem;
};
int camera_pre_capture_buf_id(cmr_u32 camera_id);

int camera_pre_capture_buf_size(cmr_u32 camera_id,
					cmr_s32 mem_size_id,
					cmr_u32 *mem_size,
					cmr_u32 *mem_sum);

int camera_capture_buf_size(uint32_t     camera_id,
					uint32_t        sn_fmt,
					struct img_size *image_size,
					uint32_t        *mem_size);

int camera_arrange_capture_buf(struct cmr_cap_2_frm *cap_2_frm,
						struct img_size *sn_size,
						struct img_rect *sn_trim,
						struct img_size *image_size,
						uint32_t orig_fmt,
						struct img_size *cap_size,
						struct img_size *thum_size,
						struct cmr_cap_mem *capture_mem,
						uint32_t need_rot,
						uint32_t image_cnt);
uint32_t camera_get_aligned_size(uint32_t type, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif
