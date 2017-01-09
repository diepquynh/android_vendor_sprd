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
#define LOG_TAG "cmr_snp"

#include <stdlib.h>
#include "cmr_snapshot.h"
#include "cmr_msg.h"
#include "isp_video.h"
#include "cmr_ipm.h"
#include "cmr_oem.h"
#include "cutils/properties.h"

#include "isp_otp.h"

#define SNP_MSG_QUEUE_SIZE                      50
#define SNP_INVALIDE_VALUE                       0xFFFFFFFF
#define SENSOR_DEFAULT_PIX_WIDTH          0x0a

#define SNP_EVT_BASE                                 (CMR_EVT_SNAPSHOT_BASE + SNPASHOT_EVT_MAX)
#define SNP_EVT_INIT                                  (SNP_EVT_BASE)
#define SNP_EVT_DEINIT                              (SNP_EVT_BASE + 1)
#define SNP_EVT_STOP                                 (SNP_EVT_BASE + 2)
#define SNP_EVT_EXIT                                  (SNP_EVT_BASE + 3)
#define SNP_EVT_START_PROC                     (SNP_EVT_BASE + 4)
#define SNP_EVT_START_CVT                        (SNP_EVT_BASE + 5)

#define SNP_EVT_CHANNEL_DONE                 (SNP_EVT_BASE + 30)/*SNAPSHOT_EVT_CHANNEL_DONE*/
#define SNP_EVT_RAW_PROC                        (SNP_EVT_BASE + 31)/*SNAPSHOT_EVT_RAW_PROC*/
#define SNP_EVT_SC_DONE                           (SNP_EVT_BASE + 32)/*SNAPSHOT_EVT_SC_DONE*/
#define SNP_EVT_HDR_DONE                         (SNP_EVT_BASE + 33)/*SNAPSHOT_EVT_HDR_DONE*/
#define SNP_EVT_CVT_RAW_DATA                (SNP_EVT_BASE + 34)/*SNAPSHOT_EVT_CVT_RAW_DATA*/
#define SNP_EVT_JPEG_ENC_DONE                (SNP_EVT_BASE + 35)/*SNAPSHOT_EVT_JPEG_ENC_DONE*/
#define SNP_EVT_JPEG_DEC_DONE                (SNP_EVT_BASE + 36)/*SNAPSHOT_EVT_JPEG_DEC_DONE*/
#define SNP_EVT_JPEG_ENC_ERR                  (SNP_EVT_BASE + 37)/*SNAPSHOT_EVT_JPEG_ENC_ERR*/
#define SNP_EVT_JPEG_DEC_ERR                  (SNP_EVT_BASE + 38)/*SNAPSHOT_EVT_JPEG_DEC_ERR*/
#define SNP_EVT_ANDROID_ZSL_DATA          (SNP_EVT_BASE + 39)/*SNAPSHOT_EVT_ANDROID_ZSL_DATA*/
#define SNP_EVT_HDR_SRC_DONE                 (SNP_EVT_BASE + 40)
#define SNP_EVT_HDR_POST_PROC               (SNP_EVT_BASE + 41)
#define SNP_EVT_FREE_FRM                          (SNP_EVT_BASE + 42)
#define SNP_EVT_WRITE_EXIF                       (SNP_EVT_BASE + 43)
#define SNP_EVT_REDISPLAY                         (SNP_EVT_BASE + 44)
#define SNP_EVT_THUMB                               (SNP_EVT_BASE + 45)
#define SNP_EVT_CHANNEL_VIDEO_DONE      (SNP_EVT_BASE + 46)

#define CHECK_HANDLE_VALID(handle) \
													 do { \
														if (!handle) { \
															CMR_LOGE("err handle");  \
															return CMR_CAMERA_INVALID_PARAM; \
														} \
													 } while(0)

#define IS_CAP_FRM(id, v)                            ((((id) & (v)) == (v)) && (((id) - (v)) < CMR_CAPTURE_MEM_SUM))
#define SRC_MIPI_RAW                                 "/data/isptool_src_mipi_raw_file.raw"

#define SNP_CHN_OUT_DATA    		                 8000 /*debug.camera.save.snpfile 1*/
#define SNP_ROT_DATA                                 8001 /*debug.camera.save.snpfile 2*/
#define SNP_SCALE_DATA                               8002
#define SNP_REDISPLAY_DATA                           8003 /*debug.camera.save.snpfile 3*/
#define SNP_ENCODE_SRC_DATA                          8004 /*debug.camera.save.snpfile 4*/
#define SNP_ENCODE_STREAM                            9000 /*debug.camera.save.snpfile 5*/
#define SNP_THUMB_DATA                               8006 /*debug.camera.save.snpfile 6*/
#define SNP_THUMB_STREAM                             1000 /*debug.camera.save.snpfile 7*/
#define SNP_JPEG_STREAM                              2000 /*debug.camera.save.snpfile 8*/
/********************************* internal data type *********************************/

enum cvt_trigger_src {
	SNP_TRIGGER = 0,
	NON_SNP_TRIGGER,
	CVT_TRIGGER_SRC_MAX
};

struct snp_thread_context {
	cmr_handle                          main_thr_handle;
	cmr_handle                          post_proc_thr_handle;
	cmr_handle                          notify_thr_handle;
	cmr_handle                          proc_cb_thr_handle;
	cmr_handle                          secondary_thr_handle;
	cmr_handle                          cvt_thr_handle;
	cmr_handle                          write_exif_thr_handle;
	cmr_handle                          proc_redisplay_handle;
	cmr_handle                          proc_thumb_handle;
	sem_t                               writte_exif_access_sm;
};

struct process_status {
	cmr_u32                             slice_height_in;
	cmr_u32                             slice_height_out;
	cmr_u32                             is_encoding;
	cmr_u32                             slice_num;
	struct frm_info                     frame_info;
};

struct snp_rot_param {
	cmr_uint                            angle;
	struct img_frm                      src_img;
	struct img_frm                      dst_img;
};

struct snp_scale_param {
	cmr_uint                            slice_height;
	struct img_frm                      src_img;
	struct img_frm                      dst_img;
};

struct snp_jpeg_param {
	struct img_frm                      src;
	struct img_frm                      dst;
	struct cmr_op_mean                  mean;
};

struct snp_exif_param {
	struct img_frm                      big_pic_stream_src;
	struct img_frm                      thumb_stream_src;
	struct img_frm                      dst;
};

struct snp_exif_out_param {
	cmr_uint                            virt_addr;
	cmr_uint                            stream_size;
};

struct snp_raw_proc_param {
	struct img_frm                      src_frame;
	struct img_frm                      dst_frame;
	cmr_u32                             src_avail_height;
	cmr_u32                             src_slice_height;
	cmr_u32                             dst_slice_height;
	cmr_u32                             lice_num;
};

struct snp_channel_param {
	cmr_u32                             is_scaling;
	cmr_u32                             is_rot;
	struct img_frm                      chn_frm[CMR_CAPTURE_MEM_SUM];
	struct img_frm                      hdr_src_frm[CMR_CAPTURE_MEM_SUM];
	struct snp_scale_param              scale[CMR_CAPTURE_MEM_SUM];
	struct snp_scale_param              convert_thumb[CMR_CAPTURE_MEM_SUM];
	struct snp_rot_param                rot[CMR_CAPTURE_MEM_SUM];
	/*jpeg encode param*/
	cmr_u32                             thumb_stream_size[CMR_CAPTURE_MEM_SUM];
	struct snp_jpeg_param               jpeg_in[CMR_CAPTURE_MEM_SUM];
	struct snp_jpeg_param               thumb_in[CMR_CAPTURE_MEM_SUM];
	struct snp_exif_param               exif_in[CMR_CAPTURE_MEM_SUM];
	struct snp_exif_out_param           exif_out[CMR_CAPTURE_MEM_SUM];
	/*jpeg decode param*/
	struct snp_jpeg_param               jpeg_dec_in[CMR_CAPTURE_MEM_SUM];
	/*isp proc*/
	struct raw_proc_param               isp_proc_in[CMR_CAPTURE_MEM_SUM];
	struct process_status               isp_process[CMR_CAPTURE_MEM_SUM];
};

struct snp_cvt_context {
	cmr_u32                             proc_height;
	cmr_u32                             is_first_slice;
	cmr_u32                             slice_height;
	cmr_u32                             trigger;
	sem_t                               cvt_sync_sm;
	struct frm_info                     frame_info;
	struct img_frm                      out_frame;
};

struct snp_context {
	cmr_handle                          oem_handle;
	cmr_uint                            is_inited;
	cmr_uint                            camera_id;
	cmr_uint                            status;
	cmr_int                             err_code;
	cmr_u32                             is_request;
	cmr_u32                             index;
	sem_t                               access_sm;
	snapshot_cb_of_state                oem_cb;
	struct snapshot_md_ops              ops;
	struct snp_thread_context           thread_cxt;
	void*                               caller_privdata;
	struct snapshot_param               req_param;
	struct sensor_exp_info              sensor_info;
	/*channel param*/
	struct snp_channel_param            chn_param;
	/*post proc contrl*/
	struct frm_info                     cur_frame_info;
	cmr_u32                             cap_cnt;
	cmr_u32                             is_frame_done;
	cmr_u32                             jpeg_stream_size;
	cmr_u32                             thumb_stream_size;
	cmr_u32                             jpeg_exif_stream_size;
	cmr_u32                             isptool_saved_file_count;
	cmr_u32                             hdr_src_num;
	cmr_u32                             padding;
	sem_t                               proc_done_sm;
	sem_t                               jpeg_sync_sm;
	sem_t                               scaler_sync_sm;
	sem_t                               takepic_callback_sem;
	sem_t                               hdr_sync_sm;
	sem_t                               redisplay_sm;
	sem_t                               writer_exif_sm;
	struct snp_cvt_context              cvt;
};
/********************************* internal data type *********************************/
static cmr_int snp_main_thread_proc(struct cmr_msg *message, void* p_data);
static cmr_int snp_postproc_thread_proc(struct cmr_msg *message, void* p_data);
static cmr_int snp_notify_thread_proc(struct cmr_msg *message, void* p_data);
static cmr_int snp_proc_cb_thread_proc(struct cmr_msg *message, void* p_data);
static cmr_int snp_secondary_thread_proc(struct cmr_msg *message, void* p_data);
static cmr_int snp_cvt_thread_proc(struct cmr_msg *message, void* p_data);
static cmr_int snp_write_exif_thread_proc(struct cmr_msg *message, void* p_data);
static cmr_int snp_redisplay_thread_proc(struct cmr_msg *message, void* p_data);
static cmr_int snp_thumb_thread_proc(struct cmr_msg *message, void* p_data);
static cmr_int snp_create_main_thread(cmr_handle snp_handle);
static cmr_int snp_destroy_main_thread(cmr_handle snp_handle);
static cmr_int snp_create_postproc_thread(cmr_handle snp_handle);
static cmr_int snp_destroy_postproc_thread(cmr_handle snp_handle);
static cmr_int snp_create_notify_thread(cmr_handle snp_handle);
static cmr_int snp_destroy_notify_thread(cmr_handle snp_handle);
static cmr_int snp_create_proc_cb_thread(cmr_handle snp_handle);
static cmr_int snp_destroy_proc_cb_thread(cmr_handle snp_handle);
static cmr_int snp_create_secondary_thread(cmr_handle snp_handle);
static cmr_int snp_destroy_secondary_thread(cmr_handle snp_handle);
static cmr_int snp_create_cvt_thread(cmr_handle snp_handle);
static cmr_int snp_destroy_cvt_thread(cmr_handle snp_handle);
static cmr_int snp_create_write_exif_thread(cmr_handle snp_handle);
static cmr_int snp_destroy_write_exif_thread(cmr_handle snp_handle);
static cmr_int snp_create_redisplay_thread(cmr_handle snp_handle);
static cmr_int snp_destroy_redisplay_thread(cmr_handle snp_handle);
static cmr_int snp_create_thumb_thread(cmr_handle snp_handle);
static cmr_int snp_destroy_thumb_thread(cmr_handle snp_handle);
static cmr_int snp_create_thread(cmr_handle snp_handle);
static cmr_int snp_destroy_thread(cmr_handle snp_handle);
static void snp_local_init(cmr_handle snp_handle);
static void snp_local_deinit(cmr_handle snp_handle);
static cmr_int snp_check_post_proc_param(struct snapshot_param *param_ptr);
static cmr_int snp_set_channel_out_param(cmr_handle snp_handle);
static cmr_int snp_set_hdr_param(cmr_handle snp_handle);
static cmr_int snp_set_scale_param(cmr_handle snp_handle);
static cmr_int snp_set_convert_thumb_param(cmr_handle snp_handle);
static cmr_int snp_set_rot_param(cmr_handle snp_handle);
static cmr_int snp_set_jpeg_enc_param(cmr_handle snp_handle);
static cmr_int snp_set_jpeg_thumb_param(cmr_handle snp_handle);
static cmr_int snp_set_jpeg_exif_param(cmr_handle snp_handle);
static cmr_int snp_set_jpeg_dec_param(cmr_handle snp_handle);
static cmr_int snp_set_isp_proc_param(cmr_handle snp_handle);
static void snp_get_is_scaling(cmr_handle snp_handle, cmr_s32 is_normal_snapshot);
static cmr_int snp_clean_thumb_param(cmr_handle snp_handle);
static cmr_int snp_set_post_proc_param(cmr_handle snp_handle, struct snapshot_param *param_ptr);
static void snp_set_request(cmr_handle snp_handle, cmr_u32 is_request);
static cmr_u32 snp_get_request(cmr_handle snp_handle);
static cmr_int snp_check_frame_is_valid(cmr_handle snp_handle, struct frm_info *frm_ptr);
static cmr_int snp_post_proc_for_yuv(cmr_handle snp_handle, void *data);
static cmr_int snp_post_proc_for_jpeg(cmr_handle snp_handle, void *data);
static cmr_int snp_post_proc_for_raw(cmr_handle snp_handle, void *data);
static void snp_reset_ctrl_condition(cmr_handle snp_handle);
static cmr_int snp_post_proc(cmr_handle snp_handle, void *data);
static cmr_int snp_stop_proc(cmr_handle snp_handle);
static cmr_int snp_start_encode(cmr_handle snp_handle, void *data);
static cmr_int snp_start_encode_thumb(cmr_handle snp_handle);
static cmr_int snp_start_isp_proc(cmr_handle snp_handle, void *data);
static cmr_int snp_start_isp_next_proc(cmr_handle snp_handle, void *data);
static cmr_int snp_start_rot(cmr_handle snp_handle, void *data);
static cmr_int snp_start_scale(cmr_handle snp_handle, void *data);
static cmr_int snp_start_convet_thumb(cmr_handle snp_handle, void *data);
static cmr_int snp_start_decode_sync(cmr_handle snp_handle, void *data);
static cmr_int snp_start_decode(cmr_handle snp_handle, void *data);
static void snp_wait_cvt_done(cmr_handle snp_handle);
static void snp_cvt_done(cmr_handle snp_handle);
static cmr_int snp_start_cvt(cmr_handle snp_handle, void *data);
static void snp_post_proc_exit(struct snp_context *snp_cxt);
/*static void snp_main_thr_proc_exit(struct snp_context *snp_cxt);*/
static cmr_int snp_send_msg_notify_thr(cmr_handle snp_handle, cmr_int func_type, cmr_int evt, void *data, cmr_uint data_len);
static cmr_int snp_send_msg_secondary_thr(cmr_handle snp_handle, cmr_int func_type, cmr_int evt, void *data);
static cmr_int snp_send_msg_write_exif_thr(cmr_handle snp_handle, cmr_int evt, void *data);
static cmr_int snp_send_msg_redisplay_thr(cmr_handle snp_handle, cmr_int evt, void *data);
static cmr_int snp_send_msg_thumb_thr(cmr_handle snp_handle, cmr_int evt, void *data);
static cmr_int snp_checkout_exit(cmr_handle snp_handle);
static cmr_int snp_take_picture_done(cmr_handle snp_handle, struct frm_info *data);
static cmr_int snp_thumbnail(cmr_handle snp_handle, struct frm_info *data);
static cmr_int camera_set_frame_type(cmr_handle snp_handle, struct camera_frame_type *frame_type, struct frm_info* info);
static void snp_takepic_callback_done(cmr_handle snp_handle);
static void snp_takepic_callback_wait(cmr_handle snp_handle);
static cmr_int snp_redisplay(cmr_handle snp_handle, struct frm_info *data);
static cmr_int snp_start_thumb_proc(cmr_handle snp_handle, struct frm_info *data);
static cmr_int snp_notify_redisplay_proc(cmr_handle snp_handle, cmr_int func_type, cmr_int evt, void *data);
static cmr_int snp_ipm_cb_handle(cmr_handle snp_handle, void *data);
static cmr_int snp_scale_cb_handle(cmr_handle snp_handle, void *data);
static cmr_int snp_jpeg_enc_cb_handle(cmr_handle snp_handle, void *data);
static cmr_int snp_jpeg_dec_cb_handle(cmr_handle snp_handle, void *data);
static void snp_post_proc_done(cmr_handle snp_handle);
static void snp_post_proc_err_exit(cmr_handle snp_handle, cmr_int err_code);
static cmr_int isp_is_have_src_data_from_picture(void);
static void isp_set_saved_file_count(cmr_handle snp_handle);
static cmr_u32 isp_get_saved_file_count(cmr_handle snp_handle);
static cmr_int isp_overwrite_cap_mem(cmr_handle snp_handle);
static cmr_int snp_proc_android_zsl_data(cmr_handle snp_handle, void *data);
static void snp_set_status(cmr_handle snp_handle, cmr_uint status);
static cmr_uint snp_get_status(cmr_handle snp_handle);
static cmr_int snp_proc_hdr_src(cmr_handle snp_handle, void *data);
static void snp_jpeg_enc_err_handle(cmr_handle snp_handle);
/*static void snp_jpeg_dec_err_handle(cmr_handle snp_handle);*/
static void snp_autotest_proc(cmr_handle snp_handle, void *data);

static cmr_int camera_open_uvde(struct camera_context *cxt, struct ipm_open_in *in_ptr, struct ipm_open_out *out_ptr);
static cmr_int camera_close_uvde(struct camera_context *cxt);
static cmr_int camera_start_uvde(struct camera_context *cxt, struct img_frm *src);
static cmr_int camera_start_yde(struct camera_context *cxt, struct img_frm *src);
static cmr_int camera_open_snr_uvde(struct camera_context *cxt, struct ipm_open_in *in_ptr, struct ipm_open_out *out_ptr);
static cmr_int camera_close_snr_uvde(struct camera_context *cxt);
static cmr_int camera_start_snr_uv(struct camera_context *cxt, struct img_frm *src);
static cmr_int camera_save_jpg_to_file(cmr_u32 index, cmr_u32 img_fmt, cmr_u32 width, cmr_u32 height, cmr_u32 stream_size, struct img_addr *addr);
static int32_t snp_img_padding(struct img_frm *src, struct img_frm *dst,
						   struct cmr_op_mean *mean);


cmr_int snp_main_thread_proc(struct cmr_msg *message, void* p_data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	cmr_handle                     snp_handle = (cmr_handle)p_data;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;

	if (!message || !p_data) {
		CMR_LOGE("param error");
		goto exit;
	}
	CMR_LOGI("message.msg_type 0x%x, data %p", message->msg_type, message->data);
	switch (message->msg_type) {
	case SNP_EVT_INIT:
		break;
	case SNP_EVT_DEINIT:
		break;
	case SNP_EVT_EXIT:
		break;
	case SNP_EVT_STOP:
		CMR_LOGI("need to stop snp");
		ret = snp_stop_proc(snp_handle);
		cxt->err_code = ret;
		if (ret) {
			CMR_LOGE("fail to stop %ld", ret);
		}
		break;
	case SNP_EVT_START_PROC:
		ret = snp_set_post_proc_param(snp_handle, (struct snapshot_param*)message->data);
		cxt->err_code = ret;
		if (!ret) {
			snp_set_request(snp_handle, TAKE_PICTURE_NEEDED);
			snp_set_status(snp_handle, IDLE);
		} else {
			CMR_LOGE("failed to set param %ld", ret);
		}
		break;
	default:
		CMR_LOGI("don't support msg");
		break;
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

/*void snp_main_thr_proc_exit(struct snp_context * snp_cxt)
{
	CMR_LOGI("done");
}*/

cmr_int snp_postproc_thread_proc(struct cmr_msg *message, void* p_data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)p_data;
	struct buffer_cfg              buf_cfg;
	struct frm_info                *chn_data_ptr;

	if (!message || !p_data) {
		CMR_LOGE("param error");
		ret = CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	chn_data_ptr = (struct frm_info*)message->data;
	CMR_LOGI("message.msg_type 0x%x, data 0x%lx", message->msg_type, (cmr_uint)message->data);
	switch (message->msg_type) {
	case SNP_EVT_HDR_SRC_DONE:
		ret = snp_proc_hdr_src((cmr_handle)p_data, message->data);
		break;
	case SNP_EVT_CHANNEL_DONE:
		ret = snp_post_proc((cmr_handle)p_data, message->data);
		break;
	case SNP_EVT_HDR_POST_PROC:
	{
		struct frm_info  frame;
/*		frame = cxt->cur_frame_info;
		ret = snp_post_proc((cmr_handle)p_data, (void*)&frame);//used for run ipm handle in snp module*/
		ret = snp_post_proc((cmr_handle)p_data, message->data);
	}
		break;
	case SNP_EVT_ANDROID_ZSL_DATA:
		CMR_LOGI("SNP_EVT_ANDROID_ZSL_DATA");
		ret = snp_proc_android_zsl_data((cmr_handle)p_data, message->data);
		break;
	case SNP_EVT_FREE_FRM:
		if (cxt->ops.channel_buff_cfg) {
			struct frm_info frame = *(struct frm_info*)message->data;
			buf_cfg.channel_id = frame.channel_id;
			buf_cfg.base_id = CMR_BASE_ID(frame.frame_id);
			buf_cfg.count = 1;
			buf_cfg.flag           = BUF_FLAG_RUNNING;
			buf_cfg.addr[0].addr_y = frame.yaddr;
			buf_cfg.addr[0].addr_u = frame.uaddr;
			buf_cfg.addr[0].addr_v = frame.vaddr;
			buf_cfg.addr_vir[0].addr_y = frame.yaddr_vir;
			buf_cfg.addr_vir[0].addr_u = frame.uaddr_vir;
			buf_cfg.addr_vir[0].addr_v = frame.vaddr_vir;
			cxt->ops.channel_buff_cfg(cxt->oem_handle, &buf_cfg);
			CMR_LOGI("free frame");
		}
		break;
	case SNP_EVT_EXIT:
		break;
	default:
		break;
	}

exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int snp_notify_thread_proc(struct cmr_msg *message, void* p_data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)p_data;

	if (!message || !p_data) {
		CMR_LOGE("param error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	CMR_LOGI("message.msg_type 0x%x, data 0x%lx", message->msg_type, (cmr_uint)message->data);

	if (SNAPSHOT_CB_MAX > message->msg_type) {
		if (cxt->oem_cb) {
			cxt->oem_cb(cxt->oem_handle, message->msg_type, message->sub_msg_type, message->data);
		} else {
			CMR_LOGE("err, oem cb is null");
		}
	} else {
		CMR_LOGE("don't support this 0x%x", message->msg_type);
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int snp_ipm_cb_handle(cmr_handle snp_handle, void *data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	CMR_MSG_INIT(message);

	message.data = malloc(sizeof(struct img_frm));
	if (!message.data) {
		CMR_LOGE("failed to malloc");
		ret = -CMR_CAMERA_NO_MEM;
		return ret;
	}
	cmr_copy(message.data, data, sizeof(struct img_frm));
	message.alloc_flag = 1;
	message.msg_type = SNP_EVT_HDR_POST_PROC;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	ret = cmr_thread_msg_send(cxt->thread_cxt.post_proc_thr_handle, &message);
	if (ret) {
		CMR_LOGE("failed to send stop msg to main thr %ld", ret);
	}
	return ret;
}

cmr_int snp_scale_cb_handle(cmr_handle snp_handle, void *data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	struct img_frm                 *scale_out_ptr = (struct img_frm*)data;
	if (cxt->req_param.lls_shot_mode || cxt->req_param.is_vendor_hdr) {
		ret = snp_take_picture_done(snp_handle, &cxt->cur_frame_info);
		snp_post_proc_done(snp_handle);
		return ret;
	}
	if (cxt->err_code) {
		CMR_LOGE("err exit");
		sem_post(&cxt->scaler_sync_sm);
		sem_post(&cxt->jpeg_sync_sm);
		return ret;
	}
	if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
		CMR_LOGI("post proc has been cancel");
		ret = CMR_CAMERA_NORNAL_EXIT;
		sem_post(&cxt->scaler_sync_sm);
		sem_post(&cxt->jpeg_sync_sm);
		goto exit;
	}
	if (scale_out_ptr->size.height == cxt->req_param.post_proc_setting.actual_snp_size.height) {
		ret = snp_start_encode(snp_handle, &cxt->cur_frame_info);
		if (ret) {
			CMR_LOGE("failed to start encode %ld", ret);
			sem_post(&cxt->scaler_sync_sm);
			goto exit;
		}
		ret = snp_start_thumb_proc(snp_handle, &cxt->cur_frame_info);
		if (ret) {
			CMR_LOGE("failed to start thumb %ld", ret);
			goto exit;
		}
		ret = snp_redisplay(snp_handle, &cxt->cur_frame_info);
		if (ret) {
			CMR_LOGE("failed to take pic done %ld", ret);
			goto exit;
		}
	} else {
		if (0 != scale_out_ptr->size.width) {
			ret = CMR_CAMERA_NO_SUPPORT;
		} else {
			ret = CMR_CAMERA_FAIL;
		}
		sem_post(&cxt->jpeg_sync_sm);
		CMR_LOGI("don't support %ld", ret);
	}
exit:
	CMR_LOGI("post jpeg sync sm");
	if ((0 == cxt->req_param.jpeg_setting.thum_size.width)
		|| (0 == cxt->req_param.jpeg_setting.thum_size.height)) {
		sem_post(&cxt->scaler_sync_sm);
	}
	if (ret) {
		snp_post_proc_err_exit(snp_handle, ret);
		cxt->err_code = ret;
	}
	CMR_LOGI("done %ld", ret);
	return ret;
}

void snp_post_proc_err_exit(cmr_handle snp_handle, cmr_int err_code)
{
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	cxt->cap_cnt = 0;
	cmr_bzero(&cxt->cur_frame_info, sizeof(cxt->cur_frame_info));
	snp_set_request(snp_handle, TAKE_PICTURE_NO);
	sem_post(&cxt->proc_done_sm);
	if (CODEC_WORKING == snp_get_request(snp_handle)) {
		sem_post(&cxt->jpeg_sync_sm);
	}
	snp_set_status(snp_handle, IDLE);
	if (CMR_CAMERA_NORNAL_EXIT != err_code) {
		ret = snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_TAKE_PICTURE, SNAPSHOT_EXIT_CB_FAILED, NULL, 0);
	}
	CMR_LOGI("done");
}

void snp_post_proc_done(cmr_handle snp_handle)
{
	struct snp_context             *cxt = (struct snp_context*)snp_handle;

	if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
		CMR_LOGI("post proc has been cancel");
		cxt->cap_cnt = 0;
		goto exit;
	}
	CMR_LOGI("wait beging for redisplay sm");
	sem_wait(&cxt->redisplay_sm);
	CMR_LOGI("wait end for redisplay sm");
	if (cxt->cap_cnt >= cxt->req_param.total_num) {
		cxt->cap_cnt = 0;
		snp_set_request(snp_handle, TAKE_PICTURE_NO);
	}
exit:
	CMR_LOGI("post");
	snp_set_status(snp_handle, IDLE);
	sem_post(&cxt->proc_done_sm);
	CMR_LOGI("done");
}

cmr_int snp_jpeg_enc_cb_handle(cmr_handle snp_handle, void *data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	struct jpeg_enc_cb_param       *enc_out_ptr = (struct jpeg_enc_cb_param*)data;
	struct cmr_cap_mem             *mem_ptr = &cxt->req_param.post_proc_setting.mem[cxt->index];
	cmr_s8                         value[PROPERTY_VALUE_MAX];
	struct camera_frame_type       frame_type = {0};

	if (cxt->err_code) {
		CMR_LOGE("error exit");
		sem_post(&cxt->jpeg_sync_sm);
		return ret;
	}
	if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
		CMR_LOGI("post proc has been cancel");
		ret = CMR_CAMERA_NORNAL_EXIT;
		goto exit;
	}
	property_get("debug.camera.save.snpfile", value, "0");
	if (atoi(value) == 5) {
		camera_save_to_file(SNP_ENCODE_STREAM+cxt->cap_cnt, IMG_DATA_TYPE_JPEG,
		cxt->req_param.post_proc_setting.actual_snp_size.width,
		cxt->req_param.post_proc_setting.actual_snp_size.height,
		&mem_ptr->target_jpeg.addr_vir);
	}
	if (enc_out_ptr->total_height == cxt->req_param.post_proc_setting.actual_snp_size.height) {
		if (CAMERA_ISP_TUNING_MODE == cxt->req_param.mode) {
			send_capture_data(0x10,/* jpg */
								cxt->req_param.post_proc_setting.actual_snp_size.width,
								cxt->req_param.post_proc_setting.actual_snp_size.height,
								(char *)mem_ptr->target_jpeg.addr_vir.addr_y,
								enc_out_ptr->stream_size,
								0, 0, 0, 0);
			//ret = camera_save_to_file(isp_get_saved_file_count(snp_handle),
			//							IMG_DATA_TYPE_JPEG,
			//							cxt->req_param.post_proc_setting.actual_snp_size.width,
			//							cxt->req_param.post_proc_setting.actual_snp_size.height,
			//							&mem_ptr->target_jpeg.addr_vir);
			ret = camera_save_jpg_to_file(isp_get_saved_file_count(snp_handle),
							IMG_DATA_TYPE_JPEG,
							cxt->req_param.post_proc_setting.actual_snp_size.width,
							cxt->req_param.post_proc_setting.actual_snp_size.height,
							enc_out_ptr->stream_size,
							&mem_ptr->target_jpeg.addr_vir);

		}
		cxt->jpeg_stream_size = enc_out_ptr->stream_size;
		if (!ret) {
			CMR_LOGI("post beging %d", cxt->jpeg_stream_size);
			sem_post(&cxt->jpeg_sync_sm);
			CMR_LOGI("post end");
		}
		snp_set_status(snp_handle, POST_PROCESSING);
		frame_type.status = ret;
		snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_STATE, SNAPSHOT_EVT_ENC_DONE, (void*)&frame_type, sizeof(struct camera_frame_type));
	} else {
		ret = CMR_CAMERA_NO_SUPPORT;
		CMR_LOGI("don't support");
		goto exit;
	}
#if 0
	if (!cxt->err_code) {
		if (cxt->ops.start_exif_encode) {
			cmr_u32 index = cxt->index;
			struct camera_jpeg_param   enc_param;
			struct snp_exif_param *exif_in_ptr = &cxt->chn_param.exif_in[index];
			struct jpeg_wexif_cb_param  enc_out_param;
			exif_in_ptr->big_pic_stream_src.buf_size = cxt->jpeg_stream_size;
			exif_in_ptr->thumb_stream_src.buf_size = cxt->thumb_stream_size;
			camera_take_snapshot_step(CMR_STEP_WR_EXIF_S);
			ret = cxt->ops.start_exif_encode(cxt->oem_handle, snp_handle, &exif_in_ptr->big_pic_stream_src,
											&exif_in_ptr->thumb_stream_src, NULL, &exif_in_ptr->dst, &enc_out_param);
			camera_take_snapshot_step(CMR_STEP_WR_EXIF_E);
			if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
				CMR_LOGI("snp has been cancel");
				ret = CMR_CAMERA_NORNAL_EXIT;
				goto exit;
			}
			enc_param.need_free = 0;
			enc_param.index = index;
			enc_param.outPtr = (void*)enc_out_param.output_buf_virt_addr;
			enc_param.size = enc_out_param.output_buf_size;
			CMR_LOGI("need free %d stream %d cap cnt %d total num %d", enc_param.need_free, enc_param.size,
					cxt->cap_cnt, cxt->req_param.total_num);
			if (cxt->cap_cnt == cxt->req_param.total_num) {
				enc_param.need_free = 1;
			}
			camera_take_snapshot_step(CMR_STEP_CALL_BACK);
			camera_snapshot_step_statisic();
			snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_ENCODE_PICTURE, SNAPSHOT_EXIT_CB_DONE, (void*)&enc_param, sizeof(struct camera_jpeg_param));
			if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
				CMR_LOGI("snp has been cancel");
				ret = CMR_CAMERA_NORNAL_EXIT;
				goto exit;
			}
#ifdef TEST_MEM_DATA
{
		struct img_addr jpeg_addr;
		jpeg_addr.addr_y = enc_out_param.output_buf_virt_addr;
		camera_save_to_file(SNP_JPEG_STREAM+cxt->cap_cnt, IMG_DATA_TYPE_JPEG,
							cxt->req_param.post_proc_setting.actual_snp_size.width,
							cxt->req_param.post_proc_setting.actual_snp_size.height,
							&jpeg_addr);
}
#endif
		} else {
			CMR_LOGE("err, start enc func is null");
			ret = CMR_CAMERA_FAIL;
		}
		if (!ret) {
			snp_post_proc_done(snp_handle);
		}
	} else {
		ret = cxt->err_code;
	}
#endif
exit:
	CMR_LOGI("done %ld", ret);
	if (ret) {
		sem_post(&cxt->jpeg_sync_sm);
		CMR_LOGI("post");
		snp_post_proc_err_exit(snp_handle, ret);
		cxt->err_code = ret;
	}
	return ret;
}
void snp_jpeg_enc_err_handle(cmr_handle snp_handle)
{
	struct snp_context             *cxt = (struct snp_context*)snp_handle;

	CMR_LOGI("err code %ld", cxt->err_code);
	sem_post(&cxt->jpeg_sync_sm);
	snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_TAKE_PICTURE, SNAPSHOT_EXIT_CB_FAILED, NULL, 0);
}

/*void snp_jpeg_dec_err_handle(cmr_handle snp_handle)
{
	//todo
}*/

cmr_int snp_proc_cb_thread_proc(struct cmr_msg *message, void* p_data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)p_data;

	CMR_LOGI("done %ld", ret);

	if (!message || !p_data) {
		CMR_LOGE("param error");
		goto exit;
	}
	CMR_LOGI("message.msg_type 0x%x, data %p", message->msg_type, message->data);
	switch (message->msg_type) {
	case SNP_EVT_RAW_PROC:
	case SNP_EVT_CVT_RAW_DATA:
		ret = snp_start_isp_next_proc((cmr_handle)cxt, message->data);
		if (CMR_CAMERA_NORNAL_EXIT == ret) {
			snp_cvt_done((cmr_handle)cxt);
			ret = CMR_CAMERA_SUCCESS;
		}
		CMR_LOGI("ret %ld", ret);
		break;
	case SNP_EVT_SC_DONE:
		ret = snp_scale_cb_handle((cmr_handle)cxt, message->data);
		cxt->err_code = ret;
		break;
	case SNP_EVT_JPEG_ENC_DONE:
		ret = snp_jpeg_enc_cb_handle((cmr_handle)cxt, message->data);
		cxt->err_code = ret;
		break;
	case SNP_EVT_JPEG_DEC_DONE:
		break;
	case SNP_EVT_JPEG_ENC_ERR:
		CMR_LOGE("jpeg enc error");
		cxt->err_code = CMR_CAMERA_FAIL;
		snp_jpeg_enc_err_handle((cmr_handle)cxt);
		break;
	case SNP_EVT_JPEG_DEC_ERR:
		CMR_LOGE("jpeg dec error");
		cxt->err_code = CMR_CAMERA_FAIL;
		break;
	case SNP_EVT_HDR_DONE:
		sem_post(&cxt->hdr_sync_sm);
		snp_ipm_cb_handle((cmr_handle)cxt, message->data);
		break;
	default:
		CMR_LOGI("don't support this evt 0x%x", message->msg_type);
		break;
	}
exit:
	return ret;
}

cmr_int snp_secondary_thread_proc(struct cmr_msg *message, void* p_data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)p_data;

	if (!message || !p_data) {
		CMR_LOGE("param error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	CMR_LOGI("message.msg_type 0x%x, data %p", message->msg_type, message->data);

	if (SNAPSHOT_CB_MAX > message->msg_type) {
		if (cxt->oem_cb) {
			cxt->oem_cb(cxt->oem_handle, message->msg_type, message->sub_msg_type, message->data);
		} else {
			CMR_LOGE("err, oem cb is null");
		}
	} else {
		CMR_LOGE("don't support this 0x%x", message->msg_type);
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int snp_start_encode(cmr_handle snp_handle, void *data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *snp_cxt = (struct snp_context*)snp_handle;
	struct snp_channel_param       *chn_param_ptr = &snp_cxt->chn_param;
	struct frm_info                *frm_ptr = (struct frm_info*)data;
	struct snp_jpeg_param          *jpeg_in_ptr;
	cmr_u32                        index = frm_ptr->frame_id - frm_ptr->base;

	if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
		sem_post(&snp_cxt->jpeg_sync_sm);
		CMR_LOGI("post proc has been cancel");
		ret = CMR_CAMERA_NORNAL_EXIT;
		goto exit;
	}
	CMR_PRINT_TIME;
	snp_cxt->index = index;
	jpeg_in_ptr = &chn_param_ptr->jpeg_in[index];
	jpeg_in_ptr->src.data_end = snp_cxt->req_param.post_proc_setting.data_endian;
	if (snp_cxt->ops.start_encode) {
		cmr_s8 value[PROPERTY_VALUE_MAX];
		property_get("debug.camera.save.snpfile", value, "0");
		if (atoi(value) == 4) {
			camera_save_to_file(SNP_ENCODE_SRC_DATA, IMG_DATA_TYPE_YUV420,
								jpeg_in_ptr->src.size.width,
								jpeg_in_ptr->src.size.height,
								&jpeg_in_ptr->src.addr_vir);
		}

#ifdef CONFIG_CAPTURE_DENOISE
		struct camera_context          *cxt = (struct camera_context*)snp_cxt->oem_handle;
		if (cxt->camera_id == 0) {
			ret = camera_start_uvde(cxt, &jpeg_in_ptr->src);
			if (ret != CMR_CAMERA_SUCCESS) {
				CMR_LOGE("camera_start_uvde fail");
				goto exit;
			}
		}
#endif

#ifdef CONFIG_CAMERA_Y_DENOISE
		struct camera_context          *cxt = (struct camera_context*)snp_cxt->oem_handle;
		ret = camera_start_yde(cxt, &jpeg_in_ptr->src);
		if (ret != CMR_CAMERA_SUCCESS) {
			CMR_LOGE("camera_start_yde fail");
			goto exit;
		}
#endif

#ifdef CONFIG_CAMERA_SNR_UV_DENOISE
		struct camera_context		   *cxt = (struct camera_context*)snp_cxt->oem_handle;
		ret = camera_start_snr_uv(cxt, &jpeg_in_ptr->src);
		if (ret != CMR_CAMERA_SUCCESS) {
			CMR_LOGE("camera_start_snr_uv fail");
			goto exit;
		}
#endif

		if (!snp_cxt->req_param.is_video_snapshot) {
			snp_img_padding(&jpeg_in_ptr->src, &jpeg_in_ptr->dst, NULL);
		}
		camera_take_snapshot_step(CMR_STEP_JPG_ENC_S);
		ret = snp_cxt->ops.start_encode(snp_cxt->oem_handle, snp_handle, &jpeg_in_ptr->src,
										&jpeg_in_ptr->dst, &jpeg_in_ptr->mean);
		if (ret) {
			CMR_LOGE("failed to start enc %ld", ret);
		} else {
			snp_set_status(snp_handle, CODEC_WORKING);
		}
	} else {
		CMR_LOGE("err start_encode is null");
		ret = -CMR_CAMERA_FAIL;
	}
	//snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_STATE, SNAPSHOT_EVT_START_ENC, (void*)ret, sizeof(cmr_int));
exit:
	CMR_LOGI("done %ld", ret);
	if (ret) {
		sem_post(&snp_cxt->jpeg_sync_sm);
		snp_cxt->err_code = ret;
	}
	return ret;
}

cmr_int snp_start_encode_thumb(cmr_handle snp_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *snp_cxt = (struct snp_context*)snp_handle;
	struct snp_channel_param       *chn_param_ptr = &snp_cxt->chn_param;
	struct snp_jpeg_param          *jpeg_in_ptr;
	cmr_u32                        index = snp_cxt->index;

	if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
		CMR_LOGI("post proc has been cancel");
		return CMR_CAMERA_NORNAL_EXIT;
	}

	if (0 == snp_cxt->req_param.jpeg_setting.thum_size.width ||
		0 == snp_cxt->req_param.jpeg_setting.thum_size.height) {
		CMR_LOGI("don't include thumb");
		goto exit;
	}
	camera_take_snapshot_step(CMR_STEP_THUM_ENC_S);
	jpeg_in_ptr = &chn_param_ptr->thumb_in[index];
	jpeg_in_ptr->src.data_end = snp_cxt->req_param.post_proc_setting.data_endian;
	jpeg_in_ptr->dst.data_end = snp_cxt->req_param.post_proc_setting.data_endian;
	if (snp_cxt->ops.start_encode) {
		ret = snp_cxt->ops.start_encode(snp_cxt->oem_handle, snp_handle, &jpeg_in_ptr->src,
										&jpeg_in_ptr->dst, &jpeg_in_ptr->mean);
		camera_take_snapshot_step(CMR_STEP_THUM_ENC_E);
		if (ret) {
			CMR_LOGE("failed to start thumb enc %ld", ret);
		} else {
			snp_cxt->thumb_stream_size = (cmr_u32)((unsigned long)jpeg_in_ptr->dst.reserved);
			cmr_s8 value[PROPERTY_VALUE_MAX];
			property_get("debug.camera.save.snpfile", value, "0");
			if (atoi(value) == 7) {
				camera_save_to_file(SNP_THUMB_STREAM+snp_cxt->cap_cnt, IMG_DATA_TYPE_JPEG,
									jpeg_in_ptr->src.size.width,
									jpeg_in_ptr->src.size.height,
									&jpeg_in_ptr->dst.addr_vir);
			}
		}
	//	snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_STATE, SNAPSHOT_EVT_ENC_THUMB_DONE, (void*)ret, sizeof(cmr_int));
	} else {
		CMR_LOGE("err start_encode is null");
		ret = -CMR_CAMERA_FAIL;
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int snp_start_decode_sync(cmr_handle snp_handle, void *data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *snp_cxt = (struct snp_context*)snp_handle;
	struct frm_info                *frm_ptr = (struct frm_info*)data;
	struct snp_channel_param       *chn_param_ptr = &snp_cxt->chn_param;
	cmr_u32                        index = frm_ptr->frame_id - frm_ptr->base;
	struct img_frm                 src, dst;
	struct cmr_op_mean             mean;
	cmr_s8                         value[PROPERTY_VALUE_MAX];

    CMR_LOGI("index %d phy addr 0x%lx 0x%lx 0x%lx vrit addr 0x%lx 0x%lx 0x%lx length %d",
			index, chn_param_ptr->jpeg_dec_in[index].src.addr_phy.addr_y,
			chn_param_ptr->jpeg_dec_in[index].src.addr_phy.addr_u,
			chn_param_ptr->jpeg_dec_in[index].src.addr_phy.addr_v,
			chn_param_ptr->jpeg_dec_in[index].src.addr_vir.addr_y,
			chn_param_ptr->jpeg_dec_in[index].src.addr_vir.addr_u,
			chn_param_ptr->jpeg_dec_in[index].src.addr_vir.addr_v,
			frm_ptr->length);

	property_get("debug.camera.save.snpfile", value, "0");
	if (atoi(value) == 1) {
		camera_save_to_file(SNP_CHN_OUT_DATA, IMG_DATA_TYPE_JPEG,
							chn_param_ptr->chn_frm[frm_ptr->frame_id-frm_ptr->base].size.width,
							chn_param_ptr->chn_frm[frm_ptr->frame_id-frm_ptr->base].size.height,
							&chn_param_ptr->jpeg_dec_in[index].src.addr_vir);
	}
	if (snp_cxt->ops.start_decode) {
		src = chn_param_ptr->jpeg_dec_in[index].src;
		dst = chn_param_ptr->jpeg_dec_in[index].dst;
		mean = chn_param_ptr->jpeg_dec_in[index].mean;
		mean.is_sync = 1;
		src.reserved = (void*)((unsigned long)frm_ptr->length);
		ret = snp_cxt->ops.start_decode(snp_cxt->oem_handle, snp_handle, &src, &dst, &mean);
		if (ret) {
			CMR_LOGE("failed to start decode proc %ld", ret);
		}
		snp_cxt->cvt.out_frame = dst;
		snp_cxt->index = index;
	} else {
		CMR_LOGE("err start_decode is null");
		ret = -CMR_CAMERA_FAIL;
	}
	//snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_STATE, SNAPSHOT_EVT_DEC_DONE, (void*)ret, sizeof(cmr_int));
	return ret;
}

cmr_int snp_start_decode(cmr_handle snp_handle, void *data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *snp_cxt = (struct snp_context*)snp_handle;
	struct frm_info                *frm_ptr = (struct frm_info*)data;
	struct snp_channel_param       *chn_param_ptr = &snp_cxt->chn_param;
	cmr_u32                        index = frm_ptr->frame_id - frm_ptr->base;
	struct img_frm                 src, dst;
	struct cmr_op_mean             mean;

	if (snp_cxt->ops.start_decode) {
		src = chn_param_ptr->jpeg_dec_in[index].src;
		dst = chn_param_ptr->jpeg_dec_in[index].dst;
		mean = chn_param_ptr->jpeg_dec_in[index].mean;
		mean.is_sync = 0;
		ret = snp_cxt->ops.start_decode(snp_cxt->oem_handle, snp_handle, &src, &dst, &mean);
		if (ret) {
			CMR_LOGE("failed to start decode proc %ld", ret);
		}
		snp_cxt->cvt.out_frame = dst;
		snp_cxt->index = index;
	} else {
		CMR_LOGE("err start_decode is null");
		ret = -CMR_CAMERA_FAIL;
	}
	//snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_STATE, SNAPSHOT_EVT_START_DEC, (void*)ret, sizeof(cmr_int));
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int snp_start_rot(cmr_handle snp_handle, void *data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *snp_cxt = (struct snp_context*)snp_handle;
	struct frm_info                *frm_ptr = (struct frm_info*)data;
	struct snp_channel_param       *chn_param_ptr = &snp_cxt->chn_param;
	cmr_u32                        index = frm_ptr->frame_id - frm_ptr->base;
	struct img_frm                 src, dst;
	struct cmr_op_mean             mean;
	cmr_s8                         value[PROPERTY_VALUE_MAX];

	if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
		CMR_LOGI("post proc has been cancel");
		ret = CMR_CAMERA_NORNAL_EXIT;
		goto exit;
	}

	if (snp_cxt->ops.start_rot) {
		src = chn_param_ptr->rot[index].src_img;
		dst = chn_param_ptr->rot[index].dst_img;
		src.data_end = snp_cxt->req_param.post_proc_setting.data_endian;
		dst.data_end = snp_cxt->req_param.post_proc_setting.data_endian;
		mean.rot = chn_param_ptr->rot[index].angle;
		if(snp_cxt->req_param.is_zsl_snapshot && (1 == snp_cxt->camera_id) && 1 == snp_cxt->req_param.flip_on){
			if(mean.rot == IMG_ANGLE_270){
				mean.rot = IMG_ANGLE_90;
			}else if(mean.rot == IMG_ANGLE_90){
				mean.rot = IMG_ANGLE_270;
			}
		}
		ret = snp_cxt->ops.start_rot(snp_cxt->oem_handle, snp_handle, &src, &dst, &mean);
	} else {
		CMR_LOGE("err start_rot is null");
		ret = -CMR_CAMERA_FAIL;
	}
	property_get("debug.camera.save.snpfile", value, "0");
	if (atoi(value) == 2) {
		camera_save_to_file(SNP_ROT_DATA, IMG_DATA_TYPE_YUV420,
							dst.size.width,
							dst.size.height,
							&dst.addr_vir);
	}
	//snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_STATE, SNAPSHOT_EVT_START_ROT, (void*)ret, sizeof(cmr_int));
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int snp_start_scale(cmr_handle snp_handle, void *data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *snp_cxt = (struct snp_context*)snp_handle;
	struct frm_info                *frm_ptr = (struct frm_info*)data;
	struct snp_channel_param       *chn_param_ptr = &snp_cxt->chn_param;
	cmr_u32                        index = frm_ptr->frame_id - frm_ptr->base;
	struct img_frm                 src, dst;
	struct cmr_op_mean             mean;

	if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
		CMR_LOGI("post proc has been cancel");
		ret = CMR_CAMERA_NORNAL_EXIT;
		goto exit;
	}
	camera_take_snapshot_step(CMR_STEP_SC_S);
	if (snp_cxt->ops.start_scale) {
		src = chn_param_ptr->scale[index].src_img;
		dst = chn_param_ptr->scale[index].dst_img;
		mean.slice_height = chn_param_ptr->scale[index].slice_height;
		mean.is_sync = 0;
		src.data_end = snp_cxt->req_param.post_proc_setting.data_endian;
		dst.data_end = snp_cxt->req_param.post_proc_setting.data_endian;

		ret = snp_cxt->ops.start_scale(snp_cxt->oem_handle, snp_handle, &src, &dst, &mean);
	} else {
		CMR_LOGE("err start_scale is null");
		ret = -CMR_CAMERA_FAIL;
	}
//	snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_STATE, SNAPSHOT_EVT_START_SCALE, (void*)ret, sizeof(cmr_int));
exit:
	CMR_LOGI("done %ld", ret);
	if (ret) {
		snp_cxt->err_code = ret;
		sem_post(&snp_cxt->scaler_sync_sm);
	}
	return ret;
}

int32_t snp_img_scaling_down(struct img_frm *src, struct img_frm *dst,
						   struct cmr_op_mean *mean)
{
	UNUSED(mean);

	cmr_u8 *dst_y_buf;
	cmr_u8 *dst_uv_buf;
	cmr_u8 *src_y_buf;
	cmr_u8 *src_uv_buf;
	cmr_u32  src_w;
	cmr_u32  src_h;
	cmr_u32  dst_w;
	cmr_u32  dst_h;
	cmr_u32  dst_uv_w;
	cmr_u32  dst_uv_h;
	cmr_u32  cur_w = 0;
	cmr_u32  cur_h = 0;
	cmr_u32  cur_size = 0;
	cmr_u32  cur_byte = 0;
	cmr_u32  ratio_w;
	cmr_u32  ratio_h;
	uint16_t i,j;
	if (NULL == dst || NULL == src) {
		return -1;
	}
	dst_y_buf     = (cmr_u8*)dst->addr_vir.addr_y;
	dst_uv_buf    = (cmr_u8*)dst->addr_vir.addr_u;
	src_y_buf     = (cmr_u8*)src->addr_vir.addr_y;
	src_uv_buf    = (cmr_u8*)src->addr_vir.addr_u;
	src_w         = src->size.width;
	src_h         = src->size.height;
	dst_w         = dst->size.width;
	dst_h         = dst->size.height;
	dst_uv_w      = dst_w / 2;
	dst_uv_h      = dst_h / 2;
	ratio_w       = (src_w <<10) / dst_w;
	ratio_h       = (src_h <<10) / dst_h;
	for (i=0 ; i<dst_h; i++) {
		cur_h = (ratio_h * i) >> 10;
		cur_size = cur_h *src_w;
		for (j=0; j<dst_w; j++) {
			cur_w = (ratio_w *j) >> 10;
			*dst_y_buf++ = src_y_buf[cur_size +cur_w];
		}
	}
	for (i=0 ; i<dst_uv_h; i++) {
		cur_h = (ratio_h * i) >> 10;
		cur_size = cur_h *(src_w/2) *2;
		for (j=0; j<dst_uv_w; j++) {
			cur_w = (ratio_w *j) >> 10;
			cur_byte = cur_size +cur_w *2;
			*dst_uv_buf++ = src_uv_buf[cur_byte];       //u
			*dst_uv_buf++ = src_uv_buf[cur_byte +1];    //v
		}
	}
	CMR_LOGI("done");
	return 0;
}

static int32_t snp_img_padding(struct img_frm *src, struct img_frm *dst,
						   struct cmr_op_mean *mean)
{
	UNUSED(mean);

	cmr_u8 *last_y_buf;
	cmr_u8 *last_uv_buf;
	cmr_u8 *dst_y_buf;
	cmr_u8 *dst_uv_buf;
	cmr_u8 *src_y_buf;
	cmr_u8 *src_uv_buf;
	cmr_u32  src_w;
	cmr_u32  src_h;
	uint16_t i;
	cmr_u32 padding_lines = 0;

	if (NULL == dst || NULL == src) {
		return -1;
	}

	src_y_buf     = (cmr_u8*)src->addr_vir.addr_y;
	src_uv_buf    = (cmr_u8*)src->addr_vir.addr_u;
	src_w         = src->size.width;
	src_h         = src->size.height;

	if (0 == (src_h & 0xf)) {
		CMR_LOGI("no padding");
		return 0;
	}

	last_y_buf = src_y_buf + src_w * (src_h - 1);
	dst_y_buf = src_y_buf + src_w * src_h;

	//for y
	CMR_LOGI("y padding_lines %d src_h %d", padding_lines, src_h);
	for (i=0; i<padding_lines; i++) {
		CMR_LOGI("dst_y_buf %p last_y_buf %p", dst_y_buf, last_y_buf);
		memcpy(dst_y_buf, last_y_buf, src_w);
		dst_y_buf += src_w;
	}

	last_uv_buf = src_uv_buf + (src_w) * (src_h / 2 - 1);
	dst_uv_buf = src_uv_buf + src_w * src_h / 2;

	padding_lines = 8 - ((src_h / 2) % 8);
	CMR_LOGI("uv padding_lines %d src_h %d", padding_lines, src_h);
	for (i=0; i<padding_lines; i++) {
		CMR_LOGI("dst_uv_buf %p last_uv_buf %p", dst_uv_buf, last_uv_buf);
		memcpy(dst_uv_buf, last_uv_buf, src_w);
		dst_uv_buf += src_w;
	}

	CMR_LOGI("done");
	return 0;
}

cmr_int snp_start_convet_thumb(cmr_handle snp_handle, void *data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *snp_cxt = (struct snp_context*)snp_handle;
	struct frm_info                *frm_ptr = (struct frm_info*)data;
	struct snp_channel_param       *chn_param_ptr = &snp_cxt->chn_param;
	cmr_u32                        index = frm_ptr->frame_id - frm_ptr->base;
	struct img_frm                 src, dst;
	struct cmr_op_mean             mean;
	cmr_s8                         value[PROPERTY_VALUE_MAX];

	if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
		CMR_LOGI("post proc has been cancel");
		return CMR_CAMERA_NORNAL_EXIT;
	}

	if (snp_cxt->ops.start_scale) {
		src = chn_param_ptr->convert_thumb[index].src_img;
		dst = chn_param_ptr->convert_thumb[index].dst_img;
		mean.slice_height = chn_param_ptr->convert_thumb[index].slice_height;
		mean.is_sync = 1;
		src.data_end = snp_cxt->req_param.post_proc_setting.data_endian;
		dst.data_end = snp_cxt->req_param.post_proc_setting.data_endian;
		camera_take_snapshot_step(CMR_STEP_CVT_THUMB_S);
		if ((src.size.width != dst.size.width) || (src.size.height != dst.size.height)) {
			if (snp_cxt->req_param.is_video_snapshot || snp_cxt->req_param.is_zsl_snapshot) {
				ret = snp_img_scaling_down(&src, &dst, &mean);
				cmr_snapshot_memory_flush(snp_cxt);
			} else {
			ret = snp_cxt->ops.start_scale(snp_cxt->oem_handle, snp_handle, &src, &dst, &mean);
			}
		} else {
			CMR_LOGI("don't need to scale");
		}
		camera_take_snapshot_step(CMR_STEP_CVT_THUMB_E);
		property_get("debug.camera.save.snpfile", value, "0");
		if (atoi(value) == 6) {
			camera_save_to_file(SNP_THUMB_DATA, IMG_DATA_TYPE_YUV420,
								src.size.width,
								src.size.height,
								&dst.addr_vir);
		}
	} else {
		CMR_LOGE("err start_scale is null");
		ret = -CMR_CAMERA_FAIL;
	}
	//snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_STATE, SNAPSHOT_EVT_CONVERT_THUMB_DONE, (void*)ret, sizeof(cmr_int));
	CMR_LOGI("done %ld", ret);
	return ret;
}

static int camera_get_system_time(char *datetime)
{
	time_t timep;
	struct tm *p;
	time(&timep);
	p = localtime(&timep);
	sprintf(datetime, "%04d%02d%02d%02d%02d%02d",
			(1900 + p->tm_year),
			(1 + p->tm_mon),
			p->tm_mday,
			p->tm_hour,
			p->tm_min,
			p->tm_sec);

	CMR_LOGI("datatime = %s", datetime);

	return 0;
}

struct isp_awbc_cfg_test {
	uint32_t r_gain;
	uint32_t g_gain;
	uint32_t b_gain;
	uint32_t r_offset;
	uint32_t g_offset;
	uint32_t b_offset;
};

static int camera_save_raw_to_file(char *name, uint32_t img_fmt,
					uint32_t width, uint32_t height, char *addr)
{
#define FILE_DIR "/data/misc/media/"

#define FILE_NAME_LEN 200
	int ret = CMR_CAMERA_SUCCESS;
	char file_name[FILE_NAME_LEN] = {0};
	char tmp_str[20] = {0};
	FILE *fp = NULL;
	uint32_t gain = 0;
	uint32_t shutter = 0;
	struct isp_awbc_cfg_test awbc_cfg;
//	void *   isp_handle = ispvideo_GetIspHandle();
	uint32_t pos = 0;

	read_sensor_gain(&gain);
	read_sensor_shutter(&shutter);
//	read_otp_awb_gain(isp_handle, (void*)&awbc_cfg);
//	read_position(isp_handle, &pos);

	CMR_LOGI("gain = %d shutter = %d\n",gain, shutter);
	CMR_LOGI("name %s, format %d, width %d, heght %d",
			name, img_fmt, width, height);

	strcpy(file_name, FILE_DIR);
	sprintf(tmp_str, "%d", width);
	strcat(file_name, tmp_str);
	strcat(file_name, "X");
	sprintf(tmp_str, "%d", height);
	strcat(file_name, tmp_str);

	strcat(file_name, "_");
	sprintf(tmp_str, "%s", name);
	strcat(file_name, tmp_str);

	strcat(file_name, "_");
	strcat(file_name, "gain");
	strcat(file_name, "_");
	sprintf(tmp_str, "%d", gain);
	strcat(file_name, tmp_str);
	strcat(file_name, "_");
	strcat(file_name, "shutter");
	strcat(file_name, "_");
	sprintf(tmp_str, "%d", shutter);
	strcat(file_name, tmp_str);

	strcat(file_name, "_");
	strcat(file_name, "awbgain");
	strcat(file_name, "_");
	strcat(file_name, "r");
	strcat(file_name, "_");
	sprintf(tmp_str, "%d", awbc_cfg.r_gain);
	strcat(file_name, tmp_str);
	strcat(file_name, "_");
	strcat(file_name, "g");
	strcat(file_name, "_");
	sprintf(tmp_str, "%d", awbc_cfg.g_gain);
	strcat(file_name, tmp_str);
	strcat(file_name, "_");
	strcat(file_name, "b");
	strcat(file_name, "_");
	sprintf(tmp_str, "%d", awbc_cfg.b_gain);
	strcat(file_name, tmp_str);
	memset(tmp_str, 0, sizeof(tmp_str));
	strcat(file_name, "_");
	strcat(file_name, "afpos");
	strcat(file_name, "_");
	sprintf(tmp_str, "%d", pos);
	strcat(file_name, tmp_str);
/*
	memset(tmp_str, 0, sizeof(tmp_str));
	strcat(file_name, "_");
	strcat(file_name, "ct");
	strcat(file_name, "_");
	sprintf(tmp_str, "%ld", isp_cur_ct);
	strcat(file_name, tmp_str);
	memset(tmp_str, 0, sizeof(tmp_str));
	strcat(file_name, "_");
	strcat(file_name, "bv");
	strcat(file_name, "_");
	sprintf(tmp_str, "%ld", isp_cur_bv);
	strcat(file_name, tmp_str);
*/
	strcat(file_name, ".mipi_raw");
	CMR_LOGI("file name %s", file_name);

	fp = fopen(file_name, "wb");
	if (NULL == fp) {
		CMR_LOGE("can not open file: %s errno = %d\n", file_name, errno);
		return -1;
	}

	fwrite((void *)addr, 1, (uint32_t) (width * height * 5 / 4), fp);
	fclose(fp);

	return 0;

}


#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
extern uint32_t isp_cur_bv;
extern uint32_t isp_cur_ct;
#endif

static cmr_int camera_save_jpg_to_file(cmr_u32 index, cmr_u32 img_fmt, cmr_u32 width, cmr_u32 height, cmr_u32 stream_size, struct img_addr *addr)
{
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
#if 1
#define FILE_DIR "/data/misc/media/"
#else
#define FILE_DIR "/storage/sdcard0/DCIM/Camera/raw/"
#endif
#define FILE_NAME_LEN 200
	int ret = CMR_CAMERA_SUCCESS;
	char file_name[FILE_NAME_LEN] = {0};
	char tmp_str[20] = {0};
	FILE *fp = NULL;
	uint32_t gain = 0;
	uint32_t shutter = 0;
	char datetime[15] = {0};
	struct isp_awbc_cfg_test awbc_cfg;
	void *   isp_handle = ispvideo_GetIspHandle();
	uint32_t pos = 0;

	read_sensor_gain(&gain);
	read_sensor_shutter(&shutter);
	read_otp_awb_gain(isp_handle, (void*)&awbc_cfg);
	read_position(isp_handle, &pos);
	CMR_LOGI("gain = %d shutter = %d\n",gain, shutter);
	CMR_LOGI("index %d, format %d, width %d, heght %d",
			index, img_fmt, width, height);

	camera_get_system_time(datetime);

	strcpy(file_name, FILE_DIR);
	sprintf(tmp_str, "%d", width);
	strcat(file_name, tmp_str);
	strcat(file_name, "X");
	sprintf(tmp_str, "%d", height);
	strcat(file_name, tmp_str);

	strcat(file_name, "_");
	sprintf(tmp_str, "%s", datetime);
	//sprintf(tmp_str, "%d", index);
	strcat(file_name, tmp_str);

	strcat(file_name, "_");
	strcat(file_name, "gain");
	strcat(file_name, "_");
	sprintf(tmp_str, "%d", gain);
	strcat(file_name, tmp_str);
	strcat(file_name, "_");
	strcat(file_name, "shutter");
	strcat(file_name, "_");
	sprintf(tmp_str, "%d", shutter);
	strcat(file_name, tmp_str);

	strcat(file_name, "_");
	strcat(file_name, "awbgain");
	strcat(file_name, "_");
	strcat(file_name, "r");
	strcat(file_name, "_");
	sprintf(tmp_str, "%d", awbc_cfg.r_gain);
	strcat(file_name, tmp_str);
	strcat(file_name, "_");
	strcat(file_name, "g");
	strcat(file_name, "_");
	sprintf(tmp_str, "%d", awbc_cfg.g_gain);
	strcat(file_name, tmp_str);
	strcat(file_name, "_");
	strcat(file_name, "b");
	strcat(file_name, "_");
	sprintf(tmp_str, "%d", awbc_cfg.b_gain);
	strcat(file_name, tmp_str);
	memset(tmp_str, 0, sizeof(tmp_str));
	strcat(file_name, "_");
	strcat(file_name, "afpos");
	strcat(file_name, "_");
	sprintf(tmp_str, "%d", pos);
	strcat(file_name, tmp_str);


	memset(tmp_str, 0, sizeof(tmp_str));
	strcat(file_name, "_");
	strcat(file_name, "ct");
	strcat(file_name, "_");
	sprintf(tmp_str, "%d", isp_cur_ct);
	strcat(file_name, tmp_str);
	memset(tmp_str, 0, sizeof(tmp_str));
	strcat(file_name, "_");
	strcat(file_name, "bv");
	strcat(file_name, "_");
	sprintf(tmp_str, "%d", isp_cur_bv);
	strcat(file_name, tmp_str);


	strcat(file_name, ".jpg");
	CMR_LOGI("file name %s", file_name);

	fp = fopen(file_name, "wb");
	if (NULL == fp) {
		CMR_LOGI("can not open file: %s \n", file_name);
		return 0;
	}

	fwrite((void*)addr->addr_y, 1, stream_size, fp);
	fclose(fp);
#endif
	return 0;
}

cmr_int snp_start_isp_proc(cmr_handle snp_handle, void *data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *snp_cxt = (struct snp_context*)snp_handle;
	struct frm_info                *frm_ptr = (struct frm_info*)data;
	struct snp_channel_param       *chn_param_ptr = &snp_cxt->chn_param;
	struct raw_proc_param          isp_in_param;
	cmr_u32                        index = frm_ptr->frame_id - frm_ptr->base;

	char *raw_pic = NULL;
	cmr_u16                        trim_start_x;
	cmr_u16                        trim_start_y;
	cmr_u16                        trim_width;
	cmr_u16                        trim_height;
	struct camera_context 		   *cmr_cxt = (struct camera_context *)snp_cxt->oem_handle;
	struct sensor_exp_info         *sensor_info = &snp_cxt->sensor_info;
	trim_start_x = sensor_info->mode_info[cmr_cxt->snp_cxt.snapshot_sn_mode].trim_start_x;
	trim_start_y = sensor_info->mode_info[cmr_cxt->snp_cxt.snapshot_sn_mode].trim_start_y;
	trim_width   = sensor_info->mode_info[cmr_cxt->snp_cxt.snapshot_sn_mode].trim_width;
	trim_height  = sensor_info->mode_info[cmr_cxt->snp_cxt.snapshot_sn_mode].trim_height;

	CMR_LOGV("LIKE:%d, %d, %d, %d, %d", sensor_info->mode_info[cmr_cxt->snp_cxt.snapshot_sn_mode].mode,
										sensor_info->mode_info[cmr_cxt->snp_cxt.snapshot_sn_mode].trim_start_x,
										sensor_info->mode_info[cmr_cxt->snp_cxt.snapshot_sn_mode].trim_start_y,
										sensor_info->mode_info[cmr_cxt->snp_cxt.snapshot_sn_mode].trim_width,
										sensor_info->mode_info[cmr_cxt->snp_cxt.snapshot_sn_mode].trim_height);
	if (snp_cxt->ops.raw_proc) {
		isp_in_param = chn_param_ptr->isp_proc_in[index];
		isp_in_param.slice_num = 1;
		chn_param_ptr->isp_process[index].slice_num = 1;
		if (CAMERA_ISP_TUNING_MODE == snp_cxt->req_param.mode) {
			cmr_u32              raw_format, raw_pixel_width;
			struct cmr_cap_mem   *mem_ptr = &snp_cxt->req_param.post_proc_setting.mem[snp_cxt->index];
			raw_pixel_width = snp_cxt->sensor_info.sn_interface.pixel_width;
			if (raw_pixel_width < SENSOR_DEFAULT_PIX_WIDTH)
				raw_pixel_width = SENSOR_DEFAULT_PIX_WIDTH;
			if (snp_cxt->sensor_info.sn_interface.type) {
				isp_in_param.src_frame.fmt = ISP_DATA_CSI2_RAW10;
				raw_format=0x08;
			} else {
				isp_in_param.src_frame.fmt = ISP_DATA_NORMAL_RAW10;
				raw_format=0x04;
			}

			raw_pic = (char *)mem_ptr->cap_raw.addr_vir.addr_y;
			if (0 != trim_start_y) {
				raw_pic += mem_ptr->cap_raw.size.width * trim_start_y * raw_pixel_width /8;
			}

			send_capture_data(raw_format,/* raw */
								mem_ptr->cap_raw.size.width,
								mem_ptr->cap_raw.size.height - trim_start_y,
								raw_pic,
								mem_ptr->cap_raw.size.width*(mem_ptr->cap_raw.size.height - trim_start_y)*raw_pixel_width /8,
								0, 0, 0, 0);
#if 0
			camera_save_to_file(isp_get_saved_file_count(snp_handle),
								IMG_DATA_TYPE_RAW,
								mem_ptr->cap_raw.size.width,
								mem_ptr->cap_raw.size.height,
								&mem_ptr->cap_raw.addr_vir);
#else
			if (CAMERA_ISP_TUNING_MODE == snp_cxt->req_param.mode) {
				char datetime[15] = {0};
				CMR_LOGI("ISP save raw to file");
				camera_get_system_time(datetime);
				camera_save_raw_to_file(datetime,
										IMG_DATA_TYPE_RAW,
										mem_ptr->cap_raw.size.width,
										mem_ptr->cap_raw.size.height - trim_start_y,
										raw_pic);
			}
#endif
		}
		ret = snp_cxt->ops.raw_proc(snp_cxt->oem_handle, snp_handle, &isp_in_param);
		if (ret) {
			CMR_LOGE("failed to start isp proc %ld", ret);
		}
		chn_param_ptr->isp_process[index].frame_info = *frm_ptr;
		snp_cxt->cvt.out_frame = chn_param_ptr->isp_proc_in[index].dst_frame;
	} else {
		CMR_LOGE("err raw_proc is null");
		ret = -CMR_CAMERA_FAIL;
	}
	//snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_STATE, SNAPSHOT_EVT_START_ISP, (void*)ret, sizeof(cmr_int));
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int snp_start_isp_next_proc(cmr_handle snp_handle, void *data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *snp_cxt = (struct snp_context*)snp_handle;
	struct ips_out_param           *isp_out_ptr = (struct ips_out_param*)data;
	struct raw_proc_param          isp_in_param;
	struct snp_channel_param       *chn_param_ptr = &snp_cxt->chn_param;
	struct process_status          *process_ptr;
	struct img_frm                 src, dst;
	struct cmr_op_mean             mean;
	cmr_u32                        index = snp_cxt->index;
	cmr_u32                        slice_out, dst_height, dst_width;
	cmr_u32                        offset;

	if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
		CMR_LOGI("post proc has been cancel");
		snp_set_status(snp_handle, IDLE);
		ret = CMR_CAMERA_NORNAL_EXIT;
		return ret;
	}
	cmr_copy((void*)&isp_in_param, (void*)&chn_param_ptr->isp_proc_in[index], sizeof(struct raw_proc_param));
	process_ptr = &chn_param_ptr->isp_process[index];
	process_ptr->slice_height_out += isp_out_ptr->output_height;
	slice_out = process_ptr->slice_height_out;
	dst_width = chn_param_ptr->isp_proc_in[index].dst_frame.size.width;
	dst_height = chn_param_ptr->isp_proc_in[index].dst_frame.size.height;
	chn_param_ptr->isp_process[index].slice_num++;
	if (slice_out == dst_height) {
		CMR_LOGI("isp proc done");
		return CMR_CAMERA_NORNAL_EXIT;
	} else if (slice_out + process_ptr->slice_height_in < dst_height) {
		isp_in_param.src_slice_height = process_ptr->slice_height_in;
	} else {
		isp_in_param.src_slice_height = dst_height - slice_out;
	}
	offset = process_ptr->slice_height_out*dst_width;
	isp_in_param.src_frame.addr_phy.addr_y += ((offset * snp_cxt->sensor_info.sn_interface.pixel_width) >> 3);
	isp_in_param.slice_num = chn_param_ptr->isp_process[index].slice_num;
	isp_in_param.dst_frame.addr_phy.addr_y = chn_param_ptr->isp_proc_in[index].dst_frame.addr_phy.addr_y + offset;
	isp_in_param.dst_frame.addr_phy.addr_u = chn_param_ptr->isp_proc_in[index].dst_frame.addr_phy.addr_u + (offset >> 1);
	if (snp_cxt->ops.raw_proc) {
		ret = snp_cxt->ops.raw_proc(snp_cxt->oem_handle, snp_handle, &isp_in_param);
		if (ret) {
			CMR_LOGE("failed to start isp proc %ld", ret);
		}
	} else {
		CMR_LOGE("err raw_proc is null");
		ret = -CMR_CAMERA_FAIL;
	}
	//snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_STATE, SNAPSHOT_EVT_START_ISP_NEXT, (void*)ret, sizeof(cmr_int));
	CMR_LOGI("done %ld", ret);
	return ret;
}

void snp_wait_cvt_done(cmr_handle snp_handle)
{
	struct snp_context             *cxt = (struct snp_context*)snp_handle;

	CMR_LOGI("waiting begin");
	sem_wait(&cxt->cvt.cvt_sync_sm);
	CMR_LOGI("wating end");
}

void snp_cvt_done(cmr_handle snp_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	struct snp_channel_param       *chn_param_ptr = &cxt->chn_param;
	CMR_MSG_INIT(message);

	sem_post(&cxt->cvt.cvt_sync_sm);
//	snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_STATE, SNAPSHOT_EVT_CVT_DONE, (void*)ret, sizeof(cmr_int));
	CMR_LOGI("cvt done");
}

cmr_int snp_start_cvt(cmr_handle snp_handle, void *data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	struct frm_info                *frm_ptr = (struct frm_info*)data;

	cxt->cur_frame_info = *frm_ptr;
	if (IMG_DATA_TYPE_JPEG == frm_ptr->fmt) {
		ret = snp_start_decode_sync(snp_handle, data);
	//	snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_STATE, SNAPSHOT_EVT_START_CVT, (void*)ret, sizeof(cmr_int));
		snp_cvt_done(snp_handle);
	} else if (IMG_DATA_TYPE_RAW == frm_ptr->fmt) {
		ret = snp_start_isp_proc(snp_handle, data);
//		snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_STATE, SNAPSHOT_EVT_START_CVT, (void*)ret, sizeof(cmr_int));
	}
	if (ret) {
		CMR_LOGE("failed to start cvt %ld", ret);
		goto exit;
	}
	cxt->index = frm_ptr->frame_id - frm_ptr->base;
exit:
	return ret;
}

cmr_int snp_cvt_thread_proc(struct cmr_msg *message, void* p_data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	cmr_handle                     snp_handle = (cmr_handle)p_data;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	struct frm_info                *frm_ptr;

	if (!message || !p_data) {
		CMR_LOGE("param error");
		goto exit;
	}
	CMR_LOGI("message.msg_type 0x%x, data 0x%lx", message->msg_type, (cmr_uint)message->data);
	frm_ptr = (struct frm_info*)message->data;
	switch (message->msg_type) {
	case SNP_EVT_START_CVT:
		cxt->cvt.trigger = message->sub_msg_type;
		cxt->err_code = snp_start_cvt(snp_handle, message->data);
		snp_wait_cvt_done(snp_handle);
		break;
	case SNP_EVT_EXIT:
/*		sem_post(&cxt->cvt.cvt_sync_sm);
		CMR_LOGI("exit");*/
		break;
	default:
		CMR_LOGI("don't support msg");
		break;
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int snp_write_exif(cmr_handle snp_handle, void *data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	struct frm_info                *frame = (struct frm_info*)data;
	cmr_u32                        index = 0;
	struct camera_frame_type       frame_type = {0};
	cmr_s32                         sm_val = 0;

	if (!data) {
		CMR_LOGE("param error");
		return -CMR_CAMERA_INVALID_PARAM;
	}
	CMR_LOGI("wait beging");
	sem_wait(&cxt->scaler_sync_sm);
	CMR_LOGI("wait beging 1");
	sem_wait(&cxt->jpeg_sync_sm);
	CMR_LOGI("wait done");

	if (cxt->ops.channel_free_frame) {
		index += cxt->cur_frame_info.base;
		ret = cxt->ops.channel_free_frame(cxt->oem_handle, cxt->req_param.channel_id, index);
	} else {
		CMR_LOGE("err,channel_free_frame is null");
	}
	if (cxt->err_code) {
		CMR_LOGE("error exit");
		sem_getvalue(&cxt->proc_done_sm, &sm_val);
		if (0 == sm_val) {
			sem_post(&cxt->proc_done_sm);
		} else {
			CMR_LOGI("cxt->proc_done_sm should be 1, actual is %d ", sm_val);
		}
		send_capture_complete_msg();
		return ret;
	}
	if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
		CMR_LOGI("post proc has been cancel");
		ret = CMR_CAMERA_NORNAL_EXIT;
		goto exit;
	}

	if (cxt->ops.start_exif_encode) {
		cmr_u32 index = cxt->index;
		struct camera_jpeg_param   enc_param;
		struct snp_exif_param *exif_in_ptr = &cxt->chn_param.exif_in[index];
		struct img_size image_size= cxt->req_param.req_size;
		struct jpeg_wexif_cb_param  enc_out_param;
		memset(&enc_param, 0, sizeof(struct camera_jpeg_param));
		snp_set_status(snp_handle, WRITE_EXIF);
		exif_in_ptr->big_pic_stream_src.buf_size = cxt->jpeg_stream_size;
		exif_in_ptr->thumb_stream_src.buf_size = cxt->thumb_stream_size;
		camera_take_snapshot_step(CMR_STEP_WR_EXIF_S);
		ret = cxt->ops.start_exif_encode(cxt->oem_handle, snp_handle, &exif_in_ptr->big_pic_stream_src,
										&exif_in_ptr->thumb_stream_src, NULL, &exif_in_ptr->dst, &enc_out_param);
		camera_take_snapshot_step(CMR_STEP_WR_EXIF_E);
		sem_post(&cxt->writer_exif_sm);
		snp_set_status(snp_handle, POST_PROCESSING);
		if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
			CMR_LOGI("snp has been cancel");
			ret = CMR_CAMERA_NORNAL_EXIT;
			goto exit;
		}
		enc_param.need_free = 0;
		enc_param.index = index;
		enc_param.outPtr = (void*)enc_out_param.output_buf_virt_addr;
		enc_param.size = enc_out_param.output_buf_size;
		if (0 == snp_get_request(snp_handle) || (cxt->cap_cnt == cxt->req_param.total_num)) {
			enc_param.need_free = 1;
		}
		CMR_LOGI("need free %d stream %d cap cnt %d total num %d", enc_param.need_free, enc_param.size,
				cxt->cap_cnt, cxt->req_param.total_num);
		camera_take_snapshot_step(CMR_STEP_CALL_BACK);
		camera_snapshot_step_statisic(&image_size);
		frame_type.timestamp = frame->sec * 1000000000LL + frame->usec * 1000;
		memcpy((void *)&frame_type.jpeg_param, (void *)&enc_param, sizeof(struct camera_jpeg_param));
		snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_ENCODE_PICTURE, SNAPSHOT_EXIT_CB_DONE, (void*)&frame_type, sizeof(struct camera_frame_type));
	//	snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_STATE, SNAPSHOT_EVT_EXIF_JPEG_DONE, (void*)ret, sizeof(cmr_int));
		if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
			CMR_LOGI("snp has been cancel");
			ret = CMR_CAMERA_NORNAL_EXIT;
			goto exit;
		}
		{
			struct img_addr jpeg_addr;
			cmr_s8 value[PROPERTY_VALUE_MAX];
			property_get("debug.camera.save.snpfile", value, "0");
			if (atoi(value) == 8) {
				jpeg_addr.addr_y = enc_out_param.output_buf_virt_addr;
				camera_save_to_file(SNP_JPEG_STREAM+cxt->cap_cnt, IMG_DATA_TYPE_JPEG,
									cxt->req_param.post_proc_setting.actual_snp_size.width,
									cxt->req_param.post_proc_setting.actual_snp_size.height,
									&jpeg_addr);
			}
		}

	} else {
		CMR_LOGE("err, start enc func is null");
		ret = CMR_CAMERA_FAIL;
	}
	if (!ret) {
		snp_post_proc_done(snp_handle);
	}

exit:
	send_capture_complete_msg();
	if (ret) {
		snp_post_proc_err_exit(snp_handle, ret);
	}
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int snp_write_exif_thread_proc(struct cmr_msg *message, void* p_data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	cmr_handle                     snp_handle = (cmr_handle)p_data;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	struct frm_info                *frm_ptr;

	if (!message || !p_data) {
		CMR_LOGE("param error");
		goto exit;
	}
	CMR_LOGI("message.msg_type 0x%x, data 0x%lx", message->msg_type, (cmr_uint)message->data);
	switch (message->msg_type) {
	case SNP_EVT_WRITE_EXIF:
		ret = snp_write_exif(snp_handle, message->data);
		break;
	default:
		CMR_LOGI("don't support msg");
		break;
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int snp_redisplay_thread_proc(struct cmr_msg *message, void* p_data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	cmr_handle                     snp_handle = (cmr_handle)p_data;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	struct frm_info                *frm_ptr;

	if (!message || !p_data) {
		CMR_LOGE("param error");
		goto exit;
	}
	CMR_LOGI("message.msg_type 0x%x, data 0x%lx", message->msg_type, (cmr_uint)message->data);
	switch (message->msg_type) {
	case SNP_EVT_REDISPLAY:
		frm_ptr = (struct frm_info*)message->data;
		ret = snp_take_picture_done(snp_handle, frm_ptr);
		break;
	default:
		CMR_LOGI("don't support msg");
		break;
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int snp_thumb_thread_proc(struct cmr_msg *message, void* p_data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	cmr_handle                     snp_handle = (cmr_handle)p_data;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	struct frm_info                *frm_ptr;

	if (!message || !p_data) {
		CMR_LOGE("param error");
		goto exit;
	}
	CMR_LOGI("message.msg_type 0x%x, data 0x%lx", message->msg_type, (cmr_uint)message->data);
	switch (message->msg_type) {
	case SNP_EVT_THUMB:
		frm_ptr = (struct frm_info*)message->data;
		ret = snp_thumbnail(snp_handle, frm_ptr);
		break;
	default:
		CMR_LOGI("don't support msg");
		break;
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int snp_create_main_thread(cmr_handle snp_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;

	ret = cmr_thread_create(&cxt->thread_cxt.main_thr_handle, SNP_MSG_QUEUE_SIZE,
							snp_main_thread_proc, (void*)snp_handle);
	CMR_LOGV("0x%lx", (cmr_uint)cxt->thread_cxt.main_thr_handle);
	if (CMR_MSG_SUCCESS != ret) {
		CMR_LOGE("failed to create main thread %ld", ret);
		ret = -CMR_CAMERA_NO_SUPPORT;
	}
	CMR_LOGV("done %ld", ret);
	return ret;
}

cmr_int snp_destroy_main_thread(cmr_handle snp_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	struct snp_thread_context      *snp_thread_cxt;
	CMR_MSG_INIT(message);

	if (!snp_handle) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	snp_thread_cxt = &cxt->thread_cxt;
	if (snp_thread_cxt->main_thr_handle) {
/*		message.msg_type = SNP_EVT_EXIT;
		message.sync_flag = CMR_MSG_SYNC_PROCESSED;
		message.alloc_flag = 0;
		ret = cmr_thread_msg_send(cxt->thread_cxt.main_thr_handle, &message);
		if (ret) {
			CMR_LOGE("failed to send exit msg to cvt thr %ld", ret);
		}*/
		ret = cmr_thread_destroy(snp_thread_cxt->main_thr_handle);
		if (!ret) {
			snp_thread_cxt->main_thr_handle = (cmr_handle)0;
		} else {
			CMR_LOGE("failed to destroy main thread %ld", ret);
		}
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int snp_create_postproc_thread(cmr_handle snp_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;

	ret = cmr_thread_create(&cxt->thread_cxt.post_proc_thr_handle, SNP_MSG_QUEUE_SIZE,
							snp_postproc_thread_proc, (void*)snp_handle);

	CMR_LOGV("0x%lx", (cmr_uint)cxt->thread_cxt.post_proc_thr_handle);
	if (CMR_MSG_SUCCESS != ret) {
		CMR_LOGE("failed to create post proc thread %ld", ret);
		ret = -CMR_CAMERA_NO_SUPPORT;
	}
	CMR_LOGV("done %ld", ret);
	return ret;
}

cmr_int snp_destroy_postproc_thread(cmr_handle snp_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	struct snp_thread_context      *snp_thread_cxt;
	CMR_MSG_INIT(message);

	if (!snp_handle) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	snp_thread_cxt = &cxt->thread_cxt;
	if (snp_thread_cxt->post_proc_thr_handle) {
/*		message.msg_type = SNP_EVT_EXIT;
		message.sync_flag = CMR_MSG_SYNC_PROCESSED;
		message.alloc_flag = 0;
		ret = cmr_thread_msg_send(cxt->thread_cxt.post_proc_thr_handle, &message);
		if (ret) {
			CMR_LOGE("failed to send exit msg to cvt thr %ld", ret);
		}*/
		ret = cmr_thread_destroy(snp_thread_cxt->post_proc_thr_handle);
		if (!ret) {
			snp_thread_cxt->post_proc_thr_handle = (cmr_handle)0;
		} else {
			CMR_LOGE("failed to destroy post proc thread %ld", ret);
		}
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}


cmr_int snp_create_notify_thread(cmr_handle snp_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;

	ret = cmr_thread_create(&cxt->thread_cxt.notify_thr_handle, SNP_MSG_QUEUE_SIZE,
							snp_notify_thread_proc, (void*)snp_handle);
	CMR_LOGV("0x%lx", (cmr_uint)cxt->thread_cxt.notify_thr_handle);
	if (CMR_MSG_SUCCESS != ret) {
		CMR_LOGE("failed to create notify thread %ld", ret);
		ret = -CMR_CAMERA_NO_SUPPORT;
	}
	CMR_LOGV("done %ld", ret);
	return ret;
}

cmr_int snp_destroy_notify_thread(cmr_handle snp_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	struct snp_thread_context      *snp_thread_cxt;

	if (!snp_handle) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	snp_thread_cxt = &cxt->thread_cxt;
	if (snp_thread_cxt->notify_thr_handle) {
		ret = cmr_thread_destroy(snp_thread_cxt->notify_thr_handle);
		if (!ret) {
			snp_thread_cxt->notify_thr_handle = (cmr_handle)0;
		} else {
			CMR_LOGE("failed to destroy notify thread %ld", ret);
		}
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int snp_create_proc_cb_thread(cmr_handle snp_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;

	ret = cmr_thread_create(&cxt->thread_cxt.proc_cb_thr_handle, SNP_MSG_QUEUE_SIZE,
							snp_proc_cb_thread_proc, (void*)snp_handle);
	CMR_LOGV("0x%lx", (cmr_uint)cxt->thread_cxt.proc_cb_thr_handle);
	if (CMR_MSG_SUCCESS != ret) {
		CMR_LOGE("failed to create proc cb thread %ld", ret);
		ret = -CMR_CAMERA_NO_SUPPORT;
	}
	CMR_LOGV("done %ld", ret);
	return ret;
}

cmr_int snp_destroy_proc_cb_thread(cmr_handle snp_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	struct snp_thread_context      *snp_thread_cxt;

	if (!snp_handle) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	snp_thread_cxt = &cxt->thread_cxt;
	if (snp_thread_cxt->proc_cb_thr_handle) {
		ret = cmr_thread_destroy(snp_thread_cxt->proc_cb_thr_handle);
		if (!ret) {
			snp_thread_cxt->proc_cb_thr_handle = (cmr_handle)0;
		} else {
			CMR_LOGE("failed to destroy proc cb %ld", ret);
		}
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int snp_create_secondary_thread(cmr_handle snp_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;

	ret = cmr_thread_create(&cxt->thread_cxt.secondary_thr_handle, SNP_MSG_QUEUE_SIZE,
							snp_secondary_thread_proc, (void*)snp_handle);
	CMR_LOGV("0x%lx", (cmr_uint)cxt->thread_cxt.secondary_thr_handle);
	if (CMR_MSG_SUCCESS != ret) {
		CMR_LOGE("failed to create secondary thread %ld", ret);
		ret = -CMR_CAMERA_NO_SUPPORT;
	}
	CMR_LOGV("done %ld", ret);
	return ret;
}

cmr_int snp_destroy_secondary_thread(cmr_handle snp_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	struct snp_thread_context      *snp_thread_cxt;

	if (!snp_handle) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	snp_thread_cxt = &cxt->thread_cxt;
	if (snp_thread_cxt->secondary_thr_handle) {
		ret = cmr_thread_destroy(snp_thread_cxt->secondary_thr_handle);
		if (!ret) {
			snp_thread_cxt->secondary_thr_handle = (cmr_handle)0;
		} else {
			CMR_LOGE("failed to destroy secondary %ld", ret);
		}
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int snp_create_cvt_thread(cmr_handle snp_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;

	ret = cmr_thread_create(&cxt->thread_cxt.cvt_thr_handle, SNP_MSG_QUEUE_SIZE,
							snp_cvt_thread_proc, (void*)snp_handle);
	CMR_LOGV("0x%lx", (cmr_uint)cxt->thread_cxt.cvt_thr_handle);
	if (CMR_MSG_SUCCESS != ret) {
		CMR_LOGE("failed to create cvt thread %ld", ret);
		ret = -CMR_CAMERA_NO_SUPPORT;
	}
	CMR_LOGV("done %ld", ret);
	return ret;
}

cmr_int snp_destroy_cvt_thread(cmr_handle snp_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	struct snp_thread_context      *snp_thread_cxt;
	CMR_MSG_INIT(message);

	if (!snp_handle) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	snp_thread_cxt = &cxt->thread_cxt;

	if (snp_thread_cxt->cvt_thr_handle) {
/*		message.msg_type = SNP_EVT_EXIT;
		message.sync_flag = CMR_MSG_SYNC_PROCESSED;
		message.alloc_flag = 0;
		ret = cmr_thread_msg_send(cxt->thread_cxt.cvt_thr_handle, &message);
		if (ret) {
			CMR_LOGE("failed to send exit msg to cvt thr %ld", ret);
		}*/
		ret = cmr_thread_destroy(snp_thread_cxt->cvt_thr_handle);
		if (!ret) {
			snp_thread_cxt->cvt_thr_handle= (cmr_handle)0;
		} else {
			CMR_LOGE("failed to destroy cvt thread %ld", ret);
		}
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int snp_create_write_exif_thread(cmr_handle snp_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;

	ret = cmr_thread_create(&cxt->thread_cxt.write_exif_thr_handle, SNP_MSG_QUEUE_SIZE,
							snp_write_exif_thread_proc, (void*)snp_handle);
	CMR_LOGV("0x%lx", (cmr_uint)cxt->thread_cxt.write_exif_thr_handle);
	if (CMR_MSG_SUCCESS != ret) {
		CMR_LOGE("failed to create write exif thread %ld", ret);
		ret = -CMR_CAMERA_NO_SUPPORT;
	} else {
		sem_init(&cxt->thread_cxt.writte_exif_access_sm, 0, 1);
	}
	CMR_LOGV("done %ld", ret);
	return ret;
}

cmr_int snp_destroy_write_exif_thread(cmr_handle snp_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	struct snp_thread_context      *snp_thread_cxt;

	if (!snp_handle) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	sem_wait(&cxt->thread_cxt.writte_exif_access_sm);
	snp_thread_cxt = &cxt->thread_cxt;
	if (snp_thread_cxt->write_exif_thr_handle) {
		ret = cmr_thread_destroy(snp_thread_cxt->write_exif_thr_handle);
		if (!ret) {
			snp_thread_cxt->write_exif_thr_handle = (cmr_handle)0;
			sem_post(&cxt->thread_cxt.writte_exif_access_sm);
			sem_destroy(&cxt->thread_cxt.writte_exif_access_sm);
		} else {
			sem_post(&cxt->thread_cxt.writte_exif_access_sm);
			CMR_LOGE("failed to destroy write exif %ld", ret);
		}
	} else {
		sem_post(&cxt->thread_cxt.writte_exif_access_sm);
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int snp_create_redisplay_thread(cmr_handle snp_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;

	ret = cmr_thread_create(&cxt->thread_cxt.proc_redisplay_handle, SNP_MSG_QUEUE_SIZE,
							snp_redisplay_thread_proc, (void*)snp_handle);
	CMR_LOGV("0x%lx", (cmr_uint)cxt->thread_cxt.proc_redisplay_handle);
	if (CMR_MSG_SUCCESS != ret) {
		CMR_LOGE("failed to create redisplay thread %ld", ret);
		ret = -CMR_CAMERA_NO_SUPPORT;
	}
	CMR_LOGV("done %ld", ret);
	return ret;
}

cmr_int snp_destroy_redisplay_thread(cmr_handle snp_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	struct snp_thread_context      *snp_thread_cxt;

	if (!snp_handle) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	snp_thread_cxt = &cxt->thread_cxt;
	if (snp_thread_cxt->proc_redisplay_handle) {
		ret = cmr_thread_destroy(snp_thread_cxt->proc_redisplay_handle);
		if (!ret) {
			snp_thread_cxt->proc_redisplay_handle = (cmr_handle)0;
		} else {
			CMR_LOGE("failed to destroy redisplay %ld", ret);
		}
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int snp_create_thumb_thread(cmr_handle snp_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;

	ret = cmr_thread_create(&cxt->thread_cxt.proc_thumb_handle, SNP_MSG_QUEUE_SIZE,
							snp_thumb_thread_proc, (void*)snp_handle);
	CMR_LOGV("0x%lx", (cmr_uint)cxt->thread_cxt.proc_thumb_handle);
	if (CMR_MSG_SUCCESS != ret) {
		CMR_LOGE("failed to create thumb thread %ld", ret);
		ret = -CMR_CAMERA_NO_SUPPORT;
	}
	CMR_LOGV("done %ld", ret);
	return ret;
}

cmr_int snp_destroy_thumb_thread(cmr_handle snp_handle)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	struct snp_thread_context      *snp_thread_cxt;

	if (!snp_handle) {
		CMR_LOGE("in parm error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	snp_thread_cxt = &cxt->thread_cxt;
	if (snp_thread_cxt->proc_redisplay_handle) {
		ret = cmr_thread_destroy(snp_thread_cxt->proc_thumb_handle);
		if (!ret) {
			snp_thread_cxt->proc_thumb_handle = (cmr_handle)0;
		} else {
			CMR_LOGE("failed to destroy thumb %ld", ret);
		}
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int snp_create_thread(cmr_handle snp_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;

	ret = snp_create_main_thread(snp_handle);
	if (ret) {
		goto exit;
	}
	ret = snp_create_postproc_thread(snp_handle);
	if (ret) {
		goto destroy_main_thr;
	}
	ret = snp_create_notify_thread(snp_handle);
	if (ret) {
		goto destroy_postproc_thr;
	}
	ret = snp_create_proc_cb_thread(snp_handle);
	if (ret) {
		goto destroy_notify_thr;
	}
	ret = snp_create_secondary_thread(snp_handle);
	if (ret) {
		goto destroy_proc_cb_thr;
	}
	ret = snp_create_cvt_thread(snp_handle);
	if (ret) {
		goto destroy_sencondary_thr;
	}
	ret = snp_create_redisplay_thread(snp_handle);
	if (ret) {
		goto destroy_cvt_thr;
	}
	ret = snp_create_thumb_thread(snp_handle);
	if (ret) {
		goto destroy_redisplay_thr;
	}
	ret = snp_create_write_exif_thread(snp_handle);
	if (ret) {
		goto destroy_thumb_thr;
	} else {
		goto exit;
	}
destroy_thumb_thr:
	snp_destroy_thumb_thread(snp_handle);
destroy_redisplay_thr:
	snp_destroy_redisplay_thread(snp_handle);
destroy_cvt_thr:
	snp_destroy_write_exif_thread(snp_handle);
destroy_sencondary_thr:
	snp_destroy_secondary_thread(snp_handle);
destroy_proc_cb_thr:
	snp_destroy_proc_cb_thread(snp_handle);
destroy_notify_thr:
	snp_destroy_notify_thread(snp_handle);
destroy_postproc_thr:
	snp_destroy_postproc_thread(snp_handle);
destroy_main_thr:
	snp_destroy_main_thread(snp_handle);
exit:
	CMR_LOGV("done %ld", ret);
	return ret;
}

cmr_int snp_destroy_thread(cmr_handle snp_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;

	ret = snp_destroy_write_exif_thread(snp_handle);
	if (ret) {
		CMR_LOGE("failed to destroy write exif thread %ld", ret);
	}
	ret = snp_destroy_thumb_thread(snp_handle);
	if (ret) {
		CMR_LOGE("failed to destroy thumb thread %ld", ret);
	}
	ret = snp_destroy_redisplay_thread(snp_handle);
	if (ret) {
		CMR_LOGE("failed to destroy redisplay thread %ld", ret);
	}
	ret = snp_destroy_cvt_thread(snp_handle);
	if (ret) {
		CMR_LOGE("failed to destroy cvt thread %ld", ret);
	}
	ret = snp_destroy_secondary_thread(snp_handle);
	if (ret) {
		CMR_LOGE("failed to destroy secondary thread %ld", ret);
	}
	ret = snp_destroy_proc_cb_thread(snp_handle);
	if (ret) {
		CMR_LOGE("failed to destroy proc cb thread %ld", ret);
	}
	ret = snp_destroy_notify_thread(snp_handle);
	if (ret) {
		CMR_LOGE("failed to destroy notify thread %ld", ret);
	}
	ret = snp_destroy_postproc_thread(snp_handle);
	if (ret) {
		CMR_LOGE("failed to destroy post proc thread %ld", ret);
	}
	ret = snp_destroy_main_thread(snp_handle);
	if (ret) {
		CMR_LOGE("failed to destroy main thread %ld", ret);
	}
	CMR_LOGI("done");
	return ret;
}

void snp_local_init(cmr_handle snp_handle)
{
	struct snp_context              *cxt = (struct snp_context*)snp_handle;

	sem_init(&cxt->access_sm, 0, 1);
	sem_init(&cxt->proc_done_sm, 0, 1);
	sem_init(&cxt->cvt.cvt_sync_sm, 0, 0);
	sem_init(&cxt->takepic_callback_sem, 0, 0);
	sem_init(&cxt->jpeg_sync_sm, 0, 0);
	sem_init(&cxt->hdr_sync_sm, 0, 0);
	sem_init(&cxt->scaler_sync_sm, 0, 0);
	sem_init(&cxt->redisplay_sm, 0, 0);
	sem_init(&cxt->writer_exif_sm, 0, 0);
}

void snp_local_deinit(cmr_handle snp_handle)
{
	struct snp_context              *cxt = (struct snp_context*)snp_handle;

	sem_destroy(&cxt->access_sm);
	sem_destroy(&cxt->proc_done_sm);
	sem_destroy(&cxt->cvt.cvt_sync_sm);
	sem_destroy(&cxt->takepic_callback_sem);
	sem_destroy(&cxt->jpeg_sync_sm);
	sem_destroy(&cxt->scaler_sync_sm);
	sem_destroy(&cxt->hdr_sync_sm);
	sem_destroy(&cxt->redisplay_sm);
	sem_destroy(&cxt->writer_exif_sm);
	cxt->is_inited = 0;
}

cmr_int snp_check_post_proc_param(struct snapshot_param *param_ptr)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct img_size                 actual_size;
	struct img_size                 snp_size;
	struct img_frm                  chn_out;
	struct img_frm                  rot_frm;
	struct snp_proc_param           *proc_param_ptr;
	cmr_u32                         channel_id = param_ptr->channel_id;

	proc_param_ptr = &param_ptr->post_proc_setting;
	actual_size = proc_param_ptr->actual_snp_size;
	if (0 == actual_size.width || 0 == actual_size.height) {
		CMR_LOGE("err, actual size is zero");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	snp_size = proc_param_ptr->snp_size;
	if (0 == snp_size.width || 0 == snp_size.height) {
		CMR_LOGE("err, snp size is zero");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	if (param_ptr->channel_id >= GRAB_CHANNEL_MAX) {
		CMR_LOGE("err, channel id %d", param_ptr->channel_id);
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int snp_set_channel_out_param(cmr_handle snp_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct snp_context              *cxt = (struct snp_context*)snp_handle;
	struct snapshot_param           *channel_param_ptr = &cxt->req_param;
	struct img_frm                  *chn_out_frm_ptr;
	cmr_uint                        i;

	chn_out_frm_ptr = &channel_param_ptr->post_proc_setting.chn_out_frm[0];
	for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
		if (0 == chn_out_frm_ptr->size.width || 0 == chn_out_frm_ptr->size.height) {
			CMR_LOGE("err, frm size %d %d", chn_out_frm_ptr->size.width, chn_out_frm_ptr->size.height);
			ret = -CMR_CAMERA_INVALID_PARAM;
			break;
		}
		if (0 == chn_out_frm_ptr->addr_phy.addr_y || 0 == chn_out_frm_ptr->addr_phy.addr_u
			|| 0 == chn_out_frm_ptr->addr_vir.addr_y || 0 == chn_out_frm_ptr->addr_vir.addr_u) {
			CMR_LOGE("err, frm addr 0x%lx 0x%lx 0x%lx 0x%lx", chn_out_frm_ptr->addr_phy.addr_y,
					chn_out_frm_ptr->addr_phy.addr_u, chn_out_frm_ptr->addr_vir.addr_y,
					chn_out_frm_ptr->addr_vir.addr_u);
			ret = -CMR_CAMERA_INVALID_PARAM;
			break;
		}
		chn_out_frm_ptr++;
	}
	if (CMR_CAMERA_SUCCESS == ret) {
		chn_out_frm_ptr = &channel_param_ptr->post_proc_setting.chn_out_frm[0];
		CMR_LOGI("data format %d", chn_out_frm_ptr->fmt);
		if (IMG_DATA_TYPE_JPEG != chn_out_frm_ptr->fmt) {
			if (IMG_DATA_TYPE_RAW != chn_out_frm_ptr->fmt) {
				cmr_copy((void*)&cxt->chn_param.chn_frm[0], (void*)chn_out_frm_ptr, CMR_CAPTURE_MEM_SUM*sizeof(cxt->chn_param.chn_frm));
			} else {
				if (CAMERA_AUTOTEST_MODE != cxt->req_param.mode) {
					for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
						cxt->chn_param.chn_frm[i] = cxt->chn_param.isp_proc_in[i].dst_frame;
						cxt->chn_param.chn_frm[i].fmt = IMG_DATA_TYPE_YUV420;
					}
				} else {
					cmr_copy((void*)&cxt->chn_param.chn_frm[0], (void*)chn_out_frm_ptr, CMR_CAPTURE_MEM_SUM*sizeof(cxt->chn_param.chn_frm));
				}
			}
		} else {
			for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
				cxt->chn_param.chn_frm[i] = cxt->chn_param.jpeg_dec_in[i].dst;
			}
		}
	}
	if (!ret) {
		chn_out_frm_ptr = &cxt->chn_param.chn_frm[0];
		for (i=0 ; i<1/*CMR_CAPTURE_MEM_SUM*/ ; i++) {
			CMR_LOGI("phy addr 0x%lx 0x%lx vir addr 0x%lx 0x%lx size %d %d", chn_out_frm_ptr->addr_phy.addr_y,
					chn_out_frm_ptr->addr_phy.addr_u, chn_out_frm_ptr->addr_vir.addr_y,
					chn_out_frm_ptr->addr_vir.addr_u, chn_out_frm_ptr->size.width, chn_out_frm_ptr->size.height);
		}
	}
	CMR_LOGD("done %ld", ret);
	return ret;
}

cmr_int snp_set_hdr_param(cmr_handle snp_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct snp_context              *cxt = (struct snp_context*)snp_handle;

	cmr_copy((void*)&cxt->chn_param.hdr_src_frm[0], (void*)&cxt->chn_param.chn_frm[0],
			sizeof(cxt->chn_param.hdr_src_frm));

	return ret;
}

cmr_int snp_set_scale_param(cmr_handle snp_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct snp_context              *cxt = (struct snp_context*)snp_handle;
	struct img_rect                 rect[CMR_CAPTURE_MEM_SUM];
	struct snapshot_param           *req_param_ptr = &cxt->req_param;
	struct snp_channel_param        *chn_param_ptr = &cxt->chn_param;
	struct img_frm                  *frm_ptr;
	cmr_uint                        i;
	cmr_u32                         offset[CMR_CAPTURE_MEM_SUM] = {0, 0, 0, 0};

	cmr_copy(&rect[0], &req_param_ptr->post_proc_setting.scaler_src_rect[0], CMR_CAPTURE_MEM_SUM*sizeof(struct img_rect));

	CMR_LOGI("scaler_src_rect %d %d %d %d", rect[0].start_x, rect[0].start_y, rect[0].width, rect[0].height);

	for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
		chn_param_ptr->scale[i].slice_height = rect[i].height;
		chn_param_ptr->scale[i].src_img.fmt = IMG_DATA_TYPE_YUV420;
		chn_param_ptr->scale[i].dst_img.fmt = IMG_DATA_TYPE_YUV420;
		chn_param_ptr->scale[i].src_img.data_end = req_param_ptr->post_proc_setting.data_endian;
		chn_param_ptr->scale[i].dst_img.data_end = req_param_ptr->post_proc_setting.data_endian;
		chn_param_ptr->scale[i].src_img.rect = rect[i];
		chn_param_ptr->scale[i].dst_img.size = req_param_ptr->post_proc_setting.actual_snp_size;
		cmr_copy((void*)&chn_param_ptr->scale[i].dst_img.addr_phy, (void*)&req_param_ptr->post_proc_setting.mem[i].target_yuv.addr_phy,
				sizeof(struct img_addr));
		cmr_copy((void*)&chn_param_ptr->scale[i].dst_img.addr_vir, (void*)&req_param_ptr->post_proc_setting.mem[i].target_yuv.addr_vir,
				sizeof(struct img_addr));
	}

	if (IMG_ANGLE_0 == req_param_ptr->post_proc_setting.rot_angle ||
		(cxt->req_param.is_cfg_rot_cap && (IMG_ANGLE_0 == cxt->req_param.rot_angle))) {
		for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
			chn_param_ptr->scale[i].src_img.addr_phy.addr_y = chn_param_ptr->chn_frm[i].addr_phy.addr_y + offset[i];
			chn_param_ptr->scale[i].src_img.addr_phy.addr_u = chn_param_ptr->chn_frm[i].addr_phy.addr_u + (offset[i] >> 1);
			chn_param_ptr->scale[i].src_img.addr_phy.addr_v = chn_param_ptr->chn_frm[i].addr_phy.addr_v + (offset[i] >> 1);
			chn_param_ptr->scale[i].src_img.size = req_param_ptr->post_proc_setting.chn_out_frm[i].size;
			chn_param_ptr->scale[i].src_img.addr_vir.addr_y = chn_param_ptr->chn_frm[i].addr_vir.addr_y + offset[i];
			chn_param_ptr->scale[i].src_img.addr_vir.addr_u = chn_param_ptr->chn_frm[i].addr_vir.addr_u + (offset[i] >> 1);
			chn_param_ptr->scale[i].src_img.addr_vir.addr_v = chn_param_ptr->chn_frm[i].addr_vir.addr_v + (offset[i] >> 1);
		}
	} else {//rotation case
		for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
			chn_param_ptr->scale[i].src_img.addr_phy.addr_y = chn_param_ptr->rot[i].dst_img.addr_phy.addr_y + offset[i];
			chn_param_ptr->scale[i].src_img.addr_phy.addr_u = chn_param_ptr->rot[i].dst_img.addr_phy.addr_u + (offset[i] >> 1);
			chn_param_ptr->scale[i].src_img.addr_phy.addr_v = chn_param_ptr->rot[i].dst_img.addr_phy.addr_v + (offset[i] >> 1);
			chn_param_ptr->scale[i].src_img.size = chn_param_ptr->rot[i].dst_img.size;
			chn_param_ptr->scale[i].src_img.addr_vir.addr_y = chn_param_ptr->rot[i].dst_img.addr_phy.addr_y + offset[i];
			chn_param_ptr->scale[i].src_img.addr_vir.addr_u = chn_param_ptr->rot[i].dst_img.addr_phy.addr_u + (offset[i] >> 1);
			chn_param_ptr->scale[i].src_img.addr_vir.addr_v = chn_param_ptr->rot[i].dst_img.addr_phy.addr_v + (offset[i] >> 1);
		}
	}

	for (i=0 ; i<1/*CMR_CAPTURE_MEM_SUM*/ ; i++) {
		CMR_LOGI("src addr 0x%lx 0x%lx dst add 0x%lx 0x%lx",
			chn_param_ptr->scale[i].src_img.addr_phy.addr_y,
			chn_param_ptr->scale[i].src_img.addr_phy.addr_u,
			chn_param_ptr->scale[i].dst_img.addr_phy.addr_y,
			chn_param_ptr->scale[i].dst_img.addr_phy.addr_u);

		CMR_LOGI("src size %d %d dst size %d %d",
			chn_param_ptr->scale[i].src_img.size.width,
			chn_param_ptr->scale[i].src_img.size.height,
			chn_param_ptr->scale[i].dst_img.size.width,
			chn_param_ptr->scale[i].dst_img.size.height);
	}
	return ret;
}

cmr_int snp_set_convert_thumb_param(cmr_handle snp_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct snp_context              *cxt = (struct snp_context*)snp_handle;
	struct img_rect                 rect;
	struct snapshot_param           *req_param_ptr = &cxt->req_param;
	struct snp_channel_param        *chn_param_ptr = &cxt->chn_param;
	struct snp_scale_param          *thumb_ptr = &cxt->chn_param.convert_thumb[0];
	struct jpeg_param               *jpeg_ptr = &req_param_ptr->jpeg_setting;
	cmr_uint                        i;

	for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
		thumb_ptr->src_img = req_param_ptr->post_proc_setting.mem[i].target_yuv;
		thumb_ptr->dst_img = req_param_ptr->post_proc_setting.mem[i].thum_yuv;
		thumb_ptr->src_img.size = req_param_ptr->post_proc_setting.actual_snp_size;
		thumb_ptr->src_img.rect.start_x = 0;
		thumb_ptr->src_img.rect.start_y = 0;
		thumb_ptr->src_img.rect.width = thumb_ptr->src_img.size.width;
		thumb_ptr->src_img.rect.height = thumb_ptr->src_img.size.height;
		thumb_ptr->src_img.data_end = req_param_ptr->post_proc_setting.data_endian;
		thumb_ptr->dst_img.data_end = req_param_ptr->post_proc_setting.data_endian;
		thumb_ptr->src_img.fmt = IMG_DATA_TYPE_YUV420;
		thumb_ptr->dst_img.fmt = IMG_DATA_TYPE_YUV420;
		thumb_ptr++;
	}
	thumb_ptr = &cxt->chn_param.convert_thumb[0];
	if (req_param_ptr->is_cfg_rot_cap &&
		(IMG_ANGLE_90 == jpeg_ptr->set_encode_rotation || IMG_ANGLE_270 == jpeg_ptr->set_encode_rotation)) {
		for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
			thumb_ptr->dst_img.size.width = jpeg_ptr->thum_size.height;
			thumb_ptr->dst_img.size.height = jpeg_ptr->thum_size.width;
			thumb_ptr->slice_height = thumb_ptr->dst_img.size.height;
			thumb_ptr++;
		}
	} else {
		for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
			thumb_ptr->dst_img.size = jpeg_ptr->thum_size;
			thumb_ptr->slice_height = thumb_ptr->dst_img.size.height;
			thumb_ptr++;
		}
	}
	thumb_ptr = &cxt->chn_param.convert_thumb[0];
	for (i=0 ; i<1/*CMR_CAPTURE_MEM_SUM*/ ; i++) {
		CMR_LOGI("src addr 0x%lx 0x%lx dst addr 0x%lx 0x%lx", thumb_ptr->src_img.addr_phy.addr_y, thumb_ptr->src_img.addr_phy.addr_u,
				thumb_ptr->dst_img.addr_phy.addr_y, thumb_ptr->dst_img.addr_phy.addr_u);
		CMR_LOGI("src size %d %d dst size %d %d slice height %d", thumb_ptr->src_img.size.width, thumb_ptr->src_img.size.height,
				thumb_ptr->dst_img.size.width, thumb_ptr->dst_img.size.height, (cmr_u32)thumb_ptr->slice_height);
		thumb_ptr++;
	}
	return ret;
}

cmr_int snp_set_rot_param(cmr_handle snp_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct snp_context              *cxt = (struct snp_context*)snp_handle;
	struct snapshot_param           *req_param_ptr = &cxt->req_param;
	struct snp_channel_param        *chn_param_ptr = &cxt->chn_param;
	struct snp_rot_param            *rot_ptr = &cxt->chn_param.rot[0];
	cmr_uint                        i;

	if (IMG_ANGLE_0 == req_param_ptr->post_proc_setting.rot_angle) {
		CMR_LOGI("don't need to rotate");
		goto exit;
	}
	chn_param_ptr->is_rot = 1;
	for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
		if (IMG_DATA_TYPE_JPEG != req_param_ptr->post_proc_setting.chn_out_frm[i].fmt) {
			rot_ptr->src_img = chn_param_ptr->chn_frm[i];
		} else {
			rot_ptr->src_img = chn_param_ptr->jpeg_dec_in[i].dst;
		}
		rot_ptr->angle = req_param_ptr->post_proc_setting.rot_angle;
		rot_ptr++;
	}
	rot_ptr = &cxt->chn_param.rot[0];
	if (chn_param_ptr->is_scaling) {
		for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
			rot_ptr->dst_img = req_param_ptr->post_proc_setting.mem[i].cap_yuv;
			rot_ptr++;
		}
	} else {
		for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
			rot_ptr->dst_img = req_param_ptr->post_proc_setting.mem[i].target_yuv;
			rot_ptr++;
		}
	}
	rot_ptr = &cxt->chn_param.rot[0];
	if (IMG_ANGLE_90 == req_param_ptr->post_proc_setting.rot_angle
		|| IMG_ANGLE_270 == req_param_ptr->post_proc_setting.rot_angle) {
		for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
			rot_ptr->dst_img.size.width = req_param_ptr->post_proc_setting.chn_out_frm[i].size.height;
			rot_ptr->dst_img.size.height = req_param_ptr->post_proc_setting.chn_out_frm[i].size.width;
			rot_ptr++;
		}
	} else {
		for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
			rot_ptr->dst_img.size = req_param_ptr->post_proc_setting.chn_out_frm[i].size;
			rot_ptr++;
		}
	}
	rot_ptr = &cxt->chn_param.rot[0];
	for (i=0 ; i<1/*CMR_CAPTURE_MEM_SUM*/ ; i++) {
		CMR_LOGI("src addr 0x%lx 0x%lx dst 0x%lx 0x%lx", rot_ptr->src_img.addr_phy.addr_y, rot_ptr->src_img.addr_phy.addr_u,
				rot_ptr->dst_img.addr_phy.addr_y, rot_ptr->dst_img.addr_phy.addr_u);
		CMR_LOGI("src size %d %d dst size %d %d", rot_ptr->src_img.size.width, rot_ptr->src_img.size.height,
				rot_ptr->dst_img.size.width, rot_ptr->dst_img.size.height);
		rot_ptr++;
	}
exit:
	return ret;
}

cmr_int snp_set_jpeg_enc_param(cmr_handle snp_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct snp_context              *cxt = (struct snp_context*)snp_handle;
	struct snapshot_param           *req_param_ptr = &cxt->req_param;
	struct snp_channel_param        *chn_param_ptr = &cxt->chn_param;
	struct snp_jpeg_param           *jpeg_ptr = &chn_param_ptr->jpeg_in[0];
	cmr_uint                        i;

	for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
		jpeg_ptr->src = req_param_ptr->post_proc_setting.mem[i].target_yuv;
		jpeg_ptr->dst = req_param_ptr->post_proc_setting.mem[i].target_jpeg;
		jpeg_ptr->src.size = req_param_ptr->post_proc_setting.actual_snp_size;
		if (req_param_ptr->is_cfg_rot_cap && (IMG_ANGLE_0 != req_param_ptr->jpeg_setting.set_encode_rotation)
			&& (IMG_ANGLE_180 != req_param_ptr->jpeg_setting.set_encode_rotation)) {
			jpeg_ptr->dst.size.width = req_param_ptr->req_size.height;
			jpeg_ptr->dst.size.height = req_param_ptr->req_size.width;
		} else {
			jpeg_ptr->dst.size = req_param_ptr->req_size;
		}
		jpeg_ptr->dst.size = req_param_ptr->post_proc_setting.dealign_actual_snp_size;
		jpeg_ptr->mean.is_sync = 0;
		jpeg_ptr->mean.is_thumb = 0;
		jpeg_ptr->mean.slice_num = 1;
		jpeg_ptr->mean.quality_level = req_param_ptr->jpeg_setting.quality;
		jpeg_ptr->mean.slice_height = req_param_ptr->post_proc_setting.actual_snp_size.height;
		jpeg_ptr->mean.slice_mode = JPEG_YUV_SLICE_ONE_BUF;
		if (req_param_ptr->is_cfg_rot_cap) {
			jpeg_ptr->mean.rot = 0;
		} else {
			jpeg_ptr->mean.rot = req_param_ptr->jpeg_setting.set_encode_rotation;
		}
		jpeg_ptr++;
	}
	jpeg_ptr = &chn_param_ptr->jpeg_in[0];
	for (i=0 ; i<1/*CMR_CAPTURE_MEM_SUM*/ ; i++) {
		CMR_LOGI("src addr 0x%lx 0x%lx dst 0x%lx", jpeg_ptr->src.addr_phy.addr_y, jpeg_ptr->src.addr_phy.addr_u,
				jpeg_ptr->dst.addr_phy.addr_y);
		CMR_LOGI("src size %d %d out size %d %d", jpeg_ptr->src.size.width, jpeg_ptr->src.size.height,
				jpeg_ptr->dst.size.width, jpeg_ptr->dst.size.height);
		CMR_LOGI("quality %d slice height %d", jpeg_ptr->mean.quality_level, jpeg_ptr->mean.slice_height);
	}
	return ret;
}


cmr_int snp_set_jpeg_thumb_param(cmr_handle snp_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct snp_context              *cxt = (struct snp_context*)snp_handle;
	struct snapshot_param           *req_param_ptr = &cxt->req_param;
	struct snp_channel_param        *chn_param_ptr = &cxt->chn_param;
	struct snp_jpeg_param           *jpeg_ptr = &chn_param_ptr->thumb_in[0];
	struct snp_scale_param          *thumb_ptr = &cxt->chn_param.convert_thumb[0];
	cmr_uint                        i;

	for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
		if ((thumb_ptr->src_img.size.width != thumb_ptr->dst_img.size.width)
			|| (thumb_ptr->src_img.size.height != thumb_ptr->dst_img.size.height)) {
			jpeg_ptr->src = req_param_ptr->post_proc_setting.mem[i].thum_yuv;
		} else {
			jpeg_ptr->src = req_param_ptr->post_proc_setting.mem[i].target_yuv;
		}
		jpeg_ptr->dst = req_param_ptr->post_proc_setting.mem[i].thum_jpeg;
		jpeg_ptr->src.size = cxt->chn_param.convert_thumb[i].dst_img.size;
		jpeg_ptr->mean.is_sync = 1;
		jpeg_ptr->mean.is_thumb = 1;
		jpeg_ptr->mean.quality_level = req_param_ptr->jpeg_setting.thumb_quality;
		jpeg_ptr->mean.slice_height = jpeg_ptr->src.size.height;
		jpeg_ptr->mean.slice_mode = JPEG_YUV_SLICE_ONE_BUF;
		jpeg_ptr->src.rect.start_x = 0;
		jpeg_ptr->src.rect.start_y = 0;
		jpeg_ptr->src.rect.width = jpeg_ptr->src.size.width;
		jpeg_ptr->src.rect.height = jpeg_ptr->src.size.height;
		jpeg_ptr->dst.size = jpeg_ptr->src.size;
		if (req_param_ptr->is_cfg_rot_cap) {
			jpeg_ptr->mean.rot = 0;
		} else {
			jpeg_ptr->mean.rot = req_param_ptr->jpeg_setting.set_encode_rotation;
		}
		jpeg_ptr++;
		thumb_ptr++;
	}
	jpeg_ptr = &chn_param_ptr->thumb_in[0];
	for (i=0 ; i<1/*CMR_CAPTURE_MEM_SUM*/ ; i++) {
		CMR_LOGI("src addr 0x%lx 0x%lx dst 0x%lx", jpeg_ptr->src.addr_phy.addr_y, jpeg_ptr->src.addr_phy.addr_u,
				jpeg_ptr->dst.addr_phy.addr_y);
		CMR_LOGI("src size %d %d out size %d %d", jpeg_ptr->src.size.width, jpeg_ptr->src.size.height,
				jpeg_ptr->dst.size.width, jpeg_ptr->dst.size.height);
		CMR_LOGI("quality %d slice height %d", jpeg_ptr->mean.quality_level, jpeg_ptr->mean.slice_height);
	}
	return ret;
}

cmr_int snp_set_jpeg_exif_param(cmr_handle snp_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct snp_context              *cxt = (struct snp_context*)snp_handle;
	struct snapshot_param           *req_param_ptr = &cxt->req_param;
	struct snp_channel_param        *chn_param_ptr = &cxt->chn_param;
	struct snp_exif_param           *exif_in_ptr = &chn_param_ptr->exif_in[0];
	cmr_uint                        i;

	for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
		exif_in_ptr->big_pic_stream_src = chn_param_ptr->jpeg_in[i].dst;
		exif_in_ptr->thumb_stream_src = chn_param_ptr->thumb_in[i].dst;
		exif_in_ptr->dst.addr_vir.addr_y = exif_in_ptr->big_pic_stream_src.addr_vir.addr_y - JPEG_EXIF_SIZE;
		exif_in_ptr->dst.buf_size = JPEG_EXIF_SIZE + exif_in_ptr->big_pic_stream_src.buf_size;
		exif_in_ptr++;
	}
	exif_in_ptr = &chn_param_ptr->exif_in[0];
	for (i=0 ; i<1/*CMR_CAPTURE_MEM_SUM*/ ; i++) {
		CMR_LOGI("src addr 0x%lx 0x%lx dst addr 0x%lx", exif_in_ptr->big_pic_stream_src.addr_vir.addr_y,
				exif_in_ptr->thumb_stream_src.addr_vir.addr_y, exif_in_ptr->dst.addr_vir.addr_y);
		CMR_LOGI("dst buf size %d", exif_in_ptr->dst.buf_size);
	}
	return ret;
}


cmr_int snp_set_jpeg_dec_param(cmr_handle snp_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct snp_context              *cxt = (struct snp_context*)snp_handle;
	struct snapshot_param           *req_param_ptr = &cxt->req_param;
	struct snp_channel_param        *chn_param_ptr = &cxt->chn_param;
	struct snp_jpeg_param           *dec_in_ptr = &chn_param_ptr->jpeg_dec_in[0];
	cmr_uint                        i;

	for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
		dec_in_ptr->src = req_param_ptr->post_proc_setting.chn_out_frm[i];
		dec_in_ptr->mean.temp_buf = req_param_ptr->post_proc_setting.mem[i].jpeg_tmp;
		dec_in_ptr++;
	}
	dec_in_ptr = &chn_param_ptr->jpeg_dec_in[0];
	if (IMG_ANGLE_0 != req_param_ptr->post_proc_setting.rot_angle) {
		for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
			if (req_param_ptr->post_proc_setting.is_need_scaling) {
				dec_in_ptr->dst.addr_phy = req_param_ptr->post_proc_setting.mem[i].cap_yuv.addr_phy;
				dec_in_ptr->dst.addr_vir = req_param_ptr->post_proc_setting.mem[i].cap_yuv.addr_vir;
			} else {
				dec_in_ptr->dst.addr_phy = req_param_ptr->post_proc_setting.mem[i].target_yuv.addr_phy;
				dec_in_ptr->dst.addr_vir = req_param_ptr->post_proc_setting.mem[i].target_yuv.addr_vir;
			}
			dec_in_ptr++;
		}
	} else {
		if (req_param_ptr->is_cfg_rot_cap) {
			for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
				if (req_param_ptr->post_proc_setting.is_need_scaling) {
					dec_in_ptr->dst.addr_phy = req_param_ptr->post_proc_setting.mem[i].cap_yuv.addr_phy;
					dec_in_ptr->dst.addr_vir = req_param_ptr->post_proc_setting.mem[i].cap_yuv.addr_vir;
				} else {
					dec_in_ptr->dst.addr_phy = req_param_ptr->post_proc_setting.mem[i].target_yuv.addr_phy;
					dec_in_ptr->dst.addr_vir = req_param_ptr->post_proc_setting.mem[i].target_yuv.addr_vir;
				}
				dec_in_ptr++;
			}
		} else if (!chn_param_ptr->is_scaling) {
			for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
				dec_in_ptr->dst.addr_phy = req_param_ptr->post_proc_setting.mem[i].target_yuv.addr_phy;
				dec_in_ptr->dst.addr_vir = req_param_ptr->post_proc_setting.mem[i].target_yuv.addr_vir;
				dec_in_ptr++;
			}
		} else {
			for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
				dec_in_ptr->dst.addr_phy = req_param_ptr->post_proc_setting.mem[i].cap_yuv.addr_phy;
				dec_in_ptr->dst.addr_vir = req_param_ptr->post_proc_setting.mem[i].cap_yuv.addr_vir;
				dec_in_ptr++;
			}
		}
	}
	dec_in_ptr = &chn_param_ptr->jpeg_dec_in[0];
	for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
		dec_in_ptr->dst.size = req_param_ptr->post_proc_setting.chn_out_frm[i].size;
		dec_in_ptr->dst.fmt = IMG_DATA_TYPE_YUV420;
		dec_in_ptr->dst.data_end = cxt->req_param.post_proc_setting.data_endian;
		dec_in_ptr->mean.slice_mode = JPEG_YUV_SLICE_ONE_BUF;
		dec_in_ptr->mean.slice_num = 1;
		dec_in_ptr->mean.slice_height = dec_in_ptr->dst.size.height;
		dec_in_ptr++;
	}
	dec_in_ptr = &chn_param_ptr->jpeg_dec_in[0];
/*	for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
		CMR_LOGI("src addr 0x%lx dst 0x%lx 0x%lx", dec_in_ptr->src.addr_phy.addr_y,
				dec_in_ptr->dst.addr_phy.addr_y, dec_in_ptr->dst.addr_phy.addr_u);
		CMR_LOGI("dst size %d %d", dec_in_ptr->dst.size.width, dec_in_ptr->dst.size.height);
	}*/
	return ret;
}

cmr_int snp_set_isp_proc_param(cmr_handle snp_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct snp_context              *cxt = (struct snp_context*)snp_handle;
	struct snapshot_param           *req_param_ptr = &cxt->req_param;
	struct snp_channel_param        *chn_param_ptr = &cxt->chn_param;
	struct raw_proc_param           *isp_proc_in_ptr = &chn_param_ptr->isp_proc_in[0];
	cmr_uint                        i;

	cmr_bzero(chn_param_ptr->isp_proc_in, sizeof(chn_param_ptr->isp_proc_in));
	cmr_bzero(chn_param_ptr->isp_process, sizeof(chn_param_ptr->isp_process));
	if (IMG_ANGLE_0 == req_param_ptr->post_proc_setting.rot_angle) {
		if (!req_param_ptr->post_proc_setting.is_need_scaling) {
			for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
				chn_param_ptr->isp_proc_in[i].dst_frame = req_param_ptr->post_proc_setting.mem[i].target_yuv;
			}
		} else if (req_param_ptr->is_cfg_rot_cap) {
			for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
				chn_param_ptr->isp_proc_in[i].dst_frame = req_param_ptr->post_proc_setting.mem[i].cap_yuv_rot;
			}
		} else {
			for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
				chn_param_ptr->isp_proc_in[i].dst_frame = req_param_ptr->post_proc_setting.mem[i].cap_yuv;
			}
		}
	} else {
		for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
			chn_param_ptr->isp_proc_in[i].dst_frame = req_param_ptr->post_proc_setting.mem[i].cap_yuv_rot;
		}
	}

	for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
		/*chn_param_ptr->isp_proc_in[i].dst_frame.size = req_param_ptr->post_proc_setting.snp_size;*/
		chn_param_ptr->isp_proc_in[i].dst_frame.size = req_param_ptr->post_proc_setting.dealign_actual_snp_size;
		chn_param_ptr->isp_proc_in[i].src_frame = req_param_ptr->post_proc_setting.mem[i].cap_raw;
		chn_param_ptr->isp_proc_in[i].src_avail_height = req_param_ptr->post_proc_setting.mem[i].cap_raw.size.height;
#if 1
		chn_param_ptr->isp_proc_in[i].src_slice_height = chn_param_ptr->isp_proc_in[i].src_avail_height;
		chn_param_ptr->isp_proc_in[i].dst_slice_height = chn_param_ptr->isp_proc_in[i].src_avail_height;
		chn_param_ptr->isp_process[i].slice_height_in = chn_param_ptr->isp_proc_in[i].src_avail_height;
#else
		chn_param_ptr->isp_proc_in[i].src_slice_height = CMR_SLICE_HEIGHT;
		chn_param_ptr->isp_proc_in[i].dst_slice_height = CMR_SLICE_HEIGHT;
		chn_param_ptr->isp_process[i].slice_height_in = CMR_SLICE_HEIGHT;
#endif
//		chn_param_ptr->isp_process[i].slice_height_out = CMR_SLICE_HEIGHT;
                if (CAMERA_ISP_TUNING_MODE == req_param_ptr->mode) {
			chn_param_ptr->isp_proc_in[i].dst_frame.fmt = ISP_DATA_YVU420_2FRAME;
                } else {
			chn_param_ptr->isp_proc_in[i].dst_frame.fmt = ISP_DATA_YUV420_2FRAME;
                }
		chn_param_ptr->isp_proc_in[i].src_frame.fmt = ISP_DATA_CSI2_RAW10;
	}

	if (!(chn_param_ptr->is_scaling || chn_param_ptr->is_rot)) {
		for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
			chn_param_ptr->isp_process[i].is_encoding = 1;
		}
	}
	for (i=0 ; i<1/*CMR_CAPTURE_MEM_SUM*/ ; i++) {
		CMR_LOGI("src addr 0x%lx  dst addr 0x%lx 0x%lx chan w=%d h=%d", chn_param_ptr->isp_proc_in[i].src_frame.addr_phy.addr_y,
				chn_param_ptr->isp_proc_in[i].dst_frame.addr_phy.addr_y, chn_param_ptr->isp_proc_in[i].dst_frame.addr_phy.addr_u,
				req_param_ptr->post_proc_setting.chn_out_frm[i].size.width,
				req_param_ptr->post_proc_setting.chn_out_frm[i].size.height);
	}
	return ret;
}

void snp_get_is_scaling(cmr_handle snp_handle, cmr_s32 is_normal_snapshot)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct snp_context              *cxt = (struct snp_context*)snp_handle;
	struct snp_proc_param           *proc_param_ptr = &cxt->req_param.post_proc_setting;
	struct snapshot_param           *req_param_ptr = &cxt->req_param;
	struct sensor_mode_info         *sensor_mode_ptr;
	struct sensor_exp_info          *sensor_info_ptr = &cxt->sensor_info;
	struct img_size                 tmp_size;
	cmr_u32                         is_scaling = 0;
	cmr_uint                        i;
/*
	ret = cxt->ops.get_sensor_info(cxt->oem_handle, req_param_ptr->camera_id, sensor_info_ptr);
	sensor_mode_ptr = &sensor_info_ptr->mode_info[req_param_ptr->sn_mode];

	if (IMG_ANGLE_0 == proc_param_ptr->rot_angle || IMG_ANGLE_180 == proc_param_ptr->rot_angle) {
		tmp_size = proc_param_ptr->chn_out_frm[0].size;
	} else {
		tmp_size.width = proc_param_ptr->chn_out_frm[0].size.height;
		tmp_size.height = proc_param_ptr->chn_out_frm[0].size.width;
	}

	if (ZOOM_POST_PROCESS == proc_param_ptr->channel_zoom_mode) {
		if (ZOOM_LEVEL == req_param_ptr->zoom_param.mode) {
			if (!((0 == req_param_ptr->zoom_param.zoom_level)
				&& (sensor_mode_ptr->trim_width == sensor_mode_ptr->scaler_trim.width)
				&& (sensor_mode_ptr->trim_height == sensor_mode_ptr->scaler_trim.height))) {
				is_scaling = 1;
			}
		} else {
			if (!((req_param_ptr->zoom_param.zoom_rect.width == sensor_mode_ptr->scaler_trim.width)
				&& (req_param_ptr->zoom_param.zoom_rect.height == sensor_mode_ptr->scaler_trim.height))) {
				is_scaling = 1;
			}
		}
	}
	if ((tmp_size.width == proc_param_ptr->actual_snp_size.width)
		&& (tmp_size.height == proc_param_ptr->actual_snp_size.height)
		&&(ZOOM_BY_CAP == proc_param_ptr->channel_zoom_mode
		|| 0 == is_scaling)) {
		cxt->chn_param.is_scaling = 0;
	} else {
		cxt->chn_param.is_scaling = 1;
	}
	cxt->chn_param.is_scaling = 0;
*/
	cxt->chn_param.is_scaling = cxt->req_param.post_proc_setting.is_need_scaling;

	CMR_LOGI("is_need_scaling %ld, rot_angle %ld, is_normal_snapshot %d",
		cxt->req_param.post_proc_setting.is_need_scaling,
		cxt->req_param.post_proc_setting.rot_angle,
		is_normal_snapshot);

	if ((0 == cxt->chn_param.is_scaling) &&
		(cxt->req_param.mode != CAMERA_ISP_TUNING_MODE) &&
		(cxt->req_param.mode != CAMERA_UTEST_MODE)) {
		if (is_normal_snapshot) {
		if (cxt->req_param.is_cfg_rot_cap && (IMG_ANGLE_0 == cxt->req_param.post_proc_setting.rot_angle)) {
			cxt->chn_param.is_scaling = 1;
			for ( i=0 ;i<CMR_CAPTURE_MEM_SUM ; i++) {
				cxt->req_param.post_proc_setting.scaler_src_rect[i].start_x = 0;
				cxt->req_param.post_proc_setting.scaler_src_rect[i].start_y = 0;
				cxt->req_param.post_proc_setting.scaler_src_rect[i].width = cxt->req_param.post_proc_setting.chn_out_frm[i].size.width;
				cxt->req_param.post_proc_setting.scaler_src_rect[i].height = cxt->req_param.post_proc_setting.chn_out_frm[i].size.height;
			}
		}
	}
	}
	CMR_LOGI("@xin channel id %d is scaling %d", cxt->req_param.channel_id, cxt->chn_param.is_scaling);
}

cmr_int snp_clean_thumb_param(cmr_handle snp_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	cmr_uint                        i;
	struct snp_context              *cxt = (struct snp_context*)snp_handle;
	struct snp_jpeg_param           *jpeg_ptr = &cxt->chn_param.thumb_in[0];

	CMR_LOGI("clean thumb param");
	for (i=0 ; i<CMR_CAPTURE_MEM_SUM ; i++) {
		cmr_bzero(&jpeg_ptr->src, sizeof(struct img_frm));
		cmr_bzero(&jpeg_ptr->dst, sizeof(struct img_frm));
		jpeg_ptr++;
	}
	cxt->thumb_stream_size = 0;
	return ret;
}

cmr_int snp_set_post_proc_param(cmr_handle snp_handle, struct snapshot_param *param_ptr)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct snp_context              *cxt = (struct snp_context*)snp_handle;
	struct snp_proc_param           *proc_param_ptr = &cxt->req_param.post_proc_setting;
	cmr_s32                         sm_val = 0;
	cmr_s32                         is_normal_cap = 0;

	cxt->cap_cnt = 0;
	cxt->chn_param.is_rot = 0;
	cxt->req_param = *param_ptr;
	CMR_LOGI("@xin chn %d total num %d", cxt->req_param.channel_id, cxt->req_param.total_num);
	if (cxt->req_param.is_video_snapshot || cxt->req_param.is_zsl_snapshot) {
		is_normal_cap = 0;
	} else {
		is_normal_cap = 1;
	}
	ret = cxt->ops.get_sensor_info(cxt->oem_handle, cxt->req_param.camera_id, &cxt->sensor_info);
	if (ret) {
		CMR_LOGE("failed to get sn info %ld", ret);
		goto exit;
	}

	ret = snp_set_jpeg_dec_param(snp_handle);
	if (ret) {
		CMR_LOGE("failed to set rot param %ld", ret);
		goto exit;
	}

	ret = snp_set_isp_proc_param(snp_handle);
	if (ret) {
		CMR_LOGE("failed to set rot param %ld", ret);
		goto exit;
	}

	ret = snp_set_channel_out_param(snp_handle);
	if (ret) {
		CMR_LOGE("failed to set out %ld", ret);
		goto exit;
	}
	ret = snp_set_hdr_param(snp_handle);
	if (ret) {
		CMR_LOGE("failed to set hdr param %ld", ret);
		goto exit;
	}
	snp_get_is_scaling(snp_handle, is_normal_cap);

	ret = snp_set_rot_param(snp_handle);
	if (ret) {
		CMR_LOGE("failed to set rot param %ld", ret);
		goto exit;
	}

	if (cxt->chn_param.is_scaling) {
		ret = snp_set_scale_param(snp_handle);
		if (ret) {
			CMR_LOGE("failed to set scale param %ld", ret);
			goto exit;
		}
	}

	ret = snp_set_jpeg_enc_param(snp_handle);
	if (ret) {
		CMR_LOGE("failed to set rot param %ld", ret);
		goto exit;
	}
	if (0 != cxt->req_param.jpeg_setting.thum_size.width
		&& 0 != cxt->req_param.jpeg_setting.thum_size.height) {
		ret = snp_set_convert_thumb_param(snp_handle);
		if (ret) {
			CMR_LOGE("failed to set convert thumb param %ld", ret);
			goto exit;
		}
		ret = snp_set_jpeg_thumb_param(snp_handle);
		if (ret) {
			CMR_LOGE("failed to set jpeg thumb param %ld", ret);
			goto exit;
		}
	} else {
		ret = snp_clean_thumb_param(snp_handle);
	}

	ret = snp_set_jpeg_exif_param(snp_handle);
	if (ret) {
		CMR_LOGE("failed to set exif param %ld", ret);
	} else {
		sem_getvalue(&cxt->redisplay_sm, &sm_val);
		if (0 != sm_val) {
			sem_destroy(&cxt->redisplay_sm);
			sem_init(&cxt->redisplay_sm, 0, 0);
			CMR_LOGI("re-initialize redisplay sm");
		}
		sem_getvalue(&cxt->writer_exif_sm, &sm_val);
		if (0 != sm_val) {
			sem_destroy(&cxt->writer_exif_sm);
			sem_init(&cxt->writer_exif_sm, 0, 0);
			CMR_LOGI("re-initialize write exif sm");
		}
	}

exit:
	return ret;
}

void snp_set_request(cmr_handle snp_handle, cmr_u32 is_request)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct snp_context              *cxt = (struct snp_context*)snp_handle;

	sem_wait(&cxt->access_sm);
	cxt->is_request = is_request;
	sem_post(&cxt->access_sm);
	CMR_LOGI("request %d", cxt->is_request);
}

cmr_u32 snp_get_request(cmr_handle snp_handle)
{
	cmr_u32                         is_request = 0;
	struct snp_context              *cxt = (struct snp_context*)snp_handle;

	sem_wait(&cxt->access_sm);
	is_request = cxt->is_request;
	sem_post(&cxt->access_sm);
	CMR_LOGI("req %d", is_request);
	return is_request;
}

cmr_int snp_checkout_exit(cmr_handle snp_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	cmr_u32                         is_request = 0;
	struct snp_context              *cxt = (struct snp_context*)snp_handle;

	if (0 == snp_get_request(snp_handle)) {
		if (IPM_WORKING == snp_get_status(snp_handle)) {
			sem_post(&cxt->hdr_sync_sm);
			CMR_LOGI("post hdr sm");
		}
		if (CODEC_WORKING == snp_get_status(snp_handle)) {
			if (cxt->ops.stop_codec) {
				sem_post(&cxt->jpeg_sync_sm);
				ret = cxt->ops.stop_codec(cxt->oem_handle);
				if (ret) {
					CMR_LOGE("failed to stop codec %ld", ret);
				}
			}
		}
		if (WRITE_EXIF == snp_get_status(snp_handle)) {
			sem_wait(&cxt->writer_exif_sm);
		}
		CMR_LOGI("cxt->oem_cb is 0x%lx", (cmr_uint)cxt->oem_cb);
		if (cxt->oem_cb) {
			//cxt->oem_cb(cxt->oem_handle, SNAPSHOT_RSP_CB_SUCCESS, SNAPSHOT_FUNC_RELEASE_PICTURE, NULL);
			snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_RELEASE_PICTURE, SNAPSHOT_RSP_CB_SUCCESS, NULL, 0);
		}
		ret = CMR_CAMERA_NORNAL_EXIT;
		if (cxt->ops.channel_free_frame) {
			cxt->ops.channel_free_frame(cxt->oem_handle, cxt->cur_frame_info.channel_id, cxt->cur_frame_info.frame_id);
			CMR_LOGI("free frame");
		}
	}
	return ret;
}

cmr_int snp_check_frame_is_valid(cmr_handle snp_handle, struct frm_info *frm_ptr)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;

	if (frm_ptr->channel_id != cxt->req_param.channel_id) {
		CMR_LOGI("don't need to process this frame %d %d", frm_ptr->channel_id, cxt->req_param.channel_id);
		ret = CMR_CAMERA_NORNAL_EXIT;
		goto exit;
	}

	if ((cxt->cap_cnt > cxt->req_param.total_num) || (!IS_CAP_FRM(frm_ptr->frame_id, frm_ptr->base))) {
		CMR_LOGI("this is invalid frame %d %d %d %d", cxt->cap_cnt ,cxt->req_param.total_num,
			frm_ptr->frame_id, frm_ptr->base);
		ret = CMR_CAMERA_INVALID_FRAME;
	}

exit:
	return ret;
}
cmr_int snp_send_msg_notify_thr(cmr_handle snp_handle, cmr_int func_type, cmr_int evt, void *data, cmr_uint data_len)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	cmr_u32                        sync_flag = CMR_MSG_SYNC_PROCESSED;
	CMR_MSG_INIT(message);

	CMR_LOGI("evt %ld", evt);
	if (SNAPSHOT_FUNC_STATE == func_type) {
		sync_flag = CMR_MSG_SYNC_NONE;
	} else if (((SNAPSHOT_FUNC_TAKE_PICTURE == func_type) || (SNAPSHOT_FUNC_ENCODE_PICTURE == func_type))
				&& (SNAPSHOT_EXIT_CB_DONE == evt)) {
		sync_flag = CMR_MSG_SYNC_NONE;
	} else {
		switch (evt) {
		case SNAPSHOT_EVT_CB_SNAPSHOT_DONE:
			sync_flag = CMR_MSG_SYNC_NONE;
			break;
		default:
			sync_flag = CMR_MSG_SYNC_PROCESSED;
			break;
		}
	}
	if (data) {
		message.data = malloc(data_len);
		if (!message.data) {
			ret = CMR_CAMERA_NO_MEM;
			goto exit;
		} else {
			memcpy(message.data, data, data_len);
			message.alloc_flag = 1;
		}
	} else {
		message.alloc_flag = 0;
	}
	message.msg_type = evt;
	message.sub_msg_type = func_type;
	message.sync_flag = sync_flag;
	ret = cmr_thread_msg_send(cxt->thread_cxt.notify_thr_handle, &message);
	if (ret) {
		CMR_LOGE("failed to send exit msg to notify thr %ld", ret);
		if (message.data) {
			free(message.data);
			message.data = NULL;
		}
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int snp_send_msg_secondary_thr(cmr_handle snp_handle, cmr_int func_type, cmr_int evt, void *data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	CMR_MSG_INIT(message);

	CMR_LOGI("evt 0x%ld", evt);
	message.msg_type = evt;
	message.sub_msg_type = func_type;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.alloc_flag = 0;
	message.data = data;
	ret = cmr_thread_msg_send(cxt->thread_cxt.secondary_thr_handle, &message);
	if (ret) {
		CMR_LOGE("failed to send exit msg to notify thr %ld", ret);
	}
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int snp_send_msg_write_exif_thr(cmr_handle snp_handle, cmr_int evt, void *data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	CMR_MSG_INIT(message);

	CMR_LOGI("evt 0x%ld", evt);
	sem_wait(&cxt->thread_cxt.writte_exif_access_sm);
	message.msg_type = evt;
	message.sync_flag = CMR_MSG_SYNC_RECEIVED;
	message.alloc_flag = 1;
	message.data = malloc(sizeof(struct frm_info));
	if (!message.data) {
		CMR_LOGE("failed to malloc");
		sem_post(&cxt->thread_cxt.writte_exif_access_sm);
		return -CMR_CAMERA_NO_MEM;
	} else {
		cmr_copy(message.data, data, sizeof(struct frm_info));
	}
	ret = cmr_thread_msg_send(cxt->thread_cxt.write_exif_thr_handle, &message);
	if (ret) {
		CMR_LOGE("failed to send msg to write thr %ld", ret);
	}
	CMR_LOGI("done %ld", ret);
	sem_post(&cxt->thread_cxt.writte_exif_access_sm);
	return ret;
}

cmr_int snp_send_msg_redisplay_thr(cmr_handle snp_handle, cmr_int evt, void *data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	CMR_MSG_INIT(message);

	CMR_LOGI("evt 0x%ld", evt);
	message.msg_type = evt;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	message.alloc_flag = 1;
	message.data = malloc(sizeof(struct frm_info));
	if (!message.data) {
		CMR_LOGE("failed to malloc");
		return -CMR_CAMERA_NO_MEM;
	} else {
		cmr_copy(message.data, data, sizeof(struct frm_info));
	}
	ret = cmr_thread_msg_send(cxt->thread_cxt.proc_redisplay_handle, &message);
	if (ret) {
		CMR_LOGE("failed to send msg to redisplay thr %ld", ret);
	}
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int snp_send_msg_thumb_thr(cmr_handle snp_handle, cmr_int evt, void *data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	CMR_MSG_INIT(message);

	CMR_LOGI("evt 0x%ld", evt);
	message.msg_type = evt;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	message.alloc_flag = 1;
	message.data = malloc(sizeof(struct frm_info));
	if (!message.data) {
		CMR_LOGE("failed to malloc");
		return -CMR_CAMERA_NO_MEM;
	} else {
		cmr_copy(message.data, data, sizeof(struct frm_info));
	}
	ret = cmr_thread_msg_send(cxt->thread_cxt.proc_thumb_handle, &message);
	if (ret) {
		CMR_LOGE("failed to send msg to thumb thr %ld", ret);
	}
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int camera_set_frame_type(cmr_handle snp_handle, struct camera_frame_type *frame_type, struct frm_info* info)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	struct snapshot_param          *req_param_ptr;
	struct cmr_cap_mem             *mem_ptr;
	cmr_u32                        frm_id = info->frame_id - info->base;
	cmr_s8                         value[PROPERTY_VALUE_MAX];

	req_param_ptr = &cxt->req_param;
	mem_ptr = &req_param_ptr->post_proc_setting.mem[frm_id];
	frame_type->buf_id = info->frame_real_id;
	frame_type->width = req_param_ptr->post_proc_setting.actual_snp_size.width;
	frame_type->height = req_param_ptr->post_proc_setting.actual_snp_size.height;
	frame_type->timestamp = info->sec * 1000000000LL + info->usec * 1000;
	switch (req_param_ptr->mode) {
	case CAMERA_UTEST_MODE:
		frame_type->y_vir_addr = mem_ptr->target_yuv.addr_vir.addr_y;
		frame_type->format = info->fmt;
		break;
	case CAMERA_AUTOTEST_MODE:
		frame_type->y_vir_addr = req_param_ptr->post_proc_setting.chn_out_frm[frm_id].addr_vir.addr_y;
		frame_type->format = info->fmt;
		break;
	default:
		{
		cmr_u32 size = cxt->req_param.post_proc_setting.actual_snp_size.width*cxt->req_param.post_proc_setting.actual_snp_size.height;
		frame_type->y_vir_addr = mem_ptr->target_yuv.addr_vir.addr_y;
		frame_type->y_phy_addr = mem_ptr->target_yuv.addr_phy.addr_y;
		frame_type->uv_vir_addr = mem_ptr->target_yuv.addr_vir.addr_u;
		frame_type->uv_phy_addr = mem_ptr->target_yuv.addr_phy.addr_u;
		frame_type->format = CAMERA_YCBCR_4_2_0;
		if (CAMERA_ISP_TUNING_MODE == cxt->req_param.mode) {
			send_capture_data(0x40,/* yvu420 */
							  cxt->req_param.post_proc_setting.actual_snp_size.width,
							  cxt->req_param.post_proc_setting.actual_snp_size.height,
							  (char *)mem_ptr->target_yuv.addr_vir.addr_y,
							  size, (char *)mem_ptr->target_yuv.addr_vir.addr_u,
							  size/2, 0, 0);
			ret = camera_save_to_file(isp_get_saved_file_count(snp_handle),
										IMG_DATA_TYPE_YUV420,
										frame_type->width,
										frame_type->height,
										&mem_ptr->target_yuv.addr_vir);
		}
		}
		break;
	}

	property_get("debug.camera.save.snpfile", value, "0");
	if (atoi(value) == 3) {
		camera_save_to_file(SNP_REDISPLAY_DATA, IMG_DATA_TYPE_YUV420,
							frame_type->width,
							frame_type->height,
							&mem_ptr->target_yuv.addr_vir);
	}

	if (cxt->req_param.lls_shot_mode || cxt->req_param.is_vendor_hdr) {
		CMR_LOGI("vendor capture cap_cnt = %d, total_num = %d",
					cxt->cap_cnt, cxt->req_param.total_num);
		if (0 == snp_get_request(snp_handle) || (cxt->cap_cnt == cxt->req_param.total_num)) {
			frame_type->need_free = 1;
		} else {
			frame_type->need_free = 0;
		}
	}

	CMR_LOGI("index 0x%x, addr 0x%lx 0x%lx, w h %d %d, format %d, sec %ld usec %ld",
			info->frame_id,
			frame_type->y_vir_addr,
			frame_type->y_phy_addr,
			frame_type->width,
			frame_type->height,
			frame_type->format,
			info->sec,
			info->usec);
	return ret;
}

void snp_takepic_callback_done(cmr_handle snp_handle)
{
	struct snp_context             *cxt = (struct snp_context*)snp_handle;

	sem_post(&cxt->takepic_callback_sem);
	CMR_LOGI("done");
}

void snp_takepic_callback_wait(cmr_handle snp_handle)
{
	struct snp_context             *cxt = (struct snp_context*)snp_handle;

	CMR_LOGI("wait beging");
	sem_wait(&cxt->takepic_callback_sem);
	CMR_LOGI("wait end");
}

cmr_int snp_redisplay(cmr_handle snp_handle, struct frm_info *data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;

	if (!data) {
		CMR_LOGE("param error");
		goto exit;
	}
	ret = snp_send_msg_redisplay_thr(snp_handle, SNP_EVT_REDISPLAY, data);
exit:
	CMR_LOGI("done");
	return ret;
}

cmr_int snp_start_thumb_proc(cmr_handle snp_handle, struct frm_info *data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;

	if (!data) {
		CMR_LOGE("param error");
		ret = CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	if ((0 != cxt->req_param.jpeg_setting.thum_size.width)
		&& (0 != cxt->req_param.jpeg_setting.thum_size.height)) {
		ret = snp_send_msg_thumb_thr(snp_handle, SNP_EVT_THUMB, data);
		if (ret) {
			CMR_LOGE("failed to send msg thumb %ld", ret);
			goto exit;
		}
	}
exit:
	if ((0 != cxt->req_param.jpeg_setting.thum_size.width)
		&& (0 != cxt->req_param.jpeg_setting.thum_size.height)
		&& (CMR_CAMERA_SUCCESS != ret)) {
		sem_post(&cxt->scaler_sync_sm);
	}
	return ret;
}

cmr_int snp_notify_redisplay_proc(cmr_handle snp_handle, cmr_int func_type, cmr_int evt, void *data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;

	if (!snp_handle) {
		CMR_LOGE("param error");
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	CMR_LOGI("evt 0x%lx, func 0x%lx", evt, func_type);

	if (SNAPSHOT_CB_MAX > evt) {
		if (cxt->oem_cb) {
			cxt->oem_cb(cxt->oem_handle, evt, func_type, data);
		} else {
			CMR_LOGE("err, oem cb is null");
		}
	} else {
		CMR_LOGE("don't support this 0x%lx", evt);
	}
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int snp_take_picture_done(cmr_handle snp_handle, struct frm_info *data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	struct camera_frame_type       frame_type;

	if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
		CMR_LOGI("post proc has been cancel");
		sem_post(&cxt->redisplay_sm);
		ret = CMR_CAMERA_NORNAL_EXIT;
		goto exit;
	}
	if (!data) {
		ret = CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	cmr_bzero(&frame_type, sizeof(frame_type));
	CMR_PRINT_TIME;
	ret = camera_set_frame_type(snp_handle, &frame_type, data);
	if (CMR_CAMERA_SUCCESS == ret) {
		snp_notify_redisplay_proc(snp_handle,
								SNAPSHOT_FUNC_TAKE_PICTURE,
								SNAPSHOT_EVT_CB_SNAPSHOT_DONE,
								(void*)&frame_type);
		CMR_LOGI("SNAPSHOT_EVT_CB_SNAPSHOT_DONE.");
		snp_notify_redisplay_proc(snp_handle,
								SNAPSHOT_FUNC_TAKE_PICTURE,
								SNAPSHOT_EXIT_CB_DONE,
								(void*)&frame_type);
		CMR_LOGI("SNAPSHOT_EXIT_CB_DONE.");
	} else {
		snp_notify_redisplay_proc(snp_handle,
								SNAPSHOT_FUNC_TAKE_PICTURE,
								SNAPSHOT_EXIT_CB_FAILED,NULL);
	}

	if (cxt->req_param.lls_shot_mode || cxt->req_param.is_vendor_hdr) {
		snp_notify_redisplay_proc(snp_handle,
						SNAPSHOT_FUNC_TAKE_PICTURE,
						SNAPSHOT_EVT_CB_SNAPSHOT_JPEG_DONE,
						(void*)&frame_type);
	} else {
		snp_notify_redisplay_proc(snp_handle,
							SNAPSHOT_FUNC_TAKE_PICTURE,
							SNAPSHOT_EVT_CB_SNAPSHOT_JPEG_DONE,NULL);
	}
	sem_post(&cxt->redisplay_sm);
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int snp_thumbnail(cmr_handle snp_handle, struct frm_info *data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;

	if (!data) {
		CMR_LOGE("param error");
		ret = CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	ret = snp_start_convet_thumb(snp_handle, data);
	if (ret) {
		CMR_LOGE("failed to convert thumb %ld", ret);
		goto exit;
	}
	ret = snp_start_encode_thumb(snp_handle);
	if (ret) {
		CMR_LOGE("failed to start encode thumb %ld", ret);
		goto exit;
	}
exit:
	sem_post(&cxt->scaler_sync_sm);
	return ret;
}
cmr_int snp_post_proc_for_yuv(cmr_handle snp_handle, void *data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct frm_info                *chn_data_ptr = (struct frm_info*)data;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	struct snp_channel_param       *chn_param_ptr = &cxt->chn_param;
	cmr_u32                        index;
	cmr_s8                         value[PROPERTY_VALUE_MAX];

	if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
		CMR_LOGI("post proc has been cancel");
		ret = CMR_CAMERA_NORNAL_EXIT;
		return ret;
	}

	cxt->cap_cnt++;

	snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_TAKE_PICTURE,
								SNAPSHOT_EVT_CB_CAPTURE_FRAME_DONE, NULL, 0);
	if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
		CMR_LOGI("post proc has been cancel");
		ret = CMR_CAMERA_NORNAL_EXIT;
		goto exit;
	}
	snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_TAKE_PICTURE,
							SNAPSHOT_RSP_CB_SUCCESS, NULL, 0);
	if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
		CMR_LOGI("post proc has been cancel");
		ret = CMR_CAMERA_NORNAL_EXIT;
		goto exit;
	}
	snp_set_status(snp_handle, POST_PROCESSING);

	if (cxt->req_param.lls_shot_mode || cxt->req_param.is_vendor_hdr) {
		if (chn_param_ptr->is_rot) {
			CMR_LOGI("need rotate");
			ret = snp_start_rot(snp_handle, data);
			if (ret) {
				CMR_LOGE("failed to start rot %ld", ret);
				goto exit;
			}
		}

		cxt->cur_frame_info = *chn_data_ptr;
		if (chn_param_ptr->is_scaling) {
			ret = snp_start_scale(snp_handle, data);
			if (ret) {
				CMR_LOGE("failed to start scale %ld", ret);
				goto exit;
			}
		} else {
			ret = snp_take_picture_done(snp_handle, chn_data_ptr);
			snp_post_proc_done(snp_handle);
		}
		goto exit;
	}

	CMR_LOGI("$$LHC:size %d %d yaddr 0x%lx", chn_param_ptr->chn_frm[chn_data_ptr->frame_id-chn_data_ptr->base].size.width,
		chn_param_ptr->chn_frm[chn_data_ptr->frame_id-chn_data_ptr->base].size.height,
		chn_param_ptr->chn_frm[chn_data_ptr->frame_id-chn_data_ptr->base].addr_vir.addr_y);
	camera_take_snapshot_step(CMR_STEP_CAP_E);
	property_get("debug.camera.save.snpfile", value, "0");
	if (atoi(value) == 1) {
		camera_save_to_file(SNP_CHN_OUT_DATA, IMG_DATA_TYPE_YUV420,
							chn_param_ptr->chn_frm[chn_data_ptr->frame_id-chn_data_ptr->base].size.width,
							chn_param_ptr->chn_frm[chn_data_ptr->frame_id-chn_data_ptr->base].size.height,
							&chn_param_ptr->chn_frm[chn_data_ptr->frame_id-chn_data_ptr->base].addr_vir);
	}

	if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
		CMR_LOGI("post proc has been cancel");
		ret = CMR_CAMERA_NORNAL_EXIT;
		goto exit;
	}

	if (CMR_CAMERA_SUCCESS != snp_send_msg_write_exif_thr(snp_handle, SNP_EVT_WRITE_EXIF, data)) {
		goto exit;
	}
	if (chn_param_ptr->is_rot) {
		CMR_LOGI("need rotate");
		ret = snp_start_rot(snp_handle, data);
		if (ret) {
			CMR_LOGE("failed to start rot %ld", ret);
			sem_post(&cxt->scaler_sync_sm);
			sem_post(&cxt->jpeg_sync_sm);
			goto exit;
		}
	}

	cxt->cur_frame_info = *chn_data_ptr;
	if (chn_param_ptr->is_scaling) {
		ret = snp_start_scale(snp_handle, data);
		if (ret) {
			CMR_LOGE("failed to start scale %ld", ret);
			sem_post(&cxt->scaler_sync_sm);
			sem_post(&cxt->jpeg_sync_sm);
			goto exit;
		}
	}

	if (!chn_param_ptr->is_scaling) {
		ret = snp_start_encode(snp_handle, data);
		if (ret) {
			CMR_LOGE("failed to start encode %ld", ret);
			sem_post(&cxt->scaler_sync_sm);
			goto exit;
		}
		if ((0 != cxt->req_param.jpeg_setting.thum_size.width)
			&& (0 != cxt->req_param.jpeg_setting.thum_size.height)) {
			ret = snp_start_thumb_proc(snp_handle, data);
			if (ret) {
				CMR_LOGE("failed to start thumb proc %ld", ret);
				goto exit;
			}
		}
		ret = snp_redisplay(snp_handle, data);
		if (ret) {
			CMR_LOGE("failed to take pic done %ld", ret);
			sem_post(&cxt->scaler_sync_sm);
			goto exit;
		}
		if ((0 == cxt->req_param.jpeg_setting.thum_size.width)
			|| (0 == cxt->req_param.jpeg_setting.thum_size.height)) {
			sem_post(&cxt->scaler_sync_sm);
		}
/*		if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
			CMR_LOGI("post proc has been cancel");
			ret = CMR_CAMERA_NORNAL_EXIT;
			goto exit;
		}*/
	}

exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int snp_post_proc_for_jpeg(cmr_handle snp_handle, void *data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct frm_info                *chn_data_ptr = (struct frm_info*)data;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	struct snp_channel_param       *chn_param_ptr = &cxt->chn_param;

	camera_take_snapshot_step(CMR_STEP_JPG_DEC_S);
	ret = snp_start_decode_sync(snp_handle, data);
	camera_take_snapshot_step(CMR_STEP_JPG_DEC_E);
	if (ret) {
		return ret;
	}
	chn_data_ptr->fmt = IMG_DATA_TYPE_YUV420;
	ret = snp_post_proc_for_yuv(snp_handle, data);
	return ret;
}

cmr_int isp_is_have_src_data_from_picture(void)
{
	FILE* fp = 0;
	char isptool_src_mipi_raw[] = SRC_MIPI_RAW;

	fp = fopen(isptool_src_mipi_raw, "r");
	if (fp != NULL) {
		fclose(fp);
		CMR_LOGI("have input_raw source file");
	} else {
		CMR_LOGE("no input_raw source file");
		return -CMR_CAMERA_FAIL;
	}

	return CMR_CAMERA_SUCCESS;

}
void isp_set_saved_file_count(cmr_handle snp_handle)
{
	struct snp_context             *cxt = (struct snp_context*)snp_handle;

	cxt->isptool_saved_file_count++;
}

cmr_u32 isp_get_saved_file_count(cmr_handle snp_handle)
{
	struct snp_context             *cxt = (struct snp_context*)snp_handle;

	return cxt->isptool_saved_file_count;
}

cmr_int isp_overwrite_cap_mem(cmr_handle snp_handle)
{
	FILE* fp = 0;
	char isptool_src_mipi_raw[] = SRC_MIPI_RAW;
	struct camera_frame_type       frame_type;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	struct cmr_cap_mem             *mem_ptr = &cxt->req_param.post_proc_setting.mem[0];
	cmr_u32                        pixel_width = cxt->sensor_info.sn_interface.pixel_width;
	cmr_u32                        memsize;

	memsize = mem_ptr->cap_raw.size.width * mem_ptr->cap_raw.size.height * pixel_width / 8;
	fp = fopen(isptool_src_mipi_raw, "r");
	if (fp != NULL) {
		fread((void *)mem_ptr->cap_raw.addr_vir.addr_y, 1, memsize, fp);
		fclose(fp);
	} else {
		CMR_LOGE("no input_raw source file");
		return -CMR_CAMERA_FAIL;
	}

	isp_set_saved_file_count(snp_handle);

	frame_type.y_vir_addr = mem_ptr->cap_raw.addr_vir.addr_y;
	frame_type.y_phy_addr = mem_ptr->cap_raw.addr_phy.addr_y;
	frame_type.width = mem_ptr->cap_raw.size.width;
	frame_type.height = mem_ptr->cap_raw.size.height * pixel_width /8;

	snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_TAKE_PICTURE,
							SNAPSHOT_EVT_CB_FLUSH, (void*)&frame_type, sizeof(struct camera_frame_type));

	return CMR_CAMERA_SUCCESS;

}

cmr_int snp_post_proc_for_isp_tuning(cmr_handle snp_handle, void *data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct frm_info                *chn_data_ptr = (struct frm_info*)data;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	CMR_MSG_INIT(message);

	if (!isp_is_have_src_data_from_picture()) {
		isp_overwrite_cap_mem(snp_handle);
	}

	message.msg_type = SNP_EVT_START_CVT;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.alloc_flag = 0;
	message.sub_msg_type = SNP_TRIGGER;
	message.data = data;
	ret = cmr_thread_msg_send(cxt->thread_cxt.cvt_thr_handle, &message);
	if (ret) {
		CMR_LOGE("failed to send start cvt msg to cvt thr %ld", ret);
	}
	chn_data_ptr->fmt = IMG_DATA_TYPE_YUV420;
	ret = snp_post_proc_for_yuv(snp_handle, data);
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int snp_post_proc_for_raw(cmr_handle snp_handle, void *data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct frm_info                *chn_data_ptr = (struct frm_info*)data;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	struct snp_channel_param       *chn_param_ptr = &cxt->chn_param;
	CMR_MSG_INIT(message);

	CMR_LOGI("snapshot mode %d", cxt->req_param.mode);

	switch (cxt->req_param.mode) {
	case CAMERA_UTEST_MODE:
		ret = snp_take_picture_done(snp_handle, (struct frm_info*)data);
		break;
	case CAMERA_ISP_TUNING_MODE:
		ret = snp_post_proc_for_isp_tuning(snp_handle, data);
		break;
	default:
	{
		message.msg_type = SNP_EVT_START_CVT;
		message.sync_flag = CMR_MSG_SYNC_PROCESSED;
		message.alloc_flag = 0;
		message.sub_msg_type = SNP_TRIGGER;
		message.data = data;
		ret = cmr_thread_msg_send(cxt->thread_cxt.cvt_thr_handle, &message);
		if (ret) {
			CMR_LOGE("failed to send stop msg to cvt thr %ld", ret);
			break;
		}
		if (cxt->err_code) {
			CMR_LOGE("failed to convert format %ld", cxt->err_code);
			ret = cxt->err_code;
			break;
		}
		chn_data_ptr->fmt = IMG_DATA_TYPE_YUV420;
		ret = snp_post_proc_for_yuv(snp_handle, data);
	}
		break;
	}

	return ret;
}

void snp_autotest_proc(cmr_handle snp_handle, void *data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct frm_info                *chn_data_ptr = (struct frm_info*)data;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;

	ret = snp_take_picture_done(snp_handle, data);
	if (ret) {
		CMR_LOGE("failed to take pic done %ld", ret);
	}
	sem_post(&cxt->proc_done_sm);
	CMR_LOGI("done %ld", ret);
}

void snp_reset_ctrl_condition(cmr_handle snp_handle)
{
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	cmr_s32                        sm_val = 0;

	if (!snp_handle) {
		CMR_LOGE("handle is null");
		return;
	}

	sem_getvalue(&cxt->writer_exif_sm, &sm_val);
	if (0 != sm_val) {
		sem_destroy(&cxt->writer_exif_sm);
		sem_init(&cxt->writer_exif_sm, 0, 0);
		CMR_LOGI("re-initialize writer exif sm");
	}

	sem_getvalue(&cxt->jpeg_sync_sm, &sm_val);
	if (0 != sm_val) {
		sem_destroy(&cxt->jpeg_sync_sm);
		sem_init(&cxt->jpeg_sync_sm, 0, 0);
		CMR_LOGI("re-initialize jpeg sync sm");
	}

	sem_getvalue(&cxt->scaler_sync_sm, &sm_val);
	if (0 != sm_val) {
		sem_destroy(&cxt->scaler_sync_sm);
		sem_init(&cxt->scaler_sync_sm, 0, 0);
		CMR_LOGI("re-initialize scaler sync sm");
	}
}

cmr_int snp_post_proc(cmr_handle snp_handle, void *data)
{
	cmr_int                        ret = CMR_CAMERA_SUCCESS;
	struct frm_info                *chn_data_ptr = (struct frm_info*)data;
	struct snp_context             *cxt = (struct snp_context*)snp_handle;
	struct snp_channel_param       *chn_param_ptr = &cxt->chn_param;
	cmr_u32                        fmt;
	struct buffer_cfg              buf_cfg;

	if (!snp_handle || !data) {
		CMR_LOGE("error param 0x%lx, 0x%lx", (cmr_uint)snp_handle, (cmr_uint)data);
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
		CMR_LOGI("post proc has been cancel");
		return ret;
	}
	CMR_LOGI("waiting begin");
	sem_wait(&cxt->proc_done_sm);
	CMR_LOGI("waiting end");

	snp_reset_ctrl_condition(snp_handle);
	cxt->err_code = 0;
	if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
		CMR_LOGI("post proc has been cancel");
		ret = CMR_CAMERA_NORNAL_EXIT;
		goto exit;
	}

	ret = snp_check_frame_is_valid(snp_handle, chn_data_ptr);
	if (CMR_CAMERA_NORNAL_EXIT == ret) {
		goto exit;
	} else if (CMR_CAMERA_INVALID_FRAME == ret) {
		if (cxt->ops.channel_buff_cfg && !cxt->req_param.is_video_snapshot) {
			buf_cfg.channel_id = chn_data_ptr->channel_id;
			buf_cfg.base_id = CMR_BASE_ID(chn_data_ptr->frame_id);
			buf_cfg.count = 1;
			buf_cfg.flag           = BUF_FLAG_RUNNING;
			buf_cfg.addr[0].addr_y = chn_data_ptr->yaddr;
			buf_cfg.addr[0].addr_u = chn_data_ptr->uaddr;
			buf_cfg.addr[0].addr_v = chn_data_ptr->vaddr;
			buf_cfg.addr_vir[0].addr_y = chn_data_ptr->yaddr_vir;
			buf_cfg.addr_vir[0].addr_u = chn_data_ptr->uaddr_vir;
			buf_cfg.addr_vir[0].addr_v = chn_data_ptr->vaddr_vir;
			cxt->ops.channel_buff_cfg(cxt->oem_handle, &buf_cfg);
			CMR_LOGI("free frame");
		}
		goto exit;
	}
	CMR_LOGI("path data index 0x%x 0x%x", chn_data_ptr->frame_id, chn_data_ptr->base);
	cxt->index = chn_data_ptr->frame_id - chn_data_ptr->base;
	fmt =  chn_data_ptr->fmt;
	CMR_LOGI("index %d", cxt->index);
	if (CAMERA_AUTOTEST_MODE == cxt->req_param.mode) {
		snp_autotest_proc(snp_handle, data);
		return ret;
	} else {
		switch (fmt) {
		case IMG_DATA_TYPE_JPEG:
			ret = snp_post_proc_for_jpeg(snp_handle, data);
			break;
		case IMG_DATA_TYPE_YUV420:
		case IMG_DATA_TYPE_YVU420:
		case IMG_DATA_TYPE_YUV422:
			ret = snp_post_proc_for_yuv(snp_handle, data);
			break;
		case IMG_DATA_TYPE_RAW:
			ret = snp_post_proc_for_raw(snp_handle, data);
			break;
		default:
			CMR_LOGE("err, fmt %d", fmt);
			ret = CMR_CAMERA_INVALID_FORMAT;
			break;
		}
	}
exit:
	if (ret) {
		cxt->err_code = ret;
		snp_post_proc_err_exit(snp_handle, ret);
	}
	//got capture frm stop flash
      if (cxt->ops.capture_close_flash) {
             cxt->ops.capture_close_flash(cxt->oem_handle,cxt->camera_id);
      }
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int snp_proc_hdr_src(cmr_handle snp_handle, void *data)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	cmr_int                         is_change_fmt = 0;
	struct snp_context              *cxt = (struct snp_context*)snp_handle;
	struct frm_info                 *data_ptr = (struct frm_info*)data;
	struct img_frm                  out_param;
	struct ipm_frame_in             ipm_in_param;

	if (CMR_CAMERA_NORNAL_EXIT == snp_checkout_exit(snp_handle)) {
		CMR_LOGI("snp has been cancel");
		return ret;
	}
	if (IMG_DATA_TYPE_JPEG == data_ptr->fmt || IMG_DATA_TYPE_RAW == data_ptr->fmt) {
		is_change_fmt = 1;
	}
	if (is_change_fmt) {
		ret = cmr_snapshot_format_convert((cmr_handle)cxt, data, &out_param);//sync
		if (ret) {
			CMR_LOGE("failed to format convert %ld", ret);
			return ret;
		}
	} else {
		out_param = cxt->req_param.post_proc_setting.chn_out_frm[data_ptr->frame_id-data_ptr->base];
	}
	cxt->hdr_src_num++;
	ipm_in_param.src_frame = out_param;
	ipm_in_param.dst_frame = out_param;
	ipm_in_param.private_data = (void*)cxt->oem_handle;
	ret = ipm_transfer_frame(cxt->req_param.hdr_handle, &ipm_in_param, NULL);/*async*/
	if (ret) {
		CMR_LOGE("failed to transfer frame to ipm %ld", ret);
		return ret;
	}
	cmr_snapshot_memory_flush(snp_handle);
	if(cxt->hdr_src_num == cxt->req_param.hdr_need_frm_num) {
		snp_set_status(snp_handle, IPM_WORKING);
		sem_wait(&cxt->hdr_sync_sm);
		snp_set_status(snp_handle, POST_PROCESSING);
		cmr_snapshot_memory_flush(snp_handle);
	}
	return ret;
}

cmr_int snp_stop_proc(cmr_handle snp_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct snp_context              *cxt = (struct snp_context*)snp_handle;

	CMR_LOGI("s");
	ret = snp_checkout_exit(snp_handle);
	snp_set_status(snp_handle, IDLE);
	CMR_LOGI("e");
	return ret;
}
cmr_int snp_proc_android_zsl_data(cmr_handle snp_handle, void *data)
{
	UNUSED(data);

	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct snp_context              *cxt = (struct snp_context*)snp_handle;
#if 0
	struct frm_info                 *frm_ptr = (struct frm_info*)data;
	struct camera_cap_frm_info      frame;
	cmr_u32                         index = frm_ptr->frame_id - frm_ptr->base;

	if (frm_ptr->channel_id != cxt->req_param.channel_id) {
		CMR_LOGI("don't need to process this frame %d %d", frm_ptr->channel_id, cxt->req_param.channel_id);
		ret = CMR_CAMERA_NORNAL_EXIT;
		goto exit;
	}

	if (!IS_CAP_FRM(frm_ptr->frame_id, frm_ptr->base)) {
		CMR_LOGI("this is invalid frame");
		if (cxt->ops.channel_free_frame) {
			cxt->ops.channel_free_frame(cxt->oem_handle, frm_ptr->channel_id, frm_ptr->frame_id);
			CMR_LOGI("free frame");
			goto exit;
		}
	}

	cxt->cur_frame_info = *frm_ptr;
	frame.frame_info = *frm_ptr;
	frame.y_virt_addr = cxt->chn_param.chn_frm[index].addr_vir.addr_y;
	frame.u_virt_addr = cxt->chn_param.chn_frm[index].addr_vir.addr_u;
	frame.width = cxt->chn_param.chn_frm[index].size.width;
	frame.height = cxt->chn_param.chn_frm[index].size.height;
	frame.timestamp = frm_ptr->sec * 1000000000LL + frm_ptr->usec * 1000;
	snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_TAKE_PICTURE, SNAPSHOT_EVT_CB_ZSL_NEW_FRM, (void*)&frame, sizeof(struct camera_cap_frm_info));
#endif
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

void snp_set_status(cmr_handle snp_handle, cmr_uint status)
{
	struct snp_context              *cxt = (struct snp_context*)snp_handle;
	struct camera_frame_type        frame_type = {0};

	CMR_LOGI("set %ld", status);
	sem_wait(&cxt->access_sm);
	cxt->status = status;
	sem_post(&cxt->access_sm);
	if (IDLE == status) {
		if (cxt->oem_cb) {
			cxt->oem_cb(cxt->oem_handle, SNAPSHOT_EVT_STATE, SNAPSHOT_FUNC_STATE, (void*)&status);
		}
	} else {
		frame_type.status = status;
		snp_send_msg_notify_thr(snp_handle, SNAPSHOT_FUNC_STATE, SNAPSHOT_EVT_STATE, (void*)&frame_type, sizeof(struct camera_frame_type));
	}
	CMR_LOGI("done");
}

cmr_uint snp_get_status(cmr_handle snp_handle)
{
	struct snp_context              *cxt = (struct snp_context*)snp_handle;
	cmr_uint                        status;

	CMR_LOGI("start");
	sem_wait(&cxt->access_sm);
	status = cxt->status;
	sem_post(&cxt->access_sm);
	CMR_LOGI("done %ld", status);
	return status;
}
/********************************* external data type *********************************/


cmr_int cmr_snapshot_init(struct snapshot_init_param *param_ptr, cmr_handle *snapshot_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct snp_context              *cxt = NULL;
	CMR_MSG_INIT(message);

	if (!param_ptr || !snapshot_handle || !param_ptr->ipm_handle
		|| !param_ptr->oem_handle || !param_ptr->oem_cb) {
		if (param_ptr) {
			CMR_LOGE("err param 0x%lx 0x%lx 0x%lx 0x%lx", (cmr_uint)param_ptr,
					(cmr_uint)snapshot_handle, (cmr_uint)param_ptr->ipm_handle,
					(cmr_uint)param_ptr->oem_handle);
		}
		ret = CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	*snapshot_handle = (cmr_handle)0;
	cxt = (struct snp_context*)malloc(sizeof(*cxt));
	if (NULL == cxt) {
		CMR_LOGE("failed to create snp context");
		ret = -CMR_CAMERA_NO_MEM;
		goto exit;
	}
	cmr_bzero(cxt, sizeof(*cxt));
	cxt->camera_id = param_ptr->id;
	cxt->oem_handle = param_ptr->oem_handle;
/*	cxt->ipm_handle = param_ptr->ipm_handle;*/
	cxt->oem_cb = param_ptr->oem_cb;
	cxt->ops = param_ptr->ops;
	cxt->caller_privdata = param_ptr->private_data;
	cxt->req_param.channel_id = SNP_INVALIDE_VALUE;
	snp_local_init((cmr_handle)cxt);
	ret = snp_create_thread((cmr_handle)cxt);
	if (ret) {
		CMR_LOGE("failed to create thread %ld", ret);
	}
exit:
	CMR_LOGI("done %ld", ret);
	if (CMR_CAMERA_SUCCESS == ret) {
		*snapshot_handle = (cmr_handle)cxt;
		cxt->is_inited = 1;
	} else {
		if (cxt) {
			free((void*)cxt);
		}
	}
	return ret;
}

cmr_int cmr_snapshot_deinit(cmr_handle snapshot_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct snp_context              *cxt = (struct snp_context*)snapshot_handle;
	CMR_MSG_INIT(message);

	CHECK_HANDLE_VALID(snapshot_handle);

	if (0 == cxt->is_inited) {
		CMR_LOGI("has been de-initialized");
		goto exit;
	}
	if (IDLE != snp_get_status(snapshot_handle)) {
		snp_set_request(snapshot_handle, TAKE_PICTURE_NO);
		message.msg_type = SNP_EVT_STOP;
		message.sync_flag = CMR_MSG_SYNC_PROCESSED;
		message.alloc_flag = 0;
		ret = cmr_thread_msg_send(cxt->thread_cxt.main_thr_handle, &message);
		if (ret) {
			CMR_LOGE("failed to send stop msg to main thr %ld", ret);
		}
	}
	ret = snp_destroy_thread(snapshot_handle);
	if (ret) {
		CMR_LOGE("failed to destroy thr %ld", ret);
	}
	snp_local_deinit(snapshot_handle);
	free((void*)snapshot_handle);
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int cmr_snapshot_post_proc(cmr_handle snapshot_handle, struct snapshot_param *param_ptr)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct snp_context              *cxt = (struct snp_context*)snapshot_handle;
	CMR_MSG_INIT(message);

	if (!snapshot_handle || !param_ptr) {
		CMR_LOGE("error param 0x%lx 0x%lx", (cmr_uint)snapshot_handle, (cmr_uint)param_ptr);
		goto exit;
	}
	if (TAKE_PICTURE_NEEDED == snp_get_request((cmr_handle)cxt)) {
		CMR_LOGE("post proc is going.try once more.");
		usleep(60*1000);
		if (TAKE_PICTURE_NEEDED == snp_get_request((cmr_handle)cxt)) {
			CMR_LOGE("post proc is going again.");
			ret = -CMR_CAMERA_FAIL;
			goto exit;
		}
	}
	ret = snp_check_post_proc_param(param_ptr);
	if (ret) {
		goto exit;
	}
	CMR_LOGI("chn id %d android zsl %d rotation snp %d, cfg rot cap %d hdr %d snp mode angle %d total num %d thumb size %d %d",
			param_ptr->channel_id, param_ptr->is_android_zsl, param_ptr->is_cfg_rot_cap,
			param_ptr->is_hdr, param_ptr->mode, param_ptr->rot_angle, param_ptr->total_num,
			param_ptr->jpeg_setting.thum_size.width, param_ptr->jpeg_setting.thum_size.height);
	message.msg_type = SNP_EVT_START_PROC;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.alloc_flag = 0;
	message.data = param_ptr;
	ret = cmr_thread_msg_send(cxt->thread_cxt.main_thr_handle, &message);
	if (ret) {
		CMR_LOGE("failed to send msg to main thr %ld", ret);
		goto exit;
	}
	ret = cxt->err_code;
exit:
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int cmr_snapshot_receive_data(cmr_handle snapshot_handle, cmr_int evt, void* data)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct snp_context              *cxt = (struct snp_context*)snapshot_handle;
	struct snp_proc_param           *proc_param_ptr = &cxt->req_param.post_proc_setting;
	cmr_u32                         malloc_len = 0;
	cmr_handle                      send_thr_handle;
	cmr_int                         snp_evt;
	cmr_u32                         flag = 0;
	struct frm_info                 chn_data;
	unsigned long		src_vir, dst_vir;
	CMR_MSG_INIT(message);

	CHECK_HANDLE_VALID(snapshot_handle);
	CMR_LOGI("evt %ld", evt);

	send_thr_handle = cxt->thread_cxt.post_proc_thr_handle;

	if ((evt >= SNPASHOT_EVT_MAX) || !data) {
		CMR_LOGE("err param %ld %p", evt, data);
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	switch (evt) {
	case SNAPSHOT_EVT_CHANNEL_DONE:
		malloc_len = sizeof(struct frm_info);
		CMR_LOGI("hdr %d, video %d zsl %d", cxt->req_param.is_hdr, cxt->req_param.is_video_snapshot, cxt->req_param.is_zsl_snapshot);
		if (1 == cxt->req_param.is_hdr) {
			snp_evt = SNP_EVT_HDR_SRC_DONE;
		} else {
			snp_evt = SNP_EVT_CHANNEL_DONE;
			if (1 == cxt->req_param.is_video_snapshot || 1 == cxt->req_param.is_zsl_snapshot) {
				flag = 1;
				cmr_u32 width = cxt->req_param.post_proc_setting.chn_out_frm[0].size.width;
				cmr_u32 height = cxt->req_param.post_proc_setting.chn_out_frm[0].size.height;
				cmr_u32 act_width = cxt->req_param.post_proc_setting.actual_snp_size.width;
				cmr_u32 act_height = cxt->req_param.post_proc_setting.actual_snp_size.height;

				memcpy(&chn_data, data, sizeof(struct frm_info));
				chn_data.base       = CMR_CAP0_ID_BASE;
				chn_data.frame_id   = CMR_CAP0_ID_BASE;
				if (1 == cxt->req_param.is_zsl_snapshot) {
					chn_data.base       = CMR_CAP1_ID_BASE;
					chn_data.frame_id   = CMR_CAP1_ID_BASE;
				}
				chn_data.yaddr      = cxt->req_param.post_proc_setting.chn_out_frm[0].addr_phy.addr_y;
				chn_data.uaddr      = cxt->req_param.post_proc_setting.chn_out_frm[0].addr_phy.addr_u;

				src_vir = (unsigned long)chn_data.yaddr_vir;
				dst_vir = (unsigned long)cxt->req_param.post_proc_setting.chn_out_frm[0].addr_vir.addr_y;
				CMR_LOGI("y src_vir = %lx dst_vir = %lx width %d height %d channel_zoom_mode %ld",
					src_vir, dst_vir, width, height, proc_param_ptr->channel_zoom_mode);
				cmr_copy((void *)dst_vir, (void *)src_vir, width * height);

				if (ZOOM_POST_PROCESS == proc_param_ptr->channel_zoom_mode) {
					src_vir = (unsigned long)chn_data.yaddr_vir + width * height;
				} else {
					src_vir = (unsigned long)chn_data.yaddr_vir + act_width * act_height;
				}

				dst_vir = (unsigned long)cxt->req_param.post_proc_setting.chn_out_frm[0].addr_vir.addr_u;
				CMR_LOGI("uv src_vir = %lx dst_vir = %lx width %d height %d", src_vir, dst_vir, width, height);

				cmr_copy((void *)dst_vir, (void *)src_vir, width * height / 2);
				cmr_snapshot_memory_flush(cxt);
			}
		}
		break;
	case SNAPSHOT_EVT_RAW_PROC:
		snp_evt = SNP_EVT_RAW_PROC;
		malloc_len = sizeof(struct ips_out_param);
		send_thr_handle = cxt->thread_cxt.proc_cb_thr_handle;
		break;
	case SNAPSHOT_EVT_CVT_RAW_DATA:
		snp_evt = SNP_EVT_CVT_RAW_DATA;
		malloc_len = sizeof(struct ips_out_param);
		send_thr_handle = cxt->thread_cxt.proc_cb_thr_handle;
		break;
	case SNAPSHOT_EVT_SC_DONE:
		snp_evt = SNP_EVT_SC_DONE;
		malloc_len = sizeof(struct img_frm);
		send_thr_handle = cxt->thread_cxt.proc_cb_thr_handle;
		break;
	case SNAPSHOT_EVT_HDR_DONE:
		snp_evt = SNP_EVT_HDR_DONE;
		malloc_len = sizeof(struct img_frm);
		send_thr_handle = cxt->thread_cxt.proc_cb_thr_handle;
		break;
	case SNAPSHOT_EVT_JPEG_ENC_DONE:
		{
		struct jpeg_enc_cb_param test_param = *(struct jpeg_enc_cb_param*)data;
		snp_evt = SNP_EVT_JPEG_ENC_DONE;
		malloc_len = sizeof(struct jpeg_enc_cb_param);
		send_thr_handle = cxt->thread_cxt.proc_cb_thr_handle;
		}
		break;
	case SNAPSHOT_EVT_JPEG_DEC_DONE:
		snp_evt = SNP_EVT_JPEG_DEC_DONE;
		malloc_len = sizeof(struct jpeg_dec_cb_param);
		send_thr_handle = cxt->thread_cxt.proc_cb_thr_handle;
		break;
	case SNAPSHOT_EVT_JPEG_ENC_ERR:
		snp_evt = SNP_EVT_JPEG_ENC_ERR;
		send_thr_handle = cxt->thread_cxt.proc_cb_thr_handle;
		break;
	case SNAPSHOT_EVT_JPEG_DEC_ERR:
		snp_evt = SNP_EVT_JPEG_DEC_ERR;
		send_thr_handle = cxt->thread_cxt.proc_cb_thr_handle;
		break;
	case SNAPSHOT_EVT_ANDROID_ZSL_DATA:
		malloc_len = sizeof(struct frm_info);
		snp_evt = SNP_EVT_ANDROID_ZSL_DATA;
		break;
	case SNAPSHOT_EVT_FREE_FRM:
		malloc_len = sizeof(struct frm_info);
		snp_evt = SNP_EVT_FREE_FRM;
		break;
	default:
		CMR_LOGI("don't support evt 0x%lx", evt);
		goto exit;
	}
	if (malloc_len) {
		message.data = malloc(malloc_len);
		if (!message.data) {
			CMR_LOGE("failed to malloc");
			ret = -CMR_CAMERA_NO_MEM;
			goto exit;
		}
		if (1 == flag) {
			cmr_copy(message.data, &chn_data, malloc_len);
		} else {
			cmr_copy(message.data, data, malloc_len);
		}
		message.alloc_flag = 1;
	} else {
		message.alloc_flag = 0;
	}
	message.msg_type = snp_evt;
	message.sync_flag = CMR_MSG_SYNC_NONE;
	ret = cmr_thread_msg_send(send_thr_handle, &message);
	if (ret) {
		CMR_LOGE("failed to send stop msg to main thr %ld", ret);
	}
exit:
	return ret;
}

cmr_int cmr_snapshot_stop(cmr_handle snapshot_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct snp_context              *cxt = (struct snp_context*)snapshot_handle;
	CMR_MSG_INIT(message);

	CHECK_HANDLE_VALID(snapshot_handle);

	snp_set_request(snapshot_handle, TAKE_PICTURE_NO);
	message.msg_type = SNP_EVT_STOP;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.alloc_flag = 0;
	ret = cmr_thread_msg_send(cxt->thread_cxt.main_thr_handle, &message);
	if (ret) {
		CMR_LOGE("failed to send stop msg to main thr %ld", ret);
	}
	return ret;
}

cmr_int cmr_snapshot_release_frame(cmr_handle snapshot_handle, cmr_uint index)
{
	UNUSED(snapshot_handle);
	UNUSED(index);

	cmr_int                         ret = CMR_CAMERA_SUCCESS;
#if 0
	cmr_u32                         buf_base;
	struct snp_context              *cxt = (struct snp_context*)snapshot_handle;

	if (cxt->ops.channel_free_frame) {
		buf_base = cxt->cur_frame_info.base;
		index += buf_base;
		ret = cxt->ops.channel_free_frame(cxt->oem_handle, cxt->req_param.channel_id, index);
	} else {
		CMR_LOGE("err,channel_free_frame is null");
	}
#endif
	return ret;
}

cmr_int cmr_snapshot_format_convert(cmr_handle snapshot_handle, void *data, struct img_frm *out_ptr)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct snp_context              *cxt = (struct snp_context*)snapshot_handle;
	struct frm_info                 *frame_info_ptr = (struct frm_info*)data;
	CMR_MSG_INIT(message);

	if (!snapshot_handle || !data || !out_ptr) {
		CMR_LOGE("err param 0x%lx 0x%lx 0x%lx", (cmr_uint)snapshot_handle, (cmr_uint)data, (cmr_uint)out_ptr);
		ret = - CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}
	if (!IS_CAP_FRM(frame_info_ptr->frame_id, frame_info_ptr->base)) {
		CMR_LOGE("error frame index 0x%x", frame_info_ptr->frame_id);
		ret = -CMR_CAMERA_INVALID_FRAME;
		goto exit;
	}
	if (frame_info_ptr->channel_id != cxt->req_param.channel_id) {
		CMR_LOGE("error channel data %d %d", frame_info_ptr->channel_id, cxt->req_param.channel_id);
		ret = -CMR_CAMERA_INVALID_FRAME;
		goto exit;
	}
	message.msg_type = SNP_EVT_START_CVT;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.alloc_flag = 0;
	message.sub_msg_type = NON_SNP_TRIGGER;
	message.data = data;
	ret = cmr_thread_msg_send(cxt->thread_cxt.cvt_thr_handle, &message);
	if (ret) {
		CMR_LOGE("failed to send stop msg to main thr %ld", ret);
	} else {
		snp_set_status(snapshot_handle, POST_PROCESSING);
	}
	*out_ptr = cxt->cvt.out_frame;
exit:
	return ret;
}

cmr_int cmr_snapshot_memory_flush(cmr_handle snapshot_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct snp_context              *cxt = (struct snp_context*)snapshot_handle;

	CHECK_HANDLE_VALID(snapshot_handle);

	ret = snp_send_msg_notify_thr(snapshot_handle, SNAPSHOT_FUNC_TAKE_PICTURE, SNAPSHOT_EVT_CB_FLUSH, NULL, 0);
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int camera_open_uvde(struct camera_context *cxt, struct ipm_open_in *in_ptr, struct ipm_open_out *out_ptr)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	if ((NULL == cxt) || (NULL == cxt->ipm_cxt.ipm_handle)) {
		CMR_LOGE("param invalid");
		return CMR_CAMERA_INVALID_PARAM;
	}
	ret = cmr_ipm_open(cxt->ipm_cxt.ipm_handle, IPM_TYPE_UVDE, in_ptr, out_ptr, &cxt->ipm_cxt.uvde_handle);
	return ret;
}

cmr_int camera_close_uvde(struct camera_context *cxt)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;

	if (cxt->ipm_cxt.uvde_handle) {
		ret = cmr_ipm_close(cxt->ipm_cxt.uvde_handle);
		cxt->ipm_cxt.uvde_handle = 0;
	}
	return ret;
}

cmr_int camera_start_uvde(struct camera_context *cxt, struct img_frm *src)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct ipm_frame_in           ipm_in_param;

	CMR_LOGI("uvdenoise start");
	if ((NULL == cxt) || (NULL == src)) {
		CMR_LOGE("param invalid");
		return CMR_CAMERA_INVALID_PARAM;
	}
	camera_take_snapshot_step(CMR_STEP_UVDENOISE_S);
	ret = camera_open_uvde(cxt, NULL, NULL);
	if (ret) {
		CMR_LOGE("failed to open uvdenoise %ld", ret);
		return ret;
	}

	ipm_in_param.src_frame = *src;
	ret = ipm_transfer_frame(cxt->ipm_cxt.uvde_handle, &ipm_in_param, NULL);
	if (ret) {
		CMR_LOGE("failed to transfer frame to ipm %ld", ret);
		return ret;
	}

	ret = camera_close_uvde(cxt);
	if (ret) {
		CMR_LOGE("fail to close uvdenoise %ld", ret);
		return ret;
	}
	camera_take_snapshot_step(CMR_STEP_UVDENOISE_E);
	CMR_LOGI("uvdenoise end");
	return ret;
}

cmr_int camera_open_yde(struct camera_context *cxt, struct ipm_open_in *in_ptr, struct ipm_open_out *out_ptr)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	if ((NULL == cxt) || (NULL == cxt->ipm_cxt.ipm_handle)) {
		CMR_LOGE("param invalid");
		return CMR_CAMERA_INVALID_PARAM;
	}
	ret = cmr_ipm_open(cxt->ipm_cxt.ipm_handle, IPM_TYPE_YDE, in_ptr, out_ptr, &cxt->ipm_cxt.yde_handle);

	return ret;
}

cmr_int camera_close_yde(struct camera_context *cxt)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;

	if (cxt->ipm_cxt.yde_handle) {
		ret = cmr_ipm_close(cxt->ipm_cxt.yde_handle);
		cxt->ipm_cxt.yde_handle = 0;
	}
	return ret;
}

static cmr_int camera_start_yde(struct camera_context *cxt, struct img_frm *src)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct ipm_frame_in           ipm_in_param;

	CMR_LOGI("ydenoise start");
	if ((NULL == cxt) || (NULL == src)) {
		CMR_LOGE("param invalid");
		return CMR_CAMERA_INVALID_PARAM;
	}
	camera_take_snapshot_step(CMR_STEP_YDENOISE_S);
	ret = camera_open_yde(cxt, NULL, NULL);
	if (ret) {
		CMR_LOGE("failed to open ydenoise %ld", ret);
		return ret;
	}

	ipm_in_param.src_frame = *src;
	ret = ipm_transfer_frame(cxt->ipm_cxt.yde_handle, &ipm_in_param, NULL);
	if (ret) {
		CMR_LOGE("failed to transfer frame to ipm %ld", ret);
		return ret;
	}

	ret = camera_close_yde(cxt);
	if (ret) {
		CMR_LOGE("fail to close ydenoise %ld", ret);
		return ret;
	}
	camera_take_snapshot_step(CMR_STEP_YDENOISE_E);
	CMR_LOGI("ydenoise end");
	return ret;
}

cmr_int camera_open_snr_uvde(struct camera_context *cxt, struct ipm_open_in *in_ptr, struct ipm_open_out *out_ptr)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	if ((NULL == cxt) || (NULL == cxt->ipm_cxt.ipm_handle)) {
		CMR_LOGE("param invalid");
		return CMR_CAMERA_INVALID_PARAM;
	}
	ret = cmr_ipm_open(cxt->ipm_cxt.ipm_handle, IPM_TYPE_SNR_UVDE, in_ptr, out_ptr, &cxt->ipm_cxt.snr_uvde_handle);

	return ret;
}

cmr_int camera_close_snr_uvde(struct camera_context *cxt)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;

	if (cxt->ipm_cxt.snr_uvde_handle) {
		ret = cmr_ipm_close(cxt->ipm_cxt.snr_uvde_handle);
		cxt->ipm_cxt.snr_uvde_handle = 0;
	}
	return ret;
}


static cmr_int camera_start_snr_uv(struct camera_context *cxt, struct img_frm *src)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;
	struct ipm_frame_in           ipm_in_param;

	CMR_LOGI("snr_uvdenoise start");
	if ((NULL == cxt) || (NULL == src)) {
		CMR_LOGE("param invalid");
		return CMR_CAMERA_INVALID_PARAM;
	}
	camera_take_snapshot_step(CMR_STEP_SNR_UVDENOISE_S);
	ret = camera_open_snr_uvde(cxt, NULL, NULL);
	if (ret) {
		CMR_LOGE("failed to open ydenoise %ld", ret);
		return ret;
	}

	ipm_in_param.src_frame = *src;
	ret = ipm_transfer_frame(cxt->ipm_cxt.snr_uvde_handle, &ipm_in_param, NULL);
	if (ret) {
		CMR_LOGE("failed to transfer frame to ipm %ld", ret);
		return ret;
	}

	ret = camera_close_snr_uvde(cxt);
	if (ret) {
		CMR_LOGE("fail to close ydenoise %ld", ret);
		return ret;
	}
	camera_take_snapshot_step(CMR_STEP_SNR_UVDENOISE_E);
	CMR_LOGI("snr_uvdenoise end");
	return ret;
}

