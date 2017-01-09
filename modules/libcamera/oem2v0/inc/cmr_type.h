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
#ifndef _CMR_TYPE_H_
#define _CMR_TYPE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <sys/types.h>
//#ifdef MINICAMERA
#include "port.h"
//#endif
#include "mtrace.h"

typedef unsigned long   cmr_uint;
typedef long            cmr_int;
typedef uint64_t        cmr_u64;
typedef int64_t         cmr_s64;
typedef unsigned int    cmr_u32;
typedef int             cmr_s32;
typedef unsigned short  cmr_u16;
typedef short           cmr_s16;
typedef unsigned char   cmr_u8;
typedef char            cmr_s8;
typedef void*           cmr_handle;


/********************************* error type *********************************/
#define CMR_CAMERA_SUCCESS                  0
#define CMR_CAMERA_FAIL                     1
#define CMR_CAMERA_INVALID_PARAM            2
#define CMR_CAMERA_NO_MEM                   3
#define CMR_CAMERA_NO_SENSOR                4
#define CMR_CAMERA_NO_SUPPORT               5
#define CMR_CAMERA_INVALID_STATE            6
#define CMR_CAMERA_INVALID_FORMAT           7
#define CMR_CAMERA_JPEG_SPECIFY_FAILED      8
#define CMR_CAMERA_NORNAL_EXIT              9
#define CMR_CAMERA_INVALID_FRAME            10
#define CMR_CAMERA_FD_INIT                  -1

/******************************************************************************/

#define FOCUS_ZONE_CNT_MAX                  6
#define WIN_AREA_COUNT_MAX                  1



enum zoom_param_mode {
	ZOOM_LEVEL = 0,
	ZOOM_INFO,
	ZOOM_MODE_MAX
};

struct img_rect {
	cmr_u32                                 start_x;
	cmr_u32                                 start_y;
	cmr_u32                                 width;
	cmr_u32                                 height;
};

struct img_size {
	cmr_u32                                 width;
	cmr_u32                                 height;
};

struct zoom_info {
	float                                   zoom_ratio;
	float                                   output_ratio;
};

struct cmr_zoom_param {
	cmr_uint                                mode;
	union {
		cmr_uint                            zoom_level;
		struct zoom_info                    zoom_info;
	};
};

struct cmr_win_area {
	cmr_uint            count;
	struct img_rect	    rect[WIN_AREA_COUNT_MAX];
};

struct cmr_ae_param {
	cmr_int                                 mode;
	struct cmr_win_area                     win_area;
};

struct cmr_preview_fps_param {
	cmr_int                                 is_recording;
	cmr_uint                                frame_rate;
	cmr_uint                                video_mode;
};

struct cmr_range_fps_param {
	cmr_int                                 is_recording;
	cmr_uint                                video_mode;
	cmr_uint                                min_fps;
	cmr_uint                                max_fps;
	cmr_uint                                *old_min_fps;
	cmr_uint                                *old_max_fps;
};

struct img_data_end {
	cmr_u8                                  y_endian;
	cmr_u8                                  uv_endian;
	cmr_u8                                  reserved0;
	cmr_u8                                  reserved1;
	//cmr_u32                                 padding;
};

struct frm_info {
	cmr_u32                             channel_id;
	cmr_u32                             frame_id;
	cmr_u32                             frame_real_id;
	cmr_u32                             height;
	cmr_uint                            sec;
	cmr_uint                            usec;
	cmr_u32                             length;
	cmr_u32                             free;
	cmr_u32                             base;
	cmr_u32                             fmt;
	cmr_u32                             yaddr;
	cmr_u32                             uaddr;
	cmr_u32                             vaddr;
	cmr_u32                             yaddr_vir;
	cmr_u32                             uaddr_vir;
	cmr_u32                             vaddr_vir;
	cmr_uint                            zsl_private;
};

#ifdef __cplusplus
}
#endif

#endif //for _CMR_COMMON_H_

