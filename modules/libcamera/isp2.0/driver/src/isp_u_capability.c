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

#define LOG_TAG "isp_u_capability"

#include "isp_drv.h"

isp_s32 isp_u_capability_chip_id(isp_handle handle, isp_u32 *chip_id)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_capability param;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.index = ISP_CAPABILITY_CHIP_ID;
	param.property_param = chip_id;
	ret = ioctl(file->fd, ISP_IO_CAPABILITY, &param);

	if (ret) {
		*chip_id = ISP_CHIP_ID_INVALID;
		ISP_LOGE("get chip id erro.");
	}

	return ret;
}

isp_s32 isp_u_capability_single_size(isp_handle handle,
		isp_u16 *width,
		isp_u16 *height)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_img_size size;
	struct isp_capability param;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.index = ISP_CAPABILITY_SINGLE_SIZE;
	param.property_param = &size;
	ret = ioctl(file->fd, ISP_IO_CAPABILITY, &param);

	if (0 == ret) {
		*width = size.width;
		*height = size.height;
	} else {
		*width = 0;
		*height = 0;
		ISP_LOGE("get single size erro.");
	}

	return ret;
}

isp_s32 isp_u_capability_continue_size(isp_handle handle,
		isp_u16 *width,
		isp_u16 *height)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_img_size size;
	struct isp_capability param;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.index = ISP_CAPABILITY_CONTINE_SIZE;
	param.property_param = &size;
	ret = ioctl(file->fd, ISP_IO_CAPABILITY, &param);

	if (0 == ret) {
		*width = size.width;
		*height = size.height;
	} else {
		*width = 0;
		*height = 0;
		ISP_LOGE("get continue size erro.");
	}

	return ret;
}

isp_s32 isp_u_capability_awb_win(isp_handle handle,
		isp_u16 *width,
		isp_u16 *height)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_img_size size;
	struct isp_capability param;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.index = ISP_CAPABILITY_AWB_WIN;
	param.property_param = &size;
	ret = ioctl(file->fd, ISP_IO_CAPABILITY, &param);

	if (0 == ret) {
		*width = size.width;
		*height = size.height;
	} else {
		*width = 0;
		*height = 0;
		ISP_LOGE("get continue size erro.");
	}

	return ret;
}

isp_u32 isp_u_capability_awb_default_gain(isp_handle handle)
{
	isp_s32 ret = 0;
	isp_u32 gain = 0;
	struct isp_file *file = NULL;
	struct isp_capability param;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.index = ISP_CAPABILITY_AWB_DEFAULT_GAIN;
	param.property_param = &gain;
	ret = ioctl(file->fd, ISP_IO_CAPABILITY, &param);

	if (ret) {
		ISP_LOGE("get awb default gain erro.");
	}

	return gain;
}

isp_u32 isp_u_capability_af_max_win_num(isp_handle handle)
{
	isp_s32 ret = 0;
	isp_u32 win_num = 0;
	struct isp_file *file = NULL;
	struct isp_capability param;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.index = ISP_CAPABILITY_AF_MAX_WIN_NUM;
	param.property_param = &win_num;
	ret = ioctl(file->fd, ISP_IO_CAPABILITY, &param);

	if (ret) {
		ISP_LOGE("get af max window num erro.");
	}

	return win_num;
}

isp_s32 isp_u_capability_time(isp_handle handle, isp_u32 *sec, isp_u32 *usec)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_capability param;
	struct isp_time time = {0, 0};

	if (!handle || !sec || !usec) {
		ISP_LOGE("handle is null error: %p %p %p", handle, sec, usec);
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.index = ISP_CAPABILITY_TIME;
	param.property_param = &time;
	ret = ioctl(file->fd, ISP_IO_CAPABILITY, &param);

	if (ret) {
		*sec = 0;
		*usec = 0;
		ISP_LOGE("get time error.");
	} else {
		*sec = time.sec;
		*usec = time.usec;
	}

	return ret;
}