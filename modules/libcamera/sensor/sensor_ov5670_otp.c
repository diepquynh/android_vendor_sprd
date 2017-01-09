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
#include "sensor_drv_u.h"
#include "sensor_raw.h"

#define DBG_OTP
#ifdef DBG_OTP
#define DEBUG_OTP_STR     "OV5670_OTP_debug: L %d, %s: "
#define DEBUG_OTP_ARGS    __LINE__,__FUNCTION__

#define OTP_PRINT(format,...) ALOGE(DEBUG_OTP_STR format, DEBUG_OTP_ARGS, ##__VA_ARGS__)
#else
#define OTP_PRINT
#endif

#define OV5670_RAW_PARAM_Sunny    0x0001
#define OV5670_RAW_PARAM_Truly    0x0002
#define OV5670_RAW_PARAM_Sunrise  0x000c

struct otp_typical_value_t{
 	 uint16_t RG_Ratio_Typical ;
	 uint16_t BG_Ratio_Typical ;
};

struct otp_struct
	{
	int 	flag; // bit[7]: info, bit[6]:wb
	int	module_integrator_id;
	int	lens_id;
	int	production_year;
	int	production_month;
	int	production_day;
	int	rg_ratio;
	int	bg_ratio;
	};

static struct otp_struct current_otp = {0x00};
static struct otp_typical_value_t otp_typical_value = {0x00};

LOCAL int ov5670_read_otp(struct otp_struct *otp_ptr)
	{
		int otp_flag, addr, temp, i;

		//set 0x5002[3] to ¡°0¡±
		int temp1;

		Sensor_WriteReg(0x0100, 0x01);
		temp1 = 	Sensor_ReadReg(0x5002);
		Sensor_WriteReg(0x5002, (0x00 & 0x08) | (temp1 & (~0x08)));
		// read OTP into buffer
		Sensor_WriteReg(0x3d84, 0xC0);
		Sensor_WriteReg(0x3d88, 0x70); // OTP start address
		Sensor_WriteReg(0x3d89, 0x10);
		Sensor_WriteReg(0x3d8A, 0x70); // OTP end address
		Sensor_WriteReg(0x3d8B, 0x29);
		Sensor_WriteReg(0x3d81, 0x01); // load otp into buffer
		usleep(5*1000);
		// OTP into
		otp_flag = Sensor_ReadReg(0x7010);
		SENSOR_PRINT("SENSOR_OV5670:otp_flag = 0x%x \n",otp_flag);

		addr = 0;
		if((otp_flag & 0xc0) == 0x40)
			{
			addr = 0x7011; // base address of info group 1
			}
		else if((otp_flag & 0x30) == 0x10)
			{
			addr = 0x7016; // base address of info group 2
			}
		else if((otp_flag & 0x0c) == 0x04)
			{
			addr = 0x701b; // base address of info group 3
			}
		if(addr != 0)
			{
			(*otp_ptr).flag = 0x80; // valid base info in OTP
			(*otp_ptr).module_integrator_id = Sensor_ReadReg( addr );
			(*otp_ptr).lens_id = Sensor_ReadReg( addr + 1);
			(*otp_ptr).production_year = Sensor_ReadReg( addr + 2);
			(*otp_ptr).production_month = Sensor_ReadReg( addr + 3);
			(*otp_ptr).production_day = Sensor_ReadReg( addr + 4);
			}
		else
			{
			(*otp_ptr).flag = 0x00; // not info in OTP
			(*otp_ptr).module_integrator_id = 0;
			(*otp_ptr).lens_id = 0;
			(*otp_ptr).production_year = 0;
			(*otp_ptr).production_month = 0;
			(*otp_ptr).production_day = 0;
			}

			// OTP WB Calibration
			otp_flag = Sensor_ReadReg(0x7020);
			addr = 0;
		if((otp_flag & 0xc0) == 0x40)
			{
			addr = 0x7021; // base address of WB Calibration group 1
			}
		else if((otp_flag & 0x30) == 0x10)
			{
			addr = 0x7024; // base address of WB Calibration group 2
			}
		else if((otp_flag & 0x0c) == 0x04)
			{
			addr = 0x7027; // base address of WB Calibration group 3
			}
		if(addr != 0)
			{
			(*otp_ptr).flag |= 0x40;
			temp = 	Sensor_ReadReg( addr + 2);
			(*otp_ptr).rg_ratio = (Sensor_ReadReg(addr)<<2) + ((temp>>6) & 0x03);
			(*otp_ptr).bg_ratio = (Sensor_ReadReg( addr + 1)<<2) + ((temp>>4) & 0x03);
			}
		else{
			(*otp_ptr).rg_ratio = 0;
			(*otp_ptr).bg_ratio = 0;
			}

		for(i=0x7010;i<=0x7029;i++)
			{
			Sensor_WriteReg(i,0); // clear OTP buffer, recommended use continuous write to accelarate
			}
			//set 0x5002[3] to ¡°1¡±
			temp1 = 	Sensor_ReadReg(0x5002);
			Sensor_WriteReg(0x5002, (0x08 & 0x08) | (temp1 & (~0x08)));
			SENSOR_PRINT("SENSOR_OV5670:addr = 0x%x \n",addr);
			SENSOR_PRINT("SENSOR_OV5670: (*otp_ptr).flag = 0x%x \n",(*otp_ptr).flag);
			SENSOR_PRINT("SENSOR_OV5670: (*otp_ptr).module_integrator_id = 0x%x \n",(*otp_ptr).module_integrator_id);
			SENSOR_PRINT("SENSOR_OV5670: (*otp_ptr).rg_ratio = 0x%x \n",(*otp_ptr).rg_ratio);
			SENSOR_PRINT("SENSOR_OV5670: (*otp_ptr).bg_ratio = 0x%x \n",(*otp_ptr).bg_ratio);
			SENSOR_PRINT("SENSOR_OV5670: otp_tipical_value.rg_ratio = 0x%x \n",otp_typical_value.RG_Ratio_Typical);
			SENSOR_PRINT("SENSOR_OV5670: otp_tipical_value.bg_ratio = 0x%x \n",otp_typical_value.BG_Ratio_Typical);

		return (*otp_ptr).flag;
	}

LOCAL uint32_t ov5670_check_otp_module_id(void)
{

	SENSOR_PRINT("check ov5670 otp \n");
//	struct otp_struct current_otp ;
	/*
	int i = 0;
	int otp_index = 0;
	int temp = 0;
*/
	ov5670_read_otp(&current_otp);
	SENSOR_PRINT("read ov5670 current_otp.module_integrator_id = %x \n", (current_otp.module_integrator_id));

	return (current_otp.module_integrator_id);

}
// return value:
// bit[7]: 0 no otp info, 1 valid otp info
// bit[6]: 0 no otp wb, 1 valib otp wb
LOCAL uint32_t ov5670_apply_otp()
{

	SENSOR_PRINT("apply ov5670 otp \n");

	int rg, bg, R_gain, G_gain, B_gain, Base_gain;

	SENSOR_PRINT("SENSOR_OV5670: current_otp.rg_ratio = 0x%x \n",current_otp.rg_ratio);
	SENSOR_PRINT("SENSOR_OV5670: current_otp.bg_ratio = 0x%x \n",current_otp.bg_ratio);
	SENSOR_PRINT("SENSOR_OV5670: current_otp.flag = 0x%x \n",current_otp.flag);

	// apply OTP WB Calibration
	if (current_otp.flag & 0x40)
	{
		rg = current_otp.rg_ratio;
		bg = current_otp.bg_ratio;
		//calculate G gain
		R_gain = (otp_typical_value.RG_Ratio_Typical*1000) / rg;
		B_gain = (otp_typical_value.BG_Ratio_Typical*1000) / bg;
		G_gain = 1000;

		if (R_gain < 1000 || B_gain < 1000)
			{
				if (R_gain < B_gain)
				Base_gain = R_gain;
				else
				Base_gain = B_gain;
			}
	else
		{
		Base_gain = G_gain;
		}
		R_gain = 0x400 * R_gain / (Base_gain);
		B_gain = 0x400 * B_gain / (Base_gain);
		G_gain = 0x400 * G_gain / (Base_gain);

		// update sensor WB gain
		if (R_gain>0x400)
			{
			Sensor_WriteReg(0x5032, R_gain>>8);
			Sensor_WriteReg(0x5033, R_gain & 0x00ff);
			}
		if(G_gain>0x400)
			{
			Sensor_WriteReg(0x5034, G_gain>>8);
			Sensor_WriteReg(0x5035, G_gain & 0x00ff);
			}
		if(B_gain>0x400)
			{
			Sensor_WriteReg(0x5036, B_gain>>8);
			Sensor_WriteReg(0x5037, B_gain & 0x00ff);
			}
		SENSOR_PRINT("SENSOR_OV5670: current_otp.R_gain = 0x%x \n",R_gain);
		SENSOR_PRINT("SENSOR_OV5670: current_otp.B_gain = 0x%x \n",B_gain);
		SENSOR_PRINT("SENSOR_OV5670: current_otp.G_gain = 0x%x \n",G_gain);
	}

	return 0;


}

LOCAL uint32_t _ov5670_Identify_otp(void* param_ptr)
{

	uint32_t rtn=SENSOR_FAIL;
	uint32_t module_id;

	UNUSED(param_ptr);

	/*read param id from sensor omap*/

	module_id=ov5670_check_otp_module_id();

	SENSOR_PRINT("SENSOR_OV5670: otp_module id = 0x%x \n",module_id);

	if(OV5670_RAW_PARAM_Sunny==module_id)
		{
		SENSOR_PRINT("SENSOR_OV5670: This is Sunny module!!\n");
		otp_typical_value.RG_Ratio_Typical = 0x152;
		otp_typical_value.BG_Ratio_Typical = 0x152;
		rtn=SENSOR_SUCCESS;
		}
	else if(OV5670_RAW_PARAM_Truly==module_id)
		{
		SENSOR_PRINT("SENSOR_OV5670: This is Truly module!!\n");
		otp_typical_value.RG_Ratio_Typical = 0x152;
		otp_typical_value.BG_Ratio_Typical = 0x137;
		rtn=SENSOR_SUCCESS;
		}
	else if(OV5670_RAW_PARAM_Sunrise==module_id)
		{
		SENSOR_PRINT("SENSOR_OV5670: This is Sunrise module!!\n");
		otp_typical_value.RG_Ratio_Typical =0x133;  // 0x85;
		otp_typical_value.BG_Ratio_Typical =0x115;  //0x73;
		rtn=SENSOR_SUCCESS;
		}
	else
		{
		SENSOR_PRINT("SENSOR_OV5670: This is unknow module!!\n");
		rtn=SENSOR_SUCCESS;
		}

	return rtn;

}

