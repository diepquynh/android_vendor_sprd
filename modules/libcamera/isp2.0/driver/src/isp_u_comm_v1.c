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

isp_s32 isp_u_comm_block(isp_handle handle, void *block_info)
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
	param.sub_block = ISP_BLOCK_COMMON_V1;
	param.property = ISP_PRO_COMMON_BLOCK;
	param.property_param = block_info;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_shadow_ctrl_all(isp_handle handle, isp_u32 auto_shadow)
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
	param.sub_block = ISP_BLOCK_COMMON_V1;
	param.property = ISP_PRO_COMMON_SHADOW_ALL_CTRL;
	param.property_param = &auto_shadow;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_awbm_shadow_ctrl(isp_handle handle, isp_u32 shadow_done)
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
	param.sub_block = ISP_BLOCK_COMMON_V1;
	param.property = ISP_PRO_COMMON_AWBM_SHADOW;
	param.property_param = &shadow_done;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_ae_shadow_ctrl(isp_handle handle, isp_u32 shadow_done)
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
	param.sub_block = ISP_BLOCK_COMMON_V1;
	param.property = ISP_PRO_COMMON_AE_SHADOW;
	param.property_param = &shadow_done;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_af_shadow_ctrl(isp_handle handle, isp_u32 shadow_done)
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
	param.sub_block = ISP_BLOCK_COMMON_V1;
	param.property = ISP_PRO_COMMON_AF_SHADOW;
	param.property_param = &shadow_done;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_afl_shadow_ctrl(isp_handle handle, isp_u32 shadow_done)
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
	param.sub_block = ISP_BLOCK_COMMON_V1;
	param.property = ISP_PRO_COMMON_AFL_SHADOW;
	param.property_param = &shadow_done;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_shadow_ctrl(isp_handle handle, isp_u32 shadow_done)
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
	param.sub_block = ISP_BLOCK_COMMON_V1;
	param.property = ISP_PRO_COMMON_COMM_SHADOW;
	param.property_param = &shadow_done;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_3a_ctrl(isp_handle handle, isp_u32 enable)
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
	param.sub_block = ISP_BLOCK_COMMON_V1;
	param.property = ISP_PRO_COMMON_3A_SINGLE_FRAME_CTRL;
	param.property_param = &enable;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_channel0_y_aem_pos(isp_handle handle, isp_u32 pos)
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
	param.sub_block = ISP_BLOCK_COMMON_V1;
	param.property = ISP_PRO_COMMON_CH0_AEM2_POS;
	param.property_param = &pos;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_comm_channel1_y_aem_pos(isp_handle handle, isp_u32 pos)
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
	param.sub_block = ISP_BLOCK_COMMON_V1;
	param.property = ISP_PRO_COMMON_CH1_AEM2_POS;
	param.property_param = &pos;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}


