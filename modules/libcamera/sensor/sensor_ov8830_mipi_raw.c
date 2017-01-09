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
#include "sensor_ov8830_raw_param.c"


#define ov8830_I2C_ADDR_W        (0x20>>1)
#define ov8830_I2C_ADDR_R         (0x21>>1)

#define OV8830_MIN_FRAME_LEN_PRV  0x5e8

LOCAL uint32_t _ov8830_GetResolutionTrimTab(uint32_t param);
LOCAL uint32_t _ov8830_PowerOn(uint32_t power_on);
LOCAL uint32_t _ov8830_Identify(uint32_t param);
LOCAL uint32_t _ov8830_BeforeSnapshot(uint32_t param);
LOCAL uint32_t _ov8830_after_snapshot(uint32_t param);
LOCAL uint32_t _ov8830_StreamOn(uint32_t param);
LOCAL uint32_t _ov8830_StreamOff(uint32_t param);
LOCAL uint32_t _ov8830_write_exposure(uint32_t param);
LOCAL uint32_t _ov8830_write_gain(uint32_t param);
LOCAL uint32_t _ov8830_write_af(uint32_t param);
LOCAL uint32_t _ov8830_flash(uint32_t param);


static uint32_t g_flash_mode_en = 0;
static uint32_t g_af_slewrate = 1;

LOCAL const SENSOR_REG_T ov8830_common_init[] = {
//@@Initialization (Global Setting)
////Slave_ID=0x6c
{0x0100,0x00},//software standby
{0x0103,0x01},//software reset
{SENSOR_WRITE_DELAY, 0x0a},

////delay(5ms)
{0x0102,0x01},
//system control
{0x3001,0x2a},//drive 2x,d1_frex_in_disable
{0x3002,0x88},//vsync,strobe output enable,//href,//pclk,//frex,//SIOD output disable
{0x3005,0x00},
{0x301b,0xb4},//sclk_bist20,sclk_snr_sync on
{0x301d,0x02},//frex_mask_arb on
{0x3021,0x00},//internal regulator on
{0x3022,0x00},//pad_pixelvdd_sel = 0
//PLL
{0x3081,0x02},
{0x3083,0x01},
{0x3093,0x00},//pll2_sel5
{0x3098,0x03},//pll3_prediv
{0x3099,0x1e},//pll3_multiplier
{0x309a,0x00},//pll3_divs
{0x309b,0x00},//pll3_div
{0x30a2,0x01},
{0x30b0,0x05},
{0x30b2,0x00},
{0x30b5,0x04},//pll1_op_pix_div
{0x30b6,0x01},//pll1_op_sys_div
{0x3104,0xa1},//sclk_sw2pll
{0x3106,0x01},
// Exposure/Gain
{0x3503,0x07},//AGC manual,//AEC manual
{0x3504,0x00},
{0x3505,0x30},
{0x3506,0x00},//ShortExposure[10:16],for inter line HDR mode
{0x3508,0x80},//ShortExposure[15:8]
{0x3509,0x10},//ShortExposure[7:0]
{0x350a,0x00},
{0x350b,0x38},
// analog
{0x3600,0x78},
{0x3601,0x02},
{0x3602,0x1c},
{0x3604,0x38},
{0x3620,0x64},
{0x3621,0xb5},
{0x3622,0x03},
{0x3625,0x64},
{0x3630,0x55},
{0x3631 ,0xd2},
{0x3632 ,0x00},
{0x3633 ,0x34},
{0x3634 ,0x03},
{0x3660 ,0x80},
{0x3662 ,0x10},
{0x3665 ,0x00},
{0x3666 ,0x00},
{0x3667 ,0x00},
{0x366a ,0x80},
{0x366c,0x00},
{0x366d ,0x00},
{0x366e ,0x00},
{0x366f ,0x20},
{0x3680 ,0xe0},
{0x3681 ,0x00},
// sensor control
{0x3701 ,0x14},
{0x3702 ,0xbf},
{0x3703 ,0x8c},
{0x3704 ,0x78},
{0x3705 ,0x02},
{0x370a ,0x00},
{0x370b ,0x20},
{0x370c ,0x0c},
{0x370d ,0x11},
{0x370e ,0x00},
{0x370f ,0x00},
{0x3710 ,0x00},
{0x371c ,0x01},
{0x371f ,0x0c},
{0x3721 ,0x00},
{0x3724 ,0x10},
{0x3726 ,0x00},
{0x372a ,0x01},
{0x3730 ,0x18},
{0x3738 ,0x22},
{0x3739 ,0x08},
{0x373a ,0x51},
{0x373b ,0x02},
{0x373c ,0x20},
{0x373f ,0x02},
{0x3740 ,0x42},
{0x3741 ,0x02},
{0x3742 ,0x18},
{0x3743 ,0x01},
{0x3744 ,0x02},
{0x3747 ,0x10},
{0x374c ,0x04},
{0x3751 ,0xf0},
{0x3752 ,0x00},
{0x3753 ,0x00},
{0x3754 ,0xc0},
{0x3755 ,0x00},
{0x3756 ,0x1a},
{0x3758 ,0x00},
{0x3759 ,0x0f},
{0x375c ,0x04},
{0x3767 ,0x01},
{0x376b ,0x44},
{0x3774 ,0x10},
{0x3776 ,0x00},
{0x377f ,0x08},
// PSRAM
{0x3780 ,0x22},
{0x3781 ,0x0c},
{0x3784 ,0x2c},
{0x3785 ,0x1e},
{0x378f ,0xf5},
{0x3791 ,0xb0},
{0x3795 ,0x00},
{0x3796 ,0x64},
{0x3797 ,0x11},
{0x3798 ,0x30},
{0x3799 ,0x41},
{0x379a ,0x07},
{0x379b ,0xb0},
{0x379c ,0x0c},
// Frex control
{0x37c5 ,0x00},
{0x37c6 ,0xa0},
{0x37c7 ,0x00},
{0x37c9 ,0x00},
{0x37ca ,0x00},
{0x37cb ,0x00},
{0x37cc ,0x00},
{0x37cd ,0x00},
{0x37ce ,0x01},
{0x37cf ,0x00},
{0x37d1 ,0x01},
{0x37de ,0x00},
{0x37df ,0x00},
// timing
{0x3823 ,0x00},
{0x3824 ,0x00},
{0x3825 ,0x00},
{0x3826 ,0x00},
{0x3827 ,0x00},
{0x382a ,0x04},
{0x3a06 ,0x00},
{0x3a07 ,0xf8},
//strobe control
{0x3b00 ,0x00},
{0x3b02 ,0x00},
{0x3b03 ,0x00},
{0x3b04 ,0x00},
{0x3b05 ,0x00},
// OTP
{0x3d00 ,0x00},//OTP buffer
{0x3d01 ,0x00},
{0x3d02 ,0x00},
{0x3d03 ,0x00},
{0x3d04 ,0x00},
{0x3d05 ,0x00},
{0x3d06 ,0x00},
{0x3d07 ,0x00},
{0x3d08 ,0x00},
{0x3d09 ,0x00},
{0x3d0a ,0x00},
{0x3d0b ,0x00},
{0x3d0c ,0x00},
{0x3d0d ,0x00},
{0x3d0e ,0x00},
{0x3d0f ,0x00},//OTP buffer
{0x3d80 ,0x00},//OTP program
{0x3d81 ,0x00},//OTP load
{0x3d84 ,0x00},//OTP program enable,//manual memory bank disable,//bank sel 0
// BLC
{0x4000 ,0x18},
{0x4001 ,0x04},//BLC start line 4
{0x4002 ,0x45},//BLC auto,//reset frame number = 5
{0x4004 ,0x02},//2 lines for BLC
{0x4006 ,0x16},//DC BLC coefficient
{0x4008 ,0x20},//First part BLC calculation start line address = 2,//DC BLC on
{0x4009 ,0x10},//BLC target
{0x4101 ,0x12},
{0x4104 ,0x5b},
//format control
{0x4307 ,0x30},//embed_line_st = 3
{0x4315 ,0x00},//Vsync trigger signal to Vsync output delay[15:8]
{0x4511 ,0x05},
//MIPI control
{0x4805 ,0x01},
{0x4806 ,0x00},
{0x481f ,0x36},
{0x4831 ,0x6c},
// LVDS
{0x4a00 ,0xaa},//SYNC code enable when only 1 lane,//Bit swap,//SAV first enable
{0x4a03 ,0x01},//Dummy data0[7:0]
{0x4a05 ,0x08},//Dummy data1[7:0]
{0x4a0a ,0x88},
// ISP
{0x5000 ,0x06},//LENC off,//BPC on,//WPC on
{0x5001 ,0x01},//MWB on
{0x5002 ,0x80},//scale on
{0x5003 ,0x20},
{0x5013 ,0x00},//LSB disable
{0x5046 ,0x4a},//ISP SOF sel - VSYNC
// DPC
{0x5780 ,0x1c},
{0x5786 ,0x20},
{0x5787 ,0x10},
{0x5788 ,0x18},
{0x578a ,0x04},
{0x578b ,0x02},
{0x578c ,0x02},
{0x578e ,0x06},
{0x578f ,0x02},
{0x5790 ,0x02},
{0x5791 ,0xff},
{0x5a08 ,0x02},//window control
{0x5e00 ,0x00},//color bar off
{0x5e10 ,0x0c},
{0x5000 ,0x06},//lenc off,//bpc on,//wpc on
// MWB
{0x5001 ,0x01},//MWB on
{0x3400 ,0x04},//red gain h
{0x3401 ,0x00},//red gain l
{0x3402 ,0x04},//green gain h
{0x3403 ,0x00},//green gain l
{0x3404 ,0x04},//blue gain h
{0x3405 ,0x00},//blue gain l
{0x3406 ,0x01},//MWB on
// BLC
{0x4000 ,0x18},//BLC on,//BLC window as BLC,//BLC ratio from register
{0x4002 ,0x45},//Format change trigger off,//auto on,//reset frame number = 5
{0x4005 ,0x18},//no black line output,//blc_man_1_en,//BLC after reset,//then stop,//BLC triggered by gain change
{0x4009 ,0x10},//BLC target
{0x3503 ,0x07},//AEC manual,//AGC manual
{0x3500 ,0x00},//Exposure[19:16]
{0x3501 ,0x29},//Exposure[15:8]
{0x3502 ,0x00},//Exposure[7:0]
{0x350b ,0x78},//Gain[7:0]
// MIPI data rate = 640Mbps
{0x30b3 ,0x50},//pll1_multiplier
{0x30b4 ,0x03},//pll1_prediv
{0x30b5 ,0x04},//pll1_op_pix_div
{0x30b6 ,0x01},//pll1_op_sys_div
{0x4837 ,0x0d},//MIPI global timing
};


const unsigned short ov8830_1632x1224_setting[][2] =
{
//@@5.1.2.1 Raw 10bit 1632*1224 30fps 4lane 640M bps/lane
//{0x0100, 0x00}, //software standby
{0x3708, 0xe2}, //sensor control
{0x3709, 0x03}, //sensor control
{0x3800, 0x00}, //HS = 8
{0x3801, 0x08}, //HS
{0x3802, 0x00}, //VS = 8
{0x3803, 0x08}, //VS
{0x3804, 0x0c}, //HE = 3287
{0x3805, 0xd7}, //HE
{0x3806, 0x09}, //VE = 2471
{0x3807, 0xa7}, //VE
{0x3808, 0x06}, //HO = 1632
{0x3809, 0x60}, //HO
{0x380a, 0x04}, //VO = 1224
{0x380b, 0xc8}, //VO
{0x380c, 0x0e}, //HTS = 3608
{0x380d, 0x18}, //HTS
{0x380e, 0x04}, //VTS = 1260
{0x380f, 0xec}, //VTS
{0x3810, 0x00}, //H OFFSET = 4
{0x3811, 0x04}, //H OFFSET
{0x3812, 0x00}, //V OFFSET = 4
{0x3813, 0x04}, //V OFFSET
{0x3814, 0x31}, //X INC
{0x3815, 0x31}, //Y INC
{0x3820, 0x11},
{0x3821, 0x0f},
{0x3a04, 0x04},
{0x3a05, 0xc9},
{0x4005, 0x1a},
{0x4512, 0x00}, //vertical average
{0x3011, 0x41}, //MIPI 4 lane, MIPI enable
{0x3015, 0x08}, //MIPI 4 lane on, select MIPI
//SCLK = 140Mhz
{0x3090, 0x03}, //pll2_prediv
{0x3091, 0x23}, //pll2_multiplier
{0x3092, 0x01}, //pll2_divs
{0x3093, 0x00}, //pll2_seld5
//MIPI data rate = 640Mbps
//{0x30b3, 0x50}, //pll1_multiplier
//{0x30b4, 0x03}, //pll1_prediv
//{0x30b5, 0x04}, //pll1_op_pix_div
//{0x30b6, 0x01}, //pll1_op_sys_div
//{0x4837, 0x0d}, //MIPI global timing
};

const unsigned short ov8830_3264x2448_setting[][2] =
{
//5.1.2.2 Raw 10bit 3264*2448 24fps 4lane 640M bps/lane
{0x0100 ,0x00},//software standby
{0x3708 ,0xe3},//sensor control
{0x3709 ,0xc3},//sensor control
{0x3800 ,0x00},//HS = 12
{0x3801 ,0x0c},//HS
{0x3802 ,0x00},//VS = 12
{0x3803 ,0x0c},//VS
{0x3804 ,0x0c},//HE = 3283
{0x3805 ,0xd3},//HE
{0x3806 ,0x09},//VE = 2467
{0x3807 ,0xa3},//VE
{0x3808 ,0x0c},//HO = 3264
{0x3809 ,0xc0},//HO
{0x380a ,0x09},//VO = 2448
{0x380b ,0x90},//VO
{0x380c ,0x0e},//HTS = 3608
{0x380d ,0x18},//HTS
{0x380e ,0x09},//VTS = 2484
{0x380f ,0xb4},//VTS
{0x3810 ,0x00},//H OFFSET = 4
{0x3811 ,0x04},//H OFFSET
{0x3812 ,0x00},//V OFFSET = 4
{0x3813 ,0x04},//V OFFSET
{0x3814 ,0x11},//X INC
{0x3815 ,0x11},//Y INC
{0x3820 ,0x10},
{0x3821 ,0x0e},
{0x3a04 ,0x09},
{0x3a05 ,0xa9},
{0x4004 ,0x08},
{0x4512 ,0x01},//vertical average
{0x3011 ,0x41},//MIPI 4 lane,//MIPI enable
{0x3015 ,0x08},//MIPI 4 lane on,//select MIPI
// SCLK = 216Mhz
{0x3090 ,0x02},//pll2_prediv
{0x3091 ,0x12},//pll2_multiplier
{0x3092 ,0x00},//pll2_divs
{0x3093 ,0x00},//pll2_seld5
{0x3094 ,0x00},
// MIPI data rate = 640Mbps
//{0x30b3 ,0x50},//pll1_multiplier
//{0x30b4 ,0x03},//pll1_prediv
//{0x30b5 ,0x04},//pll1_op_pix_div
//{0x30b6 ,0x01},//pll1_op_sys_div
//{0x4837 ,0x0d},//MIPI global timing
};

LOCAL SENSOR_REG_TAB_INFO_T s_ov8830_resolution_Tab_RAW[] = {
	{ADDR_AND_LEN_OF_ARRAY(ov8830_common_init), 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(ov8830_1632x1224_setting), 1632, 1224, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(ov8830_3264x2448_setting), 3264, 2448, 24, SENSOR_IMAGE_FORMAT_RAW},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

LOCAL SENSOR_TRIM_T s_ov8830_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0},
	{0, 0, 1632, 1224, 264, 90},
	{0, 0, 3264, 2448, 268, 82},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0}
};

static struct sensor_raw_info s_ov8830_mipi_raw_info={
	&s_ov8830_version_info,
	&s_ov8830_tune_info,
	&s_ov8830_fix_info,
	&s_ov8830_cali_info,
};

struct sensor_raw_info* s_ov8830_mipi_raw_info_ptr=&s_ov8830_mipi_raw_info;

LOCAL SENSOR_IOCTL_FUNC_TAB_T s_ov8830_ioctl_func_tab = {
	PNULL,
	_ov8830_PowerOn,
	PNULL,
	_ov8830_Identify,

	PNULL,			// write register
	PNULL,			// read  register
	PNULL,
	_ov8830_GetResolutionTrimTab,

	// External
	PNULL,
	PNULL,
	PNULL,

	PNULL, //_ov8825_set_brightness,
	PNULL, // _ov8825_set_contrast,
	PNULL,
	PNULL,			//_ov8825_set_saturation,

	PNULL, //_ov8825_set_work_mode,
	PNULL, //_ov8825_set_image_effect,

	_ov8830_BeforeSnapshot,
	_ov8830_after_snapshot,
	_ov8830_flash,
	PNULL,
	_ov8830_write_exposure,
	PNULL,
	_ov8830_write_gain,
	PNULL,
	PNULL,
	_ov8830_write_af,
	PNULL,
	PNULL, //_ov8825_set_awb,
	PNULL,
	PNULL,
	PNULL, //_ov8825_set_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL, //_ov8825_GetExifInfo,
	PNULL,
	PNULL, //_ov8825_set_anti_flicker,
	PNULL, //_ov8825_set_video_mode,
	PNULL, //pick_jpeg_stream
	PNULL,  //meter_mode
	PNULL, //get_status
	_ov8830_StreamOn,
	_ov8830_StreamOff,
};


SENSOR_INFO_T g_ov8830_mipi_raw_info = {
	ov8830_I2C_ADDR_W,	// salve i2c write address
	ov8830_I2C_ADDR_R,	// salve i2c read address

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
	{{0x300A, 0x88},		// supply two code to identify sensor.
	 {0x300B, 0x30}},		// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,	// voltage of avdd

	3264,			// max width of source image
	2448,			// max height of source image
	"ov8830",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_B,// pattern of input image form sensor;

	s_ov8830_resolution_Tab_RAW,	// point to resolution table information structure
	&s_ov8830_ioctl_func_tab,	// point to ioctl function table
	&s_ov8830_mipi_raw_info,		// information and table about Rawrgb sensor
	NULL,			//&g_ov8825_ext_info,                // extend information about sensor
	SENSOR_AVDD_1800MV,	// iovdd
	SENSOR_AVDD_1300MV,	// dvdd
	3,			// skip frame num before preview
	3,			// skip frame num before capture
	0,			// deci frame num during preview
	0,			// deci frame num during video preview

	0,
	0,
	0,
	0,
	0,
	{SENSOR_INTERFACE_TYPE_CSI2, 4, 10, 0},
	PNULL,
	3,			// skip frame num while change setting
};

LOCAL struct sensor_raw_info* Sensor_GetContext(void)
{
	return s_ov8830_mipi_raw_info_ptr;
}

LOCAL uint32_t Sensor_InitRawTuneInfo(void)
{
	uint32_t rtn=0x00;
	struct sensor_raw_info* raw_sensor_ptr=Sensor_GetContext();
	struct sensor_raw_tune_info* sensor_ptr=raw_sensor_ptr->tune_ptr;

	raw_sensor_ptr->version_info->version_id=0x00010000;
	raw_sensor_ptr->version_info->srtuct_size=sizeof(struct sensor_raw_info);

	//bypass
	sensor_ptr->version_id=0x00010000;
	sensor_ptr->blc_bypass=0x00;
	sensor_ptr->nlc_bypass=0x01;
	sensor_ptr->lnc_bypass=0x01;
	sensor_ptr->ae_bypass=0x01;
	sensor_ptr->awb_bypass=0x01;
	sensor_ptr->bpc_bypass=0x01;
	sensor_ptr->denoise_bypass=0x01;
	sensor_ptr->grgb_bypass=0x01;
	sensor_ptr->cmc_bypass=0x01;
	sensor_ptr->gamma_bypass=0x01;
	sensor_ptr->uvdiv_bypass=0x01;
	sensor_ptr->pref_bypass=0x01;
	sensor_ptr->bright_bypass=0x01;
	sensor_ptr->contrast_bypass=0x01;
	sensor_ptr->hist_bypass=0x01;
	sensor_ptr->auto_contrast_bypass=0x01;
	sensor_ptr->af_bypass=0x01;
	sensor_ptr->edge_bypass=0x01;
	sensor_ptr->fcs_bypass=0x01;
	sensor_ptr->css_bypass=0x01;
	sensor_ptr->saturation_bypass=0x01;
	sensor_ptr->hdr_bypass=0x01;
	sensor_ptr->glb_gain_bypass=0x01;
	sensor_ptr->chn_gain_bypass=0x01;

	//blc
	sensor_ptr->blc.mode=0x00;
	sensor_ptr->blc.offset[0].r=0x0f;
	sensor_ptr->blc.offset[0].gr=0x0f;
	sensor_ptr->blc.offset[0].gb=0x0f;
	sensor_ptr->blc.offset[0].b=0x0f;
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
	sensor_ptr->ae.target_lum=60;
	sensor_ptr->ae.target_zone=8;
	sensor_ptr->ae.quick_mode=1;
	sensor_ptr->ae.smart=0;
	sensor_ptr->ae.smart_rotio=255;
	sensor_ptr->ae.ev[0]=0xe8;
	sensor_ptr->ae.ev[1]=0xf0;
	sensor_ptr->ae.ev[2]=0xf8;
	sensor_ptr->ae.ev[3]=0x00;
	sensor_ptr->ae.ev[4]=0x08;
	sensor_ptr->ae.ev[5]=0x10;
	sensor_ptr->ae.ev[6]=0x18;
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
	sensor_ptr->awb.g_gain[0]=0x3fc;
	sensor_ptr->awb.b_gain[0]=0x600;
	sensor_ptr->awb.r_gain[1]=0x480;
	sensor_ptr->awb.g_gain[1]=0x3fc;
	sensor_ptr->awb.b_gain[1]=0xc00;
	sensor_ptr->awb.r_gain[2]=0x3fc;
	sensor_ptr->awb.g_gain[2]=0x3fc;
	sensor_ptr->awb.b_gain[2]=0x3fc;
	sensor_ptr->awb.r_gain[3]=0x3fc;
	sensor_ptr->awb.g_gain[3]=0x3fc;
	sensor_ptr->awb.b_gain[3]=0x3fc;
	sensor_ptr->awb.r_gain[4]=0x480;
	sensor_ptr->awb.g_gain[4]=0x3fc;
	sensor_ptr->awb.b_gain[4]=0x800;
	sensor_ptr->awb.r_gain[5]=0x700;
	sensor_ptr->awb.g_gain[5]=0x3fc;
	sensor_ptr->awb.b_gain[5]=0x500;
	sensor_ptr->awb.r_gain[6]=0xa00;
	sensor_ptr->awb.g_gain[6]=0x3fc;
	sensor_ptr->awb.b_gain[6]=0x4c0;
	sensor_ptr->awb.r_gain[7]=0x3fc;
	sensor_ptr->awb.g_gain[7]=0x3fc;
	sensor_ptr->awb.b_gain[7]=0x3fc;
	sensor_ptr->awb.r_gain[8]=0x3fc;
	sensor_ptr->awb.g_gain[8]=0x3fc;
	sensor_ptr->awb.b_gain[8]=0x3fc;
	sensor_ptr->awb.target_zone=0x40;

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

	//bpc
	sensor_ptr->bpc.flat_thr=80;
	sensor_ptr->bpc.std_thr=20;
	sensor_ptr->bpc.texture_thr=2;

	// denoise
	sensor_ptr->denoise.write_back=0x02;
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
	sensor_ptr->saturation.factor[0]=0x40;
	sensor_ptr->saturation.factor[1]=0x40;
	sensor_ptr->saturation.factor[2]=0x40;
	sensor_ptr->saturation.factor[3]=0x40;
	sensor_ptr->saturation.factor[4]=0x40;
	sensor_ptr->saturation.factor[5]=0x40;
	sensor_ptr->saturation.factor[6]=0x40;
	sensor_ptr->saturation.factor[7]=0x40;
	sensor_ptr->saturation.factor[8]=0x40;
	sensor_ptr->saturation.factor[9]=0x40;
	sensor_ptr->saturation.factor[10]=0x40;
	sensor_ptr->saturation.factor[11]=0x40;
	sensor_ptr->saturation.factor[12]=0x40;
	sensor_ptr->saturation.factor[13]=0x40;
	sensor_ptr->saturation.factor[14]=0x40;
	sensor_ptr->saturation.factor[15]=0x40;

	//af info
	sensor_ptr->af.max_step=1024;
	sensor_ptr->af.stab_period=10;

	//edge
	sensor_ptr->edge.info[0].detail_thr=0x03;
	sensor_ptr->edge.info[0].smooth_thr=0x05;
	sensor_ptr->edge.info[0].strength=10;
	sensor_ptr->edge.info[1].detail_thr=0x03;
	sensor_ptr->edge.info[1].smooth_thr=0x05;
	sensor_ptr->edge.info[1].strength=10;
	sensor_ptr->edge.info[2].detail_thr=0x03;
	sensor_ptr->edge.info[2].smooth_thr=0x05;
	sensor_ptr->edge.info[2].strength=10;
	sensor_ptr->edge.info[3].detail_thr=0x03;
	sensor_ptr->edge.info[3].smooth_thr=0x05;
	sensor_ptr->edge.info[3].strength=10;
	sensor_ptr->edge.info[4].detail_thr=0x03;
	sensor_ptr->edge.info[4].smooth_thr=0x05;
	sensor_ptr->edge.info[4].strength=10;
	sensor_ptr->edge.info[5].detail_thr=0x03;
	sensor_ptr->edge.info[5].smooth_thr=0x05;
	sensor_ptr->edge.info[5].strength=10;

	//emboss
	sensor_ptr->emboss.step=0x00;

	//global gain
	sensor_ptr->global.gain=0x40;

	//chn gain
	sensor_ptr->chn.r_gain=0x40;
	sensor_ptr->chn.g_gain=0x40;
	sensor_ptr->chn.b_gain=0x40;
	sensor_ptr->chn.r_offset=0x00;
	sensor_ptr->chn.r_offset=0x00;
	sensor_ptr->chn.r_offset=0x00;

	return rtn;
}


LOCAL uint32_t _ov8830_GetResolutionTrimTab(uint32_t param)
{
	SENSOR_PRINT("0x%x",  (uint32_t)s_ov8830_Resolution_Trim_Tab);
	return (uint32_t) s_ov8830_Resolution_Trim_Tab;
}
LOCAL uint32_t _ov8830_PowerOn(uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_ov8830_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_ov8830_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_ov8830_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_ov8830_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_ov8830_mipi_raw_info.reset_pulse_level;

	if (SENSOR_TRUE == power_on) {
		Sensor_PowerDown(power_down);
		// Open power
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		usleep(20*1000);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(10*1000);
		Sensor_PowerDown(!power_down);
		// Reset sensor
		Sensor_Reset(reset_level);
	} else {
		Sensor_PowerDown(power_down);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
	}
	SENSOR_PRINT("SENSOR_ov8830: _ov8830_Power_On(1:on, 0:off): %d", power_on);
	return SENSOR_SUCCESS;
}

LOCAL uint32_t _ov8830_Identify(uint32_t param)
{
#define ov8830_PID_VALUE    0x88
#define ov8830_PID_ADDR     0x300A
#define ov8830_VER_VALUE    0x30
#define ov8830_VER_ADDR     0x300B

	uint8_t pid_value = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("SENSOR_ov8830: mipi raw identify\n");

	pid_value = Sensor_ReadReg(ov8830_PID_ADDR);
	if (ov8830_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(ov8830_VER_ADDR);
		SENSOR_PRINT("SENSOR_ov8830: Identify: PID = %x, VER = %x", pid_value, ver_value);
		if (ov8830_VER_VALUE == ver_value) {
			Sensor_InitRawTuneInfo();
			ret_value = SENSOR_SUCCESS;
			SENSOR_PRINT("SENSOR_ov8830: this is ov8830 sensor !");
		} else {
			SENSOR_PRINT("SENSOR_ov8830: Identify this is OV%x%x sensor !", pid_value, ver_value);
		}
	} else {
		SENSOR_PRINT("SENSOR_ov8830: identify fail,pid_value=%d", pid_value);
	}
	return ret_value;
}

LOCAL uint32_t _ov8830_write_exposure(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	SENSOR_PRINT("SENSOR_ov8830: _ov8830_write_exposure= 0x%x", param);

	return ret_value;
}

LOCAL uint32_t _ov8830_write_gain(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t value=0x00;
	SENSOR_PRINT("SENSOR_ov8830: _ov8830_write_gain = 0x%x", param);

	return ret_value;
}

LOCAL uint32_t _ov8830_write_af(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint16_t reg_val = 0x0;

	SENSOR_PRINT("SENSOR_ov8830: _write_af = 0x%x", param);

	return ret_value;
}

LOCAL uint32_t _ov8830_BeforeSnapshot(uint32_t param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	param = param & 0xffff;

	SENSOR_PRINT("SENSOR_ov8830: BeforeSnapshot : %d",param);



	return SENSOR_SUCCESS;
}

LOCAL uint32_t _ov8830_after_snapshot(uint32_t param)
{
	SENSOR_PRINT("SENSOR_ov8830: after_snapshot mode:%d", param);


	return SENSOR_SUCCESS;
}

LOCAL uint32_t _ov8830_flash(uint32_t param)
{
	SENSOR_PRINT("Start:param=%d", param);

	SENSOR_PRINT_HIGH("end");

	return SENSOR_SUCCESS;
}

LOCAL uint32_t _ov8830_StreamOn(uint32_t param)
{
	SENSOR_PRINT("SENSOR_ov8830: StreamOn");

	Sensor_WriteReg(0x0100, 0x01);

	return 0;
}

LOCAL uint32_t _ov8830_StreamOff(uint32_t param)
{
	SENSOR_PRINT("SENSOR_ov8830: StreamOff");

	Sensor_WriteReg(0x0100, 0x00);
	usleep(50*1000);

	return 0;
}

