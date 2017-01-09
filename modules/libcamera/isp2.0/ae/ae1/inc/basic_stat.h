/*
* Copyright (C) 2012 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#ifndef _STAT_BASIC_H_
#define _STAT_BASIC_H_
/*----------------------------------------------------------------------------*
**				 Dependencies				*
**---------------------------------------------------------------------------*/

#ifdef CONFIG_FOR_TIZEN
#include "stdint.h"
#elif WIN32
#include "ae_porting.h"
#else/*  */
#include <sys/types.h>
#endif	/*  */
/**---------------------------------------------------------------------------*
**				 Compiler Flag				*
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
	
#endif	/*  */
/**---------------------------------------------------------------------------*
**				Macro Define				*
**----------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**				Data Structures				*
**---------------------------------------------------------------------------*/
	typedef struct  {
		int16_t top_range;//range of bigger value
		int16_t data_len;
		int8_t debug_level;
		int16_t bright_thr;
		int16_t dark_thr;
	} basic_in;//tuning info

	typedef struct  {
		int16_t histogram[256];
		int16_t media_1;
		int16_t media_2;
		float mean;
		int16_t top;	//average of bigger value
		int16_t maximum;
		int16_t minimum;
		float var;
		int16_t bright_num;
		int16_t dark_num;
		int16_t normal_num;
	} basic_rt;//calc info

	typedef struct  {
		basic_in in_basic;
		basic_rt result_basic;
	} basic_stat;//statistic information of single channel
/**---------------------------------------------------------------------------*
** 				Function Defination			*
**---------------------------------------------------------------------------*/
	int32_t initbasic(basic_stat * basic, uint32_t debug_level);
	int32_t deinitbasic(basic_stat * basic);
	int32_t calcbasic(basic_stat * basic, int len, uint8_t * data);
	int32_t round_ae(float iodata);

/**----------------------------------------------------------------------------*
**					Compiler Flag			*
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif	/*  */

#endif	/*  */