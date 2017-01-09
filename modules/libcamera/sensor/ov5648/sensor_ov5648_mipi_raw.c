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
#include "sensor_ov5648_raw_param_v3.c"
#else
#endif


#define ov5648_I2C_ADDR_W        0x36
#define ov5648_I2C_ADDR_R         0x36

#define OV5648_MIN_FRAME_LEN_PRV  0x484
#define OV5648_MIN_FRAME_LEN_CAP  0x7B6
#define OV5648_RAW_PARAM_COM  0x0000
static uint32_t g_module_id = 0;

static uint32_t g_flash_mode_en = 0;
static int s_ov5648_gain = 0;
static int s_ov5648_gain_bak = 0;
static int s_ov5648_shutter_bak = 0;
static int s_ov5648_capture_shutter = 0;
static int s_ov5648_capture_VTS = 0;

#define OV5648_RAW_PARAM_Truly     0x02
#define OV5648_RAW_PARAM_Sunny    0x01

static uint16_t RG_Ratio_Typical = 0x17d;
static uint16_t BG_Ratio_Typical = 0x164;

#if 0
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
#endif

//LOCAL uint32_t update_otp(void* param_ptr);
//LOCAL uint32_t _ov5648_Truly_Identify_otp(void* param_ptr);
//LOCAL uint32_t _ov5648_Sunny_Identify_otp(void* param_ptr);
LOCAL unsigned long _ov5648_GetResolutionTrimTab(unsigned long param);
LOCAL unsigned long _ov5648_PowerOn(unsigned long power_on);
LOCAL unsigned long _ov5648_Identify(unsigned long param);
LOCAL unsigned long _ov5648_BeforeSnapshot(unsigned long param);
LOCAL unsigned long _ov5648_after_snapshot(unsigned long param);
LOCAL unsigned long _ov5648_StreamOn(unsigned long param);
LOCAL unsigned long _ov5648_StreamOff(unsigned long param);
LOCAL unsigned long _ov5648_write_exposure(unsigned long param);
LOCAL unsigned long _ov5648_write_gain(unsigned long param);
LOCAL unsigned long _ov5648_write_af(unsigned long param);
LOCAL uint32_t _ov5648_ReadGain(void);
LOCAL unsigned long _ov5648_SetEV(unsigned long param);
LOCAL unsigned long _ov5648_ExtFunc(unsigned long ctl_param);
LOCAL unsigned long _dw9174_SRCInit(unsigned long mode);
LOCAL uint32_t _dw9174_SRCDeinit(void);
LOCAL unsigned long _ov5648_flash(unsigned long param);
LOCAL uint32_t _ov5648_com_Identify_otp(void* param_ptr);
LOCAL uint32_t _ov5648_read_otp_gain(uint32_t *param);
LOCAL unsigned long _ov5648_access_val(unsigned long param);

LOCAL const struct raw_param_info_tab s_ov5648_raw_param_tab[]={
	//{OV5648_RAW_PARAM_Sunny, &s_ov5648_mipi_raw_info, _ov5648_Sunny_Identify_otp, update_otp},
	//{OV5648_RAW_PARAM_Truly, &s_ov5648_mipi_raw_info, _ov5648_Truly_Identify_otp, update_otp},
	{OV5648_RAW_PARAM_COM, &s_ov5648_mipi_raw_info, _ov5648_com_Identify_otp, PNULL},
	{RAW_INFO_END_ID, PNULL, PNULL, PNULL}
};

struct sensor_raw_info* s_ov5648_mipi_raw_info_ptr=NULL;

LOCAL const SENSOR_REG_T ov5648_com_mipi_raw[] = {
	{0x0100, 0x00},
	{0x3001, 0x00}, // D[7:0] set to input
	{0x3002, 0x00}, // D[11:8] set to input
	{0x3011, 0x02}, // Drive strength 2x
	{0x3017, 0x05},
	{0x3018, 0x4c}, // MIPI 2 lane
	{0x3022, 0x00},
	{0x3034, 0x1a}, // 10-bit mode
	{0x3035, 0x21}, // PLL
	{0x3036, 0x69}, // PLL
	{0x3037, 0x03}, // PLL
	{0x3038, 0x00}, // PLL
	{0x3039, 0x00}, // PLL
	{0x303a, 0x00}, // PLLS
	{0x303b, 0x19}, // PLLS
	{0x303c, 0x11}, // PLLS
	{0x303d, 0x30}, // PLLS
	{0x3105, 0x11},
	{0x3106, 0x05}, // PLL
	{0x3304, 0x28},
	{0x3305, 0x41},
	{0x3306, 0x30},
	{0x3308, 0x00},
	{0x3309, 0xc8},
	{0x330a, 0x01},
	{0x330b, 0x90},
	{0x330c, 0x02},
	{0x330d, 0x58},
	{0x330e, 0x03},
	{0x330f, 0x20},
	{0x3300, 0x00},
	{0x3500, 0x00}, // exposure [19:16]
	{0x3501, 0x3d}, // exposure [15:8]
	{0x3502, 0x00}, // exposure [7:0], exposure = 0x3d0 = 976
	{0x3503, 0x07}, // gain has no delay, manual agc/aec
	{0x350a, 0x00}, // gain[9:8]
	{0x350b, 0x40}, // gain[7:0], gain = 4x
	{0x3601, 0x33}, // analog control
	{0x3602, 0x00}, // analog control
	{0x3611, 0x0e}, // analog control
	{0x3612, 0x2b}, // analog control
	{0x3614, 0x50}, // analog control
	{0x3620, 0x33}, // analog control
	{0x3622, 0x00}, // analog control
	{0x3630, 0xad}, // analog control
	{0x3631, 0x00}, // analog control
	{0x3632, 0x94}, // analog control
	{0x3633, 0x17}, // analog control
	{0x3634, 0x14}, // analog control
	{0x3704, 0xc0},
	{0x3705, 0x2a}, // analog control
	{0x3708, 0x66}, // analog control
	{0x3709, 0x52}, // analog control
	{0x370b, 0x23}, // analog control
	{0x370c, 0xcf}, // analog control
	{0x370d, 0x00}, // analog control
	{0x370e, 0x00}, // analog control
	{0x371c, 0x07}, // analog control
	{0x3739, 0xd2}, // analog control
	{0x373c, 0x00},
	{0x3800, 0x00}, // xstart = 0
	{0x3801, 0x00}, // xstart
	{0x3802, 0x00}, // ystart = 0
	{0x3803, 0x00}, // ystart
	{0x3804, 0x0a}, // xend = 2623
	{0x3805, 0x3f}, // yend
	{0x3806, 0x07}, // yend = 1955
	{0x3807, 0xa3}, // yend
	{0x3808, 0x05}, // x output size = 1296
	{0x3809, 0x10}, // x output size
	{0x380a, 0x03}, // y output size = 972
	{0x380b, 0xcc}, // y output size
	{0x380c, 0x0b}, // hts = 2816
	{0x380d, 0x00}, // hts
	{0x380e, 0x03}, // vts = 992
	{0x380f, 0xe0}, // vts
	{0x3810, 0x00}, // isp x win = 8
	{0x3811, 0x08}, // isp x win
	{0x3812, 0x00}, // isp y win = 4
	{0x3813, 0x04}, // isp y win
	{0x3814, 0x31}, // x inc
	{0x3815, 0x31}, // y inc
	{0x3817, 0x00}, // hsync start
#if CONFIG_CAMERA_IMAGE_180
	{0x3820, 0x0e}, // flip on, v bin off
	{0x3821, 0x01}, // mirror off, h bin on
#else
	{0x3820, 0x08}, // flip off, v bin off
	{0x3821, 0x07}, // mirror on, h bin on
#endif
	{0x3826, 0x03},
	{0x3829, 0x00},
	{0x382b, 0x0b},
	{0x3830, 0x00},
	{0x3836, 0x00},
	{0x3837, 0x00},
	{0x3838, 0x00},
	{0x3839, 0x04},
	{0x383a, 0x00},
	{0x383b, 0x01},
	{0x3b00, 0x00}, // strobe off
	{0x3b02, 0x08}, // shutter delay
	{0x3b03, 0x00}, // shutter delay
	{0x3b04, 0x04}, // frex_exp
	{0x3b05, 0x00}, // frex_exp
	{0x3b06, 0x04},
	{0x3b07, 0x08}, // frex inv
	{0x3b08, 0x00}, // frex exp req
	{0x3b09, 0x02}, // frex end option
	{0x3b0a, 0x04}, // frex rst length
	{0x3b0b, 0x00}, // frex strobe width
	{0x3b0c, 0x3d}, // frex strobe width
	{0x3f01, 0x0d},
	{0x3f0f, 0xf5},
	{0x4000, 0x89}, // blc enable
	{0x4001, 0x02}, // blc start line
	{0x4002, 0x45}, // blc auto, reset frame number = 5
	{0x4004, 0x02}, // black line number
	{0x4005, 0x18}, // blc normal freeze
	{0x4006, 0x08},
	{0x4007, 0x10},
	{0x4008, 0x00},
	{0x4050, 0x6e}, // blc level trigger
	{0x4051, 0x8f}, // blc level trigger
	{0x4300, 0xf8},
	{0x4303, 0xff},
	{0x4304, 0x00},
	{0x4307, 0xff},
	{0x4520, 0x00},
	{0x4521, 0x00},
	{0x4511, 0x22},
	{0x4801, 0x0f},
	{0x4814, 0x2a},
	{0x481f, 0x3c}, // MIPI clk prepare min
	{0x4823, 0x3c},
	{0x4826, 0x00}, // MIPI hs prepare min
	{0x481b, 0x3c},
	{0x4827, 0x32},
	{0x4837, 0x18}, // MIPI global timing
	{0x4b00, 0x06},
	{0x4b01, 0x0a},
	{0x4b04, 0x10},
	{0x5000, 0xff}, // bpc on, wpc on
	{0x5001, 0x00}, // awb disable
	{0x5002, 0x41}, // win enable, awb gain enable
	{0x5003, 0x0a}, // buf en, bin auto en
	{0x5004, 0x00}, // size man off
	{0x5043, 0x00},
	{0x5013, 0x00},
	{0x501f, 0x03}, // ISP output data
	{0x503d, 0x00}, // test pattern off
	{0x5180, 0x08}, // manual wb gain on
	{0x5780, 0xfc},
	{0x5781, 0x1f},
	{0x5782, 0x03},
	{0x5786, 0x20},
	{0x5787, 0x40},
	{0x5788, 0x08},
	{0x5789, 0x08},
	{0x578a, 0x02},
	{0x578b, 0x01},
	{0x578c, 0x01},
	{0x578d, 0x0c},
	{0x578e, 0x02},
	{0x578f, 0x01},
	{0x5790, 0x01},
	{0x5a00, 0x08},
	{0x5b00, 0x01},
	{0x5b01, 0x40},
	{0x5b02, 0x00},
	{0x5b03, 0xf0},
	//0x301a, 0x71, // MIPI stream off
	//0x301c, 0xf1, // clock lane in LP11 mode
	{0x0100, 0x01}, // wake up from software sleep
	//{0x350b, 0x40}, // gain = 8x
	//{0x4837, 0x17}, // MIPI global timing
};


LOCAL const SENSOR_REG_T ov5648_1296X972_mipi_raw[] = {
	{0x0100, 0x00},
	// 1296x972 30fps 2 lane MIPI 420Mbps/lane
	{0x3501, 0x3d}, // exposure
	{0x3502, 0x00}, // exposure
	//{0x3034, 0x1a},  //10-bit mode
	//{0x3035, 0x11},  //PLL
	//{0x3036, 0x3e},  //PLL
	//{0x3037, 0x03},  //PLL
	{0x3708, 0x66},
	{0x3709, 0x52},
	{0x370c, 0xcf},
	{0x3800, 0x00}, // xstart = 0
	{0x3801, 0x00}, // x start
	{0x3802, 0x00}, // y start = 0
	{0x3803, 0x00}, // y start
	{0x3804, 0x0a}, // xend = 2623
	{0x3805, 0x3f}, // xend
	{0x3806, 0x07}, // yend = 1955
	{0x3807, 0xa3}, // yend
	{0x3808, 0x05}, // x output size = 1296
	{0x3809, 0x10}, // x output size
	{0x380a, 0x03}, // y output size = 972
	{0x380b, 0xcc}, // y output size
	{0x380c, 0x0b},//hts = 2816
	{0x380d, 0x00},//hts
	{0x380e,0x04}, //vts = 1156
	{0x380f, 0x84},//vts
	{0x3810, 0x00}, // isp x win = 8
	{0x3811, 0x08}, // isp x win
	{0x3812, 0x00}, // isp y win = 4
	{0x3813, 0x04}, // isp y win
	{0x3814, 0x31}, // x inc
	{0x3815, 0x31}, // y inc
	{0x3817, 0x00}, // hsync start
#if CONFIG_CAMERA_IMAGE_180
	{0x3820, 0x0e}, // flip on, v bin off
	{0x3821, 0x01}, // mirror off, h bin on
#else
	{0x3820, 0x08}, // flip off, v bin off
	{0x3821, 0x07}, // mirror on, h bin on
#endif
	{0x4004, 0x02}, // black line number
	{0x4005, 0x18}, // blc level trigger
	{0x350b, 0x80}, // gain = 8x
	{0x4837, 0x17}, // MIPI global timing
       //{0x0100, 0x01},
};

LOCAL const SENSOR_REG_T ov5648_2592X1944_mipi_raw[] = {
	{0x0100, 0x00},
	// 2592x1944 15fps 2 lane MIPI 420Mbps/lane
	{0x3501, 0x7b}, // exposure
	{0x3502, 0x00}, // exposure
	//{0x3034, 0x1a},  //10-bit mode
	//{0x3035, 0x21},  //PLL
	//{0x3036, 0x66},  //PLL
	//{0x3037, 0x03},  //PLL
	{0x3708, 0x63},
	{0x3709, 0x12},
	{0x370c, 0xcc},
	{0x3800, 0x00}, // xstart = 0
	{0x3801, 0x00}, // xstart
	{0x3802, 0x00}, // ystart = 0
	{0x3803, 0x00}, // ystart
	{0x3804, 0x0a}, // xend = 2623
	{0x3805, 0x3f}, // xend
	{0x3806, 0x07}, // yend = 1955
	{0x3807, 0xa3}, // yend
	{0x3808, 0x0a}, // x output size = 2592
	{0x3809, 0x20}, // x output size
	{0x380a, 0x07}, // y output size = 1944
	{0x380b, 0x98}, // y output size
	{0x380c, 0x0a}, // HTS = 2752
	{0x380d, 0xc0}, // HTS
	{0x380e, 0x07}, // VTS = 1974
	{0x380f, 0xb6}, // VTS
	{0x3810, 0x00}, // isp x win = 16
	{0x3811, 0x10}, // isp x win
	{0x3812, 0x00}, // isp y win = 6
	{0x3813, 0x06}, // isp y win
	{0x3814, 0x11}, // x inc
	{0x3815, 0x11}, // y inc
	{0x3817, 0x00}, // hsync start
#if CONFIG_CAMERA_IMAGE_180
	{0x3820, 0x46}, // flip on, v bin off
	{0x3821, 0x00}, // mirror off, h bin on
#else
	{0x3820, 0x40}, // flip off, v bin off
	{0x3821, 0x06}, // mirror on, v bin off
#endif
	{0x4004, 0x04}, // black line number
	{0x4005, 0x1a}, // blc always update
	{0x350b, 0x40}, // gain = 4x
	{0x4837, 0x17}, // MIPI global timing
	//{0x0100, 0x01},
};

LOCAL SENSOR_REG_TAB_INFO_T s_ov5648_resolution_Tab_RAW[] = {
	{ADDR_AND_LEN_OF_ARRAY(ov5648_com_mipi_raw), 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},
	//{ADDR_AND_LEN_OF_ARRAY(ov5648_1296X972_mipi_raw), 1296, 972, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(ov5648_2592X1944_mipi_raw), 2592, 1944, 24, SENSOR_IMAGE_FORMAT_RAW},

	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

LOCAL SENSOR_TRIM_T s_ov5648_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	//{0, 0, 1296, 972, 284, 560, 1156, {0, 0, 1296, 972}},//sysclk*10
	{0, 0, 2592, 1944, 327, 560, 1974, {0, 0, 2592, 1944}},//sysclk*10
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}}
};

LOCAL const SENSOR_REG_T s_ov5648_1296X972_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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
LOCAL const SENSOR_REG_T s_ov5648_2592X1944_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

LOCAL SENSOR_VIDEO_INFO_T s_ov5648_video_info[] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	//{{{30, 30, 284, 90}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_ov5648_1296X972_video_tab},
	{{{15, 15, 337, 64}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_ov5648_2592X1944_video_tab},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}
};

LOCAL unsigned long _ov5648_set_video_mode(unsigned long param)
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

	if (PNULL == s_ov5648_video_info[mode].setting_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR)&s_ov5648_video_info[mode].setting_ptr[param];
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

LOCAL SENSOR_IOCTL_FUNC_TAB_T s_ov5648_ioctl_func_tab = {
	PNULL,
	_ov5648_PowerOn,
	PNULL,
	_ov5648_Identify,

	PNULL,// write register
	PNULL,// read  register
	PNULL,
	_ov5648_GetResolutionTrimTab,
	PNULL,
	PNULL,
	PNULL,

	PNULL, //_ov5648_set_brightness,
	PNULL, // _ov5648_set_contrast,
	PNULL,
	PNULL,//_ov5648_set_saturation,

	PNULL, //_ov5648_set_work_mode,
	PNULL, //_ov5648_set_image_effect,

	_ov5648_BeforeSnapshot,
	_ov5648_after_snapshot,
	PNULL,   //_ov5648_flash,
	PNULL,
	_ov5648_write_exposure,
	PNULL,
	_ov5648_write_gain,
	PNULL,
	PNULL,
	_ov5648_write_af,
	PNULL,
	PNULL, //_ov5648_set_awb,
	PNULL,
	PNULL,
	PNULL, //_ov5648_set_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL, //_ov5648_GetExifInfo,
	_ov5648_ExtFunc,
	PNULL, //_ov5648_set_anti_flicker,
	_ov5648_set_video_mode,
	PNULL, //pick_jpeg_stream
	PNULL,  //meter_mode
	PNULL, //get_status
	_ov5648_StreamOn,
	_ov5648_StreamOff,
	_ov5648_access_val,
};


SENSOR_INFO_T g_ov5648_mipi_raw_info = {
	ov5648_I2C_ADDR_W,	// salve i2c write address
	ov5648_I2C_ADDR_R,	// salve i2c read address

	SENSOR_I2C_REG_16BIT | SENSOR_I2C_REG_8BIT,	// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
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
	{{0x0A, 0x56},		// supply two code to identify sensor.
	 {0x0B, 0x48}},		// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,	// voltage of avdd

	2592,			// max width of source image
	1944,			// max height of source image
	"ov5648",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_B,// pattern of input image form sensor;

	s_ov5648_resolution_Tab_RAW,	// point to resolution table information structure
	&s_ov5648_ioctl_func_tab,	// point to ioctl function table
	&s_ov5648_mipi_raw_info_ptr,		// information and table about Rawrgb sensor
	NULL,			//&g_ov5648_ext_info,                // extend information about sensor
	SENSOR_AVDD_1800MV,	// iovdd
	SENSOR_AVDD_1500MV,	// dvdd
	0,			// skip frame num before preview
	3,			// skip frame num before capture
	0,			// deci frame num during preview
	0,			// deci frame num during video preview

	0,
	0,
	0,
	0,
	0,
	{SENSOR_INTERFACE_TYPE_CSI2, 2, 10, 0},
	s_ov5648_video_info,
	3,			// skip frame num while change setting
};

LOCAL struct sensor_raw_info* Sensor_GetContext(void)
{
	return s_ov5648_mipi_raw_info_ptr;
}

#define param_update(x1,x2) sprintf(name,"/data/ov5648_%s.bin",x1);\
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
	isp_raw_para_update_from_file(&g_ov5648_mipi_raw_info,1);

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

LOCAL unsigned long _ov5648_GetResolutionTrimTab(unsigned long param)
{
	SENSOR_PRINT("0x%lx", (unsigned long)s_ov5648_Resolution_Trim_Tab);
	return (unsigned long) s_ov5648_Resolution_Trim_Tab;
}

LOCAL unsigned long _ov5648_PowerOn(unsigned long power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_ov5648_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_ov5648_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_ov5648_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_ov5648_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_ov5648_mipi_raw_info.reset_pulse_level;

	if (SENSOR_TRUE == power_on) {
	#if 0
		Sensor_PowerDown(power_down);
		// Open power
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		_dw9174_SRCInit(2);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(10*1000);
		Sensor_PowerDown(!power_down);
		usleep(10*1000);
		// Reset sensor
		Sensor_Reset(reset_level);
		usleep(20*1000);
	#else
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
		usleep(5*1000);
		//step 2 power down pin high
		Sensor_PowerDown(!power_down);
		usleep(1000);
		//step 3 reset pin high
		Sensor_SetResetLevel(!reset_level);
		usleep(2*1000);
		Sensor_SetMIPILevel(0);
		//step 4 xvclk
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(2*1000);
		_dw9174_SRCInit(2);
	#endif
	} else {
		usleep(1000);
		Sensor_SetResetLevel(reset_level);
		Sensor_PowerDown(power_down);
		usleep(1000);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		usleep(1000);
		Sensor_SetAvddVoltage(SENSOR_AVDD_CLOSED);
		usleep(1000);
		Sensor_SetIovddVoltage(SENSOR_AVDD_CLOSED);
		_dw9174_SRCDeinit();
	}
	SENSOR_PRINT("SENSOR_OV5648: _ov5648_Power_On(1:on, 0:off): %d", power_on);
	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov5648_access_val(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_VAL_T* param_ptr = (SENSOR_VAL_T*)param;
	uint16_t tmp;

	SENSOR_PRINT("SENSOR_OV5648: cfg_otp E");
	if(!param_ptr){
		return rtn;
	}

	SENSOR_PRINT("SENSOR_OV5648: param_ptr->type=%x", param_ptr->type);
	switch(param_ptr->type)
	{
		case SENSOR_VAL_TYPE_SHUTTER:
			*((uint32_t*)param_ptr->pval) = _ov5648_get_shutter();
			break;
		case SENSOR_VAL_TYPE_READ_OTP_GAIN:
			rtn = _ov5648_read_otp_gain(param_ptr->pval);
			break;
		default:
			break;
	}

	SENSOR_PRINT("SENSOR_OV5648: cfg_otp X");

	return rtn;
}

#if 0
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
	SENSOR_PRINT("ov5648 check_otp: flag = 0x%d----index = %d---\n", flag, index);
	flag = flag & 0x80;
	// clear otp buffer
	for (i=0; i<16; i++) {
		Sensor_WriteReg(0x3d00 + i, 0x00);
	}
	SENSOR_PRINT("ov5648 check_otp: flag = 0x%d  rg = 0x%x, bg = 0x%x,-------\n", flag, rg, bg);
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
#endif
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
#if 0
// call this function after OV5648 initialization
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
	SENSOR_PRINT("ov5648 update_otp:stream_value = 0x%x\n", stream_value);
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
				SENSOR_PRINT("ov5648_otp: update otp failed!!, bg = %d, rg = %d\n", bg, rg);
				return 0;
			}
			G_gain = 0x400;
			B_gain = 0x400 * BG_Ratio_Typical / bg;
			R_gain = 0x400 * RG_Ratio_Typical / rg;
		}
		else {
			if(0 == bg){
				SENSOR_PRINT("ov5648_otp: update otp failed!!, bg = %d\n", bg);
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
				SENSOR_PRINT("ov5648_otp: update otp failed!!, rg = %d\n", rg);
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
					SENSOR_PRINT("ov5648_otp: update otp failed!!, rg = %d\n", rg);
					return 0;
				}
				B_gain = 0x400;
				G_gain = G_gain_B;
				R_gain = G_gain * RG_Ratio_Typical /rg;
			}
			else {
				if(0 == bg){
					SENSOR_PRINT("ov5648_otp: update otp failed!!, bg = %d\n", bg);
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

	SENSOR_PRINT("ov5648_otp: R_gain:0x%x, G_gain:0x%x, B_gain:0x%x------\n",R_gain, G_gain, B_gain);
	return 0;
}

LOCAL uint32_t ov5648_check_otp_module_id(void)
{
	struct otp_struct current_otp;
	int i = 0;
	int otp_index = 0;
	int temp = 0;
	uint16_t stream_value = 0;

	stream_value = Sensor_ReadReg(0x0100);
	SENSOR_PRINT("ov5648_check_otp_module_id:stream_value = 0x%x\n", stream_value);
	if(1 != (stream_value & 0x01))
	{
		Sensor_WriteReg(0x0100, 0x01);
		usleep(50 * 1000);
	}
	// R/G and B/G of current camera module is read out from sensor OTP
	// check first OTP with valid data

	for(i=1;i<=3;i++) {
		temp = check_otp(i);
		SENSOR_PRINT("ov5648_check_otp_module_id i=%d temp = %d \n",i,temp);
		if (temp == 2) {
			otp_index = i;
			break;
		}
	}
	if (i > 3) {
		// no valid wb OTP data
		SENSOR_PRINT("ov5648_check_otp_module_id no valid wb OTP data\n");
		return 1;
	}

	read_otp(otp_index, &current_otp);

	if(1 != (stream_value & 0x01))
		Sensor_WriteReg(0x0100, stream_value);

	SENSOR_PRINT("read ov5648 otp  module_id = %x \n", current_otp.module_integrator_id);

	return current_otp.module_integrator_id;
}

LOCAL uint32_t _ov5648_Truly_Identify_otp(void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_OV5648: _ov5648_Truly_Identify_otp");

	/*read param id from sensor omap*/
	param_id=ov5648_check_otp_module_id();;

	if(OV5648_RAW_PARAM_Truly==param_id){
		SENSOR_PRINT("SENSOR_OV5648: This is Truly module!!\n");
		RG_Ratio_Typical = 0x152;
		BG_Ratio_Typical = 0x137;
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}

LOCAL uint32_t _ov5648_Sunny_Identify_otp(void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_OV5648: _ov5648_Sunny_Identify_otp");

	/*read param id from sensor omap*/
	param_id=ov5648_check_otp_module_id();

	if(OV5648_RAW_PARAM_Sunny==param_id){
		SENSOR_PRINT("SENSOR_OV5648: This is Sunny module!!\n");
		RG_Ratio_Typical = 386;
		BG_Ratio_Typical = 367;
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}
#endif

LOCAL uint32_t _ov5648_com_Identify_otp(void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_OV5648: _ov5648_com_Identify_otp");

	/*read param id from sensor omap*/
	param_id=OV5648_RAW_PARAM_COM;

	if(OV5648_RAW_PARAM_COM==param_id){
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}

LOCAL uint32_t _ov5648_GetRawInof(void)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_ov5648_raw_param_tab;
	uint32_t param_id;
	uint32_t i=0x00;

	/*read param id from sensor omap*/
	param_id=OV5648_RAW_PARAM_COM;

	for(i=0x00; ; i++)
	{
		g_module_id = i;
		if(RAW_INFO_END_ID==tab_ptr[i].param_id){
			if(NULL==s_ov5648_mipi_raw_info_ptr){
				SENSOR_PRINT("SENSOR_OV5648: ov5647_GetRawInof no param error");
				rtn=SENSOR_FAIL;
			}
			SENSOR_PRINT("SENSOR_OV5648: ov5648_GetRawInof end");
			break;
		}
		else if(PNULL!=tab_ptr[i].identify_otp){
			if(SENSOR_SUCCESS==tab_ptr[i].identify_otp(0))
			{
				s_ov5648_mipi_raw_info_ptr = tab_ptr[i].info_ptr;
				SENSOR_PRINT("SENSOR_OV5648: ov5648_GetRawInof success");
				break;
			}
		}
	}

	return rtn;
}

LOCAL unsigned long _ov5648_GetMaxFrameLine(unsigned long index)
{
	uint32_t max_line=0x00;
	SENSOR_TRIM_T_PTR trim_ptr=s_ov5648_Resolution_Trim_Tab;

	max_line=trim_ptr[index].frame_line;

	return max_line;
}

LOCAL unsigned long _ov5648_Identify(unsigned long param)
{
#define ov5648_PID_VALUE    0x56
#define ov5648_PID_ADDR     0x300A
#define ov5648_VER_VALUE    0x48
#define ov5648_VER_ADDR     0x300B

	uint8_t pid_value = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("SENSOR_OV5648: mipi raw identify\n");

	pid_value = Sensor_ReadReg(ov5648_PID_ADDR);

	if (ov5648_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(ov5648_VER_ADDR);
		SENSOR_PRINT("SENSOR_OV5648: Identify: PID = %x, VER = %x", pid_value, ver_value);
		if (ov5648_VER_VALUE == ver_value) {
			ret_value=_ov5648_GetRawInof();
			Sensor_InitRawTuneInfo();
			ret_value = SENSOR_SUCCESS;
			SENSOR_PRINT("SENSOR_OV5648: this is ov5648 sensor !");
		} else {
			SENSOR_PRINT
			    ("SENSOR_OV5648: Identify this is OV%x%x sensor !", pid_value, ver_value);
		}
	} else {
		SENSOR_PRINT("SENSOR_OV5648: identify fail,pid_value=%d", pid_value);
	}

	return ret_value;
}

LOCAL unsigned long _ov5648_write_exposure(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t expsure_line=0x00;
	uint16_t dummy_line=0x00;
	uint16_t frame_len=0x00;
	uint16_t frame_len_cur=0x00;
	uint16_t max_frame_len=0x00;
	uint16_t size_index=0x00;
	uint16_t value=0x00;
	uint16_t value0=0x00;
	uint16_t value1=0x00;
	uint16_t value2=0x00;

	expsure_line=param&0xffff;
	dummy_line=(param>>0x10)&0x0fff;
	size_index=(param>>0x1c)&0x0f;

	SENSOR_PRINT("SENSOR_OV5648: write_exposure line:%d, dummy:%d, size_index:%d\n", expsure_line, dummy_line, size_index);

	max_frame_len=_ov5648_GetMaxFrameLine(size_index);

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

LOCAL unsigned long _ov5648_write_gain(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint32_t real_gain = 0;
#if 1
        real_gain = param>>3;
#else
	real_gain = ((param&0xf)+16)*(((param>>4)&0x01)+1)*(((param>>5)&0x01)+1)*(((param>>6)&0x01)+1)*(((param>>7)&0x01)+1);
	real_gain = real_gain*(((param>>8)&0x01)+1)*(((param>>9)&0x01)+1)*(((param>>10)&0x01)+1)*(((param>>11)&0x01)+1);
#endif
	SENSOR_PRINT("SENSOR_OV5648: real_gain:0x%x, param: 0x%x", real_gain, param);

	value = real_gain&0xff;
	ret_value = Sensor_WriteReg(0x350b, value);/*0-7*/
	value = (real_gain>>0x08)&0x03;
	ret_value = Sensor_WriteReg(0x350a, value);/*8*/

	return ret_value;
}

LOCAL unsigned long _ov5648_write_af(unsigned long param)
{
#define DW9714_VCM_SLAVE_ADDR (0x18>>1)

	uint32_t ret_value = SENSOR_SUCCESS;
	uint8_t cmd_val[2] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;

	SENSOR_PRINT("SENSOR_OV5648: _write_af %d", param);

	slave_addr = DW9714_VCM_SLAVE_ADDR;
	cmd_val[0] = (param&0xfff0)>>4;
	cmd_val[1] = ((param&0x0f)<<4)|0x09;
	cmd_len = 2;
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

	SENSOR_PRINT("SENSOR_OV5648: _write_af, ret =  %d, MSL:%x, LSL:%x\n", ret_value, cmd_val[0], cmd_val[1]);

	return ret_value;
}

LOCAL uint32_t _ov5648_ReadGain(void)
{
	uint32_t value=0x00;
	uint32_t gain = 0;

	value = Sensor_ReadReg(0x350b);/*0-7*/
	gain = value&0xff;
	value = Sensor_ReadReg(0x350a);/*8*/
	gain |= (value<<0x08)&0x300;


	SENSOR_PRINT("SENSOR: _ov5648_ReadGain gain: 0x%x", gain);

	return gain;
}

int _ov5648_get_shutter(void)
{
	// read shutter, in number of line period
	int shutter;

	shutter = (Sensor_ReadReg(0x03500) & 0x0f);
	shutter = (shutter<<8) + Sensor_ReadReg(0x3501);
	shutter = (shutter<<4) + (Sensor_ReadReg(0x3502)>>4);

	return shutter;
}

int _ov5648_set_shutter(int shutter)
{
	// write shutter, in number of line period
	int temp;

	shutter = shutter & 0xffff;

	temp = shutter & 0xf;
	temp = temp<<4;
	Sensor_WriteReg(0x3502, temp);

	temp = shutter & 0xfff;
	temp = temp>>4;
	Sensor_WriteReg(0x3501, temp);

	temp = (shutter >> 12) & 0x0f;
	Sensor_WriteReg(0x3500, temp);

	return 0;
}

int _ov5648_set_gain16(int gain16)
{
	// write gain, 16 = 1x
	int temp;
	gain16 = gain16 & 0x3ff;

	temp = gain16 & 0xff;
	Sensor_WriteReg(0x350b, temp);

	temp = (gain16 >> 8) & 0x03;
	Sensor_WriteReg(0x350a, temp);

	return 0;
}

LOCAL int _ov5648_get_VTS(void)
{
	// read VTS from register settings
	int VTS;

	VTS = Sensor_ReadReg(0x380e);//total vertical size[15:8] high byte

	VTS = (VTS<<8) + Sensor_ReadReg(0x380f);

	return VTS;
}

LOCAL int _ov5648_set_VTS(uint16_t VTS)
{
	// set VTS from register settings

	Sensor_WriteReg(0x380e, ((VTS >> 8) & 0xff));//total vertical size[15:8] high byte

	Sensor_WriteReg(0x380f, (VTS & 0xff));

	return 0;
}


static void _ov5648_calculate_hdr_exposure(int capture_gain16,int capture_VTS, int capture_shutter)
{
	// write capture gain
	_ov5648_set_gain16(capture_gain16);
	_ov5648_set_shutter(capture_shutter);
}

LOCAL unsigned long _ov5648_SetEV(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;
	uint32_t ev = ext_ptr->param;

	SENSOR_PRINT("SENSOR_ov5648: _ov5648_SetEV param: 0x%x", ext_ptr->param);

	switch(ev) {
		case SENSOR_HDR_EV_LEVE_0:
			_ov5648_calculate_hdr_exposure(s_ov5648_gain/2, s_ov5648_capture_VTS, s_ov5648_capture_shutter);
			break;

		case SENSOR_HDR_EV_LEVE_1:
			_ov5648_calculate_hdr_exposure(s_ov5648_gain, s_ov5648_capture_VTS, s_ov5648_capture_shutter);
			break;

		case SENSOR_HDR_EV_LEVE_2:
			_ov5648_calculate_hdr_exposure(s_ov5648_gain * 2, s_ov5648_capture_VTS, s_ov5648_capture_shutter * 2);
			break;

		default:
			break;
	}
	return rtn;
}

LOCAL unsigned long _ov5648_saveLoad_exposure(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	uint8_t  ret_h, ret_m, ret_l;
	uint32_t dummy = 0;
	SENSOR_EXT_FUN_PARAM_T_PTR sl_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR)param;

	uint32_t sl_param = sl_ptr->param;
	if (sl_param) {
		/*load exposure params to sensor*/
		SENSOR_PRINT_HIGH("_ov5648_saveLoad_exposure load shutter 0x%x gain 0x%x",
							s_ov5648_shutter_bak,
							s_ov5648_gain_bak);

		_ov5648_set_gain16(s_ov5648_gain_bak);
		_ov5648_set_shutter(s_ov5648_shutter_bak);
	} else {
		/*save exposure params from sensor*/
		s_ov5648_shutter_bak = _ov5648_get_shutter();
		s_ov5648_gain_bak = _ov5648_ReadGain();
		SENSOR_PRINT_HIGH("_ov5648_saveLoad_exposure save shutter 0x%x gain 0x%x",
							s_ov5648_shutter_bak,
							s_ov5648_gain_bak);
	}
	return rtn;
}

LOCAL unsigned long _ov5648_ExtFunc(unsigned long ctl_param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) ctl_param;

	switch (ext_ptr->cmd) {
	case SENSOR_EXT_EV:
		rtn = _ov5648_SetEV(ctl_param);
		break;

	case SENSOR_EXT_EXPOSURE_SL:
		rtn = _ov5648_saveLoad_exposure(ctl_param);
		break;

		default:
			break;
	}

	return rtn;
}

LOCAL unsigned long _ov5648_BeforeSnapshot(unsigned long param)
{
	uint32_t capture_exposure, preview_maxline;
	uint32_t capture_maxline, preview_exposure;
	uint32_t gain = 0;
	uint32_t capture_mode = param & 0xffff;
	uint32_t preview_mode = (param >> 0x10 ) & 0xffff;
	uint32_t prv_linetime=s_ov5648_Resolution_Trim_Tab[preview_mode].line_time;
	uint32_t cap_linetime = s_ov5648_Resolution_Trim_Tab[capture_mode].line_time;

	SENSOR_PRINT("SENSOR_OV5648: BeforeSnapshot mode: 0x%x, prv_linetime:%d, cap_linetime:%d",param, prv_linetime, cap_linetime);

	if (preview_mode == capture_mode) {
		SENSOR_PRINT("SENSOR_OV5648: prv mode equal to cap mode");
		goto CFG_INFO;
	}

	preview_exposure = _ov5648_get_shutter();
	preview_maxline = _ov5648_get_VTS();

	gain = _ov5648_ReadGain();

	Sensor_SetMode(capture_mode);
	Sensor_SetMode_WaitDone();

	if (prv_linetime == cap_linetime) {
		SENSOR_PRINT("SENSOR_OV5648: prvline equal to capline");
		goto CFG_INFO;
	}

	capture_maxline = _ov5648_get_VTS();
	capture_exposure = preview_exposure *prv_linetime  / cap_linetime ;

	if (0 == capture_exposure) {
		capture_exposure = 1;
	}

	while(gain >= 0x20){
		if(capture_exposure*2  > capture_maxline)
			break;

		capture_exposure =capture_exposure*2;
		gain=gain / 2;
	}


	SENSOR_PRINT("SENSOR_OV5648: BeforeSnapshot,  capture_exposure = %d, capture_maxline = %d", capture_exposure, capture_maxline);

	if(capture_exposure > (capture_maxline - 4)){
		capture_maxline = capture_exposure + 4;
		capture_maxline = (capture_maxline+1)>>1<<1;
		_ov5648_set_VTS(capture_maxline);
	}

	_ov5648_set_shutter(capture_exposure);
	_ov5648_set_gain16(gain);

CFG_INFO:
	s_ov5648_capture_shutter = _ov5648_get_shutter();
	s_ov5648_capture_VTS = _ov5648_get_VTS();
	s_ov5648_gain =_ov5648_ReadGain();
	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, s_ov5648_capture_shutter);

	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov5648_after_snapshot(unsigned long param)
{
	SENSOR_PRINT("SENSOR_OV5648: after_snapshot mode:%d", param);
	Sensor_SetMode((uint32_t)param);

	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov5648_StreamOn(unsigned long param)
{
	SENSOR_PRINT("SENSOR_OV5648: StreamOn");

	Sensor_WriteReg(0x0100, 0x01);

	return 0;
}

LOCAL unsigned long _ov5648_StreamOff(unsigned long param)
{
	SENSOR_PRINT("SENSOR_OV5648: StreamOff");

	Sensor_WriteReg(0x0100, 0x00);
	usleep(50*1000);

	return 0;
}

LOCAL unsigned long _dw9174_SRCInit(unsigned long mode)
{
	uint8_t cmd_val[6] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t ret_value = SENSOR_SUCCESS;

	slave_addr = DW9714_VCM_SLAVE_ADDR;

	Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
	usleep(10*1000);

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

LOCAL uint32_t _dw9174_SRCDeinit(void)
{
	Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);

	return 0;
}

LOCAL uint32_t _ov5648_read_otp_gain(uint32_t *param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint32_t gain = 0;

	value = Sensor_ReadReg(0x350b);/*0-7*/
	gain = value&0xff;
	value = Sensor_ReadReg(0x350a);/*8*/
	gain |= (value<<0x08)&0x300;

	*param = gain;

	return rtn;
}

