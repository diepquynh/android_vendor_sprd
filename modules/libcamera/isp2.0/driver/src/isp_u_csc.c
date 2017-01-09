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

#define LOG_TAG "isp_u_csc"

#include "isp_drv.h"

isp_s32 isp_u_csc_block(isp_handle handle, void *block_info)
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
	param.sub_block = ISP_BLOCK_CSC;
	param.property = ISP_PRO_CSC_BLOCK;
	param.property_param = block_info;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_csc_pic_size(isp_handle handle, isp_u32 width, isp_u32 height)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_img_size slice_size;

	if (!handle) {
		ISP_LOGE("isp_u_csc_pic_size: handle is null error.");
		return -1;
	}

	file = (struct isp_file*)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_CSC;
	param.property = ISP_PRO_CSC_PIC_SIZE;
	slice_size.width = width;
	slice_size.height = height;
	param.property_param = &slice_size;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}
