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
#include "sensor_imx219_raw_param.c"

#define IMX219_I2C_ADDR_W        0x1A
#define IMX219_I2C_ADDR_R        0x1A

#define IMX219_RAW_PARAM_COM  0x0000

#define IMX219_MIN_FRAME_LEN_PRV  0x060b
#define IMX219_MIN_FRAME_LEN_CAP  0x09e9
#define IMX219_4_LANES
#define DW9714A_VCM_SLAVE_ADDR (0x18 >> 1)
static int s_imx219_gain = 0;
static int s_capture_shutter = 0;
static int s_capture_VTS = 0;
static int s_video_min_framerate = 0;
static int s_video_max_framerate = 0;

static unsigned long _imx219_GetResolutionTrimTab(unsigned long param);
static unsigned long _imx219_PowerOn(unsigned long power_on);
static unsigned long _imx219_Identify(unsigned long param);
static unsigned long _imx219_BeforeSnapshot(unsigned long param);
static unsigned long _imx219_after_snapshot(unsigned long param);
static unsigned long _imx219_StreamOn(unsigned long param);
static unsigned long _imx219_StreamOff(unsigned long param);
static unsigned long _imx219_write_exposure(unsigned long param);
static unsigned long _imx219_write_gain(unsigned long param);
static unsigned long _imx219_write_af(unsigned long param);
static unsigned long _imx219_flash(unsigned long param);
static unsigned long _imx219_ExtFunc(unsigned long ctl_param);
static uint16_t _imx219_get_VTS(void);
static uint32_t _imx219_set_VTS(uint16_t VTS);
static unsigned long _imx219_ReadGain(unsigned long param);
static unsigned long _imx219_set_video_mode(unsigned long param);
static uint16_t _imx219_get_shutter(void);
static uint32_t _imx219_set_shutter(uint16_t shutter);
static uint32_t _imx219_com_Identify_otp(void* param_ptr);

static const struct raw_param_info_tab s_imx219_raw_param_tab[]={
	{IMX219_RAW_PARAM_COM, &s_imx219_mipi_raw_info, _imx219_com_Identify_otp, NULL},
	{RAW_INFO_END_ID, PNULL, PNULL, PNULL}
};

struct sensor_raw_info* s_imx219_mipi_raw_info_ptr=NULL;

static uint32_t g_module_id = 0;

static uint32_t g_flash_mode_en = 0;
static uint32_t g_af_slewrate = 1;

static const SENSOR_REG_T imx219_common_init[] = {
	//1632*1224 setting
	{0x30EB,0x05},//Access Code for address over 0x3000
	{0x30EB,0x0C},//Access Code for address over 0x3000
	{0x300A,0xFF},//Access Code for address over 0x3000
	{0x300B,0xFF},//Access Code for address over 0x3000
	{0x30EB,0x05},//Access Code for address over 0x3000
	{0x30EB,0x09},//Access Code for address over 0x3000
	{0x5041,0x00},//delete 2line data
	{0x0114,0x03},//CSI_LANE_MODE[1:0}
	{0x0128,0x00},//DPHY_CNTRL
	{0x012A,0x18},//EXCK_FREQ[15:8]
	{0x012B,0x00},//EXCK_FREQ[7:0]
	//{0x0157,-   },//ANALOG_GAIN_GLOBAL_A[7:0]
	//{0x015A,-   },//COARSE_INTEG_TIME_A[15:8]
	//{0x015B,-   },//COARSE_INTEG_TIME_A[7:0]
	{0x0160,0x09},//FRM_LENGTH_A[15:8]
	{0x0161,0xE9},//FRM_LENGTH_A[7:0]
	{0x0162,0x0D},//LINE_LENGTH_A[15:8]
	{0x0163,0x78},//LINE_LENGTH_A[7:0]
	{0x0164,0x00},//X_ADD_STA_A[11:8]
	{0x0165,0x08},//X_ADD_STA_A[7:0]
	{0x0166,0x0C},//X_ADD_END_A[11:8]
	{0x0167,0xC7},//X_ADD_END_A[7:0]
	{0x0168,0x00},//Y_ADD_STA_A[11:8]
	{0x0169,0x08},//Y_ADD_STA_A[7:0]
	{0x016A,0x09},//Y_ADD_END_A[11:8]
	{0x016B,0x97},//Y_ADD_END_A[7:0]
	{0x016C,0x0C},//X_OUTPUT_SIZE_A[11:8]
	{0x016D,0xC0},//X_OUTPUT_SIZE_A[7:0]
	{0x016E,0x09},//Y_OUTPUT_SIZE_A[11:8]
	{0x016F,0x90},//Y_OUTPUT_SIZE_A[7:0]
	{0x0170,0x01},//X_ODD_INC_A[2:0]
	{0x0171,0x01},//Y_ODD_INC_A[2:0]
	{0x0172,0x03},// FLIP
	{0x0174,0x00},//BINNING_MODE_H_A
	{0x0175,0x00},//BINNING_MODE_V_A
	{0x018C,0x0A},//CSI_DATA_FORMAT_A[15:8]
	{0x018D,0x0A},//CSI_DATA_FORMAT_A[7:0]
	{0x0301,0x05},//VTPXCK_DIV
	{0x0303,0x01},//VTSYCK_DIV
	{0x0304,0x03},//PREPLLCK_VT_DIV[3:0]
	{0x0305,0x03},//PREPLLCK_OP_DIV[3:0]
	{0x0306,0x00},//PLL_VT_MPY[10:8]
	{0x0307,0x52},//PLL_VT_MPY[7:0]
	{0x0309,0x0A},//OPPXCK_DIV[4:0]
	{0x030B,0x01},//OPSYCK_DIV
	{0x030C,0x00},//PLL_OP_MPY[10:8]
	{0x030D,0x55},//PLL_OP_MPY[7:0]
	{0x455E,0x00},//CIS Tuning
	{0x471E,0x4B},//CIS Tuning
	{0x4767,0x0F},//CIS Tuning
	{0x4750,0x14},//CIS Tuning
	{0x4540,0x00},//CIS Tuning
	{0x47B4,0x14},//CIS Tuning
	{0x4713,0x30},//CIS Tuning
	{0x478B,0x10},//CIS Tuning
	{0x478F,0x10},//CIS Tuning
	{0x4793,0x10},//CIS Tuning
	{0x4797,0x0E},//CIS Tuning
	{0x479B,0x0E},//CIS Tuning
};

static const SENSOR_REG_T imx219_1632x1224_4lane_setting[] = {
	{0x30EB,0x05},//Access Code for address over 0x3000
	{0x30EB,0x0C},//Access Code for address over 0x3000
	{0x300A,0xFF},//Access Code for address over 0x3000
	{0x300B,0xFF},//Access Code for address over 0x3000
	{0x30EB,0x05},//Access Code for address over 0x3000
	{0x30EB,0x09},//Access Code for address over 0x3000
	{0x5041,0x00},//delete 2line data
	{0x0114,0x03},//CSI_LANE_MODE[1:0]
	{0x0128,0x00},//DPHY_CNTRL
	{0x012A,0x18},//EXCK_FREQ[15:8]
	{0x012B,0x00},//EXCK_FREQ[7:0]
	//{0x0157,-   },//ANALOG_GAIN_GLOBAL_A[7:0]
	//{0x015A,-   },//COARSE_INTEG_TIME_A[15:8]
	//{0x015B,-   },//COARSE_INTEG_TIME_A[7:0]
	{0x0160,0x06},//FRM_LENGTH_A[15:8]
	{0x0161,0x0B},//FRM_LENGTH_A[7:0]
	{0x0162,0x0D},//LINE_LENGTH_A[15:8]
	{0x0163,0x78},//LINE_LENGTH_A[7:0]
	{0x0164,0x00},//X_ADD_STA_A[11:8]
	{0x0165,0x08},//X_ADD_STA_A[7:0]
	{0x0166,0x0C},//X_ADD_END_A[11:8]
	{0x0167,0xC7},//X_ADD_END_A[7:0]
	{0x0168,0x00},//Y_ADD_STA_A[11:8]
	{0x0169,0x00},//Y_ADD_STA_A[7:0]
	{0x016A,0x09},//Y_ADD_END_A[11:8]
	{0x016B,0x9F},//Y_ADD_END_A[7:0]
	{0x016C,0x06},//X_OUTPUT_SIZE_A[11:8]
	{0x016D,0x60},//X_OUTPUT_SIZE_A[7:0]
	{0x016E,0x04},//Y_OUTPUT_SIZE_A[11:8]
	{0x016F,0xD0},//Y_OUTPUT_SIZE_A[7:0]
	{0x0170,0x01},//X_ODD_INC_A[2:0]
	{0x0171,0x01},//Y_ODD_INC_A[2:0]
	{0x0172,0x03},// FLIP
	{0x0174,0x01},//BINNING_MODE_H_A
	{0x0175,0x01},//BINNING_MODE_V_A
	{0x018C,0x0A},//CSI_DATA_FORMAT_A[15:8]
	{0x018D,0x0A},//CSI_DATA_FORMAT_A[7:0]
	{0x0301,0x05},//VTPXCK_DIV
	{0x0303,0x01},//VTSYCK_DIV
	{0x0304,0x03},//PREPLLCK_VT_DIV[3:0]
	{0x0305,0x03},//PREPLLCK_OP_DIV[3:0]
	{0x0306,0x00},//PLL_VT_MPY[10:8]
	{0x0307,0x32},//PLL_VT_MPY[7:0]
	{0x0309,0x0A},//OPPXCK_DIV[4:0]
	{0x030B,0x01},//OPSYCK_DIV
	{0x030C,0x00},//PLL_OP_MPY[10:8]
	{0x030D,0x32},//PLL_OP_MPY[7:0]
	{0x455E,0x00},//CIS Tuning
	{0x471E,0x4B},//CIS Tuning
	{0x4767,0x0F},//CIS Tuning
	{0x4750,0x14},//CIS Tuning
	{0x4540,0x00},//CIS Tuning
	{0x47B4,0x14},//CIS Tuning
	{0x4713,0x30},//CIS Tuning
	{0x478B,0x10},//CIS Tuning
	{0x478F,0x10},//CIS Tuning
	{0x4793,0x10},//CIS Tuning
	{0x4797,0x0E},//CIS Tuning
	{0x479B,0x0E},//CIS Tuning
};

static const SENSOR_REG_T imx219_3264x2448_4lane_setting[] = {
#if 1//15fps
	{0x30EB,0x05},//Access Code for address over 0x3000
	{0x30EB,0x0C},//Access Code for address over 0x3000
	{0x300A,0xFF},//Access Code for address over 0x3000
	{0x300B,0xFF},//Access Code for address over 0x3000
	{0x30EB,0x05},//Access Code for address over 0x3000
	{0x30EB,0x09},//Access Code for address over 0x3000
	{0x5041,0x00},//delete 2line data
	{0x0114,0x03},//CSI_LANE_MODE[1:0}
	{0x0128,0x00},//DPHY_CNTRL
	{0x012A,0x18},//EXCK_FREQ[15:8]
	{0x012B,0x00},//EXCK_FREQ[7:0]
	//{0x015-     },//ANALOG_GAIN_GLOBAL_A[7:0]
	//{0x015-     },//COARSE_INTEG_TIME_A[15:8]
	//{0x015-     },//COARSE_INTEG_TIME_A[7:0]
	{0x0160,0x0C},//FRM_LENGTH_A[15:8]
	{0x0161,0x16},//FRM_LENGTH_A[7:0]
	{0x0162,0x0D},//LINE_LENGTH_A[15:8]
	{0x0163,0x78},//LINE_LENGTH_A[7:0]
	{0x0164,0x00},//X_ADD_STA_A[11:8]
	{0x0165,0x08},//X_ADD_STA_A[7:0]
	{0x0166,0x0C},//X_ADD_END_A[11:8]
	{0x0167,0xC7},//X_ADD_END_A[7:0]
	{0x0168,0x00},//Y_ADD_STA_A[11:8]
	{0x0169,0x08},//Y_ADD_STA_A[7:0]
	{0x016A,0x09},//Y_ADD_END_A[11:8]
	{0x016B,0x97},//Y_ADD_END_A[7:0]
	{0x016C,0x0C},//X_OUTPUT_SIZE_A[11:8]
	{0x016D,0xC0},//X_OUTPUT_SIZE_A[7:0]
	{0x016E,0x09},//Y_OUTPUT_SIZE_A[11:8]
	{0x016F,0x90},//Y_OUTPUT_SIZE_A[7:0]
	{0x0170,0x01},//X_ODD_INC_A[2:0]
	{0x0171,0x01},//Y_ODD_INC_A[2:0]
	{0x0172,0x03},// FLIP
	{0x0174,0x00},//BINNING_MODE_H_A
	{0x0175,0x00},//BINNING_MODE_V_A
	{0x018C,0x0A},//CSI_DATA_FORMAT_A[15:8]
	{0x018D,0x0A},//CSI_DATA_FORMAT_A[7:0]
	{0x0301,0x05},//VTPXCK_DIV
	{0x0303,0x01},//VTSYCK_DIV
	{0x0304,0x03},//PREPLLCK_VT_DIV[3:0]
	{0x0305,0x03},//PREPLLCK_OP_DIV[3:0]
	{0x0306,0x00},//PLL_VT_MPY[10:8]
	{0x0307,0x32},//PLL_VT_MPY[7:0]
	{0x0309,0x0A},//OPPXCK_DIV[4:0]
	{0x030B,0x01},//OPSYCK_DIV
	{0x030C,0x00},//PLL_OP_MPY[10:8]
	{0x030D,0x35},//PLL_OP_MPY[7:0]
	{0x455E,0x00},//CIS Tuning
	{0x471E,0x4B},//CIS Tuning
	{0x4767,0x0F},//CIS Tuning
	{0x4750,0x14},//CIS Tuning
	{0x4540,0x00},//CIS Tuning
	{0x47B4,0x14},//CIS Tuning
	{0x4713,0x30},
	{0x478B,0x10},
	{0x478F,0x10},
	{0x4793,0x10},
	{0x4797,0x0E},
	{0x479B,0x0E},
#endif
};

static SENSOR_REG_TAB_INFO_T s_imx219_resolution_Tab_RAW[] = {
	//{ADDR_AND_LEN_OF_ARRAY(imx219_common_init), 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},
	{PNULL, 0, 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(imx219_1632x1224_4lane_setting), 1632, 1224, 24, SENSOR_IMAGE_FORMAT_RAW},
	//{ADDR_AND_LEN_OF_ARRAY(imx219_3264x2448_2lane_setting), 3264, 2448, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(imx219_3264x2448_4lane_setting), 3264, 2448, 24, SENSOR_IMAGE_FORMAT_RAW},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
};

static SENSOR_TRIM_T s_imx219_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 1632, 1224, 215, 400, 1547,{0, 0, 1632, 1224}},
	//{0, 0, 3264, 2448, 131, 680, 2537,{0, 0, 3264, 2448}},//30fps
	{0, 0, 3264, 2448, 215, 424, 3094, {0, 0, 3264, 2448}},//15fps
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}}
};

static const SENSOR_REG_T s_imx219_1632x1224_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

static const SENSOR_REG_T s_imx219_1920x1080_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

static const SENSOR_REG_T  s_imx219_3264x2448_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

static SENSOR_VIDEO_INFO_T s_imx219_video_info[] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{30, 30, 215, 160}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_imx219_1632x1224_video_tab},
	{{{15, 15, 215, 246}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_imx219_3264x2448_video_tab},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}
};

static unsigned long _imx219_set_video_mode(unsigned long param)
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

	if (PNULL == s_imx219_video_info[mode].setting_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR)&s_imx219_video_info[mode].setting_ptr[param];
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


static SENSOR_IOCTL_FUNC_TAB_T s_imx219_ioctl_func_tab = {
	PNULL,
	_imx219_PowerOn,
	PNULL,
	_imx219_Identify,

	PNULL,			// write register
	PNULL,			// read  register
	PNULL,
	_imx219_GetResolutionTrimTab,

	// External
	PNULL,
	PNULL,
	PNULL,

	PNULL, //_imx219_set_brightness,
	PNULL, // _imx219_set_contrast,
	PNULL,
	PNULL,			//_imx219_set_saturation,

	PNULL, //_imx219_set_work_mode,
	PNULL, //_imx219_set_image_effect,

	_imx219_BeforeSnapshot,
	_imx219_after_snapshot,
	_imx219_flash,
	PNULL,
	_imx219_write_exposure,
	PNULL,
	_imx219_write_gain,
	PNULL,
	PNULL,
	_imx219_write_af,
	PNULL,
	PNULL, //_imx219_set_awb,
	PNULL,
	PNULL,
	PNULL, //_imx219_set_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL, //_imx219_GetExifInfo,
	_imx219_ExtFunc,
	PNULL, //_imx219_set_anti_flicker,
	_imx219_set_video_mode,
	PNULL, //pick_jpeg_stream
	PNULL,  //meter_mode
	PNULL, //get_status
	_imx219_StreamOn,
	_imx219_StreamOff,
	PNULL,//_imx219_cfg_otp,//
};


SENSOR_INFO_T g_imx219_mipi_raw_info = {
	IMX219_I2C_ADDR_W,	// salve i2c write address
	IMX219_I2C_ADDR_R,	// salve i2c read address

	SENSOR_I2C_REG_16BIT | SENSOR_I2C_VAL_8BIT | SENSOR_I2C_FREQ_400,	// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
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

	3264,			// max width of source image
	2448,			// max height of source image
	"imx219",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_B,//SENSOR_IMAGE_PATTERN_RAWRGB_R,// pattern of input image form sensor;

	s_imx219_resolution_Tab_RAW,	// point to resolution table information structure
	&s_imx219_ioctl_func_tab,	// point to ioctl function table
	&s_imx219_mipi_raw_info_ptr,		// information and table about Rawrgb sensor
	NULL,			//&g_imx219_ext_info,                // extend information about sensor
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
#if defined(IMX219_2_LANES)
	{SENSOR_INTERFACE_TYPE_CSI2, 2, 10, 0},
#elif defined(IMX219_4_LANES)
	{SENSOR_INTERFACE_TYPE_CSI2, 4, 10, 0},
#endif

	s_imx219_video_info,
	3,			// skip frame num while change setting
};

static struct sensor_raw_info* Sensor_GetContext(void)
{
	return s_imx219_mipi_raw_info_ptr;
}

static uint32_t Sensor_imx219_InitRawTuneInfo(void)
{
	uint32_t rtn=0x00;
	struct sensor_raw_info* raw_sensor_ptr=Sensor_GetContext();
	struct sensor_raw_tune_info* sensor_ptr=raw_sensor_ptr->tune_ptr;
	struct sensor_raw_cali_info* cali_ptr=raw_sensor_ptr->cali_ptr;

#if 1
	raw_sensor_ptr->version_info->version_id=0x00010000;
	raw_sensor_ptr->version_info->srtuct_size=sizeof(struct sensor_raw_info);

	//bypass
	sensor_ptr->version_id=0x00010000;

	sensor_ptr->blc_bypass = 1;
	sensor_ptr->nlc_bypass= 1;
	sensor_ptr->lnc_bypass = 1;
	sensor_ptr->ae_bypass = 1;
	sensor_ptr->awb_bypass = 1;
	sensor_ptr->bpc_bypass = 1;
	sensor_ptr->denoise_bypass = 1;
	sensor_ptr->grgb_bypass = 1;
	sensor_ptr->cmc_bypass = 1;
	sensor_ptr->gamma_bypass = 1;
	sensor_ptr->uvdiv_bypass = 1;
	sensor_ptr->pref_bypass = 1;
	sensor_ptr->bright_bypass = 1;
	sensor_ptr->contrast_bypass = 1;
	sensor_ptr->hist_bypass = 1;
	sensor_ptr->auto_contrast_bypass = 1;
	sensor_ptr->af_bypass = 1;
	sensor_ptr->edge_bypass = 1;
	sensor_ptr->fcs_bypass = 1;
	sensor_ptr->css_bypass = 1;
	sensor_ptr->saturation_bypass = 1;
	sensor_ptr->hdr_bypass = 1;
	sensor_ptr->glb_gain_bypass = 1;
	sensor_ptr->chn_gain_bypass = 1;


	//awb
	sensor_ptr->awb.alg_id = 1;
	sensor_ptr->awb.smart_index = 5;

	sensor_ptr->grgb.edge_thr = 0x3f;
	sensor_ptr->grgb.diff_thr = 0x3ff;


	sensor_ptr->ae.gamma_start = 0;
	sensor_ptr->ae.gamma_num = 1;
	sensor_ptr->ae.gamma_zone = 5;
	sensor_ptr->ae.gamma_thr[0] = 96;
//	sensor_ptr->ae.gamma_lun_thr = 60;

	sensor_ptr->edge.info[0].detail_thr= 0;
	sensor_ptr->edge.info[0].smooth_thr= 0;
	sensor_ptr->edge.info[0].strength= 12;

	sensor_ptr->edge.info[1].detail_thr= 0;
	sensor_ptr->edge.info[1].smooth_thr= 0;
	sensor_ptr->edge.info[1].strength= 14;

	sensor_ptr->edge.info[2].detail_thr= 0;
	sensor_ptr->edge.info[2].smooth_thr= 0;
	sensor_ptr->edge.info[2].strength= 16;

	sensor_ptr->edge.info[3].detail_thr= 0;
	sensor_ptr->edge.info[3].smooth_thr= 0;
	sensor_ptr->edge.info[3].strength= 17;

	sensor_ptr->edge.info[4].detail_thr= 0;
	sensor_ptr->edge.info[4].smooth_thr= 0;
	sensor_ptr->edge.info[4].strength= 18;

	sensor_ptr->edge.info[5].detail_thr= 0;
	sensor_ptr->edge.info[5].smooth_thr= 0;
	sensor_ptr->edge.info[5].strength= 23;

	sensor_ptr->ae.smart_edge_min_index = 0;
	sensor_ptr->ae.smart_edge_max_index = 5;
//	sensor_ptr->ae.smart_pref_y_min = 0x8;
//	sensor_ptr->ae.smart_pref_y_max = 0x40;
//	sensor_ptr->ae.smart_pref_uv_min = 0x40;
//	sensor_ptr->ae.smart_pref_uv_max = 0xC0;

//	sensor_ptr->ae.smart_denoise_diswei_outdoor_index = 0;
//	sensor_ptr->ae.smart_denoise_diswei_min_index = 2;
//	sensor_ptr->ae.smart_denoise_diswei_mid_index = 6;
//	sensor_ptr->ae.smart_denoise_diswei_max_index = 40;

//	sensor_ptr->ae.smart_denoise_ranwei_outdoor_index = 0;
//	sensor_ptr->ae.smart_denoise_ranwei_min_index = 10;
//	sensor_ptr->ae.smart_denoise_ranwei_mid_index = 20;
//	sensor_ptr->ae.smart_denoise_ranwei_max_index = 60;

//	sensor_ptr->ae.denoise_start_index = 91;
//	sensor_ptr->ae.denoise_start_zone = 5;
//	sensor_ptr->ae.denoise_lum_thr = 50;
	sensor_ptr->ae.smart_base_gain = 0x30;

//	sensor_ptr->ae.smart_denoise_soft_y_outdoor_index = 4;
//	sensor_ptr->ae.smart_denoise_soft_y_min_index = 4;
//	sensor_ptr->ae.smart_denoise_soft_y_mid_index = 5;
//	sensor_ptr->ae.smart_denoise_soft_y_max_index = 7;

//	sensor_ptr->ae.smart_denoise_soft_uv_outdoor_index = 0;
//	sensor_ptr->ae.smart_denoise_soft_uv_min_index = 0;
//	sensor_ptr->ae.smart_denoise_soft_uv_mid_index = 0;
//	sensor_ptr->ae.smart_denoise_soft_uv_max_index = 0;

	sensor_ptr->ae.smart_sta_start_index= 0;
	sensor_ptr->ae.smart_sta_low_thr = 0;
	sensor_ptr->ae.smart_sta_ratio1= 0;
	sensor_ptr->ae.smart_sta_ratio= 0;

	sensor_ptr->ae.smart = 0x73;

	sensor_ptr->saturation.factor[3] = 0X43;

	{
		uint32_t i=0;
#if 1
		for(i=0;i<2;i++){  //A light
			sensor_ptr->cmc.matrix[i][0]=0x052A;
			sensor_ptr->cmc.matrix[i][1]=0x3E9D;
			sensor_ptr->cmc.matrix[i][2]=0x0039;
			sensor_ptr->cmc.matrix[i][3]=0x3EB7;
			sensor_ptr->cmc.matrix[i][4]=0x0556;
			sensor_ptr->cmc.matrix[i][5]=0x3FF3;
			sensor_ptr->cmc.matrix[i][6]=0x004B;
			sensor_ptr->cmc.matrix[i][7]=0x3B57;
			sensor_ptr->cmc.matrix[i][8]=0x085E;

		}

		for(i=2;i<5;i++){//CWF
			sensor_ptr->cmc.matrix[i][0]=0x06C4;
			sensor_ptr->cmc.matrix[i][1]=0x3CAE;
			sensor_ptr->cmc.matrix[i][2]=0x008E;
			sensor_ptr->cmc.matrix[i][3]=0x3F31;
			sensor_ptr->cmc.matrix[i][4]=0x058D;
			sensor_ptr->cmc.matrix[i][5]=0x3F42;
			sensor_ptr->cmc.matrix[i][6]=0x0025;
			sensor_ptr->cmc.matrix[i][7]=0x3D52;
			sensor_ptr->cmc.matrix[i][8]=0x0689;
		}

		for(i=5;i<8;i++){//D light
	#if 0
			sensor_ptr->cmc.matrix[i][0]=0x0780;
			sensor_ptr->cmc.matrix[i][1]=0x3B03;
			sensor_ptr->cmc.matrix[i][2]=0x017D;
			sensor_ptr->cmc.matrix[i][3]=0x3F00;
			sensor_ptr->cmc.matrix[i][4]=0x05C7;
			sensor_ptr->cmc.matrix[i][5]=0x3F39;
			sensor_ptr->cmc.matrix[i][6]=0x3FD4;
			sensor_ptr->cmc.matrix[i][7]=0x3CE0;
			sensor_ptr->cmc.matrix[i][8]=0x074C;
	#else
			sensor_ptr->cmc.matrix[i][0]=0x070E;
			sensor_ptr->cmc.matrix[i][1]=0x3C72;
			sensor_ptr->cmc.matrix[i][2]=0x007F;
			sensor_ptr->cmc.matrix[i][3]=0x3EEE;
			sensor_ptr->cmc.matrix[i][4]=0x069F;
			sensor_ptr->cmc.matrix[i][5]=0x3E73;
			sensor_ptr->cmc.matrix[i][6]=0x0032;
			sensor_ptr->cmc.matrix[i][7]=0x3C67;
			sensor_ptr->cmc.matrix[i][8]=0x0766;
	#endif

		}

		for(i=8;i<9;i++){ //out door

	#if 0  // 0418
			sensor_ptr->cmc.matrix[i][0]=0x08DA;
			sensor_ptr->cmc.matrix[i][1]=0x3A55;
			sensor_ptr->cmc.matrix[i][2]=0x00D1;
			sensor_ptr->cmc.matrix[i][3]=0x3F37;
			sensor_ptr->cmc.matrix[i][4]=0x06DE;
			sensor_ptr->cmc.matrix[i][5]=0x3DEB;
			sensor_ptr->cmc.matrix[i][6]=0x3FF5;
			sensor_ptr->cmc.matrix[i][7]=0x3B4D;
			sensor_ptr->cmc.matrix[i][8]=0x08BE;
	#endif
	#if 0 // 04201:1:1
			sensor_ptr->cmc.matrix[i][0]=0x0844;
			sensor_ptr->cmc.matrix[i][1]=0x3A56;
			sensor_ptr->cmc.matrix[i][2]=0x0166;
			sensor_ptr->cmc.matrix[i][3]=0x3F0F;
			sensor_ptr->cmc.matrix[i][4]=0x0665;
			sensor_ptr->cmc.matrix[i][5]=0x3E8C;
			sensor_ptr->cmc.matrix[i][6]=0x006C;
			sensor_ptr->cmc.matrix[i][7]=0x3B92;
			sensor_ptr->cmc.matrix[i][8]=0x0802;
	#endif
	#if 0 // 0420   0.93:1:1.08
			sensor_ptr->cmc.matrix[i][0]=0x095B;
			sensor_ptr->cmc.matrix[i][1]=0x3996;
			sensor_ptr->cmc.matrix[i][2]=0x0196;
			sensor_ptr->cmc.matrix[i][3]=0x3F0C;
			sensor_ptr->cmc.matrix[i][4]=0x067A;
			sensor_ptr->cmc.matrix[i][5]=0x3E88;
			sensor_ptr->cmc.matrix[i][6]=0x005E;
			sensor_ptr->cmc.matrix[i][7]=0x3C29;
			sensor_ptr->cmc.matrix[i][8]=0x06F2;
	#endif
	#if 0 // 0421   0.91:1:1.1   0.98:1:1.02 cmc1
			sensor_ptr->cmc.matrix[i][0]=0x098A;
			sensor_ptr->cmc.matrix[i][1]=0x3976;
			sensor_ptr->cmc.matrix[i][2]=0x019E;
			sensor_ptr->cmc.matrix[i][3]=0x3EBE;
			sensor_ptr->cmc.matrix[i][4]=0x0665;
			sensor_ptr->cmc.matrix[i][5]=0x3EDE;
			sensor_ptr->cmc.matrix[i][6]=0x3FBA;
			sensor_ptr->cmc.matrix[i][7]=0x3CE5;
			sensor_ptr->cmc.matrix[i][8]=0x06C3;
	#endif
	#if 1 // 0421   0.91:1:1.1   1:1:1.01 cmc2
			sensor_ptr->cmc.matrix[i][0]=0x09BC;
			sensor_ptr->cmc.matrix[i][1]=0x3954;
			sensor_ptr->cmc.matrix[i][2]=0x01A6;
			sensor_ptr->cmc.matrix[i][3]=0x3E79;
			sensor_ptr->cmc.matrix[i][4]=0x064F;
			sensor_ptr->cmc.matrix[i][5]=0x3F2B;
			sensor_ptr->cmc.matrix[i][6]=0x3F97;
			sensor_ptr->cmc.matrix[i][7]=0x3D12;
			sensor_ptr->cmc.matrix[i][8]=0x06AD;
	#endif
	#if 0 // 0421   1:1:1  1:1:1 cmc3
			sensor_ptr->cmc.matrix[i][0]=0x0843;
			sensor_ptr->cmc.matrix[i][1]=0x3A56;
			sensor_ptr->cmc.matrix[i][2]=0x0166;
			sensor_ptr->cmc.matrix[i][3]=0x3F0F;
			sensor_ptr->cmc.matrix[i][4]=0x0665;
			sensor_ptr->cmc.matrix[i][5]=0x3E8C;
			sensor_ptr->cmc.matrix[i][6]=0x006C;
			sensor_ptr->cmc.matrix[i][7]=0x3B92;
			sensor_ptr->cmc.matrix[i][8]=0x0801;
	#endif
		}

	#if 0
		sensor_ptr->cmc.matrix[8][0]=0x06fd;
		sensor_ptr->cmc.matrix[8][1]=0x3cf8;
		sensor_ptr->cmc.matrix[8][2]=0x000b;
		sensor_ptr->cmc.matrix[8][3]=0x3Ea8;
		sensor_ptr->cmc.matrix[8][4]=0x064a;
		sensor_ptr->cmc.matrix[8][5]=0x3f0f;
		sensor_ptr->cmc.matrix[8][6]=0x3f6e;
		sensor_ptr->cmc.matrix[8][7]=0x3Ce6;
		sensor_ptr->cmc.matrix[8][8]=0x07ad;
	#endif
#else
		for(i=0;i<9;i++){
			sensor_ptr->cmc.matrix[i][0]=0x68f;
			sensor_ptr->cmc.matrix[i][1]=0x3c71;
			sensor_ptr->cmc.matrix[i][2]=0x100;
			sensor_ptr->cmc.matrix[i][3]=0x3f3e;
			sensor_ptr->cmc.matrix[i][4]=0x585;
			sensor_ptr->cmc.matrix[i][5]=0x3f3e;
			sensor_ptr->cmc.matrix[i][6]=0x33;
			sensor_ptr->cmc.matrix[i][7]=0x3d52;
			sensor_ptr->cmc.matrix[i][8]=0x685;
		}
#endif

	}

#endif

	return rtn;
}

static unsigned long _dw9714a_SRCInit(unsigned long mode)
{
	uint8_t cmd_val[2] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t ret_value = SENSOR_SUCCESS;
	SENSOR_PRINT("SENSOR_IMX219: %d",mode);

	slave_addr = DW9714A_VCM_SLAVE_ADDR;
	switch (mode) {
		case 1:
		break;

		case 2:
		{
#if 0
			cmd_len = 2;
			cmd_val[0] = 0xec;
			cmd_val[1] = 0xa3;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
			if(ret_value){
				SENSOR_PRINT("SENSOR_IMX219: _dw9174_SRCInit fail!1");
			}

			usleep(10*1000);
			cmd_val[0] = 0xf2;
			cmd_val[1] = 0x00;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
			if(ret_value){
				SENSOR_PRINT("SENSOR_IMX219: _dw9174_SRCInit fail!2");
			}
			usleep(10*1000);

			cmd_val[0] = 0xdc;
			cmd_val[1] = 0x51;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
			if(ret_value){
				SENSOR_PRINT("SENSOR_IMX219: _dw9174_SRCInit fail!3");
			}

			usleep(10*1000);
#else
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
#endif
		}
		break;

		case 3:
		break;

	}

	return ret_value;
}

static unsigned long _imx219_GetResolutionTrimTab(unsigned long param)
{
	SENSOR_PRINT("0x%lx",  (unsigned long)s_imx219_Resolution_Trim_Tab);
	return (unsigned long) s_imx219_Resolution_Trim_Tab;
}

static unsigned long _imx219_PowerOn(unsigned long power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_imx219_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_imx219_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_imx219_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_imx219_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_imx219_mipi_raw_info.reset_pulse_level;

	//uint32_t reset_width=g_imx219_yuv_info.reset_pulse_width;
	uint8_t pid_value = 0x00;

	if (SENSOR_TRUE == power_on) {
		Sensor_SetResetLevel(reset_level);
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		usleep(10*1000);
//		_dw9714a_SRCInit(2);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(10*1000);
		Sensor_SetResetLevel(!reset_level);
		usleep(10*1000);
	} else {
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
	}

	SENSOR_PRINT_ERR("SENSOR_IMX219: _imx219_Power_On(1:on, 0:off): %d, reset_level %d, dvdd_val %d", power_on, reset_level, dvdd_val);
	return SENSOR_SUCCESS;
}

static unsigned long _imx219_cfg_otp(unsigned long  param)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_imx219_raw_param_tab;
	uint32_t module_id=g_module_id;

	SENSOR_PRINT("SENSOR_IMX219: _imx219_cfg_otp");

	if(PNULL!=tab_ptr[module_id].cfg_otp){
		tab_ptr[module_id].cfg_otp(0);
		}

	return rtn;
}

static uint32_t _imx219_com_Identify_otp(void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_IMX219: _imx219_com_Identify_otp");

	/*read param id from sensor omap*/
	param_id=IMX219_RAW_PARAM_COM;

	if(IMX219_RAW_PARAM_COM==param_id){
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}

static uint32_t _imx219_GetRawInof(void)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_imx219_raw_param_tab;
	uint32_t param_id;
	uint32_t i=0x00;

	/*read param id from sensor omap*/
	param_id=IMX219_RAW_PARAM_COM;

	for(i=0x00; ; i++)
	{
		g_module_id = i;
		if(RAW_INFO_END_ID==tab_ptr[i].param_id){
			if(NULL==s_imx219_mipi_raw_info_ptr){
				SENSOR_PRINT("SENSOR_IMX219: ov5647_GetRawInof no param error");
				rtn=SENSOR_FAIL;
			}
			SENSOR_PRINT("SENSOR_IMX219: imx219_GetRawInof end");
			break;
		}
		else if(PNULL!=tab_ptr[i].identify_otp){
			if(SENSOR_SUCCESS==tab_ptr[i].identify_otp(0))
			{
				s_imx219_mipi_raw_info_ptr = tab_ptr[i].info_ptr;
				SENSOR_PRINT("SENSOR_IMX219: imx219_GetRawInof success");
				break;
			}
		}
	}

	return rtn;
}

static unsigned long _imx219_GetMaxFrameLine(unsigned long index)
{
	uint32_t max_line=0x00;
	SENSOR_TRIM_T_PTR trim_ptr=s_imx219_Resolution_Trim_Tab;

	max_line=trim_ptr[index].frame_line;

	return max_line;
}

static unsigned long _imx219_Identify(unsigned long param)
{
#define IMX219_PID_VALUE    0x02
#define IMX219_PID_ADDR     0x0000
#define IMX219_VER_VALUE    0x19
#define IMX219_VER_ADDR     0x0001
    uint8_t i=0x00;
	uint8_t pid_value = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT_ERR("SENSOR_IMX219: mipi raw identify\n");
//	while (1) {
	pid_value = Sensor_ReadReg(IMX219_PID_ADDR);
//	SENSOR_PRINT_ERR("SENSOR_IMX219: Identify: VER = %x", pid_value);

	if (IMX219_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(IMX219_VER_ADDR);
		SENSOR_PRINT("SENSOR_IMX219: Identify: PID = %x, VER = %x", pid_value, ver_value);
		if (IMX219_VER_VALUE == ver_value) {
			SENSOR_PRINT_ERR("SENSOR_IMX219: this is IMX219 sensor !");
			ret_value=_imx219_GetRawInof();
			if(SENSOR_SUCCESS != ret_value)
			{
				SENSOR_PRINT_ERR("SENSOR_IMX219: the module is unknow error !");
			}
			Sensor_imx219_InitRawTuneInfo();
		} else {
			SENSOR_PRINT_ERR("SENSOR_IMX219: Identify this is hm%x%x sensor !", pid_value, ver_value);
			return ret_value;
		}
	} else {
		SENSOR_PRINT_ERR("SENSOR_IMX219: identify fail,pid_value=%d", pid_value);
	}
//	usleep(5*1000);
//	}

	return ret_value;
}

static unsigned long _imx219_write_exposure(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t dummy_line = 0x00;
	uint16_t expsure_line = 0x00;
	uint16_t frame_len_cur = 0x00;
	uint16_t frame_len = 0x00;

#if 0
	expsure_line = param & 0xffff;
	dummy_line = (param >> 16) & 0xffff;
	if(expsure_line < 1){
		expsure_line = 1;
	}

	//frame_len = expsure_line + 4;
	frame_len = expsure_line + dummy_line;
	frame_len = (frame_len > (expsure_line + 4)) ? frame_len : (expsure_line + 4);
	frame_len = (frame_len > IMX219_MIN_FRAME_LEN_PRV) ? frame_len :  IMX219_MIN_FRAME_LEN_PRV;
	frame_len_cur = (Sensor_ReadReg(0x0163)) & 0xff;
	frame_len_cur |= (Sensor_ReadReg(0x0162) << 0x08) & 0xff00;

	SENSOR_PRINT("SENSOR_IMX219: write_exposure line:0x%x, dummy_line:0x%x, frame_len_cur:0x%x, frame_len:0x%x",
		expsure_line, dummy_line, frame_len_cur, frame_len);

	//ret_value = Sensor_WriteReg(0x0104, 0x01);
	if(frame_len_cur < frame_len){
		ret_value = Sensor_WriteReg(0x0163, frame_len & 0xff);
		ret_value = Sensor_WriteReg(0x0162, (frame_len >> 0x08) & 0xff);
	}

	ret_value = Sensor_WriteReg(0x15B, expsure_line & 0xff);
	ret_value = Sensor_WriteReg(0x15A, (expsure_line >> 0x08) & 0xff);

	if(frame_len_cur > frame_len){
		ret_value = Sensor_WriteReg(0x0163, frame_len & 0xff);
		ret_value = Sensor_WriteReg(0x0162, (frame_len >> 0x08) & 0xff);
	}

	//ret_value = Sensor_WriteReg(0x0104, 0x00);
	return ret_value;
#else
	uint16_t size_index=0x00;
	uint16_t max_frame_len=0x00;
	//uint16_t frame_length_lines_uint=0x01;

	expsure_line = param & 0xffff;
	dummy_line = (param >> 16) & 0x0fff;
	size_index =(param>>0x1c)&0x0f;

	if(expsure_line < 1){
	expsure_line = 1;
	}

#if 0
	max_frame_len=_imx219_GetMaxFrameLine(size_index);

 //   if((Sensor_ReadReg(0x0175)& 0x03)== 0x02) //binning mode = 3
//		frame_length_lines_uint = 0x02 ;

	//frame_len = expsure_line + 4;
	frame_len = expsure_line + dummy_line;
	frame_len = (frame_len > (expsure_line + 4)) ? frame_len : (expsure_line + 4);
	frame_len = (frame_len > max_frame_len) ? frame_len :  max_frame_len;
//	frame_len_cur = (Sensor_ReadReg(0x0161)) & 0xff;
//	frame_len_cur |= (Sensor_ReadReg(0x0160) << 0x08) & 0xff00;
//	frame_len_cur *= frame_length_lines_uint;

	SENSOR_PRINT("SENSOR_IMX219: write_exposure line:0x%x, dummy_line:0x%x, frame_len_cur:0x%x, frame_len:0x%x",
	expsure_line, dummy_line, frame_len_cur, frame_len);

	ret_value = Sensor_WriteReg(0x0104, 0x01); //thomaszhang debug 20140527

	ret_value = _imx219_set_VTS(frame_len);
	//ret_value = Sensor_WriteReg(0x0161, frame_len & 0xff);
	//ret_value = Sensor_WriteReg(0x0160, (frame_len >> 0x08) & 0xff);

	ret_value = _imx219_set_shutter(expsure_line);
	//ret_value = Sensor_WriteReg(0x15B, expsure_line & 0xff);
	//ret_value = Sensor_WriteReg(0x15A, (expsure_line >> 0x08) & 0xff);

	ret_value = Sensor_WriteReg(0x0104, 0x00); //thomaszhang debug 20140527
#else
	max_frame_len=_imx219_GetMaxFrameLine(size_index);

 //   if((Sensor_ReadReg(0x0175)& 0x03)== 0x02) //binning mode = 3
//		frame_length_lines_uint = 0x02 ;

	frame_len = expsure_line + 4;
//	frame_len = (frame_len > (expsure_line + 4)) ? frame_len : (expsure_line + 4);
	frame_len = (frame_len > max_frame_len) ? frame_len :  max_frame_len;

	SENSOR_PRINT("SENSOR_IMX219: write_exposure line:0x%x, dummy_line:0x%x, frame_len_cur:0x%x, frame_len:0x%x",
	expsure_line, dummy_line, frame_len_cur, frame_len);

	ret_value = _imx219_set_VTS(frame_len);
	//ret_value = Sensor_WriteReg(0x0161, frame_len & 0xff);
	//ret_value = Sensor_WriteReg(0x0160, (frame_len >> 0x08) & 0xff);

	ret_value = _imx219_set_shutter(expsure_line);
	//ret_value = Sensor_WriteReg(0x15B, expsure_line & 0xff);
	//ret_value = Sensor_WriteReg(0x15A, (expsure_line >> 0x08) & 0xff);

#endif
	return ret_value;
#endif
}

static unsigned long _imx219_write_gain(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint32_t real_gain = 0;

	real_gain = ((param&0xf)+16)*(((param>>4)&0x01)+1)*(((param>>5)&0x01)+1);
	real_gain = real_gain*(((param>>6)&0x01)+1)*(((param>>7)&0x01)+1)*(((param>>8)&0x01)+1);
	real_gain = real_gain*(((param>>9)&0x01)+1)*(((param>>10)&0x01)+1)*(((param>>11)&0x01)+1);

	SENSOR_PRINT("SENSOR_IMX219: rea_gain %d, %d", real_gain, real_gain/16);
	value = ((real_gain -16) * 256)/real_gain;

	ret_value = Sensor_WriteReg(0x0157, value);

	SENSOR_PRINT("SENSOR_IMX219: param: 0x%x, write_gain:0x%x, ret_value: %d", param, value, ret_value);

	return ret_value;
}

static unsigned long _imx219_write_af(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint8_t cmd_val[2] = {0x00};
	uint16_t slave_addr = 0;
	uint16_t cmd_len = 0;

	SENSOR_PRINT("SENSOR_IMX219: _write_af %d", param);
	slave_addr = DW9714A_VCM_SLAVE_ADDR;
	cmd_val[0] = (param&0x3ff0)>>4;
	cmd_val[1] = ((param&0x0f)<<4)|0x05;
	cmd_len = 2;
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

	SENSOR_PRINT("SENSOR_IMX219: _write_af, ret =  %d, MSL:%x, LSL:%x\n", ret_value, cmd_val[0], cmd_val[1]);
	return ret_value;
}

static unsigned long _imx219_BeforeSnapshot(unsigned long param)
{
	uint8_t ret_l, ret_m, ret_h;
	uint32_t capture_exposure, preview_maxline;
	uint32_t capture_maxline, preview_exposure;
	uint32_t capture_mode = param & 0xffff;
	uint32_t preview_mode = (param >> 0x10 ) & 0xffff;
	uint32_t prv_linetime=s_imx219_Resolution_Trim_Tab[preview_mode].line_time;
	uint32_t cap_linetime = s_imx219_Resolution_Trim_Tab[capture_mode].line_time;

	SENSOR_PRINT("SENSOR_IMX219: BeforeSnapshot mode: 0x%08x",param);

	if (preview_mode == capture_mode) {
		SENSOR_PRINT("SENSOR_IMX219: prv mode equal to capmode");
		goto CFG_INFO;
	}

	preview_exposure = _imx219_get_shutter();
	preview_maxline	= _imx219_get_VTS();

	Sensor_SetMode(capture_mode);
	Sensor_SetMode_WaitDone();

	if (prv_linetime == cap_linetime) {
		SENSOR_PRINT("SENSOR_IMX219: prvline equal to capline");
		goto CFG_INFO;
	}

	capture_maxline = _imx219_get_VTS();

	capture_exposure = preview_exposure * prv_linetime/cap_linetime;

	SENSOR_PRINT("BeforeSnapshot: capture_exposure 0x%x, capture_maxline 0x%x,preview_maxline 0x%x, preview_exposure 0x%x",
		capture_exposure,capture_maxline, preview_maxline, preview_exposure);

	if(0 == capture_exposure){
		capture_exposure = 1;
	}

	if(capture_exposure > (capture_maxline - 4)){
		capture_maxline = capture_exposure + 4;
//		capture_maxline = (capture_maxline+1)>>1<<1;
//		ret_l = (unsigned char)(capture_maxline&0x0ff);
//		ret_h = (unsigned char)((capture_maxline >> 8)&0xff);
		_imx219_set_VTS(capture_maxline);
	}

/*
	ret_l = ((unsigned char)capture_exposure&0xf) << 4;
	ret_m = (unsigned char)((capture_exposure&0xfff) >> 4) & 0xff;
	ret_h = (unsigned char)(capture_exposure >> 12);
	*/
	_imx219_set_shutter(capture_exposure);

	CFG_INFO:
	s_capture_shutter = _imx219_get_shutter();
	s_capture_VTS = _imx219_get_VTS();
	_imx219_ReadGain(capture_mode);
	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, s_capture_shutter);

	return SENSOR_SUCCESS;
}

static unsigned long _imx219_after_snapshot(unsigned long param)
{
	SENSOR_PRINT("SENSOR_imx219: after_snapshot mode:%ld", param);
	Sensor_SetMode((uint32_t)param);
	return SENSOR_SUCCESS;
}

static unsigned long _imx219_flash(unsigned long param)
{
	SENSOR_PRINT("Start:param=%d", param);

	/* enable flash, disable in _imx219_BeforeSnapshot */
	g_flash_mode_en = param;
	Sensor_SetFlash(param);
	SENSOR_PRINT_HIGH("end");
	return SENSOR_SUCCESS;
}

static unsigned long _imx219_StreamOn(unsigned long param)
{
	SENSOR_PRINT_ERR("SENSOR_imx219: StreamOn");

	Sensor_WriteReg(0x0100, 0x01);

	return 0;
}

static unsigned long _imx219_StreamOff(unsigned long param)
{
	SENSOR_PRINT_ERR("SENSOR_imx219: StreamOff");

	Sensor_WriteReg(0x0100, 0x00);
	usleep(10*1000);

	return 0;
}

static uint16_t _imx219_get_shutter(void)
{
	// read shutter, in number of line period
	uint16_t shutter;

	shutter = (Sensor_ReadReg(0x015A)<< 8) + Sensor_ReadReg(0x015B);

	return shutter;
}

static uint32_t _imx219_set_shutter(uint16_t shutter)
{
	// write shutter, in number of line period
	Sensor_WriteReg(0x015B, (shutter & 0xff));
	Sensor_WriteReg(0x015A, ((shutter >>8) & 0xff));

	return 0;
}

static uint32_t _imx219_get_gain16(void)
{
	// read gain, 16 = 1x
	uint32_t gain16;

	gain16 = (256*16)/(256 - Sensor_ReadReg(0x0157)); // a_gain= 256/(256-x);

	return gain16;
}

static uint16_t _imx219_set_gain16(uint32_t gain16)
{
	// write gain, 16 = 1x
	uint16_t temp;
	gain16 = gain16 & 0x3ff;

	if(gain16 < 16)
		gain16 = 16;
	 if(gain16 > 170)
		gain16 = 170;

	temp = (256*(gain16- 16))/gain16;
	Sensor_WriteReg(0x0157, temp&0xff);

	return 0;
}

static void _calculate_hdr_exposure(int capture_gain16,int capture_VTS, int capture_shutter)
{
	// write capture gain
	_imx219_set_gain16(capture_gain16);

	// write capture shutter
	/*if (capture_shutter > (capture_VTS - 4)) {
		capture_VTS = capture_shutter + 4;
		OV5640_set_VTS(capture_VTS);
	}*/
	_imx219_set_shutter(capture_shutter);
}

static unsigned long _imx219_SetEV(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	uint16_t value=0x00;
	uint32_t gain = s_imx219_gain;
	uint32_t ev = ext_ptr->param;

	SENSOR_PRINT("SENSOR: _imx219_SetEV param: 0x%x", ext_ptr->param);

	switch(ev) {
	case SENSOR_HDR_EV_LEVE_0:
		_calculate_hdr_exposure(s_imx219_gain/4,s_capture_VTS,s_capture_shutter/2);
		break;
	case SENSOR_HDR_EV_LEVE_1:
		_calculate_hdr_exposure(s_imx219_gain,s_capture_VTS,s_capture_shutter);
		break;
	case SENSOR_HDR_EV_LEVE_2:
		_calculate_hdr_exposure(s_imx219_gain*3/2,s_capture_VTS,s_capture_shutter);
		break;
	default:
		break;
	}
	return rtn;
}

static unsigned long _imx219_ExtFunc(unsigned long ctl_param)
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
		rtn = _imx219_SetEV(ctl_param);
		break;
	default:
		break;
	}
	return rtn;
}

static uint16_t _imx219_get_VTS(void)
{
	// read VTS from register settings
	uint16_t VTS;

	VTS = Sensor_ReadReg(0x0160);				//total vertical size[15:8] high byte
	VTS = (VTS<<8) + Sensor_ReadReg(0x0161);

	return VTS;
}

static uint32_t _imx219_set_VTS(uint16_t VTS)
{
	// write VTS to registers
	Sensor_WriteReg(0x0161, (VTS & 0xff));
	Sensor_WriteReg(0x0160, ((VTS>>8)& 0xff));

	return 0;
}

static unsigned long _imx219_ReadGain(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	uint32_t gain = 0;

	gain = Sensor_ReadReg(0x0157);

	s_imx219_gain=(int)gain;

	SENSOR_PRINT("SENSOR_imx219: _imx219_ReadGain gain: 0x%x", s_imx219_gain);

	return rtn;
}
