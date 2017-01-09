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
#ifndef _AWB_CTRL_H_
#define _AWB_CTRL_H_
/*------------------------------------------------------------------------------*
*				Dependencies					*
*-------------------------------------------------------------------------------*/
#include "awb_ctrl_types.h"
#include "isp_awb_types.h"
/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif
/*------------------------------------------------------------------------------*
*				Micro Define					*
*-------------------------------------------------------------------------------*/
#define AWB_CTRL_INVALID_HANDLE NULL
#define AWB_CTRL_ENVI_NUM 8

/*------------------------------------------------------------------------------*
*				Data Structures					*
*-------------------------------------------------------------------------------*/
typedef void* awb_ctrl_handle_t;

enum awb_ctrl_rtn {
	AWB_CTRL_SUCCESS = 0,
	AWB_CTRL_ERROR = 255
};

enum awb_ctrl_cmd {
	AWB_CTRL_CMD_SET_BASE = 0x100,
	AWB_CTRL_CMD_SET_SCENE_MODE = 0x101,
	AWB_CTRL_CMD_SET_WB_MODE = 0x102,
	AWB_CTRL_CMD_SET_FLASH_MODE = 0x103,
	AWB_CTRL_CMD_SET_STAT_IMG_SIZE = 0x104,
	AWB_CTRL_CMD_SET_STAT_IMG_FORMAT = 0x105,
	AWB_CTRL_CMD_SET_WORK_MODE = 0x106,
	AWB_CTRL_CMD_SET_UPDATE_TUNING_PARAM = 0X107,
	AWB_CTRL_CMD_GET_BASE = 0x200,
	AWB_CTRL_CMD_GET_PARAM_WIN_START = 0X201,
	AWB_CTRL_CMD_GET_PARAM_WIN_SIZE = 0x202,
	AWB_CTRL_CMD_GET_GAIN = 0x203,
	AWB_CTRL_CMD_FLASH_OPEN_P = 0x300,
	AWB_CTRL_CMD_FLASH_OPEN_M = 0x301,
	AWB_CTRL_CMD_FLASHING = 0x302,
	AWB_CTRL_CMD_FLASH_CLOSE = 0x303,
	AWB_CTRL_CMD_LOCK = 0x304,
	AWB_CTRL_CMD_UNLOCK = 0x305,
	AWB_CTRL_CMD_GET_STAT_SIZE = 0x306,
	AWB_CTRL_CMD_GET_WIN_SIZE = 0x307,
	AWB_CTRL_CMD_GET_CT = 0x308,
	AWB_CTRL_CMD_FLASH_BEFORE_P = 0x309,
	AWB_CTRL_CMD_SET_FLASH_STATUS = 0x30A,
};

enum awb_ctrl_wb_mode {
	AWB_CTRL_WB_MODE_AUTO			= 0x0,
	AWB_CTRL_MWB_MODE_SUNNY			= 0x1,
	AWB_CTRL_MWB_MODE_CLOUDY		= 0x2,
	AWB_CTRL_MWB_MODE_FLUORESCENT		= 0x3,
	AWB_CTRL_MWB_MODE_INCANDESCENT		= 0x4,
	AWB_CTRL_MWB_MODE_USER_0		= 0x5,
	AWB_CTRL_MWB_MODE_USER_1		= 0x6,
};

enum awb_ctrl_scene_mode {
	AWB_CTRL_SCENEMODE_AUTO			= 0x0,
	AWB_CTRL_SCENEMODE_DUSK			= 0x1,
	AWB_CTRL_SCENEMODE_USER_0		= 0x2,
	AWB_CTRL_SCENEMODE_USER_1		= 0x3
};
enum awb_ctrl_stat_img_format {
	AWB_CTRL_STAT_IMG_CHN			= 0x0,
	AWB_CTRL_STAT_IMG_RAW_8			= 0x1,
	AWB_CTRL_STAT_IMG_RAW_16		= 0x2
};

enum awb_ctrl_envi_id {
	AWB_CTRL_ENVI_COMMON = 0,
	AWB_CTRL_ENVI_LOW_LIGHT = 1,
	AWB_CTRL_ENVI_INDOOR = 2,
	AWB_CTRL_ENVI_OUTDOOR = 3
};

enum awb_ctrl_flash_mode {
	AWB_CTRL_FLASH_END = 0x0,
	AWB_CTRL_FLASH_PRE = 0x1,
	AWB_CTRL_FLASH_MAIN = 0x2,
};

enum awb_ctrl_flash_status {
	AWB_FLASH_PRE_BEFORE,
	AWB_FLASH_PRE_LIGHTING,
	AWB_FLASH_PRE_AFTER,
	AWB_FLASH_MAIN_BEFORE,
	AWB_FLASH_MAIN_LIGHTING,
	AWB_FLASH_MAIN_MEASURE,
	AWB_FLASH_MAIN_AFTER,
	AWB_FLASH_MODE_MAX
};


struct awb_ctrl_chn_img {
	uint32_t *r;
	uint32_t *g;
	uint32_t *b;
};

struct awb_ctrl_weight {
	uint32_t value[2];
	uint32_t weight[2];
};
struct awb_ctrl_size {
	uint16_t w;
	uint16_t h;
};

struct awb_ctrl_pos {
	int16_t x;
	int16_t y;
};

struct awb_ctrl_gain {
	uint32_t r;
	uint32_t g;
	uint32_t b;
};

struct awb_ctrl_range {
	short	min;
	short  	max;
};

struct awb_ctrl_bv {
	uint32_t enable;
	uint32_t num;
	struct awb_ctrl_range bv_range[AWB_CTRL_ENVI_NUM];
};

struct awb_ctrl_envi_info {
	enum awb_ctrl_envi_id envi_id;
	uint32_t weight;
};

struct awb_flash_info {
	enum awb_ctrl_flash_mode flash_mode;
	struct awb_ctrl_gain flash_ratio;
	uint32_t effect;
	uint32_t patten;
	enum awb_ctrl_flash_status flash_status;
};
union awb_ctrl_stat_img {
	struct awb_ctrl_chn_img chn_img;
	uint8_t *raw_img_8;
	uint16_t *raw_img_16;
};

struct awb_ctrl_rgb_l {
	uint32_t r;
	uint32_t g;
	uint32_t b;
};

struct awb_ctrl_opt_info {
	struct awb_ctrl_rgb_l gldn_stat_info;
	struct awb_ctrl_rgb_l rdm_stat_info;
};
struct awb_ctrl_init_param {
	uint32_t base_gain;
	uint32_t awb_enable;
	enum awb_ctrl_wb_mode wb_mode;
	enum awb_ctrl_stat_img_format stat_img_format;
	struct awb_ctrl_size stat_img_size;
	struct awb_ctrl_size stat_win_size;
	struct awb_ctrl_opt_info otp_info;
	void *tuning_param;
	uint32_t param_size;
	uint32_t camera_id;
	void* lsc_otp_random;
	void* lsc_otp_golden;
	uint32_t lsc_otp_width;
	uint32_t lsc_otp_height;
	void* priv_handle;
};

struct awb_ctrl_init_result {
	struct awb_ctrl_gain gain;
	uint32_t ct;
//ALC_S 20150517
	uint32_t use_ccm;
	uint16_t ccm[9];
//ALC_S 20150517

//ALC_S 20150519
	uint32_t use_lsc;
	uint16_t* lsc;
//ALC_S 20150519
	uint32_t lsc_size;
};

/*ALC_S*/
struct awb_ctrl_ae_info {
	int32_t bv;
	float gain;
	float exposure;
	float f_value;
	uint32_t stable;

	int32_t ev_index; /* 0 ~ 15, such as 0(ev-2.0), 1(ev-1.5), 2(ev-1.0), 3(ev-0.5), 4(ev0), 5(ev+0.5), 6(ev+1.0), 7(ev+1.5), 8(ev+2.0) */
	int32_t ev_table[16];
};

/*ALC_E*/
struct awb_ctrl_calc_param {
	uint32_t quick_mode;
	int32_t bv;
	union awb_ctrl_stat_img stat_img;
	struct awb_ctrl_envi_info envi_info[2];
	struct awb_ctrl_ae_info ae_info;
	uint32_t scalar_factor;
};


struct tg_awb_ctrl_alc_log
{
	uint8_t* log;
	uint32_t size;
}log_awb,log_lsc;


struct awb_ctrl_calc_result {
	struct awb_ctrl_gain gain;
	uint32_t ct;
	uint32_t use_ccm;
	uint16_t ccm[9];
//ALC_S 20150519
	uint32_t use_lsc;
	uint16_t* lsc;
//ALC_S 20150519
	uint32_t lsc_size;
/*ALC_S*/
	struct tg_awb_ctrl_alc_log log_awb;
	struct tg_awb_ctrl_alc_log log_lsc;
/*ALC_E*/
};

struct awb_ctrl_lock_info {
	struct awb_ctrl_gain lock_gain;
	uint32_t lock_mode;
	uint32_t lock_ct;
	uint32_t lock_num;
	uint32_t unlock_num;
};

struct awb_data_info {
	void *data_ptr;
	uint32_t data_size;
};


/*------------------------------------------------------------------------------*
*				Data Prototype					*
*-------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/
#endif
// End

