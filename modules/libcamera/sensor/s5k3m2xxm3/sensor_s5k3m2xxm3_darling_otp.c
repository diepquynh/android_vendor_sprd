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

struct otp_typical_value_t {
	uint16_t rg_ratio_typical;
	uint16_t bg_ratio_typical;
	uint16_t gbgr_ratio_typical;
};

struct otp_param_info_tab {
	uint32_t module_id;
	struct otp_typical_value_t typical_value;
	struct sensor_raw_info *info_ptr;
	uint32_t(*update_otp) (void *param_ptr);
};

struct otp_info_t {
	int flag;
	int module_id;
	int lens_id;
	int year;
	int month;
	int day;
	int rg_ratio;
	int bg_ratio;
	int gbgr_ratio;
};

struct otp_info_t otp_info={0x00};
struct otp_typical_value_t otp_typical_value={0x00};
//#define TEST_OTP
static uint32_t s5k3m2xxm3_darling_otp_read_otp_info(struct otp_info_t *otp_ptr)
{
	uint32_t rg_msb, bg_msb, rg_msb_golden,bg_msb_golden,wb_lsb,awb_flag;
	uint32_t PageCount=0x1f;
	uint32_t rtn = SENSOR_FAIL;
	uint32_t streamflag=0;
	SENSOR_PRINT("PageCount=%d\n", PageCount);	//0

	//streamflag=Sensor_ReadReg(0x0100); //stream on
	//Sensor_WriteReg(0x0100, (streamflag|(1<<8))); //stream on
	Sensor_WriteReg(0x0100, 0x100); //stream on
	usleep(10 * 1000);

	SENSOR_PRINT("windsorDBG: s5k3m2xxm3_darling_otp_read_otp_info");
	/*
	Sensor_WriteReg(0x6028, 0x4000);
	Sensor_WriteReg(0x0136, 0x1800);
	Sensor_WriteReg(0x0304, 0x0006);
	Sensor_WriteReg(0x0306, 0x0073);
	Sensor_WriteReg(0x030c, 0x0004);

	Sensor_WriteReg(0x030e, 0x0064);
	Sensor_WriteReg(0x0302, 0x0001);
	Sensor_WriteReg(0x030a, 0x0001);
	Sensor_WriteReg(0x0300, 0x0004);

	Sensor_WriteReg(0x0308, 0x0008);
	usleep(10 * 1000);
	*/
#ifndef TEST_OTP
	Sensor_WriteReg(0x0A02, 0x1f00); //page set
	Sensor_WriteReg(0x0A00, 0x0100); //otp enable read
	usleep(10 * 1000);
#else
	//Sensor_WriteReg(0x0A02, 0x200); //page set
	Sensor_WriteReg(0x0A02, 0x0000); //page set
	Sensor_WriteReg(0x0A00, 0x0100); //otp enable read
	usleep(10 * 1000);

	awb_flag=Sensor_ReadReg(0x0A24);
	SENSOR_PRINT("0x0A24=0x%x",awb_flag);
	awb_flag=Sensor_ReadReg(0x0A26);
	SENSOR_PRINT("0x0A26=0x%x",awb_flag);
	awb_flag=Sensor_ReadReg(0x0A28);
	SENSOR_PRINT("0x0A28=0x%x",awb_flag);

	awb_flag=Sensor_ReadReg(0x0000);
	SENSOR_PRINT("i2c address=0x%x",awb_flag);
#endif

	awb_flag=Sensor_ReadReg(0x0A03);
	otp_ptr->flag=(awb_flag & 0xff);
	SENSOR_PRINT("awb_flag=0x%x",awb_flag);

	//Sensor_WriteReg(0x3a00, 0x00); //otp disable read
	if((awb_flag) == 0xd0){//bit[5:4]=1
		SENSOR_PRINT("otp is in grooup 2");

		otp_ptr->module_id=(Sensor_ReadReg(0x0A0A) & 0xff);
		otp_ptr->lens_id=(Sensor_ReadReg(0x0A0B) & 0xff);
		uint32_t val = (Sensor_ReadReg(0x0A0e) & 0xff);
		otp_ptr->rg_ratio = ((Sensor_ReadReg(0x0A0c) & 0xff)<<2)+((val&0xc0)>>6);
		otp_ptr->bg_ratio = ((Sensor_ReadReg(0x0A0d)& 0xff)<<2)+((val&0x30)>>4);
		otp_ptr->gbgr_ratio = ((Sensor_ReadReg(0x0A0e)& 0xff)<<2)+((val&0x0c)>>2);

		rtn=SENSOR_SUCCESS;

	}
	else if((awb_flag) == 0x40){//bit[7:8]=1
		SENSOR_PRINT("otp is in group 1");

		//Sensor_WriteReg(0x3a02, PageCount); //page set
		//Sensor_WriteReg(0x3a00, 0x01); //otp enable read

		//usleep(2 * 1000);
		otp_ptr->module_id=(Sensor_ReadReg(0x0A04)& 0xff);
		otp_ptr->lens_id=(Sensor_ReadReg(0x0A05)& 0xff);
		uint32_t val = (Sensor_ReadReg(0x0A09)& 0xff);
		otp_ptr->rg_ratio = ((Sensor_ReadReg(0x0A06)& 0xff)<<2)+((val&0xc0)>>6);
		otp_ptr->bg_ratio = ((Sensor_ReadReg(0x0A07)& 0xff)<<2)+((val&0x30)>>4);
		otp_ptr->gbgr_ratio = ((Sensor_ReadReg(0x0A08)& 0xff)<<2)+((val&0x0c)>>2);
		rtn=SENSOR_SUCCESS;
	}
	else if(awb_flag == 0x00 ){
		SENSOR_PRINT("otp all value is zero");
	}
	else	{
		SENSOR_PRINT("otp all value is wrong");
	}



	SENSOR_PRINT("module_id=0x%x",otp_ptr->module_id);
	SENSOR_PRINT("lens_id=0x%x",otp_ptr->lens_id);

	Sensor_WriteReg(0x0A00,0x00);

	usleep(10 * 1000);
	//Sensor_WriteReg(0x0100, (streamflag&(~(1<<8)))); //stream off
	Sensor_WriteReg(0x0100, 0x000); //stream on
	/*
	SENSOR_PRINT("year=0x%x\n", otp_ptr->year);
	SENSOR_PRINT("month=0x%x\n",  otp_ptr->month);
	SENSOR_PRINT("day=0x%x\n", otp_ptr->day);
	*/
	otp_typical_value.rg_ratio_typical=274;//256;
	otp_typical_value.bg_ratio_typical=253;//256;
	otp_typical_value.gbgr_ratio_typical=512;

	SENSOR_PRINT("rg_ratio=0x%x\n", otp_ptr->rg_ratio);
	SENSOR_PRINT("bg_ratio=0x%x\n",  otp_ptr->bg_ratio);
	SENSOR_PRINT("gbgr_ratio=0x%x\n",  otp_ptr->gbgr_ratio);
	SENSOR_PRINT("rg_ratio_typical=0x%x\n",  otp_typical_value.rg_ratio_typical);
	SENSOR_PRINT("bg_ratio_typical=0x%x\n",  otp_typical_value.bg_ratio_typical);
	SENSOR_PRINT("gbgr_ratio_typical=0x%x\n",  otp_typical_value.gbgr_ratio_typical);

	return rtn;
}

static uint32_t s5k3m2xxm3_darling_otp_update_otp(void *param_ptr)
{
	uint32_t rtn = SENSOR_FAIL;
	//if without otp
	SENSOR_PRINT("otp_info.flag=0x%x\n", otp_info.flag);
	if(((otp_info.flag)&0xF0)==0x00)		return rtn;
	rtn=SENSOR_SUCCESS;
	uint16_t stream_value = 0;
	uint32_t g_gain=0,r_gain=0,b_gain = 0,g_gain_b ,g_gain_r,base_gain;

	struct otp_param_info_tab *otp_param_info = (struct otp_param_info_tab *)param_ptr;
	if(otp_typical_value.bg_ratio_typical==0 && otp_typical_value.rg_ratio_typical ==0)
	{
		otp_typical_value.bg_ratio_typical=otp_param_info->typical_value.bg_ratio_typical;
		otp_typical_value.rg_ratio_typical=otp_param_info->typical_value.rg_ratio_typical;
	}
	r_gain = (otp_typical_value.rg_ratio_typical*1000)/otp_info.rg_ratio;
	b_gain = (otp_typical_value.bg_ratio_typical*1000)/otp_info.bg_ratio;
	g_gain_b = (otp_typical_value.gbgr_ratio_typical*1000)/otp_info.gbgr_ratio;
	g_gain_r = 1000;
	base_gain = r_gain;
	if(base_gain>b_gain) base_gain=b_gain;
	if(base_gain>g_gain_b) base_gain=g_gain_b;
	if(base_gain>g_gain_r) base_gain=g_gain_r;
	r_gain = 0x100 * r_gain /base_gain;
	b_gain = 0x100 * b_gain /base_gain;
	g_gain_b = 0x100 * g_gain_b /base_gain;
	g_gain_r = 0x100 * g_gain_r /base_gain;
	if(g_gain_r>0x100)
		{
			/*
		     rtn = Sensor_WriteReg(0x020e,g_gain_r>>8);
             rtn = Sensor_WriteReg(0x020f,g_gain_r&0xff);
             */
		rtn = Sensor_WriteReg(0x020e,g_gain_r);

		}
	if(r_gain>0x100)
		{
			/*
		      rtn = Sensor_WriteReg(0x0210,r_gain>>8);
              rtn = Sensor_WriteReg(0x0211,r_gain&0xff);
              */
		 rtn = Sensor_WriteReg(0x0210,r_gain);

		}
	if(b_gain>0x100)
		{
			/*
		      rtn = Sensor_WriteReg(0x0212,b_gain>>8);
              rtn =  Sensor_WriteReg(0x0213,b_gain&0xff);
              */
		 rtn = Sensor_WriteReg(0x0212,b_gain);
		}
	if(g_gain_b>0x100)
		{
		/*
		      rtn = Sensor_WriteReg(0x0214,g_gain_b>>8);
              rtn =Sensor_WriteReg(0x0215,g_gain_b&0xff);
              */
              rtn = Sensor_WriteReg(0x0214,g_gain_b);
		}



	Sensor_WriteReg(0x6028, 0x4000);
	Sensor_WriteReg(0x6028, 0x3056);
	Sensor_WriteReg(0x6f12, 0x0100);

	//write to register
	SENSOR_PRINT("r_Gain=0x%x\n", r_gain);
	SENSOR_PRINT("g_Gain=0x%x\n", g_gain);
	SENSOR_PRINT("g_gain_b=0x%x\n", g_gain_b);
	SENSOR_PRINT("g_gain_r=0x%x\n", g_gain_r);
	return rtn;
}

static uint32_t s5k3m2xxm3_darling_otp_get_module_id(void)
{
	int i = 0;
	int otp_index = 0;
	int temp = 0;

	// R/G and B/G of current camera module is read out from sensor OTP
	// check first OTP with valid data
	s5k3m2xxm3_darling_otp_read_otp_info(&otp_info);

	SENSOR_PRINT("read s5k3m2xxm3_darling otp  module_id = %x \n", otp_info.module_id);

	return otp_info.module_id;
}

static uint32_t s5k3m2xxm3_darling_otp_identify_otp(void *param_ptr)
{
	uint32_t module_id;
	struct otp_param_info_tab *tab_ptr = (struct otp_param_info_tab *)param_ptr;

	SENSOR_PRINT("SENSOR_s5k3m2xxm3_darling: _s5k3m2xxm3_darling_com_Identify_otp");

	/*read param id from sensor omap */
	module_id = s5k3m2xxm3_darling_otp_get_module_id();

	return module_id;
}
