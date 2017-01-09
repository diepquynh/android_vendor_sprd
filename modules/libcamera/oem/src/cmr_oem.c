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
#define LOG_TAG "cmr_oem"

#include <stdlib.h>
#include <sys/stat.h>
#include <math.h>
#include <cutils/properties.h>
#include "cmr_oem.h"
#include "cmr_common.h"
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
#include "isp_otp_calibration.h"
#endif
#ifdef CONFIG_FACE_BEAUTY
#include "ts_makeup_api.h"
#endif
/**********************************************************************************************/
#define PREVIEW_MSG_QUEUE_SIZE                       50
#define SNAPSHOT_MSG_QUEUE_SIZE                      50
#define CMR_EVT_INIT                                 (CMR_EVT_OEM_BASE)
#define CMR_EVT_WAIT                                 (CMR_EVT_OEM_BASE + 1)
#define CMR_EVT_EXIT                                 (CMR_EVT_OEM_BASE + 2)
#define CMR_EVT_PRE_PREVIEW                          (CMR_EVT_OEM_BASE + 3)
#define CMR_EVT_PRE_PREVIEW_EXIT                     (CMR_EVT_OEM_BASE + 4)

#define CAMERA_OEM_MSG_QUEUE_SIZE                    10
#define CAMERA_RECOVER_CNT                           3

#define DCAM_PATH_VERTICAL_MIRROR                    3

#define OEM_HANDLE_HDR                               1
#define CAMERA_PATH_SHARE                            1
#define OEM_RESTART_SUM                              2
#define POWER2(x)                                    (1<<(x))
#define ONE_HUNDRED                                  100
#define MS_TO_NANOSEC                                1000
#define SEC_TO_NANOSEC                               1000000000LL
enum oem_ev_level {
	OEM_EV_LEVEL_1,
	OEM_EV_LEVEL_2,
	OEM_EV_LEVEL_3
};


#define CHECK_HANDLE_VALID(handle) \
                                                     do { \
                                                            if (!handle) { \
                                                                CMR_LOGE("err handle");  \
                                                                return CMR_CAMERA_INVALID_PARAM; \
                                                            } \
                                                     } while(0)
/**********************************************************************************************/

static uint32_t                                      is_support_reload = 0;

// err log is always show
// gCMRLogLevel(default is 4):
//   1 - only show ALOGE
//   2 - show ALOGE and ALOGW
//   3 - show ALOGE, ALOGW and ALOGI
//   4 - show ALOGE, ALOGW, ALOGI and ALOGD
//   5 - show ALOGE, ALOGW, ALOGI and ALOGD, ALOGV
// use the following command to change gCMRLogLevel:
//   adb shell setprop persist.sys.camera.hal.log 1
volatile uint32_t gCMRLogLevel = 4;

/************************************internal interface ***************************************/
static void camera_send_channel_data(cmr_handle oem_handle, cmr_handle receiver_handle, cmr_uint evt, void *data);
static cmr_int camera_sensor_streamctrl(cmr_u32 on_off, void *privdata);
static cmr_int camera_isp_ctrl_done(cmr_u32 cmd, void* data);
static void camera_sensor_evt_cb(cmr_int evt, void* data, void* privdata);
static cmr_int camera_is_need_change_fmt(cmr_handle oem_handle, struct frm_info *data_ptr);
static void camera_grab_evt_cb(cmr_int evt, void* data, void* privdata);
static void camera_scaler_evt_cb(cmr_int evt, void* data, void *privdata);
static void camera_jpeg_evt_cb(cmr_int evt, void* data, void *privdata);
static cmr_int camera_isp_evt_cb(cmr_handle oem_handle, cmr_u32 evt, void* data, cmr_u32 data_len);
static cmr_int camera_preview_cb(cmr_handle oem_handle, enum preview_cb_type cb_type, enum preview_func_type func, void *param);
static cmr_int camera_ipm_cb(cmr_u32 class_type, struct ipm_frame_out *cb_param);
static void camera_snapshot_cb_to_hal(cmr_handle oem_handle, enum snapshot_cb_type cb, enum snapshot_func_type func, void* param);
static void camera_snapshot_state_handle(cmr_handle oem_handle, enum snapshot_cb_type cb, enum snapshot_func_type func, void* param);
static void camera_snapshot_cb(cmr_handle oem_handle, enum snapshot_cb_type cb, enum snapshot_func_type func, void* param);
static cmr_int camera_before_set(cmr_handle oem_handle, enum preview_param_mode mode);
static cmr_int camera_after_set(cmr_handle oem_handle, struct after_set_cb_param *param);
static cmr_int camera_focus_pre_proc(cmr_handle oem_handle);
static cmr_int camera_focus_post_proc(cmr_handle oem_handle, cmr_int will_capture);
static cmr_int camera_get_preview_status(cmr_handle oem_handle);
static cmr_int camera_sensor_init(cmr_handle  oem_handle, cmr_uint is_autotest);
static cmr_int camera_sensor_deinit(cmr_handle  oem_handle);
static cmr_int camera_grab_init(cmr_handle  oem_handle);
static cmr_int camera_grab_deinit(cmr_handle  oem_handle);
static cmr_int camera_jpeg_init(cmr_handle  oem_handle);
static cmr_int camera_jpeg_deinit(cmr_handle  oem_handle);
static cmr_int camera_scaler_init(cmr_handle  oem_handle);
static cmr_int camera_scaler_deinit(cmr_handle  oem_handle);
static cmr_int camera_rotation_init(cmr_handle  oem_handle);
static cmr_int camera_rotation_deinit(cmr_handle  oem_handle);
static cmr_int camera_isp_init(cmr_handle  oem_handle);
static cmr_int camera_isp_deinit_notice(cmr_handle  oem_handle);
static cmr_int camera_isp_deinit(cmr_handle  oem_handle);
static cmr_int camera_preview_init(cmr_handle  oem_handle);
static cmr_int camera_preview_deinit(cmr_handle  oem_handle);
static cmr_int camera_snapshot_init(cmr_handle  oem_handle);
static cmr_int camera_snapshot_deinit(cmr_handle  oem_handle);
static cmr_int camera_ipm_init(cmr_handle  oem_handle);
static cmr_int camera_ipm_deinit(cmr_handle  oem_handle);
static cmr_int camera_setting_init(cmr_handle  oem_handle);
static cmr_int camera_setting_deinit(cmr_handle  oem_handle);
static cmr_int camera_focus_init(cmr_handle  oem_handle);
static cmr_int camera_focus_deinit(cmr_handle  oem_handle);
static cmr_int camera_preview_cb_thread_proc(struct cmr_msg *message, void* data);
static cmr_int camera_snapshot_cb_thread_proc(struct cmr_msg *message, void* data);
static cmr_int camera_snapshot_secondary_thread_proc(struct cmr_msg *message, void* data);
static cmr_int camera_snapshot_send_raw_thread_proc(struct cmr_msg *message, void* data);
static cmr_int camera_create_prev_thread(cmr_handle oem_handle);
static cmr_int camera_destroy_prev_thread(cmr_handle oem_handle);
static cmr_int camera_create_snp_thread(cmr_handle oem_handle);
static cmr_int camera_destroy_snp_thread(cmr_handle oem_handle);
static cmr_int camera_init_thread(cmr_handle oem_handle);
static cmr_int camera_deinit_thread(cmr_handle oem_handle);
static cmr_int camera_res_init(cmr_handle);
static cmr_int camera_res_deinit(cmr_handle);
static cmr_int camera_res_init_internal(cmr_handle oem_handle);
static cmr_int camera_res_deinit_internal(cmr_handle oem_handle);
static cmr_int camera_init_thread_proc(struct cmr_msg *message, void *p_data);
static cmr_int camera_pre_preview_thread_proc(struct cmr_msg *message, void *p_data);
static cmr_int camera_pre_preview_start(cmr_handle oem_handle);
static cmr_int camera_pre_preview_proc(cmr_handle oem_handle);
static cmr_int camera_pre_preview_deinit(cmr_handle oem_handle);
static cmr_int camera_res_init_done(cmr_handle oem_handle);
static cmr_int camera_init_internal(cmr_handle oem_handle, cmr_uint is_autotest);
static cmr_int camera_deinit_internal(cmr_handle  oem_handle);
static cmr_int camera_preview_pre_proc(cmr_handle oem_handle, cmr_u32 camera_id, cmr_u32 preview_sn_mode);
static cmr_int camera_preview_post_proc(cmr_handle oem_handle, cmr_u32 camera_id);
static cmr_int camera_start_encode(cmr_handle oem_handle, cmr_handle caller_handle,
												struct img_frm *src, struct img_frm *dst, struct cmr_op_mean *mean);
static cmr_int camera_start_decode(cmr_handle oem_handle, cmr_handle caller_handle, struct img_frm *src,
                                                struct img_frm *dst, struct cmr_op_mean *mean);
static cmr_int camera_start_exif_encode(cmr_handle oem_handle, cmr_handle caller_handle,
                                                       struct img_frm *pic_src, struct img_frm *thumb_src,
                                                       void *exif, struct img_frm *dst, struct jpeg_wexif_cb_param *out_ptr);
static cmr_int camera_stop_codec(cmr_handle oem_handle);
static cmr_int camera_start_scale(cmr_handle oem_handle, cmr_handle caller_handle, struct img_frm *src,
                                             struct img_frm *dst, struct cmr_op_mean *mean);
static cmr_int camera_start_rot(cmr_handle oem_handle, cmr_handle caller_handle, struct img_frm *src,
                                         struct img_frm *dst, struct cmr_op_mean *mean);
static cmr_int camera_ipm_pre_proc(cmr_handle oem_handle, void * private_data);
static cmr_int camera_capture_pre_proc(cmr_handle oem_handle, cmr_u32 camera_id, cmr_u32 preview_mode, cmr_u32 capture_mode, cmr_u32 is_restart, cmr_u32 is_sn_reopen);
static cmr_int camera_capture_post_proc(cmr_handle oem_handle, cmr_u32 camera_id);
static cmr_int camera_open_sensor(cmr_handle oem_handle, cmr_u32 camera_id);
static cmr_int camera_close_sensor(cmr_handle oem_handle, cmr_u32 camera_id);
static cmr_int camera_raw_proc(cmr_handle oem_handle, cmr_handle caller_handle, struct raw_proc_param *param_ptr);
static cmr_int camera_isp_start_video(cmr_handle oem_handle, struct video_start_param *param_ptr);
static cmr_int camera_isp_stop_video(cmr_handle oem_handle);
static cmr_int camera_channel_cfg(cmr_handle oem_handle, cmr_handle caller_handle, cmr_u32 camera_id,
											  struct channel_start_param *param_ptr, cmr_u32 *channel_id, struct img_data_end *endian);
static cmr_int camera_pre_channel_cfg(cmr_handle oem_handle, cmr_handle caller_handle, cmr_u32 camera_id,
                                              struct channel_start_param *param_ptr);
static cmr_int camera_channel_start(cmr_handle oem_handle, cmr_u32 channel_bits, cmr_uint skip_number);
static cmr_int camera_channel_pause(cmr_handle oem_handle, cmr_uint channel_id, cmr_u32 reconfig_flag);
static cmr_int camera_channel_resume(cmr_handle oem_handle, cmr_uint channel_id, cmr_u32 skip_number,
	                                                cmr_u32 deci_factor, cmr_u32 frm_num);
static cmr_int camera_channel_free_frame(cmr_handle oem_handle, cmr_u32 channel_id, cmr_u32 index);
static cmr_int camera_channel_stop(cmr_handle oem_handle, cmr_u32 channel_bits);
static cmr_int camera_pre_channel_stop(cmr_handle oem_handle, cmr_u32 channel_bits, cmr_u32 is_sensor_off);
static cmr_int camera_channel_buff_cfg (cmr_handle oem_handle, struct buffer_cfg *buf_cfg);
static cmr_int camera_channel_scale_capability(cmr_handle oem_handle, cmr_u32 *width, cmr_u32 *sc_factor, cmr_u32 *sc_threshold);
static cmr_int camera_channel_path_capability(cmr_handle oem_handle, struct cmr_path_capability *capability);
static cmr_int camera_channel_get_cap_time(cmr_handle oem_handle, cmr_u32 *sec, cmr_u32 *usec);
static cmr_int camera_set_hal_cb(cmr_handle oem_handle, camera_cb_of_type hal_cb);
static cmr_int camera_ioctl_for_setting(cmr_handle oem_handle, cmr_uint cmd_type, struct setting_io_parameter *param_ptr);
static cmr_int camera_sensor_ioctl(cmr_handle	oem_handle, cmr_uint cmd_type, struct common_sn_cmd_param *param_ptr);
static cmr_int camera_isp_ioctl(cmr_handle oem_handle, cmr_uint cmd_type, struct common_isp_cmd_param *param_ptr);
static void camera_get_iso_value(cmr_handle oem_handle);
static cmr_int camera_get_ae_lum_value(cmr_handle oem_handle);

static cmr_int camera_get_setting_activity(cmr_handle oem_handle, cmr_uint *is_active);
static cmr_int camera_set_preview_param(cmr_handle oem_handle, enum takepicture_mode mode,  cmr_uint is_snapshot);
static cmr_int camera_get_preview_param(cmr_handle oem_handle, enum takepicture_mode mode,
	                                                      cmr_uint is_snapshot, struct preview_param *out_param_ptr);
static cmr_int camera_get_snapshot_param(cmr_handle oem_handle, struct snapshot_param *out_ptr);
static cmr_int camera_get_sensor_info(cmr_handle oem_handle, cmr_uint sensor_id, struct sensor_exp_info *exp_info_ptr);
static cmr_int camera_get_sensor_autotest_mode(cmr_handle oem_handle, cmr_uint sensor_id, cmr_uint *is_autotest);
static cmr_int camera_set_setting(cmr_handle oem_handle, enum camera_param_type id, cmr_uint param);
static void camera_set_hdr_flag(struct camera_context *cxt, cmr_u32 hdr_flag);
static cmr_u32 camera_get_hdr_flag(struct camera_context *cxt);
static cmr_int camera_open_hdr(struct camera_context *cxt, struct ipm_open_in *in_ptr, struct ipm_open_out *out_ptr);
static cmr_int camera_close_hdr(struct camera_context *cxt);
static void camera_snapshot_channel_handle(cmr_handle oem_handle, void* param);
static void camera_post_share_path_available(cmr_handle oem_handle);
static void camera_set_share_path_sm_flag(cmr_handle oem_handle, cmr_uint flag);
static cmr_uint camera_get_share_path_sm_flag(cmr_handle oem_handle);
static void camera_wait_share_path_available(cmr_handle oem_handle);
static void camera_set_discard_frame(cmr_handle oem_handle, cmr_uint is_discard);
static cmr_uint camera_get_is_discard_frame(cmr_handle oem_handle, struct frm_info *data);
static void camera_set_snp_req(cmr_handle oem_handle, cmr_uint is_req);
static cmr_uint camera_get_snp_req(cmr_handle oem_handle);
static cmr_int camera_get_cap_time(cmr_handle snp_handle);
static cmr_int camera_check_cap_time(cmr_handle snp_handle, struct frm_info * data);
static void camera_snapshot_started(cmr_handle oem_handle);
static cmr_uint camera_param_to_isp(cmr_uint cmd, struct common_isp_cmd_param *parm);
static cmr_int camera_restart_rot(cmr_handle oem_handle);
static cmr_int camera_close_flash(cmr_handle oem_handle, cmr_u32 camera_id);
/**********************************************************************************************/

void camera_malloc(cmr_u32 mem_type, cmr_handle oem_handle, cmr_u32 *size_ptr,
	                        cmr_u32 *sum_ptr, cmr_uint *phy_addr, cmr_uint *vir_addr)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;

	if (!oem_handle || !phy_addr || !vir_addr || !size_ptr || !sum_ptr) {
		CMR_LOGE("error param 0x%lx 0x%lx 0x%lx", (cmr_uint)oem_handle, (cmr_uint)phy_addr, (cmr_uint)vir_addr);
		return;
	}
	CMR_LOGI("mem type %d size %d sum %d", mem_type, *size_ptr, *sum_ptr);
	if (cxt->hal_malloc) {
		cxt->hal_malloc(mem_type, size_ptr, sum_ptr, phy_addr, vir_addr, cxt->client_data);
		CMR_LOGI("0x%lx 0x%lx", (cmr_uint)phy_addr, (cmr_uint)vir_addr);
	} else {
		CMR_LOGE("null");
	}
}

void camera_free(cmr_u32 mem_type, cmr_handle oem_handle,cmr_uint *phy_addr,
						cmr_uint *vir_addr, cmr_u32 sum)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;

	if (!oem_handle || !phy_addr || !vir_addr) {
		CMR_LOGE("error param 0x%lx 0x%lx 0x%lx", (cmr_uint)oem_handle, (cmr_uint)phy_addr, (cmr_uint)vir_addr);
		return;
	}
	CMR_LOGI("mem type %d sum %d", mem_type, sum);
	CMR_LOGV("free 0x%lx 0x%lx", (cmr_uint)phy_addr, (cmr_uint)vir_addr);
	cxt->hal_free(mem_type, phy_addr, vir_addr, sum, cxt->client_data);
}

void camera_snapshot_started(cmr_handle oem_handle)
{
	camera_snapshot_cb_to_hal(oem_handle, CAMERA_EXIT_CB_PREPARE, CAMERA_FUNC_TAKE_PICTURE, 0);
}

void camera_set_discard_frame(cmr_handle oem_handle, cmr_uint is_discard)
{
	struct camera_context           *cxt = (struct camera_context*)oem_handle;

	sem_wait(&cxt->access_sm);
	cxt->is_discard_frm = is_discard;
	sem_post(&cxt->access_sm);
	CMR_LOGI("%ld", cxt->is_discard_frm);
}

cmr_uint camera_get_is_discard_frame(cmr_handle oem_handle, struct frm_info *data)
{
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct snapshot_context         *snp_cxt = &cxt->snp_cxt;
	cmr_uint                        is_discard = 0;

	if (snp_cxt->channel_bits & (1 << data->channel_id)) {
		sem_wait(&cxt->access_sm);
		is_discard = cxt->is_discard_frm;
		sem_post(&cxt->access_sm);
	}
	CMR_LOGV("%ld", is_discard);
	return is_discard;
}

void camera_set_snp_req(cmr_handle oem_handle, cmr_uint is_req)
{
	struct camera_context           *cxt = (struct camera_context*)oem_handle;

	sem_wait(&cxt->access_sm);
	cxt->snp_cxt.is_req_snp = is_req;
	sem_post(&cxt->access_sm);
	CMR_LOGI("%ld", cxt->snp_cxt.is_req_snp);
}

cmr_uint camera_get_snp_req(cmr_handle oem_handle)
{
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	cmr_uint                        is_req;

	sem_wait(&cxt->access_sm);
	is_req = cxt->snp_cxt.is_req_snp;
	sem_post(&cxt->access_sm);
	CMR_LOGV("%ld", is_req);

	return is_req;
}

void camera_send_channel_data(cmr_handle oem_handle, cmr_handle receiver_handle, cmr_uint evt, void *data)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct frm_info                 *frm_ptr = data;
	cmr_u32                         chn_bit;
	struct buffer_cfg               buf_cfg;
	cmr_int                         need_pause;

	(cmr_handle)(receiver_handle);
	if (!frm_ptr) {
		CMR_LOGE("err, frame is null");
		goto exit;
	}

	camera_local_zsl_snapshot_need_pause(oem_handle, &need_pause);
	chn_bit = 1 << frm_ptr->channel_id;

	CMR_LOGV("%d %d %d %d", frm_ptr->channel_id, cxt->prev_cxt.channel_bits, cxt->snp_cxt.channel_bits, cxt->snp_cxt.total_num);
	if (cxt->prev_cxt.channel_bits & chn_bit) {
		ret = cmr_preview_receive_data(cxt->prev_cxt.preview_handle, cxt->camera_id, evt, data);
	}
	if (cxt->prev_cxt.video_channel_bits & chn_bit) {
		cmr_copy(&cxt->prev_cxt.video_cur_chn_data, frm_ptr, sizeof(struct frm_info));
		ret = cmr_preview_receive_data(cxt->prev_cxt.preview_handle, cxt->camera_id, evt, data);
	}
	if (cxt->snp_cxt.channel_bits & chn_bit) {
		if (TAKE_PICTURE_NEEDED == camera_get_snp_req((cmr_handle)cxt) && CAMERA_ZSL_MODE != cxt->snp_cxt.snp_mode) {
			ret = cmr_snapshot_receive_data(cxt->snp_cxt.snapshot_handle, SNAPSHOT_EVT_CHANNEL_DONE, data);
			if (need_pause) {
				if (CAMERA_ZSL_MODE == cxt->snp_cxt.snp_mode || 1 != cxt->snp_cxt.total_num) {
					camera_set_discard_frame(cxt, 1);
					ret = cmr_preview_receive_data(cxt->prev_cxt.preview_handle, cxt->camera_id, PREVIEW_CHN_PAUSE, data);
					if (ret) {
						CMR_LOGE("failed to pause path %ld", ret);
					}
					ret = cmr_preview_receive_data(cxt->prev_cxt.preview_handle, cxt->camera_id, evt, data);
				} else {
					ret = cmr_preview_receive_data(cxt->prev_cxt.preview_handle, cxt->camera_id, evt, data);
					camera_post_share_path_available(oem_handle);
				}
			} else {
				ret = cmr_preview_receive_data(cxt->prev_cxt.preview_handle, cxt->camera_id, evt, data);
				camera_post_share_path_available(oem_handle);
			}
		} else {
			if (cxt->snp_cxt.zsl_frame) {
				cmr_copy(&cxt->snp_cxt.cur_chn_data, frm_ptr, sizeof(struct frm_info));
				ret = cmr_preview_receive_data(cxt->prev_cxt.preview_handle, cxt->camera_id, evt, data);
			} else {
			ret = cmr_snapshot_receive_data(cxt->snp_cxt.snapshot_handle, SNAPSHOT_EVT_FREE_FRM, data);
			}
		}
	}
exit:
	if (ret) {
		CMR_LOGE("failed to send channel data %ld", ret);
	}
}

/*
*privdata:oem handle
*/
cmr_int camera_sensor_streamctrl(cmr_u32 on_off, void *privdata)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)privdata;

	if (!cxt) {
		CMR_LOGE("error param");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	ret = cmr_sensor_stream_ctrl(cxt->sn_cxt.sensor_handle, cxt->camera_id, on_off);
	if (ret) {
		CMR_LOGE("err to set stream %ld", ret);
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int camera_isp_ctrl_done(cmr_u32 cmd, void* data)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;

	(void*)data;
	if (cmd >= ISP_CTRL_MAX) {
		CMR_LOGE("isp wrong cmd %d", cmd);
		ret =  -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	CMR_LOGI("isp cmd, 0x%x, ret %ld", cmd, ret);
exit:
	return ret;
}

void camera_sensor_evt_cb(cmr_int evt, void* data, void* privdata)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)privdata;

	if (!cxt || !data || CMR_EVT_SENSOR_BASE != (CMR_EVT_SENSOR_BASE & evt)) {
		CMR_LOGE("error param, handle 0x%lx data 0x%lx evt 0x%lx" , (cmr_uint)cxt, (cmr_uint)data, evt);
		goto exit;
	}
	CMR_LOGI("evt 0x%lx, handle 0x%lx", evt, (cmr_uint)privdata);
	switch (evt) {
	case CMR_SENSOR_FOCUS_MOVE:
		if (1 == cxt->focus_cxt.inited) {
			ret = cmr_focus_sensor_handle(cxt->focus_cxt.focus_handle, CMR_SENSOR_FOCUS_MOVE, cxt->camera_id, data);
		} else {
			CMR_LOGE("err, focus hasn't been initialized");
		}
		break;
	case CMR_SENSOR_ERROR:
		if (1 == cxt->prev_cxt.inited) {
			ret = cmr_preview_receive_data(cxt->prev_cxt.preview_handle, cxt->camera_id, evt, data);
		} else {
			CMR_LOGE("err, preview hasn't been initialized");
		}
		break;
	default:
		CMR_LOGE("can't handle ths evt, 0x%lx", evt);
	}
exit:
	if (ret) {
		CMR_LOGE("failed %ld", ret);
	}
}

cmr_int camera_is_need_change_fmt(cmr_handle oem_handle, struct frm_info *data_ptr)
{
	cmr_int                         is_change_fmt = 0;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct snapshot_context         *snp_cxt = &cxt->snp_cxt;
	cmr_uint                        is_snp_frm = 0;

	is_snp_frm = ((1 << data_ptr->channel_id) & snp_cxt->channel_bits);
	if ( is_snp_frm && (1 == camera_get_hdr_flag(cxt))) {
		if (IMG_DATA_TYPE_JPEG == data_ptr->fmt || IMG_DATA_TYPE_RAW == data_ptr->fmt) {
			is_change_fmt = 1;
		}
	}
	return is_change_fmt;
}

cmr_int camera_get_cap_time(cmr_handle snp_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)snp_handle;
	cmr_u32                         sec = 0;
	cmr_u32                         usec = 0;

	sem_wait(&cxt->access_sm);
	ret = cmr_grab_get_cap_time(cxt->grab_cxt.grab_handle, &sec, &usec);
	CMR_LOGI("cap time %d %d", sec, usec);
	cxt->snp_cxt.cap_time_stamp = sec * SEC_TO_NANOSEC + usec * MS_TO_NANOSEC;
	sem_post(&cxt->access_sm);
	return ret;
}

cmr_int camera_check_cap_time(cmr_handle snp_handle, struct frm_info * data)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)snp_handle;
	cmr_s64                         frame_time = data->sec * SEC_TO_NANOSEC + data->usec * MS_TO_NANOSEC;

	CMR_LOGV("time %ld, %ld", data->sec, data->usec);
	sem_wait(&cxt->access_sm);
	if (TAKE_PICTURE_NEEDED == cxt->snp_cxt.is_req_snp && (cxt->snp_cxt.channel_bits & (1 << data->channel_id))) {
		if (frame_time <= cxt->snp_cxt.cap_time_stamp) {
			CMR_LOGW("frame is earlier than picture, drop!");
			ret = CMR_CAMERA_FAIL;
		} else {
			CMR_LOGV("frame time OK!");
		}
	}
	sem_post(&cxt->access_sm);
	return ret;
}

cmr_int camera_get_post_proc_chn_out_frm_id(struct img_frm* frame, struct frm_info* data)
{
	cmr_int                         i;

	for (i = 0; i < CMR_CAPTURE_MEM_SUM; i++) {
		if(data->yaddr == (frame+i)->addr_phy.addr_y && data->uaddr == (frame+i)->addr_phy.addr_u) {
			break;
		}
	}
	CMR_LOGI("frm id %ld", i);
	return i;
}

void camera_grab_handle(cmr_int evt, void* data, void* privdata)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)privdata;
	struct ipm_context              *ipm_cxt = &cxt->ipm_cxt;
	struct frm_info                 *frame = (struct frm_info*)data;
	cmr_u32                         channel_id;
	cmr_handle                      receiver_handle;
	cmr_u32                         chn_bits = (1 <<  frame->channel_id);
	cmr_u32                         frm_id;
	struct buffer_cfg               buf_cfg;

	if ((camera_get_is_discard_frame((cmr_handle)cxt, frame) || camera_check_cap_time((cmr_handle)cxt, frame)) && CAMERA_ZSL_MODE != cxt->snp_cxt.snp_mode) {
		memset(&buf_cfg, 0, sizeof(buf_cfg));
		buf_cfg.channel_id = frame->channel_id;
		buf_cfg.base_id = CMR_BASE_ID(frame->frame_id);
		buf_cfg.count = 1;
		buf_cfg.flag = BUF_FLAG_RUNNING;
		buf_cfg.addr[0].addr_y = frame->yaddr;
		buf_cfg.addr[0].addr_u = frame->uaddr;
		buf_cfg.addr[0].addr_v = frame->vaddr;
		buf_cfg.addr_vir[0].addr_y = frame->yaddr_vir;
		buf_cfg.addr_vir[0].addr_u = frame->uaddr_vir;
		buf_cfg.addr_vir[0].addr_v = frame->vaddr_vir;
		camera_channel_buff_cfg(cxt, &buf_cfg);
		return;
	}
	receiver_handle = cxt->grab_cxt.caller_handle[frame->channel_id];
	if ((0 != cxt->snp_cxt.channel_bits) && (TAKE_PICTURE_NEEDED == camera_get_snp_req((cmr_handle)cxt))
		&& (1 == camera_get_hdr_flag(cxt))) {
		struct img_frm out_param;
		struct ipm_frame_in  ipm_in_param;
		struct ipm_frame_out imp_out_param;
		/* for bug 396318, will be removed later */
		camera_set_discard_frame((cmr_handle)cxt, 1);
		ret = cmr_preview_receive_data(cxt->prev_cxt.preview_handle, cxt->camera_id, evt, data);
		if (ret) {
			CMR_LOGE("failed to send a frame to preview %ld", ret);
		}
		if (camera_is_need_change_fmt((cmr_handle)cxt, frame)) {
			ret = cmr_snapshot_format_convert((cmr_handle)cxt, data, &out_param);//sync
			if (ret) {
				CMR_LOGE("failed to format convert %ld", ret);
				goto exit;
			}
		} else {
			frm_id = camera_get_post_proc_chn_out_frm_id(cxt->snp_cxt.post_proc_setting.chn_out_frm, frame);
			out_param = cxt->snp_cxt.post_proc_setting.chn_out_frm[frm_id];
		}
		cxt->snp_cxt.cur_frm_info = *frame;
		ipm_cxt->frm_num++;
		ipm_in_param.src_frame = out_param;
		ipm_in_param.dst_frame = out_param;
		ipm_in_param.private_data = (void*)privdata;
		imp_out_param.dst_frame = out_param;
		imp_out_param.private_data = privdata;
		ret = ipm_transfer_frame(ipm_cxt->hdr_handle, &ipm_in_param, &imp_out_param);
		if (ret) {
			CMR_LOGE("failed to transfer frame to ipm %ld", ret);
			goto exit;
		}
		cmr_snapshot_memory_flush(cxt->snp_cxt.snapshot_handle);
/*		if(ipm_cxt->frm_num == ipm_cxt->hdr_num) {
			camera_post_share_path_available((cmr_handle)cxt);
			sem_wait(&cxt->hdr_sync_sm);
			cxt->ipm_cxt.frm_num = 0;
		}*/
	} else {
		camera_send_channel_data((cmr_handle)cxt, receiver_handle, evt, data);
	}
exit:
	if (ret) {
		if (cxt->camera_cb) {
			cmr_snapshot_stop((cmr_handle)cxt);
			camera_set_snp_req((cmr_handle)cxt, TAKE_PICTURE_NO);
			cxt->camera_cb(CAMERA_EXIT_CB_FAILED, cxt->client_data, CAMERA_FUNC_TAKE_PICTURE, NULL);
		}
	}
	return;
}

void camera_grab_evt_cb(cmr_int evt, void* data, void* privdata)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)privdata;
	struct frm_info                 *frame = (struct frm_info*)data;
	cmr_u32                         channel_id;
	cmr_handle                      receiver_handle;

	if (!cxt || !data || !privdata || CMR_EVT_GRAB_BASE != (CMR_EVT_GRAB_BASE & evt)) {
		CMR_LOGE("error param, handle 0x%lx data 0x%lx evt 0x%lx", (cmr_uint)cxt, (cmr_uint)data, evt);
		return;
	}
	CMR_LOGI("evt 0x%lx, handle 0x%lx", evt, (cmr_uint)privdata);

	channel_id = frame->channel_id;
	if (channel_id >= GRAB_CHANNEL_MAX) {
		CMR_LOGE("error param, channel id %d" , channel_id);
		return;
	}

	switch (evt) {
	case CMR_GRAB_TX_DONE:
#ifdef OEM_HANDLE_HDR
		camera_grab_handle(evt, data, privdata);
#else
		camera_send_channel_data((cmr_handle)cxt, receiver_handle, evt, data);
#endif
		break;
	case CMR_GRAB_TX_ERROR:
	case CMR_GRAB_TX_NO_MEM:
	case CMR_GRAB_CSI2_ERR:
	case CMR_GRAB_TIME_OUT:
		ret = cmr_preview_receive_data(cxt->prev_cxt.preview_handle, cxt->camera_id, evt, data);
		if (ret) {
			CMR_LOGE("fail to handle error, ret %ld", ret);
		}
		break;
	default:
		CMR_LOGE("don't support evt 0x%lx", evt);
		break;
	}
}

void camera_scaler_evt_cb(cmr_int evt, void* data, void *privdata)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)privdata;

	if (!cxt || !data || !privdata) {
		CMR_LOGE("err, scale callback param");
		ret = - CMR_CAMERA_INVALID_PARAM;
		return;
	}
	CMR_LOGI("evt 0x%lx, handle 0x%lx", evt, (cmr_uint)privdata);

	if (CMR_IMG_CVT_SC_DONE == evt) {
		camera_take_snapshot_step(CMR_STEP_SC_E);
		cmr_snapshot_receive_data((cmr_handle)privdata, SNAPSHOT_EVT_SC_DONE, data);
	} else {
		CMR_LOGE("err, don't support evt 0x%lx", evt);
	}
}

void camera_jpeg_evt_cb(cmr_int evt, void* data, void *privdata)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)privdata;
	cmr_u32                         temp_evt;

	if (NULL == data || !privdata || CMR_EVT_JPEG_BASE != (CMR_EVT_JPEG_BASE & evt)) {
		CMR_LOGE("err, param, 0x%lx 0x%lx", (cmr_uint)data, evt);
		return;
	}
	CMR_LOGI("evt 0x%lx, handle 0x%lx", evt, (cmr_uint)privdata);

	switch (evt) {
	case CMR_JPEG_ENC_DONE:
		camera_take_snapshot_step(CMR_STEP_JPG_ENC_E);
		temp_evt = SNAPSHOT_EVT_JPEG_ENC_DONE;
		break;
	case CMR_JPEG_DEC_DONE:
		temp_evt = SNAPSHOT_EVT_JPEG_DEC_DONE;
		break;
	case CMR_JPEG_ENC_ERR:
		temp_evt = SNAPSHOT_EVT_JPEG_ENC_ERR;
		break;
	case CMR_JPEG_DEC_ERR:
		temp_evt = SNAPSHOT_EVT_JPEG_DEC_ERR;
		break;
	default:
		ret = - CMR_CAMERA_NO_SUPPORT;
		CMR_LOGE("err, don't support evt 0x%lx", evt);
	}
	if (ret) {
		CMR_LOGE("done %ld", ret);
		return;
	}
	ret = cmr_snapshot_receive_data(cxt->snp_cxt.snapshot_handle, temp_evt, data);
	if (ret) {
		CMR_LOGE("failed %ld", ret);
	}
}

cmr_int camera_isp_evt_cb(cmr_handle oem_handle, cmr_u32 evt, void* data, cmr_u32 data_len)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	cmr_u32                         sub_type;
	cmr_u32                         cmd = evt & 0xFF;

	if (!oem_handle || CMR_EVT_ISP_BASE != (CMR_EVT_ISP_BASE & evt)) {
		CMR_LOGE("err param, 0x%lx 0x%x 0x%lx", (cmr_uint)data, evt, (cmr_uint)oem_handle);
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	CMR_LOGI("evt 0x%x, handle 0x%lx len %d", evt, (cmr_uint)oem_handle, data_len);

	sub_type = (~CMR_EVT_ISP_BASE) & evt;
	CMR_LOGI("sub_type %0x", sub_type);

	if ((sub_type & ISP_EVT_MASK) == 0) {
		ret = camera_isp_ctrl_done(cmd, data);
		goto exit;
	}

	switch (sub_type & ISP_EVT_MASK) {
	case ISP_PROC_CALLBACK:
		if (camera_is_need_change_fmt((cmr_handle)cxt, data)) {
			ret = cmr_snapshot_receive_data(cxt->snp_cxt.snapshot_handle, SNAPSHOT_EVT_CVT_RAW_DATA, data);
		} else {
			ret = cmr_snapshot_receive_data(cxt->snp_cxt.snapshot_handle, SNAPSHOT_EVT_RAW_PROC, data);
		}
		break;
	case ISP_AF_NOTICE_CALLBACK:
		CMR_LOGI("ISP_AF_NOTICE_CALLBACK come here");
		if (1 == cxt->focus_cxt.inited) {
			ret = cmr_focus_isp_handle(cxt->focus_cxt.focus_handle, FOCUS_EVT_ISP_AF_NOTICE, cxt->camera_id, data);
		}
		break;
	case ISP_FLASH_READY_CALLBACK:
		ret = cmr_setting_flash_ready(cxt->setting_cxt.setting_handle, data);
		break;
	case ISP_FLASH_STAB_CALLBACK:
		ret = cmr_setting_flash_stab(cxt->setting_cxt.setting_handle, data);
		break;
	case ISP_QUICK_MODE_DOWN:
		cmr_setting_quick_ae_notice_done(cxt->setting_cxt.setting_handle, data);
		CMR_LOGI("ISP_QUICK_MODE_DOWN");
		break;

	default:
		break;
	}
exit:
	return ret;
}

static void camera_focus_evt_cb(enum af_cb_type cb, cmr_s32 param, void *privdata)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)privdata;
	cmr_int                         oem_cb;

	if (!privdata) {
		CMR_LOGE("err, handle for callback");
		return;
	}
	switch (cb) {
	case AF_CB_DONE:
		oem_cb = CAMERA_EXIT_CB_DONE;
		break;
    case AF_CB_FAILED:
		oem_cb = CAMERA_EXIT_CB_FAILED;
		break;
    case AF_CB_ABORT:
		oem_cb = CAMERA_EXIT_CB_ABORT;
		break;
    case AF_CB_FOCUS_MOVE:
		oem_cb = CAMERA_EVT_CB_FOCUS_MOVE;
		break;
	default:
		CMR_LOGE("failed focus cb %d", cb);
		ret = -CMR_CAMERA_NO_SUPPORT;
	}
	if (ret) {
		CMR_LOGE("done %ld", ret);
		return;
	}
	CMR_LOGI("param = 0x%x camera_cb 0x%lx focus cb %ld, oem cb 0x%lx", param, (cmr_uint)cb, (cmr_uint)oem_cb, (cmr_uint)cxt->camera_cb);
	if (cxt->camera_cb) {
		CMR_LOGV("cxt->camera_cb run");
		cxt->camera_cb(oem_cb, cxt->client_data, CAMERA_FUNC_START_FOCUS, (void*)param);
	} else {
		CMR_LOGI("cxt->camera_cb null error");
	}
}

cmr_int camera_preview_cb(cmr_handle oem_handle, enum preview_cb_type cb_type, enum preview_func_type func, void *param)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	cmr_uint                        oem_func;
	cmr_uint                        oem_cb_type;
	CMR_MSG_INIT(message);

	if (!oem_handle) {
		CMR_LOGE("error handle");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	if (PREVIEW_FUNC_START_PREVIEW == func) {
		oem_func = CAMERA_FUNC_START_PREVIEW;
	} else if (PREVIEW_FUNC_STOP_PREVIEW == func) {
		oem_func = CAMERA_FUNC_STOP_PREVIEW;
	} else if (PREVIEW_FUNC_START_CAPTURE == func) {
		oem_func = CAMERA_FUNC_TAKE_PICTURE;
	} else {
		CMR_LOGE("err, %d", func);
		goto exit;
	}
	CMR_LOGV("%ld %d", oem_func, cb_type);
	switch (cb_type) {
	case PREVIEW_RSP_CB_SUCCESS:
		oem_cb_type = CAMERA_RSP_CB_SUCCESS;
		break;
	case PREVIEW_EVT_CB_FRAME:
		oem_cb_type = CAMERA_EVT_CB_FRAME;
		break;
	case PREVIEW_EXIT_CB_FAILED:
		oem_cb_type = CAMERA_EXIT_CB_FAILED;
		break;
	case PREVIEW_EVT_CB_FLUSH:
		oem_cb_type = CAMERA_EVT_CB_FLUSH;
		break;
	case PREVIEW_EVT_CB_FD:
		oem_cb_type = CAMERA_EVT_CB_FD;
		if (param) {
			struct camera_frame_type *frame_param = (struct camera_frame_type*)param;
			struct isp_face_area face_area;
			int32_t sx = 0;
			int32_t sy = 0;
			int32_t ex = 0;
			int32_t ey = 0;
			int32_t i = 0;
			struct img_rect src_prev_rect;
			struct sensor_mode_info *sensor_mode_info = NULL;
			cmr_uint sn_mode = 0;

			face_area.frame_width = frame_param->width;
			face_area.frame_height = frame_param->height;
			face_area.face_num = frame_param->face_num;
			CMR_LOGD("face_num %d, size:%dx%d", face_area.face_num,
						face_area.frame_width, face_area.frame_height);

			cxt->fd_face_area.frame_width = frame_param->width;
			cxt->fd_face_area.frame_height = frame_param->height;
			cxt->fd_face_area.face_num = frame_param->face_num;

			for (i = 0; i < face_area.face_num; i++) {
				sx = MIN(MIN(frame_param->face_info[i].sx, frame_param->face_info[i].srx),
						MIN(frame_param->face_info[i].ex, frame_param->face_info[i].elx));
				sy = MIN(MIN(frame_param->face_info[i].sy, frame_param->face_info[i].sry),
						MIN(frame_param->face_info[i].ey, frame_param->face_info[i].ely));
				ex = MAX(MAX(frame_param->face_info[i].sx, frame_param->face_info[i].srx),
						MAX(frame_param->face_info[i].ex, frame_param->face_info[i].elx));
				ey = MAX(MAX(frame_param->face_info[i].sy, frame_param->face_info[i].sry),
						MAX(frame_param->face_info[i].ey, frame_param->face_info[i].ely));
/*
				face_area.face_info[i].sx = sx;;
				face_area.face_info[i].sy = sy;
				face_area.face_info[i].ex = ex;
				face_area.face_info[i].ey = ey;
*/
				//save face info in cmr cxt for other case.such as face beauty takepicture
				cxt->fd_face_area.face_info[i].sx = sx;;
				cxt->fd_face_area.face_info[i].sy = sy;
				cxt->fd_face_area.face_info[i].ex = ex;
				cxt->fd_face_area.face_info[i].ey = ey;

				//note:now we get the preview face crop.but ISP need sensor's crop.so we need recovery crop.
				cmr_preview_get_prev_rect(cxt->prev_cxt.preview_handle, cxt->camera_id, &src_prev_rect);
				//CMR_LOGI("%d %d %d %d", src_prev_rect.start_x, src_prev_rect.start_y, src_prev_rect.width, src_prev_rect.height);

				cmr_sensor_get_mode(cxt->sn_cxt.sensor_handle, cxt->camera_id, &sn_mode);
				sensor_mode_info = &cxt->sn_cxt.sensor_info.mode_info[sn_mode];

				face_area.face_info[i].sx = 1.0*sx*src_prev_rect.width/face_area.frame_width+src_prev_rect.start_x;
				face_area.face_info[i].sy = 1.0*sy*src_prev_rect.height/face_area.frame_height+src_prev_rect.start_y;
				face_area.face_info[i].ex = 1.0*ex*src_prev_rect.width/face_area.frame_width+src_prev_rect.start_x;
				face_area.face_info[i].ey = 1.0*ey*src_prev_rect.height/face_area.frame_height+src_prev_rect.start_y;
				face_area.face_info[i].brightness = frame_param->face_info[i].brightness;
				face_area.face_info[i].pose = frame_param->face_info[i].angle;

				face_area.frame_width = sensor_mode_info->trim_width;
				face_area.frame_height = sensor_mode_info->trim_height;
				CMR_LOGI("face_area  width, height  %d, %d", face_area.frame_width, face_area.frame_height);
			}
			if (IMG_DATA_TYPE_RAW == cxt->sn_cxt.sensor_info.image_format
				&& (!cxt->is_vendor_hdr)  /* SS requires to disable FD when HDR is on */
				) {
				isp_ioctl(cxt->isp_cxt.isp_handle, ISP_CTRL_FACE_AREA, (void*)&face_area);
			}
		}

		break;

	case PREVIEW_EVT_CB_RESUME:
		oem_cb_type = CAMERA_EVT_CB_RESUME;
		break;

	default:
		CMR_LOGE("err, %d", cb_type);
		ret = -CMR_CAMERA_NO_SUPPORT;
	}
	if (ret) {
		goto exit;
	}

	/*if (CAMERA_FUNC_STOP_PREVIEW == oem_func && CAMERA_RSP_CB_SUCCESS == oem_cb_type) {
		CMR_LOGV("stop preview response, notify directly");
		if (cxt->camera_cb) {
			cxt->camera_cb(oem_cb_type, cxt->client_data, oem_func, param);
		}
		return ret;
	}
	*/

	if (param) {
		message.data = malloc(sizeof(struct camera_frame_type));
		if (!message.data) {
			CMR_LOGE("failed to malloc msg");
			ret = -CMR_CAMERA_NO_MEM;
			goto exit;
		}
		message.alloc_flag = 1;

		if ((cxt->is_lls_enable) && (PREVIEW_EVT_CB_FRAME == cb_type)) {
			struct camera_frame_type *prev_frame = (struct camera_frame_type *)param;
			prev_frame->lls_info = camera_get_ae_lum_value(oem_handle);
		}

		memcpy(message.data, param, sizeof(struct camera_frame_type));
	}
	message.msg_type = oem_func;
	message.sub_msg_type = oem_cb_type;
	message.sync_flag  = CMR_MSG_SYNC_NONE;
	ret = cmr_thread_msg_send(cxt->prev_cb_thr_handle, &message);
	if (ret) {
		CMR_LOGE("failed to send msg, ret %ld", ret);
		free(message.data);
	}
exit:
	return ret;
}

cmr_int camera_ipm_cb(cmr_u32 class_type, struct ipm_frame_out *cb_param)
{
	UNUSED(class_type);
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = NULL;
	struct frm_info                 frame;

	CMR_LOGI("s");
	if (!cb_param || !cb_param->private_data) {
		CMR_LOGE("error param");
		return -CMR_CAMERA_INVALID_PARAM;
	}
	cxt = (struct camera_context*)cb_param->private_data;
#ifdef OEM_HANDLE_HDR
	frame = cxt->snp_cxt.cur_frm_info;
	cmr_snapshot_memory_flush(cxt->snp_cxt.snapshot_handle);
	camera_post_share_path_available((cmr_handle)cxt);
	cxt->ipm_cxt.frm_num = 0;
#endif
	ret = cmr_snapshot_receive_data(cxt->snp_cxt.snapshot_handle, SNAPSHOT_EVT_HDR_DONE, &frame);
	if (ret) {
		CMR_LOGE("fail to send frame to snp %ld", ret);
	}
	CMR_LOGI("done %ld", ret);
	return ret;
}

void camera_snapshot_cb_to_hal(cmr_handle oem_handle, enum snapshot_cb_type cb, enum snapshot_func_type func, void* param)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	cmr_uint                        oem_func = CAMERA_FUNC_TYPE_MAX;
	cmr_uint                        oem_cb_type = CAMERA_CB_TYPE_MAX;
	cmr_handle                      send_thr_handle = cxt->snp_cb_thr_handle;
	struct camera_frame_type        *frame_ptr = NULL;
	CMR_MSG_INIT(message);

	switch (func) {
	case SNAPSHOT_FUNC_RELEASE_PICTURE:
		oem_func = CAMERA_FUNC_RELEASE_PICTURE;
		break;
    case SNAPSHOT_FUNC_TAKE_PICTURE:
		oem_func = CAMERA_FUNC_TAKE_PICTURE;
		break;
    case SNAPSHOT_FUNC_ENCODE_PICTURE:
		oem_func = CAMERA_FUNC_ENCODE_PICTURE;
		break;
    default:
		oem_func = func;
		break;
	}
	switch (cb) {
	case SNAPSHOT_RSP_CB_SUCCESS:
		oem_cb_type = CAMERA_RSP_CB_SUCCESS;
		break;
	case SNAPSHOT_EVT_CB_FLUSH:
		oem_cb_type = CAMERA_EVT_CB_FLUSH;
		break;
	case SNAPSHOT_EVT_CB_ZSL_NEW_FRM:
		oem_cb_type = CAMERA_EVT_CB_ZSL_FRM;
		break;
	case SNAPSHOT_EXIT_CB_FAILED:
		oem_cb_type = CAMERA_EXIT_CB_FAILED;
		break;
	case SNAPSHOT_EVT_CB_SNAPSHOT_DONE:
		oem_cb_type = CAMERA_EVT_CB_SNAPSHOT_DONE;
		send_thr_handle = cxt->snp_send_raw_image_handle;
		break;
	case SNAPSHOT_EXIT_CB_DONE:
		oem_cb_type = CAMERA_EXIT_CB_DONE;
		if (CAMERA_FUNC_TAKE_PICTURE == oem_func) {
			send_thr_handle = cxt->snp_send_raw_image_handle;
		}
		break;
	case SNAPSHOT_EVT_CB_CAPTURE_FRAME_DONE:
		oem_cb_type = CAMERA_EVT_CB_CAPTURE_FRAME_DONE;
		send_thr_handle = cxt->snp_secondary_thr_handle;
		break;
	case SNAPSHOT_EVT_CB_SNAPSHOT_JPEG_DONE:
		oem_cb_type = CAMERA_EVT_CB_SNAPSHOT_JPEG_DONE;
		break;
	default:
		oem_cb_type = cb;
		break;
	}
	CMR_LOGI("camera_cb %ld %ld", oem_cb_type, oem_func);
	if (param) {
		message.data = malloc(sizeof(struct camera_frame_type));
		if (!message.data) {
			CMR_LOGE("failed to malloc msg");
			ret = -CMR_CAMERA_NO_MEM;
			return;
		}
		message.alloc_flag = 1;
		frame_ptr = (struct camera_frame_type*)message.data;
		memcpy(message.data, param, sizeof(struct camera_frame_type));
		frame_ptr->sensor_info.exposure_time_numerator = cxt->sn_cxt.exif_info.ExposureTime.numerator;
		frame_ptr->sensor_info.exposure_time_denominator = cxt->sn_cxt.exif_info.ExposureTime.denominator;
	}
	message.msg_type = oem_func;
	message.sub_msg_type = oem_cb_type;
	if (CAMERA_EVT_CB_CAPTURE_FRAME_DONE == oem_cb_type) {
		message.sync_flag  = CMR_MSG_SYNC_RECEIVED;
	} else if ((CAMERA_EXIT_CB_PREPARE == oem_cb_type) || (CAMERA_EVT_CB_SNAPSHOT_JPEG_DONE == oem_cb_type)
		|| (CAMERA_EVT_CB_SNAPSHOT_DONE == oem_cb_type)) {
		message.sync_flag  = CMR_MSG_SYNC_NONE;
	} else {
		message.sync_flag  = CMR_MSG_SYNC_PROCESSED;
	}
	ret = cmr_thread_msg_send(send_thr_handle, &message);
	if (ret) {
		CMR_LOGE("failed to send msg %ld", ret);
		if (message.data) {
			free(message.data);
		}
	}
}

void camera_set_hdr_flag(struct camera_context *cxt, cmr_u32 hdr_flag)
{
	CMR_LOGI("flag %d", hdr_flag);
	sem_wait(&cxt->hdr_flag_sm);
	cxt->snp_cxt.is_hdr = hdr_flag;
	sem_post(&cxt->hdr_flag_sm);
}

cmr_u32 camera_get_hdr_flag(struct camera_context *cxt)
{
	cmr_u32                         hdr_flag = 0;
	sem_wait(&cxt->hdr_flag_sm);
	hdr_flag = cxt->snp_cxt.is_hdr;
	sem_post(&cxt->hdr_flag_sm);
	CMR_LOGI("%d", hdr_flag);
	return hdr_flag;
}

cmr_int camera_open_hdr(struct camera_context *cxt, struct ipm_open_in *in_ptr, struct ipm_open_out *out_ptr)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;

	CMR_LOGI("start");
	sem_wait(&cxt->hdr_flag_sm);
	ret = cmr_ipm_open(cxt->ipm_cxt.ipm_handle, IPM_TYPE_HDR, in_ptr, out_ptr, &cxt->ipm_cxt.hdr_handle);
	sem_post(&cxt->hdr_flag_sm);
	CMR_LOGI("end");
	return ret;
}

cmr_int camera_close_hdr(struct camera_context *cxt)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;

	sem_wait(&cxt->hdr_flag_sm);
	if (cxt->ipm_cxt.hdr_handle) {
		ret = cmr_ipm_close(cxt->ipm_cxt.hdr_handle);
		cxt->ipm_cxt.hdr_handle = 0;
	}
	sem_post(&cxt->hdr_flag_sm);
	CMR_LOGI("close hdr done %ld", ret);
	return ret;
}

void camera_snapshot_channel_handle(cmr_handle oem_handle, void* param)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct frm_info                 frame;
	struct camera_jpeg_param        *jpeg_param_ptr = &((struct camera_frame_type *)param)->jpeg_param;
	cmr_uint                        i;
	cmr_uint                        is_need_resume = 0;
	struct cmr_path_capability      capability;
	cmr_int                         need_pause;
#ifdef CAMERA_PATH_SHARE
	camera_local_zsl_snapshot_need_pause(oem_handle, &need_pause);
	if (need_pause) {
		if (CAMERA_ZSL_MODE == cxt->snp_cxt.snp_mode && 0 == cxt->prev_cxt.video_size.width) {
			camera_channel_path_capability(oem_handle, &capability);
			if (!(0 != cxt->prev_cxt.video_size.width && 0 != cxt->prev_cxt.video_size.height && !capability.is_video_prev_diff)) {
				is_need_resume = 1;
			}
		} else if (1 != cxt->snp_cxt.total_num) {
			if (jpeg_param_ptr) {
				if (1 != jpeg_param_ptr->need_free && TAKE_PICTURE_NEEDED == camera_get_snp_req((cmr_handle)cxt)) {
					is_need_resume = 1;
				}
			} else {
				CMR_LOGE("param is error");
			}

			struct camera_frame_type *frame_type = (struct camera_frame_type *)param;
			if (((cxt->lls_shot_mode)|| (cxt->is_vendor_hdr)) && (1 != frame_type->need_free)) {
				is_need_resume = 1;
			}
		} else {
			is_need_resume = 0;
		}

		if (1 == is_need_resume) {
			for (i=0 ; i<GRAB_CHANNEL_MAX ; i++) {
				if (cxt->snp_cxt.channel_bits & (1<<i)) {
					break;
				}
			}
			frame.channel_id = i;
			sem_wait(&cxt->resume_sync_sm);
			ret = cmr_preview_receive_data(cxt->prev_cxt.preview_handle, cxt->camera_id, PREVIEW_CHN_RESUME, &frame);
			sem_post(&cxt->resume_sync_sm);
			if (ret) {
				CMR_LOGE("failed to resume path %ld", ret);
			}
			CMR_LOGI("done");
		}
	}
#endif
}

void camera_snapshot_state_handle(cmr_handle oem_handle, enum snapshot_cb_type cb, enum snapshot_func_type func, void* param)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;

	if (SNAPSHOT_FUNC_STATE == func) {
		switch (cb) {
		case SNAPSHOT_EVT_START_ROT:
			CMR_LOGI("start rot");
			break;
		case SNAPSHOT_EVT_ROT_DONE:
			CMR_LOGI("rot done");
			break;
		case SNAPSHOT_EVT_START_SCALE:
			CMR_LOGI("start scaler");
			break;
		case SNAPSHOT_EVT_SCALE_DONE:
			CMR_LOGI("scaler done");
			break;
		case SNAPSHOT_EVT_START_ENC:
			CMR_LOGI("start jpeg enc");
			break;
		case SNAPSHOT_EVT_ENC_DONE:
			CMR_LOGI("close hdr before jpeg enc done");
			if (1 == camera_get_hdr_flag(cxt)) {
				ret = camera_close_hdr(cxt);
			}
			CMR_LOGI("jpeg enc done");
			break;
		case SNAPSHOT_EVT_START_CONVERT_THUMB:
			CMR_LOGI("start to convert thumb");
			break;
		case SNAPSHOT_EVT_CONVERT_THUMB_DONE:
			CMR_LOGI("convert thumb done");
			break;
		case SNAPSHOT_EVT_ENC_THUMB:
			CMR_LOGI("start to enc thumb");
			break;
		case SNAPSHOT_EVT_ENC_THUMB_DONE:
			CMR_LOGI("thumb enc done");
			break;
		case SNAPSHOT_EVT_START_DEC:
			CMR_LOGI("start to jpeg dec");
			break;
		case SNAPSHOT_EVT_DEC_DONE:
			CMR_LOGI("jpeg dec done");
			break;
		case SNAPSHOT_EVT_START_EXIF_JPEG:
			CMR_LOGI("start to compound jpeg with exif");
			break;
		case SNAPSHOT_EVT_EXIF_JPEG_DONE:
			CMR_LOGI("jpeg done with exif");
	//		camera_snapshot_channel_handle(cxt);
			break;
		case SNAPSHOT_EVT_STATE:
			cxt->snp_cxt.status = ((struct camera_frame_type *)param)->status;
			CMR_LOGI("snapshot state is %d", cxt->snp_cxt.status);
			break;
		default:
			CMR_LOGE("don't support cb %d", cb);
			break;
		}
	}
    if (SNAPSHOT_FUNC_RECOVERY == func) {
		//to do
    }
}

void camera_post_share_path_available(cmr_handle oem_handle)
{
	struct camera_context           *cxt = (struct camera_context*)oem_handle;

	CMR_LOGI("post beging");
	camera_set_share_path_sm_flag(oem_handle, 0);
	sem_post(&cxt->share_path_sm);
	CMR_LOGI("post end");
}

void camera_set_share_path_sm_flag(cmr_handle oem_handle, cmr_uint flag)
{
	struct camera_context           *cxt = (struct camera_context*)oem_handle;

	CMR_LOGI("%ld", flag);
	sem_wait(&cxt->access_sm);
	cxt->share_path_sm_flag = flag;
	sem_post(&cxt->access_sm);
}

cmr_uint camera_get_share_path_sm_flag(cmr_handle oem_handle)
{
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	cmr_uint                        flag = 0;

	sem_wait(&cxt->access_sm);
	flag = cxt->share_path_sm_flag;
	sem_post(&cxt->access_sm);
	CMR_LOGI("%ld", flag);
	return flag;
}

void camera_wait_share_path_available(cmr_handle oem_handle)
{
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct cmr_path_capability      capability;
	cmr_int                         need_pause;

	CMR_LOGI("wait beging");
	camera_local_zsl_snapshot_need_pause(oem_handle, &need_pause);
	camera_channel_path_capability(oem_handle, &capability);
	if (CAMERA_ZSL_MODE == cxt->snp_cxt.snp_mode && !need_pause) {
		CMR_LOGI("no need wait");
	} else {
		if (!(0 != cxt->prev_cxt.video_size.width && 0 != cxt->prev_cxt.video_size.height && !capability.is_video_prev_diff)) {
			camera_set_share_path_sm_flag(oem_handle, 1);
			sem_wait(&cxt->share_path_sm);
		}
	}
	CMR_LOGI("wait end");
}

void camera_snapshot_cb(cmr_handle oem_handle, enum snapshot_cb_type cb, enum snapshot_func_type func, void* param)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	cmr_uint                        oem_func;
	cmr_uint                        oem_cb_type;
	struct camera_jpeg_param        enc_param;

	if (!oem_handle) {
		CMR_LOGE("error handle");
		return;
	}
	CMR_LOGI("func %d", func);
	if (SNAPSHOT_FUNC_TAKE_PICTURE == func || SNAPSHOT_FUNC_ENCODE_PICTURE == func) {
		if ((SNAPSHOT_FUNC_ENCODE_PICTURE == func) && (SNAPSHOT_EXIT_CB_DONE == cb) && param) {
			enc_param = ((struct camera_frame_type *)param)->jpeg_param;
			if (enc_param.need_free) {
				camera_set_snp_req(oem_handle, TAKE_PICTURE_NO);
				camera_set_discard_frame(cxt, 0);
			}
		}
		camera_snapshot_cb_to_hal(oem_handle, cb, func, param);
		if ((SNAPSHOT_FUNC_ENCODE_PICTURE == func) && (SNAPSHOT_EXIT_CB_DONE == cb)) {
			camera_snapshot_channel_handle(oem_handle, param);
		}

		if (cxt->lls_shot_mode || cxt->is_vendor_hdr) {
			if ((SNAPSHOT_FUNC_TAKE_PICTURE == func) && (SNAPSHOT_EVT_CB_SNAPSHOT_JPEG_DONE == cb) && param) {
		        struct camera_frame_type *frame_type = (struct camera_frame_type *)param;

				if (frame_type->need_free) {
					camera_set_snp_req(oem_handle, TAKE_PICTURE_NO);
					camera_set_discard_frame(cxt, 0);
				}
				camera_snapshot_channel_handle(oem_handle, param);
			}
		}

		if ((SNAPSHOT_FUNC_TAKE_PICTURE == func) && (SNAPSHOT_RSP_CB_SUCCESS == cb)) {
			camera_wait_share_path_available(oem_handle);
		}
	} else if (SNAPSHOT_FUNC_RELEASE_PICTURE == func) {
		if (cxt->camera_cb) {
			cxt->camera_cb(CAMERA_RSP_CB_SUCCESS, cxt->client_data, CAMERA_FUNC_RELEASE_PICTURE, NULL);
		}
	} else {
		camera_snapshot_state_handle(oem_handle, cb, func, param);
	}

}

cmr_int camera_before_set(cmr_handle oem_handle, enum preview_param_mode mode)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;

	if (!oem_handle) {
		CMR_LOGE("error handle");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	ret = cmr_preview_before_set_param(cxt->prev_cxt.preview_handle, cxt->camera_id, mode);
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int camera_after_set(cmr_handle oem_handle, struct after_set_cb_param *param)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	cmr_u32                         skip_num = 0;

	if (!oem_handle) {
		CMR_LOGE("error handle");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	if (PREVIEWING == cmr_preview_get_status(cxt->prev_cxt.preview_handle, cxt->camera_id) &&
		(IMG_DATA_TYPE_RAW == cxt->sn_cxt.sensor_info.image_format)) {
		skip_num = 0;
	} else {
		skip_num = param->skip_number;
	}
	CMR_LOGI("sensor fmt %d, skip num %d", cxt->sn_cxt.sensor_info.image_format, skip_num);

	ret = cmr_preview_after_set_param(cxt->prev_cxt.preview_handle, cxt->camera_id, param->re_mode, param->skip_mode, skip_num);

exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int camera_focus_visit_flash_info(cmr_handle oem_handle,cmr_uint camera_id)
{
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct setting_cmd_parameter    setting_param;

	setting_param.camera_id = camera_id;
	setting_param.cmd_type_value = 2;
	cmr_setting_ioctl(cxt->setting_cxt.setting_handle, SETTING_GET_HW_FLASH_STATUS, &setting_param);

	return setting_param.cmd_type_value;// 1 stands for pre-flash turned on

}

cmr_int camera_focus_pre_proc(cmr_handle oem_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct setting_cmd_parameter    setting_param;
	cmr_int			need_pre_flash = 1;
#ifdef CONFIG_SPRD_PRIVATE_ZSL
	ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, SETTING_GET_SPRD_ZSL_ENABLED, &setting_param);
	if (ret) {
		CMR_LOGE("failed to get preview sprd zsl enabled flag %ld", ret);
	}
	if (CAMERA_ZSL_MODE == cxt->snp_cxt.snp_mode && 1 == setting_param.cmd_type_value) {
		need_pre_flash = 0;
	}
#endif
	/*open flash*/
	// if (CAMERA_ZSL_MODE != cxt->snp_cxt.snp_mode) {
	if(need_pre_flash) {
		memset(&setting_param, 0, sizeof(setting_param));
		setting_param.camera_id = cxt->camera_id;
		setting_param.ctrl_flash.is_active = 1;
		setting_param.ctrl_flash.work_mode = 0;
		setting_param.ctrl_flash.flash_type = FLASH_OPEN;
		setting_param.ctrl_flash.capture_mode.capture_mode = cxt->snp_cxt.snp_mode;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, SETTING_CTRL_FLASH, &setting_param);
		if (ret) {
			CMR_LOGE("failed to open flash");
		}
	}
	// }

	return ret;
}

cmr_int camera_focus_post_proc(cmr_handle oem_handle, cmr_int will_capture)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct setting_cmd_parameter    setting_param;
	cmr_int                         need_close_flash = 1;

#ifdef CONFIG_SPRD_PRIVATE_ZSL
	ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, SETTING_GET_SPRD_ZSL_ENABLED, &setting_param);
	if (ret) {
		CMR_LOGE("failed to get preview sprd zsl enabled flag %ld", ret);
	}
	if (CAMERA_ZSL_MODE == cxt->snp_cxt.snp_mode && 1 == setting_param.cmd_type_value) {
		need_close_flash = 0;
	}
#endif
	/*close flash*/
	CMR_LOGI("camera_focus_post_proc %ld", will_capture);
	// if (CAMERA_ZSL_MODE != cxt->snp_cxt.snp_mode) {
	if(need_close_flash) {
		memset(&setting_param, 0, sizeof(setting_param));
		setting_param.camera_id = cxt->camera_id;
		setting_param.ctrl_flash.is_active = 0;
		setting_param.ctrl_flash.work_mode = 0;
		if (will_capture) {
			setting_param.ctrl_flash.flash_type = FLASH_CLOSE_AFTER_OPEN;
		} else {
			setting_param.ctrl_flash.flash_type = FLASH_CLOSE;
		}
		setting_param.ctrl_flash.will_capture = will_capture;
		setting_param.ctrl_flash.capture_mode.capture_mode = cxt->snp_cxt.snp_mode;
		prev_set_preview_skip_frame_num(cxt->prev_cxt.preview_handle,cxt->camera_id,CAMERA_FRAME_SKIP_NUM_AFTER_FLASH);
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, SETTING_CTRL_FLASH, &setting_param);
		if (ret) {
			CMR_LOGE("failed to open flash %ld", ret);
		}
	}
	// }
	return ret;
}

cmr_int camera_get_preview_status(cmr_handle oem_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	struct preview_context         *prev_cxt = &cxt->prev_cxt;
	struct snapshot_context        *snp_cxt = &cxt->snp_cxt;
	struct setting_context         *setting_cxt = &cxt->setting_cxt;

	if (1 != prev_cxt->inited) {
		CMR_LOGE("err, don't init preview");
		return ERROR;
	}

	return cmr_preview_get_status(cxt->prev_cxt.preview_handle, cxt->camera_id);
}

cmr_int camera_sensor_init(cmr_handle  oem_handle, cmr_uint is_autotest)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct sensor_context           *sn_cxt = NULL;
	struct sensor_init_param        init_param;
	cmr_handle                      sensor_handle;
	cmr_u32                         camera_id_bits = 0;

	CHECK_HANDLE_VALID(oem_handle);
	sn_cxt = &cxt->sn_cxt;
	CHECK_HANDLE_VALID(sn_cxt);

	if (1 == sn_cxt->inited) {
		CMR_LOGD("sensor has been intialized");
		goto exit;
	}
	cmr_bzero(&init_param, sizeof(init_param));
	init_param.oem_handle = oem_handle;
	init_param.is_autotest = is_autotest;
	ret = cmr_sensor_init(&init_param, &sensor_handle);
	if (ret) {
		CMR_LOGE("failed to init sensor %ld", ret);
		ret = -CMR_CAMERA_NO_SUPPORT;
		goto exit;
	}
	sn_cxt->sensor_handle = sensor_handle;
	sn_cxt->inited = 1;
	if (ret) {
		CMR_LOGE("failed to set auto test mode %ld", ret);
	}
	camera_id_bits = 1 << cxt->camera_id;
	ret = cmr_sensor_open(sensor_handle, camera_id_bits);
	if (ret) {
		CMR_LOGE("open %d sensor failed %ld", cxt->camera_id, ret);
		goto sensor_exit;
	}
	cmr_sensor_event_reg(sensor_handle, cxt->camera_id, camera_sensor_evt_cb);
	cxt->sn_cxt.sensor_handle = sensor_handle;
    goto exit;

sensor_exit:
	cmr_sensor_deinit(sn_cxt->sensor_handle);
	sn_cxt->inited = 0;
exit:
	CMR_LOGD("done %ld", ret);
	return ret;
}

cmr_int camera_sensor_deinit(cmr_handle  oem_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	cmr_handle                      sensor_handle;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct sensor_context           *sn_cxt;

	CHECK_HANDLE_VALID(oem_handle);

	sn_cxt = &cxt->sn_cxt;
	if (0 == sn_cxt->inited) {
		CMR_LOGI("sensor has been de-intialized");
		goto exit;
	}
	sensor_handle = sn_cxt->sensor_handle;
	cmr_sensor_close(sensor_handle, (1 << cxt->camera_id));
    cmr_sensor_deinit(sensor_handle);
	cmr_bzero(sn_cxt,sizeof(*sn_cxt));

exit:
	return ret;
}

cmr_int camera_grab_init(cmr_handle  oem_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct grab_context             *grab_cxt = NULL;
	cmr_handle                      grab_handle;
	struct grab_init_param          grab_param;

	CHECK_HANDLE_VALID(oem_handle);
	grab_cxt = &cxt->grab_cxt;
	CHECK_HANDLE_VALID(grab_cxt);

	if (0 == grab_cxt->inited) {
		grab_param.oem_handle = oem_handle;
		ret = cmr_grab_init(&grab_param, &grab_handle);
		if (ret) {
			CMR_LOGE("failed to init grab %ld", ret);
			ret = -CMR_CAMERA_NO_SUPPORT;
			goto exit;
		}
		cmr_grab_evt_reg(grab_handle, camera_grab_evt_cb);
		cmr_grab_stream_cb(grab_handle, camera_sensor_streamctrl);
		grab_cxt->inited = 1;
		grab_cxt->grab_handle = grab_handle;
	}

exit:
	CMR_LOGD("done %ld", ret);
	return ret;
}

cmr_int camera_grab_deinit(cmr_handle  oem_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct grab_context             *grab_cxt = NULL;

	CHECK_HANDLE_VALID(oem_handle);
	grab_cxt = &cxt->grab_cxt;
	if (0 == grab_cxt->inited) {
		CMR_LOGD("GRAB has been de-intialized");
		goto exit;
	}

	ret = cmr_grab_deinit(grab_cxt->grab_handle);
	if (ret) {
		CMR_LOGE("failed to de-init grab %ld", ret);
		goto exit;
	}
	cmr_bzero(grab_cxt, sizeof(*grab_cxt));
exit:
	CMR_LOGD("done %ld", ret);
	return ret;
}

cmr_int camera_jpeg_init(cmr_handle  oem_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct jpeg_context             *jpeg_cxt = NULL;

	CHECK_HANDLE_VALID(oem_handle);
	jpeg_cxt = &cxt->jpeg_cxt;
	CHECK_HANDLE_VALID(jpeg_cxt);

	if (0 == jpeg_cxt->inited) {
		ret = jpeg_init(oem_handle, &jpeg_cxt->jpeg_handle);
		if (CMR_CAMERA_SUCCESS == ret) {
			jpeg_evt_reg(jpeg_cxt->jpeg_handle, camera_jpeg_evt_cb);
			jpeg_cxt->inited = 1;
		} else {
			CMR_LOGE("failed to init jpeg codec %ld", ret);
			ret = -CMR_CAMERA_NO_SUPPORT;
		}
	}
	CMR_LOGD("done %ld", ret);
	return ret;
}

cmr_int camera_jpeg_deinit(cmr_handle  oem_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct jpeg_context             *jpeg_cxt;

	CHECK_HANDLE_VALID(oem_handle);
	jpeg_cxt = &cxt->jpeg_cxt;
	CHECK_HANDLE_VALID(jpeg_cxt);

	if (0 == jpeg_cxt->inited) {
		CMR_LOGD("jpeg codec has been de-intialized");
		goto exit;
	}

	ret = jpeg_deinit(jpeg_cxt->jpeg_handle);
	if (ret) {
		CMR_LOGE("failed to de-init jpeg codec %ld", ret);
		goto exit;
	}
	cmr_bzero(jpeg_cxt, sizeof(*jpeg_cxt));

exit:
	CMR_LOGD("done %ld", ret);
	return ret;
}

cmr_int camera_scaler_init(cmr_handle  oem_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct scaler_context           *scaler_cxt = NULL;

	CHECK_HANDLE_VALID(oem_handle);
	scaler_cxt = &cxt->scaler_cxt;
	CHECK_HANDLE_VALID(scaler_cxt);

	if (1 == scaler_cxt->inited) {
		CMR_LOGD("scaler has been intialized");
		goto exit;
	}

	ret = cmr_scale_open(&scaler_cxt->scaler_handle);
	if (ret) {
		CMR_LOGE("failed to init scaler %ld", ret);
		ret = -CMR_CAMERA_NO_SUPPORT;
	} else {
		scaler_cxt->inited = 1;
	}

exit:
	CMR_LOGD("done %ld", ret);
	return ret;
}

cmr_int camera_scaler_deinit(cmr_handle  oem_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct scaler_context           *scaler_cxt;

	CHECK_HANDLE_VALID(oem_handle);
	scaler_cxt = &cxt->scaler_cxt;
	CHECK_HANDLE_VALID(scaler_cxt);
	if (0 == scaler_cxt->inited) {
		CMR_LOGD("scaler has been de-intialized");
		goto exit;
	}

	ret = cmr_scale_close(scaler_cxt->scaler_handle);
	if (ret) {
		CMR_LOGE("failed to de-init scaler %ld", ret);
		goto exit;
	}
	cmr_bzero(scaler_cxt, sizeof(*scaler_cxt));
exit:
	CMR_LOGD("done %ld", ret);
	return ret;
}

cmr_int camera_rotation_init(cmr_handle  oem_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct rotation_context         *rot_cxt = NULL;
	cmr_handle                      rot_handle;

	CHECK_HANDLE_VALID(oem_handle);
	rot_cxt = &cxt->rot_cxt;
	CHECK_HANDLE_VALID(rot_cxt);

	if (1 == rot_cxt->inited) {
		CMR_LOGD("rot has been intialized");
		goto exit;
	}

	ret = cmr_rot_open(&rot_handle);
	if (ret) {
		CMR_LOGE("failed to init rot %ld", ret);
		ret = -CMR_CAMERA_NO_SUPPORT;
	} else {
		rot_cxt->rotation_handle = rot_handle;
		rot_cxt->inited = 1;
	}

exit:
	CMR_LOGD("done %ld", ret);
	return ret;
}

cmr_int camera_rotation_deinit(cmr_handle  oem_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct rotation_context         *rot_cxt;

	CHECK_HANDLE_VALID(oem_handle);
	rot_cxt = &cxt->rot_cxt;
	CHECK_HANDLE_VALID(rot_cxt);

	if (0 == rot_cxt->inited) {
		CMR_LOGD("rot has been de-intialized");
		goto exit;
	}

	ret = cmr_rot_close(rot_cxt->rotation_handle);
	if (ret) {
		CMR_LOGE("failed to de-init rot %ld", ret);
		goto exit;
	}
	cmr_bzero(rot_cxt, sizeof(*rot_cxt));

exit:
	CMR_LOGD("done %ld", ret);
	return ret;
}

void camera_set_reload_support(uint32_t is_support)
{
	CMR_LOGI("%d.",is_support);
	is_support_reload = is_support;
}

void camera_calibrationconfigure_save (uint32_t start_addr, uint32_t data_size)
{
	const char configfile[] = "/data/otpconfig.bin";

	FILE *configfile_handle = fopen(configfile, "wb");
	if (NULL == configfile_handle) {
		CMR_LOGE("failed");
		return;
	}
	fwrite(&start_addr, 1, 4, configfile_handle);
	fwrite(&data_size, 1, 4, configfile_handle);
	fclose(configfile_handle);
	CMR_LOGI("done");
}

void camera_calibrationconfigure_load (uint32_t *start_addr, uint32_t *data_size)
{
	const char configfile[] = "/data/otpconfig.bin";

	FILE *configfile_handle = fopen(configfile, "rb");
	if (NULL == configfile_handle) {
		CMR_LOGE("failed");
		return;
	}
	fread(&start_addr, 1, 4, configfile_handle);
	fread(&data_size, 1, 4, configfile_handle);
	fclose(configfile_handle);
	CMR_LOGI("done");
}

#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
#define CMR_ISP_OTP_MAX_SIZE (256)
static int camera_get_reloadinfo(cmr_handle  oem_handle, struct isp_cali_param *cali_param, struct isp_data_info* cali_result)
{
	int32_t                        rtn = 0;
	struct camera_context          *cxt     = (struct camera_context*)oem_handle;
	struct sensor_exp_info         *sensor_info_ptr;
	int32_t                        ret = 0;
	uint32_t                       otp_start_addr = 0;
	uint32_t                       otp_data_len = 0;
	SENSOR_VAL_T                   val;
	SENSOR_OTP_PARAM_T             param_ptr;
	const char                     golden_file[] = "/data/golden.bin";
	const char                     random_lsc_file[] = "/data/random_lsc.bin";
	const char                     random_awb_file[] = "/data/random_awb.bin";
	const char                     calibration_file[] = "/data/calibration_phone.bin";
	struct stat                    info;
	struct isp_data_t              lsc_otp;
	struct isp_data_t              awb_otp;
	struct isp_data_t              golden;
	struct isp_data_t              target_buf;
	struct isp_data_t              sensor_otp;
	struct isp_data_t              checksum_otp;
	struct isp_data_t              fw_version;
	struct isp_cali_info_t           cali_info;
	FILE                           *golden_handle = NULL;
	FILE                           *lsc_otp_handle = NULL;
	FILE                           *awb_otp_handle = NULL;
	FILE                           *calibration_handle = NULL;
	uint32_t                       device_flag = 0;
	uint32_t                       is_reload = 1;
	uint32_t                       is_need_checksum = 0;
	void                           *checksum_ptr = NULL;

	if (NULL == cali_param || NULL == cali_result) {
		CMR_LOGE("param error cali_param, cali_result, %p %p", cali_param, cali_result);
		return -CMR_CAMERA_INVALID_PARAM;
	}

#ifndef MINICAMERA
	if (CAMERA_ID_0 == cxt->camera_id) {
		camera_set_reload_support(1);
	} else {
		camera_set_reload_support(0);
	}
	device_flag = 1; //handset
#endif

	if (is_support_reload) {
		CMR_LOGI("support reload");

		cmr_bzero(&lsc_otp, sizeof(lsc_otp));
		cmr_bzero(&awb_otp, sizeof(awb_otp));
		cmr_bzero(&golden, sizeof(golden));
		cmr_bzero(&target_buf, sizeof(target_buf));
		cmr_bzero(&sensor_otp, sizeof(sensor_otp));
		cmr_bzero(&checksum_otp, sizeof(checksum_otp));
		cmr_bzero(&fw_version, sizeof(fw_version));
		cmr_bzero(&cali_info, sizeof(cali_info));
		cmr_bzero(&param_ptr, sizeof(param_ptr));
#if 1
#if 0
		//read FW version
		fw_version.size = CMR_ISP_OTP_MAX_SIZE;
		fw_version.data_ptr = malloc(fw_version.size);
		if (NULL == fw_version.data_ptr) {
			CMR_LOGE("malloc fw_version buffer failed");
			goto EXIT;
		}

		cmr_bzero(&param_ptr, sizeof(param_ptr));
		param_ptr.start_addr   = 0;
		param_ptr.len          = fw_version.size;
		param_ptr.buff         = fw_version.data_ptr;
		param_ptr.type         = SENSOR_OTP_PARAM_FW_VERSION;
		val.type               = SENSOR_VAL_TYPE_READ_OTP;
		val.pval               = &param_ptr;
		ret = cmr_sensor_ioctl(cxt->sn_cxt.sensor_handle, cxt->camera_id, SENSOR_ACCESS_VAL, (cmr_uint)&val);
		if (ret || 0 == param_ptr.len) {
			CMR_LOGE("read otp data fw version failed");
			goto EXIT;
		}
		CMR_LOGI(" fw version %s size %d", param_ptr.buff, param_ptr.len);
#endif
		//checksum
		checksum_otp.size = CMR_ISP_OTP_MAX_SIZE;
		checksum_otp.data_ptr = malloc(checksum_otp.size);
		if (NULL == checksum_otp.data_ptr) {
			CMR_LOGE("malloc checksum_otp buffer failed");
			goto EXIT;
		}

		cmr_bzero(&param_ptr, sizeof(param_ptr));
		param_ptr.start_addr   = 0;
		param_ptr.len          = checksum_otp.size;
		param_ptr.buff         = checksum_otp.data_ptr;
		param_ptr.type         = SENSOR_OTP_PARAM_CHECKSUM;
		val.type               = SENSOR_VAL_TYPE_READ_OTP;
		val.pval               = &param_ptr;
		ret = cmr_sensor_ioctl(cxt->sn_cxt.sensor_handle, cxt->camera_id, SENSOR_ACCESS_VAL, (cmr_uint)&val);
		if (ret || 0 == param_ptr.len) {
			CMR_LOGE("read otp data checksum failed %d,%d", ret, param_ptr.len);
			goto EXIT;
		}
		if (param_ptr.len != checksum_otp.size) {
			is_need_checksum = 1;
			checksum_otp.size = param_ptr.len;
		}
		CMR_LOGI(" checksum %s size %d", param_ptr.buff, param_ptr.len);
#endif

#if (CONFIG_READOTP_METHOD == 0)
		if (stat(calibration_file, &info) == 0 && device_flag) {
			CMR_LOGI(" %s is already exist!", calibration_file);

			calibration_handle = fopen(calibration_file, "rb");
			if (NULL == calibration_handle) {
				CMR_LOGE("open calibration file failed");
				goto EXIT;
			}
#endif
			//get the target buffer size
			rtn = isp_calibration_get_info(&golden, &cali_info);
			if (0 != rtn) {
				CMR_LOGE("isp_calibration_get_info failed");
				goto EXIT;
			}
			CMR_LOGI("get calibration info: %d", cali_info.size);
			cali_result->size = cali_info.size;
			cali_result->data_ptr = malloc(cali_result->size);
			if (NULL == cali_result->data_ptr) {
				CMR_LOGE("malloc target buffer failed");
				goto EXIT;
			}
#if (CONFIG_READOTP_METHOD == 0)
			fread(cali_result->data_ptr, 1, cali_result->size, calibration_handle);
			fclose(calibration_handle);

			if (is_need_checksum) {
				checksum_ptr = (void *)((char *)cali_result->data_ptr + cali_result->size - checksum_otp.size);
				if (0 == strcmp((void *)checksum_ptr, (const char *)checksum_otp.data_ptr)) {
					CMR_LOGI("checksum ok");
					is_reload = 0;
				} else {
					CMR_LOGI("checksum different need to reload");
				}
			} else {
				is_reload = 0;
			}
		}
#else
		is_reload = 1;
#endif
		if (is_reload) {
			CMR_LOGI(" %s will creat now!", calibration_file);
#if 0
			//read golden data
			golden_handle = fopen(golden_file, "rb");
			if (NULL == golden_handle) {
				CMR_LOGE("open golden file failed");
				goto EXIT;
			}
			fseek(golden_handle,0,SEEK_END);
			golden.size = ftell(golden_handle);
			fseek(golden_handle,0,SEEK_SET);
			golden.data_ptr = malloc(golden.size);
			if (NULL == golden.data_ptr){
				CMR_LOGE("malloc golden memory failed");
				goto EXIT;
			}
			CMR_LOGI("golden file size=%d, buf=%p", golden.size, golden.data_ptr);
			if (golden.size != fread(golden.data_ptr, 1, golden.size, golden_handle)){
				CMR_LOGE("read golden file failed");
				goto EXIT;
			}
#else
			cmr_bzero(&param_ptr, sizeof(param_ptr));
			val.type               = SENSOR_VAL_TYPE_GET_GOLDEN_DATA;
			val.pval               = &param_ptr;
			ret = cmr_sensor_ioctl(cxt->sn_cxt.sensor_handle, cxt->camera_id, SENSOR_ACCESS_VAL, (cmr_uint)&val);
			if (ret || 0 == param_ptr.golden.size) {
				CMR_LOGE("get golden data failed");
				goto EXIT;
			}
			golden.data_ptr = param_ptr.golden.data_ptr;
			golden.size = param_ptr.golden.size;
#endif
#if 0
			//read otp lsc data
			lsc_otp_handle = fopen(random_lsc_file, "rb");
			if (NULL == lsc_otp_handle) {
				CMR_LOGE("open random lsc file failed");
				goto EXIT;
			}
			fseek(lsc_otp_handle,0,SEEK_END);
			lsc_otp.size = ftell(lsc_otp_handle);
			fseek(lsc_otp_handle,0,SEEK_SET);
			lsc_otp.data_ptr = malloc(lsc_otp.size);
			if (NULL == lsc_otp.data_ptr) {
				CMR_LOGE("malloc random lsc file failed");
				goto EXIT;
			}
			CMR_LOGI("random lsc file size=%d, buf=%p", lsc_otp.size, lsc_otp.data_ptr);
			if (lsc_otp.size != fread(lsc_otp.data_ptr, 1, lsc_otp.size, lsc_otp_handle)) {
				CMR_LOGE("read random lsc file failed");
				goto EXIT;
			}

			//read otp awb data
			awb_otp_handle = fopen(random_awb_file, "rb");
			if (NULL == awb_otp_handle) {
				CMR_LOGE("open random awb file failed");
				goto EXIT;
			}
			fseek(awb_otp_handle,0,SEEK_END);
			awb_otp.size = ftell(awb_otp_handle);
			fseek(awb_otp_handle,0,SEEK_SET);
			awb_otp.data_ptr = malloc(awb_otp.size);
			if (NULL == awb_otp.data_ptr) {
				CMR_LOGE("malloc random awb file failed");
				goto EXIT;
			}
			CMR_LOGI("random awb file size=%d, buf=%p", awb_otp.size, awb_otp.data_ptr);
			if (awb_otp.size != fread(awb_otp.data_ptr, 1, awb_otp.size, awb_otp_handle)) {
				CMR_LOGE("read random awb file failed");
				goto EXIT;
			}
#else
			//read otp lsc data
			lsc_otp.size = CMR_ISP_OTP_MAX_SIZE;
			lsc_otp.data_ptr = malloc(lsc_otp.size);
			if (NULL == lsc_otp.data_ptr) {
				CMR_LOGE("malloc random lsc file failed");
				goto EXIT;
			}

			//read otp awb data
			awb_otp.size = CMR_ISP_OTP_MAX_SIZE;
			awb_otp.data_ptr = malloc(awb_otp.size);
			if (NULL == awb_otp.data_ptr) {
				CMR_LOGE("malloc random awb file failed");
				goto EXIT;
			}
			CMR_LOGI("random awb file size=%d, buf=%p", awb_otp.size, awb_otp.data_ptr);

			/*read lsc, awb from real otp */
			//camera_calibrationconfigure_load(&otp_start_addr, &otp_data_len);
			otp_start_addr   = 0;
			otp_data_len     = CMR_ISP_OTP_MAX_SIZE;
			sensor_otp.data_ptr = malloc(otp_data_len);
			if (NULL == sensor_otp.data_ptr){
				CMR_LOGE("malloc random lsc file failed");
				goto EXIT;
			}

			cmr_bzero(&param_ptr, sizeof(param_ptr));
			param_ptr.start_addr   = otp_start_addr;
			param_ptr.len          = 0;
			param_ptr.buff         = sensor_otp.data_ptr;
			param_ptr.awb.size     = awb_otp.size;
			param_ptr.awb.data_ptr = awb_otp.data_ptr;
			param_ptr.lsc.size     = lsc_otp.size;
			param_ptr.lsc.data_ptr = lsc_otp.data_ptr;
			param_ptr.type         = SENSOR_OTP_PARAM_NORMAL;
			val.type               = SENSOR_VAL_TYPE_READ_OTP;
			val.pval               = &param_ptr;
			CMR_PRINT_TIME;
			ret = cmr_sensor_ioctl(cxt->sn_cxt.sensor_handle, cxt->camera_id, SENSOR_ACCESS_VAL, (cmr_uint)&val);
			if (ret || 0 == param_ptr.len) {
				CMR_LOGE("read otp data failed %d,%d", ret, param_ptr.len);
				goto EXIT;
			}
			CMR_PRINT_TIME;
#endif
			//get the target buffer size
			rtn = isp_calibration_get_info(&golden, &cali_info);
			if (0 != rtn) {
				CMR_LOGE("isp_calibration_get_info failed");
				goto EXIT;
			}
			CMR_LOGI("get calibration info: %d", cali_info.size);
			target_buf.size = cali_info.size;
			target_buf.data_ptr = malloc(target_buf.size);
			if (NULL == target_buf.data_ptr) {
				CMR_LOGE("malloc target buffer failed");
				goto EXIT;
			}

			//get the calibration data, the real size of data will be write to cali_result.size
			cali_param->golden = golden;
			cali_param->awb_otp = awb_otp;
			cali_param->lsc_otp = lsc_otp;
			cali_param->target_buf = target_buf;
			rtn = isp_calibration(cali_param, (struct isp_data_t *)cali_result);
			if (0 != rtn) {
				CMR_LOGE("isp_calibration failed rtn %d", rtn);
				goto EXIT;
			}
			CMR_LOGI("calibration data: addr=%p, size = %d", cali_result->data_ptr, cali_result->size);

#if (CONFIG_READOTP_METHOD == 0)

			//TODO: save the calibration data
			calibration_handle = fopen(calibration_file, "wb");
			if (NULL == calibration_handle) {
				CMR_LOGE("open calibration file failed");
				goto EXIT;
			}
			memcpy((void*)((char*)cali_result->data_ptr + target_buf.size - checksum_otp.size), (void*)checksum_otp.data_ptr, checksum_otp.size);
			fwrite(cali_result->data_ptr, 1, target_buf.size, calibration_handle);
#endif
	EXIT:

#if (CONFIG_READOTP_METHOD == 0)

			if (NULL != golden_handle) {
				fclose(golden_handle);
				golden_handle = NULL;
			}
			if (NULL != lsc_otp_handle) {
				fclose(lsc_otp_handle);
				lsc_otp_handle = NULL;
			}
			if (NULL != awb_otp_handle) {
				fclose(awb_otp_handle);
				awb_otp_handle = NULL;
			}
			if (NULL != calibration_handle) {
				fclose(calibration_handle);
				calibration_handle = NULL;
			}
#endif
			if (NULL != checksum_otp.data_ptr) {
				free(checksum_otp.data_ptr);
				checksum_otp.data_ptr = NULL;
			}
			if (NULL != fw_version.data_ptr) {
				free(fw_version.data_ptr);
				fw_version.data_ptr = NULL;
			}
			if (NULL != lsc_otp.data_ptr) {
				free(lsc_otp.data_ptr);
				golden.data_ptr = NULL;
			}
			if (NULL != awb_otp.data_ptr) {
				free(awb_otp.data_ptr);
				awb_otp.data_ptr = NULL;
			}
			if (NULL != sensor_otp.data_ptr) {
				free(sensor_otp.data_ptr);
				sensor_otp.data_ptr = NULL;
			}
			return rtn;
		}
	}else {
		CMR_LOGI("not support reload");
	}

	return rtn;
}
#endif
cmr_int camera_isp_init(cmr_handle  oem_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct isp_context              *isp_cxt = NULL;
	struct sensor_context           *sn_cxt = NULL;
	struct sensor_exp_info          *sensor_info_ptr;
	struct isp_init_param           isp_param;
	struct isp_video_limit          isp_limit;
	SENSOR_VAL_T                   val;

#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
	struct isp_cali_param cali_param;
	struct isp_data_info cali_result;

	cmr_bzero(&cali_param, sizeof(cali_param));
	cmr_bzero(&cali_result, sizeof(cali_result));
#endif

	CMR_PRINT_TIME;
	CHECK_HANDLE_VALID(oem_handle);
	isp_cxt = &cxt->isp_cxt;
	CHECK_HANDLE_VALID(isp_cxt);
	sn_cxt = &cxt->sn_cxt;
	CHECK_HANDLE_VALID(sn_cxt);

	if (1 == isp_cxt->inited) {
		CMR_LOGD("isp has been intialized");
		goto exit;
	}

	ret = cmr_sensor_get_info(sn_cxt->sensor_handle, cxt->camera_id, &sn_cxt->sensor_info);
	if (ret) {
		CMR_LOGE("fail to get sensor info ret %ld", ret);
		goto exit;
	}

	if (IMG_DATA_TYPE_RAW != sn_cxt->sensor_info.image_format) {
		CMR_LOGD("no need to init isp %d ", sn_cxt->sensor_info.image_format);
		goto exit;
	}

#if !(defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4))
	#ifdef CONFIG_SP7731GEA_BOARD
	isp_param.isp_id = ISP_ID_SC8830;
	#elif defined(CONFIG_SP9630EA_BOARD)
	isp_param.isp_id = ISP_ID_SC9630;
	#endif
#endif

	sensor_info_ptr = &sn_cxt->sensor_info;
	CHECK_HANDLE_VALID(sensor_info_ptr);
	isp_param.setting_param_ptr = sensor_info_ptr->raw_info_ptr;
	if (0 != sensor_info_ptr->mode_info[SENSOR_MODE_COMMON_INIT].width) {
		isp_param.size.w = sensor_info_ptr->mode_info[SENSOR_MODE_COMMON_INIT].width;
		isp_param.size.h = sensor_info_ptr->mode_info[SENSOR_MODE_COMMON_INIT].height;
	} else {
		isp_param.size.w = sensor_info_ptr->mode_info[SENSOR_MODE_PREVIEW_ONE].width;
		isp_param.size.h = sensor_info_ptr->mode_info[SENSOR_MODE_PREVIEW_ONE].height;
	}
	isp_param.ctrl_callback = camera_isp_evt_cb;
	isp_param.oem_handle = oem_handle;
	isp_param.camera_id = cxt->camera_id;
	CMR_LOGD("w %d h %d", isp_param.size.w,isp_param.size.h);
	CMR_PRINT_TIME;
	ret = isp_init(&isp_param, &isp_cxt->isp_handle);
	if (ret) {
		CMR_LOGE("failed to init isp %ld", ret);
		ret = -CMR_CAMERA_NO_SUPPORT;
		goto exit;
	}
	ret = isp_capability(isp_cxt->isp_handle, ISP_VIDEO_SIZE, &isp_limit);
	if (ret) {
		CMR_LOGE("failed to get the limitation of isp %ld", ret);
		ret = -CMR_CAMERA_NO_SUPPORT;
		isp_deinit(isp_cxt->isp_handle);
	} else {
		isp_cxt->width_limit = isp_limit.width;
		isp_cxt->inited = 1;
	}

exit:
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
	if (NULL != cali_result.data_ptr) {
		free(cali_result.data_ptr);
		cali_result.data_ptr = NULL;
	}
#endif

	sem_post(&cxt->isp_sync_sm);

	CMR_LOGD("done %ld", ret);
	return ret;
}

cmr_int camera_isp_deinit_notice(cmr_handle  oem_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;

	ret = cmr_setting_deinit_notice(cxt->setting_cxt.setting_handle);
	ret = cmr_focus_deinit_notice(cxt->focus_cxt.focus_handle);

	return ret;
}

cmr_int camera_isp_deinit(cmr_handle  oem_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct isp_context              *isp_cxt;

	CHECK_HANDLE_VALID(oem_handle);
	isp_cxt = &cxt->isp_cxt;
	CHECK_HANDLE_VALID(isp_cxt);

	if (0 == isp_cxt->inited) {
		CMR_LOGD("isp has been de-intialized");
		goto exit;
	}

	ret = isp_deinit(isp_cxt->isp_handle);
	if (ret) {
		CMR_LOGE("failed to de-init isp %ld", ret);
		goto exit;
	}
	cmr_bzero(isp_cxt, sizeof(*isp_cxt));

#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
	if(cxt->isp_malloc_flag == 1) {
		if (cxt->hal_free) {
			cxt->hal_free(CAMERA_ISP_LSC,&cxt->isp_lsc_phys_addr,&cxt->isp_lsc_virt_addr,
				ISP_LSC_BUF_NUM, cxt->client_data);
		}
		cxt->isp_malloc_flag = 0;
	}

	if(cxt->isp_b4awb_flag == 1) {
		if (cxt->hal_free) {
			cxt->hal_free(CAMERA_ISP_BINGING4AWB,cxt->b4awb_phys_addr,cxt->b4awb_virt_addr,
				ISP_B4AWB_BUF_CNT, cxt->client_data);
		}
	cxt->isp_b4awb_flag = 0;
	}
#endif

exit:
	CMR_LOGD("done %ld", ret);
	return ret;
}

cmr_int camera_preview_init(cmr_handle  oem_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct preview_context          *prev_cxt = NULL;
    struct preview_init_param       init_param;

	CHECK_HANDLE_VALID(oem_handle);
	prev_cxt = &cxt->prev_cxt;
	CHECK_HANDLE_VALID(prev_cxt);

	if (1 == prev_cxt->inited) {
		CMR_LOGD("preview has been intialized");
		goto exit;
	}
	init_param.oem_handle = oem_handle;
	init_param.ipm_handle = cxt->ipm_cxt.ipm_handle;
	init_param.ops.channel_cfg = camera_channel_cfg;
	init_param.ops.pre_channel_cfg = camera_pre_channel_cfg;
	init_param.ops.channel_start = camera_channel_start;
	init_param.ops.channel_pause = camera_channel_pause;
	init_param.ops.channel_resume = camera_channel_resume;
	init_param.ops.channel_free_frame = camera_channel_free_frame;
	init_param.ops.channel_stop = camera_channel_stop;
	init_param.ops.pre_channel_stop = camera_pre_channel_stop;
	init_param.ops.channel_buff_cfg = camera_channel_buff_cfg;
	init_param.ops.isp_start_video = camera_isp_start_video;
	init_param.ops.isp_stop_video = camera_isp_stop_video;
	init_param.ops.start_rot = camera_start_rot;
	init_param.ops.preview_pre_proc = camera_preview_pre_proc;
	init_param.ops.preview_post_proc = camera_preview_post_proc;
	init_param.ops.get_sensor_info = camera_get_sensor_info;
	init_param.ops.get_sensor_autotest_mode = camera_get_sensor_autotest_mode;
	init_param.ops.channel_scale_capability = camera_channel_scale_capability;
	init_param.ops.channel_path_capability = camera_channel_path_capability;
	init_param.ops.channel_get_cap_time = camera_channel_get_cap_time;
	init_param.ops.capture_pre_proc = camera_capture_pre_proc;
	init_param.ops.capture_post_proc = camera_capture_post_proc;
	init_param.ops.sensor_open = camera_open_sensor;
	init_param.ops.sensor_close = camera_close_sensor;
	init_param.oem_cb = camera_preview_cb;
	init_param.private_data = NULL;
	init_param.sensor_bits = (1 << cxt->camera_id);
	ret = cmr_preview_init(&init_param, &prev_cxt->preview_handle);
	if (ret) {
		CMR_LOGE("failed to init preview,ret %ld", ret);
		ret = -CMR_CAMERA_NO_SUPPORT;
		goto exit;
	}
	prev_cxt->inited = 1;
exit:
	CMR_LOGV("done %ld", ret);
	return ret;
}

cmr_int camera_preview_deinit(cmr_handle  oem_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct preview_context          *prev_cxt;

	CHECK_HANDLE_VALID(oem_handle);
	prev_cxt = &cxt->prev_cxt;
	CHECK_HANDLE_VALID(prev_cxt);

	if (0 == prev_cxt->inited) {
		CMR_LOGD("preview has been de-intialized");
		goto exit;
	}
	ret = cmr_preview_deinit(prev_cxt->preview_handle);
	if (ret) {
		CMR_LOGE("failed to de-init preview %ld", ret);
		goto exit;
	}
	cmr_bzero(prev_cxt, sizeof(*prev_cxt));
exit:
	CMR_LOGD("done %ld", ret);
	return ret;
}

cmr_int camera_snapshot_init(cmr_handle  oem_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct snapshot_context         *snp_cxt = NULL;
	struct snapshot_init_param      init_param;

	CHECK_HANDLE_VALID(oem_handle);
	snp_cxt = &cxt->snp_cxt;
	CHECK_HANDLE_VALID(snp_cxt);

	if (1 == snp_cxt->inited) {
		CMR_LOGD("snp has been intialized");
		goto exit;
	}
	init_param.id = cxt->camera_id;
	init_param.oem_handle = oem_handle;
	init_param.ipm_handle = cxt->ipm_cxt.ipm_handle;
	init_param.oem_cb = camera_snapshot_cb;
	init_param.ops.start_encode = camera_start_encode;
	init_param.ops.start_decode = camera_start_decode;
	init_param.ops.start_exif_encode = camera_start_exif_encode;
	init_param.ops.start_scale = camera_start_scale;
	init_param.ops.start_rot = camera_start_rot;
	init_param.ops.capture_pre_proc = camera_capture_pre_proc;
	init_param.ops.capture_post_proc = camera_capture_post_proc;
	init_param.ops.raw_proc = camera_raw_proc;
	init_param.ops.isp_start_video = camera_isp_start_video;
	init_param.ops.isp_stop_video = camera_isp_stop_video;
	init_param.ops.channel_start = camera_channel_start;
	init_param.ops.channel_pause = camera_channel_pause;
	init_param.ops.channel_resume = camera_channel_resume;
	init_param.ops.channel_free_frame = camera_channel_free_frame;
	init_param.ops.channel_stop = camera_channel_stop;
	init_param.ops.channel_buff_cfg = camera_channel_buff_cfg;
	init_param.ops.get_sensor_info = camera_get_sensor_info;
	init_param.ops.stop_codec = camera_stop_codec;
	init_param.ops.capture_close_flash = camera_close_flash;
	init_param.private_data = NULL;
	ret = cmr_snapshot_init(&init_param, &snp_cxt->snapshot_handle);
	if (ret) {
		CMR_LOGE("failed to init snapshot,ret %ld", ret);
		ret = -CMR_CAMERA_NO_SUPPORT;
		goto exit;
	}
	snp_cxt->inited = 1;
exit:
	CMR_LOGD("done %ld", ret);
	return ret;
}

cmr_int camera_snapshot_deinit(cmr_handle  oem_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct snapshot_context         *snp_cxt;

	CHECK_HANDLE_VALID(oem_handle);
	snp_cxt = &cxt->snp_cxt;
	CHECK_HANDLE_VALID(snp_cxt);

	if (0 == snp_cxt->inited) {
		CMR_LOGD("snp has been de-intialized");
		goto exit;
	}
	ret = cmr_snapshot_deinit(snp_cxt->snapshot_handle);
	if (ret) {
		CMR_LOGE("failed to de-init snapshot %ld", ret);
		goto exit;
	}
	cmr_bzero(snp_cxt, sizeof(*snp_cxt));
exit:
	CMR_LOGD("done %ld", ret);
	return ret;
}

cmr_int camera_ipm_init(cmr_handle  oem_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct ipm_context              *ipm_cxt = NULL;
	struct ipm_init_in              init_param;

	CHECK_HANDLE_VALID(oem_handle);
	ipm_cxt = &cxt->ipm_cxt;
	CHECK_HANDLE_VALID(ipm_cxt);

	if (1 == ipm_cxt->inited) {
		CMR_LOGD("ipm has been intialized");
		goto exit;
	}

	init_param.oem_handle = oem_handle;
	init_param.sensor_id = cxt->camera_id;
	init_param.get_sensor_info = camera_get_sensor_info;
	init_param.ipm_sensor_ioctl = camera_sensor_ioctl;
	init_param.ipm_isp_ioctl = camera_isp_ioctl;
	ret = cmr_ipm_init(&init_param, &ipm_cxt->ipm_handle);
	if (ret) {
		CMR_LOGE("failed to init ipm,ret %ld", ret);
		ret = -CMR_CAMERA_NO_SUPPORT;
		goto exit;
	}
	ipm_cxt->inited = 1;

exit:
	CMR_LOGD("done %ld", ret);
	return ret;
}

cmr_int camera_ipm_deinit(cmr_handle  oem_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct ipm_context              *ipm_cxt;

	CHECK_HANDLE_VALID(oem_handle);
	ipm_cxt = &cxt->ipm_cxt;
	CHECK_HANDLE_VALID(ipm_cxt);

	if (0 == ipm_cxt->inited) {
		CMR_LOGD("ipm has been de-intialized");
		goto exit;
	}
	ret = cmr_ipm_deinit(ipm_cxt->ipm_handle);
	if (ret) {
		CMR_LOGE("failed to de-init ipm %ld", ret);
		goto exit;
	}
	cmr_bzero(ipm_cxt, sizeof(*ipm_cxt));
exit:
	CMR_LOGD("done %ld", ret);
	return ret;
}

cmr_int camera_setting_init(cmr_handle  oem_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct setting_context          *setting_cxt = NULL;
	struct setting_init_in          init_param;

	CHECK_HANDLE_VALID(oem_handle);
	setting_cxt = &cxt->setting_cxt;
	CHECK_HANDLE_VALID(setting_cxt);

	if (1 == setting_cxt->inited) {
		CMR_LOGD("setting has been de-intialized");
		goto exit;
	}
	init_param.oem_handle = oem_handle;
	init_param.camera_id_bits = (1 << cxt->camera_id);
	init_param.io_cmd_ioctl = camera_ioctl_for_setting;
	init_param.setting_sn_ioctl = camera_sensor_ioctl;
	init_param.setting_isp_ioctl = camera_isp_ioctl;
	init_param.get_setting_activity = camera_get_setting_activity;
	init_param.before_set_cb = camera_before_set;
	init_param.after_set_cb = camera_after_set;
	init_param.padding=0;
	ret = cmr_setting_init(&init_param, &setting_cxt->setting_handle);
	if (ret) {
		CMR_LOGE("failed to init setting %ld", ret);
		ret = -CMR_CAMERA_NO_SUPPORT;
		goto exit;
	}
	setting_cxt->inited = 1;

exit:
	CMR_LOGD("done %ld", ret);
	return ret;
}

cmr_int camera_setting_deinit(cmr_handle  oem_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct setting_context          *setting_cxt;

	CHECK_HANDLE_VALID(oem_handle);
	setting_cxt = &cxt->setting_cxt;
	CHECK_HANDLE_VALID(setting_cxt);

	if (0 == setting_cxt->inited) {
		CMR_LOGD("setting has been de-intialized");
		goto exit;
	}
	ret = cmr_setting_deinit(setting_cxt->setting_handle);
	if (ret) {
		CMR_LOGE("failed to de-init setting %ld", ret);
		goto exit;
	}
	cmr_bzero(setting_cxt, sizeof(*setting_cxt));
exit:
	CMR_LOGD("done %ld", ret);
	return ret;
}

cmr_int camera_focus_init(cmr_handle  oem_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct focus_context            *focus_cxt;
	struct af_init_param            init_param;

	CHECK_HANDLE_VALID(oem_handle);
	focus_cxt = &cxt->focus_cxt;
	CHECK_HANDLE_VALID(focus_cxt);

	if (1 == focus_cxt->inited) {
		CMR_LOGD("focus has been intialized");
		goto exit;
	}
	init_param.oem_handle             = oem_handle;
	init_param.evt_cb                 = camera_focus_evt_cb;
	init_param.ops.af_pre_proc        = camera_focus_pre_proc;
	init_param.ops.af_post_proc       = camera_focus_post_proc;
	init_param.ops.af_isp_ioctrl      = camera_isp_ioctl;
	init_param.ops.get_preview_status = camera_get_preview_status;
	init_param.ops.af_sensor_ioctrl   = camera_sensor_ioctl;
	init_param.ops.get_sensor_info    = camera_get_sensor_info;
	init_param.ops.get_flash_info = camera_focus_visit_flash_info;
	ret = cmr_focus_init(&init_param, cxt->camera_id, &focus_cxt->focus_handle);
	if (ret) {
		CMR_LOGE("failed to init focus,ret %ld", ret);
		ret = -CMR_CAMERA_NO_SUPPORT;
		goto exit;
	}
	focus_cxt->inited = 1;

exit:
	CMR_LOGD("done %ld", ret);
	return ret;
}

cmr_int camera_focus_deinit(cmr_handle  oem_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct focus_context            *focus_cxt;

	CHECK_HANDLE_VALID(oem_handle);
	focus_cxt = &cxt->focus_cxt;
	CHECK_HANDLE_VALID(focus_cxt);

	if (0 == focus_cxt->inited) {
		CMR_LOGD("focus has been de-intialized");
		goto exit;
	}
	ret = cmr_focus_deinit(focus_cxt->focus_handle);
	if (ret) {
		CMR_LOGE("failed to de-init focus %ld", ret);
		goto exit;
	}
	cmr_bzero(focus_cxt, sizeof(*focus_cxt));

exit:
	CMR_LOGD("done %ld", ret);
	return ret;
}


cmr_int camera_preview_cb_thread_proc(struct cmr_msg *message, void* data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)data;
	camera_cb_of_type              callback;

	if (!message || !data) {
		CMR_LOGE("param error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	CMR_LOGV("msg_type 0x%x, sub msg type 0x%x client_data 0x%lx", message->msg_type, message->sub_msg_type, (cmr_uint)cxt->client_data);
	callback = cxt->camera_cb;
	/*CMR_PRINT_TIME;*/
	if (callback) {
		callback(message->sub_msg_type, cxt->client_data, message->msg_type, message->data);
	} else {
		CMR_LOGE("err, camera cb is null");
		ret = -CMR_CAMERA_INVALID_PARAM;
	}
	/*CMR_PRINT_TIME;*/
exit:
	CMR_LOGV("out ret %ld", ret);
	return ret;
}

cmr_int camera_snapshot_cb_thread_proc(struct cmr_msg *message, void* data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)data;
	camera_cb_of_type              callback;

	if (!message || !data) {
		CMR_LOGE("param error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	CMR_LOGV("msg_type 0x%x, sub msg type 0x%x", message->msg_type, message->sub_msg_type);

	callback = cxt->camera_cb;
	if (callback) {
		callback(message->sub_msg_type, cxt->client_data, message->msg_type, message->data);
	} else {
		CMR_LOGE("err, camera cb is null");
		ret = -CMR_CAMERA_INVALID_PARAM;
	}

exit:
	return ret;
}

cmr_int camera_snapshot_secondary_thread_proc(struct cmr_msg *message, void* data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)data;
	camera_cb_of_type              callback;

	if (!message || !data) {
		CMR_LOGE("param error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	CMR_LOGD("msg_type 0x%x, sub msg type 0x%x", message->msg_type, message->sub_msg_type);
	callback = cxt->camera_cb;
	if (callback) {
		callback(message->sub_msg_type, cxt->client_data, message->msg_type, message->data);
	} else {
		CMR_LOGE("err, camera cb is null");
		ret = -CMR_CAMERA_INVALID_PARAM;
	}
exit:
	return ret;
}

cmr_int camera_snapshot_send_raw_thread_proc(struct cmr_msg *message, void* data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)data;
	camera_cb_of_type              callback;

	if (!message || !data) {
		CMR_LOGE("param error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	CMR_LOGD("msg_type 0x%x, sub msg type 0x%x", message->msg_type, message->sub_msg_type);
	callback = cxt->camera_cb;
	if (callback) {
		callback(message->sub_msg_type, cxt->client_data, message->msg_type, message->data);
	} else {
		CMR_LOGE("err, camera cb is null");
		ret = -CMR_CAMERA_INVALID_PARAM;
	}
exit:
	return ret;
}

cmr_int camera_create_prev_thread(cmr_handle oem_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;

	ret = cmr_thread_create(&cxt->prev_cb_thr_handle, PREVIEW_MSG_QUEUE_SIZE,
							camera_preview_cb_thread_proc,(void*)oem_handle);

	if (CMR_MSG_SUCCESS != ret) {
		CMR_LOGE("create preview thread fail");
		ret = -CMR_CAMERA_NO_SUPPORT;
	}
	CMR_LOGD("done %ld", ret);
	return ret;
}

cmr_int camera_destroy_prev_thread(cmr_handle oem_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;

	if (!oem_handle) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	if (cxt->prev_cb_thr_handle) {
		ret = cmr_thread_destroy(cxt->prev_cb_thr_handle);
		if (!ret) {
			cxt->prev_cb_thr_handle = (cmr_handle)0;
		} else {
			CMR_LOGE("failed to destroy prev thr");
		}
	}

	if (cxt->pre_preview_thread) {
		ret = cmr_thread_destroy(cxt->pre_preview_thread);
		if (!ret) {
				cxt->pre_preview_thread = (cmr_handle)0;
		} else {
				CMR_LOGE("failed to destroy pre prev thr");
		}
	}

exit:
	CMR_LOGD("done %ld", ret);
	return ret;
}

cmr_int camera_create_snp_thread(cmr_handle oem_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;

	ret = cmr_thread_create(&cxt->snp_cb_thr_handle, SNAPSHOT_MSG_QUEUE_SIZE,
							camera_snapshot_cb_thread_proc, (void*)oem_handle);

	if (CMR_MSG_SUCCESS != ret) {
		CMR_LOGE("failed to create snapshot thread %ld", ret);
		ret = -CMR_CAMERA_NO_SUPPORT;
		goto exit;
	}
	ret = cmr_thread_create(&cxt->snp_secondary_thr_handle, SNAPSHOT_MSG_QUEUE_SIZE,
							camera_snapshot_secondary_thread_proc, (void*)oem_handle);

	if (CMR_MSG_SUCCESS != ret) {
		ret = -CMR_CAMERA_NO_SUPPORT;
		goto destroy_cb_thr;
	}

	ret = cmr_thread_create(&cxt->snp_send_raw_image_handle, SNAPSHOT_MSG_QUEUE_SIZE,
							camera_snapshot_send_raw_thread_proc, (void*)oem_handle);

	if (CMR_MSG_SUCCESS != ret) {
		ret = -CMR_CAMERA_NO_SUPPORT;
		goto destroy_secondary_thr;
	} else {
		goto exit;
	}

destroy_secondary_thr:
	cmr_thread_destroy(cxt->snp_secondary_thr_handle);
	cxt->snp_secondary_thr_handle = (cmr_handle)0;
destroy_cb_thr:
	cmr_thread_destroy(cxt->snp_cb_thr_handle);
	cxt->snp_cb_thr_handle = (cmr_handle)0;
exit:
	CMR_LOGD("done %ld", ret);
	return ret;
}

cmr_int camera_destroy_snp_thread(cmr_handle oem_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;

	if (!oem_handle) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	if (cxt->snp_cb_thr_handle) {
		ret = cmr_thread_destroy(cxt->snp_cb_thr_handle);
		if (!ret) {
			cxt->snp_cb_thr_handle = (cmr_handle)0;
		} else {
			CMR_LOGE("failed to destroy snp thr %ld", ret);
		}
	}
	if (cxt->snp_secondary_thr_handle) {
		ret = cmr_thread_destroy(cxt->snp_secondary_thr_handle);
		if (!ret) {
			cxt->snp_secondary_thr_handle = (cmr_handle)0;
		} else {
			CMR_LOGE("failed to destroy snp thr %ld", ret);
		}
	}
	if (cxt->snp_send_raw_image_handle) {
		ret = cmr_thread_destroy(cxt->snp_send_raw_image_handle);
		if (!ret) {
			cxt->snp_send_raw_image_handle = (cmr_handle)0;
		} else {
			CMR_LOGE("failed to destroy snp thr %ld", ret);
		}
	}
exit:
	CMR_LOGD("done %ld", ret);
	return ret;
}

cmr_int camera_init_thread(cmr_handle oem_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;

	/*create preview thread*/
	ret = camera_create_prev_thread(oem_handle);
	if (ret) {
		CMR_LOGE("failed to create preview thread");
		ret = -CMR_CAMERA_NO_SUPPORT;
		goto exit;
	}
	/*create snapshot thread*/
	ret = camera_create_snp_thread(oem_handle);
	if (ret) {
		CMR_LOGE("failed to create snapshot thread");
		ret = -CMR_CAMERA_NO_SUPPORT;
		goto prev_thr_destroy;
	} else {
		goto exit;
	}
prev_thr_destroy:
	camera_destroy_prev_thread(oem_handle);
exit:
	return ret;
}

cmr_int camera_deinit_thread(cmr_handle oem_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;

	camera_destroy_prev_thread(oem_handle);
	camera_destroy_snp_thread(oem_handle);
	CMR_LOGI("done %ld", ret);
	return ret;
}

static cmr_int camera_res_init_internal(cmr_handle oem_handle)
{
	cmr_int                   ret = CMR_CAMERA_SUCCESS;

	CMR_PRINT_TIME;
	ret = camera_ipm_init(oem_handle);
	if (ret) {
		CMR_LOGE("failed to init ipm %ld", ret);
		goto exit;
	}

	ret = camera_setting_init(oem_handle);
	if (ret) {
		CMR_LOGE("failed to init setting %ld", ret);
		goto exit;
	}

	ret = camera_grab_init(oem_handle);
	if (ret) {
		CMR_LOGE("failed to init grab %ld", ret);
		goto exit;
	}

	ret= camera_preview_init(oem_handle);
	if (ret) {
		CMR_LOGE("failed to init preview %ld", ret);
		goto exit;
	}

	ret =  camera_pre_preview_proc(oem_handle);
	if (ret) {
		CMR_LOGE("failed to init pre preview %ld", ret);
		goto exit;
	}

	ret = camera_focus_init(oem_handle);
	if (ret) {
		CMR_LOGE("failed to init focus %ld", ret);
		goto exit;
	}

	ret = camera_jpeg_init(oem_handle);
	if (ret) {
		CMR_LOGE("failed to init jpeg %ld", ret);
		goto exit;
	}

	ret = camera_scaler_init(oem_handle);
	if (ret) {
		CMR_LOGE("failed to init scaler %ld", ret);
		goto exit;
	}

	ret = camera_rotation_init(oem_handle);
	if (ret) {
		CMR_LOGE("failed to init rotation %ld", ret);
		goto exit;
	}

	ret = camera_snapshot_init(oem_handle);
	if (ret) {
		CMR_LOGE("failed to init snapshot %ld", ret);
		goto exit;
	} else {
		CMR_LOGI("init mds ok");
		ret = camera_init_thread(oem_handle);
	}

exit:
	if (ret) {
		camera_res_deinit_internal(oem_handle);
	}

	return ret;
}

static cmr_int camera_res_deinit_internal(cmr_handle oem_handle)
{
	CMR_LOGI("In");
	camera_snapshot_deinit(oem_handle);

	camera_preview_deinit(oem_handle);

	camera_jpeg_deinit(oem_handle);

	camera_focus_deinit(oem_handle);

	camera_setting_deinit(oem_handle);

	camera_grab_deinit(oem_handle);

	camera_rotation_deinit(oem_handle);

	camera_scaler_deinit(oem_handle);

	camera_ipm_deinit(oem_handle);

	camera_deinit_thread(oem_handle);
	CMR_LOGI("Out");
	return CMR_CAMERA_SUCCESS;
}

static cmr_int camera_init_thread_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int                   ret = CMR_CAMERA_SUCCESS;
	cmr_u32                   msg_type = 0;
	cmr_uint                  evt = 0;
	cmr_u32                   camera_id = CAMERA_ID_MAX;
	struct camera_context     *cxt = (struct camera_context*)p_data;

	if (!message || !p_data) {
		return CMR_CAMERA_INVALID_PARAM;
	}

	msg_type = (cmr_u32)message->msg_type;

	switch(msg_type) {
	case CMR_EVT_INIT:
		cxt->err_code = camera_res_init_internal((cmr_handle)cxt);
		if (cxt->err_code) {
			camera_res_deinit_internal((cmr_handle)cxt);
		}
		CMR_LOGI("cb thread inited");
		break;

	case CMR_EVT_WAIT:
		CMR_LOGI("wait here");
		break;

	case CMR_EVT_EXIT:
		camera_res_deinit_internal((cmr_handle)cxt);
		CMR_LOGI("camera exit");
		break;

	default:
		break;
	}

	return ret;
}

static cmr_int camera_pre_preview_thread_proc(struct cmr_msg *message, void *p_data)
{
	cmr_int                   ret = CMR_CAMERA_SUCCESS;
	cmr_u32                   msg_type = 0;
	struct camera_context     *cxt = (struct camera_context*)p_data;

	if (!message || !p_data) {
		return CMR_CAMERA_INVALID_PARAM;
	}

	msg_type = (cmr_u32)message->msg_type;
	CMR_LOGI("pre_preview_thread,msg_type:%d", msg_type);

	switch(msg_type) {
	case CMR_EVT_PRE_PREVIEW:
		sem_wait(&cxt->isp_sync_sm);
		CMR_LOGI("pre CMR_EVT_PRE_PREVIEW");
		camera_pre_preview_start(cxt);
		break;
	case CMR_EVT_PRE_PREVIEW_EXIT:
		CMR_LOGI("pre CMR_EVT_PRE_PREVIEW_EXIT");
		cmr_preview_pre_stop(cxt->prev_cxt.preview_handle, cxt->camera_id, 1);
		break;

	default:
		break;
	}

	return ret;
}

static  cmr_int camera_pre_preview_start(cmr_handle oem_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	struct preview_context         *prev_cxt = &cxt->prev_cxt;
	struct isp_context             *isp_cxt = &cxt->isp_cxt;

	if (1 != prev_cxt->inited) {
		CMR_LOGE("err, don't init preview");
		ret = -CMR_CAMERA_INVALID_STATE;
		goto exit;
	}

	if (1 != isp_cxt->inited) {
		CMR_LOGE("err, don't init isp");
		ret = -CMR_CAMERA_INVALID_STATE;
		goto exit;
	}

	cmr_preview_pre_start(cxt->prev_cxt.preview_handle, cxt->camera_id);


exit:
    CMR_LOGI("pre end");
    return ret;
}

cmr_int camera_res_init(cmr_handle oem_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	CMR_MSG_INIT(message);

	CMR_PRINT_TIME;
#ifdef OEM_HANDLE_HDR
	sem_init(&cxt->hdr_sync_sm, 0, 0);
#endif

#ifdef CAMERA_PATH_SHARE
	sem_init(&cxt->resume_sync_sm, 0, 1);
#endif

	sem_init(&cxt->hdr_flag_sm, 0, 1);
	sem_init(&cxt->share_path_sm, 0, 0);
	sem_init(&cxt->access_sm, 0, 1);
	sem_init(&cxt->isp_sync_sm, 0, 0);

	cxt->err_code = CMR_CAMERA_SUCCESS;
	/*create thread*/
	ret = cmr_thread_create((cmr_handle*)&cxt->init_thread,
				CAMERA_OEM_MSG_QUEUE_SIZE,
				camera_init_thread_proc,
				(void*)cxt);
	if (CMR_MSG_SUCCESS != ret) {
		CMR_LOGE("create thread fail");
	}
	CMR_LOGI("init thread created");
	message.msg_type  = CMR_EVT_INIT;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	ret = cmr_thread_msg_send(cxt->init_thread, &message);
	if (ret) {
		CMR_LOGE("send msg failed!");
		ret = CMR_CAMERA_FAIL;
	}
	CMR_LOGI("async msg sent");
	return ret;
}

static cmr_int camera_res_init_done(cmr_handle oem_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	CMR_MSG_INIT(message);

	message.msg_type  = CMR_EVT_WAIT;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	ret = cmr_thread_msg_send(cxt->init_thread, &message);
	if (ret) {
		CMR_LOGE("send msg failed!");
	}
	ret = cxt->err_code;
	CMR_LOGI("res init-ed");
	return ret;
}
static cmr_int camera_res_deinit(cmr_handle oem_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	CMR_MSG_INIT(message);

	message.msg_type  = CMR_EVT_EXIT;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	ret = cmr_thread_msg_send(cxt->init_thread, &message);
	if (ret) {
		CMR_LOGE("send msg failed!");
	}

	if (cxt->camera_cb) {
		cxt->camera_cb(CAMERA_EXIT_CB_DONE, cxt->client_data, CAMERA_FUNC_STOP, 0);
	} else {
		CMR_LOGE("err, camera_cb is null, don't notify HAL");
	}
	/*destroy thread*/
	if (cxt->init_thread) {
		cmr_thread_destroy(cxt->init_thread);
		cxt->init_thread = 0;
	}
#ifdef OEM_HANDLE_HDR
	sem_destroy(&cxt->hdr_sync_sm);
#endif

#ifdef CAMERA_PATH_SHARE
	sem_destroy(&cxt->resume_sync_sm);
#endif

	sem_destroy(&cxt->hdr_flag_sm);
	sem_destroy(&cxt->share_path_sm);
	sem_destroy(&cxt->access_sm);
	sem_destroy(&cxt->isp_sync_sm);
	return ret;
}

static cmr_int camera_pre_preview_proc(cmr_handle oem_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	CMR_MSG_INIT(message);

	CMR_PRINT_TIME;

	cxt->err_code = CMR_CAMERA_SUCCESS;
	/*create thread*/
	ret = cmr_thread_create((cmr_handle*)&cxt->pre_preview_thread,
					CAMERA_OEM_MSG_QUEUE_SIZE,
					camera_pre_preview_thread_proc,
					(void*)cxt);
	if (CMR_MSG_SUCCESS != ret) {
		CMR_LOGE("create thread fail");
	}
	CMR_LOGI("pre preview thread created");
	message.msg_type  = CMR_EVT_PRE_PREVIEW;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	ret = cmr_thread_msg_send(cxt->pre_preview_thread, &message);
	if (ret) {
		CMR_LOGE("send msg failed!");
		ret = CMR_CAMERA_FAIL;
	}

	return ret;
}

static cmr_int camera_pre_preview_deinit(cmr_handle oem_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	CMR_MSG_INIT(message);

	CMR_PRINT_TIME;

	cxt->err_code = CMR_CAMERA_SUCCESS;

	message.msg_type  = CMR_EVT_PRE_PREVIEW_EXIT;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	ret = cmr_thread_msg_send(cxt->pre_preview_thread, &message);
	if (ret) {
		CMR_LOGE("send msg failed!");
		ret = CMR_CAMERA_FAIL;
	}

	return ret;
}

cmr_int camera_init_internal(cmr_handle  oem_handle, cmr_uint is_autotest)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;

	ret = camera_sensor_init(oem_handle, is_autotest);
	if (ret) {
		CMR_LOGE("failed to init sensor %ld", ret);
		goto exit;
	}
	ret = camera_res_init(oem_handle);
	if (ret) {
		CMR_LOGE("failed to init sensor %ld", ret);
		goto sensor_deinit;
	}

	ret = camera_isp_init(oem_handle);
	if (ret) {
		CMR_LOGE("failed to init isp %ld", ret);
		goto res_deinit;
	}
	ret = camera_res_init_done(oem_handle);
	goto exit;

sensor_deinit:
	ret = camera_sensor_deinit(oem_handle);
res_deinit:
	ret = camera_res_deinit(oem_handle);
exit:
	return ret;
}

cmr_int camera_deinit_internal(cmr_handle  oem_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;

	camera_pre_preview_deinit(oem_handle);
	camera_isp_deinit_notice(oem_handle);
	camera_isp_deinit(oem_handle);
	camera_res_deinit(oem_handle);
	camera_sensor_deinit(oem_handle);

	return ret;
}

cmr_int camera_start_encode(cmr_handle oem_handle, cmr_handle caller_handle,
                                         struct img_frm *src, struct img_frm *dst, struct cmr_op_mean *mean)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	struct jpeg_enc_in_param       enc_in_param;
	cmr_uint                       stream_size;
	struct setting_context         *setting_cxt = &cxt->setting_cxt;
	struct setting_cmd_parameter   setting_param;
	setting_param.camera_id = cxt->camera_id;
	if (!caller_handle || !oem_handle || !src || !dst || !mean) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	sem_wait(&cxt->access_sm);
	CMR_LOGI("src phy addr 0x%lx 0x%lx src vir addr 0x%lx 0x%lx", src->addr_phy.addr_y, src->addr_phy.addr_u,
			 src->addr_vir.addr_y, src->addr_vir.addr_u);
	CMR_LOGI("dst phr addr 0x%lx vir addr 0x%lx", dst->addr_phy.addr_y, dst->addr_vir.addr_y);
	CMR_LOGI("src size %d %d", src->size.width, src->size.height);
	CMR_LOGI("out size %d %d", dst->size.width, dst->size.height);
	CMR_LOGD("caller_handle 0x%lx jpeg handle 0x%lx", (cmr_uint)caller_handle, (cmr_uint)cxt->jpeg_cxt.jpeg_handle);

	enc_in_param.slice_height = mean->slice_height;
	enc_in_param.slice_mod = mean->slice_mode;
	enc_in_param.quality_level = mean->quality_level;
	enc_in_param.stream_buf_phy = dst->addr_phy.addr_y;
	enc_in_param.stream_buf_vir = dst->addr_vir.addr_y;
	enc_in_param.stream_buf_size = dst->buf_size;
	enc_in_param.jpeg_handle = cxt->jpeg_cxt.jpeg_handle;
	enc_in_param.size = src->size;
	enc_in_param.out_size = dst->size;/*actual picture size*/
	enc_in_param.src_addr_phy = src->addr_phy;
	enc_in_param.src_addr_vir = src->addr_vir;
	enc_in_param.src_endian = src->data_end;
	cxt->jpeg_cxt.enc_caller_handle = caller_handle;
	if (1 != mean->is_thumb) {
#ifdef CONFIG_FACE_BEAUTY 
		cmr_int PerfectSkinLevel=0;
		ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_PERFECT_SKINLEVEL, &setting_param);
		if (ret) {
			CMR_LOGE("failed to get perfect skinlevel %ld, setting_cxt->setting_handle is %p", ret, setting_cxt->setting_handle);
		} else {
			PerfectSkinLevel = setting_param.cmd_type_value;
			CMR_LOGD("perfectskinlevel is %ld", PerfectSkinLevel);
		}
		int skinWhitenLevel = PerfectSkinLevel;
		int skinCleanLevel = PerfectSkinLevel;
		int pic_width = src->size.width;
		int pic_height = src->size.height;
		CMR_LOGD("UCAM skinWhitenLevel is %d, skinCleanLevel is %d", skinWhitenLevel, skinCleanLevel);
		if( PerfectSkinLevel > 0 ){
			TSMakeupData  inMakeupData, outMakeupData;
			unsigned char *yBuf = (unsigned char *)(src->addr_vir.addr_y);
			unsigned char *uvBuf = (unsigned char *)(src->addr_vir.addr_u) ;
			unsigned char * tmpBuf = (unsigned char*)malloc(pic_width*pic_height* 3 / 2);

			inMakeupData.frameWidth = pic_width;
			inMakeupData.frameHeight = pic_height;
			inMakeupData.yBuf = yBuf;
			inMakeupData.uvBuf = uvBuf;

			outMakeupData.frameWidth = pic_width;
			outMakeupData.frameHeight = pic_height;
			outMakeupData.yBuf = tmpBuf;
			outMakeupData.uvBuf = tmpBuf + pic_width*pic_height;
			CMR_LOGD("UCAM frameWidth is %d, frameHeight is %d", pic_width, pic_height);

			if (pic_width > 0 && pic_height > 0) {
				CMR_LOGD("perfect ts_face_beautify will be call");
				TSRect  SkinWhitenTsface;
				memset(&SkinWhitenTsface,0,sizeof(TSRect));
				if(cxt->fd_face_area.face_num>0){
					SkinWhitenTsface.left = (cxt->fd_face_area.face_info[0].sx*pic_width)/(cxt->fd_face_area.frame_width);
					SkinWhitenTsface.top = (cxt->fd_face_area.face_info[0].sy*pic_height)/(cxt->fd_face_area.frame_height);
					SkinWhitenTsface.right = (cxt->fd_face_area.face_info[0].ex*pic_width)/(cxt->fd_face_area.frame_width);
					SkinWhitenTsface.bottom = (cxt->fd_face_area.face_info[0].ey*pic_height)/(cxt->fd_face_area.frame_height);
					CMR_LOGD("UCAM update rect:%d-%d-%d-%d",SkinWhitenTsface.left,SkinWhitenTsface.top,
						SkinWhitenTsface.right,SkinWhitenTsface.bottom);
				}
				int mu_retVal = ts_face_beautify(&inMakeupData, &outMakeupData, skinCleanLevel, skinWhitenLevel, &SkinWhitenTsface,0);
				if(mu_retVal !=  TS_OK) {
					CMR_LOGD("perfect ts_face_beautify ret is %d", ret);
				} else {
					CMR_LOGD("perfect ts_face_beautify return OK");
					memcpy(yBuf, tmpBuf, pic_width * pic_height * 3 / 2);
				}
			}

			free(tmpBuf);
			tmpBuf = NULL;
		}
#endif
		cmr_snapshot_memory_flush(cxt->snp_cxt.snapshot_handle);
		ret = jpeg_enc_start(&enc_in_param);
	} else {
		ret = jpeg_enc_thumbnail(&enc_in_param, &stream_size);
		dst->reserved = (void*)stream_size;
	}
	if (ret) {
		cxt->jpeg_cxt.enc_caller_handle = (cmr_handle)0;
		CMR_LOGE("failed to enc start %ld", ret);
	}
exit:
	sem_post(&cxt->access_sm);
	CMR_LOGD("done %ld", ret);
	return ret;
}

cmr_int camera_start_decode(cmr_handle  oem_handle, cmr_handle caller_handle, struct img_frm *src,
                                        struct img_frm *dst, struct cmr_op_mean *mean)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	struct jpeg_dec_in_param       dec_in_param;

	if (!caller_handle || !oem_handle || !src || !dst || !mean) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	CMR_LOGI("src phy addr 0x%lx src vir addr 0x%lx", src->addr_phy.addr_y, src->addr_vir.addr_y);
	CMR_LOGI("dst phr addr 0x%lx  0x%lx vir addr 0x%lx 0x%lx", dst->addr_phy.addr_y, dst->addr_phy.addr_u,
			 dst->addr_vir.addr_y, dst->addr_vir.addr_u);
	CMR_LOGI("src size %d %d", src->size.width, src->size.height);
	CMR_LOGI("out size %d %d", dst->size.width, dst->size.height);
	CMR_LOGI("temp size %d", mean->temp_buf.buf_size);
	CMR_LOGD("caller_handle 0x%lx", (cmr_uint)caller_handle);
	mean->out_param = 0;
	dec_in_param.stream_buf_phy = src->addr_phy.addr_y;
	dec_in_param.stream_buf_vir = src->addr_vir.addr_y;
	dec_in_param.stream_buf_size = (cmr_uint)src->reserved;
	dec_in_param.slice_height = mean->slice_height;
	dec_in_param.slice_mod = mean->slice_mode;
	dec_in_param.dst_fmt = dst->fmt;
	dec_in_param.temp_buf_phy = mean->temp_buf.addr_phy.addr_y;
	dec_in_param.temp_buf_vir = mean->temp_buf.addr_vir.addr_y;
	dec_in_param.temp_buf_size = mean->temp_buf.buf_size;
	dec_in_param.jpeg_handle = cxt->jpeg_cxt.jpeg_handle;
	dec_in_param.size = dst->size;
	dec_in_param.dst_addr_phy = dst->addr_phy;
	dec_in_param.dst_addr_vir = dst->addr_vir;
	dec_in_param.dst_endian = dst->data_end;
	cxt->jpeg_cxt.dec_caller_handle = caller_handle;
	if (1 != mean->is_sync) {
		ret = jpeg_dec_start(&dec_in_param);
	} else {
		struct jpeg_dec_cb_param      out_param;
		ret = jpeg_dec_start_sync(&dec_in_param, &out_param);
		mean->out_param = out_param.data_endian.uv_endian;
	}
    if (ret) {
		cxt->jpeg_cxt.dec_caller_handle = (cmr_handle)0;
		CMR_LOGE("dec start fail ret %ld", ret);
    }
exit:
	CMR_LOGD("done %ld", ret);
	return ret;
}

cmr_int camera_stop_codec(cmr_handle oem_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;

	ret = jpeg_stop(cxt->jpeg_cxt.jpeg_handle);
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int camera_start_exif_encode(cmr_handle oem_handle, cmr_handle caller_handle, struct img_frm *pic_src,
												struct img_frm *thumb_src, void *exif, struct img_frm *dst, struct jpeg_wexif_cb_param *out_ptr)
{
	UNUSED(exif);
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	struct jpeg_enc_exif_param     enc_exif_param;
	struct jpeg_wexif_cb_param     out_pram;
	struct setting_cmd_parameter   setting_param;

	if (!caller_handle || !oem_handle || !pic_src || !dst || !thumb_src || !out_ptr) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	enc_exif_param.jpeg_handle = cxt->jpeg_cxt.jpeg_handle;
	enc_exif_param.src_jpeg_addr_virt = pic_src->addr_vir.addr_y;
	enc_exif_param.thumbnail_addr_virt = thumb_src->addr_vir.addr_y;
	enc_exif_param.target_addr_virt = dst->addr_vir.addr_y;
	enc_exif_param.src_jpeg_size = pic_src->buf_size;
	enc_exif_param.thumbnail_size = thumb_src->buf_size;
	enc_exif_param.target_size = dst->buf_size;
	setting_param.camera_id = cxt->camera_id;
	ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, SETTING_GET_EXIF_INFO, &setting_param);
	enc_exif_param.exif_ptr = setting_param.exif_all_info_ptr;
	enc_exif_param.exif_isp_info = NULL;
	enc_exif_param.padding = 0;
	out_pram.output_buf_virt_addr = 0;
	out_pram.output_buf_size = 0;
	ret = jpeg_enc_add_eixf(&enc_exif_param, &out_pram);
	if (!ret) {
		*out_ptr = out_pram;
		CMR_LOGI("out addr 0x%lx size %ld", out_ptr->output_buf_virt_addr, out_ptr->output_buf_size);
	} else {
		CMR_LOGE("failed to compund exif jpeg %ld", ret);
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int camera_start_scale(cmr_handle oem_handle, cmr_handle caller_handle, struct img_frm *src,
                                      struct img_frm *dst, struct cmr_op_mean *mean)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;

	if (!caller_handle || !oem_handle || !src || !dst || !mean) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	CMR_LOGI("caller_handle 0x%lx is_sync %d", (cmr_uint)caller_handle, mean->is_sync);
	CMR_LOGI("src phy addr 0x%lx 0x%lx, dst phy addr 0x%lx 0x%lx",
		src->addr_phy.addr_y,
		src->addr_phy.addr_u,
		dst->addr_phy.addr_y,
		dst->addr_phy.addr_u);

	CMR_LOGI("src size %d %d dst size %d %d rect %d %d %d %d endian %d %d %d %d",
		src->size.width,
		src->size.height,
		dst->size.width,
		dst->size.height,
		src->rect.start_x,
		src->rect.start_y,
		src->rect.width,
		src->rect.height,
		src->data_end.y_endian,
		src->data_end.uv_endian,
		dst->data_end.y_endian,
		dst->data_end.uv_endian);

	if (1 != mean->is_sync) {
		ret = cmr_scale_start(cxt->scaler_cxt.scaler_handle, src, dst, camera_scaler_evt_cb, (void*)caller_handle);
	} else {
		ret = cmr_scale_start(cxt->scaler_cxt.scaler_handle, src, dst, (cmr_evt_cb)NULL, NULL);
	}
	if (ret) {
		CMR_LOGE("failed to start scaler, ret %ld", ret);
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int camera_start_rot(cmr_handle oem_handle, cmr_handle caller_handle, struct img_frm *src,
                                   struct img_frm *dst, struct cmr_op_mean *mean)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	struct cmr_rot_param           rot_param;
	cmr_uint                       restart_cnt = 0;

	if (!caller_handle || !oem_handle || !src || !dst || !mean) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	camera_take_snapshot_step(CMR_STEP_ROT_S);
	do {
		rot_param.angle = mean->rot;
		rot_param.handle = cxt->rot_cxt.rotation_handle;
		rot_param.src_img = *src;
		rot_param.dst_img = *dst;
		ret = cmr_rot(&rot_param);
		if (ret) {
			CMR_LOGE("failed to rotate %ld", ret);
			ret = camera_restart_rot(oem_handle);
		} else {
			goto rot_end;
		}
		restart_cnt++;
	} while (restart_cnt < OEM_RESTART_SUM);
rot_end:
	camera_take_snapshot_step(CMR_STEP_ROT_E);
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int camera_close_flash(cmr_handle oem_handle, cmr_u32 camera_id)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	struct snapshot_context        *snp_cxt;
	struct setting_cmd_parameter   setting_param;
	cmr_int                        flash_status = FLASH_CLOSE;

	if (!oem_handle || (camera_id != cxt->camera_id)) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	memset(&setting_param, 0, sizeof(setting_param));
	if (0 == camera_id) {
		setting_param.camera_id = camera_id;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, SETTING_GET_HW_FLASH_STATUS, &setting_param);
		if (ret) {
			CMR_LOGE("failed to get flash mode %ld", ret);
		}
		flash_status = setting_param.cmd_type_value;
		CMR_LOGI("HW flash_status=%ld", flash_status);

		snp_cxt = &cxt->snp_cxt;
		if ((!ret) && (FLASH_OPEN == flash_status || FLASH_HIGH_LIGHT == flash_status)) {
			/*close flash*/
			memset(&setting_param, 0, sizeof(setting_param));
			setting_param.camera_id = camera_id;
			setting_param.ctrl_flash.capture_mode.capture_mode= snp_cxt->snp_mode;
			setting_param.ctrl_flash.is_active = 0;
			setting_param.ctrl_flash.work_mode = 1; //capture
			setting_param.ctrl_flash.flash_type = FLASH_CLOSE;
			ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, SETTING_CTRL_FLASH, &setting_param);
			if (ret) {
				CMR_LOGE("failed to open flash");
			}
		}
	}

exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int camera_preview_pre_proc(cmr_handle oem_handle, cmr_u32 camera_id, cmr_u32 preview_sn_mode)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	struct setting_cmd_parameter   setting_param;
	cmr_u32                        sensor_mode;

	if (!oem_handle) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	CMR_LOGI("camera id %d", camera_id);

	/*
	if (CAMERA_ZSL_MODE != cxt->snp_cxt.snp_mode) {
		sensor_mode = cxt->prev_cxt.preview_sn_mode;
	} else {
		sensor_mode = cxt->snp_cxt.snapshot_sn_mode;
	} */
	sensor_mode = preview_sn_mode;
	cxt->prev_cxt.preview_sn_mode = preview_sn_mode;

	CMR_LOGI("sensor work mode %d", sensor_mode);
	ret = cmr_sensor_set_mode(cxt->sn_cxt.sensor_handle, camera_id, sensor_mode);
	if (ret) {
		CMR_LOGE("failed to set sensor work mode %ld", ret);
		goto exit;
	}
	ret = cmr_sensor_set_mode_done(cxt->sn_cxt.sensor_handle, camera_id);
	if (ret) {
		CMR_LOGE("failed to wait done %ld", ret);
		goto exit;
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int camera_preview_post_proc(cmr_handle oem_handle, cmr_u32 camera_id)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;

	if (!oem_handle || (camera_id != cxt->camera_id)) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	ret = camera_close_flash(oem_handle, camera_id);

exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int camera_capture_pre_proc(cmr_handle oem_handle, cmr_u32 camera_id, cmr_u32 preview_mode, cmr_u32 capture_mode, cmr_u32 is_restart, cmr_u32 is_sn_reopen)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	struct snapshot_context        *snp_cxt;
	struct setting_cmd_parameter   setting_param;
	struct common_sn_cmd_param param;
	cmr_uint			hw_flash_status = 0;

	if (!oem_handle || (camera_id != cxt->camera_id)) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	snp_cxt = &cxt->snp_cxt;

	CMR_LOGI("camera id %d, capture mode %d preview mode %d,  is_restart %d is_sn_reopen %d, snapshot_sn_mode %d",
			camera_id, capture_mode, preview_mode, is_restart, is_sn_reopen, snp_cxt->snp_mode);

	if (/*(CAMERA_ZSL_MODE != snp_cxt->snp_mode) &&*/ (!is_restart) && (1 != camera_get_hdr_flag(cxt))) {
		/*open flash*/
		memset(&setting_param, 0, sizeof(setting_param));
		setting_param.ctrl_flash.capture_mode.capture_mode= snp_cxt->snp_mode;
		setting_param.camera_id = camera_id;
		setting_param.ctrl_flash.is_active = 1;
		setting_param.ctrl_flash.flash_type = FLASH_HIGH_LIGHT;
		setting_param.ctrl_flash.work_mode = 1; //capture
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, SETTING_CTRL_FLASH, &setting_param);
		if (ret) {
			CMR_LOGE("failed to open flash");
		}
	}
	//set flash exif
/*
	memset(&setting_param, 0, sizeof(setting_param));
	setting_param.camera_id = camera_id;
	ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, SETTING_GET_HW_FLASH_STATUS, &setting_param);
	if (ret) {
		CMR_LOGE("failed to get flash mode %ld", ret);
	}
	hw_flash_status = setting_param.cmd_type_value;
	if (FLASH_OPEN == hw_flash_status || FLASH_HIGH_LIGHT == hw_flash_status || FLASH_TORCH == hw_flash_status) {
		cmr_sensor_set_exif(cxt->sn_cxt.sensor_handle, cxt->camera_id, SENSOR_EXIF_CTRL_FLASH, 1);
	} else {
		cmr_sensor_set_exif(cxt->sn_cxt.sensor_handle, cxt->camera_id, SENSOR_EXIF_CTRL_FLASH, 0);
	}
*/
	if ((CAMERA_ZSL_MODE != snp_cxt->snp_mode) && (!is_restart || is_sn_reopen)) {
		snp_cxt = &cxt->snp_cxt;
		snp_cxt->snapshot_sn_mode = capture_mode;
		cxt->prev_cxt.preview_sn_mode = preview_mode;
		/*set sensor before snapshot*/
		ret = cmr_sensor_ioctl(cxt->sn_cxt.sensor_handle, camera_id, SENSOR_BEFORE_SNAPSHOT,
			                  (capture_mode | (cxt->prev_cxt.preview_sn_mode << CAP_MODE_BITS)));
		if (ret) {
			CMR_LOGE("failed to set sensor %ld", ret);
		}
	}

	if (1 == camera_get_hdr_flag(cxt)) {
		ret = cmr_ipm_pre_proc(cxt->ipm_cxt.hdr_handle);
		if (ret) {
			CMR_LOGE("failed to ipm pre proc, %ld", ret);
		}
	}
	if (cxt->is_vendor_hdr) {
		cxt->cap_cnt = 0;
		param.cmd_value = OEM_EV_LEVEL_1;
		ret = camera_sensor_ioctl(oem_handle,COM_SN_SET_HDR_EV, &param);
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int camera_capture_post_proc(cmr_handle oem_handle, cmr_u32 camera_id)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	struct snapshot_context        *snp_cxt;
	struct setting_cmd_parameter   setting_param;

	if (!oem_handle) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	snp_cxt = &cxt->snp_cxt;
	/*set sensor after snapshot*/
	if ((CAMERA_ZSL_MODE != snp_cxt->snp_mode) && (CAMERA_ISP_TUNING_MODE != snp_cxt->snp_mode) ) {
		ret = cmr_sensor_ioctl(cxt->sn_cxt.sensor_handle, cxt->camera_id, SENSOR_AFTER_SNAPSHOT,
			               cxt->prev_cxt.preview_sn_mode);
		if (ret) {
			CMR_LOGE("failed to set sensor %ld", ret);
		}
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int camera_open_sensor(cmr_handle oem_handle, cmr_u32 camera_id)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct sensor_context           *sn_cxt = &cxt->sn_cxt;
	cmr_handle                      sensor_handle;
	cmr_u32                         camera_id_bits = 0;

	(void)camera_id;

	sensor_handle  = sn_cxt->sensor_handle;
	camera_id_bits = 1 << cxt->camera_id;
	ret = cmr_sensor_open(sensor_handle, camera_id_bits);
	if (ret) {
		CMR_LOGE("open %d sensor failed %ld", cxt->camera_id, ret);
		return CMR_CAMERA_FAIL;
	}
	return ret;
}

cmr_int camera_close_sensor(cmr_handle oem_handle, cmr_u32 camera_id)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct sensor_context           *sn_cxt = &cxt->sn_cxt;
	cmr_handle                      sensor_handle;
	cmr_u32                         camera_id_bits = 0;

	(void)camera_id;
	sensor_handle  = sn_cxt->sensor_handle;
	camera_id_bits = 1 << cxt->camera_id;
	ret = cmr_sensor_close(sensor_handle, camera_id_bits);
	if (ret) {
		CMR_LOGE("open %d sensor failed %ld", cxt->camera_id, ret);
		return CMR_CAMERA_FAIL;
	}
	return ret;
}

cmr_int camera_raw_proc(cmr_handle oem_handle, cmr_handle caller_handle, struct raw_proc_param *param_ptr)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	struct isp_context             *isp_cxt = &cxt->isp_cxt;
	if (!oem_handle || !param_ptr || !caller_handle) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	CMR_LOGI("slice num %d avail height %d slice height %d", param_ptr->slice_num, param_ptr->src_avail_height,
		     param_ptr->src_slice_height);
	CMR_LOGI("src addr 0x%lx 0x%lx,dst addr 0x%lx 0x%lx", param_ptr->src_frame.addr_phy.addr_y,
			 param_ptr->src_frame.addr_phy.addr_u, param_ptr->dst_frame.addr_phy.addr_y,
			 param_ptr->dst_frame.addr_phy.addr_u);

	if (1 == param_ptr->slice_num) {
		struct ips_in_param  in_param;
		struct ips_out_param out_param;
		in_param.src_frame.img_fmt = param_ptr->src_frame.fmt;
		in_param.src_frame.img_size.w = param_ptr->src_frame.size.width;
		in_param.src_frame.img_size.h = param_ptr->src_frame.size.height;
		in_param.src_frame.img_addr_phy.chn0 = param_ptr->src_frame.addr_phy.addr_y;
		in_param.src_frame.img_addr_phy.chn1 = param_ptr->src_frame.addr_phy.addr_u;
		in_param.src_frame.img_addr_vir.chn0 = param_ptr->src_frame.addr_vir.addr_y;
		in_param.src_frame.img_addr_vir.chn1 = param_ptr->src_frame.addr_vir.addr_u;
		in_param.src_avail_height = param_ptr->src_avail_height;
		in_param.src_slice_height = param_ptr->src_slice_height;
		in_param.dst_frame.img_fmt = param_ptr->dst_frame.fmt;
		in_param.dst_frame.img_size.w = param_ptr->dst_frame.size.width;
		in_param.dst_frame.img_size.h = param_ptr->dst_frame.size.height;
		in_param.dst_frame.img_addr_phy.chn0 = param_ptr->dst_frame.addr_phy.addr_y;
		in_param.dst_frame.img_addr_phy.chn1 = param_ptr->dst_frame.addr_phy.addr_u;
		in_param.dst_frame.img_addr_vir.chn0 = param_ptr->dst_frame.addr_vir.addr_y;
		in_param.dst_frame.img_addr_vir.chn1 = param_ptr->dst_frame.addr_vir.addr_u;
		in_param.dst_slice_height = param_ptr->dst_slice_height;

		ret = isp_proc_start(isp_cxt->isp_handle, &in_param, &out_param);

		if (ret) {
			CMR_LOGE("failed to start proc %ld", ret);
		}
	} else {
		struct ipn_in_param in_param;
		struct ips_out_param out_param;
		in_param.src_avail_height = param_ptr->src_avail_height;
		in_param.src_slice_height = param_ptr->src_slice_height;
		in_param.src_addr_phy.chn0 = param_ptr->src_frame.addr_phy.addr_y;
		in_param.src_addr_phy.chn1 = param_ptr->src_frame.addr_phy.addr_u;
		in_param.dst_addr_phy.chn0 = param_ptr->dst_frame.addr_phy.addr_y;
		in_param.dst_addr_phy.chn1 = param_ptr->dst_frame.addr_phy.addr_u;
		ret = isp_proc_next(isp_cxt->isp_handle, &in_param, &out_param);
		if (ret) {
			CMR_LOGE("failed to start proc %ld", ret);
		}
	}
	if (CMR_CAMERA_SUCCESS == ret) {
		cxt->isp_cxt.caller_handle = caller_handle;
		CMR_LOGD("caller handle 0x%lx", (cmr_uint)caller_handle);
	}
exit:
	return ret;
}

cmr_int camera_isp_start_video(cmr_handle oem_handle, struct video_start_param *param_ptr)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	struct isp_context             *isp_cxt = &cxt->isp_cxt;
	struct isp_video_start         isp_param;
	cmr_int                        work_mode = 0;
	struct setting_cmd_parameter   setting_param;

#if !(defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4))
	struct isp_ae_info             isp_ae_info;
	struct sensor_ae_info          *ae_info;
	struct isp_trim_size           wb_trim;
	struct sensor_mode_info        *sensor_mode_info;
	cmr_uint                       sn_mode = 0;
#endif

	if (!param_ptr || !oem_handle) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	isp_param.size.w = param_ptr->size.width;
	isp_param.size.h = param_ptr->size.height;
	isp_param.format = ISP_DATA_NORMAL_RAW10;
	isp_param.mode = param_ptr->video_mode;
#if !(defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4))
	ret = cmr_sensor_get_mode(cxt->sn_cxt.sensor_handle, cxt->camera_id, &sn_mode);
	ae_info = &cxt->sn_cxt.sensor_info.video_info[sn_mode].ae_info[0];
	isp_ae_info.gain = ae_info->gain;
	isp_ae_info.line_time = ae_info->line_time;
	isp_ae_info.min_fps = ae_info->min_frate;
	isp_ae_info.max_fps = ae_info->max_frate;
	CMR_LOGI("line time %d sn_mode %ld", isp_ae_info.line_time, sn_mode);
	isp_ioctl(NULL, ISP_CTRL_AE_INFO, (void*)&isp_ae_info);

	sensor_mode_info = &cxt->sn_cxt.sensor_info.mode_info[sn_mode];
	wb_trim.x = 0;
	wb_trim.y = 0;
	wb_trim.w = sensor_mode_info->trim_width;
	wb_trim.h = sensor_mode_info->trim_height;
	ret = isp_ioctl(NULL, ISP_CTRL_WB_TRIM,(void*)&wb_trim);
	if (ret) {
		CMR_LOGE("set wb trim information error.");
	}
#endif

#if (defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4))
	isp_param.is_need_flash = param_ptr->is_need_flash;
	isp_param.capture_skip_num = param_ptr->capture_skip_num;
	if (cxt->isp_malloc_flag == 0) {
		isp_param.lsc_buf_num = ISP_LSC_BUF_NUM;
		isp_param.lsc_buf_size = ISP_LSC_BUF_SIZE;

		if (cxt->hal_malloc) {
			cxt->hal_malloc(CAMERA_ISP_LSC, (cmr_u32 *)&isp_param.lsc_buf_size,(cmr_u32 *)&isp_param.lsc_buf_num,
					&cxt->isp_lsc_phys_addr,&cxt->isp_lsc_virt_addr, cxt->client_data);
			cxt->isp_malloc_flag = 1;
		} else {
			CMR_LOGE("failed to malloc isp lsc buffer");
		}

	}

	isp_param.lsc_phys_addr = cxt->isp_lsc_phys_addr;
	isp_param.lsc_virt_addr = cxt->isp_lsc_virt_addr;

	CMR_LOGI("lsc_phys_addr 0x%lx,lsc_virt_addr 0x%lx",isp_param.lsc_phys_addr,
		isp_param.lsc_virt_addr);

	if (cxt->isp_b4awb_flag == 0) {
		isp_param.b4awb_mem_num = ISP_B4AWB_BUF_CNT;
		isp_param.b4awb_mem_size = ISP_B4AWB_BUF_SIZE;
		cmr_bzero(isp_param.b4awb_phys_addr_array, (ISP_B4AWB_BUF_CNT)*sizeof(isp_uint));
		cmr_bzero(isp_param.b4awb_virt_addr_array, (ISP_B4AWB_BUF_CNT)*sizeof(isp_uint));

		if (cxt->hal_malloc) {
			cxt->hal_malloc(CAMERA_ISP_BINGING4AWB, (cmr_u32 *)&isp_param.b4awb_mem_size, (cmr_u32 *)&isp_param.b4awb_mem_num,
					cxt->b4awb_phys_addr,cxt->b4awb_virt_addr, cxt->client_data);
			cxt->isp_b4awb_flag = 1;
		} else {
			CMR_LOGE("failed to malloc binging4awb buffer");
			goto exit;
		}
	}

	isp_param.b4awb_phys_addr_array[0] = cxt->b4awb_phys_addr[0];
	isp_param.b4awb_phys_addr_array[1] = cxt->b4awb_phys_addr[1];
	isp_param.b4awb_virt_addr_array[0] = cxt->b4awb_virt_addr[0];
	isp_param.b4awb_virt_addr_array[1] = cxt->b4awb_virt_addr[1];

	CMR_LOGI("b4awb_phys_addr_array 0x%lx, 0x%lx,b4awb_virt_addr_array 0x%lx,0x%lx",isp_param.b4awb_phys_addr_array[0],
		isp_param.b4awb_phys_addr_array[1],isp_param.b4awb_virt_addr_array[0],isp_param.b4awb_virt_addr_array[1]);

#endif

	if (0 == param_ptr->work_mode) {
		work_mode = param_ptr->work_mode;

		cmr_bzero(&setting_param, sizeof(setting_param));
		setting_param.camera_id = cxt->camera_id;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, SETTING_GET_DV_MODE, &setting_param);
		if (setting_param.cmd_type_value) {
			work_mode = 2;
		}
	} else {
		work_mode = param_ptr->work_mode;
	}
	isp_param.work_mode = work_mode;

	CMR_LOGI("work_mode %ld", work_mode);
	CMR_LOGI("isp w h, %d %d", isp_param.size.w, isp_param.size.h);
	ret = isp_video_start(isp_cxt->isp_handle, &isp_param);
	if (!ret) {
		isp_cxt->is_work = 1;
	} else {
		isp_cxt->is_work = 0;
		CMR_LOGE("failed to start isp, ret %ld", ret);
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int camera_isp_stop_video(cmr_handle oem_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	struct isp_context             *isp_cxt = &cxt->isp_cxt;

	if (!oem_handle) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	if (cxt->isp_cxt.is_work) {
		ret = isp_video_stop(isp_cxt->isp_handle);
		if (ret) {
			CMR_LOGE("failed to stop isp %ld", ret);
		} else {
			cxt->isp_cxt.is_work = 0;
		}
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int camera_channel_cfg(cmr_handle oem_handle, cmr_handle caller_handle, cmr_u32 camera_id,
										struct channel_start_param *param_ptr, cmr_u32 *channel_id, struct img_data_end *endian)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	struct sensor_context          *sn_cxt = &cxt->sn_cxt;
	struct sensor_exp_info         sensor_info;
	struct sensor_mode_info        *sensor_mode_info;
	struct sn_cfg                  sensor_cfg;

	if (!oem_handle || !caller_handle || !param_ptr || !channel_id || !endian) {
		CMR_LOGE("in parm error 0x%lx 0x%lx 0x%lx 0x%lx 0x%lx", (cmr_uint)oem_handle, (cmr_uint)caller_handle,
				(cmr_uint)param_ptr, (cmr_uint)channel_id, (cmr_uint)endian);
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	if (param_ptr->is_lightly) {
		ret = cmr_grab_cap_cfg_lightly(cxt->grab_cxt.grab_handle, &param_ptr->cap_inf_cfg, *channel_id);
		if (ret) {
			CMR_LOGE("failed to cap cfg %ld", ret);
			goto exit;
		}
		return ret;
	}

	ret = cmr_grab_if_cfg(cxt->grab_cxt.grab_handle, &param_ptr->sn_if);
	if (ret) {
		CMR_LOGE("failed interface cfg %ld", ret);
		goto exit;
	}
	ret = cmr_sensor_get_info(sn_cxt->sensor_handle, camera_id, &sensor_info);
	if (ret) {
		CMR_LOGE("failed to get sensor info %ld", ret);
		goto exit;
	}

	CMR_LOGI("frm_num 0x%x", param_ptr->frm_num);

	sensor_mode_info = &sensor_info.mode_info[param_ptr->sensor_mode];
	sensor_cfg.sn_size.width = sensor_mode_info->width;
	sensor_cfg.sn_size.height = sensor_mode_info->height;
	sensor_cfg.frm_num = param_ptr->frm_num;
	sensor_cfg.sn_trim.start_x = sensor_mode_info->trim_start_x;
	sensor_cfg.sn_trim.start_y = sensor_mode_info->trim_start_y;
	sensor_cfg.sn_trim.width = sensor_mode_info->trim_width;
	sensor_cfg.sn_trim.height = sensor_mode_info->trim_height;
	ret = cmr_grab_sn_cfg(cxt->grab_cxt.grab_handle, &sensor_cfg);
	if (ret) {
		CMR_LOGE("failed to sn cfg %ld", ret);
		goto exit;
	}
	if (caller_handle == cxt->prev_cxt.preview_handle) {
		cxt->prev_cxt.rect = param_ptr->cap_inf_cfg.cfg.src_img_rect;
		CMR_LOGI("prev rect %d %d %d %d", cxt->prev_cxt.rect.start_x, cxt->prev_cxt.rect.start_y,
				  cxt->prev_cxt.rect.width, cxt->prev_cxt.rect.height);
	}
	if (!param_ptr->is_lightly) {
		ret = cmr_grab_cap_cfg(cxt->grab_cxt.grab_handle, &param_ptr->cap_inf_cfg, channel_id, endian);
		if (ret) {
			CMR_LOGE("failed to cap cfg %ld", ret);
			goto exit;
		}
	}

	param_ptr->buffer.channel_id = *channel_id; /*update the channel id*/
	ret = cmr_grab_buff_cfg(cxt->grab_cxt.grab_handle, &param_ptr->buffer);
	if (ret) {
		CMR_LOGE("failed to buf cfg %ld", ret);
		goto exit;
	}
	cxt->grab_cxt.caller_handle[*channel_id] = caller_handle;
	cxt->grab_cxt.skip_number[*channel_id] = param_ptr->skip_num;
	CMR_LOGI("channel id %d, caller_handle 0x%lx, skip num %d",
			  *channel_id, (cmr_uint)caller_handle, param_ptr->skip_num);
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int camera_pre_channel_cfg(cmr_handle oem_handle, cmr_handle caller_handle, cmr_u32 camera_id,
                                        struct channel_start_param *param_ptr)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	struct sensor_context          *sn_cxt = &cxt->sn_cxt;
	struct sensor_exp_info         sensor_info;
	struct sensor_mode_info        *sensor_mode_info;
	struct sn_cfg                  sensor_cfg;

	ret = cmr_grab_if_cfg(cxt->grab_cxt.grab_handle, &param_ptr->sn_if);
	if (ret) {
		CMR_LOGE("failed interface cfg %ld", ret);
		goto exit;
	}
	ret = cmr_sensor_get_info(sn_cxt->sensor_handle, camera_id, &sensor_info);
	if (ret) {
		CMR_LOGE("failed to get sensor info %ld", ret);
		goto exit;
	}

	CMR_LOGI("pre frm_num 0x%x", param_ptr->frm_num);

	sensor_mode_info = &sensor_info.mode_info[param_ptr->sensor_mode];
	sensor_cfg.sn_size.width = sensor_mode_info->width;
	sensor_cfg.sn_size.height = sensor_mode_info->height;
	sensor_cfg.frm_num = param_ptr->frm_num;
	sensor_cfg.sn_trim.start_x = sensor_mode_info->trim_start_x;
	sensor_cfg.sn_trim.start_y = sensor_mode_info->trim_start_y;
	sensor_cfg.sn_trim.width = sensor_mode_info->trim_width;
	sensor_cfg.sn_trim.height = sensor_mode_info->trim_height;
	ret = cmr_grab_sn_cfg(cxt->grab_cxt.grab_handle, &sensor_cfg);
	if (ret) {
		CMR_LOGE("failed to sn cfg %ld", ret);
		goto exit;
	}


exit:
    CMR_LOGI("done %ld", ret);
    return ret;
}

cmr_int camera_channel_buff_cfg (cmr_handle oem_handle, struct buffer_cfg *buf_cfg)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	if (!oem_handle) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	ret = cmr_grab_buff_cfg(cxt->grab_cxt.grab_handle, buf_cfg);
	if (ret) {
		CMR_LOGE("failed to buf cfg %ld", ret);
		goto exit;
	}
exit:
	CMR_LOGV("done %ld", ret);
	return ret;
}
cmr_int camera_channel_start(cmr_handle oem_handle, cmr_u32 channel_bits, cmr_uint skip_number)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;

	if (!oem_handle) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	CMR_LOGI("skip num %ld %d", skip_number, channel_bits);
	camera_take_snapshot_step(CMR_STEP_CAP_S);
	ret = cmr_grab_cap_start(cxt->grab_cxt.grab_handle, skip_number);
	if (ret) {
		CMR_LOGE("failed to start cap %ld", ret);
		goto exit;
	}
exit:
	CMR_LOGV("done %ld", ret);
	return ret;
}

cmr_int camera_channel_pause(cmr_handle oem_handle, cmr_uint channel_id, cmr_u32 reconfig_flag)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;

	if (!oem_handle) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		return ret;
	}
	CMR_LOGI("channel id %ld, reconfig flag %d", channel_id, reconfig_flag);
	ret = cmr_grab_cap_pause(cxt->grab_cxt.grab_handle, channel_id, reconfig_flag);
	if (ret) {
		CMR_LOGE("failed to pause channel %ld", ret);
		goto exit;
	}
exit:
	if (TAKE_PICTURE_NEEDED == camera_get_snp_req((cmr_handle)cxt) || (1== camera_get_share_path_sm_flag(oem_handle))) {
		camera_post_share_path_available(oem_handle);
	}
	CMR_LOGV("done %ld", ret);
	return ret;
}

cmr_int camera_channel_resume(cmr_handle oem_handle, cmr_uint channel_id, cmr_u32 skip_number,
	                                          cmr_u32 deci_factor, cmr_u32 frm_num)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	struct common_sn_cmd_param param;

	if (!oem_handle) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	CMR_LOGI("channel id %ld, skip num %d, deci %d, frm num %d",
		      channel_id, skip_number,deci_factor,frm_num);
	camera_set_discard_frame(cxt, 0);
	ret = cmr_grab_cap_resume(cxt->grab_cxt.grab_handle, channel_id, skip_number, deci_factor, frm_num);
	if (ret) {
		CMR_LOGE("failed to resume channel,ret %ld", ret);
		goto exit;
	}
	if (cxt->is_vendor_hdr) {
		cxt->cap_cnt++;
		if (cxt->cap_cnt == 1) {
			/* set ev = 1 */
			param.cmd_value = OEM_EV_LEVEL_2;
			ret = camera_sensor_ioctl(oem_handle,COM_SN_SET_HDR_EV, &param);
		} else if(cxt->cap_cnt == 2) {
			/*set ev = 2 */
			param.cmd_value = OEM_EV_LEVEL_3;
			ret = camera_sensor_ioctl(oem_handle,COM_SN_SET_HDR_EV, &param);
		} else {
			cxt->cap_cnt=0;
			/* set ev = 1 as default */
			param.cmd_value = OEM_EV_LEVEL_2;
			ret = camera_sensor_ioctl(oem_handle,COM_SN_SET_HDR_EV, &param);
		}
	}

exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int camera_channel_free_frame(cmr_handle oem_handle, cmr_u32 channel_id, cmr_u32 index)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;

	if (!oem_handle) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	ret = cmr_grab_free_frame(cxt->grab_cxt.grab_handle, channel_id, index);
	if (ret) {
		CMR_LOGE("failed to free frame %d %ld", channel_id, ret);
		goto exit;
	}
exit:
	return ret;
}

cmr_int camera_channel_stop(cmr_handle oem_handle, cmr_u32 channel_bits)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;

	(cmr_u32)channel_bits;
	if (!oem_handle) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	ret = cmr_grab_cap_stop(cxt->grab_cxt.grab_handle, 1);
	if (ret) {
		CMR_LOGE("failed to stop channel %ld", ret);
		goto exit;
	}
exit:
	CMR_LOGV("done %ld", ret);
	return ret;
}

cmr_int camera_pre_channel_stop(cmr_handle oem_handle, cmr_u32 channel_bits, cmr_u32 is_sensor_off)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;

	(cmr_u32)channel_bits;
	if (!oem_handle) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	ret = cmr_grab_cap_stop(cxt->grab_cxt.grab_handle, is_sensor_off);
	if (ret) {
		CMR_LOGE("failed to stop channel %ld", ret);
		goto exit;
	}
exit:
	CMR_LOGV("done %ld", ret);
	return ret;
}

cmr_int camera_channel_scale_capability(cmr_handle oem_handle, cmr_u32 *width, cmr_u32 *sc_factor, cmr_u32 *sc_threshold)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;

	if (!oem_handle || !width | !sc_factor) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	ret = cmr_grab_scale_capability(cxt->grab_cxt.grab_handle, width, sc_factor, sc_threshold);
exit:
	return ret;
}

cmr_int camera_channel_path_capability(cmr_handle oem_handle, struct cmr_path_capability *capability)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	if (!oem_handle || !capability) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	ret = cmr_grab_path_capability(cxt->grab_cxt.grab_handle, capability);
exit:
	return ret;
}
cmr_int camera_channel_get_cap_time(cmr_handle oem_handle, cmr_u32 *sec, cmr_u32 *usec)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;

	if (!oem_handle || !sec | !usec) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	ret = cmr_grab_get_cap_time(cxt->grab_cxt.grab_handle, sec, usec);
exit:
	return ret;
}

cmr_int camera_get_sensor_info(cmr_handle oem_handle, cmr_uint sensor_id, struct sensor_exp_info *exp_info_ptr)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;

	if (!oem_handle || !exp_info_ptr) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	CMR_LOGD("sensor id %ld", sensor_id);
	ret = cmr_sensor_get_info(cxt->sn_cxt.sensor_handle, sensor_id, exp_info_ptr);
	if (ret) {
		CMR_LOGE("failed to get sensor info %ld", ret);
		goto exit;
	}
exit:
	return ret;
}

cmr_int camera_get_sensor_autotest_mode(cmr_handle oem_handle, cmr_uint sensor_id, cmr_uint *is_autotest)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;

	if (!oem_handle || !is_autotest) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		return ret;
	}
	CMR_LOGI("sensor id %ld", sensor_id);
	ret = cmr_sensor_get_autotest_mode(cxt->sn_cxt.sensor_handle, sensor_id, is_autotest);
	if (ret) {
		CMR_LOGE("failed to get sensor info %ld", ret);
		goto exit;
	}
exit:
	CMR_LOGV("ret %ld, is_autotest %ld", ret, *is_autotest);
	return ret;
}

cmr_int camera_ioctl_for_setting(cmr_handle oem_handle, cmr_uint cmd_type, struct setting_io_parameter *param_ptr)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	cmr_handle                     grab_handle;

	if (!oem_handle) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	grab_handle = cxt->grab_cxt.grab_handle;

	switch (cmd_type) {
	case SETTING_IO_GET_CAPTURE_SIZE:
		param_ptr->size_param = cxt->snp_cxt.post_proc_setting.snp_size;
		break;
	case SETTING_IO_GET_ACTUAL_CAPTURE_SIZE:
		param_ptr->size_param = cxt->snp_cxt.post_proc_setting.actual_snp_size;
		//param_ptr->size_param = cxt->snp_cxt.post_proc_setting.dealign_actual_snp_size;
		break;
	case SETTING_IO_CTRL_FLASH:

		if (FLASH_OPEN == param_ptr->cmd_value || FLASH_HIGH_LIGHT == param_ptr->cmd_value || FLASH_TORCH == param_ptr->cmd_value) {
			cmr_sensor_set_exif(cxt->sn_cxt.sensor_handle, cxt->camera_id, SENSOR_EXIF_CTRL_FLASH, 1);
		}
		cmr_grab_flash_cb(grab_handle, param_ptr->cmd_value);
		break;
	case SETTING_IO_GET_PREVIEW_MODE:
		param_ptr->cmd_value = cxt->prev_cxt.preview_sn_mode;
		break;
	default:
		CMR_LOGE("don't support cmd %ld", cmd_type);
		ret = CMR_CAMERA_NO_SUPPORT;
		break;
	}
exit:
	return ret;
}

cmr_int camera_sensor_ioctl(cmr_handle oem_handle, cmr_uint cmd_type, struct common_sn_cmd_param *param_ptr)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	cmr_uint                       cmd = SENSOR_CMD_MAX;
	cmr_uint                       sensor_param = 0;
	cmr_uint                       set_exif_flag = 0;
	SENSOR_EXIF_CTRL_E             exif_cmd;
	SENSOR_EXT_FUN_PARAM_T_PTR 	   hdr_ev_param_ptr = 0;
	SENSOR_EXT_FUN_PARAM_T_PTR     fps_param_ptr = 0;

	if (!oem_handle || !param_ptr) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	CMR_LOGD("cmd_type =%ld",cmd_type);

	switch (cmd_type) {
	case COM_SN_GET_AUTO_FLASH_STATE:
		cmd = SENSOR_CHECK_NEED_FLASH;
		sensor_param = (cmr_uint)&param_ptr->cmd_value;
		break;

	case COM_SN_SET_BRIGHTNESS:
		cmd = SENSOR_BRIGHTNESS;
		set_exif_flag = 1;
		exif_cmd = SENSOR_EXIF_CTRL_BRIGHTNESSVALUE;
		sensor_param = param_ptr->cmd_value;
		break;
	case COM_SN_SET_CONTRAST:
		cmd = SENSOR_CONTRAST;
		set_exif_flag = 1;
		exif_cmd = SENSOR_EXIF_CTRL_CONTRAST;
		sensor_param = param_ptr->cmd_value;
		break;
	case COM_SN_SET_SATURATION:
		cmd = SENSOR_SATURATION;
		set_exif_flag = 1;
		exif_cmd = SENSOR_EXIF_CTRL_SATURATION;
		sensor_param = param_ptr->cmd_value;
		break;
	case COM_SN_SET_SHARPNESS:
		cmd = SENSOR_SHARPNESS;
		set_exif_flag = 1;
		exif_cmd = SENSOR_EXIF_CTRL_SHARPNESS;
		sensor_param = param_ptr->cmd_value;
		break;
	case COM_SN_SET_IMAGE_EFFECT:
		cmd = SENSOR_IMAGE_EFFECT;
		sensor_param = param_ptr->cmd_value;
		break;
	case COM_SN_SET_EXPOSURE_COMPENSATION:
		cmd = SENSOR_EXPOSURE_COMPENSATION;
		sensor_param = param_ptr->cmd_value;
		break;
	case COM_SN_SET_EXPOSURE_LEVEL:
		cmd = SENSOR_WRITE_EV;
		sensor_param = param_ptr->cmd_value;
		break;
	case COM_SN_SET_WB_MODE:
		cmd = SENSOR_SET_WB_MODE;
		set_exif_flag = 1;
		exif_cmd = SENSOR_EXIF_CTRL_LIGHTSOURCE;
		sensor_param = param_ptr->cmd_value;
		break;
	case COM_SN_SET_PREVIEW_MODE:
		set_exif_flag = 1;
		exif_cmd = SENSOR_EXIF_CTRL_SCENECAPTURETYPE;
		break;
	case COM_SN_SET_ANTI_BANDING:
		cmd = SENSOR_FLIP;
		sensor_param = param_ptr->cmd_value;
		break;
	case COM_SN_SET_ISO:
		cmd = SENSOR_ISO;
		sensor_param = param_ptr->cmd_value;
		break;
	case COM_SN_SET_VIDEO_MODE:
		cmd = SENSOR_VIDEO_MODE;
		sensor_param = param_ptr->cmd_value;
		break;
	case COM_SN_SET_FPS_LLS_MODE:
		break;
	case COM_SN_SET_BEFORE_SNAPSHOT:
		cmd = SENSOR_BEFORE_SNAPSHOT;
		sensor_param = param_ptr->cmd_value;
		break;
	case COM_SN_SET_AFTER_SNAPSHOT:
		cmd = SENSOR_AFTER_SNAPSHOT;
		sensor_param = param_ptr->cmd_value;
		break;
	case COM_SN_SET_EXT_FUNC:
		break;
	case COM_SN_SET_AE_ENABLE://don't support
		break;
	case COM_SN_SET_EXIF_FOCUS:
		sensor_param = param_ptr->cmd_value;
		break;
	case COM_SN_SET_FOCUS:
		cmd = SENSOR_FOCUS;
		sensor_param = (cmr_uint)&(param_ptr->yuv_sn_af_param);
		CMR_LOGE("COM_SN_SET_FOCUS cmd =%ld ",cmd);
		break;
	case COM_SN_GET_PREVIEW_MODE:
		break;
	case COM_SN_GET_CAPTURE_MODE:
		break;
	case COM_SN_GET_SENSOR_ID:
		break;
	case COM_SN_GET_VIDEO_MODE:
		break;
	case COM_SN_GET_EXIF_IMAGE_INFO:
		ret = cmr_sensor_get_exif(cxt->sn_cxt.sensor_handle, cxt->camera_id, &param_ptr->exif_pic_info);
		if (ret) {
			CMR_LOGE("sn get exif image info failed!");
		} else {
			memcpy((void*)&cxt->sn_cxt.exif_info, (void*) &param_ptr->exif_pic_info, sizeof(EXIF_SPEC_PIC_TAKING_COND_T));
		}
		return ret;
	case COM_SN_SET_HDR_EV:
	{
		cmd = SENSOR_SET_HDR_EV;
		hdr_ev_param_ptr = malloc(sizeof(SENSOR_EXT_FUN_PARAM_T));
		if (!hdr_ev_param_ptr) {
			CMR_LOGE("fail to malloc");
			return CMR_CAMERA_NO_MEM;
		}
		hdr_ev_param_ptr->cmd = SENSOR_EXT_EV;
		hdr_ev_param_ptr->param = param_ptr->cmd_value;
		sensor_param = (cmr_uint)hdr_ev_param_ptr;
		CMR_LOGI("vendor hdr ev_val = %d", param_ptr->cmd_value);
		break;
	}
	case COM_SN_GET_INFO:
		ret = cmr_sensor_get_info(cxt->sn_cxt.sensor_handle, cxt->camera_id, &param_ptr->sensor_static_info);
		if (ret) {
			CMR_LOGE("sn get info failed!");
		}
		return ret;
	case COM_SN_GET_FLASH_LEVEL:
		ret = cmr_sensor_get_flash_info(cxt->sn_cxt.sensor_handle, cxt->camera_id, &param_ptr->flash_level);
		if (ret) {
			CMR_LOGE("sn get flash level failed!");
		}
		CMR_LOGI("flash level low_light = %d, high_light = %d",
				 param_ptr->flash_level.low_light, param_ptr->flash_level.high_light);
		return ret;
	case COM_SN_SET_YUV_FPS:
		cmd = SENSOR_FOCUS;
		fps_param_ptr = malloc(sizeof(SENSOR_EXT_FUN_PARAM_T));
		if (!fps_param_ptr) {
			CMR_LOGE("fail to malloc");
			return CMR_CAMERA_NO_MEM;
		}
		fps_param_ptr->cmd = SENSOR_EXT_SET_FPS;
		fps_param_ptr->param = param_ptr->range_fps.min_fps;
		fps_param_ptr->param2 = param_ptr->range_fps.max_fps;
		sensor_param = (cmr_uint)fps_param_ptr;
		CMR_LOGI("COM_SN_SET_YUV_FPS cmd =%ld ",cmd);
		break;
	default:
		CMR_LOGE("don't support cmd %ld", cmd_type);
		ret = CMR_CAMERA_NO_SUPPORT;
		break;
	}
	if (!ret) {
		ret = cmr_sensor_ioctl(cxt->sn_cxt.sensor_handle, cxt->camera_id, cmd, sensor_param);
		if ((COM_SN_SET_HDR_EV == cmd_type) && hdr_ev_param_ptr) {
			free(hdr_ev_param_ptr);
			hdr_ev_param_ptr = 0;
		}
		if (ret) {
			CMR_LOGE("failed to sn ioctrl %ld", ret);
		}
		if (set_exif_flag) {
			CMR_LOGD("ERIC set exif");
			if (cmd_type == COM_SN_SET_WB_MODE){
				cmr_sensor_set_exif(cxt->sn_cxt.sensor_handle, cxt->camera_id, exif_cmd, param_ptr->cmd_value);
				cmr_sensor_set_exif(cxt->sn_cxt.sensor_handle, cxt->camera_id, SENSOR_EXIF_CTRL_WHITEBALANCE, param_ptr->cmd_value);
			} else
				cmr_sensor_set_exif(cxt->sn_cxt.sensor_handle, cxt->camera_id, exif_cmd, sensor_param);
		}
	}
exit:
	return ret;
}

cmr_uint camera_param_to_isp(cmr_uint cmd, struct common_isp_cmd_param *parm)
{
	cmr_uint in_param = parm->cmd_value;
	cmr_uint out_param = in_param;

	switch (cmd) {
	case COM_ISP_SET_AWB_MODE:
		{
			switch (in_param) {
			case CAMERA_WB_AUTO:
				out_param = ISP_AWB_AUTO;
				break;

			case CAMERA_WB_INCANDESCENT:
				out_param = ISP_AWB_INDEX1;
				break;

			case CAMERA_WB_FLUORESCENT:
				out_param = ISP_AWB_INDEX4;
				break;

			case CAMERA_WB_DAYLIGHT:
				out_param = ISP_AWB_INDEX5;
				break;

			case CAMERA_WB_CLOUDY_DAYLIGHT:
				out_param = ISP_AWB_INDEX6;
				break;

			default:
				break;
			}
			break;
		}
	default:
		break;

	}

	return out_param;
}

cmr_int camera_local_get_isp_info(cmr_handle oem_handle, void **addr, int *size)
{
	cmr_int                            ret = CMR_CAMERA_SUCCESS;
	struct camera_context       *cxt = (struct camera_context*)oem_handle;
	struct isp_context             *isp_cxt = &cxt->isp_cxt;
	struct isp_info                   isp_info;

	if (!oem_handle || !addr || !size) {
		CMR_LOGE("err,invlid param");
		return CMR_CAMERA_INVALID_PARAM;
	}
	*addr = 0;
	*size = 0;
#if (defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4))
	if (IMG_DATA_TYPE_RAW == cxt->sn_cxt.sensor_info.image_format) {
		ret = isp_ioctl(isp_cxt->isp_handle, ISP_CTRL_GET_INFO, (void*)&isp_info);
		if (!ret) {
			*addr = isp_info.addr;
			*size = isp_info.size;
		} else {
			CMR_LOGE("fail to get isp information");
		}
	}
#endif
	CMR_LOGI("%p %d", *addr, *size);
	return ret;
}

cmr_int camera_isp_ioctl(cmr_handle oem_handle, cmr_uint cmd_type, struct common_isp_cmd_param *param_ptr)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	cmr_u32                        isp_cmd = ISP_CTRL_MAX;
	cmr_u32                        isp_param = 0;
	void                           *isp_param_ptr = NULL;
	cmr_u32                        ptr_flag = 0;
	cmr_uint                       set_exif_flag = 0;
	cmr_uint                       set_isp_flag = 1;
	SENSOR_EXIF_CTRL_E             exif_cmd = SENSOR_EXIF_CTRL_MAX;
	struct isp_context             *isp_cxt = &cxt->isp_cxt;
	struct isp_pos_rect            trim;
	struct isp_range_fps           isp_fps;
	struct isp_ae_fps              ae_fps;

	if (!oem_handle || !param_ptr) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	switch (cmd_type) {
	case COM_ISP_SET_AE_MODE:
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
		isp_cmd = ISP_CTRL_SET_AE_NIGHT_MODE;
#else
		isp_cmd = ISP_CTRL_AE_MODE;
#endif
		set_exif_flag = 1;
		exif_cmd = SENSOR_EXIF_CTRL_SCENECAPTURETYPE;
		isp_param = param_ptr->cmd_value;
		if (ISP_AE_MODE_MAX == isp_param) {
			set_isp_flag = 0;
		}
		CMR_LOGI("ae mode %d", param_ptr->cmd_value);
		break;
	case COM_ISP_SET_AE_MEASURE_LUM:
		isp_cmd = ISP_CTRL_AE_MEASURE_LUM;
		isp_param = param_ptr->cmd_value;
		CMR_LOGI("aw measure lum %d", param_ptr->cmd_value);
		break;
	case COM_ISP_SET_AE_METERING_AREA:
		isp_cmd = ISP_CTRL_AE_TOUCH;
		trim.start_x = param_ptr->win_area.rect[0].start_x;
		trim.start_y = param_ptr->win_area.rect[0].start_y;
		trim.end_x = param_ptr->win_area.rect[0].start_x + param_ptr->win_area.rect[0].width;
		trim.end_y = param_ptr->win_area.rect[0].start_y + param_ptr->win_area.rect[0].height;
		ptr_flag = 1;
		isp_param_ptr = (void *)&trim;
		break;
	case COM_ISP_SET_BRIGHTNESS:
		isp_cmd = ISP_CTRL_BRIGHTNESS;
		set_exif_flag = 1;
		exif_cmd = SENSOR_EXIF_CTRL_BRIGHTNESSVALUE;
		isp_param = param_ptr->cmd_value;
		CMR_LOGI("brightness %d", param_ptr->cmd_value);
		break;
	case COM_ISP_SET_CONTRAST:
		isp_cmd = ISP_CTRL_CONTRAST;
		set_exif_flag = 1;
		exif_cmd = SENSOR_EXIF_CTRL_CONTRAST;
		isp_param = param_ptr->cmd_value;
		CMR_LOGI("contrast %d", param_ptr->cmd_value);
		break;
	case COM_ISP_SET_SATURATION:
		isp_cmd = ISP_CTRL_SATURATION;
		set_exif_flag = 1;
		exif_cmd = SENSOR_EXIF_CTRL_SATURATION;
		isp_param = param_ptr->cmd_value;
		CMR_LOGI("saturation %d", param_ptr->cmd_value);
		break;
	case COM_ISP_SET_SHARPNESS:
		isp_cmd = ISP_CTRL_SHARPNESS;
		set_exif_flag = 1;
		exif_cmd = SENSOR_EXIF_CTRL_SHARPNESS;
		isp_param = param_ptr->cmd_value;
		CMR_LOGI("sharpness %d", param_ptr->cmd_value);
		break;
	case COM_ISP_SET_SPECIAL_EFFECT:
		isp_cmd = ISP_CTRL_SPECIAL_EFFECT;
		isp_param = param_ptr->cmd_value;
		CMR_LOGI("effect %d", param_ptr->cmd_value);
		break;
	case COM_ISP_SET_EV:
		isp_cmd = ISP_CTRL_EV;
		isp_param = param_ptr->cmd_value;
		CMR_LOGI("ev %d", param_ptr->cmd_value);
		break;
	case COM_ISP_SET_AWB_MODE:
		CMR_LOGI("awb mode 00 %d isp param %d", param_ptr->cmd_value, isp_param);
		isp_cmd = ISP_CTRL_AWB_MODE;
		set_exif_flag = 1;
		exif_cmd = SENSOR_EXIF_CTRL_LIGHTSOURCE;
		isp_param = camera_param_to_isp(COM_ISP_SET_AWB_MODE, param_ptr);
		CMR_LOGI("awb mode %d isp param %d", param_ptr->cmd_value, isp_param);
		break;
	case COM_ISP_SET_ANTI_BANDING:
		isp_cmd = ISP_CTRL_FLICKER;
		isp_param = param_ptr->cmd_value;
		CMR_LOGI("flicker %d", param_ptr->cmd_value);
		break;
	case COM_ISP_SET_ISO:
		isp_cmd = ISP_CTRL_ISO;
		set_exif_flag = 1;
		exif_cmd = SENSOR_EXIF_CTRL_ISOSPEEDRATINGS;
		isp_param = param_ptr->cmd_value;
		CMR_LOGI("iso %d", param_ptr->cmd_value);
		break;
	case COM_ISP_SET_VIDEO_MODE:
		isp_cmd = ISP_CTRL_VIDEO_MODE;
		isp_param = param_ptr->cmd_value;
		CMR_LOGI("isp video mode %d", param_ptr->cmd_value);
		break;
	case COM_ISP_SET_FLASH_EG:
		isp_cmd = ISP_CTRL_FLASH_EG;
		isp_param = param_ptr->cmd_value;
		break;
	case COM_ISP_GET_EXIF_IMAGE_INFO:
		isp_cmd = ISP_CTRL_GET_EXIF_INFO;
		ptr_flag = 1;
		isp_param_ptr = (void*)&param_ptr->exif_pic_info;
		break;
	case COM_ISP_SET_AF:
		isp_cmd = ISP_CTRL_AF;
		ptr_flag = 1;
		isp_param_ptr = (void*)&param_ptr->af_param;
		CMR_LOGI("isp_cmd =%d", isp_cmd);
		break;
	case COM_ISP_SET_AF_MODE:
		isp_cmd = ISP_CTRL_AF_MODE;
		isp_param = param_ptr->cmd_value;
		CMR_LOGI("af mode %d", param_ptr->cmd_value);
		break;
	case COM_ISP_SET_AF_STOP:
		isp_cmd = ISP_CTRL_AF_STOP;
		isp_param = param_ptr->cmd_value;
		CMR_LOGI("isp_cmd =%d af mode %d", isp_cmd,param_ptr->cmd_value);
		break;
	case COM_ISP_GET_LOW_LUX_EB:
		isp_cmd = ISP_LOW_LUX_EB;
		isp_param_ptr = (void*)&param_ptr->cmd_value;

		ret = isp_capability(isp_cxt->isp_handle, isp_cmd, isp_param_ptr);
		if (ret) {
			CMR_LOGE("Failed to read isp capability ret = %ld", ret);
		}
		return ret;
	case COM_ISP_SET_FLASH_NOTICE:
		isp_cmd = ISP_CTRL_FLASH_NOTICE;
		ptr_flag = 1;
		isp_param_ptr = (void*)&param_ptr->flash_notice;
		CMR_LOGI("isp_cmd = %d, mode = %d", isp_cmd, param_ptr->flash_notice.mode);
		break;
	case COM_ISP_SET_FACE_AREA:
		isp_cmd = ISP_CTRL_FACE_AREA;
		ptr_flag = 1;
		isp_param_ptr = (void*)&param_ptr->fd_param;
		CMR_LOGI("isp_cmd = %d, face_num = %d", isp_cmd, param_ptr->fd_param.face_num);
		break;
	case COM_ISP_SET_RANGE_FPS:
		isp_cmd = ISP_CTRL_RANGE_FPS;
		ptr_flag = 1;
		isp_fps.min_fps = param_ptr->range_fps.min_fps;
		isp_fps.max_fps = param_ptr->range_fps.max_fps;
		isp_param_ptr =	(void*)&isp_fps;
		break;

    case COM_ISP_SET_FPS_LLS_MODE:
		isp_cmd = ISP_CTRL_SET_AE_FPS;
		ae_fps = param_ptr->fps_param;
		ptr_flag = 1;
		isp_param_ptr = (void *)&ae_fps;
		break;

	case COM_ISP_SET_HDR:
		isp_cmd = ISP_CTRL_HDR;
		isp_param = param_ptr->cmd_value;
		break;

	default:
		CMR_LOGE("don't support cmd %ld", cmd_type);
		ret = CMR_CAMERA_NO_SUPPORT;
		break;
	}

	if (ptr_flag) {
		ret = isp_ioctl(isp_cxt->isp_handle, isp_cmd, isp_param_ptr);
		if (ret) {
			CMR_LOGE("failed isp ioctl %ld", ret);
		}
		CMR_LOGI("done %ld and direct return", ret);
		return ret;
	}

	if (set_isp_flag) {
		ret = isp_ioctl(isp_cxt->isp_handle, isp_cmd, (void*)&isp_param);
		if (ret) {
			CMR_LOGE("failed isp ioctl %ld", ret);
		} else {
			if (COM_ISP_SET_ISO == cmd_type) {
				if (0 == param_ptr->cmd_value) {
					isp_capability(isp_cxt->isp_handle, ISP_CUR_ISO,(void *)&isp_param);
					cxt->setting_cxt.is_auto_iso = 1;
				} else {
					cxt->setting_cxt.is_auto_iso = 0;
				}
				isp_param = POWER2(isp_param-1) * ONE_HUNDRED;
				CMR_LOGI("auto iso %d, exif iso %d", cxt->setting_cxt.is_auto_iso, isp_param);
			}
		}
	}

	if (set_exif_flag) {
		CMR_LOGD("ERIC set exif");
		if (COM_ISP_SET_AWB_MODE == cmd_type) {
			cmr_sensor_set_exif(cxt->sn_cxt.sensor_handle, cxt->camera_id, exif_cmd, param_ptr->cmd_value);
			cmr_sensor_set_exif(cxt->sn_cxt.sensor_handle, cxt->camera_id, SENSOR_EXIF_CTRL_WHITEBALANCE, param_ptr->cmd_value);
		} else {
			cmr_sensor_set_exif(cxt->sn_cxt.sensor_handle, cxt->camera_id, exif_cmd, isp_param);
		}
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

void camera_get_iso_value(cmr_handle oem_handle)
{
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	cmr_u32                        isp_param = 0;
	if (cxt->isp_cxt.is_work && (1 == cxt->setting_cxt.is_auto_iso)) {
		struct isp_context			   *isp_cxt = &cxt->isp_cxt;
		isp_capability(isp_cxt->isp_handle, ISP_CUR_ISO,(void *)&isp_param);

		cmr_sensor_set_exif(cxt->sn_cxt.sensor_handle, cxt->camera_id, SENSOR_EXIF_CTRL_ISOSPEEDRATINGS, isp_param);
	}
}

cmr_int camera_get_ae_lum_value(cmr_handle oem_handle)
{
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	cmr_int                        lum_val = 0;
	struct isp_context			   *isp_cxt = &cxt->isp_cxt;
	isp_capability(isp_cxt->isp_handle, ISP_CTRL_GET_AE_LUM,(void *)&lum_val);

	return lum_val;
}

cmr_int camera_get_setting_activity(cmr_handle oem_handle, cmr_uint *is_active)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;

	if (!oem_handle || !is_active) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	*is_active = cxt->setting_cxt.is_active;
	CMR_LOGI("%ld", (cmr_uint)*is_active);
exit:
	return ret;
}

cmr_int camera_get_preview_param(cmr_handle oem_handle, enum takepicture_mode mode,
	                                               cmr_uint is_snapshot, struct preview_param *out_param_ptr)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	struct preview_context         *prev_cxt = &cxt->prev_cxt;
	struct setting_context         *setting_cxt = &cxt->setting_cxt;
	struct jpeg_context            *jpeg_cxt = &cxt->jpeg_cxt;
	struct snapshot_context        *snp_cxt = &cxt->snp_cxt;
	struct setting_cmd_parameter   setting_param;
	cmr_u32                        is_cfg_snp = 0;
	cmr_u32                        rotation = 0;

	setting_param.camera_id = cxt->camera_id;
	cmr_bzero((void*)out_param_ptr, sizeof(*out_param_ptr));

	out_param_ptr->memory_setting.alloc_mem = camera_malloc;
	out_param_ptr->memory_setting.free_mem = camera_free;

	if (CAMERA_SNAPSHOT != is_snapshot) {
		ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_PREVIEW_FORMAT, &setting_param);
		if (ret) {
			CMR_LOGE("failed to get preview fmt %ld", ret);
			goto exit;
		}
		out_param_ptr->preview_fmt = setting_param.cmd_type_value;
		out_param_ptr->is_fd_on = cxt->fd_on_off;
		out_param_ptr->preview_eb = 1;
		out_param_ptr->is_support_fd = cxt->is_support_fd;
		out_param_ptr->is_lls_enable = cxt->is_lls_enable;
		/*get prev rot*/
		ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_PREVIEW_ANGLE, &setting_param);
		if (ret) {
			CMR_LOGE("failed to get preview angle %ld", ret);
			goto exit;
		}
		out_param_ptr->prev_rot = setting_param.cmd_type_value;
		/*get prev size*/
		ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_PREVIEW_SIZE, &setting_param);
		if (ret) {
			CMR_LOGE("failed to get prev size %ld", ret);
			goto exit;
		}
		prev_cxt->size = setting_param.size_param;
		out_param_ptr->preview_size = prev_cxt->size;
		/*get video size*/
		ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_VIDEO_SIZE, &setting_param);
		if (ret) {
			CMR_LOGE("failed to get video size %ld", ret);
			goto exit;
		}
		prev_cxt->video_size = setting_param.size_param;
		out_param_ptr->video_size = prev_cxt->video_size;

		/*get dv mode*/
		ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_DV_MODE, &setting_param);
		if (ret) {
			CMR_LOGE("failed to get dv mode %ld", ret);
			goto exit;
		}
		out_param_ptr->is_dv = setting_param.cmd_type_value;
		if ((0 != out_param_ptr->video_size.width) && (0 != out_param_ptr->video_size.height)) {
			out_param_ptr->video_eb = 1;
		}
		if (CAMERA_ZSL_MODE == mode) {
			is_cfg_snp = 1;
		}
	} else {
		is_cfg_snp = 1;
	}
	ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_CAPTURE_FORMAT, &setting_param);
	if (ret) {
		CMR_LOGE("failed to get cap fmt %ld", ret);
		goto exit;
	}
	out_param_ptr->cap_fmt = setting_param.cmd_type_value;
	/*get zoom param*/
	ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_ZOOM_PARAM, &setting_param);
	if (ret) {
		CMR_LOGE("failed to get zoom param %ld", ret);
		goto exit;
	}
	out_param_ptr->zoom_setting = setting_param.zoom_param;
	cxt->snp_cxt.snp_mode = mode;
	if (0 == is_cfg_snp) {
		CMR_LOGI("don't need cfg snp");
		goto exit;
	}
	out_param_ptr->snapshot_eb = is_cfg_snp;
	//out_param_ptr->flip_on = cxt->flip_on;

	out_param_ptr->isp_width_limit = cxt->isp_cxt.width_limit;
	out_param_ptr->tool_eb = 0;
	if (CAMERA_ISP_TUNING_MODE == mode || CAMERA_UTEST_MODE == mode || CAMERA_AUTOTEST_MODE == mode) {
		out_param_ptr->tool_eb = 1;
	}
	/*get snapshot size*/
	ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_CAPTURE_SIZE, &setting_param);
	if (ret) {
		CMR_LOGE("failed to get capture size %ld", ret);
		goto exit;
	}
	out_param_ptr->picture_size = setting_param.size_param;
	cxt->snp_cxt.request_size = setting_param.size_param;
	/*get snapshot angle*/
	ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_CAPTURE_ANGLE, &setting_param);
	if (ret) {
		CMR_LOGE("failed to get capture rot %ld", ret);
		goto exit;
	}
	out_param_ptr->cap_rot = setting_param.cmd_type_value;
	/*get rotation snapshot cfg*/
	ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_ROTATION_CAPTURE, &setting_param);
	if (ret) {
		CMR_LOGE("failed to get capture rot %ld", ret);
		goto exit;
	}
	out_param_ptr->is_cfg_rot_cap = setting_param.cmd_type_value;
	cxt->snp_cxt.is_cfg_rot_cap = out_param_ptr->is_cfg_rot_cap;

	/*get hdr flag*/
	ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_HDR, &setting_param);
	if (ret) {
		CMR_LOGE("failed to get envir %ld", ret);
		goto exit;
	}
	camera_set_hdr_flag(cxt, setting_param.cmd_type_value);
	out_param_ptr->is_hdr = setting_param.cmd_type_value;
	/*get android zsl flag*/
	ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_ANDROID_ZSL_FLAG, &setting_param);
	if (ret) {
		CMR_LOGE("failed to get android zsl %ld", ret);
		goto exit;
	}
	cxt->is_android_zsl = setting_param.cmd_type_value;
	/*get jpeg param*/
	ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_JPEG_QUALITY, &setting_param);
	if (ret) {
		CMR_LOGE("failed to get image quality %ld", ret);
		goto exit;
	}
	jpeg_cxt->param.quality = setting_param.cmd_type_value;

	ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_THUMB_QUALITY, &setting_param);
	if (ret) {
		CMR_LOGE("failed to get thumb quality %ld", ret);
		goto exit;
	}
	jpeg_cxt->param.thumb_quality = setting_param.cmd_type_value;

	ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_THUMB_SIZE, &setting_param);
	if (ret) {
		CMR_LOGE("failed to get thumb size %ld", ret);
		goto exit;
	}
	jpeg_cxt->param.thum_size = setting_param.size_param;
	out_param_ptr->thumb_size = jpeg_cxt->param.thum_size;

	ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_ENCODE_ANGLE, &setting_param);
	if (ret) {
		CMR_LOGE("failed to get enc angle %ld", ret);
		goto exit;
	}
	rotation = setting_param.cmd_type_value;
	jpeg_cxt->param.set_encode_rotation = setting_param.cmd_type_value;
	if (cxt->snp_cxt.is_cfg_rot_cap) {
		cxt->snp_cxt.cfg_cap_rot = jpeg_cxt->param.set_encode_rotation;
	} else {
		cxt->snp_cxt.cfg_cap_rot = 0;
	}

	ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_SHOT_NUMBER, &setting_param);
	if (ret) {
		CMR_LOGE("failed to get shot num %ld", ret);
		goto exit;
	}
	cxt->snp_cxt.total_num = setting_param.cmd_type_value;

#ifdef CONFIG_SPRD_PRIVATE_ZSL
	ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_SPRD_ZSL_ENABLED, &setting_param);
	if (ret) {
		CMR_LOGE("failed to get preview sprd zsl enabled flag %ld", ret);
		goto exit;
	}
	out_param_ptr->sprd_zsl_enabled = setting_param.cmd_type_value;
	CMR_LOGI("sprd zsl_enabled flag %d", out_param_ptr->sprd_zsl_enabled);
#endif

        /*for bug500099*/
        ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, SETTING_GET_FLIP_ON, &setting_param);
        if (ret) {
		CMR_LOGE("failed to get preview sprd flip_on enabled flag %ld", ret);
		goto exit;
        }
	out_param_ptr->flip_on = setting_param.cmd_type_value;
	if (out_param_ptr->flip_on) {
		CMR_LOGI("encode_angle:%d, flip:%d", rotation, out_param_ptr->flip_on);
		if (IMG_ANGLE_90 == rotation || IMG_ANGLE_270 == rotation) {
			out_param_ptr->flip_on = 0x1; // flip
		} else if (IMG_ANGLE_0 == rotation || IMG_ANGLE_180 == rotation) {
			out_param_ptr->flip_on = 0x3; // mirror
		}
	}
        //bug500099 front cam mirror end

	if (1 == camera_get_hdr_flag(cxt)) {
		struct ipm_open_in  in_param;
		struct ipm_open_out out_param;
		in_param.frame_size.width = CAMERA_ALIGNED_16(cxt->snp_cxt.request_size.width);
		in_param.frame_size.height = CAMERA_ALIGNED_16(cxt->snp_cxt.request_size.height);
		in_param.frame_rect.start_x = 0;
		in_param.frame_rect.start_y = 0;
		in_param.frame_rect.width = in_param.frame_size.width;
		in_param.frame_rect.height = in_param.frame_size.height;
		in_param.reg_cb = camera_ipm_cb;
		ret = camera_open_hdr(cxt, &in_param, &out_param);
		if (ret) {
			CMR_LOGE("failed to open hdr %ld", ret);
			goto exit;
		} else {
			cxt->ipm_cxt.hdr_num = out_param.total_frame_number;
			CMR_LOGI("get hdr num %d", cxt->ipm_cxt.hdr_num);
		}
	}
	if (CAMERA_ZSL_MODE == mode) {
		out_param_ptr->frame_count = FRAME_NUM_MAX;
		out_param_ptr->frame_ctrl = FRAME_CONTINUE;
	} else {
		if (camera_get_hdr_flag(cxt)) {
			out_param_ptr->frame_count = cxt->ipm_cxt.hdr_num;
			out_param_ptr->frame_ctrl = FRAME_IMAGE_PROC;
		} else {
			out_param_ptr->frame_count = cxt->snp_cxt.total_num;
			if (1 == cxt->snp_cxt.total_num) {
				out_param_ptr->frame_ctrl = FRAME_STOP;
			} else {
				out_param_ptr->frame_ctrl = FRAME_CONTINUE;
			}
		}
	}
#if defined(CONFIG_CAMERA_FLASH_HIGH_AE_MEASURE)
	setting_param.ctrl_flash.capture_mode.capture_mode= snp_cxt->snp_mode;
	ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, SETTING_GET_FLASH_STATUS, &setting_param);
	if (ret) {
			CMR_LOGE("failed to get flash mode %ld", ret);
	}
	CMR_LOGI("setting_param.cmd_type_value = %d", setting_param.cmd_type_value);
	if (!ret && setting_param.cmd_type_value) {
		out_param_ptr->frame_count = FRAME_FLASH_MAX;
		out_param_ptr->frame_ctrl = FRAME_CONTINUE;
	}
#endif
exit:
	CMR_LOGI("prev size %d %d pic size %d %d", out_param_ptr->preview_size.width, out_param_ptr->preview_size.height,
		     out_param_ptr->picture_size.width, out_param_ptr->picture_size.height);
	CMR_LOGI("video size %d %d android zsl flag %d", out_param_ptr->video_size.width, out_param_ptr->video_size.height, cxt->is_android_zsl);
	CMR_LOGI("prev rot %ld snp rot %d rot snp %d", out_param_ptr->prev_rot, out_param_ptr->cap_rot, out_param_ptr->is_cfg_rot_cap);
	CMR_LOGI("zoom mode %ld fd %ld is dv %d tool eb %d", out_param_ptr->zoom_setting.mode, out_param_ptr->is_fd_on,
			out_param_ptr->is_dv, out_param_ptr->tool_eb);
	CMR_LOGI("q %d thumb q %d enc angle %d thumb size %d %d", jpeg_cxt->param.quality,
		     jpeg_cxt->param.thumb_quality, jpeg_cxt->param.set_encode_rotation,
		     jpeg_cxt->param.thum_size.width, jpeg_cxt->param.thum_size.height);
	CMR_LOGI("frame cnt %d", out_param_ptr->frame_count);

	return ret;
}

cmr_int camera_set_preview_param(cmr_handle oem_handle, enum takepicture_mode mode,  cmr_uint is_snapshot)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	struct preview_context         *prev_cxt = &cxt->prev_cxt;
	struct snapshot_context        *snp_cxt = &cxt->snp_cxt;
	struct setting_context         *setting_cxt = &cxt->setting_cxt;
	struct preview_param           start_param;
	struct preview_out_param       preview_out;

	if (1 != prev_cxt->inited) {
		CMR_LOGE("err, don't init preview");
		ret = -CMR_CAMERA_INVALID_STATE;
		goto exit;
	}
	if (PREVIEWING == cmr_preview_get_status(cxt->prev_cxt.preview_handle, cxt->camera_id)) {
		CMR_LOGI("prev has been started");
		goto exit;
	}

	ret = camera_get_preview_param(oem_handle, mode, is_snapshot, &start_param);
	if (ret) {
		CMR_LOGE("failed to get prev param %ld", ret);
		goto exit;
	}

	cmr_bzero(&preview_out, sizeof(struct preview_out_param));
	ret = cmr_preview_set_param(prev_cxt->preview_handle, cxt->camera_id, &start_param, &preview_out);
	if (ret) {
		CMR_LOGE("failed to set prev param %ld", ret);
		goto exit;
	}
	prev_cxt->preview_sn_mode = preview_out.preview_sn_mode;
	prev_cxt->channel_bits = preview_out.preview_chn_bits;
	prev_cxt->data_endian = preview_out.preview_data_endian;
	prev_cxt->video_sn_mode = preview_out.video_sn_mode;
	prev_cxt->video_channel_bits = preview_out.video_chn_bits;
	prev_cxt->actual_video_size = preview_out.actual_video_size;
	prev_cxt->video_data_endian = preview_out.video_data_endian;
	snp_cxt->snapshot_sn_mode = preview_out.snapshot_sn_mode;
	snp_cxt->channel_bits = preview_out.snapshot_chn_bits;
	snp_cxt->post_proc_setting = preview_out.post_proc_setting;
	snp_cxt->data_endian = preview_out.snapshot_data_endian;
	snp_cxt->zsl_frame = preview_out.zsl_frame;
	snp_cxt->actual_capture_size = preview_out.actual_snapshot_size;
	cmr_copy((void*)&snp_cxt->post_proc_setting, (void*)&preview_out.post_proc_setting, sizeof(snp_cxt->post_proc_setting));
	CMR_LOGI("prev mode %d prev chn %d snp mode %d snp chn %d",
		      prev_cxt->preview_sn_mode, prev_cxt->channel_bits, snp_cxt->snapshot_sn_mode, snp_cxt->channel_bits);
	CMR_LOGI("rot angle %ld", snp_cxt->post_proc_setting.rot_angle);

exit:
	return ret;
}

cmr_int camera_get_snapshot_param(cmr_handle oem_handle, struct snapshot_param *out_ptr)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	struct jpeg_context            *jpeg_cxt = &cxt->jpeg_cxt;
	struct setting_context         *setting_cxt = &cxt->setting_cxt;
	struct setting_cmd_parameter   setting_param;
	cmr_int                        i;
	cmr_u32                        chn_bits = cxt->snp_cxt.channel_bits;

	out_ptr->total_num = 0;
	out_ptr->rot_angle = 0;
	out_ptr->hdr_handle = cxt->ipm_cxt.hdr_handle;
	out_ptr->hdr_need_frm_num = cxt->ipm_cxt.hdr_num;
	out_ptr->post_proc_setting.data_endian = cxt->snp_cxt.data_endian;
	out_ptr->lls_shot_mode = cxt->lls_shot_mode;
	out_ptr->is_vendor_hdr = cxt->is_vendor_hdr;
	setting_param.camera_id = cxt->camera_id;
	ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_HDR, &setting_param);
	if (ret) {
		CMR_LOGE("failed to get envir %ld", ret);
		goto exit;
	}
	camera_set_hdr_flag(cxt, setting_param.cmd_type_value);

	ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_SHOT_NUMBER, &setting_param);
	if (ret) {
		CMR_LOGE("failed to get shot num %ld", ret);
		goto exit;
	}
	out_ptr->total_num = setting_param.cmd_type_value;
	cxt->snp_cxt.total_num = out_ptr->total_num;

	/*get jpeg param*/
	ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_JPEG_QUALITY, &setting_param);
	if (ret) {
		CMR_LOGE("failed to get image quality %ld", ret);
		goto exit;
	}
	jpeg_cxt->param.quality = setting_param.cmd_type_value;

	ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_THUMB_QUALITY, &setting_param);
	if (ret) {
		CMR_LOGE("failed to get thumb quality %ld", ret);
		goto exit;
	}
	jpeg_cxt->param.thumb_quality = setting_param.cmd_type_value;

	ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_THUMB_SIZE, &setting_param);
	if (ret) {
		CMR_LOGE("failed to get thumb size %ld", ret);
		goto exit;
	}
	jpeg_cxt->param.thum_size = setting_param.size_param;

	ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, SETTING_GET_FLIP_ON, &setting_param);
	if (ret) {
		CMR_LOGE("failed to get preview sprd flip_on enabled flag %ld", ret);
		goto exit;
	}
	out_ptr->flip_on = setting_param.cmd_type_value;

	ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_ENCODE_ANGLE, &setting_param);
	if (ret) {
		CMR_LOGE("failed to get enc angle %ld", ret);
		goto exit;
	}
	out_ptr->rot_angle = setting_param.cmd_type_value;
	jpeg_cxt->param.set_encode_rotation = setting_param.cmd_type_value;
	ret = cmr_preview_get_post_proc_param(cxt->prev_cxt.preview_handle, cxt->camera_id,
									(cmr_u32)setting_param.cmd_type_value, &out_ptr->post_proc_setting);
	if (ret) {
		CMR_LOGE("failed to get rot angle %ld", ret);
		goto exit;
	}
	out_ptr->camera_id = cxt->camera_id;
	out_ptr->is_hdr = camera_get_hdr_flag(cxt);
	out_ptr->is_android_zsl = cxt->is_android_zsl;
	out_ptr->mode = cxt->snp_cxt.snp_mode;
	out_ptr->is_cfg_rot_cap = cxt->snp_cxt.is_cfg_rot_cap;
	out_ptr->jpeg_setting = jpeg_cxt->param;
	out_ptr->req_size = cxt->snp_cxt.request_size;
	if ((0 != cxt->prev_cxt.video_size.width) && (0 != cxt->prev_cxt.video_size.height)) {
		chn_bits = cxt->prev_cxt.video_channel_bits;
		out_ptr->is_video_snapshot = 1;
	} else if (CAMERA_ZSL_MODE == cxt->snp_cxt.snp_mode) {
		out_ptr->is_zsl_snapshot = 1;
	}
	CMR_LOGI("chn_bits %d actual size %d %d", chn_bits, out_ptr->post_proc_setting.actual_snp_size.width,
			out_ptr->post_proc_setting.actual_snp_size.height);
	out_ptr->channel_id = GRAB_CHANNEL_MAX+1;
	for ( i=0 ; i<GRAB_CHANNEL_MAX ; i++) {
		if (chn_bits & (1 << i)) {
			out_ptr->channel_id = i;
			break;
		}
	}

exit:
	CMR_LOGI("done,total num %d enc angle %d", out_ptr->total_num, out_ptr->rot_angle);
	return ret;
}

cmr_int camera_set_setting(cmr_handle oem_handle, enum camera_param_type id, cmr_uint param)
{
	cmr_int                          ret = CMR_CAMERA_SUCCESS;
	struct camera_context            *cxt = (struct camera_context*)oem_handle;
	struct setting_cmd_parameter     setting_param;

	setting_param.camera_id = cxt->camera_id;
	switch (id) {
	case CAMERA_PARAM_ZOOM:
		if (param) {
			setting_param.zoom_param = *((struct cmr_zoom_param*)param);
			ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		} else {
			CMR_LOGE("err, zoom param is null");
			ret = -CMR_CAMERA_INVALID_PARAM;
		}
		break;
	case CAMERA_PARAM_ENCODE_ROTATION:
		setting_param.cmd_type_value = param;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		break;
	case CAMERA_PARAM_CONTRAST:
		setting_param.cmd_type_value = param;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		break;
	case CAMERA_PARAM_BRIGHTNESS:
		setting_param.cmd_type_value = param;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		break;
	case CAMERA_PARAM_SHARPNESS:
		setting_param.cmd_type_value = param;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		break;
	case CAMERA_PARAM_WB:
		setting_param.cmd_type_value = param;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		break;
	case CAMERA_PARAM_EFFECT:
		setting_param.cmd_type_value = param;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		break;
	case CAMERA_PARAM_FLASH:
	case CAMERA_PARAM_ISP_FLASH:
		setting_param.cmd_type_value = param;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		break;
	case CAMERA_PARAM_ANTIBANDING:
		setting_param.cmd_type_value = param;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		break;
	case CAMERA_PARAM_AUTO_EXPOSURE_MODE:
		setting_param.ae_param = *(struct cmr_ae_param*)param;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		break;
	case CAMERA_PARAM_ISO:
		setting_param.cmd_type_value = param;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		break;
	case CAMERA_PARAM_EXPOSURE_COMPENSATION:
		setting_param.cmd_type_value = param;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		break;
	case CAMERA_PARAM_PREVIEW_FPS:
		if (param) {
			setting_param.preview_fps_param = *(struct cmr_preview_fps_param*)param;
			ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		} else {
			CMR_LOGE("err, fps param is null");
			ret = -CMR_CAMERA_INVALID_PARAM;
		}
		break;

	case CAMERA_PARAM_PREVIEW_LLS_FPS:
		if (param) {
			/*to do */
			//use the cmr_preview_fps_param temporaily, to be replaced later
			setting_param.preview_fps_param = *(struct cmr_preview_fps_param*)param;
			setting_param.cmd_type_value = setting_param.preview_fps_param.frame_rate;
			ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		} else {
			CMR_LOGE("err, fps param is null");
			ret = -CMR_CAMERA_INVALID_PARAM;
		}
		break;

	case CAMERA_PARAM_SATURATION:
		setting_param.cmd_type_value = param;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		break;
	case CAMERA_PARAM_SCENE_MODE:
		setting_param.cmd_type_value = param;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		break;
	case CAMERA_PARAM_JPEG_QUALITY:
		setting_param.cmd_type_value = param;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		break;
	case CAMERA_PARAM_THUMB_QUALITY:
		setting_param.cmd_type_value = param;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		break;
	case CAMERA_PARAM_SENSOR_ORIENTATION:
		setting_param.cmd_type_value = param;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		break;
	case CAMERA_PARAM_FOCAL_LENGTH:
		setting_param.cmd_type_value = param;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		break;
	case CAMERA_PARAM_SENSOR_ROTATION:
		setting_param.cmd_type_value = param;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		break;
	case CAMERA_PARAM_PERFECT_SKIN_LEVEL:
		setting_param.cmd_type_value = param;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		break;
        case CAMERA_PARAM_FLIP_ON:
                setting_param.cmd_type_value = param;
                ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
                break;
	case CAMERA_PARAM_SHOT_NUM:
		setting_param.cmd_type_value = param;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		break;
	case CAMERA_PARAM_ROTATION_CAPTURE:
		setting_param.cmd_type_value = param;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		break;
	case CAMERA_PARAM_POSITION:
		if (param) {
			setting_param.position_info = *(struct camera_position_type*)param;
			ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		} else {
			CMR_LOGE("err, postition param is null");
			ret = -CMR_CAMERA_INVALID_PARAM;
		}
		break;
	case CAMERA_PARAM_PREVIEW_SIZE:
		if (param) {
			setting_param.size_param = *(struct img_size*)param;
			ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		} else {
			CMR_LOGE("err, prev size param is null");
			ret = -CMR_CAMERA_INVALID_PARAM;
		}
		break;
	case CAMERA_PARAM_PREVIEW_FORMAT:
		setting_param.cmd_type_value = param;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		break;
	case CAMERA_PARAM_CAPTURE_SIZE:
		if (param) {
			setting_param.size_param = *(struct img_size*)param;
			ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		} else {
			CMR_LOGE("err, capture size param is null");
			ret = -CMR_CAMERA_INVALID_PARAM;
		}
		break;
	case CAMERA_PARAM_CAPTURE_FORMAT:
		setting_param.cmd_type_value = param;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		break;
	case CAMERA_PARAM_CAPTURE_MODE:
		setting_param.cmd_type_value = param;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		break;
	case CAMERA_PARAM_THUMB_SIZE:
		if (param) {
			setting_param.size_param = *(struct img_size*)param;
			ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		} else {
			CMR_LOGE("err, thumb size param is null");
			ret = -CMR_CAMERA_INVALID_PARAM;
		}
		break;
	case CAMERA_PARAM_VIDEO_SIZE:
		if (param) {
			setting_param.size_param = *(struct img_size*)param;
			ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		} else {
			CMR_LOGE("err, video size param is null");
			ret = -CMR_CAMERA_INVALID_PARAM;
		}
		break;
	case CAMERA_PARAM_RANGE_FPS:
		if (param) {
			setting_param.range_fps = *(struct cmr_range_fps_param*)param;
			ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		} else {
			CMR_LOGE("err, range fps param is null");
			ret = -CMR_CAMERA_INVALID_PARAM;
		}
		break;
#ifdef CONFIG_SPRD_PRIVATE_ZSL
	case CAMERA_PARAM_SPRD_ZSL_ENABLED:
		setting_param.cmd_type_value = param;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, id, &setting_param);
		break;
#endif
	default:
		CMR_LOGI("don't support %d", id);
	}
	CMR_LOGV("done %ld", ret);
	return ret;
}

cmr_int camera_restart_rot(cmr_handle oem_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;

	if (!oem_handle) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	ret = camera_rotation_deinit(oem_handle);
	if (ret) {
		CMR_LOGE("failed to de-init rotate %ld", ret);
	} else {
		ret = camera_rotation_init(oem_handle);
		if (ret) {
			CMR_LOGE("failed to initizalize rot");
		}
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

void getLogLevel()
{
	char prop[PROPERTY_VALUE_MAX];
	int val = 0;

	property_get("persist.sys.camera.hal.log", prop, "0");
	val = atoi(prop);
	if (0 < val) {
		gCMRLogLevel = (uint32_t)val;
	}
}

/*****************************************external function*****************************************/

cmr_int camera_local_int(cmr_u32 camera_id, camera_cb_of_type callback,
						          void *client_data, cmr_uint is_autotest, cmr_handle *oem_handle
						          , void* cb_of_malloc, void* cb_of_free)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = NULL;

	if (!oem_handle) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	getLogLevel();

	*oem_handle = (cmr_handle)0;

	cxt = (struct camera_context*)malloc(sizeof(struct camera_context));
	if (NULL == cxt) {
		CMR_LOGE("failed to create context");
		ret = -CMR_CAMERA_NO_MEM;
		goto exit;
	}
	cmr_bzero(cxt, sizeof(*cxt));
	cxt->camera_id = camera_id;
	cxt->camera_cb = callback;
	cxt->client_data = client_data;
	cxt->hal_malloc = cb_of_malloc;
	cxt->hal_free = cb_of_free;
	cxt->hal_mem_privdata = client_data;

	CMR_LOGI("create handle 0x%lx 0x%lx", (cmr_uint)cxt, (cmr_uint)cxt->client_data);
	ret = camera_init_internal((cmr_handle)cxt, is_autotest);

#if defined(CONFIG_CAMERA_SUPPORT_8M)
	if (0 == camera_id) {
		CMR_LOGI("camera start in 8M.");
		cmr_grab_setfreq_flag(1);
	} else {
		cmr_grab_setfreq_flag(0);
	}
#endif

exit:
	CMR_LOGI("ret %ld", ret);
	if (CMR_CAMERA_SUCCESS == ret) {
		cxt->inited = 1;
	    *oem_handle = (cmr_handle)cxt;
	} else {
		if (cxt) {
			free((void*)cxt);
		}
	}
	return ret;
}

cmr_int camera_local_deinit(cmr_handle oem_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;

#if defined(CONFIG_CAMERA_SUPPORT_8M)
	CMR_LOGI("camera stop out.");
	cmr_grab_setfreq_flag(0);
#endif

	camera_deinit_internal(oem_handle);
	free((void*)oem_handle);
	return ret;
}

cmr_int camera_local_start_preview(cmr_handle oem_handle, enum takepicture_mode mode,  cmr_uint is_snapshot)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	struct preview_context         *prev_cxt = &cxt->prev_cxt;
	struct setting_cmd_parameter   setting_param;

	cmr_preview_pre_stop(cxt->prev_cxt.preview_handle, cxt->camera_id, 0);

#ifdef CAMERA_PATH_SHARE
	sem_wait(&cxt->resume_sync_sm);
#endif

	ret = camera_set_preview_param(oem_handle, mode, is_snapshot);
	if (ret) {
		CMR_LOGE("failed to set prev param %ld", ret);

#ifdef CAMERA_PATH_SHARE
	sem_post(&cxt->resume_sync_sm);
#endif
		goto exit;
	}

	cxt->setting_cxt.is_active = 1;
	setting_param.camera_id = cxt->camera_id;
	ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, SETTING_SET_ENVIRONMENT, &setting_param);

	ret = cmr_preview_start(prev_cxt->preview_handle, cxt->camera_id);
	if (ret) {
		CMR_LOGE("failed to start prev %ld", ret);
	}

#ifdef CAMERA_PATH_SHARE
	sem_post(&cxt->resume_sync_sm);
#endif

	cxt->camera_mode = mode;
	CMR_LOGI("camera mode %d", cxt->camera_mode);
#if 1
	ret = cmr_sensor_focus_init(cxt->sn_cxt.sensor_handle ,cxt->camera_id);
#endif
exit:
	return ret;
}

cmr_int camera_local_stop_preview(cmr_handle oem_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	cmr_int                         prev_ret = CMR_CAMERA_SUCCESS;
	cmr_int                         snp_ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct common_isp_cmd_param param;

	CMR_LOGI("s");

	if (PREVIEWING != cmr_preview_get_status(cxt->prev_cxt.preview_handle, cxt->camera_id)) {
		CMR_LOGI("don't previewing");
		goto exit;
	}
	if (CAMERA_ZSL_MODE == cxt->camera_mode) {
		if (IDLE != cxt->snp_cxt.status) {
			snp_ret = cmr_snapshot_stop(cxt->snp_cxt.snapshot_handle);
			if (snp_ret) {
				CMR_LOGE("failed to stop snp %ld", ret);
			}
		}
	}
	camera_get_iso_value(oem_handle);

	prev_ret = cmr_preview_stop(cxt->prev_cxt.preview_handle, cxt->camera_id);
	if (ret) {
		CMR_LOGE("failed to stop prev %ld", ret);
	}
	ret = prev_ret | snp_ret;
	cxt->setting_cxt.is_active = 0;
	cxt->prev_cxt.video_size.width = 0;
	cxt->prev_cxt.video_size.height = 0;
exit:
	CMR_LOGV("done %ld", ret);
	return ret;
}

cmr_int camera_local_highflash_ae_measure (cmr_handle oem_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct setting_cmd_parameter    setting_param;

	if (CAMERA_ZSL_MODE != cxt->snp_cxt.snp_mode) {
		setting_param.camera_id = cxt->camera_id;
		setting_param.ctrl_flash.capture_mode.capture_mode= CAMERA_NORMAL_MODE;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, SETTING_SET_HIGHFLASH_AE_MEASURE, &setting_param);
		if (ret) {
			CMR_LOGE("failed to open flash");
		}
	}

	return ret;
}

cmr_int camera_local_start_snapshot(cmr_handle oem_handle, enum takepicture_mode mode, cmr_uint is_snapshot)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	struct preview_context         *prev_cxt;
	struct snapshot_param          snp_param;
	struct common_sn_cmd_param     param;
	struct setting_cmd_parameter   setting_param;
	cmr_int                        flash_status = FLASH_CLOSE;
	cmr_s32                        sm_val = 0;

	if (!oem_handle) {
		CMR_LOGE("error handle");
		goto exit;
	}
	camera_take_snapshot_step(CMR_STEP_TAKE_PIC);
	prev_cxt = &cxt->prev_cxt;

	cmr_preview_pre_stop(cxt->prev_cxt.preview_handle, cxt->camera_id, 1);

	sem_getvalue(&cxt->share_path_sm, &sm_val);
	if (0 != sm_val) {
		sem_destroy(&cxt->share_path_sm);
		sem_init(&cxt->share_path_sm, 0, 0);
		CMR_LOGI("re-initialize share_path_sm");
	}

	if (CAMERA_ZSL_MODE != mode) {
		ret = camera_set_preview_param(oem_handle, mode, is_snapshot);
		if (ret) {
			CMR_LOGE("failed to set preview param %ld", ret);
			goto exit;
		}
	} else {
		camera_get_iso_value(oem_handle);
	}

	CMR_LOGI("@xin E");
	cmr_bzero(&snp_param, sizeof(struct snapshot_param));
	ret = camera_get_snapshot_param(oem_handle, &snp_param);
	if (ret) {
		CMR_LOGE("failed to get snp num %ld", ret);
		goto exit;
	}

	if (0 == cxt->camera_id) {
		setting_param.camera_id = cxt->camera_id;
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, SETTING_GET_HW_FLASH_STATUS, &setting_param);
		if (ret) {
			CMR_LOGE("failed to get flash mode %ld", ret);
			//goto exit;
		} else {
			flash_status = setting_param.cmd_type_value;
		}
		CMR_LOGI("flash_status=%ld", flash_status);
		if (FLASH_OPEN == flash_status || FLASH_HIGH_LIGHT == flash_status || FLASH_TORCH == flash_status) {
		} else {
			cmr_sensor_set_exif(cxt->sn_cxt.sensor_handle, cxt->camera_id, SENSOR_EXIF_CTRL_FLASH, 0);
		}

	}

	ret = cmr_snapshot_post_proc(cxt->snp_cxt.snapshot_handle, &snp_param);
	if (ret) {
		CMR_LOGE("failed to snp post proc %ld", ret);
		goto exit;
	}
	cxt->snp_cxt.actual_capture_size = snp_param.post_proc_setting.chn_out_frm[0].size;
	if (CAMERA_ZSL_MODE != mode) {
		ret = cmr_preview_start(prev_cxt->preview_handle, cxt->camera_id);
		if (ret) {
			CMR_LOGE("failed to start prev %ld", ret);
		}
		cxt->camera_mode = mode;
	}
	camera_set_snp_req((cmr_handle)cxt, TAKE_PICTURE_NEEDED);
	camera_snapshot_started((cmr_handle)cxt);
	ret = camera_get_cap_time((cmr_handle)cxt);
	cxt->snp_cxt.status = SNAPSHOTING;
	cxt->snp_cxt.post_proc_setting.actual_snp_size = snp_param.post_proc_setting.actual_snp_size;
	cmr_copy(cxt->snp_cxt.post_proc_setting.chn_out_frm,
	snp_param.post_proc_setting.chn_out_frm,
	sizeof(cxt->snp_cxt.post_proc_setting.chn_out_frm));
	cxt->snp_cxt.post_proc_setting.actual_snp_size = snp_param.post_proc_setting.actual_snp_size;

#if defined(CONFIG_CAMERA_FLASH_HIGH_AE_MEASURE)
	camera_set_discard_frame(cxt, 1);
	camera_local_highflash_ae_measure(oem_handle);
	camera_get_iso_value(oem_handle);
#endif
	camera_set_discard_frame(cxt, 0);
exit:
	CMR_LOGV("done %ld", ret);
	return ret;
}

cmr_int camera_local_stop_snapshot(cmr_handle oem_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;

	ret = cmr_snapshot_stop(cxt->snp_cxt.snapshot_handle);
	if (ret) {
		CMR_LOGE("failed to stop snp %ld", ret);
		goto exit;
	}

	if (TAKE_PICTURE_NEEDED == camera_get_snp_req(oem_handle)) {
		camera_set_snp_req(oem_handle, TAKE_PICTURE_NO);
		camera_channel_stop(oem_handle, cxt->snp_cxt.channel_bits);
	} else {
		camera_set_snp_req(oem_handle, TAKE_PICTURE_NO);
	}

	ret = cmr_preview_cancel_snapshot(cxt->prev_cxt.preview_handle, cxt->camera_id);
	if (ret) {
		CMR_LOGE("failed to cancel %ld", ret);
		goto exit;
	}

	if (camera_get_hdr_flag(cxt)) {
#ifdef OEM_HANDLE_HDR
		if (0 != cxt->ipm_cxt.frm_num) {
			cxt->ipm_cxt.frm_num = 0;
	//		sem_post(&cxt->hdr_sync_sm);
		}
#endif
		ret = camera_close_hdr(cxt);
	}
	cxt->snp_cxt.status = IDLE;

exit:
	CMR_LOGV("done %ld", ret);
	return ret;
}

cmr_int camera_local_get_prev_rect(cmr_handle oem_handle, struct img_rect *param_ptr)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;

	if (!param_ptr) {
		CMR_LOGE("error param");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	ret = cmr_preview_get_prev_rect(cxt->prev_cxt.preview_handle, cxt->camera_id, param_ptr);
	CMR_LOGI("%d %d %d %d", param_ptr->start_x, param_ptr->start_y, param_ptr->width, param_ptr->height);
exit:
	return ret;
}

cmr_int camera_get_senor_mode_trim(cmr_handle oem_handle, struct img_rect *sn_trim)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	struct sensor_exp_info         *sensor_info = NULL;
	cmr_uint                       sensor_mode  = SENSOR_MODE_MAX;

	if (!oem_handle || !sn_trim) {
		CMR_LOGE("error param");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	sensor_info = (struct sensor_exp_info*)malloc(sizeof(struct sensor_exp_info));
	if (!sensor_info) {
		CMR_LOGE("No mem!");
		ret = CMR_CAMERA_NO_MEM;
		goto exit;
	}
	ret = camera_get_sensor_info(cxt, cxt->camera_id, sensor_info);
	if (ret) {
		CMR_LOGE("get_sensor info failed!");
		ret = CMR_CAMERA_FAIL;
		goto exit;
	}

	ret = cmr_sensor_get_mode(cxt->sn_cxt.sensor_handle, cxt->camera_id, &sensor_mode);
	CMR_LOGI("camera_id =%d sns mode =%ld", cxt->camera_id, sensor_mode);

	sn_trim->start_x = sensor_info->mode_info[sensor_mode].trim_start_x;
	sn_trim->start_y = sensor_info->mode_info[sensor_mode].trim_start_y;
	sn_trim->width = sensor_info->mode_info[sensor_mode].trim_width;
	sn_trim->height = sensor_info->mode_info[sensor_mode].trim_height;
	CMR_LOGI("sensor x=%d y=%d w=%d h=%d", sn_trim->start_x, sn_trim->start_y, sn_trim->width, sn_trim->height);
exit:
	if (sensor_info) {
		free(sensor_info);
		sensor_info = NULL;
	}
	CMR_LOGV("done %ld", ret);
	return ret;
}

cmr_uint camera_get_preview_angle(cmr_handle oem_handle)
{
	cmr_uint                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	struct setting_context         *setting_cxt = NULL;
	struct setting_cmd_parameter   setting_param;

	if (!oem_handle) {
		CMR_LOGE("error param");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	setting_cxt = &cxt->setting_cxt;
	setting_param.camera_id = cxt->camera_id;
	ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_PREVIEW_ANGLE, &setting_param);
	if (ret) {
		CMR_LOGE("failed to get preview angle %ld", ret);
		goto exit;
	}
	ret = setting_param.cmd_type_value;
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_uint camera_get_exif_info(cmr_handle oem_handle, struct exif_info *exif_info)
{
	cmr_uint                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	struct setting_context         *setting_cxt = NULL;
	struct setting_cmd_parameter   setting_param;
	EXIF_SPEC_PIC_TAKING_COND_T *exif_spec = NULL;

	if (!oem_handle) {
		CMR_LOGE("error param");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	setting_cxt = &cxt->setting_cxt;
	setting_param.camera_id = cxt->camera_id;
	ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, SETTING_GET_EXIF_INFO, &setting_param);
	if (ret) {
		CMR_LOGE("failed to get exif %ld", ret);
		goto exit;
	}
	exif_spec = setting_param.exif_all_info_ptr->spec_ptr->pic_taking_cond_ptr;
	exif_info->aperture = (float)exif_spec->ApertureValue.numerator / (float)exif_spec->ApertureValue.denominator;
	exif_info->focus_distance = (float)exif_spec->FocalLength.numerator / (float)exif_spec->FocalLength.denominator;
	ret = setting_param.cmd_type_value;
exit:
	CMR_LOGI("apet %f focus dist %f", exif_info->aperture, exif_info->focus_distance);
	return ret;
}

cmr_int camera_local_get_zsl_info(cmr_handle oem_handle, cmr_uint *is_support,
                                        cmr_uint *max_width, cmr_uint *max_height)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct preview_zsl_info        zsl_info;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;

	if (!oem_handle || !is_support || !max_width || !max_height) {
		CMR_LOGE("error param");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	ret = cmr_preview_is_support_zsl(cxt->prev_cxt.preview_handle, cxt->camera_id, is_support);
	if (ret) {
		CMR_LOGE("failed to get zsl info %ld", ret);
		goto exit;
	}

	ret = cmr_preview_get_max_cap_size(cxt->prev_cxt.preview_handle, cxt->camera_id, max_width, max_height);
	if (ret) {
		CMR_LOGE("failed to get max cap size %ld", ret);
		goto exit;
	}

exit:
	CMR_LOGV("done %ld", ret);
	return ret;
}

cmr_int camera_local_redisplay_data(cmr_handle oem_handle, cmr_uint output_addr,
                                                   cmr_uint output_width, cmr_uint output_height,
                                                   cmr_uint input_addr_y, cmr_uint input_addr_uv,
                                                   cmr_uint input_width, cmr_uint input_height)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	struct img_frm                 src_img;
	struct img_frm                 dst_img;
	struct cmr_rot_param           rot_param;
	struct img_rect                rect;
	enum img_angle                 angle = IMG_ANGLE_0;
	cmr_uint                       img_len = (cmr_uint)(output_width * output_height);
	struct setting_context         *setting_cxt;
	struct setting_cmd_parameter   setting_param;
	setting_param.camera_id = cxt->camera_id;

	if (!oem_handle) {
		CMR_LOGE("err, handle is null");
		goto exit;
	}
	CMR_LOGI("0x%lx %d", (cmr_uint)oem_handle, cxt->snp_cxt.cfg_cap_rot);
	setting_cxt = &cxt->setting_cxt;

	rect.start_x = 0;
	rect.start_y = 0;
	rect.width = input_width;
	rect.height = input_height;
	src_img.size.width = input_width;
	src_img.size.height = input_height;
	src_img.fmt = IMG_DATA_TYPE_YUV420;
	cmr_grab_get_dcam_endian(&cxt->snp_cxt.data_endian, &src_img.data_end);
	src_img.addr_phy.addr_y = input_addr_y;
	src_img.addr_phy.addr_u = input_addr_uv;
	if (IMG_ANGLE_90 == cxt->snp_cxt.cfg_cap_rot || IMG_ANGLE_270 == cxt->snp_cxt.cfg_cap_rot) {
		dst_img.size.width = output_height;
		dst_img.size.height = output_width;
		dst_img.addr_phy.addr_y = output_addr + ((img_len * 3) >> 1);
		dst_img.addr_phy.addr_u = dst_img.addr_phy.addr_y + img_len;
	}else if (IMG_ANGLE_180 == cxt->snp_cxt.cfg_cap_rot) {
		dst_img.size.width = output_width;
		dst_img.size.height = output_height;
		dst_img.addr_phy.addr_y = output_addr + ((img_len * 3) >> 1);
		dst_img.addr_phy.addr_u = dst_img.addr_phy.addr_y + img_len;
	} else {
		dst_img.size.width = output_width;
		dst_img.size.height = output_height;
		dst_img.addr_phy.addr_y = output_addr;
		dst_img.addr_phy.addr_u = dst_img.addr_phy.addr_y + img_len;
	}
	dst_img.addr_phy.addr_v = 0;
	dst_img.fmt = IMG_DATA_TYPE_YUV420;
	cmr_grab_get_dcam_endian(&cxt->snp_cxt.data_endian, &dst_img.data_end);
	CMR_LOGI("data_end %d %d", cxt->snp_cxt.data_endian.y_endian, cxt->snp_cxt.data_endian.uv_endian);
	rect.start_x = 0;
	rect.start_y = 0;
	rect.width = input_width;
	rect.height = input_height;
	setting_param.camera_id = cxt->camera_id;
	ret = cmr_setting_ioctl(setting_cxt->setting_handle, SETTING_GET_ZOOM_PARAM, &setting_param);
	if (ret) {
		CMR_LOGE("failed to get zoom param %ld", ret);
		goto exit;
	}
	if (setting_param.zoom_param.mode == ZOOM_LEVEL) {
		ret = camera_get_trim_rect(&rect, 0, &dst_img.size);
	} else {
		float zoomRatio = 1.0f;
		float dstRatio = (float)dst_img.size.width / (float)dst_img.size.height;
		ret = camera_get_trim_rect2(&rect, zoomRatio, dstRatio, input_width, input_height, IMG_ANGLE_0);
	}
	if (ret) {
		CMR_LOGE("failed to get trim %ld", ret);
		goto exit;
	}
	src_img.rect = rect;
	ret = cmr_scale_start(cxt->scaler_cxt.scaler_handle, &src_img, &dst_img, (cmr_evt_cb)NULL, NULL);
	if (ret) {
		CMR_LOGE("failed to start start %ld", ret);
		ret = - CMR_CAMERA_FAIL;
		goto exit;
	}

	/* start roattion*/
	if (IMG_ANGLE_0 != cxt->snp_cxt.cfg_cap_rot) {
		if (IMG_ANGLE_90 == cxt->snp_cxt.cfg_cap_rot) {
			angle = IMG_ANGLE_270;
		} else if (IMG_ANGLE_270 == cxt->snp_cxt.cfg_cap_rot) {
			angle = IMG_ANGLE_90;
		} else {
			angle = cxt->snp_cxt.cfg_cap_rot;
		}
		rect.start_x = 0;
		rect.start_y = 0;
		rect.width = dst_img.size.width;
		rect.height = dst_img.size.height;
		src_img.addr_phy.addr_y = dst_img.addr_phy.addr_y;
		src_img.addr_phy.addr_u = dst_img.addr_phy.addr_u;
		src_img.addr_phy.addr_v = 0;
		src_img.size.width = dst_img.size.width;
		src_img.size.height = dst_img.size.height;
		src_img.fmt = IMG_DATA_TYPE_YUV420;
		src_img.data_end = cxt->snp_cxt.data_endian;
		dst_img.addr_phy.addr_y = output_addr;
		dst_img.addr_phy.addr_u = dst_img.addr_phy.addr_y + img_len;
		dst_img.data_end = cxt->snp_cxt.data_endian;
		rot_param.handle = cxt->rot_cxt.rotation_handle;
		rot_param.angle = angle;
		rot_param.src_img = src_img;
		rot_param.dst_img = dst_img;
		src_img.rect = rect;
		ret = cmr_rot(&rot_param);
		if (ret) {
			CMR_LOGI("failed to rotate %ld", ret);
			goto exit;
		}
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int camera_local_fd_start(cmr_handle oem_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;

	CMR_LOGV("fd %d", cxt->fd_on_off);
	if (PREVIEWING == cmr_preview_get_status(cxt->prev_cxt.preview_handle, cxt->camera_id)) {
		ret = cmr_preview_ctrl_facedetect(cxt->prev_cxt.preview_handle, cxt->camera_id, cxt->fd_on_off);
		if (ret) {
			CMR_LOGE("failed to fd ctrl %ld", ret);
		}
	}
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int camera_local_set_param(cmr_handle oem_handle, enum camera_param_type id, cmr_uint param)
{
	cmr_int                          ret = CMR_CAMERA_SUCCESS;
	struct camera_context            *cxt = (struct camera_context*)oem_handle;

	switch (id) {
	case CAMERA_PARAM_FOCUS_RECT:
		CMR_LOGI("set focus rect 0x%lx",param);
		ret = cmr_focus_set_param(cxt->focus_cxt.focus_handle, cxt->camera_id, id, (void*)param);
		break;
	case CAMERA_PARAM_AF_MODE:
		CMR_LOGI("set focus af mode 0x%lx",param);
		ret = cmr_focus_set_param(cxt->focus_cxt.focus_handle, cxt->camera_id, id, (void*)param);
		break;
	case CAMERA_PARAM_ZOOM:
		ret = cmr_preview_update_zoom(cxt->prev_cxt.preview_handle, cxt->camera_id, (struct cmr_zoom_param*)param);
		if (ret) {
			CMR_LOGE("failed to update zoom %ld", ret);
		}
		ret = camera_set_setting(oem_handle, id, param);
		break;
	case CAMERA_PARAM_ISO:
		cxt->setting_cxt.iso_value = param;
		ret = camera_set_setting(oem_handle, id, param);
		break;
	default:
		ret = camera_set_setting(oem_handle, id, param);
		break;
	}
	if (ret) {
		CMR_LOGE("failed to set param %ld", ret);
	}
exit:
	return ret;
}

cmr_int camera_local_start_focus(cmr_handle oem_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;

	ret = cmr_af_start_notice_flash(cxt->setting_cxt.setting_handle);
	ret = cmr_af_start_notice_focus(cxt->focus_cxt.focus_handle);
	ret = cmr_focus_start(cxt->focus_cxt.focus_handle, cxt->camera_id);

	CMR_LOGV("done %ld", ret);
	return ret;
}

cmr_int camera_local_cancel_focus(cmr_handle oem_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt = (struct camera_context*)oem_handle;

	ret = cmr_af_cancel_notice_flash(cxt->setting_cxt.setting_handle);
	ret = cmr_af_cancel_notice_focus(cxt->focus_cxt.focus_handle);
	ret = cmr_focus_stop(cxt->focus_cxt.focus_handle, cxt->camera_id, 1);

	CMR_LOGV("done %ld", ret);
	return ret;
}

cmr_int camera_local_transfer_caf_to_af(cmr_handle oem_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;

	ret = cmr_transfer_caf_to_af(cxt->focus_cxt.focus_handle);

	return ret;
}

cmr_int camera_local_transfer_af_to_caf(cmr_handle oem_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;

	ret = cmr_transfer_af_to_caf(cxt->focus_cxt.focus_handle);

	return ret;
}

cmr_int camera_local_pre_flash (cmr_handle oem_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct camera_context           *cxt = (struct camera_context*)oem_handle;
	struct setting_cmd_parameter    setting_param;

	/*start preflash*/
	//if (CAMERA_ZSL_MODE != cxt->snp_cxt.snp_mode) {
		setting_param.camera_id = cxt->camera_id;
		setting_param.ctrl_flash.capture_mode.capture_mode= CAMERA_NORMAL_MODE;
		ret = cmr_pre_flash_notice_flash(cxt->setting_cxt.setting_handle);
		ret = cmr_setting_ioctl(cxt->setting_cxt.setting_handle, SETTING_SET_PRE_LOWFLASH, &setting_param);
		if (ret) {
			CMR_LOGE("failed to open flash");
		}
	//}

	return ret;
}

cmr_int camera_local_get_viewangle(cmr_handle oem_handle, struct sensor_view_angle *view_angle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt;
	struct sensor_exp_info         exp_info;

	if (!oem_handle || !view_angle) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	cxt = (struct camera_context*)oem_handle;
	ret = cmr_sensor_get_info(cxt->sn_cxt.sensor_handle, cxt->camera_id, &exp_info);
	if (ret) {
		CMR_LOGE("failed to get sensor info %ld", ret);
		goto exit;
	}

	view_angle->horizontal_val = exp_info.view_angle.horizontal_val;
	view_angle->vertical_val = exp_info.view_angle.vertical_val;
exit:
	return ret;
}
cmr_int camera_local_set_preview_buffer(cmr_handle oem_handle, cmr_uint src_phy_addr, cmr_uint src_vir_addr)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt;
	struct sensor_exp_info         exp_info;
	if (!oem_handle || !src_phy_addr || !src_vir_addr) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	cxt = (struct camera_context*)oem_handle;
	ret = cmr_preview_set_preview_buffer(cxt->prev_cxt.preview_handle, cxt->camera_id, src_phy_addr, src_vir_addr);
	if (ret) {
		CMR_LOGE("failed to set preview buffer %ld", ret);
		goto exit;
	}
exit:
	return ret;
}
cmr_int camera_local_set_video_buffer(cmr_handle oem_handle, cmr_uint src_phy_addr, cmr_uint src_vir_addr)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt;
	struct sensor_exp_info         exp_info;
	if (!oem_handle || !src_phy_addr || !src_vir_addr) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	cxt = (struct camera_context*)oem_handle;
	ret = cmr_preview_set_video_buffer(cxt->prev_cxt.preview_handle, cxt->camera_id, src_phy_addr, src_vir_addr);
	if (ret) {
		CMR_LOGE("failed to set video buffer %ld", ret);
		goto exit;
	}
exit:
	return ret;
}
cmr_int camera_local_set_zsl_buffer(cmr_handle oem_handle, cmr_uint src_phy_addr, cmr_uint src_vir_addr)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt;
	struct sensor_exp_info         exp_info;
	if (!oem_handle || !src_phy_addr || !src_vir_addr) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	cxt = (struct camera_context*)oem_handle;
	ret = cmr_preview_set_zsl_buffer(cxt->prev_cxt.preview_handle, cxt->camera_id, src_phy_addr, src_vir_addr);
	if (ret) {
		CMR_LOGE("failed to set zsl buffer %ld", ret);
		goto exit;
	}
exit:
	return ret;
}

cmr_int camera_local_set_video_snapshot_buffer(cmr_handle oem_handle, cmr_uint src_phy_addr, cmr_uint src_vir_addr)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt;
	struct frm_info                chn_data;
	cmr_u32                        buffer_size = 0;
	if (!oem_handle || !src_phy_addr || !src_vir_addr) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	cxt = (struct camera_context*)oem_handle;
	CMR_LOGI("in video w=%d h=%d cap w=%d h=%d", cxt->prev_cxt.actual_video_size.width, cxt->prev_cxt.actual_video_size.height, cxt->snp_cxt.actual_capture_size.width, cxt->snp_cxt.actual_capture_size.height);
	if (TAKE_PICTURE_NEEDED == camera_get_snp_req((cmr_handle)cxt)) {
		if (cxt->snp_cxt.actual_capture_size.width * cxt->snp_cxt.actual_capture_size.height
				< cxt->prev_cxt.actual_video_size.width * cxt->prev_cxt.actual_video_size.height)
			buffer_size = cxt->snp_cxt.actual_capture_size.width * cxt->snp_cxt.actual_capture_size.height;
		else
			buffer_size = cxt->prev_cxt.actual_video_size.width * cxt->prev_cxt.actual_video_size.height;
		cmr_bzero(&chn_data, sizeof(struct frm_info));
		cmr_copy(&chn_data, &cxt->prev_cxt.video_cur_chn_data, sizeof(struct frm_info));
		chn_data.yaddr      = src_phy_addr;
		chn_data.uaddr      = src_phy_addr + buffer_size;
		chn_data.vaddr      = 0;
		chn_data.yaddr_vir  = src_vir_addr;
		chn_data.uaddr_vir  = src_vir_addr + buffer_size;
		chn_data.vaddr_vir  = 0;
		ret = cmr_snapshot_receive_data(cxt->snp_cxt.snapshot_handle, SNAPSHOT_EVT_CHANNEL_DONE, (void*)&chn_data);
	} else {
		CMR_LOGE("snapshot is not ready");
	}
	if (ret) {
		CMR_LOGE("failed to set snapshot buffer %ld", ret);
		goto exit;
	}
exit:
	CMR_LOGI("out");
	return ret;
}

cmr_int camera_local_set_zsl_snapshot_buffer(cmr_handle oem_handle, cmr_uint src_phy_addr, cmr_uint src_vir_addr)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt;
	struct frm_info                chn_data;
	cmr_u32                        buffer_size = 0;
	cmr_int                        need_pause = 0;
	if (!oem_handle || !src_phy_addr || !src_vir_addr) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	CMR_LOGI("in src_phy_addr 0x%lx src_vir_addr 0x%lx", src_phy_addr, src_vir_addr);
	cxt = (struct camera_context*)oem_handle;
	camera_local_zsl_snapshot_need_pause(oem_handle, &need_pause);
	if (TAKE_PICTURE_NEEDED == camera_get_snp_req((cmr_handle)cxt)) {
		buffer_size         = cxt->snp_cxt.actual_capture_size.width * cxt->snp_cxt.actual_capture_size.height;
		cmr_bzero(&chn_data, sizeof(struct frm_info));
		cmr_copy(&chn_data, &cxt->snp_cxt.cur_chn_data, sizeof(struct frm_info));
		chn_data.yaddr      = src_phy_addr;
		chn_data.uaddr      = src_phy_addr + buffer_size;
		chn_data.vaddr      = 0;
		chn_data.yaddr_vir  = src_vir_addr;
		chn_data.uaddr_vir  = src_vir_addr + buffer_size;
		chn_data.vaddr_vir  = 0;
		ret = cmr_snapshot_receive_data(cxt->snp_cxt.snapshot_handle, SNAPSHOT_EVT_CHANNEL_DONE, (void*)&chn_data);
		if (need_pause) {
			camera_set_discard_frame(cxt, 1);
			ret = cmr_preview_receive_data(cxt->prev_cxt.preview_handle, cxt->camera_id, PREVIEW_CHN_PAUSE, (void*)&chn_data);
		}
	} else {
		CMR_LOGE("snapshot is not ready");
	}
	if (ret) {
		CMR_LOGE("failed to set snapshot buffer %ld", ret);
		goto exit;
	}
exit:
	CMR_LOGI("out");
	return ret;
}

cmr_int camera_local_zsl_snapshot_need_pause(cmr_handle oem_handle, cmr_int *flag)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct camera_context          *cxt;
	struct cmr_path_capability     capability;

	if (!oem_handle || !flag) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	CMR_LOGV("in");
	cxt = (struct camera_context*)oem_handle;

	camera_channel_path_capability(oem_handle, &capability);
	*flag = capability.capture_pause;
exit:
	CMR_LOGV("out flag %ld", *flag);
	return ret;
}



void camera_local_start_burst_notice(cmr_handle oem_handle){
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	struct isp_context             *isp_cxt = &cxt->isp_cxt;
	uint32_t caf_switch = 0;
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
	isp_ioctl(isp_cxt->isp_handle, ISP_CTRL_BURST_NOTICE,&caf_switch);
#endif

}

void camera_local_end_burst_notice(cmr_handle oem_handle){
	struct camera_context          *cxt = (struct camera_context*)oem_handle;
	struct isp_context             *isp_cxt = &cxt->isp_cxt;
	uint32_t caf_switch = 1;
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
	isp_ioctl(isp_cxt->isp_handle, ISP_CTRL_BURST_NOTICE,&caf_switch);
#endif
}
