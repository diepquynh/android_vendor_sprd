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
#ifndef _ISP_TYPE_H_
#define _ISP_TYPE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef WIN32
#include <sys/types.h>
#endif


/**---------------------------------------------------------------------------*
**				Micro Define					*
**----------------------------------------------------------------------------*/
#ifdef WIN32
typedef unsigned __int64 uint64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef signed __int64	int64_t;
typedef signed int int32_t;
typedef signed short int16_t;
typedef signed char int8_t;

#endif
typedef unsigned long   isp_uint;
typedef long            isp_int;
typedef uint64_t        isp_u64;
typedef int64_t         isp_s64;
typedef unsigned int    isp_u32;
typedef int             isp_s32;
typedef unsigned short  isp_u16;
typedef short           isp_s16;
typedef unsigned char   isp_u8;
typedef char            isp_s8;
typedef void*           isp_handle;

#ifndef PNULL
#define PNULL ((void*)0)
#endif

#define ISP_FIX_10BIT_UNIT 1024
#define ISP_FIX_8BIT_UNIT 256
#define ISP_EB 0x01
#define ISP_UEB 0x00
#define ISP_ZERO 0x00
#define ISP_ONE 0x01
#define ISP_ALPHA_ONE	1024
#define ISP_ALPHA_ZERO	0
#define ISP_TWO 0x02
#define ISP_TRAC(_x_) ISP_LOGE _x_
#define ISP_RETURN_IF_FAIL(exp,warning) do{if(exp) {ISP_TRAC(warning); return exp;}}while(0)
#define ISP_TRACE_IF_FAIL(exp,warning) do{if(exp) {ISP_TRAC(warning);}}while(0)

typedef enum
{
	ISP_SUCCESS=0x00,
	ISP_PARAM_NULL,
	ISP_PARAM_ERROR,
	ISP_CALLBACK_NULL,
	ISP_ALLOC_ERROR,
	ISP_NO_READY,
	ISP_ERROR,
	ISP_RETURN_MAX=0xffffffff
}ISP_RETURN_VALUE_E;

#if __WORDSIZE == 64
typedef long int intptr_t;
#else
typedef int  intptr_t;
#endif


struct isp_sample_info {
	isp_s32 x0;
	isp_s32 x1;
	isp_u32 alpha;//x1: alpha---1024->1x
	isp_u32 dec_ratio;
};

struct isp_sample_point_info {
	isp_s32 x0;
	isp_s32 x1;
	isp_u32 weight0;
	isp_u32 weight1;
};

struct isp_data_info {
	isp_u32 size;
	void *data_ptr;
	void *param_ptr;
};
struct isp_data_bin_info {
	isp_u32 size;
	uint32_t offset;
};

struct isp_pos{
	isp_s32 x;
	isp_s32 y;
};
struct isp_point {
	isp_s16 x;
	isp_s16 y;
};
struct isp_size{
	isp_u32 w;
	isp_u32 h;
};

struct isp_slice_window {
	isp_u32 start_col;
	isp_u32 start_row;
	isp_u32 end_col;
	isp_u32 end_row;
};

struct isp_buffer_size_info {
	isp_u32 pitch;
	isp_u32 count_lines;
};
struct isp_trim_size{
	isp_s32 x;
	isp_s32 y;
	isp_u32 w;
	isp_u32 h;
};
struct isp_rect {
	isp_s32 st_x;
	isp_s32 st_y;
	isp_u32 width;
	isp_u32 height;
};
struct isp_pos_rect{
	isp_s32 start_x;
	isp_s32 start_y;
	isp_u32 end_x;
	isp_u32 end_y;
};

struct isp_rgb_gains {
	isp_u32 gain_r;
	isp_u32 gain_g;
	isp_u32 gain_b;
	isp_u32 reserved;
};

struct isp_rgb_offset {
	isp_s32 offset_r;
	isp_s32 offset_g;
	isp_s32 offset_b;
	isp_s32 reserved;
};

struct isp_range_l {
	isp_s32 min;
	isp_s32 max;
};

struct isp_range {
	isp_s16 min;
	isp_s16 max;
};

struct isp_weight_value {
	isp_s16 value[2];
	isp_u16 weight[2];
};

struct isp_yuv {
	uint16_t y;
	uint16_t u;
	uint16_t v;
	uint16_t reserved;
};
struct isp_curve_coeff{
	uint32_t p1;
	uint32_t p2;
};
struct isp_bin_param {
	uint16_t hx;
	uint16_t vx;
};

struct isp_sample {
	int16_t		x;
	int16_t		y;
};

#define ISP_PIECEWISE_SAMPLE_NUM 0x10

struct isp_piecewise_func {
	uint32_t num;
	struct isp_sample samples[ISP_PIECEWISE_SAMPLE_NUM];
};


#ifdef __cplusplus
}
#endif

#endif //for _ISP_COMMON_H_

