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

#define LOG_TAG "isp_u_awb"

#include "isp_drv.h"

isp_s32 isp_u_awb_block(isp_handle handle, void *block_info)
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
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWB_BLOCK;
	param.property_param = block_info;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_awbm_statistics(isp_handle handle,
		isp_u32 *r_info,
		isp_u32 *g_info,
		isp_u32 *b_info)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_awbm_statistics *awbm_statistics = NULL;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	if (!r_info || !g_info || !b_info) {
		ISP_LOGE("data ptr is null error: 0x%lx 0x%lx 0x%lx",
			(isp_uint)r_info, (isp_uint)g_info, (isp_uint)b_info);
		return -1;
	}

	awbm_statistics = (struct isp_awbm_statistics *)malloc(sizeof(struct isp_awbm_statistics));
	if(!awbm_statistics) {
		ISP_LOGE("No mem");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBM_STATISTICS;
	memset(awbm_statistics, 0x00, sizeof(struct isp_awbm_statistics));
	param.property_param = awbm_statistics;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (0 == ret) {
		memcpy((isp_handle)r_info, (isp_handle)(awbm_statistics->r), ISP_AWBM_ITEM * 4);
		memcpy((isp_handle)g_info, (isp_handle)(awbm_statistics->g), ISP_AWBM_ITEM * 4);
		memcpy((isp_handle)b_info, (isp_handle)(awbm_statistics->b), ISP_AWBM_ITEM * 4);
	} else {
		ISP_LOGE("copy awbm info error.");
	}

	if(awbm_statistics) {
		free(awbm_statistics);
		awbm_statistics = NULL;
	}
	return ret;
}

isp_s32 isp_u_awbm_bypass(isp_handle handle, isp_u32 bypass)
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
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBM_BYPASS;
	param.property_param = &bypass;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_awbm_mode(isp_handle handle, isp_u32 mode)
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
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBM_MODE;
	param.property_param = &mode;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_awbm_skip_num(isp_handle handle, isp_u32 num)
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
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBM_SKIP_NUM;
	param.property_param = &num;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_awbm_block_offset(isp_handle handle, isp_u32 x, isp_u32 y)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_img_offset offset;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBM_BLOCK_OFFSET;
	offset.x = x;
	offset.y = y;
	param.property_param = &offset;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_awbm_block_size(isp_handle handle, isp_u32 w, isp_u32 h)
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
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBM_BLOCK_SIZE;
	size.width = w;
	size.height = h;
	param.property_param = &size;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_awbm_shift(isp_handle handle, isp_u32 shift)
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
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBM_SHIFT;
	param.property_param = &shift;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_awbc_bypass(isp_handle handle, isp_u32 bypass)
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
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBC_BYPASS;
	param.property_param = &bypass;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_awbc_gain(isp_handle handle,
		isp_u32 r,
		isp_u32 g,
		isp_u32 b)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_awbc_rgb gain;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBC_GAIN;
	gain.r = r;
	gain.g = g;
	gain.b = b;
	param.property_param = &gain;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_awbc_thrd(isp_handle handle,
		isp_u32 r,
		isp_u32 g,
		isp_u32 b)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_awbc_rgb thrd;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBC_THRD;
	thrd.r = r;
	thrd.g = g;
	thrd.b = b;
	param.property_param = &thrd;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_awbc_gain_offset(isp_handle handle,
		isp_u32 r,
		isp_u32 g,
		isp_u32 b)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_awbc_rgb offset;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AWB;
	param.property = ISP_PRO_AWBC_GAIN_OFFSET;
	offset.r = r;
	offset.g = g;
	offset.b = b;
	param.property_param = &offset;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}
