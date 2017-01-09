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
#include "sensor_hm5040_raw_param.c"

#define hm5040_I2C_ADDR_W        0x1B
#define hm5040_I2C_ADDR_R         0x1B

#define hm5040_RAW_PARAM_COM  0x0000

#define hm5040_MIN_FRAME_LEN_PRV  0x51b
#define hm5040_2_LANES
static int s_hm5040_gain = 0;
static int s_capture_shutter = 0;
static int s_capture_VTS = 0;
static int s_video_min_framerate = 0;
static int s_video_max_framerate = 0;

LOCAL uint32_t _hm5040_GetResolutionTrimTab(uint32_t param);
LOCAL uint32_t _hm5040_PowerOn(uint32_t power_on);
LOCAL uint32_t _hm5040_Identify(uint32_t param);
LOCAL uint32_t _hm5040_BeforeSnapshot(uint32_t param);
LOCAL uint32_t _hm5040_after_snapshot(uint32_t param);
LOCAL uint32_t _hm5040_StreamOn(uint32_t param);
LOCAL uint32_t _hm5040_StreamOff(uint32_t param);
LOCAL uint32_t _hm5040_write_exposure(uint32_t param);
LOCAL uint32_t _hm5040_write_gain(uint32_t param);
LOCAL uint32_t _hm5040_write_af(uint32_t param);
LOCAL uint32_t _hm5040_flash(uint32_t param);
LOCAL uint32_t _hm5040_ExtFunc(uint32_t ctl_param);
LOCAL int _hm5040_get_VTS(void);
LOCAL int _hm5040_set_VTS(int VTS);
LOCAL uint32_t _hm5040_ReadGain(uint32_t param);
LOCAL uint32_t _hm5040_set_video_mode(uint32_t param);
LOCAL uint32_t _dw9174_SRCInit(uint32_t mode);
LOCAL int _hm5040_get_shutter(void);
LOCAL uint32_t _hm5040_com_Identify_otp(void* param_ptr);

LOCAL const struct raw_param_info_tab s_hm5040_raw_param_tab[]={
	{hm5040_RAW_PARAM_COM, &s_hm5040_mipi_raw_info, _hm5040_com_Identify_otp, PNULL},
	{RAW_INFO_END_ID, PNULL, PNULL, PNULL}
};

struct sensor_raw_info* s_hm5040_mipi_raw_info_ptr=&s_hm5040_mipi_raw_info;

static uint32_t g_module_id = 0;
static uint32_t g_flash_mode_en = 0;
static uint32_t g_af_slewrate = 1;

LOCAL const SENSOR_REG_T hm5040_common_init[] = {
	//{0x0103,0x01},
	//{0x0100,0x00},
	{0x3002,0x32},
	{0x3016,0x46},
	{0x3017,0x29},
	{0x3003,0x03},
	{0x3045,0x03},

	{0xFBD7,0x00},  // FF_SUB_G1 HI
	{0xFBD8,0x00},  // FF_SUB_G1 LO
	{0xFBD9,0x00},  // FF_DIV_G1 HI
	{0xFBDA,0x00},  // FF_DIV_G1 LO
	{0xFBDB,0x00},  // FF_SUB_G2 HI
	{0xFBDC,0x00},  // FF_SUB_G2 LO
	{0xFBDD,0x00},  // FF_DIV_G2 HI
	{0xFBDE,0x00},  // FF_DIV_G2 LO
	{0xFBDF,0x00},  // FF_SUB_G4 HI
	{0xFBE0,0x00},  // FF_SUB_G4 LO
	{0xFBE1,0x00},  // FF_DIV_G4 HI
	{0xFBE2,0x00},  // FF_DIV_G4 LO
	{0xFBE3,0x00},  // FF_SUB_G8 HI
	{0xFBE4,0x00},  // FF_SUB_G8 LO
	{0xFBE5,0x00},  // FF_DIV_G8 HI
	{0xFBE6,0x00},  // FF_DIV_G8 LO
	{0xFBE7,0x00},  // FF_SUB_G16 HI
	{0xFBE8,0x00},  // FF_SUB_G16 LO
	{0xFBE9,0x00},  // FF_DIV_G16 HI
	{0xFBEA,0x00},  // FF_DIV_G16 LO
	{0xFBEB,0x00},  // AB_SUB_G1 HI
	{0xFBEC,0x00},  // AB_SUB_G1 LO
	{0xFBED,0x00},  // AB_DIV_G1 HI
	{0xFBEE,0x00},  // AB_DIV_G1 LO
	{0xFBEF,0x00},  // AB_SUB_G2 HI
	{0xFBF0,0x00},  // AB_SUB_G2 LO
	{0xFBF1,0x00},  // AB_DIV_G2 HI
	{0xFBF2,0x00},  // AB_DIV_G2 LO
	{0xFBF3,0x00},  // AB_SUB_G4 HI
	{0xFBF4,0x00},  // AB_SUB_G4 LO
	{0xFBF5,0x00},  // AB_DIV_G4 HI
	{0xFBF6,0x00},  // AB_DIV_G4 LO
	{0xFBF7,0x00},  // AB_SUB_G8 HI
	{0xFBF8,0x00},  // AB_SUB_G8 LO
	{0xFBF9,0x00},  // AB_DIV_G8 HI
	{0xFBFA,0x00},  // AB_DIV_G8 LO
	{0xFBFB,0x00},  // AB_SUB_G16 HI
	{0xFBFC,0x00},  // AB_SUB_G16 LO
	{0xFBFD,0x00},  // AB_DIV_G16 HI
	{0xFBFE,0x00},  // AB_DIV_G16 LO
	{0xFB00,0x51},
	{0xF800,0xc0},
	{0xF801,0x24},
	{0xF802,0x7c},
	{0xF803,0xfb},
	{0xF804,0x7d},
	{0xF805,0xc7},
	{0xF806,0x7b},
	{0xF807,0x10},
	{0xF808,0x7f},
	{0xF809,0x72},
	{0xF80A,0x7e},
	{0xF80B,0x30},
	{0xF80C,0x12},
	{0xF80D,0x09},
	{0xF80E,0x47},
	{0xF80F,0xd0},
	{0xF810,0x24},
	{0xF811,0x90},
	{0xF812,0x02},
	{0xF813,0x05},
	{0xF814,0xe0},
	{0xF815,0xf5},
	{0xF816,0x77},
	{0xF817,0xe5},
	{0xF818,0x77},
	{0xF819,0xc3},
	{0xF81A,0x94},
	{0xF81B,0x80},
	{0xF81C,0x50},
	{0xF81D,0x08},
	{0xF81E,0x75},
	{0xF81F,0x7a},
	{0xF820,0xfb},
	{0xF821,0x75},
	{0xF822,0x7b},
	{0xF823,0xd7},
	{0xF824,0x80},
	{0xF825,0x33},
	{0xF826,0xe5},
	{0xF827,0x77},
	{0xF828,0xc3},
	{0xF829,0x94},
	{0xF82A,0xc0},
	{0xF82B,0x50},
	{0xF82C,0x08},
	{0xF82D,0x75},
	{0xF82E,0x7a},
	{0xF82F,0xfb},
	{0xF830,0x75},
	{0xF831,0x7b},
	{0xF832,0xdb},
	{0xF833,0x80},
	{0xF834,0x24},
	{0xF835,0xe5},
	{0xF836,0x77},
	{0xF837,0xc3},
	{0xF838,0x94},
	{0xF839,0xe0},
	{0xF83A,0x50},
	{0xF83B,0x08},
	{0xF83C,0x75},
	{0xF83D,0x7a},
	{0xF83E,0xfb},
	{0xF83F,0x75},
	{0xF840,0x7b},
	{0xF841,0xdf},
	{0xF842,0x80},
	{0xF843,0x15},
	{0xF844,0xe5},
	{0xF845,0x77},
	{0xF846,0xc3},
	{0xF847,0x94},
	{0xF848,0xf0},
	{0xF849,0x50},
	{0xF84A,0x08},
	{0xF84B,0x75},
	{0xF84C,0x7a},
	{0xF84D,0xfb},
	{0xF84E,0x75},
	{0xF84F,0x7b},
	{0xF850,0xe3},
	{0xF851,0x80},
	{0xF852,0x06},
	{0xF853,0x75},
	{0xF854,0x7a},
	{0xF855,0xfb},
	{0xF856,0x75},
	{0xF857,0x7b},
	{0xF858,0xe7},
	{0xF859,0xe5},
	{0xF85A,0x55},
	{0xF85B,0x7f},
	{0xF85C,0x00},
	{0xF85D,0xb4},
	{0xF85E,0x22},
	{0xF85F,0x02},
	{0xF860,0x7f},
	{0xF861,0x01},
	{0xF862,0xe5},
	{0xF863,0x53},
	{0xF864,0x5f},
	{0xF865,0x60},
	{0xF866,0x05},
	{0xF867,0x74},
	{0xF868,0x14},
	{0xF869,0x12},
	{0xF86A,0xfa},
	{0xF86B,0x4c},
	{0xF86C,0x75},
	{0xF86D,0x7c},
	{0xF86E,0xfb},
	{0xF86F,0x75},
	{0xF870,0x7d},
	{0xF871,0xc7},
	{0xF872,0x75},
	{0xF873,0x7e},
	{0xF874,0x30},
	{0xF875,0x75},
	{0xF876,0x7f},
	{0xF877,0x62},
	{0xF878,0xe4},
	{0xF879,0xf5},
	{0xF87A,0x77},
	{0xF87B,0xe5},
	{0xF87C,0x77},
	{0xF87D,0xc3},
	{0xF87E,0x94},
	{0xF87F,0x08},
	{0xF880,0x40},
	{0xF881,0x03},
	{0xF882,0x02},
	{0xF883,0xf9},
	{0xF884,0x0e},
	{0xF885,0x85},
	{0xF886,0x7d},
	{0xF887,0x82},
	{0xF888,0x85},
	{0xF889,0x7c},
	{0xF88A,0x83},
	{0xF88B,0xe0},
	{0xF88C,0xfe},
	{0xF88D,0xa3},
	{0xF88E,0xe0},
	{0xF88F,0xff},
	{0xF890,0x12},
	{0xF891,0x21},
	{0xF892,0x22},
	{0xF893,0x8e},
	{0xF894,0x78},
	{0xF895,0x8f},
	{0xF896,0x79},
	{0xF897,0x12},
	{0xF898,0xfa},
	{0xF899,0x40},
	{0xF89A,0x12},
	{0xF89B,0x22},
	{0xF89C,0x93},
	{0xF89D,0x50},
	{0xF89E,0x07},
	{0xF89F,0xe4},
	{0xF8A0,0xf5},
	{0xF8A1,0x78},
	{0xF8A2,0xf5},
	{0xF8A3,0x79},
	{0xF8A4,0x80},
	{0xF8A5,0x33},
	{0xF8A6,0x12},
	{0xF8A7,0xfa},
	{0xF8A8,0x40},
	{0xF8A9,0x7b},
	{0xF8AA,0x01},
	{0xF8AB,0xaf},
	{0xF8AC,0x79},
	{0xF8AD,0xae},
	{0xF8AE,0x78},
	{0xF8AF,0x12},
	{0xF8B0,0x22},
	{0xF8B1,0x4f},
	{0xF8B2,0x74},
	{0xF8B3,0x02},
	{0xF8B4,0x12},
	{0xF8B5,0xfa},
	{0xF8B6,0x4c},
	{0xF8B7,0x85},
	{0xF8B8,0x7b},
	{0xF8B9,0x82},
	{0xF8BA,0xf5},
	{0xF8BB,0x83},
	{0xF8BC,0xe0},
	{0xF8BD,0xfe},
	{0xF8BE,0xa3},
	{0xF8BF,0xe0},
	{0xF8C0,0xff},
	{0xF8C1,0x7d},
	{0xF8C2,0x03},
	{0xF8C3,0x12},
	{0xF8C4,0x17},
	{0xF8C5,0xd8},
	{0xF8C6,0x12},
	{0xF8C7,0x1b},
	{0xF8C8,0x9b},
	{0xF8C9,0x8e},
	{0xF8CA,0x78},
	{0xF8CB,0x8f},
	{0xF8CC,0x79},
	{0xF8CD,0x74},
	{0xF8CE,0xfe},
	{0xF8CF,0x25},
	{0xF8D0,0x7b},
	{0xF8D1,0xf5},
	{0xF8D2,0x7b},
	{0xF8D3,0x74},
	{0xF8D4,0xff},
	{0xF8D5,0x35},
	{0xF8D6,0x7a},
	{0xF8D7,0xf5},
	{0xF8D8,0x7a},
	{0xF8D9,0x78},
	{0xF8DA,0x24},
	{0xF8DB,0xe6},
	{0xF8DC,0xff},
	{0xF8DD,0xc3},
	{0xF8DE,0x74},
	{0xF8DF,0x20},
	{0xF8E0,0x9f},
	{0xF8E1,0x7e},
	{0xF8E2,0x00},
	{0xF8E3,0x25},
	{0xF8E4,0x79},
	{0xF8E5,0xff},
	{0xF8E6,0xee},
	{0xF8E7,0x35},
	{0xF8E8,0x78},
	{0xF8E9,0x85},
	{0xF8EA,0x7f},
	{0xF8EB,0x82},
	{0xF8EC,0x85},
	{0xF8ED,0x7e},
	{0xF8EE,0x83},
	{0xF8EF,0xf0},
	{0xF8F0,0xa3},
	{0xF8F1,0xef},
	{0xF8F2,0xf0},
	{0xF8F3,0x05},
	{0xF8F4,0x77},
	{0xF8F5,0x74},
	{0xF8F6,0x02},
	{0xF8F7,0x25},
	{0xF8F8,0x7d},
	{0xF8F9,0xf5},
	{0xF8FA,0x7d},
	{0xF8FB,0xe4},
	{0xF8FC,0x35},
	{0xF8FD,0x7c},
	{0xF8FE,0xf5},
	{0xF8FF,0x7c},
	{0xF900,0x74},
	{0xF901,0x02},
	{0xF902,0x25},
	{0xF903,0x7f},
	{0xF904,0xf5},
	{0xF905,0x7f},
	{0xF906,0xe4},
	{0xF907,0x35},
	{0xF908,0x7e},
	{0xF909,0xf5},
	{0xF90A,0x7e},
	{0xF90B,0x02},
	{0xF90C,0xf8},
	{0xF90D,0x7b},
	{0xF90E,0x22},
	{0xF90F,0x90},

	// Apply BayerAVG Config
	{0xF910,0x30},
	{0xF911,0x47},
	{0xF912,0x74},
	{0xF913,0x98},
	{0xF914,0xf0},
	{0xF915,0x90},
	{0xF916,0x30},
	{0xF917,0x36},
	{0xF918,0x74},
	{0xF919,0x1e},
	{0xF91A,0xf0},
	{0xF91B,0x90},
	{0xF91C,0x30},
	{0xF91D,0x42},
	{0xF91E,0x74},
	{0xF91F,0x24},
	{0xF920,0xf0},
	{0xF921,0xe5},
	{0xF922,0x53},
	{0xF923,0x60},
	{0xF924,0x42},
	{0xF925,0x78},
	{0xF926,0x2b},
	{0xF927,0x76},
	{0xF928,0x01},
	{0xF929,0xe5},
	{0xF92A,0x55},
	{0xF92B,0xb4},
	{0xF92C,0x22},
	{0xF92D,0x17},
	{0xF92E,0x90},
	{0xF92F,0x30},
	{0xF930,0x36},
	{0xF931,0x74},
	{0xF932,0x46},
	{0xF933,0xf0},
	{0xF934,0x78},
	{0xF935,0x28},
	{0xF936,0x76},
	{0xF937,0x31},
	{0xF938,0x90},
	{0xF939,0x30},
	{0xF93A,0x0e},
	{0xF93B,0xe0},
	{0xF93C,0xc3},
	{0xF93D,0x13},
	{0xF93E,0x30},
	{0xF93F,0xe0},
	{0xF940,0x04},
	{0xF941,0x78},
	{0xF942,0x26},
	{0xF943,0x76},
	{0xF944,0x40},
	{0xF945,0xe5},
	{0xF946,0x55},
	{0xF947,0xb4},
	{0xF948,0x44},
	{0xF949,0x21},
	{0xF94A,0x90},
	{0xF94B,0x30},
	{0xF94C,0x47},
	{0xF94D,0x74},
	{0xF94E,0x9a},
	{0xF94F,0xf0},
	{0xF950,0x90},
	{0xF951,0x30},
	{0xF952,0x42},
	{0xF953,0x74},
	{0xF954,0x64},
	{0xF955,0xf0},
	{0xF956,0x90},
	{0xF957,0x30},
	{0xF958,0x0e},
	{0xF959,0xe0},
	{0xF95A,0x13},
	{0xF95B,0x13},
	{0xF95C,0x54},
	{0xF95D,0x3f},
	{0xF95E,0x30},
	{0xF95F,0xe0},
	{0xF960,0x0a},
	{0xF961,0x78},
	{0xF962,0x24},
	{0xF963,0xe4},
	{0xF964,0xf6},
	{0xF965,0x80},
	{0xF966,0x04},
	{0xF967,0x78},
	{0xF968,0x2b},
	{0xF969,0xe4},
	{0xF96A,0xf6},
	{0xF96B,0x90},
	{0xF96C,0x30},
	{0xF96D,0x88},
	{0xF96E,0x02},
	{0xF96F,0x1d},
	{0xF970,0x4f},
	{0xF971,0x22},

	// Flash Strobe Trigger 1
	{0xF972,0x90},
	{0xF973,0x0c},
	{0xF974,0x1a},
	{0xF975,0xe0},
	{0xF976,0x30},
	{0xF977,0xe2},
	{0xF978,0x18},
	{0xF979,0x90},
	{0xF97A,0x33},
	{0xF97B,0x68},
	{0xF97C,0xe0},
	{0xF97D,0x64},
	{0xF97E,0x05},
	{0xF97F,0x70},
	{0xF980,0x2f},
	{0xF981,0x90},
	{0xF982,0x30},
	{0xF983,0x38},
	{0xF984,0xe0},
	{0xF985,0x70},
	{0xF986,0x02},
	{0xF987,0xa3},
	{0xF988,0xe0},
	{0xF989,0xc3},
	{0xF98A,0x70},
	{0xF98B,0x01},
	{0xF98C,0xd3},
	{0xF98D,0x40},
	{0xF98E,0x21},
	{0xF98F,0x80},
	{0xF990,0x1b},
	{0xF991,0x90},
	{0xF992,0x33},
	{0xF993,0x68},
	{0xF994,0xe0},
	{0xF995,0xb4},
	{0xF996,0x05},
	{0xF997,0x18},
	{0xF998,0xc3},
	{0xF999,0x90},
	{0xF99A,0x30},
	{0xF99B,0x3b},
	{0xF99C,0xe0},
	{0xF99D,0x94},
	{0xF99E,0x0d},
	{0xF99F,0x90},
	{0xF9A0,0x30},
	{0xF9A1,0x3a},
	{0xF9A2,0xe0},
	{0xF9A3,0x94},
	{0xF9A4,0x00},
	{0xF9A5,0x50},
	{0xF9A6,0x02},
	{0xF9A7,0x80},
	{0xF9A8,0x01},
	{0xF9A9,0xc3},
	{0xF9AA,0x40},
	{0xF9AB,0x04},
	{0xF9AC,0x75},
	{0xF9AD,0x10},
	{0xF9AE,0x01},
	{0xF9AF,0x22},
	{0xF9B0,0x02},
	{0xF9B1,0x16},
	{0xF9B2,0xe1},
	{0xF9B3,0x22},
	{0xF9B4,0x90},

	// Copy NVM Dark Shade Data
	{0xF9B5,0xff},
	{0xF9B6,0x33},
	{0xF9B7,0xe0},
	{0xF9B8,0x90},
	{0xF9B9,0xff},
	{0xF9BA,0x34},
	{0xF9BB,0xe0},
	{0xF9BC,0x60},
	{0xF9BD,0x0d},
	{0xF9BE,0x7c},
	{0xF9BF,0xfb},
	{0xF9C0,0x7d},
	{0xF9C1,0xd7},
	{0xF9C2,0x7b},
	{0xF9C3,0x28},
	{0xF9C4,0x7f},
	{0xF9C5,0x34},
	{0xF9C6,0x7e},
	{0xF9C7,0xff},
	{0xF9C8,0x12},
	{0xF9C9,0x09},
	{0xF9CA,0x47},
	{0xF9CB,0x7f},
	{0xF9CC,0x20},
	{0xF9CD,0x7e},
	{0xF9CE,0x01},
	{0xF9CF,0x7d},
	{0xF9D0,0x00},
	{0xF9D1,0x7c},
	{0xF9D2,0x00},
	{0xF9D3,0x12},
	{0xF9D4,0x12},
	{0xF9D5,0xa4},
	{0xF9D6,0xe4},
	{0xF9D7,0x90},
	{0xF9D8,0x3e},
	{0xF9D9,0x44},
	{0xF9DA,0xf0},
	{0xF9DB,0x02},
	{0xF9DC,0x16},
	{0xF9DD,0x7e},
	{0xF9DE,0x22},
	{0xF9DF,0xe5},

	// Call SLC Copy
	{0xF9E0,0x44},
	{0xF9E1,0x60},
	{0xF9E2,0x10},
	{0xF9E3,0x90},
	{0xF9E4,0xf6},
	{0xF9E5,0x2c},
	{0xF9E6,0x74},
	{0xF9E7,0x04},
	{0xF9E8,0xf0},
	{0xF9E9,0x90},
	{0xF9EA,0xf6},
	{0xF9EB,0x34},
	{0xF9EC,0xf0},
	{0xF9ED,0x90},
	{0xF9EE,0xf6},
	{0xF9EF,0x3c},
	{0xF9F0,0xf0},
	{0xF9F1,0x80},
	{0xF9F2,0x0e},
	{0xF9F3,0x90},
	{0xF9F4,0xf5},
	{0xF9F5,0xc0},
	{0xF9F6,0x74},
	{0xF9F7,0x04},
	{0xF9F8,0xf0},
	{0xF9F9,0x90},
	{0xF9FA,0xf5},
	{0xF9FB,0xc8},
	{0xF9FC,0xf0},
	{0xF9FD,0x90},
	{0xF9FE,0xf5},
	{0xF9FF,0xd0},
	{0xFA00,0xf0},
	{0xFA01,0x90},
	{0xFA02,0xfb},
	{0xFA03,0x7f},
	{0xFA04,0x02},
	{0xFA05,0x19},
	{0xFA06,0x0b},
	{0xFA07,0x22},
	{0xFA08,0x90},

	// Flash Strobe Start Streaming
	{0xFA09,0x0c},
	{0xFA0A,0x1a},
	{0xFA0B,0xe0},
	{0xFA0C,0x20},
	{0xFA0D,0xe2},
	{0xFA0E,0x15},
	{0xFA0F,0xe4},
	{0xFA10,0x90},
	{0xFA11,0x30},
	{0xFA12,0xf8},
	{0xFA13,0xf0},
	{0xFA14,0xa3},
	{0xFA15,0xf0},
	{0xFA16,0x90},
	{0xFA17,0x30},
	{0xFA18,0xf1},
	{0xFA19,0xe0},
	{0xFA1A,0x44},
	{0xFA1B,0x08},
	{0xFA1C,0xf0},
	{0xFA1D,0x90},
	{0xFA1E,0x30},
	{0xFA1F,0xf0},
	{0xFA20,0xe0},
	{0xFA21,0x44},
	{0xFA22,0x08},
	{0xFA23,0xf0},
	{0xFA24,0x02},
	{0xFA25,0x03},
	{0xFA26,0xde},
	{0xFA27,0x22},
	{0xFA28,0x90},

	// Flash Strobe Frame Start
	{0xFA29,0x0c},
	{0xFA2A,0x1a},
	{0xFA2B,0xe0},
	{0xFA2C,0x30},
	{0xFA2D,0xe2},
	{0xFA2E,0x0d},
	{0xFA2F,0xe0},
	{0xFA30,0x20},
	{0xFA31,0xe0},
	{0xFA32,0x06},
	{0xFA33,0x90},
	{0xFA34,0xfb},
	{0xFA35,0x85},
	{0xFA36,0x74},
	{0xFA37,0x00},
	{0xFA38,0xa5},
	{0xFA39,0x12},
	{0xFA3A,0x16},
	{0xFA3B,0xa0},
	{0xFA3C,0x02},
	{0xFA3D,0x18},
	{0xFA3E,0xac},
	{0xFA3F,0x22},
	{0xFA40,0x85},
	{0xFA41,0x7b},
	{0xFA42,0x82},
	{0xFA43,0x85},
	{0xFA44,0x7a},
	{0xFA45,0x83},
	{0xFA46,0xe0},
	{0xFA47,0xfc},
	{0xFA48,0xa3},
	{0xFA49,0xe0},
	{0xFA4A,0xfd},
	{0xFA4B,0x22},
	{0xFA4C,0x25},
	{0xFA4D,0x7b},
	{0xFA4E,0xf5},
	{0xFA4F,0x7b},
	{0xFA50,0xe4},
	{0xFA51,0x35},
	{0xFA52,0x7a},
	{0xFA53,0xf5},
	{0xFA54,0x7a},
	{0xFA55,0x22},
	{0xFA56,0xc0},

	// Flash Strobe ISR Timer 1
	{0xFA57,0xd0},
	{0xFA58,0x90},
	{0xFA59,0x35},
	{0xFA5A,0xb5},
	{0xFA5B,0xe0},
	{0xFA5C,0x54},
	{0xFA5D,0xfc},
	{0xFA5E,0x44},
	{0xFA5F,0x01},
	{0xFA60,0xf0},
	{0xFA61,0x12},
	{0xFA62,0x1f},
	{0xFA63,0x5f},
	{0xFA64,0xd0},
	{0xFA65,0xd0},
	{0xFA66,0x02},
	{0xFA67,0x0a},
	{0xFA68,0x16},
	{0xFA69,0x22},

	// Flash Strobe Line Count Int
	{0xFA6A,0x90},
	{0xFA6B,0x0c},
	{0xFA6C,0x1a},
	{0xFA6D,0xe0},
	{0xFA6E,0x20},
	{0xFA6F,0xe0},
	{0xFA70,0x06},
	{0xFA71,0x90},
	{0xFA72,0xfb},
	{0xFA73,0x85},
	{0xFA74,0x74},
	{0xFA75,0x00},
	{0xFA76,0xa5},
	{0xFA77,0xe5},
	{0xFA78,0x10},
	{0xFA79,0x02},
	{0xFA7A,0x1e},
	{0xFA7B,0x8f},
	{0xFA7C,0x22},

	// Flash Strobe Trigger 3
	{0xFA7D,0x90},
	{0xFA7E,0xfb},
	{0xFA7F,0x85},
	{0xFA80,0x74},
	{0xFA81,0x00},
	{0xFA82,0xa5},
	{0xFA83,0xe5},
	{0xFA84,0x1a},
	{0xFA85,0x60},
	{0xFA86,0x03},
	{0xFA87,0x02},
	{0xFA88,0x17},
	{0xFA89,0x47},
	{0xFA8A,0x22},

	// Call Dark Cal Transform
	{0xFA8B,0x90},
	{0xFA8C,0xfb},
	{0xFA8D,0x84},
	{0xFA8E,0x02},
	{0xFA8F,0x18},
	{0xFA90,0xd9},
	{0xFA91,0x22},

	// Flash Strobe Stop
	{0xFA92,0x02},
	{0xFA93,0x1f},
	{0xFA94,0xb1},
	{0xFA95,0x22},


	{0x35D8,0x01},
	{0x35D9,0x0F},
	{0x35DA,0x01},
	{0x35DB,0x72},
	{0x35DC,0x01},
	{0x35DD,0xB4},
	{0x35DE,0x01},
	{0x35DF,0xDF},
	{0x35E0,0x02},
	{0x35E1,0x08},
	{0x35E2,0x02},
	{0x35E3,0x28},
	{0x35E4,0x02},
	{0x35E5,0x56},
	{0x35E6,0x02},
	{0x35E7,0x6A},
	{0x35E8,0x02},
	{0x35E9,0x7D},
	{0x35EA,0x02},
	{0x35EB,0x8B},
	{0x35EC,0x02},
	{0x35ED,0x92},
	{0x35EF,0x22},
	{0x35F1,0x23},
	{0x35F3,0x22},
	{0x35F6,0x19},
	{0x35F7,0x55},
	{0x35F8,0x1D},
	{0x35F9,0x4C},
	{0x35FA,0x16},
	{0x35FB,0xC7},
	{0x35FC,0x1A},
	{0x35FD,0xA0},
	{0x35FE,0x18},
	{0x35FF,0xD6},
	{0x3600,0x03},
	{0x3601,0xD4},
	{0x3602,0x18},
	{0x3603,0x8A},
	{0x3604,0x0A},
	{0x3605,0x0D},
	{0x3606,0x1E},
	{0x3607,0x8D},
	{0x3608,0x17},
	{0x3609,0x43},
	{0x360A,0x19},
	{0x360B,0x16},
	{0x360C,0x1F},
	{0x360D,0xAD},
	{0x360E,0x19},
	{0x360F,0x08},
	{0x3610,0x14},
	{0x3611,0x26},
	{0x3612,0x1A},
	{0x3613,0xB3},
	{0x35D2,0x7F},
	{0x35D3,0xFF},
	{0x35D4,0x70},
	{0x35D0,0x01},
	{0x3E44,0x01},
	{0x0114,0x01},
	{0x2136,0x18},
	{0x2137,0x00},
	{0x0101,0x03},//Flip and mirror
	{0x0112,0x0A},
	{0x0113,0x0A},
	{0x3016,0x46},
	{0x3017,0x29},
	{0x3003,0x03},
	{0x3045,0x03},
	{0x3047,0x98},
	{0x0305,0x04},//MCLK=24Mhz
	{0x0306,0x00},
	{0x0307,0x96},//MIPI Clock=225Mhz
	{0x0301,0x0A},
	{0x0309,0x0A},
	{0x0344,0x00},
	{0x0345,0x00},
	{0x0346,0x00},
	{0x0347,0x00},
	{0x0348,0x0A},
	{0x0349,0x27},
	{0x034A,0x07},
	{0x034B,0x9F},
	{0x034C,0x05},//x_output_size=1296
	{0x040D,0x08},
	{0x034E,0x03},//y_output_size=972
	{0x034F,0xCC},
	{0x0383,0x01},
	{0x0387,0x01},
	{0x0202,0x08},
	{0x0203,0x00},
	{0x0205,0xC0},
	{0x0900,0x00},
	{0x0901,0x00},
	{0x0902,0x00},
	{0x0409,0x00},
	{0x040B,0x00},
	{0x040C,0x02},//1296
	{0x040D,0x80},
	{0x040E,0x01},//972
	{0x040F,0xE0},
};

//@@ FVGA 2lane 720x480 60fps
LOCAL const SENSOR_REG_T hm5040_720x480_setting[] = {

};

LOCAL const SENSOR_REG_T hm5040_1296x972_setting[] = {
	{0x0900,0x01},//enable binning
	{0x0901,0x22},//binning type
	{0x0902,0x00},//binning weighting
	{0x0344,0x00},//x start
	{0x0345,0x00},
	{0x0346,0x00},//y start
	{0x0347,0x00},
	{0x0348,0x0A},//x end
	{0x0349,0x27},//2599
	{0x034A,0x07},//y end
	{0x034B,0x9F},//1951
	{0x034C,0x05},//x_output_size
	{0x034D,0x08},//1296
	{0x034E,0x03},//Y_output_size
	{0x034F,0xCC},//972
	{0x0383,0x01},//x odd
	{0x0387,0x01},//y odd
	{0x0409,0x00},//Digital window X-offset
	{0x040B,0x00},//Digital window Y-offset
	{0x040C,0x05},//Digital window X=1296
	{0x040D,0x08},
	{0x040E,0x03},//Digital window Y=972
	{0x040F,0xcc},
	//{0x0100,0x01},
};

LOCAL const SENSOR_REG_T hm5040_1920x1080_setting[] = {

};

LOCAL const SENSOR_REG_T hm5040_2592x1944_setting[] = {
	{0x0900,0x00},//disable binning
	{0x0901,0x00},//binning type
	{0x0902,0x00},//binning weighting
	{0x0340,0x07},
	{0x0341,0xC4},
	{0x0344,0x00},//Analog x-start
	{0x0345,0x00},
	{0x0346,0x00},//Analog y-start
	{0x0347,0x00},
	{0x0348,0x0A},//Analog x-end
	{0x0349,0x27},//2599
	{0x034A,0x07},//Analog y-end
	{0x034B,0x9F},//1951
	{0x034C,0x0A},//X_output_size
	{0x034D,0x18},//2592
	{0x034E,0x07},//Y_output_size
	{0x034F,0x98},//1944
	{0x0383,0x01},//x odd
	{0x0387,0x01},//y odd
	{0x0409,0x00},//Digital window X-offset
	{0x040B,0x00},//Digital window Y-offset
	{0x040C,0x0A},//Digital window X=2592
	{0x040D,0x18},
	{0x040E,0x07},//Digital window X=1944
	{0x040F,0x98},
};

LOCAL SENSOR_REG_TAB_INFO_T s_hm5040_resolution_Tab_RAW[] = {
	{ADDR_AND_LEN_OF_ARRAY(hm5040_common_init), 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(hm5040_1296x972_setting), 1296, 972, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(hm5040_2592x1944_setting), 2592,1944, 24, SENSOR_IMAGE_FORMAT_RAW},

	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

LOCAL SENSOR_TRIM_T s_hm5040_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 12, 1280, 960, 229, 120, 998, {0, 0, 1280, 960}},
	{0, 24, 2560, 1920, 229, 120, 1988, {0, 0, 2560, 1920}},
	//{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}}
};

LOCAL const SENSOR_REG_T s_hm5040_1296x972_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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
LOCAL const SENSOR_REG_T s_hm5040_2592x1944_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

LOCAL const SENSOR_REG_T  s_hm5040_3264x2448_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

LOCAL SENSOR_VIDEO_INFO_T s_hm5040_video_info[] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{30, 30, 229, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_hm5040_1296x972_video_tab},
	{{{15, 15, 229, 64}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_hm5040_2592x1944_video_tab},

	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}
};

LOCAL uint32_t _hm5040_set_video_mode(uint32_t param)
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

	if (PNULL == s_hm5040_video_info[mode].setting_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR)&s_hm5040_video_info[mode].setting_ptr[param];
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

LOCAL SENSOR_IOCTL_FUNC_TAB_T s_hm5040_ioctl_func_tab = {
	PNULL,
	_hm5040_PowerOn,
	PNULL,
	_hm5040_Identify,

	PNULL,			// write register
	PNULL,			// read  register
	PNULL,
	_hm5040_GetResolutionTrimTab,

	// External
	PNULL,
	PNULL,
	PNULL,

	PNULL, //_hm5040_set_brightness,
	PNULL, // _hm5040_set_contrast,
	PNULL,
	PNULL,			//_hm5040_set_saturation,

	PNULL, //_hm5040_set_work_mode,
	PNULL, //_hm5040_set_image_effect,

	_hm5040_BeforeSnapshot,
	_hm5040_after_snapshot,
	PNULL,//	_hm5040_flash,
	PNULL,
	_hm5040_write_exposure,
	PNULL,
	_hm5040_write_gain,
	PNULL,
	PNULL,
	_hm5040_write_af,
	PNULL,
	PNULL, //_hm5040_set_awb,
	PNULL,
	PNULL,
	PNULL, //_hm5040_set_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL, //_hm5040_GetExifInfo,
	PNULL,//	_hm5040_ExtFunc,
	PNULL, //_hm5040_set_anti_flicker,
	_hm5040_set_video_mode,
	PNULL, //pick_jpeg_stream
	PNULL,  //meter_mode
	PNULL, //get_status
	_hm5040_StreamOn,
	_hm5040_StreamOff,
	PNULL
};


SENSOR_INFO_T g_hm5040_mipi_raw_info = {
	hm5040_I2C_ADDR_W,	// salve i2c write address
	hm5040_I2C_ADDR_R,	// salve i2c read address

	SENSOR_I2C_REG_16BIT | SENSOR_I2C_REG_8BIT | SENSOR_I2C_FREQ_100,	// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
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
	SENSOR_LOW_LEVEL_PWDN,

	1,			// count of identify code
	{{0x2016, 0x03},		// supply two code to identify sensor.
	 {0x2017, 0xbb}},		// for Example: index = 0-> Device id, index = 1 -> version id
	SENSOR_AVDD_2800MV,	// voltage of avdd

	2592,			// max width of source image
	1944,			// max height of source image
	"hm5040",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_R,  //SENSOR_IMAGE_PATTERN_RAWRGB_GR,// pattern of input image form sensor;

	s_hm5040_resolution_Tab_RAW,	// point to resolution table information structure
	&s_hm5040_ioctl_func_tab,	// point to ioctl function table
	&s_hm5040_mipi_raw_info_ptr,		// information and table about Rawrgb sensor
	NULL,			//&g_hm5040_ext_info,                // extend information about sensor
	SENSOR_AVDD_1800MV,	// iovdd
	SENSOR_AVDD_1800MV,	// dvdd
	1,			// skip frame num before preview
	3,			// skip frame num before capture
	0,			// deci frame num during preview
	0,			// deci frame num during video preview

	0,
	0,
	0,
	0,
	0,
	{SENSOR_INTERFACE_TYPE_CSI2, 2, 10, 0},
	s_hm5040_video_info,
	3,			// skip frame num while change setting
};

LOCAL struct sensor_raw_info* Sensor_GetContext(void)
{
	return s_hm5040_mipi_raw_info_ptr;
}

LOCAL uint32_t Sensor_hm5040_InitRawTuneInfo(void)
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

int _hm5040_get_shutter(void)
{
	// read shutter, in number of line period
	int shutter = 0;

	shutter = (uint8_t) Sensor_ReadReg(0x202);
	shutter = (shutter << 8) + (uint8_t) Sensor_ReadReg(0x203);

	return shutter;

}

LOCAL uint32_t _hm5040_GetResolutionTrimTab(uint32_t param)
{
	SENSOR_PRINT("0x%x",  (uint32_t)s_hm5040_Resolution_Trim_Tab);
	return (uint32_t) s_hm5040_Resolution_Trim_Tab;
}
LOCAL uint32_t _hm5040_PowerOn(uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_hm5040_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_hm5040_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_hm5040_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_hm5040_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_hm5040_mipi_raw_info.reset_pulse_level;
	//uint32_t reset_width=g_hm5040_yuv_info.reset_pulse_width;

	if (SENSOR_TRUE == power_on) {
		Sensor_PowerDown(power_down);
		usleep(20*1000);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		usleep(30*1000);
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		usleep(30*1000);
		_dw9174_SRCInit(2);
		Sensor_PowerDown(!power_down);
		usleep(30*1000);

	} else {
		Sensor_PowerDown(power_down);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
	}
	SENSOR_PRINT("SENSOR_hm5040: _hm5040_Power_On(1:on, 0:off): %d", power_on);
	return SENSOR_SUCCESS;
}

LOCAL uint32_t _hm5040_cfg_otp(uint32_t  param)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_hm5040_raw_param_tab;
	uint32_t module_id=g_module_id;

	SENSOR_PRINT("SENSOR_hm5040: _hm5040_cfg_otp");

	if(PNULL!=tab_ptr[module_id].cfg_otp){
		tab_ptr[module_id].cfg_otp(0);
		}

	return rtn;
}

LOCAL uint32_t _hm5040_com_Identify_otp(void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_hm5040: _hm5040_com_Identify_otp");

	/*read param id from sensor omap*/
	param_id=hm5040_RAW_PARAM_COM;

	if(hm5040_RAW_PARAM_COM==param_id){
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}

LOCAL uint32_t _hm5040_GetRawInof(void)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_hm5040_raw_param_tab;
	uint32_t param_id;
	uint32_t i=0x00;

	/*read param id from sensor omap*/
	param_id=hm5040_RAW_PARAM_COM;

	for(i=0x00; ; i++)
	{
		g_module_id = i;
		if(RAW_INFO_END_ID==tab_ptr[i].param_id){
			if(NULL==s_hm5040_mipi_raw_info_ptr){
				SENSOR_PRINT("SENSOR_hm5040: hm5040_GetRawInof no param error");
				rtn=SENSOR_FAIL;
			}
			SENSOR_PRINT("SENSOR_hm5040: hm5040_GetRawInof end");
			break;
		}
		else if(PNULL!=tab_ptr[i].identify_otp){
			if(SENSOR_SUCCESS==tab_ptr[i].identify_otp(0))
			{
				s_hm5040_mipi_raw_info_ptr = tab_ptr[i].info_ptr;
				SENSOR_PRINT("SENSOR_hm5040: hm5040_GetRawInof success");
				break;
			}
		}
	}

	return rtn;
}

LOCAL uint32_t _hm5040_GetMaxFrameLine(uint32_t index)
{
	uint32_t max_line=0x00;
	SENSOR_TRIM_T_PTR trim_ptr=s_hm5040_Resolution_Trim_Tab;

	max_line=trim_ptr[index].frame_line;

	return max_line;
}

LOCAL uint32_t _hm5040_Identify(uint32_t param)
{
#define hm5040_PID_VALUE    0x03
#define hm5040_PID_ADDR     0x2016
#define hm5040_VER_VALUE    0xbb
#define hm5040_VER_ADDR     0x2017
	uint8_t i=0;
	uint8_t pid_value = 0x00;
	uint8_t ver_value = 0x00;
	uint8_t value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("SENSOR_hm5040: mipi raw identify\n");

	pid_value = Sensor_ReadReg(hm5040_PID_ADDR);
	SENSOR_PRINT("SENSOR_hm5040: Identify: PID = %x", pid_value);
	ver_value = Sensor_ReadReg(hm5040_VER_ADDR);
	SENSOR_PRINT("SENSOR_hm5040: Identify: VER = %x", ver_value);

	for(i = 0; i<3; ) {
		if (hm5040_PID_VALUE == pid_value) {
			ver_value = Sensor_ReadReg(hm5040_VER_ADDR);
			SENSOR_PRINT("SENSOR_hm5040: Identify: PID = %x, VER = %x", pid_value, ver_value);
			if (hm5040_VER_VALUE == ver_value) {
				SENSOR_PRINT("SENSOR_hm5040: this is hm5040 sensor !");
				ret_value=_hm5040_GetRawInof();
				if(SENSOR_SUCCESS != ret_value)
				{
					SENSOR_PRINT("SENSOR_hm5040: the module is unknow error !");
				}
				Sensor_hm5040_InitRawTuneInfo();
			} else {
				SENSOR_PRINT("SENSOR_hm5040: Identify this is hm%x%x sensor !", pid_value, ver_value);
				return ret_value;
			}
		} else {
			SENSOR_PRINT("SENSOR_hm5040: identify fail,pid_value=%d", pid_value);
		}
		i++;
	}
	return ret_value;
}



LOCAL uint32_t _hm5040_write_exposure(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t expsure_line=0x00;
	uint16_t dummy_line=0x00;
	uint16_t size_index=0x00;
	uint16_t frame_len=0x00;
	uint16_t frame_len_cur=0x00;
	uint16_t max_frame_len=0x00;
	uint16_t value=0x00;
	uint16_t value0=0x00;
	uint16_t value1=0x00;
	uint16_t value2=0x00;

	expsure_line=param&0xffff;
	dummy_line=(param>>0x10)&0x0fff;
	size_index=(param>>0x1c)&0x0f;
	SENSOR_PRINT("SENSOR_hm5040: write_exposure line:%d, dummy:%d, size_index:%d", expsure_line, dummy_line, size_index);

	if(expsure_line < 3){
		expsure_line = 3;
	}
	max_frame_len=_hm5040_GetMaxFrameLine(size_index);

	frame_len = expsure_line + 36;
	frame_len = (frame_len > max_frame_len) ? frame_len : max_frame_len;

	value = Sensor_ReadReg(0x0341);
	frame_len_cur = value&0xff;
	value = Sensor_ReadReg(0x0340);
	frame_len_cur |= (value<<0x08)&0xff00;

	SENSOR_PRINT("SENSOR_hm5040: write_exposure line:%d, frame_len_cur:%d, frame_len:%d", expsure_line, frame_len_cur, frame_len);

	ret_value = Sensor_WriteReg(0x104, 0x01);

	if(frame_len_cur != frame_len){
		value = frame_len & 0xff;
		ret_value = Sensor_WriteReg(0x0341, value);
		value = (frame_len >> 0x08) & 0xff;
		ret_value = Sensor_WriteReg(0x0340, value);
	}
	value = expsure_line & 0xff;
	ret_value = Sensor_WriteReg(0x203, value);
	value = (expsure_line >> 0x08) & 0xff;
	ret_value = Sensor_WriteReg(0x202, value);
	ret_value = Sensor_WriteReg(0x104, 0x00);

	return ret_value;
}



LOCAL uint32_t _hm5040_write_gain(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint16_t real_gain = 0x00;
	uint16_t gain_tmp0 = 0,gain_tmp1=0,Reg_Cgain=0,Reg_Fgain=0;
	SENSOR_PRINT("SENSOR_hm5040gainparam =%x:", param );
	real_gain = ((param&0xf)+16)*(((param>>4)&0x01)+1)*(((param>>5)&0x01)+1)*(((param>>6)&0x01)+1)*(((param>>7)&0x01)+1);
	real_gain = real_gain*(((param>>8)&0x01)+1)*(((param>>9)&0x01)+1)*(((param>>10)&0x01)+1)*(((param>>11)&0x01)+1);
	SENSOR_PRINT("SENSOR_hm5040realgain =%x:", real_gain );
	gain_tmp0=(real_gain / 16);
	gain_tmp1=(real_gain  % 16);
	if(gain_tmp0 < 2)
	{
		Reg_Cgain=0;
		Reg_Fgain=real_gain<<4;
	}
	else if(gain_tmp0 < 4)
	{
		Reg_Cgain=0x80;
		Reg_Fgain=(real_gain>>1)<<4;
	}
	else if(gain_tmp0 < 8)
	{
		Reg_Cgain=0xc0;
		Reg_Fgain=(real_gain>>2)<<4;
	}
	else if(gain_tmp0 < 16)
	{
		Reg_Cgain=0xe0;
		Reg_Fgain=(real_gain>>3)<<4;
	}else{
		Reg_Cgain=0xf0;
		Reg_Fgain=(real_gain>>4)<<4;
	}

	ret_value = Sensor_WriteReg(0x104, 0x01);
	Reg_Cgain = Reg_Cgain & 0xFF;
	Reg_Fgain = Reg_Fgain ;
//	Reg_Fgain = 0x1f0 ;
	Sensor_WriteReg(0x0205,Reg_Cgain);
	Sensor_WriteReg(0x0204,Reg_Cgain>>8);

	Sensor_WriteReg(0x020f,Reg_Fgain&0xff);
	Sensor_WriteReg(0x020e,Reg_Fgain>>8);
	Sensor_WriteReg(0x0211,Reg_Fgain&0xff);
	Sensor_WriteReg(0x0210,Reg_Fgain>>8);
	Sensor_WriteReg(0x0213,Reg_Fgain&0xff);
	Sensor_WriteReg(0x0212,Reg_Fgain>>8);
	Sensor_WriteReg(0x0215,Reg_Fgain&0xff);
	Sensor_WriteReg(0x0214,Reg_Fgain>>8);

	ret_value = Sensor_WriteReg(0x104, 0x00);

	SENSOR_PRINT("SENSOR_hm5040writegain: 0x0205:%x,0x204:%x, 0x020f:%x,0x20e:%x", Reg_Cgain,Reg_Cgain>>8,Reg_Fgain,Reg_Fgain>>8 );
	return ret_value;
}

LOCAL uint32_t _hm5040_write_af(uint32_t param)
{
	#define DW9714_VCM_SLAVE_ADDR (0x18>>1)
	uint32_t ret_value = SENSOR_SUCCESS;
	uint8_t cmd_val[2] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;
	SENSOR_PRINT("SENSOR_hm5040: _write_af %d", param);
	slave_addr = DW9714_VCM_SLAVE_ADDR;
	cmd_val[0] = (param&0xfff0)>>4;
	cmd_val[1] = ((param&0x0f)<<4)|0x09;
	cmd_len = 2;
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
	SENSOR_PRINT("SENSOR_hm5040: _write_af, ret =  %d, MSL:%x, LSL:%x\n", ret_value, cmd_val[0], cmd_val[1]);
	return ret_value;
}
LOCAL uint32_t _hm5040_BeforeSnapshot(uint32_t param)
{
	uint8_t ret_l, ret_m, ret_h;
	uint32_t capture_exposure = 0, preview_maxline;
	uint32_t capture_maxline, preview_exposure;
	uint32_t capture_mode = param & 0xffff;
	uint32_t preview_mode = (param >> 0x10 ) & 0xffff;
	uint32_t prv_linetime=s_hm5040_Resolution_Trim_Tab[preview_mode].line_time;
	uint32_t cap_linetime = s_hm5040_Resolution_Trim_Tab[capture_mode].line_time;
	uint32_t frame_len = 0x00;

	param = param & 0xffff;
	cap_linetime = s_hm5040_Resolution_Trim_Tab[param].line_time;
	SENSOR_PRINT("SENSOR_hm5040: BeforeSnapshot moe: %d",param);

	if (SENSOR_MODE_PREVIEW_ONE >= param){
		SENSOR_PRINT("SENSOR_hm5040: prvmode equal to capmode");
		return SENSOR_SUCCESS;
	}
	preview_exposure = _hm5040_get_shutter();
	Sensor_SetMode(param);
	Sensor_SetMode_WaitDone();

	if (prv_linetime == cap_linetime) {
		SENSOR_PRINT("SENSOR_hm5040: prvline equal to capline");
		Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, preview_exposure);
		return SENSOR_SUCCESS;
	}
	capture_exposure = preview_exposure * prv_linetime/cap_linetime;

	frame_len = Sensor_ReadReg(0x0341)&0xff;
	frame_len |= (Sensor_ReadReg(0x0340)<<0x08)&0xff00;

	Sensor_WriteReg(0x104, 0x01);
	if(capture_exposure >= (frame_len - 36)){
		frame_len = capture_exposure+36;
		Sensor_WriteReg(0x0341, frame_len & 0xff);
		Sensor_WriteReg(0x0340, (frame_len >> 0x08) & 0xff);
	}
	Sensor_WriteReg(0x203, capture_exposure & 0xff);
	Sensor_WriteReg(0x202, (capture_exposure >> 0x08) & 0xff);
	Sensor_WriteReg(0x104, 0x00);

	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, capture_exposure);

	return SENSOR_SUCCESS;


}

LOCAL uint32_t _hm5040_after_snapshot(uint32_t param)
{
	SENSOR_PRINT("SENSOR_hm5040: after_snapshot mode:%d", param);
	Sensor_SetMode(param);
	return SENSOR_SUCCESS;
}

LOCAL uint32_t _hm5040_flash(uint32_t param)
{
	SENSOR_PRINT("SENSOR_hm5040: param=%d", param);

	/* enable flash, disable in _hm5040_BeforeSnapshot */
	g_flash_mode_en = param;
	Sensor_SetFlash(param);
	SENSOR_PRINT_HIGH("end");
	return SENSOR_SUCCESS;
}

LOCAL uint32_t _hm5040_StreamOn(uint32_t param)
{
	uint8_t value = 0x00;

	SENSOR_PRINT("SENSOR_hm5040: StreamOn");

	Sensor_WriteReg(0x0100, 0x01);
	usleep(100*1000);

	return 0;
}

LOCAL uint32_t _hm5040_StreamOff(uint32_t param)
{
	SENSOR_PRINT("SENSOR_hm5040: StreamOff");

	Sensor_WriteReg(0x0100, 0x00);
	usleep(100*1000);

	return 0;
}

int _hm5040_set_shutter(int shutter)
{
	// write shutter, in number of line period
	if (shutter<0x09EC) {
		Sensor_WriteReg(0x0202, (shutter >> 8) & 0xFF);
		Sensor_WriteReg(0x0203,  shutter  & 0xFF);
		Sensor_WriteReg(0x0340,  0x09);
		Sensor_WriteReg(0x0341,  0xEC);
	} else {
		Sensor_WriteReg(0x0202, (shutter >> 8) & 0xFF);
		Sensor_WriteReg(0x0203,  shutter  & 0xFF);
		Sensor_WriteReg(0x0340,  (shutter >> 8) & 0xFF);
		Sensor_WriteReg(0x0341,  shutter  & 0xFF);
	}

	return 0;
}

int _hm5040_get_gain16(void)
{
	// read gain, 16 = 1x
	int gain16;

	gain16 = Sensor_ReadReg(0x0204) & 0x03;
	gain16 = (gain16<<8) + Sensor_ReadReg(0x0205);

	return gain16;
}

int _hm5040_set_gain16(int gain16)
{
	// write gain, 16 = 1x
	int temp;
	gain16 = gain16 & 0x3ff;

	temp = gain16 & 0xff;
	Sensor_WriteReg(0x0205, temp);
	temp = gain16>>8;
	Sensor_WriteReg(0x0204, temp);

	return 0;
}

static void _calculate_hdr_exposure(int capture_gain16,int capture_VTS, int capture_shutter)
{
	// write capture gain
	_hm5040_set_gain16(capture_gain16);
	_hm5040_set_shutter(capture_shutter);
}

static uint32_t _hm5040_SetEV(uint32_t param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	uint16_t value=0x00;
	uint32_t gain = s_hm5040_gain;
	uint32_t ev = ext_ptr->param;

	SENSOR_PRINT("SENSOR_hm5040: _hm5040_SetEV param: 0x%x", ext_ptr->param);

	switch(ev) {
	case SENSOR_HDR_EV_LEVE_0:
		_calculate_hdr_exposure(s_hm5040_gain/2,s_capture_VTS,s_capture_shutter);
		break;
	case SENSOR_HDR_EV_LEVE_1:
		_calculate_hdr_exposure(s_hm5040_gain,s_capture_VTS,s_capture_shutter);
		break;
	case SENSOR_HDR_EV_LEVE_2:
		_calculate_hdr_exposure(s_hm5040_gain,s_capture_VTS,s_capture_shutter *4);
		break;
	default:
		break;
	}
	return rtn;
}

LOCAL uint32_t _hm5040_ExtFunc(uint32_t ctl_param)
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
		rtn = _hm5040_SetEV(ctl_param);
		break;
	default:
		break;
	}
	return rtn;
}

LOCAL int _hm5040_get_VTS(void)
{
	// read VTS from register settings
	int VTS;

	VTS = Sensor_ReadReg(0x0340);//total vertical size[15:8] high byte

	VTS = (VTS<<8) + Sensor_ReadReg(0x0341);

	return VTS;
}

LOCAL int _hm5040_set_VTS(int VTS)
{
	// write VTS to registers
	int temp;

	temp = VTS & 0xff;
	Sensor_WriteReg(0x0341, temp);
	temp = VTS>>8;
	Sensor_WriteReg(0x0340, temp);

	return 0;
}

LOCAL uint32_t _hm5040_ReadGain(uint32_t param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint32_t gain = 0;

	value = Sensor_ReadReg(0x0205);/*0-7*/
	gain = value&0xff;
	value = Sensor_ReadReg(0x0204);/*8*/
	gain |= (value<<0x08)&0x300;

	s_hm5040_gain=(int)gain;

	SENSOR_PRINT("SENSOR_hm5040: _hm5040_ReadGain gain: 0x%x", s_hm5040_gain);

	return rtn;
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
			cmd_val[2] = 0xf2;
			cmd_val[3] = 0x00;
			cmd_val[4] = 0xdc;
			cmd_val[5] = 0x51;
			cmd_len = 6;
			Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
		}
		break;

		case 3:
		break;

	}

	return ret_value;
}
