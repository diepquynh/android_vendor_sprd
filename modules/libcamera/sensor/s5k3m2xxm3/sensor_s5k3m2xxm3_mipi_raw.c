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
 * V2.0
 */

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"


#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
//#include "sensor_s5k3m2xxm3_raw_param_v3.c"
#include "s5k3m2xxm3_parameters/sensor_s5k3m2xxm3_raw_param_main.c"
#else
#include "sensor_s5k3m2xxm3_raw_param.c"
#endif


#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
#include "../vcm_dw9714a.h"
#endif

#define SENSOR_NAME			"s5k3m2xxm3"
#define I2C_SLAVE_ADDR			0x5A    /* 8bit slave address*/

#define s5k3m2xxm3_PID_ADDR			0x0000
#define s5k3m2xxm3_PID_VALUE		0x30D2
#define s5k3m2xxm3_VER_ADDR			0x0000
#define s5k3m2xxm3_VER_VALUE		0x30D2


//#define ONLY_CAPTURE_SIZE
//#define ONLY_PREVIEW_SIZE

/* sensor parameters begin */
/* effective sensor output image size */
#define SNAPSHOT_WIDTH			4208
#define SNAPSHOT_HEIGHT			3120
#define PREVIEW_WIDTH			2096 //2104
#define PREVIEW_HEIGHT			1552 //1560




/*Mipi output*/
#define LANE_NUM			4
#define RAW_BITS				10

#define SNAPSHOT_MIPI_PER_LANE_BPS	1080
#define PREVIEW_MIPI_PER_LANE_BPS	1080

/*line time unit: 0.1us*/
#define SNAPSHOT_LINE_TIME		185.2
#define PREVIEW_LINE_TIME		114.8

/* frame length*/
#define SNAPSHOT_FRAME_LENGTH		3188
#define PREVIEW_FRAME_LENGTH		2894//3188

/* please ref your spec */
#define FRAME_OFFSET			4
#define SENSOR_MAX_GAIN			0x200
#define SENSOR_BASE_GAIN		0x20
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
#define MODULE_ID_s5k3m2xxm3_darling		0x05    //s5k3m2xxm3: sensor P/N;  yyy: module vendor
#define MODULE_ID_END			0xFFFF
#define LSC_PARAM_QTY 240
/*
struct otp_info_t {
	uint16_t flag;
	uint16_t module_id;
	uint16_t lens_id;
	uint16_t vcm_id;
	uint16_t vcm_driver_id;
	uint16_t year;
	uint16_t month;
	uint16_t day;
	uint16_t rg_ratio_current;
	uint16_t bg_ratio_current;
	uint16_t rg_ratio_typical;
	uint16_t bg_ratio_typical;
	uint16_t vcm_dac_start;
	uint16_t vcm_dac_inifity;
	uint16_t vcm_dac_macro;
	uint16_t lsc_param[LSC_PARAM_QTY];
};
*/

#include "sensor_s5k3m2xxm3_darling_otp.c"

struct raw_param_info_tab s_s5k3m2xxm3_raw_param_tab[] = {
	{MODULE_ID_s5k3m2xxm3_darling, &s_s5k3m2xxm3_mipi_raw_info, s5k3m2xxm3_darling_otp_identify_otp, s5k3m2xxm3_darling_otp_update_otp},
	//{MODULE_ID_END, PNULL, PNULL, PNULL}
};


static uint32_t s5k3m2xxm3_InitRawTuneInfo(void);
/*

struct otp_param_info_tab s_s5k3m2xxm3_raw_param_tab[] = {
	{MODULE_ID_s5k3m2xxm3_darling,
	{0, 0},
	 &s_s5k3m2xxm3_mipi_raw_info,
	 s5k3m2xxm3_darling_otp_update_otp},
};
*/
#endif

static SENSOR_IOCTL_FUNC_TAB_T s_s5k3m2xxm3_ioctl_func_tab;
struct sensor_raw_info *s_s5k3m2xxm3_mipi_raw_info_ptr = &s_s5k3m2xxm3_mipi_raw_info;

static const SENSOR_REG_T s5k3m2xxm3_init_setting[] = {
  //========================================================
	//========================================================
	{0x6028,0x4000},//page pointer HW
	{0x6214,0x7971},
	{0x6218,0x0100}, //clock on
	{0x6028,0x2000},//pager pointer
	{0x602A,0x448C},
	{0x6F12,0x0000},
	{0x6F12,0x0000},
	{0x6F12,0x0448},
	{0x6F12,0x0349},
	{0x6F12,0x0160},
	{0x6F12,0xC26A},
	{0x6F12,0x511A},
	{0x6F12,0x8180},
	{0x6F12,0x00F0},
	{0x6F12,0x2CB8},
	{0x6F12,0x2000},
	{0x6F12,0x4538},
	{0x6F12,0x2000},
	{0x6F12,0x1FA0},
	{0x6F12,0x0000},
	{0x6F12,0x0000},
	{0x6F12,0x0000},
	{0x6F12,0x0000},
	{0x6F12,0x2DE9},
	{0x6F12,0xF041},
	{0x6F12,0x0546},
	{0x6F12,0x1348},
	{0x6F12,0x134E},
	{0x6F12,0x018A},
	{0x6F12,0x4069},
	{0x6F12,0x06F1},
	{0x6F12,0x2007},
	{0x6F12,0x4143},
	{0x6F12,0x4FEA},
	{0x6F12,0x1138},
	{0x6F12,0x0024},
	{0x6F12,0x06EB},
	{0x6F12,0xC402},
	{0x6F12,0x0423},
	{0x6F12,0x3946},
	{0x6F12,0x4046},
	{0x6F12,0x00F0},
	{0x6F12,0x1EF8},
	{0x6F12,0x25F8},
	{0x6F12,0x1400},
	{0x6F12,0x641C},
	{0x6F12,0x042C},
	{0x6F12,0xF3DB},
	{0x6F12,0x0A48},
	{0x6F12,0x2988},
	{0x6F12,0x0180},
	{0x6F12,0x6988},
	{0x6F12,0x4180},
	{0x6F12,0xA988},
	{0x6F12,0x8180},
	{0x6F12,0xE988},
	{0x6F12,0xC180},
	{0x6F12,0xBDE8},
	{0x6F12,0xF081},
	{0x6F12,0x0022},
	{0x6F12,0xAFF2},
	{0x6F12,0x4B01},
	{0x6F12,0x0448},
	{0x6F12,0x00F0},
	{0x6F12,0x0DB8},
	{0x6F12,0x2000},
	{0x6F12,0x34D0},
	{0x6F12,0x2000},
	{0x6F12,0x7900},
	{0x6F12,0x4000},
	{0x6F12,0xD22E},
	{0x6F12,0x0000},
	{0x6F12,0x2941},
	{0x6F12,0x40F2},
	{0x6F12,0xFD7C},
	{0x6F12,0xC0F2},
	{0x6F12,0x000C},
	{0x6F12,0x6047},
	{0x6F12,0x4DF2},
	{0x6F12,0x474C},
	{0x6F12,0xC0F2},
	{0x6F12,0x000C},
	{0x6F12,0x6047},
	{0x6F12,0x0000},
	{0x6F12,0x0000},
	{0x6F12,0x0000},
	{0x6F12,0x0000},
	{0x6F12,0x30D2},
	{0x6F12,0x029C},
	{0x6F12,0x0000},
	{0x6F12,0x0001},
	{0x602A,0x7900},// Alpha control, added on 150106
	{0x6F12,0x4000},// Gain_0__1_"
	{0x6F12,0x3000},// Gain_0__2_"
	{0x6F12,0x2000},// Gain_0__3_"
	{0x6F12,0x1000},// Gain_0__4_"
	{0x6F12,0x4000},// Gain_1__1_"
	{0x6F12,0x3000},// Gain_1__2_"
	{0x6F12,0x2000},// Gain_1__3_"
	{0x6F12,0x1000},// Gain_1__4_"
	{0x6F12,0x4000},// Gain_2__1_"
	{0x6F12,0x3000},// Gain_2__2_"
	{0x6F12,0x2000},// Gain_2__3_"
	{0x6F12,0x1000},// Gain_2__4_"
	{0x6F12,0x4000},// Gain_3__1_"
	{0x6F12,0x3000},// Gain_3__2_"
	{0x6F12,0x2000},// Gain_3__3_"
	{0x6F12,0x1000},// Gain_3__4_"
	{0x6F12,0x0100},// GainThr_0_"
	{0x6F12,0x0200},// GainThr_1_"
	{0x6F12,0x0400},// GainThr_2_"
	{0x6F12,0x0800},// GainThr_3_"
	{0x602A,0x43F0},
	{0x6F12,0x0128},// Ex_Reg{0x_u{0xMinDRClneq{0xizeX
	{0x6F12,0x00DC},// Ex_Reg{0x_u{0xMinDRClneq{0xizeY
	{0x6F12,0x5590},// Ex_Reg{0x_u{0xMinDRClneq{0xizePowXd2
	{0x6F12,0x3644},// Ex_Reg{0x_u{0xMinDRClneq{0xizePowYd2
	{0x602A,0x1B50},
	{0x6F12,0x0000},
	{0x602A,0x1B54},
	{0x6F12,0x0000},
	{0x602A,0x1B64},
	{0x6F12,0x0800},
	{0x602A,0x1926},
	{0x6F12,0x0011},
	{0x602A,0x14FA},
	{0x6F12,0x0F00},
	{0x602A,0x4472},
	{0x6F12,0x0102},
	{0x6028,0x4000},
	{0x0B04,0x0101},
	{0x3B22,0x1110},
	{0xF42E,0x200C},
	{0xF49E,0x004C},
	{0xF4A6,0x00F0},
	{0x3AFA,0xFBB8},
	{0xF49C,0x0000},
	{0xF496,0x0000},
	{0xF476,0x0040},
	{0x3AAA,0x0205},
	{0x3AFE,0x07DF},
	{0xF47A,0x001B},
	{0xF462,0x0003},
	{0xF460,0x0020},
	{0x3B06,0x000E},
	{0x3AD0,0x0080},
	{0x3B02,0x0020},
	{0xF468,0x0001},
	{0xF494,0x000E},
	{0xF40C,0x2180},
	{0x3870,0x004C},
	{0x3876,0x0011},
	{0x3366,0x0128},
	{0x3852,0x00EA},
	{0x623E,0x0004},
	{0x3B5C,0x0006},
	{0x3A92,0x0807},
	{0x307C,0x0430},
	{0x6028,0x2000},
	{0x602A,0x195E},
	{0x6F12,0x99BF},
	{0x602A,0x19B6},
	{0x6F12,0x99BF},
	{0x602A,0x1A0E},
	{0x6F12,0x99BF},
	{0x602A,0x1A66},
	{0x6F12,0x99BF},
	/*
	{0x6028,0x4000},//mirror¡¢flip
	{0x602A,0x0101},
	{0x6F12,0x0100},//0x0300
	*/
};

static const SENSOR_REG_T s5k3m2xxm3_preview_setting[] = {
	//Vt=400Mhz
	//MIPI=792Mbps
	//Fps=30
	{0x6028,0x2000},
	{0x602A,0x14F0},
	{0x6F12,0x0000}, // #gisp_uOutPedestal
	{0x6F12,0x0000}, // #gisp_uHvbinOutPedestal
	{0x6028,0x4000},
	{0x0344,0x000c}, //#smiaRegs_rw_frame_timing_x_addr_start	 4
	{0x0346,0x0008}, //#smiaRegs_rw_frame_timing_y_addr_start	 4
	{0x0348,0x1073}, //#smiaRegs_rw_frame_timing_x_addr_end	 4219
	{0x034A,0x0C37}, //#smiaRegs_rw_frame_timing_y_addr_end	 3131
	{0x034C,0x0830}, //#smiaRegs_rw_frame_timing_x_output_size  4208
	{0x034E,0x0610}, //#smiaRegs_rw_frame_timing_y_output_size  3120
	{0x0900,0x0112}, //#smiaRegs_rw_binning_type
	{0x0380,0x0001}, //#smiaRegs_rw_sub_sample_x_even_inc
	{0x0382,0x0001}, //#smiaRegs_rw_sub_sample_x_odd_inc
	{0x0384,0x0001}, //#smiaRegs_rw_sub_sample_y_even_inc
	{0x0386,0x0003}, //#smiaRegs_rw_sub_sample_y_odd_inc
	{0x0400,0x0001}, //#smiaRegs_rw_scaling_scaling_mode
	{0x0404,0x0020}, //#smiaRegs_rw_scaling_scale_m
	{0x0114,0x0300}, //#smiaRegs_rw_output_lane_mode
	{0x0110,0x0002}, //#smiaRegs_rw_output_signalling_mode
	{0x112C,0x0000}, //#smiaRegs_ro_clock_limits_min_vt_pix_clk_freq_mhz
	{0x112E,0x0000}, //
	{0x0136,0x1800}, //#smiaRegs_rw_op_cond_extclk_frequency_mhz		// 24MHz
	{0x0304,0x0006}, //#smiaRegs_rw_clocks_pre_pll_clk_div				// 6
	{0x0306,0x0064}, //#smiaRegs_rw_clocks_pll_multiplier				// 110
	{0x0302,0x0001}, //#smiaRegs_rw_clocks_vt_sys_clk_div				// 1
	{0x0300,0x0004}, //#smiaRegs_rw_clocks_vt_pix_clk_div				// 4
	{0x030C,0x0004}, //#smiaRegs_rw_clocks_secnd_pre_pll_clk_div		// 4
	{0x030E,0x0042}, //#smiaRegs_rw_clocks_secnd_pll_multiplier
	{0x030A,0x0001}, //#smiaRegs_rw_clocks_op_sys_clk_div				// 1
	{0x0308,0x0008}, //#smiaRegs_rw_clocks_op_pix_clk_div				// 8
	{0x0342,0x11F0}, //#smiaRegs_rw_frame_timing_line_length_pck			 4592
	{0x0340,0x0b56}, //#smiaRegs_rw_frame_timing_frame_length_lines 		 3188
	{0x0202,0x0200}, //#smiaRegs_rw_integration_time_coarse_integration_time 512
	{0x0200,0x0400}, //#smiaRegs_rw_integration_time_fine_integration_time	 1024
	{0x0204,0x0020}, //#smiaRegs_rw_analog_gain
	{0x0B04,0x0101}, //#smiaRegs_rw_isp_mapped_couplet_correct_enable
	{0x0B08,0x0000}, //#smiaRegs_rw_isp_dynamic_couplet_correct_enable
	{0x0B00,0x0180}, //#smiaRegs_rw_isp_shading_correction_enable
	{0x3058,0x0900},
	{0x3B3C,0x0107}, //#fe_isp_dadlc_bReadNvmFadlcActiveFactor
	{0x3B34,0x3030}, //#fe_isp_dadlc_FadlcActiveFactorShGrR
	{0x3B36,0x3030}, //#fe_isp_dadlc_FadlcActiveFactorShBlGb
	{0x3B38,0x3030}, //#fe_isp_dadlc_FadlcActiveFactorLongGrR
	{0x3B3A,0x3030}, //#fe_isp_dadlc_FadlcActiveFactorLongBlGb
	{0x306A,0x0068}, //#smiaRegs_vendor_gras_otp_address
};

static const SENSOR_REG_T s5k3m2xxm3_snapshot_setting[] = {
	//Vt=400Mhz
	//MIPI=792Mbps
	//Fps=17
	{0x6028,0x2000},
	{0x602A,0x14F0},
	{0x6F12,0x0000},	 // #gisp_uOutPedestal
	{0x6F12,0x0000},	 // #gisp_uHvbinOutPedestal
	{0x6028,0x4000},
	{0x0344,0x0004},	 // #smiaRegs_rw_frame_timing_x_addr_start	 4
	{0x0346,0x0004},	 // #smiaRegs_rw_frame_timing_y_addr_start	 4

	{0x0348,0x107B},	 // #smiaRegs_rw_frame_timing_x_addr_end	 4219
	{0x034A,0x0C3B},	 // #smiaRegs_rw_frame_timing_y_addr_end	 3131

	{0x034C,0x1070},	 // #smiaRegs_rw_frame_timing_x_output_size  4208
	{0x034E,0x0C30},	 // #smiaRegs_rw_frame_timing_y_output_size  3120
	{0x0900,0x0111},	 //#smiaRegs_rw_binning_type
	{0x0380,0x0001},	 //#smiaRegs_rw_sub_sample_x_even_inc
	{0x0382,0x0001},	 //#smiaRegs_rw_sub_sample_x_odd_inc
	{0x0384,0x0001},	 //#smiaRegs_rw_sub_sample_y_even_inc
	{0x0386,0x0001},	 //#smiaRegs_rw_sub_sample_y_odd_inc
	{0x0400,0x0002},	 //#smiaRegs_rw_scaling_scaling_mode
	{0x0404,0x0010},	 //#smiaRegs_rw_scaling_scale_m
	{0x0114,0x0300},	 //#smiaRegs_rw_output_lane_mode
	{0x0110,0x0002},	 //#smiaRegs_rw_output_signalling_mode
	{0x112C,0x0000},	 //#smiaRegs_ro_clock_limits_min_vt_pix_clk_freq_mhz
	{0x112E,0x0000},	 //
	{0x0136,0x1800},	 //#smiaRegs_rw_op_cond_extclk_frequency_mhz		// 24MHz
	{0x0304,0x0006},	 //#smiaRegs_rw_clocks_pre_pll_clk_div				// 6
	{0x0306,0x0064},	 //#smiaRegs_rw_clocks_pll_multiplier				// 110
	{0x0302,0x0001},	 //#smiaRegs_rw_clocks_vt_sys_clk_div				// 1
	{0x0300,0x0004},	 //#smiaRegs_rw_clocks_vt_pix_clk_div				// 4
	{0x030C,0x0004},	 //#smiaRegs_rw_clocks_secnd_pre_pll_clk_div		// 4
	{0x030E,0x0042},	 //#smiaRegs_rw_clocks_secnd_pll_multiplier
	{0x030A,0x0001},	 //#smiaRegs_rw_clocks_op_sys_clk_div				// 1
	{0x0308,0x0008},	 //#smiaRegs_rw_clocks_op_pix_clk_div				// 8
	{0x0342,0x1cF0},	 //#smiaRegs_rw_frame_timing_line_length_pck			 4592
	{0x0340,0x0C74},	 //#smiaRegs_rw_frame_timing_frame_length_lines 		 3188
	{0x0202,0x0200},	 //#smiaRegs_rw_integration_time_coarse_integration_time 512
	{0x0200,0x0400},	 //#smiaRegs_rw_integration_time_fine_integration_time	 1024
	{0x0204,0x0020},		//#smiaRegs_rw_analog_gain
	{0x0B04,0x0101},	 //#smiaRegs_rw_isp_mapped_couplet_correct_enable
	{0x0B08,0x0000},	 //#smiaRegs_rw_isp_dynamic_couplet_correct_enable
	{0x0B00,0x0180},	 //#smiaRegs_rw_isp_shading_correction_enable
	{0x3058,0x0900},
	{0x3B3C,0x0107},	 //#fe_isp_dadlc_bReadNvmFadlcActiveFactor
	{0x3B34,0x3030},	 //#fe_isp_dadlc_FadlcActiveFactorShGrR
	{0x3B36,0x3030},	 //#fe_isp_dadlc_FadlcActiveFactorShBlGb
	{0x3B38,0x3030},	 //#fe_isp_dadlc_FadlcActiveFactorLongGrR
	{0x3B3A,0x3030},	 //#fe_isp_dadlc_FadlcActiveFactorLongBlGb
	{0x306A,0x0068},	 //#smiaRegs_vendor_gras_otp_address
};

static SENSOR_REG_TAB_INFO_T s_s5k3m2xxm3_resolution_tab_raw[SENSOR_MODE_MAX] = {
	{ADDR_AND_LEN_OF_ARRAY(s5k3m2xxm3_init_setting), 0, 0, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},
#ifndef ONLY_CAPTURE_SIZE
	{ADDR_AND_LEN_OF_ARRAY(s5k3m2xxm3_preview_setting),
	 PREVIEW_WIDTH, PREVIEW_HEIGHT, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},
#endif
#ifndef ONLY_PREVIEW_SIZE
	{ADDR_AND_LEN_OF_ARRAY(s5k3m2xxm3_snapshot_setting),
	 SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},
#endif
};

static SENSOR_TRIM_T s_s5k3m2xxm3_resolution_trim_tab[SENSOR_MODE_MAX] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
#ifndef ONLY_CAPTURE_SIZE
	{0, 0, PREVIEW_WIDTH, PREVIEW_HEIGHT,
	 PREVIEW_LINE_TIME, PREVIEW_MIPI_PER_LANE_BPS, PREVIEW_FRAME_LENGTH,
	 {0, 0, PREVIEW_WIDTH, PREVIEW_HEIGHT}},
#endif

#ifndef ONLY_PREVIEW_SIZE
	{0, 0, SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT,
	 SNAPSHOT_LINE_TIME, SNAPSHOT_MIPI_PER_LANE_BPS, SNAPSHOT_FRAME_LENGTH,
	 {0, 0, SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT}},
#endif
};

static const SENSOR_REG_T s_s5k3m2xxm3_preview_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

static const SENSOR_REG_T s_s5k3m2xxm3_capture_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

static SENSOR_VIDEO_INFO_T s_s5k3m2xxm3_video_info[SENSOR_MODE_MAX] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{30, 30, 270, 90}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 (SENSOR_REG_T **) s_s5k3m2xxm3_preview_size_video_tab},
	{{{2, 5, 338, 1000}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 (SENSOR_REG_T **) s_s5k3m2xxm3_capture_size_video_tab},
};

/*==============================================================================
 * Description:
 * set video mode
 *
 *============================================================================*/
static uint32_t s5k3m2xxm3_set_video_mode(uint32_t param)
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

	if (PNULL == s_s5k3m2xxm3_video_info[mode].setting_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR) & s_s5k3m2xxm3_video_info[mode].setting_ptr[param];
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
SENSOR_INFO_T g_s5k3m2xxm3_mipi_raw_info = {
	/* salve i2c write address */
	(I2C_SLAVE_ADDR >> 1),
	/* salve i2c read address */
	(I2C_SLAVE_ADDR >> 1),
	/*bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit */
	SENSOR_I2C_REG_16BIT | SENSOR_I2C_VAL_16BIT | SENSOR_I2C_FREQ_400,
	/* bit2: 0:negative; 1:positive -> polarily of horizontal synchronization signal
	 * bit4: 0:negative; 1:positive -> polarily of vertical synchronization signal
	 * other bit: reseved
	 */
	SENSOR_HW_SIGNAL_PCLK_P | SENSOR_HW_SIGNAL_VSYNC_P | SENSOR_HW_SIGNAL_HSYNC_P,
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
	{{s5k3m2xxm3_PID_ADDR, s5k3m2xxm3_PID_VALUE}
	 ,
	 {s5k3m2xxm3_VER_ADDR, s5k3m2xxm3_VER_VALUE}
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
	SENSOR_IMAGE_PATTERN_RAWRGB_GR,
	/* point to resolution table information structure */
	s_s5k3m2xxm3_resolution_tab_raw,
	/* point to ioctl function table */
	&s_s5k3m2xxm3_ioctl_func_tab,
	/* information and table about Rawrgb sensor */
	&s_s5k3m2xxm3_mipi_raw_info_ptr,
	/* extend information about sensor
	 * like &g_s5k3m2xxm3_ext_info
	 */
	NULL,
	/* voltage of iovdd */
	SENSOR_AVDD_1800MV,
	/* voltage of dvdd */
	SENSOR_AVDD_1200MV,
	/* skip frame num before preview */
	4,
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

/*==============================================================================
 * Description:
 * get default frame length
 *
 *============================================================================*/
static uint32_t s5k3m2xxm3_get_default_frame_length(uint32_t mode)
{
	return s_s5k3m2xxm3_resolution_trim_tab[mode].frame_line;
}

/*==============================================================================
 * Description:
 * write group-hold on to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void s5k3m2xxm3_group_hold_on(void)
{
	SENSOR_PRINT("E");

	//Sensor_WriteReg(0xYYYY, 0xff);
}

/*==============================================================================
 * Description:
 * write group-hold off to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void s5k3m2xxm3_group_hold_off(void)
{
	SENSOR_PRINT("E");

	//Sensor_WriteReg(0xYYYY, 0xff);
}


/*==============================================================================
 * Description:
 * read gain from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t s5k3m2xxm3_read_gain(void)
{
	uint16_t gain= 0;

	gain = Sensor_ReadReg(0x0204);

	return gain;
}

/*==============================================================================
 * Description:
 * write gain to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void s5k3m2xxm3_write_gain(uint32_t gain)
{
	if (SENSOR_MAX_GAIN < gain)
		gain = SENSOR_MAX_GAIN;

	Sensor_WriteReg(0x0204,gain);
	s5k3m2xxm3_group_hold_off();

}

/*==============================================================================
 * Description:
 * read frame length from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t s5k3m2xxm3_read_frame_length(void)
{
	uint16_t frame_len = 0;

	frame_len = Sensor_ReadReg(0x0340);

	return frame_len;
}

/*==============================================================================
 * Description:
 * write frame length to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void s5k3m2xxm3_write_frame_length(uint32_t frame_len)
{
	Sensor_WriteReg(0x0340, frame_len);
}

/*==============================================================================
 * Description:
 * read shutter from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t s5k3m2xxm3_read_shutter(void)
{
	uint16_t shutter = 0;

	shutter = Sensor_ReadReg(0x0202);

	return shutter;
}

/*==============================================================================
 * Description:
 * write shutter to sensor registers
 * please pay attention to the frame length
 * please modify this function acording your spec
 *============================================================================*/
static void s5k3m2xxm3_write_shutter(uint32_t shutter)
{
	Sensor_WriteReg(0x0202, shutter);
}

/*==============================================================================
 * Description:
 * write exposure to sensor registers and get current shutter
 * please pay attention to the frame length
 * please don't change this function if it's necessary
 *============================================================================*/
static uint16_t s5k3m2xxm3_update_exposure(uint32_t shutter,uint32_t dummy_line)
{
	uint32_t dest_fr_len = 0;
	uint32_t cur_fr_len = 0;
	uint32_t fr_len = s_current_default_frame_length;

	s5k3m2xxm3_group_hold_on();

	if (1 == SUPPORT_AUTO_FRAME_LENGTH)
		goto write_sensor_shutter;

	dest_fr_len = ((shutter + dummy_line+FRAME_OFFSET) > fr_len) ? (shutter +dummy_line+ FRAME_OFFSET) : fr_len;

	cur_fr_len = s5k3m2xxm3_read_frame_length();

	if (shutter < SENSOR_MIN_SHUTTER)
		shutter = SENSOR_MIN_SHUTTER;

	if (dest_fr_len != cur_fr_len)
		s5k3m2xxm3_write_frame_length(dest_fr_len);
write_sensor_shutter:
	/* write shutter to sensor registers */
	s5k3m2xxm3_write_shutter(shutter);
	return shutter;
}

/*==============================================================================
 * Description:
 * sensor power on
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t s5k3m2xxm3_power_on(uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_s5k3m2xxm3_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_s5k3m2xxm3_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_s5k3m2xxm3_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_s5k3m2xxm3_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_s5k3m2xxm3_mipi_raw_info.reset_pulse_level;

	if (SENSOR_TRUE == power_on) {

		//Sensor_PowerDown(power_down);
		Sensor_SetResetLevel(reset_level);
		usleep(2 * 1000);
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		usleep(2*1000);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(2*1000);
		//Sensor_PowerDown(!power_down);
		Sensor_SetResetLevel(!reset_level);
                   Sensor_SetMIPILevel(0);

		#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		usleep(1 * 1000);
		vcm_dw9714A_init(2);
		#endif

	} else {

		#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
		//zzz_deinit(2);
		vcm_dw9714A_init(2);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
		#endif

		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_Reset(reset_level);
		Sensor_PowerDown(power_down);
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
static int s5k3m2xxm3_get_otp_info(struct otp_info_t *otp_info)
{
	uint32_t ret = SENSOR_FAIL;
	uint32_t i = 0x00;

	//identify otp information
	for (i = 0; i < NUMBER_OF_ARRAY(s_s5k3m2xxm3_raw_param_tab); i++) {
		SENSOR_PRINT("identify module_id=0x%x",s_s5k3m2xxm3_raw_param_tab[i].param_id);

		if(PNULL!=s_s5k3m2xxm3_raw_param_tab[i].identify_otp){
			//set default value;
			memset(otp_info, 0x00, sizeof(struct otp_info_t));

			if(s_s5k3m2xxm3_raw_param_tab[i].param_id==s_s5k3m2xxm3_raw_param_tab[i].identify_otp(otp_info))
			{
				//if (s_s5k3m2xxm3_raw_param_tab[i].param_id== otp_info->module_id)
				{
					SENSOR_PRINT("identify otp sucess! module_id=0x%x",s_s5k3m2xxm3_raw_param_tab[i].param_id);
					ret = SENSOR_SUCCESS;
					break;
				}
			}
			else{
				{
					SENSOR_PRINT("identify module_id failed! table module_id=0x%x, otp module_id=0x%x",s_s5k3m2xxm3_raw_param_tab[i].param_id,otp_info->module_id);
				}
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
static uint32_t s5k3m2xxm3_apply_otp(struct otp_info_t *otp_info, int id)
{
	uint32_t ret = SENSOR_FAIL;
	//apply otp parameters
	SENSOR_PRINT("otp_table_id = %d", id);
	if (PNULL != s_s5k3m2xxm3_raw_param_tab[id].cfg_otp) {

		if(SENSOR_SUCCESS==s_s5k3m2xxm3_raw_param_tab[id].cfg_otp(otp_info)){
			SENSOR_PRINT("apply otp parameters sucess! module_id=0x%x",s_s5k3m2xxm3_raw_param_tab[id].param_id);
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
static uint32_t s5k3m2xxm3_cfg_otp(uint32_t param)
{
	uint32_t ret = SENSOR_FAIL;
	struct otp_info_t otp_info={0x00};
	int table_id = 0;

	table_id = s5k3m2xxm3_get_otp_info(&otp_info);
	if (-1 != table_id)
		ret = s5k3m2xxm3_apply_otp(&otp_info, table_id);

	//checking OTP apply result

	if (SENSOR_SUCCESS != ret) {//disable lsc
		//Sensor_WriteReg(0x0d00,0x0080);
	}
	else{//enable lsc
		//Sensor_WriteReg(0x0d00,0x0180);
	}

	return ret;
}
#endif

/*==============================================================================
 * Description:
 * identify sensor id
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t s5k3m2xxm3_identify(uint32_t param)
{
	uint16_t pid_value = 0x00;
	uint16_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("mipi raw identify");

	pid_value = Sensor_ReadReg(s5k3m2xxm3_PID_ADDR);

	if (s5k3m2xxm3_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(s5k3m2xxm3_VER_ADDR);
		SENSOR_PRINT("Identify: PID = %x, VER = %x", pid_value, ver_value);
		if (s5k3m2xxm3_VER_VALUE == ver_value) {
			ret_value = SENSOR_SUCCESS;
			SENSOR_PRINT_HIGH("this is s5k3m2xxm3 sensor");
		} else {
			SENSOR_PRINT_HIGH("Identify this is %x%x sensor", pid_value, ver_value);
		}
		s5k3m2xxm3_InitRawTuneInfo();
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
static unsigned long s5k3m2xxm3_get_resolution_trim_tab(uint32_t param)
{
	return (unsigned long) s_s5k3m2xxm3_resolution_trim_tab;
}

/*==============================================================================
 * Description:
 * before snapshot
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t s5k3m2xxm3_before_snapshot(uint32_t param)
{
	uint32_t cap_shutter = 0;
	uint32_t prv_shutter = 0;
	uint32_t gain = 0;
	uint32_t cap_gain = 0;
	uint32_t capture_mode = param & 0xffff;
	uint32_t preview_mode = (param >> 0x10) & 0xffff;

	uint32_t prv_linetime = s_s5k3m2xxm3_resolution_trim_tab[preview_mode].line_time;
	uint32_t cap_linetime = s_s5k3m2xxm3_resolution_trim_tab[capture_mode].line_time;

	s_current_default_frame_length = s5k3m2xxm3_get_default_frame_length(capture_mode);
	SENSOR_PRINT("capture_mode = %d", capture_mode);

	if (preview_mode == capture_mode) {
		cap_shutter = s_sensor_ev_info.preview_shutter;
		cap_gain = s_sensor_ev_info.preview_gain;
		goto snapshot_info;
	}

	prv_shutter = s_sensor_ev_info.preview_shutter;	//s5k3m2xxm3_read_shutter();
	gain = s_sensor_ev_info.preview_gain;	//s5k3m2xxm3_read_gain();

	Sensor_SetMode(capture_mode);
	Sensor_SetMode_WaitDone();

	cap_shutter = prv_shutter * prv_linetime / cap_linetime * BINNING_FACTOR;

	while (gain >= (2 * SENSOR_BASE_GAIN)) {
		if (cap_shutter * 2 > s_current_default_frame_length)
			break;
		cap_shutter = cap_shutter * 2;
		gain = gain / 2;
	}

	cap_shutter = s5k3m2xxm3_update_exposure(cap_shutter,0);
	cap_gain = gain;
	s5k3m2xxm3_write_gain(cap_gain);
	SENSOR_PRINT("preview_shutter = 0x%x, preview_gain = 0x%x",
		     s_sensor_ev_info.preview_shutter, s_sensor_ev_info.preview_gain);

	SENSOR_PRINT("capture_shutter = 0x%x, capture_gain = 0x%x", cap_shutter, cap_gain);
snapshot_info:
	s_hdr_info.capture_shutter = cap_shutter; //s5k3m2xxm3_read_shutter();
	s_hdr_info.capture_gain = cap_gain; //s5k3m2xxm3_read_gain();
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
static uint32_t s5k3m2xxm3_write_exposure(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t exposure_line = 0x00;
	uint16_t dummy_line = 0x00;
	uint16_t mode = 0x00;

	exposure_line = param & 0xffff;
	dummy_line = (param >> 0x10) & 0xfff; /*for cits frame rate test*/
	mode = (param >> 0x1c) & 0x0f;

	SENSOR_PRINT("current mode = %d, exposure_line = %d, dummy_line=%d", mode, exposure_line,dummy_line);
	s_current_default_frame_length = s5k3m2xxm3_get_default_frame_length(mode);

	s_sensor_ev_info.preview_shutter = s5k3m2xxm3_update_exposure(exposure_line,dummy_line);

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

/*==============================================================================
 * Description:
 * write gain value to sensor
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t s5k3m2xxm3_write_gain_value(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint32_t real_gain = 0;

	real_gain = isp_to_real_gain(param);
	real_gain = real_gain * SENSOR_BASE_GAIN / ISP_BASE_GAIN;

	SENSOR_PRINT("real_gain = 0x%x", real_gain);

	s_sensor_ev_info.preview_gain = real_gain;
	s5k3m2xxm3_write_gain(real_gain);

	return ret_value;
}

#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
/*==============================================================================
 * Description:
 * write parameter to vcm
 * please add your VCM function to this function
 *============================================================================*/
static uint32_t s5k3m2xxm3_write_af(uint32_t param)
{
	return vcm_dw9714A_set_position(param,0);
}
#endif

/*==============================================================================
 * Description:
 * increase gain or shutter for hdr
 *
 *============================================================================*/
static void s5k3m2xxm3_increase_hdr_exposure(uint8_t ev_multiplier)
{
	uint32_t shutter_multiply = s_hdr_info.capture_max_shutter / s_hdr_info.capture_shutter;
	uint32_t gain = 0;

	if (0 == shutter_multiply)
		shutter_multiply = 1;

	if (shutter_multiply >= ev_multiplier) {
		s5k3m2xxm3_update_exposure(s_hdr_info.capture_shutter * ev_multiplier,0);
		s5k3m2xxm3_write_gain(s_hdr_info.capture_gain);
	} else {
		gain = s_hdr_info.capture_gain * ev_multiplier / shutter_multiply;
		s5k3m2xxm3_update_exposure(s_hdr_info.capture_shutter * shutter_multiply,0);
		s5k3m2xxm3_write_gain(gain);
	}
}

/*==============================================================================
 * Description:
 * decrease gain or shutter for hdr
 *
 *============================================================================*/
static void s5k3m2xxm3_decrease_hdr_exposure(uint8_t ev_divisor)
{
	uint16_t gain_multiply = 0;
	uint32_t shutter = 0;
	gain_multiply = s_hdr_info.capture_gain / SENSOR_BASE_GAIN;

	if (gain_multiply >= ev_divisor) {
		s5k3m2xxm3_update_exposure(s_hdr_info.capture_shutter,0);
		s5k3m2xxm3_write_gain(s_hdr_info.capture_gain / ev_divisor);

	} else {
		shutter = s_hdr_info.capture_shutter * gain_multiply / ev_divisor;
		s5k3m2xxm3_update_exposure(shutter,0);
		s5k3m2xxm3_write_gain(s_hdr_info.capture_gain / gain_multiply);
	}
}

/*==============================================================================
 * Description:
 * set hdr ev
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t s5k3m2xxm3_set_hdr_ev(unsigned long param)
{
	uint32_t ret = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	uint32_t ev = ext_ptr->param;
	uint8_t ev_divisor, ev_multiplier;

	switch (ev) {
	case SENSOR_HDR_EV_LEVE_0:
		ev_divisor = 2;
		s5k3m2xxm3_decrease_hdr_exposure(ev_divisor);
		break;
	case SENSOR_HDR_EV_LEVE_1:
		ev_multiplier = 2;
		s5k3m2xxm3_increase_hdr_exposure(ev_multiplier);
		break;
	case SENSOR_HDR_EV_LEVE_2:
		ev_multiplier = 1;
		s5k3m2xxm3_increase_hdr_exposure(ev_multiplier);
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
static uint32_t s5k3m2xxm3_ext_func(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	SENSOR_PRINT("ext_ptr->cmd: %d", ext_ptr->cmd);
	switch (ext_ptr->cmd) {
	case SENSOR_EXT_EV:
		rtn = s5k3m2xxm3_set_hdr_ev(param);
		break;
	default:
		break;
	}

	return rtn;
}

static struct sensor_raw_info* Sensor_GetContext(void)
{
	return s_s5k3m2xxm3_mipi_raw_info_ptr;
}


#define param_update(x1,x2) sprintf(name,"/data/s5k3m2xxm3_%s.bin",x1);\
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
#define param_update_v1(x1,x2,len) sprintf(name,"/data/s5k3m2xxm3_%s.bin",x1);\
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

static uint32_t s5k3m2xxm3_InitRawTuneInfo(void)
{
	uint32_t rtn=0x00;

	isp_raw_para_update_from_file(&g_s5k3m2xxm3_mipi_raw_info,0);

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
					#include "s5k3m2xxm3_parameters/NR/pwd_param.h"
				};

				param_update("pwd_param",pwd_param);

				block->param_ptr = pwd_param;
			}
			break;

		case	ISP_BLK_BPC_V1: {
				/* modify block data */
				struct sensor_bpc_param_v1* block = (struct sensor_bpc_param_v1*)data;

				static struct sensor_bpc_level bpc_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "s5k3m2xxm3_parameters/NR/bpc_param.h"
				};

				param_update("bpc_param",bpc_param);

				block->param_ptr = bpc_param;
			}
			break;

		case	ISP_BLK_BL_NR_V1: {
				/* modify block data */
				struct sensor_bdn_param* block = (struct sensor_bdn_param*)data;

				static struct sensor_bdn_level bdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "s5k3m2xxm3_parameters/NR/bdn_param.h"
				};

				param_update("bdn_param",bdn_param);

				block->param_ptr = bdn_param;
			}
			break;

		case	ISP_BLK_GRGB_V1: {
				/* modify block data */
				struct sensor_grgb_v1_param* block = (struct sensor_grgb_v1_param*)data;
				static struct sensor_grgb_v1_level grgb_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "s5k3m2xxm3_parameters/NR/grgb_param.h"
				};

				param_update("grgb_param",grgb_param);

				block->param_ptr = grgb_param;

			}
			break;

		case	ISP_BLK_NLM: {
				/* modify block data */
				struct sensor_nlm_param* block = (struct sensor_nlm_param*)data;

				static struct sensor_nlm_level nlm_param[32] = {
					#include "s5k3m2xxm3_parameters/NR/nlm_param.h"
				};

				param_update("nlm_param",nlm_param);

				static struct sensor_vst_level vst_param[32] = {
					#include "s5k3m2xxm3_parameters/NR/vst_param.h"
				};

				param_update("vst_param",vst_param);

				static struct sensor_ivst_level ivst_param[32] = {
					#include "s5k3m2xxm3_parameters/NR/ivst_param.h"
				};

				param_update("ivst_param",ivst_param);

				static struct sensor_flat_offset_level flat_offset_param[32] = {
					#include "s5k3m2xxm3_parameters/NR/flat_offset_param.h"
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
					#include "s5k3m2xxm3_parameters/NR/cfae_param.h"
				};

				param_update("cfae_param",cfae_param);

				block->param_ptr = cfae_param;
			}
			break;

		case	ISP_BLK_RGB_PRECDN: {
				/* modify block data */
				struct sensor_rgb_precdn_param* block = (struct sensor_rgb_precdn_param*)data;

				static struct sensor_rgb_precdn_level precdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "s5k3m2xxm3_parameters/NR/rgb_precdn_param.h"
				};

				param_update("rgb_precdn_param",precdn_param);

				block->param_ptr = precdn_param;
			}
			break;

		case	ISP_BLK_YUV_PRECDN: {
				/* modify block data */
				struct sensor_yuv_precdn_param* block = (struct sensor_yuv_precdn_param*)data;

				static struct sensor_yuv_precdn_level yuv_precdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "s5k3m2xxm3_parameters/NR/yuv_precdn_param.h"
				};

				param_update("yuv_precdn_param",yuv_precdn_param);

				block->param_ptr = yuv_precdn_param;
			}
			break;

		case	ISP_BLK_PREF_V1: {
				/* modify block data */
				struct sensor_prfy_param* block = (struct sensor_prfy_param*)data;

				static struct sensor_prfy_level prfy_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "s5k3m2xxm3_parameters/NR/prfy_param.h"
				};

				param_update("prfy_param",prfy_param);

				block->param_ptr = prfy_param;
			}
			break;

		case	ISP_BLK_UV_CDN: {
				/* modify block data */
				struct sensor_uv_cdn_param* block = (struct sensor_uv_cdn_param*)data;

				static struct sensor_uv_cdn_level uv_cdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "s5k3m2xxm3_parameters/NR/yuv_cdn_param.h"
				};

				param_update("yuv_cdn_param",uv_cdn_param);

				block->param_ptr = uv_cdn_param;
			}
			break;

		case	ISP_BLK_EDGE_V1: {
				/* modify block data */
				struct sensor_ee_param* block = (struct sensor_ee_param*)data;

				static struct sensor_ee_level edge_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "s5k3m2xxm3_parameters/NR/edge_param.h"
				};

				param_update("edge_param",edge_param);

				block->param_ptr = edge_param;
			}
			break;

		case	ISP_BLK_UV_POSTCDN: {
				/* modify block data */
				struct sensor_uv_postcdn_param* block = (struct sensor_uv_postcdn_param*)data;

				static struct sensor_uv_postcdn_level uv_postcdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "s5k3m2xxm3_parameters/NR/yuv_postcdn_param.h"
				};

				param_update("yuv_postcdn_param",uv_postcdn_param);

				block->param_ptr = uv_postcdn_param;
			}
			break;

		case	ISP_BLK_IIRCNR_IIR: {
				/* modify block data */
				struct sensor_iircnr_param* block = (struct sensor_iircnr_param*)data;

				static struct sensor_iircnr_level iir_cnr_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "s5k3m2xxm3_parameters/NR/iircnr_param.h"
				};

				param_update("iircnr_param",iir_cnr_param);

				block->param_ptr = iir_cnr_param;
			}
			break;

		case	ISP_BLK_IIRCNR_YRANDOM: {
				/* modify block data */
				struct sensor_iircnr_yrandom_param* block = (struct sensor_iircnr_yrandom_param*)data;
				static struct sensor_iircnr_yrandom_level iir_yrandom_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "s5k3m2xxm3_parameters/NR/iir_yrandom_param.h"
				};

				param_update("iir_yrandom_param",iir_yrandom_param);

				block->param_ptr = iir_yrandom_param;
			}
			break;

		case  ISP_BLK_UVDIV_V1: {
				/* modify block data */
				struct sensor_cce_uvdiv_param_v1* block = (struct sensor_cce_uvdiv_param_v1*)data;

				static struct sensor_cce_uvdiv_level cce_uvdiv_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "s5k3m2xxm3_parameters/NR/cce_uv_param.h"
				};

				param_update("cce_uv_param",cce_uvdiv_param);

				block->param_ptr = cce_uvdiv_param;
			}
			break;
		case ISP_BLK_YIQ_AFM:{
			/* modify block data */
			struct sensor_y_afm_param *block = (struct sensor_y_afm_param*)data;

			static struct sensor_y_afm_level y_afm_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "s5k3m2xxm3_parameters/NR/y_afm_param.h"
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



/*==============================================================================
 * Description:
 * mipi stream on
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t s5k3m2xxm3_stream_on(uint32_t param)
{
	SENSOR_PRINT("E");

	//Sensor_WriteReg(0x0100, (0x01<<8)| (Sensor_ReadReg(0x0100) & 0x00ff));
	Sensor_WriteReg(0x0100, 0x0100);
	/*delay*/
	usleep(10 * 1000);
	/*
	Sensor_WriteReg(0x0A02, 0x0000); //page set
	Sensor_WriteReg(0x0A00, 0x0100); //otp enable read
	usleep(10 * 1000);

	uint32_t awb_flag=Sensor_ReadReg(0x0A24);
	SENSOR_PRINT("0x0A24=0x%x",awb_flag);
	awb_flag=Sensor_ReadReg(0x0A26);
	SENSOR_PRINT("0x0A26=0x%x",awb_flag);
	awb_flag=Sensor_ReadReg(0x0A28);
	SENSOR_PRINT("0x0A28=0x%x",awb_flag);

	awb_flag=Sensor_ReadReg(0x0000);
	SENSOR_PRINT("i2c address=0x%x",awb_flag);
*/


	return 0;
}

/*==============================================================================
 * Description:
 * mipi stream off
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t s5k3m2xxm3_stream_off(uint32_t param)
{
	SENSOR_PRINT("E");

	//Sensor_WriteReg(0x0100, (0x00<<8)| (Sensor_ReadReg(0x0100) & 0x00ff));
	Sensor_WriteReg(0x0100, 0x0000);
	/*delay*/
	usleep(50 * 1000);

	return 0;
}


/*==============================================================================
 * Description:
 * all ioctl functoins
 * you can add functions reference SENSOR_IOCTL_FUNC_TAB_T from sensor_drv_u.h
 *
 * add ioctl functions like this:
 * .power = s5k3m2xxm3_power_on,
 *============================================================================*/
static SENSOR_IOCTL_FUNC_TAB_T s_s5k3m2xxm3_ioctl_func_tab = {
	.power = s5k3m2xxm3_power_on,
	.identify = s5k3m2xxm3_identify,
	.get_trim = s5k3m2xxm3_get_resolution_trim_tab,
	.before_snapshort = s5k3m2xxm3_before_snapshot,
	.write_ae_value = s5k3m2xxm3_write_exposure,
	.write_gain_value = s5k3m2xxm3_write_gain_value,
	#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
	.af_enable = s5k3m2xxm3_write_af,
	#endif
	.set_focus = s5k3m2xxm3_ext_func,
	//.set_video_mode = s5k3m2xxm3_set_video_mode,
	.stream_on = s5k3m2xxm3_stream_on,
	.stream_off = s5k3m2xxm3_stream_off,
	#ifdef FEATURE_OTP
	.cfg_otp=s5k3m2xxm3_cfg_otp,
	#endif

	//.group_hold_on = s5k3m2xxm3_group_hold_on,
	//.group_hold_of = s5k3m2xxm3_group_hold_off,
};
