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
#include "sensor_cfg.h"
#include "sensor_drv_u.h"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define SENSOR_I2C_ADDR_W		0x3c
#define SENSOR_I2C_ADDR_R		0x3c

// autotest simulator sensor:
// 0x80 read only, value is 0x55
// 0x00 - 0x03 rw, read as write value
// 0x04 - 0x07 read only, value is: 0x12 0x34 0x56 0x78
#define SENSOR_PID_VALUE	    0x55
#define SENSOR_PID_ADDR 	    0x80
#define SENSOR_VER_VALUE	    0x12
#define SENSOR_VER_ADDR 	    0x04
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define FUN_ENTER               SENSOR_PRINT("sensor_autotst-> %s ++.\n", __FUNCTION__)
#define FUN_LEAVE               SENSOR_PRINT("sensor_autotst-> %s --.\n", __FUNCTION__)

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
LOCAL uint32_t autotst_PowerOn(uint32_t power_on);
LOCAL uint32_t autotst_Identify(uint32_t param);
LOCAL uint32_t autotst_check_image_format_support(uint32_t param);

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
LOCAL SENSOR_REG_TAB_INFO_T s_autotst_resolution_Tab_YUV[]=
{
	// COMMON INIT
	{ PNULL, 0, 640, 480, 24, SENSOR_IMAGE_FORMAT_YUV422},

	// YUV422 PREVIEW 1
	{ PNULL, 0, 640, 480, 24, SENSOR_IMAGE_FORMAT_YUV422},
	{ PNULL, 0, 0, 0, 0, 0},

	{ PNULL, 0, 0, 0, 0, 0},
	{ PNULL, 0, 0, 0, 0, 0},

	// YUV422 PREVIEW 2
	{ PNULL, 0, 0, 0, 0, 0},
	{ PNULL, 0, 0, 0, 0, 0},
	{ PNULL, 0, 0, 0, 0, 0},
	{ PNULL, 0, 0, 0, 0, 0}
};


LOCAL SENSOR_IOCTL_FUNC_TAB_T s_autotst_ioctl_func_tab =
{
	// Internal
	PNULL,
	autotst_PowerOn,
	PNULL,
	autotst_Identify,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	// External
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
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	autotst_check_image_format_support,
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
	PNULL
};

SENSOR_INFO_T g_autotest_yuv_info =
{
	SENSOR_I2C_ADDR_W,                // salve i2c write address
	SENSOR_I2C_ADDR_R,                // salve i2c read address

	SENSOR_I2C_FREQ_100,        // bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
			  // bit1: 0: i2c register addr  is 8 bit, 1: i2c register addr  is 16 bit
			  // other bit: reseved
	SENSOR_HW_SIGNAL_PCLK_N  | \
	SENSOR_HW_SIGNAL_VSYNC_N | \
	SENSOR_HW_SIGNAL_HSYNC_P,       // bit0: 0:negative; 1:positive -> polarily of pixel clock
	                            // bit2: 0:negative; 1:positive -> polarily of horizontal synchronization signal
	                            // bit4: 0:negative; 1:positive -> polarily of vertical synchronization signal
	                            // other bit: reseved

	// preview mode
	0,
	// image effect
	0,
	// while balance mode
	0,
	0,                              // bit[0:7]: count of step in brightness, contrast, sharpness, saturation
	                            // bit[8:31] reseved

	SENSOR_LOW_PULSE_RESET,            // reset pulse level
	10,            // reset pulse width(ms)
	SENSOR_HIGH_LEVEL_PWDN,            // 1: high level valid; 0: low level valid
	1,                              // count of identify code
	{{0x80, 0x55},                  // supply two code to identify sensor.
	{0x04, 0x12}},                  // for Example: index = 0-> Device id, index = 1 -> version id
	SENSOR_AVDD_2800MV,             // voltage of avdd
	640,                            // max width of source image
	480,                            // max height of source image
	"autotst_sensor",               // name of sensor

	SENSOR_IMAGE_FORMAT_MAX,     // define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	                            // if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_YUV422_YUYV,// pattern of input image form sensor;
	s_autotst_resolution_Tab_YUV,    // point to resolution table information structure
	&s_autotst_ioctl_func_tab,       // point to ioctl function table
	0,                           // information and table about Rawrgb sensor
	NULL,                            // extend information about sensor
	SENSOR_AVDD_1800MV,              // iovdd
	SENSOR_AVDD_1500MV,              // dvdd
	//
	0,                      // skip frame num before preview
	0,                      // skip frame num before capture
	0,                      // deci frame num during preview
	0,                      // deci frame num during video preview
	// threshold
	0,
	0,
	0,
	0,
	0, // i2c_dev_handler
	{SENSOR_INTERFACE_TYPE_CCIR601, 8, 16, 1},  //SENSOR_INTERFACE_CCIR601_8BITS
	PNULL,
	3,
};

LOCAL uint32_t autotst_PowerOn(uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val  = g_autotest_yuv_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val  = g_autotest_yuv_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_autotest_yuv_info.iovdd_val;
	BOOLEAN power_down = g_autotest_yuv_info.power_down_level;
	BOOLEAN reset_level = g_autotest_yuv_info.reset_pulse_level;


	if ( SENSOR_TRUE == power_on ) {
		Sensor_SetResetLevel(reset_level);
		Sensor_PowerDown(power_down);
		// Open power
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
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
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
	}

	SENSOR_PRINT("(1:on, 0:off)xiang: %d", power_on);
	return SENSOR_SUCCESS;
}

LOCAL uint32_t autotst_Identify(uint32_t param)
{
	return 0;
}

static uint32_t autotst_check_image_format_support(uint32_t param)
{
	uint32_t ret_val = SENSOR_FAIL;

	switch(param) {
	case SENSOR_IMAGE_FORMAT_YUV422:
	    ret_val = SENSOR_SUCCESS;
	    break;
	case SENSOR_IMAGE_FORMAT_JPEG:
	    ret_val = SENSOR_SUCCESS;
	    break;
	default:
	    break;
	}

	return ret_val;
}
