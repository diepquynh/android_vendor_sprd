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
* V3.0
*/

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include "sensor_raw.h"
#include "sensor_c2590_denoise.c"

#if defined(CONFIG_CAMERA_ISP_VERSION_V3) || defined(CONFIG_CAMERA_ISP_VERSION_V4)
//#include "sensor_c2590_raw_param_v3.c"
#include "newparam/sensor_c2590_raw_param_main.c"
#else
//#include "sensor_c2590_raw_param.c"
#endif

#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
//#include "af_zzz.h"
#endif

#define SENSOR_NAME			    "c2590"
#define c2590_I2C_ADDR_W       	0x6C
#define c2590_I2C_ADDR_R       	0x6D     /* 8bit slave address*/

#define c2590_PID_ADDR			0x0000
#define c2590_PID_VALUE			0x02
#define c2590_VER_ADDR			0x0001
#define c2590_VER_VALUE			0x02

/* sensor parameters begin */
/* effective sensor output image size */
#define SNAPSHOT_WIDTH			1600
#define SNAPSHOT_HEIGHT			1200
#define PREVIEW_WIDTH           1600
#define PREVIEW_HEIGHT          1200

/*Mipi output*/
#define LANE_NUM			1
#define RAW_BITS		    10

#define SNAPSHOT_MIPI_PER_LANE_BPS	534
#define PREVIEW_MIPI_PER_LANE_BPS	534

/*line time unit: 0.1us*/
#define SNAPSHOT_LINE_TIME	    544
#define PREVIEW_LINE_TIME		544

/* frame length*/
#define SNAPSHOT_FRAME_LENGTH	    1224
#define PREVIEW_FRAME_LENGTH		1224

/* please ref your spec */
#define FRAME_OFFSET			4
#define SENSOR_MAX_GAIN			0x100	/*0x80 means 8x;0x100 means 16x*/
#define SENSOR_BASE_GAIN		0x10
#define SENSOR_MIN_SHUTTER		1

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
//#define SUPPORT_AUTO_FRAME_LENGTH
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

LOCAL cmr_uint _c2590_access_val(cmr_uint param);
/*==============================================================================
* Description:
* global variable
*============================================================================*/
static struct hdr_info_t s_hdr_info;
static uint32_t s_current_default_frame_length;
struct sensor_ev_info_t s_sensor_ev_info;

//#define FEATURE_OTP    /*OTP function switch*/

#ifdef FEATURE_OTP
#define MODULE_ID_NULL			0x0000
#define MODULE_ID_c2590_yyy		0x0001    //c2590: sensor P/N;  yyy: module vendor
#define MODULE_ID_END			0xFFFF
#define LSC_PARAM_QTY 240

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
    uint16_t r_current;
    uint16_t g_current;
    uint16_t b_current;
    uint16_t r_typical;
    uint16_t g_typical;
    uint16_t b_typical;
    uint16_t vcm_dac_start;
    uint16_t vcm_dac_inifity;
    uint16_t vcm_dac_macro;
    uint16_t lsc_param[LSC_PARAM_QTY];
};


#include "sensor_c2590_yyy_otp.c"

struct raw_param_info_tab s_c2590_raw_param_tab[] = {
    {MODULE_ID_c2590_yyy, &s_c2590_mipi_raw_info, c2590_yyy_identify_otp, c2590_yyy_update_otp},
    {MODULE_ID_END, PNULL, PNULL, PNULL}
};

#endif

static SENSOR_IOCTL_FUNC_TAB_T s_c2590_ioctl_func_tab;
struct sensor_raw_info *s_c2590_mipi_raw_info_ptr = &s_c2590_mipi_raw_info;

static const SENSOR_REG_T c2590_init_setting[] = {
		{0x0103,0x01},
		{SENSOR_WRITE_DELAY, 0x0a},
		{0x0100,0x00},
		//for group access
		{0x3400,0x00},
		{0x3404,0x05},
		{0x3500,0x10},
		{0xe000,0x02},//es H  0xE002
		{0xe001,0x02},
		{0xe002,0x02},
		{0xe003,0x02},//es L   0xE005
		{0xe004,0x03},
		{0xe005,0x68},
		{0xe006,0x02},//gain    0xE008
		{0xe007,0x05},
		{0xe008,0x00},
		{0xe009,0x03},//vts H  0xE00B
		{0xe00A,0x40},
		{0xe00B,0x02},
		{0xe00C,0x03},//vts L  0xE00E
		{0xe00D,0x41},
		{0xe00E,0x68},
		{0x3500,0x00},
		//soft reset
		{0x0101,0x00},//{0x0101,0x01},
		//pixel order
		{0x3009,0x03},
		{0x300b,0x03},
		{0x3180,0xd0},
		{0x3181,0x40},
		{0x3584,0x00},
		//analog
		{0x3280,0x06},
		{0x3281,0x05},
		{0x3282,0x93},
		{0x3283,0xd2},
		{0x3287,0x46},
		{0x3288,0x5f},
		{0x3289,0x30},
		{0x328A,0x21},
		{0x328B,0x44},
		{0x328C,0x78},
		{0x328D,0x55},
		{0x328E,0x00},
		{0x308a,0x00},
		{0x308b,0x00},
		{0x3209,0x80},
		{0x320C,0x00},
		{0x320E,0x08},
		{0x3210,0x11},
		{0x3211,0x09},
		{0x3212,0x1a},
		{0x3213,0x15},
		{0x3214,0x17},
		{0x3215,0x09},
		{0x3216,0x17},
		{0x3217,0x06},
		{0x3218,0x28},
		{0x3219,0x12},
		{0x321A,0x00},
		{0x321B,0x04},
		{0x321C,0x00},
		{0x3200,0x03},
		{0x3201,0x46},
		{0x3202,0x00},
		{0x3203,0x17},
		{0x3204,0x01},
		{0x3205,0x64},
		{0x3206,0x00},
		{0x3207,0xde},
		{0x3208,0x83},
		//pll
		{0x0303,0x01},
		{0x0304,0x00},
		{0x0305,0x03},
		{0x0307,0x59},
		{0x0309,0x30},
		{0x0342,0x0b},
		{0x0343,0x5c},
		{0x3087,0x90},// ;aec/agc: off
		{0x3089,0x18},
		{0x3c03,0x01},
		//BPC
		{0x3d00,0xad},// ;;af; BPC ON
		{0x3d01,0x15},
		{0x3d07,0x40},
		{0x3d08,0x40},
		//mipi
		{0x3805,0x06},
		{0x3806,0x06},
		{0x3807,0x06},
		{0x3808,0x14},
		{0x3809,0xc4},
		{0x380a,0x6c},
		{0x380b,0x8c},
		{0x380c,0x21},
};

static const SENSOR_REG_T c2590_preview_setting[] = {
};

static const SENSOR_REG_T c2590_snapshot_setting[] = {
};

static SENSOR_REG_TAB_INFO_T s_c2590_resolution_tab_raw[SENSOR_MODE_MAX] = {
    {ADDR_AND_LEN_OF_ARRAY(c2590_init_setting), 0, 0, EX_MCLK,
     SENSOR_IMAGE_FORMAT_RAW},
    {ADDR_AND_LEN_OF_ARRAY(c2590_preview_setting),
    PREVIEW_WIDTH, PREVIEW_HEIGHT, EX_MCLK,
    SENSOR_IMAGE_FORMAT_RAW},
    {ADDR_AND_LEN_OF_ARRAY(c2590_snapshot_setting),
    SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT, EX_MCLK,
    SENSOR_IMAGE_FORMAT_RAW},
};

static SENSOR_TRIM_T s_c2590_resolution_trim_tab[SENSOR_MODE_MAX] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},/*common init*/
	{0, 0, PREVIEW_WIDTH, PREVIEW_HEIGHT, PREVIEW_LINE_TIME, PREVIEW_MIPI_PER_LANE_BPS, PREVIEW_FRAME_LENGTH, {0, 0, PREVIEW_WIDTH, PREVIEW_HEIGHT}},/*preview setting*/
	{0, 0, SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT, SNAPSHOT_LINE_TIME, SNAPSHOT_MIPI_PER_LANE_BPS, SNAPSHOT_FRAME_LENGTH, {0, 0, SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT}},/*capture setting*/
};

static const SENSOR_REG_T s_c2590_preview_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

static const SENSOR_REG_T s_c2590_capture_size_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

static SENSOR_VIDEO_INFO_T s_c2590_video_info[SENSOR_MODE_MAX] = {
    {{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
    {{{15, 15, 544, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
		(SENSOR_REG_T **) s_c2590_preview_size_video_tab},
    {{{15, 15, 544, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
		(SENSOR_REG_T **) s_c2590_capture_size_video_tab},
};

/*==============================================================================
* Description:
* set video mode
*
*============================================================================*/
static cmr_uint c2590_set_video_mode(cmr_uint param)
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

    if (PNULL == s_c2590_video_info[mode].setting_ptr) {
        SENSOR_PRINT("fail.");
        return SENSOR_FAIL;
    }

    sensor_reg_ptr = (SENSOR_REG_T_PTR) & s_c2590_video_info[mode].setting_ptr[param];
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
SENSOR_INFO_T g_c2590_mipi_raw_info = {
	/* salve i2c write address */
	(c2590_I2C_ADDR_W >> 1),
	/* salve i2c read address */
	(c2590_I2C_ADDR_R >> 1),
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
	10,
	/* 1: high level valid; 0: low level valid */
	SENSOR_LOW_LEVEL_PWDN,
	/* count of identify code */
	1,
	/* supply two code to identify sensor.
	* for Example: index = 0-> Device id, index = 1 -> version id
	* customer could ignore it.
	*/
	{{c2590_PID_ADDR, c2590_PID_VALUE},
	 {c2590_VER_ADDR, c2590_VER_VALUE}
	},
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
	s_c2590_resolution_tab_raw,
	/* point to ioctl function table */
	&s_c2590_ioctl_func_tab,
	/* information and table about Rawrgb sensor */
	&s_c2590_mipi_raw_info_ptr,
	/* extend information about sensor
	* like &g_c2590_ext_info
	*/
	NULL,
	/* voltage of iovdd */
	SENSOR_AVDD_1800MV,
	/* voltage of dvdd */
	SENSOR_AVDD_1500MV,
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
	{SENSOR_INTERFACE_TYPE_CSI2, LANE_NUM, RAW_BITS, 0},
	s_c2590_video_info,
	/* skip frame num while change setting */
	3,
	/* horizontal  view angle*/
	48,
	/* vertical view angle*/
	48
};

/*==============================================================================
* Description:
* get default frame length
*
*============================================================================*/
static uint32_t c2590_get_default_frame_length(uint32_t mode)
{
    return s_c2590_resolution_trim_tab[mode].frame_line;
}

/*==============================================================================
* Description:
* write group-hold on to sensor registers
* please modify this function acording your spec
*============================================================================*/
static void c2590_group_hold_on(void)
{
    SENSOR_PRINT("E");
	//Sensor_WriteReg(0x340F, 0x20);
    return;
}

/*==============================================================================
* Description:
* write group-hold off to sensor registers
* please modify this function acording your spec
*============================================================================*/
static void c2590_group_hold_off(void)
{
    SENSOR_PRINT("E");
    //Sensor_WriteReg(0x340F, 0x20);
}


/*==============================================================================
* Description:
* read gain from sensor registers
* please modify this function acording your spec
*============================================================================*/
static uint16_t c2590_read_gain(void)
{
    uint16_t gain_h = 0;
    uint16_t gain_l = 0;

    gain_l = Sensor_ReadReg(0x0205) & 0xff;
	SENSOR_PRINT_HIGH("gain register = 0x%x\n",gain_l);
#if 1
	gain_l = (((gain_l >> 4) + 1) * ISP_BASE_GAIN) + (((gain_l & 0x0F) * ISP_BASE_GAIN)/16);
#else
	{
		uint16_t high = 0;
		uint16_t low = 0;
		high = (((gain_l >> 4) + 1) * ISP_BASE_GAIN);
		low = (((gain_l & 0x0F) * ISP_BASE_GAIN)/16);
		SENSOR_PRINT_HIGH("high,low:%d,%d\n",high,low);
		gain_l = high + low;
	}
#endif
    return ((gain_h << 8) | gain_l);
}

/*==============================================================================
* Description:
* write gain to sensor registers
* please modify this function acording your spec
*============================================================================*/
static void c2590_write_gain(uint32_t gain)
{
    uint16_t sensor_gain;
    if (SENSOR_MAX_GAIN < gain)
    gain = SENSOR_MAX_GAIN;
	/*0x0205 0xMN;Means:real_gain=(M+1)+(N/16)*/
    //SENSOR_PRINT_HIGH("gain= %d\n",gain);
    sensor_gain=(((gain-16)/16)<<4)+gain%16;
    Sensor_WriteReg(0x0205, sensor_gain);
    SENSOR_PRINT_HIGH("gain register = 0x%x\n",sensor_gain);
	//c2590_group_hold_off();
}

/*==============================================================================
* Description:
* read frame length from sensor registers
* please modify this function acording your spec
*============================================================================*/
static uint16_t c2590_read_frame_length(void)
{
    uint16_t frame_len_h = 0;
    uint16_t frame_len_l = 0;
    frame_len_h = Sensor_ReadReg(0x0340) & 0xff;
    frame_len_l = Sensor_ReadReg(0x0341) & 0xff;
	SENSOR_PRINT_HIGH("c2590_read_frame_length,h&l= %x,%x\n",frame_len_h,frame_len_l);
    return ((frame_len_h << 8) | frame_len_l);
}

/*==============================================================================
* Description:
* write frame length to sensor registers
* please modify this function acording your spec
*============================================================================*/
static void c2590_write_frame_length(uint32_t frame_len)
{
    Sensor_WriteReg(0x0340, (frame_len >> 8) & 0xff);
    Sensor_WriteReg(0x0341, frame_len & 0xff);
}

/*==============================================================================
* Description:
* read shutter from sensor registers
* please modify this function acording your spec
*============================================================================*/
static uint32_t c2590_read_shutter(void)
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
static void c2590_write_shutter(uint32_t shutter)
{
    SENSOR_PRINT("c2590_write_shutter:%d",shutter);
    Sensor_WriteReg(0x0202, (shutter >> 8) & 0xff);
    Sensor_WriteReg(0x0203, shutter & 0xff);
}

/*==============================================================================
* Description:
* write exposure to sensor registers and get current shutter
* please pay attention to the frame length
* please don't change this function if it's necessary
*============================================================================*/
static uint16_t c2590_update_exposure(uint32_t shutter,uint32_t dummy_line)
{
    uint32_t dest_fr_len = 0;
    uint32_t cur_fr_len = 0;
    uint32_t fr_len = s_current_default_frame_length;

    SENSOR_PRINT("c2590_write_shutter:%d",shutter);

    c2590_group_hold_on();

    #ifdef SUPPORT_AUTO_FRAME_LENGTH
    goto write_sensor_shutter;
    #endif

    dest_fr_len = ((shutter + dummy_line+FRAME_OFFSET) > fr_len) ? (shutter +dummy_line+ FRAME_OFFSET) : fr_len;

    cur_fr_len = c2590_read_frame_length();
	SENSOR_PRINT("c2590_write_shutter dest_fr_len,cur_fr_len:%d,%d",dest_fr_len,cur_fr_len);
    if (shutter < SENSOR_MIN_SHUTTER)
		shutter = SENSOR_MIN_SHUTTER;

    if (dest_fr_len != cur_fr_len)
		c2590_write_frame_length(dest_fr_len);
    write_sensor_shutter:
    /* write shutter to sensor registers */
    c2590_write_shutter(shutter);
    return shutter;
}

/*==============================================================================
* Description:
* sensor power on
* please modify this function acording your spec
*============================================================================*/
static cmr_uint c2590_power_on(cmr_uint power_on)
{
    SENSOR_AVDD_VAL_E dvdd_val = g_c2590_mipi_raw_info.dvdd_val;
    SENSOR_AVDD_VAL_E avdd_val = g_c2590_mipi_raw_info.avdd_val;
    SENSOR_AVDD_VAL_E iovdd_val = g_c2590_mipi_raw_info.iovdd_val;
    BOOLEAN power_down = g_c2590_mipi_raw_info.power_down_level;
    BOOLEAN reset_level = g_c2590_mipi_raw_info.reset_pulse_level;
    //uint32_t reset_width=g_c2590_yuv_info.reset_pulse_width;

    SENSOR_PRINT("dvdd_val:%d,avdd_val:%d,iovdd_val:%d,reset_level:%d",dvdd_val,avdd_val,iovdd_val,reset_level);
    if (SENSOR_TRUE == power_on) {
        //set all power pin to disable status
        Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
        //Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
        Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
        Sensor_SetResetLevel(reset_level);
        Sensor_PowerDown(power_down);
        usleep(10*1000);
        //step 0 power up DOVDD, the AVDD
        //Sensor_SetMonitorVoltage(SENSOR_AVDD_3300MV);
        Sensor_SetIovddVoltage(iovdd_val);
        usleep(2000);
        Sensor_SetAvddVoltage(avdd_val);
        usleep(3000);
        //step 1 power up DVDD
        //Sensor_SetDvddVoltage(dvdd_val);
        //usleep(3000);
        //step 2 power down pin high
        Sensor_PowerDown(!power_down);
        usleep(2000);
        //step 3 reset pin high
        Sensor_SetResetLevel(!reset_level);
        usleep(22*1000);
        //step 4 xvclk
        Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
        usleep(10*1000);

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
	SENSOR_PRINT("SENSOR_c2590: _c2590_Power_On(1:on, 0:off): %d",(unsigned int)power_on);
	return SENSOR_SUCCESS;
}

#ifdef FEATURE_OTP

/*==============================================================================
* Description:
* get  parameters from otp
* please modify this function acording your spec
*============================================================================*/
static int c2590_get_otp_info(struct otp_info_t *otp_info)
{
    uint32_t ret = SENSOR_FAIL;
    uint32_t i = 0x00;

    //identify otp information
    for (i = 0; i < NUMBER_OF_ARRAY(s_c2590_raw_param_tab); i++) {
        SENSOR_PRINT("identify module_id=0x%x",s_c2590_raw_param_tab[i].param_id);

        if(PNULL!=s_c2590_raw_param_tab[i].identify_otp){
            //set default value;
            memset(otp_info, 0x00, sizeof(struct otp_info_t));

            if(SENSOR_SUCCESS==s_c2590_raw_param_tab[i].identify_otp(otp_info)){
                if (s_c2590_raw_param_tab[i].param_id== otp_info->module_id) {
                    SENSOR_PRINT("identify otp sucess! module_id=0x%x",s_c2590_raw_param_tab[i].param_id);
                    ret = SENSOR_SUCCESS;
                    break;
                }
                else{
                    SENSOR_PRINT("identify module_id failed! table module_id=0x%x, otp module_id=0x%x",s_c2590_raw_param_tab[i].param_id,otp_info->module_id);
                }
            }
            else{
                SENSOR_PRINT("identify_otp failed!");
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
static uint32_t c2590_apply_otp(struct otp_info_t *otp_info, int id)
{
    uint32_t ret = SENSOR_FAIL;
    //apply otp parameters
    SENSOR_PRINT("otp_table_id = %d", id);
    if (PNULL != s_c2590_raw_param_tab[id].cfg_otp) {

        if(SENSOR_SUCCESS==s_c2590_raw_param_tab[id].cfg_otp(otp_info)){
            SENSOR_PRINT("apply otp parameters sucess! module_id=0x%x",s_c2590_raw_param_tab[id].param_id);
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
static uint32_t c2590_cfg_otp(uint32_t param)
{
    uint32_t ret = SENSOR_FAIL;
    struct otp_info_t otp_info={0x00};
    int table_id = 0;

    table_id = c2590_get_otp_info(&otp_info);
    if (-1 != table_id)
    ret = c2590_apply_otp(&otp_info, table_id);

    //checking OTP apply result
    if (SENSOR_SUCCESS != ret) {//disable lsc
                                Sensor_WriteReg(0xffff,0xff);
                               }
    else{//enable lsc
         Sensor_WriteReg(0xffff,0xff);
        }

    return ret;
}
#endif

/*==============================================================================
* Description:
* identify sensor id
* please modify this function acording your spec
*============================================================================*/
static cmr_uint c2590_identify(cmr_uint param)
{
    uint16_t pid_value = 0x00;
    uint16_t ver_value = 0x00;
    uint32_t ret_value = SENSOR_FAIL;

    SENSOR_PRINT("mipi raw identify");

    pid_value = Sensor_ReadReg(c2590_PID_ADDR);
    if (c2590_PID_VALUE == pid_value) {
        ver_value = Sensor_ReadReg(c2590_VER_ADDR);
        SENSOR_PRINT("Identify: PID = %x, VER = %x", pid_value, ver_value);
        if (c2590_VER_VALUE == ver_value) {
			Sensor_c2590_InitRawTuneInfo(&g_c2590_mipi_raw_info);
            ret_value = SENSOR_SUCCESS;
            SENSOR_PRINT_HIGH("this is c2590 sensor");
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
static cmr_uint c2590_get_resolution_trim_tab(cmr_uint param)
{
    return (cmr_uint) s_c2590_resolution_trim_tab;
}

/*==============================================================================
* Description:
* before snapshot
* you can change this function if it's necessary
*============================================================================*/
static cmr_uint c2590_before_snapshot(cmr_uint param)
{
    uint32_t cap_shutter = 0;
    uint32_t prv_shutter = 0;
    uint32_t gain = 0;
    uint32_t cap_gain = 0;
    uint32_t capture_mode = param & 0xffff;
    uint32_t preview_mode = (param >> 0x10) & 0xffff;

    uint32_t prv_linetime = s_c2590_resolution_trim_tab[preview_mode].line_time;
    uint32_t cap_linetime = s_c2590_resolution_trim_tab[capture_mode].line_time;

    s_current_default_frame_length = c2590_get_default_frame_length(capture_mode);
    SENSOR_PRINT("capture_mode = %d", capture_mode);

    if (preview_mode == capture_mode) {
        cap_shutter = s_sensor_ev_info.preview_shutter;
        cap_gain = s_sensor_ev_info.preview_gain;
        goto snapshot_info;
    }

    prv_shutter = s_sensor_ev_info.preview_shutter;	//c2590_read_shutter();
    gain = s_sensor_ev_info.preview_gain;	//c2590_read_gain();

    Sensor_SetMode(capture_mode);
    Sensor_SetMode_WaitDone();

    cap_shutter = prv_shutter * prv_linetime / cap_linetime * BINNING_FACTOR;

    while (gain >= (2 * SENSOR_BASE_GAIN)) {
        if (cap_shutter * 2 > s_current_default_frame_length)
        break;
        cap_shutter = cap_shutter * 2;
        gain = gain / 2;
    }

    cap_shutter = c2590_update_exposure(cap_shutter,0);
    cap_gain = gain;
    c2590_write_gain(cap_gain);
    SENSOR_PRINT("preview_shutter = 0x%x, preview_gain = 0x%x",
                 s_sensor_ev_info.preview_shutter, s_sensor_ev_info.preview_gain);

    SENSOR_PRINT("capture_shutter = 0x%x, capture_gain = 0x%x", cap_shutter, cap_gain);
    snapshot_info:
    s_hdr_info.capture_shutter = cap_shutter; //c2590_read_shutter();
    s_hdr_info.capture_gain = cap_gain; //c2590_read_gain();
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
static cmr_uint c2590_write_exposure(cmr_uint param)
{
    uint32_t ret_value = SENSOR_SUCCESS;
    uint16_t exposure_line = 0x00;
    uint16_t dummy_line = 0x00;
    uint16_t mode = 0x00;

    exposure_line = param & 0xffff;
    dummy_line = (param >> 0x10) & 0xfff; /*for cits frame rate test*/
	/*for test only*/
	dummy_line = 0;
    mode = (param >> 0x1c) & 0x0f;

    SENSOR_PRINT("current mode = %d, exposure_line = %d, dummy_line=%d", mode, exposure_line,dummy_line);
    s_current_default_frame_length = c2590_get_default_frame_length(mode);

    s_sensor_ev_info.preview_shutter = c2590_update_exposure(exposure_line,dummy_line);

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
static cmr_uint c2590_write_gain_value(cmr_uint param)
{
    uint32_t ret_value = SENSOR_SUCCESS;
    uint32_t real_gain = 0;

    real_gain = isp_to_real_gain(param);
    SENSOR_PRINT("param = 0x%x",(unsigned int) param);
    SENSOR_PRINT("real_gain = 0x%x", real_gain);
    //real_gain = ((real_gain & 0xf0) >> 4 + 1 ) + (real_gain & 0x0f)/16;

    real_gain = real_gain * SENSOR_BASE_GAIN / ISP_BASE_GAIN;

    SENSOR_PRINT("c2590 real gain = 0x%x", real_gain);

    s_sensor_ev_info.preview_gain = real_gain;
    c2590_write_gain(real_gain);

    return ret_value;
}

#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
/*==============================================================================
* Description:
* write parameter to vcm
* please add your VCM function to this function
*============================================================================*/
static uint32_t c2590_write_af(uint32_t param)
{
    uint32_t ret_value = SENSOR_SUCCESS;

    return ret_value;
}
#endif

/*==============================================================================
* Description:
* increase gain or shutter for hdr
*
*============================================================================*/
static void c2590_increase_hdr_exposure(uint8_t ev_multiplier)
{
    uint32_t shutter_multiply = s_hdr_info.capture_max_shutter / s_hdr_info.capture_shutter;
    uint32_t gain = 0;

    if (0 == shutter_multiply)
    shutter_multiply = 1;

    if (shutter_multiply >= ev_multiplier) {
        c2590_update_exposure(s_hdr_info.capture_shutter * ev_multiplier,0);
        c2590_write_gain(s_hdr_info.capture_gain);
    } else {
        gain = s_hdr_info.capture_gain * ev_multiplier / shutter_multiply;
        c2590_update_exposure(s_hdr_info.capture_shutter * shutter_multiply,0);
        c2590_write_gain(gain);
    }
}

/*==============================================================================
* Description:
 * decrease gain or shutter for hdr
 *
 *============================================================================*/
static void c2590_decrease_hdr_exposure(uint8_t ev_divisor)
{
	uint16_t gain_multiply = 0;
	uint32_t shutter = 0;
	gain_multiply = s_hdr_info.capture_gain / SENSOR_BASE_GAIN;

	if (gain_multiply >= ev_divisor) {
		c2590_update_exposure(s_hdr_info.capture_shutter,0);
		c2590_write_gain(s_hdr_info.capture_gain / ev_divisor);

	} else {
		shutter = s_hdr_info.capture_shutter * gain_multiply / ev_divisor;
		c2590_update_exposure(shutter,0);
		c2590_write_gain(s_hdr_info.capture_gain / gain_multiply);
	}
}

/*==============================================================================
 * Description:
 * set hdr ev
 * you can change this function if it's necessary
 *============================================================================*/
static uint32_t c2590_set_hdr_ev(unsigned long param)
{
	uint32_t ret = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	uint32_t ev = ext_ptr->param;
	uint8_t ev_divisor, ev_multiplier;

	switch (ev) {
	case SENSOR_HDR_EV_LEVE_0:
		ev_divisor = 2;
		c2590_decrease_hdr_exposure(ev_divisor);
		break;
	case SENSOR_HDR_EV_LEVE_1:
		ev_multiplier = 2;
		c2590_increase_hdr_exposure(ev_multiplier);
		break;
	case SENSOR_HDR_EV_LEVE_2:
		ev_multiplier = 1;
		c2590_increase_hdr_exposure(ev_multiplier);
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
static cmr_uint c2590_ext_func(cmr_uint param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	SENSOR_PRINT("ext_ptr->cmd: %d", ext_ptr->cmd);
	switch (ext_ptr->cmd) {
	case SENSOR_EXT_EV:
		rtn = c2590_set_hdr_ev(param);
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
static cmr_uint c2590_stream_on(cmr_uint param)
{
	SENSOR_PRINT("E");
    int temp;

	temp = Sensor_ReadReg(0x0100);

	temp=temp|0x01;

	SENSOR_PRINT("SENSOR_c2590: StreamOn: 0x%x",temp);

	Sensor_WriteReg(0x0100, temp);
	/*delay*/
//	usleep(10 * 1000);

	return 0;
}

/*==============================================================================
 * Description:
 * mipi stream off
 * please modify this function acording your spec
 *============================================================================*/
static cmr_uint c2590_stream_off(cmr_uint param)
{
	SENSOR_PRINT("E");
    int temp;

	temp = Sensor_ReadReg(0x0100);

	temp=temp&0xfe;

	SENSOR_PRINT("SENSOR_c2590: StreamOff: 0x%x",temp);

	Sensor_WriteReg(0x0100, temp);
	/*delay*/
	usleep(150 * 1000);

	return 0;
}
/*==============================================================================
 * Description:
 * extra funciton
 * please modify this function acording your spec
 *============================================================================*/

LOCAL cmr_uint _c2590_access_val(cmr_uint param)
{
	cmr_uint rtn = SENSOR_SUCCESS;
	SENSOR_VAL_T* param_ptr = (SENSOR_VAL_T*)param;
	uint16_t tmp;
	if(!param_ptr){
		return rtn;
	}
	SENSOR_PRINT("SENSOR_ov5670: param_ptr->type=%x", param_ptr->type);
	switch(param_ptr->type)
	{
		case SENSOR_VAL_TYPE_SHUTTER:
			*((uint32_t*)param_ptr->pval) = c2590_read_shutter();
			break;
		case SENSOR_VAL_TYPE_READ_OTP_GAIN:
			*((uint32_t*)param_ptr->pval) = c2590_read_gain();
		default:
			break;
	}

	SENSOR_PRINT("SENSOR_ov5670: _ov5670_access_val X");
	return rtn;
}
/*==============================================================================
 * Description:
 * all ioctl functoins
 * you can add functions reference SENSOR_IOCTL_FUNC_TAB_T from sensor_drv_u.h
 *
 * add ioctl functions like this:
 * .power = c2590_power_on,
 *============================================================================*/
static SENSOR_IOCTL_FUNC_TAB_T s_c2590_ioctl_func_tab = {
	.power = c2590_power_on,
	.identify = c2590_identify,
	.get_trim = c2590_get_resolution_trim_tab,
	.before_snapshort = c2590_before_snapshot,
	.write_ae_value = c2590_write_exposure,
	.write_gain_value = c2590_write_gain_value,
	#ifndef CONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
	.af_enable = c2590_write_af,
	#endif
	.set_focus = c2590_ext_func,
	.set_video_mode = c2590_set_video_mode,
	.stream_on = c2590_stream_on,
	.stream_off = c2590_stream_off,
#if 0
	#ifdef FEATURE_OTP
	.cfg_otp=c2590_cfg_otp,
	#endif
#else
	.cfg_otp = _c2590_access_val,
#endif
	//.group_hold_on = c2590_group_hold_on,
	//.group_hold_of = c2590_group_hold_off,
};
