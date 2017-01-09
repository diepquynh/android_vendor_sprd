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
 * V1.0
 */

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"
#include "isp_param_file_update.h"
#include "sensor_ov5670_denoise.c"
//#include "sensor_ov5670_raw_param_v3.c"
#include "ov5670_parameters/sensor_ov5670_raw_param_main.c"
#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
//#include "af_dw9714.h"
#endif

//#define FEATURE_OTP
#ifdef FEATURE_OTP
#include "sensor_ov5670_otp.c"
#endif

#define SENSOR_NAME			"ov5670"
#define I2C_SLAVE_ADDR			0x10

#define ov5670_PID_ADDR			0x300b
#define ov5670_PID_VALUE		0x56
#define ov5670_VER_ADDR			0x300c
#define ov5670_VER_VALUE		0x70

/* sensor parameters begin */
#define SNAPSHOT_WIDTH			2592
#define SNAPSHOT_HEIGHT			1944
#define PREVIEW_WIDTH			1280
#define PREVIEW_HEIGHT			960

#define LANE_NUM			2
#define RAW_BITS			10

#define SNAPSHOT_MIPI_PER_LANE_BPS	960
#define PREVIEW_MIPI_PER_LANE_BPS	560

#define SNAPSHOT_LINE_TIME		163
#define PREVIEW_LINE_TIME		163
#define SNAPSHOT_FRAME_LENGTH		2045
#define PREVIEW_FRAME_LENGTH		1022

/* please ref your spec */
#define FRAME_OFFSET			6
#define SENSOR_MAX_GAIN			0x1fff
#define SENSOR_BASE_GAIN		(0x10<<3)
#define SENSOR_MIN_SHUTTER		4

/* please ref your spec
 * 1 : average binning
 * 2 : sum-average binning
 * 4 : sum binning
 */
#define BINNING_FACTOR			2

/* please ref spec
 * 1: sensor auto caculate
 * 0: driver caculate
 */
#define SUPPORT_AUTO_FRAME_LENGTH	0
/* sensor parameters end */

/* isp parameters, please don't change it*/
#define ISP_BASE_GAIN			128
/* please don't change it */
#define EX_MCLK				24

struct hdr_info_t {
	cmr_uint capture_max_shutter;
	cmr_uint capture_shutter;
	cmr_uint capture_gain;
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
static cmr_uint s_current_default_frame_length;
static cmr_uint s_fr_len = 0;
static cmr_uint s_shutter = 0;
struct sensor_ev_info_t s_sensor_ev_info;

#ifdef FEATURE_OTP
#define OV5670_RAW_PARAM_COM      0x0000
static uint32_t g_module_id = 0;

static const struct raw_param_info_tab s_ov5670_raw_param_tab[]={
    //{OV5670_RAW_PARAM_Sunny, &s_ov5670_mipi_raw_info, _ov5670_Sunny_Identify_otp, update_otp_wb},
    //{OV5670_RAW_PARAM_Truly, &s_ov5670_mipi_raw_info, _ov5670_Truly_Identify_otp, update_otp_wb},
    {OV5670_RAW_PARAM_COM, &s_ov5670_mipi_raw_info, _ov5670_Identify_otp, PNULL/*ov5670_apply_otp*/},
    {RAW_INFO_END_ID, PNULL, PNULL, PNULL}
};
#endif

static SENSOR_IOCTL_FUNC_TAB_T s_ov5670_ioctl_func_tab;
struct sensor_raw_info *s_ov5670_mipi_raw_info_ptr = &s_ov5670_mipi_raw_info;

static const SENSOR_REG_T ov5670_init_setting[] = {
	//{0x0103, 0x01}, // software reset
	{0x0100, 0x00}, // software standby
	{0x0300, 0x04}, // PLL
	{0x0301, 0x00},
	{0x0302, 0x78},
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
#ifdef CONFIG_CAMERA_ISP_VERSION_V4
	{0x4818, 0x00},
	{0x4819, 0xC8},
	{0x4837, 0x10}, // clock period of pclk2x
#else
	{0x4837, 0x11}, // clock period of pclk2x
#endif
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

static const SENSOR_REG_T ov5670_preview_setting[] = {
};

static const SENSOR_REG_T ov5670_snapshot_setting[] = {
	//Capture 2592x1944 30fps 24M MCLK 2lane 960Mbps/lane
	{0x3501, 0x7b}, // exposore M
	{0x3623, 0x00}, // analog control
	{0x366e, 0x10}, // analog control
	{0x370b, 0x1b}, // sensor control
	{0x3808, 0x0a}, // x output size H
	{0x3809, 0x20}, // x output size L
	{0x380a, 0x07}, // y outout size H
	{0x380b, 0x98}, // y output size L
	{0x380c, 0x06}, // HTS H
	{0x380d, 0x8c}, // HTS L
	{0x380e, 0x07}, // VTS H
	{0x380f, 0xfd}, // VTS L
	{0x3814, 0x01}, // x inc odd
#ifndef CONFIG_CAMERA_IMAGE_180
	{0x3820, 0x86}, // vflip on
	{0x3821, 0x40}, // hsync_en_o, mirror off, dig_bin off
	{0x450b, 0x20}, // need to set when flip
#else
	{0x3820, 0x80}, // vflip off
	{0x3821, 0x46}, // hsync_en_o, mirror on, dig_bin off
#endif
	{0x382a, 0x01}, // y inc odd
	{0x4009, 0x0d}, // BLC, black line end line
	{0x400a, 0x02}, // BLC, offset trigger threshold H
	{0x400b, 0x00}, // BLC, offset trigger threshold L
	{0x4502, 0x40},
	{0x4508, 0xaa},
	{0x4509, 0xaa},
	{0x450a, 0x00},
	{0x4600, 0x01},
	{0x4601, 0x03},
	{0x4017, 0x08}, // BLC, offset trigger threshold
};

static SENSOR_REG_TAB_INFO_T s_ov5670_resolution_tab_raw[SENSOR_MODE_MAX] = {
	{ADDR_AND_LEN_OF_ARRAY(ov5670_init_setting), 0, 0, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},
	/*{ADDR_AND_LEN_OF_ARRAY(ov5670_preview_setting),
	 PREVIEW_WIDTH, PREVIEW_HEIGHT, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},*/
	{ADDR_AND_LEN_OF_ARRAY(ov5670_snapshot_setting),
	 SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},
};

static SENSOR_TRIM_T s_ov5670_resolution_trim_tab[SENSOR_MODE_MAX] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	/*{0, 0, PREVIEW_WIDTH, PREVIEW_HEIGHT,
	 PREVIEW_LINE_TIME, PREVIEW_MIPI_PER_LANE_BPS, PREVIEW_FRAME_LENGTH,
	 {0, 0, PREVIEW_WIDTH, PREVIEW_HEIGHT}},*/
	{0, 0, SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT,
	 SNAPSHOT_LINE_TIME, SNAPSHOT_MIPI_PER_LANE_BPS, SNAPSHOT_FRAME_LENGTH,
	 {0, 0, SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT}},
};

static const SENSOR_REG_T s_ov5670_1296X972_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

static const SENSOR_REG_T s_ov5670_2592X1944_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

static SENSOR_VIDEO_INFO_T s_ov5670_video_info[SENSOR_MODE_MAX] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{30, 30, 163, 90}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 (SENSOR_REG_T **) s_ov5670_1296X972_video_tab},
	{{{15, 15, 163, 64}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 (SENSOR_REG_T **) s_ov5670_2592X1944_video_tab},
};

/*==============================================================================
 * Description:
 * set video mode
 *
 *============================================================================*/
static cmr_uint ov5670_set_video_mode(cmr_uint param)
{
	SENSOR_REG_T_PTR sensor_reg_ptr;
	uint16_t i = 0x00;
	cmr_u32 mode;

	if (param >= SENSOR_VIDEO_MODE_MAX)
		return 0;

	if (SENSOR_SUCCESS != Sensor_GetMode(&mode)) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	if (PNULL == s_ov5670_video_info[mode].setting_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR) & s_ov5670_video_info[mode].setting_ptr[param];
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
SENSOR_INFO_T g_ov5670_mipi_raw_info = {
	/* salve i2c write address */
	(I2C_SLAVE_ADDR),
	/* salve i2c read address */
	(I2C_SLAVE_ADDR),
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
	{{ov5670_PID_ADDR, ov5670_PID_VALUE}
	 ,
	 {ov5670_VER_ADDR, ov5670_VER_VALUE}
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
	s_ov5670_resolution_tab_raw,
	/* point to ioctl function table */
	&s_ov5670_ioctl_func_tab,
	/* information and table about Rawrgb sensor */
	&s_ov5670_mipi_raw_info_ptr,
	/* extend information about sensor
	 * like &g_ov5670_ext_info
	 */
	NULL,
	/* voltage of iovdd */
	SENSOR_AVDD_1800MV,
	/* voltage of dvdd */
	SENSOR_AVDD_1200MV,
	/* skip frame num before preview */
	1,
	/* skip frame num before capture */
	2,
	/* deci frame num during preview */
	0,
	/* deci frame num during video preview */
	0,
	0,
	0,
	0,
	0,
	0,
	{SENSOR_INTERFACE_TYPE_CSI2, LANE_NUM, RAW_BITS, 0},
	0,
	/* skip frame num while change setting */
	3,
	/*horizontal view angle*/
	48,
	/*vertical view angle*/
	48,
};

/*==============================================================================
 * Description:
 * get default frame length
 *
 *============================================================================*/
static cmr_uint ov5670_get_default_frame_length(cmr_uint mode)
{
	return s_ov5670_resolution_trim_tab[mode].frame_line;
}

/*==============================================================================
 * Description:
 * read gain from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t ov5670_read_gain(void)
{
	uint16_t gain_h = 0;
	uint16_t gain_l = 0;

	gain_l = Sensor_ReadReg(0x3509);
	gain_h = Sensor_ReadReg(0x3508);

	return ((gain_h << 8) | gain_l);
}

/*==============================================================================
 * Description:
 * write gain to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov5670_write_gain(cmr_uint gain)
{
	cmr_uint gain_regH, gain_regL, change_flag;

	gain_regL = gain & 0xff;
	gain_regH = gain >> 8 & 0x1f;

	if (gain >= 1024) {
		change_flag = 0x07;
	} else if (gain >= 512) {
		change_flag = 0x03;
	} else if (gain >= 256) {
		change_flag = 0x01;
	} else {
		change_flag = 0x00;
	}

	Sensor_WriteReg(0x301d, 0xf0);
	Sensor_WriteReg(0x3209, 0x00);
	Sensor_WriteReg(0x320a, 0x01);

	//group write  hold
	//group 0:delay 0x366a for one frame
	Sensor_WriteReg(0x3208, 0x00);
	Sensor_WriteReg(0x366a, change_flag);
	Sensor_WriteReg(0x3208, 0x10);

	//group 1:all other registers( gain)
	Sensor_WriteReg(0x3208, 0x01);
	Sensor_WriteReg(0x3508, gain_regH);
	Sensor_WriteReg(0x3509, gain_regL);

	//update frame length
	Sensor_WriteReg(0x380f, (s_fr_len)&0xff);
	Sensor_WriteReg(0x380e, (s_fr_len>>0x08)&0xff);

	//update exposure
	Sensor_WriteReg(0x3502, (s_shutter<<0x04)&0xff);
	Sensor_WriteReg(0x3501, (s_shutter>>0x04)&0xff);
	Sensor_WriteReg(0x3500, (s_shutter>>0x0c)&0x0f);

	Sensor_WriteReg(0x3208, 0x11);

	//group lanch
	Sensor_WriteReg(0x320B, 0x15);
	Sensor_WriteReg(0x3208, 0xA1);

}

/*==============================================================================
 * Description:
 * read frame length from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t ov5670_read_frame_length(void)
{
	uint16_t frame_len_h = 0;
	uint16_t frame_len_l = 0;

	frame_len_h = Sensor_ReadReg(0x380e);
	frame_len_l  = Sensor_ReadReg(0x380f);

	return ((frame_len_h << 8) | frame_len_l);
}

/*==============================================================================
 * Description:
 * write frame length to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov5670_write_frame_length(cmr_uint frame_len)
{

	Sensor_WriteReg(0x380e, (frame_len >> 8) & 0xff);
	Sensor_WriteReg(0x380f, frame_len & 0xff);
}

/*==============================================================================
 * Description:
 * read shutter from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static cmr_uint ov5670_read_shutter(void)
{
	uint16_t shutter_h = 0;
	uint16_t shutter_m = 0;
	uint16_t shutter_l = 0;
	cmr_uint shutter=0;

	shutter_h = Sensor_ReadReg(0x3500);
	shutter_m = Sensor_ReadReg(0x3501);
	shutter_l =  Sensor_ReadReg(0x3502);
	shutter = ((shutter_h&0x0f) << 12) + (shutter_m << 4) + ((shutter_l >> 4)&0x0f);

	return shutter;
}

/*==============================================================================
 * Description:
 * write shutter to sensor registers
 * please pay attention to the frame length
 * please modify this function acording your spec
 *============================================================================*/
static void ov5670_write_shutter(cmr_uint shutter)
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
static uint16_t ov5670_update_exposure(cmr_uint shutter,cmr_uint dummy_line)
{
	cmr_uint dest_fr_len = 0;
	cmr_uint cur_fr_len = 0;
	cmr_uint fr_len = s_current_default_frame_length;

	if (1 == SUPPORT_AUTO_FRAME_LENGTH)
		goto write_sensor_shutter;

	dest_fr_len = ((shutter + dummy_line + FRAME_OFFSET) > fr_len) ? (shutter + dummy_line + FRAME_OFFSET) : fr_len;

	cur_fr_len = ov5670_read_frame_length();
	s_fr_len=cur_fr_len;

	if (shutter < SENSOR_MIN_SHUTTER)
		shutter = SENSOR_MIN_SHUTTER;

	if (dest_fr_len != cur_fr_len)
		//ov5670_write_frame_length(dest_fr_len);
		s_fr_len=dest_fr_len;
write_sensor_shutter:
	/* write shutter to sensor registers */
	//ov5670_write_shutter(shutter);
	s_shutter=shutter;
	return shutter;
}

/*==============================================================================
 * Description:
 * sensor power on
 * please modify this function acording your spec
 *============================================================================*/
static cmr_uint ov5670_power_on(cmr_uint power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_ov5670_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_ov5670_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_ov5670_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_ov5670_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_ov5670_mipi_raw_info.reset_pulse_level;

	if (SENSOR_TRUE == power_on) {
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
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
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(1000);
		Sensor_PowerDown(!power_down);
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
	}
	SENSOR_PRINT("SENSOR_OV5670: _ov5670_Power_On(1:on, 0:off): %ld", power_on);
	return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * identify sensor id
 * please modify this function acording your spec
 *============================================================================*/
static cmr_uint ov5670_identify(cmr_uint param)
{
	uint8_t pid_value = 0x00;
	uint8_t ver_value = 0x00;
	cmr_uint ret_value = SENSOR_FAIL;
	UNUSED(param);

	SENSOR_PRINT("mipi raw identify");

	pid_value = Sensor_ReadReg(ov5670_PID_ADDR);

	if (ov5670_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(ov5670_VER_ADDR);
		SENSOR_PRINT("Identify: PID = %x, VER = %x", pid_value, ver_value);
		if (ov5670_VER_VALUE == ver_value) {
			Sensor_InitRawTuneInfo(&g_ov5670_mipi_raw_info);
			ret_value = SENSOR_SUCCESS;
			SENSOR_PRINT_HIGH("this is ov5670 sensor");
		} else {
			SENSOR_PRINT_HIGH("Identify this is %x%x sensor", pid_value, ver_value);
		}
	} else {
		SENSOR_PRINT_HIGH("SENSOR_OV5670:identify fail, pid_value = %d", pid_value);
	}

	return ret_value;
}

/*==============================================================================
 * Description:
 * get resolution trim
 *
 *============================================================================*/
static cmr_uint ov5670_get_resolution_trim_tab(cmr_uint param)
{
	UNUSED(param);
	return (cmr_uint) s_ov5670_resolution_trim_tab;
}

/*==============================================================================
 * Description:
 * before snapshot
 * you can change this function if it's necessary
 *============================================================================*/
static cmr_uint ov5670_before_snapshot(cmr_uint param)
{
	cmr_uint cap_shutter = 0;
	cmr_uint prv_shutter = 0;
	cmr_uint gain = 0;
	cmr_uint cap_gain = 0;
	cmr_uint capture_mode = param & 0xffff;
	cmr_uint preview_mode = (param >> 0x10) & 0xffff;

	cmr_uint prv_linetime = s_ov5670_resolution_trim_tab[preview_mode].line_time;
	cmr_uint cap_linetime = s_ov5670_resolution_trim_tab[capture_mode].line_time;

	s_current_default_frame_length = ov5670_get_default_frame_length(capture_mode);
	SENSOR_PRINT("capture_mode = %ld", capture_mode);

	if (preview_mode == capture_mode) {
		cap_shutter = s_sensor_ev_info.preview_shutter;
		cap_gain = s_sensor_ev_info.preview_gain;
		goto snapshot_info;
	}

	prv_shutter = s_sensor_ev_info.preview_shutter;	//ov5670_read_shutter();
	gain = s_sensor_ev_info.preview_gain;	//ov5670_read_gain();

	Sensor_SetMode(capture_mode);
	Sensor_SetMode_WaitDone();

	cap_shutter = prv_shutter * prv_linetime / cap_linetime * BINNING_FACTOR;

	while (gain >= (2 * SENSOR_BASE_GAIN)) {
		if (cap_shutter * 2 > s_current_default_frame_length)
			break;
		cap_shutter = cap_shutter * 2;
		gain = gain / 2;
	}

	cap_shutter = ov5670_update_exposure(cap_shutter, 0);
	cap_gain = gain;
	ov5670_write_gain(cap_gain);
	SENSOR_PRINT("preview_shutter = 0x%x, preview_gain = 0x%x",
		     s_sensor_ev_info.preview_shutter, s_sensor_ev_info.preview_gain);

	SENSOR_PRINT("capture_shutter = 0x%lx, capture_gain = 0x%lx", cap_shutter, cap_gain);
snapshot_info:
	s_hdr_info.capture_shutter = cap_shutter; //ov5670_read_shutter();
	s_hdr_info.capture_gain = cap_gain; //ov5670_read_gain();
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
static cmr_uint ov5670_write_exposure(cmr_uint param)
{
	cmr_uint ret_value = SENSOR_SUCCESS;
	uint16_t exposure_line = 0x00;
	uint16_t dummy_line = 0x00;
	uint16_t mode = 0x00;

	exposure_line = param & 0xffff;
	dummy_line = (param >> 0x10) & 0x0fff;
	mode = (param >> 0x1c) & 0x0f;

	SENSOR_PRINT("current mode = %d, exposure_line = %d", mode, exposure_line);
	s_current_default_frame_length = ov5670_get_default_frame_length(mode);

	s_sensor_ev_info.preview_shutter = ov5670_update_exposure(exposure_line,dummy_line);

	return ret_value;
}

/*==============================================================================
 * Description:
 * get the parameter from isp to real gain
 * you mustn't change the funcion !
 *============================================================================*/
static cmr_uint isp_to_real_gain(cmr_uint param)
{
	cmr_uint real_gain = 0;

	real_gain = param;

	return real_gain;
}

/*==============================================================================
 * Description:
 * write gain value to sensor
 * you can change this function if it's necessary
 *============================================================================*/
static cmr_uint ov5670_write_gain_value(cmr_uint param)
{
	cmr_uint ret_value = SENSOR_SUCCESS;
	cmr_uint real_gain = 0;

	real_gain = isp_to_real_gain(param);

	real_gain = real_gain * SENSOR_BASE_GAIN / ISP_BASE_GAIN;

	SENSOR_PRINT("real_gain = 0x%lx", real_gain);

	s_sensor_ev_info.preview_gain = real_gain;
	ov5670_write_gain(real_gain);

	return ret_value;
}

#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
/*==============================================================================
 * Description:
 * write parameter to vcm
 * please add your VCM function to this function
 *============================================================================*/
static cmr_uint ov5670_write_af(cmr_uint param)
{
	UNUSED(param);
	return 0;//dw9714_write_af(param);

}
#endif

/*==============================================================================
 * Description:
 * increase gain or shutter for hdr
 *
 *============================================================================*/
static void ov5670_increase_hdr_exposure(uint8_t ev_multiplier)
{
	cmr_uint shutter_multiply = s_hdr_info.capture_max_shutter / s_hdr_info.capture_shutter;
	cmr_uint gain = 0;

	if (0 == shutter_multiply)
		shutter_multiply = 1;

	if (shutter_multiply >= ev_multiplier) {
		ov5670_update_exposure(s_hdr_info.capture_shutter * ev_multiplier,0);
		ov5670_write_gain(s_hdr_info.capture_gain);
	} else {
		gain = s_hdr_info.capture_gain * ev_multiplier / shutter_multiply;
		if (SENSOR_MAX_GAIN < gain)
			gain = SENSOR_MAX_GAIN;

		ov5670_update_exposure(s_hdr_info.capture_shutter * shutter_multiply,0);
		ov5670_write_gain(gain);
	}
}

/*==============================================================================
 * Description:
 * decrease gain or shutter for hdr
 *
 *============================================================================*/
static void ov5670_decrease_hdr_exposure(uint8_t ev_divisor)
{
	uint16_t gain_multiply = 0;
	cmr_uint shutter = 0;
	gain_multiply = s_hdr_info.capture_gain / SENSOR_BASE_GAIN;

	if (gain_multiply >= ev_divisor) {
		ov5670_update_exposure(s_hdr_info.capture_shutter,0);
		ov5670_write_gain(s_hdr_info.capture_gain / ev_divisor);
	} else {
		shutter = s_hdr_info.capture_shutter * gain_multiply / ev_divisor;
		ov5670_update_exposure(shutter,0);
		ov5670_write_gain(s_hdr_info.capture_gain / gain_multiply);
	}
}

/*==============================================================================
 * Description:
 * set hdr ev
 * you can change this function if it's necessary
 *============================================================================*/
static cmr_uint ov5670_set_hdr_ev(cmr_uint param)
{
	cmr_uint ret = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	cmr_uint ev = ext_ptr->param;
	uint8_t ev_divisor, ev_multiplier;

	switch (ev) {
	case SENSOR_HDR_EV_LEVE_0:
		ev_divisor = 2;
		ov5670_decrease_hdr_exposure(ev_divisor);
		break;
	case SENSOR_HDR_EV_LEVE_1:
		ev_multiplier = 1;
		ov5670_increase_hdr_exposure(ev_multiplier);
		break;
	case SENSOR_HDR_EV_LEVE_2:
		ev_multiplier = 2;
		ov5670_increase_hdr_exposure(ev_multiplier);
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
static cmr_uint ov5670_ext_func(cmr_uint param)
{
	cmr_uint rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	SENSOR_PRINT("ext_ptr->cmd: %d", ext_ptr->cmd);
	switch (ext_ptr->cmd) {
	case SENSOR_EXT_EV:
		rtn = ov5670_set_hdr_ev(param);
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
static cmr_uint ov5670_stream_on(cmr_uint param)
{
	SENSOR_PRINT("E");
	UNUSED(param);

	Sensor_WriteReg(0x0100, 0x01);
	usleep(10000);

	return 0;
}

/*==============================================================================
 * Description:
 * mipi stream off
 * please modify this function acording your spec
 *============================================================================*/
static cmr_uint ov5670_stream_off(cmr_uint param)
{
	SENSOR_PRINT("E");
	UNUSED(param);

	Sensor_WriteReg(0x0100, 0x00);
	usleep(10000);

	return 0;
}
#ifdef FEATURE_OTP
/*==============================================================================
 * Description:
 * cfg otp setting
 * please modify this function acording your spec
 *============================================================================*/
static cmr_uint ov5670_cfg_otp(cmr_uint param)
{
    UNUSED(param);
    cmr_uint rtn=SENSOR_SUCCESS;
    struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_ov5670_raw_param_tab;
    uint32_t module_id=g_module_id;

    SENSOR_PRINT("SENSOR_OV5670:cfg_otp E");
	Sensor_WriteReg(0x0103, 0x01);
	usleep(5*1000);
    if(!param){
        if(PNULL!=tab_ptr[module_id].identify_otp){
            tab_ptr[module_id].identify_otp(0);
        }
    }
    return rtn;
}
#endif
/*==============================================================================
 * Description:
 * write group-hold on to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov5670_group_hold_on(void)
{

}

/*==============================================================================
 * Description:
 * write group-hold off to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov5670_group_hold_off(void)
{

}

/*==============================================================================
 * Description:
 * all ioctl functoins
 * you can add functions reference SENSOR_IOCTL_FUNC_TAB_T which from sensor_drv_u.h
 *
 * add ioctl functions like this:
 * .power = ov5670_power_on,
 *============================================================================*/
static SENSOR_IOCTL_FUNC_TAB_T s_ov5670_ioctl_func_tab = {
	.power = ov5670_power_on,
	.identify = ov5670_identify,
	.get_trim = ov5670_get_resolution_trim_tab,
	.before_snapshort = ov5670_before_snapshot,
	.write_ae_value = ov5670_write_exposure,
	.write_gain_value = ov5670_write_gain_value,
#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
	.af_enable = ov5670_write_af,
#endif
	.set_focus = ov5670_ext_func,
	.set_video_mode = ov5670_set_video_mode,
	.stream_on = ov5670_stream_on,
	.stream_off = ov5670_stream_off,
#ifdef FEATURE_OTP
	.cfg_otp=ov5670_cfg_otp,
#endif
	//.group_hold_on = ov5670_group_hold_on,
	//.group_hold_of = ov5670_group_hold_off,
};
