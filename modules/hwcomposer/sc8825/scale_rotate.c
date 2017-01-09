/*
 * Copyright (C) 2010 The Android Open Source Project
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
#include <fcntl.h>
#include <sys/ioctl.h>
#include <cutils/log.h>
#include "sprd_rot_k.h"
#include "scale_rotate.h"
#include "img_scale_u.h"
#include "cmr_common.h"
#include "graphics.h"
#define HWCOMPOSER_EXIT_IF_ERR(n)                      \
	do {                                                                 \
		if (n) {                                           \
			ALOGE("hwcomposer  rotate or scale  error  . Line:%d ", __LINE__);\
			goto exit;                   \
		}                                                    \
	} while(0)


static ROT_DATA_FORMAT_E rotation_data_format_hw2kernel(HW_ROTATION_DATA_FORMAT_E data_format)
{
	ROT_DATA_FORMAT_E result_format = ROT_FMT_MAX;

	switch(data_format)
	{
		case HW_ROTATION_DATA_YUV422:
			result_format = ROT_YUV422;
			break;
		case HW_ROTATION_DATA_YUV420:
			result_format = ROT_YUV420;
			break;
		case HW_ROTATION_DATA_YUV400:
			result_format = ROT_YUV400;
			break;
		case HW_ROTATION_DATA_RGB888:
			result_format = ROT_RGB888;
			break;
		case HW_ROTATION_DATA_RGB666:
			result_format = ROT_RGB666;
			break;
		case HW_ROTATION_DATA_RGB565:
			result_format = ROT_RGB565;
			break;
		case HW_ROTATION_DATA_RGB555:
			result_format = ROT_RGB555;
			break;
		default:
			result_format = ROT_FMT_MAX;
			break;
	}
	return result_format;
}

static ROT_ANGLE_E rotation_degree_user2kernel(int degree)
{
	ROT_ANGLE_E result_degree = ROT_ANGLE_MAX;

	switch(degree)
	{
		case -1:
			result_degree = ROT_MIRROR;
			break;
		case 90:
			result_degree = ROT_90;
			break;
		case 180:
			result_degree = ROT_180;
			break;
		case 270:
			result_degree = ROT_270;
			break;
		default:
			result_degree = ROT_ANGLE_MAX;
			break;
	}
	return result_degree;
}

static int rotation_degree_hw2user(HW_ROTATION_MODE_E rot)
{
	int result_degree = 0;

	switch(rot)
	{
		case HW_ROTATION_90:
			result_degree = 90;
			break;
		case HW_ROTATION_180:
			result_degree = 180;
			break;
		case HW_ROTATION_270:
			result_degree = 270;
			break;
		case HW_ROTATION_MIRROR:
			result_degree = -1;
			break;
		default:
			result_degree = 0;
			break;
	}
	return result_degree;
}


int camera_rotation_copy_data(uint32_t width, uint32_t height, uint32_t in_addr, uint32_t out_addr)
{
	int ret = 0, rotate_fd = -1;
	struct _rot_cfg_tag      rot_cfg;

	rotate_fd = open("/dev/sprd_rotation", O_RDWR, 0);
	if (-1 == rotate_fd) {
		ALOGE("camera_rotation_copy_data fail : open rotation device. Line:%d ", __LINE__);
		return -1;
	}

	rot_cfg.format = ROT_RGB888;
	rot_cfg.img_size.w = width;
	rot_cfg.img_size.h = height;
	rot_cfg.src_addr.y_addr = in_addr;
	rot_cfg.src_addr.u_addr = rot_cfg.src_addr.y_addr + rot_cfg.img_size.w * rot_cfg.img_size.h;
	rot_cfg.src_addr.v_addr = rot_cfg.src_addr.u_addr;
	rot_cfg.dst_addr.y_addr = out_addr;
	rot_cfg.dst_addr.u_addr = rot_cfg.dst_addr.y_addr + rot_cfg.img_size.w * rot_cfg.img_size.h;
	rot_cfg.dst_addr.v_addr = rot_cfg.dst_addr.u_addr;

	ret = ioctl(rotate_fd, ROT_IO_DATA_COPY, &rot_cfg);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret =  close(rotate_fd);
	if (ret){
		ALOGE("camera_rotation_copy_data fail :close rot_fd 1 . Line:%d ", __LINE__);
		return -1;
	}

exit:
	if (ret) {
		ret =  close(rotate_fd);
		if (ret){
			ALOGE("camera_rotation_copy_data fail :close rot_fd 2 . Line:%d ", __LINE__);
		}
		return -1;
	}else{
		return 0;
	}
}

int camera_rotation_copy_data_from_virtual(uint32_t width, uint32_t height, uint32_t in_virtual_addr, uint32_t out_addr)
{
	int ret = 0, rotate_fd = -1;
	struct _rot_cfg_tag      rot_cfg;

	rotate_fd = open("/dev/sprd_rotation", O_RDWR, 0);
	if (-1 == rotate_fd) {
		ALOGE("Camera_rotation copy from virtual fail : open rotation device. Line:%d ", __LINE__);
		return -1;
	}

	rot_cfg.format = ROT_RGB888;
	rot_cfg.img_size.w = width;
	rot_cfg.img_size.h = height;
	rot_cfg.src_addr.y_addr = in_virtual_addr;
	rot_cfg.src_addr.u_addr = rot_cfg.src_addr.y_addr + rot_cfg.img_size.w * rot_cfg.img_size.h;
	rot_cfg.src_addr.v_addr = rot_cfg.src_addr.u_addr;
	rot_cfg.dst_addr.y_addr = out_addr;
	rot_cfg.dst_addr.u_addr = rot_cfg.dst_addr.y_addr + rot_cfg.img_size.w * rot_cfg.img_size.h;
	rot_cfg.dst_addr.v_addr = rot_cfg.dst_addr.u_addr;

	ret = ioctl(rotate_fd, ROT_IO_DATA_COPY_FROM_VIRTUAL, &rot_cfg);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret =  close(rotate_fd);
	if (ret){
		ALOGE("Camera_rotation copy from virtual fail :close rot_fd 1. Line:%d ", __LINE__);
		return -1;
	}

exit:
	if (ret) {
		ret =  close(rotate_fd);
		if (ret){
			ALOGE("Camera_rotation copy from virtual fail :close rot_fd 2 . Line:%d ", __LINE__);
		}
		return -1;
	}else{
		return 0;
	}
}

int camera_rotation(HW_ROTATION_DATA_FORMAT_E rot_format, int degree, uint32_t width, uint32_t height, uint32_t in_addr, uint32_t out_addr)
{
	int ret = 0, rotate_fd = -1;
	uint32_t param = 0;
	struct _rot_cfg_tag rot_cfg;

	rotate_fd = open("/dev/sprd_rotation", O_RDWR, 0);
	if (-1 == rotate_fd) {
		ALOGE("Camera_rotation fail : open rotation device. Line:%d ", __LINE__);
		return -1;
	}

	rot_cfg.format = rotation_data_format_hw2kernel( rot_format);
	rot_cfg.angle =  rotation_degree_user2kernel(degree);
	rot_cfg.img_size.w = width;
	rot_cfg.img_size.h = height;
	rot_cfg.src_addr.y_addr = in_addr;
	rot_cfg.src_addr.u_addr = rot_cfg.src_addr.y_addr + rot_cfg.img_size.w * rot_cfg.img_size.h;
	rot_cfg.src_addr.v_addr = rot_cfg.src_addr.u_addr;
	rot_cfg.dst_addr.y_addr = out_addr;
	rot_cfg.dst_addr.u_addr = rot_cfg.dst_addr.y_addr + rot_cfg.img_size.w * rot_cfg.img_size.h;
	rot_cfg.dst_addr.v_addr = rot_cfg.dst_addr.u_addr;

	ret = ioctl(rotate_fd, ROT_IO_CFG, &rot_cfg);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret = ioctl(rotate_fd, ROT_IO_START, 1);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret = ioctl(rotate_fd, ROT_IO_IS_DONE, &param);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret =  close(rotate_fd);
	if (ret){
		ALOGE("Camera_rotation fail : close rot_fd 1. Line:%d ", __LINE__);
		return -1;
	}

exit:
	if (ret) {
		ret =  close(rotate_fd);
		if (ret){
			ALOGE("Camera_rotation fail : close rot_fd 2. Line:%d ", __LINE__);
		}
		return -1;
	}else{
		return 0;
	}
}

int camera_scaling(HW_SCALE_DATA_FORMAT_E output_fmt,
	uint32_t output_width, uint32_t output_height,
	uint32_t output_yaddr,uint32_t output_uvaddr,
	HW_SCALE_DATA_FORMAT_E input_fmt,uint32_t input_uv_endian,
	uint32_t input_width,uint32_t input_height,
	uint32_t input_yaddr, uint32_t intput_uvaddr,
	struct sprd_rect *trim_rect, HW_ROTATION_MODE_E rotation, uint32_t tmp_addr)
{
	int ret = 0;
	struct img_frm src_img, dst_img;
	struct img_rect src_rect;
	enum scle_mode scale_mode = SCALE_MODE_NORMAL;
	struct scale_frame scale_frm;

	src_img.fmt = (uint32_t)input_fmt;
	src_img.size.width = input_width;
	src_img.size.height = input_height;
	src_img.addr_phy.addr_y =  input_yaddr;
	src_img.addr_phy.addr_u =  intput_uvaddr;
	src_img.addr_phy.addr_v = intput_uvaddr;
	src_img.data_end.y_endian = 1;
	src_img.data_end.uv_endian = input_uv_endian;

	src_rect.start_x = trim_rect->x;
	src_rect.start_y = trim_rect->y;
	src_rect.width = trim_rect->w;
	src_rect.height = trim_rect->h;

	dst_img.fmt = (uint32_t)output_fmt;
	dst_img.size.width = output_width;
	dst_img.size.height = output_height;
	if(HW_ROTATION_0 == rotation){
		dst_img.addr_phy.addr_y =  output_yaddr;
		dst_img.addr_phy.addr_u =  output_uvaddr;
		dst_img.addr_phy.addr_v = output_uvaddr;
	}else
	{
		dst_img.addr_phy.addr_y =  tmp_addr;
		dst_img.addr_phy.addr_u =  dst_img.addr_phy.addr_y + dst_img.size.width * dst_img.size.height ;
		dst_img.addr_phy.addr_v = dst_img.addr_phy.addr_u;
	}
	dst_img.data_end.y_endian = 1;
	dst_img.data_end.uv_endian = 1;
	int fd = open("/dev/sprd_scale", O_RDONLY);
	if(fd < 0)
	{
		ALOGE("error to open dev sprd_scale");
		goto exit;
	}
	ret = ioctl(fd, SCALE_IO_INPUT_SIZE, &src_img.size);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret = ioctl(fd, SCALE_IO_INPUT_RECT, &src_rect);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret = ioctl(fd, SCALE_IO_INPUT_FORMAT, &src_img.fmt );
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret = ioctl(fd, SCALE_IO_INPUT_ENDIAN, &src_img.data_end);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret = ioctl(fd, SCALE_IO_INPUT_ADDR, &src_img.addr_phy);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret = ioctl(fd, SCALE_IO_OUTPUT_SIZE, &dst_img.size);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret = ioctl(fd, SCALE_IO_OUTPUT_FORMAT, &dst_img.fmt);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret = ioctl(fd, SCALE_IO_OUTPUT_ENDIAN, &dst_img.data_end);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret = ioctl(fd, SCALE_IO_OUTPUT_ADDR, &dst_img.addr_phy);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret = ioctl(fd, SCALE_IO_SCALE_MODE, &scale_mode);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret = ioctl(fd, SCALE_IO_START, NULL);
	HWCOMPOSER_EXIT_IF_ERR(ret);

	ret = ioctl(fd, SCALE_IO_IS_DONE, &scale_frm);

	exit:
	if(fd >= 0)
	{
		ioctl(fd, SCALE_IO_STOP, &scale_frm);
		close(fd);
	}
	if (ret) {
		ALOGE("camera_scaling fail. Line:%d", __LINE__);
		ret = -1;
	}else{
		ret = 0;
	}

	return ret;
}

int do_scaling_and_rotaion(HW_SCALE_DATA_FORMAT_E output_fmt,
	uint32_t output_width, uint32_t output_height,
	uint32_t output_yaddr,uint32_t output_uvaddr,
	HW_SCALE_DATA_FORMAT_E input_fmt,uint32_t input_uv_endian,
	uint32_t input_width,uint32_t input_height,
	uint32_t input_yaddr, uint32_t intput_uvaddr,
	struct sprd_rect *trim_rect, HW_ROTATION_MODE_E rotation, uint32_t tmp_addr)
{
	int ret = 0;
	ret = camera_scaling(output_fmt, output_width, output_height,
			output_yaddr, output_uvaddr, input_fmt, input_uv_endian,
			input_width, input_height, input_yaddr, intput_uvaddr,
			trim_rect, rotation, tmp_addr);
	if (ret){
		ALOGE("do_scaling_and_rotaion: camera_scaling fail. Line:%d", __LINE__);
		return -1;
	}

	if(HW_ROTATION_0 == rotation)
	{
		return 0;
	}
	ret = camera_rotation( output_fmt, rotation_degree_hw2user(rotation), output_width, output_height, tmp_addr, output_yaddr);

	if (ret) {
		ALOGE("do_scaling_and_rotaion: camera_rotation fail. Line:%d", __LINE__);
		return -1;
	}else{
		return 0;
	}
}

#ifndef USE_GPU_PROCESS_VIDEO
int transform_layer(uint32_t srcPhy, uint32_t srcVirt, uint32_t srcFormat, uint32_t transform,
									uint32_t srcWidth, uint32_t srcHeight , uint32_t dstPhy ,
									uint32_t dstVirt, uint32_t dstFormat , uint32_t dstWidth, 
									uint32_t dstHeight , struct sprd_rect *trim_rect , uint32_t tmp_phy_addr,
									uint32_t tmp_vir_addr)
{
	int ret = 0;
	HW_SCALE_DATA_FORMAT_E input_format;
	int input_endian = 0;
	int dst_scale_rot_format = 0;
	switch(srcFormat) {
	case HAL_PIXEL_FORMAT_YCbCr_420_SP:
		input_format = HW_SCALE_DATA_YUV420;
		input_endian = 1;
		break;
	case HAL_PIXEL_FORMAT_YCrCb_420_SP:
		input_format = HW_SCALE_DATA_YUV420;
		break;
	case HAL_PIXEL_FORMAT_YV12:
		input_format = HW_SCALE_DATA_YUV420_3FRAME;
		break;
	default:
		return -1;
	}
	switch(dstFormat)
	{
	case HAL_PIXEL_FORMAT_YCbCr_420_SP:
		dst_scale_rot_format = HW_SCALE_DATA_YUV420;
		break;
	default:
		return -1;
	}
	int dcam_rot_degree = 0;
	HW_ROTATION_MODE_E rot;
	uint32_t outRealWidth = dstWidth;
	uint32_t outRealHeight = dstHeight;
	switch(transform) {
	case 0:
		rot = HW_ROTATION_0;
		break;
	case HAL_TRANSFORM_ROT_90:
		rot = HW_ROTATION_90;
		outRealWidth = dstHeight;
		outRealHeight = dstWidth;
		dcam_rot_degree = 90;
		break;
	case HAL_TRANSFORM_ROT_180:
		rot = HW_ROTATION_180;
		dcam_rot_degree = 180;
		break;
	case HAL_TRANSFORM_ROT_270:
		rot = HW_ROTATION_270;
		outRealWidth = dstHeight;
		outRealHeight = dstWidth;
		dcam_rot_degree = 270;
		break;
	case HAL_TRANSFORM_FLIP_H:
		rot = HW_ROTATION_MIRROR;
		dcam_rot_degree = -1;//ROTATION_MIRROR
		break;
	default://HAL_TRANSFORM_ROT_90+HAL_TRANSFORM_FLIP_H or HAL_TRANSFORM_ROT_90+HAL_TRANSFORM_FLIP_V  or HAL_TRANSFORM_FLIP_V
		rot = HW_ROTATION_90;
		outRealWidth = dstHeight;
		outRealHeight = dstWidth;
		break;
	}

	if ((srcFormat != HAL_PIXEL_FORMAT_YV12) && dcam_rot_degree
		&& (srcWidth == trim_rect->w) && (srcHeight == trim_rect->h)
		&& (srcWidth == outRealWidth) && (srcHeight == outRealHeight)) {
		ALOGV("do rotation by rot hw");
		ret = camera_rotation(HW_ROTATION_DATA_YUV420, dcam_rot_degree, srcWidth, srcHeight, srcPhy, dstPhy);
		if(-1 == ret)
			ALOGE("do rotaion fail");
	} else {
		ret = do_scaling_and_rotaion(dst_scale_rot_format,
		outRealWidth,outRealHeight,
		dstPhy,dstPhy + dstHeight*dstWidth,
		input_format,input_endian,
		srcWidth,srcHeight,
		srcPhy,srcPhy + srcWidth*srcHeight,
		trim_rect,rot, tmp_phy_addr);
		if(ret != 0)
			ALOGE("do_scaling_and_rotaion failed");
	}
	
	return ret;

}
#endif
