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

#define LOG_TAG "cmr_ipm"

#include "cmr_ipm.h"
#include "cmr_msg.h"

#ifdef CONFIG_CAMERA_HDR_CAPTURE
extern struct class_tab_t hdr_tab_info;
#endif
#ifdef CONFIG_CAMERA_FACE_DETECT
extern struct class_tab_t fd_tab_info;
#endif
#ifdef CONFIG_CAMERA_UV_DENOISE
extern struct class_tab_t uvde_tab_info;
#endif
#ifdef CONFIG_CAMERA_Y_DENOISE
extern struct class_tab_t yde_tab_info;
#endif
#ifdef CONFIG_CAMERA_SNR_UV_DENOISE
extern struct class_tab_t snr_uvde_tab_info;
#endif

struct ipm_class_tab class_type_tab[] =
{
	{IPM_TYPE_NONE,                NULL},
#ifdef CONFIG_CAMERA_HDR_CAPTURE
	{IPM_TYPE_HDR,                 &hdr_tab_info},
#endif
#ifdef CONFIG_CAMERA_FACE_DETECT
	{IPM_TYPE_FD,                  &fd_tab_info},
#endif
#ifdef CONFIG_CAMERA_UV_DENOISE
	{IPM_TYPE_UVDE,                &uvde_tab_info},
#endif
#ifdef CONFIG_CAMERA_Y_DENOISE
	{IPM_TYPE_YDE,                 &yde_tab_info},
#endif
#ifdef CONFIG_CAMERA_SNR_UV_DENOISE
	{IPM_TYPE_SNR_UVDE,                 &snr_uvde_tab_info},
#endif

};

#define CHECK_HANDLE_VALID(handle) \
	do { \
		if (!handle) { \
			return CMR_CAMERA_INVALID_PARAM; \
		} \
	} while(0)

cmr_int cmr_ipm_init(struct ipm_init_in *in, cmr_handle *ipm_handle)
{
	cmr_int                ret = CMR_CAMERA_SUCCESS;
	struct ipm_context_t   *handle;

	if (!in || !ipm_handle) {
		CMR_LOGE("Invalid Param!");
		return CMR_CAMERA_INVALID_PARAM;
	}

	handle = (struct ipm_context_t *)malloc(sizeof(struct ipm_context_t));
	if (!handle) {
		CMR_LOGE("No mem!");
		return CMR_CAMERA_NO_MEM;
	}

	cmr_bzero(handle, sizeof(struct ipm_context_t));

	handle->init_in = *in;

	*ipm_handle = (cmr_handle)handle;

	return ret;
}

cmr_int cmr_ipm_deinit(cmr_handle ipm_handle)
{
	cmr_int               ret     = CMR_CAMERA_SUCCESS;
	struct ipm_context_t  *handle = (struct ipm_context_t *)ipm_handle;

	CHECK_HANDLE_VALID(handle);

	free(handle);

	return ret;
}

cmr_int cmr_ipm_open(cmr_handle ipm_handle, cmr_uint class_type,struct ipm_open_in *in,
			struct ipm_open_out *out, cmr_handle *ipm_class_handle)
{
	cmr_int              ret = CMR_CAMERA_SUCCESS;
	cmr_int              index = 0;
	cmr_int              class_type_max;

	if (!ipm_handle || !ipm_class_handle) {
		CMR_LOGE("Invalid Param!");
		return CMR_CAMERA_INVALID_PARAM;
	}

	class_type_max = cmr_array_size(class_type_tab);
	for (index = 0; index < class_type_max; index++) {
		if (class_type_tab[index].class_type == class_type)
			break;
	}
	if ((index == 0) || (index >= class_type_max)) {
		CMR_LOGE("index = %ld class_type does't has relevant info", index);
		return CMR_CAMERA_INVALID_PARAM;
	}
	if (!class_type_tab[index].hdr_tab_info->ops->open) {
		CMR_LOGE("Invalid ops Param!");
		return CMR_CAMERA_INVALID_PARAM;
	}

	ret = class_type_tab[index].hdr_tab_info->ops->open(ipm_handle,in,out,ipm_class_handle);

	return ret;
}
cmr_int cmr_ipm_close(cmr_handle ipm_class_handle)
{
	cmr_int              ret             = CMR_CAMERA_SUCCESS;
	struct ipm_common    *common_handle  = (struct ipm_common *)ipm_class_handle;

	CHECK_HANDLE_VALID(common_handle);

	if (!common_handle->ops->close) {
		CMR_LOGE("Invalid ops Param!");
		return CMR_CAMERA_INVALID_PARAM;
	}

	ret = common_handle->ops->close(ipm_class_handle);

	return ret;
}

cmr_int ipm_transfer_frame(cmr_handle ipm_class_handle, struct ipm_frame_in *in, struct ipm_frame_out *out)
{
	cmr_int              ret             = CMR_CAMERA_SUCCESS;
	struct ipm_common    *common_handle  = (struct ipm_common *)ipm_class_handle;
	cmr_int              index;

	CHECK_HANDLE_VALID(common_handle);

	if (!common_handle->ops->transfer_frame) {
		CMR_LOGE("Invalid ops Param!");
		return CMR_CAMERA_INVALID_PARAM;
	}

	ret = common_handle->ops->transfer_frame(ipm_class_handle,in,out);

	return ret;
}

cmr_int cmr_ipm_pre_proc(cmr_handle ipm_class_handle)
{
	cmr_int              ret             = CMR_CAMERA_SUCCESS;
	struct ipm_common    *common_handle  = (struct ipm_common *)ipm_class_handle;

	CHECK_HANDLE_VALID(common_handle);

	if (NULL != common_handle->ops->pre_proc)
		ret = common_handle->ops->pre_proc(ipm_class_handle);

	return ret;
}

cmr_int cmr_ipm_post_proc(cmr_handle ipm_class_handle)
{
	cmr_int              ret             = CMR_CAMERA_SUCCESS;
	struct ipm_common    *common_handle  = (struct ipm_common *)ipm_class_handle;

	CHECK_HANDLE_VALID(common_handle);

	if (NULL != common_handle->ops->post_proc)
		ret = common_handle->ops->post_proc(ipm_class_handle);

	return ret;
}

cmr_int cmr_ipm_get_capability (struct ipm_capability *out)
{
	cmr_int              ret  = CMR_CAMERA_SUCCESS;
	cmr_uint 			 i;
	cmr_uint 			 class_type_max;

	class_type_max = cmr_array_size(class_type_tab);
	for (i = 0; i < class_type_max; i++) {
		if (NULL != class_type_tab[i].hdr_tab_info)
			out->class_type_bits |= 1<<(i-1);
	}

	return ret;
}

