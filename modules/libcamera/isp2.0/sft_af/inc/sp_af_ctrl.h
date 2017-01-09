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
#ifndef _SP_AF_CTRL_H_
#define _SP_AF_CTRL_H_
#include <sys/types.h>
#include "sp_af_common.h"
#include "AF_Common.h"

#define SFT_AF_LOGE af_ops->cb_sft_af_log

struct sft_tuning_param{
	uint32_t version;
	uint32_t bypass;
	uint32_t bv_thr[2];// outdoor: > bv_thr[0]; bv_thr[0]>=middle>=bv_thr[1]; indoor: < bv_thr[1];
	uint32_t rgbdiff_thr[3];// r=rgbdiff_thr[0] g=rgbdiff_thr[1] b=rgbdiff_thr[2]
	uint32_t reserved[32];
	AFDATA 		afCtl;
};


struct sft_af_context {
	uint32_t magic_start;
	void *isp_handle;
	uint32_t bypass;
	uint32_t active_win;
	uint32_t is_inited;
	uint32_t ae_is_stab;
	uint32_t ae_cur_lum;
	uint32_t ae_is_locked;
	uint32_t awb_is_stab;
	uint32_t awb_is_locked;
	uint32_t cur_env_mode;
	uint32_t af_mode;
	uint32_t cur_pos;
	uint32_t is_runing;
	AFDATA 		afCtl;
	AFDATA_RO 	afv;
	AFDATA_FV	fv;
	GLOBAL_OTP	gOtp;
	struct sp_afm_cfg_info	sprd_filter;
	struct sft_af_ctrl_ops af_ctrl_ops;
	uint32_t touch_win_cnt;
	struct win_coord win_pos[25];
	uint32_t cur_awb_r_gain;
	uint32_t cur_awb_g_gain;
	uint32_t cur_awb_b_gain;
	uint32_t cur_fps;
	uint32_t caf_active;
	uint32_t flash_on;
	struct sft_af_face_area fd_info;
	uint32_t cur_ae_again;
	uint32_t cur_ae_bv;
	uint32_t magic_end;
// porting to tshark3
	uint32_t filtertype1;
	uint32_t filtertype2;
	struct afm_thrd_rgb thr_rgb;
	uint32_t statis_tshark3[105];

	uint32_t bv_thr[2];
	uint32_t rgbdiff_thr[3];
};

////////////////////////// Funcs //////////////////////////


///////////////////// for System ///////////////
sft_af_handle_t sft_af_init(void* isp_handle);
int32_t sft_af_deinit(void* handle);
int32_t sft_af_calc(isp_ctrl_context* handle);
int32_t sft_af_ioctrl(sft_af_handle_t handle, enum sft_af_cmd cmd, void *param0, void *param1);
int32_t sft_af_ioctrl_set_af_mode(isp_handle isp_handler, void* param_ptr, int(*call_back)());
int32_t sft_af_ioctrl_set_flash_notice(isp_ctrl_context* handle, void* param_ptr, int(*call_back)());
int32_t sft_af_ioctrl_get_af_mode(isp_handle isp_handler, void* param_ptr, int(*call_back)());
int32_t sft_af_ioctrl_af_start(isp_handle isp_handler, void* param_ptr, int(*call_back)());
int32_t sft_af_ioctrl_set_fd_update(isp_handle isp_handler, void* param_ptr, int(*call_back)());
int32_t sft_af_ioctrl_set_isp_start_info(isp_handle isp_handler, struct isp_video_start* param_ptr);
int32_t sft_af_ioctrl_set_isp_stop_info(isp_handle isp_handler);
int32_t sft_af_ioctrl_set_ae_awb_info(isp_ctrl_context* handle,
		void* ae_result,
		void* awb_result,
		void* bv,
		void* rgb_statistics);
int32_t sft_af_ioctrl_burst_notice(isp_handle isp_handler, void* param_ptr, int(*call_back)());
int32_t sft_af_ioctrl_get_af_value(isp_handle isp_handler, void* param_ptr, int(*call_back)());
int32_t sft_af_ioctrl_iowrite(isp_handle isp_handler, void* param_ptr, int(*call_back)());
int32_t sft_af_ioctrl_ioread(isp_handle isp_handler, void* param_ptr, int(*call_back)());
int32_t sft_af_ioctrl_get_af_cur_pos(isp_ctrl_context* handle, void* param_ptr, int(*call_back)());
int32_t sft_af_ioctrl_set_af_pos(isp_ctrl_context* handle, void* param_ptr, int(*call_back)());
int32_t sft_af_ioctrl_set_af_bypass(isp_ctrl_context* handle, void* param_ptr, int(*call_back)());
int32_t sft_af_ioctrl_set_af_stop(isp_ctrl_context* handle, void* param_ptr, int(*call_back)());
int32_t sft_af_ioctrl_set_af_param(isp_ctrl_context* handle);
#endif
