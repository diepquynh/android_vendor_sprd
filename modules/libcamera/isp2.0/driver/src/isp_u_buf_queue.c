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

#define LOG_TAG "isp_u_buf_queue"

#include "isp_drv.h"

isp_s32 isp_u_bq_init_bufqueue(isp_handle handle)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_BUFQUEUE;
	param.property = ISP_PRO_BUFQUEUE_INIT;
	param.property_param = &ret;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_bq_enqueue_buf(isp_handle handle, uint64_t k_addr, uint64_t u_addr, isp_u32 type)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_buf_node bq_node;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	bq_node.type = type;
	bq_node.k_addr = k_addr;
	bq_node.u_addr = u_addr;
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_BUFQUEUE;
	param.property = ISP_PRO_BUFQUEUE_ENQUEUE_BUF;
	param.property_param = &bq_node;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_bq_dequeue_buf(isp_handle handle, uint64_t *k_addr, uint64_t *u_addr, isp_u32 type)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_buf_node bq_node;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	bq_node.type = type;
	bq_node.k_addr = 0;
	bq_node.u_addr = 0;
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_BUFQUEUE;
	param.property = ISP_PRO_BUFQUEUE_DEQUEUE_BUF;
	param.property_param = &bq_node;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	*k_addr = bq_node.k_addr;
	*u_addr = bq_node.u_addr;

	return ret;
}
