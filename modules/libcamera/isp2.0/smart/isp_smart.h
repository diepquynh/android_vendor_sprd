/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef _ISP_SMART_H_
#define _ISP_SMART_H_

#include "isp_type.h"
#include "isp_smart_com.h"

#ifdef	 __cplusplus
extern "C" {
#endif

enum isp_smart_interplate_func_type {
	ISP_SMART_INTERPLATE_FUNC0 = 0,
	ISP_SMART_INTERPLATE_FUNC1,
	ISP_SMART_INTERPLATE_FUNC2,
	ISP_SMART_INTERPLATE_FUNC_MAX = 0xffff,
};

struct isp_smart_interplate_input0 {
	isp_s32 x;
};

struct isp_smart_interplate_output0 {
	int32_t index[2];
	uint32_t weight[2];
};

struct isp_smart_interplate_input1 {
	int32_t x;
};

struct isp_smart_interplate_output1 {
	isp_u32 y;
};

struct isp_smart_interplate_input2 {
	isp_s32 x;
};

struct isp_smart_interplate_output2 {
	isp_u32 y;
};

isp_smart_handle_t isp_smart_init(void *in_ptr, void *out_ptr);

int32_t isp_smart_calculation(isp_u32 func_type,
			      struct isp_smart_interplate_piecewise_func *cur_func,
			      void *smart_cur_info_in, void *smart_calc_param_out);

int32_t isp_smart_deinit(isp_smart_handle_t handle);

#ifdef	 __cplusplus
}
#endif
#endif				//_ISP_SMART_ALG_H_
