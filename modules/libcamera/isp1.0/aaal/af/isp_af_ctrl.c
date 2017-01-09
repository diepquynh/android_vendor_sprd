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

#include "isp_com.h"
#include "isp_alg.h"
#include "isp_af_alg_v03.h"
#include "isp_af_alg_v04.h"
//#include<time.h>
#include <stdlib.h>

/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C"
{
#endif

static uint32_t _isp_af_go_position(uint32_t handler_id, uint32_t pos_step);

static void _af_v03_init(uint32_t handler_id)
{
	struct isp_context* isp_context_ptr=ispGetAlgContext(handler_id);
	struct isp_af_param* af_param_ptr=&isp_context_ptr->af;
	uint32_t i;
	struct isp_af_config_v03 *af_v03_cfg = &af_param_ptr->alg_v03_context.af_cfg;

	af_v03_cfg->MAX_STEP = af_param_ptr->max_step;
	af_v03_cfg->INIT_POS=af_param_ptr->min_step;
	af_v03_cfg->STEP_LENGTH = af_param_ptr->default_rough_step_len ? : 64;
	af_v03_cfg->INIT_POS_MACRO=(af_v03_cfg->MAX_STEP*3)>>2;
	af_v03_cfg->STEP_LENGTH_MACRO=(af_v03_cfg->STEP_LENGTH*3)>>3;
	af_v03_cfg->END_POS=af_param_ptr->max_step;
	af_v03_cfg->END_POS_MACRO=af_param_ptr->max_step;
	af_v03_cfg->STEP_TBL_COUNT = af_param_ptr->rough_count;
	for(i=0;i<32;i++){
		af_v03_cfg->STEP_TBL[i] = af_param_ptr->af_rough_step[i];
	}
	if(af_param_ptr->rough_count>0){
		af_v03_cfg->MAX_STEP = af_param_ptr->af_rough_step[af_param_ptr->rough_count - 1];
		af_v03_cfg->INIT_POS = af_param_ptr->af_rough_step[0];
		af_v03_cfg->INIT_POS_MACRO = af_v03_cfg->STEP_TBL[af_param_ptr->rough_count>>1];
		af_v03_cfg->END_POS = af_param_ptr->af_rough_step[af_param_ptr->rough_count - 1];
		af_v03_cfg->END_POS_MACRO = af_param_ptr->af_rough_step[af_param_ptr->rough_count - 1];
	}
	af_v03_cfg->PEAK_VALUE_THR = af_param_ptr->peak_thr_0 ? : 25;
	af_v03_cfg->PEAK_VALUE_THR_1 = af_param_ptr->peak_thr_1 ? : 8;
	af_v03_cfg->PEAK_VALUE_THR_2 = af_param_ptr->peak_thr_2 ? : 6;
	af_v03_cfg->DECTECT_AFM_THR = af_param_ptr->detect_thr ? : 12;
	af_v03_cfg->DECTECT_AWBM_THR = af_param_ptr->detect_thr ? : 12;
	af_v03_cfg->DECTECT_STEP_NUM = af_param_ptr->detect_step_mum ? : 6;
	af_v03_cfg->FINE_STEP_NUM = af_param_ptr->fine_count<3?3:af_param_ptr->fine_count;
	af_v03_cfg->START_AREA_RANGE = af_param_ptr->start_area_range ? : 20;
	af_v03_cfg->END_AREA_RANGE = af_param_ptr->end_area_range ? : 20;
	af_v03_cfg->NOISE_THR = af_param_ptr->noise_thr ? : 3;
	af_v03_cfg->DEBUG = af_param_ptr->debug;
	af_v03_cfg->ANTI_CRASH_POS = af_param_ptr->anti_crash_pos;
	af_param_ptr->alg_v03_context.af_go_position = _isp_af_go_position;
	isp_af_init_v03(handler_id,&af_param_ptr->alg_v03_context);
}

static void _af_v03_deinit(uint32_t handler_id)
{
	isp_af_deinit_v03(handler_id);
}



static void _af_v04_init(uint32_t handler_id)
{
	struct isp_context* isp_context_ptr=ispGetAlgContext(handler_id);
	struct isp_af_param* af_param_ptr=&isp_context_ptr->af;
	uint32_t i;
	struct isp_af_config_v04 *af_v04_cfg = &af_param_ptr->alg_v04_context.af_cfg;

	af_v04_cfg->step_len = af_param_ptr->default_rough_step_len ? : 64;
	af_v04_cfg->step_len_macro=(af_v04_cfg->step_len*3)>>3;
	af_v04_cfg->peak_value_thr = af_param_ptr->peak_thr_0 ? : 25;
	af_v04_cfg->peak_value_thr_1 = af_param_ptr->peak_thr_1 ? : 7;
	af_v04_cfg->peak_value_thr_2 = af_param_ptr->peak_thr_2 ? : 6;
	af_v04_cfg->detect_afm_thr = af_param_ptr->detect_thr ? : 12;
	af_v04_cfg->detect_awbm_thr = af_param_ptr->detect_thr ? : 12;
	af_v04_cfg->detect_step_num = af_param_ptr->detect_step_mum ? : 6;
	af_v04_cfg->fine_step_num = af_param_ptr->fine_count<3?3:af_param_ptr->fine_count;
	af_v04_cfg->start_area_range = af_param_ptr->start_area_range ? : 10;
	af_v04_cfg->end_area_range = af_param_ptr->end_area_range ? : 10;
	af_v04_cfg->noise_thr = af_param_ptr->noise_thr ? : 3;
	af_v04_cfg->video_max_tune_step = af_param_ptr->video_max_tune_step;
	af_v04_cfg->video_speed_ratio = af_param_ptr->video_speed_ratio ? : 60;
	af_v04_cfg->debug = af_param_ptr->debug;

	if (af_param_ptr->rough_count > 0) {
		for (i=0;i<32;i++) {
			af_v04_cfg->rough_step_tbl[i] = af_param_ptr->af_rough_step[i];
			ISP_LOG("hav rough tbl set, step = %d pos = %d",i,af_v04_cfg->rough_step_tbl[i]);
		}
		af_v04_cfg->rough_tbl_cnt = af_param_ptr->rough_count;
		ISP_LOG("hav rough tbl set, cnt = %d",af_v04_cfg->rough_tbl_cnt);
	} else {
		for (i=0;i<32;i++) {
			af_v04_cfg->rough_step_tbl[i] = af_param_ptr->min_step + af_v04_cfg->step_len*i;
			if (af_v04_cfg->rough_step_tbl[i] >= af_param_ptr->max_step) {
				af_v04_cfg->rough_step_tbl[i] = af_param_ptr->max_step;
				break;
			}
		}
		af_v04_cfg->rough_tbl_cnt = i+1;
	}
	af_v04_cfg->max_pos = af_v04_cfg->rough_step_tbl[af_v04_cfg->rough_tbl_cnt - 1];
	af_v04_cfg->init_pos = af_v04_cfg->rough_step_tbl[0];
	af_v04_cfg->init_pos_macro = af_v04_cfg->rough_step_tbl[af_v04_cfg->rough_tbl_cnt>>1];
	af_v04_cfg->end_pos = af_v04_cfg->rough_step_tbl[af_v04_cfg->rough_tbl_cnt - 1];
	af_v04_cfg->end_pos_macro = af_v04_cfg->rough_step_tbl[af_v04_cfg->rough_tbl_cnt - 1];
	af_v04_cfg->anti_crash_pos = af_param_ptr->anti_crash_pos?:af_v04_cfg->rough_step_tbl[1];
	af_v04_cfg->step_len = af_param_ptr->default_rough_step_len ? : (af_v04_cfg->max_pos - af_v04_cfg->init_pos)/(af_v04_cfg->rough_tbl_cnt - 1);

	for(i=0;i<32;i++){
		af_v04_cfg->fine_step_tbl[i] = af_param_ptr->af_fine_step[i];
		if(0 == af_v04_cfg->fine_step_tbl[i])
			break;
	}

	if(i<32){
		ISP_LOG("fine step have zero value,set default step = %d",af_v04_cfg->step_len/(af_param_ptr->fine_count - 1));
		for(i=0;i<32;i++){
			af_v04_cfg->fine_step_tbl[i] = af_v04_cfg->step_len/(af_param_ptr->fine_count - 1);
		}
	}
	af_param_ptr->alg_v04_context.af_go_position = _isp_af_go_position;
	isp_af_init_v04(handler_id,&af_param_ptr->alg_v04_context);

}

static void _af_v04_deinit(uint32_t handler_id)
{
	isp_af_deinit_v04(handler_id);
}


static void _af_debug_info_print(uint32_t handler_id)
{
	struct isp_context* isp_context_ptr=ispGetAlgContext(handler_id);
	struct isp_ae_param* ae_param_ptr=&isp_context_ptr->ae;
	struct isp_af_param* af_param_ptr=&isp_context_ptr->af;
	char file_name[40];
	char tmp_str[10];
	FILE *fp = NULL;
	uint32_t j,k;

	memset(file_name, '\0', 40);
	strcpy(file_name, "/data/af_debug.txt");

	fp = fopen(file_name, "wb");

	if(NULL == fp){
		ISP_LOG("can not open file: %s \n", file_name);
		return;
	}
	ISP_LOG("af debug info file : %s",file_name);

	fprintf(fp,"AF Total step: %d\n",af_param_ptr->step_cnt);
	fprintf(fp,"AF Final pos: %d\n",af_param_ptr->cur_step);
	fprintf(fp,"AF Time: %d ms\n",af_param_ptr->end_time - af_param_ptr->start_time);
	fprintf(fp,"AE Exp Time: %d ms\n",(ae_param_ptr->cur_exposure*ae_param_ptr->line_time)/10000);

	fclose(fp);



}



uint32_t isp_af_init(uint32_t handler_id)
{
	uint32_t rtn=ISP_SUCCESS;
	struct isp_context* isp_context_ptr=ispGetAlgContext(handler_id);
	struct isp_af_param* af_param_ptr=&isp_context_ptr->af;
	uint32_t i;

	ISP_LOG("af alg id:%d",af_param_ptr->alg_id);

	switch(af_param_ptr->alg_id){
		case 0:
			break;

		case 1:

			break;

		case 2:
			break;

		case 3:
			_af_v03_init(handler_id);
			break;

		case 4:
			_af_v04_init(handler_id);
			break;

		default:
			ISP_LOG("af alg id:%d is not support",af_param_ptr->alg_id);
			rtn=ISP_ERROR;
			break;

	}
	isp_context_ptr->af_get_stat=ISP_END_FLAG;
	af_param_ptr->monitor_bypass=ISP_EB;
	af_param_ptr->init=ISP_EB;
	af_param_ptr->status=ISP_AF_STOP;
	af_param_ptr->awbm_flag=ISP_UEB;
	af_param_ptr->afm_flag=ISP_UEB;
	isp_caf_reset(handler_id);

	return rtn;
}

uint32_t isp_af_deinit(uint32_t handler_id)
{
	uint32_t rtn=ISP_SUCCESS;
	struct isp_context* isp_context_ptr=ispGetAlgContext(handler_id);
	struct isp_af_param* af_param_ptr=&isp_context_ptr->af;

	ISP_LOG("af alg id:%d",af_param_ptr->alg_id);

	switch(af_param_ptr->alg_id){
		case 0:
			break;

		case 1:

			break;

		case 2:
			break;

		case 3:
			_af_v03_deinit(handler_id);
			break;

		case 4:
			_af_v04_deinit(handler_id);
			break;

		default:
			ISP_LOG("af alg id:%d is not support",af_param_ptr->alg_id);
			rtn=ISP_ERROR;
			break;
	}
	af_param_ptr->init=ISP_UEB;
	if(af_param_ptr->status==ISP_AF_CONTINUE){
		af_param_ptr->status = ISP_AF_STOP;
		isp_context_ptr->ae.bypass=af_param_ptr->ae_status;
		isp_context_ptr->awb.bypass=af_param_ptr->awb_status;
	}

	return rtn;
}


static uint32_t _isp_af_go_position(uint32_t handler_id, uint32_t pos_step)
{
	int32_t rtn = ISP_SUCCESS;
	struct isp_context* isp_context_ptr=ispGetAlgContext(handler_id);
	struct isp_af_param* af_param_ptr=&isp_context_ptr->af;
	//SENSOR_EXP_INFO_T* sensor_info_ptr=(SENSOR_EXP_INFO_T*)isp_context_ptr->cfg.sensor_info_ptr;
	static uint32_t pos_pre = 0;
	uint32_t stab_period = 0;
	uint32_t skip_frame_mum = af_param_ptr->frame_skip;

	//ISP_LOG("ISP_RAW: hait0: pos_pre: %d, stab_period: %d, max_period: %d, min_period:%d\n", pos_pre, af_param_ptr->stab_period, af_param_ptr->max_step, af_param_ptr->min_step);
	//ISP_LOG("ISP_RAW: 0x%x, 0x%x, 0x%x", sensor_info_ptr, sensor_info_ptr->ioctl_func_ptr, sensor_info_ptr->ioctl_func_ptr->af_enable);

	if(PNULL != af_param_ptr->go_position)
	{
		if(pos_step > af_param_ptr->max_step){
			pos_step = af_param_ptr->max_step;
		}
		af_param_ptr->go_position((skip_frame_mum<<16) | (pos_step&0xffff));
		stab_period = (pos_step > pos_pre?pos_step - pos_pre:pos_pre - pos_step)*af_param_ptr->stab_period/(af_param_ptr->max_step-af_param_ptr->min_step?:1);
		//ISP_LOG("ISP_RAW: hait1: stab_period: %d\n",stab_period);
		stab_period = (stab_period>af_param_ptr->stab_period/3)?af_param_ptr->stab_period:af_param_ptr->stab_period/3;
		//ISP_LOG("ISP_RAW:stab_period: %d\n",stab_period);
		//usleep(1000*(stab_period>10?stab_period:10));
		pos_pre = pos_step;
		af_param_ptr->cur_step = pos_step;
	} else {
		ISP_LOG("ISP_RAW: set_focus null error");
	}

	return rtn;

}

static uint32_t scan_temp_buffer[1024][10];

uint32_t isp_af_calculation(uint32_t handler_id)
{
	uint32_t rtn=ISP_SUCCESS;
	struct isp_context* isp_context_ptr=ispGetAlgContext(handler_id);
	struct isp_af_param* af_param_ptr=&isp_context_ptr->af;
	struct isp_af_notice af_notice={0x00};
	uint8_t af_mode=0;
	uint32_t suc_win = 0;
	uint32_t skip_num = 0;
	uint32_t cur_time;
	uint32_t i;
	struct timespec ts;


	if(ISP_EB==af_param_ptr->bypass)
	{
		return rtn;
	}

	if (clock_gettime(CLOCK_MONOTONIC, &ts)) {
		ISP_LOG("giet time fail!!!");
	} else {
		cur_time = ts.tv_sec*1000 + ts.tv_nsec/1000000;
	}

	if (ISP_AF_START == af_param_ptr->status) {
		af_param_ptr->step_cnt = 1;
	} else if (ISP_AF_CONTINUE== af_param_ptr->status) {
		af_param_ptr->step_cnt++;
	}
	if(af_param_ptr->step_cnt <= 32){
		af_param_ptr->time[af_param_ptr->step_cnt - 1] = cur_time - af_param_ptr->start_time;
		af_param_ptr->pos[af_param_ptr->step_cnt - 1] = af_param_ptr->cur_step;
		for (i=0;i<9;i++) {
			af_param_ptr->af_value[i][af_param_ptr->step_cnt - 1] = isp_context_ptr->af_stat.info[i];
		}
	}

	switch(af_param_ptr->alg_id){
		case 0:
			break;

		case 1:

			break;

		case 2:
			break;

		case 3:
			if (ISP_FOCUS_MACRO == af_param_ptr->mode){
				af_mode=ISP_AF3_MACRO;
			}else if(((ISP_FOCUS_CONTINUE == af_param_ptr->mode) || (ISP_FOCUS_VIDEO == af_param_ptr->mode))
				&&(ISP_END_FLAG != isp_context_ptr->af.continue_status)
				&&(ISP_AF_STOP != isp_context_ptr->af.status)){
				af_param_ptr->status = ISP_AF_FINISH;
				af_param_ptr->suc_win = 1;
				rtn=af_param_ptr->suc_win|ISP_AF_END_FLAG;
				break;
			}else{
				af_mode=ISP_AF3_NORMAL;
			}

			if((ISP_AF_START == af_param_ptr->status) || (ISP_AF_CONTINUE== af_param_ptr->status)){
				rtn = isp_af3_calculation(handler_id,
						af_param_ptr->status==ISP_AF_START?ISP_AF3_START_TRG:ISP_AF3_RUNING,
						(uint32_t*)isp_context_ptr->af_stat.info,
						af_param_ptr->valid_win,
						af_param_ptr->win,
						(uint32_t*)&isp_context_ptr->awb_stat,
						af_param_ptr->awbm_win_w,
						af_param_ptr->awbm_win_h,
						af_mode,
						af_param_ptr->win_priority,
						af_param_ptr->win_sel_mode,
						&suc_win,
						af_param_ptr->cur_step);

				if(ISP_AF3_QUIT==rtn)
				{
					rtn=ISP_SUCCESS;
				}
				else if(ISP_AF3_RUNING==rtn)
				{
					af_param_ptr->status = ISP_AF_CONTINUE;
					//af_param_ptr->monitor_bypass=ISP_UEB;
					rtn=ISP_SUCCESS;
					//af_param_ptr->AfmEb(handler_id,skip_num);
				} else {
					af_param_ptr->status = ISP_AF_FINISH;
					af_param_ptr->have_success_record = ISP_EB;
					af_param_ptr->suc_win = (rtn == ISP_AF3_END_SUCCES)?suc_win:0;
					rtn=af_param_ptr->suc_win|ISP_AF_END_FLAG;
				}

			}else{
				rtn=ISP_SUCCESS;
			}
			break;

		case 4:
			if(((ISP_FOCUS_CONTINUE == af_param_ptr->mode) || (ISP_FOCUS_VIDEO == af_param_ptr->mode))
				&&(ISP_END_FLAG != isp_context_ptr->af.continue_status)
				&&(ISP_AF_STOP != isp_context_ptr->af.status)){
				ISP_LOGE("af status: %d.%ld,%ld",af_param_ptr->mode,isp_context_ptr->af.continue_status,isp_context_ptr->af.status);
				af_param_ptr->status = ISP_AF_FINISH;
				af_param_ptr->suc_win = 1;
				rtn=af_param_ptr->suc_win|ISP_AF_END_FLAG;
				break;
			}else{
				switch (af_param_ptr->mode) {
				case ISP_FOCUS_MACRO:
					af_mode=ISP_AF4_MACRO;
					break;

				case ISP_FOCUS_CONTINUE:
					af_mode=ISP_AF4_CONTINUE;
					break;

				case ISP_FOCUS_VIDEO:
					af_mode=ISP_AF4_VIDEO;
					break;

				default:
					af_mode=ISP_AF4_NORMAL;
					break;
				}
			}

			if((ISP_AF_START == af_param_ptr->status) || (ISP_AF_CONTINUE== af_param_ptr->status)){
				struct af_input_param_struct_v04 in_param;
				struct af_output_param_struct_v04 out_param;

				in_param.af_ctrl.af_status = af_param_ptr->status==ISP_AF_START?ISP_AF4_START_TRG:ISP_AF4_RUNING;
				in_param.af_ctrl.af_mode = af_mode;
				in_param.af_ctrl.af_win_priority = af_param_ptr->win_priority;
				in_param.af_ctrl.af_win_sel_mode = af_param_ptr->win_sel_mode;
				in_param.af_ctrl.cur_motor_pos = af_param_ptr->cur_step;

				in_param.afm_info.af_win_value = (uint32_t*)isp_context_ptr->af_stat.info;
				in_param.afm_info.af_win_num = af_param_ptr->valid_win;
				in_param.afm_info.af_win_size = af_param_ptr->win;

				in_param.awbm_info.awbm_value = (uint32_t*)&isp_context_ptr->awb_stat;
				in_param.awbm_info.awbm_win_w = isp_context_ptr->awbm.win_size.w;
				in_param.awbm_info.awbm_win_h = isp_context_ptr->awbm.win_size.h;
				in_param.awbm_info.awbm_win_x = isp_context_ptr->awbm.win_start.x;
				in_param.awbm_info.awbm_win_y = isp_context_ptr->awbm.win_start.y;

				in_param.ae_info.exp_time = isp_context_ptr->ae.cur_exposure*isp_context_ptr->ae.line_time/10;
				in_param.ae_info.max_fps = 30;

				out_param.new_position = 0;
				out_param.rtn_suc_win = 0;
				out_param.skip_num = &af_param_ptr->frame_skip;

				if (af_param_ptr->frame_skip) {
					af_param_ptr->frame_skip--;
					rtn = ISP_SUCCESS;
				} else {
					rtn = isp_af4_calculation(handler_id,
								&in_param,
								&out_param);
					if(ISP_AF4_RUNING==rtn)
					{
						af_param_ptr->status = ISP_AF_CONTINUE;
						//af_param_ptr->monitor_bypass=ISP_UEB;
						rtn=ISP_SUCCESS;
						//af_param_ptr->AfmEb(handler_id,out_param.skip_num);
					} else {
						af_param_ptr->status = ISP_AF_FINISH;
						af_param_ptr->have_success_record = ISP_EB;
						af_param_ptr->suc_win = (rtn == ISP_AF4_END_SUCCES)?out_param.rtn_suc_win:0;
						rtn=af_param_ptr->suc_win|ISP_AF_END_FLAG;
					}
				}

			}else{
				rtn=ISP_SUCCESS;
			}
			break;

		default:
			ISP_LOG("af alg id:%d is not support",af_param_ptr->alg_id);
			rtn=ISP_ERROR;
			break;

	}

//	ISP_LOG("af mode:%d, suc_win: 0x%x, vali_win, max step: %d, min step: %d, cur step: %d, ret = %d\n",\
//		af_param_ptr->mode, af_param_ptr->suc_win, af_param_ptr->valid_win, af_param_ptr->max_step, af_param_ptr->min_step, af_param_ptr->cur_step, rtn);


	return rtn;
}

/* isp_af_end --
*@
*@
*@ return:
*/
uint32_t isp_af_end(uint32_t handler_id, uint8_t stop_mode)
{
	uint32_t rtn=ISP_SUCCESS;
	uint8_t af_mode=0;
	struct isp_context* isp_context_ptr=ispGetAlgContext(handler_id);
	struct isp_ae_param* ae_param_ptr=&isp_context_ptr->ae;
	struct isp_af_param* af_param_ptr=&isp_context_ptr->af;
	struct timespec          ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts)) {
		ISP_LOG("giet time fail!!!");
	} else {
		af_param_ptr->end_time = ts.tv_sec*1000 + ts.tv_nsec/1000000;
	}

	if(ISP_AF_STOP == af_param_ptr->status){
		return rtn;
	}
	af_param_ptr->status=ISP_AF_STOP;
	isp_context_ptr->awb_win_conter = 0;
	af_param_ptr->end_handler_flag = ISP_UEB;
	ISP_LOG("ae_status %d",af_param_ptr->ae_status);

	isp_caf_reset(handler_id);
	isp_context_ptr->af.continue_status=ISP_END_FLAG;

	if (af_param_ptr->debug) {
		_af_debug_info_print(handler_id);
	}

	isp_context_ptr->ae.bypass = isp_context_ptr->af.ae_status;
	isp_context_ptr->awb.bypass = isp_context_ptr->af.awb_status;

	return rtn;
}


uint32_t isp_af_get_mode(uint32_t handler_id, uint32_t *mode)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_context* isp_context_ptr=ispGetAlgContext(handler_id);
	struct isp_af_param* af_param_ptr=&isp_context_ptr->af;

	*mode = af_param_ptr->mode;
	return rtn;
}

uint32_t isp_af_pos_reset(uint32_t handler_id, uint32_t mode)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_context* isp_context_ptr=ispGetAlgContext(handler_id);
	struct isp_af_param* af_param_ptr=&isp_context_ptr->af;
	uint32_t set_pos;

	switch (mode) {
	case ISP_FOCUS_MACRO:
		_isp_af_go_position(handler_id,af_param_ptr->max_step);
		break;

	case ISP_FOCUS_CONTINUE:
		break;

	case ISP_FOCUS_VIDEO:
		break;

	default:
#if 0
		if (af_param_ptr->cur_step <= af_param_ptr->anti_crash_pos) {
			set_pos = af_param_ptr->min_step;
		} else if (af_param_ptr->min_step < af_param_ptr->anti_crash_pos) {
			set_pos = af_param_ptr->anti_crash_pos;
		} else {
			set_pos = af_param_ptr->min_step;
		}
		af_param_ptr->frame_skip = 1;
		_isp_af_go_position(handler_id,set_pos);
#endif
		break;

	}

	return rtn;


}

uint32_t isp_af_set_mode(uint32_t handler_id, uint32_t mode)
{
	int32_t rtn=ISP_SUCCESS;
	struct isp_context* isp_context_ptr=ispGetAlgContext(handler_id);
	struct isp_af_param* af_param_ptr=&isp_context_ptr->af;

	if ((ISP_EB == af_param_ptr->init) && (af_param_ptr->mode != mode)) {
		isp_af_pos_reset(handler_id,mode);
	}

	af_param_ptr->mode = mode;
	return rtn;
}




/* isp_af_set_postion --
*@
*@
*@ return:
*/
uint32_t isp_af_set_postion(uint32_t handler_id, uint32_t step)
{
	uint32_t rtn=ISP_SUCCESS;

	_isp_af_go_position(handler_id, step);

	return rtn;
}

/* isp_af_get_stat_value --
*@
*@
*@ return:
*/
uint32_t isp_af_get_stat_value(uint32_t handler_id, void* param_ptr)
{
	uint32_t rtn=ISP_SUCCESS;
	struct isp_context* isp_context_ptr=ispGetAlgContext(handler_id);

	if (NULL!=param_ptr) {
		memcpy(param_ptr, (void*)&isp_context_ptr->af_stat.info, sizeof(struct isp_af_statistic_info));
	}

	return rtn;
}

/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/
#ifdef	__cplusplus
}
#endif
/*------------------------------------------------------------------------------*/
