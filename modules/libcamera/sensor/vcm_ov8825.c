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
#include "vcm_ov8825.h"

vcm_ioctl_func_tab_t s_driver_OV8825_tab = {
	PNULL,
	vcm_ov8825_set_position,
};

vcm_info_tab_t driver_ov8825 = {
	VCM_DRIVER_OV8825,
	&s_driver_OV8825_tab,
};

uint32_t vcm_ov8825_set_position(uint32_t pos,uint32_t slewrate)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint16_t reg_val = 0x00;
	SENSOR_PRINT("SENSOR_ov8825: _write_af 0x%x", pos);

	value = (pos&0xf)<<0x04;
	value = value + 8 + (slewrate&0x7);
	ret_value = Sensor_WriteReg(0x3618, value);
	value = (pos&0x3f0)>>0x04;
	ret_value = Sensor_WriteReg(0x3619, value);

	return ret_value;
}