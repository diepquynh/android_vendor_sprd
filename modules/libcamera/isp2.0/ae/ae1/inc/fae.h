/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */  
#ifndef _FAE_H_
#define _FAE_H_
/*----------------------------------------------------------------------------*
 **                              Dependencies                           *
 **---------------------------------------------------------------------------*/
#include "ae_types.h"
#include "basic_stat.h"
#include "ae1_face.h"
/**---------------------------------------------------------------------------*
 **                              Compiler Flag                          *
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"  {
#endif	/*  */
/**---------------------------------------------------------------------------*
**                               Macro Define                           *
**----------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**                              Data Prototype                          *
**----------------------------------------------------------------------------*/
	typedef struct  {
		//uint8_t enable;
		uint8_t mlog_en;
		int8_t debug_level;
		struct ae1_fd_param *alg_face_info;
		struct face_tuning_param face_tuning;
		uint8_t * ydata;
		uint32_t frame_ID;
		//float face_target;
		float real_target;
		float current_lum;
	} fae_in;//tuning info

	typedef struct  {
		int32_t fd_count;
		float face_stable[25];
		int32_t no_fd_count;
		int8_t fidelity;
		float face_lum;
		float face_roi_lum;
		float no_face_lum;
		float tar_offset;
		float ftar_offset;//
		float target_offset_smooth[5];
		char* log;
	} fae_rt;//calc info

	typedef struct {
		uint8_t mlog_en;
		uint8_t debug_level;
		fae_in in_fae;
		fae_rt result_fae;
		/*algorithm status*/
		struct ae1_face_info cur_info;/*297 x 4bytes*/
		struct ae1_face_info prv_info;/*297 x 4bytes*/
		uint32_t log_buf[256];
	} fae_stat;

/**---------------------------------------------------------------------------*
**                              EBD Function Prototype                          *
**----------------------------------------------------------------------------*/
	//int32_t fae_init(fae_stat * cxt);
	int32_t fae_init(fae_stat * cxt, struct face_tuning_param *face_tuning_prt);
	int32_t fae_calc(fae_stat * fae);
	int32_t fae_deinit(fae_stat * cxt);

/**----------------------------------------------------------------------------*
**                                      Compiler Flag                           **
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif/*  */
/**---------------------------------------------------------------------------*/
#endif/*  */

