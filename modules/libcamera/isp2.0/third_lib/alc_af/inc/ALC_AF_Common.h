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
#ifndef _ALC_AF_COMMON_H_
#define _ALC_AF_COMMON_H_
#include <sys/types.h>

typedef void* alc_af_handle_t;
#define ALC_AF_MAGIC_START		0xe5a55e5a
#define ALC_AF_MAGIC_END		0x5e5ae5a5

enum alc_af_cmd {
	ALC_AF_CMD_SET_BASE 			= 0x1000,
	ALC_AF_CMD_SET_AF_MODE			= 0x1001,
	ALC_AF_CMD_SET_AF_POS			= 0x1002,
	ALC_AF_CMD_SET_SCENE_MODE		= 0x1003,
	ALC_AF_CMD_SET_AF_START			= 0x1004,
	ALC_AF_CMD_SET_AF_STOP			= 0x1005,
	ALC_AF_CMD_SET_AF_BYPASS		= 0x1006,
	ALC_AF_CMD_SET_AF_INFO			= 0x1007,
	ALC_AF_CMD_SET_ISP_START_INFO		= 0x1008,
	ALC_AF_CMD_SET_ISP_STOP_INFO		= 0x1009,
	ALC_AF_CMD_SET_AE_INFO			= 0x100A,
	ALC_AF_CMD_SET_AWB_INFO			= 0x100B,
	ALC_AF_CMD_SET_FLASH_NOTICE		= 0x100C,
	ALC_AF_CMD_SET_FD_UPDATE		= 0x100D,

	
	ALC_AF_CMD_GET_BASE			= 0x2000,
	ALC_AF_CMD_GET_AF_MODE			= 0X2001,
	ALC_AF_CMD_GET_AF_CUR_POS		= 0x2002,
	ALC_AF_CMD_GET_AF_INFO			= 0x2003,
	ALC_AF_CMD_GET_AF_VALUE			= 0x2004,

	ALC_AF_CMD_BURST_NOTICE = 0x2005,
	ALC_AF_CMD_GET_AF_MULTIWIN      	= 0x2006,
};

enum alc_af_mode{
	ALC_AF_MODE_NORMAL=0x00,
	ALC_AF_MODE_MACRO,
	ALC_AF_MODE_MANUAL,
	ALC_AF_MODE_CAF,
	ALC_AF_MODE_VIDEO_CAF,
	ALC_AF_MODE_MULTIWIN,// continue af and single af could work
	ALC_AF_MODE_MAX
};


enum alc_af_rtn {
	ALC_AF_SUCCESS = 0,
	ALC_AF_ERROR = 255
};

enum alc_af_env_mode {
	ALC_AF_OUTDOOR = 0,
	ALC_AF_INDOOR = 1,
	ALC_AF_DARK = 2,
};

struct alc_win_coord {
	uint32_t start_x;
	uint32_t start_y;
	uint32_t end_x;
	uint32_t end_y;
} ;

 struct alc_af_ctrl_ops{
	int32_t (*cb_set_afm_bypass)(alc_af_handle_t handle, uint32_t bypass);
	int32_t (*cb_set_afm_mode)(alc_af_handle_t handle, uint32_t mode);
	int32_t (*cb_set_afm_skip_num)(alc_af_handle_t handle, uint32_t skip_num);
	int32_t (*cb_set_afm_skip_num_clr)(alc_af_handle_t handle);
	int32_t (*cb_set_afm_spsmd_rtgbot_enable)(alc_af_handle_t handle, uint32_t enable);
	int32_t (*cb_set_afm_spsmd_diagonal_enable)(alc_af_handle_t handle, uint32_t enable);
	int32_t (*cb_set_afm_spsmd_cal_mode)(alc_af_handle_t handle, uint32_t mode);
	int32_t (*cb_set_afm_sel_filter1)(alc_af_handle_t handle, uint32_t sel_filter);
	int32_t (*cb_set_afm_sel_filter2)(alc_af_handle_t handle, uint32_t sel_filter);
	int32_t (*cb_set_afm_sobel_type)(alc_af_handle_t handle, uint32_t type);
	int32_t (*cb_set_afm_spsmd_type)(alc_af_handle_t handle, uint32_t type);
	int32_t (*cb_set_afm_sobel_threshold)(alc_af_handle_t handle, uint32_t min, uint32_t max);
	int32_t (*cb_set_afm_spsmd_threshold)(alc_af_handle_t handle, uint32_t min, uint32_t max);
	int32_t (*cb_set_afm_slice_size)(alc_af_handle_t handle, uint32_t width, uint32_t height);
	int32_t (*cb_set_afm_win)(alc_af_handle_t handle, struct win_coord *win_range);
	int32_t (*cb_get_afm_type1_statistic)(alc_af_handle_t handle, uint32_t *statis);
	int32_t (*cb_get_afm_type2_statistic)(alc_af_handle_t handle, uint32_t *statis);
	int32_t (*cb_set_active_win)(alc_af_handle_t handle, uint32_t active_win);
	int32_t (*cb_get_cur_env_mode)(alc_af_handle_t handle, uint8_t *mode);
	int32_t (*cb_set_motor_pos)(alc_af_handle_t handle, uint32_t pos);
	int32_t (*cb_lock_ae)(alc_af_handle_t handle);
	int32_t (*cb_lock_awb)(alc_af_handle_t handle);
	int32_t (*cb_unlock_ae)(alc_af_handle_t handle);
	int32_t (*cb_unlock_awb)(alc_af_handle_t handle);
	uint32_t (*cb_get_ae_lum)(alc_af_handle_t handle);
	uint32_t (*cb_get_ae_status)(alc_af_handle_t handle);
	uint32_t (*cb_get_awb_status)(alc_af_handle_t handle);
	int32_t (*cb_get_isp_size)(alc_af_handle_t handle, uint16_t *widith, uint16_t *height);
	int32_t (*cb_af_finish_notice)(alc_af_handle_t handle, uint32_t result);
	void (*cb_alc_af_log)(const char* format, ...);
	int32_t (*cb_set_sp_afm_cfg)(alc_af_handle_t handle);
	int32_t (*cb_set_sp_afm_win)(alc_af_handle_t handle, struct alc_win_coord *win_range);
	int32_t (*cb_get_sp_afm_statistic)(alc_af_handle_t handle, uint32_t *statis);
	int32_t (*cb_sp_write_i2c)(alc_af_handle_t handle,uint16_t slave_addr, uint8_t *cmd, uint16_t cmd_length);
	int32_t (*cd_sp_read_i2c)(alc_af_handle_t handle,uint16_t slave_addr, uint8_t *cmd, uint16_t cmd_length);
	int32_t (*cd_sp_get_cur_prv_mode)(alc_af_handle_t handle,uint32_t *mode);
	int32_t (*cb_af_move_start_notice)(alc_af_handle_t handle);
	int32_t (*cb_af_pos_update)(alc_af_handle_t handle, uint32_t pos);

	int32_t (*cb_get_motor_range)(alc_af_handle_t handle, uint16_t* min_pos,uint16_t* max_pos);
	int32_t (*cb_get_lens_info)(alc_af_handle_t handle, uint16_t *f_num, uint16_t *f_len);
	int32_t (*cb_get_accelerator_sensor_info)(alc_af_handle_t handle, uint16_t* posture);
	int32_t (*cb_get_magnetic_sensor_info)(alc_af_handle_t handle, uint16_t* ispinning);
};

struct alc_afm_cfg_info{
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

struct alc_af_face_info {
	uint32_t   sx;
	uint32_t   sy;
	uint32_t   ex;
	uint32_t   ey;
	uint32_t   brightness;
	int32_t    pose;
};

struct alc_af_face_area {
	uint16_t type;//focus or ae,
	uint16_t face_num;
	uint16_t frame_width;
	uint16_t frame_height;
	struct alc_af_face_info face_info[10];
};



#endif
