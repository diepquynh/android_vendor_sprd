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
#define LOG_TAG "ae_ctrl_v2"
#include "ae_sprd_ctrl.h"
#include "ae_misc.h"
#include "ae_log.h"
#include "ae_debug.h"
#include "isp_debug.h"
#ifndef WIN32
#include <utils/Timers.h>
#include <cutils/properties.h>
#include <math.h>
#include <string.h>
#else
#include "stdio.h"
#include "ae_porint.h"
#endif
#include "cmr_msg.h"
#include "sensor_exposure_queue.h"
#include "lib_ctrl.h"
#include "ae_debug_info_parser.h"

/**---------------------------------------------------------------------------*
 ** 				Compiler Flag					*
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif
/**---------------------------------------------------------------------------*
**				Macro Define					*
**----------------------------------------------------------------------------*/
#define AE_TUNING_VER 1

#define AE_START_ID 0x71717567
#define AE_END_ID 	0x69656E64
#define AE_CONVERGED_NUM 2
#define UNUSED(param)  (void)(param)
#define AE_FRAME_INFO_NUM 8


#define AE_SAVE_MLOG     "persist.sys.isp.ae.mlog" /**/
#define AE_SAVE_MLOG_DEFAULT ""

#define USE_ISO_TBL 0
#define SEGMENTED_ISO 1
#define AE_CHECKSUM_FLAG 1024
/*
 * should be read from driver later
 */
#define AE_FLASH_RATIO (3*256)
/*
 * for 30 LUX
 */
#define AE_FLASH_ON_OFF_THR 380
#define AE_FLASH_TARGET_LUM 55
#define AE_FLASH_MAX_RATIO 20
#define AE_CALC_TIMES 10
#define ISO_FIX_IN_VIDEO
#define VIDEO_IS0 5
#define CAL_CISION 256
#define QUANTIFICAT 1024

#define AE_MONITOR_WORK_MOD 1	/* 0: continus, 1:normal mode */
#define AE_ALG_ID 0
#define LCD_REC_MAX 10

#define AE_THREAD_QUEUE_NUM		(50)
#define AE_WRITE_QUEUE_NUM		(8)

#define AE_CALC_RESULT_QUEUE_LENGTH		8
	const char AE_MAGIC_TAG[] = "ae_debug_info";
/**---------------------------------------------------------------------------*
**				Data Structures					*
**---------------------------------------------------------------------------*/
struct ae_calc_result {
	int32_t expline;
	int32_t gain;
	int32_t dummy;
};

struct ae_calc_result_queue {
	struct ae_calc_result ae_result[AE_CALC_RESULT_QUEUE_LENGTH];
	struct ae_calc_result *write;
	struct ae_calc_result *read;
	uint32_t wcnt;
	uint32_t rcnt;
};

struct touch_zone_param {
	uint32_t enable;
	struct ae_weight_table weight_table;
	struct touch_zone zone_param;
};

enum initialization {
	AE_PARAM_NON_INIT,
	AE_PARAM_INIT
};

enum ae_flash_state {
	AE_FLASH_STATE_BEFORE,
	AE_FLASH_STATE_ACTIVE,
	AE_FLASH_STATE_AFTER,
	AE_LCD_STEP_UP,
	AE_LCD_STEP_END,
	AE_FLASH_STATE_MAX,
};

struct flash_param {
	uint32_t enable;
	enum ae_flash_state flash_state;
	uint32_t flash_ratio;	// high/low, x256
	struct ae_misc_calc_out before_result;
	struct ae_misc_calc_out capture_result;
	uint32_t effect;	// x1024
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
	uint32_t lcd_rec_lum[LCD_REC_MAX];
	uint32_t lcd_rec_time[LCD_REC_MAX];
	uint32_t lcd_rec_idx;
	uint32_t lcd_start_time;
	uint32_t lcd_tuning_a;
	uint32_t lcd_tuning_b;
};

struct ae_monitor_unit {
	uint32_t mode;
	struct ae_size win_num;
	struct ae_size win_size;
	struct ae_trim trim;
	struct ae_monitor_cfg cfg;
	uint32_t is_stop_monitor;
};

/**************************************************************************/
/*
 * BEGIN: FDAE related definitions 
 */
struct ae_fd_info {
	int8_t enable;
	int8_t pause;
	int8_t allow_to_work;	/* When allow_to_work==0, FDAE
				   * will never work */
	struct ae_fd_param face_info;	/* The current face information */
};
/*
 * END: FDAE related definitions 
 */
/**************************************************************************/
/*
 * ae handler for isp_app 
 */
struct ae_ctrl_cxt {
	uint32_t start_id;
	char alg_id[32];
	uint32_t checksum;
	uint32_t bypass;
	/*
	 * camera id: front camera or rear camera 
	 */
	int8_t camera_id;
	pthread_mutex_t data_sync_lock;
	/*
	 * ae control operation infaces 
	 */
	struct ae_isp_ctrl_ops isp_ops;
	/*
	 * ae stat monitor config 
	 */
	struct ae_monitor_unit monitor_unit;
	/*
	 * for ae tuning parameters 
	 */
	struct ae_tuning_param tuning_param[AE_MAX_PARAM_NUM];
	int8_t tuning_param_enable[AE_MAX_PARAM_NUM];
	struct ae_tuning_param *cur_param;
	struct ae_exp_gain_table back_scene_mode_ae_table[AE_SCENE_MAX][AE_FLICKER_NUM];
	/*
	 * sensor related information 
	 */
	struct ae_resolution_info snr_info;
	/*
	 * ae current status: include some tuning
	 * param/calculatioin result and so on 
	 */
	struct ae_alg_calc_param prv_status;/*just backup the alg status of normal scene,
									as switch from special scene mode to normal,
									and we use is to recover the algorithm status
									*/
	struct ae_alg_calc_param cur_status;
	struct ae_alg_calc_param sync_cur_status;
	uint32_t sync_aem[3 * 1024 + 4];/*0: frame id;1: exposure time, 2: dummy line, 3: gain;*/
	/*
	 * convergence & stable zone 
	 */
	int8_t stable_zone_ev[16];
	uint8_t cnvg_stride_ev_num;
	int16_t cnvg_stride_ev[32];
	/*
	 * Touch ae param 
	 */
	struct touch_zone_param touch_zone_param;
	/*
	 * flash ae param 
	 */
	int16_t mflash_wait_use;		//for non-zsl
	int16_t flash_on_off_thr;
	uint32_t flash_effect;
	/*
	 * fd-ae param 
	 */
	struct ae_fd_info fdae;
	/*
	 * lcd flash param 
	 */
	uint32_t lcd_mon_no_write;
	/*
	 * control information for sensor update 
	 */
	struct ae_alg_calc_result cur_result;
	struct ae_alg_calc_result sync_cur_result;
	/*
	 * AE write/effective E&G queue 
	 */
	void *thread_handle;
	void *seq_handle;
	uint32_t sof_id;
	int8_t exp_skip_num;
	int8_t gain_skip_num;
	uint32_t preview_skip_num;
	uint32_t capture_skip_num;
	int16_t sensor_gain_precision;
	/*
	 * AE calc E&G queue 
	 */
	struct ae_calc_result ae_result;
	struct ae_calc_result_queue ae_result_queue;
	struct seq_item sensor_calc_item;
	struct seq_cell out_write_cell;
	struct seq_cell out_actual_cell;
	/*
	 * AE effective E&G matching queue 
	 */
	struct ae_calc_result actual_cell[20];
	/*
	 * No-sof write sensor recording 
	 */
	int32_t No_sof_expline;
	int32_t No_sof_dummy;
	int32_t No_sof_aegain;
	/*
	 * recording when video stop
	 */
	int32_t last_linetime;
	int32_t last_expline;
	int32_t last_dummy;
	int32_t last_aegain;
	int32_t last_enable;
	/*
	 * just for debug information
	 */
	uint8_t debug_enable;
	char debug_file_name[256];
	uint32_t debug_info_handle;
	uint32_t debug_str[512];
	uint8_t debug_info_buf[20 * 1024];
	//struct debug_ae_param debug_buf;
	/*
	 * Isp gain control
	 */
	double isp_g_buff[3];
	int8_t isp_g_b_r;
	int8_t isp_g_b_w;
	/*
	 * flash_callback control
	 */
	int8_t send_once[3];
	/*
	 * HDR control
	 */
	int8_t		hdr_flag;
	int16_t		hdr_up;
	int16_t		hdr_down;
	int16_t		hdr_base_ae_idx;
	uint64_t	hdr_timestamp;
	/*
	 * ae misc layer handle 
	 */
	void *misc_handle;
	uint32_t end_id;
};

#define AE_PRINT_TIME \
		do {                                                       \
                        nsecs_t timestamp = systemTime(CLOCK_MONOTONIC);   \
                        AE_LOGD("timestamp = %lld.", timestamp/1000000);  \
		} while(0)

#ifndef MAX
#define  MAX( _x, _y ) ( ((_x) > (_y)) ? (_x) : (_y) )
#define  MIN( _x, _y ) ( ((_x) < (_y)) ? (_x) : (_y) )
#endif

static int32_t _get_real_gain(uint32_t gain);
static int32_t _fdae_init(struct ae_ctrl_cxt *cxt);
static int32_t ae_write_thread_proc(struct cmr_msg *message, void *data);
static int32_t ae_calc_result_queue_init(struct ae_calc_result_queue* queue);
static int32_t ae_calc_result_queue_write(struct ae_calc_result_queue *queue, struct ae_calc_result *ae_result);
static int32_t ae_calc_result_queue_read(struct ae_calc_result_queue *queue, struct ae_calc_result *ae_result);
/**---------------------------------------------------------------------------*
** 				Local Function Prototypes				*
**---------------------------------------------------------------------------*/
static int32_t isp_gain_buffer_init(struct ae_ctrl_cxt *cxt)
{
	int32_t rtn = AE_SUCCESS;
	memset(cxt->isp_g_buff, 0, sizeof(cxt->isp_g_buff));
	cxt->isp_g_b_w = 0;
	cxt->isp_g_b_r = -1 * cxt->gain_skip_num;
	return rtn;
}

static int32_t isp_gain_buffer_deinit(struct ae_ctrl_cxt *cxt)
{
	int32_t rtn = AE_SUCCESS;
	memset(cxt->isp_g_buff, 0, sizeof(cxt->isp_g_buff));
	cxt->isp_g_b_w = 0;
	cxt->isp_g_b_r = 0;
	return rtn;
}

static int32_t isp_gain_buffer_process(struct ae_ctrl_cxt *cxt, double rgb_gain_coeff)
{
	int32_t rtn = AE_SUCCESS;

	cxt->isp_g_buff[cxt->isp_g_b_w] = rgb_gain_coeff;
	cxt->isp_g_b_w = (cxt->isp_g_b_w + 1) % (cxt->gain_skip_num + 1);

	if (0 <= cxt->isp_g_b_r){
		//AE_LOGD("D-gain--isp_gain: W %lf R %lf\r\n", rgb_gain_coeff, cxt->isp_g_buff[cxt->isp_g_b_r]);
		cxt->isp_ops.set_rgb_gain(cxt->isp_ops.isp_handler, cxt->isp_g_buff[cxt->isp_g_b_r]);
		cxt->isp_g_b_r = (cxt->isp_g_b_r + 1) % (cxt->gain_skip_num + 1);
	}else{
		cxt->isp_g_b_r++;
	}
	return rtn;
}

static int32_t ae_info_print(struct ae_ctrl_cxt *cxt)
{
	int32_t rtn = AE_SUCCESS;
	AE_LOGD("dydl_camera_id %d\r\n", cxt->camera_id);

	AE_LOGD("dydl_alg_id %d   frame_id %d\r\n", cxt->cur_status.alg_id, cxt->cur_status.frame_id);

	AE_LOGD("dydl_stidx %d   linetime %d\r\n", cxt->cur_status.start_index, cxt->cur_status.line_time);

	AE_LOGD("dydl_mxidx %d   mnidx %d\r\n", cxt->cur_status.ae_table->max_index, cxt->cur_status.ae_table->min_index);

	AE_LOGD("dydl_target %d   target_zone %d\r\n", cxt->cur_status.target_lum, cxt->cur_status.target_lum_zone);

	AE_LOGD("dydl_flicker %d   ISO %d\r\n", cxt->cur_status.settings.flicker, cxt->cur_status.settings.iso);

	AE_LOGD("dydl_wmode %d   smode %d\r\n", cxt->cur_status.settings.work_mode, cxt->cur_status.settings.scene_mode);

	AE_LOGD("dydl_metering mode %d\r\n", cxt->cur_status.settings.metering_mode);
	return rtn;
}

static int32_t _ae_write_exp_gain(struct ae_ctrl_cxt *cxt, int32_t expline, int32_t dummy, int32_t aegain)
{
	int32_t rtn = AE_SUCCESS;
	if (NULL == cxt) {
		rtn = AE_PARAM_NULL;
		AE_RETURN_IF_FAIL(rtn, ("cxt:%p", cxt));
	}

	//AE_LOGD("expline %d dummy %d aegain %d", expline, dummy, aegain);
	if (0 <= expline) {
		struct ae_exposure exp;
		int32_t size_index = cxt->snr_info.sensor_size_index;
		if (cxt->isp_ops.ex_set_exposure) {
			memset(&exp, 0, sizeof(exp));
			exp.exposure = expline;
			exp.dummy = dummy;
			exp.size_index = size_index;

			if (cxt->No_sof_expline != expline || cxt->No_sof_dummy != dummy){
				(*cxt->isp_ops.ex_set_exposure) (cxt->isp_ops.isp_handler, &exp);
			}else{
				//AE_LOGD("no_need_write exp");
				;
			}
		} else if (cxt->isp_ops.set_exposure) {
			uint32_t ae_expline = expline;

			memset(&exp, 0, sizeof(exp));
			ae_expline = ae_expline & 0x0000ffff;
			ae_expline |= (dummy << 0x10) & 0x0fff0000;
			ae_expline |= (size_index << 0x1c) & 0xf0000000;
			exp.exposure = ae_expline;

			if (cxt->No_sof_expline != expline || cxt->No_sof_dummy != dummy){
				(*cxt->isp_ops.set_exposure) (cxt->isp_ops.isp_handler, &exp);
			}else{
				//AE_LOGD("no_need_write exp");
				;
			}
		}
	} else {
		AE_LOGE("error --expline %d", expline);
	}

	if (0 <= aegain) {
		struct ae_gain gain;

		if (cxt->isp_ops.set_again) {
			memset(&gain, 0, sizeof(gain));
			gain.gain = aegain & 0xffff;

			if (cxt->No_sof_aegain != aegain){
				(*cxt->isp_ops.set_again) (cxt->isp_ops.isp_handler, &gain);
			}else{
				//AE_LOGD("no_need_write gain");
				;
			}
		}
	} else {
		AE_LOGE("error --aegain %d", aegain);
	}

	cxt->No_sof_expline = expline;
	cxt->No_sof_dummy = dummy;
	cxt->No_sof_aegain = aegain;
	return rtn;
}

static int32_t ae_calc_result_queue_init(struct ae_calc_result_queue *queue)
{
	int32_t rtn = AE_SUCCESS;

	if (NULL == queue) {
		AE_LOGE("ae_calc_result_queue_init, queue=%p param is error", queue);
		return AE_ERROR;
	}
	AE_LOGI("ae_calc_result_queue_init");

	memset(queue, 0, sizeof(*queue));
	queue->write = &queue->ae_result[0];
	queue->read = &queue->ae_result[0];
	queue->wcnt = 0;
	queue->rcnt = 0;

	return rtn;
}

static int32_t ae_calc_result_queue_write(struct ae_calc_result_queue *queue, struct ae_calc_result *ae_result)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_calc_result *ori_ae_result = NULL;

	if (NULL == queue || NULL == ae_result || NULL == queue->read || NULL == queue->write) {
		AE_LOGE("ae_calc_result_queue_write, queue=%p,  ae_result=%p, read=%p, write=%p param is error", queue, ae_result, queue->read, queue->write);
		return AE_ERROR;
	}
	AE_LOGI("ae_calc_result_queue_write");

	ori_ae_result = queue->write;
	*queue->write++ = *ae_result;
	queue->wcnt++;
	AE_LOGI("ae_calc_result_queue_write -- queue->write %p", queue->write);
	AE_LOGI("ae_calc_result_queue_write -- queue->wcnt %u", queue->wcnt);
	if (queue->write > &queue->ae_result[AE_CALC_RESULT_QUEUE_LENGTH - 1]) {
		queue->write = &queue->ae_result[0];
	}

	if (queue->write == queue->read) {
		queue->read = ori_ae_result;
		AE_LOGI("queue is full, ae_calc_result_queue_write-----queue->write == queue->read");
	}

	return rtn;
}

static int32_t ae_calc_result_queue_read(struct ae_calc_result_queue *queue, struct ae_calc_result *ae_result)
{
	int32_t rtn = AE_SUCCESS;

	if (NULL == queue || NULL == ae_result || NULL == queue->read || NULL == queue->write) {
		AE_LOGE("ae_calc_result_queue_read, queue=%p,  ae_result=%p, read=%p, write=%p param is error",\
					queue, ae_result, queue->read, queue->write);
		return AE_ERROR;
	}
	//AE_LOGD("ae_calc_result_queue_read");

	if (queue->read != queue->write) {
		*ae_result = *queue->read++;
		queue->rcnt++;
		//AE_LOGD("ae_calc_result_queue_read -- queue->read %p", queue->read);
		//AE_LOGD("ae_calc_result_queue_read -- queue->rcnt %u", queue->rcnt);
		if (queue->read > &queue->ae_result[AE_CALC_RESULT_QUEUE_LENGTH - 1]) {
			queue->read = &queue->ae_result[0];
		}
	} else {
		//AE_LOGD("ae_calc_result_queue_read-----queue->write == queue->read");
	}

	return rtn;
}

static int32_t ae_write_thread_proc(struct cmr_msg *message, void *data)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_ctrl_cxt *cxt = (struct ae_ctrl_cxt *)data;
	uint32_t evt = (uint32_t) (message->msg_type);
	uint32_t max_again;
	double rgb_gain_coeff;
	int32_t expline;
	int32_t dummy;
	int32_t again;
	/*
	 * int pipe_fd = 0; pid_t tid = 0; if(
	 * message->first_in){ pipe_fd =
	 * open("/dev/isp_pipe/pipe_1", O_RDWR); if(pipe_fd < 0) {
	 * AE_LOGE("open pipe error!!"); }else{ tid = gettid();
	 * write(pipe_fd,&tid,sizeof(pid_t)); close(pipe_fd); } } 
	 */
	if (NULL == cxt) {
		AE_LOGE("cxt is NULL error!");
		return AE_ERROR;
	}

	switch (evt) {
		case AE_WRITE_EXP_GAIN:
			//AE_LOGD("D-gain--work_mode =%d", cxt->cur_status.settings.work_mode);
			cxt->sensor_calc_item.work_mode = SEQ_WORK_PREVIEW;	// cxt->cur_status.real_work_mode;//hu
			cxt->sensor_calc_item.cell.frame_id = cxt->sof_id;
			// pthread_mutex_unlock(&cxt->status_lock);
			seq_put(cxt->seq_handle, &cxt->sensor_calc_item, &cxt->out_actual_cell, &cxt->out_write_cell);

			//AE_LOGD("D-gain--AE lib---,%d,%d,%d,\n", cxt->sensor_calc_item.cell.frame_id, cxt->sensor_calc_item.cell.exp_line, cxt->sensor_calc_item.cell.gain);
			//AE_LOGD("D-gain--to_sensor,%d,%d,%d,\n", cxt->out_write_cell.frame_id, cxt->out_write_cell.exp_line, cxt->out_write_cell.gain);
			//AE_LOGD("D-gain--valid_out,%d,%d,%d,\n\n", cxt->out_actual_cell.frame_id, cxt->out_actual_cell.exp_line, cxt->out_actual_cell.gain);
			// maintain for matching E&G
			cxt->actual_cell[cxt->sof_id % 20].expline = cxt->out_actual_cell.exp_line;
			cxt->actual_cell[cxt->sof_id % 20].gain = cxt->out_actual_cell.gain;
			// maintain for matching E&G 

			expline = cxt->out_write_cell.exp_line;
			dummy = cxt->out_write_cell.dummy;
			max_again = cxt->cur_status.max_gain;	// 2048//cxt->sensor_max_gain;//hu
			//AE_LOGD("D-gain--setting--- %d %d %d\n", cxt->sensor_gain_precision, cxt->exp_skip_num, cxt->gain_skip_num);

			if (cxt->out_write_cell.gain <= max_again) {
				if (0 == cxt->out_write_cell.gain % cxt->sensor_gain_precision) {
					again = cxt->out_write_cell.gain;
					rgb_gain_coeff = 1;
				} else {
					again = (cxt->out_write_cell.gain / cxt->sensor_gain_precision) * cxt->sensor_gain_precision;
					rgb_gain_coeff = (double)cxt->out_write_cell.gain / (double)again;
				}
			} else {
				again = max_again;
				rgb_gain_coeff = (double)cxt->out_write_cell.gain / (double)max_again;
			}

			rtn = isp_gain_buffer_process(cxt, rgb_gain_coeff);
			rtn = _ae_write_exp_gain(cxt, expline, dummy, again);
			break;

		case AE_WRITE_RECORD:
			seq_put(cxt->seq_handle, &cxt->sensor_calc_item, &cxt->out_actual_cell, &cxt->out_write_cell);
			cxt->actual_cell[cxt->sof_id % 20].expline = cxt->sensor_calc_item.cell.exp_line;
			cxt->actual_cell[cxt->sof_id % 20].gain = cxt->sensor_calc_item.cell.gain;
			break;

		default:
			break;
	}
	return rtn;
}

static int32_t _is_ae_mlog(struct ae_ctrl_cxt *cxt)
{
	uint32_t ret = 0;
	int32_t len = 0;
	int32_t is_save = 0;
#ifndef WIN32
	char value[PROPERTY_VALUE_MAX];
	len = property_get(AE_SAVE_MLOG, value, AE_SAVE_MLOG_DEFAULT);
	if (len) {
		memcpy((void*)&cxt->debug_file_name[0], &value[0], len);
		cxt->debug_info_handle = (uint32_t)debug_file_init(cxt->debug_file_name, "w+t");
		if (cxt->debug_info_handle) {
			ret = debug_file_open((debug_handle_t) cxt->debug_info_handle, 1, 0);
			if (0 == ret) {
				is_save = 1;
				debug_file_close((debug_handle_t)cxt->debug_info_handle);
			}
		}
	}
#endif
	return is_save;
}

static void _print_ae_debug_info(char* log_str, struct ae_ctrl_cxt *cxt_ptr)
{
	float fps = 0.0;
	uint32_t pos = 0;
	struct ae_alg_calc_result *result_ptr;
	struct ae_alg_calc_param *sync_cur_status_ptr;

	sync_cur_status_ptr = &(cxt_ptr->sync_cur_status);
	result_ptr = &cxt_ptr->sync_cur_result;

	fps = 10000000.0
			/ (cxt_ptr->snr_info.line_time * (result_ptr->wts.cur_dummy + result_ptr->wts.cur_exp_line));
	if (fps > cxt_ptr->sync_cur_status.settings.max_fps) {
		fps = cxt_ptr->sync_cur_status.settings.max_fps;
	}

	pos = sprintf(log_str, "cam-id:%d frm-id:%d,flicker: %d\nidx(%d-%d):%d,cur-l:%d, tar-l:%d, lv:%d, bv: %d,expl(%d):%d, expt: %d, gain:%d, dmy:%d, FR(%d-%d):%.2f\n",
			cxt_ptr->camera_id, sync_cur_status_ptr->frame_id,sync_cur_status_ptr->settings.flicker, sync_cur_status_ptr->ae_table->min_index, sync_cur_status_ptr->ae_table->max_index,result_ptr->wts.cur_index,\
			cxt_ptr->sync_cur_result.cur_lum, cxt_ptr->sync_cur_result.target_lum, cxt_ptr->cur_result.cur_bv, cxt_ptr->cur_result.cur_bv_nonmatch, cxt_ptr->snr_info.line_time,\
			result_ptr->wts.cur_exp_line, result_ptr->wts.cur_exp_line * cxt_ptr->snr_info.line_time, result_ptr->wts.cur_again, result_ptr->wts.cur_dummy,\
			cxt_ptr->sync_cur_status.settings.min_fps,cxt_ptr->sync_cur_status.settings.max_fps,fps);

	if (result_ptr->log) {
		pos += sprintf((char*)((char*)log_str + pos), "adv info:\n%s\n", result_ptr->log);
	}
}

static int32_t save_to_mlog_file(struct ae_ctrl_cxt *cxt, struct ae_misc_calc_out *result)
{
	int32_t rtn = 0;
	char *tmp_str = (char *)cxt->debug_str;
	
	memset(tmp_str, 0, sizeof(cxt->debug_str));
	rtn = debug_file_open((debug_handle_t) cxt->debug_info_handle, 1, 0);
	if (0 == rtn) {
		_print_ae_debug_info(tmp_str, cxt);
		debug_file_print((debug_handle_t) cxt->debug_info_handle, tmp_str);
		debug_file_close((debug_handle_t) cxt->debug_info_handle);
	}

	return rtn;
}

static uint32_t _get_checksum(void)
{
	uint32_t checksum = 0;
	checksum = (sizeof(struct ae_ctrl_cxt)) % AE_CHECKSUM_FLAG;

	return checksum;
}

static int32_t _check_handle(void *handle)
{
	struct ae_ctrl_cxt *cxt = (struct ae_ctrl_cxt *)handle;
	uint32_t checksum = 0;
	if (NULL == handle) {
		AE_LOGE("handle is NULL error!");
		return AE_ERROR;
	}
	checksum = _get_checksum();
	if ((AE_START_ID != cxt->start_id) || (AE_END_ID != cxt->end_id) || (checksum != cxt->checksum)) {
		AE_LOGE("handle is error, start_id:%d, end_id:%d, check sum:%d\n", cxt->start_id, cxt->end_id, cxt->checksum);
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

	//AE_LOGD("cur target lum=%d, ev diff=%d, level=%d", cur_target_lum, ev_table->lum_diff[level], level);

	target_lum = (int32_t) cur_target_lum + ev_table->lum_diff[level];
	target_lum = (target_lum < 0) ? 0 : target_lum;

	return (uint32_t) target_lum;
}

static int32_t _update_monitor_unit(struct ae_ctrl_cxt *cxt, struct ae_trim *trim)
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

int32_t iso_shutter_mapping[7][15] = {
	// 50 ,64 ,80 ,100 
	// ,125
	// ,160 ,200 ,250 ,320
	// ,400 ,500 ,640 ,800
	// ,1000,1250
	{128, 170, 200, 230, 270, 300, 370, 490, 600, 800, 950, 1210, 1500, 1900, 2300},// 1/17
	{128, 170, 200, 230, 240, 310, 370, 450, 600, 800, 950, 1210, 1500, 1900, 2300},// 1/20
	{128, 170, 200, 230, 240, 330, 370, 450, 600, 800, 950, 1210, 1500, 1900, 2300},// 1/25
	{128, 170, 183, 228, 260, 320, 370, 450, 600, 800, 950, 1210, 1500, 1900, 2300},// 1/33
	{128, 162, 200, 228, 254, 320, 370, 450, 600, 800, 950, 1210, 1500, 1900, 2300},// 1/50
	{128, 162, 207, 255, 254, 320, 370, 450, 600, 800, 950, 1210, 1500, 1900, 2300},// 1/100
	{128, 190, 207, 245, 254, 320, 370, 450, 600, 800, 950, 1210, 1500, 1900, 2300}
};

#if 0
static int32_t _get_iso(struct ae_ctrl_cxt *cxt, int32_t * real_iso)
{
	int32_t rtn = AE_SUCCESS;
	int32_t iso = 0;
	int32_t calc_iso = 0;
	int32_t tmp_iso = 0;
	int32_t real_gain = 0;
	int32_t iso_shutter = 0;
	float f_tmp = 0;

	int32_t(*map)[15] = 0;

	if (NULL == cxt || NULL == real_iso) {
		AE_LOGE("cxt %p real_iso %p param is error!", cxt, real_iso);
		return AE_ERROR;
	}

	iso = cxt->cur_status.settings.iso;
	real_gain = cxt->cur_result.wts.cur_again;
	f_tmp = 10000000.0 / cxt->cur_result.wts.exposure_time;

	if (0.5 <= (f_tmp - (int32_t) f_tmp))
		iso_shutter = (int32_t) f_tmp + 1;
	else
		iso_shutter = (int32_t) f_tmp;
#if 0
	AE_LOGD("iso_check %d", cxt->cur_status.frame_id % 20);
	AE_LOGD("iso_check %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", cxt->actual_cell[0].gain, cxt->actual_cell[1].gain, cxt->actual_cell[2].gain, cxt->actual_cell[3].gain, cxt->actual_cell[4].gain, cxt->actual_cell[5].gain, cxt->actual_cell[6].gain, cxt->actual_cell[7].gain, cxt->actual_cell[8].gain, cxt->actual_cell[9].gain, cxt->actual_cell[10].gain, cxt->actual_cell[11].gain, cxt->actual_cell[12].gain, cxt->actual_cell[13].gain, cxt->actual_cell[14].gain, cxt->actual_cell[15].gain, cxt->actual_cell[16].gain, cxt->actual_cell[17].gain, cxt->actual_cell[18].gain, cxt->actual_cell[19].gain);
#endif
	if (AE_ISO_AUTO == iso) {
		switch (iso_shutter) {
		case 17:
			map = iso_shutter_mapping[0];
			break;
		case 20:
			map = iso_shutter_mapping[1];
			break;
		case 25:
			map = iso_shutter_mapping[2];
			break;
		case 33:
			map = iso_shutter_mapping[3];
			break;
		case 50:
			map = iso_shutter_mapping[4];
			break;
		case 100:
			map = iso_shutter_mapping[5];
			break;
		default:
			map = iso_shutter_mapping[6];
			break;
		}

		if (real_gain > *(*map + 14)) {
			calc_iso = 1250;
		} else if (real_gain > *(*map + 13)) {
			calc_iso = 1000;
		} else if (real_gain > *(*map + 12)) {
			calc_iso = 800;
		} else if (real_gain > *(*map + 11)) {
			calc_iso = 640;
		} else if (real_gain > *(*map + 10)) {
			calc_iso = 500;
		} else if (real_gain > *(*map + 9)) {
			calc_iso = 400;
		} else if (real_gain > *(*map + 8)) {
			calc_iso = 320;
		} else if (real_gain > *(*map + 7)) {
			calc_iso = 250;
		} else if (real_gain > *(*map + 6)) {
			calc_iso = 200;
		} else if (real_gain > *(*map + 5)) {
			calc_iso = 160;
		} else if (real_gain > *(*map + 4)) {
			calc_iso = 125;
		} else if (real_gain > *(*map + 3)) {
			calc_iso = 100;
		} else if (real_gain > *(*map + 2)) {
			calc_iso = 80;
		} else if (real_gain > *(*map + 1)) {
			calc_iso = 64;
		} else if (real_gain > *(*map)) {
			calc_iso = 50;
		} else {
			calc_iso = 50;
		}

	} else {
		calc_iso = (1 << (iso - 1)) * 100;
	}

	*real_iso = calc_iso;
	// AE_LOGD("calc_iso iso_shutter %d\r\n", iso_shutter);
	// AE_LOGD("calc_iso %d,real_gain %d,iso %d", calc_iso,
	// real_gain, iso);
	return rtn;
}
#else
static int32_t _get_iso(struct ae_ctrl_cxt *cxt, uint32_t * real_iso)
{
	int32_t rtn = AE_SUCCESS;
	int32_t iso = 0;
	int32_t calc_iso = 0;
	float real_gain = 0;
	float tmp_iso = 0;

	if (NULL == cxt || NULL == real_iso) {
		AE_LOGE("cxt %p real_iso %p param is error!", cxt, real_iso);
		return AE_ERROR;
	}

	iso = cxt->cur_status.settings.iso;
	real_gain = cxt->cur_result.wts.cur_again;

	if (AE_ISO_AUTO == iso) {
		tmp_iso = real_gain * 5000 / 128;
		calc_iso = 0;
		if (tmp_iso < 890) {
			calc_iso = 0;
		} else if (tmp_iso < 1122) {
			calc_iso = 10;
		} else if (tmp_iso < 1414) {
			calc_iso = 12;
		} else if (tmp_iso < 1782) {
			calc_iso = 16;
		} else if (tmp_iso < 2245) {
			calc_iso = 20;
		} else if (tmp_iso < 2828) {
			calc_iso = 25;
		} else if (tmp_iso < 3564) {
			calc_iso = 32;
		} else if (tmp_iso < 4490) {
			calc_iso = 40;
		} else if (tmp_iso < 5657) {
			calc_iso = 50;
		} else if (tmp_iso < 7127) {
			calc_iso = 64;
		} else if (tmp_iso < 8909) {
			calc_iso = 80;
		} else if (tmp_iso < 11220) {
			calc_iso = 100;
		} else if (tmp_iso < 14140) {
			calc_iso = 125;
		} else if (tmp_iso < 17820) {
			calc_iso = 160;
		} else if (tmp_iso < 22450) {
			calc_iso = 200;
		} else if (tmp_iso < 28280) {
			calc_iso = 250;
		} else if (tmp_iso < 35640) {
			calc_iso = 320;
		} else if (tmp_iso < 44900) {
			calc_iso = 400;
		} else if (tmp_iso < 56570) {
			calc_iso = 500;
		} else if (tmp_iso < 71270) {
			calc_iso = 640;
		} else if (tmp_iso < 89090) {
			calc_iso = 800;
		} else if (tmp_iso < 112200) {
			calc_iso = 1000;
		} else if (tmp_iso < 141400) {
			calc_iso = 1250;
		} else if (tmp_iso < 178200) {
			calc_iso = 1600;
		} else if (tmp_iso < 224500) {
			calc_iso = 2000;
		} else if (tmp_iso < 282800) {
			calc_iso = 2500;
		} else if (tmp_iso < 356400) {
			calc_iso = 3200;
		} else if (tmp_iso < 449000) {
			calc_iso = 4000;
		} else if (tmp_iso < 565700) {
			calc_iso = 5000;
		} else if (tmp_iso < 712700) {
			calc_iso = 6400;
		} else if (tmp_iso < 890900) {
			calc_iso = 8000;
		} else if (tmp_iso < 1122000) {
			calc_iso = 10000;
		} else if (tmp_iso < 1414000) {
			calc_iso = 12500;
		} else if (tmp_iso < 1782000) {
			calc_iso = 16000;
		}
	} else {
		calc_iso = (1 << (iso - 1)) * 100;
	}

	*real_iso = calc_iso;
	//AE_LOGD("calc_iso=%d,real_gain=%f,iso=%d", calc_iso, real_gain, iso);
	return rtn;
}
#endif

static void convert_trim_to_rect(struct ae_trim *trim, struct ae_rect *rect)
{
	rect->start_x = trim->x;
	rect->start_y = trim->y;
	rect->end_x = rect->start_x + trim->w - 1;
	rect->end_y = rect->start_y + trim->h - 1;

	if ((int32_t) (rect->end_x) < 0)
		rect->end_x = 0;
	if ((int32_t) (rect->end_y) < 0)
		rect->end_y = 0;
}

static void increase_rect_offset(struct ae_rect *rect, int32_t x_offset, int32_t y_offset, struct ae_trim *trim)
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

	if (end_x + x_offset >= (int32_t) trim->w)
		end_x = (int32_t) (trim->w - 1);
	else
		end_x += x_offset;
	rect->end_x = end_x;

	if (start_y - y_offset < 0)
		rect->start_y = 0;
	else
		start_y -= y_offset;
	rect->start_y = start_y;

	if (end_y + y_offset >= (int32_t) trim->h)
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

	if (rect->start_x <= x && rect->start_y <= y && x <= rect->end_x && y <= rect->end_y) {
		is_in = 1;
	}

	return is_in;
}
static int32_t _set_touch_zone(struct ae_ctrl_cxt *cxt, struct ae_trim *touch_zone)
{
#if 1
	return AE_SUCCESS;
#else
	int32_t rtn = AE_SUCCESS;
	struct ae_trim level_0_trim;

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
	struct ae_rect level_1_rect;
	struct ae_rect level_2_rect;
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
	level_1_x_offset = (cxt->touch_zone_param.zone_param.level_1_percent * level_0_trim.w) >> (6 + 1);
	level_1_y_offset = (cxt->touch_zone_param.zone_param.level_1_percent * level_0_trim.h) >> (6 + 1);

	level_2_x_offset = (cxt->touch_zone_param.zone_param.level_2_percent * level_0_trim.w) >> (6 + 1);
	level_2_y_offset = (cxt->touch_zone_param.zone_param.level_2_percent * level_0_trim.h) >> (6 + 1);

	// level 1 rect
	level_1_rect = level_0_rect;
	increase_rect_offset(&level_1_rect, level_1_x_offset, level_1_y_offset, &map_trim);
	// level 2 rect
	level_2_rect = level_1_rect;
	increase_rect_offset(&level_2_rect, level_2_x_offset, level_2_y_offset, &map_trim);

	level_0_weight = cxt->touch_zone_param.zone_param.level_0_weight;
	level_1_weight = cxt->touch_zone_param.zone_param.level_1_weight;
	level_2_weight = cxt->touch_zone_param.zone_param.level_2_weight;

	tab_ptr = cxt->touch_zone_param.weight_table.weight;
	for (j = 0; j < new_h; ++j) {
		for (i = 0; i < new_w; ++i) {
			weight_val = 0;

			// level 2 rect, big
			if (is_in_rect(i, j, &level_2_rect))
				weight_val = level_2_weight;
			// level 1 rect, middle
			if (is_in_rect(i, j, &level_1_rect))
				weight_val = level_1_weight;
			// level 0 rect, small
			if (is_in_rect(i, j, &level_0_rect))
				weight_val = level_0_weight;

			tab_ptr[i + new_w * j] = weight_val;
		}
	}

	{
		struct ae_weight_table *weight_tab;

		weight_tab = &cxt->touch_zone_param.weight_table;
		rtn = ae_misc_io_ctrl(cxt->misc_handle, AE_MISC_CMD_SET_WEIGHT_TABLE, (void *)weight_tab, NULL);
		if (AE_SUCCESS == rtn) {
			cxt->cur_status.touch_zone = *touch_zone;
		}
	}
	cxt->touch_zone_param.enable = 1;

	return rtn;
#endif
}

static int32_t _get_bv_by_lum_new(struct ae_ctrl_cxt *cxt, int32_t * bv)
{
	int32_t rtn = AE_SUCCESS;

	if (NULL == cxt || NULL == bv) {
		AE_LOGE("cxt %p bv %p param is error!", cxt, bv);
		return AE_ERROR;
	}

	if (0 == cxt->cur_param->alg_id) {
		uint32_t cur_lum = 0;
		uint32_t real_gain = 0, cur_exp = 0;
		*bv = 0;
		cur_lum = cxt->cur_result.cur_lum;
		cur_exp = cxt->cur_result.wts.exposure_time;
		real_gain = _get_real_gain(cxt->cur_result.wts.cur_again);
		if (0 == cur_lum) {
			cur_lum = 1;
		}
		*bv = (int32_t)(log2(((double)100000000 * cur_lum) / ((double)cur_exp * real_gain * 5)) * 16.0 + 0.5);
	} else if (2 == cxt->cur_param->alg_id){
		*bv = cxt->sync_cur_result.cur_bv;
	}

	//AE_LOGD("real bv %d", *bv);
	return rtn;
}

static int32_t _get_bv_by_lum(struct ae_ctrl_cxt *cxt, int32_t * bv)
{
	int32_t rtn = AE_SUCCESS;
	uint32_t cur_lum = 0;
	uint32_t real_gain = 0, cur_exp = 0;

	if (NULL == cxt || NULL == bv) {
		AE_LOGE("cxt %p bv %p param is error!", cxt, bv);
		return AE_ERROR;
	}

	*bv = 0;
	cur_lum = cxt->cur_result.cur_lum;
	cur_exp = cxt->cur_result.wts.exposure_time;
	real_gain = _get_real_gain(cxt->cur_result.wts.cur_again);

	if (0 == cur_lum)
		cur_lum = 1;
#ifdef AE_TABLE_32
	*bv = (int32_t) (log2(((double)100000000 * cur_lum) / ((double)cur_exp * real_gain * 5)) * 16.0 + 0.5);
#else
	*bv = (int32_t) (log2((double)(cur_exp * cur_lum) / (double)(real_gain * 5)) * 16.0 + 0.5);
#endif

	//AE_LOGD("real bv=%d", *bv);

	return rtn;
}

static int32_t _get_real_gain(uint32_t gain)
{	// x=real_gain/16
	uint32_t real_gain = 0;
	uint32_t cur_gain = gain;
	uint32_t i = 0;

#ifdef AE_TABLE_32
	real_gain = gain >> 3;	// / 128 * 16;
#else
	real_gain = ((gain & 0x0f) + 16);
	cur_gain = (cur_gain >> 4);

	for (i = 0x00; i < 7; ++i) {
		if (0x01 == (cur_gain & 0x01)) {
			real_gain *= 0x02;
		}
		cur_gain >>= 0x01;
	}
#endif

	return real_gain;
}

static int32_t _get_bv_by_gain(struct ae_ctrl_cxt *cxt, int16_t * gain, int32_t * bv)
{
	int32_t rtn = AE_SUCCESS;
	uint32_t cur_gain = 0;

	if (NULL == cxt || NULL == gain || NULL == bv) {
		AE_LOGE("cxt %p cur_result %p bv %p param is error!", cxt, gain, bv);
		return AE_ERROR;
	}
	cur_gain = *gain;
	*bv = _get_real_gain(cur_gain);

	AE_LOGV("bv=%d", *bv);

	return rtn;
}

static int32_t _cfg_monitor_win(struct ae_ctrl_cxt *cxt)
{
	int32_t rtn = AE_SUCCESS;

	if (cxt->isp_ops.set_monitor_win) {
		struct ae_monitor_info info;

		info.win_size = cxt->monitor_unit.win_size;
		info.trim = cxt->monitor_unit.trim;

		//AE_LOGD("info x=%d,y=%d,w=%d,h=%d, block_size.w=%d,block_size.h=%d", info.trim.x, info.trim.y, info.trim.w, info.trim.h, info.win_size.w, info.win_size.h);
		rtn = cxt->isp_ops.set_monitor_win(cxt->isp_ops.isp_handler, &info);
	}

	return rtn;
}

static int32_t _cfg_monitor_bypass(struct ae_ctrl_cxt *cxt)
{
	int32_t rtn = AE_SUCCESS;

	if (cxt->isp_ops.set_monitor_bypass) {
		uint32_t is_bypass = 0;

		is_bypass = cxt->monitor_unit.is_stop_monitor;
		// AE_LOGI("AE_TEST: is_bypass=%d", is_bypass);
		rtn = cxt->isp_ops.set_monitor_bypass(cxt->isp_ops.isp_handler, is_bypass);
	}

	return rtn;
}

static int32_t _cfg_set_aem_mode(struct ae_ctrl_cxt *cxt)
{
	int32_t ret = AE_SUCCESS;

	if (cxt->isp_ops.set_statistics_mode) {
		cxt->isp_ops.set_statistics_mode(cxt->isp_ops.isp_handler, cxt->monitor_unit.mode, cxt->monitor_unit.cfg.skip_num);
	} else {
		AE_LOGE("set_statistics_mode function is NULL\n");
		ret = AE_ERROR;
	}

	return ret;
}

static int32_t _set_g_stat(struct ae_ctrl_cxt *cxt, struct ae_stat_mode *stat_mode)
{
	int32_t rtn = AE_SUCCESS;

	if (NULL == cxt || NULL == stat_mode) {
		AE_LOGE("cxt %p stat_mode %p param is error!", cxt, stat_mode);
		return AE_ERROR;
	}

	rtn = _update_monitor_unit(cxt, &stat_mode->trim);
	if (AE_SUCCESS != rtn) {
		goto EXIT;
	}

	rtn = _cfg_monitor_win(cxt);
EXIT:
	return rtn;
}

static int32_t _set_scaler_trim(struct ae_ctrl_cxt *cxt, struct ae_trim *trim)
{
	int32_t rtn = AE_SUCCESS;

	rtn = _update_monitor_unit(cxt, trim);

	if (AE_SUCCESS != rtn) {
		goto EXIT;
	}

	rtn = _cfg_monitor_win(cxt);
EXIT:
	return rtn;
}

static int32_t _set_snapshot_notice(struct ae_ctrl_cxt *cxt, struct ae_snapshot_notice *notice)
{
	int32_t rtn = AE_SUCCESS;

	return rtn;
}

static uint32_t get_cur_timestamp(struct ae_ctrl_cxt *cxt)
{
	int32_t rtn = AE_SUCCESS;
	uint32_t sec = 0, usec = 0;
	uint32_t timestamp = 0;
	uint64_t calc_sec = 0;

	if (cxt->isp_ops.get_system_time) {
		rtn = (*cxt->isp_ops.get_system_time) (cxt->isp_ops.isp_handler, &sec, &usec);
		calc_sec = sec;
		timestamp = (uint32_t) (calc_sec * 1000000 + usec) / 1000;
	}

	return timestamp;
}

static int32_t _set_flash_notice(struct ae_ctrl_cxt *cxt, struct ae_flash_notice *flash_notice)
{
	int32_t rtn = AE_SUCCESS;
	enum ae_flash_mode mode = 0;

	if ((NULL == cxt) || (NULL == flash_notice)) {
		AE_LOGE("cxt %p flash_notice %p param is error!", cxt, flash_notice);
		return AE_PARAM_NULL;
	}

	mode = flash_notice->mode;
	// AE_LOGD("ae_flash_status %d\r\n", mode);
	switch (mode) {
		case AE_FLASH_PRE_BEFORE:
			AE_LOGD("ae_flash_status FLASH_PRE_BEFORE");
			cxt->cur_status.settings.flash = FLASH_PRE_BEFORE;
			break;

		case AE_FLASH_PRE_LIGHTING:
			{
				cxt->cur_status.settings.flash = FLASH_PRE;
				cxt->cur_status.settings.flash_target = cxt->cur_param->flash_param.target_lum;
				cxt->mflash_wait_use = 1;
			}
			break;

		case AE_FLASH_PRE_AFTER:
			AE_LOGD("ae_flash_status FLASH_PRE_AFTER");
			cxt->cur_status.settings.flash = FLASH_PRE_AFTER;
			break;

		case AE_FLASH_MAIN_AFTER:
			AE_LOGD("ae_flash_status FLASH_MAIN_AFTER");
			cxt->cur_status.settings.flash = FLASH_MAIN_AFTER;
			if (cxt->camera_id && cxt->fdae.pause) {
				cxt->fdae.pause = 0;
				cxt->lcd_mon_no_write = 0;
			}
			break;

		case AE_FLASH_MAIN_BEFORE:
			AE_LOGD("ae_flash_status FLASH_MAIN_BEFORE");
			cxt->cur_status.settings.flash = FLASH_MAIN_BEFORE;
			break;

		case AE_FLASH_MAIN_AE_MEASURE:
			AE_LOGD("ae_flash_status FLASH_MAIN_AE_MEASURE");
			cxt->cur_status.settings.flash = FLASH_MAIN;
			//for non-zsl
			cxt->mflash_wait_use = 0;
			break;

		case AE_FLASH_MAIN_LIGHTING:
			AE_LOGD("ae_flash_status FLASH_MAIN_LIGHTING");
			cxt->cur_status.settings.flash = FLASH_MAIN;
			//for non-zsl
			cxt->mflash_wait_use = 0;
			break;
#if 0
		case AE_LCD_FLASH_PRE_START:
			AE_LOGI("AE_TEST: AE_LCD_FLASH_PRE_START");
			if (cxt->camera_id && 0 == cxt->fdae.pause) {
				cxt->fdae.pause = 1;
			}
			memset(&cxt->flash_param, 0, sizeof(cxt->flash_param));
			if (flash_notice->lcd_flash_tune_a) {
				cxt->flash_param.lcd_tuning_a = flash_notice->lcd_flash_tune_a;
			}
			if (flash_notice->lcd_flash_tune_b) {
				cxt->flash_param.lcd_tuning_b = flash_notice->lcd_flash_tune_b;
			}

			cxt->flash_param.before_result = cxt->cur_result;
			cxt->lcd_mon_no_write = 1;
			cxt->cur_status.settings.lock_ae = AE_STATE_SEARCHING;
			cxt->flash_param.enable = 1;
			cxt->flash_param.flash_ratio = 256 * 2;
			cxt->flash_param.flash_state = AE_LCD_STEP_UP;

			cxt->flash_param.lcd_start_time = get_cur_timestamp(cxt);

			_cfg_monitor_bypass(cxt);

			cxt->flash_param.capture_result = cxt->cur_result;
			cxt->skip_calc_num = 0;
			cxt->flash_working = 1;
			break;
		case AE_LCD_FLASH_PRE_END:
			AE_LOGI("AE_TEST: AE_LCD_FLASH_PRE_END");
			cxt->flash_param.capture_result = cxt->cur_result;
			cxt->flash_param.enable = 1;
			cxt->flash_param.flash_state = AE_LCD_STEP_END;
			_cfg_monitor_bypass(cxt);
#endif
		default:
			rtn = AE_ERROR;
			break;
	}
	return rtn;
}

static int32_t _cfg_monitor_skip(struct ae_ctrl_cxt *cxt, struct ae_monitor_cfg *cfg)
{
	int32_t rtn = AE_SUCCESS;

	if (cxt->isp_ops.set_monitor) {
		// AE_LOGI("AE_TEST set monitor:%d",
		// *(uint32_t*)cfg);
		rtn = cxt->isp_ops.set_monitor(cxt->isp_ops.isp_handler, cfg);
	}

	return rtn;
}

static int32_t _cfg_monitor(struct ae_ctrl_cxt *cxt)
{
	int32_t rtn = AE_SUCCESS;

	_cfg_monitor_win(cxt);

	_cfg_monitor_skip(cxt, &cxt->monitor_unit.cfg);

	_cfg_monitor_bypass(cxt);

	return rtn;
}

static int32_t exp_time2exp_line(struct ae_ctrl_cxt *cxt, struct ae_exp_gain_table src[AE_FLICKER_NUM], struct ae_exp_gain_table dst[AE_FLICKER_NUM], int16_t linetime, int16_t tablemode)
{
	int32_t rtn = AE_SUCCESS;
	int32_t i = 0;
	float tmp_1 = 0;
	float tmp_2 = 0;
	int32_t mx = src[AE_FLICKER_50HZ].max_index;

	AE_LOGD("exp2line %d %d %d\r\n", linetime, tablemode, mx);

	dst[AE_FLICKER_60HZ].max_index = src[AE_FLICKER_50HZ].max_index;
	dst[AE_FLICKER_60HZ].min_index = src[AE_FLICKER_50HZ].min_index;
	if (0 == tablemode){
		for (i = 0; i <= mx; i++){
			tmp_1 = src[AE_FLICKER_50HZ].exposure[i] / (float)linetime;
			dst[AE_FLICKER_50HZ].exposure[i] = (int32_t)tmp_1;

			if (0 == (int32_t)tmp_1)
				tmp_2 = 1;
			else
				tmp_2 = tmp_1 / (int32_t)tmp_1;

			dst[AE_FLICKER_50HZ].again[i] =
						(int32_t)(0.5 + tmp_2 * src[AE_FLICKER_50HZ].again[i]);
		}

		for (i = 0; i <= mx; i++){
			if (83333 <= src[AE_FLICKER_50HZ].exposure[i]){
				tmp_1 = dst[AE_FLICKER_50HZ].exposure[i] * 5 / 6.0;
				dst[AE_FLICKER_60HZ].exposure[i] = (int32_t)tmp_1;

				if (0 == (int32_t)tmp_1)
					tmp_2 = 1;
				else
					tmp_2 = (float)(dst[AE_FLICKER_50HZ].exposure[i]) / (int32_t)tmp_1;

				dst[AE_FLICKER_60HZ].again[i] =
							(int32_t)(0.5 + tmp_2 * dst[AE_FLICKER_50HZ].again[i]);
			}else{
				dst[AE_FLICKER_60HZ].exposure[i] = dst[AE_FLICKER_50HZ].exposure[i];
				dst[AE_FLICKER_60HZ].again[i] = dst[AE_FLICKER_50HZ].again[i];
			}
		}
	}else{
		for (i = 0; i <= mx; i++){
			if (83333 <= dst[AE_FLICKER_50HZ].exposure[i] * linetime){
				tmp_1 = dst[AE_FLICKER_50HZ].exposure[i] * 5 / 6.0;
				dst[AE_FLICKER_60HZ].exposure[i] = (int32_t)tmp_1;

				if (0 == (int32_t)tmp_1)
					tmp_2 = 1;
				else
					tmp_2 = (float)(dst[AE_FLICKER_50HZ].exposure[i]) / (int32_t)tmp_1;

				dst[AE_FLICKER_60HZ].again[i] =
							(int32_t)(0.5 + tmp_2 * dst[AE_FLICKER_50HZ].again[i]);
			}else{
				dst[AE_FLICKER_60HZ].exposure[i] =	dst[AE_FLICKER_50HZ].exposure[i];
				dst[AE_FLICKER_60HZ].again[i] = 	dst[AE_FLICKER_50HZ].again[i];
			}
		}
	}
	return rtn;
}
static int32_t exposure_time2line(struct ae_tuning_param *tp, int16_t linetime, int16_t tablemode)
{
	int32_t rtn = AE_SUCCESS;
	int32_t i = 0;
	float tmp_1 = 0;
	float tmp_2 = 0;
	int32_t mx = tp->backup_ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].max_index;

	AE_LOGD("exp2line %d %d %d\r\n", linetime, tablemode, mx);

	tp->ae_table[AE_FLICKER_60HZ][AE_ISO_AUTO].max_index = tp->backup_ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].max_index;
	tp->ae_table[AE_FLICKER_60HZ][AE_ISO_AUTO].min_index = tp->backup_ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].min_index;
	if (0 == tablemode){
		for (i = 0; i <= mx; i++){
			tmp_1 = tp->backup_ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].exposure[i] / (float)linetime;
			tp->ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].exposure[i] = (int32_t)tmp_1;

			if (0 == (int32_t)tmp_1)
				tmp_2 = 1;
			else
				tmp_2 = tmp_1 / (int32_t)tmp_1;

			tp->ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].again[i] =
						(int32_t)(0.5 + tmp_2 * tp->backup_ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].again[i]);
		}

		for (i = 0; i <= mx; i++){
			if (83333 <= tp->backup_ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].exposure[i]){
				tmp_1 = tp->ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].exposure[i] * 5 / 6.0;
				tp->ae_table[AE_FLICKER_60HZ][AE_ISO_AUTO].exposure[i] = (int32_t)tmp_1;

				if (0 == (int32_t)tmp_1)
					tmp_2 = 1;
				else
					tmp_2 = (float)(tp->ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].exposure[i]) / (int32_t)tmp_1;

				tp->ae_table[AE_FLICKER_60HZ][AE_ISO_AUTO].again[i] =
							(int32_t)(0.5 + tmp_2 * tp->ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].again[i]);
			}else{
				tp->ae_table[AE_FLICKER_60HZ][AE_ISO_AUTO].exposure[i] =
														tp->ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].exposure[i];
				tp->ae_table[AE_FLICKER_60HZ][AE_ISO_AUTO].again[i] =
														tp->ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].again[i];
			}
		}
	}else{
		for (i = 0; i <= mx; i++){
			if (83333 <= tp->ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].exposure[i] * linetime){
				tmp_1 = tp->ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].exposure[i] * 5 / 6.0;
				tp->ae_table[AE_FLICKER_60HZ][AE_ISO_AUTO].exposure[i] = (int32_t)tmp_1;

				if (0 == (int32_t)tmp_1)
					tmp_2 = 1;
				else
					tmp_2 = (float)(tp->ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].exposure[i]) / (int32_t)tmp_1;

				tp->ae_table[AE_FLICKER_60HZ][AE_ISO_AUTO].again[i] =
							(int32_t)(0.5 + tmp_2 * tp->ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].again[i]);
			}else{
				tp->ae_table[AE_FLICKER_60HZ][AE_ISO_AUTO].exposure[i] =
														tp->ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].exposure[i];
				tp->ae_table[AE_FLICKER_60HZ][AE_ISO_AUTO].again[i] =
														tp->ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO].again[i];
			}
		}
	}
	return rtn;
}

// set default parameters when initialization or DC-DV convertion
static int32_t _set_ae_param(struct ae_ctrl_cxt *cxt, struct ae_init_in *init_param, struct ae_set_work_param *work_param, int8_t init)
{
	int32_t rtn = AE_SUCCESS;
	uint32_t i = 0;
	uint32_t j = 0;
	int8_t cur_work_mode = AE_WORK_MODE_COMMON;
	struct ae_trim trim;
	struct ae_ev_table *ev_table = NULL;

	AE_LOGD("Init param %d\r\n", init);

	if (AE_PARAM_INIT == init) {
		if (NULL == cxt || NULL == init_param) {
			AE_LOGE("cxt %p, init_param %p PARAM is error\r\n", cxt, init_param);
			return AE_ERROR;
		}
		// AE_LOGD("tunnig_param %d", sizeof(struct ae_tuning_param));
		// AE_LOGD("param_num %d", init_param->param_num);

		cur_work_mode = AE_WORK_MODE_COMMON;
		for (i = 0; i < init_param->param_num && i < AE_MAX_PARAM_NUM; ++i) {
			rtn = _unpack_tunning_param(init_param->param[i].param, init_param->param[i].size, &cxt->tuning_param[i]);
			memcpy(cxt->tuning_param[i].backup_ae_table, cxt->tuning_param[i].ae_table, sizeof(struct ae_exp_gain_table));
			exposure_time2line(&(cxt->tuning_param[i]), init_param->resolution_info.line_time,
								cxt->tuning_param[i].ae_tbl_exp_mode);
			for (j = 0; j < AE_SCENE_MAX; ++j) {
				memcpy(&cxt->back_scene_mode_ae_table[j][AE_FLICKER_50HZ], &cxt->tuning_param[i].scene_info[j].ae_table[AE_FLICKER_50HZ], AE_FLICKER_NUM * sizeof(struct ae_exp_gain_table));
				if ((1 == cxt->tuning_param[i].scene_info[j].table_enable) && (0 == cxt->tuning_param[i].scene_info[j].exp_tbl_mode)) {
					//exp_time2exp_line(cxt,j, init_param->resolution_info.line_time,
					//			cxt->tuning_param[i].scene_info[j].exp_tbl_mode, &(cxt->tuning_param[i].scene_info[j]));
					exp_time2exp_line(cxt, cxt->back_scene_mode_ae_table[j],\
										cxt->tuning_param[i].scene_info[j].ae_table,\
										init_param->resolution_info.line_time,\
										cxt->tuning_param[i].scene_info[j].exp_tbl_mode);
				}
			}
			if (AE_SUCCESS == rtn)
				cxt->tuning_param_enable[i] = 1;
			else
				cxt->tuning_param_enable[i] = 0;
		}

		cxt->camera_id = init_param->camera_id;
		cxt->isp_ops = init_param->isp_ops;
		cxt->monitor_unit.win_num = init_param->monitor_win_num;

		cxt->snr_info = init_param->resolution_info;
		cxt->cur_status.frame_size = init_param->resolution_info.frame_size;
		cxt->cur_status.line_time = init_param->resolution_info.line_time;
		trim.x = 0;
		trim.y = 0;
		trim.w = init_param->resolution_info.frame_size.w;
		trim.h = init_param->resolution_info.frame_size.h;

		cxt->sof_id = 0;
		cxt->cur_status.frame_id = 0;
		memset(&cxt->cur_result, 0, sizeof(struct ae_alg_calc_result));
	} else if (AE_PARAM_NON_INIT == init) {
		;
	}
	/*
	 * parameter of common mode should not be NULL 
	 */
	if (0 == cxt->tuning_param_enable[AE_WORK_MODE_COMMON]) {
		AE_LOGE("common param is error");
		return AE_ERROR;
	}

	cxt->start_id = AE_START_ID;

	cxt->monitor_unit.mode = AE_STATISTICS_MODE_CONTINUE;
	cxt->monitor_unit.cfg.skip_num = 0;
	cxt->monitor_unit.is_stop_monitor = 0;
	// set cxt->monitor_unit.trim &
	// cxt->monitor_unit.win_size;
	_update_monitor_unit(cxt, &trim);

	if (1 == cxt->tuning_param_enable[cur_work_mode])
		cxt->cur_param = &cxt->tuning_param[cur_work_mode];
	else
		cxt->cur_param = &cxt->tuning_param[AE_WORK_MODE_COMMON];

	for (i = 0; i < 16; ++i) {
		cxt->stable_zone_ev[i] = cxt->cur_param->stable_zone_ev[i];
	}
	for (i = 0; i < 32; ++i) {
		cxt->cnvg_stride_ev[i] = cxt->cur_param->cnvg_stride_ev[i];
	}
	cxt->cnvg_stride_ev_num = cxt->cur_param->cnvg_stride_ev_num;
	cxt->exp_skip_num = cxt->cur_param->sensor_cfg.exp_skip_num;
	cxt->gain_skip_num = cxt->cur_param->sensor_cfg.gain_skip_num;

	cxt->cur_status.alg_id = cxt->cur_param->alg_id;
	cxt->cur_status.win_size = cxt->monitor_unit.win_size;
	cxt->cur_status.win_num = cxt->monitor_unit.win_num;
	cxt->cur_status.awb_gain.r = 1024;
	cxt->cur_status.awb_gain.g = 1024;
	cxt->cur_status.awb_gain.b = 1024;

	cxt->cur_status.ae_table = &cxt->cur_param->ae_table[cxt->cur_param->flicker_index][AE_ISO_AUTO];
	cxt->cur_status.weight_table = cxt->cur_param->weight_table[AE_WEIGHT_CENTER].weight;
	cxt->cur_status.stat_img = NULL;

	if (2 == cxt->cur_status.alg_id){
		cxt->cur_status.min_exp_line = cxt->cur_param->min_line;	// 
		cxt->cur_status.max_gain = cxt->cur_param->sensor_cfg.max_gain;	// /*sensor can support the max  gain*/
		cxt->sensor_gain_precision = cxt->cur_param->sensor_cfg.gain_precision;
		cxt->cur_status.min_gain = cxt->cur_param->sensor_cfg.min_gain;/*sensor can support the min  gain*/
	}else{
		AE_LOGD("old ae params\r\n");
		cxt->cur_status.min_exp_line = 6;	// 
		cxt->cur_status.max_gain = 1024;	// /*sensor can support the max  gain*/
		cxt->sensor_gain_precision = 16;
		cxt->cur_status.min_gain = 128;/*sensor can support the min  gain*/
	}

	cxt->cur_status.start_index = cxt->cur_param->start_index;
	ev_table = &cxt->cur_param->ev_table;
	cxt->cur_status.target_lum = _calc_target_lum(cxt->cur_param->target_lum, ev_table->default_level, ev_table);
	cxt->cur_status.target_lum_zone = cxt->stable_zone_ev[ev_table->default_level];

	cxt->cur_status.b = NULL;
	cxt->cur_status.r = NULL;
	cxt->cur_status.g = NULL;
	// adv_alg module init
	cxt->cur_status.adv[0] = (void *)&cxt->cur_param->region_param;
	cxt->cur_status.adv[1] = (void *)&cxt->cur_param->flat_param;
	cxt->cur_status.adv[2] = (void *)&cxt->cur_param->mulaes_param;
	cxt->cur_status.adv[3] = (void *)&cxt->cur_param->touch_info;
	cxt->cur_status.adv[4] = (void*)&cxt->cur_param->face_param;
	// caliberation for bv match with lv
	//cxt->cur_param->lv_cali.lux_value = 11800;//hjw
	//cxt->cur_param->lv_cali.bv_value = 1118;//hjw
	cxt->cur_status.lv_cali_bv= cxt->cur_param->lv_cali.bv_value;
	{
		cxt->cur_status.lv_cali_lv = log(cxt->cur_param->lv_cali.lux_value * 2.17) * 1.45;//  (1 / ln2) = 1.45;
	}
	// refer to convergence
	cxt->cur_status.ae_start_delay = cxt->cur_param->enter_skip_num;
	cxt->cur_status.stride_config[0] = cxt->cnvg_stride_ev[ev_table->default_level * 2];
	cxt->cur_status.stride_config[1] = cxt->cnvg_stride_ev[ev_table->default_level * 2 + 1];
	cxt->cur_status.under_satu = 5;
	cxt->cur_status.ae_satur = 250;

	cxt->cur_status.settings.ver = 0;
	cxt->cur_status.settings.lock_ae = 0;
	/*
	 * Be effective when 1 == lock_ae 
	 */
	cxt->cur_status.settings.exp_line = 0;
	cxt->cur_status.settings.gain = 0;
	/*
	 * Be effective when being not in the video mode 
	 */
	cxt->cur_status.settings.min_fps = 5;
	cxt->cur_status.settings.max_fps = 30;

	AE_LOGI("snr setting fps: %d\n", cxt->snr_info.snr_setting_max_fps);
	if (0 != cxt->snr_info.snr_setting_max_fps) {
		cxt->cur_status.settings.sensor_max_fps = 30;//cxt->snr_info.snr_setting_max_fps;
	} else {
		cxt->cur_status.settings.sensor_max_fps = 30;
	}

	//flash ae param 
	cxt->cur_status.settings.flash = 0;
	cxt->cur_status.settings.flash_ration = cxt->cur_param->flash_param.adjust_ratio;
	//cxt->flash_on_off_thr;
	AE_LOGI("flash_ration_mp: %d\n", cxt->cur_status.settings.flash_ration);

	cxt->cur_status.settings.iso = AE_ISO_AUTO;
	cxt->cur_status.settings.ev_index = ev_table->default_level;
	cxt->cur_status.settings.flicker = cxt->cur_param->flicker_index;

	cxt->cur_status.settings.flicker_mode = 0;
	cxt->cur_status.settings.FD_AE = 0;
	cxt->cur_status.settings.metering_mode = AE_WEIGHT_CENTER;
	cxt->cur_status.settings.work_mode = cur_work_mode;
	cxt->cur_status.settings.scene_mode = AE_SCENE_NORMAL;
	cxt->cur_status.settings.intelligent_module = 0;

	cxt->cur_status.settings.reserve_case = 0;
	cxt->cur_status.settings.reserve_len = 0;	/* len for reserve */
	cxt->cur_status.settings.reserve_info = NULL;	/* reserve for future */

	cxt->touch_zone_param.zone_param = cxt->cur_param->touch_param;
	if (0 == cxt->touch_zone_param.zone_param.level_0_weight) {
		cxt->touch_zone_param.zone_param.level_0_weight = 1;
		cxt->touch_zone_param.zone_param.level_1_weight = 1;
		cxt->touch_zone_param.zone_param.level_2_weight = 1;
		cxt->touch_zone_param.zone_param.level_1_percent = 0;
		cxt->touch_zone_param.zone_param.level_2_percent = 0;
	}
	/*to keep the camera can work well when the touch ae setting is zero or NULL in the tuning file 	 */
	cxt->cur_status.settings.touch_tuning_enable = cxt->cur_param->touch_info.enable;
	if ((1 == cxt->cur_param->touch_info.enable)//for debug, if release, we need to change 0 to 1
		&& ((0 == cxt->cur_param->touch_info.touch_tuning_win.w)
	    || (0 == cxt->cur_param->touch_info.touch_tuning_win.h)
	    || ((0 == cxt->cur_param->touch_info.win1_weight)
		&& (0 == cxt->cur_param->touch_info.win2_weight)))) {
		cxt->cur_status.touch_tuning_win.w = cxt->snr_info.frame_size.w / 10;
		cxt->cur_status.touch_tuning_win.h = cxt->snr_info.frame_size.h / 10;
		cxt->cur_status.win1_weight = 4;
		cxt->cur_status.win2_weight = 3;
		AE_LOGD("TC_NO these setting at tuning file");
	} else {
		cxt->cur_status.touch_tuning_win = cxt->cur_param->touch_info.touch_tuning_win;
		cxt->cur_status.win1_weight = cxt->cur_param->touch_info.win1_weight;
		cxt->cur_status.win2_weight = cxt->cur_param->touch_info.win2_weight;
		AE_LOGD("TC_Have these setting at tuning file");
		AE_LOGD("TC_Beth-SPRD:touch tuning win info:W:%d,H:%d!", cxt->cur_param->touch_info.touch_tuning_win.w, cxt->cur_param->touch_info.touch_tuning_win.h);
	}
	/*
	 * fd-ae param 
	 */
	_fdae_init(cxt);
	/*
	 * lcd flash param 
	 */
	cxt->lcd_mon_no_write = 0;
	/*
	 * control information for sensor update 
	 */
	cxt->checksum = _get_checksum();
	cxt->end_id = AE_END_ID;

	_cfg_set_aem_mode(cxt);	/* set ae monitor work mode */
	AE_LOGD("ALG_id %d   %d\r\n", cxt->cur_param->alg_id, sizeof(struct ae_tuning_param));
	//AE_LOGD("DP %d   lv-bv %.3f\r\n", cxt->sensor_gain_precision, cxt->cur_status.cali_lv_eight);
	return AE_SUCCESS;
}

static int32_t _sof_handler(struct ae_ctrl_cxt *cxt)
{
	int32_t rtn = AE_SUCCESS;

	// _fdae_update_state(cxt);
	return rtn;
}

static int32_t _ae_online_ctrl_set(struct ae_ctrl_cxt *cxt, void *param)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_online_ctrl* ae_ctrl_ptr = (struct ae_online_ctrl*)param;
	struct ae_misc_calc_out ctrl_result;
	uint32_t ctrl_index = 0;

	if ((NULL == cxt) || (NULL == param)) {
		AE_LOGE("in %p out %p param is error!", cxt, param);
		return AE_PARAM_NULL;
	}

	AE_LOGD("mode:%d, idx:%d, s:%d, g:%d\n", ae_ctrl_ptr->mode, ae_ctrl_ptr->index, ae_ctrl_ptr->shutter, ae_ctrl_ptr->again);
	cxt->cur_status.settings.lock_ae = AE_STATE_LOCKED;
	if (AE_CTRL_SET_INDEX == ae_ctrl_ptr->mode) {
		cxt->cur_status.settings.manual_mode = 1;/*ae index*/
		cxt->cur_status.settings.table_idx = ae_ctrl_ptr->index;
	} else {/*exposure & gain*/
		cxt->cur_status.settings.manual_mode = 0;
		cxt->cur_status.settings.exp_line = ae_ctrl_ptr->shutter;
		cxt->cur_status.settings.gain = ae_ctrl_ptr->again;
	}

	return rtn;
}

static int32_t _ae_online_ctrl_get(struct ae_ctrl_cxt *cxt, void *result)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_online_ctrl* ae_ctrl_ptr = (struct ae_online_ctrl*)result;

	if ((NULL == cxt) || (NULL == result)) {
		AE_LOGE("in %p out %p param is error!", cxt, result);
		return AE_PARAM_NULL;
	}
	ae_ctrl_ptr->index = cxt->sync_cur_result.wts.cur_index;
	ae_ctrl_ptr->lum = cxt->sync_cur_result.cur_lum;
	ae_ctrl_ptr->shutter = cxt->sync_cur_result.wts.cur_exp_line;
	ae_ctrl_ptr->dummy= cxt->sync_cur_result.wts.cur_dummy;
	ae_ctrl_ptr->again = cxt->sync_cur_result.wts.cur_again;
	ae_ctrl_ptr->dgain = cxt->sync_cur_result.wts.cur_dgain;
	ae_ctrl_ptr->skipa = 0;
	ae_ctrl_ptr->skipd = 0;
	AE_LOGD("idx:%d, s:%d, g:%d\n", ae_ctrl_ptr->index, ae_ctrl_ptr->shutter, ae_ctrl_ptr->again);
	return rtn;
}

static int32_t _tool_online_ctrl(struct ae_ctrl_cxt *cxt, void *param, void *result)
{
	int32_t rtn = AE_SUCCESS;

	struct ae_online_ctrl *ae_ctrl_ptr = (struct ae_online_ctrl *)param;

	if ((AE_CTRL_SET_INDEX == ae_ctrl_ptr->mode)
	    || (AE_CTRL_SET == ae_ctrl_ptr->mode)) {
		rtn = _ae_online_ctrl_set(cxt, param);
	} else {
		rtn = _ae_online_ctrl_get(cxt, result);
	}

	return rtn;
}

static int32_t _printf_status_log(struct ae_ctrl_cxt *cxt, int8_t scene_mod, struct ae_alg_calc_param *alg_status)
{
	int32_t ret = AE_SUCCESS;

	AE_LOGD("scene: %d\n", scene_mod);
	AE_LOGD("target: %d, zone: %d\n", alg_status->target_lum, alg_status->target_lum_zone);
	AE_LOGD("iso: %d\n", alg_status->settings.iso);
	AE_LOGD("ev offset: %d\n", alg_status->settings.ev_index);
	AE_LOGD("fps: [%d, %d]\n", alg_status->settings.min_fps, alg_status->settings.max_fps);
	AE_LOGD("metering: %d--ptr%p\n", alg_status->settings.metering_mode, alg_status->weight_table);
	AE_LOGD("flicker: %d, table: %p, range: [%d, %d]\n", alg_status->settings.flicker, alg_status->ae_table, alg_status->ae_table->min_index, alg_status->ae_table->max_index);

	return ret;
}

/*set_scene_mode just be called in ae_sprd_calculation,*/
static int32_t _set_scene_mode(struct ae_ctrl_cxt *cxt, enum ae_scene_mode cur_scene_mod, enum ae_scene_mode nxt_scene_mod)
{

	int32_t rtn = AE_SUCCESS;
	struct ae_tuning_param *cur_param = NULL;
	struct ae_scene_info *scene_info = NULL;
	struct ae_alg_calc_param *cur_status = NULL;
	struct ae_alg_calc_param *prv_status = NULL;
	struct ae_exp_gain_table *ae_table = NULL;
	struct ae_weight_table *weight_table = NULL;
	struct ae_set_fps fps_param;
	uint32_t i = 0;
	int32_t target_lum = 0;
	uint32_t iso = 0;
	uint32_t weight_mode = 0;
	
	prv_status = &cxt->prv_status;
	cur_status = &cxt->cur_status;

	if (nxt_scene_mod >= AE_SCENE_MAX) {
		AE_LOGE("scene mod is invalidated, %d\n", nxt_scene_mod);
		return AE_ERROR;
	}

	cur_param = cxt->cur_param;
	scene_info = &cur_param->scene_info[0];

	if ((AE_SCENE_NORMAL == cur_scene_mod) && (AE_SCENE_NORMAL == nxt_scene_mod)) {
		AE_LOGI("normal  has special setting\n");
		goto SET_SCENE_MOD_EXIT;
	}
	if (AE_SCENE_NORMAL != nxt_scene_mod) {
		for (i = 0; i < AE_SCENE_MAX; ++i) {
			AE_LOGI("%d: mod: %d, eb: %d\n", i, scene_info[i].scene_mode, scene_info[i].enable);
			if ((1 == scene_info[i].enable) && (nxt_scene_mod == scene_info[i].scene_mode)) {
				break;
			}
		}

		if ((i >=  AE_SCENE_MAX) && (AE_SCENE_NORMAL != nxt_scene_mod)) {
			AE_LOGI("Not has special scene setting, just using the normal setting\n");
			goto SET_SCENE_MOD_EXIT;
		}
	}

	if (AE_SCENE_NORMAL != nxt_scene_mod) {
		/*normal scene--> special scene*/ 
		/*special scene--> special scene*/ 
		AE_LOGI("i and iso_index is %d,%d,%d",i,scene_info[i].iso_index,scene_info[i].weight_mode);
		iso = scene_info[i].iso_index;
		weight_mode = scene_info[i].weight_mode;

		if (iso >= AE_ISO_MAX || weight_mode >= AE_WEIGHT_MAX) {					
			AE_LOGE("error iso=%d, weight_mode=%d", iso, weight_mode);						
			rtn = AE_ERROR;						
			goto SET_SCENE_MOD_EXIT;
		}
		if (AE_SCENE_NORMAL == cur_scene_mod) {/*from normal scene to special scene*/					
			cxt->prv_status = *cur_status;/*backup the normal scene's information*/
		}		
		/*ae table*/
		#if 0
		for(int j=0; j <= 327; j++){
			AE_LOGD("Current_status_exp_gain is %d,%d\n",scene_info[i].ae_table[0].exposure[j],scene_info[i].ae_table[0].again[j]);
		}
		AE_LOGD("CURRENT_STATUS_EXP_GAIN : %d,%d,%d",cur_status->effect_expline,cur_status->effect_gain,cur_status->settings.iso);
		AE_LOGD("TABLE_ENABLE IS %d",scene_info[i].table_enable);
		if (scene_info[i].table_enable) {
			cur_status->ae_table = &scene_info[i].ae_table[cur_status->settings.flicker];
		}
		for(int j=0; j <= 327; j++){
			AE_LOGD("Current_status_exp_gain is %d,%d\n",cur_status->ae_table->exposure[j], cur_status->ae_table->again[j]);
		}
		#endif
		cur_status->settings.iso  = scene_info[i].iso_index;
		cur_status->settings.min_fps = scene_info[i].min_fps;
		cur_status->settings.max_fps = scene_info[i].max_fps;
		target_lum = _calc_target_lum(scene_info[i].target_lum, scene_info[i].ev_offset, &cur_param->ev_table);
		cur_status->target_lum_zone = (int16_t)(cur_param->target_lum_zone * target_lum * 1.0 / cur_param->target_lum + 0.5);
		cur_status->target_lum  = target_lum;
		cur_status->weight_table = &cur_param->weight_table[scene_info[i].weight_mode];
		cur_status->settings.metering_mode = scene_info[i].weight_mode;
		cur_status->settings.scene_mode = nxt_scene_mod;
	} 
	
	if (AE_SCENE_NORMAL == nxt_scene_mod){/*special scene --> normal scene*/ 	
SET_SCENE_MOD_2_NOAMAL:	
		iso = prv_status->settings.iso;
		weight_mode = prv_status->settings.metering_mode;
		if (iso >= AE_ISO_MAX || weight_mode >= AE_WEIGHT_MAX) {
			AE_LOGI("error iso=%d, weight_mode=%d", iso, weight_mode);
			rtn = AE_ERROR;
			goto SET_SCENE_MOD_EXIT;
		}
		target_lum = _calc_target_lum(cur_param->target_lum, prv_status->settings.ev_index, &cur_param->ev_table);
		cur_status->target_lum  = target_lum;
		cur_status->target_lum_zone = (int16_t)(cur_param->target_lum_zone * (target_lum * 1.0 / cur_param->target_lum) + 0.5);
		cur_status->settings.ev_index = prv_status->settings.ev_index;
		cur_status->settings.iso = prv_status->settings.iso;
		cur_status->settings.metering_mode = prv_status->settings.metering_mode;
		cur_status->weight_table = prv_status->weight_table;
		//cur_status->ae_table = &cur_param->ae_table[prv_status->settings.flicker][prv_status->settings.iso];
		cur_status->ae_table = &cur_param->ae_table[prv_status->settings.flicker][AE_ISO_AUTO];
		cur_status->settings.min_fps = prv_status->settings.min_fps;
		cur_status->settings.max_fps = prv_status->settings.max_fps;
		cur_status->settings.scene_mode = nxt_scene_mod;
	}

SET_SCENE_MOD_EXIT:	
	AE_LOGI("change scene mode from %d to %d, rtn=%d", cur_scene_mod, nxt_scene_mod, rtn);

	return rtn;
}

/**************************************************************************/
/*
 *Begin: FDAE related functions
 */
static int32_t _fdae_init(struct ae_ctrl_cxt *cxt)
{
	int32_t rtn = AE_SUCCESS;

	cxt->fdae.allow_to_work = 1;
	cxt->fdae.enable = 1;
	cxt->fdae.face_info.face_num = 0;
	return rtn;
}
// add by matchbox for fd_info
static int32_t _fd_info_init(struct ae_ctrl_cxt *cxt)
{
	int32_t rtn = AE_SUCCESS;
	struct ae1_fd_param *ae1_fd = NULL;
	int32_t i = 0;

	if (NULL == cxt) {
		AE_LOGE("cxt is null");
		return AE_ERROR;
	}
	/*protection mechanism for face tuning */
	/*cxt->cur_status.face_tp.face_tuning_enable = cxt->cur_param->face_param.face_tuning_enable;
	if((0 == cxt->cur_param->face_param.face_target \
		|| 0 == cxt->cur_status.face_tp.face_tuning_lum1 \
		|| 0 == cxt->cur_status.face_tp.face_tuning_lum2 \
		|| 0 == cxt->cur_status.face_tp.cur_offset_weight) \
		&& 1 == cxt->cur_param->face_param.face_tuning_enable) {
		cxt->cur_status.face_tp.face_target = 75;
		cxt->cur_status.face_tp.face_tuning_lum1 = 50;
		cxt->cur_status.face_tp.face_tuning_lum2 = 65;
		cxt->cur_status.face_tp.cur_offset_weight = 20;
	} else {
		cxt->cur_status.face_tp.face_target = cxt->cur_param->face_param.face_target;
		cxt->cur_status.face_tp.face_tuning_lum1 = cxt->cur_param->face_param.face_tuning_lum1;
		cxt->cur_status.face_tp.face_tuning_lum2 = cxt->cur_param->face_param.face_tuning_lum2;
		cxt->cur_status.face_tp.cur_offset_weight = cxt->cur_param->face_param.cur_offset_weight;
	}
	*/	
	/*for debug face tuning*/
	/*
	if(0 == cxt->cur_param->face_param.face_target && 0 == cxt->cur_param->face_param.face_tuning_enable){
		cxt->cur_status.face_tp.face_target = 65;
		cxt->cur_status.face_tp.face_tuning_enable = 1;//if tuning tool have this setting, we should change the setting from 1 to 0;
		} else {
		cxt->cur_status.face_tp.face_target = cxt->cur_param->face_param.face_target;
		cxt->cur_status.face_tp.face_tuning_enable = cxt->cur_param->face_param.face_tuning_enable;
	}
	*/
	ae1_fd = &(cxt->cur_status.ae1_finfo);
	ae1_fd->enable_flag = 1;
	ae1_fd->update_flag = 0;
	ae1_fd->cur_info.face_num = 0;
	//ae1_fd->pre_info.face_num = 0;
	for (i = 0; i != 20; i++) {
		// init pre info
		/*
		ae1_fd->pre_info.face_area[i].start_x = 0;
		ae1_fd->pre_info.face_area[i].start_y = 0;
		ae1_fd->pre_info.face_area[i].end_x = 0;
		ae1_fd->pre_info.face_area[i].end_y = 0;
		ae1_fd->pre_info.face_area[i].pose = -1;
		*/
		// init cur_info
		ae1_fd->cur_info.face_area[i].start_x = 0;
		ae1_fd->cur_info.face_area[i].start_y = 0;
		ae1_fd->cur_info.face_area[i].end_x = 0;
		ae1_fd->cur_info.face_area[i].end_y = 0;
		ae1_fd->cur_info.face_area[i].pose = -1;
	}
	for (i = 0; i != 1024; i++) {
		//ae1_fd->pre_info.rect[i] = 0;
		ae1_fd->cur_info.rect[i] = 0;
	}
	return rtn;
}
static int32_t _fd_info_pre_set(struct ae_ctrl_cxt *cxt)
{
	int32_t rtn = AE_SUCCESS;
	struct ae1_fd_param *ae1_fd = NULL;
	int32_t i = 0;

	if (NULL == cxt) {
		AE_LOGE("cxt is null");
		return AE_ERROR;
	}
	ae1_fd = &(cxt->cur_status.ae1_finfo);
	ae1_fd->update_flag = 0;	// set updata flag
	//ae1_fd->pre_info.face_num = ae1_fd->cur_info.face_num;
	AE_LOGD("face_num is %d\n",cxt->fdae.face_info.face_num);
	//for (i = 0; i != ae1_fd->cur_info.face_num; i++) {
	for (i = 0; i < cxt->fdae.face_info.face_num; i++) {
		// save pre info
		/*
		ae1_fd->pre_info.face_area[i].start_x = ae1_fd->cur_info.face_area[i].start_x;
		ae1_fd->pre_info.face_area[i].start_y = ae1_fd->cur_info.face_area[i].start_y;
		ae1_fd->pre_info.face_area[i].end_x = ae1_fd->cur_info.face_area[i].end_x;
		ae1_fd->pre_info.face_area[i].end_y = ae1_fd->cur_info.face_area[i].end_y;
		ae1_fd->pre_info.face_area[i].pose = ae1_fd->cur_info.face_area[i].pose;
		*/
		// reset cur_info
		ae1_fd->cur_info.face_area[i].start_x = 0;
		ae1_fd->cur_info.face_area[i].start_y = 0;
		ae1_fd->cur_info.face_area[i].end_x = 0;
		ae1_fd->cur_info.face_area[i].end_y = 0;
		ae1_fd->cur_info.face_area[i].pose = -1;
	}
	for (i = 0; i != 1024; i++) {
		//ae1_fd->pre_info.rect[i] = ae1_fd->cur_info.rect[i];
		ae1_fd->cur_info.rect[i] = 0;
	}
	return rtn;

}

static int32_t _fd_info_set(struct ae_ctrl_cxt *cxt)
{
	int32_t rtn = AE_SUCCESS;
	int32_t i = 0;
	int32_t x = 0;
	int32_t y = 0;
	int32_t height_tmp = 0;
	int32_t width_tmp = 0;
	int32_t win_w = 0;
	int32_t win_h = 0;
	uint32_t value = 0;
	int32_t f_w_shk_x = 0;//face win shrink ratio at x
	int32_t f_w_shk_y = 0;//face win shrink ratio at y
	struct ae_fd_param *fd = NULL;
	struct ae1_fd_param *ae1_fd = NULL;

	if (NULL == cxt) {
		AE_LOGE("cxt is null");
		return AE_ERROR;
	}
	win_w = cxt->monitor_unit.win_num.w;
	win_h = cxt->monitor_unit.win_num.h;
	fd = &(cxt->fdae.face_info);
	ae1_fd = &(cxt->cur_status.ae1_finfo);
	height_tmp = fd->height;
	width_tmp = fd->width;
	ae1_fd->update_flag = 1;// set updata_flag
	ae1_fd->cur_info.face_num = fd->face_num;
	//AE_LOGD("fd ae face number:%d ", ae1_fd->cur_info.face_num);
	if (0 == fd->face_num) {// face num = 0 ,no mapping
		return AE_SUCCESS;
	}
	for (i = 0; i < fd->face_num; i++) {
		// map every face location info	
		f_w_shk_x = (fd->face_area[i].rect.end_x - fd->face_area[i].rect.start_x + 1)*40/200;
		f_w_shk_y = (fd->face_area[i].rect.end_y - fd->face_area[i].rect.start_y + 1)*40/200;
	
		ae1_fd->cur_info.face_area[i].start_x = (fd->face_area[i].rect.start_x + f_w_shk_x)*win_w/width_tmp;
		ae1_fd->cur_info.face_area[i].end_x = (fd->face_area[i].rect.end_x - f_w_shk_x)*win_w/width_tmp;
		ae1_fd->cur_info.face_area[i].start_y = (fd->face_area[i].rect.start_y + f_w_shk_y)*win_h/height_tmp;
		ae1_fd->cur_info.face_area[i].end_y = (fd->face_area[i].rect.end_y - f_w_shk_y)*win_h/height_tmp;
		// set the frame face location info
			for (y = ae1_fd->cur_info.face_area[i].start_y; y <= ae1_fd->cur_info.face_area[i].end_y; y++) {
				for (x = ae1_fd->cur_info.face_area[i].start_x; x <= ae1_fd->cur_info.face_area[i].end_x; x++) {
				ae1_fd->cur_info.rect[x + y * win_w ] = 1;
			}
		}
	}
		 /* debug print */
/*
		 for(i = 0;i != 32;i++){
        AE_LOGD("fd_rect:%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
        ae1_fd->cur_info.rect[i * 32 + 0],ae1_fd->cur_info.rect[i * 32 + 1],ae1_fd->cur_info.rect[i * 32 + 2],ae1_fd->cur_info.rect[i * 32 + 3],
        ae1_fd->cur_info.rect[i * 32 + 4],ae1_fd->cur_info.rect[i * 32 + 5],ae1_fd->cur_info.rect[i * 32 + 6],ae1_fd->cur_info.rect[i * 32 + 7],
        ae1_fd->cur_info.rect[i * 32 + 8],ae1_fd->cur_info.rect[i * 32 + 9],ae1_fd->cur_info.rect[i * 32 + 10],ae1_fd->cur_info.rect[i * 32 + 11],
        ae1_fd->cur_info.rect[i * 32 + 12],ae1_fd->cur_info.rect[i * 32 + 13],ae1_fd->cur_info.rect[i * 32 + 14],ae1_fd->cur_info.rect[i * 32 + 15],
        ae1_fd->cur_info.rect[i * 32 + 16],ae1_fd->cur_info.rect[i * 32 + 17],ae1_fd->cur_info.rect[i * 32 + 18],ae1_fd->cur_info.rect[i * 32 + 19],
        ae1_fd->cur_info.rect[i * 32 + 20],ae1_fd->cur_info.rect[i * 32 + 21],ae1_fd->cur_info.rect[i * 32 + 22],ae1_fd->cur_info.rect[i * 32 + 23],
        ae1_fd->cur_info.rect[i * 32 + 24],ae1_fd->cur_info.rect[i * 32 + 25],ae1_fd->cur_info.rect[i * 32 + 26],ae1_fd->cur_info.rect[i * 32 + 27],
        ae1_fd->cur_info.rect[i * 32 + 28],ae1_fd->cur_info.rect[i * 32 + 29],ae1_fd->cur_info.rect[i * 32 + 30],ae1_fd->cur_info.rect[i * 32 + 31]
        );
    }
*/
	
	return rtn;
}
	// add end
static int32_t _fd_process(struct ae_ctrl_cxt *cxt, void *param)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_fd_param *fd = (struct ae_fd_param *)param;

	if (NULL == cxt || NULL == param) {
		AE_LOGE("cxt %p param %p param is error!", cxt, param);
		return AE_ERROR;
	}

	if (!(cxt->fdae.allow_to_work && cxt->fdae.enable)) {
		cxt->fdae.face_info.face_num = 0;
		return rtn;
	}
	AE_LOGD("fd num is %d",fd->face_num);
	if (fd->face_num > 0) {
		memcpy(&(cxt->fdae.face_info), fd, sizeof(struct ae_fd_param));
	} else {
		cxt->fdae.face_info.face_num = 0;
		memset(&(cxt->fdae.face_info), 0, sizeof(struct ae_fd_param));
	}
	// add by matchbox for fd_ae
	_fd_info_pre_set(cxt);
	_fd_info_set(cxt);
	// add end;
	AE_LOGD("FDAE: sx %d sy %d", fd->face_area[0].rect.start_x, fd->face_area[0].rect.start_y);
	AE_LOGD("FDAE: ex %d ey %d", fd->face_area[0].rect.end_x, fd->face_area[0].rect.end_y);
	return rtn;
}
/*  END: FDAE related functions  */
/**************************************************************************/

static int32_t _get_skip_frame_num(struct ae_ctrl_cxt *cxt, void *param, void *result)
{
	int32_t rtn = AE_SUCCESS;
	int32_t frame_num = 0;

	frame_num = cxt->monitor_unit.cfg.skip_num;

	if (result) {
		int32_t *skip_num = (int32_t *) result;

		*skip_num = frame_num;
	}

	AE_LOGD("skip frame num=%d", frame_num);
	return rtn;
}

static int32_t _get_normal_info(struct ae_ctrl_cxt *cxt, void *result)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_normal_info *info_ptr = (struct ae_normal_info *)result;
	info_ptr->exposure = cxt->cur_result.wts.exposure_time;
	info_ptr->fps = cxt->cur_result.wts.cur_fps;
	info_ptr->stable = cxt->cur_result.wts.stable;
	return rtn;
}

static int32_t _set_sensor_exp_time(struct ae_ctrl_cxt *cxt, void *param)
{
	int32_t rtn = AE_SUCCESS;
	uint32_t exp_time = *(uint32_t *) param;
	uint32_t line_time = cxt->snr_info.line_time;

	if (AE_STATE_LOCKED == cxt->cur_status.settings.lock_ae) {
		cxt->cur_status.settings.exp_line = exp_time * 10 / line_time;
	}
	return rtn;
}

static int32_t _set_sensor_sensitivity(struct ae_ctrl_cxt *cxt, void *param)
{
	int32_t rtn = AE_SUCCESS;
	uint32_t sensitivity = *(uint32_t *) param;

	if (AE_STATE_LOCKED == cxt->cur_status.settings.lock_ae) {
		cxt->cur_status.settings.gain = sensitivity * 128 / 50;
	}
	return rtn;
}

static int32_t _set_pause(struct ae_ctrl_cxt *cxt)
{
	int32_t ret = AE_SUCCESS;
	
	cxt->cur_status.settings.lock_ae = AE_STATE_PAUSE;
	cxt->cur_status.settings.pause_cnt++;
	return ret;
}

static int32_t _set_restore_cnt(struct ae_ctrl_cxt *cxt)
{
	int32_t ret = AE_SUCCESS;
	if (2 > cxt->cur_status.settings.pause_cnt){
		cxt->cur_status.settings.lock_ae = AE_STATE_NORMAL;
		cxt->cur_status.settings.pause_cnt = 0;
	}else{
		cxt->cur_status.settings.pause_cnt--;
	}
	AE_LOGD("PAUSE COUNT IS %d",cxt->cur_status.settings.pause_cnt);	
	return ret;
}

static int32_t ae_set_magic_tag(struct debug_ae_param *param_ptr)
{
	int32_t rtn = AE_SUCCESS;
	uint32_t len = 0;

	len = strlen(AE_MAGIC_TAG);
	if (len > sizeof(param_ptr->magic)) {
		AE_LOGE("magic tag buf is not proper\n");
		return AE_ERROR;
	}
	memcpy((void *)&param_ptr->magic[0], AE_MAGIC_TAG, len);
	return rtn;
}

	// wanghao @2015-12-22
static int32_t ae_get_debug_info(struct ae_ctrl_cxt *cxt, void *result)
{
	int32_t rtn = AE_SUCCESS;
	uint32_t len = 0;
#if 0
	struct debug_ae_param *param = (struct debug_ae_param *)result;
	rtn = ae_set_magic_tag(param);
	if (AE_SUCCESS != rtn) {
		goto DEBUG_INFO_EXIT;
	}
	param->version = AE_DEBUG_VERSION_ID;
	memcpy(param->alg_id, cxt->alg_id, sizeof(param->alg_id));
	param->lock_status = cxt->sync_cur_status.settings.lock_ae;
	param->FD_AE_status = cxt->fdae.enable;
	param->cur_lum = cxt->sync_cur_result.cur_lum;
	param->target_lum = cxt->sync_cur_result.target_lum;
	param->target_zone = cxt->sync_cur_result.target_zone;
	param->target_zone_ori = cxt->sync_cur_result.target_zone_ori;
	param->max_index = cxt->sync_cur_status.ae_table->max_index;
	param->cur_index = cxt->sync_cur_result.wts.cur_index;
	param->min_index = cxt->sync_cur_status.ae_table->min_index;
	param->max_fps = cxt->sync_cur_status.settings.max_fps;
	param->min_fps = cxt->sync_cur_status.settings.min_fps;
	param->cur_fps = cxt->sync_cur_result.wts.cur_fps;
	param->cur_ev = cxt->sync_cur_status.settings.ev_index;
	param->cur_bv = cxt->sync_cur_result.cur_bv;
	param->line_time = cxt->sync_cur_status.line_time;
	param->cur_exp_line = cxt->sync_cur_result.wts.cur_exp_line;
	param->exposure_time = cxt->sync_cur_result.wts.exposure_time;
	param->cur_dummy = cxt->sync_cur_result.wts.cur_dummy;
	param->frame_line = cxt->sync_cur_result.wts.cur_exp_line + cxt->sync_cur_result.wts.cur_dummy;
	param->cur_again = cxt->sync_cur_result.wts.cur_again;
	param->cur_dgain = cxt->sync_cur_result.wts.cur_dgain;
	param->cur_iso = cxt->sync_cur_status.settings.iso;
	param->cur_flicker = cxt->sync_cur_status.settings.flicker;
	param->cur_scene = cxt->sync_cur_status.settings.scene_mode;
	param->flash_effect = cxt->sync_cur_result.flash_effect;
	param->is_stab = cxt->sync_cur_result.wts.stable;
	param->cur_work_mode = cxt->sync_cur_status.settings.work_mode;
	param->cur_metering_mode = (uint32_t) cxt->sync_cur_status.settings.metering_mode;	// metering_mode
	/* Bethany
	   * add  some  touch  info  to  debug  info 
	 */
	param->TC_AE_status = cxt->sync_cur_status.settings.touch_tuning_enable;
	/* Bethany 
	   *add some  touch  info  to  debug  info
	 */
	param->TC_cur_lum = cxt->sync_cur_result.ptc->result_touch_ae.touch_roi_lum;
	/* Bethany 
	   *add some touch info to debug info 
	 */
	param->TC_target_offset = cxt->sync_cur_result.ptc->result_touch_ae.tar_offset;
	param->mulaes_target_offset = cxt->sync_cur_result.pmulaes->result_mulaes.tar_offset;
	param->region_target_offset = cxt->sync_cur_result.pregion->result_region.tar_offset_u\
								+ cxt->sync_cur_result.pregion->result_region.tar_offset_d;
	param->flat_target_offset = cxt->sync_cur_result.pflat->result_flat.tar_offset;

	// memcpy((void*)&param->histogram[0],
	// (void*)cxt->sync_cur_result.histogram, sizeof(uint32_t) 
	// 
	// * 256);
	memcpy((void *)&param->r_stat_info[0], (void *)&cxt->sync_aem[0], sizeof(uint32_t) * 1024);
	memcpy((void *)&param->g_stat_info[0], (void *)&cxt->sync_aem[1024], sizeof(uint32_t) * 1024);
	memcpy((void *)&param->b_stat_info[0], (void *)&cxt->sync_aem[2048], sizeof(uint32_t) * 1024);

	len = sizeof(*(cxt->sync_cur_result.pregion)) + sizeof(*(cxt->sync_cur_result.pflat)) + sizeof(*(cxt->sync_cur_result.pmulaes));
	/*
	 * AE_LOGD("total_len %d", sizeof(struct debug_ae_param));
	 * AE_LOGD("total_i-metering_len %d prl %d pfl %d pml
	 * %d\r\n", len, sizeof(*(cxt->sync_cur_result.pregion)),
	 * sizeof(*(cxt->sync_cur_result.pflat)),
	 * sizeof(*(cxt->sync_cur_result.pmulaes)));
	 * AE_LOGD("total_i-metering_region %d\r\n",
	 * sizeof(cxt->sync_cur_result.pregion->in_region)); 
	 */
	if (len < sizeof(param->reserved)) {
		memcpy((void *)&param->reserved[0], (void *)cxt->sync_cur_result.pregion, sizeof(*(cxt->sync_cur_result.pregion)));

		len = sizeof(*(cxt->sync_cur_result.pregion)) / sizeof(uint32_t);
		memcpy((void *)&param->reserved[len], (void *)cxt->sync_cur_result.pflat, sizeof(*(cxt->sync_cur_result.pflat)));
		// param->r_stat_info;
		len = len + (sizeof(*(cxt->sync_cur_result.pflat)) / sizeof(uint32_t));
		memcpy((void *)&param->reserved[len], (void *)cxt->sync_cur_result.pmulaes, sizeof(*(cxt->sync_cur_result.pmulaes)));
	}
	
DEBUG_INFO_EXIT:

#else

	struct ae_debug_info_packet_in debug_info_in;
	struct ae_debug_info_packet_out debug_info_out;
	char *alg_id_ptr = NULL;
	struct tg_ae_ctrl_alc_log *debug_info_result = (struct tg_ae_ctrl_alc_log*)result;

	memset((void*)&debug_info_in, 0, sizeof(struct ae_debug_info_packet_in));
	memset((void*)&debug_info_out, 0, sizeof(struct ae_debug_info_packet_out));
	alg_id_ptr = ae_debug_info_get_lib_version();
	debug_info_in.aem_stats = (void*)cxt->sync_aem;
	debug_info_in.alg_status = (void*)&cxt->sync_cur_status;
	debug_info_in.alg_results = (void*)&cxt->sync_cur_result;
	debug_info_in.packet_buf = (void*)&cxt->debug_info_buf[0];
	memcpy((void*)&debug_info_in.id[0], alg_id_ptr, sizeof(debug_info_in.id));

	rtn = ae_debug_info_packet((void*)&debug_info_in, (void*)&debug_info_out);
	debug_info_result->log = (uint8_t*)debug_info_in.packet_buf;
	debug_info_result->size = debug_info_out.size;

#endif

	return rtn;
}

// wanghao @2015-12-23
static int32_t ae_get_debug_info_for_display(struct ae_ctrl_cxt *cxt, void *result)
{
	int32_t rtn = AE_SUCCESS;
	struct debug_ae_display_param *emParam = (struct debug_ae_display_param *)result;
	emParam->version = AE_DEBUG_VERSION_ID;
	emParam->lock_status = cxt->cur_status.settings.lock_ae;
	emParam->FD_AE_status = cxt->fdae.enable;
	emParam->cur_lum = cxt->cur_result.cur_lum;
	emParam->target_lum = cxt->cur_result.target_lum;
	emParam->target_zone = cxt->cur_param->target_lum_zone;;
	emParam->max_index = cxt->cur_status.ae_table->max_index;
	emParam->cur_index = cxt->cur_result.wts.cur_index;
	emParam->min_index = cxt->cur_status.ae_table->min_index;
	emParam->max_fps = cxt->cur_status.settings.max_fps;
	emParam->min_fps = cxt->cur_status.settings.max_fps;
	emParam->exposure_time = cxt->cur_result.wts.exposure_time;
	emParam->cur_ev = cxt->cur_status.settings.ev_index;
	emParam->cur_exp_line = cxt->cur_result.wts.cur_exp_line;
	emParam->cur_dummy = cxt->cur_result.wts.cur_dummy;
	emParam->cur_again = cxt->cur_result.wts.cur_again;
	emParam->cur_dgain = cxt->cur_result.wts.cur_dgain;
	emParam->cur_iso = cxt->cur_status.settings.iso;
	emParam->cur_flicker = cxt->cur_status.settings.flicker;
	emParam->cur_scene = cxt->cur_status.settings.scene_mode;
	emParam->flash_effect = cxt->cur_status.settings.flash_ration;
	emParam->is_stab = cxt->cur_result.wts.stable;
	emParam->line_time = cxt->cur_status.line_time;
	emParam->frame_line = cxt->cur_result.wts.cur_exp_line + cxt->cur_result.wts.cur_dummy;
	emParam->cur_work_mode = cxt->cur_status.settings.work_mode;
	emParam->cur_bv = cxt->sync_cur_result.cur_bv;
	emParam->cur_fps = cxt->cur_result.wts.cur_fps;

	emParam->cur_metering_mode = (uint32_t) cxt->cur_status.settings.metering_mode;	// metering_mode
	return rtn;
}

static int32_t make_isp_result(struct ae_alg_calc_result *alg_rt, struct ae_calc_out *result)
{
	int32_t rtn = AE_SUCCESS;

	result->cur_lum = alg_rt->cur_lum;
	result->cur_again = alg_rt->wts.cur_again;
	result->cur_exp_line = alg_rt->wts.cur_exp_line;
	result->line_time = alg_rt->wts.exposure_time / alg_rt->wts.cur_exp_line;
	result->is_stab = alg_rt->wts.stable;
	return rtn;
}

void *ae_sprd_init(struct ae_init_in *param, void *in_param)
{
	int32_t rtn = AE_SUCCESS;
	int32_t i 	= 0;
	void *seq_handle = NULL;
	struct ae_ctrl_cxt	*cxt 	= NULL;
	struct ae_in_out	*reserve = NULL;
	struct ae_misc_init_in misc_init_in = { 0 };
	struct ae_misc_init_out misc_init_out = { 0 };
	struct seq_init_in init_in = { 0 };
	struct ae_set_work_param work_param = { 0x00 };

	AE_LOGD("V2_INIT ST\r\n");
	cxt = (struct ae_ctrl_cxt *)malloc(sizeof(struct ae_ctrl_cxt));

	if (NULL == cxt) {
		rtn = AE_ALLOC_ERROR;
		AE_LOGE("alloc is error!");
		goto ERR_EXIT;
	}

	memset(cxt, 0, sizeof(*cxt));

	if (NULL == param) {
		AE_LOGE("input param is error! %p\r\n", param);
		rtn = AE_ERROR;
		goto ERR_EXIT;
	}

	work_param.mode = AE_WORK_MODE_COMMON;
	work_param.fly_eb = 1;
	work_param.resolution_info = param->resolution_info;

	/*AE_LOGD("resol w %d h %d lt %d",
			work_param.resolution_info.frame_size.w,
			work_param.resolution_info.frame_size.h,
			work_param.resolution_info.line_time);*/

	cxt->cur_status.ae_initial = AE_PARAM_INIT;
	rtn = _set_ae_param(cxt, param, &work_param, AE_PARAM_INIT);
	// init fd_info add by matchbox for fd_ae
	rtn = _fd_info_init(cxt);

	/*
	 * create write/effective E&G thread 
	 */
	rtn = cmr_thread_create(&cxt->thread_handle, AE_THREAD_QUEUE_NUM, (msg_process) ae_write_thread_proc, (void *)cxt);
	if (rtn) {
		AE_LOGE("create thread failed");
		goto ERR_EXIT;
	}

	init_in.exp_valid_num = cxt->exp_skip_num;
	init_in.gain_valid_num = cxt->gain_skip_num;
	init_in.preview_skip_num = cxt->preview_skip_num;//hu
	init_in.capture_skip_num = cxt->capture_skip_num;//hu
	AE_LOGD("D-gain--exp_valid_num =%d, gain_valid_num =%d, preview_skip_num =%d, capture_skip_num =%d",\
			init_in.exp_valid_num, init_in.gain_valid_num, init_in.preview_skip_num, init_in.capture_skip_num);

	seq_init(AE_WRITE_QUEUE_NUM, &init_in, &seq_handle);
	cxt->seq_handle = seq_handle;
	/*
	 * create AE calc E&G queue 
	 */
	rtn = ae_calc_result_queue_init(&cxt->ae_result_queue);
	if (rtn) {
		AE_LOGE("ae_calc_result_queue_init param is failed!");
		goto ERR_EXIT;
	}
	/*
	 * create ISP gain queue 
	 */
	isp_gain_buffer_init(cxt);

	cxt->bypass = param->has_force_bypass;
	if (param->has_force_bypass) {
		cxt->cur_param->touch_info.enable = 0;
		cxt->cur_param->face_param.face_tuning_enable = 0;
	}
	cxt->debug_enable = _is_ae_mlog(cxt);
	cxt->cur_status.mlog_en = cxt->debug_enable;
	misc_init_in.alg_id			= cxt->cur_status.alg_id;
	misc_init_in.start_index	= cxt->cur_status.start_index;
	misc_init_in.param_ptr		= &cxt->cur_status;
	misc_init_in.size			= sizeof(cxt->cur_status);
	cxt->misc_handle = ae_misc_init(&misc_init_in, &misc_init_out);
	memcpy(cxt->alg_id, misc_init_out.alg_id, sizeof(cxt->alg_id));

	pthread_mutex_init(&cxt->data_sync_lock, NULL);
	/*
	 * update the sensor related information to cur_result
	 * structure 
	 */
	cxt->cur_result.wts.stable = 0;
	cxt->cur_result.wts.cur_dummy = 0;
	cxt->cur_result.wts.cur_index = cxt->cur_status.start_index;
	cxt->cur_result.wts.cur_again = cxt->ae_result.gain;
	cxt->cur_result.wts.cur_dgain = 0;
	cxt->cur_result.wts.cur_exp_line = cxt->cur_status.ae_table->exposure[cxt->cur_status.start_index];
	cxt->cur_result.wts.exposure_time = cxt->cur_status.ae_table->exposure[cxt->cur_status.start_index] * cxt->snr_info.line_time;

	/*
	 * read info from last ae_deinit 
	 */
	if (NULL != in_param && 1 == ((struct ae_in_out *)in_param)->enable){
		reserve = (struct ae_in_out *)in_param;
		cxt->ae_result.expline 	= reserve->cur_exp_line;
		cxt->ae_result.gain 	= reserve->cur_gain;
		cxt->ae_result.dummy 	= reserve->cur_dummy;
	}
	/*
	 * init for video start/stop 
	 */
	cxt->last_expline = 0;
	cxt->last_dummy = 0;
	cxt->last_aegain = 0;
	cxt->last_linetime = 0;
	cxt->last_enable = 0;

	/*
	 * CMR_MSG_INIT(msg); msg.msg_type = AE_WRITE_EXP_GAIN;
	 * //msg.sync_flag = CMR_MSG_SYNC_PROCESSED; //init
	 * sensor: need write two times rtn =
	 * cmr_thread_msg_send(cxt->thread_handle, &msg); rtn =
	 * cmr_thread_msg_send(cxt->thread_handle, &msg); 
	 */
	return (void *)cxt;
ERR_EXIT:
	if (NULL != cxt) {
		free(cxt);
		cxt = NULL;
	}

	return NULL;
}

static int32_t _get_flicker_switch_flag(struct ae_ctrl_cxt *cxt, void *in_param)
{
	int32_t rtn = AE_SUCCESS;
	uint32_t cur_exp = 0;
	uint32_t *flag = (uint32_t *) in_param;

	cur_exp = cxt->sync_cur_result.wts.exposure_time;
	// 50Hz/60Hz
	if (AE_FLICKER_50HZ == cxt->cur_status.settings.flicker) {
		if (cur_exp < 83333) {
			*flag = 0;
		} else {
			*flag = 1;
		}
	} else {
		if (cur_exp < 83333) {
			*flag = 0;
		} else {
			*flag = 1;
		}
	}

	AE_LOGD("ANTI_FLAG: %d, %d, %d", cur_exp, cxt->snr_info.line_time, *flag);
	return rtn;
}

static void _set_led(struct ae_ctrl_cxt *cxt){
	char str[50];
	int16_t i = 0;
	int16_t j = 0;
	int8_t led_ctl[2] = {0, 0};
	struct ae_flash_element tmp;
	property_get("persist.sys.isp.ae.led", str, "");

	if ('\0' == str[i]){
		return;
	}else{
		while(' ' == str[i])
			i++;

		while(('0' <= str[i] && '9' >= str[i]) || ' ' == str[i]){
			if (' ' == str[i]){
				if (' ' == str[i + 1]){
					;
				}else{
					if (j > 0)
						j = 1;
					else
						j++;
				}
			}else{
				led_ctl[j] = 10 * led_ctl[j] + str[i] - '0';
			}
			i++;
		}
		//AE_LOGD("isp_set_led: %d\r\n", led_ctl[0]);
		cxt->isp_ops.flash_set_charge(cxt->isp_ops.isp_handler, led_ctl[0], &tmp);
	}
	return;
}

static void ae_hdr_ctrl(struct ae_ctrl_cxt *cxt, struct ae_calc_in * param){
	cxt->hdr_up = cxt->hdr_down = 25;
	if (3 == cxt->hdr_flag){/*EV-*/
		cxt->hdr_flag--;
		cxt->hdr_timestamp = (uint64_t)param->sec * 1000000 + (uint64_t)param->usec;
		cxt->cur_status.settings.lock_ae = AE_STATE_LOCKED;
		cxt->cur_status.settings.manual_mode = 1;
		cxt->cur_status.settings.table_idx = cxt->hdr_base_ae_idx - cxt->hdr_down;
		AE_LOGD("_isp_hdr_3: %d sec_%d usec_%d %llu\n",
				cxt->cur_status.settings.table_idx, param->sec, param->usec, cxt->hdr_timestamp);
	}else if(2 == cxt->hdr_flag){/*EV+*/
		cxt->cur_status.settings.lock_ae = AE_STATE_LOCKED;
		cxt->cur_status.settings.manual_mode = 1;
		cxt->cur_status.settings.table_idx = cxt->hdr_base_ae_idx + cxt->hdr_up;
		cxt->hdr_flag--;
		AE_LOGD("_isp_hdr_2: %d\n", cxt->cur_status.settings.table_idx);
	}else if(1 == cxt->hdr_flag){/*EV0*/
		cxt->cur_status.settings.lock_ae = AE_STATE_LOCKED;
		cxt->cur_status.settings.manual_mode = 1;
		cxt->cur_status.settings.table_idx = cxt->hdr_base_ae_idx;
		cxt->hdr_flag--;
		AE_LOGD("_isp_hdr_1: %d\n", cxt->cur_status.settings.table_idx);
	}else{
		;
	}
	return;
}

int32_t ae_sprd_calculation(void *handle, struct ae_calc_in * param, struct ae_calc_out * result)
{
	int32_t rtn = AE_ERROR;
	int32_t i = 0;
	struct ae_ctrl_cxt *cxt = NULL;
	struct ae_alg_calc_param *current_status;
	struct ae_alg_calc_param *alg_status_ptr =NULL;//DEBUG
	struct ae_alg_calc_result *current_result;
	struct ae_calc_result rt;
	struct ae_misc_calc_in misc_calc_in = { 0 };
	struct ae_misc_calc_out misc_calc_out = { 0 };
	CMR_MSG_INIT(msg);

	if ((NULL == param) || (NULL == result)) {
		AE_LOGE("in %p out %p param is error!", param, result);
		return AE_PARAM_NULL;
	}

	rtn = _check_handle(handle);
	AE_RETURN_IF_FAIL(rtn, ("handle = %p", handle));
	cxt = (struct ae_ctrl_cxt *)handle;

	pthread_mutex_lock(&cxt->data_sync_lock);

	if (cxt->bypass) {
		_set_pause(cxt);
	}

	current_status = &cxt->sync_cur_status;
	current_result = &cxt->sync_cur_result;

	// acc_info_print(cxt);
	cxt->cur_status.awb_gain.b = param->awb_gain_b;
	cxt->cur_status.awb_gain.g = param->awb_gain_g;
	cxt->cur_status.awb_gain.r = param->awb_gain_r;
	memcpy(cxt->sync_aem, param->stat_img, 3 * 1024 * sizeof(uint32_t));
	cxt->cur_status.stat_img = cxt->sync_aem;
	// get effective E&g
	if ((current_status->ae_start_delay + 1) >= cxt->cur_status.frame_id){
		cxt->cur_status.effect_expline = cxt->actual_cell[0].expline;
		cxt->cur_status.effect_gain = cxt->actual_cell[0].gain;
		cxt->cur_status.effect_dummy = cxt->actual_cell[0].dummy;
	}else{
		cxt->cur_status.effect_expline = cxt->actual_cell[cxt->cur_status.frame_id % 20].expline;
		cxt->cur_status.effect_gain = cxt->actual_cell[cxt->cur_status.frame_id % 20].gain;
		cxt->cur_status.effect_dummy = cxt->actual_cell[cxt->cur_status.frame_id % 20].dummy;	
		/*
			due to set_scene_mode just be called in ae_sprd_calculation,
			and the prv_status just save the normal scene status
		*/
		if (AE_SCENE_NORMAL == cxt->sync_cur_status.settings.scene_mode) {
			cxt->prv_status = cxt->cur_status;
			if (AE_SCENE_NORMAL != cxt->cur_status.settings.scene_mode) {
				cxt->prv_status.settings.scene_mode  = AE_SCENE_NORMAL;
			}
		}
	}
	
	cxt->cur_result.face_lum = current_result->face_lum;//for debug face lum
	cxt->sync_aem[3 * 1024] = cxt->cur_status.frame_id;
	cxt->sync_aem[3 * 1024 + 1] = cxt->cur_status.effect_expline;
	cxt->sync_aem[3 * 1024 + 2] = cxt->cur_status.effect_dummy;
	cxt->sync_aem[3 * 1024 + 3] = cxt->cur_status.effect_gain;
	
//START
    alg_status_ptr = &cxt->cur_status;
    cxt->cur_param = &cxt->tuning_param[0];
    //alg_status_ptr = &cxt->cur_status;
    // change weight_table
    alg_status_ptr->weight_table = cxt->cur_param->weight_table[alg_status_ptr->settings.metering_mode].weight;
    // change ae_table
#if 0
    current_status->ae_table = &cxt->cur_param->scene_info[current_status->settings.scene_mode].ae_table[current_status->settings.flicker];
#else
    // for now video using
    alg_status_ptr->ae_table = &cxt->cur_param->ae_table[alg_status_ptr->settings.flicker][AE_ISO_AUTO];
    alg_status_ptr->ae_table->min_index = 0;//AE table start index = 0
#endif
    // change settings related by EV
    alg_status_ptr->target_lum = _calc_target_lum(cxt->cur_param->target_lum, cxt->cur_status.settings.ev_index, &cxt->cur_param->ev_table);
    alg_status_ptr->target_lum_zone = cxt->stable_zone_ev[alg_status_ptr->settings.ev_index];
    alg_status_ptr->stride_config[0] = cxt->cnvg_stride_ev[alg_status_ptr->settings.ev_index * 2];
    alg_status_ptr->stride_config[1] = cxt->cnvg_stride_ev[alg_status_ptr->settings.ev_index * 2 + 1];
    {
        int8_t cur_mod = cxt->sync_cur_status.settings.scene_mode;
        int8_t nx_mod = cxt->cur_status.settings.scene_mode;
        if (nx_mod != cur_mod) {
            AE_LOGD("before set scene mode: \n");
            _printf_status_log(cxt, cur_mod, &cxt->cur_status);
            _set_scene_mode(cxt, cur_mod, nx_mod);
            AE_LOGD("after set scene mode: \n");
            _printf_status_log(cxt, nx_mod, &cxt->cur_status);
        }
    }
    memcpy(current_status, &cxt->cur_status, sizeof(struct ae_alg_calc_param));
    memcpy(&cxt->cur_result, current_result, sizeof(struct ae_alg_calc_result));
//END	
/*
	{
		int8_t cur_mod = cxt->sync_cur_status.settings.scene_mode;
		int8_t nx_mod = cxt->cur_status.settings.scene_mode;
		if (nx_mod != cur_mod) {
			AE_LOGD("before set scene mode: \n");
			_printf_status_log(cxt, cur_mod, &cxt->cur_status);
			_set_scene_mode(cxt, cur_mod, nx_mod);
			AE_LOGD("after set scene mode: \n");
			_printf_status_log(cxt, nx_mod, &cxt->cur_status);
			}
		}
	memcpy(current_status, &cxt->cur_status, sizeof(struct ae_alg_calc_param));
	memcpy(&cxt->cur_result, current_result, sizeof(struct ae_alg_calc_result));
	cxt->cur_param = &cxt->tuning_param[0];
	// change weight_table
	current_status->weight_table = cxt->cur_param->weight_table[current_status->settings.metering_mode].weight;
	// change ae_table
#if 0
	current_status->ae_table = &cxt->cur_param->scene_info[current_status->settings.scene_mode].ae_table[current_status->settings.flicker];
#else
	// for now video using
	current_status->ae_table = &cxt->cur_param->ae_table[current_status->settings.flicker][AE_ISO_AUTO];
	current_status->ae_table->min_index = 0;//AE table start index = 0
#endif
	// change settings related by EV
	current_status->target_lum = _calc_target_lum(cxt->cur_param->target_lum, cxt->cur_status.settings.ev_index, &cxt->cur_param->ev_table);
	current_status->target_lum_zone = cxt->stable_zone_ev[current_status->settings.ev_index];
	current_status->stride_config[0] = cxt->cnvg_stride_ev[current_status->settings.ev_index * 2];
	current_status->stride_config[1] = cxt->cnvg_stride_ev[current_status->settings.ev_index * 2 + 1];

*/	
	// AE_LOGD("e_expline %d e_gain %d\r\n",
	// current_status.effect_expline,
	// current_status.effect_gain);
	// skip the first aem data (error data)
	if (current_status->ae_start_delay <= current_status->frame_id) {
		misc_calc_in.sync_settings = current_status;
		misc_calc_out.ae_output = &cxt->cur_result;
		// AE_LOGD("ae_flash_status calc %d",
		// current_status.settings.flash);
		// AE_LOGD("fd_ae: updata_flag:%d ef %d",
		// current_status->ae1_finfo.update_flag,
		// current_status->ae1_finfo.enable_flag);

		//4test
		//_set_led(cxt);
		ae_hdr_ctrl(cxt, param);
		//4test

		rtn = ae_misc_calculation(cxt->misc_handle, &misc_calc_in, &misc_calc_out);
		cxt->cur_status.ae1_finfo.update_flag = 0;	// add by match box for fd_ae reset the status
		memset((void*)&cxt->cur_status.ae1_finfo.cur_info, 0, sizeof(cxt->cur_status.ae1_finfo.cur_info));
		
		memcpy(current_result, &cxt->cur_result, sizeof(struct ae_alg_calc_result));
		make_isp_result(current_result, result);
		{
			/*just for debug: reset the status */
			if (1 == cxt->cur_status.settings.touch_scrn_status) {
				cxt->cur_status.settings.touch_scrn_status = 0;
			}
		}
		// AE_LOGD("calc_module_f %.2f %d\r\n",
		// current_result->pflat->result_flat.degree,
		// current_result->pflat->result_flat.tar_offset);
		// wait-pause function
		if (AE_STATE_WAIT_PAUSE == cxt->cur_status.settings.lock_ae && 1 == cxt->cur_result.wts.stable) {
			cxt->cur_status.settings.lock_ae = AE_STATE_PAUSE;
		}
		// check flash
		if (FLASH_PRE_BEFORE_RECEIVE == cxt->cur_result.flash_status && FLASH_PRE_BEFORE == current_status->settings.flash) {
			AE_LOGD("ae_flash_status shake_1");
			if (0 == cxt->send_once[0]){
				cxt->send_once[0]++;
				(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_CONVERGED);
				AE_LOGD("ae_flash_callback do-pre-open!\r\n");
			}
		}

		if (FLASH_PRE_RECEIVE == cxt->cur_result.flash_status && FLASH_PRE == current_status->settings.flash) {
			AE_LOGD("ae_flash_status shake_2 %d %d %d", cxt->cur_result.wts.stable, cxt->cur_result.cur_lum, cxt->cur_result.flash_effect);
			if (cxt->cur_result.wts.stable){
				if (0 == cxt->send_once[1]){
					cxt->send_once[1]++;
					(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_CONVERGED);
					AE_LOGD("ae_flash_callback do-pre-close!\r\n");
				}
				cxt->flash_effect = cxt->cur_result.flash_effect;
			}
			cxt->send_once[0] = 0;
		}

		if (FLASH_PRE_AFTER_RECEIVE == cxt->cur_result.flash_status && FLASH_PRE_AFTER == current_status->settings.flash) {
			AE_LOGD("ae_flash_status shake_3 %d", cxt->cur_result.flash_effect);
			cxt->cur_status.settings.flash = FLASH_NONE;
			cxt->send_once[1] = 0;
		}

		if (FLASH_MAIN_RECEIVE == cxt->cur_result.flash_status && FLASH_MAIN == current_status->settings.flash) {
			AE_LOGD("ae_flash_status shake_5 %d %d", cxt->cur_result.wts.stable, cxt->cur_result.cur_lum);
			if (cxt->cur_result.wts.stable){
				if (0 == cxt->send_once[2]){
					cxt->send_once[2]++;
					(*cxt->isp_ops.callback) (cxt->isp_ops.isp_handler, AE_CB_CONVERGED);
					AE_LOGD("ae_flash_callback do-capture!\r\n");
				}
			}
		}

		if (FLASH_MAIN_AFTER_RECEIVE == cxt->cur_result.flash_status && FLASH_MAIN_AFTER == current_status->settings.flash) {
			AE_LOGD("ae_flash_status shake_6");
			cxt->cur_status.settings.flash = FLASH_NONE;
			cxt->send_once[0] = cxt->send_once[1] = cxt->send_once[2] = 0;
		}

		rt.expline = cxt->cur_result.wts.cur_exp_line;
		rt.gain    = cxt->cur_result.wts.cur_again;
		rt.dummy   = cxt->cur_result.wts.cur_dummy;		
		msg.msg_type = AE_WRITE_EXP_GAIN;
	}else{ 
		rt.expline = cxt->actual_cell[0].expline;
		rt.gain    =cxt->actual_cell[0].gain;
		rt.dummy   = cxt->actual_cell[0].dummy;
		msg.msg_type = AE_WRITE_RECORD;
	}
	
/***********************************************************/
/*update parameters to sensor*/
	ae_calc_result_queue_write(&cxt->ae_result_queue, &rt);	
	cxt->sof_id++;
	//AE_LOGD("sof_handler %d", cxt->sof_id);
	rtn = ae_calc_result_queue_read(&cxt->ae_result_queue, &cxt->ae_result);
	if (rtn) {
		AE_LOGE("ae_calc_result_queue_read error");
		rtn = AE_ERROR;
		goto ERROR_EXIT;
	}

	cxt->sensor_calc_item.cell.exp_line = cxt->ae_result.expline;
	cxt->sensor_calc_item.cell.gain 	= cxt->ae_result.gain;
	cxt->sensor_calc_item.cell.dummy 	= cxt->ae_result.dummy;
	rtn = cmr_thread_msg_send(cxt->thread_handle, &msg);
	
/***********************************************************/
/******bethany lock ae*******
  *****touch have 3 states,0:touch before/release;1:touch doing; 2: toch done and AE stable*****/
    AE_LOGD("TCCTL_tcAE_status and ae_stable is %d,%d",current_result->tcAE_status,current_result->wts.stable);
	if(1 == current_result->tcAE_status && 1 == current_result->wts.stable){
		AE_LOGD("TC_start lock ae");
		//AE_LOGD("TC_pause num is %d",cxt->cur_status.settings.pause_cnt);
		rtn = _set_pause(cxt);
		current_result->tcAE_status = 2;
	}
	cxt->cur_status.to_ae_state = current_result->tcAE_status;
	//AE_LOGD("TCCTL_to_AE_state is %d",cxt->cur_status.to_ae_state);
	AE_LOGD("TCCTL_rls_cond is %d,%d",current_result->tcAE_status,current_result->tcRls_flag);
	if(0 == current_result->tcAE_status && 1 == current_result->tcRls_flag){
		rtn = _set_restore_cnt(cxt);
		AE_LOGD("TC_start release lock ae");
		current_result->tcRls_flag = 0;
	}
	AE_LOGD("TCCTL_rls_ae_lock is %d",cxt->cur_status.settings.lock_ae);

/***********************************************************/
/*display the AE running status*/
	if (1 == cxt->debug_enable) {
		save_to_mlog_file(cxt, &misc_calc_out);
	}	
/***********************************************************/
	cxt->cur_status.frame_id++;

	//AE_LOGD("AE_V2_frame id = %d\r\n", cxt->cur_status.frame_id);
	//AE_LOGD("rt_expline %d rt_gain %d rt_dummy %d\r\n", rt.expline, rt.gain, rt.dummy);

	cxt->cur_status.ae_initial = AE_PARAM_NON_INIT;
ERROR_EXIT:
	pthread_mutex_unlock(&cxt->data_sync_lock);
	return rtn;
}

int32_t ae_sprd_io_ctrl(void *handle, enum ae_io_ctrl_cmd cmd, void *param, void *result)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_ctrl_cxt *cxt = NULL;

	rtn = _check_handle(handle);
	if (AE_SUCCESS != rtn) {
		AE_RETURN_IF_FAIL(rtn, ("handle = %p", handle));
	}

	cxt = (struct ae_ctrl_cxt *)handle;
	//AE_LOGD("AE_IO_CMD %d", cmd);
	pthread_mutex_lock(&cxt->data_sync_lock);
	switch (cmd) {
	case AE_SET_PROC:
		_sof_handler(cxt);
		break;

	case AE_SET_DC_DV:
		if (param) {
			if (1 == *(uint32_t *) param)
				cxt->cur_status.settings.work_mode = AE_WORK_MODE_VIDEO;
			else
				cxt->cur_status.settings.work_mode = AE_WORK_MODE_COMMON;
		}
		//AE_LOGD("AE_SET_DC_DV %d", cxt->cur_status.settings.work_mode);
		break;

	case AE_SET_WORK_MODE:
		break;

	case AE_SET_SCENE_MODE:
		if (param) {
			struct ae_set_scene *scene_mode = param;
			if (scene_mode->mode < AE_SCENE_MAX) {
				cxt->cur_status.settings.scene_mode = (int8_t)scene_mode->mode;
			}
			AE_LOGI("scene: %d\n", scene_mode->mode);
		}
		break;

	case AE_SET_ISO:
		if (param) {
			struct ae_set_iso *iso = param;

			if (iso->mode < AE_ISO_MAX) {
				cxt->cur_status.settings.iso = iso->mode;
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

			if (flicker->mode < AE_FLICKER_MAX) {
				cxt->cur_status.settings.flicker = flicker->mode;
			}
		}
		break;

	case AE_SET_WEIGHT:
		if (param) {
			struct ae_set_weight *weight = param;

			AE_LOGD("setweight %d", weight->mode);
			// bad area
			weight->mode = 1;
			// bad area
			if (weight->mode < AE_WEIGHT_MAX) {
				cxt->cur_status.settings.metering_mode = weight->mode;
			}
		}
		break;

	case AE_SET_TOUCH_ZONE:
		if (param) {
			struct ae_set_tuoch_zone *touch_zone = param;
			if ((touch_zone->touch_zone.x >= 0)\
				&& (touch_zone->touch_zone.y >= 0)\
				&& (touch_zone->touch_zone.w > 1)\
				&& (touch_zone->touch_zone.h > 1)) {
				/*touch screen coordination */
				cxt->cur_status.touch_scrn_win = touch_zone->touch_zone;	// for touch ae
				cxt->cur_status.settings.touch_scrn_status = 1;	// for touch ae
				rtn = _set_restore_cnt(cxt);
				AE_LOGD("TC_TOUCH ae triger %d",cxt->cur_status.settings.touch_scrn_status);
			} else {
				AE_LOGD("touch ignore\n");
			}

		}
		break;

	case AE_SET_EV_OFFSET:
		if (param) {
			struct ae_set_ev *ev = param;

			AE_LOGD("setev %d", ev->level);
			// bad area
			// ev->level++;
			// bad area
			if (ev->level < AE_LEVEL_MAX) {
				cxt->cur_status.settings.ev_index = ev->level;
			}
		}
		break;

	case AE_SET_FPS:
		if (param) {
			struct ae_set_fps *fps = param;

			cxt->cur_status.settings.min_fps = fps->min_fps;
			cxt->cur_status.settings.max_fps = fps->max_fps;
		}
		break;

	case AE_SET_WAIT_PAUSE:
		cxt->cur_status.settings.lock_ae = AE_STATE_WAIT_PAUSE;
		cxt->cur_status.settings.pause_cnt++;
		break;

	case AE_SET_PAUSE:
		rtn = _set_pause(cxt);
		break;

	case AE_SET_RESTORE_CNT:
		rtn = _set_restore_cnt(cxt);
		break;

	case AE_SET_RESTORE:
		rtn = _set_restore_cnt(cxt);
		break;

	case AE_SET_FORCE_PAUSE:
		cxt->cur_status.settings.lock_ae = AE_STATE_LOCKED;
		if (param) {
			struct ae_exposure_gain *set = param;

			cxt->cur_status.settings.manual_mode = set->set_mode;
			cxt->cur_status.settings.table_idx = set->index;
			cxt->cur_status.settings.exp_line = (uint32_t)(10 * set->exposure / cxt->cur_status.line_time + 0.5);
			cxt->cur_status.settings.gain = set->again;
			//AE_LOGD("EMDEBUG_param1_manual_mode is %d,set_exp_line is (%d,%d, %f) set_gain is (%d,%d), line time: %d",cxt->cur_status.settings.manual_mode,
				//cxt->sync_cur_result.wts.cur_exp_line,cxt->cur_status.settings.exp_line, set->exposure,cxt->sync_cur_result.wts.cur_again,set->again, cxt->cur_status.line_time);
		} 
		break;

	case AE_SET_FORCE_RESTORE:
		cxt->cur_status.settings.lock_ae = AE_STATE_NORMAL;
		cxt->cur_status.settings.pause_cnt = 0;
		break;

	case AE_SET_FLASH_NOTICE:
		rtn = _set_flash_notice(cxt, param);
		break;

	case AE_GET_FLASH_EFFECT:
		if (result) {
			uint32_t *effect = (uint32_t *) result;

			*effect = cxt->flash_effect;
			AE_LOGD("flash_effect %d", *effect);
		}
		break;

	case AE_GET_AE_STATE:
		/*
		 * if (result) { uint32_t *ae_state =
		 * (uint32_t*)result; *ae_state = cxt->ae_state;
		 * } 
		 */
		break;

	case AE_GET_FLASH_EB:
		if (result) {
			uint32_t *flash_eb = (uint32_t *) result;
			int32_t bv = 0;
			int32_t bv_thr = cxt->flash_on_off_thr;

			if (0 >= bv_thr)
				bv_thr = AE_FLASH_ON_OFF_THR;

			rtn = _get_bv_by_lum_new(cxt, &bv);

			if (bv < bv_thr)
				*flash_eb = 1;
			else
				*flash_eb = 0;

			AE_LOGD("AE_GET_FLASH_EB: flash_eb=%d, bv=%d, thr=%d", *flash_eb, bv, bv_thr);
		}
		break;

	case AE_SET_FLASH_ON_OFF_THR:
		if (param) {
			cxt->flash_on_off_thr = *(int32_t *) param;
			if (0 >= cxt->flash_on_off_thr) {
				cxt->flash_on_off_thr = AE_FLASH_ON_OFF_THR;
			}
			AE_LOGD("AE_SET_FLASH_EB: cxt->flash_on_off_thr = %d", cxt->flash_on_off_thr);
		}
		break;

	case AE_SET_TUNING_EB:
		break;

	case AE_GET_BV_BY_GAIN:
	case AE_GET_BV_BY_GAIN_NEW:
		if (result) {
			int32_t *bv = (int32_t *) result;

			*bv = cxt->cur_result.wts.cur_again;
		}
		break;

	case AE_GET_EV:
		if (result) {
			uint32_t i = 0x00;
			struct ae_ev_table *ev_table = &cxt->cur_param->ev_table;
			int32_t target_lum = cxt->cur_param->target_lum;
			struct ae_get_ev *ev_info = (struct ae_get_ev *)result;
			ev_info->ev_index = cxt->cur_status.settings.ev_index;

			for (i = 0; i < ev_table->diff_num; i++) {
				ev_info->ev_tab[i] = target_lum + ev_table->lum_diff[i];
			}
			// AE_LOGD("AE default target %d
			// cur_ev_level %d\r\n", target_lum,
			// ev_info->ev_index);
			rtn = AE_SUCCESS;
		}
		break;

	case AE_GET_BV_BY_LUM:
		if (result) {
			int32_t *bv = (int32_t *) result;

			rtn = _get_bv_by_lum(cxt, bv);
		}
		break;

	case AE_GET_BV_BY_LUM_NEW:
		if (result) {
			int32_t *bv = (int32_t *) result;

			rtn = _get_bv_by_lum_new(cxt, bv);
		}
		break;

	case AE_SET_STAT_TRIM:
		if (param) {
			struct ae_trim *trim = (struct ae_trim *)param;
			rtn = _set_scaler_trim(cxt, trim);
		}
		break;

	case AE_SET_G_STAT:
		if (param) {
			struct ae_stat_mode *stat_mode = (struct ae_stat_mode *)param;
			rtn = _set_g_stat(cxt, stat_mode);
		}
		break;

	case AE_GET_LUM:
		if (result) {
			uint32_t *lum = (uint32_t *) result;

			*lum = cxt->cur_result.cur_lum;
		}
		break;

	case AE_SET_TARGET_LUM:
		if (param) {
			uint32_t *lum = (uint32_t *) param;

			cxt->cur_status.target_lum = *lum;
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
		if (result) {
			uint32_t *mode = result;

			*mode = cxt->cur_status.settings.flicker;
		}
		break;

	case AE_SET_ONLINE_CTRL:
		if (param) {
			rtn = _tool_online_ctrl(cxt, param, result);
		}
		break;

	case AE_SET_EXP_GAIN:
		// TBD
		break;

	case AE_SET_EXP_ANIT:
		// TBD
		break;

	case AE_SET_FD_ENABLE:
		if (param) {
			cxt->fdae.allow_to_work = *((uint32_t *) param);
		}
		break;

	case AE_SET_FD_PARAM:
		if (param) {
			rtn = _fd_process(cxt, param);
		}
		break;

	case AE_GET_GAIN:
		if (result) {
			*(float *)result = cxt->cur_result.wts.cur_again;
		}
		break;

	case AE_GET_EXP:
		if (result) {
			*(float *)result = cxt->cur_result.wts.exposure_time / 10000000.0;
			// AE_LOGD("wts.exposure_time %1.f
			// %d\r\n", *(float *)result,
			// cxt->cur_result.wts.exposure_time);
		}
		break;

	case AE_GET_FLICKER_SWITCH_FLAG:
		if (param) {
			rtn = _get_flicker_switch_flag(cxt, param);
		}
		break;

	case AE_GET_CUR_WEIGHT:
		*(int8_t *) result = cxt->cur_status.settings.metering_mode;
		break;

	case AE_GET_SKIP_FRAME_NUM:
		rtn = _get_skip_frame_num(cxt, param, result);
		break;

	case AE_SET_NIGHT_MODE:
		cxt->cur_status.settings.scene_mode = AE_SCENE_NIGHT;
		break;

	case AE_SET_NORMAL_MODE:
		cxt->cur_status.settings.scene_mode = AE_SCENE_NORMAL;
		break;

	case AE_SET_BYPASS:
		if (param){
			if (1 == *(int32_t*)param){
				cxt->bypass = 1;
			}else{
				cxt->cur_status.settings.lock_ae = AE_STATE_NORMAL;
				cxt->cur_status.settings.pause_cnt = 0;
				cxt->bypass = 0;
			}
		}
		break;

	case AE_GET_NORMAL_INFO:
		if (result) {
			rtn = _get_normal_info(cxt, result);
		}
		break;

	case AE_SET_SPORT_MODE:
		cxt->cur_status.settings.scene_mode = AE_SCENE_SPORT;
		break;

	case AE_SET_PANORAMA_START:
		cxt->cur_status.settings.scene_mode = AE_SCENE_PANORAMA;
		break;

	case AE_SET_PANORAMA_STOP:
		cxt->cur_status.settings.scene_mode = AE_SCENE_NORMAL;
		break;

	case AE_SET_FORCE_QUICK_MODE:
		//TBD
		break;
		
	case AE_SET_EXP_TIME:
		if (param) {
			_set_sensor_exp_time(cxt, param);
		}
		break;

	case AE_SET_SENSITIVITY:
		if (param) {
			_set_sensor_sensitivity(cxt, param);
		}
		break;

	case AE_GET_EXP_TIME:
		if (param) {
			*(uint32_t *) param = cxt->cur_result.wts.exposure_time;
		}
		break;
	case AE_GET_SENSITIVITY:
		if (param) {
			*(uint32_t *) param = cxt->cur_result.wts.cur_again * 50 / 128;
		}
		break;

	case AE_GET_DEBUG_INFO:
		if (result) {
			rtn = ae_get_debug_info(cxt, result);
		}
		break;
	case AE_GET_EM_PARAM:
		if (result) {
			rtn = ae_get_debug_info_for_display(cxt, result);
		}
		break;

	case AE_INTELLIGENT_OPEN:
		cxt->cur_status.settings.intelligent_module = 0b01;
		break;

	case AE_INTELLIGENT_CLOSE:
		cxt->cur_status.settings.intelligent_module = 0b00;
		break;

	case AE_VIDEO_STOP:
		if (0 == cxt->sync_cur_result.wts.cur_exp_line
			&& 0 == cxt->sync_cur_result.wts.cur_again){
			cxt->last_expline = cxt->cur_status.ae_table->exposure[cxt->cur_status.start_index];
			cxt->last_dummy = 0;
			cxt->last_aegain = cxt->cur_status.ae_table->again[cxt->cur_status.start_index];
			cxt->last_linetime = cxt->cur_status.line_time;
		}else{
			cxt->last_expline = cxt->sync_cur_result.wts.cur_exp_line;
			cxt->last_dummy = cxt->sync_cur_result.wts.cur_dummy;
			cxt->last_aegain = cxt->sync_cur_result.wts.cur_again;
			cxt->last_linetime = cxt->cur_status.line_time;
		}
		cxt->last_enable = 1;
		AE_LOGD("AE_VIDEO_STOP E %d G %d", cxt->last_expline, cxt->last_aegain);
		break;

	case AE_VIDEO_START:
		if (param){
			int32_t i;
			int32_t again;
			float	rgb_gain_coeff;
			struct ae_trim trim;
			struct ae_set_work_param *work_info = param;

			if (work_info->mode >= AE_WORK_MODE_MAX) {
				AE_LOGE("work mode param is error");
				work_info->mode = AE_WORK_MODE_COMMON;
			}

			cxt->snr_info = work_info->resolution_info;
			cxt->cur_status.frame_size = work_info->resolution_info.frame_size;
			cxt->cur_status.line_time = work_info->resolution_info.line_time;

			cxt->start_id = AE_START_ID;
			cxt->monitor_unit.mode = AE_STATISTICS_MODE_CONTINUE;
			cxt->monitor_unit.cfg.skip_num = 0;
			cxt->monitor_unit.is_stop_monitor = 0;

			trim.x = 0;
			trim.y = 0;
			trim.w = work_info->resolution_info.frame_size.w;
			trim.h = work_info->resolution_info.frame_size.h;
			_update_monitor_unit(cxt, &trim);
			_cfg_set_aem_mode(cxt);
			_cfg_monitor(cxt);

			cxt->cur_status.win_size = cxt->monitor_unit.win_size;
			cxt->cur_status.win_num = cxt->monitor_unit.win_num;

			exposure_time2line(&(cxt->tuning_param[work_info->mode]), cxt->cur_status.line_time,
															cxt->tuning_param[work_info->mode].ae_tbl_exp_mode);
			for (int j = 0; j < AE_SCENE_MAX; ++j) {
				exp_time2exp_line(cxt, cxt->back_scene_mode_ae_table[j],\
									cxt->tuning_param[work_info->mode].scene_info[j].ae_table,\
									cxt->cur_status.line_time,\
									cxt->tuning_param[work_info->mode].scene_info[j].exp_tbl_mode);
			}
			if (1 == cxt->tuning_param_enable[work_info->mode])
				cxt->cur_param = &cxt->tuning_param[work_info->mode];
			else
				cxt->cur_param = &cxt->tuning_param[AE_WORK_MODE_COMMON];

			cxt->cur_status.ae_table = &cxt->cur_param->ae_table[AE_FLICKER_50HZ][AE_ISO_AUTO];

			if (1 == cxt->last_enable){
				if (cxt->cur_status.line_time == cxt->last_linetime){
					cxt->ae_result.expline	= cxt->last_expline;
					cxt->ae_result.gain 	= cxt->last_aegain;
					cxt->ae_result.dummy	= cxt->last_dummy;
				}else{
					cxt->ae_result.expline = cxt->last_expline * cxt->last_linetime
												/ (float)cxt->cur_status.line_time;
					if (cxt->cur_status.min_exp_line > cxt->ae_result.expline)
						cxt->ae_result.expline = cxt->cur_status.min_exp_line;
					
					cxt->ae_result.gain 	= cxt->last_aegain;
					cxt->ae_result.dummy	= cxt->last_dummy;
				}
			}else{
				cxt->ae_result.expline	= cxt->cur_status.ae_table->exposure[cxt->cur_status.start_index];
				cxt->ae_result.gain 	= cxt->cur_status.ae_table->again[cxt->cur_status.start_index];
				cxt->ae_result.dummy	= 0;

			}
			cxt->sensor_calc_item.cell.exp_line = cxt->ae_result.expline;
			cxt->sensor_calc_item.cell.gain 	= cxt->ae_result.gain;
			cxt->sensor_calc_item.cell.dummy	= cxt->ae_result.dummy;

			for (i = 0; i < 20; i++){
				cxt->actual_cell[i] = cxt->ae_result;
				//AE_LOGD("AE_VIDEO_START E %d G %d\r\n", cxt->actual_cell[i].expline, cxt->actual_cell[i].gain);
			}

			//AE_LOGD("D-gain--setting--- %d %d %d\n", cxt->sensor_gain_precision, cxt->exp_skip_num, cxt->gain_skip_num);

			if (cxt->ae_result.gain <= cxt->cur_status.max_gain){
				if (0 == cxt->ae_result.gain % cxt->sensor_gain_precision){
					again = cxt->ae_result.gain;
					rgb_gain_coeff = 1;
				}else{
					again = (cxt->ae_result.gain / cxt->sensor_gain_precision) * cxt->sensor_gain_precision;
					rgb_gain_coeff = (double)cxt->out_write_cell.gain / (double)again;
				}
			}else{
				again = cxt->cur_status.max_gain;
				rgb_gain_coeff = (double)cxt->out_write_cell.gain / (double)cxt->cur_status.max_gain;
			}

			cxt->isp_ops.set_rgb_gain(cxt->isp_ops.isp_handler, rgb_gain_coeff);			
			_ae_write_exp_gain(cxt, cxt->ae_result.expline, cxt->sensor_calc_item.cell.dummy, again);

			if (0 == work_info->is_snapshot){
				cxt->sof_id = 0;
				cxt->cur_status.frame_id = 0;
				memset(&cxt->cur_result.wts, 0, sizeof(struct ae1_senseor_out));
				memset(&cxt->sync_cur_result.wts, 0, sizeof(struct ae1_senseor_out));
			}else{
				;
			}
			cxt->send_once[0] = cxt->send_once[1] = cxt->send_once[2] = 0;
			AE_LOGD("AE_VIDEO_START lt %d", cxt->cur_status.line_time);
		}
		break;

		case AE_HDR_START:
			cxt->hdr_flag = 3;
			cxt->hdr_timestamp = 0;
			cxt->hdr_base_ae_idx = cxt->sync_cur_result.wts.cur_index;
			break;

		case AE_HDR_FINISH:
			cxt->hdr_timestamp = 0;
			cxt->cur_status.settings.lock_ae = AE_STATE_NORMAL;
			break;

		case AE_HDR_GET_INFO:
			if (param && result){
				*(uint64_t *)(param) = cxt->hdr_timestamp;
				*(uint32_t *)(result) = 4;
			}else{
				AE_LOGD("_isp_hdr_error\n");
			}
			break;

		default:
			rtn = AE_ERROR;
			break;
	}
	pthread_mutex_unlock(&cxt->data_sync_lock);

	return rtn;
}

int32_t ae_sprd_deinit(void *handle, void *in_param, void *out_param)
{
	int32_t rtn = AE_SUCCESS;
	struct ae_ctrl_cxt *cxt = NULL;
	struct ae_in_out *reserve = NULL;
	UNUSED(in_param);
	AE_LOGD("V2_DEINIT ST\r\n");
	rtn = _check_handle(handle);
	if (AE_SUCCESS != rtn) {
		return AE_ERROR;
	}
	cxt = (struct ae_ctrl_cxt *)handle;

	if (NULL != out_param){
		reserve = (struct ae_in_out *)out_param;
		reserve->enable 		= 1;
		reserve->cur_exp_line	= cxt->sync_cur_result.wts.cur_exp_line;
		reserve->cur_gain		= cxt->sync_cur_result.wts.cur_again;
		reserve->cur_dummy		= cxt->sync_cur_result.wts.cur_dummy;
	}

	rtn = ae_misc_deinit(cxt->misc_handle, NULL, NULL);

	if (AE_SUCCESS != rtn) {
		AE_LOGE("misc deinit is failed!");
		rtn = AE_ERROR;
	}
	//ae_seq_reset(cxt->seq_handle);
	seq_deinit(cxt->seq_handle);
	//destroy ISP gain queue 
	isp_gain_buffer_deinit(cxt);

	if (cxt->debug_enable) {
		if (cxt->debug_info_handle) {
			debug_file_deinit((debug_handle_t) cxt->debug_info_handle);
			cxt->debug_info_handle = (uint32_t)NULL;
		}
	}

	/*
	 * destroy thread 
	 */
	if (cxt->thread_handle) {
		cmr_thread_destroy(cxt->thread_handle);
		cxt->thread_handle = 0;
	}

	pthread_mutex_destroy(&cxt->data_sync_lock);

	if (cxt) {
		free(cxt);
	}
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
