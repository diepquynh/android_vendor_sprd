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
#define LOG_TAG "ae_ctrl"
#include "ae_sprd_ctrl.h"
#include "ae_misc.h"
#include "ae_log.h"
#include "ae_utils.h"
#ifndef WIN32
#include <utils/Timers.h>
#include <cutils/properties.h>
#include <math.h>
#else
#include "stdio.h"
#include "sci_types.h"
#endif
#include "lib_ctrl.h"

/**---------------------------------------------------------------------------*
 ** 				Compiler Flag					*
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif
/**---------------------------------------------------------------------------*
**				Macro Define					*
**----------------------------------------------------------------------------*/
#define AE_TUNING_VER 1

#define AE_START_ID 0x71717567
#define AE_END_ID 0x69656E64
#define AE_CONVERGED_NUM 2
#define UNUSED(param)  (void)(param)

#define AE_SAVE_PARAM_STR    "persist.sys.isp.ae.param" /*save/no*/
#define AE_SAVE_AE_TABLE_STR "persist.sys.isp.ae.tbl" /*save/no*/
#define AE_SAVE_MLOG_STR     "persist.sys.isp.ae.mlog" /*save/no*/
#define AE_CAP_USE_PREVIEW_TBL  "persist.sys.isp.ae.use_prv_tbl" /*yes/no*/
#define USE_ISO_TBL 0
#define SEGMENTED_ISO 0
/*should be read from driver later*/
#define AE_FLASH_RATIO (6 * 256)
/*for 30 LUX*/
#define AE_FLASH_ON_OFF_THR 38
#define AE_FLASH_TARGET_LUM 55
#define AE_FLASH_MAX_RATIO 20
#define AE_CALC_TIMES 10
#define ISO_FIX_IN_VIDEO
#define VIDEO_IS0 5
#define CAL_CISION 256
#define QUANTIFICAT 1024

#define AE_MONITOR_WORK_MOD 1 /*0: continus, 1:normal mode*/
#define AE_ALG_ID 0

void* ae_dummy_init(struct ae_init_in *param, struct ae_init_out *result)
{
	return NULL;
}

int32_t ae_dummy_deinit(void *handle, void *in_param, void *out_param)
{
	int32_t rtn = AE_SUCCESS;
	return AE_SUCCESS;
}

#ifndef  AE_WORK_MOD_V2
/*mode 1: ae monitor work in normal mode*/
int32_t ae_dummy_calculation(void* handle, struct ae_calc_in *param, struct ae_calc_out *result)
{
	int32_t rtn = AE_SUCCESS;
	return rtn;
}
#else
/*mode 0: ae monitor work in continue mode*/
int32_t ae_dummy_calculation(void* handle, struct ae_calc_in *param, struct ae_calc_out *result)
{
	int32_t rtn = AE_SUCCESS;
	return rtn;
}

#endif
int32_t ae_dummy_io_ctrl(void* handle, enum ae_io_ctrl_cmd cmd, void *param, void *result)
{
	int32_t rtn = AE_SUCCESS;
	return rtn;
}

extern struct ae_lib_fun ae_lib_fun;

void dummy_ae_fun_init()
{
	ae_lib_fun.ae_init 			= ae_dummy_init;
	ae_lib_fun.ae_deinit			= ae_dummy_deinit;
	ae_lib_fun.ae_calculation		= ae_dummy_calculation;
	ae_lib_fun.ae_io_ctrl			= ae_dummy_io_ctrl;

	return;
}
/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef	__cplusplus
}
#endif

