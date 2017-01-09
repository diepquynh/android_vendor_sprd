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
#include <utils/Timers.h>

#ifdef     __cplusplus
extern     "C"
{
#endif

#define SR030PC50_I2C_ADDR_W 0x30
#define SR030PC50_I2C_ADDR_R 0x30
#define SR030PC50_CHIP_ID_ADDR 0x04
#define SR030PC50_CHIP_ID_VALUE 0xB8

static EXIF_SPEC_PIC_TAKING_COND_T s_Sr030pc50_Exif;

static unsigned long Sr030pc50_GetExifInfo(unsigned long param);
static unsigned long Sr030pc50_GetResolutionTrimTab(unsigned long param);
static unsigned long Sr030pc50_PowerOn(unsigned long power_on);
static unsigned long Sr030pc50_Identify(unsigned long param);
static unsigned long Sr030pc50_StreamOn(unsigned long param);
static unsigned long Sr030pc50_StreamOff(unsigned long param);
static unsigned long Sr030pc50_Set_Brightness(unsigned long level);
static unsigned long Sr030pc50_Set_Saturation(unsigned long level);
static unsigned long Sr030pc50_Set_Image_Effect(unsigned long effect_type);
static unsigned long Sr030pc50_Set_Anti_Flicker(unsigned long mode);
static unsigned long Sr030pc50_Set_Video_Mode(unsigned long mode);
static unsigned long Sr030pc50_Set_Awb(unsigned long mode);
static unsigned long Sr030pc50_Set_Work_Mode(unsigned long mode);
static unsigned long Sr030pc50_BeforeSnapshot(unsigned long param);
static unsigned long Sr030pc50_AfterSnapshot(unsigned long param);


static const SENSOR_REG_T sr030pc50_init_regs[]={
{0x03,0x00},
{0x01,0x71},
{0x01,0x73},
{0x01,0x71},

{0x03,0x00},

{0x08,0x0F},
{0x10,0x00},
{0x11,0x91},
{0x12,0x00},
{0x14,0x88},

{0x0b,0xaa},
{0x0c,0xaa},
{0x0d,0xaa},

{0xC0,0x95},
{0xC1,0x18},
{0xC2,0x91},
{0xC3,0x00},
{0xC4,0x01},

{0x03,0x20},
{0x10,0x1C},
{0x03,0x22},
{0x10,0x7B},

{0x03,0x00},
{0x12,0x00},
{0x20,0x00},
{0x21,0x04},
{0x22,0x00},
{0x23,0x04},

{0x40,0x00},
{0x41,0x90},
{0x42,0x00},
{0x43,0x62}, //98 25.08fix

{0x80,0x2E},
{0x81,0x7E},
{0x82,0x90},
{0x83,0x30},
{0x84,0x2C},
{0x85,0x4B},
{0x86,0x01},
{0x88,0x47},
{0x90,0x0c},
{0x91,0x0c},
{0x92,0x90},
{0x93,0x88},
{0x98,0x38},
{0x99,0x40},
{0xA0,0x02},
{0xA8,0x42},

//Page2 Last Update 12_01_20
{0x03,0x02},
{0x10,0x00},
{0x11,0x00},
{0x13,0x40},
{0x14,0x04},
{0x18,0x1C},
{0x19,0x00},
{0x1A,0x00},
{0x1B,0x08},
{0x1C,0x9C},
{0x1D,0x03},
{0x20,0x33},
{0x21,0x77},
{0x22,0xA7},
{0x23,0x32},
{0x24,0x33},
{0x2B,0x40},
{0x2D,0x32},
{0x31,0x99},
{0x32,0x00},
{0x33,0x00},
{0x34,0x3C},
{0x35,0x0D},
{0x3B,0x60},

//timing control 1 // //don't touch
{0x50,0x21},
{0x51,0x1C},
{0x52,0xAA},
{0x53,0x5A},
{0x54,0x30},
{0x55,0x10},
{0x56,0x0C},
{0x58,0x00},
{0x59,0x0F},

//tim,0xing control 2 // //don't touch
{0x60,0x34},
{0x61,0x3A},
{0x62,0x34},
{0x63,0x39},
{0x64,0x34},
{0x65,0x39},
{0x72,0x35},
{0x73,0x38},
{0x74,0x35},
{0x75,0x38},
{0x80,0x02},
{0x81,0x2E},
{0x82,0x0D},
{0x83,0x10},
{0x84,0x0D},
{0x85,0x10},
{0x92,0x1D},
{0x93,0x20},
{0x94,0x1D},
{0x95,0x20},
{0xA0,0x03},
{0xA1,0x2D},
{0xA4,0x2D},
{0xA5,0x03},
{0xA8,0x12},
{0xA9,0x1B},
{0xAA,0x22},
{0xAB,0x2B},
{0xAC,0x10},
{0xAD,0x0E},
{0xB8,0x33},
{0xB9,0x35},
{0xBC,0x0C},
{0xBD,0x0E},
{0xc0,0x3a},
{0xc1,0x3f},
{0xc2,0x3a},
{0xc3,0x3f},
{0xc4,0x3a},
{0xc5,0x3e},
{0xc6,0x3a},
{0xc7,0x3e},
{0xc8,0x3a},
{0xc9,0x3e},
{0xca,0x3a},
{0xcb,0x3e},
{0xcc,0x3b},
{0xcd,0x3d},
{0xce,0x3b},
{0xcf,0x3d},
{0xd0,0x33},
{0xd1,0x3f},

//Page 10
{0x03,0x10},
{0x10,0x03},
{0x11,0x43},
{0x12,0x30},
{0x40,0x80},
{0x41,0x00},
{0x48,0x88},
{0x50,0x48},

{0x60,0x01},
{0x61,0x00},
{0x62,0x7c},
{0x63,0x80},
{0x64,0x48},
{0x66,0x90},
{0x67,0x36},

{0x80,0x00},

//Page 11
//LPF
{0x03,0x11},
{0x10,0x25},
{0x11,0x07},
{0x20,0x00},
{0x21,0x60},
{0x23,0x0A},
{0x60,0x12},
{0x61,0x85},
{0x62,0x00},
{0x63,0x00},
{0x64,0x00},

{0x67,0x70},
{0x68,0x24},
{0x69,0x04},

//Page 12
//2D
{0x03,0x12},
{0x40,0xD3},
{0x41,0x09},
{0x50,0x16},
{0x51,0x24},
{0x70,0x1F},
{0x71,0x00},
{0x72,0x00},
{0x73,0x00},
{0x74,0x12},
{0x75,0x12},
{0x76,0x20},
{0x77,0x80},
{0x78,0x88},
{0x79,0x18},

///////////////////////
{0x90,0x3d},
{0x91,0x34},
{0x99,0x28},
{0x9c,0x05},
{0x9d,0x08},
{0x9e,0x28},
{0x9f,0x28},
{0xb0,0x7d},
{0xb5,0x44},
{0xb6,0x82},
{0xb7,0x52},
{0xb8,0x44},
{0xb9,0x15},
///////////////////////

//Edge
{0x03,0x13},
{0x10,0x01},
{0x11,0x89},
{0x12,0x14},
{0x13,0x19},
{0x14,0x08},
{0x20,0x03},
{0x21,0x05},
{0x23,0x25},
{0x24,0x21},
{0x25,0x08},
{0x26,0x40},
{0x27,0x00},
{0x28,0x08},
{0x29,0x50},
{0x2A,0xE0},
{0x2B,0x10},
{0x2C,0x28},
{0x2D,0x40},
{0x2E,0x00},
{0x2F,0x00},
{0x30,0x11},
{0x80,0x05},
{0x81,0x07},
{0x90,0x05},
{0x91,0x05},
{0x92,0x00},
{0x93,0x30},
{0x94,0x30},
{0x95,0x10},

{0x03,0x14},
{0x10,0x01},

{0x22,0x58},
{0x23,0x45},
{0x24,0x44},

{0x27,0x58},
{0x28,0x80},
{0x29,0x58},
{0x2a,0x80},
{0x2b,0x58},
{0x2c,0x80},

//15page//////////////////////////
{0x03,0x15},
{0x10,0x03},

{0x14,0x52},
{0x16,0x3a},
{0x17,0x2f},

//CMC
{0x30,0xf1},
{0x31,0x71},
{0x32,0x00},
{0x33,0x1f},
{0x34,0xe1},
{0x35,0x42},
{0x36,0x01},
{0x37,0x31},
{0x38,0x72},
//CMC OFS
{0x40,0x90},
{0x41,0x82},
{0x42,0x12},
{0x43,0x86},
{0x44,0x92},
{0x45,0x18},
{0x46,0x84},
{0x47,0x02},
{0x48,0x02},

{0x03,0x16},
{0x10,0x01},
{0x30,0x00},
{0x31,0x06},
{0x32,0x21},
{0x33,0x36},
{0x34,0x58},
{0x35,0x75},
{0x36,0x8e},
{0x37,0xa3},
{0x38,0xb4},
{0x39,0xc3},
{0x3a,0xcf},
{0x3b,0xe2},
{0x3c,0xf0},
{0x3d,0xf9},
{0x3e,0xff},

//Page 17 AE
{0x03,0x17},
{0xC4,0x3C},
{0xC5,0x32},

//Page 20 AE
{0x03,0x20},
{0x10,0x1C},
{0x11,0x04},

{0x20,0x01},
{0x28,0x27},
{0x29,0xA1},

{0x2A,0xF0},
{0x2B,0xf4},
{0x2C,0x2B},

{0x30,0xf8},

{0x3B,0x22},
{0x3C,0xDE},

{0x39,0x22},
{0x3A,0xDE},
{0x3B,0x22},
{0x3C,0xDE},

{0x60,0x70},
{0x61,0x20},

{0x62,0x70},
{0x63,0x20},

{0x68,0x28},
{0x69,0x79},
{0x6A,0x2C},
{0x6B,0xC4},

{0x70,0x34},

{0x76,0x11},
{0x77,0x72},

{0x78,0x12},
{0x79,0x26},
{0x7A,0x23},

{0x7C,0x17},
{0x7D,0x22},

{0x83,0x00},
{0x84,0xaf},
{0x85,0xc8},

{0x86,0x00},
{0x87,0xc8},

{0xa0,0x02},
{0xa1,0xbf},
{0xa2,0x20},

{0x8B,0x3a},
{0x8C,0x98},
{0x8D,0x30},
{0x8E,0xd4},

{0x98,0x8C},
{0x99,0x23},

{0x9c,0x04},
{0x9d,0x4c},
{0x9e,0x00},
{0x9f,0xc8},

{0xB0,0x1D},
{0xB1,0x14},
{0xB2,0x98},
{0xB3,0x17},
{0xB4,0x17},
{0xB5,0x3E},
{0xB6,0x2B},
{0xB7,0x24},
{0xB8,0x21},
{0xB9,0x1F},
{0xBA,0x1E},
{0xBB,0x1D},
{0xBC,0x1C},
{0xBD,0x1B},

{0xC0,0x1A},
{0xC3,0x48},
{0xC4,0x48},

//Page 22 AWB
{0x03,0x22},
{0x10,0xE2},
{0x11,0x2E},
{0x20,0x41},
{0x21,0x40},
{0x24,0xFE},

{0x30,0x80},
{0x31,0x80},
{0x38,0x12},
{0x39,0x33},
{0x40,0xF3},
{0x41,0x43},
{0x42,0x33},
{0x43,0xF3},
{0x44,0x88},
{0x45,0x66},
{0x46,0x08},
{0x47,0x63},

{0x80,0x38},
{0x81,0x20},
{0x82,0x38},

{0x83,0x5A},
{0x84,0x24},
{0x85,0x55},
{0x86,0x24},

{0x87,0x44},
{0x88,0x33},
{0x89,0x3e},
{0x8a,0x34},

{0x8b,0x03},
{0x8d,0x22},
{0x8e,0x21},

{0x8F,0x63},
{0x90,0x62},
{0x91,0x5E},
{0x92,0x5A},
{0x93,0x50},
{0x94,0x42},
{0x95,0x3B},
{0x96,0x34},
{0x97,0x2D},
{0x98,0x2B},
{0x99,0x29},
{0x9A,0x27},
{0x9B,0x0B},
{0xB4,0xBF},

///////////////////////////// Page 48
{0x03,0x48},
{0x10,0x05},
{0x11,0x00},
{0x12,0x00},
{0x16,0xc4},
{0x17,0x00},
{0x19,0x00},
{0x1A,0x06},
{0x1C,0x02},
{0x1D,0x04},
{0x1E,0x07},
{0x1F,0x04},
{0x20,0x00},
{0x21,0xb8},
{0x22,0x00},
{0x23,0x01},
{0x30,0x05},
{0x31,0x00},
{0x32,0x06},
{0x34,0x01},
{0x35,0x02},
{0x36,0x01},
{0x37,0x03},
{0x38,0x00},
{0x39,0x4a},
{0x3C,0x00},
{0x3D,0xFA},
{0x3F,0x10},
{0x40,0x00},
{0x41,0x20},
{0x42,0x00},
{0x03,0x22},
{0x10,0xFB},
{0x03,0x20},
{0x10,0x9C},
{0x03,0x00},
{0x01,0x71},
{SENSOR_WRITE_DELAY,0x14},
};

static SENSOR_REG_TAB_INFO_T s_Sr030pc50_Resolution_Tab_YUV[]=
{
	{ADDR_AND_LEN_OF_ARRAY(sr030pc50_init_regs), 0, 0, 24, SENSOR_IMAGE_FORMAT_YUV422},
	{PNULL, 0, 640, 480, 24, SENSOR_IMAGE_FORMAT_YUV422},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	// YUV422 PREVIEW 2
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

static SENSOR_TRIM_T s_Sr030pc50_Resolution_Trim_Tab[] = {
	{0, 0, 640, 480, 0, 192,0, {0, 0, 640, 480}},
	{0, 0, 640, 480, 68, 192, 0x2bc, {0, 0, 640, 480}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}}
};

LOCAL SENSOR_VIDEO_INFO_T s_sr030pc50_video_info[] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {30, 30, 100, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}},PNULL},
	{{{0, 0, 0, 0}, {30, 30, 100, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}
};


static SENSOR_IOCTL_FUNC_TAB_T s_Sr030pc50_Ioctl_Func_Tab =
{
	// Internal
	PNULL,
	PNULL,//Sr030pc50_PowerOn,
	PNULL,
	Sr030pc50_Identify,

	PNULL,// write register
	PNULL,// read  register
	PNULL,
	Sr030pc50_GetResolutionTrimTab,

	// External
	PNULL,
	PNULL,
	PNULL,

	Sr030pc50_Set_Brightness,
	PNULL,//Sr030pc50_Set_Contrast,
	PNULL,
	Sr030pc50_Set_Saturation,

	Sr030pc50_Set_Work_Mode,
	Sr030pc50_Set_Image_Effect,

	Sr030pc50_BeforeSnapshot,
	Sr030pc50_AfterSnapshot,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,

	PNULL,
	PNULL,
	PNULL,
	PNULL,
	Sr030pc50_Set_Awb,
	PNULL,
	PNULL,//Sr030pc50_Set_Iso,
	PNULL,//Sr030pc50_Set_Ev,
	PNULL,//Sr030pc50_Check_Image_Format_Support,
	PNULL,
	PNULL,
	Sr030pc50_GetExifInfo,
	PNULL,
	Sr030pc50_Set_Anti_Flicker,
	Sr030pc50_Set_Video_Mode,
	PNULL,
	PNULL, //meter_mode
	PNULL, //get_status
	Sr030pc50_StreamOn,
	Sr030pc50_StreamOff,
	PNULL
};

SENSOR_INFO_T g_sr030pc50_yuv_info =
{
	SR030PC50_I2C_ADDR_W,                // salve i2c write address
	SR030PC50_I2C_ADDR_R,                 // salve i2c read address
	SENSOR_I2C_FREQ_400,
	                                // bit1: 0: i2c register addr  is 8 bit, 1: i2c register addr  is 16 bit
	                                // other bit: reseved
	SENSOR_HW_SIGNAL_PCLK_P|\
	SENSOR_HW_SIGNAL_VSYNC_N|\
	SENSOR_HW_SIGNAL_HSYNC_P,        // bit0: 0:negative; 1:positive -> polarily of pixel clock
	                                // bit2: 0:negative; 1:positive -> polarily of horizontal synchronization signal
	                                // bit4: 0:negative; 1:positive -> polarily of vertical synchronization signal
	                                // other bit: reseved

	// preview mode
	SENSOR_ENVIROMENT_NORMAL|\
	SENSOR_ENVIROMENT_NIGHT,

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

	0x7,                                // bit[0:7]: count of step in brightness, contrast, sharpness, saturation
	                                // bit[8:31] reseved

	SENSOR_LOW_PULSE_RESET,            // reset pulse level
	10,                                // reset pulse width(ms)

	SENSOR_LOW_LEVEL_PWDN,            // 1: high level valid; 0: low level valid

	1,                                // count of identify code
	{{0x04, 0xB8},						// supply two code to identify sensor.
	{0x04, 0xB8}},						// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,                // voltage of avdd

	640,                            // max width of source image
	480,                            // max height of source image
	"SR030PC50",                        // name of sensor

	SENSOR_IMAGE_FORMAT_MAX,        // define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	                                // if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_YUV422_YUYV,  //UYVY  // pattern of input image form sensor;

	s_Sr030pc50_Resolution_Tab_YUV,    // point to resolution table information structure
	&s_Sr030pc50_Ioctl_Func_Tab,        // point to ioctl function table
	PNULL,                            // information and table about Rawrgb sensor
	PNULL,//&g_HI255_ext_info,                // extend information about sensor

	SENSOR_AVDD_1800MV,                     // iovdd
	SENSOR_AVDD_1800MV,                      // dvdd
	2,                     // skip frame num before preview
	0,                      // skip frame num before capture
	0,                      // deci frame num during preview
	0,                      // deci frame num during video preview

	0,
	0,
	0,
	0,
	0,
	{SENSOR_INTERFACE_TYPE_CSI2, 1, 8, 1},
	s_sr030pc50_video_info,
	1,			// skip frame num while change setting
	48,			// horizontal view angle
	48,			// vertical view angle
};

static unsigned long Sr030pc50_GetExifInfo(unsigned long param)
{
	return (unsigned long)&s_Sr030pc50_Exif;
}

static unsigned long Sr030pc50_GetResolutionTrimTab(unsigned long param)
{
	return (unsigned long) s_Sr030pc50_Resolution_Trim_Tab;
}

static unsigned long Sr030pc50_SetExifInfo_Exposure(unsigned long param)
{
	EXIF_SPEC_PIC_TAKING_COND_T *sensor_exif_info_ptr = &s_Sr030pc50_Exif;

	sensor_exif_info_ptr->valid.ExposureTime = 1;
	sensor_exif_info_ptr->ExposureTime.numerator = 0x01;
	sensor_exif_info_ptr->ExposureTime.denominator = 48000000 / 2 /  6/ param ;

	return 0;
}

static unsigned long Sr030pc50_SetExifInfo_ISO(unsigned long param)
{
	EXIF_SPEC_PIC_TAKING_COND_T *sensor_exif_info_ptr = &s_Sr030pc50_Exif;
	uint8_t iso       = 0;

	Sensor_WriteReg(0x03, 0x20);
	iso=Sensor_ReadReg(0xb0);
	Sensor_WriteReg(0x03, 0x00);
	SENSOR_PRINT("iso=%x;",iso);

	sensor_exif_info_ptr->valid.ISOSpeedRatings = 1;
	sensor_exif_info_ptr->ISOSpeedRatings.count = 0x02;
	if (iso <= 0x1b) {
		sensor_exif_info_ptr->ISOSpeedRatings.ptr[0]= 100;
		sensor_exif_info_ptr->ISOSpeedRatings.ptr[1]= 0;
	} else if (iso <= 0x35) {
		sensor_exif_info_ptr->ISOSpeedRatings.ptr[0]= 200;
		sensor_exif_info_ptr->ISOSpeedRatings.ptr[1]= 0;
	} else if (iso <= 0x65) {
		sensor_exif_info_ptr->ISOSpeedRatings.ptr[0]= 400 % 255;
		sensor_exif_info_ptr->ISOSpeedRatings.ptr[1]= 400 / 255;
	} else if (iso <= 0x95) {
		sensor_exif_info_ptr->ISOSpeedRatings.ptr[0]= 800 % 255;
		sensor_exif_info_ptr->ISOSpeedRatings.ptr[1]= 800 / 255;
	} else {
		sensor_exif_info_ptr->ISOSpeedRatings.ptr[0]= 1600 % 255;
		sensor_exif_info_ptr->ISOSpeedRatings.ptr[1]= 1600 / 255;
	}

	return 0;
}

static uint32_t Sr030pc50_InitExifInfo(void)
{
	EXIF_SPEC_PIC_TAKING_COND_T *exif_ptr = &s_Sr030pc50_Exif;

	memset(&s_Sr030pc50_Exif , 0, sizeof(EXIF_SPEC_PIC_TAKING_COND_T));

	exif_ptr->valid.FNumber = 1;
	exif_ptr->FNumber.numerator = 14;
	exif_ptr->FNumber.denominator = 5;

	exif_ptr->valid.ExposureProgram = 1;
	exif_ptr->ExposureProgram = 0x04;

	//exif_ptr->SpectralSensitivity[MAX_ASCII_STR_SIZE];
	//exif_ptr->ISOSpeedRatings;
	//exif_ptr->OECF;

	//exif_ptr->ShutterSpeedValue;

	exif_ptr->valid.ApertureValue = 1;
	exif_ptr->ApertureValue.numerator = 14;
	exif_ptr->ApertureValue.denominator = 5;

	//exif_ptr->BrightnessValue;
	//exif_ptr->ExposureBiasValue;

	exif_ptr->valid.MaxApertureValue = 1;
	exif_ptr->MaxApertureValue.numerator = 14;
	exif_ptr->MaxApertureValue.denominator = 5;

	//exif_ptr->SubjectDistance;
	//exif_ptr->MeteringMode;
	//exif_ptr->LightSource;
	//exif_ptr->Flash;

	exif_ptr->valid.FocalLength = 1;
	exif_ptr->FocalLength.numerator = 289;
	exif_ptr->FocalLength.denominator = 100;

	//exif_ptr->SubjectArea;
	//exif_ptr->FlashEnergy;
	//exif_ptr->SpatialFrequencyResponse;
	//exif_ptr->FocalPlaneXResolution;
	//exif_ptr->FocalPlaneYResolution;
	//exif_ptr->FocalPlaneResolutionUnit;
	//exif_ptr->SubjectLocation[2];
	//exif_ptr->ExposureIndex;
	//exif_ptr->SensingMethod;

	exif_ptr->valid.FileSource = 1;
	exif_ptr->FileSource = 0x03;

	//exif_ptr->SceneType;
	//exif_ptr->CFAPattern;
	//exif_ptr->CustomRendered;

	exif_ptr->valid.ExposureMode = 1;
	exif_ptr->ExposureMode = 0x00;

	exif_ptr->valid.WhiteBalance = 1;
	exif_ptr->WhiteBalance = 0x00;

	//exif_ptr->DigitalZoomRatio;
	//exif_ptr->FocalLengthIn35mmFilm;
	//exif_ptr->SceneCaptureType;
	//exif_ptr->GainControl;
	//exif_ptr->Contrast;
	//exif_ptr->Saturation;
	//exif_ptr->Sharpness;
	//exif_ptr->DeviceSettingDescription;
	//exif_ptr->SubjectDistanceRange;

	return SENSOR_SUCCESS;
}

static unsigned long Sr030pc50_PowerOn(unsigned long power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_sr030pc50_yuv_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_sr030pc50_yuv_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_sr030pc50_yuv_info.iovdd_val;
	BOOLEAN power_down = g_sr030pc50_yuv_info.power_down_level;
	BOOLEAN reset_level = g_sr030pc50_yuv_info.reset_pulse_level;

	if (SENSOR_TRUE == power_on) {
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_SetResetLevel(reset_level);
		Sensor_PowerDown(power_down);
		Sensor_SetIovddVoltage(iovdd_val);
		SENSOR_Sleep(5);
		Sensor_SetAvddVoltage(avdd_val);
		SENSOR_Sleep(10);
		Sensor_PowerDown(!power_down);
		SENSOR_Sleep(10);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		SENSOR_Sleep(30);
		Sensor_SetResetLevel(!reset_level);
		SENSOR_Sleep(10);
	} else {
		Sensor_SetResetLevel(reset_level);
		SENSOR_Sleep(1);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_PowerDown(power_down);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED,
				SENSOR_AVDD_CLOSED);
	}
	return SENSOR_SUCCESS;
}

static unsigned long Sr030pc50_Identify(unsigned long param)
{
	uint32_t i = 0;
	uint16_t chip_id = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT_HIGH("sensor_sr030pc50: mipi yuv identify.");

	Sensor_WriteReg(0x03, 0x00);

	for (i = 0; i < 3; i++) {
		chip_id = Sensor_ReadReg(SR030PC50_CHIP_ID_ADDR);
		if (SR030PC50_CHIP_ID_VALUE == chip_id) {
			SENSOR_PRINT_HIGH("sensor_sr030pc50: this is sr030pc50 sensor !");
			Sr030pc50_InitExifInfo();
			return SENSOR_SUCCESS;
		} else {
			SENSOR_PRINT_ERR("sensor_sr030pc50: identify fail, chip_id=%d", chip_id);
		}
	}

	return ret_value;
}

static unsigned long Sr030pc50_StreamOn(unsigned long param)
{
	SENSOR_PRINT_HIGH("sensor: Sr030pc50_StreamOn");

	Sensor_WriteReg(0x03, 0x00);
	Sensor_WriteReg(0x01, 0x70);

	return 0;
}

static unsigned long Sr030pc50_StreamOff(unsigned long param)
{
	SENSOR_PRINT("sensor: Sr030pc50_StreamOff");

	Sensor_WriteReg(0x03, 0x00);
	Sensor_WriteReg(0x01, 0x71);

	return 0;
}

static const SENSOR_REG_T Sr030pc50_Brightness_Tab[][3]=
{
	{
		{0xff, 0xff},
	},
	{
		{0xff,0xff}
	},
	{
		{0xff,0xff}
	},
	{
		{0xff,0xff}
	},
	{
		{0xff,0xff}
	},
	{
		{0xff,0xff}
	},
	{
		{0xff,0xff}
	},
};

static unsigned long Sr030pc50_Set_Brightness(unsigned long level)
{
	uint16_t i = 0x00;
	SENSOR_REG_T_PTR sensor_reg_ptr = (SENSOR_REG_T_PTR)Sr030pc50_Brightness_Tab[level];

	if (level > 6)
		return 0;

	for (i = 0x00; (0xff != sensor_reg_ptr[i].reg_addr)  || (0xff != sensor_reg_ptr[i].reg_value); i++) {
		Sensor_WriteReg_8bits(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	SENSOR_PRINT("sensor: Sr030pc50_Set_Brightness = %ld", level);

	return 0;
}

static const SENSOR_REG_T Sr030pc50_Saturation_Tab[][4]=
{
	{//-3
		{0xff, 0xff},
	},
	{//-2
		{0xff, 0xff},
	},
	{//-1
		{0xff, 0xff},
	},
	{//00
		{0xff, 0xff},
	},
	{//+1
		{0xff, 0xff},
	},
	{//+2
		{0xff, 0xff},
	},
	{//+3
		{0xff, 0xff},
	}
};

static unsigned long Sr030pc50_Set_Saturation(unsigned long level)
{
	uint16_t i = 0x00;
	SENSOR_REG_T_PTR sensor_reg_ptr = (SENSOR_REG_T_PTR)Sr030pc50_Saturation_Tab[level];

	if ( level > 7)
		return 0;

	for (i = 0x00; (0xff != sensor_reg_ptr[i].reg_addr) || (0xff != sensor_reg_ptr[i].reg_value); i++) {
		Sensor_WriteReg_8bits(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	SENSOR_PRINT("sensor: Sr030pc50_Set_Saturation = %ld", level);

	return 0;
}

static const SENSOR_REG_T Sr030pc50_Image_Effect_Tab[][7]=
{
	// effect normal
	{
		{0xff,0xff},
	},
	// effect mono
	{
		{0xff,0xff},
	},
	// effect RED
	{
		{0xff,0xff},
	},
	// effect GREEN
	{
		{0xff,0xff},
	},
	// effect  BLUE
	{
		{0xff,0xff},
	},
	// effect  YELLOW
	{
		{0xff,0xff},
	},
	// effect NEGATIVE
	{
		{0xff,0xff},
	},
	//effect sepia
	{
		{0xff,0xff},
	},
};

static unsigned long Sr030pc50_Set_Image_Effect(unsigned long effect_type)
{
	uint16_t i = 0x00;
	SENSOR_REG_T_PTR sensor_reg_ptr = (SENSOR_REG_T_PTR)Sr030pc50_Image_Effect_Tab[effect_type];

	if (effect_type > 7)
		return 0;

	for (i = 0x00; (0xff != sensor_reg_ptr[i].reg_addr) || (0xff != sensor_reg_ptr[i].reg_value); i++) {
		Sensor_WriteReg_8bits(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	SENSOR_PRINT("sensor: Sr030pc50_Set_Image_Effect = %ld", effect_type);

	return 0;
}

LOCAL const SENSOR_REG_T Sr030pc50_Anti_Banding_Flicker_Tab[][3]=
{

	{//50hz
		{0xff, 0xff},
	},
	{//60hz
		{0xff, 0xff},
	}
};

static unsigned long Sr030pc50_Set_Anti_Flicker(unsigned long mode)
{
	uint16_t i = 0x00;
	SENSOR_REG_T_PTR sensor_reg_ptr=(SENSOR_REG_T_PTR)Sr030pc50_Anti_Banding_Flicker_Tab[mode];

	if (mode > 1)
		return 0;

	for (i = 0x00; (0xff != sensor_reg_ptr[i].reg_addr) || (0xff!=sensor_reg_ptr[i].reg_value); i++) {
		Sensor_WriteReg_8bits(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	SENSOR_PRINT("sensor: Sr030pc50_Set_Anti_Flicker= %ld", mode);

	return 0;
}

static const SENSOR_REG_T Sr030pc50_Video_Mode_Tab[][44]=
{
	{//video preview
		{0x03, 0x00},
		{0x01, 0xf1},
		{0x11, 0x91},

		{0x40, 0x00},
		{0x41, 0x90},
		{0x42, 0x00},
		{0x43, 0x62},

		{0x03, 0x20},
		{0x10, 0x1c},

		{0x2a, 0xf0},
		{0x2b, 0xf4},
		{0x30, 0xf8},

		{0x83, 0x00},
		{0x84, 0xaf},
		{0x85, 0xc8},

		{0xa0, 0x02},
		{0xa1, 0xbf},
		{0xa2, 0x20},

		{0x03, 0x20},
		{0x10, 0x9c},

		{0x03, 0x00},
		{0x11, 0x91},

		{0x01, 0x71},

		{0xff, 0x28}, //delay 400ms
		{0xff, 0xff} ,
	},
	//video encode
	{
		{0x03, 0x00},
		{0x01, 0x71},
		{0x11, 0x91},
		{0x40, 0x00}, //Hblank 128
		{0x41, 0x80},
		{0x42, 0x00}, //Vblank 2
		{0x43, 0x02},

		{0x90, 0x03}, //BLC_TIME_TH_ON
		{0x91, 0x03}, //BLC_TIME_TH_OFF
		{0x92, 0x98}, //BLC_AG_TH_ON
		{0x93, 0x90}, //BLC_AG_TH_OFF

		{0x03, 0x20}, //Page 20
		{0x10, 0x1c},

		{0x2a, 0x90},
		{0x2b, 0xf4},
		{0x30, 0xf8},

		{0x83, 0x00}, //EXP Normal 33.33 fps
		{0x84, 0xaf},
		{0x85, 0xb6},

		{0xa0, 0x00}, //exp max 33.33fps
		{0xa1, 0xaf},
		{0xa2, 0xb6},

		{0x91, 0x00}, //EXP Fix 30.00 fps
		{0x92, 0xc3},
		{0x93, 0x3c},

		{0x10, 0x9c},
		{0x03, 0x00},
		{0x11, 0x91},
		{0x01, 0x70},

		{0x03, 0x00},
		{0x50, 0x00},

		{0xff, 0x28}, //400ms
		{0xff, 0xff},
	},
	{//upcc mode
		{0xff, 0xff},
	}
};

static unsigned long Sr030pc50_Set_Video_Mode(unsigned long mode)
{
	uint16_t i = 0x00;
	SENSOR_REG_T_PTR sensor_reg_ptr = (SENSOR_REG_T_PTR)Sr030pc50_Video_Mode_Tab[mode];

	if (mode > 2)
		return 0;

	for (i = 0x00; (0xff != sensor_reg_ptr[i].reg_addr) || (0xff!=sensor_reg_ptr[i].reg_value); i++) {
		Sensor_WriteReg_8bits(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	SENSOR_PRINT("sensor:  Sr030pc50_Set_Video_Mode = %ld", mode);

	return 0;
}

static const SENSOR_REG_T Sr030pc50_Awb_Tab[][10] =
{
	{//auto

		{0xff, 0xff},
	},
	{//incandescence

		{0xff, 0xff},
	},
	{//u30 not used
		{0xff, 0xff}
	},
	{//CWF  not used
		{0xff, 0xff}
	},
	//FLUORESCENT:
	{//fluorescent

		{0xff, 0xff},
	},
	{//daylight

		{0xff, 0xff},
	},
	{//cloudy

		{0xff, 0xff},
	}
};

static unsigned long Sr030pc50_Set_Awb(unsigned long mode)
{

	uint16_t i = 0x00;
	SENSOR_REG_T_PTR sensor_reg_ptr = (SENSOR_REG_T_PTR)Sr030pc50_Awb_Tab[mode];

	if (mode > 6)
		return 0;

	for (i = 0; (0xff != sensor_reg_ptr[i].reg_addr) || (0xff != sensor_reg_ptr[i].reg_value); i++) {
		Sensor_WriteReg_8bits(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_WHITEBALANCE, (uint32_t)mode);

	SENSOR_PRINT("sensor: Sr030pc50_Set_Awb = %ld", mode);

	return 0;
}

static const SENSOR_REG_T Sr030pc50_Work_Mode_Tab[][175]=
{
	{//auto

		{0xff, 0xff},
	},
	{//nightmode normal

		{0xff, 0xff},
	},
	{//nightmode dark

		{0xff, 0xff},
	}
};

LOCAL unsigned long Sr030pc50_Set_Work_Mode(unsigned long mode)
{
	uint16_t i = 0x00;
	SENSOR_REG_T_PTR sensor_reg_ptr = (SENSOR_REG_T_PTR)Sr030pc50_Work_Mode_Tab[mode];

	if (mode > 2)
		return 0;

	for (i = 0; (0xff != sensor_reg_ptr[i].reg_addr) || (0xff != sensor_reg_ptr[i].reg_value); i++) {
		Sensor_WriteReg_8bits(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	CMR_LOGI("sensor: Sr030pc50_Set_Work_Mode = %ld", mode);

	return 0;
}

static unsigned long Sr030pc50_BeforeSnapshot(unsigned long param)
{
	param = param&0xffff;

	if (SENSOR_MODE_PREVIEW_ONE >= param) {
		return SENSOR_SUCCESS;
	}

	return SENSOR_SUCCESS;
}

static unsigned long Sr030pc50_AfterSnapshot(unsigned long param)
{
	uint32_t exposure;

	SENSOR_PRINT("Sr030pc50_AfterSnapshot =%ld", param);

	return SENSOR_SUCCESS;
}

#ifdef __cplusplus
}
#endif

