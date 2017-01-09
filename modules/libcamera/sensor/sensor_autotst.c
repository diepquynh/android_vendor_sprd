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
#define SENSOR_I2C_ADDR_W		0x80
#define SENSOR_I2C_ADDR_R		0x80

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
#define FUN_ENTER               pr_info("sensor_autotst-> %s ++.\n", __FUNCTION__)
#define FUN_LEAVE               pr_info("sensor_autotst-> %s --.\n", __FUNCTION__)

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
	{ PNULL, 0, 120, 120, 24, SENSOR_IMAGE_FORMAT_YUV422},

	// YUV422 PREVIEW 1
	{ PNULL, 0, 120, 120, 24, SENSOR_IMAGE_FORMAT_YUV422},
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
	PNULL
};

LOCAL SENSOR_INFO_T g_autotst_yuv_info =
{
	SENSOR_I2C_ADDR_W,                // salve i2c write address
	SENSOR_I2C_ADDR_R,                // salve i2c read address

	0,        // bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
			  // bit1: 0: i2c register addr  is 8 bit, 1: i2c register addr  is 16 bit
			  // other bit: reseved
	SENSOR_HW_SIGNAL_PCLK_P  | \
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

	0,            // reset pulse level
	0,            // reset pulse width(ms)
	0,            // 1: high level valid; 0: low level valid

	0,                              // count of identify code
	{{0x00, 0x00},                  // supply two code to identify sensor.
	{0x00, 0x00}},                  // for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,             // voltage of avdd

	240,                            // max width of source image
	320,                            // max height of source image
	"autotst_sensor",               // name of sensor

	SENSOR_IMAGE_FORMAT_YUV422,     // define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	                            // if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_YUV422_YUYV,// pattern of input image form sensor;

	s_autotst_resolution_Tab_YUV,    // point to resolution table information structure
	&s_autotst_ioctl_func_tab,       // point to ioctl function table
	PNULL,                           // information and table about Rawrgb sensor
	NULL,                            // extend information about sensor
	SENSOR_AVDD_2800MV,              // iovdd
	SENSOR_AVDD_1800MV,              // dvdd
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
	0
};

LOCAL uint32_t autotst_PowerOn(uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val  = g_autotst_yuv_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val  = g_autotst_yuv_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_autotst_yuv_info.iovdd_val;

	FUN_ENTER;

	if( SENSOR_TRUE == power_on ) {
		//--Sensor_PowerDown(power_down);
		// Open power
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		msleep(10);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);

		msleep(100);
	} else {
		//--Sensor_PowerDown(power_down);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
	}

	FUN_LEAVE;
	return SENSOR_SUCCESS;
}

LOCAL uint32_t autotst_Identify(uint32_t param)
{
	uint8_t pid = 0;
	uint8_t ver = 0;

	uint32_t ret = SENSOR_FAIL;

	FUN_ENTER;

	Sensor_ReadReg_8bits(SENSOR_PID_ADDR, &pid);
	Sensor_ReadReg_8bits(SENSOR_VER_ADDR, &ver);

	pr_info("[autotst_Identify: pid = 0x%x, ver = 0x%x]\n", pid, ver);

	if ( SENSOR_PID_VALUE == pid && SENSOR_VER_VALUE == ver ) {
		pr_info("That is autotst sensor !");
		ret = SENSOR_SUCCESS;
	}

	FUN_LEAVE;
	return ret;
}

static uint32_t autotst_check_image_format_support(uint32_t param)
{
	uint32_t ret_val = SENSOR_FAIL;

	FUN_ENTER;
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

	FUN_LEAVE;
	return ret_val;
}

static struct sensor_drv_cfg sensor_autotst = {
        .sensor_pos  = 2, // sub type
        .sensor_name = "autotst",
        .driver_info = &g_autotst_yuv_info,
};

static int __init sensor_autotst_init(void)
{
        return dcam_register_sensor_drv(&sensor_autotst);
}

subsys_initcall(sensor_autotst_init);
