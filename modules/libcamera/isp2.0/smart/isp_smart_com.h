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
#ifndef _ISP_SMART_COM_H_
#define _ISP_SMART_COM_H_

#include "isp_type.h"
#include "isp_smart.h"

#ifdef	 __cplusplus
extern "C" {
#endif

#define ISP_SMART_ENVI_NUM 6
#define ISP_SMART_WEIGHT_NUM 2
#define ISP_SMART_FUNC_NUM 4
#define ISP_SMART_WEIGHT_UNIT 256
#define ISP_SMART_INTERPLATE_PIECEWISE_SAMPLE_NUM 16

typedef isp_s32(*isp_func) (isp_u32 handler_id, void *in_param, void *out_param);

struct isp_smart_interplate_sample {
	isp_s16 x;
	isp_s16 y;
};

struct isp_smart_interplate_piecewise_func {
	uint32_t num;
	struct isp_smart_interplate_sample samples[ISP_SMART_INTERPLATE_PIECEWISE_SAMPLE_NUM];
};

struct isp_smart_property_cfg {
	isp_u32 enable;
	isp_u32 func_type;
	struct isp_smart_interplate_piecewise_func func[ISP_SMART_ENVI_NUM];
	isp_u32 is_update;
};

struct isp_smart_envi_id_result {
	isp_u32 envi_id[ISP_SMART_WEIGHT_NUM];
	isp_u32 weight[ISP_SMART_WEIGHT_NUM];
	isp_u32 is_update;
};

struct isp_smart_envi_detect_module {
	isp_u32 enable;
	struct isp_range envi_range[ISP_SMART_ENVI_NUM];
	struct isp_smart_envi_id_result envi_id_result;
};

struct isp_module_cfg {
	isp_u32 module_id;
	isp_u32 version_id;
	isp_u32 param_id;
	isp_u32 func_num;
	struct isp_sample_point_info cur_index_info[ISP_SMART_FUNC_NUM];
	struct isp_smart_property_cfg smart_cfg[ISP_SMART_FUNC_NUM];
	intptr_t tuning_param_addr;
};

typedef isp_handle isp_smart_handle_t;

#ifdef	 __cplusplus
}
#endif
#endif
