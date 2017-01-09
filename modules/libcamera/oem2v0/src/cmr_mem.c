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
#define LOG_TAG "cmr_mem"

#include "cmr_mem.h"
#include "cmr_oem.h"
#include <unistd.h>

/*
          to add more.........
     8M-----------16M-------------8M
     5M-----------16M-------------0M
     3M-----------8M--------------2M
     2M-----------4M--------------2M
     1.3M---------4M--------------1M
*/

#define PIXEL_1P3_MEGA            0x180000 //actually 1.5 *1024*1024
#define PIXEL_2P0_MEGA            0x200000 //actually 2.0 *1024*1024
#define PIXEL_3P0_MEGA            0x300000 //actually 3.0 *1024*1024
#define PIXEL_4P0_MEGA            0x400000 //actually 3.0 *1024*1024
#define PIXEL_5P0_MEGA            0x500000 //5.0 *1024*1024
#define PIXEL_6P0_MEGA            0x600000 //6.0 *1024*1024
#define PIXEL_7P0_MEGA            0x700000 //7.0 *1024*1024
#define PIXEL_8P0_MEGA            0x800000 //8.0 *1024*1024
#define PIXEL_9P0_MEGA            0x900000 //9.0 *1024*1024
#define PIXEL_AP0_MEGA            0xA00000 //10.0 *1024*1024
#define PIXEL_BP0_MEGA            0xB00000 //11.0 *1024*1024
#define PIXEL_CP0_MEGA            0xC00000 //12.0 *1024*1024
#define PIXEL_DP0_MEGA            0xD00000 //13.0 *1024*1024
#define MIN_GAP_LINES              0x80
#define ALIGN_PAGE_SIZE          (1<<12)

#define ISP_YUV_TO_RAW_GAP        CMR_SLICE_HEIGHT
#define BACK_CAMERA_ID            0
#define FRONT_CAMERA_ID           1
#define DEV2_CAMERA_ID           2
#define JPEG_SMALL_SIZE           (300 * 1024)
#define ADDR_BY_WORD(a)           (((a) + 3 ) & (~3))
#define CMR_NO_MEM(a, b) \
	do { \
		if ((a) > (b)) { \
			CMR_LOGE("No memory, 0x%x 0x%x", (a), (b)); \
			return -1; \
		} \
	} while(0)

enum {
	IMG_1P3_MEGA = 0,
	IMG_2P0_MEGA,
	IMG_3P0_MEGA,
	IMG_4P0_MEGA,
	IMG_5P0_MEGA,
	IMG_6P0_MEGA,
	IMG_7P0_MEGA,
	IMG_8P0_MEGA,
	IMG_9P0_MEGA,
	IMG_AP0_MEGA,
	IMG_BP0_MEGA,
	IMG_CP0_MEGA,
	IMG_DP0_MEGA,
	IMG_SIZE_NUM
};

enum {
	JPEG_TARGET = 0,
	THUM_YUV,
	THUM_JPEG,

	BUF_TYPE_NUM
};

typedef uint32_t (*cmr_get_size)(uint32_t width, uint32_t height, uint32_t thum_width, uint32_t thum_height);

struct cap_size_to_mem {
	uint32_t    pixel_num;
	uint32_t    mem_size;
};

static const struct cap_size_to_mem back_cam_mem_size_tab[IMG_SIZE_NUM] = {
	{PIXEL_1P3_MEGA, (7  << 20)},
	{PIXEL_2P0_MEGA, (9  << 20)},
	{PIXEL_3P0_MEGA, (15 << 20)},
	{PIXEL_4P0_MEGA, (16 << 20)},
	{PIXEL_5P0_MEGA, (23 << 20)},
	{PIXEL_6P0_MEGA, (21 << 20)},
	{PIXEL_7P0_MEGA, (25 << 20)},
	{PIXEL_8P0_MEGA, (26 << 20)}
};
static const struct cap_size_to_mem back_cam_raw_mem_size_tab[IMG_SIZE_NUM] = {
	{PIXEL_1P3_MEGA, (6  << 20)},
	{PIXEL_2P0_MEGA, (9  << 20)},
	{PIXEL_3P0_MEGA, (14 << 20)},
	{PIXEL_4P0_MEGA, (18 << 20)},
	{PIXEL_5P0_MEGA, (23 << 20)},
	{PIXEL_6P0_MEGA, (26 << 20)},
	{PIXEL_7P0_MEGA, (28 << 20)},
	{PIXEL_8P0_MEGA, (33 << 20)},
	{PIXEL_9P0_MEGA, (40 << 20)},
	{PIXEL_AP0_MEGA, (40 << 20)},
	{PIXEL_BP0_MEGA, (42 << 20)},
	{PIXEL_CP0_MEGA, (42 << 20)},
#ifdef CONFIG_MEM_OPTIMIZATION
	{PIXEL_DP0_MEGA, (42 << 20)}
#else
	{PIXEL_DP0_MEGA, (67 << 20)}
#endif
};


static const struct cap_size_to_mem front_cam_mem_size_tab[IMG_SIZE_NUM] = {
	{PIXEL_1P3_MEGA, (8  << 20)},
	{PIXEL_2P0_MEGA, (10  << 20)},
	{PIXEL_3P0_MEGA, (15 << 20)},
	{PIXEL_4P0_MEGA, (20 << 20)},
	{PIXEL_5P0_MEGA, (22 << 20)},
	{PIXEL_6P0_MEGA, (22 << 20)},
	{PIXEL_7P0_MEGA, (25 << 20)},
	{PIXEL_8P0_MEGA, (26 << 20)}
};
static const struct cap_size_to_mem front_cam_raw_mem_size_tab[IMG_SIZE_NUM] = {
	{PIXEL_1P3_MEGA, (10  << 20)},
	{PIXEL_2P0_MEGA, (10  << 20)},
	{PIXEL_3P0_MEGA, (15 << 20)},
	{PIXEL_4P0_MEGA, (20 << 20)},
	{PIXEL_5P0_MEGA, (24 << 20)},
	{PIXEL_6P0_MEGA, (22 << 20)},
	{PIXEL_7P0_MEGA, (25 << 20)},
	{PIXEL_8P0_MEGA, (26 << 20)}
};

/*for ATV*/
static const struct cap_size_to_mem mem_size_tab[IMG_SIZE_NUM] = {
	{PIXEL_1P3_MEGA, (5  << 20)},
	{PIXEL_2P0_MEGA, (9  << 20)},
	{PIXEL_3P0_MEGA, (18 << 20)},
	{PIXEL_4P0_MEGA, (18 << 20)},
	{PIXEL_5P0_MEGA, (19 << 20)},
	{PIXEL_6P0_MEGA, (19 << 20)},
	{PIXEL_7P0_MEGA, (20 << 20)},
	{PIXEL_8P0_MEGA, (20 << 20)}
};

static const struct cap_size_to_mem reserve_mem_size_tab[IMG_SIZE_NUM] = {
	{PIXEL_1P3_MEGA, (3  << 20)},
	{PIXEL_2P0_MEGA, (3  << 20)},
	{PIXEL_3P0_MEGA, (5 << 20)},
	{PIXEL_4P0_MEGA, (6 << 20)},
	{PIXEL_5P0_MEGA, (8 << 20)},
	{PIXEL_6P0_MEGA, (9 << 20)},
	{PIXEL_7P0_MEGA, (11 << 20)},
	{PIXEL_8P0_MEGA, (12 << 20)},
	{PIXEL_9P0_MEGA, (14 << 20)},
	{PIXEL_AP0_MEGA, (15 << 20)},
	{PIXEL_BP0_MEGA, (17 << 20)},
	{PIXEL_CP0_MEGA, (18 << 20)},
	{PIXEL_DP0_MEGA, (20 << 20)}
};

extern int camera_get_is_noscale(void);

static uint32_t get_jpeg_size(uint32_t width, uint32_t height, uint32_t thum_width, uint32_t thum_height);
static uint32_t get_thum_yuv_size(uint32_t width, uint32_t height, uint32_t thum_width, uint32_t thum_height);
static uint32_t get_thum_jpeg_size(uint32_t width, uint32_t height, uint32_t thum_width, uint32_t thum_height);
static uint32_t get_jpg_tmp_size(uint32_t width, uint32_t height, uint32_t thum_width, uint32_t thum_height);
static uint32_t get_scaler_tmp_size(uint32_t width, uint32_t height, uint32_t thum_width, uint32_t thum_height);
static uint32_t get_isp_tmp_size(uint32_t width, uint32_t height, uint32_t thum_width, uint32_t thum_height);

static int arrange_raw_buf(struct cmr_cap_2_frm *cap_2_frm,
					struct img_size *sn_size,
					struct img_rect *sn_trim,
					struct img_size *image_size,
					uint32_t orig_fmt,
					struct img_size *cap_size,
					struct img_size *thum_size,
					struct cmr_cap_mem *capture_mem,
					uint32_t need_rot,
					uint32_t *io_mem_res,
					uint32_t *io_mem_end,
					uint32_t *io_channel_size);

static int arrange_jpeg_buf(struct cmr_cap_2_frm *cap_2_frm,
					struct img_size *sn_size,
					struct img_rect *sn_trim,
					struct img_size *image_size,
					uint32_t orig_fmt,
					struct img_size *cap_size,
					struct img_size *thum_size,
					struct cmr_cap_mem *capture_mem,
					uint32_t need_rot,
					uint32_t *io_mem_res,
					uint32_t *io_mem_end,
					uint32_t *io_channel_size);

static int arrange_yuv_buf(struct cmr_cap_2_frm *cap_2_frm,
					struct img_size *sn_size,
					struct img_rect *sn_trim,
					struct img_size *image_size,
					uint32_t orig_fmt,
					struct img_size *cap_size,
					struct img_size *thum_size,
					struct cmr_cap_mem *capture_mem,
					uint32_t need_rot,
					uint32_t *io_mem_res,
					uint32_t *io_mem_end,
					uint32_t *io_channel_size);

static int arrange_yuv_buf_optimization(struct cmr_cap_2_frm *cap_2_frm,
				struct img_size *sn_size,
				struct img_rect *sn_trim,
				struct img_size *image_size,
				uint32_t orig_fmt,
				struct img_size *cap_size,
				struct img_size *thum_size,
				struct cmr_cap_mem *capture_mem,
				uint32_t need_rot,
				uint32_t *io_mem_res,
				uint32_t *io_mem_end,
				uint32_t *io_channel_size);

static int arrange_misc_buf(struct cmr_cap_2_frm *cap_2_frm,
					struct img_size *sn_size,
					struct img_rect *sn_trim,
					struct img_size *image_size,
					uint32_t orig_fmt,
					struct img_size *cap_size,
					struct img_size *thum_size,
					struct cmr_cap_mem *capture_mem,
					uint32_t need_rot,
					uint32_t *io_mem_res,
					uint32_t *io_mem_end,
					uint32_t *io_channel_size);

static int arrange_rot_buf(struct cmr_cap_2_frm *cap_2_frm,
					struct img_size *sn_size,
					struct img_rect *sn_trim,
					struct img_size *image_size,
					uint32_t orig_fmt,
					struct img_size *cap_size,
					struct img_size *thum_size,
					struct cmr_cap_mem *capture_mem,
					uint32_t need_rot,
					uint32_t need_scale,
					uint32_t *io_mem_res,
					uint32_t *io_mem_end,
					uint32_t *io_channel_size);

static const cmr_get_size get_size[BUF_TYPE_NUM] = {
	get_jpeg_size,
	get_thum_yuv_size,
	get_thum_jpeg_size,

};

int camera_pre_capture_buf_id(cmr_u32 camera_id)
{
	UNUSED(camera_id);

	int buffer_id = 0;

	if (FRONT_CAMERA_ID == camera_id) {
#if defined(CONFIG_FRONT_CAMERA_SUPPORT_13M)
		buffer_id = IMG_DP0_MEGA;
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_8M)
		buffer_id = IMG_8P0_MEGA;
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_7M)
		buffer_id = IMG_7P0_MEGA;
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_6M)
		buffer_id = IMG_6P0_MEGA;
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_5M)
		buffer_id = IMG_5P0_MEGA;
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_4M)
		buffer_id = IMG_4P0_MEGA;
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_3M)
		buffer_id = IMG_3P0_MEGA;
#elif defined(CONFIG_FRONT_CAMERA_SUPPORT_2M)
		buffer_id = IMG_2P0_MEGA;
#else
		buffer_id = IMG_2P0_MEGA;
#endif
	} else if (DEV2_CAMERA_ID == camera_id) {
#if defined(CONFIG_CAMERA_DEV_2_SUPPORT_13M)
		buffer_id = IMG_DP0_MEGA;
#elif defined(CONFIG_CAMERA_DEV_2_SUPPORT_8M)
		buffer_id = IMG_8P0_MEGA;
#elif defined(CONFIG_CAMERA_DEV_2_SUPPORT_7M)
		buffer_id = IMG_7P0_MEGA;
#elif defined(CONFIG_CAMERA_DEV_2_SUPPORT_6M)
		buffer_id = IMG_6P0_MEGA;
#elif defined(CONFIG_CAMERA_DEV_2_SUPPORT_5M)
		buffer_id = IMG_5P0_MEGA;
#elif defined(CONFIG_CAMERA_DEV_2_SUPPORT_4M)
		buffer_id = IMG_4P0_MEGA;
#elif defined(CONFIG_CAMERA_DEV_2_SUPPORT_3M)
		buffer_id = IMG_3P0_MEGA;
#elif defined(CONFIG_CAMERA_DEV_2_SUPPORT_2M)
		buffer_id = IMG_2P0_MEGA;
#else
		buffer_id = IMG_2P0_MEGA;
#endif
	} else {
#if defined(CONFIG_CAMERA_SUPPORT_13M)
		buffer_id = IMG_DP0_MEGA;
#elif defined(CONFIG_CAMERA_SUPPORT_8M)
		buffer_id = IMG_8P0_MEGA;
#elif defined(CONFIG_CAMERA_SUPPORT_7M)
		buffer_id = IMG_7P0_MEGA;
#elif defined(CONFIG_CAMERA_SUPPORT_6M)
		buffer_id = IMG_6P0_MEGA;
#elif defined(CONFIG_CAMERA_SUPPORT_5M)
		buffer_id = IMG_5P0_MEGA;
#elif defined(CONFIG_CAMERA_SUPPORT_4M)
		buffer_id = IMG_4P0_MEGA;
#elif defined(CONFIG_CAMERA_SUPPORT_3M)
		buffer_id = IMG_3P0_MEGA;
#elif defined(CONFIG_CAMERA_SUPPORT_2M)
		buffer_id = IMG_2P0_MEGA;
#else
		buffer_id = IMG_2P0_MEGA;
#endif
	}

	CMR_LOGI("buffer id = %d", buffer_id);

	return buffer_id;
}

int camera_reserve_buf_size(cmr_u32 camera_id,
					cmr_s32 mem_size_id,
					cmr_u32 *mem_size,
					cmr_u32 *mem_sum)
{
	struct cap_size_to_mem *mem_tab_ptr = NULL;

	if (mem_size_id < IMG_1P3_MEGA
		|| mem_size_id >= IMG_SIZE_NUM
		|| NULL == mem_size) {
		CMR_LOGE("no matched size for this image: id=%d, %p",
				mem_size_id,
				mem_size);
		return -1;
	}
	*mem_sum = 1;

	mem_tab_ptr = (struct cap_size_to_mem*)&reserve_mem_size_tab[0];
	*mem_size = mem_tab_ptr[mem_size_id].mem_size;

	CMR_LOGI("image size num, %d, mem size 0x%x",
		mem_size_id,
		mem_tab_ptr[mem_size_id].mem_size);

	return 0;
}

int camera_pre_capture_buf_size(cmr_u32 camera_id,
					cmr_s32 mem_size_id,
					cmr_u32 *mem_size,
					cmr_u32 *mem_sum)
{
	struct cap_size_to_mem *mem_tab_ptr = NULL;
	struct cap_size_to_mem *yuv_mem_tab_ptr = NULL;

	if (mem_size_id < IMG_1P3_MEGA
		|| mem_size_id >= IMG_SIZE_NUM
		|| NULL == mem_size) {
		CMR_LOGE("no matched size for this image: id=%d, %p",
				mem_size_id,
				mem_size);
		return -1;
	}

	*mem_sum = 1;

	if (BACK_CAMERA_ID == camera_id) {
		mem_tab_ptr = (struct cap_size_to_mem*)&back_cam_raw_mem_size_tab[0];
		yuv_mem_tab_ptr = (struct cap_size_to_mem*)&back_cam_mem_size_tab[0];
		*mem_size = MAX(mem_tab_ptr[mem_size_id].mem_size, yuv_mem_tab_ptr[mem_size_id].mem_size);
	} else if (FRONT_CAMERA_ID == camera_id || camera_id == DEV2_CAMERA_ID) {
		mem_tab_ptr = (struct cap_size_to_mem*)&front_cam_raw_mem_size_tab[0];
		yuv_mem_tab_ptr = (struct cap_size_to_mem*)&front_cam_mem_size_tab[0];
		*mem_size = MAX(mem_tab_ptr[mem_size_id].mem_size, yuv_mem_tab_ptr[mem_size_id].mem_size);
	} else {
		mem_tab_ptr = (struct cap_size_to_mem*)&mem_size_tab[0];
		*mem_size = mem_tab_ptr[mem_size_id].mem_size;
	}

	CMR_LOGI("image size num, %d, mem size 0x%x",
		mem_size_id,
		mem_tab_ptr[mem_size_id].mem_size);

	return 0;
}

int camera_capture_buf_size(uint32_t camera_id,
					uint32_t sn_fmt,
					struct img_size *image_size,
					uint32_t *mem_size)
{
	uint32_t               size_pixel;
	int                    i;
	struct cap_size_to_mem *mem_tab_ptr = NULL;

	if (NULL == image_size ||
		NULL == mem_size) {
		CMR_LOGE("Parameter error 0x%p 0x%p ",
			image_size,
			mem_size);
		return -1;
	}

	size_pixel = (uint32_t)(image_size->width * image_size->height);

	if (SENSOR_IMAGE_FORMAT_RAW == sn_fmt) {
		if (BACK_CAMERA_ID == camera_id) {
			mem_tab_ptr = (struct cap_size_to_mem*)&back_cam_raw_mem_size_tab[0];
		} else if (FRONT_CAMERA_ID == camera_id) {
			mem_tab_ptr = (struct cap_size_to_mem*)&front_cam_raw_mem_size_tab[0];
		} else {
			mem_tab_ptr = (struct cap_size_to_mem*)&mem_size_tab[0];
		}
	} else {
		if (BACK_CAMERA_ID == camera_id) {
			mem_tab_ptr = (struct cap_size_to_mem*)&back_cam_mem_size_tab[0];
		} else if (FRONT_CAMERA_ID == camera_id) {
			mem_tab_ptr = (struct cap_size_to_mem*)&front_cam_mem_size_tab[0];
		} else {
			mem_tab_ptr = (struct cap_size_to_mem*)&mem_size_tab[0];
		}
	}

	for (i = IMG_1P3_MEGA; i < IMG_SIZE_NUM; i++) {
		if (size_pixel <= mem_tab_ptr[i].pixel_num)
			break;
	}

	if (i == IMG_SIZE_NUM) {
		CMR_LOGE("No matched size for this image, 0x%x", size_pixel);
		return -1;
	} else {
		CMR_LOGI("image size num, %d, mem size 0x%x",
			i,
			mem_tab_ptr[i].mem_size);
	}

	*mem_size = mem_tab_ptr[i].mem_size;

	return 0;
}

int camera_arrange_capture_buf(struct cmr_cap_2_frm *cap_2_frm,
						struct img_size *sn_size,
						struct img_rect *sn_trim,
						struct img_size *image_size,
						uint32_t orig_fmt,
						struct img_size *cap_size,
						struct img_size *thum_size,
						struct cmr_cap_mem *capture_mem,
						uint32_t need_rot,
						uint32_t need_scale,
						uint32_t image_cnt)
{
	uint32_t       channel_size;
	uint32_t       size_pixel;
	uint32_t       mem_res = 0, mem_end = 0;
	uint32_t       i = 0;
	int32_t        ret = -1;
	struct cmr_cap_mem *cap_mem = &capture_mem[0];
	struct img_size align16_image_size, align16_cap_size;

	if (NULL == cap_2_frm ||
		NULL == image_size ||
		NULL == thum_size ||
		NULL == capture_mem ||
		NULL == sn_size ||
		NULL == sn_trim) {
		CMR_LOGE("Parameter error 0x%p 0x%p 0x%p 0x%p 0x%p 0x%p",
			cap_2_frm,
			image_size,
			thum_size,
			capture_mem,
			sn_size,
			sn_trim);
		return -1;
	}

	align16_image_size.width = camera_get_aligned_size(cap_2_frm->type, image_size->width);
	align16_image_size.height = camera_get_aligned_size(cap_2_frm->type, image_size->height);
	align16_cap_size.width = camera_get_aligned_size(cap_2_frm->type, cap_size->width);
	align16_cap_size.height = camera_get_aligned_size(cap_2_frm->type, cap_size->height);

	channel_size = (uint32_t)(align16_image_size.width * align16_image_size.height);
	size_pixel = channel_size;
	mem_res = cap_2_frm->mem_frm.buf_size;

	CMR_LOGI("Mem frame, 0x%lx 0x%lx, 0x%x",
		cap_2_frm->mem_frm.addr_phy.addr_y,
		cap_2_frm->mem_frm.addr_vir.addr_y,
		cap_2_frm->mem_frm.buf_size);

	CMR_LOGI("channel_size, 0x%x, image_cnt %d, rot %d, orig_fmt %d",
		channel_size,
		image_cnt,
		need_rot,
		orig_fmt);

	CMR_LOGI("sn_size %d %d, sn_trim %d %d %d %d, image_size %d %d, cap_size %d %d",
		sn_size->width, sn_size->height,
		sn_trim->start_x, sn_trim->start_y, sn_trim->width, sn_trim->height,
		image_size->width, image_size->height,
		cap_size->width, cap_size->height);

	CMR_LOGI("sn_trim x,y,w,h %d %d %d %d ",
		sn_trim->start_x, sn_trim->start_y,
		sn_trim->width, sn_trim->height);

	/* get target_jpeg buffer size first, will be used later */
	memset((void*)cap_mem, 0, sizeof(struct cmr_cap_mem));
	cap_mem->target_jpeg.buf_size = get_jpeg_size(align16_image_size.width,
					align16_image_size.height,
					thum_size->width,
					thum_size->height);

	if (IMG_DATA_TYPE_RAW == orig_fmt) {
		ret = arrange_raw_buf(cap_2_frm,
					sn_size,
					sn_trim,
					image_size,
					orig_fmt,
					cap_size,
					thum_size,
					cap_mem,
					need_rot,
					&mem_res,
					&mem_end,
					&channel_size);
		if (ret) {
			CMR_LOGE("raw fmt arrange failed!");
			return -1;
		}
	} else {
		if (IMG_DATA_TYPE_JPEG == orig_fmt) {
			ret = arrange_jpeg_buf(cap_2_frm,
						sn_size,
						sn_trim,
						image_size,
						orig_fmt,
						cap_size,
						thum_size,
						cap_mem,
						need_rot,
						&mem_res,
						&mem_end,
						&channel_size);
			if (ret) {
				CMR_LOGE("jpeg fmt arrange failed!");
				return -1;
			}
		} else {
#ifdef CONFIG_MEM_OPTIMIZATION
	ret = arrange_yuv_buf_optimization(cap_2_frm,
						sn_size,
						sn_trim,
						image_size,
						orig_fmt,
						cap_size,
						thum_size,
						cap_mem,
						need_rot,
						&mem_res,
						&mem_end,
						&channel_size);
#else
			ret = arrange_yuv_buf(cap_2_frm,
						sn_size,
						sn_trim,
						image_size,
						orig_fmt,
						cap_size,
						thum_size,
						cap_mem,
						need_rot,
						&mem_res,
						&mem_end,
						&channel_size);
#endif
			if (ret) {
				CMR_LOGE("yuv fmt arrange failed!");
				return -1;
			}
		}
	}

	CMR_LOGD("cap_raw, phy 0x%lx 0x%lx, vir 0x%lx 0x%lx, size 0x%x",
		cap_mem->cap_raw.addr_phy.addr_y,
		cap_mem->cap_raw.addr_phy.addr_u,
		cap_mem->cap_raw.addr_vir.addr_y,
		cap_mem->cap_raw.addr_vir.addr_u,
		cap_mem->cap_raw.buf_size);

	CMR_LOGD("target_yuv, phy 0x%lx 0x%lx, vir 0x%lx 0x%lx, size 0x%x",
		cap_mem->target_yuv.addr_phy.addr_y,
		cap_mem->target_yuv.addr_phy.addr_u,
		cap_mem->target_yuv.addr_vir.addr_y,
		cap_mem->target_yuv.addr_vir.addr_u,
		cap_mem->target_yuv.buf_size);

	CMR_LOGD("cap_yuv, phy 0x%lx 0x%lx, vir 0x%lx 0x%lx, size 0x%x",
		cap_mem->cap_yuv.addr_phy.addr_y,
		cap_mem->cap_yuv.addr_phy.addr_u,
		cap_mem->cap_yuv.addr_vir.addr_y,
		cap_mem->cap_yuv.addr_vir.addr_u,
		cap_mem->cap_yuv.buf_size);

	/* arrange misc buffers */
	ret = arrange_misc_buf(cap_2_frm,
				sn_size,
				sn_trim,
				image_size,
				orig_fmt,
				cap_size,
				thum_size,
				cap_mem,
				need_rot,
				&mem_res,
				&mem_end,
				&channel_size);
	if (ret) {
		CMR_LOGE("misc buf arrange failed!");
		return -1;
	}

	/* arrange rot buf */
	if (need_rot) {
		ret = arrange_rot_buf(cap_2_frm,
					sn_size,
					sn_trim,
					image_size,
					orig_fmt,
					cap_size,
					thum_size,
					cap_mem,
					need_rot,
					need_scale,
					&mem_res,
					&mem_end,
					&channel_size);
		if (ret) {
			CMR_LOGE("rot buf arrange failed!");
			return -1;
		}
	}

	CMR_LOGI("mem_end, mem_res: 0x%x 0x%x ", mem_end, mem_res);

	/* resize target jpeg buffer */
	cap_mem->target_jpeg.addr_phy.addr_y = cap_mem->target_jpeg.addr_phy.addr_y + JPEG_EXIF_SIZE;
	cap_mem->target_jpeg.addr_vir.addr_y = cap_mem->target_jpeg.addr_vir.addr_y + JPEG_EXIF_SIZE;
	cap_mem->target_jpeg.buf_size = cap_mem->target_jpeg.buf_size - JPEG_EXIF_SIZE;

	CMR_LOGD("target_jpeg, phy 0x%lx, vir 0x%lx, size 0x%x",
	cap_mem->target_jpeg.addr_phy.addr_y,
	cap_mem->target_jpeg.addr_vir.addr_y,
	cap_mem->target_jpeg.buf_size);


	for (i = 1; i < image_cnt; i++) {
		memcpy((void*)&capture_mem[i].cap_raw,
			(void*)&capture_mem[0].cap_raw,
			sizeof(struct img_frm));

		memcpy((void*)&capture_mem[i].target_yuv,
			(void*)&capture_mem[0].target_yuv,
			sizeof(struct img_frm));
		memcpy((void*)&capture_mem[i].cap_yuv,
			(void*)&capture_mem[0].cap_yuv,
			sizeof(struct img_frm));

		memcpy((void*)&capture_mem[i].target_jpeg,
			(void*)&capture_mem[0].target_jpeg,
			sizeof(struct img_frm));

		memcpy((void*)&capture_mem[i].thum_yuv,
			(void*)&capture_mem[0].thum_yuv,
			sizeof(struct img_frm));

		memcpy((void*)&capture_mem[i].thum_jpeg,
			(void*)&capture_mem[0].thum_jpeg,
			sizeof(struct img_frm));

		memcpy((void*)&capture_mem[i].jpeg_tmp,
			(void*)&capture_mem[0].jpeg_tmp,
			sizeof(struct img_frm));

		memcpy((void*)&capture_mem[i].scale_tmp,
			(void*)&capture_mem[0].scale_tmp,
			sizeof(struct img_frm));

		memcpy((void*)&capture_mem[i].cap_yuv_rot,
			(void*)&capture_mem[0].cap_yuv_rot,
			sizeof(struct img_frm));

		memcpy((void*)&capture_mem[i].isp_tmp,
			(void*)&capture_mem[0].isp_tmp,
			sizeof(struct img_frm));

		CMR_LOGD("Image ID %d", i);
		CMR_LOGD("cap_raw, phy 0x%lx 0x%lx, vir 0x%lx 0x%lx, size 0x%x",
			capture_mem[i].cap_raw.addr_phy.addr_y,
			capture_mem[i].cap_raw.addr_phy.addr_u,
			capture_mem[i].cap_raw.addr_vir.addr_y,
			capture_mem[i].cap_raw.addr_vir.addr_u,
			capture_mem[i].cap_raw.buf_size);

		CMR_LOGD("target_yuv, phy 0x%lx 0x%lx, vir 0x%lx 0x%lx, size 0x%x",
			capture_mem[i].target_yuv.addr_phy.addr_y,
			capture_mem[i].target_yuv.addr_phy.addr_u,
			capture_mem[i].target_yuv.addr_vir.addr_y,
			capture_mem[i].target_yuv.addr_vir.addr_u,
			capture_mem[i].target_yuv.buf_size);

		CMR_LOGD("cap_yuv, phy 0x%lx 0x%lx, vir 0x%lx 0x%lx, size 0x%x",
			capture_mem[i].cap_yuv.addr_phy.addr_y,
			capture_mem[i].cap_yuv.addr_phy.addr_u,
			capture_mem[i].cap_yuv.addr_vir.addr_y,
			capture_mem[i].cap_yuv.addr_vir.addr_u,
			capture_mem[i].cap_yuv.buf_size);

		CMR_LOGD("target_jpeg, phy 0x%lx, vir 0x%lx, size 0x%x",
			capture_mem[i].target_jpeg.addr_phy.addr_y,
			capture_mem[i].target_jpeg.addr_vir.addr_y,
			capture_mem[i].target_jpeg.buf_size);

		CMR_LOGD("thum_yuv, phy 0x%lx 0x%lx, vir 0x%lx 0x%lx, size 0x%x",
			capture_mem[i].thum_yuv.addr_phy.addr_y,
			capture_mem[i].thum_yuv.addr_phy.addr_u,
			capture_mem[i].thum_yuv.addr_vir.addr_y,
			capture_mem[i].thum_yuv.addr_vir.addr_u,
			capture_mem[i].thum_yuv.buf_size);

		CMR_LOGD("thum_jpeg, phy 0x%lx, vir 0x%lx, size 0x%x",
			capture_mem[i].thum_jpeg.addr_phy.addr_y,
			capture_mem[i].thum_jpeg.addr_vir.addr_y,
			capture_mem[i].thum_jpeg.buf_size);

		CMR_LOGD("scale_tmp, phy 0x%lx, vir 0x%lx, size 0x%x",
			capture_mem[i].scale_tmp.addr_phy.addr_y,
			capture_mem[i].scale_tmp.addr_vir.addr_y,
			capture_mem[i].scale_tmp.buf_size);
	}

	return 0;
}

int arrange_raw_buf(struct cmr_cap_2_frm *cap_2_frm,
				struct img_size *sn_size,
				struct img_rect *sn_trim,
				struct img_size *image_size,
				uint32_t orig_fmt,
				struct img_size *cap_size,
				struct img_size *thum_size,
				struct cmr_cap_mem *capture_mem,
				uint32_t need_rot,
				uint32_t *io_mem_res,
				uint32_t *io_mem_end,
				uint32_t *io_channel_size)
{
	UNUSED(thum_size);

	uint32_t       channel_size;
	uint32_t       mem_res = 0, mem_end = 0;
	uint32_t       offset = 0;
	uint32_t       y_to_raw = 0, yy_to_y = 0, tmp = 0, raw_size = 0;
	uint32_t       uv_size = 0, useless_raw = 0;
	struct cmr_cap_mem *cap_mem = capture_mem;/*&capture_mem[0];*/
	struct img_size align16_image_size, align16_cap_size, sn_align_size;
	uint32_t yy_to_y2 = 0;

	uint32_t       tmp_img_size, tmp_cap_size, tmp_sn_size, max_size;

	if (IMG_DATA_TYPE_RAW != orig_fmt ||
		NULL == io_mem_res ||
		NULL == io_mem_end ||
		NULL == io_channel_size) {
		return -1;
	}
	CMR_LOGI("raw fmt buf arrange");

	align16_image_size.width = camera_get_aligned_size(cap_2_frm->type, image_size->width);
	align16_image_size.height = camera_get_aligned_size(cap_2_frm->type, image_size->height);
	align16_cap_size.width = camera_get_aligned_size(cap_2_frm->type, cap_size->width);
	align16_cap_size.height = camera_get_aligned_size(cap_2_frm->type, cap_size->height);
	sn_align_size.width = camera_get_aligned_size(cap_2_frm->type, sn_size->width);
	sn_align_size.height = camera_get_aligned_size(cap_2_frm->type, sn_size->height);

	mem_res = *io_mem_res;
	mem_end = *io_mem_end;
	//channel_size = *io_channel_size;

	channel_size = (uint32_t)(sn_size->width * sn_size->height);
	raw_size = (uint32_t)(channel_size * RAWRGB_BIT_WIDTH / 8);
	y_to_raw = (uint32_t)(ISP_YUV_TO_RAW_GAP * sn_size->width);
	uv_size = (channel_size >> 1);

#ifdef CONFIG_MEM_OPTIMIZATION
	CMR_LOGV("align16_image_size.width = %d, align16_image_size.height=%d ", align16_image_size.width, align16_image_size.height);
	CMR_LOGV("align16_cap_size.width = %d, align16_cap_size.height=%d ", align16_cap_size.width, align16_cap_size.height);
	CMR_LOGV("sn_align_size.width = %d, sn_align_size.height=%d ", sn_align_size.width, sn_align_size.height);

	tmp_img_size = (uint32_t)CMR_ADDR_ALIGNED((align16_image_size.width * align16_image_size.height));
	tmp_cap_size = (uint32_t)CMR_ADDR_ALIGNED((align16_cap_size.width * align16_cap_size.height));
	tmp_sn_size = (uint32_t)CMR_ADDR_ALIGNED((sn_align_size.width * sn_align_size.height));
	max_size = tmp_img_size > tmp_cap_size ? tmp_img_size: tmp_cap_size;
	max_size = max_size > tmp_sn_size ? max_size : tmp_sn_size;

	CMR_LOGV("tmp_img_size=%d, tmp_cap_size=%d, tmp_sn_size=%d", tmp_img_size, tmp_cap_size, tmp_sn_size);

	cap_mem->target_yuv.addr_phy.addr_y = cap_2_frm->mem_frm.addr_phy.addr_y;
	cap_mem->target_yuv.addr_vir.addr_y = cap_2_frm->mem_frm.addr_vir.addr_y;
	cap_mem->target_yuv.addr_phy.addr_u = cap_mem->target_yuv.addr_phy.addr_y + tmp_img_size;
	cap_mem->target_yuv.addr_vir.addr_u = cap_mem->target_yuv.addr_vir.addr_y + tmp_img_size;

	cap_mem->cap_yuv.addr_phy.addr_y = cap_2_frm->mem_frm.addr_phy.addr_y + max_size * 3 / 2;
	cap_mem->cap_yuv.addr_vir.addr_y = cap_2_frm->mem_frm.addr_vir.addr_y + max_size * 3 / 2;
	cap_mem->cap_yuv.addr_phy.addr_u = cap_mem->cap_yuv.addr_phy.addr_y + tmp_cap_size;
	cap_mem->cap_yuv.addr_vir.addr_u = cap_mem->cap_yuv.addr_vir.addr_y + tmp_cap_size;

	cap_mem->cap_raw.addr_phy.addr_y = cap_mem->target_yuv.addr_phy.addr_y;
	cap_mem->cap_raw.addr_vir.addr_y = cap_mem->target_yuv.addr_vir.addr_y;
	cap_mem->cap_raw.buf_size = raw_size;

	cap_mem->target_jpeg.addr_phy.addr_y = cap_mem->cap_yuv.addr_phy.addr_y;
	cap_mem->target_jpeg.addr_vir.addr_y = cap_mem->cap_yuv.addr_vir.addr_y;

	cap_mem->cap_yuv.buf_size = (tmp_cap_size * 3) >> 1;
	cap_mem->cap_yuv.size.width = cap_size->width;
	cap_mem->cap_yuv.size.height = cap_size->height;
	cap_mem->cap_yuv.fmt = IMG_DATA_TYPE_YUV420;

	cap_mem->cap_raw.size.width = sn_size->width;
	cap_mem->cap_raw.size.height = sn_size->height;
	cap_mem->cap_raw.fmt = IMG_DATA_TYPE_RAW;

	cap_mem->target_yuv.buf_size = (tmp_img_size * 3) >> 1;
	cap_mem->target_yuv.size.width = align16_image_size.width;
	cap_mem->target_yuv.size.height = align16_image_size.height;
	cap_mem->target_yuv.fmt = IMG_DATA_TYPE_YUV420;

	/* update io param */
	*io_mem_res = mem_res-max_size * 3 / 2 - max_size * 3 / 2;
	*io_mem_end = max_size * 3;
	*io_channel_size = channel_size;

	//goto raw_buf_bypass;
#else

	if (align16_image_size.width != sn_align_size.width ||
		align16_image_size.height != sn_align_size.height) {
		if ((uint32_t)(align16_image_size.width * align16_image_size.height) >
			(uint32_t)(sn_align_size.width * sn_align_size.height)) {
			/*if interpolation needed*/
			yy_to_y = (uint32_t)(align16_image_size.width * align16_image_size.height) -
				(uint32_t)(sn_align_size.width * sn_align_size.height);
			if (yy_to_y < (uint32_t)(ISP_YUV_TO_RAW_GAP * sn_size->width)) {
				yy_to_y = (uint32_t)(ISP_YUV_TO_RAW_GAP * sn_size->width);
			}
			useless_raw = (uint32_t)(sn_align_size.width * sn_align_size.height * RAWRGB_BIT_WIDTH / 8);
			useless_raw = useless_raw - (uint32_t)(sn_align_size.width * (sn_align_size.height - ISP_YUV_TO_RAW_GAP));
		} else {
			yy_to_y = (uint32_t)(ISP_YUV_TO_RAW_GAP * sn_size->width);
			useless_raw = (uint32_t)(yy_to_y * RAWRGB_BIT_WIDTH / 8);
		}
		uv_size = uv_size + (yy_to_y >> 1);
	} else if (sn_trim && (sn_trim->start_y ||
		sn_trim->width != sn_size->width ||
		sn_trim->height != sn_size->height)) {
		yy_to_y2 = (uint32_t)(image_size->width * image_size->height) -
				    (uint32_t)(cap_size->width * cap_size->height);
		if (yy_to_y2 < (uint32_t)(ISP_YUV_TO_RAW_GAP * sn_size->width)) {
			yy_to_y2 = (uint32_t)(ISP_YUV_TO_RAW_GAP * sn_size->width);
		}

		tmp = sn_size->height - sn_trim->height - sn_trim->start_y;
		CMR_LOGI("Recovered height, %d", tmp);
		yy_to_y = (uint32_t)(tmp * sn_size->width);
		yy_to_y = MAX(yy_to_y2, yy_to_y);
		uv_size = uv_size + (yy_to_y >> 1);
		useless_raw = (uint32_t)(yy_to_y * RAWRGB_BIT_WIDTH / 8);
	}
	cap_mem->target_yuv.addr_phy.addr_y = cap_2_frm->mem_frm.addr_phy.addr_y;
	cap_mem->target_yuv.addr_vir.addr_y = cap_2_frm->mem_frm.addr_vir.addr_y;
	cap_mem->cap_yuv.addr_phy.addr_y = cap_mem->target_yuv.addr_phy.addr_y + yy_to_y;
	cap_mem->cap_yuv.addr_vir.addr_y = cap_mem->target_yuv.addr_vir.addr_y + yy_to_y;
	cap_mem->cap_raw.addr_phy.addr_y = cap_mem->cap_yuv.addr_phy.addr_y + y_to_raw;
	cap_mem->cap_raw.addr_vir.addr_y = cap_mem->cap_yuv.addr_vir.addr_y + y_to_raw;
	cap_mem->cap_raw.buf_size = raw_size;
	CMR_LOGD("y_to_raw 0x%x, yy_to_y 0x%x, raw size 0x%x useless_raw 0x%x",
		y_to_raw, yy_to_y, raw_size, useless_raw);

	offset = raw_size + y_to_raw + yy_to_y - useless_raw;/*the end of RawRGB*/
	CMR_NO_MEM(offset, mem_res);
	mem_end += offset;
	mem_res -= offset;

	if (!need_rot) {
		CMR_NO_MEM(cap_mem->target_jpeg.buf_size, mem_res);
		cap_mem->target_jpeg.addr_phy.addr_y = cap_2_frm->mem_frm.addr_phy.addr_y + mem_end;
		cap_mem->target_jpeg.addr_vir.addr_y = cap_2_frm->mem_frm.addr_vir.addr_y + mem_end;
		mem_end += cap_mem->target_jpeg.buf_size;
		mem_res -= cap_mem->target_jpeg.buf_size;
	}

	/* start get UV buffer */
	if (uv_size < mem_res) {
		cap_mem->target_yuv.addr_phy.addr_u = cap_2_frm->mem_frm.addr_phy.addr_y + mem_end;
		cap_mem->target_yuv.addr_vir.addr_u = cap_2_frm->mem_frm.addr_vir.addr_y + mem_end;
		mem_end += uv_size;
		mem_res -= uv_size;
	} else {
		CMR_LOGI("No more memory reseved in buffer, need to alloc target YUV uv buffer!");
		unsigned long addr_phy, addr_vir;
		if (cap_2_frm->alloc_mem(cap_2_frm->handle, uv_size, &addr_phy, &addr_vir) != 0) {
			CMR_LOGE("Failed to alloc the buffer used in capture");
			return -1;
		}
		cap_mem->target_yuv.addr_phy.addr_u = addr_phy;
		cap_mem->target_yuv.addr_vir.addr_u = addr_vir;
	}
	cap_mem->cap_yuv.addr_phy.addr_u = cap_mem->target_yuv.addr_phy.addr_u + (yy_to_y >> 1);
	cap_mem->cap_yuv.addr_vir.addr_u = cap_mem->target_yuv.addr_vir.addr_u + (yy_to_y >> 1);
	/* end get UV buffer */

	cap_mem->cap_yuv.buf_size = (channel_size * 3) >> 1;
	cap_mem->cap_yuv.size.width = sn_size->width;
	cap_mem->cap_yuv.size.height = sn_size->height;
	cap_mem->cap_yuv.fmt = IMG_DATA_TYPE_YUV420;

	cap_mem->cap_raw.size.width = sn_size->width;
	cap_mem->cap_raw.size.height = sn_size->height;
	cap_mem->cap_raw.fmt = IMG_DATA_TYPE_RAW;

	cap_mem->target_yuv.buf_size = (channel_size * 3) >> 1;
	cap_mem->target_yuv.size.width = align16_image_size.width;
	cap_mem->target_yuv.size.height = align16_image_size.height;
	cap_mem->target_yuv.fmt = IMG_DATA_TYPE_YUV420;

	/* update io param */
	*io_mem_res = mem_res-CMR_YUV_BUF_GAP*align16_image_size.width;
	*io_mem_end = mem_end+CMR_YUV_BUF_GAP*align16_image_size.width;
	*io_channel_size = channel_size;
#endif
//raw_buf_bypass:
	return 0;
}

int arrange_jpeg_buf(struct cmr_cap_2_frm *cap_2_frm,
				 struct img_size *sn_size,
				 struct img_rect *sn_trim,
				 struct img_size *image_size,
				 uint32_t orig_fmt,
				 struct img_size *cap_size,
				 struct img_size *thum_size,
				 struct cmr_cap_mem *capture_mem,
				 uint32_t need_rot,
				 uint32_t *io_mem_res,
				 uint32_t *io_mem_end,
				 uint32_t *io_channel_size)
{
	UNUSED(thum_size);

	uint32_t       channel_size;
	uint32_t       mem_res = 0, mem_end = 0;
	uint32_t       offset = 0;
	uint32_t       yy_to_y = 0, tmp = 0;
	uint32_t       y_end = 0, uv_end = 0;
	uint32_t       gap_size = 0;

	struct cmr_cap_mem *cap_mem = capture_mem;/*&capture_mem[0];*/
	struct img_size align16_image_size, align16_cap_size;

	if (IMG_DATA_TYPE_JPEG != orig_fmt ||
		NULL == io_mem_res ||
		NULL == io_mem_end ||
		NULL == io_channel_size) {
		return -1;
	}
	CMR_LOGI("jpeg fmt buf arrange");

	align16_image_size.width = camera_get_aligned_size(cap_2_frm->type, image_size->width);
	align16_image_size.height = camera_get_aligned_size(cap_2_frm->type, image_size->height);
	align16_cap_size.width = camera_get_aligned_size(cap_2_frm->type, cap_size->width);
	align16_cap_size.height = camera_get_aligned_size(cap_2_frm->type, cap_size->height);

	mem_res = *io_mem_res;
	mem_end = *io_mem_end;
	//channel_size = *io_channel_size;

	channel_size = (uint32_t)(align16_image_size.width * align16_image_size.height);
	cap_mem->target_yuv.addr_phy.addr_y = cap_2_frm->mem_frm.addr_phy.addr_y;
	cap_mem->target_yuv.addr_vir.addr_y = cap_2_frm->mem_frm.addr_vir.addr_y;

	yy_to_y = (uint32_t)((sn_size->height - sn_trim->start_y - sn_trim->height) * sn_size->width);
	tmp = (uint32_t)(sn_size->height * sn_size->width);
	y_end = yy_to_y + MAX(channel_size, tmp);

	CMR_LOGI("yy_to_y, 0x%x, tmp 0x%x", yy_to_y, tmp);

	cap_mem->target_yuv.addr_phy.addr_u = cap_mem->target_yuv.addr_phy.addr_y + y_end;
	cap_mem->target_yuv.addr_vir.addr_u = cap_mem->target_yuv.addr_vir.addr_y + y_end;
	cap_mem->cap_yuv.addr_phy.addr_y = cap_mem->target_yuv.addr_phy.addr_u - tmp;
	/*to confirm the gap of scaling source and dest should be large than MIN_GAP_LINES*/
	if ((cap_mem->cap_yuv.addr_phy.addr_y - cap_mem->target_yuv.addr_phy.addr_y) <
		(MIN_GAP_LINES*align16_image_size.width)) {
		gap_size = (MIN_GAP_LINES*align16_image_size.width) - (cap_mem->cap_yuv.addr_phy.addr_y - cap_mem->target_yuv.addr_phy.addr_y);
		gap_size = (gap_size+ALIGN_PAGE_SIZE-1)&(~(ALIGN_PAGE_SIZE-1));
	}

	cap_mem->cap_yuv.addr_phy.addr_y += gap_size;
	cap_mem->cap_yuv.addr_vir.addr_y = cap_mem->target_yuv.addr_vir.addr_u - tmp + gap_size;

	cap_mem->target_yuv.addr_phy.addr_u += gap_size;
	cap_mem->target_yuv.addr_vir.addr_u += gap_size;

	cap_mem->cap_yuv.addr_phy.addr_u = cap_mem->target_yuv.addr_phy.addr_u + y_end - tmp + gap_size;
	cap_mem->cap_yuv.addr_vir.addr_u = cap_mem->target_yuv.addr_vir.addr_u + y_end - tmp + gap_size;

	if (need_rot) {
		offset = ((uint32_t)(y_end * 3) >> 1) + 2 * gap_size;
	} else {
		offset = ((y_end) << 1) + 2 * gap_size;
	}

	CMR_NO_MEM(offset, mem_res);
	mem_end = offset;
	mem_res = mem_res - mem_end;

	if (!need_rot) {
		CMR_NO_MEM(cap_mem->target_jpeg.buf_size, mem_res);
		cap_mem->target_jpeg.addr_phy.addr_y = cap_2_frm->mem_frm.addr_phy.addr_y + mem_end;
		cap_mem->target_jpeg.addr_vir.addr_y = cap_2_frm->mem_frm.addr_vir.addr_y + mem_end;
		mem_end += cap_mem->target_jpeg.buf_size;
		mem_res -= cap_mem->target_jpeg.buf_size;
	}

	cap_mem->cap_yuv.buf_size = channel_size * 3 / 2;
	cap_mem->cap_yuv.size.width = align16_cap_size.width;
	cap_mem->cap_yuv.size.height = align16_cap_size.height;
	cap_mem->cap_yuv.fmt = IMG_DATA_TYPE_YUV420;

	cap_mem->target_yuv.buf_size = channel_size * 3 / 2;
	cap_mem->target_yuv.size.width = align16_image_size.width;
	cap_mem->target_yuv.size.height = align16_image_size.height;
	cap_mem->target_yuv.fmt = IMG_DATA_TYPE_YUV420;

	/* update io param */
	*io_mem_res = mem_res;
	*io_mem_end = mem_end;
	*io_channel_size = channel_size;

	return 0;
}

int arrange_yuv_buf(struct cmr_cap_2_frm *cap_2_frm,
				struct img_size *sn_size,
				struct img_rect *sn_trim,
				struct img_size *image_size,
				uint32_t orig_fmt,
				struct img_size *cap_size,
				struct img_size *thum_size,
				struct cmr_cap_mem *capture_mem,
				uint32_t need_rot,
				uint32_t *io_mem_res,
				uint32_t *io_mem_end,
				uint32_t *io_channel_size)
{
	//UNUSED(sn_size);
	UNUSED(sn_trim);
	UNUSED(thum_size);

	uint32_t max_size = 0;
	uint32_t       channel_size;
	uint32_t       mem_res = 0, mem_end = 0;
	uint32_t       offset = 0;
	uint32_t       yy_to_y = 0, tmp = 0;
	uint32_t       flag = 0;
	struct cmr_cap_mem *cap_mem = capture_mem;/*&capture_mem[0];*/
	struct img_size align16_image_size, align16_cap_size;

	if (IMG_DATA_TYPE_JPEG == orig_fmt ||
		IMG_DATA_TYPE_RAW == orig_fmt ||
		NULL == io_mem_res ||
		NULL == io_mem_end ||
		NULL == io_channel_size) {
		return -1;
	}
	CMR_LOGI("yuv fmt buf arrange");

	CMR_LOGI("sn_size width %d height %d, sn_trim start_x %d start_y %d width %d height %d", sn_size->width, sn_size->height,
		sn_trim->start_x, sn_trim->start_y, sn_trim->width, sn_trim->height);
	align16_image_size.width = camera_get_aligned_size(cap_2_frm->type, image_size->width);
	align16_image_size.height = camera_get_aligned_size(cap_2_frm->type, image_size->height);
	align16_cap_size.width = camera_get_aligned_size(cap_2_frm->type, cap_size->width);
	align16_cap_size.height = camera_get_aligned_size(cap_2_frm->type, cap_size->height);

	mem_res = *io_mem_res;
	mem_end = *io_mem_end;
	//channel_size = *io_channel_size;

	channel_size = (uint32_t)CMR_ADDR_ALIGNED((align16_image_size.width * align16_image_size.height));
	tmp = (uint32_t)CMR_ADDR_ALIGNED((cap_size->width * cap_size->height));
	max_size = (channel_size > tmp ? channel_size: tmp);

	if (channel_size > tmp) {
		/*need scaling up*/
		cap_mem->target_yuv.addr_phy.addr_y = cap_2_frm->mem_frm.addr_phy.addr_y;
		cap_mem->target_yuv.addr_vir.addr_y = cap_2_frm->mem_frm.addr_vir.addr_y;
		cap_mem->target_yuv.addr_phy.addr_u = cap_mem->target_yuv.addr_phy.addr_y + channel_size;
		cap_mem->target_yuv.addr_vir.addr_u = cap_mem->target_yuv.addr_vir.addr_y + channel_size;
		cap_mem->target_yuv.addr_phy.addr_v = 0;
		cap_mem->cap_yuv.addr_phy.addr_y = cap_mem->target_yuv.addr_phy.addr_u - tmp;
		cap_mem->cap_yuv.addr_vir.addr_y = cap_mem->target_yuv.addr_vir.addr_u - tmp;
		cap_mem->cap_yuv.addr_phy.addr_u = cap_mem->target_yuv.addr_phy.addr_u + ((channel_size - tmp) >> 1);
		cap_mem->cap_yuv.addr_vir.addr_u = cap_mem->target_yuv.addr_vir.addr_u + ((channel_size - tmp) >> 1);
		cap_mem->cap_yuv.addr_phy.addr_v = 0;
		yy_to_y = channel_size - tmp;
	} else {
		if ( ZOOM_POST_PROCESS_WITH_TRIM == cap_2_frm->zoom_post_proc) {
			/*means scaling down not on the fly*/
			flag = 1;
		} else {
			/*means scaling down*/
			offset = (tmp * 3) >> 1;
			cap_mem->cap_yuv.addr_phy.addr_y = cap_2_frm->mem_frm.addr_phy.addr_y;
			cap_mem->cap_yuv.addr_vir.addr_y = cap_2_frm->mem_frm.addr_vir.addr_y;
			cap_mem->cap_yuv.addr_phy.addr_u = cap_mem->cap_yuv.addr_phy.addr_y + tmp;
			cap_mem->cap_yuv.addr_vir.addr_u = cap_mem->cap_yuv.addr_vir.addr_y + tmp;
			cap_mem->cap_yuv.addr_phy.addr_v = 0;
			memcpy((void*)&cap_mem->target_yuv, (void*)&cap_mem->cap_yuv, sizeof(struct img_frm));
			channel_size = tmp;
			yy_to_y = 0;
		}
	}

	if (ZOOM_POST_PROCESS == cap_2_frm->zoom_post_proc || flag) {
		cap_mem->target_yuv.addr_phy.addr_y = cap_2_frm->mem_frm.addr_phy.addr_y;
		cap_mem->target_yuv.addr_vir.addr_y = cap_2_frm->mem_frm.addr_vir.addr_y;
		cap_mem->target_yuv.addr_phy.addr_u = cap_mem->target_yuv.addr_phy.addr_y + channel_size;
		cap_mem->target_yuv.addr_vir.addr_u = cap_mem->target_yuv.addr_vir.addr_y + channel_size;
		cap_mem->target_yuv.addr_phy.addr_v = 0;
#ifdef CONFIG_MEM_OPTIMIZATION
		cap_mem->cap_yuv.addr_phy.addr_y = cap_mem->target_yuv.addr_phy.addr_y + (uint32_t)((max_size * 3) >> 1);
		cap_mem->cap_yuv.addr_vir.addr_y = cap_mem->target_yuv.addr_vir.addr_y + (uint32_t)((max_size * 3) >> 1);
#else
		cap_mem->cap_yuv.addr_phy.addr_y = cap_mem->target_yuv.addr_phy.addr_y + (uint32_t)((channel_size * 3) >> 1);
		cap_mem->cap_yuv.addr_vir.addr_y = cap_mem->target_yuv.addr_vir.addr_y + (uint32_t)((channel_size * 3) >> 1);
#endif
		cap_mem->cap_yuv.addr_phy.addr_u = cap_mem->cap_yuv.addr_phy.addr_y + tmp;
		cap_mem->cap_yuv.addr_vir.addr_u = cap_mem->cap_yuv.addr_vir.addr_y + tmp;
		cap_mem->cap_yuv.addr_phy.addr_v = 0;
#ifdef CONFIG_MEM_OPTIMIZATION
		offset = (uint32_t)(((max_size * 3) >> 1) + ((max_size * 3) >> 1));
#else
		offset = (uint32_t)(((channel_size * 3) >> 1) + ((tmp * 3) >> 1));
#endif
	} else {
		offset = ((channel_size * 3) >> 1)+CMR_YUV_BUF_GAP*align16_image_size.width;
	}
	CMR_LOGI("offset %d", offset);

	CMR_NO_MEM(offset, mem_res);
	mem_end = CMR_ADDR_ALIGNED(offset);
	mem_res = mem_res - mem_end;

	if (ZOOM_POST_PROCESS == cap_2_frm->zoom_post_proc || flag) {
		cap_mem->target_jpeg.addr_phy.addr_y = cap_mem->cap_yuv.addr_phy.addr_y;
		cap_mem->target_jpeg.addr_vir.addr_y = cap_mem->cap_yuv.addr_vir.addr_y;
	} else {
		if (!need_rot) {
			CMR_NO_MEM(cap_mem->target_jpeg.buf_size, mem_res);
			cap_mem->target_jpeg.addr_phy.addr_y = cap_2_frm->mem_frm.addr_phy.addr_y + mem_end;
			cap_mem->target_jpeg.addr_vir.addr_y = cap_2_frm->mem_frm.addr_vir.addr_y + mem_end;
			mem_end += CMR_ADDR_ALIGNED(cap_mem->target_jpeg.buf_size);
			mem_res -= CMR_ADDR_ALIGNED(cap_mem->target_jpeg.buf_size);
		}
	}

	cap_mem->cap_yuv.buf_size = (channel_size * 3) >> 1;
	cap_mem->cap_yuv.size.width = align16_cap_size.width;
	cap_mem->cap_yuv.size.height = align16_cap_size.height;
	cap_mem->cap_yuv.fmt = IMG_DATA_TYPE_YUV420;

	cap_mem->target_yuv.buf_size = (channel_size * 3) >> 1;
	cap_mem->target_yuv.size.width = align16_image_size.width;
	cap_mem->target_yuv.size.height = align16_image_size.height;
	cap_mem->target_yuv.fmt = IMG_DATA_TYPE_YUV420;

	/* update io param */
	*io_mem_res = mem_res;
	*io_mem_end = mem_end;
	*io_channel_size = channel_size;

	return 0;
}

int arrange_yuv_buf_optimization(struct cmr_cap_2_frm *cap_2_frm,
				struct img_size *sn_size,
				struct img_rect *sn_trim,
				struct img_size *image_size,
				uint32_t orig_fmt,
				struct img_size *cap_size,
				struct img_size *thum_size,
				struct cmr_cap_mem *capture_mem,
				uint32_t need_rot,
				uint32_t *io_mem_res,
				uint32_t *io_mem_end,
				uint32_t *io_channel_size)
{
	//UNUSED(sn_size);
	UNUSED(sn_trim);
	UNUSED(thum_size);

	uint32_t max_size = 0;
	uint32_t       channel_size;
	uint32_t       mem_res = 0, mem_end = 0;
	uint32_t       offset = 0;
	uint32_t       tmp = 0;
	struct cmr_cap_mem *cap_mem = capture_mem;/*&capture_mem[0];*/
	struct img_size align16_image_size, align16_cap_size;

	if (IMG_DATA_TYPE_JPEG == orig_fmt ||
		IMG_DATA_TYPE_RAW == orig_fmt ||
		NULL == io_mem_res ||
		NULL == io_mem_end ||
		NULL == io_channel_size) {
		return -1;
	}
	CMR_LOGI("yuv fmt buf arrange");

	CMR_LOGI("sn_size width %d height %d, sn_trim start_x %d start_y %d width %d height %d", sn_size->width, sn_size->height,
		sn_trim->start_x, sn_trim->start_y, sn_trim->width, sn_trim->height);
	align16_image_size.width = camera_get_aligned_size(cap_2_frm->type, image_size->width);
	align16_image_size.height = camera_get_aligned_size(cap_2_frm->type, image_size->height);
	align16_cap_size.width = camera_get_aligned_size(cap_2_frm->type, cap_size->width);
	align16_cap_size.height = camera_get_aligned_size(cap_2_frm->type, cap_size->height);

	mem_res = *io_mem_res;
	mem_end = *io_mem_end;
	//channel_size = *io_channel_size;

	channel_size = (uint32_t)CMR_ADDR_ALIGNED((align16_image_size.width * align16_image_size.height));
	tmp = (uint32_t)CMR_ADDR_ALIGNED((cap_size->width * cap_size->height));
	max_size = (channel_size > tmp ? channel_size: tmp);


	cap_mem->target_yuv.addr_phy.addr_y = cap_2_frm->mem_frm.addr_phy.addr_y;
	cap_mem->target_yuv.addr_vir.addr_y = cap_2_frm->mem_frm.addr_vir.addr_y;
	cap_mem->target_yuv.addr_phy.addr_u = cap_mem->target_yuv.addr_phy.addr_y + max_size;
	cap_mem->target_yuv.addr_vir.addr_u = cap_mem->target_yuv.addr_vir.addr_y + max_size;
	cap_mem->target_yuv.addr_phy.addr_v = 0;

	cap_mem->cap_yuv.addr_phy.addr_y = cap_mem->target_yuv.addr_phy.addr_y + (uint32_t)((max_size * 3) >> 1);
	cap_mem->cap_yuv.addr_vir.addr_y = cap_mem->target_yuv.addr_vir.addr_y + (uint32_t)((max_size * 3) >> 1);
	cap_mem->cap_yuv.addr_phy.addr_u = cap_mem->cap_yuv.addr_phy.addr_y + max_size;
	cap_mem->cap_yuv.addr_vir.addr_u = cap_mem->cap_yuv.addr_vir.addr_y + max_size;
	cap_mem->cap_yuv.addr_phy.addr_v = 0;

	offset = (uint32_t)(((max_size * 3) >> 1) + ((max_size * 3) >> 1));
	CMR_LOGI("offset %d", offset);
	CMR_NO_MEM(offset, mem_res);
	mem_end = CMR_ADDR_ALIGNED(offset);
	mem_res = mem_res - mem_end;

	cap_mem->cap_yuv.buf_size = (max_size * 3) >> 1;
	cap_mem->cap_yuv.size.width = align16_cap_size.width;
	cap_mem->cap_yuv.size.height = align16_cap_size.height;
	cap_mem->cap_yuv.fmt = IMG_DATA_TYPE_YUV420;

	cap_mem->target_yuv.buf_size = (max_size * 3) >> 1;
	cap_mem->target_yuv.size.width = align16_image_size.width;
	cap_mem->target_yuv.size.height = align16_image_size.height;
	cap_mem->target_yuv.fmt = IMG_DATA_TYPE_YUV420;

	cap_mem->target_jpeg.addr_phy.addr_y = cap_mem->cap_yuv.addr_phy.addr_y;
	cap_mem->target_jpeg.addr_vir.addr_y = cap_mem->cap_yuv.addr_vir.addr_y;

	CMR_LOGI("target_jpeg vir yaddr 0x%x, phy yaddr 0x%x",
		cap_mem->target_jpeg.addr_vir.addr_y,
		cap_mem->target_jpeg.addr_phy.addr_y);

	/* update io param */
	*io_mem_res = mem_res;
	*io_mem_end = mem_end;
	*io_channel_size = channel_size;

	return 0;
}

int arrange_misc_buf(struct cmr_cap_2_frm *cap_2_frm,
				 struct img_size *sn_size,
				 struct img_rect *sn_trim,
				 struct img_size *image_size,
				 uint32_t orig_fmt,
				 struct img_size *cap_size,
				 struct img_size *thum_size,
				 struct cmr_cap_mem *capture_mem,
				 uint32_t need_rot,
				 uint32_t *io_mem_res,
				 uint32_t *io_mem_end,
				 uint32_t *io_channel_size)
{
	UNUSED(sn_size);
	UNUSED(sn_trim);
	UNUSED(orig_fmt);
	UNUSED(need_rot);

	uint32_t       size_pixel;
	uint32_t       mem_res = 0, mem_end = 0;
	uint32_t       i = 0;
	struct cmr_cap_mem *cap_mem = capture_mem;/*&capture_mem[0];*/
	struct img_frm img_frame[BUF_TYPE_NUM];
	struct img_size align16_image_size, align16_cap_size;

	if (NULL == io_mem_res ||
		NULL == io_mem_end ||
		NULL == io_channel_size) {
		return -1;
	}

	align16_image_size.width = camera_get_aligned_size(cap_2_frm->type, image_size->width);
	align16_image_size.height = camera_get_aligned_size(cap_2_frm->type, image_size->height);
	align16_cap_size.width = camera_get_aligned_size(cap_2_frm->type, cap_size->width);
	align16_cap_size.height = camera_get_aligned_size(cap_2_frm->type, cap_size->height);

	mem_res = *io_mem_res;
	mem_end = *io_mem_end;

	CMR_LOGI("Now to alloc misc buffers");
	for (i = THUM_YUV; i < BUF_TYPE_NUM; i++) {
		/* calculate the address of target_jpeg, start */
		size_pixel = get_size[i](align16_image_size.width, align16_image_size.height, thum_size->width, thum_size->height);
		if (mem_res >= size_pixel) {
			img_frame[i].buf_size = size_pixel;
			img_frame[i].addr_phy.addr_y = cap_2_frm->mem_frm.addr_phy.addr_y + mem_end;
			img_frame[i].addr_vir.addr_y = cap_2_frm->mem_frm.addr_vir.addr_y + mem_end;
			img_frame[i].addr_phy.addr_u = img_frame[i].addr_phy.addr_y + size_pixel * 2 / 3;
			img_frame[i].addr_vir.addr_u = img_frame[i].addr_vir.addr_y + size_pixel * 2 / 3;
			/* re-calculate the currend end of mem */
			mem_res -= size_pixel;
			mem_end += size_pixel;
		} else {
			break;
		}
	}

	if (i != BUF_TYPE_NUM) {
		CMR_LOGI("No more memory reseved in buffer, to alloc misc buffers");
		/* Not all the misc buffer have been alloc-ed yet*/
		for (; i < BUF_TYPE_NUM; i++) {
			unsigned long addr_phy, addr_vir;
			/* calculate the address of target_jpeg, start */
			size_pixel = get_size[i](align16_image_size.width, align16_image_size.height, thum_size->width, thum_size->height);
			if (cap_2_frm->alloc_mem(cap_2_frm->handle, size_pixel, &addr_phy, &addr_vir) != 0) {
				CMR_LOGE("Failed to alloc the buffer used in capture");
				return -1;
			}

			img_frame[i].buf_size = size_pixel;
			img_frame[i].addr_phy.addr_y = addr_phy;
			img_frame[i].addr_vir.addr_y = addr_vir;
			img_frame[i].addr_phy.addr_u = img_frame[i].addr_phy.addr_y + size_pixel * 2 / 3;
			img_frame[i].addr_vir.addr_u = img_frame[i].addr_vir.addr_y + size_pixel * 2 / 3;
		}
	}

	if (i != BUF_TYPE_NUM) {
		CMR_LOGE("Failed to alloc all the buffers used in capture");
		return -1;
	}

	cap_mem->thum_yuv.buf_size = img_frame[THUM_YUV].buf_size;
	cap_mem->thum_yuv.addr_phy.addr_y = img_frame[THUM_YUV].addr_phy.addr_y;
	cap_mem->thum_yuv.addr_vir.addr_y = img_frame[THUM_YUV].addr_vir.addr_y;
	cap_mem->thum_yuv.addr_phy.addr_u = img_frame[THUM_YUV].addr_phy.addr_u;
	cap_mem->thum_yuv.addr_vir.addr_u = img_frame[THUM_YUV].addr_vir.addr_u;
	cap_mem->thum_yuv.size.width = thum_size->width;
	cap_mem->thum_yuv.size.height = thum_size->height;
	CMR_LOGD("thum_yuv, phy 0x%lx 0x%lx, vir 0x%lx 0x%lx, size 0x%x",
		img_frame[THUM_YUV].addr_phy.addr_y,
		img_frame[THUM_YUV].addr_phy.addr_u,
		img_frame[THUM_YUV].addr_vir.addr_y,
		img_frame[THUM_YUV].addr_vir.addr_u,
		img_frame[THUM_YUV].buf_size);

	cap_mem->thum_jpeg.buf_size = img_frame[THUM_JPEG].buf_size;
	cap_mem->thum_jpeg.addr_phy.addr_y = img_frame[THUM_JPEG].addr_phy.addr_y;
	cap_mem->thum_jpeg.addr_vir.addr_y = img_frame[THUM_JPEG].addr_vir.addr_y;
	CMR_LOGD("thum_jpeg, phy 0x%lx, vir 0x%lx, size 0x%x",
		img_frame[THUM_JPEG].addr_phy.addr_y,
		img_frame[THUM_JPEG].addr_vir.addr_y,
		img_frame[THUM_JPEG].buf_size);

	/* mem reuse, jpeg_tmp/uv */
	cap_mem->jpeg_tmp.buf_size = cap_mem->cap_yuv.buf_size;
	cap_mem->jpeg_tmp.addr_phy.addr_y = cap_mem->cap_yuv.addr_phy.addr_u;
	cap_mem->jpeg_tmp.addr_vir.addr_y = cap_mem->cap_yuv.addr_vir.addr_u;
	CMR_LOGD("jpeg_tmp, phy 0x%lx, vir 0x%lx, size 0x%x",
		cap_mem->jpeg_tmp.addr_phy.addr_y,
		cap_mem->jpeg_tmp.addr_vir.addr_y,
		cap_mem->jpeg_tmp.buf_size);

	/* update io param */
	*io_mem_res = mem_res;
	*io_mem_end = mem_end;

	CMR_LOGD("mem_res, mem_end 0x%x, 0x%x",
		mem_res,
		mem_end);

	return 0;
}

int arrange_rot_buf(struct cmr_cap_2_frm *cap_2_frm,
				struct img_size *sn_size,
				struct img_rect *sn_trim,
				struct img_size *image_size,
				uint32_t orig_fmt,
				struct img_size *cap_size,
				struct img_size *thum_size,
				struct cmr_cap_mem *capture_mem,
				uint32_t need_rot,
				uint32_t need_scale,
				uint32_t *io_mem_res,
				uint32_t *io_mem_end,
				uint32_t *io_channel_size)
{
	uint32_t       channel_size, maxsize;
	uint32_t tmpsize = 0, tmpchannelsize = 0;
	uint32_t       size_pixel;
	uint32_t       mem_res = 0, mem_end = 0;
	uint32_t       offset = 0, offset_1;
	struct cmr_cap_mem *cap_mem = capture_mem;/*&capture_mem[0];*/
	struct img_size align16_image_size, align16_cap_size;

	align16_image_size.width = camera_get_aligned_size(cap_2_frm->type, image_size->width);
	align16_image_size.height = camera_get_aligned_size(cap_2_frm->type, image_size->height);
	align16_cap_size.width = camera_get_aligned_size(cap_2_frm->type, cap_size->width);
	align16_cap_size.height = camera_get_aligned_size(cap_2_frm->type, cap_size->height);

	if (NULL == io_mem_res ||
		NULL == io_mem_end ||
		NULL == io_channel_size) {
		return -1;
	}

	if (!need_rot) {
		return -1;
	}
	CMR_LOGI("rotation buf arrange");

	mem_res = *io_mem_res;
	mem_end = *io_mem_end;
	channel_size = *io_channel_size;

	if (IMG_DATA_TYPE_JPEG == orig_fmt) {
		size_pixel = channel_size << 1;
	} else {
		size_pixel = (uint32_t)((channel_size*3) >> 1);
	}

	CMR_LOGI("Rot channel size 0x%x, buf size 0x%X", channel_size, size_pixel);

#ifdef CONFIG_MEM_OPTIMIZATION
	if (ZOOM_POST_PROCESS == cap_2_frm->zoom_post_proc) {
		tmpchannelsize = (uint32_t)CMR_ADDR_ALIGNED((align16_image_size.width * align16_image_size.height));
		tmpsize = (uint32_t)CMR_ADDR_ALIGNED((cap_size->width * cap_size->height));

		cap_mem->cap_yuv_rot.addr_phy.addr_y = cap_mem->target_yuv.addr_phy.addr_y;
		cap_mem->cap_yuv_rot.addr_vir.addr_y = cap_mem->target_yuv.addr_vir.addr_y;
		cap_mem->cap_yuv_rot.addr_phy.addr_u = cap_mem->cap_yuv_rot.addr_phy.addr_y + tmpsize;
		cap_mem->cap_yuv_rot.addr_vir.addr_u = cap_mem->cap_yuv_rot.addr_vir.addr_y + tmpsize;
		cap_mem->cap_yuv_rot.addr_phy.addr_v = 0;
		cap_mem->cap_yuv_rot.addr_vir.addr_v = 0;

		cap_mem->cap_yuv_rot.size.width = align16_cap_size.height;
		cap_mem->cap_yuv_rot.size.height = align16_cap_size.width;
		cap_mem->cap_yuv_rot.buf_size = tmpsize * 3 / 2;
		cap_mem->cap_yuv_rot.fmt = IMG_DATA_TYPE_YUV420;

		cap_mem->target_yuv.addr_phy.addr_y = cap_mem->cap_yuv.addr_phy.addr_y;
		cap_mem->target_yuv.addr_vir.addr_y = cap_mem->cap_yuv.addr_vir.addr_y;
		cap_mem->target_yuv.addr_phy.addr_u = cap_mem->target_yuv.addr_phy.addr_y + tmpchannelsize;
		cap_mem->target_yuv.addr_vir.addr_u = cap_mem->target_yuv.addr_vir.addr_y + tmpchannelsize;
		cap_mem->target_yuv.addr_phy.addr_v = 0;

		/* mem reuse when rot */
		cap_mem->target_jpeg.addr_phy.addr_y = cap_mem->cap_yuv_rot.addr_phy.addr_y;
		cap_mem->target_jpeg.addr_vir.addr_y = cap_mem->cap_yuv_rot.addr_vir.addr_y;

		CMR_LOGD("cap_yuv_rot, phy 0x%lx 0x%lx, vir 0x%lx 0x%lx, size 0x%x",
			cap_mem->cap_yuv_rot.addr_phy.addr_y,
			cap_mem->cap_yuv_rot.addr_phy.addr_u,
			cap_mem->cap_yuv_rot.addr_vir.addr_y,
			cap_mem->cap_yuv_rot.addr_vir.addr_u,
			cap_mem->cap_yuv_rot.buf_size);
		//goto rot_buf_bypass;
	} else {
		tmpchannelsize = (uint32_t)CMR_ADDR_ALIGNED((align16_image_size.width * align16_image_size.height));
		tmpsize = (uint32_t)CMR_ADDR_ALIGNED((cap_size->width * cap_size->height));

		maxsize = (tmpchannelsize > tmpsize) ?  tmpchannelsize : tmpsize;
		if (need_scale) {
			cap_mem->cap_yuv_rot.addr_phy.addr_y = cap_mem->target_yuv.addr_phy.addr_y;
			cap_mem->cap_yuv_rot.addr_vir.addr_y = cap_mem->target_yuv.addr_vir.addr_y;
			cap_mem->cap_yuv_rot.addr_phy.addr_u = cap_mem->cap_yuv_rot.addr_phy.addr_y + tmpsize;
			cap_mem->cap_yuv_rot.addr_vir.addr_u = cap_mem->cap_yuv_rot.addr_vir.addr_y + tmpsize;
			cap_mem->cap_yuv_rot.addr_phy.addr_v = 0;
			cap_mem->cap_yuv_rot.addr_vir.addr_v = 0;

			cap_mem->cap_yuv_rot.size.width = align16_cap_size.height;
			cap_mem->cap_yuv_rot.size.height = align16_cap_size.width;
			cap_mem->cap_yuv_rot.buf_size = maxsize * 3 / 2;
			cap_mem->cap_yuv_rot.fmt = IMG_DATA_TYPE_YUV420;
		} else {
			cap_mem->cap_yuv_rot.addr_phy.addr_y = cap_mem->cap_yuv.addr_phy.addr_y;
			cap_mem->cap_yuv_rot.addr_vir.addr_y = cap_mem->cap_yuv.addr_vir.addr_y;
			cap_mem->cap_yuv_rot.addr_phy.addr_u = cap_mem->cap_yuv_rot.addr_phy.addr_y + tmpsize;
			cap_mem->cap_yuv_rot.addr_vir.addr_u = cap_mem->cap_yuv_rot.addr_vir.addr_y + tmpsize;
			cap_mem->cap_yuv_rot.addr_phy.addr_v = 0;
			cap_mem->cap_yuv_rot.addr_vir.addr_v = 0;

			cap_mem->cap_yuv_rot.size.width = align16_cap_size.height;
			cap_mem->cap_yuv_rot.size.height = align16_cap_size.width;
			cap_mem->cap_yuv_rot.buf_size = maxsize * 3 / 2;
			cap_mem->cap_yuv_rot.fmt = IMG_DATA_TYPE_YUV420;
		}

		/* mem reuse when rot */
		cap_mem->target_jpeg.addr_phy.addr_y = cap_mem->cap_yuv.addr_phy.addr_y;
		cap_mem->target_jpeg.addr_vir.addr_y = cap_mem->cap_yuv.addr_vir.addr_y;

		CMR_LOGD("cap_yuv_rot, phy 0x%lx 0x%lx, vir 0x%lx 0x%lx, size 0x%x",
			cap_mem->cap_yuv_rot.addr_phy.addr_y,
			cap_mem->cap_yuv_rot.addr_phy.addr_u,
			cap_mem->cap_yuv_rot.addr_vir.addr_y,
			cap_mem->cap_yuv_rot.addr_vir.addr_u,
			cap_mem->cap_yuv_rot.buf_size);
		//goto rot_buf_bypass;
	}
#else

	if (mem_res > size_pixel) {
		CMR_LOGI("Rot buffer located at frame mem");
		offset = cap_2_frm->mem_frm.addr_phy.addr_y + mem_end;
		offset_1 = cap_2_frm->mem_frm.addr_vir.addr_y + mem_end;
		mem_res -= size_pixel;
		mem_end += size_pixel;

		cap_mem->cap_yuv_rot.addr_phy.addr_y = offset;
		cap_mem->cap_yuv_rot.addr_vir.addr_y = offset_1;
		cap_mem->cap_yuv_rot.addr_phy.addr_u = offset + channel_size;
		cap_mem->cap_yuv_rot.addr_vir.addr_u = offset_1 + channel_size;

	} else {
		unsigned long addr_phy, addr_vir;
		CMR_LOGI("No more memory reseved in buffer, Rot buffer need alloc");

		if (NULL != cap_2_frm->alloc_mem) {
			if (cap_2_frm->alloc_mem(cap_2_frm->handle,
				channel_size,
				&addr_phy,
				&addr_vir) != 0) {
				CMR_LOGE("Failed to alloc the buffer used in capture");
				return -1;
			}
			cap_mem->cap_yuv_rot.addr_phy.addr_y = addr_phy;
			cap_mem->cap_yuv_rot.addr_vir.addr_y = addr_vir;
			if (cap_2_frm->alloc_mem(cap_2_frm->handle,
				channel_size,
				&addr_phy,
				&addr_vir) != 0) {
				CMR_LOGE("Failed to alloc the buffer used in capture");
				return -1;
			}
			cap_mem->cap_yuv_rot.addr_phy.addr_u = addr_phy;
			cap_mem->cap_yuv_rot.addr_vir.addr_u = addr_vir;
		} else {
			CMR_LOGE("cap_2_frm->alloc_mem is NULL");
			return -1;
		}

	}
	cap_mem->cap_yuv_rot.addr_phy.addr_v = 0;
	cap_mem->cap_yuv_rot.size.width = align16_image_size.height;
	cap_mem->cap_yuv_rot.size.height = align16_image_size.width;
	cap_mem->cap_yuv_rot.buf_size = size_pixel;
	cap_mem->cap_yuv_rot.fmt = IMG_DATA_TYPE_YUV420;

	/* mem reuse when rot */
	cap_mem->target_jpeg.addr_phy.addr_y = cap_mem->cap_yuv_rot.addr_phy.addr_y;
	cap_mem->target_jpeg.addr_vir.addr_y = cap_mem->cap_yuv_rot.addr_vir.addr_y;

	CMR_LOGD("cap_yuv_rot, phy 0x%lx 0x%lx, vir 0x%lx 0x%lx, size 0x%x",
		cap_mem->cap_yuv_rot.addr_phy.addr_y,
		cap_mem->cap_yuv_rot.addr_phy.addr_u,
		cap_mem->cap_yuv_rot.addr_vir.addr_y,
		cap_mem->cap_yuv_rot.addr_vir.addr_u,
		cap_mem->cap_yuv_rot.buf_size);

	/* update io param */
	*io_mem_res = mem_res;
	*io_mem_end = mem_end;
	*io_channel_size = channel_size;

	cap_mem->jpeg_tmp.addr_phy.addr_y = cap_mem->cap_yuv_rot.addr_phy.addr_u;
	cap_mem->jpeg_tmp.addr_vir.addr_y = cap_mem->cap_yuv_rot.addr_vir.addr_u;
#endif

//rot_buf_bypass:
	CMR_LOGD("mem_res=%d, mem_end=%d", mem_res, mem_end);
	return 0;
}


uint32_t get_jpeg_size(uint32_t width, uint32_t height, uint32_t thum_width, uint32_t thum_height)
{
	uint32_t       size;
	(void)thum_width; (void)thum_height;

	if ((width * height) <= JPEG_SMALL_SIZE) {
		size = CMR_JPEG_SZIE(width, height) + JPEG_EXIF_SIZE;
	} else {
		size = CMR_JPEG_SZIE(width, height);
	}

	return ADDR_BY_WORD(size);
}

uint32_t get_thum_yuv_size(uint32_t width, uint32_t height, uint32_t thum_width, uint32_t thum_height)
{
	(void)width; (void)height;
	return (uint32_t)(CMR_ADDR_ALIGNED((thum_width * thum_height)) * 3 / 2);
}
uint32_t get_thum_jpeg_size(uint32_t width, uint32_t height, uint32_t thum_width, uint32_t thum_height)
{
	uint32_t       size;

	(void)width; (void)height;
	size = CMR_JPEG_SZIE(thum_width, thum_height);
	return ADDR_BY_WORD(size);
}

uint32_t get_jpg_tmp_size(uint32_t width, uint32_t height, uint32_t thum_width, uint32_t thum_height)
{
	(void)thum_width; (void)thum_height;
	return (uint32_t)(width * CMR_SLICE_HEIGHT * 2); // TBD
}
uint32_t get_scaler_tmp_size(uint32_t width, uint32_t height, uint32_t thum_width, uint32_t thum_height)
{
	UNUSED(width);
	UNUSED(height);

	uint32_t   slice_buffer;

	(void)thum_width; (void)thum_height;

	slice_buffer = 0;/*(uint32_t)(width * CMR_SLICE_HEIGHT * CMR_ZOOM_FACTOR);only UV422 need dwon sample to UV420*/

	return slice_buffer;
}

uint32_t get_isp_tmp_size(uint32_t width, uint32_t height, uint32_t thum_width, uint32_t thum_height)
{
	uint32_t   slice_buffer;

	(void)thum_width; (void)thum_height;
	UNUSED(width);
	UNUSED(height);

	slice_buffer = (uint32_t)(width * CMR_SLICE_HEIGHT);/*only UV422 need dwon sample to UV420*/

	return slice_buffer;
}

uint32_t camera_get_aligned_size(uint32_t type, uint32_t size)
{
	uint32_t    size_aligned = 0;

	CMR_LOGI("type %d",  type);
	if (CAMERA_MEM_NO_ALIGNED == type) {
		size_aligned = size;
	} else {
		size_aligned = CAMERA_ALIGNED_16(size);
	}

	return size_aligned;
}
