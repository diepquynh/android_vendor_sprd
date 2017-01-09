/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _AAA_AUTO_ADJUST_H_
#define _AAA_AUTO_ADJUST_H_
/*----------------------------------------------------------------------------*
 **				 Dependencies					*
 **---------------------------------------------------------------------------*/
#include <sys/types.h>
//#include "aaa_type.h"
/**---------------------------------------------------------------------------*
 **				 Compiler Flag					*
 **---------------------------------------------------------------------------*/
#ifdef	 __cplusplus
extern	 "C"
{
#endif

/**---------------------------------------------------------------------------*
**				 Micro Define					*
**----------------------------------------------------------------------------*/
#define AUTO_INVALID 0xffffffff
#define AUTO_ADJUST_EB 1

#define DENOISE_INDEX_NUM 10
#define DENOISE_LUM_NUM 4

typedef int32_t (* auto_adjust_fun)(uint32_t handler_id, void* in_param_ptr, void* out_param_ptr);

enum auto_adjust_ioctrl_cmd {
	AUTO_ADJUST_CMD_MAX=0xffffffff,
};

struct auto_adjust{
	uint32_t enable;

	uint32_t index_sensitive;
	uint32_t lum_sensitive;

	uint32_t dependon_index;
	uint32_t dependon_gain;
	uint32_t dependon_lum;

	uint32_t level_mode;
	uint32_t index_zone;
	uint32_t lum_zone;
	uint32_t target_lum_zone;

	uint32_t index_thr_num;
	uint32_t lum_low_thr_num;
	uint16_t index_thr[DENOISE_INDEX_NUM];
	uint16_t index_start_level[DENOISE_INDEX_NUM];
	uint16_t index_end_level[DENOISE_INDEX_NUM];
	uint16_t bais_gain[DENOISE_INDEX_NUM];

	uint16_t lum_low_thr[DENOISE_LUM_NUM];
	uint16_t lum_start_level[DENOISE_LUM_NUM];
	uint16_t lum_end_level[DENOISE_LUM_NUM];
};

struct bil_param {
	uint8_t diswei[19];
	uint8_t reserved1;
	uint8_t ranwei[31];
	uint8_t reserved0;
};

struct gamma_param{
	uint16_t axis[2][26];
};

struct cmc_param{
	uint16_t matrix[9];
	uint16_t reserved;
};

struct y_denoise_param{
	uint8_t y_thr;
};

struct uv_denoise_param {
	uint8_t u_thr;
	uint8_t v_thr;
};

struct edge_param {
	uint8_t detail_thr;
	uint8_t smooth_thr;
	uint8_t strength;
	uint8_t reserved;
};

struct auto_adjust_init_info {
	struct auto_adjust bil_denoise;
	struct auto_adjust y_denoise;
	struct auto_adjust uv_denoise;
	struct auto_adjust cmc;
	struct auto_adjust gamma;
	struct auto_adjust edge;

	struct bil_param bil_param[DENOISE_INDEX_NUM+DENOISE_LUM_NUM];
	struct y_denoise_param y_denoise_param[DENOISE_INDEX_NUM+DENOISE_LUM_NUM];
	struct uv_denoise_param uv_denoise_param[DENOISE_INDEX_NUM+DENOISE_LUM_NUM];
	struct cmc_param cmc_param[DENOISE_INDEX_NUM+DENOISE_LUM_NUM];
	struct gamma_param gamma_param[DENOISE_INDEX_NUM+DENOISE_LUM_NUM];
	struct edge_param edge_param[DENOISE_INDEX_NUM+DENOISE_LUM_NUM];

	auto_adjust_fun bil_fun;
	auto_adjust_fun y_denoise_fun;
	auto_adjust_fun uv_denoise_fun;
	auto_adjust_fun gamma_fun;
	auto_adjust_fun cmc_fun;
	auto_adjust_fun edge_fun;
};

struct auto_adjust_calc_info {
	uint32_t cur_lum;
	uint32_t cur_index;
	uint32_t min_index;
	uint32_t max_index;
	uint32_t cur_gain;
	uint32_t max_gain;
	uint32_t target_lum_thr;
};

struct auto_adjust_out{
	uint32_t level;
	uint32_t level_type;
	uint32_t index[2];
	uint32_t alpha;
};

/**---------------------------------------------------------------------------*
**				Data Prototype					*
**----------------------------------------------------------------------------*/
int32_t auto_adjust_init(uint32_t handler_id, void* in_param_ptr, void* out_param_ptr);
int32_t auto_adjust_calc(uint32_t handler_id, void* in_param_ptr, void* out_param_ptr);
int32_t auto_adjust_ioctrl(uint32_t handler_id, enum auto_adjust_ioctrl_cmd cmd, void* in_param_ptr, void* out_param_ptr);
int32_t auto_adjust_deinit(uint32_t handler_id, void* in_param_ptr, void* out_param_ptr);

/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef	__cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif

