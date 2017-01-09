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
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"
#include "sensor_hi542_raw_param.c"

#define hi542_I2C_ADDR_W        0x20
#define hi542_I2C_ADDR_R         0x20
#define DW9714A_VCM_SLAVE_ADDR 	(0x18>>1)  //0x0c

#define HI542_PREVIEW_LINE_PIXEL 100
#define HI542_CAPTURE_LINE_PIXEL 200

#define HI542_RAW_PARAM_COM  0x0000

LOCAL uint32_t _hi542_GetResolutionTrimTab(uint32_t param);
LOCAL uint32_t _hi542_PowerOn(uint32_t power_on);
LOCAL uint32_t _hi542_Identify(uint32_t param);
LOCAL uint32_t _hi542_BeforeSnapshot(uint32_t param);
LOCAL uint32_t _hi542_after_snapshot(uint32_t param);
LOCAL uint32_t _hi542_StreamOn(uint32_t param);
LOCAL uint32_t _hi542_StreamOff(uint32_t param);
LOCAL uint32_t _hi542_write_exposure(uint32_t param);
LOCAL uint32_t _hi542_write_gain(uint32_t param);
LOCAL uint32_t _hi542_write_af(uint32_t param);
LOCAL uint32_t _hi542_ReadGain(uint32_t*  gain_ptr);
LOCAL uint32_t _hi542_SetEV(uint32_t param);
LOCAL uint32_t _hi542_ExtFunc(uint32_t ctl_param);
LOCAL uint32_t _DW9714A_SRCInit(uint32_t mode);
LOCAL uint32_t _hi542_DWInit(uint32_t param);
LOCAL uint32_t _hi542_com_Identify_otp(void* param_ptr);
LOCAL uint32_t _hi542_cfg_otp(uint32_t  param);

LOCAL const struct raw_param_info_tab s_hi542_raw_param_tab[]={
	{HI542_RAW_PARAM_COM, &s_hi542_mipi_raw_info, _hi542_com_Identify_otp, NULL},
	{RAW_INFO_END_ID, PNULL, PNULL, PNULL}
};

struct sensor_raw_info* s_hi542_mipi_raw_info_ptr=NULL;

static uint32_t g_hi542_module_id = 0;

static uint32_t s_hi542_gain = 0;

LOCAL const SENSOR_REG_T hi542_com_mipi_raw[] = {
	//SW reset, LDO setting
	{0x0001, 0x02},/* SW reset *///PWRCTLA//B[1]=soft reset, B[0]=sleep mode(0:normal operation, 1:sleep mode enable)
	{0x0001, 0x01},/* SW sleep *///PWRCTLA//B[1]=soft reset, B[0]=sleep mode(0:normal operation, 1:sleep mode enable)
	{0x03d4, 0x18},//[5:4]LDO level control
	{0x03D2, 0xAD},//PLL reset 
	{0x0616, 0x00},//D-PHY reset 
	{0x0616, 0x01},//D-PHY reset disable
	{0x03D2, 0xAC},//PLL reset disable 
	{0x03D0, 0xe9},
	{0x03D1, 0x75},//thomaszhang  for 24MHZ  {0x03D1, 0x74},//for 20MHz 
	{0x0800, 0x07},//EMI disable
	{0x0801, 0x08},
	{0x0802, 0x02},

	{0x0011, 0x94}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0020, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0021, 0x08}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0022, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0023, 0x18}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0038, 0x02}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0039, 0x2C}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x003C, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x003D, 0x0C}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x003E, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x003F, 0x0C}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0040, 0x00}, //Hblank H
	{0x0041, 0x35}, //2E} Hblank L
	{0x0042, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0043, 0x11}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0045, 0x07}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0046, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0047, 0xD0}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0050, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0052, 0x10}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0053, 0x10}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0054, 0x10}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0055, 0x08}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0056, 0x80}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0057, 0x08}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0058, 0x08}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0059, 0x08}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x005A, 0x08}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x005B, 0x02}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0070, 0x00}, //03}, EMI OFF
	{0x0081, 0x01},//09}, //0B}, BLC scheme
	{0x0082, 0x23}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0083, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0085, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0086, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x008C, 0x02}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x00A0, 0x0f},//0C},//0B}, RAMP DC OFFSET
	{0x00A1, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x00A2, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x00A3, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x00A4, 0xFF}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x00A5, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x00A6, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x00A8, 0x7F}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x00A9, 0x7F}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x00AA, 0x7F}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x00B4, 0x00}, //08}, BLC offset
	{0x00B5, 0x00}, //08},
	{0x00B6, 0x02}, //07},
	{0x00B7, 0x01}, //07},
	{0x00D4, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x00D5, 0xaa},//a9}, RAMP T1
	{0x00D6, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x00D7, 0xc9}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x00D8, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x00D9, 0x59}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x00DA, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x00DB, 0xb0}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x00DC, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x00DD, 0xc9},//c5}, /*rp_rst_flg_on1*/
	{0x011C, 0x1F}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x011D, 0xFF}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x011E, 0xFF}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x011F, 0xFF}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x012A, 0xFF}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x012B, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0129, 0x40}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0210, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0212, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0213, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0216, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0219, 0x33},//66}, Pixel bias
	{0x021B, 0x55}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x021C, 0x85}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x021D, 0xFF}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x021E, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x021F, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0220, 0x02}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0221, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0222, 0xA0}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0227, 0x0A}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0228, 0x5C}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0229, 0x2D}, //01},//41},//00},//2C}, RAMP swing range jwryu120120
	{0x022A, 0x04}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x022C, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x022D, 0x23}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0237, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0238, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0239, 0xA5}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x023A, 0x20}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x023B, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x023C, 0x22}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x023F, 0x80}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0240, 0x04}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0241, 0x07}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0242, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0243, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0244, 0x80}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0245, 0xE0}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0246, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0247, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x024A, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x024B, 0x14}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x024D, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x024E, 0x03}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x024F, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0250, 0x53}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0251, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0252, 0x07}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0253, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0254, 0x4F}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0255, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0256, 0x07}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0257, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0258, 0x4F}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0259, 0x0C}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x025A, 0x0C}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x025B, 0x0C}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x026C, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x026D, 0x09}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x026E, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x026F, 0x4B}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0270, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0271, 0x09}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0272, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0273, 0x4B}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0274, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0275, 0x09}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0276, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0277, 0x4B}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0278, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0279, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x027A, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x027B, 0x55}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x027C, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x027D, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x027E, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x027F, 0x5E}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0280, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0281, 0x03}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0282, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0283, 0x45}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0284, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0285, 0x03}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0286, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0287, 0x45}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0288, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0289, 0x5c}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x028A, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x028B, 0x60}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x02A0, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x02A1, 0xe0}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x02A2, 0x02}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x02A3, 0x22}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x02A4, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x02A5, 0x5C}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x02A6, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x02A7, 0x60}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x02A8, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x02A9, 0x5C}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x02AA, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x02AB, 0x60}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x02D2, 0x0F}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x02DB, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x02DC, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x02DD, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x02DE, 0x0C}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x02DF, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x02E0, 0x04}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x02E1, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x02E2, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x02E3, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x02E4, 0x0F}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x02F0, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x02F1, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0310, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0311, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0312, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0313, 0x5A}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0314, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0315, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0316, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0317, 0x5A}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0318, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0319, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x031A, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x031B, 0x2F}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x031C, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x031D, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x031E, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x031F, 0x2F}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0320, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0321, 0xAB}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0322, 0x02}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0323, 0x55}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0324, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0325, 0xAB}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0326, 0x02}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0327, 0x55}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0328, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0329, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x032A, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x032B, 0x10}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x032C, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x032D, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x032E, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x032F, 0x10}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0330, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0331, 0x02}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0332, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0333, 0x2e}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0334, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0335, 0x02}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0336, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0337, 0x2e}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0358, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0359, 0x46}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x035A, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x035B, 0x59}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x035C, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x035D, 0x46}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x035E, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x035F, 0x59}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0360, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0361, 0x46}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0362, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0363, 0xa4}, //a2}, Black sun
	{0x0364, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0365, 0x46}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0366, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0367, 0xa4}, //a2}, Black sun
	{0x0368, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0369, 0x46}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x036A, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x036B, 0xa6},//a9}, S2 off
	{0x036C, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x036D, 0x46}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x036E, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x036F, 0xa6},//a9}, S2 off
	{0x0370, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0371, 0xb0}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0372, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0373, 0x59}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0374, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0375, 0xb0}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0376, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0377, 0x59}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0378, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0379, 0x45}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x037A, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x037B, 0xAA}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x037C, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x037D, 0x99}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x037E, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x037F, 0xAE}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0380, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0381, 0xB1}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0382, 0x02}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0383, 0x56}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0384, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0385, 0x6D}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0386, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0387, 0xDC}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x03A0, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x03A1, 0x5E}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x03A2, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x03A3, 0x62}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x03A4, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x03A5, 0xc9}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x03A6, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x03A7, 0x27}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x03A8, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x03A9, 0x59}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x03AA, 0x02}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x03AB, 0x55}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x03AC, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x03AD, 0xc5}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x03AE, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x03AF, 0x27}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x03B0, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x03B1, 0x55}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x03B2, 0x02}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x03B3, 0x55}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x03B4, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x03B5, 0x0A}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x03D3, 0x08}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x03D5, 0x44}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x03D6, 0x51}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */// hynix_test : 0x54-->0x51
	{0x03D7, 0x56}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x03D8, 0x44}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x03D9, 0x06}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0580, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0581, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0582, 0x80}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0583, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0584, 0x80}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0585, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0586, 0x80}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0587, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0588, 0x80}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0589, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x058A, 0x80}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x05C2, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x05C3, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0080, 0xC7}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0119, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x011A, 0x15}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x011B, 0xC0}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0115, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0116, 0x2A}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0117, 0x4C}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0118, 0x20}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0223, 0xED}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0224, 0xE4}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0225, 0x09}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0226, 0x36}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x023E, 0x80}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x05B0, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */

	//{0x03D2, 0xAD}, //PLL reset 20120418 ryu add
	//{0x0616, 0x00}, //D-PHY reset 20120418 ryu add
	//{0x0616, 0x01}, //D-PHY reset disable 20120418 ryu add
	//{0x03D2, 0xAC}, //PLL reset disable 20120418 ryu add
	//{0x03D0, 0xE9}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	//{0x03D1, 0x74}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	//{0x0800, 0x07}, //0F}, EMI disable
	//{0x0801, 0x08}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */

	{0x0012, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0013, 0x40}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0024, 0x07}, /* windowing */
	{0x0025, 0xA0},//A8}, /* windowing */
	{0x0026, 0x0A}, /* windowing */
	{0x0027, 0x30},//30}, /* windowing */
	{0x0030, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0031, 0xFF}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0032, 0x06}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0033, 0xB0}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0034, 0x02}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0035, 0xD8}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x003A, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x003B, 0x2E}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x004A, 0x03}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x004B, 0xC0}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x004C, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x004D, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0C98, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0C99, 0x5E}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0C9A, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0C9B, 0x62}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x05A0, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0084, 0x30},//10}, BLC control
	{0x008D, 0xFF}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0090, 0x02},//0b}, BLC defect pixel th
	{0x00A7, 0x80},//FF}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x021A, 0x15},//05}, /*CDS bias */
	{0x022B, 0xb0},//f0}, RAMP filter
	{0x0232, 0x37}, //17}, black sun enable
	{0x0010, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0740, 0x1A}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0742, 0x1A}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0743, 0x1A}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0744, 0x1A}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0745, 0x04}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0746, 0x32}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0747, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0748, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0749, 0x90}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x074A, 0x1A}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x074B, 0xB1}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	//{0x0500, 0x1f}, //0x19}, //1b}, LSC disable
	{0x0510, 0x10}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */

	//{0x0500, 0x15},
	{0x0580, 0x81}, //thomaszhang open //

	{0x0589, 0x00},
	{0x058A, 0x80},

	{0x0217, 0x44}, //adaptive NCP on
	{0x0218, 0x00}, //scn_sel

	{0x02ac, 0x00}, //outdoor on
	{0x02ad, 0x00}, 
	{0x02ae, 0x00}, //outdoor off
	{0x02af, 0x00},
	{0x02b0, 0x00}, //indoor on
	{0x02b1, 0x00},
	{0x02b2, 0x00}, //indoor off
	{0x02b3, 0x00},
	{0x02b4, 0x60}, //dark1 on
	{0x02b5, 0x21},
	{0x02b6, 0x66}, //dark1 off
	{0x02b7, 0x8a},

	{0x02c0, 0x36}, //outdoor NCP en
	{0x02c1, 0x36}, //indoor NCP en
	{0x02c2, 0x36}, //dark1 NCP en
	{0x02c3, 0x36}, //3f}, //dark2 NCP disable jwryu 20120120
	{0x02c4, 0xE4}, //outdoor NCP voltage
	{0x02c5, 0xE4}, //indoor NCP voltage
	{0x02c6, 0xE4}, //dark1 NCP voltage
	{0x02c7, 0xdb}, //24}, //dark2 NCP voltage

	{0x061A, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x061B, 0x04},//03},ryu 20120416 /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x061C, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x061D, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x061E, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x061F, 0x03}, /* ryu 20120416 man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0613, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0615, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0616, 0x01}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0617, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0619, 0x01}, //gated clk mode 20120418 ryu add
	{0x0651, 0x02},
	{0x0652, 0x10},
	{0x0654, 0x0a},
	{0x0655, 0x05},

	{0x0008, 0x0F}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0630, 0x05}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0631, 0x00}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0632, 0x03}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0633, 0xC0}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */

	//LSC
	{0x0540, 0x10}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0541, 0x1f}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0550, 0x80}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0551, 0x80}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0552, 0x80}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0553, 0x80}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0554, 0x52}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0555, 0x52}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0556, 0x52}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0571, 0x60}, //60}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */

	{0x0662, 0x04},
	{0x0663, 0x05},//0a}, trail time /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */
	{0x0660, 0x08}, /* man_spec_edof_ctrl_edof_fw_spare_0 Gain x7 */

	{0x0020, 0x00},
	{0x0021, 0x04},
	{0x0022, 0x00},
	{0x0023, 0x08},

	{0x0024, 0x07},
	{0x0025, 0xA0},
	{0x0026, 0x0A},
	{0x0027, 0x20},

	{0x0119, 0x00},
	{0x011A, 0x2b},
	{0x011B, 0x9c},

	{0x0010, 0x00},
	//{0x0011, 0x40},
	{0x0500, 0x1f},

	{0x0630, 0x0A},
	{0x0631, 0x20},
	{0x0632, 0x07},
	{0x0633, 0xA0},//A8},

	{0x0042, 0x01},
	{0x0043, 0x3d}

};

LOCAL const SENSOR_REG_T hi542_1280X960_mipi_raw[] = {
	//Stand by
	{0x0632, 0x00}, //mipi_act_row_size_h, B[3:0]=MIPI active row size higher byte
	{0x0633, 0x00}, //mipi_act_row_size_l, B[7:0]=MIPI active row size lower byte, Legal value : 0~4095={mipi_active_row_h,mipi_active_row_l}
	{0x0001, 0x01}, //PWRCTLA//B[1]=soft reset, B[0]=sleep mode(0:normal operation, 1:sleep mode enable)
	{0x0617, 0x01}, //mipi_en_n, B[0]=MIPI Tx enable (0:On, 1:Off)
	{0x0001, 0x00}, //PWRCTLA//B[1]=soft reset, B[0]=sleep mode(0:normal operation, 1:sleep mode enable)
	{0x0001, 0x01}, //PWRCTLA//B[1]=soft reset, B[0]=sleep mode(0:normal operation, 1:sleep mode enable)
	{0x0617, 0x00}, //mipi_en_n, B[0]=MIPI Tx enable (0:On, 1:Off)

	//Preview,1280x960
	{0x0020, 0x00},
	{0x0021, 0x04},
	{0x0022, 0x00},
	{0x0023, 0x0C},

	{0x0024, 0x07}, /* windowing */
	{0x0025, 0xA0}, /* windowing */
	{0x0026, 0x0A}, /* windowing */
	{0x0027, 0x30}, /* windowing */
	//{0x0119, 0x00},
	//{0x011A, 0x15},
	//{0x011B, 0xC0},

	{0x0010, 0x01},
	{0x0011, 0x14},//thomaszhang debug	{0x0011, 0x04},
	{0x0500, 0x1B}, // 1B}, LSC OFF

	{0x0630, 0x05},
	{0x0631, 0x00},
	{0x0632, 0x03},
	{0x0633, 0xC0}

	//Start streaming
	//{0x0001, 0x00}, //PWRCTLA//B[1]=soft reset, B[0]=sleep mode(0:normal operation, 1:sleep mode enable)
};

LOCAL const SENSOR_REG_T hi542_2592X1944_mipi_raw[] = {
	//Stand by
	{0x0632, 0x00}, //mipi_act_row_size_h, B[3:0]=MIPI active row size higher byte
	{0x0633, 0x00}, //mipi_act_row_size_l, B[7:0]=MIPI active row size lower byte, Legal value : 0~4095={mipi_active_row_h,mipi_active_row_l}
	{0x0001, 0x01}, //PWRCTLA//B[1]=soft reset, B[0]=sleep mode(0:normal operation, 1:sleep mode enable)
	{0x0617, 0x01}, //mipi_en_n, B[0]=MIPI Tx enable (0:On, 1:Off)
	{0x0001, 0x00}, //PWRCTLA//B[1]=soft reset, B[0]=sleep mode(0:normal operation, 1:sleep mode enable)
	{0x0001, 0x01}, //PWRCTLA//B[1]=soft reset, B[0]=sleep mode(0:normal operation, 1:sleep mode enable)
	{0x0617, 0x00}, //mipi_en_n, B[0]=MIPI Tx enable (0:On, 1:Off)

	//Capture,2592x1952
	{0x0020, 0x00},
	{0x0021, 0x04},
	{0x0022, 0x00},
	{0x0023, 0x08},

	{0x0024, 0x07}, 
	{0x0025, 0xA0}, 
	{0x0026, 0x0A}, 
	{0x0027, 0x20}, 

	{0x0010, 0x00},
	{0x0011, 0x14},//thomaszhang debug	{0x0011, 0x04},
	{0x0500, 0x13}, //0x11},

	{0x0630, 0x0A},
	{0x0631, 0x20},
	{0x0632, 0x07},
	{0x0633, 0xA0}

	//Start streaming
	//{0x0001, 0x00}, //PWRCTLA//B[1]=soft reset, B[0]=sleep mode(0:normal operation, 1:sleep mode enable)
};

LOCAL SENSOR_REG_TAB_INFO_T s_hi542_resolution_Tab_RAW[] = {
	{ADDR_AND_LEN_OF_ARRAY(hi542_com_mipi_raw), 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(hi542_1280X960_mipi_raw), 1280, 960, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(hi542_2592X1944_mipi_raw), 2592, 1944, 24, SENSOR_IMAGE_FORMAT_RAW},
	
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

LOCAL SENSOR_TRIM_T s_hi542_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 1280, 960, 331, 840, 991, {0, 0, 1280, 960}},  //OPCLK = 84M  PCLK = 42M MCLK = 24M
	{0, 0, 2592, 1944, 331, 840, 1983, {0, 0, 2592, 1944}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
};

LOCAL const SENSOR_REG_T s_hi542_1280x960_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
	/*video mode 0: ?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 1:?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 2:?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 3:?fps*/
	{
		{0xffff, 0xff}
	}
};
LOCAL const SENSOR_REG_T s_hi542_2592x1944_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
	/*video mode 0: ?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 1:?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 2:?fps*/
	{
		{0xffff, 0xff}
	},
	/* video mode 3:?fps*/
	{
		{0xffff, 0xff}
	}
};

LOCAL SENSOR_VIDEO_INFO_T s_hi542_video_info[] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{30, 30, 331, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_hi542_1280x960_video_tab},
	{{{30, 30, 331, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_hi542_2592x1944_video_tab},

	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}
};

LOCAL uint32_t _hi542_set_video_mode(uint32_t param)
{
	SENSOR_REG_T_PTR sensor_reg_ptr;
	uint16_t         i = 0x00;
	uint32_t         mode;

	if (param >= SENSOR_VIDEO_MODE_MAX)
		return 0;

	if (SENSOR_SUCCESS != Sensor_GetMode(&mode)) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	if (PNULL == s_hi542_video_info[mode].setting_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR)&s_hi542_video_info[mode].setting_ptr[param];
	if (PNULL == sensor_reg_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	for (i=0x00; (0xffff!=sensor_reg_ptr[i].reg_addr)||(0xff!=sensor_reg_ptr[i].reg_value); i++) {
		Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	SENSOR_PRINT("0x%02x", param);
	return 0;
}


LOCAL SENSOR_IOCTL_FUNC_TAB_T s_hi542_ioctl_func_tab = {
	PNULL,
	_hi542_PowerOn,
	PNULL,
	_hi542_Identify,

	PNULL,// write register
	PNULL,// read  register
	PNULL,
	_hi542_GetResolutionTrimTab,

	// External
	PNULL,
	PNULL,
	PNULL,

	PNULL, //_hi542_set_brightness,
	PNULL, // _hi542_set_contrast,
	PNULL,
	PNULL,//_hi542_set_saturation,

	PNULL, //_hi542_set_work_mode,
	PNULL, //_hi542_set_image_effect,

	_hi542_BeforeSnapshot,//PNULL, //
	_hi542_after_snapshot,//PNULL, //
	PNULL,//_hi542_flash,
	PNULL,
	_hi542_write_exposure,//PNULL, //
	PNULL,
	_hi542_write_gain,//PNULL, //
	PNULL,
	PNULL,
	_hi542_write_af,//PNULL,// 
	PNULL,
	PNULL, //_hi542_set_awb,
	PNULL,
	PNULL,
	PNULL, //_hi542_set_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL, //_hi542_GetExifInfo,
	_hi542_ExtFunc,//
	PNULL, //_hi542_set_anti_flicker,
	_hi542_set_video_mode, //_hi542_set_video_mode,
	PNULL, //pick_jpeg_stream
	PNULL,  //meter_mode
	PNULL, //get_status
	_hi542_StreamOn,
	_hi542_StreamOff,
	_hi542_cfg_otp,
};

SENSOR_INFO_T g_hi542_mipi_raw_info = {
	hi542_I2C_ADDR_W,	// salve i2c write address
	hi542_I2C_ADDR_R,	// salve i2c read address

	SENSOR_I2C_REG_16BIT | SENSOR_I2C_VAL_8BIT,	// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
	// bit1: 0: i2c register addr  is 8 bit, 1: i2c register addr  is 16 bit
	// other bit: reseved
	SENSOR_HW_SIGNAL_PCLK_N | SENSOR_HW_SIGNAL_VSYNC_N | SENSOR_HW_SIGNAL_HSYNC_P,	// bit0: 0:negative; 1:positive -> polarily of pixel clock
	// bit2: 0:negative; 1:positive -> polarily of horizontal synchronization signal
	// bit4: 0:negative; 1:positive -> polarily of vertical synchronization signal
	// other bit: reseved

	// preview mode
	SENSOR_ENVIROMENT_NORMAL | SENSOR_ENVIROMENT_NIGHT,

	// image effect
	SENSOR_IMAGE_EFFECT_NORMAL |
		SENSOR_IMAGE_EFFECT_BLACKWHITE |
		SENSOR_IMAGE_EFFECT_RED |
		SENSOR_IMAGE_EFFECT_GREEN |
		SENSOR_IMAGE_EFFECT_BLUE |
		SENSOR_IMAGE_EFFECT_YELLOW |
		SENSOR_IMAGE_EFFECT_NEGATIVE | SENSOR_IMAGE_EFFECT_CANVAS,

	// while balance mode
	0,

	7,			// bit[0:7]: count of step in brightness, contrast, sharpness, saturation
	// bit[8:31] reseved

	SENSOR_LOW_PULSE_RESET,	// reset pulse level
	50,			// reset pulse width(ms)

	SENSOR_LOW_LEVEL_PWDN,	// 1: high level valid; 0: low level valid

	1,			// count of identify code
	{{0x0004, 0xb1},		// supply two code to identify sensor.
	 {0xffff, 0xff}},		// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,	// voltage of avdd

	2592,			// max width of source image
	1944,			// max height of source image
	"hi542",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_B,// pattern of input image form sensor;

	s_hi542_resolution_Tab_RAW,	// point to resolution table information structure
	&s_hi542_ioctl_func_tab,	// point to ioctl function table
	&s_hi542_mipi_raw_info_ptr,		// information and table about Rawrgb sensor
	NULL,			//&g_hi542_ext_info,                // extend information about sensor
	SENSOR_AVDD_1800MV,	// iovdd
	SENSOR_AVDD_1500MV,	// dvdd
	3,			// skip frame num before preview
	3,			// skip frame num before capture
	0,			// deci frame num during preview
	0,			// deci frame num during video preview

	0,
	0,
	0,
	0,
	0,
	{SENSOR_INTERFACE_TYPE_CSI2, 2, 10, 0},
	s_hi542_video_info,
	3,			// skip frame num while change setting
};

LOCAL struct sensor_raw_info* Sensor_GetContext(void)
{
	return s_hi542_mipi_raw_info_ptr;
}

LOCAL uint32_t Sensor_hi542_InitRawTuneInfo(void)
{
	uint32_t rtn=0x00;
	struct sensor_raw_info* raw_sensor_ptr=Sensor_GetContext();
	struct sensor_raw_tune_info* sensor_ptr=raw_sensor_ptr->tune_ptr;
	struct sensor_raw_cali_info* cali_ptr=raw_sensor_ptr->cali_ptr;
#if 0
	raw_sensor_ptr->version_info->version_id=0x00010000;
	raw_sensor_ptr->version_info->srtuct_size=sizeof(struct sensor_raw_info);
	//bypass
	sensor_ptr->version_id=0x00010000;
	sensor_ptr->blc_bypass=0x00;
	sensor_ptr->nlc_bypass=0x01;
	sensor_ptr->lnc_bypass=0x01;
	sensor_ptr->ae_bypass=0x00;
	sensor_ptr->awb_bypass=0x00;
	sensor_ptr->bpc_bypass=0x01;
	sensor_ptr->denoise_bypass=0x01;
	sensor_ptr->grgb_bypass=0x01;
	sensor_ptr->cmc_bypass=0x00;
	sensor_ptr->gamma_bypass=0x00;
	sensor_ptr->uvdiv_bypass=0x01;
	sensor_ptr->pref_bypass=0x01;
	sensor_ptr->bright_bypass=0x00;
	sensor_ptr->contrast_bypass=0x00;
	sensor_ptr->hist_bypass=0x01;
	sensor_ptr->auto_contrast_bypass=0x01;
	sensor_ptr->af_bypass=0x00;
	sensor_ptr->edge_bypass=0x00;
	sensor_ptr->fcs_bypass=0x00;
	sensor_ptr->css_bypass=0x00;
	sensor_ptr->saturation_bypass=0x00;
	sensor_ptr->hdr_bypass=0x01;
	sensor_ptr->glb_gain_bypass=0x01;
	sensor_ptr->chn_gain_bypass=0x01;

	//blc
	sensor_ptr->blc.mode=0x00;
	sensor_ptr->blc.offset[0].r=0x0f;
	sensor_ptr->blc.offset[0].gr=0x0f;
	sensor_ptr->blc.offset[0].gb=0x0f;
	sensor_ptr->blc.offset[0].b=0x0f;

	sensor_ptr->blc.offset[1].r=0x0f;
	sensor_ptr->blc.offset[1].gr=0x0f;
	sensor_ptr->blc.offset[1].gb=0x0f;
	sensor_ptr->blc.offset[1].b=0x0f;

	//nlc
	sensor_ptr->nlc.r_node[0]=0;
	sensor_ptr->nlc.r_node[1]=16;
	sensor_ptr->nlc.r_node[2]=32;
	sensor_ptr->nlc.r_node[3]=64;
	sensor_ptr->nlc.r_node[4]=96;
	sensor_ptr->nlc.r_node[5]=128;
	sensor_ptr->nlc.r_node[6]=160;
	sensor_ptr->nlc.r_node[7]=192;
	sensor_ptr->nlc.r_node[8]=224;
	sensor_ptr->nlc.r_node[9]=256;
	sensor_ptr->nlc.r_node[10]=288;
	sensor_ptr->nlc.r_node[11]=320;
	sensor_ptr->nlc.r_node[12]=384;
	sensor_ptr->nlc.r_node[13]=448;
	sensor_ptr->nlc.r_node[14]=512;
	sensor_ptr->nlc.r_node[15]=576;
	sensor_ptr->nlc.r_node[16]=640;
	sensor_ptr->nlc.r_node[17]=672;
	sensor_ptr->nlc.r_node[18]=704;
	sensor_ptr->nlc.r_node[19]=736;
	sensor_ptr->nlc.r_node[20]=768;
	sensor_ptr->nlc.r_node[21]=800;
	sensor_ptr->nlc.r_node[22]=832;
	sensor_ptr->nlc.r_node[23]=864;
	sensor_ptr->nlc.r_node[24]=896;
	sensor_ptr->nlc.r_node[25]=928;
	sensor_ptr->nlc.r_node[26]=960;
	sensor_ptr->nlc.r_node[27]=992;
	sensor_ptr->nlc.r_node[28]=1023;

	sensor_ptr->nlc.g_node[0]=0;
	sensor_ptr->nlc.g_node[1]=16;
	sensor_ptr->nlc.g_node[2]=32;
	sensor_ptr->nlc.g_node[3]=64;
	sensor_ptr->nlc.g_node[4]=96;
	sensor_ptr->nlc.g_node[5]=128;
	sensor_ptr->nlc.g_node[6]=160;
	sensor_ptr->nlc.g_node[7]=192;
	sensor_ptr->nlc.g_node[8]=224;
	sensor_ptr->nlc.g_node[9]=256;
	sensor_ptr->nlc.g_node[10]=288;
	sensor_ptr->nlc.g_node[11]=320;
	sensor_ptr->nlc.g_node[12]=384;
	sensor_ptr->nlc.g_node[13]=448;
	sensor_ptr->nlc.g_node[14]=512;
	sensor_ptr->nlc.g_node[15]=576;
	sensor_ptr->nlc.g_node[16]=640;
	sensor_ptr->nlc.g_node[17]=672;
	sensor_ptr->nlc.g_node[18]=704;
	sensor_ptr->nlc.g_node[19]=736;
	sensor_ptr->nlc.g_node[20]=768;
	sensor_ptr->nlc.g_node[21]=800;
	sensor_ptr->nlc.g_node[22]=832;
	sensor_ptr->nlc.g_node[23]=864;
	sensor_ptr->nlc.g_node[24]=896;
	sensor_ptr->nlc.g_node[25]=928;
	sensor_ptr->nlc.g_node[26]=960;
	sensor_ptr->nlc.g_node[27]=992;
	sensor_ptr->nlc.g_node[28]=1023;

	sensor_ptr->nlc.b_node[0]=0;
	sensor_ptr->nlc.b_node[1]=16;
	sensor_ptr->nlc.b_node[2]=32;
	sensor_ptr->nlc.b_node[3]=64;
	sensor_ptr->nlc.b_node[4]=96;
	sensor_ptr->nlc.b_node[5]=128;
	sensor_ptr->nlc.b_node[6]=160;
	sensor_ptr->nlc.b_node[7]=192;
	sensor_ptr->nlc.b_node[8]=224;
	sensor_ptr->nlc.b_node[9]=256;
	sensor_ptr->nlc.b_node[10]=288;
	sensor_ptr->nlc.b_node[11]=320;
	sensor_ptr->nlc.b_node[12]=384;
	sensor_ptr->nlc.b_node[13]=448;
	sensor_ptr->nlc.b_node[14]=512;
	sensor_ptr->nlc.b_node[15]=576;
	sensor_ptr->nlc.b_node[16]=640;
	sensor_ptr->nlc.b_node[17]=672;
	sensor_ptr->nlc.b_node[18]=704;
	sensor_ptr->nlc.b_node[19]=736;
	sensor_ptr->nlc.b_node[20]=768;
	sensor_ptr->nlc.b_node[21]=800;
	sensor_ptr->nlc.b_node[22]=832;
	sensor_ptr->nlc.b_node[23]=864;
	sensor_ptr->nlc.b_node[24]=896;
	sensor_ptr->nlc.b_node[25]=928;
	sensor_ptr->nlc.b_node[26]=960;
	sensor_ptr->nlc.b_node[27]=992;
	sensor_ptr->nlc.b_node[28]=1023;

	sensor_ptr->nlc.l_node[0]=0;
	sensor_ptr->nlc.l_node[1]=16;
	sensor_ptr->nlc.l_node[2]=32;
	sensor_ptr->nlc.l_node[3]=64;
	sensor_ptr->nlc.l_node[4]=96;
	sensor_ptr->nlc.l_node[5]=128;
	sensor_ptr->nlc.l_node[6]=160;
	sensor_ptr->nlc.l_node[7]=192;
	sensor_ptr->nlc.l_node[8]=224;
	sensor_ptr->nlc.l_node[9]=256;
	sensor_ptr->nlc.l_node[10]=288;
	sensor_ptr->nlc.l_node[11]=320;
	sensor_ptr->nlc.l_node[12]=384;
	sensor_ptr->nlc.l_node[13]=448;
	sensor_ptr->nlc.l_node[14]=512;
	sensor_ptr->nlc.l_node[15]=576;
	sensor_ptr->nlc.l_node[16]=640;
	sensor_ptr->nlc.l_node[17]=672;
	sensor_ptr->nlc.l_node[18]=704;
	sensor_ptr->nlc.l_node[19]=736;
	sensor_ptr->nlc.l_node[20]=768;
	sensor_ptr->nlc.l_node[21]=800;
	sensor_ptr->nlc.l_node[22]=832;
	sensor_ptr->nlc.l_node[23]=864;
	sensor_ptr->nlc.l_node[24]=896;
	sensor_ptr->nlc.l_node[25]=928;
	sensor_ptr->nlc.l_node[26]=960;
	sensor_ptr->nlc.l_node[27]=992;
	sensor_ptr->nlc.l_node[28]=1023;

	//ae
	sensor_ptr->ae.skip_frame=0x01;
	sensor_ptr->ae.normal_fix_fps=0;
	sensor_ptr->ae.night_fix_fps=0;
	sensor_ptr->ae.video_fps=0x1e;
	sensor_ptr->ae.target_lum=120;
	sensor_ptr->ae.target_zone=8;
	sensor_ptr->ae.quick_mode=1;
	sensor_ptr->ae.smart=0x00;// bit0: denoise bit1: edge bit2: startion
	sensor_ptr->ae.smart_rotio=255;
	sensor_ptr->ae.smart_mode=0; // 0: gain 1: lum
	sensor_ptr->ae.smart_base_gain=64;
	sensor_ptr->ae.smart_wave_min=0;
	sensor_ptr->ae.smart_wave_max=1023;
	sensor_ptr->ae.smart_pref_min=0;
	sensor_ptr->ae.smart_pref_max=255;
	sensor_ptr->ae.smart_denoise_min_index=0;
	sensor_ptr->ae.smart_denoise_max_index=254;
	sensor_ptr->ae.smart_edge_min_index=0;
	sensor_ptr->ae.smart_edge_max_index=6;
	sensor_ptr->ae.smart_sta_low_thr=40;
	sensor_ptr->ae.smart_sta_high_thr=120;
	sensor_ptr->ae.smart_sta_rotio=128;
	sensor_ptr->ae.ev[0]=0xd0;
	sensor_ptr->ae.ev[1]=0xe0;
	sensor_ptr->ae.ev[2]=0xf0;
	sensor_ptr->ae.ev[3]=0x00;
	sensor_ptr->ae.ev[4]=0x10;
	sensor_ptr->ae.ev[5]=0x20;
	sensor_ptr->ae.ev[6]=0x30;
	sensor_ptr->ae.ev[7]=0x00;
	sensor_ptr->ae.ev[8]=0x00;
	sensor_ptr->ae.ev[9]=0x00;
	sensor_ptr->ae.ev[10]=0x00;
	sensor_ptr->ae.ev[11]=0x00;
	sensor_ptr->ae.ev[12]=0x00;
	sensor_ptr->ae.ev[13]=0x00;
	sensor_ptr->ae.ev[14]=0x00;
	sensor_ptr->ae.ev[15]=0x00;

	//awb
	sensor_ptr->awb.win_start.x=0x00;
	sensor_ptr->awb.win_start.y=0x00;
	sensor_ptr->awb.win_size.w=40;
	sensor_ptr->awb.win_size.h=30;
	sensor_ptr->awb.quick_mode = 1;
	sensor_ptr->awb.r_gain[0]=0x6c0;
	sensor_ptr->awb.g_gain[0]=0x400;
	sensor_ptr->awb.b_gain[0]=0x600;
	sensor_ptr->awb.r_gain[1]=0x480;
	sensor_ptr->awb.g_gain[1]=0x400;
	sensor_ptr->awb.b_gain[1]=0xc00;
	sensor_ptr->awb.r_gain[2]=0x400;
	sensor_ptr->awb.g_gain[2]=0x400;
	sensor_ptr->awb.b_gain[2]=0x400;
	sensor_ptr->awb.r_gain[3]=0x3fc;
	sensor_ptr->awb.g_gain[3]=0x400;
	sensor_ptr->awb.b_gain[3]=0x400;
	sensor_ptr->awb.r_gain[4]=0x480;
	sensor_ptr->awb.g_gain[4]=0x400;
	sensor_ptr->awb.b_gain[4]=0x800;
	sensor_ptr->awb.r_gain[5]=0x700;
	sensor_ptr->awb.g_gain[5]=0x400;
	sensor_ptr->awb.b_gain[5]=0x500;
	sensor_ptr->awb.r_gain[6]=0xa00;
	sensor_ptr->awb.g_gain[6]=0x400;
	sensor_ptr->awb.b_gain[6]=0x4c0;
	sensor_ptr->awb.r_gain[7]=0x400;
	sensor_ptr->awb.g_gain[7]=0x400;
	sensor_ptr->awb.b_gain[7]=0x400;
	sensor_ptr->awb.r_gain[8]=0x400;
	sensor_ptr->awb.g_gain[8]=0x400;
	sensor_ptr->awb.b_gain[8]=0x400;
	sensor_ptr->awb.target_zone=0x10;

	/*awb win*/
	sensor_ptr->awb.win[0].x=135;
	sensor_ptr->awb.win[0].yt=232;
	sensor_ptr->awb.win[0].yb=219;

	sensor_ptr->awb.win[1].x=139;
	sensor_ptr->awb.win[1].yt=254;
	sensor_ptr->awb.win[1].yb=193;

	sensor_ptr->awb.win[2].x=145;
	sensor_ptr->awb.win[2].yt=259;
	sensor_ptr->awb.win[2].yb=170;

	sensor_ptr->awb.win[3].x=155;
	sensor_ptr->awb.win[3].yt=259;
	sensor_ptr->awb.win[3].yb=122;

	sensor_ptr->awb.win[4].x=162;
	sensor_ptr->awb.win[4].yt=256;
	sensor_ptr->awb.win[4].yb=112;

	sensor_ptr->awb.win[5].x=172;
	sensor_ptr->awb.win[5].yt=230;
	sensor_ptr->awb.win[5].yb=110;

	sensor_ptr->awb.win[6].x=180;
	sensor_ptr->awb.win[6].yt=195;
	sensor_ptr->awb.win[6].yb=114;

	sensor_ptr->awb.win[7].x=184;
	sensor_ptr->awb.win[7].yt=185;
	sensor_ptr->awb.win[7].yb=120;

	sensor_ptr->awb.win[8].x=190;
	sensor_ptr->awb.win[8].yt=179;
	sensor_ptr->awb.win[8].yb=128;

	sensor_ptr->awb.win[9].x=199;
	sensor_ptr->awb.win[9].yt=175;
	sensor_ptr->awb.win[9].yb=131;

	sensor_ptr->awb.win[10].x=205;
	sensor_ptr->awb.win[10].yt=172;
	sensor_ptr->awb.win[10].yb=129;

	sensor_ptr->awb.win[11].x=210;
	sensor_ptr->awb.win[11].yt=169;
	sensor_ptr->awb.win[11].yb=123;

	sensor_ptr->awb.win[12].x=215;
	sensor_ptr->awb.win[12].yt=166;
	sensor_ptr->awb.win[12].yb=112;

	sensor_ptr->awb.win[13].x=226;
	sensor_ptr->awb.win[13].yt=159;
	sensor_ptr->awb.win[13].yb=98;

	sensor_ptr->awb.win[14].x=234;
	sensor_ptr->awb.win[14].yt=153;
	sensor_ptr->awb.win[14].yb=92;

	sensor_ptr->awb.win[15].x=248;
	sensor_ptr->awb.win[15].yt=144;
	sensor_ptr->awb.win[15].yb=84;

	sensor_ptr->awb.win[16].x=265;
	sensor_ptr->awb.win[16].yt=133;
	sensor_ptr->awb.win[16].yb=81;

	sensor_ptr->awb.win[17].x=277;
	sensor_ptr->awb.win[17].yt=126;
	sensor_ptr->awb.win[17].yb=79;

	sensor_ptr->awb.win[18].x=291;
	sensor_ptr->awb.win[18].yt=119;
	sensor_ptr->awb.win[18].yb=80;

	sensor_ptr->awb.win[19].x=305;
	sensor_ptr->awb.win[19].yt=109;
	sensor_ptr->awb.win[19].yb=90;

	sensor_ptr->awb.gain_convert[0].r=0x100;
	sensor_ptr->awb.gain_convert[0].g=0x100;
	sensor_ptr->awb.gain_convert[0].b=0x100;

	sensor_ptr->awb.gain_convert[1].r=0x100;
	sensor_ptr->awb.gain_convert[1].g=0x100;
	sensor_ptr->awb.gain_convert[1].b=0x100;

	//ov8825 awb param
	sensor_ptr->awb.t_func.a = 274;
	sensor_ptr->awb.t_func.b = -335;
	sensor_ptr->awb.t_func.shift = 10;

	sensor_ptr->awb.wp_count_range.min_proportion = 256 / 128;
	sensor_ptr->awb.wp_count_range.max_proportion = 256 / 4;

	sensor_ptr->awb.g_estimate.num = 4;
	sensor_ptr->awb.g_estimate.t_thr[0] = 2000;
	sensor_ptr->awb.g_estimate.g_thr[0][0] = 406;    //0.404
	sensor_ptr->awb.g_estimate.g_thr[0][1] = 419;    //0.414
	sensor_ptr->awb.g_estimate.w_thr[0][0] = 255;
	sensor_ptr->awb.g_estimate.w_thr[0][1] = 0;

	sensor_ptr->awb.g_estimate.t_thr[1] = 3000;
	sensor_ptr->awb.g_estimate.g_thr[1][0] = 406;    //0.404
	sensor_ptr->awb.g_estimate.g_thr[1][1] = 419;    //0.414
	sensor_ptr->awb.g_estimate.w_thr[1][0] = 255;
	sensor_ptr->awb.g_estimate.w_thr[1][1] = 0;

	sensor_ptr->awb.g_estimate.t_thr[2] = 6500;
	sensor_ptr->awb.g_estimate.g_thr[2][0] = 445;
	sensor_ptr->awb.g_estimate.g_thr[2][1] = 478;
	sensor_ptr->awb.g_estimate.w_thr[2][0] = 255;
	sensor_ptr->awb.g_estimate.w_thr[2][1] = 0;

	sensor_ptr->awb.g_estimate.t_thr[3] = 20000;
	sensor_ptr->awb.g_estimate.g_thr[3][0] = 407;
	sensor_ptr->awb.g_estimate.g_thr[3][1] = 414;
	sensor_ptr->awb.g_estimate.w_thr[3][0] = 255;
	sensor_ptr->awb.g_estimate.w_thr[3][1] = 0;

	sensor_ptr->awb.gain_adjust.num = 5;
	sensor_ptr->awb.gain_adjust.t_thr[0] = 1600;
	sensor_ptr->awb.gain_adjust.w_thr[0] = 192;
	sensor_ptr->awb.gain_adjust.t_thr[1] = 2200;
	sensor_ptr->awb.gain_adjust.w_thr[1] = 208;
	sensor_ptr->awb.gain_adjust.t_thr[2] = 3500;
	sensor_ptr->awb.gain_adjust.w_thr[2] = 256;
	sensor_ptr->awb.gain_adjust.t_thr[3] = 10000;
	sensor_ptr->awb.gain_adjust.w_thr[3] = 256;
	sensor_ptr->awb.gain_adjust.t_thr[4] = 12000;
	sensor_ptr->awb.gain_adjust.w_thr[4] = 128;

	sensor_ptr->awb.light.num = 7;
	sensor_ptr->awb.light.t_thr[0] = 2300;
	sensor_ptr->awb.light.w_thr[0] = 2;
	sensor_ptr->awb.light.t_thr[1] = 2850;
	sensor_ptr->awb.light.w_thr[1] = 4;
	sensor_ptr->awb.light.t_thr[2] = 4150;
	sensor_ptr->awb.light.w_thr[2] = 8;
	sensor_ptr->awb.light.t_thr[3] = 5500;
	sensor_ptr->awb.light.w_thr[3] = 160;
	sensor_ptr->awb.light.t_thr[4] = 6500;
	sensor_ptr->awb.light.w_thr[4] = 192;
	sensor_ptr->awb.light.t_thr[5] = 7500;
	sensor_ptr->awb.light.w_thr[5] = 96;
	sensor_ptr->awb.light.t_thr[6] = 8200;
	sensor_ptr->awb.light.w_thr[6] = 8;

	sensor_ptr->awb.steady_speed = 6;
	sensor_ptr->awb.debug_level = 2;
	sensor_ptr->awb.smart = 1;

	sensor_ptr->awb.alg_id = 0;
	sensor_ptr->awb.smart_index = 4;

	//bpc
	sensor_ptr->bpc.flat_thr=80;
	sensor_ptr->bpc.std_thr=20;
	sensor_ptr->bpc.texture_thr=2;

	// denoise
	sensor_ptr->denoise.write_back=0x00;
	sensor_ptr->denoise.r_thr=0x08;
	sensor_ptr->denoise.g_thr=0x08;
	sensor_ptr->denoise.b_thr=0x08;

	sensor_ptr->denoise.diswei[0]=255;
	sensor_ptr->denoise.diswei[1]=253;
	sensor_ptr->denoise.diswei[2]=251;
	sensor_ptr->denoise.diswei[3]=249;
	sensor_ptr->denoise.diswei[4]=247;
	sensor_ptr->denoise.diswei[5]=245;
	sensor_ptr->denoise.diswei[6]=243;
	sensor_ptr->denoise.diswei[7]=241;
	sensor_ptr->denoise.diswei[8]=239;
	sensor_ptr->denoise.diswei[9]=237;
	sensor_ptr->denoise.diswei[10]=235;
	sensor_ptr->denoise.diswei[11]=234;
	sensor_ptr->denoise.diswei[12]=232;
	sensor_ptr->denoise.diswei[13]=230;
	sensor_ptr->denoise.diswei[14]=228;
	sensor_ptr->denoise.diswei[15]=226;
	sensor_ptr->denoise.diswei[16]=225;
	sensor_ptr->denoise.diswei[17]=223;
	sensor_ptr->denoise.diswei[18]=221;

	sensor_ptr->denoise.ranwei[0]=255;
	sensor_ptr->denoise.ranwei[1]=252;
	sensor_ptr->denoise.ranwei[2]=243;
	sensor_ptr->denoise.ranwei[3]=230;
	sensor_ptr->denoise.ranwei[4]=213;
	sensor_ptr->denoise.ranwei[5]=193;
	sensor_ptr->denoise.ranwei[6]=170;
	sensor_ptr->denoise.ranwei[7]=147;
	sensor_ptr->denoise.ranwei[8]=125;
	sensor_ptr->denoise.ranwei[9]=103;
	sensor_ptr->denoise.ranwei[10]=83;
	sensor_ptr->denoise.ranwei[11]=66;
	sensor_ptr->denoise.ranwei[12]=51;
	sensor_ptr->denoise.ranwei[13]=38;
	sensor_ptr->denoise.ranwei[14]=28;
	sensor_ptr->denoise.ranwei[15]=20;
	sensor_ptr->denoise.ranwei[16]=14;
	sensor_ptr->denoise.ranwei[17]=10;
	sensor_ptr->denoise.ranwei[18]=6;
	sensor_ptr->denoise.ranwei[19]=4;
	sensor_ptr->denoise.ranwei[20]=2;
	sensor_ptr->denoise.ranwei[21]=1;
	sensor_ptr->denoise.ranwei[22]=0;
	sensor_ptr->denoise.ranwei[23]=0;
	sensor_ptr->denoise.ranwei[24]=0;
	sensor_ptr->denoise.ranwei[25]=0;
	sensor_ptr->denoise.ranwei[26]=0;
	sensor_ptr->denoise.ranwei[27]=0;
	sensor_ptr->denoise.ranwei[28]=0;
	sensor_ptr->denoise.ranwei[29]=0;
	sensor_ptr->denoise.ranwei[30]=0;

	//GrGb
	sensor_ptr->grgb.edge_thr=26;
	sensor_ptr->grgb.diff_thr=80;

	//cfa
	sensor_ptr->cfa.edge_thr=0x1a;
	sensor_ptr->cfa.diff_thr=0x00;

	//cmc
	sensor_ptr->cmc.matrix[0][0]=0x6f3;
	sensor_ptr->cmc.matrix[0][1]=0x3e0a;
	sensor_ptr->cmc.matrix[0][2]=0x3f03;
	sensor_ptr->cmc.matrix[0][3]=0x3ec0;
	sensor_ptr->cmc.matrix[0][4]=0x693;
	sensor_ptr->cmc.matrix[0][5]=0x3eae;
	sensor_ptr->cmc.matrix[0][6]=0x0d;
	sensor_ptr->cmc.matrix[0][7]=0x3c03;
	sensor_ptr->cmc.matrix[0][8]=0x7f0;

	//Gamma
	sensor_ptr->gamma.axis[0][0]=0;
	sensor_ptr->gamma.axis[0][1]=8;
	sensor_ptr->gamma.axis[0][2]=16;
	sensor_ptr->gamma.axis[0][3]=24;
	sensor_ptr->gamma.axis[0][4]=32;
	sensor_ptr->gamma.axis[0][5]=48;
	sensor_ptr->gamma.axis[0][6]=64;
	sensor_ptr->gamma.axis[0][7]=80;
	sensor_ptr->gamma.axis[0][8]=96;
	sensor_ptr->gamma.axis[0][9]=128;
	sensor_ptr->gamma.axis[0][10]=160;
	sensor_ptr->gamma.axis[0][11]=192;
	sensor_ptr->gamma.axis[0][12]=224;
	sensor_ptr->gamma.axis[0][13]=256;
	sensor_ptr->gamma.axis[0][14]=288;
	sensor_ptr->gamma.axis[0][15]=320;
	sensor_ptr->gamma.axis[0][16]=384;
	sensor_ptr->gamma.axis[0][17]=448;
	sensor_ptr->gamma.axis[0][18]=512;
	sensor_ptr->gamma.axis[0][19]=576;
	sensor_ptr->gamma.axis[0][20]=640;
	sensor_ptr->gamma.axis[0][21]=768;
	sensor_ptr->gamma.axis[0][22]=832;
	sensor_ptr->gamma.axis[0][23]=896;
	sensor_ptr->gamma.axis[0][24]=960;
	sensor_ptr->gamma.axis[0][25]=1023;

	sensor_ptr->gamma.axis[1][0]=0x00;
	sensor_ptr->gamma.axis[1][1]=0x05;
	sensor_ptr->gamma.axis[1][2]=0x09;
	sensor_ptr->gamma.axis[1][3]=0x0e;
	sensor_ptr->gamma.axis[1][4]=0x13;
	sensor_ptr->gamma.axis[1][5]=0x1f;
	sensor_ptr->gamma.axis[1][6]=0x2a;
	sensor_ptr->gamma.axis[1][7]=0x36;
	sensor_ptr->gamma.axis[1][8]=0x40;
	sensor_ptr->gamma.axis[1][9]=0x58;
	sensor_ptr->gamma.axis[1][10]=0x68;
	sensor_ptr->gamma.axis[1][11]=0x76;
	sensor_ptr->gamma.axis[1][12]=0x84;
	sensor_ptr->gamma.axis[1][13]=0x8f;
	sensor_ptr->gamma.axis[1][14]=0x98;
	sensor_ptr->gamma.axis[1][15]=0xa0;
	sensor_ptr->gamma.axis[1][16]=0xb0;
	sensor_ptr->gamma.axis[1][17]=0xbd;
	sensor_ptr->gamma.axis[1][18]=0xc6;
	sensor_ptr->gamma.axis[1][19]=0xcf;
	sensor_ptr->gamma.axis[1][20]=0xd8;
	sensor_ptr->gamma.axis[1][21]=0xe4;
	sensor_ptr->gamma.axis[1][22]=0xea;
	sensor_ptr->gamma.axis[1][23]=0xf0;
	sensor_ptr->gamma.axis[1][24]=0xf6;
	sensor_ptr->gamma.axis[1][25]=0xff;

	sensor_ptr->gamma.tab[0].axis[0][0]=0;
	sensor_ptr->gamma.tab[0].axis[0][1]=8;
	sensor_ptr->gamma.tab[0].axis[0][2]=16;
	sensor_ptr->gamma.tab[0].axis[0][3]=24;
	sensor_ptr->gamma.tab[0].axis[0][4]=32;
	sensor_ptr->gamma.tab[0].axis[0][5]=48;
	sensor_ptr->gamma.tab[0].axis[0][6]=64;
	sensor_ptr->gamma.tab[0].axis[0][7]=80;
	sensor_ptr->gamma.tab[0].axis[0][8]=96;
	sensor_ptr->gamma.tab[0].axis[0][9]=128;
	sensor_ptr->gamma.tab[0].axis[0][10]=160;
	sensor_ptr->gamma.tab[0].axis[0][11]=192;
	sensor_ptr->gamma.tab[0].axis[0][12]=224;
	sensor_ptr->gamma.tab[0].axis[0][13]=256;
	sensor_ptr->gamma.tab[0].axis[0][14]=288;
	sensor_ptr->gamma.tab[0].axis[0][15]=320;
	sensor_ptr->gamma.tab[0].axis[0][16]=384;
	sensor_ptr->gamma.tab[0].axis[0][17]=448;
	sensor_ptr->gamma.tab[0].axis[0][18]=512;
	sensor_ptr->gamma.tab[0].axis[0][19]=576;
	sensor_ptr->gamma.tab[0].axis[0][20]=640;
	sensor_ptr->gamma.tab[0].axis[0][21]=768;
	sensor_ptr->gamma.tab[0].axis[0][22]=832;
	sensor_ptr->gamma.tab[0].axis[0][23]=896;
	sensor_ptr->gamma.tab[0].axis[0][24]=960;
	sensor_ptr->gamma.tab[0].axis[0][25]=1023;

	sensor_ptr->gamma.tab[0].axis[1][0]=0x00;
	sensor_ptr->gamma.tab[0].axis[1][1]=0x05;
	sensor_ptr->gamma.tab[0].axis[1][2]=0x09;
	sensor_ptr->gamma.tab[0].axis[1][3]=0x0e;
	sensor_ptr->gamma.tab[0].axis[1][4]=0x13;
	sensor_ptr->gamma.tab[0].axis[1][5]=0x1f;
	sensor_ptr->gamma.tab[0].axis[1][6]=0x2a;
	sensor_ptr->gamma.tab[0].axis[1][7]=0x36;
	sensor_ptr->gamma.tab[0].axis[1][8]=0x40;
	sensor_ptr->gamma.tab[0].axis[1][9]=0x58;
	sensor_ptr->gamma.tab[0].axis[1][10]=0x68;
	sensor_ptr->gamma.tab[0].axis[1][11]=0x76;
	sensor_ptr->gamma.tab[0].axis[1][12]=0x84;
	sensor_ptr->gamma.tab[0].axis[1][13]=0x8f;
	sensor_ptr->gamma.tab[0].axis[1][14]=0x98;
	sensor_ptr->gamma.tab[0].axis[1][15]=0xa0;
	sensor_ptr->gamma.tab[0].axis[1][16]=0xb0;
	sensor_ptr->gamma.tab[0].axis[1][17]=0xbd;
	sensor_ptr->gamma.tab[0].axis[1][18]=0xc6;
	sensor_ptr->gamma.tab[0].axis[1][19]=0xcf;
	sensor_ptr->gamma.tab[0].axis[1][20]=0xd8;
	sensor_ptr->gamma.tab[0].axis[1][21]=0xe4;
	sensor_ptr->gamma.tab[0].axis[1][22]=0xea;
	sensor_ptr->gamma.tab[0].axis[1][23]=0xf0;
	sensor_ptr->gamma.tab[0].axis[1][24]=0xf6;
	sensor_ptr->gamma.tab[0].axis[1][25]=0xff;

	sensor_ptr->gamma.tab[1].axis[0][0]=0;
	sensor_ptr->gamma.tab[1].axis[0][1]=8;
	sensor_ptr->gamma.tab[1].axis[0][2]=16;
	sensor_ptr->gamma.tab[1].axis[0][3]=24;
	sensor_ptr->gamma.tab[1].axis[0][4]=32;
	sensor_ptr->gamma.tab[1].axis[0][5]=48;
	sensor_ptr->gamma.tab[1].axis[0][6]=64;
	sensor_ptr->gamma.tab[1].axis[0][7]=80;
	sensor_ptr->gamma.tab[1].axis[0][8]=96;
	sensor_ptr->gamma.tab[1].axis[0][9]=128;
	sensor_ptr->gamma.tab[1].axis[0][10]=160;
	sensor_ptr->gamma.tab[1].axis[0][11]=192;
	sensor_ptr->gamma.tab[1].axis[0][12]=224;
	sensor_ptr->gamma.tab[1].axis[0][13]=256;
	sensor_ptr->gamma.tab[1].axis[0][14]=288;
	sensor_ptr->gamma.tab[1].axis[0][15]=320;
	sensor_ptr->gamma.tab[1].axis[0][16]=384;
	sensor_ptr->gamma.tab[1].axis[0][17]=448;
	sensor_ptr->gamma.tab[1].axis[0][18]=512;
	sensor_ptr->gamma.tab[1].axis[0][19]=576;
	sensor_ptr->gamma.tab[1].axis[0][20]=640;
	sensor_ptr->gamma.tab[1].axis[0][21]=768;
	sensor_ptr->gamma.tab[1].axis[0][22]=832;
	sensor_ptr->gamma.tab[1].axis[0][23]=896;
	sensor_ptr->gamma.tab[1].axis[0][24]=960;
	sensor_ptr->gamma.tab[1].axis[0][25]=1023;

	sensor_ptr->gamma.tab[1].axis[1][0]=0x00;
	sensor_ptr->gamma.tab[1].axis[1][1]=0x05;
	sensor_ptr->gamma.tab[1].axis[1][2]=0x09;
	sensor_ptr->gamma.tab[1].axis[1][3]=0x0e;
	sensor_ptr->gamma.tab[1].axis[1][4]=0x13;
	sensor_ptr->gamma.tab[1].axis[1][5]=0x1f;
	sensor_ptr->gamma.tab[1].axis[1][6]=0x2a;
	sensor_ptr->gamma.tab[1].axis[1][7]=0x36;
	sensor_ptr->gamma.tab[1].axis[1][8]=0x40;
	sensor_ptr->gamma.tab[1].axis[1][9]=0x58;
	sensor_ptr->gamma.tab[1].axis[1][10]=0x68;
	sensor_ptr->gamma.tab[1].axis[1][11]=0x76;
	sensor_ptr->gamma.tab[1].axis[1][12]=0x84;
	sensor_ptr->gamma.tab[1].axis[1][13]=0x8f;
	sensor_ptr->gamma.tab[1].axis[1][14]=0x98;
	sensor_ptr->gamma.tab[1].axis[1][15]=0xa0;
	sensor_ptr->gamma.tab[1].axis[1][16]=0xb0;
	sensor_ptr->gamma.tab[1].axis[1][17]=0xbd;
	sensor_ptr->gamma.tab[1].axis[1][18]=0xc6;
	sensor_ptr->gamma.tab[1].axis[1][19]=0xcf;
	sensor_ptr->gamma.tab[1].axis[1][20]=0xd8;
	sensor_ptr->gamma.tab[1].axis[1][21]=0xe4;
	sensor_ptr->gamma.tab[1].axis[1][22]=0xea;
	sensor_ptr->gamma.tab[1].axis[1][23]=0xf0;
	sensor_ptr->gamma.tab[1].axis[1][24]=0xf6;
	sensor_ptr->gamma.tab[1].axis[1][25]=0xff;

	sensor_ptr->gamma.tab[2].axis[0][0]=0;
	sensor_ptr->gamma.tab[2].axis[0][1]=8;
	sensor_ptr->gamma.tab[2].axis[0][2]=16;
	sensor_ptr->gamma.tab[2].axis[0][3]=24;
	sensor_ptr->gamma.tab[2].axis[0][4]=32;
	sensor_ptr->gamma.tab[2].axis[0][5]=48;
	sensor_ptr->gamma.tab[2].axis[0][6]=64;
	sensor_ptr->gamma.tab[2].axis[0][7]=80;
	sensor_ptr->gamma.tab[2].axis[0][8]=96;
	sensor_ptr->gamma.tab[2].axis[0][9]=128;
	sensor_ptr->gamma.tab[2].axis[0][10]=160;
	sensor_ptr->gamma.tab[2].axis[0][11]=192;
	sensor_ptr->gamma.tab[2].axis[0][12]=224;
	sensor_ptr->gamma.tab[2].axis[0][13]=256;
	sensor_ptr->gamma.tab[2].axis[0][14]=288;
	sensor_ptr->gamma.tab[2].axis[0][15]=320;
	sensor_ptr->gamma.tab[2].axis[0][16]=384;
	sensor_ptr->gamma.tab[2].axis[0][17]=448;
	sensor_ptr->gamma.tab[2].axis[0][18]=512;
	sensor_ptr->gamma.tab[2].axis[0][19]=576;
	sensor_ptr->gamma.tab[2].axis[0][20]=640;
	sensor_ptr->gamma.tab[2].axis[0][21]=768;
	sensor_ptr->gamma.tab[2].axis[0][22]=832;
	sensor_ptr->gamma.tab[2].axis[0][23]=896;
	sensor_ptr->gamma.tab[2].axis[0][24]=960;
	sensor_ptr->gamma.tab[2].axis[0][25]=1023;

	sensor_ptr->gamma.tab[2].axis[1][0]=0x00;
	sensor_ptr->gamma.tab[2].axis[1][1]=0x05;
	sensor_ptr->gamma.tab[2].axis[1][2]=0x09;
	sensor_ptr->gamma.tab[2].axis[1][3]=0x0e;
	sensor_ptr->gamma.tab[2].axis[1][4]=0x13;
	sensor_ptr->gamma.tab[2].axis[1][5]=0x1f;
	sensor_ptr->gamma.tab[2].axis[1][6]=0x2a;
	sensor_ptr->gamma.tab[2].axis[1][7]=0x36;
	sensor_ptr->gamma.tab[2].axis[1][8]=0x40;
	sensor_ptr->gamma.tab[2].axis[1][9]=0x58;
	sensor_ptr->gamma.tab[2].axis[1][10]=0x68;
	sensor_ptr->gamma.tab[2].axis[1][11]=0x76;
	sensor_ptr->gamma.tab[2].axis[1][12]=0x84;
	sensor_ptr->gamma.tab[2].axis[1][13]=0x8f;
	sensor_ptr->gamma.tab[2].axis[1][14]=0x98;
	sensor_ptr->gamma.tab[2].axis[1][15]=0xa0;
	sensor_ptr->gamma.tab[2].axis[1][16]=0xb0;
	sensor_ptr->gamma.tab[2].axis[1][17]=0xbd;
	sensor_ptr->gamma.tab[2].axis[1][18]=0xc6;
	sensor_ptr->gamma.tab[2].axis[1][19]=0xcf;
	sensor_ptr->gamma.tab[2].axis[1][20]=0xd8;
	sensor_ptr->gamma.tab[2].axis[1][21]=0xe4;
	sensor_ptr->gamma.tab[2].axis[1][22]=0xea;
	sensor_ptr->gamma.tab[2].axis[1][23]=0xf0;
	sensor_ptr->gamma.tab[2].axis[1][24]=0xf6;
	sensor_ptr->gamma.tab[2].axis[1][25]=0xff;

	sensor_ptr->gamma.tab[3].axis[0][0]=0;
	sensor_ptr->gamma.tab[3].axis[0][1]=8;
	sensor_ptr->gamma.tab[3].axis[0][2]=16;
	sensor_ptr->gamma.tab[3].axis[0][3]=24;
	sensor_ptr->gamma.tab[3].axis[0][4]=32;
	sensor_ptr->gamma.tab[3].axis[0][5]=48;
	sensor_ptr->gamma.tab[3].axis[0][6]=64;
	sensor_ptr->gamma.tab[3].axis[0][7]=80;
	sensor_ptr->gamma.tab[3].axis[0][8]=96;
	sensor_ptr->gamma.tab[3].axis[0][9]=128;
	sensor_ptr->gamma.tab[3].axis[0][10]=160;
	sensor_ptr->gamma.tab[3].axis[0][11]=192;
	sensor_ptr->gamma.tab[3].axis[0][12]=224;
	sensor_ptr->gamma.tab[3].axis[0][13]=256;
	sensor_ptr->gamma.tab[3].axis[0][14]=288;
	sensor_ptr->gamma.tab[3].axis[0][15]=320;
	sensor_ptr->gamma.tab[3].axis[0][16]=384;
	sensor_ptr->gamma.tab[3].axis[0][17]=448;
	sensor_ptr->gamma.tab[3].axis[0][18]=512;
	sensor_ptr->gamma.tab[3].axis[0][19]=576;
	sensor_ptr->gamma.tab[3].axis[0][20]=640;
	sensor_ptr->gamma.tab[3].axis[0][21]=768;
	sensor_ptr->gamma.tab[3].axis[0][22]=832;
	sensor_ptr->gamma.tab[3].axis[0][23]=896;
	sensor_ptr->gamma.tab[3].axis[0][24]=960;
	sensor_ptr->gamma.tab[3].axis[0][25]=1023;

	sensor_ptr->gamma.tab[3].axis[1][0]=0x00;
	sensor_ptr->gamma.tab[3].axis[1][1]=0x05;
	sensor_ptr->gamma.tab[3].axis[1][2]=0x09;
	sensor_ptr->gamma.tab[3].axis[1][3]=0x0e;
	sensor_ptr->gamma.tab[3].axis[1][4]=0x13;
	sensor_ptr->gamma.tab[3].axis[1][5]=0x1f;
	sensor_ptr->gamma.tab[3].axis[1][6]=0x2a;
	sensor_ptr->gamma.tab[3].axis[1][7]=0x36;
	sensor_ptr->gamma.tab[3].axis[1][8]=0x40;
	sensor_ptr->gamma.tab[3].axis[1][9]=0x58;
	sensor_ptr->gamma.tab[3].axis[1][10]=0x68;
	sensor_ptr->gamma.tab[3].axis[1][11]=0x76;
	sensor_ptr->gamma.tab[3].axis[1][12]=0x84;
	sensor_ptr->gamma.tab[3].axis[1][13]=0x8f;
	sensor_ptr->gamma.tab[3].axis[1][14]=0x98;
	sensor_ptr->gamma.tab[3].axis[1][15]=0xa0;
	sensor_ptr->gamma.tab[3].axis[1][16]=0xb0;
	sensor_ptr->gamma.tab[3].axis[1][17]=0xbd;
	sensor_ptr->gamma.tab[3].axis[1][18]=0xc6;
	sensor_ptr->gamma.tab[3].axis[1][19]=0xcf;
	sensor_ptr->gamma.tab[3].axis[1][20]=0xd8;
	sensor_ptr->gamma.tab[3].axis[1][21]=0xe4;
	sensor_ptr->gamma.tab[3].axis[1][22]=0xea;
	sensor_ptr->gamma.tab[3].axis[1][23]=0xf0;
	sensor_ptr->gamma.tab[3].axis[1][24]=0xf6;
	sensor_ptr->gamma.tab[3].axis[1][25]=0xff;

	sensor_ptr->gamma.tab[4].axis[0][0]=0;
	sensor_ptr->gamma.tab[4].axis[0][1]=8;
	sensor_ptr->gamma.tab[4].axis[0][2]=16;
	sensor_ptr->gamma.tab[4].axis[0][3]=24;
	sensor_ptr->gamma.tab[4].axis[0][4]=32;
	sensor_ptr->gamma.tab[4].axis[0][5]=48;
	sensor_ptr->gamma.tab[4].axis[0][6]=64;
	sensor_ptr->gamma.tab[4].axis[0][7]=80;
	sensor_ptr->gamma.tab[4].axis[0][8]=96;
	sensor_ptr->gamma.tab[4].axis[0][9]=128;
	sensor_ptr->gamma.tab[4].axis[0][10]=160;
	sensor_ptr->gamma.tab[4].axis[0][11]=192;
	sensor_ptr->gamma.tab[4].axis[0][12]=224;
	sensor_ptr->gamma.tab[4].axis[0][13]=256;
	sensor_ptr->gamma.tab[4].axis[0][14]=288;
	sensor_ptr->gamma.tab[4].axis[0][15]=320;
	sensor_ptr->gamma.tab[4].axis[0][16]=384;
	sensor_ptr->gamma.tab[4].axis[0][17]=448;
	sensor_ptr->gamma.tab[4].axis[0][18]=512;
	sensor_ptr->gamma.tab[4].axis[0][19]=576;
	sensor_ptr->gamma.tab[4].axis[0][20]=640;
	sensor_ptr->gamma.tab[4].axis[0][21]=768;
	sensor_ptr->gamma.tab[4].axis[0][22]=832;
	sensor_ptr->gamma.tab[4].axis[0][23]=896;
	sensor_ptr->gamma.tab[4].axis[0][24]=960;
	sensor_ptr->gamma.tab[4].axis[0][25]=1023;

	sensor_ptr->gamma.tab[4].axis[1][0]=0x00;
	sensor_ptr->gamma.tab[4].axis[1][1]=0x05;
	sensor_ptr->gamma.tab[4].axis[1][2]=0x09;
	sensor_ptr->gamma.tab[4].axis[1][3]=0x0e;
	sensor_ptr->gamma.tab[4].axis[1][4]=0x13;
	sensor_ptr->gamma.tab[4].axis[1][5]=0x1f;
	sensor_ptr->gamma.tab[4].axis[1][6]=0x2a;
	sensor_ptr->gamma.tab[4].axis[1][7]=0x36;
	sensor_ptr->gamma.tab[4].axis[1][8]=0x40;
	sensor_ptr->gamma.tab[4].axis[1][9]=0x58;
	sensor_ptr->gamma.tab[4].axis[1][10]=0x68;
	sensor_ptr->gamma.tab[4].axis[1][11]=0x76;
	sensor_ptr->gamma.tab[4].axis[1][12]=0x84;
	sensor_ptr->gamma.tab[4].axis[1][13]=0x8f;
	sensor_ptr->gamma.tab[4].axis[1][14]=0x98;
	sensor_ptr->gamma.tab[4].axis[1][15]=0xa0;
	sensor_ptr->gamma.tab[4].axis[1][16]=0xb0;
	sensor_ptr->gamma.tab[4].axis[1][17]=0xbd;
	sensor_ptr->gamma.tab[4].axis[1][18]=0xc6;
	sensor_ptr->gamma.tab[4].axis[1][19]=0xcf;
	sensor_ptr->gamma.tab[4].axis[1][20]=0xd8;
	sensor_ptr->gamma.tab[4].axis[1][21]=0xe4;
	sensor_ptr->gamma.tab[4].axis[1][22]=0xea;
	sensor_ptr->gamma.tab[4].axis[1][23]=0xf0;
	sensor_ptr->gamma.tab[4].axis[1][24]=0xf6;
	sensor_ptr->gamma.tab[4].axis[1][25]=0xff;

	//uv div
	sensor_ptr->uv_div.thrd[0]=252;
	sensor_ptr->uv_div.thrd[1]=250;
	sensor_ptr->uv_div.thrd[2]=248;
	sensor_ptr->uv_div.thrd[3]=246;
	sensor_ptr->uv_div.thrd[4]=244;
	sensor_ptr->uv_div.thrd[5]=242;
	sensor_ptr->uv_div.thrd[6]=240;

	//pref
	sensor_ptr->pref.write_back=0x00;
	sensor_ptr->pref.y_thr=0x04;
	sensor_ptr->pref.u_thr=0x04;
	sensor_ptr->pref.v_thr=0x04;

	//bright
	sensor_ptr->bright.factor[0]=0xd0;
	sensor_ptr->bright.factor[1]=0xe0;
	sensor_ptr->bright.factor[2]=0xf0;
	sensor_ptr->bright.factor[3]=0x00;
	sensor_ptr->bright.factor[4]=0x10;
	sensor_ptr->bright.factor[5]=0x20;
	sensor_ptr->bright.factor[6]=0x30;
	sensor_ptr->bright.factor[7]=0x00;
	sensor_ptr->bright.factor[8]=0x00;
	sensor_ptr->bright.factor[9]=0x00;
	sensor_ptr->bright.factor[10]=0x00;
	sensor_ptr->bright.factor[11]=0x00;
	sensor_ptr->bright.factor[12]=0x00;
	sensor_ptr->bright.factor[13]=0x00;
	sensor_ptr->bright.factor[14]=0x00;
	sensor_ptr->bright.factor[15]=0x00;

	//contrast
	sensor_ptr->contrast.factor[0]=0x10;
	sensor_ptr->contrast.factor[1]=0x20;
	sensor_ptr->contrast.factor[2]=0x30;
	sensor_ptr->contrast.factor[3]=0x40;
	sensor_ptr->contrast.factor[4]=0x50;
	sensor_ptr->contrast.factor[5]=0x60;
	sensor_ptr->contrast.factor[6]=0x70;
	sensor_ptr->contrast.factor[7]=0x40;
	sensor_ptr->contrast.factor[8]=0x40;
	sensor_ptr->contrast.factor[9]=0x40;
	sensor_ptr->contrast.factor[10]=0x40;
	sensor_ptr->contrast.factor[11]=0x40;
	sensor_ptr->contrast.factor[12]=0x40;
	sensor_ptr->contrast.factor[13]=0x40;
	sensor_ptr->contrast.factor[14]=0x40;
	sensor_ptr->contrast.factor[15]=0x40;

	//hist
	sensor_ptr->hist.mode;
	sensor_ptr->hist.low_ratio;
	sensor_ptr->hist.high_ratio;

	//auto contrast
	sensor_ptr->auto_contrast.mode;

	//saturation
	sensor_ptr->saturation.factor[0]=0x28;
	sensor_ptr->saturation.factor[1]=0x30;
	sensor_ptr->saturation.factor[2]=0x38;
	sensor_ptr->saturation.factor[3]=0x40;
	sensor_ptr->saturation.factor[4]=0x48;
	sensor_ptr->saturation.factor[5]=0x50;
	sensor_ptr->saturation.factor[6]=0x58;
	sensor_ptr->saturation.factor[7]=0x40;
	sensor_ptr->saturation.factor[8]=0x40;
	sensor_ptr->saturation.factor[9]=0x40;
	sensor_ptr->saturation.factor[10]=0x40;
	sensor_ptr->saturation.factor[11]=0x40;
	sensor_ptr->saturation.factor[12]=0x40;
	sensor_ptr->saturation.factor[13]=0x40;
	sensor_ptr->saturation.factor[14]=0x40;
	sensor_ptr->saturation.factor[15]=0x40;

	//css
	sensor_ptr->css.lum_thr=255;
	sensor_ptr->css.chr_thr=2;
	sensor_ptr->css.low_thr[0]=3;
	sensor_ptr->css.low_thr[1]=4;
	sensor_ptr->css.low_thr[2]=5;
	sensor_ptr->css.low_thr[3]=6;
	sensor_ptr->css.low_thr[4]=7;
	sensor_ptr->css.low_thr[5]=8;
	sensor_ptr->css.low_thr[6]=9;
	sensor_ptr->css.low_sum_thr[0]=6;
	sensor_ptr->css.low_sum_thr[1]=8;
	sensor_ptr->css.low_sum_thr[2]=10;
	sensor_ptr->css.low_sum_thr[3]=12;
	sensor_ptr->css.low_sum_thr[4]=14;
	sensor_ptr->css.low_sum_thr[5]=16;
	sensor_ptr->css.low_sum_thr[6]=18;

	//af info
	sensor_ptr->af.max_step=0x3ff;
	sensor_ptr->af.min_step=0;
	sensor_ptr->af.max_tune_step=0;
	sensor_ptr->af.stab_period=120;
	sensor_ptr->af.alg_id=3;
	sensor_ptr->af.rough_count=12;
	sensor_ptr->af.af_rough_step[0]=320;
	sensor_ptr->af.af_rough_step[2]=384;
	sensor_ptr->af.af_rough_step[3]=448;
	sensor_ptr->af.af_rough_step[4]=512;
	sensor_ptr->af.af_rough_step[5]=576;
	sensor_ptr->af.af_rough_step[6]=640;
	sensor_ptr->af.af_rough_step[7]=704;
	sensor_ptr->af.af_rough_step[8]=768;
	sensor_ptr->af.af_rough_step[9]=832;
	sensor_ptr->af.af_rough_step[10]=896;
	sensor_ptr->af.af_rough_step[11]=960;
	sensor_ptr->af.af_rough_step[12]=1023;
	sensor_ptr->af.fine_count=4;

	//edge
	sensor_ptr->edge.info[0].detail_thr=0x00;
	sensor_ptr->edge.info[0].smooth_thr=0x30;
	sensor_ptr->edge.info[0].strength=0;
	sensor_ptr->edge.info[1].detail_thr=0x01;
	sensor_ptr->edge.info[1].smooth_thr=0x20;
	sensor_ptr->edge.info[1].strength=3;
	sensor_ptr->edge.info[2].detail_thr=0x2;
	sensor_ptr->edge.info[2].smooth_thr=0x10;
	sensor_ptr->edge.info[2].strength=5;
	sensor_ptr->edge.info[3].detail_thr=0x03;
	sensor_ptr->edge.info[3].smooth_thr=0x05;
	sensor_ptr->edge.info[3].strength=10;
	sensor_ptr->edge.info[4].detail_thr=0x06;
	sensor_ptr->edge.info[4].smooth_thr=0x05;
	sensor_ptr->edge.info[4].strength=20;
	sensor_ptr->edge.info[5].detail_thr=0x09;
	sensor_ptr->edge.info[5].smooth_thr=0x05;
	sensor_ptr->edge.info[5].strength=30;
	sensor_ptr->edge.info[6].detail_thr=0x0c;
	sensor_ptr->edge.info[6].smooth_thr=0x05;
	sensor_ptr->edge.info[6].strength=40;

	//emboss
	sensor_ptr->emboss.step=0x02;

	//global gain
	sensor_ptr->global.gain=0x40;

	//chn gain
	sensor_ptr->chn.r_gain=0x40;
	sensor_ptr->chn.g_gain=0x40;
	sensor_ptr->chn.b_gain=0x40;
	sensor_ptr->chn.r_offset=0x00;
	sensor_ptr->chn.r_offset=0x00;
	sensor_ptr->chn.r_offset=0x00;

	sensor_ptr->edge.info[0].detail_thr=0x00;
	sensor_ptr->edge.info[0].smooth_thr=0x30;
	sensor_ptr->edge.info[0].strength=0;
	sensor_ptr->edge.info[1].detail_thr=0x01;
	sensor_ptr->edge.info[1].smooth_thr=0x20;
	sensor_ptr->edge.info[1].strength=3;
	sensor_ptr->edge.info[2].detail_thr=0x2;
	sensor_ptr->edge.info[2].smooth_thr=0x10;
	sensor_ptr->edge.info[2].strength=5;
	sensor_ptr->edge.info[3].detail_thr=0x03;
	sensor_ptr->edge.info[3].smooth_thr=0x05;
	sensor_ptr->edge.info[3].strength=10;
	sensor_ptr->edge.info[4].detail_thr=0x06;
	sensor_ptr->edge.info[4].smooth_thr=0x05;
	sensor_ptr->edge.info[4].strength=20;
	sensor_ptr->edge.info[5].detail_thr=0x09;
	sensor_ptr->edge.info[5].smooth_thr=0x05;
	sensor_ptr->edge.info[5].strength=30;
	sensor_ptr->edge.info[6].detail_thr=0x0c;
	sensor_ptr->edge.info[6].smooth_thr=0x05;
	sensor_ptr->edge.info[6].strength=40;
	sensor_ptr->edge.info[7].detail_thr=0x0f;
	sensor_ptr->edge.info[7].smooth_thr=0x05;
	sensor_ptr->edge.info[7].strength=60;

	/*normal*/
	sensor_ptr->special_effect[0].matrix[0]=0x004d;
	sensor_ptr->special_effect[0].matrix[1]=0x0096;
	sensor_ptr->special_effect[0].matrix[2]=0x001d;
	sensor_ptr->special_effect[0].matrix[3]=0xffd5;
	sensor_ptr->special_effect[0].matrix[4]=0xffab;
	sensor_ptr->special_effect[0].matrix[5]=0x0080;
	sensor_ptr->special_effect[0].matrix[6]=0x0080;
	sensor_ptr->special_effect[0].matrix[7]=0xff95;
	sensor_ptr->special_effect[0].matrix[8]=0xffeb;
	sensor_ptr->special_effect[0].y_shift=0xff00;
	sensor_ptr->special_effect[0].u_shift=0x0000;
	sensor_ptr->special_effect[0].v_shift=0x0000;

	/*gray*/
	sensor_ptr->special_effect[1].matrix[0]=0x004d;
	sensor_ptr->special_effect[1].matrix[1]=0x0096;
	sensor_ptr->special_effect[1].matrix[2]=0x001d;
	sensor_ptr->special_effect[1].matrix[3]=0x0000;
	sensor_ptr->special_effect[1].matrix[4]=0x0000;
	sensor_ptr->special_effect[1].matrix[5]=0x0000;
	sensor_ptr->special_effect[1].matrix[6]=0x0000;
	sensor_ptr->special_effect[1].matrix[7]=0x0000;
	sensor_ptr->special_effect[1].matrix[8]=0x0000;
	sensor_ptr->special_effect[1].y_shift=0xff00;
	sensor_ptr->special_effect[1].u_shift=0x0000;
	sensor_ptr->special_effect[1].v_shift=0x0000;
	/*warm*/
	sensor_ptr->special_effect[2].matrix[0]=0x004d;
	sensor_ptr->special_effect[2].matrix[1]=0x0096;
	sensor_ptr->special_effect[2].matrix[2]=0x001d;
	sensor_ptr->special_effect[2].matrix[3]=0xffd5;
	sensor_ptr->special_effect[2].matrix[4]=0xffab;
	sensor_ptr->special_effect[2].matrix[5]=0x0080;
	sensor_ptr->special_effect[2].matrix[6]=0x0080;
	sensor_ptr->special_effect[2].matrix[7]=0xff95;
	sensor_ptr->special_effect[2].matrix[8]=0xffeb;
	sensor_ptr->special_effect[2].y_shift=0xff00;
	sensor_ptr->special_effect[2].u_shift=0xffd4;
	sensor_ptr->special_effect[2].v_shift=0x0080;
	/*green*/
	sensor_ptr->special_effect[3].matrix[0]=0x004d;
	sensor_ptr->special_effect[3].matrix[1]=0x0096;
	sensor_ptr->special_effect[3].matrix[2]=0x001d;
	sensor_ptr->special_effect[3].matrix[3]=0xffd5;
	sensor_ptr->special_effect[3].matrix[4]=0xffab;
	sensor_ptr->special_effect[3].matrix[5]=0x0080;
	sensor_ptr->special_effect[3].matrix[6]=0x0080;
	sensor_ptr->special_effect[3].matrix[7]=0xff95;
	sensor_ptr->special_effect[3].matrix[8]=0xffeb;
	sensor_ptr->special_effect[3].y_shift=0xff00;
	sensor_ptr->special_effect[3].u_shift=0xffd5;
	sensor_ptr->special_effect[3].v_shift=0xffca;
	/*cool*/
	sensor_ptr->special_effect[4].matrix[0]=0x004d;
	sensor_ptr->special_effect[4].matrix[1]=0x0096;
	sensor_ptr->special_effect[4].matrix[2]=0x001d;
	sensor_ptr->special_effect[4].matrix[3]=0xffd5;
	sensor_ptr->special_effect[4].matrix[4]=0xffab;
	sensor_ptr->special_effect[4].matrix[5]=0x0080;
	sensor_ptr->special_effect[4].matrix[6]=0x0080;
	sensor_ptr->special_effect[4].matrix[7]=0xff95;
	sensor_ptr->special_effect[4].matrix[8]=0xffeb;
	sensor_ptr->special_effect[4].y_shift=0xff00;
	sensor_ptr->special_effect[4].u_shift=0x0040;
	sensor_ptr->special_effect[4].v_shift=0x000a;
	/*orange*/
	sensor_ptr->special_effect[5].matrix[0]=0x004d;
	sensor_ptr->special_effect[5].matrix[1]=0x0096;
	sensor_ptr->special_effect[5].matrix[2]=0x001d;
	sensor_ptr->special_effect[5].matrix[3]=0xffd5;
	sensor_ptr->special_effect[5].matrix[4]=0xffab;
	sensor_ptr->special_effect[5].matrix[5]=0x0080;
	sensor_ptr->special_effect[5].matrix[6]=0x0080;
	sensor_ptr->special_effect[5].matrix[7]=0xff95;
	sensor_ptr->special_effect[5].matrix[8]=0xffeb;
	sensor_ptr->special_effect[5].y_shift=0xff00;
	sensor_ptr->special_effect[5].u_shift=0xff00;
	sensor_ptr->special_effect[5].v_shift=0x0028;
	/*negtive*/
	sensor_ptr->special_effect[6].matrix[0]=0xffb3;
	sensor_ptr->special_effect[6].matrix[1]=0xff6a;
	sensor_ptr->special_effect[6].matrix[2]=0xffe3;
	sensor_ptr->special_effect[6].matrix[3]=0x002b;
	sensor_ptr->special_effect[6].matrix[4]=0x0055;
	sensor_ptr->special_effect[6].matrix[5]=0xff80;
	sensor_ptr->special_effect[6].matrix[6]=0xff80;
	sensor_ptr->special_effect[6].matrix[7]=0x006b;
	sensor_ptr->special_effect[6].matrix[8]=0x0015;
	sensor_ptr->special_effect[6].y_shift=0x00ff;
	sensor_ptr->special_effect[6].u_shift=0x0000;
	sensor_ptr->special_effect[6].v_shift=0x0000;
	/*old*/
	sensor_ptr->special_effect[7].matrix[0]=0x004d;
	sensor_ptr->special_effect[7].matrix[1]=0x0096;
	sensor_ptr->special_effect[7].matrix[2]=0x001d;
	sensor_ptr->special_effect[7].matrix[3]=0x0000;
	sensor_ptr->special_effect[7].matrix[4]=0x0000;
	sensor_ptr->special_effect[7].matrix[5]=0x0000;
	sensor_ptr->special_effect[7].matrix[6]=0x0000;
	sensor_ptr->special_effect[7].matrix[7]=0x0000;
	sensor_ptr->special_effect[7].matrix[8]=0x0000;
	sensor_ptr->special_effect[7].y_shift=0xff00;
	sensor_ptr->special_effect[7].u_shift=0xffe2;
	sensor_ptr->special_effect[7].v_shift=0x0028;
#endif

	return rtn;
}


LOCAL uint32_t _hi542_GetResolutionTrimTab(uint32_t param)
{
	SENSOR_PRINT("0x%x", (uint32_t)s_hi542_Resolution_Trim_Tab);
	return (uint32_t) s_hi542_Resolution_Trim_Tab;
}

LOCAL uint32_t _hi542_PowerOn(uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_hi542_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_hi542_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_hi542_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_hi542_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_hi542_mipi_raw_info.reset_pulse_level;

	SENSOR_PRINT("SENSOR_HI542: _hi542_Power_On(1:on, 0:off): %d", power_on);

	if (SENSOR_TRUE == power_on) {
		Sensor_PowerDown(power_down);
		// Open power
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);

		usleep(20*1000);
		_DW9714A_SRCInit(2);

		Sensor_PowerDown(!power_down);
		usleep(10*1000);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(10*1000);
		// Reset sensor
		Sensor_Reset(reset_level);
		usleep(10*1000);
		
	} else {
		Sensor_PowerDown(power_down);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
	}

	return SENSOR_SUCCESS;
}

LOCAL uint32_t _hi542_cfg_otp(uint32_t  param)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr=(struct raw_param_info_tab*)s_hi542_raw_param_tab;
	uint32_t module_id=g_hi542_module_id;

	SENSOR_PRINT("SENSOR_HI542: _hi542_cfg_otp");

	if(PNULL!=tab_ptr[module_id].cfg_otp){
		tab_ptr[module_id].cfg_otp(0);
		}

	return rtn;
}

LOCAL uint32_t _hi542_com_Identify_otp(void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_HI542: _hi542_com_Identify_otp");

	/*read param id from sensor omap*/
	param_id=HI542_RAW_PARAM_COM;

	if(HI542_RAW_PARAM_COM==param_id){
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}

LOCAL uint32_t _hi542_GetRawInof(void)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr=(struct raw_param_info_tab*)s_hi542_raw_param_tab;
	uint32_t param_id;
	uint32_t i=0x00;

	/*read param id from sensor omap*/
	param_id=HI542_RAW_PARAM_COM;

	for(i=0x00; ; i++)
	{
		g_hi542_module_id = i;
		if(RAW_INFO_END_ID==tab_ptr[i].param_id){
			if(NULL==s_hi542_mipi_raw_info_ptr){
				SENSOR_PRINT("SENSOR_HI542: _hi542_GetRawInof no param error");
				rtn=SENSOR_FAIL;
			}
			SENSOR_PRINT("SENSOR_HI542: _hi542_GetRawInof end");
			break;
		}
		else if(PNULL!=tab_ptr[i].identify_otp){
			if(SENSOR_SUCCESS==tab_ptr[i].identify_otp(0))
			{
				s_hi542_mipi_raw_info_ptr = tab_ptr[i].info_ptr;
				SENSOR_PRINT("SENSOR_HI542: _hi542_GetRawInof success");
				break;
			}
		}
	}

	return rtn;
}

LOCAL uint32_t _hi542_GetMaxFrameLine(uint32_t index)
{
	uint32_t max_line=0x00;
	SENSOR_TRIM_T_PTR trim_ptr=s_hi542_Resolution_Trim_Tab;

	max_line=trim_ptr[index].frame_line;

	return max_line;
}

LOCAL uint32_t _hi542_Identify(uint32_t param)
{
#define hi542_PID_VALUE    0xb1
#define hi542_PID_ADDR     0x0004

	uint8_t pid_value = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("SENSOR_HI542: mipi raw identify\n");

	pid_value = Sensor_ReadReg(hi542_PID_ADDR);

	if (hi542_PID_VALUE == pid_value) {
		SENSOR_PRINT("SENSOR_HI542: this is hi542 sensor !");
		ret_value=_hi542_GetRawInof();
		if(SENSOR_SUCCESS != ret_value)
		{
			SENSOR_PRINT("SENSOR_HI542: the module is unknow error !");
		}
		Sensor_hi542_InitRawTuneInfo();
	} else {
		SENSOR_PRINT("SENSOR_HI542: identify fail,pid_value=%d", pid_value);
	}
	
	return ret_value;
}

LOCAL uint32_t _hi542_write_exposure(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint32_t line_pixel=0;
	uint32_t fixed_line_pixel = 0;
	uint32_t expsure_pixel=0x00;
	uint16_t expsure_line=0x00;
	uint16_t dummy_line=0x00;
	uint16_t frame_len_cur=0x00;
	uint16_t value=0x00;
	uint8_t values_1[] = { 0, 0, 0, 0, 0};
	uint8_t values_2[] = { 0, 0, 0, 0, 0};
	int i;

	expsure_line=param&0xffff;
	dummy_line=(param>>0x10)&0xfff;

	if(expsure_line < 4)
		expsure_line = 4;

	SENSOR_PRINT("SENSOR_HI542 isp_raw: write_exposure line:%d, dummy:%d", expsure_line, dummy_line);

	line_pixel= 2592 +16 +130 + 53; // 2608 + 130 +HBLANK  Hblank = Reg(0x0040)<< 0x08 +Sensor_ReadReg(0x0041) ; 2791 = 84Mhz(Oplck) => 33.2261905us 
	expsure_pixel=line_pixel*expsure_line;
	fixed_line_pixel = expsure_pixel + 5582;
	
	frame_len_cur = (Sensor_ReadReg(0x0042)&0xff)<<8;
	frame_len_cur |= Sensor_ReadReg(0x0043)&0xff;
	
	SENSOR_PRINT("SENSOR_HI542: write_exposure line:0x%x, 0x%x\n", expsure_line-971, frame_len_cur);
	
	if((frame_len_cur+971) < expsure_line)
	{
		value=(expsure_line-971)&0xff;
		ret_value = Sensor_WriteReg(0x0043, value);
		value=((expsure_line-971)>>0x08)&0xff;
		ret_value = Sensor_WriteReg(0x0042, value);
	}


	for ( i = 1 ; i < 5; i++ ) 
	{
		values_1[i] = ( 0xff & expsure_pixel ); 
		values_2[i] = ( 0xff & fixed_line_pixel);
		expsure_pixel >>= 8;
		fixed_line_pixel >>= 8;
	}
	
	 /*HI542 fixed time update*/
	Sensor_WriteReg(0x0120, values_2[4]);
	Sensor_WriteReg(0x0121, values_2[3]);
	Sensor_WriteReg(0x0122, values_2[2]);
	Sensor_WriteReg(0x0123, values_2[1]);

	/*HI542 max time update*/
	Sensor_WriteReg(0x011c, 	values_1[4]);
	Sensor_WriteReg(0x011d, 	values_1[3]);
	Sensor_WriteReg(0x011e, 	values_1[2]);
	Sensor_WriteReg(0x011f, 	values_1[1]);

	/*HI542 Exposure time update*/
	Sensor_WriteReg(0x0115, 	values_1[4]);
	Sensor_WriteReg(0x0116, 	values_1[3]);
	Sensor_WriteReg(0x0117, 	values_1[2]);
	Sensor_WriteReg(0x0118, 	values_1[1]);

	return ret_value;
}

LOCAL uint32_t _hi542_write_gain(uint32_t param)
{
// max gain 64X ; digital gain nx mode = 1x  reg[0x0580] bit[5:4] =0
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint32_t real_a_gain = 0;
	uint32_t real_d_gain = 0x80;
	uint32_t i=0x00;
	uint32_t gain_bit=0x00;
	uint8_t values_1[] = { 0, 0, 0, 0, 0 }; 
	uint32_t exposure_old = 0; //(hynix)
	uint16_t frame_len_cur=0x00;

	uint32_t real_gain = 0;

	SENSOR_PRINT("SENSOR_HI542 isp_raw: write gain:0x%08x", param);

	real_gain = ((param&0xf)+16)*(((param>>4)&0x01)+1)*(((param>>5)&0x01)+1)*(((param>>6)&0x01)+1)*(((param>>7)&0x01)+1);
	real_gain = real_gain*(((param>>8)&0x01)+1)*(((param>>9)&0x01)+1)*(((param>>10)&0x01)+1)*(((param>>11)&0x01)+1);

	real_a_gain=0x1000/real_gain-0x20;


	ret_value = Sensor_WriteReg(0x0129, real_a_gain);
	return ret_value;
}

LOCAL uint32_t _hi542_write_af(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t  slave_addr = DW9714A_VCM_SLAVE_ADDR;
	uint16_t cmd_len = 0;
	uint8_t cmd_val[2] = {0x00};

	cmd_val[0] = ((param&0xfff0)>>4) & 0x3f;
	cmd_val[1] = ((param&0x0f)<<4) & 0xf0;
	cmd_len = 2;
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

	SENSOR_PRINT("SENSOR_HI542: _write_af, ret =  %d, param = %d,  MSL:%x, LSL:%x\n", 
		ret_value, param, cmd_val[0], cmd_val[1]);
	return ret_value;
}

LOCAL uint32_t _hi542_ReadGain(uint32_t*  gain_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint32_t gain = 0;
	uint32_t real_a_gain = 0;
	uint32_t real_d_gain = 0;

	real_a_gain = Sensor_ReadReg(0x0129);

	value = Sensor_ReadReg(0x0589);
	real_d_gain |= (value&0x03)<<0x08;
	value = Sensor_ReadReg(0x058a);
	real_d_gain |= value&0xff;

	gain=(real_d_gain<<0x10)|real_a_gain;

	s_hi542_gain=gain;
	if (gain_ptr) {
		*gain_ptr = gain;
	}

	SENSOR_PRINT("SENSOR_HI542: _hi542_ReadGain gain: 0x%x", s_hi542_gain);

	return rtn;
}

LOCAL uint32_t _hi542_SetEV(uint32_t param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_T_PTR ext_ptr = (SENSOR_EXT_FUN_T_PTR) param;
	uint16_t value=0x00;
	uint32_t gain = s_hi542_gain;
	uint32_t real_a_gain = 0;
	uint32_t real_d_gain = 0;
	uint32_t cali = ext_ptr->param;
	uint32_t real_gain=0x00;
	uint32_t gain_bit=0x00;
	uint32_t i=0x00;

	SENSOR_PRINT("SENSOR_HI542: _hi542_SetEV param: 0x%x", cali);

	real_a_gain=gain&0xff;
	real_d_gain=(gain>>0x10)&0x3ff;

	real_gain=real_d_gain&0x7f;
	gain_bit=(real_d_gain>>0x07)&0x07;
	real_gain=gain_bit*128+real_gain;

	if(((real_a_gain + 32)* 256)/real_gain < cali) // gain > 8x
	{
		gain_bit=0x00;
		real_d_gain =(real_gain*cali)/(2*(real_a_gain + 32));
		real_a_gain = 0x00; //analog gain = 8x
	}
	else
	{
		real_a_gain=(256*32*(real_a_gain + 32))/(real_gain*cali) - 32;
		real_d_gain=0x80; //digital gain = 1x 
	}

	Sensor_WriteReg(0x0129, real_a_gain);
	value = (real_d_gain>>0x08)&0x03;
	Sensor_WriteReg(0x0589, value);
	value = real_d_gain&0xff;
	Sensor_WriteReg(0x058a, value);

	return rtn;
}

LOCAL uint32_t _hi542_DWInit(uint32_t param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_T_PTR ext_ptr = (SENSOR_EXT_FUN_T_PTR) param;
	uint16_t value=0x00;

	SENSOR_PRINT("SENSOR_HI542: _hi542_DWInit");

	Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
	_DW9714A_SRCInit(2);

	return rtn;
}

LOCAL uint32_t _hi542_ExtFunc(uint32_t ctl_param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) ctl_param;
	SENSOR_PRINT("SENSOR_HI542: _hi542_ExtFunc  cmd: %d ----sunao---\n", ext_ptr->cmd);
	switch (ext_ptr->cmd) {
		case SENSOR_EXT_FUNC_INIT:
			//rtn = _hi542_DWInit(ctl_param);
		break;
		case 10:
			rtn = _hi542_SetEV(ctl_param);
			break;
		default:
			break;
	}

	return rtn;
}

LOCAL uint32_t _hi542_BeforeSnapshot(uint32_t param)
{
	uint8_t exposure, exposure1, exposure2, exposurel;
	uint8_t exposure_max, exposure1_max, exposure2_max, exposurel_max;
	uint32_t capture_exposure, preview_maxline;
	uint32_t capture_maxline, preview_exposure;
	uint32_t gain = 0, value = 0;
	uint32_t prv_linetime=s_hi542_Resolution_Trim_Tab[SENSOR_MODE_PREVIEW_ONE].line_time;
	uint32_t cap_linetime = s_hi542_Resolution_Trim_Tab[param & 0xffff].line_time;

	uint16_t real_a_gain = 0;
	uint16_t real_d_gain = 0;
	uint32_t gain_bit=0x00;
	uint32_t i=0x00;
	
	param = param & 0xffff;

	SENSOR_PRINT("SENSOR_HI542: BeforeSnapshot moe: %d",param);

	if (SENSOR_MODE_PREVIEW_ONE >= param){
		_hi542_ReadGain(&gain);
		SENSOR_PRINT("SENSOR_HI542: prvmode equal to capmode");
		return SENSOR_SUCCESS;
	}

	exposure  = (uint8_t) Sensor_ReadReg(0x0111);
	exposure1 = (uint8_t) Sensor_ReadReg(0x0112);
	exposure2 = (uint8_t) Sensor_ReadReg(0x0113);
	exposurel = (uint8_t) Sensor_ReadReg(0x0114);
	preview_exposure = ((exposure<<0x18)&0x1f000000)|((exposure1<<0x10)&0xff0000)|((exposure2<<0x08)&0xff00)|(exposurel&0xff);

	exposure_max  = (uint8_t) Sensor_ReadReg(0x011c);
	exposure1_max = (uint8_t) Sensor_ReadReg(0x011d);
	exposure2_max = (uint8_t) Sensor_ReadReg(0x011e);
	exposurel_max = (uint8_t) Sensor_ReadReg(0x011f);
	capture_maxline = ((exposure_max<<0x18)&0x1f000000)|((exposure1_max<<0x10)&0xff0000)|((exposure2_max<<0x08)&0xff00)|(exposurel_max&0xff);

	_hi542_ReadGain(&gain);
	Sensor_SetMode(param);
	Sensor_SetMode_WaitDone();
	
	if (prv_linetime == cap_linetime) {
		SENSOR_PRINT("SENSOR_hi542: prvline equal to capline");
		Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, preview_exposure);
		return SENSOR_SUCCESS;
	}

	real_a_gain=gain&0xff;
	real_d_gain=(gain>>0x10)&0x3ff;
	SENSOR_PRINT("SENSOR_HI542: BeforeSnapshot gain: %d , preview_exposure: 0x%x, capture_maxline: 0x%x\n", real_a_gain, preview_exposure, capture_maxline);

	capture_exposure = preview_exposure *prv_linetime / cap_linetime;

	if (0 == capture_exposure) {
		capture_exposure = 1;
	}

	while(gain >= 0x8){
		capture_exposure = capture_exposure * 2;
		gain=gain / 2;
		if(capture_exposure > capture_maxline)
			capture_exposure = capture_maxline;
			break;
	}

	value=(capture_exposure>>0x18)&0x1f;
	Sensor_WriteReg(0x0115, value);
	value=(capture_exposure>>0x10)&0xff;
	Sensor_WriteReg(0x0116, value);
	value=(capture_exposure>>0x08)&0xff;
	Sensor_WriteReg(0x0117, value);
	value=capture_exposure&0xff;
	Sensor_WriteReg(0x0118, value);

	Sensor_WriteReg(0x0129, real_a_gain);

	s_hi542_gain = gain;

	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, capture_exposure);

	return SENSOR_SUCCESS;
}

LOCAL uint32_t _hi542_after_snapshot(uint32_t param)
{
	SENSOR_PRINT("SENSOR_HI542: after_snapshot mode:%d", param);
	Sensor_SetMode(param);

	return SENSOR_SUCCESS;
}

LOCAL uint32_t _hi542_StreamOn(uint32_t param)
{
	SENSOR_PRINT("SENSOR_HI542: StreamOn");

	Sensor_WriteReg(0x0001, 0x00);

	return 0;
}

LOCAL uint32_t _hi542_StreamOff(uint32_t param)
{
	SENSOR_PRINT("SENSOR_HI542: StreamOff");

	Sensor_WriteReg(0x0001, 0x01);

	return 0;
}

LOCAL uint32_t _DW9714A_SRCInit(uint32_t mode)
{
	uint8_t cmd_val[2] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t ret_value = SENSOR_SUCCESS;	
	int i = 0;
	
	slave_addr = DW9714A_VCM_SLAVE_ADDR;
	SENSOR_PRINT("SENSOR_HI542: _DW9714A_SRCInit: mode = %d\n", mode);
	switch (mode) {
		case 1:
		break;
		
		case 2:
		{
			cmd_val[0] = 0xec;
			cmd_val[1] = 0xa3;
			cmd_len = 2;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

			cmd_val[0] = 0xa1;
			cmd_val[1] = 0x0e;
			cmd_len = 2;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

			cmd_val[0] = 0xf2;
			cmd_val[1] = 0x90;
			cmd_len = 2;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

			cmd_val[0] = 0xdc;
			cmd_val[1] = 0x51;
			cmd_len = 2;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
		}
		break;

		case 3:
		break;

	}

	return ret_value;
}
