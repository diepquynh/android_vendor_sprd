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
#include "sensor_ov8825_raw_param.c"
#include "sensor_ov8825_otp_truly_02.c"
//#include "sensor_ov8825_ae_tab.c"

#define ov8825_I2C_ADDR_W        0x36
#define ov8825_I2C_ADDR_R         0x36

#define OV8825_MIN_FRAME_LEN_PRV  0x5e8
#define OV8825_2_LANES
static int s_ov8825_gain = 0;
static int s_ov8825_gain_bak = 0;
static int s_ov8825_shutter_bak = 0;
static int s_capture_shutter = 0;
static int s_capture_VTS = 0;
static int s_video_min_framerate = 0;
static int s_video_max_framerate = 0;
static uint32_t is_stream_on = 0;
static int s_preview_shutter = 0;

LOCAL unsigned long _ov8825_GetResolutionTrimTab(unsigned long param);
LOCAL unsigned long _ov8825_PowerOn(unsigned long power_on);
LOCAL unsigned long _ov8825_Identify(unsigned long param);
LOCAL unsigned long _ov8825_BeforeSnapshot(unsigned long param);
LOCAL unsigned long _ov8825_after_snapshot(unsigned long param);
LOCAL unsigned long _ov8825_StreamOn(unsigned long param);
LOCAL unsigned long _ov8825_StreamOff(unsigned long param);
LOCAL unsigned long _ov8825_write_exposure(unsigned long param);
LOCAL unsigned long _ov8825_write_gain(unsigned long param);
LOCAL unsigned long _ov8825_write_af(unsigned long param);
LOCAL unsigned long _ov8825_flash(unsigned long param);
LOCAL unsigned long _ov8825_ExtFunc(unsigned long ctl_param);
LOCAL int _ov8825_get_VTS(void);
LOCAL int _ov8825_set_VTS(int VTS);
LOCAL unsigned long _ov8825_ReadGain(unsigned long param);
LOCAL unsigned long _ov8825_set_video_mode(unsigned long param);
LOCAL int _ov8825_get_shutter(void);
LOCAL unsigned long _ov8825_cfg_otp(unsigned long  param);
LOCAL uint32_t _ov8825_access_val(unsigned long param);


LOCAL const struct raw_param_info_tab s_ov8825_raw_param_tab[]={
	{OV8825_TRULY_02, &s_ov8825_mipi_raw_info, _ov8825_truly_Identify_otp, _ov8825_truly_update_otp},
	{RAW_INFO_END_ID, PNULL, PNULL, PNULL}
};

struct sensor_raw_info* s_ov8825_mipi_raw_info_ptr=NULL;

static uint32_t g_ov8825_module_id = 0;

static uint32_t g_flash_mode_en = 0;
static uint32_t g_af_slewrate = 1;

LOCAL const SENSOR_REG_T ov8825_common_init[] = {
#if defined(OV8825_2_LANES)
//	{0x0100, 0x00}, // software standby
	// delay(5ms)
	{0x3000, 0x16}, // strobe disable, frex dis
	{0x3001, 0x00},
	{0x3002, 0x6c}, // SCCB ID = 0x6c
	{0x300d, 0x00}, // PLL2
	{0x301f, 0x09}, // frex_mask_mipi, frex_mas
	{0x3010, 0x00}, // strobe, sda, frex, vsync
	{0x3011, 0x01}, // MIPI_2_Lane
	{0x3018, 0x00}, // clear PHY HS TX power do
	{0x3104, 0x20}, // SCCB_PLL
	{0x3300, 0x00},
	{0x3500, 0x00}, // exposure[19:16] = 0
	{0x3503, 0x07}, // Gain has no delay, VTS m
	{0x3509, 0x00}, // use sensor gain
	{0x3603, 0x5c}, // analog control
	{0x3604, 0x98}, // analog control
	{0x3605, 0xf5}, // analog control
	{0x3609, 0xb4}, // analog control
	{0x360a, 0x7c}, // analog control
	{0x360b, 0xc9}, // analog control
	{0x360c, 0x0b}, // analog control
	{0x3612, 0x00}, // pad drive 1x, analog con
	{0x3613, 0x02}, // analog control
	{0x3614, 0x0f}, // analog control
	{0x3615, 0x00}, // analog control
	{0x3616, 0x03}, // analog control
	{0x3617, 0xa1}, // analog control
	{0x3618, 0x00}, // VCM position & slew rate
	{0x3619, 0x00}, // VCM position = 0
	{0x361a, 0xB0}, // VCM clock divider, VCM c
	{0x361b, 0x04}, // VCM clock divider
	{0x3701, 0x44}, // sensor control
	{0x3707, 0x63}, // SENCTROL7 Sensor control
	{0x370b, 0x01}, // sensor control
	{0x370c, 0x50}, // sensor control
	{0x370d, 0x00}, // sensor control
	{0x3816, 0x02}, // Hsync start H
	{0x3817, 0x40}, // Hsync start L
	{0x3818, 0x00}, // Hsync end H
	{0x3819, 0x40}, // Hsync end L
	{0x3b1f, 0x00}, // Frex conrol
	 // clear OTP data buffer
	{0x3d00, 0x00},
	{0x3d01, 0x00},
	{0x3d02, 0x00},
	{0x3d03, 0x00},
	{0x3d04, 0x00},
	{0x3d05, 0x00},
	{0x3d06, 0x00},
	{0x3d07, 0x00},
	{0x3d08, 0x00},
	{0x3d09, 0x00},
	{0x3d0a, 0x00},
	{0x3d0b, 0x00},
	{0x3d0c, 0x00},
	{0x3d0d, 0x00},
	{0x3d0e, 0x00},
	{0x3d0f, 0x00},
	{0x3d10, 0x00},
	{0x3d11, 0x00},
	{0x3d12, 0x00},
	{0x3d13, 0x00},
	{0x3d14, 0x00},
	{0x3d15, 0x00},
	{0x3d16, 0x00},
	{0x3d17, 0x00},
	{0x3d18, 0x00},
	{0x3d19, 0x00},
	{0x3d1a, 0x00},
	{0x3d1b, 0x00},
	{0x3d1c, 0x00},
	{0x3d1d, 0x00},
	{0x3d1e, 0x00},
	{0x3d1f, 0x00},
	{0x3d80, 0x00},
	{0x3d81, 0x00},
	{0x3d84, 0x00},
	{0x3f01, 0xfc}, // PSRAM Ctrl1
	{0x3f05, 0x10}, // PSRAM Ctrl5
	{0x3f06, 0x00},
	{0x3f07, 0x00},
	 // BLC
	{0x4000, 0x29},
	{0x4001, 0x02}, // BLC start line
	{0x4002, 0x45}, // BLC auto, reset 5 frames
	{0x4003, 0x08}, // BLC redo at 8 frames
	{0x4004, 0x04}, // 4 black lines are used f
	{0x4005, 0x18}, // no black line output, ap
	{0x4300, 0xff}, // max
	{0x4303, 0x00}, // format control
	{0x4304, 0x08}, // output {data[7:0], data[
	{0x4307, 0x00}, // embedded control
	 //MIPI
	{0x4800, 0x04},
	{0x4801, 0x0f}, // ECC configure
	{0x4843, 0x02}, // manual set pclk divider
	 //ISP
	{0x5000, 0x06}, // LENC off, BPC on, WPC on
	{0x5001, 0x00}, // MWB off
	{0x5002, 0x00},
	{0x501f, 0x00}, // enable ISP
	{0x5780, 0xfc}, // DPC control
	{0x5c00, 0x80}, // PBLC CTRL00
	{0x5c01, 0x00}, // PBLC CTRL01
	{0x5c02, 0x00}, // PBLC CTRL02
	{0x5c03, 0x00}, // PBLC CTRL03
	{0x5c04, 0x00}, // PBLC CTRL04
	{0x5c05, 0x00}, // pre BLC
	{0x5c06, 0x00}, // pre BLC
	{0x5c07, 0x80}, // pre BLC
	{0x5c08, 0x10}, // PBLC CTRL08
	 // temperature sensor
	{0x6700, 0x05},
	{0x6701, 0x19},
	{0x6702, 0xfd},
	{0x6703, 0xd7},
	{0x6704, 0xff},
	{0x6705, 0xff},
	{0x6800, 0x10},
	{0x6801, 0x02},
	{0x6802, 0x90},
	{0x6803, 0x10},
	{0x6804, 0x59},
	{0x6900, 0x60}, // CADC CTRL00
	{0x6901, 0x04}, // CADC control
	 // Default Lens Correction
	{0x5800, 0x0f},
	{0x5801, 0x0d},
	{0x5802, 0x09},
	{0x5803, 0x0a},
	{0x5804, 0x0d},
	{0x5805, 0x14},
	{0x5806, 0x0a},
	{0x5807, 0x04},
	{0x5808, 0x03},
	{0x5809, 0x03},
	{0x580a, 0x05},
	{0x580b, 0x0a},
	{0x580c, 0x05},
	{0x580d, 0x02},
	{0x580e, 0x00},
	{0x580f, 0x00},
	{0x5810, 0x03},
	{0x5811, 0x05},
	{0x5812, 0x09},
	{0x5813, 0x03},
	{0x5814, 0x01},
	{0x5815, 0x01},
	{0x5816, 0x04},
	{0x5817, 0x09},
	{0x5818, 0x09},
	{0x5819, 0x08},
	{0x581a, 0x06},
	{0x581b, 0x06},
	{0x581c, 0x08},
	{0x581d, 0x06},
	{0x581e, 0x33},
	{0x581f, 0x11},
	{0x5820, 0x0e},
	{0x5821, 0x0f},
	{0x5822, 0x11},
	{0x5823, 0x3f},
	{0x5824, 0x08},
	{0x5825, 0x46},
	{0x5826, 0x46},
	{0x5827, 0x46},
	{0x5828, 0x46},
	{0x5829, 0x46},
	{0x582a, 0x42},
	{0x582b, 0x42},
	{0x582c, 0x44},
	{0x582d, 0x46},
	{0x582e, 0x46},
	{0x582f, 0x60},
	{0x5830, 0x62},
	{0x5831, 0x42},
	{0x5832, 0x46},
	{0x5833, 0x46},
	{0x5834, 0x44},
	{0x5835, 0x44},
	{0x5836, 0x44},
	{0x5837, 0x48},
	{0x5838, 0x28},
	{0x5839, 0x46},
	{0x583a, 0x48},
	{0x583b, 0x68},
	{0x583c, 0x28},
	{0x583d, 0xae},
	{0x5842, 0x00},
	{0x5843, 0xef},
	{0x5844, 0x01},
	{0x5845, 0x3f},
	{0x5846, 0x01},
	{0x5847, 0x3f},
	{0x5848, 0x00},
	{0x5849, 0xd5},

	 // PLL s0xetting of preview
	{0x3003, 0xce}, // PLL_CTRL0
	{0x3004, 0xd4}, // PLL_CTRL1
	{0x3005, 0x00}, // PLL_CTRL2
	{0x3006, 0x10}, // PLL_CTRL3
	{0x3007, 0x3b}, // PLL_CTRL4
	{0x3012, 0x81}, // SC_PLL CTRL_S0
	{0x3013, 0x39}, // SC_PLL CTRL_S1
	{0x3106, 0x11}, // SRB_CTRL
	 // Exposure
	{0x3503, 0x07}, // Gain has no delay, VTS m
	{0x3500, 0x00}, // expo[19:16] = lines/16
	{0x3501, 0x27}, // expo[15:8]
	{0x3502, 0x00}, // expo[7:0]
	{0x350b, 0xff}, // gain
	 // MWB
	{0x3400, 0x04}, // red h
	{0x3401, 0x00}, // red l
	{0x3402, 0x04}, // green h
	{0x3403, 0x00}, // green l
	{0x3404, 0x04}, // blue h
	{0x3405, 0x00}, // blue l
	{0x3406, 0x01}, // MWB manual
	 // ISP
	{0x5001, 0x01}, // MWB on
	{0x5000, 0x06} // LENC off, BPC on, WPC on
#elif defined(OV8825_4_LANES)
//	{0x0100, 0x00}, // software standby
	{0x0103, 0x01}, //software reset
	{SENSOR_WRITE_DELAY, 0x0a},
	{0x3000, 0x16}, //strobe, 0xdisable,, 0xfrex, 0xdisable, vsync, 0xdisable
	{0x3001, 0x00},
	{0x3002, 0x6c},
	{0x300d, 0x00}, //PLL2
	{0x301f, 0x09}, //0xfrex_mask_mipi,, 0xfrex_mask_mipi_phy
	{0x3010, 0x00}, //strobe, sda,, 0xfrex, vsync, shutter GPIO unselected
	{0x3011, 0x02}, //MIPI_Lane_4_Lane
	{0x3018, 0x00}, //0xclear PHY HS TX power, 0xdown, 0xand PHY LP RX power, 0xdown
	{0x3104, 0x20}, //SCCB_PLL
	{0x3300, 0x00},
	{0x3500, 0x00}, //0xexposure[19:16] =, 0x0
	{0x3503, 0x07}, //Gain has no, 0xdelay, VTS manual,, 0xaGC manual,, 0xaEC manual
	{0x3509, 0x00}, //use sensor gain
	{0x3603, 0x5c}, //0xanalog, 0xcontrol
	{0x3604, 0x98}, //0xanalog, 0xcontrol
	{0x3605, 0xf5}, //0xanalog, 0xcontrol
	{0x3609, 0xb4}, //0xanalog, 0xcontrol
	{0x360a, 0x7c}, //0xanalog, 0xcontrol
	{0x360b, 0xc9}, //0xanalog, 0xcontrol
	{0x360c, 0x0b}, //0xanalog, 0xcontrol
	{0x3612, 0x00}, //pad, 0xdrive, 0x1x,, 0xanalog, 0xcontrol
	{0x3613, 0x02}, //0xanalog, 0xcontrol
	{0x3614, 0x0f}, //0xanalog, 0xcontrol
	{0x3615, 0x00}, //0xanalog, 0xcontrol
	{0x3616, 0x03}, //0xanalog, 0xcontrol
	{0x3617, 0xa1}, //0xanalog, 0xcontrol
	{0x3618, 0x00}, //VCM position & slew rate, slew rate =, 0x0
	{0x3619, 0x00}, //VCM position =, 0x0
	{0x361a, 0xb0}, //VCM, 0xclock, 0xdivider, VCM, 0xclock =, 0x24000000/0x4b0 =, 0x20000
	{0x361b, 0x04}, //VCM, 0xclock, 0xdivider

	{0x3701, 0x44}, //sensor, 0xcontrol
	{0x3707, 0x63}, //SENCTROL7 Sensor, 0xcontrol
	{0x370b, 0x01}, //sensor, 0xcontrol
	{0x370c, 0x50}, //sensor, 0xcontrol
	{0x370d, 0x00}, //sensor, 0xcontrol
	{0x3816, 0x02}, //Hsync start H
	{0x3817, 0x40}, //Hsync start L
	{0x3818, 0x00}, //Hsync, 0xend H
	{0x3819, 0x40}, //Hsync, 0xend L
	{0x3b1f, 0x00}, //0xfrex, 0xconrol
	// clear OTP, //0xdata, 0xbuffer
	{0x3d00, 0x00},
	{0x3d01, 0x00},
	{0x3d02, 0x00},
	{0x3d03, 0x00},
	{0x3d04, 0x00},
	{0x3d05, 0x00},
	{0x3d06, 0x00},
	{0x3d07, 0x00},
	{0x3d08, 0x00},
	{0x3d09, 0x00},
	{0x3d0a, 0x00},
	{0x3d0b, 0x00},
	{0x3d0c, 0x00},
	{0x3d0d, 0x00},
	{0x3d0e, 0x00},
	{0x3d0f, 0x00},
	{0x3d10, 0x00},
	{0x3d11, 0x00},
	{0x3d12, 0x00},
	{0x3d13, 0x00},
	{0x3d14, 0x00},
	{0x3d15, 0x00},
	{0x3d16, 0x00},
	{0x3d17, 0x00},
	{0x3d18, 0x00},
	{0x3d19, 0x00},
	{0x3d1a, 0x00},
	{0x3d1b, 0x00},
	{0x3d1c, 0x00},
	{0x3d1d, 0x00},
	{0x3d1e, 0x00},
	{0x3d1f, 0x00},
	{0x3d80, 0x00},
	{0x3d81, 0x00},

	{0x3d84, 0x00},
	{0x3f01, 0xfc}, //PSRAM, 0xctrl1
	{0x3f05, 0x10}, //PSRAM, 0xctrl5
	{0x3f06, 0x00},
	{0x3f07, 0x00},
	{0x4000, 0x29},
	{0x4001, 0x02}, //0xbLC start line
	{0x4002, 0x45}, //0xbLC, 0xauto, reset, 0x5, 0xframes
	{0x4003, 0x08}, //0xbLC redo, 0xat, 0x8, 0xframes
	{0x4004, 0x04}, //0x4, 0xblack lines, 0xare used, 0xfor, 0xbLC
	{0x4005, 0x18},
	{0x4300, 0xff},  //max
	{0x4303, 0x00}, //0xformat, 0xcontrol
	{0x4304, 0x08}, //output {data[7:0],, 0xdata[9:8]}
	{0x4307, 0x00}, //0xembeded, 0xcontrol
	// MIPI
	{0x4800, 0x04},
	{0x4801, 0x0f}, //0xeCC, 0xconfigure
	{0x4843, 0x02}, //manual set pclk, 0xdivider
	// ISP
	{0x5000, 0x06}, //LENC off,, 0xbPC on, WPC on
	{0x5001, 0x00}, //MWB off
	{0x5002, 0x00},
	{0x501f, 0x00}, //0xenable ISP
	{0x5780, 0xfc},
	{0x5c00, 0x80}, //PBLC, 0xcTRL00
	{0x5c01, 0x00}, //PBLC, 0xcTRL01
	{0x5c02, 0x00}, //PBLC, 0xcTRL02
	{0x5c03, 0x00}, //PBLC, 0xcTRL03
	{0x5c04, 0x00}, //PBLC, 0xcTRL04
	{0x5c05, 0x00}, //pre, 0xbLC
	{0x5c06, 0x00}, //pre, 0xbLC
	{0x5c07, 0x80}, //pre, 0xbLC
	{0x5c08, 0x10}, //PBLC, 0xcTRL08
	{0x6700, 0x05},
	{0x6701, 0x19},
	{0x6702, 0xfd},
	{0x6703, 0xd7},

	{0x6704, 0xff},
	{0x6705, 0xff},
	{0x6800, 0x10},
	{0x6801, 0x02},
	{0x6802, 0x90},
	{0x6803, 0x10},
	{0x6804, 0x59},
	{0x6900, 0x60}, //0xcADC, 0xcTRL00
	{0x6901, 0x04}, //0xcADC, 0xcontrol;, 0xdefault Lens, 0xcorrection
	{0x5800, 0x0f},
	{0x5801, 0x0d},
	{0x5802, 0x09},
	{0x5803, 0x0a},
	{0x5804, 0x0d},
	{0x5805, 0x14},
	{0x5806, 0x0a},
	{0x5807, 0x04},
	{0x5808, 0x03},
	{0x5809, 0x03},
	{0x580a, 0x05},
	{0x580b, 0x0a},
	{0x580c, 0x05},
	{0x580d, 0x02},
	{0x580e, 0x00},
	{0x580f, 0x00},
	{0x5810, 0x03},
	{0x5811, 0x05},
	{0x5812, 0x09},
	{0x5813, 0x03},
	{0x5814, 0x01},
	{0x5815, 0x01},
	{0x5816, 0x04},
	{0x5817, 0x09},
	{0x5818, 0x09},
	{0x5819, 0x08},
	{0x581a, 0x06},
	{0x581b, 0x06},
	{0x581c, 0x08},
	{0x581d, 0x06},
	{0x581e, 0x33},
	{0x581f, 0x11},
	{0x5820, 0x0e},
	{0x5821, 0x0f},
	{0x5822, 0x11},
	{0x5823, 0x3f},
	{0x5824, 0x08},
	{0x5825, 0x46},
	{0x5826, 0x46},
	{0x5827, 0x46},
	{0x5828, 0x46},
	{0x5829, 0x46},
	{0x582a, 0x42},
	{0x582b, 0x42},
	{0x582c, 0x44},
	{0x582d, 0x46},
	{0x582e, 0x46},
	{0x582f, 0x60},
	{0x5830, 0x62},
	{0x5831, 0x42},
	{0x5832, 0x46},
	{0x5833, 0x46},
	{0x5834, 0x44},
	{0x5835, 0x44},
	{0x5836, 0x44},
	{0x5837, 0x48},
	{0x5838, 0x28},
	{0x5839, 0x46},
	{0x583a, 0x48},
	{0x583b, 0x68},
	{0x583c, 0x28},
	{0x583d, 0xae},
	{0x5842, 0x00},
	{0x5843, 0xef},
	{0x5844, 0x01},
	{0x5845, 0x3f},
	{0x5846, 0x01},
	{0x5847, 0x3f},
	{0x5848, 0x00},
	{0x5849, 0xd5},
	// PLL setting, 0xfor preview
	{0x3003, 0xce}, //PLL_CTRL0
	{0x3004, 0xce}, //PLL_CTRL1
	{0x3005, 0x10}, //PLL_CTRL2
	{0x3006, 0x10}, //PLL_CTRL3
	{0x3007, 0x3b}, //PLL_CTRL4
	{0x3012, 0x81}, //SC_PLL, 0xcTRL_S0
	{0x3013, 0x39}, //SC_PLL, 0xcTRL_S1
	{0x3106, 0x11}, //SRB_CTRL
	// 0xexposure
	{0x3503, 0x07}, //Gain has no, 0xdelay, VTS manual,, 0xaGC manual,, 0xaEC manual
	{0x3500, 0x00}, //0xexpo[19:16] = lines/16

	{0x3501, 0x27}, //0xexpo[15:8]
	{0x3502, 0x00}, //0xexpo[7:0]
	{0x350b, 0xff}, //gain
	// MWB
	{0x3400, 0x04}, //red h
	{0x3401, 0x00}, //red l
	{0x3402, 0x04}, //green h
	{0x3403, 0x00}, //green l
	{0x3404, 0x04}, //0xblue h
	{0x3405, 0x00}, //0xblue l
	{0x3406, 0x01}, //MWB manual
	// ISP
	{0x5001, 0x01}, //MWB on
	{0x5000, 0x06}, //LENC off,, 0xbPC on, WPC on

//1920_1080_4lane_30fps_100Msysclk_408MBps/lane
	{0x3003, 0xce}, // PLL_CTRL0
	{0x3004, 0xce}, // PLL_CTRL1
	{0x3005, 0x10}, // PLL_CTRL2
	{0x3006, 0x00}, // PLL_CTRL3
	{0x3007, 0x3b}, // PLL_CTRL4
	{0x3012, 0x80}, // SC_PLL CTRL_S0
	{0x3013, 0x39}, // SC_PLL CTRL_S1
	{0x3106, 0x15}, // SRB_CTRL
	{0x3600, 0x06}, // ANACTRL0
	{0x3601, 0x34}, // ANACTRL1
	{0x3602, 0x42}, //
	{0x3700, 0x20}, // SENCTROL0 Sensor control
	{0x3702, 0x50}, // SENCTROL2 Sensor control
	{0x3703, 0xcc}, // SENCTROL3 Sensor control
	{0x3704, 0x19}, // SENCTROL4 Sensor control
	{0x3705, 0x14}, // SENCTROL5 Sensor control
	{0x3706, 0x4b}, // SENCTROL6 Sensor control
	{0x3708, 0x84}, // SENCTROL8 Sensor control
	{0x3709, 0x40}, // SENCTROL9 Sensor control
	{0x370a, 0x31}, // SENCTROLA Sensor control
	{0x370e, 0x00}, // SENCTROLE Sensor control
	{0x3711, 0x0f}, // SENCTROL11 Sensor control
	{0x3712, 0x9c}, // SENCTROL12 Sensor control
	{0x3724, 0x01}, // Reserved
	{0x3725, 0x92}, // Reserved
	{0x3726, 0x01}, // Reserved
	{0x3727, 0xa9}, // Reserved
	{0x3800, 0x00}, // HS(HREF start High)
	{0x3801, 0x00}, // HS(HREF start Low)
	{0x3802, 0x01}, // VS(Vertical start High)
	{0x3803, 0x30}, // VS(Vertical start Low)
	{0x3804, 0x0c}, // HW = 3295
	{0x3805, 0xdf}, // HW
	{0x3806, 0x08}, // VH = 2151
	{0x3807, 0x67}, // VH
	{0x3808, 0x07}, // ISPHO = 1920
	{0x3809, 0x80}, // ISPHO
	{0x380a, 0x04}, // ISPVO = 1080
	{0x380b, 0x38}, // ISPVO
	{0x380c, 0x0d}, // HTS = 3568
	{0x380d, 0xf0}, // HTS
	{0x380e, 0x07}, // VTS = 1868
	{0x380f, 0x4c}, // VTS
	{0x3810, 0x00}, // HOFF = 16
	{0x3811, 0x10}, // HOFF
	{0x3812, 0x00}, // VOFF = 6
	{0x3813, 0x06}, // VOFF
	{0x3814, 0x11}, // X INC
	{0x3815, 0x11}, // Y INC
#ifdef CONFIG_CAMERA_IMAGE_180
	{0x3820, 0x80}, // Timing Reg20:Vflip
	{0x3821, 0x16}, // Timing Reg21:Hmirror
#else
	{0x3820, 0x86}, // Timing Reg20:Vflip
	{0x3821, 0x10}, // Timing Reg21:Hmirror
#endif
	{0x3f00, 0x02}, // PSRAM Ctrl0
	{0x4005, 0x18}, // Gain triger for BLC
	{0x404f, 0x8F}, // Auto BLC while more than value
	{0x4600, 0x04}, // VFIFO Ctrl0
	{0x4601, 0x01}, // VFIFO Read ST High
	{0x4602, 0x00}, // VFIFO Read ST Low
	{0x4837, 0x15}, // MIPI PCLK PERIOD
	{0x5068, 0x53}, // HSCALE_CTRL
	{0x506a, 0x53}, // VSCALE_CTRL
//	{0x0100, 0x01} // wake up
#endif
};
//@@ FVGA 2lane 720x480 60fps
LOCAL const SENSOR_REG_T ov8825_720x480_setting[] = {
	{0x0103, 0x01},
	{0x3000, 0x16},
	{0x3001, 0x00},
	{0x3002, 0x6c},
	{0x3003, 0xce},
	{0x3004, 0xd0},
	{0x3005, 0x00},
	{0x3006, 0x10},
	{0x3007, 0x3b},
	{0x300d, 0x00},
	{0x301f, 0x09},
	{0x3020, 0x01},
	{0x3010, 0x00},
	{0x3011, 0x01},
	{0x3012, 0x80},
	{0x3013, 0x39},
	{0x3018, 0x00},
	{0x3104, 0x20},
	{0x3106, 0x15},
	{0x3300, 0x00},
	{0x3500, 0x00},
	{0x3501, 0x27},
	{0x3502, 0x40},
	{0x3503, 0x07},
	{0x3509, 0x00},
	{0x350b, 0x7f},
	{0x3600, 0x06},
	{0x3601, 0x34},
	{0x3602, 0x42},
	{0x3603, 0x5c},
	{0x3604, 0x98},
	{0x3605, 0xf5},
	{0x3609, 0xb4},
	{0x360a, 0x7c},
	{0x360b, 0xc9},
	{0x360c, 0x0b},
	{0x3612, 0x00},
	{0x3613, 0x02},
	{0x3614, 0x0f},
	{0x3615, 0x00},
	{0x3616, 0x03},
	{0x3617, 0xa1},
	{0x3618, 0x00},
	{0x3619, 0x00},
	{0x361a, 0xB0}, // VCM clock divider, VCM c
	{0x361b, 0x04}, // VCM clock divider
	{0x3700, 0x20},
	{0x3701, 0x44},
	{0x3702, 0x50},
	{0x3703, 0xcc},
	{0x3704, 0x19},
	{0x3705, 0x32},
	{0x3706, 0x4b},
	{0x3707, 0x63},
	{0x3708, 0x84},
	{0x3709, 0x40},
	{0x370a, 0xb2},
	{0x370b, 0x01},
	{0x370c, 0x50},
	{0x370d, 0x00},
	{0x370e, 0x08},
	{0x3711, 0x0f},
	{0x3712, 0x9c},
	{0x3724, 0x01},
	{0x3725, 0x92},
	{0x3726, 0x01},
	{0x3727, 0xc7},
	{0x3800, 0x00},
	{0x3801, 0x08},
	{0x3802, 0x00},
	{0x3803, 0x00},
	{0x3804, 0x0c},
	{0x3805, 0xd7},
	{0x3806, 0x09},
	{0x3807, 0x97},
	{0x3808, 0x02},
	{0x3809, 0xd0},
	{0x380a, 0x01},
	{0x380b, 0xe0},
	{0x380c, 0x0d},
	{0x380d, 0xb0},
	{0x380e, 0x02},
	{0x380f, 0x7a},
	{0x3810, 0x00},
	{0x3811, 0x2c},
	{0x3812, 0x00},
	{0x3813, 0x02},
	{0x3814, 0x71},
	{0x3815, 0x35},
	{0x3816, 0x02},
	{0x3817, 0x40},
	{0x3818, 0x00},
	{0x3819, 0x40},
#ifdef CONFIG_CAMERA_IMAGE_180
	{0x3820, 0x80}, // Timing Reg20:Vflip
	{0x3821, 0x16}, // Timing Reg21:Hmirror
#else
	{0x3820, 0x86}, // Timing Reg20:Vflip
	{0x3821, 0x10}, // Timing Reg21:Hmirror
#endif
	{0x3b1f, 0x00},
	{0x3d00, 0x00},
	{0x3d01, 0x00},
	{0x3d02, 0x00},
	{0x3d03, 0x00},
	{0x3d04, 0x00},
	{0x3d05, 0x00},
	{0x3d06, 0x00},
	{0x3d07, 0x00},
	{0x3d08, 0x00},
	{0x3d09, 0x00},
	{0x3d0a, 0x00},
	{0x3d0b, 0x00},
	{0x3d0c, 0x00},
	{0x3d0d, 0x00},
	{0x3d0e, 0x00},
	{0x3d0f, 0x00},
	{0x3d10, 0x00},
	{0x3d11, 0x00},
	{0x3d12, 0x00},
	{0x3d13, 0x00},
	{0x3d14, 0x00},
	{0x3d15, 0x00},
	{0x3d16, 0x00},
	{0x3d17, 0x00},
	{0x3d18, 0x00},
	{0x3d19, 0x00},
	{0x3d1a, 0x00},
	{0x3d1b, 0x00},
	{0x3d1c, 0x00},
	{0x3d1d, 0x00},
	{0x3d1e, 0x00},
	{0x3d1f, 0x00},
	{0x3d80, 0x00},
	{0x3d81, 0x00},
	{0x3d84, 0x00},
	{0x3f00, 0x00},
	{0x3f01, 0xfc},
	{0x3f05, 0x10},
	{0x3f06, 0x00},
	{0x3f07, 0x00},
	{0x4000, 0x29},
	{0x4001, 0x02},
	{0x4002, 0x45},
	{0x4003, 0x08},
	{0x4004, 0x04},
	{0x4005, 0x18},
	{0x404e, 0x37},
	{0x404f, 0x8f},
	{0x4300, 0xff},
	{0x4303, 0x00},
	{0x4304, 0x08},
	{0x4307, 0x00},
	{0x4600, 0x14},
	{0x4601, 0x14},
	{0x4602, 0x00},
	{0x4800, 0x04},
	{0x4801, 0x0f},
	{0x4837, 0x15},
	{0x4843, 0x02},
	{0x5000, 0x06},
	{0x5001, 0x00},
	{0x5002, 0x00},
	{0x5068, 0x59},
	{0x506a, 0x5a},
	{0x501f, 0x00},
	{0x5780, 0xfc},
	{0x5c00, 0x80},
	{0x5c01, 0x00},
	{0x5c02, 0x00},
	{0x5c03, 0x00},
	{0x5c04, 0x00},
	{0x5c05, 0x00},
	{0x5c06, 0x00},
	{0x5c07, 0x80},
	{0x5c08, 0x10},
	{0x6700, 0x05},
	{0x6701, 0x19},
	{0x6702, 0xfd},
	{0x6703, 0xd7},
	{0x6704, 0xff},
	{0x6705, 0xff},
	{0x6800, 0x10},
	{0x6801, 0x02},
	{0x6802, 0x90},
	{0x6803, 0x10},
	{0x6804, 0x59},
	{0x6900, 0x60},
	{0x6901, 0x04},
	{0x5800, 0x0f},
	{0x5801, 0x0d},
	{0x5802, 0x09},
	{0x5803, 0x0a},
	{0x5804, 0x0d},
	{0x5805, 0x14},
	{0x5806, 0x0a},
	{0x5807, 0x04},
	{0x5808, 0x03},
	{0x5809, 0x03},
	{0x580a, 0x05},
	{0x580b, 0x0a},
	{0x580c, 0x05},
	{0x580d, 0x02},
	{0x580e, 0x00},
	{0x580f, 0x00},
	{0x5810, 0x03},
	{0x5811, 0x05},
	{0x5812, 0x09},
	{0x5813, 0x03},
	{0x5814, 0x01},
	{0x5815, 0x01},
	{0x5816, 0x04},
	{0x5817, 0x09},
	{0x5818, 0x09},
	{0x5819, 0x08},
	{0x581a, 0x06},
	{0x581b, 0x06},
	{0x581c, 0x08},
	{0x581d, 0x06},
	{0x581e, 0x33},
	{0x581f, 0x11},
	{0x5820, 0x0e},
	{0x5821, 0x0f},
	{0x5822, 0x11},
	{0x5823, 0x3f},
	{0x5824, 0x08},
	{0x5825, 0x46},
	{0x5826, 0x46},
	{0x5827, 0x46},
	{0x5828, 0x46},
	{0x5829, 0x46},
	{0x582a, 0x42},
	{0x582b, 0x42},
	{0x582c, 0x44},
	{0x582d, 0x46},
	{0x582e, 0x46},
	{0x582f, 0x60},
	{0x5830, 0x62},
	{0x5831, 0x42},
	{0x5832, 0x46},
	{0x5833, 0x46},
	{0x5834, 0x44},
	{0x5835, 0x44},
	{0x5836, 0x44},
	{0x5837, 0x48},
	{0x5838, 0x28},
	{0x5839, 0x46},
	{0x583a, 0x48},
	{0x583b, 0x68},
	{0x583c, 0x28},
	{0x583d, 0xae},
	{0x5842, 0x00},
	{0x5843, 0xef},
	{0x5844, 0x01},
	{0x5845, 0x3f},
	{0x5846, 0x01},
	{0x5847, 0x3f},
	{0x5848, 0x00},
	{0x5849, 0xd5},
};

LOCAL const SENSOR_REG_T ov8825_1632x1224_setting[] = {
#if defined(OV8825_2_LANES)
	//@@1632_1224_2Lane_30fps_66.66Msys
	//{0x0100, 0x00}, // sleep
	{0x3003, 0xce}, // PLL_CTRL0
	{0x3004, 0xc0}, // PLL_CTRL1
	{0x3005, 0x10}, // PLL_CTRL2
	{0x3006, 0x10}, // PLL_CTRL3
	{0x3007, 0x3b}, // PLL_CTRL4
	{0x3012, 0x81}, // SC_PLL CTRL_S0
	{0x3013, 0x39}, // SC_PLL CTRL_S1
	{0x3106, 0x11}, // SRB_CTRL
	{0x3600, 0x07}, // ANACTRL0
	{0x3601, 0x33}, // ANACTRL1
	{0x3602, 0xc2}, //
	{0x361a, 0xB0}, // VCM clock divider, VCM c
	{0x361b, 0x04}, // VCM clock divider
	{0x3700, 0x10}, // SENCTROL0 Sensor c
	{0x3702, 0x28}, // SENCTROL2 Sensor c
	{0x3703, 0x6c}, // SENCTROL3 Sensor c
	{0x3704, 0x8d}, // SENCTROL4 Sensor c
	{0x3705, 0x0a}, // SENCTROL5 Sensor c
	{0x3706, 0x27}, // SENCTROL6 Sensor c
	{0x3708, 0x40}, // SENCTROL8 Sensor c
	{0x3709, 0x20}, // SENCTROL9 Sensor c
	{0x370a, 0x33}, // SENCTROLA Sensor c
	{0x370e, 0x08}, // SENCTROLE Sensor c
	{0x3711, 0x07}, // SENCTROL11 Sensor
	{0x3712, 0x4e}, // SENCTROL12 Sensor
	{0x3724, 0x00}, // Reserved
	{0x3725, 0xd4}, // Reserved
	{0x3726, 0x00}, // Reserved
	{0x3727, 0xe1}, // Reserved
	{0x3800, 0x00}, // HS(HREF start High
	{0x3801, 0x00}, // HS(HREF start Low)
	{0x3802, 0x00}, // VS(Vertical start
	{0x3803, 0x00}, // VS(Vertical start
	{0x3804, 0x0c}, // HW = 3295
	{0x3805, 0xdf}, // HW
	{0x3806, 0x09}, // VH = 2459
	{0x3807, 0x9b}, // VH
	{0x3808, 0x06}, // ISPHO = 1632
	{0x3809, 0x60}, // ISPHO
	{0x380a, 0x04}, // ISPVO = 1224
	{0x380b, 0xc8}, // ISPVO
	{0x380c, 0x0d}, // HTS = 3516
	{0x380d, 0xbc}, // HTS

    {0x380e, 0x04}, // VTS = 1264
	{0x380f, 0xf0}, // VTS
	/*
	{0x380e, 0x05}, // VTS = 1264
	{0x380f, 0xe8}, // VTS
	*/
	{0x3810, 0x00}, // HOFF = 8
	{0x3811, 0x08}, // HOFF
	{0x3812, 0x00}, // VOFF = 4
	{0x3813, 0x04}, // VOFF
	{0x3814, 0x31}, // X INC
	{0x3815, 0x31}, // Y INC
#ifdef CONFIG_CAMERA_IMAGE_180
	{0x3820, 0x80}, // Timing Reg20:Vflip
	{0x3821, 0x17}, // Timing Reg21:Hmirr
#else
	{0x3820, 0x86}, // Timing Reg20:Vflip
	{0x3821, 0x11}, // Timing Reg21:Hmirr
#endif
	{0x3f00, 0x00}, // PSRAM Ctrl0
	{0x4005, 0x18}, // Gain trigger for B
	{0x404f, 0x8F}, // Auto BLC while mor
	{0x4600, 0x04}, // VFIFO Ctrl0
	{0x4601, 0x00}, // VFIFO Read ST High
	{0x4602, 0x78}, // VFIFO Read ST Low
	{0x4837, 0x15}, // MIPI PCLK PERIOD
	{0x5068, 0x00}, // HSCALE_CTRL
	{0x506a, 0x00}, // VSCALE_CTRL
	//{0x0100, 0x01} // wake up
#elif defined(OV8825_4_LANES)
	//{0x0100, 0x00}, //sleep
	{0x3003, 0xce}, //PLL_CTRL0
	{0x3004, 0xce}, //PLL_CTRL1
	{0x3005, 0x10}, //PLL_CTRL2
	{0x3006, 0x10}, //PLL_CTRL3
	{0x3007, 0x3b}, //PLL_CTRL4
	{0x3012, 0x81}, //SC_PLL, 0xcTRL_S0
	{0x3013, 0x39}, //SC_PLL, 0xcTRL_S1
	{0x3106, 0x11}, //SRB_CTRL
	{0x3600, 0x07}, //0xaNACTRL0
	{0x3601, 0x33}, //0xaNACTRL1
	{0x3602, 0xc2},
	{0x361a, 0xB0}, // VCM clock divider, VCM c
	{0x361b, 0x04}, // VCM clock divider
	{0x3700, 0x10}, //SENCTROL0 Sensor, 0xcontrol
	{0x3702, 0x28}, //SENCTROL2 Sensor, 0xcontrol
	{0x3703, 0x6c}, //SENCTROL3 Sensor, 0xcontrol
	{0x3704, 0x8d}, //SENCTROL4 Sensor, 0xcontrol
	{0x3705, 0x0a}, //SENCTROL5 Sensor, 0xcontrol
	{0x3706, 0x27}, //SENCTROL6 Sensor, 0xcontrol
	{0x3708, 0x40}, //SENCTROL8 Sensor, 0xcontrol
	{0x3709, 0x20}, //SENCTROL9 Sensor, 0xcontrol
	{0x370a, 0x33}, //SENCTROLA Sensor, 0xcontrol
	{0x370e, 0x08}, //SENCTROLE Sensor, 0xcontrol
	{0x3711, 0x07}, //SENCTROL11 Sensor, 0xcontrol
	{0x3712, 0x4e}, //SENCTROL12 Sensor, 0xcontrol
	{0x3724, 0x00}, //Reserved

	{0x3725, 0xd4}, //Reserved
	{0x3726, 0x00}, //Reserved
	{0x3727, 0xe1}, //Reserved
	{0x3800, 0x00}, //HS(HREF start High)
	{0x3801, 0x00}, //HS(HREF start Low)
	{0x3802, 0x00}, //VS(Vertical start High)
	{0x3803, 0x00}, //VS(Vertical start Low)
	{0x3804, 0x0c}, //HW =, 0x3295
	{0x3805, 0xdf}, //HW
	{0x3806, 0x09}, //VH =, 0x2459
	{0x3807, 0x9b}, //VH
	{0x3808, 0x06}, //ISPHO =, 0x1632
	{0x3809, 0x60}, //ISPHO
	{0x380a, 0x04}, //ISPVO =, 0x1224
	{0x380b, 0xc8}, //ISPVO
	{0x380c, 0x0b}, //HTS =, 0x3516
	{0x380d, 0x72}, //HTS
	{0x380e, 0x05}, //VTS =, 0x1264
	{0x380f, 0xf4}, //VTS
	{0x3810, 0x00}, //HOFF =, 0x8
	{0x3811, 0x08}, //HOFF
	{0x3812, 0x00}, //VOFF =, 0x4
	{0x3813, 0x04}, //VOFF
	{0x3814, 0x31}, //X INC
	{0x3815, 0x31}, //Y INC
#ifdef CONFIG_CAMERA_IMAGE_180
	{0x3820, 0x80}, // Timing Reg20:Vflip
	{0x3821, 0x17}, // Timing Reg21:Hmirr
#else
	{0x3820, 0x86}, // Timing Reg20:Vflip
	{0x3821, 0x11}, // Timing Reg21:Hmirr
#endif
	{0x3f00, 0x00}, //PSRAM, 0xctrl0
	{0x4005, 0x18}, //Gain triger, 0xfor, 0xbLC
	{0x404f, 0x8F}, //0xauto, 0xbLC while more than value
	{0x4600, 0x04}, //VFIFO, 0xctrl0
	{0x4601, 0x00}, //VFIFO Read ST High
	{0x4602, 0x78}, //VFIFO Read ST Low
	{0x4837, 0x15}, // MIPI PCLK PERIOD
	{0x5068, 0x00}, //HSCALE_CTRL
	{0x506a, 0x00}, //VSCALE_CTRL
	//{0x0100, 0x01}, //wake up
#endif
	{0x301a, 0x71},
	{0x301c, 0xf4},
	{0x0100, 0x01}, //wake up
};

LOCAL const SENSOR_REG_T ov8825_1920x1080_setting[] = {
#if defined(OV8825_2_LANES)
//@@1920_1080_2Lane_30fps_100Msysclk_720MBps/lane
	{0x3003, 0xce}, // PLL_CTRL0
	{0x3004, 0xd4}, // PLL_CTRL1
	{0x3005, 0x00}, // PLL_CTRL2
	{0x3006, 0x00}, // PLL_CTRL3
	{0x3007, 0x3b}, // PLL_CTRL4
	{0x3012, 0x80}, // SC_PLL CTRL_S0
	{0x3013, 0x39}, // SC_PLL CTRL_S1
	{0x3106, 0x15}, // SRB_CTRL
	{0x3600, 0x06}, // ANACTRL0
	{0x3601, 0x34}, // ANACTRL1
	{0x3602, 0x42}, //
	{0x361a, 0xB0}, // VCM clock divider, VCM c
	{0x361b, 0x04}, // VCM clock divider
	{0x3700, 0x20}, // SENCTROL0 Sensor control
	{0x3702, 0x50}, // SENCTROL2 Sensor control
	{0x3703, 0xcc}, // SENCTROL3 Sensor control
	{0x3704, 0x19}, // SENCTROL4 Sensor control
	{0x3705, 0x14}, // SENCTROL5 Sensor control
	{0x3706, 0x4b}, // SENCTROL6 Sensor control
	{0x3708, 0x84}, // SENCTROL8 Sensor control
	{0x3709, 0x40}, // SENCTROL9 Sensor control
	{0x370a, 0x31}, // SENCTROLA Sensor control
	{0x370e, 0x00}, // SENCTROLE Sensor control
	{0x3711, 0x0f}, // SENCTROL11 Sensor control
	{0x3712, 0x9c}, // SENCTROL12 Sensor control
	{0x3724, 0x01}, // Reserved
	{0x3725, 0x92}, // Reserved
	{0x3726, 0x01}, // Reserved
	{0x3727, 0xa9}, // Reserved
	{0x3800, 0x00}, // HS(HREF start High)
	{0x3801, 0x00}, // HS(HREF start Low)
	{0x3802, 0x01}, // VS(Vertical start High)
	{0x3803, 0x30}, // VS(Vertical start Low)
	{0x3804, 0x0c}, // HW = 3295
	{0x3805, 0xdf}, // HW
	{0x3806, 0x08}, // VH = 2151
	{0x3807, 0x67}, // VH
	{0x3808, 0x07}, // ISPHO = 1920
	{0x3809, 0x80}, // ISPHO
	{0x380a, 0x04}, // ISPVO = 1080
	{0x380b, 0x38}, // ISPVO
	{0x380c, 0x0d}, // HTS = 3568
	{0x380d, 0xf0}, // HTS
	{0x380e, 0x07}, // VTS = 1868
	{0x380f, 0x4c}, // VTS
	{0x3810, 0x00}, // HOFF = 16
	{0x3811, 0x10}, // HOFF
	{0x3812, 0x00}, // VOFF = 6
	{0x3813, 0x06}, // VOFF
	{0x3814, 0x11}, // X INC
	{0x3815, 0x11}, // Y INC
#ifdef CONFIG_CAMERA_IMAGE_180
	{0x3820, 0x80}, // Timing Reg20:Vflip
	{0x3821, 0x16}, // Timing Reg21:Hmirror
#else
	{0x3820, 0x86}, // Timing Reg20:Vflip
	{0x3821, 0x10}, // Timing Reg21:Hmirror
#endif
	{0x3f00, 0x02}, //PSRAM Ctrl0
	{0x4005, 0x18}, // Gain triger for BLC
	{0x404f, 0x8F}, // Auto BLC while more than value
	{0x4600, 0x04}, // VFIFO Ctrl0
	{0x4601, 0x01}, // VFIFO Read ST High
	{0x4602, 0x00}, // VFIFO Read ST Low
	{0x4837, 0x15}, // MIPI PCLK PERIOD
	{0x5068, 0x53}, // HSCALE_CTRL
	{0x506a, 0x53}, // VSCALE_CTRL
#elif defined(OV8825_4_LANES)
//1920_1080_4lane_30fps_100Msysclk_408MBps/lane
	{0x3003, 0xce}, // PLL_CTRL0
	{0x3004, 0xce}, // PLL_CTRL1
	{0x3005, 0x10}, // PLL_CTRL2
	{0x3006, 0x00}, // PLL_CTRL3
	{0x3007, 0x3b}, // PLL_CTRL4
	{0x3012, 0x80}, // SC_PLL CTRL_S0
	{0x3013, 0x39}, // SC_PLL CTRL_S1
	{0x3106, 0x15}, // SRB_CTRL
	{0x3600, 0x06}, // ANACTRL0
	{0x3601, 0x34}, // ANACTRL1
	{0x3602, 0x42}, //
	{0x361a, 0xB0}, // VCM clock divider, VCM c
	{0x361b, 0x04}, // VCM clock divider
	{0x3700, 0x20}, // SENCTROL0 Sensor control
	{0x3702, 0x50}, // SENCTROL2 Sensor control
	{0x3703, 0xcc}, // SENCTROL3 Sensor control
	{0x3704, 0x19}, // SENCTROL4 Sensor control
	{0x3705, 0x14}, // SENCTROL5 Sensor control
	{0x3706, 0x4b}, // SENCTROL6 Sensor control
	{0x3708, 0x84}, // SENCTROL8 Sensor control
	{0x3709, 0x40}, // SENCTROL9 Sensor control
	{0x370a, 0x31}, // SENCTROLA Sensor control
	{0x370e, 0x00}, // SENCTROLE Sensor control
	{0x3711, 0x0f}, // SENCTROL11 Sensor control
	{0x3712, 0x9c}, // SENCTROL12 Sensor control
	{0x3724, 0x01}, // Reserved
	{0x3725, 0x92}, // Reserved
	{0x3726, 0x01}, // Reserved
	{0x3727, 0xa9}, // Reserved
	{0x3800, 0x00}, // HS(HREF start High)
	{0x3801, 0x00}, // HS(HREF start Low)
	{0x3802, 0x01}, // VS(Vertical start High)
	{0x3803, 0x30}, // VS(Vertical start Low)
	{0x3804, 0x0c}, // HW = 3295
	{0x3805, 0xdf}, // HW
	{0x3806, 0x08}, // VH = 2151
	{0x3807, 0x67}, // VH
	{0x3808, 0x07}, // ISPHO = 1920
	{0x3809, 0x80}, // ISPHO
	{0x380a, 0x04}, // ISPVO = 1080
	{0x380b, 0x38}, // ISPVO
	{0x380c, 0x0d}, // HTS = 3568
	{0x380d, 0xf0}, // HTS
	{0x380e, 0x07}, // VTS = 1868
	{0x380f, 0x4c}, // VTS
	{0x3810, 0x00}, // HOFF = 16
	{0x3811, 0x10}, // HOFF
	{0x3812, 0x00}, // VOFF = 6
	{0x3813, 0x06}, // VOFF
	{0x3814, 0x11}, // X INC
	{0x3815, 0x11}, // Y INC
#ifdef CONFIG_CAMERA_IMAGE_180
	{0x3820, 0x80}, // Timing Reg20:Vflip
	{0x3821, 0x16}, // Timing Reg21:Hmirror
#else
	{0x3820, 0x86}, // Timing Reg20:Vflip
	{0x3821, 0x10}, // Timing Reg21:Hmirror
#endif
	{0x3f00, 0x02}, // PSRAM Ctrl0
	{0x4005, 0x18}, // Gain triger for BLC
	{0x404f, 0x8F}, // Auto BLC while more than value
	{0x4600, 0x04}, // VFIFO Ctrl0
	{0x4601, 0x01}, // VFIFO Read ST High
	{0x4602, 0x00}, // VFIFO Read ST Low
	{0x4837, 0x15}, // MIPI PCLK PERIOD
	{0x5068, 0x53}, // HSCALE_CTRL
	{0x506a, 0x53}, // VSCALE_CTRL
#endif
};

LOCAL const SENSOR_REG_T ov8825_3264x2448_setting[] = {
#if defined(OV8825_2_LANES)
	//@@3264_2448_2lane_15fps_66.66Msysc
	//3264 x 2448
	{0x3003, 0xce}, // PLL_CTRL0
	{0x3004, 0xd8}, // PLL_CTRL1
	{0x3005, 0x00}, // PLL_CTRL2
	{0x3006, 0x10}, // PLL_CTRL3
	{0x3007, 0x3b}, // PLL_CTRL4
	{0x3012, 0x81}, // SC_PLL CTRL_S0
	{0x3013, 0x39}, // SC_PLL CTRL_S1
	{0x3106, 0x11}, // SRB_CTRL
	{0x3600, 0x07}, // ANACTRL0
	{0x3601, 0x33}, // ANACTRL1
	{0x3602, 0x42}, //
	{0x361a, 0xB0}, // VCM clock divider, VCM c
	{0x361b, 0x04}, // VCM clock divider
	{0x3700, 0x10}, // SENCTROL0 Sensor con
	{0x3702, 0x28}, // SENCTROL2 Sensor con
	{0x3703, 0x6c}, // SENCTROL3 Sensor con
	{0x3704, 0x8d}, // SENCTROL4 Sensor con
	{0x3705, 0x0a}, // SENCTROL5 Sensor con
	{0x3706, 0x27}, // SENCTROL6 Sensor con
	{0x3708, 0x40}, // SENCTROL8 Sensor con
	{0x3709, 0x20}, // SENCTROL9 Sensor con
	{0x370a, 0x31}, // SENCTROLA Sensor con
	{0x370e, 0x00}, // SENCTROLE Sensor con
	{0x3711, 0x07}, // SENCTROL11 Sensor co
	{0x3712, 0x4e}, // SENCTROL12 Sensor co
	{0x3724, 0x00}, // Reserved
	{0x3725, 0xd4}, // Reserved
	{0x3726, 0x00}, // Reserved
	{0x3727, 0xe1}, // Reserved
	{0x3800, 0x00}, // HS(HREF start High)
	{0x3801, 0x00}, // HS(HREF start Low)
	{0x3802, 0x00}, // VS(Vertical start Hi
	{0x3803, 0x00}, // VS(Vertical start Lo
	{0x3804, 0x0c}, // HW = 3295
	{0x3805, 0xdf}, // HW
	{0x3806, 0x09}, // VH = 2459
	{0x3807, 0x9b}, // VH
	{0x3808, 0x0c}, // ISPHO = 3264
	{0x3809, 0xc0}, // ISPHO
	{0x380a, 0x09}, // ISPVO = 2448
	{0x380b, 0x90}, // ISPVO
	{0x380c, 0x0e}, // HTS = 3584
	{0x380d, 0x00}, // HTS
	{0x380e, 0x09}, // VTS = 2480
	{0x380f, 0xb0}, // VTS
	{0x3810, 0x00}, // HOFF = 16
	{0x3811, 0x10}, // HOFF
	{0x3812, 0x00}, // VOFF = 6
	{0x3813, 0x06}, // VOFF
	{0x3814, 0x11}, // X INC
	{0x3815, 0x11}, // Y INC
#ifdef CONFIG_CAMERA_IMAGE_180
	{0x3820, 0x80}, // Timing Reg20:Vflip
	{0x3821, 0x16}, // Timing Reg21:Hmirror
#else
	{0x3820, 0x86}, // Timing Reg20:Vflip
	{0x3821, 0x10}, // Timing Reg21:Hmirror
#endif
	{0x3f00, 0x02}, // PSRAM Ctrl0
	{0x4005, 0x1A}, // Every frame do BLC
	{0x404f, 0x7F}, //
	{0x4600, 0x04}, // VFIFO Ctrl0
	{0x4601, 0x00}, // VFIFO Read ST High
	{0x4602, 0x78}, // VFIFO Read ST Low
	{0x4837, 0x15}, // MIPI PCLK PERIOD
	{0x5068, 0x00}, // HSCALE_CTRL
	{0x506a, 0x00}, // VSCALE_CTRL
	//{0x0100, 0x01} // wake up
#elif defined(OV8825_4_LANES)
	//{0x0100, 0x00}, //sleep
	{0x3003, 0xce}, //PLL_CTRL0
	{0x3004, 0xbf}, //PLL_CTRL1
	{0x3005, 0x10}, //PLL_CTRL2
	{0x3006, 0x00}, //PLL_CTRL3
	{0x3007, 0x3b}, //PLL_CTRL4
	{0x3012, 0x80}, //SC_PLL, 0xcTRL_S0
	{0x3013, 0x39}, //SC_PLL, 0xcTRL_S1
	{0x3106, 0x15}, //SRB_CTRL
	{0x3600, 0x06}, //0xaNACTRL0
	{0x3601, 0x34}, //0xaNACTRL1
	{0x3602, 0x42},
	{0x361a, 0xB0}, // VCM clock divider, VCM c
	{0x361b, 0x04}, // VCM clock divider
	{0x3700, 0x20}, //SENCTROL0 Sensor, 0xcontrol
	{0x3702, 0x50}, //SENCTROL2 Sensor, 0xcontrol
	{0x3703, 0xcc}, //SENCTROL3 Sensor, 0xcontrol
	{0x3704, 0x19}, //SENCTROL4 Sensor, 0xcontrol
	{0x3705, 0x14}, //SENCTROL5 Sensor, 0xcontrol
	{0x3706, 0x4b}, //SENCTROL6 Sensor, 0xcontrol
	{0x3708, 0x84}, //SENCTROL8 Sensor, 0xcontrol
	{0x3709, 0x40}, //SENCTROL9 Sensor, 0xcontrol
	{0x370a, 0x31}, //SENCTROLA Sensor, 0xcontrol
	{0x370e, 0x00}, //SENCTROLE Sensor, 0xcontrol
	{0x3711, 0x0f}, //SENCTROL11 Sensor, 0xcontrol
	{0x3712, 0x9c}, //SENCTROL12 Sensor, 0xcontrol
	{0x3724, 0x01}, //Reserved
	{0x3725, 0x92}, //Reserved
	{0x3726, 0x01}, //Reserved
	{0x3727, 0xa9}, //Reserved
	{0x3800, 0x00}, //HS(HREF start High)
	{0x3801, 0x00}, //HS(HREF start Low)
	{0x3802, 0x00}, //VS(Vertical start High)
	{0x3803, 0x00}, //VS(Vertical start Low)
	{0x3804, 0x0c}, //HW =, 0x3295
	{0x3805, 0xdf}, //HW
	{0x3806, 0x09}, //VH =, 0x2459
	{0x3807, 0x9b}, //VH
	{0x3808, 0x0c}, //ISPHO =, 0x3264
	{0x3809, 0xc0}, //ISPHO
	{0x380a, 0x09}, //ISPVO =, 0x2448
	{0x380b, 0x90}, //ISPVO
	{0x380c, 0x0d}, //HTS =, 0x3360
	{0x380d, 0x20}, //HTS
	{0x380e, 0x09}, //VTS =, 0x2480
	{0x380f, 0xb0}, //VTS
	{0x3810, 0x00}, //HOFF =, 0x16
	{0x3811, 0x10}, //HOFF
	{0x3812, 0x00}, //VOFF =, 0x6
	{0x3813, 0x06}, //VOFF

	{0x3814, 0x11}, //X INC
	{0x3815, 0x11}, //Y INC
#ifdef CONFIG_CAMERA_IMAGE_180
	{0x3820, 0x80}, // Timing Reg20:Vflip
	{0x3821, 0x16}, // Timing Reg21:Hmirror
#else
	{0x3820, 0x86}, // Timing Reg20:Vflip
	{0x3821, 0x10}, // Timing Reg21:Hmirror
#endif
	{0x3f00, 0x02}, //PSRAM, 0xctrl0
	{0x4005, 0x1A}, //0xevery, 0xframe, 0xdo, 0xbLC
	{0x404f, 0x7F},
	{0x4600, 0x04}, //VFIFO, 0xctrl0
	{0x4601, 0x00}, //VFIFO Read ST High
	{0x4602, 0x20}, //VFIFO Read ST Low
	{0x4837, 0x15}, // MIPI PCLK PERIOD
	{0x5068, 0x00}, //HSCALE_CTRL
	{0x506a, 0x00}, //VSCALE_CTRL
	//{0x0100, 0x01}, //wake up
#endif
	{0x301a, 0x71},
	{0x301c, 0xf4},
	{0x0100, 0x01}, //wake up
};

LOCAL const SENSOR_REG_T ov8825_640x480_setting[] = {
#if  defined(OV8825_2_LANES)
#elif defined(OV8825_4_LANES)
	//@@ FVGA 4lane 640x480 30fps
	//;MIPI clock = 528M
	{0x3004, 0xbf},
	{0x3006, 0x00},
	{0x3012, 0x80},
	{0x3106, 0x15},
	{0x3600, 0x06},
	{0x3601, 0x34},
	{0x361a, 0xB0}, // VCM clock divider, VCM c
	{0x361b, 0x04}, // VCM clock divider
	{0x3700, 0x20},
	{0x3702, 0x50},
	{0x3703, 0xcc},
	{0x3704, 0x19},
	{0x3705, 0x32},
	{0x3706, 0x4b},
	{0x3708, 0x84},
	{0x3709, 0x40},
	{0x370a, 0xb2},
	{0x370d, 0x00},
	{0x3711, 0x0f},
	{0x3712, 0x9c},
	{0x3724, 0x01},
	{0x3725, 0x92},
	{0x3726, 0x01},
	{0x3727, 0xc7},
	{0x3800, 0x00},
	{0x3801, 0x08},
	{0x3802, 0x00},
	{0x3803, 0x00},
	{0x3804, 0x0c},
	{0x3805, 0xd7},
	{0x3806, 0x09},
	{0x3807, 0x97},
	{0x3808, 0x02},
	{0x3809, 0x80},
	{0x380a, 0x01},
	{0x380b, 0xe0},
	{0x380c, 0x0d},
	{0x380d, 0xb0},
	{0x380e, 0x07},
	{0x380f, 0x6e},
	{0x3810, 0x00},
	{0x3811, 0x04},
	{0x3812, 0x00},
	{0x3813, 0x02},
	{0x3814, 0x71},
	{0x3815, 0x35},
#ifdef CONFIG_CAMERA_IMAGE_180
	{0x3820, 0x80}, // Timing Reg20:Vflip
	{0x3821, 0x16}, // Timing Reg21:Hmirror
#else
	{0x3820, 0x86}, // Timing Reg20:Vflip
	{0x3821, 0x10}, // Timing Reg21:Hmirror
#endif
	{0x4600, 0x14},
	{0x4601, 0x14},
	{0x4602, 0x00},
	{0x5068, 0x59},
	{0x506a, 0x5a},
	{0x4837, 0x1e},
#endif
};

LOCAL SENSOR_REG_TAB_INFO_T s_ov8825_resolution_Tab_RAW[] = {
	{ADDR_AND_LEN_OF_ARRAY(ov8825_common_init), 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(ov8825_1632x1224_setting), 1632, 1224, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(ov8825_3264x2448_setting), 3264, 2448, 24, SENSOR_IMAGE_FORMAT_RAW},

	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

LOCAL SENSOR_TRIM_T s_ov8825_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
#if defined(OV8825_2_LANES)
	{0, 0, 1632, 1224, 263, 520, 1264, {0, 0, 1632, 1224}},
	{0, 0, 3264, 2448, 268, 656, 2480, {0, 0, 3264, 2448}},
#elif defined(OV8825_4_LANES)
	{0, 0, 1632, 1224, 219, 408, 1524, {0, 0, 1632, 1224}},
	{0, 0, 3264, 2448, 84, 528, 2480, {0, 0, 3264, 2448}},
#endif

	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}}
};

LOCAL const SENSOR_REG_T s_ov8825_640x480_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

LOCAL const SENSOR_REG_T s_ov8825_1632x1224_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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
LOCAL const SENSOR_REG_T s_ov8825_1920x1080_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

LOCAL const SENSOR_REG_T  s_ov8825_3264x2448_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

LOCAL SENSOR_VIDEO_INFO_T s_ov8825_video_info[] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
#if defined(OV8825_2_LANES)
	{{{30, 30, 263, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_ov8825_1632x1224_video_tab},
	{{{15, 15, 268, 64}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_ov8825_3264x2448_video_tab},
#elif defined(OV8825_4_LANES)
	{{{30, 30, 219, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_ov8825_1632x1224_video_tab},
	{{{15, 15, 168, 64}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_ov8825_3264x2448_video_tab},
#endif

	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}
};

LOCAL unsigned long _ov8825_set_video_mode(unsigned long param)
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

	if (PNULL == s_ov8825_video_info[mode].setting_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR)&s_ov8825_video_info[mode].setting_ptr[param];
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


LOCAL SENSOR_IOCTL_FUNC_TAB_T s_ov8825_ioctl_func_tab = {
	PNULL,
	_ov8825_PowerOn,
	PNULL,
	_ov8825_Identify,

	PNULL,			// write register
	PNULL,			// read  register
	PNULL,
	_ov8825_GetResolutionTrimTab,

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

	_ov8825_BeforeSnapshot,
	_ov8825_after_snapshot,
	_ov8825_flash,
	PNULL,
	_ov8825_write_exposure,
	PNULL,
	_ov8825_write_gain,
	PNULL,
	PNULL,
	_ov8825_write_af,
	PNULL,
	PNULL, //_ov8825_set_awb,
	PNULL,
	PNULL,
	PNULL, //_ov8825_set_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL, //_ov8825_GetExifInfo,
	_ov8825_ExtFunc,
	PNULL, //_ov8825_set_anti_flicker,
	_ov8825_set_video_mode,
	PNULL, //pick_jpeg_stream
	PNULL,  //meter_mode
	PNULL, //get_status
	_ov8825_StreamOn,
	_ov8825_StreamOff,
	_ov8825_cfg_otp,
	_ov8825_access_val
};


SENSOR_INFO_T g_ov8825_mipi_raw_info = {
	ov8825_I2C_ADDR_W,	// salve i2c write address
	ov8825_I2C_ADDR_R,	// salve i2c read address

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
	{{0x30A, 0x88},		// supply two code to identify sensor.
	 {0x30B, 0x25}},		// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,	// voltage of avdd

	3264,			// max width of source image
	2448,			// max height of source image
	"ov8825",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_B,// pattern of input image form sensor;

	s_ov8825_resolution_Tab_RAW,	// point to resolution table information structure
	&s_ov8825_ioctl_func_tab,	// point to ioctl function table
	&s_ov8825_mipi_raw_info_ptr,		// information and table about Rawrgb sensor
	NULL,			//&g_ov8825_ext_info,                // extend information about sensor
	SENSOR_AVDD_1800MV,	// iovdd
	SENSOR_AVDD_1500MV,	// dvdd
	0,			// skip frame num before preview
	2,			// skip frame num before capture
	0,			// deci frame num during preview
	0,			// deci frame num during video preview

	0,
	0,
	0,
	0,
	0,
#if defined(OV8825_2_LANES)
	{SENSOR_INTERFACE_TYPE_CSI2, 2, 10, 0},
#elif defined(OV8825_4_LANES)
	{SENSOR_INTERFACE_TYPE_CSI2, 4, 10, 0},
#endif

	s_ov8825_video_info,
	1,			// skip frame num while change setting
	48,			// horizontal view angle
	48,			// vertical view angle
};

LOCAL struct sensor_raw_info* Sensor_ov8825_GetContext(void)
{
	return s_ov8825_mipi_raw_info_ptr;
}

LOCAL uint32_t Sensor_ov8825_InitRawTuneInfo(void)
{
	uint32_t rtn=0x00;
	struct sensor_raw_info* raw_sensor_ptr = Sensor_ov8825_GetContext();
	struct sensor_raw_tune_info* sensor_ptr = NULL;
	struct sensor_raw_cali_info* cali_ptr = NULL;
	struct sensor_raw_fix_info* fix_ptr=NULL;

	uint32_t edge_offset = 5;
	uint32_t pref_y = 6;

	if (raw_sensor_ptr == NULL) {
		SENSOR_PRINT("raw_sensor_ptr is NULL");
		return rtn;
	}

	sensor_ptr=raw_sensor_ptr->tune_ptr;
	cali_ptr=raw_sensor_ptr->cali_ptr;
	fix_ptr=raw_sensor_ptr->fix_ptr;
	sensor_ptr->af.video_max_tune_step = 40;
	sensor_ptr->af.video_speed_ratio = 50;
	sensor_ptr->lnc_bypass=0x01;
	sensor_ptr->flash.lum_ratio = 1093;

	fix_ptr->ae_tab.ae[1][1].index.reserved[0] =0;
	//s_ov8825_ae_tab.ae[0][0].index.reserved[0] = 0;

	sensor_ptr->ev_cali.mode = 1;
	sensor_ptr->ev_cali.f_num = 22;

	sensor_ptr->ev_cali.single.lv_exp = 400000;
	sensor_ptr->ev_cali.single.lv_gain = 196;
	sensor_ptr->ev_cali.single.lv = 8;
	sensor_ptr->ev_cali.single.lux_exp = 400000;
	sensor_ptr->ev_cali.single.lux_gain = 196;
	sensor_ptr->ev_cali.single.lux = 670;

	sensor_ptr->ev_cali.mult.num = 7;
	sensor_ptr->ev_cali.mult.tab[0].exp = 1000000;
	sensor_ptr->ev_cali.mult.tab[0].gain = 1216;
	sensor_ptr->ev_cali.mult.tab[0].lv = 4;
	sensor_ptr->ev_cali.mult.tab[0].lux = 48;

	sensor_ptr->ev_cali.mult.tab[1].exp = 1000000;
	sensor_ptr->ev_cali.mult.tab[1].gain = 656;
	sensor_ptr->ev_cali.mult.tab[1].lv = 5;
	sensor_ptr->ev_cali.mult.tab[1].lux = 87;

	sensor_ptr->ev_cali.mult.tab[2].exp = 1000000;
	sensor_ptr->ev_cali.mult.tab[2].gain = 328;
	sensor_ptr->ev_cali.mult.tab[2].lv = 6;
	sensor_ptr->ev_cali.mult.tab[2].lux = 169;

	sensor_ptr->ev_cali.mult.tab[3].exp = 800000;
	sensor_ptr->ev_cali.mult.tab[3].gain = 196;
	sensor_ptr->ev_cali.mult.tab[3].lv = 7;
	sensor_ptr->ev_cali.mult.tab[3].lux = 340;

	sensor_ptr->ev_cali.mult.tab[4].exp = 400000;
	sensor_ptr->ev_cali.mult.tab[4].gain = 196;
	sensor_ptr->ev_cali.mult.tab[4].lv = 8;
	sensor_ptr->ev_cali.mult.tab[4].lux = 670;

	sensor_ptr->ev_cali.mult.tab[5].exp = 200000;
	sensor_ptr->ev_cali.mult.tab[5].gain = 196;
	sensor_ptr->ev_cali.mult.tab[5].lv = 9;
	sensor_ptr->ev_cali.mult.tab[5].lux = 1350;

	sensor_ptr->ev_cali.mult.tab[6].exp = 100000;
	sensor_ptr->ev_cali.mult.tab[6].gain = 196;
	sensor_ptr->ev_cali.mult.tab[6].lv = 10;
	sensor_ptr->ev_cali.mult.tab[6].lux = 2700;

	sensor_ptr->ev_cali.mult.tab[7].exp = 77279;
	sensor_ptr->ev_cali.mult.tab[7].gain = 128;
	sensor_ptr->ev_cali.mult.tab[7].lv = 11;
	sensor_ptr->ev_cali.mult.tab[7].lux = 5400;

	sensor_ptr->ev_cali.mult.tab[8].exp = 38489;
	sensor_ptr->ev_cali.mult.tab[8].gain = 128;
	sensor_ptr->ev_cali.mult.tab[8].lv = 12;
	sensor_ptr->ev_cali.mult.tab[8].lux = 10800;

	sensor_ptr->ev_cali.mult.tab[9].exp = 19157;
	sensor_ptr->ev_cali.mult.tab[9].gain = 128;
	sensor_ptr->ev_cali.mult.tab[9].lv = 13;
	sensor_ptr->ev_cali.mult.tab[9].lux = 21600;

	sensor_ptr->ev_cali.mult.tab[10].exp = 9543;
	sensor_ptr->ev_cali.mult.tab[10].gain = 128;
	sensor_ptr->ev_cali.mult.tab[10].lv = 14;
	sensor_ptr->ev_cali.mult.tab[10].lux = 53200;

#if 0
	sensor_ptr->awb.alg_id = 0;
	//sensor_ptr->smart_light.enable = 0;

	/*param from jiawei*/
	sensor_ptr->af.min_step=200;
	sensor_ptr->af.max_step=1000;
	sensor_ptr->af.stab_period=20;

	sensor_ptr->pref.write_back = 1;

	sensor_ptr->ae.gamma_start = 0;
	sensor_ptr->ae.gamma_num = 1;
	sensor_ptr->ae.gamma_zone = 5;
	sensor_ptr->ae.gamma_thr[0] = 96;
	sensor_ptr->ae.gamma_lum_thr = 60;

	sensor_ptr->edge.info[0].detail_thr= 0;
	sensor_ptr->edge.info[0].smooth_thr= 0;
	sensor_ptr->edge.info[0].strength= 12 + edge_offset;

	sensor_ptr->edge.info[1].detail_thr= 0;
	sensor_ptr->edge.info[1].smooth_thr= 0;
	sensor_ptr->edge.info[1].strength= 14 + edge_offset;

	sensor_ptr->edge.info[2].detail_thr= 0;
	sensor_ptr->edge.info[2].smooth_thr= 0;
	sensor_ptr->edge.info[2].strength= 16 + edge_offset;

	sensor_ptr->edge.info[3].detail_thr= 0;
	sensor_ptr->edge.info[3].smooth_thr= 0;
	sensor_ptr->edge.info[3].strength= 17 + edge_offset;

	sensor_ptr->edge.info[4].detail_thr= 0;
	sensor_ptr->edge.info[4].smooth_thr= 0;
	sensor_ptr->edge.info[4].strength= 18 + edge_offset;

	sensor_ptr->edge.info[5].detail_thr= 0;
	sensor_ptr->edge.info[5].smooth_thr= 0;
	sensor_ptr->edge.info[5].strength= 23 + edge_offset;

	sensor_ptr->ae.normal_fix_fps = 0;
	sensor_ptr->ae.smart_edge_min_index = 0;
	sensor_ptr->ae.smart_edge_max_index = 5;

	sensor_ptr->ae.smart_pref_y_min = pref_y;
	sensor_ptr->ae.smart_pref_y_max = 0x20;
	sensor_ptr->ae.smart_pref_uv_min = 0x10;
	sensor_ptr->ae.smart_pref_uv_max = 0x30;

	sensor_ptr->ae.smart_denoise_diswei_outdoor_index = 1;
	sensor_ptr->ae.smart_denoise_diswei_min_index = 2;
	sensor_ptr->ae.smart_denoise_diswei_mid_index = 6;
	sensor_ptr->ae.smart_denoise_diswei_max_index = 40;

	sensor_ptr->ae.smart_denoise_ranwei_outdoor_index = 6;
	sensor_ptr->ae.smart_denoise_ranwei_min_index = 10;
	sensor_ptr->ae.smart_denoise_ranwei_mid_index = 20;
	sensor_ptr->ae.smart_denoise_ranwei_max_index = 60;

	//for smart function
	sensor_ptr->ae.denoise_start_index = 91;
	sensor_ptr->ae.denoise_start_zone = 5;
	sensor_ptr->ae.denoise_lum_thr = 50;
	sensor_ptr->ae.smart_base_gain = 0x30;

	sensor_ptr->ae.smart_denoise_soft_y_outdoor_index = 4;
	sensor_ptr->ae.smart_denoise_soft_y_min_index = 4;
	sensor_ptr->ae.smart_denoise_soft_y_mid_index = 5;
	sensor_ptr->ae.smart_denoise_soft_y_max_index = 7;

	sensor_ptr->ae.smart_denoise_soft_uv_outdoor_index = 0;
	sensor_ptr->ae.smart_denoise_soft_uv_min_index = 0;
	sensor_ptr->ae.smart_denoise_soft_uv_mid_index = 0;
	sensor_ptr->ae.smart_denoise_soft_uv_max_index = 0;

	sensor_ptr->ae.smart_sta_start_index= 0;
	sensor_ptr->ae.smart_sta_low_thr = 0;
	sensor_ptr->ae.smart_sta_ratio1= 0;
	sensor_ptr->ae.smart_sta_ratio= 0;

	sensor_ptr->saturation.factor[3] = 60;
#endif

#if 0
	sensor_ptr->auto_adjust.bil_denoise.enable=0x00;
	sensor_ptr->auto_adjust.y_denoise.enable=0x00;
	sensor_ptr->auto_adjust.uv_denoise.enable=0x00;
	sensor_ptr->auto_adjust.cmc.enable=0x00;
	sensor_ptr->auto_adjust.gamma.enable=0x00;
	sensor_ptr->auto_adjust.edge.enable=0x00;

	sensor_ptr->auto_adjust.gamma.start_level=0x00;;
	sensor_ptr->auto_adjust.gamma.dependon_index=0x00;
	sensor_ptr->auto_adjust.gamma.dependon_gain=0x00;
	sensor_ptr->auto_adjust.gamma.dependon_lum=0x00;

	sensor_ptr->auto_adjust.gamma.level_mode=0x00;
	sensor_ptr->auto_adjust.gamma.index_zone=0x00;
	sensor_ptr->auto_adjust.gamma.lum_zone=0x00;
	sensor_ptr->auto_adjust.gamma.target_lum_thr=0x00;

	sensor_ptr->auto_adjust.gamma.index_thr_num=0x00;
	sensor_ptr->auto_adjust.gamma.lum_low_thr_num=0x00;
	sensor_ptr->auto_adjust.gamma.index_thr[0]=0x00;
	sensor_ptr->auto_adjust.gamma.index_thr[1]=0x00;
	sensor_ptr->auto_adjust.gamma.index_thr[2]=0x00;
	sensor_ptr->auto_adjust.gamma.index_thr[3]=0x00;
	sensor_ptr->auto_adjust.gamma.index_thr[4]=0x00;
	sensor_ptr->auto_adjust.gamma.index_thr[5]=0x00;
	sensor_ptr->auto_adjust.gamma.index_thr[6]=0x00;
	sensor_ptr->auto_adjust.gamma.index_thr[7]=0x00;
	sensor_ptr->auto_adjust.gamma.index_thr[8]=0x00;
	sensor_ptr->auto_adjust.gamma.index_thr[9]=0x00;

	sensor_ptr->auto_adjust.gamma.index_start_level[0]=0x00;
	sensor_ptr->auto_adjust.gamma.index_start_level[1]=0x00;
	sensor_ptr->auto_adjust.gamma.index_start_level[2]=0x00;
	sensor_ptr->auto_adjust.gamma.index_start_level[3]=0x00;
	sensor_ptr->auto_adjust.gamma.index_start_level[4]=0x00;
	sensor_ptr->auto_adjust.gamma.index_start_level[5]=0x00;
	sensor_ptr->auto_adjust.gamma.index_start_level[6]=0x00;
	sensor_ptr->auto_adjust.gamma.index_start_level[7]=0x00;
	sensor_ptr->auto_adjust.gamma.index_start_level[8]=0x00;
	sensor_ptr->auto_adjust.gamma.index_start_level[9]=0x00;
	sensor_ptr->auto_adjust.gamma.index_end_level[0]=0x00;
	sensor_ptr->auto_adjust.gamma.index_end_level[1]=0x00;
	sensor_ptr->auto_adjust.gamma.index_end_level[2]=0x00;
	sensor_ptr->auto_adjust.gamma.index_end_level[3]=0x00;
	sensor_ptr->auto_adjust.gamma.index_end_level[4]=0x00;
	sensor_ptr->auto_adjust.gamma.index_end_level[5]=0x00;
	sensor_ptr->auto_adjust.gamma.index_end_level[6]=0x00;
	sensor_ptr->auto_adjust.gamma.index_end_level[7]=0x00;
	sensor_ptr->auto_adjust.gamma.index_end_level[8]=0x00;
	sensor_ptr->auto_adjust.gamma.index_end_level[9]=0x00;

	sensor_ptr->auto_adjust.gamma.bais_gain[0]=0x00;
	sensor_ptr->auto_adjust.gamma.bais_gain[1]=0x00;
	sensor_ptr->auto_adjust.gamma.bais_gain[2]=0x00;
	sensor_ptr->auto_adjust.gamma.bais_gain[3]=0x00;
	sensor_ptr->auto_adjust.gamma.bais_gain[4]=0x00;
	sensor_ptr->auto_adjust.gamma.bais_gain[5]=0x00;
	sensor_ptr->auto_adjust.gamma.bais_gain[6]=0x00;
	sensor_ptr->auto_adjust.gamma.bais_gain[7]=0x00;
	sensor_ptr->auto_adjust.gamma.bais_gain[8]=0x00;
	sensor_ptr->auto_adjust.gamma.bais_gain[9]=0x00;

	sensor_ptr->auto_adjust.gamma.lum_low_thr[0]=0x00;
	sensor_ptr->auto_adjust.gamma.lum_low_thr[1]=0x00;
	sensor_ptr->auto_adjust.gamma.lum_low_thr[2]=0x00;
	sensor_ptr->auto_adjust.gamma.lum_low_thr[3]=0x00;

	sensor_ptr->auto_adjust.gamma.lum_start_level[0]=0x00;
	sensor_ptr->auto_adjust.gamma.lum_start_level[1]=0x00;
	sensor_ptr->auto_adjust.gamma.lum_start_level[2]=0x00;
	sensor_ptr->auto_adjust.gamma.lum_start_level[3]=0x00;

	sensor_ptr->auto_adjust.gamma.lum_end_level[0]=0x00;
	sensor_ptr->auto_adjust.gamma.lum_end_level[4]=0x00;
	sensor_ptr->auto_adjust.gamma.lum_end_level[4]=0x00;
	sensor_ptr->auto_adjust.gamma.lum_end_level[4]=0x00;
#endif

#if 0
	raw_sensor_ptr->version_info->version_id=0x00010000;
	raw_sensor_ptr->version_info->srtuct_size=sizeof(struct sensor_raw_info);

	//bypass
	sensor_ptr->version_id=0x00010000;
	sensor_ptr->blc_bypass=0x00;
	sensor_ptr->nlc_bypass=0x01;
	sensor_ptr->lnc_bypass=0x01;
	sensor_ptr->ae_bypass=0x00;
	sensor_ptr->awb_bypass=0x00;
	sensor_ptr->bpc_bypass=0x01;
	sensor_ptr->denoise_bypass=0x01;
	sensor_ptr->grgb_bypass=0x01;
	sensor_ptr->cmc_bypass=0x00;
	sensor_ptr->gamma_bypass=0x00;
	sensor_ptr->uvdiv_bypass=0x01;
	sensor_ptr->pref_bypass=0x01;
	sensor_ptr->bright_bypass=0x00;
	sensor_ptr->contrast_bypass=0x00;
	sensor_ptr->hist_bypass=0x01;
	sensor_ptr->auto_contrast_bypass=0x01;
	sensor_ptr->af_bypass=0x00;
	sensor_ptr->edge_bypass=0x00;
	sensor_ptr->fcs_bypass=0x00;
	sensor_ptr->css_bypass=0x00;
	sensor_ptr->saturation_bypass=0x00;
	sensor_ptr->hdr_bypass=0x01;
	sensor_ptr->glb_gain_bypass=0x01;
	sensor_ptr->chn_gain_bypass=0x01;

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

	//ov8825 awb param
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

	return rtn;
}


LOCAL unsigned long _ov8825_GetResolutionTrimTab(unsigned long param)
{
	SENSOR_PRINT("0x%lx",  (unsigned long)s_ov8825_Resolution_Trim_Tab);
	return (unsigned long) s_ov8825_Resolution_Trim_Tab;
}
LOCAL unsigned long _ov8825_PowerOn(unsigned long power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_ov8825_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_ov8825_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_ov8825_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_ov8825_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_ov8825_mipi_raw_info.reset_pulse_level;
	//uint32_t reset_width=g_ov8825_yuv_info.reset_pulse_width;

	if (SENSOR_TRUE == power_on) {
		//set all power pin to disable status
		//Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		//Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
		//Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_SetResetLevel(reset_level);
		Sensor_PowerDown(power_down);
		usleep(20*1000);
		//step 0 power up DOVDD, the AVDD
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		Sensor_SetIovddVoltage(iovdd_val);
		//usleep(2000);
		Sensor_SetAvddVoltage(avdd_val);
		//usleep(6000);
		//step 1 power up DVDD
		Sensor_SetDvddVoltage(dvdd_val);
		//usleep(6000);
		//step 2 power down pin high
		Sensor_PowerDown(!power_down);
		//usleep(2000);
		//step 3 reset pin high
		Sensor_SetResetLevel(!reset_level);
		//usleep(22*1000);
		//step 4 xvclk
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(4*1000);
		//is_stream_on = 1;
	} else {
		//power off should start > 1ms after last SCCB
		usleep(4*1000);
		//step 1 reset and PWDN
		Sensor_SetResetLevel(reset_level);
		usleep(2000);
		Sensor_PowerDown(power_down);
		usleep(2000);
		//step 2 dvdd
		Sensor_SetDvddVoltage(SENSOR_AVDD_CLOSED);
		usleep(2000);
		//step 4 xvclk
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		usleep(5000);
		//step 5 AVDD IOVDD
		Sensor_SetAvddVoltage(SENSOR_AVDD_CLOSED);
		usleep(2000);
		Sensor_SetIovddVoltage(SENSOR_AVDD_CLOSED);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
		//is_stream_on = 0;
	}
	SENSOR_PRINT("SENSOR_ov8825: _ov8825_Power_On(1:on, 0:off): %ld", power_on);
	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov8825_cfg_otp(unsigned long param)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_ov8825_raw_param_tab;
	uint32_t module_id=g_ov8825_module_id;

	SENSOR_PRINT("SENSOR_OV8825: _ov8825_cfg_otp module_id:0x%x", module_id);
	Sensor_WriteReg(0x0103, 0x01);
	usleep(10 * 1000);

	if (PNULL!=tab_ptr[module_id].cfg_otp) {
		tab_ptr[module_id].cfg_otp(0);
	}
	/* do streamoff, and not sleep
	_ov8825_StreamOff(0);
	*/
	Sensor_WriteReg(0x0100, 0x00);

	SENSOR_PRINT("SENSOR_OV8825: _ov8825_cfg_otp end");

	return rtn;
}

LOCAL uint32_t _ov8825_read_otp_gain(uint32_t param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint32_t gain = 0;

	value = Sensor_ReadReg(0x350b);/*0-7*/
	gain = value&0xff;
	value = Sensor_ReadReg(0x350a);/*8*/
	gain |= (value<<0x08)&0x300;

	*((uint32_t*)param) = (int)gain;

	SENSOR_PRINT("SENSOR_ov8825: _ov8825_ReadGain gain: 0x%x", gain);

	return rtn;
}

LOCAL uint32_t _ov8825_access_val(unsigned long param)
{
	uint32_t rtn=SENSOR_SUCCESS;
	SENSOR_VAL_T* param_ptr = (SENSOR_VAL_T*)param;

	uint16_t tmp;

	SENSOR_PRINT("SENSOR_ov8825: _ov8825_access_val E");
	if(!param_ptr){
		_ov8825_cfg_otp(param);
		return rtn;
	}

	SENSOR_PRINT("SENSOR_ov8825: param_ptr->type=%x",param_ptr->type);
	switch(param_ptr->type)
	{
		case SENSOR_VAL_TYPE_SHUTTER:
			*((uint32_t*)param_ptr->pval) = _ov8825_get_shutter();
			break;
		case SENSOR_VAL_TYPE_READ_OTP_GAIN:
			rtn = _ov8825_read_otp_gain(param_ptr->pval);
		default:
			break;
	}

	SENSOR_PRINT("SENSOR_ov8825: _ov8825_access_val X");

	return rtn;

}


LOCAL uint32_t _ov8825_GetRawInof(void)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_ov8825_raw_param_tab;
	uint32_t i=0x00;
	uint16_t stream_value = 0;

	stream_value = Sensor_ReadReg(0x0100);
	if (1 != (stream_value & 0x01)) {
		Sensor_WriteReg(0x0100, 0x01);
		usleep(5 * 1000);
	}

	for(i=0x00; ; i++) {
		g_ov8825_module_id = i;
		if(RAW_INFO_END_ID==tab_ptr[i].param_id) {
			if(NULL==s_ov8825_mipi_raw_info_ptr) {
				SENSOR_PRINT("SENSOR_OV8825: ov8825_GetRawInof no param error");
				rtn=SENSOR_FAIL;
			}
			SENSOR_PRINT("SENSOR_OV8825: ov8825_GetRawInof end");
			break;
		} else if (PNULL!=tab_ptr[i].identify_otp) {
			if(SENSOR_SUCCESS==tab_ptr[i].identify_otp(0)) {
				s_ov8825_mipi_raw_info_ptr = tab_ptr[i].info_ptr;
				SENSOR_PRINT("SENSOR_OV8825: ov8825_GetRawInof id:0x%x success", g_ov8825_module_id);
				break;
			}
		}
	}

	if(1 != (stream_value & 0x01)) {
		Sensor_WriteReg(0x0100, stream_value);
		usleep(5 * 1000);
	}

	return rtn;
}

LOCAL unsigned long _ov8825_GetMaxFrameLine(unsigned long index)
{
	uint32_t max_line=0x00;
	SENSOR_TRIM_T_PTR trim_ptr=s_ov8825_Resolution_Trim_Tab;

	max_line=trim_ptr[index].frame_line;

	return max_line;
}

LOCAL unsigned long _ov8825_Identify(unsigned long param)
{
#define ov8825_PID_VALUE    0x88
#define ov8825_PID_ADDR     0x300A
#define ov8825_VER_VALUE    0x25
#define ov8825_VER_ADDR     0x300B

	uint8_t pid_value = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT_HIGH("SENSOR_ov8825: mipi raw identify\n");

	pid_value = Sensor_ReadReg(ov8825_PID_ADDR);
	if (ov8825_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(ov8825_VER_ADDR);
		SENSOR_PRINT("SENSOR_ov8825: Identify: PID = %x, VER = %x", pid_value, ver_value);
		if (ov8825_VER_VALUE == ver_value) {
			SENSOR_PRINT_HIGH("SENSOR_ov8825: this is ov8825 sensor !");
			ret_value=_ov8825_GetRawInof();
			if(SENSOR_SUCCESS != ret_value)
			{
				SENSOR_PRINT_ERR("SENSOR_ov8825: the module is unknow error !");
			}
			Sensor_ov8825_InitRawTuneInfo();
		} else {
			SENSOR_PRINT_HIGH("SENSOR_ov8825: Identify this is OV%x%x sensor !", pid_value, ver_value);
		}
	} else {
		SENSOR_PRINT_ERR("SENSOR_ov8825: identify fail,pid_value=%d", pid_value);
	}

	return ret_value;
}

LOCAL unsigned long _ov8825_write_exposure(unsigned long param)
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

	expsure_line=param&0xffff;
	dummy_line=(param>>0x10)&0x0fff;
	size_index=(param>>0x1c)&0x0f;

	SENSOR_PRINT("SENSOR_ov8825: write_exposure line:%d, dummy:%d, size_index:%d", expsure_line, dummy_line, size_index);

	max_frame_len=_ov8825_GetMaxFrameLine(size_index);

	if(0x00!=max_frame_len)
	{
		frame_len = ((expsure_line+dummy_line+4)> max_frame_len) ? (expsure_line+dummy_line+4) : max_frame_len;
		frame_len = (frame_len+1)>>1<<1;

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

LOCAL unsigned long _ov8825_gain_change(uint32_t gain)
{
	uint32_t ae_gain = gain;
	uint32_t calc_gain = 0;


	do {
		if (256 <= ae_gain) {
			calc_gain <<= 1;
			calc_gain |= 0x10;
		} else {
			ae_gain = ae_gain/8 -16;
			calc_gain |= (ae_gain&0x0f);
			break;
		}
		ae_gain /= 2;
	} while(1);
	ae_gain = calc_gain;

	return ae_gain;
}

LOCAL unsigned long _ov8825_write_gain(unsigned long param)
#if 0
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint16_t real_gain=0x00;

	CMR_LOGI("SENSOR_ov8825: write_gain:0x%x", param);

	real_gain = _ov8825_gain_change(param);

	SENSOR_PRINT("AE_TAB:-s-gain:%d, again:0x%x", param, real_gain);

	value = real_gain&0xff;
	ret_value = Sensor_WriteReg(0x350b, value);/*0-7*/
	value = (real_gain>>0x08)&0x03;
	ret_value = Sensor_WriteReg(0x350a, value);/*8*/

	return ret_value;
}
#else
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t value=0x00;

	CMR_LOGI("SENSOR_ov8825: write_gain:0x%x", param);

	value = param&0xff;
	ret_value = Sensor_WriteReg(0x350b, value);/*0-7*/
	value = (param>>0x08)&0x03;
	ret_value = Sensor_WriteReg(0x350a, value);/*8*/

	return ret_value;
}
#endif
LOCAL unsigned long _ov8825_write_af(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint16_t reg_val = 0x0;

	SENSOR_PRINT("SENSOR_ov8825: _write_af 0x%x", param);

	value = (param&0xf)<<0x04;
	value = value + 8 + (g_af_slewrate&0x7);
	ret_value = Sensor_WriteReg(0x3618, value);
	value = (param&0x3f0)>>0x04;
	ret_value = Sensor_WriteReg(0x3619, value);

	return ret_value;
}

LOCAL unsigned long _ov8825_BeforeSnapshot(unsigned long param)
{
	uint8_t ret_l, ret_m, ret_h;
	uint32_t capture_exposure, preview_maxline;
	uint32_t capture_maxline, preview_exposure;
	uint32_t capture_mode = param & 0xffff;
	uint32_t preview_mode = (param >> 0x10 ) & 0xffff;
	uint32_t prv_linetime=s_ov8825_Resolution_Trim_Tab[preview_mode].line_time;
	uint32_t cap_linetime = s_ov8825_Resolution_Trim_Tab[capture_mode].line_time;

	SENSOR_PRINT("SENSOR_ov8825: BeforeSnapshot mode: 0x%08x",param);

	if (preview_mode == capture_mode) {
		SENSOR_PRINT("SENSOR_ov8825: prv mode equal to capmode");
		goto CFG_INFO;
	}

	ret_h = (uint8_t) Sensor_ReadReg(0x3500);
	ret_m = (uint8_t) Sensor_ReadReg(0x3501);
	ret_l = (uint8_t) Sensor_ReadReg(0x3502);
	preview_exposure = (ret_h << 12) + (ret_m << 4) + (ret_l >> 4);
	s_preview_shutter = preview_exposure;
	ret_h = (uint8_t) Sensor_ReadReg(0x380e);
	ret_l = (uint8_t) Sensor_ReadReg(0x380f);
	preview_maxline = (ret_h << 8) + ret_l;

	Sensor_SetMode(capture_mode);
	Sensor_SetMode_WaitDone();

	ret_h = (uint8_t) Sensor_ReadReg(0x380e);
	ret_l = (uint8_t) Sensor_ReadReg(0x380f);
	capture_maxline = (ret_h << 8) + ret_l;
	SENSOR_PRINT("SENSOR_ov8825: CAP max line %d",capture_maxline);

	if (preview_maxline == capture_maxline) {
		SENSOR_PRINT("SENSOR_ov8825: preview_maxline equal to  capture_maxline");
		goto CFG_INFO;
	}
	capture_exposure = preview_exposure * prv_linetime/cap_linetime;
        capture_exposure = capture_exposure*2;
	CMR_LOGE("capture_exposure %d",capture_exposure);

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
	s_capture_shutter = _ov8825_get_shutter();
	s_capture_VTS = _ov8825_get_VTS();
	_ov8825_ReadGain(capture_mode);
	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, s_capture_shutter);

	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov8825_after_snapshot(unsigned long param)
{
	uint8_t ret_l, ret_m, ret_h;
	SENSOR_PRINT("SENSOR_ov8825: after_snapshot mode:%ld", param);

	if (s_preview_shutter) {
		ret_l = ((unsigned char)s_preview_shutter&0xf) << 4;
		ret_m = (unsigned char)((s_preview_shutter&0xfff) >> 4) & 0xff;
		ret_h = (unsigned char)(s_preview_shutter >> 12);

		Sensor_WriteReg(0x3502, ret_l);
		Sensor_WriteReg(0x3501, ret_m);
		Sensor_WriteReg(0x3500, ret_h);
		s_preview_shutter = 0;
	}

	Sensor_SetMode((uint32_t)param);
	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov8825_flash(unsigned long param)
{
	SENSOR_PRINT("SENSOR_ov8825: param=%ld", param);

	/* enable flash, disable in _ov8825_BeforeSnapshot */
	g_flash_mode_en = (uint32_t)param;
	Sensor_SetFlash(param);
	SENSOR_PRINT_HIGH("end");
	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov8825_StreamOn(unsigned long param)
{
	SENSOR_PRINT("SENSOR_ov8825: StreamOn");

	//Sensor_WriteReg(0x0100, 0x01);
	Sensor_WriteReg(0x301c, 0xf0);
	Sensor_WriteReg(0x301a, 0x70);
	is_stream_on = 1;
	return 0;
}

LOCAL unsigned long _ov8825_StreamOff(unsigned long param)
{
	SENSOR_PRINT("SENSOR_ov8825: StreamOff");
	if (is_stream_on) {
	//Sensor_WriteReg(0x0100, 0x00);
	Sensor_WriteReg(0x301a, 0x71);
	Sensor_WriteReg(0x301c, 0xf4);
	usleep(100*1000);
	is_stream_on = 0;
	}

	return 0;
}

int _ov8825_get_shutter(void)
{
	// read shutter, in number of line period
	int shutter;

	shutter = (Sensor_ReadReg(0x03500) & 0x0f);
	shutter = (shutter<<8) + Sensor_ReadReg(0x3501);
	shutter = (shutter<<4) + (Sensor_ReadReg(0x3502)>>4);

	return shutter;
}

int _ov8825_set_shutter(int shutter)
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

int _ov8825_get_gain16(void)
{
	// read gain, 16 = 1x
	int gain16;

	gain16 = Sensor_ReadReg(0x350a) & 0x03;
	gain16 = (gain16<<8) + Sensor_ReadReg(0x350b);

	return gain16;
}

int _ov8825_set_gain16(int gain16)
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

	SENSOR_PRINT_HIGH("capture_gain16 %d,capture_shutter %d", capture_gain16, capture_shutter);
	// write capture gain
	_ov8825_set_gain16(capture_gain16);

	// write capture shutter
	/*if (capture_shutter > (capture_VTS - 4)) {
		capture_VTS = capture_shutter + 4;
		OV5640_set_VTS(capture_VTS);
	}*/
	_ov8825_set_shutter(capture_shutter);
}

static uint32_t _ov8825_SetEV(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	uint16_t value=0x00;
	uint32_t gain = s_ov8825_gain;
	uint32_t ev = ext_ptr->param;

	SENSOR_PRINT("SENSOR_ov8825: _ov8825_SetEV param: 0x%x", ext_ptr->param);

	switch(ev) {
	case SENSOR_HDR_EV_LEVE_0:
		_calculate_hdr_exposure(s_ov8825_gain/2,s_capture_VTS,s_capture_shutter/2);
		break;
	case SENSOR_HDR_EV_LEVE_1:
		_calculate_hdr_exposure(s_ov8825_gain,s_capture_VTS,s_capture_shutter);
		break;
	case SENSOR_HDR_EV_LEVE_2:
		_calculate_hdr_exposure(s_ov8825_gain*2,s_capture_VTS,s_capture_shutter*4);
		break;
	default:
		break;
	}
	usleep(100*1000);
	return rtn;
}

LOCAL unsigned long _ov8825_saveLoad_exposure(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	uint8_t  ret_h, ret_m, ret_l;
	uint32_t dummy = 0;
	SENSOR_EXT_FUN_PARAM_T_PTR sl_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR)param;

	uint32_t sl_param = sl_ptr->param;
	if (sl_param) {
//		usleep(180*1000);     /*wait for effect after init stable(AWB)*/
		/*load exposure params to sensor*/
		SENSOR_PRINT_HIGH("_ov8825_saveLoad_exposure load shutter 0x%x gain 0x%x",
					s_ov8825_shutter_bak,
					s_ov8825_gain_bak);
		_ov8825_set_gain16(s_ov8825_gain_bak);
		ret_l = ((unsigned char)s_ov8825_shutter_bak&0xf) << 4;
		ret_m = (unsigned char)((s_ov8825_shutter_bak&0xfff) >> 4) & 0xff;
		ret_h = (unsigned char)(s_ov8825_shutter_bak >> 12);
		Sensor_WriteReg(0x3502, ret_l);
		Sensor_WriteReg(0x3501, ret_m);
		Sensor_WriteReg(0x3500, ret_h);
	} else {
		/*ave exposure params from sensor*/
		ret_h = (uint8_t) Sensor_ReadReg(0x3500);
		ret_m = (uint8_t) Sensor_ReadReg(0x3501);
		ret_l = (uint8_t) Sensor_ReadReg(0x3502);
		s_ov8825_shutter_bak = (ret_h << 12) + (ret_m << 4) + (ret_l >> 4);
		s_ov8825_gain_bak = _ov8825_ReadGain(dummy);
		SENSOR_PRINT_HIGH("_ov8825_saveLoad_exposure save shutter 0x%x gain 0x%x",
					s_ov8825_shutter_bak,
					s_ov8825_gain_bak);
	}
	return rtn;
}

LOCAL unsigned long _ov8825_ExtFunc(unsigned long ctl_param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr =
	    (SENSOR_EXT_FUN_PARAM_T_PTR) ctl_param;
	SENSOR_PRINT_HIGH("0x%x", ext_ptr->cmd);

	switch (ext_ptr->cmd) {
	case SENSOR_EXT_FUNC_INIT:
		break;
	case SENSOR_EXT_FOCUS_START:
		break;
	case SENSOR_EXT_EXPOSURE_START:
		break;
	case SENSOR_EXT_EV:
		rtn = _ov8825_SetEV(ctl_param);
		break;

	case SENSOR_EXT_EXPOSURE_SL:
		rtn = _ov8825_saveLoad_exposure(ctl_param);
		break;

	default:
		break;
	}
	return rtn;
}
LOCAL int _ov8825_get_VTS(void)
{
	// read VTS from register settings
	int VTS;

	VTS = Sensor_ReadReg(0x380e);//total vertical size[15:8] high byte

	VTS = (VTS<<8) + Sensor_ReadReg(0x380f);

	return VTS;
}

LOCAL int _ov8825_set_VTS(int VTS)
{
	// write VTS to registers
	int temp;

	temp = VTS & 0xff;
	Sensor_WriteReg(0x380f, temp);

	temp = VTS>>8;
	Sensor_WriteReg(0x380e, temp);

	return 0;
}
LOCAL unsigned long _ov8825_ReadGain(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint32_t gain = 0;

	value = Sensor_ReadReg(0x350b);/*0-7*/
	gain = value&0xff;
	value = Sensor_ReadReg(0x350a);/*8*/
	gain |= (value<<0x08)&0x300;

	s_ov8825_gain=(int)gain;

	SENSOR_PRINT("SENSOR_ov8825: _ov8825_ReadGain gain: 0x%x", s_ov8825_gain);

	return rtn;
}
