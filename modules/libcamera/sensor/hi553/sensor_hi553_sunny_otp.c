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

#define OTP_AUTO_LOAD_LSC_hi553_sunny
/*OTP address register*/
#define hi553_sunny_OTP_ADDR_H            0x010a
#define hi553_sunny_OTP_ADDR_L            0x010b
/*Command register*/
#define hi553_sunny_OTP_CMD_ADDR          0x0102
/*OTP read data register*/
#define hi553_sunny_OTP_RDATA_ADDR        0x0108
/*OTP write data register*/
#define hi553_sunny_OTP_WDATA_ADDR        0x0106

/*Module Info flag register*/
#define hi553_sunny_OTP_MID_FLAG_ADDR			0x0501
#define hi553_sunny_OTP_MID_GROUP_BASE_ADDR     0x0502
#define hi553_sunny_OTP_MID_REG_SIZE			17

/*Lens group*/
#define hi553_sunny_OTP_LENC_GROUP_FLAG_ADDR    0xffff
#define hi553_sunny_OTP_LENC_BASE_ADDR			0xffff
#define hi553_sunny_OTP_LENC_REG_SIZE			62

/*awb group*/
#define hi553_sunny_OTP_WB_GROUP_FLAG_ADDR     0x0535
#define hi553_sunny_OTP_WB_GROUP_BASE_ADDR     0x0536
#define hi553_sunny_OTP_WB_GROUP_SIZE		   30

#define hi553_sunny_GAIN_GRH_ADDR          0x0126
#define hi553_sunny_GAIN_GRL_ADDR          0x0127

#define hi553_sunny_GAIN_GBH_ADDR          0x0128
#define hi553_sunny_GAIN_GBL_ADDR          0x0129

#define hi553_sunny_GAIN_RH_ADDR          0x012A
#define hi553_sunny_GAIN_RL_ADDR          0x012B

#define hi553_sunny_GAIN_BH_ADDR          0x012C
#define hi553_sunny_GAIN_BL_ADDR          0x012D

#define hi553_sunny_GAIN_DEFAULT_VALUE    0x400 // 1x gain

#define hi553_sunny_OTP_MID               0x08
#define  hi553_SUNNY_RG_RATIO_TYPICAL     (0x015c)
#define  hi553_SUNNY_BG_RATIO_TYPICAL     (0x012e)

/* Enable OTP read function */
LOCAL void _hi553_sunny_otp_read_enable(void)
{
	Sensor_WriteReg_16bits(0x0a02,0x01);
	Sensor_WriteReg_16bits(0x0a00,0x00);
	usleep(100*1000);
	Sensor_WriteReg_16bits(0x0f02,0x00);
	Sensor_WriteReg_16bits(0x011a,0x01);
	Sensor_WriteReg_16bits(0x011b,0x09);
	Sensor_WriteReg_16bits(0x0d04,0x01);
	Sensor_WriteReg_16bits(0x0d00,0x07);
	Sensor_WriteReg_16bits(0x003f,0x10);
	Sensor_WriteReg_16bits(0x0a00,0x01);
}

/* Disable OTP read function */
LOCAL void _hi553_sunny_otp_read_disable(void)
{
	Sensor_WriteReg_16bits(0x0a00,0x00);
	usleep(100*1000);
	Sensor_WriteReg_16bits(0x003f,0x00);
	Sensor_WriteReg_16bits(0x0a00,0x01);
}
/*******************************************************************************
* Function    :  _hi553_sunny_otp_read_baseinfo
* Description :  read module base data:moduel id，Lenc id ...
* Parameters  :  otp_addr:　reg address
				 otp_data: data pointer to read
* Return      :  none
*******************************************************************************/
LOCAL void _hi553_sunny_otp_read_baseinfo(uint16_t otp_addr, uint8_t* otp_data)
{
	SENSOR_PRINT("otp_addr = 0x%x\n",otp_addr);
    Sensor_WriteReg_16bits(hi553_sunny_OTP_ADDR_H, (otp_addr>>8)&0xff);
	Sensor_WriteReg_16bits(hi553_sunny_OTP_ADDR_L, otp_addr&0xff);
    Sensor_WriteReg_16bits(hi553_sunny_OTP_CMD_ADDR,0x01);
	*otp_data = (Sensor_ReadReg(hi553_sunny_OTP_RDATA_ADDR)>>8) & 0xff;
    usleep(4*1000);
	SENSOR_PRINT("otp_data = 0x%x\n", *otp_data);//log
}
/*******************************************************************************
* Function    :  _hi553_sunny_otp_read_wb
* Description :  read write balance data
* Parameters  :  otp_addr:　reg address
				 otp_data: data pointer to read
* Return      :  none
*******************************************************************************/
LOCAL void _hi553_sunny_otp_read_wb(uint16_t otp_addr, uint8_t* otp_data)
{
	SENSOR_PRINT("otp_addr = 0x%x\n",otp_addr);
    Sensor_WriteReg_16bits(hi553_sunny_OTP_ADDR_H, (otp_addr>>8)&0xff);
	Sensor_WriteReg_16bits(hi553_sunny_OTP_ADDR_L, otp_addr&0xff);
    Sensor_WriteReg_16bits(hi553_sunny_OTP_CMD_ADDR,0x01);
	*otp_data = (Sensor_ReadReg(hi553_sunny_OTP_RDATA_ADDR)>>8)&0xff;
    usleep(4*1000);
	SENSOR_PRINT("otp_data = 0x%x\n",*otp_data);//log
}

/*******************************************************************************
* Function    :  _hi553_sunny_otp_check_wb_group
* Description :  Check OTP data is valid or not
* Parameters  :  [in]:index pointer
* Return      : SENSOR_FAIL: has no right group
				SENSOR_SUCCESS:has checked the right group
*******************************************************************************/
LOCAL int8_t _hi553_sunny_otp_check_wb_group(uint8_t * index)
{
	uint16_t otp_addr = NULL;
	uint8_t  flag;
    _hi553_sunny_otp_read_wb(hi553_sunny_OTP_WB_GROUP_FLAG_ADDR,&flag);
	SENSOR_PRINT(" flag %x\n", flag);
	if (0x01 == flag)
    {
		*index = 1;
	    SENSOR_PRINT("awb group1 is valid data\n");
    }
	else if (0x13 == flag)
	{
		*index = 2;
	    SENSOR_PRINT("awb group2 is valid data\n");
	}
	else if (0x37 == flag)
	{
		*index = 3;
	    SENSOR_PRINT("awb group3 is valid data\n");
	}
	else
	{
		SENSOR_PRINT("awb do not has valid data\n");
		return SENSOR_FAIL;
	}
	return SENSOR_SUCCESS;
}

/*******************************************************************************
* Function    :  _hi553_sunny_otp_read_wb_group
* Description :  Read group value and store it in OTP Struct
* Parameters  :  [in] param_ptr: otp data struct pointer
* Return      :  SENSOR_SUCCESS:read wb data finish!!
                 -1, error
*******************************************************************************/
LOCAL int8_t _hi553_sunny_otp_read_wb_group(void *param_ptr)
{
	uint16_t otp_addr;
	uint8_t mid;
	uint8_t index, i=0;
	uint8_t temp[4]={0x00};
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;
	// Check first OTP with valid data
	if (_hi553_sunny_otp_check_wb_group(&index) == SENSOR_SUCCESS)
	{
        SENSOR_PRINT("read wb from group %d\n", index);
	}
    else
    {
        SENSOR_PRINT("no group has valid data\n");
		return -1;
    }
	for(i=0; i<4; i++)
    {
        otp_addr = hi553_sunny_OTP_WB_GROUP_BASE_ADDR + (index-1)*hi553_sunny_OTP_WB_GROUP_SIZE+i;
	    _hi553_sunny_otp_read_wb(otp_addr,&temp[i]);
    }
	otp_info->rg_ratio_current=(temp[0]<<8 | temp[1]);
	otp_info->bg_ratio_current=(temp[2]<<8 | temp[3]);
	SENSOR_PRINT("rg_ratio_current = 0x%x, bg_ratio_current = 0x%x\n", otp_info->rg_ratio_current, otp_info->bg_ratio_current);
	SENSOR_PRINT("read wb finished\n");
	return SENSOR_SUCCESS;
}

#ifdef SUPPORT_FLOATING //Use this if support floating point values
/*******************************************************************************
* Function    :  _hi553_sunny_otp_apply_wb
* Description :  Calcualte and apply R, G, B gain to module
* Parameters  :  [in] param_ptr: otp data struct pointer
* Return      :  1, success; 0, fail
*******************************************************************************/
LOCAL uint32_t _hi553_sunny_otp_apply_wb(void *param_ptr)
{
	uint16_t gain_r = 1;
	uint16_t gain_g = 1;
	uint16_t gain_b = 1;

	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;

	if (!hi553_SUNNY_RG_RATIO_TYPICAL || !hi553_SUNNY_BG_RATIO_TYPICAL)
	{
		SENSOR_PRINT("golden_rg / golden_bg can not be zero\n");
		return 0;
	}

	// Calcualte R, G, B gain of current module from R/G, B/G of golden module
    // and R/G, B/G of current module
	gain_r = hi553_SUNNY_RG_RATIO_TYPICAL / otp_info->rg_ratio_current ;
	gain_b = hi553_SUNNY_BG_RATIO_TYPICAL / otp_info->bg_ratio_current;

    if( gain_r < gain_b )
    {
        if( gain_r < 1)
        {
            gain_b = gain_b / gain_r;
            gain_g = gain_g / gain_r;
            gain_r = 1;
        }
    }
    else
    {
        if( gain_b < 1)
        {
            gain_r = gain_r / gain_b;
            gain_g = gain_g / gain_b;
            gain_b = 1;
        }
    }
	// write sensor wb gain to registers
	// 0x0400 = 1x gain
	gain_g = (uint16_t)(hi553_sunny_GAIN_DEFAULT_VALUE * gain_g);
	Sensor_WriteReg(hi553_sunny_GAIN_GRH_ADDR, gain_g);
	Sensor_WriteReg(hi553_sunny_GAIN_GBH_ADDR, gain_g);

	gain_r = (uint16_t)(hi553_sunny_GAIN_DEFAULT_VALUE * gain_r);
	Sensor_WriteReg(hi553_sunny_GAIN_RH_ADDR, gain_r);

	gain_b = (uint16_t)(hi553_sunny_GAIN_DEFAULT_VALUE * gain_b);
	Sensor_WriteReg(hi553_sunny_GAIN_BH_ADDR, gain_b);

	SENSOR_PRINT("gain_r=0x%x, gain_g=0x%x, gain_b=0x%x\n", gain_r, gain_g, gain_b);
	return 1;
}

#else //Use this if not support floating point values

#define OTP_MULTIPLE_FAC	10000
LOCAL uint32_t _hi553_sunny_otp_apply_wb(void *param_ptr)
{
	uint16_t gain_r = 1;
	uint16_t gain_g = 1;
	uint16_t gain_b = 1;

	uint16_t ratio_r, ratio_g, ratio_b;
	uint16_t cmp_rg, cmp_bg;
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;

    if (!hi553_SUNNY_RG_RATIO_TYPICAL || !hi553_SUNNY_BG_RATIO_TYPICAL)
	{
		SENSOR_PRINT("golden_rg / golden_bg can not be zero\n");
		return 0;
	}

	// Calcualte R, G, B gain of current module from R/G, B/G of golden module
    // and R/G, B/G of current module
    gain_r = OTP_MULTIPLE_FAC * hi553_SUNNY_RG_RATIO_TYPICAL / otp_info->rg_ratio_current ;
	gain_b = OTP_MULTIPLE_FAC * hi553_SUNNY_BG_RATIO_TYPICAL / otp_info->bg_ratio_current;
    gain_g = OTP_MULTIPLE_FAC;
    if( gain_r < gain_b )
    {
        if( gain_r < 1)
        {
            gain_b = OTP_MULTIPLE_FAC * gain_b / gain_r;
            gain_g = OTP_MULTIPLE_FAC * gain_g / gain_r;
            gain_r = OTP_MULTIPLE_FAC * 1;
        }
    }
    else
    {
        if( gain_b < 1)
        {
            gain_r = OTP_MULTIPLE_FAC * gain_r / gain_b;
            gain_g = OTP_MULTIPLE_FAC * gain_g / gain_b;
            gain_b = OTP_MULTIPLE_FAC * 1;
        }
	}
	// write sensor wb gain to registers
	// 0x0400 = 1x gain
	gain_r = hi553_sunny_GAIN_DEFAULT_VALUE * gain_r / OTP_MULTIPLE_FAC;
	Sensor_WriteReg(hi553_sunny_GAIN_RH_ADDR, gain_r);

	gain_g = hi553_sunny_GAIN_DEFAULT_VALUE * gain_g / OTP_MULTIPLE_FAC;
	Sensor_WriteReg(hi553_sunny_GAIN_GRH_ADDR, gain_g);
	Sensor_WriteReg(hi553_sunny_GAIN_GBH_ADDR, gain_b);

	gain_b = hi553_sunny_GAIN_DEFAULT_VALUE * gain_b / OTP_MULTIPLE_FAC;
	Sensor_WriteReg(hi553_sunny_GAIN_BH_ADDR, gain_b);

	SENSOR_PRINT("gain_r=0x%x, gain_g=0x%x, gain_b=0x%x\n", gain_r, gain_g, gain_b);
	return 1;
}
#endif /* SUPPORT_FLOATING */


/*******************************************************************************
* Function    :  _hi553_sunny_otp_check_lenc_group
* Description :  Check OTP Space Availability
* Parameters  :  [in] uint8_t index : index of otp group (0, 1, 2)
* Return      :  0, group index is empty
                 1, group index has invalid data
                 2, group index has valid data
                -1, group index error
*******************************************************************************/
LOCAL int8_t _hi553_sunny_otp_check_lenc_group(uint8_t * index)
{
	uint16_t otp_addr = 0x00;
	uint8_t  flag;
	uint8_t  bank;
	uint8_t  i = 0;
	//_hi553_sunny_otp_read(ohi553_sunny_OTP_WB_GROUP_FLAG_ADDR, &flag);

	if (0x01 == flag)
    {
		*index = 0;
	    SENSOR_PRINT("lenc group1 is valid data\n");
    }
	else if (0x13 == flag)
	{
		*index = 1;
	    SENSOR_PRINT("lenc group2 is valid data\n");
	}
	else if (0x37 == flag)
	{
		*index = 2;
	    SENSOR_PRINT("lenc group3 is valid data\n");
	}
	else
	{
		SENSOR_PRINT("lenc do not has valid data\n");
		return -1;
	}
	return 0;
}

/*******************************************************************************
* Function    :  _hi553_sunny_otp_read_lenc_group
* Description :  Read group value and store it in OTP Struct
* Parameters  :  param_pt otp struct var
* Return      :  group index (0, 1, 2)
                 -1, error
*******************************************************************************/
LOCAL int8_t _hi553_sunny_otp_read_lenc_group(void *param_pt)
{
    return 1;
}

/*******************************************************************************
* Function    :  _hi553_sunny_otp_apply_lenc
* Description :  Apply lens correction setting to module
* Parameters  :  none
* Return      :  none
*******************************************************************************/
LOCAL int8_t _hi553_sunny_otp_apply_lenc(void* param_ptr)
{
    return 1;
}

/* new model*/
static uint32_t hi553_sunny_read_otp_info(void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
    uint8_t index=0;
    uint8_t i,flag,temp[6];
    uint16_t otp_addr;
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;
	otp_info->rg_ratio_typical=hi553_SUNNY_RG_RATIO_TYPICAL;
	otp_info->bg_ratio_typical=hi553_SUNNY_BG_RATIO_TYPICAL;

	_hi553_sunny_otp_read_enable();

	/*check Module Info*/
	_hi553_sunny_otp_read_baseinfo(hi553_sunny_OTP_MID_FLAG_ADDR, &flag);  //read flag
	otp_info->flag = flag;
	SENSOR_PRINT("flag=0x%x",otp_info->flag);
	if (0x01 == flag)
    {
		index = 0;
	    SENSOR_PRINT("Module info group1 is valid data\n");
    }
	else if (0x13 == flag)
	{
		index = 1;
	    SENSOR_PRINT("Module info group2 is valid data\n");
	}
	else if (0x37 == flag)
	{
		index = 2;
	    SENSOR_PRINT("Module info group3 is valid data\n");
	}
	else
	{
		SENSOR_PRINT("Module info do not has valid data\n");
        return 1;
	}
    /*read base info*/
    for(i=0; i<5; i++)
    {
        otp_addr = hi553_sunny_OTP_MID_GROUP_BASE_ADDR + index*hi553_sunny_OTP_MID_REG_SIZE+i;
		_hi553_sunny_otp_read_baseinfo(otp_addr,&temp[i]);
    }
    otp_info->module_id = (uint16_t)temp[0];
    otp_info->year = (uint16_t)temp[2];
    otp_info->month = (uint16_t)temp[3];
    otp_info->day = (uint16_t)temp[4];

	SENSOR_PRINT("module_id = 0x%x", otp_info->module_id);
	if (hi553_sunny_OTP_MID == otp_info->module_id)
	{
		SENSOR_PRINT("SENSOR_hi553: This is sunny module!!\n");
		rtn = SENSOR_SUCCESS;
	}
	else
	{
		SENSOR_PRINT("SENSOR_hi553: check module id faided!!\n");
		return SENSOR_FAIL;
	}

    _hi553_sunny_otp_read_wb_group(otp_info);
    //_hi553_sunny_otp_read_lenc_group(otp_info);
	/*print otp information*/
	SENSOR_PRINT("flag=0x%x",otp_info->flag);
	SENSOR_PRINT("module_id=0x%x",otp_info->module_id);
	//SENSOR_PRINT("lens_id=0x%x",otp_info->lens_id);
	//SENSOR_PRINT("vcm_id=0x%x",otp_info->vcm_id);
	//SENSOR_PRINT("vcm_id=0x%x",otp_info->vcm_id);
	//SENSOR_PRINT("vcm_driver_id=0x%x",otp_info->vcm_driver_id);
	SENSOR_PRINT("data=%d-%d-%d",otp_info->year,otp_info->month,otp_info->day);
	SENSOR_PRINT("rg_ratio_current=0x%x",otp_info->rg_ratio_current);
	SENSOR_PRINT("bg_ratio_current=0x%x",otp_info->bg_ratio_current);
	SENSOR_PRINT("rg_ratio_typical=0x%x",otp_info->rg_ratio_typical);
	SENSOR_PRINT("bg_ratio_typical=0x%x",otp_info->bg_ratio_typical);
	//SENSOR_PRINT("r_current=0x%x",otp_info->r_current);
	//SENSOR_PRINT("g_current=0x%x",otp_info->g_current);
	//SENSOR_PRINT("b_current=0x%x",otp_info->b_current);
	//SENSOR_PRINT("r_typical=0x%x",otp_info->r_typical);
	//SENSOR_PRINT("g_typical=0x%x",otp_info->g_typical);
	//SENSOR_PRINT("b_typical=0x%x",otp_info->b_typical);
	//SENSOR_PRINT("vcm_dac_start=0x%x",otp_info->vcm_dac_start);
	//SENSOR_PRINT("vcm_dac_inifity=0x%x",otp_info->vcm_dac_inifity);
	//SENSOR_PRINT("vcm_dac_macro=0x%x",otp_info->vcm_dac_macro);

	return rtn;
}
/*******************************************************************************
* Function    :  hi553_sunny_otp_update_awb
* Description :  Update white balance settings from OTP
* Parameters  :  [in] param_ptr otp struct pointer
* Return      :  1, success; 0, fail
*******************************************************************************/
static uint32_t hi553_sunny_update_awb(void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;

	/*TODO*/

	if (_hi553_sunny_otp_apply_wb(param_ptr) == 1)
	{
		SENSOR_PRINT("wb update finished\n");
		return 0;
	}
	SENSOR_PRINT("wb update failed\n");
	return 1;
}

#ifndef OTP_AUTO_LOAD_LSC_hi553_sunny
/*******************************************************************************
* Function    :  _hi553_sunny_otp_update_lsc
* Description :  Get lens correction setting from otp, then apply to module
* Parameters  :  none
* Return      :  1, success; 0, fail
*******************************************************************************/
static uint32_t hi553_sunny_update_lsc(void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;

	/*TODO*/

	SENSOR_PRINT("start lenc update\n");

	if(_hi553_sunny_otp_apply_lenc(otp_info) == 1)
    {
	    SENSOR_PRINT("lenc update finished\n");
	    return 1;
    }

	SENSOR_PRINT("lenc update failed\n");
	return rtn;
}

#endif
static uint32_t hi553_sunny_update_otp(void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct otp_info_t *otp_info=(struct otp_info_t *)param_ptr;

	rtn=hi553_sunny_update_awb(param_ptr);
	if(rtn!=SENSOR_SUCCESS)
	{
		SENSOR_PRINT_ERR("OTP awb appliy error!");
		return rtn;
	}

	#ifndef OTP_AUTO_LOAD_LSC_hi553_sunny

	rtn=hi553_sunny_update_lsc(param_ptr);
	if(rtn!=SENSOR_SUCCESS)
	{
		SENSOR_PRINT_ERR("OTP lsc appliy error!");
		return rtn;
	}
	#endif

	return rtn;
}

static uint32_t hi553_sunny_identify_otp(void *param_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;

	rtn=hi553_sunny_read_otp_info(param_ptr);
	SENSOR_PRINT("rtn=%d",rtn);

	return rtn;
}
