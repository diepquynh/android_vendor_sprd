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
#include "vcm_dw9714a.h"

vcm_ioctl_func_tab_t s_driver_DW9714A_tab = {
	vcm_dw9714A_init,
	vcm_dw9714A_set_position,
};

vcm_info_tab_t driver_dw9714a = {
	VCM_DRIVER_DW9714A,
	&s_driver_DW9714A_tab,
};

uint32_t vcm_dw9714A_init(uint32_t mode)
{
	uint8_t cmd_val[2] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t ret_value = SENSOR_SUCCESS;
	int i = 0;

	slave_addr = DW9714A_VCM_SLAVE_ADDR;
	SENSOR_PRINT("vcm_dw9714A_init: _DW9714A_SRCInit: mode = %d\n", mode);
	switch (mode) {
		case 1:
		break;
		case 2:
		{
			cmd_val[0] = 0xec;
			cmd_val[1] = 0xa3;
			cmd_len = 2;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

			cmd_val[0] = 0xa1;
			cmd_val[1] = 0x0e;
			cmd_len = 2;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

			cmd_val[0] = 0xf2;
			cmd_val[1] = 0x90;
			cmd_len = 2;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

			cmd_val[0] = 0xdc;
			cmd_val[1] = 0x51;
			cmd_len = 2;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
		}
		break;

		case 3:
		break;

	}

	return ret_value;
}
uint32_t vcm_dw9714A_set_position(uint32_t pos,uint32_t slewrate)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t  slave_addr = DW9714A_VCM_SLAVE_ADDR;
	uint16_t cmd_len = 0;
	uint8_t cmd_val[2] = {0x00};

	cmd_val[0] = ((pos&0xfff0)>>4) & 0x3f;
	cmd_val[1] = ((pos&0x0f)<<4) & 0xf0;
	cmd_len = 2;
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

	SENSOR_PRINT("vcm_dw9714A_set_position: _write_af, ret =  %d, param = %d,  MSL:%x, LSL:%x\n",
	ret_value, pos, cmd_val[0], cmd_val[1]);
	return ret_value;
}