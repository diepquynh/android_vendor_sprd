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
#ifndef _FLAT_H_
#define _FLAT_H_
/*----------------------------------------------------------------------------*
 **                              Dependencies                           *
 **---------------------------------------------------------------------------*/
#include "ae_types.h"
#include "basic_stat.h"
#include "ae_utils.h"
/**---------------------------------------------------------------------------*
 **                              Compiler Flag                          *
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"  {
#endif/*  */
/**---------------------------------------------------------------------------*
**                               Macro Define                           *
**----------------------------------------------------------------------------*/
#define FLAT_CFG_NUM AE_CFG_NUM
/**---------------------------------------------------------------------------*
**                              Data Prototype                          *
**----------------------------------------------------------------------------*/
typedef struct {
	int16_t thrd[2];
	int16_t offset[2];
} flat_cfg;/*2 * 4bytes */

struct flat_tuning_param {
	/*1 * 4bytes */
	uint8_t enable;
	uint8_t num;
	uint16_t reserved;
	/*flat tune param; total 8 group */
	flat_cfg cfg_info[FLAT_CFG_NUM];/*16 * 4bytes */
	struct ae_piecewise_func out_piecewise;/*17 * 4bytes */
	struct ae_piecewise_func in_piecewise;/*17 * 4bytes */
};/*51 * 4bytes */

typedef struct  {
	uint8_t mlog_en;
	uint16_t down_scale;
	uint8_t * ydata;
	float real_target;
	int32_t match_lv;
} flat_in;//tuning info

typedef struct  {
	int16_t tar_offset;
	int16_t input_interpolation[4];
	float degree;
	float strength;
	char* log_ptr;
} flat_rt;//calc info

typedef struct  {
	int8_t enable;
	int8_t debug_level;
	uint8_t mlog_en;
	struct flat_tuning_param tune_param;
	flat_in in_flat;
	flat_rt result_flat;
	uint32_t mlog_buf[256];
} flat_stat;
/**---------------------------------------------------------------------------*
**                              FLAT Function Prototype                          *
**----------------------------------------------------------------------------*/
int32_t flat_init(flat_stat * cxt, struct flat_tuning_param *tune_param_ptr);
int32_t flat_calc(flat_stat * cxt);
int32_t flat_deinit(flat_stat * cxt);
/**----------------------------------------------------------------------------*
**                                      Compiler Flag                           **
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif/*  */
/**---------------------------------------------------------------------------*/
#endif/*  */
