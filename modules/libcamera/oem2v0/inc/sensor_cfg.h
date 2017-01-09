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
#ifndef _SENSOR_CFG_H_
#define _SENSOR_CFG_H_

#include "sensor_drv_u.h"

#if 0
struct sensor_drv_cfg {
	struct list_head list;
	uint32_t sensor_pos;
	const char *sensor_name;
	SENSOR_INFO_T *driver_info;
};
int dcam_register_sensor_drv(struct sensor_drv_cfg *cfg);
struct list_head *Sensor_GetList(SENSOR_ID_E sensor_id);

#endif
typedef struct sensor_match_tab{
	char  sn_name[36];
	SENSOR_INFO_T* sensor_info;
}SENSOR_MATCH_T;
SENSOR_MATCH_T * Sensor_GetInforTab(struct sensor_drv_context *sensor_cxt, SENSOR_ID_E sensor_id);
uint32_t Sensor_GetInforTabLenght(struct sensor_drv_context *sensor_cxt, SENSOR_ID_E sensor_id);
cmr_u32 Sensor_IndexGet(struct sensor_drv_context *sensor_cxt, cmr_u32 index);
#endif
