/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ver: 1.0
 */

#include <utils/Log.h>
#include "sensor.h"
#include "af_dw9718s.h"

#define DW9718S_VCM_SLAVE_ADDR (0x18 >> 1)
#define MOVE_CODE_STEP_MAX 20
#define WAIT_STABLE_TIME  10    //ms

static int32_t m_cur_dac_code =0;

/*==============================================================================
 * Description:
 * init vcm driver
 * you can change this function acording your spec if it's necessary
 * mode:
 * 1: Direct Mode
 * 2: Dual Level Control Mode
 * 3: Linear Slope Cntrol Mode
 *============================================================================*/
uint32_t dw9718s_init(uint32_t mode)
{
	uint8_t cmd_val[2] = {0x00};
	uint16_t slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t ret_value = SENSOR_SUCCESS;
	SENSOR_PRINT("SENSOR_OV13850R2A: %d",mode);

	slave_addr = DW9718S_VCM_SLAVE_ADDR;
	switch (mode) {
	case 1:
		break;

	case 2:
	{
		cmd_len = 2;

		cmd_val[0] = 0x01;
		cmd_val[1] = 0x39;
		ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
		if(ret_value){
			SENSOR_PRINT("SENSOR_OV13850R2A: _dw9718s_SRCInit 1 fail!");
		}

		cmd_val[0] = 0x05;
		cmd_val[1] = 0x79;
		ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
		if(ret_value){
			SENSOR_PRINT("SENSOR_OV13850R2A: _dw9718s_SRCInit 5 fail!");
		}
	}
		break;
	case 3:
		break;

	}

	return ret_value;
}
/*==============================================================================
 * Description:
 * write code to vcm driver
 * you can change this function acording your spec if it's necessary
 * code: Dac code for vcm driver
 *============================================================================*/
uint32_t dw9718s_write_dac_code(int32_t param)
{

	uint32_t ret_value = SENSOR_SUCCESS;
	uint8_t cmd_val[2] = {0x00};
	uint16_t slave_addr = 0;
	uint16_t cmd_len = 0;

	SENSOR_PRINT("SENSOR_S5K4H5YC: _write_af %ld", param);
	slave_addr = DW9718S_VCM_SLAVE_ADDR;

	cmd_val[0] = 0x02;
	cmd_val[1] = (param>>8)&0x03;
	cmd_len = 2;
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
	SENSOR_PRINT("ov13850r2a_write_af: _write_af, ret =  %d, MSL:%x, LSL:%x\n", ret_value, cmd_val[0], cmd_val[1]);

	cmd_val[0] = 0x03;
	cmd_val[1] = param&0xff;
	cmd_len = 2;
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

	SENSOR_PRINT("ov13850r2a_write_af: _write_af, ret =  %d, MSL:%x, LSL:%x\n", ret_value, cmd_val[0], cmd_val[1]);

	return SENSOR_SUCCESS;
	return ret_value;
}
/*==============================================================================
 * Description:
 * calculate vcm driver dac code and write to vcm driver;
 *
 * Param: ISP write dac code
 *============================================================================*/
uint32_t dw9718s_write_af(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	int32_t target_code=param&0x3FF;

	SENSOR_PRINT("%d", target_code);

	while((m_cur_dac_code-target_code)>=MOVE_CODE_STEP_MAX){
		m_cur_dac_code=m_cur_dac_code-MOVE_CODE_STEP_MAX;
		dw9718s_write_dac_code(m_cur_dac_code);
		usleep(WAIT_STABLE_TIME*1000);
	}

	while((target_code-m_cur_dac_code)>=MOVE_CODE_STEP_MAX){
		m_cur_dac_code=m_cur_dac_code+MOVE_CODE_STEP_MAX;
		dw9718s_write_dac_code(m_cur_dac_code);
		usleep(WAIT_STABLE_TIME*1000);
	}

	if(m_cur_dac_code!=target_code){
		m_cur_dac_code=target_code;
		dw9718s_write_dac_code(m_cur_dac_code);
	}

	return ret_value;
}

/*==============================================================================
 * Description:
 * deinit vcm driver DRV201  PowerOff DRV201
 * you can change this function acording your Module spec if it's necessary
 * mode:
 * 1: PWM Mode
 * 2: Linear Mode
 *============================================================================*/
uint32_t dw9718s_deinit(uint32_t mode)
{
	dw9718s_write_af(0);

	return 0;
}


