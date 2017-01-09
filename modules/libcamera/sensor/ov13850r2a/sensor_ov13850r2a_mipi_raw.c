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
#include "newparam/sensor_ov13850r2a_raw_param_main.c"
#else
#endif
#include "sensor_ov13850r2a_otp.c"

#define OV13850R2A_I2C_ADDR_W         (0x36)
#define OV13850R2A_I2C_ADDR_R         (0x36)

#define OV13850_FLIP_MIRROR
#define DW9718S_VCM_SLAVE_ADDR (0x18>>1)

//#define use_sensor_gain

#define OV13850_RAW_PARAM_COM  0x0000
#define OV13850_RAW_PARAM_GLOBAL  0x0004
#define OV13850_OTP_CASE 		3
#define OV13850_UPDATE_LNC		1
#define OV13850_UPDATE_WB		2
#define OV13850_UPDATE_LNC_WB	3
#define OV13850_UPDATE_VCM		4

#define OV13850_MIN_FRAME_LEN_PRV  0x5e8
static int s_ov13850r2a_gain = 0;
static int s_capture_shutter = 0;
static int s_capture_VTS = 0;
static int s_video_min_framerate = 0;
static int s_video_max_framerate = 0;
static int s_exposure_time = 0;

LOCAL unsigned long _ov13850r2a_GetResolutionTrimTab(unsigned long param);
LOCAL unsigned long _ov13850r2a_PowerOn(unsigned long power_on);
LOCAL unsigned long _ov13850r2a_Identify(unsigned long param);
LOCAL unsigned long _ov13850r2a_BeforeSnapshot(unsigned long param);
LOCAL unsigned long _ov13850r2a_after_snapshot(unsigned long param);
LOCAL unsigned long _ov13850r2a_StreamOn(unsigned long param);
LOCAL unsigned long _ov13850r2a_StreamOff(unsigned long param);
LOCAL unsigned long _ov13850r2a_write_exposure(unsigned long param);
LOCAL unsigned long _ov13850r2a_write_gain(unsigned long param);
LOCAL unsigned long _ov13850r2a_write_af(unsigned long param);
LOCAL unsigned long _ov13850r2a_flash(unsigned long param);
LOCAL unsigned long _ov13850r2a_ExtFunc(unsigned long ctl_param);
LOCAL unsigned long _dw9718s_SRCInit(unsigned long mode);
LOCAL uint32_t _ov13850r2a_get_VTS(void);
LOCAL uint32_t _ov13850r2a_set_VTS(int VTS);
LOCAL unsigned long _ov13850r2a_ReadGain(unsigned long param);
LOCAL unsigned long _ov13850r2a_set_video_mode(unsigned long param);
LOCAL uint32_t _ov13850r2a_get_shutter(void);
LOCAL uint32_t _ov13850r2a_com_Identify_otp(void* param_ptr);
LOCAL unsigned long _ov13850r2a_cfg_otp(unsigned long  param);
LOCAL uint32_t _ov13850r2a_read_otp_gain(uint32_t *param);
LOCAL uint32_t _ov13850r2a_write_otp_gain(uint32_t *param);
LOCAL unsigned long _ov13850r2a_access_val(unsigned long param);
static unsigned long _ov13850r2a_GetExifInfo(unsigned long param);

LOCAL uint32_t _ov13850r2a_Global_Identify_otp(void* param_ptr);
LOCAL uint32_t _ov13850r2a_update_otp(void* param_ptr);

LOCAL const struct raw_param_info_tab s_ov13850r2a_raw_param_tab[]={
	{OV13850_RAW_PARAM_COM, &s_ov13850r2a_mipi_raw_info, _ov13850r2a_Global_Identify_otp, _ov13850r2a_update_otp},
	{OV13850_RAW_PARAM_COM, &s_ov13850r2a_mipi_raw_info, _ov13850r2a_com_Identify_otp, PNULL},
	{RAW_INFO_END_ID, PNULL, PNULL, PNULL}
};

struct sensor_raw_info* s_ov13850r2a_mipi_raw_info_ptr=NULL;

static uint32_t g_module_id = 0;

static uint32_t g_flash_mode_en = 0;
static uint32_t g_af_slewrate = 1;

LOCAL const SENSOR_REG_T ov13850r2a_common_init[] =
{
	//;XVCLK=24Mhz, SCLK=4x60Mhz, MIPI 640Mbps, DACCLK=240Mhz
	{0x0103, 0x01},
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
	{0x3835, 0x1c},
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
	{0x403f, 0x40},
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
	{0x5000, 0x0e},
	{0x5001, 0x01},
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

LOCAL const SENSOR_REG_T ov13850r2a_2112x1568_setting[] =
{
};

LOCAL const SENSOR_REG_T ov13850r2a_4208x3120_setting[] =
{
	//;XVCLK=24Mhz, SCLK=4x112Mhz, MIPI 1200Mbps, DACCLK=252Mhz
	{0x0100, 0x00},
	{0x0300, 0x00},
	{0x0302, 0x32},
	{0x3612, 0x08},
	{0x3614, 0x2a},
	{0x370a, 0x24},
	{0x371b, 0x01},
	{0x372a, 0x05},
	{0x3730, 0x02},
	{0x3731, 0x5c},
	{0x3732, 0x02},
	{0x3733, 0x70},
	{0x3718, 0x10},
	{0x3738, 0x02},
	{0x3739, 0x72},
	{0x373a, 0x02},
	{0x373b, 0x74},
	{0x3748, 0x21},
	{0x3749, 0x22},
	{0x3780, 0x90},
	{0x3781, 0x00},
	{0x3782, 0x01},
	{0x3801, 0x0c},
	{0x3805, 0x93},
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
	{0x4837, 0x0d},
	{0x5401, 0x71},
	{0x5405, 0x80},
	//{0x0100, 0x01},
	{0x0100, 0x00},
};

LOCAL const SENSOR_REG_T ov13850r2a_4224x3136_setting[] =
{
	//;XVCLK=24Mhz, SCLK=4x112Mhz, MIPI 1200Mbps, DACCLK=252Mhz
	{0x0100, 0x00},
	{0x0300, 0x00},
	{0x0302, 0x32},
	{0x3612, 0x08},
	{0x3614, 0x2a},
	{0x370a, 0x24},
	{0x371b, 0x01},
	{0x372a, 0x05},
	{0x3730, 0x02},
	{0x3731, 0x5c},
	{0x3732, 0x02},
	{0x3733, 0x70},
	{0x3718, 0x10},
	{0x3738, 0x02},
	{0x3739, 0x72},
	{0x373a, 0x02},
	{0x373b, 0x74},
	{0x3748, 0x21},
	{0x3749, 0x22},
	{0x3780, 0x90},
	{0x3781, 0x00},
	{0x3782, 0x01},
	{0x3801, 0x0c},
	{0x3805, 0x93},
	{0x3808, 0x10},
	{0x3809, 0x80},
	{0x380a, 0x0c},
	{0x380b, 0x40},
	{0x380c, 0x11},
	{0x380d, 0xa0},
	{0x380e, 0x0d},
	{0x380f, 0x00},
	{0x3811, 0x04},
	{0x3813, 0x04},
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
	{0x4837, 0x0d},
	{0x5401, 0x71},
	{0x5405, 0x80},
	//{0x0100, 0x01},
	{0x0100, 0x00},
};

LOCAL SENSOR_REG_TAB_INFO_T s_ov13850r2a_resolution_Tab_RAW[] = {
	{ADDR_AND_LEN_OF_ARRAY(ov13850r2a_common_init), 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},
	//{ADDR_AND_LEN_OF_ARRAY(ov13850r2a_2112x1568_setting), 2112, 1568, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(ov13850r2a_4208x3120_setting), 4208, 3120, 24, SENSOR_IMAGE_FORMAT_RAW},
	//{ADDR_AND_LEN_OF_ARRAY(ov13850r2a_4224x3136_setting), 4224, 3136, 24, SENSOR_IMAGE_FORMAT_RAW},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
    {PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

LOCAL SENSOR_TRIM_T s_ov13850r2a_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	//{0, 0,  2112, 1568, 200, 640, 1664, {0, 0,  2112, 1568}},  //vts
	{0, 0, 4208, 3120, 100, 1200, 3328, {0, 0, 4208, 3120}},
	//{0, 0, 4224, 3136, 100, 1200, 3328, {0, 0, 4224, 3136}},

	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
    {0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}}
};

LOCAL const SENSOR_REG_T s_ov13850r2a_2112x1568_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

LOCAL const SENSOR_REG_T  s_ov13850r2a_4208x3120_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

LOCAL SENSOR_VIDEO_INFO_T s_ov13850r2a_video_info[] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	//{{{30, 30, 200, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_ov13850r2a_2112x1568_video_tab},
	{{{15, 15, 200, 64}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_ov13850r2a_4208x3120_video_tab},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
        {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}
};

LOCAL unsigned long _ov13850r2a_set_video_mode(unsigned long param)
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

	if (PNULL == s_ov13850r2a_video_info[mode].setting_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR)&s_ov13850r2a_video_info[mode].setting_ptr[param];
	if (PNULL == sensor_reg_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	for (i=0x00; (0xffff!=sensor_reg_ptr[i].reg_addr)||(0xff!=sensor_reg_ptr[i].reg_value); i++) {
		Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	SENSOR_PRINT("0x%lx", param);
	return 0;
}

LOCAL SENSOR_IOCTL_FUNC_TAB_T s_ov13850r2a_ioctl_func_tab = {
	PNULL,
	_ov13850r2a_PowerOn,
	PNULL,
	_ov13850r2a_Identify,

	PNULL,			// write register
	PNULL,			// read  register
	PNULL,
	_ov13850r2a_GetResolutionTrimTab,

	// External
	PNULL,
	PNULL,
	PNULL,

	PNULL, //_ov13850r2a_set_brightness,
	PNULL, // _ov13850r2a_set_contrast,
	PNULL,
	PNULL,			//_ov13850r2a_set_saturation,

	PNULL, //_ov13850r2a_set_work_mode,
	PNULL, //_ov13850r2a_set_image_effect,

	_ov13850r2a_BeforeSnapshot,
	_ov13850r2a_after_snapshot,
	_ov13850r2a_flash,
	PNULL,
	_ov13850r2a_write_exposure,
	PNULL,
	_ov13850r2a_write_gain,
	PNULL,
	PNULL,
	_ov13850r2a_write_af,
	PNULL,
	PNULL, //_ov13850r2a_set_awb,
	PNULL,
	PNULL,
	PNULL, //_ov13850r2a_set_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL, //_ov13850r2a_GetExifInfo,
	_ov13850r2a_ExtFunc,
	PNULL, //_ov13850r2a_set_anti_flicker,
	_ov13850r2a_set_video_mode,
	PNULL, //pick_jpeg_stream
	PNULL,  //meter_mode
	PNULL, //get_status
	_ov13850r2a_StreamOn,
	_ov13850r2a_StreamOff,
	_ov13850r2a_access_val,
};


SENSOR_INFO_T g_ov13850r2a_mipi_raw_info = {
	OV13850R2A_I2C_ADDR_W,	// salve i2c write address
	OV13850R2A_I2C_ADDR_R,	// salve i2c read address

	SENSOR_I2C_REG_16BIT | SENSOR_I2C_REG_8BIT | SENSOR_I2C_FREQ_400,	// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
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
	{{0x300A, 0xD8},		// supply two code to identify sensor.
	 {0x300B, 0x50}},		// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,	// voltage of avdd

	4224,			// max width of source image
	3136,			// max height of source image
	"ov13850r2a",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_B,// pattern of input image form sensor;

	s_ov13850r2a_resolution_Tab_RAW,	// point to resolution table information structure
	&s_ov13850r2a_ioctl_func_tab,	// point to ioctl function table
	&s_ov13850r2a_mipi_raw_info_ptr,		// information and table about Rawrgb sensor
	NULL,			//&g_ov13850r2a_ext_info,                // extend information about sensor
	SENSOR_AVDD_1800MV,	// iovdd
	SENSOR_AVDD_1200MV,	// dvdd
	0,			// skip frame num before preview
	1,			// skip frame num before capture
	0,			// deci frame num during preview
	0,			// deci frame num during video preview

	0,
	0,
	0,
	0,
	0,
	{SENSOR_INTERFACE_TYPE_CSI2, 4, 10, 0},
	s_ov13850r2a_video_info,
	3,			// skip frame num while change setting
};

LOCAL struct sensor_raw_info* Sensor_GetContext(void)
{
	return s_ov13850r2a_mipi_raw_info_ptr;
}

/*
static struct sensor_libuse_info s_ov13850r2a_libuse_info=
{
#ifdef CONFIG_USE_ALC_AWB
	{
		0x00000080, //use ALC AWB
		0x0,
		0x0,
		0x0,
		{0x0}
	},
#else
	{
		0x00000000, //use sprd
		0x0,
		0x0,
		0x0,
		{0x0}
	},
#endif
#ifdef CONFIG_USE_ALC_AE
	{
		0x00000080, //use ALC AE
		0x0,
		0x0,
		0x0,
		{0x0}
	},
#endif
};
*/
#define param_update(x1,x2) sprintf(name,"/data/ov13850r2a_%s.bin",x1);\
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

#define param_update_v1(x1,x2,len) sprintf(name,"/data/ov13850r2a_%s.bin",x1);\
				if(0==access(name,R_OK))\
				{\
					FILE* fp = NULL;\
					SENSOR_PRINT("param file %s exists",name);\
					if( NULL!=(fp=fopen(name,"rb")) ){\
						fread((void*)x2,1,len,fp);\
						fclose(fp);\
					}else{\
						SENSOR_PRINT("param open %s failure",name);\
					}\
				}\
				memset(name,0,sizeof(name))

LOCAL uint32_t Sensor_ov13850r2a_InitRawTuneInfo(void)
{
	uint32_t rtn=0x00;

	isp_raw_para_update_from_file(&g_ov13850r2a_mipi_raw_info,0);

	struct sensor_raw_info* raw_sensor_ptr=Sensor_GetContext();
	struct isp_mode_param* mode_common_ptr = raw_sensor_ptr->mode_ptr[0].addr;
	int i;
	char name[100] = {'\0'};

	//raw_sensor_ptr->libuse_info = &s_ov13850r2a_libuse_info;
	
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
		case ISP_BLK_SFT_AF:{
				uint8_t* sft_af_param = (uint8_t*)data;
				param_update_v1("sft_af_param",sft_af_param,header->size);
			}
			break;

		default:
			break;
		}
	}


	return rtn;
}

LOCAL unsigned long _ov13850r2a_GetResolutionTrimTab(unsigned long param)
{
	SENSOR_PRINT("0x%lx",  (unsigned long)s_ov13850r2a_Resolution_Trim_Tab);
	return (unsigned long) s_ov13850r2a_Resolution_Trim_Tab;
}

LOCAL unsigned long _ov13850r2a_PowerOn(unsigned long power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_ov13850r2a_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_ov13850r2a_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_ov13850r2a_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_ov13850r2a_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_ov13850r2a_mipi_raw_info.reset_pulse_level;
	//uint32_t reset_width=g_ov13850r2a_yuv_info.reset_pulse_width;

	if (SENSOR_TRUE == power_on) {
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_SetResetLevel(reset_level);
		Sensor_PowerDown(power_down);
		usleep(2*1000);

		// Open power
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		usleep(1000);
		Sensor_SetIovddVoltage(iovdd_val);
		usleep(1000);
		Sensor_SetAvddVoltage(avdd_val);
		usleep(1000);
		Sensor_SetDvddVoltage(dvdd_val);

		usleep(10*1000);
		Sensor_PowerDown(!power_down);
		usleep(1000);
		Sensor_SetResetLevel(!reset_level);
		usleep(2*1000);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(2*1000);
		_dw9718s_SRCInit(2);

	} else {
		usleep(1000);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		usleep(1000);
		Sensor_SetResetLevel(reset_level);
		Sensor_PowerDown(power_down);
		usleep(1000);
		Sensor_SetDvddVoltage(SENSOR_AVDD_CLOSED);
		usleep(1000);
		Sensor_SetIovddVoltage(SENSOR_AVDD_CLOSED);
		Sensor_SetAvddVoltage(SENSOR_AVDD_CLOSED);

		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
	}
	SENSOR_PRINT("SENSOR_OV13850R2A: _ov13850r2a_Power_On(1:on, 0:off): %ld", power_on);
	return SENSOR_SUCCESS;
}

#if 1
LOCAL uint32_t _ov13850r2a_update_otp(void* param_ptr)
{
	uint16_t stream_value = 0;
	uint32_t rtn = SENSOR_FAIL;

	stream_value = Sensor_ReadReg(0x0100);
	SENSOR_PRINT("stream_value = 0x%x  OV13850_OTP_CASE = %d\n", stream_value, OV13850_OTP_CASE);
	if(1 != (stream_value & 0x01))
	{
		Sensor_WriteReg(0x0100, 0x01);
		usleep(50 * 1000);
	}

	switch(OV13850_OTP_CASE){
		case OV13850_UPDATE_LNC:
			rtn = ov13850r2a_update_otp_lenc();
			break;
		case OV13850_UPDATE_WB:
			rtn = ov13850r2a_update_otp_wb();
			break;

		case OV13850_UPDATE_LNC_WB:
			rtn = ov13850r2a_update_otp_lenc();
			rtn = ov13850r2a_update_otp_wb();
			break;

		case OV13850_UPDATE_VCM:

			break;
		default:
			break;
	}

	if(1 != (stream_value & 0x01))
		Sensor_WriteReg(0x0100, stream_value);

	return rtn;
}
#endif

LOCAL unsigned long _ov13850r2a_cfg_otp(unsigned long  param)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_ov13850r2a_raw_param_tab;
	uint32_t module_id=g_module_id;

	SENSOR_PRINT("SENSOR_OV13850R2A: _ov13850r2a_cfg_otp");

	if(PNULL!=tab_ptr[module_id].cfg_otp){
		tab_ptr[module_id].cfg_otp(0);
		}

	return rtn;
}


LOCAL unsigned long _ov13850r2a_access_val(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_VAL_T* param_ptr = (SENSOR_VAL_T*)param;
	uint16_t tmp;

	SENSOR_PRINT("SENSOR_OV13850R2A: cfg_otp E");
	if(!param_ptr){
		rtn = _ov13850r2a_update_otp(0);
		if(rtn) {
			SENSOR_PRINT("SENSOR_OV13850R2A: _ov13850r2a_update_otp failed");
		}
		return rtn;
	}

	SENSOR_PRINT("SENSOR_OV13850R2A: param_ptr->type=%x", param_ptr->type);
	switch(param_ptr->type)
	{
		case SENSOR_VAL_TYPE_SHUTTER:
			*((uint32_t*)param_ptr->pval) = _ov13850r2a_get_shutter();
			break;
		case SENSOR_VAL_TYPE_READ_OTP_GAIN:
			rtn = _ov13850r2a_read_otp_gain(param_ptr->pval);
			break;
		default:
			break;
	}

	SENSOR_PRINT("SENSOR_OV13850R2A: cfg_otp X");

	return rtn;
}


#if 1
LOCAL uint32_t _ov13850r2a_Global_Identify_otp(void* param_ptr)
{
	struct otp_struct current_otp;
	uint32_t temp = 0;
	uint16_t stream_value = 0;
	uint32_t rtn=SENSOR_FAIL;

	stream_value = Sensor_ReadReg(0x0100);
	SENSOR_PRINT("stream_value = 0x%x\n", stream_value);
	if(1 != (stream_value & 0x01))
	{
		Sensor_WriteReg(0x0100, 0x01);
		usleep(50 * 1000);
	}

	// R/G and B/G of current camera module is read out from sensor OTP
	// check first OTP with valid data
	temp = ov13850r2a_read_otp_info(&current_otp);
	if(OV13850_RAW_PARAM_GLOBAL == current_otp.module_integrator_id){
		rtn=SENSOR_SUCCESS;
		SENSOR_PRINT("This is OV13850 Global Module ! index = %d\n", current_otp.index);
	} else {
		SENSOR_PRINT("ov13850r2a_check_otp_module_id no valid wb OTP data\n");
	}

	if(1 != (stream_value & 0x01))
		Sensor_WriteReg(0x0100, stream_value);

	SENSOR_PRINT("read ov13850r2a otp  module_id = %x \n", current_otp.module_integrator_id);

	return rtn;
}
#endif

LOCAL uint32_t _ov13850r2a_com_Identify_otp(void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_OV13850R2A: _ov13850r2a_com_Identify_otp");

	/*read param id from sensor omap*/
	param_id=OV13850_RAW_PARAM_COM;

	if(OV13850_RAW_PARAM_COM==param_id){
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}

LOCAL uint32_t _ov13850r2a_GetRawInof(void)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_ov13850r2a_raw_param_tab;
	uint32_t param_id;
	uint32_t i=0x00;

	/*read param id from sensor omap*/
	param_id=OV13850_RAW_PARAM_COM;

	for(i=0x00; ; i++)
	{
		g_module_id = i;
		if(RAW_INFO_END_ID==tab_ptr[i].param_id){
			if(NULL==s_ov13850r2a_mipi_raw_info_ptr){
				SENSOR_PRINT("SENSOR_OV13850R2A: ov13850r2a_GetRawInof no param error");
				rtn=SENSOR_FAIL;
			}
			SENSOR_PRINT("SENSOR_OV13850R2A: ov13850r2a_GetRawInof end");
			break;
		}
		else if(PNULL!=tab_ptr[i].identify_otp){
			if(SENSOR_SUCCESS==tab_ptr[i].identify_otp(0))
			{
				s_ov13850r2a_mipi_raw_info_ptr = tab_ptr[i].info_ptr;
				SENSOR_PRINT("SENSOR_OV13850R2A: ov13850r2a_GetRawInof success");
				break;
			}
		}
	}

	return rtn;
}

LOCAL unsigned long _ov13850r2a_GetMaxFrameLine(unsigned long index)
{
	uint32_t max_line=0x00;
	SENSOR_TRIM_T_PTR trim_ptr=s_ov13850r2a_Resolution_Trim_Tab;

	max_line=trim_ptr[index].frame_line;

	return max_line;
}

LOCAL unsigned long _ov13850r2a_Identify(unsigned long param)
{
#define OV13850R2A_PID_VALUE_H    0xD8
#define OV13850R2A_PID_ADDR_H     0x300A
#define OV13850R2A_PID_VALUE_L    0x50
#define OV13850R2A_PID_ADDR_L     0x300B
#define OV13850R2A_VER_VALUE      0xB2
#define OV13850R2A_VER_ADDR       0x302A

	uint8_t pid_value_h = 0x00;
	uint8_t pid_value_l = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("SENSOR_OV13850R2A: mipi raw identify\n");

	pid_value_h = Sensor_ReadReg(OV13850R2A_PID_ADDR_H);
	pid_value_l = Sensor_ReadReg(OV13850R2A_PID_ADDR_L);
	if (OV13850R2A_PID_VALUE_H == pid_value_h && OV13850R2A_PID_VALUE_L == pid_value_l) {
		ver_value = Sensor_ReadReg(OV13850R2A_VER_ADDR);
		SENSOR_PRINT("SENSOR_OV13850R2A: Identify: PID = %x%x, VER = %x", pid_value_h, pid_value_l, ver_value);
		if (OV13850R2A_VER_VALUE == ver_value) {
			SENSOR_PRINT("SENSOR_OV13850R2A: this is ov13850r2a %x sensor !", ver_value);
			ret_value=_ov13850r2a_GetRawInof();
			if(SENSOR_SUCCESS != ret_value)
			{
				SENSOR_PRINT_ERR("SENSOR_OV13850R2A: the module is unknow error !");
			}
			Sensor_ov13850r2a_InitRawTuneInfo();
		} else {
			SENSOR_PRINT_HIGH("SENSOR_OV13850R2A: Identify this is OV%x%x %x sensor !", pid_value_h, pid_value_l, ver_value);
		}
	} else {
		SENSOR_PRINT_ERR("SENSOR_OV13850R2A: identify fail,pid_value_h=%x, pid_value_l=%x", pid_value_h, pid_value_l);
	}

	return ret_value;
}

LOCAL uint32_t ov13850_get_shutter()
{
	// read shutter, in number of line period
	uint32_t shutter = 0;

	shutter = (Sensor_ReadReg(0x03500) & 0x0f);
	shutter = (shutter<<8) + Sensor_ReadReg(0x3501);
	shutter = (shutter<<4) + (Sensor_ReadReg(0x3502)>>4);

	return shutter;
}

LOCAL unsigned long  ov13850_set_shutter(unsigned long shutter)
{
	// write shutter, in number of line period
	uint16_t temp = 0;

	shutter = shutter & 0xffff;
	temp = shutter & 0x0f;
	temp = temp<<4;
	Sensor_WriteReg(0x3502, temp);
	temp = shutter & 0xfff;
	temp = temp>>4;
	Sensor_WriteReg(0x3501, temp);
	temp = (shutter>>12) & 0xf;
	Sensor_WriteReg(0x3500, temp);

	return 0;
}

LOCAL uint32_t _ov13850_get_VTS(void)
{
	// read VTS from register settings
	uint32_t VTS;

	VTS = Sensor_ReadReg(0x380e);//total vertical size[15:8] high byte
	VTS = ((VTS & 0x7f)<<8) + Sensor_ReadReg(0x380f);

	return VTS;
}

LOCAL unsigned long ov13850_set_VTS(unsigned long VTS)
{
	// write VTS to registers
	uint16_t temp = 0;

	temp = VTS & 0xff;
	Sensor_WriteReg(0x380f, temp);
	temp = (VTS>>8) & 0x7f;
	Sensor_WriteReg(0x380e, temp);

	return 0;
}

//sensor gain
LOCAL uint16_t ov13850_get_sensor_gain16()
{
	// read sensor gain, 16 = 1x
	uint16_t gain16 = 0;

	gain16 = Sensor_ReadReg(0x350a) & 0x03;
	gain16 = (gain16<<8) + Sensor_ReadReg( 0x350b);
	gain16 = ((gain16 >> 4) + 1)*((gain16 & 0x0f) + 16);

	return gain16;
}

LOCAL uint16_t ov13850_set_sensor_gain16(int gain16)
{
	// write sensor gain, 16 = 1x
	int gain_reg = 0;
	uint16_t temp = 0;
	int i = 0;

	if(gain16 > 0x7ff)
	{
		gain16 = 0x7ff;
	}

	for(i=0; i<5; i++) {
		if (gain16>31) {
			gain16 = gain16/2;
			gain_reg = gain_reg | (0x10<<i);
		}
		else
			break;
	}
	gain_reg = gain_reg | (gain16 - 16);
	temp = gain_reg & 0xff;
	Sensor_WriteReg(0x350b, temp);
	temp = gain_reg>>8;
	Sensor_WriteReg(0x350a, temp);
	return 0;
}

LOCAL unsigned long _ov13850r2a_write_exposure(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t expsure_line=0x00;
	uint16_t dummy_line=0x00;
	uint16_t size_index=0x00;
	uint16_t frame_len=0x00;
	uint16_t frame_len_cur=0x00;
	uint16_t max_frame_len=0x00;
	uint16_t value=0x00;
	uint16_t value0=0x00;
	uint16_t value1=0x00;
	uint16_t value2=0x00;
	uint32_t linetime = 0;

	expsure_line=param&0xffff;
	dummy_line=(param>>0x10)&0x0fff;
	size_index=(param>>0x1c)&0x0f;

	SENSOR_PRINT("SENSOR_OV13850R2A: write_exposure line:%d, dummy:%d, size_index:%d", expsure_line, dummy_line, size_index);

	max_frame_len=_ov13850r2a_GetMaxFrameLine(size_index);
	if(0x00!=max_frame_len)
	{
		frame_len = ((expsure_line+8)> max_frame_len) ? (expsure_line+8) : max_frame_len;

		frame_len_cur = _ov13850_get_VTS();

		SENSOR_PRINT("SENSOR_OV13850R2A: frame_len: %d,   frame_len_cur:%d\n", frame_len, frame_len_cur);

		if(frame_len_cur != frame_len){
			ov13850_set_VTS(frame_len);
		}
	}
	ov13850_set_shutter(expsure_line);

	s_capture_shutter = expsure_line;
	linetime=s_ov13850r2a_Resolution_Trim_Tab[size_index].line_time;
	s_exposure_time = s_capture_shutter * linetime / 10;
	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, s_capture_shutter);

	return ret_value;
}

LOCAL unsigned long _ov13850r2a_write_gain(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint32_t real_gain = 0;

#ifdef use_sensor_gain
	SENSOR_PRINT("SENSOR_OV13850R2A: param: 0x%x", param);
	value = param & 0xff;
	Sensor_WriteReg(0x350b, value);
	value = (param>>8) & 0x7;
	Sensor_WriteReg(0x350a, value);

#else
#if 1
	real_gain = param>>3;
	if(real_gain > 0xf8)
	{
		real_gain = 0xf8;
	}
#else
	real_gain = ((param&0xf)+16)*(((param>>4)&0x01)+1)*(((param>>5)&0x01)+1)*(((param>>6)&0x01)+1)*(((param>>7)&0x01)+1);
	real_gain = real_gain*(((param>>8)&0x01)+1)*(((param>>9)&0x01)+1)*(((param>>10)&0x01)+1)*(((param>>11)&0x01)+1);
#endif
	SENSOR_PRINT("SENSOR_OV13850R2A: real_gain:0x%x, param: 0x%x", real_gain, param);

	value = real_gain & 0xff;
	Sensor_WriteReg(0x350b, value);
	value = (real_gain>>8) & 0x7;
	Sensor_WriteReg(0x350a, value);
#endif


	return ret_value;
}

LOCAL unsigned long _ov13850r2a_write_af(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint8_t cmd_val[2] = {0x00};
	uint16_t slave_addr = 0;
	uint16_t cmd_len = 0;

	SENSOR_PRINT("SENSOR_S5K4H5YC: _write_af %ld", param);
	slave_addr = DW9718S_VCM_SLAVE_ADDR;

	cmd_val[0] = 0x02;
	cmd_val[1] = (param>>8)&0x03;
	cmd_len = 2;
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
	SENSOR_PRINT("SENSOR_S5K4H5YC: _write_af, ret =  %d, MSL:%x, LSL:%x\n", ret_value, cmd_val[0], cmd_val[1]);

	cmd_val[0] = 0x03;
	cmd_val[1] = param&0xff;
	cmd_len = 2;
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

	SENSOR_PRINT("SENSOR_S5K4H5YC: _write_af, ret =  %d, MSL:%x, LSL:%x\n", ret_value, cmd_val[0], cmd_val[1]);
	return ret_value;

}

LOCAL unsigned long _ov13850r2a_BeforeSnapshot(unsigned long param)
{
	uint8_t ret_l, ret_m, ret_h;
	uint32_t capture_exposure, preview_maxline;
	uint32_t capture_maxline, preview_exposure;
	uint32_t capture_mode = param & 0xffff;
	uint32_t preview_mode = (param >> 0x10 ) & 0xffff;
	uint32_t prv_linetime=s_ov13850r2a_Resolution_Trim_Tab[preview_mode].line_time;
	uint32_t cap_linetime = s_ov13850r2a_Resolution_Trim_Tab[capture_mode].line_time;

	SENSOR_PRINT("SENSOR_OV13850R2A: BeforeSnapshot mode: 0x%08x",param);

	if (preview_mode == capture_mode) {
		SENSOR_PRINT("SENSOR_OV13850R2A: prv mode equal to capmode");
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

	SENSOR_PRINT("SENSOR_OV13850R2A: prv_linetime = %d   cap_linetime = %d\n", prv_linetime, cap_linetime);

	if (prv_linetime == cap_linetime) {
		SENSOR_PRINT("SENSOR_OV13850R2A: prvline equal to capline");
		//goto CFG_INFO;
	}

	ret_h = (uint8_t) Sensor_ReadReg(0x380e);
	ret_l = (uint8_t) Sensor_ReadReg(0x380f);
	capture_maxline = (ret_h << 8) + ret_l;

	capture_exposure = preview_exposure * prv_linetime/cap_linetime;
	//capture_exposure *= 2;

	if(0 == capture_exposure){
		capture_exposure = 1;
	}
	SENSOR_PRINT("SENSOR_OV13850R2A: capture_exposure = %d   capture_maxline = %d\n", capture_exposure, capture_maxline);

	if(capture_exposure > (capture_maxline - 4)){
		capture_maxline = capture_exposure + 4;
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
	usleep(200*1000);

CFG_INFO:
	s_capture_shutter = _ov13850r2a_get_shutter();
	s_capture_VTS = _ov13850r2a_get_VTS();
	_ov13850r2a_ReadGain(capture_mode);
	s_exposure_time = s_capture_shutter * cap_linetime / 10;
	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, s_capture_shutter);

	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov13850r2a_after_snapshot(unsigned long param)
{
	SENSOR_PRINT("SENSOR_OV13850R2A: after_snapshot mode:%ld", param);
	Sensor_SetMode((uint32_t)param);
	return SENSOR_SUCCESS;
}

static unsigned long _ov13850r2a_GetExifInfo(unsigned long param)
{
	LOCAL EXIF_SPEC_PIC_TAKING_COND_T sexif;

	sexif.ExposureTime.numerator = s_exposure_time;
	sexif.ExposureTime.denominator = 1000000;

	return (unsigned long) & sexif;
}


LOCAL unsigned long _ov13850r2a_flash(unsigned long param)
{
	SENSOR_PRINT("SENSOR_OV13850R2A: param=%d", param);

	/* enable flash, disable in _ov13850r2a_BeforeSnapshot */
	g_flash_mode_en = param;
	Sensor_SetFlash(param);
	SENSOR_PRINT("end");
	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov13850r2a_StreamOn(unsigned long param)
{
	SENSOR_PRINT("SENSOR_OV13850R2A: StreamOn");

	Sensor_WriteReg(0x0100, 0x01);

	return 0;
}

LOCAL unsigned long _ov13850r2a_StreamOff(unsigned long param)
{
	SENSOR_PRINT("SENSOR_OV13850R2A: StreamOff");

	Sensor_WriteReg(0x0100, 0x00);
	usleep(150*1000);

	return 0;
}

LOCAL uint32_t _ov13850r2a_get_shutter(void)
{
	// read shutter, in number of line period
	int shutter;

	shutter = (Sensor_ReadReg(0x03500) & 0x0f);
	shutter = (shutter<<8) + Sensor_ReadReg(0x3501);
	shutter = (shutter<<4) + (Sensor_ReadReg(0x3502)>>4);

	return shutter;
}

LOCAL uint32_t _ov13850r2a_set_shutter(int shutter)
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

	return 0;
}

LOCAL int _ov13850r2a_get_gain16(void)
{
	// read gain, 16 = 1x
	int gain16;

	gain16 = Sensor_ReadReg(0x350a) & 0x03;
	gain16 = (gain16<<8) + Sensor_ReadReg(0x350b);

	return gain16;
}

LOCAL int _ov13850r2a_set_gain16(int gain16)
{
	// write gain, 16 = 1x
	int temp;
	gain16 = gain16 & 0x3ff;

	temp = gain16 & 0xff;
	Sensor_WriteReg(0x350b, temp);

	temp = gain16>>8;
	Sensor_WriteReg(0x350a, temp);

	return 0;
}

static void _calculate_hdr_exposure(int capture_gain16,int capture_VTS, int capture_shutter)
{
	// write capture gain
	_ov13850r2a_set_gain16(capture_gain16);

	// write capture shutter
	/*if (capture_shutter > (capture_VTS - 4)) {
		capture_VTS = capture_shutter + 4;
		ov13850_set_VTS(capture_VTS);
	}*/
	_ov13850r2a_set_shutter(capture_shutter);
}

static unsigned long _ov13850r2a_SetEV(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	uint16_t value=0x00;
	uint32_t gain = s_ov13850r2a_gain;
	uint32_t ev = ext_ptr->param;

	SENSOR_PRINT("SENSOR_OV13850R2A: _ov13850r2a_SetEV param: 0x%x", ext_ptr->param);

	switch(ev) {
	case SENSOR_HDR_EV_LEVE_0:
		_calculate_hdr_exposure(s_ov13850r2a_gain,s_capture_VTS,s_capture_shutter/2);
		break;
	case SENSOR_HDR_EV_LEVE_1:
		_calculate_hdr_exposure(s_ov13850r2a_gain,s_capture_VTS,s_capture_shutter);
		break;
	case SENSOR_HDR_EV_LEVE_2:
		_calculate_hdr_exposure(s_ov13850r2a_gain,s_capture_VTS,s_capture_shutter *2);
		break;
	default:
		break;
	}
	usleep(50*1000);
	return rtn;
}
LOCAL unsigned long _ov13850r2a_ExtFunc(unsigned long ctl_param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr =
	    (SENSOR_EXT_FUN_PARAM_T_PTR) ctl_param;
	SENSOR_PRINT("0x%x", ext_ptr->cmd);

	switch (ext_ptr->cmd) {
	case SENSOR_EXT_FUNC_INIT:
		break;
	case SENSOR_EXT_FOCUS_START:
		break;
	case SENSOR_EXT_EXPOSURE_START:
		break;
	case SENSOR_EXT_EV:
		rtn = _ov13850r2a_SetEV(ctl_param);
		break;
	default:
		break;
	}
	return rtn;
}
LOCAL uint32_t _ov13850r2a_get_VTS(void)
{
	// read VTS from register settings
	int VTS;

	VTS = Sensor_ReadReg(0x380e);//total vertical size[15:8] high byte

	VTS = (VTS<<8) + Sensor_ReadReg(0x380f);

	return VTS;
}

LOCAL uint32_t _ov13850r2a_set_VTS(int VTS)
{
	// write VTS to registers
	int temp;

	temp = VTS & 0xff;
	Sensor_WriteReg(0x380f, temp);

	temp = VTS>>8;
	Sensor_WriteReg(0x380e, temp);

	return 0;
}
LOCAL unsigned long _ov13850r2a_ReadGain(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint32_t gain = 0;

	value = Sensor_ReadReg(0x350b);/*0-7*/
	gain = value&0xff;
	value = Sensor_ReadReg(0x350a);/*8*/
	gain |= (value<<0x08)&0x300;

	s_ov13850r2a_gain=(int)gain;

	SENSOR_PRINT("SENSOR_OV13850R2A: _ov13850r2a_ReadGain gain: 0x%x", s_ov13850r2a_gain);

	return rtn;
}

LOCAL unsigned long _dw9718s_SRCInit(unsigned long mode)
{
	uint8_t cmd_val[2] = {0x00};
	uint16_t slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t ret_value = SENSOR_SUCCESS;
	SENSOR_PRINT("SENSOR_OV13850R2A: %d",mode);

	slave_addr = DW9718S_VCM_SLAVE_ADDR;
	switch (mode) {
	case 1:
		break;

	case 2:
	{
		cmd_len = 2;

		cmd_val[0] = 0x01;
		cmd_val[1] = 0x39;
		ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
		if(ret_value){
			SENSOR_PRINT("SENSOR_OV13850R2A: _dw9718s_SRCInit 1 fail!");
		}

		cmd_val[0] = 0x05;
		cmd_val[1] = 0x79;
		ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
		if(ret_value){
			SENSOR_PRINT("SENSOR_OV13850R2A: _dw9718s_SRCInit 5 fail!");
		}
	}
		break;
	case 3:
		break;

	}

	return ret_value;
}

LOCAL uint32_t _ov13850r2a_read_otp_gain(uint32_t *param)
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
