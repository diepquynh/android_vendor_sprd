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
#include "vcm_dw9806.h"

vcm_ioctl_func_tab_t s_driver_DW9806_tab = {
	vcm_dw9806_init,
	vcm_dw9806_set_position,
};

vcm_info_tab_t driver_dw9806 = {
	VCM_DRIVER_DW9806,
	&s_driver_DW9806_tab,
};

uint32_t vcm_dw9806_init(uint32_t mode)
{
	uint8_t cmd_val[6] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t ret_value = SENSOR_SUCCESS;

	slave_addr = DW9806_VCM_SLAVE_ADDR;

	switch (mode) {
		case 1:
		break;

		case 2:
		{
			cmd_len = 2;
			cmd_val[0] = 0x02;
			cmd_val[1] = 0x01;
			Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
			usleep(1000);
			cmd_val[0] = 0x02;
			cmd_val[1] = 0x00;
			Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
			usleep(1000);
			cmd_val[0] = 0x02;
			cmd_val[1] = 0x02;
			Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
			usleep(1000);
			cmd_val[0] = 0x06;
			cmd_val[1] = 0x61;
			Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
			usleep(1000);
			cmd_val[0] = 0x07;
			cmd_val[1] = 0x1c;
			Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
			usleep(1000);
		}
		break;

		case 3:
		break;

	}

	return ret_value;
}
uint32_t vcm_dw9806_set_position(uint32_t pos,uint32_t slewrate)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint8_t cmd_val[2] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t time_out = 0;
	slave_addr = DW9806_VCM_SLAVE_ADDR;
	cmd_len = 2;
	cmd_val[0] = 0x03;
	cmd_val[1] = (pos&0x300)>>8;
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
	cmd_val[0] = 0x04;
	cmd_val[1] = (pos&0xff);
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
	return ret_value;
}