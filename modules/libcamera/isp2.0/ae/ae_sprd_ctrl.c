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
#define LOG_TAG "ae_ctrl"
#include "ae_sprd_ctrl.h"
#include "ae_misc.h"
#include "ae_log.h"
#include "ae_utils.h"
#ifndef WIN32
#include <utils/Timers.h>
#include <cutils/properties.h>
#include <math.h>
#include <string.h>
#else
#include "stdio.h"
#include "sci_types.h"
#endif
#include "lib_ctrl.h"

/**---------------------------------------------------------------------------*
 ** 				Compiler Flag					*
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif
/**---------------------------------------------------------------------------*
**				Macro Define					*
**----------------------------------------------------------------------------*/
#define AE_TUNING_VER 1

#define AE_START_ID 0x71717567
#define AE_END_ID 0x69656E64
#define AE_CONVERGED_NUM 2
#define UNUSED(param)  (void)(param)

#define AE_SAVE_PARAM_STR    "persist.sys.isp.ae.param" /*save/no*/
#define AE_SAVE_AE_TABLE_STR "persist.sys.isp.ae.tbl" /*save/no*/
#define AE_SAVE_MLOG_STR     "persist.sys.isp.ae.mlog" /*save/no*/
#define AE_CAP_USE_PREVIEW_TBL  "persist.sys.isp.ae.use_prv_tbl" /*yes/no*/
#define AE_MLOG_PATH "/data/mlog"
#define USE_ISO_TBL 0
#define SEGMENTED_ISO 0
/*should be read from driver later*/
#define AE_FLASH_RATIO (3*256)
/*for 30 LUX*/
#define AE_FLASH_ON_OFF_THR 38
#define AE_FLASH_TARGET_LUM 55
#define AE_FLASH_MAX_RATIO 20
#define AE_CALC_TIMES 10
//#define ISO_FIX_IN_VIDEO
#define VIDEO_IS0 5
#define CAL_CISION 256
#define QUANTIFICAT 1024

#define AE_MONITOR_WORK_MOD 1 /*0: continus, 1:normal mode*/
#define AE_ALG_ID 0

/**---------------------------------------------------------------------------*
**				Data Structures					*
**---------------------------------------------------------------------------*/
struct ae_status {
	struct ae_set_work_param work_info;
	enum ae_work_mode param_from;
	enum ae_scene_mode scene;
	enum ae_flicker_mode flicker;
	enum ae_iso_mode iso;
	enum ae_level ev_level;
	enum ae_weight_mode weight;
	struct ae_trim touch_zone;
	uint32_t target_lum;
	struct ae_set_fps fps;
	uint32_t is_quick_mode;
	enum ae_work_mode real_work_mode;
	uint32_t night_mode;
};

struct touch_zone_param {
	uint32_t enable;
	struct ae_weight_table weight_table;
	struct touch_zone zone_param;
};

enum ae_flash_state {
	AE_FLASH_STATE_BEFORE,
	AE_FLASH_STATE_ACTIVE,
	AE_FLASH_STATE_AFTER,
	AE_FLASH_STATE_MAX,
};

struct flash_param {
	uint32_t enable;
	enum ae_flash_state flash_state;
	uint32_t flash_ratio; //high/low, x256
	struct ae_misc_calc_out before_result;
	struct ae_misc_calc_out capture_result;
	uint32_t effect; // x1024
	uint32_t main_flash_lum;
	uint32_t convergence_speed;
	uint32_t flash_tuning_eb;
	struct ae_misc_calc_out flash_tuning_result;
	uint32_t highflash_skip_cnt;
	uint32_t highflash_skip_en;

	struct ae_misc_calc_out preflash_converged_result;

	uint32_t main_lum;
	uint32_t target_lum;
	uint32_t backup_target_lum;
	uint32_t calc_time;
};

struct ae_monitor_unit {
	uint32_t mode;
	struct ae_size win_num;
	struct ae_size win_size;
	struct ae_trim trim;
	struct ae_monitor_cfg cfg;
	uint32_t is_stop_monitor;
};

struct ae_update_list {
	uint32_t is_iso:1;
	uint32_t is_target_lum:1;
	uint32_t is_quick:1;
	uint32_t is_scene:1;
	uint32_t is_weight:1;
	uint32_t is_touch_zone:1;
	uint32_t is_ev:1;
	uint32_t is_fps:1;
	uint32_t is_night:1;
};

/**************************************************************************/
/* BEGIN: FDAE related definitions                                        */

enum fdae_state {
	FDAE_STATE_OFF = 0,		  		/* FDAE is OFF */
	FDAE_STATE_QUITTING_LOCK_WEIGHT,            /* FDAE is quitting; and the weight table is locked */
	FDAE_STATE_ON,			  		/* FDAE is ON: normal process of face detection and updating AE weight table */
	FDAE_STATE_ON_LOCK_AE,    		/* FDAE is ON: AE is locked */
	FDAE_STATE_ON_LOCK_WEIGHT, 		/* FDAE is ON: the weight table is locked */
};

struct ae_fd_info {
	struct ae_weight_table w_work_tbl;
	int32_t total_w[3];

    // FDAE states
	uint32_t allow_to_work;          /* When allow_to_work==0, FDAE will never work */
    uint32_t enable;                 /* is FDAE enabled?            */
    enum fdae_state state;
    uint32_t is_ae_locked;

	int32_t frames_to_lock;		     /* Number of frames to lock    */
	int32_t frames_no_face;          /* Number of frames without face */
	struct ae_fd_param face_info;    /* The current face information */
	struct ae_fd_param face_info_prev; /* The face information previously used to update weight table */

	int32_t curr_frame_idx;          /* Current frame index  */
	int32_t receive_face_frame_idx;  /* The frame index when latest faces are received */
	int32_t last_disable_frame_idx;  /* The frame index when FDAE is disabled in last time */

	// FDAE Parameters
	int32_t param_face_weight;       /* The ratio of face area weight (in percent) */
	int32_t param_convergence_speed; /* AE convergence speed         */
	int32_t param_lock_ae;           /* frames to lock AE		     */
	int32_t param_lock_weight_has_face;  /* frames to lock the weight table, when has faces */
	int32_t param_lock_weight_no_face;   /* frames to lock the weight table, when no faces */

	// The following are used to restore AE states when quitting "FD/AE" mode
	uint32_t ae_is_quick_mode;
	uint32_t ae_convergence_speed;
};

/* END: FDAE related definitions                                          */
/**************************************************************************/

/* ae handler for isp_app */
struct ae_ctrl_context {
	uint32_t start_id;
	pthread_mutex_t status_lock;
	void* misc_handle; // misc handler
	struct ae_tuning_param tuning_param[AE_MAX_PARAM_NUM];
	uint32_t tuning_param_enable[AE_MAX_PARAM_NUM];

	struct ae_resolution_info resolution_info;
	struct ae_monitor_unit monitor_unit;
	struct ae_stat_req stat_req;

	uint32_t monitor_mode;
	uint32_t monitor_skip_num;
	uint32_t exp_skip_num;
	uint32_t gain_skip_num;
	uint32_t monitor_period_num;
	uint32_t exp_period_num;
	uint32_t gain_period_num;
	uint32_t skip_frame_cnt;
	uint32_t valid_stat;
	uint32_t stab_cnt;
	uint32_t stab_num;
	uint32_t is_drop_stat;


	struct ae_tuning_param *cur_param;
	struct ae_status prv_status;
	struct ae_status cur_status;
	struct ae_status nxt_status;
	struct ae_misc_calc_out cur_result;
	struct ae_misc_calc_out prv_result;

	uint32_t is_force_lock;
	uint32_t force_ae_state;
	int32_t pause_ref_cnt;
	uint32_t pause_ae_state;
	struct ae_misc_calc_out backup_result;


	struct touch_zone_param touch_zone_param;
	uint32_t ae_state;
	struct flash_param flash_param;
	struct ae_isp_ctrl_ops isp_ops;

	uint32_t write_conter;
	uint32_t write_eb;

	uint32_t is_first;
	uint32_t is_quick_mode;
	struct ae_update_list update_list;
	uint32_t start_index;
	int32_t is_mlog;
	struct ae_fd_info fdae;

	uint32_t prv_index;
	uint32_t prv_dummy;
	uint32_t ctrl_quick_ae_cb_ext;
	uint32_t skip_calc_num;
	uint32_t convergerd_num;
	uint32_t cap_skip_num;
	uint32_t camera_id;
	uint32_t flash_working;
	uint32_t work_in_video;
	uint32_t has_force_bypass;
	uint32_t frame_id;
	struct ae_history history;
	int32_t flash_on_off_thr;
	uint32_t write_sensor; 
	uint32_t force_quick_mode;
	uint32_t end_id;
};

#define AE_PRINT_TIME \
		do {                                                       \
                        nsecs_t timestamp = systemTime(CLOCK_MONOTONIC);   \
                        AE_LOGI("timestamp = %lld.", timestamp/1000000);  \
		} while(0)

#ifndef MAX
#define  MAX( _x, _y ) ( ((_x) > (_y)) ? (_x) : (_y) )
#define  MIN( _x, _y ) ( ((_x) < (_y)) ? (_x) : (_y) )
#endif

static int32_t _process_flash(struct ae_ctrl_context *cxt, int32_t cur_lum);
static int32_t _set_frame_cnt(struct ae_ctrl_context *cxt);
static int32_t _get_real_gain(uint32_t gain);
static int32_t _is_work_to_scene(struct ae_ctrl_context *cxt,
									struct ae_status *next_status);
static int32_t _get_new_index(struct ae_ctrl_context *cxt,
								uint32_t cur_exp_gain,
								uint32_t *new_index);
static int32_t _set_target_lum(struct ae_ctrl_context *cxt, uint32_t lum);
static int32_t _get_ae0_real_gain(uint32_t gain);

static int32_t _get_flicker_switch_flag(struct ae_ctrl_context *cxt, void* in_param);

static void _fdae_update_state(struct ae_ctrl_context *cxt);
static int32_t _fdae_init(struct ae_ctrl_context *cxt);

static int32_t _set_iso(struct ae_ctrl_context *cxt, enum ae_iso_mode iso);
static void _fdae_disable_fdae(struct ae_ctrl_context *cxt);
static int32_t _cfg_monitor(struct ae_ctrl_context *cxt);
static int32_t _get_bv_by_lum(struct ae_ctrl_context *cxt,
                                   struct ae_misc_calc_out *cur_result,
                                   int32_t *bv);
/**---------------------------------------------------------------------------*
** 				Local Function Prototypes				*
**---------------------------------------------------------------------------*/

static int32_t _is_save_ae_param()
{
	int32_t is_save = 0;
#ifndef WIN32
	char value[PROPERTY_VALUE_MAX];

	property_get(AE_SAVE_PARAM_STR, value, "no");

	if (!strcmp(value, "save")) {
		is_save = 1;
	}
#endif
	return is_save;
}

static int32_t _is_save_ae_tbl()
{
	int32_t is_save = 0;
#ifndef WIN32
	char value[PROPERTY_VALUE_MAX];

	property_get(AE_SAVE_AE_TABLE_STR, value, "no");

	if (!strcmp(value, "save")) {
		is_save = 1;
	}
#endif
	return is_save;
}

static int32_t _is_ae_mlog()
{
	int32_t is_save = 0;
#ifndef WIN32
	char value[PROPERTY_VALUE_MAX];

	property_get(AE_SAVE_MLOG_STR, value, "no");

	if (!strcmp(value, "save")) {
		is_save = 1;
	}
#endif
	return is_save;
}

static int32_t _is_cap_use_preview_tbl()
{
	int32_t is_save = 0;
#ifndef WIN32
	char value[PROPERTY_VALUE_MAX];

	property_get(AE_CAP_USE_PREVIEW_TBL, value, "no");

	if (!strcmp(value, "yes")) {
		is_save = 1;
	}
#endif
	return is_save;
}

int save_param_to_file(int32_t sn, uint32_t size, uint8_t *addr)
{
	int ret = 0;
#ifndef WIN32
	char file_name[40];
	char tmp_str[30];
	FILE *fp = NULL;
	uint32_t i = 0;
	int tmp_size = 0;
	int count = 0;

	AE_LOGI("size %d", size);

	memset(file_name,0,sizeof(file_name));
	sprintf(file_name, "%s/ae_param_%d.txt", AE_MLOG_PATH, sn);

	AE_LOGI("file name %s", file_name);
	fp = fopen(file_name, "wb");

	if (NULL == fp) {
		AE_LOGI("can not open file: %s \n", file_name);
		return 0;
	}
	for (i = 0; i < size;++i) {
		count++;
		memset(tmp_str, 0, sizeof(tmp_str));
		sprintf(tmp_str,"0x%02x,", *addr++);
		if (16 == count) {
			strcat(tmp_str,"\n");
			count = 0;
		}

		tmp_size = strlen(tmp_str);
		fwrite((void*)tmp_str, 1, tmp_size, fp);
	}
	fclose(fp);
#endif
	return 0;
}

static int save_ae_tbl_to_file(int32_t sn, int32_t flicker,int32_t iso,
						int32_t scene_mode, struct ae_exp_gain_table *tb_addr)
{
	int ret = 0;
#ifndef WIN32
	char file_name[100];
	char tmp_str[100];
	FILE *fp = NULL;
	int i = 0;
	int tmp_size = 0;
	int count = 0;


	if (NULL == tb_addr) {
		AE_LOGE("tb_addr %p is NULL error", tb_addr);
		return ret;
	}

	memset(file_name,0,sizeof(file_name));
	if (scene_mode) {
		sprintf(file_name, "%s/ae_tbl%d_scene%d_flk%d.txt", AE_MLOG_PATH, sn, scene_mode, flicker);
	} else {
		sprintf(file_name, "%s/ae_tbl%d_auto_flk%d_iso%d.txt", AE_MLOG_PATH, sn, flicker, iso);
	}

	AE_LOGI("file name %s", file_name);
	fp = fopen(file_name, "wb");

	if (NULL == fp) {
		AE_LOGI("can not open file: %s \n", file_name);
		return 0;
	}

	memset(tmp_str, 0, sizeof(tmp_str));
	sprintf(tmp_str, "===start,max(%d,%d)===\n", tb_addr->min_index, tb_addr->max_index);
	tmp_size = strlen(tmp_str);
	fwrite((void*)tmp_str, 1, tmp_size, fp);

	/*shutter*/
	memset(tmp_str, 0, sizeof(tmp_str));
	sprintf(tmp_str, "===shutter===\n\n");
	tmp_size = strlen(tmp_str);
	fwrite((void*)tmp_str, 1, tmp_size, fp);

	for (i = 0; i < AE_EXP_GAIN_TABLE_SIZE; ++i) {
		count++;
		memset(tmp_str, 0, sizeof(tmp_str));
		sprintf(tmp_str,"0x%08x,", tb_addr->exposure[i]);
		if (16 == count) {
			strcat(tmp_str,"\n");
			count = 0;
		}

		tmp_size = strlen(tmp_str);
		fwrite((void*)tmp_str, 1, tmp_size, fp);
	}

	/*again*/
	memset(tmp_str, 0, sizeof(tmp_str));
	sprintf(tmp_str, "\n===again===\n\n");
	tmp_size = strlen(tmp_str);
	fwrite((void*)tmp_str, 1, tmp_size, fp);

	for (i = 0; i < AE_EXP_GAIN_TABLE_SIZE; ++i) {
		count++;
		memset(tmp_str, 0, sizeof(tmp_str));
		sprintf(tmp_str,"0x%08x,", tb_addr->again[i]);
		if (16 == count) {
			strcat(tmp_str,"\n");
			count = 0;
		}

		tmp_size = strlen(tmp_str);
		fwrite((void*)tmp_str, 1, tmp_size, fp);
	}

	/*dummy*/
	memset(tmp_str, 0, sizeof(tmp_str));
	sprintf(tmp_str, "\n===dummy===\n\n");
	tmp_size = strlen(tmp_str);
	fwrite((void*)tmp_str, 1, tmp_size, fp);

	for (i = 0; i < AE_EXP_GAIN_TABLE_SIZE; ++i) {
		count++;
		memset(tmp_str, 0, sizeof(tmp_str));
		sprintf(tmp_str,"0x%08x,", tb_addr->dummy[i]);
		if (16 == count) {
			strcat(tmp_str,"\n");
			count = 0;
		}

		tmp_size = strlen(tmp_str);
		fwrite((void*)tmp_str, 1, tmp_size, fp);
	}

	fclose(fp);
#endif

	return 0;
}

static int save_weight_to_file(int32_t sn, int32_t weight_index, struct ae_weight_table *weight_tbl)
{
	int ret = 0;
#ifndef WIN32
	char file_name[100];
	char tmp_str[100];
	FILE *fp = NULL;
	int i = 0;
	int tmp_size = 0;
	int count = 0;


	if (NULL == weight_tbl) {
		AE_LOGE("weight_tbl %p is NULL error", weight_tbl);
		return ret;
	}

	memset(file_name,0,sizeof(file_name));

	sprintf(file_name, "%s/ae_work%d_weight%d.txt", AE_MLOG_PATH, sn, weight_index);

	AE_LOGI("file name %s", file_name);
	fp = fopen(file_name, "wb");

	if (NULL == fp) {
		AE_LOGI("can not open file: %s \n", file_name);
		return 0;
	}

	/*weight*/
	for (i = 0; i < 1024; ++i) {
		count++;
		memset(tmp_str, 0, sizeof(tmp_str));
		sprintf(tmp_str,"%d,", weight_tbl->weight[i]);
		if (32 == count) {
			strcat(tmp_str,"\n");
			count = 0;
		}

		tmp_size = strlen(tmp_str);
		fwrite((void*)tmp_str, 1, tmp_size, fp);
	}

	fclose(fp);
#endif

	return 0;
}

static int save_auto_iso_to_file(int32_t sn, int32_t flicker, struct ae_auto_iso_tab *iso_tbl)
{
	int ret = 0;
#ifndef WIN32
	char file_name[100];
	char tmp_str[100];
	FILE *fp = NULL;
	int i = 0;
	int tmp_size = 0;
	int count = 0;


	if (NULL == iso_tbl) {
		AE_LOGE("weight_tbl %p is NULL error", iso_tbl);
		return ret;
	}

	memset(file_name,0,sizeof(file_name));

	sprintf(file_name, "%s/ae_work%d_autoiso%d.txt", AE_MLOG_PATH, sn, flicker);

	AE_LOGI("file name %s", file_name);
	fp = fopen(file_name, "wb");

	if (NULL == fp) {
		AE_LOGI("can not open file: %s \n", file_name);
		return 0;
	}

	/*auto iso*/
	for (i = 0; i < AE_EXP_GAIN_TABLE_SIZE; ++i) {
		count++;
		memset(tmp_str, 0, sizeof(tmp_str));
		sprintf(tmp_str,"%04d,", iso_tbl->tbl[flicker][i]);
		if (16 == count) {
			strcat(tmp_str,"\n");
			count = 0;
		}

		tmp_size = strlen(tmp_str);
		fwrite((void*)tmp_str, 1, tmp_size, fp);
	}

	fclose(fp);
#endif

	return 0;
}

static int save_work_ae_to_file(int32_t sn, struct ae_tuning_param *tuning_param)
{
	int ret = 0;
#ifndef WIN32
	char file_name[40];
	char tmp_str[100];
	FILE *fp = NULL;
	int i = 0;
	int tmp_size = 0;
	int count = 0;


	if (NULL == tuning_param) {
		AE_LOGE("tuning_param %p is NULL error", tuning_param);
		return ret;
	}

	//check normal ae
	for (i = 0; i < AE_ISO_NUM; ++i) {
		//50hz
		save_ae_tbl_to_file(sn, 0, i, 0, &tuning_param->ae_table[0][i]);
		//60hz
		save_ae_tbl_to_file(sn, 1, i, 0, &tuning_param->ae_table[1][i]);
	}

	//check scene
	for (i = 0; i < AE_SCENE_NUM; ++i) {
		//50hz
		save_ae_tbl_to_file(sn, 0, i, i+1, &tuning_param->scene_info[i].ae_table[0]);
		//60hz
		save_ae_tbl_to_file(sn, 1, i, i+1,&tuning_param->scene_info[i].ae_table[1]);
	}

	/*auto iso table*/
	save_auto_iso_to_file(sn, 0, &tuning_param->auto_iso_tab);
	save_auto_iso_to_file(sn, 1, &tuning_param->auto_iso_tab);

	/*weight*/
	for (i = 0; i < AE_WEIGHT_MAX; ++i) {
		save_weight_to_file(sn, i, &tuning_param->weight_table[i]);
	}
#endif

	return 0;
}

static int32_t save_to_mlog_file(struct ae_ctrl_context *cxt, int32_t sn, struct ae_misc_calc_out *result)
{
	int32_t rtn = 0;
	UNUSED(sn);
#ifndef WIN32
	char file_name[128] = {0x00};
	int32_t tmp_size = 0;
	char tmp_str[200];
	FILE *s_mlog_fp = NULL;

	sprintf(file_name, "%s/ae.txt", AE_MLOG_PATH);
	s_mlog_fp = fopen(file_name, "wb");
	if (s_mlog_fp) {
		int32_t bv;

		rtn = _get_bv_by_lum(cxt, result, &bv);

		memset(tmp_str, 0, sizeof(tmp_str));
		sprintf(tmp_str,"index:%d, c_lum:%d, t_lum:%d, BV:%d,exp(us*10):%d, again:%d, max_index:%d",
				result->cur_index, result->cur_lum, result->target_lum, bv, result->cur_exposure,
				result->cur_again, result->max_index);
		tmp_size = strlen(tmp_str);

		fwrite((void*)tmp_str, 1, tmp_size, s_mlog_fp);
		fclose(s_mlog_fp);
		s_mlog_fp = NULL;
	} else {
		AE_LOGI("fp is null !!!");
	}
#endif

	return rtn;
}

static int32_t _write_to_sensor(struct ae_ctrl_context *cxt,
                                   struct ae_misc_calc_out *misc_calc_result,
                                   uint32_t is_force)
{
	int32_t rtn = AE_SUCCESS;
	uint32_t is_update_exp = 0;
	uint32_t is_update_gain = 0;
	uint32_t write_exp_eb = 1;
	uint32_t write_gain_eb = 1;
	uint32_t write_exp_sync = 0;
	uint32_t write_gain_sync = 0;
	uint32_t is_write = 0;
	UNUSED(is_force);

	if (NULL == cxt
		|| NULL == misc_calc_result) {
			rtn = AE_PARAM_NULL;
			AE_RETURN_IF_FAIL(rtn, ("cxt:%p, out:%p", cxt, misc_calc_result));
	}

	AE_LOGI("exp_line=%d, again=%d, dgain=%d, dummy=%d cur_index=%d",
		misc_calc_result->cur_exp_line, misc_calc_result->cur_again,
		misc_calc_result->cur_dgain, misc_calc_result->cur_dummy, misc_calc_result->cur_index);

	//AE_LOGE("AE_TEST: -----exp------:%d, %d", misc_calc_result->cur_exp_line, misc_calc_result->cur_again);

	if ((0 != misc_calc_result->cur_again)
			&&(0 != misc_calc_result->cur_exp_line)) {
		is_update_exp = 1;
		if (is_update_exp) {
			struct ae_exposure exp;
			uint32_t ae_exposure = misc_calc_result->cur_exp_line;
			uint32_t dummy = misc_calc_result->cur_dummy;
			uint32_t size_index = cxt->resolution_info.sensor_size_index;

			if (1 == write_exp_eb) {
				memset(&exp, 0, sizeof(exp));
				if (cxt->isp_ops.ex_set_exposure) {
					exp.exposure = ae_exposure;
					exp.dummy = dummy;
					exp.size_index = size_index;
					(*cxt->isp_ops.ex_set_exposure)(cxt->isp_ops.isp_handler, &exp);
				} else if (cxt->isp_ops.set_exposure) {
					ae_exposure = ae_exposure & 0x0000ffff;
					ae_exposure |= (dummy << 0x10) & 0x0fff0000;
					ae_exposure |= (size_index << 0x1c) & 0xf0000000;

					exp.exposure = ae_exposure;
					(*cxt->isp_ops.set_exposure)(cxt->isp_ops.isp_handler, &exp);
				}
				is_write = 1;
			}
		}

		is_update_gain = 1;

		if (cxt->isp_ops.set_again && is_update_gain) {
			struct ae_gain again;
			if (1 == write_gain_eb) {
				memset(&again, 0, sizeof(again));
				again.gain = misc_calc_result->cur_again & 0xffff;

				(*cxt->isp_ops.set_again)(cxt->isp_ops.isp_handler, &again);
				is_write = 1;
			}
		}
	}else {

	}

	//cxt->prv_result.cur_exp_line = misc_calc_result->cur_exp_line;
	//cxt->prv_result.cur_again = misc_calc_result->cur_again;

	return is_write ? AE_SUCCESS : AE_ERROR;
}

static int32_t _write_exp_gain(struct ae_ctrl_context *cxt)
{
	int32_t rtn = AE_SUCCESS;
	uint32_t num = cxt->write_conter;

	AE_LOGE("AE_TEST: -----exp------:num:%d, %d, %d", num, cxt->cur_result.exp.tab[num].exp_line, cxt->cur_result.exp.tab[num].again);

	if (0 != cxt->cur_result.exp.tab[num].exp_line) {

		if (NULL != cxt->isp_ops.set_exposure || NULL != cxt->isp_ops.ex_set_exposure) {
			struct ae_exposure exp;
			uint32_t ae_exposure = cxt->cur_result.exp.tab[num].exp_line;
			uint32_t dummy = cxt->cur_result.exp.tab[num].exp_dummy;
			uint32_t size_index = cxt->resolution_info.sensor_size_index;

			memset(&exp, 0, sizeof(exp));

			if (NULL != cxt->isp_ops.ex_set_exposure) {
				exp.exposure = ae_exposure;
				exp.dummy = dummy;
				exp.size_index = size_index;
				(*cxt->isp_ops.ex_set_exposure)(cxt->isp_ops.isp_handler, &exp);
			} else if (NULL != cxt->isp_ops.set_exposure) {
				ae_exposure = ae_exposure & 0x0000ffff;
				ae_exposure |= (dummy << 0x10) & 0x0fff0000;
				ae_exposure |= (size_index << 0x1c) & 0xf0000000;
				exp.exposure = ae_exposure;
				(*cxt->isp_ops.set_exposure)(cxt->isp_ops.isp_handler, &exp);
			}
		}


		if (NULL != cxt->isp_ops.set_again) {
			struct ae_gain again;

			memset(&again, 0, sizeof(again));
			again.gain = cxt->cur_result.exp.tab[num].again & 0xffff;

			(*cxt->isp_ops.set_again)(cxt->isp_ops.isp_handler, &again);
		}
	}else {
		AE_LOGE("AE_TEST: -----exp error------:num:%d, %d, %d", num, cxt->cur_result.exp.tab[num].exp_line, cxt->cur_result.exp.tab[num].again);
	}

	cxt->write_conter++;

	if (cxt->cur_result.exp.num <= cxt->write_conter) {
		cxt->write_eb = 0;
	}

	return rtn;
}


static int32_t _check_handle(void *handle)
{
	struct ae_ctrl_context *cxt = (struct ae_ctrl_context *)handle;

	if (NULL == handle) {
		AE_LOGE("handle is NULL error!");
		return AE_ERROR;
	}
	if (AE_START_ID != cxt->start_id && AE_END_ID != cxt->end_id) {
		AE_LOGE("tag is error!");
		return AE_ERROR;
	}

	return AE_SUCCESS;
}

static int32_t _unpack_tunning_param(void *param, uint32_t size, struct ae_tuning_param *tuning_param)
{
        uint32_t *tmp = param;
	uint32_t version = 0;
	uint32_t verify = 0;
	UNUSED(size);

	if (NULL == param)
		return AE_ERROR;

	version = *tmp++;
	verify = *tmp++;
	if (AE_PARAM_VERIFY != verify || AE_TUNING_VER != version) {
		AE_LOGE("version param is error(verify=0x%x, version=%d)", verify, version);
		return AE_ERROR;
	}

	memcpy(tuning_param, param, sizeof(struct ae_tuning_param));

	return AE_SUCCESS;
}

static uint32_t _calc_target_lum(uint32_t cur_target_lum, enum ae_level level, struct ae_ev_table *ev_table)
{
	int32_t target_lum = 0;


	if (NULL == ev_table) {
		AE_LOGE("table %p param is error", ev_table);
		return target_lum;
	}
	if (ev_table->diff_num >= AE_EV_LEVEL_NUM)
		ev_table->diff_num = AE_EV_LEVEL_NUM - 1;

	if (level >= ev_table->diff_num)
		level = ev_table->diff_num - 1;

	AE_LOGI("cur target lum=%d, ev diff=%d, level=%d", cur_target_lum, ev_table->lum_diff[level], level);

	target_lum = (int32_t)cur_target_lum + ev_table->lum_diff[level];
	target_lum = (target_lum < 0) ? 0 : target_lum;

	return (uint32_t)target_lum;
}

static void _reset_stat(struct ae_ctrl_context *cxt, uint32_t is_drop_stat)
{
	cxt->valid_stat = 0;
	cxt->skip_frame_cnt = 0;
	cxt->stab_cnt  = 0;
	cxt->is_drop_stat = is_drop_stat;
}

static int32_t _update_monitor_unit(struct ae_ctrl_context *cxt,
                                        struct ae_trim *trim)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_monitor_unit *unit = NULL;


	if (NULL == cxt || NULL == trim) {
		AE_LOGE(" cxt=%p, work_info=%p param is error", cxt, trim);
		return AE_ERROR;
	}
	unit = &cxt->monitor_unit;

	if (unit) {
		unit->win_size.w = ((trim->w / unit->win_num.w) / 2) * 2;
		unit->win_size.h = ((trim->h / unit->win_num.h) / 2) * 2;
		unit->trim.w = unit->win_size.w * unit->win_num.w;
		unit->trim.h = unit->win_size.h * unit->win_num.h;
		unit->trim.x = trim->x + (trim->w - unit->trim.w) / 2;
		unit->trim.x = (unit->trim.x / 2) * 2;
		unit->trim.y = trim->y + (trim->h - unit->trim.h) / 2;
		unit->trim.y = (unit->trim.y / 2) * 2;
	}

	return rtn;
}

static int32_t _get_work_mode_ae_tbl(struct ae_tuning_param *cur_param,
										enum ae_scene_mode scene_index,
										enum ae_flicker_mode flicker_index,
										enum ae_iso_mode iso_index,
										struct ae_exp_gain_table **out_table)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_scene_info *scene_info = NULL;
	struct ae_exp_gain_table *ae_table = NULL;
	uint32_t i = 0;


	if (NULL == cur_param || NULL == out_table) {
		AE_LOGE("cur_param=%p out_table %p param is error",
					cur_param, out_table);
		return AE_ERROR;
	}
	if (flicker_index >= AE_FLICKER_OFF) {
		AE_LOGE("flicke %d is error!", flicker_index);
		rtn = AE_ERROR;
		goto EXIT;
	}
	if (iso_index >= AE_ISO_MAX) {
		AE_LOGE("iso %d is error!", iso_index);
		rtn = AE_ERROR;
		goto EXIT;
	}
	if (scene_index >= AE_SCENE_MAX) {
		AE_LOGE("scene %d is error!", scene_index);
		rtn = AE_ERROR;
		goto EXIT;
	}

	if (AE_SCENE_NORMAL == scene_index) {
		ae_table = &cur_param->ae_table[flicker_index][iso_index];
	} else {
		scene_info = cur_param->scene_info;
		for (i = 0; i < AE_SCENE_MAX; ++i) {
			if (0 == scene_info[i].enable)
				continue;

			if (scene_index == scene_info[i].scene_mode)
				break;
		}
		if (i < AE_SCENE_MAX) {
			uint32_t iso = scene_info[i].iso_index;

			if (iso >= AE_ISO_MAX) {
				AE_LOGE("scene iso %d is error!!!", iso);
				rtn = AE_ERROR;
				goto EXIT;
			}
			ae_table = &scene_info[i].ae_table[flicker_index];
		}
	}
	*out_table = ae_table;
EXIT:
	AE_LOGI("rtn=%d,scene_index=%d, flicker_index=%d, iso_index=%d",
			rtn, scene_index, flicker_index, iso_index);

	return rtn;
}

static int32_t _change_video_work_mode(struct ae_ctrl_context *cxt,
                                               struct ae_set_work_param *work_param)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_misc_work_in misc_work_in = {0};
	struct ae_misc_work_out misc_work_out = {0};
	struct ae_status *cur_status = NULL;
	struct ae_tuning_param *cur_param = NULL;
	uint32_t next_param_enable = 0;
	uint32_t next_work_mode = AE_WORK_MODE_COMMON;
	struct ae_scene_info *scene_info = NULL;
	uint32_t size_index = 0;
	struct ae_exp_gain_table *ae_table = NULL;
	struct ae_weight_table *weight_table = NULL;
	uint32_t iso = 0;
	uint32_t weight_mode = 0;
	uint32_t i = 0;

	if (NULL == cxt || NULL == work_param) {
		AE_LOGE("param is error cxt=%p, work_info=%p", cxt, work_param);
		return AE_ERROR;
	}

	next_work_mode = work_param->mode;
	if (next_work_mode >= AE_WORK_MODE_MAX) {
		AE_LOGE("param is error next_work_mode=%d", next_work_mode);
		return AE_ERROR;
	}

	next_param_enable = cxt->tuning_param_enable[next_work_mode];

	if ((AE_WORK_MODE_VIDEO == next_work_mode) && next_param_enable) {
		size_index = work_param->resolution_info.sensor_size_index;
		if (0 == size_index || size_index >= AE_SCENE_MAX) {
			AE_LOGI("warning size_index=%d", size_index);
			rtn = AE_ERROR;
			goto EXIT;
		}

		cur_param = &cxt->tuning_param[next_work_mode];
		cur_status = &cxt->cur_status;

		scene_info = cur_param->scene_info;

		for (i = 0; i < AE_SCENE_MAX; ++i) {
			if (0 == scene_info[i].enable)
				continue;

			if (size_index == scene_info[i].scene_mode)
				break;
		}

		if (i < AE_SCENE_MAX) {
			if (0 == scene_info[i].enable) {
				AE_LOGI("warning disable index");
				rtn = AE_ERROR;
				goto EXIT;
			}
			if (size_index != scene_info[i].scene_mode) {
				AE_LOGI("warning diff index=%d, scene_mode=%d", size_index, scene_info[i].scene_mode);
				rtn = AE_ERROR;
				goto EXIT;
			}

			iso = scene_info[i].iso_index;
			weight_mode = scene_info[i].weight_mode;

			if (iso >= AE_ISO_MAX || weight_mode >= AE_WEIGHT_MAX) {
				AE_LOGE("error iso=%d, weight_mode=%d", iso, weight_mode);
				rtn = AE_ERROR;
				goto EXIT;
			}

			ae_table = &scene_info[i].ae_table[cur_status->flicker];
			weight_table = &cur_param->weight_table[weight_mode];

			misc_work_in.alg_id = cur_param->alg_id;
			misc_work_in.weight_table = weight_table;
			misc_work_in.ae_table = ae_table;
			misc_work_in.convergence_speed = cur_param->convergence_speed;
			misc_work_in.target_lum = scene_info[i].target_lum;
			misc_work_in.target_lum_zone = cur_param->target_lum_zone;
			misc_work_in.line_time = work_param->resolution_info.line_time;
			misc_work_in.min_line = cur_param->min_line;
			misc_work_in.start_index = 0 == scene_info[i].default_index?
										cur_param->start_index:scene_info[i].default_index;

			if (misc_work_in.start_index < misc_work_in.ae_table->min_index) {
				cxt->start_index = misc_work_in.ae_table->min_index;
			} else {
				cxt->start_index = misc_work_in.start_index;
			}

			rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_WORK_PARAM, &misc_work_in, &misc_work_out);
			if (AE_SUCCESS == rtn) {
				cur_param->start_index = misc_work_in.start_index;
				cxt->cur_param = cur_param;
				cxt->prv_status = *cur_status;
				cur_status->work_info = *work_param;
				cxt->cur_status.param_from = next_work_mode;
				cxt->cur_status.iso = iso;
				cxt->cur_status.weight = weight_mode;
			} else {
				AE_LOGE("change video work mode failed!");
			}
		}else {
			AE_LOGI("warning do not find size index=%d, scene_mode=%d",
					size_index, scene_info[0].scene_mode);
		}
	}else {
		AE_LOGI("warning params are mode=%d, enable=%d", next_work_mode,next_param_enable);
	}


EXIT:

	return rtn;
}

static int32_t _check_cvgn_param(struct ae_misc_cvgn_param *misc_cvgn_param)
{
	uint32_t i = 0;
	if(NULL == misc_cvgn_param || NULL == misc_cvgn_param->cvgn_param){
		return -1;
	}

	if(0 == misc_cvgn_param->cvgn_param[0].highcount
		&& 0 == misc_cvgn_param->cvgn_param[0].lowcount){
		static uint32_t highcount0 = 19;
		static uint32_t lowcount0 = 15;
		static uint32_t highlum_offset_default0[] = {
			3,10,20,30,40,
			50,60,70,80,90,
			100,110,120,130,140,
			150,160,170,180
		};
		static uint32_t highlum_index0[] = {
			1,2,4,8,10,
			12,14,16,18,20,
			22,24,30,40,55,
			60,65,80,80
		};
		static uint32_t lowlum_offset_default0[] = {3,10,15,20,24,28,32,38,45,50,55,60,65,70,75};
		static uint32_t lowlum_index0[] = {1,3,5,8,10,14,16,32,36,44,50,58,80,100,120};
		//static uint32_t lowlum_index0[] = {1,4,12,18,26,30,34,38,42,46,54,64,88,100,120};
		AE_LOGI("cvgn_param[0] error!!!  set default cvgn_param");

		i = 0;
		misc_cvgn_param->cvgn_param[i].highcount = highcount0;
		misc_cvgn_param->cvgn_param[i].lowcount = lowcount0;
		memcpy((void*)&misc_cvgn_param->cvgn_param[i].highlum_offset_default, (void*)&highlum_offset_default0, sizeof(highlum_offset_default0));
		memcpy((void*)&misc_cvgn_param->cvgn_param[i].lowlum_offset_default, (void*)&lowlum_offset_default0, sizeof(lowlum_offset_default0));
		memcpy((void*)&misc_cvgn_param->cvgn_param[i].highlum_index, (void*)&highlum_index0, sizeof(highlum_index0));
		memcpy((void*)&misc_cvgn_param->cvgn_param[i].lowlum_index, (void*)&lowlum_index0, sizeof(lowlum_index0));
	}

	if(0 == misc_cvgn_param->cvgn_param[1].highcount
		&& 0 == misc_cvgn_param->cvgn_param[1].lowcount){

		static uint32_t highcount = 19;
		static uint32_t lowcount = 15;

		static uint32_t highlum_offset_default[] = {
			3,10,20,30,40,
			50,60,70,80,90,
			100,110,120,130,140,
			150,160,170,180
		};
		static uint32_t highlum_index[] = {
			1,4,12,18,26,
			30,34,38,42,46,
			50,54,58,62,64,
			66,70,75,80
		};
		static uint32_t lowlum_offset_default[] = {3,10,15,20,24,28,32,38,45,50,55,60,65,70,75};
		static uint32_t lowlum_index[] = {1,4,12,18,26,30,34,38,42,46,54,64,88,100,120};

		i = 1;
		misc_cvgn_param->cvgn_param[i].highcount = highcount;
		misc_cvgn_param->cvgn_param[i].lowcount = lowcount;
		memcpy((void*)&misc_cvgn_param->cvgn_param[i].highlum_offset_default, (void*)&highlum_offset_default, sizeof(highlum_offset_default));
		memcpy((void*)&misc_cvgn_param->cvgn_param[i].lowlum_offset_default, (void*)&lowlum_offset_default, sizeof(lowlum_offset_default));
		memcpy((void*)&misc_cvgn_param->cvgn_param[i].highlum_index, (void*)&highlum_index, sizeof(highlum_index));
		memcpy((void*)&misc_cvgn_param->cvgn_param[i].lowlum_index, (void*)&lowlum_index, sizeof(lowlum_index));
	}

	return 0;
}

static int32_t _set_work_mode(struct ae_ctrl_context *cxt,
                                   struct ae_set_work_param *work_param,
                                   uint32_t is_force)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_misc_work_in misc_work_in = {0};
	struct ae_misc_work_out misc_work_out = {0};
	struct ae_status *cur_status = NULL;
	struct ae_tuning_param *cur_param = NULL;
	struct ae_misc_cvgn_param misc_cvgn_param = {0};
	uint32_t cur_work_mode = AE_WORK_MODE_COMMON;
	uint32_t cur_param_from = AE_WORK_MODE_COMMON;
	uint32_t next_param_enable = 0;
	uint32_t next_param_from = AE_WORK_MODE_COMMON;
	uint32_t next_work_mode = AE_WORK_MODE_COMMON;


	if (NULL == cxt || NULL == work_param) {
		AE_LOGE("param is error cxt=%p, work_info=%p", cxt, work_param);
		return AE_ERROR;
	}

	next_work_mode = work_param->mode;
	if (next_work_mode >= AE_WORK_MODE_MAX) {
		AE_LOGE("param is error next_work_mode=%d", next_work_mode);
		return AE_ERROR;
	}

	next_param_from = next_work_mode;
	next_param_enable = cxt->tuning_param_enable[next_work_mode];

	if ((AE_WORK_MODE_VIDEO != next_work_mode)
		|| (AE_WORK_MODE_VIDEO == next_work_mode && 0 == next_param_enable)) {
		cur_status = &cxt->cur_status;
		cur_work_mode = cur_status->work_info.mode;
		cur_param_from = cur_status->param_from;

		if (!is_force && cur_work_mode == next_work_mode) {
			rtn = AE_SUCCESS;
			goto EXIT;
		}

		if (0 == next_param_enable)
			next_param_from = AE_WORK_MODE_COMMON;
		else
			next_param_from = AE_WORK_MODE_COMMON;

		if (!is_force && cur_param_from == next_param_from) {
			rtn = AE_SUCCESS;
			goto EXIT;
		}

		cur_param = &cxt->tuning_param[next_param_from];

		misc_work_in.alg_id = cur_param->alg_id;
		misc_work_in.weight_table = &cur_param->weight_table[cur_status->weight];
		misc_work_in.ae_table = &cur_param->ae_table[cur_status->flicker][cur_status->iso];
		misc_work_in.convergence_speed = cur_param->convergence_speed;
		misc_work_in.target_lum = cur_status->target_lum;
		misc_work_in.target_lum_zone = cur_param->target_lum_zone;
		misc_work_in.line_time = work_param->resolution_info.line_time;
		misc_work_in.min_line = cur_param->min_line;
		misc_work_in.start_index = cur_param->start_index;

		if (misc_work_in.start_index < misc_work_in.ae_table->min_index) {
			cxt->start_index = misc_work_in.ae_table->min_index;
		} else {
			cxt->start_index = misc_work_in.start_index;
		}

		rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_WORK_PARAM, &misc_work_in, &misc_work_out);
		if (AE_SUCCESS == rtn) {
			cxt->cur_param = cur_param;
			cxt->prv_status = *cur_status;
			cur_status->work_info = *work_param;
			cur_status->scene = AE_SCENE_NORMAL;
			cxt->cur_status.param_from = next_param_from;
		} else {
			AE_LOGE("change work mode failed!");
		}
	} else {
		rtn = _change_video_work_mode(cxt, work_param);
	}

	if(NULL != cxt->cur_param){
		misc_cvgn_param.cvgn_param = cxt->cur_param->cvgn_param;
		misc_cvgn_param.target_lum = cxt->cur_param->target_lum;
		_check_cvgn_param(&misc_cvgn_param);
		ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_CVGN_PARAM, &misc_cvgn_param, NULL);
	}else{
		AE_LOGE("cur_param error, can not set cvgn_param");
	}
EXIT:
	/*real & cur work mode are diff*/
	cxt->cur_status.real_work_mode = next_work_mode;

	return rtn;
}

static int32_t _set_scene_mode(struct ae_ctrl_context *cxt, enum ae_scene_mode scene_mode)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_tuning_param *cur_param = NULL;
	struct ae_scene_info *scene_info = NULL;
	struct ae_status *cur_status = NULL;
	struct ae_status *prv_status = NULL;
	struct ae_set_fps fps_param;
	uint32_t i = 0;
	uint32_t target_lum = 0;

	if (scene_mode >= AE_SCENE_MAX)
		return AE_ERROR;

	cur_param = cxt->cur_param;
	scene_info = cur_param->scene_info;
	cur_status = &cxt->cur_status;
	prv_status = &cxt->prv_status;

	for (i = 0; i < AE_SCENE_MAX; ++i) {

		if (0 == scene_info[i].enable)
			continue;

		if (scene_mode == scene_info[i].scene_mode)
			break;
	}

	if ((AE_SCENE_NORMAL == scene_mode) && (AE_SCENE_NORMAL == cur_status->scene)) {
		goto EXIT;
	}

	if ((i < AE_SCENE_MAX) && (AE_SCENE_NORMAL != scene_mode)) {
		struct ae_exp_gain_table *ae_table = NULL;
		struct ae_weight_table *weight_table = NULL;
		uint32_t iso = scene_info[i].iso_index;
		uint32_t weight_mode = scene_info[i].weight_mode;

		if (iso >= AE_ISO_MAX || weight_mode >= AE_WEIGHT_MAX) {
			AE_LOGE("error iso=%d, weight_mode=%d", iso, weight_mode);
			rtn = AE_ERROR;
			goto EXIT;
		}

		if (AE_SCENE_NORMAL == cur_status->scene) {
			cxt->prv_status = *cur_status;
		}

		/*ae table*/
		ae_table = &scene_info[i].ae_table[cur_status->flicker];
		rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_AE_TABLE,
						(void *)ae_table, NULL);
		if (AE_SUCCESS != rtn) {
			AE_LOGE("set ae table rtn=%d is error!", rtn);
			goto EXIT;
		}
		cur_status->iso = iso;

		/*weight table*/
		if (AE_SCENE_NORMAL != scene_mode) {
			weight_table = &cur_param->weight_table[weight_mode];
			rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_WEIGHT_TABLE,
							(void *)weight_table, NULL);
			if (AE_SUCCESS != rtn) {
				AE_LOGE("set weight table rtn=%d is error!", rtn);
				goto EXIT;
			}
			cur_status->weight = weight_mode;
		}
		/*fps*/
		fps_param.min_fps = scene_info[i].min_fps;
		fps_param.max_fps = scene_info[i].max_fps;
		rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_FPS, &fps_param, NULL);
		if (AE_SUCCESS == rtn) {
			cxt->cur_status.fps = fps_param;
		}
		/*ev & target lum */
		target_lum = _calc_target_lum(scene_info[i].target_lum,
									scene_info[i].ev_offset,
									&cxt->cur_param->ev_table);
		_set_target_lum(cxt, target_lum);
		/*index */
		{
			uint32_t new_index = 0;
			uint32_t cur_exp_gain = 0;
			uint32_t exp_line = 0;
			uint32_t gain = 0;

			exp_line = cxt->cur_result.cur_exp_line;
			gain = cxt->cur_result.cur_again;
			cur_exp_gain = exp_line * _get_ae0_real_gain(gain);
			rtn = _get_new_index(cxt, cur_exp_gain, &new_index);
			if (AE_SUCCESS != rtn) {
				goto EXIT;
			}
			rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_INDEX, (void *)&new_index, NULL);
			cxt->prv_index = new_index;

			rtn = ae_misc_io_ctrl(cxt->misc_handle,
						AE_MISC_CMD_GET_EXP_BY_INDEX,
						(void *)&new_index,
						&cxt->cur_result);
			if (AE_SUCCESS == rtn) {
				_write_to_sensor(cxt, &cxt->cur_result, 1);
			}
		}

		cur_status->scene = scene_mode;
	} else {
		struct ae_exp_gain_table *ae_table = NULL;
		struct ae_weight_table *weight_table = NULL;
		uint32_t iso = prv_status->iso;
		uint32_t weight_mode = prv_status->weight;

		if (iso >= AE_ISO_MAX || weight_mode >= AE_WEIGHT_MAX) {
			AE_LOGE("error iso=%d, weight_mode=%d", iso, weight_mode);
		rtn = AE_ERROR;
			goto EXIT;
		}
		/*ae table*/
		ae_table = &cur_param->ae_table[cxt->cur_status.flicker][iso];
		rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_AE_TABLE,
						(void *)ae_table, NULL);
		if (AE_SUCCESS != rtn) {
			AE_LOGE("set ae table rtn=%d is error!", rtn);
			goto EXIT;
		}
		cur_status->iso = iso;

		/*weight table*/
		weight_table = &cur_param->weight_table[weight_mode];
		rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_WEIGHT_TABLE,
						(void *)weight_table, NULL);
		if (AE_SUCCESS != rtn) {
			AE_LOGE("set weight table rtn=%d is error!", rtn);
			goto EXIT;
		}
		cur_status->weight = weight_mode;
		/*fps*/
		fps_param.min_fps = prv_status->fps.min_fps;
		fps_param.max_fps = prv_status->fps.max_fps;
		rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_FPS, &fps_param, NULL);
		if (AE_SUCCESS == rtn) {
			cxt->cur_status.fps = fps_param;
		}
		/*ev & target lum */
		_set_target_lum(cxt, prv_status->target_lum);
		/*index */
		{
			uint32_t new_index = 0;
			uint32_t cur_exp_gain = 0;
			uint32_t exp_line = 0;
			uint32_t gain = 0;

			exp_line = cxt->cur_result.cur_exp_line;
			gain = cxt->cur_result.cur_again;
			cur_exp_gain = exp_line * _get_ae0_real_gain(gain);
			rtn = _get_new_index(cxt, cur_exp_gain, &new_index);
			if (AE_SUCCESS != rtn) {
				goto EXIT;
			}
			rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_INDEX, (void *)&new_index, NULL);
			cxt->prv_index = new_index;

			rtn = ae_misc_io_ctrl(cxt->misc_handle,
						AE_MISC_CMD_GET_EXP_BY_INDEX,
						(void *)&new_index,
						&cxt->cur_result);
			if (AE_SUCCESS == rtn) {
				_write_to_sensor(cxt, &cxt->cur_result, 1);
			}
		}

		cur_status->scene = AE_SCENE_NORMAL;
		AE_LOGE("scene Set to Normal, scene set = %d",scene_mode);
	}

EXIT:
	AE_LOGI("change scene mode %d, rtn=%d", cur_status->scene, rtn);

	return rtn;
}
static int32_t _set_night_mode(struct ae_ctrl_context *cxt, uint32_t night_mode)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_tuning_param *cur_param = NULL;
	struct ae_scene_info *scene_info = NULL;
	struct ae_status *cur_status = NULL;
	uint32_t i = 0;
	uint32_t new_index = 0;
	uint32_t cur_exp_gain = 0;
	uint32_t exp_line = 0;
	uint32_t gain = 0;


	cur_param = cxt->cur_param;
	scene_info = cur_param->scene_info;
	cur_status = &cxt->cur_status;


	if (night_mode) {
		for (i = 0; i < AE_SCENE_MAX; ++i) {
			if (AE_SCENE_NIGHT == scene_info[i].scene_mode)
				break;
		}

		if (i < AE_SCENE_MAX) {
			struct ae_exp_gain_table *ae_table = NULL;
			/*ae table*/
			ae_table = &scene_info[i].ae_table[cur_status->flicker];
			rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_AE_TABLE,
							(void *)ae_table, NULL);
			if (AE_SUCCESS != rtn) {
				AE_LOGE("set ae table rtn=%d is error!", rtn);
				goto EXIT;
			}

			exp_line = cxt->cur_result.cur_exp_line;
			gain = cxt->cur_result.cur_again;
			cur_exp_gain = exp_line * _get_ae0_real_gain(gain);
			rtn = _get_new_index(cxt, cur_exp_gain, &new_index);
			if (AE_SUCCESS != rtn) {
				goto EXIT;
			}
			rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_INDEX, (void *)&new_index, NULL);
			cur_status->scene = AE_SCENE_NIGHT;
			cxt->prv_index = new_index;

			rtn = ae_misc_io_ctrl(cxt->misc_handle,
						AE_MISC_CMD_GET_EXP_BY_INDEX,
						(void *)&new_index,
						&cxt->cur_result);
			if (AE_SUCCESS == rtn) {
				_write_to_sensor(cxt, &cxt->cur_result, 1);
			}
		} else {
			rtn = AE_ERROR;
			AE_LOGE("scene AE_SCENE_NIGHT is error!");
		}

	} else {
		rtn = _set_iso(cxt,cur_status->iso);
		cur_status->scene = AE_SCENE_NORMAL;
	}

	cur_status->night_mode = night_mode;


EXIT:
	AE_LOGI("change night mode %d, rtn=%d", cur_status->night_mode, rtn);


	return rtn;
}

static int32_t _get_new_index(struct ae_ctrl_context *cxt,
								uint32_t cur_exp_gain,
								uint32_t *new_index)
{
	int32_t rtn = AE_SUCCESS;

	if (NULL == cxt || NULL == new_index) {
		AE_LOGE("cxt%p index %p param is error!", cxt, new_index);
		return AE_ERROR;
	}
	rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_GET_NEW_INDEX,
	                      (void *)&cur_exp_gain, new_index);

	return rtn;
}

static int32_t _get_ae0_real_gain(uint32_t gain)
{// x=real_gain/32
	uint32_t real_gain = 0;
	uint32_t cur_gain = 0x00;
	uint32_t i = 0x00;
#ifdef AE_TABLE_32
	real_gain = gain>>2; // /128*32;
#else
	cur_gain = (gain>>0x04) & 0xfff;
	real_gain = ((gain & 0x0f)<<0x01) + 0x20;

	for (i = 0x00; i < 11; ++i) {
		if (0x01 == (cur_gain & 0x01)) {
			real_gain*=0x02;
		}
		cur_gain>>=0x01;
	}
#endif
	return real_gain;
}

static int32_t _set_iso(struct ae_ctrl_context *cxt, enum ae_iso_mode iso)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_exp_gain_table *ae_table = NULL;
	uint32_t new_index = 0;
	uint32_t cur_exp_gain = 0;
	uint32_t exp_line = 0;
	uint32_t gain = 0;

	if (iso >= AE_ISO_MAX)
		return AE_ERROR;

	ae_table = &cxt->cur_param->ae_table[cxt->cur_status.flicker][iso];
	rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_AE_TABLE, (void *)ae_table, NULL);
	AE_LOGI("set iso from %d to %d, rtn=%d", cxt->cur_status.iso, iso, rtn);
	if (AE_SUCCESS != rtn) {
		goto EXIT;
	}
	cxt->cur_status.iso = iso;

	exp_line = cxt->cur_result.cur_exp_line;
	gain = cxt->cur_result.cur_again;
	cur_exp_gain = exp_line * _get_ae0_real_gain(gain);
	rtn = _get_new_index(cxt, cur_exp_gain, &new_index);
	if (AE_SUCCESS != rtn) {
		goto EXIT;
	}
	rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_INDEX, (void *)&new_index, NULL);
	cxt->prv_index = new_index;

	rtn = ae_misc_io_ctrl(cxt->misc_handle,
				AE_MISC_CMD_GET_EXP_BY_INDEX,
				(void *)&new_index,
				&cxt->cur_result);
	if (AE_SUCCESS == rtn) {
		_write_to_sensor(cxt, &cxt->cur_result, 1);
	}

EXIT:

	return rtn;
}

static int32_t _get_auto_iso_tab(struct ae_ctrl_context *cxt, uint16_t **iso_tab)
{
	int32_t rtn = AE_SUCCESS;
	enum ae_flicker_mode cur_flicker = 0;;


	if (NULL == cxt || NULL == iso_tab) {
		AE_LOGE("cxt %p iso_tab %p param is error!", cxt, iso_tab);
		return AE_ERROR;
	}

	cur_flicker = cxt->cur_status.flicker;
	*iso_tab = (uint16_t*)&cxt->cur_param->auto_iso_tab.tbl[cur_flicker];

	return rtn;
}

static int32_t _get_auto_iso(struct ae_ctrl_context *cxt, uint32_t cur_index, uint32_t *real_iso)
{
	int32_t rtn = AE_SUCCESS;
	uint16_t *iso_tab = 0;

	if (NULL == cxt || NULL == real_iso) {
		AE_LOGE("cxt %p real_iso %p param is error!", cxt, real_iso);
		return AE_ERROR;
	}

	 rtn = _get_auto_iso_tab(cxt, &iso_tab);
	if (AE_SUCCESS == rtn && iso_tab) {
		*real_iso = iso_tab[cur_index];
	}

	AE_LOGI("real_iso=%d", *real_iso);

	return rtn;
}

static int32_t _get_iso(struct ae_ctrl_context *cxt, uint32_t *real_iso)
{
	int32_t rtn = AE_SUCCESS;
	uint32_t iso = 0;
	uint32_t cur_index = 0;
	uint32_t calc_iso = 0;
	uint32_t real_gain = 0;

	if (NULL == cxt || NULL == real_iso) {
		AE_LOGE("cxt %p real_iso %p param is error!", cxt, real_iso);
		return AE_ERROR;
	}

	iso = cxt->cur_status.iso;
	cur_index = cxt->cur_result.cur_index;

	if (AE_ISO_AUTO == iso) {
		#if USE_ISO_TBL
		_get_auto_iso(cxt, cur_index, &calc_iso);
		#else
		real_gain = _get_real_gain(cxt->cur_result.cur_again);
		#if SEGMENTED_ISO
		if (0 == cxt->camera_id) {
			if( real_gain <= 20 ){
				calc_iso = 50;
			}else if( real_gain <= 26 ){
				calc_iso = 64;
			}else if( real_gain <= 32 ){
				calc_iso = 80;
			}else if( real_gain <= 40 ){
				calc_iso = 100;
			}else if( real_gain <= 51 ){
				calc_iso = 125;
			}else if( real_gain <= 64 ){
				calc_iso = 160;
			}else if( real_gain <= 80 ){
				calc_iso = 200;
			}else if( real_gain <= 102 ){
				calc_iso = 250;
			}else if( real_gain <= 128 ){
				calc_iso = 320;
			}else if( real_gain <= 160 ){
				calc_iso = 400;
			}else if( real_gain <= 205 ){
				calc_iso = 500;
			}else if( real_gain <= 256 ){
				calc_iso = 640;
			}else if( real_gain <= 320){
				calc_iso = 800;
			}else if( real_gain <= 400){
				calc_iso = 1000;
			}else{
				calc_iso = 1250;
			}
		} else {
			if( real_gain <= 32 ){
				calc_iso = 50;
			}else if( real_gain <= 48 ){
				calc_iso = 100;
			}else if( real_gain <= 64 ){
				calc_iso = 150;
			}else if( real_gain <= 80 ){
				calc_iso = 200;
			}else if( real_gain <= 96 ){
				calc_iso = 250;
			}else if( real_gain <= 112 ){
				calc_iso = 300;
			}else if( real_gain <= 128 ){
				calc_iso = 350;
			}else if( real_gain <= 160 ){
				calc_iso = 400;
			}else if( real_gain <= 192 ){
				calc_iso = 500;
			}else if( real_gain <= 224 ){
				calc_iso = 600;
			}else if( real_gain <= 256 ){
				calc_iso = 700;
			}else{
				calc_iso = 800;
			}
		}
		#else
		calc_iso = (real_gain*50/16)/10*10;
		#endif
		#endif
	} else {
		calc_iso = (1<<(iso-1)) * 100;
	}
	*real_iso = calc_iso;
	AE_LOGI("calc_iso=%d,real_gain=%d,iso=%d", calc_iso,real_gain,iso);

	return rtn;
}

static int32_t _get_bv_tab(struct ae_ctrl_context *cxt, void **bv_info)
{
	int32_t rtn = AE_SUCCESS;
	enum ae_scene_mode cur_scene;

	if ((NULL == cxt )||(NULL == bv_info )) {
		AE_LOGE("cxt %p vb_info %p param is error!", cxt, bv_info);
		return AE_ERROR;
	}

	cur_scene = cxt->cur_status.scene;

	*bv_info = &cxt->cur_param->ev_cali;

	return rtn;
}

static int32_t _get_max_bv(struct ae_ctrl_context *cxt, void *bv_info, uint32_t *bv)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_ev_cali *bv_cali = bv_info;
	enum ae_flicker_mode cur_flicker;
	enum ae_iso_mode cur_iso;

	uint32_t cur_exposure;
	uint32_t cur_again;
	uint32_t cur_dgain;

	uint32_t min_exposure;
	uint32_t min_again;
	uint32_t min_dgain;

	uint32_t min_index;
	uint32_t max_lux;
	uint32_t cur_index;
	uint32_t bv_index;

	if (NULL == cxt || NULL == bv_info) {
		AE_LOGE("cxt %p bv %p param is error!", cxt, bv_info);
		return AE_ERROR;
	}

	/********************/
	cur_flicker = cxt->cur_status.flicker;
	cur_iso = cxt->cur_status.iso;
	min_index = cxt->cur_result.min_index;
	cur_index = cxt->cur_result.cur_index;
	bv_index = 0;
	max_lux = 0;

	if (min_index ==  bv_cali->tab[bv_index].index) {
		max_lux = bv_cali->tab[bv_index].lux;
	} else {
		cur_flicker = cxt->cur_status.flicker;
		cur_iso = cxt->cur_status.iso;

		cur_exposure = cxt->cur_param->ae_table[cur_flicker][cur_iso].exposure[cur_index];
		cur_again = cxt->cur_param->ae_table[cur_flicker][cur_iso].again[cur_index];
		cur_again = _get_real_gain(cur_again);
		cur_dgain = 1;//cxt->tuning_param[cur_scene].ae_table[cur_flicker][cur_iso].dgain[cur_index];

		min_exposure = cxt->cur_param->ae_table[cur_flicker][cur_iso].exposure[min_index];
		min_again = cxt->cur_param->ae_table[cur_flicker][cur_iso].again[min_index];
		min_again = _get_real_gain(min_again);
		min_dgain = 1;//cxt->tuning_param[cur_scene].ae_table[cur_flicker][cur_iso].dgain[cur_index];
#ifdef AE_TABLE_32
		max_lux = (bv_cali->tab[bv_index].lux * cur_exposure * cur_again * cur_dgain)/min_exposure/min_again/min_dgain;
#else
		max_lux = (bv_cali->tab[bv_index].lux * min_exposure * cur_again * cur_dgain)/cur_exposure/min_again/min_dgain;
#endif
	}


	*bv = max_lux;
	AE_LOGI("max_lux=%d", max_lux);

	return rtn;
}

static int32_t _get_min_bv(struct ae_ctrl_context *cxt, void *bv_info, uint32_t *bv)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_ev_cali *bv_cali = bv_info;
	enum ae_flicker_mode cur_flicker;
	enum ae_iso_mode cur_iso;

	uint32_t cur_exposure;
	uint32_t cur_again;
	uint32_t cur_dgain;

	uint32_t max_exposure;
	uint32_t max_again;
	uint32_t max_dgain;

	uint32_t max_index = 255; //remove coverity warning, will be changed later
	uint32_t min_lux;
	uint32_t cur_index;
	uint32_t bv_index;

	if (NULL == cxt || NULL == bv_info) {
		AE_LOGE("cxt %p bv %p param is error!", cxt, bv_info);
		return AE_ERROR;
	}

	/********************/
	bv_index = bv_cali->num - 1;
	min_lux = 0;

	if (max_index ==  bv_cali->tab[bv_index].index) {
		min_lux = bv_cali->tab[bv_index].lux;
	} else if (max_index >  bv_cali->tab[bv_index].index){
		cur_flicker = cxt->cur_status.flicker;
		cur_iso = cxt->cur_status.iso;
		max_index = cxt->cur_result.max_index;
		cur_index = cxt->cur_result.cur_index;

		cur_exposure = cxt->cur_param->ae_table[cur_flicker][cur_iso].exposure[cur_index];
		cur_again = cxt->cur_param->ae_table[cur_flicker][cur_iso].again[cur_index];
		cur_again = _get_real_gain(cur_again);
		cur_dgain = 1;//cxt->tuning_param[cur_scene].ae_table[cur_flicker][cur_iso].dgain[cur_index];

		max_exposure = cxt->cur_param->ae_table[cur_flicker][cur_iso].exposure[max_index];
		max_again = cxt->cur_param->ae_table[cur_flicker][cur_iso].again[max_index];
		max_again = _get_real_gain(max_again);
		max_dgain = 1;//cxt->tuning_param[cur_scene].ae_table[cur_flicker][cur_iso].dgain[cur_index];
#ifdef AE_TABLE_32
		min_lux = (bv_cali->tab[bv_index].lux * cur_exposure * cur_again * cur_dgain)/max_exposure/max_again/max_dgain;
#else
		min_lux = (bv_cali->tab[bv_index].lux * max_exposure * cur_again * cur_dgain)/cur_exposure/max_again/max_dgain;
#endif
	}


	*bv = min_lux;
	AE_LOGI("min_lux=%d", min_lux);

	return rtn;
}

static int32_t _calc_bv(struct ae_ctrl_context *cxt, void *bv_info, uint32_t *bv)
{
	int32_t rtn = AE_SUCCESS;
	uint32_t iso = 0;

	uint32_t max_index = 0;
	uint32_t iso_level = 0;

	struct ae_ev_cali *bv_cali = bv_info;

	uint32_t calc_bv = 0;
	struct ae_ev_cali_param cali_low;
	struct ae_ev_cali_param cali_high;
	struct ae_misc_calc_out ae_info;

	uint32_t cur_index;

	uint32_t cur_lum;
	uint32_t min_lum;
	uint32_t target_lum;

	uint32_t bv_index = 0;

	uint32_t max_lux = 0;
	uint32_t min_lux = 0;


	if (NULL == cxt || NULL == bv_info) {
		AE_LOGE("cxt %p bv %p param is error!", cxt, bv_info);
		return AE_ERROR;
	}

	memcpy((void*)&ae_info, (void*)&cxt->cur_result, sizeof(struct ae_misc_calc_out));

	cur_index = cxt->cur_result.cur_index;
	_get_max_bv(cxt, bv_info, &max_lux);
	_get_min_bv(cxt, bv_info, &min_lux);

	/**********get cali info**********/
	for(bv_index = 0;  bv_index < bv_cali->num; bv_index++) {
		if (cur_index >= bv_cali->tab[bv_index].index) {
			if (0 == bv_index) {
				/*max index stage handle*/
				cali_low.index = ae_info.max_index;
				cali_low.lux =  min_lux;
				cali_low.lv = 0;
			} else {
				cali_low.index =  bv_cali->tab[bv_index - 1].index;
				cali_low.lux =  bv_cali->tab[bv_index - 1].lux;
				cali_low.lv =  bv_cali->tab[bv_index - 1].lv;
			}
			cali_high.index =  bv_cali->tab[bv_index].index;
			cali_high.lux  =  bv_cali->tab[bv_index].lux;
			cali_high.lv=  bv_cali->tab[bv_index].lv;
			break ;
		} else if (cur_index < bv_cali->tab[bv_cali->num - 1].index) {
			/*min index stage handle*/
			cali_low.index = bv_cali->tab[bv_index].index;
			cali_low.lux = bv_cali->tab[bv_index].lux;
			cali_low.lv = bv_cali->tab[bv_index].lv;

			cali_high.index = ae_info.max_index;
			cali_high.lux = min_lux;
			cali_high.lv = 0;
		}
	}

	if (ae_info.cur_index == ae_info.min_index) {
	/*ae cur index equal to ae min index, high light*/

		calc_bv = max_lux;
	} else if (ae_info.cur_index < ae_info.max_index) {
	/*ae cur index more than ae min index and  less than ae max index, normal light*/
		int32_t lux_delt;
		int32_t index_delt;
		int32_t index_len;

		lux_delt = cali_high.lux - cali_low.lux;
		index_delt = ae_info.cur_index - cali_high.index;
		index_len = cali_low.index - cali_high.index;

		if ((0 >= lux_delt)
			|| (0 >= index_delt)
			|| (0 >= index_len)) {
			calc_bv = cali_high.lux;
		} else {
			calc_bv = cali_low.lux + (lux_delt * index_delt / index_len);
		}

	} else if (ae_info.cur_index == ae_info.max_index) {
	/*ae cur index equal to ae max index, low light*/

		/*check target lum whither less than bv min lum*/
		if (ae_info.target_lum < bv_cali->min_lum) {
			target_lum = bv_cali->min_lum;
		} else {
			target_lum = ae_info.target_lum;
		}

		if (ae_info.cur_lum <= bv_cali->min_lum) {
			calc_bv = 0;
		} else if (ae_info.cur_lum < target_lum) {
			calc_bv = min_lux * ae_info.cur_lum/target_lum;
		} else {
			calc_bv = min_lux;
		}
	}

	*bv = calc_bv;
	AE_LOGI("calc_bv=%d", calc_bv);

	return rtn;
}

static int32_t _get_BV(struct ae_ctrl_context *cxt, uint32_t *bv)
{
	int32_t rtn = AE_SUCCESS;
	uint32_t iso = 0;
	uint32_t cur_index = 0;
	uint32_t max_index = 0;
	uint32_t iso_level = 0;

	uint32_t calc_bv = 0;


	if (NULL == cxt || NULL == bv) {
		AE_LOGE("cxt %p bv %p param is error!", cxt, bv);
		return AE_ERROR;
	}

	iso = cxt->cur_status.iso;
	cur_index = cxt->cur_result.cur_index;
	max_index = cxt->cur_result.max_index;


	*bv = calc_bv;
	AE_LOGI("calc_bv=%d", calc_bv);

	return rtn;
}

static int32_t _set_target_lum(struct ae_ctrl_context *cxt, uint32_t lum)
{
	int32_t rtn = AE_SUCCESS;
	uint32_t target_lum = lum;

	rtn = ae_misc_io_ctrl(cxt->misc_handle,
                          AE_MISC_CMD_SET_TARGET_LUM,
                          (void *)&target_lum, NULL);

	if (AE_SUCCESS == rtn) {
		cxt->cur_status.target_lum = target_lum;
	}

	return rtn;
}

static int32_t _set_ev_level(struct ae_ctrl_context *cxt, enum ae_level level)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_ev_table *ev_table = &cxt->cur_param->ev_table;
	uint32_t target_lum = 0;
	uint32_t ev_level = level;

	target_lum = _calc_target_lum(cxt->cur_param->target_lum, level, ev_table);

	rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_EV, (void *)&ev_level, NULL);

	rtn = _set_target_lum(cxt, target_lum);
	AE_LOGI("set ev level %d, target lum from %d to %d, rtn=%d", level,
				cxt->cur_status.target_lum, target_lum, rtn);
	if (AE_SUCCESS == rtn) {
		cxt->cur_status.ev_level = level;
	}


	return rtn;
}

static void convert_trim_to_rect(struct ae_trim *trim, struct ae_rect *rect)
{
	rect->start_x = trim->x;
	rect->start_y = trim->y;
	rect->end_x = rect->start_x + trim->w - 1;
	rect->end_y = rect->start_y + trim->h - 1;

	if ((int32_t)(rect->end_x) < 0)
		rect->end_x = 0;
	if ((int32_t)(rect->end_y) < 0)
		rect->end_y = 0;
}

static void increase_rect_offset(struct ae_rect *rect, int32_t x_offset,
                                    int32_t y_offset, struct ae_trim *trim)
{
	int32_t start_x = 0;
	int32_t end_x = 0;
	int32_t start_y = 0;
	int32_t end_y = 0;


	if (NULL == rect || NULL == trim) {
		AE_LOGE("rect %p trim %p param is error!", rect, trim);
		return;
	}

	start_x = rect->start_x;
	end_x = rect->end_x;
	start_y = rect->start_y;
	end_y = rect->end_y;

	if (start_x - x_offset < 0)
		start_x = 0;
	else
		start_x -= x_offset;
	rect->start_x = start_x;

	if (end_x + x_offset >= (int32_t)trim->w)
		end_x = (int32_t)(trim->w - 1);
	else
		end_x += x_offset;
	rect->end_x = end_x;

	if (start_y - y_offset < 0)
		rect->start_y = 0;
	else
		start_y -= y_offset;
	rect->start_y = start_y;

	if (end_y + y_offset >= (int32_t)trim->h)
		end_y = trim->h - 1;
	else
		end_y += y_offset;
	rect->end_y = end_y;
}

static int32_t is_in_rect(uint32_t x, uint32_t y, struct ae_rect *rect)
{
	int32_t is_in = 0;

	if (NULL == rect) {
		AE_LOGE("rect %p param is error!", rect);
		return 0;
	}

	if (rect->start_x <= x && rect->start_y <= y
		&& x <= rect->end_x && y <= rect->end_y) {
		is_in = 1;
	}

	return is_in;
}
static int32_t _set_touch_zone(struct ae_ctrl_context *cxt, struct ae_trim *touch_zone)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_trim  level_0_trim;

	uint32_t org_w = 0;
	uint32_t org_h = 0;
	uint32_t new_w = 0;
	uint32_t new_h = 0;
	int32_t touch_x = 0;
	int32_t touch_y = 0;
	int32_t touch_w = 0;
	int32_t touch_h = 0;
	uint32_t i = 0;
	uint32_t j = 0;
	struct ae_rect level_0_rect;
	struct ae_rect  level_1_rect;
	struct ae_rect  level_2_rect;
	uint32_t level_1_x_offset = 0;
	uint32_t level_1_y_offset = 0;
	uint32_t level_2_x_offset = 0;
	uint32_t level_2_y_offset = 0;
	uint32_t level_0_weight = 0;
	uint32_t level_1_weight = 0;
	uint32_t level_2_weight = 0;
	struct ae_trim map_trim;
	uint8_t *tab_ptr = NULL;
	int32_t weight_val = 0;

	if (NULL == cxt || NULL == touch_zone) {
		AE_LOGE("cxt %p touch_zone %p param is error!", cxt, touch_zone);
		return AE_ERROR;
	}
	org_w = cxt->resolution_info.frame_size.w;
	org_h = cxt->resolution_info.frame_size.h;
	new_w = cxt->monitor_unit.win_num.w;
	new_h = cxt->monitor_unit.win_num.h;


	touch_x = touch_zone->x;
	touch_y = touch_zone->y;
	touch_w = touch_zone->w;
	touch_h = touch_zone->h;
	AE_LOGI("touch_x %d, touch_y %d, touch_w %d, touch_h %d", touch_x, touch_y, touch_w, touch_h);
	level_0_trim.x = touch_x * new_w / org_w;
	level_0_trim.y = touch_y * new_h / org_h;
	level_0_trim.w = touch_w * new_w / org_w;
	level_0_trim.h = touch_h * new_h / org_h;


	map_trim.x = 0;
	map_trim.y = 0;
	map_trim.w = new_w;
	map_trim.h = new_h;
	convert_trim_to_rect(&level_0_trim, &level_0_rect);
	level_1_x_offset = (cxt->touch_zone_param.zone_param.level_1_percent * level_0_trim.w) >>(6+1);
	level_1_y_offset = (cxt->touch_zone_param.zone_param.level_1_percent * level_0_trim.h) >> (6+1);

	level_2_x_offset = (cxt->touch_zone_param.zone_param.level_2_percent * level_0_trim.w) >> (6+1);
	level_2_y_offset = (cxt->touch_zone_param.zone_param.level_2_percent * level_0_trim.h) >> (6+1);

	//level 1 rect
	level_1_rect = level_0_rect;
	increase_rect_offset(&level_1_rect, level_1_x_offset, level_1_y_offset, &map_trim);
	//level 2 rect
	level_2_rect = level_1_rect;
	increase_rect_offset(&level_2_rect, level_2_x_offset, level_2_y_offset, &map_trim);

	level_0_weight = cxt->touch_zone_param.zone_param.level_0_weight;
	level_1_weight = cxt->touch_zone_param.zone_param.level_1_weight;
	level_2_weight = cxt->touch_zone_param.zone_param.level_2_weight;


	tab_ptr = cxt->touch_zone_param.weight_table.weight;
	for (j = 0; j < new_h; ++j) {
		for (i = 0; i < new_w; ++i) {
			weight_val = 1;

			//level 2 rect, big
			if (is_in_rect(i, j, &level_2_rect))
				weight_val = level_2_weight;
			//level 1 rect, middle
			if (is_in_rect(i, j, &level_1_rect))
				weight_val = level_1_weight;
			//level 0 rect, small
			if (is_in_rect(i, j, &level_0_rect))
				weight_val = level_0_weight;

			tab_ptr[i + new_w * j] = weight_val;
		}
	}

	{
		struct ae_weight_table *weight_tab;

		weight_tab = &cxt->touch_zone_param.weight_table;
		rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_WEIGHT_TABLE,
						(void *)weight_tab, NULL);
		if (AE_SUCCESS == rtn) {
			cxt->cur_status.touch_zone = *touch_zone;
		}
	}
	cxt->touch_zone_param.enable = 1;

	return rtn;
}

static int32_t _set_weight(struct ae_ctrl_context *cxt, enum ae_weight_mode mode)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_weight_table *weight_table = NULL;

	if (mode >= AE_WEIGHT_MAX) {
		AE_LOGE("weight %d is error!", mode);
		return AE_ERROR;
	}

	weight_table = &cxt->cur_param->weight_table[mode];
	rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_WEIGHT_TABLE,
					(void *)weight_table, NULL);
	AE_LOGI("set weight from %d to %d, rtn=%d", cxt->cur_status.weight, mode, rtn);
	if (AE_SUCCESS == rtn) {
		cxt->cur_status.weight = mode;
		cxt->touch_zone_param.enable = 0;
	}

	return rtn;
}

static int32_t _set_fps(struct ae_ctrl_context *cxt, struct ae_set_fps *fps)
{
	int32_t rtn = AE_ERROR;

	if (NULL == cxt || NULL == fps) {
		AE_LOGE("cxt %p fps %p param is error!", cxt, fps);
		return AE_ERROR;
	}
	/*fps->max_fps = 0;*/
	rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_FPS, (void *)fps, NULL);
	if (AE_SUCCESS == rtn) {
		cxt->cur_status.fps = *fps;
	}

	return rtn;
}

static int32_t _set_flicker(struct ae_ctrl_context *cxt, enum ae_flicker_mode mode)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_exp_gain_table *ae_table = NULL;
	struct ae_status *cur_status = NULL;
	uint32_t new_index = 0;
	uint32_t cur_exp_gain = 0;
	uint32_t exp_line = 0;
	uint32_t gain = 0;

	if (mode >= AE_FLICKER_MAX) {
		AE_LOGE("flicker %d is error!", mode);
		return AE_ERROR;
	}

	cur_status = &cxt->cur_status;
	rtn = _get_work_mode_ae_tbl(cxt->cur_param, cur_status->scene,
								mode, cur_status->iso, &ae_table);
	if (AE_SUCCESS != rtn) {
		goto EXIT;
	}
	rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_AE_TABLE, (void *)ae_table, NULL);
	AE_LOGI("set flicker from %d to %d, rtn=%d",
			cxt->cur_status.flicker, mode, rtn);
	if (AE_SUCCESS == rtn) {
		cxt->cur_status.flicker = mode;
	}


	exp_line = cxt->cur_result.cur_exp_line;
	gain = cxt->cur_result.cur_again;
	cur_exp_gain = exp_line * _get_ae0_real_gain(gain);
	rtn = _get_new_index(cxt, cur_exp_gain, &new_index);
	if (AE_SUCCESS != rtn) {
		goto EXIT;
	}
	rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_INDEX, (void *)&new_index, NULL);
	cxt->prv_index = new_index;

	rtn = ae_misc_io_ctrl(cxt->misc_handle,
				AE_MISC_CMD_GET_EXP_BY_INDEX,
				(void *)&new_index,
				&cxt->cur_result);
	if (AE_SUCCESS == rtn) {
		_write_to_sensor(cxt, &cxt->cur_result, 1);
	}

EXIT:

	return rtn;
}

static int32_t _set_flash(struct ae_ctrl_context *cxt, struct ae_flash_ctrl *flash_ctrl)
{
	int32_t rtn = AE_SUCCESS;

	if (NULL == flash_ctrl) {
		AE_LOGE("flash_ctrl %p param is error!", flash_ctrl);
		return AE_ERROR;
	}

	rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_FLASH_PARAM, (void *)flash_ctrl, NULL);


	return rtn;
}

static int32_t _is_work_mode_diff(struct ae_set_work_param *cur, struct ae_set_work_param *next)
{
	int32_t is_diff = 0;

	if (cur && next) {
		if ((cur->mode != next->mode)
			|| (cur->resolution_info.sensor_size_index != next->resolution_info.sensor_size_index)
			|| (cur->resolution_info.line_time != next->resolution_info.line_time)
			|| (cur->resolution_info.frame_line != next->resolution_info.frame_line)) {
			is_diff = 1;
		}
	}

	return is_diff;
}

static int32_t _set_quick_mode(struct ae_ctrl_context *cxt, uint32_t is_quick)
{
	int32_t rtn = AE_SUCCESS;
	uint32_t quick_mode = is_quick;


	rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_QUICK_MODE, &quick_mode, NULL);
	if (AE_SUCCESS == rtn) {
		cxt->cur_status.is_quick_mode = is_quick;
	}
	AE_LOGI("is_quick=%d", is_quick);

	return rtn;
}

static int32_t _set_exp_anti(struct ae_ctrl_context *cxt, void* in_param)
{
	int32_t rtn = AE_SUCCESS;
	uint32_t *enable = (uint32_t*)in_param;

	rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_EXP_ANIT, in_param, NULL);

	return rtn;
}

static int32_t _get_flicker_switch_flag(struct ae_ctrl_context *cxt, void* in_param)
{
	int32_t rtn = AE_SUCCESS;
	uint32_t cur_exp = 0;
	uint32_t *flag = (uint32_t *)in_param;
	enum ae_work_mode work_mode = 0;

	work_mode = cxt->cur_status.real_work_mode;
	if(AE_WORK_MODE_COMMON == work_mode) {
		cur_exp = cxt->cur_result.cur_exposure;
		//50Hz/60Hz
		if(AE_FLICKER_50HZ == cxt->cur_status.flicker) {
			if(cur_exp < 100000) {
				*flag = 0;
			} else {
				*flag = 1;
			}
		} else {
			if(cur_exp < 83333) {
				*flag = 0;
			} else {
				*flag = 1;
			}
		}
	} else {
		*flag = 1;
	}

	*flag = cxt->flash_working ? 0 : *flag;

	AE_LOGE("ANTI_FLAG: =%d, %d, %d, %d", cur_exp, cxt->cur_result.cur_exp_line, cxt->resolution_info.line_time, *flag);

	return rtn;
}

static int32_t _get_ae_weight(struct ae_ctrl_context *cxt, void* in_param, void* out_param)
{
	int32_t rtn = AE_SUCCESS;
	uint32_t *enable = (uint32_t*)in_param;

	rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_GET_WEIGHT, in_param, out_param);

	return rtn;
}

static int32_t _update_status(struct ae_ctrl_context *cxt)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_status *cur_status = &cxt->cur_status;
	struct ae_status *next_status = &cxt->nxt_status;
	struct ae_update_list *update_list = &cxt->update_list;


	if (_is_work_to_scene(cxt, next_status)) {
		update_list->is_iso = 0;
		update_list->is_weight = 0;
		update_list->is_touch_zone = 0;
		update_list->is_ev = 0;
		update_list->is_fps = 0;
	}

	if (update_list->is_scene
		&& (cur_status->scene != next_status->scene)) {
		rtn = _set_scene_mode(cxt, next_status->scene);
	}
	update_list->is_scene = 0;

	if (update_list->is_ev
		&& (cur_status->ev_level != next_status->ev_level)) {
		rtn = _set_ev_level(cxt, next_status->ev_level);
	}
	update_list->is_ev = 0;

	if (update_list->is_iso
		&& (cur_status->iso != next_status->iso)) {
		rtn = _set_iso(cxt, next_status->iso);
	}
	update_list->is_iso = 0;

	if (cur_status->flicker != next_status->flicker) {
		rtn = _set_flicker(cxt, next_status->flicker);
	}

	if (update_list->is_weight
		&& (cur_status->weight != next_status->weight)) {
		rtn = _set_weight(cxt, next_status->weight);
	}
	update_list->is_weight = 0;

	if (update_list->is_touch_zone
		&& (AE_WEIGHT_SPOT == cur_status->weight)
		&& (cur_status->touch_zone.x != next_status->touch_zone.x
			|| cur_status->touch_zone.y != next_status->touch_zone.y
			|| cur_status->touch_zone.w != next_status->touch_zone.w
			|| cur_status->touch_zone.h != next_status->touch_zone.h )) {
			uint weight = 0;
			uint high =0;

			weight = cxt->resolution_info.frame_size.w;
			high = cxt->resolution_info.frame_size.h;
			if ((next_status->touch_zone.x < weight)
				&& (next_status->touch_zone.y < high)) {
				if (next_status->touch_zone.w > weight) {
					next_status->touch_zone.w = weight;
				}
				if (next_status->touch_zone.h > high) {
					next_status->touch_zone.h = high;
				}
				rtn = _set_touch_zone(cxt, &next_status->touch_zone);
			}
	}
	update_list->is_touch_zone = 0;

	if (update_list->is_night
		&& (cur_status->night_mode != next_status->night_mode)) {
		rtn = _set_night_mode(cxt, next_status->night_mode);
	}
	update_list->is_night = 0;

	if (_is_work_mode_diff(&cur_status->work_info, &next_status->work_info)) {
		rtn = _set_work_mode(cxt, &next_status->work_info, 0);
	}

	if (update_list->is_fps
		&& (cur_status->fps.min_fps != next_status->fps.min_fps
			|| cur_status->fps.max_fps != next_status->fps.max_fps)) {
		rtn = _set_fps(cxt, &next_status->fps);
	}
	update_list->is_fps = 0;

	if (update_list->is_quick
		&& (cur_status->is_quick_mode != next_status->is_quick_mode)) {
		rtn = _set_quick_mode(cxt, next_status->is_quick_mode);
	}
	update_list->is_quick = 0;

	if (update_list->is_target_lum
		&& (cur_status->target_lum != next_status->target_lum)) {
		rtn = _set_target_lum(cxt, next_status->target_lum);
	}
	update_list->is_target_lum = 0;

	return rtn;
}

static uint32_t _isp_ae_get_flash_effect(struct ae_ctrl_context *cxt, float k, uint32_t *effect)
{
	int32_t before_flash_luma = cxt->flash_param.before_result.cur_lum;

	uint32_t ratio = cxt->flash_param.flash_ratio;

	float effect_ratio = 0.0;

	if (NULL == effect) {
		AE_LOGE("effect %p param is error!", effect);
		return AE_ERROR;
	}

	*effect = QUANTIFICAT;

	if (!before_flash_luma)
		goto exit_end;

	if (1.0 < k)
		k = 1.0;
	else if(k > -0.000001 && k < 0.000001)
		goto exit_end;

	effect_ratio = ratio * (1 - k) / (k * CAL_CISION);
	effect_ratio = effect_ratio / (effect_ratio + 1);
	*effect = effect_ratio * QUANTIFICAT + 0.5;
	AE_LOGI("effect_ratio = %f *effect = %d", effect_ratio, *effect);

	*effect = (*effect > QUANTIFICAT) ? QUANTIFICAT : *effect;

exit_end:
	AE_LOGI("x *effect = %d", *effect);
	return 0;
}
static uint32_t _calc_flash_mainlum_effect(struct ae_ctrl_context *cxt, struct ae_misc_calc_out *bef_flash_calc,
												struct ae_misc_calc_out *pre_flash_calc, uint32_t *main_lum)
{
	uint32_t rtn = AE_SUCCESS;
	uint32_t before_flash_exp = 0;
	uint32_t before_flash_gain = 0;
	uint32_t pre_flash_exp = 0;
	uint32_t pre_flash_gain = 0;
	float pre_flash_ev = 0;
	float before_flash_ev = 0;
	float k = 0;
	uint32_t before_flash_luma = 0;
	uint32_t pre_flash_luma = 0;
	uint32_t ratio = cxt->flash_param.flash_ratio;

	if (NULL == bef_flash_calc || NULL == pre_flash_calc
		|| NULL == main_lum) {
		AE_LOGE("%p %p %p param is error!", bef_flash_calc, pre_flash_calc, main_lum);
		return AE_ERROR;
	}

	before_flash_luma = bef_flash_calc->cur_lum;
	pre_flash_luma = pre_flash_calc->cur_lum;

	before_flash_exp = bef_flash_calc->cur_exposure;
	before_flash_gain = bef_flash_calc->cur_again;
	AE_LOGI("before_flash_exp = %d before_flash_gain = %d", before_flash_exp, before_flash_gain);

	pre_flash_exp = pre_flash_calc->cur_exposure;
	pre_flash_gain = pre_flash_calc->cur_again;
	AE_LOGI("pre_flash_exp = %d pre_flash_gain = %d", pre_flash_exp, pre_flash_gain);

	pre_flash_ev = pre_flash_gain * 1.0 * pre_flash_exp;
	before_flash_ev = before_flash_gain * 1.0 * before_flash_exp;
	k = (before_flash_luma * pre_flash_ev) * 1.0 / (pre_flash_luma * before_flash_ev);
	AE_LOGI("before_flash_luma = %d, pre_flash_ev = %f before_flash_ev = %f pre_flash_luma = %d k = %f",
			before_flash_luma, pre_flash_ev, before_flash_ev, pre_flash_luma, k);

	*main_lum = pre_flash_luma * (k * CAL_CISION + (1 - k) * ratio) / CAL_CISION + 0.5;
	AE_LOGI("cap_luma = %d, ratio = %d", *main_lum, ratio);

	_isp_ae_get_flash_effect(cxt, k, &cxt->flash_param.effect);

	return rtn;
}
/*
static uint32_t _calc_flash_effect(struct ae_ctrl_context *cxt, struct ae_misc_calc_out *bef_flash_calc, struct ae_misc_calc_out *pre_flash_calc, uint32_t *effect)
{
	uint32_t rtn = AE_SUCCESS;
	uint32_t ratio = cxt->flash_param.flash_ratio;
	uint32_t bef_flash_ex = bef_flash_calc->cur_exposure;
	uint32_t bef_flash_gain = bef_flash_calc->cur_again;
	uint32_t bef_flash_lum = bef_flash_calc->cur_lum;
	uint32_t pre_flash_ex = pre_flash_calc->cur_exposure;
	uint32_t pre_flash_gain = pre_flash_calc->cur_again;
	uint32_t pre_flash_lum = pre_flash_calc->cur_lum;
	uint64_t mflash = 0;
	uint64_t evflash = 0;
	uint32_t k = 0;

	if (NULL == effect) {
		AE_LOGE("effect %p param is error!", effect);
		return AE_ERROR;
	}
	*effect  = 0;

	if (!bef_flash_lum) {
		*effect = 1024;
		goto exit;
	}

	if (((bef_flash_lum <= (bef_flash_calc->target_lum+3)) && (bef_flash_lum >= (bef_flash_calc->target_lum-3)))
		&& ((pre_flash_lum <= (pre_flash_calc->target_lum+3)) && (pre_flash_lum >= (pre_flash_calc->target_lum-3))))  {
		AE_LOGI("target lum same as cur_lum");
		mflash = (uint64_t)bef_flash_gain * (uint64_t)bef_flash_ex;
		evflash = (uint64_t)pre_flash_gain * (uint64_t)pre_flash_ex;
	}else{
		mflash = (uint64_t)pre_flash_lum * (uint64_t)bef_flash_ex * (uint64_t)bef_flash_gain;
		evflash = (uint64_t)bef_flash_lum * (uint64_t)pre_flash_ex * (uint64_t)pre_flash_gain;
	}

	k = (uint32_t)((mflash<<8)/(evflash));

	k = k < 256 ? 256 : k;
	ratio = cxt->flash_param.flash_ratio;
	ratio = ratio < 256 ? 256 : ratio;
	AE_LOGI("flash_ratio %d",ratio);

	*effect = ((ratio*k-(ratio<<8)) <<10)/(ratio*k-((ratio - 256)<<8));
	if (*effect > 1024) {
		*effect = 1024;
	}

exit:
	AE_LOGI("effect=%d", *effect);

	return rtn;
}
*/
static int32_t _process_flash(struct ae_ctrl_context *cxt, int32_t cur_lum)
{
	int32_t rtn = AE_ERROR;
	struct ae_flash_ctrl flash_ctrl;
	uint32_t main_flash_lum = 0;
	int32_t ratio = 0;
	int32_t prv_lum = 0;

	memset(&flash_ctrl, 0, sizeof(flash_ctrl));
	flash_ctrl.enable = cxt->flash_param.enable;
	if (flash_ctrl.enable) {
		prv_lum = cxt->flash_param.before_result.cur_lum;
		cxt->flash_param.convergence_speed = cxt->cur_param->convergence_speed;
		ratio = cxt->flash_param.flash_ratio;
		_calc_flash_mainlum_effect(cxt, &cxt->flash_param.before_result, &cxt->flash_param.preflash_converged_result, &main_flash_lum);
		flash_ctrl.main_flash_lum = main_flash_lum;
		AE_LOGI("prv_lum=%d,cur_lum=%d,main_lum=%d",
				prv_lum, cur_lum, main_flash_lum);
	} else {
		flash_ctrl.convergence_speed = cxt->flash_param.convergence_speed;
	}
	rtn = _set_flash(cxt, &flash_ctrl);
	if (AE_SUCCESS == rtn) {
		cxt->flash_param.main_flash_lum = main_flash_lum;
	}

	return rtn;
}

static int32_t _calc_premain_exposure(struct ae_ctrl_context *cxt,
										struct ae_misc_calc_in *calc_in,
										struct ae_misc_calc_out *calc_out)
{
	int32_t rtn = AE_ERROR;
	struct flash_param *flash = &cxt->flash_param;


	if ( NULL == calc_in || NULL == calc_out) {
		AE_LOGE("in %p out %p param is error!", calc_in, calc_out);
		return AE_ERROR;
	}
	rtn = _process_flash(cxt, calc_out->cur_lum);
	AE_RETURN_IF_FAIL(rtn, ("process flash rtn %d is error!", rtn));

	flash->main_lum = flash->main_flash_lum;
	calc_in->fd_state = cxt->fdae.state;
	rtn = ae_misc_calculation(cxt->misc_handle, calc_in, calc_out);
	AE_RETURN_IF_FAIL(rtn, ("calc rtn %d is error!", rtn));

	cxt->flash_param.capture_result = *calc_out;
	AE_LOGE("flash premain cur_index %d", calc_out->cur_index);
	//rtn = _calc_flash_effect(cxt, &flash->before_result, &flash->preflash_converged_result, &flash->effect);

	return rtn;
}

static int32_t _get_bv_by_lum(struct ae_ctrl_context *cxt,
                                   struct ae_misc_calc_out *cur_result,
                                   int32_t *bv)
{

	int32_t rtn = AE_SUCCESS;
	uint32_t cur_exp = 0;
	uint32_t real_gain = 0;
	uint32_t cur_lum =  0;


	if ( NULL == cxt || NULL == cur_result || NULL == bv) {
		AE_LOGE("cxt %p cur_result %p bv %p param is error!", cxt, cur_result, bv);
		return AE_ERROR;
	}
	*bv = 0;
	/*	 LOG2( 16 /(exp * gain * 128 / cur_lum) ) * 16
	* exp : unit: s.
       * gain: real gain
       * cur_lum:
	*/
	cur_lum = cur_result->cur_lum;
	cur_exp = cur_result->cur_exposure;
	real_gain = _get_real_gain(cur_result->cur_again);
	if (0 == cur_lum)
		cur_lum = 1;
#ifdef AE_TABLE_32
	*bv = (int32_t)(log2(((double)100000000 * cur_lum) / ((double)cur_exp * real_gain * 5)) * 16.0 + 0.5);
#else
	*bv = (int32_t)(log2((double)(cur_exp * cur_lum) / (double)(real_gain * 5)) * 16.0 + 0.5);
#endif

    AE_LOGV("real bv=%d", *bv);

    return rtn;
}

static int32_t _get_real_gain(uint32_t gain)
{// x=real_gain/16
	uint32_t real_gain = 0;
	uint32_t cur_gain = gain;
	uint32_t i = 0;
#ifdef AE_TABLE_32
	real_gain = gain >>3;/// 128 * 16;
#else
	real_gain = ((gain & 0x0f)+16);
	cur_gain = (cur_gain >> 4);

	for (i = 0x00; i < 7; ++i) {
		if (0x01 == (cur_gain & 0x01)) {
			real_gain *= 0x02;
		}
		cur_gain >>=0x01;
	}
#endif

	return real_gain;
}

static int32_t _get_bv_by_gain(struct ae_ctrl_context *cxt,
                                   struct ae_misc_calc_out *cur_result,
                                   int32_t *bv)
{
	int32_t rtn = AE_SUCCESS;
	uint32_t cur_exp = 0;
	uint32_t cur_gain = 0;


	if ( NULL == cxt || NULL == cur_result || NULL == bv) {
		AE_LOGE("cxt %p cur_result %p bv %p param is error!", cxt, cur_result, bv);
		return AE_ERROR;
	}

	*bv = _get_real_gain(cur_result->cur_again);

	AE_LOGV("bv=%d", *bv);

    return rtn;
}

static int32_t _cfg_monitor_win(struct ae_ctrl_context *cxt)
{
	int32_t rtn = AE_SUCCESS;


	if (cxt->isp_ops.set_monitor_win) {
		struct ae_monitor_info info;

		info.win_size = cxt->monitor_unit.win_size;
		info.trim = cxt->monitor_unit.trim;

		AE_LOGI("info x=%d,y=%d,w=%d,h=%d, block_size.w=%d,block_size.h=%d",
				info.trim.x, info.trim.y, info.trim.w, info.trim.h,
				info.win_size.w, info.win_size.h);
		rtn = cxt->isp_ops.set_monitor_win(cxt->isp_ops.isp_handler, &info);
	}

	return rtn;
}

static int32_t _cfg_monitor_bypass(struct ae_ctrl_context *cxt)
{
	int32_t rtn = AE_SUCCESS;


	if (cxt->isp_ops.set_monitor_bypass) {
		uint32_t is_bypass = 0;

		is_bypass = cxt->monitor_unit.is_stop_monitor;
		//AE_LOGI("AE_TEST: is_bypass=%d", is_bypass);
		rtn = cxt->isp_ops.set_monitor_bypass(cxt->isp_ops.isp_handler, is_bypass);
	}

	return rtn;
}

static int32_t _cfg_set_aem_mode(struct ae_ctrl_context *cxt)
{
	int32_t ret = AE_SUCCESS;

	if (cxt->isp_ops.set_statistics_mode) {
		cxt->isp_ops.set_statistics_mode(cxt->isp_ops.isp_handler, cxt->monitor_mode, cxt->monitor_skip_num);
	} else {
		AE_LOGE("set_statistics_mode function is NULL\n");
		ret = AE_ERROR;
	}

	return ret;
}

static int32_t _set_g_stat(struct ae_ctrl_context *cxt, struct ae_stat_mode *stat_mode)
{
	int32_t rtn = AE_SUCCESS;


	if ( NULL == cxt || NULL == stat_mode) {
		AE_LOGE("cxt %p stat_mode %p param is error!", cxt, stat_mode);
		return AE_ERROR;
	}

	cxt->stat_req.mode = stat_mode->mode;
	rtn = _update_monitor_unit(cxt, &stat_mode->trim);
	if (AE_SUCCESS != rtn) {
		goto EXIT;
	}

	rtn = _cfg_monitor_win(cxt);
	if (AE_SUCCESS == rtn){
		_reset_stat(cxt, 1);
	}
EXIT:

	return rtn;
}

static int32_t _set_scaler_trim(struct ae_ctrl_context *cxt, struct ae_trim *trim)
{
	int32_t rtn = AE_SUCCESS;


	rtn = _update_monitor_unit(cxt, trim);
	if (AE_SUCCESS != rtn) {
		goto EXIT;
	}

	rtn = _cfg_monitor_win(cxt);
	if (AE_SUCCESS == rtn) {
		_reset_stat(cxt, 1);
	}
EXIT:

	return rtn;
}

static int32_t _set_correction_param(struct ae_ctrl_context *cxt)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_misc_calc_out calc_result;
	uint32_t ae_state = cxt->ae_state;


	switch (ae_state) {
	case AE_STATE_FLASH_REQUIRED:
	case AE_STATE_LOCKED:
		rtn = AE_ERROR;
		break;
	case AE_STATE_PRECAPTURE:
		calc_result = cxt->flash_param.capture_result;
		break;
	default:
		calc_result = cxt->cur_result;
		break;
	}


	if (cxt->flash_param.flash_tuning_eb) {
		calc_result = cxt->flash_param.flash_tuning_result;
	}


	AE_LOGI("ae_state=%d", ae_state);
	if (AE_SUCCESS == rtn) {
		_write_to_sensor(cxt, &calc_result, 1);
	}

EXIT:

	return rtn;
}

static uint32_t _get_exposure_line(uint32_t line_time, uint32_t exposure)
{
	uint32_t exp_line = 0;

	if ((0 != exposure) && (0 != line_time)) {
#ifdef AE_TABLE_32
		exp_line = exposure / line_time;
#else
		exp_line = 100000000 / exposure / line_time;
#endif
	} else {
		exp_line = 0;
	}

	return exp_line;
}

static int32_t _get_cap_shutter_by_index(struct ae_exp_gain_table *ae_table,
                                          struct ae_snapshot_notice *notice,
                                          uint32_t index,
                                          struct ae_misc_calc_out *calc_result)
{
	int32_t rtn = AE_SUCCESS;
	uint32_t cur_index = 0;
	uint32_t exp_line = 0;
	uint32_t min_index = 0;
	uint32_t max_index = 0;
	uint32_t prv_line_time = 0;
	uint32_t cap_line_time = 0;

	if (NULL == ae_table || NULL == calc_result) {
		AE_LOGE("ae_table=%p result ptr=%p is error!", ae_table, calc_result);
		return AE_ERROR;
	}
	prv_line_time = notice->preview_line_time;
	cap_line_time = notice->capture_line_time;
	cur_index = index;
	max_index = ae_table->max_index;
	if (max_index >= AE_TBL_MAX_INDEX) {
		max_index = AE_TBL_MAX_INDEX - 1;
	}
	if (cur_index > max_index) {
		cur_index = max_index;
	}

	exp_line = _get_exposure_line(prv_line_time, ae_table->exposure[cur_index]);

	if (exp_line <= 0) {
		exp_line = 1;
	}

	calc_result->cur_index = cur_index;
	calc_result->cur_again = ae_table->again[cur_index];
	calc_result->cur_dgain = ae_table->dgain[cur_index];
	calc_result->cur_dummy = ae_table->dummy[cur_index];
	calc_result->cur_exp_line = exp_line;
	calc_result->cur_lum = 0;
	calc_result->min_index = min_index;
	calc_result->max_index = max_index;

	AE_LOGI("prv_line_time=%d, cap_line_time=%d, exp_line=%d, cur_index=%d",
			prv_line_time, cap_line_time, exp_line, cur_index);
	return rtn;
}

static int32_t _set_premain_exg(struct ae_ctrl_context *cxt)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_misc_calc_out calc_result;
	struct ae_misc_calc_out save_result;
	uint32_t work_mode = AE_WORK_MODE_CAPTURE;
	uint32_t capture_param_enable = 0;
	struct ae_status *cur_status = NULL;
	struct ae_tuning_param *cur_param = NULL;
	struct ae_exp_gain_table *ae_table = NULL;
	uint32_t cur_index = 0;
	//uint32_t is_restore = 0;
	uint32_t fix_fps = 0;
	uint32_t cur_shutter = 0;
	uint32_t is_high_fps = 0;


	if ((NULL == cxt)) {
		AE_LOGE("cxt %p param is error!", cxt);
		return AE_PARAM_NULL;
	}
	calc_result = cxt->flash_param.capture_result;
	cur_shutter = calc_result.cur_exposure;
	fix_fps = 30;
#ifdef AE_TABLE_32
	if ((10000000 / cur_shutter) < fix_fps) {
		is_high_fps = 1;
	}
#else
	if (cur_shutter < (fix_fps * 10)) {
		is_high_fps = 1;
	}
#endif
#if 0 //!! fix me (to remove #if 0)
	if (is_high_fps) {
		rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CME_SET_FIX_FPS, (void *)&fix_fps, &calc_result);
	}
#endif
	cur_index = calc_result.cur_index;

	AE_LOGI("cur_index=%d, cur_exp_line=%d", calc_result.cur_index, calc_result.cur_exp_line);
	rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_INDEX, (void *)&cur_index, NULL);
	rtn = _write_to_sensor(cxt, &calc_result, 1);
	//if (is_restore) {
	//	cxt->cur_result = save_result;
	//}

EXIT:

	return rtn;
}

static int32_t _set_quick_mode_status(struct ae_ctrl_context *cxt, int32_t is_quick)
{
	int32_t rtn = AE_SUCCESS;

	cxt->nxt_status.is_quick_mode = is_quick;
	cxt->update_list.is_quick = is_quick;

	return rtn;
}

static int32_t _set_snapshot_notice(struct ae_ctrl_context *cxt,
                                         struct ae_snapshot_notice *notice)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_misc_calc_out calc_result;
	struct ae_misc_calc_out save_result;
	uint32_t work_mode = AE_WORK_MODE_CAPTURE;
	uint32_t capture_param_enable = 0;
	struct ae_status *cur_status = NULL;
	struct ae_tuning_param *cur_param = NULL;
	struct ae_exp_gain_table *ae_table = NULL;
	uint32_t cur_index = 0;
	uint32_t is_restore = 0;


	if ((NULL == cxt) || (NULL == notice)) {
		AE_LOGE("cxt %p notice %p param is error!", cxt, notice);
		return AE_PARAM_NULL;
	}

	/*get exposure by index for capture,
	but current ae table is for preview*/

	if (AE_STATE_PRECAPTURE == cxt->ae_state) {
		/*when flashing*/
		calc_result = cxt->flash_param.capture_result;
	} else {
		/*first save the result, then restore for preview*/
		calc_result = cxt->cur_result;
		save_result = calc_result;
		is_restore = 1;
	}
	cur_index = calc_result.cur_index;

	capture_param_enable = cxt->tuning_param_enable[work_mode];
	if (capture_param_enable
		&& AE_WORK_MODE_CAPTURE == work_mode) {
		cur_status = &cxt->cur_status;
		cur_param = &cxt->tuning_param[work_mode];

		rtn = _get_work_mode_ae_tbl(cur_param, cur_status->scene, cur_status->flicker,
									cur_status->iso, &ae_table);
		if (AE_SUCCESS != rtn) {
			goto EXIT;
		}
		rtn = _get_cap_shutter_by_index(ae_table, notice, cur_index, &calc_result);
	} else {
		rtn = ae_misc_io_ctrl(cxt->misc_handle,
								AE_MISC_CMD_GET_EXP_BY_INDEX,
								(void *)&cur_index,
								&calc_result);
	}

	if (AE_SUCCESS != rtn) {
		goto EXIT;
	}
	if (cxt->is_force_lock) {
		AE_LOGI("force lock,do not write sensor");
		goto EXIT;
	}
	AE_LOGI("cur_index=%d, cur_exp_line=%d", calc_result.cur_index, calc_result.cur_exp_line);

	//rtn = _write_to_sensor(cxt, &calc_result, 1);
	if (is_restore) {
		cxt->cur_result = save_result;
	}

EXIT:

	return rtn;
}

static int32_t _set_before_flash(struct ae_ctrl_context *cxt)
{
	int32_t rtn = AE_SUCCESS;
	uint32_t index = 0;
	uint32_t exp_gain = 0;

	memset(&cxt->flash_param, 0, sizeof(cxt->flash_param));
	cxt->flash_param.enable = 1;
	cxt->flash_param.before_result = cxt->cur_result;
	cxt->flash_param.effect = 0;

	exp_gain = cxt->cur_param->flash_tuning.exposure_index;
	cxt->flash_param.flash_tuning_eb = exp_gain;
	if (cxt->flash_param.flash_tuning_eb) {
		rtn = _get_new_index(cxt, exp_gain, &index);
		if (AE_SUCCESS != rtn) {
			cxt->flash_param.flash_tuning_result = cxt->cur_result;
			goto EXIT;
		}
		rtn = ae_misc_io_ctrl(cxt->misc_handle,
							AE_MISC_CMD_GET_EXP_BY_INDEX,
							(void *)&index,
							&cxt->flash_param.flash_tuning_result);
	}
	cxt->flash_param.flash_state = AE_FLASH_STATE_BEFORE;

	_set_quick_mode_status(cxt, 1);
EXIT:

	return rtn;
}

static int32_t _calc_flash_target_lum(struct ae_ctrl_context *cxt)
{
	int32_t rtn = AE_SUCCESS;
	int32_t prv_lum = cxt->flash_param.before_result.cur_lum;
	int32_t main_flash_lum = cxt->flash_param.main_lum;
	int32_t cur_lum_offset = 0;
	int32_t target_lum = cxt->cur_status.target_lum;
	int32_t flash_target_lum = (target_lum > AE_FLASH_TARGET_LUM) ? AE_FLASH_TARGET_LUM : (target_lum * 60 / 100);
	int32_t max_lum_ratio = AE_FLASH_MAX_RATIO;
	int32_t target_lum_max_offset =  flash_target_lum - cxt->cur_status.target_lum;
	int32_t target_lum_offset = 0;
	int32_t target_lum_ratio = 0;
	int32_t pre_lum = cxt->flash_param.preflash_converged_result.cur_lum;
	int32_t pre_exp =  cxt->flash_param.preflash_converged_result.cur_exposure;
	int32_t pre_gain =  cxt->flash_param.preflash_converged_result.cur_again;
	int32_t before_lum = cxt->flash_param.before_result.cur_lum;
	int32_t before_exp =  cxt->flash_param.before_result.cur_exposure;
	int32_t before_gain =  cxt->flash_param.before_result.cur_again;

	if (0 == pre_exp || 0 == pre_gain)
		return target_lum;

	main_flash_lum = pre_lum * before_exp /pre_gain* before_gain / pre_exp * 6;
	cur_lum_offset = main_flash_lum - prv_lum;

	if (prv_lum <= 0) {
		target_lum_offset = target_lum_max_offset;
	} else if (cur_lum_offset <= 0) {
		target_lum_offset = 0;
	} else {
		target_lum_ratio = (cur_lum_offset * 256 / prv_lum) ;

		if(target_lum_ratio > max_lum_ratio * 256){
			target_lum_offset = target_lum_max_offset;
		}else if(target_lum_ratio <256){
			target_lum_offset = 0;
		}else{
			target_lum_offset = target_lum_max_offset * target_lum_ratio / (max_lum_ratio  * 256);
		}
	}

	target_lum += target_lum_offset;
	target_lum = (target_lum < 0) ? 0 : target_lum;

	cxt->flash_param.target_lum = target_lum;

	AE_LOGE("AE_TEST1: prv lum = %d, main lum = %d, target lum = %d, flash target lum = %d",
				prv_lum, main_flash_lum, cxt->cur_status.target_lum, target_lum);

	return rtn;
}

static int32_t _set_flash_notice(struct ae_ctrl_context *cxt,
                                   struct ae_flash_notice *flash_notice)
{
	int32_t rtn = AE_SUCCESS;
	enum ae_flash_mode mode = 0;

	if ((NULL == cxt) || (NULL == flash_notice)) {
		AE_LOGE("cxt %p flash_notice %p param is error!", cxt, flash_notice);
		return AE_PARAM_NULL;
	}


	mode = flash_notice->mode;
	switch (mode) {
	case AE_FLASH_PRE_BEFORE:
		ALOGE("AE_TEST: AE_FLASH_PRE_BEFORE");
		if (cxt->fdae.enable) {
			_fdae_disable_fdae(cxt);
		}
		cxt->ae_state = AE_STATE_SEARCHING;
		cxt->flash_working = 1;
		rtn = _set_before_flash(cxt);
		break;
	case AE_FLASH_PRE_LIGHTING: {
			uint32_t ratio = 0;
			if (flash_notice->flash_ratio >= 256) {
				ratio = flash_notice->flash_ratio;
			} else {
				ratio = AE_FLASH_RATIO;
			}
			ALOGE("AE_TEST: AE_FLASH_PRE_LIGHTING");
			cxt->flash_param.flash_tuning_eb = 0; /*wait for flashing*/
			cxt->flash_param.flash_ratio = ratio;
			cxt->flash_param.flash_state = AE_FLASH_STATE_ACTIVE;

			cxt->flash_param.capture_result = cxt->cur_result;
			cxt->skip_calc_num = 1;
			cxt->flash_working = 1;
			_set_quick_mode_status(cxt, 1);
		}
		break;
	case AE_FLASH_PRE_AFTER:
		ALOGE("AE_TEST: AE_FLASH_PRE_AFTER");
		cxt->flash_param.enable = 0;
		if (AE_FLASH_STATE_ACTIVE == cxt->flash_param.flash_state) {
			rtn = _process_flash(cxt, 0);
			cxt->cur_result = cxt->flash_param.before_result;
			rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_INDEX,
								(void *)&cxt->cur_result.cur_index, NULL);
			_write_to_sensor(cxt, &cxt->cur_result, 1);
			_cfg_monitor(cxt);
			cxt->flash_working = 0;
			cxt->flash_param.flash_state = AE_FLASH_STATE_MAX;
			cxt->ae_state = AE_STATE_SEARCHING;
		}
		break;

	case AE_FLASH_MAIN_AFTER:
		ALOGE("AE_TEST: AE_FLASH_MAIN_AFTER");
		cxt->flash_param.enable = 0;
		rtn = _process_flash(cxt, 0);
		cxt->cur_result = cxt->flash_param.before_result;
		cxt->ae_state = AE_STATE_SEARCHING;
		rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_INDEX,
							(void *)&cxt->cur_result.cur_index, NULL);
		_write_to_sensor(cxt, &cxt->cur_result, 1);
		if (cxt->flash_param.backup_target_lum)
			_set_target_lum(cxt, cxt->flash_param.backup_target_lum);
		cxt->flash_working = 0;
		_cfg_monitor(cxt);
		break;

	case AE_FLASH_MAIN_BEFORE:
		ALOGE("AE_TEST: AE_FLASH_MAIN_BEFORE");
		if (cxt->fdae.enable) {
			_fdae_disable_fdae(cxt);
		}

		cxt->cur_result = cxt->flash_param.capture_result;
		cxt->ae_state = AE_STATE_PRECAPTURE;
		cxt->flash_working = 1;
		cxt->flash_param.flash_state = AE_FLASH_STATE_ACTIVE;
		break;

	case AE_FLASH_MAIN_AE_MEASURE:
		ALOGE("AE_TEST: AE_FLASH_MAIN_AE_MEASURE");
		cxt->flash_param.flash_state = AE_FLASH_STATE_MAX;
		cxt->flash_param.backup_target_lum = cxt->cur_status.target_lum;
		_calc_flash_target_lum(cxt);
		_set_target_lum(cxt, cxt->flash_param.target_lum);
		_set_quick_mode_status(cxt, 1);
		_cfg_monitor_bypass(cxt);

		cxt->ctrl_quick_ae_cb_ext = 1;
		cxt->ae_state = AE_STATE_SEARCHING;
		cxt->skip_calc_num = 1;
		break;
	default:
		rtn = AE_ERROR;
		break;
	}

	return rtn;
}

static void _convert_result_to_out(struct ae_misc_calc_out *from, struct ae_calc_out *to)
{
	if (from && to) {
		to->cur_index = from->cur_index;
		to->cur_lum = from->cur_lum;
		to->cur_again = from->cur_again;
		to->cur_dgain = from->cur_dgain;
		to->cur_exp_line = from->cur_exp_line;
		to->cur_dummy = from->cur_dummy;
		to->target_lum = from->target_lum;
	}
}

static int32_t _cfg_monitor_skip(struct ae_ctrl_context *cxt, struct ae_monitor_cfg *cfg)
{
	int32_t rtn = AE_SUCCESS;

	if (cxt->isp_ops.set_monitor) {
		//AE_LOGI("AE_TEST set monitor:%d", *(uint32_t*)cfg);
		rtn = cxt->isp_ops.set_monitor(cxt->isp_ops.isp_handler, cfg);
	}

	return rtn;
}

static int32_t _cfg_monitor(struct ae_ctrl_context *cxt)
{
	int32_t rtn = AE_SUCCESS;

	_cfg_monitor_win(cxt);

	_cfg_monitor_skip(cxt, &cxt->monitor_unit.cfg);

	_cfg_monitor_bypass(cxt);

	return rtn;
}

static int32_t ae_work_init(struct ae_ctrl_context *cxt,
                              struct ae_set_work_param *work_param)
{
	int rtn = AE_SUCCESS;
	struct ae_trim monitor_trim;
	struct ae_stat_req stat_req;
	struct ae_misc_calc_out calc_result;
	enum ae_work_mode work_mode = 0;
	uint32_t is_changed_work_mode = 0;

	uint32_t new_index = 0;
	uint32_t cur_exp_gain = 0;
	uint32_t exp_line = 0;
	uint32_t gain = 0;

	if (work_param->mode >= AE_WORK_MODE_MAX) {
		AE_LOGE("work mode param is error");
		rtn = AE_ERROR;
		goto EXIT;
	}

	work_mode = work_param->mode;
	cxt->resolution_info = work_param->resolution_info;
	cxt->cap_skip_num = work_param->highflash_measure.capture_skip_num;
	cxt->cap_skip_num = cxt->cap_skip_num > 2 ? cxt->cap_skip_num : 3;
	AE_LOGI("work_mode=%d last mode=%d",work_mode,
				cxt->cur_status.work_info.mode);

	if ((AE_WORK_MODE_COMMON == work_mode
			&& AE_WORK_MODE_VIDEO == cxt->cur_status.work_info.mode)
		|| (AE_WORK_MODE_VIDEO == work_mode
				&& AE_WORK_MODE_COMMON == cxt->cur_status.work_info.mode)) {
		is_changed_work_mode = 1;
	}

	if (cxt->tuning_param_enable[work_mode]) {
		stat_req = cxt->tuning_param[work_mode].stat_req;
	} else {
		memset(&stat_req, 0, sizeof(stat_req));
	}

	if (AE_WORK_MODE_VIDEO == work_mode) {
		cxt->work_in_video = 1;
	} else {
		cxt->work_in_video = 0;
	}

	//update monitor win
	cxt->stat_req = stat_req;
	if (0 == stat_req.mode) {
		monitor_trim.x = 0;
		monitor_trim.y = 0;
		monitor_trim.w = work_param->resolution_info.frame_size.w;
		monitor_trim.h = work_param->resolution_info.frame_size.h;
	} else {
		monitor_trim.x = (work_param->resolution_info.frame_size.w - stat_req.G_width) / 2;
		monitor_trim.y = (work_param->resolution_info.frame_size.h - stat_req.G_width) / 2;
		monitor_trim.w = stat_req.G_width;
		monitor_trim.h = stat_req.G_width;
	}
	_update_monitor_unit(cxt, &monitor_trim);

	if (AE_SCENE_NORMAL == cxt->cur_status.scene) {
		rtn = _set_work_mode(cxt, work_param, 1);
		if (AE_SUCCESS != rtn) {
			goto EXIT;
		}
	}

	if (cxt->is_first || is_changed_work_mode) {
		cxt->is_first = 0;
		_set_quick_mode(cxt, 1);

		rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_INDEX,
							(void *)&cxt->start_index, NULL);
		if (AE_SUCCESS != rtn) {
			goto EXIT;
		}
		rtn = ae_misc_io_ctrl(cxt->misc_handle,
							AE_MISC_CMD_GET_EXP_BY_INDEX,
							(void *)&cxt->start_index,
							&cxt->cur_result);
	} else {
		cxt->start_index = cxt->prv_index;
		AE_LOGI("cxt->is_first = 0, start_index=%d", cxt->start_index);

		exp_line = cxt->cur_result.cur_exp_line;
		gain = cxt->cur_result.cur_again;
		cur_exp_gain = exp_line * _get_ae0_real_gain(gain);
		rtn = _get_new_index(cxt, cur_exp_gain, &new_index);
		cxt->cur_result.cur_index = new_index;
#if 0
		rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_INDEX,
							(void *)&cxt->start_index, NULL);
		if (AE_SUCCESS != rtn) {
			goto EXIT;
		}
		rtn = ae_misc_io_ctrl(cxt->misc_handle,
							AE_MISC_CMD_GET_EXP_BY_INDEX,
							(void *)&cxt->start_index,
							&cxt->cur_result);
#endif
	}
	if (cxt->has_force_bypass)
		goto EXIT;
	/*write exposure before sensor stream on*/
	if (work_param->fly_eb) {
		switch (work_mode) {
		case AE_WORK_MODE_CAPTURE:
			/*at snapshot notice*/
			AE_LOGI("work_param->highflash_measure.highflash_flag=%d", work_param->highflash_measure.highflash_flag);
			if (work_param->highflash_measure.highflash_flag) {
				cxt->monitor_unit.is_stop_monitor = 0;
				cxt->monitor_unit.cfg.skip_num = 0;
			} else {
				cxt->monitor_unit.is_stop_monitor = 1;
			}
			break;
		default:
			cxt->monitor_unit.is_stop_monitor = 0;
			cxt->monitor_unit.cfg.skip_num = cxt->monitor_skip_num;
			calc_result = cxt->cur_result;
			rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_INDEX, (void *)&calc_result.cur_index, NULL);
			_write_to_sensor(cxt, &calc_result, 1);
			break;
		}
	}

	_reset_stat(cxt, 0);

EXIT:

	return rtn;
}

static int32_t _set_init_param(struct ae_ctrl_context *cxt, struct ae_init_in *init_param)
{
	int32_t rtn = AE_SUCCESS;
	uint32_t i = 0;
	struct ae_status *cur_status = NULL;
	enum ae_work_mode cur_work_mode = AE_WORK_MODE_COMMON;
	struct ae_ev_table *ev_table = NULL;
	int32_t target_lum = 0;
	struct ae_trim monitor_trim;


	if (NULL == cxt || NULL == init_param) {
		AE_LOGE(" cxt=%p, init_param=%p param is error", cxt, init_param);
		return AE_ERROR;
	}

	AE_LOGI("tunnig_param=%d", sizeof(struct ae_tuning_param));
	AE_LOGI("param_num=%d", init_param->param_num);

	for (i=0; i < init_param->param_num && i < AE_MAX_PARAM_NUM; ++i) {
		rtn = _unpack_tunning_param(init_param->param[i].param,
				init_param->param[i].size, &cxt->tuning_param[i]);

		if (AE_SUCCESS == rtn)
			cxt->tuning_param_enable[i] = 1;
		else
			cxt->tuning_param_enable[i] = 0;

		if (AE_SUCCESS == rtn && _is_save_ae_param()) {
			save_param_to_file(i, init_param->param[i].size, init_param->param[i].param);
		}

		if (AE_SUCCESS == rtn && _is_save_ae_tbl()) {
			save_work_ae_to_file(i, &cxt->tuning_param[i]);
		}
#ifdef AE_WORK_MOD_V0
		cxt->tuning_param[i].alg_id = 0;/*jwh: need to be removed*/
#else
		cxt->tuning_param[i].alg_id = 1;/*jwh: need to be removed*/
#endif
	}

	/*parameter of common mode should not be NULL*/
	if (0 == cxt->tuning_param_enable[AE_WORK_MODE_COMMON]) {
		AE_LOGE("common param is error");
		return AE_ERROR;
	}

	cxt->resolution_info = init_param->resolution_info;
	cxt->monitor_unit.win_num = init_param->monitor_win_num;

	cxt->cur_param = &cxt->tuning_param[cur_work_mode];

	ev_table = &cxt->cur_param->ev_table;
	target_lum = _calc_target_lum(cxt->cur_param->target_lum, ev_table->default_level, ev_table);

	cur_status = &cxt->cur_status;
	cur_status->work_info.mode = cur_work_mode;
	cur_status->work_info.resolution_info = init_param->resolution_info;
	cur_status->param_from = cur_work_mode;
	cur_status->scene = AE_SCENE_NORMAL;
	cur_status->iso = AE_ISO_AUTO;
	cur_status->flicker = cxt->cur_param->flicker_index;
	cur_status->weight = AE_WEIGHT_CENTER;
	cur_status->target_lum = target_lum;
	cur_status->ev_level = ev_table->default_level;
	cur_status->night_mode = 0;

	cxt->prv_status = *cur_status;
	cxt->nxt_status = *cur_status;

	cxt->isp_ops = init_param->isp_ops;
	cxt->touch_zone_param.zone_param = cxt->cur_param->touch_param;

	if (0 == cxt->touch_zone_param.zone_param.level_0_weight) {
		cxt->touch_zone_param.zone_param.level_0_weight = 12;
		cxt->touch_zone_param.zone_param.level_1_weight = 8;
		cxt->touch_zone_param.zone_param.level_2_weight = 4;
		cxt->touch_zone_param.zone_param.level_1_percent = 50;
		cxt->touch_zone_param.zone_param.level_2_percent = 50;
	}
	cxt->exp_skip_num = cxt->cur_param->exp_skip_num;
	cxt->gain_skip_num = cxt->cur_param->gain_skip_num;


	if (_is_cap_use_preview_tbl()) {
		cxt->tuning_param_enable[AE_WORK_MODE_CAPTURE] = 0;
	}

	switch (cxt->cur_param->alg_id) {
		case 0:
			cxt->monitor_mode = AE_STATISTICS_MODE_SINGLE;
		break;

		case 1:
		{
#ifdef AE_WORK_MOD_V2
			uint32_t iso = 0, flicker = 0;
			uint32_t index = 0, line_time = 0;;
			uint32_t again = 0, dgain = 0;
			uint32_t exp_line = 0, exposure = 0;
			struct ae_frame_info frame_info = {0};

			flicker = cur_status->flicker;
			iso = cur_status->iso;
			index = cxt->cur_param->start_index;
			again = _get_real_gain(cxt->cur_param->ae_table[flicker][iso].again[index]);
			dgain = _get_real_gain(cxt->cur_param->ae_table[flicker][iso].dgain[index]);

			exposure = cxt->cur_param->ae_table[flicker][iso].exposure[index];
			line_time = cxt->resolution_info.line_time;
			exp_line = _get_exposure_line(line_time, exposure);
			frame_info.frame_id = 0;
			frame_info.again = again;
			frame_info.dgain = dgain;
			frame_info.exp_line = exp_line;
			frame_info.delay_num = 1;
			frame_info.index = index;
			frame_info.luma = 0;
			frame_info.skip_num = 0;
			frame_info.status = AE_FRAME_WRITE_SENSOR;

			init_history(cxt->history, &frame_info);
			cxt->monitor_mode = AE_STATISTICS_MODE_CONTINUE;
			cxt->monitor_skip_num = 0;
			//AE_LOGE("jwh: mode: %d, skip num : %d\n", cxt->monitor_mode, cxt->monitor_skip_num);
#else 
			cxt->monitor_mode = AE_STATISTICS_MODE_SINGLE;
#endif
		}
		break;

		default:
		break;
	}

	return AE_SUCCESS;
}

void* ae_sprd_init(struct ae_init_in *param, struct ae_init_out *result)
{
	int32_t rtn = AE_ERROR;
	struct ae_ctrl_context *cxt = NULL;
	void *misc_handle = NULL;
	struct ae_misc_init_in misc_init_in = {0};
	struct ae_misc_init_out misc_init_out = {0};
	struct ae_tuning_param *cur_param = NULL;
	struct ae_status *cur_status = NULL;
	struct ae_set_work_param work_param;

	if ((NULL == param) || (NULL == result)) {
		AE_LOGE("in %p out %p param is error!", param, result);
		rtn = AE_ERROR;
		goto ERR_EXIT;
	}

	cxt = (struct ae_ctrl_context *)malloc(sizeof(struct ae_ctrl_context));
	if (NULL == cxt) {
		rtn = AE_ALLOC_ERROR;
		AE_LOGE("alloc is error!");
		goto ERR_EXIT;
	}

	memset(cxt, 0, sizeof(*cxt));

	rtn = _set_init_param(cxt, param);
	if (rtn) {
		AE_LOGE("init param is failed!");
		goto ERR_EXIT;
	}

	cur_status = &cxt->cur_status;
	cur_param = cxt->cur_param;
	misc_init_in.alg_id = cur_param->alg_id;
	misc_init_in.start_index = cur_param->start_index;
	misc_init_in.cvt_mode = cxt->monitor_mode;
	misc_handle = ae_misc_init(&misc_init_in, &misc_init_out);
	if (NULL == misc_handle) {
		AE_LOGE("misc alloc is error!");
		rtn = AE_ALLOC_ERROR;
		goto ERR_EXIT;
	}

	cxt->misc_handle = misc_handle;
	cxt->start_id = AE_START_ID;
	cxt->end_id = AE_END_ID;

	cxt->is_mlog = _is_ae_mlog();
	cxt->start_index = cur_param->start_index;
	cxt->is_first = 1;
	cxt->monitor_unit.cfg.skip_num = 0;
	cxt->skip_calc_num = 0;
	cxt->ctrl_quick_ae_cb_ext = 0;

	cxt->monitor_skip_num = MAX(cxt->exp_skip_num, cxt->gain_skip_num);
	cxt->monitor_period_num = cxt->monitor_skip_num + 1;
	cxt->exp_period_num = cxt->monitor_period_num - (cxt->exp_skip_num + 1);
	cxt->gain_period_num = cxt->monitor_period_num - (cxt->gain_skip_num + 1);

	cxt->stab_num = 0;
	cxt->camera_id = param->camera_id;
	cxt->flash_working = 0;
	cxt->work_in_video = 0;
	cxt->has_force_bypass = param->has_force_bypass;
	cxt->force_quick_mode = 0;

	AE_LOGI("has_force_bypass=%d", cxt->has_force_bypass);
	_reset_stat(cxt, 0);

	pthread_mutex_init(&cxt->status_lock, NULL);

	{
		struct ae_trim trim;

		trim.x = 0;
		trim.y = 0;
		trim.w = cxt->resolution_info.frame_size.w;
		trim.h = cxt->resolution_info.frame_size.h;
		_update_monitor_unit(cxt, &trim);
	}

	work_param.mode = AE_WORK_MODE_COMMON;
	work_param.fly_eb = 1;
	work_param.resolution_info = param->resolution_info;
	rtn = _set_work_mode(cxt, &work_param, 1);
	if (AE_SUCCESS != rtn) {
		goto ERR_EXIT;
	}

	cxt->monitor_unit.cfg.skip_num = cxt->monitor_skip_num;
	cxt->monitor_unit.mode = cxt->monitor_mode;
	_cfg_set_aem_mode(cxt);/*set ae monitor work mode*/

	_fdae_init(cxt);
	return (void *)cxt;

ERR_EXIT:

	if (NULL != misc_handle) {
		ae_misc_deinit(misc_handle, NULL, NULL);
		misc_handle = NULL;
	}

	if (NULL != cxt) {
		free(cxt);
		cxt = NULL;
	}

	return NULL;
}

int32_t ae_sprd_deinit(void *handle, void *in_param, void *out_param)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_ctrl_context *cxt = NULL;
	UNUSED(in_param);
	UNUSED(out_param);

	rtn = _check_handle(handle);
	if (AE_SUCCESS != rtn) {
		return AE_ERROR;
	}

	cxt = (struct ae_ctrl_context *)handle;

	rtn = ae_misc_deinit(cxt->misc_handle, NULL, NULL);
	if (AE_SUCCESS != rtn) {
		AE_LOGE("misc deinit is failed!");
		return AE_ERROR;
	}

	pthread_mutex_destroy(&cxt->status_lock);

	memset(cxt, 0, sizeof(*cxt));
	free(cxt);

	return AE_SUCCESS;
}

static int32_t _calc_post_proc(struct ae_ctrl_context *cxt,
                                   struct ae_misc_calc_in *misc_calc_in,
                                   struct ae_misc_calc_out *misc_calc_out)
{
	int32_t rtn = AE_SUCCESS;
	int32_t is_converged = 0;
	int32_t need_exp_write_sensor = 1;

	if (NULL == cxt
		|| NULL == misc_calc_in
		|| NULL == misc_calc_out) {
			AE_LOGE("cxt %p in %p out %p param is error!", cxt, misc_calc_in, misc_calc_out);
			return AE_ERROR;
	}

	cxt->write_sensor = 0;

	//AE_LOGI("AE_TEST: --------_calc_post_proc----start----num:%d, %d, %d", cxt->cur_result.exp.num, misc_calc_out->cur_index,cxt->prv_index);

	cxt->write_conter = 0;

	if ((0 == cxt->cur_result.exp.num)
		&& (cxt->prv_index != misc_calc_out->cur_index)) {
		cxt->prv_index = misc_calc_out->cur_index;
		//cxt->prv_dummy = misc_calc_out->cur_dummy; // Bug 531021 mark

	}

	AE_LOGE("jwh: cvg: cur: %d, prv: %d\n", misc_calc_out->cur_index, cxt->prv_index);
	AE_LOGE("jwh: cvg: cur: %d %d, prv: %d, %d\n", \
		misc_calc_out->cur_exp_line, misc_calc_out->cur_again,\
		cxt->prv_result.cur_exp_line, cxt->prv_result.cur_again);
	//if (misc_calc_out->cur_index == cxt->prv_index)
	if ((misc_calc_out->cur_exp_line == cxt->prv_result.cur_exp_line) \
		&& (misc_calc_out->cur_again == cxt->prv_result.cur_again)) {

		if (cxt->flash_param.flash_state == AE_FLASH_STATE_ACTIVE) {
			if (cxt->convergerd_num >= AE_CONVERGED_NUM) {
				is_converged = 1;
			}
			cxt->convergerd_num ++;
		} else {
			is_converged = 1;
		}
		need_exp_write_sensor = 0;
		if (misc_calc_out->cur_dummy != cxt->prv_result.cur_dummy) {
			need_exp_write_sensor = 1;
		}
		cxt->prv_dummy = misc_calc_out->cur_dummy;
	} else {
		cxt->convergerd_num = 0;
		is_converged = 0;
		cxt->prv_index = misc_calc_out->cur_index;
		cxt->prv_dummy = misc_calc_out->cur_dummy;

		cxt->write_eb = 1;

		//misc_calc_out->cur_exp_line = cxt->cur_result.exp.tab[0].exp_line;
		//misc_calc_out->cur_dummy = cxt->cur_result.exp.tab[0].exp_dummy;
		//misc_calc_out->cur_again = cxt->cur_result.exp.tab[0].again;
		//misc_calc_out->cur_dgain = cxt->cur_result.exp.tab[0].dgain;
	}
	/*force flash ae calc callback*/
	if (cxt->flash_param.flash_state == AE_FLASH_STATE_ACTIVE && cxt->flash_param.enable) {
		cxt->flash_param.calc_time ++;
		if (AE_CALC_TIMES < cxt->flash_param.calc_time) {
			cxt->flash_param.calc_time = 0;
			is_converged = 1;
			AE_LOGI("force flash ae calc callback");
		}
	} else {
		cxt->flash_param.calc_time = 0;
	}

	AE_LOGE("jwh: is converge: %d\n", is_converged);
	
	if (is_converged) {
		cxt->ae_state = AE_STATE_CONVERGED;
		cxt->stab_cnt++;

		//check flash
		if (cxt->flash_param.enable) {
			switch (cxt->flash_param.flash_state) {
			case AE_FLASH_STATE_BEFORE:
            #ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
				cxt->flash_param.before_result = *misc_calc_out;
				if (cxt->isp_ops.callback) {
					AE_LOGI("AE_CB_CONVERGED");
					(*cxt->isp_ops.callback)(cxt->isp_ops.isp_handler, AE_CB_CONVERGED);
					cxt->flash_param.flash_state = AE_FLASH_STATE_MAX;
				}
            #endif
				break;
			case AE_FLASH_STATE_ACTIVE:
				cxt->ae_state = AE_STATE_FLASH_REQUIRED;
				cxt->flash_param.preflash_converged_result = *misc_calc_out;

				_calc_premain_exposure(cxt, misc_calc_in, misc_calc_out);
				if (cxt->isp_ops.callback) {
					AE_LOGI("AE_CB_FLASHING_CONVERGED");
					(*cxt->isp_ops.callback)(cxt->isp_ops.isp_handler, AE_CB_FLASHING_CONVERGED);
				}
				need_exp_write_sensor = 0;
				cxt->cur_result = *misc_calc_out;
				break;
			default:
				break;
			}
		}
	} else {
		cxt->ae_state = AE_STATE_SEARCHING;
		cxt->stab_cnt = 0;
	}

	if (cxt->cur_status.is_quick_mode) {
		if (is_converged  && (cxt->ctrl_quick_ae_cb_ext == 0)) {
			_set_quick_mode(cxt, 0);
		}

		if (cxt->ctrl_quick_ae_cb_ext) {
			_set_quick_mode(cxt, 0);
			AE_LOGI("premain %d lum %d;  qk %d lum %d", cxt->flash_param.capture_result.cur_index, cxt->flash_param.capture_result.cur_lum,
											misc_calc_out->cur_index, misc_calc_out->cur_lum);
			//_write_to_sensor(cxt, misc_calc_out, 0);
			cxt->flash_param.capture_result = *misc_calc_out;
			cxt->flash_param.highflash_skip_cnt = 0;
			cxt->flash_param.highflash_skip_en = 1;
			cxt->ctrl_quick_ae_cb_ext = 0;
			cxt->ae_state = AE_STATE_LOCKED;
			need_exp_write_sensor = 1;
		}
	}

	AE_LOGE("jwh: ae stat: %d\n", cxt->ae_state);

#ifdef AE_WORK_MOD_V2
	need_exp_write_sensor = 1;
#endif
	AE_LOGE("jwh: is write sensor: %d\n", need_exp_write_sensor);

	if (need_exp_write_sensor) {
		rtn = _write_to_sensor(cxt, misc_calc_out, 0);
		cxt->write_conter++;

		if (AE_SUCCESS == rtn) {
			cxt->write_sensor = 1;
		}

		//AE_LOGI("AE_TEST: --------_calc_post_proc--------num:%d, conter:%d", cxt->cur_result.exp.num, cxt->write_conter);
#ifndef AE_WORK_MOD_V2
		if ((0 == cxt->cur_result.exp.num)
			||(cxt->cur_result.exp.num == cxt->write_conter))
		{
			cxt->write_eb = 0;
		}
#endif

	}else{
#ifndef AE_WORK_MOD_V2
		if (cxt->cur_result.exp.num == cxt->write_conter)
		{
			cxt->write_eb = 0;
		}
#endif
	}

	return rtn;
}


static int32_t _is_calc_lum(struct ae_ctrl_context *cxt)
{
	int32_t is_eb = 1;

	switch (cxt->ae_state) {
	case AE_STATE_LOCKED:
	case AE_STATE_FLASH_REQUIRED:
	case AE_STATE_PRECAPTURE:
		is_eb = 0;
		break;
	default:
		is_eb = 1;
		break;
	}

	return is_eb;
}

static int32_t _is_stab(struct ae_ctrl_context *cxt)
{
	int32_t is_stab = 0;

	switch (cxt->ae_state) {
	case AE_STATE_FLASH_REQUIRED:
	case AE_STATE_PRECAPTURE:
	case AE_STATE_CONVERGED:
	case AE_STATE_LOCKED:
		is_stab = 1;
		break;
	default:
		break;
	}

	return is_stab;
}

static int32_t _set_frame_cnt(struct ae_ctrl_context *cxt)
{
	int32_t rtn = AE_ERROR;


	if (NULL == cxt) {
		AE_LOGE("cxt %p param is error!", cxt);
		return AE_PARAM_NULL;
	}
	if (cxt->flash_param.highflash_skip_en) {
		cxt->flash_param.highflash_skip_cnt++;
		if (cxt->flash_param.highflash_skip_cnt == cxt->cap_skip_num) {
			if (cxt->isp_ops.callback) {
				(*cxt->isp_ops.callback)(cxt->isp_ops.isp_handler, AE_CB_QUICKMODE_DOWN);
				AE_LOGI("flash quick mode AE_CB_QUICKMODE_DOWN");
				cxt->flash_param.highflash_skip_cnt = 0;
			}
			cxt->flash_param.highflash_skip_en = 0;
		}
	}

#if 0
	if (cxt->is_drop_stat) {
		rtn = AE_ERROR;
		goto DROP_STAT;
	}
#endif

	if (!_is_calc_lum(cxt)) {
		rtn = AE_ERROR;
		goto EXIT;
	}

#if 0
	if (0 != (cxt->skip_frame_cnt % cxt->monitor_period_num)) {
		rtn = _write_to_sensor(cxt, &cxt->cur_result, 0);
		goto EXIT;
	}
#endif

EXIT:
	AE_LOGV("ae_state=%d, frame_cnt=%d", cxt->ae_state, cxt->skip_frame_cnt);

	cxt->skip_frame_cnt++;

DROP_STAT:

	AE_LOGV("rtn=%d", rtn);

	return rtn;
}

#ifndef  AE_WORK_MOD_V2
/*mode 1: ae monitor work in normal mode*/
int32_t ae_sprd_calculation(void* handle, struct ae_calc_in *param, struct ae_calc_out *result)
{
	int32_t rtn = AE_ERROR;
	struct ae_ctrl_context *cxt = NULL;
	struct ae_misc_calc_in misc_calc_in = {0};
	struct ae_misc_calc_out misc_calc_out = {0};
	uint32_t skip_flag = 0;
	uint32_t ae_pause = 0;

	if ((NULL == param) || (NULL == result)) {
		AE_LOGE("in %p out %p param is error!", param, result);
		return AE_PARAM_NULL;
	}

	rtn = _check_handle(handle);
	AE_RETURN_IF_FAIL(rtn, ("handle = %p", handle));

	cxt = (struct ae_ctrl_context *)handle;
	pthread_mutex_lock(&cxt->status_lock);

	if (cxt->is_drop_stat) {
		cxt->is_drop_stat = 0;
		rtn = AE_ERROR;
		goto DROP_STAT;
	}

	if (cxt->has_force_bypass) {
		ae_pause = 1;
	}
	if (!_is_calc_lum(cxt)) {
		rtn = AE_ERROR;
		goto EXIT;
	}

	if (cxt->skip_calc_num) {
		cxt->skip_calc_num --;
		skip_flag = 1;
		rtn = AE_ERROR;
		goto EXIT;
	}

	_update_status(cxt);
	rtn = AE_SUCCESS;

	if (cxt->force_quick_mode) {
		_set_quick_mode(cxt, 1);
	}

	misc_calc_in.awb_gain.r = param->awb_gain_r;
	misc_calc_in.awb_gain.g = param->awb_gain_g;
	misc_calc_in.awb_gain.b = param->awb_gain_b;
	misc_calc_in.stat_img = param->stat_img;
	misc_calc_in.win_num = cxt->monitor_unit.win_num;
	misc_calc_in.win_size = cxt->monitor_unit.win_size;
	misc_calc_in.stat_mode = cxt->stat_req.mode;
	misc_calc_in.fd_state = cxt->fdae.state;
	misc_calc_in.ae_pause = ae_pause;
/*jwh*/
        rtn = _get_bv_by_lum(cxt, &cxt->cur_result, &misc_calc_in.bv);
        if (AE_SUCCESS == rtn) {
                AE_LOGE(" cur bv :%d!\n", misc_calc_in.bv);
        }
/*jwh*/
	rtn = ae_misc_calculation(cxt->misc_handle, &misc_calc_in, &misc_calc_out);
	if (AE_SUCCESS == rtn) {
		cxt->valid_stat = 1;
		cxt->cur_result = misc_calc_out;

		rtn = _calc_post_proc(cxt, &misc_calc_in, &misc_calc_out);
		cxt->prv_result = misc_calc_out;

		if (cxt->is_mlog) {
			save_to_mlog_file(cxt,0, &cxt->cur_result);
		}
	} else {
		AE_LOGE("misc calc rtn %d is error!", rtn);
	}

EXIT:
	AE_LOGV("ae_state=%d, frame_cnt=%d", cxt->ae_state, cxt->skip_frame_cnt);

DROP_STAT:

	if (result) {
		_convert_result_to_out(&cxt->cur_result, result);
		cxt->start_index = result->cur_index;
		result->line_time = cxt->resolution_info.line_time;
		result->frame_line = cxt->resolution_info.frame_line;
		result->is_stab = _is_stab(cxt);
	}


	_cfg_monitor_bypass(cxt);

	AE_LOGV("calc_lum=%d, cur_index=%d, is_stab=%d", result->cur_lum, result->cur_index, result->is_stab);
	AE_LOGV("target_lum=%d, ae_state=%d", result->target_lum, cxt->ae_state);

	pthread_mutex_unlock(&cxt->status_lock);
	if (result && result->is_stab) {
		if (cxt->isp_ops.callback) {
			AE_LOGI("AE_CB_STAB_NOTIFY");
			(*cxt->isp_ops.callback)(cxt->isp_ops.isp_handler, AE_CB_STAB_NOTIFY);
		}
	}
	AE_LOGV("rtn=%d", rtn);

	return rtn;
}
#else
/*mode 0: ae monitor work in continue mode*/
int32_t ae_sprd_calculation(void* handle, struct ae_calc_in *param, struct ae_calc_out *result)
{
	int32_t rtn = AE_ERROR;
	struct ae_ctrl_context *cxt = NULL;
	struct ae_misc_calc_in misc_calc_in = {0};
	struct ae_misc_calc_out misc_calc_out = {0};
	uint32_t skip_flag = 0;
	struct ae_frame_info frame_info = {0};
	struct ae_time eof = {0};
	struct ae_time sensor_write = {0};
	uint32_t frame_time = 0;

	if ((NULL == param) || (NULL == result)) {
		AE_LOGE("in %p out %p param is error!", param, result);
		return AE_PARAM_NULL;
	}

	rtn = _check_handle(handle);
	AE_RETURN_IF_FAIL(rtn, ("handle = %p", handle));

	cxt = (struct ae_ctrl_context *)handle;
	pthread_mutex_lock(&cxt->status_lock);

	cxt->frame_id++;
	frame_info.frame_id = cxt->frame_id;
	frame_info.status = AE_FRAME_SKIP;

	AE_LOGX("-----------------frame id=%d", cxt->frame_id);

	if (cxt->has_force_bypass) {
		rtn = AE_SUCCESS;
		goto EXIT;
	}
	if (!_is_calc_lum(cxt)) {
		rtn = AE_ERROR;
		goto EXIT;
	}

	_update_status(cxt);
	rtn = AE_SUCCESS;

	misc_calc_in.awb_gain.r = param->awb_gain_r;
	misc_calc_in.awb_gain.g = param->awb_gain_g;
	misc_calc_in.awb_gain.b = param->awb_gain_b;
	misc_calc_in.stat_img = param->stat_img;
	misc_calc_in.win_num = cxt->monitor_unit.win_num;
	misc_calc_in.win_size = cxt->monitor_unit.win_size;
	misc_calc_in.stat_mode = cxt->stat_req.mode;
	/*jwh*/
	rtn = _get_bv_by_lum(cxt, &cxt->cur_result, &misc_calc_in.bv);
	if (AE_SUCCESS == rtn) {
		AE_LOGE(" cur bv :%d!\n", misc_calc_in.bv);
	}
	/*jwh*/

	misc_calc_in.history = &cxt->history;
	rtn = ae_misc_calculation(cxt->misc_handle, &misc_calc_in, &misc_calc_out);
	if (AE_SUCCESS == rtn) {
		cxt->valid_stat = 1;
		cxt->cur_result = misc_calc_out;

		rtn = _calc_post_proc(cxt, &misc_calc_in, &misc_calc_out);

		/*why????*/
		if (AE_SUCCESS == rtn) {
			cxt->isp_ops.get_system_time(cxt->isp_ops.isp_handler, &sensor_write.sec, &sensor_write.usec);
			eof.sec = param->sec;
			eof.usec = param->usec;
			frame_info.delay_num = get_write_delay(&eof, &sensor_write, frame_time);
			frame_info.status = AE_FRAME_WRITE_SENSOR;
		}

		if (cxt->write_sensor) {
			uint32_t frame_lines = 0;
			uint32_t line_time = cxt->resolution_info.line_time;

			cxt->isp_ops.get_system_time(cxt->isp_ops.isp_handler, &sensor_write.sec, &sensor_write.usec);
			eof.sec = param->sec;
			eof.usec = param->usec;

			frame_lines = cxt->cur_result.cur_exp_line + cxt->cur_result.cur_dummy;
			frame_time = frame_lines * line_time /10;/*us*/
			frame_info.delay_num = get_write_delay(&eof, &sensor_write, frame_time);
			frame_info.status = AE_FRAME_WRITE_SENSOR;
		}

		frame_info.again = misc_calc_out.cur_again;
		frame_info.dgain = misc_calc_out.cur_dgain;
		frame_info.exp_line = misc_calc_out.cur_exp_line;
		frame_info.luma = misc_calc_out.cur_lum;
		frame_info.index = misc_calc_out.cur_index;
		frame_info.skip_num = 0;

		if (cxt->is_mlog) {
			save_to_mlog_file(cxt,0, &cxt->cur_result);
		}
	} else {
		AE_LOGE("misc calc rtn %d is error!", rtn);
	}

EXIT:
	AE_LOGV("ae_state=%d, frame_cnt=%d", cxt->ae_state, cxt->skip_frame_cnt);

DROP_STAT:

	if (result) {
		_convert_result_to_out(&cxt->cur_result, result);
		cxt->start_index = result->cur_index;
		result->line_time = cxt->resolution_info.line_time;
		result->frame_line = cxt->resolution_info.frame_line;
		result->is_stab = _is_stab(cxt);
	}

	AE_LOGE("calc_lum=%d, cur_index=%d, is_stab=%d", result->cur_lum, result->cur_index, result->is_stab);
	AE_LOGV("target_lum=%d, ae_state=%d", result->target_lum, cxt->ae_state);

	save_frame_info(&cxt->history, &frame_info);

	pthread_mutex_unlock(&cxt->status_lock);
	if (result && result->is_stab) {
		if (cxt->isp_ops.callback) {
			AE_LOGI("AE_CB_STAB_NOTIFY");
			(*cxt->isp_ops.callback)(cxt->isp_ops.isp_handler, AE_CB_STAB_NOTIFY);
		}
	}
	AE_LOGV("rtn=%d", rtn);

	return rtn;
}

#endif

static int32_t _sof_handler(struct ae_ctrl_context *cxt)
{
	int32_t rtn = AE_SUCCESS;
	int32_t write_num = 1;
	if (cxt->has_force_bypass)
		goto EXIT;
#ifndef AE_WORK_MOD_V2
	if(1 == cxt->write_eb)
	{
		cxt->ae_state = AE_STATE_LOCKED;
		_write_exp_gain(cxt);

		if (0 == cxt->write_eb) {
			cxt->skip_calc_num = cxt->monitor_skip_num;
			cxt->ae_state = AE_STATE_SEARCHING;
		}
	}
#endif
	_fdae_update_state(cxt);
EXIT:
	return rtn ;
}

static int32_t _set_pause(struct ae_ctrl_context *cxt)
{
	int32_t rtn = AE_SUCCESS;

	if (NULL == cxt) {
		AE_LOGE("ae cxt is NULL error!");
		return AE_ERROR;
	}

	if (cxt->is_force_lock) {
		AE_LOGI("warning force lock!");
		return AE_ERROR;
	}

	if (0 >= cxt->pause_ref_cnt) {
		cxt->pause_ae_state = cxt->ae_state;
		cxt->backup_result = cxt->cur_result;
		cxt->ae_state = AE_STATE_LOCKED;
		cxt->pause_ref_cnt = 0;
		//callback
		if (cxt->isp_ops.callback) {
			AE_LOGI("AE_CB_AE_LOCK_NOTIFY");
			(*cxt->isp_ops.callback)(cxt->isp_ops.isp_handler, AE_CB_AE_LOCK_NOTIFY);
		}
	}

	++cxt->pause_ref_cnt;

	AE_LOGI("ae_state=%d", cxt->ae_state);

	return rtn;
}

static int32_t _set_restore(struct ae_ctrl_context *cxt, struct ae_calc_out *result)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_misc_calc_out cur_result;

	if (NULL == cxt) {
		AE_LOGE("ae cxt is NULL error!");
		return AE_ERROR;
	}

	if (cxt->is_force_lock) {
		AE_LOGI("warning force lock!");
		return AE_ERROR;
	}

	if (0 >= cxt->pause_ref_cnt) {
		AE_LOGI("warning call at out of rule!");
		return AE_SUCCESS;
	}

	--cxt->pause_ref_cnt;

	if (AE_STATE_LOCKED == cxt->ae_state
		&& 0 == cxt->pause_ref_cnt) {
		cur_result = cxt->cur_result;
		cxt->cur_result = cxt->backup_result;

		if (result) {
			_convert_result_to_out(&cxt->cur_result, result);
			//_cfg_monitor_bypass(cxt);
		}
		if ((cur_result.cur_again != cxt->cur_result.cur_again)
			|| (cur_result.cur_exp_line != cxt->cur_result.cur_exp_line)
			|| (cur_result.cur_dummy != cxt->cur_result.cur_dummy)) {
			_write_to_sensor(cxt, &cxt->cur_result, 1);
		}
		cxt->ae_state = cxt->pause_ae_state;
		//callback
		if (cxt->isp_ops.callback) {
			AE_LOGI("AE_CB_AE_UNLOCK_NOTIFY");
			(*cxt->isp_ops.callback)(cxt->isp_ops.isp_handler, AE_CB_AE_UNLOCK_NOTIFY);
		}
	}

	AE_LOGI("ae_state=%d", cxt->ae_state);

	return rtn;
}

static int32_t _set_force_lock(struct ae_ctrl_context *cxt, uint32_t is_force)
{
	int32_t rtn = AE_SUCCESS;

	cxt->is_force_lock = is_force;
	if (is_force) {
		cxt->force_ae_state = cxt->ae_state;
		cxt->ae_state = AE_STATE_LOCKED;
	} else {
		cxt->ae_state = AE_STATE_SEARCHING;
	}

	AE_LOGI("is_force=%d, ae_state=%d", is_force, cxt->ae_state);

	return rtn;
}

static int32_t _ae_online_ctrl_set(struct ae_ctrl_context *cxt, void *param)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_online_ctrl* ae_ctrl_ptr = (struct ae_online_ctrl*)param;
	struct ae_misc_calc_out ctrl_result;
	uint32_t ctrl_index = 0;

	if ((NULL == cxt) || (NULL == param)) {
		AE_LOGE("in %p out %p param is error!", cxt, param);
		return AE_PARAM_NULL;
	}

	if (AE_CTRL_SET_INDEX == ae_ctrl_ptr->mode) {
		AE_LOGI("index:%d", ae_ctrl_ptr->index);
		ctrl_index = ae_ctrl_ptr->index;
		rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_INDEX, (void *)&ctrl_index, NULL);
		cxt->prv_index = ctrl_index;
		rtn = ae_misc_io_ctrl(cxt->misc_handle,
					AE_MISC_CMD_GET_EXP_BY_INDEX,
					(void *)&ctrl_index,
					&ctrl_result);
		if (AE_SUCCESS == rtn) {
			_write_to_sensor(cxt, &ctrl_result, 1);
		}
		cxt->cur_result.cur_index     = ctrl_result.cur_index;
		cxt->cur_result.cur_again     = ctrl_result.cur_again;
		cxt->cur_result.cur_dgain     = ctrl_result.cur_dgain;
		cxt->cur_result.cur_exp_line  = ctrl_result.cur_exp_line;
		cxt->cur_result.cur_exposure  = ctrl_result.cur_exposure;
		cxt->cur_result.cur_dummy     = ctrl_result.cur_dummy;
		cxt->cur_result.min_index     = ctrl_result.min_index;
		cxt->cur_result.max_index     = ctrl_result.max_index;
	} else {
		uint32_t new_index = 0;
		uint32_t cur_exp_gain = 0;
		uint32_t exp_line = 0;
		uint32_t gain = 0;

		AE_LOGI("index:%d, shutter:%d, again:0x%04x",
			ae_ctrl_ptr->index, ae_ctrl_ptr->shutter, ae_ctrl_ptr->again);
		ctrl_result.cur_exp_line = ae_ctrl_ptr->shutter;
		ctrl_result.cur_again = ae_ctrl_ptr->again;
		ctrl_result.cur_dummy = ae_ctrl_ptr->dummy;

		_write_to_sensor(cxt, &ctrl_result, 1);
	}

EXIT:

	return rtn;
}

static int32_t _ae_online_ctrl_get(struct ae_ctrl_context *cxt, void *result)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_online_ctrl* ae_ctrl_ptr = (struct ae_online_ctrl*)result;


	if ((NULL == cxt) || (NULL == result)) {
		AE_LOGE("in %p out %p param is error!", cxt, result);
		return AE_PARAM_NULL;
	}
	ae_ctrl_ptr->index = cxt->cur_result.cur_index;
	ae_ctrl_ptr->lum = cxt->cur_result.cur_lum;
	ae_ctrl_ptr->shutter = cxt->cur_result.cur_exp_line;
	ae_ctrl_ptr->dummy= cxt->cur_result.cur_dummy;
	ae_ctrl_ptr->again = cxt->cur_result.cur_again;
	ae_ctrl_ptr->dgain = cxt->cur_result.cur_dgain;
	ae_ctrl_ptr->skipa = 0;
	ae_ctrl_ptr->skipd = 0;

	AE_LOGI("mode:%d, index:%d, shutter:%d",
			ae_ctrl_ptr->mode, ae_ctrl_ptr->index, ae_ctrl_ptr->shutter);

	return rtn;
}

static int32_t _tool_online_ctrl(struct ae_ctrl_context *cxt, void *param, void *result)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_online_ctrl *ae_ctrl_ptr = (struct ae_online_ctrl*)param;

	AE_LOGI("mode=%d", ae_ctrl_ptr->mode);
	if ((AE_CTRL_SET_INDEX == ae_ctrl_ptr->mode)
		|| (AE_CTRL_SET == ae_ctrl_ptr->mode)) {
		rtn = _ae_online_ctrl_set(cxt, param);
	} else {
		rtn = _ae_online_ctrl_get(cxt, result);
	}

	return rtn;
}

/*scene mode can not change ae table & weight table*/
static int32_t _is_work_to_scene(struct ae_ctrl_context *cxt,
									struct ae_status *next_status)
{
	int32_t is_scene = 0;


	if ((NULL == cxt) || (NULL == next_status)) {
		AE_LOGE("cxt %p next_status %p param is error!", cxt, next_status);
		return AE_PARAM_NULL;
	}

	if ((cxt->update_list.is_scene
		&& AE_SCENE_NORMAL != next_status->scene)
		|| (0 == cxt->update_list.is_scene
			&& AE_SCENE_NORMAL != cxt->cur_status.scene)) {
		is_scene = 1;
	}

	AE_LOGI("is_scene=%d", is_scene);
	return is_scene;
}

/**************************************************************************/
/* Begin: FDAE related functions                                          */

static int32_t _fdae_init(struct ae_ctrl_context *cxt)
{
	int32_t rtn = AE_SUCCESS;
	int32_t i = 0;

	cxt->fdae.curr_frame_idx = 0;
	cxt->fdae.receive_face_frame_idx = 0;
	cxt->fdae.last_disable_frame_idx = 0;
	cxt->fdae.allow_to_work = 1;
    cxt->fdae.enable = 1;
    cxt->fdae.state = FDAE_STATE_OFF;
    cxt->fdae.is_ae_locked = 0;
	cxt->fdae.frames_to_lock = 0;
	cxt->fdae.face_info.face_num = 0;
	cxt->fdae.face_info_prev.face_num = 0;
	cxt->fdae.frames_no_face = 0;

	cxt->fdae.param_face_weight = 148;
	cxt->fdae.param_convergence_speed = 2;
	cxt->fdae.param_lock_ae = 3;
	cxt->fdae.param_lock_weight_has_face = 20;
	cxt->fdae.param_lock_weight_no_face = 10;

	// For tuning parameter: read parameter from file
	{
		FILE *fp = fopen("/data/fdae_param.txt", "rt");
		if (!fp)
			AE_LOGI("FDAE: failed open fdae_param.txt");
		if (fp) {
			int32_t val0 = 0, val1 = 0, val2 = 0, val3 = 0;
			fscanf(fp, "%d %d %d %d", &val0, &val1, &val2, &val3);
			if (val0 > 0) cxt->fdae.param_face_weight = val0;
			if (val1 > 0) cxt->fdae.param_convergence_speed = val1;
			if (val2 > 0) cxt->fdae.param_lock_ae = val2;
			if (val3 > 0) cxt->fdae.param_lock_weight_has_face = val3;
			fclose(fp);
		}
	}

	AE_LOGI("FDAE param: param_face_weight=%d, convergence_speed=%d, param_lock_ae=%d, param_lock_weight_has_face=%d",
			cxt->fdae.param_face_weight, cxt->fdae.param_convergence_speed,
			cxt->fdae.param_lock_ae, cxt->fdae.param_lock_weight_has_face);

	for (i = 0; i < AE_WEIGHT_TABLE_SIZE; ++i) {
		cxt->fdae.total_w[0] += cxt->cur_param->weight_table[0].weight[i];
		cxt->fdae.total_w[1] += cxt->cur_param->weight_table[1].weight[i];
		cxt->fdae.total_w[2] += cxt->cur_param->weight_table[2].weight[i];
	}
	return rtn;
}

static void _fdae_lock_ae(struct ae_ctrl_context *cxt)
{
	if (cxt->fdae.state != FDAE_STATE_OFF) {
		if (!cxt->fdae.is_ae_locked) {
			_set_pause(cxt);
			cxt->fdae.is_ae_locked = 1;
		}
		cxt->fdae.state = FDAE_STATE_ON_LOCK_AE;
		cxt->fdae.frames_to_lock = cxt->fdae.param_lock_ae;

		AE_LOGI("FDAE: ->Lock AE, frame_idx=%d", cxt->fdae.curr_frame_idx);
	}
}

#define FRAMES_NO_FACE_FOR_SURE  2
static int32_t _fd_process(struct ae_ctrl_context *cxt, void *param)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_fd_param *fd = (struct ae_fd_param*)param;

	if (NULL == cxt || NULL == param) {
		AE_LOGE("cxt %p param %p param is error!", cxt, param);
		return AE_ERROR;
	}

	cxt->fdae.receive_face_frame_idx = cxt->fdae.curr_frame_idx;

    if (!(cxt->fdae.allow_to_work && cxt->fdae.enable)) {
        cxt->fdae.face_info.face_num = 0;
        return rtn;
    }

	if (fd->face_num > 0) {
		memcpy(&(cxt->fdae.face_info), fd, sizeof(struct ae_fd_param));
		cxt->fdae.frames_no_face = 0;
	} else {
		cxt->fdae.face_info.face_num = 0;
		cxt->fdae.frames_no_face++;
	}

	AE_LOGI("FDAE: receive face_num=%d, frame_idx=%d", fd->face_num, cxt->fdae.curr_frame_idx);

	if (cxt->fdae.frames_no_face == FRAMES_NO_FACE_FOR_SURE){
		if (cxt->fdae.state != FDAE_STATE_ON_LOCK_AE &&
			cxt->fdae.state != FDAE_STATE_OFF) {
			_fdae_lock_ae(cxt);
		}
	}

	return rtn;
}

static void _fdae_enter_fdae_mode(struct ae_ctrl_context *cxt)
{
	// disable "exp_anti" because it will also change the AE weight table
	uint32_t exp_anti_enable = 0;
	uint32_t convergence_speed = cxt->fdae.param_convergence_speed;
	_set_exp_anti(cxt, (void*)&exp_anti_enable);

	// set a slow convergence_speed
	ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_CONVERGE_SPEED, &convergence_speed,
	&(cxt->fdae.ae_convergence_speed));
}

static void _fdae_leave_fdae_mode(struct ae_ctrl_context *cxt)
{
	/* restore the AE states */
	ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_CONVERGE_SPEED, &(cxt->fdae.ae_convergence_speed), NULL);

    /* restore the AE weight table */
    {
        int16_t weight_mode = cxt->cur_status.weight;
        struct ae_weight_table *cur_base_tbl = &(cxt->cur_param->weight_table[weight_mode]);
        ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_WEIGHT_TABLE,
                        (void *)cur_base_tbl, NULL);
    }

    /* Unlock AE, if it is locked by FDAE */
    if (cxt->fdae.is_ae_locked) {
		struct ae_calc_out result;
        _set_restore(cxt, &result);
        cxt->fdae.is_ae_locked = 0;
    }
}

static void _fdae_disable_fdae(struct ae_ctrl_context *cxt)
{
	if (cxt->fdae.enable) {
		cxt->fdae.enable = 0;
		if (cxt->fdae.state != FDAE_STATE_OFF) {
			_fdae_leave_fdae_mode(cxt);
			cxt->fdae.state = FDAE_STATE_OFF;
			cxt->fdae.last_disable_frame_idx = cxt->fdae.curr_frame_idx;
		}
		AE_LOGI("FDAE: ->disable, frame_idx=%d", cxt->fdae.curr_frame_idx);
	}
}

static void _fdae_enable_fdae(struct ae_ctrl_context *cxt)
{
	if (!cxt->fdae.enable) {
		cxt->fdae.enable = 1;
		cxt->fdae.state = FDAE_STATE_OFF;
		AE_LOGI("FDAE: ->enable, frame_idx=%d", cxt->fdae.curr_frame_idx);
	}
}

static void _fdae_weight_mapping(struct ae_ctrl_context *cxt,
							   struct ae_weight_table *work_tbl,
							   int32_t *total_face_grid_num)
{
	int32_t frame_w = cxt->fdae.face_info.width;
	int32_t frame_h = cxt->fdae.face_info.height;
	int32_t win_num_w = cxt->monitor_unit.win_num.w;
	int32_t win_num_h = cxt->monitor_unit.win_num.h;
	int32_t i = 0;

	memset(work_tbl->weight, 0, sizeof(work_tbl->weight));
	*total_face_grid_num = 0;

	for (i = 0; i < cxt->fdae.face_info.face_num; i++) {
		struct ae_face *f = &(cxt->fdae.face_info.face_area[i]);
		int32_t start_x = f->rect.start_x * win_num_w / frame_w;
		int32_t end_x = f->rect.end_x * win_num_w / frame_w;
		int32_t start_y = f->rect.start_y * win_num_h / frame_h;
		int32_t end_y = f->rect.end_y * win_num_h / frame_h;
		AE_LOGI("FDAE: ->start_x, end_x, start_y, end_y = %d %d %d %d", start_x, end_x, start_y, end_y);

		/* Shrink a little the face region to make face-guided exposure more effective */
		{
			int32_t shrink_x = (end_x - start_x + 1) / 10;
			int32_t shrink_y = (end_y - start_y + 1) / 10;

			shrink_x = MAX(0, shrink_x);
			shrink_y = MAX(0, shrink_y);
			end_x = MIN(MAX(end_x - shrink_x, 0), win_num_w-1);
			start_x = MIN(MAX(start_x + shrink_x, 0), win_num_w-1);
			end_y = MIN(MAX(end_y - shrink_y, 0), win_num_h-1);
			start_y = MIN(MAX(start_y + shrink_y, 0), win_num_h-1);
			AE_LOGI("FDAE: ->start_x, end_x, start_y, end_y = %d %d %d %d", start_x, end_x, start_y, end_y);
		}

		/* set the face hit information to the work_table */
		{
			int32_t y = 0, x = 0;
			for (y = start_y; y <= end_y; y++) {
				for (x = start_x; x <= end_x; x++) {
					int32_t idx = x + win_num_w * y;
					work_tbl->weight[idx] = 1;
				}
			}
		}

		*total_face_grid_num += (end_x - start_x + 1) * (end_y - start_y + 1);
	}
}

static uint32_t _is_same_to_prev_face(const struct ae_face *face_prev,
									  const struct ae_face *face_curr)
{
	static const uint32_t PERCENT_THR = 60;
	uint32_t percent = 0;

	/* get the overlapped region */
	int32_t sx = MAX(face_prev->rect.start_x, face_curr->rect.start_x);
	int32_t ex = MIN(face_prev->rect.end_x, face_curr->rect.end_x);
	int32_t sy = MAX(face_prev->rect.start_y, face_curr->rect.start_y);
	int32_t ey = MIN(face_prev->rect.end_y, face_curr->rect.end_y);

	if (ex >= sx && ey >= sy) {
		int32_t area_overlap = (ex - sx + 1) * (ey - sy + 1);
		int32_t area_prev = (face_prev->rect.end_x - face_prev->rect.start_x + 1) *
							(face_prev->rect.end_y - face_prev->rect.start_y + 1);
		percent = (100 * area_overlap) / area_prev;
	}

	return ((percent >= PERCENT_THR)? 1 : 0);

}

static uint32_t _is_face_info_changed(const struct ae_fd_param *face_info_prev,
									  const struct ae_fd_param *face_info_curr)
{
	int32_t i = 0, j = 0;
	uint32_t changed = 0;

	if (face_info_curr->face_num != face_info_prev->face_num) {
		return 1;
	}

	for (i = 0; i < face_info_curr->face_num; i++) {
		uint32_t found_same = 0;
		const struct ae_face *face_curr = &(face_info_curr->face_area[i]);
		for (j = 0; j < face_info_prev->face_num; j++) {
			const struct ae_face *face_prev = &(face_info_prev->face_area[j]);
			if (_is_same_to_prev_face(face_prev, face_curr)) {
				found_same = 1;
				break;
			}
		}

		if (!found_same) {
			changed = 1;
			break;
		}
	}

	return changed;
}

static void _fdae_update_ae_weight_table(struct ae_ctrl_context *cxt)
{
	int16_t w_index = MIN(cxt->cur_status.weight, 2); // Only support 3 weight table
	struct ae_weight_table *base_tbl = &(cxt->cur_param->weight_table[w_index]);
	struct ae_weight_table *work_tbl = &(cxt->fdae.w_work_tbl);
	int32_t win_num_w = cxt->monitor_unit.win_num.w;
	int32_t win_num_h = cxt->monitor_unit.win_num.h;
	int32_t face_num = cxt->fdae.face_info.face_num;
	int32_t total_face_grid_num = 0;
	int32_t w_face_grid = 0;
	int32_t i = 0, j = 0;

	uint32_t is_face_changed = _is_face_info_changed(&(cxt->fdae.face_info_prev),&(cxt->fdae.face_info));
	if (!is_face_changed) {
		/* don't update weight table, because faces didn't change */
		return;
	} else {
		/* copy faces */
		if (cxt->fdae.face_info.face_num > 0) {
			memcpy(&(cxt->fdae.face_info_prev), &(cxt->fdae.face_info), sizeof(struct ae_fd_param));
		} else {
			cxt->fdae.face_info_prev.face_num = 0;
		}
	}

	/* mapping face coordinates to weight table */
	if (face_num > 0) {
		_fdae_weight_mapping(cxt, work_tbl, &total_face_grid_num);
	}

	if (total_face_grid_num > 0) {
		int32_t total_face_weight = (cxt->fdae.total_w[w_index] * cxt->fdae.param_face_weight) / 100;
		w_face_grid =(int32_t)(total_face_weight / total_face_grid_num);
		w_face_grid = MIN(w_face_grid, 230); // avoid overflow

		AE_LOGI("FDAE: w_face_grid=%d, total_face_grid_num=%d", w_face_grid, total_face_grid_num);
		for (j = 0; j < win_num_h; j++) {
			for (i = 0; i < win_num_w; i++) {
				int32_t idx = i + win_num_w * j;
				int32_t is_hit = work_tbl->weight[idx];

				uint8_t smooth_w = is_hit? w_face_grid : 0;
				work_tbl->weight[idx] = smooth_w + base_tbl->weight[idx];
			}
		}

	} else {
		memcpy(work_tbl->weight, base_tbl->weight, sizeof(work_tbl->weight));
	}

	/* Change AE weight table */
	ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_WEIGHT_TABLE,
					(void *)work_tbl, NULL);
	AE_LOGI("FDAE: ->updated weight table, frame_idx=%d", cxt->fdae.curr_frame_idx);
}

static void _fdae_update_state(struct ae_ctrl_context *cxt)
{
	cxt->fdae.curr_frame_idx++;

	// Check whether FD is working or not. If not, FDAE should be disabled.
	{
		static const int32_t FD_NOT_WORK_FRAMES_THR = 30;
		int32_t frames_not_work = cxt->fdae.curr_frame_idx - cxt->fdae.receive_face_frame_idx;
		if (frames_not_work >= FD_NOT_WORK_FRAMES_THR) {
			if (cxt->fdae.enable) {
				_fdae_disable_fdae(cxt);
			}
			// If FD is not working, just return.
			return;
		}
	}

	// Check whether "flash" is working (is_quick_mode==1). If yes, FDAE should be disabled.
	if (cxt->cur_status.is_quick_mode || cxt->flash_working)
	{
		if (cxt->fdae.enable) {
			_fdae_disable_fdae(cxt);
		}
		return;
	}

	// When "allow_to_work"==0, FDAE will never be enabled.
	if (cxt->fdae.allow_to_work) {
		// FDAE can be enabled only in "normal scene"
		if (cxt->fdae.enable) {
			if ((AE_SCENE_NORMAL != cxt->cur_status.scene)) {
				_fdae_disable_fdae(cxt);
			}
		} else {
			if ((AE_SCENE_NORMAL == cxt->cur_status.scene)) {
				_fdae_enable_fdae(cxt);
			}
		}
	} else {
		if (cxt->fdae.enable) {
			_fdae_disable_fdae(cxt);
		}
	}

    if (!cxt->fdae.enable) {
        return;
    }

	switch(cxt->fdae.state) {
		case FDAE_STATE_OFF:
		{
			/* FDAE can be set ON, only if at least 10 frames passed since last disabling */
			if ((cxt->fdae.face_info.face_num > 0) &&
			    (cxt->fdae.curr_frame_idx - cxt->fdae.last_disable_frame_idx > 30)) {
				cxt->fdae.state = FDAE_STATE_ON;
				_fdae_enter_fdae_mode(cxt);

				AE_LOGI("FDAE: ->FDAE_STATE_ON, frame_idx=%d", cxt->fdae.curr_frame_idx);
			}
			break;
		}

		case FDAE_STATE_QUITTING_LOCK_WEIGHT:
		{
			cxt->fdae.frames_to_lock--;
			if (cxt->fdae.frames_to_lock <= 0) {
				cxt->fdae.state = FDAE_STATE_OFF;
				_fdae_leave_fdae_mode(cxt);

				AE_LOGI("FDAE: ->FDAE_STATE_OFF, frame_idx=%d", cxt->fdae.curr_frame_idx);
			}
			break;
		}

		case FDAE_STATE_ON:
		{
			if (cxt->fdae.face_info.face_num > 0) {
				_fdae_update_ae_weight_table(cxt);
				cxt->fdae.state = FDAE_STATE_ON_LOCK_WEIGHT;
				cxt->fdae.frames_to_lock = cxt->fdae.param_lock_weight_has_face;
				AE_LOGI("FDAE: ->FDAE_STATE_ON_LOCK_WEIGHT, frame_idx=%d", cxt->fdae.curr_frame_idx);
			} else {
				if (cxt->fdae.frames_no_face > FRAMES_NO_FACE_FOR_SURE) {
					_fdae_update_ae_weight_table(cxt);
					cxt->fdae.state = FDAE_STATE_QUITTING_LOCK_WEIGHT;
					cxt->fdae.frames_to_lock = cxt->fdae.param_lock_weight_no_face;
					AE_LOGI("FDAE: ->FDAE_STATE_QUITTING_LOCK_WEIGHT, frame_idx=%d", cxt->fdae.curr_frame_idx);
				}
			}
			break;
		}

		case FDAE_STATE_ON_LOCK_AE:
		{
			cxt->fdae.frames_to_lock--;
			if (cxt->fdae.frames_to_lock <= 0) {
				struct ae_calc_out result;
				cxt->fdae.state = FDAE_STATE_ON;
				_set_restore(cxt, &result);
				AE_LOGI("FDAE: ->FDAE_STATE_ON. Unlock AE, frame_idx=%d", cxt->fdae.curr_frame_idx);
			}
			break;
		}

		case FDAE_STATE_ON_LOCK_WEIGHT:
		{
			cxt->fdae.frames_to_lock--;
			if (cxt->fdae.frames_to_lock <= 0) {
				cxt->fdae.state = FDAE_STATE_ON;
				AE_LOGI("FDAE: ->FDAE_STATE_ON, frame_idx=%d", cxt->fdae.curr_frame_idx);
			}
			break;
		}

		default:
			break;
	}
}

/* END: FDAE related functions                                            */
/**************************************************************************/

static int32_t _get_skip_frame_num(struct ae_ctrl_context *cxt, void *param, void *result)
{
	int32_t rtn = AE_SUCCESS;
	int32_t frame_num = 0;


	frame_num = cxt->monitor_skip_num;
	if (result) {
		int32_t *skip_num = (int32_t*)result;

		*skip_num = frame_num;
	}
	AE_LOGI("skip frame num=%d", frame_num);

	return rtn ;
}

static int32_t _get_normal_info (struct ae_ctrl_context *cxt, void *result) {
	int32_t rtn = AE_SUCCESS;
	uint32_t exposure = 0;
	uint32_t line_time = 0;
	uint32_t frame_line = 0;
	struct ae_normal_info *info_ptr = (struct ae_normal_info*)result;

	line_time = cxt->resolution_info.line_time;
	frame_line = cxt->cur_result.cur_exp_line + cxt->cur_result.cur_dummy;

	exposure = frame_line * line_time;
	info_ptr->exposure = exposure /10;
	info_ptr->fps = 100000 / exposure;

	return rtn;
}

static int32_t _set_sensor_exp_time (struct ae_ctrl_context *cxt, void *param)
{
	int32_t rtn = AE_SUCCESS;
	uint32_t exp_time = *(uint32_t*)param;
	uint32_t line_time = cxt->resolution_info.line_time;

	if (AE_STATE_LOCKED == cxt->ae_state) {
		cxt->cur_result.cur_exposure = exp_time*10;
		cxt->cur_result.cur_exp_line = exp_time*10/line_time;
		cxt->cur_result.cur_dummy = 0;
		_write_to_sensor(cxt, &cxt->cur_result, 1);

	}

	return rtn;
}

static int32_t _set_sensor_sensitivity (struct ae_ctrl_context *cxt, void *param)
{
	int32_t rtn = AE_SUCCESS;
	uint32_t sensitivity  = *(uint32_t*)param;
	uint32_t ae_gain;

	if (AE_STATE_LOCKED == cxt->ae_state) {
		ae_gain = sensitivity*128/50;
		cxt->cur_result.cur_again = ae_gain;
		_write_to_sensor(cxt, &cxt->cur_result, 1);

	}

	return rtn;
}

int32_t ae_sprd_io_ctrl(void* handle, enum ae_io_ctrl_cmd cmd, void *param, void *result)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_ctrl_context *cxt = NULL;
	struct ae_status *next_status = NULL;


	rtn = _check_handle(handle);
	if (AE_SUCCESS != rtn) {
		AE_RETURN_IF_FAIL(rtn, ("handle = %p", handle));
	}

	cxt = (struct ae_ctrl_context *)handle;
	pthread_mutex_lock(&cxt->status_lock);

	next_status = &cxt->nxt_status;
	switch (cmd) {
	case AE_SET_PROC:
		_sof_handler(cxt);
		break;
	case AE_SET_WORK_MODE:
		if (param) {
			struct ae_set_work_param *work_info = param;

			if (work_info->mode >= AE_WORK_MODE_MAX) {
				AE_LOGE("work mode param is error");
				break;
			}
			next_status->work_info = *work_info;

			rtn = ae_work_init(cxt, work_info);
		}
		_cfg_monitor(cxt);
		break;

	case AE_SET_SCENE_MODE:
		if (param) {
			struct ae_set_scene *scene_mode = param;

			if (scene_mode->mode < AE_SCENE_MAX) {
				next_status->scene = scene_mode->mode;
				cxt->update_list.is_scene = 1;
			}
		}
		break;

	case AE_SET_ISO:
		if (param) {
			struct ae_set_iso *iso = param;
#ifdef ISO_FIX_IN_VIDEO
			if (cxt->work_in_video) {
				iso->mode = VIDEO_IS0;
			}
#endif
			if (iso->mode < AE_ISO_MAX) {
				next_status->iso = iso->mode;
				cxt->update_list.is_iso = 1;
			}
		}
		break;

	case AE_GET_ISO:
		if (result) {
			rtn = _get_iso(cxt, result);
		}
		break;

	case AE_SET_FLICKER:
		if (param) {
			struct ae_set_flicker *flicker = param;

			if (flicker->mode < AE_FLICKER_OFF) {
				next_status->flicker = flicker->mode;
			}
		}
		break;

	case AE_SET_WEIGHT:
		if (param) {
			struct ae_set_weight *weight = param;
			AE_LOGV("ae_weight %d",weight->mode);
			if (weight->mode < AE_WEIGHT_MAX) {
				next_status->weight = weight->mode;
				cxt->update_list.is_weight = 1;
			}
		}
		break;

	case AE_SET_TOUCH_ZONE:
		if (param) {
			struct ae_set_tuoch_zone *touch_zone = param;
			next_status->touch_zone = touch_zone->touch_zone;
			cxt->update_list.is_touch_zone = 1;
		}
		break;

	case AE_SET_EV_OFFSET:
		if (param) {
			struct ae_set_ev *ev = param;
			if (ev->level < AE_LEVEL_MAX) {
				next_status->ev_level = ev->level;
				cxt->update_list.is_ev = 1;
			}
		}
		break;

	case AE_SET_FPS:
		if (param) {
			struct ae_set_fps *fps = param;
			next_status->fps = *fps;
			cxt->update_list.is_fps = 1;
		}
		break;

	case AE_SET_PAUSE:
		rtn = _set_pause(cxt);
		break;
	case AE_SET_RESTORE:
		rtn = _set_restore(cxt, result);
		break;
	case AE_SET_FORCE_PAUSE:
		rtn = _set_force_lock(cxt, 1);
		break;
	case AE_SET_FORCE_RESTORE:
		rtn = _set_force_lock(cxt, 0);
		break;

	case AE_SET_FLASH_NOTICE:
		if (cxt->has_force_bypass)
		break;
		rtn = _set_flash_notice(cxt, param);
		break;

	case AE_GET_FLASH_EFFECT:
		if (result) {
			uint32_t *effect = (uint32_t*)result;

			*effect = cxt->flash_param.effect;
		}
		break;
	case AE_GET_AE_STATE:
		if (result) {
			uint32_t *ae_state = (uint32_t*)result;

			*ae_state = cxt->ae_state;
		}
		break;
	case AE_GET_FLASH_EB:
		if (result) {
			uint32_t *flash_eb = (uint32_t*)result;
			uint32_t cur_lum = cxt->cur_result.cur_lum;
			uint32_t low_lum_thr = cxt->cur_result.lum_low_thr;

			int32_t bv = 0;
			struct ae_misc_calc_out calc_out = {0};
			int32_t bv_thr = cxt->flash_on_off_thr;
			if (0 == bv_thr) {
				bv = AE_FLASH_ON_OFF_THR;
			}
			rtn = _get_bv_by_lum(cxt, &cxt->cur_result, &bv);

			if (bv < bv_thr)
				*flash_eb = 1;
			else
				*flash_eb = 0;

			AE_LOGI("AE_GET_FLASH_EB: flash_eb=%d, bv=%d, thr=%d", *flash_eb, bv, bv_thr);
		}
		break;

	case AE_SET_FLASH_ON_OFF_THR:
		if (param) {
			cxt->flash_on_off_thr = *(int32_t *)param;
			if (0 == cxt->flash_on_off_thr) {
				cxt->flash_on_off_thr = AE_FLASH_ON_OFF_THR;
			}

			AE_LOGI("AE_SET_FLASH_ON_OFF_THR: %d", cxt->flash_on_off_thr);
		}
		break;
	case AE_SET_TUNING_EB:
		//rtn = _set_correction_param(cxt);
		rtn = _set_frame_cnt(cxt);
		break;
	case AE_GET_BV_BY_GAIN:
		if (result) {
			int32_t *bv = (int32_t*)result;

			rtn = _get_bv_by_gain(cxt, &cxt->cur_result, bv);
		}
		break;
	case AE_GET_EV:
		if (result) {
			uint32_t i = 0x00;
			struct ae_ev_table *ev_table = &cxt->cur_param->ev_table;
			int32_t target_lum = cxt->cur_param->target_lum;
			struct ae_get_ev *ev_info = (struct ae_get_ev*)result;

			ev_info->ev_index = cxt->cur_status.ev_level;

			for (i = 0; i < ev_table->diff_num; i++) {
				ev_info->ev_tab[i] = target_lum + ev_table->lum_diff[i];
				//AE_LOGE("EV_TEST: %d ,i=%d, ev=%d, target=%d, diff=%d",ev_info->ev_index, i, ev_info->ev_tab[i], target_lum, ev_table->lum_diff[i]);
			}

			rtn = AE_SUCCESS;
		}
		break;
	case AE_GET_BV_BY_LUM:
		if (result) {
			int32_t *bv = (int32_t*)result;

			rtn = _get_bv_by_lum(cxt, &cxt->cur_result, bv);
		}
		break;
	case AE_SET_STAT_TRIM:
		if (param) {
			struct ae_trim *trim = (struct ae_trim*)param;

			if (cxt->stat_req.mode) {
				AE_LOGE("stat mode is not normal");
				break;
			}
			rtn = _set_scaler_trim(cxt, trim);
		}
		break;
	case AE_SET_G_STAT:
		if (param) {
			struct ae_stat_mode  *stat_mode = (struct ae_stat_mode *)param;

			rtn = _set_g_stat(cxt, stat_mode);
		}
		break;
	case AE_GET_LUM:
		if (result) {
			uint32_t *lum = (uint32_t*)result;

			*lum = cxt->cur_result.cur_lum;
		}
		break;
	case AE_SET_TARGET_LUM:
		if (param) {
			uint32_t *lum = (uint32_t*)param;

			next_status->target_lum = *lum;
			cxt->update_list.is_target_lum = 1;
		}
		break;
	case AE_SET_SNAPSHOT_NOTICE:
		if (param) {
			rtn = _set_snapshot_notice(cxt, param);
		}
		break;
	case AE_GET_MONITOR_INFO:
		if (result) {
			struct ae_monitor_info *info = result;

			info->win_size = cxt->monitor_unit.win_size;
			info->win_num = cxt->monitor_unit.win_num;
			info->trim = cxt->monitor_unit.trim;
		}
		break;
	case AE_GET_FLICKER_MODE:
		if(result) {
			uint32_t *mode = result;
			*mode = cxt->cur_status.flicker;
		}
		break;
	case AE_SET_ONLINE_CTRL:
		if (param) {
			rtn = _tool_online_ctrl(cxt, param, result);
		}
		break;
	case AE_SET_EXP_GAIN:
		cxt->ae_state = AE_STATE_LOCKED;
		_set_premain_exg(cxt);
		break;
	case AE_SET_EXP_ANIT:
		if (param && cxt->fdae.state == FDAE_STATE_OFF) {
			rtn = _set_exp_anti(cxt, param);
		}
		break;
	case AE_SET_FD_ENABLE:
		if (param) {
			cxt->fdae.allow_to_work = *((uint32_t *)param);
		}
		break;
	case AE_SET_FD_PARAM:
		if (param) {
			rtn = _fd_process(cxt, param);
		}
		break;
	case AE_GET_GAIN:
		if (result) {
			float real_gain = 0;
			real_gain = _get_real_gain(cxt->cur_result.cur_again) / 16.0;
			*(float *)result = real_gain;
		}
		break;
	case AE_GET_EXP:
		if (result) {
			float exposure = 0;
			exposure = (float)cxt->cur_result.cur_exposure / 10000000.0;
			AE_LOGV("exp = %d, %f", cxt->cur_result.cur_exposure, exposure);
			*(float *)result = exposure;
		}
		break;
	case AE_GET_FLICKER_SWITCH_FLAG:
		if (param) {
			rtn = _get_flicker_switch_flag(cxt, param);
		}
		break;

	case AE_GET_CUR_WEIGHT:
		if (param && cxt->fdae.state == FDAE_STATE_OFF) {
			rtn = _get_ae_weight(cxt, param, result);
		}
		break;
	case AE_GET_SKIP_FRAME_NUM:
		rtn = _get_skip_frame_num(cxt, param, result);
		break;
	case AE_SET_NIGHT_MODE:
		if (param) {
			uint32_t night_mode = *(uint32_t *)param;

			next_status->night_mode = night_mode;
			cxt->update_list.is_night = 1;
		}
		break;
	case AE_SET_BYPASS:
		if (param) {
			cxt->has_force_bypass = *(int32_t*)param;
		}
		break;
	case AE_GET_NORMAL_INFO:
		if (result) {
			rtn = _get_normal_info(cxt, result);
		}
		break;
	case AE_SET_FORCE_QUICK_MODE:
		if (param) {
			cxt->force_quick_mode = *(int32_t*)param;
		}
		break;
	case AE_SET_EXP_TIME:
		if (param) {
			_set_sensor_exp_time(cxt,param);
		}
		break;
	case AE_SET_SENSITIVITY:
		if (param) {
			_set_sensor_sensitivity(cxt,param);
		}
		break;
	default:
		rtn = AE_ERROR;
		break;
	}

	pthread_mutex_unlock(&cxt->status_lock);

	return rtn;
}

extern struct ae_lib_fun ae_lib_fun;
void sprd_ae_fun_init()
{
	ae_lib_fun.ae_init 			= ae_sprd_init;
	ae_lib_fun.ae_deinit			= ae_sprd_deinit;
	ae_lib_fun.ae_calculation		= ae_sprd_calculation;
	ae_lib_fun.ae_io_ctrl			= ae_sprd_io_ctrl;

	return;
}
/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef	__cplusplus
}
#endif

