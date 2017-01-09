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

#include "sensor_s5k5e3yx_raw_param_v3.c"

#define S5K5E3YX_I2C_ADDR_W        0x10
#define S5K5E3YX_I2C_ADDR_R        0x10
#define S5K5E3YX_RAW_PARAM_COM     0x0000

static uint32_t g_module_id = 0;
static struct sensor_raw_info* s_s5k5e3yx_mipi_raw_info_ptr = NULL;

static unsigned long _s5k5e3yx_GetResolutionTrimTab(unsigned long param);
static unsigned long _s5k5e3yx_Identify(unsigned long param);
static uint32_t _s5k5e3yx_GetRawInof(void);
static unsigned long _s5k5e3yx_StreamOn(unsigned long param);
static unsigned long _s5k5e3yx_StreamOff(unsigned long param);
static uint32_t _s5k5e3yx_com_Identify_otp(void* param_ptr);

static const struct raw_param_info_tab s_s5k5e3yx_raw_param_tab[] = {
	{S5K5E3YX_RAW_PARAM_COM, &s_s5k5e3yx_mipi_raw_info, _s5k5e3yx_com_Identify_otp, NULL},
	{RAW_INFO_END_ID, PNULL, PNULL, PNULL}
};

static const SENSOR_REG_T s5k5e3yx_common_init[] = {

};

static const SENSOR_REG_T s5k5e3yx_2592x1944_2lane_setting[] = {
	
};

static const SENSOR_REG_T s5k5e3yx_2576x1932_2lane_setting[] = {
	{0x0305,	0x06},
	{0x0306,	0x00},
	{0x0307,	0xE0},
	{0x3C1F,	0x00},
	{0x0820,	0x03},
	{0x0821,	0x80},
	{0x3C1C,	0x58},
	{0x0114,	0x01},
	{0x0340,	0x07},
	{0x0341,	0xE9},
	{0x0342,	0x0B},
	{0x0343,	0x86},
	{0x0344,	0x00},
	{0x0345,	0x00},
	{0x0346,	0x00},
	{0x0347,	0x02},
	{0x0348,	0x0A},
	{0x0349,	0x0F},
	{0x034A,	0x07},
	{0x034B,	0x8D},
	{0x034C,	0x0A},
	{0x034D,	0x10},
	{0x034E,	0x07},
	{0x034F,	0x8C},
	{0x0900,	0x00},
	{0x0901,	0x00},
	{0x0383,	0x01},
	{0x0387,	0x01},
	{0x0204,	0x00},
	{0x0205,	0x20},
	{0x0202,	0x02},
	{0x0203,	0x00},
	{0x0200,	0x04},
	{0x0201,	0x98},

	{0x0100,	0x00},
	{0x3000,	0x04},
	{0x3002,	0x03},
	{0x3003,	0x04},
	{0x3004,	0x05},
	{0x3005,	0x00},
	{0x3006,	0x10},
	{0x3007,	0x0A},
	{0x3008,	0x55},
	{0x3039,	0x00},
	{0x303A,	0x00},
	{0x303B,	0x00},
	{0x3009,	0x05},
	{0x300A,	0x55},
	{0x300B,	0x38},
	{0x300C,	0x10},
	{0x3012,	0x14},
	{0x3013,	0x00},
	{0x3014,	0x22},
	{0x300E,	0x79},
	{0x3010,	0x68},
	{0x3019,	0x03},
	{0x301A,	0x00},
	{0x301B,	0x06},
	{0x301C,	0x00},
	{0x301D,	0x22},
	{0x301E,	0x00},
	{0x301F,	0x10},
	{0x3020,	0x00},
	{0x3021,	0x00},
	{0x3022,	0x0A},
	{0x3023,	0x1E},
	{0x3024,	0x00},
	{0x3025,	0x00},
	{0x3026,	0x00},
	{0x3027,	0x00},
	{0x3028,	0x1A},
	{0x3015,	0x00},
	{0x3016,	0x84},
	{0x3017,	0x00},
	{0x3018,	0xA0},
	{0x302B,	0x10},
	{0x302C,	0x0A},
	{0x302D,	0x06},
	{0x302E,	0x05},
	{0x302F,	0x0E},
	{0x3030,	0x2F},
	{0x3031,	0x08},
	{0x3032,	0x05},
	{0x3033,	0x09},
	{0x3034,	0x05},
	{0x3035,	0x00},
	{0x3036,	0x00},
	{0x3037,	0x00},
	{0x3038,	0x00},
	{0x3088,	0x06},
	{0x308A,	0x08},
	{0x308C,	0x05},
	{0x308E,	0x07},
	{0x3090,	0x06},
	{0x3092,	0x08},
	{0x3094,	0x05},
	{0x3096,	0x21},
	{0x3055,	0x9E},
	{0x3099,	0x06},
	{0x3070,	0x10},
	{0x3085,	0x31},
	{0x3086,	0x01},
	{0x3064,	0x00},
	{0x3062,	0x08},
	{0x3061,	0x15},
	{0x307B,	0x20},
	{0x3068,	0x01},
	{0x3074,	0x00},
	{0x307D,	0x05},
	{0x3045,	0x01},
	{0x3046,	0x05},
	{0x3047,	0x78},
	{0x307F,	0xB1},
	{0x3098,	0x01},
	{0x305C,	0xF6},
	{0x3063,	0x2F},
	{0x3400,	0x01},
	{0x3235,	0x49},
	{0x3233,	0x00},
	{0x3234,	0x00},
	{0x3300,	0x0C},
	{0x3320,	0x02},
	{0x3203,	0x45},
	{0x3205,	0x4D},
	{0x320B,	0x40},
	{0x320C,	0x06},
	{0x320D,	0xC0},
	{0x3244,	0x00},
	{0x3245,	0x00},
	{0x3246,	0x01},
	{0x3247,	0x00},
	{0x3268,	0x88},
	{0x3269,	0x01},
};

static SENSOR_REG_TAB_INFO_T s_s5k5e3yx_resolution_Tab_RAW[] = {
	//{ADDR_AND_LEN_OF_ARRAY(s5k5e3yx_common_init), 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},
	{PNULL, 0, 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},
	//{ADDR_AND_LEN_OF_ARRAY(s5k5e3yx_2592x1944_2lane_setting), 2592, 1944, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(s5k5e3yx_2576x1932_2lane_setting), 2576, 1932, 24, SENSOR_IMAGE_FORMAT_RAW},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
};

static SENSOR_TRIM_T s_s5k5e3yx_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	//{0, 0, 2592, 1944, 267, 750, 1248, {0, 0, 1632, 1224}},
	{0, 0, 2576, 1932, 267, 750, 2480, {0, 0, 2576, 1932}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}}
};

static const SENSOR_REG_T s_s5k5e3yx_2576x1932_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

static SENSOR_VIDEO_INFO_T s_s5k5e3yx_video_info[] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{30, 30, 219, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_s5k5e3yx_2576x1932_video_tab},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}
};

static SENSOR_IOCTL_FUNC_TAB_T s_s5k5e3yx_ioctl_func_tab = {
	PNULL,
	PNULL,//_s5k5e3yx_PowerOn,
	PNULL,
	_s5k5e3yx_Identify,

	PNULL,// write register
	PNULL,// read  register
	PNULL,
	_s5k5e3yx_GetResolutionTrimTab,

	// External
	PNULL,
	PNULL,
	PNULL,

	PNULL,//_s5k5e3yx_set_brightness,
	PNULL,// _s5k5e3yx_set_contrast,
	PNULL,
	PNULL,//_s5k5e3yx_set_saturation,

	PNULL,//_s5k5e3yx_set_work_mode,
	PNULL,//_s5k5e3yx_set_image_effect,

	PNULL,//_s5k5e3yx_BeforeSnapshot,
	PNULL,//_s5k5e3yx_after_snapshot,
	PNULL,//_s5k5e3yx_flash,
	PNULL,
	PNULL,//_s5k5e3yx_write_exposure,
	PNULL,
	PNULL,//_s5k5e3yx_write_gain,
	PNULL,
	PNULL,
	PNULL,//_s5k5e3yx_write_af,
	PNULL,
	PNULL,//_s5k5e3yx_set_awb,
	PNULL,
	PNULL,
	PNULL,//_s5k5e3yx_set_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL,//_s5k5e3yx_GetExifInfo,
	PNULL,//_s5k5e3yx_ExtFunc,
	PNULL,//_s5k5e3yx_set_anti_flicker,
	PNULL,//_s5k5e3yx_set_video_mode,
	PNULL,//pick_jpeg_stream
	PNULL,//meter_mode
	PNULL,//get_status
	_s5k5e3yx_StreamOn,
	_s5k5e3yx_StreamOff,
	PNULL,//_s5k5e3yx_access_val,
};


SENSOR_INFO_T g_s5k5e3yx_mipi_raw_info = {
	S5K5E3YX_I2C_ADDR_W,	// salve i2c write address
	S5K5E3YX_I2C_ADDR_R,	// salve i2c read address

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
	{{0x0, 0x5e},		// supply two code to identify sensor.
	 {0x1, 0x20}},		// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,	// voltage of avdd

	2576,			// max width of source image
	1932,			// max height of source image
	"s5k5e3yx",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_GR,//SENSOR_IMAGE_PATTERN_RAWRGB_R,// pattern of input image form sensor;

	s_s5k5e3yx_resolution_Tab_RAW,	// point to resolution table information structure
	&s_s5k5e3yx_ioctl_func_tab,	// point to ioctl function table
	&s_s5k5e3yx_mipi_raw_info_ptr,		// information and table about Rawrgb sensor
	NULL,			//&g_s5k5e3yx_ext_info,                // extend information about sensor
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
	{SENSOR_INTERFACE_TYPE_CSI2, 2, 10, 0},
	s_s5k5e3yx_video_info,
	3,			// skip frame num while change setting
	48,			// horizontal view angle
	48,			// vertical view angle
};

static struct sensor_raw_info* Sensor_GetContext(void)
{
	return s_s5k5e3yx_mipi_raw_info_ptr;
}

static uint32_t Sensor_s5k5e3yx_InitRawTuneInfo(void)
{
	uint32_t rtn=0x00;

	return rtn;
}

static unsigned long _s5k5e3yx_GetResolutionTrimTab(unsigned long param)
{
	SENSOR_PRINT("0x%lx",  (unsigned long)s_s5k5e3yx_Resolution_Trim_Tab);
	return (unsigned long) s_s5k5e3yx_Resolution_Trim_Tab;
}



static unsigned long _s5k5e3yx_Identify(unsigned long param)
{
#define S5K5E3YX_PID_VALUE    0x5E
#define S5K5E3YX_PID_ADDR     0x0000
#define S5K5E3YX_VER_VALUE    0x30
#define S5K5E3YX_VER_ADDR     0x0001
	uint8_t pid_value = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT_ERR("SENSOR_S5K5E3YX: mipi raw identify\n");

	pid_value = Sensor_ReadReg(S5K5E3YX_PID_ADDR);

	if (S5K5E3YX_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(S5K5E3YX_VER_ADDR);
		SENSOR_PRINT("SENSOR_S5K5E3YX: Identify: PID = %x, VER = %x", pid_value, ver_value);
		if (S5K5E3YX_VER_VALUE == ver_value) {
			SENSOR_PRINT_ERR("SENSOR_S5K5E3YX: this is S5K5E3YX sensor !");
			ret_value=_s5k5e3yx_GetRawInof();
			if (SENSOR_SUCCESS != ret_value) {
				SENSOR_PRINT_ERR("SENSOR_S5K5E3YX: the module is unknow error !");
			}
			Sensor_s5k5e3yx_InitRawTuneInfo();
		} else {
			SENSOR_PRINT_ERR("SENSOR_S5K5E3YX: Identify this is hm%x%x sensor !", pid_value, ver_value);
			return ret_value;
		}
	} else {
		SENSOR_PRINT_ERR("SENSOR_S5K5E3YX: identify fail,pid_value=%d", pid_value);
	}

	return ret_value;
}

static uint32_t _s5k5e3yx_GetRawInof(void)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_s5k5e3yx_raw_param_tab;
	uint32_t param_id;
	uint32_t i=0x00;

	/*read param id from sensor omap*/
	param_id=S5K5E3YX_RAW_PARAM_COM;

	for (i=0x00; ; i++) {
		g_module_id = i;
		if (RAW_INFO_END_ID==tab_ptr[i].param_id) {
			if (NULL==s_s5k5e3yx_mipi_raw_info_ptr) {
				SENSOR_PRINT("SENSOR_S5K4H5YC: ov5647_GetRawInof no param error");
				rtn=SENSOR_FAIL;
			}
			SENSOR_PRINT("SENSOR_S5K4H5YC: s5k5e3yx_GetRawInof end");
			break;
		}
		else if (PNULL!=tab_ptr[i].identify_otp) {
			if (SENSOR_SUCCESS==tab_ptr[i].identify_otp(0)) {
				s_s5k5e3yx_mipi_raw_info_ptr = tab_ptr[i].info_ptr;
				SENSOR_PRINT("SENSOR_S5K4H5YC: s5k5e3yx_GetRawInof success");
				break;
			}
		}
	}

	return rtn;
}

static unsigned long _s5k5e3yx_StreamOn(unsigned long param)
{
	SENSOR_PRINT_ERR("SENSOR_s5k5e3yx: StreamOn");

	Sensor_WriteReg(0x0100, 0x01);

	return 0;
}

static unsigned long _s5k5e3yx_StreamOff(unsigned long param)
{
	SENSOR_PRINT_ERR("SENSOR_s5k5e3yx: StreamOff");

	Sensor_WriteReg(0x0100, 0x00);
	usleep(10*1000);

	return 0;
}

static uint32_t _s5k5e3yx_com_Identify_otp(void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_s5k5e3yx: _s5k5e3yx_com_Identify_otp");

	/*read param id from sensor omap*/
	param_id=S5K5E3YX_RAW_PARAM_COM;

	if(S5K5E3YX_RAW_PARAM_COM==param_id){
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}
