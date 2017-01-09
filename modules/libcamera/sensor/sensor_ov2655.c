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
#include "sensor.h"
/**---------------------------------------------------------------------------*
 ** 						   Macro Define
 **---------------------------------------------------------------------------*/
#define SENSOR_TRACE SENSOR_PRINT
#define OV2655_I2C_ADDR_W	0x30	//wxz: for nex I2C
#define OV2655_I2C_ADDR_R		0x30

/**---------------------------------------------------------------------------*
 ** 					Local Function Prototypes							  *
 **---------------------------------------------------------------------------*/
LOCAL uint32_t OV2655_set_ae_enable(uint32_t enable);
LOCAL uint32_t OV2655_set_hmirror_enable(uint32_t enable);
LOCAL uint32_t OV2655_set_vmirror_enable(uint32_t enable);
LOCAL uint32_t OV2655_set_preview_mode(uint32_t preview_mode);
LOCAL uint32_t OV2655_Identify(uint32_t param);
LOCAL uint32_t OV2655_BeforeSnapshot(uint32_t param);
LOCAL uint32_t OV2655_set_brightness(uint32_t level);
LOCAL uint32_t OV2655_set_contrast(uint32_t level);
//LOCAL uint32_t OV2655_set_sharpness(uint32_t level);
//LOCAL uint32_t OV2655_set_saturation(uint32_t level);
LOCAL uint32_t OV2655_set_image_effect(uint32_t effect_type);
LOCAL uint32_t OV2655_set_work_mode(uint32_t mode);
LOCAL uint32_t OV2655_chang_image_format(uint32_t param);
LOCAL uint32_t OV2655_check_image_format_support(uint32_t param);
LOCAL uint32_t OV2655_after_snapshot(uint32_t param);
LOCAL uint32_t set_ov2655_ev(uint32_t level);
LOCAL uint32_t set_ov2655_awb(uint32_t mode);
LOCAL uint32_t set_ov2655_anti_flicker(uint32_t mode);
LOCAL uint32_t OV2655_GetExifInfo(uint32_t param);
LOCAL uint32_t OV2655_InitExifInfo(void);
//LOCAL uint32_t set_ov2655_video_mode(uint32_t mode);
//LOCAL uint32_t OV2655_zoom(uint32_t level);
/**---------------------------------------------------------------------------*
 ** 						Local Variables 								 *
 **---------------------------------------------------------------------------*/
//LOCAL uint32_t s_preview_mode;
LOCAL uint32_t s_image_effect = 0;
LOCAL uint32_t work_mode = 0;

SENSOR_REG_T ov2655_YUV_COMMON[] = {
	{0x3012, 0x80},
	{SENSOR_WRITE_DELAY, 0x0a},
	{0x308c, 0x80},
	{0x308d, 0x0e},
	{0x360b, 0x00},
	{0x30b0, 0xff},
	{0x30b1, 0xff},
	//wxz20110527: from 4x to 1x
	//{0x30b2, 0x04},
	{0x30b2, 0x00},

	{0x300e, 0x36},		//thomas 0x34
	{0x300f, 0xa6},
	{0x3010, 0x81},
	{0x3082, 0x01},
	{0x30f4, 0x01},
	{0x3090, 0x43},
	{0x3091, 0xc0},
	{0x30ac, 0x42},

	{0x30d1, 0x08},
	{0x30a8, 0x55},
	{0x3015, 0x02},
	{0x3093, 0x00},
	{0x307e, 0xe5},
	{0x3079, 0x00},
	{0x30aa, 0x42},
	{0x3017, 0x40},
	{0x30f3, 0x83},
	{0x306a, 0x0c},
	{0x306d, 0x00},
	{0x336a, 0x3c},
	{0x3076, 0x6a},
	{0x30d9, 0x95},
	{0x3016, 0x82},
	{0x3601, 0x30},
	{0x304e, 0x88},
	{0x30f1, 0x82},

	{0x3011, 0x00},		//thomas 0x01  //0909

	{0x3013, 0xf0},		//f7 to f0
	{0x3018, 0x70},
	{0x3019, 0x60},
	{0x301a, 0xd4},
	{0x301c, 0x13},
	{0x301d, 0x17},
	{0x3070, 0x5d},
	{0x3072, 0x5d},

	{0x30af, 0x00},
	{0x3048, 0x1f},
	{0x3049, 0x4e},
	{0x304a, 0x20},
	{0x304f, 0x20},
	{0x304b, 0x02},
	{0x304c, 0x00},
	{0x304d, 0x02},
	{0x304f, 0x20},
	{0x30a3, 0x10},
	{0x3013, 0xf7},
	{0x3014, 0x44},
	{0x3071, 0x00},
	{0x3070, 0x5d},
	{0x3073, 0x00},
	{0x3072, 0x5d},
	{0x301c, 0x12},
	{0x301d, 0x16},
	{0x304d, 0x42},
	{0x304a, 0x40},
	{0x304f, 0x40},
	{0x3095, 0x07},
	{0x3096, 0x16},
	{0x3097, 0x1d},

	{0x3020, 0x01},
	{0x3021, 0x18},
	{0x3022, 0x00},
	{0x3023, 0x0a},
	{0x3024, 0x06},
	{0x3025, 0x58},
	{0x3026, 0x04},
	{0x3027, 0xbc},
	{0x3088, 0x06},
	{0x3089, 0x40},
	{0x308a, 0x04},
	{0x308b, 0xb0},
	{0x3316, 0x64},
	{0x3317, 0x4b},
	{0x3318, 0x00},
	{0x331a, 0x64},
	{0x331b, 0x4b},
	{0x331c, 0x00},
	{0x3100, 0x00},

	{0x3320, 0xfa},
	{0x3321, 0x11},
	{0x3322, 0x92},
	{0x3323, 0x01},
	{0x3324, 0x97},
	{0x3325, 0x02},
	{0x3326, 0xff},
	{0x3327, 0x10},
	{0x3328, 0x10},
	{0x3329, 0x1f},
	{0x332a, 0x58},
	{0x332b, 0x50},
	{0x332c, 0xbe},
	{0x332d, 0xce},
	{0x332e, 0x2e},
	{0x332f, 0x36},
	{0x3330, 0x4d},
	{0x3331, 0x44},
	{0x3332, 0xf0},
	{0x3333, 0x0a},
	{0x3334, 0xf0},
	{0x3335, 0xf0},
	{0x3336, 0xf0},
	{0x3337, 0x40},
	{0x3338, 0x40},
	{0x3339, 0x40},
	{0x333a, 0x00},
	{0x333b, 0x00},

	{0x3380, 0x20},
	{0x3381, 0x5b},
	{0x3382, 0x05},
	{0x3383, 0x22},
	{0x3384, 0x9d},
	{0x3385, 0xc0},
	{0x3386, 0xb6},
	{0x3387, 0xb5},
	{0x3388, 0x02},
	{0x3389, 0x98},
	{0x338a, 0x00},

	{0x3340, 0x09},
	{0x3341, 0x19},
	{0x3342, 0x2f},
	{0x3343, 0x45},
	{0x3344, 0x5a},
	{0x3345, 0x69},
	{0x3346, 0x75},
	{0x3347, 0x7e},
	{0x3348, 0x88},
	{0x3349, 0x96},
	{0x334a, 0xa3},
	{0x334b, 0xaf},
	{0x334c, 0xc4},
	{0x334d, 0xd7},
	{0x334e, 0xe8},
	{0x334f, 0x20},

	{0x3350, 0x32},
	{0x3351, 0x25},
	{0x3352, 0x80},
	{0x3353, 0x1e},
	{0x3354, 0x00},
	{0x3355, 0x84},
	{0x3356, 0x32},
	{0x3357, 0x25},
	{0x3358, 0x80},
	{0x3359, 0x1b},
	{0x335a, 0x00},
	{0x335b, 0x84},
	{0x335c, 0x32},
	{0x335d, 0x25},
	{0x335e, 0x80},
	{0x335f, 0x1b},
	{0x3360, 0x00},
	{0x3361, 0x84},
	{0x3363, 0x70},
	{0x3364, 0x7f},
	{0x3365, 0x00},
	{0x3366, 0x00},

	{0x3301, 0xff},
	{0x338B, 0x1b},
	{0x338c, 0x1f},
	{0x338d, 0x40},

	{0x3370, 0xd0},
	{0x3371, 0x00},
	{0x3372, 0x00},
	{0x3373, 0x40},
	{0x3374, 0x10},
	{0x3375, 0x10},
	{0x3376, 0x04},
	{0x3377, 0x00},
	{0x3378, 0x04},
	{0x3379, 0x80},

	{0x3069, 0x86},
	//{0x307c, 0x13}, //0x12//flip and mirror
	{0x3087, 0x02},

	//{0x3090, 0x3a}, //0x32//array mirror
	{0x3090, 0x32},		//0x32//array mirror
	{0x307c, 0x10},		//0x12//flip and mirror
	{0x30aa, 0x32},
	{0x30a3, 0x80},
	{0x30a1, 0x41},

	{0x3300, 0xfc},
	{0x3302, 0x01},
	{0x3400, 0x00},
	{0x3606, 0x20},
	{0x3601, 0x30},
	{0x300e, 0x36},		//thomas 0x34
	{0x30f3, 0x83},
	{0x304e, 0x88},
	{0x363b, 0x01},
	{0x363c, 0xf2},

	{0x3023, 0x0c},
	{0x3319, 0x4c},

	{0x3086, 0x0f},
	{0x3086, 0x00},
	{0x3302, 0x11},
	{SENSOR_WRITE_DELAY, 150},
};

//pclk = 36MHz @24MHz MCLK
//7.5fps
SENSOR_REG_T ov2655_YUV_1600X1200[] = {
	//thomas 24M MCLK  input ,7.5fps,36M PCLK
	{0x300e, 0x34},		// 0x38 7.5fps
	{0x3011, 0x01},		// 0x00 7.5fps
	{0x3012, 0x00},
	{0x302A, 0x04},
	{0x302B, 0xd4},
	{0x306f, 0x54},
	{0x3020, 0x01},
	{0x3021, 0x18},
	{0x3022, 0x00},
	{0x3023, 0x0a},
	{0x3024, 0x06},
	{0x3025, 0x58},
	{0x3026, 0x04},
	{0x3027, 0xbc},
	{0x3088, 0x06},
	{0x3089, 0x40},
	{0x308a, 0x04},
	{0x308b, 0xb0},
	{0x3316, 0x64},
	{0x3317, 0x4b},
	{0x3318, 0x01},
	{0x3319, 0x2c},
	{0x331a, 0x64},
	{0x331b, 0x4b},
	{0x331c, 0x00},
	{0x331d, 0x4c},
	{0x3302, 0x01},

	{0x302c, 0x00},
	{0x3071, 0x00},
	{0x3070, 0x5d},
	{0x301c, 0x0d},
	{0x3073, 0x00},
	{0x3072, 0x5d},
	{0x301d, 0x0f},
	{0x3013, 0xf0},
	{0x3362, 0x80},
};

SENSOR_REG_T ov2655_YUV_1280X960[] = {
	//thomas 24M MCLK  input ,7.5fps ,36M PCLK
	{0x300e, 0x34},		// 0x38 7.5fps
	{0x3011, 0x01},		// 0x00 7.5fps
	{0x3012, 0x00},
	{0x302A, 0x04},
	{0x302B, 0xd4},
	{0x306f, 0x54},
	{0x3020, 0x01},
	{0x3021, 0x18},
	{0x3022, 0x00},
	{0x3023, 0x0c},
	{0x3024, 0x06},
	{0x3025, 0x58},
	{0x3026, 0x04},
	{0x3027, 0xbc},
	{0x3088, 0x05},
	{0x3089, 0x00},
	{0x308a, 0x03},
	{0x308b, 0xc0},
	{0x3316, 0x64},
	{0x3317, 0x4b},
	{0x3318, 0x01},
	{0x3319, 0x2c},
	{0x331a, 0x50},
	{0x331b, 0x3c},
	{0x331c, 0x00},
	{0x331d, 0x4c},
	{0x3302, 0x11},

	{0x302c, 0x00},
	{0x3071, 0x00},
	{0x3070, 0x5d},
	{0x301c, 0x0d},
	{0x3073, 0x00},
	{0x3072, 0x5d},
	{0x301d, 0x0f},
	{0x3013, 0xf0},
	{0x3362, 0x80},
};

//18MHz@24MHz MCLK
//15fps
static const SENSOR_REG_T ov2655_YUV_640X480[] =
    //thomas 24M MCLK  input ,15fps ,18M PCLK
{
	{0x300e, 0x34},		//thomas 0x34
	{0x3011, 0x01},		//thomas 0x01
	{0x3012, 0x10},
	{0x302a, 0x02},
	{0x302b, 0x6a},
	{0x302d, 0x00},
	{0x302e, 0x00},
	{0x306f, 0x14},
	{0x3014, 0x84},
	{0x3070, 0x84},

	{0x3020, 0x01},
	{0x3021, 0x18},
	{0x3022, 0x00},
	{0x3023, 0x06},
	{0x3024, 0x06},
	{0x3025, 0x58},
	{0x3026, 0x02},
	{0x3027, 0x61},

	{0x3088, 0x02},
	{0x3089, 0x80},
	{0x308a, 0x01},
	{0x308b, 0xe0},
	{0x3316, 0x64},
	{0x3317, 0x25},
	{0x3318, 0x80},
	{0x3319, 0x08},
	{0x331a, 0x28},
	{0x331b, 0x1e},
	{0x331c, 0x00},
	{0x331d, 0x38},
	{0x3302, 0x11},
	{0x3362, 0x90},
	{SENSOR_WRITE_DELAY, 150},
};

const SENSOR_REG_T ov2655_yuv422_mode[] = {
	 /**/ {0x3100, 0x00},
	{0x3300, 0xfc},
	{0x3301, 0xff},
	{0x3400, 0x01},		//thomas 0x00
	{0x3606, 0x20}
};

LOCAL SENSOR_REG_TAB_INFO_T s_OV2655_resolution_Tab_YUV[] = {
	// COMMON INIT
	{ADDR_AND_LEN_OF_ARRAY(ov2655_YUV_COMMON), 0, 0, 24,
	 SENSOR_IMAGE_FORMAT_YUV422},

	// YUV422 PREVIEW 1
	{ADDR_AND_LEN_OF_ARRAY(ov2655_YUV_640X480), 640, 480, 24,
	 SENSOR_IMAGE_FORMAT_YUV422},
	{ADDR_AND_LEN_OF_ARRAY(ov2655_YUV_1280X960), 1280, 960, 24,
	 SENSOR_IMAGE_FORMAT_YUV422},
	{ADDR_AND_LEN_OF_ARRAY(ov2655_YUV_1600X1200), 1600, 1200, 24,
	 SENSOR_IMAGE_FORMAT_YUV422},
	{PNULL, 0, 0, 0, 0, 0},

	// YUV422 PREVIEW 2
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

LOCAL SENSOR_IOCTL_FUNC_TAB_T s_OV2655_ioctl_func_tab = {
	// Internal
	PNULL,
	PNULL,
	PNULL,
	OV2655_Identify,

	PNULL,			// write register
	PNULL,			// read  register
	PNULL,
	PNULL,

	// External
	OV2655_set_ae_enable,
	OV2655_set_hmirror_enable,
	OV2655_set_vmirror_enable,

	OV2655_set_brightness,
	OV2655_set_contrast,
	PNULL,			//OV2655_set_sharpness,
	PNULL,			//OV2655_set_saturation,

	OV2655_set_preview_mode,
	OV2655_set_image_effect,

	OV2655_BeforeSnapshot,
	OV2655_after_snapshot,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	set_ov2655_awb,
	PNULL,
	PNULL,
	set_ov2655_ev,
	OV2655_check_image_format_support,
	OV2655_chang_image_format,
	PNULL,
	OV2655_GetExifInfo,	//wxz:???
	PNULL,
	set_ov2655_anti_flicker,
	PNULL,
	PNULL,
	PNULL,  //meter_mode
	PNULL, //get_status
	PNULL,
	PNULL,
};

/**---------------------------------------------------------------------------*
 ** 						Global Variables								  *
 **---------------------------------------------------------------------------*/
SENSOR_INFO_T g_OV2655_yuv_info = {
	OV2655_I2C_ADDR_W,	// salve i2c write address
	OV2655_I2C_ADDR_R,	// salve i2c read address

	SENSOR_I2C_REG_16BIT,	// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
	// bit1: 0: i2c register addr  is 8 bit, 1: i2c register addr  is 16 bit
	// other bit: reseved
	SENSOR_HW_SIGNAL_PCLK_N | SENSOR_HW_SIGNAL_VSYNC_N | SENSOR_HW_SIGNAL_HSYNC_P,	// bit0: 0:negative; 1:positive -> polarily of pixel clock
	// bit2: 0:negative; 1:positive -> polarily of horizontal synchronization signal
	// bit4: 0:negative; 1:positive -> polarily of vertical synchronization signal
	// other bit: reseved

	// preview mode
	SENSOR_ENVIROMENT_NORMAL |
	    SENSOR_ENVIROMENT_NIGHT | SENSOR_ENVIROMENT_SUNNY,

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

	0x77777,		//wxz:???
	//7,                                                            // bit[0:7]: count of step in brightness, contrast, sharpness, saturation
	// bit[8:31] reseved

	SENSOR_LOW_PULSE_RESET,	// reset pulse level
	20,			// reset pulse width(ms)

	SENSOR_HIGH_LEVEL_PWDN,	// 1: high level valid; 0: low level valid

	2,			// count of identify code
	//0x300A, 0x26,                                         // supply two code to identify sensor.
	//0x300B, 0x56,                                         // for Example: index = 0-> Device id, index = 1 -> version id
	{{0x300A, 0x26},	// supply two code to identify sensor.
	 {0x300B, 0x56}},	// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,	// voltage of avdd

	1600,			// max width of source image
	1200,			// max height of source image
	"OV2655",		// name of sensor

	SENSOR_IMAGE_FORMAT_YUV422,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_YUV422_YUYV,	// pattern of input image form sensor;

	s_OV2655_resolution_Tab_YUV,	// point to resolution table information structure
	&s_OV2655_ioctl_func_tab,	// point to ioctl function table

	PNULL,			// information and table about Rawrgb sensor
	PNULL,			// extend information about sensor
	SENSOR_AVDD_2800MV,	// iovdd
	SENSOR_AVDD_1300MV,	// dvdd
	4,			// skip frame num before preview
	3,			// skip frame num before capture
	0,			// deci frame num during preview;
	0,			// deci frame num during video preview;
	0,			// threshold enable
	0,			// threshold mode
	0,			// threshold start postion
	0,			// threshold end postion
	0,			// i2c_dev_handler
	{0, 2, 8, 1},
	PNULL,
	4,			// skip frame num while change setting
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
LOCAL uint32_t OV2655_set_ae_enable(uint32_t enable)
{
	return 0;
}

/******************************************************************************/
// Description: anti 50/60 hz banding flicker
// Global resource dependence:
// Author:
// Note:
//              level  must smaller than 8
/******************************************************************************/
const SENSOR_REG_T ov2655_banding_flicker_tab[][3] =	//__align(4) const SENSOR_REG_T ov2655_banding_flicker_tab[][3]=
{
	// 50HZ
	{
	 {0x3014, 0x84}, {0x3013, 0xf7}, {0xffff, 0xff}
	 },
	//60HZ
	{
	 {0x3014, 0x04}, {0x3013, 0xf7}, {0xffff, 0xff}
	 },
};

LOCAL uint32_t set_ov2655_anti_flicker(uint32_t mode)
{				//107 us
	uint16_t i;
	SENSOR_REG_T *sensor_reg_ptr = PNULL;
	if (mode > 1)
		return 0;

	sensor_reg_ptr = (SENSOR_REG_T *) ov2655_banding_flicker_tab[mode];

	for (i = 0;
	     (0xFFFF != sensor_reg_ptr[i].reg_addr)
	     || (0xFF != sensor_reg_ptr[i].reg_value); i++) {
		Sensor_WriteReg(sensor_reg_ptr[i].reg_addr,
				sensor_reg_ptr[i].reg_value);
	}
	SENSOR_Sleep(10);
	SENSOR_TRACE("SENSOR: set_ov2655_flicker: 0x%x", mode);
	return 0;
}

/*
LOCAL uint32_t set_ov2655_video_mode(uint32_t mode)
{
    return 0;
}
*/
/******************************************************************************/
// Description: set wb mode
// Global resource dependence:
// Author:
// Note:
//
/******************************************************************************/
const SENSOR_REG_T ov2655_awb_tab[][5] = {
	//AUTO
	{
	 {0x3306, 0x00},
	 {0xffff, 0xff},
	 {0xffff, 0xff},
	 {0xffff, 0xff},
	 {0xffff, 0xff}
	 },
	//OFFICE:
	{
	 {0x3306, 0x02},
	 {0x3337, 0x52},
	 {0x3338, 0x40},
	 {0x3339, 0x58},
	 {0xffff, 0xff}
	 },
	//U30
	{
	 {0xffff, 0xff},
	 {0xffff, 0xff},
	 {0xffff, 0xff},
	 {0xffff, 0xff},
	 {0xffff, 0xff},
	 },
	//CWF
	{
	 {0xffff, 0xff},
	 {0xffff, 0xff},
	 {0xffff, 0xff},
	 {0xffff, 0xff},
	 {0xffff, 0xff},
	 },
	//HOME
	{
	 {0x3306, 0x02},
	 {0x3337, 0x44},
	 {0x3338, 0x40},
	 {0x3339, 0x70},
	 {0xffff, 0xff}
	 },

	//SUN:
	{
	 {0x3306, 0x82},
	 {0x3337, 0x68},
	 {0x3338, 0x40},
	 {0x3339, 0x4e},
	 {0xffff, 0xff}
	 },
	//CLOUD:
	{
	 {0x3306, 0x02},
	 {0x3337, 0x5e},
	 {0x3338, 0x40},
	 {0x3339, 0x46},
	 {0xffff, 0xff}
	 }
};

LOCAL uint32_t set_ov2655_awb(uint32_t mode)
{
	uint16_t i;
	SENSOR_REG_T *sensor_reg_ptr = (SENSOR_REG_T *) ov2655_awb_tab[mode];

	if (mode >= DCAMERA_WB_MODE_MAX)
		return SENSOR_OP_PARAM_ERR;

	for (i = 0;
	     (0xFFFF != sensor_reg_ptr[i].reg_addr)
	     || (0xFF != sensor_reg_ptr[i].reg_value); i++) {
		Sensor_WriteReg(sensor_reg_ptr[i].reg_addr,
				sensor_reg_ptr[i].reg_value);
	}
	SENSOR_Sleep(10);
	SENSOR_TRACE("SENSOR: set_awb_mode: mode = %d", mode);
	return 0;
}

/******************************************************************************/
// Description:
// Global resource dependence:
// Author:yangbin
// Note:
//
/******************************************************************************/
const SENSOR_REG_T ov2655_ev_tab[][4] = {
	{{0x3047, 0x05}, {0x3018, 0x40}, {0x3019, 0x30}, {0x301a, 0x71}},
	{{0x3047, 0x05}, {0x3018, 0x5a}, {0x3019, 0x4a}, {0x301a, 0xc2}},
	{{0x3047, 0x05}, {0x3018, 0x6a}, {0x3019, 0x5a}, {0x301a, 0xd4}},
	{{0x3047, 0x05}, {0x3018, 0x78}, {0x3019, 0x68}, {0x301a, 0xd4}},
	{{0x3047, 0x05}, {0x3018, 0x88}, {0x3019, 0x78}, {0x301a, 0xd5}},
	{{0x3047, 0x05}, {0x3018, 0xa8}, {0x3019, 0x98}, {0x301a, 0xe6}},
	{{0x3047, 0x05}, {0x3018, 0xc8}, {0x3019, 0xb8}, {0x301a, 0xf7}}	//yangbin 2009.09.09
};

LOCAL uint32_t set_ov2655_ev(uint32_t level)
{
	uint16_t i;
	SENSOR_REG_T *sensor_reg_ptr = (SENSOR_REG_T *) ov2655_ev_tab[level];
	if (level > 6)
		return 0;

	for (i = 0;
	     i <
	     4
	     /*(0xFF != sensor_reg_ptr[i].reg_addr) && (0xFF != sensor_reg_ptr[i].reg_value) */
	     ; i++) {
		Sensor_WriteReg(sensor_reg_ptr[i].reg_addr,
				sensor_reg_ptr[i].reg_value);
	}
	//OS_TickDelay(100);
	SENSOR_Sleep(10);

	SENSOR_TRACE("SENSOR: set_ev: level = %d", level);
	return 0;
}

/******************************************************************************/
// Description:
// Global resource dependence:
// Author:
// Note:
//
/******************************************************************************/
LOCAL uint32_t OV2655_set_hmirror_enable(uint32_t enable)
{
	uint16_t reg04;
	reg04 = Sensor_ReadReg(0x3090);
	if (enable) {
		reg04 = (reg04 | 0x08);
		Sensor_WriteReg(0x3090, reg04);
	} else {
		reg04 = (reg04 & (~(0x08)));
		Sensor_WriteReg(0x3090, reg04);
	}
	return 0;
}

/******************************************************************************/
// Description:
// Global resource dependence:
// Author:
// Note:
//
/******************************************************************************/
LOCAL uint32_t OV2655_set_vmirror_enable(uint32_t enable)
{
	uint16_t reg04;
	reg04 = Sensor_ReadReg(0x307C);

	if (enable) {
		reg04 = (reg04 | 0x01);
		Sensor_WriteReg(0x307C, reg04);
	} else {
		reg04 = (reg04 & (~(0x01)));
		Sensor_WriteReg(0x307C, reg04);
	}
	return 0;
}

/******************************************************************************/
// Description:
// Global resource dependence:
// Author:
// Note:
//
/******************************************************************************/
LOCAL uint32_t OV2655_set_preview_mode(uint32_t preview_mode)
{
	SENSOR_TRACE("set_preview_mode: preview_mode = %d", preview_mode);

	switch (preview_mode) {
	case DCAMERA_ENVIRONMENT_NORMAL:
		OV2655_set_work_mode(0);
		break;
	case DCAMERA_ENVIRONMENT_NIGHT:
		OV2655_set_work_mode(1);
		break;
	case DCAMERA_ENVIRONMENT_SUNNY:
		OV2655_set_work_mode(0);
		break;
	default:
		break;
	}
	SENSOR_Sleep(20);
	return 0;
}

/******************************************************************************/
// Description:
// Global resource dependence:
// Author:
// Note:
//
/******************************************************************************/
LOCAL uint32_t OV2655_Identify(uint32_t param)
{
	uint16_t iden_reg_val = 0;
	uint32_t ret = SENSOR_OP_ERR;

	iden_reg_val = Sensor_ReadReg(0x300A);
	if (iden_reg_val == 0x26) {
		iden_reg_val = Sensor_ReadReg(0x300B);
		if (iden_reg_val == 0x56) {
			ret = SENSOR_OP_SUCCESS;
		}
	}

	OV2655_InitExifInfo();
	if (SENSOR_OP_SUCCESS == ret) {
		SENSOR_TRACE("It Is OV2655 Sensor !");
	} else {
		SENSOR_TRACE("It Is not OV2655 Sensor !");
	}
	return ret;
}

/******************************************************************************/
// Description:
// Global resource dependence:
// Author:
// Note:
//
/******************************************************************************/
#define CAPTURE_MAX_GAIN16 			128
#define OV2650_CAPTURE_MAXIMUM_SHUTTER		1200
#define PV_PERIOD_PIXEL_NUMS 1190
#define  FULL_PERIOD_PIXEL_NUMS 1922
#define  g_Capture_Dummy_Pixels 0
#define  FULL_EXPOSURE_LIMITATION 1234
#define g_Capture_Dummy_Lines 0
#define g_Capture_PCLK_Frequency 36

uint16_t read_OV2650_gain(void)
{
	uint8_t temp_reg;
	uint16_t sensor_gain;

	temp_reg = Sensor_ReadReg(0x3000);
	sensor_gain = (16 + (temp_reg & 0x0F));
	if (temp_reg & 0x10)
		sensor_gain <<= 1;
	if (temp_reg & 0x20)
		sensor_gain <<= 1;
	if (temp_reg & 0x40)
		sensor_gain <<= 1;
	if (temp_reg & 0x80)
		sensor_gain <<= 1;
	return sensor_gain;
}				/* read_OV2650_gain */

LOCAL void OV2655_CalculateExposureGain(SENSOR_MODE_E sensor_preview_mode,
					SENSOR_MODE_E sensor_snapshot_mode)
{
	//uint16_t PV_Line_Width;
	//uint16_t Capture_Line_Width;
	//uint16_t Capture_Maximum_Shutter;
	//uint16_t Capture_Exposure;
	//uint16_t Capture_Gain16;
	//uint16_t Capture_Banding_Filter;
	//uint32_t Gain_Exposure=0;
	uint8_t Reg0x3000, Reg0x3002, Reg0x3003;
	uint32_t Shutter;
	uint16_t tmp1, tmp2;
	// uint32_t             Gain_Mul = 0;
	//uint16_t g_PV_Gain16 = 0;
	//uint16_t      g_PV_Shutter = 0;
	//uint16_t g_PV_Extra_Lines = 0;
	//uint16_t      g_PV_Dummy_Pixels = 0;
	//uint16_t      g_Capture_Max_Gain16 = 8*16;
	//uint16_t g_Capture_Gain16 = 0;
	uint16_t g_Capture_Extra_Lines = 0;
	uint16_t g_Capture_Shutter = 0;

	if (sensor_snapshot_mode < SENSOR_MODE_SNAPSHOT_ONE_FIRST) {	//less than 640X480
		SENSOR_Sleep(50);
		return;
	}
	Sensor_WriteReg(0x3013, 0xf0);
	//Stop Preview
	//Stop AE/AG
	//Read back preview shutter
	Reg0x3002 = Sensor_ReadReg(0x3002);
	Reg0x3003 = Sensor_ReadReg(0x3003);
	SENSOR_TRACE("Reg0x3002=0x%x", Reg0x3002);
	SENSOR_TRACE("Reg0x3003=0x%x", Reg0x3003);
	tmp1 = (uint16_t) Reg0x3002;
	tmp1 <<= 8;
	tmp2 = (uint16_t) Reg0x3003;
	Shutter = (uint16_t) (tmp1 | tmp2);
	Reg0x3000 = Sensor_ReadReg(0x3000);
	Sensor_SetMode(sensor_snapshot_mode);
	if (0 == work_mode) {
		Sensor_WriteReg(0x3011, 0x01);	//match with the colck change with night mode
	} else {
		Sensor_WriteReg(0x3011, 0x03);	//match with the colck change with night mode
	}
	if (Reg0x3000 & 0x40) {
		Reg0x3000 = Reg0x3000 & 0x3f;
		Shutter = Shutter * 2;
	} else if (Reg0x3000 & 0x20) {	//4//4x~8x gain
		Reg0x3000 = Reg0x3000 & 0x1f;
		Shutter = Shutter * 2;
	} else if (Reg0x3000 & 0x10)	//4  //2x~4x gain
	{
		Reg0x3000 = Reg0x3000 & 0x0f;
		Shutter = Shutter * 2;
	}
	if (Shutter > 1234) {
		g_Capture_Extra_Lines = Shutter - 1234;
		Shutter = 1234;
	}
	if (0 == Shutter) {
		Shutter = 1;
	}
	g_Capture_Shutter = Shutter;
	Sensor_WriteReg(0x3003, g_Capture_Shutter & 0xff);
	Sensor_WriteReg(0x3002, (g_Capture_Shutter >> 8) & 0xff);
	Sensor_WriteReg(0x302e, g_Capture_Extra_Lines & 0xff);
	Sensor_WriteReg(0x302d, (g_Capture_Extra_Lines >> 8) & 0xff);
	Sensor_WriteReg(0x3000, Reg0x3000);
	//   Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, (uint32_t)exposal);

	if (0 == work_mode) {
		SENSOR_Sleep(200);
	} else {
		SENSOR_Sleep(400);
	}

}

LOCAL uint32_t OV2655_BeforeSnapshot(uint32_t param)
{
	uint32_t preview_mode;
	uint32_t cap_mode = (param>>CAP_MODE_BITS);

	param = param&0xffff;
	SENSOR_PRINT("%d,%d.",cap_mode,param);

	preview_mode = (param >= SENSOR_MODE_PREVIEW_TWO) ?
	    SENSOR_MODE_PREVIEW_TWO : SENSOR_MODE_PREVIEW_ONE;
	OV2655_CalculateExposureGain(preview_mode, param);
	return 0;
}

/******************************************************************************/
// Description:
// Global resource dependence:
// Author:
// Note:
//
/******************************************************************************/
const SENSOR_REG_T ov2655_sharpness_tab[][2] = {
	//weakest
	{{0x3306, 0x08}, {0x3371, 0x00}},
	{{0x3306, 0x08}, {0x3371, 0x01}},
	{{0x3306, 0x08}, {0x3371, 0x02}},
	{{0x3306, 0x08}, {0x3371, 0x03}},
	//strongest
	{{0x3306, 0x08}, {0x3371, 0x04}}
};

/*LOCAL uint32_t OV2655_set_sharpness(uint32_t level)
{
#if	0
	uint16_t i;
	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)ov2655_sharpness_tab[level];

	SCI_ASSERT(level <= 5 );
	SCI_ASSERT(PNULL != sensor_reg_ptr);

	for(i = 0; i < 2 ; i++)//(0xFF != sensor_reg_ptr[i].reg_addr) && (0xFF != sensor_reg_ptr[i].reg_value)
	{
		Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}
	SENSOR_TRACE("set_sharpness: level = %d", level);
#endif
	return 0;
}*/

/******************************************************************************/
// Description:
// Global resource dependence:
// Author:
// Note:
//
/******************************************************************************/
const SENSOR_REG_T ov2655_saturation_tab[][4] = {
	// level 0
	{
	 {0x3301, 0xff}, {0x3391, 0x02}, {0x3394, 0x70}, {0x3395, 0x70}
	 },
	//level 1
	{
	 {0x3301, 0xff}, {0x3391, 0x02}, {0x3394, 0x50}, {0x3395, 0x50}

	 },
	//level 2
	{
	 {0x3301, 0xff}, {0x3391, 0x02}, {0x3394, 0x40}, {0x3395, 0x40}

	 },
	//level 3
	{
	 {0x3301, 0xff}, {0x3391, 0x02}, {0x3394, 0x30}, {0x3395, 0x30}

	 },
	//level 4
	{
	 {0x3301, 0xff}, {0x3391, 0x02}, {0x3394, 0x20}, {0x3395, 0x20}

	 }
};

/*
LOCAL uint32_t OV2655_set_saturation(uint32_t level)
{
#if	0
	uint16_t i;
	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)ov2655_saturation_tab[level];

	SCI_ASSERT(level <= 8 );
	SCI_ASSERT(PNULL != sensor_reg_ptr);

	for(i = 0; i < 4 ; i++)//(0xFF != sensor_reg_ptr[i].reg_addr) && (0xFF != sensor_reg_ptr[i].reg_value)
	{
		Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}
	SENSOR_TRACE("set_saturation: level = %d", level);
#endif
	return 0;
}
*/
/******************************************************************************/
// Description:
// Global resource dependence:
// Author:
// Note:
//
/******************************************************************************/
const SENSOR_REG_T ov2655_image_effect_tab[][4] = {
	// effect normal
	{
	 {0x3391, 0x00}, {0xffff, 0xff}, {0xffff, 0xff}, {0xffff, 0xff}

	 },
	//effect BLACKWHITE
	{
	 {0x3391, 0x20}, {0xffff, 0xff}, {0xffff, 0xff}, {0xffff, 0xff}

	 },
	// effect RED
	{
	 {0x3391, 0x18}, {0x3396, 0x80}, {0x3397, 0xc0}, {0xffff, 0xff}

	 },

	// effect GREEN
	{
	 {0x3391, 0x18}, {0x3396, 0x60}, {0x3397, 0x60}, {0xffff, 0xff}

	 },

	// effect  BLUE
	{
	 {0x3391, 0x18}, {0x3396, 0xa0}, {0x3397, 0x40}, {0xffff, 0xff}

	 },
	//effect YELLOW
	{
	 {0x3391, 0x18}, {0x3396, 0x30}, {0x3397, 0x90}, {0xffff, 0xff}

	 },
	// effect NEGATIVE
	{
	 {0x3391, 0x40}, {0xffff, 0xff}, {0xffff, 0xff}, {0xffff, 0xff}

	 },
	// effect CANVAS ANTIQUE
	{
	 {0x3391, 0x18}, {0x3396, 0x40}, {0x3397, 0xa6}, {0xffff, 0xff}

	 }
};

LOCAL uint32_t OV2655_set_image_effect(uint32_t effect_type)
{
	uint16_t i, reg_value = 0;
	SENSOR_REG_T *sensor_reg_ptr =
	    (SENSOR_REG_T *) ov2655_image_effect_tab[effect_type];

	if (effect_type >= DCAMERA_EFFECT_MAX)
		return SENSOR_OP_PARAM_ERR;

	s_image_effect = effect_type;

	for (i = 0;
	     (0xFFFF != sensor_reg_ptr[i].reg_addr)
	     || (0xFF != sensor_reg_ptr[i].reg_value); i++) {
		if (sensor_reg_ptr[i].reg_addr == 0x3391) {
			reg_value =
			    Sensor_ReadReg(sensor_reg_ptr[i].reg_addr) & 0x87;
			reg_value |= (sensor_reg_ptr[i].reg_value & 0xF8);
			Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, reg_value);
		} else {
			Sensor_WriteReg(sensor_reg_ptr[i].reg_addr,
					sensor_reg_ptr[i].reg_value);
		}
	}
	SENSOR_TRACE("set_image_effect: effect_type = %d", effect_type);
	return 0;
}

/******************************************************************************/
// Description: set brightness
// Global resource dependence:
// Author:
// Note:
//              level  must smaller than 8
/******************************************************************************/
const SENSOR_REG_T ov2655_brightness_tab[][5] = {
	{			//level 1
	 {0x3301, 0xff}, {0x3391, 0x04}, {0x3390, 0x49}, {0x339a, 0x30},
	 {0xffff, 0xff}
	 },
	{			//level 2
	 {0x3301, 0xff}, {0x3391, 0x04}, {0x3390, 0x49}, {0x339a, 0x20},
	 {0xffff, 0xff}
	 },
	{			//level 3
	 {0x3301, 0xff}, {0x3391, 0x04}, {0x3390, 0x49}, {0x339a, 0x10},
	 {0xffff, 0xff}
	 },
	{			//level 4
	 {0x3301, 0xff}, {0x3391, 0x04}, {0x3390, 0x41}, {0x339a, 0x00},
	 {0xffff, 0xff}
	 },
	{			//level 5
	 {0x3301, 0xff}, {0x3391, 0x04}, {0x3390, 0x41}, {0x339a, 0x10},
	 {0xffff, 0xff}
	 },
	{			//level 6
	 {0x3301, 0xff}, {0x3391, 0x04}, {0x3390, 0x41}, {0x339a, 0x20},
	 {0xffff, 0xff}
	 },
	{			//level 7
	 {0x3301, 0xff}, {0x3391, 0x04}, {0x3390, 0x41}, {0x339a, 0x30},
	 {0xffff, 0xff}
	 }
};

LOCAL uint32_t OV2655_set_brightness(uint32_t level)
{
	uint16_t i, reg_value = 0;
	SENSOR_REG_T *sensor_reg_ptr;

	sensor_reg_ptr = (SENSOR_REG_T *) ov2655_brightness_tab[level];

	if (level >= 7)
		return SENSOR_OP_PARAM_ERR;

	for (i = 0;
	     (0xFFFF != sensor_reg_ptr[i].reg_addr)
	     || (0xFF != sensor_reg_ptr[i].reg_value); i++) {
		if (sensor_reg_ptr[i].reg_addr == 0x3391) {
			reg_value =
			    ov2655_image_effect_tab[s_image_effect][0].
			    reg_value;
			reg_value |= sensor_reg_ptr[i].reg_value;
			Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, reg_value);
			SENSOR_TRACE("set_brightness: reg3391 = %d", reg_value);
		} else if (sensor_reg_ptr[i].reg_addr == 0x3390) {
			reg_value = Sensor_ReadReg(sensor_reg_ptr[i].reg_addr);
			if (level < 3) {
				reg_value = reg_value | S_BIT_3;
			} else {
				reg_value = reg_value & (~S_BIT_3);
			}
			Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, reg_value);
		} else {
			Sensor_WriteReg(sensor_reg_ptr[i].reg_addr,
					sensor_reg_ptr[i].reg_value);
		}
	}
	SENSOR_Sleep(10);
	SENSOR_TRACE("set_brightness: level = %d", level);
	return 0;
}

/******************************************************************************/
// Description: set contrast
// Global resource dependence:
// Author:
// Note:
//              level must smaller than 9
/******************************************************************************/
const SENSOR_REG_T ov2655_contrast_tab[][6] = {
	{			//level 1
	 {0x3301, 0xff}, {0x3391, 0x04}, {0x3390, 0x45}, {0x3398, 0x14},
	 {0x3399, 0x14}, {0xffff, 0xff}
	 },
	{			//level 2
	 {0x3301, 0xff}, {0x3391, 0x04}, {0x3390, 0x45}, {0x3398, 0x18},
	 {0x3399, 0x18}, {0xffff, 0xff}
	 },
	{			//level 3
	 {0x3301, 0xff}, {0x3391, 0x04}, {0x3390, 0x45}, {0x3398, 0x1c},
	 {0x3399, 0x1c}, {0xffff, 0xff}
	 },
	{			//level 4
	 {0x3301, 0xff}, {0x3391, 0x04}, {0x3390, 0x45}, {0x3398, 0x20},
	 {0x3399, 0x20}, {0xffff, 0xff}
	 },
	{			//level 5
	 {0x3301, 0xff}, {0x3391, 0x04}, {0x3390, 0x45}, {0x3398, 0x24},
	 {0x3399, 0x24}, {0xffff, 0xff}
	 },
	{			//level 6
	 {0x3301, 0xff}, {0x3391, 0x04}, {0x3390, 0x45}, {0x3398, 0x28},
	 {0x3399, 0x28}, {0xffff, 0xff}
	 },
	{			//level 7
	 {0x3301, 0xff}, {0x3391, 0x04}, {0x3390, 0x45}, {0x3398, 0x2c},
	 {0x3399, 0x2c}, {0xffff, 0xff}
	 }
};

LOCAL uint32_t OV2655_set_contrast(uint32_t level)
{
	uint16_t i, reg_value = 0;
	SENSOR_REG_T *sensor_reg_ptr;
	sensor_reg_ptr = (SENSOR_REG_T *) ov2655_contrast_tab[level];

	if (level >= 7)
		return SENSOR_OP_PARAM_ERR;

	for (i = 0;
	     (0xFFFF != sensor_reg_ptr[i].reg_addr)
	     || (0xFF != sensor_reg_ptr[i].reg_value); i++) {
		if (sensor_reg_ptr[i].reg_addr == 0x3391) {
			reg_value =
			    ov2655_image_effect_tab[s_image_effect][0].
			    reg_value;
			reg_value |= sensor_reg_ptr[i].reg_value;
			Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, reg_value);
			SENSOR_TRACE("set_contrast: reg3391 = %d", reg_value);
		} else if (sensor_reg_ptr[i].reg_addr == 0x3390) {
			reg_value = Sensor_ReadReg(sensor_reg_ptr[i].reg_addr);
			reg_value = reg_value | S_BIT_2;
			Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, reg_value);
		} else {
			Sensor_WriteReg(sensor_reg_ptr[i].reg_addr,
					sensor_reg_ptr[i].reg_value);
		}
	}
	//Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_CONTRAST, (uint32_t)level);
	SENSOR_Sleep(5);
	SENSOR_TRACE("set_contrast: level = %d", level);
	return 0;
}

/******************************************************************************/
// Description:
// Global resource dependence:
// Author:
// Note:
//              mode 0:normal;   1:night
/******************************************************************************/
const SENSOR_REG_T ov2655_work_mode_tab[][3] = {
#if 0
	{			//normal fix 25fps
	 {0x300e, 0x34},	//thomas 0x38   15fps
	 {0x3011, 0x01},	//thomas 0x00    15fps
	 {0x302c, 0x00},
	 {0x3071, 0x00},
	 {0x3070, 0x5d},
	 {0x301c, 0x05},
	 {0x3073, 0x00},
	 {0x3072, 0x5d},
	 {0x301d, 0x05},
	 {0x3015, 0x01},
	 {0x3014, 0x84},
	 {0xffff, 0xff}

	 },
	{			//night 7.5fps-15fps
	 {0x300e, 0x34},	//thomas 0x38   15fps
	 {0x3011, 0x02},	//thomas 0x00    15fps
	 {0x302c, 0x00},
	 {0x3071, 0x00},
	 {0x3070, 0x5d},
	 {0x301c, 0x05},
	 {0x3073, 0x00},
	 {0x3072, 0x5d},
	 {0x301d, 0x05},
	 {0x3015, 0x01},
	 {0x3014, 0x84},
	 {0xffff, 0xff}
	 }
#else
	{			//normal fix 15fps
	 {0x3014, 0x84},
	 {0x3015, 0x02},
	 {0xffff, 0xff}
	 },
	{			//night 7.5fps-15fps
	 {0x3014, 0x8c},
	 {0x3015, 0x22},
	 {0xffff, 0xff}
	 }
#endif
};

/******************************************************************************/
// Description:
// Global resource dependence:
// Author:
// Note:
//              mode 0:normal;   1:night
/******************************************************************************/
LOCAL uint32_t OV2655_set_work_mode(uint32_t mode)
{
	uint16_t i;
	SENSOR_REG_T *sensor_reg_ptr =
	    (SENSOR_REG_T *) ov2655_work_mode_tab[mode];

	if (mode > 1)
		return SENSOR_OP_PARAM_ERR;
	work_mode = mode;

	for (i = 0;
	     (0xFFFF != sensor_reg_ptr[i].reg_addr)
	     || (0xFF != sensor_reg_ptr[i].reg_value); i++) {
		Sensor_WriteReg(sensor_reg_ptr[i].reg_addr,
				sensor_reg_ptr[i].reg_value);
	}
	// Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_SCENECAPTURETYPE, (uint32_t)mode);
	SENSOR_TRACE("set_work_mode: mode = %d", mode);
	return 0;
}

LOCAL uint32_t OV2655_after_snapshot(uint32_t param)
{
	//Sensor_SendRegTabToSensor(&s_OV3640_resolution_Tab_YUV[0]);
	//OV2655_chang_image_format(SENSOR_IMAGE_FORMAT_YUV422);
	return Sensor_SetMode(param);
}

LOCAL uint32_t OV2655_chang_image_format(uint32_t param)
{
	SENSOR_REG_TAB_INFO_T st_yuv422_reg_table_info = { ADDR_AND_LEN_OF_ARRAY(ov2655_YUV_640X480), 0, 0, 0, 0 };	//ov2655_YUV_COMMON
	uint32_t ret_val = SENSOR_FAIL;

	switch (param) {
	case SENSOR_IMAGE_FORMAT_YUV422:
		ret_val = Sensor_SendRegTabToSensor(&st_yuv422_reg_table_info);
		break;
	case SENSOR_IMAGE_FORMAT_JPEG:
		ret_val = SENSOR_FAIL;	//Sensor_SendRegTabToSensor(&st_jpeg_reg_table_info);
		break;
	default:
		break;
	}
	return ret_val;
}

LOCAL uint32_t OV2655_check_image_format_support(uint32_t param)
{
	uint32_t ret_val = SENSOR_FAIL;

	switch (param) {
	case SENSOR_IMAGE_FORMAT_YUV422:
		ret_val = SENSOR_SUCCESS;
		break;
	case SENSOR_IMAGE_FORMAT_JPEG:
		ret_val = SENSOR_FAIL;	//SCI_SUCCESS;
		break;
	default:
		break;
	}
	return ret_val;
}

LOCAL uint32_t OV2655_InitExifInfo(void)
{
#if 0
	EXIF_SPEC_PIC_TAKING_COND_T *exif_ptr = &s_ov2655_exif;

	SENSOR_TRACE("SENSOR: OV2655_InitExifInfo");

	exif_ptr->valid.FNumber = 1;
	exif_ptr->FNumber.numerator = 14;
	exif_ptr->FNumber.denominator = 5;

	exif_ptr->valid.ExposureProgram = 1;
	exif_ptr->ExposureProgram = 0x04;

	//exif_ptr->SpectralSensitivity[MAX_ASCII_STR_SIZE];
	//exif_ptr->ISOSpeedRatings;
	//exif_ptr->OECF;

	//exif_ptr->ShutterSpeedValue;

	exif_ptr->valid.ApertureValue = 1;
	exif_ptr->ApertureValue.numerator = 14;
	exif_ptr->ApertureValue.denominator = 5;

	//exif_ptr->BrightnessValue;
	//exif_ptr->ExposureBiasValue;

	exif_ptr->valid.MaxApertureValue = 1;
	exif_ptr->MaxApertureValue.numerator = 14;
	exif_ptr->MaxApertureValue.denominator = 5;

	//exif_ptr->SubjectDistance;
	//exif_ptr->MeteringMode;
	//exif_ptr->LightSource;
	//exif_ptr->Flash;

	exif_ptr->valid.FocalLength = 1;
	exif_ptr->FocalLength.numerator = 289;
	exif_ptr->FocalLength.denominator = 100;

	//exif_ptr->SubjectArea;
	//exif_ptr->FlashEnergy;
	//exif_ptr->SpatialFrequencyResponse;
	//exif_ptr->FocalPlaneXResolution;
	//exif_ptr->FocalPlaneYResolution;
	//exif_ptr->FocalPlaneResolutionUnit;
	//exif_ptr->SubjectLocation[2];
	//exif_ptr->ExposureIndex;
	//exif_ptr->SensingMethod;

	exif_ptr->valid.FileSource = 1;
	exif_ptr->FileSource = 0x03;

	//exif_ptr->SceneType;
	//exif_ptr->CFAPattern;
	//exif_ptr->CustomRendered;

	exif_ptr->valid.ExposureMode = 1;
	exif_ptr->ExposureMode = 0x00;

	exif_ptr->valid.WhiteBalance = 1;
	exif_ptr->WhiteBalance = 0x00;

	//exif_ptr->DigitalZoomRatio;
	//exif_ptr->FocalLengthIn35mmFilm;
	//exif_ptr->SceneCaptureType;
	//exif_ptr->GainControl;
	//exif_ptr->Contrast;
	//exif_ptr->Saturation;
	//exif_ptr->Sharpness;
	//exif_ptr->DeviceSettingDescription;
	//exif_ptr->SubjectDistanceRange;
#endif
	return SENSOR_SUCCESS;
}

LOCAL uint32_t OV2655_GetExifInfo(uint32_t param)
{
	SENSOR_TRACE("###########*****SENSOR: OV2655_GetExifInfo*****");
	return 0;
	//return (uint32_t)&s_ov2655_exif;
}

#if 0
struct sensor_drv_cfg sensor_ov2655 = {
	.sensor_pos = CONFIG_DCAM_SENSOR_POS_OV2655,
	.sensor_name = "ov2655",
	.driver_info = &g_OV2655_yuv_info,
};

static int __init sensor_ov2655_init(void)
{
	return dcam_register_sensor_drv(&sensor_ov2655);
}

subsys_initcall(sensor_ov2655_init);
#endif
