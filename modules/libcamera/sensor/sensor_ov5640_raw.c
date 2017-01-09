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



#define ov5640_I2C_ADDR_W        0x3c
#define ov5640_I2C_ADDR_R         0x3c

LOCAL uint32_t _ov5640_GetResolutionTrimTab(uint32_t param);
LOCAL uint32_t _ov5640_PowerOn(uint32_t power_on);
LOCAL uint32_t _ov5640_Identify(uint32_t param);

LOCAL uint32_t _ov5640_BeforeSnapshot(uint32_t param);
LOCAL uint32_t _ov5640_after_snapshot(uint32_t param);
LOCAL uint32_t _ov5640_write_exposure(uint32_t param);
LOCAL uint32_t _ov5640_write_gain(uint32_t param);
LOCAL uint32_t _ov5640_write_af(uint32_t param);

LOCAL const SENSOR_REG_T ov5640_common_raw_init[] =
{
	{0x4202, 0x0f},/*kenxu add 20120207 for stream off*/
	{SENSOR_WRITE_DELAY, 0x64},
	{0x3103, 0x11},
	{0x3008, 0x82},//0x;delay 5ms
	{SENSOR_WRITE_DELAY, 0x20},
	{0x3008, 0x42},
	{0x3103, 0x03},
	{0x3017, 0xff},
	{0x3018, 0xff},
	{0x3034, 0x1a},
	{0x3035, 0x11},
	{0x3036, 0x46},
	{0x3037, 0x13},
	{0x3108, 0x01},
	{0x3630, 0x36},
	{0x3631, 0x0e},
	{0x3632, 0xe2},
	{0x3633, 0x12},
	{0x3621, 0xe0},
	{0x3704, 0xa0},
	{0x3703, 0x5a},
	{0x3715, 0x78},
	{0x3717, 0x01},
	{0x370b, 0x60},
	{0x3705, 0x1a},
	{0x3905, 0x02},
	{0x3906, 0x10},
	{0x3901, 0x0a},
	{0x3731, 0x12},
	{0x3600, 0x08},
	{0x3601, 0x33},
	{0x302d, 0x60},
	{0x3620, 0x52},
	{0x371b, 0x20},
	{0x471c, 0x50},
	{0x3a13, 0x43},
	{0x3a18, 0x00},
	{0x3a19, 0xf8},
	{0x3635, 0x13},
	{0x3636, 0x03},
	{0x3634, 0x40},
	{0x3622, 0x01},
	{0x3c01, 0x34},
	{0x3c04, 0x28},
	{0x3c05, 0x98},
	{0x3c06, 0x00},
	{0x3c07, 0x07},
	{0x3c08, 0x00},
	{0x3c09, 0x1c},
	{0x3c0a, 0x9c},
	{0x3c0b, 0x40},
	{0x3820, 0x41},
	{0x3821, 0x01},
	{0x3814, 0x31},
	{0x3815, 0x31},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x04},
	{0x3804, 0x0a},
	{0x3805, 0x3f},
	{0x3806, 0x07},
	{0x3807, 0x9b},
	{0x3808, 0x05},
	{0x3809, 0x00},
	{0x380a, 0x03},
	{0x380b, 0xc0},
	{0x380c, 0x07},
	{0x380d, 0x68},
	{0x380e, 0x03},
	{0x380f, 0xd8},
	{0x3810, 0x00},
	{0x3811, 0x10},
	{0x3812, 0x00},
	{0x3813, 0x06},
	{0x3618, 0x00},
	{0x3612, 0x29},
	{0x3708, 0x64},
	{0x3709, 0x52},
	{0x370c, 0x03},
	{0x3a02, 0x03},
	{0x3a03, 0xd8},
	{0x3a08, 0x01},
	{0x3a09, 0x27},
	{0x3a0a, 0x00},
	{0x3a0b, 0xf6},
	{0x3a0e, 0x03},
	{0x3a0d, 0x04},
	{0x3a14, 0x03},
	{0x3a15, 0xd8},
	{0x4001, 0x02},
	{0x4004, 0x02},
	{0x3000, 0x00},
	{0x3002, 0x1c},
	{0x3004, 0xff},
	{0x3006, 0xc3},
	{0x300e, 0x58},
	{0x302e, 0x00},
	{0x4300, 0xf8},
	{0x501f, 0x03},
	{0x4713, 0x03},
	{0x4407, 0x04},
	{0x440e, 0x00},
	{0x460b, 0x37},
	{0x460c, 0x20},
	{0x4837, 0x16},
	{0x3824, 0x04},
	{0x5000, 0x06},
	{0x5001, 0x00},
	{0x3a0f, 0x36},
	{0x3a10, 0x2e},
	{0x3a1b, 0x38},
	{0x3a1e, 0x2c},
	{0x3a11, 0x70},
	{0x3a1f, 0x18},
	{0x3008, 0x02},
	{0x3035, 0x21},
	{0x3400, 0x04},
	{0x3401, 0x00},
	{0x3402, 0x04},
	{0x3403, 0x00},
	{0x3404, 0x04},
	{0x3405, 0x00},
	{0x3406, 0x01},
	//{0x3503, 0x03},//disable ae

	{0x4202, 0x00}		/*kenxu add 20120207 for stream on*/
};

LOCAL const SENSOR_REG_T ov5640_1280X960_raw[] = {
	{0x4202, 0x0f},/*kenxu add 20120207 for stream off*/
	{SENSOR_WRITE_DELAY, 0x64},
	{0x3103, 0x11},
	{0x3008, 0x82},//0x;delay 5ms
	{SENSOR_WRITE_DELAY, 0x20},
	{0x3008, 0x42},
	{0x3103, 0x03},
	{0x3017, 0xff},
	{0x3018, 0xff},
	{0x3034, 0x1a},
	{0x3035, 0x11},
	{0x3036, 0x46},
	{0x3037, 0x13},
	{0x3108, 0x01},
	{0x3630, 0x36},
	{0x3631, 0x0e},
	{0x3632, 0xe2},
	{0x3633, 0x12},
	{0x3621, 0xe0},
	{0x3704, 0xa0},
	{0x3703, 0x5a},
	{0x3715, 0x78},
	{0x3717, 0x01},
	{0x370b, 0x60},
	{0x3705, 0x1a},
	{0x3905, 0x02},
	{0x3906, 0x10},
	{0x3901, 0x0a},
	{0x3731, 0x12},
	{0x3600, 0x08},
	{0x3601, 0x33},
	{0x302d, 0x60},
	{0x3620, 0x52},
	{0x371b, 0x20},
	{0x471c, 0x50},
	{0x3a13, 0x43},
	{0x3a18, 0x00},
	{0x3a19, 0xf8},
	{0x3635, 0x13},
	{0x3636, 0x03},
	{0x3634, 0x40},
	{0x3622, 0x01},
	{0x3c01, 0x34},
	{0x3c04, 0x28},
	{0x3c05, 0x98},
	{0x3c06, 0x00},
	{0x3c07, 0x07},
	{0x3c08, 0x00},
	{0x3c09, 0x1c},
	{0x3c0a, 0x9c},
	{0x3c0b, 0x40},
	{0x3820, 0x41},
	{0x3821, 0x01},
	{0x3814, 0x31},
	{0x3815, 0x31},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x04},
	{0x3804, 0x0a},
	{0x3805, 0x3f},
	{0x3806, 0x07},
	{0x3807, 0x9b},
	{0x3808, 0x05},
	{0x3809, 0x00},
	{0x380a, 0x03},
	{0x380b, 0xc0},
	{0x380c, 0x07},
	{0x380d, 0x68},
	{0x380e, 0x03},
	{0x380f, 0xd8},
	{0x3810, 0x00},
	{0x3811, 0x10},
	{0x3812, 0x00},
	{0x3813, 0x06},
	{0x3618, 0x00},
	{0x3612, 0x29},
	{0x3708, 0x64},
	{0x3709, 0x52},
	{0x370c, 0x03},
	{0x3a02, 0x03},
	{0x3a03, 0xd8},
	{0x3a08, 0x01},
	{0x3a09, 0x27},
	{0x3a0a, 0x00},
	{0x3a0b, 0xf6},
	{0x3a0e, 0x03},
	{0x3a0d, 0x04},
	{0x3a14, 0x03},
	{0x3a15, 0xd8},
	{0x4001, 0x02},
	{0x4004, 0x02},
	{0x3000, 0x00},
	{0x3002, 0x1c},
	{0x3004, 0xff},
	{0x3006, 0xc3},
	{0x300e, 0x58},
	{0x302e, 0x00},
	{0x4300, 0xf8},
	{0x501f, 0x03},
	{0x4713, 0x03},
	{0x4407, 0x04},
	{0x440e, 0x00},
	{0x460b, 0x37},
	{0x460c, 0x20},
	{0x4837, 0x16},
	{0x3824, 0x04},
	{0x5000, 0x06},
	{0x5001, 0x00},
	{0x3a0f, 0x36},
	{0x3a10, 0x2e},
	{0x3a1b, 0x38},
	{0x3a1e, 0x2c},
	{0x3a11, 0x70},
	{0x3a1f, 0x18},
	{0x3008, 0x02},
	{0x3035, 0x21},
	{0x3400, 0x04},
	{0x3401, 0x00},
	{0x3402, 0x04},
	{0x3403, 0x00},
	{0x3404, 0x04},
	{0x3405, 0x00},
	{0x3406, 0x01},
	//{0x3503, 0x03},//disable ae

	{0x4202, 0x00}		/*kenxu add 20120207 for stream on*/
};




LOCAL SENSOR_REG_TAB_INFO_T s_ov5640_resolution_Tab_RAW[] = {
	{ADDR_AND_LEN_OF_ARRAY(ov5640_common_raw_init), 0, 0, 24,
	 SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(ov5640_1280X960_raw), 640, 480, 24,
	 SENSOR_IMAGE_FORMAT_RAW},
 	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

LOCAL SENSOR_TRIM_T s_ov5640_Resolution_Trim_Tab[] = {
	{0, 0, 640, 480, 0, 0},
	{0, 0, 640, 480, 122, 42},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0}
};

static const uint8_t s_ov5640_lnc_00[]=
{
	0x00
};
static const uint8_t s_ov5640_lnc_01[]=
{
	0x00
};
static const uint8_t s_ov5640_lnc_02[]=
{
	0x00
};
static const uint8_t s_ov5640_lnc_03[]=
{
	0x00
};
static const uint8_t s_ov5640_lnc_04[]=
{
	0x00
};
static const uint8_t s_ov5640_lnc_05[]=
{
	0x00
};
static const uint8_t s_ov5640_lnc_06[]=
{
	0x00
};
static const uint8_t s_ov5640_lnc_07[]=
{
	0x00
};
static const uint8_t s_ov5640_lnc_08[]=
{
	0x00
};

static const uint8_t s_ov5640_ae_weight_customer[]=
{
	0x00
};

/* 00: 0x->50hz 1x->60hz x0->normal x1->night*/
static const uint16_t s_ov5640_aes_00[SENSOR_AE_NUM]={
	0x00
};

static const uint16_t s_ov5640_aeg_00[SENSOR_AE_NUM]={
	0x00
};

static const uint16_t s_ov5640_aes_10[SENSOR_AE_NUM]={
	0x00
};

static const uint16_t s_ov5640_aeg_10[SENSOR_AE_NUM]={
	0x00
};

static const uint16_t s_ov5640_aes_01[SENSOR_AE_NUM]={
	0x00
};

static const uint16_t s_ov5640_aeg_01[SENSOR_AE_NUM]={
	0x00
};

static const uint16_t s_ov5640_aes_11[SENSOR_AE_NUM]={
	0x00
};

static const uint16_t s_ov5640_aeg_11[SENSOR_AE_NUM]={
	0x00
};
#if 0
static const uint8_t s_ov5640_tune_info[sizeof(struct sensor_raw_tune_info)]={
#include "sensor_ov5640_tune_info.dat"
};
#endif
static struct sensor_raw_tune_info s_ov5640_tune_info;

static struct sensor_raw_fix_info s_ov5640_fix_info={
//ae
{
	(uint8_t*)s_ov5640_ae_weight_customer,

	{
		{
			(uint32_t*)s_ov5640_aes_00,
			(uint16_t*)s_ov5640_aes_00,
			{
				{128, 250,},
				{128, 250,},
				{128, 250,},
				{128, 250,},
				{128, 250,},
			},
		},

		{
			(uint32_t*)s_ov5640_aes_01,
			(uint16_t*)s_ov5640_aes_01,
			{
				{128, 250,},
				{128, 250,},
				{128, 250,},
				{128, 250,},
				{128, 250,},
			}
		},

		{
			(uint32_t*)s_ov5640_aes_10,
			(uint16_t*)s_ov5640_aes_10,
			{
				{128, 250,},
				{128, 250,},
				{128, 250,},
				{128, 250,},
				{128, 250,},
			}
		},

		{
			(uint32_t*)s_ov5640_aes_11,
			(uint16_t*)s_ov5640_aeg_11,
			{
				{128, 250,},
				{128, 250,},
				{128, 250,},
				{128, 250,},
				{128, 250,},
			},
		},
	},
},

//lnc
{
	{
		{
			{
				0x00,
				(uint16_t *)s_ov5640_lnc_00,
				sizeof(s_ov5640_lnc_00),
			},
			{
				0x00,
				(uint16_t *)s_ov5640_lnc_01,
				sizeof(s_ov5640_lnc_01),
			},
			{
				0x00,
				(uint16_t *)s_ov5640_lnc_02,
				sizeof(s_ov5640_lnc_02),
			},
			{
				0x00,
				(uint16_t *)s_ov5640_lnc_03,
				sizeof(s_ov5640_lnc_03),
			},
			{
				0x00,
				(uint16_t *)s_ov5640_lnc_04,
				sizeof(s_ov5640_lnc_04),
			},
			{
				0x00,
				(uint16_t *)s_ov5640_lnc_05,
				sizeof(s_ov5640_lnc_05),
			},
			{
				0x00,
				(uint16_t *)s_ov5640_lnc_06,
				sizeof(s_ov5640_lnc_06),
			},
			{
				0x00,
				(uint16_t *)s_ov5640_lnc_07,
				sizeof(s_ov5640_lnc_07),
			},
			{
				0x00,
				(uint16_t *)s_ov5640_lnc_08,
				sizeof(s_ov5640_lnc_08),
			}
		},
	},
},
};

static struct sensor_version_info s_ov5640_version_info={
	SENSOR_RAW_VERSION_ID,
	sizeof(struct sensor_raw_info),
	0x00
};

static struct sensor_raw_info s_ov5640_raw_info={
	&s_ov5640_version_info,
	&s_ov5640_tune_info,
	&s_ov5640_fix_info,
	0
};

struct sensor_raw_info* s_ov5640_raw_info_ptr=&s_ov5640_raw_info;

LOCAL SENSOR_IOCTL_FUNC_TAB_T s_ov5640_ioctl_func_tab = {
	PNULL,
	_ov5640_PowerOn,
	PNULL,
	_ov5640_Identify,

	PNULL,			// write register
	PNULL,			// read  register
	PNULL,
	_ov5640_GetResolutionTrimTab,

	// External
	PNULL,
	PNULL,
	PNULL,

	PNULL,
	PNULL,
	PNULL,
	PNULL,			//_ov5640_set_saturation,

	PNULL,
	PNULL,

	_ov5640_BeforeSnapshot,
	_ov5640_after_snapshot,
	PNULL,
	PNULL,
	_ov5640_write_exposure,
	PNULL,
	_ov5640_write_gain,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	_ov5640_write_af,
	PNULL, //_ov5640_set_anti_flicker,
	PNULL, //_ov5640_set_video_mode,
	PNULL,
	PNULL, //meter_mode
	PNULL, //get_status
	PNULL,
	PNULL,
};

SENSOR_INFO_T g_ov5640_raw_info = {
	ov5640_I2C_ADDR_W, // salve i2c write address
	ov5640_I2C_ADDR_R,// salve i2c read address

	SENSOR_I2C_REG_16BIT | SENSOR_I2C_REG_8BIT,	// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
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
	SENSOR_IMAGE_EFFECT_NEGATIVE |
	SENSOR_IMAGE_EFFECT_CANVAS,

	// while balance mode
	0,

	7,			// bit[0:7]: count of step in brightness, contrast, sharpness, saturation
	// bit[8:31] reseved

	SENSOR_LOW_PULSE_RESET,	// reset pulse level
	50,			// reset pulse width(ms)

	SENSOR_HIGH_LEVEL_PWDN,	// 1: high level valid; 0: low level valid

	1,			// count of identify code
	{{0x0A, 0x56},		// supply two code to identify sensor.
	 {0x0B, 0x40}},		// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,	// voltage of avdd

	640,			// max width of source image
	480,			// max height of source image
	"ov5640",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_B,// pattern of input image form sensor;

	s_ov5640_resolution_Tab_RAW,	// point to resolution table information structure
	&s_ov5640_ioctl_func_tab,	// point to ioctl function table
	&s_ov5640_raw_info,		// information and table about Rawrgb sensor
	NULL,			//&g_ov5640_ext_info,                // extend information about sensor
	SENSOR_AVDD_2800MV,	// iovdd
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
	{SENSOR_INTERFACE_TYPE_CCIR601, 8, 8, 1},
	PNULL,
	3,			// skip frame num while change setting
};

LOCAL struct sensor_raw_info* Sensor_GetContext(void)
{
	return s_ov5640_raw_info_ptr;
}


LOCAL uint32_t Sensor_InitRawTuneInfo(void)
{
	uint32_t rtn=0x00;

	return rtn;
}

LOCAL uint32_t _ov5640_GetResolutionTrimTab(uint32_t param)
{
	return (uint32_t) s_ov5640_Resolution_Trim_Tab;
}
LOCAL uint32_t _ov5640_PowerOn(uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_ov5640_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_ov5640_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_ov5640_raw_info.iovdd_val;
	BOOLEAN power_down = g_ov5640_raw_info.power_down_level;
	BOOLEAN reset_level = g_ov5640_raw_info.reset_pulse_level;
	//uint32_t reset_width=g_ov5640_yuv_info.reset_pulse_width;

	if (SENSOR_TRUE == power_on) {
		Sensor_PowerDown(power_down);
		// Open power
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		usleep(20*1000);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(10*1000);
		Sensor_PowerDown(!power_down);
		// Reset sensor
		Sensor_Reset(reset_level);
	} else {
		Sensor_PowerDown(power_down);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED,
				  SENSOR_AVDD_CLOSED);
	}
	SENSOR_PRINT("SENSOR: _ov5640_Power_On(1:on, 0:off): %d \n", power_on);
	return SENSOR_SUCCESS;
}

LOCAL uint32_t _ov5640_Identify(uint32_t param)
{
#define ov5640_PID_VALUE    0x56
#define ov5640_PID_ADDR     0x300A
#define ov5640_VER_VALUE    0x40
#define ov5640_VER_ADDR     0x300B

	uint8_t pid_value = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("SENSOR:ov5640 identify  .\n");

	pid_value = Sensor_ReadReg(ov5640_PID_ADDR);

	if (ov5640_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(ov5640_VER_ADDR);
		SENSOR_PRINT("SENSOR: ov5640_Identify: PID = %x, VER = %x \n",
			     pid_value, ver_value);
		if (ov5640_VER_VALUE == ver_value) {
			Sensor_InitRawTuneInfo();
			ret_value = SENSOR_SUCCESS;
			SENSOR_PRINT("SENSOR: this is ov5640 sensor ! \n");
		} else {
			SENSOR_PRINT
			    ("SENSOR: ov5640_Identify this is OV%x%x sensor ! \n",
			     pid_value, ver_value);
		}
	} else {
		SENSOR_PRINT_ERR("SENSOR:ov5640 identify fail,pid_value=%d .\n",
			     pid_value);
	}

	return ret_value;
}

LOCAL uint32_t _ov5640_write_exposure(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t expsure_line=0x00;
	uint16_t dummy_line=0x00;
	uint16_t value=0x00;

	expsure_line=(param>>0x10)&0xffff;
	dummy_line=param&0xffff;

	SENSOR_PRINT("ISP_RAW:SENSOR:_ov5640_write_exposure %d, %d\n", expsure_line, dummy_line);

	value=(expsure_line<<0x04)&0xff;
	ret_value = Sensor_WriteReg(0x3502, 0x01);
	value=(expsure_line>>0x04)&0xff;
	ret_value = Sensor_WriteReg(0x3501, 0x01);
	value=(expsure_line>>0x0c)&0x0f;
	ret_value = Sensor_WriteReg(0x3500, 0x01);

	return ret_value;
}

LOCAL uint32_t _ov5640_write_gain(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t value=0x00;

	SENSOR_PRINT("ISP_RAW:SENSOR:_ov5640_write_gain\n");

	value = param&0xff;
	ret_value = Sensor_WriteReg(0x350b, value);//0-7
	value = (param>>0x08)&0x01;
	ret_value = Sensor_WriteReg(0x350a, value);//8


	return ret_value;
}

LOCAL uint32_t _ov5640_write_af(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;

	SENSOR_PRINT("ISP_RAW:SENSOR:_ov5640_write_af\n");

	ret_value = Sensor_WriteReg(0x3406, 0x01);
	ret_value = Sensor_WriteReg(0x3503, 0x07);

	return ret_value;
}
LOCAL uint32_t _ov5640_BeforeSnapshot(uint32_t param)
{
#define Capture_Framerate  150
#define g_Preview_FrameRate 300

	uint8_t ExposureLow, ExposureMid, ExposureHigh;
	uint8_t ret_l, ret_m, ret_h, Gain, Lines_10ms;
	uint16_t ulCapture_Exposure, Preview_Maxlines;
	uint32_t Capture_MaxLines, g_preview_exposure;
	uint32_t cap_mode = (param>>CAP_MODE_BITS);

	param = param&0xffff;
	SENSOR_PRINT("%d,%d.",cap_mode,param);

	if (SENSOR_MODE_PREVIEW_ONE >= param) {
		return SENSOR_SUCCESS;
	}
	Sensor_WriteReg(0x3406, 0x01);
	Sensor_WriteReg(0x3503, 0x07);

	ret_h = ret_m = ret_l = 0;
	g_preview_exposure = 0;
	ret_h = (uint8_t) Sensor_ReadReg(0x3500);
	ret_m = (uint8_t) Sensor_ReadReg(0x3501);
	ret_l = (uint8_t) Sensor_ReadReg(0x3502);
	g_preview_exposure = (ret_h << 12) + (ret_m << 4) + (ret_l >> 4);

	ret_h = ret_m = ret_l = 0;
	Preview_Maxlines = 0;
	ret_h = (uint8_t) Sensor_ReadReg(0x380e);
	ret_l = (uint8_t) Sensor_ReadReg(0x380f);
	Preview_Maxlines = (ret_h << 8) + ret_l;
	//Read back AGC Gain for preview
	Gain = (uint8_t) Sensor_ReadReg(0x350b);
	Sensor_SetMode(param);

	ret_h = ret_m = ret_l = 0;
	ret_h = (uint8_t) Sensor_ReadReg(0x380e);
	ret_l = (uint8_t) Sensor_ReadReg(0x380f);
	Capture_MaxLines = (ret_h << 8) + ret_l;

	Lines_10ms = (uint8_t) (Capture_Framerate * Capture_MaxLines / 12000);

	if (Preview_Maxlines == 0) {
		Preview_Maxlines = 1;
	}

	ulCapture_Exposure =
	    (g_preview_exposure * (Capture_Framerate) * (Capture_MaxLines)) /
	    (((Preview_Maxlines) * (g_Preview_FrameRate)));

	ulCapture_Exposure = g_preview_exposure;

	if (Gain > 32) {
		Gain = Gain / 2;
		ulCapture_Exposure = 2 * g_preview_exposure;
	}

	ExposureLow = ((unsigned char)ulCapture_Exposure) << 4;
	ExposureMid = (unsigned char)(ulCapture_Exposure >> 4) & 0xff;
	ExposureHigh = (unsigned char)(ulCapture_Exposure >> 12);

	//m_iWrite0x3502=ExposureLow;
	Sensor_WriteReg(0x3502, ExposureLow);
	//m_iWrite0x3501=ExposureMid;
	Sensor_WriteReg(0x3501, ExposureMid);
	//m_iWrite0x3500=ExposureHigh;
	Sensor_WriteReg(0x3500, ExposureHigh);
	//m_iWrite0x350b=Gain;
	Sensor_WriteReg(0x350b, Gain + 1);
	usleep(100*1000);
	Sensor_WriteReg(0x350b, Gain);
	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME,
				 (uint32_t) ulCapture_Exposure);
	return SENSOR_SUCCESS;
}

LOCAL uint32_t _ov5640_after_snapshot(uint32_t param)
{
	SENSOR_PRINT("SENSOR: _ov5640_after_snapshot =%d \n", param);
	Sensor_SetMode(param);
	return SENSOR_SUCCESS;
}

