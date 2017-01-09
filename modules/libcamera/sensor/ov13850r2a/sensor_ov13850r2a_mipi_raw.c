/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * V4.0
 */

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
#include "newparam/sensor_ov13850r2a_raw_param_main.c"
#else
#include "sensor_ov13850r2a_raw_param.c"
#endif

#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
#include "../af_dw9718s.h"
#endif

//#define CAMERA_IMAGE_180

#define SENSOR_NAME			"ov13850r2a"
#define I2C_SLAVE_ADDR			0x6c    /* 8bit slave address*/

#define ov13850r2a_PID_ADDR			0x300A
#define ov13850r2a_PID_VALUE			0xD8
#define ov13850r2a_VER_ADDR			0x300B
#define ov13850r2a_VER_VALUE			0x50
#define OV13850R2A_SUB_VER_ADDR       0x302A
#define OV13850R2A_SUB_VER_VALUE      0xB2
#define DW9718S_VCM_SLAVE_ADDR (0x18>>1)
/* sensor parameters begin */
/* effective sensor output image size */
#define SNAPSHOT_WIDTH			4208
#define SNAPSHOT_HEIGHT			3120
#define PREVIEW_WIDTH			2112
#define PREVIEW_HEIGHT			1568

/*Mipi output*/
#define LANE_NUM			4
#define RAW_BITS				10

#define SNAPSHOT_MIPI_PER_LANE_BPS	300
#define PREVIEW_MIPI_PER_LANE_BPS	640

/*line time unit: 0.1us*/
#define SNAPSHOT_LINE_TIME		402
#define PREVIEW_LINE_TIME		100

/* frame length*/
#define SNAPSHOT_FRAME_LENGTH		3328
#define PREVIEW_FRAME_LENGTH		3328

/* please ref your spec */
#define FRAME_OFFSET			16
#define SENSOR_MAX_GAIN			0xf8//0x07ff
#define SENSOR_BASE_GAIN		0x10//0x80
#define SENSOR_MIN_SHUTTER		4

/* please ref your spec
 * 1 : average binning
 * 2 : sum-average binning
 * 4 : sum binning
 */
#define BINNING_FACTOR			1

/* please ref spec
 * 1: sensor auto caculate
 * 0: driver caculate
 */
#define SUPPORT_AUTO_FRAME_LENGTH	0
/* sensor parameters end */

/* isp parameters, please don't change it*/
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
#define ISP_BASE_GAIN			0x80
#else
#define ISP_BASE_GAIN			0x10
#endif
/* please don't change it */
#define EX_MCLK				24

struct hdr_info_t {
	uint32_t capture_max_shutter;
	uint32_t capture_shutter;
	uint32_t capture_gain;
};

struct sensor_ev_info_t {
	uint16_t preview_shutter;
	uint16_t preview_gain;
};

/*==============================================================================
 * Description:
 * global variable
 *============================================================================*/
static struct hdr_info_t s_hdr_info;
static uint32_t s_current_default_frame_length;
struct sensor_ev_info_t s_sensor_ev_info;

#define FEATURE_OTP    /*OTP function switch*/

#ifdef FEATURE_OTP
#define MODULE_ID_NULL			0x0000
#define OV13850R2A_RAW_PARAM_COM  0x0000
#define OV13850R2A_RAW_PARAM_GLOBAL  0x0004
#define MODULE_ID_ov13850r2a_sunny		0x0001    //ov13850r2a: sensor P/N;  sunny: module vendor
#define MODULE_ID_ov13850r2a_truly		0x0002    //ov13850r2a: sensor P/N;  truly: module vendor
#define MODULE_ID_END			0xFFFF
#define LSC_PARAM_QTY 240

struct otp_info_t {
	uint32_t flag; // bit[7]: info, bit[6]:wb, bit[5]:vcm, bit[4]:lenc
	uint32_t module_id;
	uint32_t lens_id;
	uint32_t production_year;
	uint32_t production_month;
	uint32_t production_day;
	uint32_t rg_ratio;
	uint32_t bg_ratio;
	//uint32_t light_rg;
	//uint32_t light_bg;
	uint32_t lenc[186];
	uint32_t checksumLSC;
	uint32_t checksumOTP;
	uint32_t checksumTotal;
	uint32_t VCM_start;
	uint32_t VCM_end;
	uint32_t VCM_dir;
	uint32_t index;
};

#include "sensor_ov13850r2a_otp.c"

struct raw_param_info_tab s_ov13850r2a_raw_param_tab[] = {
	{OV13850R2A_RAW_PARAM_COM, &s_ov13850r2a_mipi_raw_info, ov13850r2a_Global_Identify_otp, ov13850r2a_update_otp},
	{MODULE_ID_END, PNULL, PNULL, PNULL}
};
#endif
static SENSOR_IOCTL_FUNC_TAB_T s_ov13850r2a_ioctl_func_tab;
struct sensor_raw_info *s_ov13850r2a_mipi_raw_info_ptr = &s_ov13850r2a_mipi_raw_info;

static const SENSOR_REG_T ov13850r2a_init_setting[] = {
	{0x0103, 0x01}, //sensor software reset
	{0x0102, 0x01},	//fast standby enable
	{0x0300, 0x01},
	{0x0301, 0x00},
	{0x0302, 0x28},
	{0x0303, 0x00},
	{0x030a, 0x00},
	{0x300f, 0x11},
	{0x3010, 0x01},
	{0x3011, 0x76},
	{0x3012, 0x41},
	{0x3013, 0x12},
	{0x3014, 0x11},
	{0x301f, 0x03},
	{0x3106, 0x00},
	{0x3210, 0x47},
	{0x3500, 0x00},
	{0x3501, 0x60},
	{0x3502, 0x00},
	{0x3506, 0x00},
	{0x3507, 0x02},
	{0x3508, 0x00},
	{0x350a, 0x00},
	{0x350b, 0x80},
	{0x350e, 0x00},
	{0x350f, 0x10},
	{0x351a, 0x00},
	{0x351b, 0x10},
	{0x351c, 0x00},
	{0x351d, 0x20},
	{0x351e, 0x00},
	{0x351f, 0x40},
	{0x3520, 0x00},
	{0x3521, 0x80},
	{0x3600, 0xc0},
	{0x3601, 0xfc},
	{0x3602, 0x02},
	{0x3603, 0x78},
	{0x3604, 0xb1},
	{0x3605, 0x95},
	{0x3606, 0x73},
	{0x3607, 0x07},
	{0x3609, 0x40},
	{0x360a, 0x30},
	{0x360b, 0x91},
	{0x360C, 0x09},
	{0x360f, 0x02},
	{0x3611, 0x10},
	{0x3612, 0x27},
	{0x3613, 0x33},
	{0x3615, 0x0c},
	{0x3616, 0x0e},
	{0x3641, 0x02},
	{0x3660, 0x82},
	{0x3668, 0x54},
	{0x3669, 0x00},
	{0x366a, 0x3f},
	{0x3667, 0xa0},
	{0x3702, 0x40},
	{0x3703, 0x44},
	{0x3704, 0x2c},
	{0x3705, 0x01},
	{0x3706, 0x15},
	{0x3707, 0x44},
	{0x3708, 0x3c},
	{0x3709, 0x1f},
	{0x370a, 0x27},
	{0x370b, 0x3c},
	{0x3720, 0x55},
	{0x3722, 0x84},
	{0x3728, 0x40},
	{0x372a, 0x00},
	{0x372b, 0x02},
	{0x372e, 0x22},
	{0x372f, 0xa0},
	{0x3730, 0x00},
	{0x3731, 0x00},
	{0x3732, 0x00},
	{0x3733, 0x00},
	{0x3710, 0x28},
	{0x3716, 0x03},
	{0x3718, 0x1c},
	{0x3719, 0x0c},
	{0x371a, 0x08},
	{0x371c, 0xfc},
	{0x3740, 0x01},
	{0x3741, 0xd0},
	{0x3742, 0x00},
	{0x3743, 0x01},
	{0x3748, 0x00},
	{0x374a, 0x28},
	{0x3760, 0x13},
	{0x3761, 0x33},
	{0x3762, 0x86},
	{0x3763, 0x16},
	{0x3767, 0x24},
	{0x3768, 0x06},
	{0x3769, 0x45},
	{0x376c, 0x23},
	{0x376f, 0x80},
	{0x3773, 0x06},
	{0x3d84, 0x00},
	{0x3d85, 0x17},
	{0x3d8c, 0x73},
	{0x3d8d, 0xbf},
	{0x3800, 0x00},
	{0x3801, 0x00},
	{0x3802, 0x00},
	{0x3803, 0x04},
	{0x3804, 0x10},
	{0x3805, 0x9f},
	{0x3806, 0x0c},
	{0x3807, 0x4b},
	{0x3808, 0x08},
	{0x3809, 0x40},
	{0x380a, 0x06},
	{0x380b, 0x20},
	{0x380c, 0x09},
	{0x380d, 0x60},
	{0x380e, 0x0d},
	{0x380f, 0x00},
	{0x3810, 0x00},
	{0x3811, 0x08},
	{0x3812, 0x00},
	{0x3813, 0x02},
	{0x3814, 0x31},
	{0x3815, 0x31},
	{0x3820, 0x01},
	{0x3821, 0x06},
	{0x3823, 0x00},
	{0x3826, 0x00},
	{0x3827, 0x02},
	{0x3834, 0x00},
	{0x3835, 0x14},
	{0x3836, 0x08},
	{0x3837, 0x02},
	{0x4000, 0xf1},
	{0x4001, 0x00},
	{0x400b, 0x0c},
	{0x4011, 0x00},
	{0x401a, 0x00},
	{0x401b, 0x00},
	{0x401c, 0x00},
	{0x401d, 0x00},
	{0x4020, 0x00},
	{0x4021, 0xe4},
	{0x4022, 0x04},
	{0x4023, 0xd7},
	{0x4024, 0x05},
	{0x4025, 0xbc},
	{0x4026, 0x05},
	{0x4027, 0xbf},
	{0x4028, 0x00},
	{0x4029, 0x02},
	{0x402a, 0x04},
	{0x402b, 0x08},
	{0x402c, 0x02},
	{0x402d, 0x02},
	{0x402e, 0x0c},
	{0x402f, 0x08},
	{0x403d, 0x2c},
	{0x403f, 0x7f}, //0x40 neil modify 20150819
	{0x4041, 0x07},
	{0x4500, 0x82},
	{0x4501, 0x3c},
	{0x458b, 0x00},
	{0x459c, 0x00},
	{0x459d, 0x00},
	{0x459e, 0x00},
	{0x4601, 0x83},
	{0x4602, 0x22},
	{0x4603, 0x01},
	{0x4837, 0x19},
	{0x4d00, 0x04},
	{0x4d01, 0x42},
	{0x4d02, 0xd1},
	{0x4d03, 0x90},
	{0x4d04, 0x66},
	{0x4d05, 0x65},
	{0x4d0b, 0x00},
	{0x5000, 0x0e}, //enable lnc otp
	{0x5001, 0x03}, //enable MWB function
	{0x5002, 0x07},
	{0x5003, 0x4f},
	{0x5013, 0x40},
	{0x501c, 0x00},
	{0x501d, 0x10},
	{0x5100, 0x30},
	{0x5101, 0x02},
	{0x5102, 0x01},
	{0x5103, 0x01},
	{0x5104, 0x02},
	{0x5105, 0x01},
	{0x5106, 0x01},
	{0x5107, 0x00},
	{0x5108, 0x00},
	{0x5109, 0x00},
	{0x510f, 0xfc},
	{0x5110, 0xf0},
	{0x5111, 0x10},
	{0x536d, 0x02},
	{0x536e, 0x67},
	{0x536f, 0x01},
	{0x5370, 0x4c},
	{0x5400, 0x00},
	{0x5400, 0x00},
	{0x5401, 0x61},
	{0x5402, 0x00},
	{0x5403, 0x00},
	{0x5404, 0x00},
	{0x5405, 0x40},
	{0x540c, 0x05},
	{0x5501, 0x00},
	{0x5b00, 0x00},
	{0x5b01, 0x00},
	{0x5b02, 0x01},
	{0x5b03, 0xff},
	{0x5b04, 0x02},
	{0x5b05, 0x6c},
	{0x5b09, 0x02},
	{0x5e00, 0x00},
	{0x5e10, 0x1c},
	//{0x0100, 0x01},
	{0x0100, 0x00},
};

static const SENSOR_REG_T ov13850r2a_preview_setting[] = {
//;XVCLK=24Mhz, SCLK=4x60Mhz, MIPI 640Mbps, DACCLK=240Mhz
  {0x0100, 0x00},
  {0x0300, 0x01},
  {0x0302, 0x28},
  {0x0303, 0x00},
  {0x3612, 0x27},
  {0x3614, 0x28},
  {0x370a, 0x27},
  {0x3718, 0x1c},
  {0x371b, 0x00},
  {0x372a, 0x00},
  {0x3730, 0x00},
  {0x3731, 0x00},
  {0x3732, 0x00},
  {0x3733, 0x00},
  {0x3738, 0x00},
  {0x3739, 0x00},
  {0x373a, 0x00},
  {0x373b, 0x00},
  {0x3740, 0x00},
  {0x3741, 0x00},
  {0x3743, 0x00},
  {0x3748, 0x00},
  {0x3749, 0x00},
  {0x374a, 0x00},
  {0x3780, 0x10},
  {0x3782, 0x00},
  {0x3801, 0x00},
  {0x3802, 0x00},
  {0x3803, 0x04},
  {0x3805, 0x9f},
  {0x3806, 0x0c},
  {0x3807, 0x4b},
  {0x3808, 0x08},
  {0x3809, 0x40},
  {0x380a, 0x06},
  {0x380b, 0x20},
  {0x380c, 0x09},
  {0x380d, 0x60},
  {0x380e, 0x0d},
  {0x380f, 0x00},
  {0x3811, 0x08},
  {0x3813, 0x02},
  {0x3814, 0x31},
  {0x3815, 0x31},
  {0x3820, 0x01},
  {0x3821, 0x06},
  {0x3836, 0x08},
  {0x3837, 0x02},
  {0x4020, 0x00},
  {0x4021, 0xe4},
  {0x4022, 0x04},
  {0x4023, 0xd7},
  {0x4024, 0x05},
  {0x4025, 0xbc},
  {0x4026, 0x05},
  {0x4027, 0xbf},
  {0x4501, 0x3c},
  {0x4601, 0x83},
  {0x4603, 0x01},
  {0x4837, 0x19},
  {0x5401, 0x61},
  {0x5405, 0x40},
	//{0x0100, 0x01},
	{0x0100, 0x00},
};

static const SENSOR_REG_T ov13850r2a_snapshot_setting[] = {
	//;XVCLK=24Mhz, SCLK=4x56Mhz, MIPI 640Mbps, DACCLK=252Mhz
	{0x0100, 0x00},
	{0x0300, 0x00},
	{0x0302, 0x32},
	{0x0303, 0x03},
	{0x3612, 0x68},
	{0x3614, 0x2a},
	{0x370a, 0x24},
	{0x371b, 0x01},
	{0x372a, 0x05},
	{0x3730, 0x09}, //update for 7.5fps
        {0x3731, 0x70}, //update for 7.5fps
	{0x3732, 0x09}, //update for 7.5fps
	{0x3733, 0x84}, //update for 7.5fps
	{0x3718, 0x10},
	{0x3738, 0x09}, //update for 7.5fps
	{0x3739, 0x86}, //update for 7.5fps
	{0x373a, 0x09}, //update for 7.5fps
        {0x373b, 0x88}, //update for 7.5fps
	{0x3740, 0x01}, //neil add 20151112
	{0x3741, 0xd0}, //neil add 20151112
	{0x3743, 0x01}, //neil add 20151112
	{0x3748, 0x21},
	{0x3749, 0x22},
	{0x374a, 0x28},	//neil add 20151112
	{0x3780, 0x90},
	{0x3782, 0x01},
	{0x3801, 0x0c},
	{0x3802, 0x00}, //neil add 20151112
	{0x3803, 0x04},	//neil add 20151112
	{0x3805, 0x93},
	{0x3806, 0x0c}, //neil add 20151112
	{0x3807, 0x4b},	//neil add 20151112
	{0x3808, 0x10},
	{0x3809, 0x70},
	{0x380a, 0x0c},
	{0x380b, 0x30},
	{0x380c, 0x11},
	{0x380d, 0xa0},
	{0x380e, 0x0d},
	{0x380f, 0x00},
	{0x3811, 0x0c},
	{0x3813, 0x0c},
	{0x3814, 0x11},
	{0x3815, 0x11},
	{0x3820, 0x00},
	{0x3821, 0x04},
	{0x3836, 0x04},
	{0x3837, 0x01},
	{0x4020, 0x03},
	{0x4021, 0x6c},
	{0x4022, 0x0d},
	{0x4023, 0x17},
	{0x4024, 0x0d},
	{0x4025, 0xfc},
	{0x4026, 0x0d},
	{0x4027, 0xff},
	{0x4501, 0x38},
	{0x4601, 0x04},
	{0x4603, 0x00},
	{0x4837, 0x35},
	{0x5401, 0x71},
	{0x5405, 0x80},
	//{0x0100, 0x01},
	{0x0100, 0x00},
};

static SENSOR_REG_TAB_INFO_T s_ov13850r2a_resolution_tab_raw[SENSOR_MODE_MAX] = {
	{ADDR_AND_LEN_OF_ARRAY(ov13850r2a_init_setting), 0, 0, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(ov13850r2a_preview_setting),
	 PREVIEW_WIDTH, PREVIEW_HEIGHT, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(ov13850r2a_snapshot_setting),
	 SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},
};

static SENSOR_TRIM_T s_ov13850r2a_resolution_trim_tab[SENSOR_MODE_MAX] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, PREVIEW_WIDTH, PREVIEW_HEIGHT,
	 PREVIEW_LINE_TIME, PREVIEW_MIPI_PER_LANE_BPS, PREVIEW_FRAME_LENGTH,
	 {0, 0, PREVIEW_WIDTH, PREVIEW_HEIGHT}},
	{0, 0, SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT,
	 SNAPSHOT_LINE_TIME, SNAPSHOT_MIPI_PER_LANE_BPS, SNAPSHOT_FRAME_LENGTH,
	 {0, 0, SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT}},
};

static const SENSOR_REG_T s_ov13850r2a_preview_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
	/*video mode 0: ?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 1:?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 2:?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 3:?fps */
	{
	 {0xffff, 0xff}
	 }
};

static const SENSOR_REG_T s_ov13850r2a_capture_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
	/*video mode 0: ?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 1:?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 2:?fps */
	{
	 {0xffff, 0xff}
	 },
	/* video mode 3:?fps */
	{
	 {0xffff, 0xff}
	 }
};

static SENSOR_VIDEO_INFO_T s_ov13850r2a_video_info[SENSOR_MODE_MAX] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{30, 30, 270, 90}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 (SENSOR_REG_T **) s_ov13850r2a_preview_size_video_tab},
	{{{2, 5, 338, 1000}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 (SENSOR_REG_T **) s_ov13850r2a_capture_size_video_tab},
};

/*==============================================================================
 * Description:
 * set video mode
 *
 *============================================================================*/
static uint32_t ov13850r2a_set_video_mode(uint32_t param)
{
	SENSOR_REG_T_PTR sensor_reg_ptr;
	uint16_t i = 0x00;
	uint32_t mode;

	if (param >= SENSOR_VIDEO_MODE_MAX)
		return 0;

	if (SENSOR_SUCCESS != Sensor_GetMode(&mode)) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	if (PNULL == s_ov13850r2a_video_info[mode].setting_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR) & s_ov13850r2a_video_info[mode].setting_ptr[param];
	if (PNULL == sensor_reg_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	for (i = 0x00; (0xffff != sensor_reg_ptr[i].reg_addr)
	     || (0xff != sensor_reg_ptr[i].reg_value); i++) {
		Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	return 0;
}

/*==============================================================================
 * Description:
 * sensor all info
 * please modify this variable acording your spec
 *============================================================================*/
SENSOR_INFO_T g_ov13850r2a_mipi_raw_info = {
	/* salve i2c write address */
	(I2C_SLAVE_ADDR >> 1),
	/* salve i2c read address */
	(I2C_SLAVE_ADDR >> 1),
	/*bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit */
	SENSOR_I2C_REG_16BIT | SENSOR_I2C_VAL_8BIT | SENSOR_I2C_FREQ_400,
	/* bit2: 0:negative; 1:positive -> polarily of horizontal synchronization signal
	 * bit4: 0:negative; 1:positive -> polarily of vertical synchronization signal
	 * other bit: reseved
	 */
	SENSOR_HW_SIGNAL_PCLK_N | SENSOR_HW_SIGNAL_VSYNC_N | SENSOR_HW_SIGNAL_HSYNC_P,
	/* preview mode */
	SENSOR_ENVIROMENT_NORMAL | SENSOR_ENVIROMENT_NIGHT,
	/* image effect */
	SENSOR_IMAGE_EFFECT_NORMAL |
	    SENSOR_IMAGE_EFFECT_BLACKWHITE |
	    SENSOR_IMAGE_EFFECT_RED |
	    SENSOR_IMAGE_EFFECT_GREEN | SENSOR_IMAGE_EFFECT_BLUE | SENSOR_IMAGE_EFFECT_YELLOW |
	    SENSOR_IMAGE_EFFECT_NEGATIVE | SENSOR_IMAGE_EFFECT_CANVAS,

	/* while balance mode */
	0,
	/* bit[0:7]: count of step in brightness, contrast, sharpness, saturation
	 * bit[8:31] reseved
	 */
	7,
	/* reset pulse level */
	SENSOR_LOW_PULSE_RESET,
	/* reset pulse width(ms) */
	50,
	/* 1: high level valid; 0: low level valid */
	SENSOR_LOW_LEVEL_PWDN,
	/* count of identify code */
	1,
	/* supply two code to identify sensor.
	 * for Example: index = 0-> Device id, index = 1 -> version id
	 * customer could ignore it.
	 */
	{{ov13850r2a_PID_ADDR, ov13850r2a_PID_VALUE}
	 ,
	 {ov13850r2a_VER_ADDR, ov13850r2a_VER_VALUE}
	 }
	,
	/* voltage of avdd */
	SENSOR_AVDD_2800MV,
	/* max width of source image */
	SNAPSHOT_WIDTH,
	/* max height of source image */
	SNAPSHOT_HEIGHT,
	/* name of sensor */
	SENSOR_NAME,
	/* define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	 * if set to SENSOR_IMAGE_FORMAT_MAX here,
	 * image format depent on SENSOR_REG_TAB_INFO_T
	 */
	SENSOR_IMAGE_FORMAT_RAW,
	/*  pattern of input image form sensor */
	SENSOR_IMAGE_PATTERN_RAWRGB_B,
	/* point to resolution table information structure */
	s_ov13850r2a_resolution_tab_raw,
	/* point to ioctl function table */
	&s_ov13850r2a_ioctl_func_tab,
	/* information and table about Rawrgb sensor */
	&s_ov13850r2a_mipi_raw_info_ptr,
	/* extend information about sensor
	 * like &g_ov13850r2a_ext_info
	 */
	NULL,
	/* voltage of iovdd */
	SENSOR_AVDD_1800MV,
	/* voltage of dvdd */
	SENSOR_AVDD_1200MV,
	/* skip frame num before preview */
	1,
	/* skip frame num before capture */
	1,
	/* deci frame num during preview */
	0,
	/* deci frame num during video preview */
	0,
	0,
	0,
	0,
	0,
	0,
	{SENSOR_INTERFACE_TYPE_CSI2, LANE_NUM, RAW_BITS, 0}
	,
	0,
	/* skip frame num while change setting */
	1,
	/* horizontal  view angle*/
	65,
	/* vertical view angle*/
	60
};

#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)

#define param_update(x1,x2) sprintf(name,"/data/misc/media/ov13850r2a_%s.bin",x1);\
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

static uint32_t ov13850r2a_InitRawTuneInfo(void)
{
	uint32_t rtn=0x00;

	isp_raw_para_update_from_file(&g_ov13850r2a_mipi_raw_info,0);

	struct sensor_raw_info* raw_sensor_ptr=s_ov13850r2a_mipi_raw_info_ptr;
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
					#include "NR/pwd_param.h"
				};

				param_update("pwd_param",pwd_param);

				block->param_ptr = pwd_param;
			}
			break;

		case	ISP_BLK_BPC_V1: {
				/* modify block data */
				struct sensor_bpc_param_v1* block = (struct sensor_bpc_param_v1*)data;

				static struct sensor_bpc_level bpc_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/bpc_param.h"
				};

				param_update("bpc_param",bpc_param);

				block->param_ptr = bpc_param;
			}
			break;

		case	ISP_BLK_BL_NR_V1: {
				/* modify block data */
				struct sensor_bdn_param* block = (struct sensor_bdn_param*)data;

				static struct sensor_bdn_level bdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/bdn_param.h"
				};

				param_update("bdn_param",bdn_param);

				block->param_ptr = bdn_param;
			}
			break;

		case	ISP_BLK_GRGB_V1: {
				/* modify block data */
				struct sensor_grgb_v1_param* block = (struct sensor_grgb_v1_param*)data;
				static struct sensor_grgb_v1_level grgb_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/grgb_param.h"
				};

				param_update("grgb_param",grgb_param);

				block->param_ptr = grgb_param;

			}
			break;

		case	ISP_BLK_NLM: {
				/* modify block data */
				struct sensor_nlm_param* block = (struct sensor_nlm_param*)data;

				static struct sensor_nlm_level nlm_param[32] = {
					#include "NR/nlm_param.h"
				};

				param_update("nlm_param",nlm_param);

				static struct sensor_vst_level vst_param[32] = {
					#include "NR/vst_param.h"
				};

				param_update("vst_param",vst_param);

				static struct sensor_ivst_level ivst_param[32] = {
					#include "NR/ivst_param.h"
				};

				param_update("ivst_param",ivst_param);

				static struct sensor_flat_offset_level flat_offset_param[32] = {
					#include "NR/flat_offset_param.h"
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
					#include "NR/cfae_param.h"
				};

				param_update("cfae_param",cfae_param);

				block->param_ptr = cfae_param;
			}
			break;

		case	ISP_BLK_RGB_PRECDN: {
				/* modify block data */
				struct sensor_rgb_precdn_param* block = (struct sensor_rgb_precdn_param*)data;

				static struct sensor_rgb_precdn_level precdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/rgb_precdn_param.h"
				};

				param_update("rgb_precdn_param",precdn_param);

				block->param_ptr = precdn_param;
			}
			break;

		case	ISP_BLK_YUV_PRECDN: {
				/* modify block data */
				struct sensor_yuv_precdn_param* block = (struct sensor_yuv_precdn_param*)data;

				static struct sensor_yuv_precdn_level yuv_precdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/yuv_precdn_param.h"
				};

				param_update("yuv_precdn_param",yuv_precdn_param);

				block->param_ptr = yuv_precdn_param;
			}
			break;

		case	ISP_BLK_PREF_V1: {
				/* modify block data */
				struct sensor_prfy_param* block = (struct sensor_prfy_param*)data;

				static struct sensor_prfy_level prfy_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/prfy_param.h"
				};

				param_update("prfy_param",prfy_param);

				block->param_ptr = prfy_param;
			}
			break;

		case	ISP_BLK_UV_CDN: {
				/* modify block data */
				struct sensor_uv_cdn_param* block = (struct sensor_uv_cdn_param*)data;

				static struct sensor_uv_cdn_level uv_cdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/yuv_cdn_param.h"
				};

				param_update("yuv_cdn_param",uv_cdn_param);

				block->param_ptr = uv_cdn_param;
			}
			break;

		case	ISP_BLK_EDGE_V1: {
				/* modify block data */
				struct sensor_ee_param* block = (struct sensor_ee_param*)data;

				static struct sensor_ee_level edge_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/edge_param.h"
				};

				param_update("edge_param",edge_param);

				block->param_ptr = edge_param;
			}
			break;

		case	ISP_BLK_UV_POSTCDN: {
				/* modify block data */
				struct sensor_uv_postcdn_param* block = (struct sensor_uv_postcdn_param*)data;

				static struct sensor_uv_postcdn_level uv_postcdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/yuv_postcdn_param.h"
				};

				param_update("yuv_postcdn_param",uv_postcdn_param);

				block->param_ptr = uv_postcdn_param;
			}
			break;

		case	ISP_BLK_IIRCNR_IIR: {
				/* modify block data */
				struct sensor_iircnr_param* block = (struct sensor_iircnr_param*)data;

				static struct sensor_iircnr_level iir_cnr_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/iircnr_param.h"
				};

				param_update("iircnr_param",iir_cnr_param);

				block->param_ptr = iir_cnr_param;
			}
			break;

		case	ISP_BLK_IIRCNR_YRANDOM: {
				/* modify block data */
				struct sensor_iircnr_yrandom_param* block = (struct sensor_iircnr_yrandom_param*)data;
				static struct sensor_iircnr_yrandom_level iir_yrandom_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/iir_yrandom_param.h"
				};

				param_update("iir_yrandom_param",iir_yrandom_param);

				block->param_ptr = iir_yrandom_param;
			}
			break;

		case  ISP_BLK_UVDIV_V1: {
				/* modify block data */
				struct sensor_cce_uvdiv_param_v1* block = (struct sensor_cce_uvdiv_param_v1*)data;

				static struct sensor_cce_uvdiv_level cce_uvdiv_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/cce_uv_param.h"
				};

				param_update("cce_uv_param",cce_uvdiv_param);

				block->param_ptr = cce_uvdiv_param;
			}
			break;
		case ISP_BLK_YIQ_AFM:{
			/* modify block data */
			struct sensor_y_afm_param *block = (struct sensor_y_afm_param*)data;

			static struct sensor_y_afm_level y_afm_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "NR/y_afm_param.h"
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
#endif

/*==============================================================================
 * Description:
 * get default frame length
 *
 *============================================================================*/
static uint32_t ov13850r2a_get_default_frame_length(uint32_t mode)
{
	return s_ov13850r2a_resolution_trim_tab[mode].frame_line;
}

/*==============================================================================
 * Description:
 * write group-hold on to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov13850r2a_group_hold_on(void)
{
	SENSOR_PRINT("E");

	/*Sensor_WriteReg(0xsunnyY, 0xff);*/
}

/*==============================================================================
 * Description:
 * write group-hold off to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov13850r2a_group_hold_off(void)
{
	SENSOR_PRINT("E");

	/*Sensor_WriteReg(0xsunnyY, 0xff);*/
}


/*==============================================================================
 * Description:
 * read gain from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t ov13850r2a_read_gain(void)
{
	uint16_t gain_h = 0;
	uint16_t gain_l = 0;

	gain_h = Sensor_ReadReg(0x350A) & 0xff;
	gain_l = Sensor_ReadReg(0x350B) & 0xff;

	return ((gain_h << 8) | gain_l);
}

/*==============================================================================
 * Description:
 * write gain to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov13850r2a_write_gain(uint32_t gain)
{
	if (SENSOR_MAX_GAIN < gain)
			gain = SENSOR_MAX_GAIN;

	Sensor_WriteReg(0x350A, (gain >> 0x08) & 0x07);/*8*/
	Sensor_WriteReg(0x350B, gain & 0xff);/*0-7*/

	ov13850r2a_group_hold_off();

}

/*==============================================================================
 * Description:
 * read frame length from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t ov13850r2a_read_frame_length(void)
{
	uint16_t frame_len_h = 0;
	uint16_t frame_len_l = 0;

	frame_len_h = Sensor_ReadReg(0x380e) & 0xff;
	frame_len_l = Sensor_ReadReg(0x380f) & 0xff;

	return ((frame_len_h << 8) | frame_len_l);
}

/*==============================================================================
 * Description:
 * write frame length to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov13850r2a_write_frame_length(uint32_t frame_len)
{
	Sensor_WriteReg(0x380e, (frame_len >> 8) & 0xff);
	Sensor_WriteReg(0x380f, frame_len & 0xff);
}

/*==============================================================================
 * Description:
 * read shutter from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t ov13850r2a_read_shutter(void)
{
/* read shutter, in number of line period */
	int shutter;

	shutter = (Sensor_ReadReg(0x3500) & 0x0f);
	shutter = (shutter<<8) + Sensor_ReadReg(0x3501);
	shutter = (shutter<<4) + (Sensor_ReadReg(0x3502)>>4);

	return shutter;
}

/*==============================================================================
 * Description:
 * write shutter to sensor registers
 * please pay attention to the frame length
 * please modify this function acording your spec
 *============================================================================*/
static void ov13850r2a_write_shutter(uint32_t shutter)
{
	uint16_t value=0x00;
	value=(shutter<<0x04)&0xff;
	Sensor_WriteReg(0x3502, value);
	value=(shutter>>0x04)&0xff;
	Sensor_WriteReg(0x3501, value);
	value=(shutter>>0x0c)&0x0f;
	Sensor_WriteReg(0x3500, value);
}

/*==============================================================================
 * Description:
 * write exposure to sensor registers and get current shutter
 * please pay attention to the frame length
 * please don't change this function if it's necessary
 *============================================================================*/
static uint16_t ov13850r2a_update_exposure(uint32_t shutter,uint32_t dummy_line)
{
	uint32_t dest_fr_len = 0;
	uint32_t cur_fr_len = 0;
	uint32_t fr_len = s_current_default_frame_length;

	ov13850r2a_group_hold_on();

	if (1 == SUPPORT_AUTO_FRAME_LENGTH)
		goto write_sensor_shutter;

	dest_fr_len = ((shutter + dummy_line+FRAME_OFFSET) > fr_len) ? (shutter +dummy_line+ FRAME_OFFSET) : fr_len;

	cur_fr_len = ov13850r2a_read_frame_length();

	if (shutter < SENSOR_MIN_SHUTTER)
		shutter = SENSOR_MIN_SHUTTER;

	if (dest_fr_len != cur_fr_len)
		ov13850r2a_write_frame_length(dest_fr_len);
write_sensor_shutter:
	/* write shutter to sensor registers */
	ov13850r2a_write_shutter(shutter);
	return shutter;
}

/*==============================================================================
 * Description:
 * sensor power on
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t ov13850r2a_power_on(uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_ov13850r2a_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_ov13850r2a_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_ov13850r2a_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_ov13850r2a_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_ov13850r2a_mipi_raw_info.reset_pulse_level;

	if (SENSOR_TRUE == power_on) {
		Sensor_PowerDown(power_down);
		Sensor_SetResetLevel(reset_level);
		usleep(10 * 1000);
		Sensor_SetAvddVoltage(avdd_val);
		Sensor_SetIovddVoltage(iovdd_val);
		Sensor_SetDvddVoltage(dvdd_val);
		Sensor_PowerDown(!power_down);
		Sensor_SetResetLevel(!reset_level);
		usleep(10 * 1000);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);

		#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		usleep(5 * 1000);
		dw9718s_init(2);
		#endif

	} else {

		#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
		dw9718s_deinit(2);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
		#endif

		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetResetLevel(reset_level);
		Sensor_PowerDown(power_down);
		Sensor_SetDvddVoltage(SENSOR_AVDD_CLOSED);
		usleep(5 * 1000);
		Sensor_SetIovddVoltage(SENSOR_AVDD_CLOSED);
		usleep(5 * 1000);
		Sensor_SetAvddVoltage(SENSOR_AVDD_CLOSED);
	}

	SENSOR_PRINT("(1:on, 0:off): %d", power_on);
	return SENSOR_SUCCESS;
}

#ifdef FEATURE_OTP

/*==============================================================================
 * Description:
 * get  parameters from otp
 * please modify this function acording your spec
 *============================================================================*/
static int ov13850r2a_get_otp_info(struct otp_info_t *otp_info)
{
	uint32_t ret = SENSOR_FAIL;
	uint32_t i = 0x00;

	//identify otp information
	for (i = 0; i < NUMBER_OF_ARRAY(s_ov13850r2a_raw_param_tab); i++) {
		SENSOR_PRINT("identify module_id=0x%x",s_ov13850r2a_raw_param_tab[i].param_id);

		if(PNULL!=s_ov13850r2a_raw_param_tab[i].identify_otp){
			//set default value;
			memset(otp_info, 0x00, sizeof(struct otp_info_t));

			if(SENSOR_SUCCESS==s_ov13850r2a_raw_param_tab[i].identify_otp(otp_info)){
				if (s_ov13850r2a_raw_param_tab[i].param_id== otp_info->module_id) {
					SENSOR_PRINT("identify otp sucess! module_id=0x%x",s_ov13850r2a_raw_param_tab[i].param_id);
					ret = SENSOR_SUCCESS;
					break;
				}
				else{
					SENSOR_PRINT("identify module_id failed! table module_id=0x%x, otp module_id=0x%x",s_ov13850r2a_raw_param_tab[i].param_id,otp_info->module_id);
				}
			}
			else{
				SENSOR_PRINT("identify_otp failed!");
			}
		}
		else{
			SENSOR_PRINT("no identify_otp function!");
		}
	}

	if (SENSOR_SUCCESS == ret)
		return i;
	else
		return -1;
}

/*==============================================================================
 * Description:
 * apply otp parameters to sensor register
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t ov13850r2a_apply_otp(struct otp_info_t *otp_info, int id)
{
	uint32_t ret = SENSOR_FAIL;
	//apply otp parameters
	SENSOR_PRINT("otp_table_id = %d", id);
	if (PNULL != s_ov13850r2a_raw_param_tab[id].cfg_otp) {

		if(SENSOR_SUCCESS==s_ov13850r2a_raw_param_tab[id].cfg_otp(otp_info)){
			SENSOR_PRINT("apply otp parameters sucess! module_id=0x%x",s_ov13850r2a_raw_param_tab[id].param_id);
			ret = SENSOR_SUCCESS;
		}
		else{
			SENSOR_PRINT("update_otp failed!");
		}
	}else{
		SENSOR_PRINT("no update_otp function!");
	}

	return ret;
}

/*==============================================================================
 * Description:
 * cfg otp setting
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t ov13850r2a_cfg_otp(uint32_t param)
{
	uint32_t ret = SENSOR_FAIL;
	struct otp_info_t otp_info={0x00};
	int table_id = 0;

	table_id = ov13850r2a_get_otp_info(&otp_info);
	if (-1 != table_id)
		ret = ov13850r2a_apply_otp(&otp_info, table_id);
	return ret;
}
#endif

static uint32_t ov13850r2a_access_val(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_VAL_T* param_ptr = (SENSOR_VAL_T*)param;

	//Sensor_WriteReg(0x0103, 0x01);
	//usleep(5*1000);

	if(!param_ptr){
		#ifdef FEATURE_OTP
			rtn=ov13850r2a_cfg_otp(param);
		#endif

		if(rtn) {
			SENSOR_PRINT("SENSOR_OV13850R2A: _ov13850r2a_update_otp failed");
		}
		return rtn;
	}

	SENSOR_PRINT("SENSOR_OV13850R2A: param_ptr->type=%x", param_ptr->type);
	switch(param_ptr->type)
	{
		case SENSOR_VAL_TYPE_SHUTTER:
			*((uint32_t*)param_ptr->pval) = ov13850r2a_read_shutter();
			break;
		case SENSOR_VAL_TYPE_READ_OTP_GAIN:
			*((uint32_t*)param_ptr->pval) = ov13850r2a_read_gain();
			break;
		default:
			break;
	}

	return rtn;
}

/*==============================================================================
 * Description:
 * identify sensor id
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t ov13850r2a_identify(uint32_t param)
{
	uint16_t pid_value = 0x00;
	uint16_t ver_value = 0x00;
	uint16_t sub_ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("mipi raw identify");

	pid_value = Sensor_ReadReg(ov13850r2a_PID_ADDR);

	if (ov13850r2a_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(ov13850r2a_VER_ADDR);
		SENSOR_PRINT("Identify: PID = %x, VER = %x", pid_value, ver_value);

		if (ov13850r2a_VER_VALUE == ver_value) {
			sub_ver_value=Sensor_ReadReg(OV13850R2A_SUB_VER_ADDR);
			if(OV13850R2A_SUB_VER_VALUE==sub_ver_value)
			{
				#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
				ov13850r2a_InitRawTuneInfo();
				#endif
				ret_value = SENSOR_SUCCESS;
				SENSOR_PRINT_HIGH("this is ov13850r2a sensor");

			}
			else
			{
				SENSOR_PRINT_HIGH("Identify this is %x%x%x sensor", pid_value, ver_value,sub_ver_value);
				SENSOR_PRINT_HIGH("Identify this is not ov13850r2a");
			}

		} else {
			SENSOR_PRINT_HIGH("Identify this is %x%x sensor", pid_value, ver_value);
		}
	} else {
		SENSOR_PRINT_HIGH("identify fail, pid_value = %x", pid_value);
	}

	return ret_value;
}

/*==============================================================================
 * Description:
 * get resolution trim
 *
 *============================================================================*/
static unsigned long ov13850r2a_get_resolution_trim_tab(uint32_t param)
{
	return (unsigned long) s_ov13850r2a_resolution_trim_tab;
}

/*==============================================================================
 * Description:
 * before snapshot
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t ov13850r2a_before_snapshot(uint32_t param)
{
	uint32_t cap_shutter = 0;
	uint32_t prv_shutter = 0;
	uint32_t gain = 0;
	uint32_t cap_gain = 0;
	uint32_t capture_mode = param & 0xffff;
	uint32_t preview_mode = (param >> 0x10) & 0xffff;

	uint32_t prv_linetime = s_ov13850r2a_resolution_trim_tab[preview_mode].line_time;
	uint32_t cap_linetime = s_ov13850r2a_resolution_trim_tab[capture_mode].line_time;

	s_current_default_frame_length = ov13850r2a_get_default_frame_length(capture_mode);
	SENSOR_PRINT("capture_mode = %d", capture_mode);

	if (preview_mode == capture_mode) {
		cap_shutter = s_sensor_ev_info.preview_shutter;
		cap_gain = s_sensor_ev_info.preview_gain;
		goto snapshot_info;
	}

	prv_shutter = s_sensor_ev_info.preview_shutter;	//ov13850r2a_read_shutter();
	gain = s_sensor_ev_info.preview_gain;	//ov13850r2a_read_gain();

	Sensor_SetMode(capture_mode);
	Sensor_SetMode_WaitDone();

	cap_shutter = prv_shutter * prv_linetime / cap_linetime * BINNING_FACTOR;

	while (gain >= (2 * SENSOR_BASE_GAIN)) {
		if (cap_shutter * 2 > s_current_default_frame_length)
			break;
		cap_shutter = cap_shutter * 2;
		gain = gain / 2;
	}

	cap_shutter = ov13850r2a_update_exposure(cap_shutter,0);
	cap_gain = gain;
	ov13850r2a_write_gain(cap_gain);
	SENSOR_PRINT("preview_shutter = 0x%x, preview_gain = 0x%x",
		     s_sensor_ev_info.preview_shutter, s_sensor_ev_info.preview_gain);

	SENSOR_PRINT("capture_shutter = 0x%x, capture_gain = 0x%x", cap_shutter, cap_gain);
snapshot_info:
	s_hdr_info.capture_shutter = cap_shutter; //ov13850r2a_read_shutter();
	s_hdr_info.capture_gain = cap_gain; //ov13850r2a_read_gain();
	/* limit HDR capture min fps to 10;
	 * MaxFrameTime = 1000000*0.1us;
	 */
	s_hdr_info.capture_max_shutter = 1000000 / cap_linetime;

	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, cap_shutter);

	return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * get the shutter from isp
 * please don't change this function unless it's necessary
 *============================================================================*/
static uint32_t ov13850r2a_write_exposure(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t exposure_line = 0x00;
	uint16_t dummy_line = 0x00;
	uint16_t mode = 0x00;

	exposure_line = param & 0xffff;
	dummy_line = (param >> 0x10) & 0xfff; /*for cits frame rate test*/
	mode = (param >> 0x1c) & 0x0f;

	SENSOR_PRINT("current mode = %d, exposure_line = %d, dummy_line=%d", mode, exposure_line,dummy_line);
	s_current_default_frame_length = ov13850r2a_get_default_frame_length(mode);

	s_sensor_ev_info.preview_shutter = ov13850r2a_update_exposure(exposure_line,dummy_line);

	return ret_value;
}

/*==============================================================================
 * Description:
 * get the parameter from isp to real gain
 * you mustn't change the funcion !
 *============================================================================*/
static uint32_t isp_to_real_gain(uint32_t param)
{
	uint32_t real_gain = 0;


#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
	real_gain=param;
#else
	real_gain = ((param & 0xf) + 16) * (((param >> 4) & 0x01) + 1);
	real_gain = real_gain * (((param >> 5) & 0x01) + 1) * (((param >> 6) & 0x01) + 1);
	real_gain = real_gain * (((param >> 7) & 0x01) + 1) * (((param >> 8) & 0x01) + 1);
	real_gain = real_gain * (((param >> 9) & 0x01) + 1) * (((param >> 10) & 0x01) + 1);
	real_gain = real_gain * (((param >> 11) & 0x01) + 1);
#endif

	return real_gain;
}



static uint32_t ov13850r2a_write_gain_value(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint32_t real_gain = 0;

	real_gain = isp_to_real_gain(param);

	real_gain = real_gain * SENSOR_BASE_GAIN / ISP_BASE_GAIN;

	SENSOR_PRINT("real_gain = 0x%x", real_gain);

	s_sensor_ev_info.preview_gain = real_gain;
	ov13850r2a_write_gain(real_gain);

	return ret_value;
}

#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
/*==============================================================================
 * Description:
 * write parameter to vcm
 * please add your VCM function to this function
 *============================================================================*/
static uint32_t ov13850r2a_write_af(uint32_t param)
{

	return dw9718s_write_af(param);

}
#endif

/*==============================================================================
 * Description:
 * increase gain or shutter for hdr
 *
 *============================================================================*/
static void ov13850r2a_increase_hdr_exposure(uint8_t ev_multiplier)
{
	uint32_t shutter_multiply = s_hdr_info.capture_max_shutter / s_hdr_info.capture_shutter;
	uint32_t gain = 0;

	if (0 == shutter_multiply)
		shutter_multiply = 1;

	if (shutter_multiply >= ev_multiplier) {
		ov13850r2a_update_exposure(s_hdr_info.capture_shutter * ev_multiplier,0);
		ov13850r2a_write_gain(s_hdr_info.capture_gain);
	} else {
		gain = s_hdr_info.capture_gain * ev_multiplier / shutter_multiply;
		ov13850r2a_update_exposure(s_hdr_info.capture_shutter * shutter_multiply,0);
		ov13850r2a_write_gain(gain);
	}
}

/*==============================================================================
 * Description:
 * decrease gain or shutter for hdr
 *
 *============================================================================*/
static void ov13850r2a_decrease_hdr_exposure(uint8_t ev_divisor)
{
	uint16_t gain_multiply = 0;
	uint32_t shutter = 0;
	gain_multiply = s_hdr_info.capture_gain / SENSOR_BASE_GAIN;

	if (gain_multiply >= ev_divisor) {
		ov13850r2a_update_exposure(s_hdr_info.capture_shutter,0);
		ov13850r2a_write_gain(s_hdr_info.capture_gain / ev_divisor);

	} else {
		shutter = s_hdr_info.capture_shutter * gain_multiply / ev_divisor;
		ov13850r2a_update_exposure(shutter,0);
		ov13850r2a_write_gain(s_hdr_info.capture_gain / gain_multiply);
	}
}

/*==============================================================================
 * Description:
 * set hdr ev
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t ov13850r2a_set_hdr_ev(unsigned long param)
{
	uint32_t ret = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	uint32_t ev = ext_ptr->param;
	uint8_t ev_divisor, ev_multiplier;

	switch (ev) {
	case SENSOR_HDR_EV_LEVE_0:
		ev_divisor = 2;
		ov13850r2a_decrease_hdr_exposure(ev_divisor);
		break;
	case SENSOR_HDR_EV_LEVE_1:
		ev_multiplier = 2;
		ov13850r2a_increase_hdr_exposure(ev_multiplier);
		break;
	case SENSOR_HDR_EV_LEVE_2:
		ev_multiplier = 1;
		ov13850r2a_increase_hdr_exposure(ev_multiplier);
		break;
	default:
		break;
	}
	return ret;
}

/*==============================================================================
 * Description:
 * extra functoin
 * you can add functions reference SENSOR_EXT_FUNC_CMD_E which from sensor_drv_u.h
 *============================================================================*/
static uint32_t ov13850r2a_ext_func(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	SENSOR_PRINT("ext_ptr->cmd: %d", ext_ptr->cmd);
	switch (ext_ptr->cmd) {
	case SENSOR_EXT_EV:
		rtn = ov13850r2a_set_hdr_ev(param);
		break;
	default:
		break;
	}

	return rtn;
}

/*==============================================================================
 * Description:
 * mipi stream on
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t ov13850r2a_stream_on(uint32_t param)
{
	SENSOR_PRINT("E");

	Sensor_WriteReg(0x0100, 0x01);
	usleep(20*1000);

	return 0;
}

/*==============================================================================
 * Description:
 * mipi stream off
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t ov13850r2a_stream_off(uint32_t param)
{
	SENSOR_PRINT("E");

	Sensor_WriteReg(0x0100, 0x00);
	/*delay*/
	usleep(150 * 1000);

	return 0;
}

/*==============================================================================
 * Description:
 * all ioctl functoins
 * you can add functions reference SENSOR_IOCTL_FUNC_TAB_T from sensor_drv_u.h
 *
 * add ioctl functions like this:
 * .power = ov13850r2a_power_on,
 *============================================================================*/
static SENSOR_IOCTL_FUNC_TAB_T s_ov13850r2a_ioctl_func_tab = {
	.power = ov13850r2a_power_on,
	.identify = ov13850r2a_identify,
	.get_trim = ov13850r2a_get_resolution_trim_tab,
	.before_snapshort = ov13850r2a_before_snapshot,
	.write_ae_value = ov13850r2a_write_exposure,
	.write_gain_value = ov13850r2a_write_gain_value,
	#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
	.af_enable = ov13850r2a_write_af,
	#endif
	.set_focus = ov13850r2a_ext_func,
	//.set_video_mode = ov13850r2a_set_video_mode,
	.stream_on = ov13850r2a_stream_on,
	.stream_off = ov13850r2a_stream_off,
	.cfg_otp=ov13850r2a_access_val,

	//.group_hold_on = ov13850r2a_group_hold_on,
	//.group_hold_of = ov13850r2a_group_hold_off,
};
