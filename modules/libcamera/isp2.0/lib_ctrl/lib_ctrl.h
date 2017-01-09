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
#ifndef _LIB_CTRL_H_
#define _LIB_CTRL_H_
#include "isp_com.h"

//awb start
enum awb_lib_product_id {
	SPRD_AWB_LIB				= 0x0,
	AL_AWB_LIB				= 0x80,
};

enum awb_lib_version_id {
	AWB_LIB_VERSION_0				= 0x0,
	AWB_LIB_VERSION_1				= 0x1,
	AWB_LIB_VERSION_2				= 0x2,
	AWB_LIB_VERSION_3				= 0x3,
	AWB_LIB_VERSION_4				= 0x4,
};

enum al_awb_lib_version_id {
	AL_AWB_LIB_VERSION_0				= 0x0,	//ov13850r2a
	AL_AWB_LIB_VERSION_1				= 0x1,	//t4kb3
	AL_AWB_LIB_VERSION_2				= 0x2,
	AL_AWB_LIB_VERSION_3				= 0x3,
	AL_AWB_LIB_VERSION_4				= 0x4,
};

typedef void* sprd_af_handle_t;
typedef void* sft_af_handle_t;

struct awb_lib_fun {
	awb_ctrl_handle_t (*awb_ctrl_init)(struct awb_ctrl_init_param *param,
			struct awb_ctrl_init_result *result);
	uint32_t (*awb_ctrl_deinit)(awb_ctrl_handle_t handle, void *param, void *result);
	uint32_t (*awb_ctrl_calculation)(awb_ctrl_handle_t handle,
			struct awb_ctrl_calc_param *param,
			struct awb_ctrl_calc_result *result);
	uint32_t (*awb_ctrl_ioctrl)(awb_ctrl_handle_t handle, enum awb_ctrl_cmd cmd,
			void *param0, void *param1);
};
//awb end

//ae start
enum ae_lib_product_id {
	SPRD_AE_LIB               = 0x0,
	AL_AE_LIB                 = 0x80,
};

enum ae_lib_version_id {
	AE_LIB_VERSION_0				= 0x0,
	AE_LIB_VERSION_1				= 0x1,
	AE_LIB_VERSION_2				= 0x2,
	AE_LIB_VERSION_3				= 0x3,
	AE_LIB_VERSION_4				= 0x4,
};

struct ae_lib_fun {
	void* (*ae_init)(struct ae_init_in *in_param, struct ae_init_out *out_param);
	int32_t (*ae_deinit)(void* handler, void *in_param, void *out_param);
	int32_t (*ae_calculation)(void* handler, struct ae_calc_in *in_param,
			struct ae_calc_out *out_param);
	int32_t (*ae_io_ctrl)(void* handler, enum ae_io_ctrl_cmd cmd, void *in_param,
			void *out_param);
	uint32_t product_id;
	uint32_t version_id;
};
//ae end

//af start
enum af_lib_product_id {
	SPRD_AF_LIB				= 0x0,
	SFT_AF_LIB				= 0x80,
	ALC_AF_LIB				= 0x81,
};

enum af_lib_version_id {
	AF_LIB_VERSION_0				= 0x0,
	AF_LIB_VERSION_1				= 0x1,
	AF_LIB_VERSION_2				= 0x2,
	AF_LIB_VERSION_3				= 0x3,
	AF_LIB_VERSION_4				= 0x4,
};

struct af_lib_fun {
	sprd_af_handle_t (*af_init_interface)(isp_ctrl_context* handle);
	int32_t (*af_calc_interface)(isp_ctrl_context* handle);
	int32_t (*af_deinit_interface)(sft_af_handle_t handle);
	int32_t (*af_ioctrl_interface)(sft_af_handle_t handle,
			enum af_cmd cmd,
			void *param0, void *param1);
	int32_t (*af_ioctrl_set_flash_notice)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_set_af_info)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_get_af_info)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_get_af_value)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_burst_notice)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_set_af_mode)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_get_af_mode)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_ioread)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_iowrite)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_set_fd_update)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_af_start)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_set_isp_start_info)(isp_handle isp_handler,
			struct isp_video_start* param_ptr);
	int32_t (*af_ioctrl_af_info)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_set_isp_stop_info)(isp_handle isp_handler);
	int32_t (*af_ioctrl_set_ae_awb_info)(isp_ctrl_context* handle,
			void *ae_result,
			void *awb_result,
			void *bv,
			void *rgb_statistics);
	int32_t (*af_ioctrl_thread_msg_send)(isp_ctrl_context* handle,
			struct ae_calc_out* ae_result,
			struct cmr_msg* msg);
	int32_t (*af_image_data_update)(isp_ctrl_context* handle);
	int32_t (*af_ioctrl_get_af_cur_pos)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_set_af_pos)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_set_af_bypass)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_set_af_stop)(isp_handle isp_handler,
			void* param_ptr, int(*call_back)());
	int32_t (*af_ioctrl_set_af_param)(isp_handle isp_handler);
	// if af_posture_support be set ,af_posture_set_handle and af_posture_info_update must also be set
	int32_t (*af_posture_support)(isp_ctrl_context* handle,void* sensordevice);
	int32_t (*af_posture_set_handle)(isp_ctrl_context* handle,uint32_t sensor_type,int sensor_handle);
	int32_t (*af_posture_info_update)(isp_ctrl_context* handle,uint32_t sensor_type,void* data1,void *data2);


};
//af end

uint32_t isp_awblib_init(struct sensor_libuse_info* libuse_info,
		struct awb_lib_fun* awb_lib_fun);
uint32_t isp_aelib_init(struct sensor_libuse_info* libuse_info,
		struct ae_lib_fun* ae_lib_fun);
uint32_t isp_aflib_init(struct sensor_libuse_info* libuse_info,
		struct af_lib_fun* af_lib_fun);
uint32_t aaa_lib_init(isp_ctrl_context* handle, struct sensor_libuse_info* libuse_info);
#endif
