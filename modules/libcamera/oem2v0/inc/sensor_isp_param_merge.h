/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*----------------------------------------------------------------------------*
 **				Dependencies					*
 **---------------------------------------------------------------------------*/

#include <sys/types.h>
#include "ae_types.h"
#include "sensor_isp_param_awb_pac.h"
#include "awb_packet.h"
#include "awb_alg_param.h"
#include "isp_app.h"
#include "isp_com.h"
#include "sensor_raw.h"
#include "cmr_common.h"
#include "sensor_drv_u.h"

/**---------------------------------------------------------------------------*
 **				Compiler Flag					*
 **---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

int  sensor_isp_param_merge(struct sensor_raw_info* sensor_info_ptr,struct isp_data_info* out_param ,int mode);
int sensor_meger_isp_param(struct isp_init_param *init_param_ptr,struct sensor_raw_info *sensor_info_ptr);
int sensor_merge_isp_param_free(isp_ctrl_context *isp_cxt );


/**----------------------------------------------------------------------------*
**				Compiler Flag					*
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif

