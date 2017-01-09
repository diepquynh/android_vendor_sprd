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
#ifndef _ALC_AF_CTRL_H_
#define _ALC_AF_CTRL_H_
#include <sys/types.h>
#include "isp_com.h"
#include "ALC_AF_Data.h"
#include "ALC_AF_Common.h"
#include "AfIF.h"//tes_kano_0822

#define ALC_AF_LOGE af_ops->cb_alc_af_log

struct alc_af_context {
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

	struct alc_afm_cfg_info	sprd_filter;
	struct alc_af_ctrl_ops af_ctrl_ops;
	uint32_t touch_win_cnt;
	struct alc_win_coord win_pos[25];
	uint32_t cur_awb_r_gain;
	uint32_t cur_awb_g_gain;
	uint32_t cur_awb_b_gain;
	uint32_t cur_fps;
	uint32_t cur_frame_time;
	uint32_t cur_exp_time;
	uint32_t caf_active;
	uint32_t flash_on;
	struct alc_af_face_area fd_info;
	uint32_t cur_ae_again;
	uint32_t cur_ae_dgain;
	uint32_t cur_ae_iso;
	uint32_t cur_ae_bv;
	uint32_t cur_ae_ev;
	uint32_t magic_end;
	TT_AfIfBuf ttAfIfBuf;//tes_kano_0822
	
	//tes_kano_0902
	uint32_t r_info[1024];
	uint32_t g_info[1024];
	uint32_t b_info[1024];
	uint32_t touch_af;
	uint32_t fd_af_start_cnt ;
	
	uint32_t sensor_w;
	uint32_t sensor_h;
};

////////////////////////// Funcs //////////////////////////


///////////////////// for System ///////////////
alc_af_handle_t alc_af_init(void* isp_handle);
int32_t alc_af_deinit(void* isp_handle);
int32_t alc_af_calc(isp_ctrl_context* handle);
int32_t alc_af_ioctrl(alc_af_handle_t handle, enum alc_af_cmd cmd,
				void *param0, void *param1);


int32_t alc_af_ioctrl_af_start(isp_handle isp_handler, void* param_ptr, int(*call_back)());

int32_t alc_af_ioctrl_set_fd_update(isp_handle isp_handler, void* param_ptr, int(*call_back)());

int32_t alc_af_ioctrl_set_isp_start_info(isp_handle isp_handler, struct isp_video_start* param_ptr);

int32_t alc_af_ioctrl_set_ae_awb_info(isp_ctrl_context* handle,
		void* ae_result,
		void* awb_result,
		void* bv,
		void *rgb_statistics);

int32_t alc_af_ioctrl_set_af_mode(isp_handle isp_handler, void* param_ptr, int(*call_back)());



#endif
