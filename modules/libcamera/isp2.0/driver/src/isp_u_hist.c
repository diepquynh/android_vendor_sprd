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

#define LOG_TAG "isp_u_hist"

#include "isp_drv.h"

isp_s32 isp_u_hist_block(isp_handle handle, void *block_info)
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
	param.sub_block = ISP_BLOCK_HIST;
	param.property = ISP_PRO_HIST_BLOCK;
	param.property_param = block_info;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_hist_slice_size(isp_handle handle, isp_u32 width, isp_u32 height)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_img_size size;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_HIST;
	param.property = ISP_PRO_HIST_SLICE_SIZE;
	size.width = width;
	size.height = height;
	param.property_param = &size;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_hist_bypass(isp_handle handle, isp_u32 bypass)
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
	param.sub_block = ISP_BLOCK_HIST;
	param.property = ISP_PRO_HIST_BYPASS;
	param.property_param = &bypass;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_hist_auto_rst_disable(isp_handle handle, isp_u32 off)
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
	param.sub_block = ISP_BLOCK_HIST;
	param.property = ISP_PRO_HIST_AUTO_RST_DISABLE;
	param.property_param = &off;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_hist_mode(isp_handle handle, isp_u32 mode)
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
	param.sub_block = ISP_BLOCK_HIST;
	param.property = ISP_PRO_HIST_MODE;
	param.property_param = &mode;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_hist_ratio(isp_handle handle, isp_u16 low_ratio, isp_u16 high_ratio)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_hist_ratio ratio;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_HIST;
	param.property = ISP_PRO_HIST_RATIO;
	memset(&ratio, 0, sizeof(ratio));
	ratio.low_ratio = low_ratio;
	ratio.high_ratio = high_ratio;
	param.property_param = &ratio;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_hist_maxmin(isp_handle handle,
		isp_u32 in_min,
		isp_u32 in_max,
		isp_u32 out_min,
		isp_u32 out_max)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_hist_maxmin maxmin;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_HIST;
	param.property = ISP_PRO_HIST_MAXMIN;
	memset(&maxmin, 0x00, sizeof(maxmin));
	maxmin.in_min = in_min;
	maxmin.in_max = in_max;
	maxmin.out_min = out_min;
	maxmin.out_max = out_max;
	param.property_param = &maxmin;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_hist_clear_eb(isp_handle handle, isp_u32 eb)
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
	param.sub_block = ISP_BLOCK_HIST;
	param.property = ISP_PRO_HIST_CLEAR_EB;
	param.property_param = &eb;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_hist_statistic(isp_handle handle, void *out_value)
{
	isp_s32 ret = 0, i = 0;
	isp_u32 num = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	isp_u32 *item = NULL;

	if (!handle || !out_value) {
		ISP_LOGE("handle is null error: 0x%lx 0x%lx",
				(isp_uint)handle, (isp_uint)out_value);
		return -1;
	}

	isp_u_hist_statistic_num(handle, &num);
	if (0 == num) {
		ISP_LOGW("warning: num is 0.");
		ret = -1;
		goto out;
	}
	item = malloc(num * 4);
	if (!item) {
		ISP_LOGE("no memory error");
		ret = -1;
		goto out;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_HIST;
	param.property = ISP_PRO_HIST_STATISTIC;
	param.property_param = item;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (0 == ret) {
		memcpy(out_value, item, num * 4);
	}
out:
	return ret;
}

isp_s32 isp_u_hist_statistic_num(isp_handle handle, isp_u32 *num)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle || !num) {
		ISP_LOGE("handle is null error: 0x%lx 0x%lx",
				(isp_uint)handle, (isp_uint)num);
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_HIST;
	param.property = ISP_PRO_HIST_STATISTIC_NUM;
	param.property_param = num;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}
