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

#define LOG_TAG "isp_u_raw_aem"

#include "isp_drv.h"

isp_s32 isp_u_raw_aem_block(isp_handle handle, void *block_info)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("handle is null error: 0x%lx 0x%lx",
				(isp_uint)handle, (isp_uint)block_info);
		return -1;
	}

	if (!block_info) {
		ISP_LOGE("block info is null");
		return ret;
	}

	if(((struct isp_dev_raw_aem_info*)block_info)->bypass > 1){
		return ret;

	}
	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_RAW_AEM;
	param.property = ISP_PRO_RAW_AEM_BLOCK;
	param.property_param = block_info;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_aem_bypass(isp_handle handle, void *block_info)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle || !block_info) {
		ISP_LOGE("handle is null error: 0x%lx x%lx",
				(isp_uint)handle, (isp_uint)block_info);
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_RAW_AEM;
	param.property = ISP_PRO_RAW_AEM_BYPASS;
	param.property_param = block_info;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_aem_mode(isp_handle handle, isp_u32 mode)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("handle is null error: 0x%lx",
				(isp_uint)handle);
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_RAW_AEM;
	param.property = ISP_PRO_RAW_AEM_MODE;
	param.property_param = &mode;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_aem_statistics(isp_handle handle,
		isp_u32 *r_info,
		isp_u32 *g_info,
		isp_u32 *b_info)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_raw_aem_statistics *aem_statistics = NULL;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	if (!r_info || !g_info || !b_info) {
		ISP_LOGE("data ptr is null error: 0x%lx 0x%lx 0x%lx",
			(isp_uint)r_info, (isp_uint)g_info, (isp_uint)b_info);
		return -1;
	}

	aem_statistics = (struct isp_raw_aem_statistics *)malloc(sizeof(struct isp_raw_aem_statistics));
	if (!aem_statistics) {
		ISP_LOGE("NO mem");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_RAW_AEM;
	param.property = ISP_PRO_RAW_AEM_STATISTICS;
	memset(aem_statistics, 0x00, sizeof(struct isp_raw_aem_statistics));
	param.property_param = aem_statistics;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (0 == ret) {
		memcpy((void *)r_info, (void *)(aem_statistics->r), ISP_RAW_AEM_ITEM * 4);
		memcpy((void *)g_info, (void *)(aem_statistics->g), ISP_RAW_AEM_ITEM * 4);
		memcpy((void *)b_info, (void *)(aem_statistics->b), ISP_RAW_AEM_ITEM * 4);
	} else {
		ISP_LOGE("copy aem info error.");
	}

	if (aem_statistics) {
		free(aem_statistics);
		aem_statistics = NULL;
	}

	return ret;
}

isp_s32 isp_u_raw_aem_skip_num(isp_handle handle, isp_u32 skip_num)
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
	param.sub_block = ISP_BLOCK_RAW_AEM;
	param.property = ISP_PRO_RAW_AEM_SKIP_NUM;
	param.property_param = &skip_num;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_aem_shift(isp_handle handle, isp_u32 shift)
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
	param.sub_block = ISP_BLOCK_RAW_AEM;
	param.property = ISP_PRO_RAW_AEM_SHIFT;
	param.property_param = &shift;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_aem_offset(isp_handle handle, isp_u32 x, isp_u32 y)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct img_offset offset;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_RAW_AEM;
	param.property = ISP_PRO_RAW_AEM_OFFSET;
	offset.x = x;
	offset.y = y;
	param.property_param = &offset;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_aem_blk_size(isp_handle handle, isp_u32 width, isp_u32 height)
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
	param.sub_block = ISP_BLOCK_RAW_AEM;
	param.property = ISP_PRO_RAW_AEM_BLK_SIZE;
	size.width = width;
	size.height = height;
	param.property_param = &size;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_aem_slice_size(isp_handle handle, isp_u32 width, isp_u32 height)
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
	param.sub_block = ISP_BLOCK_RAW_AEM;
	param.property = ISP_PRO_RAW_AEM_SLICE_SIZE;
	size.width = width;
	size.height = height;
	param.property_param = &size;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}
