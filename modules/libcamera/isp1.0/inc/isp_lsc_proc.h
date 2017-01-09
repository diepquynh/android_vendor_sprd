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
#ifndef _ISP_LSC_PROC_
#define _ISP_LSC_PROC_
/*------------------------------------------------------------------------------*
*				Dependencies					*
*-------------------------------------------------------------------------------*/
#ifdef WIN32
#include "sci_types.h"
#else
#include <sys/types.h>
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

/*------------------------------------------------------------------------------*
*				Data Structures					*
*-------------------------------------------------------------------------------*/
struct isp_lsc_dec_gain_param {
	/*interleaved lsc gain of 4 channel*/
	const uint16_t *src_data;
	/*number of gain in the x direction for one channel*/
	uint32_t width;
	/*number of gain in the y direction for one channel*/
	uint32_t height;
	/*output lsc gain after gain decreasing*/
	uint16_t *dst_data;
	/*ratio = 1-99, if ratio = 10, means 10% of the gain will be cut off at most*/
	uint32_t ratio;
	uint32_t center_x;
	uint32_t center_y;
};
/*------------------------------------------------------------------------------*
*				Data Prototype					*
*-------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------*
*				Functions					*
*-------------------------------------------------------------------------------*/
int32_t isp_lsc_dec_gain(struct isp_lsc_dec_gain_param *param);
/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/
#endif
// End
