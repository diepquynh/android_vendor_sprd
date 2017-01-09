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

#ifdef CONFIG_CAMERA_HDR_CAPTURE

#define LOG_TAG "cmr_hdr"

#include "cmr_msg.h"
#include "cmr_ipm.h"
#include "cmr_common.h"
#include "cmr_sensor.h"
#include "cmr_oem.h"
#ifdef CONFIG_SPRD_HDR_LIB
#include "../../arithmetic/inc/HDR_SPRD.h"
#else
#include "../../arithmetic/inc/HDR2.h"
#endif

struct class_hdr {
	struct ipm_common               common;
	cmr_u8                          *alloc_addr[HDR_CAP_NUM];
	cmr_uint                        mem_size;
	cmr_uint                        width;
	cmr_uint                        height;
	cmr_uint                        is_inited;
	cmr_handle                      hdr_thread;
	struct img_addr                dst_addr;
	ipm_callback                    reg_cb;
	struct ipm_frame_in            frame_in;
};

enum oem_ev_level {
	OEM_EV_LEVEL_1,
	OEM_EV_LEVEL_2,
	OEM_EV_LEVEL_3
};

#define CHECK_HANDLE_VALID(handle) \
	do { \
		if (!handle) { \
			return -CMR_CAMERA_INVALID_PARAM; \
		} \
	} while(0)


#define IMAGE_FORMAT                    "YVU420_SEMIPLANAR"
#define CAMERA_HDR_MSG_QUEUE_SIZE       5

#define CMR_EVT_HDR_BASE                (CMR_EVT_IPM_BASE + 0X100)
#define CMR_EVT_HDR_INIT                (CMR_EVT_HDR_BASE + 0)
#define CMR_EVT_HDR_START               (CMR_EVT_HDR_BASE + 1)
#define CMR_EVT_HDR_EXIT                (CMR_EVT_HDR_BASE + 2)
#define CMR_EVT_HDR_SAVE_FRAME          (CMR_EVT_HDR_BASE + 3)
#define CMR_EVT_HDR_PRE_PROC            (CMR_EVT_HDR_BASE + 4)

typedef cmr_int (*ipm_get_sensor_info)(cmr_handle oem_handle, cmr_uint sensor_id, struct sensor_exp_info *sensor_info);
typedef cmr_int (*ipm_sensor_ioctl)(cmr_handle oem_handle, cmr_uint cmd_type, struct common_sn_cmd_param *parm);
typedef cmr_int (*ipm_isp_ioctl)(cmr_handle oem_handle, cmr_uint cmd_type, struct common_isp_cmd_param *parm);

static cmr_int hdr_open(cmr_handle ipm_handle, struct ipm_open_in *in, struct ipm_open_out *out,
				cmr_handle *class_handle);
static cmr_int hdr_close(cmr_handle class_handle);
static cmr_int hdr_transfer_frame(cmr_handle class_handle, struct ipm_frame_in *in,
				struct ipm_frame_out *out);
static cmr_int hdr_pre_proc(cmr_handle class_handle);
static cmr_int hdr_post_proc(cmr_handle class_handle);
static cmr_int req_hdr_do(cmr_handle class_handle, struct img_addr *dst_addr,
				struct img_size frame_size);
static cmr_int hdr_arithmetic(cmr_handle class_handle, struct img_addr *dst_addr,
				cmr_u32 width,cmr_u32 height);
static cmr_int hdr_thread_proc(struct cmr_msg *message, void *private_data);
static cmr_int hdr_thread_create(struct class_hdr *class_handle);
static cmr_int hdr_thread_destroy(struct class_hdr *class_handle);
static cmr_int hdr_save_frame(cmr_handle class_handle, struct ipm_frame_in *in);
static cmr_int req_hdr_save_frame(cmr_handle class_handle, struct ipm_frame_in *in);
static cmr_int hdr_frame_proc(cmr_handle class_handle);

static struct class_ops hdr_ops_tab_info = {
	hdr_open,
	hdr_close,
	hdr_transfer_frame,
	hdr_pre_proc,
	hdr_post_proc,
};

struct class_tab_t hdr_tab_info = {
	&hdr_ops_tab_info,
};

static cmr_int hdr_open(cmr_handle ipm_handle, struct ipm_open_in *in, struct ipm_open_out *out,
				cmr_handle *class_handle)
{
	cmr_int               ret         = CMR_CAMERA_SUCCESS;
	struct class_hdr     *hdr_handle = NULL;
	cmr_uint              size;
	cmr_int               i = 0;

	if (!out || !in || !ipm_handle || !class_handle) {
		CMR_LOGE("Invalid Param!");
		return CMR_CAMERA_INVALID_PARAM;
	}

	hdr_handle = (struct class_hdr *)malloc(sizeof(struct class_hdr));
	if (!hdr_handle) {
		CMR_LOGE("No mem!");
		return CMR_CAMERA_NO_MEM;
	}

	cmr_bzero(hdr_handle, sizeof(struct class_hdr));

	out->format = IMG_FMT_YCBCR420;
	out->total_frame_number = HDR_CAP_NUM;

	size = (cmr_uint)(in->frame_size.width * in->frame_size.height * 3 / 2);

	CMR_LOGI("in->frame_size.width = %d,in->frame_size.height = %d", in->frame_size.width, in->frame_size.height);

	hdr_handle->common.ipm_cxt = (struct ipm_context_t*)ipm_handle;
	hdr_handle->common.class_type = IPM_TYPE_HDR;
	hdr_handle->common.receive_frame_count = 0;
	hdr_handle->common.save_frame_count = 0;
	hdr_handle->common.ops = &hdr_ops_tab_info;

	for (i = 0; i < HDR_CAP_NUM; i++) {
		hdr_handle->alloc_addr[i] = (cmr_u8*)malloc(size);
		if (NULL == hdr_handle->alloc_addr[i]){
			CMR_LOGE("mem alloc failed i = %ld", i);
			goto free_all;
		}
	}
	hdr_handle->mem_size = size;

	CMR_LOGI("hdr_handle->mem_size = 0x%lx", hdr_handle->mem_size);

	hdr_handle->height = in->frame_size.height;
	hdr_handle->width = in->frame_size.width;
	hdr_handle->reg_cb = in->reg_cb;

	ret = hdr_thread_create(hdr_handle);
	if (ret) {
		CMR_LOGE("HDR error: create thread.");
		goto free_all;
	}

	*class_handle = (cmr_handle)hdr_handle;

#ifdef CONFIG_SPRD_HDR_LIB
    sprd_hdr_pool_init ();
#endif

	return ret;

free_all:
	for (i = 0; i < HDR_CAP_NUM; i++) {
		if (NULL != hdr_handle->alloc_addr[i]) {
			free(hdr_handle->alloc_addr[i]);
		}
	}
	if(NULL != hdr_handle)
		free(hdr_handle);
	return CMR_CAMERA_NO_MEM;

}

static cmr_int hdr_close(cmr_handle class_handle)
{
	cmr_int               ret         = CMR_CAMERA_SUCCESS;
	struct class_hdr     *hdr_handle = (struct class_hdr *)class_handle;
	cmr_int               i;
	CHECK_HANDLE_VALID(hdr_handle);

	ret = hdr_thread_destroy(hdr_handle);
	if (ret) {
		CMR_LOGE("HDR failed to destroy hdr thread.");
	}
	for (i = 0; i < HDR_CAP_NUM; i++) {
		if (NULL != hdr_handle->alloc_addr[i]) {
			free(hdr_handle->alloc_addr[i]);
		}
	}
	if (NULL != hdr_handle)
		free(hdr_handle);

#ifdef CONFIG_SPRD_HDR_LIB
    sprd_hdr_pool_destroy ();
#endif

	return ret;
}

static cmr_int req_hdr_save_frame(cmr_handle class_handle, struct ipm_frame_in *in)
{
	cmr_int                 ret         = CMR_CAMERA_SUCCESS;
	struct class_hdr       *hdr_handle = (struct class_hdr *)class_handle;

	CMR_MSG_INIT(message);

	if (!class_handle || !in) {
		CMR_LOGE("Invalid Param!");
		return CMR_CAMERA_INVALID_PARAM;
	}

	hdr_handle->common.save_frame_count++;
	if (hdr_handle->common.save_frame_count > HDR_CAP_NUM) {
		CMR_LOGE("cap cnt error,%ld.",hdr_handle->common.save_frame_count);
		return CMR_CAMERA_FAIL;
	}
	message.data = (struct ipm_frame_in*)malloc(sizeof(struct ipm_frame_in));
	if (!message.data) {
		CMR_LOGE("No mem!");
		ret = CMR_CAMERA_NO_MEM;
		return ret;
	}
	memcpy(message.data, in, sizeof(struct ipm_frame_in));
	message.msg_type = CMR_EVT_HDR_SAVE_FRAME;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	message.alloc_flag = 1;
	ret = cmr_thread_msg_send(hdr_handle->hdr_thread, &message);
	if (ret) {
		CMR_LOGE("Failed to send one msg to hdr thread.");
		if (message.data) {
			free(message.data);
		}
	}
	return ret;
}

static cmr_int hdr_transfer_frame(cmr_handle class_handle, struct ipm_frame_in *in, struct ipm_frame_out *out)
{
	cmr_int               ret = CMR_CAMERA_SUCCESS;
	struct class_hdr      *hdr_handle = (struct class_hdr *)class_handle;
	cmr_u32               frame_in_cnt;
	struct img_addr       *addr;
	struct img_size       size;
	cmr_handle            oem_handle;
	ipm_sensor_ioctl      sensor_ioctl;
	ipm_isp_ioctl         isp_ioctl;
	struct common_sn_cmd_param sn_param;
	struct common_isp_cmd_param isp_param;
	cmr_u32               sensor_id = 0;
	ipm_get_sensor_info   get_sensor_info;

	if (!out || !in || !class_handle) {
		CMR_LOGE("Invalid Param!");
		return CMR_CAMERA_INVALID_PARAM;
	}

	CMR_LOGI("ipm_frame_in.private_data 0x%lx", (cmr_int)in->private_data);
	addr = &in->dst_frame.addr_vir;
	size = in->src_frame.size;

	ret = req_hdr_save_frame(class_handle, in);
	if (ret != CMR_CAMERA_SUCCESS) {
		CMR_LOGE("req_hdr_save_frame fail");
		return CMR_CAMERA_FAIL;
	}

	if (hdr_handle->common.save_frame_count == HDR_CAP_NUM) {
		cmr_bzero(&out->dst_frame,sizeof(struct img_frm));
		sensor_ioctl = hdr_handle->common.ipm_cxt->init_in.ipm_sensor_ioctl;
		isp_ioctl = hdr_handle ->common.ipm_cxt->init_in.ipm_isp_ioctl;
		oem_handle = hdr_handle->common.ipm_cxt->init_in.oem_handle;
		hdr_handle->frame_in = *in;
		ret = req_hdr_do(class_handle, addr, size);
		if (ret != CMR_CAMERA_SUCCESS){
			CMR_LOGE("req_hdr_do fail");
		}
		out->dst_frame = in->dst_frame;
		out->private_data = in->private_data;
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
		struct sensor_exp_info sensor_info;
		sensor_id = hdr_handle->common.ipm_cxt->init_in.sensor_id;
		get_sensor_info = hdr_handle->common.ipm_cxt->init_in.get_sensor_info;
		get_sensor_info(oem_handle, sensor_id, &sensor_info);

		if (SENSOR_IMAGE_FORMAT_RAW == sensor_info.image_format) {
			isp_param.cmd_value= OEM_EV_LEVEL_2;
			ret = isp_ioctl(oem_handle,COM_ISP_SET_HDR,(void *)&isp_param);
		} else {
			sn_param.cmd_value = OEM_EV_LEVEL_2;
			ret = sensor_ioctl(oem_handle,COM_SN_SET_HDR_EV,(void *)&sn_param);
		}
#else
		sn_param.cmd_value = OEM_EV_LEVEL_2;
		ret = sensor_ioctl(oem_handle,COM_SN_SET_HDR_EV,(void *)&sn_param);
#endif
		if (ret) {
			CMR_LOGE("HDR failed to set ev.");
		}
	}

	return ret;
}

static cmr_int hdr_pre_proc(cmr_handle class_handle)
{
	cmr_int               ret          = CMR_CAMERA_SUCCESS;
	struct class_hdr      *hdr_handle  = (struct class_hdr *)class_handle;
	CMR_MSG_INIT(message);

	if (!class_handle) {
		CMR_LOGE("Invalid Param!");
		return CMR_CAMERA_INVALID_PARAM;
	}
	message.msg_type = CMR_EVT_HDR_PRE_PROC;
	message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	ret = cmr_thread_msg_send(hdr_handle->hdr_thread, &message);
	if (ret) {
		CMR_LOGE("Failed to send one msg to hdr thread.");
	}
	return ret;
}

static cmr_int hdr_frame_proc(cmr_handle class_handle)
{
	cmr_int               ret          = CMR_CAMERA_SUCCESS;
	struct class_hdr      *hdr_handle  = (struct class_hdr *)class_handle;
	cmr_u32               sensor_id = 0;
	cmr_u32               img_fmt = 0;
	cmr_u32               frame_in_cnt = 0;
	cmr_handle            oem_handle = NULL;
	enum oem_ev_level     ev_level = OEM_EV_LEVEL_1;
	ipm_get_sensor_info   get_sensor_info;
	ipm_sensor_ioctl      sensor_ioctl;
	ipm_isp_ioctl         isp_ioctl;
	struct common_sn_cmd_param sn_param;
	struct common_isp_cmd_param isp_param;
	struct sensor_exp_info sensor_info;

	if (!hdr_handle){
		CMR_LOGE("hdr_handle is NULL");
		ret = CMR_CAMERA_INVALID_PARAM;
		return ret;
	}
	frame_in_cnt = ++hdr_handle->common.receive_frame_count;
	sensor_id = hdr_handle->common.ipm_cxt->init_in.sensor_id;
	get_sensor_info = hdr_handle->common.ipm_cxt->init_in.get_sensor_info;
	sensor_ioctl = hdr_handle->common.ipm_cxt->init_in.ipm_sensor_ioctl;
	isp_ioctl = hdr_handle->common.ipm_cxt->init_in.ipm_isp_ioctl;
	oem_handle = hdr_handle->common.ipm_cxt->init_in.oem_handle;

	CMR_LOGI("frame cnt %d", frame_in_cnt);
	switch (frame_in_cnt) {
	case 1:
		ev_level = OEM_EV_LEVEL_1;
		break;
	case 2:
		ev_level = OEM_EV_LEVEL_2;
		break;
	case 3:
		ev_level = OEM_EV_LEVEL_3;
		break;
	default:
		ev_level = OEM_EV_LEVEL_1;
		break;
	}

	get_sensor_info(oem_handle, sensor_id, &sensor_info);

	CMR_LOGI("HDR ev_level = %d, img_fmt=%d", ev_level, sensor_info.image_format);

#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
	if (SENSOR_IMAGE_FORMAT_RAW == sensor_info.image_format) {
		isp_param.cmd_value = (cmr_u32)ev_level;
		ret = isp_ioctl(oem_handle,COM_ISP_SET_HDR,(void *)&isp_param);
	} else {
		sn_param.cmd_value = (cmr_u32)ev_level;
		ret = sensor_ioctl(oem_handle,COM_SN_SET_HDR_EV,(void *)&sn_param);
	}
#else
	sn_param.cmd_value = (cmr_u32)ev_level;
	ret = sensor_ioctl(oem_handle,COM_SN_SET_HDR_EV,(void *)&sn_param);
#endif

	if (ret) {
		CMR_LOGE("HDR failed to set ev.");
	}

	return ret;
}

static cmr_int hdr_post_proc(cmr_handle class_handle)
{
	UNUSED(class_handle);

	cmr_int               ret = CMR_CAMERA_SUCCESS;

	/*no need to do*/

	return ret;
}

static cmr_int req_hdr_do(cmr_handle class_handle, struct img_addr *dst_addr, struct img_size frame_size)
{
	cmr_int               ret          = CMR_CAMERA_SUCCESS;
	struct class_hdr      *hdr_handle  = (struct class_hdr *)class_handle;

	CMR_MSG_INIT(message);

	if (!dst_addr || !class_handle) {
		CMR_LOGE("Invalid Param!");
		return CMR_CAMERA_INVALID_PARAM;
	}

	hdr_handle->dst_addr = *dst_addr;
	hdr_handle->width = frame_size.width;
	hdr_handle->height = frame_size.height;

	message.msg_type = CMR_EVT_HDR_START;
	if (NULL != hdr_handle->reg_cb)
		message.sync_flag = CMR_MSG_SYNC_RECEIVED;
	else
		message.sync_flag = CMR_MSG_SYNC_PROCESSED;
	ret = cmr_thread_msg_send(hdr_handle->hdr_thread, &message);
	if (ret) {
		CMR_LOGE("Failed to send one msg to hdr thread.");
	}

	return ret;
}

static cmr_int hdr_arithmetic(cmr_handle class_handle, struct img_addr *dst_addr,cmr_u32 width,cmr_u32 height)
{
	cmr_int            ret         = CMR_CAMERA_SUCCESS;
	cmr_u32            size        = width*height;
	cmr_u8             *temp_addr0 = NULL;
	cmr_u8             *temp_addr1 = NULL;
	cmr_u8             *temp_addr2 = NULL;
	cmr_s8             *p_format   = IMAGE_FORMAT;
	struct class_hdr  *hdr_handle = (struct class_hdr *)class_handle;

	if (!class_handle || !dst_addr) {
		CMR_LOGE("Invalid Param!");
		return CMR_CAMERA_INVALID_PARAM;
	}

	temp_addr0 = hdr_handle->alloc_addr[0];
	temp_addr1 = hdr_handle->alloc_addr[1];
	temp_addr2 = hdr_handle->alloc_addr[2];
	CMR_LOGD("width %d,height %d.",width,height);
	/*save_input_data(width,height);*/

	if ((NULL != temp_addr0) && (NULL != temp_addr1) && (NULL != temp_addr2)) {
		if (0 != HDR_Function(temp_addr0, temp_addr1, temp_addr2, temp_addr0,
			height, width, p_format)) {
			CMR_LOGE("hdr error!");
			ret = CMR_CAMERA_FAIL;
		}
	} else {
			CMR_LOGE("can't handle hdr.");
			ret = CMR_CAMERA_FAIL;
	}
	if (NULL != temp_addr0) {
		memcpy((void *)dst_addr->addr_y,(void *)temp_addr0,size);
		memcpy((void *)dst_addr->addr_u,(void *)(temp_addr0+size),size / 2);
	}

	if (CMR_CAMERA_SUCCESS == ret) {
		CMR_LOGD("hdr done.");
	}
	return ret;
}

static cmr_int hdr_save_frame(cmr_handle class_handle, struct ipm_frame_in *in)
{
	cmr_int                 ret         = CMR_CAMERA_SUCCESS;
	cmr_u8                  *uv_addr    = NULL;
	cmr_uint                y_size      = 0;
	cmr_uint                uv_size     = 0;
	struct class_hdr       *hdr_handle = (struct class_hdr *)class_handle;
	cmr_int                 frame_sn    = 0;
	if (!class_handle || !in) {
		CMR_LOGE("Invalid Param!");
		return CMR_CAMERA_INVALID_PARAM;
	}

	if (hdr_handle->common.save_frame_count > HDR_CAP_NUM) {
		CMR_LOGE("cap cnt error,%ld.", hdr_handle->common.save_frame_count);
		return CMR_CAMERA_FAIL;
	}

	y_size = in->src_frame.size.height * in->src_frame.size.width;
	uv_size = in->src_frame.size.height * in->src_frame.size.width / 2;
	frame_sn = hdr_handle->common.save_frame_count -1;
	if (frame_sn < 0) {
		CMR_LOGE("frame_sn error,%ld.", frame_sn);
		return CMR_CAMERA_FAIL;
	}
	if (NULL == hdr_handle->alloc_addr[frame_sn]) {
		CMR_LOGE("no memory.");
		return CMR_CAMERA_FAIL;
	}

	if (hdr_handle->mem_size >= in->src_frame.buf_size) {
		memcpy((void *)hdr_handle->alloc_addr[frame_sn], (void *)in->src_frame.addr_vir.addr_y, y_size);
		uv_addr = hdr_handle->alloc_addr[frame_sn] + y_size;
		memcpy((void *)uv_addr, (void *)in->src_frame.addr_vir.addr_u, uv_size);
	} else {
		CMR_LOGW(" HDR:mem size:0x%lx,data y_size:0x%lx.",hdr_handle->mem_size,y_size);
	}

	return ret;
}

static cmr_int hdr_thread_proc(struct cmr_msg *message, void *private_data)
{
	cmr_int                 ret           = CMR_CAMERA_SUCCESS;
	struct class_hdr       *class_handle = (struct class_hdr *)private_data;
	cmr_u32                 evt = 0;
	struct ipm_frame_out   out;
	struct 					ipm_frame_in *in;

	if (!message || !class_handle) {
		CMR_LOGE("parameter is fail");
		return CMR_CAMERA_INVALID_PARAM;
	}

	evt = (cmr_u32)message->msg_type;

	switch (evt) {
	case CMR_EVT_HDR_INIT:
		CMR_LOGI("HDR thread inited.");
		break;
	case CMR_EVT_HDR_PRE_PROC:
		CMR_LOGI("HDR pre_proc");
		hdr_frame_proc(class_handle);
		break;
	case CMR_EVT_HDR_SAVE_FRAME:
		CMR_LOGI("HDR save frame");
		in = message->data;
		ret = hdr_save_frame(class_handle, in);
		if (ret != CMR_CAMERA_SUCCESS) {
			CMR_LOGE("HDR save frame failed.");
		}
		break;
	case CMR_EVT_HDR_START:
		class_handle->common.receive_frame_count = 0;
		class_handle->common.save_frame_count = 0;
		out.dst_frame = class_handle->frame_in.dst_frame;
		out.private_data = class_handle->frame_in.private_data;
		CMR_LOGI("out private_data 0x%lx", (cmr_int)out.private_data);
		CMR_LOGI("CMR_EVT_HDR_START addr 0x%lx %ld %ld", class_handle->dst_addr.addr_y, class_handle->width, class_handle->height);
		CMR_LOGI("HDR thread proc start ");
		hdr_arithmetic(class_handle, &class_handle->dst_addr, class_handle->width, class_handle->height);
		CMR_LOGI("HDR thread proc done ");

		if (class_handle->reg_cb) {
			(class_handle->reg_cb)(IPM_TYPE_HDR, &out);
		}

		break;
	case CMR_EVT_HDR_EXIT:
		CMR_LOGD("HDR thread exit.");
		break;
	default:
		break;
	}

	return ret;
}

static cmr_int hdr_thread_create(struct class_hdr *class_handle)
{
	cmr_int                 ret = CMR_CAMERA_SUCCESS;
	CMR_MSG_INIT(message);

	CHECK_HANDLE_VALID(class_handle);

	if (!class_handle->is_inited) {
		ret = cmr_thread_create(&class_handle->hdr_thread,
					CAMERA_HDR_MSG_QUEUE_SIZE,
					hdr_thread_proc,
					(void*)class_handle);
		if (ret) {
			CMR_LOGE("send msg failed!");
			ret = CMR_CAMERA_FAIL;
			return ret;
		}

		class_handle->is_inited = 1;
	}

	return ret;
}

static cmr_int hdr_thread_destroy(struct class_hdr *class_handle)
{
	cmr_int                 ret = CMR_CAMERA_SUCCESS;
	CMR_MSG_INIT(message);

	CHECK_HANDLE_VALID(class_handle);

	if (class_handle->is_inited) {

		ret = cmr_thread_destroy(class_handle->hdr_thread);
		class_handle->hdr_thread = 0;

		class_handle->is_inited = 0;
	}

	return ret;
}
#endif
