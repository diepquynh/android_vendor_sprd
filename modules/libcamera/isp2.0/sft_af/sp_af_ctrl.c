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
#define LOG_TAG "sp_af_ctrl"
#include "isp_com.h"
#include "isp_drv.h"
#include "sp_af_ctrl.h"
#include "AF_Main.h"
#include "AF_Common.h"
#include "AF_Control.h"
#include "AF_Interface.h"
#include "stdio.h"
#include <utils/Log.h>
#include "ae_sprd_ctrl.h"
#include "af_ctrl.h"
#include "awb_ctrl.h"
#include "lib_ctrl.h"
#include "isp_pm.h"

#ifdef WIN32
#define SFT_AF_LOG
#define SFT_AF_LOGW
#define SFT_AF_LOGI
#define SFT_AF_LOGD
#define SFT_AF_LOGV
#else
#define SFT_AF_DEBUG_STR     "SFT_AF: %d, %s: "
#define SFT_AF_DEBUG_ARGS    __LINE__,__FUNCTION__

#define SFT_AF_LOG(format,...) ALOGE(SFT_AF_DEBUG_STR format, SFT_AF_DEBUG_ARGS, ##__VA_ARGS__)
#define SFT_AF_LOGE(format,...) ALOGE(SFT_AF_DEBUG_STR format, SFT_AF_DEBUG_ARGS, ##__VA_ARGS__)
#define SFT_AF_LOGW(format,...) ALOGW(SFT_AF_DEBUG_STR format, SFT_AF_DEBUG_ARGS, ##__VA_ARGS__)
#define SFT_AF_LOGI(format,...) ALOGI(SFT_AF_DEBUG_STR format, SFT_AF_DEBUG_ARGS, ##__VA_ARGS__)
#define SFT_AF_LOGD(format,...) ALOGD(SFT_AF_DEBUG_STR format, SFT_AF_DEBUG_ARGS, ##__VA_ARGS__)
#define SFT_AF_LOGV(format,...) ALOGI(SFT_AF_DEBUG_STR format, SFT_AF_DEBUG_ARGS, ##__VA_ARGS__)
#endif


#define AF_CALLBACK_EVT 0x00040000

void sft_af_log(const char* format, ...)
{
	char buffer[2048]={0};
	va_list arg;
	va_start (arg, format);
	vsnprintf(buffer, 2048, format, arg);
	va_end (arg);
	ALOGE("SFT_AF: %s",buffer);
}



static int32_t _check_handle(sft_af_handle_t handle)
{
	int32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *cxt = (struct sft_af_context *)handle;

	if (NULL == cxt) {
		SFT_AF_LOG("invalid cxt pointer");
		return SFT_AF_ERROR;
	}

	if (SFT_AF_MAGIC_START != cxt->magic_start
		|| SFT_AF_MAGIC_END!= cxt->magic_end) {
		SFT_AF_LOG("invalid magic begin = 0x%x, magic end = 0x%x",
					cxt->magic_start, cxt->magic_end);
		return SFT_AF_ERROR;
	}

	return rtn;
}



int32_t set_afm_bypass(sft_af_handle_t handle,uint32_t bypass)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;


	SFT_AF_LOG(" %d",bypass);
	isp_u_raw_afm_bypass(ctrl_context->handle_device,bypass);

	return rtn;
}

int32_t set_afm_mode(sft_af_handle_t handle,uint32_t mode)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;


	SFT_AF_LOG(" %d",mode);
	isp_u_raw_afm_mode(ctrl_context->handle_device,mode);

	return rtn;
}

int32_t set_afm_skip_num(sft_af_handle_t handle,uint32_t skip_num)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	SFT_AF_LOG(" %d",skip_num);
	isp_u_raw_afm_skip_num(ctrl_context->handle_device,skip_num);
	return rtn;
}

int32_t set_afm_skip_num_clr(sft_af_handle_t handle)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	SFT_AF_LOG(" ");


	isp_u_raw_afm_skip_num_clr(ctrl_context->handle_device,1);
	isp_u_raw_afm_skip_num_clr(ctrl_context->handle_device,0);

	return rtn;
}

int32_t set_afm_spsmd_rtgbot_enable(sft_af_handle_t handle,uint32_t enable)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	SFT_AF_LOG("set_afm_spsmd_rtgbot_enable %d",enable);
	isp_u_raw_afm_spsmd_rtgbot_enable(ctrl_context->handle_device,enable);
	return rtn;
}

int32_t set_afm_spsmd_diagonal_enable(sft_af_handle_t handle,uint32_t enable)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;


	SFT_AF_LOG("set_afm_spsmd_diagonal_enable %d",enable);
	isp_u_raw_afm_spsmd_diagonal_enable(ctrl_context->handle_device,enable);
	return rtn;
}

int32_t set_afm_spsmd_cal_mode(sft_af_handle_t handle,uint32_t mode)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;


	SFT_AF_LOG("set_afm_spsmd_cal_mode %d",mode);
	if( ISP_CHIP_ID_TSHARK3==isp_dev_get_chip_id(ctrl_context->handle_device) ){
		isp_u_raw_afm_spsmd_square_en(ctrl_context->handle_device,mode);
	}else{
		isp_u_raw_afm_spsmd_cal_mode(ctrl_context->handle_device,mode);
	}
	return rtn;
}

int32_t set_afm_sel_filter1(sft_af_handle_t handle,uint32_t sel_filter)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	SFT_AF_LOG("sel_filter1 %d",sel_filter);
	if( ISP_CHIP_ID_TSHARK3==isp_dev_get_chip_id(ctrl_context->handle_device) ){
		af_cxt->filtertype1 = sel_filter;
	}else{
		isp_u_raw_afm_sel_filter1(ctrl_context->handle_device,sel_filter);
	}

	return rtn;
}

int32_t set_afm_sel_filter2(sft_af_handle_t handle,uint32_t sel_filter)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	SFT_AF_LOG("sel_filter2 %d",sel_filter);
	if( ISP_CHIP_ID_TSHARK3==isp_dev_get_chip_id(ctrl_context->handle_device) ){
		af_cxt->filtertype2 = sel_filter;
	}else{
		isp_u_raw_afm_sel_filter2(ctrl_context->handle_device,sel_filter);
	}

	return rtn;
}

int32_t set_afm_sobel_type(sft_af_handle_t handle,uint32_t type)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	SFT_AF_LOG("sobel_type %d",type);
	isp_u_raw_afm_sobel_type(ctrl_context->handle_device,type);

	return rtn;
}

int32_t set_afm_spsmd_type(sft_af_handle_t handle,uint32_t type)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;


	SFT_AF_LOG("spsmd_type %d",type);
	if( ISP_CHIP_ID_TSHARK3==isp_dev_get_chip_id(ctrl_context->handle_device) ){
		// average=(~type)&0x01, median = (type>>1)&0x01
		isp_u_raw_afm_subfilter(ctrl_context->handle_device,(~type)&0x01,(type>>1)&0x01);
	}else{
		isp_u_raw_afm_spsmd_type(ctrl_context->handle_device,type);
	}

	return rtn;
}

int32_t set_afm_sobel_threshold(sft_af_handle_t handle,uint32_t min, uint32_t max)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	SFT_AF_LOG("sobel_threshold %d %d",min,max);
	if( ISP_CHIP_ID_TSHARK3==isp_dev_get_chip_id(ctrl_context->handle_device) ){
		af_cxt->thr_rgb.sobel9_thr_min_red = min;
		af_cxt->thr_rgb.sobel9_thr_max_red = max;
		af_cxt->thr_rgb.sobel9_thr_min_green = min;
		af_cxt->thr_rgb.sobel9_thr_max_green = max;
		af_cxt->thr_rgb.sobel9_thr_min_blue = min;
		af_cxt->thr_rgb.sobel9_thr_max_blue = max;
		isp_u_raw_afm_threshold_rgb(ctrl_context->handle_device, (void*)&af_cxt->thr_rgb);
	}else{
		isp_u_raw_afm_sobel_threshold(ctrl_context->handle_device,min,max);
	}


	return rtn;
}

int32_t set_afm_spsmd_threshold(sft_af_handle_t handle,uint32_t min, uint32_t max)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;
	SFT_AF_LOG("spsmd_threshold %d %d",min,max);
	if( ISP_CHIP_ID_TSHARK3==isp_dev_get_chip_id(ctrl_context->handle_device) ){
		af_cxt->thr_rgb.spsmd_thr_min_red = min;
		af_cxt->thr_rgb.spsmd_thr_max_red = max;
		af_cxt->thr_rgb.spsmd_thr_min_green = min;
		af_cxt->thr_rgb.spsmd_thr_max_green = max;
		af_cxt->thr_rgb.spsmd_thr_min_blue = min;
		af_cxt->thr_rgb.spsmd_thr_max_blue = max;
		isp_u_raw_afm_threshold_rgb(ctrl_context->handle_device, (void*)&af_cxt->thr_rgb);
	}else{
		isp_u_raw_afm_spsmd_threshold(ctrl_context->handle_device,min,max);
	}

	return rtn;
}

int32_t set_afm_slice_size(sft_af_handle_t handle,uint32_t width, uint32_t height)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;


	SFT_AF_LOG("slice %d %d",width,height);
	isp_u_raw_afm_slice_size(ctrl_context->handle_device,width,height);

	return rtn;
}
int32_t set_afm_win(sft_af_handle_t handle, struct win_coord *win_range)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	uint32_t max_win_num;
	uint32_t i;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;


	isp_u_raw_afm_win_num(ctrl_context->handle_device,&max_win_num);
	SFT_AF_LOG("active_win_s =  %04x", af_cxt->active_win);
	for (i=0;i<max_win_num;i++) {
		SFT_AF_LOGV("1win[%d] %d %d %d %d",i,win_range[i].start_x,win_range[i].start_y,win_range[i].end_x,win_range[i].end_y);

		if (!(af_cxt->active_win & (1<<i))){
			win_range[i].start_x = 0;
			win_range[i].start_y = 0;
			win_range[i].end_x = 2;
			win_range[i].end_y = 2;
		}
		if ((win_range[i].start_x == 0) && (win_range[i].start_y == 0)&&(win_range[i].end_x == 0)&&(win_range[i].end_y == 0)) {
			win_range[i].start_x = 0;
			win_range[i].start_y = 0;
			win_range[i].end_x = 2;
			win_range[i].end_y = 2;


		}
		SFT_AF_LOGV("2win[%d] %d %d %d %d",i,win_range[i].start_x,win_range[i].start_y,win_range[i].end_x,win_range[i].end_y);
	}
	isp_u_raw_afm_win(ctrl_context->handle_device,(void*)win_range);

	return rtn;
}

int32_t get_afm_type1_statistic(sft_af_handle_t handle, uint32_t *statis)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	uint32_t max_win_num;
	uint32_t i;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;


	isp_u_raw_afm_win_num(ctrl_context->handle_device,&max_win_num);
	if( ISP_CHIP_ID_TSHARK3==isp_dev_get_chip_id(ctrl_context->handle_device) ){
		rtn = isp_u_raw_afm_statistic_r6p9(ctrl_context->handle_device, (void*)af_cxt->statis_tshark3);
		switch(af_cxt->filtertype1){// low 31bit value multiply 2 equals tshark2 performance
			case 0:// 0 stands for spsmd type
				for( i=0; i<5 ; i++ ){
					*(statis+2*i) = (*(af_cxt->statis_tshark3+61+6*i))<<1;// spsmd green channel
					*(statis+2*i+1) = (*(af_cxt->statis_tshark3+61+6*i+3))<<1;// spsmd green channel
				}
				break;
			case 1:// 1 stands for sobel9x9 type
				for( i=0; i<5 ; i++ ){
					*(statis+2*i) = (*(af_cxt->statis_tshark3+31+6*i))<<1;// sobel9x9 green channel
					*(statis+2*i+1) = (*(af_cxt->statis_tshark3+31+6*i+3))<<1;// sobel9x9 green channel
				}
				break;
			default:
				SFT_AF_LOGE("filtertype1 is not correct");
				break;
		}
	}else{
		isp_u_raw_afm_type1_statistic(ctrl_context->handle_device,(void*)statis);
	}

	for (i=0;i<max_win_num;i++) {
		if (!(af_cxt->active_win & (1<<i))){
			statis[i] = 0;
		}
	}
	SFT_AF_LOGV(" act win %d : %d %d %d %d %d %d %d",af_cxt->active_win,statis[0],statis[1],statis[2],statis[3],statis[4],statis[5],statis[6]);

	return rtn;
}

int32_t get_afm_type2_statistic(sft_af_handle_t handle, uint32_t *statis)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	uint32_t max_win_num;
	uint32_t i;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;


	isp_u_raw_afm_win_num(ctrl_context->handle_device,&max_win_num);
	if( ISP_CHIP_ID_TSHARK3==isp_dev_get_chip_id(ctrl_context->handle_device) ){
		rtn = isp_u_raw_afm_statistic_r6p9(ctrl_context->handle_device, (void*)af_cxt->statis_tshark3);
		switch(af_cxt->filtertype2){// low 31bit value multiply 2 equals tshark2 performance
			case 0:// 0 stands for spsmd type
				for( i=0; i<5 ; i++ ){
					*(statis+2*i) = (*(af_cxt->statis_tshark3+61+6*i))<<1;// spsmd green channel
					*(statis+2*i+1) = (*(af_cxt->statis_tshark3+61+6*i+3))<<1;// spsmd green channel
				}
				break;
			case 1:// 1 stands for sobel9x9 type
				for( i=0; i<5 ; i++ ){
					*(statis+2*i) = (*(af_cxt->statis_tshark3+31+6*i))<<1;// sobel9x9 green channel
					*(statis+2*i+1) = (*(af_cxt->statis_tshark3+31+6*i+3))<<1;// sobel9x9 green channel
				}
				break;
			default:
				SFT_AF_LOGE("filtertype2 is not correct");
				break;
		}
	}else{
		isp_u_raw_afm_type2_statistic(ctrl_context->handle_device,(void*)statis);
	}

	for (i=0;i<max_win_num;i++) {
		if (!(af_cxt->active_win & (1<<i))){
			statis[i] = 0;
		}
	}
	SFT_AF_LOGV(" act win %d : %d %d %d %d %d %d %d",af_cxt->active_win,statis[0],statis[1],statis[2],statis[3],statis[4],statis[5],statis[6]);

	return rtn;
}

int32_t set_sp_afm_cfg(sft_af_handle_t handle)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;
	struct isp_dev_yiq_afm_info_v1 afm_info;
	struct sp_afm_cfg_info *cfg_ptr = &af_cxt->sprd_filter;
	uint32_t i;

	afm_info.bypass = cfg_ptr->bypass;
	afm_info.mode= cfg_ptr->mode;
	afm_info.source_pos = cfg_ptr->source_pos;
	afm_info.shift= cfg_ptr->shift;
	afm_info.skip_num= cfg_ptr->skip_num;
	afm_info.skip_num_clear = cfg_ptr->skip_num_clear;
	afm_info.format = cfg_ptr->format;
	afm_info.iir_bypass = cfg_ptr->iir_bypass;
	afm_info.skip_num= cfg_ptr->skip_num;

	for (i=0;i<11;i++) {
		afm_info.IIR_c[i] = cfg_ptr->IIR_c[i];
	}

	isp_u_yiq_afm_block(ctrl_context->handle_device,(void*)&afm_info);

	return rtn;
}

int32_t set_sp_afm_win(sft_af_handle_t handle, struct win_coord *win_range)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	uint32_t max_win_num;
	uint32_t i;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;


	isp_u_yiq_afm_win_num(ctrl_context->handle_device,&max_win_num);
	SFT_AF_LOG("active_win_s =  %04x", af_cxt->active_win);
	for (i=0;i<max_win_num;i++) {
		SFT_AF_LOG("1win[%d] %d %d %d %d",i,win_range[i].start_x,win_range[i].start_y,win_range[i].end_x,win_range[i].end_y);

		if (!(af_cxt->active_win & (1<<i))){
			win_range[i].start_x = 0;
			win_range[i].start_y = 0;
			win_range[i].end_x = 2;
			win_range[i].end_y = 2;
		}
		SFT_AF_LOG("2win[%d] %d %d %d %d",i,win_range[i].start_x,win_range[i].start_y,win_range[i].end_x,win_range[i].end_y);
	}
	isp_u_yiq_afm_win(ctrl_context->handle_device,(void*)win_range);

	return rtn;
}

int32_t get_sp_afm_statistic(sft_af_handle_t handle, uint32_t *statis)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	uint32_t max_win_num;
	uint32_t i;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	isp_u_yiq_afm_win_num(ctrl_context->handle_device,&max_win_num);
	isp_u_yiq_afm_statistic(ctrl_context->handle_device,(void*)statis);
	for (i=0;i<max_win_num;i++) {
		if (!(af_cxt->active_win & (1<<i))){
			statis[i] = 0;
			statis[max_win_num+i] = 0;
			statis[max_win_num*2+i] = 0;
			statis[max_win_num*3+i] = 0;
		}
	}
	SFT_AF_LOG(" act win %d : %d %d %d %d %d %d %d",af_cxt->active_win,statis[0],statis[1],statis[2],statis[3],statis[4],statis[5],statis[6]);

	return rtn;
}


int32_t sp_write_i2c(sft_af_handle_t handle,uint16_t slave_addr, uint8_t *cmd, uint16_t cmd_length)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	if (NULL == ctrl_context) {
		SFT_AF_LOG("cxt is NULL error");
		return ISP_ERROR;
	} else if (NULL == ctrl_context->ioctrl_ptr) {
		SFT_AF_LOG("ioctl is NULL error");
		return ISP_ERROR;
	} else if (NULL == ctrl_context->ioctrl_ptr->write_i2c) {
		SFT_AF_LOG("write_i2c is NULL error");
		return ISP_ERROR;
	}
 
	if (0 == ctrl_context->camera_id) {
		if (NULL == ctrl_context->ioctrl_ptr->set_focus) {
			ctrl_context->ioctrl_ptr->write_i2c(slave_addr,cmd,cmd_length);
		}
	}

	return rtn;


}

int32_t sp_read_i2c(sft_af_handle_t handle,uint16_t slave_addr, uint8_t *cmd, uint16_t cmd_length)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	if (NULL == ctrl_context) {
		SFT_AF_LOG("cxt is NULL error");
		return ISP_ERROR;
 	} else if (NULL == ctrl_context->ioctrl_ptr) {
		SFT_AF_LOG("ioctl is NULL error");
		return ISP_ERROR;
	} else if (NULL == ctrl_context->ioctrl_ptr->read_i2c) {
		SFT_AF_LOG("read_i2c is NULL error");
		return ISP_ERROR;
	}
 
	if (0 == ctrl_context->camera_id) {
		if (NULL == ctrl_context->ioctrl_ptr->set_focus) {
			ctrl_context->ioctrl_ptr->read_i2c(slave_addr,cmd,cmd_length);
		} else {
			uint32_t i;
			for (i=0;i<cmd_length;i++) {
				cmd[i] = 0;
			}
		}
	}
	return rtn;


}

int32_t sp_get_cur_prv_mode(sft_af_handle_t handle,uint32_t *mode)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	if (ctrl_context->isp_mode >= ISP_MODE_ID_VIDEO_0) {
		*mode = 1;
	} else {
		*mode = 0;
	}

	return rtn;
}




int32_t set_active_win(sft_af_handle_t handle, uint32_t active_win)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	SFT_AF_LOG("active win 0x%x",active_win);
	af_cxt->active_win = active_win;
	return rtn;

}


int32_t lock_ae(sft_af_handle_t handle)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;
	struct ae_calc_out ae_result = {0};

	SFT_AF_LOG("lock_ae");
	af_cxt->ae_is_locked = 1;
	ctrl_context->ae_lib_fun->ae_io_ctrl(ctrl_context->handle_ae, AE_SET_PAUSE, NULL, (void*)&ae_result);
	return rtn;

}

int32_t lock_awb(sft_af_handle_t handle)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	SFT_AF_LOG("lock_awb");
	af_cxt->awb_is_locked = 1;
	ctrl_context->awb_lib_fun->awb_ctrl_ioctrl(ctrl_context->handle_awb, AWB_CTRL_CMD_LOCK, NULL,NULL);
	return rtn;

}

int32_t unlock_ae(sft_af_handle_t handle)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;
	struct ae_calc_out ae_result = {0};

	SFT_AF_LOG("unlock_ae");
	ctrl_context->ae_lib_fun->ae_io_ctrl(ctrl_context->handle_ae, AE_SET_RESTORE, NULL, (void*)&ae_result);
	af_cxt->ae_is_locked = 0;
	return rtn;

}

int32_t unlock_awb(sft_af_handle_t handle)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	SFT_AF_LOG("unlock_awb");
	ctrl_context->awb_lib_fun->awb_ctrl_ioctrl(ctrl_context->handle_awb, AWB_CTRL_CMD_UNLOCK, NULL,NULL);
	af_cxt->awb_is_locked = 0;
	return rtn;

}

int32_t get_cur_env_mode(sft_af_handle_t handle, uint8_t *mode)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	*mode = (uint8_t)af_cxt->cur_env_mode;

	SFT_AF_LOGV("ISP_AF:cur env mode in get_cur_env_mode is %p  ",mode);

	return rtn;
}


int32_t set_motor_pos(sft_af_handle_t handle, uint32_t pos)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	if (NULL == ctrl_context) {
		SFT_AF_LOG("cxt is NULL error");
		return ISP_ERROR;
 	} else if (NULL == ctrl_context->ioctrl_ptr) {
		SFT_AF_LOG("ioctl is NULL error");
		return ISP_ERROR;
	} else if (NULL == ctrl_context->ioctrl_ptr->set_focus) {
		SFT_AF_LOG("set_focus is NULL error");
		return ISP_ERROR;
	}

	SFT_AF_LOG("pos %d",pos);
	ctrl_context->ioctrl_ptr->set_focus(pos);
	af_cxt->cur_pos = pos;

	return rtn;
}

uint32_t get_ae_lum(sft_af_handle_t handle)
{
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	return af_cxt->ae_cur_lum;
}

uint32_t get_ae_status(sft_af_handle_t handle)
{
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	return af_cxt->ae_is_stab;
}

uint32_t get_awb_status(sft_af_handle_t handle)
{
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	return af_cxt->awb_is_stab;
}

int32_t get_isp_size(sft_af_handle_t handle, uint16_t *widith, uint16_t *height)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;
	*widith = ctrl_context->input_size_trim[ctrl_context->param_index].width;
	*height = ctrl_context->input_size_trim[ctrl_context->param_index].height;
	SFT_AF_LOG("w %d  h %d",*widith,*height);
	return rtn;
}

int32_t af_finish_notice(sft_af_handle_t handle, uint32_t result)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;
	struct isp_system* isp_system_ptr = &ctrl_context->system;

	SFT_AF_LOG("AF_TAG: move end");
	if ((ISP_ZERO == isp_system_ptr->isp_callback_bypass) && af_cxt->is_runing) {
		struct isp_af_notice af_notice = {0x00};
		af_notice.mode=ISP_FOCUS_MOVE_END;
		af_notice.valid_win = result?1:0;
		SFT_AF_LOGD("callback ISP_AF_NOTICE_CALLBACK");
		isp_system_ptr->callback(isp_system_ptr->caller_id, AF_CALLBACK_EVT|ISP_AF_NOTICE_CALLBACK, (void*)&af_notice, sizeof(struct isp_af_notice));
	}
	unlock_ae(handle);// because sft lock ae before af but not unlock it after af
	unlock_awb(handle);
	af_cxt->is_runing = 0;

	return rtn;
}

int32_t af_move_start_notice(sft_af_handle_t handle)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;
	struct isp_system* isp_system_ptr = &ctrl_context->system;

	SFT_AF_LOG("AF_TAG: move start");
	if ((ISP_ZERO == isp_system_ptr->isp_callback_bypass) && (0 == af_cxt->is_runing) && (1 == af_cxt->caf_active)) {
		struct isp_af_notice af_notice = {0x00};
		af_notice.mode=ISP_FOCUS_MOVE_START;
		af_notice.valid_win = 0;
		SFT_AF_LOGD("callback ISP_AF_NOTICE_CALLBACK");
		isp_system_ptr->callback(isp_system_ptr->caller_id, AF_CALLBACK_EVT|ISP_AF_NOTICE_CALLBACK, (void*)&af_notice, sizeof(struct isp_af_notice));
	}
	af_cxt->is_runing = 1;

	return rtn;
}

int32_t af_pos_update(sft_af_handle_t handle, uint32_t pos)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	if (NULL == ctrl_context) {
		SFT_AF_LOG("cxt is NULL error");
		rtn = ISP_ERROR;
	} else if (NULL == ctrl_context->ioctrl_ptr) {
		SFT_AF_LOG("ioctl is NULL error");
		rtn = ISP_ERROR;
	} else if (NULL == ctrl_context->ioctrl_ptr->set_focus) {
		SFT_AF_LOG("set_focus is NULL error");
		rtn = ISP_ERROR;
	} else {
		ctrl_context->ioctrl_ptr->set_focus(pos);
	}

	SFT_AF_LOG("pos %d",pos);
	af_cxt->cur_pos = pos;

	return rtn;
}

// for sharkLT8 sharkLS PikeB SFT Tuning data
void sft_load_tuning_param(void* isp_handle,uint32_t update){
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)isp_handle;
	struct sft_af_context *af_cxt = ctrl_context->handle_af;

	struct isp_pm_ioctl_input af_pm_input;
	struct isp_pm_ioctl_output af_pm_output;
	struct sft_tuning_param* sft_param=NULL;
	uint32_t i=0;

	if( 0==update ){
		goto DEFAULT;
	}

	memset((void*)&af_pm_input, 0, sizeof(af_pm_input));
	memset((void*)&af_pm_output, 0, sizeof(af_pm_output));

	isp_pm_ioctl(ctrl_context->handle_pm, ISP_PM_CMD_GET_THIRD_PART_INIT_SFT_AF, &af_pm_input, &af_pm_output);
	// common mode is the first mode in the list "af_pm_output.param_data" and sft tuning param exist in common mode
	if( NULL==af_pm_output.param_data || 0==af_pm_output.param_num ){
		SFT_AF_LOG("There exist no sft param %d",af_pm_output.param_num);
		goto DEFAULT;
	}
	i=0;
	while( i<af_pm_output.param_num ){
		if( i==ctrl_context->isp_mode ) {
			if( sizeof(struct sft_tuning_param)!=af_pm_output.param_data[i].data_size ){
				if( 0==i ){// the common mode
					SFT_AF_LOG("in common mode,exists wrong SFT AF tuning param 1st");
					goto DEFAULT;
				}
				SFT_AF_LOG("in the current isp mode %d,exists wrong SFT AF tuning param",ctrl_context->isp_mode);
				// load from common mode
				if( sizeof(struct sft_tuning_param)!=af_pm_output.param_data[0].data_size ){
					SFT_AF_LOG("in common mode,exists wrong SFT AF tuning param 2nd");
					goto DEFAULT;
				}
				sft_param = (struct sft_tuning_param*)af_pm_output.param_data[0].data_ptr;
				SFT_AF_LOG("load param from common isp mode");
				break;
			}
			sft_param = (struct sft_tuning_param*)af_pm_output.param_data[i].data_ptr;
			SFT_AF_LOG("load param from isp mode %d",i);
			break;
		}
		i++;
	}

	if( NULL==sft_param ){
		SFT_AF_LOG("no tuning data,isp mode %d",ctrl_context->isp_mode);
		goto DEFAULT;
	}

	if( 1==sft_param->bypass ){
		SFT_AF_LOG("tuning data bypass");
		goto DEFAULT;
	}
	memcpy(af_cxt->bv_thr,sft_param->bv_thr,sizeof(sft_param->bv_thr));
	memcpy(af_cxt->rgbdiff_thr,sft_param->rgbdiff_thr,sizeof(sft_param->rgbdiff_thr));
	memcpy(&af_cxt->afCtl,&sft_param->afCtl,sizeof(AFDATA));

	return;
DEFAULT:
	af_cxt->bv_thr[0] = 151;
	af_cxt->bv_thr[1] = 65;
	af_cxt->rgbdiff_thr[0] = 16;
	af_cxt->rgbdiff_thr[1] = 16;
	af_cxt->rgbdiff_thr[2] = 16;
	return;
}

sft_af_handle_t sft_af_init(void* isp_handle)
{
	struct sft_af_context *af_cxt = NULL;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)isp_handle;

	isp_u_raw_afm_bypass(ctrl_context->handle_device,1);

	if (1 == ctrl_context->camera_id) {
		SFT_AF_LOGE("Front camera nor support AF!");
		return NULL;
	}

	af_cxt = (struct sft_af_context *)malloc(sizeof(struct sft_af_context));
	if (NULL == af_cxt) {
		return NULL;
	}
	SFT_AF_LOG("S");

	memset(af_cxt,0,sizeof(struct sft_af_context));
	af_cxt->magic_start = SFT_AF_MAGIC_START;
	af_cxt->active_win = 0;
	af_cxt->isp_handle = isp_handle;
	af_cxt->ae_cur_lum = 128;
	af_cxt->ae_is_locked = 0;
	af_cxt->ae_is_stab = 1;
	af_cxt->awb_is_stab = 1;
	af_cxt->awb_is_locked = 0;
	af_cxt->is_runing = 0;
	af_cxt->caf_active = 0;
	af_cxt->flash_on = 0;
	af_cxt->af_mode = 0;
	af_cxt->fd_info.type = 0;
	af_cxt->fd_info.face_num = 0;
	af_cxt->cur_ae_again = 0;
	af_cxt->cur_ae_bv = 0;

	af_cxt->af_ctrl_ops.cb_set_afm_bypass                    = set_afm_bypass;
	af_cxt->af_ctrl_ops.cb_set_afm_mode                      = set_afm_mode;
	af_cxt->af_ctrl_ops.cb_set_afm_skip_num                  = set_afm_skip_num;
	af_cxt->af_ctrl_ops.cb_set_afm_skip_num_clr              = set_afm_skip_num_clr;
	af_cxt->af_ctrl_ops.cb_set_afm_spsmd_rtgbot_enable       = set_afm_spsmd_rtgbot_enable;
	af_cxt->af_ctrl_ops.cb_set_afm_spsmd_diagonal_enable     = set_afm_spsmd_diagonal_enable;
	af_cxt->af_ctrl_ops.cb_set_afm_spsmd_cal_mode            = set_afm_spsmd_cal_mode;
	af_cxt->af_ctrl_ops.cb_set_afm_sel_filter1               = set_afm_sel_filter1;
	af_cxt->af_ctrl_ops.cb_set_afm_sel_filter2               = set_afm_sel_filter2;
	af_cxt->af_ctrl_ops.cb_set_afm_sobel_type                = set_afm_sobel_type;
	af_cxt->af_ctrl_ops.cb_set_afm_spsmd_type                = set_afm_spsmd_type;
	af_cxt->af_ctrl_ops.cb_set_afm_sobel_threshold           = set_afm_sobel_threshold;
	af_cxt->af_ctrl_ops.cb_set_afm_spsmd_threshold           = set_afm_spsmd_threshold;
	af_cxt->af_ctrl_ops.cb_set_afm_slice_size                = set_afm_slice_size;
	af_cxt->af_ctrl_ops.cb_set_afm_win                       = set_afm_win;
	af_cxt->af_ctrl_ops.cb_get_afm_type1_statistic           = get_afm_type1_statistic;
	af_cxt->af_ctrl_ops.cb_get_afm_type2_statistic           = get_afm_type2_statistic;
	af_cxt->af_ctrl_ops.cb_set_active_win                    = set_active_win;
	af_cxt->af_ctrl_ops.cb_get_cur_env_mode                  = get_cur_env_mode;
	af_cxt->af_ctrl_ops.cb_set_motor_pos                     = set_motor_pos;
	af_cxt->af_ctrl_ops.cb_lock_ae                           = lock_ae;
	af_cxt->af_ctrl_ops.cb_lock_awb                          = lock_awb;
	af_cxt->af_ctrl_ops.cb_unlock_ae                         = unlock_ae;
	af_cxt->af_ctrl_ops.cb_unlock_awb                        = unlock_awb;
	af_cxt->af_ctrl_ops.cb_get_ae_lum                        = get_ae_lum;
	af_cxt->af_ctrl_ops.cb_get_ae_status                     = get_ae_status;
	af_cxt->af_ctrl_ops.cb_get_awb_status                    = get_awb_status;
	af_cxt->af_ctrl_ops.cb_get_isp_size                      = get_isp_size;
	af_cxt->af_ctrl_ops.cb_af_finish_notice                  = af_finish_notice;
	af_cxt->af_ctrl_ops.cb_sft_af_log                        = sft_af_log;
	af_cxt->af_ctrl_ops.cb_set_sp_afm_cfg                    = set_sp_afm_cfg;
	af_cxt->af_ctrl_ops.cb_set_sp_afm_win                    = set_sp_afm_win;
	af_cxt->af_ctrl_ops.cb_get_sp_afm_statistic              = get_sp_afm_statistic;
	af_cxt->af_ctrl_ops.cb_sp_write_i2c                      = sp_write_i2c;
	af_cxt->af_ctrl_ops.cd_sp_read_i2c                       = sp_read_i2c;
	af_cxt->af_ctrl_ops.cd_sp_get_cur_prv_mode               = sp_get_cur_prv_mode;
	af_cxt->af_ctrl_ops.cb_af_move_start_notice              = af_move_start_notice;
	af_cxt->af_ctrl_ops.cb_af_pos_update                     = af_pos_update;

	af_cxt->magic_end = SFT_AF_MAGIC_END;
	ctrl_context->handle_af = af_cxt;
	if( ISP_CHIP_ID_TSHARK3==isp_dev_get_chip_id(ctrl_context->handle_device) ){
		af_cxt->thr_rgb.sobel5_thr_min_red = 0x0;
		af_cxt->thr_rgb.sobel5_thr_max_red = 0xffff;
		af_cxt->thr_rgb.sobel5_thr_min_green = 0x0;
		af_cxt->thr_rgb.sobel5_thr_max_green = 0xffff;
		af_cxt->thr_rgb.sobel5_thr_min_blue = 0x0;
		af_cxt->thr_rgb.sobel5_thr_max_blue = 0xffff;
		af_cxt->thr_rgb.sobel9_thr_min_red = 0x0;
		af_cxt->thr_rgb.sobel9_thr_max_red = 0xffff;
		af_cxt->thr_rgb.sobel9_thr_min_green = 0x0;
		af_cxt->thr_rgb.sobel9_thr_max_green = 0xffff;
		af_cxt->thr_rgb.sobel9_thr_min_blue = 0x0;
		af_cxt->thr_rgb.sobel9_thr_max_blue = 0xffff;
		af_cxt->thr_rgb.spsmd_thr_min_red = 0x0;
		af_cxt->thr_rgb.spsmd_thr_max_red = 0xffff;
		af_cxt->thr_rgb.spsmd_thr_min_green = 0x0;
		af_cxt->thr_rgb.spsmd_thr_max_green = 0xffff;
		af_cxt->thr_rgb.spsmd_thr_min_blue = 0x0;
		af_cxt->thr_rgb.spsmd_thr_max_blue = 0xffff;
		// default threshold

		isp_u_raw_afm_bypass(ctrl_context->handle_device,1);
		isp_u_raw_afm_skip_num_clr(ctrl_context->handle_device,1);
		isp_u_raw_afm_skip_num_clr(ctrl_context->handle_device,0);
		isp_u_raw_afm_skip_num(ctrl_context->handle_device,0);
		isp_u_raw_afm_spsmd_rtgbot_enable(ctrl_context->handle_device,1);
		isp_u_raw_afm_spsmd_diagonal_enable(ctrl_context->handle_device,1);
		isp_u_raw_afm_spsmd_square_en(ctrl_context->handle_device,1);
		// open more than 32bit mode
		isp_u_raw_afm_overflow_protect(ctrl_context->handle_device,0);
		isp_u_raw_afm_subfilter(ctrl_context->handle_device,0,1);
		isp_u_raw_afm_spsmd_touch_mode(ctrl_context->handle_device,0);
		isp_u_raw_afm_shfit(ctrl_context->handle_device,0,0,0);

		isp_u_raw_afm_threshold_rgb(ctrl_context->handle_device, (void*)&af_cxt->thr_rgb);
		isp_u_raw_afm_mode(ctrl_context->handle_device,1);
	}
	AF_SetDefault((sft_af_handle_t)af_cxt);
	sft_load_tuning_param(isp_handle,1);
	return (sft_af_handle_t)af_cxt;
}

int32_t sft_af_deinit(void* isp_handle)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)isp_handle;
	struct sft_af_context *af_cxt = (struct sft_af_context *)ctrl_context->handle_af;

	rtn = _check_handle(af_cxt);
	if (SFT_AF_SUCCESS != rtn) {
		SFT_AF_LOG("_check_cxt failed");
		return SFT_AF_SUCCESS;
	}
	SFT_AF_LOG("S");
	AF_Deinit_Notice(af_cxt);
	memset(af_cxt,0,sizeof(*af_cxt));
	free(af_cxt);
	return rtn;
}

int32_t sft_af_calc(isp_ctrl_context* handle)
{

	_sft_af_calc(handle->handle_af);

	return 0;
}

int32_t _sft_af_calc(sft_af_handle_t handle)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;

	rtn = _check_handle(handle);
	if (SFT_AF_SUCCESS != rtn) {
		SFT_AF_LOG("_check_cxt failed");
		return SFT_AF_SUCCESS;
	}

	SFT_AF_LOGV("af runing");
	AF_Running(handle);
	return rtn;
}
struct isp_parser_buf_rtn{
	unsigned long buf_addr;
	uint32_t buf_len;
};
int32_t sft_af_ioctrl(sft_af_handle_t handle, enum sft_af_cmd cmd,
				void *param0, void *param1)
{
	uint32_t rtn = SFT_AF_SUCCESS;
	struct sft_af_context *af_cxt = (struct sft_af_context *)handle;
	struct isp_parser_buf_rtn *rtn_buf = (struct isp_parser_buf_rtn*)param0;
	int32_t *bv = (int32_t *)param1;

	rtn = _check_handle(handle);
	if (SFT_AF_SUCCESS != rtn) {
		SFT_AF_LOG("_check_cxt failed");
		return SFT_AF_SUCCESS;
	}

	switch (cmd) {
	case SFT_AF_CMD_GET_AF_INFO:
		SFT_AF_LOG("Chj--_ispSFTIORead in");
		memmove((uint8_t*)rtn_buf->buf_addr,(uint8_t*)&af_cxt->afCtl,sizeof(AFDATA));
		rtn_buf->buf_len = sizeof(AFDATA);
		memmove((uint8_t*)rtn_buf->buf_addr+rtn_buf->buf_len,(uint8_t*)&af_cxt->afv,sizeof(AFDATA_RO));
		rtn_buf->buf_len+=sizeof(AFDATA_RO);
		memmove((uint8_t*)rtn_buf->buf_addr+rtn_buf->buf_len,(uint8_t*)&af_cxt->fv,sizeof(AFDATA_FV));
		rtn_buf->buf_len+=sizeof(AFDATA_FV);
		SFT_AF_LOG("Chj--_ispSFTIORead exit");
		break;
	case SFT_AF_CMD_SET_AF_INFO:
		rtn_buf->buf_len-=sizeof(AFDATA_FV);
		memmove((uint8_t*)&af_cxt->fv,(uint8_t*)rtn_buf->buf_addr+rtn_buf->buf_len,sizeof(AFDATA_FV));
		rtn_buf->buf_len-=sizeof(AFDATA_RO);
		memmove((uint8_t*)&af_cxt->afv,(uint8_t*)rtn_buf->buf_addr+rtn_buf->buf_len,sizeof(AFDATA_RO));
		rtn_buf->buf_len -= sizeof(AFDATA);
		memmove((uint8_t*)&af_cxt->afCtl,(uint8_t*)rtn_buf->buf_addr+rtn_buf->buf_len,sizeof(AFDATA));
		break;
	case SFT_AF_CMD_GET_AF_VALUE://added for sft
		af_cxt->af_ctrl_ops.cb_get_afm_type1_statistic(handle,(uint32_t *)param0);
		af_cxt->af_ctrl_ops.cb_get_afm_type2_statistic(handle,(uint32_t *)((uint32_t*)param0+25));
		break;
	case SFT_AF_CMD_SET_AF_MODE:{
		uint32_t af_mode = *(uint32_t*)param0;
		if (0 == af_cxt->flash_on) {
			AF_Set_Mode(handle,af_mode);
			if (((SFT_AF_MODE_CAF == af_mode) || (SFT_AF_MODE_VIDEO_CAF == af_mode))
				&& (0 == af_cxt->caf_active)) {
				AF_Control_CAF(&af_cxt->afCtl,&af_cxt->afv,&af_cxt->fv,1);
				af_cxt->caf_active = 1;
				SFT_AF_LOG("af mode %d caf act %d",af_mode,af_cxt->caf_active);
			} else if ((SFT_AF_MODE_CAF != af_mode) && (SFT_AF_MODE_VIDEO_CAF != af_mode)) {
				AF_Control_CAF(&af_cxt->afCtl,&af_cxt->afv,&af_cxt->fv,0);
				af_cxt->caf_active = 0;
				SFT_AF_LOG("af mode %d caf act %d",af_mode,af_cxt->caf_active);
			}
			af_cxt->af_mode = af_mode;
		} else {
			SFT_AF_LOGI("Flash is on, af mode do not change!");
		}
		break;
	}

	case SFT_AF_CMD_SET_AF_POS:
		set_motor_pos(handle,*(uint32_t*)param0);
		break;

	case SFT_AF_CMD_SET_SCENE_MODE:
		break;

	case SFT_AF_CMD_SET_AF_START: {
		struct isp_af_win* af_ptr = (struct isp_af_win*)param0;
		uint32_t af_mode = af_cxt->af_mode;
		uint32_t i=0;
		SFT_AF_LOG("af start");

		if (((SFT_AF_MODE_CAF == af_mode) || (SFT_AF_MODE_VIDEO_CAF == af_mode))
			&& (0 == af_cxt->flash_on)
			&& (0 == af_ptr->valid_win)) {
			if (af_cxt->is_runing) {
				break;
			} else {
				af_cxt->is_runing = 1;
				af_finish_notice(handle,1);
				break;
			}
		}

		if (af_ptr) {
			af_cxt->touch_win_cnt = af_ptr->valid_win;
			for (i=0; i<af_ptr->valid_win; i++) {
				af_cxt->win_pos[i].start_x = af_ptr->win[i].start_x;
				af_cxt->win_pos[i].start_y = af_ptr->win[i].start_y;
				af_cxt->win_pos[i].end_x = af_ptr->win[i].end_x;
				af_cxt->win_pos[i].end_y = af_ptr->win[i].end_y;
			}
		} else {
			af_cxt->touch_win_cnt = 0;
		}

		AF_Start(handle, af_cxt->touch_win_cnt ? 1 : 0);
		af_cxt->is_runing = 1;
		break;

	}

	case SFT_AF_CMD_SET_AF_STOP:
		SFT_AF_LOG("af stop");
		unlock_ae(handle);
		unlock_awb(handle);
		AF_Stop(handle);
		af_cxt->is_runing = 1;
		af_finish_notice(handle,0);
		break;

	case SFT_AF_CMD_SET_AF_BYPASS:
		SFT_AF_LOG("af bypass %d",*(uint32_t*)param0);
		af_cxt->bypass= *(uint32_t*)param0;
		if (af_cxt->bypass) {
			AF_Stop(handle);
			af_cxt->is_runing = 0;
		}
		break;

	case SFT_AF_CMD_SET_ISP_START_INFO:
		AF_Prv_Start_Notice(handle);
		AF_Start_Debug(handle);
		break;

	case SFT_AF_CMD_SET_ISP_STOP_INFO:
		AF_Stop_Debug(handle);
		break;

	case SFT_AF_CMD_SET_AE_INFO:{
		struct ae_calc_out *ae_result = (struct ae_calc_out*)param0;

		af_cxt->ae_cur_lum = ae_result->cur_lum;
		af_cxt->ae_is_stab = ae_result->is_stab;
		//SFT_AF_LOG("cur exp %d  line_time %d",ae_result->cur_exp_line,ae_result->line_time);
		SFT_AF_LOGV("ae_is_stab %d cur lum %d ",ae_result->is_stab,af_cxt->ae_cur_lum);
		if (ae_result->cur_exp_line && ae_result->line_time)
			af_cxt->cur_fps = 100000000/(ae_result->cur_exp_line*ae_result->line_time);
		//SFT_AF_LOG("cur fps %d  ",af_cxt->cur_fps);
		af_cxt->cur_ae_again = ae_result->cur_again;
		af_cxt->cur_ae_bv = *bv;
		//SFT_AF_LOG("cur again %d  ",af_cxt->cur_ae_again);

		if (*bv >= af_cxt->bv_thr[0]){
			af_cxt->cur_env_mode = 0;
		}else if (*bv >= af_cxt->bv_thr[1] && *bv < af_cxt->bv_thr[0]) {
			af_cxt->cur_env_mode = 1;
		}else {
			af_cxt->cur_env_mode = 2;
		}

		//SFT_AF_LOG("ISP_AF:cur bv in ioctrl is %d  ",*bv);
		//SFT_AF_LOG("ISP_AF:cur env mode in ioctrl is %d  ",af_cxt->cur_env_mode);
		break;
	}

	case SFT_AF_CMD_SET_AWB_INFO:{
		struct awb_ctrl_calc_result *awb_result = (struct awb_ctrl_calc_result*)param0;
		uint32_t r_diff;
		uint32_t g_diff;
		uint32_t b_diff;

		r_diff = awb_result->gain.r > af_cxt->cur_awb_r_gain ? awb_result->gain.r - af_cxt->cur_awb_r_gain : af_cxt->cur_awb_r_gain - awb_result->gain.r;
		g_diff = awb_result->gain.g > af_cxt->cur_awb_g_gain ? awb_result->gain.g - af_cxt->cur_awb_g_gain : af_cxt->cur_awb_g_gain - awb_result->gain.g;
		b_diff = awb_result->gain.b > af_cxt->cur_awb_b_gain ? awb_result->gain.b - af_cxt->cur_awb_b_gain : af_cxt->cur_awb_b_gain - awb_result->gain.b;

		//SFT_AF_LOG("awb_is_stab r_diff %d g_diff %d  b_diff %d",r_diff,g_diff,b_diff);
		if ((r_diff <= af_cxt->rgbdiff_thr[0])
			&& (g_diff <= af_cxt->rgbdiff_thr[1])
			&& (b_diff <= af_cxt->rgbdiff_thr[2])) {
			af_cxt->awb_is_stab = 1;
		} else {
			af_cxt->awb_is_stab = 0;
		}
		af_cxt->cur_awb_r_gain = awb_result->gain.r;
		af_cxt->cur_awb_g_gain = awb_result->gain.g;
		af_cxt->cur_awb_b_gain = awb_result->gain.b;
		SFT_AF_LOGV("awb_is_stab %d ",af_cxt->awb_is_stab);

		break;
	}

	case SFT_AF_CMD_SET_FLASH_NOTICE:{
		uint32_t flash_status;
		uint32_t af_mode = af_cxt->af_mode;

		flash_status = *(uint32_t*)param0;

		switch (flash_status) {
		case ISP_FLASH_PRE_BEFORE:
		case ISP_FLASH_PRE_LIGHTING:
		case ISP_FLASH_MAIN_BEFORE:
		case ISP_FLASH_MAIN_LIGHTING:
			if (((SFT_AF_MODE_CAF == af_mode) || (SFT_AF_MODE_VIDEO_CAF == af_mode))
				&& (1 == af_cxt->caf_active)){
				AF_Control_CAF(&af_cxt->afCtl,&af_cxt->afv,&af_cxt->fv,0);
				af_cxt->caf_active = 0;
				SFT_AF_LOG("falsh: af mode %d caf act %d",af_mode,af_cxt->caf_active);
			}
			af_cxt->flash_on = 1;
			break;

		case ISP_FLASH_PRE_AFTER:
		case ISP_FLASH_MAIN_AFTER:
			if (((SFT_AF_MODE_CAF == af_mode) || (SFT_AF_MODE_VIDEO_CAF == af_mode))
				&& (0 == af_cxt->caf_active)){
				AF_Control_CAF(&af_cxt->afCtl,&af_cxt->afv,&af_cxt->fv,1);
				af_cxt->caf_active = 1;
				SFT_AF_LOG("falsh: af mode %d caf act %d",af_mode,af_cxt->caf_active);
			}
			af_cxt->flash_on = 0;
			break;
		default:
			break;

		}


		break;
	}


	case SFT_AF_CMD_SET_FD_UPDATE:{
		struct isp_face_area *face_area = (struct isp_face_area*)param0;
		uint32_t i;

		af_cxt->fd_info.type = face_area->type;
		af_cxt->fd_info.face_num = face_area->face_num;
		af_cxt->fd_info.frame_width = face_area->frame_width;
		af_cxt->fd_info.frame_height = face_area->frame_height;

		SFT_AF_LOG("face_num %d",af_cxt->fd_info.face_num);

		for (i=0; i<10; i++) {
			af_cxt->fd_info.face_info[i].sx = face_area->face_info[i].sx;
			af_cxt->fd_info.face_info[i].sy = face_area->face_info[i].sy;
			af_cxt->fd_info.face_info[i].ex = face_area->face_info[i].ex;
			af_cxt->fd_info.face_info[i].ey = face_area->face_info[i].ey;
			af_cxt->fd_info.face_info[i].brightness = face_area->face_info[i].brightness;
			af_cxt->fd_info.face_info[i].pose = face_area->face_info[i].pose;
		}


		break;
	}

	case SFT_AF_CMD_SET_AF_PARAM:
		//sft_load_tuning_param(af_cxt->isp_handle,1);
		break;



	case SFT_AF_CMD_GET_AF_MODE:
		*(uint32_t*)param0 = af_cxt->af_mode;
		break;

	case SFT_AF_CMD_GET_AF_CUR_POS:
		*(uint32_t*)param0 = af_cxt->cur_pos;
		break;
	case SFT_AF_CMD_BURST_NOTICE:
		AF_Control_CAF(&af_cxt->afCtl,&af_cxt->afv,&af_cxt->fv,*(uint32_t*)param0);
		break;
	default:
		break;
	}

	return rtn;
}

int32_t sft_af_ioctrl_set_af_mode(isp_handle isp_handler, void* param_ptr, int(*call_back)())
{
	isp_ctrl_context* handle = (isp_ctrl_context*)isp_handler;
	uint32_t set_mode;
	uint32_t rtn;

	switch (*(uint32_t *)param_ptr) {
	case ISP_FOCUS_MACRO:
		set_mode = SFT_AF_MODE_MACRO;
		break;
	case ISP_FOCUS_MANUAL:
		set_mode = SFT_AF_MODE_MANUAL;
		break;
	case ISP_FOCUS_CONTINUE:
		set_mode = SFT_AF_MODE_CAF;
		break;
	case ISP_FOCUS_VIDEO:
		set_mode = SFT_AF_MODE_VIDEO_CAF;
		break;
	default:
		set_mode = AF_MODE_NORMAL;
		break;
	}

	rtn = sft_af_ioctrl(handle->handle_af, SFT_AF_CMD_SET_AF_MODE,(void*)&set_mode,NULL);

	return rtn;
}

int32_t sft_af_ioctrl_set_flash_notice(isp_ctrl_context* handle, void* param_ptr, int(*call_back)())
{
	struct isp_flash_notice *flash_notice = (struct isp_flash_notice*)param_ptr;
	int32_t rtn;
	rtn = sft_af_ioctrl(handle->handle_af,SFT_AF_CMD_SET_FLASH_NOTICE,(void *)&(flash_notice->mode),NULL);

	return rtn;
}

int32_t sft_af_ioctrl_get_af_mode(isp_handle isp_handler, void* param_ptr, int(*call_back)())
{
	isp_ctrl_context *handle = (isp_ctrl_context*)isp_handler;
	uint32_t param = 0;
	uint32_t rtn;

	rtn = sft_af_ioctrl(handle->handle_af, SFT_AF_CMD_GET_AF_MODE, (void*)&param,NULL);

	switch (param) {
	case SFT_AF_MODE_NORMAL:
		*(uint32_t*)param_ptr = ISP_FOCUS_TRIG;
		break;
	case SFT_AF_MODE_MACRO:
		*(uint32_t*)param_ptr = ISP_FOCUS_MACRO;
		break;
	case SFT_AF_MODE_MANUAL:
		*(uint32_t*)param_ptr  = ISP_FOCUS_MANUAL;
		break;
	case SFT_AF_MODE_CAF:
		*(uint32_t*)param_ptr  = ISP_FOCUS_CONTINUE;
		break;
	case SFT_AF_MODE_VIDEO_CAF:
		*(uint32_t*)param_ptr  = ISP_FOCUS_VIDEO;
		break;
	default:
		*(uint32_t*)param_ptr = ISP_FOCUS_TRIG;
		break;
	}

	ISP_LOGI("rtn %d param %d af %d", rtn, param, *(uint32_t*)param_ptr);

	return rtn;
}

int32_t sft_af_ioctrl_af_start(isp_handle isp_handler, void* param_ptr, int(*call_back)())
{
	int32_t rtn = 0;
	isp_ctrl_context* handle = (isp_ctrl_context*)isp_handler;
	rtn = sft_af_ioctrl(handle->handle_af,SFT_AF_CMD_SET_AF_START,param_ptr,NULL);
	return rtn;
}

int32_t sft_af_ioctrl_set_fd_update(isp_handle isp_handler, void* param_ptr, int(*call_back)())
{
	int32_t rtn = 0;
	isp_ctrl_context* handle = (isp_ctrl_context*)isp_handler;
	struct isp_face_area *face_area = (struct isp_face_area*)param_ptr;
	if (face_area) {
		rtn = sft_af_ioctrl(handle->handle_af,SFT_AF_CMD_SET_FD_UPDATE,(void *)face_area,NULL);

	}
	return rtn;
}

int32_t sft_af_ioctrl_set_isp_start_info(isp_handle isp_handler, struct isp_video_start* param_ptr)
{
	int rtn = 0;
	isp_ctrl_context* handle = (isp_ctrl_context*)isp_handler;

	if (handle->handle_af && ((ISP_VIDEO_MODE_CONTINUE == param_ptr->mode))) {
		rtn = sft_af_ioctrl(handle->handle_af,SFT_AF_CMD_SET_ISP_START_INFO,NULL,NULL);
	}

	return rtn;
}

int32_t sft_af_ioctrl_set_isp_stop_info(isp_handle isp_handler)
{
	int rtn = ISP_SUCCESS;
	isp_ctrl_context* handle = (isp_ctrl_context*)isp_handler;
	if (handle->handle_af) {
		rtn = sft_af_ioctrl(handle->handle_af,SFT_AF_CMD_SET_ISP_STOP_INFO,NULL,NULL);
	}

	return rtn;
}

int32_t sft_af_ioctrl_set_ae_awb_info(isp_ctrl_context* handle,
		void* ae_result,
		void* awb_result,
		void* bv,
		void *rgb_statistics)
{
	int rtn = 0;
	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_BV_BY_LUM, NULL, (void *)bv);
	rtn = sft_af_ioctrl(handle->handle_af,SFT_AF_CMD_SET_AE_INFO,(void*)ae_result,(void*)bv);
	rtn = sft_af_ioctrl(handle->handle_af,SFT_AF_CMD_SET_AWB_INFO,(void*)awb_result,NULL);

	return rtn;
}

int32_t sft_af_ioctrl_burst_notice(isp_handle isp_handler, void* param_ptr, int(*call_back)())
{
	isp_ctrl_context* handle = (isp_ctrl_context*)isp_handler;
	int32_t rtn = ISP_SUCCESS;
	rtn = sft_af_ioctrl(handle->handle_af,SFT_AF_CMD_BURST_NOTICE,param_ptr,NULL);

	return rtn;
}

int32_t sft_af_ioctrl_get_af_value(isp_handle isp_handler, void* param_ptr, int(*call_back)())
{
	int32_t rtn = 0;
	isp_ctrl_context* handle = (isp_ctrl_context*)isp_handler;
	rtn = sft_af_ioctrl(handle->handle_af,SFT_AF_CMD_GET_AF_VALUE,param_ptr,NULL);

	return rtn;
}

int32_t sft_af_ioctrl_iowrite(isp_handle isp_handler, void* param_ptr, int(*call_back)())
{
	int32_t rtn = 0;
	isp_ctrl_context* handle = (isp_ctrl_context*)isp_handler;
	rtn = sft_af_ioctrl(handle->handle_af,SFT_AF_CMD_SET_AF_INFO,param_ptr,NULL);

	return rtn;
}

int32_t sft_af_ioctrl_ioread(isp_handle isp_handler, void* param_ptr, int(*call_back)())
{
	int32_t rtn = 0;
	isp_ctrl_context* handle = (isp_ctrl_context*)isp_handler;
	rtn = sft_af_ioctrl(handle->handle_af,SFT_AF_CMD_GET_AF_INFO,param_ptr,NULL);

	return rtn;
}

int32_t sft_af_ioctrl_get_af_cur_pos(isp_ctrl_context* handle, void* param_ptr, int(*call_back)())
{
	int32_t rtn;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)handle;
	rtn = sft_af_ioctrl(ctrl_context->handle_af,SFT_AF_CMD_GET_AF_CUR_POS,param_ptr,NULL);

	return rtn;
}

int32_t sft_af_ioctrl_set_af_pos(isp_ctrl_context* handle, void* param_ptr, int(*call_back)())
{
	int32_t rtn;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)handle;
	rtn = sft_af_ioctrl(ctrl_context->handle_af,SFT_AF_CMD_SET_AF_POS,param_ptr,NULL);

	return rtn;
}

int32_t sft_af_ioctrl_set_af_bypass(isp_ctrl_context* handle, void* param_ptr, int(*call_back)())
{
	int32_t rtn;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)handle;
	rtn = sft_af_ioctrl(ctrl_context->handle_af,SFT_AF_CMD_SET_AF_BYPASS,param_ptr,NULL);

	return rtn;
}

int32_t sft_af_ioctrl_set_af_stop(isp_ctrl_context* handle, void* param_ptr, int(*call_back)())
{
	int32_t rtn;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)handle;
	rtn = sft_af_ioctrl(ctrl_context->handle_af, SFT_AF_CMD_SET_AF_STOP, NULL, NULL);

	return rtn;
}

int32_t sft_af_ioctrl_set_af_param(isp_ctrl_context* handle)
{
	int32_t rtn;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)handle;
	rtn = sft_af_ioctrl(ctrl_context->handle_af, SFT_AF_CMD_SET_AF_PARAM, NULL, NULL);

	return rtn;
}

extern struct af_lib_fun af_lib_fun; 
void sft_af_fun_init()
{
	af_lib_fun.af_init_interface		= sft_af_init;
	af_lib_fun.af_calc_interface		= sft_af_calc;
	af_lib_fun.af_deinit_interface		= sft_af_deinit;
	af_lib_fun.af_ioctrl_interface		= sft_af_ioctrl;
	af_lib_fun.af_ioctrl_set_flash_notice	= sft_af_ioctrl_set_flash_notice;
	af_lib_fun.af_ioctrl_get_af_value	= sft_af_ioctrl_get_af_value;
	af_lib_fun.af_ioctrl_burst_notice	= sft_af_ioctrl_burst_notice;
	af_lib_fun.af_ioctrl_set_af_mode	= sft_af_ioctrl_set_af_mode;
	af_lib_fun.af_ioctrl_get_af_mode	= sft_af_ioctrl_get_af_mode;
	af_lib_fun.af_ioctrl_ioread		= sft_af_ioctrl_ioread;
	af_lib_fun.af_ioctrl_iowrite		= sft_af_ioctrl_iowrite;
	af_lib_fun.af_ioctrl_set_fd_update	= sft_af_ioctrl_set_fd_update;
	af_lib_fun.af_ioctrl_af_start		= sft_af_ioctrl_af_start;
	af_lib_fun.af_ioctrl_set_isp_start_info	= sft_af_ioctrl_set_isp_start_info;
	af_lib_fun.af_ioctrl_set_isp_stop_info	= sft_af_ioctrl_set_isp_stop_info;
	af_lib_fun.af_ioctrl_set_ae_awb_info	= sft_af_ioctrl_set_ae_awb_info;
	af_lib_fun.af_ioctrl_get_af_cur_pos	= sft_af_ioctrl_get_af_cur_pos;
	af_lib_fun.af_ioctrl_set_af_pos	= sft_af_ioctrl_set_af_pos;
	af_lib_fun.af_ioctrl_set_af_bypass	= sft_af_ioctrl_set_af_bypass;
	af_lib_fun.af_ioctrl_set_af_stop	= sft_af_ioctrl_set_af_stop;

	return;
}
