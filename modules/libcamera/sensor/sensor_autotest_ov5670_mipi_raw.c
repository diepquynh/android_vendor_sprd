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
#include "sensor_ov5670_otp.c"
#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
#include "ov5670/sensor_ov5670_raw_param_v3.c"
#else
#include "ov5670/sensor_ov5670_raw_param.c"
#endif

#define ov5670_I2C_ADDR_W        0x10
#define ov5670_I2C_ADDR_R         0x10
#define OV5670_MIN_FRAME_LEN_PRV  0x484
#define OV5670_MIN_FRAME_LEN_CAP  0x7B6
#define OV5670_RAW_PARAM_COM      0x0000
static uint32_t g_module_id = 0;

LOCAL unsigned long _ov5670_GetResolutionTrimTab(unsigned long param);
LOCAL unsigned long _ov5670_PowerOn(unsigned long power_on);
LOCAL unsigned long _ov5670_Identify(unsigned long param);
LOCAL unsigned long _ov5670_StreamOn(unsigned long param);
LOCAL unsigned long _ov5670_StreamOff(unsigned long param);

LOCAL uint32_t _ov5670_com_Identify_otp(void* param_ptr);

LOCAL const struct raw_param_info_tab s_ov5670_raw_param_tab[]={
	//{OV5670_RAW_PARAM_Sunny, &s_ov5670_mipi_raw_info, _ov5670_Sunny_Identify_otp, update_otp_wb},
	//{OV5670_RAW_PARAM_Truly, &s_ov5670_mipi_raw_info, _ov5670_Truly_Identify_otp, update_otp_wb},
	{OV5670_RAW_PARAM_COM, &s_ov5670_mipi_raw_info, _ov5670_Identify_otp, ov5670_apply_otp},
	{RAW_INFO_END_ID, PNULL, PNULL, PNULL}
};

struct sensor_raw_info* s_at_ov5670_mipi_raw_info_ptr=NULL;

LOCAL const SENSOR_REG_T ov5670_com_mipi_raw[] = {
	{0x0103, 0x01}, // software reset
	{0x0100, 0x00}, // software standby
	{0x0300, 0x04}, // PLL
	{0x0301, 0x00},
	{0x0302, 0x69},
	{0x0303, 0x00},
	{0x0304, 0x03},
	{0x0305, 0x01},
	{0x0306, 0x01},
	{0x030a, 0x00},
	{0x030b, 0x00},
	{0x030c, 0x00},
	{0x030d, 0x1e},
	{0x030e, 0x00},
	{0x030f, 0x06},
	{0x0312, 0x01}, // PLL
	{0x3000, 0x00}, // Fsin/Vsync input
	{0x3002, 0x21}, // ULPM output
	{0x3005, 0xf0}, // sclk_psram on, sclk_syncfifo on
	{0x3007, 0x00},
	{0x3015, 0x0f}, // npump clock div = 1, disable Ppumu_clk
	{0x3018, 0x32}, // MIPI 2 lane
	{0x301a, 0xf0}, // sclk_stb on, sclk_ac on, slck_tc on
	{0x301b, 0xf0}, // sclk_blc on, sclk_isp on, sclk_testmode on, sclk_vfifo on
	{0x301c, 0xf0}, // sclk_mipi on, sclk_dpcm on, sclk_otp on
	{0x301d, 0xf0}, // sclk_asram_tst on, sclk_grp on, sclk_bist on,
	{0x301e, 0xf0}, // sclk_ilpwm on, sclk_lvds on, sclk-vfifo on, sclk_mipi on,
	{0x3030, 0x00}, // sclk normal, pclk normal
	{0x3031, 0x0a}, // 10-bit mode
	{0x303c, 0xff}, // reserved
	{0x303e, 0xff}, // reserved
	{0x3040, 0xf0}, // sclk_isp_fc_en, sclk_fc-en, sclk_tpm_en, sclk_fmt_en
	{0x3041, 0x00}, // reserved
	{0x3042, 0xf0}, // reserved
	{0x3106, 0x11}, // sclk_div = 1, sclk_pre_div = 1
	{0x3500, 0x00}, // exposure H
	{0x3501, 0x3d}, // exposure M
	{0x3502, 0x00}, // exposure L
	{0x3503, 0x04}, // gain no delay, use sensor gain
	{0x3504, 0x03}, // exposure manual, gain manual
	{0x3505, 0x83}, // sensor gain fixed bit
	{0x3508, 0x07}, // gain H
	{0x3509, 0x80}, // gain L
	{0x350e, 0x04}, // short digital gain H
	{0x350f, 0x00}, // short digital gain L
	{0x3510, 0x00}, // short exposure H
	{0x3511, 0x02}, // short exposure M
	{0x3512, 0x00}, // short exposure L
	{0x3601, 0xc8}, // analog control
	{0x3610, 0x88},
	{0x3612, 0x48},
	{0x3614, 0x5b},
	{0x3615, 0x96},
	{0x3621, 0xd0},
	{0x3622, 0x00},
	{0x3623, 0x00},
	{0x3633, 0x13},
	{0x3634, 0x13},
	{0x3635, 0x13},
	{0x3636, 0x13},
	{0x3645, 0x13},
	{0x3646, 0x82},
	{0x3650, 0x00},
	{0x3652, 0xff},
	{0x3656, 0xff},
	{0x365a, 0xff},
	{0x365e, 0xff},
	{0x3668, 0x00},
	{0x366a, 0x07},
	{0x366e, 0x08},
	{0x366d, 0x00},
	{0x366f, 0x80}, // analog control
	{0x3700, 0x28}, // sensor control
	{0x3701, 0x10},
	{0x3702, 0x3a},
	{0x3703, 0x19},
	{0x3705, 0x00},
	{0x3706, 0x66},
	{0x3707, 0x08},
	{0x3708, 0x34},
	{0x3709, 0x40},
	{0x370a, 0x01},
	{0x370b, 0x1b},
	{0x3714, 0x24},
	{0x371a, 0x3e},
	{0x3733, 0x00},
	{0x3734, 0x00},
	{0x373a, 0x05},
	{0x373b, 0x06},
	{0x373c, 0x0a},
	{0x373f, 0xa0},
	{0x3755, 0x00},
	{0x3758, 0x00},
	{0x3766, 0x5f},
	{0x3768, 0x00},
	{0x3769, 0x22},
	{0x3773, 0x08},
	{0x3774, 0x1f},
	{0x3776, 0x06},
	{0x37a0, 0x88},
	{0x37a1, 0x5c},
	{0x37a7, 0x88},
	{0x37a8, 0x70},
	{0x37aa, 0x88},
	{0x37ab, 0x48},
	{0x37b3, 0x66},
	{0x37c2, 0x04},
	{0x37c5, 0x00},
	{0x37c8, 0x00}, // sensor control
	{0x3800, 0x00}, // x addr start H
	{0x3801, 0x0c}, // x addr start L
	{0x3802, 0x00}, // y addr start H
	{0x3803, 0x04}, // y addr start L
	{0x3804, 0x0a}, // x addr end H
	{0x3805, 0x33}, // x addr end L
	{0x3806, 0x07}, // y addr end H
	{0x3807, 0xa3}, // y addr end L
	{0x3808, 0x05}, // x output size H
	{0x3809, 0x10}, // x outout size L
	{0x380a, 0x03}, // y output size H
	{0x380b, 0xc0}, // y output size L
	{0x380c, 0x06}, // HTS H
	{0x380d, 0x8c}, // HTS L
	{0x380e, 0x07}, // VTS H
	{0x380f, 0xfd}, // VTS L
	{0x3811, 0x04}, // ISP x win L
	{0x3813, 0x02}, // ISP y win L
	{0x3814, 0x03}, // x inc odd
	{0x3815, 0x01}, // x inc even
	{0x3816, 0x00}, // vsync start H
	{0x3817, 0x00}, // vsync star L
	{0x3818, 0x00}, // vsync end H
	{0x3819, 0x00}, // vsync end L
#ifndef CONFIG_CAMERA_IMAGE_180
	{0x3820, 0x96}, // vsyn48_blc on, vflip on
	{0x3821, 0x41}, // hsync_en_o, mirror off, dig_bin on
	{0x450b, 0x20}, // need to set when flip
#else
	{0x3820, 0x90}, // vsyn48_blc on, vflip off
	{0x3821, 0x47}, // hsync_en_o, mirror on, dig_bin on
#endif
	{0x3822, 0x48}, // addr0_num[3:1]=0x02, ablc_num[5:1]=0x08
	{0x3826, 0x00}, // r_rst_fsin H
	{0x3827, 0x08}, // r_rst_fsin L
	{0x382a, 0x03}, // y inc odd
	{0x382b, 0x01}, // y inc even
	{0x3830, 0x08},
	{0x3836, 0x02},
	{0x3837, 0x00},
	{0x3838, 0x10},
	{0x3841, 0xff},
	{0x3846, 0x48},
	{0x3861, 0x00},
	{0x3862, 0x00},
	{0x3863, 0x18},
	{0x3a11, 0x01},
	{0x3a12, 0x78},
	{0x3b00, 0x00}, // strobe
	{0x3b02, 0x00},
	{0x3b03, 0x00},
	{0x3b04, 0x00},
	{0x3b05, 0x00}, // strobe
	{0x3c00, 0x89},
	{0x3c01, 0xab},
	{0x3c02, 0x01},
	{0x3c03, 0x00},
	{0x3c04, 0x00},
	{0x3c05, 0x03},
	{0x3c06, 0x00},
	{0x3c07, 0x05},
	{0x3c0c, 0x00},
	{0x3c0d, 0x00},
	{0x3c0e, 0x00},
	{0x3c0f, 0x00},
	{0x3c40, 0x00},
	{0x3c41, 0xa3},
	{0x3c43, 0x7d},
	{0x3c45, 0xd7},
	{0x3c47, 0xfc},
	{0x3c50, 0x05},
	{0x3c52, 0xaa},
	{0x3c54, 0x71},
	{0x3c56, 0x80},
	{0x3f03, 0x00}, // PSRAM
	{0x3f0a, 0x00},
	{0x3f0b, 0x00}, // PSRAM
	{0x4001, 0x60}, // BLC, K enable
	{0x4009, 0x05}, // BLC, black line end line
	{0x4020, 0x00}, // BLC, offset compensation th000
	{0x4021, 0x00}, // BLC, offset compensation K000
	{0x4022, 0x00},
	{0x4023, 0x00},
	{0x4024, 0x00},
	{0x4025, 0x00},
	{0x4026, 0x00},
	{0x4027, 0x00},
	{0x4028, 0x00},
	{0x4029, 0x00},
	{0x402a, 0x00},
	{0x402b, 0x00},
	{0x402c, 0x00},
	{0x402d, 0x00},
	{0x402e, 0x00},
	{0x402f, 0x00},
	{0x4040, 0x00},
	{0x4041, 0x00},
	{0x4042, 0x00},
	{0x4043, 0x80},
	{0x4044, 0x00},
	{0x4045, 0x80},
	{0x4046, 0x00},
	{0x4047, 0x80},
	{0x4048, 0x00}, // BLC, kcoef_r_man H
	{0x4049, 0x80}, // BLC, kcoef_r_man L
	{0x4303, 0x00},
	{0x4307, 0x30},
	{0x4500, 0x58},
	{0x4501, 0x04},
	{0x4502, 0x48},
	{0x4503, 0x10},
	{0x4508, 0x55},
	{0x4509, 0x55},
	{0x450a, 0x00},
	{0x450b, 0x00},
	{0x4600, 0x00},
	{0x4601, 0x81},
	{0x4700, 0xa4},
	{0x4800, 0x4c}, // MIPI conrol
	{0x4816, 0x53}, // emb_dt
	{0x481f, 0x40}, // clock_prepare_min
	{0x4837, 0x13}, // clock period of pclk2x
	{0x5000, 0x16}, // awb_gain_en, bc_en, wc_en
	{0x5001, 0x01}, // blc_en
	{0x5002, 0xa8}, // otp_dpc_en
	{0x5004, 0x0c}, // ISP size auto control enable
	{0x5006, 0x0c},
	{0x5007, 0xe0},
	{0x5008, 0x01},
	{0x5009, 0xb0},
	{0x5901, 0x00}, // VAP
	{0x5a01, 0x00}, // WINC x start offset H
	{0x5a03, 0x00}, // WINC x start offset L
	{0x5a04, 0x0c}, // WINC y start offset H
	{0x5a05, 0xe0}, // WINC y start offset L
	{0x5a06, 0x09}, // WINC window width H
	{0x5a07, 0xb0}, // WINC window width L
	{0x5a08, 0x06}, // WINC window height H
	{0x5e00, 0x00}, // WINC window height L
	{0x3618, 0x2a},
	//Ally031414
	{0x3734, 0x40}, // Improve HFPN
	{0x5b00, 0x01}, // [2:0] otp start addr[10:8]
	{0x5b01, 0x10}, // [7:0] otp start addr[7:0]
	{0x5b02, 0x01}, // [2:0] otp end addr[10:8]
	{0x5b03, 0xDB}, // [7:0] otp end addr[7:0]
	{0x3d8c, 0x71}, // Header address high byte
	{0x3d8d, 0xEA}, // Header address low byte
	{0x4017, 0x10}, // threshold = 4LSB for Binning sum format.
	//Strong DPC1.53
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
	//Ping
	{0x3503, 0x00}, // exposure gain/exposure delay not used
	//added
	{0x3d85, 0x17}, // OTP power up load data enable, otp power up load setting enable, otp write register load setting
	//enable
	{0x3655, 0x20},
	//{0x0100, 0x01}, // wake up from software standby, stream on
};


LOCAL const SENSOR_REG_T ov5670_640X480_mipi_raw[] = {
	//@@res5_reg_array-Video 640x480 30fps 24M MCLK 2lane 840Mbps/lane
	//{ 0x0100, 0x00},
	{0x3501, 0x1f},
	{0x3623, 0x04},
	{0x366e, 0x08},
	{0x370b, 0x1b},
	{0x3808, 0x02},
	{0x3809, 0x80},
	{0x380a, 0x01},
	{0x380b, 0xe0},
	{0x380c, 0x06},
	{0x380d, 0x90},
	{0x380e, 0x07},
	{0x380f, 0xf8},
	{0x3814, 0x07},
	{0x3820, 0x90},
	{0x3821, 0xc6},
	{0x382a, 0x07},
	{0x4009, 0x05},
	{0x400a, 0x02},
	{0x400b, 0x00},
	{0x4502, 0x44},
	{0x4508, 0x55},
	{0x4509, 0x55},
	{0x450a, 0x03},
	{0x4600, 0x00},
	{0x4601, 0x40},
	{0x4017, 0x10},
	{0x4303, 0x08},//color bar enable
	//{ 0x0100, 0x01},
};

LOCAL SENSOR_REG_TAB_INFO_T s_ov5670_resolution_Tab_RAW[] = {
	{ADDR_AND_LEN_OF_ARRAY(ov5670_com_mipi_raw), 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(ov5670_640X480_mipi_raw), 640, 480, 24, SENSOR_IMAGE_FORMAT_RAW},
	{PNULL, 0, 0, 0, 0, 0},
};

LOCAL SENSOR_TRIM_T s_ov5670_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 640, 480, 163, 840, 2040, {0, 0, 640, 480}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
};

LOCAL SENSOR_IOCTL_FUNC_TAB_T s_ov5670_ioctl_func_tab = {
	PNULL,
	_ov5670_PowerOn,
	PNULL,
	_ov5670_Identify,

	PNULL,// write register
	PNULL,// read  register
	PNULL,
	_ov5670_GetResolutionTrimTab,
	PNULL,
	PNULL,
	PNULL,

	PNULL, //_ov5670_set_brightness,
	PNULL, // _ov5670_set_contrast,
	PNULL,
	PNULL,//_ov5670_set_saturation,

	PNULL, //_ov5670_set_work_mode,
	PNULL, //_ov5670_set_image_effect,

	PNULL, //_ov5670_BeforeSnapshot,
	PNULL, //_ov5670_after_snapshot,
	PNULL,   //_ov5670_flash,
	PNULL,
	PNULL, //_ov5670_write_exposure,
	PNULL,
	PNULL, //_ov5670_write_gain,
	PNULL,
	PNULL,
	PNULL, //_ov5670_write_af,
	PNULL,
	PNULL, //_ov5670_set_awb,
	PNULL,
	PNULL,
	PNULL, //_ov5670_set_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL, //_ov5670_GetExifInfo,
	PNULL, //_ov5670_ExtFunc,
	PNULL, //_ov5670_set_anti_flicker,
	PNULL, //_ov5670_set_video_mode,
	PNULL, //pick_jpeg_stream
	PNULL,  //meter_mode
	PNULL, //get_status
	_ov5670_StreamOn,
	_ov5670_StreamOff,
	PNULL, //_ov5670_cfg_otp,
};


SENSOR_INFO_T g_at_ov5670_mipi_raw_info = {
	ov5670_I2C_ADDR_W,	// salve i2c write address
	ov5670_I2C_ADDR_R,	// salve i2c read address

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
	50,			// reset pulse width(ms)

	SENSOR_LOW_LEVEL_PWDN,	// 1: high level valid; 0: low level valid

	1,			// count of identify code
	{{0x300B, 0x56},		// supply two code to identify sensor.
	 {0x300C, 0x70}},		// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,	// voltage of avdd

	640,			// max width of source image
	480,			// max height of source image
	"ov5670",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_B,// pattern of input image form sensor;

	s_ov5670_resolution_Tab_RAW,	// point to resolution table information structure
	&s_ov5670_ioctl_func_tab,	// point to ioctl function table
	&s_at_ov5670_mipi_raw_info_ptr,		// information and table about Rawrgb sensor
	NULL,			//&g_ov5670_ext_info,                // extend information about sensor
	SENSOR_AVDD_1800MV,	// iovdd
	SENSOR_AVDD_1200MV,	// dvdd
	1,			// skip frame num before preview
	2,			// skip frame num before capture
	0,			// deci frame num during preview
	0,			// deci frame num during video preview

	0,
	0,
	0,
	0,
	0,
	{SENSOR_INTERFACE_TYPE_CSI2, 2, 10, 0},
	NULL,			//s_ov5670_video_info,
	3,			// skip frame num while change setting
	60,//48,			// horizontal view angle
	60,//48,			// vertical view angle
};

/*LOCAL struct sensor_raw_info* Sensor_GetContext(void)
{
	return s_at_ov5670_mipi_raw_info_ptr;
}
*/
LOCAL unsigned long _ov5670_GetResolutionTrimTab(unsigned long param)
{
	UNUSED(param);
	SENSOR_PRINT("0x%lx", (unsigned long)s_ov5670_Resolution_Trim_Tab);
	return (unsigned long) s_ov5670_Resolution_Trim_Tab;
}

LOCAL unsigned long _ov5670_PowerOn(unsigned long power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_at_ov5670_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_at_ov5670_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_at_ov5670_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_at_ov5670_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_at_ov5670_mipi_raw_info.reset_pulse_level;

	if (SENSOR_TRUE == power_on) {
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_SetResetLevel(reset_level);
		Sensor_PowerDown(power_down);
		// Open power
		usleep(1000);
		Sensor_SetAvddVoltage(avdd_val);
		usleep(1000);
		Sensor_SetIovddVoltage(iovdd_val);
		Sensor_SetResetLevel(!reset_level);
		usleep(2000);
		Sensor_SetDvddVoltage(dvdd_val);
		usleep(1000);
		//_dw9174_SRCInit(2);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(2*1000);
		Sensor_PowerDown(!power_down);
		usleep(2*1000);
	} else {
		Sensor_PowerDown(power_down);
		usleep(1000);
		Sensor_SetDvddVoltage(SENSOR_AVDD_CLOSED);
		usleep(1000);
		Sensor_SetIovddVoltage(SENSOR_AVDD_CLOSED);
		Sensor_SetResetLevel(reset_level);
		usleep(1000);
		Sensor_SetAvddVoltage(SENSOR_AVDD_CLOSED);
		usleep(1000);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		//_dw9174_SRCDeinit();
	}
	SENSOR_PRINT("SENSOR_OV5670: _ov5670_Power_On(1:on, 0:off): %d", power_on);
	return SENSOR_SUCCESS;
}

LOCAL uint32_t _ov5670_com_Identify_otp(void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;
	UNUSED(param_ptr);
	SENSOR_PRINT("SENSOR_OV5670: _ov5670_com_Identify_otp");

	/*read param id from sensor omap*/
	param_id=OV5670_RAW_PARAM_COM;

	if(OV5670_RAW_PARAM_COM==param_id){
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}
LOCAL uint32_t _ov5670_GetRawInof(void)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_ov5670_raw_param_tab;
	uint32_t param_id;
	uint32_t i=0x00;

	/*read param id from sensor omap*/
	param_id=OV5670_RAW_PARAM_COM;

	for(i=0x00; ; i++)
	{
		g_module_id = i;
		if(RAW_INFO_END_ID==tab_ptr[i].param_id){
			if(NULL==s_at_ov5670_mipi_raw_info_ptr){
				SENSOR_PRINT("SENSOR_OV5670: ov5647_GetRawInof no param error");
				rtn=SENSOR_FAIL;
			}
			SENSOR_PRINT("SENSOR_OV5670: ov5670_GetRawInof end");
			break;
		}
		else if(PNULL!=tab_ptr[i].identify_otp){
			if(SENSOR_SUCCESS==tab_ptr[i].identify_otp(0))
			{
				s_at_ov5670_mipi_raw_info_ptr = tab_ptr[i].info_ptr;
				SENSOR_PRINT("SENSOR_OV5670: ov5670_GetRawInof success");
				break;
			}
		}
	}

	return rtn;
}
LOCAL unsigned long _ov5670_Identify(unsigned long param)
{
#define ov5670_PID_VALUE    0x56
#define ov5670_PID_ADDR     0x300B
#define ov5670_VER_VALUE    0x70
#define ov5670_VER_ADDR     0x300C

	uint8_t pid_value = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	UNUSED(param);
	SENSOR_PRINT("SENSOR_OV5670: mipi raw identify\n");

	pid_value = Sensor_ReadReg(ov5670_PID_ADDR);

	if (ov5670_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(ov5670_VER_ADDR);
		SENSOR_PRINT("SENSOR_OV5670: Identify: PID = %x, VER = %x", pid_value, ver_value);
		if (ov5670_VER_VALUE == ver_value) {
			ret_value=_ov5670_GetRawInof();
//			Sensor_InitRawTuneInfo();
			ret_value = SENSOR_SUCCESS;
			SENSOR_PRINT("SENSOR_OV5670: this is ov5670 sensor !");
		} else {
			SENSOR_PRINT
			    ("SENSOR_OV5670: Identify this is OV%x%x sensor !", pid_value, ver_value);
		}
	} else {
		SENSOR_PRINT("SENSOR_OV5670: identify fail,pid_value=%d", pid_value);
	}

	return ret_value;
}

LOCAL unsigned long _ov5670_StreamOn(unsigned long param)
{
	SENSOR_PRINT("SENSOR_OV5670: StreamOn");
	UNUSED(param);
	Sensor_WriteReg(0x0100, 0x01);

	return 0;
}

LOCAL unsigned long _ov5670_StreamOff(unsigned long param)
{
	SENSOR_PRINT("SENSOR_OV5670: StreamOff");
	UNUSED(param);
	Sensor_WriteReg(0x0100, 0x00);
	usleep(80*1000);

	return 0;
}
