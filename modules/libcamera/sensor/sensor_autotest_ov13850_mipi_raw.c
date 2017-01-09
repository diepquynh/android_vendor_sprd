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
#include "ov13850/sensor_ov13850_raw_param_v3.c"
#else
#endif


//#include "sensor_ov13850_raw_param.c"
#include "sensor_ov13850_otp.c"

#define ov13850_I2C_ADDR_W         (0x10)
#define ov13850_I2C_ADDR_R         (0x10)

#define OV13850_RAW_PARAM_COM  0x0000

LOCAL unsigned long _at_ov13850_GetResolutionTrimTab(unsigned long param);
LOCAL unsigned long _at_ov13850_PowerOn(unsigned long power_on);
LOCAL unsigned long _at_ov13850_Identify(unsigned long param);
LOCAL unsigned long _at_ov13850_StreamOn(unsigned long param);
LOCAL unsigned long _at_ov13850_StreamOff(unsigned long param);
LOCAL uint32_t _at_ov13850_com_Identify_otp(void* param_ptr);

LOCAL const struct raw_param_info_tab s_at_ov13850_raw_param_tab[]={
	//{OV13850_RAW_PARAM_COM, &s_ov13850_mipi_raw_info, _ov13850_Oflim_Identify_otp, _ov13850_update_otp},
	{OV13850_RAW_PARAM_COM, &s_ov13850_mipi_raw_info, _at_ov13850_com_Identify_otp, PNULL},
	{RAW_INFO_END_ID, PNULL, PNULL, PNULL}
};

struct sensor_raw_info* s_at_ov13850_mipi_raw_info_ptr=NULL;

static uint32_t g_ov13850_module_id = 0;



LOCAL const SENSOR_REG_T ov13850_common_init[] =
{
  {0x0103, 0x01},
  {0x0100, 0x00},
  {0x0300, 0x01},	//for 640Mbps
  {0x0301, 0x00},
  {0x0302, 0x28},
  {0x0303, 0x00},
  {0x030a, 0x00},
  {0x300f, 0x11},
  {0x3010, 0x01},
  {0x3011, 0x76},
  {0x3012, 0x41},
  {0x3013, 0x12},
  {0x3014, 0x11},
  {0x301f, 0x03},
  {0x3106, 0x00},
  {0x3210, 0x47},
  {0x3500, 0x00},
  {0x3501, 0x60},
  {0x3502, 0x00},
  {0x3506, 0x00},
  {0x3507, 0x02},
  {0x3508, 0x00},
#ifdef use_sensor_gain
	{0x3509, 0x00}, // use sensor gain
#else
	{0x3509, 0x10}, // use real gain
#endif
  {0x350a, 0x00},
  {0x350b, 0x80},
  {0x350e, 0x00},
  {0x350f, 0x10},
  {0x3600, 0x00}, //0x40
  {0x3601, 0xfc},
  {0x3602, 0x02},
  {0x3603, 0x48},
  {0x3604, 0xa5},
  {0x3605, 0x9f},
  {0x3607, 0x00},
  {0x360a, 0x40},
  {0x360b, 0x91},
  {0x360c, 0x49},
  {0x360f, 0x8a},
  {0x3611, 0x10},
  {0x3612, 0x27},
  {0x3613, 0x33},
  {0x3615, 0x08},
  {0x3641, 0x02},
  {0x3660, 0x82},
  {0x3668, 0x54},
  {0x3669, 0x00},
  {0x3667, 0xa0},
  {0x3702, 0x40},
  {0x3703, 0x44},
  {0x3704, 0x2c},
  {0x3705, 0x24},
  {0x3706, 0x50},
  {0x3707, 0x44},
  {0x3708, 0x3c},
  {0x3709, 0x1f},
  {0x370a, 0x26},
  {0x370b, 0x3c},
  {0x3720, 0x66},
  {0x3722, 0x84},
  {0x3728, 0x40},
  {0x372a, 0x00},
  {0x372e, 0x22},
  {0x372f, 0xa0},
  {0x3730, 0x00},
  {0x3731, 0x00},
  {0x3732, 0x00},
  {0x3733, 0x00},
  {0x3748, 0x00},
  {0x3710, 0x28},
  {0x3716, 0x03},
  {0x3718, 0x1C},
  {0x3719, 0x08},
  {0x371c, 0xfc},
  {0x3760, 0x13},
  {0x3761, 0x34},
  {0x3762, 0x86},
  {0x3763, 0x16},
  {0x3767, 0x24},
  {0x3768, 0x06},
  {0x3769, 0x45},
  {0x376c, 0x23},
  {0x3d84, 0x00},
  {0x3d85, 0x17},
  {0x3d8c, 0x73},
  {0x3d8d, 0xbf},
  {0x3800, 0x00},
  {0x3801, 0x00},
  {0x3802, 0x00},
  {0x3803, 0x04},
  {0x3804, 0x10},
  {0x3805, 0x9f},
  {0x3806, 0x0c},
  {0x3807, 0x4b},
  {0x3808, 0x08},
  {0x3809, 0x40},
  {0x380a, 0x06},
  {0x380b, 0x20},
  {0x380c, 0x09},
  {0x380d, 0x60},
  {0x380e, 0x0d},
  {0x380f, 0x00},
  {0x3810, 0x00},
  {0x3811, 0x08},
  {0x3812, 0x00},
  {0x3813, 0x02},
  {0x3814, 0x31},
  {0x3815, 0x31},
  {0x3820, 0x01},
  {0x3821, 0x06},
  {0x3834, 0x00},
  {0x3835, 0x1c},
  {0x3836, 0x08},
  {0x3837, 0x02},
  {0x4000, 0xf1},
  {0x4001, 0x00},
  {0x400b, 0x0c},
  {0x4011, 0x00},
  {0x401a, 0x00},
  {0x401b, 0x00},
  {0x401c, 0x00},
  {0x401d, 0x00},
  {0x4020, 0x00},
  {0x4021, 0xe4},
  {0x4022, 0x04},
  {0x4023, 0xd7},
  {0x4024, 0x05},
  {0x4025, 0xbc},
  {0x4026, 0x05},
  {0x4027, 0xbf},
  {0x4028, 0x00},
  {0x4029, 0x02},
  {0x402a, 0x04},
  {0x402b, 0x08},
  {0x402c, 0x02},
  {0x402d, 0x02},
  {0x402e, 0x0c},
  {0x402f, 0x08},
  {0x403d, 0x2c},
  {0x403f, 0x7f},
  {0x4500, 0x82},
  {0x4501, 0x3c},
  {0x4601, 0x83},
  {0x4602, 0x22},
  {0x4603, 0x01},
  {0x4837, 0x19},
  {0x4d00, 0x04},
  {0x4d01, 0x42},
  {0x4d02, 0xd1},
  {0x4d03, 0x90},
  {0x4d04, 0x66},
  {0x4d05, 0x65},
  {0x5000, 0x0e},
  {0x5001, 0x01},
  {0x5002, 0x07},
  {0x5013, 0x40},
  {0x501c, 0x00},
  {0x501d, 0x10},
  {0x5242, 0x00},
  {0x5243, 0xb8},
  {0x5244, 0x00},
  {0x5245, 0xf9},
  {0x5246, 0x00},
  {0x5247, 0xf6},
  {0x5248, 0x00},
  {0x5249, 0xa6},
  {0x5300, 0xfc},
  {0x5301, 0xdf},
  {0x5302, 0x3f},
  {0x5303, 0x08},
  {0x5304, 0x0c},
  {0x5305, 0x10},
  {0x5306, 0x20},
  {0x5307, 0x40},
  {0x5308, 0x08},
  {0x5309, 0x08},
  {0x530a, 0x02},
  {0x530b, 0x01},
  {0x530c, 0x01},
  {0x530d, 0x0c},
  {0x530e, 0x02},
  {0x530f, 0x01},
  {0x5310, 0x01},
  {0x5400, 0x00},
  {0x5401, 0x61},
  {0x5402, 0x00},
  {0x5403, 0x00},
  {0x5404, 0x00},
  {0x5405, 0x40},
  {0x540c, 0x05},
  {0x5b00, 0x00},
  {0x5b01, 0x00},
  {0x5b02, 0x01},
  {0x5b03, 0xff},
  {0x5b04, 0x02},
  {0x5b05, 0x6c},
  {0x5b09, 0x02},
  {0x5e00, 0x00},
  {0x5e10, 0x1c},
  {0x0100, 0x01},
};

LOCAL const SENSOR_REG_T ov13850_640x480_setting[] =
{
  //{0x0100, 0x00},
  {0x0300, 0x01},
  {0x0302, 0x28},
  {0x3612, 0x27},
  {0x370a, 0xa9},
  {0x372f, 0x88},
  {0x372a, 0x00},
  {0x3800, 0x02},
  {0x3801, 0x00},
  {0x3802, 0x01},
  {0x3803, 0x70},
  {0x3804, 0x0e},
  {0x3805, 0x9f},
  {0x3806, 0x0A},
  {0x3807, 0xDf},
  {0x3808, 0x02},
  {0x3809, 0x80},
  {0x380a, 0x01},
  {0x380b, 0xe0},
  {0x380c, 0x10},
  {0x380d, 0x60},
  {0x380e, 0x07},
  {0x380f, 0x60},
  {0x3810, 0x00},
  {0x3811, 0x04},
  {0x3812, 0x00},
  {0x3813, 0x02},
  {0x3814, 0x31},
  {0x3815, 0x35},
  {0x3820, 0x00},
  {0x3821, 0x04},
  {0x3834, 0x02},
  {0x3836, 0x08},
  {0x3837, 0x04},
  {0x4020, 0x00},
  {0x4021, 0xe3},
  {0x4022, 0x02},
  {0x4023, 0x3f},
  {0x4024, 0x03},
  {0x4025, 0x24},
  {0x4026, 0x03},
  {0x4027, 0x27},
  {0x402a, 0x02},
  {0x402b, 0x04},
  {0x402c, 0x06},
  {0x402d, 0x02},
  {0x402e, 0x08},
  {0x402f, 0x04},
  {0x4501, 0x3c},
  {0x4601, 0x19},
  {0x4603, 0x01},
  {0x4837, 0x19},
  {0x5401, 0x51},
  {0x5405, 0x20},
//enable color bar
  {0x5e00, 0x80},//0x00 disable
  //{0x0100, 0x01},
};


LOCAL SENSOR_REG_TAB_INFO_T s_at_ov13850_resolution_Tab_RAW[] = {
	{ADDR_AND_LEN_OF_ARRAY(ov13850_common_init), 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(ov13850_640x480_setting), 640, 480, 24, SENSOR_IMAGE_FORMAT_RAW},
	{PNULL, 0, 0, 0, 0, 0},
};

LOCAL SENSOR_TRIM_T s_at_ov13850_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	//{0, 0,  2112, 1568, 200, 640, 1664, {0, 0,  2112, 1568}},  //vts
	{0, 0, 640, 480, 174, 640, 1889, {0, 0, 640, 480}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
};


LOCAL SENSOR_IOCTL_FUNC_TAB_T s_at_ov13850_ioctl_func_tab = {
	PNULL,
	_at_ov13850_PowerOn,
	PNULL,
	_at_ov13850_Identify,

	PNULL,			// write register
	PNULL,			// read  register
	PNULL,
	_at_ov13850_GetResolutionTrimTab,

	// External
	PNULL,
	PNULL,
	PNULL,

	PNULL, //_ov13850_set_brightness,
	PNULL, // _ov13850_set_contrast,
	PNULL,
	PNULL,			//_ov13850_set_saturation,

	PNULL, //_ov13850_set_work_mode,
	PNULL, //_ov13850_set_image_effect,

	PNULL,//_ov13850_BeforeSnapshot,
	PNULL, //_ov13850_after_snapshot,
	PNULL, //_ov13850_flash,
	PNULL,
	PNULL, //_ov13850_write_exposure,
	PNULL,
	PNULL, //_ov13850_write_gain,
	PNULL,
	PNULL,
	PNULL, //_ov13850_write_af,
	PNULL,
	PNULL, //_ov13850_set_awb,
	PNULL,
	PNULL,
	PNULL, //_ov13850_set_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL, //_ov13850_GetExifInfo,
	PNULL, //_ov13850_ExtFunc,
	PNULL, //_ov13850_set_anti_flicker,
	PNULL, //_ov13850_set_video_mode,
	PNULL, //pick_jpeg_stream
	PNULL,  //meter_mode
	PNULL, //get_status
	_at_ov13850_StreamOn,
	_at_ov13850_StreamOff,
	PNULL, //_ov13850_access_val,
};


SENSOR_INFO_T g_autotest_ov13850_mipi_raw_info = {
	ov13850_I2C_ADDR_W,	// salve i2c write address
	ov13850_I2C_ADDR_R,	// salve i2c read address

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
	{{0x300A, 0xD8},		// supply two code to identify sensor.
	 {0x300B, 0x50}},		// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,	// voltage of avdd

	4208,			// max width of source image
	3120,			// max height of source image
	"ov13850",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_B,// pattern of input image form sensor;

	s_at_ov13850_resolution_Tab_RAW,	// point to resolution table information structure
	&s_at_ov13850_ioctl_func_tab,	// point to ioctl function table
	&s_at_ov13850_mipi_raw_info_ptr,		// information and table about Rawrgb sensor
	NULL,			//&g_ov13850_ext_info,                // extend information about sensor
	SENSOR_AVDD_1800MV,	// iovdd
	SENSOR_AVDD_1200MV,	// dvdd
	1,			// skip frame num before preview
	1,			// skip frame num before capture
	0,			// deci frame num during preview
	0,			// deci frame num during video preview

	0,
	0,
	0,
	0,
	0,
	{SENSOR_INTERFACE_TYPE_CSI2, 4, 10, 0},
	NULL,//s_ov13850_video_info,
	3,			// skip frame num while change setting
};

/*LOCAL struct sensor_raw_info* Sensor_GetContext(void)
{
	return s_at_ov13850_mipi_raw_info_ptr;
}
*/

LOCAL unsigned long _at_ov13850_GetResolutionTrimTab(unsigned long param)
{
	SENSOR_PRINT("0x%lx",  (unsigned long)s_at_ov13850_Resolution_Trim_Tab);
	return (unsigned long) s_at_ov13850_Resolution_Trim_Tab;
}

LOCAL unsigned long _at_ov13850_PowerOn(unsigned long power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_autotest_ov13850_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_autotest_ov13850_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_autotest_ov13850_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_autotest_ov13850_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_autotest_ov13850_mipi_raw_info.reset_pulse_level;
	//uint32_t reset_width=g_ov13850_yuv_info.reset_pulse_width;

	SENSOR_PRINT("SENSOR_OV13850: _at_ov13850_PowerOn tony");

	if (SENSOR_TRUE == power_on) {
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_SetResetLevel(reset_level);
		Sensor_PowerDown(power_down);
		usleep(10*1000);

		// Open power
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		usleep(1000);
		Sensor_SetIovddVoltage(iovdd_val);
		usleep(1000);
		Sensor_SetAvddVoltage(avdd_val);
		usleep(1000);
		Sensor_SetDvddVoltage(dvdd_val);

		usleep(10*1000);
		Sensor_PowerDown(!power_down);
		usleep(1000);
		Sensor_SetResetLevel(!reset_level);
		usleep(20*1000);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(20*1000);
		//_dw9174_SRCInit(2);

	} else {
		usleep(4*1000);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		usleep(1000);
		Sensor_SetResetLevel(reset_level);
		Sensor_PowerDown(power_down);
		usleep(1000);
		Sensor_SetDvddVoltage(SENSOR_AVDD_CLOSED);
		usleep(1000);
		Sensor_SetIovddVoltage(SENSOR_AVDD_CLOSED);
		Sensor_SetAvddVoltage(SENSOR_AVDD_CLOSED);

		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
	}
	SENSOR_PRINT("SENSOR_ov13850: _ov13850_Power_On(1:on, 0:off): %ld", power_on);
	return SENSOR_SUCCESS;
}


LOCAL uint32_t _at_ov13850_com_Identify_otp(void* param_ptr)
{
	uint32_t rtn=SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_OV13850: _at_ov13850_com_Identify_otp");

	/*read param id from sensor omap*/
	param_id=OV13850_RAW_PARAM_COM;

	if(OV13850_RAW_PARAM_COM==param_id){
		rtn=SENSOR_SUCCESS;
	}

	return rtn;
}

LOCAL uint32_t _at_ov13850_GetRawInof(void)
{
	uint32_t rtn=SENSOR_SUCCESS;
	struct raw_param_info_tab* tab_ptr = (struct raw_param_info_tab*)s_at_ov13850_raw_param_tab;
	uint32_t param_id;
	uint32_t i=0x00;

	/*read param id from sensor omap*/
	param_id=OV13850_RAW_PARAM_COM;
	SENSOR_PRINT("SENSOR_OV13850: _at_ov13850_GetRawInof tony");

	for(i=0x00; ; i++)
	{
		g_ov13850_module_id = i;
		if(RAW_INFO_END_ID==tab_ptr[i].param_id){
			if(NULL==s_at_ov13850_mipi_raw_info_ptr){
				SENSOR_PRINT("SENSOR_OV13850: ov5647_GetRawInof no param error");
				rtn=SENSOR_FAIL;
			}
			SENSOR_PRINT("SENSOR_OV13850: _at_ov13850_GetRawInof end");
			break;
		}
		else if(PNULL!=tab_ptr[i].identify_otp){
			if(SENSOR_SUCCESS==tab_ptr[i].identify_otp(0))
			{
				s_at_ov13850_mipi_raw_info_ptr = tab_ptr[i].info_ptr;
				SENSOR_PRINT("SENSOR_OV13850: _at_ov13850_GetRawInof success");
				break;
			}
		}
	}

	return rtn;
}

LOCAL unsigned long _at_ov13850_Identify(unsigned long param)
{
#define ov13850_PID_VALUE    0xD8
#define ov13850_PID_ADDR     0x300A
#define ov13850_VER_VALUE    0x50
#define ov13850_VER_ADDR     0x300B

	uint8_t pid_value = 0x00;
	uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("SENSOR_ov13850: mipi raw identify\n");
	SENSOR_PRINT("SENSOR_OV13850: _at_ov13850_Identify tony");

	pid_value = Sensor_ReadReg(ov13850_PID_ADDR);
	if (ov13850_PID_VALUE == pid_value) {
		ver_value = Sensor_ReadReg(ov13850_VER_ADDR);
		SENSOR_PRINT("sssss= %x, VER = %x", pid_value, ver_value);
		if (ov13850_VER_VALUE == ver_value) {
			SENSOR_PRINT("SENSOR_ov13850: this is ov13850 sensor !");
			ret_value=_at_ov13850_GetRawInof();
			if(SENSOR_SUCCESS != ret_value)
			{
				SENSOR_PRINT_ERR("SENSOR_ov13850: the module is unknow error !");
			}
		//	Sensor_ov13850_InitRawTuneInfo();
		} else {
			SENSOR_PRINT_HIGH("SENSOR_ov13850: Identify this is OV%x%x sensor !", pid_value, ver_value);
		}
	} else {
		SENSOR_PRINT_ERR("SENSOR_ov13850: identify fail,pid_value=%d", pid_value);
	}

	return ret_value;
}


LOCAL unsigned long _at_ov13850_StreamOn(unsigned long param)
{
	SENSOR_PRINT("SENSOR_ov13850: StreamOn tony");

	Sensor_WriteReg(0x0100, 0x01);

	return 0;
}

LOCAL unsigned long _at_ov13850_StreamOff(unsigned long param)
{
	SENSOR_PRINT("SENSOR_ov13850: StreamOff tony");

	Sensor_WriteReg(0x0100, 0x00);
	usleep(100*1000);

	return 0;
}
