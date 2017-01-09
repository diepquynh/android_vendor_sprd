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

#define OV8825_TRULY_02 0x02

#define OV8825_TRULY_OTP_DATA_ADDR         0x3D00
#define OV8825_TRULY_OTP_LOAD_ADDR         0x3D81
#define OV8825_TRULY_OTP_BANK_ADDR         0x3D84

#define OV8825_TRULY_LENC_START_ADDR       0x5800
#define OV8825_TRULY_LENC_REG_SIZE         62

#define OV8825_TRULY_OTP_LENC_GROUP_ADDR   0x3D00

#define OV8825_TRULY_OTP_WB_GROUP_ADDR     0x3D05
#define OV8825_TRULY_OTP_WB_GROUP_SIZE     9

#define OV8825_TRULY_GAIN_RH_ADDR          0x3400
#define OV8825_TRULY_GAIN_RL_ADDR          0x3401
#define OV8825_TRULY_GAIN_GH_ADDR          0x3402
#define OV8825_TRULY_GAIN_GL_ADDR          0x3403
#define OV8825_TRULY_GAIN_BH_ADDR          0x3404
#define OV8825_TRULY_GAIN_BL_ADDR          0x3405

#define OV8825_TRULY_GAIN_DEFAULT_VALUE    0x0400 // 1x gain

#define OV8825_TRULY_OTP_MID               OV8825_TRULY_02
#define  OV8825_TRULY_BG_RATIO_TYPICAL (0x4E)
#define  OV8825_TRULY_RG_RATIO_TYPICAL (0x4A)

struct ov8825_otp_info {
	int32_t module_integrator_id;
	int32_t lens_id;
	int32_t production_year;
	int32_t production_month;
	int32_t production_day;
	int32_t rg_ratio;
	int32_t bg_ratio;
	int32_t light_rg;
	int32_t light_bg;
	int32_t user_data[5];
	int32_t lenc[62];
	int32_t VCM_start;
	int32_t VCM_end;
	int32_t VCM_dir;
};

// R/G and B/G of current camera module
LOCAL uint8_t truly_rg_ratio = 0;
LOCAL uint8_t truly_bg_ratio = 0;
LOCAL uint8_t truly_otp_lenc_data[62];

// Enable OTP read function
LOCAL void _ov8825_truly_otp_read_enable(void)
{
	Sensor_WriteReg(OV8825_TRULY_OTP_LOAD_ADDR, 0x01);
	usleep(15*1000); // sleep > 10ms
}

// Disable OTP read function
LOCAL void _ov8825_truly_otp_read_disable(void)
{
	Sensor_WriteReg(OV8825_TRULY_OTP_LOAD_ADDR, 0x00);
}

LOCAL void _ov8825_truly_otp_read(uint16_t otp_addr, uint8_t* otp_data)
{
	_ov8825_truly_otp_read_enable();
	*otp_data = Sensor_ReadReg(otp_addr);
	_ov8825_truly_otp_read_disable();
}

/*******************************************************************************
* Function    :  _ov8825_truly_otp_clear
* Description :  Clear OTP buffer 
* Parameters  :  none
* Return      :  none
*******************************************************************************/
LOCAL void _ov8825_truly_otp_clear(void)
{
	// After read/write operation, the OTP buffer should be cleared to avoid accident write
	uint8_t i;
	for (i=0; i<32; i++) 
	{
		Sensor_WriteReg(OV8825_TRULY_OTP_DATA_ADDR+i, 0x00);
	}
}

/*******************************************************************************
* Function    :  _ov8825_truly_otp_check_wb_group
* Description :  Check OTP Space Availability
* Parameters  :  [in] index : index of otp group (0, 1, 2)
* Return      :  0, group index is empty
                 1, group index has invalid data
                 2, group index has valid data
                -1, group index error
*******************************************************************************/
LOCAL int8_t _ov8825_truly_otp_check_wb_group(uint8_t index)
{
	uint16_t otp_addr = OV8825_TRULY_OTP_WB_GROUP_ADDR + index * OV8825_TRULY_OTP_WB_GROUP_SIZE;
	uint8_t  flag;
	int      rtn = 0;

	if (index > 2)
	{
		SENSOR_PRINT("OTP input wb group index %d error\n", index);
		return -1;
	}
		
	// select bank 0
	rtn = Sensor_WriteReg(OV8825_TRULY_OTP_BANK_ADDR, 0x08);
	if (rtn) {
		SENSOR_PRINT("Another sensor module");
		return -1;
	}

	_ov8825_truly_otp_read(otp_addr, &flag);
	_ov8825_truly_otp_clear();

	// Check all bytes of a group. If all bytes are '0', then the group is empty. 
	// Check from group 1 to group 2, then group 3.
	if (!flag)
	{
		SENSOR_PRINT("wb group %d is empty\n", index);
		return 0;
	}
	else if ((!(flag&0x80)) && (flag&0x7f))
	{
		SENSOR_PRINT("wb group %d has valid data\n", index);
		return 2;
	}
	else
	{
		SENSOR_PRINT("wb group %d has invalid data\n", index);
		return 1;
	}
}

/*******************************************************************************
* Function    :  _ov8825_truly_otp_read_wb_group
* Description :  Read group value and store it in OTP Struct 
* Parameters  :  [in] index : index of otp group (0, 1, 2)
* Return      :  group index (0, 1, 2)
                 -1, error
*******************************************************************************/
LOCAL int8_t _ov8825_truly_otp_read_wb_group(int8_t index)
{
	uint16_t otp_addr;
	uint8_t mid;

	if (index == -1)
	{
		// Check first OTP with valid data
		for (index=0; index<3; index++)
		{
			if (_ov8825_truly_otp_check_wb_group(index) == 2)
			{
				SENSOR_PRINT("read wb from group %d\n", index);
				break;
			}
		}

		if (index > 2)
		{
			SENSOR_PRINT("no group has valid data\n");
			return -1;
		}
	}
	else
	{
		if (_ov8825_truly_otp_check_wb_group(index) != 2)
		{
			SENSOR_PRINT("read wb from group %d failed\n", index);
			return -1;
		}
	}

	otp_addr = OV8825_TRULY_OTP_WB_GROUP_ADDR + index * OV8825_TRULY_OTP_WB_GROUP_SIZE;

	// select bank 0
	Sensor_WriteReg(OV8825_TRULY_OTP_BANK_ADDR, 0x08);

	_ov8825_truly_otp_read(otp_addr, &mid);
	if ((mid&0x7f) != OV8825_TRULY_OTP_MID)
	{
		return -1;
	}

	_ov8825_truly_otp_read(otp_addr+2, &truly_rg_ratio);
	_ov8825_truly_otp_read(otp_addr+3, &truly_bg_ratio);
	_ov8825_truly_otp_clear();

	SENSOR_PRINT("read wb finished\n");
	return index;
}

#ifdef SUPPORT_FLOATING //Use this if support floating point values
/*******************************************************************************
* Function    :  _ov8825_truly_otp_apply_wb
* Description :  Calcualte and apply R, G, B gain to module
* Parameters  :  [in] golden_rg : R/G of golden camera module
                 [in] golden_bg : B/G of golden camera module
* Return      :  1, success; 0, fail
*******************************************************************************/
LOCAL uint32_t _ov8825_truly_otp_apply_wb(uint8_t golden_rg, uint8_t golden_bg)
{
	uint16_t gain_r = OV8825_TRULY_GAIN_DEFAULT_VALUE;
	uint16_t gain_g = OV8825_TRULY_GAIN_DEFAULT_VALUE;
	uint16_t gain_b = OV8825_TRULY_GAIN_DEFAULT_VALUE;

	double ratio_r, ratio_g, ratio_b;
	double cmp_rg, cmp_bg;

	if (!golden_rg || !golden_bg)
	{
		SENSOR_PRINT("golden_rg / golden_bg can not be zero\n");
		return 0;
	}

	// Calcualte R, G, B gain of current module from R/G, B/G of golden module
    // and R/G, B/G of current module
	cmp_rg = 1.0 * truly_rg_ratio / golden_rg;
	cmp_bg = 1.0 * truly_bg_ratio / golden_bg;

	if ((cmp_rg<1) && (cmp_bg<1))
	{
		// R/G < R/G golden, B/G < B/G golden
		ratio_g = 1;
		ratio_r = 1 / cmp_rg;
		ratio_b = 1 / cmp_bg;
	}
	else if (cmp_rg > cmp_bg)
	{
		// R/G >= R/G golden, B/G < B/G golden
		// R/G >= R/G golden, B/G >= B/G golden
		ratio_r = 1;
		ratio_g = cmp_rg;
		ratio_b = cmp_rg / cmp_bg;
	}
	else
	{
		// B/G >= B/G golden, R/G < R/G golden
		// B/G >= B/G golden, R/G >= R/G golden
		ratio_b = 1;
		ratio_g = cmp_bg;
		ratio_r = cmp_bg / cmp_rg;
	}

	// write sensor wb gain to registers
	// 0x0400 = 1x gain
	if (ratio_r != 1)
	{
		gain_r = (uint16_t)(OV8825_TRULY_GAIN_DEFAULT_VALUE * ratio_r);
		Sensor_WriteReg(OV8825_TRULY_GAIN_RH_ADDR, gain_r >> 8);
		Sensor_WriteReg(OV8825_TRULY_GAIN_RL_ADDR, gain_r & 0x00ff);
	}

	if (ratio_g != 1)
	{
		gain_g = (uint16_t)(OV8825_TRULY_GAIN_DEFAULT_VALUE * ratio_g);
		Sensor_WriteReg(OV8825_TRULY_GAIN_GH_ADDR, gain_g >> 8);
		Sensor_WriteReg(OV8825_TRULY_GAIN_GL_ADDR, gain_g & 0x00ff);
	}

	if (ratio_b != 1)
	{
		gain_b = (uint16_t)(OV8825_TRULY_GAIN_DEFAULT_VALUE * ratio_b);
		Sensor_WriteReg(OV8825_TRULY_GAIN_BH_ADDR, gain_b >> 8);
		Sensor_WriteReg(OV8825_TRULY_GAIN_BL_ADDR, gain_b & 0x00ff);
	}

	SENSOR_PRINT("cmp_rg=%f, cmp_bg=%f\n", cmp_rg, cmp_bg);
	SENSOR_PRINT("ratio_r=%f, ratio_g=%f, ratio_b=%f\n", ratio_r, ratio_g, ratio_b);
	SENSOR_PRINT("gain_r=0x%x, gain_g=0x%x, gain_b=0x%x\n", gain_r, gain_g, gain_b);
	return 1;
}

#else //Use this if not support floating point values

#define OTP_MULTIPLE_FAC	10000
LOCAL uint32_t _ov8825_truly_otp_apply_wb(uint8_t golden_rg, uint8_t golden_bg)
{
	uint16_t gain_r = OV8825_TRULY_GAIN_DEFAULT_VALUE;
	uint16_t gain_g = OV8825_TRULY_GAIN_DEFAULT_VALUE;
	uint16_t gain_b = OV8825_TRULY_GAIN_DEFAULT_VALUE;

	uint16_t ratio_r, ratio_g, ratio_b;
	uint16_t cmp_rg, cmp_bg;

	if (!golden_rg || !golden_bg)
	{
		SENSOR_PRINT("golden_rg / golden_bg can not be zero\n");
		return 0;
	}

	// Calcualte R, G, B gain of current module from R/G, B/G of golden module
    // and R/G, B/G of current module
	cmp_rg = OTP_MULTIPLE_FAC * truly_rg_ratio / golden_rg;
	cmp_bg = OTP_MULTIPLE_FAC * truly_bg_ratio / golden_bg;

	if ((cmp_rg < 1 * OTP_MULTIPLE_FAC) && (cmp_bg < 1 * OTP_MULTIPLE_FAC))
	{
		// R/G < R/G golden, B/G < B/G golden
		ratio_g = 1 * OTP_MULTIPLE_FAC;
		ratio_r = 1 * OTP_MULTIPLE_FAC * OTP_MULTIPLE_FAC / cmp_rg;
		ratio_b = 1 * OTP_MULTIPLE_FAC * OTP_MULTIPLE_FAC / cmp_bg;
	}
	else if (cmp_rg > cmp_bg)
	{
		// R/G >= R/G golden, B/G < B/G golden
		// R/G >= R/G golden, B/G >= B/G golden
		ratio_r = 1 * OTP_MULTIPLE_FAC;
		ratio_g = cmp_rg;
		ratio_b = OTP_MULTIPLE_FAC * cmp_rg / cmp_bg;
	}
	else
	{
		// B/G >= B/G golden, R/G < R/G golden
		// B/G >= B/G golden, R/G >= R/G golden
		ratio_b = 1 * OTP_MULTIPLE_FAC;
		ratio_g = cmp_bg;
		ratio_r = OTP_MULTIPLE_FAC * cmp_bg / cmp_rg;
	}

	// write sensor wb gain to registers
	// 0x0400 = 1x gain
	if (ratio_r != 1 * OTP_MULTIPLE_FAC)
	{
		gain_r = OV8825_TRULY_GAIN_DEFAULT_VALUE * ratio_r / OTP_MULTIPLE_FAC;
		Sensor_WriteReg(OV8825_TRULY_GAIN_RH_ADDR, gain_r >> 8);
		Sensor_WriteReg(OV8825_TRULY_GAIN_RL_ADDR, gain_r & 0x00ff);
	}

	if (ratio_g != 1 * OTP_MULTIPLE_FAC)
	{
		gain_g = OV8825_TRULY_GAIN_DEFAULT_VALUE * ratio_g / OTP_MULTIPLE_FAC;
		Sensor_WriteReg(OV8825_TRULY_GAIN_GH_ADDR, gain_g >> 8);
		Sensor_WriteReg(OV8825_TRULY_GAIN_GL_ADDR, gain_g & 0x00ff);
	}

	if (ratio_b != 1 * OTP_MULTIPLE_FAC)
	{
		gain_b = OV8825_TRULY_GAIN_DEFAULT_VALUE * ratio_b / OTP_MULTIPLE_FAC;
		Sensor_WriteReg(OV8825_TRULY_GAIN_BH_ADDR, gain_b >> 8);
		Sensor_WriteReg(OV8825_TRULY_GAIN_BL_ADDR, gain_b & 0x00ff);
	}

	SENSOR_PRINT("cmp_rg=%d, cmp_bg=%d\n", cmp_rg, cmp_bg);
	SENSOR_PRINT("ratio_r=%d, ratio_g=%d, ratio_b=%d\n", ratio_r, ratio_g, ratio_b);
	SENSOR_PRINT("gain_r=0x%x, gain_g=0x%x, gain_b=0x%x\n", gain_r, gain_g, gain_b);
	return 1;
}
#endif /* SUPPORT_FLOATING */

/*******************************************************************************
* Function    :  _ov8825_truly_otp_update_wb
* Description :  Update white balance settings from OTP
* Parameters  :  [in] golden_rg : R/G of golden camera module
                 [in] golden_bg : B/G of golden camera module
* Return      :  1, success; 0, fail
*******************************************************************************/
LOCAL uint32_t _ov8825_truly_otp_update_wb(uint8_t golden_rg, uint8_t golden_bg)
{
//	SENSOR_PRINT("start wb update\n");

	if (_ov8825_truly_otp_read_wb_group(-1) != -1)
	{
		if (_ov8825_truly_otp_apply_wb(golden_rg, golden_bg) == 1)
		{
			SENSOR_PRINT("wb update finished\n");
			return 1;
		}
	}

//	SENSOR_PRINT("wb update failed\n");
	return 0;
}

/*******************************************************************************
* Function    :  _ov8825_truly_otp_check_lenc_group
* Description :  Check OTP Space Availability
* Parameters  :  [in] uint8_t index : index of otp group (0, 1, 2)
* Return      :  0, group index is empty
                 1, group index has invalid data
                 2, group index has valid data
                -1, group index error
*******************************************************************************/
LOCAL int8_t _ov8825_truly_otp_check_lenc_group(uint8_t index)
{
	uint16_t otp_addr = OV8825_TRULY_OTP_LENC_GROUP_ADDR;
	uint8_t  flag;
	uint8_t  bank;

	if (index > 2)
	{
		SENSOR_PRINT("OTP input lenc group index %d error\n", index);
		return -1;
	}
		
	// select bank: index*2 + 1
	bank = 0x08 | (index*2 + 1);
	Sensor_WriteReg(OV8825_TRULY_OTP_BANK_ADDR, bank);

	_ov8825_truly_otp_read(otp_addr, &flag);
	_ov8825_truly_otp_clear();

	flag = flag & 0xc0;

	// Check all bytes of a group. If all bytes are '0', then the group is empty. 
	// Check from group 1 to group 2, then group 3.
	if (!flag)
	{
		SENSOR_PRINT("lenc group %d is empty\n", index);
		return 0;
	}
	else if (flag == 0x40)
	{
		SENSOR_PRINT("lenc group %d has valid data\n", index);
		return 2;
	}
	else
	{
		SENSOR_PRINT("lenc group %d has invalid data\n", index);
		return 1;
	}
}

/*******************************************************************************
* Function    :  _ov8825_truly_otp_read_lenc_group
* Description :  Read group value and store it in OTP Struct 
* Parameters  :  [in] int index : index of otp group (0, 1, 2)
* Return      :  group index (0, 1, 2)
                 -1, error
*******************************************************************************/
LOCAL int8_t _ov8825_truly_otp_read_lenc_group(int index)
{
	uint16_t otp_addr;
	uint8_t bank;
	uint8_t i;

	if (index == -1)
	{
		// Check first OTP with valid data
		for (index=0; index<3; index++)
		{
			if (_ov8825_truly_otp_check_lenc_group(index) == 2)
			{
				SENSOR_PRINT("read lenc from group %d\n", index);
				break;
			}
		}

		if (index > 2)
		{
			SENSOR_PRINT("no group has valid data\n");
			return -1;
		}
	}
	else
	{
		if (_ov8825_truly_otp_check_lenc_group(index) != 2) 
		{
			SENSOR_PRINT("read lenc from group %d failed\n", index);
			return -1;
		}
	}

	// select bank: index*2 + 1
	bank = 0x08 | (index*2 + 1);
	Sensor_WriteReg(OV8825_TRULY_OTP_BANK_ADDR, bank);

	otp_addr = OV8825_TRULY_OTP_LENC_GROUP_ADDR+1;

	_ov8825_truly_otp_read_enable();
	for (i=0; i<31; i++) 
	{
		truly_otp_lenc_data[i] = Sensor_ReadReg(otp_addr);
		otp_addr++;
	}
	_ov8825_truly_otp_read_disable();
	_ov8825_truly_otp_clear();

	// select next bank
	bank++;
	Sensor_WriteReg(OV8825_TRULY_OTP_BANK_ADDR, bank);

	otp_addr = OV8825_TRULY_OTP_LENC_GROUP_ADDR;

	_ov8825_truly_otp_read_enable();
	for (i=31; i<62; i++) 
	{
		truly_otp_lenc_data[i] = Sensor_ReadReg(otp_addr);
		otp_addr++;
	}
	_ov8825_truly_otp_read_disable();
	_ov8825_truly_otp_clear();
	
	SENSOR_PRINT("read lenc finished\n");
	return index;
}

/*******************************************************************************
* Function    :  _ov8825_truly_otp_apply_lenc
* Description :  Apply lens correction setting to module
* Parameters  :  none
* Return      :  none
*******************************************************************************/
LOCAL void _ov8825_truly_otp_apply_lenc(void)
{
	// write lens correction setting to registers
	//SENSOR_PRINT("apply lenc setting\n");

	uint8_t data = truly_otp_lenc_data[0] | 0x80;
	uint8_t i;

	Sensor_WriteReg(OV8825_TRULY_LENC_START_ADDR, data);

	for (i=1; i<OV8825_TRULY_LENC_REG_SIZE; i++)
	{
		Sensor_WriteReg(OV8825_TRULY_LENC_START_ADDR+i, truly_otp_lenc_data[i]);
		//SENSOR_PRINT("0x%x, 0x%x\n", OV8825_TRULY_LENC_START_ADDR+i, truly_otp_lenc_data[i]);
	}
}

/*******************************************************************************
* Function    :  _ov8825_truly_otp_update_lenc
* Description :  Get lens correction setting from otp, then apply to module
* Parameters  :  none
* Return      :  1, success; 0, fail
*******************************************************************************/
LOCAL uint32_t _ov8825_truly_otp_update_lenc(void) 
{
	SENSOR_PRINT("start lenc update\n");

	if (_ov8825_truly_otp_read_lenc_group(-1) != -1)
	{
		_ov8825_truly_otp_apply_lenc();
		SENSOR_PRINT("lenc update finished\n");
		return 1;
	}

	SENSOR_PRINT("lenc update failed\n");
	return 0;
}

LOCAL uint32_t _ov8825_truly_Identify_otp(void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint16_t otp_addr;
	uint8_t mid;
	uint8_t index;
	
	SENSOR_PRINT("SENSOR_ov8825: _ov8825_truly_Identify_otp");

	for (index=0; index<3; index++)
	{
		if (_ov8825_truly_otp_check_wb_group(index) == 2)
		{
			SENSOR_PRINT("SENSOR_ov8825:read wb from group %d\n", index);
			break;
		}
	}

	if(index < 3) {
		otp_addr = OV8825_TRULY_OTP_WB_GROUP_ADDR + index * OV8825_TRULY_OTP_WB_GROUP_SIZE;

		// select bank 0
		rtn = Sensor_WriteReg(OV8825_TRULY_OTP_BANK_ADDR, 0x08);
		if (rtn) {
			SENSOR_PRINT("Another sensor module");
			return rtn;
		}

		_ov8825_truly_otp_read(otp_addr, &mid);
		mid = mid&0x7f;
		
		SENSOR_PRINT("SENSOR_ov8825:read ov8825 otp module_id = %x \n", mid);

		if (OV8825_TRULY_OTP_MID == mid) {
			//SENSOR_PRINT("SENSOR_OV8825: This is truly module!!\n");
			rtn = SENSOR_SUCCESS;
		} else {
			SENSOR_PRINT("SENSOR_OV8825: check module id faided!!\n");
			rtn = SENSOR_FAIL;
		}
	} else {
		/* no valid wb OTP data */
		SENSOR_PRINT("SENSOR_ov8825:ov8825_check_otp_module_id no valid wb OTP data\n");
		rtn = SENSOR_FAIL;
	}

	if(SENSOR_SUCCESS == rtn){
		for (index=0; index<3; index++)
		{
			if (_ov8825_truly_otp_check_lenc_group(index) == 2)
			{
				SENSOR_PRINT("SENSOR_ov8825:read lenc from group %d\n", index);
				SENSOR_PRINT("SENSOR_OV8825: This is truly module!!\n");
				break;
			}
		}
		if(index >= 3) {
			/* no valid lenc OTP data */
			SENSOR_PRINT("SENSOR_ov8825:ov8825_check_otp_module_id no valid lenc OTP data\n");
			rtn = SENSOR_FAIL;
		}

	}

	return rtn;
}

LOCAL uint32_t  _ov8825_truly_update_otp(void* param_ptr)
{
	uint16_t temp;

	SENSOR_PRINT("SENSOR_OV8825: _ov8825_truly_update_otp");

	temp = Sensor_ReadReg(0x5001);
	temp = 0x01 | temp;
	Sensor_WriteReg(0x5001, temp);

	temp = Sensor_ReadReg(0x3406);
	temp = 0x01 | temp;
	Sensor_WriteReg(0x3406, temp);

	temp = Sensor_ReadReg(0x5000);
	temp = 0x80 | temp;
	Sensor_WriteReg(0x5000, temp);

	if (0x00 == _ov8825_truly_otp_update_wb(OV8825_TRULY_RG_RATIO_TYPICAL,OV8825_TRULY_BG_RATIO_TYPICAL)) {
		SENSOR_PRINT("SENSOR_OV8825: otp wb update error");
	}

	if (0x00 == _ov8825_truly_otp_update_lenc()) {
		SENSOR_PRINT("SENSOR_OV8825: otp lnc update error");
	}

	return 0;
}

