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

#define LOG_TAG "isp_u_afm"

#include "isp_drv.h"

isp_s32 isp_u_afm_block(isp_handle handle, void *block_info)
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
	param.sub_block = ISP_BLOCK_AFM;
	param.property = ISP_PRO_AFM_BLOCK;
	param.property_param = block_info;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_afm_bypass(isp_handle handle, isp_u32 bypass)
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
	param.sub_block = ISP_BLOCK_AFM;
	param.property = ISP_PRO_AFM_BYPASS;
	param.property_param = &bypass;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_afm_shift(isp_handle handle, isp_u32 shift)
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
	param.sub_block = ISP_BLOCK_AFM;
	param.property = ISP_PRO_AFM_SHIFT;
	param.property_param = &shift;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_afm_mode(isp_handle handle, isp_u32 mode)
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
	param.sub_block = ISP_BLOCK_AFM;
	param.property = ISP_PRO_AFM_MODE;
	param.property_param = &mode;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_afm_skip_num(isp_handle handle, isp_u32 skip_num)
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
	param.sub_block = ISP_BLOCK_AFM;
	param.property = ISP_PRO_AFM_SKIP_NUM;
	param.property_param = &skip_num;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_afm_skip_num_clr(isp_handle handle, isp_u32 is_clear)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("handle is null error");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AFM;
	param.property = ISP_PRO_AFM_SKIP_NUM_CLR;
	param.property_param = &is_clear;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_afm_win(isp_handle handle, void *win_rangs)
{
	isp_s32 ret = 0;
	isp_u32 num = 0;
	struct isp_file *file = NULL;
	struct isp_img_coord *win = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}
	isp_u_afm_win_num(handle, &num);
	if (0 == num) {
		ISP_LOGW("warning: num is 0.");
		ret = -1;
		goto out;
	}
	win = calloc(num, sizeof(*win));
	if (!win) {
		ISP_LOGE("no memory error");
		ret = -1;
		goto out;
	}
	memcpy(win, win_rangs, num * sizeof(*win));

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AFM;
	param.property = ISP_PRO_AFM_WIN;
	param.property_param = win;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

out:
	return ret;
}

isp_s32 isp_u_afm_statistic(isp_handle handle, void *out_statistic)
{
	isp_s32 ret = 0;
	isp_u32 num = 0;
	struct isp_file *file = NULL;
	struct isp_afm_statistic *statis = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}
	isp_u_afm_win_num(handle, &num);
	if (0 == num) {
		ISP_LOGW("warning: num is 0.");
		ret = -1;
		goto out;
	}
	statis = calloc(num, sizeof(*statis));
	if (!statis) {
		ISP_LOGE("no memory error");
		ret = -1;
		goto out;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AFM;
	param.property = ISP_PRO_AFM_STATISTIC;
	param.property_param = statis;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);
	if (0 == ret) {
		memcpy(out_statistic, statis, num * sizeof(*statis));
	}
out:
	return ret;
}

isp_s32 isp_u_afm_win_num(isp_handle handle, isp_u32 *win_num)
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
	param.sub_block = ISP_BLOCK_AFM;
	param.property = ISP_PRO_AFM_WIN_NUM;
	param.property_param = win_num;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}
