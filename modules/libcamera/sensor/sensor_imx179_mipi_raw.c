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
#include "sensor_imx179_raw_param.c"


#define IMX179_I2C_ADDR_W        0x10
#define IMX179_I2C_ADDR_R         0x10

#define IMX179_RAW_PARAM_COM  0x0000

#define IMX179_MIN_FRAME_LEN_PRV  0x04d8
#define IMX179_MIN_FRAME_LEN_CAP  0x09b1
#define IMX179_4_LANES
#define DW9714_VCM_SLAVE_ADDR (0x18 >> 1)
static int s_imx179_gain = 0;
static int s_capture_shutter = 0;
static int s_capture_VTS = 0;
static int s_video_min_framerate = 0;
static int s_video_max_framerate = 0;

LOCAL unsigned long _imx179_GetResolutionTrimTab(unsigned long param);
LOCAL unsigned long _imx179_PowerOn(unsigned long power_on);
LOCAL unsigned long _imx179_Identify(unsigned long param);
LOCAL unsigned long _imx179_BeforeSnapshot(unsigned long param);
LOCAL unsigned long _imx179_after_snapshot(unsigned long param);
LOCAL unsigned long _imx179_StreamOn(unsigned long param);
LOCAL unsigned long _imx179_StreamOff(unsigned long param);
LOCAL unsigned long _imx179_write_exposure(unsigned long param);
LOCAL unsigned long _imx179_write_gain(unsigned long param);
LOCAL unsigned long _imx179_write_af(unsigned long param);
LOCAL unsigned long _imx179_flash(unsigned long param);
LOCAL unsigned long _imx179_ExtFunc(unsigned long ctl_param);
LOCAL int _imx179_get_VTS(void);
LOCAL int _imx179_set_VTS(int VTS);
LOCAL uint32_t _imx179_ReadGain(uint16_t *data);
LOCAL unsigned long _imx179_set_video_mode(unsigned long param);
LOCAL int _imx179_get_shutter(void);
LOCAL uint32_t _imx179_com_Identify_otp(void* param_ptr);

LOCAL const struct raw_param_info_tab s_imx179_raw_param_tab[]={
	{IMX179_RAW_PARAM_COM, &s_imx179_mipi_raw_info, _imx179_com_Identify_otp, NULL},
	{RAW_INFO_END_ID, PNULL, PNULL, PNULL}
};

struct sensor_raw_info* s_imx179_mipi_raw_info_ptr=NULL;

static uint32_t g_module_id = 0;

static uint32_t g_flash_mode_en = 0;
static uint32_t g_af_slewrate = 1;

LOCAL const SENSOR_REG_T imx179_common_init[] = {
{0x0100, 0x00},
{0x0101, 0x00},
{0x0202, 0x03},
{0x0203, 0xC5},
{0x0301, 0x0A},
{0x0303, 0x01},
{0x0305, 0x06},
{0x0309, 0x0A},
{0x030B, 0x01},
{0x030C, 0x00},
{0x030D, 0x7D},
{0x0340, 0x03},
{0x0341, 0xC9},
{0x0342, 0x0D},
{0x0343, 0x70},
{0x0344, 0x00},
{0x0345, 0x28},
{0x0346, 0x01},
{0x0347, 0x4C},
{0x0348, 0x0C},
{0x0349, 0xA7},
{0x034A, 0x08},
{0x034B, 0x53},
{0x034C, 0x05},
{0x034D, 0x00},
{0x034E, 0x02},
{0x034F, 0xD0},
{0x0383, 0x01},
{0x0387, 0x01},
{0x0390, 0x01},
{0x0401, 0x02},
{0x0405, 0x14},
{0x3020, 0x10},
{0x3041, 0x15},
{0x3042, 0x87},
{0x3089, 0x4F},
{0x3309, 0x9A},
{0x3344, 0x47},
{0x3345, 0x1F},
{0x3362, 0x0A},
{0x3363, 0x0A},
{0x3364, 0x02},
{0x3368, 0x18},
{0x3369, 0x00},
{0x3370, 0x67},
{0x3371, 0x6F},
{0x3372, 0x47},
{0x3373, 0x27},
{0x3374, 0x1F},
{0x3375, 0x1F},
{0x3376, 0x7F},
{0x3377, 0x2F},
{0x33C8, 0x00},
{0x33D4, 0x06},
{0x33D5, 0x40},
{0x33D6, 0x03},
{0x33D7, 0x84},
{0x4100, 0x0E},
{0x4108, 0x01},
{0x4109, 0x7C},
};

LOCAL const SENSOR_REG_T imx179_1280x720_2lane_setting[] = {
{0x0101, 0x00},
{0x0202, 0x03},
{0x0203, 0xC5},
{0x0301, 0x0A},
{0x0303, 0x01},
{0x0305, 0x06},
{0x0309, 0x0A},
{0x030B, 0x01},
{0x030C, 0x00},
{0x030D, 0x7D},
{0x0340, 0x03},
{0x0341, 0xC9},
{0x0342, 0x0D},
{0x0343, 0x70},
{0x0344, 0x00},
{0x0345, 0x28},
{0x0346, 0x01},
{0x0347, 0x4C},
{0x0348, 0x0C},
{0x0349, 0xA7},
{0x034A, 0x08},
{0x034B, 0x53},
{0x034C, 0x05},
{0x034D, 0x00},
{0x034E, 0x02},
{0x034F, 0xD0},
{0x0383, 0x01},
{0x0387, 0x01},
{0x0390, 0x01},
{0x0401, 0x02},
{0x0405, 0x14},
{0x3020, 0x10},
{0x3041, 0x15},
{0x3042, 0x87},
{0x3089, 0x4F},
{0x3309, 0x9A},
{0x3344, 0x47},
{0x3345, 0x1F},
{0x3362, 0x0A},
{0x3363, 0x0A},
{0x3364, 0x02},
{0x3368, 0x18},
{0x3369, 0x00},
{0x3370, 0x67},
{0x3371, 0x6F},
{0x3372, 0x47},
{0x3373, 0x27},
{0x3374, 0x1F},
{0x3375, 0x1F},
{0x3376, 0x7F},
{0x3377, 0x2F},
{0x33C8, 0x00},
{0x33D4, 0x06},
{0x33D5, 0x40},
{0x33D6, 0x03},
{0x33D7, 0x84},
{0x4100, 0x0E},
{0x4108, 0x01},
{0x4109, 0x7C},
};

//15 fps
LOCAL const SENSOR_REG_T imx179_3264x2448_2lane_setting[] = {
//{0x0100, 0x00},
{0x0101, 0x03},
{0x0202, 0x09},
{0x0203, 0xAD},
{0x0301, 0x0A},
{0x0303, 0x01},
{0x0305, 0x06},
{0x0309, 0x0A},
{0x030B, 0x01},
{0x030C, 0x00},
{0x030D, 0xA0},
{0x0340, 0x09},
{0x0341, 0xB1},
{0x0342, 0x0D},
{0x0343, 0x70},
{0x0344, 0x00},
{0x0345, 0x08},
{0x0346, 0x00},
{0x0347, 0x08},
{0x0348, 0x0C},
{0x0349, 0xC7},
{0x034A, 0x09},
{0x034B, 0x97},
{0x034C, 0x0C},
{0x034D, 0xC0},
{0x034E, 0x09},
{0x034F, 0x90},
{0x0383, 0x01},
{0x0387, 0x01},
{0x0390, 0x00},
{0x0401, 0x00},
{0x0405, 0x10},
{0x3020, 0x10},
{0x3041, 0x15},
{0x3042, 0x87},
{0x3089, 0x4F},
{0x3309, 0x9A},
{0x3344, 0x57},
{0x3345, 0x1F},
{0x3362, 0x0A},
{0x3363, 0x0A},
{0x3364, 0x02},
{0x3368, 0x18},
{0x3369, 0x00},
{0x3370, 0x77},
{0x3371, 0x6F},
{0x3372, 0x4F},
{0x3373, 0x2F},
{0x3374, 0x2F},
{0x3375, 0x37},
{0x3376, 0x9F},
{0x3377, 0x37},
{0x33C8, 0x00},
{0x33D4, 0x0C},
{0x33D5, 0xC0},
{0x33D6, 0x09},
{0x33D7, 0x90},
{0x4100, 0x0E},
{0x4108, 0x01},
{0x4109, 0x7C},
};
//30fps
LOCAL const SENSOR_REG_T imx179_3264x2448_4lane_setting[] = {
{0x0100, 0x00},
{0x0101, 0x00},
{0x0202, 0x09},
{0x0203, 0xAD},
{0x0301, 0x05},
{0x0303, 0x01},
{0x0305, 0x06},
{0x0309, 0x05},
{0x030B, 0x01},
{0x030C, 0x00},
{0x030D, 0xA0},
{0x0340, 0x09},
{0x0341, 0xB1},
{0x0342, 0x0D},
{0x0343, 0x70},
{0x0344, 0x00},
{0x0345, 0x08},
{0x0346, 0x00},
{0x0347, 0x08},
{0x0348, 0x0C},
{0x0349, 0xC7},
{0x034A, 0x09},
{0x034B, 0x97},
{0x034C, 0x0C},
{0x034D, 0xC0},
{0x034E, 0x09},
{0x034F, 0x90},
{0x0383, 0x01},
{0x0387, 0x01},
{0x0390, 0x00},
{0x0401, 0x00},
{0x0405, 0x10},
{0x3020, 0x10},
{0x3041, 0x15},
{0x3042, 0x87},
{0x3089, 0x4F},
{0x3309, 0x9A},
{0x3344, 0x57},
{0x3345, 0x1F},
{0x3362, 0x0A},
{0x3363, 0x0A},
{0x3364, 0x00},
{0x3368, 0x18},
{0x3369, 0x00},
{0x3370, 0x77},
{0x3371, 0x6F},
{0x3372, 0x4F},
{0x3373, 0x2F},
{0x3374, 0x2F},
{0x3375, 0x37},
{0x3376, 0x9F},
{0x3377, 0x37},
{0x33C8, 0x00},
{0x33D4, 0x0C},
{0x33D5, 0xC0},
{0x33D6, 0x09},
{0x33D7, 0x90},
{0x4100, 0x0E},
{0x4108, 0x01},
{0x4109, 0x7C},
};

//22fps 500bps/lane 8M
LOCAL const SENSOR_REG_T imx179_3264x2448_4lane_setting_a[] = {
//{0x0100, 0x00},
{0x0101, 0x00},
{0x0202, 0x0A},
{0x0203, 0x4F},
{0x0301, 0x05},
{0x0303, 0x01},
{0x0305, 0x06},
{0x0309, 0x05},
{0x030B, 0x01},
{0x030C, 0x00},
{0x030D, 0x7D},
{0x0340, 0x0A},
{0x0341, 0x53},
{0x0342, 0x0D},
{0x0343, 0x70},
{0x0344, 0x00},
{0x0345, 0x08},
{0x0346, 0x00},
{0x0347, 0x08},
{0x0348, 0x0C},
{0x0349, 0xC7},
{0x034A, 0x09},
{0x034B, 0x97},
{0x034C, 0x0C},
{0x034D, 0xC0},
{0x034E, 0x09},
{0x034F, 0x90},
{0x0383, 0x01},
{0x0387, 0x01},
{0x0390, 0x00},
{0x0401, 0x00},
{0x0405, 0x10},
{0x3020, 0x10},
{0x3041, 0x15},
{0x3042, 0x87},
{0x3089, 0x4F},
{0x3309, 0x9A},
{0x3344, 0x47},
{0x3345, 0x1F},
{0x3362, 0x0A},
{0x3363, 0x0A},
{0x3364, 0x00},
{0x3368, 0x18},
{0x3369, 0x00},
{0x3370, 0x67},
{0x3371, 0x6F},
{0x3372, 0x47},
{0x3373, 0x27},
{0x3374, 0x1F},
{0x3375, 0x1F},
{0x3376, 0x7F},
{0x3377, 0x2F},
{0x33C8, 0x00},
{0x33D4, 0x0C},
{0x33D5, 0xC0},
{0x33D6, 0x09},
{0x33D7, 0x90},
{0x4100, 0x0E},
{0x4108, 0x01},
{0x4109, 0x7C},
{0x0100, 0x00},
{0x0101, 0x00},
{0x0202, 0x0A},
{0x0203, 0x4F},
{0x0301, 0x05},
{0x0303, 0x01},
{0x0305, 0x06},
{0x0309, 0x05},
{0x030B, 0x01},
{0x030C, 0x00},
{0x030D, 0x7D},
{0x0340, 0x0A},
{0x0341, 0x53},
{0x0342, 0x0D},
{0x0343, 0x70},
{0x0344, 0x00},
{0x0345, 0x08},
{0x0346, 0x00},
{0x0347, 0x08},
{0x0348, 0x0C},
{0x0349, 0xC7},
{0x034A, 0x09},
{0x034B, 0x97},
{0x034C, 0x0C},
{0x034D, 0xC0},
{0x034E, 0x09},
{0x034F, 0x90},
{0x0383, 0x01},
{0x0387, 0x01},
{0x0390, 0x00},
{0x0401, 0x00},
{0x0405, 0x10},
{0x3020, 0x10},
{0x3041, 0x15},
{0x3042, 0x87},
{0x3089, 0x4F},
{0x3309, 0x9A},
{0x3344, 0x47},
{0x3345, 0x1F},
{0x3362, 0x0A},
{0x3363, 0x0A},
{0x3364, 0x00},
{0x3368, 0x18},
{0x3369, 0x00},
{0x3370, 0x67},
{0x3371, 0x6F},
{0x3372, 0x47},
{0x3373, 0x27},
{0x3374, 0x1F},
{0x3375, 0x1F},
{0x3376, 0x7F},
{0x3377, 0x2F},
{0x33C8, 0x00},
{0x33D4, 0x0C},
{0x33D5, 0xC0},
{0x33D6, 0x09},
{0x33D7, 0x90},
{0x4100, 0x0E},
{0x4108, 0x01},
{0x4109, 0x7C},
//{0x0100, 0x01},
};

LOCAL const SENSOR_REG_T imx179_3264x2448_4lane_setting_b[] = {
/*
3264*2448 MIPI 4 lane with 560Mbps/lane 26fps
*/
{0x0101, 0x00},
{0x0202, 0x09},
{0x0203, 0xC4},
{0x0301, 0x05},
{0x0303, 0x01},
{0x0305, 0x06},
{0x0309, 0x05},
{0x030B, 0x01},
{0x030C, 0x00},
{0x030D, 0x8C},
{0x0340, 0x09},
{0x0341, 0xC8},
{0x0342, 0x0D},
{0x0343, 0x70},
{0x0344, 0x00},
{0x0345, 0x08},
{0x0346, 0x00},
{0x0347, 0x08},
{0x0348, 0x0C},
{0x0349, 0xC7},
{0x034A, 0x09},
{0x034B, 0x97},
{0x034C, 0x0C},
{0x034D, 0xC0},
{0x034E, 0x09},
{0x034F, 0x90},
{0x0383, 0x01},
{0x0387, 0x01},
{0x0390, 0x00},
{0x0401, 0x00},
{0x0405, 0x10},
{0x3020, 0x10},
{0x3041, 0x15},
{0x3042, 0x87},
{0x3089, 0x4F},
{0x3309, 0x9A},
{0x3344, 0x4F},
{0x3345, 0x1F},
{0x3362, 0x0A},
{0x3363, 0x0A},
{0x3364, 0x00},
{0x3368, 0x18},
{0x3369, 0x00},
{0x3370, 0x6F},
{0x3371, 0x54},
{0x3372, 0x47},
{0x3373, 0x27},
{0x3374, 0x27},
{0x3375, 0x27},
{0x3376, 0x8F},
{0x3377, 0x37},
{0x33C8, 0x00},
{0x33D4, 0x0C},
{0x33D5, 0xC0},
{0x33D6, 0x09},
{0x33D7, 0x90},
{0x4100, 0x0E},
{0x4108, 0x01},
{0x4109, 0x7C},
};

LOCAL const SENSOR_REG_T imx179_3264x2448_4lane_setting_bpp[] = {
/*3264x2448 MIPI 4lane @600Mbps/lane data rate 27fps*/
{0x0101, 0x00},
{0x0202, 0x09},
{0x0203, 0xA1},
{0x0301, 0x05},
{0x0303, 0x01},
{0x0305, 0x06},
{0x0309, 0x05},
{0x030B, 0x01},
{0x030C, 0x00},
{0x030D, 0x96},
{0x0340, 0x09},
{0x0341, 0xA5},
{0x0342, 0x0E},
{0x0343, 0x10},
{0x0344, 0x00},
{0x0345, 0x08},
{0x0346, 0x00},
{0x0347, 0x08},
{0x0348, 0x0C},
{0x0349, 0xC7},
{0x034A, 0x09},
{0x034B, 0x97},
{0x034C, 0x0C},
{0x034D, 0xC0},
{0x034E, 0x09},
{0x034F, 0x90},
{0x0383, 0x01},
{0x0387, 0x01},
{0x0390, 0x00},
{0x0401, 0x00},
{0x0405, 0x10},
{0x3020, 0x10},
{0x3041, 0x15},
{0x3042, 0x87},
{0x3089, 0x4F},
{0x3309, 0x9A},
{0x3344, 0x57},
{0x3345, 0x1F},
{0x3362, 0x0A},
{0x3363, 0x0A},
{0x3364, 0x00},
{0x3368, 0x18},
{0x3369, 0x00},
{0x3370, 0x6F},
{0x3371, 0x5A},
{0x3372, 0x4F},
{0x3373, 0x2F},
{0x3374, 0x27},
{0x3375, 0x2F},
{0x3376, 0x97},
{0x3377, 0x37},
{0x33C8, 0x00},
{0x33D4, 0x0C},
{0x33D5, 0xC0},
{0x33D6, 0x09},
{0x33D7, 0x90},
{0x4100, 0x0E},
{0x4108, 0x01},
{0x4109, 0x7C},
};

LOCAL const SENSOR_REG_T imx179_3264x2448_4lane_setting_c[] = {
/*3264x2448 MIPI 4lane @600Mbps/lane data rate 28fps*/
{0x0101,0x00},
{0x0202,0x09},
{0x0203,0xB8},
{0x0301,0x05},
{0x0303,0x01},
{0x0305,0x06},
{0x0309,0x05},
{0x030B,0x01},
{0x030C,0x00},
{0x030D,0x96},
{0x0340,0x09},
{0x0341,0xBC},
{0x0342,0x0D},
{0x0343,0x70},
{0x0344,0x00},
{0x0345,0x08},
{0x0346,0x00},
{0x0347,0x08},
{0x0348,0x0C},
{0x0349,0xC7},
{0x034A,0x09},
{0x034B,0x97},
{0x034C,0x0C},
{0x034D,0xC0},
{0x034E,0x09},
{0x034F,0x90},
{0x0383,0x01},
{0x0387,0x01},
{0x0390,0x00},
{0x0401,0x00},
{0x0405,0x10},
{0x3020,0x10},
{0x3041,0x15},
{0x3042,0x87},
{0x3089,0x4F},
{0x3309,0x9A},
{0x3344,0x57},
{0x3345,0x1F},
{0x3362,0x0A},
{0x3363,0x0A},
{0x3364,0x00},
{0x3368,0x18},
{0x3369,0x00},
{0x3370,0x6F},
{0x3371,0x5A},
{0x3372,0x4F},
{0x3373,0x2F},
{0x3374,0x27},
{0x3375,0x2F},
{0x3376,0x97},
{0x3377,0x37},
{0x33C8,0x00},
{0x33D4,0x0C},
{0x33D5,0xC0},
{0x33D6,0x09},
{0x33D7,0x90},
{0x4100,0x0E},
{0x4108,0x01},
{0x4109,0x7C},
};

LOCAL const SENSOR_REG_T imx179_3264x2448_4lane_setting_d[] = {
/* 8M&29fps. 640Mbps/lane */
{0x0100,0x00},
{0x0101,0x00},
{0x0202,0x09},
{0x0203,0x9E},
{0x0301,0x05},
{0x0303,0x01},
{0x0305,0x06},
{0x0309,0x05},
{0x030B,0x01},
{0x030C,0x00},
{0x030D,0xA0},
{0x0340,0x09},
{0x0341,0xA2},
{0x0342,0x0D},
{0x0343,0xFC},
{0x0344,0x00},
{0x0345,0x08},
{0x0346,0x00},
{0x0347,0x08},
{0x0348,0x0C},
{0x0349,0xC7},
{0x034A,0x09},
{0x034B,0x97},
{0x034C,0x0C},
{0x034D,0xC0},
{0x034E,0x09},
{0x034F,0x90},
{0x0383,0x01},
{0x0387,0x01},
{0x0390,0x00},
{0x0401,0x00},
{0x0405,0x10},
{0x3020,0x10},
{0x3041,0x15},
{0x3042,0x87},
{0x3089,0x4F},
{0x3309,0x9A},
{0x3344,0x57},
{0x3345,0x1F},
{0x3362,0x0A},
{0x3363,0x0A},
{0x3364,0x00},
{0x3368,0x18},
{0x3369,0x00},
{0x3370,0x77},
{0x3371,0x60},
{0x3372,0x4F},
{0x3373,0x2F},
{0x3374,0x2F},
{0x3375,0x37},
{0x3376,0x9F},
{0x3377,0x37},
{0x33C8,0x00},
{0x33D4,0x0C},
{0x33D5,0xC0},
{0x33D6,0x09},
{0x33D7,0x90},
{0x4100,0x0E},
{0x4108,0x01},
{0x4109,0x7C},
};

LOCAL const SENSOR_REG_T imx179_3084x1736_4lane_setting_a[] = {
/*3084x1736, with 30fps, 4 lanes, about 640Mbps/lanse*/
{0x0100, 0x00},
{0x0101, 0x00},
{0x0202, 0x06},
{0x0203, 0xEE},
{0x0301, 0x05},
{0x0303, 0x01},
{0x0305, 0x06},
{0x0309, 0x05},
{0x030B, 0x01},
{0x030C, 0x00},
{0x030D, 0xA0},
{0x0340, 0x06},
{0x0341, 0xF2},
{0x0342, 0x12},
{0x0343, 0xC0},
{0x0344, 0x00},
{0x0345, 0x62},
{0x0346, 0x01},
{0x0347, 0x6C},
{0x0348, 0x0C},
{0x0349, 0x6D},
{0x034A, 0x08},
{0x034B, 0x33},
{0x034C, 0x0C},
{0x034D, 0x0C},
{0x034E, 0x06},
{0x034F, 0xC8},
{0x0383, 0x01},
{0x0387, 0x01},
{0x0390, 0x00},
{0x0401, 0x00},
{0x0405, 0x10},
{0x3020, 0x10},
{0x3041, 0x15},
{0x3042, 0x87},
{0x3089, 0x4F},
{0x3309, 0x9A},
{0x3344, 0x57},
{0x3345, 0x1F},
{0x3362, 0x0A},
{0x3363, 0x0A},
{0x3364, 0x00},
{0x3368, 0x18},
{0x3369, 0x00},
{0x3370, 0x77},
{0x3371, 0x60},
{0x3372, 0x4F},
{0x3373, 0x2F},
{0x3374, 0x2F},
{0x3375, 0x37},
{0x3376, 0x9F},
{0x3377, 0x37},
{0x33C8, 0x00},
{0x33D4, 0x0C},
{0x33D5, 0x0C},
{0x33D6, 0x06},
{0x33D7, 0xC8},
{0x4100, 0x0E},
{0x4108, 0x01},
{0x4109, 0x7C},
};

LOCAL const SENSOR_REG_T imx179_3084x1736_4lane_setting_b[] = {
/*3084x1736, with 25fps, 4 lanes, about 600Mbps/lanse*/
{0x0100, 0x00},
{0x0101, 0x00},
{0x0202, 0x07},
{0x0203, 0xCC},
{0x0301, 0x05},
{0x0303, 0x01},
{0x0305, 0x06},
{0x0309, 0x05},
{0x030B, 0x01},
{0x030C, 0x00},
{0x030D, 0x96},
{0x0340, 0x07},
{0x0341, 0xD0},
{0x0342, 0x12},
{0x0343, 0xC0},
{0x0344, 0x00},
{0x0345, 0x62},
{0x0346, 0x01},
{0x0347, 0x6C},
{0x0348, 0x0C},
{0x0349, 0x6D},
{0x034A, 0x08},
{0x034B, 0x33},
{0x034C, 0x0C},
{0x034D, 0x0C},
{0x034E, 0x06},
{0x034F, 0xC8},
{0x0383, 0x01},
{0x0387, 0x01},
{0x0390, 0x00},
{0x0401, 0x00},
{0x0405, 0x10},
{0x3020, 0x10},
{0x3041, 0x15},
{0x3042, 0x87},
{0x3089, 0x4F},
{0x3309, 0x9A},
{0x3344, 0x57},
{0x3345, 0x1F},
{0x3362, 0x0A},
{0x3363, 0x0A},
{0x3364, 0x00},
{0x3368, 0x18},
{0x3369, 0x00},
{0x3370, 0x6F},
{0x3371, 0x5A},
{0x3372, 0x4F},
{0x3373, 0x2F},
{0x3374, 0x27},
{0x3375, 0x2F},
{0x3376, 0x97},
{0x3377, 0x37},
{0x33C8, 0x00},
{0x33D4, 0x0C},
{0x33D5, 0x0C},
{0x33D6, 0x06},
{0x33D7, 0xC8},
{0x4100, 0x0E},
{0x4108, 0x01},
{0x4109, 0x7C},
};

LOCAL const SENSOR_REG_T imx179_1632x1224_4lane_setting[] =
{
//{0x0100, 0x00},
{0x0101, 0x00},
{0x0202, 0x05},
{0x0203, 0xBD},
{0x0301, 0x05},
{0x0303, 0x01},
{0x0305, 0x06},
{0x0309, 0x05},
{0x030B, 0x01},
{0x030C, 0x00},
{0x030D, 0x5F},
{0x0340, 0x05},
{0x0341, 0xC1},
{0x0342, 0x0D},
{0x0343, 0x70},
{0x0344, 0x00},
{0x0345, 0x08},
{0x0346, 0x00},
{0x0347, 0x08},
{0x0348, 0x0C},
{0x0349, 0xC7},
{0x034A, 0x09},
{0x034B, 0x97},
{0x034C, 0x06},
{0x034D, 0x60},
{0x034E, 0x04},
{0x034F, 0xC8},
{0x0383, 0x01},
{0x0387, 0x01},
{0x0390, 0x01},
{0x0401, 0x00},
{0x0405, 0x10},
{0x3020, 0x10},
{0x3041, 0x15},
{0x3042, 0x87},
{0x3089, 0x4F},
{0x3309, 0x9A},
{0x3344, 0x37},
{0x3345, 0x1F},
{0x3362, 0x0A},
{0x3363, 0x0A},
{0x3364, 0x00},
{0x3368, 0x18},
{0x3369, 0x00},
{0x3370, 0x5F},
{0x3371, 0x6f},
{0x3372, 0x3F},
{0x3373, 0x1F},
{0x3374, 0x1F},
{0x3375, 0x17},
{0x3376, 0x5F},
{0x3377, 0x27},
{0x33C8, 0x00},
{0x33D4, 0x06},
{0x33D5, 0x60},
{0x33D6, 0x04},
{0x33D7, 0xC8},
{0x4100, 0x0E},
{0x4108, 0x01},
{0x4109, 0x7C},
};

//30 fps
LOCAL const SENSOR_REG_T imx179_1632x1224_2lane_setting[] = {
{0x0100, 0x00},
{0x0101, 0x03},
{0x0202, 0x04},
{0x0203, 0xD4},
{0x0301, 0x0A},
{0x0303, 0x01},
{0x0305, 0x06},
{0x0309, 0x0A},
{0x030B, 0x01},
{0x030C, 0x00},
{0x030D, 0xA0},
{0x0340, 0x04},
{0x0341, 0xD8},
{0x0342, 0x0D},
{0x0343, 0x70},
{0x0344, 0x00},
{0x0345, 0x08},
{0x0346, 0x00},
{0x0347, 0x08},
{0x0348, 0x0C},
{0x0349, 0xC7},
{0x034A, 0x09},
{0x034B, 0x97},
{0x034C, 0x06},
{0x034D, 0x60},
{0x034E, 0x04},
{0x034F, 0xC8},
{0x0383, 0x01},
{0x0387, 0x01},
{0x0390, 0x01},
{0x0401, 0x00},
{0x0405, 0x10},
{0x3020, 0x10},
{0x3041, 0x15},
{0x3042, 0x87},
{0x3089, 0x4F},
{0x3309, 0x9A},
{0x3344, 0x57},
{0x3345, 0x1F},
{0x3362, 0x0A},
{0x3363, 0x0A},
{0x3364, 0x02},
{0x3368, 0x18},
{0x3369, 0x00},
{0x3370, 0x77},
{0x3371, 0x6F},
{0x3372, 0x4F},
{0x3373, 0x2F},
{0x3374, 0x2F},
{0x3375, 0x37},
{0x3376, 0x9F},
{0x3377, 0x37},
{0x33C8, 0x00},
{0x33D4, 0x06},
{0x33D5, 0x60},
{0x33D6, 0x04},
{0x33D7, 0xC8},
{0x4100, 0x0E},
{0x4108, 0x01},
{0x4109, 0x7C},
};

LOCAL SENSOR_REG_TAB_INFO_T s_imx179_resolution_Tab_RAW[] = {
	//{ADDR_AND_LEN_OF_ARRAY(imx179_common_init), 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},
	{PNULL, 0, 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},
	//{ADDR_AND_LEN_OF_ARRAY(imx179_1632x1224_2lane_setting), 1632, 1224, 24, SENSOR_IMAGE_FORMAT_RAW},
	//{ADDR_AND_LEN_OF_ARRAY(imx179_3264x2448_2lane_setting), 3264, 2448, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(imx179_3264x2448_4lane_setting), 3264, 2448, 24, SENSOR_IMAGE_FORMAT_RAW},
	//{ADDR_AND_LEN_OF_ARRAY(imx179_1280x720_2lane_setting), 1280, 720, 24, SENSOR_IMAGE_FORMAT_RAW},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
};

LOCAL SENSOR_TRIM_T s_imx179_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},

//	{0, 0, 1920, 1080, 178, 90, 1868},
	{0, 0, 3264, 2448, 168, 0, 2480, {0, 0, 3264, 2448}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}}
};

LOCAL const SENSOR_REG_T s_imx179_1632x1224_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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
LOCAL const SENSOR_REG_T s_imx179_1920x1080_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

LOCAL const SENSOR_REG_T  s_imx179_3264x2448_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

LOCAL SENSOR_VIDEO_INFO_T s_imx179_video_info[] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{30, 30, 178, 90}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_imx179_1920x1080_video_tab},
	{{{15, 15, 168, 64}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},(SENSOR_REG_T**)s_imx179_3264x2448_video_tab},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}
};

LOCAL unsigned long _imx179_set_video_mode(unsigned long param)
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

	if (PNULL == s_imx179_video_info[mode].setting_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR)&s_imx179_video_info[mode].setting_ptr[param];
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


LOCAL SENSOR_IOCTL_FUNC_TAB_T s_imx179_ioctl_func_tab = {
	PNULL,
	_imx179_PowerOn,
	PNULL,
	_imx179_Identify,

	PNULL,			// write register
	PNULL,			// read  register
	PNULL,
	_imx179_GetResolutionTrimTab,

	// External
	PNULL,
	PNULL,
	PNULL,

	PNULL, //_imx179_set_brightness,
	PNULL, // _imx179_set_contrast,
	PNULL,
	PNULL,			//_imx179_set_saturation,

	PNULL, //_imx179_set_work_mode,
	PNULL, //_imx179_set_image_effect,

	_imx179_BeforeSnapshot,
	_imx179_after_snapshot,
	_imx179_flash,
	PNULL,
	_imx179_write_exposure,
	PNULL,
	_imx179_write_gain,
	PNULL,
	PNULL,
	_imx179_write_af,
	PNULL,
	PNULL, //_imx179_set_awb,
	PNULL,
	PNULL,
	PNULL, //_imx179_set_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL, //_imx179_GetExifInfo,
	_imx179_ExtFunc,
	PNULL, //_imx179_set_anti_flicker,
	_imx179_set_video_mode,
	PNULL, //pick_jpeg_stream
	PNULL,  //meter_mode
	PNULL, //get_status
	_imx179_StreamOn,
	_imx179_StreamOff,
	PNULL,
};


SENSOR_INFO_T g_imx179_mipi_raw_info = {
	IMX179_I2C_ADDR_W,	// salve i2c write address
	IMX179_I2C_ADDR_R,	// salve i2c read address

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
	{{0x2, 0x1},		// supply two code to identify sensor.
	 {0x3, 0x79}},		// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,	// voltage of avdd

	3264,			// max width of source image
	2448,			// max height of source image
	"imx179",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_R,// pattern of input image form sensor;

	s_imx179_resolution_Tab_RAW,	// point to resolution table information structure
	&s_imx179_ioctl_func_tab,	// point to ioctl function table
	&s_imx179_mipi_raw_info_ptr,		// information and table about Rawrgb sensor
	NULL,			//&g_imx179_ext_info,                // extend information about sensor
	SENSOR_AVDD_1800MV,	// iovdd
	SENSOR_AVDD_1200MV,	// dvdd
	3,			// skip frame num before preview
	3,			// skip frame num before capture
	0,			// deci frame num during preview
	0,			// deci frame num during video preview

	0,
	0,
	0,
	0,
	0,
#if defined(IMX179_2_LANES)
	{SENSOR_INTERFACE_TYPE_CSI2, 2, 10, 0},
#elif defined(IMX179_4_LANES)
	{SENSOR_INTERFACE_TYPE_CSI2, 4, 10, 0},
#endif

	s_imx179_video_info,
	3,			// skip frame num while change setting
};

LOCAL struct sensor_raw_info* Sensor_GetContext(void)
{
	return s_imx179_mipi_raw_info_ptr;
}

LOCAL uint32_t Sensor_imx179_InitRawTuneInfo(void)
{
	uint32_t rtn=0x00;
	struct sensor_raw_info* raw_sensor_ptr=Sensor_GetContext();
	struct sensor_raw_tune_info* sensor_ptr=raw_sensor_ptr->tune_ptr;
	struct sensor_raw_cali_info* cali_ptr=raw_sensor_ptr->cali_ptr;
#if 0
	raw_sensor_ptr->version_info->version_id=0x00010000;
	raw_sensor_ptr->version_info->srtuct_size=sizeof(struct sensor_raw_info);

	//bypass
	sensor_ptr->version_id=0x00010000;
	sensor_ptr->blc_bypass=0x00;
	sensor_ptr->nlc_bypass=0x01;
	sensor_ptr->lnc_bypass=0x01;
	sensor_ptr->ae_bypass=0x01;
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
	sensor_ptr->css_bypass=0x01;
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
	sensor_ptr->ae.smart=0;
	sensor_ptr->ae.smart_rotio=255;
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

	//af info
	sensor_ptr->af.max_step=1024;
	sensor_ptr->af.stab_period=10;

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
#endif
	return rtn;
}

LOCAL unsigned long _dw9174_SRCInit(unsigned long mode)
{
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
				SENSOR_PRINT("SENSOR_IMX179: _dw9174_SRCInit fail!1");
			}
			cmd_val[0] = 0xf2;
			cmd_val[1] = 0x00;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
			if(ret_value){
				SENSOR_PRINT("SENSOR_IMX179: _dw9174_SRCInit fail!2");
			}

			cmd_val[0] = 0xdc;
			cmd_val[1] = 0x51;
			ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);
			if(ret_value){
				SENSOR_PRINT("SENSOR_IMX179: _dw9174_SRCInit fail!3");
			}
		}
		break;

		case 3:
		break;

	}

	return ret_value;
}

LOCAL unsigned long _imx179_GetResolutionTrimTab(unsigned long param)
{
	SENSOR_PRINT("0x%lx",  (unsigned long)s_imx179_Resolution_Trim_Tab);
	return (unsigned long) s_imx179_Resolution_Trim_Tab;
}
LOCAL unsigned long _imx179_PowerOn(unsigned long power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_imx179_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_imx179_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_imx179_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_imx179_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_imx179_mipi_raw_info.reset_pulse_level;

	//uint32_t reset_width=g_imx179_yuv_info.reset_pulse_width;
	uint8_t pid_value = 0x00;

	if (SENSOR_TRUE == power_on) {
	#if 0
		Sensor_SetResetLevel(reset_level);
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		usleep(200*1000);
		//usleep(10*1000);
		//Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		//usleep(10*1000);
		usleep(108*1000);
		//Sensor_PowerDown(!power_down);
		Sensor_SetResetLevel(!reset_level);
		usleep(530*1000);
	#else
		Sensor_SetResetLevel(reset_level);
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		usleep(10*1000);
		_dw9174_SRCInit(2);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(10*1000);
		Sensor_SetResetLevel(!reset_level);
		usleep(10*1000);
	#endif
	} else {
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
	}

	SENSOR_PRINT("SENSOR_IMX179: _imx179_Power_On(1:on, 0:off): %d, reset_level %d, dvdd_val %ld", power_on, reset_level, dvdd_val);
	return SENSOR_SUCCESS;
}

LOCAL unsigned long _imx179_cfg_otp(unsigned long  param)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_imx179_raw_param_tab;
	uint32_t module_id=g_module_id;

	SENSOR_PRINT("SENSOR_IMX179: _imx179_cfg_otp");

	if(PNULL!=tab_ptr[module_id].cfg_otp){
		tab_ptr[module_id].cfg_otp(0);
		}

	return rtn;
}

LOCAL uint32_t _imx179_com_Identify_otp(void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_IMX179: _imx179_com_Identify_otp");

	/*read param id from sensor omap*/
	param_id=IMX179_RAW_PARAM_COM;

	if(IMX179_RAW_PARAM_COM==param_id){
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}

LOCAL uint32_t _imx179_GetRawInof(void)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_imx179_raw_param_tab;
	uint32_t param_id;
	uint32_t i=0x00;

	/*read param id from sensor omap*/
	param_id=IMX179_RAW_PARAM_COM;

	for(i=0x00; ; i++)
	{
		g_module_id = i;
		if(RAW_INFO_END_ID==tab_ptr[i].param_id){
			if(NULL==s_imx179_mipi_raw_info_ptr){
				SENSOR_PRINT("SENSOR_IMX179: ov5647_GetRawInof no param error");
				rtn=SENSOR_FAIL;
			}
			SENSOR_PRINT("SENSOR_IMX179: imx179_GetRawInof end");
			break;
		}
		else if(PNULL!=tab_ptr[i].identify_otp){
			if(SENSOR_SUCCESS==tab_ptr[i].identify_otp(0))
			{
				s_imx179_mipi_raw_info_ptr = tab_ptr[i].info_ptr;
				SENSOR_PRINT("SENSOR_IMX179: imx179_GetRawInof success");
				break;
			}
		}
	}

	return rtn;
}

LOCAL unsigned long _imx179_GetMaxFrameLine(unsigned long index)
{
	uint32_t max_line=0x00;
	SENSOR_TRIM_T_PTR trim_ptr=s_imx179_Resolution_Trim_Tab;

	max_line=trim_ptr[index].frame_line;

	return max_line;
}

LOCAL unsigned long _imx179_Identify(unsigned long param)
{
#define IMX179_PID_VALUE    0x01
#define IMX179_PID_ADDR     0x0002
#define IMX179_VER_VALUE    0x79
#define IMX179_VER_ADDR     0x0003

	uint8_t pid_value = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("SENSOR_IMX179: mipi raw identify\n");
	pid_value = Sensor_ReadReg(IMX179_PID_ADDR);
	pid_value = pid_value & 0x0f;
	if (IMX179_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(IMX179_VER_ADDR);
		SENSOR_PRINT("SENSOR_IMX179: Identify: PID = %x, VER = %x", pid_value, ver_value);
		if (IMX179_VER_VALUE == ver_value) {
			_imx179_GetRawInof();
			Sensor_imx179_InitRawTuneInfo();
			ret_value = SENSOR_SUCCESS;
			SENSOR_PRINT("SENSOR_IMX179: it's IMX179 sensor!");
		} else {
			SENSOR_PRINT("SENSOR_IMX179: Identify this is IMX%x%x sensor !", pid_value, ver_value);
		}
	} else {
		SENSOR_PRINT("SENSOR_IMX179: identify fail,pid_value=%d", pid_value);
	}

	return ret_value;
}

LOCAL unsigned long _imx179_write_exposure(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t dummy_line = 0x00;
	uint16_t expsure_line = 0x00;
	uint16_t frame_len_cur = 0x00;
	uint16_t frame_len = 0x00;

	return 0;
#if 0
	expsure_line = param & 0xffff;
	dummy_line = (param >> 16) & 0xffff;
	if(expsure_line < 1){
		expsure_line = 1;
	}

	//frame_len = expsure_line + 4;
	frame_len = expsure_line + dummy_line;
	frame_len = (frame_len > (expsure_line + 4)) ? frame_len : (expsure_line + 4);
	frame_len = (frame_len > IMX179_MIN_FRAME_LEN_PRV) ? frame_len :  IMX179_MIN_FRAME_LEN_PRV;
	frame_len_cur = (Sensor_ReadReg(0x0341)) & 0xff;
	frame_len_cur |= (Sensor_ReadReg(0x0340) << 0x08) & 0xff00;

	SENSOR_PRINT("SENSOR_IMX179: write_exposure line:0x%x, dummy_line:0x%x, frame_len_cur:0x%x, frame_len:0x%x",
		expsure_line, dummy_line, frame_len_cur, frame_len);

	ret_value = Sensor_WriteReg(0x0104, 0x01);
	if(frame_len_cur < frame_len){
		ret_value = Sensor_WriteReg(0x0341, frame_len & 0xff);
		ret_value = Sensor_WriteReg(0x0340, (frame_len >> 0x08) & 0xff);
	}

	ret_value = Sensor_WriteReg(0x203, expsure_line & 0xff);
	ret_value = Sensor_WriteReg(0x202, (expsure_line >> 0x08) & 0xff);

	if(frame_len_cur > frame_len){
		ret_value = Sensor_WriteReg(0x0341, frame_len & 0xff);
		ret_value = Sensor_WriteReg(0x0340, (frame_len >> 0x08) & 0xff);
	}

	ret_value = Sensor_WriteReg(0x0104, 0x00);
	return ret_value;
#endif
}

LOCAL unsigned long _imx179_write_gain(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t value=0x00;
	uint32_t real_gain = 0;
	return 0;
#if 0
	real_gain = ((param&0xf)+16)*(((param>>4)&0x01)+1)*(((param>>5)&0x01)+1);
	real_gain = real_gain*(((param>>6)&0x01)+1)*(((param>>7)&0x01)+1)*(((param>>8)&0x01)+1);
	real_gain = real_gain*(((param>>9)&0x01)+1)*(((param>>10)&0x01)+1)*(((param>>11)&0x01)+1);

	//value = real_gain & 0xff;
	value = (real_gain -16) * 256/real_gain;
	SENSOR_PRINT("SENSOR_IMX179: Bill@@@ rea_gain %d, %d", real_gain, real_gain/16);
	//ret_value = Sensor_WriteReg(0x0104, 0x01);
	ret_value = Sensor_WriteReg(0x0205, value);
	//ret_value = Sensor_WriteReg(0x0104, 0x00);
	SENSOR_PRINT("SENSOR_IMX179: Bill@@@ param: 0x%x, write_gain:0x%x, ret_value: %d", param, value, ret_value);
#endif
	return ret_value;
}

LOCAL unsigned long _imx179_write_af(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint8_t cmd_val[2] = {0x00};
	uint16_t slave_addr = 0;
	uint16_t cmd_len = 0;

	SENSOR_PRINT("SENSOR_IMX179: _write_af %d", param);
	slave_addr = DW9714_VCM_SLAVE_ADDR;
	cmd_val[0] = (param&0x3ff0)>>4;
	cmd_val[1] = ((param&0x0f)<<4)|0x05;
	cmd_len = 2;
	ret_value = Sensor_WriteI2C(slave_addr,(uint8_t*)&cmd_val[0], cmd_len);

	SENSOR_PRINT("SENSOR_IMX179: _write_af, ret =  %d, MSL:%x, LSL:%x\n", ret_value, cmd_val[0], cmd_val[1]);
	return ret_value;
}

LOCAL unsigned long _imx179_BeforeSnapshot(unsigned long param)
{
	uint8_t ret_l, ret_m, ret_h;
	uint16_t gain;
	uint32_t capture_exposure, preview_maxline;
	uint32_t capture_maxline, preview_exposure, preview_gain;
	uint32_t prv_linetime=s_imx179_Resolution_Trim_Tab[SENSOR_MODE_PREVIEW_ONE].line_time;
	uint32_t cap_linetime = s_imx179_Resolution_Trim_Tab[(param & 0xffff)].line_time;
	uint32_t frame_len = 0x00;

	param = param & 0xffff;
	SENSOR_PRINT("SENSOR_IMX179: BeforeSnapshot moe: %d",param);
	if (SENSOR_MODE_PREVIEW_ONE >= param){
		SENSOR_PRINT("SENSOR_IMX179: prvmode equal to capmode");
		return SENSOR_SUCCESS;
	}

	ret_h = (uint8_t) Sensor_ReadReg(0x202);
	ret_l = (uint8_t) Sensor_ReadReg(0x203);
	preview_exposure = (ret_h << 8) + (ret_l);

	_imx179_ReadGain(&gain);
	Sensor_SetMode((uint32_t)param);
	Sensor_SetMode_WaitDone();

	capture_exposure = preview_exposure * cap_linetime /prv_linetime;
	frame_len = Sensor_ReadReg(0x0341)&0xff;
	frame_len |= (Sensor_ReadReg(0x0340)<<0x08)&0xff00;
	if(cap_linetime == prv_linetime){
		SENSOR_PRINT("SENSOR_IMX179: cap_linetime equal with prv_linetime");
		//return SENSOR_SUCCESS;
	}

	SENSOR_PRINT("BeforeSnapshot: capture_exposure 0x%x, frame_len 0x%x, preview_exposure 0x%x",
		capture_exposure, frame_len, preview_exposure);

	Sensor_WriteReg(0x0104, 0x01);
	if(capture_exposure >= (frame_len - 4)){
		frame_len = capture_exposure+4;
		SENSOR_PRINT("BeforeSnapshot: frame_len 0x%x", frame_len);
		Sensor_WriteReg(0x0341, frame_len & 0xff);
		Sensor_WriteReg(0x0340, (frame_len >> 0x08) & 0xff);
	}
	Sensor_WriteReg(0x203, capture_exposure & 0xff);
	Sensor_WriteReg(0x202, (capture_exposure >> 0x08) & 0xff);
	Sensor_WriteReg(0x0104, 0x00);

	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, capture_exposure);
	return SENSOR_SUCCESS;
}

LOCAL unsigned long _imx179_after_snapshot(unsigned long param)
{
	SENSOR_PRINT("SENSOR_imx179: after_snapshot mode:%ld", param);
	Sensor_SetMode((uint32_t)param);
	return SENSOR_SUCCESS;
}

LOCAL unsigned long _imx179_flash(unsigned long param)
{
	SENSOR_PRINT("Start:param=%d", param);

	/* enable flash, disable in _imx179_BeforeSnapshot */
	g_flash_mode_en = param;
	Sensor_SetFlash(param);
	SENSOR_PRINT_HIGH("end");
	return SENSOR_SUCCESS;
}

LOCAL unsigned long _imx179_StreamOn(unsigned long param)
{
	SENSOR_PRINT("SENSOR_imx179: StreamOn");

	Sensor_WriteReg(0x0100, 0x01);

	return 0;
}

LOCAL unsigned long _imx179_StreamOff(unsigned long param)
{
	SENSOR_PRINT("SENSOR_imx179: StreamOff");

	Sensor_WriteReg(0x0100, 0x00);
	usleep(10*1000);

	return 0;
}

int _imx179_get_shutter(void)
{
	// read shutter, in number of line period
	int shutter;

	shutter = (Sensor_ReadReg(0x03500) & 0x0f);
	shutter = (shutter<<8) + Sensor_ReadReg(0x3501);
	shutter = (shutter<<4) + (Sensor_ReadReg(0x3502)>>4);

	return shutter;
}

int _imx179_set_shutter(int shutter)
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

int _imx179_get_gain16(void)
{
	// read gain, 16 = 1x
	int gain16;

	gain16 = Sensor_ReadReg(0x350a) & 0x03;
	gain16 = (gain16<<8) + Sensor_ReadReg(0x350b);

	return gain16;
}

int _imx179_set_gain16(int gain16)
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
	_imx179_set_gain16(capture_gain16);

	// write capture shutter
	/*if (capture_shutter > (capture_VTS - 4)) {
		capture_VTS = capture_shutter + 4;
		OV5640_set_VTS(capture_VTS);
	}*/
	_imx179_set_shutter(capture_shutter);
}

static unsigned long _imx179_SetEV(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	uint16_t value=0x00;
	uint32_t gain = s_imx179_gain;
	uint32_t ev = ext_ptr->param;

	SENSOR_PRINT("SENSOR: _ov5640_SetEV param: 0x%x", ext_ptr->param);

	switch(ev) {
	case SENSOR_HDR_EV_LEVE_0:
		_calculate_hdr_exposure(s_imx179_gain/4,s_capture_VTS,s_capture_shutter/2);
		break;
	case SENSOR_HDR_EV_LEVE_1:
		_calculate_hdr_exposure(s_imx179_gain,s_capture_VTS,s_capture_shutter);
		break;
	case SENSOR_HDR_EV_LEVE_2:
		_calculate_hdr_exposure(s_imx179_gain*3/2,s_capture_VTS,s_capture_shutter);
		break;
	default:
		break;
	}
	return rtn;
}
LOCAL unsigned long _imx179_ExtFunc(unsigned long ctl_param)
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
		rtn = _imx179_SetEV(ctl_param);
		break;
	default:
		break;
	}
	return rtn;
}
LOCAL int _imx179_get_VTS(void)
{
	// read VTS from register settings
	int VTS;

	VTS = Sensor_ReadReg(0x380e);//total vertical size[15:8] high byte

	VTS = (VTS<<8) + Sensor_ReadReg(0x380f);

	return VTS;
}

LOCAL int _imx179_set_VTS(int VTS)
{
	// write VTS to registers
	int temp;

	temp = VTS & 0xff;
	Sensor_WriteReg(0x380f, temp);

	temp = VTS>>8;
	Sensor_WriteReg(0x380e, temp);

	return 0;
}
LOCAL uint32_t _imx179_ReadGain(uint16_t *data)
{
	uint16_t value = 0x00;

	value = Sensor_ReadReg(0x0205);
	if(data){
		*data = value;
	}

	SENSOR_PRINT("_imx179_ReadGain: gain %d", value);
	return value;
}
