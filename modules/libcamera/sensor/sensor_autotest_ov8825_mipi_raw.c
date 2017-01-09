/*
 * Copyright (C) 2012 The Android Open Source Project
 *  * Licensed under the Apache License, Version 2.0 (the "License");
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

#define ov8825_I2C_ADDR_W        0x36
#define ov8825_I2C_ADDR_R         0x36
#define OV8825_MIN_FRAME_LEN_PRV  0x5e8
#define OV8825_2_LANES

LOCAL unsigned long _at_ov8825_GetResolutionTrimTab(unsigned long param);
LOCAL unsigned long _at_ov8825_PowerOn(unsigned long power_on);
LOCAL unsigned long _at_ov8825_Identify(unsigned long param);
LOCAL unsigned long _at_ov8825_StreamOn(unsigned long param);
LOCAL unsigned long _at_ov8825_StreamOff(unsigned long param);

LOCAL const struct raw_param_info_tab s_at_ov8825_raw_param_tab[]={
	{OV8825_TRULY_02, &s_ov8825_mipi_raw_info, _ov8825_truly_Identify_otp, _ov8825_truly_update_otp},
	{RAW_INFO_END_ID, PNULL, PNULL, PNULL}
};

struct sensor_raw_info* s_at_ov8825_mipi_raw_info_ptr = NULL;
static uint32_t g_ov8825_module_id = 0;

LOCAL const SENSOR_REG_T ov8825_common_init[] = {
#if defined(OV8825_2_LANES)
	{0x0100, 0x00}, // software standby
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
	{0x0100, 0x00}, // software standby
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
	{0x3820, 0x86}, // Timing Reg20:Vflip
	{0x3821, 0x10}, // Timing Reg21:Hmirror
	{0x3f00, 0x02}, // PSRAM Ctrl0
	{0x4005, 0x18}, // Gain triger for BLC
	{0x404f, 0x8F}, // Auto BLC while more than value
	{0x4600, 0x04}, // VFIFO Ctrl0
	{0x4601, 0x01}, // VFIFO Read ST High
	{0x4602, 0x00}, // VFIFO Read ST Low
	{0x4837, 0x15}, // MIPI PCLK PERIOD
	{0x5068, 0x53}, // HSCALE_CTRL
	{0x506a, 0x53}, // VSCALE_CTRL
	{0x0100, 0x01} // wake up
#endif
};

LOCAL const SENSOR_REG_T ov8825_640x480_setting[] = {
#if  defined(OV8825_2_LANES)
#elif defined(OV8825_4_LANES)
	//@@ FVGA 4lane 640x480 30fps
	//;MIPI clock = 528M

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
	{0x3820, 0x86}, // Timing Reg20:Vflip
	{0x3821, 0x11}, // Timing Reg21:Hmirr
	{0x3f00, 0x00}, //PSRAM, 0xctrl0
	{0x4005, 0x18}, //Gain triger, 0xfor, 0xbLC
	{0x404f, 0x8F}, //0xauto, 0xbLC while more than value
	{0x4600, 0x04}, //VFIFO, 0xctrl0
	{0x4601, 0x00}, //VFIFO Read ST High
	{0x4602, 0x78}, //VFIFO Read ST Low
	{0x4837, 0x15}, // MIPI PCLK PERIOD
	{0x5068, 0x00}, //HSCALE_CTRL
	{0x506a, 0x00}, //VSCALE_CTRL

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
	{0x3821, 0x10},
	{0x4600, 0x14},
	{0x4601, 0x14},
	{0x4602, 0x00},
	{0x5068, 0x59},
	{0x506a, 0x5a},
	{0x4837, 0x1e},
	{0x5e00, 0x80},//color bar normal
#endif
};

LOCAL SENSOR_REG_TAB_INFO_T s_at_ov8825_resolution_Tab_RAW[] = {
	{ADDR_AND_LEN_OF_ARRAY(ov8825_common_init), 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(ov8825_640x480_setting), 640, 480, 24, SENSOR_IMAGE_FORMAT_RAW},
	{PNULL, 0, 0, 0, 0, 0},
};

LOCAL SENSOR_TRIM_T s_at_ov8825_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 640, 480, 175, 528, 1902, {0, 0, 640, 480}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
};

LOCAL SENSOR_IOCTL_FUNC_TAB_T s_at_ov8825_ioctl_func_tab = {
	PNULL,
	_at_ov8825_PowerOn,
	PNULL,
	_at_ov8825_Identify,

	PNULL,			// write register
	PNULL,			// read  register
	PNULL,
	_at_ov8825_GetResolutionTrimTab,

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

	PNULL, //_at_ov8825_BeforeSnapshot,
	PNULL, //_at_ov8825_after_snapshot,
	PNULL, //_at_ov8825_flash,
	PNULL,
	PNULL, //_at_ov8825_write_exposure,
	PNULL,
	PNULL, //_at_ov8825_write_gain,
	PNULL,
	PNULL,
	PNULL, //_at_ov8825_write_af,
	PNULL,
	PNULL, //_ov8825_set_awb,
	PNULL,
	PNULL,
	PNULL, //_ov8825_set_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL, //_ov8825_GetExifInfo,
	PNULL, //_at_ov8825_ExtFunc,
	PNULL, //_ov8825_set_anti_flicker,
	PNULL, //_at_ov8825_set_video_mode,
	PNULL, //pick_jpeg_stream
	PNULL,  //meter_mode
	PNULL, //get_status
	_at_ov8825_StreamOn,
	_at_ov8825_StreamOff,
	PNULL, //_at_ov8825_cfg_otp
};

SENSOR_INFO_T g_autotest_ov8825_mipi_raw_info = {
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

	s_at_ov8825_resolution_Tab_RAW,	// point to resolution table information structure
	&s_at_ov8825_ioctl_func_tab,	// point to ioctl function table
	&s_at_ov8825_mipi_raw_info_ptr,		// information and table about Rawrgb sensor
	NULL,			//&g_ov8825_ext_info,                // extend information about sensor
	SENSOR_AVDD_1800MV,	// iovdd
	SENSOR_AVDD_1500MV,	// dvdd
	1,			// skip frame num before preview
	1,			// skip frame num before capture
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

	NULL,//s_at_ov8825_video_info,
	3,			// skip frame num while change setting
};

LOCAL struct sensor_raw_info* Sensor_at_GetContext(void)
{
	return s_at_ov8825_mipi_raw_info_ptr;
}

LOCAL uint32_t Sensor_at_ov8825_InitRawTuneInfo(void)
{
	uint32_t rtn=0x00;
	CMR_LOGE("debug %s  %d E\n ",__func__,__LINE__);
	struct sensor_raw_info* raw_sensor_ptr=Sensor_at_GetContext();
	struct sensor_raw_tune_info* sensor_ptr=raw_sensor_ptr->tune_ptr;
	struct sensor_raw_cali_info* cali_ptr=raw_sensor_ptr->cali_ptr;
	sensor_ptr->awb.alg_id = 0;
	sensor_ptr->awb.smart_index = 4;
	return rtn;
}

LOCAL unsigned long _at_ov8825_GetResolutionTrimTab(unsigned long param)
{
	SENSOR_PRINT("0x%lx",  (unsigned long)s_at_ov8825_Resolution_Trim_Tab);
	return (unsigned long) s_at_ov8825_Resolution_Trim_Tab;
}

LOCAL unsigned long _at_ov8825_PowerOn(unsigned long power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_autotest_ov8825_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_autotest_ov8825_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_autotest_ov8825_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_autotest_ov8825_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_autotest_ov8825_mipi_raw_info.reset_pulse_level;
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
		Sensor_SetMonitorVoltage(SENSOR_AVDD_3300MV);
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
	}
	SENSOR_PRINT("SENSOR_ov8825: _ov8825_Power_On(1:on, 0:off): %ld", power_on);
	return SENSOR_SUCCESS;
}

LOCAL uint32_t _at_ov8825_GetRawInof(void)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_at_ov8825_raw_param_tab;
	uint32_t i=0x00;
	uint16_t stream_value = 0;

	stream_value = Sensor_ReadReg(0x0100);
	if (1 != (stream_value & 0x01)) {
		Sensor_WriteReg(0x0100, 0x01);
		usleep(5 * 1000);
	}

	for (i=0x00; ; i++) {
		g_ov8825_module_id = i;
		if (RAW_INFO_END_ID == tab_ptr[i].param_id) {
			if (NULL == s_at_ov8825_mipi_raw_info_ptr) {
				SENSOR_PRINT("SENSOR_OV8825: ov8825_GetRawInof no param error");
				rtn=SENSOR_FAIL;
			}
			SENSOR_PRINT("SENSOR_at_OV8825: _at_ov8825_GetRawInof end");
			break;
		} else if (PNULL!=tab_ptr[i].identify_otp) {
			if (SENSOR_SUCCESS==tab_ptr[i].identify_otp(0)) {
				s_at_ov8825_mipi_raw_info_ptr = tab_ptr[i].info_ptr;
				SENSOR_PRINT("SENSOR_at_OV8825: _at_ov8825_GetRawInof id:0x%x success", g_ov8825_module_id);
				break;
			}
		}
	}

	if (1 != (stream_value & 0x01)) {
		Sensor_WriteReg(0x0100, stream_value);
		usleep(5 * 1000);
	}

	return rtn;
}

LOCAL unsigned long _at_ov8825_Identify(unsigned long param)
{
	#define ov8825_PID_VALUE    0x88
	#define ov8825_PID_ADDR     0x300A
	#define ov8825_VER_VALUE    0x25
	#define ov8825_VER_ADDR     0x300B
	uint8_t pid_value = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	ALOGE("auto_test 1 SENSOR_at_ov8825: mipi raw identify\n");

	pid_value = Sensor_ReadReg(ov8825_PID_ADDR);
	if (ov8825_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(ov8825_VER_ADDR);
		SENSOR_PRINT("SENSOR_at_ov8825: Identify: PID = %x, VER = %x", pid_value, ver_value);
		if (ov8825_VER_VALUE == ver_value) {
			SENSOR_PRINT("SENSOR_at_ov8825: this is _at_ov8825 sensor !");
			ret_value=_at_ov8825_GetRawInof();
			if (SENSOR_SUCCESS != ret_value)
			{
				SENSOR_PRINT("SENSOR_at_ov8825: the module is unknow error !");
			}
			Sensor_at_ov8825_InitRawTuneInfo();
		} else {
			SENSOR_PRINT("SENSOR_at_ov8825: Identify this is OV%x%x sensor !", pid_value, ver_value);
		}
	} else {
		SENSOR_PRINT("SENSOR_at_ov8825: identify fail,pid_value=%d", pid_value);
	}

	return ret_value;
}


LOCAL unsigned long _at_ov8825_StreamOn(unsigned long param)
{
	SENSOR_PRINT("SENSOR_ov8825: StreamOn");

	Sensor_WriteReg(0x0100, 0x01);

	return 0;
}

LOCAL unsigned long _at_ov8825_StreamOff(unsigned long param)
{
	SENSOR_PRINT("SENSOR_ov8825: StreamOff");

	Sensor_WriteReg(0x0100, 0x00);
	usleep(100*1000);

	return 0;
}
