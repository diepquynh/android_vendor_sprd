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
#ifdef	 __cplusplus
	extern	 "C"
	{
#endif

#define S5K4EC_I2C_ADDR_W        0xac>>1//0x5A//0x5A-0101 1010->0010 1101
#define S5K4EC_I2C_ADDR_R        0xac>>1//0x5A//0x5B-0101 1011->0010 1101

#define FOCUS_ZONE_W 80
#define FOCUS_ZONE_H 60

#define EXPOSURE_ZONE_W 1280
#define EXPOSURE_ZONE_H 960

static uint32_t  g_flash_mode_en = 0;
static uint32_t is_cap = 0;

LOCAL uint32_t _s5k4ec_InitExifInfo(void);
LOCAL unsigned long _s5k4ec_GetResolutionTrimTab(unsigned long param);
LOCAL unsigned long _s5k4ec_PowerOn(unsigned long power_on);
LOCAL unsigned long _s5k4ec_Identify(unsigned long param);
LOCAL unsigned long _s5k4ec_set_brightness(unsigned long level);
LOCAL unsigned long _s5k4ec_set_contrast(unsigned long level);
//LOCAL uint32_t _s5k4ec_set_sharpness(uint32_t level);
LOCAL unsigned long _s5k4ec_set_saturation(unsigned long level);
LOCAL unsigned long _s5k4ec_set_image_effect(unsigned long effect_type);
LOCAL unsigned long _s5k4ec_set_ev(unsigned long level);
LOCAL unsigned long _s5k4ec_set_anti_flicker(unsigned long mode);
LOCAL unsigned long _s5k4ec_set_video_mode(unsigned long mode);
LOCAL unsigned long _s5k4ec_set_awb(unsigned long mode);
LOCAL unsigned long _s5k4ec_set_work_mode(unsigned long mode);
LOCAL unsigned long _s5k4ec_BeforeSnapshot(unsigned long param);
LOCAL unsigned long _s5k4ec_check_image_format_support(unsigned long param);
LOCAL unsigned long _s5k4ec_pick_out_jpeg_stream(unsigned long param);
LOCAL unsigned long _s5k4ec_after_snapshot(unsigned long param);
LOCAL unsigned long _s5k4ec_flash(unsigned long param);
LOCAL unsigned long _s5k4ec_GetExifInfo(unsigned long param);
LOCAL unsigned long _s5k4ec_ExtFunc(unsigned long ctl_param);
LOCAL unsigned long _s5k4ec_StreamOn(unsigned long param);
LOCAL unsigned long _s5k4ec_set_iso(unsigned long level);
LOCAL unsigned long _s5k4ec_StreamOff(unsigned long param);
LOCAL uint32_t _s5k4ec_recovery_init();

LOCAL const SENSOR_REG_T s5k4ec_common_init[]=
{
//===============================================================
// 02.ETC Setting
//==============================================================

	{0x0028, 0xD000}, //02.ETC Setting
	{0x002A, 0x1082},
	{0x0F12, 0x0155},
	{0x002A, 0x1084},
	{0x0F12, 0x0155},
	{0x002A, 0x1086},
	{0x0F12, 0x0055},
	{0x002A, 0x1088},
	{0x0F12, 0x0555},
	{0x002A, 0x100E},
	{0x0F12, 0x0000},


//==================================================================================
// 03.Analog Setting1 & ASP Control
//==================================================================================
//This register is for FACTORY ONLY.
//If you change it without prior notification
// YOU are RESPONSIBLE for the FAILURE that will happen in the future.

	{0x0028, 0xD000}, // 03.Analog Setting & ASP Control-1
	{0x002A, 0x007A},
	{0x0F12, 0x0000},	//config_clk_setting
	{0x002A, 0xE406},
	{0x0F12, 0x0092},	//adlc_enable
	{0x002A, 0xE410},
	{0x0F12, 0x3804},	//adlc_fadlc_filter_co
	{0x002A, 0xE41A},
	{0x0F12, 0x0010},	//adlc_ptune_total
	{0x002A, 0xF132},
	{0x0F12, 0x0200},
	{0x002A, 0xF142},
	{0x0F12, 0x0200}, //110404 AE haunting - from_LSI
	{0x002A, 0xE420},
	{0x0F12, 0x0003}, //adlc_fadlc_filter_refresh
	{0x0F12, 0x0060},	//adlc_filter_level_diff_threshold
	{0x002A, 0xE42E},
	{0x0F12, 0x0004},	//adlc_qec
	{0x002A, 0xF400},
	{0x0F12, 0x5A3C}, //aig_shutter_width
	{0x0F12, 0x0023}, //aig_cds_tune
	{0x0F12, 0x8080}, //aig_cds_option
	{0x0F12, 0x03AF}, //aig_mx
	{0x0F12, 0x000A}, //aig_mode_en
	{0x0F12, 0xAA54}, //aig_ms
	{0x0F12, 0x0040}, //aig_rmp_tune_1
	{0x0F12, 0x464E}, //aig_rmp_tune_2
	{0x0F12, 0x0240}, //aig_bist_sig_width_e
	{0x0F12, 0x0240}, //aig_bist_sig_width_o
	{0x0F12, 0x0040}, //aig_dbs_bist
	{0x0F12, 0x1000}, //aig_dbs_tune
	{0x0F12, 0x55FF}, //aig_bias_tune
	{0x0F12, 0xD000}, //aig_ref_tune_1
	{0x0F12, 0x0010}, //aig_ref_tune_2
	{0x0F12, 0x0202}, //aig_reg_tune_1
	{0x0F12, 0x0401}, //aig_reg_tune_2
	{0x0F12, 0x0022}, //aig_rosc_tune
	{0x0F12, 0x0088}, //aig_dbr_tune_1
	{0x0F12, 0x009F}, //aig_dbr_tune_2
	{0x0F12, 0x0000}, //aig_bist_en_cintr
	{0x0F12, 0x1800}, //aig_vdec_tune
	{0x0F12, 0x0088}, //aig_pmg_reg_tune
	{0x0F12, 0x0000}, //aig_pmg_tune_1
	{0x0F12, 0x2428}, //aig_shutter_gap
	{0x0F12, 0x0000}, //aig_atx_option
	{0x0F12, 0x03EE}, //aig_avg_half
	{0x0F12, 0x0000}, //aig_hvs_test_reg
	{0x0F12, 0x0000}, //aig_dbus_bist_auto
	{0x0F12, 0x0000},	//aig_dbr_option
	{0x002A, 0xF552},
	{0x0F12, 0x0708}, //aig_1h_time_1
	{0x0F12, 0x080C},	//aig_1h_time_2



//===================================================================
// Trap & Patch
//===================================================================

// TnP setting
// Start of Patch data
	{0x0028, 0x7000}, // 05.Trap and Patch
	{0x002A, 0x3AF8},
	{0x0F12, 0xB5F8},
	{0x0F12, 0x4B44},
	{0x0F12, 0x4944},
	{0x0F12, 0x4845},
	{0x0F12, 0x2200},
	{0x0F12, 0xC008},
	{0x0F12, 0x6001},
	{0x0F12, 0x4944},
	{0x0F12, 0x4844},
	{0x0F12, 0x2401},
	{0x0F12, 0xF000},
	{0x0F12, 0xFCA4},
	{0x0F12, 0x4943},
	{0x0F12, 0x4844},
	{0x0F12, 0x2702},
	{0x0F12, 0x0022},
	{0x0F12, 0xF000},
	{0x0F12, 0xFC9E},
	{0x0F12, 0x0260},
	{0x0F12, 0x4C42},
	{0x0F12, 0x8020},
	{0x0F12, 0x2600},
	{0x0F12, 0x8066},
	{0x0F12, 0x4941},
	{0x0F12, 0x4841},
	{0x0F12, 0x6041},
	{0x0F12, 0x4941},
	{0x0F12, 0x4842},
	{0x0F12, 0x003A},
	{0x0F12, 0x2503},
	{0x0F12, 0xF000},
	{0x0F12, 0xFC90},
	{0x0F12, 0x483D},
	{0x0F12, 0x4940},
	{0x0F12, 0x30C0},
	{0x0F12, 0x63C1},
	{0x0F12, 0x4F3B},
	{0x0F12, 0x483F},
	{0x0F12, 0x3F80},
	{0x0F12, 0x6438},
	{0x0F12, 0x483E},
	{0x0F12, 0x493F},
	{0x0F12, 0x6388},
	{0x0F12, 0x002A},
	{0x0F12, 0x493E},
	{0x0F12, 0x483F},
	{0x0F12, 0x2504},
	{0x0F12, 0xF000},
	{0x0F12, 0xFC7F},
	{0x0F12, 0x002A},
	{0x0F12, 0x493D},
	{0x0F12, 0x483E},
	{0x0F12, 0x2505},
	{0x0F12, 0xF000},
	{0x0F12, 0xF8A7},
	{0x0F12, 0x483C},
	{0x0F12, 0x002A},
	{0x0F12, 0x493C},
	{0x0F12, 0x2506},
	{0x0F12, 0x1D80},
	{0x0F12, 0xF000},
	{0x0F12, 0xF8A0},
	{0x0F12, 0x4838},
	{0x0F12, 0x002A},
	{0x0F12, 0x4939},
	{0x0F12, 0x2507},
	{0x0F12, 0x300C},
	{0x0F12, 0xF000},
	{0x0F12, 0xF899},
	{0x0F12, 0x4835},
	{0x0F12, 0x002A},
	{0x0F12, 0x4937},
	{0x0F12, 0x2508},
	{0x0F12, 0x3010},
	{0x0F12, 0xF000},
	{0x0F12, 0xF892},
	{0x0F12, 0x002A},
	{0x0F12, 0x4935},
	{0x0F12, 0x4835},
	{0x0F12, 0x2509},
	{0x0F12, 0xF000},
	{0x0F12, 0xFC5E},
	{0x0F12, 0x002A},
	{0x0F12, 0x4934},
	{0x0F12, 0x4834},
	{0x0F12, 0x250A},
	{0x0F12, 0xF000},
	{0x0F12, 0xFC58},
	{0x0F12, 0x002A},
	{0x0F12, 0x4933},
	{0x0F12, 0x4833},
	{0x0F12, 0x250B},
	{0x0F12, 0xF000},
	{0x0F12, 0xFC52},
	{0x0F12, 0x002A},
	{0x0F12, 0x4932},
	{0x0F12, 0x4832},
	{0x0F12, 0x250C},
	{0x0F12, 0xF000},
	{0x0F12, 0xFC4C},
	{0x0F12, 0x002A},
	{0x0F12, 0x4931},
	{0x0F12, 0x4831},
	{0x0F12, 0x250D},
	{0x0F12, 0xF000},
	{0x0F12, 0xFC46},
	{0x0F12, 0x002A},
	{0x0F12, 0x4930},
	{0x0F12, 0x4830},
	{0x0F12, 0x250E},
	{0x0F12, 0xF000},
	{0x0F12, 0xFC40},
	{0x0F12, 0x002A},
	{0x0F12, 0x492F},
	{0x0F12, 0x482F},
	{0x0F12, 0x250F},
	{0x0F12, 0xF000},
	{0x0F12, 0xFC3A},
	{0x0F12, 0x8626},
	{0x0F12, 0x20FF},
	{0x0F12, 0x1C40},
	{0x0F12, 0x8660},
	{0x0F12, 0x482C},
	{0x0F12, 0x64F8},
	{0x0F12, 0x492C},
	{0x0F12, 0x482D},
	{0x0F12, 0x2410},
	{0x0F12, 0x002A},
	{0x0F12, 0xF000},
	{0x0F12, 0xFC2E},
	{0x0F12, 0x492B},
	{0x0F12, 0x482C},
	{0x0F12, 0x0022},
	{0x0F12, 0xF000},
	{0x0F12, 0xFC29},
	{0x0F12, 0xBCF8},
	{0x0F12, 0xBC08},
	{0x0F12, 0x4718},
	{0x0F12, 0x019C},
	{0x0F12, 0x4EC2},
	{0x0F12, 0x73FF},
	{0x0F12, 0x0000},
	{0x0F12, 0x1F90},
	{0x0F12, 0x7000},
	{0x0F12, 0x3CCD},
	{0x0F12, 0x7000},
	{0x0F12, 0xE38B},
	{0x0F12, 0x0000},
	{0x0F12, 0x3D05},
	{0x0F12, 0x7000},
	{0x0F12, 0xC3B1},
	{0x0F12, 0x0000},
	{0x0F12, 0x4780},
	{0x0F12, 0x7000},
	{0x0F12, 0x3D63},
	{0x0F12, 0x7000},
	{0x0F12, 0x0080},
	{0x0F12, 0x7000},
	{0x0F12, 0x3D9F},
	{0x0F12, 0x7000},
	{0x0F12, 0xB49D},
	{0x0F12, 0x0000},
	{0x0F12, 0x3E4B},
	{0x0F12, 0x7000},
	{0x0F12, 0x3DFF},
	{0x0F12, 0x7000},
	{0x0F12, 0xFFFF},
	{0x0F12, 0x00FF},
	{0x0F12, 0x17E0},
	{0x0F12, 0x7000},
	{0x0F12, 0x3FC7},
	{0x0F12, 0x7000},
	{0x0F12, 0x053D},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0A89},
	{0x0F12, 0x6CD2},
	{0x0F12, 0x0000},
	{0x0F12, 0x02C9},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0A9A},
	{0x0F12, 0x0000},
	{0x0F12, 0x02D2},
	{0x0F12, 0x4015},
	{0x0F12, 0x7000},
	{0x0F12, 0x9E65},
	{0x0F12, 0x0000},
	{0x0F12, 0x4089},
	{0x0F12, 0x7000},
	{0x0F12, 0x7C49},
	{0x0F12, 0x0000},
	{0x0F12, 0x40FD},
	{0x0F12, 0x7000},
	{0x0F12, 0x7C63},
	{0x0F12, 0x0000},
	{0x0F12, 0x4119},
	{0x0F12, 0x7000},
	{0x0F12, 0x8F01},
	{0x0F12, 0x0000},
	{0x0F12, 0x41BB},
	{0x0F12, 0x7000},
	{0x0F12, 0x7F3F},
	{0x0F12, 0x0000},
	{0x0F12, 0x4249},
	{0x0F12, 0x7000},
	{0x0F12, 0x98C5},
	{0x0F12, 0x0000},
	{0x0F12, 0x43B5},
	{0x0F12, 0x7000},
	{0x0F12, 0x6099},
	{0x0F12, 0x0000},
	{0x0F12, 0x430F}, // End of TnP
	{0x0F12, 0x7000},
	{0x0F12, 0x4365},
	{0x0F12, 0x7000},
	{0x0F12, 0xA70B},
	{0x0F12, 0x0000},
	{0x0F12, 0x4387},
	{0x0F12, 0x7000},
	{0x0F12, 0x400D},
	{0x0F12, 0x0000},
	{0x0F12, 0xB570},
	{0x0F12, 0x000C},
	{0x0F12, 0x0015},
	{0x0F12, 0x0029},
	{0x0F12, 0xF000},
	{0x0F12, 0xFBD4},
	{0x0F12, 0x49F8},
	{0x0F12, 0x00A8},
	{0x0F12, 0x500C},
	{0x0F12, 0xBC70},
	{0x0F12, 0xBC08},
	{0x0F12, 0x4718},
	{0x0F12, 0x6808},
	{0x0F12, 0x0400},
	{0x0F12, 0x0C00},
	{0x0F12, 0x6849},
	{0x0F12, 0x0409},
	{0x0F12, 0x0C09},
	{0x0F12, 0x4AF3},
	{0x0F12, 0x8992},
	{0x0F12, 0x2A00},
	{0x0F12, 0xD00D},
	{0x0F12, 0x2300},
	{0x0F12, 0x1A89},
	{0x0F12, 0xD400},
	{0x0F12, 0x000B},
	{0x0F12, 0x0419},
	{0x0F12, 0x0C09},
	{0x0F12, 0x23FF},
	{0x0F12, 0x33C1},
	{0x0F12, 0x1810},
	{0x0F12, 0x4298},
	{0x0F12, 0xD800},
	{0x0F12, 0x0003},
	{0x0F12, 0x0418},
	{0x0F12, 0x0C00},
	{0x0F12, 0x4AEB},
	{0x0F12, 0x8150},
	{0x0F12, 0x8191},
	{0x0F12, 0x4770},
	{0x0F12, 0xB5F3},
	{0x0F12, 0x0004},
	{0x0F12, 0xB081},
	{0x0F12, 0x9802},
	{0x0F12, 0x6800},
	{0x0F12, 0x0600},
	{0x0F12, 0x0E00},
	{0x0F12, 0x2201},
	{0x0F12, 0x0015},
	{0x0F12, 0x0021},
	{0x0F12, 0x3910},
	{0x0F12, 0x408A},
	{0x0F12, 0x40A5},
	{0x0F12, 0x4FE4},
	{0x0F12, 0x0016},
	{0x0F12, 0x2C10},
	{0x0F12, 0xDA03},
	{0x0F12, 0x8839},
	{0x0F12, 0x43A9},
	{0x0F12, 0x8039},
	{0x0F12, 0xE002},
	{0x0F12, 0x8879},
	{0x0F12, 0x43B1},
	{0x0F12, 0x8079},
	{0x0F12, 0xF000},
	{0x0F12, 0xFBA0},
	{0x0F12, 0x2C10},
	{0x0F12, 0xDA03},
	{0x0F12, 0x8839},
	{0x0F12, 0x4329},
	{0x0F12, 0x8039},
	{0x0F12, 0xE002},
	{0x0F12, 0x8879},
	{0x0F12, 0x4331},
	{0x0F12, 0x8079},
	{0x0F12, 0x49DA},
	{0x0F12, 0x8809},
	{0x0F12, 0x2900},
	{0x0F12, 0xD102},
	{0x0F12, 0xF000},
	{0x0F12, 0xFB99},
	{0x0F12, 0x2000},
	{0x0F12, 0x9902},
	{0x0F12, 0x6008},
	{0x0F12, 0xBCFE},
	{0x0F12, 0xBC08},
	{0x0F12, 0x4718},
	{0x0F12, 0xB538},
	{0x0F12, 0x9C04},
	{0x0F12, 0x0015},
	{0x0F12, 0x002A},
	{0x0F12, 0x9400},
	{0x0F12, 0xF000},
	{0x0F12, 0xFB94},
	{0x0F12, 0x4AD1},
	{0x0F12, 0x8811},
	{0x0F12, 0x2900},
	{0x0F12, 0xD00F},
	{0x0F12, 0x8820},
	{0x0F12, 0x4281},
	{0x0F12, 0xD20C},
	{0x0F12, 0x8861},
	{0x0F12, 0x8853},
	{0x0F12, 0x4299},
	{0x0F12, 0xD200},
	{0x0F12, 0x1E40},
	{0x0F12, 0x0400},
	{0x0F12, 0x0C00},
	{0x0F12, 0x8020},
	{0x0F12, 0x8851},
	{0x0F12, 0x8061},
	{0x0F12, 0x4368},
	{0x0F12, 0x1840},
	{0x0F12, 0x6060},
	{0x0F12, 0xBC38},
	{0x0F12, 0xBC08},
	{0x0F12, 0x4718},
	{0x0F12, 0xB5F8},
	{0x0F12, 0x0004},
	{0x0F12, 0x6808},
	{0x0F12, 0x0400},
	{0x0F12, 0x0C00},
	{0x0F12, 0x2201},
	{0x0F12, 0x0015},
	{0x0F12, 0x0021},
	{0x0F12, 0x3910},
	{0x0F12, 0x408A},
	{0x0F12, 0x40A5},
	{0x0F12, 0x4FBE},
	{0x0F12, 0x0016},
	{0x0F12, 0x2C10},
	{0x0F12, 0xDA03},
	{0x0F12, 0x8839},
	{0x0F12, 0x43A9},
	{0x0F12, 0x8039},
	{0x0F12, 0xE002},
	{0x0F12, 0x8879},
	{0x0F12, 0x43B1},
	{0x0F12, 0x8079},
	{0x0F12, 0xF000},
	{0x0F12, 0xFB6D},
	{0x0F12, 0x2C10},
	{0x0F12, 0xDA03},
	{0x0F12, 0x8838},
	{0x0F12, 0x4328},
	{0x0F12, 0x8038},
	{0x0F12, 0xE002},
	{0x0F12, 0x8878},
	{0x0F12, 0x4330},
	{0x0F12, 0x8078},
	{0x0F12, 0x48B6},
	{0x0F12, 0x8800},
	{0x0F12, 0x0400},
	{0x0F12, 0xD507},
	{0x0F12, 0x4BB5},
	{0x0F12, 0x7819},
	{0x0F12, 0x4AB5},
	{0x0F12, 0x7810},
	{0x0F12, 0x7018},
	{0x0F12, 0x7011},
	{0x0F12, 0x49B4},
	{0x0F12, 0x8188},
	{0x0F12, 0xBCF8},
	{0x0F12, 0xBC08},
	{0x0F12, 0x4718},
	{0x0F12, 0xB538},
	{0x0F12, 0x48B2},
	{0x0F12, 0x4669},
	{0x0F12, 0xF000},
	{0x0F12, 0xFB58},
	{0x0F12, 0x48B1},
	{0x0F12, 0x49B0},
	{0x0F12, 0x69C2},
	{0x0F12, 0x2400},
	{0x0F12, 0x31A8},
	{0x0F12, 0x2A00},
	{0x0F12, 0xD008},
	{0x0F12, 0x61C4},
	{0x0F12, 0x684A},
	{0x0F12, 0x6242},
	{0x0F12, 0x6282},
	{0x0F12, 0x466B},
	{0x0F12, 0x881A},
	{0x0F12, 0x6302},
	{0x0F12, 0x885A},
	{0x0F12, 0x6342},
	{0x0F12, 0x6A02},
	{0x0F12, 0x2A00},
	{0x0F12, 0xD00A},
	{0x0F12, 0x6204},
	{0x0F12, 0x6849},
	{0x0F12, 0x6281},
	{0x0F12, 0x466B},
	{0x0F12, 0x8819},
	{0x0F12, 0x6301},
	{0x0F12, 0x8859},
	{0x0F12, 0x6341},
	{0x0F12, 0x49A5},
	{0x0F12, 0x88C9},
	{0x0F12, 0x63C1},
	{0x0F12, 0xF000},
	{0x0F12, 0xFB40},
	{0x0F12, 0xE7A6},
	{0x0F12, 0xB5F0},
	{0x0F12, 0xB08B},
	{0x0F12, 0x20FF},
	{0x0F12, 0x1C40},
	{0x0F12, 0x49A1},
	{0x0F12, 0x89CC},
	{0x0F12, 0x4E9E},
	{0x0F12, 0x6AB1},
	{0x0F12, 0x4284},
	{0x0F12, 0xD101},
	{0x0F12, 0x489F},
	{0x0F12, 0x6081},
	{0x0F12, 0x6A70},
	{0x0F12, 0x0200},
	{0x0F12, 0xF000},
	{0x0F12, 0xFB37},
	{0x0F12, 0x0400},
	{0x0F12, 0x0C00},
	{0x0F12, 0x4A96},
	{0x0F12, 0x8A11},
	{0x0F12, 0x9109},
	{0x0F12, 0x2101},
	{0x0F12, 0x0349},
	{0x0F12, 0x4288},
	{0x0F12, 0xD200},
	{0x0F12, 0x0001},
	{0x0F12, 0x4A92},
	{0x0F12, 0x8211},
	{0x0F12, 0x4D97},
	{0x0F12, 0x8829},
	{0x0F12, 0x9108},
	{0x0F12, 0x4A8B},
	{0x0F12, 0x2303},
	{0x0F12, 0x3222},
	{0x0F12, 0x1F91},
	{0x0F12, 0xF000},
	{0x0F12, 0xFB28},
	{0x0F12, 0x8028},
	{0x0F12, 0x488E},
	{0x0F12, 0x4987},
	{0x0F12, 0x6BC2},
	{0x0F12, 0x6AC0},
	{0x0F12, 0x4282},
	{0x0F12, 0xD201},
	{0x0F12, 0x8CC8},
	{0x0F12, 0x8028},
	{0x0F12, 0x88E8},
	{0x0F12, 0x9007},
	{0x0F12, 0x2240},
	{0x0F12, 0x4310},
	{0x0F12, 0x80E8},
	{0x0F12, 0x2000},
	{0x0F12, 0x0041},
	{0x0F12, 0x194B},
	{0x0F12, 0x001E},
	{0x0F12, 0x3680},
	{0x0F12, 0x8BB2},
	{0x0F12, 0xAF04},
	{0x0F12, 0x527A},
	{0x0F12, 0x4A7D},
	{0x0F12, 0x188A},
	{0x0F12, 0x8897},
	{0x0F12, 0x83B7},
	{0x0F12, 0x33A0},
	{0x0F12, 0x891F},
	{0x0F12, 0xAE01},
	{0x0F12, 0x5277},
	{0x0F12, 0x8A11},
	{0x0F12, 0x8119},
	{0x0F12, 0x1C40},
	{0x0F12, 0x0400},
	{0x0F12, 0x0C00},
	{0x0F12, 0x2806},
	{0x0F12, 0xD3E9},
	{0x0F12, 0xF000},
	{0x0F12, 0xFB09},
	{0x0F12, 0xF000},
	{0x0F12, 0xFB0F},
	{0x0F12, 0x4F79},
	{0x0F12, 0x37A8},
	{0x0F12, 0x2800},
	{0x0F12, 0xD10A},
	{0x0F12, 0x1FE0},
	{0x0F12, 0x38FD},
	{0x0F12, 0xD001},
	{0x0F12, 0x1CC0},
	{0x0F12, 0xD105},
	{0x0F12, 0x4874},
	{0x0F12, 0x8829},
	{0x0F12, 0x3818},
	{0x0F12, 0x6840},
	{0x0F12, 0x4348},
	{0x0F12, 0x6078},
	{0x0F12, 0x4972},
	{0x0F12, 0x6878},
	{0x0F12, 0x6B89},
	{0x0F12, 0x4288},
	{0x0F12, 0xD300},
	{0x0F12, 0x0008},
	{0x0F12, 0x6078},
	{0x0F12, 0x2000},
	{0x0F12, 0x0041},
	{0x0F12, 0xAA04},
	{0x0F12, 0x5A53},
	{0x0F12, 0x194A},
	{0x0F12, 0x269C},
	{0x0F12, 0x52B3},
	{0x0F12, 0xAB01},
	{0x0F12, 0x5A59},
	{0x0F12, 0x32A0},
	{0x0F12, 0x8111},
	{0x0F12, 0x1C40},
	{0x0F12, 0x0400},
	{0x0F12, 0x0C00},
	{0x0F12, 0x2806},
	{0x0F12, 0xD3F0},
	{0x0F12, 0x4965},
	{0x0F12, 0x9809},
	{0x0F12, 0x8208},
	{0x0F12, 0x9808},
	{0x0F12, 0x8028},
	{0x0F12, 0x9807},
	{0x0F12, 0x80E8},
	{0x0F12, 0x1FE0},
	{0x0F12, 0x38FD},
	{0x0F12, 0xD13B},
	{0x0F12, 0x4D64},
	{0x0F12, 0x89E8},
	{0x0F12, 0x1FC1},
	{0x0F12, 0x39FF},
	{0x0F12, 0xD136},
	{0x0F12, 0x4C5F},
	{0x0F12, 0x8AE0},
	{0x0F12, 0xF000},
	{0x0F12, 0xFADE},
	{0x0F12, 0x0006},
	{0x0F12, 0x8B20},
	{0x0F12, 0xF000},
	{0x0F12, 0xFAE2},
	{0x0F12, 0x9000},
	{0x0F12, 0x6AA1},
	{0x0F12, 0x6878},
	{0x0F12, 0x1809},
	{0x0F12, 0x0200},
	{0x0F12, 0xF000},
	{0x0F12, 0xFAB5},
	{0x0F12, 0x0400},
	{0x0F12, 0x0C00},
	{0x0F12, 0x0022},
	{0x0F12, 0x3246},
	{0x0F12, 0x0011},
	{0x0F12, 0x310A},
	{0x0F12, 0x2305},
	{0x0F12, 0xF000},
	{0x0F12, 0xFAB2},
	{0x0F12, 0x66E8},
	{0x0F12, 0x6B23},
	{0x0F12, 0x0002},
	{0x0F12, 0x0031},
	{0x0F12, 0x0018},
	{0x0F12, 0xF000},
	{0x0F12, 0xFAD3},
	{0x0F12, 0x466B},
	{0x0F12, 0x8518},
	{0x0F12, 0x6EEA},
	{0x0F12, 0x6B60},
	{0x0F12, 0x9900},
	{0x0F12, 0xF000},
	{0x0F12, 0xFACC},
	{0x0F12, 0x466B},
	{0x0F12, 0x8558},
	{0x0F12, 0x0029},
	{0x0F12, 0x980A},
	{0x0F12, 0x3170},
	{0x0F12, 0xF000},
	{0x0F12, 0xFACD},
	{0x0F12, 0x0028},
	{0x0F12, 0x3060},
	{0x0F12, 0x8A02},
	{0x0F12, 0x4946},
	{0x0F12, 0x3128},
	{0x0F12, 0x808A},
	{0x0F12, 0x8A42},
	{0x0F12, 0x80CA},
	{0x0F12, 0x8A80},
	{0x0F12, 0x8108},
	{0x0F12, 0xB00B},
	{0x0F12, 0xBCF0},
	{0x0F12, 0xBC08},
	{0x0F12, 0x4718},
	{0x0F12, 0xB570},
	{0x0F12, 0x2400},
	{0x0F12, 0x4D46},
	{0x0F12, 0x4846},
	{0x0F12, 0x8881},
	{0x0F12, 0x4846},
	{0x0F12, 0x8041},
	{0x0F12, 0x2101},
	{0x0F12, 0x8001},
	{0x0F12, 0xF000},
	{0x0F12, 0xFABC},
	{0x0F12, 0x4842},
	{0x0F12, 0x3820},
	{0x0F12, 0x8BC0},
	{0x0F12, 0xF000},
	{0x0F12, 0xFABF},
	{0x0F12, 0x4B42},
	{0x0F12, 0x220D},
	{0x0F12, 0x0712},
	{0x0F12, 0x18A8},
	{0x0F12, 0x8806},
	{0x0F12, 0x00E1},
	{0x0F12, 0x18C9},
	{0x0F12, 0x81CE},
	{0x0F12, 0x8846},
	{0x0F12, 0x818E},
	{0x0F12, 0x8886},
	{0x0F12, 0x824E},
	{0x0F12, 0x88C0},
	{0x0F12, 0x8208},
	{0x0F12, 0x3508},
	{0x0F12, 0x042D},
	{0x0F12, 0x0C2D},
	{0x0F12, 0x1C64},
	{0x0F12, 0x0424},
	{0x0F12, 0x0C24},
	{0x0F12, 0x2C07},
	{0x0F12, 0xD3EC},
	{0x0F12, 0xE658},
	{0x0F12, 0xB510},
	{0x0F12, 0x4834},
	{0x0F12, 0x4C34},
	{0x0F12, 0x88C0},
	{0x0F12, 0x8060},
	{0x0F12, 0x2001},
	{0x0F12, 0x8020},
	{0x0F12, 0x4831},
	{0x0F12, 0x3820},
	{0x0F12, 0x8BC0},
	{0x0F12, 0xF000},
	{0x0F12, 0xFA9C},
	{0x0F12, 0x88E0},
	{0x0F12, 0x4A31},
	{0x0F12, 0x2800},
	{0x0F12, 0xD003},
	{0x0F12, 0x4930},
	{0x0F12, 0x8849},
	{0x0F12, 0x2900},
	{0x0F12, 0xD009},
	{0x0F12, 0x2001},
	{0x0F12, 0x03C0},
	{0x0F12, 0x8050},
	{0x0F12, 0x80D0},
	{0x0F12, 0x2000},
	{0x0F12, 0x8090},
	{0x0F12, 0x8110},
	{0x0F12, 0xBC10},
	{0x0F12, 0xBC08},
	{0x0F12, 0x4718},
	{0x0F12, 0x8050},
	{0x0F12, 0x8920},
	{0x0F12, 0x80D0},
	{0x0F12, 0x8960},
	{0x0F12, 0x0400},
	{0x0F12, 0x1400},
	{0x0F12, 0x8090},
	{0x0F12, 0x89A1},
	{0x0F12, 0x0409},
	{0x0F12, 0x1409},
	{0x0F12, 0x8111},
	{0x0F12, 0x89E3},
	{0x0F12, 0x8A24},
	{0x0F12, 0x2B00},
	{0x0F12, 0xD104},
	{0x0F12, 0x17C3},
	{0x0F12, 0x0F5B},
	{0x0F12, 0x1818},
	{0x0F12, 0x10C0},
	{0x0F12, 0x8090},
	{0x0F12, 0x2C00},
	{0x0F12, 0xD1E6},
	{0x0F12, 0x17C8},
	{0x0F12, 0x0F40},
	{0x0F12, 0x1840},
	{0x0F12, 0x10C0},
	{0x0F12, 0x8110},
	{0x0F12, 0xE7E0},
	{0x0F12, 0xB510},
	{0x0F12, 0x000C},
	{0x0F12, 0x4919},
	{0x0F12, 0x2204},
	{0x0F12, 0x6820},
	{0x0F12, 0x5E8A},
	{0x0F12, 0x0140},
	{0x0F12, 0x1A80},
	{0x0F12, 0x0280},
	{0x0F12, 0x8849},
	{0x0F12, 0xF000},
	{0x0F12, 0xFA6A},
	{0x0F12, 0x6020},
	{0x0F12, 0xE7D2},
	{0x0F12, 0x38D4},
	{0x0F12, 0x7000},
	{0x0F12, 0x17D0},
	{0x0F12, 0x7000},
	{0x0F12, 0x5000},
	{0x0F12, 0xD000},
	{0x0F12, 0x1100},
	{0x0F12, 0xD000},
	{0x0F12, 0x171A},
	{0x0F12, 0x7000},
	{0x0F12, 0x4780},
	{0x0F12, 0x7000},
	{0x0F12, 0x2FCA},
	{0x0F12, 0x7000},
	{0x0F12, 0x2FC5},
	{0x0F12, 0x7000},
	{0x0F12, 0x2FC6},
	{0x0F12, 0x7000},
	{0x0F12, 0x2ED8},
	{0x0F12, 0x7000},
	{0x0F12, 0x2BD0},
	{0x0F12, 0x7000},
	{0x0F12, 0x17E0},
	{0x0F12, 0x7000},
	{0x0F12, 0x2DE8},
	{0x0F12, 0x7000},
	{0x0F12, 0x37E0},
	{0x0F12, 0x7000},
	{0x0F12, 0x210C},
	{0x0F12, 0x7000},
	{0x0F12, 0x1484},
	{0x0F12, 0x7000},
	{0x0F12, 0xA006},
	{0x0F12, 0x0000},
	{0x0F12, 0x0724},
	{0x0F12, 0x7000},
	{0x0F12, 0xA000},
	{0x0F12, 0xD000},
	{0x0F12, 0x2270},
	{0x0F12, 0x7000},
	{0x0F12, 0x2558},
	{0x0F12, 0x7000},
	{0x0F12, 0x146C},
	{0x0F12, 0x7000},
	{0x0F12, 0xB510},
	{0x0F12, 0x000C},
	{0x0F12, 0x49C7},
	{0x0F12, 0x2208},
	{0x0F12, 0x6820},
	{0x0F12, 0x5E8A},
	{0x0F12, 0x0140},
	{0x0F12, 0x1A80},
	{0x0F12, 0x0280},
	{0x0F12, 0x88C9},
	{0x0F12, 0xF000},
	{0x0F12, 0xFA30},
	{0x0F12, 0x6020},
	{0x0F12, 0xE798},
	{0x0F12, 0xB5FE},
	{0x0F12, 0x000C},
	{0x0F12, 0x6825},
	{0x0F12, 0x6866},
	{0x0F12, 0x68A0},
	{0x0F12, 0x9001},
	{0x0F12, 0x68E7},
	{0x0F12, 0x1BA8},
	{0x0F12, 0x42B5},
	{0x0F12, 0xDA00},
	{0x0F12, 0x1B70},
	{0x0F12, 0x9000},
	{0x0F12, 0x49BB},
	{0x0F12, 0x48BC},
	{0x0F12, 0x884A},
	{0x0F12, 0x8843},
	{0x0F12, 0x435A},
	{0x0F12, 0x2304},
	{0x0F12, 0x5ECB},
	{0x0F12, 0x0A92},
	{0x0F12, 0x18D2},
	{0x0F12, 0x02D2},
	{0x0F12, 0x0C12},
	{0x0F12, 0x88CB},
	{0x0F12, 0x8880},
	{0x0F12, 0x4343},
	{0x0F12, 0x0A98},
	{0x0F12, 0x2308},
	{0x0F12, 0x5ECB},
	{0x0F12, 0x18C0},
	{0x0F12, 0x02C0},
	{0x0F12, 0x0C00},
	{0x0F12, 0x0411},
	{0x0F12, 0x0400},
	{0x0F12, 0x1409},
	{0x0F12, 0x1400},
	{0x0F12, 0x1A08},
	{0x0F12, 0x49B0},
	{0x0F12, 0x39E0},
	{0x0F12, 0x6148},
	{0x0F12, 0x9801},
	{0x0F12, 0x3040},
	{0x0F12, 0x7880},
	{0x0F12, 0x2800},
	{0x0F12, 0xD103},
	{0x0F12, 0x9801},
	{0x0F12, 0x0029},
	{0x0F12, 0xF000},
	{0x0F12, 0xFA03},
	{0x0F12, 0x8839},
	{0x0F12, 0x9800},
	{0x0F12, 0x4281},
	{0x0F12, 0xD814},
	{0x0F12, 0x8879},
	{0x0F12, 0x9800},
	{0x0F12, 0x4281},
	{0x0F12, 0xD20C},
	{0x0F12, 0x9801},
	{0x0F12, 0x0029},
	{0x0F12, 0xF000},
	{0x0F12, 0xF9FF},
	{0x0F12, 0x9801},
	{0x0F12, 0x0029},
	{0x0F12, 0xF000},
	{0x0F12, 0xF9FB},
	{0x0F12, 0x9801},
	{0x0F12, 0x0029},
	{0x0F12, 0xF000},
	{0x0F12, 0xF9F7},
	{0x0F12, 0xE003},
	{0x0F12, 0x9801},
	{0x0F12, 0x0029},
	{0x0F12, 0xF000},
	{0x0F12, 0xF9F2},
	{0x0F12, 0x9801},
	{0x0F12, 0x0032},
	{0x0F12, 0x0039},
	{0x0F12, 0xF000},
	{0x0F12, 0xF9F5},
	{0x0F12, 0x6020},
	{0x0F12, 0xE5D0},
	{0x0F12, 0xB57C},
	{0x0F12, 0x489A},
	{0x0F12, 0xA901},
	{0x0F12, 0x0004},
	{0x0F12, 0xF000},
	{0x0F12, 0xF979},
	{0x0F12, 0x466B},
	{0x0F12, 0x88D9},
	{0x0F12, 0x8898},
	{0x0F12, 0x4B95},
	{0x0F12, 0x3346},
	{0x0F12, 0x1E9A},
	{0x0F12, 0xF000},
	{0x0F12, 0xF9ED},
	{0x0F12, 0x4894},
	{0x0F12, 0x4992},
	{0x0F12, 0x3812},
	{0x0F12, 0x3140},
	{0x0F12, 0x8A42},
	{0x0F12, 0x888B},
	{0x0F12, 0x18D2},
	{0x0F12, 0x8242},
	{0x0F12, 0x8AC2},
	{0x0F12, 0x88C9},
	{0x0F12, 0x1851},
	{0x0F12, 0x82C1},
	{0x0F12, 0x0020},
	{0x0F12, 0x4669},
	{0x0F12, 0xF000},
	{0x0F12, 0xF961},
	{0x0F12, 0x488D},
	{0x0F12, 0x214D},
	{0x0F12, 0x8301},
	{0x0F12, 0x2196},
	{0x0F12, 0x8381},
	{0x0F12, 0x211D},
	{0x0F12, 0x3020},
	{0x0F12, 0x8001},
	{0x0F12, 0xF000},
	{0x0F12, 0xF9DB},
	{0x0F12, 0xF000},
	{0x0F12, 0xF9E1},
	{0x0F12, 0x4888},
	{0x0F12, 0x4C88},
	{0x0F12, 0x6E00},
	{0x0F12, 0x60E0},
	{0x0F12, 0x466B},
	{0x0F12, 0x8818},
	{0x0F12, 0x8859},
	{0x0F12, 0x0025},
	{0x0F12, 0x1A40},
	{0x0F12, 0x3540},
	{0x0F12, 0x61A8},
	{0x0F12, 0x487F},
	{0x0F12, 0x9900},
	{0x0F12, 0x3060},
	{0x0F12, 0xF000},
	{0x0F12, 0xF9D9},
	{0x0F12, 0x466B},
	{0x0F12, 0x8819},
	{0x0F12, 0x1DE0},
	{0x0F12, 0x30F9},
	{0x0F12, 0x8741},
	{0x0F12, 0x8859},
	{0x0F12, 0x8781},
	{0x0F12, 0x2000},
	{0x0F12, 0x71A0},
	{0x0F12, 0x74A8},
	{0x0F12, 0xBC7C},
	{0x0F12, 0xBC08},
	{0x0F12, 0x4718},
	{0x0F12, 0xB5F8},
	{0x0F12, 0x0005},
	{0x0F12, 0x6808},
	{0x0F12, 0x0400},
	{0x0F12, 0x0C00},
	{0x0F12, 0x684A},
	{0x0F12, 0x0412},
	{0x0F12, 0x0C12},
	{0x0F12, 0x688E},
	{0x0F12, 0x68CC},
	{0x0F12, 0x4970},
	{0x0F12, 0x884B},
	{0x0F12, 0x4343},
	{0x0F12, 0x0A98},
	{0x0F12, 0x2304},
	{0x0F12, 0x5ECB},
	{0x0F12, 0x18C0},
	{0x0F12, 0x02C0},
	{0x0F12, 0x0C00},
	{0x0F12, 0x88CB},
	{0x0F12, 0x4353},
	{0x0F12, 0x0A9A},
	{0x0F12, 0x2308},
	{0x0F12, 0x5ECB},
	{0x0F12, 0x18D1},
	{0x0F12, 0x02C9},
	{0x0F12, 0x0C09},
	{0x0F12, 0x2701},
	{0x0F12, 0x003A},
	{0x0F12, 0x40AA},
	{0x0F12, 0x9200},
	{0x0F12, 0x002A},
	{0x0F12, 0x3A10},
	{0x0F12, 0x4097},
	{0x0F12, 0x2D10},
	{0x0F12, 0xDA06},
	{0x0F12, 0x4A69},
	{0x0F12, 0x9B00},
	{0x0F12, 0x8812},
	{0x0F12, 0x439A},
	{0x0F12, 0x4B67},
	{0x0F12, 0x801A},
	{0x0F12, 0xE003},
	{0x0F12, 0x4B66},
	{0x0F12, 0x885A},
	{0x0F12, 0x43BA},
	{0x0F12, 0x805A},
	{0x0F12, 0x0023},
	{0x0F12, 0x0032},
	{0x0F12, 0xF000},
	{0x0F12, 0xF981},
	{0x0F12, 0x2D10},
	{0x0F12, 0xDA05},
	{0x0F12, 0x4961},
	{0x0F12, 0x9A00},
	{0x0F12, 0x8808},
	{0x0F12, 0x4310},
	{0x0F12, 0x8008},
	{0x0F12, 0xE003},
	{0x0F12, 0x485E},
	{0x0F12, 0x8841},
	{0x0F12, 0x4339},
	{0x0F12, 0x8041},
	{0x0F12, 0x4D5B},
	{0x0F12, 0x2000},
	{0x0F12, 0x3580},
	{0x0F12, 0x88AA},
	{0x0F12, 0x5E30},
	{0x0F12, 0x2100},
	{0x0F12, 0xF000},
	{0x0F12, 0xF98D},
	{0x0F12, 0x8030},
	{0x0F12, 0x2000},
	{0x0F12, 0x88AA},
	{0x0F12, 0x5E20},
	{0x0F12, 0x2100},
	{0x0F12, 0xF000},
	{0x0F12, 0xF986},
	{0x0F12, 0x8020},
	{0x0F12, 0xE587},
	{0x0F12, 0xB510},
	{0x0F12, 0xF000},
	{0x0F12, 0xF989},
	{0x0F12, 0x4A53},
	{0x0F12, 0x8D50},
	{0x0F12, 0x2800},
	{0x0F12, 0xD007},
	{0x0F12, 0x494E},
	{0x0F12, 0x31C0},
	{0x0F12, 0x684B},
	{0x0F12, 0x4950},
	{0x0F12, 0x4283},
	{0x0F12, 0xD202},
	{0x0F12, 0x8D90},
	{0x0F12, 0x81C8},
	{0x0F12, 0xE6A0},
	{0x0F12, 0x8DD0},
	{0x0F12, 0x81C8},
	{0x0F12, 0xE69D},
	{0x0F12, 0xB5F8},
	{0x0F12, 0xF000},
	{0x0F12, 0xF97E},
	{0x0F12, 0x4D49},
	{0x0F12, 0x8E28},
	{0x0F12, 0x2800},
	{0x0F12, 0xD01F},
	{0x0F12, 0x4E49},
	{0x0F12, 0x4844},
	{0x0F12, 0x68B4},
	{0x0F12, 0x6800},
	{0x0F12, 0x4284},
	{0x0F12, 0xD903},
	{0x0F12, 0x1A21},
	{0x0F12, 0x0849},
	{0x0F12, 0x1847},
	{0x0F12, 0xE006},
	{0x0F12, 0x4284},
	{0x0F12, 0xD203},
	{0x0F12, 0x1B01},
	{0x0F12, 0x0849},
	{0x0F12, 0x1A47},
	{0x0F12, 0xE000},
	{0x0F12, 0x0027},
	{0x0F12, 0x0020},
	{0x0F12, 0x493B},
	{0x0F12, 0x3120},
	{0x0F12, 0x7A0C},
	{0x0F12, 0x2C00},
	{0x0F12, 0xD004},
	{0x0F12, 0x0200},
	{0x0F12, 0x0039},
	{0x0F12, 0xF000},
	{0x0F12, 0xF8C3},
	{0x0F12, 0x8668},
	{0x0F12, 0x2C00},
	{0x0F12, 0xD000},
	{0x0F12, 0x60B7},
	{0x0F12, 0xE54D},
	{0x0F12, 0x20FF},
	{0x0F12, 0x1C40},
	{0x0F12, 0x8668},
	{0x0F12, 0xE549},
	{0x0F12, 0xB510},
	{0x0F12, 0x000C},
	{0x0F12, 0x6820},
	{0x0F12, 0x0400},
	{0x0F12, 0x0C00},
	{0x0F12, 0x4933},
	{0x0F12, 0x8E0A},
	{0x0F12, 0x2A00},
	{0x0F12, 0xD003},
	{0x0F12, 0x8E49},
	{0x0F12, 0x0200},
	{0x0F12, 0xF000},
	{0x0F12, 0xF8AD},
	{0x0F12, 0x6020},
	{0x0F12, 0x0400},
	{0x0F12, 0x0C00},
	{0x0F12, 0xE661},
	{0x0F12, 0xB570},
	{0x0F12, 0x680C},
	{0x0F12, 0x4D2F},
	{0x0F12, 0x0020},
	{0x0F12, 0x6F29},
	{0x0F12, 0xF000},
	{0x0F12, 0xF946},
	{0x0F12, 0x6F69},
	{0x0F12, 0x1D20},
	{0x0F12, 0xF000},
	{0x0F12, 0xF942},
	{0x0F12, 0x4827},
	{0x0F12, 0x8E00},
	{0x0F12, 0x2800},
	{0x0F12, 0xD006},
	{0x0F12, 0x4922},
	{0x0F12, 0x2214},
	{0x0F12, 0x3168},
	{0x0F12, 0x0008},
	{0x0F12, 0x383C},
	{0x0F12, 0xF000},
	{0x0F12, 0xF93F},
	{0x0F12, 0xE488},
	{0x0F12, 0xB5F8},
	{0x0F12, 0x0004},
	{0x0F12, 0x4D24},
	{0x0F12, 0x8B68},
	{0x0F12, 0x2800},
	{0x0F12, 0xD012},
	{0x0F12, 0x4823},
	{0x0F12, 0x8A00},
	{0x0F12, 0x06C0},
	{0x0F12, 0xD50E},
	{0x0F12, 0x4822},
	{0x0F12, 0x7800},
	{0x0F12, 0x2800},
	{0x0F12, 0xD00A},
	{0x0F12, 0x481D},
	{0x0F12, 0x6FC1},
	{0x0F12, 0x2000},
	{0x0F12, 0xF000},
	{0x0F12, 0xF923},
	{0x0F12, 0x8B28},
	{0x0F12, 0x2201},
	{0x0F12, 0x2180},
	{0x0F12, 0xF000},
	{0x0F12, 0xF92C},
	{0x0F12, 0x8328},
	{0x0F12, 0x2101},
	{0x0F12, 0x000D},
	{0x0F12, 0x0020},
	{0x0F12, 0x3810},
	{0x0F12, 0x4081},
	{0x0F12, 0x40A5},
	{0x0F12, 0x4F11},
	{0x0F12, 0x000E},
	{0x0F12, 0x2C10},
	{0x0F12, 0xDA03},
	{0x0F12, 0x8838},
	{0x0F12, 0x43A8},
	{0x0F12, 0x8038},
	{0x0F12, 0xE002},
	{0x0F12, 0x8878},
	{0x0F12, 0x43B0},
	{0x0F12, 0x8078},
	{0x0F12, 0xF000},
	{0x0F12, 0xF920},
	{0x0F12, 0x2C10},
	{0x0F12, 0xDA03},
	{0x0F12, 0x8838},
	{0x0F12, 0x4328},
	{0x0F12, 0x8038},
	{0x0F12, 0xE4EF},
	{0x0F12, 0x8878},
	{0x0F12, 0x4330},
	{0x0F12, 0x8078},
	{0x0F12, 0xE4EB},
	{0x0F12, 0x2558},
	{0x0F12, 0x7000},
	{0x0F12, 0x2AB8},
	{0x0F12, 0x7000},
	{0x0F12, 0x145E},
	{0x0F12, 0x7000},
	{0x0F12, 0x2698},
	{0x0F12, 0x7000},
	{0x0F12, 0x2BB8},
	{0x0F12, 0x7000},
	{0x0F12, 0x2998},
	{0x0F12, 0x7000},
	{0x0F12, 0x1100},
	{0x0F12, 0xD000},
	{0x0F12, 0x4780},
	{0x0F12, 0x7000},
	{0x0F12, 0xE200},
	{0x0F12, 0xD000},
	{0x0F12, 0x210C},
	{0x0F12, 0x7000},
	{0x0F12, 0x0000},
	{0x0F12, 0x7000},
	{0x0F12, 0x308C},
	{0x0F12, 0x7000},
	{0x0F12, 0xB040},
	{0x0F12, 0xD000},
	{0x0F12, 0x3858},
	{0x0F12, 0x7000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0x1789},
	{0x0F12, 0x0001},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0x16F1},
	{0x0F12, 0x0001},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0xC3B1},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0xC36D},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0xF6D7},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0xB49D},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0x7EDF},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0x448D},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xF004},
	{0x0F12, 0xE51F},
	{0x0F12, 0x29EC},
	{0x0F12, 0x0001},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0x2EF1},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0xEE03},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0xA58B},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0x7C49},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0x7C63},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0x2DB7},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0xEB3D},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0xF061},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0xF0EF},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xF004},
	{0x0F12, 0xE51F},
	{0x0F12, 0x2824},
	{0x0F12, 0x0001},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0x8EDD},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0x8DCB},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0x8E17},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0x98C5},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0x7C7D},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0x7E31},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0x7EAB},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0x7501},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0xF63F},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0x3D0B},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0x29BF},
	{0x0F12, 0x0001},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xF004},
	{0x0F12, 0xE51F},
	{0x0F12, 0x26D8},
	{0x0F12, 0x0001},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0x306B},
	{0x0F12, 0x0000},
	{0x0F12, 0x4778},
	{0x0F12, 0x46C0},
	{0x0F12, 0xC000},
	{0x0F12, 0xE59F},
	{0x0F12, 0xFF1C},
	{0x0F12, 0xE12F},
	{0x0F12, 0x6099},
	{0x0F12, 0x0000},// End of Patch Data(Last : 7000449Eh)
// End of Patch Data(Last : 7000465Ah)
// Total Size 2916 (0B64)
// Addr : 3AF8  Size : 2914(B62h)


//	TNP_USER_MBCV_CONTROL
// TNP_4EC_MBR_TUNE
// TNP_4EC_FORBIDDEN_TUNE
// TNP_AF_FINESEARCH_DRIVEBACK
// TNP_FLASH_ALG
// TNP_GAS_ALPHA_OTP
//	TNP_AWB_MODUL_COMP
// TNP_AWB_INIT_QUEUE
// TNP_AWB_GRID_LOWBR
// TNP_AWB_GRID_MODULECOMP
//	TNP_ADLC_TUNE
// TNP_1FRAME_AE
// TNP_TG_OFF_CFG_CHG_IN_SPOOF_MODE

//===================================================================
// OTP setting
//===================================================================
	{0x002A, 0x0722}, // OTP block
	{0x0F12, 0x0100},
	{0x002A, 0x0726},
	{0x0F12, 0x0001},
	{0x002A, 0x08D6},
	{0x0F12, 0x0001},
	{0x002A, 0x146E},
	{0x0F12, 0x0000},
	{0x002A, 0x08DC},
	{0x0F12, 0x0000},
	{0x0028, 0xD000},
	{0x002A, 0x1000},
	{0x0F12, 0x0001},




//===================================================================
// GAS setting (Shading)
//===================================================================
// If OTP is used, GAS setting should be deleted.
//===================================================================
// GAS Alpha setting
//===================================================================
// Refer Mon_AWB_RotGain
	{0x0028, 0x7000},
	{0x002A, 0x08B4},
	{0x0F12, 0x0001},       //wbt_bUseOutdoorASH
	{0x002A, 0x08BC},
	{0x0F12, 0x00C0},       //TVAR_ash_AwbAshCord_0_ 2300K
	{0x0F12, 0x00DF},       //TVAR_ash_AwbAshCord_1_ 2750K
	{0x0F12, 0x0100},       //TVAR_ash_AwbAshCord_2_ 3300K
	{0x0F12, 0x0125},       //TVAR_ash_AwbAshCord_3_ 4150K
	{0x0F12, 0x015F},       //TVAR_ash_AwbAshCord_4_ 5250K
	{0x0F12, 0x017C},       //TVAR_ash_AwbAshCord_5_ 6400K
	{0x0F12, 0x0194},  	    //TVAR_ash_AwbAshCord_6_ 7500K

// GAS Alpha Table
	{0x002A, 0x08F6},
	{0x0F12, 0x4000},       //TVAR_ash_GASAlpha_0__0_ R  // 2300K
	{0x0F12, 0x4000},       //TVAR_ash_GASAlpha_0__1_ GR
	{0x0F12, 0x4000},       //TVAR_ash_GASAlpha_0__2_ GB
	{0x0F12, 0x4000},       //TVAR_ash_GASAlpha_0__3_ B
	{0x0F12, 0x4000},       //TVAR_ash_GASAlpha_1__0_ R  // 2750K
	{0x0F12, 0x4000},       //TVAR_ash_GASAlpha_1__1_ GR
	{0x0F12, 0x4000},       //TVAR_ash_GASAlpha_1__2_ GB
	{0x0F12, 0x4000},       //TVAR_ash_GASAlpha_1__3_ B
	{0x0F12, 0x4000},       //TVAR_ash_GASAlpha_2__0_ R  // 3300K
	{0x0F12, 0x4000},       //TVAR_ash_GASAlpha_2__1_ GR
	{0x0F12, 0x4000},       //TVAR_ash_GASAlpha_2__2_ GB
	{0x0F12, 0x4000},       //TVAR_ash_GASAlpha_2__3_ B
	{0x0F12, 0x3B00},       //TVAR_ash_GASAlpha_3__0_ R  // 4150K
	{0x0F12, 0x4000},       //TVAR_ash_GASAlpha_3__1_ GR
	{0x0F12, 0x4000},       //TVAR_ash_GASAlpha_3__2_ GB
	{0x0F12, 0x4000},       //TVAR_ash_GASAlpha_3__3_ B
	{0x0F12, 0x3E00},       //TVAR_ash_GASAlpha_4__0_ R  // 5250K
	{0x0F12, 0x4000},       //TVAR_ash_GASAlpha_4__1_ GR
	{0x0F12, 0x4000},       //TVAR_ash_GASAlpha_4__2_ GB
	{0x0F12, 0x4000},       //TVAR_ash_GASAlpha_4__3_ B
	{0x0F12, 0x4100},       //TVAR_ash_GASAlpha_5__0_ R  // 6400K
	{0x0F12, 0x4000},       //TVAR_ash_GASAlpha_5__1_ GR
	{0x0F12, 0x4000},       //TVAR_ash_GASAlpha_5__2_ GB
	{0x0F12, 0x4000},       //TVAR_ash_GASAlpha_5__3_ B
	{0x0F12, 0x4300},       //TVAR_ash_GASAlpha_6__0_ R  // 7500K
	{0x0F12, 0x4000},       //TVAR_ash_GASAlpha_6__1_ GR
	{0x0F12, 0x4000},       //TVAR_ash_GASAlpha_6__2_ GB
	{0x0F12, 0x4000},       //TVAR_ash_GASAlpha_6__3_ B
// Outdoor GAS Alpha
	{0x0F12, 0X4900},       //TVAR_ash_GASOutdoorAlpha_0_ R
	{0x0F12, 0X4000},       //TVAR_ash_GASOutdoorAlpha_1_ GR
	{0x0F12, 0X4000},       //TVAR_ash_GASOutdoorAlpha_2_ GB
	{0x0F12, 0X4000},       //TVAR_ash_GASOutdoorAlpha_3_ B
	{0x002A, 0X08F4},
	{0x0F12, 0X0001},       //ash_bUseGasAlpha


//==================================================================================
// 07. Analog Setting 2
//==================================================================================
//This register is for FACTORY ONLY.
//If you change it without prior notification
//YOU are RESPONSIBLE for the FAILURE that will happen in the future
//For subsampling Size

	{0x0028, 0X7000},       //REG_ANALOG_SETTING2
	{0x002A, 0X18BC},
	{0x0F12, 0X0004},     	//senHal_ContPtrs_senModesDataArr_0_
	{0x0F12, 0X05B6},     	//senHal_ContPtrs_senModesDataArr_1_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_2_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_3_
	{0x0F12, 0X0001},     	//senHal_ContPtrs_senModesDataArr_4_
	{0x0F12, 0X05BA},     	//senHal_ContPtrs_senModesDataArr_5_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_6_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_7_
	{0x0F12, 0X0007},     	//senHal_ContPtrs_senModesDataArr_8_
	{0x0F12, 0X05BA},     	//senHal_ContPtrs_senModesDataArr_9_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_10_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_11_
	{0x0F12, 0X01F4},     	//senHal_ContPtrs_senModesDataArr_12_
	{0x0F12, 0X024E},     	//senHal_ContPtrs_senModesDataArr_13_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_14_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_15_
	{0x0F12, 0X01F4},     	//senHal_ContPtrs_senModesDataArr_16_
	{0x0F12, 0X05B6},     	//senHal_ContPtrs_senModesDataArr_17_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_18_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_19_
	{0x0F12, 0X01F4},     	//senHal_ContPtrs_senModesDataArr_20_
	{0x0F12, 0X05BA},     	//senHal_ContPtrs_senModesDataArr_21_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_22_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_23_
	{0x0F12, 0X01F4},     	//senHal_ContPtrs_senModesDataArr_24_
	{0x0F12, 0X024F},     	//senHal_ContPtrs_senModesDataArr_25_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_26_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_27_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_28_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_29_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_30_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_31_
	{0x0F12, 0X0075},     	//senHal_ContPtrs_senModesDataArr_32_
	{0x0F12, 0X00CF},     	//senHal_ContPtrs_senModesDataArr_33_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_34_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_35_
	{0x0F12, 0X0075},     	//senHal_ContPtrs_senModesDataArr_36_
	{0x0F12, 0X00D6},     	//senHal_ContPtrs_senModesDataArr_37_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_38_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_39_
	{0x0F12, 0X0004},     	//senHal_ContPtrs_senModesDataArr_40_
	{0x0F12, 0X01F4},     	//senHal_ContPtrs_senModesDataArr_41_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_42_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_43_
	{0x0F12, 0X00F0},     	//senHal_ContPtrs_senModesDataArr_44_
	{0x0F12, 0X01F4},     	//senHal_ContPtrs_senModesDataArr_45_
	{0x0F12, 0X029E},     	//senHal_ContPtrs_senModesDataArr_46_
	{0x0F12, 0X05B2},     	//senHal_ContPtrs_senModesDataArr_47_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_48_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_49_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_50_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_51_
	{0x0F12, 0X01F8},     	//senHal_ContPtrs_senModesDataArr_52_
	{0x0F12, 0X0228},     	//senHal_ContPtrs_senModesDataArr_53_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_54_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_55_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_56_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_57_
	{0x0F12, 0X0208},     	//senHal_ContPtrs_senModesDataArr_58_
	{0x0F12, 0X0238},     	//senHal_ContPtrs_senModesDataArr_59_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_60_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_61_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_62_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_63_
	{0x0F12, 0X0218},     	//senHal_ContPtrs_senModesDataArr_64_
	{0x0F12, 0X0238},     	//senHal_ContPtrs_senModesDataArr_65_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_66_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_67_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_68_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_69_
	{0x0F12, 0X0001},     	//senHal_ContPtrs_senModesDataArr_70_
	{0x0F12, 0X0009},     	//senHal_ContPtrs_senModesDataArr_71_
	{0x0F12, 0X00DE},     	//senHal_ContPtrs_senModesDataArr_72_
	{0x0F12, 0X05C0},     	//senHal_ContPtrs_senModesDataArr_73_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_74_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_75_
	{0x0F12, 0X00DF},     	//senHal_ContPtrs_senModesDataArr_76_
	{0x0F12, 0X00E4},     	//senHal_ContPtrs_senModesDataArr_77_
	{0x0F12, 0X01F8},     	//senHal_ContPtrs_senModesDataArr_78_
	{0x0F12, 0X01FD},     	//senHal_ContPtrs_senModesDataArr_79_
	{0x0F12, 0X05B6},     	//senHal_ContPtrs_senModesDataArr_80_
	{0x0F12, 0X05BB},     	//senHal_ContPtrs_senModesDataArr_81_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_82_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_83_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_84_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_85_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_86_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_87_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_88_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_89_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_90_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_91_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_92_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_93_
	{0x0F12, 0X01F8},     	//senHal_ContPtrs_senModesDataArr_94_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_95_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_96_
	{0x0F12, 0X0077},     	//senHal_ContPtrs_senModesDataArr_97_
	{0x0F12, 0X007E},     	//senHal_ContPtrs_senModesDataArr_98_
	{0x0F12, 0X024F},     	//senHal_ContPtrs_senModesDataArr_99_
	{0x0F12, 0X025E},     	//senHal_ContPtrs_senModesDataArr_100_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_101_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_102_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_103_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senModesDataArr_104_

	{0x0F12, 0X0004},     	//senHal_ContPtrs_senAvgModesDataArr_0_
	{0x0F12, 0X09D1},     	//senHal_ContPtrs_senAvgModesDataArr_1_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_2_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_3_
	{0x0F12, 0X0001},     	//senHal_ContPtrs_senAvgModesDataArr_4_
	{0x0F12, 0X09D5},     	//senHal_ContPtrs_senAvgModesDataArr_5_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_6_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_7_
	{0x0F12, 0X0008},     	//senHal_ContPtrs_senAvgModesDataArr_8_
	{0x0F12, 0X09D5},     	//senHal_ContPtrs_senAvgModesDataArr_9_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_10_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_11_
	{0x0F12, 0X02AA},     	//senHal_ContPtrs_senAvgModesDataArr_12_
	{0x0F12, 0X0326},     	//senHal_ContPtrs_senAvgModesDataArr_13_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_14_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_15_
	{0x0F12, 0X02AA},     	//senHal_ContPtrs_senAvgModesDataArr_16_
	{0x0F12, 0X09D1},     	//senHal_ContPtrs_senAvgModesDataArr_17_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_18_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_19_
	{0x0F12, 0X02AA},     	//senHal_ContPtrs_senAvgModesDataArr_20_
	{0x0F12, 0X09D5},     	//senHal_ContPtrs_senAvgModesDataArr_21_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_22_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_23_
	{0x0F12, 0X02AA},     	//senHal_ContPtrs_senAvgModesDataArr_24_
	{0x0F12, 0X0327},     	//senHal_ContPtrs_senAvgModesDataArr_25_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_26_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_27_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_28_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_29_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_30_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_31_
	{0x0F12, 0X0008},     	//senHal_ContPtrs_senAvgModesDataArr_32_
	{0x0F12, 0X0084},     	//senHal_ContPtrs_senAvgModesDataArr_33_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_34_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_35_
	{0x0F12, 0X0008},     	//senHal_ContPtrs_senAvgModesDataArr_36_
	{0x0F12, 0X008D},     	//senHal_ContPtrs_senAvgModesDataArr_37_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_38_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_39_
	{0x0F12, 0X0008},     	//senHal_ContPtrs_senAvgModesDataArr_40_
	{0x0F12, 0X02AA},     	//senHal_ContPtrs_senAvgModesDataArr_41_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_42_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_43_
	{0x0F12, 0X00AA},     	//senHal_ContPtrs_senAvgModesDataArr_44_
	{0x0F12, 0X02AA},     	//senHal_ContPtrs_senAvgModesDataArr_45_
	{0x0F12, 0X03AD},     	//senHal_ContPtrs_senAvgModesDataArr_46_
	{0x0F12, 0X09CD},     	//senHal_ContPtrs_senAvgModesDataArr_47_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_48_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_49_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_50_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_51_
	{0x0F12, 0X02AE},     	//senHal_ContPtrs_senAvgModesDataArr_52_
	{0x0F12, 0X02DE},     	//senHal_ContPtrs_senAvgModesDataArr_53_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_54_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_55_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_56_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_57_
	{0x0F12, 0X02BE},     	//senHal_ContPtrs_senAvgModesDataArr_58_
	{0x0F12, 0X02EE},     	//senHal_ContPtrs_senAvgModesDataArr_59_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_60_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_61_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_62_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_63_
	{0x0F12, 0X02CE},     	//senHal_ContPtrs_senAvgModesDataArr_64_
	{0x0F12, 0X02EE},     	//senHal_ContPtrs_senAvgModesDataArr_65_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_66_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_67_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_68_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_69_
	{0x0F12, 0X0001},     	//senHal_ContPtrs_senAvgModesDataArr_70_
	{0x0F12, 0X0009},     	//senHal_ContPtrs_senAvgModesDataArr_71_
	{0x0F12, 0X0095},     	//senHal_ContPtrs_senAvgModesDataArr_72_
	{0x0F12, 0X09DB},     	//senHal_ContPtrs_senAvgModesDataArr_73_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_74_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_75_
	{0x0F12, 0X0096},     	//senHal_ContPtrs_senAvgModesDataArr_76_
	{0x0F12, 0X009B},     	//senHal_ContPtrs_senAvgModesDataArr_77_
	{0x0F12, 0X02AE}, 	    //senHal_ContPtrs_senAvgModesDataArr_78_
	{0x0F12, 0X02B3},     	//senHal_ContPtrs_senAvgModesDataArr_79_
	{0x0F12, 0X09D1},     	//senHal_ContPtrs_senAvgModesDataArr_80_
	{0x0F12, 0X09D6},     	//senHal_ContPtrs_senAvgModesDataArr_81_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_82_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_83_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_84_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_85_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_86_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_87_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_88_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_89_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_90_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_91_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_92_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_93_
	{0x0F12, 0X02AE},     	//senHal_ContPtrs_senAvgModesDataArr_94_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_95_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_96_
	{0x0F12, 0X0009},     	//senHal_ContPtrs_senAvgModesDataArr_97_
	{0x0F12, 0X0010},     	//senHal_ContPtrs_senAvgModesDataArr_98_
	{0x0F12, 0X0327},     	//senHal_ContPtrs_senAvgModesDataArr_99_
	{0x0F12, 0X0336},     	//senHal_ContPtrs_senAvgModesDataArr_100_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_101_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_102_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_103_
	{0x0F12, 0X0000},     	//senHal_ContPtrs_senAvgModesDataArr_104_

	{0x002A, 0X1AF8},
	{0x0F12, 0X5A3C},       //senHal_TuneStr_AngTuneData1_2_ register at subsampling
	{0x002A, 0X1896},
	{0x0F12, 0X0002},       //senHal_SamplingType
	{0x0F12, 0X0000},       //senHal_SamplingMode 0000 : 2PLA}, 0001 : 4PLA
	{0x0F12, 0X0003},       //senHal_PLAOption [0] VPLA enable}, [1] HPLA enable

	{0x002A, 0X1B00},       //Add for low lux flash from LSI
	{0x0F12, 0XF428},
	{0x0F12, 0XFFFF},
	{0x0F12, 0X0000},

	{0x002A, 0X189E},
	{0x0F12, 0X0FB0},	      //senHal_ExpMinPixels

	{0x002A, 0X18AC},
	{0x0F12, 0X0060},  	    //senHal_uAddColsBin
	{0x0F12, 0X0060},  	    //senHal_uAddColsNoBin
	{0x0F12, 0X05C0},  	    //senHal_uMinColsBin
	{0x0F12, 0X05C0},	      //senHal_uMinColsNoBin

	{0x002A, 0X1AEA},
	{0x0F12, 0X8080},       //senHal_SubF404Tune
	{0x0F12, 0X0080}, 	    //senHal_FullF404Tune
	{0x002A, 0X1AE0},
	{0x0F12, 0X0000}, 	    //senHal_bSenAAC

	{0x002A, 0X1A72},
	{0x0F12, 0X0000}, 	    //senHal_bSRX SRX off
	{0x002A, 0X18A2},
	{0x0F12, 0X0004}, 	    //senHal_NExpLinesCheckFine extend Forbidden area line
	{0x002A, 0X1A6A},
	{0x0F12, 0X009A}, 	    //senHal_usForbiddenRightOfs extend right Forbidden area line
	{0x002A, 0X385E},
	{0x0F12, 0X024C}, 	    //Mon_Sen_uExpPixelsOfs

	{0x002A, 0X0EE6},
	{0x0F12, 0X0000}, 	    //setot_bUseDigitalHbin
	{0x002A, 0X1B2A},
	{0x0F12, 0X0300},       //senHal_TuneStr2_usAngTuneGainTh
	{0x0F12, 0X00D6},       //senHal_TuneStr2_AngTuneF4CA_0_
	{0x0F12, 0X008D},       //senHal_TuneStr2_AngTuneF4CA_1_
	{0x0F12, 0X00CF},       //senHal_TuneStr2_AngTuneF4C2_0_
	{0x0F12, 0X0084},       //senHal_TuneStr2_AngTuneF4C2_1_


//===================================================================
// 08.AF Setting
//===================================================================

	{0x0028, 0X7000}, //
	{0x002A, 0X01FC}, //
	{0x0F12, 0X0001}, //REG_TC_IPRM_LedGpio

	{0x002A, 0X01FE}, //
	{0x0F12, 0X0003},	//REG_TC_IPRM_CM_Init_AfModeType VCM IIC
	{0x0F12, 0X0000}, //REG_TC_IPRM_CM_Init_PwmConfig1
	{0x002A, 0X0204}, //
	{0x0F12, 0X0061}, //REG_TC_IPRM_CM_Init_GpioConfig1 AF Enable GPIO 6
	{0x002A, 0X020C}, //
	{0x0F12, 0X2F0C},	//REG_TC_IPRM_CM_Init_Mi2cBits
	{0x0F12, 0X0190}, //REG_TC_IPRM_CM_Init_Mi2cRateKhz IIC Speed
// AF Window Settings
	{0x002A, 0X0294}, //
	{0x0F12, 0X0100},	//REG_TC_AF_FstWinStartX
	{0x0F12, 0X00E3},	//REG_TC_AF_FstWinStartY
	{0x0F12, 0X0200},	//REG_TC_AF_FstWinSizeX
	{0x0F12, 0X0238},	//REG_TC_AF_FstWinSizeY
	{0x0F12, 0X01C6}, // LSI_Cho AF Window Center from_LSI
	{0x0F12, 0X0166},	//REG_TC_AF_ScndWinStartY
	{0x0F12, 0X0074}, // LSI_Cho AF Fail when Size change from_LSI
	{0x0F12, 0X0132},	//REG_TC_AF_ScndWinSizeY
	{0x0F12, 0X0001}, //REG_TC_AF_WinSizesUpdated
// 2nd search setting
	{0x002A, 0X070E},
	{0x0F12, 0X00C0},
	{0x002A, 0X071E},
	{0x0F12, 0X0000},
	{0x002A, 0X163C},
	{0x0F12, 0X0000},
	{0x002A, 0X1648},
	{0x0F12, 0X9002},
	{0x002A, 0X1652},
	{0x0F12, 0X0002},
	{0x0F12, 0X0000},
	{0x002A, 0X15E0},
	{0x0F12, 0X0402},
// Peak Threshold
	{0x002A, 0X164C},
	{0x0F12, 0X0003},
	{0x002A, 0X163E},
	{0x0F12, 0X00C2},
	{0x0F12, 0X0098},
// Home Pos
	{0x002A, 0X15D4},
	{0x0F12, 0X0000},
	{0x0F12, 0XD000},
// AF statistics
	{0x002A, 0X169A},
	{0x0F12, 0XFF95},
	{0x002A, 0X166A},
	{0x0F12, 0X0280},
	{0x002A, 0X1676},
	{0x0F12, 0X03A0},
	{0x0F12, 0X0320},
	{0x002A, 0X16BC},
	{0x0F12, 0X0030},
	{0x002A, 0X16E0},
	{0x0F12, 0X0060},
	{0x002A, 0X16D4},
	{0x0F12, 0X0010},
	{0x002A, 0X1656},
	{0x0F12, 0X0000},
	{0x002A, 0X15E6},
	{0x0F12, 0X003C},

	{0x0F12, 0X0018},
	{0x0F12, 0X002A},
	{0x0F12, 0X0030},
	{0x0F12, 0X0036},
	{0x0F12, 0X003C},
	{0x0F12, 0X0042},
	{0x0F12, 0X0048},
	{0x0F12, 0X004E},
	{0x0F12, 0X0054},
	{0x0F12, 0X005A},
	{0x0F12, 0X0060},
	{0x0F12, 0X0066},
	{0x0F12, 0X006C},
	{0x0F12, 0X0072},
	{0x0F12, 0X0078},
	{0x0F12, 0X007E},
	{0x0F12, 0X0084},
	{0x0F12, 0X008A},
	{0x0F12, 0X0090},
	{0x0F12, 0X0096},
	{0x0F12, 0X009C},
	{0x0F12, 0X00A2},
	{0x0F12, 0X00A8},
	{0x0F12, 0X00AE},
	{0x0F12, 0X00B4},
	{0x0F12, 0X00BA},

	{0x002A, 0X1722},
	{0x0F12, 0X8000},
	{0x0F12, 0X0006},
	{0x0F12, 0X3FF0},
	{0x0F12, 0X03E8},
	{0x0F12, 0X0000},
	{0x0F12, 0X0008},
	{0x0F12, 0X0001},
	{0x0F12, 0X0004},
	{0x0F12, 0X0080},
	{0x0F12, 0X00C0},
	{0x0F12, 0X00E0},
	{0x0F12, 0X0000},

	{0x002A, 0X028C},
	{0x0F12, 0X0003},

//===================================================================
// AWB setting
//===================================================================
// AWB White Locus should be in front of REG_TC_IPRM_InitParamsUpdated //

// AWB init Start point
	{0x002A, 0X145E},
	{0x0F12, 0X0580},   //awbb_GainsInit_0_
	{0x0F12, 0X0428},   //awbb_GainsInit_1_
	{0x0F12, 0X07B0},

// White Locus
	{0x002A, 0X11F0},
	{0x0F12, 0X0120},   //awbb_IntcR
	{0x0F12, 0X0121}, 	//awbb_IntcB

// Indoor Zone
	{0x002A, 0X101C},
	{0x0F12, 0X03B8},
	{0x0F12, 0X0410},
	{0x0F12, 0X0344},
	{0x0F12, 0X040C},
	{0x0F12, 0X02E6},
	{0x0F12, 0X03F0},
	{0x0F12, 0X029A},
	{0x0F12, 0X03A8},
	{0x0F12, 0X0272},
	{0x0F12, 0X0366},
	{0x0F12, 0X0250},
	{0x0F12, 0X032E},
	{0x0F12, 0X0240},
	{0x0F12, 0X02F4},
	{0x0F12, 0X0236},
	{0x0F12, 0X02BC},
	{0x0F12, 0X021A},
	{0x0F12, 0X02A0},
	{0x0F12, 0X01F4},
	{0x0F12, 0X0298},
	{0x0F12, 0X01B8},
	{0x0F12, 0X0290},
	{0x0F12, 0X01A4},
	{0x0F12, 0X0276},
	{0x0F12, 0X01AA},
	{0x0F12, 0X0260},
	{0x0F12, 0X01C6},
	{0x0F12, 0X0210},
	{0x0F12, 0X0000},
	{0x0F12, 0X0000},
	{0x0F12, 0X0000},
	{0x0F12, 0X0000},
	{0x0F12, 0X0000},
	{0x0F12, 0X0000},
	{0x0F12, 0X0000},
	{0x0F12, 0X0000},
	{0x0F12, 0X0000},
	{0x0F12, 0X0000},
	{0x0F12, 0X0000},
	{0x0F12, 0X0000},
	{0x0F12, 0x0005},
	{0x002A, 0x1070},
	{0x0F12, 0x000E},
	{0x002A, 0x1074},
	{0x0F12, 0x0106},
// Outdoor Zone
	{0x002A, 0x1078},
	{0x0F12, 0x025A},
	{0x0F12, 0x027C},
	{0x0F12, 0x0236},
	{0x0F12, 0x028E},
	{0x0F12, 0x0222},
	{0x0F12, 0x0290},
	{0x0F12, 0x0214},
	{0x0F12, 0x0290},
	{0x0F12, 0x0208},
	{0x0F12, 0x028E},
	{0x0F12, 0x0200},
	{0x0F12, 0x0288},
	{0x0F12, 0x01F8},
	{0x0F12, 0x027E},
	{0x0F12, 0x01F4},
	{0x0F12, 0x0272},
	{0x0F12, 0x01FC},
	{0x0F12, 0x0260},
	{0x0F12, 0x021C},
	{0x0F12, 0x023C},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},

	{0x0F12, 0x0004},
	{0x002A, 0x10AC},
	{0x0F12, 0x000A},
	{0x002A, 0x10B0},
	{0x0F12, 0x01DC},


	{0x002A, 0x10B4},
	{0x0F12, 0x039E},
	{0x0F12, 0x0470},
	{0x0F12, 0x0330},
	{0x0F12, 0x0460},
	{0x0F12, 0x02C4},
	{0x0F12, 0x0452},
	{0x0F12, 0x0278},
	{0x0F12, 0x041C},
	{0x0F12, 0x0230},
	{0x0F12, 0x03EE},
	{0x0F12, 0x01F0},
	{0x0F12, 0x0392},
	{0x0F12, 0x01C0},
	{0x0F12, 0x0340},
	{0x0F12, 0x0194},
	{0x0F12, 0x0302},
	{0x0F12, 0x016E},
	{0x0F12, 0x02C2},
	{0x0F12, 0x0148},
	{0x0F12, 0x0286},
	{0x0F12, 0x018A},
	{0x0F12, 0x0242},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},

	{0x0F12, 0x0006},
	{0x002A, 0x10E8},
	{0x0F12, 0x000B},
	{0x002A, 0x10EC},
	{0x0F12, 0x00C6},


	{0x002A, 0x10F0},
	{0x0F12, 0x0380},
	{0x0F12, 0x0000},
	{0x0F12, 0x0168},
	{0x0F12, 0x0000},
	{0x0F12, 0x2D90},
	{0x0F12, 0x0000},


	{0x002A, 0x1464},
	{0x0F12, 0x0008},
	{0x0F12, 0x0190},
	{0x0F12, 0x00A0},

	{0x002A, 0x1228},
	{0x0F12, 0x00C0},
	{0x002A, 0x122C},
	{0x0F12, 0x0010},
	{0x002A, 0x122A},
	{0x0F12, 0x0010},

	{0x002A, 0x120A},
	{0x0F12, 0x05D5},
	{0x002A, 0x120E},
	{0x0F12, 0x0000},

	{0x0F12, 0x0771},
	{0x0F12, 0x03A4},
	{0x0F12, 0x0036},
	{0x0F12, 0x002A},

	{0x002A, 0x1278},
	{0x0F12, 0xFEF7},
	{0x0F12, 0x0021},
	{0x0F12, 0x0AF0},
	{0x0F12, 0x0AF0},
	{0x0F12, 0x018F},
	{0x0F12, 0x0096},
	{0x0F12, 0x000E},
	{0x002A, 0x1224},
	{0x0F12, 0x0032},
	{0x0F12, 0x001E},
	{0x0F12, 0x00C0},
	{0x0F12, 0x0010},
	{0x0F12, 0x0002},
	{0x002A, 0x2BA4},
	{0x0F12, 0x0004},

	{0x002A, 0x146C},
	{0x0F12, 0x0002},

// grid const
	{0x002A, 0x1434},
	{0x0F12, 0x02CE},
	{0x0F12, 0x0347},
	{0x0F12, 0x03C2},
	{0x0F12, 0x1060},
	{0x0F12, 0x1000},
	{0x0F12, 0x1150},
	{0x0F12, 0x11E5},
	{0x0F12, 0x123B},
	{0x0F12, 0x1296},
	{0x0F12, 0x00AB},
	{0x0F12, 0x00BF},
	{0x0F12, 0x00D2},
	{0x0F12, 0x0093},

// Indoor Grid Offset_LYA
	{0x002A, 0x13A4},
	{0x0F12, 0x0010},
	{0x0F12, 0x0010},
	{0x0F12, 0xFFE0},
	{0x0F12, 0xFFE0},
	{0x0F12, 0xFFE0},
	{0x0F12, 0xFFE0},
	{0x0F12, 0x0010},
	{0x0F12, 0x0010},
	{0x0F12, 0xFFE0},
	{0x0F12, 0xFFE0},
	{0x0F12, 0xFFE0},
	{0x0F12, 0xFFE0},
	{0x0F12, 0x0010},
	{0x0F12, 0x0010},
	{0x0F12, 0xFFE0},
	{0x0F12, 0xFFE0},
	{0x0F12, 0xFFE0},
	{0x0F12, 0xFFE0},

	{0x0F12, 0xFFE0},
	{0x0F12, 0xFFE0},
	{0x0F12, 0xFFE0},
	{0x0F12, 0xFF80},
	{0x0F12, 0xFF00},
	{0x0F12, 0xFE80},
	{0x0F12, 0xFFE0},
	{0x0F12, 0xFFE0},
	{0x0F12, 0xFFE0},
	{0x0F12, 0xFF80},
	{0x0F12, 0xFF00},
	{0x0F12, 0xFE80},
	{0x0F12, 0xFFE0},
	{0x0F12, 0xFFE0},
	{0x0F12, 0xFFE0},
	{0x0F12, 0xFF80},
	{0x0F12, 0xFF00},
	{0x0F12, 0xFE80},

// Outdoor Grid Offset_LYA
	{0x0F12, 0xFFB0},
	{0x0F12, 0xFFB0},
	{0x0F12, 0xFFB0},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},

	{0x0F12, 0xFFB0},
	{0x0F12, 0xFFB0},
	{0x0F12, 0xFFB0},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},

	{0x0F12, 0xFFB0},
	{0x0F12, 0xFFB0},
	{0x0F12, 0xFFB0},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},

	{0x0F12, 0x0030},
	{0x0F12, 0x0030},
	{0x0F12, 0x0030},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},

	{0x0F12, 0x0030},
	{0x0F12, 0x0030},
	{0x0F12, 0x0030},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},

	{0x0F12, 0x0030},
	{0x0F12, 0x0030},
	{0x0F12, 0x0030},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},


	{0x002A, 0x1208},
	{0x0F12, 0x0020},

	{0x002A, 0x144E},
	{0x0F12, 0x0000},
	{0x0F12, 0xFFE0},
	{0x0F12, 0x0000},


//==================================================================================
// 10.Clock Setting                          PCLK  X
//==================================================================================

	{0x002A, 0x01F8}, // Clock setting block
	{0x0F12, 0x5DC0}, //REG_TC_IPRM_InClockLSBs //MCLK : 24Mhz
	{0x002A, 0x0212},
	{0x0F12, 0x0000}, //REG_TC_IPRM_UseNPviClocks
	{0x0F12, 0x0002}, //REG_TC_IPRM_UseNMipiClocks
	{0x0F12, 0x0002}, //REG_TC_IPRM_NumberOfMipiLanes
	{0x002A, 0x021A},
	{0x0F12, 0x3A98}, //REG_TC_IPRM_OpClk4KHz_0 //SCLK : 60Mhz
	{0x0F12, 0x278D}, //REG_TC_IPRM_MinOutRate4KHz_0 //PCLK Min : 81Mhz
	{0x0F12, 0x278D}, //REG_TC_IPRM_MaxOutRate4KHz_0 //PCLK Max : 81Mhz
	{0x0F12, 0x4F1A}, //REG_TC_IPRM_OpClk4KHz_1 //SCLK : 81Mhz
	{0x0F12, 0x278D}, //REG_TC_IPRM_MinOutRate4KHz_1 //PCLK Min : 81Mhz
	{0x0F12, 0x278D}, //REG_TC_IPRM_MaxOutRate4KHz_1 //PCLK Max : 81Mhz


//==================================================================================
// 11.Auto Flicker Detection
//==================================================================================

	{0x002A, 0x0F30},
	{0x0F12, 0x0001}, ///*AFC_D_ConvAccelerPower */


	{0x002A, 0x0F2A},
	{0x0F12, 0x0000}, ///*AFC_Default BIT[0] 1:60Hz 0:50Hz */
	{0x002A, 0x04E6},
	{0x0F12, 0x077F}, ///*REG_TC_DBG 7F: 60Hz  5F:50Hz */

//==================================================================================
// 12.AE Setting
//==================================================================================
//AE Target
	{0x002A, 0x1484},
	{0x0F12, 0x003C}, 	//TVAR_ae_BrAve//

//ae_StatMode bit[3] BLC has to be bypassed to prevent AE weight change especially backlight scene //
	{0x002A, 0x148A},
	{0x0F12, 0x000F}, 	//ae_StatMode//

	{0x002A, 0x058C},
	{0x0F12, 0x3520},

	{0x0F12, 0x0000},
	{0x0F12, 0xC350},
	{0x0F12, 0x0000},
	{0x0F12, 0x3520},
	{0x0F12, 0x0000},
	{0x0F12, 0xC350},
	{0x0F12, 0x0000},
	{0x002A, 0x059C},
	{0x0F12, 0x0470},
	{0x0F12, 0x0C00},
	{0x0F12, 0x0100},
	{0x0F12, 0x1000},


	{0x002A, 0x0544},
	{0x0F12, 0x0111},
	{0x0F12, 0x00EF},




	{0x002A, 0x0608},
	{0x0F12, 0x0001},
	{0x0F12, 0x0001},
	{0x0F12, 0x0800},
	{0x0F12, 0x0100},


	{0x0F12, 0x0001},
	{0x0F12, 0x0000},
	{0x0F12, 0x0A3C},
	{0x0F12, 0x0000},
	{0x0F12, 0x0D05},
	{0x0F12, 0x0000},
	{0x0F12, 0x4008},
	{0x0F12, 0x0000},
	{0x0F12, 0x7000},
	{0x0F12, 0x0000},
	{0x0F12, 0x9E00},
	{0x0F12, 0x0000},
	{0x0F12, 0xAD00},
	{0x0F12, 0x0001},
	{0x0F12, 0xF1D4},
	{0x0F12, 0x0002},
	{0x0F12, 0xDC00},
	{0x0F12, 0x0005},
	{0x0F12, 0xDC00},
	{0x0F12, 0x0005},


	{0x002A, 0x0638},
	{0x0F12, 0x0001},
	{0x0F12, 0x0000},
	{0x0F12, 0x0A3C},
	{0x0F12, 0x0000},
	{0x0F12, 0x0D05},
	{0x0F12, 0x0000},
	{0x0F12, 0x3408},
	{0x0F12, 0x0000},
	{0x0F12, 0x3408},
	{0x0F12, 0x0000},
	{0x0F12, 0x6810},
	{0x0F12, 0x0000},
	{0x0F12, 0x8214},
	{0x0F12, 0x0000},
	{0x0F12, 0xC350},
	{0x0F12, 0x0000},
	{0x0F12, 0xC350},
	{0x0F12, 0x0000},
	{0x0F12, 0xC350},
	{0x0F12, 0x0000},



	{0x002A, 0x0660},
	{0x0F12, 0x0650},
	{0x0F12, 0x0100},


// Lei Control	//
	{0x002A, 0x06B8},
	{0x0F12, 0x452C},
	{0x0F12, 0x0005}, 	 //lt_uMaxLei//

	{0x002A, 0x05D0},
	{0x0F12, 0x0000},

//==================================================================================
// 13.AE Weight (Normal)
//==================================================================================
	{0x002A, 0x1492},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0202},
	{0x0F12, 0x0202},
	{0x0F12, 0x0101},
	{0x0F12, 0x0201},
	{0x0F12, 0x0202},
	{0x0F12, 0x0202},
	{0x0F12, 0x0102},
	{0x0F12, 0x0201},
	{0x0F12, 0x0302},
	{0x0F12, 0x0203},
	{0x0F12, 0x0102},
	{0x0F12, 0x0201},
	{0x0F12, 0x0302},
	{0x0F12, 0x0203},
	{0x0F12, 0x0102},
	{0x0F12, 0x0101},
	{0x0F12, 0x0202},
	{0x0F12, 0x0202},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0201},
	{0x0F12, 0x0102},
	{0x0F12, 0x0101},
	{0x0F12, 0x0100},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0001},

//==================================================================================
// 14.Flash Setting
//==================================================================================

	{0x0028, 0x7000},
	{0x002A, 0x0484},
	{0x0F12, 0x0002}, //REG_TC_FLS_Mode

	{0x002A, 0x183A},
	{0x0F12, 0x0001}, //fls_afl_usCounter

	{0x002A, 0x17F6},
	{0x0F12, 0x023C}, //fls_afl_DefaultWPr //5C 54 54
	{0x0F12, 0x0230}, //fls_afl_DefaultWPb //28 30 28

	{0x002A, 0x1840},
	{0x0F12, 0x0001},	//fls_afl_bFlsMode

	{0x0F12, 0x0100},	//fls_afl_FlsAFIn_0_
	{0x0F12, 0x0120},	//fls_afl_FlsAFIn_1_
	{0x0F12, 0x0180},	//fls_afl_FlsAFIn_2_
	{0x0F12, 0x0200},	//fls_afl_FlsAFIn_3_
	{0x0F12, 0x0400},	//fls_afl_FlsAFIn_4_
	{0x0F12, 0x0800},	//fls_afl_FlsAFIn_5_
	{0x0F12, 0x0A00},	//fls_afl_FlsAFIn_6_
	{0x0F12, 0x1000},	//fls_afl_FlsAFIn_7_

	{0x0F12, 0x0100},	//fls_afl_FlsAFOut_0_
	{0x0F12, 0x00A0},	//fls_afl_FlsAFOut_1_
	{0x0F12, 0x0090},	//fls_afl_FlsAFOut_2_
	{0x0F12, 0x0080},	//fls_afl_FlsAFOut_3_
	{0x0F12, 0x0070},	//fls_afl_FlsAFOut_4_
	{0x0F12, 0x0045},	//fls_afl_FlsAFOut_5_
	{0x0F12, 0x0030},	//fls_afl_FlsAFOut_6_
	{0x0F12, 0x0010}, //fls_afl_FlsAFOut_7_

	{0x002A, 0x1884},
	{0x0F12, 0x0100},	//fls_afl_FlsNBOut_0_
	{0x0F12, 0x0100},	//fls_afl_FlsNBOut_1_
	{0x0F12, 0x0100},	//fls_afl_FlsNBOut_2_
	{0x0F12, 0x0100},	//fls_afl_FlsNBOut_3_
	{0x0F12, 0x0100},	//fls_afl_FlsNBOut_4_
	{0x0F12, 0x0100},	//fls_afl_FlsNBOut_5_
	{0x0F12, 0x0100},	//fls_afl_FlsNBOut_6_
	{0x0F12, 0x0100}, //fls_afl_FlsNBOut_7_

	{0x002A, 0x1826},

	{0x0F12, 0x0100},	//fls_afl_FlashWP_Weight2_0_
	{0x0F12, 0x00C0},	//fls_afl_FlashWP_Weight2_1_
	{0x0F12, 0x0080},	//fls_afl_FlashWP_Weight2_2_
	{0x0F12, 0x000A},	//fls_afl_FlashWP_Weight2_3_
	{0x0F12, 0x0000}, //fls_afl_FlashWP_Weight2_4_

	{0x0F12, 0x0030},	//fls_afl_FlashWP_Lei_Thres2_0_
	{0x0F12, 0x0040},	//fls_afl_FlashWP_Lei_Thres2_1_
	{0x0F12, 0x0048},	//fls_afl_FlashWP_Lei_Thres2_2_
	{0x0F12, 0x0050},	//fls_afl_FlashWP_Lei_Thres2_3_
	{0x0F12, 0x0060}, //fls_afl_FlashWP_Lei_Thres2_4_

	{0x002A, 0x4784},
	{0x0F12, 0x00A0},	// TNP_Regs_FlsWeightRIn  weight tune start in
	{0x0F12, 0x00C0},
	{0x0F12, 0x00D0},
	{0x0F12, 0x0100},
	{0x0F12, 0x0200},
	{0x0F12, 0x0300},

	{0x0F12, 0x0088},	// TNP_Regs_FlsWeightROut  weight tune start out
	{0x0F12, 0x00B0},
	{0x0F12, 0x00C0},
	{0x0F12, 0x0100},
	{0x0F12, 0x0200},
	{0x0F12, 0x0300},

	{0x002A, 0x479C},

	{0x0F12, 0x0120},	//Fls  BRIn
	{0x0F12, 0x0150},
	{0x0F12, 0x0200},

	{0x0F12, 0x003C},	// Fls  BROut
	{0x0F12, 0x003B},
	{0x0F12, 0x002B}, 	//brightness

//==================================================================================
// 15.CCM Setting
//==================================================================================
	 //CCM
	{0x002A, 0x08A6},
	{0x0F12, 0x00C0},
	{0x0F12, 0x0100},
	{0x0F12, 0x0125},
	{0x0F12, 0x015F},
	{0x0F12, 0x017C},
	{0x0F12, 0x0194},

	{0x0F12, 0x0001}, 	//wbt_bUseOutdoorCCM

	{0x002A, 0x0898},
	{0x0F12, 0x4800},
	{0x0F12, 0x7000}, 	//TVAR_wbt_pBaseCcms
	{0x002A, 0x08A0},
	{0x0F12, 0x48D8},
	{0x0F12, 0x7000}, 	//TVAR_wbt_pOutdoorCcm

	{0x002A, 0x4800},   // Horizon
	{0x0F12, 0x0208},   //TVAR_wbt_pBaseCcms[0]
	{0x0F12, 0xFFB5},   //TVAR_wbt_pBaseCcms[1]
	{0x0F12, 0xFFE8},   //TVAR_wbt_pBaseCcms[2]
	{0x0F12, 0xFF99},   //TVAR_wbt_pBaseCcms[3]
	{0x0F12, 0x00EB},   //TVAR_wbt_pBaseCcms[4]
	{0x0F12, 0xFFAD},   //TVAR_wbt_pBaseCcms[5]
	{0x0F12, 0x0022},   //TVAR_wbt_pBaseCcms[6]
	{0x0F12, 0xFFEA},   //TVAR_wbt_pBaseCcms[7]
	{0x0F12, 0x01C2},   //TVAR_wbt_pBaseCcms[8]
	{0x0F12, 0x00C6},   //TVAR_wbt_pBaseCcms[9]
	{0x0F12, 0x0095},   //TVAR_wbt_pBaseCcms[10]
	{0x0F12, 0xFEFD},   //TVAR_wbt_pBaseCcms[11]
	{0x0F12, 0x0206},   //TVAR_wbt_pBaseCcms[12]
	{0x0F12, 0xFF7F},   //TVAR_wbt_pBaseCcms[13]
	{0x0F12, 0x0191},   //TVAR_wbt_pBaseCcms[14]
	{0x0F12, 0xFF06},   //TVAR_wbt_pBaseCcms[15]
	{0x0F12, 0x01BA},   //TVAR_wbt_pBaseCcms[16]
	{0x0F12, 0x0108},   //TVAR_wbt_pBaseCcms[17]

	{0x0F12, 0x0208},		//TVAR_wbt_pBaseCcms[18]/* inca A */
	{0x0F12, 0xFFB5},   //TVAR_wbt_pBaseCcms[19]
	{0x0F12, 0xFFE8},   //TVAR_wbt_pBaseCcms[20]
	{0x0F12, 0xFF82},   //TVAR_wbt_pBaseCcms[21]
	{0x0F12, 0x0115},   //TVAR_wbt_pBaseCcms[22]
	{0x0F12, 0xFF9A},   //TVAR_wbt_pBaseCcms[23]
	{0x0F12, 0xFFFB},   //TVAR_wbt_pBaseCcms[24]
	{0x0F12, 0xFFB4},   //TVAR_wbt_pBaseCcms[25]
	{0x0F12, 0x021E},   //TVAR_wbt_pBaseCcms[26]
	{0x0F12, 0x00C6},   //TVAR_wbt_pBaseCcms[27]
	{0x0F12, 0x0095},   //TVAR_wbt_pBaseCcms[28]
	{0x0F12, 0xFEFD},   //TVAR_wbt_pBaseCcms[29]
	{0x0F12, 0x0206},   //TVAR_wbt_pBaseCcms[30]
	{0x0F12, 0xFF7F},   //TVAR_wbt_pBaseCcms[31]
	{0x0F12, 0x0191},   //TVAR_wbt_pBaseCcms[32]
	{0x0F12, 0xFF06},   //TVAR_wbt_pBaseCcms[33]
	{0x0F12, 0x01BA},   //TVAR_wbt_pBaseCcms[34]
	{0x0F12, 0x0108},   //TVAR_wbt_pBaseCcms[35]

	{0x0F12, 0x0208},	  //TVAR_wbt_pBaseCcms[36] /*WW*/
	{0x0F12, 0xFFB5},   //TVAR_wbt_pBaseCcms[37]
	{0x0F12, 0xFFE8},   //TVAR_wbt_pBaseCcms[38]
	{0x0F12, 0xFF99},   //TVAR_wbt_pBaseCcms[39]
	{0x0F12, 0x00EB},   //TVAR_wbt_pBaseCcms[40]
	{0x0F12, 0xFFAD},   //TVAR_wbt_pBaseCcms[41]
	{0x0F12, 0x0022},   //TVAR_wbt_pBaseCcms[42]
	{0x0F12, 0xFFEA},   //TVAR_wbt_pBaseCcms[43]
	{0x0F12, 0x01C2},   //TVAR_wbt_pBaseCcms[44]
	{0x0F12, 0x00C6},   //TVAR_wbt_pBaseCcms[45]
	{0x0F12, 0x0095},   //TVAR_wbt_pBaseCcms[46]
	{0x0F12, 0xFEFD},   //TVAR_wbt_pBaseCcms[47]
	{0x0F12, 0x0206},   //TVAR_wbt_pBaseCcms[48]
	{0x0F12, 0xFF7F},   //TVAR_wbt_pBaseCcms[49]
	{0x0F12, 0x0191},   //TVAR_wbt_pBaseCcms[50]
	{0x0F12, 0xFF06},   //TVAR_wbt_pBaseCcms[51]
	{0x0F12, 0x01BA},   //TVAR_wbt_pBaseCcms[52]
	{0x0F12, 0x0108},   //TVAR_wbt_pBaseCcms[53]

	{0x0F12, 0x0204},  	//TVAR_wbt_pBaseCcms[54] // CW
	{0x0F12, 0xFFB2},  	//TVAR_wbt_pBaseCcms[55]
	{0x0F12, 0xFFF5},  	//TVAR_wbt_pBaseCcms[56]
	{0x0F12, 0xFEF1},  	//TVAR_wbt_pBaseCcms[57]
	{0x0F12, 0x014E},  	//TVAR_wbt_pBaseCcms[58]
	{0x0F12, 0xFF18},  	//TVAR_wbt_pBaseCcms[59]
	{0x0F12, 0xFFE6},  	//TVAR_wbt_pBaseCcms[60]
	{0x0F12, 0xFFDD},  	//TVAR_wbt_pBaseCcms[61]
	{0x0F12, 0x01B2},  	//TVAR_wbt_pBaseCcms[62]
	{0x0F12, 0x00F2},  	//TVAR_wbt_pBaseCcms[63]
	{0x0F12, 0x00CA},  	//TVAR_wbt_pBaseCcms[64]
	{0x0F12, 0xFF48},  	//TVAR_wbt_pBaseCcms[65]
	{0x0F12, 0x0151},  	//TVAR_wbt_pBaseCcms[66]
	{0x0F12, 0xFF50},  	//TVAR_wbt_pBaseCcms[67]
	{0x0F12, 0x0147},  	//TVAR_wbt_pBaseCcms[68]
	{0x0F12, 0xFF75},  	//TVAR_wbt_pBaseCcms[69]
	{0x0F12, 0x0187},  	//TVAR_wbt_pBaseCcms[70]
	{0x0F12, 0x01BF},  	//TVAR_wbt_pBaseCcms[71]

	{0x0F12, 0x0204},  	//TVAR_wbt_pBaseCcms[72] // D50
	{0x0F12, 0xFFB2},  	//TVAR_wbt_pBaseCcms[73]
	{0x0F12, 0xFFF5},  	//TVAR_wbt_pBaseCcms[74]
	{0x0F12, 0xFEF1},  	//TVAR_wbt_pBaseCcms[75]
	{0x0F12, 0x014E},  	//TVAR_wbt_pBaseCcms[76]
	{0x0F12, 0xFF18},  	//TVAR_wbt_pBaseCcms[77]
	{0x0F12, 0xFFD9},  	//TVAR_wbt_pBaseCcms[78]
	{0x0F12, 0xFFBA},  	//TVAR_wbt_pBaseCcms[79]
	{0x0F12, 0x01D4},  	//TVAR_wbt_pBaseCcms[80]
	{0x0F12, 0x00F2},  	//TVAR_wbt_pBaseCcms[81]
	{0x0F12, 0x00CA},  	//TVAR_wbt_pBaseCcms[82]
	{0x0F12, 0xFF48},  	//TVAR_wbt_pBaseCcms[83]
	{0x0F12, 0x0151},  	//TVAR_wbt_pBaseCcms[84]
	{0x0F12, 0xFF50},  	//TVAR_wbt_pBaseCcms[85]
	{0x0F12, 0x0147},  	//TVAR_wbt_pBaseCcms[86]
	{0x0F12, 0xFF75},  	//TVAR_wbt_pBaseCcms[87]
	{0x0F12, 0x0187},  	//TVAR_wbt_pBaseCcms[88]
	{0x0F12, 0x01BF},  	//TVAR_wbt_pBaseCcms[89]

	{0x0F12, 0x0204},  	//TVAR_wbt_pBaseCcms[90] // D65
	{0x0F12, 0xFFB2},  	//TVAR_wbt_pBaseCcms[91]
	{0x0F12, 0xFFF5},  	//TVAR_wbt_pBaseCcms[92]
	{0x0F12, 0xFEF1},  	//TVAR_wbt_pBaseCcms[93]
	{0x0F12, 0x014E},  	//TVAR_wbt_pBaseCcms[94]
	{0x0F12, 0xFF18},  	//TVAR_wbt_pBaseCcms[95]
	{0x0F12, 0xFFD9},  	//TVAR_wbt_pBaseCcms[96]
	{0x0F12, 0xFFBA},  	//TVAR_wbt_pBaseCcms[97]
	{0x0F12, 0x01D4},  	//TVAR_wbt_pBaseCcms[98]
	{0x0F12, 0x00F2},  	//TVAR_wbt_pBaseCcms[99]
	{0x0F12, 0x00CA},  	//TVAR_wbt_pBaseCcms[100]
	{0x0F12, 0xFF48},  	//TVAR_wbt_pBaseCcms[101]
	{0x0F12, 0x0151},  	//TVAR_wbt_pBaseCcms[102]
	{0x0F12, 0xFF50},  	//TVAR_wbt_pBaseCcms[103]
	{0x0F12, 0x0147},  	//TVAR_wbt_pBaseCcms[104]
	{0x0F12, 0xFF75},  	//TVAR_wbt_pBaseCcms[105]
	{0x0F12, 0x0187},  	//TVAR_wbt_pBaseCcms[106]
	{0x0F12, 0x01BF},  	//TVAR_wbt_pBaseCcms[107]

	{0x0F12, 0x01E5}, 	//TVAR_wbt_pOutdoorCcm[0]
	{0x0F12, 0xFFA4}, 	//TVAR_wbt_pOutdoorCcm[1]
	{0x0F12, 0xFFDC}, 	//TVAR_wbt_pOutdoorCcm[2]
	{0x0F12, 0xFE69},   //TVAR_wbt_pOutdoorCcm[3]
	{0x0F12, 0x0130},   //TVAR_wbt_pOutdoorCcm[4]
	{0x0F12, 0xFF55},   //TVAR_wbt_pOutdoorCcm[5]
	{0x0F12, 0xFFD2}, 	//TVAR_wbt_pOutdoorCcm[6]
	{0x0F12, 0xFFDF}, 	//TVAR_wbt_pOutdoorCcm[7]
	{0x0F12, 0x0236}, 	//TVAR_wbt_pOutdoorCcm[8]
	{0x0F12, 0x00C7}, 	//TVAR_wbt_pOutdoorCcm[9]
	{0x0F12, 0x0118}, 	//TVAR_wbt_pOutdoorCcm[10]
	{0x0F12, 0xFF38}, 	//TVAR_wbt_pOutdoorCcm[11]
	{0x0F12, 0x01CE}, 	//TVAR_wbt_pOutdoorCcm[12]
	{0x0F12, 0xFF83}, 	//TVAR_wbt_pOutdoorCcm[13]
	{0x0F12, 0x0195}, 	//TVAR_wbt_pOutdoorCcm[14]
	{0x0F12, 0xFEF3}, 	//TVAR_wbt_pOutdoorCcm[15]
	{0x0F12, 0x0126}, 	//TVAR_wbt_pOutdoorCcm[16]
	{0x0F12, 0x0162},   //TVAR_wbt_pOutdoorCcm[17]

//===================================================================
// 16.GAMMA
//===================================================================
	//RGB Indoor Gamma
	{0x002A, 0x0734},  ///*R*/
	{0x0F12, 0x0001},
	{0x0F12, 0x0003},
	{0x0F12, 0x000F},
	{0x0F12, 0x0028},
	{0x0F12, 0x0066},
	{0x0F12, 0x00D9},
	{0x0F12, 0x0138},
	{0x0F12, 0x0163},
	{0x0F12, 0x0189},
	{0x0F12, 0x01C6},
	{0x0F12, 0x01F8},
	{0x0F12, 0x0222},
	{0x0F12, 0x0247},
	{0x0F12, 0x0282},
	{0x0F12, 0x02B5},
	{0x0F12, 0x030F},
	{0x0F12, 0x035F},
	{0x0F12, 0x03A2},
	{0x0F12, 0x03D8},
	{0x0F12, 0x03FF},
	{0x0F12, 0x0001},//  /*G*/
	{0x0F12, 0x0003},
	{0x0F12, 0x000F},
	{0x0F12, 0x0028},
	{0x0F12, 0x0066},
	{0x0F12, 0x00D9},
	{0x0F12, 0x0138},
	{0x0F12, 0x0163},
	{0x0F12, 0x0189},
	{0x0F12, 0x01C6},
	{0x0F12, 0x01F8},
	{0x0F12, 0x0222},
	{0x0F12, 0x0247},
	{0x0F12, 0x0282},
	{0x0F12, 0x02B5},
	{0x0F12, 0x030F},
	{0x0F12, 0x035F},
	{0x0F12, 0x03A2},
	{0x0F12, 0x03D8},
	{0x0F12, 0x03FF},
	{0x0F12, 0x0001}, // /*B*/
	{0x0F12, 0x0003},
	{0x0F12, 0x000F},
	{0x0F12, 0x0028},
	{0x0F12, 0x0066},
	{0x0F12, 0x00D9},
	{0x0F12, 0x0138},
	{0x0F12, 0x0163},
	{0x0F12, 0x0189},
	{0x0F12, 0x01C6},
	{0x0F12, 0x01F8},
	{0x0F12, 0x0222},
	{0x0F12, 0x0247},
	{0x0F12, 0x0282},
	{0x0F12, 0x02B5},
	{0x0F12, 0x030F},
	{0x0F12, 0x035F},
	{0x0F12, 0x03A2},
	{0x0F12, 0x03D8},
	{0x0F12, 0x03FF},


	{0x0F12, 0x0007},//RED
	{0x0F12, 0x0012},
	{0x0F12, 0x0020},
	{0x0F12, 0x0038},
	{0x0F12, 0x0071},
	{0x0F12, 0x00DA},
	{0x0F12, 0x0137},
	{0x0F12, 0x0161},
	{0x0F12, 0x0187},
	{0x0F12, 0x01C3},
	{0x0F12, 0x01FE},
	{0x0F12, 0x021B},
	{0x0F12, 0x0245},
	{0x0F12, 0x028C},
	{0x0F12, 0x02CB},
	{0x0F12, 0x0325},
	{0x0F12, 0x0365},
	{0x0F12, 0x039A},
	{0x0F12, 0x03C7},
	{0x0F12, 0x03F4},

	{0x0F12, 0x0005},
	{0x0F12, 0x0010},
	{0x0F12, 0x001E},
	{0x0F12, 0x0036},
	{0x0F12, 0x006F},
	{0x0F12, 0x00D8},
	{0x0F12, 0x0135},
	{0x0F12, 0x015F},
	{0x0F12, 0x0185},
	{0x0F12, 0x01C1},
	{0x0F12, 0x01F3},
	{0x0F12, 0x0220},
	{0x0F12, 0x024A},
	{0x0F12, 0x0291},
	{0x0F12, 0x02D0},
	{0x0F12, 0x032A},
	{0x0F12, 0x036A},
	{0x0F12, 0x039F},
	{0x0F12, 0x03CC},
	{0x0F12, 0x03F9},

	{0x0F12, 0x0003},//Blue
	{0x0F12, 0x000E},
	{0x0F12, 0x001C},
	{0x0F12, 0x0034},
	{0x0F12, 0x006D},
	{0x0F12, 0x00D6},
	{0x0F12, 0x0133},
	{0x0F12, 0x015D},
	{0x0F12, 0x0183},
	{0x0F12, 0x01BF},
	{0x0F12, 0x01F5},
	{0x0F12, 0x0222},
	{0x0F12, 0x024C},
	{0x0F12, 0x0293},
	{0x0F12, 0x02D2},
	{0x0F12, 0x032C},
	{0x0F12, 0x036C},
	{0x0F12, 0x03A1},
	{0x0F12, 0x03CE},
	{0x0F12, 0x03FB},

//==================================================================================
// 17.AFIT
//==================================================================================
	{0x002A, 0x0944}, // Noise Index setting
	{0x0F12, 0x0050}, 	  	///*afit_uNoiseIndInDoor */
	{0x0F12, 0x00B0}, 		///*afit_uNoiseIndInDoor */
	{0x0F12, 0x0196}, 		///*afit_uNoiseIndInDoor */
	{0x0F12, 0x0245}, 		///*afit_uNoiseIndInDoor */
	{0x0F12, 0x0300}, 		///*afit_uNoiseIndInDoor */
	{0x002A, 0x0976},
	{0x0F12, 0x0070}, 	  	///*afit_usGamutTh */
	{0x0F12, 0x0005}, 		///*afit_usNeargrayOffset */
	{0x0F12, 0x0000}, 		///*afit_bUseSenBpr */
	{0x0F12, 0x01CC}, 		///*afit_usBprThr_0_ */
	{0x0F12, 0x01CC}, 		///*afit_usBprThr_1_ */
	{0x0F12, 0x01CC}, 		///*afit_usBprThr_2_ */
	{0x0F12, 0x01CC}, 		///*afit_usBprThr_3_ */
	{0x0F12, 0x01CC}, 		///*afit_usBprThr_4_ */
	{0x0F12, 0x0180}, 		///*afit_NIContrastAFITValue */
	{0x0F12, 0x0196}, 		///*afit_NIContrastTh */
	{0x002A, 0x0938},
	{0x0F12, 0x0000},     	///* on/off AFIT by NB option */
	{0x0F12, 0x0014}, 		///*SARR_uNormBrInDoor */
	{0x0F12, 0x00D2}, 		///*SARR_uNormBrInDoor */
	{0x0F12, 0x0384}, 		///*SARR_uNormBrInDoor */
	{0x0F12, 0x07D0}, 		///*SARR_uNormBrInDoor */
	{0x0F12, 0x1388}, 		///*SARR_uNormBrInDoor */
	{0x002A, 0x098C},
	{0x0F12, 0xFFEC},    //7000098C AFIT16_BRIGHTNESS
	{0x0F12, 0x0000},    //7000098E AFIT16_CONTRAST
	{0x0F12, 0x0000},    //70000990 AFIT16_SATURATION
	{0x0F12, 0x0000},    //70000992 AFIT16_SHARP_BLUR
	{0x0F12, 0x0000},    //70000994 AFIT16_GLAMOUR
	{0x0F12, 0x00C0},    //70000996 AFIT16_bnr_edge_high
	{0x0F12, 0x0064},    //70000998 AFIT16_postdmsc_iLowBright
	{0x0F12, 0x0384},    //7000099A AFIT16_postdmsc_iHighBright
	{0x0F12, 0x005F},    //7000099C AFIT16_postdmsc_iLowSat
	{0x0F12, 0x01F4},    //7000099E AFIT16_postdmsc_iHighSat
	{0x0F12, 0x0070},    //700009A0 AFIT16_postdmsc_iTune
	{0x0F12, 0x0040},    //700009A2 AFIT16_yuvemix_mNegRanges_0
	{0x0F12, 0x00A0},    //700009A4 AFIT16_yuvemix_mNegRanges_1
	{0x0F12, 0x0100},    //700009A6 AFIT16_yuvemix_mNegRanges_2
	{0x0F12, 0x0010},    //700009A8 AFIT16_yuvemix_mPosRanges_0
	{0x0F12, 0x0040},    //700009AA AFIT16_yuvemix_mPosRanges_1
	{0x0F12, 0x00A0},    //700009AC AFIT16_yuvemix_mPosRanges_2
	{0x0F12, 0x1430},    //700009AE AFIT8_bnr_edge_low  [7:0] AFIT8_bnr_repl_thresh
	{0x0F12, 0x0201},    //700009B0 AFIT8_bnr_repl_force  [7:0] AFIT8_bnr_iHotThreshHigh
	{0x0F12, 0x0204},    //700009B2 AFIT8_bnr_iHotThreshLow   [7:0] AFIT8_bnr_iColdThreshHigh
	{0x0F12, 0x3604},    //700009B4 AFIT8_bnr_iColdThreshLow   [7:0] AFIT8_bnr_DispTH_Low
	{0x0F12, 0x032A},    //700009B6 AFIT8_bnr_DispTH_High   [7:0] AFIT8_bnr_DISP_Limit_Low
	{0x0F12, 0x0103},    //700009B8 AFIT8_bnr_DISP_Limit_High   [7:0] AFIT8_bnr_iDistSigmaMin
	{0x0F12, 0x1205},    //700009BA AFIT8_bnr_iDistSigmaMax   [7:0] AFIT8_bnr_iDiffSigmaLow
	{0x0F12, 0x400D},    //700009BC AFIT8_bnr_iDiffSigmaHigh   [7:0] AFIT8_bnr_iNormalizedSTD_TH
	{0x0F12, 0x0080},    //700009BE AFIT8_bnr_iNormalizedSTD_Limit   [7:0] AFIT8_bnr_iDirNRTune
	{0x0F12, 0x2080},    //700009C0 AFIT8_bnr_iDirMinThres   [7:0] AFIT8_bnr_iDirFltDiffThresHigh
	{0x0F12, 0x3840},    //700009C2 AFIT8_bnr_iDirFltDiffThresLow   [7:0] AFIT8_bnr_iDirSmoothPowerHigh
	{0x0F12, 0x0638},    //700009C4 AFIT8_bnr_iDirSmoothPowerLow   [7:0] AFIT8_bnr_iLowMaxSlopeAllowed
	{0x0F12, 0x0306},    //700009C6 AFIT8_bnr_iHighMaxSlopeAllowed   [7:0] AFIT8_bnr_iLowSlopeThresh
	{0x0F12, 0x2003},    //700009C8 AFIT8_bnr_iHighSlopeThresh   [7:0] AFIT8_bnr_iSlopenessTH
	{0x0F12, 0xFF01},    //700009CA AFIT8_bnr_iSlopeBlurStrength   [7:0] AFIT8_bnr_iSlopenessLimit
	{0x0F12, 0x0000},    //700009CC AFIT8_bnr_AddNoisePower1   [7:0] AFIT8_bnr_AddNoisePower2
	{0x0F12, 0x0400},    //700009CE AFIT8_bnr_iRadialTune   [7:0] AFIT8_bnr_iRadialPower
	{0x0F12, 0x245A},    //700009D0 AFIT8_bnr_iRadialLimit   [7:0] AFIT8_ee_iFSMagThLow
	{0x0F12, 0x102A},    //700009D2 AFIT8_ee_iFSMagThHigh   [7:0] AFIT8_ee_iFSVarThLow
	{0x0F12, 0x000B},    //700009D4 AFIT8_ee_iFSVarThHigh   [7:0] AFIT8_ee_iFSThLow
	{0x0F12, 0x0600},    //700009D6 AFIT8_ee_iFSThHigh   [7:0] AFIT8_ee_iFSmagPower
	{0x0F12, 0x5A0F},    //700009D8 AFIT8_ee_iFSVarCountTh   [7:0] AFIT8_ee_iRadialLimit
	{0x0F12, 0x0505},    //700009DA AFIT8_ee_iRadialPower   [7:0] AFIT8_ee_iSmoothEdgeSlope
	{0x0F12, 0x1802},    //700009DC AFIT8_ee_iROADThres   [7:0] AFIT8_ee_iROADMaxNR
	{0x0F12, 0x0000},    //700009DE AFIT8_ee_iROADSubMaxNR   [7:0] AFIT8_ee_iROADSubThres
	{0x0F12, 0x2006},    //700009E0 AFIT8_ee_iROADNeiThres   [7:0] AFIT8_ee_iROADNeiMaxNR
	{0x0F12, 0x3028},    //700009E2 AFIT8_ee_iSmoothEdgeThres   [7:0] AFIT8_ee_iMSharpen
	{0x0F12, 0x0418},    //700009E4 AFIT8_ee_iWSharpen   [7:0] AFIT8_ee_iMShThresh
	{0x0F12, 0x0101},    //700009E6 AFIT8_ee_iWShThresh   [7:0] AFIT8_ee_iReduceNegative
	{0x0F12, 0x0800},    //700009E8 AFIT8_ee_iEmbossCentAdd   [7:0] AFIT8_ee_iShDespeckle
	{0x0F12, 0x1804},    //700009EA AFIT8_ee_iReduceEdgeThresh   [7:0] AFIT8_dmsc_iEnhThresh
	{0x0F12, 0x4008},    //700009EC AFIT8_dmsc_iDesatThresh   [7:0] AFIT8_dmsc_iDemBlurHigh
	{0x0F12, 0x0540},    //700009EE AFIT8_dmsc_iDemBlurLow   [7:0] AFIT8_dmsc_iDemBlurRange
	{0x0F12, 0x8006},    //700009F0 AFIT8_dmsc_iDecisionThresh   [7:0] AFIT8_dmsc_iCentGrad
	{0x0F12, 0x0020},    //700009F2 AFIT8_dmsc_iMonochrom   [7:0] AFIT8_dmsc_iGBDenoiseVal
	{0x0F12, 0x0000},    //700009F4 AFIT8_dmsc_iGRDenoiseVal   [7:0] AFIT8_dmsc_iEdgeDesatThrHigh
	{0x0F12, 0x1800},    //700009F6 AFIT8_dmsc_iEdgeDesatThrLow   [7:0] AFIT8_dmsc_iEdgeDesat
	{0x0F12, 0x0000},    //700009F8 AFIT8_dmsc_iNearGrayDesat   [7:0] AFIT8_dmsc_iEdgeDesatLimit
	{0x0F12, 0x1E10},    //700009FA AFIT8_postdmsc_iBCoeff   [7:0] AFIT8_postdmsc_iGCoeff
	{0x0F12, 0x000B},    //700009FC AFIT8_postdmsc_iWideMult   [7:0] AFIT8_yuvemix_mNegSlopes_0
	{0x0F12, 0x0607},    //700009FE AFIT8_yuvemix_mNegSlopes_1   [7:0] AFIT8_yuvemix_mNegSlopes_2
	{0x0F12, 0x0005},    //70000A00 AFIT8_yuvemix_mNegSlopes_3   [7:0] AFIT8_yuvemix_mPosSlopes_0
	{0x0F12, 0x0607},    //70000A02 AFIT8_yuvemix_mPosSlopes_1   [7:0] AFIT8_yuvemix_mPosSlopes_2
	{0x0F12, 0x0405},    //70000A04 AFIT8_yuvemix_mPosSlopes_3   [7:0] AFIT8_yuviirnr_iXSupportY
	{0x0F12, 0x0205},    //70000A06 AFIT8_yuviirnr_iXSupportUV   [7:0] AFIT8_yuviirnr_iLowYNorm
	{0x0F12, 0x0304},    //70000A08 AFIT8_yuviirnr_iHighYNorm   [7:0] AFIT8_yuviirnr_iLowUVNorm
	{0x0F12, 0x0409},    //70000A0A AFIT8_yuviirnr_iHighUVNorm   [7:0] AFIT8_yuviirnr_iYNormShift
	{0x0F12, 0x0306},    //70000A0C AFIT8_yuviirnr_iUVNormShift   [7:0] AFIT8_yuviirnr_iVertLength_Y
	{0x0F12, 0x0407},    //70000A0E AFIT8_yuviirnr_iVertLength_UV   [7:0] AFIT8_yuviirnr_iDiffThreshL_Y
	{0x0F12, 0x1C04},    //70000A10 AFIT8_yuviirnr_iDiffThreshH_Y   [7:0] AFIT8_yuviirnr_iDiffThreshL_UV
	{0x0F12, 0x0214},    //70000A12 AFIT8_yuviirnr_iDiffThreshH_UV   [7:0] AFIT8_yuviirnr_iMaxThreshL_Y
	{0x0F12, 0x1002},    //70000A14 AFIT8_yuviirnr_iMaxThreshH_Y   [7:0] AFIT8_yuviirnr_iMaxThreshL_UV
	{0x0F12, 0x0610},    //70000A16 AFIT8_yuviirnr_iMaxThreshH_UV   [7:0] AFIT8_yuviirnr_iYNRStrengthL
	{0x0F12, 0x1A02},    //70000A18 AFIT8_yuviirnr_iYNRStrengthH   [7:0] AFIT8_yuviirnr_iUVNRStrengthL
	{0x0F12, 0x4A18},    //70000A1A AFIT8_yuviirnr_iUVNRStrengthH   [7:0] AFIT8_byr_gras_iShadingPower
	{0x0F12, 0x0080},    //70000A1C AFIT8_RGBGamma2_iLinearity   [7:0] AFIT8_RGBGamma2_iDarkReduce
	{0x0F12, 0x0350},    //70000A1E AFIT8_ccm_oscar_iSaturation   [7:0] AFIT8_RGB2YUV_iYOffset
	{0x0F12, 0x0180},    //70000A20 AFIT8_RGB2YUV_iRGBGain   [7:0] AFIT8_bnr_nClustLevel_H
	{0x0F12, 0x0A0A},    //70000A22 AFIT8_bnr_iClustMulT_H   [7:0] AFIT8_bnr_iClustMulT_C
	{0x0F12, 0x0101},    //70000A24 AFIT8_bnr_iClustThresh_H   [7:0] AFIT8_bnr_iClustThresh_C
	{0x0F12, 0x2A36},    //70000A26 AFIT8_bnr_iDenThreshLow   [7:0] AFIT8_bnr_iDenThreshHigh
	{0x0F12, 0x6024},    //70000A28 AFIT8_ee_iLowSharpPower   [7:0] AFIT8_ee_iHighSharpPower
	{0x0F12, 0x2A36},    //70000A2A AFIT8_ee_iLowShDenoise   [7:0] AFIT8_ee_iHighShDenoise
	{0x0F12, 0xFFFF},    //70000A2C AFIT8_ee_iLowSharpClamp   [7:0] AFIT8_ee_iHighSharpClamp
	{0x0F12, 0x0808},    //70000A2E AFIT8_ee_iReduceEdgeMinMult   [7:0] AFIT8_ee_iReduceEdgeSlope
	{0x0F12, 0x0A01},    //70000A30 AFIT8_bnr_nClustLevel_H_Bin   [7:0] AFIT8_bnr_iClustMulT_H_Bin
	{0x0F12, 0x010A},    //70000A32 AFIT8_bnr_iClustMulT_C_Bin   [7:0] AFIT8_bnr_iClustThresh_H_Bin
	{0x0F12, 0x3601},    //70000A34 AFIT8_bnr_iClustThresh_C_Bin   [7:0] AFIT8_bnr_iDenThreshLow_Bin
	{0x0F12, 0x242A},    //70000A36 AFIT8_bnr_iDenThreshHigh_Bin   [7:0] AFIT8_ee_iLowSharpPower_Bin
	{0x0F12, 0x3660},    //70000A38 AFIT8_ee_iHighSharpPower_Bin   [7:0] AFIT8_ee_iLowShDenoise_Bin
	{0x0F12, 0xFF2A},    //70000A3A AFIT8_ee_iHighShDenoise_Bin   [7:0] AFIT8_ee_iLowSharpClamp_Bin
	{0x0F12, 0x08FF},    //70000A3C AFIT8_ee_iHighSharpClamp_Bin   [7:0] AFIT8_ee_iReduceEdgeMinMult_Bin
	{0x0F12, 0x0008},    //70000A3E AFIT8_ee_iReduceEdgeSlope_Bin [7:0]
	{0x0F12, 0x0001},    //70000A40 AFITB_bnr_nClustLevel_C      [0]
	{0x0F12, 0x0000},    //70000A42 AFIT16_BRIGHTNESS
	{0x0F12, 0x0000},    //70000A44 AFIT16_CONTRAST
	{0x0F12, 0xFFFB},    // 0000},
	{0x0F12, 0x0000},    //70000A48 AFIT16_SHARP_BLUR
	{0x0F12, 0x0000},    //70000A4A AFIT16_GLAMOUR
	{0x0F12, 0x00C0},    //70000A4C AFIT16_bnr_edge_high
	{0x0F12, 0x0064},    //70000A4E AFIT16_postdmsc_iLowBright
	{0x0F12, 0x0384},    //70000A50 AFIT16_postdmsc_iHighBright
	{0x0F12, 0x0051},    //70000A52 AFIT16_postdmsc_iLowSat
	{0x0F12, 0x01F4},    //70000A54 AFIT16_postdmsc_iHighSat
	{0x0F12, 0x0070},    //70000A56 AFIT16_postdmsc_iTune
	{0x0F12, 0x0040},    //70000A58 AFIT16_yuvemix_mNegRanges_0
	{0x0F12, 0x00A0},    //70000A5A AFIT16_yuvemix_mNegRanges_1
	{0x0F12, 0x0100},    //70000A5C AFIT16_yuvemix_mNegRanges_2
	{0x0F12, 0x0010},    //70000A5E AFIT16_yuvemix_mPosRanges_0
	{0x0F12, 0x0060},    //70000A60 AFIT16_yuvemix_mPosRanges_1
	{0x0F12, 0x0100},    //70000A62 AFIT16_yuvemix_mPosRanges_2
	{0x0F12, 0x1430},    //70000A64 AFIT8_bnr_edge_low  [7:0] AFIT8_bnr_repl_thresh
	{0x0F12, 0x0201},    //70000A66 AFIT8_bnr_repl_force  [7:0] AFIT8_bnr_iHotThreshHigh
	{0x0F12, 0x0204},    //70000A68 AFIT8_bnr_iHotThreshLow   [7:0] AFIT8_bnr_iColdThreshHigh
	{0x0F12, 0x2404},    //70000A6A AFIT8_bnr_iColdThreshLow   [7:0] AFIT8_bnr_DispTH_Low
	{0x0F12, 0x031B},    //70000A6C AFIT8_bnr_DispTH_High   [7:0] AFIT8_bnr_DISP_Limit_Low
	{0x0F12, 0x0103},    //70000A6E AFIT8_bnr_DISP_Limit_High   [7:0] AFIT8_bnr_iDistSigmaMin
	{0x0F12, 0x1004},    //70000A70 AFIT8_bnr_iDistSigmaMax   [7:0] AFIT8_bnr_iDiffSigmaLow
	{0x0F12, 0x3A0C},    //70000A72 AFIT8_bnr_iDiffSigmaHigh   [7:0] AFIT8_bnr_iNormalizedSTD_TH
	{0x0F12, 0x0070},    //70000A74 AFIT8_bnr_iNormalizedSTD_Limit   [7:0] AFIT8_bnr_iDirNRTune
	{0x0F12, 0x1C80},    //70000A76 AFIT8_bnr_iDirMinThres   [7:0] AFIT8_bnr_iDirFltDiffThresHigh
	{0x0F12, 0x3030},    //70000A78 AFIT8_bnr_iDirFltDiffThresLow   [7:0] AFIT8_bnr_iDirSmoothPowerHigh
	{0x0F12, 0x0630},    //70000A7A AFIT8_bnr_iDirSmoothPowerLow   [7:0] AFIT8_bnr_iLowMaxSlopeAllowed
	{0x0F12, 0x0306},    //70000A7C AFIT8_bnr_iHighMaxSlopeAllowed   [7:0] AFIT8_bnr_iLowSlopeThresh
	{0x0F12, 0x2003},    //70000A7E AFIT8_bnr_iHighSlopeThresh   [7:0] AFIT8_bnr_iSlopenessTH
	{0x0F12, 0xFF01},    //70000A80 AFIT8_bnr_iSlopeBlurStrength   [7:0] AFIT8_bnr_iSlopenessLimit
	{0x0F12, 0x0404},    //70000A82 AFIT8_bnr_AddNoisePower1   [7:0] AFIT8_bnr_AddNoisePower2
	{0x0F12, 0x0300},    //70000A84 AFIT8_bnr_iRadialTune   [7:0] AFIT8_bnr_iRadialPower
	{0x0F12, 0x245A},    //70000A86 AFIT8_bnr_iRadialLimit   [7:0] AFIT8_ee_iFSMagThLow
	{0x0F12, 0x1018},    //70000A88 AFIT8_ee_iFSMagThHigh   [7:0] AFIT8_ee_iFSVarThLow
	{0x0F12, 0x000B},    //70000A8A AFIT8_ee_iFSVarThHigh   [7:0] AFIT8_ee_iFSThLow
	{0x0F12, 0x0B00},    //70000A8C AFIT8_ee_iFSThHigh   [7:0] AFIT8_ee_iFSmagPower
	{0x0F12, 0x5A0F},    //70000A8E AFIT8_ee_iFSVarCountTh   [7:0] AFIT8_ee_iRadialLimit
	{0x0F12, 0x0505},    //70000A90 AFIT8_ee_iRadialPower   [7:0] AFIT8_ee_iSmoothEdgeSlope
	{0x0F12, 0x1802},    //70000A92 AFIT8_ee_iROADThres   [7:0] AFIT8_ee_iROADMaxNR
	{0x0F12, 0x0000},    //70000A94 AFIT8_ee_iROADSubMaxNR   [7:0] AFIT8_ee_iROADSubThres
	{0x0F12, 0x2006},    //70000A96 AFIT8_ee_iROADNeiThres   [7:0] AFIT8_ee_iROADNeiMaxNR
	{0x0F12, 0x3428},    //70000A98 AFIT8_ee_iSmoothEdgeThres   [7:0] AFIT8_ee_iMSharpen
	{0x0F12, 0x041C},    //70000A9A AFIT8_ee_iWSharpen   [7:0] AFIT8_ee_iMShThresh
	{0x0F12, 0x0101},    //70000A9C AFIT8_ee_iWShThresh   [7:0] AFIT8_ee_iReduceNegative
	{0x0F12, 0x0800},    //70000A9E AFIT8_ee_iEmbossCentAdd   [7:0] AFIT8_ee_iShDespeckle
	{0x0F12, 0x1004},    //70000AA0 AFIT8_ee_iReduceEdgeThresh   [7:0] AFIT8_dmsc_iEnhThresh
	{0x0F12, 0x4008},    //70000AA2 AFIT8_dmsc_iDesatThresh   [7:0] AFIT8_dmsc_iDemBlurHigh
	{0x0F12, 0x0540},    //70000AA4 AFIT8_dmsc_iDemBlurLow   [7:0] AFIT8_dmsc_iDemBlurRange
	{0x0F12, 0x8006},    //70000AA6 AFIT8_dmsc_iDecisionThresh   [7:0] AFIT8_dmsc_iCentGrad
	{0x0F12, 0x0020},    //70000AA8 AFIT8_dmsc_iMonochrom   [7:0] AFIT8_dmsc_iGBDenoiseVal
	{0x0F12, 0x0000},    //70000AAA AFIT8_dmsc_iGRDenoiseVal   [7:0] AFIT8_dmsc_iEdgeDesatThrHigh
	{0x0F12, 0x1800},    //70000AAC AFIT8_dmsc_iEdgeDesatThrLow   [7:0] AFIT8_dmsc_iEdgeDesat
	{0x0F12, 0x0000},    //70000AAE AFIT8_dmsc_iNearGrayDesat   [7:0] AFIT8_dmsc_iEdgeDesatLimit
	{0x0F12, 0x1E10},    //70000AB0 AFIT8_postdmsc_iBCoeff   [7:0] AFIT8_postdmsc_iGCoeff
	{0x0F12, 0x000B},    //70000AB2 AFIT8_postdmsc_iWideMult   [7:0] AFIT8_yuvemix_mNegSlopes_0
	{0x0F12, 0x0607},    //70000AB4 AFIT8_yuvemix_mNegSlopes_1   [7:0] AFIT8_yuvemix_mNegSlopes_2
	{0x0F12, 0x0005},    //70000AB6 AFIT8_yuvemix_mNegSlopes_3   [7:0] AFIT8_yuvemix_mPosSlopes_0
	{0x0F12, 0x0607},    //70000AB8 AFIT8_yuvemix_mPosSlopes_1   [7:0] AFIT8_yuvemix_mPosSlopes_2
	{0x0F12, 0x0405},    //70000ABA AFIT8_yuvemix_mPosSlopes_3   [7:0] AFIT8_yuviirnr_iXSupportY
	{0x0F12, 0x0205},    //70000ABC AFIT8_yuviirnr_iXSupportUV   [7:0] AFIT8_yuviirnr_iLowYNorm
	{0x0F12, 0x0304},    //70000ABE AFIT8_yuviirnr_iHighYNorm   [7:0] AFIT8_yuviirnr_iLowUVNorm
	{0x0F12, 0x0409},    //70000AC0 AFIT8_yuviirnr_iHighUVNorm   [7:0] AFIT8_yuviirnr_iYNormShift
	{0x0F12, 0x0306},    //70000AC2 AFIT8_yuviirnr_iUVNormShift   [7:0] AFIT8_yuviirnr_iVertLength_Y
	{0x0F12, 0x0407},    //70000AC4 AFIT8_yuviirnr_iVertLength_UV   [7:0] AFIT8_yuviirnr_iDiffThreshL_Y
	{0x0F12, 0x1F04},    //70000AC6 AFIT8_yuviirnr_iDiffThreshH_Y   [7:0] AFIT8_yuviirnr_iDiffThreshL_UV
	{0x0F12, 0x0218},    //70000AC8 AFIT8_yuviirnr_iDiffThreshH_UV   [7:0] AFIT8_yuviirnr_iMaxThreshL_Y
	{0x0F12, 0x1102},    //70000ACA AFIT8_yuviirnr_iMaxThreshH_Y   [7:0] AFIT8_yuviirnr_iMaxThreshL_UV
	{0x0F12, 0x0611},    //70000ACC AFIT8_yuviirnr_iMaxThreshH_UV   [7:0] AFIT8_yuviirnr_iYNRStrengthL
	{0x0F12, 0x1A02},    //70000ACE AFIT8_yuviirnr_iYNRStrengthH   [7:0] AFIT8_yuviirnr_iUVNRStrengthL
	{0x0F12, 0x8018},    //70000AD0 AFIT8_yuviirnr_iUVNRStrengthH   [7:0] AFIT8_byr_gras_iShadingPower
	{0x0F12, 0x0080},    //70000AD2 AFIT8_RGBGamma2_iLinearity   [7:0] AFIT8_RGBGamma2_iDarkReduce
	{0x0F12, 0x0380},    //70000AD4 AFIT8_ccm_oscar_iSaturation   [7:0] AFIT8_RGB2YUV_iYOffset
	{0x0F12, 0x0180},    //70000AD6 AFIT8_RGB2YUV_iRGBGain   [7:0] AFIT8_bnr_nClustLevel_H
	{0x0F12, 0x0A0A},    //70000AD8 AFIT8_bnr_iClustMulT_H   [7:0] AFIT8_bnr_iClustMulT_C
	{0x0F12, 0x0101},    //70000ADA AFIT8_bnr_iClustThresh_H   [7:0] AFIT8_bnr_iClustThresh_C
	{0x0F12, 0x1B24},    //70000ADC AFIT8_bnr_iDenThreshLow   [7:0] AFIT8_bnr_iDenThreshHigh
	{0x0F12, 0x6024},    //70000ADE AFIT8_ee_iLowSharpPower   [7:0] AFIT8_ee_iHighSharpPower
	{0x0F12, 0x1D22},    //70000AE0 AFIT8_ee_iLowShDenoise   [7:0] AFIT8_ee_iHighShDenoise
	{0x0F12, 0xFFFF},    //70000AE2 AFIT8_ee_iLowSharpClamp   [7:0] AFIT8_ee_iHighSharpClamp
	{0x0F12, 0x0808},    //70000AE4 AFIT8_ee_iReduceEdgeMinMult   [7:0] AFIT8_ee_iReduceEdgeSlope
	{0x0F12, 0x0A01},    //70000AE6 AFIT8_bnr_nClustLevel_H_Bin   [7:0] AFIT8_bnr_iClustMulT_H_Bin
	{0x0F12, 0x010A},    //70000AE8 AFIT8_bnr_iClustMulT_C_Bin   [7:0] AFIT8_bnr_iClustThresh_H_Bin
	{0x0F12, 0x2401},    //70000AEA AFIT8_bnr_iClustThresh_C_Bin   [7:0] AFIT8_bnr_iDenThreshLow_Bin
	{0x0F12, 0x241B},    //70000AEC AFIT8_bnr_iDenThreshHigh_Bin   [7:0] AFIT8_ee_iLowSharpPower_Bin
	{0x0F12, 0x1E60},    //70000AEE AFIT8_ee_iHighSharpPower_Bin   [7:0] AFIT8_ee_iLowShDenoise_Bin
	{0x0F12, 0xFF18},    //70000AF0 AFIT8_ee_iHighShDenoise_Bin   [7:0] AFIT8_ee_iLowSharpClamp_Bin
	{0x0F12, 0x08FF},    //70000AF2 AFIT8_ee_iHighSharpClamp_Bin   [7:0] AFIT8_ee_iReduceEdgeMinMult_Bin
	{0x0F12, 0x0008},    //70000AF4 AFIT8_ee_iReduceEdgeSlope_Bin [7:0]
	{0x0F12, 0x0001},    //70000AF6 AFITB_bnr_nClustLevel_C      [0]
	{0x0F12, 0x0000},    //70000AF8 AFIT16_BRIGHTNESS
	{0x0F12, 0x0000},    //70000AFA AFIT16_CONTRAST
	{0x0F12, 0xFFFB},    //70000AFC AFIT16_SATURATION
	{0x0F12, 0x0000},    //70000AFE AFIT16_SHARP_BLUR
	{0x0F12, 0x0000},    //70000B00 AFIT16_GLAMOUR
	{0x0F12, 0x00C0},    //70000B02 AFIT16_bnr_edge_high
	{0x0F12, 0x0064},    //70000B04 AFIT16_postdmsc_iLowBright
	{0x0F12, 0x0384},    //70000B06 AFIT16_postdmsc_iHighBright
	{0x0F12, 0x0043},    //70000B08 AFIT16_postdmsc_iLowSat
	{0x0F12, 0x01F4},    //70000B0A AFIT16_postdmsc_iHighSat
	{0x0F12, 0x0070},    //70000B0C AFIT16_postdmsc_iTune
	{0x0F12, 0x0040},    //70000B0E AFIT16_yuvemix_mNegRanges_0
	{0x0F12, 0x00A0},    //70000B10 AFIT16_yuvemix_mNegRanges_1
	{0x0F12, 0x0100},    //70000B12 AFIT16_yuvemix_mNegRanges_2
	{0x0F12, 0x0010},    //70000B14 AFIT16_yuvemix_mPosRanges_0
	{0x0F12, 0x0060},    //70000B16 AFIT16_yuvemix_mPosRanges_1
	{0x0F12, 0x0100},    //70000B18 AFIT16_yuvemix_mPosRanges_2
	{0x0F12, 0x1430},    //70000B1A AFIT8_bnr_edge_low  [7:0] AFIT8_bnr_repl_thresh
	{0x0F12, 0x0201},    //70000B1C AFIT8_bnr_repl_force  [7:0] AFIT8_bnr_iHotThreshHigh
	{0x0F12, 0x0204},    //70000B1E AFIT8_bnr_iHotThreshLow   [7:0] AFIT8_bnr_iColdThreshHigh
	{0x0F12, 0x1B04},    //70000B20 AFIT8_bnr_iColdThreshLow   [7:0] AFIT8_bnr_DispTH_Low
	{0x0F12, 0x0312},    //70000B22 AFIT8_bnr_DispTH_High   [7:0] AFIT8_bnr_DISP_Limit_Low
	{0x0F12, 0x0003},    //70000B24 AFIT8_bnr_DISP_Limit_High   [7:0] AFIT8_bnr_iDistSigmaMin
	{0x0F12, 0x0C03},    //70000B26 AFIT8_bnr_iDistSigmaMax   [7:0] AFIT8_bnr_iDiffSigmaLow
	{0x0F12, 0x2806},    //70000B28 AFIT8_bnr_iDiffSigmaHigh   [7:0] AFIT8_bnr_iNormalizedSTD_TH
	{0x0F12, 0x0060},    //70000B2A AFIT8_bnr_iNormalizedSTD_Limit   [7:0] AFIT8_bnr_iDirNRTune
	{0x0F12, 0x1580},    //70000B2C AFIT8_bnr_iDirMinThres   [7:0] AFIT8_bnr_iDirFltDiffThresHigh
	{0x0F12, 0x2020},    //70000B2E AFIT8_bnr_iDirFltDiffThresLow   [7:0] AFIT8_bnr_iDirSmoothPowerHigh
	{0x0F12, 0x0620},    //70000B30 AFIT8_bnr_iDirSmoothPowerLow   [7:0] AFIT8_bnr_iLowMaxSlopeAllowed
	{0x0F12, 0x0306},    //70000B32 AFIT8_bnr_iHighMaxSlopeAllowed   [7:0] AFIT8_bnr_iLowSlopeThresh
	{0x0F12, 0x2003},    //70000B34 AFIT8_bnr_iHighSlopeThresh   [7:0] AFIT8_bnr_iSlopenessTH
	{0x0F12, 0xFF01},    //70000B36 AFIT8_bnr_iSlopeBlurStrength   [7:0] AFIT8_bnr_iSlopenessLimit
	{0x0F12, 0x0404},    //70000B38 AFIT8_bnr_AddNoisePower1   [7:0] AFIT8_bnr_AddNoisePower2
	{0x0F12, 0x0300},    //70000B3A AFIT8_bnr_iRadialTune   [7:0] AFIT8_bnr_iRadialPower
	{0x0F12, 0x145A},    //70000B3C AFIT8_bnr_iRadialLimit   [7:0] AFIT8_ee_iFSMagThLow
	{0x0F12, 0x1010},    //70000B3E AFIT8_ee_iFSMagThHigh   [7:0] AFIT8_ee_iFSVarThLow
	{0x0F12, 0x000B},    //70000B40 AFIT8_ee_iFSVarThHigh   [7:0] AFIT8_ee_iFSThLow
	{0x0F12, 0x0E00},    //70000B42 AFIT8_ee_iFSThHigh   [7:0] AFIT8_ee_iFSmagPower
	{0x0F12, 0x5A0F},    //70000B44 AFIT8_ee_iFSVarCountTh   [7:0] AFIT8_ee_iRadialLimit
	{0x0F12, 0x0504},    //70000B46 AFIT8_ee_iRadialPower   [7:0] AFIT8_ee_iSmoothEdgeSlope
	{0x0F12, 0x1802},    //70000B48 AFIT8_ee_iROADThres   [7:0] AFIT8_ee_iROADMaxNR
	{0x0F12, 0x0000},    //70000B4A AFIT8_ee_iROADSubMaxNR   [7:0] AFIT8_ee_iROADSubThres
	{0x0F12, 0x2006},    //70000B4C AFIT8_ee_iROADNeiThres   [7:0] AFIT8_ee_iROADNeiMaxNR
	{0x0F12, 0x3828},    //70000B4E AFIT8_ee_iSmoothEdgeThres   [7:0] AFIT8_ee_iMSharpen
	{0x0F12, 0x0428},    //70000B50 AFIT8_ee_iWSharpen   [7:0] AFIT8_ee_iMShThresh
	{0x0F12, 0x0101},    //70000B52 AFIT8_ee_iWShThresh   [7:0] AFIT8_ee_iReduceNegative
	{0x0F12, 0x8000},    //70000B54 AFIT8_ee_iEmbossCentAdd   [7:0] AFIT8_ee_iShDespeckle
	{0x0F12, 0x0A04},    //70000B56 AFIT8_ee_iReduceEdgeThresh   [7:0] AFIT8_dmsc_iEnhThresh
	{0x0F12, 0x4008},    //70000B58 AFIT8_dmsc_iDesatThresh   [7:0] AFIT8_dmsc_iDemBlurHigh
	{0x0F12, 0x0540},    //70000B5A AFIT8_dmsc_iDemBlurLow   [7:0] AFIT8_dmsc_iDemBlurRange
	{0x0F12, 0x8006},    //70000B5C AFIT8_dmsc_iDecisionThresh   [7:0] AFIT8_dmsc_iCentGrad
	{0x0F12, 0x0020},    //70000B5E AFIT8_dmsc_iMonochrom   [7:0] AFIT8_dmsc_iGBDenoiseVal
	{0x0F12, 0x0000},    //70000B60 AFIT8_dmsc_iGRDenoiseVal   [7:0] AFIT8_dmsc_iEdgeDesatThrHigh
	{0x0F12, 0x1800},    //70000B62 AFIT8_dmsc_iEdgeDesatThrLow   [7:0] AFIT8_dmsc_iEdgeDesat
	{0x0F12, 0x0000},    //70000B64 AFIT8_dmsc_iNearGrayDesat   [7:0] AFIT8_dmsc_iEdgeDesatLimit
	{0x0F12, 0x1E10},    //70000B66 AFIT8_postdmsc_iBCoeff   [7:0] AFIT8_postdmsc_iGCoeff
	{0x0F12, 0x000B},    //70000B68 AFIT8_postdmsc_iWideMult   [7:0] AFIT8_yuvemix_mNegSlopes_0
	{0x0F12, 0x0607},    //70000B6A AFIT8_yuvemix_mNegSlopes_1   [7:0] AFIT8_yuvemix_mNegSlopes_2
	{0x0F12, 0x0005},    //70000B6C AFIT8_yuvemix_mNegSlopes_3   [7:0] AFIT8_yuvemix_mPosSlopes_0
	{0x0F12, 0x0607},    //70000B6E AFIT8_yuvemix_mPosSlopes_1   [7:0] AFIT8_yuvemix_mPosSlopes_2
	{0x0F12, 0x0405},    //70000B70 AFIT8_yuvemix_mPosSlopes_3   [7:0] AFIT8_yuviirnr_iXSupportY
	{0x0F12, 0x0207},    //70000B72 AFIT8_yuviirnr_iXSupportUV   [7:0] AFIT8_yuviirnr_iLowYNorm
	{0x0F12, 0x0304},    //70000B74 AFIT8_yuviirnr_iHighYNorm   [7:0] AFIT8_yuviirnr_iLowUVNorm
	{0x0F12, 0x0409},    //70000B76 AFIT8_yuviirnr_iHighUVNorm   [7:0] AFIT8_yuviirnr_iYNormShift
	{0x0F12, 0x0306},    //70000B78 AFIT8_yuviirnr_iUVNormShift   [7:0] AFIT8_yuviirnr_iVertLength_Y
	{0x0F12, 0x0407},    //70000B7A AFIT8_yuviirnr_iVertLength_UV   [7:0] AFIT8_yuviirnr_iDiffThreshL_Y
	{0x0F12, 0x2404},    //70000B7C AFIT8_yuviirnr_iDiffThreshH_Y   [7:0] AFIT8_yuviirnr_iDiffThreshL_UV
	{0x0F12, 0x0221},    //70000B7E AFIT8_yuviirnr_iDiffThreshH_UV   [7:0] AFIT8_yuviirnr_iMaxThreshL_Y
	{0x0F12, 0x1202},    //70000B80 AFIT8_yuviirnr_iMaxThreshH_Y   [7:0] AFIT8_yuviirnr_iMaxThreshL_UV
	{0x0F12, 0x0613},    //70000B82 AFIT8_yuviirnr_iMaxThreshH_UV   [7:0] AFIT8_yuviirnr_iYNRStrengthL
	{0x0F12, 0x1A02},    //70000B84 AFIT8_yuviirnr_iYNRStrengthH   [7:0] AFIT8_yuviirnr_iUVNRStrengthL
	{0x0F12, 0x8018},    //70000B86 AFIT8_yuviirnr_iUVNRStrengthH   [7:0] AFIT8_byr_gras_iShadingPower
	{0x0F12, 0x0080},    //70000B88 AFIT8_RGBGamma2_iLinearity   [7:0] AFIT8_RGBGamma2_iDarkReduce
	{0x0F12, 0x0080},    //70000B8A AFIT8_ccm_oscar_iSaturation   [7:0] AFIT8_RGB2YUV_iYOffset
	{0x0F12, 0x0180},    //70000B8C AFIT8_RGB2YUV_iRGBGain   [7:0] AFIT8_bnr_nClustLevel_H
	{0x0F12, 0x0A0A},    //70000B8E AFIT8_bnr_iClustMulT_H   [7:0] AFIT8_bnr_iClustMulT_C
	{0x0F12, 0x0101},    //70000B90 AFIT8_bnr_iClustThresh_H   [7:0] AFIT8_bnr_iClustThresh_C
	{0x0F12, 0x141D},    //70000B92 AFIT8_bnr_iDenThreshLow   [7:0] AFIT8_bnr_iDenThreshHigh
	{0x0F12, 0x6024},    //70000B94 AFIT8_ee_iLowSharpPower   [7:0] AFIT8_ee_iHighSharpPower
	{0x0F12, 0x0C0C},    //70000B96 AFIT8_ee_iLowShDenoise   [7:0] AFIT8_ee_iHighShDenoise
	{0x0F12, 0xFFFF},    //70000B98 AFIT8_ee_iLowSharpClamp   [7:0] AFIT8_ee_iHighSharpClamp
	{0x0F12, 0x0808},    //70000B9A AFIT8_ee_iReduceEdgeMinMult   [7:0] AFIT8_ee_iReduceEdgeSlope
	{0x0F12, 0x0A01},    //70000B9C AFIT8_bnr_nClustLevel_H_Bin   [7:0] AFIT8_bnr_iClustMulT_H_Bin
	{0x0F12, 0x010A},    //70000B9E AFIT8_bnr_iClustMulT_C_Bin   [7:0] AFIT8_bnr_iClustThresh_H_Bin
	{0x0F12, 0x1B01},    //70000BA0 AFIT8_bnr_iClustThresh_C_Bin   [7:0] AFIT8_bnr_iDenThreshLow_Bin
	{0x0F12, 0x2412},    //70000BA2 AFIT8_bnr_iDenThreshHigh_Bin   [7:0] AFIT8_ee_iLowSharpPower_Bin
	{0x0F12, 0x0C60},    //70000BA4 AFIT8_ee_iHighSharpPower_Bin   [7:0] AFIT8_ee_iLowShDenoise_Bin
	{0x0F12, 0xFF0C},    //70000BA6 AFIT8_ee_iHighShDenoise_Bin   [7:0] AFIT8_ee_iLowSharpClamp_Bin
	{0x0F12, 0x08FF},    //70000BA8 AFIT8_ee_iHighSharpClamp_Bin   [7:0] AFIT8_ee_iReduceEdgeMinMult_Bin
	{0x0F12, 0x0008},    //70000BAA AFIT8_ee_iReduceEdgeSlope_Bin [7:0]
	{0x0F12, 0x0001},    //70000BAC AFITB_bnr_nClustLevel_C      [0]
	{0x0F12, 0x0000},    //70000BAE AFIT16_BRIGHTNESS
	{0x0F12, 0x0000},    //70000BB0 AFIT16_CONTRAST
	{0x0F12, 0x0000},    //70000BB2 AFIT16_SATURATION
	{0x0F12, 0x0000},    //70000BB4 AFIT16_SHARP_BLUR
	{0x0F12, 0x0000},    //70000BB6 AFIT16_GLAMOUR
	{0x0F12, 0x00C0},    //70000BB8 AFIT16_bnr_edge_high
	{0x0F12, 0x0064},    //70000BBA AFIT16_postdmsc_iLowBright
	{0x0F12, 0x0384},    //70000BBC AFIT16_postdmsc_iHighBright
	{0x0F12, 0x0032},    //70000BBE AFIT16_postdmsc_iLowSat
	{0x0F12, 0x01F4},    //70000BC0 AFIT16_postdmsc_iHighSat
	{0x0F12, 0x0070},    //70000BC2 AFIT16_postdmsc_iTune
	{0x0F12, 0x0040},    //70000BC4 AFIT16_yuvemix_mNegRanges_0
	{0x0F12, 0x00A0},    //70000BC6 AFIT16_yuvemix_mNegRanges_1
	{0x0F12, 0x0100},    //70000BC8 AFIT16_yuvemix_mNegRanges_2
	{0x0F12, 0x0010},    //70000BCA AFIT16_yuvemix_mPosRanges_0
	{0x0F12, 0x0060},    //70000BCC AFIT16_yuvemix_mPosRanges_1
	{0x0F12, 0x0100},    //70000BCE AFIT16_yuvemix_mPosRanges_2
	{0x0F12, 0x1430},    //70000BD0 AFIT8_bnr_edge_low  [7:0] AFIT8_bnr_repl_thresh
	{0x0F12, 0x0201},    //70000BD2 AFIT8_bnr_repl_force  [7:0] AFIT8_bnr_iHotThreshHigh
	{0x0F12, 0x0204},    //70000BD4 AFIT8_bnr_iHotThreshLow   [7:0] AFIT8_bnr_iColdThreshHigh
	{0x0F12, 0x1504},    //70000BD6 AFIT8_bnr_iColdThreshLow   [7:0] AFIT8_bnr_DispTH_Low
	{0x0F12, 0x030F},    //70000BD8 AFIT8_bnr_DispTH_High   [7:0] AFIT8_bnr_DISP_Limit_Low
	{0x0F12, 0x0003},    //70000BDA AFIT8_bnr_DISP_Limit_High   [7:0] AFIT8_bnr_iDistSigmaMin
	{0x0F12, 0x0902},    //70000BDC AFIT8_bnr_iDistSigmaMax   [7:0] AFIT8_bnr_iDiffSigmaLow
	{0x0F12, 0x2004},    //70000BDE AFIT8_bnr_iDiffSigmaHigh   [7:0] AFIT8_bnr_iNormalizedSTD_TH
	{0x0F12, 0x0050},    //70000BE0 AFIT8_bnr_iNormalizedSTD_Limit   [7:0] AFIT8_bnr_iDirNRTune
	{0x0F12, 0x1140},    //70000BE2 AFIT8_bnr_iDirMinThres   [7:0] AFIT8_bnr_iDirFltDiffThresHigh
	{0x0F12, 0x201C},    //70000BE4 AFIT8_bnr_iDirFltDiffThresLow   [7:0] AFIT8_bnr_iDirSmoothPowerHigh
	{0x0F12, 0x0620},    //70000BE6 AFIT8_bnr_iDirSmoothPowerLow   [7:0] AFIT8_bnr_iLowMaxSlopeAllowed
	{0x0F12, 0x0306},    //70000BE8 AFIT8_bnr_iHighMaxSlopeAllowed   [7:0] AFIT8_bnr_iLowSlopeThresh
	{0x0F12, 0x2003},    //70000BEA AFIT8_bnr_iHighSlopeThresh   [7:0] AFIT8_bnr_iSlopenessTH
	{0x0F12, 0xFF01},    //70000BEC AFIT8_bnr_iSlopeBlurStrength   [7:0] AFIT8_bnr_iSlopenessLimit
	{0x0F12, 0x0404},    //70000BEE AFIT8_bnr_AddNoisePower1   [7:0] AFIT8_bnr_AddNoisePower2
	{0x0F12, 0x0300},    //70000BF0 AFIT8_bnr_iRadialTune   [7:0] AFIT8_bnr_iRadialPower
	{0x0F12, 0x145A},    //70000BF2 AFIT8_bnr_iRadialLimit   [7:0] AFIT8_ee_iFSMagThLow
	{0x0F12, 0x1010},    //70000BF4 AFIT8_ee_iFSMagThHigh   [7:0] AFIT8_ee_iFSVarThLow
	{0x0F12, 0x000B},    //70000BF6 AFIT8_ee_iFSVarThHigh   [7:0] AFIT8_ee_iFSThLow
	{0x0F12, 0x1000},    //70000BF8 AFIT8_ee_iFSThHigh   [7:0] AFIT8_ee_iFSmagPower
	{0x0F12, 0x5A0F},    //70000BFA AFIT8_ee_iFSVarCountTh   [7:0] AFIT8_ee_iRadialLimit
	{0x0F12, 0x0503},    //70000BFC AFIT8_ee_iRadialPower   [7:0] AFIT8_ee_iSmoothEdgeSlope
	{0x0F12, 0x1802},    //70000BFE AFIT8_ee_iROADThres   [7:0] AFIT8_ee_iROADMaxNR
	{0x0F12, 0x0000},    //70000C00 AFIT8_ee_iROADSubMaxNR   [7:0] AFIT8_ee_iROADSubThres
	{0x0F12, 0x2006},    //70000C02 AFIT8_ee_iROADNeiThres   [7:0] AFIT8_ee_iROADNeiMaxNR
	{0x0F12, 0x3C28},    //70000C04 AFIT8_ee_iSmoothEdgeThres   [7:0] AFIT8_ee_iMSharpen
	{0x0F12, 0x042C},    //70000C06 AFIT8_ee_iWSharpen   [7:0] AFIT8_ee_iMShThresh
	{0x0F12, 0x0101},    //70000C08 AFIT8_ee_iWShThresh   [7:0] AFIT8_ee_iReduceNegative
	{0x0F12, 0xFF00},    //70000C0A AFIT8_ee_iEmbossCentAdd   [7:0] AFIT8_ee_iShDespeckle
	{0x0F12, 0x0904},    //70000C0C AFIT8_ee_iReduceEdgeThresh   [7:0] AFIT8_dmsc_iEnhThresh
	{0x0F12, 0x4008},    //70000C0E AFIT8_dmsc_iDesatThresh   [7:0] AFIT8_dmsc_iDemBlurHigh
	{0x0F12, 0x0540},    //70000C10 AFIT8_dmsc_iDemBlurLow   [7:0] AFIT8_dmsc_iDemBlurRange
	{0x0F12, 0x8006},    //70000C12 AFIT8_dmsc_iDecisionThresh   [7:0] AFIT8_dmsc_iCentGrad
	{0x0F12, 0x0020},    //70000C14 AFIT8_dmsc_iMonochrom   [7:0] AFIT8_dmsc_iGBDenoiseVal
	{0x0F12, 0x0000},    //70000C16 AFIT8_dmsc_iGRDenoiseVal   [7:0] AFIT8_dmsc_iEdgeDesatThrHigh
	{0x0F12, 0x1800},    //70000C18 AFIT8_dmsc_iEdgeDesatThrLow   [7:0] AFIT8_dmsc_iEdgeDesat
	{0x0F12, 0x0000},    //70000C1A AFIT8_dmsc_iNearGrayDesat   [7:0] AFIT8_dmsc_iEdgeDesatLimit
	{0x0F12, 0x1E10},    //70000C1C AFIT8_postdmsc_iBCoeff   [7:0] AFIT8_postdmsc_iGCoeff
	{0x0F12, 0x000B},    //70000C1E AFIT8_postdmsc_iWideMult   [7:0] AFIT8_yuvemix_mNegSlopes_0
	{0x0F12, 0x0607},    //70000C20 AFIT8_yuvemix_mNegSlopes_1   [7:0] AFIT8_yuvemix_mNegSlopes_2
	{0x0F12, 0x0005},    //70000C22 AFIT8_yuvemix_mNegSlopes_3   [7:0] AFIT8_yuvemix_mPosSlopes_0
	{0x0F12, 0x0607},    //70000C24 AFIT8_yuvemix_mPosSlopes_1   [7:0] AFIT8_yuvemix_mPosSlopes_2
	{0x0F12, 0x0405},    //70000C26 AFIT8_yuvemix_mPosSlopes_3   [7:0] AFIT8_yuviirnr_iXSupportY
	{0x0F12, 0x0206},    //70000C28 AFIT8_yuviirnr_iXSupportUV   [7:0] AFIT8_yuviirnr_iLowYNorm
	{0x0F12, 0x0304},    //70000C2A AFIT8_yuviirnr_iHighYNorm   [7:0] AFIT8_yuviirnr_iLowUVNorm
	{0x0F12, 0x0409},    //70000C2C AFIT8_yuviirnr_iHighUVNorm   [7:0] AFIT8_yuviirnr_iYNormShift
	{0x0F12, 0x0305},    //70000C2E AFIT8_yuviirnr_iUVNormShift   [7:0] AFIT8_yuviirnr_iVertLength_Y
	{0x0F12, 0x0406},    //70000C30 AFIT8_yuviirnr_iVertLength_UV   [7:0] AFIT8_yuviirnr_iDiffThreshL_Y
	{0x0F12, 0x2804},    //70000C32 AFIT8_yuviirnr_iDiffThreshH_Y   [7:0] AFIT8_yuviirnr_iDiffThreshL_UV
	{0x0F12, 0x0228},    //70000C34 AFIT8_yuviirnr_iDiffThreshH_UV   [7:0] AFIT8_yuviirnr_iMaxThreshL_Y
	{0x0F12, 0x1402},    //70000C36 AFIT8_yuviirnr_iMaxThreshH_Y   [7:0] AFIT8_yuviirnr_iMaxThreshL_UV
	{0x0F12, 0x0618},    //70000C38 AFIT8_yuviirnr_iMaxThreshH_UV   [7:0] AFIT8_yuviirnr_iYNRStrengthL
	{0x0F12, 0x1A02},    //70000C3A AFIT8_yuviirnr_iYNRStrengthH   [7:0] AFIT8_yuviirnr_iUVNRStrengthL
	{0x0F12, 0x8018},    //70000C3C AFIT8_yuviirnr_iUVNRStrengthH   [7:0] AFIT8_byr_gras_iShadingPower
	{0x0F12, 0x0080},    //70000C3E AFIT8_RGBGamma2_iLinearity   [7:0] AFIT8_RGBGamma2_iDarkReduce
	{0x0F12, 0x0080},    //70000C40 AFIT8_ccm_oscar_iSaturation   [7:0] AFIT8_RGB2YUV_iYOffset
	{0x0F12, 0x0180},    //70000C42 AFIT8_RGB2YUV_iRGBGain   [7:0] AFIT8_bnr_nClustLevel_H
	{0x0F12, 0x0A0A},    //70000C44 AFIT8_bnr_iClustMulT_H   [7:0] AFIT8_bnr_iClustMulT_C
	{0x0F12, 0x0101},    //70000C46 AFIT8_bnr_iClustThresh_H   [7:0] AFIT8_bnr_iClustThresh_C
	{0x0F12, 0x1117},    //70000C48 AFIT8_bnr_iDenThreshLow   [7:0] AFIT8_bnr_iDenThreshHigh
	{0x0F12, 0x6024},    //70000C4A AFIT8_ee_iLowSharpPower   [7:0] AFIT8_ee_iHighSharpPower
	{0x0F12, 0x0A0A},    //70000C4C AFIT8_ee_iLowShDenoise   [7:0] AFIT8_ee_iHighShDenoise
	{0x0F12, 0xFFFF},    //70000C4E AFIT8_ee_iLowSharpClamp   [7:0] AFIT8_ee_iHighSharpClamp
	{0x0F12, 0x0808},    //70000C50 AFIT8_ee_iReduceEdgeMinMult   [7:0] AFIT8_ee_iReduceEdgeSlope
	{0x0F12, 0x0A01},    //70000C52 AFIT8_bnr_nClustLevel_H_Bin   [7:0] AFIT8_bnr_iClustMulT_H_Bin
	{0x0F12, 0x010A},    //70000C54 AFIT8_bnr_iClustMulT_C_Bin   [7:0] AFIT8_bnr_iClustThresh_H_Bin
	{0x0F12, 0x1501},    //70000C56 AFIT8_bnr_iClustThresh_C_Bin   [7:0] AFIT8_bnr_iDenThreshLow_Bin
	{0x0F12, 0x240F},    //70000C58 AFIT8_bnr_iDenThreshHigh_Bin   [7:0] AFIT8_ee_iLowSharpPower_Bin
	{0x0F12, 0x0A60},    //70000C5A AFIT8_ee_iHighSharpPower_Bin   [7:0] AFIT8_ee_iLowShDenoise_Bin
	{0x0F12, 0xFF0A},    //70000C5C AFIT8_ee_iHighShDenoise_Bin   [7:0] AFIT8_ee_iLowSharpClamp_Bin
	{0x0F12, 0x08FF},    //70000C5E AFIT8_ee_iHighSharpClamp_Bin   [7:0] AFIT8_ee_iReduceEdgeMinMult_Bin
	{0x0F12, 0x0008},    //70000C60 AFIT8_ee_iReduceEdgeSlope_Bin [7:0]
	{0x0F12, 0x0001},    //70000C62 AFITB_bnr_nClustLevel_C      [0]
	{0x0F12, 0x0000},    //70000C64 AFIT16_BRIGHTNESS  	//AFIT 4
	{0x0F12, 0x0000},    //70000C66 AFIT16_CONTRAST
	{0x0F12, 0x0000},    //70000C68 AFIT16_SATURATION
	{0x0F12, 0x0000},    //70000C6A AFIT16_SHARP_BLUR
	{0x0F12, 0x0000},    //70000C6C AFIT16_GLAMOUR
	{0x0F12, 0x00C0},    //70000C6E AFIT16_bnr_edge_high
	{0x0F12, 0x0064},    //70000C70 AFIT16_postdmsc_iLowBright
	{0x0F12, 0x0384},    //70000C72 AFIT16_postdmsc_iHighBright
	{0x0F12, 0x0032},    //70000C74 AFIT16_postdmsc_iLowSat
	{0x0F12, 0x01F4},    //70000C76 AFIT16_postdmsc_iHighSat
	{0x0F12, 0x0070},    //70000C78 AFIT16_postdmsc_iTune
	{0x0F12, 0x0040},    //70000C7A AFIT16_yuvemix_mNegRanges_0
	{0x0F12, 0x00A0},    //70000C7C AFIT16_yuvemix_mNegRanges_1
	{0x0F12, 0x0100},    //70000C7E AFIT16_yuvemix_mNegRanges_2
	{0x0F12, 0x0010},    //70000C80 AFIT16_yuvemix_mPosRanges_0
	{0x0F12, 0x0060},    //70000C82 AFIT16_yuvemix_mPosRanges_1
	{0x0F12, 0x0100},    //70000C84 AFIT16_yuvemix_mPosRanges_2
	{0x0F12, 0x1430},    //70000C86 AFIT8_bnr_edge_low  [7:0] AFIT8_bnr_repl_thresh
	{0x0F12, 0x0201},    //70000C88 AFIT8_bnr_repl_force  [7:0] AFIT8_bnr_iHotThreshHigh
	{0x0F12, 0x0204},    //70000C8A AFIT8_bnr_iHotThreshLow   [7:0] AFIT8_bnr_iColdThreshHigh
	{0x0F12, 0x0F04},    //70000C8C AFIT8_bnr_iColdThreshLow   [7:0] AFIT8_bnr_DispTH_Low
	{0x0F12, 0x030C},    //70000C8E AFIT8_bnr_DispTH_High   [7:0] AFIT8_bnr_DISP_Limit_Low
	{0x0F12, 0x0003},    //70000C90 AFIT8_bnr_DISP_Limit_High   [7:0] AFIT8_bnr_iDistSigmaMin
	{0x0F12, 0x0602},    //70000C92 AFIT8_bnr_iDistSigmaMax   [7:0] AFIT8_bnr_iDiffSigmaLow
	{0x0F12, 0x1803},    //70000C94 AFIT8_bnr_iDiffSigmaHigh   [7:0] AFIT8_bnr_iNormalizedSTD_TH
	{0x0F12, 0x0040},    //70000C96 AFIT8_bnr_iNormalizedSTD_Limit   [7:0] AFIT8_bnr_iDirNRTune
	{0x0F12, 0x0E20},    //70000C98 AFIT8_bnr_iDirMinThres   [7:0] AFIT8_bnr_iDirFltDiffThresHigh
	{0x0F12, 0x2018},    //70000C9A AFIT8_bnr_iDirFltDiffThresLow   [7:0] AFIT8_bnr_iDirSmoothPowerHigh
	{0x0F12, 0x0620},    //70000C9C AFIT8_bnr_iDirSmoothPowerLow   [7:0] AFIT8_bnr_iLowMaxSlopeAllowed
	{0x0F12, 0x0306},    //70000C9E AFIT8_bnr_iHighMaxSlopeAllowed   [7:0] AFIT8_bnr_iLowSlopeThresh
	{0x0F12, 0x2003},    //70000CA0 AFIT8_bnr_iHighSlopeThresh   [7:0] AFIT8_bnr_iSlopenessTH
	{0x0F12, 0xFF01},    //70000CA2 AFIT8_bnr_iSlopeBlurStrength   [7:0] AFIT8_bnr_iSlopenessLimit
	{0x0F12, 0x0404},    //70000CA4 AFIT8_bnr_AddNoisePower1   [7:0] AFIT8_bnr_AddNoisePower2
	{0x0F12, 0x0200},    //70000CA6 AFIT8_bnr_iRadialTune   [7:0] AFIT8_bnr_iRadialPower
	{0x0F12, 0x145A},    //70000CA8 AFIT8_bnr_iRadialLimit   [7:0] AFIT8_ee_iFSMagThLow
	{0x0F12, 0x1010},    //70000CAA AFIT8_ee_iFSMagThHigh   [7:0] AFIT8_ee_iFSVarThLow
	{0x0F12, 0x000B},    //70000CAC AFIT8_ee_iFSVarThHigh   [7:0] AFIT8_ee_iFSThLow
	{0x0F12, 0x1200},    //70000CAE AFIT8_ee_iFSThHigh   [7:0] AFIT8_ee_iFSmagPower
	{0x0F12, 0x5A0F},    //70000CB0 AFIT8_ee_iFSVarCountTh   [7:0] AFIT8_ee_iRadialLimit
	{0x0F12, 0x0502},    //70000CB2 AFIT8_ee_iRadialPower   [7:0] AFIT8_ee_iSmoothEdgeSlope
	{0x0F12, 0x1802},    //70000CB4 AFIT8_ee_iROADThres   [7:0] AFIT8_ee_iROADMaxNR
	{0x0F12, 0x0000},    //70000CB6 AFIT8_ee_iROADSubMaxNR   [7:0] AFIT8_ee_iROADSubThres
	{0x0F12, 0x2006},    //70000CB8 AFIT8_ee_iROADNeiThres   [7:0] AFIT8_ee_iROADNeiMaxNR
	{0x0F12, 0x4028},    //70000CBA AFIT8_ee_iSmoothEdgeThres   [7:0] AFIT8_ee_iMSharpen
	{0x0F12, 0x0430},    //70000CBC AFIT8_ee_iWSharpen   [7:0] AFIT8_ee_iMShThresh
	{0x0F12, 0x0101},    //70000CBE AFIT8_ee_iWShThresh   [7:0] AFIT8_ee_iReduceNegative
	{0x0F12, 0xFF00},    //70000CC0 AFIT8_ee_iEmbossCentAdd   [7:0] AFIT8_ee_iShDespeckle
	{0x0F12, 0x0804},    //70000CC2 AFIT8_ee_iReduceEdgeThresh   [7:0] AFIT8_dmsc_iEnhThresh
	{0x0F12, 0x4008},    //70000CC4 AFIT8_dmsc_iDesatThresh   [7:0] AFIT8_dmsc_iDemBlurHigh
	{0x0F12, 0x0540},    //70000CC6 AFIT8_dmsc_iDemBlurLow   [7:0] AFIT8_dmsc_iDemBlurRange
	{0x0F12, 0x8006},    //70000CC8 AFIT8_dmsc_iDecisionThresh   [7:0] AFIT8_dmsc_iCentGrad
	{0x0F12, 0x0020},    //70000CCA AFIT8_dmsc_iMonochrom   [7:0] AFIT8_dmsc_iGBDenoiseVal
	{0x0F12, 0x0000},    //70000CCC AFIT8_dmsc_iGRDenoiseVal   [7:0] AFIT8_dmsc_iEdgeDesatThrHigh
	{0x0F12, 0x1800},    //70000CCE AFIT8_dmsc_iEdgeDesatThrLow   [7:0] AFIT8_dmsc_iEdgeDesat
	{0x0F12, 0x0000},    //70000CD0 AFIT8_dmsc_iNearGrayDesat   [7:0] AFIT8_dmsc_iEdgeDesatLimit
	{0x0F12, 0x1E10},    //70000CD2 AFIT8_postdmsc_iBCoeff   [7:0] AFIT8_postdmsc_iGCoeff
	{0x0F12, 0x000B},    //70000CD4 AFIT8_postdmsc_iWideMult   [7:0] AFIT8_yuvemix_mNegSlopes_0
	{0x0F12, 0x0607},    //70000CD6 AFIT8_yuvemix_mNegSlopes_1   [7:0] AFIT8_yuvemix_mNegSlopes_2
	{0x0F12, 0x0005},    //70000CD8 AFIT8_yuvemix_mNegSlopes_3   [7:0] AFIT8_yuvemix_mPosSlopes_0
	{0x0F12, 0x0607},    //70000CDA AFIT8_yuvemix_mPosSlopes_1   [7:0] AFIT8_yuvemix_mPosSlopes_2
	{0x0F12, 0x0405},    //70000CDC AFIT8_yuvemix_mPosSlopes_3   [7:0] AFIT8_yuviirnr_iXSupportY
	{0x0F12, 0x0205},    //70000CDE AFIT8_yuviirnr_iXSupportUV   [7:0] AFIT8_yuviirnr_iLowYNorm
	{0x0F12, 0x0304},    //70000CE0 AFIT8_yuviirnr_iHighYNorm   [7:0] AFIT8_yuviirnr_iLowUVNorm
	{0x0F12, 0x0409},    //70000CE2 AFIT8_yuviirnr_iHighUVNorm   [7:0] AFIT8_yuviirnr_iYNormShift
	{0x0F12, 0x0306},    //70000CE4 AFIT8_yuviirnr_iUVNormShift   [7:0] AFIT8_yuviirnr_iVertLength_Y
	{0x0F12, 0x0407},    //70000CE6 AFIT8_yuviirnr_iVertLength_UV   [7:0] AFIT8_yuviirnr_iDiffThreshL_Y
	{0x0F12, 0x2C04},    //70000CE8 AFIT8_yuviirnr_iDiffThreshH_Y   [7:0] AFIT8_yuviirnr_iDiffThreshL_UV
	{0x0F12, 0x022C},    //70000CEA AFIT8_yuviirnr_iDiffThreshH_UV   [7:0] AFIT8_yuviirnr_iMaxThreshL_Y
	{0x0F12, 0x1402},    //70000CEC AFIT8_yuviirnr_iMaxThreshH_Y   [7:0] AFIT8_yuviirnr_iMaxThreshL_UV
	{0x0F12, 0x0618},    //70000CEE AFIT8_yuviirnr_iMaxThreshH_UV   [7:0] AFIT8_yuviirnr_iYNRStrengthL
	{0x0F12, 0x1A02},    //70000CF0 AFIT8_yuviirnr_iYNRStrengthH   [7:0] AFIT8_yuviirnr_iUVNRStrengthL
	{0x0F12, 0x8018},    //70000CF2 AFIT8_yuviirnr_iUVNRStrengthH   [7:0] AFIT8_byr_gras_iShadingPower
	{0x0F12, 0x0080},    //70000CF4 AFIT8_RGBGamma2_iLinearity   [7:0] AFIT8_RGBGamma2_iDarkReduce
	{0x0F12, 0x0080},    //70000CF6 AFIT8_ccm_oscar_iSaturation   [7:0] AFIT8_RGB2YUV_iYOffset
	{0x0F12, 0x0180},    //70000CF8 AFIT8_RGB2YUV_iRGBGain   [7:0] AFIT8_bnr_nClustLevel_H
	{0x0F12, 0x0A0A},    //70000CFA AFIT8_bnr_iClustMulT_H   [7:0] AFIT8_bnr_iClustMulT_C
	{0x0F12, 0x0101},    //70000CFC AFIT8_bnr_iClustThresh_H   [7:0] AFIT8_bnr_iClustThresh_C
	{0x0F12, 0x0C0F},    //70000CFE AFIT8_bnr_iDenThreshLow   [7:0] AFIT8_bnr_iDenThreshHigh
	{0x0F12, 0x6024},    //70000D00 AFIT8_ee_iLowSharpPower   [7:0] AFIT8_ee_iHighSharpPower
	{0x0F12, 0x0808},    //70000D02 AFIT8_ee_iLowShDenoise   [7:0] AFIT8_ee_iHighShDenoise
	{0x0F12, 0xFFFF},    //70000D04 AFIT8_ee_iLowSharpClamp   [7:0] AFIT8_ee_iHighSharpClamp
	{0x0F12, 0x0808},    //70000D06 AFIT8_ee_iReduceEdgeMinMult   [7:0] AFIT8_ee_iReduceEdgeSlope
	{0x0F12, 0x0A01},    //70000D08 AFIT8_bnr_nClustLevel_H_Bin   [7:0] AFIT8_bnr_iClustMulT_H_Bin
	{0x0F12, 0x010A},    //70000D0A AFIT8_bnr_iClustMulT_C_Bin   [7:0] AFIT8_bnr_iClustThresh_H_Bin
	{0x0F12, 0x0F01},    //70000D0C AFIT8_bnr_iClustThresh_C_Bin   [7:0] AFIT8_bnr_iDenThreshLow_Bin
	{0x0F12, 0x240C},    //70000D0E AFIT8_bnr_iDenThreshHigh_Bin   [7:0] AFIT8_ee_iLowSharpPower_Bin
	{0x0F12, 0x0860},    //70000D10 AFIT8_ee_iHighSharpPower_Bin   [7:0] AFIT8_ee_iLowShDenoise_Bin
	{0x0F12, 0xFF08},    //70000D12 AFIT8_ee_iHighShDenoise_Bin   [7:0] AFIT8_ee_iLowSharpClamp_Bin
	{0x0F12, 0x08FF},    //70000D14 AFIT8_ee_iHighSharpClamp_Bin   [7:0] AFIT8_ee_iReduceEdgeMinMult_Bin
	{0x0F12, 0x0008},    //70000D16 AFIT8_ee_iReduceEdgeSlope_Bin [7:0]
	{0x0F12, 0x0001},    //70000D18 AFITB_bnr_nClustLevel_C      [0]   bWideWide[1]
	{0x0F12, 0x23CE}, 	//70000D1A	//[0]CAFITB_bnr_bypass
	{0x0F12, 0xFDC8}, 	//70000D1C	//[0]CAFITB_bnr_bSlopenessTune
	{0x0F12, 0x112E}, 	//70000D1E	//[0]CAFITB_ee_bReduceNegMedSh
	{0x0F12, 0x93A5}, 	//70000D20	//[0]CAFITB_dmsc_bDoDesat
	{0x0F12, 0xFE67}, 	//70000D22	//[0]CAFITB_postdmsc_bSat
	{0x0F12, 0x0000}, 	//70000D24	//[0]CAFITB_yuviirnr_bWideY																				//


//===================================================================
// 18.JPEG Thumnail Setting
//===================================================================
// JPEG Quality
	{0x002A, 0x0478},
	{0x0F12, 0x005F}, 	//REG_TC_BRC_usPrevQuality
	{0x0F12, 0x005F}, 	//REG_TC_BRC_usCaptureQuality


	{0x0F12, 0x0001}, 	//REG_TC_THUMB_Thumb_bActive // JPEG Thumnail
	{0x0F12, 0x0280}, 	//REG_TC_THUMB_Thumb_uWidth //640
	{0x0F12, 0x01E0}, 	//REG_TC_THUMB_Thumb_uHeight //480
	{0x0F12, 0x0005}, 	//REG_TC_THUMB_Thumb_Format //YUV


	{0x002A, 0x17DC},   // JPEG setting
	{0x0F12, 0x0054}, 	//jpeg_ManualMBCV
	{0x002A, 0x1AE4},
	{0x0F12, 0x001C}, 	//senHal_bExtraAddLine
	{0x002A, 0x0284},
	{0x0F12, 0x0001}, 	//REG_TC_GP_bBypassScalerJpg
	{0x002A, 0x028A},
	{0x0F12, 0x0000}, 	//REG_TC_GP_bUse1FrameCaptureMode //0:continus capture frame}, 1:single capture frame

	{0x002A, 0x1CC2}, //DRx_uDRxWeight for AutoCont function
	{0x0F12, 0x0100},
	{0x0F12, 0x0100},
	{0x0F12, 0x0100},
	{0x0F12, 0x0100},
	{0x002A, 0x147C}, 	// Brightness min/Max
	{0x0F12, 0x0170},  ///*bp_uMaxBrightnessFactor*/
	{0x002A, 0x1482},
	{0x0F12, 0x01E0},  ///*bp_uMinBrightnessFactor	*/

//===================================================================
// Input Width & Height
//===================================================================
	{0x002A, 0x0250},
	{0x0F12, 0x0A00},
	{0x0F12, 0x0780},
	{0x0F12, 0x0010},
	{0x0F12, 0x000C},
	{0x0F12, 0x0A00},
	{0x0F12, 0x0780},
	{0x0F12, 0x0010},
	{0x0F12, 0x000C},
	{0x002A, 0x0494},
	{0x0F12, 0x0A00},
	{0x0F12, 0x0780},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0A00},
	{0x0F12, 0x0780},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x002A, 0x0262},
	{0x0F12, 0x0001},
	{0x0F12, 0x0001},


//===================================================================
// Preview
//===================================================================
	{0x002A, 0x02A6},   	//Preview config[0] 640 480  10~30fps
	{0x0F12, 0x0280},   	//REG_0TC_PCFG_usWidth
	{0x0F12, 0x01E0},   	//REG_0TC_PCFG_usHeight
	{0x0F12, 0x0005},   	//REG_0TC_PCFG_Format //5:YUV}, 7:RAW}, 9:JPEG
	{0x0F12, 0x278D},   	//REG_0TC_PCFG_usMaxOut4KHzRate
	{0x0F12, 0x278D},   	//REG_0TC_PCFG_usMinOut4KHzRate
	{0x0F12, 0x0100}, 	//REG_0TC_PCFG_OutClkPerPix88
	{0x0F12, 0x0300}, 	//REG_0TC_PCFG_uBpp88
	{0x0F12, 0x0012}, 	//REG_0TC_PCFG_PVIMask //[1]:PCLK Inversion
	{0x0F12, 0x0000}, 	//REG_0TC_PCFG_OIFMask
	{0x0F12, 0x01E0}, 	//REG_0TC_PCFG_usJpegPacketSize
	{0x0F12, 0x0000}, 	//REG_0TC_PCFG_usJpegTotalPackets
	{0x0F12, 0x0000}, 	//REG_0TC_PCFG_uClockInd
	{0x0F12, 0x0000}, 	//REG_0TC_PCFG_usFrTimeType
	{0x0F12, 0x0001}, 	//REG_0TC_PCFG_FrRateQualityType
	{0x0F12, 0x03E8}, 	//REG_0TC_PCFG_usMaxFrTimeMsecMult10 //03E8h:10fps
	{0x0F12, 0x014A}, 	//REG_0TC_PCFG_usMinFrTimeMsecMult10 //014Ah:30fps
	{0x002A, 0x02D0},
	{0x0F12, 0x0000}, 	//REG_0TC_PCFG_uPrevMirror
	{0x0F12, 0x0000}, 	//REG_0TC_PCFG_uCaptureMirror

//===================================================================
// Capture
//===================================================================
	{0x002A, 0x0396},
	{0x0F12, 0x0000}, 	//REG_0TC_CCFG_uCaptureMode
	{0x0F12, 0x0A00}, 	//REG_0TC_CCFG_usWidth //2560
	{0x0F12, 0x0780}, 	//REG_0TC_CCFG_usHeight //1920
	{0x0F12, 0x0005}, 	//REG_0TC_CCFG_Format //5:YUV}, 7:RAW}, 9:JPEG
	{0x0F12, 0x278D},   	//REG_0TC_CCFG_usMaxOut4KHzRate
	{0x0F12, 0x278D},   	//REG_0TC_CCFG_usMinOut4KHzRate
	{0x0F12, 0x0100}, 	//REG_0TC_CCFG_OutClkPerPix88
	{0x0F12, 0x0300}, 	//REG_0TC_CCFG_uBpp88
	{0x0F12, 0x0012}, 	//REG_0TC_CCFG_PVIMask //[1]:PCLK Inversion
	{0x0F12, 0x0070}, 	//REG_0TC_CCFG_OIFMask
	{0x0F12, 0x0810}, 	//REG_0TC_CCFG_usJpegPacketSize //2064d
	{0x0F12, 0x0900}, 	//REG_0TC_CCFG_usJpegTotalPackets //2304d //Must be multiples of 16
	{0x0F12, 0x0001}, 	//REG_0TC_CCFG_uClockInd
	{0x0F12, 0x0000}, 	//REG_0TC_CCFG_usFrTimeType
	{0x0F12, 0x0002}, 	//REG_0TC_CCFG_FrRateQualityType
	{0x0F12, 0x0535}, 	//REG_0TC_CCFG_usMaxFrTimeMsecMult10 //0535h:7.5fps
	{0x0F12, 0x029A}, 	//REG_0TC_CCFG_usMinFrTimeMsecMult10 //029Ah:15fps
	{0x002A, 0x022C},
	{0x0F12, 0x0001}, 	//REG_TC_IPRM_InitParamsUpdated

//===========================================================
// 21.Select Cofigration Display
//===========================================================
	{0x0028, 0x7000},
	{0x002A, 0x0266},
	{0x0F12, 0x0000}, 	//REG_TC_GP_ActivePrevConfig
	{0x002A, 0x026A},
	{0x0F12, 0x0001}, 	//REG_TC_GP_PrevOpenAfterChange
	{0x002A, 0x0268},
	{0x0F12, 0x0001}, 	//REG_TC_GP_PrevConfigChanged
	{0x002A, 0x026E},
	{0x0f12, 0x0000},
	{0x002A, 0x026A},
	{0x0F12, 0x0001}, 	//REG_TC_GP_PrevOpenAfterChange
	{0x002A, 0x0270},
	{0x0F12, 0x0001}, 	//REG_TC_GP_CapConfigChanged                      '
	{0x002A, 0x024E},
	{0x0F12, 0x0001}, 	//REG_TC_GP_NewConfigSync//
	{0x002A, 0x023E},
	{0x0F12, 0x0001},     //REG_TC_GP_EnablePreview
	{0x0F12, 0x0001}, 	//REG_TC_GP_EnablePreviewChanged




//===================================================================================
// 22. ESD Check
//===================================================================================

	{0x0028, 0x7000},
	{0x002A, 0x01A8}, 	///*ESD Check*/
	{0x0F12, 0xAAAA},
	{0x0028, 0x147C},
	{0x0F12, 0x0170},
	{0x0028, 0x1482},
	{0x0F12, 0x01E0},


//===================================================================================
// 24.ISSUE
//===================================================================================
//20110728 : Sequence Changed by image dev. (by J.M.Ahn)
//20110728 : ESD Check Register Address Change
//20110829 : TnP Changed ( by S.Y.Lee)
//20120104 : init Parm Update sequence changed by J.M.Ahn)
//20120201 : Flash  Green Noise  setting (by J.M.Ahn)
//20120229 : Brightness Block  (by J.W.Yoo)
};

//640X480 YUV
LOCAL const SENSOR_REG_T s5k4ec_640X480[]=
{
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},

	{0x002A, 0x18AC},
	{0x0F12, 0x0060},	//senHal_uAddColsBin
	{0x0F12, 0x0060},	//senHal_uAddColsNoBin
	{0x0F12, 0x05C0},	//senHal_uMinColsBin
	{0x0F12, 0x05C0},	//senHal_uMinColsNoBin

	{0x002A, 0x0250},
	{0x0F12, 0x0A00},	//REG_TC_GP_PrevReqInputWidth //2560
	{0x0F12, 0x0780},	//REG_TC_GP_PrevReqInputHeight //1920
	{0x0F12, 0x0010},	//REG_TC_GP_PrevInputWidthOfs //(2592-2560)/2
	{0x0F12, 0x000C},	//REG_TC_GP_PrevInputHeightOfs //(1944-1920)/2

	{0x002A, 0x0262},
	{0x0F12, 0x0001},	//REG_TC_GP_bUseReqInputInPre

	{0x002A, 0x0494},
	{0x0F12, 0x0A00},	//REG_TC_PZOOM_PrevZoomReqInputWidth //2560
	{0x0F12, 0x0780},	//REG_TC_PZOOM_PrevZoomReqInputHeight //1920
	{0x0F12, 0x0000},	//REG_TC_PZOOM_PrevZoomReqInputWidthOfs
	{0x0F12, 0x0000},	//REG_TC_PZOOM_PrevZoomReqInputHeightOfs

	{0x002A, 0x02A6},
	{0x0F12, 0x0280},	//REG_0TC_PCFG_usWidth //640
	{0x0F12, 0x01E0},	//REG_0TC_PCFG_usHeight //480

	{0x002A, 0x0266},
	{0x0F12, 0x0000},	//REG_TC_GP_ActivePrevConfig
	{0x002A, 0x026A},
	{0x0F12, 0x0001},	//REG_TC_GP_PrevOpenAfterChange
	{0x002A, 0x024E},
	{0x0F12, 0x0001},	//REG_TC_GP_NewConfigSync
	{0x002A, 0x0268},
	{0x0F12, 0x0001},	//REG_TC_GP_PrevConfigChanged

};

LOCAL const SENSOR_REG_T s5k4ec_1280X720[] =
{
	{0xFCFC, 0xD000},
	{0x0028, 0xD000},
	{0x002A, 0xE410},
	{0x0F12, 0x3E01},
	{0x0028, 0x7000},
	{0x002A, 0x18AC},
	{0x0F12, 0x0060},
	{0x0F12, 0x0060},
	{0x0F12, 0x07DC},
	{0x0F12, 0x05C0},

	{0x002A, 0x1484},
	{0x0F12, 0x002A},

	{0x002A, 0x1492},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},
	{0x0F12, 0x0101},

	{0x002A, 0x1568},
	{0x0F12, 0x0030},
	{0x0F12, 0x0090},
	{0x0F12, 0x00A8},
	{0x0F12, 0x00C0},
	{0x0F12, 0x0100},
	{0x0F12, 0x0140},
	{0x0F12, 0x0180},
	{0x0F12, 0x0400},
	{0x0F12, 0x2000},

	{0x0F12, 0x0080},
	{0x0F12, 0x00D0},
	{0x0F12, 0x00D8},
	{0x0F12, 0x00f8},
	{0x0F12, 0x0100},
	{0x0F12, 0x0103},
	{0x0F12, 0x0110},
	{0x0F12, 0x0150},
	{0x0F12, 0x0400},

	{0x002A, 0x0544},
	{0x0F12, 0x0105},
	{0x0F12, 0x00FA},

	{0x002A, 0x0588},
	{0x0F12, 0x0001},

	{0x002A, 0x0582},
	{0x0F12, 0x00D0},

	{0x002A, 0x47B0},
	{0x0F12, 0x0001},

	{0x002A, 0x139A},
	{0x0F12, 0x0258},

	{0x002A, 0x1464},
	{0x0F12, 0x0008},
	{0x0F12, 0xFFFF},
	{0x0F12, 0x0010},
	{0x0F12, 0x0020},

	{0x002A, 0x13A4},
	{0x0F12, 0x0000},
	{0x0F12, 0xFFD8},
	{0x0F12, 0xFFD8},
	{0x0F12, 0xFFD8},
	{0x0F12, 0xFFD8},
	{0x0F12, 0xFFD0},

	{0x0F12, 0x0000},
	{0x0F12, 0xFFD8},
	{0x0F12, 0xFFD8},
	{0x0F12, 0xFFD8},
	{0x0F12, 0xFFD8},
	{0x0F12, 0xFFD0},

	{0x0F12, 0x0000},
	{0x0F12, 0xFFD8},
	{0x0F12, 0xFFD8},
	{0x0F12, 0xFFD8},
	{0x0F12, 0xFFD8},
	{0x0F12, 0xFFD0},

	{0x0F12, 0xFFEC},
	{0x0F12, 0x000A},
	{0x0F12, 0x000A},
	{0x0F12, 0xFFC4},
	{0x0F12, 0xFFC4},
	{0x0F12, 0xFF66},

	{0x0F12, 0xFFEC},
	{0x0F12, 0x000A},
	{0x0F12, 0x000A},
	{0x0F12, 0xFFC4},
	{0x0F12, 0xFFC4},
	{0x0F12, 0xFF66},

	{0x0F12, 0xFFEC},
	{0x0F12, 0x000A},
	{0x0F12, 0x000A},
	{0x0F12, 0xFFC4},
	{0x0F12, 0xFFC4},
	{0x0F12, 0xFF66},

	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},

	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},

	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},

	{0x0F12, 0xFFC0},
	{0x0F12, 0xFFC0},
	{0x0F12, 0xFFC0},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},

	{0x0F12, 0xFFC0},
	{0x0F12, 0xFFC0},
	{0x0F12, 0xFFC0},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},

	{0x0F12, 0xFFC0},
	{0x0F12, 0xFFC0},
	{0x0F12, 0xFFC0},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},

	{0x002A, 0x1208},
	{0x0F12, 0x0020},

	{0x002A, 0x144E},
	{0x0F12, 0xFFE0},
	{0x0F12, 0xFFE0},
	{0x0F12, 0x0000},

	{0x002A, 0x4800},
	{0x0F12, 0x0208},
	{0x0F12, 0xFFB5},
	{0x0F12, 0xFFE8},
	{0x0F12, 0xFF20},
	{0x0F12, 0x01BF},
	{0x0F12, 0xFF53},
	{0x0F12, 0x0022},
	{0x0F12, 0xFFEA},
	{0x0F12, 0x01C2},
	{0x0F12, 0x00C6},
	{0x0F12, 0x0095},
	{0x0F12, 0xFEFD},
	{0x0F12, 0x0206},
	{0x0F12, 0xFF7F},
	{0x0F12, 0x0191},
	{0x0F12, 0xFF06},
	{0x0F12, 0x01BA},
	{0x0F12, 0x0108},

	{0x0F12, 0x0208},
	{0x0F12, 0xFFB5},
	{0x0F12, 0xFFE8},
	{0x0F12, 0xFF20},
	{0x0F12, 0x01BF},
	{0x0F12, 0xFF53},
	{0x0F12, 0x0022},
	{0x0F12, 0xFFEA},
	{0x0F12, 0x01C2},
	{0x0F12, 0x00C6},
	{0x0F12, 0x0095},
	{0x0F12, 0xFEFD},
	{0x0F12, 0x0206},
	{0x0F12, 0xFF7F},
	{0x0F12, 0x0191},
	{0x0F12, 0xFF06},
	{0x0F12, 0x01BA},
	{0x0F12, 0x0108},

	{0x0F12, 0x0208},
	{0x0F12, 0xFFB5},
	{0x0F12, 0xFFE8},
	{0x0F12, 0xFF20},
	{0x0F12, 0x01BF},
	{0x0F12, 0xFF53},
	{0x0F12, 0x0022},
	{0x0F12, 0xFFEA},
	{0x0F12, 0x01C2},
	{0x0F12, 0x00C6},
	{0x0F12, 0x0095},
	{0x0F12, 0xFEFD},
	{0x0F12, 0x0206},
	{0x0F12, 0xFF7F},
	{0x0F12, 0x0191},
	{0x0F12, 0xFF06},
	{0x0F12, 0x01BA},
	{0x0F12, 0x0108},

	{0x0F12, 0x0209},
	{0x0F12, 0xFFBD},
	{0x0F12, 0xFFDF},
	{0x0F12, 0xFF20},
	{0x0F12, 0x01BF},
	{0x0F12, 0xFF53},
	{0x0F12, 0x0022},
	{0x0F12, 0xFFEA},
	{0x0F12, 0x01C2},
	{0x0F12, 0x00CC},
	{0x0F12, 0x008F},
	{0x0F12, 0xFEFE},
	{0x0F12, 0x0206},
	{0x0F12, 0xFF7F},
	{0x0F12, 0x0191},
	{0x0F12, 0xFF06},
	{0x0F12, 0x01BA},
	{0x0F12, 0x0108},

	{0x0F12, 0x0209},
	{0x0F12, 0xFFBD},
	{0x0F12, 0xFFDF},
	{0x0F12, 0xFF20},
	{0x0F12, 0x01BF},
	{0x0F12, 0xFF53},
	{0x0F12, 0x0022},
	{0x0F12, 0xFFEA},
	{0x0F12, 0x01C2},
	{0x0F12, 0x00CC},
	{0x0F12, 0x008F},
	{0x0F12, 0xFEFE},
	{0x0F12, 0x0206},
	{0x0F12, 0xFF7F},
	{0x0F12, 0x0191},
	{0x0F12, 0xFF06},
	{0x0F12, 0x01BA},
	{0x0F12, 0x0108},

	{0x0F12, 0x0209},
	{0x0F12, 0xFFBD},
	{0x0F12, 0xFFDF},
	{0x0F12, 0xFF20},
	{0x0F12, 0x01BF},
	{0x0F12, 0xFF53},
	{0x0F12, 0x0022},
	{0x0F12, 0xFFEA},
	{0x0F12, 0x01C2},
	{0x0F12, 0x00CC},
	{0x0F12, 0x008F},
	{0x0F12, 0xFEFE},
	{0x0F12, 0x0206},
	{0x0F12, 0xFF7F},
	{0x0F12, 0x0191},
	{0x0F12, 0xFF06},
	{0x0F12, 0x01BA},
	{0x0F12, 0x0108},



	{0x002A, 0x0938},
	{0x0F12, 0x0001},
	{0x0F12, 0x0014},
	{0x0F12, 0x00D2},
	{0x0F12, 0x0784},
	{0x0F12, 0x10D0},
	{0x0F12, 0x1388},

	{0x002A, 0x098C},
	{0x0F12, 0x0000},   //7000098C AFIT16_BRIGHTNESS
	{0x0F12, 0x0000},   //7000098E AFIT16_CONTRAST
	{0x0F12, 0x0000},   //70000990 AFIT16_SATURATION
	{0x0F12, 0x0000},   //70000992 AFIT16_SHARP_BLUR
	{0x0F12, 0x0000},   //70000994 AFIT16_GLAMOUR
	{0x0F12, 0x00C0},   //70000996 AFIT16_bnr_edge_high
	{0x0F12, 0x0064},   //70000998 AFIT16_postdmsc_iLowBright
	{0x0F12, 0x0384},   //7000099A AFIT16_postdmsc_iHighBright
	{0x0F12, 0x0051},   //7000099C AFIT16_postdmsc_iLowSat
	{0x0F12, 0x01F4},   //7000099E AFIT16_postdmsc_iHighSat
	{0x0F12, 0x0070},   //700009A0 AFIT16_postdmsc_iTune
	{0x0F12, 0x0040},   //700009A2 AFIT16_yuvemix_mNegRanges_0
	{0x0F12, 0x00A0},   //700009A4 AFIT16_yuvemix_mNegRanges_1
	{0x0F12, 0x0100},   //700009A6 AFIT16_yuvemix_mNegRanges_2
	{0x0F12, 0x0010},   //700009A8 AFIT16_yuvemix_mPosRanges_0
	{0x0F12, 0x0060},   //700009AA AFIT16_yuvemix_mPosRanges_1
	{0x0F12, 0x0100},   //700009AC AFIT16_yuvemix_mPosRanges_2
	{0x0F12, 0x1430},   //700009AE AFIT8_bnr_edge_low  [7:0] AFIT8_bnr_repl_thresh
	{0x0F12, 0x0201},   //700009B0 AFIT8_bnr_repl_force  [7:0] AFIT8_bnr_iHotThreshHigh
	{0x0F12, 0x0204},   //700009B2 AFIT8_bnr_iHotThreshLow   [7:0] AFIT8_bnr_iColdThreshHigh
	{0x0F12, 0x2404},   //700009B4 AFIT8_bnr_iColdThreshLow   [7:0] AFIT8_bnr_DispTH_Low
	{0x0F12, 0x031B},   //700009B6 AFIT8_bnr_DispTH_High   [7:0] AFIT8_bnr_DISP_Limit_Low
	{0x0F12, 0x0103},   //700009B8 AFIT8_bnr_DISP_Limit_High   [7:0] AFIT8_bnr_iDistSigmaMin
	{0x0F12, 0x1205},   //700009BA AFIT8_bnr_iDistSigmaMax   [7:0] AFIT8_bnr_iDiffSigmaLow
	{0x0F12, 0x400D},   //700009BC AFIT8_bnr_iDiffSigmaHigh   [7:0] AFIT8_bnr_iNormalizedSTD_TH
	{0x0F12, 0x0080},   //700009BE AFIT8_bnr_iNormalizedSTD_Limit   [7:0] AFIT8_bnr_iDirNRTune
	{0x0F12, 0x2080},   //700009C0 AFIT8_bnr_iDirMinThres   [7:0] AFIT8_bnr_iDirFltDiffThresHigh
	{0x0F12, 0x3040},   //700009C2 AFIT8_bnr_iDirFltDiffThresLow   [7:0] AFIT8_bnr_iDirSmoothPowerHigh
	{0x0F12, 0x0630},   //700009C4 AFIT8_bnr_iDirSmoothPowerLow   [7:0] AFIT8_bnr_iLowMaxSlopeAllowed
	{0x0F12, 0x0306},   //700009C6 AFIT8_bnr_iHighMaxSlopeAllowed   [7:0] AFIT8_bnr_iLowSlopeThresh
	{0x0F12, 0x2003},   //700009C8 AFIT8_bnr_iHighSlopeThresh   [7:0] AFIT8_bnr_iSlopenessTH
	{0x0F12, 0xFF01},   //700009CA AFIT8_bnr_iSlopeBlurStrength   [7:0] AFIT8_bnr_iSlopenessLimit
	{0x0F12, 0x0404},   //700009CC AFIT8_bnr_AddNoisePower1   [7:0] AFIT8_bnr_AddNoisePower2
	{0x0F12, 0x0300},   //700009CE AFIT8_bnr_iRadialTune   [7:0] AFIT8_bnr_iRadialPower
	{0x0F12, 0x245A},   //700009D0 AFIT8_bnr_iRadialLimit   [7:0] AFIT8_ee_iFSMagThLow
	{0x0F12, 0x1018},   //700009D2 AFIT8_ee_iFSMagThHigh   [7:0] AFIT8_ee_iFSVarThLow
	{0x0F12, 0x000B},   //700009D4 AFIT8_ee_iFSVarThHigh   [7:0] AFIT8_ee_iFSThLow
	{0x0F12, 0x0B00},   //700009D6 AFIT8_ee_iFSThHigh   [7:0] AFIT8_ee_iFSmagPower
	{0x0F12, 0x5A0F},   //700009D8 AFIT8_ee_iFSVarCountTh   [7:0] AFIT8_ee_iRadialLimit
	{0x0F12, 0x0505},   //700009DA AFIT8_ee_iRadialPower   [7:0] AFIT8_ee_iSmoothEdgeSlope
	{0x0F12, 0x1802},   //700009DC AFIT8_ee_iROADThres   [7:0] AFIT8_ee_iROADMaxNR
	{0x0F12, 0x0000},   //700009DE AFIT8_ee_iROADSubMaxNR   [7:0] AFIT8_ee_iROADSubThres
	{0x0F12, 0x2006},   //700009E0 AFIT8_ee_iROADNeiThres   [7:0] AFIT8_ee_iROADNeiMaxNR
	{0x0F12, 0x3428},   //700009E2 AFIT8_ee_iSmoothEdgeThres   [7:0] AFIT8_ee_iMSharpen
	{0x0F12, 0x041C},   //700009E4 AFIT8_ee_iWSharpen   [7:0] AFIT8_ee_iMShThresh
	{0x0F12, 0x0101},   //700009E6 AFIT8_ee_iWShThresh   [7:0] AFIT8_ee_iReduceNegative
	{0x0F12, 0x0800},   //700009E8 AFIT8_ee_iEmbossCentAdd   [7:0] AFIT8_ee_iShDespeckle
	{0x0F12, 0x1004},   //700009EA AFIT8_ee_iReduceEdgeThresh   [7:0] AFIT8_dmsc_iEnhThresh
	{0x0F12, 0x4008},   //700009EC AFIT8_dmsc_iDesatThresh   [7:0] AFIT8_dmsc_iDemBlurHigh
	{0x0F12, 0x0540},   //700009EE AFIT8_dmsc_iDemBlurLow   [7:0] AFIT8_dmsc_iDemBlurRange
	{0x0F12, 0x8006},   //700009F0 AFIT8_dmsc_iDecisionThresh   [7:0] AFIT8_dmsc_iCentGrad
	{0x0F12, 0x0020},   //700009F2 AFIT8_dmsc_iMonochrom   [7:0] AFIT8_dmsc_iGBDenoiseVal
	{0x0F12, 0x0000},   //700009F4 AFIT8_dmsc_iGRDenoiseVal   [7:0] AFIT8_dmsc_iEdgeDesatThrHigh
	{0x0F12, 0x1800},   //700009F6 AFIT8_dmsc_iEdgeDesatThrLow   [7:0] AFIT8_dmsc_iEdgeDesat
	{0x0F12, 0x0000},   //700009F8 AFIT8_dmsc_iNearGrayDesat   [7:0] AFIT8_dmsc_iEdgeDesatLimit
	{0x0F12, 0x1E10},   //700009FA AFIT8_postdmsc_iBCoeff   [7:0] AFIT8_postdmsc_iGCoeff
	{0x0F12, 0x000B},   //700009FC AFIT8_postdmsc_iWideMult   [7:0] AFIT8_yuvemix_mNegSlopes_0
	{0x0F12, 0x0607},   //700009FE AFIT8_yuvemix_mNegSlopes_1   [7:0] AFIT8_yuvemix_mNegSlopes_2
	{0x0F12, 0x0005},   //70000A00 AFIT8_yuvemix_mNegSlopes_3   [7:0] AFIT8_yuvemix_mPosSlopes_0
	{0x0F12, 0x0607},   //70000A02 AFIT8_yuvemix_mPosSlopes_1   [7:0] AFIT8_yuvemix_mPosSlopes_2
	{0x0F12, 0x0405},   //70000A04 AFIT8_yuvemix_mPosSlopes_3   [7:0] AFIT8_yuviirnr_iXSupportY
	{0x0F12, 0x0205},   //70000A06 AFIT8_yuviirnr_iXSupportUV   [7:0] AFIT8_yuviirnr_iLowYNorm
	{0x0F12, 0x0304},   //70000A08 AFIT8_yuviirnr_iHighYNorm   [7:0] AFIT8_yuviirnr_iLowUVNorm
	{0x0F12, 0x0409},   //70000A0A AFIT8_yuviirnr_iHighUVNorm   [7:0] AFIT8_yuviirnr_iYNormShift
	{0x0F12, 0x0306},   //70000A0C AFIT8_yuviirnr_iUVNormShift   [7:0] AFIT8_yuviirnr_iVertLength_Y
	{0x0F12, 0x0407},   //70000A0E AFIT8_yuviirnr_iVertLength_UV   [7:0] AFIT8_yuviirnr_iDiffThreshL_Y
	{0x0F12, 0x1804},   //70000A10 AFIT8_yuviirnr_iDiffThreshH_Y   [7:0] AFIT8_yuviirnr_iDiffThreshL_UV
	{0x0F12, 0x0214},   //70000A12 AFIT8_yuviirnr_iDiffThreshH_UV   [7:0] AFIT8_yuviirnr_iMaxThreshL_Y
	{0x0F12, 0x1002},   //70000A14 AFIT8_yuviirnr_iMaxThreshH_Y   [7:0] AFIT8_yuviirnr_iMaxThreshL_UV
	{0x0F12, 0x0610},   //70000A16 AFIT8_yuviirnr_iMaxThreshH_UV   [7:0] AFIT8_yuviirnr_iYNRStrengthL
	{0x0F12, 0x1A02},   //70000A18 AFIT8_yuviirnr_iYNRStrengthH   [7:0] AFIT8_yuviirnr_iUVNRStrengthL
	{0x0F12, 0x0000},  //0000},   //8018},   //70000A1A AFIT8_yuviirnr_iUVNRStrengthH   [7:0] AFIT8_byr_gras_iShadingPower
	{0x0F12, 0x00B0},  //0080},  //00B0},   //70000A1C AFIT8_RGBGamma2_iLinearity   [7:0] AFIT8_RGBGamma2_iDarkReduce
	{0x0F12, 0x0080},  //1480},   //70000A1E AFIT8_ccm_oscar_iSaturation   [7:0] AFIT8_RGB2YUV_iYOffset
	{0x0F12, 0x0180},   //70000A20 AFIT8_RGB2YUV_iRGBGain   [7:0] AFIT8_bnr_nClustLevel_H
	{0x0F12, 0x0A0A},   //70000A22 AFIT8_bnr_iClustMulT_H   [7:0] AFIT8_bnr_iClustMulT_C
	{0x0F12, 0x0101},   //70000A24 AFIT8_bnr_iClustThresh_H   [7:0] AFIT8_bnr_iClustThresh_C
	{0x0F12, 0x1B24},   //70000A26 AFIT8_bnr_iDenThreshLow   [7:0] AFIT8_bnr_iDenThreshHigh
	{0x0F12, 0x6024},   //70000A28 AFIT8_ee_iLowSharpPower   [7:0] AFIT8_ee_iHighSharpPower
	{0x0F12, 0x1D22},   //70000A2A AFIT8_ee_iLowShDenoise   [7:0] AFIT8_ee_iHighShDenoise
	{0x0F12, 0xFFFF},   //70000A2C AFIT8_ee_iLowSharpClamp   [7:0] AFIT8_ee_iHighSharpClamp
	{0x0F12, 0x0808},   //70000A2E AFIT8_ee_iReduceEdgeMinMult   [7:0] AFIT8_ee_iReduceEdgeSlope
	{0x0F12, 0x0A01},   //70000A30 AFIT8_bnr_nClustLevel_H_Bin   [7:0] AFIT8_bnr_iClustMulT_H_Bin
	{0x0F12, 0x010A},   //70000A32 AFIT8_bnr_iClustMulT_C_Bin   [7:0] AFIT8_bnr_iClustThresh_H_Bin
	{0x0F12, 0x2401},   //70000A34 AFIT8_bnr_iClustThresh_C_Bin   [7:0] AFIT8_bnr_iDenThreshLow_Bin
	{0x0F12, 0x241B},   //70000A36 AFIT8_bnr_iDenThreshHigh_Bin   [7:0] AFIT8_ee_iLowSharpPower_Bin
	{0x0F12, 0x1E60},   //70000A38 AFIT8_ee_iHighSharpPower_Bin   [7:0] AFIT8_ee_iLowShDenoise_Bin
	{0x0F12, 0xFF18},   //70000A3A AFIT8_ee_iHighShDenoise_Bin   [7:0] AFIT8_ee_iLowSharpClamp_Bin
	{0x0F12, 0x08FF},   //70000A3C AFIT8_ee_iHighSharpClamp_Bin   [7:0] AFIT8_ee_iReduceEdgeMinMult_Bin
	{0x0F12, 0x0008},   //70000A3E AFIT8_ee_iReduceEdgeSlope_Bin [7:0]
	{0x0F12, 0x0001},   //70000A40 AFITB_bnr_nClustLevel_C      [0]

	{0x0F12, 0x0000},   //70000A42 AFIT16_BRIGHTNESS
	{0x0F12, 0x0000},   //70000A44 AFIT16_CONTRAST
	{0x0F12, 0x0000},   //70000A46 AFIT16_SATURATION
	{0x0F12, 0x0000},   //70000A48 AFIT16_SHARP_BLUR
	{0x0F12, 0x0000},   //70000A4A AFIT16_GLAMOUR
	{0x0F12, 0x00C0},   //70000A4C AFIT16_bnr_edge_high
	{0x0F12, 0x0064},   //70000A4E AFIT16_postdmsc_iLowBright
	{0x0F12, 0x0384},   //70000A50 AFIT16_postdmsc_iHighBright
	{0x0F12, 0x0051},   //70000A52 AFIT16_postdmsc_iLowSat
	{0x0F12, 0x01F4},   //70000A54 AFIT16_postdmsc_iHighSat
	{0x0F12, 0x0070},   //70000A56 AFIT16_postdmsc_iTune
	{0x0F12, 0x0040},   //70000A58 AFIT16_yuvemix_mNegRanges_0
	{0x0F12, 0x00A0},   //70000A5A AFIT16_yuvemix_mNegRanges_1
	{0x0F12, 0x0100},   //70000A5C AFIT16_yuvemix_mNegRanges_2
	{0x0F12, 0x0010},   //70000A5E AFIT16_yuvemix_mPosRanges_0
	{0x0F12, 0x0060},   //70000A60 AFIT16_yuvemix_mPosRanges_1
	{0x0F12, 0x0100},   //70000A62 AFIT16_yuvemix_mPosRanges_2
	{0x0F12, 0x1430},   //70000A64 AFIT8_bnr_edge_low  [7:0] AFIT8_bnr_repl_thresh
	{0x0F12, 0x0201},   //70000A66 AFIT8_bnr_repl_force  [7:0] AFIT8_bnr_iHotThreshHigh
	{0x0F12, 0x0204},   //70000A68 AFIT8_bnr_iHotThreshLow   [7:0] AFIT8_bnr_iColdThreshHigh
	{0x0F12, 0x1B04},   //70000A6A AFIT8_bnr_iColdThreshLow   [7:0] AFIT8_bnr_DispTH_Low
	{0x0F12, 0x0312},   //70000A6C AFIT8_bnr_DispTH_High   [7:0] AFIT8_bnr_DISP_Limit_Low
	{0x0F12, 0x0003},   //70000A6E AFIT8_bnr_DISP_Limit_High   [7:0] AFIT8_bnr_iDistSigmaMin
	{0x0F12, 0x0C03},   //70000A70 AFIT8_bnr_iDistSigmaMax   [7:0] AFIT8_bnr_iDiffSigmaLow
	{0x0F12, 0x2806},   //70000A72 AFIT8_bnr_iDiffSigmaHigh   [7:0] AFIT8_bnr_iNormalizedSTD_TH
	{0x0F12, 0x0060},   //70000A74 AFIT8_bnr_iNormalizedSTD_Limit   [7:0] AFIT8_bnr_iDirNRTune
	{0x0F12, 0x1540},   //70000A76 AFIT8_bnr_iDirMinThres   [7:0] AFIT8_bnr_iDirFltDiffThresHigh
	{0x0F12, 0x201C},   //70000A78 AFIT8_bnr_iDirFltDiffThresLow   [7:0] AFIT8_bnr_iDirSmoothPowerHigh
	{0x0F12, 0x0620},   //70000A7A AFIT8_bnr_iDirSmoothPowerLow   [7:0] AFIT8_bnr_iLowMaxSlopeAllowed
	{0x0F12, 0x0306},   //70000A7C AFIT8_bnr_iHighMaxSlopeAllowed   [7:0] AFIT8_bnr_iLowSlopeThresh
	{0x0F12, 0x2003},   //70000A7E AFIT8_bnr_iHighSlopeThresh   [7:0] AFIT8_bnr_iSlopenessTH
	{0x0F12, 0xFF01},   //70000A80 AFIT8_bnr_iSlopeBlurStrength   [7:0] AFIT8_bnr_iSlopenessLimit
	{0x0F12, 0x0404},   //70000A82 AFIT8_bnr_AddNoisePower1   [7:0] AFIT8_bnr_AddNoisePower2
	{0x0F12, 0x0300},   //70000A84 AFIT8_bnr_iRadialTune   [7:0] AFIT8_bnr_iRadialPower
	{0x0F12, 0x145A},   //70000A86 AFIT8_bnr_iRadialLimit   [7:0] AFIT8_ee_iFSMagThLow
	{0x0F12, 0x1010},   //70000A88 AFIT8_ee_iFSMagThHigh   [7:0] AFIT8_ee_iFSVarThLow
	{0x0F12, 0x000B},   //70000A8A AFIT8_ee_iFSVarThHigh   [7:0] AFIT8_ee_iFSThLow
	{0x0F12, 0x0B00},   //70000A8C AFIT8_ee_iFSThHigh   [7:0] AFIT8_ee_iFSmagPower
	{0x0F12, 0x5A0F},   //70000A8E AFIT8_ee_iFSVarCountTh   [7:0] AFIT8_ee_iRadialLimit
	{0x0F12, 0x0503},   //70000A90 AFIT8_ee_iRadialPower   [7:0] AFIT8_ee_iSmoothEdgeSlope
	{0x0F12, 0x1802},   //70000A92 AFIT8_ee_iROADThres   [7:0] AFIT8_ee_iROADMaxNR
	{0x0F12, 0x0000},   //70000A94 AFIT8_ee_iROADSubMaxNR   [7:0] AFIT8_ee_iROADSubThres
	{0x0F12, 0x2006},   //70000A96 AFIT8_ee_iROADNeiThres   [7:0] AFIT8_ee_iROADNeiMaxNR
	{0x0F12, 0x3C28},   //70000A98 AFIT8_ee_iSmoothEdgeThres   [7:0] AFIT8_ee_iMSharpen
	{0x0F12, 0x0428},   //70000A9A AFIT8_ee_iWSharpen   [7:0] AFIT8_ee_iMShThresh
	{0x0F12, 0x0101},   //70000A9C AFIT8_ee_iWShThresh   [7:0] AFIT8_ee_iReduceNegative
	{0x0F12, 0x8000},   //70000A9E AFIT8_ee_iEmbossCentAdd   [7:0] AFIT8_ee_iShDespeckle
	{0x0F12, 0x1004},   //70000AA0 AFIT8_ee_iReduceEdgeThresh   [7:0] AFIT8_dmsc_iEnhThresh
	{0x0F12, 0x4008},   //70000AA2 AFIT8_dmsc_iDesatThresh   [7:0] AFIT8_dmsc_iDemBlurHigh
	{0x0F12, 0x0540},   //70000AA4 AFIT8_dmsc_iDemBlurLow   [7:0] AFIT8_dmsc_iDemBlurRange
	{0x0F12, 0x8006},   //70000AA6 AFIT8_dmsc_iDecisionThresh   [7:0] AFIT8_dmsc_iCentGrad
	{0x0F12, 0x0020},   //70000AA8 AFIT8_dmsc_iMonochrom   [7:0] AFIT8_dmsc_iGBDenoiseVal
	{0x0F12, 0x0000},   //70000AAA AFIT8_dmsc_iGRDenoiseVal   [7:0] AFIT8_dmsc_iEdgeDesatThrHigh
	{0x0F12, 0x1800},   //70000AAC AFIT8_dmsc_iEdgeDesatThrLow   [7:0] AFIT8_dmsc_iEdgeDesat
	{0x0F12, 0x0000},   //70000AAE AFIT8_dmsc_iNearGrayDesat   [7:0] AFIT8_dmsc_iEdgeDesatLimit
	{0x0F12, 0x1E10},   //70000AB0 AFIT8_postdmsc_iBCoeff   [7:0] AFIT8_postdmsc_iGCoeff
	{0x0F12, 0x000B},   //70000AB2 AFIT8_postdmsc_iWideMult   [7:0] AFIT8_yuvemix_mNegSlopes_0
	{0x0F12, 0x0607},   //70000AB4 AFIT8_yuvemix_mNegSlopes_1   [7:0] AFIT8_yuvemix_mNegSlopes_2
	{0x0F12, 0x0005},   //70000AB6 AFIT8_yuvemix_mNegSlopes_3   [7:0] AFIT8_yuvemix_mPosSlopes_0
	{0x0F12, 0x0607},   //70000AB8 AFIT8_yuvemix_mPosSlopes_1   [7:0] AFIT8_yuvemix_mPosSlopes_2
	{0x0F12, 0x0405},   //70000ABA AFIT8_yuvemix_mPosSlopes_3   [7:0] AFIT8_yuviirnr_iXSupportY
	{0x0F12, 0x0205},   //70000ABC AFIT8_yuviirnr_iXSupportUV   [7:0] AFIT8_yuviirnr_iLowYNorm
	{0x0F12, 0x0304},   //70000ABE AFIT8_yuviirnr_iHighYNorm   [7:0] AFIT8_yuviirnr_iLowUVNorm
	{0x0F12, 0x0409},   //70000AC0 AFIT8_yuviirnr_iHighUVNorm   [7:0] AFIT8_yuviirnr_iYNormShift
	{0x0F12, 0x0306},   //70000AC2 AFIT8_yuviirnr_iUVNormShift   [7:0] AFIT8_yuviirnr_iVertLength_Y
	{0x0F12, 0x0407},   //70000AC4 AFIT8_yuviirnr_iVertLength_UV   [7:0] AFIT8_yuviirnr_iDiffThreshL_Y
	{0x0F12, 0x1804},   //70000AC6 AFIT8_yuviirnr_iDiffThreshH_Y   [7:0] AFIT8_yuviirnr_iDiffThreshL_UV
	{0x0F12, 0x0214},   //70000AC8 AFIT8_yuviirnr_iDiffThreshH_UV   [7:0] AFIT8_yuviirnr_iMaxThreshL_Y
	{0x0F12, 0x1002},   //70000ACA AFIT8_yuviirnr_iMaxThreshH_Y   [7:0] AFIT8_yuviirnr_iMaxThreshL_UV
	{0x0F12, 0x0610},   //70000ACC AFIT8_yuviirnr_iMaxThreshH_UV   [7:0] AFIT8_yuviirnr_iYNRStrengthL
	{0x0F12, 0x1A02},   //70000ACE AFIT8_yuviirnr_iYNRStrengthH   [7:0] AFIT8_yuviirnr_iUVNRStrengthL
	{0x0F12, 0x8018},   //70000AD0 AFIT8_yuviirnr_iUVNRStrengthH   [7:0] AFIT8_byr_gras_iShadingPower
	{0x0F12, 0x00A0},   //70000AD2 AFIT8_RGBGamma2_iLinearity   [7:0] AFIT8_RGBGamma2_iDarkReduce
	{0x0F12, 0x1480},   //70000AD4 AFIT8_ccm_oscar_iSaturation   [7:0] AFIT8_RGB2YUV_iYOffset
	{0x0F12, 0x0180},   //70000AD6 AFIT8_RGB2YUV_iRGBGain   [7:0] AFIT8_bnr_nClustLevel_H
	{0x0F12, 0x0A0A},   //70000AD8 AFIT8_bnr_iClustMulT_H   [7:0] AFIT8_bnr_iClustMulT_C
	{0x0F12, 0x0101},   //70000ADA AFIT8_bnr_iClustThresh_H   [7:0] AFIT8_bnr_iClustThresh_C
	{0x0F12, 0x1B24},   //70000ADC AFIT8_bnr_iDenThreshLow   [7:0] AFIT8_bnr_iDenThreshHigh
	{0x0F12, 0x6024},   //70000ADE AFIT8_ee_iLowSharpPower   [7:0] AFIT8_ee_iHighSharpPower
	{0x0F12, 0x0C0C},   //70000AE0 AFIT8_ee_iLowShDenoise   [7:0] AFIT8_ee_iHighShDenoise
	{0x0F12, 0xFFFF},   //70000AE2 AFIT8_ee_iLowSharpClamp   [7:0] AFIT8_ee_iHighSharpClamp
	{0x0F12, 0x0808},   //70000AE4 AFIT8_ee_iReduceEdgeMinMult   [7:0] AFIT8_ee_iReduceEdgeSlope
	{0x0F12, 0x0A01},   //70000AE6 AFIT8_bnr_nClustLevel_H_Bin   [7:0] AFIT8_bnr_iClustMulT_H_Bin
	{0x0F12, 0x010A},   //70000AE8 AFIT8_bnr_iClustMulT_C_Bin   [7:0] AFIT8_bnr_iClustThresh_H_Bin
	{0x0F12, 0x1501},   //70000AEA AFIT8_bnr_iClustThresh_C_Bin   [7:0] AFIT8_bnr_iDenThreshLow_Bin
	{0x0F12, 0x240F},   //70000AEC AFIT8_bnr_iDenThreshHigh_Bin   [7:0] AFIT8_ee_iLowSharpPower_Bin
	{0x0F12, 0x0C60},   //70000AEE AFIT8_ee_iHighSharpPower_Bin   [7:0] AFIT8_ee_iLowShDenoise_Bin
	{0x0F12, 0xFF0C},   //70000AF0 AFIT8_ee_iHighShDenoise_Bin   [7:0] AFIT8_ee_iLowSharpClamp_Bin
	{0x0F12, 0x08FF},   //70000AF2 AFIT8_ee_iHighSharpClamp_Bin   [7:0] AFIT8_ee_iReduceEdgeMinMult_Bin
	{0x0F12, 0x0008},   //70000AF4 AFIT8_ee_iReduceEdgeSlope_Bin [7:0]
	{0x0F12, 0x0001},   //70000AF6 AFITB_bnr_nClustLevel_C      [0]

	{0x0F12, 0x0000},   //70000AF8 AFIT16_BRIGHTNESS
	{0x0F12, 0x0000},   //70000AFA AFIT16_CONTRAST
	{0x0F12, 0x0000},   //70000AFC AFIT16_SATURATION
	{0x0F12, 0x0000},   //70000AFE AFIT16_SHARP_BLUR
	{0x0F12, 0x0000},   //70000B00 AFIT16_GLAMOUR
	{0x0F12, 0x00C0},   //70000B02 AFIT16_bnr_edge_high
	{0x0F12, 0x0064},   //70000B04 AFIT16_postdmsc_iLowBright
	{0x0F12, 0x0384},   //70000B06 AFIT16_postdmsc_iHighBright
	{0x0F12, 0x0043},   //70000B08 AFIT16_postdmsc_iLowSat
	{0x0F12, 0x01F4},   //70000B0A AFIT16_postdmsc_iHighSat
	{0x0F12, 0x0070},   //70000B0C AFIT16_postdmsc_iTune
	{0x0F12, 0x0040},   //70000B0E AFIT16_yuvemix_mNegRanges_0
	{0x0F12, 0x00A0},   //70000B10 AFIT16_yuvemix_mNegRanges_1
	{0x0F12, 0x0100},   //70000B12 AFIT16_yuvemix_mNegRanges_2
	{0x0F12, 0x0010},   //70000B14 AFIT16_yuvemix_mPosRanges_0
	{0x0F12, 0x0060},   //70000B16 AFIT16_yuvemix_mPosRanges_1
	{0x0F12, 0x0100},   //70000B18 AFIT16_yuvemix_mPosRanges_2
	{0x0F12, 0x1430},   //70000B1A AFIT8_bnr_edge_low  [7:0] AFIT8_bnr_repl_thresh
	{0x0F12, 0x0201},   //70000B1C AFIT8_bnr_repl_force  [7:0] AFIT8_bnr_iHotThreshHigh
	{0x0F12, 0x0204},   //70000B1E AFIT8_bnr_iHotThreshLow   [7:0] AFIT8_bnr_iColdThreshHigh
	{0x0F12, 0x1B04},   //70000B20 AFIT8_bnr_iColdThreshLow   [7:0] AFIT8_bnr_DispTH_Low
	{0x0F12, 0x0312},   //70000B22 AFIT8_bnr_DispTH_High   [7:0] AFIT8_bnr_DISP_Limit_Low
	{0x0F12, 0x0003},   //70000B24 AFIT8_bnr_DISP_Limit_High   [7:0] AFIT8_bnr_iDistSigmaMin
	{0x0F12, 0x0C03},   //70000B26 AFIT8_bnr_iDistSigmaMax   [7:0] AFIT8_bnr_iDiffSigmaLow
	{0x0F12, 0x2806},   //70000B28 AFIT8_bnr_iDiffSigmaHigh   [7:0] AFIT8_bnr_iNormalizedSTD_TH
	{0x0F12, 0x0060},   //70000B2A AFIT8_bnr_iNormalizedSTD_Limit   [7:0] AFIT8_bnr_iDirNRTune
	{0x0F12, 0x1540},   //70000B2C AFIT8_bnr_iDirMinThres   [7:0] AFIT8_bnr_iDirFltDiffThresHigh
	{0x0F12, 0x201C},   //70000B2E AFIT8_bnr_iDirFltDiffThresLow   [7:0] AFIT8_bnr_iDirSmoothPowerHigh
	{0x0F12, 0x0620},   //70000B30 AFIT8_bnr_iDirSmoothPowerLow   [7:0] AFIT8_bnr_iLowMaxSlopeAllowed
	{0x0F12, 0x0306},   //70000B32 AFIT8_bnr_iHighMaxSlopeAllowed   [7:0] AFIT8_bnr_iLowSlopeThresh
	{0x0F12, 0x2003},   //70000B34 AFIT8_bnr_iHighSlopeThresh   [7:0] AFIT8_bnr_iSlopenessTH
	{0x0F12, 0xFF01},   //70000B36 AFIT8_bnr_iSlopeBlurStrength   [7:0] AFIT8_bnr_iSlopenessLimit
	{0x0F12, 0x0404},   //70000B38 AFIT8_bnr_AddNoisePower1   [7:0] AFIT8_bnr_AddNoisePower2
	{0x0F12, 0x0300},   //70000B3A AFIT8_bnr_iRadialTune   [7:0] AFIT8_bnr_iRadialPower
	{0x0F12, 0x145A},   //70000B3C AFIT8_bnr_iRadialLimit   [7:0] AFIT8_ee_iFSMagThLow
	{0x0F12, 0x1010},   //70000B3E AFIT8_ee_iFSMagThHigh   [7:0] AFIT8_ee_iFSVarThLow
	{0x0F12, 0x000B},   //70000B40 AFIT8_ee_iFSVarThHigh   [7:0] AFIT8_ee_iFSThLow
	{0x0F12, 0x0E00},   //70000B42 AFIT8_ee_iFSThHigh   [7:0] AFIT8_ee_iFSmagPower
	{0x0F12, 0x5A0F},   //70000B44 AFIT8_ee_iFSVarCountTh   [7:0] AFIT8_ee_iRadialLimit
	{0x0F12, 0x0503},   //70000B46 AFIT8_ee_iRadialPower   [7:0] AFIT8_ee_iSmoothEdgeSlope
	{0x0F12, 0x1802},   //70000B48 AFIT8_ee_iROADThres   [7:0] AFIT8_ee_iROADMaxNR
	{0x0F12, 0x0000},   //70000B4A AFIT8_ee_iROADSubMaxNR   [7:0] AFIT8_ee_iROADSubThres
	{0x0F12, 0x2006},   //70000B4C AFIT8_ee_iROADNeiThres   [7:0] AFIT8_ee_iROADNeiMaxNR
	{0x0F12, 0x3C28},   //70000B4E AFIT8_ee_iSmoothEdgeThres   [7:0] AFIT8_ee_iMSharpen
	{0x0F12, 0x0428},   //70000B50 AFIT8_ee_iWSharpen   [7:0] AFIT8_ee_iMShThresh
	{0x0F12, 0x0101},   //70000B52 AFIT8_ee_iWShThresh   [7:0] AFIT8_ee_iReduceNegative
	{0x0F12, 0x8000},   //70000B54 AFIT8_ee_iEmbossCentAdd   [7:0] AFIT8_ee_iShDespeckle
	{0x0F12, 0x0A04},   //70000B56 AFIT8_ee_iReduceEdgeThresh   [7:0] AFIT8_dmsc_iEnhThresh
	{0x0F12, 0x4008},   //70000B58 AFIT8_dmsc_iDesatThresh   [7:0] AFIT8_dmsc_iDemBlurHigh
	{0x0F12, 0x0540},   //70000B5A AFIT8_dmsc_iDemBlurLow   [7:0] AFIT8_dmsc_iDemBlurRange
	{0x0F12, 0x8006},   //70000B5C AFIT8_dmsc_iDecisionThresh   [7:0] AFIT8_dmsc_iCentGrad
	{0x0F12, 0x0020},   //70000B5E AFIT8_dmsc_iMonochrom   [7:0] AFIT8_dmsc_iGBDenoiseVal
	{0x0F12, 0x0000},   //70000B60 AFIT8_dmsc_iGRDenoiseVal   [7:0] AFIT8_dmsc_iEdgeDesatThrHigh
	{0x0F12, 0x1800},   //70000B62 AFIT8_dmsc_iEdgeDesatThrLow   [7:0] AFIT8_dmsc_iEdgeDesat
	{0x0F12, 0x0000},   //70000B64 AFIT8_dmsc_iNearGrayDesat   [7:0] AFIT8_dmsc_iEdgeDesatLimit
	{0x0F12, 0x1E10},   //70000B66 AFIT8_postdmsc_iBCoeff   [7:0] AFIT8_postdmsc_iGCoeff
	{0x0F12, 0x000B},   //70000B68 AFIT8_postdmsc_iWideMult   [7:0] AFIT8_yuvemix_mNegSlopes_0
	{0x0F12, 0x0607},   //70000B6A AFIT8_yuvemix_mNegSlopes_1   [7:0] AFIT8_yuvemix_mNegSlopes_2
	{0x0F12, 0x0005},   //70000B6C AFIT8_yuvemix_mNegSlopes_3   [7:0] AFIT8_yuvemix_mPosSlopes_0
	{0x0F12, 0x0607},   //70000B6E AFIT8_yuvemix_mPosSlopes_1   [7:0] AFIT8_yuvemix_mPosSlopes_2
	{0x0F12, 0x0405},   //70000B70 AFIT8_yuvemix_mPosSlopes_3   [7:0] AFIT8_yuviirnr_iXSupportY
	{0x0F12, 0x0205},   //70000B72 AFIT8_yuviirnr_iXSupportUV   [7:0] AFIT8_yuviirnr_iLowYNorm
	{0x0F12, 0x0304},   //70000B74 AFIT8_yuviirnr_iHighYNorm   [7:0] AFIT8_yuviirnr_iLowUVNorm
	{0x0F12, 0x0409},   //70000B76 AFIT8_yuviirnr_iHighUVNorm   [7:0] AFIT8_yuviirnr_iYNormShift
	{0x0F12, 0x0306},   //70000B78 AFIT8_yuviirnr_iUVNormShift   [7:0] AFIT8_yuviirnr_iVertLength_Y
	{0x0F12, 0x0407},   //70000B7A AFIT8_yuviirnr_iVertLength_UV   [7:0] AFIT8_yuviirnr_iDiffThreshL_Y
	{0x0F12, 0x1804},   //70000B7C AFIT8_yuviirnr_iDiffThreshH_Y   [7:0] AFIT8_yuviirnr_iDiffThreshL_UV
	{0x0F12, 0x0214},   //70000B7E AFIT8_yuviirnr_iDiffThreshH_UV   [7:0] AFIT8_yuviirnr_iMaxThreshL_Y
	{0x0F12, 0x1002},   //70000B80 AFIT8_yuviirnr_iMaxThreshH_Y   [7:0] AFIT8_yuviirnr_iMaxThreshL_UV
	{0x0F12, 0x0610},   //70000B82 AFIT8_yuviirnr_iMaxThreshH_UV   [7:0] AFIT8_yuviirnr_iYNRStrengthL
	{0x0F12, 0x1A02},   //70000B84 AFIT8_yuviirnr_iYNRStrengthH   [7:0] AFIT8_yuviirnr_iUVNRStrengthL
	{0x0F12, 0x8018},   //70000B86 AFIT8_yuviirnr_iUVNRStrengthH   [7:0] AFIT8_byr_gras_iShadingPower
	{0x0F12, 0x0080},   //70000B88 AFIT8_RGBGamma2_iLinearity   [7:0] AFIT8_RGBGamma2_iDarkReduce
	{0x0F12, 0x1180},   //70000B8A AFIT8_ccm_oscar_iSaturation   [7:0] AFIT8_RGB2YUV_iYOffset
	{0x0F12, 0x0180},   //70000B8C AFIT8_RGB2YUV_iRGBGain   [7:0] AFIT8_bnr_nClustLevel_H
	{0x0F12, 0x0A0A},   //70000B8E AFIT8_bnr_iClustMulT_H   [7:0] AFIT8_bnr_iClustMulT_C
	{0x0F12, 0x0101},   //70000B90 AFIT8_bnr_iClustThresh_H   [7:0] AFIT8_bnr_iClustThresh_C
	{0x0F12, 0x141D},   //70000B92 AFIT8_bnr_iDenThreshLow   [7:0] AFIT8_bnr_iDenThreshHigh
	{0x0F12, 0x6024},   //70000B94 AFIT8_ee_iLowSharpPower   [7:0] AFIT8_ee_iHighSharpPower
	{0x0F12, 0x0C0C},   //70000B96 AFIT8_ee_iLowShDenoise   [7:0] AFIT8_ee_iHighShDenoise
	{0x0F12, 0xFFFF},   //70000B98 AFIT8_ee_iLowSharpClamp   [7:0] AFIT8_ee_iHighSharpClamp
	{0x0F12, 0x0808},   //70000B9A AFIT8_ee_iReduceEdgeMinMult   [7:0] AFIT8_ee_iReduceEdgeSlope
	{0x0F12, 0x0A01},   //70000B9C AFIT8_bnr_nClustLevel_H_Bin   [7:0] AFIT8_bnr_iClustMulT_H_Bin
	{0x0F12, 0x010A},   //70000B9E AFIT8_bnr_iClustMulT_C_Bin   [7:0] AFIT8_bnr_iClustThresh_H_Bin
	{0x0F12, 0x1501},   //70000BA0 AFIT8_bnr_iClustThresh_C_Bin   [7:0] AFIT8_bnr_iDenThreshLow_Bin
	{0x0F12, 0x240F},   //70000BA2 AFIT8_bnr_iDenThreshHigh_Bin   [7:0] AFIT8_ee_iLowSharpPower_Bin
	{0x0F12, 0x0C60},   //70000BA4 AFIT8_ee_iHighSharpPower_Bin   [7:0] AFIT8_ee_iLowShDenoise_Bin
	{0x0F12, 0xFF0C},   //70000BA6 AFIT8_ee_iHighShDenoise_Bin   [7:0] AFIT8_ee_iLowSharpClamp_Bin
	{0x0F12, 0x08FF},   //70000BA8 AFIT8_ee_iHighSharpClamp_Bin   [7:0] AFIT8_ee_iReduceEdgeMinMult_Bin
	{0x0F12, 0x0008},   //70000BAA AFIT8_ee_iReduceEdgeSlope_Bin [7:0]
	{0x0F12, 0x0001},   //70000BAC AFITB_bnr_nClustLevel_C      [0]

	{0x0F12, 0x0000},
	{0x0F12, 0x0000},   //70000BAE AFIT16_BRIGHTNESS
	{0x0F12, 0x0000},   //70000BB0 AFIT16_CONTRAST
	{0x0F12, 0x0000},   //70000BB2 AFIT16_SATURATION
	{0x0F12, 0x0000},   //70000BB4 AFIT16_SHARP_BLUR
	{0x0F12, 0x00C0},   //70000BB6 AFIT16_GLAMOUR
	{0x0F12, 0x0064},   //70000BB8 AFIT16_bnr_edge_high
	{0x0F12, 0x0384},   //70000BBA AFIT16_postdmsc_iLowBright
	{0x0F12, 0x0032},   //70000BBC AFIT16_postdmsc_iHighBright
	{0x0F12, 0x01F4},   //70000BBE AFIT16_postdmsc_iLowSat
	{0x0F12, 0x0070},   //70000BC0 AFIT16_postdmsc_iHighSat
	{0x0F12, 0x0040},   //70000BC2 AFIT16_postdmsc_iTune
	{0x0F12, 0x00A0},   //70000BC4 AFIT16_yuvemix_mNegRanges_0
	{0x0F12, 0x0100},   //70000BC6 AFIT16_yuvemix_mNegRanges_1
	{0x0F12, 0x0010},   //70000BC8 AFIT16_yuvemix_mNegRanges_2
	{0x0F12, 0x0060},   //70000BCA AFIT16_yuvemix_mPosRanges_0
	{0x0F12, 0x0100},   //70000BCC AFIT16_yuvemix_mPosRanges_1
	{0x0F12, 0x1430},   //70000BCE AFIT16_yuvemix_mPosRanges_2
	{0x0F12, 0x0201},   //70000BD0 AFIT8_bnr_edge_low  [7:0] AFIT8_bnr_repl_thresh
	{0x0F12, 0x0204},   //70000BD2 AFIT8_bnr_repl_force  [7:0] AFIT8_bnr_iHotThreshHigh
	{0x0F12, 0x1504},   //70000BD4 AFIT8_bnr_iHotThreshLow   [7:0] AFIT8_bnr_iColdThreshHigh
	{0x0F12, 0x030F},   //70000BD6 AFIT8_bnr_iColdThreshLow   [7:0] AFIT8_bnr_DispTH_Low
	{0x0F12, 0x0003},   //70000BD8 AFIT8_bnr_DispTH_High   [7:0] AFIT8_bnr_DISP_Limit_Low
	{0x0F12, 0x0902},   //70000BDA AFIT8_bnr_DISP_Limit_High   [7:0] AFIT8_bnr_iDistSigmaMin
	{0x0F12, 0x2004},   //70000BDC AFIT8_bnr_iDistSigmaMax   [7:0] AFIT8_bnr_iDiffSigmaLow
	{0x0F12, 0x0050},   //70000BDE AFIT8_bnr_iDiffSigmaHigh   [7:0] AFIT8_bnr_iNormalizedSTD_TH
	{0x0F12, 0x1140},   //70000BE0 AFIT8_bnr_iNormalizedSTD_Limit   [7:0] AFIT8_bnr_iDirNRTune
	{0x0F12, 0x201C},   //70000BE2 AFIT8_bnr_iDirMinThres   [7:0] AFIT8_bnr_iDirFltDiffThresHigh
	{0x0F12, 0x0620},   //70000BE4 AFIT8_bnr_iDirFltDiffThresLow   [7:0] AFIT8_bnr_iDirSmoothPowerHigh
	{0x0F12, 0x0306},   //70000BE6 AFIT8_bnr_iDirSmoothPowerLow   [7:0] AFIT8_bnr_iLowMaxSlopeAllowed
	{0x0F12, 0x2003},   //70000BE8 AFIT8_bnr_iHighMaxSlopeAllowed   [7:0] AFIT8_bnr_iLowSlopeThresh
	{0x0F12, 0xFF01},   //70000BEA AFIT8_bnr_iHighSlopeThresh   [7:0] AFIT8_bnr_iSlopenessTH
	{0x0F12, 0x0404},   //70000BEC AFIT8_bnr_iSlopeBlurStrength   [7:0] AFIT8_bnr_iSlopenessLimit
	{0x0F12, 0x0300},   //70000BEE AFIT8_bnr_AddNoisePower1   [7:0] AFIT8_bnr_AddNoisePower2
	{0x0F12, 0x145A},   //70000BF0 AFIT8_bnr_iRadialTune   [7:0] AFIT8_bnr_iRadialPower
	{0x0F12, 0x1010},   //70000BF2 AFIT8_bnr_iRadialLimit   [7:0] AFIT8_ee_iFSMagThLow
	{0x0F12, 0x000B},   //70000BF4 AFIT8_ee_iFSMagThHigh   [7:0] AFIT8_ee_iFSVarThLow
	{0x0F12, 0x1000},   //70000BF6 AFIT8_ee_iFSVarThHigh   [7:0] AFIT8_ee_iFSThLow
	{0x0F12, 0x5A0F},   //70000BF8 AFIT8_ee_iFSThHigh   [7:0] AFIT8_ee_iFSmagPower
	{0x0F12, 0x0503},   //70000BFA AFIT8_ee_iFSVarCountTh   [7:0] AFIT8_ee_iRadialLimit
	{0x0F12, 0x1802},   //70000BFC AFIT8_ee_iRadialPower   [7:0] AFIT8_ee_iSmoothEdgeSlope
	{0x0F12, 0x0000},   //70000BFE AFIT8_ee_iROADThres   [7:0] AFIT8_ee_iROADMaxNR
	{0x0F12, 0x2006},   //70000C00 AFIT8_ee_iROADSubMaxNR   [7:0] AFIT8_ee_iROADSubThres
	{0x0F12, 0x3C28},   //70000C02 AFIT8_ee_iROADNeiThres   [7:0] AFIT8_ee_iROADNeiMaxNR
	{0x0F12, 0x042C},   //70000C04 AFIT8_ee_iSmoothEdgeThres   [7:0] AFIT8_ee_iMSharpen
	{0x0F12, 0x0101},   //70000C06 AFIT8_ee_iWSharpen   [7:0] AFIT8_ee_iMShThresh
	{0x0F12, 0x8000},   //70000C08 AFIT8_ee_iWShThresh   [7:0] AFIT8_ee_iReduceNegative
	{0x0F12, 0x0904},   //70000C0A AFIT8_ee_iEmbossCentAdd   [7:0] AFIT8_ee_iShDespeckle
	{0x0F12, 0x4008},   //70000C0C AFIT8_ee_iReduceEdgeThresh   [7:0] AFIT8_dmsc_iEnhThresh
	{0x0F12, 0x0540},   //70000C0E AFIT8_dmsc_iDesatThresh   [7:0] AFIT8_dmsc_iDemBlurHigh
	{0x0F12, 0x8006},   //70000C10 AFIT8_dmsc_iDemBlurLow   [7:0] AFIT8_dmsc_iDemBlurRange
	{0x0F12, 0x0020},   //70000C12 AFIT8_dmsc_iDecisionThresh   [7:0] AFIT8_dmsc_iCentGrad
	{0x0F12, 0x0000},   //70000C14 AFIT8_dmsc_iMonochrom   [7:0] AFIT8_dmsc_iGBDenoiseVal
	{0x0F12, 0x1800},   //70000C16 AFIT8_dmsc_iGRDenoiseVal   [7:0] AFIT8_dmsc_iEdgeDesatThrHigh
	{0x0F12, 0x0000},   //70000C18 AFIT8_dmsc_iEdgeDesatThrLow   [7:0] AFIT8_dmsc_iEdgeDesat
	{0x0F12, 0x1E10},   //70000C1A AFIT8_dmsc_iNearGrayDesat   [7:0] AFIT8_dmsc_iEdgeDesatLimit
	{0x0F12, 0x000B},   //70000C1C AFIT8_postdmsc_iBCoeff   [7:0] AFIT8_postdmsc_iGCoeff
	{0x0F12, 0x0607},   //70000C1E AFIT8_postdmsc_iWideMult   [7:0] AFIT8_yuvemix_mNegSlopes_0
	{0x0F12, 0x0005},   //70000C20 AFIT8_yuvemix_mNegSlopes_1   [7:0] AFIT8_yuvemix_mNegSlopes_2
	{0x0F12, 0x0607},   //70000C22 AFIT8_yuvemix_mNegSlopes_3   [7:0] AFIT8_yuvemix_mPosSlopes_0
	{0x0F12, 0x0405},   //70000C24 AFIT8_yuvemix_mPosSlopes_1   [7:0] AFIT8_yuvemix_mPosSlopes_2
	{0x0F12, 0x0205},   //70000C26 AFIT8_yuvemix_mPosSlopes_3   [7:0] AFIT8_yuviirnr_iXSupportY
	{0x0F12, 0x0304},   //70000C28 AFIT8_yuviirnr_iXSupportUV   [7:0] AFIT8_yuviirnr_iLowYNorm
	{0x0F12, 0x0409},   //70000C2A AFIT8_yuviirnr_iHighYNorm   [7:0] AFIT8_yuviirnr_iLowUVNorm
	{0x0F12, 0x0306},   //70000C2C AFIT8_yuviirnr_iHighUVNorm   [7:0] AFIT8_yuviirnr_iYNormShift
	{0x0F12, 0x0407},   //70000C2E AFIT8_yuviirnr_iUVNormShift   [7:0] AFIT8_yuviirnr_iVertLength_Y
	{0x0F12, 0x2804},   //70000C30 AFIT8_yuviirnr_iVertLength_UV   [7:0] AFIT8_yuviirnr_iDiffThreshL_Y
	{0x0F12, 0x0228},   //70000C32 AFIT8_yuviirnr_iDiffThreshH_Y   [7:0] AFIT8_yuviirnr_iDiffThreshL_UV
	{0x0F12, 0x1402},   //70000C34 AFIT8_yuviirnr_iDiffThreshH_UV   [7:0] AFIT8_yuviirnr_iMaxThreshL_Y
	{0x0F12, 0x0618},   //70000C36 AFIT8_yuviirnr_iMaxThreshH_Y   [7:0] AFIT8_yuviirnr_iMaxThreshL_UV
	{0x0F12, 0x1A02},   //70000C38 AFIT8_yuviirnr_iMaxThreshH_UV   [7:0] AFIT8_yuviirnr_iYNRStrengthL
	{0x0F12, 0x8018},   //70000C3A AFIT8_yuviirnr_iYNRStrengthH   [7:0] AFIT8_yuviirnr_iUVNRStrengthL
	{0x0F12, 0x0080},   //70000C3C AFIT8_yuviirnr_iUVNRStrengthH   [7:0] AFIT8_byr_gras_iShadingPower
	{0x0F12, 0x1080},   //70000C3E AFIT8_RGBGamma2_iLinearity   [7:0] AFIT8_RGBGamma2_iDarkReduce
	{0x0F12, 0x0180},   //70000C40 AFIT8_ccm_oscar_iSaturation   [7:0] AFIT8_RGB2YUV_iYOffset
	{0x0F12, 0x0A0A},   //70000C42 AFIT8_RGB2YUV_iRGBGain   [7:0] AFIT8_bnr_nClustLevel_H
	{0x0F12, 0x0101},   //70000C44 AFIT8_bnr_iClustMulT_H   [7:0] AFIT8_bnr_iClustMulT_C
	{0x0F12, 0x1117},   //70000C46 AFIT8_bnr_iClustThresh_H   [7:0] AFIT8_bnr_iClustThresh_C
	{0x0F12, 0x6024},   //70000C48 AFIT8_bnr_iDenThreshLow   [7:0] AFIT8_bnr_iDenThreshHigh
	{0x0F12, 0x0A0A},   //70000C4A AFIT8_ee_iLowSharpPower   [7:0] AFIT8_ee_iHighSharpPower
	{0x0F12, 0xFFFF},   //70000C4C AFIT8_ee_iLowShDenoise   [7:0] AFIT8_ee_iHighShDenoise
	{0x0F12, 0x0808},   //70000C4E AFIT8_ee_iLowSharpClamp   [7:0] AFIT8_ee_iHighSharpClamp
	{0x0F12, 0x0A01},   //70000C50 AFIT8_ee_iReduceEdgeMinMult   [7:0] AFIT8_ee_iReduceEdgeSlope
	{0x0F12, 0x010A},   //70000C52 AFIT8_bnr_nClustLevel_H_Bin   [7:0] AFIT8_bnr_iClustMulT_H_Bin
	{0x0F12, 0x1501},   //70000C54 AFIT8_bnr_iClustMulT_C_Bin   [7:0] AFIT8_bnr_iClustThresh_H_Bin
	{0x0F12, 0x240F},   //70000C56 AFIT8_bnr_iClustThresh_C_Bin   [7:0] AFIT8_bnr_iDenThreshLow_Bin
	{0x0F12, 0x0A60},   //70000C58 AFIT8_bnr_iDenThreshHigh_Bin   [7:0] AFIT8_ee_iLowSharpPower_Bin
	{0x0F12, 0xFF0A},   //70000C5A AFIT8_ee_iHighSharpPower_Bin   [7:0] AFIT8_ee_iLowShDenoise_Bin
	{0x0F12, 0x08FF},   //70000C5C AFIT8_ee_iHighShDenoise_Bin   [7:0] AFIT8_ee_iLowSharpClamp_Bin
	{0x0F12, 0x0008},   //70000C5E AFIT8_ee_iHighSharpClamp_Bin   [7:0] AFIT8_ee_iReduceEdgeMinMult_Bin
	{0x0F12, 0x0001},   //70000C60 AFIT8_ee_iReduceEdgeSlope_Bin [7:0]
	                    //70000C62 AFITB_bnr_nClustLevel_C      [0]
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},   //70000C64 AFIT16_BRIGHTNESS
	{0x0F12, 0x0000},   //70000C66 AFIT16_CONTRAST
	{0x0F12, 0x0000},   //70000C68 AFIT16_SATURATION
	{0x0F12, 0x00C0},   //70000C6A AFIT16_SHARP_BLUR
	{0x0F12, 0x0064},   //70000C6C AFIT16_GLAMOUR
	{0x0F12, 0x0384},   //70000C6E AFIT16_bnr_edge_high
	{0x0F12, 0x0032},   //70000C70 AFIT16_postdmsc_iLowBright
	{0x0F12, 0x01F4},   //70000C72 AFIT16_postdmsc_iHighBright
	{0x0F12, 0x0070},   //70000C74 AFIT16_postdmsc_iLowSat
	{0x0F12, 0x0040},   //70000C76 AFIT16_postdmsc_iHighSat
	{0x0F12, 0x00A0},   //70000C78 AFIT16_postdmsc_iTune
	{0x0F12, 0x0100},   //70000C7A AFIT16_yuvemix_mNegRanges_0
	{0x0F12, 0x0010},   //70000C7C AFIT16_yuvemix_mNegRanges_1
	{0x0F12, 0x0060},   //70000C7E AFIT16_yuvemix_mNegRanges_2
	{0x0F12, 0x0100},   //70000C80 AFIT16_yuvemix_mPosRanges_0
	{0x0F12, 0x1430},   //70000C82 AFIT16_yuvemix_mPosRanges_1
	{0x0F12, 0x0201},   //70000C84 AFIT16_yuvemix_mPosRanges_2
	{0x0F12, 0x0204},   //70000C86 AFIT8_bnr_edge_low  [7:0] AFIT8_bnr_repl_thresh
	{0x0F12, 0x0F04},   //70000C88 AFIT8_bnr_repl_force  [7:0] AFIT8_bnr_iHotThreshHigh
	{0x0F12, 0x030C},   //70000C8A AFIT8_bnr_iHotThreshLow   [7:0] AFIT8_bnr_iColdThreshHigh
	{0x0F12, 0x0003},   //70000C8C AFIT8_bnr_iColdThreshLow   [7:0] AFIT8_bnr_DispTH_Low
	{0x0F12, 0x0602},   //70000C8E AFIT8_bnr_DispTH_High   [7:0] AFIT8_bnr_DISP_Limit_Low
	{0x0F12, 0x1803},   //70000C90 AFIT8_bnr_DISP_Limit_High   [7:0] AFIT8_bnr_iDistSigmaMin
	{0x0F12, 0x0040},   //70000C92 AFIT8_bnr_iDistSigmaMax   [7:0] AFIT8_bnr_iDiffSigmaLow
	{0x0F12, 0x0E20},   //70000C94 AFIT8_bnr_iDiffSigmaHigh   [7:0] AFIT8_bnr_iNormalizedSTD_TH
	{0x0F12, 0x2018},   //70000C96 AFIT8_bnr_iNormalizedSTD_Limit   [7:0] AFIT8_bnr_iDirNRTune
	{0x0F12, 0x0620},   //70000C98 AFIT8_bnr_iDirMinThres   [7:0] AFIT8_bnr_iDirFltDiffThresHigh
	{0x0F12, 0x0306},   //70000C9A AFIT8_bnr_iDirFltDiffThresLow   [7:0] AFIT8_bnr_iDirSmoothPowerHigh
	{0x0F12, 0x2003},   //70000C9C AFIT8_bnr_iDirSmoothPowerLow   [7:0] AFIT8_bnr_iLowMaxSlopeAllowed
	{0x0F12, 0xFF01},   //70000C9E AFIT8_bnr_iHighMaxSlopeAllowed   [7:0] AFIT8_bnr_iLowSlopeThresh
	{0x0F12, 0x0404},   //70000CA0 AFIT8_bnr_iHighSlopeThresh   [7:0] AFIT8_bnr_iSlopenessTH
	{0x0F12, 0x0200},   //70000CA2 AFIT8_bnr_iSlopeBlurStrength   [7:0] AFIT8_bnr_iSlopenessLimit
	{0x0F12, 0x145A},   //70000CA4 AFIT8_bnr_AddNoisePower1   [7:0] AFIT8_bnr_AddNoisePower2
	{0x0F12, 0x1010},   //70000CA6 AFIT8_bnr_iRadialTune   [7:0] AFIT8_bnr_iRadialPower
	{0x0F12, 0x000B},   //70000CA8 AFIT8_bnr_iRadialLimit   [7:0] AFIT8_ee_iFSMagThLow
	{0x0F12, 0x1200},   //70000CAA AFIT8_ee_iFSMagThHigh   [7:0] AFIT8_ee_iFSVarThLow
	{0x0F12, 0x5A0F},   //70000CAC AFIT8_ee_iFSVarThHigh   [7:0] AFIT8_ee_iFSThLow
	{0x0F12, 0x0502},   //70000CAE AFIT8_ee_iFSThHigh   [7:0] AFIT8_ee_iFSmagPower
	{0x0F12, 0x1802},   //70000CB0 AFIT8_ee_iFSVarCountTh   [7:0] AFIT8_ee_iRadialLimit
	{0x0F12, 0x0000},   //70000CB2 AFIT8_ee_iRadialPower   [7:0] AFIT8_ee_iSmoothEdgeSlope
	{0x0F12, 0x2006},   //70000CB4 AFIT8_ee_iROADThres   [7:0] AFIT8_ee_iROADMaxNR
	{0x0F12, 0x4028},   //70000CB6 AFIT8_ee_iROADSubMaxNR   [7:0] AFIT8_ee_iROADSubThres
	{0x0F12, 0x0430},   //70000CB8 AFIT8_ee_iROADNeiThres   [7:0] AFIT8_ee_iROADNeiMaxNR
	{0x0F12, 0x0101},   //70000CBA AFIT8_ee_iSmoothEdgeThres   [7:0] AFIT8_ee_iMSharpen
	{0x0F12, 0xFF00},   //70000CBC AFIT8_ee_iWSharpen   [7:0] AFIT8_ee_iMShThresh
	{0x0F12, 0x0804},   //70000CBE AFIT8_ee_iWShThresh   [7:0] AFIT8_ee_iReduceNegative
	{0x0F12, 0x4008},   //70000CC0 AFIT8_ee_iEmbossCentAdd   [7:0] AFIT8_ee_iShDespeckle
	{0x0F12, 0x0540},   //70000CC2 AFIT8_ee_iReduceEdgeThresh   [7:0] AFIT8_dmsc_iEnhThresh
	{0x0F12, 0x8006},   //70000CC4 AFIT8_dmsc_iDesatThresh   [7:0] AFIT8_dmsc_iDemBlurHigh
	{0x0F12, 0x0020},   //70000CC6 AFIT8_dmsc_iDemBlurLow   [7:0] AFIT8_dmsc_iDemBlurRange
	{0x0F12, 0x0000},   //70000CC8 AFIT8_dmsc_iDecisionThresh   [7:0] AFIT8_dmsc_iCentGrad
	{0x0F12, 0x1800},   //70000CCA AFIT8_dmsc_iMonochrom   [7:0] AFIT8_dmsc_iGBDenoiseVal
	{0x0F12, 0x0000},   //70000CCC AFIT8_dmsc_iGRDenoiseVal   [7:0] AFIT8_dmsc_iEdgeDesatThrHigh
	{0x0F12, 0x1E10},   //70000CCE AFIT8_dmsc_iEdgeDesatThrLow   [7:0] AFIT8_dmsc_iEdgeDesat
	{0x0F12, 0x000B},   //70000CD0 AFIT8_dmsc_iNearGrayDesat   [7:0] AFIT8_dmsc_iEdgeDesatLimit
	{0x0F12, 0x0607},   //70000CD2 AFIT8_postdmsc_iBCoeff   [7:0] AFIT8_postdmsc_iGCoeff
	{0x0F12, 0x0005},   //70000CD4 AFIT8_postdmsc_iWideMult   [7:0] AFIT8_yuvemix_mNegSlopes_0
	{0x0F12, 0x0607},   //70000CD6 AFIT8_yuvemix_mNegSlopes_1   [7:0] AFIT8_yuvemix_mNegSlopes_2
	{0x0F12, 0x0405},   //70000CD8 AFIT8_yuvemix_mNegSlopes_3   [7:0] AFIT8_yuvemix_mPosSlopes_0
	{0x0F12, 0x0205},   //70000CDA AFIT8_yuvemix_mPosSlopes_1   [7:0] AFIT8_yuvemix_mPosSlopes_2
	{0x0F12, 0x0304},   //70000CDC AFIT8_yuvemix_mPosSlopes_3   [7:0] AFIT8_yuviirnr_iXSupportY
	{0x0F12, 0x0409},   //70000CDE AFIT8_yuviirnr_iXSupportUV   [7:0] AFIT8_yuviirnr_iLowYNorm
	{0x0F12, 0x0306},   //70000CE0 AFIT8_yuviirnr_iHighYNorm   [7:0] AFIT8_yuviirnr_iLowUVNorm
	{0x0F12, 0x0407},   //70000CE2 AFIT8_yuviirnr_iHighUVNorm   [7:0] AFIT8_yuviirnr_iYNormShift
	{0x0F12, 0x2C04},   //70000CE4 AFIT8_yuviirnr_iUVNormShift   [7:0] AFIT8_yuviirnr_iVertLength_Y
	{0x0F12, 0x022C},   //70000CE6 AFIT8_yuviirnr_iVertLength_UV   [7:0] AFIT8_yuviirnr_iDiffThreshL_Y
	{0x0F12, 0x1402},   //70000CE8 AFIT8_yuviirnr_iDiffThreshH_Y   [7:0] AFIT8_yuviirnr_iDiffThreshL_UV
	{0x0F12, 0x0618},   //70000CEA AFIT8_yuviirnr_iDiffThreshH_UV   [7:0] AFIT8_yuviirnr_iMaxThreshL_Y
	{0x0F12, 0x1A02},   //70000CEC AFIT8_yuviirnr_iMaxThreshH_Y   [7:0] AFIT8_yuviirnr_iMaxThreshL_UV
	{0x0F12, 0x8018},   //70000CEE AFIT8_yuviirnr_iMaxThreshH_UV   [7:0] AFIT8_yuviirnr_iYNRStrengthL
	{0x0F12, 0x0080},   //70000CF0 AFIT8_yuviirnr_iYNRStrengthH   [7:0] AFIT8_yuviirnr_iUVNRStrengthL
	{0x0F12, 0x1080},   //70000CF2 AFIT8_yuviirnr_iUVNRStrengthH   [7:0] AFIT8_byr_gras_iShadingPower
	{0x0F12, 0x0180},   //70000CF4 AFIT8_RGBGamma2_iLinearity   [7:0] AFIT8_RGBGamma2_iDarkReduce
	{0x0F12, 0x0A0A},   //70000CF6 AFIT8_ccm_oscar_iSaturation   [7:0] AFIT8_RGB2YUV_iYOffset
	{0x0F12, 0x0101},   //70000CF8 AFIT8_RGB2YUV_iRGBGain   [7:0] AFIT8_bnr_nClustLevel_H
	{0x0F12, 0x0C0F},   //70000CFA AFIT8_bnr_iClustMulT_H   [7:0] AFIT8_bnr_iClustMulT_C
	{0x0F12, 0x6024},   //70000CFC AFIT8_bnr_iClustThresh_H   [7:0] AFIT8_bnr_iClustThresh_C
	{0x0F12, 0x0808},   //70000CFE AFIT8_bnr_iDenThreshLow   [7:0] AFIT8_bnr_iDenThreshHigh
	{0x0F12, 0xFFFF},   //70000D00 AFIT8_ee_iLowSharpPower   [7:0] AFIT8_ee_iHighSharpPower
	{0x0F12, 0x0808},   //70000D02 AFIT8_ee_iLowShDenoise   [7:0] AFIT8_ee_iHighShDenoise
	{0x0F12, 0x0A01},   //70000D04 AFIT8_ee_iLowSharpClamp   [7:0] AFIT8_ee_iHighSharpClamp
	{0x0F12, 0x010A},   //70000D06 AFIT8_ee_iReduceEdgeMinMult   [7:0] AFIT8_ee_iReduceEdgeSlope
	{0x0F12, 0x0F01},   //70000D08 AFIT8_bnr_nClustLevel_H_Bin   [7:0] AFIT8_bnr_iClustMulT_H_Bin
	{0x0F12, 0x240C},   //70000D0A AFIT8_bnr_iClustMulT_C_Bin   [7:0] AFIT8_bnr_iClustThresh_H_Bin
	{0x0F12, 0x0860},   //70000D0C AFIT8_bnr_iClustThresh_C_Bin   [7:0] AFIT8_bnr_iDenThreshLow_Bin
	{0x0F12, 0xFF08},   //70000D0E AFIT8_bnr_iDenThreshHigh_Bin   [7:0] AFIT8_ee_iLowSharpPower_Bin
	{0x0F12, 0x08FF},   //70000D10 AFIT8_ee_iHighSharpPower_Bin   [7:0] AFIT8_ee_iLowShDenoise_Bin
	{0x0F12, 0x0008},   //70000D12 AFIT8_ee_iHighShDenoise_Bin   [7:0] AFIT8_ee_iLowSharpClamp_Bin
	{0x0F12, 0x0001},   //70000D14 AFIT8_ee_iHighSharpClamp_Bin   [7:0] AFIT8_ee_iReduceEdgeMinMult_Bin
                      //70000D16 AFIT8_ee_iReduceEdgeSlope_Bin [7:0]
                      //70000D18 AFITB_bnr_nClustLevel_C      [0]   bWideWide[1]
	{0x002A, 0x060C},
	{0x0F12, 0x0940},

	{0x0028, 0xD000},
	{0x002A, 0xE410},
	{0x0F12, 0x3E01},
	{0x0028, 0x7000},
	{0x002A, 0x18AC},
	{0x0F12, 0x0060},
	{0x0f12, 0x0060},
	{0x0f12, 0x07DC},
	{0x0f12, 0x05C0},

	{0x002A, 0x02A6},
	{0x0F12, 0x0500},
	{0x0f12, 0x02D0},

	{0x002A, 0x0250},
	{0x0F12, 0x0A00},
	{0x0F12, 0x05A0},
	{0x0F12, 0x0010},
	{0x0F12, 0x00F0},
	{0x0F12, 0x0A00},
	{0x0F12, 0x05A0},
	{0x0F12, 0x0010},
	{0x0F12, 0x00F0},
	{0x002A, 0x0494},
	{0x0F12, 0x0A00},
	{0x0F12, 0x05A0},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},
	{0x0F12, 0x0A00},
	{0x0F12, 0x05A0},
	{0x0F12, 0x0000},
	{0x0F12, 0x0000},

	{0x002A, 0x0262},
	{0x0F12, 0x0001},
	{0x0F12, 0x0001},

	{0x002A, 0x0266},
	{0x0F12, 0x0000},
	{0x002A, 0x026A},
	{0x0F12, 0x0001},
	{0x002A, 0x024E},
	{0x0F12, 0x0001},
	{0x002A, 0x0268},
	{0x0F12, 0x0001},
	{0x002A, 0x0270},
	{0x0F12, 0x0001},
	{0x002A, 0x023E},
	{0x0F12, 0x0001},
	{0x0F12, 0x0001},
};

//1280X960  YUV   Mode
LOCAL const SENSOR_REG_T s5k4ec_1280X960[] =
{
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x0258},
	{0x0F12, 0x0A00},	//REG_TC_GP_CapReqInputWidth //2560
	{0x0F12, 0x0600},	//REG_TC_GP_CapReqInputHeight //1536
	{0x0F12, 0x0010},	//REG_TC_GP_CapInputWidthOfs //(2592-2560)/2
	{0x0F12, 0x00CC},	//REG_TC_GP_CapInputHeightOfs //(1944-1536)/2

	{0x002A, 0x0264},
	{0x0F12, 0x0001},	//REG_TC_GP_bUseReqInputInCap

	{0x002A, 0x049C},
	{0x0F12, 0x0A00},	//REG_TC_PZOOM_CapZoomReqInputWidth //2560
	{0x0F12, 0x0600},	//REG_TC_PZOOM_CapZoomReqInputHeight //1536
	{0x0F12, 0x0000},	//REG_TC_PZOOM_CapZoomReqInputWidthOfs
	{0x0F12, 0x0000},	//REG_TC_PZOOM_CapZoomReqInputHeightOfs

	{0x002A, 0x047C},
	{0x0F12, 0x0001},	//REG_TC_THUMB_Thumb_bActive
	{0x0F12, 0x0320},	//REG_TC_THUMB_Thumb_uWidth //800
	{0x0F12, 0x01E0},	//REG_TC_THUMB_Thumb_uHeight //480

	{0x002A, 0x0398},
	{0x0F12, 0x0640},	//REG_0TC_CCFG_usWidth //1600
	{0x0F12, 0x03C0},	//REG_0TC_CCFG_usHeight //960

	{0x002A, 0x024E},
	{0x0F12, 0x0001},	//REG_TC_GP_NewConfigSync
	{0x002A, 0x0270},
	{0x0F12, 0x0001},	//REG_TC_GP_CapConfigChanged
};

//1600X1200  YUV   Mode
LOCAL const SENSOR_REG_T s5k4ec_1600X1200[] =
{
// 1600 1200 capture
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x0258},
	{0x0F12, 0x0A00}, 	  /*REG_TC_GP_CapReqInputWidth 2560 */
	{0x0F12, 0x0780}, 	  /*REG_TC_GP_CapReqInputHeight 1920 */
	{0x0F12, 0x0010}, 	  /*REG_TC_GP_CapInputWidthOfs (2592-2560)/2 */
	{0x0F12, 0x000C}, 	  /*REG_TC_GP_CapInputHeightOfs (1944-1920)/2 */

	{0x002A, 0x0264},
	{0x0F12, 0x0001}, 	  /*REG_TC_GP_bUseReqInputInCap */

	{0x002A, 0x049C},
	{0x0F12, 0x0A00}, 	  /*REG_TC_PZOOM_CapZoomReqInputWidth 2560 */
	{0x0F12, 0x0780}, 	  /*REG_TC_PZOOM_CapZoomReqInputHeight 1920 */
	{0x0F12, 0x0000}, 	  /*REG_TC_PZOOM_CapZoomReqInputWidthOfs */
	{0x0F12, 0x0000}, 	  /*REG_TC_PZOOM_CapZoomReqInputHeightOfs */

	{0x002A, 0x047C},
	{0x0F12, 0x0001}, 	  /*REG_TC_THUMB_Thumb_bActive */
	{0x0F12, 0x0280}, 	  /*REG_TC_THUMB_Thumb_uWidth 640 */
	{0x0F12, 0x01E0}, 	  /*REG_TC_THUMB_Thumb_uHeight 480 */

	{0x002A, 0x0398},
	{0x0F12, 0x0640}, 	  /*REG_0TC_CCFG_usWidth 1600 */
	{0x0F12, 0x04B0}, 	  /*REG_0TC_CCFG_usHeight 1200 */

	{0x002A, 0x024E},
	{0x0F12, 0x0001}, 	  /*REG_TC_GP_NewConfigSync */
	{0x002A, 0x0270},
	{0x0F12, 0x0001}, 	  /*REG_TC_GP_CapConfigChanged */
};

//20480X1536  YUV   Mode
LOCAL const SENSOR_REG_T s5k4ec_2048X1536[] =
{
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x0258},
	{0x0F12, 0x0A00}, 	  /*REG_TC_GP_CapReqInputWidth 2560 */
	{0x0F12, 0x0780}, 	  /*REG_TC_GP_CapReqInputHeight 1920 */
	{0x0F12, 0x0010}, 	  /*REG_TC_GP_CapInputWidthOfs (2592-2560)/2 */
	{0x0F12, 0x000C}, 	  /*REG_TC_GP_CapInputHeightOfs (1944-1920)/2 */

	{0x002A, 0x0264},
	{0x0F12, 0x0001}, 	  /*REG_TC_GP_bUseReqInputInCap */

	{0x002A, 0x049C},
	{0x0F12, 0x0A00}, 	  /*REG_TC_PZOOM_CapZoomReqInputWidth 2560 */
	{0x0F12, 0x0780}, 	  /*REG_TC_PZOOM_CapZoomReqInputHeight 1920 */
	{0x0F12, 0x0000}, 	  /*REG_TC_PZOOM_CapZoomReqInputWidthOfs */
	{0x0F12, 0x0000}, 	  /*REG_TC_PZOOM_CapZoomReqInputHeightOfs */

	{0x002A, 0x047C},
	{0x0F12, 0x0001}, 	  /*REG_TC_THUMB_Thumb_bActive */
	{0x0F12, 0x0280}, 	  /*REG_TC_THUMB_Thumb_uWidth 640 */
	{0x0F12, 0x01E0}, 	  /*REG_TC_THUMB_Thumb_uHeight 480 */

	{0x002A, 0x0398},
	{0x0F12, 0x0800}, 	  /*REG_0TC_CCFG_usWidth 2048 */
	{0x0F12, 0x0600}, 	  /*REG_0TC_CCFG_usHeight 1536 */

	{0x002A, 0x024E},
	{0x0F12, 0x0001}, 	  /*REG_TC_GP_NewConfigSync */
	{0x002A, 0x0270},
	{0x0F12, 0x0001}, 	  /*REG_TC_GP_CapConfigChanged */
};
//2592X1944  YUV   Mode
LOCAL const SENSOR_REG_T s5k4ec_2592X1944[] =
{
	{0xFCFC, 0xD000},
	{0x0028, 0x7000},
	{0x002A, 0x0258},
	{0x0F12, 0x0A00},		/*REG_TC_GP_CapReqInputWidth 2560 */
	{0x0F12, 0x0780},		/*REG_TC_GP_CapReqInputHeight 1920 */
	{0x0F12, 0x0010},		/*REG_TC_GP_CapInputWidthOfs (2592-2560)/2 */
	{0x0F12, 0x000C},		/*REG_TC_GP_CapInputHeightOfs (1944-1920)/2 */

	{0x002A, 0x0264},
	{0x0F12, 0x0001},		/*REG_TC_GP_bUseReqInputInCap */

	{0x002A, 0x049C},
	{0x0F12, 0x0A00},		/*REG_TC_PZOOM_CapZoomReqInputWidth 2560 */
	{0x0F12, 0x0780},		/*REG_TC_PZOOM_CapZoomReqInputHeight 1920 */
	{0x0F12, 0x0000},		/*REG_TC_PZOOM_CapZoomReqInputWidthOfs */
	{0x0F12, 0x0000},		/*REG_TC_PZOOM_CapZoomReqInputHeightOfs */

	{0x002A, 0x047C},
	{0x0F12, 0x0001},		/*REG_TC_THUMB_Thumb_bActive */
	{0x0F12, 0x0280},		/*REG_TC_THUMB_Thumb_uWidth 640 */
	{0x0F12, 0x01E0},		/*REG_TC_THUMB_Thumb_uHeight 480 */

	{0x002A, 0x0398},
	{0x0F12, 0x0A00},		/*REG_0TC_CCFG_usWidth 2560 */
	{0x0F12, 0x0780},		/*REG_0TC_CCFG_usHeight 1920 */

	{0x002A, 0x024E},
	{0x0F12, 0x0001},		/*REG_TC_GP_NewConfigSync */
	{0x002A, 0x0270},
	{0x0F12, 0x0001},		/*REG_TC_GP_CapConfigChanged */
};

LOCAL SENSOR_REG_TAB_INFO_T s_s5k4ec_resolution_Tab_YUV[] = {
	//COMMON INIT
	{ADDR_AND_LEN_OF_ARRAY(s5k4ec_common_init), 0, 0, 24, SENSOR_IMAGE_FORMAT_YUV422},
	//YUV422 PREVIEW 1
	{ADDR_AND_LEN_OF_ARRAY(s5k4ec_640X480), 640, 480, 24, SENSOR_IMAGE_FORMAT_YUV422},
	{ADDR_AND_LEN_OF_ARRAY(s5k4ec_1280X720), 1280, 720, 24, SENSOR_IMAGE_FORMAT_YUV422},
	{ADDR_AND_LEN_OF_ARRAY(s5k4ec_2592X1944), 2560, 1920, 24,	 SENSOR_IMAGE_FORMAT_YUV422},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

LOCAL unsigned long s5k4ec_set_ae_enable(unsigned long enable)
{
	return 0;
}

LOCAL unsigned long s5k4ec_set_hmirror_enable(unsigned long enable)
{
	return 0;
}

LOCAL unsigned long s5k4ec_set_vmirror_enable(unsigned long enable)
{
	return 0;
}
LOCAL SENSOR_TRIM_T s_s5k4ec_Resolution_Trim_Tab[]=
{
	// COMMON INIT
	{0, 0, 640, 480, 0, 0, 0, {0, 0, 0, 0}},
	// YUV422 PREVIEW 1
	{0, 0, 640, 480, 680, 648, 40, {0, 0, 640, 480}},
	{0, 0, 1280, 720, 664, 648, 0, {0, 0, 1280, 720}},
	{0, 0, 2560, 1920, 660, 648, 0, {0, 0, 2560, 1920}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}}
};
LOCAL EXIF_SPEC_PIC_TAKING_COND_T s_s5k4ec_exif;
LOCAL SENSOR_IOCTL_FUNC_TAB_T s_s5k4ec_ioctl_func_tab =
{
	// Internal
	PNULL, /*0*/
	_s5k4ec_PowerOn,
	PNULL,/*2*/
	_s5k4ec_Identify,
	PNULL,/*4*/			// write register
	PNULL,/*5*/                 // read  register
	PNULL,//    cus_func_1//s5k4ec_init_by_burst_write,/*6*/
	_s5k4ec_GetResolutionTrimTab,//PNULL,/*7*/

	// External
	PNULL,/*8*///s5k4ec_set_ae_enable,
	PNULL,/*9*///s5k4ec_set_hmirror_enable,
	PNULL,/*10*///s5k4ec_set_vmirror_enable,
	PNULL,//_s5k4ec_set_brightness,//PNULL,//    s5k4ec_set_brightness,/*11*/
	_s5k4ec_set_contrast,//PNULL,//    s5k4ec_set_contrast,/*12*/
	PNULL,/*13*///s5k4ec_set_sharpness,
	PNULL,//_s5k4ec_set_saturation,//PNULL,/*14*///s5k4ec_set_saturation,
	PNULL,//_s5k4ec_set_work_mode,//PNULL,//    s5k4ec_set_scene_mode ,/*15*///s5k4ec_set_preview_mode,
	PNULL,//_s5k4ec_set_image_effect,//PNULL,//    s5k4ec_set_image_effect,/*16*/
	_s5k4ec_BeforeSnapshot,//PNULL,//    s5k4ec_BeforeSnapshot,/*17*/
	_s5k4ec_after_snapshot,//PNULL,//    s5k4ec_after_snapshot,/*18*/
	PNULL,//_s5k4ec_flash,//PNULL,/*19*/
	PNULL,/*20*///read_ae_value
	PNULL,/*21*///write_ae_value
	PNULL,/*22*///read_gain_value
	PNULL,/*23*///write_gain_value
	PNULL,/*24*///read_gain_scale
	PNULL,//    set_frame_rate,/*25*/
	PNULL,//    af_enable/*26*/
	PNULL,//    af_get_status,/*27*/
	PNULL,//_s5k4ec_set_awb,//PNULL,//    s5k4ec_set_awb,/*28*/
	PNULL,//   get_skip_frame,/*29*/
	PNULL,//_s5k4ec_set_iso,/*30*///iso
	PNULL,//_s5k4ec_set_ev,//PNULL,/*31*///exposure
	PNULL,//_s5k4ec_check_image_format_support,//PNULL,/*32*///check_image_format_support
	PNULL,/*33*///change_image_format)
	PNULL,/*34*/ //set_zoom
	PNULL,//_s5k4ec_GetExifInfo,//PNULL,/*35*/// set_focus
	_s5k4ec_ExtFunc,//PNULL,/*36*///get_exif
	PNULL,//_s5k4ec_set_anti_flicker,//PNULL,/*37*///set_anti_banding_flicker
	PNULL,//_s5k4ec_set_video_mode,//PNULL,/*38*/// set_video_mode
	PNULL,//_s5k4ec_pick_out_jpeg_stream,//PNULL,/*39*///pick_jpeg_stream
	PNULL,//    s5k4ec_set_Metering,/*40*///set_meter_mode
	PNULL, /*41*///get_status
	_s5k4ec_StreamOn, /*42*///stream_on
#ifdef CONFIG_CAMERA_SENSOR_NEW_FEATURE
	_s5k4ec_StreamOff, /*43*/ // stream_off
	NULL,
#else
	_s5k4ec_StreamOff,
	NULL,
#endif
};


/**---------------------------------------------------------------------------*
 ** 						Global Variables								  *
 **---------------------------------------------------------------------------*/
SENSOR_INFO_T g_s5k4ec_mipi_yuv_info =
{
	S5K4EC_I2C_ADDR_W,				// salve i2c write address
	S5K4EC_I2C_ADDR_R, 				// salve i2c read address
	SENSOR_I2C_VAL_16BIT|SENSOR_I2C_REG_16BIT|SENSOR_I2C_FREQ_400,//SENSOR_I2C_VAL_8BIT|SENSOR_I2C_REG_8BIT,			// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
									// bit1: 0: i2c register addr  is 8 bit, 1: i2c register addr  is 16 bit
									// other bit: reseved
	SENSOR_HW_SIGNAL_PCLK_N|\
	SENSOR_HW_SIGNAL_VSYNC_P|\
	SENSOR_HW_SIGNAL_HSYNC_P,		// bit0: 0:negative; 1:positive -> polarily of pixel clock
									// bit2: 0:negative; 1:positive -> polarily of horizontal synchronization signal
									// bit4: 0:negative; 1:positive -> polarily of vertical synchronization signal
									// other bit: reseved

	// preview mode
	SENSOR_ENVIROMENT_NORMAL|\
	SENSOR_ENVIROMENT_NIGHT,

	// image effect
	SENSOR_IMAGE_EFFECT_NORMAL|\
	SENSOR_IMAGE_EFFECT_BLACKWHITE|\
	SENSOR_IMAGE_EFFECT_RED|\
	SENSOR_IMAGE_EFFECT_GREEN|\
	SENSOR_IMAGE_EFFECT_BLUE|\
	SENSOR_IMAGE_EFFECT_YELLOW|\
	SENSOR_IMAGE_EFFECT_NEGATIVE|\
	SENSOR_IMAGE_EFFECT_CANVAS,

	// while balance mode
	0,

	0x7,
// bit[0:7]: count of step in brightness, contrast, sharpness, saturation
									// bit[8:31] reseved

	SENSOR_LOW_PULSE_RESET,//SENSOR_LOW_PULSE_RESET,		// reset pulse level
	50,								// reset pulse width(ms)

	SENSOR_LOW_LEVEL_PWDN,		// 1: high level valid; 0: low level valid

	1,								// count of identify code
	 {{0x01a4, 0x4ec0},                // supply two code to identify sensor.
	{0x01a6, 0x0011}},               // for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,			// voltage of avdd
	2560,							// max width of source image
	1920,							// max height of source image
	"s5k4ec",						// name of sensor

	SENSOR_IMAGE_FORMAT_MAX,        // define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
                                    // if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_YUV422_UYVY,	// pattern of input image form sensor;
	s_s5k4ec_resolution_Tab_YUV,	// point to resolution table information structure
	&s_s5k4ec_ioctl_func_tab,		// point to ioctl function table

	PNULL,							// information and table about Rawrgb sensor
	PNULL,				// extend information about sensor
	SENSOR_AVDD_1800MV,                     // iovdd
	SENSOR_AVDD_1200MV,                      // dvdd
	1,                     // skip frame num before preview
	1,                     // skip frame num before capture
	0,                     // deci frame num during preview;
	0,                     // deci frame num during video preview;

	0,                     // threshold enable
	0,                     // threshold mode
	0,                     // threshold start postion
	0,                     // threshold end postion
	0,                     // i2c_dev_handler
	{SENSOR_INTERFACE_TYPE_CSI2, 2, 8, 1},
	PNULL,
	1,                     // skip frame num while change setting
};

LOCAL unsigned long _s5k4ec_GetExifInfo(unsigned long param)
{
	return (unsigned long)&s_s5k4ec_exif;
}

LOCAL uint32_t _s5k4ec_InitExifInfo(void)
{
	EXIF_SPEC_PIC_TAKING_COND_T* exif_ptr=&s_s5k4ec_exif;

	memset(&s_s5k4ec_exif, 0, sizeof(EXIF_SPEC_PIC_TAKING_COND_T));

	SENSOR_PRINT("SENSOR: _s5k4ec_InitExifInfo \n");

	exif_ptr->valid.FNumber=1;
	exif_ptr->FNumber.numerator=14;
	exif_ptr->FNumber.denominator=5;

	exif_ptr->valid.ExposureProgram=1;
	exif_ptr->ExposureProgram=0x04;

	//exif_ptr->SpectralSensitivity[MAX_ASCII_STR_SIZE];
	//exif_ptr->ISOSpeedRatings;
	//exif_ptr->OECF;

	//exif_ptr->ShutterSpeedValue;

	exif_ptr->valid.ApertureValue=1;
	exif_ptr->ApertureValue.numerator=14;
	exif_ptr->ApertureValue.denominator=5;

	//exif_ptr->BrightnessValue;
	//exif_ptr->ExposureBiasValue;

	exif_ptr->valid.MaxApertureValue=1;
	exif_ptr->MaxApertureValue.numerator=14;
	exif_ptr->MaxApertureValue.denominator=5;

	//exif_ptr->SubjectDistance;
	//exif_ptr->MeteringMode;
	//exif_ptr->LightSource;
	//exif_ptr->Flash;

	exif_ptr->valid.FocalLength=1;
	exif_ptr->FocalLength.numerator=289;
	exif_ptr->FocalLength.denominator=100;

	//exif_ptr->SubjectArea;
	//exif_ptr->FlashEnergy;
	//exif_ptr->SpatialFrequencyResponse;
	//exif_ptr->FocalPlaneXResolution;
	//exif_ptr->FocalPlaneYResolution;
	//exif_ptr->FocalPlaneResolutionUnit;
	//exif_ptr->SubjectLocation[2];
	//exif_ptr->ExposureIndex;
	//exif_ptr->SensingMethod;

	exif_ptr->valid.FileSource=1;
	exif_ptr->FileSource=0x03;

	//exif_ptr->SceneType;
	//exif_ptr->CFAPattern;
	//exif_ptr->CustomRendered;

	exif_ptr->valid.ExposureMode=1;
	exif_ptr->ExposureMode=0x00;

	exif_ptr->valid.WhiteBalance=1;
	exif_ptr->WhiteBalance=0x00;

	//exif_ptr->DigitalZoomRatio;
	//exif_ptr->FocalLengthIn35mmFilm;
	//exif_ptr->SceneCaptureType;
	//exif_ptr->GainControl;
	//exif_ptr->Contrast;
	//exif_ptr->Saturation;
	//exif_ptr->Sharpness;
	//exif_ptr->DeviceSettingDescription;
	//exif_ptr->SubjectDistanceRange;

	return SENSOR_SUCCESS;
}

LOCAL unsigned long _s5k4ec_GetResolutionTrimTab(unsigned long param)
{
	return (unsigned long)s_s5k4ec_Resolution_Trim_Tab;
}

LOCAL unsigned long _s5k4ec_PowerOn(unsigned long power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_s5k4ec_mipi_yuv_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_s5k4ec_mipi_yuv_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_s5k4ec_mipi_yuv_info.iovdd_val;
	BOOLEAN power_down = g_s5k4ec_mipi_yuv_info.power_down_level;
	BOOLEAN reset_level = g_s5k4ec_mipi_yuv_info.reset_pulse_level;
	//uint32_t reset_width=g_ov5640_yuv_info.reset_pulse_width;

	SENSOR_PRINT("SENSOR_s5k4ec: _s5k4ec_PowerOn:E!!  (1:on, 0:off): %d \n", power_on);

 	if (SENSOR_TRUE == power_on) {
		Sensor_PowerDown(power_down);
		Sensor_SetResetLevel(reset_level);
		// Open power
		SENSOR_Sleep(10);
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		SENSOR_Sleep(20);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		SENSOR_Sleep(10);
		Sensor_PowerDown(!power_down);
		SENSOR_Sleep(15);
		// Reset sensor
		Sensor_Reset(reset_level);
		SENSOR_Sleep(12);
        } else {
		Sensor_SetResetLevel(reset_level);
		SENSOR_Sleep(10);

		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_PowerDown(power_down);
		//SENSOR_Sleep(10);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		SENSOR_Sleep(10);
        }

	SENSOR_PRINT("SENSOR: _s5k4ec_PowerOn (1:on, 0:off): %ld \n", power_on);
	return SENSOR_SUCCESS;
}

LOCAL unsigned long _s5k4ec_Identify(unsigned long param)
{
#define s5k4ec_PID_VALUE    0x4ec0
#define s5k4ec_PID_ADDR     0x01a4
#define s5k4ec_VER_VALUE    0x0011
#define s5k4ec_VER_ADDR     0x01a6
	uint16_t test=0x00;

	uint16_t pid_value=0x00;
	uint16_t ver_value=0x00;
	uint32_t ret_value=SENSOR_FAIL;
	#if 1
	SENSOR_PRINT("SENSOR:s5k4ec identify  -----20130610----.\n");

	Sensor_WriteReg(0xfcfc,0xd000);
	Sensor_WriteReg(0x002c, 0x7000);
	Sensor_WriteReg(0x002e, 0x01a4);
	pid_value = Sensor_ReadReg(0x0f12);

	SENSOR_PRINT("=====sonia SENSOR:s5k4ec identify  .pid_value=%x\n",pid_value);

	if (s5k4ec_PID_VALUE == pid_value) {
		Sensor_WriteReg(0x002c, 0x7000);
		Sensor_WriteReg(0x002e, s5k4ec_VER_ADDR);
		ver_value = Sensor_ReadReg(0x0f12);
		SENSOR_PRINT("SENSOR: s5k4ec_Identify: PID = %x, VER = %x \n",
			     pid_value, ver_value);
		if (s5k4ec_VER_VALUE == ver_value) {
			ret_value = SENSOR_SUCCESS;
			SENSOR_PRINT_HIGH("SENSOR: this is s5k4ec sensor ! \n");
		} else {
			SENSOR_PRINT_ERR("SENSOR: s5k4ec_Identify this is xx%x%x sensor ! \n",
			     pid_value, ver_value);
		}
	} else {
		SENSOR_PRINT_ERR("SENSOR:s5k4ec identify fail,pid_value=%d .\n",
			     pid_value);
	}

	#endif

	//_s5k4ec_InitExifInfo();  //

	return ret_value;
}

LOCAL const SENSOR_REG_T s5k4ec_brightness_tab[][4] = {
	{//level -3
		{0x0028, 0x7000},  {0x002A ,0x0230}, {0x0F12 ,0xFF81}, {0xffff, 0xffff}
	},
	{//level -2
		{0x0028, 0x7000}, {0x002A ,0x0230}, {0x0F12 ,0xFFAC}, {0xffff, 0xffff}
	},
	{//level -1
		{0x0028, 0x7000},  {0x002A ,0x0230}, {0x0F12 ,0xFFD5}, {0xffff, 0xffff}
	},
	{//level 0
		{0x0028, 0x7000},  {0x002A ,0x0230}, {0x0F12 ,0x0000}, {0xffff, 0xffff}
	},
	{//level 1
		{0x0028, 0x7000},  {0x002A ,0x0230}, {0x0F12 ,0x002B}, {0xffff, 0xffff}
	},
	{//level 2
		{0x0028, 0x7000},  {0x002A ,0x0230}, {0x0F12 ,0x0057}, {0xffff, 0xffff}
	},
	{//level 3
		{0x0028, 0x7000},  {0x002A ,0x0230}, {0x0F12 ,0x007F}, {0xffff, 0xffff}
	}
};
LOCAL unsigned long _s5k4ec_set_brightness(unsigned long level)
{
	uint16_t i=0x00;
	//uint32_t reg_bits = 0;
	//uint32_t reg_value = 0;
	SENSOR_REG_T_PTR sensor_reg_ptr =(SENSOR_REG_T_PTR) s5k4ec_brightness_tab[level];

	if (level > 7)
		return 0;

	for (i = 0x00;(0xffff != sensor_reg_ptr[i].reg_addr)|| (0xffff != sensor_reg_ptr[i].reg_value); i++)
	{
			Sensor_WriteReg(sensor_reg_ptr[i].reg_addr,sensor_reg_ptr[i].reg_value);
	}

	SENSOR_PRINT("SENSOR: _s5k4ec_set_brightness = 0x%02x.\n",level);
	return 0;
}

LOCAL const SENSOR_REG_T s5k4ec_contrast_tab[][4] = {
	{//level -3
		{0x0028, 0x7000}, {0x002A ,0x0232}, {0x0F12 ,0xFF9C}, {0xffff, 0xffff}
	},
	{//level -2
		{0x0028, 0x7000}, {0x002A ,0x0232}, {0x0F12 ,0xFFAC}, {0xffff, 0xffff}
	},
	{//level -1
		{0x0028, 0x7000}, {0x002A ,0x0232}, {0x0F12 ,0xFFD5}, {0xffff, 0xffff}
	},
	{//level 0
		{0x0028, 0x7000}, {0x002A ,0x0232}, {0x0F12 ,0x0000}, {0xffff, 0xffff}
	},
	{//level 1
		{0x0028, 0x7000}, {0x002A ,0x0232}, {0x0F12 ,0x002B}, {0xffff, 0xffff}
	},
	{//level 2
		{0x0028, 0x7000}, {0x002A ,0x0232}, {0x0F12 ,0x0057}, {0xffff, 0xffff}
	},
	{//level 3
		{0x0028, 0x7000}, {0x002A ,0x0232}, {0x0F12 ,0x0064}, {0xffff, 0xffff}
	}
};

LOCAL unsigned long _s5k4ec_set_contrast(unsigned long level)
{
#if 0
	uint16_t i=0x00;
	SENSOR_REG_T_PTR sensor_reg_ptr =(SENSOR_REG_T_PTR) s5k4ec_contrast_tab[level];

	if (level > 7)
		return 0;

	for (i = 0x00;(0xffff != sensor_reg_ptr[i].reg_addr)|| (0xffff != sensor_reg_ptr[i].reg_value); i++)
	{
			Sensor_WriteReg(sensor_reg_ptr[i].reg_addr,sensor_reg_ptr[i].reg_value);
	}
	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_CONTRAST, (uint32_t) level);
        usleep(100*1000);
#endif
	SENSOR_PRINT("SENSOR: _s5k4ec_set_contrast = 0x%lx.\n",level);
	return 0;
}
/******************************************************************************/
// Description: set contrast
// Global resource dependence:
// Author: Tim.zhu
// Note:
//
/******************************************************************************/
LOCAL const SENSOR_REG_T s5k4ec_iso_tab[][6] = {
    {/*ISO AUTO*/
         {0x0028, 0x7000}, {0x002A ,0x04D0}, {0x0F12 ,0x0000}, {0x0F12 ,0x0000}, {0x0F12 ,0x0001},{0xffff, 0xffff}
    },
    {/*ISO 100*/
         {0x0028, 0x7000}, {0x002A ,0x04D0}, {0x0F12 ,0x0001}, {0x0F12 ,0x0064}, {0x0F12 ,0x0001},{0xffff, 0xffff}
    },
    {/*ISO 200*/
         {0x0028, 0x7000}, {0x002A ,0x04D0}, {0x0F12 ,0x0001}, {0x0F12 ,0x00C8}, {0x0F12 ,0x0001},{0xffff, 0xffff}
    },
    {/*ISO 400*/
         {0x0028, 0x7000}, {0x002A ,0x04D0}, {0x0F12 ,0x0001}, {0x0F12 ,0x0190}, {0x0F12 ,0x0001},{0xffff, 0xffff}
    },
    {/*ISO 800*/
         {0x0028, 0x7000}, {0x002A ,0x04D0}, {0x0F12 ,0x0001}, {0x0F12 ,0x0320}, {0x0F12 ,0x0001},{0xffff, 0xffff}
    },
    {/*ISO 1600*/
         {0x0028, 0x7000}, {0x002A ,0x04D0}, {0x0F12 ,0x0001}, {0x0F12 ,0x0640}, {0x0F12 ,0x0001},{0xffff, 0xffff}
    }

};

LOCAL unsigned long _s5k4ec_set_iso(unsigned long level)
{
        uint16_t i=0x00;
        SENSOR_REG_T_PTR sensor_reg_ptr =(SENSOR_REG_T_PTR) s5k4ec_iso_tab[level];

        if (level > 5)
                return 0;

        for (i = 0x00;(0xffff != sensor_reg_ptr[i].reg_addr)|| (0xffff != sensor_reg_ptr[i].reg_value); i++)
        {
                        Sensor_WriteReg(sensor_reg_ptr[i].reg_addr,sensor_reg_ptr[i].reg_value);
        }
        //Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_ISO, (uint32_t) level);
        SENSOR_PRINT("SENSOR: _s5k4ec_set_iso = 0x%lx.\n",level);
        return 0;
}

LOCAL const SENSOR_REG_T s5k4ec_saturation_tab[][4] = {
	{//level -3
         {0x0028, 0x7000}, {0x002A ,0x0234}, {0x0F12 ,0xFF81}, {0xffff, 0xffff}
    },
    {//level -2
         {0x0028, 0x7000}, {0x002A ,0x0234}, {0x0F12 ,0xFFAC}, {0xffff, 0xffff}
    },
    {//level -1
         {0x0028, 0x7000}, {0x002A ,0x0234}, {0x0F12 ,0xFFD5}, {0xffff, 0xffff}
    },
    {//level 0
         {0x0028, 0x7000}, {0x002A ,0x0234}, {0x0F12 ,0x0010}, {0xffff, 0xffff}
    },
    {//level 1
         {0x0028, 0x7000}, {0x002A ,0x0234}, {0x0F12 ,0x002B}, {0xffff, 0xffff}
    },
    {//level 2
         {0x0028, 0x7000}, {0x002A ,0x0234}, {0x0F12 ,0x0057}, {0xffff, 0xffff}
    },
    {//level 3
         {0x0028, 0x7000}, {0x002A ,0x0234}, {0x0F12 ,0x007F}, {0xffff, 0xffff}
    }
};
LOCAL unsigned long _s5k4ec_set_saturation(unsigned long level)
{
	uint16_t i=0x00;

	SENSOR_REG_T_PTR sensor_reg_ptr =(SENSOR_REG_T_PTR) s5k4ec_saturation_tab[level];

	if(level>6)
		return 0;

	for (i = 0x00;(0xffff != sensor_reg_ptr[i].reg_addr)|| (0xffff != sensor_reg_ptr[i].reg_value); i++)
	{
			Sensor_WriteReg(sensor_reg_ptr[i].reg_addr,sensor_reg_ptr[i].reg_value);
	}
	SENSOR_PRINT("SENSOR: _s5k4ec_set_saturation = 0x%lx.\n",level);
	return 0;
}

/******************************************************************************/
// Description:
// Global resource dependence:
// Author: Tim.zhu
// Note:
//
/******************************************************************************/
LOCAL const SENSOR_REG_T s5k4ec_image_effect_tab[][15] = {
	// effect normal        --effect normal
	{
		{0x0028, 0x7000},
		{0x002A, 0x04E6},
		{0x0F12, 0x077f},
		{0x002A, 0x023C},
		{0x0F12, 0x0000},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff}
	},
    // effect monochrome    --effect BLACKWHITE
	{
		{0x0028, 0x7000},
		{0x002A, 0x04E6},
		{0x0F12, 0x077f},
		{0x002A, 0x023C},
		{0x0F12, 0x0001},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff}
    },
    // effect negative      --effect RED
	{
		{0x0028, 0x7000},
		{0x002A, 0x023C},
		{0x0F12, 0x0000},
		{0x002A, 0x04E6},
		{0x0F12, 0x0777},

		{0x002A, 0x04BA},
		{0x0F12, 0x0300},
		{0x0F12, 0x0001},
		{0x002A, 0x04BE},
		{0x0F12, 0x0100},
		{0x0F12, 0x0001},
		{0x002A, 0x04C2},
		{0x0F12, 0x0100},
		{0x0F12, 0x0001},
		{0xffff, 0xffff}
	},
    // effect sepia         --effect GREEN
	{
		{0x0028, 0x7000},
		{0x002A, 0x023C},
		{0x0F12, 0x0000},
		{0x002A, 0x04E6},
		{0x0F12, 0x0777},

	  	{0x002A, 0x04BA},
	  	{0x0F12, 0x0100},
	  	{0x0F12, 0x0001},
	  	{0x002A, 0x04BE},
	  	{0x0F12, 0x0200},
	  	{0x0F12, 0x0001},
	  	{0x002A, 0x04C2},
	  	{0x0F12, 0x0100},
	  	{0x0F12, 0x0001},
		{0xffff, 0xffff}
	},
	// effect  aqua         --effect  BLUE
	{
		{0x0028, 0x7000},
		{0x002A, 0x023C},
		{0x0F12, 0x0000},
		{0x002A, 0x04E6},
		{0x0F12, 0x0777},

		{0x002A, 0x04BA},
		{0x0F12, 0x0100},
		{0x0F12, 0x0001},
		{0x002A, 0x04BE},
		{0x0F12, 0x0100},
		{0x0F12, 0x0001},
		{0x002A, 0x04C2},
		{0x0F12, 0x0200},
		{0x0F12, 0x0001},
		{0xffff, 0xffff}
	},
	// effect  sketch       --effect  YELLOW
	{
		{0x0028, 0x7000},
		{0x002A, 0x023C},
		{0x0F12, 0x0000},
		{0x002A, 0x04E6},
		{0x0F12, 0x0777},

		{0x002A, 0x04BA},
		{0x0F12, 0x0200},
		{0x0F12, 0x0001},
		{0x002A, 0x04BE},
		{0x0F12, 0x0200},
		{0x0F12, 0x0001},
		{0x002A, 0x04C2},
		{0x0F12, 0x0100},
		{0x0F12, 0x0001},
		{0xffff, 0xffff}
	},
	// effect emboss color  --effect NEGATIVE
	{
		{0x0028, 0x7000},
		{0x002A, 0x04E6},
		{0x0F12, 0x077f},
		{0x002A, 0x023C},
		{0x0F12, 0x0002},

		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff}
	},
	//effect emboss mono    --effect ANTIQUE
	{
		{0x0028, 0x7000},
		{0x002A, 0x04E6},
		{0x0F12, 0x077f},
		{0x002A, 0x023C},
		{0x0F12, 0x0004},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff},
		{0xffff, 0xffff}
	},
};

LOCAL unsigned long _s5k4ec_set_image_effect(unsigned long effect_type)
{
	uint16_t i = 0x00;
	SENSOR_REG_T_PTR sensor_reg_ptr =(SENSOR_REG_T_PTR) s5k4ec_image_effect_tab[effect_type];

	if (effect_type > 7)
		return 0;

	for (i = 0x00;(0xffff != sensor_reg_ptr[i].reg_addr)|| (0xffff != sensor_reg_ptr[i].reg_value); i++)
	{
 			Sensor_WriteReg(sensor_reg_ptr[i].reg_addr,sensor_reg_ptr[i].reg_value);
	}

	SENSOR_PRINT("SENSOR: _s5k4ec_set_image_effect = 0x%lx\n",effect_type);
	return 0;
}

LOCAL const SENSOR_REG_T s5k4ec_ev_tab[][7] = {
	{//level -3 TVAR_ae_BrAve
		{0x0028, 0x7000},{0x002A, 0x1484},{0x0F12, 0x000C}, {0xffff, 0xffff}
	},
	{//level -2
		{0x0028, 0x7000},{0x002A, 0x1484},{0x0F12, 0x001E}, {0xffff, 0xffff}
	},
	{//level -1
		{0x0028, 0x7000},{0x002A, 0x1484},{0x0F12, 0x002E}, {0xffff, 0xffff}
	},
	{//level 0
		{0x0028, 0x7000},{0x002A, 0x1484},{0x0F12, 0x003C}, {0xffff, 0xffff}
	},
	{//level 1
		{0x0028, 0x7000},{0x002A, 0x1484},{0x0F12, 0x0054}, {0xffff, 0xffff}
	},
	{//level 2
		{0x0028, 0x7000},{0x002A, 0x1484},{0x0F12, 0x0066}, {0xffff, 0xffff}
	},
	{//level 3
		{0x0028, 0x7000},{0x002A, 0x1484},{0x0F12, 0x0078}, {0xffff, 0xffff}
	}
};

LOCAL unsigned long _s5k4ec_set_ev(unsigned long level)
{
	uint16_t i = 0x00;
	SENSOR_REG_T_PTR sensor_reg_ptr =(SENSOR_REG_T_PTR) s5k4ec_ev_tab[level];


	if (level > 6)
		return 0;

	for (i = 0x00;(0xffff != sensor_reg_ptr[i].reg_addr)|| (0xffff != sensor_reg_ptr[i].reg_value); i++)
	{
 			Sensor_WriteReg(sensor_reg_ptr[i].reg_addr,sensor_reg_ptr[i].reg_value);
	}

	SENSOR_PRINT("SENSOR: _s5k4ec_set_ev = 0x%lx \n", level);
	return 0;
}
/******************************************************************************/
// Description: anti 50/60 hz banding flicker
// Global resource dependence:
// Author: Tim.zhu
// Note:
//
/******************************************************************************/
LOCAL const SENSOR_REG_T s5k4ec_anti_banding_flicker_tab[][7] = {
	{//50hz
		{0x0028, 0x7000},
		{0x002a, 0x04e6},
		{0x0f12, 0x075f},
		{0x002a, 0x04d6},
		{0x0f12, 0x0001},
		{0x0f12, 0x0001},
		{0xffff, 0xffff}
	},
	{//60hz
		{0x0028, 0x7000},
		{0x002a, 0x04e6},
		{0x0f12, 0x075f},
		{0x002a, 0x04d6},
		{0x0f12, 0x0002},
		{0x0f12, 0x0001},
		{0xffff, 0xffff}
	}
};

LOCAL unsigned long _s5k4ec_set_anti_flicker(unsigned long mode)
{
	uint16_t i = 0x00;
	SENSOR_REG_T_PTR sensor_reg_ptr =(SENSOR_REG_T_PTR) s5k4ec_anti_banding_flicker_tab[mode];

	if (mode > 1)
		return 0;

	for (i = 0x00;(0xffff != sensor_reg_ptr[i].reg_addr)|| (0xffff != sensor_reg_ptr[i].reg_value); i++)
	{
 			Sensor_WriteReg(sensor_reg_ptr[i].reg_addr,sensor_reg_ptr[i].reg_value);
	}

	SENSOR_PRINT("SENSOR: _s5k4ec_set_anti_flicker = 0x%lx \n", mode);

	return 0;
}

LOCAL const SENSOR_REG_T s5k4ec_video_mode_tab[][40]=
{
	/* preview mode: 30 fps*/
	{
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	////////////////////////////////////////////////////
	// Preview Config 0     640*480
	////////////////////////////////////////////////////
	{0x002A, 0x02A6},
	{0x0F12, 0x0280},  //#REG_0TC_PCFG_usWidth
	{0x0F12, 0x01e0},  //#REG_0TC_PCFG_usHeight
	{0x0F12, 0x0005},  //#REG_0TC_PCFG_Format
	{0x0F12, 0x2EE0},  //#REG_0TC_PCFG_usMaxOut4KHzRate
	{0x0F12, 0x2EE0},  //#REG_0TC_PCFG_usMinOut4KHzRate
	{0x0F12, 0x0100},  //#REG_0TC_PCFG_OutClkPerPix88
	{0x0F12, 0x0300},  //#REG_0TC_PCFG_uBpp88
	{0x0F12, 0x0042},  //#REG_0TC_PCFG_PVIMask
	{0x0F12, 0x0800},  //#REG_0TC_PCFG_OIFMask
	{0x0F12, 0x01E0},  //#REG_0TC_PCFG_usJpegPacketSize
	{0x0F12, 0x0000},  //#REG_0TC_PCFG_usJpegTotalPackets
	{0x0F12, 0x0000},  //#REG_0TC_PCFG_uClockInd
	{0x0F12, 0x0000},  //#REG_0TC_PCFG_usFrTimeType
	{0x0F12, 0x0001},  //#REG_0TC_PCFG_FrRateQualityType
	{0x0F12, 0x03e8},  //#REG_0TC_PCFG_usMaxFrTimeMsecMult10
	{0x0F12, 0x0000},  //#REG_0TC_PCFG_usMinFrTimeMsecMult10
	{0x002A, 0x02D0},
	{0x0F12, 0x0000},  //#REG_0TC_PCFG_uPrevMirror
	{0x0F12, 0x0000},  //#REG_0TC_PCFG_uCaptureMirror
	{0x0F12, 0x0000},  //#REG_0TC_PCFG_uRotation

	{0x002A, 0x0266},
	{0x0F12, 0x0000},  //#REG_TC_GP_ActivePrevConfig
	{0x002A, 0x026A},
	{0x0F12, 0x0001},  //#REG_TC_GP_PrevOpenAfterChange
	{0x002A, 0x026E},
	{0x0F12, 0x0000},	 //REG_TC_GP_ActiveCapConfig
	{0x002A, 0x024E},
	{0x0F12, 0x0001},  //#REG_TC_GP_NewConfigSync
	{0x002A, 0x0268},
	{0x0F12, 0x0001},  //#REG_TC_GP_PrevConfigChanged
	{0x002A, 0x0270},
	{0x0F12, 0x0001},  //#REG_TC_GP_CapConfigChanged
	{0x002A, 0x023E},
	{0x0F12, 0x0001},  //#REG_TC_GP_EnablePreview
	{0x0F12, 0x0001},  //#REG_TC_GP_EnablePreviewChanged
	{0xffff, 0xffff},
	{0xffff, 0xffff}
	},
	/* video mode: if use 35 fps, change it to 0x50*/
	{
	{0xfcfc, 0xd000},
	{0x0028, 0x7000},
	////////////////////////////////////////////////////
	// Preview Config 0     640*480
	////////////////////////////////////////////////////
	{0x002A, 0x02A6},
	{0x0F12, 0x0280},  //#REG_0TC_PCFG_usWidth
	{0x0F12, 0x01e0},  //#REG_0TC_PCFG_usHeight
	{0x0F12, 0x0005},  //#REG_0TC_PCFG_Format
	{0x0F12, 0x2EE0},  //#REG_0TC_PCFG_usMaxOut4KHzRate
	{0x0F12, 0x2EE0},  //#REG_0TC_PCFG_usMinOut4KHzRate
	{0x0F12, 0x0100},  //#REG_0TC_PCFG_OutClkPerPix88
	{0x0F12, 0x0300},  //#REG_0TC_PCFG_uBpp88
	{0x0F12, 0x0042},  //#REG_0TC_PCFG_PVIMask
	{0x0F12, 0x0800},  //#REG_0TC_PCFG_OIFMask
	{0x0F12, 0x01E0},  //#REG_0TC_PCFG_usJpegPacketSize
	{0x0F12, 0x0000},  //#REG_0TC_PCFG_usJpegTotalPackets
	{0x0F12, 0x0000},  //#REG_0TC_PCFG_uClockInd
	{0x0F12, 0x0002},  //#REG_0TC_PCFG_usFrTimeType
	{0x0F12, 0x0001},  //#REG_0TC_PCFG_FrRateQualityType
	{0x0F12, 0x029a},  //#REG_0TC_PCFG_usMaxFrTimeMsecMult10
	{0x0F12, 0x0000},  //#REG_0TC_PCFG_usMinFrTimeMsecMult10
	{0x002A, 0x02D0},
	{0x0F12, 0x0000},  //#REG_0TC_PCFG_uPrevMirror
	{0x0F12, 0x0000},  //#REG_0TC_PCFG_uCaptureMirror
	{0x0F12, 0x0000},  //#REG_0TC_PCFG_uRotation

	{0x002A, 0x0266},
	{0x0F12, 0x0000},  //#REG_TC_GP_ActivePrevConfig
	{0x002A, 0x026A},
	{0x0F12, 0x0001},  //#REG_TC_GP_PrevOpenAfterChange
	{0x002A, 0x026E},
	{0x0F12, 0x0000},	 //REG_TC_GP_ActiveCapConfig
	{0x002A, 0x024E},
	{0x0F12, 0x0001},  //#REG_TC_GP_NewConfigSync
	{0x002A, 0x0268},
	{0x0F12, 0x0001},  //#REG_TC_GP_PrevConfigChanged
	{0x002A, 0x0270},
	{0x0F12, 0x0001},  //#REG_TC_GP_CapConfigChanged
	{0x002A, 0x023E},
	{0x0F12, 0x0001},  //#REG_TC_GP_EnablePreview
	{0x0F12, 0x0001},  //#REG_TC_GP_EnablePreviewChanged
	{0xffff, 0xffff},
	{0xffff, 0xffff}
	}
};

LOCAL unsigned long _s5k4ec_set_video_mode(unsigned long mode)
{
	SENSOR_REG_T_PTR sensor_reg_ptr=(SENSOR_REG_T_PTR)s5k4ec_video_mode_tab[mode];
	uint16_t i=0x00;
        SENSOR_PRINT("SENSOR: _s5k4ec_set_video_mode,in = 0x%02x \n", mode);
	if(mode>1)
		return 0;

	for (i = 0x00;(0xffff != sensor_reg_ptr[i].reg_addr)|| (0xffff != sensor_reg_ptr[i].reg_value); i++)
	{
 			Sensor_WriteReg(sensor_reg_ptr[i].reg_addr,sensor_reg_ptr[i].reg_value);
	}

	SENSOR_PRINT("SENSOR: _s5k4ec_set_video_mode = 0x%lx \n", mode);
	return 0;
}

LOCAL const SENSOR_REG_T s5k4ec_awb_tab[][13] = {
	//AUTO
	{
		{0x0028, 0x7000},
		{0x002A ,0x04E6}, {0x0F12 ,0x077F}, {0xffff, 0xffff},
		{0xffff, 0xffff}, {0xffff, 0xffff}, {0xffff, 0xffff},
		{0xffff, 0xffff}, {0xffff, 0xffff}, {0xffff, 0xffff},
		{0xffff, 0xffff}, {0xffff, 0xffff}, {0xffff, 0xffff},
	},
	//INCANDESCENCE:
	{
		{0x0028, 0x7000},
		{0x002A ,0x04E6}, {0x0F12 ,0x0777}, {0x002A ,0x04BA},
		{0x0F12 ,0x066e}, {0x0F12 ,0x0001}, {0x002A ,0x04Be},
		{0x0F12 ,0x0400}, {0x0F12 ,0x0001}, {0x002A ,0x04c2},
		{0x0F12 ,0x0476}, {0x0F12 ,0x0001}, {0xffff, 0xffff}
	},
	//CWF equal to flourescent(auto),1
	{
		{0x0028, 0x7000},
		{0x002A ,0x04E6}, {0x0F12 ,0x0777}, {0x002A ,0x04BA},
		{0x0F12 ,0x0575}, {0x0F12 ,0x0001}, {0x002A ,0x04Be},
		{0x0F12 ,0x0400}, {0x0F12 ,0x0001}, {0x002A ,0x04c2},
		{0x0F12 ,0x0800}, {0x0F12 ,0x0001}, {0xffff, 0xffff}
	},
	//U30(auto),2
	{
		{0x0028, 0x7000},
		{0x002A ,0x04E6}, {0x0F12 ,0x0777}, {0x002A ,0x04BA},
		{0x0F12 ,0x0800}, {0x0F12 ,0x0001}, {0x002A ,0x04Be},
		{0x0F12 ,0x0400}, {0x0F12 ,0x0001}, {0x002A ,0x04c2},
		{0x0F12 ,0x0800}, {0x0F12 ,0x0001}, {0xffff, 0xffff}
	},
	//CWF equal to flourescent(auto),3
	{
		{0x0028, 0x7000},
		{0x002A ,0x04E6}, {0x0F12 ,0x0777}, {0x002A ,0x04BA},
		{0x0F12 ,0x0575}, {0x0F12 ,0x0001}, {0x002A ,0x04Be},
		{0x0F12 ,0x0400}, {0x0F12 ,0x0001}, {0x002A ,0x04c2},
		{0x0F12 ,0x0800}, {0x0F12 ,0x0001}, {0xffff, 0xffff}
	},
	//FLUORESCENT:4
	{
		{0x0028, 0x7000},
		{0x002A ,0x04E6}, {0x0F12 ,0x0777}, {0x002A ,0x04BA},
		{0x0F12 ,0x0400}, {0x0F12 ,0x0001}, {0x002A ,0x04Be},
		{0x0F12 ,0x0400}, {0x0F12 ,0x0001}, {0x002A ,0x04c2},
		{0x0F12 ,0x0940}, {0x0F12 ,0x0001}, {0xffff, 0xffff}
	},
	//SUN:5
	{
		{0x0028, 0x7000},
		{0x002A ,0x04E6}, {0x0F12 ,0x0777}, {0x002A ,0x04BA},
		{0x0F12 ,0x05e0}, {0x0F12 ,0x0001}, {0x002A ,0x04Be},
		{0x0F12 ,0x0400}, {0x0F12 ,0x0001}, {0x002A ,0x04c2},
		{0x0F12 ,0x0530}, {0x0F12 ,0x0001}, {0xffff, 0xffff}
	},
	//CLOUD:6
	{
		{0x0028, 0x7000},
		{0x002A ,0x04E6}, {0x0F12 ,0x0777}, {0x002A ,0x04BA},
		{0x0F12 ,0x0740}, {0x0F12 ,0x0001}, {0x002A ,0x04Be},
		{0x0F12 ,0x0400}, {0x0F12 ,0x0001}, {0x002A ,0x04c2},
		{0x0F12 ,0x0460}, {0x0F12 ,0x0001}, {0xffff, 0xffff}
	}
};

LOCAL unsigned long _s5k4ec_set_awb(unsigned long mode)
{
	uint16_t i=0x00;
#if 0   //sunao 20130608
	SENSOR_REG_T_PTR sensor_reg_ptr =(SENSOR_REG_T_PTR) s5k4ec_awb_tab[mode];

	if (mode > 6)
		return 0;

	for (i = 0x00;(0xffff != sensor_reg_ptr[i].reg_addr)|| (0xffff != sensor_reg_ptr[i].reg_value); i++)
	{
 			Sensor_WriteReg(sensor_reg_ptr[i].reg_addr,sensor_reg_ptr[i].reg_value);
	}


	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_LIGHTSOURCE, (uint32_t) mode);
	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_WHITEBALANCE, (uint32_t) mode);
#endif
	SENSOR_PRINT("SENSOR: _s5k4ec_set_awb = 0x%lx\n", mode);
	return 0;
}

LOCAL const SENSOR_REG_T s5k4ec_work_mode_tab[][16] = {
	/* Normal Mode,0 */
	{    //normal fix 25fps
		{0x0028, 0x7000},
		{0x002A, 0x0288},
		{0x0F12, 0x03E8},
		{0x0F12, 0x029A},
		{0x002A, 0x023c},
		{0x0F12, 0x0000},
		{0x002A, 0x0240},
		{0x0F12, 0x0001},
		{0x002A, 0x0230},
		{0x0F12, 0x0001},
		{0x002A, 0x023E},
		{0x0F12, 0x0001},
		{0x002A, 0x0220},
		{0x0F12, 0x0001},
		{0x0F12, 0x0001},
		{0xffff, 0xffff}
	},
	{    //night mode,1; min 12.5-25fps
		{0x0028, 0x7000},
		{0x002A, 0x0288},
		{0x0F12, 0x0535},
		{0x0F12, 0x0535},
		{0x002A, 0x023c},
		{0x0F12, 0x0000},
		{0x002A, 0x0240},
		{0x0F12, 0x0001},
		{0x002A, 0x0230},
		{0x0F12, 0x0001},
		{0x002A, 0x023E},
		{0x0F12, 0x0001},
		{0x002A, 0x0220},
		{0x0F12, 0x0001},
		{0x0F12, 0x0001},
		{0xffff, 0xffff}
	},
	//sports mode,2
	{
		{0xffff, 0xffff}
	},
	//portrait mode,3
	{
		{0xffff, 0xffff}
	},
	//landscape mode,4
	{
		{0xffff, 0xffff}
	},
	//normal mode,5
	{
		{0xffff, 0xffff}
	}
};

LOCAL unsigned long _s5k4ec_set_work_mode(unsigned long mode)
{
	uint16_t i = 0x00;
	SENSOR_REG_T_PTR sensor_reg_ptr =
	(SENSOR_REG_T_PTR) s5k4ec_work_mode_tab[mode];

	if (mode > 5)
		return 0;

	for (i = 0x00;(0xffff != sensor_reg_ptr[i].reg_addr)|| (0xffff != sensor_reg_ptr[i].reg_value); i++)
	{
 			Sensor_WriteReg(sensor_reg_ptr[i].reg_addr,sensor_reg_ptr[i].reg_value);
	}

	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_SCENECAPTURETYPE,(uint32_t) mode);
	SENSOR_PRINT("SENSOR: _s5k4ec_set_work_mode: mode = %ld \n", mode);
	return 0;
}

LOCAL uint16_t s_current_shutter = 0;
LOCAL uint16_t s_current_gain = 0;
LOCAL unsigned long _s5k4ec_BeforeSnapshot(unsigned long param)
{
	uint32_t capture_mode = param & 0xffff;

	SENSOR_PRINT("SENSOR: _s5k4ec_BeforeSnapshot = %d \n", capture_mode);
	if(1 != capture_mode){
        is_cap = 1;
    }
	Sensor_SetMode(capture_mode);
	usleep(10*1000);

    SENSOR_PRINT("SENSOR: s_current_shutter,s_current_gain = %x,%x \n", s_current_shutter,s_current_gain);
    return SENSOR_SUCCESS;
}

LOCAL unsigned long _s5k4ec_check_image_format_support(unsigned long param)
{
	uint32_t ret_val = SENSOR_FAIL;
	SENSOR_PRINT("SENSOR: _s5k4ec_check_image_format_support \n");
	switch (param) {
	case SENSOR_IMAGE_FORMAT_YUV422:
		ret_val = SENSOR_SUCCESS;
		break;
	case SENSOR_IMAGE_FORMAT_JPEG:
		ret_val = SENSOR_SUCCESS;
		break;
	default:
		break;
	}
	return ret_val;
}

LOCAL unsigned long _s5k4ec_pick_out_jpeg_stream(unsigned long param)
{
#if 0
	uint8_t *p_frame =
	    ((DCAMERA_SNAPSHOT_RETURN_PARAM_T *) param)->return_data_addr;
	uint32_t buf_len =
	    ((DCAMERA_SNAPSHOT_RETURN_PARAM_T *) param)->return_data_len;
	uint32_t i = 0x00;

	SENSOR_PRINT("SENSOR: s5k4ec jpeg capture head: 0x%x, 0x%x \n",
		     *((uint8 *) p_frame), *((uint8 *) p_frame + 1));

	/* Find the tail position */
	for (i = 0x00; i < buf_len; i++)
	{
		#define TAIL_VAL 0xffd9
		uint8_t* p_cur_val = (uint8*)p_frame;

		uint16_t tail_val = ((p_cur_val[i]<<8) | p_cur_val[i+1]);

		if (TAIL_VAL == tail_val)
		{
			i += 2;
			break;
		}
	}

	/* check if the tail is found */
	if (i < buf_len)
	{
		SENSOR_PRINT("SENSOR: s5k4ec Found the jpeg tail at %d: 0x%x 0x%x \n",
		     i + 1, *((uint8 *) p_frame + i),*((uint8 *) p_frame + i + 1));
	}
	else
	{
		SENSOR_PRINT("SENSOR: s5k4ec can not find the jpeg tail: %d \n",i);
		i = 0x00;
	}

	return i;
#endif
        SENSOR_PRINT("SENSOR: _s5k4ec_pick_out_jpeg_stream \n");
	return 0;
}

#if 0
LOCAL uint32_t _s5k4ec_chang_image_format(uint32_t param)
{
	uint32_t ret_val = SENSOR_FAIL;
	SENSOR_REG_TAB_INFO_T st_yuv422_reg_table_info ={ ADDR_AND_LEN_OF_ARRAY(s5k4ec_640X480), 0, 0, 0, 0 };

	switch (param) {
	case SENSOR_IMAGE_FORMAT_YUV422:
		SENSOR_PRINT("SENSOR: s5k4ec  chang_image_format  YUV422 \n");
		ret_val = Sensor_SendRegTabToSensor(&st_yuv422_reg_table_info);
		break;

		case SENSOR_IMAGE_FORMAT_JPEG:
			SENSOR_PRINT("SENSOR: s5k4ec  chang_image_format  jpg \n");
			ret_val = SENSOR_FAIL;//Sensor_SendRegTabToSensor(&st_jpeg_reg_table_info);
			break;

		default:
			break;
	}

	return ret_val;
}
#endif

LOCAL unsigned long _s5k4ec_after_snapshot(unsigned long param)
{
	SENSOR_PRINT("=========sonia SENSOR: _s5k4ec_after_snapshot =%ld \n",param);
	uint32_t with,height=0;

	//_s5k4ec_recovery_init();
	Sensor_SetMode((uint32_t)param);
	//SENSOR_Sleep(10);
	is_cap = 0;

	//Sensor_WriteReg(0x002c, 0x7000);
	//Sensor_WriteReg(0x002e, 0x1d02);
	//with   = Sensor_ReadReg(0x0f12);
	//height = Sensor_ReadReg(0x0f12);
	//SENSOR_PRINT("SENSOR: _s5k4ec_after_snapshot:with=%x, height=%x.\n",with,height);

	return SENSOR_SUCCESS;
}

LOCAL unsigned long _s5k4ec_flash(unsigned long param)
{
	SENSOR_PRINT("SENSOR: _s5k4ec_flash:param=%ld .\n",param);

	/* enable flash, disable in _s5k4ec_BeforeSnapshot */
	g_flash_mode_en = param;

	Sensor_SetFlash(param);

	SENSOR_PRINT("SENSOR: _s5k4ec_flash:end .\n");

	return SENSOR_SUCCESS;
}

LOCAL uint32_t _s5k4ec_MatchZone(SENSOR_EXT_FUN_T_PTR param_ptr)
{
	SENSOR_RECT_T zone_rect;
	uint32_t rtn=SENSOR_SUCCESS;
    memset((void*)&zone_rect, 0, sizeof(SENSOR_RECT_T));
	switch (param_ptr->cmd)
	{
	case SENSOR_EXT_FOCUS_START:
			switch (param_ptr->param)
			{
			case SENSOR_EXT_FOCUS_ZONE:
			case SENSOR_EXT_FOCUS_MULTI_ZONE:
					zone_rect.w = FOCUS_ZONE_W;
					zone_rect.h = FOCUS_ZONE_H;
					break;
			default:
					break;
			}
			break;
	case SENSOR_EXT_EXPOSURE_START:
			switch (param_ptr->param)
			{
			case SENSOR_EXT_EXPOSURE_ZONE:
					zone_rect.w = EXPOSURE_ZONE_W;
					zone_rect.h = EXPOSURE_ZONE_H;
					break;
			default:
					break;
			}
			break;
	 default:
		  break;
	}

	if ((0x00 != s_s5k4ec_resolution_Tab_YUV[SENSOR_MODE_PREVIEW_ONE].width)&& (0x00 !=s_s5k4ec_resolution_Tab_YUV[SENSOR_MODE_PREVIEW_ONE].height)
	    && (0x00 != zone_rect.w)&& (0x00 != zone_rect.h))
	{
		param_ptr->zone.x =(zone_rect.w * param_ptr->zone.x) /
		    s_s5k4ec_resolution_Tab_YUV[SENSOR_MODE_PREVIEW_ONE].width;
		param_ptr->zone.y =(zone_rect.h * param_ptr->zone.y) /
		    s_s5k4ec_resolution_Tab_YUV[SENSOR_MODE_PREVIEW_ONE].height;
	}
	else
	{
		SENSOR_PRINT_ERR("SENSOR: _s5k4ec_MatchZone, w:%d, h:%d error \n",zone_rect.w, zone_rect.h);
		rtn = SENSOR_FAIL;
	}

	SENSOR_PRINT_HIGH("SENSOR: _s5k4ec_MatchZone, x:%d, y:%d \n",param_ptr->zone.x, param_ptr->zone.y);
	return rtn;
}

LOCAL uint32_t _s5k4ec_AutoFocusTrig(SENSOR_EXT_FUN_PARAM_T_PTR param_ptr)
{
	uint32_t rtn=SENSOR_SUCCESS;
	uint16_t i=30;
	uint16_t reg_value=0x00;
	SENSOR_PRINT("Start");
	Sensor_WriteReg(0xFCFC, 0xD000);
	Sensor_WriteReg(0x0028, 0x7000);
	Sensor_WriteReg(0x002A, 0x028E);
	Sensor_WriteReg(0x0F12, 0x001a);
	Sensor_WriteReg(0x002A, 0x028C);
	Sensor_WriteReg(0x0F12, 0x0005);
	do {
		usleep(100*1000);
		Sensor_WriteReg(0x002C, 0x7000);
		Sensor_WriteReg(0x002E, 0x2EEE);
		reg_value = Sensor_ReadReg(0x0F12);
		i--;
		if ((0x00 == i) || (0x0 == reg_value)) {
			SENSOR_PRINT("error!");
			rtn = SENSOR_FAIL;
			break;
		}
	} while (0x2 != reg_value);
	return rtn;
}

LOCAL uint32_t _s5k4ec_AutoFocusZone(SENSOR_EXT_FUN_PARAM_T_PTR param_ptr)
{
	uint32_t i=30; // 30 * 100 = 3 seconds
	uint16_t reg_value=0x00;
	uint32_t rtn=SENSOR_SUCCESS;
	uint32_t j=0;

	return rtn;
}

LOCAL uint32_t _s5k4ec_AutoFocusMultiZone(SENSOR_EXT_FUN_PARAM_T_PTR param_ptr)
{
	uint32_t rtn=SENSOR_SUCCESS;
	SENSOR_EXT_FUN_T ext_param[5];
	uint32_t i=100;
	uint16_t reg_value=0x00;
	uint32_t zone_cnt = 0;
	uint32_t zone_num = 0x90;

	return rtn;
}

LOCAL uint32_t _s5k4ec_AutoFocusMacro(SENSOR_EXT_FUN_PARAM_T_PTR param_ptr)
{
	uint32_t rtn=SENSOR_SUCCESS;
	uint16_t i=30;
	uint16_t reg_value=0x00;

	return rtn;
}

LOCAL unsigned long _s5k4ec_StartAutoFocus(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;
	SENSOR_PRINT("SENSOR: _s5k4ec_StartAutoFocus param =%d", ext_ptr->param);

	switch (ext_ptr->param) {
	case SENSOR_EXT_FOCUS_TRIG:
		rtn = _s5k4ec_AutoFocusTrig(ext_ptr);
		break;
	case SENSOR_EXT_FOCUS_ZONE:
		rtn = _s5k4ec_AutoFocusZone(ext_ptr);
		break;
	case SENSOR_EXT_FOCUS_MULTI_ZONE:
		rtn = _s5k4ec_AutoFocusMultiZone(ext_ptr);
		break;
	case SENSOR_EXT_FOCUS_MACRO:
		rtn = _s5k4ec_AutoFocusMacro(ext_ptr);
		break;
	default:
		break;
	}
	return rtn;
}

LOCAL uint32_t _s5k4ec_ExposureAuto(void)
{
	uint32_t rtn=SENSOR_SUCCESS;

	SENSOR_PRINT("SENSOR: _s5k4ec_ExposureAuto \n");

	//Sensor_WriteReg(0x501d, 0x00);

	return rtn;
}

LOCAL uint32_t _s5k4ec_ExposureZone(SENSOR_EXT_FUN_T_PTR param_ptr)
{
	uint32_t rtn=SENSOR_SUCCESS;

#if 0 //exposure zone \B2\BB\B8Ķ\AF

	SENSOR_EXT_FUN_T_PTR ext_ptr=(SENSOR_EXT_FUN_T_PTR)param_ptr;
	uint16_t reg_value=0x00;

	SENSOR_PRINT("SENSOR: _s5k4ec_ExposureZone: %d, %d \n",ext_ptr->zone.x,ext_ptr->zone.y);

	rtn=_s5k4ec_MatchZone(ext_ptr);

	if(SENSOR_SUCCESS==rtn)
	{
		// h zone
		if(0x00<(ext_ptr->zone.x-(EXPOSURE_ZONE_W/0x08)))
		{
			ext_ptr->zone.x-=(EXPOSURE_ZONE_W/0x08);
		}
		else
		{
			ext_ptr->zone.x=0x00;
		}
		ext_ptr->zone.w=EXPOSURE_ZONE_W/0x04;
		if(EXPOSURE_ZONE_W<(ext_ptr->zone.x+ext_ptr->zone.w))
		{
			ext_ptr->zone.x=EXPOSURE_ZONE_W-(EXPOSURE_ZONE_W/0x04);
		}

		// v zone
		if(0x00<(ext_ptr->zone.y-(EXPOSURE_ZONE_H/0x08)))
		{
			ext_ptr->zone.y-=(EXPOSURE_ZONE_H/0x08);
		}
		else
		{
			ext_ptr->zone.y=0x00;
		}
		ext_ptr->zone.h=EXPOSURE_ZONE_H/0x04;
		if(EXPOSURE_ZONE_H<(ext_ptr->zone.y+ext_ptr->zone.h))
		{
			ext_ptr->zone.y=EXPOSURE_ZONE_H-(EXPOSURE_ZONE_H/0x04);
		}

		SENSOR_PRINT("SENSOR: _s5k4ec_ExposureZone: %d, %d, %d, %d \n",
			     ext_ptr->zone.x, ext_ptr->zone.y, ext_ptr->zone.w,
			     ext_ptr->zone.h);

		reg_value=((ext_ptr->zone.x>>0x08)&0x07);
		Sensor_WriteReg(0x5680, reg_value);
		reg_value=(ext_ptr->zone.x&0xff);
		Sensor_WriteReg(0x5681, reg_value);

		reg_value=((ext_ptr->zone.y>>0x08)&0x07);
		Sensor_WriteReg(0x5682, reg_value);
		reg_value=(ext_ptr->zone.y&0xff);
		Sensor_WriteReg(0x5683, reg_value);

		reg_value=((ext_ptr->zone.w>>0x08)&0x0f);
		Sensor_WriteReg(0x5684, reg_value);
		reg_value=(ext_ptr->zone.w&0xff);
		Sensor_WriteReg(0x5685, reg_value);

		reg_value=((ext_ptr->zone.h>>0x08)&0x0f);
		Sensor_WriteReg(0x5686, reg_value);
		reg_value=(ext_ptr->zone.h&0xff);
		Sensor_WriteReg(0x5687, reg_value);

		Sensor_WriteReg(0x501d, 0x10);

	}
#endif
	return rtn;
}

LOCAL unsigned long _s5k4ec_StartExposure(unsigned long param)
{
	uint32_t rtn=SENSOR_SUCCESS;
    //exposure
    SENSOR_EXT_FUN_T_PTR ext_ptr=(SENSOR_EXT_FUN_T_PTR)param;

	SENSOR_PRINT("SENSOR: _s5k4ec_StartExposure param =%d \n",
		     ext_ptr->param);
#if 0
	switch (ext_ptr->param) {
	case SENSOR_EXT_EXPOSURE_AUTO:
		rtn = _s5k4ec_ExposureAuto();
		break;
	case SENSOR_EXT_EXPOSURE_ZONE:
		rtn = _s5k4ec_ExposureZone(ext_ptr);
		break;
	default:
		break;
    }
#endif
    return rtn;
}
LOCAL unsigned long _s5k4ec_SetEV(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;
	SENSOR_PRINT("SENSOR: _s5k4ec_SetEV param: 0x%x", ext_ptr->param);
	uint32_t shutter = 0;
	uint32_t gain = 0;

	uint32_t ev = ext_ptr->param;
	switch(ev) {
	case SENSOR_HDR_EV_LEVE_0:
		Sensor_WriteReg(0x0028, 0x7000);
		Sensor_WriteReg(0x002A, 0x04E6);
		Sensor_WriteReg(0x0F12, 0x0679);
		shutter =(uint32_t) s_current_shutter/2;
		s_current_shutter = s_current_shutter>>2;
		gain = (uint32_t)s_current_gain/4;
		SENSOR_PRINT("SENSOR: SENSOR_HDR_EV_LEVE_0: 0x%x,0x%x", (uint16_t)shutter,(uint16_t)gain);
		Sensor_WriteReg(0x0028, 0x7000);
		Sensor_WriteReg(0x002A, 0x04AC);
		Sensor_WriteReg(0x0F12, (uint16_t)shutter);
		Sensor_WriteReg(0x0028, 0x7000);
		Sensor_WriteReg(0x002A, 0x04B0);
		Sensor_WriteReg(0x0F12, 0x0001);

		Sensor_WriteReg(0x0028, 0x7000);
		Sensor_WriteReg(0x002A, 0x04C8);
		Sensor_WriteReg(0x0F12, (uint16_t)gain);
		Sensor_WriteReg(0x0028, 0x7000);
		Sensor_WriteReg(0x002A, 0x04CA);
		Sensor_WriteReg(0x0F12, 0x0001);
		break;
	case SENSOR_HDR_EV_LEVE_1:

		break;
	case SENSOR_HDR_EV_LEVE_2:

		break;
	default:
		break;
	}
	return rtn;
}

LOCAL uint8_t af_firmware[] = {
	#if 0 //\B2\BB\D3\C3AF
	0x80,
	0x00,
	#endif
};

LOCAL int _s5k4ec_init_firmware(uint32_t param)
{
	int ret = 0;
	#if 0
	uint32_t i = 0;
	uint32_t init_num = NUMBER_OF_ARRAY(af_firmware);
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR)param;
	uint8_t  *reg_ptr = af_firmware;
	struct i2c_msg msg_w;
	struct i2c_client *i2c_client = Sensor_GetI2CClien();
	uint16_t reg_val_1,reg_val_2;

	if (0 == i2c_client) {
		SENSOR_PRINT_HIGH("SENSOR: _s5k4ec_init_firmware:error,i2c_client is NULL!.\n");
	}
	//      sc8810_i2c_set_clk(1,400000); //wjp
	SENSOR_PRINT_HIGH("SENSOR: _s5k4ec_init_firmware: cmd=%d!.\n", ext_ptr->cmd);
	switch (ext_ptr->param)
	{
	 case SENSOR_EXT_FOCUS_TRIG:	//auto focus
		    reg_ptr = af_firmware;
		    break;
	 default:
		    break;
	}
	Sensor_WriteReg(0x3000, 0x20);
	for (i = 0; i < 4; i++) {
		msg_w.addr = i2c_client->addr;
		msg_w.flags = 0;
		msg_w.buf = reg_ptr;
		msg_w.len = init_num;
		ret = i2c_transfer(i2c_client->adapter, &msg_w, 1);
		if (ret != 1) {
			SENSOR_PRINT_ERR
			    ("SENSOR: write sensor reg fai, ret : %d, I2C w addr: 0x%x, \n",
			     ret, i2c_client->addr);
			continue;
		} else {
			ret = 0;
			break;
		}
	}
	Sensor_WriteReg(0x3022, 0x00);
	Sensor_WriteReg(0x3023, 0x00);
	Sensor_WriteReg(0x3024, 0x00);
	Sensor_WriteReg(0x3025, 0x00);
	Sensor_WriteReg(0x3026, 0x00);
	Sensor_WriteReg(0x3027, 0x00);
	Sensor_WriteReg(0x3028, 0x00);
	Sensor_WriteReg(0x3029, 0x7F);
	Sensor_WriteReg(0x3000, 0x00);

	reg_val_1 = Sensor_ReadReg(0x3000);
	reg_val_2 = Sensor_ReadReg(0x3004);
	//      sc8810_i2c_set_clk(1,100000); //wjp
	SENSOR_PRINT_HIGH("SENSOR: _s5k4ec_init_firmware: E!.\n");

	//SENSOR_PRINT_HIGH("SENSOR: 0x3029=0x%x,0x3000=0x%x,0x3004=0x%x.\n",Sensor_ReadReg(0x3029),reg_val_1,reg_val_2);
	//SENSOR_PRINT_HIGH("SENSOR: 0x8000=0x%x,0x8002=0x%x,0x8f57=0x%x.\n",Sensor_ReadReg(0x8000),Sensor_ReadReg(0x8002),Sensor_ReadReg(0x8f57));
	#endif
	return ret;
}


LOCAL unsigned long _s5k4ec_ExtFunc(unsigned long ctl_param)
{
	uint32_t rtn = SENSOR_SUCCESS;

	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr =(SENSOR_EXT_FUN_PARAM_T_PTR) ctl_param;
	SENSOR_PRINT("SENSOR: _s5k4ec_ExtFunc cmd:0x%x  \n", ext_ptr->cmd);

	switch (ext_ptr->cmd)
	{
	 case SENSOR_EXT_FUNC_INIT://AF \B3\F5ʼ\BB\AF
		    //rtn = _s5k4ec_init_firmware(ctl_param);
		    break;
	 case SENSOR_EXT_FOCUS_START://AF
		    //rtn = _s5k4ec_StartAutoFocus(ctl_param);
		    break;
	case SENSOR_EXT_EXPOSURE_START://\C6ع\E2
		    //rtn = _s5k4ec_StartExposure(ctl_param);
		    break;
	default:
		    break;
	}

	return rtn;
}

LOCAL uint32_t _s5k4ec_recovery_init()
{
	SENSOR_PRINT("SENSOR: s5k4ec_steamon recovery\n");

	Sensor_WriteReg(0x0028, 0x7000);
	Sensor_WriteReg(0x002A, 0x0242);
	Sensor_WriteReg(0x0F12, 0x0000);	//#REG_TC_GP_EnablePreview

	Sensor_WriteReg(0x0028, 0xD000);
	Sensor_WriteReg(0x002A, 0xB0A0);
	Sensor_WriteReg(0x0F12, 0x0000);	//Clear cont. clock befor config change

	Sensor_WriteReg(0x0028, 0x7000);
	Sensor_WriteReg(0x002A, 0x0244);
	Sensor_WriteReg(0x0F12, 0x0001);	//#REG_TC_GP_EnablePreviewChanged
	return 0;
}

LOCAL unsigned long _s5k4ec_StreamOn(unsigned long param)
{
	SENSOR_PRINT("SENSOR:Start s5k4ec_steamon 1613\n");

	if (1 != is_cap) {
		SENSOR_PRINT("zxdbg preview stream on");
		Sensor_WriteReg(0x0028, 0x7000);
		Sensor_WriteReg(0x002A, 0x023E);
		Sensor_WriteReg(0x0F12, 0x0001);  //#REG_TC_GP_EnablePreview
		Sensor_WriteReg(0x0F12, 0x0001);  //#REG_TC_GP_EnablePreviewChanged

		Sensor_WriteReg (0x0028, 0xD000);
		Sensor_WriteReg(0x002A, 0x1000);
		Sensor_WriteReg(0x0F12, 0x0001);
	} else {
		SENSOR_PRINT("zxdbg capture stream on");
		Sensor_WriteReg(0x0028, 0x7000);
		Sensor_WriteReg(0x002A, 0x0242);
		Sensor_WriteReg(0x0F12, 0x0001); // #REG_TC_GP_EnableCAPTURE // Start CAPTURE
		Sensor_WriteReg(0x002A, 0x024E);
		Sensor_WriteReg(0x0F12, 0x0001); // #REG_TC_GP_NewConfigSync // Update preview configuration
		Sensor_WriteReg(0x002A, 0x0244);
		Sensor_WriteReg(0x0F12, 0x0001); // #REG_TC_GP_EnableCAPTUREChanged
	}
	usleep(10*1000);
	return 0;
}

LOCAL unsigned long _s5k4ec_StreamOff(unsigned long param)
{
	SENSOR_PRINT("Stop");
	Sensor_WriteReg(0x0028, 0x7000);
	Sensor_WriteReg(0x002A, 0x023E);
	Sensor_WriteReg(0x0F12, 0x0000);	//#REG_TC_GP_EnablePreview

	Sensor_WriteReg(0x0028, 0xD000);
	Sensor_WriteReg(0x002A, 0xB0A0);
	Sensor_WriteReg(0x0F12, 0x0000);	//Clear cont. clock befor config change

	Sensor_WriteReg(0x0028, 0x7000);
	Sensor_WriteReg(0x002A, 0x0240);
	Sensor_WriteReg(0x0F12, 0x0001);	//#REG_TC_GP_EnablePreviewChanged
	usleep(10*1000);
	return 0;
}
