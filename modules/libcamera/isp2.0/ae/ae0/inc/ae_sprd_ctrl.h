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

#ifndef _AE_CTRL_H_
#define _AE_CTRL_H_
/*----------------------------------------------------------------------------*
 **				 Dependencies				*
 **---------------------------------------------------------------------------*/
#include "ae_ctrl_types.h"
/**---------------------------------------------------------------------------*
 **				 Compiler Flag				*
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif
struct ae_in_out {
	uint16_t cur_exp_line;/*current exposure line: the value is related to the resolution */
	uint16_t cur_dummy;/*dummy line: the value is related to the resolution & fps */
	int16_t cur_gain;/*current analog gain */
	int16_t enable;
};

void* ae_sprd_init(struct ae_init_in *in_param, struct ae_init_out *out_param);
int32_t ae_sprd_deinit(void* handler, void *in_param, void *out_param);
int32_t ae_sprd_calculation(void* handler, struct ae_calc_in *in_param,
		struct ae_calc_out *out_param);
int32_t ae_sprd_io_ctrl(void* handler, enum ae_io_ctrl_cmd cmd, void *in_param,
		void *out_param);

/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif

