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

#ifndef _AFT_INTERFACE_H_
#define _AFT_INTERFACE_H_
/*------------------------------------------------------------------------------*
*					Dependencies				*
*-------------------------------------------------------------------------------*/
//#include <sys/types.h>
//#include "cmr_type.h"


/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/
#ifdef  __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------*
*					Data Structures				*
*-------------------------------------------------------------------------------*/
#define AFT_INVALID_HANDLE NULL
#define MAX_AF_FILTER_CNT 10
#define MAX_AF_WIN 32

#ifdef WIN32
typedef __int64               int64_t;
typedef unsigned __int64      uint64_t;
#else
#include <sys/types.h>
#endif

typedef uint64_t        cmr_u64;
typedef int64_t         cmr_s64;
typedef unsigned int    cmr_u32;
typedef unsigned short  cmr_u16;
typedef unsigned char   cmr_u8;

enum aft_posture_type {
	AFT_POSTURE_ACCELEROMETER,
	AFT_POSTURE_MAGNETIC,
	AFT_POSTURE_ORIENTATION,
	AFT_POSTURE_GYRO,
	AFT_POSTURE_MAX
};

enum aft_err_type {
	AFT_SUCCESS = 0x00,
	AFT_ERROR,
	AFT_HANDLER_NULL,
	AFT_ERR_MAX
};

enum aft_mode{
	AFT_MODE_NORMAL = 0x00,
	AFT_MODE_MACRO,
	AFT_MODE_CONTINUE,
	AFT_MODE_VIDEO,
	AFT_MODE_MAX
};

enum aft_calc_data_type {
	AFT_DATA_AF,
	AFT_DATA_IMG_BLK,
	AFT_DATA_AE,
	AFT_DATA_SENSOR,
	AFT_DATA_CAF,
	AFT_DATA_MAX

};

enum aft_cmd {
	AFT_CMD_SET_BASE 			= 0x1000,
	AFT_CMD_SET_AF_MODE			= 0x1001,
	AFT_CMD_SET_CAF_RESET		= 0x1002,
	AFT_CMD_SET_CAF_STOP			= 0x1003,

	AFT_CMD_GET_BASE			= 0x2000,
	AFT_CMD_GET_FV_STATS_CFG	= 0X2001,
	AFT_CMD_GET_AE_SKIP_INFO	= 0X2002
};

enum aft_log_level {
	AFT_LOG_VERBOSE = 0,
	AFT_LOG_DEBUG,
	AFT_LOG_INFO,
	AFT_LOG_WARN,
	AFT_LOG_ERROR,
	AFT_LOG_MAX
};

struct aft_tuning_block_param {
	cmr_u8 *data;
	cmr_u32 data_len;
};


struct aft_af_win_rect {
	cmr_u32 sx;
	cmr_u32 sy;
	cmr_u32 ex;
	cmr_u32 ey;
};

struct aft_af_filter_data {
	cmr_u32 type;
	cmr_u64 *data;
};

struct aft_af_filter_info {
	cmr_u32 filter_num;
	struct aft_af_filter_data filter_data[MAX_AF_FILTER_CNT];
};

struct aft_af_win_cfg {
	cmr_u32 win_cnt;
	struct aft_af_win_rect win_pos[MAX_AF_WIN];
	cmr_u32 win_prio[MAX_AF_WIN];
	cmr_u32 win_sel_mode;
};

struct aft_afm_info {
	struct aft_af_win_cfg win_cfg;
	struct aft_af_filter_info filter_info;
};

struct aft_img_blk_info {
	cmr_u32 block_w;
	cmr_u32 block_h;
	cmr_u32 pix_per_blk;
	cmr_u32 chn_num;
	cmr_u32 *data;
};


struct aft_ae_info {
	cmr_u32 exp_time;  //us
	cmr_u32 gain;   //256 --> 1X
	cmr_u32 cur_lum;
	cmr_u32 target_lum;
	cmr_u32 is_stable;
	cmr_u32 bv;
	cmr_u32 y_sum;
	cmr_u32 cur_scene;
	cmr_u32 registor_pos;
};

struct aft_sensor_info {
	cmr_u32 sensor_type;
//	cmr_s64 timestamp;
	float x;
	float y;
	float z;
};

struct caf_time_stamp {
	cmr_u32 time_stamp_sec;
	cmr_u32 time_stamp_us;
};

struct aft_caf_blk_info {
	cmr_u16 caf_token_id;
	cmr_u16 frame_id;
	cmr_u8 valid_column_num;
	cmr_u8 valid_row_num;
	struct caf_time_stamp time_stamp;
	cmr_u32 *data;
};

struct aft_proc_result {
	cmr_u32 is_caf_trig;
	cmr_u32 is_caf_trig_in_taf;
	cmr_u32 is_need_rough_search;
	cmr_u32 is_cancel_caf;
};

struct aft_proc_calc_param {
	cmr_u32 active_data_type;
	cmr_u32 af_has_suc_rec;
	struct aft_afm_info afm_info;
	struct aft_img_blk_info img_blk_info;
	struct aft_ae_info ae_info;
	struct aft_sensor_info sensor_info;
	struct aft_caf_blk_info caf_blk_info;
};

struct aft_caf_stats_cfg {
	cmr_u8 roi_left_ration;
	cmr_u8 roi_top_ration;
	cmr_u8 roi_width_ration;
	cmr_u8 roi_height_ration;
	cmr_u8 num_blk_hor;
	cmr_u8 num_blk_ver;
};

struct aft_ae_skip_info
{
	uint32_t ae_select_support;
	uint32_t ae_skip_line;
};

typedef struct aft_ctrl_ops
{
	cmr_u8 (*get_sys_time)(cmr_u64* p_time,void *cookie);
	cmr_u8 (*binfile_is_exist)(cmr_u8* is_exist, void *cookie);
	cmr_u8 (*is_aft_mlog)(cmr_u32 *is_mlog, void *cookie);
	cmr_u8 (*aft_log)(cmr_u32 log_level, const char* format, ...);

	void *aft_cookie;
} aft_ctrl_ops_t;

typedef void* aft_sub_handle_t;

typedef struct aft_context{
	aft_sub_handle_t aft_sub_handle;
	cmr_u16 tuning_param_len;
	cmr_u32 af_mode;
	struct aft_ae_info ae_info;
	aft_ctrl_ops_t aft_ops;
}aft_proc_handle_t;

/*------------------------------------------------------------------------------*
*					Data Prototype				*
*-------------------------------------------------------------------------------*/

signed int caf_trigger_init(struct aft_tuning_block_param *init_param, aft_proc_handle_t *handle);
signed int caf_trigger_deinit(aft_proc_handle_t *handle);
signed int caf_trigger_calculation(aft_proc_handle_t *handle,
				struct aft_proc_calc_param *aft_calc_in,
				struct aft_proc_result *aft_calc_result);
signed int caf_trigger_ioctrl(aft_proc_handle_t *handle, enum aft_cmd cmd, void *param0, void *param1);

/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/
#ifdef	 __cplusplus
}
#endif
/*-----------------------------------------------------------------------------*/
#endif
// End
