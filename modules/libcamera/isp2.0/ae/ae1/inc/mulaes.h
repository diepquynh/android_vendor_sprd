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
#ifndef _MULAES_H_
#define _MULAES_H_
/*----------------------------------------------------------------------------*
 **                              Dependencies                           *
 **---------------------------------------------------------------------------*/
#include "ae_types.h"
#include "basic_stat.h"
/**---------------------------------------------------------------------------*
 **                              Compiler Flag                          *
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"  {
#endif/*  */
/**---------------------------------------------------------------------------*
**                               Macro Define                           *
**----------------------------------------------------------------------------*/
#define MULAES_CFG_NUM AE_CFG_NUM
/**---------------------------------------------------------------------------*
**                              Data Prototype                                                                                  *
**----------------------------------------------------------------------------*/
	struct mulaes_cfg {
		int16_t x_idx;
		int16_t y_lum;
	};

	struct mulaes_tuning_param {
		uint8_t enable;
		uint8_t num;
		uint16_t reserved;	/*1 * 4bytes */
		struct mulaes_cfg cfg[MULAES_CFG_NUM];/*8 * 4bytes */
	};/*9 * 4bytes */

	typedef struct {
		uint8_t mlog_en;
		int16_t effect_idx;
		int16_t match_lv;
		float real_target;
	} mulaes_in;//tuning info

	typedef struct {
		float artifact_tar;
		int16_t tar_offset;
		char *log;
	} mulaes_rt;//result info

	typedef struct {
		int8_t enable;
		int8_t debug_level;
		int8_t mlog_en;
		int8_t reserved;
		struct mulaes_tuning_param tune_param;
		int16_t dynamic_table[600];
		mulaes_in in_mulaes;
		mulaes_rt result_mulaes;
		uint32_t log_buf[256];
	} mulaes_stat;

/**---------------------------------------------------------------------------*
**				Function Defination 		*
**---------------------------------------------------------------------------*/
	int32_t mulaes_init(mulaes_stat * cxt, struct mulaes_tuning_param *tune_param_ptr);
	int32_t mulaes_calc(mulaes_stat * cxt);
	int32_t mulaes_deinit(mulaes_stat * cxt);

/**----------------------------------------------------------------------------*
**                                      Compiler Flag                           **
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
} 
#endif/*  */
/**---------------------------------------------------------------------------*/
#endif/*  */
