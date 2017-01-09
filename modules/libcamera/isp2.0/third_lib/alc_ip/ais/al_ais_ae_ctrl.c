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
#define LOG_TAG "ae_alc_ctrl"
#include "ae_alc_ctrl.h"
#include "ae_misc.h"
#include "ae_log.h"
#include "ae_types.h"
#include <math.h>
#ifndef WIN32
#include <utils/Timers.h>
#include <cutils/properties.h>
#include <math.h>
#endif

#include "AlAisInterface.h"

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
#define     UNUSED(param)  (void)(param)

#define AE_SAVE_PARAM_STR    "persist.sys.isp.ae.param" /*save/no*/
#define AE_SAVE_AE_TABLE_STR "persist.sys.isp.ae.tbl" /*save/no*/
#define AE_SAVE_MLOG_STR     "persist.sys.isp.ae.mlog" /*save/no*/
#define AE_CAP_USE_PREVIEW_TBL  "persist.sys.isp.ae.use_prv_tbl" /*yes/no*/
#define USE_ISO_TBL 0
#define SEGMENTED_ISO 0
/*should be read from drive later*/
#define AE_FLASH_RATIO (6 * 256)
/*for 30 LUX*/
#define AE_FLASH_ON_OFF_THR 38
#define AE_FLASH_TARGET_LUM 55
#define AE_FLASH_MAX_RATIO 20
#define ISO_FIX_IN_VIDEO
#define VIDEO_IS0 5

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
	uint32_t backup_taget_lum;
	int32_t flash_on_off_thr;
};

struct ae_monitor_unit {
	struct ae_size win_num;
	struct ae_size win_size;
	struct ae_trim trim;
	struct ae_monitor_cfg cfg;
	uint32_t is_stop_monitor;
};

struct ae_update_list{
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

		//ALC_S
enum al_ais_ae_flash_seq {
	AIS_FLASH_SEQ_NONE	=	0,
	AIS_FLASH_SEQ_TOUCH_ON,
	AIS_FLASH_SEQ_AF_SEQ,
	AIS_FLASH_SEQ_TRIGGER,
	AIS_FLASH_SEQ_TRIGGER_P3,		//LED Seq. P3 recv
	AIS_FLASH_SEQ_TRIGGER_M0,		//LED Seq. M0 recv
	AIS_FLASH_SEQ_FLASH_ON,
	AIS_FLASH_SEQ_MAX
};
		//ALC_E

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
	//struct ae_misc_calc_out prv_result;

	uint32_t is_force_lock;
	uint32_t force_ae_state;
	uint32_t pause_ref_cnt;
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

	uint32_t end_id;
	uint32_t prv_index;
	uint32_t ctrl_quick_ae_cb_ext;
	uint32_t skip_calc_num;
	uint32_t convergerd_num;
	uint32_t cap_skip_num;
	uint32_t camera_id;
	uint32_t flash_working;
	uint32_t work_in_video;


		//ALC_S
	TT_AlAisInterface			mttAis;

	enum al_ais_ae_flash_seq	muiFlashSeq;
	uint32_t					muiFlashSeqFrameCount;

	uint32_t					use_flash_save;
	struct ae_misc_calc_out		cur_result_flash_save;
	uint32_t					cur_result_flash_save_LedBright;
		//ALC_E
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

static int save_param_to_file(int32_t sn, uint32_t size, uint8_t *addr)
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
	sprintf(file_name, "/data/misc/media/ae_param_%d.txt", sn);

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
		sprintf(file_name, "/data/misc/media/ae_tbl%d_scene%d_flk%d.txt", sn, scene_mode, flicker);
	} else {
		sprintf(file_name, "/data/misc/media/ae_tbl%d_auto_flk%d_iso%d.txt", sn, flicker, iso);
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

	sprintf(file_name, "/data/misc/media/ae_work%d_weight%d.txt", sn, weight_index);

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

	sprintf(file_name, "/data/misc/media/ae_work%d_autoiso%d.txt", sn, flicker);

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
		save_ae_tbl_to_file(sn, 0, i, i+1, &tuning_param->ae_table[1][i]);
		//60hz
		save_ae_tbl_to_file(sn, 1, i, i+1,&tuning_param->ae_table[1][i]);
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

static int32_t save_to_mlog_file(int32_t sn, struct ae_misc_calc_out *result)
{
	int32_t rtn = 0;
	UNUSED(sn);
#ifndef WIN32
	char file_name[] = "/data/ae.txt";
	int32_t tmp_size = 0;
	char tmp_str[200];
	FILE *s_mlog_fp = NULL;


	s_mlog_fp = fopen(file_name, "wb");
	if (s_mlog_fp) {
		memset(tmp_str, 0, sizeof(tmp_str));
		sprintf(tmp_str,"index:%d, c_lum:%d, t_lum:%d, exp(us*10):%d, again:%d, max_index:%d",
				result->cur_index, result->cur_lum, result->target_lum, result->cur_exposure,
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
	UNUSED(is_force);
	int32_t rtn = AE_SUCCESS;
	uint32_t is_update_exp = 0;
	uint32_t is_update_gain = 0;
	uint32_t write_exp_eb = 1;
	uint32_t write_gain_eb = 1;
	uint32_t write_exp_sync = 0;
	uint32_t write_gain_sync = 0;

	if (NULL == cxt
		|| NULL == misc_calc_result) {
			rtn = AE_PARAM_NULL;
			AE_RETURN_IF_FAIL(rtn, ("cxt:%p, out:%p", cxt, misc_calc_result));
	}

	AE_LOGV("exp_line=%d, again=%d, dgain=%d, dummy=%d cur_index=%d",
		misc_calc_result->cur_exp_line, misc_calc_result->cur_again,
		misc_calc_result->cur_dgain, misc_calc_result->cur_dummy, misc_calc_result->cur_index);

	//AE_LOGE("AE_TEST: -----exp------:%d, %d", misc_calc_result->cur_exp_line, misc_calc_result->cur_again);

	if ((0 != misc_calc_result->cur_again)
			&&(0 != misc_calc_result->cur_exp_line)) {
		is_update_exp = 1;
		if (cxt->isp_ops.set_exposure && is_update_exp) {
			struct ae_exposure exp;
			uint32_t ae_exposure = misc_calc_result->cur_exp_line;
			uint32_t dummy = misc_calc_result->cur_dummy;
			uint32_t size_index = cxt->resolution_info.sensor_size_index;

			if (1 == write_exp_eb) {
				memset(&exp, 0, sizeof(exp));

				ae_exposure = ae_exposure & 0x0000ffff;
				ae_exposure |= (dummy << 0x10) & 0x0fff0000;
				ae_exposure |= (size_index << 0x1c) & 0xf0000000;

				exp.exposure = ae_exposure;
				(*cxt->isp_ops.set_exposure)(cxt->isp_ops.isp_handler, &exp);
			}
		}

		is_update_gain = 1;

		if (cxt->isp_ops.set_again && is_update_gain) {
			struct ae_gain again;
			if (1 == write_gain_eb) {
				memset(&again, 0, sizeof(again));
				again.gain = misc_calc_result->cur_again & 0xffff;

				(*cxt->isp_ops.set_again)(cxt->isp_ops.isp_handler, &again);
			}
		}
	}else {

	}

	//cxt->prv_result.cur_exp_line = misc_calc_result->cur_exp_line;
	//cxt->prv_result.cur_again = misc_calc_result->cur_again;

	return rtn;
}

static int32_t _write_exp_gain(struct ae_ctrl_context *cxt)
{
	int32_t rtn = AE_SUCCESS;
	uint32_t num = cxt->write_conter;

	//AE_LOGE("AE_TEST: -----exp------:num:%d, %d, %d", num, cxt->cur_result.exp.tab[num].exp_line, cxt->cur_result.exp.tab[num].again);

	if ((0 != cxt->cur_result.exp.tab[num].exp_line)
		&& (0 != cxt->cur_result.exp.tab[num].exp_line)){

		if (NULL != cxt->isp_ops.set_exposure) {
			struct ae_exposure exp;
			uint32_t ae_exposure = cxt->cur_result.exp.tab[num].exp_line;
			uint32_t dummy = cxt->cur_result.exp.tab[num].exp_dummy;
			uint32_t size_index = cxt->resolution_info.sensor_size_index;

			memset(&exp, 0, sizeof(exp));
			ae_exposure = ae_exposure & 0x0000ffff;
			ae_exposure |= (dummy << 0x10) & 0x0fff0000;
			ae_exposure |= (size_index << 0x1c) & 0xf0000000; 

			exp.exposure = ae_exposure;
			(*cxt->isp_ops.set_exposure)(cxt->isp_ops.isp_handler, &exp);
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
	UNUSED(size);
	uint32_t *tmp = param;
	uint32_t version = 0;
	uint32_t verify = 0;

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
	if (flicker_index >= AE_FLICKER_MAX) {
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
#if 0
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
#endif
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
		AE_LOGI("cvgn_param[0] error!!!  set default cvgn_param");
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
#if 0
		rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_WORK_PARAM, &misc_work_in, &misc_work_out);
		if (AE_SUCCESS == rtn) {
			cxt->cur_param = cur_param;
			cxt->prv_status = *cur_status;
			cur_status->work_info = *work_param;
			cxt->cur_status.param_from = next_param_from;
		} else {
			AE_LOGE("change work mode failed!");
		}
#endif

	} else {
		rtn = _change_video_work_mode(cxt, work_param);
	}

	if(NULL != cxt->cur_param){
		misc_cvgn_param.cvgn_param = cxt->cur_param->cvgn_param;
		misc_cvgn_param.target_lum = cxt->cur_param->target_lum;
		_check_cvgn_param(&misc_cvgn_param);
		//ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_CVGN_PARAM, &misc_cvgn_param, NULL);
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
	struct ae_set_fps fps_param;
	uint32_t i = 0;
	uint32_t target_lum = 0;

	if (scene_mode >= AE_SCENE_MAX)
		return AE_ERROR;

	cur_param = cxt->cur_param;
	scene_info = cur_param->scene_info;
	cur_status = &cxt->cur_status;
	
	for (i = 0; i < AE_SCENE_MAX; ++i) {

		if (0 == scene_info[i].enable)
			continue;

		if (scene_mode == scene_info[i].scene_mode)
			break;
	}

	if (i < AE_SCENE_MAX) {
		struct ae_exp_gain_table *ae_table = NULL;
		struct ae_weight_table *weight_table = NULL;
		uint32_t iso = scene_info[i].iso_index;
		uint32_t weight_mode = scene_info[i].weight_mode;

		if (iso >= AE_ISO_MAX || weight_mode >= AE_WEIGHT_MAX) {
			AE_LOGE("error iso=%d, weight_mode=%d", iso, weight_mode);
			rtn = AE_ERROR;
			goto EXIT;
		}
#if 0
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
#endif
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
			//rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_INDEX, (void *)&new_index, NULL);
		}

		cur_status->scene = scene_mode;
	} else {
		rtn = AE_ERROR;
		AE_LOGE("scene %d is error!", scene_mode);
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
#if 0
			rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_AE_TABLE,
							(void *)ae_table, NULL);
			if (AE_SUCCESS != rtn) {
				AE_LOGE("set ae table rtn=%d is error!", rtn);
				goto EXIT;
			}
#endif
			exp_line = cxt->cur_result.cur_exp_line;
			gain = cxt->cur_result.cur_again;
			cur_exp_gain = exp_line * _get_ae0_real_gain(gain);
			rtn = _get_new_index(cxt, cur_exp_gain, &new_index);
#if 0
			if (AE_SUCCESS != rtn) {
				goto EXIT;
			}
			rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_INDEX, (void *)&new_index, NULL);
#endif
		} else {
			rtn = AE_ERROR;
			AE_LOGE("scene AE_SCENE_NIGHT is error!");
		}

	} else {
		rtn = _set_iso(cxt,cur_status->iso);
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
#if 0
	rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_GET_NEW_INDEX,
	                      (void *)&cur_exp_gain, new_index);
#endif
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
#if 0
	ae_table = &cxt->cur_param->ae_table[cxt->cur_status.flicker][iso];
	rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_AE_TABLE, (void *)ae_table, NULL);
	AE_LOGI("set iso from %d to %d, rtn=%d", cxt->cur_status.iso, iso, rtn);
	if (AE_SUCCESS != rtn) {
		goto EXIT;
	}
#endif
	cxt->cur_status.iso = iso;

	exp_line = cxt->cur_result.cur_exp_line;
	gain = cxt->cur_result.cur_again;
	cur_exp_gain = exp_line * _get_ae0_real_gain(gain);
	rtn = _get_new_index(cxt, cur_exp_gain, &new_index);
	if (AE_SUCCESS != rtn) {
		goto EXIT;
	}
//	rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_INDEX, (void *)&new_index, NULL);

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

	uint32_t max_index;
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
#if 0
	rtn = ae_misc_io_ctrl(cxt->misc_handle,
                          AE_MISC_CMD_SET_TARGET_LUM,
                          (void *)&target_lum, NULL);

	if (AE_SUCCESS == rtn) {
		cxt->cur_status.target_lum = target_lum;
	}
#endif
	return rtn;
}

static int32_t _set_ev_level(struct ae_ctrl_context *cxt, enum ae_level level)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_ev_table *ev_table = &cxt->cur_param->ev_table;
	uint32_t target_lum = 0;
	uint32_t ev_level = level;

	target_lum = _calc_target_lum(cxt->cur_param->target_lum, level, ev_table);

	//rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_EV, (void *)&ev_level, NULL);

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

	return rtn;

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
			weight_val = 0;

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
#if 0
		weight_tab = &cxt->touch_zone_param.weight_table;
		rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_WEIGHT_TABLE,
						(void *)weight_tab, NULL);
		if (AE_SUCCESS == rtn) {
			cxt->cur_status.touch_zone = *touch_zone;
		}
#endif
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
#if 0
	weight_table = &cxt->cur_param->weight_table[mode];
	rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_WEIGHT_TABLE,
					(void *)weight_table, NULL);
	AE_LOGI("set weight from %d to %d, rtn=%d", cxt->cur_status.weight, mode, rtn);
	if (AE_SUCCESS == rtn) {		
		cxt->cur_status.weight = mode;
		cxt->touch_zone_param.enable = 0;
	}
#endif
	return rtn;
}

static int32_t _set_fps(struct ae_ctrl_context *cxt, struct ae_set_fps *fps)
{
	int32_t rtn = AE_ERROR;

	if (NULL == cxt || NULL == fps) {
		AE_LOGE("cxt %p fps %p param is error!", cxt, fps);
		return AE_ERROR;
	}
	fps->max_fps = 0;
#if 0
	rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_FPS, (void *)fps, NULL);
#endif
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
#if 0
	rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_AE_TABLE, (void *)ae_table, NULL);
	AE_LOGI("set flicker from %d to %d, rtn=%d",
			cxt->cur_status.flicker, mode, rtn);
	if (AE_SUCCESS == rtn) {
		cxt->cur_status.flicker = mode;
	}
#endif

	//ALC_S
	cxt->cur_status.flicker = mode;
			AE_LOGI("cxt->cur_status.flicker = %d", cxt->cur_status.flicker );
	//ALC_E

	exp_line = cxt->cur_result.cur_exp_line;
	gain = cxt->cur_result.cur_again;
	cur_exp_gain = exp_line * _get_ae0_real_gain(gain);
	rtn = _get_new_index(cxt, cur_exp_gain, &new_index);
	if (AE_SUCCESS != rtn) {
		goto EXIT;
	}
	//rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_INDEX, (void *)&new_index, NULL);

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

	//rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_FLASH_PARAM, (void *)flash_ctrl, NULL);


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

#if 0
	rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_QUICK_MODE, &quick_mode, NULL);
	if (AE_SUCCESS == rtn) {
		cxt->cur_status.is_quick_mode = is_quick;
	}
#endif
	AE_LOGI("is_quick=%d", is_quick);

	return rtn;
}

static int32_t _set_exp_anti(struct ae_ctrl_context *cxt, void* in_param)
{
	int32_t rtn = AE_SUCCESS;
	uint32_t *enable = (uint32_t*)in_param;

	//rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_EXP_ANIT, in_param, NULL);

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

	//rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_GET_WEIGHT, in_param, out_param);

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
				&& (next_status->touch_zone.y < high)
				&& (next_status->touch_zone.x >= 0)
				&& (next_status->touch_zone.y >= 0)) {
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

	if (update_list->is_scene
		&& (cur_status->scene != next_status->scene)) {
		rtn = _set_scene_mode(cxt, next_status->scene);
	}
	update_list->is_scene = 0;

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
		main_flash_lum = prv_lum + (((cur_lum - prv_lum) * ratio )>>8);
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
	//rtn = ae_misc_calculation(cxt->misc_handle, calc_in, calc_out);
	AE_RETURN_IF_FAIL(rtn, ("calc rtn %d is error!", rtn));

	cxt->flash_param.capture_result = *calc_out;
	AE_LOGE("flash premain cur_index %d", calc_out->cur_index);
	rtn = _calc_flash_effect(cxt, &flash->before_result, &flash->preflash_converged_result, &flash->effect);

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
	AE_LOGI("[_cfg_monitor_bypass] is_bypass=%d", cxt->monitor_unit.is_stop_monitor);

	if (cxt->isp_ops.set_monitor_bypass) {
		uint32_t is_bypass = 0;
	
		is_bypass = cxt->monitor_unit.is_stop_monitor;
		//AE_LOGI("AE_TEST: is_bypass=%d", is_bypass);
		rtn = cxt->isp_ops.set_monitor_bypass(cxt->isp_ops.isp_handler, is_bypass);
	}

	return rtn;
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
	uint32_t is_restore = 0;
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
	//rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_INDEX, (void *)&cur_index, NULL);
	rtn = _write_to_sensor(cxt, &calc_result, 1);
	if (is_restore) {
		cxt->cur_result = save_result;
	}

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
#if 0
		rtn = ae_misc_io_ctrl(cxt->misc_handle,
								AE_MISC_CMD_GET_EXP_BY_INDEX,
								(void *)&cur_index,
#endif								&calc_result);
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
#if 0
		rtn = ae_misc_io_ctrl(cxt->misc_handle,
							AE_MISC_CMD_GET_EXP_BY_INDEX,
							(void *)&index,
							&cxt->flash_param.flash_tuning_result);
#endif
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
#if 0	// 1=original
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
			uint32_t ratio = AE_FLASH_RATIO;//flash_notice->flash_ratio;
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
		rtn = _process_flash(cxt, 0);
		//cxt->cur_result = cxt->flash_param.before_result;
/*		if (flash_notice->will_capture) {
			cxt->ae_state = AE_STATE_LOCKED;
		} else {
			cxt->ae_state = AE_STATE_SEARCHING;
		}*/
/*
		if (AE_FLASH_STATE_ACTIVE == cxt->flash_param.flash_state) {
			cxt->cur_result = cxt->flash_param.before_result;
			rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_INDEX,
								(void *)&cxt->cur_result.cur_index, NULL);
			_write_to_sensor(cxt, &cxt->cur_result, 1);
			_cfg_monitor_bypass(cxt);
			cxt->flash_working = 0;
		}
*/
		cxt->ae_state = AE_STATE_SEARCHING;
		break;

	case AE_FLASH_MAIN_AFTER:
		ALOGE("AE_TEST: AE_FLASH_MAIN_AFTER");
		cxt->flash_param.enable = 0;
		rtn = _process_flash(cxt, 0);
		cxt->cur_result = cxt->flash_param.before_result;
		cxt->ae_state = AE_STATE_SEARCHING;
/*
		rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_INDEX,
							(void *)&cxt->cur_result.cur_index, NULL);
*/
		//_write_to_sensor(cxt, &cxt->cur_result, 1);
		_set_target_lum(cxt, cxt->flash_param.backup_taget_lum);
		cxt->flash_working = 0;
		break;

	case AE_FLASH_MAIN_BEFORE:
		ALOGE("AE_TEST: AE_FLASH_MAIN_BEFORE");
		if (cxt->fdae.enable) {
			_fdae_disable_fdae(cxt);
		}

		cxt->cur_result = cxt->flash_param.capture_result;
		cxt->ae_state = AE_STATE_PRECAPTURE;
		cxt->flash_working = 1;
		break;

	case AE_FLASH_MAIN_AE_MEASURE:
		ALOGE("AE_TEST: AE_FLASH_MAIN_AE_MEASURE");
		cxt->flash_param.flash_state = AE_FLASH_STATE_MAX;
		cxt->flash_param.backup_taget_lum = cxt->cur_status.target_lum;
		_calc_flash_target_lum(cxt);
		_set_target_lum(cxt, cxt->flash_param.target_lum);
		_set_quick_mode_status(cxt, 1);

		cxt->ctrl_quick_ae_cb_ext = 1;
		cxt->ae_state = AE_STATE_SEARCHING;
		cxt->skip_calc_num = 1;
		break;
	default:
		rtn = AE_ERROR;
		break;
	}

	return rtn;

#else

	int32_t rtn = AE_SUCCESS;
	enum ae_flash_mode mode = 0;

	if ((NULL == cxt) || (NULL == flash_notice)) {
		AE_LOGE("cxt %p flash_notice %p param is error!", cxt, flash_notice);
		return AE_PARAM_NULL;
	}

/*
enum al_ais_ae_flash_seq {
	AIS_FLASH_SEQ_NONE	=	0,
	AIS_FLASH_SEQ_TOUCH_ON,
	AIS_FLASH_SEQ_AF_SEQ,
	AIS_FLASH_SEQ_TRIGGER,
	AIS_FLASH_SEQ_TRIGGER_P3,		//LED Seq. P3 recv
	AIS_FLASH_SEQ_FLASH_ON,
	AIS_FLASH_SEQ_MAX
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
};
	UI_16			muiFcsReq;		//LED Focusing Seq Trigger(0:Off,1:Request)
	UI_16			muiLedReq;		//LED TTL Request  Trigger(0:Off,1:Request)
	UI_16			muiPebReq;		//ZSL PEB Request  Trigger(0:Off,1:Request):use by HDR-ON function
	UI_16			muiLedSta;		//LED Status(0:Off,bit0=1:Tourch,bit1=1:Flash,bit15=1:Configuration in Progress(0x8000))
	UI_16			muiFlashReq;	//LED (NEW)Input to start or stop a "Flash" Sequence(0:Stop/1:Start)
*/

	mode = flash_notice->mode;
	switch (mode) {

	case AE_FLASH_PRE_BEFORE	:	//P0
						AE_LOGI("_set_flash_notice : [P0] AE_FLASH_PRE_BEFORE");
						AE_LOGI("_set_flash_notice : max_charge=%d, max_time=%d", flash_notice->power.max_charge, flash_notice->power.max_time);
		{
			uint16_t	pre_flash_val[]	=	{
				0x9,	100,
				0x13,	200,
				0x1d,	300
			};
			struct ae_flash_element ele;

			ele.index = 0x1d;	
			ele.val = 300;
			cxt->isp_ops.flash_set_charge(cxt->isp_ops.isp_handler,  AE_FLASH_TYPE_PREFLASH,&ele);
		}
		if (cxt->isp_ops.callback) {
						AE_LOGI("_set_flash_notice : [C0] AE_CB_CONVERGED");
			(*cxt->isp_ops.callback)(cxt->isp_ops.isp_handler, AE_CB_CONVERGED);
		}
		cxt->muiFlashSeq			=	AIS_FLASH_SEQ_NONE;
		cxt->muiFlashSeqFrameCount	=	0;
		cxt->mttAis.muiFcsReq		=	0;	//LED Focusing Seq Trigger(0:Off,1:Request)
		cxt->mttAis.muiLedReq		=	0;	//LED TTL Request  Trigger(0:Off,1:Request)
		cxt->mttAis.muiLedSta		=	0;	//LED Status(0:Off,bit0=1:Tourch,bit1=1:Flash,bit15=1:Configuration in Progress(0x8000))
		cxt->mttAis.muiFlashReq		=	0;	//LED (NEW)Input to start or stop a "Flash" Sequence(0:Stop/1:Start)

		cxt->use_flash_save			=	0;
		break;
	case AE_FLASH_PRE_LIGHTING	:	//P1
						AE_LOGI("_set_flash_notice : [P1] AE_FLASH_PRE_LIGHTING");
		cxt->muiFlashSeq			=	AIS_FLASH_SEQ_TOUCH_ON;
		cxt->muiFlashSeqFrameCount	=	0;
		cxt->mttAis.muiFcsReq		=	0;	//LED Focusing Seq Trigger(0:Off,1:Request)
		cxt->mttAis.muiLedReq		=	0;	//LED TTL Request  Trigger(0:Off,1:Request)
		cxt->mttAis.muiLedSta		=	0;	//LED Status(0:Off,bit0=1:Tourch,bit1=1:Flash,bit15=1:Configuration in Progress(0x8000))
		cxt->mttAis.muiFlashReq		=	0;	//LED (NEW)Input to start or stop a "Flash" Sequence(0:Stop/1:Start)

		cxt->use_flash_save			=	0;
		break;
	case AE_FLASH_AF_DONE		:	//P2
						AE_LOGI("_set_flash_notice : [P2] AE_FLASH_AF_DONE");
		_ais_cmd_set_ae_LockMode(&cxt->mttAis, ALAIS_EXPOS_LOCK_MODE_OFF);
		cxt->muiFlashSeq			=	AIS_FLASH_SEQ_TRIGGER;
		cxt->muiFlashSeqFrameCount	=	0;
		cxt->mttAis.muiFcsReq		=	0;	//LED Focusing Seq Trigger(0:Off,1:Request)
		cxt->mttAis.muiLedReq		=	1;	//LED TTL Request  Trigger(0:Off,1:Request)
		cxt->mttAis.muiLedSta		=	0;	//LED Status(0:Off,bit0=1:Tourch,bit1=1:Flash,bit15=1:Configuration in Progress(0x8000))
		cxt->mttAis.muiFlashReq		=	1;	//LED (NEW)Input to start or stop a "Flash" Sequence(0:Stop/1:Start)
		break;
	case AE_FLASH_PRE_AFTER		:	//P3
						AE_LOGI("_set_flash_notice : [P3] AE_FLASH_PRE_AFTER");
		cxt->muiFlashSeq			=	AIS_FLASH_SEQ_TRIGGER_P3;		//LED Seq. P3 recv
		break;
	case AE_FLASH_MAIN_BEFORE	:	//M0
						AE_LOGI("_set_flash_notice : [M0] AE_FLASH_MAIN_BEFORE");
		cxt->muiFlashSeq			=	AIS_FLASH_SEQ_TRIGGER_M0;		//LED Seq. M0 recv
		{
			uint16_t	main_flash_val[]	=	{
				0x0,	300,
				0x4,	400,
			//	0x8,	500,
				0xc,	600,
			//	0xf,	700,
				0x11,	800,
			//	0x13,	900,
				0x15,	1000
			};
			struct ae_flash_element ele;
#if 0
static	uint16_t	index=0;
#endif

			cxt->use_flash_save			=	1;
			cxt->mttAis.muiLedBright	=	cxt->cur_result_flash_save_LedBright;
#if 0
//cxt->mttAis.muiLedBright=0;
cxt->mttAis.muiLedBright=(index>4)?(4):(index);
index=(index>=4)?(0):(index+1);
#endif
			ele.index = main_flash_val[cxt->mttAis.muiLedBright*2+0];
			ele.val   = main_flash_val[cxt->mttAis.muiLedBright*2+1];
		//	ele.index = 0x15;
		//	ele.val = 1000;
		//	ele.index = 0;
		//	ele.val = 300;
			cxt->isp_ops.flash_set_charge(cxt->isp_ops.isp_handler,  AE_FLASH_TYPE_MAIN,&ele);
								AE_LOGI("_set_flash_notice : index=%d, val=%d", ele.index, ele.val);
		
			ele.index = 0xb;
			ele.val = 1200;
			cxt->isp_ops.flash_set_time(cxt->isp_ops.isp_handler, AE_FLASH_TYPE_MAIN,&ele);
		}
		break;

	case AE_FLASH_MAIN_LIGHTING	:	//M1
						AE_LOGI("_set_flash_notice : [M1] AE_FLASH_MAIN_LIGHTING");
		cxt->muiFlashSeq			=	AIS_FLASH_SEQ_FLASH_ON;
		cxt->muiFlashSeqFrameCount	=	0;
		break;
	case AE_FLASH_MAIN_AE_MEASURE	:
						AE_LOGI("_set_flash_notice : [  ] AE_FLASH_MAIN_AE_MEASURE");
		break;
	case AE_FLASH_MAIN_AFTER	:
						AE_LOGI("_set_flash_notice : [  ] AE_FLASH_MAIN_AFTER");
		cxt->use_flash_save			=	0;
		break;

	default:
		rtn = AE_ERROR;
		break;
	}

	return rtn;
#endif
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

	rtn = _set_work_mode(cxt, work_param, 1);
	if (AE_SUCCESS != rtn) {
		goto EXIT;
	}

	if (cxt->is_first || is_changed_work_mode) {
		cxt->is_first = 0;
		_set_quick_mode(cxt, 0);
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
	} else {
		cxt->start_index = cxt->prv_index;
		AE_LOGI("cxt->is_first = 0, start_index=%d", cxt->start_index);
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
	//		rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_INDEX, (void *)&calc_result.cur_index, NULL);
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
		cxt->touch_zone_param.zone_param.level_0_weight = 1;
		cxt->touch_zone_param.zone_param.level_1_weight = 1;
		cxt->touch_zone_param.zone_param.level_2_weight = 1;
		cxt->touch_zone_param.zone_param.level_1_percent = 0;
		cxt->touch_zone_param.zone_param.level_2_percent = 0;
	}
	cxt->exp_skip_num = cxt->cur_param->exp_skip_num;
	cxt->gain_skip_num = cxt->cur_param->gain_skip_num;


	if (_is_cap_use_preview_tbl()) {
		cxt->tuning_param_enable[AE_WORK_MODE_CAPTURE] = 0;
	}

	return AE_SUCCESS;
}

void* ae_alc_init(struct ae_init_in *param, struct ae_init_out *result)
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
	if (AE_ERROR == rtn) {
		AE_LOGE("init param is failed!");
		goto ERR_EXIT;
	}

	cur_status = &cxt->cur_status;
	cur_param = cxt->cur_param;
	misc_init_in.alg_id = cur_param->alg_id;
	misc_init_in.start_index = cur_param->start_index;
#if 0
	misc_handle = ae_misc_init(&misc_init_in, &misc_init_out);
	if (NULL == misc_handle) {
		AE_LOGE("misc alloc is error!");
		rtn = AE_ALLOC_ERROR;
		goto ERR_EXIT;
	}
#endif

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
	_fdae_init(cxt);

//----------------------------------------------------
//awb otp
//----------------------------------------------------
	extern void split_lsc_otp_random(uint16_t* buffer);
	extern void correct_lsc_otp_random_16(uint16_t* buffer);

	TT_AlAisInterface* aptIns = &cxt->mttAis;


	aptIns->mttLscLineData.mpiOtp 		= param->lsc_otp_random;
	aptIns->mttLscLineData.mpiOtpRef	= param->lsc_otp_golden;
	aptIns->mttLscLineData.muiWidth		= param->lsc_otp_width;
	aptIns->mttLscLineData.muiHight		= param->lsc_otp_height;

	if (param->lsc_otp_random != NULL) // color order: GrRBGb, GrRBGb, ..., GrRBGb, GrRBGb, GrRBGb
	{
		split_lsc_otp_random(aptIns->mttLscLineData.mpiOtp ); // now color order: GrGrGr...GrGr, RRR...RR, BBB...BB, GbGbGb...GbGb
		correct_lsc_otp_random_16(aptIns->mttLscLineData.mpiOtp);
	}

	AWB_CTRL_LOGE("[otp] r=%p g=%p  w=%d h=%d ",param->lsc_otp_random,param->lsc_otp_golden,param->lsc_otp_width,param->lsc_otp_height);

//---
	if(( param->otp_info.gldn_stat_info.g != 0 ) && ( param->otp_info.rdm_stat_info.g != 0 ) )
	{
		aptIns->mttAwbLineData.muiMode = 1 | 0x80000000; //On(gldn provide by system)
		//Do not substract OB here!!
		//Random
		aptIns->mttAwbLineData.mttLo.muiR = param->otp_info.rdm_stat_info.r;
		aptIns->mttAwbLineData.mttLo.muiG = param->otp_info.rdm_stat_info.g;
		aptIns->mttAwbLineData.mttLo.muiB = param->otp_info.rdm_stat_info.b;
		aptIns->mttAwbLineData.mttHi.muiR = param->otp_info.rdm_stat_info.r;
		aptIns->mttAwbLineData.mttHi.muiG = param->otp_info.rdm_stat_info.g;
		aptIns->mttAwbLineData.mttHi.muiB = param->otp_info.rdm_stat_info.b;

		//Golden
		aptIns->mttAwbLineData.mttRefLo.muiR = param->otp_info.gldn_stat_info.r;
		aptIns->mttAwbLineData.mttRefLo.muiG = param->otp_info.gldn_stat_info.g;
		aptIns->mttAwbLineData.mttRefLo.muiB = param->otp_info.gldn_stat_info.b;
		aptIns->mttAwbLineData.mttRefHi.muiR = param->otp_info.gldn_stat_info.r;
		aptIns->mttAwbLineData.mttRefHi.muiG = param->otp_info.gldn_stat_info.g;
		aptIns->mttAwbLineData.mttRefHi.muiB = param->otp_info.gldn_stat_info.b;
	}
	else
	{//invalid data -> force OFF
		aptIns->mttAwbLineData.muiMode = 0;	//off
	}




	//if( cxt->camera_id == 0 )
	AWB_CTRL_LOGE("lsc random = %p, golden = %p, %d x %d", param->lsc_otp_random, param->lsc_otp_golden, param->lsc_otp_width, param->lsc_otp_height);
//	AWB_CTRL_LOGD("[otp]tar,ref :%p, %p ,log: e h %d,%d, ",aptIns->mttLscLineData.mpiOtp,aptIns->mttLscLineData.mpiOtpRef,aptIns->mttLscLineData.muiWidth,aptIns->mttLscLineData.muiHight);
	if(( param->lsc_otp_random != 0 ) && ( param->lsc_otp_golden != 0 ) && 
		( param->lsc_otp_height != 0 ) && ( param->lsc_otp_width != 0 ))
	{
		aptIns->mttLscLineData.muiMode		= 1;
	}
	else
	{//disable lsc otp 
		aptIns->mttLscLineData.muiMode		= 0;	
	}
//--------------------------------------------------------
	aptIns->muiModuleNo = param->camera_id;	//0:Back/Rear camera ,1:Front camera
	AlAisIfInit(&cxt->mttAis);
	
	return (void *)cxt;

ERR_EXIT:

	if (NULL != misc_handle) {
	//	ae_misc_deinit(misc_handle, NULL, NULL);
		misc_handle = NULL;
	}

	if (NULL != cxt) {
		free(cxt);
		cxt = NULL;
	}

	return NULL;
}

int32_t ae_alc_deinit(void *handle, void *in_param, void *out_param)
{
	UNUSED(in_param);
	UNUSED(out_param);
	int32_t rtn = AE_SUCCESS;
	struct ae_ctrl_context *cxt = NULL;

	rtn = _check_handle(handle);
	if (AE_SUCCESS != rtn) {
		return AE_ERROR;
	}

	cxt = (struct ae_ctrl_context *)handle;
#if 0
	rtn = ae_misc_deinit(cxt->misc_handle, NULL, NULL);
	if (AE_SUCCESS != rtn) {
		AE_LOGE("misc deinit is failed!");
		return AE_ERROR;
	}
#endif
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

	//AE_LOGI("AE_TEST: --------_calc_post_proc----start----num:%d, %d, %d", cxt->cur_result.exp.num, misc_calc_out->cur_index,cxt->prv_index);
	
	cxt->write_conter = 0;

	if ((0 == cxt->cur_result.exp.num)
		&& (cxt->prv_index != misc_calc_out->cur_index)) {
		cxt->prv_index == misc_calc_out->cur_index;
	}

	if (misc_calc_out->cur_index == cxt->prv_index) {

		if (cxt->flash_param.flash_state == AE_FLASH_STATE_ACTIVE) {
			if (cxt->convergerd_num >= AE_CONVERGED_NUM) {
				is_converged = 1;
			}
			cxt->convergerd_num ++;
		} else {
			is_converged = 1;
		}
		need_exp_write_sensor = 0;
	} else {
		cxt->convergerd_num = 0;
		is_converged = 0;
		cxt->prv_index = misc_calc_out->cur_index;

		cxt->write_eb = 1;

		misc_calc_out->cur_exp_line = cxt->cur_result.exp.tab[0].exp_line;
		misc_calc_out->cur_dummy = cxt->cur_result.exp.tab[0].exp_dummy;
		misc_calc_out->cur_again = cxt->cur_result.exp.tab[0].again;
		misc_calc_out->cur_dgain = cxt->cur_result.exp.tab[0].dgain;
	}

	if (is_converged) {
		cxt->ae_state = AE_STATE_CONVERGED;
		cxt->stab_cnt++;

		//check flash
		if (cxt->flash_param.enable) {
			switch (cxt->flash_param.flash_state) {
			case AE_FLASH_STATE_BEFORE:
				cxt->flash_param.before_result = *misc_calc_out;
				if (cxt->isp_ops.callback) {
					AE_LOGI("AE_CB_CONVERGED");
					(*cxt->isp_ops.callback)(cxt->isp_ops.isp_handler, AE_CB_CONVERGED);
				}
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
	

	if (need_exp_write_sensor) {
		_write_to_sensor(cxt, misc_calc_out, 0);
		cxt->write_conter++;

		//AE_LOGI("AE_TEST: --------_calc_post_proc--------num:%d, conter:%d", cxt->cur_result.exp.num, cxt->write_conter);

		if ((0 == cxt->cur_result.exp.num)
			||(cxt->cur_result.exp.num == cxt->write_conter))
		{
			cxt->write_eb = 0;
			cxt->monitor_unit.cfg.skip_num = cxt->monitor_skip_num;
			_cfg_monitor_skip(cxt, &cxt->monitor_unit.cfg);
			_cfg_monitor_bypass(cxt);
		}
	}else{
		if (cxt->cur_result.exp.num == cxt->write_conter) 
		{
			cxt->write_eb = 0;
			cxt->monitor_unit.cfg.skip_num = 0;
			_cfg_monitor_skip(cxt, &cxt->monitor_unit.cfg);
			_cfg_monitor_bypass(cxt);
		}
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
	++cxt->skip_frame_cnt;

DROP_STAT:

	AE_LOGV("rtn=%d", rtn);

	return rtn;
}

int32_t ae_alc_calculation(void* handle, struct ae_calc_in *param, struct ae_calc_out *result)
{
	int32_t rtn = AE_ERROR;
	struct ae_ctrl_context *cxt = NULL;
	struct ae_misc_calc_in misc_calc_in = {0};
	struct ae_misc_calc_out misc_calc_out = {0};
	uint32_t skip_flag = 0;

	if ((NULL == param) || (NULL == result)) {
		AE_LOGE("in %p out %p param is error!", param, result);
		return AE_PARAM_NULL;
	}

	rtn = _check_handle(handle);
	AE_RETURN_IF_FAIL(rtn, ("handle = %p", handle));

	cxt = (struct ae_ctrl_context *)handle;
	pthread_mutex_lock(&cxt->status_lock);

	if( cxt->muiFlashSeq == AIS_FLASH_SEQ_NONE )
	{
		if (cxt->is_drop_stat) {		//Statsを捨てる指示		立ち上げ時などで使用している。
			cxt->is_drop_stat = 0;
			rtn = AE_ERROR;
									AE_LOGE("_set_flash_notice : DROP_STAT", param, result);
			goto DROP_STAT;
		}

			#if 0
		if (!_is_calc_lum(cxt)) {		//SPRD AE 時	AEを処理したくない時に使っていた。
									AE_LOGE("_set_flash_notice : AE_ERROR", param, result);
			rtn = AE_ERROR;
			goto EXIT;
		}
			#endif

		if (cxt->skip_calc_num) {		//現在、使用していないとのこと。
									AE_LOGE("_set_flash_notice : skip_flag", param, result);
			cxt->skip_calc_num --;
			skip_flag = 1;
			goto EXIT;
		}

	}

	_update_status(cxt);
	rtn = AE_SUCCESS;

	misc_calc_in.awb_gain.r = param->awb_gain_r;
	misc_calc_in.awb_gain.g = param->awb_gain_g;
	misc_calc_in.awb_gain.b = param->awb_gain_b;
	misc_calc_in.stat_img = param->rgb_stat_img;
	misc_calc_in.win_num = cxt->monitor_unit.win_num;
	misc_calc_in.win_size = cxt->monitor_unit.win_size;
	misc_calc_in.stat_mode = cxt->stat_req.mode;
#if 0
	rtn = ae_misc_calculation(cxt->misc_handle, &misc_calc_in, &misc_calc_out);
	if (AE_SUCCESS == rtn) {
		cxt->valid_stat = 1;
		cxt->cur_result = misc_calc_out;

		rtn = _calc_post_proc(cxt, &misc_calc_in, &misc_calc_out);

		if (cxt->is_mlog) {
			save_to_mlog_file(0, &cxt->cur_result);
		}
	} else {
		AE_LOGE("misc calc rtn %d is error!", rtn);
	}
#else
/*
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
*/
//
		AE_LOGI("ae_calculation:line_time=%d  frame_line=%d",cxt->resolution_info.line_time,cxt->resolution_info.frame_line);

		cxt->mttAis.muiBlkH = 32;
		cxt->mttAis.muiBlkV = 32;
		cxt->mttAis.muiBlkPixH = cxt->monitor_unit.win_size.w;
		cxt->mttAis.muiBlkPixV = cxt->monitor_unit.win_size.h;

		cxt->mttAis.mpvWdAe = param->yiq_stat_img;
		cxt->mttAis.mpvWdAwb= param->rgb_stat_img;
		cxt->mttAis.muiHDTime = cxt->resolution_info.line_time;

		AE_LOGI("ae_state=%d, frame_cnt=%d, rgb_stat_img=%p, yiq_stat_img=%p", cxt->ae_state, cxt->skip_frame_cnt,param->rgb_stat_img,param->yiq_stat_img);

		//s-koba
		#if 1
		{
			AE_LOGI("muiBlkPixH,V=%d,%d", cxt->mttAis.muiBlkPixH, cxt->mttAis.muiBlkPixV);

			/*	struct ae_status {
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
			*/

		//	cxt->mttAis.msqMaxFPS	=	cxt->cur_status.fps.max_fps;		// Effective value of FPS.(Max Frame Rate)[s15.16]
			cxt->mttAis.muiLineTime	=	cxt->resolution_info.line_time;		// usec/10	cxt->resolution_info.line_time=134=13.4usec
	#if 0
			cxt->mttAis.muiHDTime	=	cxt->mttAis.muiLineTime * cxt->mttAis.muiBlkPixV*2 / 10;		//100*us[u32.00]
	#else
			cxt->mttAis.muiHDTime	=	cxt->mttAis.muiLineTime * 10;		//100*us[u32.00]	cxt->mttAis.muiHDTime=1340=13.4usec
	#endif
			AE_LOGI("max_fps=0x%08x, min_fps=0x%08x, line_time=0x%08x", cxt->cur_status.fps.max_fps, cxt->cur_status.fps.min_fps, cxt->resolution_info.line_time);
		}
		#endif

		//s-koba
		#if 0
		{
			int32_t asiLp;
			uint32_t* auiSrc;

			auiSrc = (uint32_t*)cxt->mttAis.mpvWdAe;

	//		AE_LOGI("AE STATS");
			for (asiLp = 0; asiLp < 32; asiLp++)
			{
				AE_LOGI("AE STATS (%d) [%02d]	%08x	%08x	%08x	%08x	%08x	%08x	%08x	%08x	%08x	%08x	%08x	%08x	%08x	%08x	%08x	%08x	%08x	%08x	%08x	%08x	%08x	%08x	%08x	%08x	%08x	%08x	%08x	%08x	%08x	%08x	%08x	%08x",
					cxt->mttAis.muiDebug[0],
					asiLp,
					auiSrc[asiLp*32+ 0],auiSrc[asiLp*32+ 1],auiSrc[asiLp*32+ 2],auiSrc[asiLp*32+ 3],auiSrc[asiLp*32+ 4],auiSrc[asiLp*32+ 5],auiSrc[asiLp*32+ 6],auiSrc[asiLp*32+ 7],
					auiSrc[asiLp*32+ 8],auiSrc[asiLp*32+ 9],auiSrc[asiLp*32+10],auiSrc[asiLp*32+11],auiSrc[asiLp*32+12],auiSrc[asiLp*32+13],auiSrc[asiLp*32+14],auiSrc[asiLp*32+15],
					auiSrc[asiLp*32+16],auiSrc[asiLp*32+17],auiSrc[asiLp*32+18],auiSrc[asiLp*32+19],auiSrc[asiLp*32+20],auiSrc[asiLp*32+21],auiSrc[asiLp*32+22],auiSrc[asiLp*32+23],
					auiSrc[asiLp*32+24],auiSrc[asiLp*32+25],auiSrc[asiLp*32+26],auiSrc[asiLp*32+27],auiSrc[asiLp*32+28],auiSrc[asiLp*32+29],auiSrc[asiLp*32+30],auiSrc[asiLp*32+31]);
			}
		}
		#endif

		cxt->mttAis.muiDualFlashMode	=	0;				//Single : 0, Dual : 1

		AlAisIfMain(&cxt->mttAis);

		//Flash
		{
		//	if( cxt->muiFlashSeq != AIS_FLASH_SEQ_NONE )
		//	{
		//		AE_LOGI("_set_flash_notice : muiFlashSeq=%d", cxt->muiFlashSeq);
		//	}
			if( cxt->muiFlashSeq == AIS_FLASH_SEQ_TOUCH_ON )	//LED Focusing Seq Trigger(0:Off,1:Request)
			{
				if( cxt->muiFlashSeqFrameCount > 4 ) {
					if( (cxt->mttAis.muiAeSettle == 1) || (cxt->muiFlashSeqFrameCount >= 60) )	//60 frame = time out
					{
						if (cxt->isp_ops.callback) {
							AE_LOGI("_set_flash_notice : [C1] AE_CB_FLASHING_CONVERGED");
							(*cxt->isp_ops.callback)(cxt->isp_ops.isp_handler, AE_CB_FLASHING_CONVERGED);
						}
						cxt->muiFlashSeq			=	AIS_FLASH_SEQ_AF_SEQ;
						cxt->mttAis.muiFcsReq		=	1;	//LED Focusing Seq Trigger(0:Off,1:Request)
				
						_ais_cmd_set_ae_LockMode(&cxt->mttAis, ALAIS_EXPOS_LOCK_MODE_ON);
					}
				}
				cxt->muiFlashSeqFrameCount++;
			}
			if( cxt->muiFlashSeq == AIS_FLASH_SEQ_TRIGGER )
			{
				if( cxt->muiFlashSeqFrameCount == 4 )
				{
					AE_LOGI("_set_flash_notice : [C2] AE_CB_CLOSE_PREFLASH");
					(*cxt->isp_ops.callback)(cxt->isp_ops.isp_handler, AE_CB_CLOSE_PREFLASH);
				}
				AE_LOGI("_set_flash_notice : muiFlashSeqFrameCount=%d",cxt->muiFlashSeqFrameCount);
				cxt->muiFlashSeqFrameCount++;
			}
			else
			if( cxt->muiFlashSeq == AIS_FLASH_SEQ_TRIGGER_P3 )
			{
				if( cxt->muiFlashSeqFrameCount == 8 )
				{
					AE_LOGI("_set_flash_notice : [C3] AE_CB_PREFLASH_PERIOD_END");
					(*cxt->isp_ops.callback)(cxt->isp_ops.isp_handler, AE_CB_CLOSE_PREFLASH);

					_ais_cmd_set_ae_LockMode(&cxt->mttAis, ALAIS_EXPOS_LOCK_MODE_ON);

					//ここで、Flash Exposeを保存しとく
					cxt->cur_result_flash_save.cur_exposure	=	cxt->mttAis.mflExpTime[0][0] * 10000000;
					cxt->cur_result_flash_save.cur_exp_line =	cxt->mttAis.muiExpLine[0][0];
					cxt->cur_result_flash_save.cur_again	=	(uint32_t)(cxt->mttAis.mflSensor_gain[0][0]*128);

					AE_LOGI("_set_flash_notice : save muiLedBright=%d",cxt->mttAis.muiLedBright);
					cxt->cur_result_flash_save_LedBright	=	(cxt->mttAis.muiLedBright>4)?(4):(cxt->mttAis.muiLedBright);
				}
				if( cxt->muiFlashSeqFrameCount >= 8+2 )
				{
					/*
						(m0)  AE_FLASH_MAIN_BEFORE ====>
							C3の後、M0が２フレーム以内にこなかったらFlashシーケンスを抜ける処理が必要
					*/
					cxt->muiFlashSeq			=	AIS_FLASH_SEQ_NONE;
					cxt->muiFlashSeqFrameCount	=	-1;
					cxt->mttAis.muiFcsReq		=	0;	//LED Focusing Seq Trigger(0:Off,1:Request)
					cxt->mttAis.muiLedReq		=	0;	//LED TTL Request  Trigger(0:Off,1:Request)
					cxt->mttAis.muiLedSta		=	0;	//LED Status(0:Off,bit0=1:Tourch,bit1=1:Flash,bit15=1:Configuration in Progress(0x8000))
					cxt->mttAis.muiFlashReq		=	0;	//LED (NEW)Input to start or stop a "Flash" Sequence(0:Stop/1:Start)
					_ais_cmd_set_ae_LockMode(&cxt->mttAis, ALAIS_EXPOS_LOCK_MODE_OFF);
				
					cxt->use_flash_save			=	0;
				}
				cxt->muiFlashSeqFrameCount++;
			}
			if( cxt->muiFlashSeq == AIS_FLASH_SEQ_TRIGGER_M0 )
			{
			}
			if( cxt->muiFlashSeq == AIS_FLASH_SEQ_FLASH_ON )
			{
				if( cxt->muiFlashSeqFrameCount == 1 )
				{
					AE_LOGI("_set_flash_notice : [C4] AE_CB_CLOSE_MAIN_FLASH");
					(*cxt->isp_ops.callback)(cxt->isp_ops.isp_handler, AE_CB_CLOSE_MAIN_FLASH);

					cxt->muiFlashSeq			=	AIS_FLASH_SEQ_NONE;
					cxt->muiFlashSeqFrameCount	=	-1;
					cxt->mttAis.muiFcsReq		=	0;	//LED Focusing Seq Trigger(0:Off,1:Request)
					cxt->mttAis.muiLedReq		=	0;	//LED TTL Request  Trigger(0:Off,1:Request)
					cxt->mttAis.muiLedSta		=	0;	//LED Status(0:Off,bit0=1:Tourch,bit1=1:Flash,bit15=1:Configuration in Progress(0x8000))
					cxt->mttAis.muiFlashReq		=	0;	//LED (NEW)Input to start or stop a "Flash" Sequence(0:Stop/1:Start)

					cxt->use_flash_save			=	0;

					_ais_cmd_set_ae_LockMode(&cxt->mttAis, ALAIS_EXPOS_LOCK_MODE_OFF);
				}
				cxt->muiFlashSeqFrameCount++;
			}
		}
		if( cxt->use_flash_save	!=	0 )
		{
			cxt->cur_result.cur_exposure	=	cxt->cur_result_flash_save.cur_exposure	;
			cxt->cur_result.cur_exp_line 	=	cxt->cur_result_flash_save.cur_exp_line ;
			cxt->cur_result.cur_again		=	cxt->cur_result_flash_save.cur_again	;
			result->is_stab					=	1;
		}
		else
		{
			cxt->cur_result.cur_exposure	=	cxt->mttAis.mflExpTime[0][0] * 10000000;
			cxt->cur_result.cur_exp_line 	=	cxt->mttAis.muiExpLine[0][0];
			cxt->cur_result.cur_again		=	(uint32_t) (cxt->mttAis.mflSensor_gain[0][0]*128);
			result->is_stab					=	1;
		}

#if 0
cxt->cur_result.cur_exp_line 	=	2865;
//cxt->cur_result.cur_again		=	(uint32_t) (10.0f*128);
//cxt->cur_result.cur_again		=	(uint32_t) (10.0f*128);
//cxt->cur_result.cur_again		=	0x04a0/4;
cxt->cur_result.cur_again		=	(uint32_t)((float)19.0f * 128);

cxt->cur_result.cur_exp_line 	=	4477;
cxt->cur_result.cur_again		=	(uint32_t)((float)10.0f * 128);
#endif

		AE_LOGI("cur_exp_line=%d, cur_again=0x%08x, cur_exposure=%d", cxt->cur_result.cur_exp_line, cxt->cur_result.cur_again, cxt->cur_result.cur_exposure);

		result->log_ae.log  = cxt->mttAis.mpiLog;
		result->log_ae.size = cxt->mttAis.muiLogSize;
		#if 0
		{
			char	str[]="AlAis jpg log test @@@@@@@@@@@----------============11111111112222222223333333";
			memcpy(cxt->mttAis.mpiLog,str,sizeof(str));
			result->log_ae.size = sizeof(str);
		}
		#endif

		rtn = _calc_post_proc(cxt, &misc_calc_in, &cxt->cur_result);
		_write_to_sensor(cxt, &cxt->cur_result, 0);
		rtn = AE_SUCCESS;
		
		


#endif

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

	//if (1 == skip_flag)
		{
		_cfg_monitor_bypass(cxt);
	}
	AE_LOGV("calc_lum=%d, cur_index=%d, is_stab=%d", result->cur_lum, result->cur_index, result->is_stab);
	AE_LOGV("target_lum=%d, ae_state=%d", result->target_lum, cxt->ae_state);

	pthread_mutex_unlock(&cxt->status_lock);

	AE_LOGV("rtn=%d", rtn);

	return rtn;
}

static int32_t _sof_handler(struct ae_ctrl_context *cxt)
{
	int32_t rtn = AE_SUCCESS;
	int32_t write_num = 1;

	if(1 == cxt->write_eb)
	{
		_write_exp_gain(cxt);

		if (0 == cxt->write_eb) {
				cxt->monitor_unit.cfg.skip_num = cxt->monitor_skip_num;
				_cfg_monitor_skip(cxt, &cxt->monitor_unit.cfg);
				_cfg_monitor_bypass(cxt);
		}
	}

	_fdae_update_state(cxt);

	return rtn ;
}

static int32_t _set_pause(struct ae_ctrl_context *cxt)
{
	int32_t rtn = AE_SUCCESS;


	if (cxt->is_force_lock) {
		AE_LOGI("warning force lock!");
		return AE_ERROR;
	}

	if (0 == cxt->pause_ref_cnt) {
		cxt->pause_ae_state = cxt->ae_state;
		cxt->backup_result = cxt->cur_result;
		cxt->ae_state = AE_STATE_LOCKED;
	}

	++cxt->pause_ref_cnt;

	AE_LOGI("ae_state=%d", cxt->ae_state);

	return rtn;
}

static int32_t _set_restore(struct ae_ctrl_context *cxt, struct ae_calc_out *result)
{
	int32_t rtn = AE_SUCCESS;


	if (cxt->is_force_lock) {
		AE_LOGI("warning force lock!");
		return AE_ERROR;
	}

	if (0 == cxt->pause_ref_cnt) {
		AE_LOGI("warning call at out of rule!");
		return AE_ERROR;
	}

	--cxt->pause_ref_cnt;

	if (AE_STATE_LOCKED == cxt->ae_state
		&& 0 == cxt->pause_ref_cnt) {
		cxt->cur_result = cxt->backup_result;

		if (result) {
			_convert_result_to_out(&cxt->cur_result, result);
			_cfg_monitor_bypass(cxt);
		}
		cxt->ae_state = cxt->pause_ae_state;
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
#if 0
		rtn = ae_misc_io_ctrl(cxt->misc_handle,
					AE_MISC_CMD_GET_EXP_BY_INDEX,
					(void *)&ctrl_index,
					&ctrl_result);
#else
	ctrl_index = 0;
#endif
		if (AE_SUCCESS == rtn) {
			_write_to_sensor(cxt, &ctrl_result, 1);
		}
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

	AE_LOGV("is_scene=%d", is_scene);
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
#if 0
	ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_CONVERGE_SPEED, &convergence_speed, 
	&(cxt->fdae.ae_convergence_speed));
#endif
}

static void _fdae_leave_fdae_mode(struct ae_ctrl_context *cxt)
{
	/* restore the AE states */
//	ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_CONVERGE_SPEED, &(cxt->fdae.ae_convergence_speed), NULL);	
	
    /* restore the AE weight table */
    {
#if 0 
       int16_t weight_mode = cxt->cur_status.weight;
        struct ae_weight_table *cur_base_tbl = &(cxt->cur_param->weight_table[weight_mode]);    
        ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_WEIGHT_TABLE,
                        (void *)cur_base_tbl, NULL);    
#endif  
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
#if 0
	/* Change AE weight table */
	ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_WEIGHT_TABLE,
					(void *)work_tbl, NULL);
	AE_LOGI("FDAE: ->updated weight table, frame_idx=%d", cxt->fdae.curr_frame_idx);
#endif
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



int32_t ae_alc_io_ctrl(void* handle, enum ae_io_ctrl_cmd cmd, void *param, void *result)
{
	#define	AE_LOGI_ALC		AE_LOGI
//	#define	AE_LOGI_ALC

	int32_t rtn = AE_SUCCESS;
	struct ae_ctrl_context *cxt = NULL;
	struct ae_status *next_status = NULL;

	// TestCode
//	AE_LOGI_ALC("ae_io_ctrl=%d", cmd);

	rtn = _check_handle(handle);
	if (AE_SUCCESS != rtn) {
		AE_RETURN_IF_FAIL(rtn, ("handle = %p", handle));
	}

	cxt = (struct ae_ctrl_context *)handle;
	pthread_mutex_lock(&cxt->status_lock);

	next_status = &cxt->nxt_status;
	switch (cmd) {
	case AE_SET_PROC:
//								AE_LOGI_ALC("AE_SET_PROC");
		_sof_handler(cxt);
		break;
	case AE_SET_WORK_MODE:
								AE_LOGI_ALC("AE_SET_WORK_MODE");
		if (param) {
			struct ae_set_work_param *work_info = param;

			if (work_info->mode >= AE_WORK_MODE_MAX) {
				AE_LOGE("work mode param is error");
				break;
			}
			next_status->work_info = *work_info;

			//ALC_S
			switch( next_status->work_info.mode ) 
			{
			case	AE_WORK_MODE_COMMON	:
				cxt->mttAis.muiCamMode	=	0;	//0:Preview,1:Capture,2:Movie30fps,3:Movie60fps
				break;
			case	AE_WORK_MODE_CAPTURE	:
				cxt->mttAis.muiCamMode	=	1;	//0:Preview,1:Capture,2:Movie30fps,3:Movie60fps
				break;
			case	AE_WORK_MODE_VIDEO	:
				cxt->mttAis.muiCamMode	=	2;	//0:Preview,1:Capture,2:Movie30fps,3:Movie60fps
				break;
			default:
				cxt->mttAis.muiCamMode	=	0;	//0:Preview,1:Capture,2:Movie30fps,3:Movie60fps
				break;
			}
			AlAisIfReset(&cxt->mttAis);
								AE_LOGI_ALC("work_info->mode=%d", work_info->mode);
			//ALC_E

			rtn = ae_work_init(cxt, work_info);
		}
		_cfg_monitor(cxt);
		break;

	case AE_SET_SCENE_MODE:
								AE_LOGI_ALC("AE_SET_SCENE_MODE");
		if (param) {
			struct ae_set_scene *scene_mode = param;

			if (scene_mode->mode < AE_SCENE_MAX) {
				next_status->scene = scene_mode->mode;
				cxt->update_list.is_scene = 1;
			
				//ALC_S
				/*
enum ae_scene_mode {
	AE_SCENE_NORMAL = 0x00,
	AE_SCENE_NIGHT,
	AE_SCENE_SPORT,
	AE_SCENE_PORTRAIT,
	AE_SCENE_LANDSPACE,
	AE_SCENE_MAX
};
				*/
			rtn = _ais_cmd_set_ae_bestshot(&cxt->mttAis, scene_mode->mode);
			rtn = _ais_cmd_rtn_cvt( rtn );
				//ALC_E
			}
		}
		break;

	case AE_SET_ISO:
								AE_LOGI_ALC("AE_SET_ISO");
		if (param) {
			struct ae_set_iso *iso = param;

			if (_is_work_to_scene(cxt, next_status))
				break;
#ifdef ISO_FIX_IN_VIDEO
			if (cxt->work_in_video) {
				iso->mode = VIDEO_IS0;
			}
#endif
			if (iso->mode < AE_ISO_MAX) {
				next_status->iso = iso->mode;
				cxt->update_list.is_iso = 1;
			}
			
			//ALC_S
			{
			uint32_t iso_mode;

			AE_LOGI_ALC("[ALC_AE_TEST]CMD:AE_SET_ISO (Mode:%d)", iso->mode);

			switch( iso->mode ) 
			{
			case AE_ISO_AUTO:
				iso_mode = ALAIS_EXPOS_ISO_MODE_AUTO;		/* ISOオート	*/
				break;
			case AE_ISO_100:
				iso_mode = ALAIS_EXPOS_ISO_MODE_SV05p00;	/* ISO100		*/
				break;
			case AE_ISO_200:
				iso_mode = ALAIS_EXPOS_ISO_MODE_SV06p00;	/* ISO200		*/
				break;
			case AE_ISO_400:
				iso_mode = ALAIS_EXPOS_ISO_MODE_SV07p00;	/* ISO400		*/
				break;
			case AE_ISO_800:
				iso_mode = ALAIS_EXPOS_ISO_MODE_SV08p00;	/* ISO800		*/
				break;
			case AE_ISO_1600:
				iso_mode = ALAIS_EXPOS_ISO_MODE_SV09p00;	/* ISO1600		*/
				break;
			default:
				/* その他はAutoとして扱う */
				iso_mode = ALAIS_EXPOS_ISO_MODE_AUTO;		/* ISOオート	*/
				break;
			/* ALCとして対応しているが、System側未対応項目 */
//				iso_mode = ALAIS_EXPOS_ISO_MODE_SV04p00;	/* ISO50		*/
//				iso_mode = ALAIS_EXPOS_ISO_MODE_SV10p00;	/* ISO3200		*/
//				iso_mode = ALAIS_EXPOS_ISO_MODE_SV11p00;	/* ISO6400		*/
			}
			
			rtn = _ais_cmd_set_ae_IsoMode(&cxt->mttAis, iso_mode);
			rtn = _ais_cmd_rtn_cvt( rtn );
			}
			//ALC_E
		}
		break;

	case AE_GET_ISO:
								AE_LOGI_ALC("AE_GET_ISO");
		if (result) {
			rtn = _get_iso(cxt, result);
			
			//ALC_S
			#if 1		//rtn = _get_iso(cxt, result);	で問題なければこちらは使わない。		
			{
				uint32_t *real_iso = result;

				/*	フィルム感度[ISO] = 100 * 2^(SV-5)	*/
				*real_iso = 100 * pow(2.0f,cxt->mttAis.mflSv-5.0f);
			}
			#endif
			//ALC_E
		}
		break;

	case AE_SET_FLICKER:
								AE_LOGI_ALC("AE_SET_FLICKER");
		if (param) {
			struct ae_set_flicker *flicker = param;

			if (flicker->mode < AE_FLICKER_MAX) {
				next_status->flicker = flicker->mode;
			}
			
			//ALC_S
			#if 1
			{
							/*
							enum ae_flicker_mode {
								AE_FLICKER_50HZ = 0x00,
								AE_FLICKER_60HZ,
								AE_FLICKER_OFF,
								AE_FLICKER_AUTO,
								AE_FLICKER_MAX
							};
							*/
				struct ae_set_flicker *flicker = param;
				uint32_t	flicker_mode;

				AE_LOGI_ALC("[ALC_AE_TEST]CMD:AE_SET_FLICKER (Mode:%d)", flicker->mode);

				switch( flicker->mode ) 
				{
				case AE_FLICKER_50HZ:
					flicker_mode = ALAIS_EXPOS_FLK_MODE_50HZ;
					break;
				case AE_FLICKER_60HZ:
					flicker_mode = ALAIS_EXPOS_FLK_MODE_60HZ;
					break;
				case AE_FLICKER_OFF:
					flicker_mode = ALAIS_EXPOS_FLK_MODE_OFF;
					break;
				default:
					flicker_mode = ALAIS_EXPOS_FLK_MODE_AUTO;
					break;
				/* System側未対応の為、50/60Hz指定以外はAutoとして扱う */
	//				mode = ALAIS_EXPOS_FLK_MODE_AUTO50HZ;
	//				mode = ALAIS_EXPOS_FLK_MODE_AUTO60HZ;
				}
				rtn = _ais_cmd_set_ae_FlickerMode(&cxt->mttAis, flicker_mode);
				rtn = _ais_cmd_rtn_cvt( rtn );
			}
			#endif
			//ALC_E
			next_status->flicker = 0;
		}
		break;

	case AE_SET_WEIGHT:
								AE_LOGI_ALC("AE_SET_WEIGHT");
		if (param) {
			struct ae_set_weight *weight = param;
			AE_LOGV("ae_weight %d",weight->mode);
			if (_is_work_to_scene(cxt, next_status))
				break;
			if (weight->mode < AE_WEIGHT_MAX) {
				next_status->weight = weight->mode;
				cxt->update_list.is_weight = 1;
			}
			//ALC_S
			{
/*
enum ae_weight_mode {
	AE_WEIGHT_AVG = 0x00,
	AE_WEIGHT_CENTER,
	AE_WEIGHT_SPOT,
	AE_WEIGHT_MAX
};
*/
				UI_08		auiIdx;

				switch( weight->mode ) {
				default:
				case AE_WEIGHT_AVG:
					auiIdx	=	0;
					break;
				case AE_WEIGHT_CENTER:
					auiIdx	=	1;
					break;
				case AE_WEIGHT_SPOT:
					auiIdx	=	2;
					break;
				}
				rtn = _ais_cmd_set_ae_weight(&cxt->mttAis, auiIdx);
				rtn = _ais_cmd_rtn_cvt( rtn );
			}
			//ALC_E
		}
		break;

	case AE_SET_TOUCH_ZONE:
								AE_LOGI_ALC("AE_SET_TOUCH_ZONE");
		if (param) {
			struct ae_set_tuoch_zone *touch_zone = param;

			if (_is_work_to_scene(cxt, next_status))
				break;
			next_status->touch_zone = touch_zone->touch_zone;
			cxt->update_list.is_touch_zone = 1;
			//ALC_S
/*
struct ae_set_tuoch_zone {
	struct ae_trim touch_zone;
};
struct ae_trim {
	uint32_t x;
	uint32_t y;
	uint32_t w;
	uint32_t h;
};
*/
			{
				struct ae_trim	*	aptP	=	&touch_zone->touch_zone;
				
				AE_LOGI_ALC("AE_SET_TOUCH_ZONE x,y,w,h,W,H=%d,%d,%d,%d,%d,%d",	aptP->x, aptP->y, aptP->w, aptP->h, cxt->resolution_info.frame_size.w, cxt->resolution_info.frame_size.h );
#if 1
				if( aptP->x==0 && aptP->y==0 && aptP->w==cxt->resolution_info.frame_size.w+1 && aptP->h==cxt->resolution_info.frame_size.h+1 ) {
					_ais_reset_touch_ae(	&cxt->mttAis	);
				} else {
					_ais_set_touch_ae( 
						&cxt->mttAis		,
						aptP->x				,		//	i4Left	(0 ~ + resolution_info.frame_size.w 必須)
						aptP->x+aptP->w		,		//	i4Right
						aptP->y				,		//	i4Top	(0 ~ + resolution_info.frame_size.h 必須)
						aptP->y+aptP->h		,		//	i4Bottom
						cxt->resolution_info.frame_size.w	,
						cxt->resolution_info.frame_size.h	);
				}
#endif
					//ALC_S test
					#if 0					//AE_SET_FLICKER	AE_GET_FLICKER_MODE
					if( aptP->x==0 && aptP->y==0 && aptP->w==cxt->resolution_info.frame_size.w+1 && aptP->h==cxt->resolution_info.frame_size.h+1 )
					{						//AE_GET_FLICKER_MODE
						AE_LOGI_ALC("AE_GET_FLICKER_MODE = %d",cxt->cur_status.flicker);
					}
					else
					{						//AE_SET_FLICKER
						uint32_t	flicker_mode;
		
						static	uint32_t	flicker_index;
		
						switch( flicker_index )
						{
						case 0:
							flicker_mode = ALAIS_EXPOS_FLK_MODE_50HZ;
							flicker_index++;
							break;
						case 1:
							flicker_mode = ALAIS_EXPOS_FLK_MODE_60HZ;
							flicker_index++;
							break;
						case 2:
							flicker_mode = ALAIS_EXPOS_FLK_MODE_OFF;
							flicker_index++;
							break;
						default:
							flicker_mode = ALAIS_EXPOS_FLK_MODE_AUTO;
							flicker_index=0;
							break;
						}
		
						if (flicker_mode < AE_FLICKER_MAX) {
							next_status->flicker = flicker_mode;
						}

						AE_LOGI_ALC("AE_SET_FLICKER = %d",flicker_mode);
		
						rtn = _ais_cmd_set_ae_FlickerMode(&cxt->mttAis, flicker_mode);
						rtn = _ais_cmd_rtn_cvt( rtn );
					}
					#endif
					#if 0					//AE_SET_PAUSE
					{
						AE_LOGI_ALC("AE_SET_PAUSE");
						rtn = _ais_cmd_set_ae_LockMode(&cxt->mttAis, ALAIS_EXPOS_LOCK_MODE_ON);
						rtn = _ais_cmd_rtn_cvt( rtn );
					}
					#endif
					#if 0					//AE_SET_ISO
					{
						uint32_t iso_mode;
		
						AE_LOGI_ALC("[ALC_AE_TEST]CMD:AE_SET_ISO (Mode:%d)", 0);
					//		iso_mode = ALAIS_EXPOS_ISO_MODE_AUTO;		/* ISOオート	*/
					//o		iso_mode = ALAIS_EXPOS_ISO_MODE_SV05p00;	/* ISO100		*/
					//		iso_mode = ALAIS_EXPOS_ISO_MODE_SV06p00;	/* ISO200		*/
					//		iso_mode = ALAIS_EXPOS_ISO_MODE_SV07p00;	/* ISO400		*/
					//		iso_mode = ALAIS_EXPOS_ISO_MODE_SV08p00;	/* ISO800		*/
					//o		iso_mode = ALAIS_EXPOS_ISO_MODE_SV09p00;	/* ISO1600		*/
							/* その他はAutoとして扱う */
					//		iso_mode = ALAIS_EXPOS_ISO_MODE_AUTO;		/* ISOオート	*/
						
						rtn = _ais_cmd_set_ae_IsoMode(&cxt->mttAis, iso_mode);
						rtn = _ais_cmd_rtn_cvt( rtn );
					}
					#endif
					#if 0					//AE_GET_LUM
					{
						uint32_t lum;
							lum = cxt->mttAis.mflAisFrmLuma;
								AE_LOGI_ALC("AE_GET_LUM = %d <%f>",lum,cxt->mttAis.mflAisFrmLuma);
					}
					#endif
					#if 0					//AE_GET_ISO
					{
						uint32_t real_iso,real_iso1;

						rtn = _get_iso(cxt, &real_iso1);

						/*	フィルム感度[ISO] = 100 * 2^(SV-5)	*/
						real_iso = 100 * pow(2.0f,cxt->mttAis.mflSv-5.0f);

								AE_LOGI_ALC("AE_GET_ISO = %d <_get_iso()=%d>",real_iso,real_iso1);
					}
					#endif
					#if 0					//AE_GET_EV
					{
						uint32_t i = 0x00;
						struct ae_ev_table *ev_table = &cxt->cur_param->ev_table;
						int32_t target_lum = cxt->cur_param->target_lum;
						struct ae_get_ev *ev_info = (struct ae_get_ev*)result;

						//ALC_S
						int32_t target_lum1 = cxt->mttAis.mflAisTrgLuma;
						//ALC_E

								AE_LOGI_ALC("AE_GET_EV : target_lum=%d, mflAisTrgLuma=%d>", target_lum, cxt->mttAis.mflAisTrgLuma);

					//	ev_info->ev_index = cxt->cur_status.ev_level;

					//	for (i = 0; i < ev_table->diff_num; i++) {
					//		ev_info->ev_tab[i] = target_lum + ev_table->lum_diff[i];
					//		//AE_LOGE("EV_TEST: %d ,i=%d, ev=%d, target=%d, diff=%d",ev_info->ev_index, i, ev_info->ev_tab[i], target_lum, ev_table->lum_diff[i]);
					//	}

						rtn = AE_SUCCESS;
					}
					#endif
					#if 0					//AE_GET_EXP
					{
						float exposure = (float)cxt->cur_result.cur_exposure / 10000000.0;
						AE_LOGI_ALC("AE_GET_EXP : exp = %d, %f", cxt->cur_result.cur_exposure, exposure);
					}
					#endif
					#if 0					//AE_GET_MONITOR_INFO
					{
						struct ae_monitor_info info;

						info.win_size = cxt->monitor_unit.win_size;
						info.win_num = cxt->monitor_unit.win_num;
						info.trim = cxt->monitor_unit.trim;

						AE_LOGI_ALC("AE_GET_MONITOR_INFO : win_size.w .h=%d,%d, win_num.w .h=%d,%d, trim.x .y .w .h=%d,%d,%d,%d",
							info.win_size.w,	info.win_size.h,
							info.win_num.w,	info.win_num.h,
							info.trim.x,	info.trim.y,	info.trim.w,	info.trim.h
						);
					}
					#endif
					#if 0					//AE_SET_TARGET_LUM
					{
						uint32_t lum = 200;

						next_status->target_lum = lum;
						cxt->update_list.is_target_lum = 1;

						{
						uint8_t		Target;

						// AE_SET_TARGET_LUM = SPRD Debug command
						AE_LOGI_ALC("[ALC_AE_TEST]CMD:AE_SET_TARGET_LUM (Target:%d)", lum);
					
						// 128=18% Base      0=0%、255=100%
						if (lum <= 128)
						{
							Target = (uint8_t)(lum * 18 / 128);
						}
						else
						{
							Target = (uint8_t)(((lum - 128) * (100 - 18) / 127) + 18 );
						}
						
						rtn = _ais_cmd_set_ae_Target(&cxt->mttAis, Target);
						rtn = _ais_cmd_rtn_cvt( rtn );
						}
					}
					#endif
					#if 0					//AE_SET_FPS
					{
						struct ae_set_fps fps = {30,30};

						if (_is_work_to_scene(cxt, next_status))
							break;
						next_status->fps = fps;
						cxt->update_list.is_fps = 1;

						{
						uint16_t	min_fps = (uint16_t)fps.min_fps;
						uint16_t	max_fps = (uint16_t)fps.max_fps;

						AE_LOGI_ALC("[ALC_AE_TEST]CMD:AE_SET_FPS (minfps:%d maxfps:%d)", min_fps, max_fps);

						if( min_fps == 0 )	min_fps=15;
						if( max_fps == 0 )	min_fps=30;
						
				///		min_fps = (uint16_t)(((float)1/min_fps)*(1<<8));
				///		max_fps = (uint16_t)(((float)1/max_fps)*(1<<8));
						min_fps = (uint16_t)min_fps*(1<<8);
						max_fps = (uint16_t)max_fps*(1<<8);

						rtn = _ais_cmd_set_ae_setFpsCtrl(&cxt->mttAis, min_fps, max_fps);
						rtn = _ais_cmd_rtn_cvt( rtn );
						}
					}
					#endif
					#if 0					//AE_GET_FLICKER_MODE
					{
						//int32_t ae_io_ctrl(void* handle, enum ae_io_ctrl_cmd cmd, void *param, void *result)
						uint32_t	mode;
						struct ae_set_flicker flicker;
						flicker.mode = AE_FLICKER_60HZ;
						AE_LOGI_ALC("AE_SET_FLICKER---------------------------------------");
				//	??	rtn = ae_io_ctrl((void *)cxt, AE_SET_FLICKER,			(void *)&flicker,		NULL				);
				//	??	rtn = ae_io_ctrl((void *)cxt, AE_GET_FLICKER_MODE,		NULL,					(void *)&mode		);
					}
					#endif
					#if 0					//AE_SET_EV_OFFSET
					{
						uint32_t	ev_level	=	14;
						if (_is_work_to_scene(cxt, next_status))
							break;
						if (ev_level < AE_LEVEL_MAX) {
							next_status->ev_level = ev_level;
							cxt->update_list.is_ev = 1;

							//ALC_S
										AE_LOGI_ALC("AE_SET_EV_OFFSET---------------------------------------");
							{
								rtn = _ais_cmd_set_ae_Brightness(&cxt->mttAis, ev_level);
								rtn = _ais_cmd_rtn_cvt( rtn );
							}
							//ALC_E
						}
					}
					#endif
					#if 0					//AE_SET_WEIGHT:
					{
						UI_08		auiIdx;
						UI_08		weight_mode=AE_WEIGHT_SPOT;

						switch( weight_mode ) {
						default:
						case AE_WEIGHT_AVG:
							auiIdx	=	0;
							break;
						case AE_WEIGHT_CENTER:
							auiIdx	=	1;
							break;
						case AE_WEIGHT_SPOT:
							auiIdx	=	2;
							break;
						}
										AE_LOGI_ALC("AE_SET_WEIGHT---------------------------------------");
						rtn = _ais_cmd_set_ae_weight(&cxt->mttAis, auiIdx);
						rtn = _ais_cmd_rtn_cvt( rtn );
					}
					#endif
					#if 0					//AE_SET_SCENE_MODE
					{
				/*
enum ae_scene_mode {
	AE_SCENE_NORMAL = 0x00,
	AE_SCENE_NIGHT,
	AE_SCENE_SPORT,
	AE_SCENE_PORTRAIT,
	AE_SCENE_LANDSPACE,
	AE_SCENE_MAX
};
				*/
						uint32_t	scene_mode	=	AE_SCENE_NIGHT;
										AE_LOGI_ALC("AE_SET_SCENE_MODE---------------------------------------");
						rtn = _ais_cmd_set_ae_bestshot(&cxt->mttAis, scene_mode);
						rtn = _ais_cmd_rtn_cvt( rtn );
					}
					#endif
					#if 0					//AE_GET_AE_STATE
					{
						if( aptP->x==0 && aptP->y==0 && aptP->w==cxt->resolution_info.frame_size.w+1 && aptP->h==cxt->resolution_info.frame_size.h+1 ) {
							AE_LOGI_ALC("AE_GET_AE_STATE=%d---------------------------------------",cxt->ae_state);
						} else {
							AE_LOGI_ALC("AE_SET_PAUSE=%d---------------------------------------",cxt->ae_state);
							rtn = _set_pause(cxt);

							rtn = _ais_cmd_set_ae_LockMode(&cxt->mttAis, ALAIS_EXPOS_LOCK_MODE_ON);
							rtn = _ais_cmd_rtn_cvt( rtn );
						}
					}
					#endif
					//ALC_E test

			}
			//ALC_E
		}
		break;

	case AE_SET_EV_OFFSET:
								AE_LOGI_ALC("AE_SET_EV_OFFSET");
		if (param) {
			struct ae_set_ev *ev = param;

			if (_is_work_to_scene(cxt, next_status))
				break;
			if (ev->level < AE_LEVEL_MAX) {
				next_status->ev_level = ev->level;
				cxt->update_list.is_ev = 1;

				//ALC_S
				{
					rtn = _ais_cmd_set_ae_Brightness(&cxt->mttAis, ev->level);
					rtn = _ais_cmd_rtn_cvt( rtn );
				}
				//ALC_E
			}
		}
		break;

	case AE_SET_FPS:
								AE_LOGI_ALC("AE_SET_FPS");
		if (param) {
			struct ae_set_fps *fps = param;

			if (_is_work_to_scene(cxt, next_status))
				break;
			next_status->fps = *fps;
			cxt->update_list.is_fps = 1;
/*
			struct ae_set_fps {
				uint32_t min_fps; // min fps
				uint32_t max_fps; // fix fps flag
			};
*/
			//ALC_S
			{
				uint16_t	min_fps = (uint16_t)fps->min_fps;
				uint16_t	max_fps = (uint16_t)fps->max_fps;

				AE_LOGI_ALC("[ALC_AE_TEST]CMD:AE_SET_FPS (minfps:%d maxfps:%d)", min_fps, max_fps);

				if( min_fps == 0 )	min_fps=15;
				if( max_fps == 0 )	min_fps=30;
				
		///		min_fps = (uint16_t)(((float)1/min_fps)*(1<<8));
		///		max_fps = (uint16_t)(((float)1/max_fps)*(1<<8));
				min_fps = (uint16_t)min_fps*(1<<8);
				max_fps = (uint16_t)max_fps*(1<<8);

				rtn = _ais_cmd_set_ae_setFpsCtrl(&cxt->mttAis, min_fps, max_fps);
				rtn = _ais_cmd_rtn_cvt( rtn );
			}
			//ALC_E
		}
		break;

	case AE_SET_PAUSE:
								AE_LOGI_ALC("AE_SET_PAUSE");
		rtn = _set_pause(cxt);

		//ALC_S
		rtn = _ais_cmd_set_ae_LockMode(&cxt->mttAis, ALAIS_EXPOS_LOCK_MODE_ON);
		rtn = _ais_cmd_rtn_cvt( rtn );
		//ALC_E

		break;
	case AE_SET_RESTORE:
								AE_LOGI_ALC("AE_SET_RESTORE");
		rtn = _set_restore(cxt, result);

		//ALC_S
		rtn = _ais_cmd_set_ae_LockMode(&cxt->mttAis, ALAIS_EXPOS_LOCK_MODE_OFF);
		rtn = _ais_cmd_rtn_cvt( rtn );
		//ALC_E

		break;
	case AE_SET_FORCE_PAUSE:		//	I do not use it.
								AE_LOGI_ALC("AE_SET_FORCE_PAUSE");
		rtn = _set_force_lock(cxt, 1);
		break;
	case AE_SET_FORCE_RESTORE:		//	I do not use it.
								AE_LOGI_ALC("AE_SET_FORCE_RESTORE");
		rtn = _set_force_lock(cxt, 0);
		break;

	case AE_SET_FLASH_NOTICE:
								AE_LOGI_ALC("AE_SET_FLASH_NOTICE");
		rtn = _set_flash_notice(cxt, param);
		break;

	case AE_GET_FLASH_EFFECT:
								AE_LOGI_ALC("AE_GET_FLASH_EFFECT");
		if (result) {
			uint32_t *effect = (uint32_t*)result;

			*effect = cxt->flash_param.effect;
		}
		break;
	case AE_GET_AE_STATE:
								AE_LOGI_ALC("AE_GET_AE_STATE");
		if (result) {
			uint32_t *ae_state = (uint32_t*)result;

			*ae_state = cxt->ae_state;
		}
		break;
	case AE_GET_FLASH_EB:
								AE_LOGI_ALC("AE_GET_FLASH_EB");
		if (result) {
			uint32_t *flash_eb = (uint32_t*)result;
			uint32_t cur_lum = cxt->cur_result.cur_lum;
			uint32_t low_lum_thr = cxt->cur_result.lum_low_thr;
 
			int32_t bv = 0;
			struct ae_misc_calc_out calc_out = {0};
			int32_t bv_thr = cxt->flash_param.flash_on_off_thr;

			rtn = _get_bv_by_lum(cxt, &cxt->cur_result, &bv);

			if (bv < bv_thr)
				*flash_eb = 1;
			else
				*flash_eb = 0;

			//AE_LOGV("AE_GET_FLASH_EB: flash_eb=%d, bv=%d, thr=%d", *flash_eb, bv, bv_thr);
			//AE_LOGI_ALC("AE_GET_FLASH_EB: flash_eb=%d, bv=%d, thr=%d", *flash_eb, bv, bv_thr);

			//ALC_S
			{
				#define	ALC_FLASH_BV_THR	(0.0f)

				// Flash Auto Judgment
				AE_LOGI_ALC("AE_GET_FLASH_EB: BV=%f", cxt->mttAis.mflBv);
				if( cxt->mttAis.mflBv < ALC_FLASH_BV_THR )
				{
					*flash_eb = 1;
				}
				else
				{
					*flash_eb = 0;
				}
			}
			//ALC_E
		}
		break;

	case AE_SET_FLASH_ON_OFF_THR:			//	A document does not have a mention.	I do not use it.
//								AE_LOGI_ALC("AE_SET_FLASH_ON_OFF_THR");
		if (param) {
			cxt->flash_param.flash_on_off_thr = *(int32_t *)param;
			if (0 == cxt->flash_param.flash_on_off_thr) {
				cxt->flash_param.flash_on_off_thr = AE_FLASH_ON_OFF_THR;
			}
				
			AE_LOGV("AE_SET_FLASH_ON_OFF_THR: %d", cxt->flash_param.flash_on_off_thr);
		}
		break;
	case AE_SET_TUNING_EB:					//	A document does not have a mention.	I do not use it.
//								AE_LOGI_ALC("AE_SET_TUNING_EB");
		//rtn = _set_correction_param(cxt);
		rtn = _set_frame_cnt(cxt);
		break;
	case AE_GET_BV_BY_GAIN:
								AE_LOGI_ALC("AE_GET_BV_BY_GAIN");
		if (result) {
			int32_t *bv = (int32_t*)result;

			rtn = _get_bv_by_gain(cxt, &cxt->cur_result, bv);
		}
		break;
	case AE_GET_EV:
								AE_LOGI_ALC("AE_GET_EV");
		if (result) {
			uint32_t i = 0x00;
			struct ae_ev_table *ev_table = &cxt->cur_param->ev_table;
			int32_t target_lum = cxt->cur_param->target_lum;
			struct ae_get_ev *ev_info = (struct ae_get_ev*)result;

			//ALC_S
			int32_t target_lum1 = cxt->mttAis.mflAisTrgLuma;
								AE_LOGI_ALC("AE_GET_EV (target_lum) = %d <%d>",target_lum1,target_lum);
			target_lum = (int32_t)cxt->mttAis.mflAisTrgLuma;

								AE_LOGI_ALC("AE_GET_EV : target_lum=%d, mflAisTrgLuma=%d", cxt->cur_param->target_lum, target_lum);
			//ALC_E

			ev_info->ev_index = cxt->cur_status.ev_level;

			for (i = 0; i < ev_table->diff_num; i++) {
				ev_info->ev_tab[i] = target_lum + ev_table->lum_diff[i];
				//AE_LOGE("EV_TEST: %d ,i=%d, ev=%d, target=%d, diff=%d",ev_info->ev_index, i, ev_info->ev_tab[i], target_lum, ev_table->lum_diff[i]);
			}

			rtn = AE_SUCCESS;
		}
		break;
	case AE_GET_BV_BY_LUM:
								AE_LOGI_ALC("AE_GET_BV_BY_LUM");
		if (result) {
			int32_t *bv = (int32_t*)result;

			rtn = _get_bv_by_lum(cxt, &cxt->cur_result, bv);
		}
		break;
	case AE_SET_STAT_TRIM:
								AE_LOGI_ALC("AE_SET_STAT_TRIM");
		if (param) {
			struct ae_trim *trim = (struct ae_trim*)param;

			if (cxt->stat_req.mode) {
				AE_LOGE("stat mode is not normal");
				break;
			}
			rtn = _set_scaler_trim(cxt, trim);
			
			//ALC_S
			{
				struct ae_trim	*	aptP	=	trim;
				
				AE_LOGI_ALC("AE_SET_TOUCH_ZONE x,y,w,h,W,H=%d,%d,%d,%d,%d,%d",	aptP->x, aptP->y, aptP->w, aptP->h, cxt->resolution_info.frame_size.w, cxt->resolution_info.frame_size.h );

				if( aptP->x==0 && aptP->y==0 && aptP->w==cxt->resolution_info.frame_size.w+1 && aptP->h==cxt->resolution_info.frame_size.h+1 ) {
					_ais_reset_touch_ae(	&cxt->mttAis	);
				} else {
					_ais_set_touch_ae( 
						&cxt->mttAis		,
						aptP->x				,		//	i4Left	(0 ~ + resolution_info.frame_size.w 必須)
						aptP->x+aptP->w		,		//	i4Right
						aptP->y				,		//	i4Top	(0 ~ + resolution_info.frame_size.h 必須)
						aptP->y+aptP->h		,		//	i4Bottom
						cxt->resolution_info.frame_size.w	,
						cxt->resolution_info.frame_size.h	);
				}
			}
			//ALC_E
		}
		break;
	case AE_SET_G_STAT:
								AE_LOGI_ALC("AE_SET_G_STAT");
		if (param) {
			struct ae_stat_mode  *stat_mode = (struct ae_stat_mode *)param;

			rtn = _set_g_stat(cxt, stat_mode);
		}
		break;
	case AE_GET_LUM:
								AE_LOGI_ALC("AE_GET_LUM");
		if (result) {
			uint32_t *lum = (uint32_t*)result;

			*lum = cxt->cur_result.cur_lum;

			//ALC_S
			*lum = cxt->mttAis.mflAisFrmLuma;
								AE_LOGI_ALC("AE_GET_LUM = %d",*lum);
			//ALC_E
		}
		break;
	case AE_SET_TARGET_LUM:
								AE_LOGI_ALC("AE_SET_TARGET_LUM");
		if (param) {
			uint32_t *lum = (uint32_t*)param;

			next_status->target_lum = *lum;
			cxt->update_list.is_target_lum = 1;

			//ALC_S
			{
			uint8_t		Target;

			// AE_SET_TARGET_LUM = SPRD Debug command
			AE_LOGI_ALC("[ALC_AE_TEST]CMD:AE_SET_TARGET_LUM (Target:%d)", *lum);
		
			// 128=18% Base      0=0%、255=100%
			if (*lum <= 128)
			{
				Target = (uint8_t)(*lum * 18 / 128);
			}
			else
			{
				Target = (uint8_t)(((*lum - 128) * (100 - 18) / 127) + 18 );
			}
			
			rtn = _ais_cmd_set_ae_Target(&cxt->mttAis, Target);
			rtn = _ais_cmd_rtn_cvt( rtn );
			}
			//ALC_E
		}
		break;
	case AE_SET_SNAPSHOT_NOTICE:	//		A document does not have a mention.	I do not use it.
								AE_LOGI_ALC("AE_SET_SNAPSHOT_NOTICE");
		if (param) {
			rtn = _set_snapshot_notice(cxt, param);
		}
		break;
	case AE_GET_MONITOR_INFO:
		//						AE_LOGI_ALC("AE_GET_MONITOR_INFO");
		if (result) {
			struct ae_monitor_info *info = result;

			info->win_size = cxt->monitor_unit.win_size;
			info->win_num = cxt->monitor_unit.win_num;
			info->trim = cxt->monitor_unit.trim;

			//ALC_S
				AE_LOGI_ALC("AE_GET_MONITOR_INFO : win_size.w .h=%d,%d, win_num.w .h=%d,%d, trim.x .y .w .h=%d,%d,%d,%d",
					info->win_size.w,	info->win_size.h,
					info->win_num.w,	info->win_num.h,
					info->trim.x,	info->trim.y,	info->trim.w,	info->trim.h
				);
			//ALC_E
		}
		break;
	case AE_GET_FLICKER_MODE:
		//						AE_LOGI_ALC("AE_GET_FLICKER_MODE");
		if(result) {
			uint32_t *mode = result;
			*mode = cxt->cur_status.flicker;

			//ALC_S
				AE_LOGI_ALC("AE_GET_FLICKER_MODE : mode=%d", cxt->cur_status.flicker);
			//ALC_E
		}
	case AE_SET_ONLINE_CTRL:			//		A document does not have a mention.	I do not use it.
								AE_LOGI_ALC("AE_SET_ONLINE_CTRL");
		if (param) {
			rtn = _tool_online_ctrl(cxt, param, result);
		}
		break;
	case AE_SET_EXP_GAIN:
								AE_LOGI_ALC("AE_SET_EXP_GAIN");
		cxt->ae_state = AE_STATE_LOCKED;
		_set_premain_exg(cxt);
		break;
	case AE_SET_EXP_ANIT:
								AE_LOGI_ALC("AE_SET_EXP_ANIT");
		if (param && cxt->fdae.state == FDAE_STATE_OFF) {
			rtn = _set_exp_anti(cxt, param);
		}
		break;
	case AE_SET_FD_ENABLE:
								AE_LOGI_ALC("AE_SET_FD_ENABLE");
		if (param) {
			cxt->fdae.allow_to_work = *((uint32_t *)param);
		}
		break;
	case AE_SET_FD_PARAM:
						//		AE_LOGI_ALC("AE_SET_FD_PARAM");
		if (param) {
			rtn = _fd_process(cxt, param);

			//ALC_S
			{
#if 0
struct ae_rect {
	uint32_t start_x;
	uint32_t start_y;
	uint32_t end_x;
	uint32_t end_y;
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
#endif
				struct ae_fd_param *fd = (struct ae_fd_param*)param;
	
				if (NULL == cxt || NULL == param) {
					AE_LOGE("cxt %p param %p param is error!", cxt, param);
					rtn = AE_ERROR;
				}
				else
				{
					if( cxt->mttAis.muiFaceMode != 2)	//ROI mode:(0:OFF, 1:FaceDetect-ON, 2:TouchAE-ON)
					{
						if (fd->face_num > 0) {
							uint8_t		aui0;
							
							AE_LOGI_ALC("AE_SET_FD_PARAM : face_num=%d, w=%d, h=%d", fd->face_num, fd->width, fd->height );
							
							for( aui0=0; aui0<fd->face_num; aui0++ )
							{
								_ais_set_face_detection_posi(
										&cxt->mttAis						,	//	TT_AlAisInterface*  pptAisIf	,
										aui0								,	//	UI_32				puiIdx		,		//	0 ~ 4/*ALAIS_MULTI_FD_MAX-1*/
										fd->face_area[aui0].rect.start_x	,	//	SI_32				psiLeft		,		//	i4Left	(0 ~ + resolution_info.frame_size.w 必須)
										fd->face_area[aui0].rect.start_y	,	//	SI_32				psiRight	,		//	i4Right
										fd->face_area[aui0].rect.end_x		,	//	SI_32				psiTop		,		//	i4Top	(0 ~ + resolution_info.frame_size.h 必須)
										fd->face_area[aui0].rect.end_y		,	//	SI_32				psiBottom	,		//	i4Bottom
										fd->width							,	//	SI_32				psiFrameW	,		//		resolution_info.frame_size.w
										fd->height							);	//	SI_32				psiFrameH			//		resolution_info.frame_size.h
								AE_LOGI_ALC("AE_SET_FD_PARAM : [%d] sx=%d, sy=%d, ed=%d, ey=%d, face_lum=%d, pose=%d",
										aui0								,
										fd->face_area[aui0].rect.start_x	,
										fd->face_area[aui0].rect.start_y	,
										fd->face_area[aui0].rect.end_x		,
										fd->face_area[aui0].rect.end_y		,
									fd->face_area[aui0].face_lum		,
										fd->face_area[aui0].pose			);
							}
							rtn	=	_ais_set_face_detection(&cxt->mttAis,fd->face_num);
						} else {
							AE_LOGI_ALC("AE_SET_FD_PARAM : face_num=0");
							_ais_reset_face_detection(&cxt->mttAis);
						}
					}
				}
			}
			//ALC_E
		}
		break;
	case AE_GET_GAIN:
		//						AE_LOGI_ALC("AE_GET_GAIN");
		if (result) {
			float real_gain = 0;
			real_gain = _get_real_gain(cxt->cur_result.cur_again) / 16.0;
			*(float *)result = real_gain;

			//ALC_S
				AE_LOGI_ALC("AE_GET_GAIN : cur_again=0x%08x(%d), real_gain=%f",cxt->cur_result.cur_again,cxt->cur_result.cur_again,real_gain);
			//ALC_E
		}
		break;
	case AE_GET_EXP:
		//						AE_LOGI_ALC("AE_GET_EXP");
		if (result) {
			float exposure = 0;
			exposure = (float)cxt->cur_result.cur_exposure / 10000000.0;
			AE_LOGV("exp = %d, %f", cxt->cur_result.cur_exposure, exposure);
			*(float *)result = exposure;

			//ALC_S
				AE_LOGI_ALC("AE_GET_EXP : exp = %d, %f", cxt->cur_result.cur_exposure, exposure);
			//ALC_E
		}
		break;
	case AE_GET_FLICKER_SWITCH_FLAG:
								AE_LOGI_ALC("AE_GET_FLICKER_SWITCH_FLAG");
		if (param) {
			rtn = _get_flicker_switch_flag(cxt, param);
		}
		break;

	case AE_GET_CUR_WEIGHT:			//		A document does not have a mention.	I do not use it.
								AE_LOGI_ALC("AE_GET_CUR_WEIGHT");
		if (param && cxt->fdae.state == FDAE_STATE_OFF) {
			rtn = _get_ae_weight(cxt, param, result);
		}
		break;
	case AE_GET_SKIP_FRAME_NUM:
								AE_LOGI_ALC("AE_GET_SKIP_FRAME_NUM");
		rtn = _get_skip_frame_num(cxt, param, result);
		break;
	case AE_SET_NIGHT_MODE:			//		A document does not have a mention.	I do not use it.
								AE_LOGI_ALC("AE_SET_NIGHT_MODE");
		if (param) {
			uint32_t night_mode = *(uint32_t *)param;

			next_status->night_mode = night_mode;
			cxt->update_list.is_night = 1;
		}
		break;

	case AE_GET_AIS_HANDLE:
		if (result) {
			void** ais_handle  = (void**)result;
			*ais_handle = &cxt->mttAis;
		}
		
		break;
	default:
								AE_LOGI_ALC("AE_ERROR");
		rtn = AE_ERROR;
		break;
	}

	pthread_mutex_unlock(&cxt->status_lock);

	return rtn;

	#undef	AE_LOGI_ALC
}

extern struct ae_lib_fun ae_lib_fun;
void alc_ae_fun_init()
{
	ae_lib_fun.ae_init 			= ae_alc_init;
	ae_lib_fun.ae_deinit			= ae_alc_deinit;
	ae_lib_fun.ae_calculation		= ae_alc_calculation;
	ae_lib_fun.ae_io_ctrl			= ae_alc_io_ctrl;

	return;
}
/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef	__cplusplus
}
#endif

