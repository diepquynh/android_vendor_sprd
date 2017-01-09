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
#include "vcm_dw8714.h"

vcm_ioctl_func_tab_t s_driver_DW8714_tab = {
	PNULL,
	vcm_dw8714_set_position,
};

vcm_info_tab_t driver_dw8714 = {
	VCM_DRIVER_DW8714,
	&s_driver_DW8714_tab,
};


uint32_t vcm_dw8714_set_position(uint32_t pos,uint32_t slewrate)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint8_t cmd_val[2] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;
	//for direct mode
	slave_addr = DW8714_VCM_SLAVE_ADDR;
	cmd_val[0] = (pos&0xfff0)>>4;
	cmd_val[1] = (pos&0x0f)<<4;
	cmd_len = 2;
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
	return ret_value;
}