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
/**---------------------------------------------------------------------------*
 ** 						   Macro Define
 **---------------------------------------------------------------------------*/
#define HI702_I2C_ADDR_W	(0x60>>1)
#define HI702_I2C_ADDR_R	(0x61>>1)

#define SENSOR_GAIN_SCALE		16 
/**---------------------------------------------------------------------------*
 ** 					Local Function Prototypes							  *
 **---------------------------------------------------------------------------*/
LOCAL uint32_t set_hi702_ae_enable(uint32_t enable);
LOCAL uint32_t set_hmirror_enable(uint32_t enable);
LOCAL uint32_t set_vmirror_enable(uint32_t enable);
LOCAL uint32_t set_preview_mode(uint32_t preview_mode);
LOCAL uint32_t _hi702_PowerOn(uint32_t power_on);
LOCAL uint32_t HI702_Identify(uint32_t param);
/*
LOCAL uint32_t HI702_BeforeSnapshot(uint32_t param);
LOCAL uint32_t HI702_After_Snapshot(uint32_t param);
*/
LOCAL uint32_t set_brightness(uint32_t level);
LOCAL uint32_t set_contrast(uint32_t level);
LOCAL uint32_t set_sharpness(uint32_t level);
LOCAL uint32_t set_saturation(uint32_t level);
LOCAL uint32_t set_image_effect(uint32_t effect_type);
LOCAL uint32_t read_ev_value(uint32_t value);
LOCAL uint32_t write_ev_value(uint32_t exposure_value);
LOCAL uint32_t read_gain_value(uint32_t value);
LOCAL uint32_t write_gain_value(uint32_t gain_value);
LOCAL uint32_t read_gain_scale(uint32_t value);
LOCAL uint32_t set_frame_rate(uint32_t param);
LOCAL uint32_t set_hi702_ev(uint32_t level);
LOCAL uint32_t set_hi702_awb(uint32_t mode);
LOCAL uint32_t set_hi702_anti_flicker(uint32_t mode);
LOCAL uint32_t set_hi702_video_mode(uint32_t mode);


LOCAL uint32_t HI702_After_Snapshot(uint32_t param);
LOCAL uint32_t HI702_BeforeSnapshot(uint32_t param);


/**---------------------------------------------------------------------------*
 ** 						Local Variables 								 *
 **---------------------------------------------------------------------------*/
 typedef enum
{
        FLICKER_50HZ = 0,
        FLICKER_60HZ,
        FLICKER_MAX
}FLICKER_E;

SENSOR_REG_T hi702_YUV_640X480[]=
{

/*
{0x03,0x00},
{0x01,0xf1},
{0x01,0xf3},
{0x01,0xf1},
{0x03,0x20},
{0x10,0x1c},
{0x03,0x22},
{0x10,0x7b},
{0x03,0x00},
{0x0b,0xaa},
{0x0c,0xaa},
{0x0d,0xaa},
{0x10,0x00},
{0x11,0x90},
{0x12,0x04},
{0x20,0x00},
{0x21,0x06},
{0x22,0x00},
{0x23,0x06},
{0x24,0x01},
{0x25,0xe0},
{0x26,0x02},
{0x27,0x80},
*/

{0x03, 0x00},
{0x01, 0xf1},
{0x01, 0xf3},
{0x01, 0xf1},

{0x03, 0x20},//page 3
{0x10, 0x1c},//ae off
{0x03, 0x22},//page 4
{0x10, 0x6b},//awb off


{0x03, 0x00},
{0x10, 0x00},
{0x11, 0x90},
{0x12, 0x04},
{0x20, 0x04},
{0x21, 0x06},
{0x22, 0x00},
{0x23, 0x06},
{0x24, 0x01},
{0x25, 0xe0},
{0x26, 0x02},
{0x27, 0x80},
{0x40, 0x01},// Hblank 336
{0x41, 0x50},

{0x42, 0x00},// Vsync 170
{0x43, 0xaa},

////  BLC setting 090216 by steve
{0x80, 0x3e},
{0x81, 0x96},//add calvin 090410
{0x82, 0x90},
{0x83, 0x29},
{0x84, 0x00},
{0x85, 0x00},

{0x90, 0x0a},
{0x91, 0x0a},
{0x92, 0x48},
{0x93, 0x40},
{0x94, 0x88},//add calvin 090410
{0x95, 0x80},//add calvin 090410

{0x98, 0x0e},
{0xa0, 0x00},
{0xa2, 0x01},
{0xa4, 0x02},
{0xa6, 0x01},
{0xa8, 0x43},
{0xaa, 0x46},
{0xac, 0x42},
{0xae, 0x41},

{0x03, 0x02},
{0x10, 0x00},
{0x13, 0x00},
{0x18, 0x0C},
{0x19, 0x08},
{0x1a, 0x00},
{0x1b, 0x08},
{0x1C, 0x00},
{0x1D, 0x00},
{0x20, 0x33},
{0x21, 0x77},
{0x22, 0xA4},
{0x23, 0xb0},
{0x31, 0x99},
{0x32, 0x00},
{0x33, 0x00},
{0x34, 0x3C},
{0x50, 0x21},
{0x54, 0x30},
{0x56, 0xfe},
{0x62, 0x78},
{0x63, 0x9E},
{0x64, 0x78},
{0x65, 0x9E},
{0x72, 0x8A},
{0x73, 0x9A},
{0x74, 0x8A},
{0x75, 0x9A},
{0xa0, 0x03},
{0xa8, 0x1D},
{0xaa, 0x49},
{0xbf, 0x14},

{0x03, 0x10},//page 10:image effect
{0x10, 0x03},//ISPCTL1
{0x12, 0x30},//Y offet, dy offset enable
{0x40, 0x00},
{0x41, 0x00},  
{0x50, 0x90},

{0x60, 0x1f},
{0x61, 0x88},
{0x62, 0x88},
{0x63, 0x30},
{0x64, 0x80},

{0x03, 0x11},
{0x10, 0x1d},
{0x11, 0x0e},
{0x21, 0x04},
{0x60, 0x04},
{0x62, 0x43},
{0x63, 0x63},

{0x03, 0x12},
{0x40, 0x23},
{0x41, 0x37},
{0x50, 0x04},
{0x70, 0x1d},
{0x74, 0x08},
{0x75, 0x08},
{0x91, 0x10},
{0xd0, 0xb1},

{0x03, 0x13},
{0x10, 0x1f},
{0x11, 0x07},
{0x12, 0x01},
{0x13, 0x02},
{0x20, 0x03},
{0x21, 0x03},
{0x23, 0x14},
{0x24, 0x01},
{0x25, 0x00},

{0x80, 0x1d},
{0x81, 0x01},

{0x83, 0x5d},

{0x90, 0x01},
{0x91, 0x01},
{0x93, 0x15},
{0x94, 0x01},
{0x95, 0x00},

{0x03, 0x14},
{0x10, 0x01},
{0x20, 0x80},
{0x21, 0x80},
{0x22, 0x69},
{0x23, 0x56},
{0x24, 0x4e},

//  CMC
{0x03, 0x15},
{0x10, 0x03},
{0x14, 0x58},
{0x16, 0x40},
{0x17, 0x3f},

{0x30, 0xb3},
{0x31, 0x29},
{0x32, 0x0a},
{0x33, 0x17},
{0x34, 0xaf},
{0x35, 0x18},
{0x36, 0x02},
{0x37, 0x2a},
{0x38, 0x6c},

{0x40, 0x00},
{0x41, 0x00},
{0x42, 0x00},
{0x43, 0x00},
{0x44, 0x00},
{0x45, 0x00},
{0x46, 0x00},
{0x47, 0x00},
{0x48, 0x00},

{0x03, 0x16},
{0x30, 0x00},
{0x31, 0x13},
{0x32, 0x1f},
{0x33, 0x33},
{0x34, 0x56},
{0x35, 0x74},
{0x36, 0x8d},
{0x37, 0x9f},
{0x38, 0xb0},
{0x39, 0xbf},
{0x3a, 0xca},
{0x3b, 0xdb},
{0x3c, 0xe5},
{0x3d, 0xec},
{0x3e, 0xf0},

{0x03, 0x17},
{0xc0, 0x03},

{0xc4, 0x3c},
{0xc5, 0x32},

{0xc6, 0x02},
{0xc7, 0x20},

//PAGE 20
{0x03, 0x20},
{0x10, 0x1c},//100Hz
{0x11, 0x00},

{0x20, 0x01},
{0x28, 0x0f},
{0x29, 0xa3},
{0x2a, 0xf0},
{0x2b, 0x34},//1/120 Anti banding

{0x30, 0x78},//0xf8->0x78 1/120 Anti banding

{0x60, 0x80},

{0x70, 0x42},

{0x78, 0x11},//yth1
{0x79, 0x26},//yth2
{0x7A, 0x22},//yth3

{0x83, 0x01},//EXP Normal 20.00 fps 
{0x84, 0x24}, 
{0x85, 0xf8}, 
{0x86, 0x00},//EXPMin 6000.00 fps
{0x87, 0xfa}, 
{0x88, 0x02},//EXP Max 25.00 fps 
{0x89, 0x49}, 
{0x8a, 0xf0}, 
{0x8B, 0x3a},//EXP100 
{0x8C, 0x98}, 
{0x8D, 0x30},//EXP120 
{0x8E, 0xd4}, 
{0x91, 0x01},//EXP Fix 17.02 fps
{0x92, 0x33}, 
{0x93, 0x9e}, 

{0x98, 0x8c},//outdoor th1
{0x99, 0x23},//outdoor th2

{0x9c, 0x0b},//EXP Limit 857.14 fps 
{0x9d, 0xb8}, 
{0x9e, 0x00},//EXP Unit 
{0x9f, 0xfa}, 

{0xb0, 0x10},
{0xb1, 0x10},
{0xb2, 0x50},
{0xb3, 0x10},
{0xb4, 0x10},
{0xb5, 0x30},
{0xb6, 0x20},
{0xb7, 0x1a},
{0xb8, 0x18},
{0xb9, 0x16},
{0xba, 0x15},
{0xbb, 0x14},
{0xbc, 0x14},
{0xbd, 0x13},

{0xc0, 0x10},//skygain

{0xc8, 0x90},
{0xc9, 0x80},

//Page 22

{0x03, 0x22},
{0x10, 0x6b},
{0x11, 0x28},
{0x21, 0x40},

{0x30, 0x7e},
{0x31, 0x80},
{0x38, 0x12},
{0x39, 0x66},

{0x40, 0xf3},
{0x41, 0x55},
{0x42, 0x33},
{0x43, 0xf0},
{0x44, 0xaa},
{0x45, 0x66},
{0x46, 0x0a},

{0x80, 0x4f},
{0x81, 0x20},
{0x82, 0x30},
{0x83, 0x65},
{0x84, 0x20},
{0x85, 0x68},
{0x86, 0x18},

{0x87, 0x65},
{0x88, 0x4e},
{0x89, 0x3b},
{0x8a, 0x20},

{0x8b, 0x03},
{0x8d, 0x14},
{0x8e, 0x41},

{0x8f, 0x61},
{0x90, 0x61},
{0x91, 0x61},
{0x92, 0x55},
{0x93, 0x48},
{0x94, 0x46},
{0x95, 0x46},
{0x96, 0x46},
{0x97, 0x35},
{0x98, 0x28},
{0x99, 0x28},
{0x9a, 0x28},
{0x9b, 0x05},

{0xb4, 0xea},

{0x03, 0x22},
{0x10, 0xeb},

{0x03, 0x20},
{0x10, 0x9c},//50Hz

{0x01, 0xf0},

{SENSOR_WRITE_DELAY, 20},//delay 20ms





};

LOCAL SENSOR_REG_TAB_INFO_T s_HI702_resolution_Tab_YUV[]=
{
        // COMMON INIT
        {ADDR_AND_LEN_OF_ARRAY(hi702_YUV_640X480), 640, 480, 24, SENSOR_IMAGE_FORMAT_YUV422},

        // YUV422 PREVIEW 1	
        {PNULL, 0, 640, 480,24, SENSOR_IMAGE_FORMAT_YUV422},
        {PNULL, 0, 0, 0, 0, 0},
        {PNULL, 0, 0, 0, 0, 0},
        {PNULL, 0, 0, 0, 0, 0},

        // YUV422 PREVIEW 2 
        {PNULL, 0, 0, 0, 0, 0},
        {PNULL, 0, 0, 0, 0, 0},
        {PNULL, 0, 0, 0, 0, 0},
        {PNULL, 0, 0, 0, 0, 0}
};

LOCAL SENSOR_IOCTL_FUNC_TAB_T s_HI702_ioctl_func_tab = 
{
        // Internal 
        PNULL,
	 _hi702_PowerOn,
        PNULL,
        HI702_Identify,

        PNULL,			// write register
        PNULL,			// read  register	
        PNULL,
        PNULL,

        // External
        set_hi702_ae_enable,
        set_hmirror_enable,
        set_vmirror_enable,

        set_brightness,
        set_contrast,
        set_sharpness,
        set_saturation,

        set_preview_mode,	
        set_image_effect,

        HI702_BeforeSnapshot,
        HI702_After_Snapshot,

        PNULL,

        read_ev_value,
        write_ev_value,
        read_gain_value,
        write_gain_value,
        read_gain_scale,
        set_frame_rate,	
        PNULL,
        PNULL,
        set_hi702_awb,
        PNULL,
        PNULL,
        set_hi702_ev,
        PNULL,
        PNULL,
        PNULL,
        PNULL,
        PNULL,
        set_hi702_anti_flicker,
        set_hi702_video_mode,
        PNULL,
        PNULL,
        PNULL,
        PNULL,
#ifdef CONFIG_CAMERA_SENSOR_NEW_FEATURE
	PNULL,
	PNULL
#else
	PNULL,
	PNULL
#endif
};

/**---------------------------------------------------------------------------*
 ** 						Global Variables								  *
 **---------------------------------------------------------------------------*/
 SENSOR_INFO_T g_HI702_yuv_info =
{
	HI702_I2C_ADDR_W,				// salve i2c write address
	HI702_I2C_ADDR_R, 				// salve i2c read address

	SENSOR_I2C_VAL_8BIT|SENSOR_I2C_REG_8BIT|SENSOR_I2C_FREQ_400, // bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
								// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
								// bit2: 0: i2c register addr  is 8 bit, 1: i2c register addr  is 16 bit
								// other bit: reseved
	SENSOR_HW_SIGNAL_PCLK_P|\
	SENSOR_HW_SIGNAL_VSYNC_N|\
	SENSOR_HW_SIGNAL_HSYNC_P,		// bit0: 0:negative; 1:positive -> polarily of pixel clock
								// bit2: 0:negative; 1:positive -> polarily of horizontal synchronization signal
								// bit4: 0:negative; 1:positive -> polarily of vertical synchronization signal
								// other bit: reseved											
										
	// preview mode
	SENSOR_ENVIROMENT_NORMAL|\
	SENSOR_ENVIROMENT_NIGHT|\
	SENSOR_ENVIROMENT_SUNNY,		

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

	7,								// bit[0:7]: count of step in brightness, contrast, sharpness, saturation
								// bit[8:31] reseved

	SENSOR_LOW_PULSE_RESET,			// reset pulse level
	100,								// reset pulse width(ms)

	SENSOR_LOW_LEVEL_PWDN,			// 1: high level valid; 0: low level valid	

	2,								// count of identify code
	{{0x04, 0x8c},						// supply two code to identify sensor.
	{0x04, 0x8c}},						// for Example: index = 0-> Device id, index = 1 -> version id	
								
	SENSOR_AVDD_2800MV,				// voltage of avdd	

	640,							// max width of source image
	480,							// max height of source image
	"HI702",						// name of sensor												

	SENSOR_IMAGE_FORMAT_YUV422,		// define in SENSOR_IMAGE_FORMAT_E enum,
								// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T
	SENSOR_IMAGE_PATTERN_YUV422_YUYV,	// pattern of input image form sensor;			

	s_HI702_resolution_Tab_YUV,	// point to resolution table information structure
	&s_HI702_ioctl_func_tab,		// point to ioctl function table
		
	PNULL,							// information and table about Rawrgb sensor
	PNULL,							// extend information about sensor	
	SENSOR_AVDD_1800MV,                     // iovdd
#if 1  //change dvdd value  ao.sun 20130828
	SENSOR_AVDD_1800MV, 					// dvdd
#else
	SENSOR_AVDD_1200MV,                      // dvdd
#endif	
	1,
	0,
	0,
	2,        
	0,			// threshold enable
	0,			// threshold mode
	0,			// threshold start postion
	0,			// threshold end postion
	0,
	{0, 2, 8, 1},
	PNULL,
	1
};
/**---------------------------------------------------------------------------*
 ** 							Function  Definitions
 **---------------------------------------------------------------------------*/
LOCAL void HI702_WriteReg( uint8_t  subaddr, uint8_t data )
{	
#ifndef	_USE_DSP_I2C_
        Sensor_WriteReg_8bits(subaddr, data);
#else
        DSENSOR_IICWrite((uint16_t)subaddr, (uint16_t)data);
#endif

        SENSOR_TRACE("SENSOR: HI702_WriteReg reg/value(%x,%x) !!\n", subaddr, data);
}
LOCAL uint8_t HI702_ReadReg( uint8_t  subaddr)
{
        uint8_t value = 0;

#ifndef	_USE_DSP_I2C_
        value = Sensor_ReadReg( subaddr);
#else
        value = (uint16_t)DSENSOR_IICRead((uint16_t)subaddr);
#endif

        SENSOR_TRACE("SENSOR: HI702_ReadReg reg/value(%x,%x) !!\n", subaddr, value);
        return value;
}

LOCAL uint32_t _hi702_PowerOn(uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_HI702_yuv_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_HI702_yuv_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_HI702_yuv_info.iovdd_val;
	BOOLEAN power_down = g_HI702_yuv_info.power_down_level;
	BOOLEAN reset_level = g_HI702_yuv_info.reset_pulse_level;
	SENSOR_PRINT("SENSOR_HI702: _hi702_Power_On:E!!  (1:on, 0:off): %d ------sunaodebug---- =\n", power_on);


	if (SENSOR_TRUE == power_on) {
                Sensor_PowerDown(power_down);
                SENSOR_Sleep(10);
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
		SENSOR_Sleep(10);

		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);	
		SENSOR_Sleep(10);
	}
	SENSOR_PRINT("SENSOR_HI702: _hi702_Power_On(1:on, 0:off): %d ------sunaodebug----\n", power_on);
	return SENSOR_SUCCESS;
}

LOCAL uint32_t HI702_Identify(uint32_t param)
{

        uint32_t i = 0;
        uint32_t nLoop = 0;
        uint8_t ret = 0;
        uint32_t err_cnt = 0;
        uint8_t reg[2] 	= {0x04, 0x04};
        uint8_t value[2] 	= {0x8c, 0x8c};

    SENSOR_Sleep(50);

	HI702_WriteReg(0x03, 0x00);
	HI702_WriteReg(0x01, 0xf1);
	HI702_WriteReg(0x01, 0xf3);
	HI702_WriteReg(0x01, 0xf1);

        SENSOR_TRACE("HI702_Identify-----sunao702----\n");
        for(i = 0; i<2; ) {
                nLoop = 1000;
                ret = HI702_ReadReg(reg[i]);
        	   SENSOR_TRACE("HI702 read reg0x00 = 0x%x -----sunao702----\n", ret);
                if( ret != value[i]) {
                        err_cnt++;
                        if(err_cnt>3) {
                                SENSOR_PRINT_HIGH("It is not HI702\n");
                                return SENSOR_FAIL;
                        } else {
                                while(nLoop--);
                                continue;
                        }
                }
                err_cnt = 0;
                i++;
        }

        SENSOR_TRACE("HI702_Identify: it is HI702----sunao702---\n");
        return (uint32_t)SENSOR_SUCCESS;
}

LOCAL uint32_t set_hi702_ae_enable(uint32_t enable)
{
        SENSOR_TRACE("set_hi702_ae_enable: enable = %d\n", enable);
        return 0;
}
LOCAL uint32_t set_hmirror_enable(uint32_t enable)
{
#if 0
        uint8_t value = 0;	
        value = HI702_ReadReg(0x14);
        value = (value & 0xFE) | (enable == 1 ? 0 : 1); //landscape
        SENSOR_TRACE("set_hmirror_enable: enable = %d, 0x14: 0x%x.\n", enable, value);
        HI702_WriteReg(0x14, value);
#endif		
        return 0;
}
LOCAL uint32_t set_vmirror_enable(uint32_t enable)
{
#if 0
        uint8_t value = 0;	
        value = HI702_ReadReg(0x14);
        value = (value & 0xFD) | ((enable & 0x1) << 1); //portrait
        SENSOR_TRACE("set_vmirror_enable: enable = %d, 0x14: 0x%x.\n", enable, value);
        HI702_WriteReg(0x14, value);
#endif		
        return 0;
}
/******************************************************************************/
// Description: set brightness 
// Global resource dependence: 
// Author:
// Note:
//		level  must smaller than 8
/******************************************************************************/
SENSOR_REG_T hi702_brightness_tab[][2]=
{
        {		
        	{0xb5, 0xd0},{0xff,0xff},
        },

        {
        	{0xb5, 0xe0},{0xff,0xff},
        },

        {
        	{0xb5, 0xf0},{0xff,0xff},
        },

        {
        	{0xb5, 0x00},{0xff,0xff},
        },

        {
        	{0xb5, 0x20},{0xff,0xff},
        },

        {
        	{0xb5, 0x30},{0xff,0xff},
        },

        {
        	{0xb5, 0x40},{0xff,0xff},
        },
};

LOCAL uint32_t set_brightness(uint32_t level)
{
#if 0
        uint16_t i;
        SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)hi702_brightness_tab[level];

        if(level>6)
                return 0;
	
        for(i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) && (0xFF != sensor_reg_ptr[i].reg_value); i++) {
                HI702_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
        }
#endif		
        SENSOR_TRACE("set_brightness: level = %d\n", level);
        return 0;
}

SENSOR_REG_T HI702_ev_tab[][3]=
{   
        {{0xd3, 0x48}, {0xb5, 0xd0},{0xff, 0xff}},
        {{0xd3, 0x50}, {0xb5, 0xe0},{0xff, 0xff}},
        {{0xd3, 0x58}, {0xb5, 0xf0},{0xff, 0xff}},
        {{0xd3, 0x60}, {0xb5, 0x10},{0xff, 0xff}},
        {{0xd3, 0x68}, {0xb5, 0x20},{0xff, 0xff}},
        {{0xd3, 0x70}, {0xb5, 0x30},{0xff, 0xff}},
        {{0xd3, 0x78}, {0xb5, 0x40},{0xff, 0xff}},    
};

LOCAL uint32_t set_hi702_ev(uint32_t level)
{
#if 0
        uint16_t i; 
        SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)HI702_ev_tab[level];

        if(level>6)
                return 0;

        for(i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) ||(0xFF != sensor_reg_ptr[i].reg_value) ; i++) {
                HI702_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
        }
#endif
        SENSOR_TRACE("SENSOR: set_ev: level = %d\n", level);
        return 0;
}

/******************************************************************************/
// Description: anti 50/60 hz banding flicker
// Global resource dependence: 
// Author:
// Note:
//		level  must smaller than 8
/******************************************************************************/
LOCAL uint32_t set_hi702_anti_flicker(uint32_t param )
{
        switch (param) {
        case FLICKER_50HZ:
                HI702_WriteReg(0x03, 0x20); 	
                HI702_WriteReg(0x10, 0x9c); 
                break;
        case FLICKER_60HZ:
                HI702_WriteReg(0x03, 0x20); 	
                HI702_WriteReg(0x10, 0x8c); 
                break;
        default:
                break;
        }
		
        return 0;
}

/******************************************************************************/
// Description: set video mode
// Global resource dependence: 
// Author:
// Note:
//		 
/******************************************************************************/
LOCAL uint32_t set_hi702_video_mode(uint32_t mode)
{
#if 0
        if(0 == mode)
                HI702_WriteReg(0xec,0x20);
        else if(1 == mode)
                HI702_WriteReg(0xec,0x00);
        SENSOR_TRACE("SENSOR: HI702_ReadReg(0xec) = %x\n", HI702_ReadReg(0xec));
#endif		
        SENSOR_TRACE("SENSOR: set_video_mode: mode = %d\n", mode);
        return 0;
}
/******************************************************************************/
// Description: set wb mode 
// Global resource dependence: 
// Author:
// Note:
//		
/******************************************************************************/
SENSOR_REG_T HI702_awb_tab[][5]=
{
        //AUTO
        {
                {0x5a, 0x4c}, {0x5b, 0x40}, {0x5c, 0x4a},
                {0x22, 0x57},    // the reg value is not written here, rewrite in set_HI702_awb();
                {0xff, 0xff}
        },	  
        //INCANDESCENCE:
        {
                {0x22, 0x55},	 // Disable AWB 
                {0x5a, 0x48},{0x5b, 0x40},{0x5c, 0x5c},
                {0xff, 0xff} 
        },
        //U30 ?
        {
                {0x41, 0x39},
                {0xca, 0x60},
                {0xcb, 0x40},
                {0xcc, 0x50},
                {0xff, 0xff}      
        },  
        //CWF ?
        {
                {0x41, 0x39},
                {0xca, 0x60},
                {0xcb, 0x40},
                {0xcc, 0x50},
                {0xff, 0xff}            
        },    
        //FLUORESCENT:
        {
                {0x22, 0x55},	// Disable AWB 
                {0x5a, 0x40},{0x5b, 0x42}, {0x5c, 0x50},
                {0xff, 0xff} 
        },
        //SUN:
        {
                {0x22, 0x55},	// Disable AWB
                {0x5a, 0x45},{0x5b, 0x3a},{0x5c, 0x40},
                {0xff, 0xff} 
        },
        //CLOUD:
        {
                {0x22, 0x55},   // Disable AWB
                {0x5a, 0x4a}, {0x5b, 0x32},{0x5c, 0x40},
                {0xff, 0xff} 
        },
};
	
LOCAL uint32_t set_hi702_awb(uint32_t mode)
{
#if 0
        uint8_t awb_en_value;
        uint16_t i;
        SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)HI702_awb_tab[mode];
        
        awb_en_value = HI702_ReadReg(0x22);	

        if(mode>6)
                return 0;

        for(i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) || (0xFF != sensor_reg_ptr[i].reg_value); i++) {
                if(0x22 == sensor_reg_ptr[i].reg_addr) {
                        if(mode == 0)
                                HI702_WriteReg(0x22, awb_en_value |0x02 );
                        else
                                HI702_WriteReg(0x22, awb_en_value &0xfd );
                } else {
                        HI702_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
                }
        }
#endif		
        SENSOR_TRACE("SENSOR: set_awb_mode: mode = %d\n", mode);

        return 0;
}

SENSOR_REG_T hi702_contrast_tab[][2]=
{
        //level 0
        {
        	{0xb3,0x50},{0xff,0xff},
        },
        //level 1
        {
        	{0xb3,0x48},{0xff,0xff}, 
        },
        //level 2
        {
        	{0xb3,0x44},{0xff,0xff}, 
        },
        //level 3
        {
        	{0xb3,0x3c},{0xff,0xff},
        },
        //level 4
        {
        	{0xb3,0x3d},{0xff,0xff}, 
        },
        //level 5
        {
        	{0xb3,0x38},{0xff,0xff}, 
        },
        //level 6
        {
        	{0xb3,0x34},{0xff,0xff},
        },
};
LOCAL uint32_t set_contrast(uint32_t level)
{
#if 0
        uint16_t i;
        SENSOR_REG_T* sensor_reg_ptr;

        sensor_reg_ptr = (SENSOR_REG_T*)hi702_contrast_tab[level];

        if(level>6)
                return 0;

        for(i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) && (0xFF != sensor_reg_ptr[i].reg_value); i++) {
                HI702_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
        }
#endif
        SENSOR_TRACE("set_contrast: level = %d\n", level);
        return 0;
}
LOCAL uint32_t set_sharpness(uint32_t level)
{
        return 0;
}
LOCAL uint32_t set_saturation(uint32_t level)
{
        return 0;
}

/******************************************************************************/
// Description: set brightness 
// Global resource dependence: 
// Author:
// Note:
//		level  must smaller than 8
/******************************************************************************/
LOCAL uint32_t set_preview_mode(uint32_t preview_mode)
{
        SENSOR_TRACE("set_preview_mode: preview_mode = %d\n", preview_mode);

        set_hi702_anti_flicker(0);
#if 0		
        switch (preview_mode) {
        case DCAMERA_ENVIRONMENT_NORMAL: 
                HI702_WriteReg(0xec,0x20);
                break;
        case DCAMERA_ENVIRONMENT_NIGHT:
                HI702_WriteReg(0xec,0x30);
                break;
        case DCAMERA_ENVIRONMENT_SUNNY:
                HI702_WriteReg(0xec,0x10);
                break;
        default:
                break;
        }
#endif		
        SENSOR_Sleep(10);
        return 0;
}
	
SENSOR_REG_T HI702_image_effect_tab[][11]=	
{
        // effect normal
        {
                {0x23,0x00}, {0x2d,0x0a}, {0x20,0x7f}, {0xd2,0x90}, {0x73,0x00}, {0x77,0x78},
                {0xb3,0x42}, {0xb4,0x80}, {0xba,0x00}, {0xbb,0x00}, {0xff,0xff}
        },
        //effect BLACKWHITE
        {
                {0x23,0x02}, {0x2d,0x0a}, {0x20,0x7f}, {0xd2,0x90}, {0x73,0x00},  
                {0xb3,0x40},	{0xb4,0x80}, {0xba,0x00}, {0xbb,0x00}, {0xff,0xff}
        },
        // effect RED pink
        {
        //TODO: later work
                {0x23,0x02},{0x2d,0x0a},{0x20,0x7f},{0xd2,0x90},{0x77,0x88},
                {0xb3,0x40},{0xb4,0x80},{0xba,0x10},{0xbb,0x50},{0xff, 0xff}
        },
        // effect GREEN
        {
                {0x23,0x02},{0x2d,0x0a},{0x20,0x7f},{0xd2,0x90},{0x77,0x88},
                {0xb3,0x40},{0xb4,0x80},{0xba,0xc0},{0xbb,0xc0},{0xff, 0xff}
        },
        // effect  BLUE
        {
                {0x23,0x02},{0x2d,0x0a},{0x20,0x7f},{0xd2,0x90},{0x73,0x00},
                {0xb3,0x40},{0xb4,0x80},{0xba,0x50},{0xbb,0xe0},{0xff, 0xff}
        },
        // effect  YELLOW
        {
                //TODO:later work
                {0x23,0x02},{0x2d,0x0a},{0x20,0x7f},{0xd2,0x90},{0x77,0x88},
                {0xb3,0x40},{0xb4,0x80},{0xba,0x80},{0xbb,0x20},{0xff, 0xff}
        },  
        // effect NEGATIVE
        {	     
                {0x23,0x01},{0x2d,0x0a},{0x20,0x7f},{0xd2,0x90},{0x73,0x00},
                {0xb3,0x40},{0xb4,0x80},{0xba,0x00},{0xbb,0x00},{0xff, 0xff}
        },    
        //effect ANTIQUE
        {
                {0x23,0x02},{0x2d,0x0a},{0x20,0x7f},{0xd2,0x90},{0x73,0x00},
                {0xb3,0x40},{0xb4,0x80},{0xba,0xd0},{0xbb,0x28},{0xff, 0xff}
        },
};
LOCAL uint32_t set_image_effect(uint32_t effect_type)
{
#if 0
        uint16_t i;
        SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)HI702_image_effect_tab[effect_type];
        if(effect_type>7)
                return 0;

        for(i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) || (0xFF != sensor_reg_ptr[i].reg_value) ; i++) {
                Sensor_WriteReg_8bits(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
        }
#endif		
        SENSOR_TRACE("-----------set_image_effect: effect_type = %d------------\n", effect_type);
        return 0;
}

LOCAL uint32_t HI702_After_Snapshot(uint32_t param)
{

#if 0
	Sensor_SetMCLK(24);
	
	HI702_WriteReg(0x41,HI702_ReadReg(0x41) | 0xf7);
	SENSOR_Sleep(200);
#endif	
	return 0;
    
}

LOCAL uint32_t HI702_BeforeSnapshot(uint32_t param)
{
#if 0

    uint16_t shutter = 0x00;
    uint16_t temp_reg = 0x00;
    uint16_t temp_r =0x00;
    uint16_t temp_g =0x00;
    uint16_t temp_b =0x00;    
    BOOLEAN b_AEC_on;
    

    SENSOR_TRACE("HI702_BeforeSnapshot ");   
    	if(HI702_ReadReg(0X41)  & 0x08 == 0x08)  //AEC on
    		b_AEC_on = SENSOR_TRUE;
    	else
    		b_AEC_on = SENSOR_FALSE;

	temp_reg = HI702_ReadReg(0xdb);
	temp_r = HI702_ReadReg(0xcd);
	temp_g = HI702_ReadReg(0xce);
	temp_b = HI702_ReadReg(0xcf);

	shutter = (HI702_ReadReg(0x03)<<8)  | (HI702_ReadReg(0x04)&0x00ff) ;
	shutter = shutter /2;

	if(b_AEC_on)
		HI702_WriteReg(0x41,HI702_ReadReg(0x41) & 0xc5); //0x01);
	SENSOR_Sleep(300); 

///12m
	Sensor_SetMCLK(12);
	
	HI702_WriteReg(0x03,shutter/256);
	HI702_WriteReg(0x04,shutter & 0x00ff);	
   	//SENSOR_TRACE("HI702_BeforeSnapshot, temp_r=%x,temp_reg=%x, final = %x ",temp_r,temp_reg, temp_r*temp_reg/ 0x80);    

	temp_r = (temp_r*temp_reg) / 0x80;
	temp_g = (temp_g*temp_reg) / 0x80;
	temp_b = (temp_b*temp_reg) / 0x80;
	if(b_AEC_on)
	{
		HI702_WriteReg(0xcd, temp_r);
		HI702_WriteReg(0xce, temp_g);
		HI702_WriteReg(0xcf , temp_b);
	}
   	//SENSOR_TRACE("HI702_BeforeSnapshot, temp_r=%x,temp_g=%x, temp_b = %x ",temp_r,temp_g,temp_b);    

	SENSOR_Sleep(300); 
#endif	
    	return 0;
    
}

LOCAL uint32_t read_ev_value(uint32_t value)
{
        return 0;
}
LOCAL uint32_t write_ev_value(uint32_t exposure_value)
{
        return 0;	
}
LOCAL uint32_t read_gain_value(uint32_t value)
{
        return 0;
}
LOCAL uint32_t write_gain_value(uint32_t gain_value)
{	
        return 0;
}
LOCAL uint32_t read_gain_scale(uint32_t value)
{
        return SENSOR_GAIN_SCALE;
}
LOCAL uint32_t set_frame_rate(uint32_t param)  
{
        //HI702_WriteReg( 0xd8, uint8_t data );
        return 0;
}
#if 0
struct sensor_drv_cfg sensor_hi702 = {
        .sensor_pos = CONFIG_DCAM_SENSOR_POS_HI702,
        .sensor_name = "hi702",
        .driver_info = &g_HI702_yuv_info,
};

static int __init sensor_hi702_init(void)
{
        return dcam_register_sensor_drv(&sensor_hi702);
}

subsys_initcall(sensor_hi702_init);
#endif
