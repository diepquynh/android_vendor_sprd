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

#define LOG_TAG "isp_u_css"

#include "isp_drv.h"

isp_s32 isp_u_css_block(isp_handle handle, void *block_info)
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
	param.sub_block = ISP_BLOCK_CSS;
	param.property = ISP_PRO_CSS_BLOCK;
	param.property_param = block_info;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_css_bypass(isp_handle handle, isp_u32 bypass)
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
	param.sub_block = ISP_BLOCK_CSS;
	param.property = ISP_PRO_CSS_BYPASS;
	param.property_param = &bypass;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_css_thrd(isp_handle handle,
		isp_u8 *low_thrd,
		isp_u8 *low_sum_thrd,
		isp_u8 lum_thrd,
		isp_u8 chr_thrd)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_css_thrd thrd;

	if (!handle || !low_thrd || !low_sum_thrd) {
		ISP_LOGE("handle is null error: 0x%lx 0x%lx 0x%lx",
				(isp_uint)handle,
				(isp_uint)low_thrd,
				(isp_uint)low_sum_thrd);
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_CSS;
	param.property = ISP_PRO_CSS_THRD;
	memset(&thrd, 0x00, sizeof(thrd));
	memcpy((void *)(thrd.lower_thrd), (void *)low_thrd, sizeof(thrd.lower_thrd));
	memcpy((void *)(thrd.lower_sum_thrd), (void *)low_sum_thrd, sizeof(thrd.lower_sum_thrd));
	thrd.luma_thrd = lum_thrd;
	thrd.chroma_thrd = chr_thrd;
	param.property_param = &thrd;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_css_slice_size(isp_handle handle, isp_u32 w, isp_u32 h)
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
	param.sub_block = ISP_BLOCK_CSS;
	param.property = ISP_PRO_CSS_SLICE_SIZE;
	size.width = w;
	size.height = h;
	param.property_param = &size;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_css_ratio(isp_handle handle, isp_u8 *ratio)
{
	isp_s32 ret = 0;
	isp_u32 val = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_css_ratio css_ratio;

	if (!handle || !ratio) {
		ISP_LOGE("handle is null error: 0x%lx 0x%lx",
				(isp_uint)handle, (isp_uint)ratio);
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_CSS;
	param.property = ISP_PRO_CSS_RATIO;
	memset(&css_ratio, 0x00, sizeof(css_ratio));
	memcpy((void *)(css_ratio.ratio), (void *)ratio, sizeof(css_ratio.ratio));
	param.property_param = &val;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}
