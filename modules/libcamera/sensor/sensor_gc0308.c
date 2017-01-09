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

#define GC0308_I2C_ADDR_W 0x21
#define GC0308_I2C_ADDR_R 0x21
#define SENSOR_GAIN_SCALE 16

 typedef enum
{
	FLICKER_50HZ = 0,
	FLICKER_60HZ,
	FLICKER_MAX
}FLICKER_E;

static uint32_t set_GC0308_ae_enable(uint32_t enable);
static uint32_t GC0308_PowerOn(uint32_t power_on);
static uint32_t set_preview_mode(uint32_t preview_mode);
static uint32_t GC0308_Identify(uint32_t param);
static uint32_t GC0308_BeforeSnapshot(uint32_t param);
static uint32_t GC0308_After_Snapshot(uint32_t param);
static uint32_t set_brightness(uint32_t level);
static uint32_t set_contrast(uint32_t level);
static uint32_t set_sharpness(uint32_t level);
static uint32_t set_saturation(uint32_t level);
static uint32_t set_image_effect(uint32_t effect_type);
static uint32_t read_ev_value(uint32_t value);
static uint32_t write_ev_value(uint32_t exposure_value);
static uint32_t read_gain_value(uint32_t value);
static uint32_t write_gain_value(uint32_t gain_value);
static uint32_t read_gain_scale(uint32_t value);
static uint32_t set_frame_rate(uint32_t param);
static uint32_t set_GC0308_ev(uint32_t level);
static uint32_t set_GC0308_awb(uint32_t mode);
static uint32_t set_GC0308_anti_flicker(uint32_t mode);
static uint32_t set_GC0308_video_mode(uint32_t mode);
static void GC0308_set_shutter();
static uint32_t set_sensor_flip(uint32_t param);

static SENSOR_REG_T GC0308_YUV_COMMON[]=
{
       {0xfe , 0x80},

	{0xfe , 0x00},   // set page0

	{0xd2 , 0x10},   // close AEC
	{0x22 , 0x55},   // close AWB

	{0x03 , 0x01},
	{0x04 , 0x2c},
	{0x5a , 0x56},
	{0x5b , 0x40},
	{0x5c , 0x4a},

	{0x22 , 0x57},   // Open AWB

	{0x01 , 0x2c},
	{0x02 , 0x58},
	{0x0f , 0x02},


	{0xe2 , 0x00},   //anti-flicker step [11:8]
	{0xe3 , 0x60},   //anti-flicker step [7:0]

	{0xe4 , 0x02},   //exp level 1  16.67fps
	{0xe5 , 0x40},
	{0xe6 , 0x03},   //exp level 2  12.5fps
	{0xe7 , 0xc0},
	{0xe8 , 0x04},   //exp level 3  8.33fps
	{0xe9 , 0x80},
	{0xea , 0x06},   //exp level 4  4.00fps
	{0xeb , 0x00},

	//{0xec , 0x20},

	{0x05 , 0x00},
	{0x06 , 0x00},
	{0x07 , 0x00},
	{0x08 , 0x00},
	{0x09 , 0x01},
	{0x0a , 0xe8},
	{0x0b , 0x02},
	{0x0c , 0x88},
	{0x0d , 0x02},
	{0x0e , 0x02},
	{0x10 , 0x22},
	{0x11 , 0xfd},
	{0x12 , 0x2a},
	{0x13 , 0x00},
	//{0x14 , 0x10},
	{0x15 , 0x0a},
	{0x16 , 0x05},
	{0x17 , 0x01},
	{0x18 , 0x44},
	{0x19 , 0x44},
	{0x1a , 0x1e},
	{0x1b , 0x00},
	{0x1c , 0xc1},
	{0x1d , 0x08},
	{0x1e , 0x60},
	{0x1f , 0x16},


	{0x20 , 0xff},
	{0x21 , 0xf8},
	{0x22 , 0x57},
	{0x24 , 0xa2},
	{0x25 , 0x0f},

	//output sync_mode
	{0x26 , 0x03},
	{0x2f , 0x01},
	{0x30 , 0xf7},
	{0x31 , 0x50},
	{0x32 , 0x00},
	{0x39 , 0x04},
	{0x3a , 0x18},
	{0x3b , 0x20},
	{0x3c , 0x00},
	{0x3d , 0x00},
	{0x3e , 0x00},
	{0x3f , 0x00},
	{0x50 , 0x10},
	{0x53 , 0x82},
	{0x54 , 0x80},
	{0x55 , 0x80},
	{0x56 , 0x82},
	{0x8b , 0x40},
	{0x8c , 0x40},
	{0x8d , 0x40},
	{0x8e , 0x2e},
	{0x8f , 0x2e},
	{0x90 , 0x2e},
	{0x91 , 0x3c},
	{0x92 , 0x50},
	{0x5d , 0x12},
	{0x5e , 0x1a},
	{0x5f , 0x24},
	{0x60 , 0x07},
	{0x61 , 0x15},
	{0x62 , 0x0f},
	{0x64 , 0x02},
	{0x66 , 0xe8},
	{0x67 , 0x86},
	{0x68 , 0xa2},
	{0x69 , 0x18},
	{0x6a , 0x0f},
	{0x6b , 0x00},
	{0x6c , 0x5f},
	{0x6d , 0x8f},
	{0x6e , 0x55},
	{0x6f , 0x38},
	{0x70 , 0x15},
	{0x71 , 0x33},
	{0x72 , 0xdc},
	{0x73 , 0x80},
	{0x74 , 0x02},
	{0x75 , 0x3f},
	{0x76 , 0x02},
	{0x77 , 0x33},
	{0x78 , 0x88},
	{0x79 , 0x81},
	{0x7a , 0x81},
	{0x7b , 0x22},
	{0x7c , 0xff},
	{0x93 , 0x48},
	{0x94 , 0x00},
	{0x95 , 0x05},
	{0x96 , 0xe8},
	{0x97 , 0x40},
	{0x98 , 0xf0},
	{0xb1 , 0x38},
	{0xb2 , 0x38},
	{0xbd , 0x38},
	{0xbe , 0x36},
	{0xd0 , 0xc9},
	{0xd1 , 0x10},
	//{0xd2 , 0x90},
	{0xd3 , 0x80},
	{0xd5 , 0xf2},
	{0xd6 , 0x16},
	{0xdb , 0x92},
	{0xdc , 0xa5},
	{0xdf , 0x23},
	{0xd9 , 0x00},
	{0xda , 0x00},
	{0xe0 , 0x09},

	{0xed , 0x04},
	{0xee , 0xa0},
	{0xef , 0x40},
	{0x80 , 0x03},
	{0x80 , 0x03},
	{0x9F , 0x10},
	{0xA0 , 0x20},
	{0xA1 , 0x38},
	{0xA2 , 0x4E},
	{0xA3 , 0x63},
	{0xA4 , 0x76},
	{0xA5 , 0x87},
	{0xA6 , 0xA2},
	{0xA7 , 0xB8},
	{0xA8 , 0xCA},
	{0xA9 , 0xD8},
	{0xAA , 0xE3},
	{0xAB , 0xEB},
	{0xAC , 0xF0},
	{0xAD , 0xF8},
	{0xAE , 0xFD},
	{0xAF , 0xFF},
	{0xc0 , 0x00},
	{0xc1 , 0x10},
	{0xc2 , 0x1C},
	{0xc3 , 0x30},
	{0xc4 , 0x43},
	{0xc5 , 0x54},
	{0xc6 , 0x65},
	{0xc7 , 0x75},
	{0xc8 , 0x93},
	{0xc9 , 0xB0},
	{0xca , 0xCB},
	{0xcb , 0xE6},
	{0xcc , 0xFF},
	{0xf0 , 0x02},
	{0xf1 , 0x01},
	{0xf2 , 0x01},
	{0xf3 , 0x30},
	{0xf9 , 0x9f},
	{0xfa , 0x78},

	//---------------------------------------------------------------
	{0xfe , 0x01},// set page1

	{0x00 , 0xf5},
	{0x02 , 0x1a},
	{0x0a , 0xa0},
	{0x0b , 0x60},
	{0x0c , 0x08},
	{0x0e , 0x4c},
	{0x0f , 0x39},
	{0x11 , 0x3f},
	{0x12 , 0x72},
	{0x13 , 0x13},
	{0x14 , 0x42},
	{0x15 , 0x43},
	{0x16 , 0xc2},
	{0x17 , 0xa8},
	{0x18 , 0x18},
	{0x19 , 0x40},
	{0x1a , 0xd0},
	{0x1b , 0xf5},
	{0x70 , 0x40},
	{0x71 , 0x58},
	{0x72 , 0x30},
	{0x73 , 0x48},
	{0x74 , 0x20},
	{0x75 , 0x60},
	{0x77 , 0x20},
	{0x78 , 0x32},
	{0x30 , 0x03},
	{0x31 , 0x40},
	{0x32 , 0xe0},
	{0x33 , 0xe0},
	{0x34 , 0xe0},
	{0x35 , 0xb0},
	{0x36 , 0xc0},
	{0x37 , 0xc0},
	{0x38 , 0x04},
	{0x39 , 0x09},
	{0x3a , 0x12},
	{0x3b , 0x1C},
	{0x3c , 0x28},
	{0x3d , 0x31},
	{0x3e , 0x44},
	{0x3f , 0x57},
	{0x40 , 0x6C},
	{0x41 , 0x81},
	{0x42 , 0x94},
	{0x43 , 0xA7},
	{0x44 , 0xB8},
	{0x45 , 0xD6},
	{0x46 , 0xEE},
	{0x47 , 0x0d},

	{0xfe , 0x00}, // set page0
	{0xd2 , 0x90},


	//-----------Update the registers 2010/07/06-------------//
	//Registers of Page0
	{0xfe , 0x00}, // set page0
	{0x10 , 0x26},
	{0x11 , 0x0d},  // fd,modified by mormo 2010/07/06
	{0x1a , 0x2a},  // 1e,modified by mormo 2010/07/06

	{0x1c , 0x49}, // c1,modified by mormo 2010/07/06
	{0x1d , 0x9a}, // 08,modified by mormo 2010/07/06
	{0x1e , 0x61}, // 60,modified by mormo 2010/07/06

	{0x3a , 0x20},

	{0x50 , 0x24},  // 10,modified by mormo 2010/07/06
	{0x53 , 0x80},
	{0x56 , 0x80},

	{0x8b , 0x20}, //LSC
	{0x8c , 0x20},
	{0x8d , 0x20},
	{0x8e , 0x14},
	{0x8f , 0x10},
	{0x90 , 0x14},

	{0x94 , 0x02},
	{0x95 , 0x07},
	{0x96 , 0xe0},

	{0xb1 , 0x38}, // YCPT
	{0xb2 , 0x38},
	{0xb3 , 0x3c},
	{0xb6 , 0xe0},

	{0xd0 , 0xc9}, // AECT  c9,modifed by mormo 2010/07/06
	{0xd3 , 0x80}, // 80,modified by mormor 2010/07/06

	{0xf2 , 0x02},
	{0xf7 , 0x12},
	{0xf8 , 0x0a},

	//Registers of Page1
	{0xfe , 0x01},// set page1
	{0x02 , 0x20},
	{0x04 , 0x10},
	{0x05 , 0x08},
	{0x06 , 0x20},
	{0x08 , 0x0a},

	{0x0e , 0x44},
	{0x0f , 0x32},
	{0x10 , 0x41},
	{0x11 , 0x37},
	{0x12 , 0x22},
	{0x13 , 0x19},
	{0x14 , 0x44},
	{0x15 , 0x44},

	{0x19 , 0x50},
	{0x1a , 0xd8},

	{0x32 , 0x10},

	{0x35 , 0x00},
	{0x36 , 0x80},
	{0x37 , 0x00},
	//-----------Update the registers end---------//
	{0xfe , 0x00},// set page0

	//-----------GAMMA Select(2)---------------//
			{0x9F , 0x0E},
			{0xA0 , 0x1C},
			{0xA1 , 0x34},
			{0xA2 , 0x48},
			{0xA3 , 0x5A},
			{0xA4 , 0x6B},
			{0xA5 , 0x7B},
			{0xA6 , 0x95},
			{0xA7 , 0xAB},
			{0xA8 , 0xBF},
			{0xA9 , 0xCE},
			{0xAA , 0xD9},
			{0xAB , 0xE4},
			{0xAC , 0xEC},
			{0xAD , 0xF7},
			{0xAE , 0xFD},
			{0xAF , 0xFF},

	 /*GC0308_GAMMA_Select,
		1:                                             //smallest gamma curve
			{0x9F , 0x0B},
			{0xA0 , 0x16},
			{0xA1 , 0x29},
			{0xA2 , 0x3C},
			{0xA3 , 0x4F},
			{0xA4 , 0x5F},
			{0xA5 , 0x6F},
			{0xA6 , 0x8A},
			{0xA7 , 0x9F},
			{0xA8 , 0xB4},
			{0xA9 , 0xC6},
			{0xAA , 0xD3},
			{0xAB , 0xDD},
			{0xAC , 0xE5},
			{0xAD , 0xF1},
			{0xAE , 0xFA},
			{0xAF , 0xFF},

		2:
			{0x9F , 0x0E},
			{0xA0 , 0x1C},
			{0xA1 , 0x34},
			{0xA2 , 0x48},
			{0xA3 , 0x5A},
			{0xA4 , 0x6B},
			{0xA5 , 0x7B},
			{0xA6 , 0x95},
			{0xA7 , 0xAB},
			{0xA8 , 0xBF},
			{0xA9 , 0xCE},
			{0xAA , 0xD9},
			{0xAB , 0xE4},
			{0xAC , 0xEC},
			{0xAD , 0xF7},
			{0xAE , 0xFD},
			{0xAF , 0xFF},

		3:
			{0x9F , 0x10},
			{0xA0 , 0x20},
			{0xA1 , 0x38},
			{0xA2 , 0x4E},
			{0xA3 , 0x63},
			{0xA4 , 0x76},
			{0xA5 , 0x87},
			{0xA6 , 0xA2},
			{0xA7 , 0xB8},
			{0xA8 , 0xCA},
			{0xA9 , 0xD8},
			{0xAA , 0xE3},
			{0xAB , 0xEB},
			{0xAC , 0xF0},
			{0xAD , 0xF8},
			{0xAE , 0xFD},
			{0xAF , 0xFF},

		4:
			{0x9F , 0x14},
			{0xA0 , 0x28},
			{0xA1 , 0x44},
			{0xA2 , 0x5D},
			{0xA3 , 0x72},
			{0xA4 , 0x86},
			{0xA5 , 0x95},
			{0xA6 , 0xB1},
			{0xA7 , 0xC6},
			{0xA8 , 0xD5},
			{0xA9 , 0xE1},
			{0xAA , 0xEA},
			{0xAB , 0xF1},
			{0xAC , 0xF5},
			{0xAD , 0xFB},
			{0xAE , 0xFE},
			{0xAF , 0xFF},

		5:								//largest gamma curve
			{0x9F , 0x15},
			{0xA0 , 0x2A},
			{0xA1 , 0x4A},
			{0xA2 , 0x67},
			{0xA3 , 0x79},
			{0xA4 , 0x8C},
			{0xA5 , 0x9A},
			{0xA6 , 0xB3},
			{0xA7 , 0xC5},
			{0xA8 , 0xD5},
			{0xA9 , 0xDF},
			{0xAA , 0xE8},
			{0xAB , 0xEE},
			{0xAC , 0xF3},
			{0xAD , 0xFA},
			{0xAE , 0xFD},
			{0xAF , 0xFF}, */
	//-----------GAMMA Select End--------------//




	//-------------H_V_Switch(4)---------------//
			{0x14 , 0x10},

	 /*GC0308_H_V_Switch,

		1:  // normal
		{0x14 , 0x10},

		2:  // IMAGE_H_MIRROR
		{0x14 , 0x11},

		3:  // IMAGE_V_MIRROR
		{0x14 , 0x12},

		4:  // IMAGE_HV_MIRROR
		{0x14 , 0x13},
	*/
	//-------------H_V_Select End--------------//


       {SENSOR_WRITE_DELAY, 200},//delay 200ms

	{0xff , 0xff},
};

static SENSOR_REG_TAB_INFO_T s_GC0308_resolution_Tab_YUV[]=
{
	// COMMON INIT
	{ADDR_AND_LEN_OF_ARRAY(GC0308_YUV_COMMON), 640, 480, 24, SENSOR_IMAGE_FORMAT_YUV422},

	// YUV422 PREVIEW 1
	{ADDR_AND_LEN_OF_ARRAY(GC0308_YUV_COMMON), 640, 480, 24, SENSOR_IMAGE_FORMAT_YUV422},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},

	// YUV422 PREVIEW 2
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

LOCAL SENSOR_VIDEO_INFO_T s_GC0308_video_info[] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}
};

static SENSOR_IOCTL_FUNC_TAB_T s_GC0308_ioctl_func_tab =
{
	// Internal
	PNULL,
	GC0308_PowerOn,
	PNULL,
	GC0308_Identify,

	PNULL, //write register
	PNULL, //read  register
	PNULL, //set_sensor_flip
	PNULL,

	// External
	PNULL,//set_GC0308_ae_enable,
	PNULL,
	PNULL,

	set_brightness,
	set_contrast,
	PNULL,//set_sharpness,
	PNULL,//set_saturation,

	PNULL,//set_preview_mode,
	set_image_effect,

	PNULL,//GC0308_BeforeSnapshot,
	PNULL,//GC0308_After_Snapshot,

	PNULL,

	PNULL,//read_ev_value,
	PNULL,//write_ev_value,
	PNULL,//read_gain_value,
	PNULL,//write_gain_value,
	PNULL,//read_gain_scale,
	PNULL,//set_frame_rate,
	PNULL,
	PNULL,
	set_GC0308_awb,
	PNULL,
	PNULL,
	set_GC0308_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	set_GC0308_anti_flicker,
	set_GC0308_video_mode,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
};

SENSOR_INFO_T g_GC0308_yuv_info =
{
	GC0308_I2C_ADDR_W,				//salve i2c write address
	GC0308_I2C_ADDR_R,				//salve i2c read address

	0,						//bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
							//bit2: 0: i2c register addr  is 8 bit, 1: i2c register addr  is 16 bit
							//other bit: reseved
	SENSOR_HW_SIGNAL_PCLK_N|\
	SENSOR_HW_SIGNAL_VSYNC_P|\
	SENSOR_HW_SIGNAL_HSYNC_P,			//bit0: 0:negative; 1:positive -> polarily of pixel clock
							//bit2: 0:negative; 1:positive -> polarily of horizontal synchronization signal
							//bit4: 0:negative; 1:positive -> polarily of vertical synchronization signal
							//other bit: reseved

	// preview mode
	SENSOR_ENVIROMENT_NORMAL|\
	SENSOR_ENVIROMENT_NIGHT|\
	SENSOR_ENVIROMENT_SUNNY,

	// image effect
	SENSOR_IMAGE_EFFECT_NORMAL|\
	SENSOR_IMAGE_EFFECT_BLACKWHITE|\
	SENSOR_IMAGE_EFFECT_RED|\
	SENSOR_IMAGE_EFFECT_GREEN|\
	SENSOR_IMAGE_EFFECT_BLUE|\
	SENSOR_IMAGE_EFFECT_YELLOW|\
	SENSOR_IMAGE_EFFECT_NEGATIVE|\
	SENSOR_IMAGE_EFFECT_CANVAS,

	// while balance mode
	0,

	7,						//bit[0:7]: count of step in brightness, contrast, sharpness, saturation
							//bit[8:31] reseved

	SENSOR_LOW_PULSE_RESET,				//reset pulse level
	100,						//reset pulse width(ms)

	SENSOR_HIGH_LEVEL_PWDN,				//power donw pulse level

	2,						//count of identify code
	{{0x00, 0x9b},					//supply two code to identify sensor.
	{0x00, 0x9b}},					//for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,				//voltage of avdd

	640,						//max width of source image
	480,						//max height of source image
	"GC0308",					//name of sensor

	SENSOR_IMAGE_FORMAT_YUV422,			//define in SENSOR_IMAGE_FORMAT_E enum,
							//if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T
	SENSOR_IMAGE_PATTERN_YUV422_YUYV,		//pattern of input image form sensor;

	s_GC0308_resolution_Tab_YUV,			//point to resolution table information structure
	&s_GC0308_ioctl_func_tab,				//point to ioctl function table

	PNULL,						//information and table about Rawrgb sensor
	PNULL,						//extend information about sensor
	SENSOR_AVDD_2800MV,				//iovdd
	SENSOR_AVDD_1800MV,				//dvdd
	4,						//skip frame num before preview
	3,						//skip frame num before capture
	0,						//deci frame num during preview
	0,						//deci frame num during video preview
	0,						//threshold enable(only analog TV)
	0,						//atv output mode 0 fix mode 1 auto mode
	0,						//atv output start postion
	0,						//atv output end postion
	0,
	{SENSOR_INTERFACE_TYPE_CCIR601, 8, 16, 1},
	s_GC0308_video_info,
	4,						//skip frame num while change setting
};

static void GC0308_WriteReg( uint8_t  subaddr, uint8_t data )
{
	Sensor_WriteReg(subaddr, data);
}

static uint8_t GC0308_ReadReg( uint8_t subaddr)
{
	uint8_t value = 0;

	value = Sensor_ReadReg( subaddr);

	return value;
}

static uint32_t GC0308_PowerOn(uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_GC0308_yuv_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_GC0308_yuv_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_GC0308_yuv_info.iovdd_val;
	BOOLEAN power_down = g_GC0308_yuv_info.power_down_level;
	BOOLEAN reset_level = g_GC0308_yuv_info.reset_pulse_level;

	if (SENSOR_TRUE == power_on) {
		Sensor_PowerDown(power_down);
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		usleep(10*1000);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(10*1000);
		Sensor_PowerDown(!power_down);
		Sensor_Reset(reset_level);
	} else {
		Sensor_PowerDown(power_down);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED,
				SENSOR_AVDD_CLOSED);
	}
	SENSOR_PRINT("(1:on, 0:off): %d", power_on);
	return (uint32_t)SENSOR_SUCCESS;
}

static uint32_t GC0308_Identify(uint32_t param)
{
#define GC0308_PID_VALUE	0x9b
#define GC0308_PID_ADDR	0x00
#define GC0308_VER_VALUE	0x9b
#define GC0308_VER_ADDR	0x00

	uint32_t i;
	uint32_t nLoop;
	uint8_t ret;
	uint32_t err_cnt = 0;
	uint8_t reg[2] 	= {0x00, 0x00};
	uint8_t value[2] 	= {0x9b, 0x9b};

	SENSOR_TRACE("GC0308_Identify");
	for(i = 0; i<2; ) {
		nLoop = 1000;
		ret = GC0308_ReadReg(reg[i]);
		if( ret != value[i]) {
			err_cnt++;
			if(err_cnt>3) {
				SENSOR_PRINT_ERR("It is not GC0308\n");
				return SENSOR_FAIL;
			} else {
				while(nLoop--);
				continue;
			}
		}
		err_cnt = 0;
		i++;
	}

	SENSOR_PRINT_HIGH("GC0308_Identify: it is GC0308\n");
	return (uint32_t)SENSOR_SUCCESS;
}

static uint32_t set_GC0308_ae_enable(uint32_t enable)
{

	return 0;
}


static void GC0308_set_shutter()
{

}

static SENSOR_REG_T GC0308_brightness_tab[][4]=
{
	{
		{0xb5, 0xc0},{0xff,0xff},
	},

	{
		{0xb5, 0xd0},{0xff,0xff},
	},

	{
		{0xb5, 0xe0},{0xff,0xff},
	},

	{
		{0xb5, 0xf0},{0xff,0xff},
	},

	{
		{0xb5, 0x20},{0xff,0xff},
	},

	{
		{0xb5, 0x30},{0xff,0xff},
	},

	{
		{0xb5, 0x40},{0xff,0xff},
	},
};

static uint32_t set_brightness(uint32_t level)
{
	uint16_t i;
	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)GC0308_brightness_tab[level];

	if(level>6)
		return 0;

	for(i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) && (0xFF != sensor_reg_ptr[i].reg_value); i++) {
		GC0308_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}
	SENSOR_TRACE("set_brightness: level = %d\n", level);
	return 0;
}


static SENSOR_REG_T GC0308_ev_tab[][4]=
{
	{{0xd3, 0x68}, {0xff, 0xff}},
	{{0xd3, 0x70}, {0xff, 0xff}},
	{{0xd3, 0x78}, {0xff, 0xff}},
	{{0xd3, 0x80}, {0xff, 0xff}},
	{{0xd3, 0x88}, {0xff, 0xff}},
	{{0xd3, 0x90}, {0xff, 0xff}},
	{{0xd3, 0x98}, {0xff, 0xff}},
};

static uint32_t set_GC0308_ev(uint32_t level)
{
	uint16_t i;
	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)GC0308_ev_tab[level];

	if(level>6)
		return 0;

	for(i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) ||(0xFF != sensor_reg_ptr[i].reg_value) ; i++) {
		GC0308_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	SENSOR_TRACE("SENSOR: set_ev: level = %d\n", level);
	return 0;
}

static uint32_t set_GC0308_anti_flicker(uint32_t param )
{
	switch (param) {
	case FLICKER_50HZ:
		GC0308_WriteReg(0x01, 0x2c);
		GC0308_WriteReg(0x02, 0x58);
		GC0308_WriteReg(0x0f, 0x02);
		GC0308_WriteReg(0xe2, 0x00);	//anti-flicker step [11:8]
		GC0308_WriteReg(0xe3, 0x60);   //anti-flicker step [7:0]
		GC0308_WriteReg(0xe4, 0x03);   //exp level 1  16.67fps
		GC0308_WriteReg(0xe5, 0x00);
		GC0308_WriteReg(0xe6, 0x03);   //exp level 2  12.5fps
		GC0308_WriteReg(0xe7, 0xc0);
		GC0308_WriteReg(0xe8, 0x04);   //exp level 3  10fps
		GC0308_WriteReg(0xe9, 0x80);
		GC0308_WriteReg(0xea, 0x06);   //exp level 4  6.25fps
		GC0308_WriteReg(0xeb, 0x00);
	break;
	case FLICKER_60HZ:
		GC0308_WriteReg(0x01, 0x2c);
		GC0308_WriteReg(0x02, 0x98);
		GC0308_WriteReg(0x0f, 0x02);
		GC0308_WriteReg(0xe2, 0x00);	//anti-flicker step [11:8]
		GC0308_WriteReg(0xe3, 0x50);	//anti-flicker step [7:0]

		GC0308_WriteReg(0xe4, 0x02);	//exp level 1  15.00fps
		GC0308_WriteReg(0xe5, 0x80);
		GC0308_WriteReg(0xe6, 0x03);	//exp level 2  12fps
		GC0308_WriteReg(0xe7, 0x00);
		GC0308_WriteReg(0xe8, 0x04);	//exp level 3  10fps
		GC0308_WriteReg(0xe9, 0x60);
		GC0308_WriteReg(0xea, 0x05);	//exp level 4  7.5fps
		GC0308_WriteReg(0xeb, 0xf0);
	break;
	default:
	break;
	}
	return 0;
}

static uint32_t set_GC0308_video_mode(uint32_t mode)
{
	if(0 == mode)
		GC0308_WriteReg(0xec,0x20);
	else if(1 == mode)
		GC0308_WriteReg(0xec,0x00);
	SENSOR_TRACE("SENSOR: GC0308_ReadReg(0xec) = %x\n", GC0308_ReadReg(0xec));
	SENSOR_TRACE("SENSOR: set_video_mode: mode = %d\n", mode);
	return 0;
}

static SENSOR_REG_T GC0308_awb_tab[][6]=
{
	//AUTO
	{
	   	    {0x5a, 0x4c}, 
	   	    {0x5b, 0x40},
	   	    {0x5c, 0x4a},
                  {0x22, 0x57},    // the reg value is not written here, rewrite in set_GC0308_awb();
                  {0xff, 0xff}
		},    
		//INCANDESCENCE:
		{
		    {0x22, 0x55},    // Disable AWB 
		    {0x5a, 0x48},
		    {0x5b, 0x40},
		    {0x5c, 0x5c},
                  {0xff, 0xff} 
		},
		//U30
		{
		    {0x22, 0x55},   // Disable AWB 
		    {0x5a, 0x40},
		    {0x5b, 0x54},
		    {0x5c, 0x70},
		    {0xff, 0xff} 
		},  
		//CWF  //
		{
		    {0x22, 0x55},   // Disable AWB 
		    {0x5a, 0x40},
		    {0x5b, 0x54},
		    {0x5c, 0x70},
		    {0xff, 0xff} 
		},    
		//FLUORESCENT:
		{
		    {0x22, 0x55},   // Disable AWB 
                  {0x5a, 0x40},
                  {0x5b, 0x42}, 
                  {0x5c, 0x50},
		    {0xff, 0xff} 
		},
		//SUN:
		{
		    {0x22, 0x55},   // Disable AWB 
		    {0x5a, 0x50},
		    {0x5b, 0x45},
		    {0x5c, 0x40},
		    {0xff, 0xff} 
		},
                //CLOUD:
              {
                  {0x22, 0x55},   // Disable AWB 
		    {0x5a, 0x5a},
		    {0x5b, 0x42},
		    {0x5c, 0x40},
		    {0xff, 0xff} 
		},
};

static uint32_t set_GC0308_awb(uint32_t mode)
{
	uint8_t awb_en_value;
	uint16_t i;
	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)GC0308_awb_tab[mode];

	awb_en_value = GC0308_ReadReg(0x22);

	if(mode>6)
	return 0;

	for(i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) || (0xFF != sensor_reg_ptr[i].reg_value); i++) {
	if(0x22 == sensor_reg_ptr[i].reg_addr) {
	if(mode == 0)
		GC0308_WriteReg(0x22, awb_en_value |0x02 );
	else
		GC0308_WriteReg(0x22, awb_en_value &0xfd );
	} else {
		GC0308_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}
	}
	SENSOR_TRACE("SENSOR: set_awb_mode: mode = %d\n", mode);

	return 0;
}

static SENSOR_REG_T GC0308_contrast_tab[][4]=
{
	//level 0
	{
		{0xb3,0x50},{0xff,0xff},
	},
	//level 1
	{
		{0xb3,0x48},{0xff,0xff},
	},
	//level 2
	{
		{0xb3,0x44},{0xff,0xff},
	},
	//level 3
	{
		{0xb3,0x38},{0xff,0xff},
	},
	//level 4
	{
		{0xb3,0x3d},{0xff,0xff},
	},
	//level 5
	{
		{0xb3,0x38},{0xff,0xff},
	},
	//level 6
	{
		{0xb3,0x34},{0xff,0xff},
	},
};

static uint32_t set_contrast(uint32_t level)
{
	uint16_t i;
	SENSOR_REG_T* sensor_reg_ptr;

	sensor_reg_ptr = (SENSOR_REG_T*)GC0308_contrast_tab[level];

	if(level>6)
		return 0;

	for(i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) && (0xFF != sensor_reg_ptr[i].reg_value); i++) {
		GC0308_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	SENSOR_TRACE("set_contrast: level = %d\n", level);
	return 0;
}

static uint32_t set_sharpness(uint32_t level)
{
	return 0;
}

static uint32_t set_saturation(uint32_t level)
{
	return 0;
}

static uint32_t set_preview_mode(uint32_t preview_mode)
{
        SENSOR_TRACE("set_preview_mode: preview_mode = %d\n", preview_mode);

        set_GC0308_anti_flicker(0);
        switch (preview_mode) {
        case DCAMERA_ENVIRONMENT_NORMAL:
                GC0308_WriteReg(0xec,0x20);
                break;
        case DCAMERA_ENVIRONMENT_NIGHT:
                GC0308_WriteReg(0xec,0x30);
                break;
        case DCAMERA_ENVIRONMENT_SUNNY:
                GC0308_WriteReg(0xec,0x10);
                break;
        default:
                break;
        }
        SENSOR_Sleep(10);
        return 0;
}

static SENSOR_REG_T GC0308_image_effect_tab[][11] =
{
	// effect normal
	{
		{0x23,0x00}, {0x2d,0x0a}, {0x20,0x7f}, {0xd2,0x90}, {0x73,0x00}, {0x77,0x33},
		{0xb3,0x38}, {0xb4,0x80}, {0xba,0x00}, {0xbb,0x00}, {0xff,0xff}
	},
	//effect BLACKWHITE
	{
		{0x23,0x02}, {0x2d,0x0a}, {0x20,0x7f}, {0xd2,0x90}, {0x73,0x00},
		{0xb3,0x40},	{0xb4,0x80}, {0xba,0x00}, {0xbb,0x00}, {0xff,0xff}
	},
	// effect RED pink
	{
	//TODO: later work
		{0x23,0x02},{0x2d,0x0a},{0x20,0x7f},{0xd2,0x90},{0x77,0x88},
		{0xb3,0x40},{0xb4,0x80},{0xba,0x10},{0xbb,0x50},{0xff, 0xff}
	},
	// effect GREEN
	{
		{0x23,0x02},{0x2d,0x0a},{0x20,0x7f},{0xd2,0x90},{0x77,0x88},
		{0xb3,0x40},{0xb4,0x80},{0xba,0xc0},{0xbb,0xc0},{0xff, 0xff}
	},
	// effect  BLUE
	{
		{0x23,0x02},{0x2d,0x0a},{0x20,0x7f},{0xd2,0x90},{0x73,0x00},
		{0xb3,0x40},{0xb4,0x80},{0xba,0x50},{0xbb,0xe0},{0xff, 0xff}
	},
	// effect  YELLOW
	{
		//TODO:later work
		{0x23,0x02},{0x2d,0x0a},{0x20,0x7f},{0xd2,0x90},{0x77,0x88},
		{0xb3,0x40},{0xb4,0x80},{0xba,0x80},{0xbb,0x20},{0xff, 0xff}
	},
	// effect NEGATIVE
	{
		{0x23,0x01},{0x2d,0x0a},{0x20,0x7f},{0xd2,0x90},{0x73,0x00},
		{0xb3,0x40},{0xb4,0x80},{0xba,0x00},{0xbb,0x00},{0xff, 0xff}
	},
	//effect ANTIQUE
	{
		{0x23,0x02},{0x2d,0x0a},{0x20,0x7f},{0xd2,0x90},{0x73,0x00},
		{0xb3,0x40},{0xb4,0x80},{0xba,0xd0},{0xbb,0x28},{0xff, 0xff}
	},
};

static uint32_t set_image_effect(uint32_t effect_type)
{
	uint16_t i;
	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)GC0308_image_effect_tab[effect_type];
	if(effect_type>7)
		return 0;

	for(i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) || (0xFF != sensor_reg_ptr[i].reg_value) ; i++) {
		Sensor_WriteReg_8bits(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}
	SENSOR_TRACE("-----------set_image_effect: effect_type = %d------------\n", effect_type);
	return 0;
}

static uint32_t GC0308_After_Snapshot(uint32_t param)
{
	return 0;
}

static uint32_t GC0308_BeforeSnapshot(uint32_t sensor_snapshot_mode)
{
	return 0;
}

static uint32_t read_ev_value(uint32_t value)
{
	return 0;
}

static uint32_t write_ev_value(uint32_t exposure_value)
{
	return 0;
}

static uint32_t read_gain_value(uint32_t value)
{
	return 0;
}

static uint32_t write_gain_value(uint32_t gain_value)
{
	return 0;
}

static uint32_t read_gain_scale(uint32_t value)
{
	return SENSOR_GAIN_SCALE;
}


static uint32_t set_frame_rate(uint32_t param)
{
	return 0;
}
static uint32_t set_sensor_flip(uint32_t param)
{
	return 0;
}
