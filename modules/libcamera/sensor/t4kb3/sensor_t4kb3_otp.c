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

#define T4KB3_OTP_CASE 		3
#define T4KB3_UPDATE_LNC	1
#define T4KB3_UPDATE_WB		2
#define T4KB3_UPDATE_LNC_WB	3
#define T4KB3_UPDATE_VCM	4

#define T4KB3_OTP_DATA_ADDR         0x3504
#define T4KB3_OTP_LOAD_ADDR         0x3500
#define T4KB3_OTP_PAGE_ADDR         0x3502

#define T4KB3_GOLDEN_FLAG
/*zouyu If golden module awb info is not written in OTP, please write here*/
#define GOLDEN_MODULE_R 326
#define GOLDEN_MODULE_G 582
#define GOLDEN_MODULE_B 324

static uint8_t pBuf_0[64];
static uint8_t pBuf_1[64];
static uint8_t pBuf_2[64];
static uint8_t pBuf_15[64];

// Enable OTP read function
LOCAL void _T4KB3_otp_read_enable(void)
{
	Sensor_WriteReg(T4KB3_OTP_LOAD_ADDR, 0x01);
	usleep(15*1000); // sleep > 10ms
}

LOCAL void _T4KB3_otp_read_start(void)
{
	Sensor_WriteReg(T4KB3_OTP_LOAD_ADDR, 0x81);
	usleep(15*1000); // sleep > 10ms
}

// Disable OTP read function
LOCAL void _T4KB3_otp_read_disable(void)
{
	Sensor_WriteReg(T4KB3_OTP_LOAD_ADDR, 0x00);
}

LOCAL void _T4KB3_otp_update_awb(void)
{
	uint16_t Golden_AWB_AVG_R = 0, Golden_AWB_AVG_G = 0, Golden_AWB_AVG_B = 0;
	uint16_t AWB_AVG_R = 0, AWB_AVG_G = 0, AWB_AVG_B = 0;
	uint16_t r_gain = 0, b_gain = 0, g_gain = 0;
	int sum=0, i;
	
	#ifndef T4KB3_GOLDEN_FLAG
		Golden_AWB_AVG_R = GOLDEN_MODULE_R;
		Golden_AWB_AVG_G = GOLDEN_MODULE_G;
		Golden_AWB_AVG_B = GOLDEN_MODULE_B;
	#else
		SENSOR_PRINT("cds pBuf_0[1]== %x, pBuf_0[4]== %x,", pBuf_0[1],pBuf_0[4]);
	//zouyu get golden module AWB info
	if ((pBuf_0[1]&0x02)==0x02)
	{  
		// golden 2 check sum
		for(sum=0,i=16;i<31;i++)
			sum+=pBuf_1[i];

		if((sum&0xff)==pBuf_1[31])
		{
			SENSOR_PRINT("cdsGolden AWB 2 information Load\n");
			Golden_AWB_AVG_R = (pBuf_1[25]<<8) + pBuf_1[26];
			Golden_AWB_AVG_G = (pBuf_1[27]<<8) + pBuf_1[28];
			Golden_AWB_AVG_B = (pBuf_1[29]<<8) + pBuf_1[30];
		}
		else
		{
			SENSOR_PRINT("cdsGolden AWB info 2 check sum error!\n");
			return;
		}
	} 
	else if ((pBuf_0[1]&0x01)==0x01)
	{
		// golden 1 check sum
		for(i=0;i<15;i++)
			sum+=pBuf_1[i];

		if((sum&0xff)==pBuf_1[15])
		{
			SENSOR_PRINT("cdsGolden AWB 1 information Load\n");
			Golden_AWB_AVG_R = (pBuf_1[9]<<8) + pBuf_1[10];
			Golden_AWB_AVG_G = (pBuf_1[11]<<8) + pBuf_1[12];
			Golden_AWB_AVG_B = (pBuf_1[13]<<8) + pBuf_1[14];
		}
		else
		{
			SENSOR_PRINT("cdsGolden AWB 1 check sum error!\n");
			return;
		}
	} 
	else
	{
		//zouyu  No Golden module information.
		SENSOR_PRINT("cdsGolden module information is not written.\n");
		return;
	}
	#endif
	
	SENSOR_PRINT("cdsGolden_AWB_AVG_R : 0x%x\n",Golden_AWB_AVG_R);
	SENSOR_PRINT("cdsGolden_AWB_AVG_G : 0x%x\n",Golden_AWB_AVG_G);
	SENSOR_PRINT("cdsGolden_AWB_AVG_B : 0x%x\n",Golden_AWB_AVG_B);

	//zouyu Get AWB infomation
	
	if ((pBuf_0[4]&0x04)==0x04)
	{  

		// AWB 3 check sum
		for(sum=0,i=16;i<22;i++)
			sum+=pBuf_2[i];

		if((sum&0xff)==pBuf_2[22])
		{
			SENSOR_PRINT("cdsGolden AWB 3 information Load\n");
			AWB_AVG_R = (pBuf_2[16]<<8) + pBuf_2[17];
			AWB_AVG_G = (pBuf_2[18]<<8) + pBuf_2[19];
			AWB_AVG_B = (pBuf_2[20]<<8) + pBuf_2[21];
		}
		else
		{
			SENSOR_PRINT("cdsGolden AWB info 3 check sum error!\n");
			return;
		}
	} 
	else if((pBuf_0[4]&0x02)==0x02)
	{  

		// AWB 2 check sum
		for(sum=0,i=8;i<14;i++)
			sum+=pBuf_2[i];

		if((sum&0xff)==pBuf_2[14])
		{
			SENSOR_PRINT("cdsGolden AWB 2 information Load\n");
			AWB_AVG_R = (pBuf_2[8]<<8) + pBuf_2[9];
			AWB_AVG_G = (pBuf_2[10]<<8) + pBuf_2[11];
			AWB_AVG_B = (pBuf_2[12]<<8) + pBuf_2[13];
		}
		else
		{
			SENSOR_PRINT("cdsAWB 2 check sum error!\n");
			return;
		}
	}
	else if ((pBuf_0[4]&0x01)==0x01)
	{
		// AWB 1 check sum
		for(sum=0,i=0;i<6;i++)
			sum+=pBuf_2[i];

		if((sum&0xff)==pBuf_2[6])
		{
			SENSOR_PRINT("cdsGolden AWB 1 information Load\n");
			AWB_AVG_R = (pBuf_2[0]<<8) + pBuf_2[1];
			AWB_AVG_G = (pBuf_2[2]<<8) + pBuf_2[3];
			AWB_AVG_B = (pBuf_2[4]<<8) + pBuf_2[5];
		}
		else
		{
			SENSOR_PRINT("cdsAWB 1 check sum error!\n");
			return;
		}
	} 
	
	else 
	{
		//zouyu  No AWB information.
		SENSOR_PRINT("cdsAWB information is not written.\n");
		return;
	}

	SENSOR_PRINT("cdsAWB_AVG_R : 0x%x\n",AWB_AVG_R);
	SENSOR_PRINT("cdsAWB_AVG_G : 0x%x\n",AWB_AVG_G);
	SENSOR_PRINT("cdsAWB_AVG_B : 0x%x\n",AWB_AVG_B);

	
	//calculate Digital gain
	r_gain=256*(((float)Golden_AWB_AVG_R*AWB_AVG_G)/(AWB_AVG_R*Golden_AWB_AVG_G));
	b_gain=256*(((float)Golden_AWB_AVG_B*AWB_AVG_G)/(AWB_AVG_B*Golden_AWB_AVG_G));
	if(r_gain<b_gain){
		g_gain=((float)256*256/r_gain);}
	else{
		g_gain=((float)256*256/b_gain);}
	if(g_gain<256)
		g_gain=256;


	SENSOR_PRINT("cds r_gain : 0x%x\n",r_gain);
	SENSOR_PRINT("cds g_gain : 0x%x\n",g_gain);
	SENSOR_PRINT("cds b_gain : 0x%x\n",b_gain);


	Sensor_WriteReg(0x020e, g_gain >>8);
	Sensor_WriteReg(0x020f, g_gain & 0xFF);	
	Sensor_WriteReg(0x0210, r_gain >>8);
	Sensor_WriteReg(0x0211, r_gain & 0xFF);
	Sensor_WriteReg(0x0212, b_gain >>8);
	Sensor_WriteReg(0x0213, b_gain & 0xFF);
	Sensor_WriteReg(0x0214, g_gain >>8);
	Sensor_WriteReg(0x0215, g_gain & 0xFF);	
}

void _T4KB3_otp_update_lsc(void)
{
	int sum1=0,sum2=0;
	int i;
	//zouyu start load LSC data
	SENSOR_PRINT("cds pBuf_0[0]== %x,", pBuf_0[0]);
	if((pBuf_0[0]&0x02)==0x02)
	{
		SENSOR_PRINT("cdsLSC#2 is used.\n");
		Sensor_WriteReg(0x3551, 0x85);
		return;
	}
	else if((pBuf_0[0]&0x01)==0x01)
	{
		SENSOR_PRINT("cdsLSC#1 is used.\n");
		Sensor_WriteReg(0x3551, 0x80);
		return;
	}
	else
	{
		SENSOR_PRINT("cdsNo LSC information\n");
		return;
	}
}

LOCAL void _T4KB3_otp_read_awb(void)
{
	uint8_t page;
	uint8_t i;
	SENSOR_PRINT("cdsotp read awb start\n");

	_T4KB3_otp_read_enable();
	Sensor_WriteReg(T4KB3_OTP_PAGE_ADDR, 1);
	for(i=0;i<32;i++)
	{
		pBuf_1[i] = Sensor_ReadReg(T4KB3_OTP_DATA_ADDR+i) | Sensor_ReadReg(T4KB3_OTP_DATA_ADDR + 32 + i);
	}
	Sensor_WriteReg(T4KB3_OTP_PAGE_ADDR, 2);
	for(i=0;i<32;i++)
	{
		pBuf_2[i] = Sensor_ReadReg(T4KB3_OTP_DATA_ADDR+i) | Sensor_ReadReg(T4KB3_OTP_DATA_ADDR + 32 + i);
	}
	_T4KB3_otp_read_disable();

	SENSOR_PRINT("cdsotp read all finish\n");
	return 0;
}

LOCAL void _T4KB3_otp_read_lsc(void)
{
	uint8_t page;
	uint8_t i;
	SENSOR_PRINT("cdsotp read lsc start\n");

	SENSOR_PRINT("cdsotp read all finish\n");
	return 0;
}

LOCAL uint32_t _T4KB3_otp_read_all(void)
{
	uint8_t page;
	uint8_t i;
	SENSOR_PRINT("cdsotp read all start\n");

	_T4KB3_otp_read_enable();
	Sensor_WriteReg(T4KB3_OTP_PAGE_ADDR, 0);
	_T4KB3_otp_read_start();
	for(i=0;i<32;i++)
	{
		pBuf_0[i] = Sensor_ReadReg(T4KB3_OTP_DATA_ADDR+i) | Sensor_ReadReg(T4KB3_OTP_DATA_ADDR + 32 + i);
	}
	Sensor_WriteReg(T4KB3_OTP_PAGE_ADDR, 1);
	_T4KB3_otp_read_start();
	for(i=0;i<32;i++)
	{
		pBuf_1[i] = Sensor_ReadReg(T4KB3_OTP_DATA_ADDR+i) | Sensor_ReadReg(T4KB3_OTP_DATA_ADDR + 32 + i);
	}
	Sensor_WriteReg(T4KB3_OTP_PAGE_ADDR, 2);
	_T4KB3_otp_read_start();
	for(i=0;i<32;i++)
	{
		pBuf_2[i] = Sensor_ReadReg(T4KB3_OTP_DATA_ADDR+i) | Sensor_ReadReg(T4KB3_OTP_DATA_ADDR + 32 + i);
	}
	Sensor_WriteReg(T4KB3_OTP_PAGE_ADDR, 15);
	_T4KB3_otp_read_start();
	for(i=0;i<64;i++)
	{
		pBuf_15[i] = Sensor_ReadReg(T4KB3_OTP_DATA_ADDR+i);
		SENSOR_PRINT("cdsotp SENSOR_T4KB3: OTP = %x", pBuf_15[i]);
	}
	_T4KB3_otp_read_disable();

	SENSOR_PRINT("cdsotp read all finish\n");
	return 0;
}

LOCAL int8_t _T4KB3_otp_read_flag(int index)
{
	uint8_t page;
	uint8_t i;

	SENSOR_PRINT("cdsotp read flag start\n");

	_T4KB3_otp_read_enable();
	Sensor_WriteReg(T4KB3_OTP_PAGE_ADDR, 0);
	for(i=0;i<5;i++)
	{
		pBuf_0[i] = Sensor_ReadReg(T4KB3_OTP_DATA_ADDR+i) | Sensor_ReadReg(T4KB3_OTP_DATA_ADDR + 32 + i);
	}
	Sensor_WriteReg(T4KB3_OTP_PAGE_ADDR, 1);
	i=0x02;
	{
		pBuf_1[i] = Sensor_ReadReg(T4KB3_OTP_DATA_ADDR+i) | Sensor_ReadReg(T4KB3_OTP_DATA_ADDR + 32 + i);
	}
	_T4KB3_otp_read_disable();

	SENSOR_PRINT("cdsotp read flag finish\n");
	return 0;
}

LOCAL void _T4KB3_otp_check_lsc()
{
	SENSOR_PRINT("cdsotp check lsc start\n");

	SENSOR_PRINT("cdsotp check lsc finish\n");
	return 0;
}

LOCAL void _T4KB3_otp_check_awb()
{
	SENSOR_PRINT("cdsotp check lsc start\n");

	if (pBuf_0[4] != 0)
	{
		_T4KB3_otp_read_awb();
	}

	SENSOR_PRINT("cdsotp check lsc finish\n");
	return 0;
}


LOCAL int _T4KB3_Identify_otp()
{
	uint32_t rtn=SENSOR_FAIL;
	uint16_t otp_addr;
	uint8_t mid;
	uint8_t index;
	int i = 0;
	
	SENSOR_PRINT("cdsSENSOR_T4KB3: _T4KB3_Identify_otp");

#if 0	
	_T4KB3_otp_read_flag();

	_T4KB3_otp_check_awb();
	_T4KB3_otp_check_lsc();
#else
	_T4KB3_otp_read_all();
#endif

	return 0;
}

LOCAL uint32_t  _T4KB3_update_otp(void* param_ptr)
{
	_T4KB3_otp_update_awb();
	_T4KB3_otp_update_lsc();

	return 0;

}

LOCAL uint32_t _t4kb3_truly_identify_otp(void *param_ptr)
{
	uint32_t temp = 0;
	uint16_t stream_value = 0;
	uint32_t rtn = SENSOR_FAIL;
#if 0
	stream_value = Sensor_ReadReg(0x0100);
	SENSOR_PRINT("stream_value = 0x%x\n", stream_value);
	if (1 != (stream_value & 0x01)) {
		Sensor_WriteReg(0x0100, 0x01);
		usleep(50 * 1000);
	}
	// R/G and B/G of current camera module is read out from sensor OTP
	// check first OTP with valid data
	temp = t4kb3_read_otp_info(&current_otp); 
	if (T4KB3_RAW_PARAM_COM == current_otp.module_integrator_id) {
		rtn = SENSOR_SUCCESS;
		SENSOR_PRINT("This is T4KB3 Global Module ! index = %d\n", current_otp.index);
	} else {
		SENSOR_PRINT("t4kb3_check_otp_module_id no valid wb OTP data\n");
	}


	SENSOR_PRINT("cdsSENSOR_T4KB3: _T4KB3_Identify_otp");
	

	if (1 != (stream_value & 0x01))
		Sensor_WriteReg(0x0100, stream_value);

	//SENSOR_PRINT("read t4kb3 otp  module_id = %x \n", current_otp.module_integrator_id);
#else
	rtn = _T4KB3_otp_read_all();

#endif

	return rtn;
}

LOCAL uint32_t _t4kb3_update_otp(void *param_ptr)
{
	uint16_t stream_value = 0;
	uint32_t rtn = SENSOR_FAIL;
/*
	stream_value = Sensor_ReadReg(0x0100);
	SENSOR_PRINT("stream_value = 0x%x  T4KB3_OTP_CASE = %d\n", stream_value, T4KB3_OTP_CASE);
	if (1 != (stream_value & 0x01)) {
		Sensor_WriteReg(0x0100, 0x01);
		usleep(50 * 1000);
	}
*/
	switch (T4KB3_OTP_CASE) {
	case T4KB3_UPDATE_LNC:
		//rtn = t4kb3_update_otp_lenc();
		_T4KB3_otp_update_lsc();
		break;
	case T4KB3_UPDATE_WB:
		//rtn = t4kb3_update_otp_wb();
		_T4KB3_otp_update_awb();
		break;

	case T4KB3_UPDATE_LNC_WB:
		_T4KB3_otp_update_lsc();
		_T4KB3_otp_update_awb();
		break;

	case T4KB3_UPDATE_VCM:

		break;
	default:
		break;
	}
/*
	if (1 != (stream_value & 0x01))
		Sensor_WriteReg(0x0100, stream_value);
	*/

	return 0;//rtn;
}
