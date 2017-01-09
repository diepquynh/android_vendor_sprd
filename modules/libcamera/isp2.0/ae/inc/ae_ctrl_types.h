/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _AE_CTRL_TYPES_H_
#define _AE_CTRL_TYPES_H_
/*----------------------------------------------------------------------------*
 **				 Dependencies				*
 **---------------------------------------------------------------------------*/
#include "ae_types.h"

#include "awb_ctrl.h"

/**---------------------------------------------------------------------------*
 **				 Compiler Flag				*
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif

/**---------------------------------------------------------------------------*
**				 Macro Define				*
**----------------------------------------------------------------------------*/
#define AE_MAX_PARAM_NUM 8
#define AE_TBL_MAX_INDEX 256
#define AE_FD_NUM 20
#define AE_FLASH_MAX_CELL	40
/**---------------------------------------------------------------------------*
**				Data Prototype				*
**----------------------------------------------------------------------------*/

enum ae_level {
	AE_LEVEL0 = 0x00,
	AE_LEVEL1,
	AE_LEVEL2,
	Ae_LEVEL3,
	AE_LEVEL4,
	AE_LEVEL5,
	AE_LEVEL6,
	AE_LEVEL7,
	AE_LEVEL8,
	AE_LEVEL9,
	AE_LEVEL10,
	AE_LEVEL11,
	AE_LEVEL12,
	AE_LEVEL13,
	AE_LEVEL14,
	AE_LEVEL15,
	AE_LEVEL_MAX
};

enum ae_weight_mode {
	AE_WEIGHT_AVG = 0x00,
	AE_WEIGHT_CENTER,
	AE_WEIGHT_SPOT,
	AE_WEIGHT_MAX
};

enum ae_scene_mode {
	AE_SCENE_NORMAL = 0x00,
	AE_SCENE_NIGHT,
	AE_SCENE_SPORT,
	AE_SCENE_PORTRAIT,
	AE_SCENE_LANDSPACE,
	AE_SCENE_MAX
};

enum ae_iso_mode {
	AE_ISO_AUTO = 0x00,
	AE_ISO_100,
	AE_ISO_200,
	AE_ISO_400,
	AE_ISO_800,
	AE_ISO_1600,
	AE_ISO_MAX
};

enum ae_flicker_mode {
	AE_FLICKER_50HZ = 0x00,
	AE_FLICKER_60HZ,
	AE_FLICKER_OFF,
	AE_FLICKER_AUTO,
	AE_FLICKER_MAX
};

enum ae_io_ctrl_cmd {
	AE_GET_LUM = 0x00,
	AE_GET_INDEX,
	AE_GET_EXP_GAIN,
	AE_GET_ISO,
	AE_GET_EV,
	AE_GET_FPS,
	AE_GET_STAB,
	AE_SET_BYPASS,
	AE_SET_FLICKER,
	AE_SET_SCENE_MODE,
	AE_SET_ISO,
	AE_SET_FPS,
	AE_SET_EV_OFFSET,
	AE_SET_WEIGHT,
	AE_SET_STAT_TRIM,
	AE_SET_TOUCH_ZONE,
	AE_SET_INDEX,
	AE_SET_EXP_GAIN,
	AE_SET_CAP_EXP,
	AE_SET_PROC,
	AE_SET_WORK_MODE,
	AE_SET_PAUSE,
	AE_SET_RESTORE,
	AE_SET_FLASH_NOTICE,
	AE_SET_FORCE_QUICK_MODE,
	AE_SET_EXP_TIME,
	AE_SET_SENSITIVITY,
	AE_GET_FLASH_EFFECT,
	AE_GET_AE_STATE,
	AE_GET_FLASH_EB,
	AE_SET_TUNING_EB,
	AE_GET_BV_BY_LUM,
	AE_GET_BV_BY_GAIN,
	AE_SET_G_STAT,
	AE_SET_FORCE_PAUSE, //for mp tool, not change by af or other
	AE_SET_FORCE_RESTORE, //for mp tool, not change by af or other
	AE_SET_TARGET_LUM,
	AE_SET_SNAPSHOT_NOTICE,
	AE_GET_MONITOR_INFO,
	AE_GET_FLICKER_MODE,
	AE_SET_ONLINE_CTRL,
	AE_SET_EXP_ANIT,
	AE_GET_FLICKER_SWITCH_FLAG,
	AE_SET_FD_ENABLE,
	AE_SET_FD_PARAM,
	AE_GET_EXP,
	AE_GET_GAIN,
	AE_GET_CUR_WEIGHT,
	AE_GET_SKIP_FRAME_NUM,
	AE_SET_FLASH_ON_OFF_THR,
	AE_SET_NIGHT_MODE,
	AE_GET_NORMAL_INFO,
	AE_GET_AIS_HANDLE,
	AE_IO_MAX
};

enum ae_work_mode {
	AE_WORK_MODE_COMMON,
	AE_WORK_MODE_CAPTURE,
	AE_WORK_MODE_VIDEO,
	AE_WORK_MODE_MAX
};

enum ae_state {
	AE_STATE_INACTIVE,
	AE_STATE_SEARCHING,
	AE_STATE_CONVERGED,
	AE_STATE_LOCKED,
	AE_STATE_FLASH_REQUIRED,
	AE_STATE_PRECAPTURE,
	AE_STATE_MAX
};

enum ae_cb_type {
	AE_CB_CONVERGED,
	AE_CB_FLASHING_CONVERGED,
	AE_CB_QUICKMODE_DOWN,
	AE_CB_STAB_NOTIFY,
	AE_CB_AE_LOCK_NOTIFY,
	AE_CB_AE_UNLOCK_NOTIFY,
	AE_CB_CLOSE_PREFLASH,
	AE_CB_PREFLASH_PERIOD_END,
	AE_CB_CLOSE_MAIN_FLASH,
	AE_CB_MAX
};

enum ae_flash_mode {
	AE_FLASH_PRE_BEFORE,
	AE_FLASH_PRE_LIGHTING,
	AE_FLASH_PRE_AFTER,
	AE_FLASH_MAIN_BEFORE,
	AE_FLASH_MAIN_LIGHTING,
	AE_FLASH_MAIN_AE_MEASURE,
	AE_FLASH_MAIN_AFTER,
	AE_FLASH_AF_DONE,
	AE_FLASH_MODE_MAX
};

enum ae_statistics_mode {
	AE_STATISTICS_MODE_SINGLE,
	AE_STATISTICS_MODE_CONTINUE,
	AE_STATISTICS_MODE_MAX
};


enum ae_aem_fmt {
	AE_AEM_FMT_RGB      = 0x0001,
	AE_AEM_FMT_YIQ      = 0x0002,
	AE_AEM_FMT_BINNING  = 0x0004,
	AE_AEM_FMT_MAX
};

enum ae_flash_type {
	AE_FLASH_TYPE_PREFLASH,
	AE_FLASH_TYPE_MAIN,
	AE_FLASH_TYPE_MAX
};

struct ae_set_flicker {
	enum ae_flicker_mode mode;
};

struct ae_set_weight {
	enum ae_weight_mode mode;
};

struct ae_set_scene {
	enum ae_scene_mode mode;
};

struct ae_resolution_info {
	struct ae_size frame_size;
	uint32_t line_time;
	uint32_t frame_line;
	uint32_t sensor_size_index;
};
struct ae_measure_highflash {
	uint32_t highflash_flag;
	uint32_t capture_skip_num;
};

struct ae_set_work_param {
	uint32_t fly_eb;
	enum ae_work_mode mode;
	struct ae_resolution_info resolution_info;
	struct ae_measure_highflash highflash_measure;
};

struct ae_set_iso {
	enum ae_iso_mode mode;
};

struct ae_set_pfs {
	uint32_t fps; // min fps
	uint32_t fix_fps; // fix fps flag
};

struct ae_set_ev {
	enum ae_level level;
};

struct ae_get_ev {
	enum ae_level ev_index;
	int32_t ev_tab[16];
};

struct ae_set_tuoch_zone {
	struct ae_trim touch_zone;
};

struct ae_set_index {
	uint32_t index;
};

struct ae_exposure {
	uint32_t exposure;
	uint32_t dummy;
	uint32_t size_index;
};

struct ae_gain {
	uint32_t gain;
};

struct ae_exposure_gain {
	uint32_t exposure;
	uint32_t dummy;
	uint32_t again;
	uint32_t dgain;
};

struct ae_monitor_cfg {
	uint32_t skip_num;
};

struct ae_monitor_info {
	struct ae_size win_size;
	struct ae_size win_num;
	struct ae_trim trim;
};

struct ae_scene_mode_info {
	enum ae_scene_mode scene_mode;
	struct ae_set_fps fps;
};

struct ae_mode_info {
	/*after dcam init, those para will be configered by app*/
	uint32_t enable;
	uint32_t ev_offset;
	uint32_t mode;
	uint32_t iso;
	uint32_t flicker;
	uint32_t fix_fps;
	uint32_t min_fps;
	uint32_t weight;
	uint32_t index_default;
};

struct ae_normal_info {
	//uint32_t gain; TOB
	uint32_t exposure;//unit: us
	uint32_t fps;
};


struct ae_flash_element {
	uint16_t index;
	uint16_t val;
};

struct ae_flash_cell {
	uint8_t type;
	uint8_t count;
	uint8_t def_val;
	struct ae_flash_element element[AE_FLASH_MAX_CELL];
};

struct ae_isp_ctrl_ops {
	void *isp_handler;
	int32_t (*set_exposure)(void *handler, struct ae_exposure *in_param);
	int32_t (*set_again)(void *handler, struct ae_gain *in_param);
	int32_t (*set_monitor)(void *handler, struct ae_monitor_cfg *in_param);
	int32_t (*set_monitor_win)(void *handler,struct ae_monitor_info *in_param);
	int32_t (*callback)(void *handler, uint32_t cb_type);
	int32_t (*set_monitor_bypass)(void *handler, uint32_t is_bypass);
	int32_t (*get_system_time)(void *handler, uint32_t *sec, uint32_t *usec);
	int32_t (*set_statistics_mode)(void *handler, enum ae_statistics_mode mode, uint32_t skip_number);

	int32_t (*flash_get_charge)(void *handler, struct ae_flash_cell *cell);
	int32_t (*flash_get_time)(void *handler, struct ae_flash_cell *cell);
	int32_t (*flash_set_charge)(void *handler, uint8_t type, struct ae_flash_element *element);
	int32_t (*flash_set_time)(void *handler, uint8_t type, struct ae_flash_element *element);
	int32_t (*ex_set_exposure)(void *handler, struct ae_exposure *in_param);
};

struct ae_init_in {
	uint32_t param_num;
	struct ae_param param[AE_MAX_PARAM_NUM];
	struct ae_size monitor_win_num;
	struct ae_isp_ctrl_ops isp_ops;
	struct ae_resolution_info resolution_info;
	uint32_t camera_id;
	uint32_t has_force_bypass;

	struct awb_ctrl_opt_info otp_info;
	void* lsc_otp_random;
	void* lsc_otp_golden;
	uint32_t lsc_otp_width;
	uint32_t lsc_otp_height;
};

struct ae_init_out {
	uint32_t cur_index;
	uint32_t cur_exposure;
	uint32_t cur_again;
	uint32_t cur_dgain;
	uint32_t cur_dummy;
};

struct ae_calc_in {
	uint32_t stat_fmt; //enum ae_aem_fmt
	union {
		uint32_t *stat_img;
		uint32_t *rgb_stat_img;
	};
	uint32_t *yiq_stat_img;
	uint32_t awb_gain_r;
	uint32_t awb_gain_g;
	uint32_t awb_gain_b;
	uint32_t sec;
	uint32_t usec;
};

struct tg_ae_ctrl_alc_log {
	uint8_t* log;
	uint32_t size;
} log_ae;

struct ae_calc_out {
	uint32_t cur_lum;
	uint32_t cur_index;
	uint32_t cur_ev;
	uint32_t cur_exp_line;
	uint32_t cur_dummy;
	uint32_t cur_again;
	uint32_t cur_dgain;
	uint32_t cur_iso;
	uint32_t is_stab;
	uint32_t line_time;
	uint32_t frame_line;
	uint32_t target_lum;
	struct tg_ae_ctrl_alc_log log_ae;
};

struct ae_flash_power {
	int32_t max_charge; //mA
	int32_t max_time; //ms
};

struct ae_flash_notice {
	uint32_t mode; //enum isp_flash_mode
	uint32_t will_capture;
	union {
		uint32_t flash_ratio;
		struct ae_flash_power power;
	};
	uint32_t capture_skip_num;
};

struct ae_stat_mode {
	uint32_t mode; //0:normal; 1: G width
	uint32_t will_capture;
	struct ae_trim trim;
};

struct ae_snapshot_notice {
	uint32_t type;
	uint32_t preview_line_time;
	uint32_t capture_line_time;
};

enum ae_online_ctrl_mode{
	AE_CTRL_SET_INDEX=0x00,
	AE_CTRL_SET,
	AE_CTRL_GET,
	AE_CTRL_MODE_MAX
};

struct ae_online_ctrl{
	enum ae_online_ctrl_mode mode;
	uint32_t index;
	uint32_t lum;
	uint32_t shutter;
	uint32_t dummy;
	uint32_t again;
	uint32_t dgain;
	uint32_t skipa;
	uint32_t skipd;
};

struct ae_face {
	struct ae_rect rect;
	uint32_t face_lum;
	int32_t pose;  /* face pose: frontal, half-profile, full-profile */
};

struct ae_fd_param {
	uint16_t width;
	uint16_t height;
	uint16_t face_num;
	struct ae_face face_area[AE_FD_NUM];
};

/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif

