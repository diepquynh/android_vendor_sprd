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
 * V5.0
 */
 /*History
 *Date                  Modification                                 Reason
 *
 */

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"

#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
#include "ov5675_parameters/sensor_ov5675_raw_param_main.c"
#else
#include "sensor_ov5675_raw_param.c"
#endif

#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
#include "../af_et9714.h"
#endif
#include "packet_convert.c"

//#define CAMERA_IMAGE_180

#define SENSOR_NAME			"ov5675"
#define I2C_SLAVE_ADDR			0x6c    /* 8bit slave address*/

#define ov5675_PID_ADDR			0x300B
#define ov5675_PID_VALUE		0x56
#define ov5675_VER_ADDR			0x300C
#define ov5675_VER_VALUE		0x75

/* sensor parameters begin */
/* effective sensor output image size */
#define SNAPSHOT_WIDTH			2592 
#define SNAPSHOT_HEIGHT			1944
#define PREVIEW_WIDTH			1296
#define PREVIEW_HEIGHT			972

/*Raw Trim parameters*/
#define SNAPSHOT_TRIM_X			0
#define SNAPSHOT_TRIM_Y			0
#define SNAPSHOT_TRIM_W			2592
#define SNAPSHOT_TRIM_H			1944
#define PREVIEW_TRIM_X			0
#define PREVIEW_TRIM_Y			0
#define PREVIEW_TRIM_W			1296
#define PREVIEW_TRIM_H			972

/*Mipi output*/
#define LANE_NUM			2
#define RAW_BITS				10

#define SNAPSHOT_MIPI_PER_LANE_BPS	900  /* 2*Mipi clk */
#define PREVIEW_MIPI_PER_LANE_BPS	900  /* 2*Mipi clk */

/*line time unit: 0.1us*/
#define SNAPSHOT_LINE_TIME		167
#define PREVIEW_LINE_TIME		167

/* frame length*/
#define SNAPSHOT_FRAME_LENGTH		2000
#define PREVIEW_FRAME_LENGTH		2000

/* please ref your spec */
#define FRAME_OFFSET			4
#define SENSOR_MAX_GAIN			0x800
#define SENSOR_BASE_GAIN		0x80
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

/*delay 1 frame to write sensor gain*/
//#define GAIN_DELAY_1_FRAME

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
static struct hdr_info_t s_hdr_info={
	2000000/SNAPSHOT_LINE_TIME,/*min 5fps*/
	SNAPSHOT_FRAME_LENGTH-FRAME_OFFSET,
	SENSOR_BASE_GAIN
	};
static uint32_t s_current_default_frame_length=PREVIEW_FRAME_LENGTH;
static struct sensor_ev_info_t s_sensor_ev_info={
	PREVIEW_FRAME_LENGTH-FRAME_OFFSET,
	SENSOR_BASE_GAIN
	};

#define FEATURE_OTP    /*OTP function switch*/

#ifdef FEATURE_OTP
#include "sensor_ov5675_darling_otp.c"
static struct otp_info_t *s_ov5675_otp_info_ptr=&s_ov5675_darling_otp_info;
static struct raw_param_info_tab *s_ov5675_raw_param_tab_ptr=&s_ov5675_darling_raw_param_tab;  /*otp function interface*/
#endif

static SENSOR_IOCTL_FUNC_TAB_T s_ov5675_ioctl_func_tab;
struct sensor_raw_info *s_ov5675_mipi_raw_info_ptr = &s_ov5675_mipi_raw_info;
static EXIF_SPEC_PIC_TAKING_COND_T s_ov5675_exif_info;

/*//delay 200ms
{SENSOR_WRITE_DELAY, 200},
*/

static const SENSOR_REG_T ov5675_init_setting[] = {
	{0x0100,0x00},
	{0x0103,0x01},
	{0x0300,0x05},
	{0x0302,0x96},
	{0x0303,0x00},
	{0x030d,0x1e},
	{0x3002,0x21},
	{0x3107,0x01},//0x23->0x01
	{0x3501,0x20},
	{0x3503,0x0c},
	{0x3508,0x03},
	{0x3509,0x00},
	{0x3600,0x66},
	{0x3602,0x30},
	{0x3610,0xa5},
	{0x3612,0x93},
	{0x3620,0x80},
	{0x3642,0x0e},
	{0x3661,0x00},
	{0x3662,0x10},
	{0x3664,0xf3},
	{0x3665,0x9e},
	{0x3667,0xa5},
	{0x366e,0x55},
	{0x366f,0x55},
	{0x3670,0x11},
	{0x3671,0x11},
	{0x3672,0x11},
	{0x3673,0x11},
	{0x3714,0x24},
	{0x371a,0x3e},
	{0x3733,0x10},
	{0x3734,0x00},
	{0x373d,0x24},
	{0x3764,0x20},
	{0x3765,0x20},
	{0x3766,0x12},
	{0x37a1,0x14},
	{0x37a8,0x1c},
	{0x37ab,0x0f},
	{0x37c2,0x04},
	{0x37cb,0x00},
	{0x37cc,0x00},
	{0x37cd,0x00},
	{0x37ce,0x00},
	{0x37d8,0x02},
	{0x37d9,0x08},
	{0x37dc,0x04},
	{0x3800,0x00},
	{0x3801,0x00},
	{0x3802,0x00},
	{0x3803,0x04},
	{0x3804,0x0a},
	{0x3805,0x3f},
	{0x3806,0x07},
	{0x3807,0xb3},
	{0x3808,0x0a},
	{0x3809,0x20},
	{0x380a,0x07},
	{0x380b,0x98},
	{0x380c,0x02},
	{0x380d,0xee},
	{0x380e,0x07},
	{0x380f,0xd0},
	{0x3811,0x10},
	{0x3813,0x0c},
	{0x3814,0x01},
	{0x3815,0x01},
	{0x3816,0x01},
	{0x3817,0x01},
	{0x381e,0x02},
	{0x3820,0x88},
	{0x3821,0x01},
	{0x3832,0x04},
	{0x3c80,0x08},//0x01->0x08
	{0x3c82,0x00},
	{0x3c83,0xb1},//0xc8->0xb1
	{0x3c8c,0x10},//0x0f->0x10
	{0x3c8d,0x00},//0xa0->0x00
	{0x3c90,0x00},//0x07->0x00
	{0x3c91,0x00},
	{0x3c92,0x00},
	{0x3c93,0x00},
	{0x3c94,0x00},//0xd0->0x00
	{0x3c95,0x00},//0x50->0x00
	{0x3c96,0x00},//0x35->0x00
	{0x3c97,0x00},
	{0x4001,0xe0},
	{0x4008,0x02},
	{0x4009,0x0d},
	{0x400f,0x80},
	{0x4013,0x02},
	{0x4040,0x00},
	{0x4041,0x07},
	{0x404c,0x50},
	{0x404e,0x20},
	{0x4500,0x06},
	{0x4503,0x00},
	{0x450a,0x04},
	{0x4809,0x04},
	{0x480c,0x12},
	{0x4819,0x70},
	{0x4825,0x32},
	{0x4826,0x32},
	{0x482a,0x06},
	{0x4833,0x08},
	{0x4837,0x0d},
	{0x5000,0x77},
	{0x5b00,0x01},
	{0x5b01,0x10},
	{0x5b02,0x01},
	{0x5b03,0xdb},
	{0x5b05,0x6c},
	{0x5e10,0xfc},
	{0x3500,0x00},
	{0x3501,0x3E},// ;max expo= ([380e,380f]-4)/2.
	{0x3502,0x60},
#ifdef CONFIG_CAMERA_ISP_AE_VERSION_V1
	{0x3503,0x78},// ;Bit[6:4]=1, AE1 do not delay frame
#else
	{0x3503,0x08},// ;[2]=0 real gain
#endif
	{0x3508,0x04},
	{0x3509,0x00},// ;[3508,3509]=0x0080 is 1xgain
	{0x3832,0x48},// ; [7:4]vsync_width ; R3002[5] p_fsin_oen
	{0x3c90,0x00},// ;MIPI Continuous mode (07 Gated mode)
	{0x5780,0x3e},// ;strong DPC
	{0x5781,0x0f},
	{0x5782,0x44},
	{0x5783,0x02},
	{0x5784,0x01},
	{0x5785,0x01},
	{0x5786,0x00},
	{0x5787,0x04},
	{0x5788,0x02},
	{0x5789,0x0f},
	{0x578a,0xfd},
	{0x578b,0xf5},
	{0x578c,0xf5},
	{0x578d,0x03},
	{0x578e,0x08},
	{0x578f,0x0c},
	{0x5790,0x08},
	{0x5791,0x06},
	{0x5792,0x00},
	{0x5793,0x52},
	{0x5794,0xa3},
	{0x4003,0x40},//;BLC target
};

static const SENSOR_REG_T ov5675_preview_setting[] = {
	{0x3662,0x08},
	{0x3714,0x28}, 
	{0x371a,0x3e}, 
	{0x37c2,0x14},   
	{0x37d9,0x04},  
	{0x3800,0x00},  
	{0x3801,0x00},  
	{0x3802,0x00},  
	{0x3803,0x00},  
	{0x3804,0x0a},  
	{0x3805,0x3f},  
	{0x3806,0x07},  
	{0x3807,0xb7},  
	{0x3808,0x05},  
	{0x3809,0x10},  
	{0x380a,0x03},  
	{0x380b,0xcc},  
	{0x380c,0x02},
	{0x380d,0xee},
	{0x380e,0x07},
	{0x380f,0xd0}, 
	{0x3811,0x08},  
	{0x3813,0x08},  
	{0x3814,0x03},  
	{0x3815,0x01},  
	{0x3816,0x03}, 
	#ifdef CAMERA_IMAGE_180
	{0x3820,0xb3},  
	{0x373d,0x26},  
	#else
	{0x3820,0x8b}, 
	{0x373d,0x24},  
	#endif
	{0x3821,0x01},   
	{0x4008,0x00},
	{0x4009,0x07},  
	{0x4041,0x03},
};

static const SENSOR_REG_T ov5675_snapshot_setting[] = {
	{0x3662,0x10}, 
	{0x3714,0x24},  
	{0x371a,0x3e}, 
	{0x37c2,0x04},   
	{0x37d9,0x08},  
	{0x3800,0x00},  
	{0x3801,0x00},  
	{0x3802,0x00},  
	{0x3803,0x04},  
	{0x3804,0x0a},  
	{0x3805,0x3f},  
	{0x3806,0x07},  
	{0x3807,0xb3},  
	{0x3808,0x0a},  
	{0x3809,0x20},  
	{0x380a,0x07},  
	{0x380b,0x98},  
	{0x380c,0x02},
	{0x380d,0xee},
	{0x380e,0x07},
	{0x380f,0xd0},
	{0x3811,0x10},  
	{0x3813,0x0c},  
	{0x3814,0x01},  
	{0x3815,0x01},  
	{0x3816,0x01}, 
#ifdef CAMERA_IMAGE_180
	{0x3820,0xb0},  
	{0x373d,0x26},  
#else
	{0x3820,0x88}, 
	{0x373d,0x24},  
#endif
	{0x3821,0x01},   
	{0x4008,0x02},  
	{0x4009,0x0d},  
	{0x4041,0x07}, 
};

static SENSOR_REG_TAB_INFO_T s_ov5675_resolution_tab_raw[SENSOR_MODE_MAX] = {
	{ADDR_AND_LEN_OF_ARRAY(ov5675_init_setting), 0, 0, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},
	/*{ADDR_AND_LEN_OF_ARRAY(ov5675_preview_setting),
	 PREVIEW_WIDTH, PREVIEW_HEIGHT, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},*/
	{ADDR_AND_LEN_OF_ARRAY(ov5675_snapshot_setting),
	 SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT, EX_MCLK,
	 SENSOR_IMAGE_FORMAT_RAW},
};

static SENSOR_TRIM_T s_ov5675_resolution_trim_tab[SENSOR_MODE_MAX] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	/*{PREVIEW_TRIM_X, PREVIEW_TRIM_Y, PREVIEW_TRIM_W, PREVIEW_TRIM_H,
	 PREVIEW_LINE_TIME, PREVIEW_MIPI_PER_LANE_BPS, PREVIEW_FRAME_LENGTH,
	 {0, 0, PREVIEW_TRIM_W, PREVIEW_TRIM_H}},*/
	{SNAPSHOT_TRIM_X, SNAPSHOT_TRIM_Y, SNAPSHOT_TRIM_W, SNAPSHOT_TRIM_H,
	 SNAPSHOT_LINE_TIME, SNAPSHOT_MIPI_PER_LANE_BPS, SNAPSHOT_FRAME_LENGTH,
	 {0, 0, SNAPSHOT_TRIM_W, SNAPSHOT_TRIM_H}},
};

static const SENSOR_REG_T s_ov5675_preview_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

static const SENSOR_REG_T s_ov5675_capture_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

static SENSOR_VIDEO_INFO_T s_ov5675_video_info[SENSOR_MODE_MAX] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{30, 30, 270, 90}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 (SENSOR_REG_T **) s_ov5675_preview_size_video_tab},
	{{{2, 5, 338, 1000}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
	 (SENSOR_REG_T **) s_ov5675_capture_size_video_tab},
};

/*==============================================================================
 * Description:
 * set video mode
 *
 *============================================================================*/
static uint32_t ov5675_set_video_mode(uint32_t param)
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

	if (PNULL == s_ov5675_video_info[mode].setting_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR) & s_ov5675_video_info[mode].setting_ptr[param];
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
SENSOR_INFO_T g_ov5675_mipi_raw_info = {
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
	{{ov5675_PID_ADDR, ov5675_PID_VALUE}
	 ,
	 {ov5675_VER_ADDR, ov5675_VER_VALUE}
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
	s_ov5675_resolution_tab_raw,
	/* point to ioctl function table */
	&s_ov5675_ioctl_func_tab,
	/* information and table about Rawrgb sensor */
	&s_ov5675_mipi_raw_info_ptr,
	/* extend information about sensor
	 * like &g_ov5675_ext_info
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
	65,
	/* vertical view angle*/
	60
};

#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)

#define param_update(x1,x2) sprintf(name,"/data/misc/media/ov5675_%s.bin",x1);\
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
				}else{\
					SENSOR_PRINT("access %s failure",name);\
				}\
				memset(name,0,sizeof(name))

static uint32_t ov5675_InitRawTuneInfo(void)
{
	uint32_t rtn=0x00;

	isp_raw_para_update_from_file(&g_ov5675_mipi_raw_info,0);

	struct sensor_raw_info* raw_sensor_ptr=s_ov5675_mipi_raw_info_ptr;
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
					#include "ov5675_parameters/NR/pwd_param.h"
				};

				param_update("pwd_param",pwd_param);

				block->param_ptr = pwd_param;
			}
			break;

		case	ISP_BLK_BPC_V1: {
				/* modify block data */
				struct sensor_bpc_param_v1* block = (struct sensor_bpc_param_v1*)data;

				static struct sensor_bpc_level bpc_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "ov5675_parameters/NR/bpc_param.h"
				};

				param_update("bpc_param",bpc_param);

				block->param_ptr = bpc_param;
			}
			break;

		case	ISP_BLK_BL_NR_V1: {
				/* modify block data */
				struct sensor_bdn_param* block = (struct sensor_bdn_param*)data;

				static struct sensor_bdn_level bdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "ov5675_parameters/NR/bdn_param.h"
				};

				param_update("bdn_param",bdn_param);

				block->param_ptr = bdn_param;
			}
			break;

		case	ISP_BLK_GRGB_V1: {
				/* modify block data */
				struct sensor_grgb_v1_param* block = (struct sensor_grgb_v1_param*)data;
				static struct sensor_grgb_v1_level grgb_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "ov5675_parameters/NR/grgb_param.h"
				};

				param_update("grgb_param",grgb_param);

				block->param_ptr = grgb_param;

			}
			break;

		case	ISP_BLK_NLM: {
				/* modify block data */
				struct sensor_nlm_param* block = (struct sensor_nlm_param*)data;

				static struct sensor_nlm_level nlm_param[32] = {
					#include "ov5675_parameters/NR/nlm_param.h"
				};

				param_update("nlm_param",nlm_param);

				static struct sensor_vst_level vst_param[32] = {
					#include "ov5675_parameters/NR/vst_param.h"
				};

				param_update("vst_param",vst_param);

				static struct sensor_ivst_level ivst_param[32] = {
					#include "ov5675_parameters/NR/ivst_param.h"
				};

				param_update("ivst_param",ivst_param);

				static struct sensor_flat_offset_level flat_offset_param[32] = {
					#include "ov5675_parameters/NR/flat_offset_param.h"
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
					#include "ov5675_parameters/NR/cfae_param.h"
				};

				param_update("cfae_param",cfae_param);

				block->param_ptr = cfae_param;
			}
			break;

		case	ISP_BLK_RGB_PRECDN: {
				/* modify block data */
				struct sensor_rgb_precdn_param* block = (struct sensor_rgb_precdn_param*)data;

				static struct sensor_rgb_precdn_level precdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "ov5675_parameters/NR/rgb_precdn_param.h"
				};

				param_update("rgb_precdn_param",precdn_param);

				block->param_ptr = precdn_param;
			}
			break;

		case	ISP_BLK_YUV_PRECDN: {
				/* modify block data */
				struct sensor_yuv_precdn_param* block = (struct sensor_yuv_precdn_param*)data;

				static struct sensor_yuv_precdn_level yuv_precdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "ov5675_parameters/NR/yuv_precdn_param.h"
				};

				param_update("yuv_precdn_param",yuv_precdn_param);

				block->param_ptr = yuv_precdn_param;
			}
			break;

		case	ISP_BLK_PREF_V1: {
				/* modify block data */
				struct sensor_prfy_param* block = (struct sensor_prfy_param*)data;

				static struct sensor_prfy_level prfy_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "ov5675_parameters/NR/prfy_param.h"
				};

				param_update("prfy_param",prfy_param);

				block->param_ptr = prfy_param;
			}
			break;

		case	ISP_BLK_UV_CDN: {
				/* modify block data */
				struct sensor_uv_cdn_param* block = (struct sensor_uv_cdn_param*)data;

				static struct sensor_uv_cdn_level uv_cdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "ov5675_parameters/NR/yuv_cdn_param.h"
				};

				param_update("yuv_cdn_param",uv_cdn_param);

				block->param_ptr = uv_cdn_param;
			}
			break;

		case	ISP_BLK_EDGE_V1: {
				/* modify block data */
				struct sensor_ee_param* block = (struct sensor_ee_param*)data;

				static struct sensor_ee_level edge_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "ov5675_parameters/NR/edge_param.h"
				};

				param_update("edge_param",edge_param);

				block->param_ptr = edge_param;
			}
			break;

		case	ISP_BLK_UV_POSTCDN: {
				/* modify block data */
				struct sensor_uv_postcdn_param* block = (struct sensor_uv_postcdn_param*)data;

				static struct sensor_uv_postcdn_level uv_postcdn_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "ov5675_parameters/NR/yuv_postcdn_param.h"
				};

				param_update("yuv_postcdn_param",uv_postcdn_param);

				block->param_ptr = uv_postcdn_param;
			}
			break;

		case	ISP_BLK_IIRCNR_IIR: {
				/* modify block data */
				struct sensor_iircnr_param* block = (struct sensor_iircnr_param*)data;

				static struct sensor_iircnr_level iir_cnr_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "ov5675_parameters/NR/iircnr_param.h"
				};

				param_update("iircnr_param",iir_cnr_param);

				block->param_ptr = iir_cnr_param;
			}
			break;

		case	ISP_BLK_IIRCNR_YRANDOM: {
				/* modify block data */
				struct sensor_iircnr_yrandom_param* block = (struct sensor_iircnr_yrandom_param*)data;
				static struct sensor_iircnr_yrandom_level iir_yrandom_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "ov5675_parameters/NR/iir_yrandom_param.h"
				};

				param_update("iir_yrandom_param",iir_yrandom_param);

				block->param_ptr = iir_yrandom_param;
			}
			break;

		case  ISP_BLK_UVDIV_V1: {
				/* modify block data */
				struct sensor_cce_uvdiv_param_v1* block = (struct sensor_cce_uvdiv_param_v1*)data;

				static struct sensor_cce_uvdiv_level cce_uvdiv_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "ov5675_parameters/NR/cce_uv_param.h"
				};

				param_update("cce_uv_param",cce_uvdiv_param);

				block->param_ptr = cce_uvdiv_param;
			}
			break;
		case ISP_BLK_YIQ_AFM:{
			/* modify block data */
			struct sensor_y_afm_param *block = (struct sensor_y_afm_param*)data;

			static struct sensor_y_afm_level y_afm_param[SENSOR_SMART_LEVEL_NUM] = {
					#include "ov5675_parameters/NR/y_afm_param.h"
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
#endif

/*==============================================================================
 * Description:
 * get default frame length
 *
 *============================================================================*/
static uint32_t ov5675_get_default_frame_length(uint32_t mode)
{
	return s_ov5675_resolution_trim_tab[mode].frame_line;
}

/*==============================================================================
 * Description:
 * write group-hold on to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov5675_group_hold_on(void)
{
	SENSOR_PRINT("E");

	//Sensor_WriteReg(0x3208, 0x001);
}

/*==============================================================================
 * Description:
 * write group-hold off to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov5675_group_hold_off(void)
{
	SENSOR_PRINT("E");

	//Sensor_WriteReg(0x3208, 0x10);
	//Sensor_WriteReg(0x3208, 0xa0);
}


/*==============================================================================
 * Description:
 * read gain from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t ov5675_read_gain(void)
{
	return s_sensor_ev_info.preview_gain;
}

/*==============================================================================
 * Description:
 * write gain to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov5675_write_gain(uint32_t gain)
{ 
	if (SENSOR_MAX_GAIN < gain)
			gain = SENSOR_MAX_GAIN;
	uint16_t value=0;

	Sensor_WriteReg(0x3208, 0x00);
	
	value=(gain >> 8) & 0x1f;
	Sensor_WriteReg(0x3508,value );
	//SENSOR_PRINT("0x3508: write=0x%x, read=0x%x",value,Sensor_ReadReg(0x3508));

	value=gain& 0xff;
	Sensor_WriteReg(0x3509,value);
	//SENSOR_PRINT("0x3509: write=0x%x, read=0x%x",value,Sensor_ReadReg(0x3509));

	Sensor_WriteReg(0x3208, 0x10);
	Sensor_WriteReg(0x3208, 0xa0);
	
	ov5675_group_hold_off();
	
	/*SENSOR_PRINT("0x3500: read=0x%x",Sensor_ReadReg(0x3500));
	SENSOR_PRINT("0x3501: read=0x%x",Sensor_ReadReg(0x3501));
	SENSOR_PRINT("0x3502: read=0x%x",Sensor_ReadReg(0x3502));
	SENSOR_PRINT("0x3508: read=0x%x",Sensor_ReadReg(0x3508));
	SENSOR_PRINT("0x3509: read=0x%x",Sensor_ReadReg(0x3509));*/

}

/*==============================================================================
 * Description:
 * read frame length from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint16_t ov5675_read_frame_length(void)
{
	uint16_t frame_len_h = 0;
	uint16_t frame_len_l = 0;

	frame_len_h = Sensor_ReadReg(0x380E) & 0xff;
	frame_len_l = Sensor_ReadReg(0x380F) & 0xff;

	return ((frame_len_h << 8) | frame_len_l);
}

/*==============================================================================
 * Description:
 * write frame length to sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static void ov5675_write_frame_length(uint32_t frame_len)
{
	Sensor_WriteReg(0x380E, (frame_len >> 8) & 0xff);
	Sensor_WriteReg(0x380F, frame_len & 0xff);
}

/*==============================================================================
 * Description:
 * read shutter from sensor registers
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t ov5675_read_shutter(void)
{
	return s_sensor_ev_info.preview_shutter;
}


/*==============================================================================
 * Description:
 * write shutter to sensor registers
 * please pay attention to the frame length
 * please modify this function acording your spec
 *============================================================================*/
static void ov5675_write_shutter(uint32_t shutter)
{
	uint16_t value=0x00;
	uint16_t ov_shutter=(shutter>>1);
	
	value=(ov_shutter<<0x04)&0xff;
	Sensor_WriteReg(0x3502,value);
	//SENSOR_PRINT("0x3502: write=0x%x, read=0x%x",value,Sensor_ReadReg(0x3502));
	value=(ov_shutter>>0x04)&0xff;
	Sensor_WriteReg(0x3501,value);
	
	//SENSOR_PRINT("0x3501: write=0x%x, read=0x%x",value,Sensor_ReadReg(0x3501));
	value=(ov_shutter>>0x0c)&0x0f;
	Sensor_WriteReg(0x3500,value);
	
	//SENSOR_PRINT("0x3500: write=0x%x, read=0x%x",value,Sensor_ReadReg(0x3500));
}


/*==============================================================================
 * Description:
 * write exposure to sensor registers and get current shutter
 * please pay attention to the frame length
 * please don't change this function if it's necessary
 *============================================================================*/
static uint16_t ov5675_update_exposure(uint32_t shutter,uint32_t dummy_line)
{
	uint32_t dest_fr_len = 0;
	uint32_t cur_fr_len = 0;
	uint32_t fr_len = s_current_default_frame_length;

	ov5675_group_hold_on();

	if (1 == SUPPORT_AUTO_FRAME_LENGTH)
		goto write_sensor_shutter;

	dest_fr_len = ((shutter + dummy_line+FRAME_OFFSET) > fr_len) ? (shutter +dummy_line+ FRAME_OFFSET) : fr_len;

	cur_fr_len = ov5675_read_frame_length();

	if (shutter < SENSOR_MIN_SHUTTER)
		shutter = SENSOR_MIN_SHUTTER;

	if (dest_fr_len != cur_fr_len)
		ov5675_write_frame_length(dest_fr_len);
write_sensor_shutter:
	/* write shutter to sensor registers */
	ov5675_write_shutter(shutter);

	#ifdef GAIN_DELAY_1_FRAME
	usleep(dest_fr_len*PREVIEW_LINE_TIME/10);
	#endif
	
	return shutter;
}

/*==============================================================================
 * Description:
 * sensor power on
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t ov5675_power_on(uint32_t power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_ov5675_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_ov5675_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_ov5675_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_ov5675_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_ov5675_mipi_raw_info.reset_pulse_level;

	if (SENSOR_TRUE == power_on) {
		Sensor_PowerDown(power_down);
		Sensor_SetResetLevel(reset_level);
		usleep(10 * 1000);
		Sensor_SetAvddVoltage(avdd_val);
		Sensor_SetDvddVoltage(dvdd_val);
		Sensor_SetIovddVoltage(iovdd_val);
		Sensor_PowerDown(!power_down);
		Sensor_SetResetLevel(!reset_level);
		usleep(10 * 1000);
		Sensor_SetMCLK(EX_MCLK);

		#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		//Sensor_Set_AfEnableLevel(1);
		usleep(5 * 1000);
		et9714_init(2);
		#else
		//Sensor_Set_AfEnableLevel(0);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
		#endif

	} else {

		#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
		et9714_deinit(0);
		//Sensor_Set_AfEnableLevel(0);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
		#endif

		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetResetLevel(reset_level);
		Sensor_PowerDown(power_down);
		Sensor_SetDvddVoltage(SENSOR_AVDD_CLOSED);
		usleep(5 * 1000);
		Sensor_SetAvddVoltage(SENSOR_AVDD_CLOSED);
		usleep(5 * 1000);
		Sensor_SetIovddVoltage(SENSOR_AVDD_CLOSED);
	}
	SENSOR_PRINT("(1:on, 0:off): %d", power_on);
	return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * here just read awb,lsc items.But here just read awb data.
 *============================================================================*/
LOCAL uint32_t ov5675_read_otp(unsigned long param)
{
	uint32_t ret                  = SENSOR_SUCCESS;
	SENSOR_OTP_PARAM_T* param_ptr = (SENSOR_OTP_PARAM_T*)param;
	uint32_t start_addr           = 0;
	uint32_t len                  = 0;
	uint8_t *buff                 = NULL;
	uint16_t tmp;

	if (NULL == param_ptr) {
		return -1;
	}
	CMR_LOGI("E:");

	if (SENSOR_OTP_PARAM_NORMAL == param_ptr->type) {
		param_ptr->len = 8192;
	}
	if (NULL != param_ptr->awb.data_ptr && SENSOR_OTP_PARAM_NORMAL == param_ptr->type) {
		uint32_t real_size = 0;
		ret = awb_package_convert(&s_ov5675_darling_otp_info.r_current, param_ptr->awb.data_ptr,param_ptr->awb.size, &real_size);
		param_ptr->awb.size = real_size;
		CMR_LOGI("awb real_size %d", real_size);
		ret = lsc_package_convert(&s_ov5675_darling_otp_info.lsc_param, param_ptr->lsc.data_ptr,param_ptr->lsc.size, &real_size);
		param_ptr->lsc.size = real_size;
		CMR_LOGI("lsc real_size %d", real_size);
	}
	CMR_LOGI("X:");

	return ret;
}

/*==============================================================================
 * Description:
 * read golden module data.you can get the data from sensor FAE.But we should
 * package golden data,otherwise isp can not distinguish it.
 *============================================================================*/
LOCAL uint32_t ov5675_get_golden_data(unsigned long param){
	uint32_t ret  = SENSOR_SUCCESS;
	ret = construct_golden_data(param);
	return ret;
}


/*==============================================================================
 * Description:
 * cfg otp setting
 * please modify this function acording your spec
 *============================================================================*/
static unsigned long ov5675_access_val(unsigned long param)
{
	uint32_t ret = SENSOR_FAIL;
    SENSOR_VAL_T* param_ptr = (SENSOR_VAL_T*)param;

	if(!param_ptr){
		#ifdef FEATURE_OTP
		if(PNULL!=s_ov5675_raw_param_tab_ptr->cfg_otp){
			ret = s_ov5675_raw_param_tab_ptr->cfg_otp(s_ov5675_otp_info_ptr);
			//checking OTP apply result
			if (SENSOR_SUCCESS != ret) {
				SENSOR_PRINT("apply otp failed");
			}
		} else {
			SENSOR_PRINT("no update otp function!");
	}
	#endif
		return ret;
	}
	
	SENSOR_PRINT("sensor ov5675: param_ptr->type=%x", param_ptr->type);
	
	switch(param_ptr->type)
	{
		case SENSOR_VAL_TYPE_SHUTTER:
			*((uint32_t*)param_ptr->pval) = ov5675_read_shutter();
			break;
		case SENSOR_VAL_TYPE_READ_OTP_GAIN:
			*((uint32_t*)param_ptr->pval) = ov5675_read_gain();
			break;
		case SENSOR_VAL_TYPE_READ_OTP:
			ret = ov5675_read_otp((cmr_uint)param_ptr->pval);
			break;
		case SENSOR_VAL_TYPE_GET_GOLDEN_DATA:
			ret = ov5675_get_golden_data((cmr_uint)param_ptr->pval);
			break;
		default:
			break;
	}
    ret = SENSOR_SUCCESS;

	return ret;
}

/*==============================================================================
 * Description:
 * Initialize Exif Info
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t ov5675_InitExifInfo(void)
{
	EXIF_SPEC_PIC_TAKING_COND_T *exif_ptr = &s_ov5675_exif_info;

	memset(&s_ov5675_exif_info, 0, sizeof(EXIF_SPEC_PIC_TAKING_COND_T));

	SENSOR_PRINT("Start");

	/*aperture = numerator/denominator */
	/*fnumber = numerator/denominator */
	exif_ptr->valid.FNumber=1;
	exif_ptr->FNumber.numerator=14;
	exif_ptr->FNumber.denominator=5;
	
	exif_ptr->valid.ApertureValue=1;
	exif_ptr->ApertureValue.numerator=14;
	exif_ptr->ApertureValue.denominator=5;
	exif_ptr->valid.MaxApertureValue=1;
	exif_ptr->MaxApertureValue.numerator=14;
	exif_ptr->MaxApertureValue.denominator=5;
	exif_ptr->valid.FocalLength=1;
	exif_ptr->FocalLength.numerator=289;
	exif_ptr->FocalLength.denominator=100;

	return SENSOR_SUCCESS;
}

static uint32_t  ov5675_get_exif_info(unsigned long param)
{
	return (unsigned long) & s_ov5675_exif_info;
}

/*==============================================================================
 * Description:
 * identify sensor id
 * please modify this function acording your spec
 *============================================================================*/
static uint32_t ov5675_identify(uint32_t param)
{
	uint16_t pid_value = 0x00;
	uint16_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("mipi raw identify");

	pid_value = Sensor_ReadReg(ov5675_PID_ADDR);

	if (ov5675_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(ov5675_VER_ADDR);
		SENSOR_PRINT("Identify: PID = %x, VER = %x", pid_value, ver_value);
		if (ov5675_VER_VALUE == ver_value) {
			SENSOR_PRINT_HIGH("this is ov5675 sensor");
			
			#ifdef FEATURE_OTP
			/*if read otp info failed or module id mismatched ,identify failed ,return SENSOR_FAIL ,exit identify*/
			if(PNULL!=s_ov5675_raw_param_tab_ptr->identify_otp){
				SENSOR_PRINT("identify module_id=0x%x",s_ov5675_raw_param_tab_ptr->param_id);
				//set default value
				memset(s_ov5675_otp_info_ptr, 0x00, sizeof(struct otp_info_t));
				ret_value = s_ov5675_raw_param_tab_ptr->identify_otp(s_ov5675_otp_info_ptr);
				
				if(SENSOR_SUCCESS == ret_value ){
					SENSOR_PRINT("identify otp sucess! module_id=0x%x, module_name=%s",s_ov5675_raw_param_tab_ptr->param_id,MODULE_NAME);
				} else{
					SENSOR_PRINT("identify otp fail! exit identify");
					return ret_value;
				}
			} else{
			SENSOR_PRINT("no identify_otp function!");
			}

			#endif
			
			ov5675_InitExifInfo();

			#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
			ov5675_InitRawTuneInfo();
			#endif
			
			ret_value = SENSOR_SUCCESS;
			
		} else {
			SENSOR_PRINT_HIGH("Identify this is %x%x sensor", pid_value, ver_value);
		}
	} else {
		SENSOR_PRINT_HIGH("sensor identify fail, pid_value = %x", pid_value);
	}

	return ret_value;
}

/*==============================================================================
 * Description:
 * get resolution trim
 *
 *============================================================================*/
static unsigned long ov5675_get_resolution_trim_tab(uint32_t param)
{
	return (unsigned long) s_ov5675_resolution_trim_tab;
}

/*==============================================================================
 * Description:
 * before snapshot
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t ov5675_before_snapshot(uint32_t param)
{
	uint32_t cap_shutter = 0;
	uint32_t prv_shutter = 0;
	uint32_t gain = 0;
	uint32_t cap_gain = 0;
	uint32_t capture_mode = param & 0xffff;
	uint32_t preview_mode = (param >> 0x10) & 0xffff;

	uint32_t prv_linetime = s_ov5675_resolution_trim_tab[preview_mode].line_time;
	uint32_t cap_linetime = s_ov5675_resolution_trim_tab[capture_mode].line_time;

	s_current_default_frame_length = ov5675_get_default_frame_length(capture_mode);
	SENSOR_PRINT("capture_mode = %d", capture_mode);

	if (preview_mode == capture_mode) {
		cap_shutter = s_sensor_ev_info.preview_shutter;
		cap_gain = s_sensor_ev_info.preview_gain;
		goto snapshot_info;
	}

	prv_shutter = s_sensor_ev_info.preview_shutter;	//ov5675_read_shutter();
	gain = s_sensor_ev_info.preview_gain;	//ov5675_read_gain();

	Sensor_SetMode(capture_mode);
	Sensor_SetMode_WaitDone();

	cap_shutter = prv_shutter * prv_linetime / cap_linetime * BINNING_FACTOR;

	while (gain >= (2 * SENSOR_BASE_GAIN)) {
		if (cap_shutter * 2 > s_current_default_frame_length)
			break;
		cap_shutter = cap_shutter * 2;
		gain = gain / 2;
	}

	cap_shutter = ov5675_update_exposure(cap_shutter,0);
	cap_gain = gain;
	ov5675_write_gain(cap_gain);
	SENSOR_PRINT("preview_shutter = 0x%x, preview_gain = 0x%x",
		     s_sensor_ev_info.preview_shutter, s_sensor_ev_info.preview_gain);

	SENSOR_PRINT("capture_shutter = 0x%x, capture_gain = 0x%x", cap_shutter, cap_gain);
snapshot_info:
	s_hdr_info.capture_shutter = cap_shutter; //ov5675_read_shutter();
	s_hdr_info.capture_gain = cap_gain; //ov5675_read_gain();
	/* limit HDR capture min fps to 10;
	 * MaxFrameTime = 1000000*0.1us;
	 */
	//s_hdr_info.capture_max_shutter = 1000000/ cap_linetime;

	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, cap_shutter);

	return SENSOR_SUCCESS;
}

/*==============================================================================
 * Description:
 * get the shutter from isp
 * please don't change this function unless it's necessary
 *============================================================================*/
static uint32_t ov5675_write_exposure(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t exposure_line = 0x00;
	uint16_t dummy_line = 0x00;
	uint16_t mode = 0x00;

	exposure_line = param & 0xffff;
	dummy_line = (param >> 0x10) & 0xfff; /*for cits frame rate test*/
	mode = (param >> 0x1c) & 0x0f;

	SENSOR_PRINT("current mode = %d, exposure_line = %d, dummy_line=%d", mode, exposure_line,dummy_line);
	s_current_default_frame_length = ov5675_get_default_frame_length(mode);

	s_sensor_ev_info.preview_shutter = ov5675_update_exposure(exposure_line,dummy_line);

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
static uint32_t ov5675_write_gain_value(uint32_t param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint32_t real_gain = 0;

	real_gain = isp_to_real_gain(param);

	real_gain = real_gain * SENSOR_BASE_GAIN / ISP_BASE_GAIN;

	SENSOR_PRINT("real_gain = 0x%x", real_gain);

	s_sensor_ev_info.preview_gain = real_gain;
	ov5675_write_gain(real_gain);

	return ret_value;
}

#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
/*==============================================================================
 * Description:
 * write parameter to vcm
 * please add your VCM function to this function
 *============================================================================*/
static uint32_t ov5675_write_af(uint32_t param)
{
	return et9714_write_af(param);
}
#endif

/*==============================================================================
 * Description:
 * increase gain or shutter for hdr
 *
 *============================================================================*/
static void ov5675_increase_hdr_exposure(uint8_t ev_multiplier)
{
	uint32_t shutter_multiply = s_hdr_info.capture_max_shutter / s_hdr_info.capture_shutter;
	uint32_t gain = 0;

	if (0 == shutter_multiply)
		shutter_multiply = 1;

	if (shutter_multiply >= ev_multiplier) {
		ov5675_update_exposure(s_hdr_info.capture_shutter * ev_multiplier,0);
		ov5675_write_gain(s_hdr_info.capture_gain);
	} else {
		gain = s_hdr_info.capture_gain * ev_multiplier / shutter_multiply;
		ov5675_update_exposure(s_hdr_info.capture_shutter * shutter_multiply,0);
		ov5675_write_gain(gain);
	}
}

/*==============================================================================
 * Description:
 * decrease gain or shutter for hdr
 *
 *============================================================================*/
static void ov5675_decrease_hdr_exposure(uint8_t ev_divisor)
{
	uint16_t gain_multiply = 0;
	uint32_t shutter = 0;
	gain_multiply = s_hdr_info.capture_gain / SENSOR_BASE_GAIN;

	if (gain_multiply >= ev_divisor) {
		ov5675_update_exposure(s_hdr_info.capture_shutter,0);
		ov5675_write_gain(s_hdr_info.capture_gain / ev_divisor);

	} else {
		shutter = s_hdr_info.capture_shutter * gain_multiply / ev_divisor;
		ov5675_update_exposure(shutter,0);
		ov5675_write_gain(s_hdr_info.capture_gain / gain_multiply);
	}
}

/*==============================================================================
 * Description:
 * set hdr ev
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t ov5675_set_hdr_ev(unsigned long param)
{
	uint32_t ret = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	uint32_t ev = ext_ptr->param;
	uint8_t ev_divisor, ev_multiplier;

	switch (ev) {
	case SENSOR_HDR_EV_LEVE_0:
		ev_divisor = 2;
		ov5675_decrease_hdr_exposure(ev_divisor);
		break;
	case SENSOR_HDR_EV_LEVE_1:
		ev_multiplier = 1;
		ov5675_increase_hdr_exposure(ev_multiplier);
		break;
	case SENSOR_HDR_EV_LEVE_2:
		ev_multiplier = 2;
		ov5675_increase_hdr_exposure(ev_multiplier);
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
static uint32_t ov5675_ext_func(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	SENSOR_PRINT("ext_ptr->cmd: %d", ext_ptr->cmd);
	switch (ext_ptr->cmd) {
	case SENSOR_EXT_EV:
		rtn = ov5675_set_hdr_ev(param);
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
static uint32_t ov5675_stream_on(uint32_t param)
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
static uint32_t ov5675_stream_off(uint32_t param)
{
	SENSOR_PRINT("E");

	Sensor_WriteReg(0x0100, 0x00);
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
 * .power = ov5675_power_on,
 *============================================================================*/
static SENSOR_IOCTL_FUNC_TAB_T s_ov5675_ioctl_func_tab = {
	.power = ov5675_power_on,
	.identify = ov5675_identify,
	.get_trim = ov5675_get_resolution_trim_tab,
	.before_snapshort = ov5675_before_snapshot,
	.write_ae_value = ov5675_write_exposure,
	.write_gain_value = ov5675_write_gain_value,
	#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
	.af_enable = ov5675_write_af,
	#endif
	.get_exif = ov5675_get_exif_info,
	.set_focus = ov5675_ext_func,
	//.set_video_mode = ov5675_set_video_mode,
	.stream_on = ov5675_stream_on,
	.stream_off = ov5675_stream_off,
	.cfg_otp = ov5675_access_val,	

	//.group_hold_on = ov5675_group_hold_on,
	//.group_hold_of = ov5675_group_hold_off,
};
