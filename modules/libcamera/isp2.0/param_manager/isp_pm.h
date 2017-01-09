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
#ifndef _ISP_PM_H_
#define _ISP_PM_H_
#include "isp_type.h"
#include "isp_pm_com_type.h"

#ifdef	 __cplusplus
extern	 "C"
{
#endif

#define isp_pm_cmd_mask 0xf000

#define ISP_PARAM_FROM_TOOL 1

enum isp_pm_cmd {
	ISP_PM_CMD_SET_BASE 				= 0x1000,
	ISP_PM_CMD_SET_MODE				,
	ISP_PM_CMD_SET_AWB				,
	ISP_PM_CMD_SET_AE				,
	ISP_PM_CMD_SET_AF				,
	ISP_PM_CMD_SET_SMART				,
	ISP_PM_CMD_SET_OTHERS			,	//init isp block parameter, except 3A&Smart Blocks
	ISP_PM_CMD_SET_SCENE_MODE,
	ISP_PM_CMD_SET_SPECIAL_EFFECT,
	ISP_PM_CMD_ALLOC_BUF_MEMORY       ,
	ISP_PM_CMD_SET_PARAM_SOURCE,
	ISP_PM_CMD_GET_BASE				= 0x2000,
	ISP_PM_CMD_GET_INIT_AE			,
	ISP_PM_CMD_GET_INIT_AWB			,
	ISP_PM_CMD_GET_INIT_AF			,
	ISP_PM_CMD_GET_INIT_SMART		,
	ISP_PM_CMD_GET_SINGLE_SETTING   ,
	ISP_PM_CMD_GET_ISP_SETTING		,
	ISP_PM_CMD_GET_ISP_ALL_SETTING	,
	ISP_PM_CMD_GET_MODEID_BY_FPS,
	ISP_PM_CMD_GET_MODEID_BY_RESOLUTION,
	ISP_PM_CMD_GET_AE_VERSION_ID,
	ISP_PM_CMD_UPDATE_BASE			= 0x3000,
	ISP_PM_CMD_UPDATE_ALL_PARAMS,
	ISP_PM_CMD_UPDATE_LSC_OTP		,

	ISP_PM_CMD_SET_THIRD_PART_BASE	= 0x4000,
	ISP_PM_CMD_GET_THIRD_PART_BASE	= 0x5000,
	ISP_PM_CMD_GET_THIRD_PART_INIT_SFT_AF,

	ISP_PM_CMD_UPDATE_THIRD_PART_BASE	= 0x6000,

};

typedef isp_handle isp_pm_handle_t;


struct isp_pm_init_input {
	uint32_t num;
	struct isp_data_info tuning_data[ISP_TUNE_MODE_MAX];
	isp_ctrl_context *isp_ctrl_cxt_handle;
};

struct isp_pm_init_output {
	void* param_ptr;
	uint32_t param_size;
};

struct isp_pm_ioctl_input {
	struct isp_pm_param_data *param_data_ptr;
	uint32_t param_num;
};

struct isp_pm_ioctl_output {
	struct isp_pm_param_data *param_data;
	uint32_t param_num;
};

struct isp_pm_update_input {
	isp_u32 num;
	struct isp_data_info tuning_data[ISP_TUNE_MODE_MAX];
};
struct isp_pm_update_output {
	void* param_ptr;
	uint32_t param_size;
};

isp_pm_handle_t isp_pm_init(struct isp_pm_init_input *input, void* output);
int32_t isp_pm_ioctl(isp_pm_handle_t handle, enum isp_pm_cmd cmd, void *input, void *output);
int32_t isp_pm_update(isp_pm_handle_t handle, enum isp_pm_cmd cmd, void *input, void *output);
int32_t isp_pm_deinit(isp_pm_handle_t handle, void *input, void* output);


#ifdef	 __cplusplus
}
#endif

#endif //_ISP_PM_H_
