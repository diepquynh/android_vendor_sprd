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

//#include<time.h>
#include <stdlib.h>
#include "af_ctrl.h"
#include "af_alg.h"
#include "isp_com.h"

/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/
#define     UNUSED(param)  (void)(param)
#ifdef __cplusplus
extern "C"
{
#endif

/**---------------------------------------------------------------------------*
**				Micro Define					*
**----------------------------------------------------------------------------*/


static int32_t _caf_reset(af_handle_t handle);


static int32_t _check_handle(af_handle_t handle)
{
	int32_t rtn = AF_SUCCESS;
	struct af_context_t *cxt = (struct af_context_t *)handle;

	if (NULL == cxt) {
		AF_LOGE("invalid cxt pointer");
		return AF_HANDLER_NULL;
	}

	if (AF_MAGIC_START != cxt->magic_start
		|| AF_MAGIC_END != cxt->magic_end) {
		AF_LOGE("invalid magic begin = 0x%x, magic end = 0x%x",
					cxt->magic_start, cxt->magic_end);
		return AF_HANDLER_CXT_ERROR;
	}

	return rtn;
}

static int32_t _af_set_motor_pos(af_handle_t handle, uint32_t motot_pos)
{
	int32_t rtn = AF_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;
	struct af_motor_pos set_pos;

	rtn = _check_handle(handle);
	if (AF_SUCCESS != rtn) {
		AF_LOGE("_check_cxt failed");
		return AF_ERROR;
	}

	set_pos.motor_pos = motot_pos;
	set_pos.skip_frame = 0;
	set_pos.wait_time = 0;
	af_cxt->go_position(af_cxt->caller,&set_pos);
	af_cxt->cur_af_pos = motot_pos;

	return rtn;
}


static int32_t _alg_init(af_handle_t handle,
				struct af_init_in_param *init_param,
				struct af_init_result *result)
{
	int32_t rtn = AF_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;
	struct af_alg_init_param alg_init_param;
	struct af_alg_init_result alg_result;
	uint32_t i;

	rtn = _check_handle(handle);
	if (AF_SUCCESS != rtn) {
		AF_LOGE("_check_cxt failed");
		return AF_ERROR;
	}

	alg_init_param.tuning_param_cnt = init_param->tuning_param_cnt;
	alg_init_param.cur_tuning_mode = init_param->cur_tuning_mode;
	alg_init_param.init_mode= init_param->af_mode;
	alg_init_param.plat_info.afm_filter_type_cnt = init_param->plat_info.afm_filter_type_cnt;
	alg_init_param.plat_info.afm_win_max_cnt = init_param->plat_info.afm_win_max_cnt;
	if (init_param->tuning_param_cnt > 0) {
		alg_init_param.tuning_param = (struct af_alg_tuning_block_param *)malloc(sizeof(*alg_init_param.tuning_param)*init_param->tuning_param_cnt);
		if (NULL == alg_init_param.tuning_param) {
			AF_LOGE("mem alloc for tuning_param error !!!");
			return AF_ERROR;
		}
		for (i=0; i<init_param->tuning_param_cnt; i++) {
			alg_init_param.tuning_param[i].data = init_param->tuning_param[i].data;
			alg_init_param.tuning_param[i].data_len = init_param->tuning_param[i].data_len;
			alg_init_param.tuning_param[i].cfg_mode = init_param->tuning_param[i].cfg_mode;
		}
	}

	af_cxt->af_alg_handle = af_alg_init(&alg_init_param,&alg_result);
	free(alg_init_param.tuning_param);
	if (NULL == af_cxt->af_alg_handle) {
		AF_LOGE("init af_alg_handle error !!!");
		return AF_ERROR;
	}
	result->init_motor_pos = alg_result.init_motor_pos;
	return rtn;

}

static int32_t _cfg_af_calc_param(af_handle_t handle,
				struct af_calc_param *param,
				struct af_alg_calc_param *alg_calc_param)
{
	int32_t rtn = AF_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;
	uint32_t i;

	rtn = _check_handle(handle);
	if (AF_SUCCESS != rtn) {
		AF_LOGE("_check_cxt failed");
		return AF_ERROR;
	}

	switch (param->data_type) {
	case AF_DATA_AF:
		alg_calc_param->active_data_type = AF_ALG_DATA_AF;
		{
			struct af_filter_info *filter_info = (struct af_filter_info *)param->data;
			alg_calc_param->afm_info.win_cfg.win_cnt = af_cxt->win_cfg.win_cnt;
			alg_calc_param->afm_info.win_cfg.win_sel_mode = af_cxt->win_cfg.win_sel_mode;
			if (0 == alg_calc_param->afm_info.win_cfg.win_cnt) {
				alg_calc_param->afm_info.win_cfg.win_cnt = 1;
			}
			for (i=0; i<af_cxt->win_cfg.win_cnt; i++) {
				alg_calc_param->afm_info.win_cfg.win_pos[i].sx = af_cxt->win_cfg.win_pos[i].sx;
				alg_calc_param->afm_info.win_cfg.win_pos[i].sy = af_cxt->win_cfg.win_pos[i].sy;
				alg_calc_param->afm_info.win_cfg.win_pos[i].ex = af_cxt->win_cfg.win_pos[i].ex;
				alg_calc_param->afm_info.win_cfg.win_pos[i].ey = af_cxt->win_cfg.win_pos[i].ey;
				alg_calc_param->afm_info.win_cfg.win_prio[i] = af_cxt->win_cfg.win_prio[i];
			}
			alg_calc_param->afm_info.filter_info.filter_num = filter_info->filter_num;
			for (i=0; i<filter_info->filter_num; i++) {
				alg_calc_param->afm_info.filter_info.filter_data[i].type = filter_info->filter_data[i].type;
				alg_calc_param->afm_info.filter_info.filter_data[i].data = filter_info->filter_data[i].data;
			}

		}
		break;

	case AF_DATA_IMG_BLK:
		alg_calc_param->active_data_type = AF_ALG_DATA_IMG_BLK;
		{
			struct af_img_blk_info *img_blk_info = (struct af_img_blk_info *)param->data;
			alg_calc_param->img_blk_info.block_w = img_blk_info->block_w;
			alg_calc_param->img_blk_info.block_h = img_blk_info->block_h;
			alg_calc_param->img_blk_info.pix_per_blk = img_blk_info->pix_per_blk;
			alg_calc_param->img_blk_info.chn_num = img_blk_info->chn_num;
			alg_calc_param->img_blk_info.data = img_blk_info->data;
		}
		break;

	case AF_DATA_AE:
		alg_calc_param->active_data_type = AF_ALG_DATA_AE;
		{
			struct af_ae_info *ae_info = (struct af_ae_info *)param->data;
			alg_calc_param->ae_info.exp_time = ae_info->exp_time;
			alg_calc_param->ae_info.gain = ae_info->gain;
			alg_calc_param->ae_info.cur_fps = ae_info->cur_fps;
			alg_calc_param->ae_info.cur_lum = ae_info->cur_lum;
			alg_calc_param->ae_info.target_lum = ae_info->target_lum;
			alg_calc_param->ae_info.is_stable = ae_info->is_stable;
		}
		break;

	case AF_DATA_FD:
		alg_calc_param->active_data_type = AF_ALG_DATA_FD;
		{
			struct af_fd_info *fd_info = (struct af_fd_info *)param->data;
			alg_calc_param->fd_info.face_num = fd_info->face_num;
			for (i=0; i<fd_info->face_num; i++) {
				alg_calc_param->fd_info.face_pose[i].sx = fd_info->face_pose[i].sx;
				alg_calc_param->fd_info.face_pose[i].sy = fd_info->face_pose[i].sy;
				alg_calc_param->fd_info.face_pose[i].ex = fd_info->face_pose[i].ex;
				alg_calc_param->fd_info.face_pose[i].ey = fd_info->face_pose[i].ey;
			}
		}
		break;

	default:
		AF_LOGE("unsupport data type! type: %d",param->data_type);
		rtn = AF_ERROR;
		break;
	}


	return rtn;
}

static int32_t _af_cfg_afm_win(af_handle_t handle, uint32_t type, uint32_t win_num, struct af_win_rect *win_pos)
{
	int32_t rtn = AF_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;
	isp_ctrl_context *ctrl_context = NULL;
	uint32_t hw_max_win_num;
	uint32_t i;
	uint32_t w,h;
	struct af_win_rect set_pos[MAX_AF_WIN];
	struct af_monitor_win monitor_win;

	rtn = _check_handle(handle);
	if (AF_SUCCESS != rtn) {
		AF_LOGE("_check_cxt failed");
		return AF_ERROR;
	}
	ctrl_context = (isp_ctrl_context *)af_cxt->caller;

	rtn = af_cxt->get_monitor_win_num(af_cxt->caller,&hw_max_win_num);
	hw_max_win_num = hw_max_win_num > MAX_AF_WIN ? MAX_AF_WIN : hw_max_win_num;

	AF_LOGE("max hw win num %d, cfg win num %d",hw_max_win_num,win_num);
	AF_LOGE("isp w %d, h %d",ctrl_context->src.w,ctrl_context->src.h);

	if (0 == win_num) {
		w = ctrl_context->src.w;
		h = ctrl_context->src.h;
		win_num = 1;
		set_pos[0].sx =((((w>>1)-(w/10))>>1)<<1);
		set_pos[0].sy =((((h>>1)-(h/10))>>1)<<1);
		set_pos[0].ex =((((w>>1)+(w/10))>>1)<<1);
		set_pos[0].ey =((((h>>1)+(h/10))>>1)<<1);



	} else {
		for (i=0; i<win_num; i++) {
			set_pos[i] = win_pos[i];
		}
	}

	af_cxt->win_cfg.win_cnt = win_num;
	af_cxt->win_cfg.win_sel_mode = 0;
	for (i=0; i<win_num; i++) {
		af_cxt->win_cfg.win_pos[i].sx = set_pos[i].sx;
		af_cxt->win_cfg.win_pos[i].sy = set_pos[i].sy;
		af_cxt->win_cfg.win_pos[i].ex = set_pos[i].ex;
		af_cxt->win_cfg.win_pos[i].ey = set_pos[i].ey;
		af_cxt->win_cfg.win_prio[i] = 1;
	}


	AF_LOGE("sx w %d, sy %d,ex w %d, ey %d",set_pos[0].sx,set_pos[0].sy,set_pos[0].ex,set_pos[0].ey);


	for (i=win_num; i<hw_max_win_num; i++) {
		set_pos[i] = set_pos[0];
	}
	monitor_win.type = type;
	monitor_win.win_pos = set_pos;
	rtn = af_cxt->set_monitor_win(af_cxt->caller,&monitor_win);
	return rtn;


}

static int32_t _af_set_mode(af_handle_t handle, void *in_param)
{
	int32_t rtn = AF_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;
	uint32_t af_mode = *(uint32_t*)in_param;
	uint32_t set_mode;
	uint32_t init_pos;

	rtn = _check_handle(handle);
	if (AF_SUCCESS != rtn) {
		AF_LOGE("_check_cxt failed");
		return AF_ERROR;
	}

	if (0 == af_cxt->flash_on) {
		AF_LOGE("mode %d",af_mode);
		switch (af_mode) {
		case AF_MODE_NORMAL:
			set_mode = AF_ALG_MODE_NORMAL;
			break;
		case AF_MODE_MACRO:
			set_mode = AF_ALG_MODE_MACRO;
			break;
		case AF_MODE_CONTINUE:
			set_mode = AF_ALG_MODE_CONTINUE;
			break;
		case AF_MODE_VIDEO:
			set_mode = AF_ALG_MODE_VIDEO;
			break;
		case AF_MODE_MANUAL:
			set_mode = AF_ALG_MODE_NORMAL;
			break;
		default:
			AF_LOGE("AF mode: %d not support !!!",af_mode);
			rtn = AF_PARAM_ERROR;
			break;
		}
		if (AF_SUCCESS == rtn) {
			rtn = af_alg_ioctrl(af_cxt->af_alg_handle,AF_ALG_CMD_SET_AF_MODE,(void*)&set_mode,NULL);
			rtn = af_alg_ioctrl(af_cxt->af_alg_handle,AF_ALG_CMD_GET_AF_INIT_POS,(void*)&init_pos,NULL);
#if 0
			if ((af_cxt->af_mode != af_mode) && (AF_MODE_MANUAL != af_mode) && (AF_MODE_MANUAL != af_cxt->af_mode) ) {
				_af_set_motor_pos(handle,init_pos);
			}
#endif
			if ((af_mode == AF_ALG_MODE_CONTINUE) || (af_mode == AF_ALG_MODE_VIDEO)) {
				rtn = af_alg_ioctrl(af_cxt->af_alg_handle,AF_ALG_CMD_SET_CAF_RESET,NULL,NULL);
			} else {
				rtn = af_alg_ioctrl(af_cxt->af_alg_handle,AF_ALG_CMD_SET_CAF_STOP,NULL,NULL);
			}

		}
		af_cxt->af_mode = af_mode;
	}

	return rtn;

}

static int32_t _af_set_status(af_handle_t handle, uint32_t af_status)
{
	int32_t rtn = AF_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;
	uint32_t set_status;

	rtn = _check_handle(handle);
	if (AF_SUCCESS != rtn) {
		AF_LOGE("_check_cxt failed");
		return AF_ERROR;
	}

	switch (af_status) {
	case AF_STATUS_START:
		set_status = AF_ALG_STATUS_START;
		break;
	case AF_STATUS_RUNNING:
		set_status = AF_ALG_STATUS_RUNNING;
		break;
	case AF_STATUS_FINISH:
		set_status = AF_ALG_STATUS_FINISH;
		break;
	case AF_STATUS_PAUSE:
		set_status = AF_ALG_STATUS_PAUSE;
		break;
	case AF_STATUS_RESUME:
		set_status = AF_ALG_STATUS_RESUME;
		break;
	case AF_STATUS_RESTART:
		set_status = AF_ALG_STATUS_RESTART;
		break;
	case AF_STATUS_STOP:
		set_status = AF_ALG_STATUS_STOP;
		break;
	default:
		AF_LOGE("AF status: %d not support !!!",af_status);
		rtn = AF_PARAM_ERROR;
		break;
	}
	if (AF_SUCCESS == rtn) {
		rtn = af_alg_ioctrl(af_cxt->af_alg_handle,AF_ALG_CMD_SET_AF_STATUS,(void*)&set_status,NULL);
	}

	return rtn;

}

static int32_t _af_set_start(af_handle_t handle, struct af_trig_info *trig_info )
{
	int32_t rtn = AF_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;
	struct af_monitor_set monitor_set;
	uint32_t af_pos;

	rtn = _check_handle(handle);
	if (AF_SUCCESS != rtn) {
		AF_LOGE("_check_cxt failed");
		return AF_ERROR;
	}
	rtn = af_alg_ioctrl(af_cxt->af_alg_handle,AF_ALG_CMD_GET_AF_INIT_POS,(void*)&af_pos,NULL);
	rtn = _af_set_motor_pos(handle, af_pos);

	if (0 == af_cxt->ae_awb_lock_cnt) {
		rtn = af_cxt->ae_awb_lock(af_cxt->caller);
		af_cxt->ae_awb_lock_cnt++;
	}

	af_cxt->is_running = AF_TRUE;
	rtn = _af_cfg_afm_win(handle,1,trig_info->win_num,trig_info->win_pos);
	rtn = _af_set_status(handle,AF_ALG_STATUS_START);
	monitor_set.bypass = 0;
	monitor_set.int_mode = 1;
	monitor_set.need_denoise = 0;
	monitor_set.skip_num = 0;
	monitor_set.type = 1;
	rtn = af_cxt->set_monitor(af_cxt->caller,&monitor_set,af_cxt->cur_envi);
	return rtn;
}


static int32_t _af_end_proc(af_handle_t handle, struct af_result_param *result, uint32_t need_notice)
{
	int32_t rtn = AF_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;
	struct af_monitor_set monitor_set;

	rtn = _check_handle(handle);
	if (AF_SUCCESS != rtn) {
		AF_LOGE("_check_cxt failed");
		return AF_ERROR;
	}
	monitor_set.bypass = 1;
	monitor_set.int_mode = 1;
	monitor_set.need_denoise = 0;
	monitor_set.skip_num = 0;
	monitor_set.type = 1;
	rtn = af_cxt->set_monitor(af_cxt->caller,&monitor_set,af_cxt->cur_envi);
	if (need_notice) {
		af_cxt->end_notice(af_cxt->caller,result);
	}
	af_cxt->af_result = *result;
	af_cxt->af_has_suc_rec = AF_TRUE;
	if (af_cxt->ae_awb_lock_cnt) {
		af_cxt->ae_awb_release(af_cxt->caller);
		af_cxt->ae_awb_lock_cnt--;
	}
	af_cxt->is_running = AF_FALSE;

	return rtn;
}

static int32_t _af_finish(af_handle_t handle, struct af_result_param *result )
{
	int32_t rtn = AF_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;

	rtn = _check_handle(handle);
	if (AF_SUCCESS != rtn) {
		AF_LOGE("_check_cxt failed");
		return AF_ERROR;
	}
	rtn = af_ioctrl(handle,AF_CMD_SET_AF_FINISH,(void*)result,NULL);

	return rtn;
}

static int32_t _caf_trig_af_start(af_handle_t handle)
{
	int32_t rtn = AF_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;
	struct af_trig_info trig_info;

	rtn = _check_handle(handle);
	if (AF_SUCCESS != rtn) {
		AF_LOGE("_check_cxt failed");
		return AF_ERROR;
	}

	trig_info.win_num = 0;
	trig_info.mode = af_cxt->af_mode;

	rtn = af_ioctrl(handle,AF_CMD_SET_CAF_TRIG_START,(void*)&trig_info,NULL);

	return rtn;
}

static int32_t _caf_reset(af_handle_t handle)
{
	int32_t rtn = AF_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;

	rtn = _check_handle(handle);
	if (AF_SUCCESS != rtn) {
		AF_LOGE("_check_cxt failed");
		return AF_ERROR;
	}

	rtn = af_ioctrl(handle,AF_CMD_SET_CAF_RESET,NULL,NULL);

	return rtn;
}

static int32_t _caf_stop(af_handle_t handle)
{
	int32_t rtn = AF_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;

	rtn = _check_handle(handle);
	if (AF_SUCCESS != rtn) {
		AF_LOGE("_check_cxt failed");
		return AF_ERROR;
	}

	rtn = af_ioctrl(handle,AF_CMD_SET_CAF_STOP,NULL,NULL);

	return rtn;
}

static int32_t _caf_reset_after_af(af_handle_t handle)
{
	int32_t rtn = AF_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;

	rtn = _check_handle(handle);
	if (AF_SUCCESS != rtn) {
		AF_LOGE("_check_cxt failed");
		return AF_ERROR;
	}

	if ((AF_MODE_CONTINUE== af_cxt->af_mode) || (AF_MODE_VIDEO == af_cxt->af_mode)) {
		rtn = _caf_reset(handle);
	}

	return rtn;
}



af_handle_t af_init(struct af_init_in_param *init_param,
			struct af_init_result *result)
{
	struct af_context_t *af_cxt = NULL;
	int32_t rtn = AF_SUCCESS;

	if (NULL == init_param) {
		AF_LOGE("init_param error!!init_param : %p , result : %p  !!!", init_param, result);
		goto INIT_ERROR_EXIT;
	}

	af_cxt = (struct af_context_t *)malloc(sizeof(struct af_context_t));
	if (NULL == af_cxt) {
		AF_LOGE("Malloc fail!!!");
		goto INIT_ERROR_EXIT;
	}
	AF_LOGE("af_init start");
	memset(af_cxt,0,sizeof(struct af_context_t));
	af_cxt->magic_start = AF_MAGIC_START;
	af_cxt->magic_end   = AF_MAGIC_END;
	af_cxt->af_mode = init_param->af_mode;
	af_cxt->bypass = init_param->af_bypass;
	af_cxt->caller = init_param->caller;
	af_cxt->go_position = init_param->go_position;
	af_cxt->end_notice= init_param->end_notice;
	af_cxt->start_notice= init_param->start_notice;
	af_cxt->set_monitor= init_param->set_monitor;
	af_cxt->set_monitor_win= init_param->set_monitor_win;
	af_cxt->get_monitor_win_num = init_param->get_monitor_win_num;
	af_cxt->ae_awb_lock = init_param->ae_awb_lock;
	af_cxt->ae_awb_release = init_param->ae_awb_release;
	af_cxt->plat_info = init_param->plat_info;
	af_cxt->cur_envi = AF_ENVI_INDOOR;
	af_cxt->bv_thr[0] = 60;
	af_cxt->bv_thr[1] = 150;
	af_cxt->rgbdiff_thr[0] = 8;
	af_cxt->rgbdiff_thr[1] = 8;
	af_cxt->rgbdiff_thr[2] = 8;

	rtn = _alg_init((af_handle_t)af_cxt,init_param,result);
	if (AF_SUCCESS != rtn) {
		AF_LOGE("af_alg_handle init fail!!!");
		goto INIT_ERROR_EXIT;
	}

	af_cxt->init_flag = AF_TRUE;
	af_cxt->af_has_suc_rec = AF_FALSE;
	af_cxt->ae_awb_lock_cnt = 0;
	//_af_set_motor_pos((af_handle_t)af_cxt,result->init_motor_pos);
	pthread_mutex_init(&af_cxt->status_lock, NULL);

	AF_LOGE("af_init end ");
	return (af_handle_t)af_cxt;

INIT_ERROR_EXIT:
	if (af_cxt) {
		rtn = af_deinit((af_handle_t)af_cxt,NULL,NULL);
		if(rtn){
			AF_LOGE("af deinit error rtn:%d !!!",rtn);
		}
	}
	af_cxt = NULL;
	return (af_handle_t)af_cxt;
}

int32_t af_deinit(af_handle_t handle, void *param, void *result)
{
	UNUSED(param);
	UNUSED(result);
	int32_t rtn = AF_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;
	struct af_monitor_set monitor_set;

	rtn = _check_handle(handle);
	if (AF_SUCCESS != rtn) {
		AF_LOGE("_check_cxt failed");
		return AF_ERROR;
	}

	af_cxt = (struct af_context_t *)handle;

	monitor_set.bypass = 1;
	monitor_set.int_mode = 1;
	monitor_set.need_denoise = 0;
	monitor_set.skip_num = 0;
	monitor_set.type = 1;
	rtn = af_cxt->set_monitor(af_cxt->caller,&monitor_set,af_cxt->cur_envi);

	if (af_cxt->af_alg_handle) {
		rtn = af_alg_deinit(af_cxt->af_alg_handle);
		if(rtn){
			AF_LOGE("af_alg_deinit error rtn:%d !!!",rtn);
		}
	}
	pthread_mutex_destroy(&af_cxt->status_lock);

	memset(af_cxt,0,sizeof(*af_cxt));
	free(af_cxt);

	return rtn;

}


int32_t af_calculation(af_handle_t handle,
				struct af_calc_param *param,
				struct af_result_param *result)
{
	int32_t rtn = AF_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;
	struct af_alg_calc_param alg_calc_param;
	struct af_alg_result alg_calc_result;


	rtn = _check_handle(handle);
	if (AF_SUCCESS != rtn) {
		AF_LOGE("_check_cxt failed");
		return AF_ERROR;
	}

	if(1==af_cxt->isp_tool_af_test){// isp tool af test
		AF_LOGE("ISP_TOOL_AF_TEST");
		return rtn;
	}

	_cfg_af_calc_param(handle,param,&alg_calc_param);
	alg_calc_result = af_cxt->alg_result;
	alg_calc_result.is_caf_trig = AF_FALSE;
	alg_calc_result.is_finish = AF_FALSE;
	alg_calc_param.cur_motor_pos = af_cxt->cur_af_pos;
	alg_calc_result.motor_pos = af_cxt->cur_af_pos;
	alg_calc_param.af_has_suc_rec = af_cxt->af_has_suc_rec;
	rtn = af_alg_calculation(af_cxt->af_alg_handle,
					&alg_calc_param,
					&alg_calc_result);

	if (af_cxt->cur_af_pos != alg_calc_result.motor_pos) {
		_af_set_motor_pos(handle,alg_calc_result.motor_pos);
	}
	if ((!af_cxt->alg_result.is_finish) && alg_calc_result.is_finish) {
		result->motor_pos = alg_calc_result.motor_pos;
		result->suc_win = alg_calc_result.suc_win;
		_af_finish(handle,result);
		_caf_reset_after_af(handle);
	}else if ((!af_cxt->alg_result.is_caf_trig) && alg_calc_result.is_caf_trig) {
		af_cxt->start_notice(af_cxt->caller);
		_caf_trig_af_start(handle);
	}else if((af_cxt->af_mode == AF_ALG_MODE_CONTINUE ||af_cxt->af_mode == AF_ALG_MODE_VIDEO)  && af_cxt->alg_result.is_caf_trig_in_saf) {
		af_cxt->start_notice(af_cxt->caller);
		_caf_trig_af_start(handle);
	}
	af_cxt->alg_result = alg_calc_result;
	if((af_cxt->af_mode == AF_ALG_MODE_CONTINUE ||af_cxt->af_mode == AF_ALG_MODE_VIDEO) && af_cxt->alg_result.is_caf_trig_in_saf) {
		af_cxt->alg_result.is_caf_trig_in_saf = AF_FALSE;
	}

	return rtn;



}



int32_t af_ioctrl(af_handle_t handle, enum af_cmd cmd,
				void *param0, void *param1)
{
	UNUSED(param1);
	int32_t rtn = AF_SUCCESS;
	struct af_context_t *af_cxt = (struct af_context_t *)handle;

	rtn = _check_handle(handle);

	pthread_mutex_lock(&af_cxt->status_lock);

	if (AF_SUCCESS != rtn) {
		AF_LOGE("_check_cxt failed");
		return AF_ERROR;
	}
	switch (cmd) {
	case AF_CMD_SET_AF_MODE:
		rtn = _af_set_mode(handle,param0);
		break;

	case AF_CMD_SET_AF_POS:
		rtn = _af_set_motor_pos(handle,*(uint32_t*)param0);
		break;

	case AF_CMD_SET_TUNING_MODE:
		rtn = af_alg_ioctrl(af_cxt->af_alg_handle,AF_ALG_CMD_SET_CAF_STOP,NULL,NULL);
		rtn = _af_set_status(handle,AF_ALG_STATUS_STOP);
		if (af_cxt->is_running) {
			af_cxt->af_result.suc_win = 0;
			rtn = _af_end_proc(handle,&af_cxt->af_result,AF_TRUE);
		}

		rtn = af_alg_ioctrl(af_cxt->af_alg_handle,AF_ALG_CMD_SET_TUNING_MODE,param0,NULL);
		break;
	case AF_CMD_SET_ISP_TOOL_AF_TEST:
		af_cxt->isp_tool_af_test = *(uint32_t*)param0;
		break;
	case AF_CMD_SET_SCENE_MODE:

		break;

	case AF_CMD_SET_AF_START:{
		uint32_t af_mode = af_cxt->af_mode;
		struct af_trig_info *trig_info = (struct af_trig_info *)param0;

		if (((AF_MODE_CONTINUE == af_mode) || (AF_MODE_VIDEO == af_mode))
			&& (0 == af_cxt->flash_on)
			&& (0 == trig_info->win_num)) {
			if (af_cxt->is_running) {
				break;
			} else {
				af_cxt->af_result.suc_win = 1;
				rtn = _af_end_proc(handle,&af_cxt->af_result,AF_TRUE);
				break;
			}
		}

		af_cxt->isp_tool_af_test = 0;
		rtn = af_alg_ioctrl(af_cxt->af_alg_handle,AF_ALG_CMD_SET_CAF_STOP,NULL,NULL);
		rtn = _af_set_status(handle,AF_ALG_STATUS_STOP);
		rtn = _af_set_start(handle,(struct af_trig_info *)param0);
		break;
	}

	case AF_CMD_SET_CAF_TRIG_START:
		af_cxt->isp_tool_af_test = 0;
		rtn = af_alg_ioctrl(af_cxt->af_alg_handle,AF_ALG_CMD_SET_CAF_STOP,NULL,NULL);
		rtn = _af_set_status(handle,AF_ALG_STATUS_STOP);
		rtn = _af_set_start(handle,(struct af_trig_info *)param0);
		break;

	case AF_CMD_SET_AF_STOP:
		rtn = af_alg_ioctrl(af_cxt->af_alg_handle,AF_ALG_CMD_SET_CAF_STOP,NULL,NULL);
		rtn = _af_set_status(handle,AF_ALG_STATUS_STOP);
		af_cxt->af_result.suc_win = 0;
		rtn = _af_end_proc(handle,&af_cxt->af_result,AF_TRUE);
		break;

	case AF_CMD_SET_AF_RESTART:

		break;

	case AF_CMD_SET_CAF_RESET:
		rtn = af_alg_ioctrl(af_cxt->af_alg_handle,AF_ALG_CMD_SET_CAF_RESET,NULL,NULL);
		break;

	case AF_CMD_SET_CAF_STOP:
		rtn = af_alg_ioctrl(af_cxt->af_alg_handle,AF_ALG_CMD_SET_CAF_STOP,NULL,NULL);
		break;
	case AF_CMD_SET_AF_FINISH:
		if (af_cxt->is_running) {
			rtn = _af_end_proc(handle,(struct af_result_param *)param0,AF_TRUE);
		}
		break;
	case AF_CMD_SET_AF_BYPASS:
		af_cxt->bypass = *(uint32_t *)param0;
		if (af_cxt->bypass) {
			rtn = af_alg_ioctrl(af_cxt->af_alg_handle,AF_ALG_CMD_SET_CAF_STOP,NULL,NULL);
			rtn = _af_set_status(handle,AF_ALG_STATUS_STOP);
		} else {
			if ((af_cxt->af_mode == AF_ALG_MODE_CONTINUE) || (af_cxt->af_mode == AF_ALG_MODE_VIDEO)){
				rtn = _af_set_mode(handle,(void*)&af_cxt->af_mode);
			}
		}
		break;

	case AF_CMD_SET_DEFAULT_AF_WIN:
		rtn = _af_cfg_afm_win(handle,1,0,NULL);
		break;

	case AF_CMD_SET_FLASH_NOTICE:{
		uint32_t flash_status;
		uint32_t af_mode = af_cxt->af_mode;

		flash_status = *(uint32_t*)param0;

		switch (flash_status) {
		case ISP_FLASH_PRE_BEFORE:
		case ISP_FLASH_PRE_LIGHTING:
		case ISP_FLASH_MAIN_BEFORE:
		case ISP_FLASH_MAIN_LIGHTING:
			if (((AF_MODE_CONTINUE == af_mode) || (AF_MODE_VIDEO== af_mode))){
				rtn = af_alg_ioctrl(af_cxt->af_alg_handle,AF_ALG_CMD_SET_CAF_STOP,NULL,NULL);
			}
			if (af_cxt->is_running) {
				rtn = _af_end_proc(handle,&af_cxt->af_result,AF_FALSE);
			}
			af_cxt->flash_on = 1;
			break;

		case ISP_FLASH_PRE_AFTER:
		case ISP_FLASH_MAIN_AFTER:
			if (((AF_MODE_CONTINUE == af_mode) || (AF_MODE_VIDEO == af_mode))){
				rtn = af_alg_ioctrl(af_cxt->af_alg_handle,AF_ALG_CMD_SET_CAF_RESET,NULL,NULL);
			}
			af_cxt->flash_on = 0;
			break;
		default:
			break;

		}


		break;
	}

	case AF_CMD_SET_ISP_START_INFO:
		break;

	case AF_CMD_SET_ISP_STOP_INFO:
		if (af_cxt->is_running) {
			rtn = _af_end_proc(handle,&af_cxt->af_result,AF_FALSE);
		}
		break;

	case AF_CMD_SET_AE_INFO:{
		struct ae_calc_out *ae_result = (struct ae_calc_out*)param0;
		int32_t *bv = (int32_t *)param1;


		af_cxt->ae_cur_lum = ae_result->cur_lum;
		af_cxt->ae_is_stab = ae_result->is_stab;
		//AF_LOGI("ae_is_stab %d cur lum %d ",ae_result->is_stab,af_cxt->ae_cur_lum);
		if (ae_result->cur_exp_line && ae_result->line_time)
			af_cxt->cur_fps = 100000000/(ae_result->cur_exp_line*ae_result->line_time);
		af_cxt->cur_ae_again = ae_result->cur_again;
		af_cxt->cur_ae_bv = *bv;

		if (*bv <= af_cxt->bv_thr[0]){
			af_cxt->cur_envi = AF_ENVI_LOWLUX;
		}else if (*bv < af_cxt->bv_thr[1] && *bv > af_cxt->bv_thr[0]) {
			af_cxt->cur_envi = AF_ENVI_INDOOR;
		}else {
			af_cxt->cur_envi = AF_ENVI_OUTDOOR;
		}

		break;
	}

	case AF_CMD_SET_AWB_INFO:{
		struct awb_ctrl_calc_result *awb_result = (struct awb_ctrl_calc_result*)param0;
		uint32_t r_diff;
		uint32_t g_diff;
		uint32_t b_diff;

		r_diff = awb_result->gain.r > af_cxt->cur_awb_r_gain ? awb_result->gain.r - af_cxt->cur_awb_r_gain : af_cxt->cur_awb_r_gain - awb_result->gain.r;
		g_diff = awb_result->gain.g > af_cxt->cur_awb_g_gain ? awb_result->gain.g - af_cxt->cur_awb_g_gain : af_cxt->cur_awb_g_gain - awb_result->gain.g;
		b_diff = awb_result->gain.b > af_cxt->cur_awb_b_gain ? awb_result->gain.b - af_cxt->cur_awb_b_gain : af_cxt->cur_awb_b_gain - awb_result->gain.b;

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
		AF_LOGI("awb_is_stab %d ",af_cxt->awb_is_stab);

		break;
	}



	case AF_CMD_GET_AF_MODE:
		*(uint32_t*)param0 = af_cxt->af_mode;
		break;

	case AF_CMD_GET_AF_CUR_POS:
		*(uint32_t*)param0 = af_cxt->cur_af_pos;
		break;
	default:
		AF_LOGE("cmd not support! cmd: %d",cmd);
		rtn = AF_ERROR;
		break;

	}

	pthread_mutex_unlock(&af_cxt->status_lock);

	return rtn;

}



/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/
#ifdef	__cplusplus
}
#endif
/*------------------------------------------------------------------------------*/
