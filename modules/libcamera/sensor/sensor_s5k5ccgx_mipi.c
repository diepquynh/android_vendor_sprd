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

/**---------------------------------------------------------------------------*
 ** 						Dependencies									  *
 **---------------------------------------------------------------------------*/
#include "sensor.h"

//#include <mach/ldo.h>

//#include "sensor_s5k5ccgx_regs.h"
#include "sensor_s5k5ccgx_regs_mipi.h"
#include "sensor_drv_u.h"

/**---------------------------------------------------------------------------*
 ** 						Compiler Flag									  *
 **---------------------------------------------------------------------------*/
#ifdef	 __cplusplus
	extern	 "C"
	{
#endif
/**---------------------------------------------------------------------------*
 ** 					Extern Function Declaration 						  *
 **---------------------------------------------------------------------------*/
//extern uint32_t OS_TickDelay(uint32_t ticks);

/**---------------------------------------------------------------------------*
 ** 						Const variables 								  *
 **---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
 ** 						   Macro Define
 **---------------------------------------------------------------------------*/
#define S5K5CCGX_I2C_ADDR_W		0x002D
#define S5K5CCGX_I2C_ADDR_R		0x002D

/*Kyle-TD Tuning*/
//#define CONFIG_LOAD_FILE

#ifdef CONFIG_LOAD_FILE
#define CAM_DEBUG   SENSOR_PRINT
#else
//#define CAM_DEBUG(...)
#define CAM_DEBUG SENSOR_PRINT
#endif



// Address can change by pin  0(AF mode) -> 78,79  1(FF mode)-> 5a,5b
// In Amazing_TD,  Set 1 by IO level

/**---------------------------------------------------------------------------*
 ** 					Local Function Prototypes							  *
 **---------------------------------------------------------------------------*/
LOCAL uint32_t s5k5ccgx_set_ae_enable(uint32_t enable);
LOCAL uint32_t s5k5ccgx_set_hmirror_enable(uint32_t enable);
LOCAL uint32_t s5k5ccgx_set_vmirror_enable(uint32_t enable);
LOCAL uint32_t s5k5ccgx_set_scene_mode(uint32_t scene_mode);
LOCAL uint32_t s5k5ccgx_set_preview_mode(uint32_t preview_mode);
LOCAL uint32_t s5k5ccgx_set_capture_mode(uint32_t capture_mode);
LOCAL uint32_t s5k5ccgx_Power_Ctrl(uint32_t param);
LOCAL uint32_t s5k5ccgx_Identify(uint32_t param);
LOCAL uint32_t s5k5ccgx_BeforeSnapshot(uint32_t param);

LOCAL uint32_t s5k5ccgx_set_brightness(uint32_t level);
LOCAL uint32_t s5k5ccgx_set_contrast(uint32_t level);
LOCAL uint32_t s5k5ccgx_set_image_effect(uint32_t effect_type);
LOCAL uint32_t s5k5ccgx_chang_image_format(uint32_t param);
LOCAL uint32_t s5k5ccgx_check_image_format_support(uint32_t param);
LOCAL uint32_t s5k5ccgx_after_snapshot(uint32_t param);

LOCAL uint32_t s5k5ccgx_set_awb(uint32_t mode);
LOCAL uint32_t s5k5ccgx_GetExifInfo(uint32_t param);
LOCAL uint32_t s5k5ccgx_set_focus(uint32_t effect_type);
LOCAL uint32_t s5k5ccgx_InitExifInfo(void);

LOCAL uint32_t s5k5ccgx_set_quality(uint32_t quality_type);
LOCAL uint32_t s5k5ccgx_set_DTP(uint32_t dtp_mode);
LOCAL uint32_t s5k5ccgx_set_Metering(uint32_t metering_mode);
LOCAL uint32_t s5k5ccgx_set_FPS(uint32_t fps_mode);
LOCAL uint32_t s5k5ccgx_I2C_write(SENSOR_REG_T* sensor_reg_ptr);


LOCAL uint32_t s5k5ccgx_LightCheck(void);
LOCAL uint32_t s5k5ccgx_set_lightcapture(uint32_t level);
LOCAL uint32_t s5k5ccgx_Get_Exif_Exporsure(uint32_t level);
LOCAL uint32_t s5k5ccgx_Get_Exif_ISO(uint32_t level);
LOCAL uint32_t s5k5ccgx_Get_Exif_Flash(uint32_t level);
LOCAL uint32_t s5k5ccgx_GetExifInfo(uint32_t level);
LOCAL uint32_t s5k5ccgx_InitExt(uint32_t param);
LOCAL uint32_t s5k5ccgx_init_by_burst_write(uint32_t param);
LOCAL uint32_t s5k5ccgx_streamon(uint32_t param);
LOCAL uint32_t s5k5ccgx_streamoff(uint32_t param);

/**---------------------------------------------------------------------------*
 ** 						Local Variables 								 *
 **---------------------------------------------------------------------------*/

LOCAL uint32_t s_image_effect = 0;

LOCAL uint32_t work_mode = 0;

/*++ FOR CHECKING THE LIGHT BEFORE CAPTURE :dhee79.lee@samsung.com++*/
typedef enum
{
	HIGH_LIGHT_CAPTURE,
	NORMAL_LIGHT_CAPTURE,
	LOW_LIGHT_CAPTURE
}SensorLightCaptureType;
/*-- FOR CHECKING THE LIGHT BEFORE CAPTURE :dhee79.lee@samsung.com--*/


/*lint -save -e533 */

LOCAL const SENSOR_REG_T s5k5ccgx_640x480_setting[] = {
	//PREVIEW
	{0x002A, 0x0208},
	{0x0F12, 0x0000},	//REG_TC_GP_ActivePrevConfig
	{0x002A, 0x0210},
	{0x0F12, 0x0000},	//REG_TC_GP_ActiveCapConfig

	{0x002A, 0x020C},
	{0x0F12, 0x0001},	//REG_TC_GP_PrevOpenAfterChange
	{0x002A, 0x01F4},
	{0x0F12, 0x0001},	//REG_TC_GP_NewConfigSync
	{0x002A, 0x020A},
	{0x0F12, 0x0001},	//REG_TC_GP_PrevConfigChanged
	{0x002A, 0x0212},
	{0x0F12, 0x0001},	//REG_TC_GP_CapConfigChanged
	{0x002A, 0x01E8},
	{0x0F12, 0x0000},	//REG_TC_GP_EnableCapture
	{0x0F12, 0x0001},	//REG_TC_GP_EnableCaptureChanged
};

LOCAL const SENSOR_REG_T s5k5ccgx_1280X960_setting[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x0330},
	{0x0F12, 0x0500},	//REG_0TC_CCFG_usWidth
	{0x0F12, 0x03C0},	//REG_0TC_CCFG_usHeight

	{0x002A, 0x0210},
	{0x0F12, 0x0000},	//REG_TC_GP_ActiveCapConfig
	{0x002A, 0x01F4},
	{0x0F12, 0x0001},	//REG_TC_GP_NewConfigSync
	{0x002A, 0x0212},
	{0x0F12, 0x0001},	//REG_TC_GP_CapConfigChanged
	{0x002A, 0x01E8},
	{0x0F12, 0x0001},	//REG_TC_GP_EnableCapture
	{0x0F12, 0x0001},	//REG_TC_GP_EnableCaptureChanged

};

LOCAL const SENSOR_REG_T s5k5ccgx_1600X1200_setting[] = {
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},

	{0x002A, 0x0330},
	{0x0F12, 0x0640},	//REG_0TC_CCFG_usWidth
	{0x0F12, 0x04B0},	//REG_0TC_CCFG_usHeight


	{0x002A, 0x0210},
	{0x0F12, 0x0000},	//REG_TC_GP_ActiveCapConfig
	{0x002A, 0x01F4},
	{0x0F12, 0x0001},	//REG_TC_GP_NewConfigSync
	{0x002A, 0x0212},
	{0x0F12, 0x0001},	//REG_TC_GP_CapConfigChanged
	{0x002A, 0x01E8},
	{0x0F12, 0x0001},	//REG_TC_GP_EnableCapture
	{0x0F12, 0x0001},	//REG_TC_GP_EnableCaptureChanged

};

LOCAL const SENSOR_REG_T s5k5ccgx_2048X1536_setting[] = {
	// ==========================================================
	//	Capture Size : 2048 x 1536 0
	// ==========================================================
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},

	{0x002A, 0x0330},
	{0x0F12, 0x0800},	//REG_0TC_CCFG_usWidth
	{0x0F12, 0x0600},	//REG_0TC_CCFG_usHeight


	{0x002A, 0x0210},
	{0x0F12, 0x0000},	//REG_TC_GP_ActiveCapConfig
	{0x002A, 0x01F4},
	{0x0F12, 0x0001},	//REG_TC_GP_NewConfigSync
	{0x002A, 0x0212},
	{0x0F12, 0x0001},	//REG_TC_GP_CapConfigChanged
	{0x002A, 0x01E8},
	{0x0F12, 0x0001},	//REG_TC_GP_EnableCapture
	{0x0F12, 0x0001},	//REG_TC_GP_EnableCaptureChanged

};


LOCAL SENSOR_REG_TAB_INFO_T s_s5k5ccgx_resolution_Tab_YUV[]=
{
	// COMMON INIT
	{&reg_main_init[0], NUMBER_OF_ARRAY(reg_main_init),640, 480, 24, SENSOR_IMAGE_FORMAT_YUV422},

	// YUV422 PREVIEW 1
	{ADDR_AND_LEN_OF_ARRAY(s5k5ccgx_640x480_setting), 640, 480, 24, SENSOR_IMAGE_FORMAT_YUV422},
	{ADDR_AND_LEN_OF_ARRAY(s5k5ccgx_1280X960_setting), 1280, 960, 24, SENSOR_IMAGE_FORMAT_YUV422},
	{ADDR_AND_LEN_OF_ARRAY(s5k5ccgx_1600X1200_setting), 1600, 1200, 24, SENSOR_IMAGE_FORMAT_YUV422},
	{ADDR_AND_LEN_OF_ARRAY(s5k5ccgx_2048X1536_setting), 2048, 1536, 24, SENSOR_IMAGE_FORMAT_YUV422},

	// YUV422 PREVIEW 2
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

LOCAL SENSOR_IOCTL_FUNC_TAB_T s_s5k5ccgx_ioctl_func_tab =
{
    // Internal
    PNULL, /*0*/
    s5k5ccgx_Power_Ctrl, /*1*/
    PNULL,/*2*/
    s5k5ccgx_Identify,/*3*/		// write register
    PNULL,/*4*/			// read  register
    PNULL,/*5*/
    s5k5ccgx_init_by_burst_write,/*6*/
    PNULL,/*7*/

    // External
    PNULL,/*8*///s5k5ccgx_set_ae_enable,
    PNULL,/*9*///s5k5ccgx_set_hmirror_enable,
    PNULL,/*10*///s5k5ccgx_set_vmirror_enable,
    s5k5ccgx_set_brightness,/*11*/
    s5k5ccgx_set_contrast,/*12*/
    PNULL,/*13*///s5k5ccgx_set_sharpness,
    PNULL,/*14*///s5k5ccgx_set_saturation,
    s5k5ccgx_set_scene_mode ,/*15*///s5k5ccgx_set_preview_mode,
    s5k5ccgx_set_image_effect,/*16*/
    s5k5ccgx_BeforeSnapshot,/*17*/
    s5k5ccgx_after_snapshot,/*18*/
    PNULL,/*19*/
    PNULL,/*20*/
    PNULL,/*21*/
    PNULL,/*22*/
    PNULL,/*23*/
    PNULL,/*24*/
    s5k5ccgx_Get_Exif_ISO,/*25*/
    s5k5ccgx_Get_Exif_Exporsure,/*26*/
    s5k5ccgx_Get_Exif_Flash,/*27*/
    s5k5ccgx_set_awb,/*28*/
    s5k5ccgx_set_DTP,/*29*/
    PNULL,/*30*///iso
    PNULL,/*31*///exposure
    PNULL,/*32*/
    PNULL,/*33*/
    PNULL,/*34*/ //wxz:???
    PNULL,/*35*/// set FPS
    PNULL,/*36*///focus,
    PNULL,/*37*///anti flicker
    PNULL,/*38*/// video mode
    PNULL,/*39*///pick j peg
    s5k5ccgx_set_Metering,/*40*/// set mertering
    PNULL, /*41*///get_status
    s5k5ccgx_streamon, /*42*///stream_on
	s5k5ccgx_streamoff, /*43*/ // stream_off
/*    s5k5ccgx_set_FPS,*/
};


/**---------------------------------------------------------------------------*
 ** 						Global Variables								  *
 **---------------------------------------------------------------------------*/
SENSOR_INFO_T g_s5k5ccgx_yuv_info_mipi =
{
	S5K5CCGX_I2C_ADDR_W,				// salve i2c write address
	S5K5CCGX_I2C_ADDR_R, 				// salve i2c read address
	SENSOR_I2C_VAL_16BIT|SENSOR_I2C_REG_16BIT|SENSOR_I2C_FREQ_400,//SENSOR_I2C_VAL_8BIT|SENSOR_I2C_REG_8BIT,			// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
									// bit1: 0: i2c register addr  is 8 bit, 1: i2c register addr  is 16 bit
									// other bit: reseved
	SENSOR_HW_SIGNAL_PCLK_P|\
	SENSOR_HW_SIGNAL_VSYNC_P|\
	SENSOR_HW_SIGNAL_HSYNC_P,		// bit0: 0:negative; 1:positive -> polarily of pixel clock
									// bit2: 0:negative; 1:positive -> polarily of horizontal synchronization signal
									// bit4: 0:negative; 1:positive -> polarily of vertical synchronization signal
									// other bit: reseved

	// preview mode
	SENSOR_ENVIROMENT_NORMAL|\
	SENSOR_ENVIROMENT_NIGHT|\
	SENSOR_ENVIROMENT_SUNNY,

	// image effect
	0,

	// while balance mode
	 SENSOR_WB_MODE_AUTO|\
        SENSOR_WB_MODE_INCANDESCENCE|\
        SENSOR_WB_MODE_U30|\
        SENSOR_WB_MODE_CWF|\
        SENSOR_WB_MODE_FLUORESCENT|\
        SENSOR_WB_MODE_SUN|\
        SENSOR_WB_MODE_CLOUD,

	0x0005,								// bit[0:7]: count of step in brightness, contrast, sharpness, saturation
									// bit[8:31] reseved

	SENSOR_LOW_PULSE_RESET,		// reset pulse level
	20,								// reset pulse width(ms)

	SENSOR_LOW_LEVEL_PWDN,		// 1: high level valid; 0: low level valid

	1,								// count of identify code
	{{0x04, 0x84}},						// supply two code to identify sensor.
									// for Example: index = 0-> Device id, index = 1 -> version id
									// supply two code to identify sensor.
									// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,			// voltage of avdd

	2048,							// max width of source image
	1536,							// max height of source image
	"s5k5ccgx",						// name of sensor

	SENSOR_IMAGE_FORMAT_YUV422,		// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
									// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_YUV422_UYVY,	// pattern of input image form sensor;

	s_s5k5ccgx_resolution_Tab_YUV,	// point to resolution table information structure
	&s_s5k5ccgx_ioctl_func_tab,		// point to ioctl function table

	PNULL,							// information and table about Rawrgb sensor
	PNULL,				// extend information about sensor
	SENSOR_AVDD_1800MV,                     // iovdd
	SENSOR_AVDD_1800MV,                      // dvdd
	4,                     // skip frame num before preview
	3,                     // skip frame num before capture
	0,                     // deci frame num during preview;
    0,                     // deci frame num during video preview;

	0,                     // threshold enable
    0,                     // threshold mode
    0,                     // threshold start postion
    0,                     // threshold end postion
	-1,                     // i2c_dev_handler
	//{SENSOR_INTERFACE_TYPE_CCIR601, 8, 16, 1} // SENSOR_INF_T
	{SENSOR_INTERFACE_TYPE_CSI2, 1, 8, 1}, // SENSOR_INF_T
	PNULL,
	4,						// skip frame num while change setting
};

/**---------------------------------------------------------------------------*
 ** 							Function  Definitions
 **---------------------------------------------------------------------------*/
/******************************************************************************/
// Description:
// Global resource dependence:
// Author:
// Note:
//
/******************************************************************************/
LOCAL uint32_t s5k5ccgx_set_ae_enable(uint32_t enable)
{

	return 0;
}

/******************************************************************************/
// Description:
// Global resource dependence:
// Author:
// Note:
//
/******************************************************************************/
LOCAL uint32_t s5k5ccgx_set_hmirror_enable(uint32_t enable)
{
	return 0;
}

/******************************************************************************/
// Description:
// Global resource dependence:
// Author:
// Note:
//
/******************************************************************************/
LOCAL uint32_t s5k5ccgx_set_vmirror_enable(uint32_t enable)
{
	return 0;
}

/******************************************************************************/
// Description: s5k5ccgx_Power_Ctrl
// Global resource dependence:
// Author:
// Note:
//
/******************************************************************************/
#if 1
LOCAL uint32_t s5k5ccgx_Power_Ctrl(uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_s5k5ccgx_yuv_info_mipi.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_s5k5ccgx_yuv_info_mipi.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_s5k5ccgx_yuv_info_mipi.iovdd_val;
	BOOLEAN power_down = g_s5k5ccgx_yuv_info_mipi.power_down_level;
	BOOLEAN reset_level = g_s5k5ccgx_yuv_info_mipi.reset_pulse_level;
	//uint32_t reset_width=g_ov5640_yuv_info.reset_pulse_width;

	if (SENSOR_TRUE == power_on) {
		Sensor_PowerDown(power_down);
		// Open power
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		SENSOR_Sleep(20);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		SENSOR_Sleep(10);
		Sensor_PowerDown(!power_down);
		// Reset sensor
		Sensor_Reset(reset_level);
		SENSOR_Sleep(12);
	} else {
		Sensor_PowerDown(power_down);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED,
				  SENSOR_AVDD_CLOSED);
	}
	SENSOR_PRINT("SENSOR: s5k5ccgx_Power_Ctrl (1:on, 0:off): %d \n", power_on);
	return SENSOR_SUCCESS;
}
#else
LOCAL uint32_t s5k5ccgx_Power_Ctrl(uint32_t power_on)
{
	int err = 0xff;

	SENSOR_PRINT("s5k5ccgx-> power_ctrl = %d\n",power_on);

	if(power_on)
	{
       	err = gpio_request(72,"ccirrst");
       	gpio_direction_output(72,0);
		gpio_set_value(72,0);
       	udelay(1);

		Sensor_SetVoltage(SENSOR_AVDD_1200MV, SENSOR_AVDD_2800MV, SENSOR_AVDD_1800MV);
	   	SENSOR_Sleep(5);
		Sensor_SetMCLK(SENSOR_MCLK_24M);

		SENSOR_Sleep(20);
	   	gpio_set_value(72,1);

		SENSOR_Sleep(12);	//warm up for I2C

       }
	else
	{
 		gpio_set_value(72,0);
		gpio_free(72);
	   	udelay(60);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
	   	udelay(1);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
	   	udelay(1);
	}
}
#endif
/******************************************************************************/
// Description: s5k5ccgx_Identify
// Global resource dependence:
// Author:
// Note:
//
/******************************************************************************/
LOCAL uint32_t s5k5ccgx_Identify(uint32_t param)
{

        uint32_t  ret = SENSOR_OP_ERR;
		uint16_t  value;


        Sensor_WriteReg(0xFCFC,0xD000);
		CAM_DEBUG("The S5K5CCGX sensor is not Connecting1\n");

        Sensor_WriteReg(0x002C,0x7000);
		CAM_DEBUG("The S5K5CCGX sensor is not Connecting2\n");

        Sensor_WriteReg(0x002E,0x0150);
		CAM_DEBUG("The S5K5CCGX sensor is not Connecting3\n");

		value = Sensor_ReadReg(0x0F12);

        if(value != 0x05CC)
        {
                SENSOR_PRINT_ERR("The S5K5CCGX sensor is not Connected..!! value=%x \n", value);
                return SENSOR_OP_ERR;
        }
        else
        {
                 SENSOR_PRINT_HIGH("The S5K5CCGX sensor is Connected..!!");
                return SENSOR_OP_SUCCESS;
        }

        return ret;
}

/******************************************************************************/
// Description: S5K5CCGX_LightCheck
// Global resource dependence:
// Author: dhee79.lee@samsung.com
// Note:
//
/******************************************************************************/
LOCAL uint32_t s5k5ccgx_LightCheck(void)
{
	SensorLightCaptureType SensorLight = NORMAL_LIGHT_CAPTURE;
	uint32_t lightStatus = 0;
	uint16_t lightStatus_low_word = 0;
	uint16_t lightStatus_high_word = 0;

	CAM_DEBUG("S5K5CCGX_LightCheck..");

	Sensor_WriteReg(0xFCFC,0xD000);
	Sensor_WriteReg(0x002C,0x7000);
	Sensor_WriteReg(0x002E,0x2A3C);
	lightStatus_low_word = Sensor_ReadReg(0x0F12);
	Sensor_WriteReg(0x002E,0x2A3E);
	lightStatus_high_word = Sensor_ReadReg(0x0F12);

	lightStatus = lightStatus_low_word | (lightStatus_high_word <<16);

	if(lightStatus > 0xFFFE)
	{
		CAM_DEBUG("HIGH_LIGHT_CAPTURE..");
		SensorLight = HIGH_LIGHT_CAPTURE;
	}
	else if(lightStatus < 0x20)
	{
		CAM_DEBUG("LOW_LIGHT_CAPTURE..");
		SensorLight = LOW_LIGHT_CAPTURE;
	}
	else
	{
		CAM_DEBUG("NORMAL_LIGHT_CAPTURE..");
		SensorLight = NORMAL_LIGHT_CAPTURE;
	}

	return SensorLight;

}

/******************************************************************************/
// Description: s5k5ccgx_set_lightcapture
// Global resource dependence:
// Author:
// Note:
//
/******************************************************************************/
LOCAL uint32_t s5k5ccgx_set_lightcapture(uint32_t level)
{

#ifdef CONFIG_LOAD_FILE
	CAM_DEBUG("SENSOR Tuning: s5k5ccgx_set_lightcapture ");

	switch(level)
		{
			case 0:
				Sensor_regs_table_write("s5k5ccgx_highlight_snapshot");
				break;
			case 1:
				Sensor_regs_table_write("s5k5ccgx_normal_snapshot");
				break;
			case 2:
				Sensor_regs_table_write("s5k5ccgx_lowlight_snapshot");
				break;
		}
#else
	if(level >=3)
		return SENSOR_OP_PARAM_ERR;

	if(level == HIGH_LIGHT_CAPTURE)
	{
		CAM_DEBUG("HIGH_LIGHT_CAPTURE..");
		s5k5ccgx_I2C_write((SENSOR_REG_T*) s5k5ccgx_highlight_snapshot);
	}
	else if(level == LOW_LIGHT_CAPTURE)
	{
		CAM_DEBUG("LOW_LIGHT_CAPTURE..");
		s5k5ccgx_I2C_write((SENSOR_REG_T*) s5k5ccgx_lowlight_snapshot);
	}
	else
	{
		CAM_DEBUG("NORMAL_LIGHT_CAPTURE..");
		s5k5ccgx_I2C_write((SENSOR_REG_T*) s5k5ccgx_normal_snapshot);
	}

	CAM_DEBUG("s5k5ccgx_set_lightcapture: level = %d", level);

#endif
	return 0;
}
/******************************************************************************/
// Description: s5k5ccgx_set_brightness
// Global resource dependence:
// Author:
// Note:
//
/******************************************************************************/
LOCAL uint32_t s5k5ccgx_set_brightness(uint32_t level)
{

#ifdef CONFIG_LOAD_FILE
	CAM_DEBUG("SENSOR Tuning: s5k5ccgx_set_brightness\n ");

	switch(level)
		{
			case 0:
				Sensor_regs_table_write("s5k5ccgx_brightness_tab_LEVEL_0");
				break;
			case 1:
				Sensor_regs_table_write("s5k5ccgx_brightness_tab_LEVEL_1");
				break;
			case 2:
				Sensor_regs_table_write("s5k5ccgx_brightness_tab_LEVEL_2");
				break;
			case 3:
				Sensor_regs_table_write("s5k5ccgx_brightness_tab_LEVEL_3");
				break;
			case 4:
				Sensor_regs_table_write("s5k5ccgx_brightness_tab_LEVEL_4");
				break;
			case 5:
				Sensor_regs_table_write("s5k5ccgx_brightness_tab_LEVEL_5");
				break;
			case 6:
				Sensor_regs_table_write("s5k5ccgx_brightness_tab_LEVEL_6");
				break;
			case 7:
				Sensor_regs_table_write("s5k5ccgx_brightness_tab_LEVEL_7");
				break;
			case 8:
				Sensor_regs_table_write("s5k5ccgx_brightness_tab_LEVEL_8");
				break;

			default:
				CAM_DEBUG("[AWB]Invalid value is ordered!!!\n");
				break;
		}
#else
	if(level >= 9)
		return SENSOR_OP_PARAM_ERR;

	 s5k5ccgx_I2C_write((SENSOR_REG_T*) s5k5ccgx_brightness_tab[level]);

	CAM_DEBUG("s5k5ccgx_set_brightness: level = %d", level);

#endif
       SENSOR_Sleep(10);
	return 0;
}

/******************************************************************************/
// Description: s5k5ccgx_set_contrast
// Global resource dependence:
// Author:
// Note:
//
/******************************************************************************/
LOCAL uint32_t s5k5ccgx_set_contrast(uint32_t level)
{
#ifdef CONFIG_LOAD_FILE
	CAM_DEBUG("SENSOR Tuning: S5K5CCGX_set_brightness ");

	switch(level)
		{
			case 0:
				Sensor_regs_table_write("s5k5ccgx_contrast_tab_LEVEL_0");
				break;
			case 1:
				Sensor_regs_table_write("s5k5ccgx_contrast_tab_LEVEL_1");
				break;
			case 2:
				Sensor_regs_table_write("s5k5ccgx_contrast_tab_LEVEL_2");
				break;
			case 3:
				Sensor_regs_table_write("s5k5ccgx_contrast_tab_LEVEL_3");
				break;
			case 4:
				Sensor_regs_table_write("s5k5ccgx_contrast_tab_LEVEL_4");
				break;
			default:
				CAM_DEBUG("[AWB]Invalid value is ordered!!!\n");
				break;
		}
#else
	if(level >= 5)
		return SENSOR_OP_PARAM_ERR;

 s5k5ccgx_I2C_write((SENSOR_REG_T*) s5k5ccgx_contrast_tab[level]);
#endif
       SENSOR_Sleep(5);
	CAM_DEBUG("s5k5ccgx_set_contrast: level = %d", level);

	return 0;
}

/******************************************************************************/
// Description: s5k5ccgx_set_image_effect
// Global resource dependence:
// Author:
// Note:
//
/******************************************************************************/
LOCAL uint32_t s5k5ccgx_set_image_effect(uint32_t level)
{
#ifdef CONFIG_LOAD_FILE
	CAM_DEBUG("SENSOR Tuning: s5k5ccgx_set_image_effect ");

	switch(level)
		{
			case 0:
				Sensor_regs_table_write("s5k5ccgx_image_effect_tab_LEVEL_0");
				break;
			case 1:
				Sensor_regs_table_write("s5k5ccgx_image_effect_tab_LEVEL_1");
				break;
			case 2:
				Sensor_regs_table_write("s5k5ccgx_image_effect_tab_LEVEL_2");
				break;
			case 3:
				Sensor_regs_table_write("s5k5ccgx_image_effect_tab_LEVEL_3");
				break;
			default:
				SENSOR_TRACE("[Effect]Invalid value is ordered!!!\n");
				break;
		}
#else
	if(level >=4)
		return SENSOR_OP_PARAM_ERR;

	 s5k5ccgx_I2C_write((SENSOR_REG_T*) s5k5ccgx_image_effect_tab[level]);
#endif
       SENSOR_Sleep(5);
	CAM_DEBUG("s5k5ccgx_set_image_effect: level = %d", level);
	return 0;
}


/******************************************************************************/
// Description: set_s5k5ccgx_awb
// Global resource dependence:
// Author:
// Note:
//
/******************************************************************************/
LOCAL uint32_t s5k5ccgx_set_awb(uint32_t mode)
{
#ifdef CONFIG_LOAD_FILE
	CAM_DEBUG("SENSOR Tuning: set_s5k5ccgx_awb ");

	switch(mode)
		{
			case 0:
				Sensor_regs_table_write("s5k5ccgx_awb_tab_AUTO");
				break;
			case 1:
				Sensor_regs_table_write("s5k5ccgx_awb_tab_INCANDESCENT");
				break;
			case 4:
				Sensor_regs_table_write("s5k5ccgx_awb_tab_FLUORESCENT");
				break;
			case 5:
				Sensor_regs_table_write("s5k5ccgx_awb_tab_DAYLIGHT");
				break;
			case 6:
				Sensor_regs_table_write("s5k5ccgx_awb_tab_CLOUDY");
				break;

			default:
				CAM_DEBUG("[AWB]Invalid value is ordered!!!\n");
				break;
		}
#else

	if(mode >= DCAMERA_WB_MODE_MAX)
		return SENSOR_OP_PARAM_ERR;

	 s5k5ccgx_I2C_write((SENSOR_REG_T*) s5k5ccgx_awb_tab[mode]);
#endif
	SENSOR_Sleep(10);
	CAM_DEBUG("s5k5ccgx_set_awb_mode: mode = %d\n", mode);
	return 0;

}

/******************************************************************************/
// Description: s5k5ccgx_set_preview_mode
// Global resource dependence:
// Author:
// Note:
//
/******************************************************************************/
LOCAL uint32_t s5k5ccgx_set_scene_mode(uint32_t scene_mode)
{
#ifdef CONFIG_LOAD_FILE
		CAM_DEBUG("SENSOR Tuning: s5k5ccgx_set_scene_mode ");

		Sensor_regs_table_write("s5k5ccgx_scene_tab_off");
		switch(scene_mode)
			{
				case 0:
					Sensor_regs_table_write("s5k5ccgx_scene_tab_off");
					break;
				case 1:
					Sensor_regs_table_write("s5k5ccgx_scene_tab_portrait");
					break;
				case 2:
					Sensor_regs_table_write("s5k5ccgx_scene_tab_landscape");
					break;
				case 3:
					Sensor_regs_table_write("s5k5ccgx_scene_tab_party");
					break;
				case 4:
					Sensor_regs_table_write("s5k5ccgx_scene_tab_sunset");
					break;
				case 5:
					Sensor_regs_table_write("s5k5ccgx_scene_tab_dawn");
					break;
				case 6:
					Sensor_regs_table_write("s5k5ccgx_scene_tab_fall");
					break;
				case 7:
					Sensor_regs_table_write("s5k5ccgx_scene_tab_night");
					break;
				case 8:
					Sensor_regs_table_write("s5k5ccgx_scene_tab_fire");
					break;
				case 9:
					Sensor_regs_table_write("s5k5ccgx_scene_tab_candle");
					break;
				case 10:
					Sensor_regs_table_write("s5k5ccgx_scene_tab_sports");
					break;
				case 11:
					Sensor_regs_table_write("s5k5ccgx_scene_tab_beach");
					break;
				case 12:
					Sensor_regs_table_write("s5k5ccgx_scene_tab_backlight");
					break;

				default:
					CAM_DEBUG("[SCENE]Invalid value is ordered!!!\n");
					break;
			}
#else
	if(scene_mode >= 13)
		return SENSOR_OP_PARAM_ERR;

	 s5k5ccgx_I2C_write((SENSOR_REG_T*) s5k5ccgx_scene_mode_tab[0]); // Scene Off before setting Scene mode
	 s5k5ccgx_I2C_write((SENSOR_REG_T*) s5k5ccgx_scene_mode_tab[scene_mode]);
#endif
	CAM_DEBUG("s5k5ccgx_set_scene_mode : mode = %d", scene_mode);
	return 0;
}

/******************************************************************************/
// Description: s5k5ccgx_set_quality
// Global resource dependence:
// Author:
// Note:
//
//******************************************************************************/
LOCAL uint32_t s5k5ccgx_set_quality(uint32_t quality_type)
{
#ifdef CONFIG_LOAD_FILE
	CAM_DEBUG("SENSOR Tuning: s5k5ccgx_set_quality ");

	switch(quality_type)
		{
			case 0:
				Sensor_regs_table_write("s5k5ccgx_quality_tab_superfine");
				break;
			case 1:
				Sensor_regs_table_write("s5k5ccgx_quality_tab_fine");
				break;
			case 2:
				Sensor_regs_table_write("s5k5ccgx_quality_tab_normal");
				break;
			default:
				CAM_DEBUG("[Quality]Invalid value is ordered!!!\n");
				break;
		}
#else

	if(quality_type >= 3)
		return SENSOR_OP_PARAM_ERR;

	 s5k5ccgx_I2C_write((SENSOR_REG_T*) s5k5ccgx_quality_tab[quality_type]);
#endif
       SENSOR_Sleep(5);
	CAM_DEBUG("s5k5ccgx_set_quality : level = %d", quality_type);
	return 0;
}


/******************************************************************************/
// Description: s5k5ccgx_set_Metering
// Global resource dependence:
// Author:
// Note:
//
//******************************************************************************/
LOCAL uint32_t s5k5ccgx_set_Metering(uint32_t metering_mode)
{
#ifdef CONFIG_LOAD_FILE
	CAM_DEBUG("SENSOR Tuning: s5k5ccgx_set_Metering_mode ");

	switch(metering_mode)
		{
			case 0:
				Sensor_regs_table_write("s5k5ccgx_metering_mode_normal");
				break;
			case 1:
				Sensor_regs_table_write("s5k5ccgx_metering_mode_spot");
				break;
			case 2:
				Sensor_regs_table_write("s5k5ccgx_metering_mode_centerweighted");
				break;
			default:
				CAM_DEBUG("[Metering]Invalid value is ordered!!!\n");
				break;
		}
#else
	if(metering_mode >= 3)
		return SENSOR_OP_PARAM_ERR;

	 s5k5ccgx_I2C_write((SENSOR_REG_T*) s5k5ccgx_metering_mode_tab[metering_mode]);

#endif
       SENSOR_Sleep(5);
	CAM_DEBUG("s5k5ccgx_set_Metering_mode : level = %d", metering_mode);
	return 0;
}


/******************************************************************************/
// Description: s5k5ccgx_set_DTP
// Global resource dependence:
// Author:
// Note:
//
//******************************************************************************/
LOCAL uint32_t s5k5ccgx_set_DTP(uint32_t dtp_mode)
{
#ifdef CONFIG_LOAD_FILE
	CAM_DEBUG("SENSOR Tuning: s5k5ccgx_set_DTP_mode ");

	switch(dtp_mode)
		{
			case 0:
				Sensor_regs_table_write("s5k5ccgx_dtp_mode_off");
				break;
			case 1:
				Sensor_regs_table_write("s5k5ccgx_dtp_mode_on");
				break;
			default:
				CAM_DEBUG("[DTP]Invalid value is ordered!!!\n");
				break;
		}
#else
	if(dtp_mode >= 2)
		return SENSOR_OP_PARAM_ERR;

	 s5k5ccgx_I2C_write((SENSOR_REG_T*) s5k5ccgx_dtp_mode_tab[dtp_mode]);
#endif

       SENSOR_Sleep(5);
	CAM_DEBUG("s5k5ccgx_set_DTP_mode : level = %d", dtp_mode);
	return 0;
}


/******************************************************************************/
// Description: s5k5ccgx_set_FPS
// Global resource dependence:
// Author:
// Note:
//
//******************************************************************************/
LOCAL uint32_t s5k5ccgx_set_FPS(uint32_t fps_mode)
{
#ifdef CONFIG_LOAD_FILE
	CAM_DEBUG("SENSOR Tuning: s5k5ccgx_set_FPS ");

	switch(fps_mode)
		{
			case 0:
				Sensor_regs_table_write("s5k5ccgx_5_FPS");
				break;
			case 1:
				Sensor_regs_table_write("s5k5ccgx_7_FPS");
				break;
			case 2:
				Sensor_regs_table_write("s5k5ccgx_10_FPS");
				break;
			case 3:
				Sensor_regs_table_write("s5k5ccgx_12_FPS");
				break;
			case 4:
				Sensor_regs_table_write("s5k5ccgx_15_FPS");
				break;
			case 5:
				Sensor_regs_table_write("s5k5ccgx_25_FPS");
				break;
			case 6:
				Sensor_regs_table_write("s5k5ccgx_30_FPS");
				break;
			case 7:
				Sensor_regs_table_write("s5k5ccgx_Auto_FPS");
				break;
			case 8:
				Sensor_regs_table_write("s5k5ccgx_Auto15_FPS");
				break;
			case 9:
				Sensor_regs_table_write("s5k5ccgx_Auto30_FPS");
				break;
			default:
				CAM_DEBUG("[FPS]Invalid value is ordered!!!\n");
				break;
		}
#else
	if(fps_mode >= 10)
		return SENSOR_OP_PARAM_ERR;

	s5k5ccgx_I2C_write((SENSOR_REG_T*) s5k5ccgx_fps_mode_tab[fps_mode]);
#endif
       SENSOR_Sleep(5);
	CAM_DEBUG("s5k5ccgx_set_FPS : level = %d", fps_mode);
	return 0;
}


/******************************************************************************/
// Description:S5K5CCGX_BeforeSnapshot
// Global resource dependence:
// Author:
// Note:
/******************************************************************************/
LOCAL uint32_t s5k5ccgx_BeforeSnapshot(uint32_t param)
{
	uint32_t sensorlight;
	uint32_t cap_mode = (param>>CAP_MODE_BITS);

	param = param&0xffff;
	SENSOR_PRINT("%d,%d.",cap_mode,param);

	CAM_DEBUG("s5k5ccgx_BeforeSnapsho: mode = %d", param);

	sensorlight = s5k5ccgx_LightCheck();
	s5k5ccgx_set_lightcapture(sensorlight);
#ifdef CONFIG_LOAD_FILE
	CAM_DEBUG("SENSOR Tuning: s5k5ccgx_BeforeSnapshot \n");

	switch(param)
		{
			case 0:
				Sensor_regs_table_write("s5k5ccgx_capture_mode_2048x1536");
				break;
			case 1:
				Sensor_regs_table_write("s5k5ccgx_capture_mode_2048x1232");
				break;
			case 2:
				Sensor_regs_table_write("s5k5ccgx_capture_mode_1600x1200");
				break;
			case 4:
				Sensor_regs_table_write("s5k5ccgx_capture_mode_1280x960");
				break;
			case 8:
				Sensor_regs_table_write("s5k5ccgx_capture_mode_800x480");
				break;
			case 10:
				Sensor_regs_table_write("s5k5ccgx_capture_mode_640x480");
				break;
			default:
				CAM_DEBUG("[Capture_mode]Invalid value is ordered!!!\n");
				break;
		}
#else
	//s5k5ccgx_I2C_write((SENSOR_REG_T*) s5k5ccgx_capture_mode_tab[param]);
	Sensor_SetMode(param);

#endif
	return 0;
}

/******************************************************************************/
// Description:s5k5ccgx_after_snapshot
// Global resource dependence:
// Author:
// Note:
/******************************************************************************/
LOCAL uint32_t s5k5ccgx_after_snapshot(uint32_t param)
{

    uint32_t  preview_mode = (param >= SENSOR_MODE_PREVIEW_TWO) ? \
                            SENSOR_MODE_PREVIEW_TWO:SENSOR_MODE_PREVIEW_ONE;

#ifdef CONFIG_LOAD_FILE
	CAM_DEBUG("SENSOR Tuning: s5k5ccgx_after_snapshot \n");

	Sensor_regs_table_write("s5k5ccgx_update_preview_setting");

	switch(param)
	{
		case 0:
			Sensor_regs_table_write("s5k5ccgx_preview_mode_720x480");
			break;
		case 1:
			Sensor_regs_table_write("s5k5ccgx_preview_mode_640x480");
			break;
		case 2:
			Sensor_regs_table_write("s5k5ccgx_preview_mode_800x480");
			break;
		case 3:
			Sensor_regs_table_write("s5k5ccgx_preview_mode_424x318");
			break;
		case 4:
			Sensor_regs_table_write("s5k5ccgx_preview_mode_352x288");
			break;
		case 5:
			Sensor_regs_table_write("s5k5ccgx_preview_mode_320x240");
			break;
		case 6:
			Sensor_regs_table_write("s5k5ccgx_preview_mode_176x144");
			break;
		case 7:
			Sensor_regs_table_write("s5k5ccgx_preview_mode_160x120");
			break;
		case 8:
			Sensor_regs_table_write("s5k5ccgx_preview_mode_176x144");
			break;
		default:
			CAM_DEBUG("[Capture_mode]Invalid value is ordered!!!\n");
			break;
	}
#else
	s5k5ccgx_I2C_write((SENSOR_REG_T*) s5k5ccgx_update_preview_setting);
	s5k5ccgx_I2C_write((SENSOR_REG_T*) s5k5ccgx_preview_mode_tab[param]);
#endif
	CAM_DEBUG("s5k5ccgx_AfterSnapsho: mode = %d\n", param);
	return 0;
}

/******************************************************************************/
// Description:s5k5ccgx_set_preview_mode
// Global resource dependence:
// Author:
// Note:
/******************************************************************************/

LOCAL uint32_t s5k5ccgx_set_preview_mode(uint32_t preview_mode)
{

#ifdef CONFIG_LOAD_FILE
			CAM_DEBUG("SENSOR Tuning: s5k5ccgx_set_preview_mode ");

			switch(preview_mode)
				{
					case 0:
						Sensor_regs_table_write("s5k5ccgx_preview_tab_720_480");
						break;
					case 1:
						Sensor_regs_table_write("s5k5ccgx_preview_tab_640_480");
						break;
					case 2:
						Sensor_regs_table_write("s5k5ccgx_preview_tab_480_320");
						break;
					case 3:
						Sensor_regs_table_write("s5k5ccgx_preview_tab_424_318");
						break;
					case 4:
						Sensor_regs_table_write("s5k5ccgx_preview_tab_352_288");
						break;
					case 5:
						Sensor_regs_table_write("s5k5ccgx_preview_tab_320_240");
						break;
					case 6:
						Sensor_regs_table_write("s5k5ccgx_preview_tab_176_144");
						break;
					case 7:
						Sensor_regs_table_write("s5k5ccgx_preview_tab_160_120");
						break;
					case 8:
						Sensor_regs_table_write("s5k5ccgx_preview_tab_144_176");
						break;

					default:
						SENSOR_TRACE("[PREVIEW]Invalid value is ordered!!!\n");
						break;
				}
#else
		if(preview_mode >= 9)
			return SENSOR_OP_PARAM_ERR;

		s5k5ccgx_I2C_write((SENSOR_REG_T*) s5k5ccgx_preview_mode_tab[preview_mode]);
#endif
		CAM_DEBUG("s5k5ccgx_set_preview_mode : mode = %d", preview_mode);
		return 0;

}

/******************************************************************************/
// Description:s5k5ccgx_set_capture_mode
// Global resource dependence:
// Author:
// Note:
/******************************************************************************/

LOCAL uint32_t s5k5ccgx_set_capture_mode(uint32_t capture_mode)
{

#ifdef CONFIG_LOAD_FILE
			CAM_DEBUG("SENSOR Tuning: s5k5ccgx_set_capture_mode ");

			switch(capture_mode)
				{
					case 0:
						Sensor_regs_table_write("s5k5ccgx_capture_tab_2048_1536");
						break;
					case 1:
						Sensor_regs_table_write("s5k5ccgx_capture_tab_2048_1360");
						break;
					case 2:
						Sensor_regs_table_write("s5k5ccgx_capture_tab_1600_1200");
						break;
					case 3:
						Sensor_regs_table_write("s5k5ccgx_capture_tab_1600_1072");
						break;
					case 4:
						Sensor_regs_table_write("s5k5ccgx_capture_tab_1280_960");
						break;
					case 5:
						Sensor_regs_table_write("s5k5ccgx_capture_tab_1280_848");
						break;
					case 6:
						Sensor_regs_table_write("s5k5ccgx_capture_tab_1024_768");
						break;
					case 7:
						Sensor_regs_table_write("s5k5ccgx_capture_tab_800_600");
						break;
					case 8:
						Sensor_regs_table_write("s5k5ccgx_capture_tab_720_480");
						break;
					case 9:
						Sensor_regs_table_write("s5k5ccgx_capture_tab_640_480");
						break;
					case 10:
						Sensor_regs_table_write("s5k5ccgx_capture_tab_480_320");
						break;
					case 11:
						Sensor_regs_table_write("s5k5ccgx_capture_tab_352_288");
						break;
					case 12:
						Sensor_regs_table_write("s5k5ccgx_capture_tab_320_240");
						break;
					case 13:
						Sensor_regs_table_write("s5k5ccgx_capture_tab_176_144");
						break;
					case 14:
						Sensor_regs_table_write("s5k5ccgx_capture_tab_160_120");
						break;

					default:
						SENSOR_TRACE("[CAPTURE]Invalid value is ordered!!!\n");
						break;
				}
#else
		if(capture_mode >= 15)
			return SENSOR_OP_PARAM_ERR;

		s5k5ccgx_I2C_write((SENSOR_REG_T*) s5k5ccgx_capture_mode_tab[capture_mode]);

#endif
		CAM_DEBUG("s5k5ccgx_set_capture_mode : mode = %d", capture_mode);
		return 0;

}

/******************************************************************************/
// Description:s5k5ccgx_GetExifInfo
// Global resource dependence:
// Author:
// Note:
/******************************************************************************/
LOCAL uint32_t s5k5ccgx_Get_Exif_Exporsure(uint32_t level)
{
	uint32_t shutter_speed =0;
	uint32_t exposure_time = 0;
	uint16_t shutter_speed_lsb = 0;
	uint16_t shutter_speed_msb= 0;
	uint32_t err;

	CAM_DEBUG( "Exposure_Time() \r\n");

	err = Sensor_WriteReg(0xFCFC, 0xD000);
	err = Sensor_WriteReg(0x002C, 0x7000);

	err = Sensor_WriteReg(0x002E, 0x2A14);
	shutter_speed_lsb = Sensor_ReadReg(0x0F12);
	err = Sensor_WriteReg(0x002E, 0x2A16);
	shutter_speed_msb = Sensor_ReadReg(0x0F12);

	shutter_speed = shutter_speed_lsb  | (shutter_speed_msb << 16);

	CAM_DEBUG( "shutter_speed : %d \n",shutter_speed);
	if(shutter_speed ==0 ) shutter_speed =1;

	exposure_time = 400000/shutter_speed;

	CAM_DEBUG( "exposure_time : %d \n",exposure_time);


	return exposure_time;

}


LOCAL uint32_t s5k5ccgx_Get_Exif_ISO(uint32_t level)
{
	uint16_t iso_gain= 0;
	uint16_t iso_value= 0;
	uint32_t err;

	CAM_DEBUG( "s5k5ccgx_Get_ISO() \r\n");

	err = Sensor_WriteReg(0xFCFC, 0xD000);
	err = Sensor_WriteReg(0x002C, 0x7000);

	err = Sensor_WriteReg(0x002E, 0x2A18);
	iso_gain = Sensor_ReadReg (0x0F12);

	CAM_DEBUG( "iso_gain() %d \n",iso_gain);

	iso_gain =(iso_gain/256)*100;

	CAM_DEBUG( "iso_gain() %d \n",iso_gain);

	if(iso_gain >=350)
		iso_value =400;
	else if(iso_gain >=250)
		iso_value =200;
	else if(iso_gain >=150)
		iso_value =100;
	else if(iso_gain >=100)
		iso_value =50;

	SENSOR_TRACE( "iso_value : %d \n",iso_value);

	return iso_value;
}

LOCAL uint32_t s5k5ccgx_Get_Exif_Flash(uint32_t level)
{
	CAM_DEBUG( "s5k5ccgx_Get_Exif_Flash() \r\n");

	return 0;
}

LOCAL uint32_t s5k5ccgx_I2C_write(SENSOR_REG_T* sensor_reg_ptr)
{
	uint16_t 	i;

       for(i = 0; (0xFFFF != sensor_reg_ptr[i].reg_addr) || (0xFFFF != sensor_reg_ptr[i].reg_value) ; i++)
       {
            Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
       }

       if((sensor_reg_ptr[i].reg_addr == 0xFFFF) && (sensor_reg_ptr[i].reg_value != 0xFFFF))
       {
	     SENSOR_Sleep(sensor_reg_ptr[i].reg_value);
       }

	 CAM_DEBUG("s5k5ccgx_I2C_write : count = %d\n", i);

	 return 0;
}


#define BURST_MODE_BUFFER_MAX_SIZE 2700
uint8_t s5k5ccgx_buf_for_burstmode[BURST_MODE_BUFFER_MAX_SIZE];
LOCAL uint32_t s5k5ccgx_init_by_burst_write(uint32_t param)
{
    uint32_t i = 0;
    int idx = 0;
    int err = -1;
    int retry = 0;
    unsigned short subaddr=0,next_subaddr=0;
    unsigned short value=0;

    //struct i2c_client *i2c_client = Sensor_GetI2CClien();
    //struct timeval time1, time2;

    //struct i2c_msg msg = {i2c_client->addr, 0, 0, s5k5ccgx_buf_for_burstmode };
    uint32_t init_table_size = s_s5k5ccgx_resolution_Tab_YUV[SENSOR_MODE_COMMON_INIT].reg_count;
    SENSOR_REG_T_PTR  reg_table = s_s5k5ccgx_resolution_Tab_YUV[SENSOR_MODE_COMMON_INIT].sensor_reg_tab_ptr;


    //do_gettimeofday(&time1);

I2C_RETRY:

    idx = 0;


    for (i = 0; i < init_table_size; i++)
    {
        if(idx > (BURST_MODE_BUFFER_MAX_SIZE-10))
        {
            SENSOR_PRINT_ERR("s5k5ccgx_sensor_burst_write_buffer_overflow!!!\n");
            return err;
        }

        subaddr = reg_table[i].reg_addr;

        if(subaddr == 0x0F12)
        {
		next_subaddr= reg_table[i+1].reg_addr;
        }

		value = reg_table[i].reg_value;

        switch(subaddr)
        {
            case 0x0F12 :
                if(idx ==0)
                {
                    s5k5ccgx_buf_for_burstmode[idx++] = 0x0F;
                    s5k5ccgx_buf_for_burstmode[idx++] = 0x12;
                }
                    s5k5ccgx_buf_for_burstmode[idx++] = value>> 8;
                    s5k5ccgx_buf_for_burstmode[idx++] = value & 0xFF;

                if(next_subaddr != 0x0F12)
                {
#if 1
					err = Sensor_WriteData(s5k5ccgx_buf_for_burstmode, idx);
#else
					msg.len = idx;
                    err = i2c_transfer(i2c_client->adapter, &msg, 1) == 1 ? 0 : -EIO;
#endif
                    //SENSOR_PRINT("s5k5ccgx_sensor_burst_write, idx = %d\n",idx);
                    idx=0;
                }
            break;

            case 0xFFFF :
			if(value == 0xFFFF)
			{
           			 break;
			}
			else if(value >= 10)
			{
				SENSOR_Sleep(value);
			}
			else
			{
				SENSOR_Sleep(value);
			}
                SENSOR_PRINT("s5k5ccgx_sensor_burst_write, delay %dms\n",value);
            break;

		default:
                idx=0;
                err = Sensor_WriteReg(subaddr,value);
            break;

        }
	}

	if (err < 0)
        {
            SENSOR_PRINT_ERR("[S5K5CCGX]%s: register set failed. try again.\n",__func__);
		retry++;
            if((retry++)<3) goto I2C_RETRY;
            return err;
        }

    //do_gettimeofday(&time2);
    //printk("SENSOR: _s5k5ccgx_InitExt time=%d.\n",((time2.tv_sec-time1.tv_sec)*1000+(time2.tv_usec-time1.tv_usec)/1000));
    SENSOR_PRINT("SENSOR: _s5k5ccgx_InitExt, success \n");

    return 0;
}

LOCAL uint32_t s5k5ccgx_streamon(uint32_t param)
{
	SENSOR_PRINT("SENSOR: s5k5ccgx_streamon");

	//Sensor_PowerDown(1);

	//Sensor_WriteReg(0x3008, 0x02);

	return 0;
}

LOCAL uint32_t s5k5ccgx_streamoff(uint32_t param)
{
	SENSOR_PRINT("SENSOR: s5k5ccgx_streamoff");

	//Sensor_WriteReg(0x3008, 0x42);
	//Sensor_PowerDown(0);

	return 0;
}


#if 0
LOCAL uint32_t s5k5ccgx_InitExt(uint32_t param)
{
	uint32_t              rtn = SENSOR_SUCCESS;
	int ret = 0;
	uint32_t              i = 0;
	uint32_t              written_num = 0;
	uint16_t              wr_reg = 0;
	uint16_t              wr_val = 0;
	uint32_t              wr_num_once = 0;
	uint32_t              wr_num_once_ret = 0;
	uint32_t		   alloc_size = 0;
	//uint32_t              init_table_size = NUMBER_OF_ARRAY(reg_main_init);
	//SENSOR_REG_T_PTR    p_reg_table = reg_main_init;
	uint32_t              init_table_size = s_s5k5ccgx_resolution_Tab_YUV[SENSOR_MODE_COMMON_INIT].reg_count;
	SENSOR_REG_T_PTR    p_reg_table = s_s5k5ccgx_resolution_Tab_YUV[SENSOR_MODE_COMMON_INIT].sensor_reg_tab_ptr;
	uint8_t               *p_reg_val_tmp = 0;
	//struct i2c_msg msg_w;
	//struct i2c_client *i2c_client = Sensor_GetI2CClien();
	//struct timeval time1, time2;

	CAM_DEBUG("SENSOR: _s5k5ccgx_InitExt, init_table_size = %d \n", init_table_size);
	if(0 == i2c_client)
	{
		CAM_DEBUG("SENSOR: _s5k5ccgx_InitExt:error,i2c_client is NULL!.\n");
	}

	do_gettimeofday(&time1);

	alloc_size = init_table_size*sizeof(uint16_t) + 16;
	p_reg_val_tmp = (uint8_t*)kzalloc(alloc_size, GFP_KERNEL);

	if(0 == p_reg_val_tmp)
	{
		CAM_DEBUG("_s5k5ccgx_InitExt: kzalloc failed, size = %d \n", alloc_size);
		return 1;
	}

	while(written_num < init_table_size)
	{
		wr_reg = p_reg_table[written_num].reg_addr;
		wr_val = p_reg_table[written_num].reg_value;
		if(SENSOR_WRITE_DELAY == wr_reg)
		{
			if(wr_val >= 10)
			{
				SENSOR_Sleep(wr_val);
			}
			else
			{
				SENSOR_Sleep(wr_val);
			}
		}
		else
		{
			p_reg_val_tmp[0] = (uint8)((wr_reg >> 8) & 0xFF);
			p_reg_val_tmp[1] = (uint8)(wr_reg & 0xFF);
			p_reg_val_tmp[2] = (uint8)((wr_val >> 8) & 0xFF);
			p_reg_val_tmp[3] = (uint8)(wr_val & 0xFF);
			wr_num_once = 2;
			for(i = written_num + 1; i< init_table_size; i++)
			{
				if(p_reg_table[i].reg_addr != wr_reg)
				{
					break;
				}
				else
				{
					wr_val = p_reg_table[i].reg_value;
					p_reg_val_tmp[2*wr_num_once] = (uint8)((wr_val >> 8) & 0xFF);
					p_reg_val_tmp[2*wr_num_once+1] = (uint8)(wr_val & 0xFF);
					wr_num_once ++;
#if 0
					if(wr_num_once >= I2C_WRITE_BURST_LENGTH)
					{
						break;
					}
#endif
				}
			}

			msg_w.addr = i2c_client->addr;
			msg_w.flags = 0;
			msg_w.buf = p_reg_val_tmp;
			msg_w.len = (uint32)(wr_num_once*2);
			ret = i2c_transfer(i2c_client->adapter, &msg_w, 1);
			if(ret!=1)
			{
				CAM_DEBUG("SENSOR: _s5k5ccgx_InitExt, i2c write once error \n");
				rtn = 1;
				break;
			}
			else
			{
#if 0
				SENSOR_PRINT("SENSOR: _s5k5ccgx_InitExt, i2c write once from %d {0x%x 0x%x}, total %d registers {0x%x 0x%x}",
				      written_num,cmd[0],cmd[1],wr_num_once,p_reg_val_tmp[0],p_reg_val_tmp[1]);
				if(wr_num_once > 1)
				{
					SENSOR_PRINT("SENSOR: _s5k5ccgx_InitExt, val {0x%x 0x%x} {0x%x 0x%x} {0x%x 0x%x} {0x%x 0x%x} {0x%x 0x%x} {0x%x 0x%x}.\n",
				          p_reg_val_tmp[0],p_reg_val_tmp[1],p_reg_val_tmp[2],p_reg_val_tmp[3],
				          p_reg_val_tmp[4],p_reg_val_tmp[5],p_reg_val_tmp[6],p_reg_val_tmp[7],
				          p_reg_val_tmp[8],p_reg_val_tmp[9],p_reg_val_tmp[10],p_reg_val_tmp[11]);

				}
#endif
			}
		}
		written_num += wr_num_once-1;
	}

    kfree(p_reg_val_tmp);

    do_gettimeofday(&time2);
    CAM_DEBUG("SENSOR: _s5k5ccgx_InitExt time=%d.\n",((time2.tv_sec-time1.tv_sec)*1000+(time2.tv_usec-time1.tv_usec)/1000));

    CAM_DEBUG("SENSOR: _s5k5ccgx_InitExt, success \n");

    return rtn;
}
#endif

#if 0
struct class *camera_class;

struct sensor_drv_cfg sensor_s5k5ccgx = {
	.sensor_pos = CONFIG_DCAM_SENSOR_POS_S5K5CCGX,
	.sensor_name = "s5k5ccgx",
	.driver_info = &g_s5k5ccgx_yuv_info,
};


static ssize_t Rear_Cam_Sensor_ID(struct device *dev, struct device_attribute *attr, char *buf)
{
	CAM_DEBUG("Rear_Cam_Sensor_ID\n");
	return  sprintf(buf, "S5K5CCGX");
}

static DEVICE_ATTR(rear_camfw, S_IRUGO | S_IWUSR , Rear_Cam_Sensor_ID, NULL);
static int __init sensor_s5k5ccgx_init(void)
{

	struct device *dev_t;

	camera_class = class_create(THIS_MODULE, "camera");

	if (IS_ERR(camera_class))
	{
	 CAM_DEBUG("Failed to create camera_class!\n");
	 return PTR_ERR( camera_class );
	}


	dev_t = device_create(camera_class, NULL, 0, "%s", "rear");

	if (device_create_file(dev_t, &dev_attr_rear_camfw) < 0)
	 CAM_DEBUG("Failed to create device file(%s)!\n", dev_attr_rear_camfw.attr.name);


	return dcam_register_sensor_drv(&sensor_s5k5ccgx);
}

subsys_initcall(sensor_s5k5ccgx_init);
#endif


