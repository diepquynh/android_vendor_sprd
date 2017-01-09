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

#define LOG_TAG "cmr_rotate"

#include <fcntl.h>
#include <sys/ioctl.h>
#include "cmr_type.h"
#include "cmr_cvt.h"
#include "sprd_rot_k.h"

static cmr_s8 rot_dev_name[50] = "/dev/sprd_rotation";

struct rot_file{
	cmr_int            fd;
};

static ROT_DATA_FORMAT_E cmr_rot_fmt_cvt(cmr_u32 cmr_fmt)
{
	ROT_DATA_FORMAT_E fmt = ROT_FMT_MAX;

	switch (cmr_fmt) {
	case IMG_DATA_TYPE_YUV422:
		fmt = ROT_YUV422;
		break;

	case IMG_DATA_TYPE_YUV420:
	case IMG_DATA_TYPE_YVU420:
		fmt = ROT_YUV420;
		break;

	case IMG_DATA_TYPE_RGB565:
		fmt = ROT_RGB565;
		break;

	case IMG_DATA_TYPE_RGB888:
		fmt = ROT_RGB888;
		break;

	default:
		break;
	}

	return fmt;
}

cmr_int cmr_rot_open(cmr_handle *rot_handle)
{
	cmr_int                 ret = CMR_CAMERA_SUCCESS;
	struct rot_file         *file = NULL;
	cmr_int                 fd = -1;

	file = malloc(sizeof(struct rot_file));
	if (!file) {
		ret = - CMR_CAMERA_FAIL;
		goto open_out;
	}

	fd = open(rot_dev_name, O_RDWR, 0);
	if (fd < 0) {
		CMR_LOGE("Fail to open rotation device.");
		goto rot_free;
	}

	file->fd = fd;

	*rot_handle = (cmr_handle)file;
	goto open_out;

rot_free:
	if (file)
		free(file);
	file = NULL;
open_out:

	return ret;
}

cmr_int cmr_rot(struct cmr_rot_param *rot_param)
{
	struct _rot_cfg_tag     rot_cfg;
	cmr_int                 ret = CMR_CAMERA_SUCCESS;
	enum img_angle          angle;
	struct img_frm          *src_img;
	struct img_frm          *dst_img;
	cmr_int                 fd;
	struct rot_file         *file = NULL;

	if (!rot_param) {
		CMR_LOGE("Invalid Param!");
		ret = -CMR_CAMERA_FAIL;
		goto rot_exit;
	}

	file = (struct rot_file*)rot_param->handle;
	if (!file) {
		CMR_LOGE("Invalid Param rot_file !");
		ret = -CMR_CAMERA_FAIL;
		goto rot_exit;
	}

	fd = file->fd;
	if (fd < 0) {
		CMR_LOGE("Invalid Param handle!");
		ret = -CMR_CAMERA_FAIL;
		goto rot_exit;
	}

	angle = rot_param->angle;
	src_img =&rot_param->src_img;
	dst_img =&rot_param->dst_img;

	if (NULL == src_img || NULL == dst_img) {
		CMR_LOGE("Wrong parameter 0x%lx 0x%lx", (cmr_uint)src_img, (cmr_uint)dst_img);
		ret = -CMR_CAMERA_FAIL;
		goto rot_exit;
	}

	CMR_LOGI("angle %ld, src 0x%lx 0x%lx, w h %ld %ld, dst 0x%lx 0x%lx",
		(cmr_int)angle,
		src_img->addr_phy.addr_y,
		src_img->addr_phy.addr_u,
		(cmr_int)src_img->size.width,
		(cmr_int)src_img->size.height,
		dst_img->addr_phy.addr_y,
		dst_img->addr_phy.addr_u);

	if ((cmr_u32)angle < (cmr_u32)(IMG_ANGLE_90)) {
		CMR_LOGE("Wrong angle %ld", (cmr_int)angle);
		ret = -CMR_CAMERA_FAIL;
		goto rot_exit;
	}

	rot_cfg.format = cmr_rot_fmt_cvt(src_img->fmt);
	if (rot_cfg.format >= ROT_FMT_MAX) {
		CMR_LOGE("Unsupported format %d, %d", src_img->fmt, rot_cfg.format);
		ret = -CMR_CAMERA_FAIL;
		goto rot_exit;
	}

	rot_cfg.angle = angle - IMG_ANGLE_90 + ROT_90;
	rot_cfg.src_addr.y_addr = (uint32_t)src_img->addr_phy.addr_y;
	rot_cfg.src_addr.u_addr = (uint32_t)src_img->addr_phy.addr_u;
	rot_cfg.src_addr.v_addr = (uint32_t)src_img->addr_phy.addr_v;
	rot_cfg.dst_addr.y_addr = (uint32_t)dst_img->addr_phy.addr_y;
	rot_cfg.dst_addr.u_addr = (uint32_t)dst_img->addr_phy.addr_u;
	rot_cfg.dst_addr.v_addr = (uint32_t)dst_img->addr_phy.addr_v;
	rot_cfg.img_size.w = (cmr_u16)src_img->size.width;
	rot_cfg.img_size.h = (cmr_u16)src_img->size.height;
	rot_cfg.src_endian = src_img->data_end.uv_endian;
	rot_cfg.dst_endian = dst_img->data_end.uv_endian;

	ret = ioctl(fd, ROT_IO_START, &rot_cfg);
	if (ret) {
		CMR_LOGE("src y=%x u=%x v=%x", rot_cfg.src_addr.y_addr, rot_cfg.src_addr.u_addr, rot_cfg.src_addr.v_addr);
		CMR_LOGE("dst y=%x u=%x v=%x", rot_cfg.dst_addr.y_addr, rot_cfg.dst_addr.u_addr, rot_cfg.dst_addr.v_addr);
		CMR_LOGE("Unsupported format %d, %d", src_img->fmt, rot_cfg.format);
		ret = -CMR_CAMERA_FAIL;
		goto rot_exit;
	}

rot_exit:
	CMR_LOGI("X ret=%ld", ret);
	return ret;
}

cmr_int cmr_rot_close(cmr_handle rot_handle)
{
	cmr_int                 ret = CMR_CAMERA_SUCCESS;
	struct rot_file         *file = (struct rot_file*)(rot_handle);

	CMR_LOGI("Start to close rotation device.");

	if (!file)
		goto out;

	if (-1 == file->fd) {
		CMR_LOGE("Invalid fd");
		ret = -CMR_CAMERA_FAIL;
		goto close_free;
	}

	close(file->fd);

close_free:
	free(file);
out:

	CMR_LOGI("ret=%ld",ret);
	return ret;
}
