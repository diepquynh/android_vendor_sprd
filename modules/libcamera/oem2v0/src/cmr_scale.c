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

#define LOG_TAG "Cmr_scale"

#include <fcntl.h>
#include <sys/ioctl.h>
#include "cmr_type.h"
#include "cmr_msg.h"
#include "cmr_cvt.h"
#include "sprd_scale_k.h"
#ifdef CONFIG_MEM_OPTIMIZATION
#include "../hwcomposer/sc8830/gsp_hal.h"
#include <sys/ioctl.h>
#endif


#define CMR_EVT_SCALE_INIT              (CMR_EVT_OEM_BASE + 16)
#define CMR_EVT_SCALE_START             (CMR_EVT_OEM_BASE + 17)
#define CMR_EVT_SCALE_EXIT              (CMR_EVT_OEM_BASE + 18)

#define SCALE_MSG_QUEUE_SIZE 20
#define SCALE_RESTART_SUM    2

enum scale_work_mode {
	SC_FRAME = SCALE_MODE_NORMAL,
	SC_SLICE_EXTERNAL = SCALE_MODE_SLICE,
	SC_SLICE_INTERNAL
};

struct scale_file {
	cmr_int                         handle;
	cmr_uint                        is_inited;
	cmr_int                         err_code;
	cmr_int                         sync_none_err_code;
	cmr_handle                      scale_thread;
	sem_t                           sync_sem;
	pthread_mutex_t                 scale_mutex;
};

struct scale_cfg_param_t{
	struct scale_frame_param_t      frame_params;
	cmr_evt_cb                      scale_cb;
	cmr_handle                      cb_handle;
};

static cmr_s8 scaler_dev_name[50] = "/dev/sprd_scale";

static cmr_int cmr_scale_restart(struct scale_file *file);

static enum scale_fmt_e cmr_scale_fmt_cvt(cmr_u32 cmt_fmt)
{
	enum scale_fmt_e sc_fmt = SCALE_FTM_MAX;

	switch (cmt_fmt) {
	case IMG_DATA_TYPE_YUV422:
		sc_fmt = SCALE_YUV422;
		break;

	case IMG_DATA_TYPE_YUV420:
	case IMG_DATA_TYPE_YVU420:
		sc_fmt = SCALE_YUV420;
		break;

	case IMG_DATA_TYPE_RGB565:
		sc_fmt = SCALE_RGB565;
		break;

	case IMG_DATA_TYPE_RGB888:
		sc_fmt = SCALE_RGB888;
		break;

	default:
		CMR_LOGE("scale format error");
		break;
	}

	return sc_fmt;
}

int32_t cmr_scaling_down(struct img_frm *src, struct img_frm *dst)
{
	cmr_uint *dst_y_buf;
	cmr_uint *dst_uv_buf;
	cmr_uint *src_y_buf;
	cmr_uint *src_uv_buf;
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
	dst_y_buf     = &dst->addr_vir.addr_y;
	dst_uv_buf    = &dst->addr_vir.addr_u;
	src_y_buf     = &src->addr_vir.addr_y;
	src_uv_buf    = &src->addr_vir.addr_u;
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
static cmr_int cmr_scale_sw_start(struct scale_cfg_param_t *cfg_params, struct scale_file *file)
{
	cmr_int               ret = CMR_CAMERA_SUCCESS;
	struct img_frm        src;
	struct img_frm        dst;
	struct img_frm        frame;
	struct scale_frame_param_t *frame_params = NULL;
	if (!cfg_params || !file) {
		CMR_LOGE("scale erro: cfg_params is null");
		return CMR_CAMERA_INVALID_PARAM;
	}
	frame_params = &cfg_params->frame_params;
	if (!frame_params) {
		CMR_LOGE("scale erro: frame_params is null");
		return CMR_CAMERA_INVALID_PARAM;
	}
	if (frame_params->input_size.w >= frame_params->output_size.w && frame_params->input_size.h >= frame_params->output_size.h) {
		src.addr_vir.addr_y  = frame_params->input_addr.yaddr;
		src.addr_vir.addr_u  = frame_params->input_addr.uaddr;
		src.addr_vir.addr_v  = frame_params->input_addr.vaddr;
		src.size.width       = frame_params->input_size.w;
		src.size.height      = frame_params->input_size.h;
		dst.addr_vir.addr_y  = frame_params->output_addr.yaddr;
		dst.addr_vir.addr_u  = frame_params->output_addr.uaddr;
		dst.addr_vir.addr_v  = frame_params->output_addr.vaddr;
		dst.size.width       = frame_params->output_size.w;
		dst.size.height      = frame_params->output_size.h;
		ret = cmr_scaling_down(&src, &dst);
		if (ret) {
			goto exit;
		}
		if (cfg_params->scale_cb) {
			sem_post(&file->sync_sem);
		}
		if (cfg_params->scale_cb) {
			memset((void *)&frame, 0x00, sizeof(frame));
			frame.size.width = frame_params->output_size.w;
			frame.size.height = frame_params->output_size.h;
			frame.addr_phy.addr_y = (cmr_uint)frame_params->output_addr.yaddr;
			frame.addr_phy.addr_u = (cmr_uint)frame_params->output_addr.uaddr;
			frame.addr_phy.addr_v = (cmr_uint)frame_params->output_addr.vaddr;
			(*cfg_params->scale_cb)(CMR_IMG_CVT_SC_DONE, &frame, cfg_params->cb_handle);
		}
} else {
		ret = CMR_CAMERA_FAIL;
	}
exit:
	CMR_LOGI("done ret %d", ret);
	return ret;
}
static cmr_int cmr_scale_thread_proc(struct cmr_msg *message, void *private_data)
{
	cmr_int               ret = CMR_CAMERA_SUCCESS;
	cmr_u32               evt = 0;
	cmr_int               restart_cnt = 0;
	struct scale_file     *file = (struct scale_file *)private_data;

	if (!file) {
		CMR_LOGE("scale erro: file is null");
		return CMR_CAMERA_INVALID_PARAM;
	}

	if (-1 == file->handle) {
		CMR_LOGE("scale error: handle is invalid");
		return CMR_CAMERA_INVALID_PARAM;
	}

	CMR_LOGV("scale message.msg_type 0x%x, data %p", message->msg_type, message->data);

	evt = (cmr_u32)message->msg_type;

	switch (evt) {
	case CMR_EVT_SCALE_INIT:
		CMR_LOGI("scale init");
		break;

	case CMR_EVT_SCALE_START:
		CMR_LOGI("scale start");
		struct img_frm frame;

		struct scale_cfg_param_t *cfg_params = (struct scale_cfg_param_t *)message->data;
		struct scale_frame_param_t *frame_params = &cfg_params->frame_params;

		while ((restart_cnt < SCALE_RESTART_SUM) && (CMR_CAMERA_SUCCESS == file->err_code)) {
			file->err_code = CMR_CAMERA_SUCCESS;
			ret = ioctl(file->handle, SCALE_IO_START, frame_params);
			if (ret) {
				if (!(ret = cmr_scale_sw_start(cfg_params, file))) {
					break;
				}
				CMR_PERROR;
				CMR_LOGE("scale error: start");
			}
			CMR_LOGI("scale started");

			if (CMR_CAMERA_SUCCESS != ret) {
				file->err_code = CMR_CAMERA_INVALID_PARAM;
			}
			if (cfg_params->scale_cb) {
				if (file->err_code == CMR_CAMERA_INVALID_PARAM)
					file->sync_none_err_code = CMR_CAMERA_INVALID_PARAM;
				else
					file->sync_none_err_code = CMR_CAMERA_SUCCESS;
				sem_post(&file->sync_sem);
			}
			if (CMR_CAMERA_SUCCESS == ret) {
				ret = ioctl(file->handle, SCALE_IO_DONE, frame_params);
				if (ret) {
					CMR_LOGE("scale done error");
					ret = cmr_scale_restart(file);
					if (ret) {
						file->err_code = CMR_CAMERA_FAIL;
					} else {
						restart_cnt++;
				}
				}

				if (CMR_CAMERA_SUCCESS == file->err_code) {
					if (cfg_params->scale_cb) {
						memset((void *)&frame, 0x00, sizeof(frame));
						if (CMR_CAMERA_SUCCESS == ret) {
							frame.size.width = frame_params->output_size.w;
							frame.size.height = frame_params->output_size.h;
							frame.addr_phy.addr_y = (cmr_uint)frame_params->output_addr.yaddr;
							frame.addr_phy.addr_u = (cmr_uint)frame_params->output_addr.uaddr;
							frame.addr_phy.addr_v = (cmr_uint)frame_params->output_addr.vaddr;
						}
						(*cfg_params->scale_cb)(CMR_IMG_CVT_SC_DONE, &frame, cfg_params->cb_handle);
					}
					break;
				}
			}
		}
		restart_cnt = 0;
		break;

	case CMR_EVT_SCALE_EXIT:
		CMR_LOGI("scale exit");
		break;

	default:
		break;
	}

	CMR_LOGI("scale thread: Out");

	return ret;
}

static cmr_int cmr_scale_create_thread(struct scale_file *file)
{
	cmr_int                 ret = CMR_CAMERA_SUCCESS;
	CMR_MSG_INIT(message);

	if (!file) {
		CMR_LOGE("scale error: file is null");
		ret = CMR_CAMERA_FAIL;
		goto out;
	}

	if(!file->is_inited) {
		ret = cmr_thread_create(&file->scale_thread,
					SCALE_MSG_QUEUE_SIZE,
					cmr_scale_thread_proc,
					(void*)file);
		if (ret) {
			CMR_LOGE("create thread failed!");
			ret = CMR_CAMERA_FAIL;
			goto out;
		}

		file->is_inited = 1;

	}

out:
	return ret;

}

static cmr_int cmr_scale_destory_thread(struct scale_file *file)
{
	cmr_int                ret = CMR_CAMERA_SUCCESS;
	CMR_MSG_INIT(message);

	if (!file) {
		CMR_LOGE("scale error: file is null");
		return CMR_CAMERA_FAIL;
	}

	if (file->is_inited) {
		ret = cmr_thread_destroy(file->scale_thread);
		file->scale_thread= 0;

		file->is_inited = 0;
	}

	return ret;
}

static cmr_int cmr_scale_restart(struct scale_file *file)
{
	cmr_int                 ret = CMR_CAMERA_SUCCESS;
	cmr_int                 fd = -1;
	cmr_int                 time_out = 3;

	if (!file) {
		ret = CMR_CAMERA_INVALID_PARAM;
		CMR_LOGE("param error");
		return ret;
	}

	if (-1 != file->handle) {
		if (-1 == close(file->handle)) {
			CMR_LOGE("scale error: close");
		}
	} else {
		CMR_LOGE("scale error: handle is invalid");
		ret = CMR_CAMERA_FAIL;
		goto exit;
	}

	for ( ; time_out > 0; time_out--) {
		fd = open(scaler_dev_name, O_RDWR, 0);

		if (-1 == fd) {
			CMR_LOGI("scale sleep 50ms");
			usleep(50*1000);
		} else {
			break;
		}
	};
	if (-1 == fd) {
		CMR_LOGE("failed to open scale dev");
		ret = CMR_CAMERA_FAIL;
		goto exit;
	}
exit:
	file->handle = fd;
	CMR_LOGI("done %ld", ret);
	return ret;
}

cmr_int cmr_scale_open(cmr_handle *scale_handle)
{
	cmr_int                 ret = CMR_CAMERA_SUCCESS;
	cmr_int                 fd = -1;
	cmr_int                 time_out = 3;
	struct scale_file       *file = NULL;

	file = (struct scale_file*)calloc(1, sizeof(struct scale_file));
	if(!file) {
		CMR_LOGE("scale error: no memory for file");
		ret = CMR_CAMERA_NO_MEM;
		goto exit;
	}

	for ( ; time_out > 0; time_out--) {
		fd = open(scaler_dev_name, O_RDWR, 0);

		if (-1 == fd) {
			CMR_LOGI("scale sleep 50ms");
			usleep(50*1000);
		} else {
			break;
		}
	};

	if (0 == time_out) {
		CMR_LOGE("scale error: open device");
		goto free_file;
	}

	file->handle = fd;

	ret = cmr_scale_create_thread(file);
	if (ret) {
		CMR_LOGE("scale error: create thread");
		goto free_cb;
	}
	sem_init(&file->sync_sem, 0, 0);
	pthread_mutex_init(&file->scale_mutex, NULL);

	*scale_handle = (cmr_handle)file;

	goto exit;

free_cb:
	close(fd);
free_file:
	if(file)
		free(file);
	file = NULL;
exit:

	return ret;
}


#ifdef CONFIG_MEM_OPTIMIZATION
static cmr_int need_notify_close_gsp_hwc=1;

cmr_int cmr_notify_close_gsp_hwc(cmr_int need_close)
{
	GSP_CONFIG_INFO_T gsp_cfg_info;
	cmr_int	gsp_fd = 0;
	cmr_int	ret = 0;

	ALOGE("begin to notify GSP HWC, close=%d, flag=%d", need_close, need_notify_close_gsp_hwc);

	if((need_notify_close_gsp_hwc==0 && need_close==1) || (need_notify_close_gsp_hwc==1 && need_close==0))
		return 0;
	memset(&gsp_cfg_info, 0, sizeof(GSP_CONFIG_INFO_T));

	gsp_fd = open("/dev/sprd_gsp", O_RDWR, 0);
	if (-1 == gsp_fd) {
		ALOGE("open gsp device failed! Line:%d \n", __LINE__);
		return -1;
	}
	if(need_close)
		gsp_cfg_info.layer0_info.endian_mode.va_lng_wrd_endn = 11;
	else
		gsp_cfg_info.layer0_info.endian_mode.va_lng_wrd_endn = 12;
	ret = ioctl(gsp_fd, GSP_IO_SET_PARAM, &gsp_cfg_info);

	if (ret != 0) {
		CMR_LOGE("gsp_trigger fail");
		close(gsp_fd);
		return -1;
	}
	close(gsp_fd);
#if 1
	if(need_close)
		need_notify_close_gsp_hwc=0;
	else
		need_notify_close_gsp_hwc=1;
#endif

	return 0;
}

cmr_int cmr_gsp_start_scale(struct img_frm *src, struct img_frm *dst, int src_fd, int dst_fd)
{
	FILE *gsp_in_fp;
	FILE *gsp_out_fp;
	cmr_int	ret = 0;
	GSP_CONFIG_INFO_T gsp_cfg_info;
	hw_module_t const* pModule;
	gsp_device_t *mGspDev;
#if 0
	if(need_notify_close_gsp_hwc){
		if(cmr_notify_close_gsp_hwc(1))
		return -1;
	}
#endif
	if (hw_get_module(GSP_HARDWARE_MODULE_ID, &pModule) == 0) {
		pModule->methods->open(pModule, "gsp", (hw_device_t**)(&mGspDev));
		if (mGspDev == NULL) {
			CMR_LOGE("hwcomposer open GSP lib failed! ");
			return -1;
		}
	} else {
		CMR_LOGE("hwcomposer can't find GSP lib ! ");
		return -1;
	}
	memset(&gsp_cfg_info, 0, sizeof(GSP_CONFIG_INFO_T));
	gsp_cfg_info.layer0_info.layer_en = 1;
	gsp_cfg_info.layer1_info.layer_en = 0;
	if ( (IMG_DATA_TYPE_YUV420 == src->fmt) && (IMG_DATA_TYPE_YUV420 == dst->fmt) ) {
		gsp_cfg_info.layer0_info.img_format = GSP_SRC_FMT_ARGB888;
		gsp_cfg_info.layer0_info.pitch = src->size.width;
		if ((src->rect.width != dst->size.width) ||	(src->rect.height != dst->size.height) )
			gsp_cfg_info.layer0_info.scaling_en = 1;
		//gsp_cfg_info.layer0_info.src_addr.addr_y = src->addr_phy.addr_y;
		//gsp_cfg_info.layer0_info.src_addr.addr_uv =
		//gsp_cfg_info.layer0_info.src_addr.addr_v = src->addr_phy.addr_u;
			gsp_cfg_info.layer0_info.clip_rect.rect_w = src->rect.width;
			gsp_cfg_info.layer0_info.clip_rect.rect_h = src->rect.height;
			gsp_cfg_info.layer0_info.clip_rect.st_x = src->rect.start_x;
			gsp_cfg_info.layer0_info.clip_rect.st_y = src->rect.start_y;
			gsp_cfg_info.layer0_info.des_rect.rect_w = dst->rect.width;;
			gsp_cfg_info.layer0_info.des_rect.rect_h = dst->rect.height;
			gsp_cfg_info.layer0_info.endian_mode.y_word_endn = 1;
			gsp_cfg_info.layer0_info.endian_mode.uv_word_endn = 2;
			//gsp_cfg_info.layer0_info.alpha = 0xff;
			//gsp_cfg_info.layer0_info.des_rect.st_x  need to be done
			//gsp_cfg_info.layer0_info.des_rect.st_y  need to be done
			gsp_cfg_info.layer0_info.mem_info.is_pa = 0;
			gsp_cfg_info.layer0_info.mem_info.share_fd = src_fd;
			gsp_cfg_info.layer0_info.mem_info.uv_offset = src->size.width * src->size.height;
			gsp_cfg_info.layer_des_info.img_format = GSP_SRC_FMT_ARGB888;
			//gsp_cfg_info.layer_des_info.src_addr.addr_y = dst->addr_phy.addr_y;
			//gsp_cfg_info.layer_des_info.src_addr.addr_uv =
			//gsp_cfg_info.layer_des_info.src_addr.addr_v = dst->addr_phy.addr_u;
			gsp_cfg_info.layer_des_info.pitch = dst->size.width;
			gsp_cfg_info.layer_des_info.mem_info.uv_offset = dst->size.width * dst->size.height;
			gsp_cfg_info.layer_des_info.mem_info.share_fd = dst_fd;
			gsp_cfg_info.layer_des_info.endian_mode.y_word_endn = 1;
			gsp_cfg_info.layer_des_info.endian_mode.uv_word_endn = 2;
			gsp_cfg_info.layer_des_info.mem_info.is_pa = 0;
			//gsp_cfg_info.misc_info.y2r_opt = 1;
		}
	ret = mGspDev->GSP_Proccess(&gsp_cfg_info);
	if (ret) {
		CMR_LOGE("gsp_trigger fail");
		return -1;
	}
	return 0;
}
#endif

cmr_int cmr_scale_start(cmr_handle scale_handle, struct img_frm *src_img,
			struct img_frm *dst_img, cmr_evt_cb cmr_event_cb, cmr_handle cb_handle)
{
	cmr_int                         ret = CMR_CAMERA_SUCCESS;

	struct scale_cfg_param_t        *cfg_params = NULL;
	struct scale_frame_param_t      *frame_params = NULL;
	struct scale_file               *file = (struct scale_file*)(scale_handle);

	CMR_MSG_INIT(message);

	if (!file) {
		CMR_LOGE("scale error: file hand is null");
		ret = CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	cfg_params =(struct scale_cfg_param_t*)calloc(1, sizeof(struct scale_cfg_param_t));
	if (!cfg_params) {
		CMR_LOGE("scale error: no mem for cfg parameters");
		ret = CMR_CAMERA_NO_MEM;
		goto exit;
	}

	frame_params = &cfg_params->frame_params;
	cfg_params->scale_cb = cmr_event_cb;
	cfg_params->cb_handle = cb_handle;

	/*set scale input parameters*/
	memcpy((void*)&frame_params->input_size, (void*)&src_img->size,
		sizeof(struct scale_size_t));

	memcpy((void*)&frame_params->input_rect, (void*)&src_img->rect,
		sizeof(struct scale_rect_t));

	frame_params->input_format = cmr_scale_fmt_cvt(src_img->fmt);

#if 0
	memcpy((void*)&frame_params->input_addr , (void*)&src_img->addr_phy,
		sizeof(struct scale_addr_t));
#else
	frame_params->input_addr.yaddr = (uint32_t)src_img->addr_phy.addr_y;
	frame_params->input_addr.uaddr = (uint32_t)src_img->addr_phy.addr_u;
	frame_params->input_addr.vaddr = (uint32_t)src_img->addr_phy.addr_v;
#endif
	memcpy((void*)&frame_params->input_endian, (void*)&src_img->data_end,
		sizeof(struct scale_endian_sel_t));

	/*set scale output parameters*/
	memcpy((void*)&frame_params->output_size, (void*)&dst_img->size,
		sizeof(struct scale_size_t));

	frame_params->output_format =cmr_scale_fmt_cvt(dst_img->fmt);

#if 0
	memcpy((void*)&frame_params->output_addr , (void*)&dst_img->addr_phy,
		sizeof(struct scale_addr_t));
#else
	frame_params->output_addr.yaddr = (uint32_t)dst_img->addr_phy.addr_y;
	frame_params->output_addr.uaddr = (uint32_t)dst_img->addr_phy.addr_u;
	frame_params->output_addr.vaddr = (uint32_t)dst_img->addr_phy.addr_v;
#endif
	memcpy((void*)&frame_params->output_endian, (void*)&dst_img->data_end,
		sizeof(struct scale_endian_sel_t));

	/*set scale mode*/
	frame_params->scale_mode = SCALE_MODE_NORMAL;

	/*start scale*/
	message.msg_type   = CMR_EVT_SCALE_START;

	if(NULL == cmr_event_cb) {
		message.sync_flag  = CMR_MSG_SYNC_PROCESSED;
		pthread_mutex_lock(&file->scale_mutex);
	} else {
		message.sync_flag  = CMR_MSG_SYNC_NONE;
	}

	message.data       = (void *)cfg_params;
	message.alloc_flag = 1;
	ret = cmr_thread_msg_send(file->scale_thread, &message);
	if (ret) {
		CMR_LOGE("scale error: fail to send start msg");
		ret = CMR_CAMERA_FAIL;
		goto free_frame;
	}

	if (cmr_event_cb) {
		sem_wait(&file->sync_sem);
		ret = file->sync_none_err_code;
		file->sync_none_err_code = CMR_CAMERA_SUCCESS;
		return ret;
	}

	ret = file->err_code;
	file->err_code = CMR_CAMERA_SUCCESS;
	pthread_mutex_unlock(&file->scale_mutex);
	return ret;

free_frame:
	free(cfg_params);
	cfg_params = NULL;
	if(NULL == cmr_event_cb) {
		pthread_mutex_unlock(&file->scale_mutex);
	}

exit:

	return ret;

}

cmr_int cmr_scale_close(cmr_handle scale_handle)
{
	cmr_int                 ret = CMR_CAMERA_SUCCESS;
	struct scale_file       *file = (struct scale_file*)(scale_handle);

	CMR_LOGI("scale close device enter");

	if (!file) {
		CMR_LOGI("scale fail: file hand is null");
		ret = CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	ret = cmr_scale_destory_thread(file);
	if (ret) {
		CMR_LOGE("scale error: kill thread");
	}

	if (-1 != file->handle) {
		if (-1 == close(file->handle)) {
			CMR_LOGE("scale error: close");
		}
	} else {
		CMR_LOGE("scale error: handle is invalid");
	}
	sem_destroy(&file->sync_sem);
	pthread_mutex_destroy(&file->scale_mutex);
	free(file);

exit:
	CMR_LOGI("scale close device exit");

	return ret;
}

cmr_int cmr_scale_capability(cmr_handle scale_handle,cmr_u32 *width, cmr_u32 *sc_factor)
{
	int             ret = CMR_CAMERA_SUCCESS;
	cmr_u32         rd_word[2] = {0, 0};

	struct scale_file *file = (struct scale_file*)(scale_handle);

	if (!file) {
		CMR_LOGE("scale error: file hand is null");
		return CMR_CAMERA_INVALID_PARAM;
	}

	if (-1 == file->handle) {
		CMR_LOGE("Fail to open scaler device.");
		return CMR_CAMERA_FAIL;
	}

	if (NULL == width || NULL == sc_factor) {
		CMR_LOGE("scale error: param={%p, %p)", width, sc_factor);
		return CMR_CAMERA_INVALID_PARAM;
	}

	ret = read(file->handle, rd_word, 2*sizeof(uint32_t));
	*width = rd_word[0];
	*sc_factor = rd_word[1];

	CMR_LOGI("scale width=%d, sc_factor=%d", *width, *sc_factor);

	return ret;
}

