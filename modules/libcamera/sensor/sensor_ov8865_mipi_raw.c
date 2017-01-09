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
#include "sensor_ov8865_raw_param.c"

#define ov8865_I2C_ADDR_W        (0x20>>1)
#define ov8865_I2C_ADDR_R         (0x21>>1)

#define OV8865_RAW_PARAM_COM  (0x0000)
#define OV8865_RAW_PARAM_O_FILM (0x0007)

#define  BG_RATIO_TYPICAL_OFILM (0x152)
#define  RG_RATIO_TYPICAL_OFILM (0x137)
#define OV8865_4_LANES

struct ov8865_otp_info {
	int32_t module_integrator_id;
	int32_t lens_id;
	int32_t production_year;
	int32_t production_month;
	int32_t production_day;
	int32_t rg_ratio;
	int32_t bg_ratio;
	int32_t light_rg;
	int32_t light_bg;
	int32_t user_data[5];
	int32_t lenc[62];
	int32_t VCM_start;
	int32_t VCM_end;
	int32_t VCM_dir;
};

LOCAL unsigned long _ov8865_GetResolutionTrimTab(unsigned long param);
LOCAL unsigned long _ov8865_PowerOn(unsigned long power_on);
LOCAL unsigned long _ov8865_Identify(unsigned long param);
LOCAL unsigned long _ov8865_BeforeSnapshot(unsigned long param);
LOCAL unsigned long _ov8865_after_snapshot(unsigned long param);
LOCAL unsigned long _ov8865_StreamOn(unsigned long param);
LOCAL unsigned long _ov8865_StreamOff(unsigned long param);
LOCAL unsigned long _ov8865_write_exposure(unsigned long param);
LOCAL unsigned long _ov8865_write_gain(unsigned long param);
LOCAL unsigned long _ov8865_write_af(unsigned long param);
LOCAL unsigned long _ov8865_flash(unsigned long param);
LOCAL unsigned long _ov8865_ExtFunc(unsigned long ctl_param);
LOCAL uint32_t _ov8865_get_VTS(void);
LOCAL unsigned long _ov8865_set_VTS(unsigned long VTS);
LOCAL uint32_t _ov8865_ReadGain(uint32_t *real_gain);
LOCAL unsigned long _ov8865_set_video_mode(unsigned long param);
LOCAL uint32_t _ov8865_get_shutter(void);
LOCAL uint32_t _ov8865_com_Identify_otp(void* param_ptr);
LOCAL uint32_t _ov8865_o_film_Identify_otp(void* param_ptr);
LOCAL unsigned long _ov8865_dw9714_SRCInit(unsigned long mode);
LOCAL uint32_t _ov8865_o_film_update_otp(void* param_ptr);
LOCAL unsigned long _ov8865_cfg_otp(unsigned long  param);
LOCAL int32_t _ov8865_check_o_film_otp(int32_t cmd, int32_t index);
LOCAL int32_t _ov8865_read_o_film_otp(int32_t cmd, int32_t index, struct ov8865_otp_info *otp_ptr);
LOCAL int32_t _ov8865_update_o_film_otp_wb(void);
LOCAL int32_t _ov8865_update_o_film_otp_lenc(void);

LOCAL uint32_t s_ov8865_realgain_128 = 0;
LOCAL uint32_t s_capture_shutter = 0;
LOCAL uint32_t s_capture_VTS = 0;
LOCAL uint32_t g_module_id = 0;
struct sensor_raw_info* s_ov8865_mipi_raw_info_ptr=NULL;

LOCAL const struct raw_param_info_tab s_ov8865_raw_param_tab[]={
	{OV8865_RAW_PARAM_O_FILM, &s_ov8865_mipi_raw_info, _ov8865_o_film_Identify_otp, _ov8865_o_film_update_otp},
	{OV8865_RAW_PARAM_COM, &s_ov8865_mipi_raw_info, _ov8865_com_Identify_otp, PNULL},
	{RAW_INFO_END_ID, PNULL, PNULL, PNULL}
};

LOCAL const SENSOR_REG_T ov8865_common_init[] = {
	// Initialization (Global Setting)
	//Initial to Raw 10bit 1632x1224 30fps 4lane 624M bps/lane
	{0x0103,0x01}, // software reset
	{0x0100,0x00}, // software standby
	{0x0100,0x00},
	{0x0100,0x00},
	{0x0100,0x00},
	{0x3638,0xff}, // analog control
	{0x0302,0x1a}, // 0x1a for MIPI_clk 624M/0x1e for MIPI_clk 720M
	{0x0303,0x00}, // PLL
	{0x0304,0x03}, // PLL
	{0x030e,0x00}, // PLL
	{0x030f,0x09}, // PLL
	{0x0312,0x01}, // PLL
	{0x031e,0x0c}, // PLL
	{0x3015,0x01}, // clock Div
	{0x3018,0x72}, // MIPI 4 lane
	{0x3020,0x93}, // clock normal, pclk/1
	{0x3022,0x01}, // pd_mini enable when rst_sync
	{0x3031,0x0a}, // 10-bit
	{0x3106,0x01}, // PLL
	{0x3305,0xf1},
	{0x3308,0x00},
	{0x3309,0x28},
	{0x330a,0x00},
	{0x330b,0x20},
	{0x330c,0x00},
	{0x330d,0x00},
	{0x330e,0x00},
	{0x330f,0x40},
	{0x3307,0x04},
	{0x3604,0x04}, // analog control
	{0x3602,0x30},
	{0x3605,0x00},
	{0x3607,0x20},
	{0x3608,0x11},
	{0x3609,0x68},
	{0x360a,0x40},
	{0x360c,0xdd},
	{0x360e,0x0c},
	{0x3610,0x07},
	{0x3612,0x89},
	{0x3614,0x28},
	{0x3617,0x40},
	{0x3618,0x5a},
	{0x3619,0x9b},
	{0x361c,0x00},
	{0x361d,0x60},
	{0x3631,0x60},
	{0x3633,0x10},
	{0x3634,0x10},
	{0x3635,0x10},
	{0x3636,0x0a},
	{0x3641,0x55}, // MIPI settings
	{0x3646,0x86}, // MIPI settings
	{0x3647,0x27}, // MIPI settings
	{0x364a,0x1b}, // MIPI settings
	{0x3500,0x00}, // exposurre HH
	{0x3501,0x4c}, // expouere H
	{0x3502,0x00}, // exposure L
	{0x3503,0x04}, // gain delay 1 frame, exposure delay 1 frame
	{0x3508,0x02}, // gain H
	{0x3509,0x00}, // gain L
	{0x3700,0x18}, // sensor control
	{0x3701,0x0c}, //
	{0x3702,0x28},
	{0x3703,0x19},
	{0x3704,0x14},
	{0x3705,0x00},
	{0x3706,0x28},
	{0x3707,0x04},
	{0x3708,0x24},
	{0x3709,0x40},
	{0x370a,0x00},
	{0x370b,0xa8},
	{0x370c,0x04},
	{0x3718,0x12},
	{0x3719,0x31},
	{0x3712,0x42},
	{0x3714,0x12},
	{0x371e,0x19},
	{0x371f,0x40},
	{0x3720,0x05},
	{0x3721,0x05},
	{0x3724,0x02},
	{0x3725,0x02},
	{0x3726,0x06},
	{0x3728,0x05},
	{0x3729,0x02},
	{0x372a,0x03},
	{0x372b,0x53},
	{0x372c,0xa3},
	{0x372d,0x53},
	{0x372e,0x06},
	{0x372f,0x10},
	{0x3730,0x01},
	{0x3731,0x06},
	{0x3732,0x14},
	{0x3733,0x10},
	{0x3734,0x40},
	{0x3736,0x20},
	{0x373a,0x02},
	{0x373b,0x0c},
	{0x373c,0x0a},
	{0x373e,0x03},
	{0x3755,0x40},
	{0x3758,0x00},
	{0x3759,0x4c},
	{0x375a,0x06},
	{0x375b,0x13},
	{0x375c,0x20},
	{0x375d,0x02},
	{0x375e,0x00},
	{0x375f,0x14},
	{0x3767,0x04},
	{0x3768,0x04},
	{0x3769,0x20},
	{0x376c,0x00},
	{0x376d,0x00},
	{0x376a,0x08},
	{0x3761,0x00},
	{0x3762,0x00},
	{0x3763,0x00},
	{0x3766,0xff},
	{0x376b,0x42},
	{0x3772,0x23},
	{0x3773,0x02},
	{0x3774,0x16},
	{0x3775,0x12},
	{0x3776,0x08},
	{0x37a0,0x44},
	{0x37a1,0x3d},
	{0x37a2,0x3d},
	{0x37a3,0x01},
	{0x37a4,0x00},
	{0x37a5,0x08},
	{0x37a6,0x00},
	{0x37a7,0x44},
	{0x37a8,0x4c},
	{0x37a9,0x4c},
	{0x3760,0x00},
	{0x376f,0x01},
	{0x37aa,0x44},
	{0x37ab,0x2e},
	{0x37ac,0x2e},
	{0x37ad,0x33},
	{0x37ae,0x0d},
	{0x37af,0x0d},
	{0x37b0,0x00},
	{0x37b1,0x00},
	{0x37b2,0x00},
	{0x37b3,0x42},
	{0x37b4,0x42},
	{0x37b5,0x33},
	{0x37b6,0x00},
	{0x37b7,0x00},
	{0x37b8,0x00},
	{0x37b9,0xff}, // sensor control
	{0x3800,0x00}, // X start H
	{0x3801,0x0c}, // X start L
	{0x3802,0x00}, // Y start H
	{0x3803,0x0c}, // Y start L
	{0x3804,0x0c}, // X end H
	{0x3805,0xd3}, // X end L
	{0x3806,0x09}, // Y end H
	{0x3807,0xa3}, // Y end L
	{0x3808,0x06}, // X output size H
	{0x3809,0x60}, // X output size L
	{0x380a,0x04}, // Y output size H
	{0x380b,0xc8}, // Y output size L
	{0x380c,0x07}, // HTS H
	{0x380d,0x83}, // HTS L
	{0x380e,0x04}, // VTS H
	{0x380f,0xe0}, // VTS L
	{0x3810,0x00}, // ISP X win H
	{0x3811,0x04}, // ISP X win L
	{0x3813,0x04}, // ISP Y win L
	{0x3814,0x03}, // X inc odd
	{0x3815,0x01}, // X inc even
	{0x3820,0x00}, // flip off
	{0x3821,0x67}, // hsync_en_o, fst_vbin, mirror on
	{0x382a,0x03}, // Y inc odd
	{0x382b,0x01}, // Y inc even
	{0x3830,0x08}, // ablc_use_num[5:1]
	{0x3836,0x02}, // zline_use_num[5:1]
	{0x3837,0x18}, // vts_add_dis, cexp_gt_vts_offs=8
	{0x3841,0xff}, // auto size
	{0x3846,0x88}, // Y/X boundary pixel numbber for auto size mode
	{0x3f08,0x0b},
	{0x4000,0xf1}, // our range trig en, format chg en, gan chg en, exp chg en, median en
	{0x4001,0x14}, // left 32 column, final BLC offset limitation enable
	{0x4005,0x10}, // BLC target
	{0x400b,0x0c}, // start line =0, offset limitation en, cut range function en
	{0x400d,0x10}, // offset trigger threshold
	{0x401b,0x00},
	{0x401d,0x00},
	{0x4020,0x01}, // anchor left start H
	{0x4021,0x20}, // anchor left start L
	{0x4022,0x01}, // anchor left end H
	{0x4023,0x9f}, // anchor left end L
	{0x4024,0x03}, // anchor right start H
	{0x4025,0xe0}, // anchor right start L
	{0x4026,0x04}, // anchor right end H
	{0x4027,0x5f}, // anchor right end L
	{0x4028,0x00}, // top zero line start
	{0x4029,0x02}, // top zero line number
	{0x402a,0x04}, // top black line start
	{0x402b,0x04}, // top black line number
	{0x402c,0x02}, // bottom zero line start
	{0x402d,0x02}, // bottom zero line number
	{0x402e,0x08}, // bottom black line start
	{0x402f,0x02}, // bottom black line number
	{0x401f,0x00}, // anchor one disable
	{0x4034,0x3f}, // limitation BLC offset
	{0x4300,0xff}, // clip max H
	{0x4301,0x00}, // clip min H
	{0x4302,0x0f}, // clip min L/clip max L
	{0x4500,0x40}, // ADC sync control
	{0x4503,0x10},
	{0x4601,0x74}, // V FIFO
	{0x481f,0x32}, // clk_prepare_min
	{0x4837,0x19}, // clock period
	{0x4850,0x10}, // lane select
	{0x4851,0x32}, // lane select
	{0x4b00,0x2a}, // LVDS settings
	{0x4b0d,0x00}, // LVDS settings
	{0x4d00,0x04}, // temperature sensor
	{0x4d01,0x18}, // temperature sensor
	{0x4d02,0xc3}, // temperature sensor
	{0x4d03,0xff}, // temperature sensor
	{0x4d04,0xff}, // temperature sensor
	{0x4d05,0xff}, // temperature sensor
	{0x5000,0x96}, // LENC on, MWB on, BPC on, WPC on
	{0x5001,0x01}, // BLC on
	{0x5002,0x08}, // vario pixel off
	{0x5901,0x00},
	{0x5e00,0x00}, // test pattern off
	{0x5e01,0x41}, // window cut enable
//	{0x0100,0x01}, // wake up, streaming
	{0x5800,0x1d}, // lens correction
	{0x5801,0x0e},
	{0x5802,0x0c},
	{0x5803,0x0c},
	{0x5804,0x0f},
	{0x5805,0x22},
	{0x5806,0x0a},
	{0x5807,0x06},
	{0x5808,0x05},
	{0x5809,0x05},
	{0x580a,0x07},
	{0x580b,0x0a},
	{0x580c,0x06},
	{0x580d,0x02},
	{0x580e,0x00},
	{0x580f,0x00},
	{0x5810,0x03},
	{0x5811,0x07},
	{0x5812,0x06},
	{0x5813,0x02},
	{0x5814,0x00},
	{0x5815,0x00},
	{0x5816,0x03},
	{0x5817,0x07},
	{0x5818,0x09},
	{0x5819,0x06},
	{0x581a,0x04},
	{0x581b,0x04},
	{0x581c,0x06},
	{0x581d,0x0a},
	{0x581e,0x19},
	{0x581f,0x0d},
	{0x5820,0x0b},
	{0x5821,0x0b},
	{0x5822,0x0e},
	{0x5823,0x22},
	{0x5824,0x23},
	{0x5825,0x28},
	{0x5826,0x29},
	{0x5827,0x27},
	{0x5828,0x13},
	{0x5829,0x26},
	{0x582a,0x33},
	{0x582b,0x32},
	{0x582c,0x33},
	{0x582d,0x16},
	{0x582e,0x14},
	{0x582f,0x30},
	{0x5830,0x31},
	{0x5831,0x30},
	{0x5832,0x15},
	{0x5833,0x26},
	{0x5834,0x23},
	{0x5835,0x21},
	{0x5836,0x23},
	{0x5837,0x05},
	{0x5838,0x36},
	{0x5839,0x27},
	{0x583a,0x28},
	{0x583b,0x26},
	{0x583c,0x24},
	{0x583d,0xdf}, // lens correction
};

LOCAL const SENSOR_REG_T ov8865_1632x1224_setting[] = {
#if defined(OV8865_4_LANES)
	//Raw 10bit 1632x1224 30fps 4lane 624M bps/lane
	{0x0100,0x00}, //; software standby
	{0x030d,0x1e},
	{0x030f,0x09}, //; PLL
	{0x3018,0x72},
	{0x3106,0x01},
	{0x3501,0x4c}, //; expouere H
	{0x3502,0x00}, //; exposure L
	{0x3700,0x18}, //; sensor control
	{0x3701,0x0c},
	{0x3702,0x28},
	{0x3703,0x19},
	{0x3704,0x14},
	{0x3706,0x28},
	{0x3707,0x04},
	{0x3708,0x24},
	{0x3709,0x40},
	{0x370a,0x00},
	{0x370b,0xa8},
	{0x370c,0x04},
	{0x3718,0x12},
	{0x3712,0x42},
	{0x371e,0x19},
	{0x371f,0x40},
	{0x3720,0x05},
	{0x3721,0x05},
	{0x3724,0x02},
	{0x3725,0x02},
	{0x3726,0x06},
	{0x3728,0x05},
	{0x3729,0x02},
	{0x372a,0x03},
	{0x372b,0x53},
	{0x372c,0xa3},
	{0x372d,0x53},
	{0x372e,0x06},
	{0x372f,0x10},
	{0x3730,0x01},
	{0x3731,0x06},
	{0x3732,0x14},
	{0x3736,0x20},
	{0x373a,0x02},
	{0x373b,0x0c},
	{0x373c,0x0a},
	{0x373e,0x03},
	{0x375a,0x06},
	{0x375b,0x13},
	{0x375d,0x02},
	{0x375f,0x14},
	{0x3767,0x04},
	{0x3769,0x20},
	{0x3772,0x23},
	{0x3773,0x02},
	{0x3774,0x16},
	{0x3775,0x12},
	{0x3776,0x08},
	{0x37a0,0x44},
	{0x37a1,0x3d},
	{0x37a2,0x3d},
	{0x37a3,0x01},
	{0x37a5,0x08},
	{0x37a7,0x44},
	{0x37a8,0x4c},
	{0x37a9,0x4c},
	{0x37aa,0x44},
	{0x37ab,0x2e},
	{0x37ac,0x2e},
	{0x37ad,0x33},
	{0x37ae,0x0d},
	{0x37af,0x0d},
	{0x37b3,0x42},
	{0x37b4,0x42},
	{0x37b5,0x33},
	{0x3808,0x06}, //X output size H
	{0x3809,0x60}, //X output size L
	{0x380a,0x04}, //Y output size H
	{0x380b,0xc8}, //Y output size L
	{0x380c,0x07}, //HTS H
	{0x380d,0x83}, //HTS L
	{0x380e,0x04}, //VTS H
	{0x380f,0xe0}, //VTS L
	{0x3813,0x04}, //ISP Y win L
	{0x3814,0x03}, //X inc odd
	{0x3820,0x06},
	{0x3821,0x61}, //hsync_en_o, fst_vbin, mirror on
	{0x382a,0x03}, //Y inc odd
	{0x382b,0x01}, //Y inc even
	{0x3830,0x08}, //ablc_use_num[5:1]
	{0x3836,0x02}, //zline_use_num[5:1]
	{0x3846,0x88}, //Y/X boundary pixel numbber for auto size mode
	{0x3f08,0x0b},
	{0x4000,0xf1}, //our range trig en, format chg en, gan chg en, exp chg en, median en
	{0x4001,0x14}, //left 32 column, final BLC offset limitation enable
	{0x4020,0x01}, //anchor left start H
	{0x4021,0x20}, //anchor left start L
	{0x4022,0x01}, //anchor left end H
	{0x4023,0x9f}, //anchor left end L
	{0x4024,0x03}, //anchor right start H
	{0x4025,0xe0}, //anchor right start L
	{0x4026,0x04}, //anchor right end H
	{0x4027,0x5f}, //anchor right end L
	{0x402a,0x04}, //top black line start
	{0x402c,0x02}, //bottom zero line start
	{0x402d,0x02}, //bottom zero line number
	{0x402e,0x08}, //bottom black line start
	{0x4500,0x40}, //ADC sync control
	{0x4601,0x74}, //V FIFO control
	{0x5002,0x08}, //vario pixel off
	{0x5901,0x00},
//	{0x0100,0x01}, //wake up, streaming
#elif defined(OV8865_2_LANES)
	//Raw 10bit 1632x1224 30fps 2lane 720M bps/lane
	//XVCLK=24Mhz, sysclk=72Mhz
	//MIPI 2 lane, 720Mbps/lane
	{0x0100,0x00},//software standby
	{0x030f,0x09},//PLL
	{0x3018,0x32},//MIPI 2 lane
	{0x3106,0x01},
	{0x3501,0x4c},//expouere H
	{0x3502,0x00},//exposure L
	{0x3700,0x18},//sensor control
	{0x3701,0x0c},
	{0x3702,0x28},
	{0x3703,0x19},
	{0x3704,0x14},
	{0x3706,0x28},
	{0x3707,0x04},
	{0x3708,0x24},
	{0x3709,0x40},
	{0x370a,0x00},
	{0x370b,0xa8},
	{0x370c,0x04},
	{0x3718,0x12},
	{0x3712,0x42},
	{0x371e,0x19},
	{0x371f,0x40},
	{0x3720,0x05},
	{0x3721,0x05},
	{0x3724,0x02},
	{0x3725,0x02},
	{0x3726,0x06},
	{0x3728,0x05},
	{0x3729,0x02},
	{0x372a,0x03},
	{0x372b,0x53},
	{0x372c,0xa3},
	{0x372d,0x53},
	{0x372e,0x06},
	{0x372f,0x10},
	{0x3730,0x01},
	{0x3731,0x06},
	{0x3732,0x14},
	{0x3736,0x20},
	{0x373a,0x02},
	{0x373b,0x0c},
	{0x373c,0x0a},
	{0x373e,0x03},
	{0x375a,0x06},
	{0x375b,0x13},
	{0x375d,0x02},
	{0x375f,0x14},
	{0x3767,0x04},
	{0x3769,0x20},
	{0x3772,0x23},
	{0x3773,0x02},
	{0x3774,0x16},
	{0x3775,0x12},
	{0x3776,0x08},
	{0x37a0,0x44},
	{0x37a1,0x3d},
	{0x37a2,0x3d},
	{0x37a3,0x01},
	{0x37a5,0x08},
	{0x37a7,0x44},
	{0x37a8,0x4c},
	{0x37a9,0x4c},
	{0x37aa,0x44},
	{0x37ab,0x2e},
	{0x37ac,0x2e},
	{0x37ad,0x33},
	{0x37ae,0x0d},
	{0x37af,0x0d},
	{0x37b3,0x42},
	{0x37b4,0x42},
	{0x37b5,0x33},
	{0x3808,0x06},//X output size H
	{0x3809,0x60},//X output size L
	{0x380a,0x04},//Y output size H
	{0x380b,0xc8},//Y output size L
	{0x380c,0x07},//HTS H
	{0x380d,0x83},//HTS L
	{0x380e,0x04},//VTS H
	{0x380f,0xe0},//VTS L
	{0x3813,0x04},//ISP Y win L
	{0x3814,0x03},//X inc odd
	{0x3820,0x06},
	{0x3821,0x61},//hsync_en_o, fst_vbin, mirror on
	{0x382a,0x03},//Y inc odd
	{0x382b,0x01},//Y inc even
	{0x3830,0x08},//ablc_use_num[5:1]
	{0x3836,0x02},//zline_use_num[5:1]
	{0x3846,0x88},//Y/X boundary pixel numbber for auto size mode
	{0x3f08,0x0b},//
	{0x4000,0xf1},//our range trig en, format chg en, gan chg en, exp chg en, median en
	{0x4001,0x14},//left 32 column, final BLC offset limitation enable
	{0x4020,0x01},//anchor left start H
	{0x4021,0x20},//anchor left start L
	{0x4022,0x01},//anchor left end H
	{0x4023,0x9f},//anchor left end L
	{0x4024,0x03},//anchor right start H
	{0x4025,0xe0},//anchor right start L
	{0x4026,0x04},//anchor right end H
	{0x4027,0x5f},//anchor right end L
	{0x402a,0x04},//top black line start
	{0x402c,0x02},//bottom zero line start
	{0x402d,0x02},//bottom zero line number
	{0x402e,0x08},//bottom black line start
	{0x4500,0x40},//ADC sync control
	{0x4601,0x74},//V FIFO control
	{0x5002,0x08},//vario pixel off
	{0x5901,0x00},//
//	{0x0100,0x01},//wake up, streaming
#endif
};

LOCAL const SENSOR_REG_T ov8865_3264x2448_setting[] = {
#if defined(OV8865_2_LANES)
	//Raw 10bit 3264x2448 15fps 2lane 720M bps/lane
	//XVCLK=24Mhz, sysclk=72Mhz
	//MIPI 2 lane, 720Mbps/lane
	{0x0100,0x00},//software standby
	{0x030f,0x04},//PLL
	{0x3018,0x32},
	{0x3106,0x21},
	{0x3501,0x98},//expouere H
	{0x3502,0x60},//exposure L
	{0x3700,0x18},//sensor control
	{0x3701,0x0c},
	{0x3702,0x28},
	{0x3703,0x19},
	{0x3704,0x14},
	{0x3706,0x28},
	{0x3707,0x04},
	{0x3708,0x24},
	{0x3709,0x40},
	{0x370a,0x00},
	{0x370b,0xa8},
	{0x370c,0x04},
	{0x3718,0x12},
	{0x3712,0x42},
	{0x371e,0x19},
	{0x371f,0x40},
	{0x3720,0x05},
	{0x3721,0x05},
	{0x3724,0x02},
	{0x3725,0x02},
	{0x3726,0x06},
	{0x3728,0x05},
	{0x3729,0x02},
	{0x372a,0x03},
	{0x372b,0x53},
	{0x372c,0xa3},
	{0x372d,0x53},
	{0x372e,0x06},
	{0x372f,0x10},
	{0x3730,0x01},
	{0x3731,0x06},
	{0x3732,0x14},
	{0x3736,0x20},
	{0x373a,0x02},
	{0x373b,0x0c},
	{0x373c,0x0a},
	{0x373e,0x03},
	{0x375a,0x06},
	{0x375b,0x13},
	{0x375d,0x02},
	{0x375f,0x14},
	{0x3767,0x04},
	{0x3769,0x20},
	{0x3772,0x23},
	{0x3773,0x02},
	{0x3774,0x16},
	{0x3775,0x12},
	{0x3776,0x08},
	{0x37a0,0x44},
	{0x37a1,0x3d},
	{0x37a2,0x3d},
	{0x37a3,0x02},
	{0x37a5,0x09},
	{0x37a7,0x44},
	{0x37a8,0x4c},
	{0x37a9,0x4c},
	{0x37aa,0x44},
	{0x37ab,0x2e},
	{0x37ac,0x2e},
	{0x37ad,0x33},
	{0x37ae,0x0d},
	{0x37af,0x0d},
	{0x37b3,0x42},
	{0x37b4,0x42},
	{0x37b5,0x33},
	{0x3808,0x0c},//X output size H
	{0x3809,0xc0},//X output size L
	{0x380a,0x09},//Y output size H
	{0x380b,0x90},//Y output size L
	{0x380c,0x07},//HTS H
	{0x380d,0x98},//HTS L
	{0x380e,0x09},//VTS H
	{0x380f,0xa6},//VTS L
	{0x3813,0x02},//ISP Y win L
	{0x3814,0x01},//X inc odd
	{0x3820,0x06},
	{0x3821,0x40},
	{0x382a,0x01},//Y inc odd
	{0x382b,0x01},//Y inc even
	{0x3830,0x04},//ablc_use_num[5:1]
	{0x3836,0x01},//zline_use_num[5:1]
	{0x3846,0x48},//Y/X boundary pixel numbber for auto size mode
	{0x3f08,0x0b},//
	{0x4000,0xf1},//our range trig en, format chg en, gan chg en, exp chg en, median en
	{0x4001,0x04},//left 32 column, final BLC offset limitation enable
	{0x4020,0x02},//anchor left start H
	{0x4021,0x40},//anchor left start L
	{0x4022,0x03},//anchor left end H
	{0x4023,0x3f},//anchor left end L
	{0x4024,0x07},//anchor right start H
	{0x4025,0xc0},//anchor right start L
	{0x4026,0x08},//anchor right end H
	{0x4027,0xbf},//anchor right end L
	{0x402a,0x04},//top black line start
	{0x402c,0x02},//bottom zero line start
	{0x402d,0x02},//bottom zero line number
	{0x402e,0x08},//bottom black line start
	{0x4500,0x68},//ADC sync control
	{0x4601,0x10},//V FIFO control
	{0x5002,0x08},//vario pixel off
	{0x5901,0x00},//
//	{0x0100,0x01},//wake up, strea1ming
#elif defined(OV8865_4_LANES)
	//Raw 10bit 3264*2448 29fps 4lane 624M bps/lane
	{0x0100,0x00}, //software standby
	{0x030d,0x1d},
	{0x030f,0x04}, //PLL
	{0x3018,0x72},
	{0x3106,0x01},
	{0x3501,0x98}, //expouere H
	{0x3502,0x60}, //exposure L
	{0x3700,0x30}, //sensor control
	{0x3701,0x18},
	{0x3702,0x50},
	{0x3703,0x32},
	{0x3704,0x28},
	{0x3706,0x50},
	{0x3707,0x08},
	{0x3708,0x48},
	{0x3709,0x80},
	{0x370a,0x01},
	{0x370b,0x50},
	{0x370c,0x07},
	{0x3718,0x14},
	{0x3712,0x44},
	{0x371e,0x31},
	{0x371f,0x7f},
	{0x3720,0x0a},
	{0x3721,0x0a},
	{0x3724,0x04},
	{0x3725,0x04},
	{0x3726,0x0c},
	{0x3728,0x0a},
	{0x3729,0x03},
	{0x372a,0x06},
	{0x372b,0xa6},
	{0x372c,0xa6},
	{0x372d,0xa6},
	{0x372e,0x0c},
	{0x372f,0x20},
	{0x3730,0x02},
	{0x3731,0x0c},
	{0x3732,0x28},
	{0x3736,0x30},
	{0x373a,0x04},
	{0x373b,0x18},
	{0x373c,0x14},
	{0x373e,0x06},
	{0x375a,0x0c},
	{0x375b,0x26},
	{0x375d,0x04},
	{0x375f,0x28},
	{0x3767,0x04},
	{0x3769,0x20},
	{0x3772,0x46},
	{0x3773,0x04},
	{0x3774,0x2c},
	{0x3775,0x13},
	{0x3776,0x10},
	{0x37a0,0x88},
	{0x37a1,0x7a},
	{0x37a2,0x7a},
	{0x37a3,0x02},
	{0x37a5,0x09},
	{0x37a7,0x88},
	{0x37a8,0x98},
	{0x37a9,0x98},
	{0x37aa,0x88},
	{0x37ab,0x5c},
	{0x37ac,0x5c},
	{0x37ad,0x55},
	{0x37ae,0x19},
	{0x37af,0x19},
	{0x37b3,0x84},
	{0x37b4,0x84},
	{0x37b5,0x66},
	{0x3808,0x0c},//X output size H
	{0x3809,0xc0},//X output size L
	{0x380a,0x09},//Y output size H
	{0x380b,0x90},//Y output size L
	{0x380c,0x07},//HTS H
	{0x380d,0x98},//HTS L
	{0x380e,0x09},//VTS H
	{0x380f,0xa6},//VTS L
	{0x3813,0x02},//ISP Y win L
	{0x3814,0x01},//X inc odd
	{0x3820,0x06},
	{0x3821,0x40},
	{0x382a,0x01},//Y inc odd
	{0x382b,0x01},//Y inc even
	{0x3830,0x04},//ablc_use_num[5:1]
	{0x3836,0x01},//zline_use_num[5:1]
	{0x3846,0x48},//Y/X boundary pixel numbber for auto size mode
	{0x3f08,0x16},
	{0x4000,0xf1},//our range trig en, format chg en, gan chg en, exp chg en, median en
	{0x4001,0x04},//left 32 column, final BLC offset limitation enable
	{0x4020,0x02},//anchor left start H
	{0x4021,0x40},//anchor left start L
	{0x4022,0x03},//anchor left end H
	{0x4023,0x3f},//anchor left end L
	{0x4024,0x07},//anchor right start H
	{0x4025,0xc0},//anchor right start L
	{0x4026,0x08},//anchor right end H
	{0x4027,0xbf},//anchor right end L
	{0x402a,0x04},//top black line start
	{0x402c,0x02},//bottom zero line start
	{0x402d,0x02},//bottom zero line number
	{0x402e,0x08},//bottom black line start
	{0x4500,0x68},//ADC sync control
	{0x4601,0x10},//V FIFO control
	{0x5002,0x08},//vario pixel off
	{0x5901,0x00},
//	{0x0100,0x01},//wake up, streaming
#endif
};

LOCAL SENSOR_REG_TAB_INFO_T s_ov8865_resolution_Tab_RAW[] = {
	{ADDR_AND_LEN_OF_ARRAY(ov8865_common_init), 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(ov8865_1632x1224_setting), 1632, 1224, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(ov8865_3264x2448_setting), 3264, 2448, 24, SENSOR_IMAGE_FORMAT_RAW},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

LOCAL SENSOR_TRIM_T s_ov8865_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 1632, 1224, 267, 900, 0x4e0, {0, 0, 1632, 1224}},
	{0, 0, 3264, 2448, 140, 900, 0x9a6, {0, 0, 3264, 2448}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}}
};

LOCAL const SENSOR_REG_T s_ov8865_1632x1224_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

LOCAL const SENSOR_REG_T s_ov8865_3264x2448_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

LOCAL SENSOR_VIDEO_INFO_T s_ov8865_video_info[] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{30, 30, 267, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_ov8865_1632x1224_video_tab},
	{{{30, 30, 140, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_ov8865_3264x2448_video_tab},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}
};

LOCAL unsigned long _ov8865_set_video_mode(unsigned long param)
{

	return 0;
}


LOCAL SENSOR_IOCTL_FUNC_TAB_T s_ov8865_ioctl_func_tab = {
	PNULL,
	_ov8865_PowerOn,
	PNULL,
	_ov8865_Identify,

	PNULL,			// write register
	PNULL,			// read  register
	PNULL,
	_ov8865_GetResolutionTrimTab,

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

	_ov8865_BeforeSnapshot,
	_ov8865_after_snapshot,
	_ov8865_flash,
	PNULL,
	_ov8865_write_exposure,
	PNULL,
	_ov8865_write_gain,
	PNULL,
	PNULL,
	_ov8865_write_af,
	PNULL,
	PNULL, //_ovov8865_set_awb,
	PNULL,
	PNULL,
	PNULL, //_ov8865_set_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL, //_ov8865_GetExifInfo,
	_ov8865_ExtFunc,
	PNULL, //_ov8865_set_anti_flicker,
#ifdef CONFIG_CAMERA_SENSOR_NEW_FEATURE
	_ov8865_set_video_mode,
#else
	PNULL,
#endif
	PNULL, //pick_jpeg_stream
	PNULL,  //meter_mode
	PNULL, //get_status
	_ov8865_StreamOn,
	_ov8865_StreamOff,
	_ov8865_cfg_otp,
};


SENSOR_INFO_T g_ov8865_mipi_raw_info = {
	ov8865_I2C_ADDR_W,	// salve i2c write address
	ov8865_I2C_ADDR_R,	// salve i2c read address

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
	{{0x30B, 0x88},// supply two code to identify sensor.
	 {0x30c, 0x65}},		// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,	// voltage of avdd

	3264,			// max width of source image
	2448,			// max height of source image
	"ov8865",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_B,// pattern of input image form sensor;

	s_ov8865_resolution_Tab_RAW,	// point to resolution table information structure
	&s_ov8865_ioctl_func_tab,	// point to ioctl function table
	&s_ov8865_mipi_raw_info_ptr,		// information and table about Rawrgb sensor
	NULL,			//&g_ov8825_ext_info,                // extend information about sensor
	SENSOR_AVDD_1800MV,	// iovdd
	SENSOR_AVDD_1200MV,	// dvdd
	1,			// skip frame num before preview
	3,			// skip frame num before capture
	0,			// deci frame num during preview
	0,			// deci frame num during video preview

	0,
	0,
	0,
	0,
	0,
#if defined(OV8865_4_LANES)
	{SENSOR_INTERFACE_TYPE_CSI2, 4, 10, 0},
#elif defined(OV8865_2_LANES)
	{SENSOR_INTERFACE_TYPE_CSI2, 2, 10, 0},
#endif
	s_ov8865_video_info,
	3,			// skip frame num while change setting
};

LOCAL struct sensor_raw_info* Sensor_GetContext(void)
{
	return s_ov8865_mipi_raw_info_ptr;
}

LOCAL uint32_t Sensor_ov8865_InitRawTuneInfo(void)
{
	uint32_t rtn=0x00;
	struct sensor_raw_info* raw_sensor_ptr=Sensor_GetContext();
	struct sensor_raw_tune_info* sensor_ptr=raw_sensor_ptr->tune_ptr;
	struct sensor_raw_cali_info* cali_ptr=raw_sensor_ptr->cali_ptr;
#if 0
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
	sensor_ptr->ae.smart_edge_max_index=7;

	sensor_ptr->af.max_step = 0x3ff;
	sensor_ptr->af.min_step = 0;
	sensor_ptr->af.stab_period = 100;
	sensor_ptr->af.alg_id = 2;
	sensor_ptr->af.rough_count = 17;
	sensor_ptr->af.af_rough_step[0] = 0;
	sensor_ptr->af.af_rough_step[1] = 64;
	sensor_ptr->af.af_rough_step[2] = 128;
	sensor_ptr->af.af_rough_step[3] = 192;
	sensor_ptr->af.af_rough_step[4] = 256;
	sensor_ptr->af.af_rough_step[5] = 320;
	sensor_ptr->af.af_rough_step[6] = 384;
	sensor_ptr->af.af_rough_step[7] = 448;
	sensor_ptr->af.af_rough_step[8] = 512;
	sensor_ptr->af.af_rough_step[9] = 576;
	sensor_ptr->af.af_rough_step[10] = 640;
	sensor_ptr->af.af_rough_step[11] = 704;
	sensor_ptr->af.af_rough_step[12] = 768;
	sensor_ptr->af.af_rough_step[13] = 832;
	sensor_ptr->af.af_rough_step[14] = 896;
	sensor_ptr->af.af_rough_step[15] = 960;
	sensor_ptr->af.af_rough_step[16] = 1023;

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


LOCAL unsigned long _ov8865_GetResolutionTrimTab(unsigned long param)
{
	SENSOR_PRINT("0x%lx",  (unsigned long)s_ov8865_Resolution_Trim_Tab);
	return (unsigned long) s_ov8865_Resolution_Trim_Tab;
}
LOCAL unsigned long _ov8865_PowerOn(unsigned long power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_ov8865_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_ov8865_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_ov8865_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_ov8865_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_ov8865_mipi_raw_info.reset_pulse_level;

	if (SENSOR_TRUE == power_on) {
		Sensor_PowerDown(power_down);
		// Open power
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		usleep(20*1000);
		_ov8865_dw9714_SRCInit(2);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(10*1000);
		Sensor_PowerDown(!power_down);
		usleep(10*1000);
		// Reset sensor
		Sensor_Reset(reset_level);
		usleep(20*1000);
	} else {
		Sensor_PowerDown(power_down);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
	}
	SENSOR_PRINT("SENSOR_ov8865: _ov8865_Power_On(1:on, 0:off): %ld", power_on);
	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov8865_cfg_otp(unsigned long  param)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_ov8865_raw_param_tab;
	uint32_t module_id = g_module_id;
	uint16_t stream_value = 0;

	SENSOR_PRINT("SENSOR_OV8865: _ov8865_cfg_otp");

	stream_value = Sensor_ReadReg(0x0100);
	SENSOR_PRINT("_ov8865_cfg_otp: stream_value = 0x%x\n", stream_value);
	if(1 != (stream_value & 0x01)) {
		Sensor_WriteReg(0x0100, 0x01);
		usleep(5 * 1000);
	}

	if (PNULL!=tab_ptr[module_id].cfg_otp) {
		tab_ptr[module_id].cfg_otp(0);
	}

	if(1 != (stream_value & 0x01)) {
		Sensor_WriteReg(0x0100, stream_value);
		usleep(5 * 1000);
	}

	return rtn;
}

LOCAL int32_t _ov8865_check_o_film_otp(int32_t cmd, int32_t index)
{
	int32_t rtn_index = 0;
	int32_t flag = 0x00, i;
	int32_t address_start = 0x00;
	int32_t address_end = 0x00;

	switch (cmd)
	{
		case 0://for check otp info
		{
			address_start = 0x7010;
			address_end = 0x7010;
			/* read otp into buffer*/
			Sensor_WriteReg(0x3d84, 0xc0);

			/* program disable, manual mode*/
			/*partial mode OTP write start address*/
			Sensor_WriteReg(0x3d88, (address_start>>8));
			Sensor_WriteReg(0x3d89, (address_start & 0xff));
			/*partial mode OTP write end address*/
			Sensor_WriteReg(0x3d8A, (address_end>>8));
			Sensor_WriteReg(0x3d8B, (address_end & 0xff));
			Sensor_WriteReg(0x3d81, 0x01);

			/*read otp*/
			usleep(5*1000);
			flag = Sensor_ReadReg(0x7010);
		}
		break;

		case 1://for check opt wb
		{
			address_start = 0x7020;
			address_end = 0x7020;
			/*read otp into buffer*/
			Sensor_WriteReg(0x3d84, 0xc0);

			/* program disable, manual mode*/
			/*partial mode OTP write start address*/
			Sensor_WriteReg(0x3d88, (address_start>>8));
			Sensor_WriteReg(0x3d89, (address_start & 0xff));

			/*partial mode OTP write end address*/
			Sensor_WriteReg(0x3d8A, (address_end>>8));
			Sensor_WriteReg(0x3d8B, (address_end & 0xff));
			Sensor_WriteReg(0x3d81, 0x01);
			usleep(5*1000);
			/*read OTP; select group*/
			flag = Sensor_ReadReg(0x7020);
		}
		break;

		case 2://for check otp lenc
		{
			address_start = 0x703a;
			address_end = 0x703a;
			/* read otp into buffer*/
			Sensor_WriteReg(0x3d84, 0xc0);
			/*
			program disable, manual mode
			partial mode OTP write start address
			*/
			Sensor_WriteReg(0x3d88, (address_start>>8));
			Sensor_WriteReg(0x3d89, (address_start & 0xff));
			/*partial mode OTP write end address*/
			Sensor_WriteReg(0x3d8A, (address_end>>8));
			Sensor_WriteReg(0x3d8B, (address_end & 0xff));
			Sensor_WriteReg(0x3d81, 0x01);
			usleep(5*1000);
			flag = Sensor_ReadReg(0x703a);
		}
		break;

		case 3:
		{
			address_start = 0x7030;
			address_end = 0x7030;
			// read otp into buffer
			Sensor_WriteReg(0x3d84, 0xc0);
			// program disable, manual mode
			//partial mode OTP write start address
			Sensor_WriteReg(0x3d88, (address_start>>8));
			Sensor_WriteReg(0x3d89, (address_start & 0xff));
			// partial mode OTP write end address
			Sensor_WriteReg(0x3d8A, (address_end>>8));
			Sensor_WriteReg(0x3d8B, (address_end & 0xff));
			Sensor_WriteReg(0x3d81, 0x01);
			usleep(5*1000);
			//select group
			flag = Sensor_ReadReg(0x7030);
		}
		break;

		default:
		break;
	}

	if(index==1) {
		flag = (flag>>6) & 0x03;
	}else if (index==2) {
		flag = (flag>>4) & 0x03;
	} else {
		flag =( flag>>2) & 0x03;
	}

	/* clear otp buffer*/
	for (i=address_start;i<=address_end;i++) {
		Sensor_WriteReg(i, 0x00);
	}
	if (flag == 0x00) {
		rtn_index = 0;
	} else if (flag & 0x02) {
		rtn_index = 1;
	} else {
		rtn_index = 2;
	}

	return rtn_index;
}

LOCAL int32_t _ov8865_read_o_film_otp(int32_t cmd, int32_t index, struct ov8865_otp_info *otp_ptr)
{
	int32_t i = 0x00;
	int32_t address_start = 0x00;
	int32_t address_end = 0x00;
	int32_t temp = 0;

	/*read otp into buffer*/
	Sensor_WriteReg(0x3d84, 0xc0);

	switch (cmd)
	{
		case 0:/*for read otp info*/
		{
			/*program disable, manual mode; select group*/
			if (index==1) {
				address_start = 0x7011;
				address_end = 0x7015;
			}
			else if (index==2) {
				address_start = 0x7016;
				address_end = 0x701a;
			} else {
				address_start = 0x701b;
				address_end = 0x701f;
			}

			/*partial mode OTP write start address*/
			Sensor_WriteReg(0x3d88, (address_start>>8));
			Sensor_WriteReg(0x3d89, (address_start & 0xff));

			/*partial mode OTP write end address*/
			Sensor_WriteReg(0x3d8A, (address_end>>8));
			Sensor_WriteReg(0x3d8B, (address_end & 0xff));
			Sensor_WriteReg(0x3d81, 0x01);
			usleep(5*1000);
			/*load otp into buffer*/
			(*otp_ptr).module_integrator_id = Sensor_ReadReg(address_start);
			(*otp_ptr).lens_id = Sensor_ReadReg(address_start + 1);
			(*otp_ptr).production_year = Sensor_ReadReg(address_start + 2);
			(*otp_ptr).production_month = Sensor_ReadReg(address_start + 3);
			(*otp_ptr).production_day = Sensor_ReadReg(address_start + 4);
		}
		break;

		case 1:/*for read otp wb*/
		{
			/* program disable, manual mode select group*/
			if (index==1) {
			address_start = 0x7021;
			address_end = 0x7025;
			}
			else if (index==2) {
			address_start = 0x7026;
			address_end = 0x702a;
			}
			else {
			address_start = 0x702b;
			address_end = 0x702f;
			}

			/*partial mode OTP write start address*/
			Sensor_WriteReg(0x3d88, (address_start>>8));
			Sensor_WriteReg(0x3d89, (address_start & 0xff));

			/*partial mode OTP write end address*/
			Sensor_WriteReg(0x3d8A, (address_end>>8));
			Sensor_WriteReg(0x3d8B, (address_end & 0xff));
			Sensor_WriteReg(0x3d81, 0x01);
			usleep(5*1000);
			/*load otp into buffer*/
			temp = Sensor_ReadReg(address_start + 4);
			(*otp_ptr).rg_ratio = (Sensor_ReadReg(address_start )<<2) + ((temp>>6) & 0x03);
			(*otp_ptr).bg_ratio = (Sensor_ReadReg(address_start + 1)<<2) + ((temp>>4) & 0x03);
			(*otp_ptr).light_rg = (Sensor_ReadReg(address_start + 2) <<2) + ((temp>>2) & 0x03);
			(*otp_ptr).light_bg = (Sensor_ReadReg(address_start + 3)<<2) + (temp & 0x03);
		}
		break;

		case 2:/*for read otp lenc*/
		{
			/* program disable, manual mode; select group*/
			if (index==1) {
				address_start = 0x703b;
				address_end = 0x7078;
			}
			else if (index==2) {
				address_start = 0x7079;
				address_end = 0x70b6;
			}
			else if (index==3) {
				address_start = 0x70B7;
				address_end = 0x70f4;
			}
			/*partial mode OTP write start address*/
			Sensor_WriteReg(0x3d88, (address_start>>8));
			Sensor_WriteReg(0x3d89, (address_start & 0xff));
			/*partial mode OTP write end address*/
			Sensor_WriteReg(0x3d8A, (address_end>>8));
			Sensor_WriteReg(0x3d8B, (address_end & 0xff));
			Sensor_WriteReg(0x3d81, 0x01);
			usleep(5*1000);
			/* load otp into buffer */
			for(i=0; i<62; i++) {
				(* otp_ptr).lenc[i]=Sensor_ReadReg(address_start + i);
			}
		}
		break;

		case 3:
		{
			// program disable, manual mode
			//check group
			if(index==1){
				address_start = 0x7031;
				address_end = 0x7033;
			} else if(index==2) {
				address_start = 0x7034;
				address_end = 0x7036;
			} else {
				address_start = 0x7037;
				address_end = 0x7039;
			}
			//partial mode OTP write start address
			Sensor_WriteReg(0x3d88, (address_start>>8));
			Sensor_WriteReg(0x3d89, (address_start & 0xff));
			// partial mode OTP write end address
			Sensor_WriteReg(0x3d8A, (address_end>>8));
			Sensor_WriteReg(0x3d8B, (address_end & 0xff));
			Sensor_WriteReg(0x3d81, 0x01);
			usleep(5*1000);
			// load otp into buffer
			//flag and lsb of VCM start code
			temp = Sensor_ReadReg(address_start + 2);
			(* otp_ptr).VCM_start = (Sensor_ReadReg(address_start)<<2) | ((temp>>6) & 0x03);
			(* otp_ptr).VCM_end = (Sensor_ReadReg(address_start + 1) << 2) | ((temp>>4) & 0x03);
			(* otp_ptr).VCM_dir = (temp>>2) & 0x03;
		}
		break;

		default:
		break;
	}

	/* clear otp buffer */
	for (i=address_start; i<=address_end; i++) {
		Sensor_WriteReg(i, 0x00);
	}

	return 0;
}

/*
call this function after OV8865 initialization
return value: 0 update success; 1, no OTP
*/
LOCAL int32_t _ov8865_update_o_film_otp_wb(void)
{
	struct ov8865_otp_info current_otp;
	int32_t i;
	int32_t otp_index;/*bank 1,2,3*/
	int32_t temp, rg, bg;
	int32_t R_gain, G_gain, B_gain, G_gain_R, G_gain_B;
	int32_t BG_Ratio_Typical = BG_RATIO_TYPICAL_OFILM;
	int32_t RG_Ratio_Typical = RG_RATIO_TYPICAL_OFILM;

	/*
	R/G and B/G of current camera module is read out from sensor OTP
	check first OTP with valid data
	*/
	for(i=1;i<4;i++) {
		temp = _ov8865_check_o_film_otp(1, i);
		if (temp == 2) {
			otp_index = i;
			break;
		}
	}
	if (i==4) {
		/* no valid wb OTP data*/
		return 1;
	}

	/*set right bank*/
	_ov8865_read_o_film_otp(1, otp_index, &current_otp);
	if(current_otp.light_rg==0) {
		/*no light source information in OTP, light factor = 1*/
		rg = current_otp.rg_ratio;
	}
	else {
		rg = current_otp.rg_ratio * (current_otp.light_rg + 512) / 1024;
	}
	if(current_otp.light_bg==0) {
		/*not light source information in OTP, light factor = 1*/
		bg = current_otp.bg_ratio;
	}
	else {
		bg = current_otp.bg_ratio * (current_otp.light_bg + 512) / 1024;
	}

	/*calculate G gain
	0x400 = 1x gain*/
	if(bg < BG_Ratio_Typical) {
		if (rg< RG_Ratio_Typical) {
			/*
			current_otp.bg_ratio < BG_Ratio_typical &&
			current_otp.rg_ratio < RG_Ratio_typical
			*/
			G_gain = 0x400;
			B_gain = 0x400 * BG_Ratio_Typical / bg;
			R_gain = 0x400 * RG_Ratio_Typical / rg;
		}
		else {
			/* current_otp.bg_ratio < BG_Ratio_typical &&
			 current_otp.rg_ratio >= RG_Ratio_typical
			 */
			R_gain = 0x400;
			G_gain = 0x400 * rg / RG_Ratio_Typical;
			B_gain = G_gain * BG_Ratio_Typical /bg;
		}
	}
	else {
		if (rg < RG_Ratio_Typical) {
			/* current_otp.bg_ratio >= BG_Ratio_typical &&
			current_otp.rg_ratio < RG_Ratio_typical
			*/
			B_gain = 0x400;
			G_gain = 0x400 * bg / BG_Ratio_Typical;
			R_gain = G_gain * RG_Ratio_Typical / rg;
		}
		else {
			/*
			current_otp.bg_ratio >= BG_Ratio_typical &&
			current_otp.rg_ratio >= RG_Ratio_typical
			*/
			G_gain_B = 0x400 * bg / BG_Ratio_Typical;
			G_gain_R = 0x400 * rg / RG_Ratio_Typical;
			if(G_gain_B > G_gain_R ) {
				B_gain = 0x400;
				G_gain = G_gain_B;
				R_gain = G_gain * RG_Ratio_Typical /rg;
		}
		else {
			R_gain = 0x400;
			G_gain = G_gain_R;
			B_gain = G_gain * BG_Ratio_Typical / bg;
		}
		}
	}

	/*update_awb_gain(R_gain, G_gain, B_gain)
	R_gain, sensor red gain of AWB, 0x400 =1
	G_gain, sensor green gain of AWB, 0x400 =1
	B_gain, sensor blue gain of AWB, 0x400 =1
	*/
	if (R_gain>0x400) {
		Sensor_WriteReg(0x5018, R_gain>>8);
		Sensor_WriteReg(0x5019, R_gain & 0x00ff);
	}
	if (G_gain>0x400) {
		Sensor_WriteReg(0x501A, G_gain>>8);
		Sensor_WriteReg(0x501B, G_gain & 0x00ff);
	}
	if (B_gain>0x400) {
		Sensor_WriteReg(0x501C, B_gain>>8);
		Sensor_WriteReg(0x501D, B_gain & 0x00ff);
	}

	return 0;
}

/* call this function after OV8865 initialization
return value: 0 update success*/
LOCAL int32_t _ov8865_update_o_film_otp_lenc(void)
{
	struct ov8865_otp_info current_otp;
	int32_t i;
	int32_t otp_index;
	int32_t temp;

	/* check first lens correction OTP with valid data */
	for (i=1;i<=3;i++) {
		temp = _ov8865_check_o_film_otp(2, i);
		if (temp == 2) {
			otp_index = i;
			break;
		}
	}

	if (i>3) {
		/* no valid WB OTP data */
		return 1;
	}
	_ov8865_read_o_film_otp(2, otp_index, &current_otp);

	/*update_lenc*/
	temp = Sensor_ReadReg(0x5000);
	temp = 0x80 | temp;
	Sensor_WriteReg(0x5000, temp);
	for(i=0;i<62;i++) {
		Sensor_WriteReg(0x5800 + i, current_otp.lenc[i]);
	}

	return 0;
}

LOCAL uint32_t _ov8865_o_film_Identify_otp(void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;
	struct ov8865_otp_info current_otp;
	int32_t i;
	int32_t otp_index;
	int32_t temp;
	int32_t ret;

	SENSOR_PRINT("SENSOR_ov8865: _ov8865_o_film_Identify_otp");

	/*read param id from sensor omap*/
	for (i=1;i<=3;i++) {
		temp = _ov8865_check_o_film_otp(0, i);
		SENSOR_PRINT("_ov8865_o_film_Identify_otp i=%d temp = %d \n",i,temp);
		if (temp == 2) {
			otp_index = i;
			break;
		}
	}

	if (i <= 3) {
		_ov8865_read_o_film_otp(0, otp_index, &current_otp);
		SENSOR_PRINT("read ov8865 otp  module_id = %x \n", current_otp.module_integrator_id);
		if (OV8865_RAW_PARAM_O_FILM == current_otp.module_integrator_id) {
			SENSOR_PRINT("SENSOR_OV8865: This is o_film module!!\n");
			rtn = SENSOR_SUCCESS;
		} else {
			SENSOR_PRINT("SENSOR_OV8865: check module id faided!!\n");
			rtn = SENSOR_FAIL;
		}
	} else {
		/* no valid wb OTP data */
		SENSOR_PRINT("ov8865_check_otp_module_id no valid wb OTP data\n");
		rtn = SENSOR_FAIL;
	}

	return rtn;
}

LOCAL uint32_t  _ov8865_o_film_update_otp(void* param_ptr)
{

	SENSOR_PRINT("SENSOR_OV8865: _ov8865_o_film_update_otp");

	//_ov8865_update_o_film_otp_wb();
	_ov8865_update_o_film_otp_lenc();

	return 0;
}

LOCAL uint32_t _ov8865_com_Identify_otp(void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_OV8865: _ov8865_com_Identify_otp");

	/*read param id from sensor omap*/
	param_id=OV8865_RAW_PARAM_COM;

	if(OV8865_RAW_PARAM_COM==param_id){
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}

LOCAL uint32_t _ov8865_GetRawInof(void)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_ov8865_raw_param_tab;
	uint32_t i=0x00;
	uint16_t stream_value = 0;

	stream_value = Sensor_ReadReg(0x0100);
	SENSOR_PRINT("_ov8865_GetRawInof:stream_value = 0x%x\n", stream_value);
	if (1 != (stream_value & 0x01)) {
		Sensor_WriteReg(0x0100, 0x01);
		usleep(5 * 1000);
	}

	for (i=0x00; ; i++) {
		g_module_id = i;
		if(RAW_INFO_END_ID==tab_ptr[i].param_id) {
			if(NULL==s_ov8865_mipi_raw_info_ptr) {
				SENSOR_PRINT("SENSOR_OV8865: ov8865_GetRawInof no param error");
				rtn=SENSOR_FAIL;
			}
			SENSOR_PRINT("SENSOR_OV8865: ov8865_GetRawInof end");
			break;
		}
		else if (PNULL!=tab_ptr[i].identify_otp) {
			if (SENSOR_SUCCESS==tab_ptr[i].identify_otp(0))
			{
				s_ov8865_mipi_raw_info_ptr = tab_ptr[i].info_ptr;
				SENSOR_PRINT("SENSOR_OV8865: ov8865_GetRawInof success");
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

LOCAL unsigned long _ov8865_GetMaxFrameLine(unsigned long index)
{
	uint32_t max_line=0x00;
	SENSOR_TRIM_T_PTR trim_ptr=s_ov8865_Resolution_Trim_Tab;

	max_line=trim_ptr[index].frame_line;

	return max_line;
}

LOCAL unsigned long _ov8865_Identify(unsigned long param)
{
#define ov8865_pid_val		(0x0088)
#define ov8865_ver_val		(0x65)
#define ov8865_pid_h_addr	(0x300a)
#define ov8865_pid_l_addr	(0x300b)
#define ov8865_ver_addr		(0x300c)

	uint8_t pid_h_value = 0x00;
	uint8_t pid_l_value = 0x00;
	uint8_t pid_value = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("SENSOR_ov8865: mipi raw identify\n");

	pid_h_value = Sensor_ReadReg(ov8865_pid_h_addr);
	pid_l_value = Sensor_ReadReg(ov8865_pid_l_addr);
	pid_value = (pid_h_value<<8)|pid_l_value;
	if (ov8865_pid_val == pid_value) {
		ver_value = Sensor_ReadReg(ov8865_ver_addr);
		SENSOR_PRINT("SENSOR_ov8865: Identify: PID = %x, VER = %x", pid_value, ver_value);
		if (ov8865_ver_val == ver_value) {
			SENSOR_PRINT("SENSOR_ov8865: this is ov8865 sensor !");
			ret_value=_ov8865_GetRawInof();
			if(SENSOR_SUCCESS != ret_value)
			{
				SENSOR_PRINT("SENSOR_ov8865: the module is unknow error !");
			}
			Sensor_ov8865_InitRawTuneInfo();
		} else {
			SENSOR_PRINT("SENSOR_ov8865: Identify this is OV%x%x sensor !", pid_value, ver_value);
		}
	} else {
		SENSOR_PRINT("SENSOR_ov8865: identify fail,pid_value=%d", pid_value);
	}

	return ret_value;
}

LOCAL unsigned long _ov8865_write_exposure(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t expsure_line=0x00;
	uint16_t dummy_line=0x00;
	uint16_t size_index=0x00;
	uint16_t frame_len=0x00;
	uint16_t frame_len_cur=0x00;
	uint16_t max_frame_len=0x00;
	uint16_t value=0x00;

	expsure_line=param&0xffff;
	dummy_line=(param>>0x10)&0x0fff;
	size_index=(param>>0x1c)&0x0f;

	SENSOR_PRINT("SENSOR_ov8865: write_exposure line:%d, dummy:%d, size_index:%d", expsure_line, dummy_line, size_index);

	max_frame_len=_ov8865_GetMaxFrameLine(size_index);

	if(0x00!=max_frame_len)
	{
		frame_len = ((expsure_line+4)> max_frame_len) ? (expsure_line+4) : max_frame_len;

		frame_len_cur = (Sensor_ReadReg(0x380e)&0xff)<<8;
		frame_len_cur |= Sensor_ReadReg(0x380f)&0xff;

		SENSOR_PRINT("SENSOR_ov8865: write_exposure max line:%d, cur frame line:%d", max_frame_len, frame_len_cur);

		if(frame_len_cur != frame_len){
			frame_len = ((frame_len + 1)>>1)<<1;
			value=(frame_len)&0xff;
			ret_value = Sensor_WriteReg(0x380f, value);
			value=(frame_len>>0x08)&0xff;
			ret_value = Sensor_WriteReg(0x380e, value);
		}
	}

	value = (expsure_line & 0xf);
	value = value<<0x04;
	ret_value = Sensor_WriteReg(0x3502, value);

	value = expsure_line & 0xfff;
	value = value >> 4;
	ret_value = Sensor_WriteReg(0x3501, value);

	value = expsure_line>>12;
	ret_value = Sensor_WriteReg(0x3500, value);

	return ret_value;
}

LOCAL unsigned long _ov8865_write_gain(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t gain128 =0x00, tmp_val = 0;

	SENSOR_PRINT("SENSOR_ov8865: write_gain:0x%x", param);

	gain128 = 0x80|((param&0xf)<<3)|((param&0x1f0)<<4);

	tmp_val = gain128>>8;
	ret_value = Sensor_WriteReg(0x3508, tmp_val);

	tmp_val =  gain128 & 0xff;
	ret_value = Sensor_WriteReg(0x3509, tmp_val);

	return ret_value;
}

LOCAL unsigned long _ov8865_write_af(unsigned long param)
{
#define DW9714_VCM_SLAVE_ADDR (0x18>>1)

	uint32_t ret_value = SENSOR_SUCCESS;
	uint8_t cmd_val[2] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;

	SENSOR_PRINT("SENSOR_ov8865: _write_af %d", param);

	slave_addr = DW9714_VCM_SLAVE_ADDR;
	cmd_val[0] = (param&0xfff0)>>4;
	cmd_val[1] = ((param&0x0f)<<4)|0x09;
	cmd_len = 2;
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

	SENSOR_PRINT("SENSOR_ov8865: _write_af, ret =  %d, MSL:%x, LSL:%x\n", ret_value, cmd_val[0], cmd_val[1]);

	return ret_value;
}

LOCAL unsigned long _ov8865_BeforeSnapshot(unsigned long param)
{
	uint8_t ret_l, ret_m, ret_h;
	uint32_t capture_exposure;
	uint32_t capture_maxline, preview_exposure;
	uint32_t capture_mode = param & 0xffff;
	uint32_t preview_mode = (param >> 0x10 ) & 0xffff;
	uint32_t prv_linetime=s_ov8865_Resolution_Trim_Tab[preview_mode].line_time;
	uint32_t cap_linetime = s_ov8865_Resolution_Trim_Tab[capture_mode].line_time;

	SENSOR_PRINT("SENSOR_ov8865: BeforeSnapshot mode: 0x%08x",param);

	if (preview_mode == capture_mode) {
		SENSOR_PRINT("SENSOR_ov8865: prv mode equal to capmode");
		goto CFG_INFO;
	}

	ret_h = (uint8_t) Sensor_ReadReg(0x3500);
	ret_m = (uint8_t) Sensor_ReadReg(0x3501);
	ret_l = (uint8_t) Sensor_ReadReg(0x3502);
	preview_exposure = (ret_h << 12) + (ret_m << 4) + (ret_l >> 4);

	Sensor_SetMode(capture_mode);
	Sensor_SetMode_WaitDone();

	if (prv_linetime == cap_linetime) {
		SENSOR_PRINT("SENSOR_ov8865: prvline equal to capline");
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
	s_capture_shutter = _ov8865_get_shutter();
	s_capture_VTS = _ov8865_get_VTS();
	_ov8865_ReadGain(&capture_mode);
	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, s_capture_shutter);

	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov8865_after_snapshot(unsigned long param)
{
	SENSOR_PRINT("SENSOR_ov8865: after_snapshot mode:%ld", param);

	Sensor_SetMode((uint32_t)param);

	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov8865_flash(unsigned long param)
{
	SENSOR_PRINT("SENSOR_ov8865: param=%d", param);

	/* enable flash, disable in _ov8865_BeforeSnapshot */
	Sensor_SetFlash(param);
	SENSOR_PRINT_HIGH("end");
	return SENSOR_SUCCESS;
}

LOCAL unsigned long _ov8865_StreamOn(unsigned long param)
{
	SENSOR_PRINT("SENSOR_ov8865: StreamOn");

	Sensor_WriteReg(0x0100, 0x01);

	return 0;
}

LOCAL unsigned long _ov8865_StreamOff(unsigned long param)
{
	SENSOR_PRINT("SENSOR_ov8865: StreamOff");

	Sensor_WriteReg(0x0100, 0x00);

	usleep(50*1000);

	return 0;
}

LOCAL uint32_t _ov8865_get_shutter(void)
{
	// read shutter, in number of line period
	uint32_t shutter;

	shutter = (Sensor_ReadReg(0x03500) & 0x0f);
	shutter = (shutter<<8) + Sensor_ReadReg(0x3501);
	shutter = (shutter<<4) + (Sensor_ReadReg(0x3502)>>4);

	return shutter;
}

LOCAL unsigned long _ov8865_set_shutter(unsigned long shutter)
{
	// write shutter, in number of line period
	uint32_t temp;

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

LOCAL uint32_t _ov8865_get_gain128(void)
{
	// read gain, 128 = 1x
	uint32_t gain128, value;

	value = Sensor_ReadReg(0x3509);
	gain128 = value&0xff;
	value = Sensor_ReadReg(0x3508);
	gain128 = gain128 + ((value&0x1f)<<0x08);

	return gain128;
}

LOCAL unsigned long _ov8865_set_gain128(unsigned long gain128)
{
	// write gain, 128 = 1x
	uint32_t gain_128 = 0x00, temp= 0x00;

	gain_128 = gain128 & 0x1ff;

	temp = gain_128 & 0xff;
	Sensor_WriteReg(0x3509, temp);
	temp = (gain_128>>8) & 0x1f;
	Sensor_WriteReg(0x3508, temp);

	return 0;
}

LOCAL void _calculate_hdr_exposure(int32_t capture_gain128,int32_t capture_VTS, int32_t capture_shutter)
{
	// write capture gain

	_ov8865_set_gain128(capture_gain128);
	_ov8865_set_shutter(capture_shutter);

}

LOCAL unsigned long _ov8865_SetEV(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	uint32_t ev ;
	uint32_t real_gain_128 = s_ov8865_realgain_128;
	uint32_t capture_shutter = s_capture_shutter;
	uint32_t capture_vts = s_capture_VTS;

	SENSOR_PRINT("SENSOR_ov8865: _ov8865_SetEV param: 0x%x", ext_ptr->param);

	ev = ext_ptr->param;
	switch(ev) {
	case SENSOR_HDR_EV_LEVE_0:
		real_gain_128 = s_ov8865_realgain_128 / 2;
		capture_shutter = s_capture_shutter;
		_calculate_hdr_exposure(real_gain_128, capture_vts, capture_shutter);
		break;
	case SENSOR_HDR_EV_LEVE_1:
		real_gain_128 = s_ov8865_realgain_128;
		capture_shutter = s_capture_shutter;
		_calculate_hdr_exposure(real_gain_128, capture_vts, capture_shutter);
		break;
	case SENSOR_HDR_EV_LEVE_2:
		real_gain_128 = s_ov8865_realgain_128;
		capture_shutter = s_capture_shutter * 4;
		_calculate_hdr_exposure(real_gain_128, capture_vts, capture_shutter);
		break;
	default:
		SENSOR_PRINT_ERR("SENSOR_ov8865: _ov8865_SetEV ev is invalidated: 0x%x\n", ev);
		rtn = SENSOR_FALSE;
		break;
	}

	return rtn;
}
LOCAL unsigned long _ov8865_ExtFunc(unsigned long ctl_param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr =
	    (SENSOR_EXT_FUN_PARAM_T_PTR) ctl_param;

	if (PNULL == ext_ptr) {

		SENSOR_PRINT_ERR("SENSOR_ov8865: _ov8865_ExtFunc -- ctl_param is NULL\n");
		return SENSOR_FALSE;
	}

	SENSOR_PRINT_HIGH("0x%x", ext_ptr->cmd);

	switch (ext_ptr->cmd) {
	case SENSOR_EXT_FUNC_INIT:
		break;
	case SENSOR_EXT_FOCUS_START:
		break;
	case SENSOR_EXT_EXPOSURE_START:
		break;
	case SENSOR_EXT_EV:
		rtn = _ov8865_SetEV(ctl_param);
		break;
	default:
		SENSOR_PRINT_ERR("SENSOR_ov8865: _ov8865_ExtFunc -- cmd is invalidated: 0x%x\n", ext_ptr->cmd);
		rtn = SENSOR_FALSE;
		break;
	}

	return rtn;

}
LOCAL uint32_t _ov8865_get_VTS(void)
{
	// read VTS from register settings
	uint32_t VTS;

	VTS = Sensor_ReadReg(0x380e);//total vertical size[15:8] high byte

	VTS = (VTS<<8) + Sensor_ReadReg(0x380f);

	return VTS;
}

LOCAL unsigned long _ov8865_set_VTS(unsigned long VTS)
{
	// write VTS to registers
	uint32_t temp;

	VTS = ((VTS + 1)>>1)<<1;
	temp = VTS & 0xff;
	Sensor_WriteReg(0x380f, temp);

	temp = VTS>>8;
	Sensor_WriteReg(0x380e, temp);

	return 0;
}

LOCAL uint32_t _ov8865_ReadGain(uint32_t *real_gain)
{
	uint32_t rtn = SENSOR_SUCCESS;
	uint32_t gain128, value;

	value = Sensor_ReadReg(0x3509);
	gain128 = value&0xff;
	value = Sensor_ReadReg(0x3508);
	gain128 = gain128 + ((value&0x1f)<<0x08);

	s_ov8865_realgain_128 = (uint32_t)gain128;
	if (real_gain) {
		*real_gain = gain128;
	}

	SENSOR_PRINT("SENSOR_ov8865: _ov8865_ReadGain gain: 0x%x", gain128);

	return rtn;
}

LOCAL unsigned long _ov8865_dw9714_SRCInit(unsigned long mode)
{
#define DW9714_VCM_SLAVE_ADDR (0x18>>1)

	uint8_t cmd_val[2] = {0x00};
	uint16_t  slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t ret_value = SENSOR_SUCCESS;

	slave_addr = DW9714_VCM_SLAVE_ADDR;
	switch (mode) {
		case 1:
		break;

		case 2:
		{
			cmd_len = 2;
			cmd_val[0] = 0xec;
			cmd_val[1] = 0xa3;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
			if(ret_value){
				SENSOR_PRINT("SENSOR_OV8865: _dw9174_SRCInit fail!1");
			}
			cmd_val[0] = 0xf2;
			cmd_val[1] = 0x00;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
			if(ret_value){
				SENSOR_PRINT("SENSOR_OV8865: _dw9174_SRCInit fail!2");
			}

			cmd_val[0] = 0xdc;
			cmd_val[1] = 0x51;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
			if(ret_value){
				SENSOR_PRINT("SENSOR_OV8865: _dw9174_SRCInit fail!3");
			}
		}
		break;

		case 3:
		break;

	}

	return ret_value;
}



