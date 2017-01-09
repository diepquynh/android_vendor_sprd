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
#define LOG_TAG __FILE__
#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "cmr_oem.h"

#define ov5640_I2C_ADDR_W        0x3c
#define ov5640_I2C_ADDR_R         0x3c

#define FOCUS_ZONE_W 80
#define FOCUS_ZONE_H 60
#define AUTOFOCUS_TIMEOUT (280)

#define FOCUS_MOVE_GAIN_CHECK 10000
#ifndef ABS
#define ABS(x)  (((x) < 0) ? (-(x)):(x))
#endif

#define EXPOSURE_ZONE_W 1280
#define EXPOSURE_ZONE_H 960

#define CMD_MAIN 0x3022
#define CMD_ACK 0x3023
#define CMD_PARAM0 0x3024
#define CMD_PARAM1 0x3025
#define CMD_PARAM2 0x3026
#define CMD_PARAM3 0x3027
#define CMD_PARAM4 0x3028

LOCAL unsigned long _at_ov5640_ccir_GetResolutionTrimTab(unsigned long param);
LOCAL unsigned long _at_ov5640_ccir_PowerOn(unsigned long power_on);
LOCAL unsigned long _at_ov5640_ccir_Identify(unsigned long param);

LOCAL const SENSOR_REG_T _at_ov5640_ccir_common_init[] = {
	{0x4202, 0x0f},/*kenxu add 20120207 for stream off*/
	{SENSOR_WRITE_DELAY, 0x10},
	{0x3103, 0x11},		/* sysclk from pad*/
	{0x3008, 0x82},		/*software reset*/
	{SENSOR_WRITE_DELAY, 0x20},
	{0x3008, 0x42},		/*software power down*/
	{0x3103, 0x03},		/*sysclk from pll*/
	{0x3017, 0xff},		/*Frex, Vsync, Href, PCLK, D[9:6] output*/
	{0x3018, 0xff},		/*D[5:0], GPIO[1:0] output*/
	/*{0x3031, 0x08}, *//*kenxu add 20120201 for external 1.5v dvdd @ 2.8v DOVDD*/
	{0x3034, 0x1a},
	{0x3035, 0x11},		/*0x11-->30fps; 0x21-->15fps*/
	{0x3036, 0x46},
	{0x3037, 0x13},
	{0x3108, 0x01},		/* clock divider*/
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
	{0x3600, 0x08},		/*VCM debug*/
	{0x3601, 0x33},		/*VCM debug*/
	{0x302d, 0x60},		/*system control*/
	{0x3620, 0x52},
	{0x371b, 0x20},
	{0x471c, 0x50},
	{0x3a13, 0x43},		/*pre-gain = 1.05x*/

	{0x3a18, 0x00},		/*gain ceiling*/
	{0x3a19, 0xf8},
	{0x3635, 0x13},
	{0x3636, 0x03},
	{0x3634, 0x40},
	{0x3622, 0x01},

	/*50/60Hz detection*/
	{0x3c01, 0x34},		/* sum auto, band counter enable, threshold = 4*/
	{0x3c04, 0x28},		/* threshold low sum*/
	{0x3c05, 0x98},		/* threshold high sum*/
	{0x3c06, 0x00},		/* light meter 1 threshold H*/
	{0x3c07, 0x07},		/* light meter 1 threshold L*/
	{0x3c08, 0x00},		/* light meter 2 threshold H*/
	{0x3c09, 0x1c},		/* light meter 2 threshold L*/
	{0x3c0a, 0x9c},		/* sample number H*/
	{0x3c0b, 0x40},		/* sample number L*/
	{0x3810, 0x00},		/* X offset*/
	{0x3811, 0x10},		/* X offset*/
	{0x3812, 0x00},		/* Y offset*/
	{0x3708, 0x64},
	{0x4001, 0x02},		/* BLC start line*/
	{0x4005, 0x1a},		/* BLC always update*/
	{0x3000, 0x00},		/* enable MCU, OTP*/
	{0x3004, 0xff},		/* enable BIST, MCU memory, MCU, OTP, STROBE, D5060, timing, array clock*/
	{0x300e, 0x58},		/* MIPI 2 lane? power down PHY HS TX, PHY LP RX, DVP enable*/
	{0x302e, 0x00},
	{0x4300, 0x30},		/* YUV 422, YUYV*/
	{0x501f, 0x00},		/* ISP YUV 422*/
	{0x440e, 0x00},
	{0x5000, 0xa7},		/* LENC on, raw gamma on, BPC on, WPC on, CIP on*/

	/*AEC target*/

	{0x3a0f, 0x38},		/*ae target*/
	{0x3a10, 0x30},
	{0x3a11, 0x61},
	{0x3a1b, 0x38},
	{0x3a1e, 0x30},
	{0x3a1f, 0x10},

	/*LENC*/

	{0x5800, 0x34},		/*lenc*/
	{0x5801, 0x17},
	{0x5802, 0x11},
	{0x5803, 0x11},
	{0x5804, 0x18},
	{0x5805, 0x35},
	{0x5806, 0x0E},
	{0x5807, 0x09},
	{0x5808, 0x06},
	{0x5809, 0x06},
	{0x580A, 0x08},
	{0x580B, 0x0E},
	{0x580C, 0x0A},
	{0x580D, 0x04},
	{0x580E, 0x00},
	{0x580F, 0x00},
	{0x5810, 0x04},
	{0x5811, 0x09},
	{0x5812, 0x0A},
	{0x5813, 0x04},
	{0x5814, 0x00},
	{0x5815, 0x00},
	{0x5816, 0x04},
	{0x5817, 0x09},
	{0x5818, 0x0D},
	{0x5819, 0x09},
	{0x581A, 0x07},
	{0x581B, 0x06},
	{0x581C, 0x08},
	{0x581D, 0x0D},
	{0x581E, 0x33},
	{0x581F, 0x16},
	{0x5820, 0x11},
	{0x5821, 0x10},
	{0x5822, 0x16},
	{0x5823, 0x34},
	{0x5824, 0x58},
	{0x5825, 0x38},
	{0x5826, 0x18},
	{0x5827, 0x38},
	{0x5828, 0x48},
	{0x5829, 0x29},
	{0x582A, 0x25},
	{0x582B, 0x25},
	{0x582C, 0x24},
	{0x582D, 0x27},
	{0x582E, 0x06},
	{0x582F, 0x42},
	{0x5830, 0x50},
	{0x5831, 0x42},
	{0x5832, 0x05},
	{0x5833, 0x17},
	{0x5834, 0x14},
	{0x5835, 0x13},
	{0x5836, 0x14},
	{0x5837, 0x16},
	{0x5838, 0x68},
	{0x5839, 0x38},
	{0x583A, 0x17},
	{0x583B, 0x37},
	{0x583C, 0x68},
	{0x583D, 0xCF},

	/*AWB*/

	{0x5180, 0xff},		/*awb*/
	{0x5181, 0xf2},
	{0x5182, 0x0},
	{0x5183, 0x14},
	{0x5184, 0x25},
	{0x5185, 0x24},
	{0x5186, 0x0e},
	{0x5187, 0x18},
	{0x5188, 0x1a},
	{0x5189, 0x7e},
	{0x518a, 0x5c},
	{0x518b, 0xda},
	{0x518c, 0xa3},
	{0x518d, 0x3f},
	{0x518e, 0x2d},
	{0x518f, 0x53},
	{0x5190, 0x45},
	{0x5191, 0xf8},
	{0x5192, 0x4},
	{0x5193, 0xf0},
	{0x5194, 0xf0},
	{0x5195, 0xf0},
	{0x5196, 0x3},
	{0x5197, 0x1},
	{0x5198, 0x4},
	{0x5199, 0x21},
	{0x519a, 0x4},
	{0x519b, 0x0},
	{0x519c, 0x8},
	{0x519d, 0x9e},
	{0x519e, 0x38},

	/*Gamma*/

	{0x5490, 0x1d},
	{0x5481, 0x5},
	{0x5482, 0x10},
	{0x5483, 0x25},
	{0x5484, 0x4f},
	{0x5485, 0x64},
	{0x5486, 0x71},
	{0x5487, 0x7d},
	{0x5488, 0x87},
	{0x5489, 0x91},
	{0x548a, 0x9a},
	{0x548b, 0xaa},
	{0x548c, 0xb8},
	{0x548d, 0xcd},
	{0x548e, 0xdd},
	{0x548f, 0xea},

	/*color matrix*/

	{0x5381, 0x1c},
	{0x5382, 0x5a},
	{0x5383, 0x6},
	{0x5384, 0x8},
	{0x5385, 0x65},
	{0x5386, 0x6d},
	{0x5387, 0x7c},
	{0x5388, 0x6c},
	{0x5389, 0x10},
	{0x538b, 0x98},
	{0x538a, 0x1},

	/*UV adjust*/

	/* Mini Q, Auto UV, Sharpness/De-noise*/
	{0x583e, 0x20},		/*max gain 2x, 0x20/2=16x*/
	{0x583f, 0x10},		/*min gain 2x, 0x10/2=8x*/
	{0x5840, 0x00},		/*mini Q, down from 0x40 to 00*/

	/* UV Adjust Auto Mode*/
	{0x5580, 0x02},		/*Sat enable*/
	{0x5588, 0x01},		/*enable UV adj*/
	{0x5583, 0x40},		/*offset high*/
	{0x5584, 0x20},		/*offset low*/
	{0x5589, 0x40},		/*gth1 8x*/
	{0x558a, 0x00},
	{0x358b, 0x80},		/*gth2 16x*/

	/* Sharpness Auto*/
	{0x5308, 0x25},
	{0x5300, 0x08},
	{0x5301, 0x20},
	{0x5302, 0x20},
	{0x5303, 0x00},
	{0x5309, 0x08},
	{0x530a, 0x20},
	{0x530b, 0x04},
	{0x530c, 0x06},

	/* De-Noise Auto*/
	{0x5304, 0x08},
	{0x5305, 0x20},
	{0x5306, 0x08},
	{0x5307, 0x16},

	{0x5025, 0x00},

	{0x3008, 0x02},		/*wake up from software power down*/
	{0x4202, 0x00},		/*kenxu add 20120207 for stream on*/
};

LOCAL const SENSOR_REG_T _at_ov5640_ccir_640X480_new[] = {
	{0x4202, 0x0f},		/*kenxu add 20120207 for stream off*/
	{SENSOR_WRITE_DELAY, 0x10},
	{0x3c07, 0x08},		/* lightmeter 1 threshold[7:0]*/
	{0x3820, 0x41},		/* flip*/
	{0x3821, 0x07},		/* mirror*/
	{0x3814, 0x31},		/* timing X inc*/
	{0x3815, 0x31},		/* timing Y inc*/
	{0x3800, 0x00},		/* HS*/
	{0x3801, 0x00},		/* HS*/
	{0x3802, 0x00},		/* VS*/
	{0x3803, 0x04},		/* VS*/
	{0x3804, 0x0a},		/* HW (HE)*/
	{0x3805, 0x3f},		/* HW (HE)*/
	{0x3806, 0x07},		/* VH (VE)*/
	{0x3807, 0x9b},		/* VH (VE)*/
	{0x3808, 0x02},		/* DVPHO*/
	{0x3809, 0x80},		/* DVPHO*/
	{0x380a, 0x01},		/* DVPVO*/
	{0x380b, 0xe0},		/* DVPVO*/
	{0x380c, 0x07},		/* HTS*/
	{0x380d, 0x68},		/* HTS*/
	{0x380e, 0x03},		/* VTS*/
	{0x380f, 0xd8},		/* VTS*/
	{0x3813, 0x06},		/* timing V offset*/
	{0x3618, 0x00},
	{0x3612, 0x29},
	{0x3709, 0x52},
	{0x370c, 0x03},
	{0x3a02, 0x0b},		/* 60Hz max exposure, night mode 5fps*/
	{0x3a03, 0x88},		/* 60Hz max exposure*/
	{0x3a14, 0x0b},		/* 50Hz max exposure, night mode 5fps*/
	{0x3a15, 0x88},		/* 50Hz max exposure*/
	{0x4004, 0x02},		/* BLC line number*/
	{0x3002, 0x1c},		/* reset JFIFO, SFIFO, JPG*/
	{0x3006, 0xc3},		/* disable clock of JPEG2x, JPEG*/
	{0x4713, 0x03},		/* JPEG mode 3*/
	{0x4407, 0x04},		/* Quantization sacle*/
	{0x460b, 0x35},
	{0x460c, 0x22},
	{0x4837, 0x22},		/* MIPI global timing*/
	{0x3824, 0x02},		/* PCLK manual divider*/
	{0x5001, 0xa3},		/* SDE on, CMX on, AWB on*/

	{0x3034, 0x1a},
	{0x3035, 0x11},		/* MIPI global timing*/
	{0x3036, 0x46},		/* PCLK manual divider*/
	{0x3037, 0x13},		/* SDE on, CMX on, AWB on*/
	{0x3108, 0x01}, 	/* clock divider*/

	{0x3406, 0x00},		/*awb auto*/

	{0x3503, 0x00},		/* AEC/AGC on*/
	{0x4202, 0x00},		/*kenxu add 20120207 for stream on*/

	{0x503d,0x80},/*color bar*/
};


LOCAL SENSOR_REG_TAB_INFO_T s_at_ov5640_ccir_resolution_Tab_YUV[] = {
	{ADDR_AND_LEN_OF_ARRAY(_at_ov5640_ccir_common_init), 0, 0, 12, SENSOR_IMAGE_FORMAT_YUV422},
	{ADDR_AND_LEN_OF_ARRAY(_at_ov5640_ccir_640X480_new), 640, 480, 24, SENSOR_IMAGE_FORMAT_YUV422},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

LOCAL SENSOR_TRIM_T s_at_ov5640_ccir_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0, 0,{0, 0, 0, 0}},
	{0, 0, 640, 480, 68, 56, 0,{0, 0, 640, 480}},
	{0, 0, 0, 0, 0, 0, 0,{0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0,{0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0,{0, 0, 0, 0}}
};

LOCAL SENSOR_IOCTL_FUNC_TAB_T s_at_ov5640_ccir_ioctl_func_tab = {
	PNULL,
	_at_ov5640_ccir_PowerOn,
	PNULL,
	_at_ov5640_ccir_Identify,

	PNULL,			// write register
	PNULL,			// read  register
	PNULL,
	_at_ov5640_ccir_GetResolutionTrimTab,

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

	PNULL,
	PNULL,
	PNULL,/*_ov540_flash,*/
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
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,  //meter_mode
	PNULL, //get_status
	PNULL,
	PNULL,
	PNULL,
};

SENSOR_INFO_T g_at_ov5640_ccir_yuv_info = {
	ov5640_I2C_ADDR_W,	// salve i2c write address
	ov5640_I2C_ADDR_R,	// salve i2c read address

	SENSOR_I2C_REG_16BIT | SENSOR_I2C_REG_8BIT | SENSOR_I2C_FREQ_400,	// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
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
	10,			// reset pulse width(ms)

	SENSOR_HIGH_LEVEL_PWDN,	// 1: high level valid; 0: low level valid

	1,			// count of identify code
	{{0x0A, 0x56},		// supply two code to identify sensor.
	{0x0B, 0x40}},		// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,	// voltage of avdd

	2592,			// max width of source image
	1944,			// max height of source image
	"ov5640",		// name of sensor

	SENSOR_IMAGE_FORMAT_MAX,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_YUV422_YUYV,	// pattern of input image form sensor;

	s_at_ov5640_ccir_resolution_Tab_YUV,	// point to resolution table information structure
	&s_at_ov5640_ccir_ioctl_func_tab,	// point to ioctl function table
	0,		// information and table about Rawrgb sensor
	NULL,			//&g_ov5640_ext_info,                // extend information about sensor
	SENSOR_AVDD_1800MV,	// iovdd
	SENSOR_AVDD_1500MV,	// dvdd
	1,			// skip frame num before preview
	1,			// skip frame num before capture
	0,			// deci frame num during preview
	0,			// deci frame num during video preview

	0,
	0,
	0,
	0,
	0,
	{SENSOR_INTERFACE_TYPE_CCIR601, 8, 16, 1},
	NULL,
	1,			// skip frame num while change setting
};

LOCAL unsigned long _at_ov5640_ccir_GetResolutionTrimTab(unsigned long param)
{
		return (unsigned long) s_at_ov5640_ccir_Resolution_Trim_Tab;
}

LOCAL unsigned long _at_ov5640_ccir_PowerOn(unsigned long power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_at_ov5640_ccir_yuv_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_at_ov5640_ccir_yuv_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_at_ov5640_ccir_yuv_info.iovdd_val;
	BOOLEAN power_down = g_at_ov5640_ccir_yuv_info.power_down_level;
	BOOLEAN reset_level = g_at_ov5640_ccir_yuv_info.reset_pulse_level;

	CMR_LOGV("dvdd_val %d,  dvdd_val %d, avdd_val %d, iovdd_val %d",
			power_on,
			dvdd_val,
			avdd_val,
			iovdd_val);
	CMR_LOGV("power_down %d reset_level %d", power_down, reset_level);

	if (SENSOR_TRUE == power_on) {
		//reset
		Sensor_SetResetLevel(reset_level);
		Sensor_PowerDown(power_down);
		usleep(10*1000);
		// Open power
		Sensor_SetIovddVoltage(iovdd_val);
		usleep(1*1000);
		Sensor_SetAvddVoltage(avdd_val);
		usleep(1*1000);
		Sensor_SetDvddVoltage(dvdd_val);
		usleep(5*1000);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		Sensor_PowerDown(!power_down);
		usleep(1*1000);
		Sensor_SetResetLevel(!reset_level);
		usleep(20*1000);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_3000MV);
	} else {
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
		Sensor_SetResetLevel(reset_level);
		usleep(1*1000);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		usleep(1*1000);
		Sensor_SetDvddVoltage(SENSOR_AVDD_CLOSED);
		usleep(1*1000);
		Sensor_SetAvddVoltage(SENSOR_AVDD_CLOSED);
		usleep(1*1000);
		Sensor_SetIovddVoltage(SENSOR_AVDD_CLOSED);
	}
	CMR_LOGV("(1:on, 0:off): %ld_end\n ", power_on);
	return SENSOR_SUCCESS;
}

LOCAL unsigned long _at_ov5640_ccir_Identify(unsigned long param)
{
#define ov5640_PID_VALUE    0x56
#define ov5640_PID_ADDR     0x300A
#define ov5640_VER_VALUE    0x40
#define ov5640_VER_ADDR     0x300B

	uint8_t pid_value = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("SENSOR:ov5640 identify CCIR .\n");

	pid_value = Sensor_ReadReg(ov5640_PID_ADDR);

	if (ov5640_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(ov5640_VER_ADDR);
		SENSOR_PRINT("PID = %x, VER = %x",
			     pid_value, ver_value);
		if (ov5640_VER_VALUE == ver_value) {
			ret_value = SENSOR_SUCCESS;
			SENSOR_PRINT("SENSOR: this is ov5640 yuv ccir sensor  ! \n");
		} else {
			SENSOR_PRINT("this is OV%x%x sensor !", pid_value, ver_value);
		}
	} else {
		SENSOR_PRINT("identify fail,pid_value = %d ", pid_value);
	}

	return ret_value;
}
