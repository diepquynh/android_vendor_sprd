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

#ifndef _AE_MISC_H_
#define _AE_MISC_H_
/*----------------------------------------------------------------------------*
 **				 Dependencies				*
 **---------------------------------------------------------------------------*/
#include "ae_types.h"
/**---------------------------------------------------------------------------*
 **				 Compiler Flag				*
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif

/**---------------------------------------------------------------------------*
**				 Macro Define				*
**----------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**				Data Prototype				*
**----------------------------------------------------------------------------*/

enum ae_misc_io_cmd {
	AE_MISC_CMD_SET = 0x0,
	AE_MISC_CMD_SET_AE_TABLE = 0x1,
	AE_MISC_CMD_SET_WEIGHT_TABLE = 0x2,
	AE_MISC_CMD_SET_WORK_PARAM = 0x3,
	AE_MISC_CMD_SET_TARGET_LUM = 0x4,
	AE_MISC_CMD_SET_QUICK_MODE = 0x5,
	AE_MISC_CMD_SET_FLASH_PARAM = 0x6,
	AE_MISC_CMD_SET_FPS = 0x7,
	AE_MISC_CMD_SET_INDEX = 0x8,
	AE_MISC_CMD_SET_EXP_ANIT = 0x9,
	AE_MISC_CME_SET_FIX_FPS = 0xA,
	AE_MISC_CMD_SET_CVGN_PARAM = 0xB,
	AE_MISC_CMD_SET_CONVERGE_SPEED = 0xC,
	AE_MISC_CMD_SET_EV = 0xD,
	AE_MISC_CMD_GET =0x100,
	AE_MISC_CMD_GET_EXP_BY_INDEX,
	AE_MISC_CMD_GET_NEW_INDEX,
	AE_MISC_CMD_GET_WEIGHT
};

struct ae_misc_init_in {
	uint32_t alg_id;
	uint32_t start_index;
	uint32_t cvt_mode;
};

struct ae_misc_init_out {
	uint32_t start_index;
};


struct ae_misc_work_out {
	uint32_t cur_lum;
	uint32_t cur_index;
	uint32_t cur_ev;
	uint32_t cur_exposure;
	uint32_t cur_dummy;
	uint32_t cur_again;
	uint32_t cur_dgain;
	uint32_t target_lum;
};

struct ae_misc_work_in {
	uint32_t alg_id;
	struct ae_exp_gain_table *ae_table;
	struct ae_weight_table *weight_table;
	uint32_t quick_mode;
	uint32_t target_lum;
	uint32_t target_lum_zone; 	// precent  100% 256
	uint32_t convergence_speed; 	// precent  100% 256  the low the fast
	uint32_t line_time;
	uint32_t min_line;
	uint32_t start_index;
};

struct ae_misc_cvgn_param {
	struct ae_convergence_parm *cvgn_param;
	uint32_t target_lum;
};


struct ae_misc_awb_gain {
	uint32_t r;
	uint32_t g;
	uint32_t b;
};

struct ae_misc_calc_in {
	void* stat_img;
	int32_t bv;
	struct ae_size win_size;
	struct ae_size win_num;
	uint32_t stat_mode;
	struct ae_misc_awb_gain awb_gain;
	uint32_t fd_state;
	struct ae_history *history;
	uint32_t ae_pause;
};

struct ae_misc_exp_gain {
	uint32_t exp_line;
	uint32_t exp_dummy;
	uint32_t again;
	uint32_t dgain;
	uint32_t index;
};

struct ae_misc_exp_gain_tab {
	uint32_t num;
	struct ae_misc_exp_gain tab[16];
};

struct ae_misc_calc_out {
	uint32_t cur_lum;
	uint32_t cur_index;
	uint32_t cur_exp_line;
	uint32_t cur_exposure;
	uint32_t cur_dummy;
	uint32_t cur_again;
	uint32_t cur_dgain;
	uint32_t stab;
	uint32_t lum_low_thr;
	uint32_t lum_high_thr;
	uint32_t min_index;
	uint32_t max_index;
	uint32_t target_lum;
	struct ae_misc_exp_gain_tab exp;
};


void* ae_misc_init(struct ae_misc_init_in *in_param, struct ae_misc_init_out *out_param);
int32_t ae_misc_deinit(void* handle, void *in_param, void *out_param);
int32_t ae_misc_calculation(void* handle, struct ae_misc_calc_in *in_param, struct ae_misc_calc_out *out_param);
int32_t ae_misc_io_ctrl(void* handle, enum ae_misc_io_cmd cmd, void *in_param, void *out_param);
/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif

