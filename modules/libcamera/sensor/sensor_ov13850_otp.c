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
#define DEBUG_OTP_STR     "OV13850_OTP_debug: L %d, %s: "
#define DEBUG_OTP_ARGS    __LINE__,__FUNCTION__

#define OTP_PRINT(format,...) ALOGE(DEBUG_OTP_STR format, DEBUG_OTP_ARGS, ##__VA_ARGS__)
#else
#define OTP_PRINT
#endif

static uint16_t OV13850_RG_Ratio_Typical = 0x100;
static uint16_t OV13850_BG_Ratio_Typical = 0x100;

#define OTP_DRV_START_ADDR    			0x7220
#define OTP_DRV_INFO_GROUP_COUNT		3
#define OTP_DRV_INFO_SIZE				5
#define OTP_DRV_AWB_GROUP_COUNT		3
#define OTP_DRV_AWB_SIZE				5
#define OTP_DRV_VCM_GROUP_COUNT		3
#define OTP_DRV_VCM_SIZE				3
#define OTP_DRV_LSC_GROUP_COUNT		3
#define OTP_DRV_LSC_SIZE				62
#define OTP_DRV_LSC_REG_ADDR			0x5200

struct otp_struct {
	uint8_t module_integrator_id;
	uint8_t lens_id;
	uint8_t production_year;
	uint8_t production_month;
	uint8_t production_day;
	uint8_t lenc[OTP_DRV_LSC_SIZE ];
	uint8_t VCM_dir;
	uint16_t VCM_start;
	uint16_t VCM_end;
	uint16_t rg_ratio;
	uint16_t bg_ratio;
	uint16_t light_rg;
	uint16_t light_bg;
	uint32_t index;
};

// index: index of otp group. (1, 2, ... , OTP_DRV_INFO_GROUP_COUNT)
// return:0, group index is empty
// 1, group index has invalid data
// 2, group index has valid data
LOCAL uint32_t ov13850_check_otp_info(uint32_t index)
{
	uint32_t flag = 0;
	uint32_t nFlagAddress = OTP_DRV_START_ADDR;

	OTP_PRINT("index = %d\n", index);

	Sensor_WriteReg(0x3d84, 0xC0);
	//partial mode OTP write start address
	Sensor_WriteReg(0x3d88, (nFlagAddress>>8) & 0xff);
	Sensor_WriteReg(0x3d89, nFlagAddress & 0xff);

	// partial mode OTP write end address
	Sensor_WriteReg(0x3d8A, (nFlagAddress>>8) & 0xff );
	Sensor_WriteReg(0x3d8B, nFlagAddress & 0xff );
	// read otp uint32_to buffer
	Sensor_WriteReg(0x3d81, 0x01);
	usleep(5 * 1000);
	flag = Sensor_ReadReg(nFlagAddress);
	//select group
	if (index == 1)
	{
		flag = (flag>>6) & 0x03;
	}
	else if (index == 2)
	{
		flag = (flag>>4) & 0x03;
	}
	else if (index ==3)
	{
		flag = (flag>>2) & 0x03;
	}
	// clear otp buffer
	Sensor_WriteReg(nFlagAddress, 0x00);
	if (flag == 0x00) {
		return 0;
	}
	else if (flag & 0x02) {
		return 1;
	}
	else {
		return 2;
	}
}

// index: index of otp group. (1, 2,...,OTP_DRV_AWB_GROUP_COUNT)
// return:0, group index is empty
// 1, group index has invalid data
// 2, group index has valid data
LOCAL uint32_t ov13850_check_otp_wb(uint32_t index)
{
	uint32_t flag = 0;
	uint32_t nFlagAddress = OTP_DRV_START_ADDR+1+OTP_DRV_INFO_GROUP_COUNT*OTP_DRV_INFO_SIZE;
	Sensor_WriteReg(0x3d84, 0xC0);
	//partial mode OTP write start address
	Sensor_WriteReg(0x3d88, (nFlagAddress>>8) & 0xff );
	Sensor_WriteReg(0x3d89, nFlagAddress & 0xff);
	// partial mode OTP write end address
	Sensor_WriteReg(0x3d8A, (nFlagAddress>>8) & 0xff );

	Sensor_WriteReg(0x3d8B, nFlagAddress & 0xff);
	// read otp uint32_to buffer
	Sensor_WriteReg(0x3d81, 0x01);
	usleep(5 * 1000);
	//select group
	flag = Sensor_ReadReg(nFlagAddress);
	if (index == 1)
	{
		flag = (flag>>6) & 0x03;
	}
	else if (index == 2)
	{
		flag = (flag>>4) & 0x03;
	}
	else if (index == 3)
	{
		flag = (flag>>2) & 0x03;
	}
	// clear otp buffer
	Sensor_WriteReg(nFlagAddress, 0x00);
	if (flag == 0x00) {
		return 0;
	}
	else if (flag & 0x02) {
		return 1;
	}
	else {
		return 2;
	}
}

// index: index of otp group. (1, 2,...,OTP_DRV_VCM_GROUP_COUNT)
// code: 0 for start code, 1 for stop code
// return:0, group index is empty
// 1, group index has invalid data
// 2, group index has valid data
LOCAL uint32_t ov13850_check_otp_VCM(uint32_t index, uint32_t code)
{
	uint32_t flag = 0;
	uint32_t nFlagAddress= OTP_DRV_START_ADDR+1+OTP_DRV_INFO_GROUP_COUNT*OTP_DRV_INFO_SIZE +1+OTP_DRV_AWB_GROUP_COUNT*OTP_DRV_AWB_SIZE;
	Sensor_WriteReg(0x3d84, 0xC0);
	//partial mode OTP write start address
	Sensor_WriteReg(0x3d88, (nFlagAddress>>8) & 0xff );
	Sensor_WriteReg(0x3d89, nFlagAddress & 0xff);
	// partial mode OTP write end address

	Sensor_WriteReg(0x3d8A, (nFlagAddress>>8) & 0xff );
	Sensor_WriteReg(0x3d8B, nFlagAddress & 0xff);
	// read otp uint32_to buffer
	Sensor_WriteReg(0x3d81, 0x01);
	usleep(5 * 1000);
	//select group
	flag = Sensor_ReadReg(nFlagAddress);
	if (index == 1)
	{
		flag = (flag>>6) & 0x03;
	}
	else if (index == 2)
	{
		flag = (flag>>4) & 0x03;
	}
	else if (index == 3)
	{
		flag = (flag>>2) & 0x03;
	}
	// clear otp buffer
	Sensor_WriteReg(nFlagAddress, 0x00);
	if (flag == 0x00) {
		return 0;
	}
	else if (flag & 0x02) {
		return 1;
	}
	else {
		return 2;
	}
}

// index: index of otp group. (1, 2,...,OTP_DRV_LSC_GROUP_COUNT)
// return:0, group index is empty
// 1, group index has invalid data
// 2, group index has valid data
LOCAL uint32_t ov13850_check_otp_lenc(uint32_t index)
{
	uint32_t flag = 0;
	uint32_t nFlagAddress = OTP_DRV_START_ADDR+1+OTP_DRV_INFO_GROUP_COUNT*OTP_DRV_INFO_SIZE+1+OTP_DRV_AWB_GROUP_COUNT*OTP_DRV_AWB_SIZE+1+OTP_DRV_VCM_GROUP_COUNT*OTP_DRV_VCM_SIZE ;
	Sensor_WriteReg(0x3d84, 0xC0);
	//partial mode OTP write start address
	Sensor_WriteReg(0x3d88, (nFlagAddress>>8) & 0xff );
	Sensor_WriteReg(0x3d89, nFlagAddress & 0xff);

	// partial mode OTP write end address
	Sensor_WriteReg(0x3d8A, (nFlagAddress>>8) & 0xff );
	Sensor_WriteReg(0x3d8B, nFlagAddress & 0xff);
	// read otp uint32_to buffer
	Sensor_WriteReg(0x3d81, 0x01);
	usleep(5 * 1000);
	flag = Sensor_ReadReg(nFlagAddress);
	if (index == 1)
	{
		flag = (flag>>6) & 0x03;
	}
	else if (index == 2)
	{
		flag = (flag>>4) & 0x03;
	}
	else if (index == 3)
	{
		flag = (flag>> 2)& 0x03;
	}
	// clear otp buffer
	Sensor_WriteReg(nFlagAddress, 0x00);
	if (flag == 0x00) {
		return 0;
	}
	else if (flag & 0x02) {
		return 1;
	}
	else {
		return 2;
	}
}

// index: index of otp group. (1, 2,...,OTP_DRV_INFO_GROUP_COUNT)
// otp_ptr: pouint32_ter of otp_struct
// return: 0,
LOCAL uint32_t ov13850_read_otp_info(uint32_t index, struct otp_struct *otp_ptr)
{
	uint32_t i = 0;
	uint16_t nFlagAddress = OTP_DRV_START_ADDR;
	uint16_t start_addr = 0;
	uint16_t end_addr = 0;
	start_addr = nFlagAddress+1+(index-1)*OTP_DRV_INFO_SIZE;
	end_addr = start_addr+OTP_DRV_INFO_SIZE - 1;
	Sensor_WriteReg(0x3d84, 0xC0);
	//partial mode OTP write start address
	Sensor_WriteReg(0x3d88, (start_addr >> 8) & 0xff);
	Sensor_WriteReg(0x3d89, start_addr & 0xff);


	// partial mode OTP write end address
	Sensor_WriteReg(0x3d8A, (end_addr >> 8) & 0xff);
	Sensor_WriteReg(0x3d8B, end_addr & 0xff);
	// read otp uint32_to buffer
	Sensor_WriteReg(0x3d81, 0x01);
	usleep(5 * 1000);
	(*otp_ptr).module_integrator_id = Sensor_ReadReg(start_addr);
	(*otp_ptr).lens_id = Sensor_ReadReg(start_addr + 1);
	(*otp_ptr).production_year = Sensor_ReadReg(start_addr + 2);
	(*otp_ptr).production_month = Sensor_ReadReg(start_addr + 3);
	(*otp_ptr).production_day = Sensor_ReadReg(start_addr + 4);

	OTP_PRINT("module_integrator_id = %d\n", (*otp_ptr).module_integrator_id );
	OTP_PRINT("lens_id = %d\n", (*otp_ptr).lens_id );
	OTP_PRINT("production_year = %d\n", (*otp_ptr).production_year );
	OTP_PRINT("production_month = %d\n", (*otp_ptr).production_month );
	OTP_PRINT("production_day = %d\n", (*otp_ptr).production_day );

	// clear otp buffer
	for (i=start_addr; i<=end_addr; i++) {
		Sensor_WriteReg(i, 0x00);
	}
	return 0;
}

// index: index of otp group. (1, 2,...,OTP_DRV_AWB_GROUP_COUNT)
// otp_ptr: pouint32_ter of otp_struct
// return:0,
LOCAL uint32_t ov13850_read_otp_wb(uint32_t index, struct otp_struct *otp_ptr)
{
	uint32_t i = 0;
	uint32_t temp = 0;
	uint16_t start_addr = 0;
	uint16_t end_addr = 0;
	uint16_t nFlagAddress = OTP_DRV_START_ADDR + 1 + OTP_DRV_INFO_GROUP_COUNT * OTP_DRV_INFO_SIZE;
	start_addr = nFlagAddress + 1 + (index - 1) * OTP_DRV_AWB_SIZE;
	end_addr = start_addr + OTP_DRV_AWB_SIZE - 1;
	Sensor_WriteReg(0x3d84, 0xC0);
	//partial mode OTP write start address
	Sensor_WriteReg(0x3d88, (start_addr >> 8) & 0xff);
	Sensor_WriteReg(0x3d89, start_addr & 0xff);
	// partial mode OTP write end address
	Sensor_WriteReg(0x3d8A, (end_addr >> 8) & 0xff);
	Sensor_WriteReg(0x3d8B, end_addr & 0xff);
	// read otp uint32_to buffer
	Sensor_WriteReg(0x3d81, 0x01);
	usleep(5);
	temp = Sensor_ReadReg(start_addr + 4);
	(*otp_ptr).rg_ratio = (Sensor_ReadReg(start_addr)<<2) + ((temp>>6) & 0x03);
	(*otp_ptr).bg_ratio = (Sensor_ReadReg(start_addr + 1)<<2) + ((temp>>4) & 0x03);
	(*otp_ptr).light_rg = (Sensor_ReadReg(start_addr + 2) <<2) + ((temp>>2) & 0x03);
	(*otp_ptr).light_bg = (Sensor_ReadReg(start_addr + 3)<<2) + (temp & 0x03);

	OTP_PRINT("rg_ratio = %d\n", (*otp_ptr).rg_ratio );
	OTP_PRINT("bg_ratio = %d\n", (*otp_ptr).bg_ratio );
	OTP_PRINT("light_rg = %d\n", (*otp_ptr).light_rg );
	OTP_PRINT("light_bg = %d\n", (*otp_ptr).light_bg );

	// clear otp buffer
	for (i=start_addr; i<=end_addr; i++) {
		Sensor_WriteReg(i, 0x00);
	}
	return 0;
}

// index: index of otp group. (1, 2,...,OTP_DRV_VCM_GROUP_COUNT)
// code: 0 start code, 1 stop code
// return:0
LOCAL uint32_t ov13850_read_otp_VCM(uint32_t index, struct otp_struct * otp_ptr)
{
	uint32_t i = 0;
	uint32_t temp = 0;
	uint16_t start_addr = 0;
	uint16_t end_addr = 0;
	uint16_t nFlagAddress = OTP_DRV_START_ADDR + 1 + OTP_DRV_INFO_GROUP_COUNT * OTP_DRV_INFO_SIZE + 1 + OTP_DRV_AWB_GROUP_COUNT * OTP_DRV_AWB_SIZE;
	start_addr = nFlagAddress + 1 + (index - 1) * OTP_DRV_VCM_SIZE;
	end_addr = start_addr + OTP_DRV_VCM_SIZE - 1;
	Sensor_WriteReg(0x3d84, 0xC0);
	//partial mode OTP write start address
	Sensor_WriteReg(0x3d88, (start_addr >> 8) & 0xff);
	Sensor_WriteReg(0x3d89, start_addr & 0xff);
	// partial mode OTP write end address
	Sensor_WriteReg(0x3d8A, (end_addr >> 8) & 0xff);
	Sensor_WriteReg(0x3d8B, end_addr & 0xff);
	// read otp uint32_to buffer
	Sensor_WriteReg(0x3d81, 0x01);
	usleep(5 * 1000);
	//flag and lsb of VCM start code
	temp = Sensor_ReadReg(start_addr+2);
	(* otp_ptr).VCM_start = (Sensor_ReadReg(start_addr)<<2) | ((temp>>6) & 0x03);
	(* otp_ptr).VCM_end = (Sensor_ReadReg(start_addr + 1) << 2) | ((temp>>4) & 0x03);
	(* otp_ptr).VCM_dir = (temp>>2) & 0x03;

	OTP_PRINT("VCM_start = %d\n", (*otp_ptr).VCM_start );
	OTP_PRINT("VCM_end = %d\n", (*otp_ptr).VCM_end );
	OTP_PRINT("VCM_dir = %d\n", (*otp_ptr).VCM_dir );

	// clear otp buffer
	for (i=start_addr; i<=end_addr; i++) {
		Sensor_WriteReg(i, 0x00);
	}
	return 0;
}

// index: index of otp group. (1, 2,...,OTP_DRV_LSC_GROUP_COUNT)
// otp_ptr: pouint32_ter of otp_struct
// return:0,
LOCAL uint32_t ov13850_read_otp_lenc(uint32_t index, struct otp_struct *otp_ptr)
{
	uint32_t i = 0;
	uint32_t start_addr = 0;
	uint32_t end_addr = 0;
	uint32_t nFlagAddress= OTP_DRV_START_ADDR + 1 + OTP_DRV_INFO_GROUP_COUNT * OTP_DRV_INFO_SIZE + 1 + OTP_DRV_AWB_GROUP_COUNT * OTP_DRV_AWB_SIZE + 1 + OTP_DRV_VCM_GROUP_COUNT * OTP_DRV_VCM_SIZE ;

	start_addr = nFlagAddress + 1 + (index - 1) * OTP_DRV_LSC_SIZE ;
	end_addr = start_addr + OTP_DRV_LSC_SIZE - 1;

	Sensor_WriteReg(0x3d84, 0xC0);
	//partial mode OTP write start address
	Sensor_WriteReg(0x3d88, (start_addr >> 8) & 0xff);
	Sensor_WriteReg(0x3d89, start_addr & 0xff);
	// partial mode OTP write end address
	Sensor_WriteReg(0x3d8A, (end_addr >> 8) & 0xff);
	Sensor_WriteReg(0x3d8B, end_addr & 0xff);
	// read otp uint32_to buffer
	Sensor_WriteReg(0x3d81, 0x01);
	usleep(10 * 1000);

	for(i=0; i<OTP_DRV_LSC_SIZE; i++) {
		(* otp_ptr).lenc[i] = Sensor_ReadReg(start_addr + i);
	}

	// clear otp buffer
	for (i=start_addr; i<=end_addr; i++) {
		Sensor_WriteReg(i, 0x00);
	}
	return 0;
}

// R_gain, sensor red gain of AWB, 0x400 =1
// G_gain, sensor green gain of AWB, 0x400 =1
// B_gain, sensor blue gain of AWB, 0x400 =1
// return 0;
LOCAL uint32_t ov13850_update_awb_gain(uint32_t R_gain, uint32_t G_gain, uint32_t B_gain)
{
	OTP_PRINT("R_gain = 0x%x,  G_gain = 0x%x,  B_gain = 0x%x\n", R_gain, G_gain, B_gain );

	if (R_gain>0x400) {
		Sensor_WriteReg(0x5056, R_gain>>8);
		Sensor_WriteReg(0x5057, R_gain & 0x00ff);
	}
	if (G_gain>0x400) {
		Sensor_WriteReg(0x5058, G_gain>>8);
		Sensor_WriteReg(0x5059, G_gain & 0x00ff);
	}

	if (B_gain>0x400) {
		Sensor_WriteReg(0x505A, B_gain>>8);
		Sensor_WriteReg(0x505B, B_gain & 0x00ff);
	}
	return 0;
}

// otp_ptr: pouint32_ter of otp_struct
LOCAL uint32_t ov13850_update_lenc(struct otp_struct * otp_ptr)
{
	uint32_t i = 0;
	uint32_t temp = 0;
	temp = Sensor_ReadReg(0x5000);
	temp = 0x01 | temp;

	Sensor_WriteReg(0x5000, temp);

	for(i=0; i<OTP_DRV_LSC_SIZE; i++) {
		Sensor_WriteReg(OTP_DRV_LSC_REG_ADDR + i, (*otp_ptr).lenc[i]);
	}
	return 0;
}

// call this function after OV13850 initialization
// return value: 0 update success
// 1, no OTP
LOCAL uint32_t ov13850_update_otp_wb()
{
	struct otp_struct current_otp;
	uint32_t i = 0;
	uint32_t otp_index = 0;
	uint32_t temp = 0;
	uint32_t rg = 0;
	uint32_t bg = 0;
	uint32_t nR_G_gain = 0;
	uint32_t nB_G_gain = 0;
	uint32_t nG_G_gain = 0;
	uint32_t nBase_gain = 0;
	uint32_t R_gain = 0;
	uint32_t B_gain = 0;
	uint32_t G_gain = 0;

	// R/G and B/G of current camera module is read out from sensor OTP
	// check first OTP with valid data
	for(i=1;i<=OTP_DRV_AWB_GROUP_COUNT;i++) {
		temp = ov13850_check_otp_wb(i);
		if (temp == 2) {
			otp_index = i;
			break;
		}
	}
	if (i>OTP_DRV_AWB_GROUP_COUNT) {
		// no valid wb OTP data
		return 1;
	}
	ov13850_read_otp_wb(otp_index, &current_otp);
	if(current_otp.light_rg==0) {
		// no light source information in OTP, light factor = 1
		rg = current_otp.rg_ratio;
	}
	else {
		rg = current_otp.rg_ratio * (current_otp.light_rg +512) / 1024;
	}
	if(current_otp.light_bg==0) {
		// not light source information in OTP, light factor = 1
		bg = current_otp.bg_ratio;
	}
	else {
		bg = current_otp.bg_ratio * (current_otp.light_bg +512) / 1024;
	}
	//calculate G gain
	nR_G_gain = (OV13850_RG_Ratio_Typical*1000) / rg;
	nB_G_gain = (OV13850_BG_Ratio_Typical*1000) / bg;
	nG_G_gain = 1000;
	if (nR_G_gain < 1000 || nB_G_gain < 1000)
	{
		if (nR_G_gain < nB_G_gain)
			nBase_gain = nR_G_gain;
		else
			nBase_gain = nB_G_gain;
	}
	else
	{
		nBase_gain = nG_G_gain;
	}
	R_gain = 0x400 * nR_G_gain / (nBase_gain);
	B_gain = 0x400 * nB_G_gain / (nBase_gain);
	G_gain = 0x400 * nG_G_gain / (nBase_gain);

	ov13850_update_awb_gain(R_gain, G_gain, B_gain);
	return 0;
}

// call this function after OV13850 initialization
// return value: 0 update success
// 1, no OTP
LOCAL uint32_t ov13850_update_otp_lenc()
{
	struct otp_struct current_otp;
	uint32_t i = 0;
	uint32_t otp_index = 0;
	uint32_t temp = 0;

	// check first lens correction OTP with valid data
	for(i=1; i<=OTP_DRV_LSC_GROUP_COUNT; i++) {
		temp = ov13850_check_otp_lenc(i);
		if (temp == 2) {
			otp_index = i;
			break;
		}
	}
	if (i > OTP_DRV_LSC_GROUP_COUNT) {
		// no valid WB OTP data
		return 1;
	}
	ov13850_read_otp_lenc(otp_index, &current_otp);
	ov13850_update_lenc(&current_otp);
	// success
	return 0;
}
