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

#define LOG_TAG "isp_u_comm"

#include "isp_drv.h"

isp_s32 isp_u_comm_start(isp_handle handle, isp_u32 start)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_START;
	param.property_param = &start;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_in_mode(isp_handle handle, isp_u32 mode)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_IN_MODE;
	param.property_param = &mode;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_out_mode(isp_handle handle, isp_u32 mode)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_OUT_MODE;
	param.property_param = &mode;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_fetch_endian(isp_handle handle,
		isp_u32 endian,
		isp_u32 bit_reorder)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_fetch_endian fetch_endian;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_FETCH_ENDIAN;
	fetch_endian.endian = endian;
	fetch_endian.bit_recorder = bit_reorder;
	param.property_param = &fetch_endian;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_bpc_endian(isp_handle handle, isp_u32 endian)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_BPC_ENDIAN;
	param.property_param = &endian;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_store_endian(isp_handle handle, isp_u32 endian)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_STORE_ENDIAN;
	param.property_param = &endian;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_fetch_data_format(isp_handle handle, isp_u32 format)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_FETCH_DATA_FORMAT;
	param.property_param = &format;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_store_format(isp_handle handle, isp_u32 format)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_STORE_FORMAT;
	param.property_param = &format;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_burst_size(isp_handle handle, isp_u16 burst_size)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_BURST_SIZE;
	param.property_param = &burst_size;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_mem_switch(isp_handle handle, isp_u8 mem_switch)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_MEM_SWITCH;
	param.property_param = &mem_switch;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_shadow(isp_handle handle, isp_u32 shadow)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_SHADOW;
	param.property_param = &shadow;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_shadow_all(isp_handle handle, isp_u8 shadow)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_SHADOW_ALL;
	param.property_param = &shadow;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_bayer_mode(isp_handle handle,
		isp_u32 nlc_bayer,
		isp_u32 awbc_bayer,
		isp_u32 wave_bayer,
		isp_u32 cfa_bayer,
		isp_u32 gain_bayer)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_bayer_mode mode;
	struct isp_io_param param;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_BAYER_MODE;
	mode.nlc_bayer = nlc_bayer;
	mode.awbc_bayer = awbc_bayer;
	mode.wave_bayer = wave_bayer;
	mode.cfa_bayer = cfa_bayer;
	mode.gain_bayer = gain_bayer;
	param.property_param = &mode;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_int_clear(isp_handle handle, isp_u32 int_num)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_INT_CLEAR;
	param.property_param = &int_num;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_get_int_raw(isp_handle handle, isp_u32 *raw)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_GET_INT_RAW;
	param.property_param = raw;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_pmu_raw_mask(isp_handle handle, isp_u8 raw_mask)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_PMU_RAW_MASK;
	param.property_param = &raw_mask;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_hw_mask(isp_handle handle, isp_u32 hw_logic)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_HW_MASK;
	param.property_param = &hw_logic;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_hw_enable(isp_handle handle, isp_u32 hw_logic)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_HW_ENABLE;
	param.property_param = &hw_logic;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_pmu_pmu_sel(isp_handle handle, isp_u8 sel)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_PMU_SEL;
	param.property_param = &sel;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_sw_enable(isp_handle handle, isp_u32 sw_logic)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_SW_ENABLE;
	param.property_param = &sw_logic;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_preview_stop(isp_handle handle, isp_u8 eb)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_PREVIEW_STOP;
	param.property_param = &eb;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_set_shadow_control(isp_handle handle, isp_u32 control)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_SET_SHADOW_CONTROL;
	param.property_param = &control;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_shadow_control_clear(isp_handle handle, isp_u8 eb)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_SHADOW_CONTROL_CLEAR;
	param.property_param = &eb;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_axi_stop(isp_handle handle, isp_u8 eb)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_AXI_STOP;
	param.property_param = &eb;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_slice_cnt_enable(isp_handle handle, isp_u8 eb)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_SLICE_CNT_ENABLE;
	param.property_param = &eb;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_preform_cnt_enable(isp_handle handle, isp_u8 eb)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_PREFORM_CNT_ENABLE;
	param.property_param = &eb;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_set_slice_num(isp_handle handle, isp_u8 num)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_SET_SLICE_NUM;
	param.property_param = &num;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_get_slice_num(isp_handle handle, isp_u8 *slice_num)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_GET_SLICE_NUM;
	param.property_param = slice_num;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_perform_cnt_rstatus(isp_handle handle, isp_u32 *status)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_PERFORM_CNT_RSTATUS;
	param.property_param = status;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_preform_cnt_status(isp_handle handle, isp_u32 *status)
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
	param.sub_block = ISP_BLOCK_COMMON;
	param.property = ISP_PRO_COMMON_PERFORM_CNT_STATUS;
	param.property_param = status;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}
