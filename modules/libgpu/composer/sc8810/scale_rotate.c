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
#include "sprd_scale.h"
#include "sprd_rotation.h"
#include "scale_rotate.h"
#include "graphics.h"

#define SPRD_Y2R_CONTRAST 	74
#define SPRD_Y2R_SATURATION	73
#define SPRD_Y2R_BRIGHTNESS	2
#define SPRD_Y2R_CLIP(_x)	((unsigned char)((_x) < 0 ? 0 : ((_x) > 255 ? 255 : (_x))))

static void sprd_yuv420toargb888(uint32_t argb_data_ptr, uint32_t y_data_ptr, uint32_t uv_data_ptr,
						uint32_t width, uint32_t height)
{
	uint32_t *argb_ptr = (uint32_t *)argb_data_ptr;
	unsigned char *y_ptr = (unsigned char *)y_data_ptr;
	unsigned char *uv_ptr = (unsigned char *)uv_data_ptr;
	uint32_t i, j;

	if ((width & 1) > 0 || (height & 1) > 0)
		return;

	for (i=0; i<height; i+=2)
	{
		for (j=0; j<width; j+=2)
		{
			int y, u, v;
			int r, g, b;
			int temp_r, temp_g, temp_b;

			u = *uv_ptr++;
			v = *uv_ptr++;

			u = (u - 128) * SPRD_Y2R_SATURATION / 64 + 128;
			u = SPRD_Y2R_CLIP(u);
			v = (v - 128) * SPRD_Y2R_SATURATION / 64 + 128;
			v= SPRD_Y2R_CLIP(v);

			temp_r = 359 * v / 256 - 180;
			temp_g = - (88 * u + 183 * v) / 256 + 136;
			temp_b = 454 * u / 256 - 277;

			/////////////////////////////////////////////////////////////
			y = *y_ptr;
			y = (y - 128) * SPRD_Y2R_CONTRAST / 64 + 128 + SPRD_Y2R_BRIGHTNESS;
			y = SPRD_Y2R_CLIP(y);

			r = y + temp_r;
			g = y + temp_g;
			b = y + temp_b;

			r = SPRD_Y2R_CLIP(r);
			g = SPRD_Y2R_CLIP(g);
			b = SPRD_Y2R_CLIP(b);

			*argb_ptr = 0xff000000 | (r << 16) | (g << 8) | (b);

			/////////////////////////////////////////////////////////////
			y = *(y_ptr + 1);
			y = (y - 128) * SPRD_Y2R_CONTRAST / 64 + 128 + SPRD_Y2R_BRIGHTNESS;
			y = SPRD_Y2R_CLIP(y);

			r = y + temp_r;
			g = y + temp_g;
			b = y + temp_b;

			r = SPRD_Y2R_CLIP(r);
			g = SPRD_Y2R_CLIP(g);
			b = SPRD_Y2R_CLIP(b);

			*(argb_ptr + 1) = 0xff000000 | (r << 16) | (g << 8) | (b);

			/////////////////////////////////////////////////////////////
			y = *(y_ptr + width);
			y = (y - 128) * SPRD_Y2R_CONTRAST / 64 + 128 + SPRD_Y2R_BRIGHTNESS;
			y = SPRD_Y2R_CLIP(y);

			r = y + temp_r;
			g = y + temp_g;
			b = y + temp_b;

			r = SPRD_Y2R_CLIP(r);
			g = SPRD_Y2R_CLIP(g);
			b = SPRD_Y2R_CLIP(b);

			*(argb_ptr + width) = 0xff000000 | (r << 16) | (g << 8) | (b);

			/////////////////////////////////////////////////////////////
			y = *(y_ptr + width + 1);
			y = (y - 128) * SPRD_Y2R_CONTRAST / 64 + 128 + SPRD_Y2R_BRIGHTNESS;
			y = SPRD_Y2R_CLIP(y);

			r = y + temp_r;
			g = y + temp_g;
			b = y + temp_b;

			r = SPRD_Y2R_CLIP(r);
			g = SPRD_Y2R_CLIP(g);
			b = SPRD_Y2R_CLIP(b);

			*(argb_ptr + width + 1) = 0xff000000 | (r << 16) | (g << 8) | (b);

			argb_ptr += 2;
			y_ptr += 2;
		}

		y_ptr += width;
		argb_ptr += width;
	}
}

static ROTATION_DATA_FORMAT_E rotation_data_format_convertion(HW_ROTATION_DATA_FORMAT_E data_format)
{
	ROTATION_DATA_FORMAT_E result_format = ROTATION_MAX;

	switch(data_format)
	{
		case HW_ROTATION_DATA_YUV422:
			result_format = ROTATION_YUV422;
			break;
		case HW_ROTATION_DATA_YUV420:
			result_format = ROTATION_YUV420;
			break;
		case HW_ROTATION_DATA_YUV400:
			result_format = ROTATION_YUV400;
			break;
		case HW_ROTATION_DATA_RGB888:
			result_format = ROTATION_RGB888;
			break;
		case HW_ROTATION_DATA_RGB666:
			result_format = ROTATION_RGB666;
			break;
		case HW_ROTATION_DATA_RGB565:
			result_format = ROTATION_RGB565;
			break;
		case HW_ROTATION_DATA_RGB555:
			result_format = ROTATION_RGB555;
			break;
		default:
			result_format = ROTATION_MAX;
			break;
	}
	return result_format;
}

static ROTATION_DIR_E rotation_degree_convertion(int degree)
{
	ROTATION_DIR_E result_degree = ROTATION_DIR_MAX;

	switch(degree)
	{
		case -1:
			result_degree = ROTATION_MIRROR;
			break;
		case 90:
			result_degree = ROTATION_90;
			break;
		case 180:
			result_degree = ROTATION_180;
			break;
		case 270:
			result_degree = ROTATION_270;
			break;
		default:
			result_degree = ROTATION_DIR_MAX;
			break;
	}
	return result_degree;
}


int camera_rotation_copy_data(uint32_t width, uint32_t height, uint32_t in_addr, uint32_t out_addr)
{
	int fd = -1;
	ROTATION_PARAM_T rot_params;

	rot_params.data_format = ROTATION_RGB888;
	rot_params.img_size.w = width;
	rot_params.img_size.h = height;
	rot_params.src_addr.y_addr = in_addr;
	rot_params.src_addr.uv_addr = rot_params.src_addr.y_addr + rot_params.img_size.w * rot_params.img_size.h;
	rot_params.src_addr.v_addr = rot_params.src_addr.uv_addr;
	rot_params.dst_addr.y_addr = out_addr;
	rot_params.dst_addr.uv_addr = rot_params.dst_addr.y_addr + rot_params.img_size.w * rot_params.img_size.h;
	rot_params.dst_addr.v_addr = rot_params.dst_addr.uv_addr;

	fd = open("/dev/sprd_rotation", O_RDWR /* required */, 0);
	if (-1 == fd)
	{
		ALOGE("Fail to open rotation device.");
		return -1;
	}

	//done
	if (-1 == ioctl(fd, SPRD_ROTATION_DATA_COPY, &rot_params))
	{
		ALOGE("Fail to SC8800G_ROTATION_DATA_COPY");
		return -1;
	}

	if(-1 == close(fd))
	{
		ALOGE("Fail to close rotation device.");
		return -1;
	}

	fd = -1;
	return 0;
}

int camera_rotation_copy_data_from_virtual(uint32_t width, uint32_t height, uint32_t in_virtual_addr, uint32_t out_addr)
{
	int fd = -1;
	ROTATION_PARAM_T rot_params;

	rot_params.data_format = ROTATION_RGB888;
	rot_params.img_size.w = width;
	rot_params.img_size.h = height;
	rot_params.src_addr.y_addr = in_virtual_addr;
	rot_params.src_addr.uv_addr = rot_params.src_addr.y_addr + rot_params.img_size.w * rot_params.img_size.h;
	rot_params.src_addr.v_addr = rot_params.src_addr.uv_addr;
	rot_params.dst_addr.y_addr = out_addr;
	rot_params.dst_addr.uv_addr = rot_params.dst_addr.y_addr + rot_params.img_size.w * rot_params.img_size.h;
	rot_params.dst_addr.v_addr = rot_params.dst_addr.uv_addr;

	fd = open("/dev/sprd_rotation", O_RDWR /* required */, 0);
	if (-1 == fd)
	{
		ALOGE("Fail to open rotation device %s.", __FILE__);
		return -1;
	}

	//done
	if (-1 == ioctl(fd, SPRD_ROTATION_DATA_COPY_FROM_V_TO_P, &rot_params))
	{
		ALOGE("Fail to SC8800G_ROTATION_DATA_COPY %s", __FILE__);
		return -1;
	}

	if(-1 == close(fd))
	{
		ALOGE("Fail to close rotation device. %s", __FILE__);
		return -1;
	}
	fd = -1;
	return 0;
}

int camera_rotation(HW_ROTATION_DATA_FORMAT_E rot_format, int degree, uint32_t width, uint32_t height, uint32_t in_addr, uint32_t out_addr)
{
	int fd = -1;
	int ret = 0;
	ROTATION_PARAM_T rot_params;

	rot_params.data_format = rotation_data_format_convertion(rot_format);
	rot_params.rotation_dir = rotation_degree_convertion(degree);
	rot_params.img_size.w = width;
	rot_params.img_size.h = height;
	rot_params.src_addr.y_addr = in_addr;
	rot_params.src_addr.uv_addr = rot_params.src_addr.y_addr + rot_params.img_size.w * rot_params.img_size.h;
	rot_params.src_addr.v_addr = rot_params.src_addr.uv_addr;
	rot_params.dst_addr.y_addr = out_addr;
	rot_params.dst_addr.uv_addr = rot_params.dst_addr.y_addr + rot_params.img_size.w * rot_params.img_size.h;
	rot_params.dst_addr.v_addr = rot_params.dst_addr.uv_addr;

	fd = open("/dev/sprd_rotation", O_RDWR /* required */, 0);
	if (-1 == fd)
	{
		ALOGE("Fail to open rotation device.");
		return -1;
	}

	//done
	if (-1 == ioctl(fd, SPRD_ROTATION_DONE, &rot_params))
	{
		ALOGE("Fail to SC8800G_ROTATION_DONE");
		ret = -1;
	}

	if(-1 == close(fd))
	{
		ALOGE("Fail to close rotation device.");
		return -1;
	}
	fd = -1;
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
	SCALE_CONFIG_T scale_config;
	SCALE_SIZE_T scale_size;
	SCALE_RECT_T scale_rect;
	SCALE_ADDRESS_T scale_address;
	HW_SCALE_DATA_FORMAT_E data_format;
	SCALE_MODE_E scale_mode;
	uint32_t enable = 0, mode;
	uint32_t slice_height = 0;
	ISP_ENDIAN_T in_endian;
	ISP_ENDIAN_T out_endian;
	uint32_t scaling_mode = rotation;
	int fd = open("/dev/sprd_scale", O_RDONLY);
	if(fd < 0)
	{
		ALOGE("error to open dev sprd_scale");
		return -1;
	}
	//set mode
	scale_config.id = SCALE_PATH_MODE;
	scale_mode = SCALE_MODE_SCALE;
	scale_config.param = &scale_mode;
	if (-1 == ioctl(fd, SCALE_IOC_CONFIG, &scale_config))
	{
		ALOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		close(fd);
		return -1;
	}

	//set input data format
	scale_config.id = SCALE_PATH_INPUT_FORMAT;
	data_format = input_fmt;
	scale_config.param = &data_format;
	if (-1 == ioctl(fd, SCALE_IOC_CONFIG, &scale_config))
	{
		ALOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		close(fd);
		return -1;
	}
	//set output data format
	scale_config.id = SCALE_PATH_OUTPUT_FORMAT;
	data_format = output_fmt;
	scale_config.param = &data_format;
	if (-1 == ioctl(fd, SCALE_IOC_CONFIG, &scale_config))
	{
		ALOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		close(fd);
		return -1;
	}
	//set input size
	scale_config.id = SCALE_PATH_INPUT_SIZE;
	scale_size.w = input_width;// trim_rect->w;
	scale_size.h = input_height;//trim_rect->h;
	scale_config.param = &scale_size;
	if (-1 == ioctl(fd, SCALE_IOC_CONFIG, &scale_config))
	{
		ALOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		close(fd);
		return -1;
	}
	//set output size
	scale_config.id = SCALE_PATH_OUTPUT_SIZE;
	scale_size.w = output_width;
	scale_size.h = output_height;
	scale_config.param = &scale_size;
	if (-1 == ioctl(fd, SCALE_IOC_CONFIG, &scale_config))
	{
		ALOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		close(fd);
		return -1;
	}
	//set input size
	scale_config.id = SCALE_PATH_INPUT_RECT;
	scale_rect.x = trim_rect->x;
	scale_rect.y = trim_rect->y;
	scale_rect.w = trim_rect->w;
	scale_rect.h = trim_rect->h;
	scale_config.param = &scale_rect;
	if (-1 == ioctl(fd, SCALE_IOC_CONFIG, &scale_config))
	{
		ALOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		close(fd);
		return -1;
	}
	//set input address
	scale_config.id = SCALE_PATH_INPUT_ADDR;
	scale_address.yaddr = input_yaddr;
	scale_address.uaddr = intput_uvaddr;
	scale_address.vaddr = scale_address.uaddr + input_width*input_height/4;
	scale_config.param = &scale_address;
	ALOGV("scale input y addr:0x%x,uv addr:0x%x.",scale_address.yaddr,scale_address.uaddr);
	if (-1 == ioctl(fd, SCALE_IOC_CONFIG, &scale_config))
	{
		ALOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		close(fd);
		return -1;
	}
	//set output address
	scale_config.id = SCALE_PATH_OUTPUT_ADDR;
	scale_address.yaddr = output_yaddr;
	scale_address.uaddr = output_uvaddr;//output_addr + output_width * output_height;
	scale_address.vaddr = scale_address.uaddr;//todo
	scale_config.param = &scale_address;
	ALOGV("scale out  y addr:0x%x,uv addr:0x%x.",scale_address.yaddr,scale_address.uaddr);;
	if (-1 == ioctl(fd, SCALE_IOC_CONFIG, &scale_config))
	{
		ALOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		close(fd);
		return -1;
	}
	//set input endian
	scale_config.id = SCALE_PATH_INPUT_ENDIAN;
	in_endian.endian_y = 1;
	in_endian.endian_uv = input_uv_endian;//1;
	scale_config.param = &in_endian;
	if (-1 == ioctl(fd, SCALE_IOC_CONFIG, &scale_config))
	{
		ALOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		close(fd);
		return -1;
	}
	//set output endian
	scale_config.id = SCALE_PATH_OUTPUT_ENDIAN;
	out_endian.endian_y = 1;
	out_endian.endian_uv = 1;
	scale_config.param = &out_endian;
	if (-1 == ioctl(fd, SCALE_IOC_CONFIG, &scale_config))
	{
		ALOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		close(fd);
		return -1;;
	}

	//set rotation mode
	scale_config.id = SCALE_PATH_ROT_MODE;
	scale_config.param = &scaling_mode;
	if (-1 == ioctl(fd, SCALE_IOC_CONFIG, &scale_config))
	{
		ALOGE("Fail to SCALE_IOC_CONFIG: id=%d", scale_config.id);
		close(fd);
		return -1;
	}

	//done
	if (-1 == ioctl(fd, SCALE_IOC_DONE, 0))
	{
		ALOGE("Fail to SCALE_IOC_DONE");
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
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
	uint32_t tmp_dst_phy_addr = 0;

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
	case HAL_PIXEL_FORMAT_RGBX_8888:
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

	if(HAL_PIXEL_FORMAT_RGBX_8888 == dstFormat){
		tmp_dst_phy_addr = tmp_phy_addr;
	}
	else{
		tmp_dst_phy_addr = dstPhy;
	}

	if ((srcFormat != HAL_PIXEL_FORMAT_YV12) && dcam_rot_degree
		&& (srcWidth == trim_rect->w) && (srcHeight == trim_rect->h)
		&& (srcWidth == outRealWidth) && (srcHeight == outRealHeight)) {
		ALOGV("do rotation by rot hw");

		ret = camera_rotation(HW_ROTATION_DATA_YUV420, dcam_rot_degree, srcWidth, srcHeight, srcPhy, tmp_dst_phy_addr);
		if(-1 == ret)
			ALOGE("do rotaion fail");
		else{
			if(HAL_PIXEL_FORMAT_RGBX_8888 == dstFormat)
				sprd_yuv420toargb888(dstVirt,tmp_vir_addr,tmp_vir_addr+srcWidth*srcHeight,srcWidth, srcHeight);
		}
	} else {
		ret = do_scaling_and_rotaion(dst_scale_rot_format,
		outRealWidth,outRealHeight,
		tmp_dst_phy_addr,tmp_dst_phy_addr + outRealWidth*outRealHeight,
		input_format,input_endian,
		srcWidth,srcHeight,
		srcPhy,srcPhy + srcWidth*srcHeight,
		trim_rect,rot, tmp_phy_addr);
		if(ret != 0){
			ALOGE("do_scaling_and_rotaion failed");
		}
		else{
			if(HAL_PIXEL_FORMAT_RGBX_8888 == dstFormat)
				sprd_yuv420toargb888(dstVirt,tmp_vir_addr,tmp_vir_addr+outRealWidth*outRealHeight,outRealWidth, outRealHeight);
		}
	}

	return ret;

}
#endif
