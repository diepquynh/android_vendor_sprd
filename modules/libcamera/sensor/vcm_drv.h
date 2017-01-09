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
#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"
#define VCM_UNIDENTIFY 0xFFFF

typedef struct vcm_ioctl_func_tab_tag{
	uint32_t(*vcm_init)(uint32_t mode);
	uint32_t(*vcm_set_pos)(uint32_t pos,uint32_t slewrate);
}vcm_ioctl_func_tab_t;

typedef enum{
	VCM_DRIVER_DW9714 = 0,
	VCM_DRIVER_DW9714A,
	VCM_DRIVER_DW9806,
	VCM_DRIVER_DW8714,
	VCM_DRIVER_OV8825,
	VCM_DRIVER_MAX
}vcm_id;

typedef struct vcm_info_tab_tag{
	uint32_t vcm_cur_id;
	vcm_ioctl_func_tab_t *vcm_func;
}vcm_info_tab_t;

uint32_t vcm_init(uint32_t id,uint32_t mode);
uint32_t vcm_go_position(uint32_t id, uint32_t pos,uint32_t slewrate);
