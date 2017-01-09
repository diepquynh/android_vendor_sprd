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

#ifndef _AF_ALG_H_
#define _AF_ALG_H_
/*------------------------------------------------------------------------------*
*					Dependencies				*
*-------------------------------------------------------------------------------*/
#include <sys/types.h>
#include "af_log.h"

#ifdef WIN32
#include "sci_type.h"
#endif

/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/
#ifdef  __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------*
*					Micro Define				*
*-------------------------------------------------------------------------------*/
#define AF_INVALID_HANDLE NULL
#define MAX_AF_FILTER_CNT 10
#define MAX_AF_WIN 32
#define MAX_FD_CNT 20
#define AF_ALG_TRUE 1
#define AF_ALG_FALSE 0

/*------------------------------------------------------------------------------*
*					Data Structures				*
*-------------------------------------------------------------------------------*/


typedef void* af_alg_handle_t;


enum af_alg_err_type {
	AF_ALG_SUCCESS = 0x00,
	AF_ALG_ERROR,
	AF_ALG_PARAM_ERROR,
	AF_ALG_PARAM_NULL,
	AF_ALG_FUN_NULL,
	AF_ALG_HANDLER_NULL,
	AF_ALG_HANDLER_ID_ERROR,
	AF_ALG_ALLOC_ERROR,
	AF_ALG_FREE_ERROR,
	AF_ALG_ERR_MAX
};


enum af_alg_status{
	AF_ALG_STATUS_START = 0x00,
	AF_ALG_STATUS_RUNNING,
	AF_ALG_STATUS_FINISH,
	AF_ALG_STATUS_PAUSE,
	AF_ALG_STATUS_RESUME,
	AF_ALG_STATUS_RESTART,
	AF_ALG_STATUS_STOP,
	AF_ALG_STATUS_MAX
};


enum af_alg_mode{
	AF_ALG_MODE_NORMAL = 0x00,
	AF_ALG_MODE_MACRO,
	AF_ALG_MODE_CONTINUE,
	AF_ALG_MODE_VIDEO,
	AF_ALG_MODE_MAX
};

enum af_alg_calc_data_type {
	AF_ALG_DATA_AF,
	AF_ALG_DATA_IMG_BLK,
	AF_ALG_DATA_AE,
	AF_ALG_DATA_FD,
	AF_ALG_DATA_MAX

};


enum af_alg_cmd {
	AF_ALG_CMD_SET_BASE 			= 0x1000,
	AF_ALG_CMD_SET_AF_MODE			= 0x1001,
	AF_ALG_CMD_SET_AF_POS			= 0x1002,
	AF_ALG_CMD_SET_TUNING_MODE		= 0x1003,
	AF_ALG_CMD_SET_SCENE_MODE		= 0x1004,
	AF_ALG_CMD_SET_AF_STATUS		= 0x1005,
	AF_ALG_CMD_SET_CAF_RESET		= 0x1006,
	AF_ALG_CMD_SET_CAF_STOP			= 0x1007,

	AF_ALG_CMD_GET_BASE			= 0x2000,
	AF_ALG_CMD_GET_AF_MODE			= 0X2001,
	AF_ALG_CMD_GET_AF_CUR_POS		= 0x2002,
	AF_ALG_CMD_GET_AF_INIT_POS		= 0x2003,
	AF_ALG_CMD_GET_AF_STATUS		= 0x2004,
	AF_ALG_CMD_GET_MULTI_WIN_CFG		= 0x2005,
};


struct af_alg_plat_info {
	uint32_t afm_filter_type_cnt;
	uint32_t afm_win_max_cnt;
};

struct af_alg_tuning_block_param {
	uint8_t *data;
	uint32_t data_len;
	uint32_t cfg_mode;
};

struct af_alg_filter_data {
	uint32_t type;
	uint64_t *data;
};


struct af_alg_win_rect {
	uint32_t sx;
	uint32_t sy;
	uint32_t ex;
	uint32_t ey;
};

struct af_alg_filter_info {
	uint32_t filter_num;
	struct af_alg_filter_data filter_data[MAX_AF_FILTER_CNT];
};

struct af_alg_af_win_cfg {
	uint32_t win_cnt;
	struct af_alg_win_rect win_pos[MAX_AF_WIN];
	uint32_t win_prio[MAX_AF_WIN];
	uint32_t win_sel_mode;
};

struct af_alg_afm_info {
	struct af_alg_af_win_cfg win_cfg;
	struct af_alg_filter_info filter_info;
};

struct af_alg_img_blk_info {
	uint32_t block_w;
	uint32_t block_h;
	uint32_t pix_per_blk;
	uint32_t chn_num;
	uint32_t *data;
};


struct af_alg_ae_info {
	uint32_t exp_time;  //us
	uint32_t gain;   //256 --> 1X
	uint32_t cur_fps;
	uint32_t cur_lum;
	uint32_t target_lum;
	uint32_t is_stable;
};

struct af_alg_fd_info {
	uint32_t face_num;
	struct af_alg_win_rect face_pose[MAX_FD_CNT];
};



struct af_alg_init_param {
	uint32_t tuning_param_cnt;
	uint32_t cur_tuning_mode;
	uint32_t init_mode;
	struct af_alg_tuning_block_param *tuning_param;
	struct af_alg_plat_info plat_info;
};

struct af_alg_init_result {
	uint32_t init_motor_pos;
};


struct af_alg_calc_param {
	uint32_t cur_motor_pos;
	uint32_t active_data_type;
	uint32_t af_has_suc_rec;
	struct af_alg_afm_info afm_info;
	struct af_alg_img_blk_info img_blk_info;
	struct af_alg_fd_info fd_info;
	struct af_alg_ae_info ae_info;
};


struct af_alg_fd_result
{
	uint32_t is_fd_detected;
	uint32_t face_num;
	struct af_alg_win_rect face_pose[MAX_FD_CNT];
	uint32_t fd_win_prio[MAX_FD_CNT];
};


struct af_alg_result {
	uint32_t motor_pos;
	uint32_t skip_frame;
	uint32_t suc_win;
	uint32_t is_finish;
	uint32_t is_moving;
	uint32_t is_stab;
	uint32_t is_caf_trig;
	uint32_t is_caf_trig_in_saf;
	struct af_alg_fd_result fd_result;
};





struct caf_alg_calc_param {
	uint32_t active_data_type;
	struct af_alg_af_win_cfg win_cfg;
	struct af_alg_filter_info filter_info;
	struct af_alg_img_blk_info img_blk_info;
	struct af_alg_fd_info fd_info;
	struct af_alg_ae_info ae_status;

};

struct caf_alg_result {
	uint32_t need_af;
};



/*------------------------------------------------------------------------------*
*					Data Prototype				*
*-------------------------------------------------------------------------------*/

af_alg_handle_t af_alg_init(struct af_alg_init_param *init_param, struct af_alg_init_result *result);
int32_t af_alg_deinit(af_alg_handle_t handle);
int32_t af_alg_calculation(af_alg_handle_t handle,
				struct af_alg_calc_param *alg_calc_in,
				struct af_alg_result *alg_calc_result);
uint32_t af_alg_ioctrl(af_alg_handle_t handle, enum af_alg_cmd cmd, void *param0, void *param1);

/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/
#ifdef	 __cplusplus
}
#endif
/*-----------------------------------------------------------------------------*/
#endif
// End
