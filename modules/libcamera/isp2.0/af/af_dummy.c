#include "isp_com.h"
#include "isp_pm.h"
#include "lib_ctrl.h"
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

typedef void* dummy_af_handle_t;



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


static int32_t af_set_monitor(void* handle, struct af_monitor_set* in_param)
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
		isp_u_raw_afm_sel_filter1(ctrl_context->handle_device,0);
		isp_u_raw_afm_sel_filter2(ctrl_context->handle_device,1);
		isp_u_raw_afm_sobel_type(ctrl_context->handle_device,0);
		isp_u_raw_afm_spsmd_type(ctrl_context->handle_device,2);
		isp_u_raw_afm_sobel_threshold(ctrl_context->handle_device,0,0xffff);
		isp_u_raw_afm_spsmd_threshold(ctrl_context->handle_device,0,0xffff);
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

dummy_af_handle_t dummy_af_init(isp_ctrl_context* handle)
{
	int32_t rtn = ISP_SUCCESS;
	return (sprd_af_handle_t)handle->handle_af;
}

int32_t dummy_af_calc(isp_ctrl_context* handle)
{
	int32_t rtn = ISP_SUCCESS;
	return rtn;
}

void dummy_af_deinit(isp_ctrl_context* handle)
{
	af_deinit((af_handle_t)handle->handle_af, NULL, NULL);
}

int32_t dummy_af_image_data_update(isp_ctrl_context* handle)
{
	int32_t rtn = ISP_SUCCESS;
	return rtn;
}

int32_t dummy_af_ioctrl(sft_af_handle_t handle, enum af_cmd cmd, void *param0, void *param1)
{
	int32_t rtn = ISP_SUCCESS;
	return rtn;
}

int32_t dummy_af_ioctrl_set_af_mode(isp_handle isp_handler, void* param_ptr, int(*call_back)())
{
	int32_t rtn = ISP_SUCCESS;
	return rtn;
}

int32_t dummy_af_ioctrl_get_af_mode(isp_handle isp_handler, void* param_ptr, int(*call_back)())
{
	int32_t rtn = ISP_SUCCESS;
	return rtn;
}

int32_t dummy_af_ioctrl_af_start(isp_handle isp_handler, void* param_ptr, int(*call_back)())
{
	int32_t rtn = ISP_SUCCESS;
	return rtn;
}

int32_t dummy_af_ioctrl_af_info(isp_handle isp_handler, void* param_ptr, int(*call_back)())
{
	int32_t rtn = ISP_SUCCESS;
	return rtn;
}

int32_t dummy_af_ioctrl_thread_msg_send(isp_ctrl_context* handle,
		struct ae_calc_out* ae_result,
		struct cmr_msg* msg)
{
	int32_t rtn = ISP_SUCCESS;
	return rtn;
}

int32_t dummy_af_ioctrl_set_flash_notice(isp_ctrl_context* handle, void* param_ptr, int(*call_back)())
{
	int32_t rtn = ISP_SUCCESS;
	return rtn;
}

int32_t dummy_af_ioctrl_set_af_info(isp_handle isp_handler, void* param_ptr, int(*call_back)())
{
	int32_t rtn = ISP_SUCCESS;
	return rtn;
}

int32_t dummy_af_ioctrl_get_af_info(isp_handle isp_handler, void* param_ptr, int(*call_back)())
{
	int32_t rtn = ISP_SUCCESS;
	return rtn;
}

int32_t dummy_af_ioctrl_get_af_value(isp_handle isp_handler, void* param_ptr, int(*call_back)())
{
	int32_t rtn = ISP_SUCCESS;
	return rtn;
}

int32_t dummy_af_ioctrl_burst_notice(isp_handle isp_handler, void* param_ptr, int(*call_back)())
{
	int32_t rtn = ISP_SUCCESS;
	return rtn;
}

int32_t dummy_af_ioctrl_ioread(isp_handle isp_handler, void* param_ptr, int(*call_back)())
{
	int32_t rtn = ISP_SUCCESS;
	return rtn;
}

int32_t dummy_af_ioctrl_iowrite(isp_handle isp_handler, void* param_ptr, int(*call_back)())
{
	int32_t rtn = ISP_SUCCESS;
	return rtn;
}

int32_t dummy_af_ioctrl_set_fd_update(isp_handle isp_handler, void* param_ptr, int(*call_back)())
{
	int32_t rtn = ISP_SUCCESS;
	return rtn;
}

int32_t dummy_af_ioctrl_set_isp_start_info(isp_handle isp_handler, struct isp_video_start* param_ptr)
{
	int32_t rtn = ISP_SUCCESS;
	return rtn;
}

int32_t dummy_af_ioctrl_set_isp_stop_info(isp_handle isp_handler)
{
	int rtn = ISP_SUCCESS;
	return rtn;
}

int32_t dummy_af_ioctrl_set_ae_awb_info(isp_ctrl_context* handle, void *ae_result, void *awb_result, void *bv, void *rgb_statistics)
{
	int rtn = ISP_SUCCESS;
	return rtn;
}

int32_t dummy_af_ioctrl_get_af_cur_pos(isp_ctrl_context* handle, void* param_ptr, int(*call_back)())
{
	int32_t rtn = ISP_SUCCESS;
	return rtn;
}

int32_t dummy_af_ioctrl_set_af_pos(isp_ctrl_context* handle, void* param_ptr, int(*call_back)())
{
	int32_t rtn = ISP_SUCCESS;
	return rtn;
}

int32_t dummy_af_ioctrl_set_af_bypass(isp_ctrl_context* handle, void* param_ptr, int(*call_back)())
{
	int32_t rtn = ISP_SUCCESS;
	return rtn;
}

int32_t dummy_af_ioctrl_set_af_stop(isp_ctrl_context* handle, void* param_ptr, int(*call_back)())
{
	int32_t rtn = ISP_SUCCESS;
	return rtn;
}

extern struct af_lib_fun af_lib_fun;
void dummy_af_fun_init()
{
	af_lib_fun.af_init_interface		= dummy_af_init;
	af_lib_fun.af_calc_interface		= dummy_af_calc;
	af_lib_fun.af_deinit_interface		= dummy_af_deinit;
	af_lib_fun.af_ioctrl_interface		= dummy_af_ioctrl;
	af_lib_fun.af_ioctrl_set_flash_notice	= dummy_af_ioctrl_set_flash_notice;
	af_lib_fun.af_ioctrl_set_af_info	= dummy_af_ioctrl_set_af_info;
	af_lib_fun.af_ioctrl_get_af_info	= dummy_af_ioctrl_get_af_info;
	af_lib_fun.af_ioctrl_get_af_value	= dummy_af_ioctrl_get_af_value;
	af_lib_fun.af_ioctrl_burst_notice	= dummy_af_ioctrl_burst_notice;
	af_lib_fun.af_ioctrl_set_af_mode	= dummy_af_ioctrl_set_af_mode;
	af_lib_fun.af_ioctrl_get_af_mode	= dummy_af_ioctrl_get_af_mode;
	af_lib_fun.af_ioctrl_ioread		= dummy_af_ioctrl_ioread;
	af_lib_fun.af_ioctrl_iowrite		= dummy_af_ioctrl_iowrite;
	af_lib_fun.af_ioctrl_set_fd_update	= dummy_af_ioctrl_set_fd_update;
	af_lib_fun.af_ioctrl_af_start		= dummy_af_ioctrl_af_start;
	af_lib_fun.af_ioctrl_set_isp_start_info	= dummy_af_ioctrl_set_isp_start_info;
	af_lib_fun.af_ioctrl_af_info		= dummy_af_ioctrl_af_info;
	af_lib_fun.af_ioctrl_set_isp_stop_info	= dummy_af_ioctrl_set_isp_stop_info;
	af_lib_fun.af_ioctrl_set_ae_awb_info	= dummy_af_ioctrl_set_ae_awb_info;
	af_lib_fun.af_ioctrl_thread_msg_send	= dummy_af_ioctrl_thread_msg_send;
	af_lib_fun.af_image_data_update	= dummy_af_image_data_update;
	af_lib_fun.af_ioctrl_get_af_cur_pos	= dummy_af_ioctrl_get_af_cur_pos;
	af_lib_fun.af_ioctrl_set_af_pos	= dummy_af_ioctrl_set_af_pos;
	af_lib_fun.af_ioctrl_set_af_bypass	= dummy_af_ioctrl_set_af_bypass;
	af_lib_fun.af_ioctrl_set_af_stop	= dummy_af_ioctrl_set_af_stop;

	return;
}
