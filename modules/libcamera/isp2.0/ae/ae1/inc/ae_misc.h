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
extern "C" {
#endif

/**---------------------------------------------------------------------------*
**				 Macro Define				*
**----------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**				Data Prototype				*
**----------------------------------------------------------------------------*/
struct ae_misc_init_in {
	uint32_t alg_id;
	uint32_t start_index;
	void *param_ptr;
	uint32_t size;
};

struct ae_misc_init_out {
	uint32_t start_index;
	char alg_id[32];
};

struct ae_misc_calc_in {
	void *sync_settings;
};

struct ae_misc_calc_out {
	void *ae_output;
};

void *ae_misc_init(struct ae_misc_init_in *in_param, struct ae_misc_init_out *out_param);
int32_t ae_misc_deinit(void *handle, void *in_param, void *out_param);
int32_t ae_misc_calculation(void *handle, struct ae_misc_calc_in *in_param, struct ae_misc_calc_out *out_param);
/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif
