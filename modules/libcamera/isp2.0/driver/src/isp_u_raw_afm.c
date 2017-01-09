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

#define LOG_TAG "isp_u_raw_afm"

#include "isp_drv.h"

isp_s32 isp_u_raw_afm_block(isp_handle handle, void *block_info)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle || !block_info) {
		ISP_LOGE("handle is null error: 0x%p 0x%p", handle, block_info);
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_BLOCK;
	param.property_param = block_info;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_afm_slice_size(isp_handle handle, isp_u32 width, isp_u32 height)
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
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_FRAME_SIZE;
	size.width = width;
	size.height = height;
	param.property_param = &size;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_afm_type1_statistic(isp_handle handle, void *statis)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle || !statis) {
		ISP_LOGE("handle is null error: 0x%p 0x%p", handle, statis);
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_TYPE1_STATISTIC;
	param.property_param = statis;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_afm_type2_statistic(isp_handle handle, void *statis)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle || !statis) {
		ISP_LOGE("handle is null error: 0x%p 0x%p", handle, statis);
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_TYPE2_STATISTIC;
	param.property_param = statis;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_afm_statistic_r6p9(isp_handle handle, void *statis)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle || !statis) {
		ISP_LOGE("handle is null error: 0x%p 0x%p", handle, statis);
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_STATISTIC;
	param.property_param = statis;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_afm_bypass(isp_handle handle, isp_u32 bypass)
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
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_BYPASS;
	param.property_param = &bypass;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_afm_mode(isp_handle handle, isp_u32 mode)
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
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_MODE;
	param.property_param = &mode;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_afm_skip_num(isp_handle handle, isp_u32 skip_num)
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
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_SKIP_NUM;
	param.property_param = &skip_num;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_afm_skip_num_clr(isp_handle handle, isp_u32 clear)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file*)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_SKIP_NUM_CLR;
	param.property_param = &clear;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_afm_spsmd_rtgbot_enable(isp_handle handle, isp_u32 enable)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file*)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_SPSMD_RTGBOT_ENABLE;
	param.property_param = &enable;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_afm_spsmd_diagonal_enable(isp_handle handle, isp_u32 enable)
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
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_SPSMD_DIAGONAL_ENABLE;
	param.property_param = &enable;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_afm_spsmd_cal_mode(isp_handle handle, isp_u32 mode)
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
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_SPSMD_CAL_MOD;
	param.property_param = &mode;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_afm_spsmd_square_en(isp_handle handle, isp_u32 en)
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
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_SPSMD_SQUARE_ENABLE;
	param.property_param = &en;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_afm_sel_filter1(isp_handle handle, isp_u32 sel_filter)
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
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_SEL_FILTER1;
	param.property_param = &sel_filter;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_afm_sel_filter2(isp_handle handle, isp_u32 sel_filter)
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
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_SEL_FILTER2;
	param.property_param = &sel_filter;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_afm_sobel_type(isp_handle handle, isp_u32 type)
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
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_SOBEL_TYPE;
	param.property_param = &type;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_afm_overflow_protect(isp_handle handle, isp_u32 en)
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
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_OVERFLOW_PROTECT;
	param.property_param = &en;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_afm_spsmd_type(isp_handle handle, isp_u32 type)
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
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_SPSMD_TYPE;
	param.property_param = &type;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_afm_subfilter(isp_handle handle, isp_u32 average, isp_u32 median)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct afm_subfilter subfilter;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	subfilter.average = average;
	subfilter.median = median;
	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_SUBFILTER;
	param.property_param = &subfilter;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_afm_spsmd_touch_mode(isp_handle handle, isp_u32 mode)
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
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_SPSMD_TOUCH_MODE;
	param.property_param = &mode;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_afm_shfit(isp_handle handle, isp_u32 spsmd, isp_u32 sobel5, isp_u32 sobel9)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct afm_shift shift;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	shift.shift_spsmd = spsmd;
	shift.shift_sobel5= sobel5;
	shift.shift_sobel9 = sobel9;
	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_SHFIT;
	param.property_param = &shift;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_afm_sobel_threshold(isp_handle handle, isp_u32 min, isp_u32 max)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct sobel_thrd thrd;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	thrd.min = min;
	thrd.max = max;
	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_SOBEL_THRESHOLD;
	param.property_param = &thrd;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_afm_spsmd_threshold(isp_handle handle, isp_u32 min, isp_u32 max)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct spsmd_thrd thrd;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	thrd.min = min;
	thrd.max = max;
	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_SPSMD_THRESHOLD;
	param.property_param = &thrd;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_afm_threshold_rgb(isp_handle handle, void *thr_rgb)
{
	isp_s32 ret = 0;
	isp_u32 num = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_THRESHOLD_RGB;
	param.property_param = thr_rgb;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_afm_win(isp_handle handle, void *win_range)
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
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_WIN;
	param.property_param = win_range;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_raw_afm_win_num(isp_handle handle, isp_u32 *win_num)
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
	param.sub_block = ISP_BLOCK_AFM_V1;
	param.property = ISP_PRO_RGB_AFM_WIN_NUM;
	param.property_param = win_num;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}
