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
//#include "sensor_t4kb3_raw_param_v3.c"
#include "newparam/sensor_t4kb3_raw_param_main.c"
#else
#endif
#include "sensor_t4kb3_otp.c"

#define T4KB3_I2C_ADDR_W         (0x36)
#define T4KB3_I2C_ADDR_R         (0x36)

#define T4KB3_FLIP_MIRROR
#define DW9714A_VCM_SLAVE_ADDR (0x18>>1)
#define T4KB3_RAW_PARAM_COM  0x0000
#define T4KB3_RAW_PARAM_GLOBAL  0x0004

static int s_t4kb3_gain = 0;
static int s_capture_shutter = 0;
static int s_capture_VTS = 0;
static int s_video_min_framerate = 0;
static int s_video_max_framerate = 0;
static int s_exposure_time = 0;

LOCAL unsigned long _t4kb3_GetResolutionTrimTab(unsigned long param);
LOCAL unsigned long _t4kb3_PowerOn(unsigned long power_on);
LOCAL unsigned long _t4kb3_Identify(unsigned long param);
LOCAL unsigned long _t4kb3_BeforeSnapshot(unsigned long param);
LOCAL unsigned long _t4kb3_after_snapshot(unsigned long param);
LOCAL unsigned long _t4kb3_StreamOn(unsigned long param);
LOCAL unsigned long _t4kb3_StreamOff(unsigned long param);
LOCAL unsigned long _t4kb3_write_exposure(unsigned long param);
LOCAL unsigned long _t4kb3_write_gain(unsigned long param);
LOCAL unsigned long _t4kb3_write_af(unsigned long param);
LOCAL unsigned long _t4kb3_flash(unsigned long param);
LOCAL unsigned long _t4kb3_ExtFunc(unsigned long ctl_param);
LOCAL unsigned long _dw9714a_SRCInit(unsigned long mode);
LOCAL unsigned long _t4kb3_ReadGain(unsigned long param);
LOCAL unsigned long _t4kb3_set_video_mode(unsigned long param);
LOCAL unsigned long _t4kb3_cfg_otp(unsigned long param);
LOCAL unsigned long _t4kb3_access_val(unsigned long param);
LOCAL unsigned long _t4kb3_GetExifInfo(unsigned long param);
LOCAL uint32_t _t4kb3_get_shutter(void);
LOCAL uint32_t _t4kb3_set_shutter(uint16_t shutter);
LOCAL uint32_t _t4kb3_get_VTS(void);
LOCAL uint32_t _t4kb3_set_VTS(int VTS);
LOCAL uint32_t _t4kb3_get_gain(void);
LOCAL uint32_t _t4kb3_set_gain(uint32_t gain);
LOCAL uint32_t _t4kb3_com_Identify_otp(void *param_ptr);
LOCAL uint32_t _t4kb3_read_otp_gain(uint32_t * param);
LOCAL uint32_t _t4kb3_write_otp_gain(uint32_t * param);
//LOCAL uint32_t _t4kb3_truly_identify_otp(void *param_ptr);
//LOCAL uint32_t _t4kb3_update_otp(void *param_ptr);

LOCAL const struct raw_param_info_tab s_t4kb3_raw_param_tab[] = {
	{T4KB3_RAW_PARAM_COM, &s_t4kb3_mipi_raw_info, _t4kb3_truly_identify_otp, _t4kb3_update_otp},
	{T4KB3_RAW_PARAM_COM, &s_t4kb3_mipi_raw_info, _t4kb3_com_Identify_otp, PNULL},
	{RAW_INFO_END_ID, PNULL, PNULL, PNULL}
};

struct sensor_raw_info *s_t4kb3_mipi_raw_info_ptr = NULL;

static uint32_t g_module_id = 0;

static uint32_t g_flash_mode_en = 0;
static uint32_t g_af_slewrate = 1;

LOCAL const SENSOR_REG_T t4kb3_common_init[] = {
	//{0x0103,0x01},
	//{SENSOR_WRITE_DELAY, 0x1f}, 
	{0x0100, 0x00},
	//{SENSOR_WRITE_DELAY, 0x1f}, 
	{0x0233, 0x01},		//A gain mode select
	{0x4136, 0x18},
	{0x4137, 0x00},
	{0x0305, 0x01},
	{0x0205, 0x7a},
	{0x0307, 0xdc},
	{0x4B05, 0x01},		//00
	{0x306D, 0x0A},
	{0x3094, 0x01},
	{0x3098, 0xb6},
	{0x315b, 0x34},
	{0x315c, 0x34},
	{0x315d, 0x38},
	{0x3169, 0x05},
	{0x3170, 0x77},
	{0x3171, 0x77},
	{0x31A1, 0xb2},
	{0x31A2, 0xaa},
	{0x31A3, 0x90},
	{0x31A4, 0x9a},
	{0x31A5, 0xf0},
	{0x31A6, 0xEE},
	{0x31A8, 0xF4},
	{0x3216, 0x58},
	{0x3217, 0x58},
	{0x3218, 0x58},
	{0x321A, 0x68},
	{0x321B, 0x60},
	{0x3238, 0x20},
	{0x323B, 0x40},
	{0x3243, 0x03},
	{0x3244, 0x09},
	{0x3247, 0x03},
	{0x3307, 0x1e},
	{0x3308, 0x1e},
	{0x3380, 0x01},
	{0x339E, 0x07},
	{0x354f, 0x00},
	{0x3551, 0x80},
	//{0x0100,0x01},
};

LOCAL const SENSOR_REG_T t4kb3_2112x1568_setting[] = {
};

LOCAL const SENSOR_REG_T t4kb3_4208x3120_setting[] = {
	{0x0100, 0x00},
	{0x0112, 0x0a},
	{0x0113, 0x0a},
	{0x0114, 0x03},
	{0x4136, 0x18},
	{0x4137, 0x00},
	{0x0820, 0x10},
	{0x0821, 0xb0},
	{0x0822, 0x00},
	{0x0823, 0x00},
	{0x0301, 0x01},
	{0x0303, 0x0a},
	{0x0305, 0x01},
	{0x0306, 0x00},
	{0x0307, 0x59},		//b8
	{0x030B, 0x01},
	{0x034C, 0x10},
	{0x034D, 0x70},
	{0x034E, 0x0c},
	{0x034F, 0x30},
	{0x0340, 0x0c},
	{0x0341, 0x4c},
	{0x0342, 0x11},
	{0x0343, 0x80},
	{0x0344, 0x00},
	{0x0345, 0x00},
	{0x0346, 0x00},
	{0x0347, 0x00},
	{0x0348, 0x10},
	{0x0349, 0x6f},
	{0x034A, 0x0c},
	{0x034B, 0x2f},
	{0x0408, 0x00},
	{0x0409, 0x00},
	{0x040A, 0x00},
	{0x040B, 0x00},
	{0x040C, 0x10},
	{0x040D, 0x70},
	{0x040E, 0x0c},
	{0x040F, 0x30},
	{0x0900, 0x01},
	{0x0901, 0x11},
	{0x4220, 0x00},
	{0x4222, 0x01},
	{0x3380, 0x01},
	//MCLK=24Mhz,PCLK=427.2Mhz,image=4208x3120,frame=4480x3148
	//frame_rate=30.29fps,MIPI/L=1068Mbps,1H=10.49us
};

LOCAL SENSOR_REG_TAB_INFO_T s_t4kb3_resolution_Tab_RAW[] = {
	{ADDR_AND_LEN_OF_ARRAY(t4kb3_common_init), 0, 0, 24, SENSOR_IMAGE_FORMAT_RAW},
	{ADDR_AND_LEN_OF_ARRAY(t4kb3_4208x3120_setting), 4208, 3120, 24, SENSOR_IMAGE_FORMAT_RAW},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

LOCAL SENSOR_TRIM_T s_t4kb3_Resolution_Trim_Tab[] = {
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 4208, 3120, 105, 1068, 3148, {0, 0, 4208, 3120}},

	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}}
};

LOCAL const SENSOR_REG_T s_t4kb3_2112x1568_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

LOCAL const SENSOR_REG_T s_t4kb3_4208x3120_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

LOCAL SENSOR_VIDEO_INFO_T s_t4kb3_video_info[] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{15, 15, 200, 64}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, (SENSOR_REG_T **) s_t4kb3_4208x3120_video_tab},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}
};

LOCAL unsigned long _t4kb3_set_video_mode(unsigned long param)
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

	if (PNULL == s_t4kb3_video_info[mode].setting_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	sensor_reg_ptr = (SENSOR_REG_T_PTR) & s_t4kb3_video_info[mode].setting_ptr[param];
	if (PNULL == sensor_reg_ptr) {
		SENSOR_PRINT("fail.");
		return SENSOR_FAIL;
	}

	for (i = 0x00; (0xffff != sensor_reg_ptr[i].reg_addr) || (0xff != sensor_reg_ptr[i].reg_value); i++) {
		Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	SENSOR_PRINT("0x%lx", param);
	return 0;
}

LOCAL SENSOR_IOCTL_FUNC_TAB_T s_t4kb3_ioctl_func_tab = {
	PNULL,
	_t4kb3_PowerOn,
	PNULL,
	_t4kb3_Identify,

	PNULL,			// write register
	PNULL,			// read  register
	PNULL,
	_t4kb3_GetResolutionTrimTab,

	// External
	PNULL,
	PNULL,
	PNULL,

	PNULL,			//_t4kb3_set_brightness,
	PNULL,			// _t4kb3_set_contrast,
	PNULL,
	PNULL,			//_t4kb3_set_saturation,

	PNULL,			//_t4kb3_set_work_mode,
	PNULL,			//_t4kb3_set_image_effect,

	_t4kb3_BeforeSnapshot,
	_t4kb3_after_snapshot,
	_t4kb3_flash,
	PNULL,
	_t4kb3_write_exposure,
	PNULL,
	_t4kb3_write_gain,
	PNULL,
	PNULL,
	_t4kb3_write_af,
	PNULL,
	PNULL,			//_t4kb3_set_awb,
	PNULL,
	PNULL,
	PNULL,			//_t4kb3_set_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL,			//_t4kb3_GetExifInfo,
	_t4kb3_ExtFunc,
	PNULL,			//_t4kb3_set_anti_flicker,
	_t4kb3_set_video_mode,
	PNULL,			//pick_jpeg_stream
	PNULL,			//meter_mode
	PNULL,			//get_status
	_t4kb3_StreamOn,
	_t4kb3_StreamOff,
	_t4kb3_access_val,
};

SENSOR_INFO_T g_t4kb3_mipi_raw_info = {
	T4KB3_I2C_ADDR_W,	// salve i2c write address
	T4KB3_I2C_ADDR_R,	// salve i2c read address

	SENSOR_I2C_REG_16BIT | SENSOR_I2C_VAL_8BIT | SENSOR_I2C_FREQ_400,	// bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
	// bit1: 0: i2c register addr  is 8 bit, 1: i2c register addr  is 16 bit
	// other bit: reseved
	SENSOR_HW_SIGNAL_PCLK_P | SENSOR_HW_SIGNAL_VSYNC_P | SENSOR_HW_SIGNAL_HSYNC_P,	// bit0: 0:negative; 1:positive -> polarily of pixel clock
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
	    SENSOR_IMAGE_EFFECT_YELLOW | SENSOR_IMAGE_EFFECT_NEGATIVE | SENSOR_IMAGE_EFFECT_CANVAS,

	// while balance mode
	0,

	7,			// bit[0:7]: count of step in brightness, contrast, sharpness, saturation
	// bit[8:31] reseved

	SENSOR_LOW_PULSE_RESET,	// reset pulse level
	10,			// reset pulse width(ms)

	SENSOR_LOW_LEVEL_PWDN,	// 1: high level valid; 0: low level valid

	10,			// count of identify code
	{{0x0000, 0x1c}
	 ,			// supply two code to identify sensor.
	 {0x0001, 0x50}
	 }
	,			// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,	// voltage of avdd

	4208,			// max width of source image
	3120,			// max height of source image
	"t4kb3",		// name of sensor

	SENSOR_IMAGE_FORMAT_RAW,	// define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	// if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_RAWRGB_GR,	// pattern of input image form sensor;

	s_t4kb3_resolution_Tab_RAW,	// point to resolution table information structure
	&s_t4kb3_ioctl_func_tab,	// point to ioctl function table
	&s_t4kb3_mipi_raw_info_ptr,	// information and table about Rawrgb sensor
	NULL,			//&g_t4kb3_ext_info,                // extend information about sensor
	SENSOR_AVDD_1800MV,	// iovdd
	SENSOR_AVDD_1200MV,	// dvdd
	1,			// skip frame num before preview
	2,			// skip frame num before capture
	0,			// deci frame num during preview
	0,			// deci frame num during video preview

	0,
	0,
	0,
	0,
	0,
	{SENSOR_INTERFACE_TYPE_CSI2, 4, 10, 0}
	,
	s_t4kb3_video_info,
	3,			// skip frame num while change setting
};

LOCAL struct sensor_raw_info *Sensor_GetContext(void)
{
	return s_t4kb3_mipi_raw_info_ptr;
}
/*
static struct sensor_libuse_info s_t4kb3_libuse_info = {
	0x00000080,		//use ALC AWB
	0x0,
	0x0,
	0x0,
	{0x0}
};
*/
#define param_update(x1,x2) sprintf(name,"/data/t4kb3_%s.bin",x1);\
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

LOCAL uint32_t Sensor_t4kb3_InitRawTuneInfo(void)
{
	uint32_t rtn = 0x00;

	isp_raw_para_update_from_file(&g_t4kb3_mipi_raw_info, 0);

	struct sensor_raw_info *raw_sensor_ptr = Sensor_GetContext();
	struct isp_mode_param *mode_common_ptr = raw_sensor_ptr->mode_ptr[0].addr;
	int i;
	char name[100] = { '\0' };

	//raw_sensor_ptr->libuse_info = &s_t4kb3_libuse_info;

	for (i = 0; i < mode_common_ptr->block_num; i++) {
		struct isp_block_header *header = &(mode_common_ptr->block_header[i]);
		uint8_t *data = (uint8_t *) mode_common_ptr + header->offset;
		switch (header->block_id) {
		case ISP_BLK_PRE_WAVELET_V1:{
				/* modify block data */
				struct sensor_pwd_param *block = (struct sensor_pwd_param *)data;

				static struct sensor_pwd_level pwd_param[SENSOR_SMART_LEVEL_NUM] = {
#include "noise/pwd_param.h"
				};

				param_update("pwd_param", pwd_param);

				block->param_ptr = pwd_param;
			}
			break;

		case ISP_BLK_BPC_V1:{
				/* modify block data */
				struct sensor_bpc_param_v1 *block = (struct sensor_bpc_param_v1 *)data;

				static struct sensor_bpc_level bpc_param[SENSOR_SMART_LEVEL_NUM] = {
#include "noise/bpc_param.h"
				};

				param_update("bpc_param", bpc_param);

				block->param_ptr = bpc_param;
			}
			break;

		case ISP_BLK_BL_NR_V1:{
				/* modify block data */
				struct sensor_bdn_param *block = (struct sensor_bdn_param *)data;

				static struct sensor_bdn_level bdn_param[SENSOR_SMART_LEVEL_NUM] = {
#include "noise/bdn_param.h"
				};

				param_update("bdn_param", bdn_param);

				block->param_ptr = bdn_param;
			}
			break;

		case ISP_BLK_GRGB_V1:{
				/* modify block data */
				struct sensor_grgb_v1_param *block = (struct sensor_grgb_v1_param *)data;
				static struct sensor_grgb_v1_level grgb_param[SENSOR_SMART_LEVEL_NUM] = {
#include "noise/grgb_param.h"
				};

				param_update("grgb_param", grgb_param);

				block->param_ptr = grgb_param;

			}
			break;

		case ISP_BLK_NLM:{
				/* modify block data */
				struct sensor_nlm_param *block = (struct sensor_nlm_param *)data;

				static struct sensor_nlm_level nlm_param[32] = {
#include "noise/nlm_param.h"
				};

				param_update("nlm_param", nlm_param);

				static struct sensor_vst_level vst_param[32] = {
#include "noise/vst_param.h"
				};

				param_update("vst_param", vst_param);

				static struct sensor_ivst_level ivst_param[32] = {
#include "noise/ivst_param.h"
				};

				param_update("ivst_param", ivst_param);

				static struct sensor_flat_offset_level flat_offset_param[32] = {
#include "noise/flat_offset_param.h"
				};

				param_update("flat_offset_param", flat_offset_param);

				block->param_nlm_ptr = nlm_param;
				block->param_vst_ptr = vst_param;
				block->param_ivst_ptr = ivst_param;
				block->param_flat_offset_ptr = flat_offset_param;
			}
			break;

		case ISP_BLK_CFA_V1:{
				/* modify block data */
				struct sensor_cfa_param_v1 *block = (struct sensor_cfa_param_v1 *)data;
				static struct sensor_cfae_level cfae_param[SENSOR_SMART_LEVEL_NUM] = {
#include "noise/cfae_param.h"
				};

				param_update("cfae_param", cfae_param);

				block->param_ptr = cfae_param;
			}
			break;

		case ISP_BLK_RGB_PRECDN:{
				/* modify block data */
				struct sensor_rgb_precdn_param *block = (struct sensor_rgb_precdn_param *)data;

				static struct sensor_rgb_precdn_level precdn_param[SENSOR_SMART_LEVEL_NUM] = {
#include "noise/rgb_precdn_param.h"
				};

				param_update("rgb_precdn_param", precdn_param);

				block->param_ptr = precdn_param;
			}
			break;

		case ISP_BLK_YUV_PRECDN:{
				/* modify block data */
				struct sensor_yuv_precdn_param *block = (struct sensor_yuv_precdn_param *)data;

				static struct sensor_yuv_precdn_level yuv_precdn_param[SENSOR_SMART_LEVEL_NUM] = {
#include "noise/yuv_precdn_param.h"
				};

				param_update("yuv_precdn_param", yuv_precdn_param);

				block->param_ptr = yuv_precdn_param;
			}
			break;

		case ISP_BLK_PREF_V1:{
				/* modify block data */
				struct sensor_prfy_param *block = (struct sensor_prfy_param *)data;

				static struct sensor_prfy_level prfy_param[SENSOR_SMART_LEVEL_NUM] = {
#include "noise/prfy_param.h"
				};

				param_update("prfy_param", prfy_param);

				block->param_ptr = prfy_param;
			}
			break;

		case ISP_BLK_UV_CDN:{
				/* modify block data */
				struct sensor_uv_cdn_param *block = (struct sensor_uv_cdn_param *)data;

				static struct sensor_uv_cdn_level uv_cdn_param[SENSOR_SMART_LEVEL_NUM] = {
#include "noise/yuv_cdn_param.h"
				};

				param_update("yuv_cdn_param", uv_cdn_param);

				block->param_ptr = uv_cdn_param;
			}
			break;

		case ISP_BLK_EDGE_V1:{
				/* modify block data */
				struct sensor_ee_param *block = (struct sensor_ee_param *)data;

				static struct sensor_ee_level edge_param[SENSOR_SMART_LEVEL_NUM] = {
#include "noise/edge_param.h"
				};

				param_update("edge_param", edge_param);

				block->param_ptr = edge_param;
			}
			break;

		case ISP_BLK_UV_POSTCDN:{
				/* modify block data */
				struct sensor_uv_postcdn_param *block = (struct sensor_uv_postcdn_param *)data;

				static struct sensor_uv_postcdn_level uv_postcdn_param[SENSOR_SMART_LEVEL_NUM] = {
#include "noise/yuv_postcdn_param.h"
				};

				param_update("yuv_postcdn_param", uv_postcdn_param);

				block->param_ptr = uv_postcdn_param;
			}
			break;

		case ISP_BLK_IIRCNR_IIR:{
				/* modify block data */
				struct sensor_iircnr_param *block = (struct sensor_iircnr_param *)data;

				static struct sensor_iircnr_level iir_cnr_param[SENSOR_SMART_LEVEL_NUM] = {
#include "noise/iircnr_param.h"
				};

				param_update("iircnr_param", iir_cnr_param);

				block->param_ptr = iir_cnr_param;
			}
			break;

		case ISP_BLK_IIRCNR_YRANDOM:{
				/* modify block data */
				struct sensor_iircnr_yrandom_param *block = (struct sensor_iircnr_yrandom_param *)data;
				static struct sensor_iircnr_yrandom_level iir_yrandom_param[SENSOR_SMART_LEVEL_NUM] = {
#include "noise/iir_yrandom_param.h"
				};

				param_update("iir_yrandom_param", iir_yrandom_param);

				block->param_ptr = iir_yrandom_param;
			}
			break;

		case ISP_BLK_UVDIV_V1:{
				/* modify block data */
				struct sensor_cce_uvdiv_param_v1 *block = (struct sensor_cce_uvdiv_param_v1 *)data;

				static struct sensor_cce_uvdiv_level cce_uvdiv_param[SENSOR_SMART_LEVEL_NUM] = {
#include "noise/cce_uv_param.h"
				};

				param_update("cce_uv_param", cce_uvdiv_param);

				block->param_ptr = cce_uvdiv_param;
			}
			break;
		case ISP_BLK_YIQ_AFM:{
				/* modify block data */
				struct sensor_y_afm_param *block = (struct sensor_y_afm_param *)data;

				static struct sensor_y_afm_level y_afm_param[SENSOR_SMART_LEVEL_NUM] = {
#include "noise/y_afm_param.h"
				};

				param_update("y_afm_param", y_afm_param);

				block->param_ptr = y_afm_param;
			}
			break;

		default:
			break;
		}
	}

	return rtn;
}

LOCAL unsigned long _t4kb3_GetResolutionTrimTab(unsigned long param)
{
	SENSOR_PRINT("0x%lx", (unsigned long)s_t4kb3_Resolution_Trim_Tab);
	return (unsigned long)s_t4kb3_Resolution_Trim_Tab;
}

LOCAL unsigned long _t4kb3_PowerOn(unsigned long power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_t4kb3_mipi_raw_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_t4kb3_mipi_raw_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_t4kb3_mipi_raw_info.iovdd_val;
	BOOLEAN power_down = g_t4kb3_mipi_raw_info.power_down_level;
	BOOLEAN reset_level = g_t4kb3_mipi_raw_info.reset_pulse_level;
	//uint32_t reset_width=g_t4kb3_yuv_info.reset_pulse_width;

	if (SENSOR_TRUE == power_on) {
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetMonitorVoltage(SENSOR_AVDD_CLOSED);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_SetResetLevel(reset_level);
		Sensor_PowerDown(power_down);
		usleep(2 * 1000);

		// Open power
		Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
		usleep(10 * 1000);
		Sensor_SetIovddVoltage(iovdd_val);
		usleep(10 * 1000);
		Sensor_SetAvddVoltage(avdd_val);
		usleep(10 * 1000);
		Sensor_SetDvddVoltage(dvdd_val);
		usleep(10 * 1000);

		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(10 * 1000);
		Sensor_PowerDown(!power_down);
		usleep(50 * 1000);
		Sensor_SetResetLevel(!reset_level);
		usleep(50 * 1000);

		_dw9714a_SRCInit(2);

	} else {
		usleep(1000);
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
	SENSOR_PRINT("SENSOR_T4KB3: _t4kb3_Power_On(1:on, 0:off): %ld", power_on);
	return SENSOR_SUCCESS;
}

LOCAL unsigned long _t4kb3_cfg_otp(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct raw_param_info_tab *tab_ptr = (struct raw_param_info_tab *)s_t4kb3_raw_param_tab;
	uint32_t module_id = g_module_id;

	SENSOR_PRINT("SENSOR_T4KB3: _t4kb3_cfg_otp");

	if (PNULL != tab_ptr[module_id].cfg_otp) {
		tab_ptr[module_id].cfg_otp(0);
	}

	return rtn;
}

LOCAL unsigned long _t4kb3_access_val(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_VAL_T *param_ptr = (SENSOR_VAL_T *) param;
	uint16_t tmp;

	SENSOR_PRINT("SENSOR_T4KB3: cfg_otp E");
	if (!param_ptr) {
		rtn = _t4kb3_update_otp(0);
		if (rtn) {
			SENSOR_PRINT("SENSOR_T4KB3: _t4kb3_update_otp failed");
		}
		return rtn;
	}

	SENSOR_PRINT("SENSOR_T4KB3: param_ptr->type=%x", param_ptr->type);
	switch (param_ptr->type) {
	case SENSOR_VAL_TYPE_SHUTTER:
		*((uint32_t *) param_ptr->pval) = _t4kb3_get_shutter();
		break;
	case SENSOR_VAL_TYPE_READ_OTP_GAIN:
		rtn = _t4kb3_read_otp_gain(param_ptr->pval);
		break;
	default:
		break;
	}

	SENSOR_PRINT("SENSOR_T4KB3: cfg_otp X");

	return rtn;
}

LOCAL uint32_t _t4kb3_com_Identify_otp(void *param_ptr)
{
	uint32_t rtn = SENSOR_FAIL;
	uint32_t param_id;

	SENSOR_PRINT("SENSOR_T4KB3: _t4kb3_com_Identify_otp");

	/*read param id from sensor omap */
	param_id = T4KB3_RAW_PARAM_COM;

	if (T4KB3_RAW_PARAM_COM == param_id) {
		rtn = SENSOR_SUCCESS;
	}

	return rtn;
}

LOCAL uint32_t _t4kb3_GetRawInof(void)
{
	uint32_t rtn = SENSOR_SUCCESS;
	struct raw_param_info_tab *tab_ptr = (struct raw_param_info_tab *)s_t4kb3_raw_param_tab;
	uint32_t param_id;
	uint32_t i = 0x00;

	/*read param id from sensor omap */
	param_id = T4KB3_RAW_PARAM_COM;

	for (i = 0x00;; i++) {
		g_module_id = i;
		if (RAW_INFO_END_ID == tab_ptr[i].param_id) {
			if (NULL == s_t4kb3_mipi_raw_info_ptr) {
				SENSOR_PRINT("SENSOR_T4KB3: t4kb3_GetRawInof no param error");
				rtn = SENSOR_FAIL;
			}
			SENSOR_PRINT("SENSOR_T4KB3: t4kb3_GetRawInof end");
			break;
		} else if (PNULL != tab_ptr[i].identify_otp) {
			if (SENSOR_SUCCESS == tab_ptr[i].identify_otp(0)) {
				s_t4kb3_mipi_raw_info_ptr = tab_ptr[i].info_ptr;
				SENSOR_PRINT("SENSOR_T4KB3: t4kb3_GetRawInof success");
				break;
			}
		}
	}

	return rtn;
}

LOCAL unsigned long _t4kb3_GetMaxFrameLine(unsigned long index)
{
	uint32_t max_line = 0x00;
	SENSOR_TRIM_T_PTR trim_ptr = s_t4kb3_Resolution_Trim_Tab;

	max_line = trim_ptr[index].frame_line;

	return max_line;
}

LOCAL unsigned long _t4kb3_Identify(unsigned long param)
{
#define T4KB3_PID_ADDR_H     0x0000
#define T4KB3_PID_VALUE_H    0x1c
#define T4KB3_PID_ADDR_L     0x0001
#define T4KB3_PID_VALUE_L    0x50
//#define T4KB3_VER_VALUE      0xB2
//#define T4KB3_VER_ADDR       0x302A

	uint8_t pid_value_h = 0x00;
	uint8_t pid_value_l = 0x00;
	//uint8_t ver_value = 0x00;
	uint32_t ret_value = SENSOR_FAIL;

	SENSOR_PRINT("SENSOR_T4KB3: mipi raw identify\n");

	pid_value_h = Sensor_ReadReg(T4KB3_PID_ADDR_H);
	pid_value_l = Sensor_ReadReg(T4KB3_PID_ADDR_L);
	SENSOR_PRINT("SENSOR_T4KB3: Identify: PID = %x%x", pid_value_h, pid_value_l);
	if (T4KB3_PID_VALUE_H == pid_value_h && T4KB3_PID_VALUE_L == pid_value_l) {
		//ver_value = Sensor_ReadReg(T4KB3_VER_ADDR);
		//SENSOR_PRINT("SENSOR_T4KB3: Identify: PID = %x%x, VER = %x", pid_value_h, pid_value_l, ver_value);
		//if (T4KB3_VER_VALUE == ver_value) {
		//SENSOR_PRINT("SENSOR_T4KB3: this is t4kb3 %x sensor !", ver_value);
		ret_value = _t4kb3_GetRawInof();
		if (SENSOR_SUCCESS != ret_value) {
			SENSOR_PRINT_ERR("SENSOR_T4KB3: the module is unknow error !");
		}
		Sensor_t4kb3_InitRawTuneInfo();
		//} else {
		//      SENSOR_PRINT_HIGH("SENSOR_T4KB3: Identify this is OV%x%x %x sensor !", pid_value_h, pid_value_l, ver_value);
		//}
	} else {
		SENSOR_PRINT_ERR("SENSOR_T4KB3: identify fail,pid_value_h=%x, pid_value_l=%x", pid_value_h,
				 pid_value_l);
	}

	return ret_value;
}

LOCAL unsigned long _t4kb3_write_exposure(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t expsure_line = 0x00;
	uint16_t dummy_line = 0x00;
	uint16_t size_index = 0x00;
	uint16_t frame_len = 0x00;
	uint16_t frame_len_cur = 0x00;
	uint16_t max_frame_len = 0x00;
	uint16_t value = 0x00;
	uint16_t value0 = 0x00;
	uint16_t value1 = 0x00;
	uint16_t value2 = 0x00;
	uint32_t linetime = 0;

	expsure_line = param & 0xffff;
	dummy_line = (param >> 0x10) & 0x0fff;
	size_index = (param >> 0x1c) & 0x0f;

	SENSOR_PRINT("SENSOR_T4KB3: write_exposure line:%d, dummy:%d, size_index:%d", expsure_line, dummy_line,
		     size_index);

	max_frame_len = _t4kb3_GetMaxFrameLine(size_index);
	if (0x00 != max_frame_len) {
		frame_len = ((expsure_line + 6) > max_frame_len) ? (expsure_line + 6) : max_frame_len;

		frame_len_cur = _t4kb3_get_VTS();

		SENSOR_PRINT("SENSOR_T4KB3: frame_len: %d,   frame_len_cur:%d\n", frame_len, frame_len_cur);

		if (frame_len_cur != frame_len) {
			_t4kb3_set_VTS(frame_len);
		}
	}
	_t4kb3_set_shutter(expsure_line);

	s_capture_shutter = expsure_line;
	linetime = s_t4kb3_Resolution_Trim_Tab[size_index].line_time;
	s_exposure_time = s_capture_shutter * linetime / 10;
	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, s_capture_shutter);

	return ret_value;
}

LOCAL unsigned long _t4kb3_write_gain(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint16_t value = 0x00;
	uint32_t real_gain = 0;

	SENSOR_PRINT("SENSOR_T4KB3: param: 0x%x", param);
	_t4kb3_set_gain(param);
/*
	real_gain = param;
	if (real_gain > 0xf8) {
		real_gain = 0xf8;
	}

	SENSOR_PRINT("SENSOR_T4KB3: real_gain:0x%x, param: 0x%x", real_gain, param);
	//_t4kb3_set_gain(real_gain);
	value = (real_gain >> 8) & 0xff;
	Sensor_WriteReg(0x0234, value);

	value = real_gain & 0xff;
	Sensor_WriteReg(0x0235, value);
	_t4kb3_group_hold_off();
*/
	return ret_value;
}

LOCAL unsigned long _t4kb3_write_af(unsigned long param)
{
	uint32_t ret_value = SENSOR_SUCCESS;
	uint8_t cmd_val[2] = { 0x00 };
	uint16_t slave_addr = 0;
	uint16_t cmd_len = 0;

	SENSOR_PRINT("SENSOR_T4KB3: _write_af %ld", param);

	slave_addr = DW9714A_VCM_SLAVE_ADDR;
	cmd_val[0] = (param & 0xfff0) >> 4;
	cmd_val[1] = ((param & 0x0f) << 4) | 0x09;
	cmd_len = 2;
	ret_value = Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);
	SENSOR_PRINT("SENSOR_T4KB3: _write_af, ret =  %d, MSL:%x, LSL:%x\n", ret_value, cmd_val[0], cmd_val[1]);

	return ret_value;

}

LOCAL unsigned long _t4kb3_BeforeSnapshot(unsigned long param)
{
	uint8_t ret_l, ret_h;
	uint32_t capture_exposure, preview_maxline;
	uint32_t capture_maxline, preview_exposure;
	uint32_t capture_mode = param & 0xffff;
	uint32_t preview_mode = (param >> 0x10) & 0xffff;
	uint32_t prv_linetime = s_t4kb3_Resolution_Trim_Tab[preview_mode].line_time;
	uint32_t cap_linetime = s_t4kb3_Resolution_Trim_Tab[capture_mode].line_time;

	SENSOR_PRINT("SENSOR_T4KB3: BeforeSnapshot mode: 0x%08x", param);

	if (preview_mode == capture_mode) {
		SENSOR_PRINT("SENSOR_T4KB3: prv mode equal to capmode");
		goto CFG_INFO;
	}

	ret_h = (uint8_t) Sensor_ReadReg(0x0202);
	ret_l = (uint8_t) Sensor_ReadReg(0x0203);
	preview_exposure = (ret_h << 8) + ret_l;

	ret_h = (uint8_t) Sensor_ReadReg(0x0340);
	ret_l = (uint8_t) Sensor_ReadReg(0x0341);
	preview_maxline = (ret_h << 8) + ret_l;

	Sensor_SetMode(capture_mode);
	Sensor_SetMode_WaitDone();

	SENSOR_PRINT("SENSOR_T4KB3: prv_linetime = %d   cap_linetime = %d\n", prv_linetime, cap_linetime);

	if (prv_linetime == cap_linetime) {
		SENSOR_PRINT("SENSOR_T4KB3: prvline equal to capline");
		//goto CFG_INFO;
	}

	ret_h = (uint8_t) Sensor_ReadReg(0x0340);
	ret_l = (uint8_t) Sensor_ReadReg(0x0341);
	capture_maxline = (ret_h << 8) + ret_l;

	capture_exposure = preview_exposure * prv_linetime / cap_linetime;
	//capture_exposure *= 2;

	if (0 == capture_exposure) {
		capture_exposure = 1;
	}
	SENSOR_PRINT("SENSOR_T4KB3: capture_exposure = %d   capture_maxline = %d\n", capture_exposure, capture_maxline);

	if (capture_exposure > (capture_maxline - 4)) {
		capture_maxline = capture_exposure + 4;
		ret_l = (unsigned char)(capture_maxline & 0x0ff);
		ret_h = (unsigned char)((capture_maxline >> 8) & 0xff);
		Sensor_WriteReg(0x0340, ret_h);
		Sensor_WriteReg(0x0341, ret_l);
	}
	ret_l = ((unsigned char)capture_exposure & 0xff);
	ret_h = (unsigned char)((capture_exposure >> 8) & 0xff);

	Sensor_WriteReg(0x0202, ret_l);
	Sensor_WriteReg(0x0203, ret_h);
	usleep(200 * 1000);
	SENSOR_PRINT("SENSOR_T4KB3: exposure update");

      CFG_INFO:
	s_capture_shutter = _t4kb3_get_shutter();
	s_capture_VTS = _t4kb3_get_VTS();
	_t4kb3_ReadGain(capture_mode);
	s_exposure_time = s_capture_shutter * cap_linetime / 10;
	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_EXPOSURETIME, s_capture_shutter);

	return SENSOR_SUCCESS;
}

LOCAL unsigned long _t4kb3_after_snapshot(unsigned long param)
{
	SENSOR_PRINT("SENSOR_T4KB3: after_snapshot mode:%ld", param);
	Sensor_SetMode((uint32_t) param);
	return SENSOR_SUCCESS;
}

LOCAL unsigned long _t4kb3_GetExifInfo(unsigned long param)
{
	LOCAL EXIF_SPEC_PIC_TAKING_COND_T sexif;

	sexif.ExposureTime.numerator = s_exposure_time;
	sexif.ExposureTime.denominator = 1000000;

	return (unsigned long)&sexif;
}

LOCAL unsigned long _t4kb3_flash(unsigned long param)
{
	SENSOR_PRINT("SENSOR_T4KB3: param=%d", param);

	/* enable flash, disable in _t4kb3_BeforeSnapshot */
	g_flash_mode_en = param;
	Sensor_SetFlash(param);
	SENSOR_PRINT("end");
	return SENSOR_SUCCESS;
}

LOCAL unsigned long _t4kb3_StreamOn(unsigned long param)
{
	SENSOR_PRINT("SENSOR_T4KB3: StreamOn");

	Sensor_WriteReg(0x0100, 0x01);
	usleep(20 * 1000);

	return 0;
}

LOCAL unsigned long _t4kb3_StreamOff(unsigned long param)
{
	SENSOR_PRINT("SENSOR_T4KB3: StreamOff");

	Sensor_WriteReg(0x0100, 0x00);
	usleep(20 * 1000);

	return 0;
}

LOCAL void _t4kb3_group_hold_on(void)
{
	Sensor_WriteReg(0x0104, 0x01);

}

LOCAL void _t4kb3_group_hold_off(void)
{
	Sensor_WriteReg(0x0104, 0x00);

}

LOCAL uint32_t _t4kb3_get_shutter(void)
{
	// read shutter, in number of line period
	int shutter;

	shutter = (Sensor_ReadReg(0x0202) & 0xff);
	shutter = (shutter << 8) + Sensor_ReadReg(0x0203);

	return shutter;
}

LOCAL uint32_t _t4kb3_set_shutter(uint16_t shutter)
{
	// write shutter, in number of line period
	int temp;
	_t4kb3_group_hold_on();
	SENSOR_PRINT_HIGH("Sensor_group_hold_on!");

	shutter = shutter & 0xffff;

	temp = (shutter >> 8) & 0xff;
	Sensor_WriteReg(0x0202, temp);

	temp = shutter & 0xff;
	Sensor_WriteReg(0x0203, temp);

	return 0;
}

LOCAL uint32_t _t4kb3_get_VTS(void)
{
	// read VTS from register settings
	int VTS;

	VTS = Sensor_ReadReg(0x0340);	//total vertical size[15:8] high byte

	VTS = (VTS << 8) + Sensor_ReadReg(0x0341);

	return VTS;
}

LOCAL uint32_t _t4kb3_set_VTS(int VTS)
{
	// write VTS to registers
	int temp;

	temp = VTS & 0xff;
	Sensor_WriteReg(0x0341, temp);

	temp = VTS >> 8;
	Sensor_WriteReg(0x0340, temp);

	return 0;
}

LOCAL uint32_t _t4kb3_get_gain(void)
{
	// read gain, 128 = 1x
	int gain;

	gain = Sensor_ReadReg(0x0234) & 0xff;
	gain = (gain << 8) + Sensor_ReadReg(0x0235);

	return gain;
}

LOCAL uint32_t _t4kb3_set_gain(uint32_t gain)
{
	// write gain, 128 = 1x
	int temp;
	gain = gain & 0xffff;

	temp = (gain >> 8) & 0xff;
	Sensor_WriteReg(0x0234, temp);

	temp = gain & 0xff;
	Sensor_WriteReg(0x0235, temp);

	_t4kb3_group_hold_off();
	SENSOR_PRINT_HIGH("Sensor_group_hold_off!");

	return 0;
}

LOCAL unsigned long _t4kb3_ReadGain(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
/*	uint16_t value = 0x00;
	uint32_t gain_h = 0;
	uint32_t gain_l = 0;
	uint32_t gain = 0;

	value = Sensor_ReadReg(0x0235);	
	gain_l = value & 0xff;
	value = Sensor_ReadReg(0x0234);	
	gain_h |= value & 0xff;

	gain = ((gain_h << 8) | gain_l);
*/
	s_t4kb3_gain = _t4kb3_get_gain();

	SENSOR_PRINT("SENSOR_T4KB3: _t4kb3_ReadGain gain: 0x%x", s_t4kb3_gain);

	return rtn;
}

static void _calculate_hdr_exposure(int capture_gain16, int capture_VTS, int capture_shutter)
{
	// write capture shutter
	/*if (capture_shutter > (capture_VTS - 4)) {
	   capture_VTS = capture_shutter + 4;
	   t4kb3_set_VTS(capture_VTS);
	   } */
	_t4kb3_set_shutter((uint16_t) capture_shutter);

	// write capture gain
	_t4kb3_set_gain(capture_gain16);
}

static unsigned long _t4kb3_SetEV(unsigned long param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) param;

	uint16_t value = 0x00;
	uint32_t gain = s_t4kb3_gain;
	uint32_t ev = ext_ptr->param;

	SENSOR_PRINT("SENSOR_T4KB3: _t4kb3_SetEV param: 0x%x", ext_ptr->param);

	switch (ev) {
	case SENSOR_HDR_EV_LEVE_0:
		_calculate_hdr_exposure(s_t4kb3_gain, s_capture_VTS, s_capture_shutter / 4);
		break;
	case SENSOR_HDR_EV_LEVE_1:
		_calculate_hdr_exposure(s_t4kb3_gain, s_capture_VTS, s_capture_shutter);
		break;
	case SENSOR_HDR_EV_LEVE_2:
		_calculate_hdr_exposure(s_t4kb3_gain, s_capture_VTS, s_capture_shutter * 4);
		break;
	default:
		break;
	}
	usleep(50 * 1000);
	return rtn;
}

LOCAL unsigned long _t4kb3_ExtFunc(unsigned long ctl_param)
{
	uint32_t rtn = SENSOR_SUCCESS;
	SENSOR_EXT_FUN_PARAM_T_PTR ext_ptr = (SENSOR_EXT_FUN_PARAM_T_PTR) ctl_param;
	SENSOR_PRINT("0x%x", ext_ptr->cmd);

	switch (ext_ptr->cmd) {
	case SENSOR_EXT_FUNC_INIT:
		break;
	case SENSOR_EXT_FOCUS_START:
		break;
	case SENSOR_EXT_EXPOSURE_START:
		break;
	case SENSOR_EXT_EV:
		rtn = _t4kb3_SetEV(ctl_param);
		break;
	default:
		break;
	}
	return rtn;
}

LOCAL unsigned long _dw9714a_SRCInit(unsigned long mode)
{
	uint8_t cmd_val[6] = { 0x00 };
	uint16_t slave_addr = 0;
	uint16_t cmd_len = 0;
	uint32_t ret_value = SENSOR_SUCCESS;
	SENSOR_PRINT("SENSOR_T4KB3: %d", mode);

	slave_addr = DW9714A_VCM_SLAVE_ADDR;
	Sensor_SetMonitorVoltage(SENSOR_AVDD_2800MV);
	usleep(12 * 1000);

	switch (mode) {
	case 1:
		break;

	case 2:
		{
			cmd_val[0] = 0xa1;
			cmd_val[1] = 0x05;
			cmd_val[2] = 0xf2;
			cmd_val[3] = 0x00;
			cmd_val[4] = 0xdc;
			cmd_val[5] = 0x51;
			cmd_len = 6;
			Sensor_WriteI2C(slave_addr, (uint8_t *) & cmd_val[0], cmd_len);
		}
		break;
	case 3:
		break;

	}

	return ret_value;
}

LOCAL uint32_t _t4kb3_read_otp_gain(uint32_t * param)
{
	uint32_t rtn = SENSOR_SUCCESS;
#if 0
	uint16_t value = 0x00;
	uint32_t gain = 0;

	value = Sensor_ReadReg(0x350b);	/*0-7 */
	gain = value & 0xff;
	value = Sensor_ReadReg(0x350a);	/*8 */
	gain |= (value << 0x08) & 0x300;
#else
	*param = _t4kb3_get_gain();
#endif

	return rtn;
}
