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
#ifndef _ISP_AWB_TYPES_
#define _ISP_AWB_TYPES_
/*------------------------------------------------------------------------------*
*				Dependencies					*
*-------------------------------------------------------------------------------*/
#ifdef WIN32
#include <stdlib.h>
#include <stdio.h>
#include "sci_types.h"
#else
#include <linux/types.h>
#include <sys/types.h>
#include <utils/Log.h>
#endif

/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif
/*------------------------------------------------------------------------------*
*				Micro Define					*
*-------------------------------------------------------------------------------*/
#define AWB_SUCCESS				0
#define AWB_E_NO_WHITE_BLOCK 			1
#define AWB_ERROR				255

#define AWB_MAX_INSTANCE_NUM			4
#define AWB_MAX_PIECEWISE_NUM			16

#define ISP_AWB_LOG
//#define ISP_LOG

#define AWB_TRUE	1
#define AWB_FALSE	0
/*------------------------------------------------------------------------------*
*				Data Structures					*
*-------------------------------------------------------------------------------*/
typedef void* awb_handle_t;



enum awb_cmd {
	AWB_CMD_SET_BASE 			= 0x1000,
	AWB_CMD_SET_INIT_PARAM			= (AWB_CMD_SET_BASE + 1),
	AWB_CMD_GET_BASE			= 0x2000,
	AWB_CMD_GET_INIT_PARAM			= (AWB_CMD_GET_BASE + 1),
	AWB_CMD_GET_CALC_DETAIL			= (AWB_CMD_GET_BASE + 2),

	AWB_CMD_UNKNOWN				= 0xffffffff
};

struct awb_size {
	uint16_t	w;
	uint16_t	h;
};

struct awb_range {
	uint16_t	min;
	uint16_t  	max;
};

struct awb_point {
	uint16_t	x;
	uint16_t	y;
};

struct awb_rect {
	struct awb_point begin;
	struct awb_point end;
};

struct awb_gain {
	uint16_t r;
	uint16_t g;
	uint16_t b;
};

struct awb_linear_func {
	int32_t a;
	int32_t b;
	uint32_t shift;
};

struct awb_sample {
	/* input value of the sample point */
	int32_t	x;
	/* output value of the sample point */
	int32_t	y;
};

/* piecewise linear function */
struct awb_piecewise_func {
	/* sample points of the piecewise linear function, the x value
	 of the samples must be in an ascending sort  order */
	struct awb_sample samples[AWB_MAX_PIECEWISE_NUM];
	/* number of the samples*/
	uint16_t num;
	uint16_t base;
};

/* scanline */
struct awb_scanline {
	uint16_t y;
	uint16_t x0;
	uint16_t x1;
};

/* scanline of the window */
struct awb_win_scanline {
	/* scanline number */
	uint16_t num;
	struct awb_scanline scanline;
};
/*------------------------------------------------------------------------------*
*				Data Prototype					*
*-------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------*
*				Functions						*
*-------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/
#endif
// End
