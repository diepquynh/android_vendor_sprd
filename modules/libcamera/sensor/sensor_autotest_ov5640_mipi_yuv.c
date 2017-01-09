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

#define ov5640_I2C_ADDR_W        0x3c
#define ov5640_I2C_ADDR_R         0x3c

#define CMD_MAIN 0x3022
#define CMD_ACK 0x3023
#define CMD_PARAM0 0x3024
#define CMD_PARAM1 0x3025
#define CMD_PARAM2 0x3026
#define CMD_PARAM3 0x3027
#define CMD_PARAM4 0x3028

LOCAL uint32_t s_is_streamoff = 0;
LOCAL unsigned long _at_ov5640_GetResolutionTrimTab(unsigned long param);
LOCAL unsigned long _at_ov5640_PowerOn(unsigned long power_on);
LOCAL unsigned long _at_ov5640_Identify(unsigned long param);
LOCAL unsigned long _at_ov5640_StreamOn(unsigned long param);
LOCAL unsigned long _at_ov5640_StreamOff(unsigned long param);

LOCAL const SENSOR_REG_T at_ov5640_common_init[] = {
	{0x3103, 0x11},		/* sysclk from pad*/
	{0x3008, 0x82},		/*software reset*/
	{SENSOR_WRITE_DELAY, 0x0a},
	{0x3008, 0x42},		/*software power down*/
	{0x3103, 0x03},		/*sysclk from pll*/
	{0x3017, 0x00},		/*Frex, Vsync, Href, PCLK, D[9:6] output*/
	{0x3018, 0x00},		/*D[5:0], GPIO[1:0] output*/
	{0x3034, 0x18},
	{0x3035, 0x14},		/*0x11-->30fps; 0x21-->15fps*/
	{0x3036, 0x38},
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
	{0x3c01, 0x34},		/* sum auto, band counter enable, threshold = 4*/
	{0x3c04, 0x28},		/* threshold low sum*/
	{0x3c05, 0x98},		/* threshold high sum*/
	{0x3c06, 0x00},		/* light meter 1 threshold H*/
	{0x3c07, 0x08},		/* light meter 1 threshold L*/
	{0x3c08, 0x00},		/* light meter 2 threshold H*/
	{0x3c09, 0x1c},		/* light meter 2 threshold L*/
	{0x3c0a, 0x9c},		/* sample number H*/
	{0x3c0b, 0x40},		/* sample number L*/
	{0x3820, 0x41},
	{0x3821, 0x07},
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
	{0x3808, 0x02},
	{0x3809, 0x80},
	{0x380a, 0x01},
	{0x380b, 0xe0},
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
	{0x4005,0x1a},
	{0x3000,0x00},
	{0x3002,0x1c},
	{0x3004,0xff},
	{0x3006,0xc3},
	{0x300e,0x45},
	{0x302e,0x08},
	{0x4300,0x30},
	{0x501f,0x00},
	{0x4713,0x03},
	{0x4407,0x04},
	{0x440e,0x00},
	{0x460b,0x35},
	{0x460c,0x22},
	{0x4837,0x44},
	{0x3824,0x02},
	{0x5000,0xa7},
	{0x5001,0xa3},
	{0x5180,0xff},
	{0x5181,0xf2},
	{0x5182,0x00},
	{0x5183,0x14},
	{0x5184,0x25},
	{0x5185,0x24},
	{0x5186,0x09},
	{0x5187,0x09},
	{0x5188,0x09},
	{0x5189,0x75},
	{0x518a,0x54},
	{0x518b,0xe0},
	{0x518c,0xb2},
	{0x518d,0x42},
	{0x518e,0x3d},
	{0x518f,0x56},
	{0x5190,0x46},
	{0x5191,0xf8},
	{0x5192,0x04},
	{0x5193,0x70},
	{0x5194,0xf0},
	{0x5195,0xf0},
	{0x5196,0x03},
	{0x5197,0x01},
	{0x5198,0x04},
	{0x5199,0x12},
	{0x519a,0x04},
	{0x519b,0x00},
	{0x519c,0x06},
	{0x519d,0x82},
	{0x519e,0x38},

	{0x5025,0x00},

	{0x3034,0x18},
	{0x3035,0x14},
	{0x3036,0x38},
	{0x3037,0x13},
	{0x4837,0x0a},

	//update iq setting davis 20120105
	//AEC target
	{0x3a0f, 0x30}, //stable in high
	{0x3a10, 0x28}, //stable in low
	{0x3a1b, 0x30}, //stable out high
	{0x3a1e, 0x26}, //stable out low
	{0x3a11, 0x60}, //fast zone high
	{0x3a1f, 0x14}, //fast zone low

	//sp-tigher-lenc-cus
	{0x5800, 0x29},
	{0x5801, 0x1a},
	{0x5802, 0x12},
	{0x5803, 0x12},
	{0x5804, 0x19},
	{0x5805, 0x27},
	{0x5806, 0x12},
	{0x5807, 0x09},
	{0x5808, 0x06},
	{0x5809, 0x06},
	{0x580a, 0x09},
	{0x580b, 0x0f},
	{0x580c, 0x0a},
	{0x580d, 0x04},
	{0x580e, 0x00},
	{0x580f, 0x00},
	{0x5810, 0x03},
	{0x5811, 0x09},
	{0x5812, 0x0a},
	{0x5813, 0x04},
	{0x5814, 0x00},
	{0x5815, 0x00},
	{0x5816, 0x03},
	{0x5817, 0x09},
	{0x5818, 0x12},
	{0x5819, 0x09},
	{0x581a, 0x06},
	{0x581b, 0x06},
	{0x581c, 0x09},
	{0x581d, 0x10},
	{0x581e, 0x2c},
	{0x581f, 0x1a},
	{0x5820, 0x13},
	{0x5821, 0x12},
	{0x5822, 0x18},
	{0x5823, 0x26},
	{0x5824, 0x4a},
	{0x5825, 0x48},
	{0x5826, 0x2a},
	{0x5827, 0x4a},
	{0x5828, 0x4a},
	{0x5829, 0x46},
	{0x582a, 0x44},
	{0x582b, 0x44},
	{0x582c, 0x44},
	{0x582d, 0x28},
	{0x582e, 0x46},
	{0x582f, 0x62},
	{0x5830, 0x60},
	{0x5831, 0x62},
	{0x5832, 0x26},
	{0x5833, 0x46},
	{0x5834, 0x44},
	{0x5835, 0x42},
	{0x5836, 0x44},
	{0x5837, 0x28},
	{0x5838, 0x48},
	{0x5839, 0x28},
	{0x583a, 0x0a},
	{0x583b, 0x2a},
	{0x583c, 0x2a},
	{0x583d, 0xae},

	//sp-tiger-awb-1
	{0x5180, 0xff},
	{0x5181, 0xf2},
	{0x5182, 0x00},
	{0x5183, 0x14},
	{0x5184, 0x25},
	{0x5185, 0x24},
	{0x5186, 0x11},
	{0x5187, 0x18},
	{0x5188, 0x1a},
	{0x5189, 0x8b},
	{0x518a, 0x6a},
	{0x518b, 0x9c},
	{0x518c, 0x96},
	{0x518d, 0x3f},
	{0x518e, 0x2d},
	{0x518f, 0x53},
	{0x5190, 0x47},
	{0x5191, 0xf8},
	{0x5192, 0x04},
	{0x5193, 0xf0},
	{0x5194, 0xf0},
	{0x5195, 0xf0},
	{0x5196, 0x03},
	{0x5197, 0x01},
	{0x5198, 0x07},
	{0x5199, 0xa9},
	{0x519a, 0x04},
	{0x519b, 0x00},
	{0x519c, 0x04},
	{0x519d, 0x8b},
	{0x519e, 0x38},

	//Gamma
	{0x5480, 0x01}, //BIAS plus on
	{0x5481, 0x08},
	{0x5482, 0x14},
	{0x5483, 0x28},
	{0x5484, 0x51},
	{0x5485, 0x65},
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
	{0x5490, 0x1d},

	//color matrix
	{0x5381, 0x1a}, //CMX1 for Y
	{0x5382, 0x60}, //CMX2 for Y
	{0x5383, 0x07}, //CMX3 for Y
	{0x5384, 0x08}, //CMX4 for U
	{0x5385, 0x7a}, //CMX5 for U
	{0x5386, 0x82}, //CMX6 for U
	{0x5387, 0x76}, //CMX7 for V
	{0x5388, 0x67}, //CMX8 for V
	{0x5389, 0x0f}, //CMX9 for V
	{0x538a, 0x01}, //sign[9]
	{0x538b, 0x98}, //sign[8:1]

	//UV adjust
	{0x5580, 0x02},
	{0x5583, 0x40},
	{0x5584, 0x10},
	{0x5589, 0x10},
	{0x558a, 0x00},
	{0x558b, 0xf8},
	{0x501d, 0x40}, //enable manual offset in contrast

	//CIP
	{0x5300, 0x08}, //sharpen-MT th1
	{0x5301, 0x30}, //sharpen-MT th2
	{0x5302, 0x16}, //sharpen-MT off1
	{0x5303, 0x05}, //sharpen-MT off2
	{0x5304, 0x08}, //De-noise th1
	{0x5305, 0x30}, //De-noise th2
	{0x5306, 0x08}, //De-noise off1
	{0x5307, 0x16}, //De-noise off2
	{0x5309, 0x08}, //sharpen-TH th1
	{0x530a, 0x30}, //sharpen-TH th2
	{0x530b, 0x04}, //sharpen-TH off1
	{0x530c, 0x06} //sharpen-TH off2
};

LOCAL const SENSOR_REG_T at_ov5640_640x480_setting[] = {
	/*@@YUV_1280*960_15fps*/
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x04},
	{0x3804, 0x0a},
	{0x3805, 0x3f},
	{0x3806, 0x07},
	{0x3807, 0x9b},
	{0x3808, 0x02},
	{0x3809, 0x80},
	{0x380a, 0x01},
	{0x380b, 0xe0},
	{0x380c, 0x07},
	{0x380d, 0x68},
	{0x380e, 0x03},
	{0x380f, 0xd8},
	{0x3810, 0x00},
	{0x3811, 0x10},
	{0x3812, 0x00},
	{0x3813, 0x06},
	{0x3820, 0x41},	// flip
	{0x3821, 0x07},	// mirror
	{0x3814, 0x31},	// timing X inc
	{0x3815, 0x31},	// timing Y inc
	{0x3618, 0x00},
	{0x3612, 0x29},
	{0x3708, 0x64},
	{0x3709, 0x52},
	{0x370c, 0x03},
	{0x3a02, 0x03},
	{0x3a03, 0xd8},
	{0x3a14, 0x03},
	{0x3a15, 0xd8},
	{0x4001, 0x02},
	{0x4004, 0x02},
	{0x3000, 0x00},
	{0x3002, 0x1c},
	{0x3004, 0xff},
	{0x3006, 0xc3},
	{0x4713, 0x03},
	{0x4407, 0x04},
	{0x440e, 0x00},
	{0x460b, 0x35},

	{0x5001, 0xa7},	// SDE on, CMX on, AWB on, scale off

	{0x3c07, 0x08},	// lightmeter 1 threshold[7:0]

	/*pll*/
	{0x3034,0x18},
	{0x3035,0x14},
	{0x3036,0x38},
	{0x3037,0x13},
	{0x4837,0x0a},
	{0x3108, 0x01},

	 	 //yuv
	{0x3002,0x1c},
	{0x3006,0xc3},
	{0x3821,0x06},
	{0x501f,0x00},
	{0x460c,0x22},
	{0x3824,0x02},
	{0x460b,0x37},

#ifdef CONFIG_ARCH_SC8825
	{0x3503,0x00},	// AEC/AGC on
#endif
	{0x503d,0x80},/*color bar*/

};

LOCAL SENSOR_REG_TAB_INFO_T s_at_ov5640_resolution_Tab_YUV[] = {
	{ADDR_AND_LEN_OF_ARRAY(at_ov5640_common_init), 640, 480, 24,SENSOR_IMAGE_FORMAT_YUV422},
	{ADDR_AND_LEN_OF_ARRAY(at_ov5640_640x480_setting), 640, 480, 24, SENSOR_IMAGE_FORMAT_YUV422},
	{PNULL, 0, 0, 0, 0, 0},

};

LOCAL SENSOR_TRIM_T s_at_ov5640_Resolution_Trim_Tab[] = {
	{0, 0, 640, 480, 0, 900, 0,{0, 0, 640, 480}},
	{0, 0, 640, 480, 0, 900, 0,{0, 0, 640, 480}},
	{0, 0, 0, 0, 0, 0, 0,{0, 0, 0, 0}},

};

LOCAL SENSOR_IOCTL_FUNC_TAB_T s_at_ov5640_ioctl_func_tab = {
	PNULL,
	_at_ov5640_PowerOn,
	PNULL,
	_at_ov5640_Identify,
	PNULL,			// write register
	PNULL,			// read  register
	PNULL,
	_at_ov5640_GetResolutionTrimTab,
	// External
	PNULL,
	PNULL,
	PNULL,
	PNULL, //_at_ov5640_set_brightness,
	PNULL, //_at_ov5640_set_contrast,
	PNULL,
	PNULL,			//_at_ov5640_set_saturation,

	PNULL, //_at_ov5640_set_work_mode,
	PNULL, //_at_ov5640_set_image_effect,

	PNULL, //_at_ov5640_BeforeSnapshot,
	PNULL, //_at_ov5640_after_snapshot,
	PNULL, //_at_ov5640_flash,/*_ov540_flash,*/
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL, //_at_ov5640_set_awb,
	PNULL,
	PNULL, //_at_ov5640_set_iso,
	PNULL, //_at_ov5640_set_ev,
	PNULL, //_at_ov5640_check_image_format_support,
	PNULL,
	PNULL,
	PNULL, //_at_ov5640_GetExifInfo,
	PNULL, //_at_ov5640_ExtFunc,
	PNULL, //_at_ov5640_set_anti_flicker,
	PNULL, //_at_ov5640_set_video_mode,
	PNULL, //_at_ov5640_pick_out_jpeg_stream,
	PNULL, //meter_mode
	PNULL, //get_status
	_at_ov5640_StreamOn,
	_at_ov5640_StreamOff,
	PNULL,
};

SENSOR_INFO_T g_autotest_ov5640_mipi_yuv_info = {
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

	s_at_ov5640_resolution_Tab_YUV,	// point to resolution table information structure
	&s_at_ov5640_ioctl_func_tab,	// point to ioctl function table
	NULL,		// information and table about Rawrgb sensor
	NULL,			//&g_at_ov5640_ext_info,                // extend information about sensor
	SENSOR_AVDD_1800MV,	// iovdd
	SENSOR_AVDD_1500MV,	// dvdd
	3,			// skip frame num before preview
	1,			// skip frame num before capture
	0,			// deci frame num during preview
	0,			// deci frame num during video preview

	0,
	0,
	0,
	0,
	0,
	{SENSOR_INTERFACE_TYPE_CSI2, 2, 8, 1},
	PNULL,
	3,			// skip frame num while change setting
};

LOCAL unsigned long _at_ov5640_GetResolutionTrimTab(unsigned long param)
{
	return (unsigned long) s_at_ov5640_Resolution_Trim_Tab;
}

LOCAL unsigned long _at_ov5640_PowerOn(unsigned long power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_autotest_ov5640_mipi_yuv_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_autotest_ov5640_mipi_yuv_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_autotest_ov5640_mipi_yuv_info.iovdd_val;
	BOOLEAN power_down = g_autotest_ov5640_mipi_yuv_info.power_down_level;
	BOOLEAN reset_level = g_autotest_ov5640_mipi_yuv_info.reset_pulse_level;
	//uint32_t reset_width=g_ov5640_yuv_info.reset_pulse_width;

	if (SENSOR_TRUE == power_on) {
		//reset
		Sensor_SetResetLevel(reset_level);
		Sensor_PowerDown(power_down);
		// Open power
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_3000MV);
		usleep(10*1000);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(10*1000);
		Sensor_PowerDown(!power_down);
		usleep(10*1000);
		// Reset sensor
		Sensor_SetResetLevel(!reset_level);
		usleep(20*1000);
	} else {
		Sensor_PowerDown(power_down);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED,
				  SENSOR_AVDD_CLOSED);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
	}
	SENSOR_PRINT("(1:on, 0:off): %ld", power_on);
	return SENSOR_SUCCESS;
}

LOCAL unsigned long _at_ov5640_Identify(unsigned long param)
{
	#define ov5640_PID_VALUE    0x56
	#define ov5640_PID_ADDR     0x300A
	#define ov5640_VER_VALUE    0x40
	#define ov5640_VER_ADDR     0x300B

	uint8_t pid_value = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("Start");

	pid_value = Sensor_ReadReg(ov5640_PID_ADDR);

	if (ov5640_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(ov5640_VER_ADDR);
		SENSOR_PRINT("PID = %x, VER = %x",
			     pid_value, ver_value);
		if (ov5640_VER_VALUE == ver_value) {
			ret_value = SENSOR_SUCCESS;
			SENSOR_PRINT("this is ov5640 yuv mipi sensor !");
		} else {
			SENSOR_PRINT("this is OV%x%x sensor !", pid_value, ver_value);
		}
	} else {
		SENSOR_PRINT("fail,pid_value=%d", pid_value);
	}
	return ret_value;
}

LOCAL unsigned long _at_ov5640_StreamOn(unsigned long param)
{
	SENSOR_PRINT("Start");

	Sensor_WriteReg(0x3008, 0x02);
	s_is_streamoff = 0;

	return 0;
}

LOCAL unsigned long _at_ov5640_StreamOff(unsigned long param)
{
	SENSOR_PRINT("Stop");

	Sensor_WriteReg(0x3008, 0x42);
	s_is_streamoff = 1;

	return 0;
}
