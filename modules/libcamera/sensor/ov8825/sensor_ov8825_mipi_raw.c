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
#include "sensor_ov8825_raw_param_v3.c"
#else
#include "sensor_ov8825_raw_param_v2.c"
#endif
#include "sensor_ov8825_otp_truly_02.c"

#define ov8825_I2C_ADDR_W        0x36
#define ov8825_I2C_ADDR_R         0x36

#define OV8825_MIN_FRAME_LEN_PRV  0x5e8

#define OV8825_4_LANES
static int s_ov8825_gain = 0;
static int s_ov8825_gain_bak = 0;
static int s_ov8825_shutter_bak = 0;
static int s_capture_shutter = 0;
static int s_capture_VTS = 0;
static int s_exposure_time = 0;
static int s_video_min_framerate = 0;
static int s_video_max_framerate = 0;
static uint32_t is_stream_on = 1;

LOCAL unsigned long _ov8825_GetResolutionTrimTab(uint32_t param);
LOCAL uint32_t _ov8825_PowerOn(uint32_t power_on);
LOCAL uint32_t _ov8825_Identify(uint32_t param);
LOCAL uint32_t _ov8825_BeforeSnapshot(uint32_t param);
LOCAL uint32_t _ov8825_after_snapshot(uint32_t param);
LOCAL uint32_t _ov8825_StreamOn(uint32_t param);
LOCAL uint32_t _ov8825_StreamOff(uint32_t param);
LOCAL unsigned long _ov8825_write_exposure(unsigned long param);
LOCAL unsigned long _ov8825_ex_write_exposure(unsigned long param);
LOCAL uint32_t _ov8825_write_gain(uint32_t param);
LOCAL uint32_t _ov8825_write_af(uint32_t param);
LOCAL uint32_t _ov8825_flash(uint32_t param);
LOCAL unsigned long _ov8825_GetExifInfo(unsigned long param);
LOCAL uint32_t _ov8825_ExtFunc(unsigned long ctl_param);
LOCAL int _ov8825_get_VTS(void);
LOCAL int _ov8825_set_VTS(int VTS);
LOCAL uint32_t _ov8825_ReadGain(uint32_t param);
LOCAL uint32_t _ov8825_set_video_mode(uint32_t param);
LOCAL int _ov8825_get_shutter(void);
LOCAL uint32_t _ov8825_cfg_otp(uint32_t  param);
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
	{0x0103, 0x01}, // software reset
	// delay(5ms)
	{SENSOR_WRITE_DELAY, 0x0a},
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
	{0x3004, 0xd4}, // PLL_CTRL1
	{0x3005, 0x00}, // PLL_CTRL2
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
	{0x380e, 0x05}, // VTS = 1264
	{0x380f, 0xe8}, // VTS
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
	{0x506a, 0x00} // VSCALE_CTRL
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
	{0x380e, 0x0b}, // VTS = 2480
	{0x380f, 0xa8}, // VTS
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
//	{ADDR_AND_LEN_OF_ARRAY(ov8825_1920x1080_setting), 1920, 1080, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(ov8825_3264x2448_setting), 3264, 2448, 24, SENSOR_IMAGE_FORMAT_RAW},

	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

LOCAL SENSOR_TRIM_T s_ov8825_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
//	{0, 0, 1920, 1080, 219, 408, 1524, {0, 0, 1920, 1080}},
	{0, 0, 3264, 2448, 168, 528, 2480, {0, 0, 3264, 2448}},

	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
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
	{{{30, 30, 219, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_ov8825_1632x1224_video_tab},
	{{{15, 15, 168, 64}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_ov8825_3264x2448_video_tab},

	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}
};

LOCAL uint32_t _ov8825_set_video_mode(uint32_t param)
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

	SENSOR_PRINT("0x%02x", param);
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
	_ov8825_GetExifInfo,
	_ov8825_ExtFunc,
	PNULL, //_ov8825_set_anti_flicker,
	_ov8825_set_video_mode,
	PNULL, //pick_jpeg_stream
	PNULL,  //meter_mode
	PNULL, //get_status
	_ov8825_StreamOn,
	_ov8825_StreamOff,
	_ov8825_access_val,
	 _ov8825_ex_write_exposure,  // PNULL,
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
	1,			// skip frame num before preview
	0,			// skip frame num before capture
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
	3,			// skip frame num while change setting
	57,			// horizontal view angle
	57,			// vertical view angle
};

LOCAL struct sensor_raw_info* Sensor_ov8825_GetContext(void)
{
	return s_ov8825_mipi_raw_info_ptr;
}

#define param_update(x1,x2) sprintf(name,"/data/ov8825_%s.bin",x1);\
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
LOCAL uint32_t Sensor_ov8825_InitRawTuneInfo(void)
{
	uint32_t rtn=0x00;
	struct sensor_raw_info* raw_sensor_ptr=Sensor_ov8825_GetContext();
	struct isp_mode_param* mode_common_ptr = raw_sensor_ptr->mode_ptr[0].addr;
	int i;
	char name[100] = {'\0'};

	isp_raw_para_update_from_file(&g_ov8825_mipi_raw_info,0);

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

				static struct sensor_nlm_level nlm_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "noise/nlm_param.h"
				};

				param_update("nlm_param",nlm_param);

				static struct sensor_vst_level vst_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "noise/vst_param.h"
				};

				param_update("vst_param",vst_param);

				static struct sensor_ivst_level ivst_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "noise/ivst_param.h"
				};

				param_update("ivst_param",ivst_param);

				static struct sensor_flat_offset_level flat_offset_param[SENSOR_SMART_LEVEL_NUM] = {
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


LOCAL unsigned long _ov8825_GetResolutionTrimTab(uint32_t param)
{
	SENSOR_PRINT("0x%lx",  (unsigned long)s_ov8825_Resolution_Trim_Tab);
	return (unsigned long) s_ov8825_Resolution_Trim_Tab;
}
LOCAL uint32_t _ov8825_PowerOn(uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_ov8825_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_ov8825_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_ov8825_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_ov8825_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_ov8825_mipi_raw_info.reset_pulse_level;
	//uint32_t reset_width=g_ov8825_yuv_info.reset_pulse_width;

	if (SENSOR_TRUE == power_on) {
		//set all power pin to disable status
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_SetResetLevel(reset_level);
		Sensor_PowerDown(power_down);
		usleep(20*1000);
		//step 0 power up DOVDD, the AVDD
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		Sensor_SetIovddVoltage(iovdd_val);
		usleep(2000);
		Sensor_SetAvddVoltage(avdd_val);
		usleep(6000);
		//step 1 power up DVDD
		Sensor_SetDvddVoltage(dvdd_val);
		usleep(6000);
		//step 2 power down pin high
		Sensor_PowerDown(!power_down);
		usleep(2000);
		//step 3 reset pin high
		Sensor_SetResetLevel(!reset_level);
		usleep(22*1000);
		//step 4 xvclk
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(4*1000);
		is_stream_on = 1;
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
		is_stream_on = 0;
	}
	SENSOR_PRINT("SENSOR_ov8825: _ov8825_Power_On(1:on, 0:off): %d", power_on);
	return SENSOR_SUCCESS;
}

LOCAL uint32_t _ov8825_cfg_otp(uint32_t param)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_ov8825_raw_param_tab;
	uint32_t module_id=g_ov8825_module_id;

	SENSOR_PRINT("SENSOR_OV8825: _ov8825_cfg_otp module_id:0x%x", module_id);

	/*be called in sensor thread, so not call Sensor_SetMode_WaitDone()*/
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

static uint8_t otp[256];
static uint8_t vcm;
LOCAL uint32_t _ov8825_write_vcm(uint32_t *param)
{
	uint32_t ret_value  = SENSOR_SUCCESS;
	uint8_t  cmd_val[2] = {0x00};
	uint16_t slave_addr = 0;
	uint16_t cmd_len 	= 2;
	cmd_val[0] = ((*param)>>16) & 0xff;  //addr
	cmd_val[1] = (*param) & 0xff;        //data

	vcm = cmd_val[1];
	SENSOR_PRINT("SENSOR_ov8825: _write_vcm, ret =  %d, MSL:%x, LSL:%x\n", ret_value, cmd_val[0], cmd_val[1]);
	return ret_value;
}

LOCAL uint32_t _ov8825_read_vcm(uint32_t *param)
{
	uint32_t ret_value  = SENSOR_SUCCESS;
	uint8_t  cmd_val[2] = {0x00};
	uint16_t slave_addr = 0;
	uint16_t cmd_len = 1;

	cmd_val[0] = ((*param)>>16) & 0xff;  //addr
	*param = vcm;

	SENSOR_PRINT("SENSOR_ov8825: _read_vcm, ret =  %d, MSL:%x, LSL:%x\n", ret_value, cmd_val[0], cmd_val[1]);
	return ret_value;
}

LOCAL uint32_t _ov8825_write_otp(uint32_t *param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_OTP_PARAM_T* param_ptr = (SENSOR_OTP_PARAM_T*)param;
	uint32_t start_addr = param_ptr->start_addr;
	uint32_t len  		= param_ptr->len;
	uint8_t *buff 		= param_ptr->buff;
	SENSOR_PRINT("SENSOR_ov8825: _ov8825_write_otp E");
	memcpy(otp, buff, len);
	SENSOR_PRINT("SENSOR_ov8825: _ov8825_write_otp %x, %x",start_addr, len);
	return rtn;
}

LOCAL uint32_t _ov8825_read_otp(uint32_t *param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_OTP_PARAM_T* param_ptr = (SENSOR_OTP_PARAM_T*)param;
	uint32_t start_addr = param_ptr->start_addr;
	uint32_t len  		= param_ptr->len;
	uint8_t *buff 		= param_ptr->buff;
	SENSOR_PRINT("SENSOR_ov8825: _ov8825_read_otp E");
	memcpy(buff, otp, len);
	SENSOR_PRINT("SENSOR_ov8825: _ov8825_read_otp %x, %x",start_addr, len);
	return rtn;

}

LOCAL uint32_t _ov8825_read_af(uint32_t *param)
{
	uint32_t ret_value  = SENSOR_SUCCESS;
	uint16_t value		= 0x00;
	uint16_t reg_val 	= 0x0;

	SENSOR_PRINT("SENSOR_ov8825: _ov8825_read_af 0x%x", param);

	*param = ((Sensor_ReadReg(0x3619) & 0x3f) << 4) | (Sensor_ReadReg(0x3618) & 0x0f);

	return ret_value;
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
		case SENSOR_VAL_TYPE_READ_VCM:
			rtn = _ov8825_read_vcm(param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_WRITE_VCM:
			rtn = _ov8825_write_vcm(param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_READ_OTP:
			rtn = _ov8825_read_otp(param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_WRITE_OTP:
			rtn = _ov8825_write_otp(param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_GET_RELOADINFO:
#if 0
			{
				struct isp_calibration_info **p= (struct isp_calibration_info **)param_ptr->pval;
				*p=&calibration_info;
			}
#endif
			break;
		case SENSOR_VAL_TYPE_GET_AFPOSITION:
			rtn = _ov8825_read_af(param_ptr->pval);
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

	for(i=0x00; ; i++)
	{
		g_ov8825_module_id = i;
		if(RAW_INFO_END_ID==tab_ptr[i].param_id){
			if(NULL==s_ov8825_mipi_raw_info_ptr){
				s_ov8825_mipi_raw_info_ptr = &s_ov8825_mipi_raw_info;
				SENSOR_PRINT("SENSOR_OV8825: ov8825_GetRawInof no param error, set as default");
			}
			SENSOR_PRINT("SENSOR_OV8825: ov8825_GetRawInof end");
			break;
		}
		else if(PNULL!=tab_ptr[i].identify_otp){
			if(SENSOR_SUCCESS==tab_ptr[i].identify_otp(0))
			{
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

LOCAL uint32_t _ov8825_GetMaxFrameLine(uint32_t index)
{
	uint32_t max_line=0x00;
	SENSOR_TRIM_T_PTR trim_ptr=s_ov8825_Resolution_Trim_Tab;

	max_line=trim_ptr[index].frame_line;

	return max_line;
}

LOCAL uint32_t _ov8825_Identify(uint32_t param)
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
LOCAL unsigned long _ov8825_write_exposure_dummy(uint16_t expsure_line,uint16_t dummy_line,uint16_t size_index)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t frame_len=0x00;
	uint16_t frame_len_cur=0x00;
	uint16_t max_frame_len=0x00;
	uint16_t value=0x00;
	uint16_t value0=0x00;
	uint16_t value1=0x00;
	uint16_t value2=0x00;


//	SENSOR_PRINT("SENSOR_ov8825: write_exposure line:%d, dummy:%d, size_index:%d", expsure_line, dummy_line, size_index);

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

LOCAL unsigned long _ov8825_write_exposure(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t expsure_line=0x00;
	uint16_t dummy_line=0x00;
	uint16_t size_index=0x00;

	expsure_line=param&0xffff;
	dummy_line=(param>>0x10)&0x0fff;
	size_index=(param>>0x1c)&0x0f;

	 _ov8825_write_exposure_dummy(expsure_line,dummy_line,size_index);

	//SENSOR_PRINT("SENSOR_ov8825: write_exposure line:%d, dummy:%d, size_index:%d\n", expsure_line, dummy_line, size_index);

	return ret_value;
}

LOCAL unsigned long _ov8825_ex_write_exposure(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t expsure_line=0x00;
	uint16_t dummy_line=0x00;
	uint16_t size_index=0x00;

	SENSOR_PRINT("SENSOR_ov8825: write_param:%d", param);

	struct sensor_ex_exposure  *exp_dummy = NULL;

	exp_dummy = (struct sensor_ex_exposure*)param;

	expsure_line = exp_dummy->exposure;
	dummy_line = exp_dummy->dummy;
	size_index = exp_dummy->size_index;

	 _ov8825_write_exposure_dummy(expsure_line,dummy_line,size_index);

	SENSOR_PRINT("SENSOR_ov8825: write_exposure line:%d, dummy:%d, size_index:%d\n", expsure_line, dummy_line, size_index);

	return ret_value;
}

LOCAL uint32_t _ov8825_write_gain(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t value=0x00;

	SENSOR_PRINT("SENSOR_ov8825: write_gain:0x%x", param);
#if 1 // ae table 32
	uint16_t real_gain = 0;
	int cur_gain = 0x00;
	uint32_t i = 0x00;

	cur_gain = param >> 3;

	for (i = 0x00; i < 11; ++i) {
		if (32 <= cur_gain) {
			cur_gain>>=0x01;
			real_gain = (real_gain | 1)<<1;
		} else {
			cur_gain = cur_gain - 16;
			real_gain = (real_gain<<4) | (cur_gain&0x0f);
			break;
		}
	}
	value = real_gain&0xff;
	ret_value = Sensor_WriteReg(0x350b, value);/*0-7*/
	value = (real_gain>>0x08)&0x01;
	ret_value = Sensor_WriteReg(0x350a, value);/*8*/
#else
	value = param&0xff;
	ret_value = Sensor_WriteReg(0x350b, value);/*0-7*/
	value = (param>>0x08)&0x01;
	ret_value = Sensor_WriteReg(0x350a, value);/*8*/
#endif
	return ret_value;
}

LOCAL uint32_t _ov8825_write_af(uint32_t param)
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

LOCAL uint32_t _ov8825_BeforeSnapshot(uint32_t param)
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

	ret_h = (uint8_t) Sensor_ReadReg(0x380e);
	ret_l = (uint8_t) Sensor_ReadReg(0x380f);
	preview_maxline = (ret_h << 8) + ret_l;

	Sensor_SetMode(capture_mode);
	Sensor_SetMode_WaitDone();

	if (prv_linetime == cap_linetime) {
		SENSOR_PRINT("SENSOR_ov8825: prvline equal to capline");
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
	s_capture_shutter = _ov8825_get_shutter();
	s_capture_VTS = _ov8825_get_VTS();
	_ov8825_ReadGain(capture_mode);
	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, s_capture_shutter);
	s_exposure_time = s_capture_shutter * cap_linetime / 10;

	return SENSOR_SUCCESS;
}

LOCAL uint32_t _ov8825_after_snapshot(uint32_t param)
{
	SENSOR_PRINT("SENSOR_ov8825: after_snapshot mode:%d", param);
	Sensor_SetMode(param);
	return SENSOR_SUCCESS;
}

LOCAL uint32_t _ov8825_flash(uint32_t param)
{
	SENSOR_PRINT("SENSOR_ov8825: param=%d", param);

	/* enable flash, disable in _ov8825_BeforeSnapshot */
	g_flash_mode_en = param;
	Sensor_SetFlash(param);
	SENSOR_PRINT_HIGH("end");
	return SENSOR_SUCCESS;
}

LOCAL uint32_t _ov8825_StreamOn(uint32_t param)
{
	SENSOR_PRINT("SENSOR_ov8825: StreamOn");

	Sensor_WriteReg(0x0100, 0x01);
	is_stream_on = 1;
	return 0;
}

LOCAL uint32_t _ov8825_StreamOff(uint32_t param)
{
	SENSOR_PRINT("SENSOR_ov8825: StreamOff");
	if (is_stream_on) {
	Sensor_WriteReg(0x0100, 0x00);
	usleep(150*1000);
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
		_calculate_hdr_exposure(s_ov8825_gain/2,s_capture_VTS,s_capture_shutter);
		break;
	case SENSOR_HDR_EV_LEVE_1:
		_calculate_hdr_exposure(s_ov8825_gain,s_capture_VTS,s_capture_shutter);
		break;
	case SENSOR_HDR_EV_LEVE_2:
		_calculate_hdr_exposure(s_ov8825_gain,s_capture_VTS,s_capture_shutter *4);
		break;
	default:
		break;
	}
	return rtn;
}

LOCAL uint32_t _ov8825_saveLoad_exposure(unsigned long param)
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

LOCAL unsigned long _ov8825_GetExifInfo(unsigned long param)
{
	LOCAL EXIF_SPEC_PIC_TAKING_COND_T sexif;

	sexif.ExposureTime.numerator = s_exposure_time;
	sexif.ExposureTime.denominator = 1000000;

	return (unsigned long) & sexif;
}

LOCAL uint32_t _ov8825_ExtFunc(unsigned long ctl_param)
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
LOCAL uint32_t _ov8825_ReadGain(uint32_t param)
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
