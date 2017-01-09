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

#define LOG_TAG "isp_u_aca"

#include "isp_drv.h"

isp_s32 isp_u_aca_block(isp_handle handle, void *block_info)
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
	param.sub_block = ISP_BLOCK_ACA;
	param.property = ISP_PRO_ACA_BLOCK;
	param.property_param = block_info;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_aca_bypass(isp_handle handle, isp_u32 bypass)
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
	param.sub_block = ISP_BLOCK_ACA;
	param.property = ISP_PRO_ACA_BYPASS;
	param.property_param = &bypass;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_aca_mode(isp_handle handle, isp_u32 mode)
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
	param.sub_block = ISP_BLOCK_ACA;
	param.property = ISP_PRO_ACA_MODE;
	param.property_param = &mode;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_aca_maxmin(isp_handle handle,
		isp_u32 in_min,
		isp_u32 in_max,
		isp_u32 out_min,
		isp_u32 out_max)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_aca_maxmin maxmin;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	memset(&maxmin, 0, sizeof(maxmin));
	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_ACA;
	param.property = ISP_PRO_ACA_MAXMIN;
	maxmin.in_min = in_min;
	maxmin.in_max = in_max;
	maxmin.out_min = out_min;
	maxmin.out_max = out_max;
	param.property_param = &maxmin;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_aca_adjust(isp_handle handle,
		isp_u32 diff,
		isp_u32 small,
		isp_u32 big)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_aca_adjust adjust;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	memset(&adjust, 0, sizeof(adjust));
	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_ACA;
	param.property = ISP_PRO_ACA_ADJUST;
	adjust.diff = diff;
	adjust.small = small;
	adjust.big = big;
	param.property_param = &adjust;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}
