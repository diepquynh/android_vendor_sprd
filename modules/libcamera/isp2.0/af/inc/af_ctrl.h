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

#ifndef _ISP_AF_H_
#define _ISP_AF_H_
/*------------------------------------------------------------------------------*
*					Dependencies				*
*-------------------------------------------------------------------------------*/
#include <sys/types.h>
#include "af_log.h"
#include "af_alg.h"

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
#define ISP_AF_END_FLAG 0x80000000

#define AF_MAGIC_START		0xe5a55e5a
#define AF_MAGIC_END		0x5e5ae5a5

#define AF_TRUE 1
#define AF_FALSE 0

#define AF_TRAC(_x_) AF_LOGI _x_
#define AF_RETURN_IF_FAIL(exp,warning) do{if(exp) {AF_TRAC(warning); return exp;}}while(0)
#define AF_TRACE_IF_FAIL(exp,warning) do{if(exp) {AF_TRAC(warning);}}while(0)




/*------------------------------------------------------------------------------*
*					Data Structures				*
*-------------------------------------------------------------------------------*/

typedef void* af_handle_t;


enum af_err_type {
	AF_SUCCESS = 0x00,
	AF_ERROR,
	AF_PARAM_ERROR,
	AF_PARAM_NULL,
	AF_FUN_NULL,
	AF_HANDLER_NULL,
	AF_HANDLER_ID_ERROR,
	AF_HANDLER_CXT_ERROR,
	AF_ALLOC_ERROR,
	AF_FREE_ERROR,
	AF_ERR_MAX
};


enum af_status{
	AF_STATUS_START  = 0x00,
	AF_STATUS_RUNNING,
	AF_STATUS_FINISH,
	AF_STATUS_PAUSE,
	AF_STATUS_RESUME,
	AF_STATUS_RESTART,
	AF_STATUS_STOP,
	AF_STATUS_MAX,
};


enum af_mode{
	AF_MODE_NORMAL=0x00,
	AF_MODE_MACRO,
	AF_MODE_CONTINUE,
	AF_MODE_VIDEO,
	AF_MODE_MANUAL,
	AF_MODE_MAX
};



enum af_cmd {
	AF_CMD_SET_BASE 			= 0x1000,
	AF_CMD_SET_AF_MODE			= 0x1001,
	AF_CMD_SET_AF_POS			= 0x1002,
	AF_CMD_SET_TUNING_MODE			= 0x1003,
	AF_CMD_SET_SCENE_MODE			= 0x1004,
	AF_CMD_SET_AF_START			= 0x1005,
	AF_CMD_SET_AF_STOP			= 0x1006,
	AF_CMD_SET_AF_RESTART			= 0x1007,
	AF_CMD_SET_CAF_RESET			= 0x1008,
	AF_CMD_SET_CAF_STOP			= 0x1009,
	AF_CMD_SET_AF_FINISH			= 0x100A,
	AF_CMD_SET_AF_BYPASS			= 0x100B,
	AF_CMD_SET_DEFAULT_AF_WIN		= 0x100C,
	AF_CMD_SET_FLASH_NOTICE			= 0x100D,
	AF_CMD_SET_ISP_START_INFO		= 0x100E,
	AF_CMD_SET_ISP_STOP_INFO		= 0x100F,
	AF_CMD_SET_ISP_TOOL_AF_TEST		= 0x1010,
	AF_CMD_SET_CAF_TRIG_START		= 0x1011,
	AF_CMD_SET_AE_INFO			= 0x1012,
	AF_CMD_SET_AWB_INFO			= 0x1013,

	AF_CMD_GET_BASE				= 0x2000,
	AF_CMD_GET_AF_MODE			= 0X2001,
	AF_CMD_GET_AF_CUR_POS			= 0x2002,
	AF_CMD_GET_AF_INIT_POS			= 0x2003,
	AF_CMD_GET_MULTI_WIN_CFG		= 0x2004,
};

enum af_filter_type{
	AF_FILTER_SFT_0 = 0x00,
	AF_FILTER_SFT_1 = 0x01,
	AF_FILTER_LAPLACE = 0x02,
	AF_FILTER_SOBEL = 0x03,
	AF_FILTER_MAX
};


enum af_calc_data_type {
	AF_DATA_AF,
	AF_DATA_IMG_BLK,
	AF_DATA_AE,
	AF_DATA_FD,
	AF_DATA_MAX

};


enum af_envi_type {
	AF_ENVI_LOWLUX = 0,
	AF_ENVI_INDOOR,
	AF_ENVI_OUTDOOR,
};


struct af_motor_pos {
	uint32_t motor_pos;
	uint32_t skip_frame;
	uint32_t wait_time;
};

struct af_win_rect {
	uint32_t sx;
	uint32_t sy;
	uint32_t ex;
	uint32_t ey;
};


struct af_monitor_set {
	uint32_t type;
	uint32_t bypass;
	uint32_t int_mode;
	uint32_t skip_num;
	uint32_t need_denoise;
};

struct af_monitor_win {
	uint32_t type;
	struct af_win_rect *win_pos;
};


struct af_trig_info {
	uint32_t win_num;
	uint32_t mode;
	struct af_win_rect win_pos[MAX_AF_WIN];
};


struct af_plat_info {
	uint32_t afm_filter_type_cnt;
	uint32_t afm_win_max_cnt;
	uint32_t isp_w;
	uint32_t isp_h;
};

struct af_tuning_param {
	uint8_t *data;
	uint32_t data_len;
	uint32_t cfg_mode;
};

struct af_filter_data {
	uint32_t type;
	uint64_t *data;
};

struct af_win_cfg {
	uint32_t win_cnt;
	struct af_win_rect win_pos[MAX_AF_WIN];
	uint32_t win_prio[MAX_AF_WIN];
	uint32_t win_sel_mode;
};


struct af_init_result {
	uint32_t init_motor_pos;
};

struct af_filter_info {
	uint32_t filter_num;
	struct af_filter_data *filter_data;
};


struct af_img_blk_info {
	uint32_t block_w;
	uint32_t block_h;
	uint32_t pix_per_blk;
	uint32_t chn_num;
	uint32_t *data;
};

struct af_fd_info {
	uint32_t face_num;
	struct af_win_rect face_pose[MAX_FD_CNT];
};

struct af_ae_info {
	uint32_t exp_time;  //us
	uint32_t gain;   //256 --> 1X
	uint32_t cur_fps;
	uint32_t cur_lum;
	uint32_t target_lum;
	uint32_t is_stable;
};



struct af_calc_param {
	uint32_t data_type;
	void* data;

};

struct af_result_param {
	uint32_t motor_pos;
	uint32_t suc_win;
};

struct af_fd_pos_param {
	uint32_t sx;
	uint32_t sy;
	uint32_t ex;
	uint32_t ey;
};


struct af_status_param {

};

struct af_init_in_param {
	uint32_t af_bypass;
	void* caller;
	uint32_t af_mode;
	uint32_t tuning_param_cnt;
	uint32_t cur_tuning_mode;
	struct af_tuning_param *tuning_param;
	struct af_plat_info plat_info;
	int32_t(*go_position) (void* handle,struct af_motor_pos* in_param);
	int32_t(*end_notice) (void* handle,struct af_result_param* in_param);
	int32_t(*start_notice) (void* handle);
	int32_t(*set_monitor) (void* handle, struct af_monitor_set* in_param, uint32_t cur_envi);
	int32_t(*set_monitor_win) (void* handle, struct af_monitor_win* in_param);
	int32_t(*get_monitor_win_num) (void* handle, uint32_t *win_num);
	int32_t(*ae_awb_lock) (void* handle);
	int32_t(*ae_awb_release) (void* handle);
};



struct af_context_t{
	uint32_t magic_start;
	uint32_t bypass;
	void* caller;
	pthread_mutex_t status_lock;
	af_alg_handle_t af_alg_handle;
	uint32_t af_mode;
	uint32_t running_status;
	uint32_t init_flag;
	uint32_t af_has_suc_rec;
	uint32_t skip_frame_cnt;
	uint32_t cur_af_pos;
	uint32_t ae_awb_lock_cnt;
	uint32_t is_running;
	uint32_t flash_on;
	uint32_t isp_tool_af_test;
	uint32_t cur_awb_r_gain;
	uint32_t cur_awb_g_gain;
	uint32_t cur_awb_b_gain;
	uint32_t awb_is_stab;
	uint32_t cur_fps;
	uint32_t cur_ae_again;
	uint32_t cur_ae_bv;
	uint32_t ae_is_stab;
	uint32_t ae_cur_lum;
	uint32_t bv_thr[2];
	uint32_t rgbdiff_thr[3];
	uint32_t cur_envi;
	struct af_plat_info plat_info;
	struct af_win_cfg win_cfg;
	struct af_fd_info fd_info;
	struct af_result_param af_result;
	struct af_alg_result alg_result;
	int32_t(*go_position) (void* handle,struct af_motor_pos* in_param);
	int32_t(*end_notice) (void* handle,struct af_result_param* in_param);
	int32_t(*start_notice) (void* handle);
	int32_t(*set_monitor) (void* handle, struct af_monitor_set* in_param, uint32_t cur_envi);
	int32_t(*set_monitor_win) (void* handler, struct af_monitor_win* in_param);
	int32_t(*get_monitor_win_num) (void* handler, uint32_t *win_num);
	int32_t(*ae_awb_lock) (void* handle);
	int32_t(*ae_awb_release) (void* handle);
	uint32_t magic_end;


};





/*------------------------------------------------------------------------------*
*					Data Prototype				*
*-------------------------------------------------------------------------------*/
af_handle_t af_init(struct af_init_in_param *init_param,struct af_init_result *result);
int32_t af_deinit(af_handle_t handle, void *param, void *result);
int32_t af_calculation(af_handle_t handle, struct af_calc_param *param, struct af_result_param *result);
int32_t af_ioctrl(af_handle_t handle, enum af_cmd cmd, void *param0, void *param1);



/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/
#ifdef	 __cplusplus
}
#endif
/*-----------------------------------------------------------------------------*/
#endif
// End
