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
#ifndef _SP_AF_COMMON_H_
#define _SP_AF_COMMON_H_
#include <sys/types.h>

typedef void* sft_af_handle_t;
#define SFT_AF_MAGIC_START		0xe5a55e5a
#define SFT_AF_MAGIC_END		0x5e5ae5a5

enum sft_af_cmd {
	SFT_AF_CMD_SET_BASE 			= 0x1000,
	SFT_AF_CMD_SET_AF_MODE			= 0x1001,
	SFT_AF_CMD_SET_AF_POS			= 0x1002,
	SFT_AF_CMD_SET_SCENE_MODE		= 0x1003,
	SFT_AF_CMD_SET_AF_START			= 0x1004,
	SFT_AF_CMD_SET_AF_STOP			= 0x1005,
	SFT_AF_CMD_SET_AF_BYPASS		= 0x1006,
	SFT_AF_CMD_SET_AF_INFO			= 0x1007,
	SFT_AF_CMD_SET_ISP_START_INFO		= 0x1008,
	SFT_AF_CMD_SET_ISP_STOP_INFO		= 0x1009,
	SFT_AF_CMD_SET_AE_INFO			= 0x100A,
	SFT_AF_CMD_SET_AWB_INFO			= 0x100B,
	SFT_AF_CMD_SET_FLASH_NOTICE		= 0x100C,
	SFT_AF_CMD_SET_FD_UPDATE		= 0x100D,
	SFT_AF_CMD_SET_AF_PARAM			= 0x100E,

	SFT_AF_CMD_GET_BASE			= 0x2000,
	SFT_AF_CMD_GET_AF_MODE			= 0X2001,
	SFT_AF_CMD_GET_AF_CUR_POS		= 0x2002,
	SFT_AF_CMD_GET_AF_INFO			= 0x2003,
	SFT_AF_CMD_GET_AF_VALUE			= 0x2004,

	SFT_AF_CMD_BURST_NOTICE = 0x2005,
};

enum sft_af_mode{
	SFT_AF_MODE_NORMAL=0x00,
	SFT_AF_MODE_MACRO,
	SFT_AF_MODE_MANUAL,
	SFT_AF_MODE_CAF,
	SFT_AF_MODE_VIDEO_CAF,
	SFT_AF_MODE_MAX
};


enum sft_af_rtn {
	SFT_AF_SUCCESS = 0,
	SFT_AF_ERROR = 255
};

enum sft_af_env_mode {
	SFT_AF_OUTDOOR = 0,
	SFT_AF_INDOOR = 1,
	SFT_AF_DARK = 2,
};

struct win_coord {
	uint32_t start_x;
	uint32_t start_y;
	uint32_t end_x;
	uint32_t end_y;
};

struct sft_af_ctrl_ops{
	int32_t (*cb_set_afm_bypass)(sft_af_handle_t handle, uint32_t bypass);
	int32_t (*cb_set_afm_mode)(sft_af_handle_t handle, uint32_t mode);
	int32_t (*cb_set_afm_skip_num)(sft_af_handle_t handle, uint32_t skip_num);
	int32_t (*cb_set_afm_skip_num_clr)(sft_af_handle_t handle);
	int32_t (*cb_set_afm_spsmd_rtgbot_enable)(sft_af_handle_t handle, uint32_t enable);
	int32_t (*cb_set_afm_spsmd_diagonal_enable)(sft_af_handle_t handle, uint32_t enable);
	int32_t (*cb_set_afm_spsmd_cal_mode)(sft_af_handle_t handle, uint32_t mode);
	int32_t (*cb_set_afm_sel_filter1)(sft_af_handle_t handle, uint32_t sel_filter);
	int32_t (*cb_set_afm_sel_filter2)(sft_af_handle_t handle, uint32_t sel_filter);
	int32_t (*cb_set_afm_sobel_type)(sft_af_handle_t handle, uint32_t type);
	int32_t (*cb_set_afm_spsmd_type)(sft_af_handle_t handle, uint32_t type);
	int32_t (*cb_set_afm_sobel_threshold)(sft_af_handle_t handle, uint32_t min, uint32_t max);
	int32_t (*cb_set_afm_spsmd_threshold)(sft_af_handle_t handle, uint32_t min, uint32_t max);
	int32_t (*cb_set_afm_slice_size)(sft_af_handle_t handle, uint32_t width, uint32_t height);
	int32_t (*cb_set_afm_win)(sft_af_handle_t handle, struct win_coord *win_range);
	int32_t (*cb_get_afm_type1_statistic)(sft_af_handle_t handle, uint32_t *statis);
	int32_t (*cb_get_afm_type2_statistic)(sft_af_handle_t handle, uint32_t *statis);
	int32_t (*cb_set_active_win)(sft_af_handle_t handle, uint32_t active_win);
	int32_t (*cb_get_cur_env_mode)(sft_af_handle_t handle, uint8_t *mode);
	int32_t (*cb_set_motor_pos)(sft_af_handle_t handle, uint32_t pos);
	int32_t (*cb_lock_ae)(sft_af_handle_t handle);
	int32_t (*cb_lock_awb)(sft_af_handle_t handle);
	int32_t (*cb_unlock_ae)(sft_af_handle_t handle);
	int32_t (*cb_unlock_awb)(sft_af_handle_t handle);
	uint32_t (*cb_get_ae_lum)(sft_af_handle_t handle);
	uint32_t (*cb_get_ae_status)(sft_af_handle_t handle);
	uint32_t (*cb_get_awb_status)(sft_af_handle_t handle);
	int32_t (*cb_get_isp_size)(sft_af_handle_t handle, uint16_t *widith, uint16_t *height);
	int32_t (*cb_af_finish_notice)(sft_af_handle_t handle, uint32_t result);
	void (*cb_sft_af_log)(const char* format, ...);
	int32_t (*cb_set_sp_afm_cfg)(sft_af_handle_t handle);
	int32_t (*cb_set_sp_afm_win)(sft_af_handle_t handle, struct win_coord *win_range);
	int32_t (*cb_get_sp_afm_statistic)(sft_af_handle_t handle, uint32_t *statis);
	int32_t (*cb_sp_write_i2c)(sft_af_handle_t handle,uint16_t slave_addr, uint8_t *cmd, uint16_t cmd_length);
	int32_t (*cd_sp_read_i2c)(sft_af_handle_t handle,uint16_t slave_addr, uint8_t *cmd, uint16_t cmd_length);
	int32_t (*cd_sp_get_cur_prv_mode)(sft_af_handle_t handle,uint32_t *mode);
	int32_t (*cb_af_move_start_notice)(sft_af_handle_t handle);
	int32_t (*cb_af_pos_update)(sft_af_handle_t handle, uint32_t pos);
};

//typedef union{
//	uint32_t val;
//	struct{
//		uint32_t
//			BYPASS_FLAG 	:1,	// [0] 0:work, 1:bypass
//			AFM_MODE 		:1,	// [6] 1
//			SOURCE_POS  	:1,	// 0:RGB, 1:YCbCr
//			SHIFT 			:5,	// [5:1] ???????
//			SKIP_NUM 		:4, // [10:7] 0
//			SKIP_NUM_CLEAR 	:1, // [11] 0
//			FORMAT 			:3,	// [15:13] 111
//			IIR_BYPASS 		:1,	// [16] 0:work, 1:bypass
//			Reserve1		:1,
//			Reserve2		:1,
//			Reserve3		:1,
//			Reserve4		:1,
//			Reserve5		:1,
//			Reserve6		:1,
//			Reserve7		:1,
//			Reserve8		:1,
//			Reserve9		:1,
//			Reserve10		:1,
//			Reserve11		:1,
//			Reserve12		:1,
//			Reserve13		:1,
//			Reserve14		:1,
//			Reserve15		:1;
//	}dot;
//}sp_afm_cfg_info_regs;

struct sp_afm_cfg_info{
	uint32_t bypass;			//   [0] 0:work, 1:bypass
	uint32_t mode;				//   [6] 1
	uint32_t source_pos;		//   0:RGB, 1:YCbCr
	uint32_t shift;				//   [5:1] ???????
	uint32_t skip_num;			//   [10:7] 0
	uint32_t skip_num_clear;	//   [11] 0
	uint32_t format;			//   [15:13] 111
	uint32_t iir_bypass;		//   [16] 0:work, 1:bypass
	//sp_afm_cfg_info_regs	cfg;
	int16_t 				IIR_c[11];
};

struct sft_af_face_info {
	uint32_t   sx;
	uint32_t   sy;
	uint32_t   ex;
	uint32_t   ey;
	uint32_t   brightness;
	int32_t    pose;
};

struct sft_af_face_area {
	uint16_t type;//focus or ae,
	uint16_t face_num;
	uint16_t frame_width;
	uint16_t frame_height;
	struct sft_af_face_info face_info[10];
};



#endif
