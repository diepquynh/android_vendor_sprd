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
#define LOG_TAG "ALC_AF_Ctrl"
#include "isp_com.h"
#include "isp_drv.h"
#include "ALC_AF_Ctrl.h"
#include "ALC_AF_Data.h"
#include "stdio.h"
#include <utils/Log.h>
#include "ae_sprd_ctrl.h"
#include "awb_ctrl.h"
#include "lib_ctrl.h"


#ifdef WIN32
#define ALC_AF_LOG
#define ALC_AF_LOGW
#define ALC_AF_LOGI
#define ALC_AF_LOGD
#define ALC_AF_LOGV
#else
#define ALC_AF_DEBUG_STR     "ALC_AF: %d, %s: "
#define ALC_AF_DEBUG_ARGS    __LINE__,__FUNCTION__

#define ALC_AF_LOG(format,...) ALOGE(ALC_AF_DEBUG_STR format, ALC_AF_DEBUG_ARGS, ##__VA_ARGS__)
#define ALC_AF_LOGE(format,...) ALOGE(ALC_AF_DEBUG_STR format, ALC_AF_DEBUG_ARGS, ##__VA_ARGS__)
#define ALC_AF_LOGW(format,...) ALOGW(ALC_AF_DEBUG_STR format, ALC_AF_DEBUG_ARGS, ##__VA_ARGS__)
#define ALC_AF_LOGI(format,...) ALOGI(ALC_AF_DEBUG_STR format, ALC_AF_DEBUG_ARGS, ##__VA_ARGS__)
#define ALC_AF_LOGD(format,...) ALOGD(ALC_AF_DEBUG_STR format, ALC_AF_DEBUG_ARGS, ##__VA_ARGS__)
#define ALC_AF_LOGV(format,...) ALOGV(ALC_AF_DEBUG_STR format, ALC_AF_DEBUG_ARGS, ##__VA_ARGS__)
#endif

#define AF_CALLBACK_EVT 0x00040000

void alc_af_log(const char* format, ...)
{
	char buffer[2048]={0};
	va_list arg;
	va_start (arg, format);
	vsnprintf(buffer, 2048, format, arg);
	va_end (arg);
	ALOGE("ALC_AF: %s",buffer);
}



static int32_t _alc_check_handle(alc_af_handle_t handle)
{
	int32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *cxt = (struct alc_af_context *)handle;

	if (NULL == cxt) {
		ALC_AF_LOG("invalid cxt pointer");
		return ALC_AF_ERROR;
	}

	if (ALC_AF_MAGIC_START != cxt->magic_start
		|| ALC_AF_MAGIC_END!= cxt->magic_end) {
		ALC_AF_LOG("invalid magic begin = 0x%x, magic end = 0x%x",
					cxt->magic_start, cxt->magic_end);
		return ALC_AF_ERROR;
	}

	return rtn;
}



int32_t alc_set_afm_bypass(alc_af_handle_t handle,uint32_t bypass)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	
	ALC_AF_LOG(" %d",bypass);
	isp_u_raw_afm_bypass(ctrl_context->handle_device,bypass);
	
	return rtn;
}

int32_t alc_set_afm_mode(alc_af_handle_t handle,uint32_t mode)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	
	ALC_AF_LOG(" %d",mode);
	isp_u_raw_afm_mode(ctrl_context->handle_device,mode);
	
	return rtn;
}

int32_t alc_set_afm_skip_num(alc_af_handle_t handle,uint32_t skip_num)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	ALC_AF_LOG(" %d",skip_num);
	isp_u_raw_afm_skip_num(ctrl_context->handle_device,skip_num);
	return rtn;
}

int32_t alc_set_afm_skip_num_clr(alc_af_handle_t handle)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	ALC_AF_LOG(" ");


	isp_u_raw_afm_skip_num_clr(ctrl_context->handle_device,1);
	isp_u_raw_afm_skip_num_clr(ctrl_context->handle_device,0);

	return rtn;
}

int32_t alc_set_afm_spsmd_rtgbot_enable(alc_af_handle_t handle,uint32_t enable)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	ALC_AF_LOG("set_afm_spsmd_rtgbot_enable %d",enable);
	isp_u_raw_afm_spsmd_rtgbot_enable(ctrl_context->handle_device,enable);
	return rtn;
}

int32_t alc_set_afm_spsmd_diagonal_enable(alc_af_handle_t handle,uint32_t enable)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;


	ALC_AF_LOG("set_afm_spsmd_diagonal_enable %d",enable);
	isp_u_raw_afm_spsmd_diagonal_enable(ctrl_context->handle_device,enable);
	return rtn;
}

int32_t alc_set_afm_spsmd_cal_mode(alc_af_handle_t handle,uint32_t mode)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;


	ALC_AF_LOG("set_afm_spsmd_cal_mode %d",mode);
	isp_u_raw_afm_spsmd_cal_mode(ctrl_context->handle_device,mode);
	return rtn;
}

int32_t alc_set_afm_sel_filter1(alc_af_handle_t handle,uint32_t sel_filter)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	ALC_AF_LOG("sel_filter1 %d",sel_filter);
	isp_u_raw_afm_sel_filter1(ctrl_context->handle_device,sel_filter);

	return rtn;
}

int32_t alc_set_afm_sel_filter2(alc_af_handle_t handle,uint32_t sel_filter)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	ALC_AF_LOG("sel_filter2 %d",sel_filter);
	isp_u_raw_afm_sel_filter2(ctrl_context->handle_device,sel_filter);

	return rtn;
}

int32_t alc_set_afm_sobel_type(alc_af_handle_t handle,uint32_t type)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	ALC_AF_LOG("sobel_type %d",type);
	isp_u_raw_afm_sobel_type(ctrl_context->handle_device,type);

	return rtn;
}

int32_t alc_set_afm_spsmd_type(alc_af_handle_t handle,uint32_t type)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	
	ALC_AF_LOG("spsmd_type %d",type);
	isp_u_raw_afm_spsmd_type(ctrl_context->handle_device,type);

	return rtn;
}

int32_t alc_set_afm_sobel_threshold(alc_af_handle_t handle,uint32_t min, uint32_t max)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	ALC_AF_LOG("sobel_threshold %d %d",min,max);
	isp_u_raw_afm_sobel_threshold(ctrl_context->handle_device,min,max);

	return rtn;
}

int32_t alc_set_afm_spsmd_threshold(alc_af_handle_t handle,uint32_t min, uint32_t max)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;
	ALC_AF_LOG("spsmd_threshold %d %d",min,max);
	isp_u_raw_afm_spsmd_threshold(ctrl_context->handle_device,min,max);

	return rtn;
}

int32_t alc_set_afm_slice_size(alc_af_handle_t handle,uint32_t width, uint32_t height)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	
	ALC_AF_LOG("slice %d %d",width,height);
	isp_u_raw_afm_slice_size(ctrl_context->handle_device,width,height);

	return rtn;
}
int32_t alc_set_afm_win(alc_af_handle_t handle, struct alc_win_coord *win_range)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	uint32_t max_win_num;
	uint32_t i;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	
	isp_u_raw_afm_win_num(ctrl_context->handle_device,&max_win_num);
	ALC_AF_LOG("active_win_s =  %04x", af_cxt->active_win);
	for (i=0;i<max_win_num;i++) {
		//ALC_AF_LOGV("1win[%d] %d %d %d %d",i,win_range[i].start_x,win_range[i].start_y,win_range[i].end_x,win_range[i].end_y);
		//ALC_AF_LOG("ALC_1win[%d] %d %d %d %d",i,win_range[i].start_x,win_range[i].start_y,win_range[i].end_x,win_range[i].end_y);
		if (!(af_cxt->active_win & (1<<i))){
			win_range[i].start_x = 0;
			win_range[i].start_y = 0;
			win_range[i].end_x = 0;
			win_range[i].end_y = 0;
		}
		//ALC_AF_LOGV("ALC_2win[%d] %d %d %d %d",i,win_range[i].start_x,win_range[i].start_y,win_range[i].end_x,win_range[i].end_y);
		//ALC_AF_LOG("ALC_2win[%d] %d %d %d %d",i,win_range[i].start_x,win_range[i].start_y,win_range[i].end_x,win_range[i].end_y);
	}
	isp_u_raw_afm_win(ctrl_context->handle_device,(void*)win_range);

	return rtn;
}

int32_t alc_get_afm_type1_statistic(alc_af_handle_t handle, uint32_t *statis)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	uint32_t max_win_num;
	uint32_t i;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	
	isp_u_raw_afm_win_num(ctrl_context->handle_device,&max_win_num);
	isp_u_raw_afm_type1_statistic(ctrl_context->handle_device,(void*)statis);
	for (i=0;i<max_win_num;i++) {
		if (!(af_cxt->active_win & (1<<i))){
			statis[i] = 0;
		}
	}
	//ALC_AF_LOGV("ALC_act win %d : %d %d %d %d %d %d %d",af_cxt->active_win,statis[0],statis[1],statis[2],statis[3],statis[4],statis[5],statis[6]);
	//ALC_AF_LOG("ALC_act win %d : %d %d %d %d %d %d %d",af_cxt->active_win,statis[0],statis[1],statis[2],statis[3],statis[4],statis[5],statis[6]);
	return rtn;
}

int32_t alc_get_afm_type2_statistic(alc_af_handle_t handle, uint32_t *statis)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	uint32_t max_win_num;
	uint32_t i;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	
	isp_u_raw_afm_win_num(ctrl_context->handle_device,&max_win_num);
	isp_u_raw_afm_type2_statistic(ctrl_context->handle_device,(void*)statis);
	for (i=0;i<max_win_num;i++) {
		if (!(af_cxt->active_win & (1<<i))){
			statis[i] = 0;
		}
	}
	//ALC_AF_LOGV("ALC_act win %d : %d %d %d %d %d %d %d",af_cxt->active_win,statis[0],statis[1],statis[2],statis[3],statis[4],statis[5],statis[6]);
	//ALC_AF_LOG("ALC_act win %d : %d %d %d %d %d %d %d",af_cxt->active_win,statis[0],statis[1],statis[2],statis[3],statis[4],statis[5],statis[6]);
	return rtn;
}

int32_t alc_set_sp_afm_cfg(alc_af_handle_t handle)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;
	struct isp_dev_yiq_afm_info_v1 afm_info;
	struct alc_afm_cfg_info *cfg_ptr = &af_cxt->sprd_filter;
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

int32_t alc_set_sp_afm_win(alc_af_handle_t handle, struct alc_win_coord *win_range)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	uint32_t max_win_num;
	uint32_t i;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	
	isp_u_yiq_afm_win_num(ctrl_context->handle_device,&max_win_num);
	ALC_AF_LOG("active_win_s =  %04x", af_cxt->active_win);
	for (i=0;i<max_win_num;i++) {
		ALC_AF_LOG("ALC_1winsp[%d] %d %d %d %d",i,win_range[i].start_x,win_range[i].start_y,win_range[i].end_x,win_range[i].end_y);

		if (!(af_cxt->active_win & (1<<i))){
			win_range[i].start_x = 0;
			win_range[i].start_y = 0;
			win_range[i].end_x = 0;
			win_range[i].end_y = 0;
		}

		ALC_AF_LOG("ALC_2winsp[%d] %d %d %d %d",i,win_range[i].start_x,win_range[i].start_y,win_range[i].end_x,win_range[i].end_y);
	}
	isp_u_yiq_afm_win(ctrl_context->handle_device,(void*)win_range);

	return rtn;
}

int32_t alc_get_sp_afm_statistic(alc_af_handle_t handle, uint32_t *statis)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	uint32_t max_win_num;
	uint32_t i;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	//ALC_AF_LOGI("ALC_callspstatistic ");//for test

	isp_u_yiq_afm_win_num(ctrl_context->handle_device,&max_win_num);
	isp_u_yiq_afm_statistic(ctrl_context->handle_device,(void*)statis);
	//ALC_AF_LOG("ALC_max_win_num %d",max_win_num);//for test
	//ALC_AF_LOG("ALC_active_win %d",af_cxt->active_win);//for test
/*
	for (i=0;i<max_win_num;i++) {

		if (!(af_cxt->active_win & (1<<i))){
			statis[i] = 0;
			statis[max_win_num+i] = 0;
			statis[max_win_num*2+i] = 0;
			statis[max_win_num*3+i] = 0;
		}
	}
*/
/*
	for(i=0;i<=3;i++){// 0 for LAPlACE; 1 for SOBEL; 2 for IIR1; 3 for IIR2
		if( 0==i ){
			ALC_AF_LOG("ALC_act LAPLACE");
		}else if( 1==i ){
			ALC_AF_LOG("ALC_act SOBEL");
		}else if( 2==i ){
			ALC_AF_LOG("ALC_act IIR1");
		}else if( 3==i ){
			ALC_AF_LOG("ALC_act IIR2");
		}
		ALC_AF_LOG("ALC_act win %d : %d %d %d %d %d %d %d",af_cxt->active_win,statis[i*25+0],statis[i*25+1],statis[i*25+2],statis[i*25+3],statis[i*25+4],statis[i*25+5],statis[i*25+6]);
		ALC_AF_LOG("ALC_act win %d : %d %d %d %d %d %d %d",af_cxt->active_win,statis[i*25+7],statis[i*25+8],statis[i*25+9],statis[i*25+10],statis[i*25+11],statis[i*25+12],statis[i*25+13]);
		ALC_AF_LOG("ALC_act win %d : %d %d %d %d %d %d %d",af_cxt->active_win,statis[i*25+14],statis[i*25+15],statis[i*25+16],statis[i*25+17],statis[i*25+18],statis[i*25+19],statis[i*25+20]);
		ALC_AF_LOG("ALC_act win %d : %d %d %d %d ",af_cxt->active_win,statis[i*25+21],statis[i*25+22],statis[i*25+23],statis[i*25+24]);
		ALC_AF_LOG("ALC_act  ");
	}
*/
//ALC_AF_LOG("ALC_act win %d : %d %d %d %d %d %d %d %d %d",af_cxt->active_win,statis[0],statis[1],statis[3],statis[4],statis[5],statis[6],statis[7],statis[8]);

	return rtn;
}


int32_t alc_sp_write_i2c(alc_af_handle_t handle,uint16_t slave_addr, uint8_t *cmd, uint16_t cmd_length)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	
	if (NULL == ctrl_context
		|| NULL == ctrl_context->ioctrl_ptr
		|| NULL == ctrl_context->ioctrl_ptr->write_i2c) {
			ALC_AF_LOG("failed pointer is NULL error");
			return ISP_ERROR;
	}

	if (0 == ctrl_context->camera_id) {
		if (NULL == ctrl_context
			|| NULL == ctrl_context->ioctrl_ptr
			|| NULL == ctrl_context->ioctrl_ptr->set_focus) {
			ctrl_context->ioctrl_ptr->write_i2c(slave_addr,cmd,cmd_length);
		}
	}

	return rtn;


}

int32_t alc_sp_read_i2c(alc_af_handle_t handle,uint16_t slave_addr, uint8_t *cmd, uint16_t cmd_length)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	
	if (NULL == ctrl_context
		|| NULL == ctrl_context->ioctrl_ptr
		|| NULL == ctrl_context->ioctrl_ptr->read_i2c) {
			ALC_AF_LOG("cxt=%p,ioctl=%p, set_focus=%p is NULL error", ctrl_context,
				ctrl_context->ioctrl_ptr, ctrl_context->ioctrl_ptr->read_i2c);
			return ISP_ERROR;
	}

	if (0 == ctrl_context->camera_id) {
		if (NULL == ctrl_context
			|| NULL == ctrl_context->ioctrl_ptr
			|| NULL == ctrl_context->ioctrl_ptr->set_focus) {
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

int32_t alc_sp_get_cur_prv_mode(alc_af_handle_t handle,uint32_t *mode)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	if (ctrl_context->isp_mode >= ISP_MODE_ID_VIDEO_0) {
		*mode = 1;
	} else {
		*mode = 0;
	}

	return rtn;
}




int32_t alc_set_active_win(alc_af_handle_t handle, uint32_t active_win)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;
	
	ALC_AF_LOG("active win 0x%x",active_win);
	af_cxt->active_win = active_win;
	return rtn;

}


int32_t alc_lock_ae(alc_af_handle_t handle)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;
	struct ae_calc_out ae_result = {0};

	ALC_AF_LOG("lock_ae");
	af_cxt->ae_is_locked = 1;
	ctrl_context->ae_lib_fun->ae_io_ctrl(ctrl_context->handle_ae, AE_SET_PAUSE, NULL, (void*)&ae_result);
	return rtn;

}

int32_t alc_lock_awb(alc_af_handle_t handle)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	ALC_AF_LOG("lock_awb");
	af_cxt->awb_is_locked = 1;
	ctrl_context->awb_lib_fun->awb_ctrl_ioctrl(ctrl_context->handle_awb, AWB_CTRL_CMD_LOCK, NULL,NULL);
	return rtn;

}

int32_t alc_unlock_ae(alc_af_handle_t handle)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;
	struct ae_calc_out ae_result = {0};

	ALC_AF_LOG("unlock_ae");
	ctrl_context->ae_lib_fun->ae_io_ctrl(ctrl_context->handle_ae, AE_SET_RESTORE, NULL, (void*)&ae_result);
	af_cxt->ae_is_locked = 0;
	return rtn;

}

int32_t alc_unlock_awb(alc_af_handle_t handle)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	ALC_AF_LOG("unlock_awb");
	ctrl_context->awb_lib_fun->awb_ctrl_ioctrl(ctrl_context->handle_awb, AWB_CTRL_CMD_UNLOCK, NULL,NULL);
	af_cxt->awb_is_locked = 0;
	return rtn;

}

int32_t alc_get_cur_env_mode(alc_af_handle_t handle, uint8_t *mode)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	*mode = (uint8_t)af_cxt->cur_env_mode;

	ALC_AF_LOGV("ISP_AF:cur env mode in get_cur_env_mode is %p  ",mode);

	return rtn;
}


int32_t alc_set_motor_pos(alc_af_handle_t handle, uint32_t pos)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;
	
	if (NULL == ctrl_context
		|| NULL == ctrl_context->ioctrl_ptr
		|| NULL == ctrl_context->ioctrl_ptr->set_focus) {
			ALC_AF_LOG("failed pointer is NULL error");
			return ISP_ERROR;
	}

	ALC_AF_LOG("ALC_posb %d",pos);
	ctrl_context->ioctrl_ptr->set_focus(pos);
	af_cxt->cur_pos = pos;

	return rtn;
}

uint32_t alc_get_ae_lum(alc_af_handle_t handle)
{
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;
	
	return af_cxt->ae_cur_lum;
}

uint32_t alc_get_ae_status(alc_af_handle_t handle)
{
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	return af_cxt->ae_is_stab;
}

uint32_t alc_get_awb_status(alc_af_handle_t handle)
{
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	return af_cxt->awb_is_stab;
}

int32_t alc_get_isp_size(alc_af_handle_t handle, uint16_t *widith, uint16_t *height)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;
	*widith = ctrl_context->input_size_trim[ctrl_context->param_index].width;
	*height = ctrl_context->input_size_trim[ctrl_context->param_index].height;
	ALC_AF_LOG("w %d  h %d",*widith,*height);
	return rtn;
}

int32_t alc_af_finish_notice(alc_af_handle_t handle, uint32_t result)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;
	struct isp_system* isp_system_ptr = &ctrl_context->system;

	ALC_AF_LOG("AF_TAG: move end");
	if ((ISP_ZERO == isp_system_ptr->isp_callback_bypass) && af_cxt->is_runing) {
		struct isp_af_notice af_notice = {0x00};
		af_notice.mode=ISP_FOCUS_MOVE_END;
		af_notice.valid_win = result?1:0;
		ALC_AF_LOGD("callback ISP_AF_NOTICE_CALLBACK");
		isp_system_ptr->callback(isp_system_ptr->caller_id, AF_CALLBACK_EVT|ISP_AF_NOTICE_CALLBACK, (void*)&af_notice, sizeof(struct isp_af_notice));
	}
	af_cxt->is_runing = 0;

	return rtn;
}

int32_t alc_af_move_start_notice(alc_af_handle_t handle)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;
	struct isp_system* isp_system_ptr = &ctrl_context->system;

	ALC_AF_LOG("AF_TAG: move start");
	if ((ISP_ZERO == isp_system_ptr->isp_callback_bypass) && (0 == af_cxt->is_runing) && (1 == af_cxt->caf_active)) {
		struct isp_af_notice af_notice = {0x00};
		af_notice.mode=ISP_FOCUS_MOVE_START;
		af_notice.valid_win = 0;
		ALC_AF_LOGD("callback ISP_AF_NOTICE_CALLBACK");
		isp_system_ptr->callback(isp_system_ptr->caller_id, AF_CALLBACK_EVT|ISP_AF_NOTICE_CALLBACK, (void*)&af_notice, sizeof(struct isp_af_notice));
	}
	af_cxt->is_runing = 1;

	return rtn;
}

int32_t alc_af_pos_update(alc_af_handle_t handle, uint32_t pos)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)af_cxt->isp_handle;

	
	if (NULL == ctrl_context
		|| NULL == ctrl_context->ioctrl_ptr
		|| NULL == ctrl_context->ioctrl_ptr->set_focus) {
		ALC_AF_LOG("failed pointer is NULL error");
		rtn = ISP_ERROR;
	} else {
		ctrl_context->ioctrl_ptr->set_focus(pos);
	}

	ALC_AF_LOG("pos %d",pos);
	af_cxt->cur_pos = pos;

	return rtn;
}

int32_t alc_get_motor_range(alc_af_handle_t handle, uint16_t *min_pos, uint16_t *max_pos)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;

	*min_pos = 0;
	*max_pos = 1023;
	
	return rtn;
}

int32_t alc_get_lens_info(alc_af_handle_t handle, uint16_t *f_num, uint16_t *f_len)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;

	*f_num = 12; // the unit is 0.1
	*f_len = 395; // the unit is 0.01mm
	
	return rtn;
}

int32_t alc_get_accelerator_sensor_info(alc_af_handle_t handle, uint16_t* posture)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;

	*posture = 0; // 0 stands for UPWARD , 1 stands for DOWNWARD, 2 stands for HORIZONTAL

	return rtn;
}

int32_t alc_get_magnetic_sensor_info(alc_af_handle_t handle, uint16_t* ispinning)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;

	*ispinning = 0; // 0 stands for STILL , 1 stands for PINNING

	return rtn;
}

alc_af_handle_t alc_af_init(void* isp_handle)
{
	struct alc_af_context *af_cxt = NULL;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)isp_handle;

	isp_u_raw_afm_bypass(ctrl_context->handle_device,1);

	if (1 == ctrl_context->camera_id) {
		ALC_AF_LOGE("Front camera nor support AF!");
		return NULL;
	}

	af_cxt = (struct alc_af_context *)malloc(sizeof(struct alc_af_context));
	if (NULL == af_cxt) {
		return NULL;
	}
	ALC_AF_LOG("S");

	memset(af_cxt,0,sizeof(struct alc_af_context));
	af_cxt->magic_start = ALC_AF_MAGIC_START;
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
	af_cxt->sensor_w = ctrl_context->input_size_trim[ctrl_context->param_index].width;
	af_cxt->sensor_h = ctrl_context->input_size_trim[ctrl_context->param_index].height;
	
	af_cxt->af_ctrl_ops.cb_set_afm_bypass                    = alc_set_afm_bypass;
	af_cxt->af_ctrl_ops.cb_set_afm_mode                      = alc_set_afm_mode;
	af_cxt->af_ctrl_ops.cb_set_afm_skip_num                  = alc_set_afm_skip_num;
	af_cxt->af_ctrl_ops.cb_set_afm_skip_num_clr              = alc_set_afm_skip_num_clr;
	af_cxt->af_ctrl_ops.cb_set_afm_spsmd_rtgbot_enable       = alc_set_afm_spsmd_rtgbot_enable;
	af_cxt->af_ctrl_ops.cb_set_afm_spsmd_diagonal_enable     = alc_set_afm_spsmd_diagonal_enable;
	af_cxt->af_ctrl_ops.cb_set_afm_spsmd_cal_mode            = alc_set_afm_spsmd_cal_mode;
	af_cxt->af_ctrl_ops.cb_set_afm_sel_filter1               = alc_set_afm_sel_filter1;
	af_cxt->af_ctrl_ops.cb_set_afm_sel_filter2               = alc_set_afm_sel_filter2;
	af_cxt->af_ctrl_ops.cb_set_afm_sobel_type                = alc_set_afm_sobel_type;
	af_cxt->af_ctrl_ops.cb_set_afm_spsmd_type                = alc_set_afm_spsmd_type;
	af_cxt->af_ctrl_ops.cb_set_afm_sobel_threshold           = alc_set_afm_sobel_threshold;
	af_cxt->af_ctrl_ops.cb_set_afm_spsmd_threshold           = alc_set_afm_spsmd_threshold;
	af_cxt->af_ctrl_ops.cb_set_afm_slice_size                = alc_set_afm_slice_size;
	af_cxt->af_ctrl_ops.cb_set_afm_win                       = alc_set_afm_win;
	af_cxt->af_ctrl_ops.cb_get_afm_type1_statistic           = alc_get_afm_type1_statistic;
	af_cxt->af_ctrl_ops.cb_get_afm_type2_statistic           = alc_get_afm_type2_statistic;
	af_cxt->af_ctrl_ops.cb_set_active_win                    = alc_set_active_win;
	af_cxt->af_ctrl_ops.cb_get_cur_env_mode                  = alc_get_cur_env_mode;
	af_cxt->af_ctrl_ops.cb_set_motor_pos                     = alc_set_motor_pos;
	af_cxt->af_ctrl_ops.cb_lock_ae                           = alc_lock_ae;
	af_cxt->af_ctrl_ops.cb_lock_awb                          = alc_lock_awb;
	af_cxt->af_ctrl_ops.cb_unlock_ae                         = alc_unlock_ae;
	af_cxt->af_ctrl_ops.cb_unlock_awb                        = alc_unlock_awb;
	af_cxt->af_ctrl_ops.cb_get_ae_lum                        = alc_get_ae_lum;
	af_cxt->af_ctrl_ops.cb_get_ae_status                     = alc_get_ae_status;
	af_cxt->af_ctrl_ops.cb_get_awb_status                    = alc_get_awb_status;
	af_cxt->af_ctrl_ops.cb_get_isp_size                      = alc_get_isp_size;
	af_cxt->af_ctrl_ops.cb_af_finish_notice                  = alc_af_finish_notice;
	af_cxt->af_ctrl_ops.cb_alc_af_log                        = alc_af_log;
	af_cxt->af_ctrl_ops.cb_set_sp_afm_cfg                    = alc_set_sp_afm_cfg;
	af_cxt->af_ctrl_ops.cb_set_sp_afm_win                    = alc_set_sp_afm_win;
	af_cxt->af_ctrl_ops.cb_get_sp_afm_statistic              = alc_get_sp_afm_statistic;
	af_cxt->af_ctrl_ops.cb_sp_write_i2c                      = alc_sp_write_i2c;
	af_cxt->af_ctrl_ops.cd_sp_read_i2c                       = alc_sp_read_i2c;
	af_cxt->af_ctrl_ops.cd_sp_get_cur_prv_mode               = alc_sp_get_cur_prv_mode;
	af_cxt->af_ctrl_ops.cb_af_move_start_notice              = alc_af_move_start_notice;
	af_cxt->af_ctrl_ops.cb_af_pos_update                     = alc_af_pos_update;

	af_cxt->af_ctrl_ops.cb_get_motor_range                 = alc_get_motor_range;
	af_cxt->af_ctrl_ops.cb_get_lens_info                      = alc_get_lens_info;
	af_cxt->af_ctrl_ops.cb_get_accelerator_sensor_info = alc_get_accelerator_sensor_info;
	af_cxt->af_ctrl_ops.cb_get_magnetic_sensor_info = alc_get_magnetic_sensor_info;
	
	af_cxt->magic_end = ALC_AF_MAGIC_END;
	ctrl_context->handle_af = af_cxt;
	ALC_AF_LOGI("ALC afinitstart");
//	ALC_AF_LOGI("ALC SEN_SIZE: Width:%d , Height:%d",af_cxt->sensor_w , af_cxt->sensor_h) ;
	return (alc_af_handle_t)af_cxt;

}



int32_t alc_af_deinit(void* isp_handle)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)isp_handle;
	struct alc_af_context *af_cxt = (struct alc_af_context *)ctrl_context->handle_af;

	rtn = _alc_check_handle(af_cxt);
	if (ALC_AF_SUCCESS != rtn) {
		ALC_AF_LOG("_check_cxt failed");
		return ALC_AF_SUCCESS;
	}
	ALC_AF_LOG("S");
	memset(af_cxt,0,sizeof(*af_cxt));
	free(af_cxt);
	return rtn;
}


int32_t alc_af_calc(isp_ctrl_context* handle)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)handle;
	struct alc_af_context *af_cxt = (struct alc_af_context *)ctrl_context->handle_af;
	struct alc_af_ctrl_ops *af_ops = &af_cxt->af_ctrl_ops;
	TT_AfIfBuf *ppAfIfBuf = &af_cxt->ttAfIfBuf ;//tes_kano_20150820

	rtn = _alc_check_handle(af_cxt);
	if (ALC_AF_SUCCESS != rtn) {
		ALC_AF_LOG("_check_cxt failed");
		return ALC_AF_SUCCESS;
	}
	
	//tes_kano_0824 AF_IP Mode setting
	if(ppAfIfBuf->mttAcoAf.af_finish == 1){
		ppAfIfBuf->mttAcoAf.af_finish = 2 ;
		if(af_cxt->af_mode == 3){
			ppAfIfBuf->mttInfo.mttAfAfMode.muiAfMode = 1 ; 
			ppAfIfBuf->mttInfo.mttAfWindow.muiAfWindowMode = 4 ;
			//ppAfIfBuf->mttInfo.mttAfAfMode.muiAfContMode = 0 ; //0:Preview,1:Movie
		}
		else if(af_cxt->af_mode == 4){
			ppAfIfBuf->mttInfo.mttAfAfMode.muiAfMode = 1 ; 
			ppAfIfBuf->mttInfo.mttAfWindow.muiAfWindowMode = 4 ;
			//ppAfIfBuf->mttInfo.mttAfAfMode.muiAfContMode = 1 ; //0:Preview,1:Movie
		}
		else{
			ppAfIfBuf->mttInfo.mttAfAfMode.muiAfMode = 2 ; //0:Still,1:Continuos tes_kano_0826 2:CAF off
			ppAfIfBuf->mttInfo.mttAfWindow.muiAfWindowMode = 1 ;
			//ppAfIfBuf->mttInfo.mttAfAfMode.muiAfContMode = 0 ; //0:Preview,1:Movie
		}
		af_ops->cb_af_finish_notice(af_cxt,1);
	}
#if 1 // 0:tap af off , 1:tap af on tes_kano_0826
	else if ((ppAfIfBuf->mttInfo.mttAfAfMode.muiAfMode != 0) && (af_cxt->is_runing == 1)){
		ppAfIfBuf->mttInfo.mttAfAfMode.muiAfMode = 0 ; //0:Still,1:Continuos
		if(af_cxt->touch_af == 1){
			ppAfIfBuf->mttInfo.mttAfWindow.muiAfWindowMode = 8 ;
			af_cxt->touch_af = 0 ;
		}
		else{
			ppAfIfBuf->mttInfo.mttAfWindow.muiAfWindowMode = 2 ;
		}
		//ppAfIfBuf->mttInfo.mttAfAfMode.muiAfContMode = 0 ; //0:Preview,1:Movie
	}
#endif
#if 1 // 0:face af off , 1:face af on tes_kano_0826
	else if ((ppAfIfBuf->mttInfo.mttAfAfMode.muiAfMode != 0) && (ppAfIfBuf->mttInfo.mttAfWindow.muiAfWindowMode == 16)){ //tes_kano_0825
		ppAfIfBuf->mttInfo.mttAfAfMode.muiAfMode = 0 ; //0:Still,1:Continuos
	}
#endif
#if ALC_AF_DEFOCUS_TEST > 0 //for defocus
	else if (ppAfIfBuf->mttInfo.mttAfAfMode.muiAfMode == 8){
		
	}
#endif
	else if ((ppAfIfBuf->mttInfo.mttAfAfMode.muiAfMode != 1) && (ppAfIfBuf->mttAcoAf.af_status == 0) && (af_cxt->af_mode == 3)){
		ppAfIfBuf->mttInfo.mttAfAfMode.muiAfMode = 1 ; //0:Still,1:Continuos
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfWindowMode = 4 ;
		//ppAfIfBuf->mttInfo.mttAfAfMode.muiAfContMode = 0 ; //0:Preview,1:Movie
	}
	else if ((ppAfIfBuf->mttInfo.mttAfAfMode.muiAfMode != 1) && (ppAfIfBuf->mttAcoAf.af_status == 0) && (af_cxt->af_mode == 4)){
		ppAfIfBuf->mttInfo.mttAfAfMode.muiAfMode = 1 ; //0:Still,1:Continuos
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfWindowMode = 4 ;
		//ppAfIfBuf->mttInfo.mttAfAfMode.muiAfContMode = 1 ; //0:Preview,1:Movie
	}
	else if ((ppAfIfBuf->mttInfo.mttAfAfMode.muiAfMode != 2) && (ppAfIfBuf->mttAcoAf.af_status == 0) && (af_cxt->af_mode == 0)){
		ppAfIfBuf->mttInfo.mttAfAfMode.muiAfMode = 2 ; //0:Still,1:Continuos
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfWindowMode = 1 ;
		//ppAfIfBuf->mttInfo.mttAfAfMode.muiAfContMode = 0 ; //0:Preview,1:Movie
	}
	else if ((ppAfIfBuf->mttInfo.mttAfAfMode.muiAfMode != 2) && (ppAfIfBuf->mttAcoAf.af_status == 0) && (af_cxt->af_mode == 1)){
		ppAfIfBuf->mttInfo.mttAfAfMode.muiAfMode = 2 ; //0:Still,1:Continuos
		ppAfIfBuf->mttInfo.mttAfWindow.muiAfWindowMode = 1 ;
		//ppAfIfBuf->mttInfo.mttAfAfMode.muiAfContMode = 0 ; //0:Preview,1:Movie
	}
	
	if((ppAfIfBuf->mttInfo.mttAfAfMode.muiAfContMode != 0) && (af_cxt->af_mode == 3)){
		ppAfIfBuf->mttInfo.mttAfAfMode.muiAfContMode = 0 ; //0:Preview,1:Movie
	}
	else if((ppAfIfBuf->mttInfo.mttAfAfMode.muiAfContMode != 1) && (af_cxt->af_mode == 4)){
		ppAfIfBuf->mttInfo.mttAfAfMode.muiAfContMode = 1 ; //0:Preview,1:Movie
	}
	
//	ALC_AF_LOG("alc_af_calc : af_mode=%d",af_cxt->af_mode);
//	ALC_AF_LOGI("ALC af runing ");
	Al_AF_Running(af_cxt , ppAfIfBuf);//tes_kano_20150820

	return rtn;

}


int32_t alc_af_ioctrl(alc_af_handle_t handle, enum alc_af_cmd cmd,
				void *param0, void *param1)
{
	uint32_t rtn = ALC_AF_SUCCESS;
	struct alc_af_context *af_cxt = (struct alc_af_context *)handle;
	struct alc_af_ctrl_ops *af_ops = &af_cxt->af_ctrl_ops;
	TT_AfIfBuf *ppAfIfBuf = &af_cxt->ttAfIfBuf ;//tes_kano_0822
	uint32_t face_af = 0 ; //tes_kano_0903
		
	ALC_AF_LOGI("ALC__af_ioctrl"); //for test
	rtn = _alc_check_handle(handle);
	if (ALC_AF_SUCCESS != rtn) {
		ALC_AF_LOG("_check_cxt failed");
		return ALC_AF_SUCCESS;
	}

	switch (cmd) {
	case ALC_AF_CMD_GET_AF_INFO:
		break;
	case ALC_AF_CMD_SET_AF_INFO:
		break;
	case ALC_AF_CMD_GET_AF_VALUE://added for alc
	ALC_AF_LOGI("ALC_AF_CMD_GET_AF_VALUE"); //for test
		break;
	case ALC_AF_CMD_SET_AF_MODE:{
		uint32_t af_mode = *(uint32_t*)param0; // 0 stands for auto , 1 stands for multi-win
		af_cxt->af_mode = af_mode;//tes_kano_0902
		ALC_AF_LOGI("alc af mode %d",af_mode);
		
		break;
	}
	case ALC_AF_CMD_GET_AF_MULTIWIN:
		//param0 = ;
		break;
	case ALC_AF_CMD_SET_AF_POS:
		break;
	case ALC_AF_CMD_SET_SCENE_MODE:
		break;
	case ALC_AF_CMD_SET_AF_START: {
		struct isp_af_win* af_ptr = (struct isp_af_win*)param0;
		uint32_t af_mode = af_cxt->af_mode;
		uint32_t i=0;
		af_cxt->touch_af = 0 ; //tes_kano_0902
		//ALC_AF_LOG("ALC af start");
		//ALC_AF_LOG("ALC af start : af_mode=%d",af_mode);
		//ALC_AF_LOGI("ALC_AF_CMD_SET_AF_START"); //for test
		if (af_ptr) {
			af_cxt->touch_af = af_ptr->valid_win ; //tes_kano_0914
			ALC_AF_LOG("Shutter or Touch %d",af_ptr->valid_win);

			if (((ALC_AF_MODE_CAF == af_mode) || (ALC_AF_MODE_VIDEO_CAF == af_mode))
				&& (0 == af_cxt->flash_on)
				&& (0 == af_ptr->valid_win)) {
				if (af_cxt->is_runing) {
					break;
				} else {
					af_cxt->is_runing = 1;
					af_ops->cb_af_finish_notice(af_cxt,1);
					break;
				}
			}

			af_cxt->touch_win_cnt = af_ptr->valid_win;
			//af_cxt->touch_af = 1 ; //tes_kano_0902
			for (i=0; i<af_ptr->valid_win; i++) {
				af_cxt->win_pos[i].start_x = af_ptr->win[i].start_x;
				af_cxt->win_pos[i].start_y = af_ptr->win[i].start_y;
				af_cxt->win_pos[i].end_x = af_ptr->win[i].end_x;
				af_cxt->win_pos[i].end_y = af_ptr->win[i].end_y;
			}
		} else {
			af_cxt->touch_win_cnt = 0;
		}
#if 0
		ALC_AF_LOG("ALC af start : af_mode=%d",af_cxt->af_mode);
		ALC_AF_LOG("ALC af start : af_ptr=%d",af_ptr);
		ALC_AF_LOG("ALC af start : touch_win_cnt=%d",af_cxt->touch_win_cnt);
		ALC_AF_LOG("ALC af start : touch_af=%d",af_cxt->touch_af);
		ALC_AF_LOG("ALC af start : touch_sx/sy/ex/ey[0]=%d,%d,%d,%d",af_cxt->win_pos[0].start_x,af_cxt->win_pos[0].start_y,af_cxt->win_pos[0].end_x,af_cxt->win_pos[0].end_y);
		ALC_AF_LOG("ALC af start : touch_sx/sy/ex/ey[1]=%d,%d,%d,%d",af_cxt->win_pos[1].start_x,af_cxt->win_pos[1].start_y,af_cxt->win_pos[1].end_x,af_cxt->win_pos[1].end_y);
#endif
		af_cxt->is_runing = 1;
		break;
	}
	case ALC_AF_CMD_SET_AF_STOP:
		break;
	case ALC_AF_CMD_SET_AF_BYPASS:
		break;
	case ALC_AF_CMD_SET_ISP_START_INFO:
		ALC_AF_LOGI("ALC_AF_CMD_SET_ISP_START_INFO"); //for test
		Al_AF_Prv_Start_Notice(handle , ppAfIfBuf);//tes_kano_20150820
		//AF_Start_Debug(handle);
		break;
	case ALC_AF_CMD_SET_ISP_STOP_INFO:
		break;
	case ALC_AF_CMD_SET_AE_INFO:{
		struct ae_calc_out*  ae_result = (struct ae_calc_out*)param0;
		struct isp_awb_statistic_info* rgb_statistic = (struct isp_awb_statistic_info*)param1; 
		//uint32_t i;
		//i = ppAfIfBuf->mttAcoAf.vd_cnt % 1200 ;
#if 0 //tes_kano_0908 rgb_log
		if(i>1180 && i<1190){
			memcpy(&af_cxt->r_info[0], &rgb_statistic->r_info[0], sizeof(af_cxt->r_info));
			memcpy(&af_cxt->g_info[0], &rgb_statistic->g_info[0], sizeof(af_cxt->g_info));
			memcpy(&af_cxt->b_info[0], &rgb_statistic->b_info[0], sizeof(af_cxt->b_info));
		}
#else

		//tes_kano_0902 get rgb data to af_cxt->r_info[1024]/g_info[1024]/b_info[1024]
		memcpy(&af_cxt->r_info[0], &rgb_statistic->r_info[0], sizeof(af_cxt->r_info));
		memcpy(&af_cxt->g_info[0], &rgb_statistic->g_info[0], sizeof(af_cxt->g_info));
		memcpy(&af_cxt->b_info[0], &rgb_statistic->b_info[0], sizeof(af_cxt->b_info));
#endif
		af_cxt->cur_exp_time = ae_result->cur_exp_line*ae_result->line_time; // exposure time
		if(ae_result->cur_exp_line > ae_result->frame_line){
			af_cxt->cur_frame_time = ae_result->cur_exp_line*ae_result->line_time; // exposure time
		}
		else{
			af_cxt->cur_frame_time = ae_result->frame_line*ae_result->line_time; //frame time
		}
		af_cxt->cur_ae_again = ae_result->cur_again ;
		af_cxt->cur_ae_dgain = ae_result->cur_dgain ;
		af_cxt->cur_ae_ev = ae_result->cur_ev ;
		af_cxt->cur_ae_iso = ae_result->cur_iso ;
		ppAfIfBuf->mttAcoAf.ae_stabl = ae_result->is_stab ;
		if( 0!=af_cxt->cur_frame_time )
			af_cxt->cur_fps = 10000000/af_cxt->cur_frame_time;//  frame per second

#if 0 //ae log
		if(i>1160 && i<1170){
			ALC_AF_LOG("Akn_rgb , vd_cnt,line_time,exp_line,frame_line,ev");
			ALC_AF_LOG("Akn_rgb , %d,%d,%d,%d,%d", ppAfIfBuf->mttAcoAf.vd_cnt , ae_result->line_time , ae_result->cur_exp_line , ae_result->frame_line , af_cxt->cur_ae_ev);
			ALC_AF_LOG("Akn_rgb , exp_time,frame_time,again,dgain,fps");
			ALC_AF_LOG("Akn_rgb , %d,%d,%d,%d,%d",af_cxt->cur_exp_time , af_cxt->cur_frame_time , af_cxt->cur_ae_again , af_cxt->cur_ae_dgain ,af_cxt->cur_fps);
		}

#endif
		break;
	}
	case ALC_AF_CMD_SET_AWB_INFO:{
		break;
	}
	case ALC_AF_CMD_SET_FLASH_NOTICE:{
		break;
	}
	case ALC_AF_CMD_SET_FD_UPDATE:{
		struct isp_face_area *face_area = (struct isp_face_area*)param0;
		uint32_t i;

		af_cxt->fd_info.type = face_area->type;
		af_cxt->fd_info.face_num = face_area->face_num;
		af_cxt->fd_info.frame_width = face_area->frame_width;
		af_cxt->fd_info.frame_height = face_area->frame_height;

		//ALC_AF_LOG("ALC_FD_UPDATE : face_num , %d , fd_af_start_cnt , %d" , af_cxt->fd_info.face_num , ppAfIfBuf->mttAcoAf.fd_af_start_cnt);

		for (i=0; i<10; i++) {
/*
			if(af_cxt->fd_info.face_num > 0){//tes_kano_0826 for fd_af_freaze
				af_cxt->fd_info.face_info[i].sx = face_area->face_info[i].sx;
				af_cxt->fd_info.face_info[i].sy = face_area->face_info[i].sy;
				af_cxt->fd_info.face_info[i].ex = face_area->face_info[i].ex;
				af_cxt->fd_info.face_info[i].ey = face_area->face_info[i].ey;
			}
*/
			af_cxt->fd_info.face_info[i].brightness = face_area->face_info[i].brightness;
			af_cxt->fd_info.face_info[i].pose = face_area->face_info[i].pose;
		}
		
		//tes_kano_0825 for fd_af repeat
		if((af_cxt->af_mode == 0) || (af_cxt->af_mode == 1) || (af_cxt->af_mode == 3)){ //af_mode: 0=auto,1=macro,3=caf,4=video-caf
			face_af = 1 ;
		}
		else{
			face_af = 0 ;
		}
		if((ppAfIfBuf->mttInfo.mttAfWindow.muiAfWindowMode !=16) && (ppAfIfBuf->mttInfo.mttAfAfMode.muiAfMode != 0) && (ppAfIfBuf->mttAcoAf.af_status == 0) && (face_af == 1)){
			if((af_cxt->fd_af_start_cnt > 20) && (af_cxt->fd_info.face_num == 0)){
				af_cxt->fd_af_start_cnt -- ;
			}
			else if((af_cxt->fd_af_start_cnt > 20) && (af_cxt->fd_info.face_num > 0)){
				af_cxt->fd_af_start_cnt = 40 ;
			}
			else if((af_cxt->fd_af_start_cnt <= 20) && (af_cxt->fd_info.face_num > 0)){
				if(af_cxt->fd_af_start_cnt > 0){
					af_cxt->fd_af_start_cnt -- ;
				}
				else if(af_cxt->fd_af_start_cnt == 0){
					af_cxt->fd_af_start_cnt = 40 ;
					//ALC_AF_LOG("ALC_FD_UPDATE : face_num , %d" , af_cxt->fd_info.face_num);
#if 0 //0:face af off , 1:face af on tes_kano_0826
					for (i=0; i<10; i++) {
						af_cxt->fd_info.face_info[i].sx = face_area->face_info[i].sx;
						af_cxt->fd_info.face_info[i].sy = face_area->face_info[i].sy;
						af_cxt->fd_info.face_info[i].ex = face_area->face_info[i].ex;
						af_cxt->fd_info.face_info[i].ey = face_area->face_info[i].ey;
						ALC_AF_LOG("ALC_FD_UPDATE : sx/sy/ex/ey[%d] , %d , %d , %d , %d" , i ,
							af_cxt->fd_info.face_info[i].sx , af_cxt->fd_info.face_info[i].sy , af_cxt->fd_info.face_info[i].ex , af_cxt->fd_info.face_info[i].ey);
					}

					ppAfIfBuf->mttInfo.mttAfWindow.muiAfWindowMode = 16 ;
#endif
				}
			}
		}
		
		break;
	}
	case ALC_AF_CMD_GET_AF_MODE:
		break;
	case ALC_AF_CMD_GET_AF_CUR_POS:
		break;
	case ALC_AF_CMD_BURST_NOTICE:
		break;
	default:
		break;
	}

	
	return rtn;

	
}



int32_t alc_af_ioctrl_af_start(isp_handle isp_handler, void* param_ptr, int(*call_back)())
{
	int32_t rtn = 0;
	isp_ctrl_context* handle = (isp_ctrl_context*)isp_handler;
	rtn = alc_af_ioctrl(handle->handle_af,ALC_AF_CMD_SET_AF_START,param_ptr,NULL);
	return rtn;
}

int32_t alc_af_ioctrl_set_fd_update(isp_handle isp_handler, void* param_ptr, int(*call_back)())
{
	int32_t rtn = 0;
	isp_ctrl_context* handle = (isp_ctrl_context*)isp_handler;
	struct isp_face_area *face_area = (struct isp_face_area*)param_ptr;
	if (face_area) {
		rtn = alc_af_ioctrl(handle->handle_af,ALC_AF_CMD_SET_FD_UPDATE,(void *)face_area,NULL);

	}
	return rtn;
}

int32_t alc_af_ioctrl_set_isp_start_info(isp_handle isp_handler, struct isp_video_start* param_ptr)
{
	int rtn = 0;
	isp_ctrl_context* handle = (isp_ctrl_context*)isp_handler;
	ALC_AF_LOG("alc_af_ioctrl_set_isp_start_info %d %p",param_ptr->mode,handle->handle_af);
	if (handle->handle_af && ((ISP_VIDEO_MODE_CONTINUE == param_ptr->mode))) {
		rtn = alc_af_ioctrl(handle->handle_af,ALC_AF_CMD_SET_ISP_START_INFO,NULL,NULL);
	}

	return rtn;
}

int32_t alc_af_ioctrl_set_ae_awb_info(isp_ctrl_context* handle,
		void* ae_result,
		void* awb_result,
		void* bv,
		void *rgb_statistics)
{
	int rtn = 0;
	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_BV_BY_LUM, NULL,(void *)&bv);
	rtn = alc_af_ioctrl(handle->handle_af,ALC_AF_CMD_SET_AE_INFO,(void *)ae_result,(void *)rgb_statistics);

	return rtn;
}

int32_t alc_af_ioctrl_set_af_mode(isp_handle isp_handler, void* param_ptr, int(*call_back)())
{
	isp_ctrl_context* handle = (isp_ctrl_context*)isp_handler;
	uint32_t set_mode;
	uint32_t rtn;

	switch (*(uint32_t *)param_ptr) {
	case ISP_FOCUS_MACRO:
		set_mode = ALC_AF_MODE_MACRO;
		break;
	case ISP_FOCUS_MANUAL:
		set_mode = ALC_AF_MODE_MANUAL;
		break;
	case ISP_FOCUS_CONTINUE:
		set_mode = ALC_AF_MODE_CAF;
		break;
	case ISP_FOCUS_VIDEO:
		set_mode = ALC_AF_MODE_VIDEO_CAF;
		break;
	default:
		set_mode = ALC_AF_MODE_NORMAL;
		break;
	}

	rtn = alc_af_ioctrl(handle->handle_af, ALC_AF_CMD_SET_AF_MODE,(void*)&set_mode,NULL);

	return rtn;
}

extern struct af_lib_fun af_lib_fun;
void alc_af_fun_init()
{
	af_lib_fun.af_init_interface		= alc_af_init;
	af_lib_fun.af_calc_interface		= alc_af_calc;
	af_lib_fun.af_deinit_interface		= alc_af_deinit;
	af_lib_fun.af_ioctrl_interface		= alc_af_ioctrl;
	af_lib_fun.af_ioctrl_set_af_mode	= alc_af_ioctrl_set_af_mode;
	af_lib_fun.af_ioctrl_set_fd_update	= alc_af_ioctrl_set_fd_update;
	af_lib_fun.af_ioctrl_af_start		= alc_af_ioctrl_af_start;
	af_lib_fun.af_ioctrl_set_isp_start_info	= alc_af_ioctrl_set_isp_start_info;
	af_lib_fun.af_ioctrl_set_ae_awb_info	= alc_af_ioctrl_set_ae_awb_info;

	return;
}
