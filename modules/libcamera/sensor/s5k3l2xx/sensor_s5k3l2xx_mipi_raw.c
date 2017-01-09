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
//#include "packet_convert.h"
//#include "sensor_s5k3l2xx_golden.c"
//#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
#include "sensor_s5k3l2xx_raw_param_v3.c"
//#else
//#endif

#define CONFIG_S5K3L2XX_XCE  1
#define S5K3L2XX_4_LANES

#if (!CONFIG_S5K3L2XX_XCE)
#define S5K3L2XX_I2C_ADDR_W        0x10
#define S5K3L2XX_I2C_ADDR_R        0x10
#else
#define S5K3L2XX_I2C_ADDR_W        0x2d
#define S5K3L2XX_I2C_ADDR_R        0x2d
#endif

#define DW9807_VCM_SLAVE_ADDR     (0x18 >> 1)
#define DW9807_EEPROM_SLAVE_ADDR  (0xB0 >> 1)
#define S5K3L2XX_RAW_PARAM_COM     0x0000


#define SWAP16(x)  (x)//(((x)<<8)&0xff00)|(((x)>>8)&0xff)

static int s_s5k3l2xx_gain = 0;
static int s_capture_shutter = 0;
static int s_capture_VTS = 0;
static int s_exposure_time = 0;
static uint32_t g_module_id = 0;
static uint32_t g_flash_mode_en = 0;
static struct sensor_raw_info* s_s5k3l2xx_mipi_raw_info_ptr = NULL;
static uint32_t s_set_gain;
static uint32_t s_set_exposure;

static unsigned long _s5k3l2xx_GetResolutionTrimTab(unsigned long param);
static unsigned long _s5k3l2xx_PowerOn(unsigned long power_on);
static unsigned long _s5k3l2xx_Identify(unsigned long param);
static unsigned long _s5k3l2xx_BeforeSnapshot(unsigned long param);
static unsigned long _s5k3l2xx_after_snapshot(unsigned long param);
static unsigned long _s5k3l2xx_StreamOn(unsigned long param);
static unsigned long _s5k3l2xx_StreamOff(unsigned long param);
static unsigned long _s5k3l2xx_write_exposure(unsigned long param);
static unsigned long _s5k3l2xx_write_gain(unsigned long param);
static unsigned long _s5k3l2xx_write_af(unsigned long param);
static unsigned long _s5k3l2xx_flash(unsigned long param);
static unsigned long _s5k3l2xx_GetExifInfo(unsigned long param);
static unsigned long _s5k3l2xx_ExtFunc(unsigned long ctl_param);
static uint16_t _s5k3l2xx_get_VTS(void);
static uint32_t _s5k3l2xx_set_VTS(uint16_t VTS);
static uint32_t _s5k3l2xx_ReadGain(uint32_t* param);
static unsigned long _s5k3l2xx_set_video_mode(unsigned long param);
static uint16_t _s5k3l2xx_get_shutter(void);
static uint32_t _s5k3l2xx_set_shutter(uint16_t shutter);
static uint32_t _s5k3l2xx_com_Identify_otp(void* param_ptr);
static unsigned long _s5k3l2xx_access_val(unsigned long param);
static uint32_t _s5k3l2xx_read_otp(unsigned long param);
static uint32_t _s5k3l2xx_write_otp(unsigned long param);


static const struct raw_param_info_tab s_s5k3l2xx_raw_param_tab[] = {
	{S5K3L2XX_RAW_PARAM_COM, &s_s5k3l2xx_mipi_raw_info, _s5k3l2xx_com_Identify_otp, NULL},
	{RAW_INFO_END_ID, PNULL, PNULL, PNULL}
};

static const SENSOR_REG_T s5k3l2xx_common_init[] = {
// reset

{0xFCFC,	0xD000},
{0x6010,	0x0001},
{0x6214,	0x7970},
{0x6218,	0x7150},
{0x6004,	0x0001},
{0x6028,	0x7000},
{0x602A,	0x2200},
{0x6F12,	0x9FE5},
{0x6F12,	0x6810},
{0x6F12,	0x9FE5},
{0x6F12,	0x6800},
{0x6F12,	0x2DE9},
{0x6F12,	0x1040},
{0x6F12,	0x80E5},
{0x6F12,	0x0010},
{0x6F12,	0x90E5},
{0x6F12,	0x2C20},
{0x6F12,	0x42E0},
{0x6F12,	0x0110},
{0x6F12,	0xC0E1},
{0x6F12,	0xB410},
{0x6F12,	0x9FE5},
{0x6F12,	0x5410},
{0x6F12,	0x9FE5},
{0x6F12,	0x5400},
{0x6F12,	0x00EB},
{0x6F12,	0xD100},
{0x6F12,	0x9FE5},
{0x6F12,	0x5010},
{0x6F12,	0xC1E1},
{0x6F12,	0xB000},
{0x6F12,	0x9FE5},
{0x6F12,	0x4C10},
{0x6F12,	0x9FE5},
{0x6F12,	0x4C00},
{0x6F12,	0x00EB},
{0x6F12,	0xCE00},
{0x6F12,	0x9FE5},
{0x6F12,	0x4800},
{0x6F12,	0x9FE5},
{0x6F12,	0x4820},
{0x6F12,	0x9FE5},
{0x6F12,	0x4810},
{0x6F12,	0x82E5},
{0x6F12,	0x1000},
{0x6F12,	0xA0E3},
{0x6F12,	0x0000},
{0x6F12,	0xC1E1},
{0x6F12,	0xB200},
{0x6F12,	0xC1E1},
{0x6F12,	0xB000},
{0x6F12,	0xC1E1},
{0x6F12,	0xB400},
{0x6F12,	0xC1E1},
{0x6F12,	0xB600},
{0x6F12,	0x9FE5},
{0x6F12,	0x3000},
{0x6F12,	0x82E5},
{0x6F12,	0x2400},
{0x6F12,	0xBDE8},
{0x6F12,	0x1040},
{0x6F12,	0x2FE1},
{0x6F12,	0x1EFF},
{0x6F12,	0x0070},
{0x6F12,	0x0426},
{0x6F12,	0x0070},
{0x6F12,	0x6018},
{0x6F12,	0x0070},
{0x6F12,	0xE424},
{0x6F12,	0x0000},
{0x6F12,	0xA436},
{0x6F12,	0x0070},
{0x6F12,	0xF825},
{0x6F12,	0x0070},
{0x6F12,	0x8C24},
{0x6F12,	0x0000},
{0x6F12,	0x08C5},
{0x6F12,	0x0070},
{0x6F12,	0x3023},
{0x6F12,	0x0070},
{0x6F12,	0xF004},
{0x6F12,	0x0070},
{0x6F12,	0xFA25},
{0x6F12,	0x0070},
{0x6F12,	0x9C22},
{0x6F12,	0x2DE9},
{0x6F12,	0x3840},
{0x6F12,	0x9DE5},
{0x6F12,	0x1040},
{0x6F12,	0x8DE5},
{0x6F12,	0x0040},
{0x6F12,	0x00EB},
{0x6F12,	0xB400},
{0x6F12,	0x9FE5},
{0x6F12,	0x8402},
{0x6F12,	0xD4E1},
{0x6F12,	0xB420},
{0x6F12,	0x90E5},
{0x6F12,	0x0030},
{0x6F12,	0xA0E3},
{0x6F12,	0x0010},
{0x6F12,	0x83E0},
{0x6F12,	0x8101},
{0x6F12,	0x80E2},
{0x6F12,	0x250E},
{0x6F12,	0xD0E1},
{0x6F12,	0xB0C0},
{0x6F12,	0x5CE1},
{0x6F12,	0x0200},
{0x6F12,	0x00CA},
{0x6F12,	0x1300},
{0x6F12,	0xD0E1},
{0x6F12,	0xB2C0},
{0x6F12,	0x5CE1},
{0x6F12,	0x0200},
{0x6F12,	0x00BA},
{0x6F12,	0x1000},
{0x6F12,	0xD0E1},
{0x6F12,	0xF450},
{0x6F12,	0xD0E1},
{0x6F12,	0xF610},
{0x6F12,	0x41E0},
{0x6F12,	0x0530},
{0x6F12,	0xD0E1},
{0x6F12,	0xB210},
{0x6F12,	0xD0E1},
{0x6F12,	0xB000},
{0x6F12,	0x41E0},
{0x6F12,	0x0010},
{0x6F12,	0x42E0},
{0x6F12,	0x0000},
{0x6F12,	0x00E0},
{0x6F12,	0x9300},
{0x6F12,	0x00EB},
{0x6F12,	0xA100},
{0x6F12,	0xD4E1},
{0x6F12,	0xB610},
{0x6F12,	0x80E0},
{0x6F12,	0x0500},
{0x6F12,	0x00E0},
{0x6F12,	0x9100},
{0x6F12,	0xA0E1},
{0x6F12,	0x0003},
{0x6F12,	0xA0E1},
{0x6F12,	0x2008},
{0x6F12,	0xC4E1},
{0x6F12,	0xB600},
{0x6F12,	0xBDE8},
{0x6F12,	0x3840},
{0x6F12,	0x2FE1},
{0x6F12,	0x1EFF},
{0x6F12,	0x81E2},
{0x6F12,	0x0110},
{0x6F12,	0x51E3},
{0x6F12,	0x0500},
{0x6F12,	0xFF3A},
{0x6F12,	0xE3FF},
{0x6F12,	0xFFEA},
{0x6F12,	0xF9FF},
{0x6F12,	0x2DE9},
{0x6F12,	0x7040},
{0x6F12,	0x9FE5},
{0x6F12,	0xFC51},
{0x6F12,	0x95E5},
{0x6F12,	0x0000},
{0x6F12,	0x80E2},
{0x6F12,	0x020C},
{0x6F12,	0xD0E1},
{0x6F12,	0xB012},
{0x6F12,	0x51E3},
{0x6F12,	0x0000},
{0x6F12,	0x9F15},
{0x6F12,	0xEC11},
{0x6F12,	0xD115},
{0x6F12,	0x0B10},
{0x6F12,	0x5113},
{0x6F12,	0x0000},
{0x6F12,	0x000A},
{0x6F12,	0x3E00},
{0x6F12,	0x9FE5},
{0x6F12,	0xE011},
{0x6F12,	0xD1E1},
{0x6F12,	0xB210},
{0x6F12,	0xD0E1},
{0x6F12,	0xB222},
{0x6F12,	0x9FE5},
{0x6F12,	0xD841},
{0x6F12,	0x51E1},
{0x6F12,	0x0200},
{0x6F12,	0xA023},
{0x6F12,	0x0010},
{0x6F12,	0xA033},
{0x6F12,	0x0110},
{0x6F12,	0xC4E1},
{0x6F12,	0xB410},
{0x6F12,	0xD0E1},
{0x6F12,	0xB402},
{0x6F12,	0x00EB},
{0x6F12,	0x8300},
{0x6F12,	0xA0E1},
{0x6F12,	0x0060},
{0x6F12,	0x95E5},
{0x6F12,	0x0000},
{0x6F12,	0x80E2},
{0x6F12,	0x020C},
{0x6F12,	0xD0E1},
{0x6F12,	0xB602},
{0x6F12,	0x00EB},
{0x6F12,	0x7E00},
{0x6F12,	0x9FE5},
{0x6F12,	0xAC11},
{0x6F12,	0xA0E3},
{0x6F12,	0x0120},
{0x6F12,	0xD1E1},
{0x6F12,	0xB811},
{0x6F12,	0xA0E3},
{0x6F12,	0x0030},
{0x6F12,	0x51E1},
{0x6F12,	0x0600},
{0x6F12,	0xC491},
{0x6F12,	0xB220},
{0x6F12,	0x009A},
{0x6F12,	0x0100},
{0x6F12,	0x51E1},
{0x6F12,	0x0000},
{0x6F12,	0xC481},
{0x6F12,	0xB230},
{0x6F12,	0x95E5},
{0x6F12,	0x0010},
{0x6F12,	0x91E5},
{0x6F12,	0x3002},
{0x6F12,	0x50E3},
{0x6F12,	0x0000},
{0x6F12,	0x001A},
{0x6F12,	0x0B00},
{0x6F12,	0xA0E3},
{0x6F12,	0x8D0F},
{0x6F12,	0x90E1},
{0x6F12,	0xB1E0},
{0x6F12,	0x9FE5},
{0x6F12,	0x7401},
{0x6F12,	0x9FE5},
{0x6F12,	0x74C1},
{0x6F12,	0x5EE3},
{0x6F12,	0x0000},
{0x6F12,	0xD001},
{0x6F12,	0xB0E6},
{0x6F12,	0xD001},
{0x6F12,	0xB206},
{0x6F12,	0xD011},
{0x6F12,	0xB4E5},
{0x6F12,	0xD011},
{0x6F12,	0xB605},
{0x6F12,	0x8EE1},
{0x6F12,	0x0008},
{0x6F12,	0x8C05},
{0x6F12,	0x2C00},
{0x6F12,	0x8C15},
{0x6F12,	0x1800},
{0x6F12,	0x91E5},
{0x6F12,	0x2CC2},
{0x6F12,	0x5CE1},
{0x6F12,	0x0000},
{0x6F12,	0xC4D1},
{0x6F12,	0xB020},
{0x6F12,	0x00DA},
{0x6F12,	0x0200},
{0x6F12,	0x91E5},
{0x6F12,	0x28C2},
{0x6F12,	0x5CE1},
{0x6F12,	0x0000},
{0x6F12,	0xC4C1},
{0x6F12,	0xB030},
{0x6F12,	0xD4E1},
{0x6F12,	0xB400},
{0x6F12,	0x50E3},
{0x6F12,	0x0000},
{0x6F12,	0xD411},
{0x6F12,	0xB200},
{0x6F12,	0x5013},
{0x6F12,	0x0000},
{0x6F12,	0xD411},
{0x6F12,	0xB000},
{0x6F12,	0x5013},
{0x6F12,	0x0000},
{0x6F12,	0x9F15},
{0x6F12,	0x2001},
{0x6F12,	0x9011},
{0x6F12,	0xB100},
{0x6F12,	0xC411},
{0x6F12,	0xB600},
{0x6F12,	0xD4E1},
{0x6F12,	0xB600},
{0x6F12,	0x50E3},
{0x6F12,	0x0000},
{0x6F12,	0x4012},
{0x6F12,	0x0100},
{0x6F12,	0xC411},
{0x6F12,	0xB600},
{0x6F12,	0x9F15},
{0x6F12,	0x0801},
{0x6F12,	0xC011},
{0x6F12,	0xB030},
{0x6F12,	0xC011},
{0x6F12,	0xB820},
{0x6F12,	0x00EB},
{0x6F12,	0x4F00},
{0x6F12,	0x00EB},
{0x6F12,	0x5000},
{0x6F12,	0xA0E1},
{0x6F12,	0x0040},
{0x6F12,	0x9FE5},
{0x6F12,	0xF400},
{0x6F12,	0x00EB},
{0x6F12,	0x4F00},
{0x6F12,	0x54E1},
{0x6F12,	0x0000},
{0x6F12,	0x4480},
{0x6F12,	0x0000},
{0x6F12,	0xBD88},
{0x6F12,	0x7040},
{0x6F12,	0xA081},
{0x6F12,	0x0018},
{0x6F12,	0xA081},
{0x6F12,	0x2118},
{0x6F12,	0xA083},
{0x6F12,	0x2100},
{0x6F12,	0x008A},
{0x6F12,	0x4A00},
{0x6F12,	0xBDE8},
{0x6F12,	0x7040},
{0x6F12,	0x2FE1},
{0x6F12,	0x1EFF},
{0x6F12,	0x2DE9},
{0x6F12,	0x1040},
{0x6F12,	0x00EB},
{0x6F12,	0x4800},
{0x6F12,	0x50E3},
{0x6F12,	0x0000},
{0x6F12,	0x000B},
{0x6F12,	0x4800},
{0x6F12,	0x00EA},
{0x6F12,	0x0100},
{0x6F12,	0xA0E3},
{0x6F12,	0x0100},
{0x6F12,	0x00EB},
{0x6F12,	0x4700},
{0x6F12,	0x00EB},
{0x6F12,	0x4800},
{0x6F12,	0x50E3},
{0x6F12,	0x0000},
{0x6F12,	0xFF0A},
{0x6F12,	0xFAFF},
{0x6F12,	0x9FE5},
{0x6F12,	0xA400},
{0x6F12,	0xA0E3},
{0x6F12,	0x0020},
{0x6F12,	0xA0E3},
{0x6F12,	0x0810},
{0x6F12,	0x00EB},
{0x6F12,	0x4400},
{0x6F12,	0x9FE5},
{0x6F12,	0x7000},
{0x6F12,	0x9FE5},
{0x6F12,	0x9410},
{0x6F12,	0xD0E1},
{0x6F12,	0xB020},
{0x6F12,	0xC1E1},
{0x6F12,	0xB421},
{0x6F12,	0xD0E1},
{0x6F12,	0xB200},
{0x6F12,	0xC1E1},
{0x6F12,	0xB801},
{0x6F12,	0xBDE8},
{0x6F12,	0x1040},
{0x6F12,	0x2FE1},
{0x6F12,	0x1EFF},
{0x6F12,	0x2DE9},
{0x6F12,	0x7040},
{0x6F12,	0x9FE5},
{0x6F12,	0x7850},
{0x6F12,	0x9FE5},
{0x6F12,	0x6840},
{0x6F12,	0xD5E1},
{0x6F12,	0xB000},
{0x6F12,	0xA0E3},
{0x6F12,	0x0010},
{0x6F12,	0x00EB},
{0x6F12,	0x3800},
{0x6F12,	0xD4E1},
{0x6F12,	0xB200},
{0x6F12,	0xD4E5},
{0x6F12,	0x4210},
{0x6F12,	0x80E0},
{0x6F12,	0x0100},
{0x6F12,	0x00EB},
{0x6F12,	0x3600},
{0x6F12,	0xD4E1},
{0x6F12,	0xB612},
{0x6F12,	0xC0E3},
{0x6F12,	0x0100},
{0x6F12,	0x80E0},
{0x6F12,	0x0100},
{0x6F12,	0x9FE5},
{0x6F12,	0x4C10},
{0x6F12,	0x81E5},
{0x6F12,	0x0400},
{0x6F12,	0xA0E1},
{0x6F12,	0x0400},
{0x6F12,	0x00EB},
{0x6F12,	0x3100},
{0x6F12,	0xD5E1},
{0x6F12,	0xB000},
{0x6F12,	0xBDE8},
{0x6F12,	0x7040},
{0x6F12,	0xA0E3},
{0x6F12,	0x0110},
{0x6F12,	0x00EA},
{0x6F12,	0x2900},
{0x6F12,	0x0070},
{0x6F12,	0x0000},
{0x6F12,	0x0070},
{0x6F12,	0x4005},
{0x6F12,	0x00D0},
{0x6F12,	0x00C2},
{0x6F12,	0x0070},
{0x6F12,	0xFA25},
{0x6F12,	0x0070},
{0x6F12,	0x901F},
{0x6F12,	0x00D0},
{0x6F12,	0x0096},
{0x6F12,	0x0070},
{0x6F12,	0x801E},
{0x6F12,	0x0000},
{0x6F12,	0x3602},
{0x6F12,	0x00D0},
{0x6F12,	0x00A6},
{0x6F12,	0x0070},
{0x6F12,	0xE018},
{0x6F12,	0x0000},
{0x6F12,	0x1662},
{0x6F12,	0x00D0},
{0x6F12,	0x0062},
{0x6F12,	0x0070},
{0x6F12,	0xF825},
{0x6F12,	0x0070},
{0x6F12,	0x0014},
{0x6F12,	0x1FE5},
{0x6F12,	0x04F0},
{0x6F12,	0x0000},
{0x6F12,	0xD0DD},
{0x6F12,	0x1FE5},
{0x6F12,	0x04F0},
{0x6F12,	0x0000},
{0x6F12,	0x44DD},
{0x6F12,	0x1FE5},
{0x6F12,	0x04F0},
{0x6F12,	0x0000},
{0x6F12,	0xA0B1},
{0x6F12,	0x1FE5},
{0x6F12,	0x04F0},
{0x6F12,	0x0000},
{0x6F12,	0x7CE4},
{0x6F12,	0x1FE5},
{0x6F12,	0x04F0},
{0x6F12,	0x0000},
{0x6F12,	0x5055},
{0x6F12,	0x1FE5},
{0x6F12,	0x04F0},
{0x6F12,	0x0000},
{0x6F12,	0x9090},
{0x6F12,	0x1FE5},
{0x6F12,	0x04F0},
{0x6F12,	0x0000},
{0x6F12,	0x50B8},
{0x6F12,	0x1FE5},
{0x6F12,	0x04F0},
{0x6F12,	0x0000},
{0x6F12,	0x5C9D},
{0x6F12,	0x1FE5},
{0x6F12,	0x04F0},
{0x6F12,	0x0000},
{0x6F12,	0x1402},
{0x6F12,	0x1FE5},
{0x6F12,	0x04F0},
{0x6F12,	0x0000},
{0x6F12,	0x187F},
{0x6F12,	0x1FE5},
{0x6F12,	0x04F0},
{0x6F12,	0x0000},
{0x6F12,	0x00C0},
{0x6F12,	0x1FE5},
{0x6F12,	0x04F0},
{0x6F12,	0x0000},
{0x6F12,	0x98C4},
{0x6F12,	0x1FE5},
{0x6F12,	0x04F0},
{0x6F12,	0x0000},
{0x6F12,	0x2C4B},
{0x6F12,	0x1FE5},
{0x6F12,	0x04F0},
{0x6F12,	0x0000},
{0x6F12,	0xDC0C},
{0x6F12,	0x1FE5},
{0x6F12,	0x04F0},
{0x6F12,	0x0000},
{0x6F12,	0x68DC},
{0x6F12,	0x1FE5},
{0x6F12,	0x04F0},
{0x6F12,	0x0000},
{0x6F12,	0xFCE5},
{0x6F12,	0x1FE5},
{0x6F12,	0x04F0},
{0x6F12,	0x0000},
{0x6F12,	0xA436},
{0x6028,	0xD000},
{0x6004,	0x0000},
{0x3690,	0x0100},
{0x369A,	0xF446},
{0x369C,	0x5176},
{0x369E,	0x5C76},
{0x36A0,	0x5C76},
{0x36AE,	0xF40E},
{0x36B0,	0x0B00},
{0x36B2,	0x4B00},
{0x36B4,	0x4B00},
{0xF412,	0x0040},
{0x3664,	0x0BB8},
{0x35E4,	0x4776},
{0x3674,	0x0776},
{0x3672,	0x0520},
{0x367A,	0x00ED},
{0x372A,	0x0101},
{0x3246,	0x0092},
{0x324A,	0x009C},
{0x3256,	0x01BD},
{0x325A,	0x01C7},
{0x3248,	0x0092},
{0x324C,	0x009C},
{0x3258,	0x01B2},
{0x325C,	0x01BC},
{0x3792,	0x0000},
{0x3794,	0x0000},
{0x3796,	0x0000},
{0x379A,	0x0000},
{0x379C,	0x0000},
{0x379E,	0x0000},
{0x33DA,	0x00BC},
{0x33DC,	0x00BC},
{0x33DE,	0x00BE},
{0x33E0,	0x00BE},
{0x33EA,	0x00BC},
{0x33EC,	0x00BC},
{0x33EE,	0x00D8},
{0x33F0,	0x00D8},
{0x36A2,	0x5176},
{0x36A4,	0x2F76},
{0x36A6,	0x2F76},
{0x36B6,	0x0B00},
{0x36B8,	0x4B00},
{0x36BA,	0x4B00},
{0x3692,	0x0060},
{0x0C00,	0x0100},
{0x0C02,	0x01FF},
{0x0C04,	0x0400},
{0x0C06,	0x0438},
{0x0C08,	0x0200},
{0x0C0A,	0x03FF},
{0x0C0C,	0x0415},
{0x0C0E,	0x045B},
{0x0C10,	0x0400},
{0x0C12,	0x07FF},
{0x0C14,	0x0421},
{0x0C16,	0x043B},
{0x0C18,	0x0800},
{0x0C1A,	0x0FFF},
{0x0C1C,	0x0423},
{0x0C1E,	0x043B},
{0x0C20,	0x1000},
{0x0C22,	0x1FFF},
{0x0C24,	0x0426},
{0x0C26,	0x0426},
{0x0B80,	0x0001},
{0x0B86,	0x0020},
{0x0B84,	0x0020},
{0x0B82,	0x0003},
{0x0B8C,	0x000A},
{0x0B8E,	0x0000},
{0x0B88,	0x0008},
{0x0B8A,	0x0000},
{0x0B96,	0x0005},
{0x0B94,	0x0001},
{0x0602,	0x03FF},
{0x0604,	0x03FF},
{0x0606,	0x03FF},
{0x0608,	0x03FF},
{0x0B08,	0x0000},
{0x3BE0,	0x03DA},
{0x3BE2,	0x01D6},
{0x3BE4,	0x01BF},
{0x3BE6,	0x0001},
{0x3BE8,	0x0003},
{0x3BEA,	0x000A},
{0x3BEC,	0x0022},
{0x3BEE,	0x000A},
{0x3BF0,	0x0022},
{0x3BF2,	0x000A},
{0x3BF4,	0x0022},
{0x3BF6,	0x000A},
{0x3BF8,	0x0022},
{0x3BFA,	0x0001},
{0x3BFC,	0x0003},
{0x3BFE,	0x000A},
{0x3C00,	0x0022},
{0x3C02,	0x000A},
{0x3C04,	0x0022},
{0x3C06,	0x000A},
{0x3C08,	0x0022},
{0x3C0A,	0x000A},
{0x3C0C,	0x0022},
{0x3C0E,	0x0200},
{0x3C10,	0x0200},
{0x3C12,	0x0002},
{0x3C14,	0x0003},
{0x3C16,	0x0008},
{0x3C18,	0x0018},
{0x3C1A,	0x0008},
{0x3C1C,	0x0018},
{0x3C1E,	0x0008},
{0x3C20,	0x0018},
{0x3C22,	0x0008},
{0x3C24,	0x0018},
{0x3C26,	0x0002},
{0x3C28,	0x0003},
{0x3C2A,	0x0008},
{0x3C2C,	0x0018},
{0x3C2E,	0x0008},
{0x3C30,	0x0018},
{0x3C32,	0x0008},
{0x3C34,	0x0018},
{0x3C36,	0x0008},
{0x3C38,	0x0018},
{0x3C3A,	0x0200},
{0x3C3C,	0x0200},
{0x3C3E,	0x0002},
{0x3C40,	0x0003},
{0x3C42,	0x0014},
{0x3C44,	0x0040},
{0x3C46,	0x0014},
{0x3C48,	0x0040},
{0x3C4A,	0x0014},
{0x3C4C,	0x0040},
{0x3C4E,	0x0014},
{0x3C50,	0x0040},
{0x3C52,	0x0002},
{0x3C54,	0x0003},
{0x3C56,	0x0014},
{0x3C58,	0x0040},
{0x3C5A,	0x0014},
{0x3C5C,	0x0040},
{0x3C5E,	0x0014},
{0x3C60,	0x0040},
{0x3C62,	0x0014},
{0x3C64,	0x0040},
{0x3C66,	0x0200},
{0x3C68,	0x0200},
{0x3C6A,	0x0002},
{0x3C6C,	0x0003},
{0x3C6E,	0x0008},
{0x3C70,	0x0018},
{0x3C72,	0x0008},
{0x3C74,	0x0018},
{0x3C76,	0x0008},
{0x3C78,	0x0018},
{0x3C7A,	0x0008},
{0x3C7C,	0x0018},
{0x3C7E,	0x0002},
{0x3C80,	0x0003},
{0x3C82,	0x0008},
{0x3C84,	0x0018},
{0x3C86,	0x0008},
{0x3C88,	0x0018},
{0x3C8A,	0x0008},
{0x3C8C,	0x0018},
{0x3C8E,	0x0008},
{0x3C90,	0x0018},
{0x3C92,	0x0200},
{0x3C94,	0x0200},
{0x3C96,	0x0002},
{0x3C98,	0x0003},
{0x3C9A,	0x0014},
{0x3C9C,	0x0040},
{0x3C9E,	0x0014},
{0x3CA0,	0x0040},
{0x3CA2,	0x0014},
{0x3CA4,	0x0040},
{0x3CA6,	0x0014},
{0x3CA8,	0x0040},
{0x3CAA,	0x0002},
{0x3CAC,	0x0003},
{0x3CAE,	0x0014},
{0x3CB0,	0x0040},
{0x3CB2,	0x0014},
{0x3CB4,	0x0040},
{0x3CB6,	0x0014},
{0x3CB8,	0x0040},
{0x3CBA,	0x0014},
{0x3CBC,	0x0040},
{0x3CBE,	0x0200},
{0x3CC0,	0x0200},
{0x3CC2,	0x0002},
{0x3CC4,	0x0003},
{0x3CC6,	0x0008},
{0x3CC8,	0x0018},
{0x3CCA,	0x0008},
{0x3CCC,	0x0018},
{0x3CCE,	0x0008},
{0x3CD0,	0x0018},
{0x3CD2,	0x0008},
{0x3CD4,	0x0018},
{0x3CD6,	0x0002},
{0x3CD8,	0x0003},
{0x3CDA,	0x0008},
{0x3CDC,	0x0018},
{0x3CDE,	0x0008},
{0x3CE0,	0x0018},
{0x3CE2,	0x0008},
{0x3CE4,	0x0018},
{0x3CE6,	0x0008},
{0x3CE8,	0x0018},
{0x3CEA,	0x0200},
{0x3CEC,	0x0200},
{0x3CEE,	0x0002},
{0x3CF0,	0x0003},
{0x3CF2,	0x0014},
{0x3CF4,	0x0040},
{0x3CF6,	0x0014},
{0x3CF8,	0x0040},
{0x3CFA,	0x0014},
{0x3CFC,	0x0040},
{0x3CFE,	0x0014},
{0x3D00,	0x0040},
{0x3D02,	0x0002},
{0x3D04,	0x0003},
{0x3D06,	0x0014},
{0x3D08,	0x0040},
{0x3D0A,	0x0014},
{0x3D0C,	0x0040},
{0x3D0E,	0x0014},
{0x3D10,	0x0040},
{0x3D12,	0x0014},
{0x3D14,	0x0040},
{0x3D16,	0x0200},
{0x3D18,	0x0200},
{0x3D1A,	0x0002},
{0x3D1C,	0x0003},
{0x3D1E,	0x0008},
{0x3D20,	0x0018},
{0x3D22,	0x0008},
{0x3D24,	0x0018},
{0x3D26,	0x0008},
{0x3D28,	0x0018},
{0x3D2A,	0x0008},
{0x3D2C,	0x0018},
{0x3D2E,	0x0002},
{0x3D30,	0x0003},
{0x3D32,	0x0008},
{0x3D34,	0x0018},
{0x3D36,	0x0008},
{0x3D38,	0x0018},
{0x3D3A,	0x0008},
{0x3D3C,	0x0018},
{0x3D3E,	0x0008},
{0x3D40,	0x0018},
{0x3D42,	0x0200},
{0x3D44,	0x0200},
{0x3D46,	0x0002},
{0x3D48,	0x0003},
{0x3D4A,	0x0014},
{0x3D4C,	0x0040},
{0x3D4E,	0x0014},
{0x3D50,	0x0040},
{0x3D52,	0x0014},
{0x3D54,	0x0040},
{0x3D56,	0x0014},
{0x3D58,	0x0040},
{0x3D5A,	0x0002},
{0x3D5C,	0x0003},
{0x3D5E,	0x0014},
{0x3D60,	0x0040},
{0x3D62,	0x0014},
{0x3D64,	0x0040},
{0x3D66,	0x0014},
{0x3D68,	0x0040},
{0x3D6A,	0x0014},
{0x3D6C,	0x0040},
{0x3D6E,	0x0200},
{0x3D70,	0x0200},
{0x3D72,	0x0002},
{0x3D74,	0x0003},
{0x3D76,	0x0008},
{0x3D78,	0x0018},
{0x3D7A,	0x0008},
{0x3D7C,	0x0018},
{0x3D7E,	0x0008},
{0x3D80,	0x0018},
{0x3D82,	0x0008},
{0x3D84,	0x0018},
{0x3D86,	0x0002},
{0x3D88,	0x0003},
{0x3D8A,	0x0008},
{0x3D8C,	0x0018},
{0x3D8E,	0x0008},
{0x3D90,	0x0018},
{0x3D92,	0x0008},
{0x3D94,	0x0018},
{0x3D96,	0x0008},
{0x3D98,	0x0018},
{0x3D9A,	0x0200},
{0x3D9C,	0x0200},
{0x3D9E,	0x0F00},
{0x3DA0,	0x4B00},
{0x3DA2,	0x8700},
{0x3DA4,	0xC300},
{0x3DA6,	0xFF00},
{0x3DA8,	0x0032},
{0x3DAA,	0x42FF},
{0x3DAC,	0x0800},
{0x3DAE,	0x0001},
{0x3DB0,	0x0008},
{0x3DB2,	0x00FF},
{0x3DB4,	0x0008},
{0x3DB6,	0x0100},
{0x3DB8,	0x0100},
{0x3DBA,	0x0F00},
{0x0300,	0x0002},
{0x0302,	0x0001},
{0x0304,	0x0006},
{0x0306,	0x00D8},
{0x0308,	0x0008},
{0x030A,	0x0001},
{0x030C,	0x0004},
{0x030E,	0x00B4},
{0x35D8,	0x0000},
{0x303A,	0x02BC},
{0x0B04,	0x0101},
{0x38E8,	0x0A01},
{0x380E,	0x0100},
{0x0114,	0x0300},
{0x3026,	0x0010},
{0x3BD6,	0x1010},
{0x3BD8,	0x1010},



};

static const SENSOR_REG_T s5k3l2xx_4144x3106_setting[] = {

{0x0104,	0x0100},
{0x35D8,	0x0000},
{0x3026,	0x0010},
{0x3690,	0x0100},
{0x372C,	0x4848},
{0x372E,	0x0002},
{0x3730,	0x1106},
{0x0306,	0x00D8},
{0x030A,	0x0001},
{0x030C,	0x0004},
{0x030E,	0x00B4},
{0x0342,	0x11A0},
{0x0340,	0x0C76},
{0x034C,	0x1030},
{0x034E,	0x0C22},
{0x0344,	0x0024},
{0x0346,	0x000B},
{0x0348,	0x1053},
{0x034A,	0x0C2C},
{0x0386,	0x0001},
{0x0900,	0x0011},
{0x0400,	0x0000},
{0x0404,	0x0010},
{0x0104,	0x0000},


};

#if 0
static const SENSOR_REG_T s5k3l2xx_2072x1554_setting[] = {
{0x0104,	0x0100},
{0x0112,	0x0A0A},
{0x0500,	0x0000},
{0x35D8,	0x0000},
{0x3026,	0x0020},
{0x3690,	0x0100},
{0x372C,	0x4848},
{0x372E,	0x0002},
{0x3730,	0x1106},
{0x0306,	0x010A},
{0x030A,	0x0001},
{0x030C,	0x0008},
{0x030E,	0x0114},
{0x0342,	0x11A0},
{0x0340,	0x0C76},
{0x034C,	0x0818},
{0x034E,	0x0612},
{0x0344,	0x0024},
{0x0346,	0x0009},
{0x0348,	0x1053},
{0x034A,	0x0C2C},
{0x0386,	0x0003},
{0x0900,	0x0112},
{0x0400,	0x0000},
{0x0404,	0x0010},
{0x0104,	0x0000},
};
#endif

static SENSOR_REG_TAB_INFO_T s_s5k3l2xx_resolution_Tab_RAW[] = {

	{ADDR_AND_LEN_OF_ARRAY(s5k3l2xx_common_init), 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},
//	{ADDR_AND_LEN_OF_ARRAY(s5k3l2xx_2072x1554_setting), 2072, 1554, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(s5k3l2xx_4144x3106_setting), 4144, 3106, 24, SENSOR_IMAGE_FORMAT_RAW},
	
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},

};

static SENSOR_TRIM_T s_s5k3l2xx_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
//	{0, 0, 2072, 1554, 134, 900, 1664, {0, 0, 2072, 1554}},
	{0, 0, 4144, 3106, 113, 950, 3190, {0, 0, 4144, 3106}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},

};

static const SENSOR_REG_T s_s5k3l2xx_1632x1224_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

static const SENSOR_REG_T s_s5k3l2xx_1920x1080_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

static const SENSOR_REG_T  s_s5k3l2xx_3264x2448_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

static SENSOR_VIDEO_INFO_T s_s5k3l2xx_video_info[] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{30, 30, 219, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_s5k3l2xx_1632x1224_video_tab},
	{{{15, 15, 131, 64}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_s5k3l2xx_3264x2448_video_tab},
	//{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}
};

static unsigned long _s5k3l2xx_set_video_mode(unsigned long param)
{
	SENSOR_REG_T_PTR sensor_reg_ptr;
	uint16_t i = 0;
	uint32_t mode = 0;

	if (param >= SENSOR_VIDEO_MODE_MAX)
		return 0;

	if (SENSOR_SUCCESS != Sensor_GetMode(&mode)) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	if (PNULL == s_s5k3l2xx_video_info[mode].setting_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR)&s_s5k3l2xx_video_info[mode].setting_ptr[param];
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


static SENSOR_IOCTL_FUNC_TAB_T s_s5k3l2xx_ioctl_func_tab = {
	PNULL,
	_s5k3l2xx_PowerOn,
	PNULL,
	_s5k3l2xx_Identify,

	PNULL,// write register
	PNULL,// read  register
	PNULL,
	_s5k3l2xx_GetResolutionTrimTab,

	// External
	PNULL,
	PNULL,
	PNULL,

	PNULL,//_s5k3l2xx_set_brightness,
	PNULL,// _s5k3l2xx_set_contrast,
	PNULL,
	PNULL,//_s5k3l2xx_set_saturation,

	PNULL,//_s5k3l2xx_set_work_mode,
	PNULL,//_s5k3l2xx_set_image_effect,

	_s5k3l2xx_BeforeSnapshot,
	_s5k3l2xx_after_snapshot,
	PNULL,//_s5k3l2xx_flash,
	PNULL,
	_s5k3l2xx_write_exposure,
	PNULL,
	_s5k3l2xx_write_gain,
	PNULL,
	PNULL,
	_s5k3l2xx_write_af,
	PNULL,
	PNULL,//_s5k3l2xx_set_awb,
	PNULL,
	PNULL,
	PNULL,//_s5k3l2xx_set_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL,//_s5k3l2xx_GetExifInfo,
	PNULL,//_s5k3l2xx_ExtFunc,
	PNULL,//_s5k3l2xx_set_anti_flicker,
	PNULL,//_s5k3l2xx_set_video_mode,
	PNULL,//pick_jpeg_stream
	PNULL,//meter_mode
	PNULL,//get_status
	_s5k3l2xx_StreamOn,
	_s5k3l2xx_StreamOff,
	PNULL,//_s5k3l2xx_access_val,
};


SENSOR_INFO_T g_s5k3l2xx_mipi_raw_info = {
	S5K3L2XX_I2C_ADDR_W,	// salve i2c write address
	S5K3L2XX_I2C_ADDR_R,	// salve i2c read address

	SENSOR_I2C_REG_16BIT | SENSOR_I2C_VAL_16BIT | SENSOR_I2C_FREQ_400,	// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
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
	5,			// reset pulse width(ms)

	SENSOR_LOW_LEVEL_PWDN,	// 1: high level valid; 0: low level valid

	1,			// count of identify code
	{{0x0, 0x2},		// supply two code to identify sensor.
	 {0x1, 0x19}},		// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,	// voltage of avdd

	4144,			// max width of source image
	3106,			// max height of source image
	"s5k3l2xx",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_GR,//SENSOR_IMAGE_PATTERN_RAWRGB_R,// pattern of input image form sensor;

	s_s5k3l2xx_resolution_Tab_RAW,	// point to resolution table information structure
	&s_s5k3l2xx_ioctl_func_tab,	// point to ioctl function table
	&s_s5k3l2xx_mipi_raw_info_ptr,		// information and table about Rawrgb sensor
	NULL,			//&g_s5k3l2xx_ext_info,                // extend information about sensor
	SENSOR_AVDD_1800MV,	// iovdd
	SENSOR_AVDD_1200MV,	// dvdd
	3,			// skip frame num before preview
	3,			// skip frame num before capture
	0,			// deci frame num during preview
	0,			// deci frame num during video preview

	0,
	0,
	0,
	0,
	0,

	{SENSOR_INTERFACE_TYPE_CSI2, 4, 10, 0},


	s_s5k3l2xx_video_info,
	3,			// skip frame num while change setting
	48,			// horizontal view angle
	48,			// vertical view angle
};

static struct sensor_raw_info* Sensor_GetContext(void)
{
	return s_s5k3l2xx_mipi_raw_info_ptr;
}

static uint32_t Sensor_s5k3l2xx_InitRawTuneInfo(void)
{
	uint32_t rtn=0x00;

	return rtn;
}

static uint32_t _dw9807_SRCInit(uint32_t mode)
{
	uint8_t cmd_val[2] = {0x00};
	uint16_t slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t ret_value = SENSOR_SUCCESS;
	SENSOR_PRINT("SENSOR_S5K3L2XX: %d",mode);

	slave_addr = DW9807_VCM_SLAVE_ADDR;
	switch (mode) {
	case 1:
		break;

	case 2:
	{
		cmd_len = 2;

		cmd_val[0] = 0x02;
		cmd_val[1] = 0x01;
		ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
		if(ret_value){
			SENSOR_PRINT("SENSOR_S5K3L2XX: _dw9807_SRCInit 0 fail!");
		}

		cmd_val[0] = 0x02;
		cmd_val[1] = 0x00;
		ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
		if(ret_value){
			SENSOR_PRINT("SENSOR_S5K3L2XX: _dw9807_SRCInit 1 fail!");
		}

		usleep(200);

		cmd_val[0] = 0x02;
		cmd_val[1] = 0x02;
		ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
		if(ret_value){
			SENSOR_PRINT("SENSOR_S5K3L2XX: _dw9807_SRCInit 2 fail!");
		}

		cmd_val[0] = 0x06;
		cmd_val[1] = 0x61;
		ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
		if(ret_value){
			SENSOR_PRINT("SENSOR_S5K3L2XX: _dw9807_SRCInit 3 fail!");
		}


		cmd_val[0] = 0x07;
		cmd_val[1] = 0x30;
		ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
		if(ret_value){
			SENSOR_PRINT("SENSOR_S5K3L2XX: _dw9807_SRCInit 4 fail!");
		}

	}
		break;
	case 3:
		break;

	}

	return ret_value;
}

static unsigned long _s5k3l2xx_GetResolutionTrimTab(unsigned long param)
{
	SENSOR_PRINT("0x%lx",  (unsigned long)s_s5k3l2xx_Resolution_Trim_Tab);
	return (unsigned long) s_s5k3l2xx_Resolution_Trim_Tab;
}

static unsigned long _s5k3l2xx_PowerOn(unsigned long power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_s5k3l2xx_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_s5k3l2xx_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_s5k3l2xx_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_s5k3l2xx_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_s5k3l2xx_mipi_raw_info.reset_pulse_level;

	uint8_t pid_value = 0x00;

	if (SENSOR_TRUE == power_on) {
		Sensor_SetResetLevel(reset_level);
		//Sensor_PowerDown(power_down);
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		usleep(30*1000);
		//Sensor_PowerDown(!power_down);
		Sensor_SetResetLevel(!reset_level);
		usleep(10*1000);
		//_dw9807_SRCInit(2);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(10*1000);
	} else {
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetResetLevel(reset_level);
		//Sensor_PowerDown(power_down);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
	}
	SENSOR_PRINT_ERR("SENSOR_S5K3L2XX: _s5k3l2xx_Power_On(1:on, 0:off): %d, reset_level %d, dvdd_val %d", power_on, reset_level, dvdd_val);
	return SENSOR_SUCCESS;
}

static uint32_t _s5k3l2xx_cfg_otp(uint32_t  param)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_s5k3l2xx_raw_param_tab;
	uint32_t module_id=g_module_id;

	SENSOR_PRINT("SENSOR_S5K3L2XX: _s5k3l2xx_cfg_otp");

	if(PNULL!=tab_ptr[module_id].cfg_otp){
		tab_ptr[module_id].cfg_otp(0);
		}

	return rtn;
}

static uint32_t _s5k3l2xx_com_Identify_otp(void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_S5K3L2XX: _s5k3l2xx_com_Identify_otp");

	/*read param id from sensor omap*/
	param_id=S5K3L2XX_RAW_PARAM_COM;

	if(S5K3L2XX_RAW_PARAM_COM==param_id){
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}

static uint32_t _s5k3l2xx_GetRawInof(void)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_s5k3l2xx_raw_param_tab;
	uint32_t param_id;
	uint32_t i=0x00;

	/*read param id from sensor omap*/
	param_id=S5K3L2XX_RAW_PARAM_COM;

	for (i=0x00; ; i++) {
		g_module_id = i;
		if (RAW_INFO_END_ID==tab_ptr[i].param_id) {
			if (NULL==s_s5k3l2xx_mipi_raw_info_ptr) {
				SENSOR_PRINT("SENSOR_S5K3L2XX: ov5647_GetRawInof no param error");
				rtn=SENSOR_FAIL;
			}
			SENSOR_PRINT("SENSOR_S5K3L2XX: s5k3l2xx_GetRawInof end");
			break;
		}
		else if (PNULL!=tab_ptr[i].identify_otp) {
			if (SENSOR_SUCCESS==tab_ptr[i].identify_otp(0)) {
				s_s5k3l2xx_mipi_raw_info_ptr = tab_ptr[i].info_ptr;
				SENSOR_PRINT("SENSOR_S5K3L2XX: s5k3l2xx_GetRawInof success");
				break;
			}
		}
	}

	return rtn;
}

static uint32_t _s5k3l2xx_GetMaxFrameLine(uint32_t index)
{
	uint32_t max_line=0x00;
	SENSOR_TRIM_T_PTR trim_ptr=s_s5k3l2xx_Resolution_Trim_Tab;

	max_line=trim_ptr[index].frame_line;

	return max_line;
}

static unsigned long _s5k3l2xx_Identify(unsigned long param)
{
#define S5K3L2XX_PID_ADDR     	0x0000
#define S5K3L2XX_VER_ADDR     	0x0004

#define S5K3L2XX_PID_VALUE    	0x30c2
#define S5K3L2XX_VER_VALUE     	0x0010

		uint32_t pid_value 	= 0x00;
		uint32_t ver_value 	= 0x00;
		uint32_t ret_value 	= SENSOR_FAIL;
	
		SENSOR_PRINT_ERR("SENSOR_S5K3L2XX: mipi raw identify\n");
	
		pid_value = Sensor_ReadReg(S5K3L2XX_PID_ADDR);
		ver_value = (Sensor_ReadReg(S5K3L2XX_VER_ADDR) >> 8) & 0xff;
		SENSOR_PRINT("SENSOR_S5K3L2XX: Identify: PID = %x, VER = %x", pid_value, ver_value);
	
		if (S5K3L2XX_PID_VALUE == pid_value) {
			if (S5K3L2XX_VER_VALUE == ver_value) {
				SENSOR_PRINT_ERR("SENSOR_S5K3L2XX: this is S5K3L2XX sensor !");
				ret_value=_s5k3l2xx_GetRawInof();
				if (SENSOR_SUCCESS != ret_value) {
					SENSOR_PRINT_ERR("SENSOR_S5K3L2XX: the module is unknow error !");
				}
				Sensor_s5k3l2xx_InitRawTuneInfo();
			} else {
				SENSOR_PRINT_ERR("SENSOR_S5K3L2XX: Identify this is hm%x%x sensor !", pid_value, ver_value);
				return ret_value;
			}
		} else {
			SENSOR_PRINT_ERR("SENSOR_S5K3L2XX: identify fail,pid_value=0x%x,0x%x", pid_value,ver_value);
		}


	return ret_value;
}

//uint32_t s_af_step=0x00;
static unsigned long _s5k3l2xx_write_exposure(unsigned long param)
{

	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t expsure_line = 0x00;
	uint16_t dummy_line = 0x00;
	uint16_t frame_len_cur = 0x00;
	uint16_t frame_len = 0x00;
	uint16_t size_index=0x00;
	uint16_t max_frame_len=0x00;

	expsure_line=param&0xffff;
	dummy_line=(param>>0x10)&0x0fff;
	size_index=(param>>0x1c)&0x0f;
	SENSOR_PRINT("_s5k3l2xx: write_exposure line:%d, dummy:%d, size_index:%d", expsure_line, dummy_line, size_index);
	max_frame_len=_s5k3l2xx_GetMaxFrameLine(size_index);
	if(expsure_line < 4){
		expsure_line = 4;
	}

	frame_len = expsure_line + dummy_line;
	frame_len = frame_len > (expsure_line + 4) ? frame_len : (expsure_line + 4);
	frame_len = (frame_len > max_frame_len) ? frame_len : max_frame_len;
	if(0x00!=(0x01&frame_len))
	{
		frame_len+=0x01;
	}

	frame_len_cur = Sensor_ReadReg(0x0340);

	SENSOR_PRINT("SENSOR_s5k3h7yx: write_exposure line:%d, frame_len_cur:%d, frame_len:%d", expsure_line, frame_len_cur, frame_len);


	if(frame_len_cur < frame_len){
		ret_value = Sensor_WriteReg(0x0340, frame_len);
	}
	
	ret_value = Sensor_WriteReg(0x202, expsure_line);
	
	if(frame_len_cur > frame_len){
		ret_value = Sensor_WriteReg(0x0340, frame_len);
	}
	return ret_value;

}

static unsigned long _s5k3l2xx_write_gain(unsigned long param)
{

	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint32_t real_gain = 0;

	real_gain = ((param&0xf)+16)*(((param>>4)&0x01)+1)*(((param>>5)&0x01)+1);
	real_gain = real_gain*(((param>>6)&0x01)+1)*(((param>>7)&0x01)+1)*(((param>>8)&0x01)+1);

//	real_gain = real_gain<<1;
	real_gain = param/4;

	SENSOR_PRINT("_s5k3l2xx: real_gain:0x%x, param: 0x%x", real_gain, param);

	//ret_value = Sensor_WriteReg(0x104, 0x01);
	value = real_gain;
	ret_value = Sensor_WriteReg(0x204, value);
	//ret_value = Sensor_WriteReg(0x104, 0x00);
	return ret_value;

}

static unsigned long _s5k3l2xx_write_af(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint8_t cmd_val[2] = {0x00};
	uint16_t slave_addr = 0;
	uint16_t cmd_len = 0;

	SENSOR_PRINT("SENSOR_S5K3L2XX: _write_af %ld", param);
	slave_addr = DW9807_VCM_SLAVE_ADDR;

	cmd_val[0] = 0x03;
	cmd_val[1] = (param>>8)&0x03;
	cmd_len = 2;
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
	SENSOR_PRINT("SENSOR_S5K3L2XX: _write_af, ret =  %d, MSL:%x, LSL:%x\n", ret_value, cmd_val[0], cmd_val[1]);


	cmd_val[0] = 0x04;
	cmd_val[1] = param&0xff;
	cmd_len = 2;
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

	SENSOR_PRINT("SENSOR_S5K3L2XX: _write_af, ret =  %d, MSL:%x, LSL:%x\n", ret_value, cmd_val[0], cmd_val[1]);
	return ret_value;
}

static unsigned long _s5k3l2xx_BeforeSnapshot(unsigned long param)
{

	uint16_t ret;
	uint32_t cap_mode = (param>>CAP_MODE_BITS);
	uint32_t capture_exposure;
	uint32_t preview_exposure;
	uint32_t prv_linetime=s_s5k3l2xx_Resolution_Trim_Tab[SENSOR_MODE_PREVIEW_ONE].line_time;
	uint32_t cap_linetime;
	uint32_t frame_len = 0x00;
	uint32_t gain = 0;

	param = param&0xffff;
	SENSOR_PRINT("SENSOR_s5k3h7yx:cap_mode = %d,param = %d.",cap_mode,param);
	cap_linetime = s_s5k3l2xx_Resolution_Trim_Tab[param].line_time;


	SENSOR_PRINT("SENSOR_s5k3h7yx: BeforeSnapshot moe: %d",param);

	if (SENSOR_MODE_PREVIEW_ONE >= param){
		SENSOR_PRINT("SENSOR_s5k3h7yx: prvmode equal to capmode");
		return SENSOR_SUCCESS;
	}


	preview_exposure = Sensor_ReadReg(0x202);
	_s5k3l2xx_ReadGain(&gain);

	Sensor_SetMode(param);
	Sensor_SetMode_WaitDone();

	capture_exposure = preview_exposure * prv_linetime / cap_linetime;

	frame_len = Sensor_ReadReg(0x340);
	
	while(gain >= 0x40){
		capture_exposure = capture_exposure * 2;
		gain=gain / 2;
		if(capture_exposure > frame_len*2)
			break;
	}
	SENSOR_PRINT("SENSOR_s5k3h7yx: cap moe: %d,FL: %x,exp=%d,g=%x",param,frame_len,capture_exposure,gain);

	if(capture_exposure >= (frame_len - 4)){
		frame_len = capture_exposure+4;
		frame_len = ((frame_len+1)>>1)<<1;
		Sensor_WriteReg(0x0340, frame_len);
	}
	Sensor_WriteReg(0x202, capture_exposure);
	Sensor_WriteReg(0x204, gain);

	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, capture_exposure);

	return SENSOR_SUCCESS;

}

static unsigned long _s5k3l2xx_after_snapshot(unsigned long param)
{
	SENSOR_PRINT("SENSOR_s5k3l2xx: after_snapshot mode:%ld", param);
	Sensor_SetMode((uint32_t)param);
	return SENSOR_SUCCESS;
}

static unsigned long _s5k3l2xx_flash(unsigned long param)
{
	SENSOR_PRINT("Start:param=%ld", param);

	/* enable flash, disable in _s5k3l2xx_BeforeSnapshot */
	g_flash_mode_en = (uint32_t)param;
	Sensor_SetFlash((uint32_t)param);
	SENSOR_PRINT_HIGH("end");
	return SENSOR_SUCCESS;
}

static unsigned long _s5k3l2xx_GetExifInfo(unsigned long param)
{
	LOCAL EXIF_SPEC_PIC_TAKING_COND_T sexif;

	sexif.ExposureTime.numerator = s_exposure_time;
	sexif.ExposureTime.denominator = 1000000;

	return (unsigned long) & sexif;
}

static unsigned long _s5k3l2xx_StreamOn(unsigned long param)
{
	uint32_t value = 0;
	SENSOR_PRINT_ERR("SENSOR_s5k3l2xx: StreamOn");
	Sensor_WriteReg(0x0100, 0x0100);
	usleep(50*1000);
	return 0;
}

static unsigned long _s5k3l2xx_StreamOff(unsigned long param)
{
	SENSOR_PRINT_ERR("SENSOR_s5k3l2xx: StreamOff");
	Sensor_WriteReg(0x0100, 0x0000);
	usleep(50*1000);
	return 0;
}

static uint16_t _s5k3l2xx_get_shutter(void)
{
	// read shutter, in number of line period
	uint16_t shutter_h = 0;
	uint16_t shutter_l = 0;
#if 1  // MP tool //!??
	shutter_h = Sensor_ReadReg(0x0202) & 0xff;
	shutter_l = Sensor_ReadReg(0x0203) & 0xff;

	return (shutter_h << 8) | shutter_l;
#else
	return s_set_exposure;
#endif
}

static uint32_t _s5k3l2xx_set_shutter(uint16_t shutter)
{
	uint16_t temp1,temp2;
	Sensor_WriteReg(0x104, 0x0100);
	// write shutter, in number of line period
//	Sensor_WriteReg(0x0202, (shutter >> 8) & 0xff);
//	Sensor_WriteReg(0x0203, shutter & 0xff);
	temp1=(shutter<<8)&0xff00;
	temp2=(shutter>>8)&0xff;
	shutter=temp1|temp2;
	Sensor_WriteReg(0x0202, shutter);
	Sensor_WriteReg(0x104, 0x00);

	return 0;
}

static uint32_t _s5k3l2xx_get_gain16(void)
{
	// read gain, 16 = 1x
	uint32_t gain16;

	gain16 = (256*16)/(256 - Sensor_ReadReg(0x0157)); // a_gain= 256/(256-x);

	return gain16;
}

static uint16_t _s5k3l2xx_set_gain16(uint32_t gain16)
{
	// write gain, 16 = 1x
	uint16_t temp;
	gain16 = gain16 & 0x3ff;

	if (gain16 < 16)
		gain16 = 16;
	if (gain16 > 170)
		gain16 = 170;

	temp = (256*(gain16- 16))/gain16;
	Sensor_WriteReg(0x0157, temp&0xff);

	return 0;
}

//uint32_t s_af_step=0x00;
static unsigned long _s5k3l2xx_write_exposure_ev(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t expsure_line = 0x00;
	uint16_t dummy_line = 0x00;
	uint16_t frame_len_cur = 0x00;
	uint16_t frame_len = 0x00;
	uint16_t size_index=0x00;
	uint16_t max_frame_len=0x00;

	expsure_line=param;
	s_set_exposure = expsure_line;
	SENSOR_PRINT("SENSOR_s5k3h2yx: write_exposure line:%d, dummy:%d, size_index:%d", expsure_line, dummy_line, size_index);
	max_frame_len=2480;
	if (expsure_line < 3) {
		expsure_line = 3;
	}

	frame_len = 2480;
	frame_len = frame_len > (expsure_line + 8) ? frame_len : (expsure_line + 8);
	if (0x00!=(0x01&frame_len)) {
		frame_len+=0x01;
	}

	frame_len_cur = (Sensor_ReadReg(0x0341))&0xff;
	frame_len_cur |= (Sensor_ReadReg(0x0340)<<0x08)&0xff00;


	ret_value = Sensor_WriteReg(0x104, 0x01);
	if (frame_len_cur != frame_len) {
		ret_value = Sensor_WriteReg(0x0341, frame_len & 0xff);
		ret_value = Sensor_WriteReg(0x0340, (frame_len >> 0x08) & 0xff);
	}

	ret_value = Sensor_WriteReg(0x203, expsure_line & 0xff);
	ret_value = Sensor_WriteReg(0x202, (expsure_line >> 0x08) & 0xff);
	ret_value = Sensor_WriteReg(0x104, 0x00);
	return ret_value;

}
static void _calculate_hdr_exposure(int capture_gain16,int capture_VTS, int capture_shutter)
{
	// write capture gain
	//_s5k3l2xx_set_gain16(capture_gain16);

	// write capture shutter
	/*if (capture_shutter > (capture_VTS - 4)) {
		capture_VTS = capture_shutter + 4;
		OV5640_set_VTS(capture_VTS);
	}*/
	//_s5k3l2xx_set_shutter(capture_shutter);
	_s5k3l2xx_write_exposure_ev(capture_shutter);
}

static uint32_t _s5k3l2xx_SetEV(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	uint16_t value=0x00;
	uint32_t gain = s_s5k3l2xx_gain;
	uint32_t ev = ext_ptr->param;

	SENSOR_PRINT_HIGH("SENSOR: _s5k3l2xx_SetEV param: 0x%x s_capture_shutter %d", ext_ptr->param, s_capture_shutter);

	switch(ev) {
	case SENSOR_HDR_EV_LEVE_0:
		_calculate_hdr_exposure(s_s5k3l2xx_gain/4,s_capture_VTS,s_capture_shutter/2);
		break;
	case SENSOR_HDR_EV_LEVE_1:
		_calculate_hdr_exposure(s_s5k3l2xx_gain,s_capture_VTS,s_capture_shutter);
		break;
	case SENSOR_HDR_EV_LEVE_2:
		_calculate_hdr_exposure(s_s5k3l2xx_gain*3/2,s_capture_VTS,s_capture_shutter*2);
		break;
	default:
		break;
	}
	/*usleep(50000);*/
	return rtn;
}

static unsigned long _s5k3l2xx_ExtFunc(unsigned long ctl_param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr =
	    (SENSOR_EXT_FUN_PARAM_T_PTR) ctl_param;
	SENSOR_PRINT_HIGH("0x%x", ext_ptr->cmd);

	switch (ext_ptr->cmd) {
	case SENSOR_EXT_FUNC_INIT:
		break;
	case SENSOR_EXT_FOCUS_START:
		break;
	case SENSOR_EXT_EXPOSURE_START:
		break;
	case SENSOR_EXT_EV:
		rtn = _s5k3l2xx_SetEV(ctl_param);
		break;
	default:
		break;
	}
	return rtn;
}

static uint16_t _s5k3l2xx_get_VTS(void)
{
	// read VTS from register settings
	uint16_t VTS;

	VTS = Sensor_ReadReg(0x0160);				//total vertical size[15:8] high byte
	VTS = (VTS<<8) + Sensor_ReadReg(0x0161);

	return VTS;
}

static uint32_t _s5k3l2xx_set_VTS(uint16_t VTS)
{
// write VTS to registers
	uint16_t temp1,temp2;
//	Sensor_WriteReg(0x0161, (VTS & 0xff));
//	Sensor_WriteReg(0x0160, ((VTS>>8)& 0xff));
	temp1=(VTS<<8)&0xff00;
	temp2=(VTS>>8)&0xff;
	VTS=temp1|temp2;
	Sensor_WriteReg(0x0160, VTS);

	return 0;
}

static uint32_t _s5k3l2xx_ReadGain(uint32_t* param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	uint32_t gain = 0;

	gain = Sensor_ReadReg(0x204);/*8*/
	s_s5k3l2xx_gain=(int)gain;
	SENSOR_PRINT("SENSOR_s5k3l2xx: _s5k3l2xx_ReadGain gain: 0x%x", s_s5k3l2xx_gain);
	return rtn;

}

static uint32_t _s5k3l2xx_write_otp_gain(uint32_t *param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t value = 0x00;

	SENSOR_PRINT("SENSOR_s5k3l2xx: write_gain:0x%x\n", *param);

	//ret_value = Sensor_WriteReg(0x104, 0x01);
	value = (*param)>>0x08;
	ret_value = Sensor_WriteReg(0x204, value);
	value = (*param)&0xff;
	ret_value = Sensor_WriteReg(0x205, value);
	ret_value = Sensor_WriteReg(0x104, 0x00);

	return ret_value;
}

static uint32_t _s5k3l2xx_read_otp_gain(uint32_t *param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	uint16_t gain_h = 0;
	uint16_t gain_l = 0;
	#if 1 // for MP tool //!??
	gain_h = Sensor_ReadReg(0x0204) & 0xff;
	gain_l = Sensor_ReadReg(0x0205) & 0xff;
	*param = ((gain_h << 8) | gain_l);
	#else
	uint8_t cmd_val[3]            = {0x00};
	uint16_t cmd_len;

	cmd_len = 2;
	cmd_val[0] = 0x02;
	cmd_val[1] = 0x04;
	rtn = Sensor_ReadI2C(S5K3L2XX_I2C_ADDR_W,(uint8_t*)&cmd_val[0], cmd_len);
	if (SENSOR_SUCCESS == rtn) {
		gain_h = (cmd_val[0]) & 0xff;
	}
	cmd_val[0] = 0x02;
	cmd_val[1] = 0x05;
	rtn = Sensor_ReadI2C(S5K3L2XX_I2C_ADDR_W,(uint8_t*)&cmd_val[0], cmd_len);
	if (SENSOR_SUCCESS == rtn) {
		gain_l = (cmd_val[0]) & 0xff;
	}
	*param = ((gain_h << 8) | gain_l);
	//*param = s_set_gain;
	#endif
	SENSOR_PRINT("SENSOR_s5k3l2xx: gain: %d", *param);

	return rtn;
}

static uint32_t _s5k3l2xx_write_vcm(uint32_t *param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint8_t cmd_val[2] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;

	slave_addr = DW9807_VCM_SLAVE_ADDR;

	cmd_len = 2;
	cmd_val[0] = ((*param)>>16) & 0xff;
	cmd_val[1] = (*param) & 0xff;
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

	SENSOR_PRINT("SENSOR_s5k3l2xx: _write_vcm, ret =  %d, MSL:%x, LSL:%x\n", ret_value, cmd_val[0], cmd_val[1]);

	return ret_value;
}

static uint32_t _s5k3l2xx_read_vcm(uint32_t *param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint8_t cmd_val[2] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;

	slave_addr = DW9807_VCM_SLAVE_ADDR;

	cmd_len = 1;
	cmd_val[0] = ((*param)>>16) & 0xff;
	ret_value = (uint32_t)Sensor_ReadI2C(slave_addr,(cmr_u8*)&cmd_val[0], cmd_len);
	if (SENSOR_SUCCESS == ret_value)
		*param |= cmd_val[0];

	SENSOR_PRINT("SENSOR_s5k3l2xx: _read_vcm, ret =  %d, MSL:%x, LSL:%x\n", ret_value, cmd_val[0], cmd_val[1]);

	return ret_value;
}

LOCAL uint32_t _s5k3l2xx_write_otp(unsigned long param)
{
	uint32_t rtn=SENSOR_SUCCESS;
	SENSOR_OTP_PARAM_T* param_ptr = (SENSOR_OTP_PARAM_T*)param;
	uint32_t start_addr = param_ptr->start_addr;
	uint32_t len  		= param_ptr->len;
	uint8_t *buff 		= param_ptr->buff;

	uint8_t cmd_val[3] = {0x00};
	uint16_t cmd_len;
	uint32_t i;

	CMR_LOGI("SENSOR_s5k3l2xx: _s5k3l2xx_write_otp E");

	// EEPROM Erase
	cmd_len = 2;
	cmd_val[0] = 0x81;
	cmd_val[1] = 0xEE;
	Sensor_WriteI2C(DW9807_EEPROM_SLAVE_ADDR,(uint8_t*)&cmd_val[0], cmd_len);
	usleep(1000*1000);

	// EEPROM Write
	cmd_len = 3;
	for (i = 0; i < len; i++) {
		cmd_val[0] = ((start_addr + i) >> 8) & 0xff;
		cmd_val[1] = (start_addr + i) & 0xff;
		cmd_val[2] = buff[i];
		Sensor_WriteI2C(DW9807_EEPROM_SLAVE_ADDR,(uint8_t*)&cmd_val[0], cmd_len);
	}

	CMR_LOGI("SENSOR_s5k3l2xx: _s5k3l2xx_write_otp X");

	return rtn;
}

LOCAL uint32_t _s5k3l2xx_read_otp(unsigned long param)
{
	uint32_t rtn=SENSOR_SUCCESS;
	SENSOR_OTP_PARAM_T* param_ptr = (SENSOR_OTP_PARAM_T*)param;
	uint32_t start_addr           = param_ptr->start_addr;
	uint32_t len                  = 8192;
	uint8_t *buff                 = param_ptr->buff;
	uint16_t tmp;
	uint8_t cmd_val[3]            = {0x00};
	uint16_t cmd_len;
	uint32_t i;

	CMR_LOGI("SENSOR_s5k3l2xx: _s5k3l2xx_read_otp E");

	cmd_len = 2;
	cmd_val[0] = 0x00;
	cmd_val[1] = 0x00;
	rtn = Sensor_WriteI2C(DW9807_EEPROM_SLAVE_ADDR,(uint8_t*)&cmd_val[0], cmd_len);
	usleep(1000*1000);

	CMR_LOGI("write i2c rtn %d", rtn);

	// EEPROM READ
	for(i = 0;i < len; i++) {
		cmd_val[0] = ((start_addr + i) >> 8) & 0xff;
		cmd_val[1] = (start_addr + i) & 0xff;
		rtn = Sensor_ReadI2C(DW9807_EEPROM_SLAVE_ADDR,(uint8_t*)&cmd_val[0], cmd_len);
		if (SENSOR_SUCCESS == rtn) {
			buff[i] = (cmd_val[0]) & 0xff;
		}
		//CMR_LOGI("rtn %d value %d addr 0x%x", rtn, buff[i], start_addr + i);
	}
	param_ptr->len = len;

	if (NULL != param_ptr->awb.data_ptr) {
		uint32_t awb_src_addr = 0x900;
		uint32_t lsc_src_addr = 0xA00;
		uint32_t real_size = 0;
		rtn = awb_package_convert((void *)(buff + awb_src_addr), param_ptr->awb.data_ptr,
			param_ptr->awb.size, &real_size);
		param_ptr->awb.size = real_size;
		CMR_LOGI("SENSOR_s5k3l2xx: awb real_size %d", real_size);
		rtn = lsc_package_convert((void *)(buff + lsc_src_addr), param_ptr->lsc.data_ptr,
			param_ptr->lsc.size, &real_size);
		param_ptr->lsc.size = real_size;
		CMR_LOGI("SENSOR_s5k3l2xx: lsc real_size %d", real_size);
	}
	CMR_LOGI("SENSOR_s5k3l2xx: _s5k3l2xx_read_otp X");

	return rtn;

}

static uint32_t _s5k3l2xx_get_golden_data(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_OTP_PARAM_T* param_ptr = (SENSOR_OTP_PARAM_T*)param;

//	param_ptr->golden.data_ptr = (void*)golden_data;
//	param_ptr->golden.size = sizeof(golden_data);

	SENSOR_PRINT("SENSOR_s5k3l2xx: golden: %d", param_ptr->golden.size);

	return rtn;
}

static unsigned long _s5k3l2xx_access_val(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_VAL_T* param_ptr = (SENSOR_VAL_T*)param;
	uint16_t tmp;

	SENSOR_PRINT("SENSOR_s5k3l2xx: _s5k3l2xx_access_val E");
	if(!param_ptr){
		return rtn;
	}

	SENSOR_PRINT("SENSOR_s5k3l2xx: param_ptr->type=%x", param_ptr->type);
	switch(param_ptr->type)
	{
		case SENSOR_VAL_TYPE_SHUTTER:
			*((uint32_t*)param_ptr->pval) = _s5k3l2xx_get_shutter();
			break;
		case SENSOR_VAL_TYPE_READ_VCM:
			rtn = _s5k3l2xx_read_vcm(param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_WRITE_VCM:
			rtn = _s5k3l2xx_write_vcm(param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_READ_OTP:
			rtn = _s5k3l2xx_read_otp((unsigned long)param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_WRITE_OTP:
			rtn = _s5k3l2xx_write_otp((unsigned long)param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_GET_RELOADINFO:
			{
//				struct isp_calibration_info **p= (struct isp_calibration_info **)param_ptr->pval;
//				*p=&calibration_info;
			}
			break;
		case SENSOR_VAL_TYPE_GET_AFPOSITION:
			*(uint32_t*)param_ptr->pval = 0;//cur_af_pos;
			break;
		case SENSOR_VAL_TYPE_WRITE_OTP_GAIN:
			rtn = _s5k3l2xx_write_otp_gain(param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_READ_OTP_GAIN:
			rtn = _s5k3l2xx_read_otp_gain(param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_GET_GOLDEN_DATA:
			rtn = _s5k3l2xx_get_golden_data((unsigned long)param_ptr->pval);
			break;
		default:
			break;
	}

	SENSOR_PRINT("SENSOR_s5k3l2xx: _s5k3l2xx_access_val X");

	return rtn;
}
