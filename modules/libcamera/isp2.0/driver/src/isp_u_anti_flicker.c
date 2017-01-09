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

#define LOG_TAG "isp_u_anti_flicker"

#include "isp_drv.h"

isp_s32 isp_u_anti_flicker_statistic(isp_handle handle, void *addr)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle || !addr) {
		ISP_LOGE("handle is null error: 0x%lx 0x%lx",
				(isp_uint)handle, (isp_uint)addr);
		return -1;
	}

	file = (struct isp_file*)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_ANTI_FLICKER;
	param.property = ISP_PRO_ANTI_FLICKER_STATISTIC;
	param.property_param = addr;

	ISP_LOGE("$$LHC:isp_u_anti_flicker_statistic");
	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}


isp_s32 isp_u_anti_flicker_bypass(isp_handle handle, void *block_info)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle || !block_info) {
		ISP_LOGE("handle is null error: 0x%lx 0x%lx",
				(isp_uint)handle, (isp_uint)block_info);
		return -1;
	}

	file = (struct isp_file*)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_ANTI_FLICKER;
	param.property = ISP_PRO_ANTI_FLICKER_BYPASS;
	param.property_param = block_info;

	ISP_LOGE("$$LHC:bypass %d", *(uint32_t *)block_info);
	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}


isp_s32 isp_u_anti_flicker_block(isp_handle handle, void *block_info)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle || !block_info) {
		ISP_LOGE("handle is null error: 0x%lx 0x%lx",
				(isp_uint)handle, (isp_uint)block_info);
		return -1;
	}

	file = (struct isp_file*)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_ANTI_FLICKER;
	param.property = ISP_PRO_ANTI_FLICKER_BLOCK;
	param.property_param = block_info;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_anti_flicker_transaddr(isp_handle handle, isp_u32 phys_addr)
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
	param.sub_block = ISP_BLOCK_ANTI_FLICKER;
	param.property = ISP_PRO_ANTI_FLICKER_TRANSADDR;
	param.property_param = &phys_addr;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}
