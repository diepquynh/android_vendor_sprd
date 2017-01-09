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
#include "af_dw9714.h"

#define DW9714_VCM_SLAVE_ADDR (0x18 >> 1)
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
uint32_t dw9714_init(uint32_t mode)
{
	uint8_t cmd_val[2] = { 0x00 };
	uint16_t slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t ret_value = SENSOR_SUCCESS;

	slave_addr = DW9714_VCM_SLAVE_ADDR;
	SENSOR_PRINT("mode = %d\n", mode);
	switch (mode) {
	case 1:
		/* When you use direct mode after power on, you don't need register set. Because, DLC disable is default.*/
		break;
	case 2:
		/*Protection off */
		cmd_val[0] = 0xec;
		cmd_val[1] = 0xa3;
		cmd_len = 2;
		ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

		/*DLC and MCLK[1:0] setting */
		cmd_val[0] = 0xa1;
		/*for better performace, cmd_val[1][1:0] should adjust to matching with Tvib of your camera VCM*/
		cmd_val[1] = 0x0e;
		cmd_len = 2;
		ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

		/*T_SRC[4:0] setting */
		cmd_val[0] = 0xf2;
		/*for better performace, cmd_val[1][7:3] should be adjusted to matching with Tvib of your camera VCM*/
		cmd_val[1] = 0x90;
		cmd_len = 2;
		ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

		/*Protection on */
		cmd_val[0] = 0xdc;
		cmd_val[1] = 0x51;
		cmd_len = 2;
		ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);
		break;
	case 3:
		/*Protection off */
		cmd_val[0] = 0xec;
		cmd_val[1] = 0xa3;
		cmd_len = 2;
		ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

		/*DLC and MCLK[1:0] setting */
		cmd_val[0] = 0xa1;
		cmd_val[1] = 0x05;
		cmd_len = 2;
		ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

		/*T_SRC[4:0] setting */
		cmd_val[0] = 0xf2;
		/*for better performace, cmd_val[1][7:3] should be adjusted to matching with the Tvib of your camera VCM*/
		cmd_val[1] = 0x00;
		cmd_len = 2;
		ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

		/*Protection on */
		cmd_val[0] = 0xdc;
		cmd_val[1] = 0x51;
		cmd_len = 2;
		ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);
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
uint32_t dw9714_write_dac_code(int32_t code)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint8_t cmd_val[2] = { 0x00 };
	uint16_t slave_addr = DW9714_VCM_SLAVE_ADDR;
	uint16_t cmd_len = 0;
	uint16_t step_4bit = 0x09;

	SENSOR_PRINT("%d", code);

	cmd_val[0] = (code & 0xfff0) >> 4;
	cmd_val[1] = ((code & 0x0f) << 4) | step_4bit;
	cmd_len = 2;
	ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

	return ret_value;
}
/*==============================================================================
 * Description:
 * calculate vcm driver dac code and write to vcm driver;
 *
 * Param: ISP write dac code
 *============================================================================*/
uint32_t dw9714_write_af(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	int32_t target_code=param&0x3FF;

	SENSOR_PRINT("%d", target_code);

	while((m_cur_dac_code-target_code)>=MOVE_CODE_STEP_MAX){
		m_cur_dac_code=m_cur_dac_code-MOVE_CODE_STEP_MAX;
		dw9714_write_dac_code(m_cur_dac_code);
		usleep(WAIT_STABLE_TIME*1000);
	}

	while((target_code-m_cur_dac_code)>=MOVE_CODE_STEP_MAX){
		m_cur_dac_code=m_cur_dac_code+MOVE_CODE_STEP_MAX;
		dw9714_write_dac_code(m_cur_dac_code);
		usleep(WAIT_STABLE_TIME*1000);
	}

	if(m_cur_dac_code!=target_code){
		m_cur_dac_code=target_code;
		dw9714_write_dac_code(m_cur_dac_code);
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
uint32_t dw9714_deinit(uint32_t mode)
{
	dw9714_write_af(0);

	return 0;
}

