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
/*----------------------------------------------------------------------------*
 **				Dependencies					*
 **---------------------------------------------------------------------------*/
#include "isp_param_size.h"
/**---------------------------------------------------------------------------*
 **				Compiler Flag					*
 **---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

/**---------------------------------------------------------------------------*
**				Micro Define					*
**----------------------------------------------------------------------------*/
struct isp_size_info s_isp_size_info[]={
	{ISP_SIZE_480x270, 480, 270},
	{ISP_SIZE_640x480, 640, 480},
	{ISP_SIZE_480x360, 480, 360},
	{ISP_SIZE_800x600, 800, 600},
	{ISP_SIZE_1280x960, 1280, 960},
	{ISP_SIZE_1296x972, 1296, 972},
	{ISP_SIZE_1280x1024, 1280, 1024},
	{ISP_SIZE_1600x1200, 1600, 1200},
	{ISP_SIZE_1632x1224, 1632, 1224},
	{ISP_SIZE_1920x1080, 1920, 1080},
	{ISP_SIZE_2048x1536, 2048, 1536},
	{ISP_SIZE_2112x1568, 2112, 1568},
	{ISP_SIZE_2592x1944, 2592, 1944},
	{ISP_SIZE_3264x2448, 3264, 2448},
	{ISP_SIZE_4144x3106, 4144, 3106},
	{ISP_SIZE_4208x3120, 4208, 3120},
	{ISP_SIZE_END, ISP_SIZE_END, ISP_SIZE_END}
};
/**---------------------------------------------------------------------------*
*				Data Prototype					*
**----------------------------------------------------------------------------*/
struct isp_size_info* ISP_ParamGetSizeInfo(void)
{
	return s_isp_size_info;
}

/**----------------------------------------------------------------------------*
**				Compiler Flag					*
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/

// End

