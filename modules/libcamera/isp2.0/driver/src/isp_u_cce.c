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

#define LOG_TAG "isp_u_cce"

#include "isp_drv.h"


isp_s32 isp_u_cce_matrix_block(isp_handle handle, void *block_info)
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
	param.sub_block = ISP_BLOCK_CCE;
	param.property = ISP_PRO_CCE_BLOCK_MATRIX;
	param.property_param = block_info;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_cce_uv_block(isp_handle handle, void *block_info)
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
	param.sub_block = ISP_BLOCK_CCE;
	param.property = ISP_PRO_CCE_BLOCK_UV;
	param.property_param = block_info;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_cce_uvdivision_bypass(isp_handle handle, isp_u32 bypass)
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
	param.sub_block = ISP_BLOCK_CCE;
	param.property = ISP_PRO_CCE_UVDIVISION_BYPASS;
	param.property_param = &bypass;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_cce_mode(isp_handle handle, isp_u32 mode)
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
	param.sub_block = ISP_BLOCK_CCE;
	param.property = ISP_PRO_CCE_MODE;
	param.property_param = &mode;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_cce_matrix(isp_handle handle, isp_u16 *matrix_ptr)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_cce_matrix_tab matrix_tab;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_CCE;
	param.property = ISP_PRO_CCE_MATRIX;
	memset(&matrix_tab, 0x00, sizeof(matrix_tab));
	memcpy(&(matrix_tab.matrix), matrix_ptr, sizeof(matrix_tab.matrix));
	param.property_param = &matrix_tab;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_cce_shift(isp_handle handle,
		isp_u32 y_shift,
		isp_u32 u_shift,
		isp_u32 v_shift)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_cce_shift cce_shift;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_CCE;
	param.property = ISP_PRO_CCE_SHIFT;
	memset(&(cce_shift), 0x00, sizeof(cce_shift));
	cce_shift.y_shift = y_shift;
	cce_shift.u_shift = u_shift;
	cce_shift.v_shift = v_shift;
	param.property_param = &cce_shift;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_cce_uvd(isp_handle handle, isp_u8 *div_ptr)
{
	isp_s32 ret = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_cce_uvd cce_uvd;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_CCE;
	param.property = ISP_PRO_CCE_UVD_THRD;
	memset(&cce_uvd, 0x00, sizeof(cce_uvd));
	memcpy(&(cce_uvd.uvd), div_ptr, sizeof(cce_uvd.uvd));
	param.property_param = &cce_uvd;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}

isp_s32 isp_u_cce_uvc(isp_handle handle, isp_u8 *t_ptr, isp_u8 *m_ptr)
{
	isp_s32 ret = 0;
	isp_u32 val = 0;
	struct isp_file *file = NULL;
	struct isp_io_param param;
	struct isp_cce_uvc cce_uvc;

	if (!handle) {
		ISP_LOGE("handle is null error.");
		return -1;
	}

	file = (struct isp_file *)(handle);
	param.isp_id = file->isp_id;
	param.sub_block = ISP_BLOCK_CCE;
	param.property = ISP_PRO_CCE_UVC_PARAM;
	memset(&cce_uvc, 0x00, sizeof(cce_uvc));
	memcpy(&(cce_uvc.uvc0), t_ptr, sizeof(cce_uvc.uvc0));
	memcpy(&(cce_uvc.uvc1), m_ptr, sizeof(cce_uvc.uvc1));
	param.property_param = &cce_uvc;

	ret = ioctl(file->fd, ISP_IO_CFG_PARAM, &param);

	return ret;
}
