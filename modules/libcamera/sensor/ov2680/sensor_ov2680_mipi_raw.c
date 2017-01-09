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
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
#include "newparam/sensor_ov2680_raw_param_main.c"
#else
#include "newparam/sensor_ov2680_raw_param_main.c"
#endif

#define ov2680_I2C_ADDR_W        (0x6c>>1)
#define ov2680_I2C_ADDR_R         (0x6c>>1)

#define ov2680_MIN_FRAME_LEN_PRV  0x484
#define ov2680_MIN_FRAME_LEN_CAP  0x7B6
#define ov2680_RAW_PARAM_COM  0x0000
static uint32_t g_module_id = 0;

static uint32_t g_flash_mode_en = 0;
static uint32_t s_ov2680_gain = 0;
static int s_capture_VTS = 0;
static int s_capture_shutter = 0;

#define ov2680_RAW_PARAM_Truly     0x02
#define ov2680_RAW_PARAM_Sunny    0x01

static uint16_t RG_Ratio_Typical = 0x17d;
static uint16_t BG_Ratio_Typical = 0x164;

/*Revision: R2.52*/
struct otp_struct {
	int module_integrator_id;
	int lens_id;
	int rg_ratio;
	int bg_ratio;
	int user_data[2];
	int light_rg;
	int light_bg;
};

LOCAL uint32_t update_otp(void* param_ptr);
LOCAL uint32_t _ov2680_Truly_Identify_otp(void* param_ptr);
LOCAL uint32_t _ov2680_Sunny_Identify_otp(void* param_ptr);
LOCAL unsigned long _ov2680_GetResolutionTrimTab(unsigned long param);
LOCAL unsigned long _ov2680_PowerOn(unsigned long power_on);
LOCAL unsigned long _ov2680_Identify(unsigned long param);
LOCAL unsigned long _ov2680_BeforeSnapshot(unsigned long param);
LOCAL unsigned long _ov2680_after_snapshot(unsigned long param);
LOCAL unsigned long _ov2680_StreamOn(unsigned long param);
LOCAL unsigned long _ov2680_StreamOff(unsigned long param);
LOCAL unsigned long _ov2680_write_exposure(unsigned long param);
LOCAL unsigned long _ov2680_write_gain(unsigned long param);
LOCAL unsigned long _ov2680_write_af(unsigned long param);
LOCAL unsigned long _ov2680_ReadGain(unsigned long param);
LOCAL unsigned long _ov2680_SetEV(unsigned long param);
LOCAL unsigned long _ov2680_ExtFunc(unsigned long ctl_param);
LOCAL unsigned long _dw9174_SRCInit(unsigned long mode);
LOCAL unsigned long _ov2680_flash(unsigned long param);
LOCAL uint32_t _ov2680_com_Identify_otp(void* param_ptr);
LOCAL unsigned long _ov2680_cfg_otp(unsigned long  param);
LOCAL unsigned long _ov2680_ex_write_exposure(unsigned long param);

LOCAL const struct raw_param_info_tab s_ov2680_raw_param_tab[]={
	//{ov2680_RAW_PARAM_Sunny, &s_ov2680_mipi_raw_info, _ov2680_Sunny_Identify_otp, update_otp},
	//{ov2680_RAW_PARAM_Truly, &s_ov2680_mipi_raw_info, _ov2680_Truly_Identify_otp, update_otp},
	{ov2680_RAW_PARAM_COM, &s_ov2680_mipi_raw_info, _ov2680_com_Identify_otp, PNULL},
	{RAW_INFO_END_ID, PNULL, PNULL, PNULL}
};

struct sensor_raw_info* s_ov2680_mipi_raw_info_ptr=NULL;

//800x600
LOCAL const SENSOR_REG_T ov2680_com_mipi_raw[] = {
	{0x0103, 0x01},
	{0xffff, 0xa0},
	{0x3002, 0x00},
	{0x3016, 0x1c},
	{0x3018, 0x44},
	{0x3020, 0x00},
	{0x3080, 0x02},
	{0x3082, 0x37},
	{0x3084, 0x09},
	{0x3085, 0x04},
	{0x3086, 0x01},
	{0x3501, 0x26},
	{0x3502, 0x40},
	{0x3503, 0x03},
	{0x350b, 0x36},
	{0x3600, 0xb4},
	{0x3603, 0x39},
	{0x3604, 0x24},
	{0x3605, 0x00}, //bit3:   1: use external regulator  0: use internal regulator
	{0x3620, 0x26},
	{0x3621, 0x37},
	{0x3622, 0x04},
	{0x3628, 0x00},
	{0x3705, 0x3c},
	{0x370c, 0x50},
	{0x370d, 0xc0},
	{0x3718, 0x88},
	{0x3720, 0x00},
	{0x3721, 0x00},
	{0x3722, 0x00},
	{0x3723, 0x00},
	{0x3738, 0x00},
	{0x370a, 0x23},
	{0x3717, 0x58},
	{0x3781, 0x80},
	{0x3784, 0x0c},
	{0x3789, 0x60},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x00},
	{0x3804, 0x06},
	{0x3805, 0x4f},
	{0x3806, 0x04},
	{0x3807, 0xbf},
	{0x3808, 0x03},
	{0x3809, 0x20},
	{0x380a, 0x02},
	{0x380b, 0x58},
	{0x380c, 0x06},
	{0x380d, 0xac},
	{0x380e, 0x02},
	{0x380f, 0x84},
	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3812, 0x00},
	{0x3813, 0x04},
	{0x3814, 0x31},
	{0x3815, 0x31},
	{0x3819, 0x04},
#ifdef CONFIG_CAMERA_IMAGE_180
	{0x3820, 0xc4},//c2
	{0x3821, 0x04},//00
#else
	{0x3820, 0xc6},
	{0x3821, 0x04},
#endif
	{0x4000, 0x81},
	{0x4001, 0x40},
	{0x4008, 0x00},
	{0x4009, 0x03},
	{0x4602, 0x02},
	{0x481b, 0x43},
	{0x481f, 0x36},
	{0x4825, 0x36},
	{0x4837, 0x30},
	{0x5002, 0x30},
	{0x5080, 0x00},
	{0x5081, 0x41},
	{0x0100, 0x00}

};


LOCAL const SENSOR_REG_T ov2680_640X480_mipi_raw[] = {
	{0x3086, 0x01},
	{0x3501, 0x26},
	{0x3502, 0x40},
	{0x3620, 0x26},
	{0x3621, 0x37},
	{0x3622, 0x04},
	{0x370a, 0x23},
	{0x370d, 0xc0},
	{0x3718, 0x88},
	{0x3721, 0x00},
	{0x3722, 0x00},
	{0x3723, 0x00},
	{0x3738, 0x00},
	{0x3800, 0x00},
	{0x3801, 0xa0},
	{0x3802, 0x00},
	{0x3803, 0x78},
	{0x3804, 0x05},
	{0x3805, 0xaf},
	{0x3806, 0x04},
	{0x3807, 0x47},
	{0x3808, 0x02},
	{0x3809, 0x80},
	{0x380a, 0x01},
	{0x380b, 0xe0},
	{0x380c, 0x06},
	{0x380d, 0xac},
	{0x380e, 0x02},
	{0x380f, 0x84},
	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3812, 0x00},
	{0x3813, 0x04},
	{0x3814, 0x31},
	{0x3815, 0x31},
#ifdef CONFIG_CAMERA_IMAGE_180
	{0x3820, 0xc2},
	{0x3821, 0x00},
#else
	{0x3820, 0xc6},
	{0x3821, 0x04},
#endif
	{0x4008, 0x00},
	{0x4009, 0x03},
	{0x4837, 0x30},
	{0x0100, 0x00}

};


LOCAL const SENSOR_REG_T ov2680_800X600_mipi_raw[] = {
	{0x0103, 0x01},
	{0x3002, 0x00},
	{0x3016, 0x1c},
	{0x3018, 0x44},
	{0x3020, 0x00},


	{0x3080, 0x02},
	{0x3082, 0x37},
	{0x3084, 0x09},
	{0x3085, 0x04},
	{0x3086, 0x01},

	{0x3501, 0x26},
	{0x3502, 0x40},
	{0x3503, 0x03},
	{0x350b, 0x36},
	{0x3600, 0xb4},
	{0x3603, 0x39},
	{0x3604, 0x24},
	{0x3605, 0x00},  //bit3:   1: use external regulator  0: use internal regulator
	{0x3620, 0x26},
	{0x3621, 0x37},
	{0x3622, 0x04},
	{0x3628, 0x00},
	{0x3705, 0x3c},
	{0x370c, 0x50},
	{0x370d, 0xc0},
	{0x3718, 0x88},
	{0x3720, 0x00},
	{0x3721, 0x00},
	{0x3722, 0x00},
	{0x3723, 0x00},
	{0x3738, 0x00},
	{0x370a, 0x23},
	{0x3717, 0x58},
	{0x3781, 0x80},
	{0x3784, 0x0c},
	{0x3789, 0x60},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x00},
	{0x3804, 0x06},
	{0x3805, 0x4f},
	{0x3806, 0x04},
	{0x3807, 0xbf},
	{0x3808, 0x03},
	{0x3809, 0x20},
	{0x380a, 0x02},
	{0x380b, 0x58},
	{0x380c, 0x06},//hts 1708 6ac  1710 6ae
	{0x380d, 0xae},
	{0x380e, 0x02}, //vts 644
	{0x380f, 0x84},
	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3812, 0x00},
	{0x3813, 0x04},
	{0x3814, 0x31},
	{0x3815, 0x31},
	{0x3819, 0x04},
#ifdef CONFIG_CAMERA_IMAGE_180
	{0x3820, 0xc2}, //bit[2] flip
	{0x3821, 0x00}, //bit[2] mirror
#else
	{0x3820, 0xc6}, //bit[2] flip
	{0x3821, 0x05}, //bit[2] mirror
#endif
	{0x4000, 0x81},
	{0x4001, 0x40},
	{0x4008, 0x00},
	{0x4009, 0x03},
	{0x4602, 0x02},
	{0x481f, 0x36},
	{0x4825, 0x36},
	{0x4837, 0x30},
	{0x5002, 0x30},
	{0x5080, 0x00},
	{0x5081, 0x41},
	{0x0100, 0x00}

};



LOCAL const SENSOR_REG_T ov2680_1600X1200_mipi_raw[] = {
	{0x3086, 0x00},
	{0x3501, 0x4e},
	{0x3502, 0xe0},
	{0x3620, 0x26},
	{0x3621, 0x37},
	{0x3622, 0x04},
	{0x370a, 0x21},
	{0x370d, 0xc0},
	{0x3718, 0x88},
	{0x3721, 0x00},
	{0x3722, 0x00},
	{0x3723, 0x00},
	{0x3738, 0x00},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x00},
	{0x3804, 0x06},
	{0x3805, 0x4f},
	{0x3806, 0x04},
	{0x3807, 0xbf},
	{0x3808, 0x06},
	{0x3809, 0x40},
	{0x380a, 0x04},
	{0x380b, 0xb0},
	{0x380c, 0x06},
	{0x380d, 0xa4},
	{0x380e, 0x05},
	{0x380f, 0x0e},
	{0x3811, 0x08},
	{0x3813, 0x08},
	{0x3814, 0x11},
	{0x3815, 0x11},
#ifdef CONFIG_CAMERA_IMAGE_180
	{0x3820, 0xc4},
	{0x3821, 0x04},
#else
	{0x3820, 0xc4},
	{0x3821, 0x04},
#endif
	{0x4008, 0x02},
	{0x4009, 0x09},
	{0x481b, 0x43},
	{0x4837, 0x18},

};

LOCAL SENSOR_REG_TAB_INFO_T s_ov2680_resolution_Tab_RAW[] = {
	{ADDR_AND_LEN_OF_ARRAY(ov2680_com_mipi_raw), 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},
	//{ADDR_AND_LEN_OF_ARRAY(ov2680_800X600_mipi_raw), 800, 600, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(ov2680_1600X1200_mipi_raw), 1600, 1200, 24, SENSOR_IMAGE_FORMAT_RAW},
	{PNULL, 0, 0, 0, 0, 0},
	//{PNULL, 0, 800, 600, 24, SENSOR_IMAGE_FORMAT_RAW},
	//{ADDR_AND_LEN_OF_ARRAY(ov2680_640x480_mipi_raw), 640, 480, 24, SENSOR_IMAGE_FORMAT_RAW},
	//{ADDR_AND_LEN_OF_ARRAY(ov2680_1600x1200_mipi_raw), 1600, 1200, 24, SENSOR_IMAGE_FORMAT_RAW},

	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

LOCAL SENSOR_TRIM_T s_ov2680_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	//{0, 0, 800, 600, 518, 330, 644, {0, 0, 800, 600}},
	{0, 0, 1600, 1200, 258, 628, 1294, {0, 0, 1600, 1200}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}}
};

LOCAL const SENSOR_REG_T s_ov2680_640X480_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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
LOCAL const SENSOR_REG_T s_ov2680_1600X1200_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

LOCAL SENSOR_VIDEO_INFO_T s_ov2680_video_info[] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	//{{{30, 30, 284, 90}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_ov2680_640X480_video_tab},
	{{{25, 25, 258, 64}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_ov2680_1600X1200_video_tab},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}
};

LOCAL unsigned long _ov2680_set_video_mode(unsigned long param)
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

	if (PNULL == s_ov2680_video_info[mode].setting_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR)&s_ov2680_video_info[mode].setting_ptr[param];
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

LOCAL SENSOR_IOCTL_FUNC_TAB_T s_ov2680_ioctl_func_tab = {
	PNULL,
	_ov2680_PowerOn,
	PNULL,
	_ov2680_Identify,

	PNULL,// write register
	PNULL,// read  register
	PNULL,
	_ov2680_GetResolutionTrimTab,
	PNULL,
	PNULL,
	PNULL,

	PNULL, //_ov2680_set_brightness,
	PNULL, // _ov2680_set_contrast,
	PNULL,
	PNULL,//_ov2680_set_saturation,

	PNULL, //_ov2680_set_work_mode,
	PNULL, //_ov2680_set_image_effect,

	_ov2680_BeforeSnapshot,
	_ov2680_after_snapshot,
	PNULL,   //_ov2680_flash,
	PNULL,
	_ov2680_write_exposure,
	PNULL,
	_ov2680_write_gain,
	PNULL,
	PNULL,
	PNULL,//_ov2680_write_af,
	PNULL,
	PNULL, //_ov2680_set_awb,
	PNULL,
	PNULL,
	PNULL, //_ov2680_set_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL, //_ov2680_GetExifInfo,
	_ov2680_ExtFunc,
	PNULL, //_ov2680_set_anti_flicker,
	_ov2680_set_video_mode,
	PNULL, //pick_jpeg_stream
	PNULL,  //meter_mode
	PNULL, //get_status
	_ov2680_StreamOn,
	_ov2680_StreamOff,
	PNULL,//_ov2680_cfg_otp,
	_ov2680_ex_write_exposure,
};


SENSOR_INFO_T g_ov2680_mipi_raw_info = {
	ov2680_I2C_ADDR_W,	// salve i2c write address
	ov2680_I2C_ADDR_R,	// salve i2c read address

	SENSOR_I2C_REG_16BIT | SENSOR_I2C_REG_8BIT|SENSOR_I2C_FREQ_100,	// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
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
	{{0x0A, 0x26},		// supply two code to identify sensor.
	 {0x0B, 0x80}},		// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,	// voltage of avdd

	1600,			// max width of source image
	1200,			// max height of source image
	"ov2680",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_B,// pattern of input image form sensor;

	s_ov2680_resolution_Tab_RAW,	// point to resolution table information structure
	&s_ov2680_ioctl_func_tab,	// point to ioctl function table
	&s_ov2680_mipi_raw_info_ptr,		// information and table about Rawrgb sensor
	NULL,			//&g_ov2680_ext_info,                // extend information about sensor
	SENSOR_AVDD_1800MV,	// iovdd
	SENSOR_AVDD_1500MV,	// dvdd
	1,			// skip frame num before preview
	0,			// skip frame num before capture
	0,			// deci frame num during preview
	0,			// deci frame num during video preview

	0,
	0,
	0,
	0,
	0,
	{SENSOR_INTERFACE_TYPE_CSI2, 1, 10, 0},
	s_ov2680_video_info,
	1,			// skip frame num while change setting
};

LOCAL struct sensor_raw_info* Sensor_GetContext(void)
{
	return s_ov2680_mipi_raw_info_ptr;
}

#define param_update(x1,x2) sprintf(name,"/data/ov2680_%s.bin",x1);\
				if(0==access(name,R_OK))\
				{\
					FILE* fp = NULL;\
					SENSOR_PRINT("param file %s exists",name);\
					if( NULL!=(fp=fopen(name,"rb")) ){\
						fread((void*)x2,1,sizeof(x2),fp);\
						fclose(fp);\
					}else{\
						SENSOR_PRINT("param open %s failure",name);\
					}\
				}\
				memset(name,0,sizeof(name))


LOCAL uint32_t Sensor_InitRawTuneInfo(void)
{
	uint32_t rtn=0x00;
#if 0
	struct sensor_raw_info* raw_sensor_ptr=Sensor_GetContext();
	struct sensor_raw_tune_info* sensor_ptr=raw_sensor_ptr->tune_ptr;
	struct sensor_raw_cali_info* cali_ptr=raw_sensor_ptr->cali_ptr;

	raw_sensor_ptr->version_info->srtuct_size=sizeof(struct sensor_raw_info);

	//sensor_ptr->ae.smart_denoise_soft_y_outdoor_index =2;
	//sensor_ptr->ae.smart_denoise_soft_y_min_index =4;
	//sensor_ptr->ae.smart_denoise_soft_y_mid_index =4;
	//sensor_ptr->ae.smart_denoise_soft_y_max_index =6;
	sensor_ptr->ae.smart_denoise_soft_uv_outdoor_index = 9;
	sensor_ptr->ae.smart_denoise_soft_uv_min_index = 12;
	sensor_ptr->ae.smart_denoise_soft_uv_mid_index = 24;
	sensor_ptr->ae.smart_denoise_soft_uv_max_index = 36;


	sensor_ptr->css.ratio[0] = 3;
	sensor_ptr->css.ratio[1] = 4;
	sensor_ptr->css.ratio[2] = 5;
	sensor_ptr->css.ratio[3] = 5;
	sensor_ptr->css.ratio[4] = 6;
	sensor_ptr->css.ratio[5] = 7;
	sensor_ptr->css.ratio[6] = 7;
	sensor_ptr->css.ratio[7] = 8;
#endif
#if 0
	raw_sensor_ptr->version_info->version_id=0x00020000;
	raw_sensor_ptr->version_info->srtuct_size=sizeof(struct sensor_raw_info);

	//bypass

	sensor_ptr->version_id=0x00020000;
	sensor_ptr->blc_bypass=0x00;
	sensor_ptr->nlc_bypass=0x01;
	sensor_ptr->lnc_bypass=0x01;
	sensor_ptr->ae_bypass=0x00;
	sensor_ptr->awb_bypass=0x00;
	sensor_ptr->bpc_bypass=0x00;
	sensor_ptr->denoise_bypass=0x01;
	sensor_ptr->grgb_bypass=0x00;
	sensor_ptr->cmc_bypass=0x00;
	sensor_ptr->gamma_bypass=0x01;
	sensor_ptr->uvdiv_bypass=0x01;
	sensor_ptr->pref_bypass=0x00;
	sensor_ptr->bright_bypass=0x00;
	sensor_ptr->contrast_bypass=0x00;
	sensor_ptr->hist_bypass=0x01;
	sensor_ptr->auto_contrast_bypass=0x00;
	sensor_ptr->af_bypass=0x01;
	sensor_ptr->edge_bypass=0x01;
	sensor_ptr->fcs_bypass=0x00;
	sensor_ptr->css_bypass=0x01;
	sensor_ptr->saturation_bypass=0x00;
	sensor_ptr->hdr_bypass=0x01;
	sensor_ptr->glb_gain_bypass=0x01;
	sensor_ptr->chn_gain_bypass=0x00;
#endif
#if 0

	//blc
	sensor_ptr->blc.mode=0x00;
	sensor_ptr->blc.offset[0].r=0x0f;
	sensor_ptr->blc.offset[0].gr=0x0f;
	sensor_ptr->blc.offset[0].gb=0x0f;
	sensor_ptr->blc.offset[0].b=0x0f;
	sensor_ptr->blc.offset[1].r=0x0f;
	sensor_ptr->blc.offset[1].gr=0x0f;
	sensor_ptr->blc.offset[1].gb=0x0f;
	sensor_ptr->blc.offset[1].b=0x0f;
	//nlc
	sensor_ptr->nlc.r_node[0]=0;
	sensor_ptr->nlc.r_node[1]=16;
	sensor_ptr->nlc.r_node[2]=32;
	sensor_ptr->nlc.r_node[3]=64;
	sensor_ptr->nlc.r_node[4]=96;
	sensor_ptr->nlc.r_node[5]=128;
	sensor_ptr->nlc.r_node[6]=160;
	sensor_ptr->nlc.r_node[7]=192;
	sensor_ptr->nlc.r_node[8]=224;
	sensor_ptr->nlc.r_node[9]=256;
	sensor_ptr->nlc.r_node[10]=288;
	sensor_ptr->nlc.r_node[11]=320;
	sensor_ptr->nlc.r_node[12]=384;
	sensor_ptr->nlc.r_node[13]=448;
	sensor_ptr->nlc.r_node[14]=512;
	sensor_ptr->nlc.r_node[15]=576;
	sensor_ptr->nlc.r_node[16]=640;
	sensor_ptr->nlc.r_node[17]=672;
	sensor_ptr->nlc.r_node[18]=704;
	sensor_ptr->nlc.r_node[19]=736;
	sensor_ptr->nlc.r_node[20]=768;
	sensor_ptr->nlc.r_node[21]=800;
	sensor_ptr->nlc.r_node[22]=832;
	sensor_ptr->nlc.r_node[23]=864;
	sensor_ptr->nlc.r_node[24]=896;
	sensor_ptr->nlc.r_node[25]=928;
	sensor_ptr->nlc.r_node[26]=960;
	sensor_ptr->nlc.r_node[27]=992;
	sensor_ptr->nlc.r_node[28]=1023;

	sensor_ptr->nlc.g_node[0]=0;
	sensor_ptr->nlc.g_node[1]=16;
	sensor_ptr->nlc.g_node[2]=32;
	sensor_ptr->nlc.g_node[3]=64;
	sensor_ptr->nlc.g_node[4]=96;
	sensor_ptr->nlc.g_node[5]=128;
	sensor_ptr->nlc.g_node[6]=160;
	sensor_ptr->nlc.g_node[7]=192;
	sensor_ptr->nlc.g_node[8]=224;
	sensor_ptr->nlc.g_node[9]=256;
	sensor_ptr->nlc.g_node[10]=288;
	sensor_ptr->nlc.g_node[11]=320;
	sensor_ptr->nlc.g_node[12]=384;
	sensor_ptr->nlc.g_node[13]=448;
	sensor_ptr->nlc.g_node[14]=512;
	sensor_ptr->nlc.g_node[15]=576;
	sensor_ptr->nlc.g_node[16]=640;
	sensor_ptr->nlc.g_node[17]=672;
	sensor_ptr->nlc.g_node[18]=704;
	sensor_ptr->nlc.g_node[19]=736;
	sensor_ptr->nlc.g_node[20]=768;
	sensor_ptr->nlc.g_node[21]=800;
	sensor_ptr->nlc.g_node[22]=832;
	sensor_ptr->nlc.g_node[23]=864;
	sensor_ptr->nlc.g_node[24]=896;
	sensor_ptr->nlc.g_node[25]=928;
	sensor_ptr->nlc.g_node[26]=960;
	sensor_ptr->nlc.g_node[27]=992;
	sensor_ptr->nlc.g_node[28]=1023;

	sensor_ptr->nlc.b_node[0]=0;
	sensor_ptr->nlc.b_node[1]=16;
	sensor_ptr->nlc.b_node[2]=32;
	sensor_ptr->nlc.b_node[3]=64;
	sensor_ptr->nlc.b_node[4]=96;
	sensor_ptr->nlc.b_node[5]=128;
	sensor_ptr->nlc.b_node[6]=160;
	sensor_ptr->nlc.b_node[7]=192;
	sensor_ptr->nlc.b_node[8]=224;
	sensor_ptr->nlc.b_node[9]=256;
	sensor_ptr->nlc.b_node[10]=288;
	sensor_ptr->nlc.b_node[11]=320;
	sensor_ptr->nlc.b_node[12]=384;
	sensor_ptr->nlc.b_node[13]=448;
	sensor_ptr->nlc.b_node[14]=512;
	sensor_ptr->nlc.b_node[15]=576;
	sensor_ptr->nlc.b_node[16]=640;
	sensor_ptr->nlc.b_node[17]=672;
	sensor_ptr->nlc.b_node[18]=704;
	sensor_ptr->nlc.b_node[19]=736;
	sensor_ptr->nlc.b_node[20]=768;
	sensor_ptr->nlc.b_node[21]=800;
	sensor_ptr->nlc.b_node[22]=832;
	sensor_ptr->nlc.b_node[23]=864;
	sensor_ptr->nlc.b_node[24]=896;
	sensor_ptr->nlc.b_node[25]=928;
	sensor_ptr->nlc.b_node[26]=960;
	sensor_ptr->nlc.b_node[27]=992;
	sensor_ptr->nlc.b_node[28]=1023;

	sensor_ptr->nlc.l_node[0]=0;
	sensor_ptr->nlc.l_node[1]=16;
	sensor_ptr->nlc.l_node[2]=32;
	sensor_ptr->nlc.l_node[3]=64;
	sensor_ptr->nlc.l_node[4]=96;
	sensor_ptr->nlc.l_node[5]=128;
	sensor_ptr->nlc.l_node[6]=160;
	sensor_ptr->nlc.l_node[7]=192;
	sensor_ptr->nlc.l_node[8]=224;
	sensor_ptr->nlc.l_node[9]=256;
	sensor_ptr->nlc.l_node[10]=288;
	sensor_ptr->nlc.l_node[11]=320;
	sensor_ptr->nlc.l_node[12]=384;
	sensor_ptr->nlc.l_node[13]=448;
	sensor_ptr->nlc.l_node[14]=512;
	sensor_ptr->nlc.l_node[15]=576;
	sensor_ptr->nlc.l_node[16]=640;
	sensor_ptr->nlc.l_node[17]=672;
	sensor_ptr->nlc.l_node[18]=704;
	sensor_ptr->nlc.l_node[19]=736;
	sensor_ptr->nlc.l_node[20]=768;
	sensor_ptr->nlc.l_node[21]=800;
	sensor_ptr->nlc.l_node[22]=832;
	sensor_ptr->nlc.l_node[23]=864;
	sensor_ptr->nlc.l_node[24]=896;
	sensor_ptr->nlc.l_node[25]=928;
	sensor_ptr->nlc.l_node[26]=960;
	sensor_ptr->nlc.l_node[27]=992;
	sensor_ptr->nlc.l_node[28]=1023;

	//ae
	sensor_ptr->ae.skip_frame=0x01;
	sensor_ptr->ae.normal_fix_fps=0;
	sensor_ptr->ae.night_fix_fps=0;
	sensor_ptr->ae.video_fps=0x1e;
	sensor_ptr->ae.target_lum=120;
	sensor_ptr->ae.target_zone=8;
	sensor_ptr->ae.quick_mode=1;
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

	//ov2680 awb param
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
	sensor_ptr->awb.debug_level = 2;
	sensor_ptr->awb.smart = 1;

	sensor_ptr->awb.alg_id = 0;
	sensor_ptr->awb.smart_index = 0;
#endif

#if 0
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
	sensor_ptr->af.alg_id=3;
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
	isp_raw_para_update_from_file(&g_ov2680_mipi_raw_info,0);

	struct sensor_raw_info* raw_sensor_ptr=Sensor_GetContext();
	struct isp_mode_param* mode_common_ptr = raw_sensor_ptr->mode_ptr[0].addr;
	int i;
	char name[100] = {'\0'};

	for (i=0; i<mode_common_ptr->block_num; i++) {
		struct isp_block_header* header = &(mode_common_ptr->block_header[i]);
		uint8_t* data = (uint8_t*)mode_common_ptr + header->offset;
		switch (header->block_id)
		{
		case	ISP_BLK_PRE_WAVELET_V1: {
				/* modify block data */
				struct sensor_pwd_param* block = (struct sensor_pwd_param*)data;

				static struct sensor_pwd_level pwd_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "noise/pwd_param.h"
				};

				param_update("pwd_param",pwd_param);

				block->param_ptr = pwd_param;
			}
			break;

		case	ISP_BLK_BPC_V1: {
				/* modify block data */
				struct sensor_bpc_param_v1* block = (struct sensor_bpc_param_v1*)data;

				static struct sensor_bpc_level bpc_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "noise/bpc_param.h"
				};

				param_update("bpc_param",bpc_param);

				block->param_ptr = bpc_param;
			}
			break;

		case	ISP_BLK_BL_NR_V1: {
				/* modify block data */
				struct sensor_bdn_param* block = (struct sensor_bdn_param*)data;

				static struct sensor_bdn_level bdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "noise/bdn_param.h"
				};

				param_update("bdn_param",bdn_param);

				block->param_ptr = bdn_param;
			}
			break;

		case	ISP_BLK_GRGB_V1: {
				/* modify block data */
				struct sensor_grgb_v1_param* block = (struct sensor_grgb_v1_param*)data;
				static struct sensor_grgb_v1_level grgb_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "noise/grgb_param.h"
				};

				param_update("grgb_param",grgb_param);

				block->param_ptr = grgb_param;

			}
			break;

		case	ISP_BLK_NLM: {
				/* modify block data */
				struct sensor_nlm_param* block = (struct sensor_nlm_param*)data;

				static struct sensor_nlm_level nlm_param[32] = {
					#include "noise/nlm_param.h"
				};

				param_update("nlm_param",nlm_param);

				static struct sensor_vst_level vst_param[32] = {
					#include "noise/vst_param.h"
				};

				param_update("vst_param",vst_param);

				static struct sensor_ivst_level ivst_param[32] = {
					#include "noise/ivst_param.h"
				};

				param_update("ivst_param",ivst_param);

				static struct sensor_flat_offset_level flat_offset_param[32] = {
					#include "noise/flat_offset_param.h"
				};

				param_update("flat_offset_param",flat_offset_param);

				block->param_nlm_ptr = nlm_param;
				block->param_vst_ptr = vst_param;
				block->param_ivst_ptr = ivst_param;
				block->param_flat_offset_ptr = flat_offset_param;
			}
			break;

		case	ISP_BLK_CFA_V1: {
				/* modify block data */
				struct sensor_cfa_param_v1* block = (struct sensor_cfa_param_v1*)data;
				static struct sensor_cfae_level cfae_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "noise/cfae_param.h"
				};

				param_update("cfae_param",cfae_param);

				block->param_ptr = cfae_param;
			}
			break;

		case	ISP_BLK_RGB_PRECDN: {
				/* modify block data */
				struct sensor_rgb_precdn_param* block = (struct sensor_rgb_precdn_param*)data;

				static struct sensor_rgb_precdn_level precdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "noise/rgb_precdn_param.h"
				};

				param_update("rgb_precdn_param",precdn_param);

				block->param_ptr = precdn_param;
			}
			break;

		case	ISP_BLK_YUV_PRECDN: {
				/* modify block data */
				struct sensor_yuv_precdn_param* block = (struct sensor_yuv_precdn_param*)data;

				static struct sensor_yuv_precdn_level yuv_precdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "noise/yuv_precdn_param.h"
				};

				param_update("yuv_precdn_param",yuv_precdn_param);

				block->param_ptr = yuv_precdn_param;
			}
			break;

		case	ISP_BLK_PREF_V1: {
				/* modify block data */
				struct sensor_prfy_param* block = (struct sensor_prfy_param*)data;

				static struct sensor_prfy_level prfy_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "noise/prfy_param.h"
				};

				param_update("prfy_param",prfy_param);

				block->param_ptr = prfy_param;
			}
			break;

		case	ISP_BLK_UV_CDN: {
				/* modify block data */
				struct sensor_uv_cdn_param* block = (struct sensor_uv_cdn_param*)data;

				static struct sensor_uv_cdn_level uv_cdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "noise/yuv_cdn_param.h"
				};

				param_update("yuv_cdn_param",uv_cdn_param);

				block->param_ptr = uv_cdn_param;
			}
			break;

		case	ISP_BLK_EDGE_V1: {
				/* modify block data */
				struct sensor_ee_param* block = (struct sensor_ee_param*)data;

				static struct sensor_ee_level edge_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "noise/edge_param.h"
				};

				param_update("edge_param",edge_param);

				block->param_ptr = edge_param;
			}
			break;

		case	ISP_BLK_UV_POSTCDN: {
				/* modify block data */
				struct sensor_uv_postcdn_param* block = (struct sensor_uv_postcdn_param*)data;

				static struct sensor_uv_postcdn_level uv_postcdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "noise/yuv_postcdn_param.h"
				};

				param_update("yuv_postcdn_param",uv_postcdn_param);

				block->param_ptr = uv_postcdn_param;
			}
			break;

		case	ISP_BLK_IIRCNR_IIR: {
				/* modify block data */
				struct sensor_iircnr_param* block = (struct sensor_iircnr_param*)data;

				static struct sensor_iircnr_level iir_cnr_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "noise/iircnr_param.h"
				};

				param_update("iircnr_param",iir_cnr_param);

				block->param_ptr = iir_cnr_param;
			}
			break;

		case	ISP_BLK_IIRCNR_YRANDOM: {
				/* modify block data */
				struct sensor_iircnr_yrandom_param* block = (struct sensor_iircnr_yrandom_param*)data;
				static struct sensor_iircnr_yrandom_level iir_yrandom_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "noise/iir_yrandom_param.h"
				};

				param_update("iir_yrandom_param",iir_yrandom_param);

				block->param_ptr = iir_yrandom_param;
			}
			break;

		case  ISP_BLK_UVDIV_V1: {
				/* modify block data */
				struct sensor_cce_uvdiv_param_v1* block = (struct sensor_cce_uvdiv_param_v1*)data;

				static struct sensor_cce_uvdiv_level cce_uvdiv_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "noise/cce_uv_param.h"
				};

				param_update("cce_uv_param",cce_uvdiv_param);

				block->param_ptr = cce_uvdiv_param;
			}
			break;
		case ISP_BLK_YIQ_AFM:{
			/* modify block data */
			struct sensor_y_afm_param *block = (struct sensor_y_afm_param*)data;

			static struct sensor_y_afm_level y_afm_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "noise/y_afm_param.h"
				};

				param_update("y_afm_param",y_afm_param);

				block->param_ptr = y_afm_param;
			}
			break;

		default:
			break;
		}
	}
	return rtn;
}

LOCAL unsigned long _ov2680_GetResolutionTrimTab(unsigned long param)
{
	SENSOR_PRINT("0x%lx", (unsigned long)s_ov2680_Resolution_Trim_Tab);
	return (unsigned long) s_ov2680_Resolution_Trim_Tab;
}

LOCAL unsigned long _ov2680_PowerOn(unsigned long power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_ov2680_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_ov2680_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_ov2680_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_ov2680_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_ov2680_mipi_raw_info.reset_pulse_level;

	if (SENSOR_TRUE == power_on) {
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_SetResetLevel(reset_level);
		Sensor_PowerDown(power_down);
		usleep(2*1000);
		//step 0 power up DOVDD, the AVDD
		Sensor_SetIovddVoltage(iovdd_val);
		usleep(1000);
		Sensor_SetAvddVoltage(avdd_val);
		usleep(1000);
		//step 2 power down pin high
		Sensor_PowerDown(!power_down);
		usleep(1000);
		//step 3 reset pin high
		Sensor_SetResetLevel(!reset_level);
		usleep(2*1000);
		//step 4 xvclk
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(2*1000);
	} else {
		usleep(1000);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		usleep(1000);
		Sensor_SetResetLevel(reset_level);
		Sensor_PowerDown(power_down);
		usleep(1000);
		Sensor_SetAvddVoltage(SENSOR_AVDD_CLOSED);
		usleep(1000);
		Sensor_SetIovddVoltage(SENSOR_AVDD_CLOSED);
	}
	SENSOR_PRINT("SENSOR_ov2680: _ov2680_Power_On(1:on, 0:off): %d", power_on);
	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov2680_cfg_otp(unsigned long  param)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_ov2680_raw_param_tab;
	uint32_t module_id=g_module_id;

	SENSOR_PRINT("SENSOR_ov2680: _ov2680_cfg_otp");

	if(PNULL!=tab_ptr[module_id].cfg_otp){
		tab_ptr[module_id].cfg_otp(0);
		}

	return rtn;
}

// index: index of otp group. (1, 2, 3)
// return: 0, group index is empty
// 1, group index has invalid data
// 2, group index has valid data
LOCAL uint32_t check_otp(int index)
{
	uint32_t flag = 0;
	uint32_t i = 0;
	uint32_t rg = 0;
	uint32_t bg = 0;

	if (index == 1)
	{
		// read otp --Bank 0
		Sensor_WriteReg(0x3d84, 0xc0);
		Sensor_WriteReg(0x3d85, 0x00);
		Sensor_WriteReg(0x3d86, 0x0f);
		Sensor_WriteReg(0x3d81, 0x01);
		usleep(5 * 1000);
		flag = Sensor_ReadReg(0x3d05);
		rg = Sensor_ReadReg(0x3d07);
		bg = Sensor_ReadReg(0x3d08);
	}
	else if (index == 2)
	{
		// read otp --Bank 0
		Sensor_WriteReg(0x3d84, 0xc0);
		Sensor_WriteReg(0x3d85, 0x00);
		Sensor_WriteReg(0x3d86, 0x0f);
		Sensor_WriteReg(0x3d81, 0x01);
		usleep(5 * 1000);
		flag = Sensor_ReadReg(0x3d0e);
		// read otp --Bank 1
		Sensor_WriteReg(0x3d84, 0xc0);
		Sensor_WriteReg(0x3d85, 0x10);
		Sensor_WriteReg(0x3d86, 0x1f);
		Sensor_WriteReg(0x3d81, 0x01);
		usleep(5 * 1000);
		rg = Sensor_ReadReg(0x3d00);
		bg = Sensor_ReadReg(0x3d01);
	}
	else if (index == 3)
	{
		// read otp --Bank 1
		Sensor_WriteReg(0x3d84, 0xc0);
		Sensor_WriteReg(0x3d85, 0x10);
		Sensor_WriteReg(0x3d86, 0x1f);
		Sensor_WriteReg(0x3d81, 0x01);
		usleep(5 * 1000);
		flag = Sensor_ReadReg(0x3d07);
		rg = Sensor_ReadReg(0x3d09);
		bg = Sensor_ReadReg(0x3d0a);
	}
	SENSOR_PRINT("ov2680 check_otp: flag = 0x%d----index = %d---\n", flag, index);
	flag = flag & 0x80;
	// clear otp buffer
	for (i=0; i<16; i++) {
		Sensor_WriteReg(0x3d00 + i, 0x00);
	}
	SENSOR_PRINT("ov2680 check_otp: flag = 0x%d  rg = 0x%x, bg = 0x%x,-------\n", flag, rg, bg);
	if (flag) {
		return 1;
	}
	else
	{
		if (rg == 0 && bg == 0)
		{
			return 0;
		}
		else
		{
			return 2;
		}
	}
}
// index: index of otp group. (1, 2, 3)
// return: 0,
static int read_otp(int index, struct otp_struct *otp_ptr)
{
	int i = 0;
	int temp = 0;
	// read otp into buffer
	if (index == 1)
	{
		// read otp --Bank 0
		Sensor_WriteReg(0x3d84, 0xc0);
		Sensor_WriteReg(0x3d85, 0x00);
		Sensor_WriteReg(0x3d86, 0x0f);
		Sensor_WriteReg(0x3d81, 0x01);
		usleep(5 * 1000);
		(*otp_ptr).module_integrator_id = (Sensor_ReadReg(0x3d05) & 0x7f);
		(*otp_ptr).lens_id = Sensor_ReadReg(0x3d06);
		temp = Sensor_ReadReg(0x3d0b);
		(*otp_ptr).rg_ratio = (Sensor_ReadReg(0x3d07)<<2) + ((temp>>6) & 0x03);
		(*otp_ptr).bg_ratio = (Sensor_ReadReg(0x3d08)<<2) + ((temp>>4) & 0x03);
		(*otp_ptr).light_rg = ((Sensor_ReadReg(0x3d0c)<<2) + (temp>>2)) & 0x03;
		(*otp_ptr).light_bg = ((Sensor_ReadReg(0x3d0d)<<2) + temp) & 0x03;

		(*otp_ptr).user_data[0] = Sensor_ReadReg(0x3d09);
		(*otp_ptr).user_data[1] = Sensor_ReadReg(0x3d0a);
	}
	else if (index == 2)
	{
		// read otp --Bank 0
		Sensor_WriteReg(0x3d84, 0xc0);
		Sensor_WriteReg(0x3d85, 0x00);
		Sensor_WriteReg(0x3d86, 0x0f);
		Sensor_WriteReg(0x3d81, 0x01);
		usleep(5 * 1000);
		(*otp_ptr).module_integrator_id = (Sensor_ReadReg(0x3d0e) & 0x7f);
		(*otp_ptr).lens_id = Sensor_ReadReg(0x3d0f);
		// read otp --Bank 1
		Sensor_WriteReg(0x3d84, 0xc0);
		Sensor_WriteReg(0x3d85, 0x10);
		Sensor_WriteReg(0x3d86, 0x1f);
		Sensor_WriteReg(0x3d81, 0x01);
		usleep(5 * 1000);
		temp = Sensor_ReadReg(0x3d04);
		(*otp_ptr).rg_ratio = (Sensor_ReadReg(0x3d00)<<2) + ((temp>>6) & 0x03);
		(*otp_ptr).bg_ratio = (Sensor_ReadReg(0x3d01)<<2) + ((temp>>4) & 0x03);
		(*otp_ptr).light_rg = ((Sensor_ReadReg(0x3d05)<<2) + (temp>>2)) & 0x03;
		(*otp_ptr).light_bg = ((Sensor_ReadReg(0x3d06)<<2) + temp) & 0x03;
		(*otp_ptr).user_data[0] = Sensor_ReadReg(0x3d02);
		(*otp_ptr).user_data[1] = Sensor_ReadReg(0x3d03);
	}
	else if (index == 3)
	{
		// read otp --Bank 1
		Sensor_WriteReg(0x3d84, 0xc0);
		Sensor_WriteReg(0x3d85, 0x10);
		Sensor_WriteReg(0x3d86, 0x1f);
		Sensor_WriteReg(0x3d81, 0x01);
		usleep(5 * 1000);
		(*otp_ptr).module_integrator_id = (Sensor_ReadReg(0x3d07) & 0x7f);
		(*otp_ptr).lens_id = Sensor_ReadReg(0x3d08);
		temp = Sensor_ReadReg(0x3d0d);
		(*otp_ptr).rg_ratio = (Sensor_ReadReg(0x3d09)<<2) + ((temp>>6) & 0x03);
		(*otp_ptr).bg_ratio = (Sensor_ReadReg(0x3d0a)<<2) + ((temp>>4) & 0x03);
		(*otp_ptr).light_rg = ((Sensor_ReadReg(0x3d0e)<<2) + (temp>>2)) & 0x03;
		(*otp_ptr).light_bg = ((Sensor_ReadReg(0x3d0f)<<2) + temp) & 0x03;
		(*otp_ptr).user_data[0] = Sensor_ReadReg(0x3d0b);
		(*otp_ptr).user_data[1] = Sensor_ReadReg(0x3d0c);
	}

	// clear otp buffer
	for (i=0;i<16;i++) {
		Sensor_WriteReg(0x3d00 + i, 0x00);
	}

	return 0;
}
// R_gain, sensor red gain of AWB, 0x400 =1
// G_gain, sensor green gain of AWB, 0x400 =1
// B_gain, sensor blue gain of AWB, 0x400 =1
// return 0;
static int update_awb_gain(int R_gain, int G_gain, int B_gain)
{
	if (R_gain>0x400) {
		Sensor_WriteReg(0x5186, R_gain>>8);
		Sensor_WriteReg(0x5187, R_gain & 0x00ff);
	}
	if (G_gain>0x400) {
		Sensor_WriteReg(0x5188, G_gain>>8);
		Sensor_WriteReg(0x5189, G_gain & 0x00ff);
	}
	if (B_gain>0x400) {
		Sensor_WriteReg(0x518a, B_gain>>8);
		Sensor_WriteReg(0x518b, B_gain & 0x00ff);
	}
	return 0;
}
// call this function after ov2680 initialization
// return: 0 update success
// 1, no OTP
LOCAL uint32_t update_otp (void* param_ptr)
{
	struct otp_struct current_otp;
	int i = 0;
	int otp_index = 0;
	int temp = 0;
	int R_gain, G_gain, B_gain, G_gain_R, G_gain_B;
	int rg = 0;
	int bg = 0;
	uint16_t stream_value = 0;
	uint16_t reg_value = 0;

	stream_value = Sensor_ReadReg(0x0100);
	SENSOR_PRINT("ov2680 update_otp:stream_value = 0x%x\n", stream_value);
	if(1 != (stream_value & 0x01))
	{
		Sensor_WriteReg(0x0100, 0x01);
		usleep(50 * 1000);
	}

	// R/G and B/G of current camera module is read out from sensor OTP
	// check first OTP with valid data
	for(i=1;i<=3;i++) {
		temp = check_otp(i);
		if (temp == 2) {
			otp_index = i;
			break;
		}
	}
	if (i>3) {
		// no valid wb OTP data
		return 1;
	}
	read_otp(otp_index, &current_otp);
	if(current_otp.light_rg==0) {
		// no light source information in OTP
		rg = current_otp.rg_ratio;
	}
	else {
		// light source information found in OTP
		rg = current_otp.rg_ratio * (current_otp.light_rg +512) / 1024;
	}
	if(current_otp.light_bg==0) {
		// no light source information in OTP
		bg = current_otp.bg_ratio;
	}
	else {
		// light source information found in OTP
		bg = current_otp.bg_ratio * (current_otp.light_bg +512) / 1024;
	}
	//calculate G gain
	//0x400 = 1x gain
	if(bg < BG_Ratio_Typical) {
		if (rg< RG_Ratio_Typical) {
			// current_otp.bg_ratio < BG_Ratio_typical &&
			// current_otp.rg_ratio < RG_Ratio_typical
			if((0 == bg) || (0 == rg)){
				SENSOR_PRINT("ov2680_otp: update otp failed!!, bg = %d, rg = %d\n", bg, rg);
				return 0;
			}
			G_gain = 0x400;
			B_gain = 0x400 * BG_Ratio_Typical / bg;
			R_gain = 0x400 * RG_Ratio_Typical / rg;
		}
		else {
			if(0 == bg){
				SENSOR_PRINT("ov2680_otp: update otp failed!!, bg = %d\n", bg);
				return 0;
			}
			// current_otp.bg_ratio < BG_Ratio_typical &&
			// current_otp.rg_ratio >= RG_Ratio_typical
			R_gain = 0x400;
			G_gain = 0x400 * rg / RG_Ratio_Typical;
			B_gain = G_gain * BG_Ratio_Typical /bg;
		}
	}
	else {
		if (rg < RG_Ratio_Typical) {
			// current_otp.bg_ratio >= BG_Ratio_typical &&
			// current_otp.rg_ratio < RG_Ratio_typical
			if(0 == rg){
				SENSOR_PRINT("ov2680_otp: update otp failed!!, rg = %d\n", rg);
				return 0;
			}
			B_gain = 0x400;
			G_gain = 0x400 * bg / BG_Ratio_Typical;
			R_gain = G_gain * RG_Ratio_Typical / rg;
		}
		else {
			// current_otp.bg_ratio >= BG_Ratio_typical &&
			// current_otp.rg_ratio >= RG_Ratio_typical
			G_gain_B = 0x400 * bg / BG_Ratio_Typical;
			G_gain_R = 0x400 * rg / RG_Ratio_Typical;
			if(G_gain_B > G_gain_R ) {
				if(0 == rg){
					SENSOR_PRINT("ov2680_otp: update otp failed!!, rg = %d\n", rg);
					return 0;
				}
				B_gain = 0x400;
				G_gain = G_gain_B;
				R_gain = G_gain * RG_Ratio_Typical /rg;
			}
			else {
				if(0 == bg){
					SENSOR_PRINT("ov2680_otp: update otp failed!!, bg = %d\n", bg);
					return 0;
				}
				R_gain = 0x400;
				G_gain = G_gain_R;
				B_gain = G_gain * BG_Ratio_Typical / bg;
			}
		}
	}
	update_awb_gain(R_gain, G_gain, B_gain);

	if(1 != (stream_value & 0x01))
		Sensor_WriteReg(0x0100, stream_value);

	SENSOR_PRINT("ov2680_otp: R_gain:0x%x, G_gain:0x%x, B_gain:0x%x------\n",R_gain, G_gain, B_gain);
	return 0;
}

LOCAL uint32_t ov2680_check_otp_module_id(void)
{
	struct otp_struct current_otp;
	int i = 0;
	int otp_index = 0;
	int temp = 0;
	uint16_t stream_value = 0;

	stream_value = Sensor_ReadReg(0x0100);
	SENSOR_PRINT("ov2680_check_otp_module_id:stream_value = 0x%x\n", stream_value);
	if(1 != (stream_value & 0x01))
	{
		Sensor_WriteReg(0x0100, 0x01);
		usleep(50 * 1000);
	}
	// R/G and B/G of current camera module is read out from sensor OTP
	// check first OTP with valid data

	for(i=1;i<=3;i++) {
		temp = check_otp(i);
		SENSOR_PRINT("ov2680_check_otp_module_id i=%d temp = %d \n",i,temp);
		if (temp == 2) {
			otp_index = i;
			break;
		}
	}
	if (i > 3) {
		// no valid wb OTP data
		SENSOR_PRINT("ov2680_check_otp_module_id no valid wb OTP data\n");
		return 1;
	}

	read_otp(otp_index, &current_otp);

	if(1 != (stream_value & 0x01))
		Sensor_WriteReg(0x0100, stream_value);

	SENSOR_PRINT("read ov2680 otp  module_id = %x \n", current_otp.module_integrator_id);

	return current_otp.module_integrator_id;
}

LOCAL uint32_t _ov2680_Truly_Identify_otp(void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_ov2680: _ov2680_Truly_Identify_otp");

	/*read param id from sensor omap*/
	param_id=ov2680_check_otp_module_id();;

	if(ov2680_RAW_PARAM_Truly==param_id){
		SENSOR_PRINT("SENSOR_ov2680: This is Truly module!!\n");
		RG_Ratio_Typical = 0x152;
		BG_Ratio_Typical = 0x137;
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}

LOCAL uint32_t _ov2680_Sunny_Identify_otp(void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_ov2680: _ov2680_Sunny_Identify_otp");

	/*read param id from sensor omap*/
	param_id=ov2680_check_otp_module_id();

	if(ov2680_RAW_PARAM_Sunny==param_id){
		SENSOR_PRINT("SENSOR_ov2680: This is Sunny module!!\n");
		RG_Ratio_Typical = 386;
		BG_Ratio_Typical = 367;
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}

LOCAL uint32_t _ov2680_com_Identify_otp(void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_ov2680: _ov2680_com_Identify_otp");

	/*read param id from sensor omap*/
	param_id=ov2680_RAW_PARAM_COM;

	if(ov2680_RAW_PARAM_COM==param_id){
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}

LOCAL uint32_t _ov2680_GetRawInof(void)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_ov2680_raw_param_tab;
	uint32_t param_id;
	uint32_t i=0x00;

	/*read param id from sensor omap*/
	param_id=ov2680_RAW_PARAM_COM;

	for(i=0x00; ; i++)
	{
		g_module_id = i;
		if(RAW_INFO_END_ID==tab_ptr[i].param_id){
			if(NULL==s_ov2680_mipi_raw_info_ptr){
				SENSOR_PRINT("SENSOR_ov2680: ov5647_GetRawInof no param error");
				rtn=SENSOR_FAIL;
			}
			SENSOR_PRINT("SENSOR_ov2680: ov2680_GetRawInof end");
			break;
		}
		else if(PNULL!=tab_ptr[i].identify_otp){
			if(SENSOR_SUCCESS==tab_ptr[i].identify_otp(0))
			{
				s_ov2680_mipi_raw_info_ptr = tab_ptr[i].info_ptr;
				SENSOR_PRINT("SENSOR_ov2680: ov2680_GetRawInof success");
				break;
			}
		}
	}

	return rtn;
}

LOCAL unsigned long _ov2680_GetMaxFrameLine(unsigned long index)
{
	uint32_t max_line=0x00;
	SENSOR_TRIM_T_PTR trim_ptr=s_ov2680_Resolution_Trim_Tab;

	max_line=trim_ptr[index].frame_line;

	return max_line;
}

LOCAL unsigned long _ov2680_Identify(unsigned long param)
{
#define ov2680_PID_VALUE    0x26
#define ov2680_PID_ADDR     0x300a
#define ov2680_VER_VALUE    0x80
#define ov2680_VER_ADDR     0x300b

	uint8_t pid_value = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("SENSOR_ov2680: mipi raw identify\n");
	//Sensor_WriteReg(0x0103, 0x01);
	//usleep(100*1000);
	pid_value = Sensor_ReadReg(ov2680_PID_ADDR);

	if (ov2680_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(ov2680_VER_ADDR);
		SENSOR_PRINT("SENSOR_ov2680: Identify: PID = %x, VER = %x", pid_value, ver_value);
		if (ov2680_VER_VALUE == ver_value) {
			ret_value=_ov2680_GetRawInof();
			Sensor_InitRawTuneInfo();
			ret_value = SENSOR_SUCCESS;
			SENSOR_PRINT("SENSOR_ov2680: this is ov2680 sensor !");
		} else {
			SENSOR_PRINT
			    ("SENSOR_ov2680: Identify this is OV%x%x sensor !", pid_value, ver_value);
		}
	} else {
		SENSOR_PRINT("SENSOR_ov2680: identify fail,pid_value=%d", pid_value);
	}

	return ret_value;
}

LOCAL unsigned long _ov2680_write_exp_dummy(uint16_t expsure_line,
								uint16_t dummy_line, uint16_t size_index)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t frame_len=0x00;
	uint16_t frame_len_cur=0x00;
	uint16_t max_frame_len=0x00;
	uint16_t value=0x00;
	uint16_t value0=0x00;
	uint16_t value1=0x00;
	uint16_t value2=0x00;

	SENSOR_PRINT("SENSOR_ov2680: write_exposure line:%d, dummy:%d, size_index:%d\n", expsure_line, dummy_line, size_index);
	SENSOR_PRINT("SENSOR_ov2680: read reg :0x3820=%x\n", Sensor_ReadReg(0x3820));
	SENSOR_PRINT("SENSOR_ov2680: read reg :0x3821=%x\n", Sensor_ReadReg(0x3821));
	max_frame_len=_ov2680_GetMaxFrameLine(size_index);

	if(0x00!=max_frame_len)
	{
		frame_len = ((expsure_line+dummy_line+4)> max_frame_len) ? (expsure_line+dummy_line+4) : max_frame_len;

		if(0x00!=(0x01&frame_len))
		{
			frame_len+=0x01;
		}

		frame_len_cur = (Sensor_ReadReg(0x380e)&0xff)<<8;
		frame_len_cur |= Sensor_ReadReg(0x380f)&0xff;

		if(frame_len_cur != frame_len){
			value=(frame_len)&0xff;
			ret_value = Sensor_WriteReg(0x380f, value);
			value=(frame_len>>0x08)&0xff;
			ret_value = Sensor_WriteReg(0x380e, value);
		}
	}

	value=(expsure_line<<0x04)&0xff;
	ret_value = Sensor_WriteReg(0x3502, value);
	value=(expsure_line>>0x04)&0xff;
	ret_value = Sensor_WriteReg(0x3501, value);
	value=(expsure_line>>0x0c)&0x0f;
	ret_value = Sensor_WriteReg(0x3500, value);

	return ret_value;
}

LOCAL unsigned long _ov2680_write_exposure(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint32_t expsure_line = 0x00;
	uint32_t dummy_line = 0x00;
	uint32_t size_index=0x00;


	expsure_line = param&0xffff;
	dummy_line = (param>>0x10)&0x0fff;
	size_index = (param>>0x1c)&0x0f;

	_ov2680_write_exp_dummy(expsure_line, dummy_line, size_index);

	return ret_value;
}


LOCAL unsigned long _ov2680_ex_write_exposure(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t expsure_line = 0x00;
	uint16_t dummy_line = 0x00;
	uint16_t size_index=0x00;
	struct sensor_ex_exposure  *ex = (struct sensor_ex_exposure*)param;

	expsure_line = ex->exposure;
	dummy_line = ex->dummy;
	size_index = ex->size_index;

	_ov2680_write_exp_dummy(expsure_line, dummy_line, size_index);

	return ret_value;
}

LOCAL unsigned long _ov2680_write_gain(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint32_t real_gain = 0;

	//real_gain = ((param&0xf)+16)*(((param>>4)&0x01)+1)*(((param>>5)&0x01)+1)*(((param>>6)&0x01)+1)*(((param>>7)&0x01)+1);
	//real_gain = real_gain*(((param>>8)&0x01)+1)*(((param>>9)&0x01)+1)*(((param>>10)&0x01)+1)*(((param>>11)&0x01)+1);
       real_gain = param >>3;
	SENSOR_PRINT("SENSOR_ov2680: real_gain:0x%x, param: 0x%x", real_gain, param);
	SENSOR_PRINT("SENSOR_ov2680: real_gain:%d, param: %d", real_gain, param);

	value = real_gain&0xff;
	ret_value = Sensor_WriteReg(0x350b, value);/*0-7*/
	value = (real_gain>>0x08)&0x03;
	ret_value = Sensor_WriteReg(0x350a, value);/*8*/

	return ret_value;
}

LOCAL unsigned long _ov2680_write_af(unsigned long param)
{
#define DW9714_VCM_SLAVE_ADDR (0x18>>1)

	uint32_t ret_value = SENSOR_SUCCESS;
	uint8_t cmd_val[2] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;

	SENSOR_PRINT("SENSOR_ov2680: _write_af %d", param);

	slave_addr = DW9714_VCM_SLAVE_ADDR;
	cmd_val[0] = (param&0xfff0)>>4;
	cmd_val[1] = ((param&0x0f)<<4)|0x09;
	cmd_len = 2;
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

	SENSOR_PRINT("SENSOR_ov2680: _write_af, ret =  %d, MSL:%x, LSL:%x\n", ret_value, cmd_val[0], cmd_val[1]);

	return ret_value;
}

LOCAL unsigned long _ov2680_ReadGain(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint32_t gain = 0;

	value = Sensor_ReadReg(0x350b);/*0-7*/
	gain = value&0xff;
	value = Sensor_ReadReg(0x350a);/*8*/
	gain |= (value<<0x08)&0x300;

	s_ov2680_gain=(int)gain;

	SENSOR_PRINT("SENSOR_ov2680: _ov2680_ReadGain gain: 0x%x", s_ov2680_gain);

	return rtn;
}

LOCAL int _ov2680_set_gain16(int gain16)
{
	// write gain, 16 = 1x
	int temp;
	gain16 = gain16 & 0x3ff;

	temp = gain16 & 0xff;
	Sensor_WriteReg(0x350b, temp);

	temp = gain16>>8;
	Sensor_WriteReg(0x350a, temp);
	SENSOR_PRINT("gain %d",gain16);

	return 0;
}

LOCAL int _ov2680_set_shutter(int shutter)
{
	// write shutter, in number of line period
	int temp;

	shutter = shutter & 0xffff;

	temp = shutter & 0x0f;
	temp = temp<<4;
	Sensor_WriteReg(0x3502, temp);

	temp = shutter & 0xfff;
	temp = temp>>4;
	Sensor_WriteReg(0x3501, temp);

	temp = shutter>>12;
	Sensor_WriteReg(0x3500, temp);

	SENSOR_PRINT("shutter %d",shutter);

	return 0;
}

LOCAL void _calculate_hdr_exposure(int capture_gain16,int capture_VTS, int capture_shutter)
{
	// write capture gain
	_ov2680_set_gain16(capture_gain16);

	_ov2680_set_shutter(capture_shutter);
}

LOCAL unsigned long _ov2680_SetEV(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint32_t gain = s_ov2680_gain;
	uint32_t ev = param;

	SENSOR_PRINT("_ov2680_SetEV param: 0x%x,0x%x,0x%x,0x%x", param, s_ov2680_gain,s_capture_VTS,s_capture_shutter);

	switch(ev) {
	case SENSOR_HDR_EV_LEVE_0:
		_calculate_hdr_exposure(s_ov2680_gain,s_capture_VTS,s_capture_shutter/2);
		break;
	case SENSOR_HDR_EV_LEVE_1:
		_calculate_hdr_exposure(s_ov2680_gain,s_capture_VTS,s_capture_shutter);
		break;
	case SENSOR_HDR_EV_LEVE_2:
		_calculate_hdr_exposure(s_ov2680_gain,s_capture_VTS,s_capture_shutter*2);
		break;
	default:
		break;
	}
	return rtn;
}

LOCAL unsigned long _ov2680_ExtFunc(unsigned long ctl_param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) ctl_param;
	SENSOR_PRINT("cmd %d",ext_ptr->cmd);
	switch (ext_ptr->cmd) {
		case SENSOR_EXT_EV:
			rtn = _ov2680_SetEV(ext_ptr->param);
			break;
		default:
			break;
	}

	return rtn;
}

int _ov2680_get_shutter(void)
{
	// read shutter, in number of line period
	int shutter;

	shutter = (Sensor_ReadReg(0x03500) & 0x0f);
	shutter = (shutter<<8) + Sensor_ReadReg(0x3501);
	shutter = (shutter<<4) + (Sensor_ReadReg(0x3502)>>4);

	return shutter;
}

LOCAL int _ov2680_get_VTS(void)
{
	// read VTS from register settings
	int VTS;

	VTS = Sensor_ReadReg(0x380e);//total vertical size[15:8] high byte

	VTS = (VTS<<8) + Sensor_ReadReg(0x380f);

	return VTS;
}

LOCAL unsigned long _ov2680_PreBeforeSnapshot(unsigned long param)
{
	uint8_t ret_l, ret_m, ret_h;
	uint32_t capture_exposure, preview_maxline;
	uint32_t capture_maxline, preview_exposure;
	uint32_t capture_mode = param & 0xffff;
	uint32_t preview_mode = (param >> 0x10 ) & 0xffff;
	uint32_t prv_linetime = s_ov2680_Resolution_Trim_Tab[preview_mode].line_time;
	uint32_t cap_linetime = s_ov2680_Resolution_Trim_Tab[capture_mode].line_time;

	SENSOR_PRINT("SENSOR_ov2680: BeforeSnapshot mode: 0x%08x",param);

	if (preview_mode == capture_mode) {
		SENSOR_PRINT("SENSOR_ov2680: prv mode equal to capmode");
		goto CFG_INFO;
	}

	ret_h = (uint8_t) Sensor_ReadReg(0x3500);
	ret_m = (uint8_t) Sensor_ReadReg(0x3501);
	ret_l = (uint8_t) Sensor_ReadReg(0x3502);
	preview_exposure = (ret_h << 12) + (ret_m << 4) + (ret_l >> 4);

	ret_h = (uint8_t) Sensor_ReadReg(0x380e);
	ret_l = (uint8_t) Sensor_ReadReg(0x380f);
	preview_maxline = (ret_h << 8) + ret_l;

	Sensor_SetMode(capture_mode);
	Sensor_SetMode_WaitDone();

	if (prv_linetime == cap_linetime) {
		SENSOR_PRINT("SENSOR_ov2680: prvline equal to capline");
		goto CFG_INFO;
	}

	ret_h = (uint8_t) Sensor_ReadReg(0x380e);
	ret_l = (uint8_t) Sensor_ReadReg(0x380f);
	capture_maxline = (ret_h << 8) + ret_l;

	capture_exposure = preview_exposure * prv_linetime/cap_linetime;

	if(0 == capture_exposure){
		capture_exposure = 1;
	}

	if(capture_exposure > (capture_maxline - 4)){
		capture_maxline = capture_exposure + 4;
		capture_maxline = (capture_maxline+1)>>1<<1;
		ret_l = (unsigned char)(capture_maxline&0x0ff);
		ret_h = (unsigned char)((capture_maxline >> 8)&0xff);
		Sensor_WriteReg(0x380e, ret_h);
		Sensor_WriteReg(0x380f, ret_l);
	}

	ret_l = ((unsigned char)capture_exposure&0xf) << 4;
	ret_m = (unsigned char)((capture_exposure&0xfff) >> 4) & 0xff;
	ret_h = (unsigned char)(capture_exposure >> 12);

	Sensor_WriteReg(0x3502, ret_l);
	Sensor_WriteReg(0x3501, ret_m);
	Sensor_WriteReg(0x3500, ret_h);

	CFG_INFO:
	s_capture_shutter = _ov2680_get_shutter();
	s_capture_VTS = _ov2680_get_VTS();
	_ov2680_ReadGain(capture_mode);
	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, s_capture_shutter);

	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov2680_BeforeSnapshot(unsigned long param)
{
	uint32_t cap_mode = (param>>CAP_MODE_BITS);
	uint32_t rtn = SENSOR_SUCCESS;

	SENSOR_PRINT("%d,%d.",cap_mode,param);

	rtn = _ov2680_PreBeforeSnapshot(param);

	return rtn;
}

LOCAL unsigned long _ov2680_after_snapshot(unsigned long param)
{
	SENSOR_PRINT("SENSOR_ov2680: after_snapshot mode:%ld", param);
	Sensor_SetMode((uint32_t)param);

	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov2680_StreamOn(unsigned long param)
{
	SENSOR_PRINT("SENSOR_ov2680: StreamOn");

	Sensor_WriteReg(0x0100, 0x01);

	return 0;
}

LOCAL unsigned long _ov2680_StreamOff(unsigned long param)
{
	SENSOR_PRINT("SENSOR_ov2680: StreamOff");

	Sensor_WriteReg(0x0100, 0x00);
	usleep(100*1000);

	return 0;
}

LOCAL unsigned long _dw9174_SRCInit(unsigned long mode)
{
	uint8_t cmd_val[6] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t ret_value = SENSOR_SUCCESS;

	slave_addr = DW9714_VCM_SLAVE_ADDR;

	switch (mode) {
		case 1:
		break;

		case 2:
		{
			cmd_val[0] = 0xec;
			cmd_val[1] = 0xa3;
			cmd_val[2] = 0xf2;
			cmd_val[3] = 0x00;
			cmd_val[4] = 0xdc;
			cmd_val[5] = 0x51;
			cmd_len = 6;
			Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
		}
		break;

		case 3:
		break;

	}

	return ret_value;
}
