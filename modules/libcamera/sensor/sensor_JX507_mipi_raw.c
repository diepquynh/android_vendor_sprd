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
#include "sensor_JX507_mipi_raw_param.c"

#define JX507_I2C_ADDR_W        	0x30  //0x60
#define JX507_I2C_ADDR_R         	0x30

#define DW9714_VCM_SLAVE_ADDR 		(0x18 >> 1)
#define JX507_RAW_PARAM_COM  		0x0000
#define JX507_MAX_SHUTTER_OFFSET 	3

#define JX507_PID_VALUE    0xa5
#define JX507_PID_ADDR     0x0a
#define JX507_VER_VALUE    0x07
#define JX507_VER_ADDR     0x0b

#define JX507_GROUP_WRITE_EN		1//1
#define JX507_FLIP_EN			1//1
#define JX507_MIRROR_EN			1//1
#define JX507_USE_VERTICAL_BINNING_EN 	1//0

static int s_JX507_capture_shutter = 0;
static int s_JX507_capture_VTS = 0;
static int s_JX507_video_min_framerate = 0;
static int s_JX507_video_max_framerate = 0;

LOCAL uint32_t _JX507_GetResolutionTrimTab(uint32_t param);
LOCAL uint32_t _JX507_PowerOn(uint32_t power_on);
LOCAL uint32_t _JX507_Identify(uint32_t param);
LOCAL uint32_t _JX507_BeforeSnapshot(uint32_t param);
LOCAL uint32_t _JX507_after_snapshot(uint32_t param);
LOCAL uint32_t _JX507_StreamOn(uint32_t param);
LOCAL uint32_t _JX507_StreamOff(uint32_t param);
LOCAL uint32_t _JX507_write_exposure(uint32_t param);
LOCAL uint32_t _JX507_write_gain(uint32_t param);
LOCAL uint32_t _JX507_SetEV(uint32_t param);
LOCAL uint32_t _JX507_write_af(uint32_t param);
LOCAL uint32_t _JX507_flash(uint32_t param);
LOCAL uint32_t _JX507_ExtFunc(uint32_t ctl_param);
LOCAL int _JX507_get_VTS(void);
LOCAL int _JX507_set_VTS(int VTS);
LOCAL uint32_t _JX507_ReadGain(uint32_t param);
LOCAL uint32_t _JX507_set_video_mode(uint32_t param);
LOCAL int _JX507_get_shutter(void);
LOCAL uint32_t _JX507_com_Identify_otp(void* param_ptr);
static uint32_t s_JX507_gain = 0;
static uint32_t g_module_id = 0;

static uint32_t g_flash_mode_en = 0;
static uint32_t g_af_slewrate = 1;

LOCAL const struct raw_param_info_tab s_JX507_raw_param_tab[]={
	{JX507_RAW_PARAM_COM, &s_JX507_mipi_raw_info, _JX507_com_Identify_otp, PNULL},
	{RAW_INFO_END_ID, PNULL, PNULL, PNULL}
};

struct sensor_raw_info* s_JX507_raw_info_ptr=NULL;

LOCAL const SENSOR_REG_T JX507_com_raw[] =
{
	{0xc0, 0x00},  // gain

};

LOCAL const SENSOR_REG_T JX507_1600X1200_raw[] = {
	{0x00,0x7f},//;;---Gain
	{0x01,0xF6},//;;---Exposure
	{0x02,0x05},
	{0x03,0xFF},
	{0x04,0xFF},
	{0x0C,0x40},
	{0x0D,0x50},//0x04},//@0226 new setting
	{0x0E,0x10},
	{0x0F,0x04},
	{0x10,0x13},
	{0x11,0x80},
	{0x13,0x87},//;;---AEC/AGC
	{0x14,0x80},
	{0x15,0x44},
	{0x16,0xC0},
	{0x17,0x40},
	{0x18,0x7D},
	{0x19,0x29},
	{0x1A,0x80},
	{0x1B,0x4F},
	{0x1D,0x00},
	{0x1E,0x1C},
	{0x1F,0x00},
	{0x20,0xC8},
	{0x21,0x07},
	{0x22,0xF6},//;;---Frame
	{0x23,0x05},
	{0x24,0x40},
	{0x25,0xB0},
	{0x26,0x46},
	{0x27,0x96},
	{0x28,0x0c},
	{0x29,0x01},
	{0x2A,0x80},
	{0x2B,0x29},
	{0x2C,0x3E},
	{0x2D,0x5D},
	{0x2E,0x8D},
	{0x2F,0x04},
	{0x30,0x9C},
	{0x31,0x14},
	{0x32,0xBE},
	{0x33,0x18},
	{0x34,0x3E},
	{0x35,0xEC},
	{0x3A,0x00},
	{0x36,0x00},
	{0x37,0x40},
	{0x38,0x2D},
	{0x39,0xFF},
	{0x3B,0x00},
	{0x3C,0x08},
	{0x3D,0x10},
	{0x3E,0x08},
	{0x3F,0x10},
	{0x40,0x08},
	{0x48,0x00},
	{0x49,0x04},//;;---BLC
	{0x4A,0x03},
	{0x4B,0xA4},
	{0x4C,0xA4},
	{0x4D,0xA4},
	{0x4E,0xA4},
	{0x4F,0x55},
	{0x50,0x10},
	{0x51,0x88},
	{0x52,0x00},
	{0x53,0x84},
	{0x54,0x80},
	{0x55,0x00},
	{0x56,0x42},
	{0x57,0x00},
	{0x5F,0x03},
	{0x60,0x27},
	{0x61,0xFC},
	{0x62,0x00},//0x03},//@0226 new setting
	{0x63,0xC0},
	{0x64,0x07},
	{0x65,0x80},
	{0x66,0x10},
	{0x67,0x79},
	{0x68,0x00},
	{0x69,0x72},
	{0x6A,0x3A},
	{0x6C,0x00},
	{0x6B,0x00},
	{0x6D,0x02},
	{0x6E,0x8E},
	{0x70,0x69},
	{0x71,0x8A},
	{0x72,0x68},
	{0x73,0x33},
	{0x74,0x02},
	//;;---SDE
	{0x75,0x2B},
	{0x76,0xD0},
	{0x77,0x07},
	{0x78,0x14},
};

LOCAL const SENSOR_REG_T JX507_1280X960_raw[] = {
	{0x12,(0x41 | (JX507_MIRROR_EN << 5) |(JX507_FLIP_EN << 4) | (JX507_USE_VERTICAL_BINNING_EN << 1))},
	{0x00,0x70},
	{0x01,0x28},
	{0x02,0x06},
	{0x03,0xFF},
	{0x04,0xFF},
	{0x0C,0x40},
	{0x0D,0x50},
	{0x0E,0x10},
	{0x0F,0x04},
	{0x10,0x13},
	{0x11,0x80},
	{0x13,0x87},
	{0x14,0x80},
	{0x15,0x44},
	{0x16,0xC0},
	{0x17,0x40},
	{0x18,0x8A},
	{0x19,0x29},
	{0x1A,0x80},
	{0x1B,0x4F},
	{0x1D,0x00},
	{0x1E,0x1C},
	{0x1F,0x00},
	{0x20,0x88},
	{0x21,0x07},
	{0x22,0x28},
	{0x23,0x06},
	{0x24,0x00},
	{0x25,0xC0},
	{0x26,0x35},
	{0x27,(0xA9 + JX507_FLIP_EN*12 + JX507_MIRROR_EN)},//(0x95 + JX507_FLIP_EN*12)},
	{0x28,(0x06 - JX507_FLIP_EN)},
	{0x29,0x02},
	{0x2A,0x91},   //0x81
	{0x2B,0x2A},
	{0x2C,0x02},
	{0x2D,0x03},
	{0x2E,0xE8},
	{0x2F,0x04},
	{0x30,0x9C},
	{0x31,0x14},
	{0x32,0xBE},
	{0x33,0x18},
	{0x34,0x3E},
	{0x35,0xEC},
	{0x3A,0x00},
	{0x36,0x00},
	{0x37,0x40},
	{0x38,0xE9},
	{0x39,0x20},
	{0x3B,0x00},
	{0x3C,0x08},
	{0x3D,0x10},
	{0x3E,0x08},
	{0x3F,0x10},
	{0x40,0x08},
	{0x48,0x00},
	{0x49,0x04},
	{0x4A,0x03},
	{0x4B,0xA4},
	{0x4C,0xA4},
	{0x4D,0xA4},
	{0x4E,0xA4},
	{0x4F,0x55},
	{0x50,0x10},
	{0x51,0x88},
	{0x52,0x00},
	{0x53,0x84},
	{0x54,0x80},
	{0x55,0x00},
	{0x56,0x42},
	{0x57,0x00},
	{0x5F,0x03},
	{0x60,0x27},
	{0x61,0xFC},
	{0x62,0x00},//0x03},//@0226 new setting
	{0x63,0xC0},
	{0x64,0x07},
	{0x65,0x80},
	{0x66,0x10},
	{0x67,0x79},
	{0x68,0x00},
	{0x69,0x72},
	{0x6A,0x3A},
	{0x6C,0x00},

	{0x6B,0x00},
	{0x6D,0x02},
	{0x6E,0x8E},
	{0x70,0x69},
	{0x71,0x8A},
	{0x72,0x68},
	{0x73,0x33},
	{0x74,0x02},
	{0x75,0x2B},
	{0x76,0x40},
	{0x77,0x06},
	{0x78,0x14}
};

LOCAL const SENSOR_REG_T JX507_2592X1944_raw[] = {
	{0x12,(0x40 | (JX507_MIRROR_EN << 5) |(JX507_FLIP_EN << 4))},
	{0x03,0xFF},
	{0x04,0xFF},
	{0x0C,0x40},
	{0x0D,0x50},//0x04},//@0226 new setting
	{0x0E,0x10},
	{0x0F,0x04},
	{0x10,0x13},
	{0x11,0x80},
	{0x13,0x87},//;;---AEC/AGC
	{0x14,0x80},
	{0x15,0x44},
	{0x16,0xC0},
	{0x17,0x40},
	{0x18,0xFE},
	{0x19,0x28},
	{0x1A,0x80},
	{0x1B,0x4F},
	{0x1D,0x00},
	{0x1E,0x1C},
	{0x1F,0x00},
	{0x20,0xA8},
	{0x21,0x0B},
	{0x22,0xF5},//;;---Frame
	{0x23,0x07},
	{0x24,0x20},
	{0x25,0x98},
	{0x26,0x7A},
	{0x27,(0xA0 + JX507_FLIP_EN*12 + JX507_MIRROR_EN)},//(0x96 + JX507_FLIP_EN*12)},
	{0x28,(0x0c - JX507_FLIP_EN)},
	{0x29,0x01},
	{0x2A,0x90},  //0x80
	{0x2B,0x29},
	{0x2C,0x00},
	{0x2D,0x00},
	{0x2E,0xEA},
	{0x2F,0x04},
	{0x30,0x9C},
	{0x31,0x14},
	{0x32,0xBE},
	{0x33,0x18},
	{0x34,0x3E},
	{0x35,0xEC},
	{0x3A,0x00},
	{0x36,0x00},
	{0x37,0x40},
	{0x38,0xE7},
	{0x39,0x34},
	{0x3B,0x00},
	{0x3C,0x08},
	{0x3D,0x10},
	{0x3E,0x08},
	{0x3F,0x10},
	{0x40,0x08},
	{0x48,0x00},
	{0x49,0x04},//;;---BLC
	{0x4A,0x03},//{0x4A,0x03},
	{0x4B,0xA4},
	{0x4C,0xA4},
	{0x4D,0xA4},
	{0x4E,0xA4},
	{0x4F,0x55},
	{0x50,0x10},
	{0x51,0x88},
	{0x52,0x00},
	{0x53,0x84},
	{0x54,0x80},
	{0x55,0x00},
	{0x56,0x42},
	{0x57,0x00},
	{0x5F,0x03},
	{0x60,0x27},
	{0x61,0xFC},
	{0x62,0x00},//0x03},//@0226 new setting
	{0x63,0xC0},
	{0x64,0x07},
	{0x65,0x80},
	{0x66,0x10},
	{0x67,0x79},
	{0x68,0x00},
	{0x69,0x72},
	{0x6A,0x3A},
	{0x6C,0x00},
	{0x6B,0x00},
	{0x6D,0x02},
	{0x6E,0x8E},
	{0x70,0x69},
	{0x71,0x8A},
	{0x72,0x68},
	{0x73,0x33},
	{0x74,0x02},
	{0x75,0x2B},
	{0x76,0xA8},
	{0x77,0x0C},
	{0x78,0x14}
};

LOCAL SENSOR_REG_TAB_INFO_T s_JX507_resolution_Tab_RAW[] = {
	{ADDR_AND_LEN_OF_ARRAY(JX507_com_raw), 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(JX507_1280X960_raw), 1280, 960, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(JX507_2592X1944_raw), 2592, 1944, 24, SENSOR_IMAGE_FORMAT_RAW},

	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

LOCAL SENSOR_TRIM_T s_JX507_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 1280,  960, 211, 912, 1576, {0, 0, 1280, 960}},//sysclk*10
	{0, 0, 2592, 1944, 327, 912, 2038, {0, 0, 2592, 1944}},//sysclk*10
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}}
};

LOCAL const SENSOR_REG_T s_JX507_1280x960_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

LOCAL const SENSOR_REG_T s_JX507_1600x1200_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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
LOCAL const SENSOR_REG_T s_JX507_2592x1944_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

LOCAL SENSOR_VIDEO_INFO_T s_JX507_video_info[] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{30, 30, 211, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_JX507_1280x960_video_tab},
	{{{15, 15, 327, 64}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_JX507_2592x1944_video_tab},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}
};

LOCAL uint32_t _JX507_set_video_mode(uint32_t param)
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

	if (PNULL == s_JX507_video_info[mode].setting_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR)&s_JX507_video_info[mode].setting_ptr[param];
	if (PNULL == sensor_reg_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	for (i=0x00; (0xffff!=sensor_reg_ptr[i].reg_addr)||(0xff!=sensor_reg_ptr[i].reg_value); i++) {
		Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	SENSOR_PRINT("_JX507_set_video_mode = 0x%02x", param);
	return 0;
}

LOCAL SENSOR_IOCTL_FUNC_TAB_T s_JX507_ioctl_func_tab = {
	PNULL,
	_JX507_PowerOn,
	PNULL,
	_JX507_Identify,

	PNULL,// write register
	PNULL,// read  register
	PNULL,
	_JX507_GetResolutionTrimTab,

	// External
	PNULL,
	PNULL,
	PNULL,

	PNULL, //_JX507_set_brightness,
	PNULL, // _JX507_set_contrast,
	PNULL,
	PNULL,//_JX507_set_saturation,

	PNULL, //_JX507_set_work_mode,
	PNULL, //_JX507_set_image_effect,

	_JX507_BeforeSnapshot,
	_JX507_after_snapshot,
	_JX507_flash,
	PNULL,
	_JX507_write_exposure,
	PNULL,
	_JX507_write_gain,
	PNULL,
	PNULL,
	_JX507_write_af,
	PNULL,
	PNULL, //_JX507_set_awb,
	PNULL,
	PNULL,
	PNULL, //_JX507_set_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL, //_JX507_GetExifInfo,
	_JX507_ExtFunc,
	PNULL, //_JX507_set_anti_flicker,
	_JX507_set_video_mode, //_JX507_set_video_mode,
	PNULL, //pick_jpeg_stream
	PNULL, //meter_mode
	PNULL, //get_status
	_JX507_StreamOn,
	_JX507_StreamOff,
	PNULL
};

SENSOR_INFO_T g_JX507_mipi_raw_info = {
	JX507_I2C_ADDR_W,	// salve i2c write address
	JX507_I2C_ADDR_R,	// salve i2c read address

	SENSOR_I2C_REG_8BIT | SENSOR_I2C_REG_8BIT | SENSOR_I2C_FREQ_400,	// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
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
	{{0x0a, 0xa5},		// supply two code to identify sensor.
	 {0x0b, 0x07}},		// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,	// voltage of avdd

	2592,			// max width of source image
	1944,			// max height of source image
	"JX507",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_R,// pattern of input image form sensor;

	s_JX507_resolution_Tab_RAW,	// point to resolution table information structure
	&s_JX507_ioctl_func_tab,	// point to ioctl function table
	&s_JX507_raw_info_ptr,		// information and table about Rawrgb sensor
	NULL,			//&g_JX507_ext_info,                // extend information about sensor
	SENSOR_AVDD_1800MV,	// iovdd
	SENSOR_AVDD_CLOSED,	// dvdd
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
	s_JX507_video_info,
	3,			// skip frame num while change setting
};

LOCAL struct sensor_raw_info* Sensor_GetContext(void)
{
	return s_JX507_raw_info_ptr;
}

LOCAL uint32_t _JX507_InitRawTuneInfo(void)
{
	uint32_t rtn=0x00;
	struct sensor_raw_info* raw_sensor_ptr=Sensor_GetContext();
	struct sensor_raw_tune_info* sensor_ptr=raw_sensor_ptr->tune_ptr;
	struct sensor_raw_cali_info* cali_ptr=raw_sensor_ptr->cali_ptr;

	raw_sensor_ptr->version_info->version_id=0x00010000;
	raw_sensor_ptr->version_info->srtuct_size=sizeof(struct sensor_raw_info);

#if 0
	//bypass
	sensor_ptr->version_id		= 0x00010000;
	sensor_ptr->blc_bypass		= 0x00;
	sensor_ptr->nlc_bypass		= 	0x01;
	sensor_ptr->lnc_bypass		= 	0x00;
	sensor_ptr->ae_bypass		= 0x00;
	sensor_ptr->awb_bypass		= 0x00;
	sensor_ptr->bpc_bypass		= 0x00;
	sensor_ptr->denoise_bypass	= 0x00;
	sensor_ptr->grgb_bypass		= 	0x01;
	sensor_ptr->cmc_bypass		= 0x00;
	sensor_ptr->gamma_bypass	= 0x00;
	sensor_ptr->uvdiv_bypass	= 	0x01;
	sensor_ptr->pref_bypass		= 0x00;
	sensor_ptr->bright_bypass	= 0x00;
	sensor_ptr->contrast_bypass	= 0x00;
	sensor_ptr->hist_bypass 	= 	0x01;
	sensor_ptr->auto_contrast_bypass= 0x00;
	sensor_ptr->af_bypass		= 0x00;
	sensor_ptr->edge_bypass 	= 0x00;
	sensor_ptr->fcs_bypass 		= 0x00;
	sensor_ptr->css_bypass 		= 0x00;
	sensor_ptr->saturation_bypass 	= 0x00;
	sensor_ptr->hdr_bypass 		= 	0x01;
	sensor_ptr->glb_gain_bypass 	= 	0x01;
	sensor_ptr->chn_gain_bypass	= 	0x01;

	//ae
	sensor_ptr->ae.min_exposure 	= 1;
	sensor_ptr->ae.skip_frame	= 0x01;
	sensor_ptr->ae.normal_fix_fps	= 0;
	sensor_ptr->ae.night_fix_fps 	= 0;
	sensor_ptr->ae.video_fps	= 0x1e;
	sensor_ptr->ae.target_lum	= 120;
	sensor_ptr->ae.target_zone 	= 8;
	sensor_ptr->ae.quick_mode	= 1;
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
	sensor_ptr->awb.debug_level = 0;

	sensor_ptr->awb.alg_id = 0;
	sensor_ptr->awb.smart_index = 4;

	//blc
	sensor_ptr->blc.mode		= 0x00;
	sensor_ptr->blc.offset[0].r	= 4;
	sensor_ptr->blc.offset[0].gr	= 4;
	sensor_ptr->blc.offset[0].gb	= 4;
	sensor_ptr->blc.offset[0].b	= 4;

	sensor_ptr->blc.offset[1].r	= 4;
	sensor_ptr->blc.offset[1].gr	= 4;
	sensor_ptr->blc.offset[1].gb	= 4;
	sensor_ptr->blc.offset[1].b	= 4;

	sensor_ptr->cmc.matrix[0][0] = 0x0747;
	sensor_ptr->cmc.matrix[0][1] = 0x3BD9;
	sensor_ptr->cmc.matrix[0][2] = 0x00E0;
	sensor_ptr->cmc.matrix[0][3] = 0x3F0E;
	sensor_ptr->cmc.matrix[0][4] = 0x04AD;
	sensor_ptr->cmc.matrix[0][5] = 0x0045;
	sensor_ptr->cmc.matrix[0][6] = 0x00BE;
	sensor_ptr->cmc.matrix[0][7] = 0x3B54;
	sensor_ptr->cmc.matrix[0][8] = 0x07ED;

	//af info
	sensor_ptr->af.rough_count = 17;
	sensor_ptr->af.af_rough_step[0] = 0;
	sensor_ptr->af.af_rough_step[1] = 64;
	sensor_ptr->af.af_rough_step[2] = 128;
	sensor_ptr->af.af_rough_step[3] = 192;
	sensor_ptr->af.af_rough_step[4] = 256;
	sensor_ptr->af.af_rough_step[5] = 320;
	sensor_ptr->af.af_rough_step[6] = 384;
	sensor_ptr->af.af_rough_step[7] = 448;
	sensor_ptr->af.af_rough_step[8] = 512;
	sensor_ptr->af.af_rough_step[9] = 576;
	sensor_ptr->af.af_rough_step[10] = 640;
	sensor_ptr->af.af_rough_step[11] = 704;
	sensor_ptr->af.af_rough_step[12] = 768;
	sensor_ptr->af.af_rough_step[13] = 832;
	sensor_ptr->af.af_rough_step[14] = 896;
	sensor_ptr->af.af_rough_step[15] = 960;
	sensor_ptr->af.af_rough_step[16] = 1023;
//#if 0

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
	sensor_ptr->af.alg_id=2;
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

LOCAL uint32_t _dw9174_SRCInit(uint32_t mode)
{
	uint8_t cmd_val[2] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t ret_value = SENSOR_SUCCESS;

	slave_addr = DW9714_VCM_SLAVE_ADDR;
	switch (mode) {
		case 1:
		break;

		case 2:
		{
			cmd_len = 2;
			cmd_val[0] = 0xec;
			cmd_val[1] = 0xa3;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
			if(ret_value){
				SENSOR_PRINT("SENSOR_JX507: _dw9174_SRCInit fail!1");
			}
			cmd_val[0] = 0xf2;
			cmd_val[1] = 0x00;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
			if(ret_value){
				SENSOR_PRINT("SENSOR_JX507: _dw9174_SRCInit fail!2");
			}

			cmd_val[0] = 0xdc;
			cmd_val[1] = 0x51;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
			if(ret_value){
				SENSOR_PRINT("SENSOR_JX507: _dw9174_SRCInit fail!3");
			}
		}
		break;

		case 3:
		break;

	}

	return ret_value;
}

LOCAL uint32_t _JX507_GetResolutionTrimTab(uint32_t param)
{
	SENSOR_PRINT("SENSOR_JX507 GetResolutionTrimTab param = 0x%x, param = 0x%x", (uint32_t)s_JX507_Resolution_Trim_Tab, param);
	return (uint32_t) s_JX507_Resolution_Trim_Tab;
}

LOCAL uint32_t _JX507_PowerOn(uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_JX507_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_JX507_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_JX507_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_JX507_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_JX507_mipi_raw_info.reset_pulse_level;

	if (SENSOR_TRUE == power_on) {

		Sensor_PowerDown(!power_down);
		// Open power
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		usleep(20*1000);
		_dw9174_SRCInit(2);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(10*1000);
		Sensor_PowerDown(power_down);
		// Reset sensor
		Sensor_Reset(reset_level);
	} else {
		Sensor_PowerDown(power_down);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
	}
	SENSOR_PRINT("SENSOR_JX507: _JX507_Power_On(1:on, 0:off): %d  ", power_on);
	return SENSOR_SUCCESS;
}

LOCAL uint32_t _JX507_cfg_otp(uint32_t  param)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_JX507_raw_param_tab;
	uint32_t module_id=g_module_id;

	SENSOR_PRINT("SENSOR_JX507: _JX507_cfg_otp");

	if(PNULL!=tab_ptr[module_id].cfg_otp){
		tab_ptr[module_id].cfg_otp(0);
	}

	return rtn;
}

LOCAL uint32_t _JX507_com_Identify_otp(void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_JX507: _JX507_com_Identify_otp");

	/*read param id from sensor omap*/
	param_id=JX507_RAW_PARAM_COM;

	if(JX507_RAW_PARAM_COM==param_id){
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}

LOCAL uint32_t _JX507_GetRawInof(void)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_JX507_raw_param_tab;
	uint32_t param_id;
	uint32_t i=0x00;

	/*read param id from sensor omap*/
	param_id=JX507_RAW_PARAM_COM;

	for(i=0x00; ; i++)
	{
		g_module_id = i;
		if(RAW_INFO_END_ID==tab_ptr[i].param_id){
			if(NULL==s_JX507_raw_info_ptr){
				SENSOR_PRINT("SENSOR_JX507: JX507_GetRawInof no param error");
				rtn=SENSOR_FAIL;
			}
			SENSOR_PRINT("SENSOR_JX507: JX507_GetRawInof end");
			break;
		}
		else if(PNULL!=tab_ptr[i].identify_otp){
			if(SENSOR_SUCCESS==tab_ptr[i].identify_otp(0))
			{
				s_JX507_raw_info_ptr = tab_ptr[i].info_ptr;
				SENSOR_PRINT("SENSOR_JX507: JX507_GetRawInof success");
				break;

			}
		}
	}

	return rtn;
}

LOCAL uint32_t _JX507_GetMaxFrameLine(uint32_t index)
{
	uint32_t max_line=0x00;
	SENSOR_TRIM_T_PTR trim_ptr=s_JX507_Resolution_Trim_Tab;

	max_line=trim_ptr[index].frame_line;

	SENSOR_PRINT("SENSOR_JX507: _JX507_GetMaxFrameLine maxline = 0x%x, index = 0x%x", max_line, index);
	return max_line;
}

LOCAL uint32_t _JX507_Identify(uint32_t param)
{
	uint8_t pid_value = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("SENSOR_JX507:  raw identify \n");

	pid_value = Sensor_ReadReg(JX507_PID_ADDR);

	if (JX507_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(JX507_VER_ADDR);
		SENSOR_PRINT("SENSOR_JX507: Identify: PID = %x, VER = %x", pid_value, ver_value);
		if (JX507_VER_VALUE == ver_value) {
			_JX507_GetRawInof();
			_JX507_InitRawTuneInfo();
			ret_value = SENSOR_SUCCESS;
			SENSOR_PRINT("SENSOR_JX507: this is JX507 sensor !");
		} else {
			SENSOR_PRINT
			    ("SENSOR_JX507: Identify this is OV%x%x sensor !", pid_value, ver_value);
		}
	} else {
		SENSOR_PRINT("SENSOR_JX507: identify fail,pid_value=%x", pid_value);
	}

	return ret_value;
}

LOCAL uint32_t _JX507_write_exposure(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t expsure_line=0x00;
	uint16_t size_index=0x00;
	uint16_t frame_len=0x00;
	uint16_t frame_len_cur=0x00;
	uint16_t max_frame_len=0x00;
	uint16_t value=0x00;
	uint8_t lsb=0x00;
	uint8_t msb=0x00;

	expsure_line=param&0xffff;
	size_index=(param>>0x1c)&0x0f;

	if (!expsure_line) expsure_line = 1;

	max_frame_len =_JX507_GetMaxFrameLine(size_index);

	if(0x00!=max_frame_len)
	{
		frame_len = ((expsure_line + JX507_MAX_SHUTTER_OFFSET)> max_frame_len) ? (expsure_line + JX507_MAX_SHUTTER_OFFSET) : max_frame_len;

		if(0x00!=(0x01&frame_len))
		{
			frame_len+=0x01;
		}

		frame_len_cur = (Sensor_ReadReg(0x23)&0xff)<<8;
		frame_len_cur |= Sensor_ReadReg(0x22)&0xff;
		if (frame_len_cur != frame_len){
			lsb=(frame_len)&0xff;
			msb=(frame_len>>0x08)&0xff;

			ret_value = Sensor_WriteReg(0x22, lsb);
			ret_value = Sensor_WriteReg(0x23, msb);
		}
		lsb=(expsure_line)&0xff;
		msb=(expsure_line>>0x08)&0xff;

		ret_value = Sensor_WriteReg(0x01, lsb);
		ret_value = Sensor_WriteReg(0x02, msb);
	}

	SENSOR_PRINT("SENSOR_JX507: JX507_Write_Shutter expsure_line = 0x%x, max_frame_len = 0x%x", expsure_line, max_frame_len);
	return ret_value;
}

LOCAL uint32_t _JX507_write_gain(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;

#if JX507_GROUP_WRITE_EN
	uint16_t val;
	val = Sensor_ReadReg(0x12);
	if (val & 0x08) {
		SENSOR_PRINT("SENSOR_JX507: write gain reg[0x12][3] not clear!! (0x%x)", val);

	}
	Sensor_WriteReg(0xc0, 0x00); // gain
	Sensor_WriteReg(0xC1, (param & 0x7f));
	Sensor_WriteReg(0x12, (val | 0x08));
#else
	Sensor_WriteReg(0x00, (param & 0x7f));
#endif
	return ret_value;
}

LOCAL uint32_t _JX507_write_af(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint8_t cmd_val[2] = {0x00};
	uint16_t slave_addr = 0;
	uint16_t cmd_len = 0;

	slave_addr = DW9714_VCM_SLAVE_ADDR;
	cmd_val[0] = (param >> 4) & 0x3f;
	cmd_val[1] = ((param << 4) & 0xf0) | 0x09;
	cmd_len = 2;
	ret_value = Sensor_WriteI2C(slave_addr, (uint8_t*)&cmd_val[0], cmd_len);
	SENSOR_PRINT("SENSOR_JX507: _JX507_write_af pos = 0x%x", param);
	return ret_value;
}

LOCAL uint32_t _JX507_BeforeSnapshot(uint32_t param)
{
	uint8_t ret_l, ret_m, ret_h;
	uint32_t capture_exposure, preview_maxline;
	uint32_t capture_maxline, preview_exposure;

	uint32_t capture_mode = param & 0xffff;
	uint32_t preview_mode = (param >> 0x10 ) & 0xffff;
	uint32_t prv_linetime = s_JX507_Resolution_Trim_Tab[preview_mode].line_time;
	uint32_t cap_linetime = s_JX507_Resolution_Trim_Tab[capture_mode].line_time;

	if (SENSOR_MODE_PREVIEW_ONE >= param){
		_JX507_ReadGain(0x00);
		SENSOR_PRINT("SENSOR_JX507: prvmode equal to capmode");
		return SENSOR_SUCCESS;
	}

	ret_h = (uint8_t) Sensor_ReadReg(0x02);
	ret_l = (uint8_t) Sensor_ReadReg(0x01);
	preview_exposure = (ret_h << 8) + ret_l;

	ret_h = (uint8_t) Sensor_ReadReg(0x23);
	ret_l = (uint8_t) Sensor_ReadReg(0x22);
	preview_maxline = (ret_h << 8) + ret_l;

	//_JX507_ReadGain(&gain);

	Sensor_SetMode(capture_mode);
	Sensor_SetMode_WaitDone();

	if (prv_linetime == cap_linetime) {
		SENSOR_PRINT("SENSOR_JX507: prvline equal to capline");
		return SENSOR_SUCCESS;
	}

	ret_h = (uint8_t) Sensor_ReadReg(0x23);
	ret_l = (uint8_t) Sensor_ReadReg(0x22);
	capture_maxline = (ret_h << 8) + ret_l;
	capture_exposure = preview_exposure *prv_linetime  / cap_linetime ;

	if (0 == capture_exposure) {
		capture_exposure = 1;
	}

	capture_exposure = capture_exposure * 2;
	if(capture_exposure > (capture_maxline - JX507_MAX_SHUTTER_OFFSET)){
		capture_maxline = capture_exposure + JX507_MAX_SHUTTER_OFFSET;
		ret_l = (unsigned char) (capture_maxline & 0xff);
		ret_h = (unsigned char)((capture_maxline >> 8)&0xff);
		Sensor_WriteReg(0x23, ret_h);
		Sensor_WriteReg(0x22, ret_l);
	}
	ret_l = (unsigned char)((capture_exposure)&0xff);
	ret_h = (unsigned char)((capture_exposure >> 8)&0xff);

	Sensor_WriteReg(0x01, ret_l);
	Sensor_WriteReg(0x02, ret_h);

	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, capture_exposure);

	return SENSOR_SUCCESS;
}

LOCAL uint32_t _JX507_after_snapshot(uint32_t param)
{
	uint16_t i, j;
	uint16_t val[16];
	for (i = 0; i < 16; i++) {
		for (j = 0; j < 16; j++) {
			val[j] = Sensor_ReadReg(i*16+j);
		}
	}
	Sensor_SetMode(param);

	return SENSOR_SUCCESS;
}

LOCAL uint32_t _JX507_flash(uint32_t param)
{
	SENSOR_PRINT("SENSOR_JX507: param=%d", param);

	/* enable flash, disable in _JX507_BeforeSnapshot */
	g_flash_mode_en = param;
	Sensor_SetFlash(param);
	SENSOR_PRINT_HIGH("end");
	return SENSOR_SUCCESS;
}

LOCAL uint32_t _JX507_StreamOn(uint32_t param)
{
	int val;
	SENSOR_PRINT("SENSOR_JX507: StreamOn");
	val = Sensor_ReadReg(0x12);
	val &= ~(0x40);

	Sensor_WriteReg(0x12, val);

	return 0;
}

LOCAL uint32_t _JX507_StreamOff(uint32_t param)
{
	int val;
	SENSOR_PRINT("SENSOR_JX507: StreamOff");
	val = Sensor_ReadReg(0x12);
	val |= 0x40;
	Sensor_WriteReg(0x12, val);
	usleep(100*1000);
	return 0;
}

int _JX507_get_shutter(void)
{
	// read shutter, in number of line period
	int shutter;

	shutter = Sensor_ReadReg(0x01);
	shutter = (shutter<<8) + Sensor_ReadReg(0x02);

	SENSOR_PRINT("SENSOR_JX507: _JX507_get_shutter shutter = 0x%x", shutter);
	return shutter;
}

int _JX507_set_shutter(int shutter)
{
	// write shutter, in number of line period
	int temp;

	shutter = shutter & 0xffff;

	temp = shutter & 0xff;
	Sensor_WriteReg(0x01, temp);

	temp = (shutter >> 8) & 0xff;
	Sensor_WriteReg(0x02, temp);

	SENSOR_PRINT("SENSOR_JX507: _JX507_set_shutter shutter = 0x%x", shutter);
	return 0;
}

int _JX507_get_gain16(void)
{
	// read gain, 16 = 1x
	int gain16, param;

	param = Sensor_ReadReg(0x00);
	gain16 = ((param&0xf)+16)*(((param>>4)&0x01)+1)*(((param>>5)&0x01)+1)*(((param>>6)&0x01)+1);

	SENSOR_PRINT("SENSOR_JX507: _JX507_get_gain16 gain16 = 0x%x", gain16);
	return gain16;
}

int _JX507_set_gain16(int gain16)
{
	uint16_t iReg,temp;
	uint16_t gainMSB, gainLSB;
	//param : 1x = 16
	if (16*16 < gain16)
		gain16 = 255;
	if (1*16 > gain16)
		gain16 = 16;

	if(8*16 <= gain16) {
		gainMSB = 7;
	} else if (4*16 <= gain16) {
		gainMSB = 3;
	} else if (2*16 <= gain16) {
		gainMSB = 1;
	} else {
		gainMSB = 0;
	}

	gainLSB = gain16 / (gainMSB + 1) - 16;
	if (gainLSB > 15) {
		gainLSB = 15;
	}
	Sensor_WriteReg(0x00, (gainMSB << 4) + gainLSB);

	SENSOR_PRINT("SENSOR_JX507: _JX507_set_gain16 gain16 = 0x%x, gainMSB,LSB = 0x%x,0x%x ", gain16,gainMSB,gainLSB);
	return 0;
}

static void _calculate_hdr_exposure(int capture_gain16,int capture_VTS, int capture_shutter)
{
	_JX507_set_gain16(capture_gain16);

	_JX507_set_shutter(capture_shutter);
}

LOCAL uint32_t _JX507_SetEV(uint32_t param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	uint16_t value=0x00;
	uint32_t gain = s_JX507_gain;
	uint32_t ev = ext_ptr->param;

	SENSOR_PRINT("SENSOR: _JX507_SetEV param: 0x%x", ev);

	switch(ev) {
	case SENSOR_HDR_EV_LEVE_0:
		_calculate_hdr_exposure(s_JX507_gain/2,s_JX507_capture_VTS,s_JX507_capture_shutter);
		break;
	case SENSOR_HDR_EV_LEVE_1:
		_calculate_hdr_exposure(s_JX507_gain,s_JX507_capture_VTS,s_JX507_capture_shutter);
		break;
	case SENSOR_HDR_EV_LEVE_2:
		_calculate_hdr_exposure(s_JX507_gain,s_JX507_capture_VTS,s_JX507_capture_shutter *4);
		break;
	default:
		break;
	}

	return rtn;
}

LOCAL uint32_t _JX507_ExtFunc(uint32_t ctl_param)
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
		rtn = _JX507_SetEV(ctl_param);
		break;
	default:
		break;
	}
	return rtn;
}

LOCAL int _JX507_get_VTS(void)
{
	int VTS;

	VTS = Sensor_ReadReg(0x23);//total vertical size[15:8] high byte

	VTS = (VTS<<8) + Sensor_ReadReg(0x22);

	return VTS;
}

LOCAL int _JX507_set_VTS(int VTS)
{
	// write VTS to registers
	int temp;

	temp = VTS & 0xff;
	Sensor_WriteReg(0x22, temp);

	temp = VTS>>8;
	Sensor_WriteReg(0x23, temp);

	return 0;
}
LOCAL uint32_t _JX507_ReadGain(uint32_t param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint32_t gain = 0;

	value = Sensor_ReadReg(0x00);
	s_JX507_gain=(int)value;

	SENSOR_PRINT("SENSOR_JX507: _JX507_ReadGain gain: 0x%x", s_JX507_gain);

	return rtn;
}
