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
#include "sensor_imx214_raw_param_v3.c"
#else
#include "sensor_imx214_raw_param.c"
#endif


#define SENSOR_NAME			"imx214"
#define I2C_SLAVE_ADDR			0x20    /* 8bit slave address*/

#define imx214_PID_ADDR			0x0016
#define imx214_PID_VALUE			0x02
#define imx214_VER_ADDR			0x0017
#define imx214_VER_VALUE			0x14

/* sensor parameters begin */
/* effective sensor output image size */
#define SNAPSHOT_WIDTH			4208
#define SNAPSHOT_HEIGHT			3120
#define PREVIEW_WIDTH			2096
#define PREVIEW_HEIGHT			1552

/*Mipi output*/
#define LANE_NUM			4
#define RAW_BITS				10

#define SNAPSHOT_MIPI_PER_LANE_BPS	640
#define PREVIEW_MIPI_PER_LANE_BPS	640

/*line time unit: 0.1us*/
#define SNAPSHOT_LINE_TIME		5008
#define PREVIEW_LINE_TIME		5008

/* frame length*/
#define SNAPSHOT_FRAME_LENGTH		3406
#define PREVIEW_FRAME_LENGTH		1704

/* please ref your spec */
#define FRAME_OFFSET			5
#define SENSOR_MAX_GAIN			0xF0
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

static SENSOR_IOCTL_FUNC_TAB_T s_imx214_ioctl_func_tab;
struct sensor_raw_info *s_imx214_mipi_raw_info_ptr = &s_imx214_mipi_raw_info;

static const SENSOR_REG_T imx214_init_setting[] = {
	{0x0100,0x00},
	{0x0136,0x18},
	{0x0137,0x00},
	{0x0101,0x00},
	{0x0105,0x01},
	{0x0106,0x01},
	{0x4550,0x02},
	{0x4601,0x00},
	{0x4642,0x05},
	{0x6227,0x11},
	{0x6276,0x00},
	{0x900E,0x06},
	{0xA802,0x90},
	{0xA803,0x11},
	{0xA804,0x62},
	{0xA805,0x77},
	{0xA806,0xAE},
	{0xA807,0x34},
	{0xA808,0xAE},
	{0xA809,0x35},
	{0xA80A,0x62},
	{0xA80B,0x83},
	{0xAE33,0x00},
	{0x4174,0x00},
	{0x4175,0x11},
	{0x4612,0x29},
	{0x461B,0x12},
	{0x461F,0x06},
	{0x4635,0x07},
	{0x4637,0x30},
	{0x463F,0x18},
	{0x4641,0x0D},
	{0x465B,0x12},
	{0x465F,0x11},
	{0x4663,0x11},
	{0x4667,0x0F},
	{0x466F,0x0F},
	{0x470E,0x09},
	{0x4909,0xAB},
	{0x490B,0x95},
	{0x4915,0x5D},
	{0x4A5F,0xFF},
	{0x4A61,0xFF},
	{0x4A73,0x62},
	{0x4A85,0x00},
	{0x4A87,0xFF},
	{0x583C,0x04},
	{0x620E,0x04},
	{0x6EB2,0x01},
	{0x6EB3,0x00},
	{0x9300,0x02},
	{0x3001,0x07},
	{0x9344,0x03},
	{0x9706,0x10},
	{0x9707,0x03},
	{0x9708,0x03},
	{0x97BE,0x01},
	{0x97BF,0x01},
	{0x97C0,0x01},
	{0x9E04,0x01},
	{0x9E05,0x00},
	{0x9E0C,0x01},
	{0x9E0D,0x02},
	{0x9E24,0x00},
	{0x9E25,0x8C},
	{0x9E26,0x00},
	{0x9E27,0x94},
	{0x9E28,0x00},
	{0x9E29,0x96},
	{0x69DB,0x01},
	{0x6957,0x01},
	{0x6987,0x17},
	{0x698A,0x03},
	{0x698B,0x03},
	{0x0B8E,0x01},
	{0x0B8F,0x00},
	{0x0B90,0x01},
	{0x0B91,0x00},
	{0x0B92,0x01},
	{0x0B93,0x00},
	{0x0B94,0x01},
	{0x0B95,0x00},
	{0x6E50,0x00},
	{0x6E51,0x32},
	{0x9340,0x00},
	{0x9341,0x3C},
	{0x9342,0x03},
	{0x9343,0xFF},
};

	/*
	{0x0100,0x00},
	{0x0136,0x18},
	{0x0137,0x00},
	{0x0101,0x00},
	{0x0105,0x01},
	{0x0106,0x01},
	{0x4550,0x02},
	{0x4601,0x00},
	{0x4642,0x05},
	{0x6227,0x00},
	{0x6276,0x00},
	{0x900E,0x06},
	{0xA802,0x90},
	{0xA803,0x11},
	{0xA804,0x62},
	{0xA805,0x77},
	{0xA806,0xAE},
	{0xA807,0x34},
	{0xA808,0xAE},
	{0xA809,0x35},
	{0xA80A,0x62},
	{0xAE33,0x00},
	{0x4174,0x00},
	{0x4175,0x11},
	{0x4612,0x29},
	{0x461B,0x12},
	{0x461F,0x06},
	{0x4635,0x07},
	{0x4637,0x30},
	{0x463F,0x18},
	{0x4641,0x0D},
	{0x465B,0x12},
	{0x465F,0x11},
	{0x4663,0x11},
	{0x4667,0x0F},
	{0x466F,0x0F},
	{0x4635,0x07},
	{0x470E,0x09},
	{0x4909,0xAB},
	{0x490B,0x95},
	{0x4915,0x5D},
	{0x4A5F,0xFF},
	{0x4A61,0xFF},
	{0x4A73,0x62},
	{0x4A85,0x00},
	{0x4A87,0xFF},
	{0x583C,0x04},
	{0x620E,0x04},
	{0x6EB2,0x01},
	{0x6EB3,0x00},
	{0x9300,0x02},
	{0x0114,0x03},
	{0x0220,0x00},
	{0x0221,0x11},
	{0x0222,0x01},
	{0x0340,0x0D},
	{0x0341,0x4E},
	{0x0342,0x13},
	{0x0343,0x90},
	{0x0344,0x00},
	{0x0345,0x00},
	{0x0346,0x00},
	{0x0347,0x00},
	{0x0348,0x10},
	{0x0349,0x6F},
	{0x034A,0x0C},
	{0x034B,0x2F},
	{0x0381,0x01},
	{0x0383,0x01},
	{0x0385,0x01},
	{0x0387,0x01},
	{0x0900,0x00},
	{0x0901,0x00},
	{0x0902,0x00},
	{0x3000,0x35},
	{0x3054,0x01},
	{0x305C,0x11},
	{0x0112,0x0A},
	{0x0113,0x0A},
	{0x034C,0x10},
	{0x034D,0x70},
	{0x034E,0x0C},
	{0x034F,0x30},
	{0x0401,0x00},
	{0x0404,0x00},
	{0x0405,0x10},
	{0x0408,0x00},
	{0x0409,0x00},
	{0x040A,0x00},
	{0x040B,0x00},
	{0x040C,0x10},
	{0x040D,0x70},
	{0x040E,0x0C},
	{0x040F,0x30},
	{0x0301,0x05},
	{0x0303,0x02},
	{0x0305,0x03},
	{0x0306,0x00},
	{0x0307,0x50},
	{0x0309,0x0A},
	{0x030B,0x01},
	{0x0310,0x00},
	{0x0820,0x0A},
	{0x0821,0x00},
	{0x0822,0x00},
	{0x0823,0x00},
	{0x3A03,0x08},
	{0x3A04,0xC0},
	{0x3A05,0x02},
	{0x0B06,0x01},
	{0x30A2,0x00},
	{0x30B4,0x00},
	{0x3002,0xFF},
	{0x3011,0x00},
	{0x3013,0x01},
	{0x0204,0x00},
	{0x0205,0x00},
	{0x020E,0x01},
	{0x020F,0x00},
	{0x0210,0x01},
	{0x0211,0x00},
	{0x0212,0x01},
	{0x0213,0x00},
	{0x0214,0x01},
	{0x0215,0x00},
	{0x0216,0x00},
	{0x0217,0x00},
	{0x4170,0x00},
	{0x4171,0x10},
	{0x4176,0x00},
	{0x4177,0x3C},
	{0xAE20,0x04},
	{0xAE21,0x5C},
	{0x0138,0x01},
	{0x9300,0x02},
};
*/

static const SENSOR_REG_T imx214_snapshot_setting[] = {
	{0x0114,0x03},
	{0x0220,0x00},
	{0x0221,0x11},
	{0x0222,0x01},
	{0x0340,0x0D},
	{0x0341,0x4E},
	{0x0342,0x13},
	{0x0343,0x90},
	{0x0344,0x00},
	{0x0345,0x00},
	{0x0346,0x00},
	{0x0347,0x00},
	{0x0348,0x10},
	{0x0349,0x6F},
	{0x034A,0x0C},
	{0x034B,0x2F},
	{0x0381,0x01},
	{0x0383,0x01},
	{0x0385,0x01},
	{0x0387,0x01},
	{0x0900,0x00},
	{0x0901,0x00},
	{0x0902,0x00},
	{0x3000,0x35},
	{0x3054,0x01},
	{0x305C,0x11},
	{0x0112,0x0A},
	{0x0113,0x0A},
	{0x034C,0x10},
	{0x034D,0x70},
	{0x034E,0x0C},
	{0x034F,0x30},
	{0x0401,0x00},
	{0x0404,0x00},
	{0x0405,0x10},
	{0x0408,0x00},
	{0x0409,0x00},
	{0x040A,0x00},
	{0x040B,0x00},
	{0x040C,0x10},
	{0x040D,0x70},
	{0x040E,0x0C},
	{0x040F,0x30},
	{0x0301,0x05},
	{0x0303,0x02},
	{0x0305,0x03},
	{0x0306,0x00},
	{0x0307,0x50},
	{0x0309,0x0A},
	{0x030B,0x01},
	{0x0310,0x00},
	{0x0820,0x0A},
	{0x0821,0x00},
	{0x0822,0x00},
	{0x0823,0x00},
	{0x3A03,0x08},
	{0x3A04,0xC0},
	{0x3A05,0x02},
	{0x0B06,0x01},
	{0x30A2,0x00},
	{0x30B4,0x00},
	{0x3A02,0xFF},
	{0x3011,0x00},
	{0x3013,0x01},
	{0x0202,0x0D},
	{0x0203,0x44},
	{0x0224,0x01},
	{0x0225,0xF4},
	{0x0204,0x00},
	{0x0205,0x00},
	{0x020E,0x01},
	{0x020F,0x00},
	{0x0210,0x01},
	{0x0211,0x00},
	{0x0212,0x01},
	{0x0213,0x00},
	{0x0214,0x01},
	{0x0215,0x00},
	{0x0216,0x00},
	{0x0217,0x00},
	{0x4170,0x00},
	{0x4171,0x10},
	{0x4176,0x00},
	{0x4177,0x3C},
	{0xAE20,0x04},
	{0xAE21,0x5C},
};


static const SENSOR_REG_T imx214_preview_setting[] = {
	{0x0114,0x03},
	{0x0220,0x00},
	{0x0221,0x11},
	{0x0222,0x01},
	{0x0340,0x06},
	{0x0341,0xA8},
	{0x0342,0x13},
	{0x0343,0x90},
	{0x0344,0x00},
	{0x0345,0x08},
	{0x0346,0x00},
	{0x0347,0x08},
	{0x0348,0x10},
	{0x0349,0x67},
	{0x034A,0x0C},
	{0x034B,0x27},
	{0x0381,0x01},
	{0x0383,0x01},
	{0x0385,0x01},
	{0x0387,0x01},
	{0x0900,0x01},
	{0x0901,0x22},
	{0x0902,0x02},
	{0x3000,0x35},
	{0x3054,0x01},
	{0x305C,0x11},
	{0x0112,0x0A},
	{0x0113,0x0A},
	{0x034C,0x08},
	{0x034D,0x30},
	{0x034E,0x06},
	{0x034F,0x10},
	{0x0401,0x00},
	{0x0404,0x00},
	{0x0405,0x10},
	{0x0408,0x00},
	{0x0409,0x00},
	{0x040A,0x00},
	{0x040B,0x00},
	{0x040C,0x08},
	{0x040D,0x30},
	{0x040E,0x06},
	{0x040F,0x10},
	{0x0301,0x05},
	{0x0303,0x02},
	{0x0305,0x03},
	{0x0306,0x00},
	{0x0307,0x50},
	{0x0309,0x0A},
	{0x030B,0x01},
	{0x0310,0x00},
	{0x0820,0x0A},
	{0x0821,0x00},
	{0x0822,0x00},
	{0x0823,0x00},
	{0x3A03,0x06},
	{0x3A04,0x80},
	{0x3A05,0x04},
	{0x0B06,0x01},
	{0x30A2,0x00},
	{0x30B4,0x00},
	{0x3A02,0xFF},
	{0x3011,0x00},
	{0x3013,0x01},
	{0x0202,0x06},
	{0x0203,0x9E},
	{0x0224,0x01},
	{0x0225,0xF4},
	{0x0204,0x00},
	{0x0205,0x00},
	{0x020E,0x01},
	{0x020F,0x00},
	{0x0210,0x01},
	{0x0211,0x00},
	{0x0212,0x01},
	{0x0213,0x00},
	{0x0214,0x01},
	{0x0215,0x00},
	{0x0216,0x00},
	{0x0217,0x00},
	{0x4170,0x00},
	{0x4171,0x10},
	{0x4176,0x00},
	{0x4177,0x3C},
	{0xAE20,0x04},
	{0xAE21,0x5C},
};

static SENSOR_REG_TAB_INFO_T s_imx214_resolution_tab_raw[SENSOR_MODE_MAX] = {
	{ADDR_AND_LEN_OF_ARRAY(imx214_init_setting), 0, 0, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(imx214_preview_setting),
	 PREVIEW_WIDTH, PREVIEW_HEIGHT, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(imx214_snapshot_setting),
	 SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},
};

static SENSOR_TRIM_T s_imx214_resolution_trim_tab[SENSOR_MODE_MAX] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, PREVIEW_WIDTH, PREVIEW_HEIGHT,
	 PREVIEW_LINE_TIME, PREVIEW_MIPI_PER_LANE_BPS, PREVIEW_FRAME_LENGTH,
	 {0, 0, PREVIEW_WIDTH, PREVIEW_HEIGHT}},
	{0, 0, SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT,
	 SNAPSHOT_LINE_TIME, SNAPSHOT_MIPI_PER_LANE_BPS, SNAPSHOT_FRAME_LENGTH,
	 {0, 0, SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT}},
};

static const SENSOR_REG_T s_imx214_preview_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

static const SENSOR_REG_T s_imx214_capture_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

static SENSOR_VIDEO_INFO_T s_imx214_video_info[SENSOR_MODE_MAX] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{30, 30, 270, 90}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 (SENSOR_REG_T **) s_imx214_preview_size_video_tab},
	{{{2, 5, 338, 1000}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 (SENSOR_REG_T **) s_imx214_capture_size_video_tab},
};

/*==============================================================================
 * Description:
 * set video mode
 *
 *============================================================================*/
static uint32_t imx214_set_video_mode(uint32_t param)
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

	if (PNULL == s_imx214_video_info[mode].setting_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR) & s_imx214_video_info[mode].setting_ptr[param];
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
SENSOR_INFO_T g_imx214_mipi_raw_info = {
	/* salve i2c write address */
	(I2C_SLAVE_ADDR >> 1),
	/* salve i2c read address */
	(I2C_SLAVE_ADDR >> 1),
	/*bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit */
	SENSOR_I2C_REG_16BIT | SENSOR_I2C_VAL_8BIT | SENSOR_I2C_FREQ_400,
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
	{{imx214_PID_ADDR, imx214_PID_VALUE}
	 ,
	 {imx214_VER_ADDR, imx214_VER_VALUE}
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
	SENSOR_IMAGE_PATTERN_RAWRGB_R,
	/* point to resolution table information structure */
	s_imx214_resolution_tab_raw,
	/* point to ioctl function table */
	&s_imx214_ioctl_func_tab,
	/* information and table about Rawrgb sensor */
	&s_imx214_mipi_raw_info_ptr,
	/* extend information about sensor
	 * like &g_imx132_ext_info
	 */
	NULL,
	/* voltage of iovdd */
	SENSOR_AVDD_1800MV,
	/* voltage of dvdd */
	SENSOR_AVDD_1200MV,
	/* skip frame num before preview */
	1,
	/* skip frame num before capture */
	1,
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
	35,
	/* vertical view angle*/
	35
};

/*==============================================================================
 * Description:
 * get default frame length
 *
 *============================================================================*/
static uint32_t imx214_get_default_frame_length(uint32_t mode)
{
	return s_imx214_resolution_trim_tab[mode].frame_line;
}

/*==============================================================================
 * Description:
 * write group-hold on to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void imx214_group_hold_on(void)
{
	SENSOR_PRINT("E");

	//Sensor_WriteReg(0x0104, 0x01);
}

/*==============================================================================
 * Description:
 * write group-hold off to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void imx214_group_hold_off(void)
{
	SENSOR_PRINT("E");

	//Sensor_WriteReg(0x0104, 0x00);
}


/*==============================================================================
 * Description:
 * read gain from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t imx214_read_gain(void)
{
	uint16_t gain_l = 0;

	gain_l = Sensor_ReadReg(0x0205);

	return gain_l;
}

/*==============================================================================
 * Description:
 * write gain to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void imx214_write_gain(uint32_t gain)
{
	uint32_t sensor_again = 0;

	sensor_again=256-(256*SENSOR_BASE_GAIN/gain);
	sensor_again=sensor_again&0xFF;

	if (SENSOR_MAX_GAIN < sensor_again)
			sensor_again = SENSOR_MAX_GAIN;
	SENSOR_PRINT("sensor_again=0x%x",sensor_again);
	Sensor_WriteReg(0x0205, sensor_again);

	imx214_group_hold_off();

}

/*==============================================================================
 * Description:
 * read frame length from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t imx214_read_frame_length(void)
{
	uint16_t frame_len_h = 0;
	uint16_t frame_len_l = 0;

	frame_len_h = Sensor_ReadReg(0x0340) & 0xff;
	frame_len_l = Sensor_ReadReg(0x0341) & 0xff;

	return ((frame_len_h << 8) | frame_len_l);
}

/*==============================================================================
 * Description:
 * write frame length to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void imx214_write_frame_length(uint32_t frame_len)
{
	Sensor_WriteReg(0x0340, (frame_len >> 8) & 0xff);
	Sensor_WriteReg(0x0341, frame_len & 0xff);
}

/*==============================================================================
 * Description:
 * read shutter from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t imx214_read_shutter(void)
{
	uint16_t shutter_h = 0;
	uint16_t shutter_l = 0;

	shutter_h = Sensor_ReadReg(0x0202) & 0xff;
	shutter_l = Sensor_ReadReg(0x0203) & 0xff;

	return (shutter_h << 8) | shutter_l;
}

/*==============================================================================
 * Description:
 * write shutter to sensor registers
 * please pay attention to the frame length
 * please modify this function acording your spec
 *============================================================================*/
static void imx214_write_shutter(uint32_t shutter)
{
	Sensor_WriteReg(0x0202, (shutter >> 8) & 0xff);
	Sensor_WriteReg(0x0203, shutter & 0xff);
}

/*==============================================================================
 * Description:
 * write exposure to sensor registers and get current shutter
 * please pay attention to the frame length
 * please don't change this function if it's necessary
 *============================================================================*/
static uint16_t imx214_update_exposure(uint32_t shutter,uint32_t dummy_line)
{
	uint32_t dest_fr_len = 0;
	uint32_t cur_fr_len = 0;
	uint32_t fr_len = s_current_default_frame_length;

	imx214_group_hold_on();

	if (1 == SUPPORT_AUTO_FRAME_LENGTH)
		goto write_sensor_shutter;

	dest_fr_len = ((shutter + dummy_line+FRAME_OFFSET) > fr_len) ? (shutter +dummy_line+ FRAME_OFFSET) : fr_len;

	cur_fr_len = imx214_read_frame_length();

	if (shutter < SENSOR_MIN_SHUTTER)
		shutter = SENSOR_MIN_SHUTTER;

	if (dest_fr_len != cur_fr_len)
		imx214_write_frame_length(dest_fr_len);
write_sensor_shutter:
	/* write shutter to sensor registers */
	imx214_write_shutter(shutter);
	return shutter;
}

/*==============================================================================
 * Description:
 * sensor power on
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t imx214_power_on(uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_imx214_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_imx214_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_imx214_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_imx214_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_imx214_mipi_raw_info.reset_pulse_level;

	if (SENSOR_TRUE == power_on) {
		Sensor_PowerDown(power_down);
		Sensor_SetResetLevel(reset_level);
		usleep(10 * 1000);
		Sensor_SetVoltage(dvdd_val, avdd_val, iovdd_val);
		usleep(10 * 1000);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(10 * 1000);
		Sensor_SetMIPILevel(0);
		Sensor_PowerDown(!power_down);
		Sensor_SetResetLevel(!reset_level);

	} else {

		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_Reset(reset_level);
		Sensor_PowerDown(power_down);
	}
	SENSOR_PRINT("(1:on, 0:off): %d", power_on);
	return SENSOR_SUCCESS;
}


/*==============================================================================
 * Description:
 * identify sensor id
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t imx214_identify(uint32_t param)
{
	uint8_t pid_value = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("mipi raw identify");

	pid_value = Sensor_ReadReg(imx214_PID_ADDR);

	if (imx214_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(imx214_VER_ADDR);
		SENSOR_PRINT("Identify: PID = %x, VER = %x", pid_value, ver_value);
		if (imx214_VER_VALUE == ver_value) {
			ret_value = SENSOR_SUCCESS;
			SENSOR_PRINT_HIGH("this is imx214 sensor");
		} else {
			SENSOR_PRINT_HIGH("Identify this is %x%x sensor", pid_value, ver_value);
		}
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
static unsigned long imx214_get_resolution_trim_tab(uint32_t param)
{
	return (unsigned long) s_imx214_resolution_trim_tab;
}

/*==============================================================================
 * Description:
 * before snapshot
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t imx214_before_snapshot(uint32_t param)
{
	uint32_t cap_shutter = 0;
	uint32_t prv_shutter = 0;
	uint32_t gain = 0;
	uint32_t cap_gain = 0;
	uint32_t capture_mode = param & 0xffff;
	uint32_t preview_mode = (param >> 0x10) & 0xffff;

	uint32_t prv_linetime = s_imx214_resolution_trim_tab[preview_mode].line_time;
	uint32_t cap_linetime = s_imx214_resolution_trim_tab[capture_mode].line_time;

	s_current_default_frame_length = imx214_get_default_frame_length(capture_mode);
	SENSOR_PRINT("capture_mode = %d", capture_mode);

	if (preview_mode == capture_mode) {
		cap_shutter = s_sensor_ev_info.preview_shutter;
		cap_gain = s_sensor_ev_info.preview_gain;
		goto snapshot_info;
	}

	prv_shutter = s_sensor_ev_info.preview_shutter;	//imx132_read_shutter();
	gain = s_sensor_ev_info.preview_gain;	//imx132_read_gain();

	Sensor_SetMode(capture_mode);
	Sensor_SetMode_WaitDone();

	cap_shutter = prv_shutter * prv_linetime / cap_linetime * BINNING_FACTOR;

	while (gain >= (2 * SENSOR_BASE_GAIN)) {
		if (cap_shutter * 2 > s_current_default_frame_length)
			break;
		cap_shutter = cap_shutter * 2;
		gain = gain / 2;
	}

	cap_shutter = imx214_update_exposure(cap_shutter,0);
	cap_gain = gain;
	imx214_write_gain(cap_gain);
	SENSOR_PRINT("preview_shutter = 0x%x, preview_gain = 0x%x",
		     s_sensor_ev_info.preview_shutter, s_sensor_ev_info.preview_gain);

	SENSOR_PRINT("capture_shutter = 0x%x, capture_gain = 0x%x", cap_shutter, cap_gain);
snapshot_info:
	s_hdr_info.capture_shutter = cap_shutter; //imx132_read_shutter();
	s_hdr_info.capture_gain = cap_gain; //imx132_read_gain();
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
static uint32_t imx214_write_exposure(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t exposure_line = 0x00;
	uint16_t dummy_line = 0x00;
	uint16_t mode = 0x00;

	exposure_line = param & 0xffff;
	dummy_line = (param >> 0x10) & 0xfff; /*for cits frame rate test*/
	mode = (param >> 0x1c) & 0x0f;

	SENSOR_PRINT("current mode = %d, exposure_line = %d, dummy_line=%d", mode, exposure_line,dummy_line);
	s_current_default_frame_length = imx214_get_default_frame_length(mode);

	s_sensor_ev_info.preview_shutter = imx214_update_exposure(exposure_line,dummy_line);

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
static uint32_t imx214_write_gain_value(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint32_t real_gain = 0;

	real_gain = isp_to_real_gain(param);

	real_gain = real_gain * SENSOR_BASE_GAIN / ISP_BASE_GAIN;

	SENSOR_PRINT("real_gain = 0x%x", real_gain);

	s_sensor_ev_info.preview_gain = real_gain;
	imx214_write_gain(real_gain);

	return ret_value;
}

/*==============================================================================
 * Description:
 * increase gain or shutter for hdr
 *
 *============================================================================*/
static void imx214_increase_hdr_exposure(uint8_t ev_multiplier)
{
	uint32_t shutter_multiply = s_hdr_info.capture_max_shutter / s_hdr_info.capture_shutter;
	uint32_t gain = 0;

	if (0 == shutter_multiply)
		shutter_multiply = 1;

	if (shutter_multiply >= ev_multiplier) {
		imx214_update_exposure(s_hdr_info.capture_shutter * ev_multiplier,0);
		imx214_write_gain(s_hdr_info.capture_gain);
	} else {
		gain = s_hdr_info.capture_gain * ev_multiplier / shutter_multiply;
		imx214_update_exposure(s_hdr_info.capture_shutter * shutter_multiply,0);
		imx214_write_gain(gain);
	}
}

/*==============================================================================
 * Description:
 * decrease gain or shutter for hdr
 *
 *============================================================================*/
static void imx214_decrease_hdr_exposure(uint8_t ev_divisor)
{
	uint16_t gain_multiply = 0;
	uint32_t shutter = 0;
	gain_multiply = s_hdr_info.capture_gain / SENSOR_BASE_GAIN;

	if (gain_multiply >= ev_divisor) {
		imx214_update_exposure(s_hdr_info.capture_shutter,0);
		imx214_write_gain(s_hdr_info.capture_gain / ev_divisor);

	} else {
		shutter = s_hdr_info.capture_shutter * gain_multiply / ev_divisor;
		imx214_update_exposure(shutter,0);
		imx214_write_gain(s_hdr_info.capture_gain / gain_multiply);
	}
}

/*==============================================================================
 * Description:
 * set hdr ev
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t imx214_set_hdr_ev(unsigned long param)
{
	uint32_t ret = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	uint32_t ev = ext_ptr->param;
	uint8_t ev_divisor, ev_multiplier;

	switch (ev) {
	case SENSOR_HDR_EV_LEVE_0:
		ev_divisor = 2;
		imx214_decrease_hdr_exposure(ev_divisor);
		break;
	case SENSOR_HDR_EV_LEVE_1:
		ev_multiplier = 2;
		imx214_increase_hdr_exposure(ev_multiplier);
		break;
	case SENSOR_HDR_EV_LEVE_2:
		ev_multiplier = 1;
		imx214_increase_hdr_exposure(ev_multiplier);
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
static uint32_t imx214_ext_func(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	SENSOR_PRINT("ext_ptr->cmd: %d", ext_ptr->cmd);
	switch (ext_ptr->cmd) {
	case SENSOR_EXT_EV:
		rtn = imx214_set_hdr_ev(param);
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
static uint32_t imx214_stream_on(uint32_t param)
{
	SENSOR_PRINT("E");

	Sensor_WriteReg(0x0100, 0x01);
	/*delay*/
	usleep(10 * 1000);

	return 0;
}

/*==============================================================================
 * Description:
 * mipi stream off
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t imx214_stream_off(uint32_t param)
{
	SENSOR_PRINT("E");

	Sensor_WriteReg(0x0100, 0x00);
	/*delay*/
	usleep(10 * 1000);

	return 0;
}

/*==============================================================================
 * Description:
 * all ioctl functoins
 * you can add functions reference SENSOR_IOCTL_FUNC_TAB_T from sensor_drv_u.h
 *
 * add ioctl functions like this:
 * .power = imx132_power_on,
 *============================================================================*/
static SENSOR_IOCTL_FUNC_TAB_T s_imx214_ioctl_func_tab = {
	.power = imx214_power_on,
	.identify = imx214_identify,
	.get_trim = imx214_get_resolution_trim_tab,
	.before_snapshort = imx214_before_snapshot,
	.write_ae_value = imx214_write_exposure,
	.write_gain_value = imx214_write_gain_value,
	.set_focus = imx214_ext_func,
	//.set_video_mode = imx132_set_video_mode,
	.stream_on = imx214_stream_on,
	.stream_off = imx214_stream_off,

	//.group_hold_on = imx132_group_hold_on,
	//.group_hold_of = imx132_group_hold_off,
};
