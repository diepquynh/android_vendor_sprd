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

#define LOG_TAG "isp_app"


#include <sys/time.h>
#include <time.h>
// COMMON
#include "isp_app.h"
#include "isp_com.h"
#include "isp_log.h"
#include "cmr_msg.h"
#include "isp_type.h"
#include "isp_debug.h"
// PARAM_MANAGER
#include "isp_pm.h"

// DRIVER
#include "isp_drv.h"

// 4A
#include "ae_sprd_ctrl.h"
#include "awb_sprd_ctrl.h"
#include "af_ctrl.h"
#include "aem_binning.h"
#include "smart_ctrl.h"
#include "lsc_adv.h"

//otp calibration
#include "isp_otp_calibration.h"

#include "deflicker.h"
#include "lsc_adv.h"
#include "sensor_drv_u.h"
#include "lib_ctrl.h"
#define SMART_LSC_VERSION 1
// posture sensor info
#include <hardware/sensors.h>
#define LSC_ADV_ENABLE

#include "sensor_isp_param_merge.h"

#define AF_REALTIME_CTRL
#define AE_MONITOR_CHANGE

#define SATURATION_DEPRESS_USE_BLK_SATURATION

#define SEPARATE_GAMMA_IN_VIDEO
#define VIDEO_GAMMA_INDEX                    (8)
#define Y_GAMMA_SMART_WITH_RGB_GAMMA
#define ISP_PARAM_VERSION                    (0x00030002)
#define SPRD_DEBUG_VERSION_ID 				 (0x00000003)

#define ISP_CTRL_EVT_INIT                    (1 << 2)
#define ISP_CTRL_EVT_DEINIT                  (1 << 3)
#define ISP_CTRL_EVT_CONTINUE                (1 << 4)
#define ISP_CTRL_EVT_CONTINUE_STOP           (1 << 5)
#define ISP_CTRL_EVT_SIGNAL                  (1 << 6)
#define ISP_CTRL_EVT_SIGNAL_NEXT             (1 << 7)
#define ISP_CTRL_EVT_IOCTRL                  (1 << 8)
#define ISP_CTRL_EVT_TX                      (1 << 9)
#define ISP_CTRL_EVT_SOF                     (1 << 10)
#define ISP_CTRL_EVT_EOF                     (1 << 11)
#define ISP_CTRL_EVT_AE                      (1 << 12)
#define ISP_CTRL_EVT_AWB                     (1 << 13)
#define ISP_CTRL_EVT_AF                      (1 << 14)
#define ISP_CTRL_EVT_CTRL_SYNC               (1 << 15)
#define ISP_CTRL_EVT_CONTINUE_AF             (1 << 16)
#define ISP_CTRL_EVT_AEM2                    (1 << 17)
#define ISP_CTRL_EVT_BINNING_DONE            (1 << 18)
#define ISP_CTRL_EVT_MONITOR_STOP            (1 << 31)
#define ISP_CTRL_EVT_MASK                    (uint32_t)(ISP_CTRL_EVT_INIT \
					|ISP_CTRL_EVT_CONTINUE_STOP|ISP_CTRL_EVT_DEINIT|ISP_CTRL_EVT_CONTINUE \
					|ISP_CTRL_EVT_SIGNAL|ISP_CTRL_EVT_SIGNAL_NEXT|ISP_CTRL_EVT_IOCTRL \
					|ISP_CTRL_EVT_TX|ISP_CTRL_EVT_SOF|ISP_CTRL_EVT_EOF|ISP_CTRL_EVT_AWB \
					|ISP_CTRL_EVT_AE|ISP_CTRL_EVT_AF|ISP_CTRL_EVT_CTRL_SYNC|ISP_CTRL_EVT_CONTINUE_AF \
					|ISP_CTRL_EVT_AEM2|ISP_CTRL_EVT_BINNING_DONE|ISP_CTRL_EVT_MONITOR_STOP)

#define ISP_PROC_EVT_AE			             (1 << 2)
#define ISP_PROC_EVT_AWB		             (1 << 3)
#define ISP_PROC_EVT_AF			             (1 << 4)
#define ISP_PROC_EVT_AF_STOP		         (1 << 5)
#define ISP_PROC_EVT_CONTINUE_AF	         (1 << 6)
#define ISP_PROC_EVT_CONTINUE_AF_STOP	     (1 << 7)
#define ISP_PROC_EVT_STOP_HANDLER	         (1 << 8)
#define ISP_PROC_EVT_AEM2                    (1 << 9)
#define ISP_PROC_EVT_MASK	(uint32_t)(ISP_PROC_EVT_AE | ISP_PROC_EVT_AWB \
					| ISP_PROC_EVT_AF | ISP_PROC_EVT_AF_STOP | ISP_PROC_EVT_CONTINUE_AF \
					| ISP_PROC_EVT_CONTINUE_AF_STOP | ISP_PROC_EVT_STOP_HANDLER \
					| ISP_PROC_EVT_AEM2)

#define ISP_THREAD_QUEUE_NUM                 (50)

#define ISP_PROC_AFL_DONE                    (1 << 2)
#define ISP_PROC_AFL_STOP                    (1 << 3)
#define ISP_AFL_EVT_MASK                     (uint32_t)(ISP_PROC_AFL_DONE | ISP_PROC_AFL_STOP)

#define ISP_AFL_BUFFER_LEN                   (3120 * 4 * 61)

#define ISP_PROC_AF_CALC                     (1<<2)
#define ISP_PROC_AF_IMG_DATA_UPDATE          (1<<3)
#define ISP_PROC_AF_STOP                     (1<<4)
#define ISP_PROC_AF_EVT_MASK                 (uint32_t)(ISP_PROC_AF_CALC|ISP_PROC_AF_IMG_DATA_UPDATE|ISP_PROC_AF_STOP)

#define ISP_PROC_AWB_CALC                    (1<<2)
#define ISP_PROC_AWB_STOP                    (1<<3)
#define ISP_PROC_AWB_EVT_MASK                (uint32_t)(ISP_PROC_AWB_CALC|ISP_PROC_AWB_STOP)

#define ISP_PROC_LSC_CALC 	(1<<2) //gerald merge http://review.source.spreadtrum.com/gerrit/#/c/281621 start
#define ISP_PROC_LSC_DATA_UPDATE    (1<<3)
#define ISP_PROC_LSC_STOP 	(1<<4)
#define ISP_PROC_LSC_EVT_MASK (uint32_t)(ISP_PROC_LSC_CALC|ISP_PROC_LSC_DATA_UPDATE|ISP_PROC_LSC_STOP)  //gerald merge http://review.source.spreadtrum.com/gerrit/#/c/281621 end



#define ISP_CALLBACK_EVT                     0x00040000

#define BLOCK_PARAM_CFG(input, param_data, blk_cmd, blk_id, cfg_ptr, cfg_size)\
	do {\
		param_data.cmd = blk_cmd;\
		param_data.id = blk_id;\
		param_data.data_ptr = cfg_ptr;\
		param_data.data_size = cfg_size;\
		input.param_data_ptr = &param_data;\
		input.param_num = 1;} while (0);

#define SET_GLB_ERR(rtn, handle)\
	do {\
			handle->last_err = rtn;\
	} while(0)

#define GET_GLB_ERR(handle)                  (0 != handle->last_err)

#define ISP_SAVE_REG_LOG                     "isp.reg.log.path"
#define ISP_SET_AFL_THR                      "isp.afl.thr"

#define ISP_REG_NUM                          20467
uint32_t isp_cur_bv;
uint32_t isp_cur_ct;

int gAWBGainR=1; //gerald merge http://review.source.spreadtrum.com/gerrit/#/c/286361
int gAWBGainB=1; //gerald merge http://review.source.spreadtrum.com/gerrit/#/c/286361

int g_isp_app_proc_cnt = 0;
int gCntSendMsgLsc = 0;
static uint32_t aeStatistic[32*32*4];


struct isp_aem_info {
	isp_u32 stat_fmt;
	struct isp_ae_grgb_statistic_info *rgb_stat;
	struct isp_ae_statistic_info  *yiq_stat;
};

struct isp_awb_calc_info {
	struct ae_calc_out ae_result;
	struct isp_ae_grgb_statistic_info ae_stat;
	uint64_t k_addr;
	uint64_t u_addr;
	isp_u32 type;
};

struct isp_alsc_calc_info{//gerald merge http://review.source.spreadtrum.com/gerrit/#/c/281621 start
	uint32_t* stat_ptr;
	int image_width;//gerald merge http://review.source.spreadtrum.com/gerrit/#/c/286361
	int image_height;//gerald merge http://review.source.spreadtrum.com/gerrit/#/c/286361
	struct awb_size stat_img_size;
	struct awb_size win_size;
	uint32_t awb_ct; int awb_r_gain; int awb_b_gain;
	uint32_t stable;
};//gerald merge http://review.source.spreadtrum.com/gerrit/#/c/281621 end

/* Local Function Prototypes */

static int32_t isp_update_2d_lsc_slice_info(isp_handle isp_handler, void *param_ptr) //gerald 20161031
{
	int32_t rtn = ISP_SUCCESS;
	isp_ctrl_context *handle = (isp_ctrl_context*)isp_handler;
	struct isp_interface_param_v1 *interface_ptr_v1 = &handle->interface_param_v1;
	struct isp_slice_window *cur_slice_win = &interface_ptr_v1->slice.cur_slice_window;
	struct isp_dev_2d_lsc_info *lens_info_in = (struct isp_dev_2d_lsc_info *)(param_ptr);
	uint32_t gridX, grid_pitch, lsc_grid, img_width;
	uint32_t startGridRow, startGridCol, endGridRow, endGridCol;
	uint32_t tmp1 = 0, tmp2 = 0;

	tmp1 = lens_info_in->buf_addr[0];
	tmp2 = lens_info_in->buf_addr[1];
	img_width = interface_ptr_v1->data.input_size.w;
	lsc_grid = lens_info_in->grid_width;
	gridX = ((img_width/2)%lsc_grid)?((img_width/2)/lsc_grid+2):((img_width/2)/lsc_grid+1);
	gridX += 2;
	grid_pitch = ((img_width/2 - 1)%lsc_grid)?((img_width/2 - 1)/lsc_grid+2):((img_width/2 - 1)/lsc_grid+1);
	grid_pitch += 2;

	startGridRow = (cur_slice_win->start_row / 2) / lsc_grid;
	startGridCol = (cur_slice_win->start_col / 2) / lsc_grid;
	endGridRow = ((cur_slice_win->end_row - 1) / 2) / lsc_grid + 1;
	endGridCol = ((cur_slice_win->end_col - 1) / 2) / lsc_grid + 1;

	lens_info_in->offset_x = cur_slice_win->start_col & 0xFFFF;
	lens_info_in->offset_y = cur_slice_win->start_row & 0xFFFF;
	lens_info_in->grid_x_num = endGridCol - startGridCol + 1;
	lens_info_in->grid_y_num = endGridRow - startGridRow + 1;
	lens_info_in->grid_num_t = (lens_info_in->grid_x_num + 2) * (lens_info_in->grid_y_num + 2);
	lens_info_in->buf_addr[0] = lens_info_in->buf_addr[0] + (startGridRow * gridX + startGridCol) * 4 * 2;
	lens_info_in->buf_addr[1] = lens_info_in->buf_addr[1] + (startGridRow * gridX + startGridCol) * 4 * 2;
	lens_info_in->relative_x = (cur_slice_win->start_col % (lsc_grid * 2));
	lens_info_in->relative_y = (cur_slice_win->start_row % (lsc_grid * 2));
	lens_info_in->grid_pitch = grid_pitch;

	isp_u_2d_lsc_block(handle->handle_device, (void *)(lens_info_in));
	isp_u_2d_lsc_param_update(handle->handle_device);

	lens_info_in->buf_addr[0] = tmp1;
	lens_info_in->buf_addr[1] = tmp2;

	return rtn;
}

static nsecs_t isp_get_timestamp(void)
{
	nsecs_t                         timestamp = 0;

	timestamp = systemTime(CLOCK_MONOTONIC);
	timestamp = timestamp / 1000000;

	return timestamp;
}

static int32_t _ispEmGetParam(isp_handle isp_handler, void *param_ptr, int(*call_back)());

static uint32_t _smart_calc(isp_ctrl_context* handle, int32_t bv, int32_t bv_gain, uint32_t ct, uint32_t alc_awb)
{
	uint32_t                        rtn = ISP_SUCCESS;
	isp_smart_handle_t              smart_handle = handle->handle_smart;
	isp_pm_handle_t                 pm_handle = handle->handle_pm;
	struct smart_calc_param         smart_calc_param = {0};
	struct smart_calc_result        smart_calc_result = {0};
	struct isp_pm_ioctl_input       io_pm_input = {0};
	struct isp_pm_param_data        pm_param = {0};
	struct smart_block_result       *block_result = NULL;
	uint32_t                        i = 0;

	smart_calc_param.bv      = bv;
	smart_calc_param.ct      = ct;
	smart_calc_param.bv_gain = bv_gain;

	ISP_LOGV("ISP_SMART: bv=%d, ct=%d, bv_gain=%d", bv, ct, bv_gain);

	rtn = smart_ctl_calculation(smart_handle, &smart_calc_param, &smart_calc_result);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("smart init failed");
		return rtn;
	}
	
	/*gerald ,merge http://review.source.spreadtrum.com/gerrit/#/c/278298 start*/
       struct alsc_ver_info lsc_ver = {0};
	rtn  = handle->lsc_lib_fun->alsc_io_ctrl(handle->handle_lsc_adv, ALSC_GET_VER, NULL, (void *)&lsc_ver);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("Get ALSC ver info failed!");
	}	
	if( lsc_ver.LSC_SPD_VERSION >= 3 ){
		for (i=0; i<smart_calc_result.counts; i++) {
			if (ISP_SMART_LNC == smart_calc_result.block_result[i].smart_id) {
				smart_calc_result.block_result[i].update = 0;
			}
			//ALOGE("ISP_SMART_LSCs: smart_calc_result.block_result[i].update=%d\n",smart_calc_result.block_result[i].update); http://review.source.spreadtrum.com/gerrit/#/c/281621
		}
	}


	/*use alc ccm, disable spreadtrum smart ccm*/
	for (i = 0; i < smart_calc_result.counts; i++) {
		switch (smart_calc_result.block_result[i].smart_id) {
		case ISP_SMART_CMC:
			if (alc_awb & (1<<0)) {
				smart_calc_result.block_result[i].update = 0;
			}
			break;
		case ISP_SMART_LNC:
			if (alc_awb & (1<<8)) {
				smart_calc_result.block_result[i].update = 0;
			}
			break;
		case ISP_SMART_COLOR_CAST:
			if (alc_awb & (1<<0)) {
				smart_calc_result.block_result[i].update = 0;
			}
			break;
		case ISP_SMART_GAIN_OFFSET:
			if (alc_awb & (1<<0)) {
				smart_calc_result.block_result[i].update = 0;
			}
			break;
		case ISP_SMART_SATURATION_DEPRESS:
			smart_calc_result.block_result[i].block_id = ISP_BLK_SATURATION_V1;
			break;
		default:
			break;
		}
	}

	for (i = 0; i < smart_calc_result.counts; i++) {
		block_result               = &smart_calc_result.block_result[i];
		if((block_result->mode_flag != handle->mode_flag) || (block_result->scene_flag != handle->scene_flag)) {
			block_result->mode_flag_changed = 1;
			block_result->mode_flag = handle->mode_flag;
			block_result->scene_flag = handle->scene_flag;
		}

		pm_param.cmd               = ISP_PM_BLK_SMART_SETTING;
		pm_param.id                = block_result->block_id;
		pm_param.data_size         = sizeof(*block_result);
		pm_param.data_ptr          = (void*)block_result;

		io_pm_input.param_data_ptr = &pm_param;
		io_pm_input.param_num      = 1;

		ISP_LOGV("ISP_SMART: set param %d, id=%d, data=%p, size=%d", i, pm_param.id, pm_param.data_ptr,
				pm_param.data_size);
		rtn = isp_pm_ioctl(pm_handle, ISP_PM_CMD_SET_SMART, &io_pm_input, NULL);
	#ifdef Y_GAMMA_SMART_WITH_RGB_GAMMA
		if (ISP_BLK_RGB_GAMC == pm_param.id) {
			pm_param.id = ISP_BLK_Y_GAMMC;
			rtn = isp_pm_ioctl(pm_handle, ISP_PM_CMD_SET_SMART, &io_pm_input, NULL);
		}
	#endif
	}

	ISP_LOGV("ISP_SMART: handle=%p, counts=%d", smart_handle, smart_calc_result.counts);

	return rtn;
}

static int32_t _ispChangeAwbGain(isp_ctrl_context* isp_ctrl_ptr)
{
	int32_t                         rtn = ISP_SUCCESS;
	struct awb_ctrl_cxt             *awb_handle = (struct awb_ctrl_cxt*)(isp_ctrl_ptr->handle_awb);
	struct awb_gain                 result;
	struct isp_pm_ioctl_input       ioctl_input;
	struct isp_pm_param_data        ioctl_data;
	struct isp_awbc_cfg             awbc_cfg;

	rtn = isp_ctrl_ptr->awb_lib_fun->awb_ctrl_ioctrl(awb_handle, AWB_CTRL_CMD_GET_GAIN, (void*)&result, NULL);

	awbc_cfg.r_gain            = result.r;
	awbc_cfg.g_gain            = result.g;
	awbc_cfg.b_gain            = result.b;
	awbc_cfg.r_offset          = 0;
	awbc_cfg.g_offset          = 0;
	awbc_cfg.b_offset          = 0;

	ioctl_data.id              = ISP_BLK_AWB_V1;
	ioctl_data.cmd             = ISP_PM_BLK_AWBC;
	ioctl_data.data_ptr        = &awbc_cfg;
	ioctl_data.data_size       = sizeof(awbc_cfg);

	ioctl_input.param_data_ptr = &ioctl_data;
	ioctl_input.param_num      = 1;

	rtn = isp_pm_ioctl(isp_ctrl_ptr->handle_pm, ISP_PM_CMD_SET_AWB, (void*)&ioctl_input, NULL);
	ISP_LOGE("AWB_TAG: isp_pm_ioctl rtn=%d, gain=(%d, %d, %d)", rtn, awbc_cfg.r_gain, awbc_cfg.g_gain, awbc_cfg.b_gain);

	return rtn;
}

static int32_t _ispChangeSmartParam(isp_ctrl_context * isp_ctrl_ptr)
{
	int32_t                         rtn = ISP_SUCCESS;
	struct awb_ctrl_cxt             *awb_handle = (struct awb_ctrl_cxt*)(isp_ctrl_ptr->handle_awb);
	struct ae_ctrl_context          *ae_handle = (struct ae_ctrl_context*)(isp_ctrl_ptr->handle_ae);
	struct smart_context            *smart_handle = (struct smart_context*)(isp_ctrl_ptr->handle_smart);
	uint32_t                        ct = 0;
	int32_t                         bv_new = 0;
	int32_t                         bv_gain = 0;

	rtn = isp_ctrl_ptr->awb_lib_fun->awb_ctrl_ioctrl((awb_ctrl_handle_t*)awb_handle, AWB_CTRL_CMD_GET_CT, (void*)&ct, NULL);
	ISP_TRACE_IF_FAIL(rtn, ("awb get ct error"));

	rtn = isp_ctrl_ptr->ae_lib_fun->ae_io_ctrl((void *)ae_handle, AE_GET_BV_BY_LUM_NEW, NULL, (void*)&bv_new);
	ISP_TRACE_IF_FAIL(rtn, ("ae get bv error"));

	rtn = isp_ctrl_ptr->ae_lib_fun->ae_io_ctrl((void*)ae_handle, AE_GET_BV_BY_GAIN, NULL, (void*)&bv_gain);
	ISP_TRACE_IF_FAIL(rtn, ("ae get bv gain error"));

	rtn = smart_ctl_ioctl(smart_handle, ISP_SMART_IOCTL_SET_WORK_MODE,
		(void*)&isp_ctrl_ptr->isp_mode, NULL);
	ISP_TRACE_IF_FAIL(rtn, ("smart io_ctrl error"));

	if ((0 != bv_gain) && (0 != ct)) {
		rtn = _smart_calc(isp_ctrl_ptr, bv_new, bv_gain, ct, isp_ctrl_ptr->alc_awb);
		ISP_TRACE_IF_FAIL(rtn, ("smart calc error"));
	}

	ISP_LOGI("done");

	return rtn;
}

static int32_t _isp_get_flash_cali_param(isp_pm_handle_t pm_handle, struct isp_flash_param **out_param_ptr)
{
	int32_t                         rtn = ISP_SUCCESS;
	struct isp_pm_param_data        param_data = {0};
	struct isp_pm_ioctl_input       input = {NULL, 0};
	struct isp_pm_ioctl_output      output = {NULL, 0};

	if (NULL == out_param_ptr) {
		return ISP_PARAM_NULL;
	}

	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_ISP_SETTING, ISP_BLK_FLASH_CALI, NULL, 0);
	rtn = isp_pm_ioctl(pm_handle, ISP_PM_CMD_GET_SINGLE_SETTING, &input, &output);
	if (ISP_SUCCESS == rtn && 1 == output.param_num) {
		(*out_param_ptr) = (struct isp_flash_param*)output.param_data->data_ptr;
	} else {
		rtn = ISP_ERROR;
	}

	return rtn;
}

static int32_t _isp_set_awb_flash_gain(isp_ctrl_context *handle)
{
	int32_t                          rtn = ISP_SUCCESS;
	isp_ctrl_context                 *isp_handle = (isp_ctrl_context*)handle;
	struct isp_flash_param           *flash = NULL;
	struct awb_flash_info            flash_awb = {0};
	uint32_t                         ae_effect = 0;

	rtn = _isp_get_flash_cali_param(isp_handle->handle_pm, &flash);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("get flash cali parm failed");
		return rtn;
	}
	ISP_LOGI("flash param rgb ratio = (%d,%d,%d), lum_ratio = %d",
			flash->cur.r_ratio, flash->cur.g_ratio, flash->cur.b_ratio, flash->cur.lum_ratio);

	rtn = handle->ae_lib_fun->ae_io_ctrl(isp_handle->handle_ae, AE_GET_FLASH_EFFECT, NULL, &ae_effect);
	ISP_TRACE_IF_FAIL(rtn, ("ae get flash effect error"));

	flash_awb.effect        = ae_effect;
	flash_awb.flash_ratio.r = flash->cur.r_ratio;
	flash_awb.flash_ratio.g = flash->cur.g_ratio;
	flash_awb.flash_ratio.b = flash->cur.b_ratio;

	ISP_LOGI("FLASH_TAG get effect from ae effect = %d", ae_effect);

	rtn = handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_FLASHING, (void*)&flash_awb, NULL);
	ISP_TRACE_IF_FAIL(rtn, ("awb set flash gain error"));

	return rtn;
}

static int32_t ae_set_work_mode(isp_ctrl_context *handle, uint32_t new_mode, uint32_t fly_mode, struct isp_video_start *param_ptr)
{
	int32_t                         rtn = ISP_SUCCESS;
	struct ae_set_work_param        ae_param;
	enum ae_work_mode               ae_mode = 0;

	/*ae should known preview/capture/video work mode*/
	switch (new_mode) {
	case ISP_MODE_ID_PRV_0:
	case ISP_MODE_ID_PRV_1:
	case ISP_MODE_ID_PRV_2:
	case ISP_MODE_ID_PRV_3:
		ae_mode = AE_WORK_MODE_COMMON;
		break;

	case ISP_MODE_ID_CAP_0:
	case ISP_MODE_ID_CAP_1:
	case ISP_MODE_ID_CAP_2:
	case ISP_MODE_ID_CAP_3:
		ae_mode = AE_WORK_MODE_CAPTURE;
		break;

	case ISP_MODE_ID_VIDEO_0:
	case ISP_MODE_ID_VIDEO_1:
	case ISP_MODE_ID_VIDEO_2:
	case ISP_MODE_ID_VIDEO_3:
		ae_mode = AE_WORK_MODE_VIDEO;
		break;

	default:
		break;
	}

	ae_param.mode                               = ae_mode;
	ae_param.fly_eb                             = fly_mode;
	ae_param.highflash_measure.highflash_flag   = param_ptr->is_need_flash;
	ae_param.highflash_measure.capture_skip_num = param_ptr->capture_skip_num;
	ae_param.resolution_info.frame_size.w       = handle->src.w;
	ae_param.resolution_info.frame_size.h       = handle->src.h;
	ae_param.resolution_info.frame_line         = handle->input_size_trim[handle->param_index].frame_line;
	ae_param.resolution_info.line_time          = handle->input_size_trim[handle->param_index].line_time;
	ae_param.resolution_info.sensor_size_index  = handle->param_index;
	ae_param.is_snapshot						= param_ptr->is_snapshot;

	ISP_LOGI("ISP_AE: frame =%dx%d", ae_param.resolution_info.frame_size.w,
						ae_param.resolution_info.frame_size.h);

	ISP_LOGI("ISP_AE: mode=%d, size_index=%d", ae_param.mode,
						ae_param.resolution_info.sensor_size_index);
	ISP_LOGI("ISP_AE: frame_line=%d, line_time=%d", ae_param.resolution_info.frame_line,
						ae_param.resolution_info.line_time);

#ifdef CONFIG_CAMERA_ISP_AE_VERSION_V1
	//ISP_LOGI("is_snapshot %d param_ptr->mode %d", param_ptr->is_snapshot, param_ptr->mode);
	rtn = isp_dev_enable_irq(handle->handle_device, ISP_INT_VIDEO_MODE);
	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_VIDEO_START, &ae_param, NULL);
	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_DC_DV, &param_ptr->dv_mode, NULL);
#else
	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_WORK_MODE, &ae_param, NULL);
#endif
	ISP_LOGI("done");

	return rtn;
}

static int32_t ae_ex_set_exposure(void *handler, struct ae_exposure *in_param)
{
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)handler;
	struct sensor_ex_exposure ex_exposure;

	if (NULL == ctrl_context
		|| NULL == ctrl_context->ioctrl_ptr) {
			ISP_LOGE("cxt=%p,ioctl=%p is NULL error", ctrl_context,
				ctrl_context->ioctrl_ptr);
			return ISP_ERROR;
	}

	if (ctrl_context->ioctrl_ptr->ex_set_exposure) {
		ex_exposure.exposure = in_param->exposure;
		ex_exposure.dummy = in_param->dummy;
		ex_exposure.size_index = in_param->size_index;
		ctrl_context->ioctrl_ptr->ex_set_exposure((uint32_t)&ex_exposure);
	} else {
		ISP_LOGE("ex_set_exposure is NULL");
	}

	return 0;
}

static int32_t ae_set_exposure(void *handler, struct ae_exposure *in_param)
{
	isp_ctrl_context                *ctrl_context = (isp_ctrl_context*)handler;

	if (NULL == ctrl_context) {
		ISP_LOGE("cxt is NULL error");
		return ISP_PARAM_NULL;
	} else if (NULL == ctrl_context->ioctrl_ptr) {
		ISP_LOGE("ioctl is NULL error");
		return ISP_PARAM_NULL;
	} else if (NULL == ctrl_context->ioctrl_ptr->set_exposure) {
		ISP_LOGE("set_exposure is NULL error");
		return ISP_PARAM_NULL;
	}
 
	ctrl_context->ioctrl_ptr->set_exposure(in_param->exposure);

	return 0;
}

static int32_t _ispAeGetISO(isp_handle isp_handler, uint32_t *param_ptr)
{

        int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	uint32_t out_param = 0;

	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_ISO, NULL, &out_param);
	*((uint32_t*)param_ptr) = out_param;

	return rtn;
}

/*
 * temp function to save cur real gain
 */
int g_cur_real_gain = 0;
int get_cur_real_gain(void)
{
	return g_cur_real_gain;
}

static int32_t ae_set_again(void *handler, struct ae_gain *in_param)
{
	isp_ctrl_context                *ctrl_context = (isp_ctrl_context*)handler;

	if (NULL == ctrl_context) {
		ISP_LOGE("cxt is NULL error");
		return ISP_PARAM_NULL;
	} else if (NULL == ctrl_context->ioctrl_ptr) {
		ISP_LOGE("ioctl is NULL error");
		return ISP_PARAM_NULL;
	} else if (NULL == ctrl_context->ioctrl_ptr->set_gain) {
		ISP_LOGE("set_gain is NULL error");
		return ISP_PARAM_NULL;
	}

 	/* temp code begin */
	g_cur_real_gain = in_param->gain;
	/* temp code end */
	ctrl_context->ioctrl_ptr->set_gain(in_param->gain);

	return 0;
}

#ifdef CONFIG_CAMERA_ISP_AE_VERSION_V1
static int32_t ae_set_rgb_gain(isp_handle *isp_handler, double rgb_gain_coeff)
{
	int32_t rtn = ISP_SUCCESS;
	isp_ctrl_context *handle = (isp_ctrl_context*)isp_handler;
	uint32_t rgb_gain_offset = 4096;
	struct isp_dev_rgb_gain_info_v1 gain_info;

	gain_info.bypass 		= 0;
	gain_info.global_gain 	= handle->global_ob_gain * rgb_gain_coeff;
	gain_info.r_gain 		= rgb_gain_offset;
	gain_info.g_gain 		= rgb_gain_offset;
	gain_info.b_gain 		= rgb_gain_offset;
	//ISP_LOGD("D-gain--global_ob_gain = %d, rgb_gain_coeff = %.4f", handle->global_ob_gain, rgb_gain_coeff);

	isp_u_rgb_gain_block(handle->handle_device, &gain_info);

	return rtn;
}
#endif

static int32_t ae_set_monitor(void *handler, struct ae_monitor_cfg *in_param)
{
	isp_ctrl_context                *ctrl_context = (isp_ctrl_context*)handler;
	struct isp_soft_ae_cfg          *ae_cfg = NULL;

	if (NULL == ctrl_context || NULL == in_param) {
		ISP_LOGE("cxt=%p, param=%p is NULL error", ctrl_context, in_param);
		return ISP_PARAM_NULL;
	}
	if (0 == ctrl_context->need_soft_ae) {
		isp_u_raw_aem_skip_num(ctrl_context->handle_device, in_param->skip_num);
	} else {
		ae_cfg = ctrl_context->handle_soft_ae;
		ae_cfg->skip_num = in_param->skip_num + 1;
	}

	if (ctrl_context->ae_lib_fun->product_id) {
		isp_u_yiq_aem_skip_num(ctrl_context->handle_device, in_param->skip_num);
	}

	return 0;
}

static int32_t ae_set_monitor_win(void *handler, struct ae_monitor_info *in_param)
{
	isp_ctrl_context                *ctrl_context = (isp_ctrl_context*)handler;

	if (NULL == ctrl_context || NULL == in_param) {
		ISP_LOGE("cxt=%p, param=%p is NULL error", ctrl_context, in_param);
		return ISP_PARAM_NULL;
	}

	isp_u_raw_aem_offset(ctrl_context->handle_device, in_param->trim.x, in_param->trim.y);

	isp_u_raw_aem_blk_size(ctrl_context->handle_device, in_param->win_size.w, in_param->win_size.h);

	if (ctrl_context->ae_lib_fun->product_id) {
		isp_u_yiq_aem_offset(ctrl_context->handle_device, in_param->trim.x, in_param->trim.y);
		isp_u_yiq_aem_blk_size(ctrl_context->handle_device, in_param->win_size.w, in_param->win_size.h);
	}
	return 0;
}

static int32_t ae_set_monitor_bypass(void *handler, uint32_t is_bypass)
{
	isp_ctrl_context                *ctrl_context = (isp_ctrl_context*)handler;
	struct isp_soft_ae_cfg          *ae_cfg_param = NULL;

	if (NULL == ctrl_context) {
		ISP_LOGE("cxt is NULL error");
		return ISP_PARAM_NULL;
	}

	ISP_LOGI("ae bypass %d", is_bypass);
	if (0 == ctrl_context->need_soft_ae) {
		isp_u_raw_aem_bypass(ctrl_context->handle_device, &is_bypass);
	} else {
		ae_cfg_param = (struct isp_soft_ae_cfg*)ctrl_context->handle_soft_ae;
		isp_u_binning4awb_bypass(ctrl_context->handle_device, is_bypass);
		ae_cfg_param->bypass_status = 0;
	}

	if (ctrl_context->ae_lib_fun->product_id) {
		isp_u_yiq_aem_bypass(ctrl_context->handle_device, &is_bypass);
		is_bypass = 1;
		isp_u_yiq_aem_ygamma_bypass(ctrl_context->handle_device, &is_bypass);
	}

	return 0;
}

static int32_t ae_set_statistics_mode(void *handler, enum ae_statistics_mode mode, uint32_t skip_number)
{
	isp_ctrl_context                *ctrl_context = (isp_ctrl_context*)handler;

	if (NULL == ctrl_context) {
		ISP_LOGE("cxt is NULL error");
		return ISP_PARAM_NULL;
	}

	switch (mode) {
		case AE_STATISTICS_MODE_SINGLE:
			isp_u_3a_ctrl(ctrl_context->handle_device, 1);
			isp_u_raw_aem_mode(ctrl_context->handle_device, 0);
			isp_u_raw_aem_skip_num(ctrl_context->handle_device, skip_number);
			break;

		case AE_STATISTICS_MODE_CONTINUE:
			isp_u_raw_aem_mode(ctrl_context->handle_device, 1);
			isp_u_raw_aem_skip_num(ctrl_context->handle_device, 0);
			break;

		default:
			break;
	};

	return 0;
}

static int32_t ae_callback(void *handler, enum ae_cb_type cb_type)
{
	isp_ctrl_context           *ctrl_context = (isp_ctrl_context*)handler;
	enum isp_callback_cmd      cmd = 0;

	if (NULL != ctrl_context) {
		switch (cb_type) {
			case AE_CB_FLASHING_CONVERGED:
			case AE_CB_CONVERGED:
			case AE_CB_CLOSE_PREFLASH:
			case AE_CB_PREFLASH_PERIOD_END:
			case AE_CB_CLOSE_MAIN_FLASH:
				cmd = ISP_AE_STAB_CALLBACK;
				break;
			case AE_CB_QUICKMODE_DOWN:
				cmd = ISP_QUICK_MODE_DOWN;
				break;
			case AE_CB_STAB_NOTIFY:
				cmd = ISP_AE_STAB_NOTIFY;
				break;
			case AE_CB_AE_LOCK_NOTIFY:
				cmd = ISP_AE_LOCK_NOTIFY;
				break;
			case AE_CB_AE_UNLOCK_NOTIFY:
				cmd = ISP_AE_UNLOCK_NOTIFY;
				break;
			default:			
				cmd = ISP_AE_STAB_CALLBACK;
				break;
		}

		if (ctrl_context->system.callback) {
			ctrl_context->system.callback(ctrl_context->system.caller_id, ISP_CALLBACK_EVT|cmd, NULL, 0);
		}
	}

	return 0;
}

static int32_t ae_get_system_time(void *handler, uint32_t *sec, uint32_t *usec)
{
	isp_ctrl_context                *ctrl_context = (isp_ctrl_context *)handler;

	if (NULL == ctrl_context || NULL == sec || NULL == usec) {
		ISP_LOGE("ptr is null error: %p %p %p", ctrl_context, sec, usec);
		return ISP_PARAM_NULL;
	}

	isp_u_capability_time(ctrl_context->handle_device, sec, usec);

	return 0;
}

static int32_t ae_flash_get_charge(void *handler, struct ae_flash_cell *cell)
{
	int32_t                         ret = 0;
	struct isp_flash_cell           isp_cell = {0};
	isp_ctrl_context                *ctrl_context = (isp_ctrl_context*)handler;

	if (NULL == ctrl_context || NULL == cell) {
		ISP_LOGE("cxt=%p, cell=%p is NULL error", ctrl_context, cell);
		return ISP_PARAM_NULL;
	}
	switch (cell->type) {
	case AE_FLASH_TYPE_PREFLASH:
		isp_cell.type = ISP_FLASH_TYPE_PREFLASH;
		break;
	case AE_FLASH_TYPE_MAIN:
		isp_cell.type = ISP_FLASH_TYPE_MAIN;
		break;
	default:
		ISP_LOGE("not support type!!!");
		goto out;
	}
	ret = ctrl_context->system.ops.flash_get_charge(ctrl_context->system.caller_id, &isp_cell);
	if (0 == ret) {
		uint32_t i = 0;

		for (i = 0; i < isp_cell.count; ++i) {
			cell->element[i].index = isp_cell.element[i].index;
			cell->element[i].val   = isp_cell.element[i].val;
		}
		cell->count   = isp_cell.count;
		cell->def_val = isp_cell.def_val;
	}
out:
	return ret;
}

static int32_t ae_flash_get_time(void *handler, struct ae_flash_cell *cell)
{
	int32_t                         ret = 0;
	struct isp_flash_cell           isp_cell = {0};
	isp_ctrl_context                *ctrl_context = (isp_ctrl_context*)handler;

	if (NULL == ctrl_context || NULL == cell) {
		ISP_LOGE("cxt=%p, cell=%p is NULL error", ctrl_context, cell);
		return ISP_PARAM_NULL;
	}
	switch (cell->type) {
	case AE_FLASH_TYPE_PREFLASH:
		isp_cell.type = ISP_FLASH_TYPE_PREFLASH;
		break;
	case AE_FLASH_TYPE_MAIN:
		isp_cell.type = ISP_FLASH_TYPE_MAIN;
		break;
	default:
		ISP_LOGE("not support type!!!");
		goto out;
	}
	ret = ctrl_context->system.ops.flash_get_time(ctrl_context->system.caller_id, &isp_cell);
	if (0 == ret) {
		uint32_t i = 0;

		for (i = 0; i < isp_cell.count; ++i) {
			cell->element[i].index = isp_cell.element[i].index;
			cell->element[i].val   = isp_cell.element[i].val;
		}
		cell->count = isp_cell.count;
	}
out:
	return ret;
}

static int32_t ae_flash_set_charge(void *handler, uint8_t type, struct ae_flash_element *element)
{
	int32_t                         ret = 0;
	struct isp_flash_element        isp_ele = {0};
	uint8_t                         isp_type = 0;
	isp_ctrl_context                *ctrl_context = (isp_ctrl_context*)handler;

	if (NULL == ctrl_context || NULL == element) {
		ISP_LOGE("cxt=%p, element=%p is NULL error", ctrl_context, element);
		return ISP_PARAM_NULL;
	}
	switch (type) {
	case AE_FLASH_TYPE_PREFLASH:
		isp_type = ISP_FLASH_TYPE_PREFLASH;
		break;
	case AE_FLASH_TYPE_MAIN:
		isp_type = ISP_FLASH_TYPE_MAIN;
		break;
	default:
		ISP_LOGE("not support type!!!");
		goto out;
	}
	isp_ele.index = element->index;
	isp_ele.val   = element->val;
	ret = ctrl_context->system.ops.flash_set_charge(ctrl_context->system.caller_id,\
													isp_type, &isp_ele);
out:
	return ret;
}

static int32_t ae_flash_set_time(void *handler, uint8_t type, struct ae_flash_element *element)
{
	int32_t                         ret = 0;
	struct isp_flash_element        isp_ele = {0};
	uint8_t                         isp_type = 0;
	isp_ctrl_context                *ctrl_context = (isp_ctrl_context*)handler;

	if (NULL == ctrl_context || NULL == element) {
		ISP_LOGE("cxt=%p, element=%p is NULL error", ctrl_context, element);
		return ISP_PARAM_NULL;
	}
	switch (type) {
	case AE_FLASH_TYPE_MAIN:
		isp_type = ISP_FLASH_TYPE_MAIN;
		break;
	default:
		ISP_LOGE("not support type!!!");
		goto out;
	}
	isp_ele.index = element->index;
	isp_ele.val   = element->val;
	ret = ctrl_context->system.ops.flash_set_time(ctrl_context->system.caller_id,\
													isp_type, &isp_ele);
out:
	return ret;
}

static int32_t _ae_init(isp_ctrl_context *handle)
{
	int32_t                         rtn = ISP_SUCCESS;
	struct isp_flash_param *flash = NULL;
	struct isp_pm_ioctl_input       input = {0};
	struct isp_pm_ioctl_output      output = {0};
	struct ae_init_in               init_in_param;
	struct ae_init_out              init_result = {0};
	struct isp_pm_param_data        *param_data = NULL;
	uint32_t                        i = 0;
	uint32_t                        num = 0;

	memset((void*)&init_in_param, 0, sizeof(init_in_param));

	rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_INIT_AE, &input, &output);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("AE_TAG: get ae init param failed");
		return rtn;
	}
	ISP_LOGI(" _ae_init");

	if (0 == output.param_num) {
		ISP_LOGE("ISP_AE: ae param num=%d", output.param_num);
		return ISP_ERROR;
	}

	param_data = output.param_data;
	init_in_param.has_force_bypass = param_data->user_data[0];
	for (i = 0; i < output.param_num; ++i) {
		if (NULL != param_data->data_ptr) {
			init_in_param.param[num].param = param_data->data_ptr;
			init_in_param.param[num].size  = param_data->data_size;

			ISP_LOGV("ISP_AE: ae init param[%d]=(%p, %d)", num,
				init_in_param.param[num].param,
				init_in_param.param[num].size);

			++num;
		}
		++param_data;
	}
	init_in_param.param_num                         = num;
	init_in_param.resolution_info.frame_size.w      = handle->src.w;
	init_in_param.resolution_info.frame_size.h      = handle->src.h;
	init_in_param.resolution_info.frame_line        = handle->input_size_trim[1].frame_line;
	init_in_param.resolution_info.line_time         = handle->input_size_trim[1].line_time;
	init_in_param.resolution_info.sensor_size_index = 1;
	init_in_param.monitor_win_num.w                 = 32;
	init_in_param.monitor_win_num.h                 = 32;
	init_in_param.camera_id                         = handle->camera_id;
	init_in_param.isp_ops.isp_handler               = handle;
	init_in_param.isp_ops.set_again                 = ae_set_again;
	init_in_param.isp_ops.set_exposure              = ae_set_exposure;
	init_in_param.isp_ops.set_monitor_win           = ae_set_monitor_win;
	init_in_param.isp_ops.set_monitor               = ae_set_monitor;
	init_in_param.isp_ops.callback                  = ae_callback;
	init_in_param.isp_ops.set_monitor_bypass        = ae_set_monitor_bypass;
	init_in_param.isp_ops.get_system_time           = ae_get_system_time;
	init_in_param.isp_ops.set_statistics_mode       = ae_set_statistics_mode;

	init_in_param.isp_ops.flash_get_charge          = ae_flash_get_charge;
	init_in_param.isp_ops.flash_get_time            = ae_flash_get_time;
	init_in_param.isp_ops.flash_set_charge          = ae_flash_set_charge;
	init_in_param.isp_ops.flash_set_time            = ae_flash_set_time;
#ifdef CONFIG_CAMERA_ISP_AE_VERSION_V1
	init_in_param.isp_ops.set_rgb_gain 				= ae_set_rgb_gain;
#else
	init_in_param.isp_ops.set_rgb_gain 				= NULL;
#endif
	if (NULL == handle
	|| NULL == handle->ioctrl_ptr
	|| NULL == handle->ioctrl_ptr->ex_set_exposure) {
		init_in_param.isp_ops.ex_set_exposure = PNULL;
	} else {
		init_in_param.isp_ops.ex_set_exposure = ae_ex_set_exposure;
	}

	if (AL_AE_LIB == handle->ae_lib_fun->product_id) {
		//AIS needs AWB infomation at Init
		struct isp_otp_info *otp_info = &handle->otp_info;
		struct isp_cali_awb_info *awb_cali_info = otp_info->awb.data_ptr;

		init_in_param.lsc_otp_golden = otp_info->lsc_golden;
		init_in_param.lsc_otp_random = otp_info->lsc_random;
		init_in_param.lsc_otp_width  = otp_info->width;
		init_in_param.lsc_otp_height = otp_info->height;
		if (NULL != awb_cali_info) {
			init_in_param.otp_info.gldn_stat_info.r = awb_cali_info->golden_avg[0];
			init_in_param.otp_info.gldn_stat_info.g = awb_cali_info->golden_avg[1];
			init_in_param.otp_info.gldn_stat_info.b = awb_cali_info->golden_avg[2];
			init_in_param.otp_info.rdm_stat_info.r  = awb_cali_info->ramdon_avg[0];
			init_in_param.otp_info.rdm_stat_info.g  = awb_cali_info->ramdon_avg[1];
			init_in_param.otp_info.rdm_stat_info.b  = awb_cali_info->ramdon_avg[2];
		}
	}
	handle->handle_ae = handle->ae_lib_fun->ae_init(&init_in_param, &handle->ae_reserve);
	if (NULL == handle->handle_ae) {
		rtn = ISP_ERROR;
		ISP_LOGE("ISP_AE: ae init failed");
	}

	rtn = _isp_get_flash_cali_param(handle->handle_pm, &flash);
	ISP_LOGE(" flash param rgb ratio = (%d,%d,%d), auto_flash_thr = %d",
	flash->cur.r_ratio, flash->cur.g_ratio, flash->cur.b_ratio, flash->cur.auto_flash_thr);
	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_FLASH_ON_OFF_THR,
		(void*)&flash->cur.auto_flash_thr, NULL);

	ISP_LOGD("ISP_AE: ae handle=%p, e %d g %d d %d en %d", rtn,
					handle->ae_reserve.cur_exp_line,
					handle->ae_reserve.cur_gain,
					handle->ae_reserve.cur_dummy,
					handle->ae_reserve.enable);
	return rtn;
}

static void _ae_deinit(isp_ctrl_context* handle)
{
	int32_t rtn = ISP_SUCCESS;

	ISP_LOGV("ISP_AE: ae handle=%p", handle->handle_ae);

	if (NULL != handle->handle_ae) {
		rtn = handle->ae_lib_fun->ae_deinit(handle->handle_ae, NULL, &(handle->ae_reserve));
		ISP_LOGD("ISP_AE: ae deinit rtn = %d e %d g %d d %d en %d", rtn,
					handle->ae_reserve.cur_exp_line,
					handle->ae_reserve.cur_gain,
					handle->ae_reserve.cur_dummy,
					handle->ae_reserve.enable);
		handle->handle_ae = PNULL;
	}
}

static int32_t _ae_calc(isp_ctrl_context* handle, struct isp_aem_info *aem_info, struct ae_calc_out *result)
{
	int32_t                         rtn = ISP_SUCCESS;
	struct ae_calc_in               in_param;
	struct awb_gain                 gain;

	handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_GET_GAIN, (void*)&gain, NULL);

	memset((void*)&in_param, 0, sizeof(in_param));
	if ((0 == gain.r) || (0 == gain.g) || (0 == gain.b)) {
		in_param.awb_gain_r = 1024;
		in_param.awb_gain_g = 1024;
		in_param.awb_gain_b = 1024;
	} else {
		in_param.awb_gain_r = gain.r;
		in_param.awb_gain_g = gain.g;
		in_param.awb_gain_b = gain.b;
	}

	ISP_LOGV("Hist: src: w:%d, h:%d", handle->src.w, handle->src.h);

	in_param.stat_fmt = aem_info->stat_fmt;
	if (AE_AEM_FMT_RGB & in_param.stat_fmt) {
		in_param.rgb_stat_img = (uint32_t*)aem_info->rgb_stat->r_info;
		in_param.stat_img     = (uint32_t*)aem_info->rgb_stat->r_info;
	}
	if (AE_AEM_FMT_YIQ & in_param.stat_fmt) {
		in_param.yiq_stat_img = (uint32_t*)aem_info->yiq_stat->y;
	}

	in_param.sec  = handle->system_time_ae.sec;
	in_param.usec = handle->system_time_ae.usec;

	rtn = handle->ae_lib_fun->ae_calculation(handle->handle_ae, &in_param, result);

	if (AL_AE_LIB == handle->ae_lib_fun->product_id) {
		handle->log_alc_ae      = result->log_ae.log;
		handle->log_alc_ae_size = result->log_ae.size;
	}

	return rtn;
}

static int32_t _awb_init(isp_ctrl_context *handle)
{
	int32_t                         rtn = ISP_SUCCESS;
	struct isp_pm_ioctl_input       input;
	struct isp_pm_ioctl_output      output;
	struct awb_ctrl_init_param      param;
	struct awb_ctrl_init_result     result;
	struct ae_monitor_info          info;

	memset((void*)&input, 0, sizeof(input));
	memset((void*)&output, 0, sizeof(output));
	memset((void*)&param, 0, sizeof(param));
	memset((void*)&result, 0, sizeof(result));	
	
	if(handle->sn_raw_info->libuse_info->awb_lib_info.version_id & 0x01)
		rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_INIT_AWB_NEW, &input, &output);
	else
		rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_INIT_AWB, &input, &output);
	
	ISP_TRACE_IF_FAIL(rtn, ("get awb init param error"));

	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_MONITOR_INFO, NULL, (void*)&info);
	if (ISP_SUCCESS == rtn) {
		if (AL_AE_LIB == handle->ae_lib_fun->product_id) {
			void *ais_handle = NULL;

			rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_AIS_HANDLE, NULL, (void*)&ais_handle);
			ISP_TRACE_IF_FAIL(rtn, ("ae get ais handle error"));
			param.priv_handle = ais_handle;
			param.awb_enable  = 1;
		} else {
			//if use AIS, AWB this does not  need for awb_ctrl
			struct isp_otp_info *otp_info = &handle->otp_info;
			struct isp_cali_awb_info *awb_cali_info = otp_info->awb.data_ptr;

			param.camera_id       = handle->camera_id;
			param.base_gain       = 1024;
			param.awb_enable      = 1;
			param.wb_mode         = 0;
			param.stat_img_format = AWB_CTRL_STAT_IMG_CHN;
			param.stat_img_size.w = info.win_num.w;
			param.stat_img_size.h = info.win_num.h;
			param.stat_win_size.w = info.win_size.w;
			param.stat_win_size.h = info.win_size.h;
			param.tuning_param    = output.param_data->data_ptr;
			param.param_size      = output.param_data->data_size;
			param.lsc_otp_golden  = otp_info->lsc_golden;
			param.lsc_otp_random  = otp_info->lsc_random;
			param.lsc_otp_width   = otp_info->width;
			param.lsc_otp_height  = otp_info->height;

			if (NULL != awb_cali_info) {
				param.otp_info.gldn_stat_info.r = awb_cali_info->golden_avg[0];
				param.otp_info.gldn_stat_info.g = awb_cali_info->golden_avg[1];
				param.otp_info.gldn_stat_info.b = awb_cali_info->golden_avg[2];
				param.otp_info.rdm_stat_info.r  = awb_cali_info->ramdon_avg[0];
				param.otp_info.rdm_stat_info.g  = awb_cali_info->ramdon_avg[1];
				param.otp_info.rdm_stat_info.b  = awb_cali_info->ramdon_avg[2];
			}

			ISP_LOGV("AWB_TAG: param = %p, size = %d.\n", param.tuning_param, param.param_size);
		}
		handle->handle_awb = handle->awb_lib_fun->awb_ctrl_init(&param, &result);
		ISP_TRACE_IF_FAIL(!handle->handle_awb, ("isp_awb_init error"));

		ISP_LOGV("AWB_TAG init: handle=%p", handle->handle_awb);

		if (result.use_ccm) {
			struct isp_pm_param_data param_data;
			struct isp_pm_ioctl_input input = {NULL, 0};
			struct isp_pm_ioctl_output output = {NULL, 0};

			memset(&param_data, 0x0, sizeof(param_data));
			BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_CMC10, ISP_BLK_CMC10, result.ccm, 9*sizeof(uint16_t));

			rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_OTHERS, &input, &output);
			ISP_TRACE_IF_FAIL(rtn, ("pm init isp block param error"));
		}

		if (result.use_lsc) {
			struct isp_pm_param_data param_data;
			struct isp_pm_ioctl_input input = {NULL, 0};
			struct isp_pm_ioctl_output output = {NULL, 0};

			memset(&param_data, 0x0, sizeof(param_data));
			BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_LSC_MEM_ADDR, ISP_BLK_2D_LSC, result.lsc, result.lsc_size);

			rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_OTHERS, &input, &output);
			ISP_TRACE_IF_FAIL(rtn, ("pm init isp block param error"));
		}
	} else {
		ISP_LOGE("AWB_TAG: get awb init param failed!");
	}

	return rtn;
}

static int32_t _awb_calc(isp_ctrl_context *handle, struct awb_ctrl_ae_info *ae_info, struct isp_ae_grgb_statistic_info *stat_info, struct awb_ctrl_calc_result *result)
{
	int32_t                         rtn = ISP_SUCCESS;
	struct awb_ctrl_calc_param      param;
	struct isp_pm_ioctl_input       ioctl_input;
	struct isp_pm_param_data        ioctl_data;
	struct isp_awbc_cfg             awbc_cfg;
	 struct ae_monitor_info          info; //get monitor

	memset((void*)&param, 0, sizeof(param));
	memset((void*)&ioctl_input, 0, sizeof(ioctl_input));
	memset((void*)&ioctl_data, 0, sizeof(ioctl_data));
	memset((void*)&awbc_cfg, 0, sizeof(awbc_cfg));

	param.quick_mode         = 0;
	param.stat_img.chn_img.r = stat_info->r_info;
	param.stat_img.chn_img.g = stat_info->g_info;
	param.stat_img.chn_img.b = stat_info->b_info;
	param.bv                 = ae_info->bv;

//ALC_S
	//param.ae_info           = *ae_info; //compile error
	param.ae_info.bv		= ae_info->bv;
	param.ae_info.iso		= ae_info->iso;
	param.ae_info.gain		= ae_info->gain;
	param.ae_info.exposure	= ae_info->exposure;
	param.ae_info.f_value   = ae_info->f_value;
	param.ae_info.stable	= ae_info->stable;
	param.ae_info.ev_index  = ae_info->ev_index;
	memcpy(param.ae_info.ev_table, ae_info->ev_table, 16*sizeof(int32_t));
//ALC_E
     rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_MONITOR_INFO, NULL, (void*)&info);
     param.scalar_factor = (info.win_size.h/2)*(info.win_size.w/2);
// simulation info
	if (1)
	{
		struct isp_pm_ioctl_input io_pm_input = {0x00};
		struct isp_pm_ioctl_output io_pm_output= {PNULL, 0};
		struct isp_pm_param_data pm_param = {0x00};

		// LSC
		BLOCK_PARAM_CFG(io_pm_input, pm_param, ISP_PM_BLK_LSC_INFO, ISP_BLK_2D_LSC, PNULL, 0);
		rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, (void*)&io_pm_input, (void*)&io_pm_output);
		struct isp_lsc_info *lsc_info = (struct isp_lsc_info *)io_pm_output.param_data->data_ptr;
		param.lsc_width = lsc_info->gain_w;
		param.lsc_height = lsc_info->gain_h;
		if (lsc_info->data_ptr != NULL)
		{
			memcpy(param.lsc_table, lsc_info->data_ptr, lsc_info->len);
		}

		// CMC
		BLOCK_PARAM_CFG(io_pm_input, pm_param, ISP_PM_BLK_CMC10, ISP_BLK_CMC10, 0, 0);
		rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, &io_pm_input, &io_pm_output);
		uint16_t* cmc_info = io_pm_output.param_data->data_ptr;
		if (cmc_info != NULL)
		{
			#define CMC10(n) (((n)>>13)?((n)-(1<<14)):(n))
			int i;
			for (i=0; i<9; i++)
			{
				param.matrix[i] = CMC10(cmc_info[i]);
			}
		}

		// GAMMA
		BLOCK_PARAM_CFG(io_pm_input, pm_param, ISP_PM_BLK_ISP_SETTING, ISP_BLK_RGB_GAMC, 0, 0);
		rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, &io_pm_input, &io_pm_output);
		struct isp_dev_gamma_info_v1* gamma_info = io_pm_output.param_data->data_ptr;
		if (gamma_info != NULL)
		{
			int i;
			for (i=0; i<256; i++)
			{
				param.gamma[i] = (gamma_info->nodes[i].node_y + gamma_info->nodes[i+1].node_y) / 2;
			}
		}
	}
	

	rtn = handle->awb_lib_fun->awb_ctrl_calculation((awb_ctrl_handle_t)handle->handle_awb, &param, result);
	ISP_LOGV("AWB_TAG calc: rtn=%d, gain=(%d, %d, %d), ct=%d", rtn, result->gain.r, result->gain.g, result->gain.b, result->ct);

	/*set awb gain*/
	awbc_cfg.r_gain            = result->gain.r;
	awbc_cfg.g_gain            = result->gain.g;
	awbc_cfg.b_gain            = result->gain.b;
	awbc_cfg.r_offset          = 0;
	awbc_cfg.g_offset          = 0;
	awbc_cfg.b_offset          = 0;

	ioctl_data.id              = ISP_BLK_AWB_V1;
	ioctl_data.cmd             = ISP_PM_BLK_AWBC;
	ioctl_data.data_ptr        = &awbc_cfg;
	ioctl_data.data_size       = sizeof(awbc_cfg);

	ioctl_input.param_data_ptr = &ioctl_data;
	ioctl_input.param_num      = 1;

	if (0 == awbc_cfg.r_gain && 0 == awbc_cfg.g_gain && 0 == awbc_cfg.b_gain) {
		awbc_cfg.r_gain = 1800;
		awbc_cfg.g_gain = 1024;
		awbc_cfg.b_gain = 1536;
	}
	rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_AWB, (void*)&ioctl_input, NULL);
	ISP_LOGV("AWB_TAG: isp_pm_ioctl rtn=%d, gain=(%d, %d, %d)", rtn, awbc_cfg.r_gain, awbc_cfg.g_gain, awbc_cfg.b_gain);

	if (result->use_ccm) {
		struct isp_pm_param_data param_data;
		struct isp_pm_ioctl_input input = {NULL, 0};
		struct isp_pm_ioctl_output output = {NULL, 0};

		memset(&param_data, 0x0, sizeof(param_data));
		BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_CMC10, ISP_BLK_CMC10, result->ccm, 9*sizeof(uint16_t));

		rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_OTHERS, &input, &output);
		ISP_TRACE_IF_FAIL(rtn, ("pm init isp block param error"));
	}
	
	handle->log_awb      = result->log_awb.log;
	handle->log_awb_size = result->log_awb.size;

	if (result->use_lsc) {
		struct isp_pm_param_data param_data;
		struct isp_pm_ioctl_input input = {NULL, 0};
		struct isp_pm_ioctl_output output = {NULL, 0};

		memset(&param_data, 0x0, sizeof(param_data));
		BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_LSC_MEM_ADDR, ISP_BLK_2D_LSC, result->lsc, result->lsc_size);

		rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_OTHERS, &input, &output);
		ISP_TRACE_IF_FAIL(rtn, ("pm init isp block param error"));

		handle->log_lsc = result->log_lsc.log;
		handle->log_lsc_size = result->log_lsc.size;
	}

	handle->awb_pg_flag = result->pg_flag;
	handle->green100 = result->green100;

	return rtn;
}

static void _awb_deinit(isp_ctrl_context* handle)
{
	int32_t                         rtn = ISP_SUCCESS;

	struct isp_system* isp_system_ptr = &handle->system;
	CMR_MSG_INIT(msg);

	msg.msg_type = ISP_PROC_AWB_STOP;
	msg.sync_flag = CMR_MSG_SYNC_PROCESSED;
	rtn = cmr_thread_msg_send(isp_system_ptr->thread_awb_proc, &msg);
	if(rtn) {
		ISP_LOGE("Send msg failed");
	}

	rtn = handle->awb_lib_fun->awb_ctrl_deinit((awb_ctrl_handle_t)handle->handle_awb, NULL, NULL);
	ISP_LOGV("_awb_deinit: handle=%p, rtn=%d", handle->handle_awb, rtn);
	handle->handle_awb = PNULL;
}


//gerald merge start, http://review.source.spreadtrum.com/gerrit/#/c/281621
/* // for LSC2.X neet to reopen
void _alsc_set_param(struct lsc_adv_init_param *lsc_param)
{
	//	alg_open
	//	0: front_camera close, back_camera close;
	//	1: front_camera open, back_camera open;
	//	2: front_camera close, back_camera open;
	lsc_param->alg_open = 1;
       	lsc_param->tune_param.enable = 1;
#if SMART_LSC_VERSION	   
	lsc_param->tune_param.alg_id = 2;
#else
	lsc_param->tune_param.alg_id = 0;
#endif
	//common
	lsc_param->tune_param.strength_level = 5;
	lsc_param->tune_param.pa = 1;
	lsc_param->tune_param.pb = 1;
	lsc_param->tune_param.fft_core_id = 0;
	lsc_param->tune_param.con_weight = 5;		//double avg weight:[1~16]; weight =100 for 10 tables avg
	lsc_param->tune_param.restore_open = 0;
	lsc_param->tune_param.freq = 3; //gerald merge http://review.source.spreadtrum.com/gerrit/#/c/282531
	
	//Smart LSC Setting //gerald merge  start http://review.source.spreadtrum.com/gerrit/#/c/286361
	lsc_param->SLSC_setting.num_seg_queue = 10;
	lsc_param->SLSC_setting.num_seg_vote_th = 8;
	lsc_param->SLSC_setting.proc_inter = 3;
	lsc_param->SLSC_setting.seg_count = 4;
	lsc_param->SLSC_setting.smooth_ratio = 0.9;
	lsc_param->SLSC_setting.segs[0].table_pair[0]=LSC_TAB_A;
	lsc_param->SLSC_setting.segs[0].table_pair[1]=LSC_TAB_TL84;
	lsc_param->SLSC_setting.segs[0].max_CT = 3999;
	lsc_param->SLSC_setting.segs[0].min_CT = 0;
	lsc_param->SLSC_setting.segs[1].table_pair[0]=LSC_TAB_A;
	lsc_param->SLSC_setting.segs[1].table_pair[1]=LSC_TAB_CWF;
	lsc_param->SLSC_setting.segs[1].max_CT = 3999;
	lsc_param->SLSC_setting.segs[1].min_CT = 0;
	lsc_param->SLSC_setting.segs[2].table_pair[0]=LSC_TAB_D65;
	lsc_param->SLSC_setting.segs[2].table_pair[1]=LSC_TAB_TL84;
	lsc_param->SLSC_setting.segs[2].max_CT = 10000;
	lsc_param->SLSC_setting.segs[2].min_CT = 0;
	lsc_param->SLSC_setting.segs[3].table_pair[0]=LSC_TAB_D65;
	lsc_param->SLSC_setting.segs[3].table_pair[1]=LSC_TAB_CWF;
	lsc_param->SLSC_setting.segs[3].max_CT = 10000;
	lsc_param->SLSC_setting.segs[3].min_CT = 0;

	lsc_param->SLSC_setting.segs[4].table_pair[0]=LSC_TAB_D65;
	lsc_param->SLSC_setting.segs[4].table_pair[1]=LSC_TAB_DNP;
	lsc_param->SLSC_setting.segs[4].max_CT = 10000;
	lsc_param->SLSC_setting.segs[4].min_CT = 0;
}
*/

uint32_t _alsc_init(isp_ctrl_context* handle)
{

	uint32_t rtn = ISP_SUCCESS;
	int i = 0;
	lsc_adv_handle_t lsc_adv_handle = NULL;
	struct lsc_adv_init_param lsc_param = {0};
	isp_pm_handle_t pm_handle = handle->handle_pm;
	struct isp_pm_ioctl_input   get_pm_input = {0};
       struct isp_pm_ioctl_output get_pm_output = {0};
	struct isp_pm_ioctl_input  pm_tab_input = {NULL, 0};
	struct isp_pm_ioctl_output  pm_tab_output = {NULL, 0};
	struct isp_pm_param_data pm_tab_param = {0};
	struct isp_pm_ioctl_input io_pm_input = {0x00};
	struct isp_pm_ioctl_output io_pm_output= {PNULL, 0};
	struct isp_pm_param_data pm_param = {0x00};	
	
       rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_INIT_ALSC, &get_pm_input, &get_pm_output);
       if (ISP_SUCCESS != rtn) {
	   	ISP_LOGE("ALSC_TAG: get alsc init param failed");
       }

	BLOCK_PARAM_CFG(pm_tab_input, pm_tab_param, ISP_PM_BLK_LSC_GET_LSCTAB, ISP_BLK_2D_LSC, NULL, 0);
	rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, (void*)&pm_tab_input, (void*)&pm_tab_output);
	handle->lsc_tab_address = pm_tab_output.param_data->data_ptr;	
	struct isp_2d_lsc_param* lsc_tab_param_ptr =  (struct isp_2d_lsc_param *)(handle->lsc_tab_address);

	BLOCK_PARAM_CFG(io_pm_input, pm_param, ISP_PM_BLK_LSC_INFO, ISP_BLK_2D_LSC, PNULL, 0);
	rtn = isp_pm_ioctl(pm_handle, ISP_PM_CMD_GET_SINGLE_SETTING, (void*)&io_pm_input, (void*)&io_pm_output);
	struct isp_lsc_info *lsc_info = (struct isp_lsc_info *)io_pm_output.param_data->data_ptr;

	ISP_LOGI(" _alsc_init");

       if( get_pm_output.param_data->data_size != sizeof(struct lsc2_tune_param) ){
	   	lsc_param.tune_param_ptr = NULL;
		ISP_LOGE("ALSC_TAG: get alsc param from sensor file failed");
	}else{
	       lsc_param.tune_param_ptr = get_pm_output.param_data->data_ptr;
	}
	
	//_alsc_set_param(&lsc_param);   // for LSC2.X neet to reopen	

	for(i=0;i<9;i++){
		lsc_param.lsc_tab_address[i] = lsc_tab_param_ptr->map_tab[i].param_addr;
	}

	lsc_param.gain_width = lsc_info->gain_w;
	lsc_param.gain_height = lsc_info->gain_h;
	lsc_param.lum_gain = (uint16_t*)lsc_info->data_ptr;
       lsc_param.grid = lsc_info->grid;//gerald merge  http://review.source.spreadtrum.com/gerrit/#/c/286361
  
	switch (handle->image_pattern) {
		case SENSOR_IMAGE_PATTERN_RAWRGB_GR:
			lsc_param.gain_pattern = LSC_GAIN_PATTERN_RGGB;
			break;

		case SENSOR_IMAGE_PATTERN_RAWRGB_R:
			lsc_param.gain_pattern = LSC_GAIN_PATTERN_GRBG;
			break;

		case SENSOR_IMAGE_PATTERN_RAWRGB_B:
			lsc_param.gain_pattern = LSC_GAIN_PATTERN_GBRG;
			break;

		case SENSOR_IMAGE_PATTERN_RAWRGB_GB:
			lsc_param.gain_pattern = LSC_GAIN_PATTERN_BGGR;
			break;

		default:
			break;
	}

	if (NULL == handle->handle_lsc_adv) {
		
		//ISP_LOGV("_awb_deinit[gerald]: handle->lsc_lib_fun=%p", handle->lsc_lib_fun);
		//ISP_LOGV("_awb_deinit[gerald]: handle->lsc_lib_fun->alsc_init=%p", handle->lsc_lib_fun->alsc_init);
		
		lsc_adv_handle = handle->lsc_lib_fun->alsc_init(&lsc_param);
		if (NULL == lsc_adv_handle) {
			ALOGE("lsc adv init failed");
			return ISP_ERROR;
		}

		handle->handle_lsc_adv = lsc_adv_handle;
	}

	return rtn;
}

int32_t _alsc_calc(isp_ctrl_context *handle,
					uint32_t* ae_stat_r, uint32_t* ae_stat_g, uint32_t* ae_stat_b,
					struct awb_size *stat_img_size,
					struct awb_size *win_size,
					int image_width, int image_height,//gerald merge http://review.source.spreadtrum.com/gerrit/#/c/286361
					uint32_t awb_ct, int awb_r_gain, int awb_b_gain,
					uint32_t ae_stable)
{
	int32_t	rtn = ISP_SUCCESS;
#ifdef 	LSC_ADV_ENABLE
       struct alsc_ver_info lsc_ver = {0};
	rtn  = handle->lsc_lib_fun->alsc_io_ctrl(handle->handle_lsc_adv, ALSC_GET_VER, NULL, (void *)&lsc_ver);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("Get ALSC ver info failed!");
	}
	
if( lsc_ver.LSC_SPD_VERSION >= 2 ){
	lsc_adv_handle_t lsc_adv_handle = handle->handle_lsc_adv;
	isp_pm_handle_t pm_handle = handle->handle_pm;
	struct isp_pm_ioctl_input io_pm_input = {0x00};
	struct isp_pm_ioctl_output io_pm_output= {PNULL, 0};
	struct isp_pm_param_data pm_param = {0x00};
	uint32_t i = 0;
	int32_t bv0 = 0;
	if (NULL == ae_stat_r) {
		ALOGE("invalid stat info param");
		return ISP_ERROR;
	}
	//rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_BV_BY_LUM, NULL, (void *)&bv0);
	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_BV_BY_LUM_NEW, NULL, (void *)&bv0);
	BLOCK_PARAM_CFG(io_pm_input, pm_param, ISP_PM_BLK_LSC_INFO, ISP_BLK_2D_LSC, PNULL, 0);
	rtn = isp_pm_ioctl(pm_handle, ISP_PM_CMD_GET_SINGLE_SETTING, (void*)&io_pm_input, (void*)&io_pm_output);
	struct isp_lsc_info *lsc_info = (struct isp_lsc_info *)io_pm_output.param_data->data_ptr;
	struct isp_2d_lsc_param * lsc_tab_param_ptr =  (struct isp_2d_lsc_param *)(handle->lsc_tab_address);

	struct lsc_adv_calc_param calc_param;
	struct lsc_adv_calc_result calc_result = {0};
	memset(&calc_param, 0, sizeof(calc_param));

	calc_result.dst_gain = (uint16_t *)lsc_info->data_ptr;
	calc_param.stat_img.r  = ae_stat_r;
	calc_param.stat_img.gr = ae_stat_g;
	calc_param.stat_img.gb  = ae_stat_g;
	calc_param.stat_img.b  = ae_stat_b;
	calc_param.stat_size.w = stat_img_size->w;
	calc_param.stat_size.h = stat_img_size->h;
	calc_param.gain_width = lsc_info->gain_w;
	calc_param.gain_height = lsc_info->gain_h;
	calc_param.block_size.w = win_size->w;
	calc_param.block_size.h = win_size->h;
	calc_param.lum_gain = (uint16_t *)lsc_info->param_ptr;
	calc_param.ct = awb_ct;
	calc_param.r_gain = awb_r_gain;
	calc_param.b_gain = awb_b_gain;
	
	gAWBGainR = awb_r_gain;//gerald merge  http://review.source.spreadtrum.com/gerrit/#/c/286361
	gAWBGainB = awb_b_gain;//gerald merge  http://review.source.spreadtrum.com/gerrit/#/c/286361
	
	calc_param.bv = bv0;
	calc_param.ae_stable = ae_stable;
	calc_param.isp_mode = handle->isp_mode;
	calc_param.isp_id = ISP_2_0;
	calc_param.camera_id = handle->camera_id;
	calc_param.awb_pg_flag = handle->awb_pg_flag;
	
	calc_param.img_size.w = image_width;//gerald merge  http://review.source.spreadtrum.com/gerrit/#/c/286361
	calc_param.img_size.h = image_height;//gerald merge  http://review.source.spreadtrum.com/gerrit/#/c/286361
	
	for(i=0;i<9;i++){
		calc_param.lsc_tab_address[i] = lsc_tab_param_ptr->map_tab[i].param_addr;
	}
	calc_param.lsc_tab_size = handle->lsc_tab_size;

	/*AF lock ALSC*/
	//ISP_LOGI("[gerald] isp_smart_lsc_lock:%d", handle->isp_smart_lsc_lock);
	
	if (handle->isp_smart_lsc_lock == 0)
	{
		//ISP_LOGI("[gerald] lsc_lib_fun alsc_calc start");
		uint64_t time0 = systemTime(CLOCK_MONOTONIC);
		rtn = handle->lsc_lib_fun->alsc_calc(lsc_adv_handle, &calc_param, &calc_result);
		if (ISP_SUCCESS != rtn) {
			ALOGE("lsc adv gain map calc error");
			return rtn;
		}
 		uint64_t time1 = systemTime(CLOCK_MONOTONIC);
		//ALOGE("ALSC2 system time alsc_proc time =  %dus", (int)((time1-time0)/1000));

		BLOCK_PARAM_CFG(io_pm_input, pm_param, ISP_PM_BLK_LSC_INFO, ISP_BLK_2D_LSC, PNULL, 0);
		io_pm_input.param_data_ptr = &pm_param;
		rtn = isp_pm_ioctl(pm_handle, ISP_PM_CMD_SET_OTHERS, &io_pm_input, NULL);
	}

}
#endif
	return rtn;
}

void _alsc_deinit(isp_ctrl_context *handle)
{
	uint32_t                        rtn = ISP_SUCCESS;
	lsc_adv_handle_t                lsc_adv_handle = handle->handle_lsc_adv;
	struct isp_system* isp_system_ptr = &handle->system;
	CMR_MSG_INIT(msg);

	msg.msg_type = ISP_PROC_LSC_STOP;
	msg.sync_flag = CMR_MSG_SYNC_PROCESSED;
	rtn = cmr_thread_msg_send(isp_system_ptr->thread_lsc_proc, &msg);
	if(rtn){
		ALOGE("ALSC2 send stop msg failed");
	}
	rtn = handle->lsc_lib_fun->alsc_deinit(lsc_adv_handle);

	handle->handle_lsc_adv = NULL;
}
//gerald merge end, http://review.source.spreadtrum.com/gerrit/#/c/281621

static int32_t _af_init(isp_ctrl_context *handle)
{
	if(NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_init_interface)
		handle->af_lib_fun->af_init_interface(handle);
	return ISP_SUCCESS;
}

static void _af_deinit(isp_ctrl_context *handle)
{
	if(NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_deinit_interface)
		handle->af_lib_fun->af_deinit_interface(handle);
	handle->handle_af = PNULL;
}

int32_t _af_calc(isp_ctrl_context *handle, uint32_t data_type, void *in_param, struct af_result_param *result)
{
	int32_t                         rtn = ISP_SUCCESS;

	struct af_calc_param calc_param;
	memset((void*)&calc_param, 0, sizeof(calc_param));
	ISP_LOGE("AF_TAG:_af_calc data type %d",data_type);
	switch (data_type) {
	case AF_DATA_AF:{
		struct af_filter_info afm_param;
		struct af_filter_data afm_data;

		memset((void*)&afm_param, 0, sizeof(afm_param));
		memset((void*)&afm_data, 0, sizeof(afm_data));
		afm_data.data = (uint64_t *)in_param;
		afm_data.type = 1;
		afm_param.filter_num = 1;
		afm_param.filter_data = &afm_data;
		calc_param.data_type = AF_DATA_AF;
		calc_param.data = (void*)(&afm_param);
		rtn = af_calculation(handle->handle_af,&calc_param,result);
		break;
	}
	case AF_DATA_IMG_BLK:{
		struct af_img_blk_info img_blk_info;

		memset((void*)&img_blk_info, 0, sizeof(img_blk_info));
		img_blk_info.block_w = 32;
		img_blk_info.block_h = 32;
		img_blk_info.chn_num = 3;
		img_blk_info.pix_per_blk = 1;
		img_blk_info.data = (uint32_t *)in_param;
		calc_param.data_type = AF_DATA_IMG_BLK;
		calc_param.data = (void*)(&img_blk_info);
		rtn = af_calculation(handle->handle_af,&calc_param,result);
		break;
	}
	case AF_DATA_AE:{
		struct af_ae_info ae_info;
		struct ae_calc_out *ae_result = (struct ae_calc_out *)in_param;
		uint32_t line_time = ae_result->line_time;
		uint32_t frame_len = ae_result->frame_line;
		uint32_t dummy_line = ae_result->cur_dummy;
		uint32_t exp_line= ae_result->cur_exp_line;
		uint32_t frame_time;

		memset((void*)&ae_info, 0, sizeof(ae_info));
		ae_info.exp_time = ae_result->cur_exp_line*line_time/10;
		ae_info.gain = ae_result->cur_again;
		frame_len = (frame_len > (exp_line+dummy_line))? frame_len : (exp_line+dummy_line);
		frame_time = frame_len*line_time/10;
		frame_time = frame_time > 0 ? frame_time : 1;
		ae_info.cur_fps = 1000000/frame_time;
		ae_info.cur_lum = ae_result->cur_lum;
		ae_info.target_lum = 128;
		ae_info.is_stable = ae_result->is_stab;
		calc_param.data_type = AF_DATA_AE;
		calc_param.data = (void*)(&ae_info);
		ISP_LOGE("AF_TAG:cur_exp %d cur_lum %d is_stab %d,fps %d",ae_result->cur_exp_line,ae_result->cur_lum,ae_result->is_stab,ae_info.cur_fps);
		rtn = af_calculation(handle->handle_af,&calc_param,result);
		break;
	}
	case AF_DATA_FD:{
		break;
	}
	default:{
		break;
	}
	}

	return rtn;
}

static int32_t _smart_init(isp_ctrl_context *handle)
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_smart_handle_t              smart_handle = NULL;
	struct smart_init_param         smart_init_param;
	struct isp_pm_ioctl_input       pm_input = {0};
	struct isp_pm_ioctl_output      pm_output = {0};
	uint32_t                        i = 0;

	handle->isp_smart_eb = 0;

	memset(&smart_init_param, 0, sizeof(smart_init_param));

	rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_INIT_SMART, &pm_input, &pm_output);
	if (ISP_SUCCESS == rtn) {
		for (i = 0; i < pm_output.param_num; ++i){
			smart_init_param.tuning_param[i].data.size     = pm_output.param_data[i].data_size;
			smart_init_param.tuning_param[i].data.data_ptr = pm_output.param_data[i].data_ptr;
		}
	} else {
		ISP_LOGE("get smart init param failed");
		return rtn;
	}

	/*FOR TEST*/
	//tuning_param = get_smart_param();
	//memcpy(smart_init_param.tuning_param, tuning_param, sizeof(smart_init_param.tuning_param));

	smart_handle = smart_ctl_init(&smart_init_param, NULL);
	if (NULL == smart_handle) {
		ISP_LOGE("smart init failed");
		return rtn;
	}

	handle->handle_smart = smart_handle;
	ISP_LOGI("ISP_SMART: handle=%p", smart_handle);

	return rtn;
}

static void _smart_deinit(isp_ctrl_context *handle)
{
	uint32_t                        rtn = ISP_SUCCESS;
	isp_smart_handle_t              smart_handle = handle->handle_smart;

	rtn = smart_ctl_deinit(smart_handle, NULL, NULL);
	ISP_LOGV("ISP_SMART: handle=%p, rtn=%d", smart_handle, rtn);
	handle->handle_smart = PNULL;
}

//remove isp_smart_lsc_set_param, gerald merge, http://review.source.spreadtrum.com/gerrit/#/c/281621

//remove smart_lsc_init(isp_ctrl_context* handle), gerald merge, http://review.source.spreadtrum.com/gerrit/#/c/281621

//remove _smart_lsc_pre_calc, gerald merge, http://review.source.spreadtrum.com/gerrit/#/c/281621 
	
//remove  _smart_lsc_deinit, gerald merge, http://review.source.spreadtrum.com/gerrit/#/c/281621



static int32_t _set_afl_thr(struct isp_ctrl_context *handle, int *thr)
{
#ifdef WIN32
	return ISP_SUCCESS;
#else
	int rtn = ISP_SUCCESS;
	uint32_t temp = 0;
	char temp_thr[4] = {0};
	int i = 0, j = 0;

	if(NULL == handle){
		ISP_LOGE(" _is_isp_reg_log param error ");
		return -1;
	}
	char value[30];

	property_get(ISP_SET_AFL_THR, value, "/dev/close_afl");
	ISP_LOGI("_set_afl_thr:%s", value);

	if (strcmp(value, "/dev/close_afl")) {
		for(i = 0; i < 9; i++) {
			for(j = 0; j < 3; j++){
				temp_thr[j] = value[3*i + j];
			}
			temp_thr[j] = '/0';
			temp = atoi(temp_thr);
			*thr++ = temp;

			if(0 == temp) {
				rtn = -1;
				ISP_LOGE("ERR:Invalid anti_flicker threshold param!");
				break;
			}
		}
	} else {
		rtn = -1;
		return rtn;
	}
#endif

	return rtn;
}

/*************This code just use for anti-flicker debug*******************/
#if 1
static FILE *fp = NULL;
static int32_t cnt = 0;

cmr_int afl_statistc_file_open()
{
	char                            file_name[40] = {0};
	char                            tmp_str[10];

	strcpy(file_name, "/data/misc/media/");
	sprintf(tmp_str, "%d", cnt);
	strcat(file_name, tmp_str);

	strcat(file_name, "_afl");
	ISP_LOGE("file name %s", file_name);
	fp = fopen(file_name, "w+");
	if (NULL == fp) {
		ISP_LOGI("can not open file: %s \n", file_name);
		return -1;
	}

	return 0;
}

static int32_t afl_statistic_save_to_file(int32_t height, int32_t *addr, FILE *fp_ptr)
{
	int32_t                         i = 0;
	int32_t                         *ptr = addr;
	FILE                            *fp = fp_ptr;

	ISP_LOGI("addr %p line num %d", ptr, height);

	for (i = 0; i < height; i++) {
		fprintf(fp, "%d\n", *ptr);
		ptr++;
	}

	return 0;
}

void afl_statistic_file_close(FILE *fp)
{
	if (NULL != fp) {
		fclose(fp);
	}
}
#endif

static int32_t _ispAntiflicker_init(isp_handle isp_handler)
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_anti_flicker_cfg     *afl_param;
	void                            *afl_buf_ptr = NULL;

	rtn = antiflcker_sw_init();
	if (rtn) {
		ISP_LOGE("AFL_TAG:antiflcker_sw_init failed");
		return ISP_ERROR;
	}

	afl_param = (struct isp_anti_flicker_cfg*)malloc(sizeof(struct isp_anti_flicker_cfg));
	if (NULL == afl_param){
		ISP_LOGE("AFL_TAG:malloc failed");
		return ISP_ERROR;
	}

	afl_buf_ptr = (void*)malloc(ISP_AFL_BUFFER_LEN); //(handle->src.h * 4 * 16);//max frame_num 15
	if (NULL == afl_buf_ptr) {
		ISP_LOGE("AFL_TAG:malloc failed");
		free(afl_param);
		return ISP_ERROR;
	}

	afl_param->bypass = 0;
	afl_param->skip_frame_num = 1;
	afl_param->mode = 0;
	afl_param->line_step = 0;
	afl_param->frame_num = 3;//1~15
	afl_param->vheight = handle->src.h;
	afl_param->start_col = 0;
	afl_param->end_col = handle->src.w - 1;
	afl_param->addr = afl_buf_ptr;

	handle->handle_antiflicker = (void *)afl_param;

	ISP_LOGI("initial ok!");

	//afl_statistc_file_open();

	return rtn;
}

static int32_t _ispAntiflicker_cfg(isp_handle isp_handler)
{
	int32_t                             rtn = ISP_SUCCESS;
	isp_ctrl_context                    *handle = (isp_ctrl_context*)isp_handler;
	struct isp_anti_flicker_cfg         *cfg;
	struct isp_pm_param_data            param_data;
	struct isp_pm_ioctl_input           input = {NULL, 0};
	struct isp_pm_ioctl_output          output = {NULL, 0};
	struct isp_dev_anti_flicker_info_v1 afl_info;

	cfg = (struct isp_anti_flicker_cfg*)handle->handle_antiflicker;

	ISP_LOGI("wxh %dx%d start_col %d end_col %d", handle->src.w, cfg->vheight, cfg->start_col, cfg->end_col);

	memset(&param_data, 0x0, sizeof(param_data));
	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_YIQ_AFL_CFG, ISP_BLK_YIQ_AFL, cfg, sizeof(struct isp_anti_flicker_cfg));
	rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_OTHERS, &input, &output);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("Anti flicker init failed");
		return rtn;
	}

	afl_info.bypass         = cfg->bypass;
	afl_info.skip_frame_num = cfg->skip_frame_num;
	afl_info.mode           = cfg->mode;
	afl_info.line_step      = cfg->line_step;
	afl_info.frame_num      = cfg->frame_num;
	afl_info.start_col      = cfg->start_col;
	afl_info.end_col        = cfg->end_col;
	afl_info.vheight        = cfg->vheight;
	//afl_info.addr           = cfg->addr;

	isp_u_anti_flicker_block(handle->handle_device, &afl_info);

	return rtn;
}

static int32_t _ispAntiflicker_calc(isp_handle isp_handler)
{
	int32_t                         rtn = ISP_SUCCESS;
	int32_t                         flag = 0;
	uint32_t                        bypass = 0;
	uint32_t                         i = 0;
	int32_t                         *addr = NULL;
	//int32_t                         *addr1 = NULL;
	uint32_t                        cur_flicker = 0;
	uint32_t                        nxt_flicker = 0;
	uint32_t                        cur_exp_flag = 0;
	int32_t                         ae_exp_flag = 0;
	float                           ae_exp = 0.0;
	int32_t                         penelity_LUT_50hz[12] = {40,50,60,70,90,100,110,120,130,140,150,160};//50Hz
	int32_t                         penelity_LUT_60hz[12] = {40,50,60,70,90,100,110,120,130,140,150,160};//60Hz
	int32_t                         thr[9] = {0,0,0,0,0,0,0,0,0};
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_anti_flicker_cfg     *cfg;
	struct isp_ae_grgb_statistic_info   *ae_stat_ptr = NULL;
	struct isp_pm_param_data        param_data;
	struct isp_pm_ioctl_input       input = {NULL, 0};
	struct isp_pm_ioctl_output      output = {NULL, 0};

	cfg = (struct isp_anti_flicker_cfg*)handle->handle_antiflicker;
	if (NULL == cfg) {
		ISP_LOGE("AFL_TAG:invalid pointer");
		return ISP_PARAM_NULL;
	}
	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_AEM_STATISTIC, ISP_BLK_AE_V1, NULL, 0);
	isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, (void*)&input, (void*)&output);
	if (NULL == output.param_data || NULL == output.param_data->data_ptr) {
		ISP_LOGE("AFL_TAG:invalid param_data pointer");
		return ISP_PARAM_NULL;
	}
	ae_stat_ptr = output.param_data->data_ptr;

	bypass = 1;
	isp_u_anti_flicker_bypass(handle->handle_device, (void*)&bypass);

	//copy statistics from kernel
	//isp_u_anti_flicker_statistic(handle->handle_device, cfg->addr);
	//addr = cfg->addr;
	addr = (int32_t *)handle->isp_anti_flicker_virtaddr;

	//addr1 = addr;
	//afl_statistic_save_to_file(handle->src.h * cfg->frame_num, addr1, fp);

	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_FLICKER_MODE, NULL, &cur_flicker);
	ISP_LOGI("cur flicker mode %d", cur_flicker);

	//exposure 1/33 s  -- 302921 (+/-10)
	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_EXP, NULL, &ae_exp);
	if (fabs(ae_exp - 0.06) < 0.000001 || ae_exp > 0.06) {
		ae_exp_flag = 1;
	}

	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_FLICKER_SWITCH_FLAG, &cur_exp_flag, NULL);
	ISP_LOGI("cur exposure flag %d", cur_exp_flag);

	if(cur_exp_flag) {
		if(cur_flicker) {
			rtn = _set_afl_thr(isp_handler, thr);
			if(0 == rtn) {
				ISP_LOGI("%d %d %d %d %d %d %d %d %d", thr[0], thr[1], thr[2], thr[3], thr[4], thr[5], thr[6], thr[7], thr[8]);
				ISP_LOGI("60Hz setting working");
			} else {
				thr[0] = 200;
				thr[1] = 20;
				thr[2] = 160;
				thr[3] = (ae_exp_flag==1) ? 100 : 160;
				thr[4] = 100;
				thr[5] = 4;
				thr[6] = 30;//ver2.1; thr[6] = 50;//ver2.0
				thr[7] = 20;
				thr[8] = 120;
				ISP_LOGI("60Hz using default threshold");
			}
		} else {
			rtn = _set_afl_thr(isp_handler, thr);
			if(0 == rtn) {
				ISP_LOGI("%d %d %d %d %d %d %d %d %d", thr[0], thr[1], thr[2], thr[3], thr[4], thr[5], thr[6], thr[7], thr[8]);
				ISP_LOGI("50Hz setting working");
			} else {
				thr[0] = 200;
				thr[1] = 20;
				thr[2] = 160;
				thr[3] = 200;
				thr[3] = (ae_exp_flag==1) ? 100 : 280;
				thr[4] = 100;
				thr[5] = 4;
				thr[6] = 30;//ver2.1; thr[6] = 50;//ver2.0
				thr[7] = 20;
				thr[8] = 120;
				ISP_LOGI("50Hz using default threshold");
			}
		}

		for(i = 0;i < cfg->frame_num;i++) {
			if(cur_flicker) {
				flag = antiflcker_sw_process(handle->src.w, handle->src.h, addr, 0, thr[0], thr[1], thr[2],\
					thr[3], thr[4], thr[5], thr[6], thr[7], thr[8], (int *)ae_stat_ptr->r_info, (int *)ae_stat_ptr->g_info, (int *)ae_stat_ptr->b_info);
				ISP_LOGI("flag %d %s", flag, "60Hz");
			} else {
				flag = antiflcker_sw_process(handle->src.w, handle->src.h, addr, 1, thr[0], thr[1], thr[2],\
					thr[3], thr[4], thr[5], thr[6], thr[7], thr[8], (int *)ae_stat_ptr->r_info, (int *)ae_stat_ptr->g_info, (int *)ae_stat_ptr->b_info);
				ISP_LOGI("flag %d %s", flag, "50Hz");
			}

			if(flag) {
				break;
			}
			addr += handle->src.h;
		}
		//change ae table
		if(flag) {
			if(cur_flicker) {
				nxt_flicker = AE_FLICKER_50HZ;
			} else {
				nxt_flicker = AE_FLICKER_60HZ;
			}
			//_ispFlickerIOCtrl(isp_handler, &nxt_flicker, NULL);
			handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_FLICKER, &nxt_flicker, NULL);
		}
	}

	bypass = 0;
	isp_u_anti_flicker_bypass(handle->handle_device, (void *)&bypass);

	return rtn;
}

static int32_t _ispAntiflicker_deinit(isp_ctrl_context *handle)
{
	int32_t                         rtn = ISP_SUCCESS;
	struct isp_anti_flicker_cfg     *afl_param = NULL;
	struct isp_system               *isp_system_ptr = &handle->system;

	CMR_MSG_INIT(msg);

	afl_param = (struct isp_anti_flicker_cfg*)handle->handle_antiflicker;

	msg.msg_type  = ISP_PROC_AFL_STOP;
	msg.sync_flag = CMR_MSG_SYNC_PROCESSED;
	rtn = cmr_thread_msg_send(isp_system_ptr->thread_afl_proc, &msg);
	if (rtn) {
		ISP_LOGE("Send msg failed");
	}

	if (NULL != afl_param) {
		if (NULL != afl_param->addr) {
			free(afl_param->addr);
			afl_param->addr = NULL;
		}

		free(handle->handle_antiflicker);
		handle->handle_antiflicker = NULL;
	}

	rtn = antiflcker_sw_deinit();
	if (rtn) {
		ISP_LOGE("AFL_TAG:antiflcker_sw_deinit error");
		return ISP_ERROR;
	}

	//afl_statistic_file_close(fp);

	return rtn;
}

static int32_t _ispSoft_ae_init(isp_handle isp_handler)
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_soft_ae_cfg          *aem_bining_param = NULL;
	struct ae_set_tuoch_zone        trim;

	if (0 == handle->need_soft_ae) {
		return rtn;
	}

	aem_bining_param = (struct isp_soft_ae_cfg*)malloc(sizeof(struct isp_soft_ae_cfg));
	if (NULL == aem_bining_param) {
		ISP_LOGE("SOFT_AE_TAG:malloc failed");
		return ISP_ERROR;
	}

	aem_bining_param->addr = (void *)malloc(ISP_RAWAEM_BUF_SIZE*ISP_BQ_BIN_CNT);
	if (NULL == aem_bining_param->addr) {
		ISP_LOGE("SOFT_AE_TAG:malloc failed");
		free(aem_bining_param);
		return ISP_ERROR;
	}
	aem_bining_param->win_num.w     = 32;
	aem_bining_param->win_num.h     = 32;
	aem_bining_param->h_ratio       = ISP_AEM_BIN_SCAL_RATIO;//8x
	aem_bining_param->v_ratio       = ISP_AEM_BIN_SCAL_RATIO;//8x
	aem_bining_param->img_size.w    = (((handle->src.w/(1 << ISP_AEM_BIN_SCAL_RATIO)) >> 1) << 1);
	aem_bining_param->img_size.h    = (((handle->src.h/(1 << ISP_AEM_BIN_SCAL_RATIO)) >> 1) << 1);

	aem_bining_param->blk_size.w    = ((aem_bining_param->img_size.w / aem_bining_param->win_num.w) / 2) * 2;
	aem_bining_param->blk_size.h    = ((aem_bining_param->img_size.h / aem_bining_param->win_num.h) / 2) * 2;

	aem_bining_param->trim_size.w   = aem_bining_param->blk_size.w * aem_bining_param->win_num.w;
	aem_bining_param->trim_size.h   = aem_bining_param->blk_size.h * aem_bining_param->win_num.h;

	aem_bining_param->win_start.x   = (((aem_bining_param->img_size.w - aem_bining_param->trim_size.w) / 2) / 2) * 2;
	aem_bining_param->win_start.y   = (((aem_bining_param->img_size.h - aem_bining_param->trim_size.h) / 2) / 2) * 2;

	aem_bining_param->bayer_mode    = handle->image_pattern;
	aem_bining_param->skip_num      = 0;
	aem_bining_param->bypass        = 0;
	aem_bining_param->bypass_status = 0;
	aem_bining_param->endian        = 1;//endian 1--> little, endian 0--> big

	handle->handle_soft_ae          = (void*)aem_bining_param;

	/*ae io_ctrl trim aera*/
	trim.touch_zone.w = aem_bining_param->blk_size.w * 32 * (1 << aem_bining_param->h_ratio);
	trim.touch_zone.h = aem_bining_param->blk_size.h * 32 * (1 << aem_bining_param->v_ratio);
	trim.touch_zone.x = 0;
	trim.touch_zone.y = 0;

	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_STAT_TRIM, (void*)&trim.touch_zone, NULL);
	ISP_LOGI("AE trim wxh %dx%d", trim.touch_zone.w, trim.touch_zone.h);


	ISP_LOGI("img_width %dx%d trim_width %dx%d blk_size %dx%d pos(%d,%d)",aem_bining_param->img_size.w, aem_bining_param->img_size.h,
			aem_bining_param->trim_size.w, aem_bining_param->trim_size.h, aem_bining_param->blk_size.w, aem_bining_param->blk_size.h,
			aem_bining_param->win_start.x, aem_bining_param->win_start.y);

	return rtn;
}

static int32_t _ispSoft_ae_cfg(isp_handle isp_handler)
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_soft_ae_cfg          *ae_cfg = (struct isp_soft_ae_cfg*)handle->handle_soft_ae;
	struct ae_set_tuoch_zone        trim;
	int32_t                         bypass = 0;

	if (0 == handle->need_soft_ae) {
		return rtn;
	}

	ISP_LOGI("need_soft_ae %d ratio h v %d %d", handle->need_soft_ae, ae_cfg->h_ratio, ae_cfg->v_ratio);
	/*aem module bypass*/
	bypass = 1;
	isp_u_raw_aem_bypass(handle->handle_device, &bypass);
	isp_u_raw_aem_slice_size(handle->handle_device, handle->src.w, handle->src.h);

	/*ae io_ctrl trim aera*/
	trim.touch_zone.w = ae_cfg->blk_size.w * 32 * (1 << ae_cfg->h_ratio);
	trim.touch_zone.h = ae_cfg->blk_size.h * 32 * (1 << ae_cfg->v_ratio);
	trim.touch_zone.x = 0;
	trim.touch_zone.y = 0;

	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_STAT_TRIM, (void*)&trim.touch_zone, NULL);
	ISP_LOGI("AE trim wxh %dx%d", trim.touch_zone.w, trim.touch_zone.h);

	/*configure binning module*/
	isp_u_binning4awb_scaling_ratio(handle->handle_device, (ae_cfg->v_ratio), (ae_cfg->h_ratio));
	isp_u_binning4awb_endian(handle->handle_device, ae_cfg->endian);
	isp_u_binning4awb_initbuf(handle->handle_device);

	bypass = 0;
	isp_u_binning4awb_bypass(handle->handle_device, bypass);
	ae_cfg->bypass_status = 0;

	return rtn;
}

static int32_t _ispSoft_ae_deinit(isp_ctrl_context* handle)
{
	int32_t                         rtn = ISP_SUCCESS;
	struct isp_soft_ae_cfg          *ae_cfg = NULL;

	if (0 == handle->need_soft_ae) {
		return rtn;
	}

	ae_cfg = (struct isp_soft_ae_cfg*)handle->handle_soft_ae;

	if (NULL != handle->handle_soft_ae) {
		if (NULL != ae_cfg->addr) {
			free(ae_cfg->addr);
			ae_cfg->addr = NULL;
		}

		free(handle->handle_soft_ae);
		handle->handle_soft_ae = NULL;
	}

	ISP_LOGI("_ispSoft_ae_deinit ok");
	return rtn;
}

static int32_t _ispSoft_bin_to_aem_statistics(isp_handle isp_handler, void *dest_addr, void *src_addr)
{
	int32_t rtn = ISP_SUCCESS;
	isp_ctrl_context *handle = (isp_ctrl_context *)isp_handler;
	struct isp_soft_ae_cfg *ae_cfg;

	uint16_t width = 0, height = 0, start_x = 0, start_y = 0;
	uint16_t blk_width = 0, blk_height = 0, bayer_mode = 0;
	uint32_t *ptr = NULL;
	uint32_t i = 0, scale_ratio = 0;

	if(NULL == handle) {
		ISP_LOGE("Empty handle pointer error");
		return -1;
	}

	ae_cfg = (struct isp_soft_ae_cfg *)handle->handle_soft_ae;
	if(NULL == ae_cfg) {
		ISP_LOGE("Empty ae_cfg pointer error");
		return -1;
	}
	width = (uint16_t)ae_cfg->img_size.w;
	height = (uint16_t)ae_cfg->img_size.h;
	start_x = (uint16_t)ae_cfg->win_start.x;
	start_y = (uint16_t)ae_cfg->win_start.y;
	blk_width = (uint16_t)ae_cfg->blk_size.w;
	blk_height = (uint16_t)ae_cfg->blk_size.h;
	bayer_mode = (uint16_t)ae_cfg->bayer_mode;

	scale_ratio = (1 << ae_cfg->h_ratio) * (1 << ae_cfg->v_ratio);

	ISP_LOGI("LHC:bayermode %d image_size %dx%d pos(%d,%d) blk_size %dx%d", bayer_mode, width, height, start_x, start_y, blk_width, blk_height);
	rtn = aem_binning(bayer_mode, width, height, (uint16_t *)src_addr, start_x, start_y, blk_width, blk_height, (uint32_t *)dest_addr);
	if(rtn) {
		ISP_LOGE("Parameter error");
		return -1;
	}

	ptr = (uint32_t *)dest_addr;

	/*binning statistics need scale*/
	for(i = 0; i < ISP_AEM_ITEM * 3; i++) {
		ptr[i] *= scale_ratio;
	}

	/*for(i = 500; i < 510; i++) {
		ISP_LOGE("%d (r,g,b) (%d,%d,%d)", i, ptr[i], ptr[1024+i], ptr[2048+i]);
	}*/
	return rtn;
}

static int32_t _smart_param_update(isp_ctrl_context *handle)
{
	int32_t                         rtn = ISP_SUCCESS;
	uint32_t                        i = 0;
	struct smart_init_param         smart_init_param;
	struct isp_pm_ioctl_input       pm_input = {0};
	struct isp_pm_ioctl_output      pm_output = {0};

	memset(&smart_init_param, 0, sizeof(smart_init_param));

	rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_INIT_SMART, &pm_input, &pm_output);
	if ((ISP_SUCCESS == rtn) && pm_output.param_data) {
		for (i = 0; i < pm_output.param_num; ++i){
			smart_init_param.tuning_param[i].data.size = pm_output.param_data[i].data_size;
			smart_init_param.tuning_param[i].data.data_ptr = pm_output.param_data[i].data_ptr;
		}
	} else {
		ISP_LOGE("get smart init param failed");
		return rtn;
	}

	rtn = smart_ctl_ioctl(handle->handle_smart, ISP_SMART_IOCTL_GET_UPDATE_PARAM, (void *)&smart_init_param, NULL);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("smart param reinit failed");
		return rtn;
	}

	ISP_LOGI("ISP_SMART: handle=%p, param=%p", handle->handle_smart,
			pm_output.param_data->data_ptr);

	return rtn;
}
static int32_t _isp_change_lsc_param(isp_handle isp_handler, uint32_t ct)
{
	int32_t                         rtn = ISP_SUCCESS;
#ifdef LSC_ADV_ENABLE

	//ALOGE("ISP2.0 ALSC: change lsc param");//gerald merge http://review.source.spreadtrum.com/gerrit/#/c/279914, http://review.source.spreadtrum.com/gerrit/#/c/281621 

	isp_ctrl_context *handle = (isp_ctrl_context *)isp_handler;
	lsc_adv_handle_t lsc_adv_handle = handle->handle_lsc_adv;
	struct isp_pm_ioctl_input input = {PNULL, 0};
	struct isp_pm_ioctl_output output= {PNULL, 0};
	struct isp_pm_param_data param_data = {0};
	struct lsc_adv_calc_param calc_param;
	struct lsc_adv_calc_result calc_result = {0};
	uint32_t i = 0;
	memset(&calc_param, 0, sizeof(calc_param));

	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_LSC_INFO, ISP_BLK_2D_LSC, PNULL, 0);
	rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, (void*)&input, (void*)&output);
	struct isp_lsc_info *lsc_info = (struct isp_lsc_info *)output.param_data->data_ptr;
	
	struct isp_2d_lsc_param* lsc_tab_pram_ptr = (struct isp_2d_lsc_param*)(handle->lsc_tab_address);//gerald merge http://review.source.spreadtrum.com/gerrit/#/c/279914
	//ALOGE("ALSC_PROC ISP2.0 ALSC: change lsc param, lsc_tab_pram_ptr=0x%x",lsc_tab_pram_ptr);
	//gerald merge http://review.source.spreadtrum.com/gerrit/#/c/279914, http://review.source.spreadtrum.com/gerrit/#/c/281621?        

	calc_result.dst_gain = (uint16_t *)lsc_info->data_ptr;
	struct awb_size stat_img_size;
	struct awb_size win_size;
	struct isp_ae_grgb_statistic_info *stat_info;
	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_AEM_STATISTIC, ISP_BLK_AE_V1, NULL, 0);
	isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, (void*)&input, (void*)&output);
	stat_info = output.param_data->data_ptr;

	handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_GET_STAT_SIZE, (void *)&stat_img_size, NULL);
	handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_GET_WIN_SIZE, (void *)&win_size, NULL);

	calc_param.stat_img.r  = stat_info->r_info;
	calc_param.stat_img.gr = stat_info->g_info;
	calc_param.stat_img.gb = stat_info->g_info;
	calc_param.stat_img.b  = stat_info->b_info;
	calc_param.stat_size.w = stat_img_size.w;
	calc_param.stat_size.h = stat_img_size.h;
	calc_param.gain_width = lsc_info->gain_w;
	calc_param.gain_height = lsc_info->gain_h;
	calc_param.lum_gain = (uint16_t *)lsc_info->param_ptr;
	calc_param.block_size.w = win_size.w;
	calc_param.block_size.h = win_size.h;
	calc_param.ct = ct;
	calc_param.bv = isp_cur_bv;
	calc_param.isp_id = ISP_2_0;
	
	//gerald merge start http://review.source.spreadtrum.com/gerrit/#/c/286361
	calc_param.r_gain = gAWBGainR;
	calc_param.b_gain = gAWBGainB;
	calc_param.img_size.w = handle->src.w;
 	calc_param.img_size.h = handle->src.h;
 	
 	//gerald merge end http://review.source.spreadtrum.com/gerrit/#/c/286361
	
	//gerald merge start http://review.source.spreadtrum.com/gerrit/#/c/279914
	if(lsc_tab_pram_ptr==0)
		return 0;
	else{
	 	for(i=0;i<9;i++){
			calc_param.lsc_tab_address[i] = lsc_tab_pram_ptr->map_tab[i].param_addr;
		}	
	}
	//gerald merge end http://review.source.spreadtrum.com/gerrit/#/c/279914
	
	rtn = handle->lsc_lib_fun->alsc_calc(lsc_adv_handle, &calc_param, &calc_result);//gerald merge, http://review.source.spreadtrum.com/gerrit/#/c/281621
	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_LSC_INFO, ISP_BLK_2D_LSC, PNULL, 0);
	input.param_data_ptr = &param_data;

	rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_OTHERS, &input, NULL);
#endif
	return rtn;
}

static uint32_t _ispAlgInit(isp_handle isp_handler)
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;

	ISP_LOGI("E");

#ifdef CONFIG_CAMERA_AFL_AUTO_DETECTION
	rtn = _ispAntiflicker_init(isp_handler);
	ISP_RETURN_IF_FAIL(rtn, ("anti_flicker param update failed"));
#endif

	rtn = _ae_init(handle);
	ISP_TRACE_IF_FAIL(rtn, ("_ae_init error"));

	rtn = _ispSoft_ae_init(handle);
	ISP_TRACE_IF_FAIL(rtn, ("_ispSoft_ae_init error"));

	rtn = _awb_init(handle);
	ISP_TRACE_IF_FAIL(rtn, ("_awb_init error"));

	rtn = _smart_init(handle);
	ISP_TRACE_IF_FAIL(rtn, ("_smart_init error"));

	rtn = _af_init(handle);
	ISP_TRACE_IF_FAIL(rtn, ("_af_init error"));
	
	rtn = _alsc_init(handle);//gerald merge, http://review.source.spreadtrum.com/gerrit/#/c/281621
	ISP_TRACE_IF_FAIL(rtn, ("_smart_lsc_init error"));

	return rtn;
}

static uint32_t _ispAlgDeInit(isp_handle isp_handler)
{
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;

#ifdef CONFIG_CAMERA_AFL_AUTO_DETECTION
	_ispAntiflicker_deinit(handle);
#endif
	_ae_deinit(handle);

	_smart_deinit(handle);

	_awb_deinit(handle);

	_af_deinit(handle);

	_alsc_deinit(handle);//gerald merge, http://review.source.spreadtrum.com/gerrit/#/c/281621

	_ispSoft_ae_deinit(handle);

	aaa_lib_deinit(handle);

	return ISP_SUCCESS;
}

static int32_t _ispSetInterfaceParam(isp_handle isp_handler)
{
	int32_t                         rtn = ISP_SUCCESS;

	rtn = isp_set_comm_param_v1(isp_handler);
	ISP_TRACE_IF_FAIL(rtn, ("isp_set_comm_param_v1 error"));

	rtn = isp_set_slice_size_v1(isp_handler);
	ISP_TRACE_IF_FAIL(rtn, ("isp_set_slice_size_v1 error"));

	rtn = isp_set_fetch_param_v1(isp_handler);
	ISP_TRACE_IF_FAIL(rtn, ("isp_set_fetch_param_v1 error"));

	rtn = isp_set_store_param_v1(isp_handler);
	ISP_TRACE_IF_FAIL(rtn, ("isp_set_store_param_v1 error"));

	rtn = isp_set_dispatch(isp_handler);
	ISP_TRACE_IF_FAIL(rtn, ("isp_set_dispatch error"));

	rtn = isp_set_arbiter(isp_handler);
	ISP_TRACE_IF_FAIL(rtn, ("isp_set_arbiter error"));

	return rtn;
}

static int32_t isp_u_ae_transaddr(isp_handle isp_handler)
{
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_soft_ae_cfg *ae_cfg_param = (struct isp_soft_ae_cfg *)handle->handle_soft_ae;
	isp_u32 addr_cnt;
	isp_u32 node_type;
	uint32_t i = 0;

	if (handle->need_soft_ae) {
		ae_cfg_param->addr_num = 0;
		addr_cnt = handle->isp_bq_mem_num -1;
		node_type = ISP_NODE_TYPE_BINNING4AWB;
		isp_u_bq_enqueue_buf(handle->handle_device,handle->isp_bq_k_addr_array[handle->isp_bq_mem_num-1]
						,handle->isp_bq_u_addr_array[handle->isp_bq_mem_num-1], ISP_NODE_TYPE_AE_RESERVED);
	} else {
		addr_cnt = handle->isp_bq_mem_num;
		node_type = ISP_NODE_TYPE_RAWAEM;
	}

	for (i=0;i<addr_cnt;i++) {
		isp_u_bq_enqueue_buf(handle->handle_device, handle->isp_bq_k_addr_array[i], handle->isp_bq_u_addr_array[i], node_type);
	}

	return 0;
}

/* [Vendor_21/40] */
static int32_t isp_u_af_transaddr(isp_handle isp_handler)  
{
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	isp_u32 addr_cnt;
	isp_u32 node_type;
	uint32_t i = 0;

	addr_cnt = handle->isp_af_bq_mem_num;
	node_type = ISP_NODE_TYPE_RAWAFM;

	for (i=0;i<addr_cnt;i++) {
		isp_u_bq_enqueue_buf(handle->handle_device, handle->isp_af_bq_k_addr_array[i], handle->isp_af_bq_u_addr_array[i], node_type);
		ISP_LOGI("[3A Porting]isp_u_af_transaddr: For i=%d, k_addr = 0x%x, u_addr = 0x%x", i, handle->isp_af_bq_k_addr_array[i], handle->isp_af_bq_u_addr_array[i]);
	}

	return 0;
}

static int32_t _ispTransBuffAddr(isp_handle isp_handler)
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_file                 *file = NULL;

	file = (struct isp_file*)(handle->handle_device);
	file->reserved = handle->isp_lsc_virtaddr;

	isp_u_bq_init_bufqueue(handle->handle_device);

	/*isp lsc addr transfer*/
	isp_u_2d_lsc_transaddr(handle->handle_device, handle->isp_lsc_physaddr);

	isp_u_anti_flicker_transaddr(handle->handle_device, handle->isp_anti_flicker_physaddr);

	isp_u_ae_transaddr(isp_handler);
/* [Vendor_22/40] */
	isp_u_af_transaddr(isp_handler);  
	ISP_LOGI("[3A Porting]int32_t _ispTransBuffAddr");

	return rtn;
}

static int32_t _ispCfg(isp_handle isp_handler)
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_pm_ioctl_input       input;
	struct isp_pm_ioctl_output      output;
	struct isp_pm_param_data        *param_data;
	uint32_t                        i = 0;

	handle->gamma_sof_cnt = 0;
	handle->gamma_sof_cnt_eb = 0;
	handle->update_gamma_eb = 0;

	rtn = isp_dev_reset(handle->handle_device);
	ISP_TRACE_IF_FAIL(rtn, ("isp_dev_reset error"));

	isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_ISP_ALL_SETTING, &input, &output);
	param_data = output.param_data;
	for (i = 0; i < output.param_num; i++) {
		if (ISP_BLK_2D_LSC != param_data->id) {
			isp_cfg_block(handle->handle_device, param_data->data_ptr, param_data->id);
		} else {
			isp_update_2d_lsc_slice_info(handle, param_data->data_ptr);//gerald 20161031
		}
		param_data ++;
	}

#ifdef AE_MONITOR_CHANGE
	isp_cfg_3a_single_frame_shadow_v1(handle->handle_device, 1);
#endif

	/*************pike*****************/
	isp_u32 chip_id = isp_dev_get_chip_id(handle->handle_device);
	if (ISP_CHIP_ID_PIKE == chip_id) {
		struct isp_dev_rgb2y_info rgb2y_info;
		struct isp_dev_uv_prefilter_info uv_info;
		struct isp_dev_yuv_nlm_info ynlm_info;
		struct isp_dev_hdr_info hdr_info;

		rgb2y_info.signal_sel         = 1;
		uv_info.bypass                = 1;

		ynlm_info.nlm_bypass          = 1;
		ynlm_info.nlm_adaptive_bypass = 1;
		ynlm_info.nlm_radial_bypass   = 1;
		ynlm_info.nlm_vst_bypass      = 1;

		hdr_info.bypass               = 1;

		isp_u_rgb2y_block(handle->handle_device, &rgb2y_info);
		isp_u_uv_prefilter_block(handle->handle_device, &uv_info);
		isp_u_yuv_nlm_block(handle->handle_device, &ynlm_info);
		isp_u_hdr_block(handle->handle_device, &hdr_info);
	}
#if 0
	uint32_t ygamma_bypass = 1, yiq_aem_bypass = 0, skip_num = 3;
	isp_u_yiq_aem_ygamma_bypass(handle->handle_device, &ygamma_bypass);
	isp_u_yiq_aem_bypass(handle->handle_device, &yiq_aem_bypass);
	isp_u_yiq_aem_skip_num(handle->handle_device, skip_num);
	isp_u_yiq_aem_offset(handle->handle_device, 11, 22);
	isp_u_yiq_aem_blk_size(handle->handle_device, 22, 33);
#endif

	_ispSoft_ae_cfg(isp_handler);

/*************anti_flicker*************************/
#ifdef CONFIG_CAMERA_AFL_AUTO_DETECTION
	ISP_LOGI("antiflicker cfg");
	rtn = _ispAntiflicker_cfg(isp_handler);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("anti_flicker param update failed");
		return rtn;
	}
#endif

	return rtn;
}

static int32_t _ispUncfg(isp_handle  handle)
{
	int32_t                         rtn = ISP_SUCCESS;

	rtn = isp_dev_reset(handle);
	ISP_RETURN_IF_FAIL(rtn, ("isp_dev_reset error"));

	return rtn;
}

static int32_t _ispStart(isp_handle isp_handler)
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_interface_param_v1   *isp_context_ptr = &handle->interface_param_v1;
	struct isp_dev_fetch_info_v1    fetch_param;
	struct isp_dev_store_info_v1    store_param;

	rtn = isp_get_fetch_addr_v1(isp_context_ptr, &fetch_param);
	ISP_RETURN_IF_FAIL(rtn, ("isp get fetch addr error"));

	rtn = isp_get_store_addr_v1(isp_context_ptr, &store_param);
	ISP_RETURN_IF_FAIL(rtn, ("isp get store addr error"));

	isp_u_fetch_block(handle->handle_device, (void*)&fetch_param);
	ISP_RETURN_IF_FAIL(rtn, ("isp cfg fetch error"));

	rtn = isp_u_store_block(handle->handle_device, (void *)&store_param);
	ISP_RETURN_IF_FAIL(rtn, ("isp cfg store error"));

	rtn = isp_u_dispatch_block(handle->handle_device,(void*)&isp_context_ptr->dispatch);
	ISP_RETURN_IF_FAIL(rtn, ("isp cfg dispatch error"));

	isp_u_arbiter_block(handle->handle_device,(void*)&isp_context_ptr->arbiter);
	ISP_RETURN_IF_FAIL(rtn, ("isp cfg arbiter error"));

	isp_cfg_comm_data_v1(handle->handle_device,(void*)&isp_context_ptr->com);

	isp_cfg_slice_size_v1(handle->handle_device, (void*)&isp_context_ptr->slice);

	isp_u_fetch_start_isp(handle->handle_device, ISP_ONE);

	handle->gamma_sof_cnt_eb = 1;

	ISP_LOGI("done");

	return rtn;
}

static int32_t _ispProcessEndHandle(isp_handle isp_handler)
{
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_system               *isp_system_ptr = &handle->system;
	struct ips_out_param            callback_param = {0x00};
	struct isp_interface_param_v1   *interface_ptr_v1 = &handle->interface_param_v1;

	if (NULL != isp_system_ptr->callback) {
		callback_param.output_width = interface_ptr_v1->data.slice_width;//gerald 20161031
		ISP_LOGI("callback ISP_PROC_CALLBACK");
		isp_system_ptr->callback(isp_system_ptr->caller_id, ISP_CALLBACK_EVT|ISP_PROC_CALLBACK, (void*)&callback_param, sizeof(struct ips_out_param));
	}

	return ISP_SUCCESS;
}

static int32_t _ispSetTuneParam(isp_handle isp_handler)
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_pm_ioctl_input       input;
	struct isp_pm_ioctl_output      output;
	struct isp_pm_param_data        *param_data;
	uint32_t                        i;

	//ISP_LOGE("AE_TEST:-------Sof------------");
#ifndef CONFIG_CAMERA_ISP_AE_VERSION_V1
	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_PROC, NULL, NULL);
	ISP_TRACE_IF_FAIL(rtn, ("ae set proc error"));
#endif
	isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_ISP_SETTING, &input, &output);
	param_data = output.param_data;
	for (i = 0; i < output.param_num; i++) {
		if (ISP_BLK_AE_V1 == param_data->id) {
			if (ISP_PM_BLK_ISP_SETTING == param_data->cmd) {
				isp_cfg_block(handle->handle_device, param_data->data_ptr, param_data->id);
			}
		} else if (ISP_BLK_2D_LSC != param_data->id) {
			isp_cfg_block(handle->handle_device, param_data->data_ptr, param_data->id);
    		}

		if (ISP_BLK_2D_LSC == param_data->id) {
			isp_update_2d_lsc_slice_info(handle, param_data->data_ptr);//gerald 20161031
		}

		if (ISP_BLK_RGB_GAMC == param_data->id) {
			handle->gamma_sof_cnt = 0;
			handle->update_gamma_eb = 0;
		}

		param_data++;
	}
#ifndef CONFIG_CAMERA_ISP_AE_VERSION_V1
	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_TUNING_EB, NULL, NULL);
	ISP_TRACE_IF_FAIL(rtn, ("ae set tuning eb error"));
#endif
	rtn = isp_u_comm_shadow(handle->handle_device, ISP_ONE);

	return rtn;
}


/**---------------------------------------------------------------------------*
**					IOCtrl Function Prototypes			*
**---------------------------------------------------------------------------*/
static int32_t _ispAwbModeIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_pm_ioctl_input       ioctl_input = {0};
	struct isp_pm_param_data        ioctl_data = {0};
	struct isp_awbc_cfg             awbc_cfg = {0};
	struct awb_gain                 result = {0};
	uint32_t                        awb_mode = *(uint32_t*)param_ptr;
	uint32_t                        awb_id = 0;

	switch (awb_mode) {
	case ISP_AWB_AUTO:
		awb_id = AWB_CTRL_WB_MODE_AUTO;
		break;

	case ISP_AWB_INDEX1:
		awb_id = AWB_CTRL_MWB_MODE_INCANDESCENT;
		break;

	case ISP_AWB_INDEX4:
		awb_id = AWB_CTRL_MWB_MODE_FLUORESCENT;
		break;

	case ISP_AWB_INDEX5:
		awb_id = AWB_CTRL_MWB_MODE_SUNNY;
		break;

	case ISP_AWB_INDEX6:
		awb_id = AWB_CTRL_MWB_MODE_CLOUDY;
		break;

	default:
		break;
	}

	ISP_LOGI("--IOCtrl--AWB_MODE--:0x%x", awb_id);
	rtn = handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_SET_WB_MODE, (void*)&awb_id, NULL);
	ISP_TRACE_IF_FAIL(rtn, ("awb set wb mode error"));

	rtn = handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_GET_GAIN, (void*)&result, NULL);
	ISP_TRACE_IF_FAIL(rtn, ("awb get gain error"));

	/*set awb gain*/
	awbc_cfg.r_gain            = result.r;
	awbc_cfg.g_gain            = result.g;
	awbc_cfg.b_gain            = result.b;
	awbc_cfg.r_offset          = 0;
	awbc_cfg.g_offset          = 0;
	awbc_cfg.b_offset          = 0;

	ioctl_data.id              = ISP_BLK_AWB_V1;
	ioctl_data.cmd             = ISP_PM_BLK_AWBC;
	ioctl_data.data_ptr        = &awbc_cfg;
	ioctl_data.data_size       = sizeof(awbc_cfg);

	ioctl_input.param_data_ptr = &ioctl_data;
	ioctl_input.param_num      = 1;

	rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_AWB, (void *)&ioctl_input, NULL);
	ISP_LOGI("AWB_TAG: isp_pm_ioctl rtn=%d, gain=(%d, %d, %d)", rtn, awbc_cfg.r_gain, awbc_cfg.g_gain, awbc_cfg.b_gain);

	return rtn;
}

static int32_t _ispAeAwbBypassIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	uint32_t                        type = 0;
	uint32_t                        bypass = 0;

	if (NULL == param_ptr) {
		return ISP_PARAM_NULL;
	}

	type = *(uint32_t*)param_ptr;
	switch (type) {
	case 0: /*ae awb normal*/
		bypass = 0;
		isp_u_raw_aem_bypass(handle->handle_device, &bypass);
		handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_BYPASS, &bypass, NULL);
		break;
	case 1:
		break;
	case 2: /*ae by pass*/
		bypass = 1;
		handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_BYPASS, &bypass, NULL);
		break;
	case 3: /*awb by pass*/
		bypass = 1;
		isp_u_raw_aem_bypass(handle->handle_device, &bypass);
		break;
	default:
		break;
	}

	ISP_LOGI("type=%d", type);

	return rtn;
}

static int32_t _ispEVIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct ae_set_ev                set_ev = {0};

	if (NULL == param_ptr) {
		return ISP_PARAM_NULL;
	}

	set_ev.level = *(uint32_t*)param_ptr;
	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_EV_OFFSET, &set_ev, NULL);

	ISP_LOGI("ISP_AE: AE_SET_EV_OFFSET=%d, rtn=%d", set_ev.level, rtn);

	return rtn;
}

static int32_t _ispFlickerIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct ae_set_flicker           set_flicker = {0};
	int32_t				bypass = 0;

	if (NULL == param_ptr) {
		return ISP_PARAM_NULL;
	}

	handle->anti_flicker_mode = *(uint32_t*)param_ptr;
	ISP_LOGI("handle->anti_flicker_mode = %d",handle->anti_flicker_mode);
	set_flicker.mode = *(uint32_t*)param_ptr;
	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_FLICKER, &set_flicker, NULL);
	ISP_LOGI("ISP_AE: AE_SET_FLICKER=%d, rtn=%d", set_flicker.mode, rtn);
	isp_u_anti_flicker_bypass(handle->handle_device, (void*)&bypass);

	return rtn;
}

static int32_t _ispSnapshotNoticeIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_snapshot_notice      *isp_notice = param_ptr;
	struct ae_snapshot_notice       ae_notice;

	if (NULL == handle || NULL == isp_notice) {
		ISP_LOGE("handle %p isp_notice %p is NULL error", handle, isp_notice);
		return ISP_PARAM_NULL;
	}

	ae_notice.type = isp_notice->type;
	ae_notice.preview_line_time = isp_notice->preview_line_time;
	ae_notice.capture_line_time = isp_notice->capture_line_time;
	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_SNAPSHOT_NOTICE, &ae_notice, NULL);

	return rtn;
}

static int32_t _ispFlashNoticeIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_flash_notice         *flash_notice = (struct isp_flash_notice*)param_ptr;
	struct ae_flash_notice          ae_notice;
	enum smart_ctrl_flash_mode      flash_mode = 0;
	enum awb_ctrl_flash_status      awb_flash_status = 0;

	if (NULL == handle || NULL == flash_notice) {
		ISP_LOGE("handle %p,notice %p is NULL error", handle, flash_notice);
		return ISP_PARAM_NULL;
	}

	ISP_LOGV("mode=%d", flash_notice->mode);
	switch (flash_notice->mode) {
	case ISP_FLASH_PRE_BEFORE:
		ae_notice.mode = AE_FLASH_PRE_BEFORE;
		if(NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_set_flash_notice) {
			rtn = handle->af_lib_fun->af_ioctrl_set_flash_notice(isp_handler,param_ptr,NULL);
		}

		ae_notice.power.max_charge = flash_notice->power.max_charge;
		ae_notice.power.max_time = flash_notice->power.max_time;
		ae_notice.capture_skip_num = flash_notice->capture_skip_num;

		rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_FLASH_NOTICE, &ae_notice, NULL);
		rtn = handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_FLASH_BEFORE_P, NULL, NULL);
		awb_flash_status = AWB_FLASH_PRE_BEFORE;
		rtn = handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_SET_FLASH_STATUS, (void *)&awb_flash_status, NULL);
		flash_mode = SMART_CTRL_FLASH_PRE;
		rtn = smart_ctl_ioctl(handle->handle_smart, ISP_SMART_IOCTL_SET_FLASH_MODE, (void *)&flash_mode, NULL);

		ISP_LOGI("ISP_AE: rtn=%d", rtn);
		break;

	case ISP_FLASH_PRE_LIGHTING:
	{
		uint32_t ratio = 0;// = flash_notice->flash_ratio;
		struct isp_flash_param *flash_cali = NULL;

		rtn = _isp_get_flash_cali_param(handle->handle_pm, &flash_cali);
		if (ISP_SUCCESS == rtn) {
			ISP_LOGI("flash param rgb ratio = (%d,%d,%d), lum_ratio = %d",
					flash_cali->cur.r_ratio, flash_cali->cur.g_ratio,
					flash_cali->cur.b_ratio, flash_cali->cur.lum_ratio);
			if (0 != flash_cali->cur.lum_ratio) {
				ratio = flash_cali->cur.lum_ratio;
			}
		}
		ae_notice.mode = AE_FLASH_PRE_LIGHTING;
		ae_notice.flash_ratio = ratio;
		rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_FLASH_NOTICE, &ae_notice, NULL);
		rtn = handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_FLASH_OPEN_P, NULL, NULL);
		//rtn = _isp_set_awb_flash_gain(handle);
		awb_flash_status = AWB_FLASH_PRE_LIGHTING;
		rtn = handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_SET_FLASH_STATUS, (void *)&awb_flash_status, NULL);
		flash_mode = SMART_CTRL_FLASH_PRE;
		rtn = smart_ctl_ioctl(handle->handle_smart, ISP_SMART_IOCTL_SET_FLASH_MODE, (void *)&flash_mode, NULL);

		if(NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_set_flash_notice) {
			rtn = handle->af_lib_fun->af_ioctrl_set_flash_notice(isp_handler,param_ptr,NULL);
		}

		ISP_LOGI("ISP_AE: Flashing ratio=%d, rtn=%d", ratio, rtn);
		break;
	}

	case ISP_FLASH_PRE_AFTER:
		ae_notice.mode = AE_FLASH_PRE_AFTER;
		ae_notice.will_capture = flash_notice->will_capture;
		rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_FLASH_NOTICE, &ae_notice, NULL);
		rtn = handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_FLASH_CLOSE, NULL, NULL);
		awb_flash_status = AWB_FLASH_PRE_AFTER;
		rtn = handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_SET_FLASH_STATUS, (void *)&awb_flash_status, NULL);
		flash_mode = SMART_CTRL_FLASH_CLOSE;
		rtn = smart_ctl_ioctl(handle->handle_smart, ISP_SMART_IOCTL_SET_FLASH_MODE, (void *)&flash_mode, NULL);

		if(NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_set_flash_notice) {
			rtn = handle->af_lib_fun->af_ioctrl_set_flash_notice(isp_handler,param_ptr,NULL);
		}

		break;

	case ISP_FLASH_MAIN_AFTER:
		rtn  = handle->lsc_lib_fun->alsc_io_ctrl(handle->handle_lsc_adv, ALSC_FLASH_OFF, NULL, NULL);
		ae_notice.mode = AE_FLASH_MAIN_AFTER;
		rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_FLASH_NOTICE, &ae_notice, NULL);
		rtn = handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_FLASH_CLOSE, NULL, NULL);
		awb_flash_status = AWB_FLASH_MAIN_AFTER;
		rtn = handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_SET_FLASH_STATUS, (void *)&awb_flash_status, NULL);
		flash_mode = SMART_CTRL_FLASH_CLOSE;
		rtn = smart_ctl_ioctl(handle->handle_smart, ISP_SMART_IOCTL_SET_FLASH_MODE, (void *)&flash_mode, NULL);

		if(NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_set_flash_notice) {
			rtn = handle->af_lib_fun->af_ioctrl_set_flash_notice(isp_handler,param_ptr,NULL);
		}

		break;

	case ISP_FLASH_MAIN_BEFORE:
		rtn  = handle->lsc_lib_fun->alsc_io_ctrl(handle->handle_lsc_adv, ALSC_FLASH_ON, NULL, NULL);
		ae_notice.mode = AE_FLASH_MAIN_BEFORE;
		if(NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_set_flash_notice) {
			rtn = handle->af_lib_fun->af_ioctrl_set_flash_notice(isp_handler,param_ptr,NULL);
		}
		rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_FLASH_NOTICE, &ae_notice, NULL);
		rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_EXP_GAIN, NULL, NULL);
		awb_flash_status = AWB_FLASH_MAIN_BEFORE;
		rtn = handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_SET_FLASH_STATUS, (void *)&awb_flash_status, NULL);

		break;

	case ISP_FLASH_MAIN_LIGHTING:
		//ISP_LOGI("ISP_AE: main-Flashing1");
		//if (handle->ae_lib_fun->product_id) {
			ae_notice.mode = AE_FLASH_MAIN_LIGHTING;
			rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_FLASH_NOTICE, &ae_notice, NULL);
		//}

		rtn = handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_FLASH_OPEN_M, NULL, NULL);
		rtn = _isp_set_awb_flash_gain(handle);
		awb_flash_status = AWB_FLASH_MAIN_LIGHTING;
		rtn = handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_SET_FLASH_STATUS, (void *)&awb_flash_status, NULL);
		flash_mode = SMART_CTRL_FLASH_MAIN;
		rtn = smart_ctl_ioctl(handle->handle_smart, ISP_SMART_IOCTL_SET_FLASH_MODE, (void *)&flash_mode, NULL);

		if(NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_set_flash_notice) {
			rtn = handle->af_lib_fun->af_ioctrl_set_flash_notice(isp_handler,param_ptr,NULL);
		}

		break;

	case ISP_FLASH_MAIN_AE_MEASURE:
		//ISP_LOGI("ISP_AE: main-Flashing2");
		ae_notice.mode = AE_FLASH_MAIN_AE_MEASURE;
		ae_notice.flash_ratio = 0;
		rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_FLASH_NOTICE, &ae_notice, NULL);
		awb_flash_status = AWB_FLASH_MAIN_MEASURE;
		rtn = handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_SET_FLASH_STATUS, (void *)&awb_flash_status, NULL);
		break;

	case ISP_FLASH_AF_DONE:
		if (handle->ae_lib_fun->product_id) {
			ae_notice.mode = AE_FLASH_AF_DONE;
			rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_FLASH_NOTICE, &ae_notice, NULL);
		}
		break;

	default:
		break;
	}

	return rtn;
}

static int32_t _ispIsoIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct ae_set_iso               set_iso = {0};

	if (NULL == param_ptr)
		return ISP_PARAM_NULL;

	set_iso.mode = *(uint32_t*)param_ptr;
	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_ISO, &set_iso, NULL);
	ISP_LOGI("ISP_AE: AE_SET_ISO=%d, rtn=%d", set_iso.mode, rtn);

	return rtn;
}

static int32_t _ispBrightnessIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_bright_cfg           cfg = {0};
	struct isp_pm_param_data        param_data;
	struct isp_pm_ioctl_input       input = {NULL, 0};
	struct isp_pm_ioctl_output      output = {NULL, 0};

	cfg.factor = *(uint32_t*)param_ptr;
	memset(&param_data, 0x0, sizeof(param_data));
	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_BRIGHT, ISP_BLK_BRIGHT, &cfg, sizeof(cfg));

	rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_OTHERS, &input, &output);

	return rtn;
}

static int32_t _ispContrastIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_contrast_cfg         cfg = {0};
	struct isp_pm_param_data        param_data;
	struct isp_pm_ioctl_input       input = {NULL, 0};
	struct isp_pm_ioctl_output      output = {NULL, 0};

	cfg.factor = *(uint32_t*)param_ptr;
	memset(&param_data, 0x0, sizeof(param_data));
	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_CONTRAST, ISP_BLK_CONTRAST, &cfg, sizeof(cfg));

	rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_OTHERS, &input, &output);

	return rtn;
}

static int32_t _ispSaturationIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_saturation_cfg       cfg = {0};
	struct isp_pm_param_data        param_data;
	struct isp_pm_ioctl_input       input = {NULL, 0};
	struct isp_pm_ioctl_output      output = {NULL, 0};

	cfg.factor = *(uint32_t*)param_ptr;
	memset(&param_data, 0x0, sizeof(param_data));
	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_SATURATION, ISP_BLK_SATURATION_V1, &cfg, sizeof(cfg));

	rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_OTHERS, &input, &output);

	return rtn;
}

static int32_t _ispSharpnessIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_edge_cfg             cfg = {0};
	struct isp_pm_param_data        param_data;
	struct isp_pm_ioctl_input       input = {NULL, 0};
	struct isp_pm_ioctl_output      output = {NULL, 0};

	cfg.factor = *(uint32_t*)param_ptr;
	memset(&param_data, 0x0, sizeof(param_data));
	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_EDGE_STRENGTH, ISP_BLK_EDGE_V1, &cfg, sizeof(cfg));

	rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_OTHERS, &input, &output);

	return rtn;
}

static int32_t _ispVideoModeIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	int                             mode = 0;
	struct ae_set_fps               fps;
	struct isp_pm_param_data        param_data = {0x00};
	struct isp_pm_ioctl_input       input = {NULL, 0};
	struct isp_pm_ioctl_output      output = {NULL, 0};

	if (NULL == param_ptr) {
		ISP_LOGE("param is NULL error!");
		return ISP_ERROR;
	}

	ISP_LOGI("param val=%d", *((int*)param_ptr));

	if (0 == *((int*)param_ptr)) {
		mode = ISP_MODE_ID_PRV_0;
	} else {
		rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_MODEID_BY_FPS, param_ptr, &mode);
		if (rtn) {
			ISP_LOGE("isp_pm_ioctl failed");
		}
	}
	if (mode != handle->isp_mode) {
//		handle->isp_mode = mode;
//		rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_MODE, &handle->isp_mode, NULL);
	}

	fps.min_fps = *((uint32_t*)param_ptr);
	fps.max_fps = 0;//*((uint32_t*)param_ptr);
	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_FPS, &fps, NULL);
	ISP_TRACE_IF_FAIL(rtn, ("ae set fps error"));

	if (0 != *((uint32_t*)param_ptr)) {
		uint32_t work_mode = 2;
		rtn = handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_SET_WORK_MODE, &work_mode, NULL);
		ISP_RETURN_IF_FAIL(rtn, ("awb set_work_mode error"));
	} else {
		uint32_t work_mode = 0;
		rtn = handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_SET_WORK_MODE, &work_mode, NULL);
		ISP_RETURN_IF_FAIL(rtn, ("awb set_work_mode error"));
	}

#ifdef SEPARATE_GAMMA_IN_VIDEO
	if (*((uint32_t*)param_ptr) != 0) {
		uint32_t idx = VIDEO_GAMMA_INDEX;

		smart_ctl_block_disable(handle->handle_smart,ISP_SMART_GAMMA);
		BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_GAMMA, ISP_BLK_RGB_GAMC, &idx, sizeof(idx));
		isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_OTHERS, (void*)&input, (void*)&output);
	#ifdef Y_GAMMA_SMART_WITH_RGB_GAMMA
		BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_YGAMMA, ISP_BLK_Y_GAMMC, &idx, sizeof(idx));
		isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_OTHERS, (void*)&input, (void*)&output);
	#endif
	} else {
		smart_ctl_block_enable_recover(handle->handle_smart,ISP_SMART_GAMMA);
	}
#endif
	return rtn;
}

static int32_t _ispRangeFpsIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_range_fps            *range_fps = (struct isp_range_fps*)param_ptr;
	struct ae_set_fps               fps;

	if (NULL == range_fps) {
		ISP_LOGE("param is NULL error!");
		return ISP_PARAM_NULL;
	}

	ISP_LOGI("param val=%d", *((int*)param_ptr));

	fps.min_fps = range_fps->min_fps;
	fps.max_fps = range_fps->max_fps;
	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_FPS, &fps, NULL);

	return rtn;
}

static int32_t _ispAeOnlineIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;

	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_ONLINE_CTRL, param_ptr, param_ptr);

	return rtn;
}

static int32_t _ispAeForceIOCtrl(isp_handle isp_handler, void* param_ptr, int(*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct ae_calc_out              ae_result = {0};
	uint32_t                        ae;

	if (NULL == param_ptr) {
		ISP_LOGE("param is NULL error!");
		return ISP_PARAM_NULL;
	}

	ae = *(uint32_t*)param_ptr;

	if (0 == ae) {//lock
		rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_FORCE_PAUSE, NULL, (void*)&ae_result);
	} else {//unlock
		rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_FORCE_RESTORE, NULL, (void*)&ae_result);
	}

	ISP_LOGI("rtn %d", rtn);

	return rtn;
}

static int32_t _ispGetAeStateIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	uint32_t                        param = 0;

	if (NULL == param_ptr) {
		ISP_LOGE("param is NULL error!");
		return ISP_PARAM_NULL;
	}

	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_AE_STATE, NULL, (void*)&param);

	if (AE_STATE_LOCKED == param) {//lock
		*(uint32_t*)param_ptr = 0;
	} else {//unlock
		*(uint32_t*)param_ptr = 1;
	}

	ISP_LOGI("rtn %d param %d ae %d", rtn, param, *(uint32_t*)param_ptr);

	return rtn;
}

static int32_t _ispSetAeFpsIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct ae_set_fps               *fps = (struct ae_set_fps*)param_ptr;

	if (NULL == fps) {
		ISP_LOGE("param is NULL error!");
		return ISP_PARAM_NULL;
	}

	ISP_LOGI("LLS min_fps =%d, max_fps = %d", fps->min_fps, fps->max_fps);

	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_FPS, fps, NULL);

	return rtn;
}

static int32_t isp_get_debug_info(isp_ctrl_context *handle)
{
	int32_t rtn = ISP_SUCCESS;
	int32_t ret = ISP_SUCCESS;

	if ( NULL == handle) {
		ISP_LOGE("Get isp debug info error!");
		rtn =ISP_ERROR;
		return rtn;
	}

	if (0x00 == handle->ae_lib_fun->product_id) {
		struct tg_ae_ctrl_alc_log ae_log = {0};
		rtn  = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_DEBUG_INFO, NULL, (void *)&ae_log);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("Get AE debug info failed!");
		}
		handle->log_ae = ae_log.log;
		handle->log_ae_size = ae_log.size;
		ret += rtn;
	}

        // ALSC JPG Debug info
        struct alsc_ver_info lsc_ver = {0};
	 rtn  = handle->lsc_lib_fun->alsc_io_ctrl(handle->handle_lsc_adv, ALSC_GET_VER, NULL, (void *)&lsc_ver);
	 if (ISP_SUCCESS != rtn) {
		 ISP_LOGE("Get ALSC ver info failed!");
	 }	

	 if( lsc_ver.LSC_SPD_VERSION >= 3 ){
	 	if (0x00 == handle->lsc_lib_fun->product_id) {
			struct tg_alsc_debug_info lsc_log = {0};
			rtn  = handle->lsc_lib_fun->alsc_io_ctrl(handle->handle_lsc_adv, ALSC_CMD_GET_DEBUG_INFO, NULL, (void *)&lsc_log);
			if (ISP_SUCCESS != rtn) {
				ISP_LOGE("Get ALSC debug info failed!");
			}
			handle->log_lsc = lsc_log.log;
			handle->log_lsc_size = lsc_log.size;
			ret += rtn;
	 	}
	 }
	
	return ret;
}

static const char *DEBUG_MAGIC = "SPRD_ISP";  // 8 bytes
static const char *AE_START    = "ISP_AE__";
static const char *AE_END      = "ISP_AE__";
static const char *AF_START    = "ISP_AF__";
static const char *AF_END      = "ISP_AF__";
static const char *AWB_START   = "ISP_AWB_";
static const char *AWB_END     = "ISP_AWB_";
static const char *LSC_START   = "ISP_LSC_";
static const char *LSC_END     = "ISP_LSC_";

typedef struct _isp_log_info
{
	int32_t ae_off;
	uint32_t ae_len;
	int32_t af_off;
	uint32_t af_len;
	int32_t awb_off;
	uint32_t awb_len;
	int32_t lsc_off;
	uint32_t lsc_len;
	int32_t ver;
	char AF_version[20];//AF-yyyymmdd-xx
	char magic[8];
} isp_log_info_t;

static size_t calc_log_size(const void *log, size_t size, const char *begin_magic, const char *end_magic)
{
    if (!log || !size)
        return 0;

    return size + strlen(begin_magic) + strlen(end_magic);
}

#define COPY_MAGIC(m) \
{size_t len; len = strlen(m); memcpy((char *)dst + off, m, len); off += len;}

static size_t copy_log(void *dst, const void *log, size_t size, const char *begin_magic, const char *end_magic)
{
    size_t off = 0;

    if (!log || !size)
        return 0;

    COPY_MAGIC(begin_magic);
    memcpy((char *)dst + off, log, size); off += size;
    COPY_MAGIC(end_magic);
    return off;
}

#define COPY_LOG(l, L) \
{size_t len = copy_log(handle->log_isp + off, handle->log_##l, handle->log_##l##_size, L##_START, L##_END); \
if (len) {log.l##_off = off; off += len; log.l##_len = len;} else {log.l##_off = 0;}}

static int32_t _ispGetInfoIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_info                 *info_ptr = param_ptr;
	uint32_t                        total_size = 0;
	uint32_t                        mem_offset = 0;
	uint32_t                        log_ae_size = 0;
	size_t total, off;
	isp_log_info_t log;
	struct sprd_isp_debug_info *p;

	info_ptr->addr = 0;
	info_ptr->size = 0;

	if (NULL == info_ptr) {
		ISP_LOGE("err,param is null");
		return ISP_PARAM_NULL;
	}

	if (AL_AE_LIB == handle->ae_lib_fun->product_id) {
		log_ae_size = handle->log_alc_ae_size;
	}

	if (handle->alc_awb || log_ae_size) {
		total_size = handle->log_alc_awb_size + handle->log_alc_lsc_size;
		if (handle->ae_lib_fun->product_id) {
			total_size += log_ae_size;
		}
		if (handle->log_alc_size < total_size) {
			if (handle->log_alc != NULL) {
				free (handle->log_alc);
				handle->log_alc = NULL;
			}
			handle->log_alc = malloc(total_size);
			if (handle->log_alc == NULL) {
				handle->log_alc_size = 0;
				return ISP_ERROR;
			}
			handle->log_alc_size = total_size;
		}

		if (handle->log_alc_awb != NULL) {
			memcpy(handle->log_alc, handle->log_alc_awb, handle->log_alc_awb_size);
		}
		mem_offset += handle->log_alc_awb_size;
		if (handle->log_alc_lsc != NULL) {
			memcpy(handle->log_alc + mem_offset, handle->log_alc_lsc, handle->log_alc_lsc_size);
		}
		mem_offset += handle->log_alc_lsc_size;
		if (handle->ae_lib_fun->product_id) {
			if (handle->log_alc_ae != NULL) {
				memcpy(handle->log_alc + mem_offset, handle->log_alc_ae, handle->log_alc_ae_size);
			}
			mem_offset += handle->log_alc_ae_size;
		}
		info_ptr->addr = handle->log_alc;
		info_ptr->size = handle->log_alc_size;
	} else {

		if (ISP_SUCCESS != isp_get_debug_info(handle)) {
			ISP_LOGE("isp_get_debug_info failed");
		}

		total = sizeof(struct sprd_isp_debug_info) + sizeof(uint32_t)/*for end flag*/
				+ calc_log_size(handle->log_ae, handle->log_ae_size, AE_START, AE_END)
				+ calc_log_size(handle->log_af, handle->log_af_size, AF_START, AF_END)
				+ calc_log_size(handle->log_awb, handle->log_awb_size, AWB_START, AWB_END)
				+ calc_log_size(handle->log_lsc, handle->log_lsc_size, LSC_START, LSC_END)
				+ sizeof(isp_log_info_t);

		if (!handle->log_isp || (handle->log_isp_size < total)) {
			if (handle->log_isp) {
				free(handle->log_isp);
			}

			handle->log_isp = malloc(total);
			if (NULL == handle->log_isp) {
				ISP_LOGE("failed to malloc %d", total);
				info_ptr->addr = 0;
				info_ptr->size = 0;
				return ISP_ERROR;
			}
			handle->log_isp_size = total;
		}

		memset(&log, 0, sizeof(log));
		memcpy(log.magic, DEBUG_MAGIC, sizeof(log.magic));
		log.ver = 0;

		off = 0;

		p = (struct sprd_isp_debug_info *)handle->log_isp;
		p->debug_startflag = SPRD_DEBUG_START_FLAG;
		*((uint32_t*)((uint8_t*)p + total - 4))= SPRD_DEBUG_END_FLAG;
		p->debug_len	   = total;
		p->version_id	   = SPRD_DEBUG_VERSION_ID;

		off = sizeof(isp_log_info_t) + sizeof(struct sprd_isp_debug_info);

		COPY_LOG(ae, AE);
		COPY_LOG(af, AF);
		COPY_LOG(awb, AWB);
		COPY_LOG(lsc, LSC);

//		ISP_LOGI("ae oft: %d, len: %d, af oft: %d, len:%d,awb oft: %d, len: %d, lsc oft: %d, len: %d", \
//			log.ae_off, log.ae_len, log.af_off, log.af_len, log.awb_off, log.awb_len, log.lsc_off, log.lsc_len);

		if(NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_get_lib_version)
			handle->af_lib_fun->af_get_lib_version(handle,log.AF_version,sizeof(log.AF_version));

		memcpy((char*)handle->log_isp + sizeof(struct sprd_isp_debug_info), &log, sizeof(log));

		info_ptr->addr = handle->log_isp;
		info_ptr->size = handle->log_isp_size;

	}

	ISP_LOGI("ISP INFO:addr 0x%p, size = %d", info_ptr->addr, info_ptr->size);
	return rtn;
}

static int32_t _isp_get_awb_gain_ioctrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	struct awb_gain                 result;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_awbc_cfg             *awbc_cfg = (struct isp_awbc_cfg*)param_ptr;

	if (NULL == awbc_cfg) {
		ISP_LOGE("param is NULL error!");
		return ISP_PARAM_NULL;
	}

	rtn = handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_GET_GAIN, (void *)&result, NULL);

	awbc_cfg->r_gain = result.r;
	awbc_cfg->g_gain = result.g;
	awbc_cfg->b_gain = result.b;
	awbc_cfg->r_offset = 0;
	awbc_cfg->g_offset = 0;
	awbc_cfg->b_offset = 0;

	ISP_LOGI("rtn %d r %d g %d b %d", rtn, result.r, result.g, result.b);

	return rtn;
}

static int32_t _isp_get_rgb_gain_ioctrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	uint32_t* gain = (struct uint32_t*)param_ptr;

	if (NULL == gain) {
		ISP_LOGE("param is NULL error!");
		return ISP_PARAM_NULL;
	}

	*gain = handle->global_gain;

	return rtn;
}

static int32_t _ispSetLumIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	uint32_t                        param = 0;

	if (NULL == param_ptr) {
		ISP_LOGE("param is NULL error!");
		return ISP_PARAM_NULL;
	}

	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_TARGET_LUM, param_ptr, (void*)&param);

	ISP_LOGI("rtn %d param %d Lum %d", rtn, param, *(uint32_t*)param_ptr);

	return rtn;
}

static int32_t _ispGetLumIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	uint32_t                        param = 0;

	if (NULL == param_ptr) {
		ISP_LOGE("param is NULL error!");
		return ISP_PARAM_NULL;
	}

	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_LUM, NULL, (void*)&param);
	*(uint32_t*)param_ptr = param;

	ISP_LOGI("rtn %d param %d Lum %d", rtn, param, *(uint32_t*)param_ptr);

	return rtn;
}

static int32_t _ispHueIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_hue_cfg              cfg = {0};
	struct isp_pm_param_data        param_data;
	struct isp_pm_ioctl_input       input = {NULL, 0};
	struct isp_pm_ioctl_output      output = {NULL, 0};

	if (NULL == param_ptr) {
		ISP_LOGE("param is NULL error!");
		return ISP_PARAM_NULL;
	}

	cfg.factor = *(uint32_t*)param_ptr;
	memset(&param_data, 0x0, sizeof(param_data));
	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_HUE, ISP_BLK_HUE_V1, &cfg, sizeof(cfg));

	rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_OTHERS, &input, &output);

	return rtn;
}

static int32_t _ispAfStopIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;

	if(NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_set_af_stop)
		rtn = handle->af_lib_fun->af_ioctrl_set_af_stop(handle, NULL, NULL);

	return rtn;
}

static int32_t _ispOnlineFlashIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;

	handle->system.callback(handle->system.caller_id, ISP_CALLBACK_EVT|ISP_ONLINE_FLASH_CALLBACK, param_ptr, 0);

	return rtn;
}

static int32_t _ispAeMeasureLumIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct ae_set_weight            set_weight = {0};

	if (NULL == param_ptr) {
		ISP_LOGE("param is NULL error!");
		return ISP_PARAM_NULL;
	}

	set_weight.mode = *(uint32_t*)param_ptr;
	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_WEIGHT, &set_weight, NULL);
	ISP_LOGI("ISP_AE: AE_SET_WEIGHT=%d, rtn=%d", set_weight.mode, rtn);

	return rtn;
}

static int32_t _ispSceneModeIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct ae_set_scene             set_scene = {0};

	if (NULL == param_ptr) {
		ISP_LOGE("param is NULL error!");
		return ISP_PARAM_NULL;
	}
#if 0
	struct isp_pm_param_data param_data = {0x00};
	struct isp_pm_ioctl_input input = {NULL, 0};
	struct isp_pm_ioctl_output output = {NULL, 0};


	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_SCENE_MODE, ISP_BLK_BRIGHT, param_ptr, sizeof(param_ptr));
	isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_SCENE_MODE, (void*)&input, (void*)&output);

	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_SCENE_MODE, ISP_BLK_CONTRAST, param_ptr, sizeof(param_ptr));
	isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_SCENE_MODE, (void*)&input, (void*)&output);

	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_SCENE_MODE, ISP_BLK_EDGE_V1, param_ptr, sizeof(param_ptr));
	isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_SCENE_MODE, (void*)&input, (void*)&output);

	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_SCENE_MODE, ISP_BLK_SATURATION_V1, param_ptr, sizeof(param_ptr));
	isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_SCENE_MODE, (void*)&input, (void*)&output);
#endif
	set_scene.mode = *(uint32_t*)param_ptr;
	handle->scene_flag = *(uint32_t*)param_ptr;
	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_SCENE_MODE, &set_scene, NULL);
	ISP_LOGD("scene: %d\n", set_scene.mode);
	//rtn = awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_SET_SCENE_MODE, param_ptr, NULL);

	return rtn;
}

static int32_t _ispSFTIORead(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;

	if (NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_ioread) {
		rtn = handle->af_lib_fun->af_ioctrl_ioread(isp_handler, param_ptr, NULL);
	}

	return rtn;
}

static int32_t _ispSFTIOWrite(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;

	if (NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_iowrite) {
		rtn = handle->af_lib_fun->af_ioctrl_iowrite(isp_handler, param_ptr, NULL);
	}

	return rtn;
}

static int32_t _ispSFTIOSetPass(isp_handle isp_handler, void *param_ptr, int (*all_back)())
{
	return ISP_SUCCESS;
}

static int32_t _ispSFTIOSetBypass(isp_handle isp_handler, void *param_ptr, int (*all_back)())
{
	return ISP_SUCCESS;
}

static int32_t _ispSFTIOGetAfValue(isp_handle isp_handler, void *param_ptr, int (*all_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;

	if (NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_get_af_value) {
		rtn = handle->af_lib_fun->af_ioctrl_get_af_value(isp_handler, param_ptr, NULL);
	}

	//memmove(param_ptr,statis,sizeof(statis));

	return rtn;
}

int isp_SetDcamTimeStamp(isp_handle isp_handler, void* param_ptr,int capture){
	int32_t rtn = ISP_SUCCESS;

	isp_ctrl_context* handle = (isp_ctrl_context*)isp_handler;

	if(NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_set_dcam_timestamp)
		rtn = handle->af_lib_fun->af_ioctrl_set_dcam_timestamp(handle,param_ptr,capture);


	return rtn;
}

static int32_t _ispAfIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;

	if(NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_af_start)
		rtn = handle->af_lib_fun->af_ioctrl_af_start(isp_handler, param_ptr, NULL);

	return rtn;
}

static int32_t _ispBurstIONotice(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;

	if (NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_burst_notice) {
		rtn = handle->af_lib_fun->af_ioctrl_burst_notice(isp_handler, param_ptr, NULL);
	}

	return rtn;
}


static int32_t _ispSpecialEffectIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_pm_param_data        param_data = {0x00};
	struct isp_pm_ioctl_input       input = {NULL, 0};
	struct isp_pm_ioctl_output      output = {NULL, 0};

	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_SPECIAL_EFFECT, ISP_BLK_CCE_V1, param_ptr, sizeof(param_ptr));
	isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_SPECIAL_EFFECT, (void*)&input, (void*)&output);
#if 0
	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_SPECIAL_EFFECT, ISP_BLK_EMBOSS_V1, param_ptr, sizeof(param_ptr));
	isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_SPECIAL_EFFECT, (void*)&input, (void*)&output);

	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_SPECIAL_EFFECT, ISP_BLK_POSTERIZE, param_ptr, sizeof(param_ptr));
	isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_SPECIAL_EFFECT, (void*)&input, (void*)&output);

	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_SPECIAL_EFFECT, ISP_BLK_Y_GAMMC, param_ptr, sizeof(param_ptr));
	isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_SPECIAL_EFFECT, (void*)&input, (void*)&output);

	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_SPECIAL_EFFECT, ISP_BLK_HSV, param_ptr, sizeof(param_ptr));
	isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_SPECIAL_EFFECT, (void*)&input, (void*)&output);
#endif
	return rtn;
}

static int32_t _ispFixParamUpdateIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct sensor_raw_info          *sensor_raw_info_ptr= NULL;
	struct isp_pm_init_input        input;
	uint32_t                        i;
	uint32_t param_source = 0;
	struct sensor_ae_tab            *ae = NULL;
	struct sensor_lsc_map           *lnc = NULL;
	struct sensor_awb_map_weight_param *awb = NULL;
	struct isp_pm_ioctl_input awb_input= {0};
	struct isp_pm_ioctl_output awb_output = {0};
	struct awb_data_info awb_data_ptr = {0};

	if (NULL == handle || NULL == handle->sn_raw_info) {
		ISP_LOGE("update param error");
		rtn = ISP_ERROR;
		return rtn;
	}
	sensor_raw_info_ptr = (struct sensor_raw_info *)handle->sn_raw_info;
	ISP_LOGI("--IOCtrl--FIX_PARAM_UPDATE--");

	if (NULL == handle->handle_pm) {
		ISP_LOGE("param is NULL error!");
		return ISP_PARAM_NULL;
	}

	/* set param source flag */
	param_source = ISP_PARAM_FROM_TOOL;
	rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_PARAM_SOURCE,  (void *)&param_source, NULL);

	input.num = MAX_MODE_NUM;
	for (i = 0; i < MAX_MODE_NUM; i++) {
		if (NULL != sensor_raw_info_ptr->mode_ptr[i].addr) {
			ae  = &sensor_raw_info_ptr->fix_ptr[i]->ae;
			lnc = &sensor_raw_info_ptr->fix_ptr[i]->lnc;
			awb = &sensor_raw_info_ptr->fix_ptr[i]->awb;
			ISP_LOGI("ISP_TOOL: input.tuning_data[%d].data_ptr = %p,input.tuning_data[i].size = %d", i, handle->isp_update_data[i].data_ptr, handle->isp_update_data[i].size);
			if (NULL != ae->block.block_info || NULL != awb->block.block_info || NULL != lnc->block.block_info ) {
				if ( NULL != handle->isp_update_data[i].data_ptr ) {
					free(handle->isp_update_data[i].data_ptr);
					handle->isp_update_data[i].data_ptr = NULL;
					handle->isp_update_data[i].size     = 0;
				}
			}
		}
		ISP_LOGI("ISP_TOOL: input.tuning_data[%d].data_ptr = %p,input.tuning_data[i].size = %d", i, handle->isp_update_data[i].data_ptr, handle->isp_update_data[i].size);
		rtn = sensor_isp_param_merge(sensor_raw_info_ptr, &handle->isp_update_data[i], i);
		if (0 != rtn ) {
			ISP_LOGE("isp get param error");
		}
		input.tuning_data[i].data_ptr = handle->isp_update_data[i].data_ptr;
		input.tuning_data[i].size     = handle->isp_update_data[i].size;
		ISP_LOGI("ISP_TOOL: input.tuning_data[%d].data_ptr = %p,input.tuning_data[i].size = %d", i, input.tuning_data[i].data_ptr, input.tuning_data[i].size);
	}
	rtn = isp_pm_update(handle->handle_pm, ISP_PM_CMD_UPDATE_ALL_PARAMS, &input, PNULL);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("isp param update failed");
		return rtn;
	}
	param_source = 0;
	rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_PARAM_SOURCE,  (void *)&param_source, NULL);

	rtn = _smart_param_update(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("smart param update failed");
		return rtn;
	}

	if(handle->sn_raw_info->libuse_info->awb_lib_info.version_id & 0x01)
		rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_INIT_AWB_NEW, &awb_input, &awb_output);
	else
		rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_INIT_AWB, &awb_input, &awb_output);
	if (ISP_SUCCESS == rtn && awb_output.param_num) {
		awb_data_ptr.data_ptr = (void *)awb_output.param_data->data_ptr;
		awb_data_ptr.data_size = awb_output.param_data->data_size;
		handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_SET_UPDATE_TUNING_PARAM, (void *)&awb_data_ptr, NULL);
	}

	{
		struct isp_pm_ioctl_input input = {0};
		struct isp_pm_ioctl_output output = {0};
		struct isp_flash_param *flash_param_ptr;

		rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_INIT_AE, &input, &output);
		if (ISP_SUCCESS == rtn && output.param_num) {
			int32_t bypass = 0;
			uint32_t target_lum = 0;
			uint32_t *target_lum_ptr = NULL;

			bypass = output.param_data->user_data[0];
			target_lum_ptr = (uint32_t *)output.param_data->data_ptr;
			target_lum = target_lum_ptr[3];
			handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_BYPASS, &bypass, NULL);
#ifndef CONFIG_CAMERA_ISP_AE_VERSION_V1
			handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_TARGET_LUM, &target_lum, NULL);
#endif
		}

		rtn = _isp_get_flash_cali_param(handle->handle_pm, &flash_param_ptr);
		if (ISP_SUCCESS == rtn) {
			handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_FLASH_ON_OFF_THR, (void*)&flash_param_ptr->cur.auto_flash_thr, NULL);
		}
	}

	if( NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_set_af_bypass )
		handle->af_lib_fun->af_ioctrl_set_af_bypass(handle,NULL,NULL);

	return rtn;
}

static int32_t _ispParamUpdateIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_pm_init_input        input;
	uint32_t param_source = 0;
	struct isp_mode_param           *mode_param_ptr = param_ptr;
	uint32_t                        i;
	struct isp_pm_ioctl_input awb_input= {0};
	struct isp_pm_ioctl_output awb_output = {0};
	struct awb_data_info awb_data_ptr = {0};

	ISP_LOGI("--IOCtrl--PARAM_UPDATE--");

	if (NULL == mode_param_ptr) {
		ISP_LOGE("param is NULL error!");
		return ISP_PARAM_NULL;
	}

	if (NULL == handle || NULL == handle->handle_pm) {
		ISP_LOGE("param is NULL error!");
		return ISP_PARAM_NULL;
	}

	param_source = ISP_PARAM_FROM_TOOL;
	rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_PARAM_SOURCE, (void *)&param_source, NULL);

	input.num = MAX_MODE_NUM;
	for (i = 0; i < MAX_MODE_NUM; i++) {
		if (mode_param_ptr->mode_id == i) {
			input.tuning_data[i].data_ptr = mode_param_ptr;
			input.tuning_data[i].size = mode_param_ptr->size;
		} else {
			input.tuning_data[i].data_ptr = NULL;
			input.tuning_data[i].size = 0;
		}
		mode_param_ptr = (struct isp_mode_param*)((uint8_t*)mode_param_ptr + mode_param_ptr->size);
	}

	rtn = isp_pm_update(handle->handle_pm, ISP_PM_CMD_UPDATE_ALL_PARAMS, &input, NULL);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("isp param update failed");
		return rtn;
	}
	param_source = 0;
	rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_PARAM_SOURCE,  (void *)&param_source, NULL);

	rtn = _smart_param_update(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("smart param update failed");
		return rtn;
	}

	rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_INIT_AWB, &awb_input, &awb_output);
	if (ISP_SUCCESS == rtn && awb_output.param_num) {
		awb_data_ptr.data_ptr = (void *)awb_output.param_data->data_ptr;
		awb_data_ptr.data_size = awb_output.param_data->data_size;
		handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_SET_UPDATE_TUNING_PARAM, (void *)&awb_data_ptr, NULL);
	}

	{
		struct isp_pm_ioctl_input input = {0};
		struct isp_pm_ioctl_output output = {0};
 
		rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_INIT_AE, &input, &output);
		if (ISP_SUCCESS == rtn && output.param_num) {
			int32_t bypass = 0;

			bypass = output.param_data->user_data[0];
			handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_BYPASS, &bypass, NULL);
		}
	}
	return rtn;
}

static int32_t _ispAeTouchIOCtrl(isp_handle isp_handler, void* param_ptr, int(*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	int                             out_param = 0;
	struct isp_pos_rect             *rect = NULL;
	struct ae_set_tuoch_zone        touch_zone;

	if (NULL == param_ptr) {
		ISP_LOGE("param is NULL error");
		return ISP_PARAM_NULL;
	}

	memset(&touch_zone, 0, sizeof(touch_zone));
	rect = (struct isp_pos_rect*)param_ptr;
	touch_zone.touch_zone.x = rect->start_x;
	touch_zone.touch_zone.y = rect->start_y;
	touch_zone.touch_zone.w = rect->end_x - rect->start_x + 1;
	touch_zone.touch_zone.h = rect->end_y - rect->start_y + 1;
	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_TOUCH_ZONE, &touch_zone, &out_param);

	ISP_LOGI("ISP_CTRL_TC_cordination is (x,y)=(%d,%d),w,h=(%d,%d)", touch_zone.touch_zone.x,touch_zone.touch_zone.y,touch_zone.touch_zone.w, touch_zone.touch_zone.h);
	return rtn;
}

static int32_t _ispAfModeIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                          rtn = ISP_SUCCESS;
	isp_ctrl_context                 *handle = (isp_ctrl_context*)isp_handler;

	if (NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_set_af_mode) {
		rtn = handle->af_lib_fun->af_ioctrl_set_af_mode(isp_handler, param_ptr, NULL);
	}

	return rtn;
}

int32_t isp_set_take_picture_mode(isp_handle isp_handler, void* param_ptr)
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;

	if (NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_set_af_mode) {
		rtn = handle->af_lib_fun->af_ioctrl_set_af_mode(isp_handler,param_ptr,NULL);
	}

	return rtn;
}

static int32_t _ispGetAfModeIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;

	if (NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_get_af_mode) {
		rtn = handle->af_lib_fun->af_ioctrl_get_af_mode(isp_handler, param_ptr, NULL);
	}

	return rtn;
}

static int32_t _ispAfInfoIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;

	if (NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_af_info) {
		rtn = handle->af_lib_fun->af_ioctrl_af_info(isp_handler, param_ptr, NULL);
	}

	return rtn;
}

static int32_t _ispGetAfPosIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;

	if (NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_get_af_cur_pos != NULL) {
		rtn = handle->af_lib_fun->af_ioctrl_get_af_cur_pos(isp_handler, param_ptr,
						NULL);
	}

	return rtn;
}

static int32_t _ispSetAfPosIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;

	if(NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_set_af_pos)
		rtn = handle->af_lib_fun->af_ioctrl_set_af_pos(handle->handle_af,
			param_ptr, NULL);

	return rtn;
}

static int32_t _ispRegIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_reg_ctrl             *reg_ctrl_ptr = (struct isp_reg_ctrl*)param_ptr;

	ISP_LOGI("--IOCtrl--REG_CTRL--mode:0x%x", reg_ctrl_ptr->mode);

	if (ISP_CTRL_SET == reg_ctrl_ptr->mode) {
		if (0 != isp_dev_reg_write(handle->handle_device, reg_ctrl_ptr->num, reg_ctrl_ptr->reg_tab)) {
			rtn = ISP_ERROR;
		}
	} else {
		if (0 != isp_dev_reg_read(handle->handle_device, reg_ctrl_ptr->num, reg_ctrl_ptr->reg_tab)) {
			rtn = ISP_ERROR;
		}
	}

	return rtn;
}

static int32_t _ispScalerTrimIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_trim_size            *trim = (struct isp_trim_size*)param_ptr;

	if (NULL != trim) {
		struct ae_trim scaler;

		scaler.x = trim->x;
		scaler.y = trim->y;
		scaler.w = trim->w;
		scaler.h = trim->h;

		ISP_LOGI("x=%d,y=%d,w=%d,h=%d",scaler.x, scaler.y, scaler.w, scaler.h);

		if (handle->ae_lib_fun->ae_io_ctrl) {
			rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_STAT_TRIM, &scaler, NULL);
		}
	}

	return rtn;
}

static int32_t _ispFaceAreaIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_face_area            *face_area = (struct isp_face_area*)param_ptr;

	if (NULL != face_area) {
		struct ae_fd_param fd_param;
		int32_t i;

		fd_param.width    = face_area->frame_width;
		fd_param.height   = face_area->frame_height;
		fd_param.face_num = face_area->face_num;
		for (i = 0; i < fd_param.face_num; ++i) {
			fd_param.face_area[i].rect.start_x = face_area->face_info[i].sx;
			fd_param.face_area[i].rect.start_y = face_area->face_info[i].sy;
			fd_param.face_area[i].rect.end_x   = face_area->face_info[i].ex;
			fd_param.face_area[i].rect.end_y   = face_area->face_info[i].ey;
			fd_param.face_area[i].face_lum     = face_area->face_info[i].brightness;
			fd_param.face_area[i].pose         = face_area->face_info[i].pose;
		}

		if (handle->ae_lib_fun->ae_io_ctrl) {
			handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_FD_PARAM, &fd_param, NULL);
		}

		if(NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_set_fd_update) {
			handle->af_lib_fun->af_ioctrl_set_fd_update(isp_handler, param_ptr, NULL);
		}
	}

	return rtn;
}

static int32_t _ispStart3AIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct ae_calc_out              ae_result = {0};
	uint32_t                        af_bypass = 0;

	if (handle->ae_lib_fun->ae_io_ctrl) {
		handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_FORCE_RESTORE, NULL, (void*)&ae_result);
	}

	if (handle->awb_lib_fun->awb_ctrl_ioctrl) {
		handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_UNLOCK, NULL, NULL);
	}

	if(NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_set_af_bypass)
		handle->af_lib_fun->af_ioctrl_set_af_bypass(isp_handler, (void *)&af_bypass, NULL);

	ISP_LOGI("done");

	return ISP_SUCCESS;
}

static int32_t _ispStop3AIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	isp_ctrl_context                *handle = (isp_ctrl_context *)isp_handler;
	struct ae_calc_out              ae_result = {0};
	uint32_t                        af_bypass = 1;

	if (handle->ae_lib_fun->ae_io_ctrl) {
		handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_FORCE_PAUSE, NULL, (void*)&ae_result);
	}

	if (handle->awb_lib_fun->awb_ctrl_ioctrl) {
		handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_LOCK, NULL, NULL);
	}

	if(NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_set_af_bypass)
		handle->af_lib_fun->af_ioctrl_set_af_bypass(isp_handler, (void *)&af_bypass, NULL);

	ISP_LOGI("done");

	return ISP_SUCCESS;
}

static int32_t _ispHdrIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	isp_ctrl_context                *ctrl_context = (isp_ctrl_context *)isp_handler;
	int16_t                         smart_block_eb[ISP_SMART_MAX_BLOCK_NUM];
	SENSOR_EXT_FUN_PARAM_T          hdr_ev_param;
	struct isp_hdr_ev_param         *isp_hdr = NULL;

	if (NULL == ctrl_context || NULL == param_ptr) {
		ISP_LOGE("cxt=%p param_ptr=%p is NULL error", ctrl_context, param_ptr);
		return ISP_PARAM_NULL;
	}

	if (NULL == ctrl_context->ioctrl_ptr) {
		ISP_LOGE("ioctl is NULL error");
		return ISP_PARAM_NULL;
	}

	if (NULL == ctrl_context->ioctrl_ptr->ext_fuc) {
		ISP_LOGE("ext_fuc is NULL error");
		return ISP_PARAM_NULL;
	}

	isp_hdr = (struct isp_hdr_ev_param*)param_ptr;

	memset(&smart_block_eb, 0x00, sizeof(smart_block_eb));

	smart_ctl_block_eb(ctrl_context->handle_smart, &smart_block_eb, 0);
	if (ctrl_context->awb_lib_fun->awb_ctrl_ioctrl) {
		ctrl_context->awb_lib_fun->awb_ctrl_ioctrl(ctrl_context->handle_awb, AWB_CTRL_CMD_LOCK, NULL, NULL);
	}

	hdr_ev_param.cmd   = SENSOR_EXT_EV;
	hdr_ev_param.param = isp_hdr->level & 0xFF;
	ctrl_context->ioctrl_ptr->ext_fuc(&hdr_ev_param);
	if (ctrl_context->awb_lib_fun->awb_ctrl_ioctrl) {
		ctrl_context->awb_lib_fun->awb_ctrl_ioctrl(ctrl_context->handle_awb, AWB_CTRL_CMD_UNLOCK, NULL, NULL);
	}
	smart_ctl_block_eb(ctrl_context->handle_smart, &smart_block_eb, 1);
	if (ctrl_context->ae_lib_fun->ae_io_ctrl) {
//		ctrl_context->ae_lib_fun->ae_io_ctrl(ctrl_context->handle_ae, AE_GET_SKIP_FRAME_NUM, NULL, &isp_hdr->skip_frame_num);
	}

	return ISP_SUCCESS;
}

static int32_t _ispSetAeNightModeIOCtrl(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	uint32_t                        night_mode;

	if (NULL == param_ptr) {
		return ISP_PARAM_NULL;
	}

	night_mode = *(uint32_t*)param_ptr;
	if (handle->ae_lib_fun->ae_io_ctrl) {
		rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_NIGHT_MODE, &night_mode, NULL);
	}

	ISP_LOGI("ISP_AE: AE_SET_NIGHT_MODE=%d, rtn=%d", night_mode, rtn);

	return rtn;
}

static int32_t _ispSetAeAwbLockUnlock(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct ae_calc_out              ae_result = {0};
	uint32_t                        ae_awb_mode;

	if (NULL == param_ptr) {
		return ISP_PARAM_NULL;
	}

	ae_awb_mode = *(uint32_t*)param_ptr;
	if (ISP_AE_AWB_LOCK == ae_awb_mode) { // AE & AWB Lock
		ISP_LOGI("AE & AWB Lock");
		if (handle->ae_lib_fun->ae_io_ctrl) {
			handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_PAUSE, NULL, (void*)&ae_result);
		}
		if (handle->awb_lib_fun->awb_ctrl_ioctrl) {
			handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_LOCK, NULL, NULL);
		}
	} else if (ISP_AE_AWB_UNLOCK == ae_awb_mode) { // AE & AWB Unlock
		ISP_LOGI("AE & AWB Un-Lock\n");
		if (handle->ae_lib_fun->ae_io_ctrl) {
			handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_RESTORE, NULL, (void*)&ae_result);
		}
		if (handle->awb_lib_fun->awb_ctrl_ioctrl) {
			handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_UNLOCK, NULL, NULL);
		}
	} else {
		ISP_LOGI("Unsupported AE & AWB mode (%d)\n", ae_awb_mode);
	}

	return ISP_SUCCESS;
}

static int32_t _ispSetAeLockUnlock(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct ae_calc_out              ae_result = {0};
	uint32_t                        ae_mode;

	if (NULL == param_ptr) {
		return ISP_PARAM_NULL;
	}

	ae_mode = *(uint32_t*)param_ptr;
	if (ISP_AE_LOCK == ae_mode) { // AE & AWB Lock
		ISP_LOGI("AE Lock\n");
		if (handle->ae_lib_fun->ae_io_ctrl) {
			handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_PAUSE, NULL, (void*)&ae_result);
		}
	} else if (ISP_AE_UNLOCK == ae_mode) { // AE  Unlock
		ISP_LOGI("AE Un-Lock\n");
		if (handle->ae_lib_fun->ae_io_ctrl) {
			handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_RESTORE, NULL, (void*)&ae_result);
		}
	} else {
		ISP_LOGI("Unsupported AE  mode (%d)\n", ae_mode);
	}

	return ISP_SUCCESS;
}

static int32_t _ispDenoiseParamRead(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct sensor_raw_info          *raw_sensor_ptr = handle->sn_raw_info;
	struct isp_mode_param* mode_common_ptr = (struct isp_mode_param*)raw_sensor_ptr->mode_ptr[0].addr;
	struct denoise_param_update     *update_param = (struct denoise_param_update*)param_ptr;
	uint32_t i;

	if (NULL == param_ptr) {
		return ISP_PARAM_NULL;
	}
	if(handle->multi_nr_flag == SENSOR_MULTI_MODE_FLAG) {
		update_param->multi_nr_flag = SENSOR_MULTI_MODE_FLAG;
	}
	for (i = 0; i < mode_common_ptr->block_num; i++) {
		struct isp_block_header *header = &(mode_common_ptr->block_header[i]);
		uint8_t *data = (uint8_t*)mode_common_ptr + header->offset;

		switch (header->block_id) {
		case ISP_BLK_PRE_WAVELET_V1: {
			struct sensor_pwd_param* block = (struct sensor_pwd_param*)data;
			update_param->pwd_level_ptr = block->param_ptr;
			break;
		}
		case ISP_BLK_BPC_V1: {
			struct sensor_bpc_param_v1* block = (struct sensor_bpc_param_v1*)data;
			update_param->bpc_level_ptr = block->param_ptr;
			break;
		}
		case ISP_BLK_BL_NR_V1: {
			struct sensor_bdn_param* block = (struct sensor_bdn_param*)data;
			update_param->bdn_level_ptr = block->param_ptr;
			break;
		}
		case ISP_BLK_GRGB_V1: {
			struct sensor_grgb_v1_param* block = (struct sensor_grgb_v1_param*)data;
			update_param->grgb_v1_level_ptr = block->param_ptr;
			break;
		}
		case ISP_BLK_NLM: {
			struct sensor_nlm_param* block = (struct sensor_nlm_param*)data;
			update_param->nlm_level_ptr = block->param_nlm_ptr;
			update_param->vst_level_ptr = block->param_vst_ptr;
			update_param->ivst_level_ptr =  block->param_ivst_ptr;
			update_param->flat_offset_level_ptr = block->param_flat_offset_ptr;
			break;
		}
		case ISP_BLK_CFA_V1: {
			struct sensor_cfa_param_v1* block = (struct sensor_cfa_param_v1*)data;
			update_param->cfae_level_ptr = block->param_ptr;
			break;
		}
		case ISP_BLK_RGB_PRECDN: {
			struct sensor_rgb_precdn_param* block = (struct sensor_rgb_precdn_param*)data;
			update_param->rgb_precdn_level_ptr = block->param_ptr;
			break;
		}
		case ISP_BLK_YUV_PRECDN: {
			struct sensor_yuv_precdn_param* block = (struct sensor_yuv_precdn_param*)data;
			update_param->yuv_precdn_level_ptr = block->param_ptr;
			break;
		}
		case ISP_BLK_PREF_V1: {
			struct sensor_prfy_param* block = (struct sensor_prfy_param*)data;
			update_param->prfy_level_ptr = block->param_ptr;
			break;
		}
		case ISP_BLK_UV_CDN: {
			struct sensor_uv_cdn_param* block = (struct sensor_uv_cdn_param*)data;
			update_param->uv_cdn_level_ptr = block->param_ptr;
			break;
		}
		case ISP_BLK_EDGE_V1: {
			struct sensor_ee_param* block = (struct sensor_ee_param*)data;
			update_param->ee_level_ptr = block->param_ptr;
			break;
		}
		case ISP_BLK_UV_POSTCDN: {
			struct sensor_uv_postcdn_param* block = (struct sensor_uv_postcdn_param*)data;
			update_param->uv_postcdn_level_ptr = block->param_ptr;
			break;
		}
		case ISP_BLK_IIRCNR_IIR: {
			struct sensor_iircnr_param* block = (struct sensor_iircnr_param*)data;
			update_param->iircnr_level_ptr = block->param_ptr;
			break;
		}
		case ISP_BLK_IIRCNR_YRANDOM: {
			struct sensor_iircnr_yrandom_param* block = (struct sensor_iircnr_yrandom_param*)data;
			update_param->iircnr_yrandom_level_ptr = block->param_ptr;
			break;
		}
		case ISP_BLK_UVDIV_V1: {
			struct sensor_cce_uvdiv_param_v1* block = (struct sensor_cce_uvdiv_param_v1*)data;
			update_param->cce_uvdiv_level_ptr = block->param_ptr;
			break;
		}
		case ISP_BLK_YIQ_AFM:{
			struct sensor_y_afm_param *block = (struct sensor_y_afm_param*)data;
			update_param->y_afm_level_ptr = block->param_ptr;
			break;
		}
		default:
			break;
		}
	}
	ISP_LOGD("_ispDenoiseParamRead over");
	return ISP_SUCCESS;
}

static int32_t _ispSensorDenoiseParamUpdate(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct sensor_raw_info          *raw_sensor_ptr = handle->sn_raw_info;
	struct isp_mode_param           *mode_common_ptr = raw_sensor_ptr->mode_ptr[0].addr;
	struct denoise_param_update     *update_param = (struct denoise_param_update*)param_ptr;
	uint32_t i;

	if (NULL == param_ptr) {
		return ISP_PARAM_NULL;
	}

	for (i = 0; i < mode_common_ptr->block_num; i++) {
		struct isp_block_header *header = &(mode_common_ptr->block_header[i]);
		uint8_t *data = (uint8_t*)mode_common_ptr + header->offset;

		switch (header->block_id) {
		case ISP_BLK_PRE_WAVELET_V1: {
			struct sensor_pwd_param* block = (struct sensor_pwd_param*)data;
			block->param_ptr = update_param->pwd_level_ptr;
			break;
		}
		case ISP_BLK_BPC_V1: {
			struct sensor_bpc_param_v1* block = (struct sensor_bpc_param_v1*)data;
			block->param_ptr = update_param->bpc_level_ptr;
			break;
		}
		case ISP_BLK_BL_NR_V1: {
			struct sensor_bdn_param* block = (struct sensor_bdn_param*)data;
			block->param_ptr = update_param->bdn_level_ptr;
			break;
		}
		case ISP_BLK_GRGB_V1: {
			struct sensor_grgb_v1_param* block = (struct sensor_grgb_v1_param*)data;
			block->param_ptr = update_param->grgb_v1_level_ptr;
			break;
		}
		case ISP_BLK_NLM: {
			struct sensor_nlm_param* block = (struct sensor_nlm_param*)data;
			block->param_nlm_ptr = update_param->nlm_level_ptr;
			block->param_vst_ptr = update_param->vst_level_ptr;
			block->param_ivst_ptr = update_param->ivst_level_ptr;
			block->param_flat_offset_ptr = update_param->flat_offset_level_ptr;
			break;
		}
		case ISP_BLK_CFA_V1: {
			struct sensor_cfa_param_v1* block = (struct sensor_cfa_param_v1*)data;
			block->param_ptr = update_param->cfae_level_ptr;
			break;
		}
		case ISP_BLK_RGB_PRECDN: {
			struct sensor_rgb_precdn_param* block = (struct sensor_rgb_precdn_param*)data;
			block->param_ptr = update_param->rgb_precdn_level_ptr;
			break;
		}
		case ISP_BLK_YUV_PRECDN: {
			struct sensor_yuv_precdn_param* block = (struct sensor_yuv_precdn_param*)data;
			block->param_ptr = update_param->yuv_precdn_level_ptr;
			break;
		}
		case ISP_BLK_PREF_V1: {
			struct sensor_prfy_param* block = (struct sensor_prfy_param*)data;
			block->param_ptr = update_param->prfy_level_ptr;
			break;
		}
		case ISP_BLK_UV_CDN: {
			struct sensor_uv_cdn_param* block = (struct sensor_uv_cdn_param*)data;
			block->param_ptr = update_param->uv_cdn_level_ptr;
			break;
		}
		case ISP_BLK_EDGE_V1: {
			struct sensor_ee_param* block = (struct sensor_ee_param*)data;
			block->param_ptr = update_param->ee_level_ptr;
			break;
		}
		case ISP_BLK_UV_POSTCDN: {
			struct sensor_uv_postcdn_param* block = (struct sensor_uv_postcdn_param*)data;
			block->param_ptr = update_param->uv_postcdn_level_ptr;
			break;
		}
		case ISP_BLK_IIRCNR_IIR: {
			struct sensor_iircnr_param* block = (struct sensor_iircnr_param*)data;
			block->param_ptr = update_param->iircnr_level_ptr;
			break;
		}
		case ISP_BLK_IIRCNR_YRANDOM: {
			struct sensor_iircnr_yrandom_param* block = (struct sensor_iircnr_yrandom_param*)data;
			block->param_ptr = update_param->iircnr_yrandom_level_ptr;
			break;
		}
		case ISP_BLK_UVDIV_V1: {
			struct sensor_cce_uvdiv_param_v1* block = (struct sensor_cce_uvdiv_param_v1*)data;
			block->param_ptr = update_param->cce_uvdiv_level_ptr;
			break;
		}
		case ISP_BLK_YIQ_AFM:{
			struct sensor_y_afm_param *block = (struct sensor_y_afm_param*)data;
			block->param_ptr = update_param->y_afm_level_ptr;
			break;
		}
		default:
			break;
		}
	}
	ISP_LOGI("_ispSensorDenoiseParamUpdate over");
	return ISP_SUCCESS;
}

static int32_t _ispDumpReg(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         ret = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	uint32_t                        num;
	uint8_t                         buff[27];
	uint8_t                         *tx_ptr = NULL;
	struct isp_info                 *param_info = (struct isp_info*)param_ptr;
	struct isp_file                 *file;
	struct isp_reg_bits             *temp;
	struct isp_reg_bits             *param;

	if (NULL == handle || NULL == param_info) {
		return ISP_PARAM_ERROR;
	}

	file = (struct isp_file*)(handle->handle_device);

	param = (struct isp_reg_bits*)malloc(ISP_REG_NUM * sizeof(struct isp_reg_bits));
	if (NULL == param) {
		ISP_LOGE("error: _isp_reg_read---param");
		return ISP_ALLOC_ERROR;
	}
	memset(param, 0, ISP_REG_NUM * sizeof(struct isp_reg_bits));
	temp = param;

	ret = ioctl(file->fd, ISP_REG_READ, param);
	if (-1 == ret) {
		ISP_LOGE("error: _isp_reg_read---ioctl");
		free(param);
		return ISP_ERROR;
	}
	tx_ptr = (uint8_t*)malloc(sizeof(buff) * ISP_REG_NUM);
	if (NULL == tx_ptr) {
		ISP_LOGE("error: _isp_reg_read---tx_ptr");
		free(param);
		return ISP_ALLOC_ERROR;
	}
	memset(tx_ptr, 0, sizeof(buff) * ISP_REG_NUM);
	for(num = 0; num < ISP_REG_NUM; num++) {
		memset(buff, '\n', sizeof(buff));
		sprintf(buff, "0x%08x      0x%08x", temp->reg_addr, temp->reg_value);
		buff[26] = '\n';
		memcpy(tx_ptr + num * sizeof(buff), buff, sizeof(buff));
		temp += 1;
	}
	free(param);
	param_info->addr = (void*)tx_ptr;
	param_info->size = sizeof(buff) * ISP_REG_NUM;

	return ret;
}


static int32_t _ispToolSetSceneParam(isp_handle isp_handler, void *param_ptr, int(*call_back)())
{
	isp_ctrl_context* handle = (isp_ctrl_context*)isp_handler;
	struct isptool_scene_param *scene_parm = NULL;
	struct isp_pm_ioctl_input ioctl_input;
	struct isp_pm_param_data ioctl_data;
	struct isp_awbc_cfg awbc_cfg;
	uint32_t rtn = ISP_SUCCESS;
	UNUSED(call_back);

	scene_parm = (struct isptool_scene_param *)param_ptr;
	/*set awb gain*/
	awbc_cfg.r_gain = scene_parm->awb_gain_r;
	awbc_cfg.g_gain = scene_parm->awb_gain_g;
	awbc_cfg.b_gain = scene_parm->awb_gain_b;
	awbc_cfg.r_offset = 0;
	awbc_cfg.g_offset = 0;
	awbc_cfg.b_offset = 0;

	ioctl_data.id = ISP_BLK_AWB_V1;
	ioctl_data.cmd = ISP_PM_BLK_AWBC;
	ioctl_data.data_ptr = &awbc_cfg;
	ioctl_data.data_size = sizeof(awbc_cfg);

	ioctl_input.param_data_ptr = &ioctl_data;
	ioctl_input.param_num = 1;

	if(0 == awbc_cfg.r_gain && 0 == awbc_cfg.g_gain && 0 == awbc_cfg.b_gain) {
		awbc_cfg.r_gain = 1800;
		awbc_cfg.g_gain = 1024;
		awbc_cfg.b_gain = 1536;
	}

	ISP_LOGI("AWB_TAG: isp_pm_ioctl rtn=%d, gain=(%d, %d, %d)", rtn, awbc_cfg.r_gain, awbc_cfg.g_gain, awbc_cfg.b_gain);
	rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_AWB, (void *)&ioctl_input, NULL);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("set awb gain failed");
		return rtn;
	}

	ISP_LOGI("ISP_SMART: bv=%d, ct=%d, bv_gain=%d",
			scene_parm->smart_bv, scene_parm->smart_ct, scene_parm->gain);
	rtn = _smart_calc(handle, scene_parm->smart_bv, scene_parm->gain, scene_parm->smart_ct, 0);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("set smart gain failed");
		return rtn;
	}
	rtn =  _isp_change_lsc_param(handle, isp_cur_ct);

	return rtn;
}

static int32_t _ispGetSimulatorLscAddr(isp_handle isp_handler, void *param_ptr, int(*call_back)())
{
	uint32_t rtn = ISP_SUCCESS;
	isp_ctrl_context* handle = (isp_ctrl_context*)isp_handler;
	struct lsc_isp_addr* lsc_addr = (struct lsc_isp_addr *)param_ptr;

	if (NULL == lsc_addr->lsc_phys_addr || NULL == lsc_addr->lsc_vir_addr) {
		ISP_LOGE("Isp simulator lsc addr error!");
		rtn = ISP_ERROR;
		return rtn;
	}

	handle->isp_lsc_len        = lsc_addr->lsc_buf_size;
	handle->isp_lsc_physaddr   = lsc_addr->lsc_phys_addr;
	handle->isp_lsc_virtaddr   = lsc_addr->lsc_vir_addr;
	ISP_LOGI("Isp simulator lsc phys_addr = %p,vir_addr = %p,buf_size = %d",handle->isp_lsc_physaddr,
									handle->isp_lsc_virtaddr,handle->isp_lsc_len);

	return rtn;
}

static int32_t _ispForceAeQuickMode(isp_handle isp_handler, void *param_ptr, int(*call_back)())
{
	isp_ctrl_context* handle = (isp_ctrl_context*)isp_handler;
	uint32_t rtn = ISP_SUCCESS;
	uint32_t force_quick_mode = *(uint32_t*)param_ptr;
	UNUSED(call_back);
#ifndef CONFIG_CAMERA_ISP_AE_VERSION_V1
	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_FORCE_QUICK_MODE, (void*)&force_quick_mode, NULL);
#endif
	return rtn;
}

static int32_t _ispSetAeExpTime(isp_handle isp_handler, void *param_ptr, int(*call_back)())
{
	isp_ctrl_context* handle = (isp_ctrl_context*)isp_handler;
	uint32_t rtn = ISP_SUCCESS;
	uint32_t exp_time = *(uint32_t*)param_ptr;
	UNUSED(call_back);

	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_EXP_TIME, (void*)&exp_time, NULL);
	return rtn;
}

static int32_t _ispSetAeSensitivity(isp_handle isp_handler, void *param_ptr, int(*call_back)())
{
	isp_ctrl_context* handle = (isp_ctrl_context*)isp_handler;
	uint32_t rtn = ISP_SUCCESS;
	uint32_t sensitivity = *(uint32_t*)param_ptr;
	UNUSED(call_back);

	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_SET_SENSITIVITY, (void*)&sensitivity, NULL);
	return rtn;
}

static int32_t _ispEmAwbGetParam(isp_handle isp_handler, void *param_ptr)
{
	uint32_t rtn = ISP_SUCCESS;
	isp_ctrl_context *handle = (isp_ctrl_context *)isp_handler;
	if (NULL == param_ptr) {
		ISP_LOGE("ERROR:Em Ae Param ptr is NULL");
		rtn = ISP_PARAM_NULL;
		return rtn;
	}
	rtn = handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_EM_GET_PARAM, NULL, param_ptr);
	return rtn;
}

static int32_t _ispEmAFGetParam(isp_handle isp_handler, void *param_ptr)
{
	uint32_t rtn = ISP_SUCCESS;
	isp_ctrl_context *handle = (isp_ctrl_context *)isp_handler;
	if (NULL == param_ptr) {
		ISP_LOGE("ERROR:Em Ae Param ptr is NULL");
		rtn = ISP_PARAM_NULL;
		return rtn;
	}

	if(NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_interface)
		rtn = handle->af_lib_fun->af_ioctrl_interface(handle->handle_af, AF_CMD_GET_EM_PARAM, NULL, param_ptr);
	return rtn;
}

static int32_t _ispEmAeGetParam(isp_handle isp_handler, void *param_ptr)
{
	int32_t rtn = ISP_SUCCESS;
	isp_ctrl_context *handle = (isp_ctrl_context *)isp_handler;
	if (NULL == param_ptr) {
		ISP_LOGE("ERROR:Em Ae Param ptr is NULL");
		rtn = ISP_PARAM_NULL;
		return rtn;
	}
	struct debug_ae_display_param *ae_param_out = (struct debug_ae_display_param *)param_ptr;

	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_EM_PARAM, NULL, param_ptr);

	return rtn;
}

static int32_t _ispEmGetParam(isp_handle isp_handler, void *param_ptr, int(*call_back)())
{
	int32_t rtn = ISP_SUCCESS;
	struct em_param *param = (struct em_param *)param_ptr;
	UNUSED(call_back);
	if (NULL == param_ptr) {
		ISP_LOGE("ERROR:Em Param ptr is NULL");
		rtn = ISP_PARAM_NULL;
		return rtn;
	}
	switch(param->em_mode) {
		case EM_AE_MODE:
			rtn = _ispEmAeGetParam(isp_handler, param->param_ptr);
		break;
		case EM_AWB_MODE:
			rtn = _ispEmAwbGetParam(isp_handler, param->param_ptr);
		break;
		case EM_AF_MODE:
			rtn = _ispEmAFGetParam(isp_handler, param->param_ptr);
		break;
		default:
			break;
	}
	return rtn;
}

static int32_t _ispSetCaptureRawMode(isp_handle isp_handler, void *param_ptr, int(*call_back)())
{
	isp_ctrl_context* handle = (isp_ctrl_context*)isp_handler;
	int32_t rtn = ISP_SUCCESS;
	struct isp_video_start isp_video_start = {0};
	struct img_size *size_param = (struct img_size *)param_ptr;
	int mode = 0;
	UNUSED(call_back);
	isp_video_start.work_mode = 1;
	isp_video_start.size.w = size_param->width;
	isp_video_start.size.h = size_param->height;
	rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_MODEID_BY_RESOLUTION, &isp_video_start, &mode);
	handle->mode_flag = mode;
	return rtn;
}

static int32_t _isp_hdr_start(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;

	memset(&handle->smart_block_eb[0], 0x00, sizeof(handle->smart_block_eb));
	smart_ctl_block_eb(handle->handle_smart, &handle->smart_block_eb[0], 0);
	if (handle->awb_lib_fun->awb_ctrl_ioctrl) {
		handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_LOCK, NULL, NULL);
	}
	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_HDR_START, NULL, NULL);
	ISP_LOGI("_isp_hdr_start\r\n");
	return rtn;
}

static int32_t _isp_hdr_finish(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;

	smart_ctl_block_eb(handle->handle_smart, &handle->smart_block_eb[0], 1);
	if (handle->awb_lib_fun->awb_ctrl_ioctrl) {
		handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_UNLOCK, NULL, NULL);
	}

	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_HDR_FINISH, NULL, NULL);
	ISP_LOGI("_isp_hdr_finish\r\n");
	return rtn;
}

static int32_t _isp_hdr_ginfo(isp_handle isp_handler, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_hdr_ev_param			*phdr = (struct isp_hdr_ev_param *)param_ptr;
	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_HDR_GET_INFO, &phdr->level, &phdr->skip_frame_num);
	//ISP_LOGI("_isp_hdr_ginfo %llu %d\r\n", phdr->level, phdr->skip_frame_num);
	return rtn;
}

static struct isp_io_ctrl_fun _s_isp_io_ctrl_fun_tab[] = {
	{IST_CTRL_SNAPSHOT_NOTICE,           _ispSnapshotNoticeIOCtrl},
	{ISP_CTRL_AE_MEASURE_LUM,            _ispAeMeasureLumIOCtrl},
	{ISP_CTRL_EV,                        _ispEVIOCtrl},
	{ISP_CTRL_FLICKER,                   _ispFlickerIOCtrl},
	{ISP_CTRL_ISO,                       _ispIsoIOCtrl},
	{ISP_CTRL_AE_TOUCH,                  _ispAeTouchIOCtrl},
	{ISP_CTRL_FLASH_NOTICE,              _ispFlashNoticeIOCtrl},
	{ISP_CTRL_VIDEO_MODE,                _ispVideoModeIOCtrl},
	{ISP_CTRL_SCALER_TRIM,               _ispScalerTrimIOCtrl},
	{ISP_CTRL_RANGE_FPS,                 _ispRangeFpsIOCtrl},
	{ISP_CTRL_FACE_AREA,                 _ispFaceAreaIOCtrl},

	{ISP_CTRL_AEAWB_BYPASS,              _ispAeAwbBypassIOCtrl},
	{ISP_CTRL_AWB_MODE,                  _ispAwbModeIOCtrl},

	{ISP_CTRL_AF,                        _ispAfIOCtrl},
	{ISP_CTRL_BURST_NOTICE,              _ispBurstIONotice},
	{ISP_CTRL_SFT_READ,                  _ispSFTIORead},
	{ISP_CTRL_SFT_WRITE,                 _ispSFTIOWrite},
	{ISP_CTRL_SFT_SET_PASS,              _ispSFTIOSetPass},// added for sft
	{ISP_CTRL_SFT_SET_BYPASS,            _ispSFTIOSetBypass},// added for sft
	{ISP_CTRL_SFT_GET_AF_VALUE,          _ispSFTIOGetAfValue},// added for sft
	{ISP_CTRL_AF_MODE,                   _ispAfModeIOCtrl},
	{ISP_CTRL_AF_STOP,                   _ispAfStopIOCtrl},
	{ISP_CTRL_FLASH_CTRL,                _ispOnlineFlashIOCtrl},

	{ISP_CTRL_SCENE_MODE,                _ispSceneModeIOCtrl},
	{ISP_CTRL_SPECIAL_EFFECT,            _ispSpecialEffectIOCtrl},
	{ISP_CTRL_BRIGHTNESS,                _ispBrightnessIOCtrl},
	{ISP_CTRL_CONTRAST,                  _ispContrastIOCtrl},
	{ISP_CTRL_SATURATION,                _ispSaturationIOCtrl},
	{ISP_CTRL_SHARPNESS,                 _ispSharpnessIOCtrl},
	{ISP_CTRL_HDR,                       _ispHdrIOCtrl},

	{ISP_CTRL_PARAM_UPDATE,              _ispParamUpdateIOCtrl},
	{ISP_CTRL_IFX_PARAM_UPDATE,          _ispFixParamUpdateIOCtrl},
	{ISP_CTRL_AE_CTRL,                   _ispAeOnlineIOCtrl}, // for isp tool cali
	{ISP_CTRL_SET_LUM,                   _ispSetLumIOCtrl}, // for tool cali
	{ISP_CTRL_GET_LUM,                   _ispGetLumIOCtrl}, // for tool cali
	{ISP_CTRL_AF_CTRL,                   _ispAfInfoIOCtrl}, // for tool cali
	{ISP_CTRL_SET_AF_POS,                _ispSetAfPosIOCtrl}, // for tool cali
	{ISP_CTRL_GET_AF_POS,                _ispGetAfPosIOCtrl}, // for tool cali
	{ISP_CTRL_GET_AF_MODE,               _ispGetAfModeIOCtrl}, // for tool cali
	{ISP_CTRL_REG_CTRL,                  _ispRegIOCtrl}, // for tool cali
	{ISP_CTRL_AF_END_INFO,               _ispRegIOCtrl}, // for tool cali
	{ISP_CTRL_DENOISE_PARAM_READ,        _ispDenoiseParamRead}, //for tool cali
	{ISP_CTRL_DUMP_REG,   			     _ispDumpReg}, //for tool cali
	{ISP_CTRL_START_3A,                  _ispStart3AIOCtrl},
	{ISP_CTRL_STOP_3A,                   _ispStop3AIOCtrl},

	{ISP_CTRL_AE_FORCE_CTRL,             _ispAeForceIOCtrl}, // for mp tool cali
	{ISP_CTRL_GET_AE_STATE,              _ispGetAeStateIOCtrl}, // for mp tool cali
	{ISP_CTRL_GET_AWB_GAIN,              _isp_get_awb_gain_ioctrl}, // for mp tool cali
	{ISP_CTRL_SET_AE_FPS,                _ispSetAeFpsIOCtrl},  //for LLS feature
	{ISP_CTRL_GET_INFO,                  _ispGetInfoIOCtrl},
	{ISP_CTRL_SET_AE_NIGHT_MODE,         _ispSetAeNightModeIOCtrl},
	{ISP_CTRL_SET_3A_LOCK_UNLOCK,    _ispSetAeAwbLockUnlock}, // AE & AWB Lock or Unlock
	{ISP_CTRL_SET_AE_LOCK_UNLOCK,        _ispSetAeLockUnlock},//AE Lock or Unlock
	{ISP_CTRL_TOOL_SET_SCENE_PARAM,      _ispToolSetSceneParam}, // for tool scene param setting
	{ISP_CTRL_FORCE_AE_QUICK_MODE,      _ispForceAeQuickMode},
	{ISP_CTRL_DENOISE_PARAM_UPDATE,      _ispSensorDenoiseParamUpdate}, //for tool cali
	{ISP_CTRL_SET_AE_EXP_TIME,      _ispSetAeExpTime}, 
	{ISP_CTRL_SET_AE_SENSITIVITY,      _ispSetAeSensitivity}, 
	{ISP_CTRL_EM_GET_PARAM,				 _ispEmGetParam},
	{ISP_CTRL_SET_CAPTURE_RAW_MODE,      _ispSetCaptureRawMode},
	{ISP_CTRL_GET_RGB_GAIN,              _isp_get_rgb_gain_ioctrl},
	//HDR CONTROL
	{ISP_CTRL_START_HDR,			_isp_hdr_start},
	{ISP_CTRL_STOP_HDR,				_isp_hdr_finish},
	{ISP_CTRL_GET_HDR_TIMESTAMP,	_isp_hdr_ginfo},
	{ISP_CTRL_TOOL_SIMULATOR_GET_LSC_ADDR,      _ispGetSimulatorLscAddr},

	{ISP_CTRL_MAX,                       NULL}
};

static io_fun _ispGetIOCtrlFun(enum isp_ctrl_cmd cmd)
{
	io_fun                          io_ctrl = NULL;
	uint32_t                        total_num;
	uint32_t                        i;

	total_num = sizeof(_s_isp_io_ctrl_fun_tab) / sizeof(struct isp_io_ctrl_fun);
	for (i = 0; i < total_num; i++) {
		if (cmd == _s_isp_io_ctrl_fun_tab[i].cmd) {
			io_ctrl = _s_isp_io_ctrl_fun_tab[i].io_ctrl;
			break;
		}
	}

	return io_ctrl;
}

static int32_t _ispTuneIOCtrl(isp_handle isp_handler, enum isp_ctrl_cmd io_cmd, void *param_ptr, int (*call_back)())
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_system               *isp_system_ptr = &handle->system;
	enum isp_ctrl_cmd               cmd = io_cmd&0x7fffffff;
	io_fun                          io_ctrl = NULL;

	isp_system_ptr->isp_callback_bypass = io_cmd&0x80000000;

	io_ctrl=_ispGetIOCtrlFun(cmd);
	if (NULL != io_ctrl) {
		rtn = io_ctrl(handle, param_ptr, call_back);
	} else {
		ISP_LOGD("io ctrl fun is null error");
	}

	return rtn;
}

static int32_t _isp_check_video_param(struct isp_video_start *param_ptr)
{
	int32_t                         rtn = ISP_SUCCESS;

	if ((ISP_ZERO != (param_ptr->size.w&ISP_ONE)) || (ISP_ZERO != (param_ptr->size.h&ISP_ONE))) {
		rtn = ISP_PARAM_ERROR;
		ISP_RETURN_IF_FAIL(rtn, ("input size: w:%d, h:%d error", param_ptr->size.w, param_ptr->size.h));
	}

	return rtn;
}

static int32_t _isp_check_proc_start_param(struct ips_in_param* in_param_ptr)
{
	int32_t                         rtn = ISP_SUCCESS;

	if ((ISP_ZERO != (in_param_ptr->src_frame.img_size.w&ISP_ONE)) || (ISP_ZERO != (in_param_ptr->src_frame.img_size.h&ISP_ONE))) {
		rtn = ISP_PARAM_ERROR;
		ISP_RETURN_IF_FAIL(rtn, ("input size: w:%d, h:%d error", in_param_ptr->src_frame.img_size.w, in_param_ptr->src_frame.img_size.h));
	}

	return rtn;
}

static int32_t _isp_check_proc_next_param(struct ipn_in_param* in_param_ptr)
{
	int32_t                         rtn = ISP_SUCCESS;

	if ((ISP_ZERO != (in_param_ptr->src_slice_height&ISP_ONE)) || (ISP_ZERO != (in_param_ptr->src_avail_height&ISP_ONE))) {
		rtn = ISP_PARAM_ERROR;
		ISP_RETURN_IF_FAIL(rtn, ("input size:src_slice_h:%d,src_avail_h:%d error", in_param_ptr->src_slice_height, in_param_ptr->src_avail_height));
	}

	return rtn;
}

static uint32_t _ispGetIspParamIndex(struct sensor_raw_resolution_info* input_size_trim, struct isp_size* size)
{
	uint32_t                        param_index = 0x01;
	uint32_t                        i;

	for (i = 0x01; i < ISP_INPUT_SIZE_NUM_MAX; i++) {
		if (size->h == input_size_trim[i].height) {
			param_index = i;
			break;
		}
	}

	return param_index;
}

static int32_t _otp_init(isp_handle isp_handler, struct isp_data_info *calibration_param)
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_otp_info             *otp_info = &handle->otp_info;
	struct isp_data_t               lsc = {0};
	struct isp_data_t               awb = {0};
	struct isp_pm_param_data        update_param = {0};

	if (NULL == calibration_param) {
		ISP_LOGE("invalid parameter pointer");
		return ISP_PARAM_NULL;
	}

	if (NULL == calibration_param->data_ptr || 0 == calibration_param->size) {
		ISP_LOGE("calibration param error: %p, %d!", calibration_param->data_ptr,
							calibration_param->size);
		return ISP_SUCCESS;
	}

	rtn = isp_parse_calibration_data(calibration_param, &lsc, &awb);
	if (ISP_SUCCESS != rtn) {
		/*do not return error*/
		ISP_LOGE("isp_parse_calibration_data failed!");
		return ISP_SUCCESS;
	}

	ISP_LOGV("lsc data: (%p, %d), awb data: (%p, %d)", lsc.data_ptr, lsc.size,
						awb.data_ptr, awb.size);

	otp_info->awb.data_ptr = (void *)malloc(awb.size);
	if (NULL != otp_info->awb.data_ptr) {
		otp_info->awb.size = awb.size;
		memcpy(otp_info->awb.data_ptr, awb.data_ptr, otp_info->awb.size);
	}

	otp_info->lsc.data_ptr = (void *)malloc(lsc.size);
	if (NULL != otp_info->lsc.data_ptr) {
		otp_info->lsc.size = lsc.size;
		memcpy(otp_info->lsc.data_ptr, lsc.data_ptr, otp_info->lsc.size);
	}

#ifdef CONFIG_USE_ALC_AWB
	struct isp_cali_lsc_info *cali_lsc_ptr = otp_info->lsc.data_ptr;
	if (cali_lsc_ptr) {
		otp_info->width = cali_lsc_ptr->map[0].width;
		otp_info->height = cali_lsc_ptr->map[0].height;
		otp_info->lsc_random = (isp_u16 *)((isp_u8 *)&cali_lsc_ptr->data_area + cali_lsc_ptr->map[0].offset);
		otp_info->lsc_golden = handle->lsc_golden_data;
	}
#else
	update_param.id        = ISP_BLK_2D_LSC;
	update_param.cmd       = ISP_PM_CMD_UPDATE_LSC_OTP;
	update_param.data_ptr  = lsc.data_ptr;
	update_param.data_size = lsc.size;
	rtn = isp_pm_update(handle->handle_pm, ISP_PM_CMD_UPDATE_LSC_OTP, &update_param, NULL);
	if (ISP_SUCCESS != rtn) {
		/*do not return error*/
		ISP_LOGE("isp_parse_calibration_data failed!");
	}
#endif

	return ISP_SUCCESS;
}

static void _otp_deinit(isp_ctrl_context *handle)
{
	struct isp_otp_info             *otp_info = &handle->otp_info;

	if (NULL != otp_info->awb.data_ptr) {
		free(otp_info->awb.data_ptr);
		otp_info->awb.data_ptr = NULL;
		otp_info->awb.size = 0;
	}

	if (NULL != otp_info->lsc.data_ptr) {
		free(otp_info->lsc.data_ptr);
		otp_info->lsc.data_ptr = NULL;
		otp_info->lsc.size = 0;
	}
}

static int32_t af_posture_activate(void *handle, void *device, uint32_t sensor_handle, uint32_t enable)
{
#if 0
	isp_ctrl_context                *ctrl_context = (isp_ctrl_context*)handle;
	struct isp_system               *isp_system_ptr = &ctrl_context->system;
	sensors_poll_device_1_t         *SensorDevice = (sensors_poll_device_1_t*)device;

	pthread_mutex_lock(&isp_system_ptr->posture_mutex);
	if (ISP_EXIT != isp_system_ptr->posture_status) {
		SensorDevice->activate((struct sensors_poll_device_t *)SensorDevice,
			sensor_handle,enable);
	}
	pthread_mutex_unlock(&isp_system_ptr->posture_mutex);
#endif
	return ISP_SUCCESS;
}

static int32_t af_posture_setDelay(void *handle, void *device, uint32_t sensor_handle, uint64_t delay)
{
#if 0
	isp_ctrl_context                *ctrl_context = (isp_ctrl_context*)handle;
	struct isp_system               *isp_system_ptr = &ctrl_context->system;
	sensors_poll_device_1_t         *SensorDevice = (sensors_poll_device_1_t*)device;

	pthread_mutex_lock(&isp_system_ptr->posture_mutex);
	if (ISP_EXIT != isp_system_ptr->posture_status) {
		SensorDevice->setDelay((struct sensors_poll_device_t *)SensorDevice,
			sensor_handle,delay);
	}
	pthread_mutex_unlock(&isp_system_ptr->posture_mutex);
#endif
	return ISP_SUCCESS;
}

static void *_isp_posture_routine(void *client_data)
{
#if 0
	int                             rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)client_data;
	struct isp_system               *isp_system_ptr = &handle->system;
	struct sensors_module_t         *SensorModule=NULL;
	sensors_poll_device_1_t         *SensorDevice=NULL;
	struct sensor_t const           *list;
	sensors_event_t                 sensor_buffer;

	rtn = hw_get_module(SENSORS_HARDWARE_MODULE_ID,(hw_module_t const**)&SensorModule);
	if (rtn || NULL == SensorModule) {
		isp_system_ptr->posture_status = ISP_EXIT;
		ISP_LOGE("couldn't load %s module,module %p",SENSORS_HARDWARE_MODULE_ID,SensorModule);
		goto EXIT;
	}

	rtn = sensors_open_1(&SensorModule->common, &SensorDevice);
	if( rtn || NULL == SensorDevice) {
		isp_system_ptr->posture_status = ISP_EXIT;
		ISP_LOGE("couldn't load %s module,device %p",SENSORS_HARDWARE_MODULE_ID,SensorDevice);
		goto EXIT;
	}

	if (SensorDevice->common.version == SENSORS_DEVICE_API_VERSION_1_1 ||
		SensorDevice->common.version == SENSORS_DEVICE_API_VERSION_1_2) {
		ISP_LOGE(">>>> WARNING <<< Upgrade sensor HAL to version 1_3");
	}
	// judge whether af alg support posture feature
	if (handle->af_lib_fun->af_posture_support) {
		uint32_t count = 0, i = 0, exist = 0;

		handle->af_lib_fun->af_posture_support(handle,SensorDevice);
		count = SensorModule->get_sensors_list(SensorModule, &list);
		// assume there only exists one sensor from one kind
		for (i = 0 ; i < count; i++) {
			switch(list[i].type) {
			case SENSOR_TYPE_ACCELEROMETER:
				exist = 1;
				rtn = handle->af_lib_fun->af_posture_set_handle(handle,
					ISP_POSTURE_ACCELEROMETER, list[i].handle);
				break;
			case SENSOR_TYPE_MAGNETIC_FIELD:
				exist = 1;
				rtn = handle->af_lib_fun->af_posture_set_handle(handle,
					ISP_POSTURE_MAGNETIC, list[i].handle);
				break;
			case SENSOR_TYPE_ORIENTATION:
				exist = 1;
				rtn = handle->af_lib_fun->af_posture_set_handle(handle,
					ISP_POSTURE_ORIENTATION, list[i].handle);
				break;
			case SENSOR_TYPE_GYROSCOPE:
				exist = 1;
				rtn = handle->af_lib_fun->af_posture_set_handle(handle,
					ISP_POSTURE_GYRO, list[i].handle);
				break;
			default:
				break;
			}
		}
		if (0 == exist) {
			ISP_LOGE("there doesn't exist wanted sensor");
			goto CLOSE;
		}
	} else {
			ISP_LOGE("the alg doesn't support posture feature");
			goto CLOSE;
	}

	while (1) {
		if (ISP_EXIT == isp_system_ptr->posture_status) {
			break;
		}

		memset(&sensor_buffer,0,sizeof(sensors_event_t));
		rtn = SensorDevice->poll((struct sensors_poll_device_t *)SensorDevice,
			&sensor_buffer, 1);

		switch(sensor_buffer.type){
		case SENSOR_TYPE_ACCELEROMETER:
			rtn = handle->af_lib_fun->af_posture_info_update(handle,
				ISP_POSTURE_ACCELEROMETER, (void*)&sensor_buffer.acceleration, NULL);
			break;
		case SENSOR_TYPE_MAGNETIC_FIELD:
			rtn = handle->af_lib_fun->af_posture_info_update(handle,
				ISP_POSTURE_MAGNETIC, (void*)&sensor_buffer.magnetic, (void*)&sensor_buffer.uncalibrated_magnetic);
			break;
		case SENSOR_TYPE_ORIENTATION:
			rtn = handle->af_lib_fun->af_posture_info_update(handle,
				ISP_POSTURE_ORIENTATION, (void*)&sensor_buffer.orientation, NULL);
			break;
		case SENSOR_TYPE_GYROSCOPE:
			rtn = handle->af_lib_fun->af_posture_info_update(handle,
				ISP_POSTURE_GYRO, (void*)&sensor_buffer.gyro, (void*)&sensor_buffer.uncalibrated_gyro);
			break;
		default:
			break;
		}
	}
CLOSE:
	pthread_mutex_lock(&isp_system_ptr->posture_mutex);
	isp_system_ptr->posture_status = ISP_EXIT;
	SensorDevice->common.close((struct hw_device_t*)SensorDevice);
	pthread_mutex_unlock(&isp_system_ptr->posture_mutex);
EXIT:
	isp_system_ptr->posture_status = ISP_EXIT;
#endif
	return NULL;
}

static void* _isp_monitor_routine(void *client_data)
{
	int32_t                         rtn = ISP_SUCCESS;

	isp_ctrl_context                *handle = (isp_ctrl_context*)client_data;
	struct isp_system               *isp_system_ptr = &handle->system;
	struct isp_soft_ae_cfg          *ae_cfg = NULL;
	struct isp_irq                  evt;
	uint32_t                        bypass = 0;
/*
#ifdef AF_REALTIME_CTRL
		int pipe_fd = 0;
		pid_t tid = 0;

		pipe_fd = open("/dev/isp_pipe/pipe_1", O_RDWR);
		if (pipe_fd < 0) {
			ISP_LOGE("open pipe error!!");
		} else {
			tid = gettid();
			write(pipe_fd, &tid, sizeof(pid_t));
			close(pipe_fd);
		       ISP_LOGI("[3A Porting]_isp_monitor_routine:open pipe succeed!!");
		}
#endif
*/
	CMR_MSG_INIT(msg);

	while (1) {
		if (ISP_SUCCESS != isp_dev_get_irq(handle->handle_device, (uint32_t*)&evt)) {
			rtn = -1;
			ISP_LOGE("wait int error: %x %x %x %x %x",
					evt.irq_val0, evt.irq_val1, evt.irq_val2,
					evt.irq_val3, evt.reserved);
			break;
		}

		if (ISP_EXIT == isp_system_ptr->monitor_status) {
			break;
		}

		if (ISP_ZERO != (evt.reserved & ISP_INT_EVT_STOP)) {
			ISP_LOGI("monitor thread stopped");
			break;
		}

/* [Vendor_24/40] */
#ifdef AF_REALTIME_CTRL
			ISP_LOGE("[3A Porting] monitor routine-evt.irq_val1= %d",evt.irq_val1);
		if (ISP_ZERO != (evt.irq_val1 & ISP_INT_EVT_AFM_RGB_DONE)) {
			msg.msg_type = ISP_PROC_AF_CALC;
			ISP_LOGE("[3A Porting] Inside monitor routine-ISP_PROC_AF_CALC");
			msg.sync_flag = CMR_MSG_SYNC_NONE;
			rtn = cmr_thread_msg_send(isp_system_ptr->thread_af_proc, &msg);
			ISP_LOGI("[3A Porting]_isp_monitor_routine: Send ISP_PROC_AF_CALC!!");
		}
#endif

		if ((ISP_ZERO != (evt.irq_val1 & ISP_INT_EVT_AFM_Y_DONE))
			|| (ISP_ZERO != (evt.irq_val2 & ISP_INT_EVT_AFM_Y_WIN0))) {
			msg.msg_type = ISP_CTRL_EVT_AF;
			rtn = cmr_thread_msg_send(isp_system_ptr->thread_ctrl, &msg);
		}

		if (ISP_ZERO != (evt.irq_val1 & ISP_INT_EVT_AWBM_DONE)) {
			msg.msg_type = ISP_CTRL_EVT_AWB;
			rtn = cmr_thread_msg_send(isp_system_ptr->thread_ctrl, &msg);
		}

		if (ISP_ZERO != (evt.irq_val1 & ISP_INT_EVT_AEM_DONE)) {
			isp_u_raw_aem_statistics(handle->handle_device, handle->aem_statistic.r_info,
									handle->aem_statistic.g_info, handle->aem_statistic.b_info);
			ISP_LOGI("ISP_INT_EVT_AEM_DONE");
			//handle->system_time_ae = evt.time;
			msg.msg_type = ISP_CTRL_EVT_AE;
			rtn = cmr_thread_msg_send(isp_system_ptr->thread_ctrl, &msg);
		}

		if (ISP_ZERO != (evt.irq_val1 & ISP_INT_EVT_AEM2_DONE)) {
			ISP_LOGI("ISP_INT_EVT_AEM2_DONE");
			msg.msg_type = ISP_CTRL_EVT_AEM2;
			rtn = cmr_thread_msg_send(isp_system_ptr->thread_ctrl, &msg);
		}

		if (ISP_ZERO != (evt.irq_val1 & ISP_INT_EVT_BINNING_DONE)) {
			ISP_LOGI("ISP_INT_EVT_BINNING_DONE");
			msg.msg_type = ISP_CTRL_EVT_BINNING_DONE;
			rtn = cmr_thread_msg_send(isp_system_ptr->thread_ctrl, &msg);
		}
/* [Vendor_25/40] */
#ifndef AF_REALTIME_CTRL
		if (ISP_ZERO != (evt.irq_val1 & ISP_INT_EVT_AFM_RGB_DONE)) {
			msg.msg_type = ISP_CTRL_EVT_AF;
			rtn = cmr_thread_msg_send(isp_system_ptr->thread_ctrl, &msg);
			ISP_LOGI("[3A Porting]_isp_monitor_routine: Send ISP_CTRL_EVT_AF!!");
		}
#endif

		if (ISP_ZERO != (evt.irq_val1 & ISP_INT_EVT_DCAM_SOF)) {
			if (handle->gamma_sof_cnt_eb) {
				handle->gamma_sof_cnt++;
				if (handle->gamma_sof_cnt >= 2) {
					handle->update_gamma_eb = 1;
				}
			}
			handle->system_time_ae = evt.time;
			msg.msg_type = ISP_CTRL_EVT_SOF;
			rtn = cmr_thread_msg_send(isp_system_ptr->thread_ctrl, &msg);
		}

		if (ISP_ZERO != (evt.irq_val1 & ISP_INT_EVT_STORE_DONE)) {
			msg.msg_type = ISP_CTRL_EVT_TX;
 			rtn = cmr_thread_msg_send(isp_system_ptr->thread_ctrl, &msg);
		}

		if (ISP_ZERO != (evt.irq_val1 & ISP_INT_EVT_AFL_DONE)
			|| ISP_ZERO != (evt.irq_val3 & ISP_INT_EVT_AFL_NEW_DDR_DONE)) {
		#ifdef CONFIG_CAMERA_AFL_AUTO_DETECTION
			msg.msg_type = ISP_PROC_AFL_DONE;
			rtn = cmr_thread_msg_send(isp_system_ptr->thread_afl_proc, &msg);
		#endif
		}
	}

	if (rtn) {
		isp_system_ptr->monitor_status = ISP_EXIT;
	}

	return NULL;
}

static int _isp_init(isp_handle isp_handler, struct isp_init_param *ptr)
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	pthread_attr_t                  attr;
	struct sensor_raw_info          *sensor_raw_info_ptr = (struct sensor_raw_info*)ptr->setting_param_ptr;
	struct sensor_version_info      *version_info = PNULL;
	struct isp_pm_init_input        input;
	struct sensor_libuse_info       *libuse_info;
	uint32_t                        i;

	handle->sn_raw_info        = sensor_raw_info_ptr;
	memcpy((void *)handle->isp_init_data,(void *)ptr->mode_ptr,ISP_MODE_NUM_MAX*sizeof(struct isp_data_info));

	input.isp_ctrl_cxt_handle = handle;
	input.num = MAX_MODE_NUM;
	version_info =  (struct sensor_version_info *)sensor_raw_info_ptr->version_info;
	for (i = 0; i < MAX_MODE_NUM; i++) {
		if (ISP_PARAM_VERSION < version_info->version_id) {
			input.tuning_data[i].data_ptr = ptr->mode_ptr[i].data_ptr;
			input.tuning_data[i].size     = ptr->mode_ptr[i].size;
		} else {
			input.tuning_data[i].data_ptr = sensor_raw_info_ptr->mode_ptr[i].addr;
			input.tuning_data[i].size     = sensor_raw_info_ptr->mode_ptr[i].len;
		}
	}
	struct sensor_mode_fix_param *common_mode_ptr =  &sensor_raw_info_ptr->fix_ptr[0]->mode;
	struct isp_fix_param_mode_info *common_isp_fix_param = (struct isp_fix_param_mode_info *)common_mode_ptr->mode_info;
	strcpy((isp_s8 *)(input.sensor_name), (isp_s8 *)(common_isp_fix_param->sensor_name));
	handle->handle_pm = isp_pm_init(&input, NULL);

	handle->src.w           = ptr->size.w;
	handle->src.h           = ptr->size.h;
	handle->camera_id       = ptr->camera_id;
	handle->lsc_golden_data = ptr->sensor_lsc_golden_data;

	struct isp_pm_ioctl_input       pm_input;
	struct isp_pm_ioctl_output      pm_output;
	struct isp_pm_param_data        param_data;
	struct isp_dev_rgb_gain_info_v1 *gain_info;
	BLOCK_PARAM_CFG(pm_input, param_data, ISP_PM_BLK_GBL_GAIN, ISP_BLK_RGB_GAIN_V1, NULL, 0);
	isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, &pm_input, &pm_output);
	gain_info = (struct isp_dev_rgb_gain_info_v1 *)pm_output.param_data->data_ptr;
	handle->global_ob_gain = gain_info->global_gain;

	/* init system */
	handle->system.caller_id = ptr->oem_handle;
	handle->system.callback  = ptr->ctrl_callback;
	handle->system.ops       = ptr->ops;

	/* init sensor param */
	handle->ioctrl_ptr    = sensor_raw_info_ptr->ioctrl_ptr;
	handle->image_pattern = sensor_raw_info_ptr->resolution_info_ptr->image_pattern;
	memcpy(handle->input_size_trim,
		sensor_raw_info_ptr->resolution_info_ptr->tab,
		ISP_INPUT_SIZE_NUM_MAX*sizeof(struct sensor_raw_resolution_info));
	handle->param_index = _ispGetIspParamIndex(handle->input_size_trim, &ptr->size);

	/*for lib switch */
	libuse_info = sensor_raw_info_ptr->libuse_info;
	aaa_lib_init(handle, libuse_info);

	ISP_LOGV("param_index, 0x%x", handle->param_index);
	/* todo: base on param_index to get sensor line_time/frame_line */

	/*Notice: otp_init must be called before _ispAlgInit*/
	rtn = _otp_init(handle, &ptr->calibration_param);
	ISP_TRACE_IF_FAIL(rtn, ("_otp_init error"));

	/* init algorithm */
	rtn = _ispAlgInit(handle);
	ISP_TRACE_IF_FAIL(rtn, ("_ispAlgInit error"));

	/*create posture sensor thread*/
	pthread_mutex_init(&handle->system.posture_mutex, NULL);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	rtn = pthread_create(&handle->system.posture_thread, &attr, _isp_posture_routine, handle);
	pthread_attr_destroy(&attr);
	ISP_RETURN_IF_FAIL(rtn, ("create posture thread error"));

	/*create monitor thread*/
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	rtn = pthread_create(&handle->system.monitor_thread, &attr, _isp_monitor_routine, handle);
	pthread_attr_destroy(&attr);
	ISP_RETURN_IF_FAIL(rtn, ("create monitor thread error"));

	return rtn;
}

static int _isp_deinit(isp_handle isp_handler)
{
	int32_t                         rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;

	rtn = _ispUncfg(handle->handle_device);
	ISP_TRACE_IF_FAIL(rtn, ("isp uncfg error"));

	rtn = isp_dev_stop(handle->handle_device);
	ISP_TRACE_IF_FAIL(rtn, ("isp_dev_stop error"));

	return rtn;
}

static int _isp_wr_log(struct isp_reg_bits *param_t, char *log_name, int num)
{
	int                             fd = -1;
	int                             ret = ISP_SUCCESS;
	int                             i;
	unsigned char                   buff[34];
	unsigned long                   ADDR_OFF = 0x60a00000;
	struct isp_reg_bits             *param = param_t;

	if ((NULL == param) || (NULL == log_name)) {
		ISP_LOGE("error: _isp_wr_log---param");
		return -1;
	}

	fd = open(log_name, O_RDWR|O_CREAT|O_TRUNC, 0666);

	if (fd < 0) {
		ISP_LOGE("error: _isp_wr_log---open");
		return -1;
	}

	for (i = 0; i < num; i++)
	{
		memset(buff, '\n', sizeof(buff));

		sprintf(buff, "  0x%08x           0x%08x", param->reg_addr + ADDR_OFF, param->reg_value);
		buff[33] = '\n';

		ret = write(fd, buff, sizeof(buff));
		if (ret < 0) {
			ISP_LOGE("error: _isp_wr_log---write");
			close(fd);
			return -1;
		}
		param += 1;
	}

	close(fd);

	return ret;
}

static int _isp_reg_read(isp_ctrl_context *handle, char *log_name)
{
	int                             ret = ISP_SUCCESS;
	struct isp_file                 *file = NULL;

	if ((NULL == handle) || (NULL == log_name)) {
		ISP_LOGE("error: _isp_reg_read---handle/name");
		return -1;
	}

	struct isp_reg_bits *param = (struct isp_reg_bits*)malloc(ISP_REG_NUM * sizeof(struct isp_reg_bits));
	if (NULL == param) {
		ISP_LOGE("error: _isp_reg_read---param");
		return -2;
	}

	file = (struct isp_file*)(handle->handle_device);
	if (NULL == file) {
		ISP_LOGE("error: _isp_reg_read---file");
		free(param);
		return -3;
	}
	ret = ioctl(file->fd, ISP_REG_READ, param);
	if (-1 == ret) {
		ISP_LOGE("error: _isp_reg_read---ioctl");
		free(param);
		return -4;
	}

	ret = _isp_wr_log(param, log_name, ISP_REG_NUM);
	if (-1 == ret) {
		ISP_LOGE("error: _isp_reg_read---_isp_wr_log");
		free(param);
		return -5;
	}

	free(param);

	return ret;
}

static int _is_isp_reg_log(isp_ctrl_context *handle)
{
#ifdef WIN32
	return ISP_SUCCESS;
#else
	int                             rtn = ISP_SUCCESS;
	char                            value[30];

	if (NULL == handle){
		ISP_LOGE(" _is_isp_reg_log param error ");
		return -1;
	}

	property_get(ISP_SAVE_REG_LOG, value, "/dev/null");

	if (strcmp(value, "/dev/null")) {
		rtn = _isp_reg_read(handle, value);
		if (rtn < 0) {
			ISP_LOGE(" _isp_reg_read error rtn = %d", rtn);
			return -1;
		}
	}

	ISP_LOGD("Like_isp:_is_isp_reg_log: %s", value);
	return rtn;
#endif
}

static int _is_need_soft_ae(isp_handle isp_handler, uint32_t width, uint32_t height)
{
	int                             rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;

	/*AEM block num 32 x 32,  if each block's  pixel more than 8224, the AEM statistics maybe overflow*/
	if ((width >> 5) * (height >> 5) > ISP_AEM_MAX_BLOCK_SIZE) {
		handle->need_soft_ae = 1;
	} else {
		handle->need_soft_ae = 0;
	}
	ISP_LOGI("need_soft_ae %d", handle->need_soft_ae);

	return rtn;
}

static int _isp_video_start(isp_handle isp_handler, struct isp_video_start *param_ptr)
{
	int                             rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_interface_param      *interface_ptr = &handle->interface_param;
	struct isp_interface_param_v1   *interface_ptr_v1 = &handle->interface_param_v1;
	struct isp_file                 *file = (struct isp_file*)handle->handle_device;
	struct isp_size                 org_size;
	int mode = 0, dv_mode = 0;
	isp_uint                        type = 0;

	ISP_LOGI("src.w: %d, src.h: %d", handle->src.w, handle->src.h);
	ISP_LOGI("size.w: %d, size.h: %d, format: 0x%x", param_ptr->size.w, param_ptr->size.h, param_ptr->format);

	org_size.w    = handle->src.w;
	org_size.h    = handle->src.h;
	handle->src.w = param_ptr->size.w;
	handle->src.h = param_ptr->size.h;

	handle->isp_lsc_len        = param_ptr->lsc_buf_size;
	handle->isp_lsc_physaddr   = param_ptr->lsc_phys_addr;
	handle->isp_lsc_virtaddr   = param_ptr->lsc_virt_addr;

	/*init ae buffer queue*/

	if (handle->isp_bq_alloc_flag == 0) {
		handle->buffer_client_data = param_ptr->buffer_client_data;
		handle->cb_of_malloc = param_ptr->cb_of_malloc;
		handle->cb_of_free = param_ptr->cb_of_free;

		_is_need_soft_ae(handle, handle->src.w, handle->src.h);
		rtn = _ispSoft_ae_init(handle);
		ISP_TRACE_IF_FAIL(rtn, ("_ispSoft_ae_init error"));

		if (handle->need_soft_ae) {
			handle->isp_bq_mem_num = ISP_BQ_BIN_CNT+1;
			handle->isp_bq_mem_size = ISP_AEM_BINNING_SIZE;
			type = CAMERA_ISP_BINGING4AWB_FLAG;
		} else {
			handle->isp_bq_mem_num = ISP_BQ_AEM_CNT;
			handle->isp_bq_mem_size = ISP_RAWAEM_BUF_SIZE;
			type = CAMERA_ISP_RAWAEM_FLAG;
		}

/* [Vendor_26/40] */
		handle->isp_af_bq_mem_num = ISP_BQ_AFM_CNT;
		handle->isp_af_bq_mem_size = ISP_RAWAFM_BUF_SIZE;

		if (handle->cb_of_malloc) {
			isp_cb_of_malloc cb_malloc = handle->cb_of_malloc;
			cb_malloc(type, &handle->isp_bq_mem_size, &handle->isp_bq_mem_num,
					(isp_uint *)handle->isp_bq_k_addr_array,(isp_uint *)handle->isp_bq_u_addr_array, handle->buffer_client_data);

/* [Vendor_27/40] */
			cb_malloc(CAMERA_ISP_RAWAFM_FLAG, &handle->isp_af_bq_mem_size, &handle->isp_af_bq_mem_num,
					(isp_uint *)handle->isp_af_bq_k_addr_array,(isp_uint *)handle->isp_af_bq_u_addr_array, handle->buffer_client_data);
		} else {
			ISP_LOGE("failed to malloc isp_bq buffer");
			return ISP_PARAM_NULL;
		}

		handle->isp_bq_alloc_flag = 1;
	}

	handle->isp_anti_flicker_len = param_ptr->anti_flicker_buf_size;
	handle->isp_anti_flicker_physaddr = param_ptr->anti_flicker_phys_addr;
	handle->isp_anti_flicker_virtaddr = param_ptr->anti_flicker_virt_addr;

	interface_ptr_v1->data.work_mode      = ISP_CONTINUE_MODE;
	interface_ptr_v1->data.input          = ISP_CAP_MODE;
	interface_ptr_v1->data.input_format   = param_ptr->format;
	interface_ptr_v1->data.format_pattern = handle->image_pattern;
	interface_ptr_v1->data.input_size.w   = param_ptr->size.w;
	interface_ptr_v1->data.input_size.h   = param_ptr->size.h;
	interface_ptr_v1->data.output_format  = ISP_DATA_UYVY;
	interface_ptr_v1->data.output         = ISP_DCAM_MODE;
	interface_ptr_v1->data.slice_height   = param_ptr->size.h;
	
	interface_ptr_v1->slice.cur_slice_window.start_row = 0;
	interface_ptr_v1->slice.cur_slice_window.start_col = 0;
	interface_ptr_v1->slice.cur_slice_window.end_row = interface_ptr_v1->data.input_size.h - 1;
	interface_ptr_v1->slice.cur_slice_window.end_col = interface_ptr_v1->data.input_size.w - 1;

	rtn = _ispSetInterfaceParam(handle);
	ISP_RETURN_IF_FAIL(rtn, ("set param error"));

	switch (param_ptr->work_mode) {
	case 0: /*preview*/
		rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_MODEID_BY_RESOLUTION, param_ptr, &mode);
		break;
	case 1: /*capture*/
		rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_MODEID_BY_RESOLUTION, param_ptr, &mode);
		break;
#ifdef CONFIG_CAMERA_AFL_AUTO_DETECTION
	case 2:
		mode = ISP_MODE_ID_VIDEO_0;
		break;
#endif
	default:
		mode = ISP_MODE_ID_PRV_0;
		break;
	}

	if( SENSOR_MULTI_MODE_FLAG != handle->multi_nr_flag) {
		if ((mode != handle->isp_mode) && (org_size.w != handle->src.w)) {
			handle->isp_mode = mode;
			rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_MODE, &handle->isp_mode, NULL);
		}
	} else {
		if (0 != param_ptr->dv_mode) {
			rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_DV_MODEID_BY_RESOLUTION, param_ptr, &dv_mode);
			handle->mode_flag = dv_mode;
		} else {
			handle->mode_flag = mode;
		}
		if (handle->mode_flag != handle->isp_mode) {
			handle->isp_mode = handle->mode_flag;
			rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_MODE, &handle->isp_mode, NULL);
		}
	}
	ISP_LOGI("handle->mode_flag: %d", handle->mode_flag);

	/* isp param index */
	interface_ptr->src.w = param_ptr->size.w;
	interface_ptr->src.h = param_ptr->size.h;
	handle->param_index  = _ispGetIspParamIndex(handle->input_size_trim, &param_ptr->size);
	/* todo: base on param_index to get sensor line_time/frame_line */

	rtn = _ispChangeAwbGain(handle);
	ISP_RETURN_IF_FAIL(rtn, ("isp change awb gain error"));

	rtn = _ispChangeSmartParam(handle);
	ISP_RETURN_IF_FAIL(rtn, ("isp smart param calc error"));

	rtn = _ispTransBuffAddr(handle);
	ISP_RETURN_IF_FAIL(rtn, ("isp trans buff error"));

	//smart must be added here first
	rtn =  _isp_change_lsc_param(handle, 0);
	ISP_RETURN_IF_FAIL(rtn, ("isp change lsc gain error"));

	rtn = _ispCfg(handle);
	ISP_RETURN_IF_FAIL(rtn, ("isp cfg error"));

	if (ISP_VIDEO_MODE_CONTINUE == param_ptr->mode) {
		rtn = isp_dev_enable_irq(handle->handle_device, ISP_INT_VIDEO_MODE);
		ISP_RETURN_IF_FAIL(rtn, ("isp cfg int error"));
	}

	rtn = handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_SET_WORK_MODE, &param_ptr->work_mode, NULL);
	ISP_RETURN_IF_FAIL(rtn, ("awb set_work_mode error"));

	rtn = ae_set_work_mode(handle, mode, 1, param_ptr);

	ISP_RETURN_IF_FAIL(rtn, ("ae cfg error"));

	rtn = _ispStart(handle);
	ISP_RETURN_IF_FAIL(rtn, ("video isp start error"));

	if(NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_set_af_param){
		handle->af_lib_fun->af_ioctrl_set_af_param(handle);
	}

	ISP_LOGI("af info start");

	if (NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_set_isp_start_info) {
		handle->af_lib_fun->af_ioctrl_set_isp_start_info(handle, param_ptr);
	}

	ISP_LOGI("af info end");

	if (ISP_VIDEO_MODE_SINGLE == param_ptr->mode) {
		 _is_isp_reg_log(handle);
	}

	return rtn;
}

static int _isp_video_stop(isp_handle isp_handler)
{
	int                              rtn = ISP_SUCCESS;
	isp_ctrl_context                 *handle = (isp_ctrl_context*)isp_handler;
#ifdef CONFIG_CAMERA_ISP_AE_VERSION_V1
	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_VIDEO_STOP, NULL, NULL);
#endif
	if (NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_set_isp_stop_info) {
		rtn = handle->af_lib_fun->af_ioctrl_set_isp_stop_info(handle);
		if (rtn) {
			ISP_LOGE("af_ioctrl_set_isp_stop_info failed %d", rtn);
		}
	}

	rtn = isp_dev_enable_irq(handle->handle_device, ISP_INT_CLEAR_MODE);
	ISP_RETURN_IF_FAIL(rtn, ("isp_dev_enable_irq error"));

	rtn = _ispUncfg(handle->handle_device);
	ISP_RETURN_IF_FAIL(rtn, ("isp cfg error"));

	return rtn;
}

static int _isp_proc_start(isp_handle isp_handler, struct ips_in_param* param_ptr)
{
	int                             rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_interface_param      *interface_ptr = &handle->interface_param;
	struct isp_interface_param_v1   *interface_ptr_v1 = &handle->interface_param_v1;
	struct isp_file                 *file = (struct isp_file*)handle->handle_device;
	struct isp_size                 org_size;

	org_size.w    = handle->src.w;
	org_size.h    = handle->src.h;

	handle->src.w = param_ptr->src_frame.img_size.w;
	handle->src.h = param_ptr->src_frame.img_size.h;

	interface_ptr_v1->slice.cur_slice_num.w = 0;
	interface_ptr_v1->data.work_mode        = ISP_SINGLE_MODE;
	interface_ptr_v1->data.input            = ISP_EMC_MODE;
	interface_ptr_v1->data.input_format     = param_ptr->src_frame.img_fmt;

	if (INVALID_FORMAT_PATTERN == param_ptr->src_frame.format_pattern) {
		interface_ptr_v1->data.format_pattern = handle->image_pattern;
	} else {
		interface_ptr_v1->data.format_pattern = param_ptr->src_frame.format_pattern;
	}
	interface_ptr_v1->data.input_size.w     = param_ptr->src_frame.img_size.w;
	interface_ptr_v1->data.input_size.h     = param_ptr->src_frame.img_size.h;
	interface_ptr_v1->data.input_addr.chn0  = param_ptr->src_frame.img_addr_phy.chn0;
	interface_ptr_v1->data.input_addr.chn1  = param_ptr->src_frame.img_addr_phy.chn1;
	interface_ptr_v1->data.input_addr.chn2  = param_ptr->src_frame.img_addr_phy.chn2;
	interface_ptr_v1->data.slice_width     = param_ptr->src_slice_width;
	
	interface_ptr_v1->slice.cur_slice_window.start_row = 0;
	interface_ptr_v1->slice.cur_slice_window.start_col = 0;
	interface_ptr_v1->slice.cur_slice_window.end_row = interface_ptr_v1->data.input_size.h - 1;
	interface_ptr_v1->slice.cur_slice_window.end_col = interface_ptr_v1->data.slice_width+ ISP_SLICE_WIDTH_OVERLAP - 1;
	

	interface_ptr_v1->data.output_format    = param_ptr->dst_frame.img_fmt;
	interface_ptr_v1->data.output           = ISP_EMC_MODE;
	interface_ptr_v1->data.output_addr.chn0 = param_ptr->dst_frame.img_addr_phy.chn0;
	interface_ptr_v1->data.output_addr.chn1 = param_ptr->dst_frame.img_addr_phy.chn1;
	interface_ptr_v1->data.output_addr.chn2 = param_ptr->dst_frame.img_addr_phy.chn2;

	rtn = _ispSetInterfaceParam(handle);
	ISP_RETURN_IF_FAIL(rtn, ("set param error"));

	if( SENSOR_MULTI_MODE_FLAG == handle->multi_nr_flag) {
		if(handle->isp_mode !=  (int32_t)handle->mode_flag) {
			handle->isp_mode = (int32_t)handle->mode_flag;
			rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_MODE, &handle->isp_mode, NULL);
			ISP_RETURN_IF_FAIL(rtn, ("isp multi nr set isp mode error"));
			rtn = _ispChangeSmartParam(handle);
			ISP_RETURN_IF_FAIL(rtn, ("isp multi nr smart change param error"));
			rtn = _ispSetTuneParam(handle);
			ISP_RETURN_IF_FAIL(rtn, ("isp multi nr set tune param error"));
		}
	}
	else {
	if (org_size.w != handle->src.w) {
		handle->isp_mode = ISP_MODE_ID_CAP_1;
		rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_SET_MODE, &handle->isp_mode, NULL);
		}
	}

	/* isp param index */
	//interface_ptr->src.w = param_ptr->src_frame.img_size.w;
	//interface_ptr->src.h = param_ptr->src_frame.img_size.h;
	handle->param_index  = _ispGetIspParamIndex(handle->input_size_trim, &param_ptr->src_frame.img_size);
	ISP_LOGI("proc param index :0x%x", handle->param_index);
	/* todo: base on param_index to get sensor line_time/frame_line */

	rtn = isp_set_store_proc_param_v1(handle);
	ISP_TRACE_IF_FAIL(rtn, ("isp_set_store_param_v1 error"));

	rtn = _ispChangeAwbGain(handle);
	ISP_RETURN_IF_FAIL(rtn, ("isp change awb gain error"));

	rtn = _ispChangeSmartParam(handle);
	ISP_RETURN_IF_FAIL(rtn, ("isp smart param calc error"));

	rtn = _ispTransBuffAddr(handle);
	ISP_RETURN_IF_FAIL(rtn, ("isp trans buff error"));

	rtn = _ispCfg(handle);
	ISP_RETURN_IF_FAIL(rtn, ("isp cfg error"));

	rtn = isp_dev_enable_irq(handle->handle_device, ISP_INT_CAPTURE_MODE);
	ISP_RETURN_IF_FAIL(rtn, ("isp_dev_enable_irq error"));


	rtn = _ispStart(handle);
	ISP_RETURN_IF_FAIL(rtn, ("proc isp start error"));

	return rtn;
}

static int _isp_proc_next(isp_handle isp_handler, struct ipn_in_param *in_ptr)
{
	int                             rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_interface_param_v1      *interface_ptr_v1 = &handle->interface_param_v1;
	uint32_t img_width = interface_ptr_v1->data.input_size.w;

	interface_ptr_v1->slice.cur_slice_num.w = ISP_ONE;
	interface_ptr_v1->data.slice_width     = in_ptr->src_slice_width;

	interface_ptr_v1->slice.cur_slice_window.start_row = 0;
	interface_ptr_v1->slice.cur_slice_window.start_col = img_width - interface_ptr_v1->data.slice_width - ISP_SLICE_WIDTH_OVERLAP;
	interface_ptr_v1->slice.cur_slice_window.end_row = interface_ptr_v1->data.input_size.h - 1;
	interface_ptr_v1->slice.cur_slice_window.end_col = interface_ptr_v1->data.input_size.w - 1;

	rtn = _ispSetInterfaceParam(handle);
	ISP_RETURN_IF_FAIL(rtn, ("set param error"));

	rtn = isp_set_store_next_param_v1(handle);
	ISP_TRACE_IF_FAIL(rtn, ("isp_set_store_param_v1 error"));

	rtn = _ispTransBuffAddr(handle);
	ISP_RETURN_IF_FAIL(rtn, ("isp trans buff error"));

	rtn = _ispCfg(handle);
	ISP_RETURN_IF_FAIL(rtn, ("isp cfg error"));

	rtn = isp_dev_enable_irq(handle->handle_device, ISP_INT_CAPTURE_MODE);
	ISP_RETURN_IF_FAIL(rtn, ("isp_dev_enable_irq error"));

	rtn = _ispStart(handle);
	ISP_RETURN_IF_FAIL(rtn, ("proc next isp start error"));

	return rtn;
}

static cmr_int  _isp_ctrl_routine(struct cmr_msg *message, void *client_data)
{
	int                             rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)client_data;
	struct isp_system               *isp_system_ptr = &handle->system;
	uint32_t                        evt = (uint32_t)(message->msg_type&ISP_CTRL_EVT_MASK);
	uint32_t                        sub_type = message->sub_msg_type;
	void                            *param_ptr = (void*)message->data;

	CMR_MSG_INIT(msg);

	switch (evt) {
	case ISP_CTRL_EVT_INIT:
		rtn = isp_dev_open(&handle->handle_device);
		if (ISP_SUCCESS == rtn) {
			rtn = _isp_init(handle, (struct isp_init_param*)param_ptr);
		}
		break;

	case ISP_CTRL_EVT_DEINIT:
		rtn = _isp_deinit(handle);
		if (ISP_SUCCESS == rtn) {
			rtn = isp_dev_close(handle->handle_device);
			handle->handle_device = PNULL;
		}
		/*destroy monitor and posture thread*/
		isp_system_ptr->monitor_status = ISP_EXIT;
		isp_system_ptr->posture_status = ISP_EXIT;
		break;

	case ISP_CTRL_EVT_MONITOR_STOP:
		break;

	case ISP_CTRL_EVT_CONTINUE: {
		rtn = _isp_video_start(handle, (struct isp_video_start*)param_ptr);
		break;
	}

	case ISP_CTRL_EVT_CONTINUE_STOP:
		rtn = _isp_video_stop(handle);
		if (ISP_SUCCESS == rtn) {
			msg.msg_type = ISP_PROC_EVT_STOP_HANDLER;
			msg.sync_flag = CMR_MSG_SYNC_PROCESSED;
			rtn = cmr_thread_msg_send(isp_system_ptr->thread_proc, &msg);
		}
		break;

	case ISP_CTRL_EVT_SIGNAL: {
		rtn = _isp_proc_start(handle, (struct ips_in_param*)param_ptr);
		break;
	}

	case ISP_CTRL_EVT_SIGNAL_NEXT: {
		rtn = _isp_proc_next(handle, (struct ipn_in_param*)param_ptr);
		break;
	}

	case ISP_CTRL_EVT_IOCTRL: {
		rtn = _ispTuneIOCtrl(handle, sub_type, param_ptr,NULL);
		break;
	}

	case ISP_CTRL_EVT_TX:
		rtn = _ispProcessEndHandle(handle);
		break;

	case ISP_CTRL_EVT_SOF:
		ISP_LOGI("SOF");
#ifdef CONFIG_CAMERA_ISP_AE_VERSION_V1
		//struct timespec ts;
		//clock_gettime(CLOCK_REALTIME, &ts);
		//ISP_LOGI("DYT_ctrl sof: %lds.%ldns", ts.tv_sec, ts.tv_nsec);
		//rtn = _ispSetTuneParam(handle, message->timestamp);

		if (0 < handle->aem_irq){
			msg.msg_type = ISP_PROC_EVT_AE;
			//msg.timestamp = message->timestamp;
			rtn = cmr_thread_msg_send(isp_system_ptr->thread_proc, &msg);
		}else{
			;
		}
#endif
		rtn = _ispSetTuneParam(handle);
		break;

	case ISP_CTRL_EVT_AE: {
#ifdef CONFIG_CAMERA_ISP_AE_VERSION_V1
		handle->aem_irq++;
#else
		msg.msg_type = ISP_PROC_EVT_AE;
		rtn = cmr_thread_msg_send(isp_system_ptr->thread_proc, &msg);
#endif
		break;
	}

	case ISP_CTRL_EVT_AEM2:
	{
		if (handle->ae_lib_fun->product_id) {
			struct isp_ae_statistic_info *ae_y_stat_ptr = NULL;
			struct isp_pm_param_data param_data;
			struct isp_pm_ioctl_input input = {NULL, 0};
			struct isp_pm_ioctl_output output = {NULL, 0};

			BLOCK_PARAM_CFG(input,
					param_data,
					ISP_PM_BLK_YIQ_AE_STATISTIC, ISP_BLK_YIQ_AEM,
					NULL,
					0);
			rtn = isp_pm_ioctl(handle->handle_pm,
				ISP_PM_CMD_GET_SINGLE_SETTING,
				(void*)&input,
				(void*)&output);
			if (ISP_SUCCESS == rtn) {
				ae_y_stat_ptr = output.param_data->data_ptr;
				rtn = isp_u_yiq_aem_statistics(handle->handle_device, ae_y_stat_ptr->y);
				if (ISP_SUCCESS == rtn) {
					msg.msg_type = ISP_PROC_EVT_AEM2;
					rtn = cmr_thread_msg_send(isp_system_ptr->thread_proc, &msg);
				}
			}
		}
		break;
	}

	case ISP_CTRL_EVT_BINNING_DONE: {
		struct isp_soft_ae_cfg *ae_cfg_param = NULL;
		ae_cfg_param = (struct isp_soft_ae_cfg *)handle->handle_soft_ae;
		if (1 == ae_cfg_param->bypass_status)
			ae_cfg_param->skip_num++;

		msg.msg_type = ISP_PROC_EVT_AE;
		rtn = cmr_thread_msg_send(isp_system_ptr->thread_proc, &msg);

		break;
	}

	case ISP_CTRL_EVT_AWB:
	{
		struct isp_ae_grgb_statistic_info* stat_ptr;
		struct isp_pm_param_data param_data;
		struct isp_pm_ioctl_input input = {NULL, 0};
		struct isp_pm_ioctl_output output = {NULL, 0};

		BLOCK_PARAM_CFG(input,
				param_data,
				ISP_PM_BLK_AWBM_STATISTIC,ISP_BLK_AWB_V1,
				NULL,
				0);
		rtn = isp_pm_ioctl(handle->handle_pm,
			ISP_PM_CMD_GET_SINGLE_SETTING,
			(void*)&input,
			(void*)&output);
		if ((ISP_SUCCESS == rtn) && output.param_data) {
			stat_ptr = output.param_data->data_ptr;
			rtn = isp_u_awbm_statistics(handle->handle_device,
				stat_ptr->r_info,
				stat_ptr->g_info,
				stat_ptr->b_info);
			if (ISP_SUCCESS == rtn) {
				msg.msg_type = ISP_PROC_EVT_AWB;
				rtn = cmr_thread_msg_send(isp_system_ptr->thread_proc, &msg);
			}
		}
		break;
	}

	case ISP_CTRL_EVT_AF:
		//msg.msg_type = ISP_PROC_EVT_AF;
		//rtn = cmr_thread_msg_send(isp_system_ptr->thread_proc, &msg);
		msg.msg_type = ISP_PROC_AF_CALC;
		rtn = cmr_thread_msg_send(isp_system_ptr->thread_af_proc, &msg);
		break;

	default:
		break;
	}

	if (rtn) {
		ISP_LOGI("Error happened");
		SET_GLB_ERR(rtn, handle);
	}

	return 0;
}


//gerald merge, http://review.source.spreadtrum.com/gerrit/#/c/281621 start
static isp_int _isp_lsc_routine(struct cmr_msg *message, void *client_data)
{
	int rtn = ISP_SUCCESS;
    	int pipe_fd = 0;
	pid_t tid = 0;

	isp_ctrl_context* handle = (isp_ctrl_context*)client_data;
	uint32_t evt = (uint32_t)(message->msg_type & ISP_PROC_LSC_EVT_MASK);

	switch (evt) {
		case ISP_PROC_LSC_CALC: {
			struct isp_alsc_calc_info alsc_info = {0};
			struct isp_alsc_calc_info *alsc_calc_info = (struct isp_alsc_calc_info *)(message->data);

			if(NULL== alsc_calc_info){
				ISP_LOGE("ALSC2 receive alsc msg failed");
				break;
			}
			memcpy(&alsc_info, alsc_calc_info,sizeof(struct isp_alsc_calc_info));
			uint32_t * stat_ptr_r = alsc_calc_info->stat_ptr;
			uint32_t * stat_ptr_g = alsc_calc_info->stat_ptr + 1024;
			uint32_t * stat_ptr_b = alsc_calc_info->stat_ptr + 1024*2;
			
			//gerald merge http://review.source.spreadtrum.com/gerrit/#/c/286361
			//ISP_LOGI("[gerald] _alsc_calc start");
			rtn = _alsc_calc(handle, stat_ptr_r,stat_ptr_g, stat_ptr_b, &alsc_info.stat_img_size, &alsc_info.win_size, alsc_info.image_width, alsc_info.image_height,alsc_info.awb_ct, alsc_info.awb_r_gain, alsc_info.awb_b_gain,alsc_info.stable);
		break;}

		case ISP_PROC_LSC_STOP:
		break;

		case ISP_PROC_LSC_DATA_UPDATE:
		break;

		default:
		break;

	}

	return rtn;
}
//gerald merge, http://review.source.spreadtrum.com/gerrit/#/c/281621 end


static cmr_int _isp_proc_routine(struct cmr_msg *message, void *client_data)
{
	int32_t                         rtn = ISP_SUCCESS;
/* [Vendor_28/40] */
    int pipe_fd = 0;
    pid_t tid = 0;
    ISP_LOGE("[3A Porting-AE]_isp_proc_routine: tid=%lu, pid=%lu\n", gettid(), getpid());

    if( message->first_in){
        pipe_fd = open("/dev/isp_pipe/pipe_1", O_RDWR);
        if(pipe_fd < 0) {
            ISP_LOGE("open pipe error!!");
        }else{
            tid = gettid();
            write(pipe_fd,&tid,sizeof(pid_t));
            close(pipe_fd);
            ISP_LOGE("[3A Porting]_isp_proc_routine: open pipe succeed!!");
        }
    }

	isp_ctrl_context                *handle = (isp_ctrl_context*)client_data;
	struct isp_system               *isp_system_ptr = &handle->system;
	struct ae_calc_out              ae_result = {0};
	struct awb_ctrl_calc_result     awb_result;
	nsecs_t                         system_time0 = 0;
	nsecs_t                         system_time1 = 0;
	uint32_t                        evt = (uint32_t)(message->msg_type&ISP_PROC_EVT_MASK);
	uint32_t                        sub_type = message->sub_msg_type;
	void                            *param_ptr = (void*)message->data;

	memset(&awb_result, 0, sizeof(awb_result));

	CMR_MSG_INIT(msg);

	switch (evt) {
	case ISP_PROC_EVT_AE:
	{
#ifdef CONFIG_CAMERA_ISP_AE_VERSION_V1
		ISP_LOGI("CONFIG_CAMERA_ISP_AE_VERSION_V1");
		struct isp_aem_info aem_info;
		struct isp_ae_grgb_statistic_info *ae_stat_ptr = NULL;
		struct isp_pm_param_data param_data;
		struct isp_pm_ioctl_input input = {NULL, 0};
		struct isp_pm_ioctl_output output = {NULL, 0};
		struct isp_awb_calc_info *awb_calc_info_ptr;
		int32_t ae_rtn = 0;

		if (handle->ae_lib_fun->product_id) {
			break;
		}

		ae_stat_ptr = (struct isp_ae_grgb_statistic_info *)&handle->aem_statistic;
		memset(&aem_info, 0, sizeof(aem_info));
		aem_info.stat_fmt = AE_AEM_FMT_RGB;
		aem_info.rgb_stat = ae_stat_ptr;
		{
			BLOCK_PARAM_CFG(input,
								param_data,
								ISP_PM_BLK_AEM_STATISTIC,ISP_BLK_AE_V1,
								ae_stat_ptr,
								sizeof(struct isp_ae_grgb_statistic_info));

			rtn = isp_pm_ioctl(handle->handle_pm,
							ISP_PM_CMD_SET_OTHERS,
							(void*)&input,
							(void*)&output);
		}

		system_time0 = isp_get_timestamp();
		ae_rtn = _ae_calc(handle, &aem_info, &ae_result);
		handle->isp_smart_eb = 1;
		system_time1 = isp_get_timestamp();
		ISP_LOGE("SYSTEM_TEST-ae:%lldms", system_time1-system_time0);

		if (ISP_SUCCESS == ae_rtn)
		{
			msg.msg_type = ISP_PROC_AWB_CALC;
			msg.sync_flag = CMR_MSG_SYNC_NONE;
			msg.data = (void*)malloc(sizeof(struct isp_awb_calc_info));
			if (NULL == msg.data) {
				ISP_LOGE("NULL == msg.data malloc failed");
				break;
			}
			awb_calc_info_ptr = (struct isp_awb_calc_info*)msg.data;
			awb_calc_info_ptr->ae_result = ae_result;
			memcpy((void*)&awb_calc_info_ptr->ae_stat, (void*)ae_stat_ptr, sizeof(struct isp_ae_grgb_statistic_info));
			awb_calc_info_ptr->k_addr = 0;
			awb_calc_info_ptr->u_addr = 0;
			awb_calc_info_ptr->type = 0;

			msg.alloc_flag = 1;
			rtn = cmr_thread_msg_send(isp_system_ptr->thread_awb_proc, &msg);
			if (ISP_SUCCESS != rtn) {
				if (msg.data != NULL) {
					free(msg.data);
					msg.data = NULL;
				}
				
				ISP_LOGE("thread awb send msg failed");
			}
		} else {
			;
		}
		break;
#else
		ISP_LOGI("CONFIG_CAMERA_ISP_AE_VERSION_V0");
		struct isp_aem_info aem_info;
		
		//new start gerald
		struct ae_calc_out ae_result = {0};
		struct awb_ctrl_calc_result awb_result;
		struct isp_alsc_calc_info alsc_calc_info;
		//new end gerald
		
		struct isp_awb_statistic_info *ae_stat_ptr=NULL;
		struct isp_pm_param_data param_data;
		struct isp_pm_ioctl_input input = {NULL, 0};
		struct isp_pm_ioctl_output output = {NULL, 0};
		struct awb_size win_size = {0};
		int32_t ae_rtn = 0;
		int32_t bv_new = 0;

		ae_stat_ptr = (struct isp_ae_grgb_statistic_info *)&handle->aem_statistic;
		memset(&aem_info, 0, sizeof(aem_info));
		aem_info.stat_fmt = AE_AEM_FMT_RGB;
		aem_info.rgb_stat = ae_stat_ptr;

		system_time0 = isp_get_timestamp();
		ae_rtn = _ae_calc(handle, &aem_info, &ae_result);
		handle->isp_smart_eb = 1;
		system_time1 = isp_get_timestamp();
		ISP_LOGE("SYSTEM_TEST-ae:%lldms", system_time1-system_time0);

		if (ISP_SUCCESS == ae_rtn) {
			struct awb_ctrl_ae_info ae_info = {0};
			float gain = 0;
			float exposure = 0;
			struct awb_size stat_img_size = {0};
			struct awb_size win_size = {0};
			int32_t iso = 0;
			int32_t bv0 = 0;
			int32_t bv_gain = 0;
			struct ae_get_ev ae_ev = {0};
			memset(&awb_result, 0, sizeof(awb_result));
			handle->log_lsc_size = sizeof(struct debug_lsc_param);
			rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_BV_BY_LUM, NULL, (void *)&bv0);
			rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_BV_BY_LUM_NEW, NULL, (void *)&bv_new);
			rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_ISO, NULL, &iso);
			rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_BV_BY_GAIN, NULL, (void *)&bv_gain);
			rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_GAIN, NULL, (void *)&gain);
			rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_EXP, NULL, (void *)&exposure);
			rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_EV, NULL, (void*)&ae_ev);

			ae_info.bv = bv_new;
			ae_info.iso = iso;
			ae_info.exposure = exposure;
			ae_info.gain = gain;
			ae_info.stable = ae_result.is_stab;
			ae_info.f_value = 2.2;	/*get from sensor driver later*/
			ae_info.ev_index = ae_ev.ev_index;
			memcpy(ae_info.ev_table, ae_ev.ev_tab, 16*sizeof(int32_t));

			/*AWB calc*/
			if (0 == handle->lock_awb_calc) {
				rtn = _awb_calc(handle, &ae_info, ae_stat_ptr, &awb_result);
				handle->awb_pg_flag = awb_result.pg_flag;
			}
			system_time1 = isp_get_timestamp();
			ISP_LOGI("SYSTEM_TEST-awb:%lldms", system_time1-system_time0);


			if (ISP_SUCCESS == rtn) {
				handle->alc_awb = awb_result.use_ccm | (awb_result.use_lsc << 8);
			}

			/*enable smart & alsc*/
			gCntSendMsgLsc++;
			if ((1 == handle->isp_smart_eb) && (0 == gCntSendMsgLsc % 3)) {
				/*smart lsc calc*/
				rtn = _smart_calc(handle, bv_new, bv_gain,awb_result.ct, handle->alc_awb);

				rtn = handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_GET_STAT_SIZE, (void *)&stat_img_size, NULL);

				rtn = handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_GET_WIN_SIZE, (void *)&win_size, NULL);

				struct isp_pm_param_data param_data_alsc = {0};
				struct isp_pm_ioctl_input  param_data_alsc_input = {NULL, 0};
				struct isp_pm_ioctl_output  param_data_alsc_output = {NULL, 0};

				BLOCK_PARAM_CFG(param_data_alsc_input, param_data_alsc, ISP_PM_BLK_LSC_GET_LSCTAB, ISP_BLK_2D_LSC, NULL, 0);
				rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, (void*)&param_data_alsc_input, (void*)&param_data_alsc_output);
				handle->lsc_tab_address = param_data_alsc_output.param_data->data_ptr;
				handle->lsc_tab_size = param_data_alsc_output.param_data->data_size;

				uint32_t * buf_stat_lsc = aeStatistic;
				uint32_t * ptr_r_stat = buf_stat_lsc;
				uint32_t * ptr_g_stat = buf_stat_lsc + 1024;
				uint32_t * ptr_b_stat = buf_stat_lsc + 1024*2;
				memcpy(ptr_r_stat, ae_stat_ptr->r_info, 1024*sizeof(uint32_t));
				memcpy(ptr_g_stat, ae_stat_ptr->g_info, 1024*sizeof(uint32_t));
				memcpy(ptr_b_stat, ae_stat_ptr->b_info, 1024*sizeof(uint32_t));

				/*send message to alsc proc thread*/
				alsc_calc_info.stat_ptr= buf_stat_lsc;
				alsc_calc_info.awb_b_gain = awb_result.gain.b;
				alsc_calc_info.awb_r_gain = awb_result.gain.r;
				alsc_calc_info.awb_ct = awb_result.ct;
				alsc_calc_info.stat_img_size.w= stat_img_size.w;
				alsc_calc_info.stat_img_size.h= stat_img_size.h;
				alsc_calc_info.win_size.w = win_size.w;
				alsc_calc_info.win_size.h = win_size.h;
				alsc_calc_info.stable = ae_info.stable;
				alsc_calc_info.image_width = handle->src.w;
				alsc_calc_info.image_height = handle->src.h;
				msg.msg_type = ISP_PROC_LSC_CALC;
				msg.data = (void*)malloc(sizeof(struct isp_alsc_calc_info));
				if(NULL==msg.data) {
					ALOGE("ALSC2 msg to lsc thread failed");
					break;
				}
				memcpy((void*)(msg.data), (void*)&alsc_calc_info, sizeof(struct isp_alsc_calc_info));
				msg.alloc_flag = 1;
				rtn = cmr_thread_msg_send(isp_system_ptr->thread_lsc_proc, &msg);
				if (ISP_SUCCESS != rtn) {
					if (msg.data != NULL) {
						free(msg.data);
						msg.data = NULL;
					}
				}				
			}
		}

		//???
		if(NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_thread_msg_send) {
			memset(&msg, 0, sizeof(msg));
			handle->af_lib_fun->af_ioctrl_thread_msg_send(handle, &ae_result, &msg);
		}
		if(NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_set_ae_awb_info) {
			handle->af_lib_fun->af_ioctrl_set_ae_awb_info(handle, &ae_result, &awb_result, &bv_new,ae_stat_ptr);
		}
		
		break;
#endif
	}

	case ISP_PROC_EVT_AWB:
	{
		struct isp_aem_info aem_info;
		struct isp_ae_grgb_statistic_info* stat_ptr;
		struct isp_pm_param_data param_data;
		struct isp_pm_ioctl_input input = {NULL, 0};
		struct isp_pm_ioctl_output output = {NULL, 0};
		struct awb_size stat_img_size = {0};
		struct awb_size win_size = {0};
		int32_t bv_new= 0;
		int32_t bv_gain = 0;
		int32_t ae_rtn = 0;

		BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_AWBM_STATISTIC, ISP_BLK_AWB_V1, NULL, 0);
		isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, (void*)&input, (void*)&output);
		if (output.param_data) {
			stat_ptr = output.param_data->data_ptr;
		}

		aem_info.stat_fmt = AE_AEM_FMT_RGB;
		aem_info.rgb_stat = stat_ptr;

		ae_rtn = _ae_calc(handle, &aem_info, &ae_result);

		if (ISP_SUCCESS == ae_rtn) {
			struct awb_ctrl_ae_info ae_info = {0};
			float gain = 0;
			float exposure = 0;
			struct ae_get_ev ae_ev = {0};

			rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_BV_BY_LUM_NEW, NULL, (void*)&bv_new);
			rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_BV_BY_GAIN, NULL, (void*)&bv_gain);
			rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_GAIN, NULL, (void*)&gain);
			rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_EXP, NULL, (void*)&exposure);
			rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_EV, NULL, (void*)&ae_ev);

			ae_info.bv = bv_new;
			ae_info.exposure = exposure;
			ae_info.gain = gain;
			ae_info.stable = ae_result.is_stab;
			ae_info.f_value = 2.2; /*get from sensor driver later*/
			ae_info.ev_index = ae_ev.ev_index;
			memcpy(ae_info.ev_table, ae_ev.ev_tab, 16*sizeof(int32_t));

			rtn = _awb_calc(handle, &ae_info,stat_ptr, &awb_result);
			if (ISP_SUCCESS == rtn) {
				handle->alc_awb = awb_result.use_ccm | (awb_result.use_lsc << 8);
			}

			rtn = _smart_calc(handle, bv_new, bv_gain, awb_result.ct, handle->alc_awb);

			rtn = handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_GET_STAT_SIZE, (void *)&stat_img_size, NULL);

			rtn = handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_GET_WIN_SIZE, (void *)&win_size, NULL);

			rtn = _alsc_calc(handle, stat_ptr->r_info, stat_ptr->g_info, stat_ptr->b_info, &stat_img_size, &win_size, handle->src.w, handle->src.h, awb_result.ct, awb_result.gain.r, awb_result.gain.b, ae_info.stable); //http://review.source.spreadtrum.com/gerrit/#/c/281621 gerald merge, http://review.source.spreadtrum.com/gerrit/#/c/286361

			//rtn = _smart_lsc_pre_calc(handle, awb_stat_ptr, &stat_img_size, &win_size, awb_result.ct, ae_info.stable);gerald todo
			isp_cur_bv = bv_new;
			isp_cur_ct = awb_result.ct;


		}

		if (NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_set_ae_awb_info) {
			handle->af_lib_fun->af_ioctrl_set_ae_awb_info(handle, &ae_result, &awb_result, &bv_new,stat_ptr);
		}

		break;
	}

	case ISP_PROC_EVT_AF:
		break;

	case ISP_PROC_EVT_AF_STOP:
		break;

	case ISP_PROC_EVT_STOP_HANDLER:
		break;

	case ISP_PROC_EVT_AEM2:
		break;

	default:
		break;
	}

	return 0;
}

static cmr_int _isp_proc_afl_routine(struct cmr_msg *message, void *client_data)
{
	isp_ctrl_context                *handle = (isp_ctrl_context*)client_data;
	struct isp_system               *isp_system_ptr = &handle->system;
	uint32_t                        evt = (uint32_t)(message->msg_type&ISP_AFL_EVT_MASK);

	switch (evt) {
	case ISP_PROC_AFL_DONE: {
		ISP_LOGI("ISP_PROC_AFL_DONE");
		if (handle->anti_flicker_mode > AE_FLICKER_60HZ) {
			_ispAntiflicker_calc(handle);
			ISP_LOGI("handle->anti_flicker_mode = %d , anti flicker auto control open !", handle->anti_flicker_mode);
		}
		break;
	}
	case ISP_PROC_AFL_STOP: {
		uint32_t bypass = 1;
		isp_u_anti_flicker_bypass(handle->handle_device, (void*)&bypass);
		break;
	}
	default:
		break;
	}

	return 0;
}

static cmr_int _isp_proc_af_routine(struct cmr_msg *message, void *client_data)
{
	int32_t                         rtn = ISP_SUCCESS;
/* [Vendor_29/40] */
    int pipe_fd = 0;
    pid_t tid = 0;

    if( message->first_in){
        pipe_fd = open("/dev/isp_pipe/pipe_1", O_RDWR);
        if(pipe_fd < 0) {
            ISP_LOGE("open pipe error!!");
        }else{
            tid = gettid();
            write(pipe_fd,&tid,sizeof(pid_t));
            close(pipe_fd);
			ISP_LOGE("[3A Porting]_isp_proc_af_routine: open pipe succeed!!");
        }
    }

	isp_ctrl_context                *handle = (isp_ctrl_context*)client_data;
	uint32_t                        evt = (uint32_t)(message->msg_type&ISP_PROC_AF_EVT_MASK);

	switch (evt) {
	case ISP_PROC_AF_CALC: {
		handle->af_lib_fun->af_calc_interface(handle);
		break;
	}
	case ISP_PROC_AF_IMG_DATA_UPDATE: {
		if(NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_image_data_update) {
			rtn = handle->af_lib_fun->af_image_data_update(handle);
		}
		break;
	}
	default:
		break;
	}

	return 0;
}

static isp_int _isp_proc_awb_routine(struct cmr_msg *message, void *client_data)
{
	int rtn = ISP_SUCCESS;

	isp_ctrl_context* handle = (isp_ctrl_context*)client_data;
	uint32_t  evt = (uint32_t)(message->msg_type & ISP_PROC_AWB_EVT_MASK);
	struct isp_system               *isp_system_ptr = &handle->system;
	CMR_MSG_INIT(msg);

	switch (evt) {
		case ISP_PROC_AWB_CALC: {
			struct  isp_awb_calc_info *awb_calc_info = (struct isp_awb_calc_info *)message->data;
			if (NULL == awb_calc_info) {
				ISP_LOGE("NULL == awb_calc_info");
				break;
			}
			struct  ae_calc_out *ae_result = &awb_calc_info->ae_result;
			struct isp_ae_grgb_statistic_info *ae_stat_ptr = &awb_calc_info->ae_stat;
			struct awb_ctrl_ae_info ae_info = {0};
			float gain = 0;
			float exposure = 0;
			struct awb_size stat_img_size = {0};
			struct awb_size win_size = {0};
			//int32_t bv = 0;
			int32_t bv_new = 0;
			int32_t iso = 0;
			int32_t bv_gain = 0;
			struct ae_get_ev ae_ev = {0};
			struct awb_ctrl_calc_result awb_result;
			memset(&awb_result, 0, sizeof(awb_result));
			nsecs_t system_time0 = 0;
			nsecs_t system_time1 = 0;

			//rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_BV_BY_LUM, NULL, (void *)&bv);
			rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_BV_BY_LUM_NEW, NULL, (void *)&bv_new);
			rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_ISO, NULL, &iso);
			rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_BV_BY_GAIN, NULL, (void *)&bv_gain);
			rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_GAIN, NULL, (void *)&gain);
			rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_EXP, NULL, (void *)&exposure);
			rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_EV, NULL, (void*)&ae_ev);

			ae_info.bv = bv_new;
			ae_info.iso = iso;
			ae_info.exposure = exposure;
			ae_info.gain = gain;
			ae_info.stable = ae_result->is_stab;
			ae_info.f_value = 2.2;	/*get from sensor driver later*/
			ae_info.ev_index = ae_ev.ev_index;
			memcpy(ae_info.ev_table, ae_ev.ev_tab, 16*sizeof(int32_t));

			system_time0 = isp_get_timestamp();
			rtn = _awb_calc(handle, &ae_info, ae_stat_ptr, &awb_result);
			system_time1 = isp_get_timestamp();
			ISP_LOGI("SYSTEM_TEST-awb:%lldms", system_time1-system_time0);

			if (ISP_SUCCESS == rtn) {
				handle->alc_awb = awb_result.use_ccm | (awb_result.use_lsc << 8);
			}

			system_time0 = isp_get_timestamp();
			//ISP_LOGI("[gerald] isp_smart_eb:%d", handle->isp_smart_eb);
			if (1 == handle->isp_smart_eb) {
				rtn = _smart_calc(handle, bv_new, bv_gain,awb_result.ct, handle->alc_awb);
				gCntSendMsgLsc++;
				if(0 == gCntSendMsgLsc % 3){
					uint32_t * buf_stat_lsc = aeStatistic;
					uint32_t * ptr_r_stat = buf_stat_lsc;
					uint32_t * ptr_g_stat = buf_stat_lsc + 1024;
					uint32_t * ptr_b_stat = buf_stat_lsc + 1024*2;
					rtn = handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_GET_STAT_SIZE, (void *)&stat_img_size, NULL);
					rtn = handle->awb_lib_fun->awb_ctrl_ioctrl(handle->handle_awb, AWB_CTRL_CMD_GET_WIN_SIZE, (void *)&win_size, NULL);

					struct isp_pm_param_data param_data_alsc = {0};
					struct isp_pm_ioctl_input  param_data_alsc_input = {NULL, 0};
					struct isp_pm_ioctl_output  param_data_alsc_output = {NULL, 0};

					BLOCK_PARAM_CFG(param_data_alsc_input, param_data_alsc, ISP_PM_BLK_LSC_GET_LSCTAB, ISP_BLK_2D_LSC, NULL, 0);
					rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, (void*)&param_data_alsc_input, (void*)&param_data_alsc_output);
					handle->lsc_tab_address = param_data_alsc_output.param_data->data_ptr;
					handle->lsc_tab_size = param_data_alsc_output.param_data->data_size;
					memcpy(ptr_r_stat, ae_stat_ptr->r_info, 1024*sizeof(uint32_t));
					memcpy(ptr_g_stat, ae_stat_ptr->g_info, 1024*sizeof(uint32_t));
					memcpy(ptr_b_stat, ae_stat_ptr->b_info, 1024*sizeof(uint32_t));
					struct isp_alsc_calc_info alsc_calc_info;
					/*send message to alsc proc thread*/
					alsc_calc_info.stat_ptr= buf_stat_lsc;
					alsc_calc_info.awb_b_gain = awb_result.gain.b;
					alsc_calc_info.awb_r_gain = awb_result.gain.r;
					alsc_calc_info.awb_ct = awb_result.ct;
					alsc_calc_info.stat_img_size.w= stat_img_size.w;
					alsc_calc_info.stat_img_size.h= stat_img_size.h;
					alsc_calc_info.win_size.w = win_size.w;
					alsc_calc_info.win_size.h = win_size.h;
					alsc_calc_info.stable = ae_info.stable;
					alsc_calc_info.image_width = handle->src.w;
					alsc_calc_info.image_height = handle->src.h;
					msg.msg_type = ISP_PROC_LSC_CALC;
					msg.sync_flag = CMR_MSG_SYNC_PROCESSED;
					msg.data = (void*)malloc(sizeof(struct isp_alsc_calc_info));
					if(NULL == msg.data){
						ISP_LOGE("ALSC2 msg to lsc thread failed");
						break;
					} else {
						memcpy((void*)(msg.data), (void*)&alsc_calc_info, sizeof(struct isp_alsc_calc_info));
						msg.alloc_flag = 1;
						rtn = cmr_thread_msg_send(isp_system_ptr->thread_lsc_proc, &msg);
						if (ISP_SUCCESS != rtn) {
							ISP_LOGE("thread lsc send msg failed");
							if (msg.data != NULL) {
								free(msg.data);
								msg.data = NULL;
							}
						}
					}
				}
			}
			system_time1 = isp_get_timestamp();
			ISP_LOGI("SYSTEM_TEST-smart:%lldms", system_time1-system_time0);

			isp_cur_bv = bv_new;
			isp_cur_ct = awb_result.ct;

			if(NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_thread_msg_send) {
				memset(&msg, 0, sizeof(msg));
				handle->af_lib_fun->af_ioctrl_thread_msg_send(handle, ae_result, &msg);
			}
			if(NULL!=handle->ioctrl_ptr->set_focus && NULL!=handle->af_lib_fun->af_ioctrl_set_ae_awb_info) {
				handle->af_lib_fun->af_ioctrl_set_ae_awb_info(handle, ae_result, &awb_result, &bv_new,ae_stat_ptr);
			}

			break;
		}
		case ISP_PROC_AWB_STOP:
			break;
		default:
			break;
	}
	return 0;
}

static void _isp_delete_thread(struct isp_system *ptr)
{
	if (NULL == ptr) {
		return;
	}

	/*destroy proc thread*/
	if (NULL != ptr->thread_proc) {
		cmr_thread_destroy(ptr->thread_proc);
		ptr->thread_proc = NULL;
	}

	/*destroy ctrl thread*/
	if (NULL != ptr->thread_ctrl) {
		cmr_thread_destroy(ptr->thread_ctrl);
		ptr->thread_ctrl = NULL;
	}

#ifdef CONFIG_CAMERA_AFL_AUTO_DETECTION
	/*destroy anti_flicker thread*/
	if (NULL != ptr->thread_afl_proc) {
		cmr_thread_destroy(ptr->thread_afl_proc);
		ptr->thread_afl_proc = NULL;
	}
#endif

	/*destroy af thread*/
	if (NULL != ptr->thread_af_proc) {
		cmr_thread_destroy(ptr->thread_af_proc);
		ptr->thread_af_proc = NULL;
	}

	/*destroy lsc thread*/
	if (NULL != ptr->thread_lsc_proc) { //gerald merge http://review.source.spreadtrum.com/gerrit/#/c/281621
		cmr_thread_destroy(ptr->thread_lsc_proc); //gerald merge http://review.source.spreadtrum.com/gerrit/#/c/281621
		ptr->thread_lsc_proc = NULL; //gerald merge http://review.source.spreadtrum.com/gerrit/#/c/281621
	}
	
	/*destroy awb thread*/
	if (NULL != ptr->thread_awb_proc) {
		cmr_thread_destroy(ptr->thread_awb_proc);
		ptr->thread_awb_proc = NULL;
	}

	return;
}

static int _isp_create_resource(isp_handle isp_handler, struct isp_init_param *param)
{
	int                             rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_system               *isp_system_ptr = &handle->system;
	struct sensor_raw_info          *sensor_raw_info_ptr = (struct sensor_raw_info*)param->setting_param_ptr;

	CMR_MSG_INIT(msg);

	SET_GLB_ERR(0, handle);

	/*create proc thread*/
	rtn = cmr_thread_create(&isp_system_ptr->thread_proc, ISP_THREAD_QUEUE_NUM, _isp_proc_routine, (void*)handle);
	if (rtn) {
		ISP_LOGE("create proc thread error");
		goto EXIT;
	}

	/*create ctrl thread*/
	rtn = cmr_thread_create(&isp_system_ptr->thread_ctrl, ISP_THREAD_QUEUE_NUM, _isp_ctrl_routine, (void*)handle);
	if (rtn) {
		ISP_LOGE("create ctrl thread error");
		goto EXIT;
	}

	/*create anti_flicker thread*/
#ifdef CONFIG_CAMERA_AFL_AUTO_DETECTION
	rtn = cmr_thread_create(&isp_system_ptr->thread_afl_proc, ISP_THREAD_QUEUE_NUM, _isp_proc_afl_routine, (void*)handle);
	if (rtn) {
		ISP_LOGE("create afl thread error");
		goto EXIT;
	}
#endif
	/*create af  thread*/
	if(NULL!=sensor_raw_info_ptr->ioctrl_ptr->set_focus){
		rtn = cmr_thread_create(&isp_system_ptr->thread_af_proc, ISP_THREAD_QUEUE_NUM, _isp_proc_af_routine, (void*)handle);
		if (rtn) {
			ISP_LOGE("create af thread error");
			goto EXIT;
		}
	}
	/*create lsc  thread*/ /*gerald merge http://review.source.spreadtrum.com/gerrit/#/c/281621*/
	rtn = cmr_thread_create(&isp_system_ptr->thread_lsc_proc, ISP_THREAD_QUEUE_NUM, _isp_lsc_routine, (void*)handle);
	if (rtn) {
		ISP_LOGE("create lsc thread error");
		goto EXIT;
	}

	/*create awb  thread*/
	rtn = cmr_thread_create(&isp_system_ptr->thread_awb_proc, ISP_THREAD_QUEUE_NUM, _isp_proc_awb_routine, (void*)handle);
	if (rtn) {
		ISP_LOGE("create awb thread error");
		goto EXIT;
	}

	msg.msg_type  = ISP_CTRL_EVT_INIT;
	msg.sync_flag = CMR_MSG_SYNC_PROCESSED;
	msg.data      = (void*)malloc(sizeof(struct isp_init_param));
	if (NULL == msg.data) {
		ISP_LOGE("No memory");
		rtn = ISP_ERROR;
		goto EXIT;
	}
	memcpy((void*)msg.data, (void*)param, sizeof(struct isp_init_param));
	msg.alloc_flag = 1;
	rtn = cmr_thread_msg_send(handle->system.thread_ctrl, &msg);
	if (ISP_SUCCESS == rtn) {
		rtn = GET_GLB_ERR(handle);
	}
	if (rtn) {
		if (msg.data != NULL) {
			free(msg.data);
			msg.data = NULL;
		}
		//free(msg.data);
		ISP_LOGE("Something error happened");
		rtn = ISP_ERROR;
		goto EXIT;
	}

	isp_system_ptr->monitor_status = ISP_IDLE;
	isp_system_ptr->posture_status = ISP_IDLE;

EXIT:
	if (rtn) {
		_isp_delete_thread(isp_system_ptr);
	}
	return rtn;
}

static int _isp_release_resource(isp_handle isp_handler)
{
	int                             rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_system               *isp_system_ptr = &handle->system;
	uint32_t dummy = 0;

	CMR_MSG_INIT(msg);

	SET_GLB_ERR(0, handle);

	msg.msg_type = ISP_CTRL_EVT_DEINIT;
	msg.sync_flag = CMR_MSG_SYNC_PROCESSED;
	cmr_thread_msg_send(isp_system_ptr->thread_ctrl, &msg);
	cmr_thread_destroy(isp_system_ptr->thread_ctrl);
	isp_system_ptr->thread_ctrl = NULL;

	/*destroy proc thread*/
	rtn = cmr_thread_destroy(isp_system_ptr->thread_proc);
	isp_system_ptr->thread_proc = NULL;

	/*destroy anti_flicker thread*/
#ifdef CONFIG_CAMERA_AFL_AUTO_DETECTION
	rtn = cmr_thread_destroy(isp_system_ptr->thread_afl_proc);
	isp_system_ptr->thread_afl_proc = NULL;
#endif

	/*destroy awb thread*/
	rtn = cmr_thread_destroy(isp_system_ptr->thread_awb_proc);
	isp_system_ptr->thread_awb_proc = NULL;
	
	/*destroy lsc thread*/
	if (NULL != isp_system_ptr->thread_lsc_proc) {
		rtn = cmr_thread_destroy(isp_system_ptr->thread_lsc_proc);
		isp_system_ptr->thread_lsc_proc = NULL;
	}

	/*destroy af thread*/
	if(NULL!=handle->ioctrl_ptr->set_focus){
		rtn = cmr_thread_destroy(isp_system_ptr->thread_af_proc);
		isp_system_ptr->thread_af_proc = NULL;
	}

	rtn = _ispAlgDeInit(handle);
	ISP_TRACE_IF_FAIL(rtn, ("_ispAlgDeInit error"));

	rtn = sensor_merge_isp_param_free(handle);
	ISP_TRACE_IF_FAIL(rtn, ("merge isp param free error"));

	rtn = isp_pm_deinit(handle->handle_pm, NULL, NULL);
	ISP_TRACE_IF_FAIL(rtn, ("isp_pm_deinit error"));
	handle->handle_pm = PNULL;

	_otp_deinit(handle);

	if (handle->log_alc) {
		free(handle->log_alc);
		handle->log_alc = NULL;
	}

	if (handle->log_isp) {
		free(handle->log_isp);
		handle->log_isp = NULL;
	}

	handle->aem_irq = 0;
	rtn = pthread_join(handle->system.monitor_thread, &dummy);
	handle->system.monitor_thread = 0;

	rtn = pthread_join(handle->system.posture_thread, &dummy);
	handle->system.posture_thread = 0;
	pthread_mutex_destroy(&handle->system.posture_mutex);

	if (ISP_SUCCESS == rtn) {
		rtn = GET_GLB_ERR(handle);
	}

	return rtn;
}


/* Public Function Prototypes */

int isp_init(struct isp_init_param *ptr, isp_handle *isp_handler)
{
 	int                             rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = NULL;
	struct isp_system               *isp_system_ptr = NULL;

	ISP_LOGI("isp_init");

	handle = (isp_ctrl_context*)malloc(sizeof(isp_ctrl_context));
	if (NULL == handle) {
		ISP_LOGE("No memory");
		return ISP_ERROR;
	}

	memset((void*)handle, 0x00, sizeof(isp_ctrl_context));

	rtn = _isp_create_resource(handle, ptr);
	if (rtn) {
		free((void*)handle);
		ISP_LOGE("create resource error");
		return rtn;
	}

	*isp_handler = handle;

	ISP_LOGV("---isp_init-- end, 0x%x", rtn);

	return rtn;
}

int isp_deinit(isp_handle isp_handler)
{
	int                             rtn = ISP_SUCCESS;
	isp_uint type                    = 0;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_system               *isp_system_ptr = &handle->system;

	CMR_MSG_INIT(msg);

	ISP_LOGI("isp_deinit");
	if (NULL == handle) {
		return rtn;
	}
	rtn = _isp_release_resource(handle);

	if(handle->isp_bq_alloc_flag == 1) {
		if (handle->need_soft_ae)
			type = CAMERA_ISP_BINGING4AWB_FLAG;
		else
			type = CAMERA_ISP_RAWAEM_FLAG;

		if (handle->cb_of_free) {
			isp_cb_of_free cb_free = handle->cb_of_free;
			cb_free(type,(isp_uint *)handle->isp_bq_k_addr_array,(isp_uint *)handle->isp_bq_u_addr_array,
					handle->isp_bq_mem_num, handle->buffer_client_data);
/* [Vendor_30/40] */
			cb_free(CAMERA_ISP_RAWAFM_FLAG,(isp_uint *)handle->isp_af_bq_k_addr_array,(isp_uint *)handle->isp_af_bq_u_addr_array,
					handle->isp_af_bq_mem_num, handle->buffer_client_data);
			ISP_LOGI("[3A Porting] isp_deinit: cb_free.");
		}

		handle->isp_bq_alloc_flag = 0;
	}

	if (NULL != handle) {
		free(handle);
		handle = NULL;
	}

	ISP_LOGV("---isp_deinit------- end, 0x%x", rtn);

	return rtn;
}

int isp_capability(isp_handle isp_handler, enum isp_capbility_cmd cmd, void *param_ptr)
{
	int                             rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;

	switch (cmd) {
	case ISP_VIDEO_SIZE:
	{
		struct isp_video_limit *size_ptr = param_ptr;
		rtn = isp_u_capability_continue_size(handle->handle_device, &size_ptr->width, &size_ptr->height);
		break;
  	}

	case ISP_CAPTURE_SIZE:
	{
		struct isp_video_limit *size_ptr = param_ptr;
		rtn = isp_u_capability_single_size(handle->handle_device, &size_ptr->width, &size_ptr->height);
		break;
	}

	case ISP_LOW_LUX_EB:
	{
		uint32_t out_param = 0;
		rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_FLASH_EB, NULL, &out_param);
		*((uint32_t*)param_ptr) = out_param;
		break;
	}

	case ISP_CUR_ISO:
	{
		uint32_t out_param = 0;
		rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_ISO, NULL, &out_param);
		*((uint32_t*)param_ptr) = out_param;
		break;
	}

	case ISP_REG_VAL:
	{
		rtn = isp_dev_reg_fetch(handle->handle_device, 0, (uint32_t*)param_ptr,0x1000);
		break;
	}
	case ISP_CTRL_GET_AE_LUM:
	{
		uint32_t out_param = 0;
		rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_BV_BY_LUM, NULL, &out_param);
		*((uint32_t*)param_ptr) = out_param;
		ISP_LOGI("ISP_CTRL_GET_AE_LUM lls_info = %d", out_param);
		break;
	}

	default:
		break;
	}

	return rtn;
}

int isp_ioctl(isp_handle isp_handler, enum isp_ctrl_cmd cmd, void *param_ptr)
{
	int                             rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_system               *isp_system_ptr = NULL;

	if (NULL == handle) {
		ISP_LOGE("isp handler is NULL");
		return ISP_PARAM_NULL;
	}

	isp_system_ptr = &handle->system;
	if (NULL == isp_system_ptr) {
		ISP_LOGE("isp system is NULL");
		return ISP_PARAM_NULL;
	}

	CMR_MSG_INIT(msg);
	ISP_LOGV("E cmd:0x%x", cmd);
	SET_GLB_ERR(0, handle);

	msg.msg_type     = ISP_CTRL_EVT_IOCTRL;
	msg.sub_msg_type = cmd;
	msg.sync_flag    = CMR_MSG_SYNC_PROCESSED;
	msg.data         = (void*)param_ptr;

	if (NULL != isp_system_ptr->thread_ctrl) {
		rtn = cmr_thread_msg_send(isp_system_ptr->thread_ctrl, &msg);
	}

	if (NULL != isp_system_ptr->callback) {
		isp_system_ptr->callback(isp_system_ptr->caller_id, ISP_CALLBACK_EVT|ISP_CTRL_CALLBACK|cmd, NULL, ISP_ZERO);
	}
	if (ISP_SUCCESS == rtn) {
		rtn = GET_GLB_ERR(handle);
	}
	ISP_TRACE_IF_FAIL(rtn, ("isp_ioctl error"));

	return rtn;
}

int isp_video_start(isp_handle isp_handler, struct isp_video_start *param_ptr)
{
	int                             rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_system               *isp_system_ptr = NULL;

	if (NULL == handle) {
		ISP_LOGE("isp handler is NULL");
		return ISP_PARAM_NULL;
	}

	isp_system_ptr = &handle->system;
	if (NULL == isp_system_ptr) {
		ISP_LOGE("isp system is NULL");
		return ISP_PARAM_NULL;
	}

	CMR_MSG_INIT(msg);

	ISP_LOGI("E");
	SET_GLB_ERR(0, handle);

	rtn = _isp_check_video_param(param_ptr);
	ISP_RETURN_IF_FAIL(rtn, ("check param error"));

	msg.data = (void*)malloc(sizeof(struct isp_video_start));
	if (NULL == msg.data) {
		ISP_LOGE("No memory");
		return -ISP_ALLOC_ERROR;
	}

	memcpy((void*)msg.data, (void*)param_ptr, sizeof(struct isp_video_start));
	msg.alloc_flag = 1;
	msg.msg_type   = ISP_CTRL_EVT_CONTINUE;
	msg.sync_flag  = CMR_MSG_SYNC_PROCESSED;
	rtn = cmr_thread_msg_send(isp_system_ptr->thread_ctrl, &msg);
	if (ISP_SUCCESS == rtn) {
		rtn = GET_GLB_ERR(handle);
	} else {
		if (msg.data != NULL) {
			free(msg.data);
			msg.data = NULL;
		}
	}
	ISP_TRACE_IF_FAIL(rtn, ("isp_video_start error"));

	return rtn;
}

int isp_video_stop(isp_handle isp_handler)
{
	int                             rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_system               *isp_system_ptr = NULL;

	if (NULL == handle) {
		ISP_LOGE("isp handler is NULL");
		return ISP_PARAM_NULL;
	}

	isp_system_ptr = &handle->system;
	if (NULL == isp_system_ptr) {
		ISP_LOGE("isp system is NULL");
		return ISP_PARAM_NULL;
	}

	CMR_MSG_INIT(msg);

	ISP_LOGI("E");
	SET_GLB_ERR(0, handle);

	msg.msg_type  = ISP_CTRL_EVT_CONTINUE_STOP;
	msg.sync_flag = CMR_MSG_SYNC_PROCESSED;
	rtn = cmr_thread_msg_send(isp_system_ptr->thread_ctrl, &msg);
	if (ISP_SUCCESS == rtn) {
		rtn = GET_GLB_ERR(handle);
	}
	ISP_TRACE_IF_FAIL(rtn, ("isp_video_stop error"));

	return rtn;
}

int isp_proc_start(isp_handle isp_handler, struct ips_in_param *in_param_ptr, struct ips_out_param *out_param_ptr)
{
	int                             rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_system               *isp_system_ptr = NULL;

	if (NULL == handle) {
		ISP_LOGE("isp handler is NULL");
		return ISP_PARAM_NULL;
	}

	isp_system_ptr = &handle->system;
	if (NULL == isp_system_ptr) {
		ISP_LOGE("isp system is NULL");
		return ISP_PARAM_NULL;
	}

	CMR_MSG_INIT(msg);

	ISP_LOGD("E");
	ISP_LOGD("src image_format 0x%x", in_param_ptr->src_frame.img_fmt);
	ISP_LOGD("src img_size: %d, %d", in_param_ptr->src_frame.img_size.w, in_param_ptr->src_frame.img_size.h);
	ISP_LOGD("src addr:0x%x", in_param_ptr->src_frame.img_addr_phy.chn0);
	ISP_LOGD("src format pattern: %d", in_param_ptr->src_frame.format_pattern);
	ISP_LOGD("dst image_format 0x%x", in_param_ptr->dst_frame.img_fmt);
	ISP_LOGD("dst img_size: %d, %d", in_param_ptr->dst_frame.img_size.w, in_param_ptr->dst_frame.img_size.h);
	ISP_LOGD("dst addr:y=0x%x, uv=0x%x", in_param_ptr->dst_frame.img_addr_phy.chn0, in_param_ptr->dst_frame.img_addr_phy.chn1);
	ISP_LOGD("src_avail_height:%d", in_param_ptr->src_avail_height);
	ISP_LOGD("src_slice_height:%d", in_param_ptr->src_slice_height);
	ISP_LOGD("dst_slice_height:%d", in_param_ptr->dst_slice_height);

	SET_GLB_ERR(0, handle);

	rtn = _isp_check_proc_start_param(in_param_ptr);
	ISP_RETURN_IF_FAIL(rtn, ("check init param error"));

	msg.msg_type = ISP_CTRL_EVT_SIGNAL;
	msg.sync_flag = CMR_MSG_SYNC_PROCESSED;
	msg.data = (void*)in_param_ptr;
	rtn = cmr_thread_msg_send(isp_system_ptr->thread_ctrl, &msg);
	if (ISP_SUCCESS == rtn) {
		rtn = GET_GLB_ERR(handle);
	}
	ISP_TRACE_IF_FAIL(rtn, ("isp_proc_start error"));

	return rtn;
}

int isp_proc_next(isp_handle isp_handler, struct ipn_in_param *in_ptr, struct ips_out_param *out_ptr)
{
	int                             rtn = ISP_SUCCESS;
	isp_ctrl_context                *handle = (isp_ctrl_context*)isp_handler;
	struct isp_system               *isp_system_ptr = NULL;

	if (NULL == handle) {
		ISP_LOGE("isp handler is NULL");
		return ISP_PARAM_NULL;
	}

	isp_system_ptr = &handle->system;
	if (NULL == isp_system_ptr) {
		ISP_LOGE("isp system is NULL");
		return ISP_PARAM_NULL;
	}

	CMR_MSG_INIT(msg);

	ISP_LOGI("E");
	SET_GLB_ERR(0, handle);

	rtn = _isp_check_proc_next_param(in_ptr);
	ISP_RETURN_IF_FAIL(rtn, ("check init param error"));

	msg.msg_type  = ISP_CTRL_EVT_SIGNAL_NEXT;
	msg.sync_flag = CMR_MSG_SYNC_PROCESSED;
	msg.data      = (void*)in_ptr;
	rtn = cmr_thread_msg_send(isp_system_ptr->thread_ctrl, &msg);
	if (ISP_SUCCESS == rtn) {
		rtn = GET_GLB_ERR(handle);
	}
	ISP_TRACE_IF_FAIL(rtn, ("isp_proc_next error"));

	ISP_LOGI("X");

	return rtn;
}

