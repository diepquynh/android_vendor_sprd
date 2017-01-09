/*
 * Copyright (C) 2008 The Android Open Source Project
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
#ifndef _DCAM_HAL_H_
#define _DCAM_HAL_H_

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef USE_GPU_PROCESS_VIDEO
#define SRCRECT_X_ALLIGNED                   0
#define SRCRECT_Y_ALLIGNED                   0
#define SRCRECT_WIDTH_ALLIGNED               0
#define SRCRECT_HEIGHT_ALLIGNED              0
#else
#define SRCRECT_X_ALLIGNED                   3
#define SRCRECT_Y_ALLIGNED                   3
#define SRCRECT_WIDTH_ALLIGNED               3
#define SRCRECT_HEIGHT_ALLIGNED              3
#endif
#define FB_X_ALLIGNED                        3
#define FB_Y_ALLIGNED                        3
#define FB_WIDTH_ALLIGNED                    3
#define FB_HEIGHT_ALLIGNED                   3

typedef enum {
	HW_ROTATION_0 = 0,
	HW_ROTATION_90,
	HW_ROTATION_180,
	HW_ROTATION_270,
	HW_ROTATION_MIRROR,
	HW_ROTATION_ANGLE_MAX
}HW_ROTATION_MODE_E;

typedef enum {
	HW_ROTATION_DATA_YUV422 = 0,
	HW_ROTATION_DATA_YUV420,
	HW_ROTATION_DATA_YUV400,
	HW_ROTATION_DATA_RGB888,
	HW_ROTATION_DATA_RGB666,
	HW_ROTATION_DATA_RGB565,
	HW_ROTATION_DATA_RGB555,
	HW_ROTATION_DATA_FMT_MAX
} HW_ROTATION_DATA_FORMAT_E;

typedef enum  {
	HW_SCALE_DATA_YUV422 = 0,
	HW_SCALE_DATA_YUV420,
	HW_SCALE_DATA_YUV400,
	HW_SCALE_DATA_YUV420_3FRAME,
	HW_SCALE_DATA_RGB565,
	HW_SCALE_DATA_RGB888,
	HW_SCALE_DATA_FMT_MAX
}HW_SCALE_DATA_FORMAT_E;

struct sprd_rect {
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
};

int camera_rotation_copy_data(uint32_t width, uint32_t height, uint32_t in_addr, uint32_t out_addr);

int camera_rotation_copy_data_from_virtual(uint32_t width, uint32_t height, uint32_t in_virtual_addr, uint32_t out_addr);

int camera_rotation(HW_ROTATION_DATA_FORMAT_E rot_format, int degree, uint32_t width, uint32_t height, uint32_t in_addr, uint32_t out_addr);

#ifdef __cplusplus
}
#endif

#endif
