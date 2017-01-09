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

#define LOG_TAG "isp_u_hdr"

#include "isp_drv.h"

isp_s32 isp_u_hdr_block(isp_handle handle, void *block_info)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle || !block_info) {
		ISP_LOGE("handle is null error: 0x%lx 0x%lx",
				(isp_uint)handle, (isp_uint)block_info);
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_HDR;
	param.property = ISP_PRO_HDR_BLOCK;
	param.property_param = block_info;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_hdr_bypass(isp_handle handle, isp_u32 bypass)
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
	param.sub_block = ISP_BLOCK_HDR;
	param.property = ISP_PRO_HDR_BYPASS;
	param.property_param = &bypass;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_hdr_level(isp_handle handle, isp_u32 level)
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
	param.sub_block = ISP_BLOCK_HDR;
	param.property = ISP_PRO_HDR_LEVEL;
	param.property_param = &level;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_hdr_index(isp_handle handle,
		isp_u32 r_index,
		isp_u32 g_index,
		isp_u32 b_index)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_hdr_rgb_index index;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_HDR;
	param.property = ISP_PRO_HDR_INDEX;
	index.r = r_index;
	index.g = g_index;
	index.b = b_index;
	param.property_param = &index;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_hdr_tab(isp_handle handle,
		isp_u8 *com_ptr,
		isp_u8 *p2e_ptr,
		isp_u8 *e2p_ptr)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_hdr_tab tab;

	if (!handle || !com_ptr || !p2e_ptr || !e2p_ptr) {
		ISP_LOGE("handle is null error: 0x%lx 0x%lx 0x%lx 0x%lx",
				(isp_uint)handle,
				(isp_uint)com_ptr,
				(isp_uint)p2e_ptr,
				(isp_uint)e2p_ptr);
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_HDR;
	param.property = ISP_PRO_HDR_TAB;
	/*config the compensation value */
	memcpy((void *)(tab.com), (void *)com_ptr, sizeof(tab.com));
	/*config the p2e map */
	memcpy((void *)(tab.p2e), (void *)p2e_ptr, sizeof(tab.p2e));
	/*config the e2p map */
	memcpy((void *)(tab.e2p), (void *)e2p_ptr, sizeof(tab.e2p));
	param.property_param = &tab;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}
