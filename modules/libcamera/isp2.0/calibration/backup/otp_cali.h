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
#ifndef _OTP_CALI_H_
#define _OTP_CALI_H_

/*------------------------------------------------------------------------------*
*				Dependencies					*
*-------------------------------------------------------------------------------*/
#include "sci_types.h"
/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif
/*------------------------------------------------------------------------------*
				Micro Define					*
*-------------------------------------------------------------------------------*/
// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the OTPDLLV01_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// OTPDLLV01_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef OTPDLLV01_EXPORTS
#define OTPDLLV01_API __declspec(dllexport)
#else
#define OTPDLLV01_API __declspec(dllimport)
#endif

#define GR_PATTERN_RAW 		0
#define R_PATTERN_RAW 		1
#define B_PATTERN_RAW 		2
#define GB_PATTERN_RAW 		3

#define PIXEL_CENTER        0
#define GRID_CENTER         1
///////////////////////////////////////////////////////////////////////////////////

/*------------------------------------------------------------------------------*
*				Data Structures					*
*-------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------*
*				Functions														*
*-------------------------------------------------------------------------------*/

//calc Optical Center by pixel or grid, choose type by setting center_type
typedef int32_t (*FNUC_OTP_calcOpticalCenter)(void* raw_image,uint32_t raw_width,uint32_t raw_height, uint32_t data_pattern,
										uint32_t data_type, uint32_t version_id, uint32_t grid, uint32_t base_gain,uint32_t corr_per,
										uint32_t center_type, uint16_t* oc_centerx, uint16_t* oc_centery);

OTPDLLV01_API int32_t calcOpticalCenter(void* raw_image,uint32_t raw_width,uint32_t raw_height, uint32_t data_pattern,
								uint32_t data_type, uint32_t version_id, uint32_t grid, uint32_t base_gain,uint32_t corr_per,
								uint32_t center_type, uint16_t* oc_centerx, uint16_t* oc_centery);

//get the memory size which one channel needs to malloc
typedef int32_t (*FNUC_OTP_getLSCOneChannelSize)(uint32_t grid, uint32_t raw_width,uint32_t raw_height, uint32_t
													lsc_algid, uint32_t compress, uint32_t* chnnel_size);

OTPDLLV01_API int getLSCOneChannelSize(uint32_t grid, uint32_t raw_width,uint32_t raw_height, uint32_t
														lsc_algid, uint32_t compress, uint32_t* chnnel_size);

//calc gains of four channels
typedef int32_t (*FNUC_OTP_calcLSC)(void* raw_image,uint32_t raw_width,uint32_t raw_height, uint32_t data_pattern,uint32_t data_type,
		uint32_t lsc_algid, uint32_t compress, uint32_t grid, uint32_t base_gain, uint32_t corr_per,
		uint16_t* lsc_r_gain,uint16_t* lsc_gr_gain,uint16_t* lsc_gb_gain, uint16_t* lsc_b_gain,uint32_t* lsc_versionid);

OTPDLLV01_API int32_t calcLSC(void* raw_image,uint32_t raw_width,uint32_t raw_height, uint32_t data_pattern,uint32_t data_type,
		uint32_t lsc_algid, uint32_t compress, uint32_t grid, uint32_t base_gain, uint32_t corr_per,
		uint16_t* lsc_r_gain,uint16_t* lsc_gr_gain,uint16_t* lsc_gb_gain, uint16_t* lsc_b_gain,uint32_t* lsc_versionid);

//calc mean r/g/b value of the trim area
typedef int32_t (*FNUC_OTP_calcAWB)(void* raw_image, uint32_t raw_width, uint32_t raw_height,
			uint32_t data_pattern, uint32_t data_type, uint32_t trim_width, uint32_t trim_height, int32 trim_x, int32 trim_y,
			uint16_t* awb_r_Mean, uint16_t* awb_g_Mean, uint16_t* awb_b_Mean, uint32_t* awb_versionid);

OTPDLLV01_API int32_t calcAWB(void* raw_image, uint32_t raw_width, uint32_t raw_height, uint32_t data_pattern,
			uint32_t data_type, uint32_t trim_width, uint32_t trim_height, int32 trim_x, int32 trim_y,
			uint16_t* awb_r_Mean, uint16_t* awb_g_Mean, uint16_t* awb_b_Mean, uint32_t* awb_versionid);


/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/

#endif
// End
