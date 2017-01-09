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

#ifndef _ISP_SPRD_AF_CTRL_H_
#define _ISP_SPRD_AF_CTRL_H_
sprd_af_handle_t sprd_af_init(isp_ctrl_context* handle);
int32_t sprd_af_calc(isp_ctrl_context* handle);
void sprd_af_deinit(isp_ctrl_context* handle);
int32_t sprd_af_image_data_update(isp_ctrl_context* handle);
int32_t sprd_af_ioctrl_set_af_mode(isp_handle isp_handler, void* param_ptr, int(*call_back)());
int32_t sprd_af_ioctrl_get_af_mode(isp_handle isp_handler, void* param_ptr, int(*call_back)());
int32_t sprd_af_ioctrl_af_start(isp_handle isp_handler, void* param_ptr, int(*call_back)());
int32_t sprd_af_ioctrl_af_info(isp_handle isp_handler, void* param_ptr, int(*call_back)());
int32_t sprd_af_ioctrl_thread_msg_send(isp_ctrl_context* handle,
		struct ae_calc_out* ae_result,
		struct cmr_msg* msg);
int32_t sprd_af_ioctrl_set_flash_notice(isp_ctrl_context* handle, void* param_ptr, int(*call_back)());
int32_t sprd_af_ioctrl_set_isp_start_info(isp_handle isp_handler, struct isp_video_start* param_ptr);
int32_t sprd_af_ioctrl_set_isp_stop_info(isp_handle isp_handler);
int32_t sprd_af_ioctrl_get_af_cur_pos(isp_ctrl_context* handle, void* param_ptr, int(*call_back)());
int32_t sprd_af_ioctrl_set_af_pos(isp_ctrl_context* handle, void* param_ptr, int(*call_back)());
int32_t sprd_af_ioctrl_set_af_bypass(isp_ctrl_context* handle, void* param_ptr, int(*call_back)());
int32_t sprd_af_ioctrl_set_af_stop(isp_ctrl_context* handle, void* param_ptr, int(*call_back)());
int32_t sprd_af_ioctrl_set_ae_awb_info(isp_ctrl_context* handle,
		void* ae_result,
		void* awb_result,
		void* bv,
		void *rgb_statistics);

#endif


