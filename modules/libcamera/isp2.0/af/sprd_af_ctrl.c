#include "isp_com.h"
#include "isp_pm.h"
#include "lib_ctrl.h"
#include "awb_sprd_ctrl.h"
#include "af_ctrl.h"
#include "isp_app.h"
#include "isp_log.h"
#include "cmr_msg.h"
#include "isp_type.h"

#define ISP_CALLBACK_EVT 0x00040000
#define ISP_PROC_AF_IMG_DATA_UPDATE    (1<<3)
#define UNUSED(x) (void)x
#define BLOCK_PARAM_CFG(input, param_data, blk_cmd, blk_id, cfg_ptr, cfg_size)\
	do {\
		param_data.cmd = blk_cmd;\
		param_data.id = blk_id;\
		param_data.data_ptr = cfg_ptr;\
		param_data.data_size = cfg_size;\
		input.param_data_ptr = &param_data;\
		input.param_num = 1;} while (0);

typedef void* sprd_af_handle_t;



static int32_t af_set_pos(void* handle, struct af_motor_pos* in_param)
{
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)handle;

	if (ctrl_context->ioctrl_ptr->set_focus) {
		ctrl_context->ioctrl_ptr->set_focus(in_param->motor_pos);
	}

	return ISP_SUCCESS;
}

static int32_t af_end_notice(void* handle, struct af_result_param* in_param)
{
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)handle;
	struct isp_system* isp_system_ptr = &ctrl_context->system;

	ISP_LOGE("AF_TAG: move end");
	if (ISP_ZERO == isp_system_ptr->isp_callback_bypass) {
		struct isp_af_notice af_notice = {0x00};
		af_notice.mode = ISP_FOCUS_MOVE_END;
		af_notice.valid_win = in_param->suc_win;
		ISP_LOGD("callback ISP_AF_NOTICE_CALLBACK");
		isp_system_ptr->callback(isp_system_ptr->caller_id, ISP_CALLBACK_EVT|ISP_AF_NOTICE_CALLBACK, (void*)&af_notice, sizeof(struct isp_af_notice));
	}

	return ISP_SUCCESS;
}

static int32_t af_start_notice(void* handle)
{
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)handle;
	struct isp_system* isp_system_ptr = &ctrl_context->system;
	struct isp_af_notice af_notice = {0x00};

	ISP_LOGE("AF_TAG: move start");
	af_notice.mode = ISP_FOCUS_MOVE_START;
	af_notice.valid_win = 0x00;
	isp_system_ptr->callback(isp_system_ptr->caller_id, ISP_CALLBACK_EVT|ISP_AF_NOTICE_CALLBACK, (void*)&af_notice, sizeof(struct isp_af_notice));

	return ISP_SUCCESS;
}

static int32_t af_ae_awb_lock(void* handle)
{
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)handle;
	struct ae_calc_out ae_result = {0};

	ctrl_context->ae_lib_fun->ae_io_ctrl(ctrl_context->handle_ae, AE_SET_PAUSE, NULL, (void*)&ae_result);
	ctrl_context->awb_lib_fun->awb_ctrl_ioctrl(ctrl_context->handle_awb, AWB_CTRL_CMD_LOCK, NULL,NULL);

	return ISP_SUCCESS;
}

static int32_t af_ae_awb_release(void* handle)
{
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)handle;

	struct ae_calc_out ae_result = {0};

	ctrl_context->ae_lib_fun->ae_io_ctrl(ctrl_context->handle_ae, AE_SET_RESTORE, NULL, (void*)&ae_result);
	ctrl_context->awb_lib_fun->awb_ctrl_ioctrl(ctrl_context->handle_awb, AWB_CTRL_CMD_UNLOCK, NULL,NULL);

	return ISP_SUCCESS;
}


static int32_t af_set_monitor(void* handle, struct af_monitor_set* in_param, uint32_t cur_envi)
{
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)handle;

	if( ISP_CHIP_ID_TSHARK3==isp_dev_get_chip_id(ctrl_context->handle_device) ){
		struct afm_thrd_rgb thr_rgb;
		thr_rgb.sobel5_thr_min_red = 0x0;
		thr_rgb.sobel5_thr_max_red = 0xffff;
		thr_rgb.sobel5_thr_min_green = 0x0;
		thr_rgb.sobel5_thr_max_green = 0xffff;
		thr_rgb.sobel5_thr_min_blue = 0x0;
		thr_rgb.sobel5_thr_max_blue = 0xffff;
		thr_rgb.sobel9_thr_min_red = 0x0;
		thr_rgb.sobel9_thr_max_red = 0xffff;
		thr_rgb.sobel9_thr_min_green = 0x0;
		thr_rgb.sobel9_thr_max_green = 0xffff;
		thr_rgb.sobel9_thr_min_blue = 0x0;
		thr_rgb.sobel9_thr_max_blue = 0xffff;
		thr_rgb.spsmd_thr_min_red = 0x0;
		thr_rgb.spsmd_thr_max_red = 0xffff;
		thr_rgb.spsmd_thr_min_green = 0x0;
		thr_rgb.spsmd_thr_max_green = 0xffff;
		thr_rgb.spsmd_thr_min_blue = 0x0;
		thr_rgb.spsmd_thr_max_blue = 0xffff;

		isp_u_raw_afm_bypass(ctrl_context->handle_device,1);
		isp_u_raw_afm_skip_num_clr(ctrl_context->handle_device,1);
		isp_u_raw_afm_skip_num_clr(ctrl_context->handle_device,0);
		isp_u_raw_afm_skip_num(ctrl_context->handle_device,in_param->skip_num);
		isp_u_raw_afm_spsmd_rtgbot_enable(ctrl_context->handle_device,1);
		isp_u_raw_afm_spsmd_diagonal_enable(ctrl_context->handle_device,1);
		isp_u_raw_afm_spsmd_square_en(ctrl_context->handle_device,1);
		isp_u_raw_afm_overflow_protect(ctrl_context->handle_device,0);
		isp_u_raw_afm_subfilter(ctrl_context->handle_device,0,1);
		isp_u_raw_afm_spsmd_touch_mode(ctrl_context->handle_device,0);
		isp_u_raw_afm_shfit(ctrl_context->handle_device,0,0,0);

		isp_u_raw_afm_threshold_rgb(ctrl_context->handle_device, (void*)&thr_rgb);
		isp_u_raw_afm_mode(ctrl_context->handle_device,in_param->int_mode);
		isp_u_raw_afm_bypass(ctrl_context->handle_device,in_param->bypass);
	}else{
		isp_u_raw_afm_bypass(ctrl_context->handle_device,1);
		isp_u_raw_afm_skip_num_clr(ctrl_context->handle_device,1);
		isp_u_raw_afm_skip_num_clr(ctrl_context->handle_device,0);
		isp_u_raw_afm_skip_num(ctrl_context->handle_device,in_param->skip_num);
		isp_u_raw_afm_spsmd_rtgbot_enable(ctrl_context->handle_device,1);
		isp_u_raw_afm_spsmd_diagonal_enable(ctrl_context->handle_device,1);
		isp_u_raw_afm_spsmd_cal_mode(ctrl_context->handle_device,1);
		isp_u_raw_afm_sobel_type(ctrl_context->handle_device,1);
		isp_u_raw_afm_spsmd_type(ctrl_context->handle_device,2);
		if (cur_envi == AF_ENVI_LOWLUX) {
		isp_u_raw_afm_sel_filter1(ctrl_context->handle_device,0);
		isp_u_raw_afm_sel_filter2(ctrl_context->handle_device,1);
		isp_u_raw_afm_sobel_threshold(ctrl_context->handle_device,0,0xffff);
		isp_u_raw_afm_spsmd_threshold(ctrl_context->handle_device,0,0xffff);
		} else if (cur_envi == AF_ENVI_INDOOR) {
			isp_u_raw_afm_sel_filter1(ctrl_context->handle_device,0);
			isp_u_raw_afm_sel_filter2(ctrl_context->handle_device,1);
			isp_u_raw_afm_sobel_threshold(ctrl_context->handle_device,0,40000);
			isp_u_raw_afm_spsmd_threshold(ctrl_context->handle_device,0,0xffff);
		} else {
			isp_u_raw_afm_sel_filter1(ctrl_context->handle_device,1);
			isp_u_raw_afm_sel_filter2(ctrl_context->handle_device,0);
			isp_u_raw_afm_sobel_threshold(ctrl_context->handle_device,0,0xffff);
			isp_u_raw_afm_spsmd_threshold(ctrl_context->handle_device,0,0xffff);
		}
		isp_u_raw_afm_mode(ctrl_context->handle_device,in_param->int_mode);
		isp_u_raw_afm_bypass(ctrl_context->handle_device,in_param->bypass);
	}

	return ISP_SUCCESS;
}

static int32_t af_set_monitor_win(void* handler, struct af_monitor_win* in_param)
{
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)handler;

	isp_u_raw_afm_win(ctrl_context->handle_device, (void *)(in_param->win_pos));

	return ISP_SUCCESS;
}

static int32_t af_get_monitor_win_num(void* handler, uint32_t *win_num)
{
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)handler;

	isp_u_raw_afm_win_num(ctrl_context->handle_device,win_num);

	return ISP_SUCCESS;
}

sprd_af_handle_t sprd_af_init(isp_ctrl_context* handle)
{
	int32_t rtn = ISP_SUCCESS;
	struct af_init_in_param init_input;
	struct af_init_result result;
	struct isp_pm_ioctl_input af_pm_input;
	struct isp_pm_ioctl_output af_pm_output;
	struct af_tuning_param *af_tuning;
	uint32_t i;

	memset((void*)&init_input, 0, sizeof(init_input));
	memset((void*)&result, 0, sizeof(result));
	memset((void*)&af_pm_input, 0, sizeof(af_pm_input));
	memset((void*)&af_pm_output, 0, sizeof(af_pm_output));

	rtn = isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_INIT_AF, &af_pm_input, &af_pm_output);
	if (ISP_SUCCESS == rtn) {
		init_input.af_bypass = 0;
		init_input.caller = handle;
		init_input.af_mode = 0;
		init_input.tuning_param_cnt = af_pm_output.param_num;
		init_input.cur_tuning_mode = 0;
		af_tuning = (struct af_tuning_param *)malloc(sizeof(struct af_tuning_param )*af_pm_output.param_num);
		if (NULL == af_tuning) {
			ISP_LOGE("AF_TAG: malloc failed!");
			return NULL;
		}
		for (i=0; i<af_pm_output.param_num; i++) {
			af_tuning[i].cfg_mode = (af_pm_output.param_data->id & 0xffff0000) >> 16;
			af_tuning[i].data = af_pm_output.param_data->data_ptr;
			af_tuning[i].data_len = af_pm_output.param_data->data_size;
			af_pm_output.param_data++;
		}

		init_input.tuning_param = af_tuning;
		init_input.plat_info.afm_filter_type_cnt = 1;
		init_input.plat_info.afm_win_max_cnt = 9;
		init_input.plat_info.isp_w = handle->input_size_trim[handle->param_index].width;
		init_input.plat_info.isp_h = handle->input_size_trim[handle->param_index].height;
		init_input.go_position = af_set_pos;
		init_input.end_notice = af_end_notice;
		init_input.start_notice = af_start_notice;
		init_input.set_monitor = af_set_monitor;
		init_input.set_monitor_win = af_set_monitor_win;
		init_input.get_monitor_win_num  = af_get_monitor_win_num;
		init_input.ae_awb_lock = af_ae_awb_lock;
		init_input.ae_awb_release = af_ae_awb_release;
		handle->handle_af = af_init(&init_input, &result);
		ISP_TRACE_IF_FAIL(!handle->handle_af, ("af_init error"));
		ISP_LOGE("AF_TAG init: handle=%p", handle->handle_af);
		free(af_tuning);
	} else {
		ISP_LOGE("AF_TAG: get af init param failed!");
	}

	return (sprd_af_handle_t)handle->handle_af;
}

int32_t sprd_af_calc(isp_ctrl_context* handle)
{
	int32_t rtn, i=0;
	struct af_result_param calc_result;
	struct isp_af_statistic_info afm_stat;
	memset((void*)&afm_stat, 0, sizeof(afm_stat));
	memset((void*)&calc_result, 0, sizeof(calc_result));
	if( ISP_CHIP_ID_TSHARK3==isp_dev_get_chip_id(handle->handle_device) ){
		uint64_t temp;
		rtn = isp_u_raw_afm_statistic_r6p9(handle->handle_device, (void*)afm_stat.info_tshark3);
		if (rtn) {
			ISP_LOGE("AF_TAG: isp_u_raw_afm_statistic_r6p9 failed!");
		}
		for( i=0; i<5 ; i++ ){
			temp = *(afm_stat.info_tshark3+95+i);// green high bit
			*(afm_stat.info+2*i) = ((temp&0x000000ff)<<32)+*(afm_stat.info_tshark3+61+3*2*i);// spsmd green channel
			*(afm_stat.info+2*i+1) = ((temp&0x00ff0000)<<16)+*(afm_stat.info_tshark3+61+3*(2*i+1));// spsmd green channel
		}
	}else{
		rtn = isp_u_raw_afm_type1_statistic(handle->handle_device,(void*)afm_stat.info_tshark3);
		if (rtn) {
			ISP_LOGE("AF_TAG: isp_u_raw_afm_type1_statistic failed!");
		}
		for( i=0; i<32 ; i++ ){
			*(afm_stat.info+i) = *(afm_stat.info_tshark3+i);
		}
	}

	rtn = _af_calc(handle,AF_DATA_AF,(void*)afm_stat.info,&calc_result);

	return rtn;
}

void sprd_af_deinit(isp_ctrl_context* handle)
{
	af_deinit((af_handle_t)handle->handle_af, NULL, NULL);
}

int32_t sprd_af_image_data_update(isp_ctrl_context* handle)
{
	int32_t rtn = ISP_SUCCESS;
/*	struct af_result_param calc_result;
	struct isp_awb_statistic_info *ae_stat_ptr = NULL;
	struct isp_pm_param_data param_data;
	struct isp_pm_ioctl_input input = {NULL, 0};
	struct isp_pm_ioctl_output output = {NULL, 0};

	memset((void*)&calc_result, 0, sizeof(calc_result));

	BLOCK_PARAM_CFG(input, param_data, ISP_PM_BLK_AEM_STATISTIC, ISP_BLK_AE_V1, NULL, 0);
	isp_pm_ioctl(handle->handle_pm, ISP_PM_CMD_GET_SINGLE_SETTING, (void*)&input, (void*)&output);
	if (NULL != output.param_data) {
		ae_stat_ptr = output.param_data->data_ptr;
		rtn = _af_calc(handle,AF_DATA_IMG_BLK,(void*)ae_stat_ptr,&calc_result);
	}*/

	return rtn;
}

int32_t sprd_af_ioctrl_set_af_mode(isp_handle isp_handler, void* param_ptr, int(*call_back)())
{
	isp_ctrl_context* handle = (isp_ctrl_context*)isp_handler;
	uint32_t set_mode;
	int32_t rtn;

	switch (*(uint32_t *)param_ptr) {
	case ISP_FOCUS_MACRO:
		set_mode = AF_MODE_MACRO;
		break;
	case ISP_FOCUS_CONTINUE:
		set_mode = AF_MODE_CONTINUE;
		break;
	case ISP_FOCUS_VIDEO:
		set_mode = AF_MODE_VIDEO;
		break;
	case ISP_FOCUS_MANUAL:
		set_mode = AF_MODE_MANUAL;
		break;
	default:
		set_mode = AF_MODE_NORMAL;
		break;
	}

	rtn = af_ioctrl(handle->handle_af, AF_CMD_SET_AF_MODE,(void*)&set_mode,NULL);

	return rtn;
}

int32_t sprd_af_ioctrl_get_af_mode(isp_handle isp_handler, void* param_ptr, int(*call_back)())
{
	isp_ctrl_context *handle = (isp_ctrl_context*)isp_handler;
	uint32_t param = 0;
	int32_t rtn;

	rtn = af_ioctrl(handle->handle_af, AF_CMD_GET_AF_MODE, (void*)&param, NULL);

	switch (param) {
	case AF_MODE_NORMAL:
		*(uint32_t*)param_ptr = ISP_FOCUS_TRIG;
		break;
	case AF_MODE_MACRO:
		*(uint32_t*)param_ptr = ISP_FOCUS_MACRO;
		break;
	case AF_MODE_CONTINUE:
		*(uint32_t*)param_ptr = ISP_FOCUS_CONTINUE;
		break;
	case AF_MODE_MANUAL:
		*(uint32_t*)param_ptr  = ISP_FOCUS_MANUAL;
		break;
	case AF_MODE_VIDEO:
		*(uint32_t*)param_ptr = ISP_FOCUS_VIDEO;
		break;
	default:
		*(uint32_t*)param_ptr = ISP_FOCUS_TRIG;
		break;
	}

	ISP_LOGI("rtn %d param %d af %d", rtn, param, *(uint32_t*)param_ptr);

	return rtn;
}

int32_t sprd_af_ioctrl_af_start(isp_handle isp_handler, void* param_ptr, int(*call_back)())
{
	isp_ctrl_context *handle = (isp_ctrl_context*)isp_handler;
	struct isp_af_win* af_ptr = (struct isp_af_win*)param_ptr;
	struct af_trig_info trig_info;
	int32_t rtn;
	uint32_t i;

	trig_info.win_num = af_ptr->valid_win;
	switch (af_ptr->mode) {
	case ISP_FOCUS_TRIG:
		trig_info.mode = AF_MODE_NORMAL;
		break;
	case ISP_FOCUS_MACRO:
		trig_info.mode = AF_MODE_MACRO;
		break;
	case ISP_FOCUS_CONTINUE:
		trig_info.mode = AF_MODE_CONTINUE;
		break;
	case ISP_FOCUS_MANUAL:
		trig_info.mode = AF_MODE_MANUAL;
		break;
	case ISP_FOCUS_VIDEO:
		trig_info.mode = AF_MODE_VIDEO;
		break;
	default:
		trig_info.mode = AF_MODE_NORMAL;
		break;
	}

	for (i=0; i<trig_info.win_num; i++) {
		trig_info.win_pos[i].sx = af_ptr->win[i].start_x;
		trig_info.win_pos[i].sy = af_ptr->win[i].start_y;
		trig_info.win_pos[i].ex = af_ptr->win[i].end_x;
		trig_info.win_pos[i].ey = af_ptr->win[i].end_y;
	}
	rtn = af_ioctrl(handle->handle_af,AF_CMD_SET_AF_START,(void*)&trig_info, NULL);

	return rtn;
}

int32_t sprd_af_ioctrl_af_info(isp_handle isp_handler, void* param_ptr, int(*call_back)())
{
	int32_t rtn = ISP_SUCCESS;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)isp_handler;
	UNUSED(call_back);
	struct isp_af_ctrl* af_ctrl_ptr = (struct isp_af_ctrl*)param_ptr;
	struct af_monitor_set monitor_set;
	uint32_t isp_tool_af_test;

	ISP_LOGI("--IOCtrl--AF_CTRL--mode:0x%x", af_ctrl_ptr->mode);

	if (ISP_CTRL_SET==af_ctrl_ptr->mode) {
		isp_tool_af_test = 1;
		rtn = af_ioctrl(ctrl_context->handle_af,AF_CMD_SET_ISP_TOOL_AF_TEST,&isp_tool_af_test,NULL);

		monitor_set.bypass = 0;
		monitor_set.int_mode = 1;
		monitor_set.need_denoise = 0;
		monitor_set.skip_num = 0;
		monitor_set.type = 1;
		rtn = af_set_monitor(ctrl_context,&monitor_set,AF_ENVI_INDOOR);

		rtn = af_ioctrl(ctrl_context->handle_af,AF_CMD_SET_DEFAULT_AF_WIN,NULL,NULL);
		rtn = af_ioctrl(ctrl_context->handle_af,AF_CMD_SET_AF_POS,(void*)&af_ctrl_ptr->step,NULL);
	} else if (ISP_CTRL_GET==af_ctrl_ptr->mode){
		uint32_t cur_pos;
		struct isp_af_statistic_info afm_stat;
		uint32_t i;
		memset((void*)&afm_stat, 0, sizeof(afm_stat));

		rtn = af_ioctrl(ctrl_context->handle_af,AF_CMD_GET_AF_CUR_POS,(void*)&cur_pos,NULL);
		if( ISP_CHIP_ID_TSHARK3==isp_dev_get_chip_id(ctrl_context->handle_device) ){
			uint64_t temp;
			rtn = isp_u_raw_afm_statistic_r6p9(ctrl_context->handle_device, (void*)afm_stat.info_tshark3);
			if (rtn) {
				ISP_LOGE("AF_TAG: isp_u_raw_afm_statistic_r6p9 failed!");
			}
			for( i=0; i<5 ; i++ ){
				temp = *(afm_stat.info_tshark3+95+i);// green high bit
				*(afm_stat.info+2*i) = ((temp&0x000000ff)<<32)+*(afm_stat.info_tshark3+61+3*2*i);// spsmd green channel
				*(afm_stat.info+2*i+1) = ((temp&0x00ff0000)<<16)+*(afm_stat.info_tshark3+61+3*(2*i+1));// spsmd green channel
			}
		}else{
			rtn = isp_u_raw_afm_type1_statistic(ctrl_context->handle_device,(void*)afm_stat.info_tshark3);
			if (rtn) {
				ISP_LOGE("AF_TAG: isp_u_raw_afm_type1_statistic failed!");
			}
			for( i=0; i<32 ; i++ ){
				*(afm_stat.info+i) = *(afm_stat.info_tshark3+i);
			}
		}

		af_ctrl_ptr->step = cur_pos;
		af_ctrl_ptr->num = 9;
		for (i=0;i<af_ctrl_ptr->num;i++) {
			af_ctrl_ptr->stat_value[i] = afm_stat.info[i];
		}
	} else {
		isp_tool_af_test = 0;
		rtn = af_ioctrl(ctrl_context->handle_af,AF_CMD_SET_ISP_TOOL_AF_TEST,&isp_tool_af_test,NULL);
		isp_u_raw_afm_bypass(ctrl_context->handle_device,1);
	}

	return rtn;
}

int32_t sprd_af_ioctrl_thread_msg_send(isp_ctrl_context* handle,
		struct ae_calc_out* ae_result,
		struct cmr_msg* msg)
{
	int32_t rtn;
	struct af_result_param calc_result;
	struct isp_system* isp_system_ptr = &handle->system;

	memset((void*)&calc_result, 0, sizeof(calc_result));
	rtn = _af_calc(handle,AF_DATA_AE,(void*)ae_result,&calc_result);
/*
	msg->msg_type = ISP_PROC_AF_IMG_DATA_UPDATE;
	rtn = cmr_thread_msg_send(isp_system_ptr->thread_af_proc, msg);
*/
	return rtn;
}

int32_t sprd_af_ioctrl_set_flash_notice(isp_ctrl_context* handle, void* param_ptr, int(*call_back)())
{
	struct isp_flash_notice *flash_notice = (struct isp_flash_notice*)param_ptr;
	int32_t rtn;
	rtn = af_ioctrl(handle->handle_af,AF_CMD_SET_FLASH_NOTICE,(void *)&(flash_notice->mode),NULL);

	return rtn;
}

int32_t sprd_af_ioctrl_set_isp_start_info(isp_handle isp_handler, struct isp_video_start* param_ptr)
{
	int32_t rtn = 0;
	isp_ctrl_context* handle = (isp_ctrl_context*)isp_handler;

	if (handle->handle_af && ((ISP_VIDEO_MODE_CONTINUE == param_ptr->mode))) {
		rtn = af_ioctrl(handle->handle_af,AF_CMD_SET_ISP_START_INFO,NULL,NULL);
	}

	return rtn;
}

int32_t sprd_af_ioctrl_set_isp_stop_info(isp_handle isp_handler)
{
	int32_t rtn = ISP_SUCCESS;
	isp_ctrl_context* handle = (isp_ctrl_context*)isp_handler;
	if (handle->handle_af) {
		rtn = af_ioctrl(handle->handle_af,AF_CMD_SET_ISP_STOP_INFO,NULL,NULL);
	}

	return rtn;
}

int32_t sprd_af_ioctrl_get_af_cur_pos(isp_ctrl_context* handle, void* param_ptr, int(*call_back)())
{
	int32_t rtn;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)handle;
	rtn = af_ioctrl(ctrl_context->handle_af,AF_CMD_GET_AF_CUR_POS,param_ptr,NULL);

	return rtn;
}

int32_t sprd_af_ioctrl_set_af_pos(isp_ctrl_context* handle, void* param_ptr, int(*call_back)())
{
	int32_t rtn;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)handle;
	rtn = af_ioctrl(ctrl_context->handle_af,AF_CMD_SET_AF_POS,param_ptr,NULL);

	return rtn;
}

int32_t sprd_af_ioctrl_set_af_bypass(isp_ctrl_context* handle, void* param_ptr, int(*call_back)())
{
	int32_t rtn;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)handle;
	rtn = af_ioctrl(ctrl_context->handle_af,AF_CMD_SET_AF_BYPASS,param_ptr,NULL);

	return rtn;
}

int32_t sprd_af_ioctrl_set_af_stop(isp_ctrl_context* handle, void* param_ptr, int(*call_back)())
{
	int32_t rtn;
	isp_ctrl_context *ctrl_context = (isp_ctrl_context *)handle;
	rtn = af_ioctrl(ctrl_context->handle_af, AF_CMD_SET_AF_STOP, NULL, NULL);

	return rtn;
}

int32_t sprd_af_ioctrl_set_ae_awb_info(isp_ctrl_context* handle,
		void* ae_result,
		void* awb_result,
		void* bv,
		void *rgb_statistics)
{
	int rtn = 0;
    	struct af_result_param calc_result;
	struct isp_awb_statistic_info *ae_stat_ptr = NULL;
	rtn = handle->ae_lib_fun->ae_io_ctrl(handle->handle_ae, AE_GET_BV_BY_LUM, NULL, (void *)bv);
	rtn = af_ioctrl(handle->handle_af,AF_CMD_SET_AE_INFO,(void*)ae_result,(void*)bv);
	rtn = af_ioctrl(handle->handle_af,AF_CMD_SET_AWB_INFO,(void*)awb_result,NULL);
    
    	if (NULL != rgb_statistics) {
		ae_stat_ptr = rgb_statistics;
            AF_LOGD("rgb info %d %d %d",ae_stat_ptr->r_info[0],ae_stat_ptr->g_info[511],ae_stat_ptr->b_info[1023]);
		rtn = _af_calc(handle,AF_DATA_IMG_BLK,(void*)ae_stat_ptr,&calc_result);
	}

	return rtn;
}

extern struct af_lib_fun af_lib_fun;
void sprd_af_fun_init()
{
	af_lib_fun.af_init_interface		= sprd_af_init;
	af_lib_fun.af_calc_interface		= sprd_af_calc;
	af_lib_fun.af_deinit_interface		= sprd_af_deinit;
	af_lib_fun.af_ioctrl_interface		= af_ioctrl;
	af_lib_fun.af_ioctrl_set_flash_notice	= sprd_af_ioctrl_set_flash_notice;
	af_lib_fun.af_ioctrl_set_af_mode	= sprd_af_ioctrl_set_af_mode;
	af_lib_fun.af_ioctrl_get_af_mode	= sprd_af_ioctrl_get_af_mode;
	af_lib_fun.af_ioctrl_af_start		= sprd_af_ioctrl_af_start;
	af_lib_fun.af_ioctrl_set_isp_start_info= sprd_af_ioctrl_set_isp_start_info;
	af_lib_fun.af_ioctrl_af_info		= sprd_af_ioctrl_af_info;
	af_lib_fun.af_ioctrl_set_isp_stop_info	= sprd_af_ioctrl_set_isp_stop_info;
	af_lib_fun.af_ioctrl_thread_msg_send	= sprd_af_ioctrl_thread_msg_send;
	af_lib_fun.af_image_data_update	= sprd_af_image_data_update;
	af_lib_fun.af_ioctrl_get_af_cur_pos	= sprd_af_ioctrl_get_af_cur_pos;
	af_lib_fun.af_ioctrl_set_af_pos	= sprd_af_ioctrl_set_af_pos;
	af_lib_fun.af_ioctrl_set_af_bypass	= sprd_af_ioctrl_set_af_bypass;
	af_lib_fun.af_ioctrl_set_af_stop	= sprd_af_ioctrl_set_af_stop;
	af_lib_fun.af_ioctrl_set_ae_awb_info	= sprd_af_ioctrl_set_ae_awb_info;

	return;
}
