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
#include "sensor_s5k3h7yx_raw_param.c"


#define s5k3h7yx_I2C_ADDR_W        0x10
#define s5k3h7yx_I2C_ADDR_R         0x10


#define DW9714_VCM_SLAVE_ADDR (0x18>>1)

#define s5k3h7yx_MIN_FRAME_LEN_PRV  0x4F8
#define s5k3h7yx_MIN_FRAME_LEN_CAP  0x9B5
#define s5k3h7yx_RAW_PARAM_COM  0x0000

LOCAL uint32_t _s5k3h7yx_GetResolutionTrimTab(uint32_t param);
LOCAL uint32_t _s5k3h7yx_PowerOn(uint32_t power_on);
LOCAL uint32_t _s5k3h7yx_Identify(uint32_t param);
LOCAL uint32_t _s5k3h7yx_BeforeSnapshot(uint32_t param);
LOCAL uint32_t _s5k3h7yx_after_snapshot(uint32_t param);
LOCAL uint32_t _s5k3h7yx_StreamOn(uint32_t param);
LOCAL uint32_t _s5k3h7yx_StreamOff(uint32_t param);
LOCAL uint32_t _s5k3h7yx_write_exposure(uint32_t param);
LOCAL uint32_t _s5k3h7yx_write_gain(uint32_t param);
LOCAL uint32_t _s5k3h7yx_write_af(uint32_t param);
LOCAL uint32_t _s5k3h7yx_ReadGain(uint32_t*  gain_ptr);
LOCAL uint32_t _dw9174_SRCInit(uint32_t mode);
LOCAL uint32_t _s5k3h7yx_com_Identify_otp(void* param_ptr);

LOCAL const struct raw_param_info_tab s_s5k3h7yx_raw_param_tab[]={
	{s5k3h7yx_RAW_PARAM_COM, &s_s5k3h7yx_mipi_raw_info, _s5k3h7yx_com_Identify_otp, PNULL},
	{RAW_INFO_END_ID, PNULL, PNULL, PNULL}
};

static uint32_t s_s5k3h7yx_gain = 0;
static uint32_t g_module_id = 0;

LOCAL const SENSOR_REG_T s5k3h7yx_com_mipi_raw[] = {
	{0x6010, 0x0001},	// Reset
	{SENSOR_WRITE_DELAY, 0x03},
	{0x6028, 0x7000},
	{0x602A, 0x1750},
	{0x6F12, 0x10B5},
	{0x6F12, 0x00F0},
	{0x6F12, 0xE1FB},
	{0x6F12, 0x00F0},
	{0x6F12, 0xE3FB},
	{0x6F12, 0x10BC},
	{0x6F12, 0x08BC},
	{0x6F12, 0x1847},
	{0x6F12, 0x2DE9},
	{0x6F12, 0x7040},
	{0x6F12, 0x9FE5},
	{0x6F12, 0x3867},
	{0x6F12, 0xA0E3},
	{0x6F12, 0x0140},
	{0x6F12, 0xD6E1},
	{0x6F12, 0xB010},
	{0x6F12, 0x9FE5},
	{0x6F12, 0x3057},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x1421},
	{0x6F12, 0x81E2},
	{0x6F12, 0x0110},
	{0x6F12, 0x82E1},
	{0x6F12, 0x1411},
	{0x6F12, 0xD5E1},
	{0x6F12, 0xB020},
	{0x6F12, 0xC2E1},
	{0x6F12, 0x0110},
	{0x6F12, 0xC5E1},
	{0x6F12, 0xB010},
	{0x6F12, 0x00EB},
	{0x6F12, 0xE601},
	{0x6F12, 0xD6E1},
	{0x6F12, 0xB000},
	{0x6F12, 0x9FE5},
	{0x6F12, 0x1827},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x1410},
	{0x6F12, 0x80E2},
	{0x6F12, 0x0100},
	{0x6F12, 0x81E1},
	{0x6F12, 0x1400},
	{0x6F12, 0xD5E1},
	{0x6F12, 0xB010},
	{0x6F12, 0x80E1},
	{0x6F12, 0x0100},
	{0x6F12, 0xC5E1},
	{0x6F12, 0xB000},
	{0x6F12, 0x9FE5},
	{0x6F12, 0xF406},
	{0x6F12, 0xD0E1},
	{0x6F12, 0xB00C},
	{0x6F12, 0xA0E1},
	{0x6F12, 0xA011},
	{0x6F12, 0x9FE5},
	{0x6F12, 0xEC06},
	{0x6F12, 0x90E5},
	{0x6F12, 0x0000},
	{0x6F12, 0xD0E1},
	{0x6F12, 0xBA39},
	{0x6F12, 0x53E1},
	{0x6F12, 0x0100},
	{0x6F12, 0xD091},
	{0x6F12, 0xBE09},
	{0x6F12, 0xD081},
	{0x6F12, 0xBC09},
	{0x6F12, 0xC2E1},
	{0x6F12, 0xB003},
	{0x6F12, 0xBDE8},
	{0x6F12, 0x7040},
	{0x6F12, 0x2FE1},
	{0x6F12, 0x1EFF},
	{0x6F12, 0x2DE9},
	{0x6F12, 0x3840},
	{0x6F12, 0x10E3},
	{0x6F12, 0x0100},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x0050},
	{0x6F12, 0x9F15},
	{0x6F12, 0xC406},
	{0x6F12, 0x9015},
	{0x6F12, 0x2400},
	{0x6F12, 0x5013},
	{0x6F12, 0x0000},
	{0x6F12, 0x000A},
	{0x6F12, 0x1900},
	{0x6F12, 0x9FE5},
	{0x6F12, 0xB846},
	{0x6F12, 0xD4E5},
	{0x6F12, 0xD700},
	{0x6F12, 0x50E3},
	{0x6F12, 0x0000},
	{0x6F12, 0x001A},
	{0x6F12, 0x0600},
	{0x6F12, 0xA0E3},
	{0x6F12, 0x0120},
	{0x6F12, 0x8DE2},
	{0x6F12, 0x0010},
	{0x6F12, 0xA0E3},
	{0x6F12, 0x2F00},
	{0x6F12, 0x00EB},
	{0x6F12, 0xC501},
	{0x6F12, 0xDDE5},
	{0x6F12, 0x0000},
	{0x6F12, 0xA0E1},
	{0x6F12, 0xA001},
	{0x6F12, 0xC4E5},
	{0x6F12, 0xD700},
	{0x6F12, 0x9FE5},
	{0x6F12, 0x8046},
	{0x6F12, 0x94E5},
	{0x6F12, 0x0000},
	{0x6F12, 0xD0E5},
	{0x6F12, 0x1102},
	{0x6F12, 0x50E3},
	{0x6F12, 0x0000},
	{0x6F12, 0x001A},
	{0x6F12, 0x0900},
	{0x6F12, 0xA0E3},
	{0x6F12, 0x0120},
	{0x6F12, 0x8DE2},
	{0x6F12, 0x0010},
	{0x6F12, 0xA0E3},
	{0x6F12, 0x3700},
	{0x6F12, 0x00EB},
	{0x6F12, 0xB901},
	{0x6F12, 0xDDE5},
	{0x6F12, 0x0010},
	{0x6F12, 0x94E5},
	{0x6F12, 0x0000},
	{0x6F12, 0xC0E5},
	{0x6F12, 0x1112},
	{0x6F12, 0x01E2},
	{0x6F12, 0xFF00},
	{0x6F12, 0x9FE5},
	{0x6F12, 0x4816},
	{0x6F12, 0xC1E1},
	{0x6F12, 0xBE04},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x0500},
	{0x6F12, 0xBDE8},
	{0x6F12, 0x3840},
	{0x6F12, 0x00EA},
	{0x6F12, 0xB201},
	{0x6F12, 0x9FE5},
	{0x6F12, 0x3416},
	{0x6F12, 0xA0E3},
	{0x6F12, 0x0000},
	{0x6F12, 0xA0E3},
	{0x6F12, 0x012C},
	{0x6F12, 0x81E0},
	{0x6F12, 0x8030},
	{0x6F12, 0x83E2},
	{0x6F12, 0x013C},
	{0x6F12, 0x80E2},
	{0x6F12, 0x0100},
	{0x6F12, 0x50E3},
	{0x6F12, 0x0400},
	{0x6F12, 0xC3E1},
	{0x6F12, 0xBE28},
	{0x6F12, 0xFFBA},
	{0x6F12, 0xF9FF},
	{0x6F12, 0x2FE1},
	{0x6F12, 0x1EFF},
	{0x6F12, 0x2DE9},
	{0x6F12, 0x7040},
	{0x6F12, 0x9FE5},
	{0x6F12, 0x08C6},
	{0x6F12, 0xDCE5},
	{0x6F12, 0x1021},
	{0x6F12, 0x52E3},
	{0x6F12, 0x0000},
	{0x6F12, 0x001A},
	{0x6F12, 0x0A00},
	{0x6F12, 0x9FE5},
	{0x6F12, 0x0CE6},
	{0x6F12, 0x8CE0},
	{0x6F12, 0x0231},
	{0x6F12, 0x8EE0},
	{0x6F12, 0x8250},
	{0x6F12, 0xD5E1},
	{0x6F12, 0xB050},
	{0x6F12, 0x93E5},
	{0x6F12, 0xD840},
	{0x6F12, 0x82E2},
	{0x6F12, 0x0120},
	{0x6F12, 0x04E0},
	{0x6F12, 0x9504},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x2444},
	{0x6F12, 0x52E3},
	{0x6F12, 0x0400},
	{0x6F12, 0x83E5},
	{0x6F12, 0xD840},
	{0x6F12, 0xFFBA},
	{0x6F12, 0xF5FF},
	{0x6F12, 0xBDE8},
	{0x6F12, 0x7040},
	{0x6F12, 0x00EA},
	{0x6F12, 0x9801},
	{0x6F12, 0x2DE9},
	{0x6F12, 0x1040},
	{0x6F12, 0x00EB},
	{0x6F12, 0x9801},
	{0x6F12, 0x9FE5},
	{0x6F12, 0xD405},
	{0x6F12, 0xD0E5},
	{0x6F12, 0x7310},
	{0x6F12, 0xBDE8},
	{0x6F12, 0x1040},
	{0x6F12, 0x9FE5},
	{0x6F12, 0xCC05},
	{0x6F12, 0xFFEA},
	{0x6F12, 0xE6FF},
	{0x6F12, 0x2DE9},
	{0x6F12, 0xFF4F},
	{0x6F12, 0x9FE5},
	{0x6F12, 0xB445},
	{0x6F12, 0x4DE2},
	{0x6F12, 0xA4D0},
	{0x6F12, 0xD4E1},
	{0x6F12, 0xB20D},
	{0x6F12, 0xD4E5},
	{0x6F12, 0x9CA0},
	{0x6F12, 0xA0E3},
	{0x6F12, 0x0150},
	{0x6F12, 0x8DE5},
	{0x6F12, 0x5800},
	{0x6F12, 0xD4E1},
	{0x6F12, 0xB40D},
	{0x6F12, 0x5AE3},
	{0x6F12, 0x1000},
	{0x6F12, 0xA023},
	{0x6F12, 0x10A0},
	{0x6F12, 0x8DE5},
	{0x6F12, 0x5400},
	{0x6F12, 0xD4E5},
	{0x6F12, 0xDB00},
	{0x6F12, 0xD4E5},
	{0x6F12, 0xD710},
	{0x6F12, 0xA0E3},
	{0x6F12, 0x2020},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x1500},
	{0x6F12, 0x81E2},
	{0x6F12, 0x0310},
	{0x6F12, 0x01E2},
	{0x6F12, 0xFF70},
	{0x6F12, 0xA0E3},
	{0x6F12, 0x0010},
	{0x6F12, 0x8DE5},
	{0x6F12, 0xA000},
	{0x6F12, 0xCDE1},
	{0x6F12, 0xBC07},
	{0x6F12, 0xCDE1},
	{0x6F12, 0xBC05},
	{0x6F12, 0x8DE5},
	{0x6F12, 0x4C10},
	{0x6F12, 0x9DE5},
	{0x6F12, 0xB000},
	{0x6F12, 0x8DE5},
	{0x6F12, 0x5010},
	{0x6F12, 0xD0E1},
	{0x6F12, 0xF600},
	{0x6F12, 0x8DE5},
	{0x6F12, 0x4800},
	{0x6F12, 0x9DE5},
	{0x6F12, 0xB000},
	{0x6F12, 0xD4E5},
	{0x6F12, 0xD910},
	{0x6F12, 0xD0E5},
	{0x6F12, 0x0800},
	{0x6F12, 0x80E0},
	{0x6F12, 0x0100},
	{0x6F12, 0x8DE5},
	{0x6F12, 0x4400},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x1500},
	{0x6F12, 0x80E0},
	{0x6F12, 0xA00F},
	{0x6F12, 0xA0E1},
	{0x6F12, 0xC000},
	{0x6F12, 0x8DE5},
	{0x6F12, 0x4000},
	{0x6F12, 0x9FE5},
	{0x6F12, 0x3C15},
	{0x6F12, 0x8DE2},
	{0x6F12, 0x2000},
	{0x6F12, 0x00EB},
	{0x6F12, 0x6F01},
	{0x6F12, 0x9FE5},
	{0x6F12, 0x3415},
	{0x6F12, 0xA0E3},
	{0x6F12, 0x1820},
	{0x6F12, 0x8DE2},
	{0x6F12, 0x0800},
	{0x6F12, 0x00EB},
	{0x6F12, 0x6B01},
	{0x6F12, 0x9FE5},
	{0x6F12, 0x2825},
	{0x6F12, 0xA0E3},
	{0x6F12, 0x0000},
	{0x6F12, 0x8DE5},
	{0x6F12, 0x0400},
	{0x6F12, 0x92E5},
	{0x6F12, 0x0020},
	{0x6F12, 0x9FE5},
	{0x6F12, 0xF004},
	{0x6F12, 0xD2E5},
	{0x6F12, 0x5921},
	{0x6F12, 0x90E5},
	{0x6F12, 0x4010},
	{0x6F12, 0x9DE5},
	{0x6F12, 0xAC30},
	{0x6F12, 0x82E0},
	{0x6F12, 0x8221},
	{0x6F12, 0x81E0},
	{0x6F12, 0x8210},
	{0x6F12, 0x81E0},
	{0x6F12, 0x8310},
	{0x6F12, 0xD1E1},
	{0x6F12, 0xFA30},
	{0x6F12, 0xD0E1},
	{0x6F12, 0xBE04},
	{0x6F12, 0xD1E1},
	{0x6F12, 0xF210},
	{0x6F12, 0x60E2},
	{0x6F12, 0x012C},
	{0x6F12, 0x02E0},
	{0x6F12, 0x9302},
	{0x6F12, 0x20E0},
	{0x6F12, 0x9120},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x0004},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x4008},
	{0x6F12, 0x8DE5},
	{0x6F12, 0x0000},
	{0x6F12, 0x9FE5},
	{0x6F12, 0xC084},
	{0x6F12, 0x9DE5},
	{0x6F12, 0x5000},
	{0x6F12, 0x9DE5},
	{0x6F12, 0x5410},
	{0x6F12, 0x88E0},
	{0x6F12, 0x8000},
	{0x6F12, 0xD0E1},
	{0x6F12, 0xFC0B},
	{0x6F12, 0xA0E3},
	{0x6F12, 0x0150},
	{0x6F12, 0x00E0},
	{0x6F12, 0x9100},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x0064},
	{0x6F12, 0xB0E1},
	{0x6F12, 0x4668},
	{0x6F12, 0x9DE5},
	{0x6F12, 0xA000},
	{0x6F12, 0xA053},
	{0x6F12, 0x0210},
	{0x6F12, 0xE043},
	{0x6F12, 0x0110},
	{0x6F12, 0x00EB},
	{0x6F12, 0x4C01},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x0098},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x4998},
	{0x6F12, 0xA0E3},
	{0x6F12, 0x0140},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x05B0},
	{0x6F12, 0x00EA},
	{0x6F12, 0x0800},
	{0x6F12, 0x8DE2},
	{0x6F12, 0x5C00},
	{0x6F12, 0x80E0},
	{0x6F12, 0x8450},
	{0x6F12, 0x55E1},
	{0x6F12, 0xF200},
	{0x6F12, 0x9DE5},
	{0x6F12, 0xA010},
	{0x6F12, 0x00E0},
	{0x6F12, 0x9600},
	{0x6F12, 0x89E0},
	{0x6F12, 0x8000},
	{0x6F12, 0x00EB},
	{0x6F12, 0x4001},
	{0x6F12, 0x84E2},
	{0x6F12, 0x0140},
	{0x6F12, 0xC5E1},
	{0x6F12, 0xB000},
	{0x6F12, 0x54E1},
	{0x6F12, 0x0A00},
	{0x6F12, 0xFFDA},
	{0x6F12, 0xF4FF},
	{0x6F12, 0xA0E3},
	{0x6F12, 0x0090},
	{0x6F12, 0x9FE5},
	{0x6F12, 0x4804},
	{0x6F12, 0x9DE5},
	{0x6F12, 0x5810},
	{0x6F12, 0x80E0},
	{0x6F12, 0x8900},
	{0x6F12, 0xD0E1},
	{0x6F12, 0xFE09},
	{0x6F12, 0x00E0},
	{0x6F12, 0x9100},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x0064},
	{0x6F12, 0xB0E1},
	{0x6F12, 0x4668},
	{0x6F12, 0x9DE5},
	{0x6F12, 0xA000},
	{0x6F12, 0xA053},
	{0x6F12, 0x0210},
	{0x6F12, 0xE043},
	{0x6F12, 0x0110},
	{0x6F12, 0x00EB},
	{0x6F12, 0x3001},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x0088},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x4888},
	{0x6F12, 0xA0E3},
	{0x6F12, 0x0140},
	{0x6F12, 0x00EA},
	{0x6F12, 0x0800},
	{0x6F12, 0x8DE2},
	{0x6F12, 0x7C00},
	{0x6F12, 0x80E0},
	{0x6F12, 0x8450},
	{0x6F12, 0x55E1},
	{0x6F12, 0xF200},
	{0x6F12, 0x9DE5},
	{0x6F12, 0xA010},
	{0x6F12, 0x00E0},
	{0x6F12, 0x9600},
	{0x6F12, 0x88E0},
	{0x6F12, 0x8000},
	{0x6F12, 0x00EB},
	{0x6F12, 0x2501},
	{0x6F12, 0x84E2},
	{0x6F12, 0x0140},
	{0x6F12, 0xC5E1},
	{0x6F12, 0xB000},
	{0x6F12, 0x54E1},
	{0x6F12, 0x0A00},
	{0x6F12, 0xFFDA},
	{0x6F12, 0xF4FF},
	{0x6F12, 0xA0E3},
	{0x6F12, 0x0080},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x0860},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x0850},
	{0x6F12, 0x00EA},
	{0x6F12, 0x2300},
	{0x6F12, 0xA0E3},
	{0x6F12, 0x0040},
	{0x6F12, 0x00EA},
	{0x6F12, 0x1E00},
	{0x6F12, 0x45E0},
	{0x6F12, 0x0400},
	{0x6F12, 0x8DE2},
	{0x6F12, 0x7C10},
	{0x6F12, 0x8DE2},
	{0x6F12, 0x5C20},
	{0x6F12, 0x81E0},
	{0x6F12, 0x8410},
	{0x6F12, 0x82E0},
	{0x6F12, 0x8000},
	{0x6F12, 0xD1E1},
	{0x6F12, 0xF010},
	{0x6F12, 0xD0E1},
	{0x6F12, 0xF000},
	{0x6F12, 0x0BE0},
	{0x6F12, 0x9100},
	{0x6F12, 0x5BE3},
	{0x6F12, 0x0000},
	{0x6F12, 0x9DE5},
	{0x6F12, 0xA000},
	{0x6F12, 0xA0A3},
	{0x6F12, 0x0210},
	{0x6F12, 0xE0B3},
	{0x6F12, 0x0110},
	{0x6F12, 0x00EB},
	{0x6F12, 0x0E01},
	{0x6F12, 0x9DE5},
	{0x6F12, 0xA010},
	{0x6F12, 0x80E0},
	{0x6F12, 0x0B00},
	{0x6F12, 0x00EB},
	{0x6F12, 0x0B01},
	{0x6F12, 0x9DE5},
	{0x6F12, 0xA410},
	{0x6F12, 0xA0E3},
	{0x6F12, 0x0120},
	{0x6F12, 0x81E0},
	{0x6F12, 0x8610},
	{0x6F12, 0xD1E1},
	{0x6F12, 0xF010},
	{0x6F12, 0x00E0},
	{0x6F12, 0x9100},
	{0x6F12, 0xA0E3},
	{0x6F12, 0x0210},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x1117},
	{0x6F12, 0x50E3},
	{0x6F12, 0x0000},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x1227},
	{0x6F12, 0x62B2},
	{0x6F12, 0x0020},
	{0x6F12, 0x80E0},
	{0x6F12, 0x0200},
	{0x6F12, 0x00EB},
	{0x6F12, 0xFF00},
	{0x6F12, 0x88E0},
	{0x6F12, 0x0080},
	{0x6F12, 0x86E2},
	{0x6F12, 0x0160},
	{0x6F12, 0x84E2},
	{0x6F12, 0x0140},
	{0x6F12, 0x54E1},
	{0x6F12, 0x0500},
	{0x6F12, 0xFFDA},
	{0x6F12, 0xDEFF},
	{0x6F12, 0x85E2},
	{0x6F12, 0x0150},
	{0x6F12, 0x55E1},
	{0x6F12, 0x0A00},
	{0x6F12, 0xFFDA},
	{0x6F12, 0xD9FF},
	{0x6F12, 0x9DE5},
	{0x6F12, 0x0000},
	{0x6F12, 0xA0E3},
	{0x6F12, 0x021B},
	{0x6F12, 0x00E0},
	{0x6F12, 0x9800},
	{0x6F12, 0x81E0},
	{0x6F12, 0x4014},
	{0x6F12, 0x51E3},
	{0x6F12, 0x020B},
	{0x6F12, 0x9DE5},
	{0x6F12, 0x4C10},
	{0x6F12, 0x9DE5},
	{0x6F12, 0xAC20},
	{0x6F12, 0xA0A1},
	{0x6F12, 0x4004},
	{0x6F12, 0x80A2},
	{0x6F12, 0x020B},
	{0x6F12, 0x82E0},
	{0x6F12, 0x0111},
	{0x6F12, 0x9DE5},
	{0x6F12, 0xA820},
	{0x6F12, 0xA0B3},
	{0x6F12, 0x020B},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x0008},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x4008},
	{0x6F12, 0x82E0},
	{0x6F12, 0x8110},
	{0x6F12, 0xC1E1},
	{0x6F12, 0xB000},
	{0x6F12, 0x9DE5},
	{0x6F12, 0x0410},
	{0x6F12, 0x89E2},
	{0x6F12, 0x0190},
	{0x6F12, 0x50E1},
	{0x6F12, 0x0100},
	{0x6F12, 0xA0D1},
	{0x6F12, 0x0100},
	{0x6F12, 0x8DE5},
	{0x6F12, 0x0400},
	{0x6F12, 0x9DE5},
	{0x6F12, 0x4C00},
	{0x6F12, 0x59E3},
	{0x6F12, 0x0F00},
	{0x6F12, 0x80E2},
	{0x6F12, 0x0100},
	{0x6F12, 0x8DE5},
	{0x6F12, 0x4C00},
	{0x6F12, 0xFFBA},
	{0x6F12, 0xA1FF},
	{0x6F12, 0x9DE5},
	{0x6F12, 0x5000},
	{0x6F12, 0x80E2},
	{0x6F12, 0x0100},
	{0x6F12, 0x50E3},
	{0x6F12, 0x0B00},
	{0x6F12, 0x8DE5},
	{0x6F12, 0x5000},
	{0x6F12, 0xFFBA},
	{0x6F12, 0x7EFF},
	{0x6F12, 0x9DE5},
	{0x6F12, 0x0400},
	{0x6F12, 0x9DE5},
	{0x6F12, 0xAC20},
	{0x6F12, 0x50E3},
	{0x6F12, 0x020A},
	{0x6F12, 0xA0C1},
	{0x6F12, 0x0004},
	{0x6F12, 0xA0C1},
	{0x6F12, 0xC01F},
	{0x6F12, 0x80C0},
	{0x6F12, 0xA109},
	{0x6F12, 0x9FE5},
	{0x6F12, 0xA812},
	{0x6F12, 0xA0D3},
	{0x6F12, 0x010C},
	{0x6F12, 0x81E0},
	{0x6F12, 0x8210},
	{0x6F12, 0xA0C1},
	{0x6F12, 0xC006},
	{0x6F12, 0x8DE5},
	{0x6F12, 0x9C10},
	{0x6F12, 0xC1E1},
	{0x6F12, 0xB000},
	{0x6F12, 0x9DE5},
	{0x6F12, 0xB000},
	{0x6F12, 0x9DE5},
	{0x6F12, 0x9C10},
	{0x6F12, 0xD0E1},
	{0x6F12, 0xF400},
	{0x6F12, 0xD1E1},
	{0x6F12, 0xB010},
	{0x6F12, 0xA0E3},
	{0x6F12, 0x0050},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x0004},
	{0x6F12, 0x00EB},
	{0x6F12, 0xC500},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x0088},
	{0x6F12, 0x9DE5},
	{0x6F12, 0xB000},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x4888},
	{0x6F12, 0xC0E1},
	{0x6F12, 0xB480},
	{0x6F12, 0x9DE5},
	{0x6F12, 0x4800},
	{0x6F12, 0xA0E3},
	{0x6F12, 0x0060},
	{0x6F12, 0x40E2},
	{0x6F12, 0x029B},
	{0x6F12, 0x9DE5},
	{0x6F12, 0xB010},
	{0x6F12, 0x8DE2},
	{0x6F12, 0x0800},
	{0x6F12, 0x80E0},
	{0x6F12, 0x8600},
	{0x6F12, 0xD0E1},
	{0x6F12, 0xF000},
	{0x6F12, 0xD1E1},
	{0x6F12, 0xF210},
	{0x6F12, 0xA0E3},
	{0x6F12, 0x0040},
	{0x6F12, 0x40E0},
	{0x6F12, 0x0100},
	{0x6F12, 0x07E0},
	{0x6F12, 0x9000},
	{0x6F12, 0x9DE5},
	{0x6F12, 0xB010},
	{0x6F12, 0x8DE2},
	{0x6F12, 0x2000},
	{0x6F12, 0x80E0},
	{0x6F12, 0x8400},
	{0x6F12, 0xD0E1},
	{0x6F12, 0xF000},
	{0x6F12, 0xD1E1},
	{0x6F12, 0xF010},
	{0x6F12, 0x9FE5},
	{0x6F12, 0x20C2},
	{0x6F12, 0xA0E3},
	{0x6F12, 0x0130},
	{0x6F12, 0x40E0},
	{0x6F12, 0x0100},
	{0x6F12, 0x02E0},
	{0x6F12, 0x9000},
	{0x6F12, 0xDCE5},
	{0x6F12, 0xD800},
	{0x6F12, 0x82E0},
	{0x6F12, 0x0720},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x1310},
	{0x6F12, 0x81E0},
	{0x6F12, 0xA11F},
	{0x6F12, 0x82E0},
	{0x6F12, 0xC110},
	{0x6F12, 0xDCE5},
	{0x6F12, 0xDA20},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x3110},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x1302},
	{0x6F12, 0x9DE5},
	{0x6F12, 0x4030},
	{0x6F12, 0x9DE5},
	{0x6F12, 0x44C0},
	{0x6F12, 0x23E0},
	{0x6F12, 0x9931},
	{0x6F12, 0x80E0},
	{0x6F12, 0x533C},
	{0x6F12, 0x9DE5},
	{0x6F12, 0x40C0},
	{0x6F12, 0x80E0},
	{0x6F12, 0xA00F},
	{0x6F12, 0x21E0},
	{0x6F12, 0x98C1},
	{0x6F12, 0x9DE5},
	{0x6F12, 0x44C0},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x511C},
	{0x6F12, 0x01E0},
	{0x6F12, 0x9301},
	{0x6F12, 0x81E0},
	{0x6F12, 0xC000},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x50B2},
	{0x6F12, 0x9DE5},
	{0x6F12, 0x9C00},
	{0x6F12, 0x9DE5},
	{0x6F12, 0xA820},
	{0x6F12, 0xD0E1},
	{0x6F12, 0xB010},
	{0x6F12, 0x9DE5},
	{0x6F12, 0xAC00},
	{0x6F12, 0x80E0},
	{0x6F12, 0x0501},
	{0x6F12, 0x82E0},
	{0x6F12, 0x80A0},
	{0x6F12, 0xDAE1},
	{0x6F12, 0xF000},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x0004},
	{0x6F12, 0x00EB},
	{0x6F12, 0x9000},
	{0x6F12, 0x40E0},
	{0x6F12, 0x0B00},
	{0x6F12, 0x84E2},
	{0x6F12, 0x0140},
	{0x6F12, 0x54E3},
	{0x6F12, 0x0F00},
	{0x6F12, 0x85E2},
	{0x6F12, 0x0150},
	{0x6F12, 0xCAE1},
	{0x6F12, 0xB000},
	{0x6F12, 0xFFBA},
	{0x6F12, 0xD3FF},
	{0x6F12, 0x86E2},
	{0x6F12, 0x0160},
	{0x6F12, 0x56E3},
	{0x6F12, 0x0B00},
	{0x6F12, 0xFFBA},
	{0x6F12, 0xC8FF},
	{0x6F12, 0x8DE2},
	{0x6F12, 0xB4D0},
	{0x6F12, 0xBDE8},
	{0x6F12, 0xF04F},
	{0x6F12, 0x2FE1},
	{0x6F12, 0x1EFF},
	{0x6F12, 0x2DE9},
	{0x6F12, 0xF041},
	{0x6F12, 0x00EB},
	{0x6F12, 0x8400},
	{0x6F12, 0x50E3},
	{0x6F12, 0x0000},
	{0x6F12, 0xBD08},
	{0x6F12, 0xF041},
	{0x6F12, 0xA003},
	{0x6F12, 0x0010},
	{0x6F12, 0xA003},
	{0x6F12, 0x3800},
	{0x6F12, 0x000A},
	{0x6F12, 0x8100},
	{0x6F12, 0x9FE5},
	{0x6F12, 0x6C11},
	{0x6F12, 0xD1E1},
	{0x6F12, 0xBA01},
	{0x6F12, 0xD1E1},
	{0x6F12, 0xBC21},
	{0x6F12, 0xD1E1},
	{0x6F12, 0xBE11},
	{0x6F12, 0x80E1},
	{0x6F12, 0x0208},
	{0x6F12, 0x00EB},
	{0x6F12, 0x7D00},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x0070},
	{0x6F12, 0x9FE5},
	{0x6F12, 0x5451},
	{0x6F12, 0x9FE5},
	{0x6F12, 0x5401},
	{0x6F12, 0xD5E1},
	{0x6F12, 0xF030},
	{0x6F12, 0xD0E1},
	{0x6F12, 0xBAEA},
	{0x6F12, 0xD0E1},
	{0x6F12, 0xBCCA},
	{0x6F12, 0xD5E1},
	{0x6F12, 0xF220},
	{0x6F12, 0x00E0},
	{0x6F12, 0x930C},
	{0x6F12, 0x42E0},
	{0x6F12, 0x0360},
	{0x6F12, 0x02E0},
	{0x6F12, 0x9E02},
	{0x6F12, 0x4CE0},
	{0x6F12, 0x0E40},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x0410},
	{0x6F12, 0x40E0},
	{0x6F12, 0x0200},
	{0x6F12, 0x00EB},
	{0x6F12, 0x6900},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x0080},
	{0x6F12, 0x9FE5},
	{0x6F12, 0x2401},
	{0x6F12, 0xD0E1},
	{0x6F12, 0xB000},
	{0x6F12, 0x10E3},
	{0x6F12, 0x020C},
	{0x6F12, 0xA011},
	{0x6F12, 0x0700},
	{0x6F12, 0x001B},
	{0x6F12, 0x6B00},
	{0x6F12, 0x56E3},
	{0x6F12, 0x0000},
	{0x6F12, 0xE003},
	{0x6F12, 0x0000},
	{0x6F12, 0x000A},
	{0x6F12, 0x0300},
	{0x6F12, 0x47E0},
	{0x6F12, 0x0800},
	{0x6F12, 0x00E0},
	{0x6F12, 0x9400},
	{0x6F12, 0xA0E1},
	{0x6F12, 0x0610},
	{0x6F12, 0x00EB},
	{0x6F12, 0x6200},
	{0x6F12, 0xC5E1},
	{0x6F12, 0xB400},
	{0x6F12, 0xBDE8},
	{0x6F12, 0xF041},
	{0x6F12, 0x2FE1},
	{0x6F12, 0x1EFF},
	{0x6F12, 0x9FE5},
	{0x6F12, 0xEC10},
	{0x6F12, 0x9FE5},
	{0x6F12, 0xEC00},
	{0x6F12, 0x2DE9},
	{0x6F12, 0x1040},
	{0x6F12, 0x9FE5},
	{0x6F12, 0xE820},
	{0x6F12, 0x80E5},
	{0x6F12, 0x5010},
	{0x6F12, 0x42E0},
	{0x6F12, 0x0110},
	{0x6F12, 0xC0E1},
	{0x6F12, 0xB415},
	{0x6F12, 0x9FE5},
	{0x6F12, 0xDC00},
	{0x6F12, 0x4FE2},
	{0x6F12, 0xD410},
	{0x6F12, 0x00EB},
	{0x6F12, 0x5900},
	{0x6F12, 0x9FE5},
	{0x6F12, 0xD400},
	{0x6F12, 0x9FE5},
	{0x6F12, 0xD440},
	{0x6F12, 0x9FE5},
	{0x6F12, 0x9420},
	{0x6F12, 0x84E5},
	{0x6F12, 0x0400},
	{0x6F12, 0xA0E3},
	{0x6F12, 0x0000},
	{0x6F12, 0xA0E3},
	{0x6F12, 0x011C},
	{0x6F12, 0x82E0},
	{0x6F12, 0x8030},
	{0x6F12, 0x80E2},
	{0x6F12, 0x0100},
	{0x6F12, 0x50E3},
	{0x6F12, 0x0400},
	{0x6F12, 0xC3E1},
	{0x6F12, 0xB010},
	{0x6F12, 0xFF3A},
	{0x6F12, 0xFAFF},
	{0x6F12, 0x9FE5},
	{0x6F12, 0xB000},
	{0x6F12, 0x9FE5},
	{0x6F12, 0xB410},
	{0x6F12, 0x84E5},
	{0x6F12, 0x5C00},
	{0x6F12, 0x9FE5},
	{0x6F12, 0xA800},
	{0x6F12, 0x84E5},
	{0x6F12, 0x2C00},
	{0x6F12, 0x9FE5},
	{0x6F12, 0xA800},
	{0x6F12, 0x00EB},
	{0x6F12, 0x4700},
	{0x6F12, 0x9FE5},
	{0x6F12, 0xA400},
	{0x6F12, 0x4FE2},
	{0x6F12, 0x711E},
	{0x6F12, 0x84E5},
	{0x6F12, 0x0000},
	{0x6F12, 0x9FE5},
	{0x6F12, 0x9C00},
	{0x6F12, 0x00EB},
	{0x6F12, 0x4200},
	{0x6F12, 0x9FE5},
	{0x6F12, 0x2410},
	{0x6F12, 0xC1E1},
	{0x6F12, 0xB000},
	{0x6F12, 0x9FE5},
	{0x6F12, 0x5C00},
	{0x6F12, 0xD0E1},
	{0x6F12, 0xB012},
	{0x6F12, 0x51E3},
	{0x6F12, 0x1000},
	{0x6F12, 0x009A},
	{0x6F12, 0x0200},
	{0x6F12, 0xA0E3},
	{0x6F12, 0x090C},
	{0x6F12, 0x00EB},
	{0x6F12, 0x3400},
	{0x6F12, 0xFFEA},
	{0x6F12, 0xFEFF},
	{0x6F12, 0xBDE8},
	{0x6F12, 0x1040},
	{0x6F12, 0x2FE1},
	{0x6F12, 0x1EFF},
	{0x6F12, 0x0070},
	{0x6F12, 0xC41F},
	{0x6F12, 0x00D0},
	{0x6F12, 0x0061},
	{0x6F12, 0x0070},
	{0x6F12, 0x5014},
	{0x6F12, 0x0070},
	{0x6F12, 0x0000},
	{0x6F12, 0x00D0},
	{0x6F12, 0x00F4},
	{0x6F12, 0x0070},
	{0x6F12, 0x7004},
	{0x6F12, 0x0070},
	{0x6F12, 0xD005},
	{0x6F12, 0x0070},
	{0x6F12, 0xC61F},
	{0x6F12, 0x0070},
	{0x6F12, 0x1013},
	{0x6F12, 0x0070},
	{0x6F12, 0xB412},
	{0x6F12, 0x0070},
	{0x6F12, 0x8C1F},
	{0x6F12, 0x0070},
	{0x6F12, 0xAC1F},
	{0x6F12, 0x0070},
	{0x6F12, 0x0400},
	{0x6F12, 0x00D0},
	{0x6F12, 0x0093},
	{0x6F12, 0x0070},
	{0x6F12, 0x8012},
	{0x6F12, 0x0070},
	{0x6F12, 0xC00B},
	{0x6F12, 0x0070},
	{0x6F12, 0xE012},
	{0x6F12, 0x0070},
	{0x6F12, 0xD01F},
	{0x6F12, 0x0070},
	{0x6F12, 0x7005},
	{0x6F12, 0x0070},
	{0x6F12, 0x902D},
	{0x6F12, 0x0000},
	{0x6F12, 0x90A6},
	{0x6F12, 0x0070},
	{0x6F12, 0xFC18},
	{0x6F12, 0x0070},
	{0x6F12, 0xF804},
	{0x6F12, 0x0070},
	{0x6F12, 0x9818},
	{0x6F12, 0x0070},
	{0x6F12, 0xE018},
	{0x6F12, 0x0070},
	{0x6F12, 0x7018},
	{0x6F12, 0x0000},
	{0x6F12, 0xC06A},
	{0x6F12, 0x0070},
	{0x6F12, 0xE017},
	{0x6F12, 0x0000},
	{0x6F12, 0x781C},
	{0x6F12, 0x7847},
	{0x6F12, 0xC046},
	{0x6F12, 0xFFEA},
	{0x6F12, 0xB4FF},
	{0x6F12, 0x7847},
	{0x6F12, 0xC046},
	{0x6F12, 0x1FE5},
	{0x6F12, 0x04F0},
	{0x6F12, 0x0000},
	{0x6F12, 0x6CCE},
	{0x6F12, 0x1FE5},
	{0x6F12, 0x04F0},
	{0x6F12, 0x0000},
	{0x6F12, 0x781C},
	{0x6F12, 0x1FE5},
	{0x6F12, 0x04F0},
	{0x6F12, 0x0000},
	{0x6F12, 0x54C0},
	{0x6F12, 0x1FE5},
	{0x6F12, 0x04F0},
	{0x6F12, 0x0000},
	{0x6F12, 0x8448},
	{0x6F12, 0x1FE5},
	{0x6F12, 0x04F0},
	{0x6F12, 0x0000},
	{0x6F12, 0x146C},
	{0x6F12, 0x1FE5},
	{0x6F12, 0x04F0},
	{0x6F12, 0x0000},
	{0x6F12, 0x4C7E},
	{0x6F12, 0x1FE5},
	{0x6F12, 0x04F0},
	{0x6F12, 0x0000},
	{0x6F12, 0x8CDC},
	{0x6F12, 0x1FE5},
	{0x6F12, 0x04F0},
	{0x6F12, 0x0000},
	{0x6F12, 0x48DD},
	{0x6F12, 0x1FE5},
	{0x6F12, 0x04F0},
	{0x6F12, 0x0000},
	{0x6F12, 0x7C55},
	{0x6F12, 0x1FE5},
	{0x6F12, 0x04F0},
	{0x6F12, 0x0000},
	{0x6F12, 0x744C},
	{0x6F12, 0x1FE5},
	{0x6F12, 0x04F0},
	{0x6F12, 0x0000},
	{0x6F12, 0xE8DE},
	{0x6F12, 0x1FE5},
	{0x6F12, 0x04F0},
	{0x6F12, 0x0000},
	{0x6F12, 0x4045},
	{0x6F12, 0x1FE5},
	{0x6F12, 0x04F0},
	{0x6F12, 0x0000},
	{0x6F12, 0xE8CD},
	{0x6F12, 0x80F9},
	{0x6F12, 0x00FA},
	{0x6F12, 0x00FB},
	{0x6F12, 0x00FC},
	{0x6F12, 0x00FD},
	{0x6F12, 0x00FE},
	{0x6F12, 0x00FF},
	{0x6F12, 0x0000},
	{0x6F12, 0x0001},
	{0x6F12, 0x0002},
	{0x6F12, 0x0003},
	{0x6F12, 0x0004},
	{0x6F12, 0x0005},
	{0x6F12, 0x0006},
	{0x6F12, 0x8006},
	{0x6F12, 0x0000},
	{0x6F12, 0x00FB},
	{0x6F12, 0x00FC},
	{0x6F12, 0x00FD},
	{0x6F12, 0x00FE},
	{0x6F12, 0x00FF},
	{0x6F12, 0x0000},
	{0x6F12, 0x0001},
	{0x6F12, 0x0002},
	{0x6F12, 0x0003},
	{0x6F12, 0x0004},
	{0x6F12, 0x0005},
	{0x6F12, 0x0000},
	//=====================================================================================
	// Base setfile : Rev - 3126 
	// Date: 2012-01-05 15:10:35 +0900 (THU, 05 JAN 2012) 
	// 3H7 Analog set file BQ Mode
	//=====================================================================================
	{0x6028, 0xD000},
	//=====================================================================================
	// APS/Analog setting (Date: 2012-01-05 15:10:35 +0900 (THU, 05 JAN 2012)) 
	//=====================================================================================
	//=====================================================================================
	// START OF FW REGISTERS APS/Analog UPDATING
	//=====================================================================================
	// Offset control
	{0x38FA, 0x0030},  // gisp_offs_gains_bls_offs_0_
	{0x38FC, 0x0030},  // gisp_offs_gains_bls_offs_1_
	
	// Sensor XY cordination
	{0x32CE, 0x0060},    // senHal_usWidthStOfsInit
	{0x32D0, 0x0024},    // senHal_usHeightStOfsInit
	
	{0x0086, 0x01FF},	//#smiaRegs_rd_analog_gain_analogue_gain_code_max	 
									  
	{0x012A, 0x0040},	//#smiaRegs_rw_analog_gain_mode_AG_th		 
	{0x012C, 0x7077},	//#smiaRegs_rw_analog_gain_mode_F430_val	 
	{0x012E, 0x7777},	//#smiaRegs_rw_analog_gain_mode_F430_default_val 
	
	// For 35Mhz BQ Mode
	//========================================================
	// Setting for MIPI CLK (Don't change)
	{0x6218, 0xF1D0},	// open all clocks
	{0x6214, 0xF9F0},	// open all clocks
	{0x6226, 0x0001},	// open APB clock for I2C transaction
	//=====================================================================================
	// START OF HW REGISTERS APS/Analog UPDATING
	//=====================================================================================
	{0xB0C0, 0x000C},
	{0xF400, 0x0BBC}, 
	{0xF616, 0x0004}, //aig_tmc_gain 
	//=====================================================================================
	// END OF HW REGISTERS APS/Analog UPDATING
	//=====================================================================================
	{0x6226, 0x0000}, //close APB clock for I2C transaction
	{0x6218, 0xF9F0}, //close all clocks
	//=====================================================================================
	// End of APS/Analog setting
	//=====================================================================================//=====================================================================================
	// START default setting
	//=====================================================================================
	
	//=====================================================================================
	// End default setting
	//=====================================================================================
	// Test Name : "fw_8M_280_sclk_BQ_35_mipi.tset"
	////////////////////////////////////////////////
	//					      //
	//     PLUSARGS for configuration	//
	//					      //
	////////////////////////////////////////////////
	{0x3338, 0x0264}, //senHal_MaxCdsTime								0264
	{0x0114, 0x0100},    // 2Lane
	// set PLL
	//{0x030E, 0x00a2},	// smiaRegs_rw_clocks_secnd_pll_multiplier
	//{0x030A, 0x0004}, // smiaRegs_rw_clocks_op_sys_clk_div
	
	{0x311C, 0x0BB8},	//#skl_uEndFrCyclesNoCfgDiv4	0BB8		//Increase Blank time on account of process time 
	{0x311E, 0x0BB8},	//#skl_uEndFrCyclesWithCfgDiv4	0BB8		//Increase Blank time on account of process time 
		

};

LOCAL const SENSOR_REG_T s5k3h7yx_1632x1224_mipi_raw[] = {
	// Set FPS
	{0x0342, 0x1C68},//WRITE	#smiaRegs_rw_frame_timing_line_length_pck	    1C68//0E50
	{0x0340, s5k3h7yx_MIN_FRAME_LEN_PRV},//WRITE	#smiaRegs_rw_frame_timing_frame_length_lines	09B5//04F8
	{0x034C, 0x0660},//WRITE	#smiaRegs_rw_frame_timing_x_output_size 0660
	{0x034E, 0x04C8},//WRITE	#smiaRegs_rw_frame_timing_y_output_size 04C8
	{0x0344, 0x0004},//WRITE	#smiaRegs_rw_frame_timing_x_addr_start	0004
	{0x0346, 0x0004},//WRITE	#smiaRegs_rw_frame_timing_y_addr_start	0004
	{0x0348, 0x0CC3},//WRITE	#smiaRegs_rw_frame_timing_x_addr_end	0CC3
	{0x034A, 0x0993},//WRITE	#smiaRegs_rw_frame_timing_y_addr_end	0993
	{0x0382, 0x0003},//WRITE	#smiaRegs_rw_sub_sample_x_odd_inc	0003
	{0x0386, 0x0003},//WRITE	#smiaRegs_rw_sub_sample_y_odd_inc	0003
	{0x0900, 0x0022},//WRITE	#smiaRegs_rw_binning_type	22
	
};

LOCAL const SENSOR_REG_T s5k3h7yx_3264x2448_mipi_raw[] = {
	// Set FPS
	{0x0342, 0x1C68},//WRITE	#smiaRegs_rw_frame_timing_line_length_pck	    1C68//0E50
	{0x0340, s5k3h7yx_MIN_FRAME_LEN_CAP},//WRITE	#smiaRegs_rw_frame_timing_frame_length_lines	09B5//04F8
	{0x034C, 0x0CC0},//WRITE	#smiaRegs_rw_frame_timing_x_output_size 0660
	{0x034E, 0x0990},//WRITE	#smiaRegs_rw_frame_timing_y_output_size 04C8
	{0x0344, 0x0004},//WRITE	#smiaRegs_rw_frame_timing_x_addr_start	0004
	{0x0346, 0x0004},//WRITE	#smiaRegs_rw_frame_timing_y_addr_start	0004
	{0x0348, 0x0CC3},//WRITE	#smiaRegs_rw_frame_timing_x_addr_end	0CC3
	{0x034A, 0x0993},//WRITE	#smiaRegs_rw_frame_timing_y_addr_end	0993
	{0x0382, 0x0001},//WRITE	#smiaRegs_rw_sub_sample_x_odd_inc	0001
	{0x0386, 0x0001},//WRITE	#smiaRegs_rw_sub_sample_y_odd_inc	0001
	{0x0900, 0x0011},//WRITE	#smiaRegs_rw_binning_type	11
	
	
};

LOCAL SENSOR_REG_TAB_INFO_T s_s5k3h7yx_resolution_Tab_RAW[] = {
	{ADDR_AND_LEN_OF_ARRAY(s5k3h7yx_com_mipi_raw), 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},

	{ADDR_AND_LEN_OF_ARRAY(s5k3h7yx_1632x1224_mipi_raw), 1632, 1224, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(s5k3h7yx_3264x2448_mipi_raw), 3264, 2448, 24, SENSOR_IMAGE_FORMAT_RAW},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

LOCAL SENSOR_TRIM_T s_s5k3h7yx_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0, 0},
	{0, 0, 1632, 1224, 260, 280, 1272},
	{0, 0, 3264, 2448, 260, 280, 2485},
	{0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 0}
};


struct sensor_raw_info* s_s5k3h7yx_mipi_raw_info_ptr;

LOCAL SENSOR_IOCTL_FUNC_TAB_T s_s5k3h7yx_ioctl_func_tab = {
	PNULL,
	_s5k3h7yx_PowerOn,
	PNULL,
	_s5k3h7yx_Identify,

	PNULL,			// write register
	PNULL,			// read  register
	PNULL,
	_s5k3h7yx_GetResolutionTrimTab,

	// External
	PNULL,
	PNULL,
	PNULL,

	PNULL, //_s5k3h7yx_set_brightness,
	PNULL, // _s5k3h7yx_set_contrast,
	PNULL,
	PNULL,			//_s5k3h7yx_set_saturation,

	PNULL, //_s5k3h7yx_set_work_mode,
	PNULL, //_s5k3h7yx_set_image_effect,

	_s5k3h7yx_BeforeSnapshot,
	_s5k3h7yx_after_snapshot,
	PNULL,//_s5k3h7yx_flash,
	PNULL,
	_s5k3h7yx_write_exposure,
	PNULL,
	_s5k3h7yx_write_gain,
	PNULL,
	PNULL,
	_s5k3h7yx_write_af,
	PNULL,
	PNULL, //_s5k3h7yx_set_awb,
	PNULL,
	PNULL,
	PNULL, //_s5k3h7yx_set_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL, //_s5k3h7yx_GetExifInfo,
	PNULL, //_s5k3h7yx_ExtFunc,
	PNULL, //_s5k3h7yx_set_anti_flicker,
	PNULL, //_s5k3h7yx_set_video_mode,
	PNULL, //pick_jpeg_stream
	PNULL,  //meter_mode
	PNULL, //get_status
	_s5k3h7yx_StreamOn,
	_s5k3h7yx_StreamOff,
	PNULL,
};


SENSOR_INFO_T g_s5k3h7yx_mipi_raw_info = {
	s5k3h7yx_I2C_ADDR_W,	// salve i2c write address
	s5k3h7yx_I2C_ADDR_R,	// salve i2c read address

	SENSOR_I2C_REG_16BIT | SENSOR_I2C_VAL_16BIT,	// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
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
	{{0x00, 0x30},		// supply two code to identify sensor.
	 {0x01, 0x87}},		// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,	// voltage of avdd

	3264,			// max width of source image
	2448,			// max height of source image
	"s5k3h7yx",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_GR,// pattern of input image form sensor;

	s_s5k3h7yx_resolution_Tab_RAW,	// point to resolution table information structure
	&s_s5k3h7yx_ioctl_func_tab,	// point to ioctl function table
	(uint32_t *)&s_s5k3h7yx_mipi_raw_info_ptr,		// information and table about Rawrgb sensor
	NULL,			//&g_s5k3h7yx_ext_info,                // extend information about sensor
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
	{SENSOR_INTERFACE_TYPE_CSI2, 2, 10, 0},
	NULL,//s_s5k3h7yx_video_info,
	3,			// skip frame num while change setting
};

LOCAL struct sensor_raw_info* Sensor_GetContext(void)
{
	return s_s5k3h7yx_mipi_raw_info_ptr;
}

LOCAL uint32_t Sensor_s5k3h7yx_InitRawTuneInfo(void)
{
	uint32_t rtn=0x00;
	struct sensor_raw_info* raw_sensor_ptr=Sensor_GetContext();
	struct sensor_raw_tune_info* sensor_ptr=raw_sensor_ptr->tune_ptr;
#if 0
	//raw_sensor_ptr->version_info->version_id=0x00000000;
	raw_sensor_ptr->version_info->srtuct_size=sizeof(struct sensor_raw_info);

	//bypass
	//sensor_ptr->version_id=0x00000000;
	sensor_ptr->blc_bypass=0x0;
	sensor_ptr->nlc_bypass=0x1;
	sensor_ptr->lnc_bypass=0x0;
	sensor_ptr->ae_bypass=0x0;
	sensor_ptr->awb_bypass=0x0;
	sensor_ptr->bpc_bypass=0x0;
	sensor_ptr->denoise_bypass=0x0;
	sensor_ptr->grgb_bypass=0x1;
	sensor_ptr->cmc_bypass=0x0;
	sensor_ptr->gamma_bypass=0x0;
	sensor_ptr->uvdiv_bypass=0x1;
	sensor_ptr->pref_bypass=0x0;
	sensor_ptr->bright_bypass=0x0;
	sensor_ptr->contrast_bypass=0x0;
	sensor_ptr->hist_bypass=0x1;
	sensor_ptr->auto_contrast_bypass=0x1;
	sensor_ptr->af_bypass=0x0;
	sensor_ptr->edge_bypass=0x0;
	sensor_ptr->fcs_bypass=0x0;
	sensor_ptr->css_bypass=0x1;
	sensor_ptr->saturation_bypass=0x1;
	sensor_ptr->hdr_bypass=0x1;
	sensor_ptr->glb_gain_bypass=0x1;
	sensor_ptr->chn_gain_bypass=0x1;

	//blc
	sensor_ptr->blc.mode=0x0;
	sensor_ptr->blc.offset[0].r=0x40;
	sensor_ptr->blc.offset[0].gr=0x40;
	sensor_ptr->blc.offset[0].gb=0x40;
	sensor_ptr->blc.offset[0].b=0x40;

	sensor_ptr->blc.offset[1].r=0x40;
	sensor_ptr->blc.offset[1].gr=0x40;
	sensor_ptr->blc.offset[1].gb=0x40;
	sensor_ptr->blc.offset[1].b=0x40;
	//nlc
	sensor_ptr->nlc.r_node[0]=0x0;
	sensor_ptr->nlc.r_node[1]=0x10;
	sensor_ptr->nlc.r_node[2]=0x20;
	sensor_ptr->nlc.r_node[3]=0x40;
	sensor_ptr->nlc.r_node[4]=0x60;
	sensor_ptr->nlc.r_node[5]=0x80;
	sensor_ptr->nlc.r_node[6]=0xA0;
	sensor_ptr->nlc.r_node[7]=0xC0;
	sensor_ptr->nlc.r_node[8]=0xE0;
	sensor_ptr->nlc.r_node[9]=0x100;
	sensor_ptr->nlc.r_node[10]=0x120;
	sensor_ptr->nlc.r_node[11]=0x140;
	sensor_ptr->nlc.r_node[12]=0x180;
	sensor_ptr->nlc.r_node[13]=0x1C0;
	sensor_ptr->nlc.r_node[14]=0x200;
	sensor_ptr->nlc.r_node[15]=0x240;
	sensor_ptr->nlc.r_node[16]=0x280;
	sensor_ptr->nlc.r_node[17]=0x2A0;
	sensor_ptr->nlc.r_node[18]=0x2C0;
	sensor_ptr->nlc.r_node[19]=0x2E0;
	sensor_ptr->nlc.r_node[20]=0x300;
	sensor_ptr->nlc.r_node[21]=0x320;
	sensor_ptr->nlc.r_node[22]=0x340;
	sensor_ptr->nlc.r_node[23]=0x360;
	sensor_ptr->nlc.r_node[24]=0x380;
	sensor_ptr->nlc.r_node[25]=0x3A0;
	sensor_ptr->nlc.r_node[26]=0x3C0;
	sensor_ptr->nlc.r_node[27]=0x3E0;
	sensor_ptr->nlc.r_node[28]=0x3FF;

	sensor_ptr->nlc.g_node[0]=0x0;
	sensor_ptr->nlc.g_node[1]=0x10;
	sensor_ptr->nlc.g_node[2]=0x20;
	sensor_ptr->nlc.g_node[3]=0x40;
	sensor_ptr->nlc.g_node[4]=0x60;
	sensor_ptr->nlc.g_node[5]=0x80;
	sensor_ptr->nlc.g_node[6]=0xA0;
	sensor_ptr->nlc.g_node[7]=0xC0;
	sensor_ptr->nlc.g_node[8]=0xE0;
	sensor_ptr->nlc.g_node[9]=0x100;
	sensor_ptr->nlc.g_node[10]=0x120;
	sensor_ptr->nlc.g_node[11]=0x140;
	sensor_ptr->nlc.g_node[12]=0x180;
	sensor_ptr->nlc.g_node[13]=0x1C0;
	sensor_ptr->nlc.g_node[14]=0x200;
	sensor_ptr->nlc.g_node[15]=0x240;
	sensor_ptr->nlc.g_node[16]=0x280;
	sensor_ptr->nlc.g_node[17]=0x2A0;
	sensor_ptr->nlc.g_node[18]=0x2C0;
	sensor_ptr->nlc.g_node[19]=0x2E0;
	sensor_ptr->nlc.g_node[20]=0x300;
	sensor_ptr->nlc.g_node[21]=0x320;
	sensor_ptr->nlc.g_node[22]=0x340;
	sensor_ptr->nlc.g_node[23]=0x360;
	sensor_ptr->nlc.g_node[24]=0x380;
	sensor_ptr->nlc.g_node[25]=0x3A0;
	sensor_ptr->nlc.g_node[26]=0x3C0;
	sensor_ptr->nlc.g_node[27]=0x3E0;
	sensor_ptr->nlc.g_node[28]=0x3FF;

	sensor_ptr->nlc.b_node[0]=0x0;
	sensor_ptr->nlc.b_node[1]=0x10;
	sensor_ptr->nlc.b_node[2]=0x20;
	sensor_ptr->nlc.b_node[3]=0x40;
	sensor_ptr->nlc.b_node[4]=0x60;
	sensor_ptr->nlc.b_node[5]=0x80;
	sensor_ptr->nlc.b_node[6]=0xA0;
	sensor_ptr->nlc.b_node[7]=0xC0;
	sensor_ptr->nlc.b_node[8]=0xE0;
	sensor_ptr->nlc.b_node[9]=0x100;
	sensor_ptr->nlc.b_node[10]=0x120;
	sensor_ptr->nlc.b_node[11]=0x140;
	sensor_ptr->nlc.b_node[12]=0x180;
	sensor_ptr->nlc.b_node[13]=0x1C0;
	sensor_ptr->nlc.b_node[14]=0x200;
	sensor_ptr->nlc.b_node[15]=0x240;
	sensor_ptr->nlc.b_node[16]=0x280;
	sensor_ptr->nlc.b_node[17]=0x2A0;
	sensor_ptr->nlc.b_node[18]=0x2C0;
	sensor_ptr->nlc.b_node[19]=0x2E0;
	sensor_ptr->nlc.b_node[20]=0x300;
	sensor_ptr->nlc.b_node[21]=0x320;
	sensor_ptr->nlc.b_node[22]=0x340;
	sensor_ptr->nlc.b_node[23]=0x360;
	sensor_ptr->nlc.b_node[24]=0x380;
	sensor_ptr->nlc.b_node[25]=0x3A0;
	sensor_ptr->nlc.b_node[26]=0x3C0;
	sensor_ptr->nlc.b_node[27]=0x3E0;
	sensor_ptr->nlc.b_node[28]=0x3FF;

	sensor_ptr->nlc.l_node[0]=0x0;
	sensor_ptr->nlc.l_node[1]=0x10;
	sensor_ptr->nlc.l_node[2]=0x20;
	sensor_ptr->nlc.l_node[3]=0x40;
	sensor_ptr->nlc.l_node[4]=0x60;
	sensor_ptr->nlc.l_node[5]=0x80;
	sensor_ptr->nlc.l_node[6]=0xA0;
	sensor_ptr->nlc.l_node[7]=0xC0;
	sensor_ptr->nlc.l_node[8]=0xE0;
	sensor_ptr->nlc.l_node[9]=0x100;
	sensor_ptr->nlc.l_node[10]=0x120;
	sensor_ptr->nlc.l_node[11]=0x140;
	sensor_ptr->nlc.l_node[12]=0x180;
	sensor_ptr->nlc.l_node[13]=0x1C0;
	sensor_ptr->nlc.l_node[14]=0x200;
	sensor_ptr->nlc.l_node[15]=0x240;
	sensor_ptr->nlc.l_node[16]=0x280;
	sensor_ptr->nlc.l_node[17]=0x2A0;
	sensor_ptr->nlc.l_node[18]=0x2C0;
	sensor_ptr->nlc.l_node[19]=0x2E0;
	sensor_ptr->nlc.l_node[20]=0x300;
	sensor_ptr->nlc.l_node[21]=0x320;
	sensor_ptr->nlc.l_node[22]=0x340;
	sensor_ptr->nlc.l_node[23]=0x360;
	sensor_ptr->nlc.l_node[24]=0x380;
	sensor_ptr->nlc.l_node[25]=0x3A0;
	sensor_ptr->nlc.l_node[26]=0x3C0;
	sensor_ptr->nlc.l_node[27]=0x3E0;
	sensor_ptr->nlc.l_node[28]=0x3FF;

	//ae
	sensor_ptr->ae.skip_frame=0x1;
	sensor_ptr->ae.normal_fix_fps=0;
	sensor_ptr->ae.night_fix_fps=0;
	sensor_ptr->ae.video_fps=0x1E;
	sensor_ptr->ae.target_lum=0x32;
	sensor_ptr->ae.target_zone=0x6;
	sensor_ptr->ae.quick_mode=0x1;
	sensor_ptr->ae.smart=0x0;
	sensor_ptr->ae.smart_rotio=0xFF;
	sensor_ptr->ae.ev[0]=0xE8;
	sensor_ptr->ae.ev[1]=0xF0;
	sensor_ptr->ae.ev[2]=0xF8;
	sensor_ptr->ae.ev[3]=0x0;
	sensor_ptr->ae.ev[4]=0x8;
	sensor_ptr->ae.ev[5]=0x10;
	sensor_ptr->ae.ev[6]=0x18;
	sensor_ptr->ae.ev[7]=0x0;
	sensor_ptr->ae.ev[8]=0x0;
	sensor_ptr->ae.ev[9]=0x0;
	sensor_ptr->ae.ev[10]=0x0;
	sensor_ptr->ae.ev[11]=0x0;
	sensor_ptr->ae.ev[12]=0x0;
	sensor_ptr->ae.ev[13]=0x0;
	sensor_ptr->ae.ev[14]=0x0;
	sensor_ptr->ae.ev[15]=0x0;

	//awb
	sensor_ptr->awb.win_start.x=0x0;
	sensor_ptr->awb.win_start.y=0x4;
	sensor_ptr->awb.win_size.w=0x33;
	sensor_ptr->awb.win_size.h=0x26;
	sensor_ptr->awb.quick_mode = 1;
	sensor_ptr->awb.r_gain[0]=0x1B0;
	sensor_ptr->awb.g_gain[0]=0xFF;
	sensor_ptr->awb.b_gain[0]=0x180;
	sensor_ptr->awb.r_gain[1]=0x100;
	sensor_ptr->awb.g_gain[1]=0xFF;
	sensor_ptr->awb.b_gain[1]=0x210;
	sensor_ptr->awb.r_gain[2]=0xFF;
	sensor_ptr->awb.g_gain[2]=0xFF;
	sensor_ptr->awb.b_gain[2]=0xFF;
	sensor_ptr->awb.r_gain[3]=0xFF;
	sensor_ptr->awb.g_gain[3]=0xFF;
	sensor_ptr->awb.b_gain[3]=0xFF;
	sensor_ptr->awb.r_gain[4]=0x13E;
	sensor_ptr->awb.g_gain[4]=0xFF;
	sensor_ptr->awb.b_gain[4]=0x1B4;
	sensor_ptr->awb.r_gain[5]=0x194;
	sensor_ptr->awb.g_gain[5]=0xFF;
	sensor_ptr->awb.b_gain[5]=0x134;
	sensor_ptr->awb.r_gain[6]=0x240;
	sensor_ptr->awb.g_gain[6]=0xFF;
	sensor_ptr->awb.b_gain[6]=0x12C;
	sensor_ptr->awb.r_gain[7]=0xFF;
	sensor_ptr->awb.g_gain[7]=0xFF;
	sensor_ptr->awb.b_gain[7]=0xFF;
	sensor_ptr->awb.r_gain[8]=0xFF;
	sensor_ptr->awb.g_gain[8]=0xFF;
	sensor_ptr->awb.b_gain[8]=0xFF;
	sensor_ptr->awb.target_zone=0x40;


	/*awb win*/
	sensor_ptr->awb.win[0].x =0x91;
	sensor_ptr->awb.win[1].x =0x93;
	sensor_ptr->awb.win[2].x =0x96;
	sensor_ptr->awb.win[3].x =0x98;
	sensor_ptr->awb.win[4].x =0x9A;
	sensor_ptr->awb.win[5].x =0xA1;
	sensor_ptr->awb.win[6].x =0xA8;
	sensor_ptr->awb.win[7].x =0xAE;
	sensor_ptr->awb.win[8].x =0xB3;
	sensor_ptr->awb.win[9].x =0xB9;
	sensor_ptr->awb.win[10].x=0xC1;
	sensor_ptr->awb.win[11].x=0xCB;
	sensor_ptr->awb.win[12].x=0xD6;
	sensor_ptr->awb.win[13].x=0xE1;
	sensor_ptr->awb.win[14].x=0xF1;
	sensor_ptr->awb.win[15].x=0xFD;
	sensor_ptr->awb.win[16].x=0x108;
	sensor_ptr->awb.win[17].x=0x118;
	sensor_ptr->awb.win[18].x=0x124;
	sensor_ptr->awb.win[19].x=0x12D;

	sensor_ptr->awb.win[0].yb =0x9D;
	sensor_ptr->awb.win[1].yb =0x94;
	sensor_ptr->awb.win[2].yb =0x89;
	sensor_ptr->awb.win[3].yb =0x85;
	sensor_ptr->awb.win[4].yb =0x7C;
	sensor_ptr->awb.win[5].yb =0x6F;
	sensor_ptr->awb.win[6].yb =0x69;
	sensor_ptr->awb.win[7].yb =0x65;
	sensor_ptr->awb.win[8].yb =0x67;
	sensor_ptr->awb.win[9].yb =0x68;
	sensor_ptr->awb.win[10].yb=0x68;
	sensor_ptr->awb.win[11].yb=0x6A;
	sensor_ptr->awb.win[12].yb=0x6F;
	sensor_ptr->awb.win[13].yb=0x70;
	sensor_ptr->awb.win[14].yb=0x67;
	sensor_ptr->awb.win[15].yb=0x5A;
	sensor_ptr->awb.win[16].yb=0x55;
	sensor_ptr->awb.win[17].yb=0x50;
	sensor_ptr->awb.win[18].yb=0x51;
	sensor_ptr->awb.win[19].yb=0x51;

	sensor_ptr->awb.win[0].yt =0xD4;
	sensor_ptr->awb.win[1].yt =0xE2;
	sensor_ptr->awb.win[2].yt =0xEA;
	sensor_ptr->awb.win[3].yt =0xED;
	sensor_ptr->awb.win[4].yt =0xEF;
	sensor_ptr->awb.win[5].yt =0xEF;
	sensor_ptr->awb.win[6].yt =0xE9;
	sensor_ptr->awb.win[7].yt =0xCF;
	sensor_ptr->awb.win[8].yt =0x8F;
	sensor_ptr->awb.win[9].yt =0x8E;
	sensor_ptr->awb.win[10].yt=0x90;
	sensor_ptr->awb.win[11].yt=0x92;
	sensor_ptr->awb.win[12].yt=0x92;
	sensor_ptr->awb.win[13].yt=0x93;
	sensor_ptr->awb.win[14].yt=0x8D;
	sensor_ptr->awb.win[15].yt=0x85;
	sensor_ptr->awb.win[16].yt=0x7E;
	sensor_ptr->awb.win[17].yt=0x76;
	sensor_ptr->awb.win[18].yt=0x70;
	sensor_ptr->awb.win[19].yt=0x6D;

	sensor_ptr->awb.gain_convert[0].r=0x100;
	sensor_ptr->awb.gain_convert[0].g=0x100;
	sensor_ptr->awb.gain_convert[0].b=0x100;

	sensor_ptr->awb.gain_convert[1].r=0x100;
	sensor_ptr->awb.gain_convert[1].g=0x100;
	sensor_ptr->awb.gain_convert[1].b=0x100;

	//bpc
	sensor_ptr->bpc.flat_thr=0x50;
	sensor_ptr->bpc.std_thr=0x14;
	sensor_ptr->bpc.texture_thr=0x2;

	// denoise
	sensor_ptr->denoise.write_back=0x2;
	sensor_ptr->denoise.r_thr=0x8;
	sensor_ptr->denoise.g_thr=0x8;
	sensor_ptr->denoise.b_thr=0x8;
	sensor_ptr->denoise.diswei[0]=0xFF;
	sensor_ptr->denoise.diswei[1]=0xF7;
	sensor_ptr->denoise.diswei[2]=0xEF;
	sensor_ptr->denoise.diswei[3]=0xE8;
	sensor_ptr->denoise.diswei[4]=0xE1;
	sensor_ptr->denoise.diswei[5]=0xDA;
	sensor_ptr->denoise.diswei[6]=0xD3;
	sensor_ptr->denoise.diswei[7]=0xCC;
	sensor_ptr->denoise.diswei[8]=0xC6;
	sensor_ptr->denoise.diswei[9]=0xC0;
	sensor_ptr->denoise.diswei[10]=0xBA;
	sensor_ptr->denoise.diswei[11]=0xB4;
	sensor_ptr->denoise.diswei[12]=0xAF;
	sensor_ptr->denoise.diswei[13]=0xA9;
	sensor_ptr->denoise.diswei[14]=0xA4;
	sensor_ptr->denoise.diswei[15]=0x9F;
	sensor_ptr->denoise.diswei[16]=0x9A;
	sensor_ptr->denoise.diswei[17]=0x95;
	sensor_ptr->denoise.diswei[18]=0x91;

	sensor_ptr->denoise.ranwei[0]=0xFF;
	sensor_ptr->denoise.ranwei[1]=0xF7;
	sensor_ptr->denoise.ranwei[2]=0xE1;
	sensor_ptr->denoise.ranwei[3]=0xC0;
	sensor_ptr->denoise.ranwei[4]=0x9A;
	sensor_ptr->denoise.ranwei[5]=0x74;
	sensor_ptr->denoise.ranwei[6]=0x52;
	sensor_ptr->denoise.ranwei[7]=0x37;
	sensor_ptr->denoise.ranwei[8]=0x22;
	sensor_ptr->denoise.ranwei[9]=0x14;
	sensor_ptr->denoise.ranwei[10]=0x13;
	sensor_ptr->denoise.ranwei[11]=0x12;
	sensor_ptr->denoise.ranwei[12]=0x11;
	sensor_ptr->denoise.ranwei[13]=0x10;
	sensor_ptr->denoise.ranwei[14]=0xF;
	sensor_ptr->denoise.ranwei[15]=0xE;
	sensor_ptr->denoise.ranwei[16]=0xD;
	sensor_ptr->denoise.ranwei[17]=0xC;
	sensor_ptr->denoise.ranwei[18]=0xB;
	sensor_ptr->denoise.ranwei[19]=0xA;
	sensor_ptr->denoise.ranwei[20]=0x9;
	sensor_ptr->denoise.ranwei[21]=0x8;
	sensor_ptr->denoise.ranwei[22]=0x7;
	sensor_ptr->denoise.ranwei[23]=0x6;
	sensor_ptr->denoise.ranwei[24]=0x5;
	sensor_ptr->denoise.ranwei[25]=0x4;
	sensor_ptr->denoise.ranwei[26]=0x3;
	sensor_ptr->denoise.ranwei[27]=0x2;
	sensor_ptr->denoise.ranwei[28]=0x1;
	sensor_ptr->denoise.ranwei[29]=0x1;
	sensor_ptr->denoise.ranwei[30]=0x1;

	//GrGb
	sensor_ptr->grgb.edge_thr=0x1A;
	sensor_ptr->grgb.diff_thr=0x50;

	//cfa
	sensor_ptr->cfa.edge_thr=0x1A;
	sensor_ptr->cfa.diff_thr=0x0;

	//cmc
	sensor_ptr->cmc.matrix[0][0]=0x6F5;
	sensor_ptr->cmc.matrix[0][1]=0x3E0B;
	sensor_ptr->cmc.matrix[0][2]=0x3F00;
	sensor_ptr->cmc.matrix[0][3]=0x3E67;
	sensor_ptr->cmc.matrix[0][4]=0x6B8;
	sensor_ptr->cmc.matrix[0][5]=0x3EE1;
	sensor_ptr->cmc.matrix[0][6]=0x3FC3;
	sensor_ptr->cmc.matrix[0][7]=0x3D71;
	sensor_ptr->cmc.matrix[0][8]=0x6CC;

	//Gamma
	sensor_ptr->gamma.axis[0][0]=0x0;
	sensor_ptr->gamma.axis[0][1]=0x8;
	sensor_ptr->gamma.axis[0][2]=0x10;
	sensor_ptr->gamma.axis[0][3]=0x18;
	sensor_ptr->gamma.axis[0][4]=0x20;
	sensor_ptr->gamma.axis[0][5]=0x30;
	sensor_ptr->gamma.axis[0][6]=0x40;
	sensor_ptr->gamma.axis[0][7]=0x50;
	sensor_ptr->gamma.axis[0][8]=0x60;
	sensor_ptr->gamma.axis[0][9]=0x80;
	sensor_ptr->gamma.axis[0][10]=0xA0;
	sensor_ptr->gamma.axis[0][11]=0xC0;
	sensor_ptr->gamma.axis[0][12]=0xE0;
	sensor_ptr->gamma.axis[0][13]=0x100;
	sensor_ptr->gamma.axis[0][14]=0x120;
	sensor_ptr->gamma.axis[0][15]=0x140;
	sensor_ptr->gamma.axis[0][16]=0x180;
	sensor_ptr->gamma.axis[0][17]=0x1C0;
	sensor_ptr->gamma.axis[0][18]=0x200;
	sensor_ptr->gamma.axis[0][19]=0x240;
	sensor_ptr->gamma.axis[0][20]=0x280;
	sensor_ptr->gamma.axis[0][21]=0x300;
	sensor_ptr->gamma.axis[0][22]=0x340;
	sensor_ptr->gamma.axis[0][23]=0x380;
	sensor_ptr->gamma.axis[0][24]=0x3C0;
	sensor_ptr->gamma.axis[0][25]=0x3FF;

	sensor_ptr->gamma.axis[1][0]=0x0;
	sensor_ptr->gamma.axis[1][1]=0x5;
	sensor_ptr->gamma.axis[1][2]=0x9;
	sensor_ptr->gamma.axis[1][3]=0xE;
	sensor_ptr->gamma.axis[1][4]=0x15;
	sensor_ptr->gamma.axis[1][5]=0x20;
	sensor_ptr->gamma.axis[1][6]=0x32;
	sensor_ptr->gamma.axis[1][7]=0x44;
	sensor_ptr->gamma.axis[1][8]=0x51;
	sensor_ptr->gamma.axis[1][9]=0x60;
	sensor_ptr->gamma.axis[1][10]=0x70;
	sensor_ptr->gamma.axis[1][11]=0x7C;
	sensor_ptr->gamma.axis[1][12]=0x86;
	sensor_ptr->gamma.axis[1][13]=0x92;
	sensor_ptr->gamma.axis[1][14]=0x9F;
	sensor_ptr->gamma.axis[1][15]=0xA9;
	sensor_ptr->gamma.axis[1][16]=0xB9;
	sensor_ptr->gamma.axis[1][17]=0xC9;
	sensor_ptr->gamma.axis[1][18]=0xD4;
	sensor_ptr->gamma.axis[1][19]=0xE0;
	sensor_ptr->gamma.axis[1][20]=0xEA;
	sensor_ptr->gamma.axis[1][21]=0xF6;
	sensor_ptr->gamma.axis[1][22]=0xF9;
	sensor_ptr->gamma.axis[1][23]=0xFB;
	sensor_ptr->gamma.axis[1][24]=0xFC;
	sensor_ptr->gamma.axis[1][25]=0xFF;

	//uv div
	sensor_ptr->uv_div.thrd[0]=0xFC;
	sensor_ptr->uv_div.thrd[1]=0xFA;
	sensor_ptr->uv_div.thrd[2]=0xF8;
	sensor_ptr->uv_div.thrd[3]=0xF6;
	sensor_ptr->uv_div.thrd[4]=0xF4;
	sensor_ptr->uv_div.thrd[5]=0xF2;
	sensor_ptr->uv_div.thrd[6]=0xF0;

	//pref
	sensor_ptr->pref.write_back=0x0;
	sensor_ptr->pref.y_thr=0x4;
	sensor_ptr->pref.u_thr=0x4;
	sensor_ptr->pref.v_thr=0x4;

	//bright
	sensor_ptr->bright.factor[0]=0xD0;
	sensor_ptr->bright.factor[1]=0xE0;
	sensor_ptr->bright.factor[2]=0xF0;
	sensor_ptr->bright.factor[3]=0x0;
	sensor_ptr->bright.factor[4]=0x10;
	sensor_ptr->bright.factor[5]=0x20;
	sensor_ptr->bright.factor[6]=0x30;
	sensor_ptr->bright.factor[7]=0x0;
	sensor_ptr->bright.factor[8]=0x0;
	sensor_ptr->bright.factor[9]=0x0;
	sensor_ptr->bright.factor[10]=0x0;
	sensor_ptr->bright.factor[11]=0x0;
	sensor_ptr->bright.factor[12]=0x0;
	sensor_ptr->bright.factor[13]=0x0;
	sensor_ptr->bright.factor[14]=0x0;
	sensor_ptr->bright.factor[15]=0x0;

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
	sensor_ptr->hist.mode=0x0;
	sensor_ptr->hist.low_ratio=0x0;
	sensor_ptr->hist.high_ratio=0x0;

	//auto contrast
	sensor_ptr->auto_contrast.mode=0x0;

	//saturation
	sensor_ptr->saturation.factor[0]=0x40;
	sensor_ptr->saturation.factor[1]=0x40;
	sensor_ptr->saturation.factor[2]=0x40;
	sensor_ptr->saturation.factor[3]=0x40;
	sensor_ptr->saturation.factor[4]=0x40;
	sensor_ptr->saturation.factor[5]=0x40;
	sensor_ptr->saturation.factor[6]=0x40;
	sensor_ptr->saturation.factor[7]=0x40;
	sensor_ptr->saturation.factor[8]=0x40;
	sensor_ptr->saturation.factor[9]=0x40;
	sensor_ptr->saturation.factor[10]=0x40;
	sensor_ptr->saturation.factor[11]=0x40;
	sensor_ptr->saturation.factor[12]=0x40;
	sensor_ptr->saturation.factor[13]=0x40;
	sensor_ptr->saturation.factor[14]=0x40;
	sensor_ptr->saturation.factor[15]=0x40;

	//af info
	sensor_ptr->af.max_step=0x400;
	sensor_ptr->af.stab_period=0x14;

	//edge
	sensor_ptr->edge.info[0].detail_thr=0x3;
	sensor_ptr->edge.info[0].smooth_thr=0x5;
	sensor_ptr->edge.info[0].strength=0xA;
	sensor_ptr->edge.info[1].detail_thr=0x3;
	sensor_ptr->edge.info[1].smooth_thr=0x5;
	sensor_ptr->edge.info[1].strength=0xA;
	sensor_ptr->edge.info[2].detail_thr=0x3;
	sensor_ptr->edge.info[2].smooth_thr=0x5;
	sensor_ptr->edge.info[2].strength=0xA;
	sensor_ptr->edge.info[3].detail_thr=0x3;
	sensor_ptr->edge.info[3].smooth_thr=0x5;
	sensor_ptr->edge.info[3].strength=0xA;
	sensor_ptr->edge.info[4].detail_thr=0x3;
	sensor_ptr->edge.info[4].smooth_thr=0x5;
	sensor_ptr->edge.info[4].strength=0xA;
	sensor_ptr->edge.info[5].detail_thr=0x3;
	sensor_ptr->edge.info[5].smooth_thr=0x5;
	sensor_ptr->edge.info[5].strength=0xA;

	//emboss
	sensor_ptr->emboss.step=0x0;

	//global gain
	sensor_ptr->global.gain=0x0;

	//chn gain
	sensor_ptr->chn.r_gain=0x40;
	sensor_ptr->chn.g_gain=0x40;
	sensor_ptr->chn.b_gain=0x40;
	sensor_ptr->chn.r_offset=0x0;
	sensor_ptr->chn.r_offset=0x0;
	sensor_ptr->chn.r_offset=0x0;
#endif
	return rtn;
}


LOCAL uint32_t _s5k3h7yx_GetResolutionTrimTab(uint32_t param)
{
	SENSOR_PRINT("0x%x", (uint32_t)s_s5k3h7yx_Resolution_Trim_Tab);
	return (uint32_t) s_s5k3h7yx_Resolution_Trim_Tab;
}

LOCAL uint32_t _s5k3h7yx_PowerOn(uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_s5k3h7yx_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_s5k3h7yx_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_s5k3h7yx_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_s5k3h7yx_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_s5k3h7yx_mipi_raw_info.reset_pulse_level;
	//uint32_t reset_width=g_s5k3h7yx_yuv_info.reset_pulse_width;

	if (SENSOR_TRUE == power_on) {
		Sensor_PowerDown(power_down);
		// Open power
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		usleep(20*1000);
		_dw9174_SRCInit(2);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(10*1000);
		Sensor_PowerDown(!power_down);
		// Reset sensor
		Sensor_Reset(reset_level);
	} else {
		Sensor_PowerDown(power_down);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
	}
	SENSOR_PRINT("SENSOR_s5k3h7yx: _s5k3h7yx_Power_On(1:on, 0:off): %d", power_on);
	return SENSOR_SUCCESS;
}

LOCAL uint32_t _s5k3h7yx_cfg_otp(uint32_t  param)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_s5k3h7yx_raw_param_tab;
	uint32_t module_id=g_module_id;

	SENSOR_PRINT("SENSOR_s5k3h7yx: _s5k3h7yx_cfg_otp");

	if(PNULL!=tab_ptr[module_id].cfg_otp){
		tab_ptr[module_id].cfg_otp(0);
		}

	return rtn;
}

LOCAL uint32_t _s5k3h7yx_com_Identify_otp(void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_s5k3h7yx: _s5k3h7yx_com_Identify_otp");

	/*read param id from sensor omap*/
	param_id=s5k3h7yx_RAW_PARAM_COM;

	if(s5k3h7yx_RAW_PARAM_COM==param_id){
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}

LOCAL uint32_t _s5k3h7yx_GetRawInof(void)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_s5k3h7yx_raw_param_tab;
	uint32_t param_id;
	uint32_t i=0x00;

	/*read param id from sensor omap*/
	param_id=s5k3h7yx_RAW_PARAM_COM;

	for(i=0x00; ; i++)
	{
		g_module_id = i;
		if(RAW_INFO_END_ID==tab_ptr[i].param_id){
			if(NULL==s_s5k3h7yx_mipi_raw_info_ptr){
				SENSOR_PRINT("SENSOR_s5k3h7yx: ov5647_GetRawInof no param error");
				rtn=SENSOR_FAIL;
			}
			SENSOR_PRINT("SENSOR_s5k3h7yx: s5k3h7yx_GetRawInof end");
			break;
		}
		else if(PNULL!=tab_ptr[i].identify_otp){
			if(SENSOR_SUCCESS==tab_ptr[i].identify_otp(0))
			{
				s_s5k3h7yx_mipi_raw_info_ptr = tab_ptr[i].info_ptr;
				SENSOR_PRINT("SENSOR_s5k3h7yx: s5k3h7yx_GetRawInof success");
				break;
			}
		}
	}

	return rtn;
}

LOCAL uint32_t _s5k3h7yx_GetMaxFrameLine(uint32_t index)
{
	uint32_t max_line=0x00;
	SENSOR_TRIM_T_PTR trim_ptr=s_s5k3h7yx_Resolution_Trim_Tab;

	max_line=trim_ptr[index].frame_line;

	return max_line;
}
LOCAL uint32_t _s5k3h7yx_Identify(uint32_t param)
{
#define s5k3h7yx_PID_VALUE    0x3087
#define s5k3h7yx_PID_ADDR     0x0000
	
	uint16_t pid_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("SENSOR_s5k3h7yx: mipi raw identify\n");

	pid_value = Sensor_ReadReg(s5k3h7yx_PID_ADDR);

	if (s5k3h7yx_PID_VALUE == pid_value) {
		SENSOR_PRINT_HIGH("SENSOR_s5k3h7yx: this is s5k3h7yx sensor !");
		ret_value=_s5k3h7yx_GetRawInof();
		if(SENSOR_SUCCESS != ret_value)
		{
			SENSOR_PRINT_ERR("SENSOR_s5k3h7yx: the module is unknow error !");
		}
		Sensor_s5k3h7yx_InitRawTuneInfo();
	} else {
		SENSOR_PRINT_ERR("SENSOR_s5k3h7yx: identify fail,pid_value=%x", pid_value);
	}
	
	return ret_value;
}

LOCAL uint32_t _s5k3h7yx_write_exposure(uint32_t param)
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
	SENSOR_PRINT("SENSOR_s5k3h7yx: write_exposure line:%d, dummy:%d, size_index:%d", expsure_line, dummy_line, size_index);
	max_frame_len=_s5k3h7yx_GetMaxFrameLine(size_index);
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

LOCAL uint32_t _s5k3h7yx_write_gain(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint32_t real_gain = 0;

	real_gain = ((param&0xf)+16)*(((param>>4)&0x01)+1)*(((param>>5)&0x01)+1);
	real_gain = real_gain*(((param>>6)&0x01)+1)*(((param>>7)&0x01)+1)*(((param>>8)&0x01)+1);

	real_gain = real_gain<<1;

	SENSOR_PRINT("SENSOR_s5k3h7yx: real_gain:0x%x, param: 0x%x", real_gain, param);

	//ret_value = Sensor_WriteReg(0x104, 0x01);
	value = real_gain;
	ret_value = Sensor_WriteReg(0x204, value);
	//ret_value = Sensor_WriteReg(0x104, 0x00);

	return ret_value;
}

LOCAL uint32_t _s5k3h7yx_write_af(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint8_t cmd_val[2] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;

	SENSOR_PRINT("SENSOR_s5k3h7yx: _write_af %d", param);

	//for direct mode
	slave_addr = DW9714_VCM_SLAVE_ADDR;
	cmd_val[0] = (param&0xfff0)>>4;
	cmd_val[1] = ((param&0x0f)<<4)|0x0C;
	cmd_len = 2;	
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
	SENSOR_PRINT("SENSOR_s5k3h7yx: _write_af, ret =  %d, MSL:%x, LSL:%x\n", ret_value, cmd_val[0], cmd_val[1]);

	return ret_value;

}

LOCAL uint32_t _s5k3h7yx_ReadGain(uint32_t*  gain_ptr)
{
	uint32_t rtn = SENSOR_SUCCESS;
	uint32_t gain = 0;

	gain = Sensor_ReadReg(0x204);/*8*/

	s_s5k3h7yx_gain=gain;

	if (gain_ptr) {
		*gain_ptr = gain;
	}

	SENSOR_PRINT("SENSOR_s5k3h7yx: _ReadGain gain: 0x%x", s_s5k3h7yx_gain);

	return rtn;
}


LOCAL uint32_t _s5k3h7yx_BeforeSnapshot(uint32_t param)
{
	uint16_t ret;
	uint32_t cap_mode = (param>>CAP_MODE_BITS);
	uint32_t capture_exposure;
	uint32_t preview_exposure;
	uint32_t prv_linetime=s_s5k3h7yx_Resolution_Trim_Tab[SENSOR_MODE_PREVIEW_ONE].line_time;
	uint32_t cap_linetime;
	uint32_t frame_len = 0x00;
	uint32_t gain = 0;

	param = param&0xffff;
	SENSOR_PRINT("SENSOR_s5k3h7yx:cap_mode = %d,param = %d.",cap_mode,param);
	cap_linetime = s_s5k3h7yx_Resolution_Trim_Tab[param].line_time;


	SENSOR_PRINT("SENSOR_s5k3h7yx: BeforeSnapshot moe: %d",param);

	if (SENSOR_MODE_PREVIEW_ONE >= param){
		SENSOR_PRINT("SENSOR_s5k3h7yx: prvmode equal to capmode");
		return SENSOR_SUCCESS;
	}


	preview_exposure = Sensor_ReadReg(0x202);
	_s5k3h7yx_ReadGain(&gain);

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

LOCAL uint32_t _s5k3h7yx_after_snapshot(uint32_t param)
{
	SENSOR_PRINT("SENSOR_s5k3h7yx: after_snapshot mode:%d", param);
	Sensor_SetMode(param);
	return SENSOR_SUCCESS;
}

LOCAL uint32_t _s5k3h7yx_StreamOn(uint32_t param)
{
	SENSOR_PRINT("SENSOR_s5k3h7yx: StreamOn");

	Sensor_WriteReg(0x0100, 0x0100);

	return 0;
}

LOCAL uint32_t _s5k3h7yx_StreamOff(uint32_t param)
{
	SENSOR_PRINT("SENSOR_s5k3h7yx: StreamOff");

	Sensor_WriteReg(0x0100, 0x0000);
	usleep(40*1000);

	return 0;
}
LOCAL uint32_t _dw9174_SRCInit(uint32_t mode)
{
	uint8_t cmd_val[6] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t ret_value = SENSOR_SUCCESS;	
	
	slave_addr = DW9714_VCM_SLAVE_ADDR;
	
	switch (mode) {
		case 1:
		break;
		
		case 2:
		{			
			cmd_val[0] = 0xec;
			cmd_val[1] = 0xa3;
			cmd_len = 2;
			Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);			
			usleep(20*1000);
			cmd_val[0] = 0xf2;
			cmd_val[1] = 0x00;
			cmd_len = 2;
			Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);			
			usleep(20*1000);
			cmd_val[0] = 0xdc;
			cmd_val[1] = 0x51;			
			cmd_len = 2;
			Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);			
		}
		break;
		
		case 3:
		break;
		
	}

	return ret_value;
}
