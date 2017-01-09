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

 #define LOG_TAG "sensor_pattern.c"

#include <utils/Log.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "sensor.h"
#include "pattern_regs.h"
#include "jpeg_exif_header.h"

#define dcam_patternI2C_ADDR_W 0
#define dcam_patternI2C_ADDR_R 0
#define POL (1 << SHIFT_HSYNC_POL) | ( 1 << SHIFT_VSYNC_POL)

typedef struct {
	uint32_t reg_addr;
	uint32_t reg_value;
	uint32_t reg_mask;
} SENSOR_REG_T2;;

static int g_fd_pattern = -1;

LOCAL const SENSOR_REG_T2 dcam_patterncommon_init[] = {
	{REG_MM_AHB_GEN_CKG_CFG, BIT_MM_MTX_AXI_CKG_EN | BIT_MM_AXI_CKG_EN | BIT_DCAM_AXI_CKG_EN | BIT_SENSOR_CKG_EN, BIT_MM_MTX_AXI_CKG_EN | BIT_MM_AXI_CKG_EN | BIT_DCAM_AXI_CKG_EN | BIT_SENSOR_CKG_EN},
	{REG_MM_AHB_AHB_EB, BIT_MM_CKG_EB | BIT_CCIR_EB | BIT_DCAM_EB, BIT_MM_CKG_EB | BIT_CCIR_EB | BIT_DCAM_EB},
	{REG_MM_AHB_AHB_RST, BIT_MM_CKG_SOFT_RST | BIT_CCIR_SOFT_RST | BIT_DCAM_SOFT_RST, BIT_MM_CKG_SOFT_RST | BIT_CCIR_SOFT_RST | BIT_DCAM_SOFT_RST },
	{SENSOR_WRITE_DELAY, 10, 0xFFFFFFFF},
	{REG_MM_AHB_AHB_RST, ~(BIT_MM_CKG_SOFT_RST | BIT_CCIR_SOFT_RST | BIT_DCAM_SOFT_RST) , BIT_MM_CKG_SOFT_RST | BIT_CCIR_SOFT_RST | BIT_DCAM_SOFT_RST },
	{SENSOR_WRITE_DELAY, 10, 0xFFFFFFFF},

	{DCAM_CONTROL, 1 << SHIFT_PATTERN_CLEAR, PATTERN_CLEAR},
	{SENSOR_WRITE_DELAY, 10, 0xFFFFFFFF},
	{DCAM_CONTROL, 0 << SHIFT_PATTERN_CLEAR, PATTERN_CLEAR},
	{SENSOR_WRITE_DELAY, 10, 0xFFFFFFFF},

	{DCAM_CFG, PATTERN_SEL, PATTERN_SEL},
	{REG_PATTERN_CFG, POL | COLOR_MODE_YUV | SEQ_MODE_INTER | COLORBAR, 0xFFFFFFFF},
	{REG_PATTERN_SIZE, (480 << 16) | 640, 0x0FFF0FFF},
	{REG_PATTERN_VBLANK, (4 << 16) | (4 << 8)|16, 0x00FFFFFF},
	{REG_PATTERN_HBLANK, 64, 0x0000FFFF},
	{DCAM_CFG, PATTERN_SEL, PATTERN_SEL},
};

LOCAL const SENSOR_REG_T2 dcam_pattern640X480[] = {
	{REG_PATTERN_CFG, POL | COLOR_MODE_YUV | SEQ_MODE_INTER | COLORBAR, 0xFFFFFFFF},
	{REG_PATTERN_VBLANK, (4 << 16)|(4 << 8) | 16, 0x00FFFFFF},
	{REG_PATTERN_HBLANK, 64, 0x0000FFFF},
	{DCAM_CFG, PATTERN_SEL, PATTERN_SEL},

	{REG_PATTERN_SIZE, (480 << 16) | 640, 0x0FFF0FFF},
	{SENSOR_WRITE_DELAY, 10, 0xFFFFFFFF},
	{DCAM_CONTROL, PATTERN_START, PATTERN_START},
};

LOCAL const SENSOR_REG_T2 dcam_pattern1280X960[] = {
	{REG_PATTERN_CFG, POL | COLOR_MODE_YUV | SEQ_MODE_INTER | COLORBAR, 0xFFFFFFFF},
	{REG_PATTERN_VBLANK, (4<<16)|(4<<8) |16, 0x00FFFFFF},
	{REG_PATTERN_HBLANK, 64, 0x0000FFFF},
	{DCAM_CFG, PATTERN_SEL, PATTERN_SEL},

	{REG_PATTERN_SIZE, (960 << 16) |1280, 0x0FFF0FFF},
	{SENSOR_WRITE_DELAY, 10, 0xFFFFFFFF},
	{DCAM_CONTROL, PATTERN_START, PATTERN_START},
};

LOCAL const SENSOR_REG_T2 dcam_pattern1600X1200[] = {
	{REG_PATTERN_CFG, POL | COLOR_MODE_YUV | SEQ_MODE_INTER | COLORBAR, 0xFFFFFFFF},
	{REG_PATTERN_VBLANK, (4<<16)|(4<<8)|16,0x00FFFFFF},
	{REG_PATTERN_HBLANK, 64,0x0000FFFF},
	{DCAM_CFG,PATTERN_SEL ,PATTERN_SEL},

	{REG_PATTERN_SIZE, (1200 << 16)| (1600), 0x0FFF0FFF},
	{SENSOR_WRITE_DELAY, 10, 0xFFFFFFFF},
	{DCAM_CONTROL, PATTERN_START, PATTERN_START},
};

LOCAL const SENSOR_REG_T2 dcam_pattern2048X1536[] = {
	{DCAM_CONTROL, 1 << SHIFT_PATTERN_CLEAR, PATTERN_CLEAR},
	{SENSOR_WRITE_DELAY, 10, 0xFFFFFFFF},
	{DCAM_CONTROL, 0 << SHIFT_PATTERN_CLEAR, PATTERN_CLEAR},
	{REG_PATTERN_SIZE, (1536 << 16) | (2048), 0x0FFF0FFF},
	{SENSOR_WRITE_DELAY, 10, 0xFFFFFFFF},
	{DCAM_CONTROL,PATTERN_START,PATTERN_START},
};

LOCAL const SENSOR_REG_T2 dcam_pattern2592X1944[] = {
	{DCAM_CONTROL, 1 << SHIFT_PATTERN_CLEAR, PATTERN_CLEAR},
	{SENSOR_WRITE_DELAY, 10, 0xFFFFFFFF},
	{DCAM_CONTROL, 0 << SHIFT_PATTERN_CLEAR, PATTERN_CLEAR},
	{REG_PATTERN_SIZE, (1944 << 16) | (2592), 0x0FFF0FFF},
	{SENSOR_WRITE_DELAY, 10, 0xFFFFFFFF},
	{DCAM_CONTROL,PATTERN_START,PATTERN_START},
};

LOCAL SENSOR_REG_TAB_INFO_T _resolution_Tab_YUV[] = {
	{ADDR_AND_LEN_OF_ARRAY(dcam_patterncommon_init), 0, 0, 12,
		SENSOR_IMAGE_FORMAT_YUV422},
	{ADDR_AND_LEN_OF_ARRAY(dcam_pattern640X480), 640, 480, 12,
		SENSOR_IMAGE_FORMAT_YUV422},
	{ADDR_AND_LEN_OF_ARRAY(dcam_pattern1280X960), 1280, 960, 12,
		SENSOR_IMAGE_FORMAT_YUV422},
	{ADDR_AND_LEN_OF_ARRAY(dcam_pattern1600X1200), 1600, 1200, 12,
		SENSOR_IMAGE_FORMAT_YUV422},
	{ADDR_AND_LEN_OF_ARRAY(dcam_pattern2048X1536), 2048, 1536, 12,
		SENSOR_IMAGE_FORMAT_YUV422},
	{ADDR_AND_LEN_OF_ARRAY(dcam_pattern2592X1944), 2592, 1944, 12,
		SENSOR_IMAGE_FORMAT_YUV422},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

LOCAL SENSOR_TRIM_T sResolution_Trim_Tab[] = {
	{0, 0, 640, 480, 0, 0, 0, {0, 0, 640, 480}},
	{0, 0, 640, 480, 68, 56, 0, {0, 0, 640, 480}},
	{0, 0, 1280, 960, 122, 42, 0, {0, 0, 1280, 960}},
	{0, 0, 1600, 1200, 122, 42, 0, {0, 0, 1600, 1200}},
	{0, 0, 2048, 1536, 122, 42, 0, {0, 0, 2048, 1536}},
	{0, 0, 2592, 1944, 122, 42, 0, {0, 0, 2592, 1944}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}}
};

LOCAL const SENSOR_REG_T2 dcam_patterncommon_init_RAWRGB[] = {
	{DCAM_CFG, PATTERN_SEL, 0xFFFFFFFF},
	{REG_PATTERN_SIZE, 0x00, 0xFFFFFFFF},
	{REG_PATTERN_VBLANK, 0x00, 0xFFFFFFFF},
	{REG_PATTERN_HBLANK, 0x00, 0xFFFFFFFF},
	{REG_PATTERN_CFG, 0x00, 0xFFFFFFFF},
};

LOCAL const SENSOR_REG_T2 dcam_pattern640X480_RAWRGB[] = {
	{DCAM_CFG, PATTERN_SEL, 0xFFFFFFFF},
	{REG_PATTERN_SIZE, (640 << 16) | (480), 0xFFFFFFFF},
	{REG_PATTERN_VBLANK, (3 << 16) | (3 << 8) | 3, 0xFFFFFFFF},
	{REG_PATTERN_HBLANK, 16, 0xFFFFFFFF},
	{REG_PATTERN_CFG, (0 << 3) | 1, 0xFFFFFFFF},
};

LOCAL const SENSOR_REG_T2 dcam_pattern1280X960_RAWRGB[] = {
	{DCAM_CFG, PATTERN_SEL, 0xFFFFFFFF},
	{REG_PATTERN_SIZE, (1280 << 16) | (960), 0xFFFFFFFF},
	{REG_PATTERN_VBLANK, (3 << 16) | (3 << 8) | 3, 0xFFFFFFFF},
	{REG_PATTERN_HBLANK, 16, 0xFFFFFFFF},
	{REG_PATTERN_CFG, 0, 0xFFFFFFFF},
};

LOCAL const SENSOR_REG_T2 dcam_pattern1600X1200_RAWRGB[] = {
	{DCAM_CFG, PATTERN_SEL, 0xFFFFFFFF},
	{REG_PATTERN_SIZE, (1600 << 16 ) | (1200), 0xFFFFFFFF},
	{REG_PATTERN_VBLANK, (3 << 16 ) | (3 << 8) | 3, 0xFFFFFFFF},
	{REG_PATTERN_HBLANK, 16, 0xFFFFFFFF},
	{REG_PATTERN_CFG, 0, 0xFFFFFFFF},
};

LOCAL const SENSOR_REG_T2 dcam_pattern2048X1536_RAWRGB[] = {
	{DCAM_CFG, PATTERN_SEL, 0xFFFFFFFF},
	{REG_PATTERN_SIZE, (2048 << 16) | (1536), 0xFFFFFFFF},
	{REG_PATTERN_VBLANK, (3 << 16) | (3 << 8) | 3, 0xFFFFFFFF},
	{REG_PATTERN_HBLANK, 16, 0xFFFFFFFF},
	{REG_PATTERN_CFG, 0, 0xFFFFFFFF},
};

LOCAL const SENSOR_REG_T2 dcam_pattern2592X1944_RAWRGB[] = {
	{DCAM_CFG, PATTERN_SEL, 0xFFFFFFFF},
	{REG_PATTERN_SIZE, (2592 << 16) | (1944), 0xFFFFFFFF},
	{REG_PATTERN_VBLANK, (3 << 16) | (3 << 8) | 3, 0xFFFFFFFF},
	{REG_PATTERN_HBLANK, 16, 0xFFFFFFFF},
	{REG_PATTERN_CFG, 0, 0xFFFFFFFF},
};

LOCAL SENSOR_REG_TAB_INFO_T sresolution_Tab_RAWRGB[] = {
	{ADDR_AND_LEN_OF_ARRAY(dcam_patterncommon_init_RAWRGB), 0, 0, 24,
		SENSOR_IMAGE_FORMAT_YUV422},
	{ADDR_AND_LEN_OF_ARRAY(dcam_pattern640X480_RAWRGB), 640, 480, 24,
		SENSOR_IMAGE_FORMAT_YUV422},
	{ADDR_AND_LEN_OF_ARRAY(dcam_pattern1280X960_RAWRGB), 1280, 960, 24,
		SENSOR_IMAGE_FORMAT_YUV422},
	{ADDR_AND_LEN_OF_ARRAY(dcam_pattern1600X1200_RAWRGB), 1600, 1200, 24,
		SENSOR_IMAGE_FORMAT_YUV422},
	{ADDR_AND_LEN_OF_ARRAY(dcam_pattern2048X1536_RAWRGB), 2048, 1536, 24,
		SENSOR_IMAGE_FORMAT_YUV422},
	{ADDR_AND_LEN_OF_ARRAY(dcam_pattern2592X1944_RAWRGB), 2592, 1944, 24,
		SENSOR_IMAGE_FORMAT_YUV422},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

static int xioctl(int fd, int request, void * arg)
{
	int r;

	r = ioctl(fd, request, arg);

	return r;
}

LOCAL uint32_t PowerOn(uint32_t power_on)
{
	int ret  = 0 ;

	return ret;
}

LOCAL uint32_t Identify(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;

	SENSOR_PRINT("SENSOR:CCIR  pattern sensor identify   OK.\n");

	return ret_value;
}


LOCAL uint32_t _write(uint32_t param)
{
	int ret;

	ret = xioctl(g_fd_pattern, SENSOR_IO_I2C_WRITE, &param);
	SENSOR_PRINT("SENSOR: ccir pattern  write");

	return ret;
}

LOCAL uint32_t _read(uint32_t param)
{
	int ret;

	ret = xioctl(g_fd_pattern, SENSOR_IO_I2C_READ, &param);
	SENSOR_PRINT("SENSOR: ccir pattern read");

	return 0;
}

LOCAL unsigned long GetResolutionTrimTab(unsigned long param)
{
	return (unsigned long) sResolution_Trim_Tab;
}


LOCAL uint32_t set_brightness(uint32_t level)
{
	uint16_t i = 0x00;
	uint32_t reg_value = 0;

	SENSOR_PRINT("0x%02x,data=0x%x", level, reg_value);

	return 0;
}

LOCAL uint32_t set_contrast(uint32_t level)
{
	uint16_t i = 0x00;
	uint32_t reg_value = 0;

	SENSOR_PRINT("0x%02x,data=0x%x", level, reg_value);

	return 0;
}

LOCAL uint32_t set_saturation(uint32_t level)
{
	uint16_t i = 0x00;
	uint32_t reg_value = 0;

	SENSOR_PRINT("0x%02x,data=0x%x", level, reg_value);

	return 0;
}

LOCAL uint32_t set_image_effect(uint32_t effect_type)
{
	return 0;
}

LOCAL uint32_t set_ev(uint32_t level)
{
	SENSOR_PRINT("0x%02x", level);

	return 0;
}

LOCAL uint32_t check_image_format_support(uint32_t param)
{
	uint32_t ret_val = SENSOR_SUCCESS;

	switch (param) {
	case SENSOR_IMAGE_FORMAT_YUV422:
	case SENSOR_IMAGE_FORMAT_RAW:
		break;
	default:
		ret_val = SENSOR_FAIL;
		break;
	}
	return ret_val;
}

LOCAL uint32_t chang_image_format(uint32_t param)
{
	uint32_t ret_val = SENSOR_FAIL;

	switch (param) {
	case SENSOR_IMAGE_FORMAT_YUV422:
		SENSOR_PRINT("SENSOR: pattern  chang_image_format  YUV422 \n");
		ret_val = Sensor_SendRegTabToSensor(&_resolution_Tab_YUV[1]);
		break;

	case SENSOR_IMAGE_FORMAT_RAW:
		SENSOR_PRINT("SENSOR: pattern  chang_image_format  YUV422 \n");
		ret_val = Sensor_SendRegTabToSensor(&sresolution_Tab_RAWRGB[1]);
		break;

	default:
		break;
	}

	return ret_val;
}

LOCAL uint32_t set_awb(uint32_t mode)
{

	return 0;
}

LOCAL uint32_t SetEV(uint32_t param)
{
	uint32_t rtn = SENSOR_SUCCESS;

	return rtn;
}

LOCAL uint32_t set_iso(uint32_t mode)
{
	return 0;
}

LOCAL unsigned long GetExifInfo(uint32_t param)
{
	LOCAL EXIF_SPEC_PIC_TAKING_COND_T sexif;

	return (unsigned long)&sexif;
}

LOCAL uint32_t ExtFunc(uint32_t ctl_param)
{
	uint32_t rtn = SENSOR_SUCCESS;

	return rtn;
}

LOCAL uint32_t set_anti_flicker(uint32_t mode)
{

	SENSOR_PRINT("0x%02x", mode);

	return 0;
}

LOCAL uint32_t set_work_mode(uint32_t mode)
{
	SENSOR_PRINT("SENSOR: set_work_mode: mode = %d", mode);

	return 0;
}

LOCAL uint32_t set_video_mode(uint32_t mode)
{
	SENSOR_PRINT("0x%02x", mode);

	return 0;
}

LOCAL uint32_t check_status(uint32_t param)
{
	return 0;
}

LOCAL uint32_t streamon(uint32_t param)
{
	int ret;
	param = 1;

	ret = xioctl(g_fd_pattern, SENSOR_IO_PD, &param);
	SENSOR_PRINT("SENSOR: ccir pattern streamon");

	return ret;
}

LOCAL uint32_t streamoff(uint32_t param)
{
	int ret;
	param = 0;

	ret = xioctl(g_fd_pattern, SENSOR_IO_PD, &param);
	SENSOR_PRINT("SENSOR: ccir pattern streamoff");

	return ret;
}


LOCAL SENSOR_IOCTL_FUNC_TAB_T _ioctl_func_tab = {
	PowerOn,
	PNULL,
	PNULL,
	Identify,

	PNULL,
	PNULL,
	PNULL,
	GetResolutionTrimTab,

	/*External*/
	PNULL,
	PNULL,
	PNULL,

	set_brightness,
	set_contrast,
	PNULL,
	PNULL,

	set_work_mode,
	set_image_effect,

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
	set_awb,
	PNULL,
	set_iso,
	set_ev,
	check_image_format_support,
	chang_image_format,
	PNULL,
	GetExifInfo,
	ExtFunc,
	set_anti_flicker,
	set_video_mode,
	PNULL,
	PNULL,
	check_status,
	PNULL,
	PNULL,
	PNULL,
};

SENSOR_INFO_T g_patternyuv_info = {
	dcam_patternI2C_ADDR_W,	// salve i2c _write address
	dcam_patternI2C_ADDR_R,	// salve i2c _read address

	SENSOR_I2C_CUSTOM| SENSOR_I2C_FREQ_400,	// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
	// bit1: 0: i2c register addr  is 8 bit, 1: i2c register addr  is 16 bit
	// other bit: reseved
	SENSOR_HW_SIGNAL_PCLK_N | SENSOR_HW_SIGNAL_VSYNC_P | SENSOR_HW_SIGNAL_HSYNC_P,	// bit0: 0:negative; 1:positive -> polarily of pixel clock
	// bit2: 0:negative; 1:positive -> polarily of horizontal synchronization signal
	// bit4: 0:negative; 1:positive -> polarily of vertical synchronization signal
	// other bit: reseved

	// preview mode
	SENSOR_ENVIROMENT_NORMAL,

	// image effect
	SENSOR_IMAGE_EFFECT_NORMAL,

	// while balance mode
	0,

	7,			// bit[0:7]: count of step in brightness, contrast, sharpness, saturation bit[8:31] reseved

	SENSOR_LOW_PULSE_RESET,	// reset pulse level
	10,			// reset pulse width(ms)

	SENSOR_HIGH_LEVEL_PWDN,	// 1: high level valid; 0: low level valid

	1,			// count of identify code
	{{0x0A, 0x56},		// supply two code to identify sensor.
	 {0x0B, 0x40}},		// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,	// voltage of avdd

	1<<12,			// max width of source image
	1<<12,			// max height of source image
	"pattern_yuv",		// name of sensor

	SENSOR_IMAGE_FORMAT_YUV422,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_YUV422_YUYV,	// pattern of input image form sensor;

	_resolution_Tab_YUV,	// point to resolution table information structure
	&_ioctl_func_tab,		// point to ioctl function table
	0,			// information and table about Rawrgb sensor
	NULL,			// extend information about sensor
	SENSOR_AVDD_1800MV,	// iovdd
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
	{SENSOR_INTERFACE_TYPE_CCIR601, 8, 16, 1},
	PNULL,
	3,			// skip frame num while change setting
};

SENSOR_INFO_T g_patternyuv_info_RAWRGB = {
	dcam_patternI2C_ADDR_W,	// salve i2c _write address
	dcam_patternI2C_ADDR_R,	// salve i2c _read address

	SENSOR_I2C_CUSTOM| SENSOR_I2C_FREQ_400,	// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
	// bit1: 0: i2c register addr  is 8 bit, 1: i2c register addr  is 16 bit
	// other bit: reseved
	SENSOR_HW_SIGNAL_PCLK_N | SENSOR_HW_SIGNAL_VSYNC_P | SENSOR_HW_SIGNAL_HSYNC_P,	// bit0: 0:negative; 1:positive -> polarily of pixel clock
	// bit2: 0:negative; 1:positive -> polarily of horizontal synchronization signal
	// bit4: 0:negative; 1:positive -> polarily of vertical synchronization signal
	// other bit: reseved

	// preview mode
	SENSOR_ENVIROMENT_NORMAL ,

	// image effect
	SENSOR_IMAGE_EFFECT_NORMAL,
	// while balance mode
	0,

	7,			// bit[0:7]: count of step in brightness, contrast, sharpness, saturation bit[8:31] reseved

	SENSOR_LOW_PULSE_RESET,	// reset pulse level
	10,			// reset pulse width(ms)

	SENSOR_HIGH_LEVEL_PWDN,	// 1: high level valid; 0: low level valid

	1,			// count of identify code
	{{0x0A, 0x56},		// supply two code to identify sensor.
	 {0x0B, 0x40}},		// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,	// voltage of avdd

	1<<12,			// max width of source image
	1<<12,			// max height of source image
	"pattern_RAW",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_GR,	// pattern of input image form sensor;

	sresolution_Tab_RAWRGB,	// point to resolution table information structure
	&_ioctl_func_tab,		// point to ioctl function table
	0,			// information and table about Rawrgb sensor
	NULL,			// extend information about sensor
	SENSOR_AVDD_1800MV,	// iovdd
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
	{SENSOR_INTERFACE_TYPE_CCIR601, 8, 16, 1},
	PNULL,
	3,			// skip frame num while change setting
};
