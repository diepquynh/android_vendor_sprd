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
#include "vcm_dw_9714.h"

vcm_ioctl_func_tab_t s_driver_DW9714_tab = {
	vcm_dw9714_init,
	vcm_dw9714_set_position,
};

vcm_info_tab_t driver_dw9714 = {
	VCM_DRIVER_DW9714,
	&s_driver_DW9714_tab,
};

uint32_t vcm_dw9714_init(uint32_t mode)
{
	uint8_t cmd_val[6] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t ret_value = SENSOR_SUCCESS;
	slave_addr = DW9714_VCM_SLAVE_ADDR;

	switch (mode) {
		case 1:
		break;

		case 2:
		{
			cmd_val[0] = 0xec;
			cmd_val[1] = 0xa3;
			cmd_val[2] = 0xf2;
			cmd_val[3] = 0x00;
			cmd_val[4] = 0xdc;
			cmd_val[5] = 0x51;
			cmd_len = 6;
			Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
		}
		break;

		case 3:
		break;

	}

	return ret_value;
}
uint32_t vcm_dw9714_set_position(uint32_t pos,uint32_t slewrate)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint8_t cmd_val[2] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;
	SENSOR_PRINT_HIGH("VCM pos %d  ",pos);

	slave_addr = DW9714_VCM_SLAVE_ADDR;
	cmd_val[0] = (pos&0xfff0)>>4;
	cmd_val[1] = ((pos&0x0f)<<4)|0x09;
	cmd_len = 2;
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
	SENSOR_PRINT_HIGH("VCM ret_value%d  ",ret_value);
	return ret_value;
}