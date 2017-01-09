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

#include <stdlib.h>
#include "cmr_common.h"

#define CAMERA_ZOOM_LEVEL_MAX                 8
#define ZOOM_STEP(x)                          (((x) - (x) / CMR_ZOOM_FACTOR) / CAMERA_ZOOM_LEVEL_MAX)

struct CAMERA_TAKEPIC_STAT cap_stp[CMR_STEP_MAX] ={
		{"takepicture",        0, 0, 0},
		{"capture start",      0, 0, 0},
		{"capture end",        0, 0, 0},
		{"rotate start",       0, 0, 0},
		{"rotate end",         0, 0, 0},
		{"isp pp start",       0, 0, 0},
		{"isp pp end",         0, 0, 0},
		{"jpeg dec start",     0, 0, 0},
		{"jpeg dec end",       0, 0, 0},
		{"scaling start",      0, 0, 0},
		{"scaling end",        0, 0, 0},
		{"uv denoise start",   0, 0, 0},
		{"uv denoise end",     0, 0, 0},
		{"y denoise start",    0, 0, 0},
		{"y denoise end",      0, 0, 0},
		{"snr uv denoise start",    0, 0, 0},
		{"snr uv denoise end",      0, 0, 0},
		{"jpeg enc start",     0, 0, 0},
		{"jpeg enc end",       0, 0, 0},
		{"cvt thumb start",    0, 0, 0},
		{"cvt thumb end",      0, 0, 0},
		{"thumb enc start",    0, 0, 0},
		{"thumb enc end",      0, 0, 0},
		{"write exif start",   0, 0, 0},
		{"write exif end",     0, 0, 0},
		{"call back",          0, 0, 0},
};

cmr_int camera_get_trim_rect(struct img_rect *src_trim_rect, cmr_uint zoom_level, struct img_size *dst_size)
{
	cmr_int                  ret = CMR_CAMERA_SUCCESS;
	cmr_uint                 trim_width = 0, trim_height = 0;
	cmr_uint                 zoom_step_w = 0, zoom_step_h = 0;

	if (!src_trim_rect || !dst_size) {
		CMR_LOGE("0x%lx 0x%lx", (cmr_uint)src_trim_rect, (cmr_uint)dst_size);
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	trim_width = src_trim_rect->width;
	trim_height = src_trim_rect->height;

	if (0 == dst_size->width || 0 == dst_size->height) {
		CMR_LOGE("0x%x 0x%x", dst_size->width, dst_size->height);
		ret = -CMR_CAMERA_INVALID_PARAM;
		goto exit;
	}

	if (dst_size->width * src_trim_rect->height < dst_size->height * src_trim_rect->width) {
		trim_width = dst_size->width * src_trim_rect->height / dst_size->height;
	} else {
		trim_height = dst_size->height * src_trim_rect->width / dst_size->width;
	}

	zoom_step_w = ZOOM_STEP(trim_width);
	zoom_step_w &= ~1;
	zoom_step_w *= zoom_level;
	zoom_step_h = ZOOM_STEP(trim_height);
	zoom_step_h &= ~1;
	zoom_step_h *= zoom_level;
	trim_width = trim_width - zoom_step_w;
	trim_height = trim_height - zoom_step_h;

	src_trim_rect->start_x += (src_trim_rect->width - trim_width) >> 1;
	src_trim_rect->start_y += (src_trim_rect->height - trim_height) >> 1;
	src_trim_rect->start_x = CAMERA_WIDTH(src_trim_rect->start_x);
	src_trim_rect->start_y = CAMERA_HEIGHT(src_trim_rect->start_y);
	src_trim_rect->width = CAMERA_WIDTH(trim_width);
	src_trim_rect->height = CAMERA_HEIGHT(trim_height);
	CMR_LOGI("zoom_level %ld trim rect %d %d %d %d", zoom_level, src_trim_rect->start_x,
		      src_trim_rect->start_y, src_trim_rect->width, src_trim_rect->height);
exit:
	return ret;
}

cmr_int camera_get_trim_rect2(struct img_rect *src_trim_rect, float zoom_ratio, float dst_ratio,
											cmr_u32 sensor_w, cmr_u32 sensor_h, cmr_u8 rot)//for hal2.0 calculate crop again
{
	cmr_u32                  trim_width;
	cmr_u32                  trim_height;
	float                    minOutputRatio;
	float                    zoom_width, zoom_height, sensor_ratio;

	if (NULL == src_trim_rect) {
		CMR_LOGE("param error");
		return -CMR_CAMERA_INVALID_PARAM;
	} else if (src_trim_rect->width == 0 || src_trim_rect->height == 0) {
		CMR_LOGE("0x%lx w %d h %d", (cmr_uint)src_trim_rect, src_trim_rect->width, src_trim_rect->height);
		return -CMR_CAMERA_INVALID_PARAM;
	}
	minOutputRatio = dst_ratio;
	sensor_ratio = (float)sensor_w / (float)sensor_h;
	if (rot != IMG_ANGLE_0 && rot != IMG_ANGLE_180) {
		minOutputRatio = 1 / minOutputRatio;
	}
	if (minOutputRatio > sensor_ratio) {
		zoom_width = (float)sensor_w / zoom_ratio;
		zoom_height = zoom_width / minOutputRatio;
	} else {
		zoom_height = (float)sensor_h / zoom_ratio;
		zoom_width = zoom_height * minOutputRatio;
	}
	trim_width = (cmr_u32)zoom_width;
	trim_height = (cmr_u32)zoom_height;

	CMR_LOGI("sensor_ratio %f, minOutputRatio %f, zoom_ratio %f", sensor_ratio, minOutputRatio, zoom_ratio);
	CMR_LOGI("trim_width %d, trim_height %d", trim_width, trim_height);
	src_trim_rect->start_x += (src_trim_rect->width - CAMERA_START(trim_width)) >> 1;
	src_trim_rect->start_y += (src_trim_rect->height - CAMERA_START(trim_height)) >> 1;
	src_trim_rect->start_x = CAMERA_START(src_trim_rect->start_x);
	src_trim_rect->start_y = CAMERA_START(src_trim_rect->start_y);
	src_trim_rect->width = CAMERA_START(trim_width);
	src_trim_rect->height = CAMERA_START(trim_height);

	CMR_LOGI("zoom_level %f trim rect %d %d %d %d",
			zoom_ratio,
			src_trim_rect->start_x,
			src_trim_rect->start_y,
			src_trim_rect->width,
			src_trim_rect->height);

	return CMR_CAMERA_SUCCESS;
}

cmr_int camera_save_to_file(cmr_u32 index, cmr_u32 img_fmt, cmr_u32 width, cmr_u32 height, struct img_addr *addr)
{
	cmr_int                      ret = CMR_CAMERA_SUCCESS;
	char                         file_name[40];
	char                         tmp_str[10];
	FILE                         *fp = NULL;

	CMR_LOGI("index %d format %d width %d heght %d", index, img_fmt, width, height);

	cmr_bzero(file_name, 40);
	strcpy(file_name, "/data/misc/media/");
	sprintf(tmp_str, "%d", width);
	strcat(file_name, tmp_str);
	strcat(file_name, "X");
	sprintf(tmp_str, "%d", height);
	strcat(file_name, tmp_str);

	if (IMG_DATA_TYPE_YUV420 == img_fmt ||
		IMG_DATA_TYPE_YUV422 == img_fmt) {
		strcat(file_name, "_y_");
		sprintf(tmp_str, "%d", index);
		strcat(file_name, tmp_str);
		strcat(file_name, ".raw");
		CMR_LOGI("file name %s", file_name);
		fp = fopen(file_name, "wb");

		if (NULL == fp) {
			CMR_LOGI("can not open file: %s \n", file_name);
			return 0;
		}

		fwrite((void*)addr->addr_y, 1, width * height, fp);
		fclose(fp);

		bzero(file_name, 40);
		strcpy(file_name, "/data/misc/media/");
		sprintf(tmp_str, "%d", width);
		strcat(file_name, tmp_str);
		strcat(file_name, "X");
		sprintf(tmp_str, "%d", height);
		strcat(file_name, tmp_str);
		strcat(file_name, "_uv_");
		sprintf(tmp_str, "%d", index);
		strcat(file_name, tmp_str);
		strcat(file_name, ".raw");
		CMR_LOGI("file name %s", file_name);
		fp = fopen(file_name, "wb");
		if (NULL == fp) {
			CMR_LOGI("can not open file: %s \n", file_name);
			return 0;
		}

		if (IMG_DATA_TYPE_YUV420 == img_fmt) {
			fwrite((void*)addr->addr_u, 1, width * height / 2, fp);
		} else {
			fwrite((void*)addr->addr_u, 1, width * height, fp);
		}
		fclose(fp);
	} else if (IMG_DATA_TYPE_JPEG == img_fmt) {
		strcat(file_name, "_");
		sprintf(tmp_str, "%d", index);
		strcat(file_name, tmp_str);
		strcat(file_name, ".jpg");
		CMR_LOGI("file name %s", file_name);

		fp = fopen(file_name, "wb");
		if (NULL == fp) {
			CMR_LOGI("can not open file: %s \n", file_name);
			return 0;
		}

		fwrite((void*)addr->addr_y, 1, width * height*2, fp);
		fclose(fp);
	} else if (IMG_DATA_TYPE_RAW == img_fmt) {
		strcat(file_name, "_");
		sprintf(tmp_str, "%d", index);
		strcat(file_name, tmp_str);
		strcat(file_name, ".mipi_raw");
		CMR_LOGI("file name %s", file_name);

		fp = fopen(file_name, "wb");
		if(NULL == fp){
			CMR_LOGI("can not open file: %s \n", file_name);
			return 0;
		}

		fwrite((void*)addr->addr_y, 1, (uint32_t)(width * height * 5 / 4), fp);
		fclose(fp);
	}
	return 0;
}

void camera_snapshot_step_statisic(struct img_size *image_size)
{
	cmr_int i = 0, time_delta = 0;

	if (NULL == image_size) {
		ALOGE("image_size is null,para invalid");
		return;
	}
	ALOGE("*********************Take picture statistic*******Start****%4d*%4d*****",
		  image_size->width,
		  image_size->height);

	for (i = 0; i < CMR_STEP_MAX; i++) {
		if (i == 0) {
			ALOGE("%20s, %10d",
				cap_stp[i].step_name,
				0);
			continue;
		}

		if (1 == cap_stp[i].valid) {
			time_delta = (int)((cap_stp[i].timestamp - cap_stp[CMR_STEP_TAKE_PIC].timestamp)/1000000);
			ALOGE("%20s, %10ld",
				cap_stp[i].step_name,
				time_delta);
		}
	}
	ALOGE("*********************Take picture statistic********End*******************");
}

void camera_take_snapshot_step(enum CAMERA_TAKEPIC_STEP step)
{
	if (step > CMR_STEP_CALL_BACK) {
		CMR_LOGE("error %d", step);
		return;
	}
	cap_stp[step].timestamp = systemTime(CLOCK_MONOTONIC);
	cap_stp[step].valid = 1;
}
