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

#ifndef _ISP_SMART_CTL_H_
#define _ISP_SMART_CTL_H_

#include "isp_type.h"
#include "sensor_raw.h"

#ifdef	 __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
#define SMART_CMD_MASK        0x0F00
#define SMART_WEIGHT_UNIT	  256
#define SMART_MAX_WORK_MODE		16

typedef void *isp_smart_handle_t;
typedef void *smart_handle_t;

enum {
	ISP_SMART_IOCTL_GET_BASE = 0x100,
	ISP_SMART_IOCTL_GET_PARAM,
	ISP_SMART_IOCTL_GET_UPDATE_PARAM,
	ISP_SMART_IOCTL_SET_BASE = 0x200,
	ISP_SMART_IOCTL_SET_PARAM,
	ISP_SMART_IOCTL_SET_WORK_MODE,
	ISP_SMART_IOCTL_SET_FLASH_MODE,
	ISP_SMART_IOCTL_CMD_MAX,
};

enum smart_ctrl_flash_mode {
	SMART_CTRL_FLASH_CLOSE = 0x0,
	SMART_CTRL_FLASH_PRE = 0x1,
	SMART_CTRL_FLASH_MAIN = 0x2,
	SMART_CTRL_FLASH_END = 0x3
};

struct smart_component_result {
	uint32_t type;	//0: normal data(directly to driver); 1: index (few block support);
	// 2: index and weight (few block support); 3 gain: gain (new lens shading support)
	uint32_t offset;
	uint32_t size;	//if data is not NULL, use the data as the output buffer
	int32_t fix_data[4];
	void *data;
};

struct smart_block_result {
	uint32_t block_id;
	uint32_t smart_id;
	uint32_t update;
	uint32_t component_num;
	uint32_t mode_flag;
	uint32_t mode_flag_changed;
	struct smart_component_result component[4];
};

struct smart_tuning_param {
	uint32_t version;
	uint32_t bypass;
	struct isp_data_info data;
};

struct smart_init_param {
	struct smart_tuning_param tuning_param[SMART_MAX_WORK_MODE];
};

struct smart_calc_param {
	int32_t bv;
	int32_t bv_gain;
	isp_u32 stable;
	isp_u32 ct;
};

struct smart_calc_result {
	struct smart_block_result *block_result;
	isp_u32 counts;
};

smart_handle_t smart_ctl_init(struct smart_init_param *param, void *result);
int32_t smart_ctl_calculation(smart_handle_t handle, struct smart_calc_param *param,
			      struct smart_calc_result *result);
int32_t smart_ctl_deinit(smart_handle_t handle, void *param, void *result);
int32_t smart_ctl_ioctl(smart_handle_t handle, uint32_t cmd, void *param, void *result);
int32_t smart_ctl_block_eb(smart_handle_t handle, void *block_eb, uint32_t is_eb);
int32_t smart_ctl_block_enable_recover(smart_handle_t handle, uint32_t smart_id);
int32_t smart_ctl_block_disable(smart_handle_t handle, uint32_t smart_id);

#ifdef	 __cplusplus
}
#endif
#endif				//_ISP_SMART_CTRL_H_
