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
#define DEBUG_OTP_STR     "OV13850R2A_OTP_debug: L %d, %s: "
#define DEBUG_OTP_ARGS    __LINE__,__FUNCTION__

#define OTP_PRINT(format,...) ALOGE(DEBUG_OTP_STR format, DEBUG_OTP_ARGS, ##__VA_ARGS__)
#else
#define OTP_PRINT
#endif

#define RG_TYPICAL_R2A 0x121
#define BG_TYPICAL_R2A 0x127
/*
static uint16_t RG_Ratio_Typical = 0x100;
static uint16_t BG_Ratio_Typical = 0x100;
*/

struct otp_struct {
	uint32_t flag; // bit[7]: info, bit[6]:wb, bit[5]:vcm, bit[4]:lenc
	uint32_t module_integrator_id;
	uint32_t lens_id;
	uint32_t production_year;
	uint32_t production_month;
	uint32_t production_day;
	uint32_t rg_ratio;
	uint32_t bg_ratio;
	//uint32_t light_rg;
	//uint32_t light_bg;
	uint32_t lenc[186];
	uint32_t checksumLSC;
	uint32_t checksumOTP;
	uint32_t checksumTotal;
	uint32_t VCM_start;
	uint32_t VCM_end;
	uint32_t VCM_dir;
	uint32_t index;
};

// index: index of otp group. (1, 2,...,OTP_DRV_INFO_GROUP_COUNT)
// otp_ptr: pouint32_ter of otp_struct
// return: 0,
LOCAL uint32_t ov13850r2a_read_otp_info(struct otp_struct *otp_ptr)
{
	//uint32_t i = 0;
	uint32_t otp_flag, addr, temp;
	//set 0x5002[1] to "0"
	uint32_t temp1;
	temp1 = Sensor_ReadReg(0x5002);
	Sensor_WriteReg(0x5002, (0x00 & 0x02) | (temp1 & (~0x02)));
	// read OTP into buffer
	Sensor_WriteReg(0x3d84, 0xC0);
	Sensor_WriteReg(0x3d88, 0x72); // OTP start address
	Sensor_WriteReg(0x3d89, 0x20);
	Sensor_WriteReg(0x3d8A, 0x73); // OTP end address
	Sensor_WriteReg(0x3d8B, 0xBE);
	Sensor_WriteReg(0x3d81, 0x01); // load otp into buffer
	usleep(10*1000);
	// OTP base information and WB calibration data
	otp_flag = Sensor_ReadReg(0x7220);
	addr= 0;
	if((otp_flag & 0xc0) == 0x40) {
		addr= 0x7221; // base address of info group 1
	} else if((otp_flag & 0x30) == 0x10) {
		addr= 0x7229; // base address of info group 2
	}
	if(addr != 0) {
		(*otp_ptr).flag = 0xC0; // valid info and AWB in OTP
		(*otp_ptr).module_integrator_id = Sensor_ReadReg( addr);
		(*otp_ptr).lens_id = Sensor_ReadReg( addr+ 1);
		(*otp_ptr).production_year = Sensor_ReadReg( addr + 2);
		(*otp_ptr).production_month = Sensor_ReadReg( addr + 3);
		(*otp_ptr).production_day = Sensor_ReadReg(addr + 4);
		temp = Sensor_ReadReg(addr + 7);
		(*otp_ptr).rg_ratio = (Sensor_ReadReg(addr + 5)<<2) + ((temp>>6) & 0x03);
		(*otp_ptr).bg_ratio = (Sensor_ReadReg(addr + 6)<<2) + ((temp>>4) & 0x03);
	} else {
		(*otp_ptr).flag = 0x00; // not info in OTP
		(*otp_ptr).module_integrator_id = 0;
		(*otp_ptr).lens_id = 0;
		(*otp_ptr).production_year = 0;
		(*otp_ptr).production_month = 0;
		(*otp_ptr).production_day = 0;
	}
	if( 0x7221 == addr) {
		(*otp_ptr).index = 1;
	} else if( 0x7229 == addr) {
		(*otp_ptr).index = 2;
	} else {
		(*otp_ptr).index = 0xff;
		return 1;
	}
	OTP_PRINT("(*otp_ptr).rg_ratio: %d,\n", (*otp_ptr).rg_ratio);
	OTP_PRINT("(*otp_ptr).bg_ratio: %d,\n", (*otp_ptr).bg_ratio);

	return 0;
}

/** read_otp_VCM:
 *    @otp_ptr: point to sensor otp structure
 *    @e_ctrl: point to sensor_eeprom_data_t of the eeprom device
 *
 * Read OTP VCM Calibration data from sensor
 *
 * return value. 0
 **/
uint8_t read_otp_VCM_R2A(uint32_t index, struct otp_struct * otp_ptr)
{
	uint32_t otp_flag, addr,temp;
	uint8_t ret =0;
	(*otp_ptr).flag  = 0;

	// OTP VCM Calibration
	otp_flag = Sensor_ReadReg(0x73ac);
	addr = 0;
	if((otp_flag & 0xc0) == 0x40) {
		addr = 0x73ad; // base address of VCM Calibration group 1
	}
	else if((otp_flag & 0x30) == 0x10) {
		addr = 0x73b0; // base address of VCM Calibration group 2
	}
	if(addr != 0) {
		(*otp_ptr).flag |= 0x20;
		temp = Sensor_ReadReg(addr + 2);
		(* otp_ptr).VCM_start = (Sensor_ReadReg(addr)<<2) | ((temp>>6) & 0x03);
		(* otp_ptr).VCM_end = (Sensor_ReadReg(addr + 1) << 2) | ((temp>>4) & 0x03);
		(* otp_ptr).VCM_dir = (temp>>2) & 0x03;
		ret = 0;
	}else {
		(* otp_ptr).VCM_start = 0;
		(* otp_ptr).VCM_end = 0;
		(* otp_ptr).VCM_dir = 0;
		ret = 1;
	}

	return ret;
}


/**************************Decode LENC Para Process Start*****************************************************/
void LumaDecoder(uint8_t *pData, uint8_t *pPara)
{
	uint32_t Offset, Bit, Option;
	uint32_t i, k;
	uint8_t pCenter[16], pMiddle[32], pCorner[72];
	Offset = pData[0];
	Bit = pData[1]>>4;
	Option = pData[1]&0xf;

	if(Bit <= 5){
		for(i=0,k=2; i<120; i+=8,k+=5){
			pPara[i] = pData[k]>>3;	// 7~3 (byte0)
			pPara[i+1] = ((pData[k]&0x7)<<2)|(pData[k+1]>>6); // 2~0 (byte0) and 7~6 (byte1)
			pPara[i+2] = (pData[k+1]&0x3e)>>1;	// 5~1 (byte1)
			pPara[i+3] = ((pData[k+1]&0x1)<<4)|(pData[k+2]>>4); // 0 (byte1) and 7~4 (byte2)
			pPara[i+4] = ((pData[k+2]&0xf)<<1)|(pData[k+3]>>7); // 3~0 (byte2) and 7 (byte3)
			pPara[i+5] = (pData[k+3]&0x7c)>>2;	// 6~2 (byte3)
			pPara[i+6] = ((pData[k+3]&0x3)<<3)|(pData[k+4]>>5); // 1~0 (byte3) and 7~5 (byte4)
			pPara[i+7] = pData[k+4]&0x1f;	// 4~0 (byte4)
		}
	}else{
		for(i=0,k=2; i<48; i+=8,k+=5){
			pPara[i] = pData[k]>>3;	// 7~3 (byte0)
			pPara[i+1] = ((pData[k]&0x7)<<2)|(pData[k+1]>>6); // 2~0 (byte0) and 7~6 (byte1)
			pPara[i+2] = (pData[k+1]&0x3e)>>1;	// 5~1 (byte1)
			pPara[i+3] = ((pData[k+1]&0x1)<<4)|(pData[k+2]>>4); // 0 (byte1) and 7~4 (byte2)
			pPara[i+4] = ((pData[k+2]&0xf)<<1)|(pData[k+3]>>7); // 3~0 (byte2) and 7 (byte3)
			pPara[i+5] = (pData[k+3]&0x7c)>>2;	// 6~2 (byte3)
			pPara[i+6] = ((pData[k+3]&0x3)<<3)|(pData[k+4]>>5); // 1~0 (byte3) and 7~5 (byte4)
			pPara[i+7] = pData[k+4]&0x1f;	// 4~0 (byte4)
		}

		for(i=48,k=32; i<120; i+=4,k+=3){
			pPara[i] = pData[k]>>2;	// 7~2 (byte0)
			pPara[i+1] = ((pData[k]&0x3)<<4)|(pData[k+1]>>4); //1~0 (byte0) and 7~4(byte1)
			pPara[i+2] = ((pData[k+1]&0xf)<<2)|(pData[k+2]>>6); //3~0 (byte1) and 7~6(byte2)
			pPara[i+3] = pData[k+2]&0x3f; // 5~0 (byte2)
		}

		memcpy(pCenter, pPara, 16);
		memcpy(pMiddle, pPara+16, 32);
		memcpy(pCorner, pPara+48, 72);
		for(i=0; i<32; i++){
			pMiddle[i] <<= (Bit-6);
		}
		for(i=0; i<72; i++){
			pCorner[i] <<= (Bit-6);
    }
		if(Option == 0){  // 10x12
			memcpy(pPara, pCorner, 26);
			memcpy(pPara+26, pMiddle, 8);
			memcpy(pPara+34, pCorner+26, 4);
			memcpy(pPara+38, pMiddle+8, 2);
			memcpy(pPara+40, pCenter, 4);
			memcpy(pPara+44, pMiddle+10, 2);
			memcpy(pPara+46, pCorner+30, 4);
			memcpy(pPara+50, pMiddle+12, 2);
			memcpy(pPara+52, pCenter+4, 4);
			memcpy(pPara+56, pMiddle+14, 2);
			memcpy(pPara+58, pCorner+34, 4);
			memcpy(pPara+62, pMiddle+16, 2);
			memcpy(pPara+64, pCenter+8, 4);
			memcpy(pPara+68, pMiddle+18, 2);
			memcpy(pPara+70, pCorner+38, 4);
			memcpy(pPara+74, pMiddle+20, 2);
			memcpy(pPara+76, pCenter+12, 4);
			memcpy(pPara+80, pMiddle+22, 2);
			memcpy(pPara+82, pCorner+42, 4);
			memcpy(pPara+86, pMiddle+24, 8);
			memcpy(pPara+94, pCorner+46, 26);
		}else{  // 12x10
			memcpy(pPara, pCorner, 22);
			memcpy(pPara+22, pMiddle, 6);
			memcpy(pPara+28, pCorner+22, 4);
			memcpy(pPara+32, pMiddle+6, 6);
			memcpy(pPara+38, pCorner+26, 4);
			memcpy(pPara+42, pMiddle+12, 1);
			memcpy(pPara+43, pCenter, 4);
			memcpy(pPara+47, pMiddle+13, 1);
			memcpy(pPara+48, pCorner+30, 4);
			memcpy(pPara+52, pMiddle+14, 1);
			memcpy(pPara+53, pCenter+4, 4);
			memcpy(pPara+57, pMiddle+15, 1);
			memcpy(pPara+58, pCorner+34, 4);
			memcpy(pPara+62, pMiddle+16, 1);
			memcpy(pPara+63, pCenter+8, 4);
			memcpy(pPara+67, pMiddle+17, 1);
			memcpy(pPara+68, pCorner+38, 4);
			memcpy(pPara+72, pMiddle+18, 1);
			memcpy(pPara+73, pCenter+12, 4);
			memcpy(pPara+77, pMiddle+19, 1);
			memcpy(pPara+78, pCorner+42, 4);
			memcpy(pPara+82, pMiddle+20, 6);
			memcpy(pPara+88, pCorner+46, 4);
			memcpy(pPara+92, pMiddle+26, 6);
			memcpy(pPara+98, pCorner+50, 22);
		}
	}

	for(i=0; i<120; i++){
	  pPara[i] += Offset;
	}
}

//
void ColorDecoder(uint8_t *pData, uint8_t *pPara)
{
	uint32_t Offset, Bit, Option;
	uint32_t i, k;
	uint8_t pBase[30];

	Offset = pData[0];
	Bit = pData[1]>>7;
	Option = (pData[1]&0x40)>>6;
	pPara[0] = (pData[1]&0x3e)>>1; // 5~1 (byte1)
	pPara[1] = ((pData[1]&0x1)<<4)|(pData[2]>>4); // 0 (byte1) and 7~4 (byte2)
	pPara[2] = ((pData[2]&0xf)<<1)|(pData[3]>>7); // 3~0 (byte2) and 7 (byte3)
	pPara[3] = (pData[3]&0x7c)>>2; // 6~2 (byte3)
	pPara[4] = ((pData[3]&0x3)<<3)|(pData[4]>>5); // 1~0 (byte3) and 7~5 (byte4)
	pPara[5] = pData[4]&0x1f; // 4~0 (byte4)

	for(i=6,k=5; i<30; i+=8,k+=5){
		pPara[i] = pData[k]>>3;	// 7~3 (byte0)
		pPara[i+1] = ((pData[k]&0x7)<<2)|(pData[k+1]>>6); // 2~0 (byte0) and 7~6 (byte1)
		pPara[i+2] = (pData[k+1]&0x3e)>>1;	// 5~1 (byte1)
		pPara[i+3] = ((pData[k+1]&0x1)<<4)|(pData[k+2]>>4); // 0 (byte1) and 7~4 (byte2)
		pPara[i+4] = ((pData[k+2]&0xf)<<1)|(pData[k+3]>>7); // 3~0 (byte2) and 7 (byte3)
		pPara[i+5] = (pData[k+3]&0x7c)>>2;	// 6~2 (byte3)
		pPara[i+6] = ((pData[k+3]&0x3)<<3)|(pData[k+4]>>5); // 1~0 (byte3) and 7~5 (byte4)
		pPara[i+7] = pData[k+4]&0x1f;	// 4~0 (byte4)
	}
	memcpy(pBase, pPara, 30);

	for(i=0,k=20; i<120; i+=4,k++){
		pPara[i] = pData[k]>>6;
		pPara[i+1] = (pData[k]&0x30)>>4;
		pPara[i+2] = (pData[k]&0xc)>>2;
		pPara[i+3] = pData[k]&0x3;
	}

	if(Option == 0){ // 10x12
		for(i=0; i<5; i++){
			for(k=0; k<6; k++){
				pPara[i*24+k*2] += pBase[i*6+k];
				pPara[i*24+k*2+1] += pBase[i*6+k];
				pPara[i*24+k*2+12] += pBase[i*6+k];
				pPara[i*24+k*2+13] += pBase[i*6+k];
			}
		}
	}else{  // 12x10
		for(i=0; i<6; i++){
			for(k=0; k<5; k++){
				pPara[i*20+k*2] += pBase[i*5+k];
				pPara[i*20+k*2+1] += pBase[i*5+k];
				pPara[i*20+k*2+10] += pBase[i*5+k];
				pPara[i*20+k*2+11] += pBase[i*5+k];
			}
		}
	}

	for(i=0; i<120; i++){
		pPara[i] = (pPara[i]<<Bit) + Offset;
	}
}

//
void OV13850_R2A_LENC_Decoder(uint8_t *pData, uint8_t *pPara)
{
	LumaDecoder(pData, pPara);
	ColorDecoder(pData+86, pPara+120);
	ColorDecoder(pData+136, pPara+240);
}

/***********************Decode LENC Process End**********************************************************/

/** read_otp_lenc:
 *    @otp_ptr: pointer of otp_struct
 *    @e_ctrl: point to sensor_eeprom_data_t of the eeprom device
 *
 * Read LENC OTP Calibration Data from sensor
 *
 * return value 0 success  1 fail
 **/
uint8_t read_otp_lenc_R2A(struct otp_struct *otp_ptr)
{
	uint32_t otp_flag, addr, i;
	uint8_t ret =0;

	(*otp_ptr).flag = 0x00; //
	// OTP Lenc Calibration
	otp_flag = Sensor_ReadReg(0x7231);
	addr = 0;
	if((otp_flag & 0xc0) == 0x40) {
		addr = 0x7232; // base address of Lenc Calibration group 1
		(*otp_ptr).index = 1;
	}else if((otp_flag & 0x30) == 0x10) {
		addr = 0x72ef; // base address of Lenc Calibration group 2
		(*otp_ptr).index = 2;
	}
	OTP_PRINT("read_otp_lenc_R2A otp_index : %d\n",(*otp_ptr).index);

	if(addr != 0) {
		for(i=0;i<186;i++) {
			(* otp_ptr).lenc[i]= Sensor_ReadReg(addr + i);
		}
		(* otp_ptr).flag = ((* otp_ptr).flag | 0x10);
		ret = 0;
	}else {
		for(i=0;i<186;i++) {
			(* otp_ptr).lenc[i]=0;
		}
		ret = 1;
		OTP_PRINT("Fail to read_otp_lenc_R2A\n");
	}

	return ret;
}

/** apply_otp_lenc:
 *    @otp_ptr: pointer of otp_struct
 *    @e_ctrl: point to sensor_eeprom_data_t of the eeprom device
 *
 * apply LENC OTP Calibration Data to sensor
 *
 * return value 0 success 1 fail
 **/
uint8_t apply_otp_lenc_R2A(struct otp_struct *otp_ptr)
{
	uint32_t i;
	uint8_t lenc_out[360];
	uint32_t checksumLSC = 0, checksumOTP = 0, checksumTotal = 0;
	uint32_t lenc_flag  = 0;
	uint8_t ret = 0;
	uint32_t addr = 0, otp_flag = 0;
	uint8_t data_in[186];

	memset(data_in, 0, sizeof(data_in));
	// apply OTP Lenc Calibration
	if ((*otp_ptr).flag & 0x10) {
		//Decode the lenc buffer from OTP , from 186 bytes to 360 bytes
		for(i=0;i<360;i++){
			lenc_out[i] = 0;
		}

		for(i=0;i<186;i++) {
			checksumLSC += (* otp_ptr).lenc[i];
		}

		for(i = 0; i < sizeof(data_in); i++){
		  data_in[i] = (uint8_t)(*otp_ptr).lenc[i];
		}

		OV13850_R2A_LENC_Decoder(data_in, lenc_out);
		for(i = 0; i < 360; i++) {
				checksumOTP += lenc_out[i];
		}

		checksumLSC = (checksumLSC)%255 + 1;
		checksumOTP = (checksumOTP)%255 + 1;
		otp_flag = Sensor_ReadReg(0x7231);
		addr = 0;

		if((otp_flag & 0xc0) == 0x40) {
			addr = 0x7232; // base address of Lenc Calibration group 1
		} else if((otp_flag & 0x30) == 0x10) {
			addr = 0x72ef; // base address of Lenc Calibration group 2
		}

		checksumTotal = (checksumLSC) ^ (checksumOTP);
		(* otp_ptr).checksumLSC = Sensor_ReadReg((addr + 186));
		(* otp_ptr).checksumOTP = Sensor_ReadReg((addr + 187));
		(* otp_ptr).checksumTotal = Sensor_ReadReg((addr + 188));

		if((* otp_ptr).checksumLSC == checksumLSC && (* otp_ptr).checksumOTP == checksumOTP){
			lenc_flag = 1;
		}else if((* otp_ptr).checksumTotal == checksumTotal){
			lenc_flag = 1;
		}

		OTP_PRINT("(* otp_ptr).checksumLSC: %d, checksumLSC %d\n", (* otp_ptr).checksumLSC, checksumLSC);
		OTP_PRINT("(* otp_ptr).checksumOTP: %d, checksumOTP %d\n", (* otp_ptr).checksumOTP, checksumOTP);
		OTP_PRINT("(* otp_ptr).checksumTotal: %d, checksumTotal %d\n", (* otp_ptr).checksumTotal, checksumTotal);

		if(lenc_flag == 1){
			for(i=0;i<360 ;i++) {
				//g_reg_array[g_reg_setting.size].reg_addr = 0x5200 +i;
				//g_reg_array[g_reg_setting.size].reg_data = lenc_out[i];
				//g_reg_setting.size++;
				Sensor_WriteReg(0x5200+i, lenc_out[i]);
			}
			//for(i=350;i<360 ;i++) {
			//	OTP_PRINT("decoder success lenc_out[%d] 0x%x\n",i,lenc_out[i]);
			//}

			OTP_PRINT("decoder success\n");
		}else{
			ret = 1;
			OTP_PRINT("Fail to apply_otp_lenc_R2A\n");
		}
	}else{
		ret = 1;
	}

	return ret;
}


/** read_otp_wb:
 *    @otp_ptr: pointer of otp_struct
 *    @e_ctrl: point to sensor_eeprom_data_t of the eeprom device
 *
 * Read WB OTP calibration data from sensor
 *
 * return value 0 success  1 fail
 **/
static uint8_t read_otp_wb_R2A(struct otp_struct *otp_ptr)
{
	uint32_t otp_flag, addr, temp;
	uint8_t ret =0;

	// OTP base information and WB calibration data
	otp_flag = Sensor_ReadReg(0x7220);
	addr = 0;
	(*otp_ptr).flag = ((*otp_ptr).flag & (0x3f) );
	OTP_PRINT("read otp_wb_R2A otp_flag : 0x%x\n",(*otp_ptr).flag);

	if((otp_flag & 0xc0) == 0x40) {
		addr = 0x7221; // base address of info group 1
		(*otp_ptr).index = 1;
	}else if((otp_flag & 0x30) == 0x10) {
		addr = 0x7229; // base address of info group 2
		(*otp_ptr).index = 2;
	}
	OTP_PRINT("read otp_wb_R2A otp_index : %d\n",(*otp_ptr).index);

	if(addr != 0) {
		(*otp_ptr).flag = ((*otp_ptr).flag | 0xc0);
		temp = Sensor_ReadReg(addr + 7);
		(*otp_ptr).rg_ratio = (Sensor_ReadReg(addr + 5)<<2) + ((temp>>6) & 0x03);
		(*otp_ptr).bg_ratio = (Sensor_ReadReg(addr + 6)<<2) + ((temp>>4) & 0x03);
		ret = 0;
	}else {
		(*otp_ptr).flag = 0;
		(*otp_ptr).rg_ratio = 0;
		(*otp_ptr).bg_ratio = 0;
		//(*otp_ptr).light_rg = 0;
		//(*otp_ptr).light_bg = 0;
		OTP_PRINT("Fail to read otp_wb_R2A\n");
		ret = 1;
	}

	return ret; //0 success  1 fail
}

//
/** apply_otp_wb:
 *    @otp_ptr: pointer of otp_struct
 *    @e_ctrl: point to sensor_eeprom_data_t of the eeprom device
 *
 * Apply WB OTP calibration data to sensor
 *
 * return value 0 success  1 fail
 **/
static uint8_t apply_otp_wb_R2A(struct otp_struct *otp_ptr)
{
	uint8_t ret =0;
	uint32_t rg, bg, R_gain, G_gain, B_gain, Base_gain;

	// apply OTP WB Calibration
	if ((*otp_ptr).flag & 0x40) {
		rg = (*otp_ptr).rg_ratio;
		bg = (*otp_ptr).bg_ratio;

		//calculate G gain
		R_gain = (RG_TYPICAL_R2A*1000) / rg;
		B_gain = (BG_TYPICAL_R2A*1000) / bg;
		G_gain = 1000;

		if (R_gain < 1000 || B_gain < 1000){
			if (R_gain < B_gain){
			  Base_gain = R_gain;
			}else{
			  Base_gain = B_gain;
			}
		}else{
			Base_gain = G_gain;
		}

		R_gain = 0x400 * R_gain / (Base_gain);
		B_gain = 0x400 * B_gain / (Base_gain);
		G_gain = 0x400 * G_gain / (Base_gain);

		// update sensor WB gain
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
		OTP_PRINT("Succeed to apply otp_wb_R2A, R_gain: %d, G_gain: %d, B_gain: %d,\n",R_gain,G_gain,B_gain);
	}else{
		OTP_PRINT("Fail to apply otp_wb_R2A\n");
		ret = 1;
	}

	return ret;
}

// call this function after OV13850R2A initialization
// return value: 0 update success
// 1, no OTP
LOCAL uint32_t ov13850r2a_update_otp_wb()
{
	struct otp_struct current_otp;
	uint32_t rtn=1;
        memset(&current_otp, 0, sizeof(struct otp_struct));

	rtn = read_otp_wb_R2A(&current_otp);
	if (rtn){
		OTP_PRINT("Fail to read_otp_wb_R2A\n");
	}

	rtn = apply_otp_wb_R2A(&current_otp);
	if (rtn){
		OTP_PRINT("Fail to apply_otp_wb_R2A\n");
	}

	return 0;
}
// call this function after OV13850R2A initialization
// return value: 0 update success
// 1, no OTP
LOCAL uint32_t ov13850r2a_update_otp_lenc()
{
	struct otp_struct current_otp;
	uint32_t rtn=1;
	memset(&current_otp, 0, sizeof(struct otp_struct));

	rtn = read_otp_lenc_R2A(&current_otp);
	if (rtn){
		OTP_PRINT("Fail to read_otp_lenc_R2A\n");
	}
	rtn = apply_otp_lenc_R2A(&current_otp);
	if (rtn){
		OTP_PRINT("Fail to apply_otp_lenc_R2A\n");
	}

	return 0;
}
