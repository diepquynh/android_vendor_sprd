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
#include "sensor_ov8858_raw_param.c"

#include "sensor_ov8858_r2a_otp.c"

#define USE_CNR_ONE_DIMEN_ARRAY
#ifdef USE_CNR_ONE_DIMEN_ARRAY
#include "sensor_ov8858_cnr.h"
#else
#include "sensor_ov8858_cnr_v01.h"
#endif

#define DW9714_VCM_SLAVE_ADDR (0x18>>1)
#define BU64241GWZ_VCM_SLAVE_ADDR (0x18 >> 1)

#define ov8858_I2C_ADDR_W        (0x6c>>1)
#define ov8858_I2C_ADDR_R         (0x6c>>1)

#define ov8858_RAW_PARAM_COM  0x0000

#define SENSOR_BASE_GAIN 			0x80 //0x10

#define ov8858_MIN_FRAME_LEN_PRV  0x5e8
#define ov8858_4_LANES
static int s_ov8858_gain = 0;
static int s_capture_shutter = 0;
static int s_capture_VTS = 0;
static int s_video_min_framerate = 0;
static int s_video_max_framerate = 0;

LOCAL unsigned long _ov8858_GetResolutionTrimTab(unsigned long param);
LOCAL unsigned long _ov8858_PowerOn(unsigned long power_on);
LOCAL unsigned long _ov8858_Identify(unsigned long param);
LOCAL unsigned long _ov8858_BeforeSnapshot(unsigned long param);
LOCAL unsigned long _ov8858_after_snapshot(unsigned long param);
LOCAL unsigned long _ov8858_StreamOn(unsigned long param);
LOCAL unsigned long _ov8858_StreamOff(unsigned long param);
LOCAL unsigned long _ov8858_write_exposure(unsigned long param);
LOCAL unsigned long _ov8858_ex_write_exposure(unsigned long param);
LOCAL unsigned long _ov8858_write_gain(unsigned long param);
LOCAL unsigned long _ov8858_read_gain_scale(unsigned long param);
LOCAL unsigned long _ov8858_write_af(unsigned long param);
LOCAL unsigned long _ov8858_flash(unsigned long param);
LOCAL unsigned long _ov8858_ExtFunc(unsigned long ctl_param);
LOCAL int _ov8858_get_VTS(void);
LOCAL int _ov8858_set_VTS(int VTS);
LOCAL uint32_t _ov8858_ReadGain(uint32_t param);
LOCAL unsigned long _ov8858_set_video_mode(unsigned long param);
LOCAL int _ov8858_get_shutter(void);
LOCAL unsigned long _ov8858_com_Identify_otp(void* param_ptr);
LOCAL uint32_t _ov8858_dw9714_SRCInit(uint32_t mode);
LOCAL unsigned long _ov8858_cfg_otp(unsigned long  param);
LOCAL unsigned long _ov8858_read_snapshot_gain(unsigned long  param);

LOCAL const struct raw_param_info_tab s_ov8858_raw_param_tab[]={
	{ov8858_RAW_PARAM_COM, &s_ov8858_mipi_raw_info, ov8858_otp_identify_otp, ov8858_otp_update_otp},
//	{ov8858_RAW_PARAM_COM, &s_ov8858_mipi_raw_info, _ov8858_com_Identify_otp, PNULL},
	{RAW_INFO_END_ID, PNULL, PNULL, PNULL}
};

struct sensor_raw_info* s_ov8858_mipi_raw_info_ptr=NULL;

static uint32_t g_module_id = 0;

static uint32_t g_flash_mode_en = 0;
static uint32_t g_af_slewrate = 1;

LOCAL const SENSOR_REG_T ov8858_common_init[] = {

//  {0x0103, 0x01},
//  {SENSOR_WRITE_DELAY, 0x0a},
  {0x0100, 0x00},
  {0x0302, 0x1e},
  {0x0303, 0x00},
  {0x0304, 0x03},
  {0x030e, 0x00},
  {0x030f, 0x04},
  {0x0312, 0x01},
  {0x031e, 0x0c},
  {0x3600, 0x00},
  {0x3601, 0x00},
  {0x3602, 0x00},
  {0x3603, 0x00},
  {0x3604, 0x22},
  {0x3605, 0x20},
  {0x3606, 0x00},
  {0x3607, 0x20},
  {0x3608, 0x11},
  {0x3609, 0x28},
  {0x360a, 0x00},
  {0x360b, 0x05},
  {0x360c, 0xd4},
  {0x360d, 0x40},
  {0x360e, 0x0c},
  {0x360f, 0x20},
  {0x3610, 0x07},
  {0x3611, 0x20},
  {0x3612, 0x88},
  {0x3613, 0x80},
  {0x3614, 0x58},
  {0x3615, 0x00},
  {0x3616, 0x4a},
  {0x3617, 0x40},
  {0x3618, 0x5a},
  {0x3619, 0x70},
  {0x361a, 0x99},
  {0x361b, 0x0a},
  {0x361c, 0x07},
  {0x361d, 0x00},
  {0x361e, 0x00},
  {0x361f, 0x00},
  {0x3638, 0xff},
  {0x3633, 0x0f},
  {0x3634, 0x0f},
  {0x3635, 0x0f},
  {0x3636, 0x12},
  {0x3645, 0x13},
  {0x3646, 0x83},
  {0x364a, 0x07},
  {0x3015, 0x01},
  {0x3018, 0x72},
  {0x3020, 0x93},
  {0x3022, 0x01},
  {0x3031, 0x0a},
  {0x3034, 0x00},
  {0x3106, 0x01},
  {0x3305, 0xf1},
  {0x3308, 0x00},
  {0x3309, 0x28},
  {0x330a, 0x00},
  {0x330b, 0x20},
  {0x330c, 0x00},
  {0x330d, 0x00},
  {0x330e, 0x00},
  {0x330f, 0x40},
  {0x3307, 0x04},
  {0x3500, 0x00},
  {0x3501, 0x9a},
  {0x3502, 0x20},
  {0x3503, 0x80},
  {0x3505, 0x80},
  {0x3508, 0x02},
  {0x3509, 0x00},
  {0x350c, 0x00},
  {0x350d, 0x80},
  {0x3510, 0x00},
  {0x3511, 0x02},
  {0x3512, 0x00},
  {0x3700, 0x30},
  {0x3701, 0x18},
  {0x3702, 0x50},
  {0x3703, 0x32},
  {0x3704, 0x28},
  {0x3705, 0x00},
  {0x3706, 0x82},
  {0x3707, 0x08},
  {0x3708, 0x48},
  {0x3709, 0x66},
  {0x370a, 0x01},
  {0x370b, 0x82},
  {0x370c, 0x07},
  {0x3718, 0x14},
  {0x3719, 0x31},
  {0x3712, 0x44},
  {0x3714, 0x24},
  {0x371e, 0x31},
  {0x371f, 0x7f},
  {0x3720, 0x0a},
  {0x3721, 0x0a},
  {0x3724, 0x0c},
  {0x3725, 0x02},
  {0x3726, 0x0c},
  {0x3728, 0x0a},
  {0x3729, 0x03},
  {0x372a, 0x06},
  {0x372b, 0xa6},
  {0x372c, 0xa6},
  {0x372d, 0xa6},
  {0x372e, 0x0c},
  {0x372f, 0x20},
  {0x3730, 0x02},
  {0x3731, 0x0c},
  {0x3732, 0x28},
  {0x3733, 0x10},
  {0x3734, 0x40},
  {0x3736, 0x30},
  {0x373a, 0x0a},
  {0x373b, 0x0b},
  {0x373c, 0x14},
  {0x373e, 0x06},
  {0x3750, 0x0a},
  {0x3751, 0x0e},
  {0x3755, 0x10},
  {0x3758, 0x00},
  {0x3759, 0x4c},
  {0x375a, 0x0c},
  {0x375b, 0x26},
  {0x375c, 0x20},
  {0x375d, 0x04},
  {0x375e, 0x00},
  {0x375f, 0x28},
  {0x3768, 0x22},
  {0x3769, 0x44},
  {0x376a, 0x44},
  {0x3761, 0x00},
  {0x3762, 0x00},
  {0x3763, 0x00},
  {0x3766, 0xff},
  {0x376b, 0x00},
  {0x3772, 0x46},
  {0x3773, 0x04},
  {0x3774, 0x2c},
  {0x3775, 0x13},
  {0x3776, 0x08},
  {0x3777, 0x00},
  {0x3778, 0x17},
  {0x37a0, 0x88},
  {0x37a1, 0x7a},
  {0x37a2, 0x7a},
  {0x37a3, 0x00},
  {0x37a4, 0x00},
  {0x37a5, 0x00},
  {0x37a6, 0x00},
  {0x37a7, 0x88},
  {0x37a8, 0x98},
  {0x37a9, 0x98},
  {0x3760, 0x00},
  {0x376f, 0x01},
  {0x37aa, 0x88},
  {0x37ab, 0x5c},
  {0x37ac, 0x5c},
  {0x37ad, 0x55},
  {0x37ae, 0x19},
  {0x37af, 0x19},
  {0x37b0, 0x00},
  {0x37b1, 0x00},
  {0x37b2, 0x00},
  {0x37b3, 0x84},
  {0x37b4, 0x84},
  {0x37b5, 0x60},
  {0x37b6, 0x00},
  {0x37b7, 0x00},
  {0x37b8, 0x00},
  {0x37b9, 0xff},
  {0x3800, 0x00},
  {0x3801, 0x0c},
  {0x3802, 0x00},
  {0x3803, 0x0c},
  {0x3804, 0x0c},
  {0x3805, 0xd3},
  {0x3806, 0x09},
  {0x3807, 0xa3},
  {0x3808, 0x0c},
  {0x3809, 0xc0},
  {0x380a, 0x09},
  {0x380b, 0x90},
  {0x380c, 0x07},
  {0x380d, 0x94},
  {0x380e, 0x09},
  {0x380f, 0xaa},
  {0x3810, 0x00},
  {0x3811, 0x04},
  {0x3813, 0x02},
  {0x3814, 0x01},
  {0x3815, 0x01},
  {0x3820, 0x00},
  {0x3821, 0x46},
  {0x382a, 0x01},
  {0x382b, 0x01},
  {0x3830, 0x06},
  {0x3836, 0x01},
  {0x3837, 0x18},
  {0x3841, 0xff},
  {0x3846, 0x48},
  {0x3d85, 0x16},
  {0x3d8c, 0x73},
  {0x3d8d, 0xde},
  {0x3f08, 0x10},
  {0x4000, 0xf1},
  {0x4001, 0x00},
  {0x4005, 0x10},
  {0x4002, 0x27},
  {0x4009, 0x81},
  {0x400b, 0x0c},
  {0x4011, 0x20},
  {0x401b, 0x00},
  {0x401d, 0x00},
  {0x4020, 0x00},
  {0x4021, 0x04},
  {0x4022, 0x0c},
  {0x4023, 0x60},
  {0x4024, 0x0f},
  {0x4025, 0x36},
  {0x4026, 0x0f},
  {0x4027, 0x37},
  {0x4028, 0x00},
  {0x4029, 0x02},
  {0x402a, 0x04},
  {0x402b, 0x08},
  {0x402c, 0x00},
  {0x402d, 0x02},
  {0x402e, 0x04},
  {0x402f, 0x08},
  {0x401f, 0x00},
  {0x4034, 0x3f},
  {0x403d, 0x04},
  {0x4300, 0xff},
  {0x4301, 0x00},
  {0x4302, 0x0f},
  {0x4316, 0x00},
  {0x4503, 0x18},
  {0x4600, 0x01},
  {0x4601, 0x97},
  {0x481f, 0x32},
  {0x4837, 0x16},
  {0x4850, 0x10},
  {0x4851, 0x32},
  {0x4b00, 0x2a},
  {0x4b0d, 0x00},
  {0x4d00, 0x04},
  {0x4d01, 0x18},
  {0x4d02, 0xc3},
  {0x4d03, 0xff},
  {0x4d04, 0xff},
  {0x4d05, 0xff},
  {0x5000, 0x7e},
  {0x5001, 0x01},
  {0x5002, 0x08},
  {0x5003, 0x20},
  {0x5046, 0x12},
  {0x5780, 0x3e},
  {0x5781, 0x0f},
  {0x5782, 0x44},
  {0x5783, 0x02},
  {0x5784, 0x01},
  {0x5785, 0x00},
  {0x5786, 0x00},
  {0x5787, 0x04},
  {0x5788, 0x02},
  {0x5789, 0x0f},
  {0x578a, 0xfd},
  {0x578b, 0xf5},
  {0x578c, 0xf5},
  {0x578d, 0x03},
  {0x578e, 0x08},
  {0x578f, 0x0c},
  {0x5790, 0x08},
  {0x5791, 0x04},
  {0x5792, 0x00},
  {0x5793, 0x52},
  {0x5794, 0xa3},
  {0x5871, 0x0d},
  {0x5870, 0x18},
  {0x586e, 0x10},
  {0x586f, 0x08},
  {0x58f8, 0x3d},
  {0x5901, 0x00},
  {0x5b00, 0x02},
  {0x5b01, 0x10},
  {0x5b02, 0x03},
  {0x5b03, 0xcf},
  {0x5b05, 0x6c},
  {0x5e00, 0x00},
  {0x5e01, 0x41},
  {0x4825, 0x3a},
  {0x4826, 0x40},
  {0x4808, 0x25},
  {0x3763, 0x18},
  {0x3768, 0xcc},
  {0x470b, 0x28},
  {0x4202, 0x00},
  {0x400d, 0x10},
  {0x4040, 0x07},
  {0x403e, 0x08},
  {0x4041, 0xc6},
  {0x3007, 0x80},
  {0x400a, 0x01},
  {0x0100, 0x00},

};

LOCAL const SENSOR_REG_T ov8858_1408X792_setting[] = {
   {0x3808, 0x05},
   {0x3809, 0x80},
   {0x380a, 0x03},
   {0x380b, 0x18},
   {0x380c, 0x07},
   {0x380d, 0x94},
   {0x380e, 0x09},
   {0x380f, 0xaa},
   {0x3814, 0x03},
   {0x3821, 0x67},
   {0x382a, 0x03},
   {0x3830, 0x08},
   {0x3836, 0x02},
   {0x4001, 0x10},
   {0x4022, 0x05},
   {0x4023, 0x20},
   {0x4025, 0xe0},
   {0x4027, 0x5f},
   {0x402b, 0x04},
   {0x402f, 0x04},
   {0x4600, 0x00},
   {0x4601, 0xaf},
};

LOCAL const SENSOR_REG_T ov8858_3264X1836_setting[] = {
  {0x3808, 0x0c},
  {0x3809, 0xc0},
  {0x380a, 0x07},
  {0x380b, 0x2c},
  {0x380c, 0x07},
  {0x380d, 0x94},
  {0x380e, 0x09},
  {0x380f, 0xaa},
  {0x3814, 0x01},
  {0x3821, 0x46},
  {0x382a, 0x01},
  {0x3830, 0x06},
  {0x3836, 0x01},
  {0x4001, 0x00},
  {0x4022, 0x0c},
  {0x4023, 0x60},
  {0x4025, 0x36},
  {0x4027, 0x37},
  {0x402b, 0x08},
  {0x402f, 0x08},
  {0x4600, 0x01},
  {0x4601, 0x97},

};

LOCAL const SENSOR_REG_T ov8858_3264x2448_setting[] = {
  {0x3808, 0x0c},
  {0x3809, 0xc0},
  {0x380a, 0x09},
  {0x380b, 0x90},
  {0x380c, 0x07},
  {0x380d, 0x94},
  {0x380e, 0x09},
  {0x380f, 0xaa},
  {0x3814, 0x01},
  {0x3821, 0x46},
  {0x382a, 0x01},
  {0x3830, 0x06},
  {0x3836, 0x01},
  {0x4001, 0x00},
  {0x4022, 0x0c},
  {0x4023, 0x60},
  {0x4025, 0x36},
  {0x4027, 0x37},
  {0x402b, 0x08},
  {0x402f, 0x08},
  {0x4600, 0x01},
  {0x4601, 0x97},
};


LOCAL SENSOR_REG_TAB_INFO_T s_ov8858_resolution_Tab_RAW[] = {
	{ADDR_AND_LEN_OF_ARRAY(ov8858_common_init), 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},
	//{ADDR_AND_LEN_OF_ARRAY(ov8858_1408X792_setting), 1408, 792, 24, SENSOR_IMAGE_FORMAT_RAW},
	//{ADDR_AND_LEN_OF_ARRAY(ov8858_3264X1836_setting), 3264, 1836, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(ov8858_3264x2448_setting), 3264, 2448, 24, SENSOR_IMAGE_FORMAT_RAW},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},

	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

LOCAL SENSOR_TRIM_T s_ov8858_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	//{0, 0, 1408, 792, 70, 408, 2366},
	//{0, 0, 3264, 1836, 89, 528, 1872},
	{0, 0, 3264, 2448, 134, 720, 2474, {0, 0, 3264, 2448}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}}
};

LOCAL const SENSOR_REG_T s_ov8858_1408x792_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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
LOCAL const SENSOR_REG_T ov8858_3264X1836_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

LOCAL const SENSOR_REG_T  s_ov8858_3264x2448_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

LOCAL SENSOR_VIDEO_INFO_T s_ov8858_video_info[] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	//{{{30, 30, 70, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_ov8858_1408x792_video_tab},
	//{{{30, 30, 89, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)ov8858_3264X1836_video_tab},
	{{{15, 15, 162, 64}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_ov8858_3264x2448_video_tab},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},

	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}
};

LOCAL unsigned long _ov8858_set_video_mode(unsigned long param)
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

	if (PNULL == s_ov8858_video_info[mode].setting_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR)&s_ov8858_video_info[mode].setting_ptr[param];
	if (PNULL == sensor_reg_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	for (i=0x00; (0xffff!=sensor_reg_ptr[i].reg_addr)||(0xff!=sensor_reg_ptr[i].reg_value); i++) {
		Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	SENSOR_PRINT("0x%02lx", param);
	return 0;
}


LOCAL SENSOR_IOCTL_FUNC_TAB_T s_ov8858_ioctl_func_tab = {
	PNULL,
	_ov8858_PowerOn,
	PNULL,
	_ov8858_Identify,

	PNULL,			// write register
	PNULL,			// read  register
	PNULL,
	_ov8858_GetResolutionTrimTab,

	// External
	PNULL,
	PNULL,
	PNULL,

	PNULL, //_ov8858_set_brightness,
	PNULL, // _ov8858_set_contrast,
	PNULL,
	PNULL,			//_ov8858_set_saturation,

	PNULL, //_ov8858_set_work_mode,
	PNULL, //_ov8858_set_image_effect,

	_ov8858_BeforeSnapshot,
	_ov8858_after_snapshot,
	_ov8858_flash,
	PNULL,
	_ov8858_write_exposure,
	PNULL,
	_ov8858_write_gain,
	_ov8858_read_gain_scale,
	PNULL,
	_ov8858_write_af,
	PNULL,
	PNULL, //_ov8858_set_awb,
	PNULL,
	PNULL,
	PNULL, //_ov8858_set_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL, //_ov8858_GetExifInfo,
	_ov8858_ExtFunc,
	PNULL, //_ov8858_set_anti_flicker,
	_ov8858_set_video_mode,
	PNULL, //pick_jpeg_stream
	PNULL,  //meter_mode
	PNULL, //get_status
	_ov8858_StreamOn,
	_ov8858_StreamOff,
	_ov8858_cfg_otp,
	PNULL,
	_ov8858_read_snapshot_gain, //read_snapshot_gain
	 _ov8858_ex_write_exposure,

};


SENSOR_INFO_T g_ov8858_mipi_raw_info = {
	ov8858_I2C_ADDR_W,	// salve i2c write address
	ov8858_I2C_ADDR_R,	// salve i2c read address

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
	{{0x300A, 0x88},		// supply two code to identify sensor.
	 {0x300B, 0x58}},		// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,	// voltage of avdd

	3264,			// max width of source image
	2448,			// max height of source image
	"ov8858",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_B,// pattern of input image form sensor;
	s_ov8858_resolution_Tab_RAW,	// point to resolution table information structure
	&s_ov8858_ioctl_func_tab,	// point to ioctl function table
	&s_ov8858_mipi_raw_info_ptr,		// information and table about Rawrgb sensor
	NULL,			//&g_ov8858_ext_info,                // extend information about sensor
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
#if defined(ov8858_2_LANES)
	{SENSOR_INTERFACE_TYPE_CSI2, 2, 10, 0},
#elif defined(ov8858_4_LANES)
	{SENSOR_INTERFACE_TYPE_CSI2, 4, 10, 0},
#endif

	s_ov8858_video_info,
	3,			// skip frame num while change setting
	48,
	48,
};

LOCAL struct sensor_raw_info* Sensor_GetContext(void)
{
	return s_ov8858_mipi_raw_info_ptr;
}

LOCAL uint32_t Sensor_ov8858_InitRawTuneInfo(void)
{
	uint32_t rtn=0x00;
	//struct sensor_raw_info* raw_sensor_ptr=Sensor_GetContext();
	//struct sensor_raw_tune_info* sensor_ptr=raw_sensor_ptr->tune_ptr;
	//struct sensor_raw_cali_info* cali_ptr=raw_sensor_ptr->cali_ptr;

	return rtn;
}


LOCAL unsigned long _ov8858_GetResolutionTrimTab(unsigned long param)
{
        UNUSED(param);
	SENSOR_PRINT("0x%lx",  (unsigned long)s_ov8858_Resolution_Trim_Tab);
	return (unsigned long) s_ov8858_Resolution_Trim_Tab;
}
LOCAL unsigned long _ov8858_PowerOn(unsigned long power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_ov8858_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_ov8858_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_ov8858_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_ov8858_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_ov8858_mipi_raw_info.reset_pulse_level;
	//uint32_t reset_width=g_ov8858_yuv_info.reset_pulse_width;

	if (SENSOR_TRUE == power_on) {
		Sensor_PowerDown(power_down);
		// Open power
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		//Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		Sensor_SetAvddVoltage(avdd_val);
		usleep(1000);
		Sensor_SetIovddVoltage(iovdd_val);
		usleep(1000);
		Sensor_SetDvddVoltage(dvdd_val);
		usleep(2*1000);
		//_ov8858_dw9714_SRCInit(2);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(2*1000);
		Sensor_PowerDown(!power_down);
		usleep(2*1000);
		// Reset sensor
		Sensor_Reset(reset_level);
		usleep(5*1000);
	} else {
		Sensor_PowerDown(power_down);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		//Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_SetDvddVoltage(SENSOR_AVDD_CLOSED);
		Sensor_SetIovddVoltage(SENSOR_AVDD_CLOSED);
		Sensor_SetAvddVoltage(SENSOR_AVDD_CLOSED);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
	}
	SENSOR_PRINT("SENSOR_ov8858: _ov8858_Power_On(1:on, 0:off): %ld", power_on);
	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov8858_cfg_otp(unsigned long  param)
{
        UNUSED(param);
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_ov8858_raw_param_tab;
	uint32_t module_id=g_module_id;
//	uint16_t module_id= 0;

//	module_id=ov8858_otp_identify_otp(NULL);
	SENSOR_PRINT("SENSOR_ov8858: _ov8858_cfg_otp" );
        Sensor_WriteReg(0x0103, 0x01);
	usleep(5*1000);
	if(PNULL!=tab_ptr[module_id].cfg_otp){
		tab_ptr[module_id].cfg_otp(0);
		}

	return rtn;
}

LOCAL unsigned long _ov8858_com_Identify_otp(void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;
        UNUSED(param_ptr);
	SENSOR_PRINT("SENSOR_ov8858: _ov8858_com_Identify_otp");

	/*read param id from sensor omap*/
	param_id=ov8858_RAW_PARAM_COM;

	if(ov8858_RAW_PARAM_COM==param_id){
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}

LOCAL uint32_t _ov8858_GetRawInof(void)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_ov8858_raw_param_tab;
	uint32_t param_id;
	uint32_t i=0x00;

	/*read param id from sensor omap*/
	param_id=ov8858_RAW_PARAM_COM;

	for(i=0x00; ; i++)
	{
		g_module_id = i;
		if(RAW_INFO_END_ID==tab_ptr[i].param_id){
			if(NULL==s_ov8858_mipi_raw_info_ptr){
				SENSOR_PRINT("SENSOR_ov8858: ov5647_GetRawInof no param error");
				rtn=SENSOR_FAIL;
			}
			SENSOR_PRINT("SENSOR_ov8858: ov8858_GetRawInof end");
			break;
		}
		else if(PNULL!=tab_ptr[i].identify_otp){
			if(SENSOR_SUCCESS==tab_ptr[i].identify_otp(0))
			{
				s_ov8858_mipi_raw_info_ptr = tab_ptr[i].info_ptr;
				SENSOR_PRINT("SENSOR_ov8858: ov8858_GetRawInof success");
				break;
			}
		}
	}

	return rtn;
}

LOCAL uint32_t _ov8858_GetMaxFrameLine(uint32_t index)
{
	uint32_t max_line=0x00;
	SENSOR_TRIM_T_PTR trim_ptr=s_ov8858_Resolution_Trim_Tab;

	max_line=trim_ptr[index].frame_line;

	return max_line;
}

extern int Sensor_Set_slave_adr(uint32_t sensor_slave_adr);


LOCAL unsigned long _ov8858_Identify(unsigned long param)
{
#define ov8858_PID_VALUE_0    0x00
#define ov8858_PID_ADDR_0     0x300A
#define ov8858_PID_VALUE_1    0x88
#define ov8858_PID_ADDR_1     0x300B
#define ov8858_VER_VALUE    0x58
#define ov8858_VER_ADDR     0x300C
       UNUSED(param);
	uint8_t pid_value_0 = 0x01;
	uint8_t pid_value_1 = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("SENSOR_ov8858: mipi raw identify\n");

	pid_value_0 = Sensor_ReadReg(ov8858_PID_ADDR_0);
	if (ov8858_PID_VALUE_0 == pid_value_0) {
		pid_value_1 = Sensor_ReadReg(ov8858_PID_ADDR_1);
		if (ov8858_PID_VALUE_1 == pid_value_1) {
			ver_value = Sensor_ReadReg(ov8858_VER_ADDR);
			SENSOR_PRINT("SENSOR_ov8858: Identify: PID = 0x%x, VER = 0x%x", pid_value_1, ver_value);
			if (ov8858_VER_VALUE == ver_value) {
				SENSOR_PRINT("SENSOR_ov8858: this is ov8858 sensor !");
				ret_value=_ov8858_GetRawInof();
				if(SENSOR_SUCCESS != ret_value)
				{
					SENSOR_PRINT("SENSOR_ov8858: the module is unknow error !");
				}
				Sensor_ov8858_InitRawTuneInfo();
			} else {
				SENSOR_PRINT("SENSOR_ov8858: Identify this is OV%x%x sensor !", pid_value_1, ver_value);
			}
		} else {
			SENSOR_PRINT("SENSOR_ov8858: identify fail, PID_ADDR = 0x%x,  pid_value= 0x%d", ov8858_PID_ADDR_1, pid_value_1);
		}
	} else {
		SENSOR_PRINT("SENSOR_ov8858: identify fail, PID_ADDR = 0x%x, pid_value= 0x%d", ov8858_PID_ADDR_0, pid_value_0);

	}

	return ret_value;
}

static uint32_t Sexpsure_line = 0;

LOCAL unsigned long _ov8858_write_exposure_dummy(uint16_t expsure_line,uint16_t dummy_line,uint16_t size_index)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t frame_len=0x00;
	uint16_t frame_len_cur=0x00;
	uint16_t max_frame_len=0x00;
	uint16_t value=0x00;
	uint16_t value0=0x00;
	uint16_t value1=0x00;
	uint16_t value2=0x00;


	SENSOR_PRINT("SENSOR_ov8858: write_exposure line:%d, dummy:%d, size_index:%d", expsure_line, dummy_line, size_index);

	max_frame_len=_ov8858_GetMaxFrameLine(size_index);

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

LOCAL unsigned long _ov8858_write_exposure(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t expsure_line=0x00;
	uint16_t dummy_line=0x00;
	uint16_t size_index=0x00;

	expsure_line=param&0xffff;
	dummy_line=(param>>0x10)&0x0fff;
	size_index=(param>>0x1c)&0x0f;

	 _ov8858_write_exposure_dummy(expsure_line,dummy_line,size_index);

	SENSOR_PRINT("SENSOR_ov8858: write_exposure line:%d, dummy:%d, size_index:%d\n", expsure_line, dummy_line, size_index);

	return ret_value;
}

LOCAL unsigned long _ov8858_ex_write_exposure(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t exposure_line=0x00;
	uint16_t dummy_line=0x00;
	uint16_t size_index=0x00;

	struct sensor_ex_exposure  *exp_dummy = NULL;

	exp_dummy = (struct sensor_ex_exposure*)param;

	exposure_line = exp_dummy->exposure;
	dummy_line = exp_dummy->dummy;
	size_index = exp_dummy->size_index;

	 _ov8858_write_exposure_dummy(exposure_line,dummy_line,size_index);

	SENSOR_PRINT("SENSOR_ov8858: write_exposure line:%d, dummy:%d, size_index:%d\n", exposure_line, dummy_line, size_index);

	return ret_value;
}

LOCAL unsigned long _ov8858_read_gain_scale(unsigned long param)
{
       UNUSED(param);
	SENSOR_PRINT("SENSOR_OV8858:_ov8858_read_gain_scale =0x%x", SENSOR_BASE_GAIN);
	return SENSOR_BASE_GAIN;
}

LOCAL unsigned long _ov8858_write_gain(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint32_t value=0x00;
	uint32_t real_gain = 0;

	//param = Sgain;
	SENSOR_PRINT("SENSOR_ov8858: write_gain:0x%lx", param);
	real_gain = ((param&0xf)+16)*(((param>>4)&0x01)+1)*(((param>>5)&0x01)+1)*(((param>>6)&0x01)+1)*(((param>>7)&0x01)+1);
	real_gain = real_gain*(((param>>8)&0x01)+1)*(((param>>9)&0x01)+1)*(((param>>10)&0x01)+1)*(((param>>11)&0x01)+1);

	value = real_gain*8;
//	value = param;
	SENSOR_PRINT("SENSOR_ov8858: write_gain:0x%lx,  real_gain = 0x%x", param, value);
	real_gain = value & 0xff;
	ret_value = Sensor_WriteReg(0x3509, real_gain);/*0-7*/
	real_gain = (value>>0x08)&0x07;
	ret_value = Sensor_WriteReg(0x3508, real_gain);/*8*/

	return ret_value;
}

static uint32_t BU64241GWZ_write_af(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t a_u2Data = param&0x0ffff;
	uint8_t cmd_val[2] = {(uint8_t)(((a_u2Data >> 8) & 0x03) | 0xc0), (uint8_t)(a_u2Data & 0xff)};
	uint16_t slave_addr = BU64241GWZ_VCM_SLAVE_ADDR;
	uint16_t cmd_len = 2;

	SENSOR_PRINT("%d", param);
	SENSOR_PRINT("BU64241GWZ_write_af");
	ret_value = Sensor_WriteI2C(slave_addr, cmd_val, cmd_len);

	return ret_value;
}

static uint32_t dw9714_write_af(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint8_t cmd_val[2] = { 0x00 };
	uint16_t slave_addr = DW9714_VCM_SLAVE_ADDR;
	uint16_t cmd_len = 2;
	uint16_t step_4bit = 0x09;

	SENSOR_PRINT("%d", param);
	SENSOR_PRINT("dw9714_write_af");
	cmd_val[0] = (param & 0xfff0) >> 4;
	cmd_val[1] = ((param & 0x0f) << 4) | step_4bit;
	ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);

	return ret_value;
}

LOCAL unsigned long _ov8858_write_af(unsigned long param)
{
	SENSOR_PRINT("SENSOR_ov8858: _ov8858_write_af %ld",param);
#if defined(CONFIG_VCM_BU64241GWZ)
	return BU64241GWZ_write_af(param);
#else
	return dw9714_write_af(param);
#endif
}

LOCAL unsigned long _ov8858_BeforeSnapshot(unsigned long param)
{
	uint8_t ret_l, ret_m, ret_h;
	uint32_t capture_exposure, preview_maxline;
	uint32_t capture_maxline, preview_exposure;
	uint32_t gain = 0;
	uint32_t capture_mode = param & 0xffff;
	uint32_t preview_mode = (param >> 0x10 ) & 0xffff;
	uint32_t prv_linetime=s_ov8858_Resolution_Trim_Tab[preview_mode].line_time;
	uint32_t cap_linetime = s_ov8858_Resolution_Trim_Tab[capture_mode].line_time;
	float cnr_gain = 1.0;

	SENSOR_PRINT("SENSOR_ov8858: BeforeSnapshot mode: 0x%08lx",param);

	if (preview_mode == capture_mode) {
		SENSOR_PRINT("SENSOR_ov8858: prv mode equal to capmode");
		goto CFG_INFO;
	}

	ret_h = (uint8_t) Sensor_ReadReg(0x3500);
	ret_m = (uint8_t) Sensor_ReadReg(0x3501);
	ret_l = (uint8_t) Sensor_ReadReg(0x3502);
	preview_exposure = (ret_h << 12) + (ret_m << 4) + (ret_l >> 4);

	ret_h = (uint8_t) Sensor_ReadReg(0x380e);
	ret_l = (uint8_t) Sensor_ReadReg(0x380f);
	preview_maxline = (ret_h << 8) + ret_l;

	gain = _ov8858_ReadGain(param);

	Sensor_SetMode(capture_mode);
	Sensor_SetMode_WaitDone();

	if (prv_linetime == cap_linetime) {
		SENSOR_PRINT("SENSOR_ov8858: prvline equal to capline");
		goto CFG_INFO;
	}

	ret_h = (uint8_t) Sensor_ReadReg(0x380e);
	ret_l = (uint8_t) Sensor_ReadReg(0x380f);
	capture_maxline = (ret_h << 8) + ret_l;

	capture_exposure = preview_exposure * prv_linetime/cap_linetime;
	//capture_exposure *= 2;

	if(0 == capture_exposure){
		capture_exposure = 1;
	}
	SENSOR_PRINT("SENSOR_OV8858: BeforeSnapshot, preview_exposure =%d, capture_exposure = %d, capture_maxline = %d", preview_exposure,capture_exposure, capture_maxline);

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

	cnr_gain = (float) gain;
	cnr_gain /= SENSOR_BASE_GAIN;
	SENSOR_PRINT_HIGH("cnr: gain = %0x, cnr_gain = %f", gain, cnr_gain);
	//set_cnr_analog_gain(cnr_gain);

	CFG_INFO:
	s_capture_shutter = _ov8858_get_shutter();
	s_capture_VTS = _ov8858_get_VTS();
	_ov8858_ReadGain(capture_mode);
	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, s_capture_shutter);

	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov8858_after_snapshot(unsigned long param)
{
	SENSOR_PRINT("SENSOR_ov8858: after_snapshot mode:%ld", param);
	Sensor_SetMode(param);
	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov8858_flash(unsigned long param)
{
	SENSOR_PRINT("SENSOR_ov8858: param=%ld", param);

	/* enable flash, disable in _ov8858_BeforeSnapshot */
	g_flash_mode_en = param;
	Sensor_SetFlash(param);
	SENSOR_PRINT_HIGH("end");
	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov8858_StreamOn(unsigned long param)
{
        UNUSED(param);
	SENSOR_PRINT("SENSOR_ov8858: StreamOn");

	Sensor_WriteReg(0x0100, 0x01);

	return 0;
}

LOCAL unsigned long _ov8858_StreamOff(unsigned long param)
{
        UNUSED(param);
	SENSOR_PRINT("SENSOR_ov8858: StreamOff");

	Sensor_WriteReg(0x0100, 0x00);
	usleep(70*1000);

	return 0;
}


LOCAL unsigned long _ov8858_read_snapshot_gain(unsigned long  param)
{
	uint32_t gain = 0;
	gain = _ov8858_ReadGain(param);

	SENSOR_PRINT("SENSOR_OV8858: gain = %d base gain %d real gain %d",gain,SENSOR_BASE_GAIN, gain*100/SENSOR_BASE_GAIN);
	return gain*100/SENSOR_BASE_GAIN;

}


int _ov8858_get_shutter(void)
{
	// read shutter, in number of line period
	int shutter;

	shutter = (Sensor_ReadReg(0x03500) & 0x0f);
	shutter = (shutter<<8) + Sensor_ReadReg(0x3501);
	shutter = (shutter<<4) + (Sensor_ReadReg(0x3502)>>4);

	return shutter;
}

int _ov8858_set_shutter(int shutter)
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

int _ov8858_get_gain16(void)
{
	// read gain, 16 = 1x
	int gain16;

	gain16 = Sensor_ReadReg(0x350a) & 0x03;
	gain16 = (gain16<<8) + Sensor_ReadReg(0x350b);

	return gain16;
}

int _ov8858_set_gain16(int gain16)
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
        UNUSED(capture_VTS);
	// write capture gain
	_ov8858_set_gain16(capture_gain16);

	// write capture shutter
	/*if (capture_shutter > (capture_VTS - 4)) {
		capture_VTS = capture_shutter + 4;
		OV5640_set_VTS(capture_VTS);
	}*/
	_ov8858_set_shutter(capture_shutter);
}

static uint32_t _ov8858_get_real_gain(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct sensor_real_gain_tag *gain_param = (struct sensor_real_gain_tag *)param;
	gain_param->gain = (float)(_ov8858_ReadGain(param))/SENSOR_BASE_GAIN;
	//SENSOR_PRINT_HIGH("real gain = %f", real_gain_thrs);
	SENSOR_PRINT_HIGH("gain_param->gain = %f", gain_param->gain);
	return rtn;
}

static uint32_t _ov8858_get_cnr_param(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct sensor_cnr_param_tag *cur_cnr_param = (struct sensor_cnr_param_tag *)param;
#ifdef USE_CNR_ONE_DIMEN_ARRAY
	struct cnr_param *p_ov8858_cnr_param[2]= {(cur_param*)&cnrparam_0,(cur_param*)&cnrparam_1};
	struct cnr_param ov8858_cnr_param[2] ={*(p_ov8858_cnr_param[0]),*(p_ov8858_cnr_param[1])};
#endif
	cur_cnr_param->cnr_thrs_param = ov8858_cnr_param;
	SENSOR_PRINT("ov8858_cnr_param = %d, &ov8858_cnr_param = %d", ov8858_cnr_param,&ov8858_cnr_param);
	SENSOR_PRINT_HIGH("ov8858_cnr_param[0].res.height = %ld", ov8858_cnr_param[0].res.height);
	SENSOR_PRINT_HIGH("cur_cnr_param->cnr_thrs_param = %p,&ov8858_cnr_param=%p,ov8858_cnr_param =%p", cur_cnr_param->cnr_thrs_param,&ov8858_cnr_param,ov8858_cnr_param);
	return rtn;
}

static uint32_t _ov8858_SetEV(unsigned long  param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	uint16_t value=0x00;
	uint32_t gain = s_ov8858_gain;
	uint32_t ev = ext_ptr->param;

	SENSOR_PRINT("SENSOR_ov8858: _ov8858_SetEV param: 0x%x", ext_ptr->param);

	switch(ev) {
	case SENSOR_HDR_EV_LEVE_0:
		_calculate_hdr_exposure(s_ov8858_gain/2,s_capture_VTS,s_capture_shutter);
		break;
	case SENSOR_HDR_EV_LEVE_1:
		_calculate_hdr_exposure(s_ov8858_gain,s_capture_VTS,s_capture_shutter);
		break;
	case SENSOR_HDR_EV_LEVE_2:
		_calculate_hdr_exposure(s_ov8858_gain,s_capture_VTS,s_capture_shutter *4);
		break;
	default:
		break;
	}
	return rtn;
}
LOCAL unsigned long _ov8858_ExtFunc(unsigned long  ctl_param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr =
	    (SENSOR_EXT_FUN_PARAM_T_PTR) ctl_param;
	SENSOR_PRINT_HIGH("0x%x", ext_ptr->cmd);
	SENSOR_PRINT("SENSOR_OV8858: _ov8858_ExtFunc = %d",ext_ptr->cmd);

	switch (ext_ptr->cmd) {
	case SENSOR_EXT_FUNC_INIT:
		break;
	case SENSOR_EXT_FOCUS_START:
		break;
	case SENSOR_EXT_EXPOSURE_START:
		break;
	case SENSOR_EXT_EV:
		rtn = _ov8858_SetEV(ctl_param);
		break;
	case SENSOR_EXT_REAL_GAIN:
		rtn = _ov8858_get_real_gain(ctl_param);
		break;
	case SENSOR_EXT_CNR_PARAM:
		rtn = _ov8858_get_cnr_param(ctl_param);
		break;
	default:
		break;
	}
	return rtn;
}
LOCAL int _ov8858_get_VTS(void)
{
	// read VTS from register settings
	int VTS;

	VTS = Sensor_ReadReg(0x380e);//total vertical size[15:8] high byte
	VTS = (VTS<<8) + Sensor_ReadReg(0x380f);
	return VTS;
}

LOCAL int _ov8858_set_VTS(int VTS)
{
	// write VTS to registers
	int temp;

	temp = VTS & 0xff;
	Sensor_WriteReg(0x380f, temp);
	temp = VTS>>8;
	Sensor_WriteReg(0x380e, temp);
	return 0;
}
LOCAL uint32_t _ov8858_ReadGain(uint32_t param)
{
//	uint32_t rtn = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint32_t gain = 0;
        UNUSED(param);
	value = Sensor_ReadReg(0x3509);/*0-7*/
	gain = value&0xff;
	value = Sensor_ReadReg(0x3508);/*8*/
	gain |= (value<<0x08)&0x700;

	s_ov8858_gain=(int)gain;

	SENSOR_PRINT("SENSOR_ov8858: _ov8858_ReadGain gain: 0x%x", s_ov8858_gain);

	return gain;
}

LOCAL uint32_t _ov8858_dw9714_SRCInit(uint32_t mode)
{
	uint8_t cmd_val[2] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t ret_value = SENSOR_SUCCESS;
	int i = 0;

	slave_addr = DW9714_VCM_SLAVE_ADDR;
	SENSOR_PRINT(" _ov8858_dw9714_SRCInit: mode = %d\n", mode);
	switch (mode) {
		case 1:
		break;

		case 2:
		{
			cmd_val[0] = 0xec;
			cmd_val[1] = 0xa3;
			cmd_len = 2;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

			cmd_val[0] = 0xa1;
			cmd_val[1] = 0x0e;
			cmd_len = 2;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

			cmd_val[0] = 0xf2;
			cmd_val[1] = 0x90;
			cmd_len = 2;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

			cmd_val[0] = 0xdc;
			cmd_val[1] = 0x51;
			cmd_len = 2;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
		}
		break;

		case 3:
		break;

	}

	return ret_value;
}

