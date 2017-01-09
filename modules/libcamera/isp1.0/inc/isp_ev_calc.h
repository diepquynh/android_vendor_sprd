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

//#include <sys/types.h>
#include "isp_com.h"

#ifndef _ISP_EV_CALC_H_
#define _ISP_EV_CALC_H_
/*----------------------------------------------------------------------------*
 **				 Dependencies					*
 **---------------------------------------------------------------------------*/

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
#define AE_EV_CALI_NUM 20

#define LOG2(a,b)  (log(a)/log(b))

#if 0
struct ae_lv_cali_tab {
	uint32_t exp;
	uint32_t gain;
	uint32_t lv;
	uint32_t lux;
};
#endif

struct ae_lv_cali_tab {
	uint32_t exp;
	uint32_t gain;
	uint32_t lv;
	uint32_t lux;
};

struct ae_lv_cali {
	uint32_t num;
	struct ae_lv_cali_tab tab[AE_EV_CALI_NUM];
};

struct ae_lv_cali_param {
	uint32_t lv_exp;
	uint32_t lv_gain;
	uint32_t lv;
	uint32_t lux_exp;
	uint32_t lux_gain;
	uint32_t lux;
};

struct ae_ev_cali {
	uint32_t mode;
	uint32_t f_num;
	struct ae_lv_cali_param single;
	struct ae_lv_cali mult;
};

struct ae_bv_calc_in {
	/* ae calc out param*/
	uint32_t index;
	uint32_t max_index;
	uint32_t exp;
	uint32_t gain;
	uint32_t lum;
	uint32_t target_lum;
	uint32_t target_lum_low_thr;
	uint32_t base_lum;
	/* ev cali param*/
	struct ae_ev_cali cali;
};

struct ae_bv_out {
	/* ae calc out param*/
	int32_t av;
	int32_t tv;
	int32_t sv;
	int32_t ev;
	int32_t bv;

	int32_t lv; /*light box value *10 */
	int32_t lux;
};

/**---------------------------------------------------------------------------*
**				Data Prototype					*
**----------------------------------------------------------------------------*/
int32_t ae_bv_init(void* in_param, void* out_param);
int32_t ae_bv_calc(struct ae_bv_calc_in* in_param, struct ae_bv_out* out_param);
/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef	__cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif

