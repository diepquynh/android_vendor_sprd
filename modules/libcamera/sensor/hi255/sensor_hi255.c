/******************************************************************************
 ** Copyright (c)
 ** File Name:        sensor_HI255.c                                           *
 ** Author:                                                       *
 ** DATE:                                                              *
 ** Description:   This file contains driver for sensor HI255.
 **
 ******************************************************************************

 ******************************************************************************
 **                        Edit History                                       *
 ** ------------------------------------------------------------------------- *
 ** DATE           NAME             DESCRIPTION                               *
 **
 ******************************************************************************/

/**---------------------------------------------------------------------------*
 **                         Dependencies                                      *
 **---------------------------------------------------------------------------*/
#define LOG_TAG __FILE__

#include <utils/Log.h>
#include "sensor.h"
#include "jpeg_exif_header.h"
#include "sensor_drv_u.h"
#include <utils/Timers.h>

//#include "sensor_hi255_mipi_d98x.h"
//#include "sensor_hi255_mipi_w300.h"
/*lint -save -e765 */
/**--------------------------------------------------------------------------*
 **                         Compiler Flag                                      *
 **---------------------------------------------------------------------------*/
#ifdef     __cplusplus
extern     "C"
{
#endif

/**---------------------------------------------------------------------------*
 **                         Const variables                                   *
 **---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
 **                            Macro Define
 **---------------------------------------------------------------------------*/
#define HI255_I2C_ADDR_W        0x20//0x20
#define HI255_I2C_ADDR_R         0x20//0x20
#define I2C_WRITE_BURST_LENGTH    512

static uint32_t  g_flash_mode_en = 0;
static uint32_t  setmode =0;
static EXIF_SPEC_PIC_TAKING_COND_T s_HI255_exif;

/**---------------------------------------------------------------------------*
 **                     Local Function Prototypes                              *
 **---------------------------------------------------------------------------*/
LOCAL uint32_t _HI255_InitExifInfo(void);
//LOCAL uint32_t _HI255_GetResolutionTrimTab(uint32_t param);
LOCAL unsigned long _HI255_PowerOn(unsigned long power_on);
LOCAL unsigned long _HI255_Identify(unsigned long param);
LOCAL unsigned long _HI255_set_brightness(unsigned long level);
LOCAL unsigned long _HI255_set_saturation(unsigned long level);
LOCAL unsigned long _HI255_set_image_effect(unsigned long effect_type);
LOCAL unsigned long _HI255_set_anti_flicker(unsigned long mode);
LOCAL unsigned long _HI255_set_video_mode(unsigned long mode);
LOCAL unsigned long _HI255_set_awb(unsigned long mode);
LOCAL unsigned long _HI255_set_work_mode(unsigned long mode);
LOCAL unsigned long _HI255_BeforeSnapshot(unsigned long param);
LOCAL unsigned long _HI255_check_image_format_support(unsigned long param);
LOCAL unsigned long _HI255_pick_out_jpeg_stream(unsigned long param);
LOCAL unsigned long _HI255_after_snapshot(unsigned long param);
//LOCAL uint32_t _ov540_flash(uint32_t param);
LOCAL unsigned long _HI255_GetExifInfo(unsigned long param);
//LOCAL uint32_t _HI255_ExtFunc(uint32_t ctl_param);
LOCAL unsigned long HI255_InitExt(unsigned long param);
LOCAL uint32_t sensor_InitTflash(uint32_t param);
LOCAL unsigned long _hi255_StreamOn(unsigned long param);
LOCAL unsigned long _hi255_StreamOff(unsigned long param);
static unsigned long _HI255_GetResolutionTrimTab(unsigned long param);


LOCAL const SENSOR_REG_T HI255_common[]=
{
	{0x01, 0x31,}, /* sleep on */
	{0x01, 0x33,}, /* sleep on */
	{0x01, 0x31,}, /* sleep on */
	{0x08, 0x2f,}, /* sleep on */
	{0x0a, 0x00,}, /* sleep on */

	/* PAGE 20 */
	{0x03, 0x20,}, /* page 20 */
	{0x10, 0x1c,}, /* AE off 50hz */

	/* PAGE 22 */
	{0x03, 0x22,}, /* page 22 */
	{0x10, 0x69,}, /* AWB off */

	{0x03, 0x12,},
	{0x20, 0x00,},
	{0x21, 0x00,},

	{0x03, 0x13,},
	{0x10, 0xcb,},

	/* Initial Start */
	/* PAGE 0 START */
	{0x03, 0x00,},
	{0x10, 0x11,}, /* Vsync Active High B:[3], Sub1/2 + Preview 1mode */
	{0x11, 0x90,},
	{0x12, 0x04,}, /* Pclk Falling Edge B:[2] */

	{0x0b, 0xaa,}, /* ESD Check Register */
	{0x0c, 0xaa,}, /* ESD Check Register */
	{0x0d, 0xaa,}, /* ESD Check Register */

	{0x20, 0x00,},
	{0x21, 0x02,}, /* modify 20110929, 0x04 -> 0x02 */
	{0x22, 0x00,},
	{0x23, 0x0a,}, /* modify 20110929, 0x14 -> 0x0a */

	{0x24, 0x04,},
	{0x25, 0xb0,},
	{0x26, 0x06,},
	{0x27, 0x40,},

	{0x28, 0x0c,},
	{0x29, 0x04,},
	{0x2a, 0x02,},
	{0x2b, 0x04,},
	{0x2c, 0x06,},
	{0x2d, 0x02,},

	{0x40, 0x01,}, /* Hblank_360 */
	{0x41, 0x68,},
	{0x42, 0x00,},
	{0x43, 0x58,}, /* 88 */

	{0x44, 0x09,}, /* VSCLIP */

	{0x45, 0x04,},
	{0x46, 0x18,},
	{0x47, 0xd8,},

	/* BLC */
	{0x80, 0x2e,},
	{0x81, 0x7e,},
	{0x82, 0x90,},
	{0x83, 0x00,},
	{0x84, 0x0c,},
	{0x85, 0x00,},
	{0x90, 0x0c,}, /* BLC_TIME_TH_ON */
	{0x91, 0x0c,}, /* BLC_TIME_TH_OFF */
	{0x92, 0xc8,}, /* BLC_AG_TH_ON */
	{0x93, 0xc0,}, /* BLC_AG_TH_OFF */
	{0x94, 0xff,},
	{0x95, 0xff,},
	{0x96, 0xdc,},
	{0x97, 0xfe,},
	{0x98, 0x38,},

	/* Dark BLC */
	{0xa0, 0x00,},
	{0xa2, 0x00,},
	{0xa4, 0x00,},
	{0xa6, 0x00,},

	/* Normal BLC */
	{0xa8, 0x43,}, /* B */
	{0xaa, 0x43,},
	{0xac, 0x43,},
	{0xae, 0x43,},

	/* OutDoor BLC */
	{0x99, 0x43,},
	{0x9a, 0x43,},
	{0x9b, 0x43,},
	{0x9c, 0x43,},
	/* PAGE 0 END */

	/* PAGE 2 START */
	{0x03, 0x02,},
	{0x12, 0x03,},
	{0x13, 0x03,},
	{0x16, 0x00,},
	{0x17, 0x8C,},
	{0x18, 0x4c,}, /* Double_AG */
	{0x19, 0x00,},
	{0x1a, 0x39,}, /* Double_AG 38 -> 39 */
	{0x1c, 0x09,},
	{0x1d, 0x40,},
	{0x1e, 0x30,},
	{0x1f, 0x10,},

	{0x20, 0x77,},
	{0x21, 0xde,},
	{0x22, 0xa7,},
	{0x23, 0x30,}, /* CLAMP */
	{0x27, 0x3c,},
	{0x2b, 0x80,},
	{0x2e, 0x00,},
	{0x2f, 0x00,},
	{0x30, 0x05,}, /* For Hi-253 never no change 0x05 */

	{0x50, 0x20,},
	{0x51, 0x1c,}, /* add 20110826 */
	{0x52, 0x01,}, /* 0x03 -> 0x01 */
	{0x53, 0xc1,}, /* add 20110818 */
	{0x54, 0xc0,},
	{0x55, 0x1c,},
	{0x56, 0x11,},
	{0x58, 0x22,}, /*add 20120430 */
	{0x59, 0x20,}, /*add 20120430 */
	{0x5d, 0xa2,},
	{0x5e, 0x5a,},

	{0x60, 0x87,},
	{0x61, 0x99,},
	{0x62, 0x88,},
	{0x63, 0x97,},
	{0x64, 0x88,},
	{0x65, 0x97,},

	{0x67, 0x0c,},
	{0x68, 0x0c,},
	{0x69, 0x0c,},

	{0x72, 0x89,},
	{0x73, 0x96,},
	{0x74, 0x89,},
	{0x75, 0x96,},
	{0x76, 0x89,},
	{0x77, 0x96,},

	{0x7c, 0x85,},
	{0x7d, 0xaf,},
	{0x80, 0x01,},
	{0x81, 0x7f,},
	{0x82, 0x13,},
	{0x83, 0x24,},
	{0x84, 0x7d,},
	{0x85, 0x81,},
	{0x86, 0x7d,},
	{0x87, 0x81,},

	{0x92, 0x48,},
	{0x93, 0x54,},
	{0x94, 0x7d,},
	{0x95, 0x81,},
	{0x96, 0x7d,},
	{0x97, 0x81,},

	{0xa0, 0x02,},
	{0xa1, 0x7b,},
	{0xa2, 0x02,},
	{0xa3, 0x7b,},
	{0xa4, 0x7b,},
	{0xa5, 0x02,},
	{0xa6, 0x7b,},
	{0xa7, 0x02,},

	{0xa8, 0x85,},
	{0xa9, 0x8c,},
	{0xaa, 0x85,},
	{0xab, 0x8c,},
	{0xac, 0x10,},
	{0xad, 0x16,},
	{0xae, 0x10,},
	{0xaf, 0x16,},

	{0xb0, 0x99,},
	{0xb1, 0xa3,},
	{0xb2, 0xa4,},
	{0xb3, 0xae,},
	{0xb4, 0x9b,},
	{0xb5, 0xa2,},
	{0xb6, 0xa6,},
	{0xb7, 0xac,},
	{0xb8, 0x9b,},
	{0xb9, 0x9f,},
	{0xba, 0xa6,},
	{0xbb, 0xaa,},
	{0xbc, 0x9b,},
	{0xbd, 0x9f,},
	{0xbe, 0xa6,},
	{0xbf, 0xaa,},

	{0xc4, 0x2c,},
	{0xc5, 0x43,},
	{0xc6, 0x63,},
	{0xc7, 0x79,},

	{0xc8, 0x2d,},
	{0xc9, 0x42,},
	{0xca, 0x2d,},
	{0xcb, 0x42,},
	{0xcc, 0x64,},
	{0xcd, 0x78,},
	{0xce, 0x64,},
	{0xcf, 0x78,},
	{0xd0, 0x0a,},
	{0xd1, 0x09,},
	{0xd4, 0x0c,}, /* DCDC_TIME_TH_ON */
	{0xd5, 0x0c,}, /* DCDC_TIME_TH_OFF */
	{0xd6, 0xc8,}, /* DCDC_AG_TH_ON */
	{0xd7, 0xc0,}, /* DCDC_AG_TH_OFF */
	{0xe0, 0xc4,},
	{0xe1, 0xc4,},
	{0xe2, 0xc4,},
	{0xe3, 0xc4,},
	{0xe4, 0x00,},
	{0xe8, 0x80,},
	{0xe9, 0x40,},
	{0xea, 0x7f,},

	{0xf0, 0x01,},
	{0xf1, 0x01,},
	{0xf2, 0x01,},
	{0xf3, 0x01,},
	{0xf4, 0x01,},
	/* PAGE 2 END */

	/* PAGE 3 */
	{0x03, 0x03,},
	{0x10, 0x10,},
	/* PAGE 3 END */

	/* PAGE 10 START */
	{0x03, 0x10,},
	{0x10, 0x01,}, /* 00: CrYCbY, 01: CbYCrY */
	{0x12, 0x30,},
	{0x20, 0x00,},
	{0x30, 0x00,},
	{0x31, 0x00,},
	{0x32, 0x00,},
	{0x33, 0x00,},

	{0x34, 0x30,},
	{0x35, 0x00,},
	{0x36, 0x00,},
	{0x38, 0x00,},
	{0x3e, 0x58,},

	{0x40, 0x80,},
	{0x41, 0x00,},

	{0x60, 0x6b,},
	{0x61, 0x7a,}, /* 77 */
	{0x62, 0x76,}, /* 77 */
	{0x63, 0xa0,}, /* Double_AG 50 -> 30 */
	{0x64, 0x80,},

	{0x66, 0x42,},
	{0x67, 0x00,},

	{0x6a, 0x8a,}, /* 8a */
	{0x6b, 0x74,}, /* 74 */
	{0x6c, 0x71,}, /* 7e */
	{0x6d, 0x8e,}, /* 8e */
	{0x76, 0x01,}, /* ADD 20120522 */
	{0x79, 0x04,}, /* ADD 20120522 */

	/* PAGE 11 START */
	{0x03, 0x11,},
	{0x10, 0x7f,},
	{0x11, 0x40,},
	{0x12, 0x0a,}, /* Blue Max-Filter Delete */
	{0x13, 0xb9,},

	{0x26, 0x68,}, /* Double_AG 31 -> 20 */
	{0x27, 0x62,}, /* Double_AG 34 -> 22 */
	{0x28, 0x0f,},
	{0x29, 0x10,},
	{0x2b, 0x30,},
	{0x2c, 0x32,},

	/* Out2 D-LPF th */
	{0x30, 0x70,},
	{0x31, 0x10,},
	{0x32, 0x58,},
	{0x33, 0x09,},
	{0x34, 0x06,},
	{0x35, 0x03,},

	/* Out1 D-LPF th */
	{0x36, 0x70,},
	{0x37, 0x18,},
	{0x38, 0x58,},
	{0x39, 0x20,},
	{0x3a, 0x1f,},
	{0x3b, 0x03,},

	/* Indoor D-LPF th */
	{0x3c, 0x80,},
	{0x3d, 0x18,},
	{0x3e, 0x80,},
	{0x3f, 0x0c,},
	{0x40, 0x09,},
	{0x41, 0x03,},

	/* Dark1 D-LPF th */
	{0x42, 0x80,},
	{0x43, 0x18,},
	{0x44, 0x80,},
	{0x45, 0x0f,},
	{0x46, 0x0c,},
	{0x47, 0x0b,},

	/* Dark2 D-LPF th */
	{0x48, 0x88,},
	{0x49, 0x2c,},
	{0x4a, 0x80,},
	{0x4b, 0x0f,},
	{0x4c, 0x0c,},
	{0x4d, 0x0b,},

	/* Dark3 D-LPF th */
	{0x4e, 0x80,},
	{0x4f, 0x23,},
	{0x50, 0x80,},
	{0x51, 0x0f,},
	{0x52, 0x0c,},
	{0x53, 0x0c,},

	{0x54, 0x11,},
	{0x55, 0x17,},
	{0x56, 0x20,},
	{0x57, 0x01,},
	{0x58, 0x00,},
	{0x59, 0x00,},

	{0x5a, 0x18,},
	{0x5b, 0x00,},
	{0x5c, 0x00,},

	{0x60, 0x3f,},
	{0x62, 0x60,},
	{0x70, 0x06,},
	/* PAGE 11 END */

	/* PAGE 12 START */
	{0x03, 0x12,},
	{0x20, 0x00,},
	{0x21, 0x00,},

	{0x25, 0x00,}, /* 0x30 */

	{0x28, 0x00,},
	{0x29, 0x00,},
	{0x2a, 0x00,},

	{0x30, 0x50,},
	{0x31, 0x18,},
	{0x32, 0x32,},
	{0x33, 0x40,},
	{0x34, 0x50,},
	{0x35, 0x70,},
	{0x36, 0xa0,},

	/* Out2 th */
	{0x40, 0xa0,},
	{0x41, 0x40,},
	{0x42, 0xa0,},
	{0x43, 0x90,},
	{0x44, 0x90,},
	{0x45, 0x80,},

	/* Out1 th */
	{0x46, 0xb0,},
	{0x47, 0x55,},
	{0x48, 0xb0,},
	{0x49, 0xb0,},
	{0x4a, 0x90,},
	{0x4b, 0x80,},

	/* Indoor th */
	{0x4c, 0xb0,},
	{0x4d, 0x40,},
	{0x4e, 0x90,},
	{0x4f, 0x90,},
	{0x50, 0x90,},
	{0x51, 0x80,},

	/* Dark1 th */
	{0x52, 0xb0,},
	{0x53, 0x50,},
	{0x54, 0xa8,},
	{0x55, 0xa8,},
	{0x56, 0xb0,},
	{0x57, 0x7b,},

	/* Dark2 th */
	{0x58, 0xa0,},
	{0x59, 0x40,},
	{0x5a, 0xb8,},
	{0x5b, 0xb8,},
	{0x5c, 0xc8,},
	{0x5d, 0x7b,},

	/* Dark3 th */
	{0x5e, 0x9c,},
	{0x5f, 0x40,},
	{0x60, 0xc0,},
	{0x61, 0xc0,},
	{0x62, 0xc8,},
	{0x63, 0x7b,},

	{0x70, 0x15,},
	{0x71, 0x01,}, /* Don't Touch register */

	{0x72, 0x18,},
	{0x73, 0x01,}, /* Don't Touch register */

	{0x74, 0x25,},
	{0x75, 0x15,},

	{0x80, 0x20,},
	{0x81, 0x40,},
	{0x82, 0x65,},
	{0x85, 0x1a,},
	{0x88, 0x00,},
	{0x89, 0x00,},
	{0x90, 0x5d,}, /* add 20120430 */

	/* Dont Touch register */
	{0xD0, 0x0c,},
	{0xD1, 0x80,},
	{0xD2, 0x17,},
	{0xD3, 0x00,},
	{0xD4, 0x00,},
	{0xd5, 0x0f,},
	{0xD6, 0xff,},
	{0xd7, 0xff,},
	/* End */

	{0x3b, 0x06,},
	{0x3c, 0x06,},

	/* Don't Touch register */
	{0xc5, 0x30,}, /* 55 -> 48 */
	{0xc6, 0x2a,}, /* 48 -> 40 */
	/* PAGE 12 END */

	/* PAGE 13 START */

	{0x03, 0x13,},
	{0x10, 0xcb,},
	{0x11, 0x7b,},
	{0x12, 0x07,},
	{0x14, 0x00,},

	{0x20, 0x15,},
	{0x21, 0x13,},
	{0x22, 0x33,},
	{0x23, 0x05,},
	{0x24, 0x09,},

	{0x25, 0x0a,},

	{0x26, 0x18,},
	{0x27, 0x30,},
	{0x29, 0x12,},
	{0x2a, 0x50,},

	/* Low clip th */
	{0x2b, 0x02,},
	{0x2c, 0x02,},
	{0x25, 0x06,},
	{0x2d, 0x0c,},
	{0x2e, 0x12,},
	{0x2f, 0x12,},

	/* Out2 Edge */
	{0x50, 0x10,},
	{0x51, 0x14,},
	{0x52, 0x12,},
	{0x53, 0x0c,},
	{0x54, 0x0f,},
	{0x55, 0x0c,},

	/* Out1 Edge */
	{0x56, 0x0f,},
	{0x57, 0x12,},
	{0x58, 0x12,},
	{0x59, 0x09,},
	{0x5a, 0x0c,},
	{0x5b, 0x0c,},

	/* Indoor Edge */
	{0x5c, 0x0a,},
	{0x5d, 0x0b,},
	{0x5e, 0x0a,},
	{0x5f, 0x08,},
	{0x60, 0x09,},
	{0x61, 0x08,},

	/* Dark1 Edge */
	{0x62, 0x09,},
	{0x63, 0x09,},
	{0x64, 0x09,},
	{0x65, 0x07,},
	{0x66, 0x07,},
	{0x67, 0x07,},

	/* Dark2 Edge */
	{0x68, 0x08,},
	{0x69, 0x08,},
	{0x6a, 0x08,},
	{0x6b, 0x06,},
	{0x6c, 0x06,},
	{0x6d, 0x06,},

	/* Dark3 Edge */
	{0x6e, 0x08,},
	{0x6f, 0x08,},
	{0x70, 0x08,},
	{0x71, 0x06,},
	{0x72, 0x06,},
	{0x73, 0x06,},

	/* 2DY */
	{0x80, 0x00,}, /* For Preview */
	{0x81, 0x1f,},
	{0x82, 0x05,},
	{0x83, 0x31,},

	{0x90, 0x05,},
	{0x91, 0x05,},
	{0x92, 0x33,},
	{0x93, 0x30,},
	{0x94, 0x03,},
	{0x95, 0x14,},
	{0x97, 0x20,},
	{0x99, 0x20,},

	{0xa0, 0x01,},
	{0xa1, 0x02,},
	{0xa2, 0x01,},
	{0xa3, 0x02,},
	{0xa4, 0x05,},
	{0xa5, 0x05,},
	{0xa6, 0x07,},
	{0xa7, 0x08,},
	{0xa8, 0x07,},
	{0xa9, 0x08,},
	{0xaa, 0x07,},
	{0xab, 0x08,},

	/* Out2 */
	{0xb0, 0x22,},
	{0xb1, 0x2a,},
	{0xb2, 0x28,},
	{0xb3, 0x22,},
	{0xb4, 0x2a,},
	{0xb5, 0x28,},

	/* Out1 */
	{0xb6, 0x22,},
	{0xb7, 0x2a,},
	{0xb8, 0x28,},
	{0xb9, 0x22,},
	{0xba, 0x2a,},
	{0xbb, 0x28,},

	/* Indoor */
	{0xbc, 0x25,},
	{0xbd, 0x2a,},
	{0xbe, 0x27,},
	{0xbf, 0x25,},
	{0xc0, 0x2a,},
	{0xc1, 0x27,},

	/* Dark1 */
	{0xc2, 0x1e,},
	{0xc3, 0x24,},
	{0xc4, 0x20,},
	{0xc5, 0x1e,},
	{0xc6, 0x24,},
	{0xc7, 0x20,},

	/*Dark2*/
	{0xc8, 0x18,},
	{0xc9, 0x20,},
	{0xca, 0x1e,},
	{0xcb, 0x18,},
	{0xcc, 0x20,},
	{0xcd, 0x1e,},

	/* Dark3 */
	{0xce, 0x18,},
	{0xcf, 0x20,},
	{0xd0, 0x1e,},
	{0xd1, 0x18,},
	{0xd2, 0x20,},
	{0xd3, 0x1e,},
	/* PAGE 13 END */

	/* PAGE 14 START */
	{0x03, 0x14,},
	{0x10, 0x11,},

	{0x14, 0x80,}, /* GX */
	{0x15, 0x80,}, /* GY */
	{0x16, 0x80,}, /* RX */
	{0x17, 0x80,}, /* RY */
	{0x18, 0x80,}, /* BX */
	{0x19, 0x80,}, /* BY */

	{0x20, 0x80,}, /* X */
	{0x21, 0x80,}, /* Y */

	{0x22, 0x80,},
	{0x23, 0x80,},
	{0x24, 0x80,},

	{0x30, 0xc8,},
	{0x31, 0x2b,},
	{0x32, 0x00,},
	{0x33, 0x00,},
	{0x34, 0x90,},

	{0x40, 0x34,}, /* 3e */
	{0x50, 0x24,}, /* 28 */
	{0x60, 0x20,}, /* 24 */
	{0x70, 0x24,}, /* 28 */
	/* PAGE 14 END */

	/* PAGE 15 START */
	{0x03, 0x15,},
	{0x10, 0x0f,},

	/* Rstep H 16 */
	/* Rstep L 14 */
	{0x14, 0x46,}, /* CMCOFSGH */
	{0x15, 0x38,}, /* CMCOFSGM */
	{0x16, 0x28,}, /* CMCOFSGL */
	{0x17, 0x2f,}, /* CMC SIGN */

	/* CMC */
	{0x30, 0x8f,},
	{0x31, 0x59,},
	{0x32, 0x0a,},
	{0x33, 0x15,},
	{0x34, 0x5b,},
	{0x35, 0x06,},
	{0x36, 0x07,},
	{0x37, 0x40,},
	{0x38, 0x87,},

	/* CMC OFS */
	{0x40, 0x94,},
	{0x41, 0x20,},
	{0x42, 0x89,},
	{0x43, 0x84,},
	{0x44, 0x03,},
	{0x45, 0x01,},
	{0x46, 0x88,},
	{0x47, 0x9c,},
	{0x48, 0x28,},

	/* CMC POFS */
	{0x50, 0x00,},
	{0x51, 0x00,},
	{0x52, 0x00,},
	{0x53, 0x84,},
	{0x54, 0x20,},
	{0x55, 0x9c,},
	{0x56, 0x00,},
	{0x57, 0x00,},
	{0x58, 0x00,},

	{0x80, 0x00,},
	{0x85, 0x80,},
	{0x87, 0x02,},
	{0x88, 0x00,},
	{0x89, 0x00,},
	{0x8a, 0x00,},
	/* PAGE 15 END */

	/* PAGE 16 START */
	{0x03, 0x16,},
	{0x10, 0x31,},
	{0x18, 0x5e,},/* Double_AG 5e->37 */
	{0x19, 0x5d,},/* Double_AG 5e->36 */
	{0x1a, 0x0e,},
	{0x1b, 0x01,},
	{0x1c, 0xdc,},
	{0x1d, 0xfe,},

	/* GMA Default */
	{0x30, 0x00,},
	{0x31, 0x0e,},
	{0x32, 0x1e,},
	{0x33, 0x34,},
	{0x34, 0x56,},
	{0x35, 0x74,},
	{0x36, 0x8b,},
	{0x37, 0x9c,},
	{0x38, 0xaa,},
	{0x39, 0xb9,},
	{0x3a, 0xc5,},
	{0x3b, 0xd2,},
	{0x3c, 0xdb,},
	{0x3d, 0xe5,},
	{0x3e, 0xeb,},
	{0x3f, 0xf3,},
	{0x40, 0xf8,},
	{0x41, 0xfd,},
	{0x42, 0xff,},

	{0x50, 0x00,},
	{0x51, 0x08,},
	{0x52, 0x1e,},
	{0x53, 0x36,},
	{0x54, 0x5a,},
	{0x55, 0x75,},
	{0x56, 0x8d,},
	{0x57, 0xa1,},
	{0x58, 0xb2,},
	{0x59, 0xbe,},
	{0x5a, 0xc9,},
	{0x5b, 0xd2,},
	{0x5c, 0xdb,},
	{0x5d, 0xe3,},
	{0x5e, 0xeb,},
	{0x5f, 0xf0,},
	{0x60, 0xf5,},
	{0x61, 0xf7,},
	{0x62, 0xf8,},

	{0x70, 0x00,},
	{0x71, 0x0e,},
	{0x72, 0x1f,},
	{0x73, 0x3f,},
	{0x74, 0x5d,},
	{0x75, 0x75,},
	{0x76, 0x8a,},
	{0x77, 0x9c,},
	{0x78, 0xad,},
	{0x79, 0xbb,},
	{0x7a, 0xc6,},
	{0x7b, 0xd1,},
	{0x7c, 0xda,},
	{0x7d, 0xe3,},
	{0x7e, 0xea,},
	{0x7f, 0xf1,},
	{0x80, 0xf6,},
	{0x81, 0xfb,},
	{0x82, 0xff,},
	/* PAGE 16 END */

	/* PAGE 17 START */
	{0x03, 0x17,},
	{0x10, 0xf7,},
	/* PAGE 17 END */

	/* 640x480 size */
	{0x03, 0x18,},
	{0x10, 0x07,},
	{0x11, 0x00,},
	{0x12, 0x58,},
	{0x20, 0x05,},
	{0x21, 0x00,},
	{0x22, 0x01,},
	{0x23, 0xe0,},
	{0x24, 0x00,}, /* X start position */
	{0x25, 0x08,},
	{0x26, 0x00,}, /* Y start position */
	{0x27, 0x02,},
	{0x28, 0x05,}, /* X End position */
	{0x29, 0x08,},
	{0x2a, 0x01,}, /* Y End position */
	{0x2b, 0xe2,},
	{0x2c, 0x0a,},
	{0x2d, 0x00,},
	{0x2e, 0x0a,},
	{0x2f, 0x00,},
	{0x30, 0x46,},
	{0x15, 0x01,},

	/* PAGE 20 START */
	{0x03, 0x20,},
	{0x11, 0x1c,},
	{0x18, 0x30,},
	{0x1a, 0x08,},
	{0x20, 0x05,},
	{0x21, 0x30,},
	{0x22, 0x10,},
	{0x23, 0x00,},
	{0x24, 0x00,},

	{0x28, 0xe7,},
	{0x29, 0x0d,}, /* 20100305 ad->0d */
	{0x2a, 0xff,},
	{0x2b, 0xf4,},

	{0x2c, 0xc2,},
	{0x2d, 0x5f,},
	{0x2e, 0x33,},
	{0x30, 0xf8,},
	{0x32, 0x03,},
	{0x33, 0x2e,},
	{0x34, 0x30,},
	{0x35, 0xd4,},
	{0x36, 0xfe,},
	{0x37, 0x32,},
	{0x38, 0x04,},
	{0x39, 0x22,},
	{0x3a, 0xde,},
	{0x3b, 0x22,},
	{0x3c, 0xde,},

	{0x50, 0x45,},
	{0x51, 0x88,},

	{0x56, 0x27,},
	{0x57, 0xa0,},
	{0x58, 0x20,},
	{0x59, 0x74,},
	{0x5a, 0x04,},

	{0x60, 0x55,},
	{0x61, 0x55,},
	{0x62, 0x6A,},
	{0x63, 0xA9,},
	{0x64, 0x6A,},
	{0x65, 0xA9,},
	{0x66, 0x6B,},
	{0x67, 0xE9,},
	{0x68, 0x6B,},
	{0x69, 0xE9,},
	{0x6a, 0x6A,},
	{0x6b, 0xA9,},
	{0x6c, 0x6A,},
	{0x6d, 0xA9,},
	{0x6e, 0x55,},
	{0x6f, 0x55,},

	{0x70, 0x70,}, /* 6c */
	{0x71, 0x80,}, /* 82(+8) */

	{0x76, 0x43,},
	{0x77, 0xE2,},
	{0x78, 0x23,}, /* 24 */
	{0x79, 0x43,}, /* Y Target 70 => 25, 72 => 26 */
	{0x7a, 0x23,}, /* 23 */
	{0x7b, 0x22,}, /* 22 */
	{0x7d, 0x23,},

	{0x83, 0x01,}, /* EXP Normal 33.33 fps */
	{0x84, 0x5f,},
	{0x85, 0x90,},

	{0x86, 0x01,}, /* EXPMin 6500.00 fps */
	{0x87, 0xf4,},

	{0x88, 0x05,}, /*EXP Max 8.33 fps */
	{0x89, 0x7e,},
	{0x8a, 0x40,},

	{0x8B, 0x75,}, /* EXP100 */
	{0x8C, 0x30,},

	{0x8D, 0x61,}, /* EXP120 */
	{0x8E, 0xa8,},

	{0x98, 0x9d,},
	{0x99, 0x45,},
	{0x9a, 0x0d,},
	{0x9b, 0xde,},

	{0x9c, 0x17,}, /*EXP Limit 500.00 fps */
	{0x9d, 0x70,},

	{0x9e, 0x01,}, /* EXP Unit */
	{0x9f, 0xf4,},

	{0xb0, 0x18,},
	{0xb1, 0x14,},
	{0xb2, 0xd0,},
	{0xb3, 0x18,},
	{0xb4, 0x1c,},
	{0xb5, 0x48,},
	{0xb6, 0x32,},
	{0xb7, 0x2b,},
	{0xb8, 0x27,},
	{0xb9, 0x25,},
	{0xba, 0x23,},
	{0xbb, 0x22,},
	{0xbc, 0x46,},
	{0xbd, 0x44,},

	{0xc0, 0x10,},
	{0xc1, 0x3c,},
	{0xc2, 0x3c,},
	{0xc3, 0x3c,},
	{0xc4, 0x08,},

	{0xc8, 0x80,},
	{0xc9, 0x80,},
	/* PAGE 20 END */

	/* PAGE 22 START */
	{0x03, 0x22,},
	{0x10, 0xfd,},
	{0x11, 0x2e,},
	{0x19, 0x01,}, /* Low On */
	{0x20, 0x30,},
	{0x21, 0x40,},
	{0x24, 0x01,},
	{0x25, 0x7e,}, /* Add 20120514 light stable */

	{0x30, 0x80,},
	{0x31, 0x80,},
	{0x38, 0x11,},
	{0x39, 0x34,},
	{0x40, 0xe4,},

	{0x41, 0x43,}, /* 33 */
	{0x42, 0x22,}, /* 22 */
	{0x43, 0xf1,}, /* f6 */
	{0x44, 0x54,}, /* 44 */
	{0x45, 0x22,}, /* 33 */
	{0x46, 0x02,},
	{0x50, 0xb2,},
	{0x51, 0x81,},
	{0x52, 0x98,},

	{0x80, 0x38,},
	{0x81, 0x20,},
	{0x82, 0x3a,}, /* 3a */

	{0x83, 0x56,},
	{0x84, 0x22,},
	{0x85, 0x55,},
	{0x86, 0x20,},

	{0x87, 0x41,},
	{0x88, 0x31,},
	{0x89, 0x39,},
	{0x8a, 0x29,},

	{0x8b, 0x3c,},
	{0x8c, 0x38,},
	{0x8d, 0x32,},
	{0x8e, 0x2c,},

	{0x8f, 0x5c,},
	{0x90, 0x5b,},
	{0x91, 0x57,},
	{0x92, 0x4f,},
	{0x93, 0x41,},
	{0x94, 0x3a,},
	{0x95, 0x32,},
	{0x96, 0x2b,},
	{0x97, 0x23,},
	{0x98, 0x20,},
	{0x99, 0x1f,},
	{0x9a, 0x1f,},

	{0x9b, 0x78,},
	{0x9c, 0x77,},
	{0x9d, 0x48,},
	{0x9e, 0x38,},
	{0x9f, 0x30,},

	{0xa0, 0xb0,},
	{0xa1, 0x44,},
	{0xa2, 0x6f,},
	{0xa3, 0xff,},

	{0xa4, 0x14,}, /* 1500fps */
	{0xa5, 0x2c,}, /* 700fps */
	{0xa6, 0xcf,},

	{0xad, 0x40,},
	{0xae, 0x4a,},

	{0xaf, 0x2f,},  /* low temp Rgain */
	{0xb0, 0x2d,},  /* low temp Rgain */

	{0xb1, 0x00,}, /* 0x20 -> 0x00 0405 modify */
	{0xb4, 0xbf,},
	{0xb8, 0x09,}, /* a2:b-2,R+2  b4:B-3,R+4 lowtemp b0 a1 Spec AWB A modify */
	{0xb9, 0x00,},
	/* PAGE 22 END */

	/* PAGE 48 START */
	{0x03, 0x48,},

	/* PLL Setting */
	{0x70, 0x05,},
	{0x71, 0x30,}, /* MiPi Pllx2 */
	{0x72, 0x85,},
	{0x70, 0xa5,}, /* PLL Enable */
	{0x03, 0x48,},
	{0x03, 0x48,},
	{0x03, 0x48,},
	{0x03, 0x48,},
	{0x70, 0x95,}, /* CLK_GEN_ENABLE */

	/* MIPI TX Setting */
	{0x11, 0x00,}, /* 20111013 0x10 continuous -> 0x00 not Continuous */
	/* {0x17, 0xcc,}, */
	{0x10, 0x1c,},
	{0x12, 0x00,},
	{0x14, 0x30,}, /*	0x1470 */ /* 20111013	0x00 -> 0x30 Clock Delay */
	{0x16, 0x04,}, /* 1016 	0x04 ->	0x05*/

	{0x19, 0x00,},
	{0x1a, 0x32,},
	{0x1b, 0x17,},
	{0x1c, 0x0c,},
	{0x1d, 0x0f,},
	{0x1e, 0x06,},
	{0x1f, 0x02,}, /* 0x05->0x03 20131101 */ /* 0x03->0x02 20140509 */
	{0x20, 0x00,},

	{0x23, 0x01,},
	{0x24, 0x1e,},
	{0x25, 0x00,},
	{0x26, 0x00,},
	{0x27, 0x01,},
	{0x28, 0x00,},
	{0x2a, 0x06,},
	{0x2b, 0x40,},
	{0x2c, 0x04,},
	{0x2d, 0xb0,},

	{0x30, 0x00,}, /* 640x480 MiPi OutPut */
	{0x31, 0x05,},

	{0x32, 0x0c,},
	{0x33, 0x0a,},
	{0x34, 0x02,}, /* CLK LP -> HS Prepare time 24MHz:0x02, 48MHz:0x03 */
	{0x35, 0x03,},
	{0x36, 0x01,},
	{0x37, 0x07,},
	{0x38, 0x02,},
	{0x39, 0x02,}, /* drivability 24MHZ: 0x02, 48MHz:0x03 */
	/* {0x17, 0xc4,}, */ /* MHSHIM */
	/* {0x17, 0xc0,}, */ /* MHSHIM */
	/* {0x17, 0x00,}, */ /* MHSHIM */
	{0x50, 0x00,},
	/* PAGE 48 END */

	/* PAGE 20 */
	{0x03, 0x20,},
	{0x10, 0x9c,}, /* AE on 50hz */

	/* PAGE 22 */
	{0x03, 0x22,},
	{0x10, 0xe9,},

	/* PAGE 0 */
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},

	{0x03, 0x00,},

	//{0x01, 0x30,},

	{SENSOR_WRITE_DELAY, 0x0a,}, /* NEED Delay 100ms */
};

LOCAL const SENSOR_REG_T HI255_640X480[]=
{
	{0x03, 0x00,}, /* Sleep On */
	{0x01, 0x31,},

	{0x03, 0x20,}, /* page 20 */
	{0x18, 0x30,}, /* for Preview */
	{0x10, 0x1c,}, /* AE off 50hz */

	{0x03, 0x22,}, /* page 22 */
	{0x10, 0x69,}, /* awb off */

	{0x03, 0x00,}, /* page 0 */
	{0x10, 0x11,},

	{0x20, 0x00,}, /* windowing, row start, high byte */
	{0x21, 0x02,}, /* windowing, row start, low byte */ /* modify 20110929 0x04 -> 0x02 */
	{0x22, 0x00,}, /* windowing, column start, high byte */
	{0x23, 0x0a,}, /* windowing, column start, low byte */ /* modify 20110929 0x14 -> 0x0a */

	/* Page 10 */
	{0x03, 0x10,},
	{0x3f, 0x00,},
	{0x60, 0x63,},

	/* Page12 */
	{0x03, 0x12,},
	{0x20, 0x0f,}, /* 0f */
	{0x21, 0x0f,}, /* 0f */
	{0x90, 0x5d,}, /* 5f */

	/* only for Preview DPC */
	{0xd2, 0x67,},
	{0xd5, 0x02,},
	{0xd7, 0x18,},

	/* Page13 */
	{0x03, 0x13,},
	{0x10, 0x4a,}, /* Edge Off */
	{0x80, 0x00,},

	/* Page18 */
	{0x03, 0x18,},
	{0x10, 0x07,},

	/* PAGE 48 TART */
	{0x03, 0x48,},

	/* PLL Setting */
	{0x70, 0x05,},
	{0x71, 0x30,}, /* MiPi Pllx2, orig 0x30 => 16.5, 0x60 => 32fps */
	{0x72, 0x85,},
	{0x70, 0xa5,}, /* PLL Enable */
	{0x03, 0x48,},
	{0x03, 0x48,},
	{0x03, 0x48,},
	{0x03, 0x48,},
	{0x70, 0x95,}, /* CLK_GEN_ENABLE */

	/* MIPI TX Setting */
	{0x11, 0x00,}, /* 20111013 0x10 : continuous -> 0x00 : not Continuous */
	{0x10, 0x1c,},
	{0x12, 0x00,},
	{0x14, 0x30,}, /* 20111013 0x00 -> 0x30 Clock Delay */
	{0x16, 0x04,},

	{0x19, 0x00,},
	{0x1a, 0x32,},
	{0x1b, 0x17,},
	{0x1c, 0x0c,},
	{0x1d, 0x0f,},
	{0x1e, 0x06,},
	{0x1f, 0x02,}, /* 0x05->0x03 20131101 */ /* 0x03->0x02 20140509 */
	{0x20, 0x00,},

	{0x23, 0x01,},
	{0x24, 0x1e,},
	{0x25, 0x00,},
	{0x26, 0x00,},
	{0x27, 0x01,},
	{0x28, 0x00,},
	{0x2a, 0x06,},
	{0x2b, 0x40,},
	{0x2c, 0x04,},
	{0x2d, 0xb0,},

	{0x30, 0x00,}, /* 640 x 480 MiPi OutPut */
	{0x31, 0x05,},

	{0x32, 0x0c,},
	{0x33, 0x0a,},
	{0x34, 0x02,}, /* CLK LP -> HS Prepare time 24MHz:0x02, 48MHz:0x03 */
	{0x35, 0x01,},
	{0x36, 0x03,},
	{0x37, 0x07,},
	{0x38, 0x02,},
	{0x39, 0x02,},/* drivability 24MHZ:02, 48MHz:03 */

	{0x50, 0x00,},
	/* PAGE 48 END */

	{0x03, 0x20,},
	{0x10, 0x9c,}, /* AE on 50hz */

	{0x03, 0x22,},
	{0x10, 0xe9,}, /* AWB ON */

	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},

	{0x03, 0x00,}, /* Sleep Off */
	//{0x01, 0x30,},

	{SENSOR_WRITE_DELAY, 0x28,}, /* 400ms */
};


LOCAL const SENSOR_REG_T HI255_800X600[]=
{
	{0x03, 0x00,}, /* Sleep On */
	{0x01, 0x31,},

	{0x03, 0x20,}, /* page 20 */
	{0x18, 0x30,}, /* for Preview */
	{0x10, 0x1c,}, /* AE off 50hz */

	{0x03, 0x22,}, /* page 22 */
	{0x10, 0x69,}, /* awb off */

	{0x03, 0x00,}, /* page 0 */
	{0x10, 0x11,},

	{0x20, 0x00,}, /* windowing, row start, high byte */
	{0x21, 0x02,}, /* windowing, row start, low byte */ /* modify 20110929 0x04 -> 0x02 */
	{0x22, 0x00,}, /* windowing, column start, high byte */
	{0x23, 0x0a,}, /* windowing, column start, low byte */ /* modify 20110929 0x14 -> 0x0a */

	/* Page 10 */
	{0x03, 0x10,},
	{0x3f, 0x00,},
	{0x60, 0x63,},

	/* Page12 */
	{0x03, 0x12,},
	{0x20, 0x0f,}, /* 0f */
	{0x21, 0x0f,}, /* 0f */
	{0x90, 0x5d,}, /* 5f */

	/* only for Preview DPC */
	{0xd2, 0x67,},
	{0xd5, 0x02,},
	{0xd7, 0x18,},

	/* Page13 */
	{0x03, 0x13,},
	{0x10, 0x4a,}, /* Edge Off */
	{0x80, 0x00,},

	/* Page18 */
	{0x03, 0x18,},
	{0x10, 0x00,},

	/* PAGE 48 TART */
	{0x03, 0x48,},

	/* PLL Setting */
	{0x70, 0x05,},
	{0x71, 0x30,}, /* MiPi Pllx2, orig 0x30 => 16.5, 0x60 => 32fps */
	{0x72, 0x85,},
	{0x70, 0xa5,}, /* PLL Enable */
	{0x03, 0x48,},
	{0x03, 0x48,},
	{0x03, 0x48,},
	{0x03, 0x48,},
	{0x70, 0x95,}, /* CLK_GEN_ENABLE */

	/* MIPI TX Setting */
	{0x11, 0x00,}, /* 20111013 0x10 : continuous -> 0x00 : not Continuous */
	{0x10, 0x1c,},
	{0x12, 0x00,},
	{0x14, 0x30,}, /* 20111013 0x00 -> 0x30 Clock Delay */
	{0x16, 0x04,},

	{0x19, 0x00,},
	{0x1a, 0x32,},
	{0x1b, 0x17,},
	{0x1c, 0x0c,},
	{0x1d, 0x0f,},
	{0x1e, 0x06,},
	{0x1f, 0x02,}, /* 0x05->0x03 20131101 */ /* 0x03->0x02 20140509 */
	{0x20, 0x00,},

	{0x23, 0x01,},
	{0x24, 0x1e,},
	{0x25, 0x00,},
	{0x26, 0x00,},
	{0x27, 0x01,},
	{0x28, 0x00,},
	{0x2a, 0x06,},
	{0x2b, 0x40,},
	{0x2c, 0x04,},
	{0x2d, 0xb0,},

	{0x30, 0x40,}, /* 800x600 MiPi OutPut */
	{0x31, 0x06,},

	/* {0x30, 0x00,}, */ /* 640 x 480 MiPi OutPut */
	/* {0x31, 0x05,}, */

	{0x32, 0x0c,},
	{0x33, 0x0a,},
	{0x34, 0x02,}, /* CLK LP -> HS Prepare time 24MHz:0x02, 48MHz:0x03 */
	{0x35, 0x01,},
	{0x36, 0x03,},
	{0x37, 0x07,},
	{0x38, 0x02,},
	{0x39, 0x02,},/* drivability 24MHZ:02, 48MHz:03 */

	{0x50, 0x00,},
	/* PAGE 48 END */

	{0x03, 0x20,},
	{0x10, 0x9c,}, /* AE on 50hz */

	{0x03, 0x22,},
	{0x10, 0xe9,}, /* AWB ON */

	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},

	{0x03, 0x00,}, /* Sleep Off */
	//{0x01, 0x30,},

	{SENSOR_WRITE_DELAY, 0x28,}, /* 400ms */
};

LOCAL const SENSOR_REG_T HI255_1600X1200[] =
{

	{0x03, 0x00,},
	{0x01, 0x31,}, /* b[0] set power sleep by preserving all register values, 0=OFF 1=ON */

	{0x03, 0x22,}, /* Page 22 */
	{0x10, 0x69,}, /* AWB Off */

	{0x03, 0x00,},
	{0x10, 0x00,}, /* Imge size, windowing, Hsync, Vsync */

	{0x20, 0x00,}, /* windowing */
	{0x21, 0x0a,}, /* modify 20110929 0x0c -> 0x0a */
	{0x22, 0x00,},
	{0x23, 0x0a,}, /* modify 20110929 0x14 -> 0x0a */

	/* Page10 */
	{0x03, 0x10,},
	{0x3f, 0x00,}, /* not defined in data sheet */
	{0x60, 0x63,}, /* color saturation */

	/* Page12 */  /* Noise reduction */
	{0x03, 0x12,},
	{0x20, 0x0f,},
	{0x21, 0x0f,},
	{0x90, 0x5d,},

	/* only for Preview DPC Off */
	{0xd2, 0x67,},
	{0xd5, 0x02,},
	{0xd7, 0x18,},

	/* Page13 */  /* Edge enhancement */
	{0x03, 0x13,},
	{0x10, 0xcb,}, /* Edge On */
	{0x80, 0xfd,},

	/* PAGE 18 */ /* Image scaling */
	{0x03, 0x18,},
	{0x10, 0x00,}, /* Scaling Off */

	/* PAGE 48 START */
	{0x03, 0x48,},

	/* PLL Setting */
	{0x70, 0x05,},
	{0x71, 0x30,}, /* MiPi Pllx2 */
	{0x72, 0x81,},
	{0x70, 0x85,}, /* PLL Enable */
	{0x03, 0x48,},
	{0x03, 0x48,},
	{0x03, 0x48,},
	{0x03, 0x48,},
	{0x70, 0x95,}, /* CLK_GEN_ENABLE */

	/* MIPI TX Setting */
	{0x11, 0x00,}, /* 20111013 0x10 continuous -> 0x00 not Continuous */
	{0x10, 0x1c,},
	{0x12, 0x00,},
	{0x14, 0x30,}, /* 20111013 0x00 -> 0x30 Clock Delay */
	{0x16, 0x04,},

	{0x19, 0x00,},
	{0x1a, 0x32,},
	{0x1b, 0x17,},
	{0x1c, 0x0b,},
	{0x1d, 0x0e,},
	{0x1e, 0x08,},
	{0x1f, 0x03,},
	{0x20, 0x00,},

	{0x23, 0x01,},
	{0x24, 0x1e,},
	{0x25, 0x00,},
	{0x26, 0x00,},
	{0x27, 0x01,},
	{0x28, 0x00,},
	{0x2a, 0x06,},
	{0x2b, 0x40,},
	{0x2c, 0x04,},
	{0x2d, 0xb0,},

	{0x30, 0x80,}, /* 1600 x 1200 MiPi OutPut */
	{0x31, 0x0c,},

	/* {0x30, 0x40,}, */ /* 800x600 MiPi OutPut */
	/* {0x31, 0x06,}, */

	{0x32, 0x0c,},
	{0x33, 0x0a,},
	{0x34, 0x03,}, /* CLK LP -> HS Prepare time 24MHz:0x02, 48MHz:0x03 */
	{0x35, 0x03,},
	{0x36, 0x01,},
	{0x37, 0x07,},
	{0x38, 0x02,},
	{0x39, 0x03,}, /* drivability 24MHZ:02, 48MHz:03 */

	{0x50, 0x00,},
	/* PAGE 48 END */

	/* Page0 */
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,}, /* Dummy 750us */
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},
	{0x03, 0x00,},

	{0x03, 0x00,},
	//{0x01, 0x30,}, /* Sleep Off */

	{0xffff, 0x30,},  //Increase from 30ms


};

LOCAL SENSOR_REG_TAB_INFO_T s_HI255_resolution_Tab_YUV[]=
{
	// COMMON INIT
	{ADDR_AND_LEN_OF_ARRAY(HI255_common),   0, 0, 24, SENSOR_IMAGE_FORMAT_YUV422},
	//{PNULL, 0, 0, 24, SENSOR_IMAGE_FORMAT_YUV422},
	{ADDR_AND_LEN_OF_ARRAY(HI255_640X480),   640,  480,   24, SENSOR_IMAGE_FORMAT_YUV422},
	{ADDR_AND_LEN_OF_ARRAY(HI255_1600X1200), 1600, 1200,  24, SENSOR_IMAGE_FORMAT_YUV422},
	//{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},

	// YUV422 PREVIEW 2
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

static SENSOR_TRIM_T s_HI255_Resolution_Trim_Tab[] = {
	{0, 0, 640, 480, 0, 0,0, {0, 0, 640, 480}},
	{0, 0, 640, 480, 68, 500, 0x2bc, {0, 0, 640, 480}},
	//{0, 0, 1280, 960, 68, 900, 0x03b8, {0, 0, 1280, 960}},
	{0, 0, 1600, 1200, 68, 500, 0x03b8, {0, 0, 1600, 1200}},
	//{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},

	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}}
};
LOCAL unsigned long HI255_InitExt(unsigned long param)	//wujinyou, 2012.11.14
{
	SENSOR_REG_TAB_INFO_T * sensor_reg_tab_info_ptr;
	uint32_t i;
	SENSOR_IOCTL_FUNC_PTR write_reg_func;
	uint16_t subaddr;
	uint16_t data;
	int32_t ret = -1;
	nsecs_t 			timestamp_old;
	nsecs_t				timestamp_new;

	SENSOR_PRINT("HI255_InitExt start ;%ld;\n",param);

	setmode = param;

	if ( sensor_InitTflash((uint32_t)param) == SENSOR_SUCCESS)
		return SENSOR_SUCCESS;

	if (param != SENSOR_MODE_COMMON_INIT)
		return SENSOR_FAIL;

	timestamp_old = systemTime(CLOCK_MONOTONIC);

	sensor_reg_tab_info_ptr = &s_HI255_resolution_Tab_YUV[param];


	{
		SENSOR_REG_TAB_T regTab;
		regTab.reg_count			= sensor_reg_tab_info_ptr->reg_count;
		regTab.reg_bits				= SENSOR_I2C_REG_8BIT | SENSOR_I2C_VAL_8BIT;	//s_sensor_info_ptr->reg_addr_value_bits;
		regTab.burst_mode			= 7;
		regTab.sensor_reg_tab_ptr 	= sensor_reg_tab_info_ptr->sensor_reg_tab_ptr;

		ret = Sensor_Device_WriteRegTab(&regTab);
	}

	timestamp_new = systemTime(CLOCK_MONOTONIC);
	SENSOR_PRINT("SENSOR: HI255_InitExt end, ret=%d, time=%d us\n", ret, (uint32_t)((timestamp_new-timestamp_old)/1000));

	return SENSOR_SUCCESS;
};

LOCAL SENSOR_IOCTL_FUNC_TAB_T s_HI255_ioctl_func_tab =
{
	// Internal
	PNULL,
	PNULL,//_HI255_PowerOn,
	PNULL,
	_HI255_Identify,

	PNULL,            // write register
	PNULL,            // read  register
	PNULL,     //HI255_InitExt,
	_HI255_GetResolutionTrimTab,
	//PNULL,

	// External
	PNULL,
	PNULL,
	PNULL,

	_HI255_set_brightness,
	PNULL,//_HI255_set_contrast,
	PNULL,
	_HI255_set_saturation,

	_HI255_set_work_mode,
	_HI255_set_image_effect,

	_HI255_BeforeSnapshot,
	_HI255_after_snapshot,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,

	PNULL,
	PNULL,
	PNULL,
	PNULL,
	_HI255_set_awb,
	PNULL,
	PNULL,//_HI255_set_iso,
	PNULL,//_HI255_set_ev,
	_HI255_check_image_format_support,
	PNULL,
	PNULL,
	_HI255_GetExifInfo,
	PNULL,//_HI255_ExtFunc,
	_HI255_set_anti_flicker,
	_HI255_set_video_mode,
	_HI255_pick_out_jpeg_stream,
	PNULL, //meter_mode
	PNULL, //get_status
	_hi255_StreamOn,
	_hi255_StreamOff,
	PNULL
};

/**---------------------------------------------------------------------------*
 **                         Global Variables                                  *
 **---------------------------------------------------------------------------*/
//LOCAL SENSOR_EXTEND_INFO_T g_HI255_ext_info = {
//    (SENSOR_FOCUS_TRIG| SENSOR_FOCUS_ZONE),
//    (SENSOR_EXPOSURE_AUTO|  SENSOR_EXPOSURE_ZONE)
//};

//PUBLIC
SENSOR_INFO_T g_hi255_yuv_info =
{
	HI255_I2C_ADDR_W,                // salve i2c write address
	HI255_I2C_ADDR_R,                 // salve i2c read address
	SENSOR_I2C_FREQ_400,
	                                // bit1: 0: i2c register addr  is 8 bit, 1: i2c register addr  is 16 bit
	                                // other bit: reseved
	SENSOR_HW_SIGNAL_PCLK_P|\
	SENSOR_HW_SIGNAL_VSYNC_N|\
	SENSOR_HW_SIGNAL_HSYNC_P,        // bit0: 0:negative; 1:positive -> polarily of pixel clock
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

	0x7,                                // bit[0:7]: count of step in brightness, contrast, sharpness, saturation
	                                // bit[8:31] reseved

	SENSOR_LOW_PULSE_RESET,            // reset pulse level
	10,                                // reset pulse width(ms)

	SENSOR_LOW_LEVEL_PWDN,            // 1: high level valid; 0: low level valid

	1,                                // count of identify code
	{{0x04, 0xb4},						// supply two code to identify sensor.
	{0x04, 0xb4}},						// for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,                // voltage of avdd

	1600,                            // max width of source image
	1200,                            // max height of source image
	"HI255",                        // name of sensor

	SENSOR_IMAGE_FORMAT_MAX,        // define in SENSOR_IMAGE_FORMAT_E enum,SENSOR_IMAGE_FORMAT_MAX
	                                // if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T

	SENSOR_IMAGE_PATTERN_YUV422_UYVY,  //UYVY  // pattern of input image form sensor;

	s_HI255_resolution_Tab_YUV,    // point to resolution table information structure
	&s_HI255_ioctl_func_tab,        // point to ioctl function table
	PNULL,                            // information and table about Rawrgb sensor
	PNULL,//&g_HI255_ext_info,                // extend information about sensor
	//5,
	SENSOR_AVDD_1800MV,                     // iovdd
	SENSOR_AVDD_1800MV,                      // dvdd
	2,                     // skip frame num before preview
	0,                      // skip frame num before capture
	0,                      // deci frame num during preview
	0,                      // deci frame num during video preview

	0,
	0,
	0,
	0,
	0,
	//{SENSOR_INTERFACE_TYPE_CCIR601, 8, 16, 1},
	//{SENSOR_INTERFACE_TYPE_CSI2, 1, 10, 0},
	{SENSOR_INTERFACE_TYPE_CSI2, 1, 8, 1},
	PNULL,
	0,			// skip frame num while change setting
	48,			// horizontal view angle
	48,			// vertical view angle
};


static unsigned long _HI255_GetResolutionTrimTab(unsigned long param)
{
	CMR_LOGI("_HI255_GetResolutionTrimTab\n");
	return (unsigned long) s_HI255_Resolution_Trim_Tab;
}

LOCAL uint32_t _HI255_SetExifInfo_exposure(uint32_t param)
{
	EXIF_SPEC_PIC_TAKING_COND_T *sensor_exif_info_ptr = &s_HI255_exif;

       sensor_exif_info_ptr->valid.ExposureTime = 1;
	sensor_exif_info_ptr->ExposureTime.numerator =
	    0x01;
	sensor_exif_info_ptr->ExposureTime.denominator =
	    48000000 /2/6/ param ;

       return 0;

}

LOCAL uint32_t _HI255_SetExifInfo_ISO(uint32_t param)
{
	EXIF_SPEC_PIC_TAKING_COND_T *sensor_exif_info_ptr = &s_HI255_exif;
	uint8_t iso       = 0;

    	Sensor_WriteReg(0x03, 0x20);
	iso=Sensor_ReadReg(0xb0);
    	Sensor_WriteReg(0x03, 0x00);
	SENSOR_PRINT("iso=%x;",iso);

	sensor_exif_info_ptr->valid.ISOSpeedRatings = 1;
	sensor_exif_info_ptr->ISOSpeedRatings.count = 0x02;
	if (iso <= 0x1b) {
		sensor_exif_info_ptr->ISOSpeedRatings.ptr[0]= 100;
		sensor_exif_info_ptr->ISOSpeedRatings.ptr[1]= 0;
       }
	else if (iso <= 0x35) {
		sensor_exif_info_ptr->ISOSpeedRatings.ptr[0]= 200;
		sensor_exif_info_ptr->ISOSpeedRatings.ptr[1]= 0;
       }
	else if (iso <= 0x65) {
		sensor_exif_info_ptr->ISOSpeedRatings.ptr[0]= 400%255;
		sensor_exif_info_ptr->ISOSpeedRatings.ptr[1]= 400/255;
       }
	else if (iso <= 0x95) {
		sensor_exif_info_ptr->ISOSpeedRatings.ptr[0]= 800%255;
		sensor_exif_info_ptr->ISOSpeedRatings.ptr[1]= 800/255;
       }
	else {
		sensor_exif_info_ptr->ISOSpeedRatings.ptr[0]= 1600%255;
		sensor_exif_info_ptr->ISOSpeedRatings.ptr[1]= 1600/255;
       }

        return 0;
}

/******************************************************************************/
// Description:
// Global resource dependence:
// Author: Tim.zhu
// Note:
/******************************************************************************/
LOCAL unsigned long _HI255_GetExifInfo(unsigned long param)
{
	SENSOR_PRINT("Start");
	return (unsigned long)&s_HI255_exif;
}

/******************************************************************************/
// Description: get ov7670 rssolution trim tab
// Global resource dependence:
// Author: Tim.zhu
// Note:
/******************************************************************************/
LOCAL uint32_t _HI255_InitExifInfo(void)
{
	EXIF_SPEC_PIC_TAKING_COND_T *exif_ptr = &s_HI255_exif;

	memset(&s_HI255_exif , 0, sizeof(EXIF_SPEC_PIC_TAKING_COND_T));

	SENSOR_PRINT("Start");

	exif_ptr->valid.FNumber = 1;
	exif_ptr->FNumber.numerator = 14;
	exif_ptr->FNumber.denominator = 5;

	exif_ptr->valid.ExposureProgram = 1;
	exif_ptr->ExposureProgram = 0x04;

	//exif_ptr->SpectralSensitivity[MAX_ASCII_STR_SIZE];
	//exif_ptr->ISOSpeedRatings;
	//exif_ptr->OECF;

	//exif_ptr->ShutterSpeedValue;

	exif_ptr->valid.ApertureValue = 1;
	exif_ptr->ApertureValue.numerator = 14;
	exif_ptr->ApertureValue.denominator = 5;

	//exif_ptr->BrightnessValue;
	//exif_ptr->ExposureBiasValue;

	exif_ptr->valid.MaxApertureValue = 1;
	exif_ptr->MaxApertureValue.numerator = 14;
	exif_ptr->MaxApertureValue.denominator = 5;

	//exif_ptr->SubjectDistance;
	//exif_ptr->MeteringMode;
	//exif_ptr->LightSource;
	//exif_ptr->Flash;

	exif_ptr->valid.FocalLength = 1;
	exif_ptr->FocalLength.numerator = 289;
	exif_ptr->FocalLength.denominator = 100;

	//exif_ptr->SubjectArea;
	//exif_ptr->FlashEnergy;
	//exif_ptr->SpatialFrequencyResponse;
	//exif_ptr->FocalPlaneXResolution;
	//exif_ptr->FocalPlaneYResolution;
	//exif_ptr->FocalPlaneResolutionUnit;
	//exif_ptr->SubjectLocation[2];
	//exif_ptr->ExposureIndex;
	//exif_ptr->SensingMethod;

	exif_ptr->valid.FileSource = 1;
	exif_ptr->FileSource = 0x03;

	//exif_ptr->SceneType;
	//exif_ptr->CFAPattern;
	//exif_ptr->CustomRendered;

	exif_ptr->valid.ExposureMode = 1;
	exif_ptr->ExposureMode = 0x00;

	exif_ptr->valid.WhiteBalance = 1;
	exif_ptr->WhiteBalance = 0x00;

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

/******************************************************************************/
// Description: get ov7670 rssolution trim tab
// Global resource dependence:
// Author: Tim.zhu
// Note:
/******************************************************************************/
/*LOCAL uint32_t _HI255_GetResolutionTrimTab(uint32_t param)
{
    return (uint32_t)s_HI255_Resolution_Trim_Tab;
}
*/
/******************************************************************************/
// Description: sensor HI255 power on/down sequence
// Global resource dependence:
// Author: Tim.zhu
// Note:
/******************************************************************************/
LOCAL unsigned long _HI255_PowerOn(unsigned long power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_hi255_yuv_info.dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_hi255_yuv_info.avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_hi255_yuv_info.iovdd_val;
	BOOLEAN power_down = g_hi255_yuv_info.power_down_level;
	BOOLEAN reset_level = g_hi255_yuv_info.reset_pulse_level;

	CMR_LOGI("SENSOR: _HI255_PowerOn (1:on, 0:off): %d \n", power_on);

	if (SENSOR_TRUE == power_on) {
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_SetResetLevel(reset_level);
		Sensor_PowerDown(power_down);
		// Open power
		Sensor_SetIovddVoltage(iovdd_val);
		SENSOR_Sleep(5);
		Sensor_SetAvddVoltage(avdd_val);
		SENSOR_Sleep(10);
		Sensor_PowerDown(!power_down);
		SENSOR_Sleep(10);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		SENSOR_Sleep(30);
		// Reset sensor
		//Sensor_Reset(reset_level);
		Sensor_SetResetLevel(!reset_level);
		SENSOR_Sleep(10);
	} else {
		Sensor_SetResetLevel(reset_level);
		SENSOR_Sleep(1);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
		Sensor_PowerDown(power_down);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED,
				SENSOR_AVDD_CLOSED);
	}
	return SENSOR_SUCCESS;
}

/******************************************************************************/
// Description:
// Global resource dependence:
// Author: Tim.zhu
// Note:
//
/******************************************************************************/
LOCAL unsigned long _HI255_Identify(unsigned long param)
{
	uint8_t reg[2]    = {0x04, 0x04};
	//uint8_t value[2]  = {0x92, 0x92};
	uint8_t value[2]  = {0xb4, 0xb4};//test guangyao.ou
	uint8_t ret       = 0;
	uint32_t i;
	uint8_t   err_cnt = 0;
	uint32_t nLoop = 1000;

	CMR_LOGI("anrry:enter 255_Identify\n");

	for(i = 0; i<2; )
	{
		nLoop = 1000;
		CMR_LOGI("anrry:for before hi255\n");

		ret = Sensor_ReadReg(reg[i]);

		CMR_LOGI("anrry:for after hi255\n");
		CMR_LOGI("anrry: hi255 Read reg0x04 = %x\n",ret);

		if( ret != value[i])
		{
			err_cnt++;
			if(err_cnt>3)
			{
				CMR_LOGI( "255 Fail\n" );
				return SENSOR_FAIL;
			}
			else
			{
				usleep(10000);
				while(nLoop--);
				continue;
			}
		}
		err_cnt = 0;
		i++;
	}
	CMR_LOGI("255: it is HI255\n");

       _HI255_InitExifInfo();

	return (uint32_t)SENSOR_SUCCESS;
}

LOCAL unsigned long _hi255_StreamOn(unsigned long param)
{
	SENSOR_PRINT("SENSOR: _hi255_StreamOn");

	Sensor_WriteReg(0x03, 0x00);
	Sensor_WriteReg(0x01, 0x30);

	return 0;
}

LOCAL unsigned long _hi255_StreamOff(unsigned long param)
{
	SENSOR_PRINT("SENSOR: _hi255_StreamOff");

	Sensor_WriteReg(0x03, 0x00);
	Sensor_WriteReg(0x01, 0x31);

	return 0;
}
/******************************************************************************/
// Description: set brightness
// Global resource dependence:
// Author: Tim.zhu
// Note:
//
/******************************************************************************/

LOCAL const SENSOR_REG_T HI255_brightness_tab[][3]=
{
	{
		{0x03, 0x10},
		{0x40, 0xc0},
		{0xff, 0xff},
	},
	{
		{0x03, 0x10},
		{0x40, 0xb0},
		{0xff,0xff}
	},
	{
		{0x03, 0x10},
		{0x40, 0xa0},
		{0xff,0xff}
	},
	{
		{0x03, 0x10},
		{0x40, 0x00},
		{0xff,0xff}
	},
	{
		{0x03, 0x10},
		{0x40, 0x20},
		{0xff,0xff}
	},
	{
		{0x03, 0x10},
		{0x40, 0x30},
		{0xff,0xff}
	},
	{
		{0x03, 0x10},
		{0x40, 0x40},
		{0xff,0xff}
	},
};
LOCAL unsigned long _HI255_set_brightness(unsigned long level)
{
	uint16_t i=0x00;
	if(level>6)
	return 0;

	SENSOR_REG_T_PTR sensor_reg_ptr=(SENSOR_REG_T_PTR)HI255_brightness_tab[level];

	//    SCI_ASSERT(PNULL!=sensor_reg_ptr);

	for(i=0x00; (0xff!=sensor_reg_ptr[i].reg_addr)||(0xff != sensor_reg_ptr[i].reg_value); i++)
	{
		Sensor_WriteReg_8bits(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	SENSOR_PRINT("sensor: terry HI255_set_brightness = 0x%02x.\n", level);
	return 0;
}

/******************************************************************************/
// Description:
// Global resource dependence:
// Author: Tim.zhu
// Note:
//
/******************************************************************************/
LOCAL const SENSOR_REG_T HI255_saturation_tab[][4]=
{
	{//-3
		{0x03, 0x10},
		{0x61, 0x50},
		{0x62, 0x50},
		{0xff, 0xff},
	},
	{//-2
		{0x03, 0x10},
		{0x61, 0x60},
		{0x62, 0x60},
		{0xff, 0xff},
	},
	{//-1
		{0x03, 0x10},
		{0x61, 0x70},
		{0x62, 0x70},
		{0xff, 0xff},
	},
	{//00
		{0x03, 0x10},
		{0x61, 0x80},
		{0x62, 0x80},
		{0xff, 0xff},
	},
	{//+1
		{0x03, 0x10},
		{0x61, 0x90},
		{0x62, 0x90},
		{0xff, 0xff},
	},
	{//+2
		{0x03, 0x10},
		{0x61, 0xa0},
		{0x62, 0xa0},
		{0xff, 0xff},
	},
	{//+3
		{0x03, 0x10},
		{0x61, 0xb0},
		{0x62, 0xb0},
		{0xff, 0xff},
	}
};
LOCAL unsigned long _HI255_set_saturation(unsigned long level)
{
	uint16_t i=0x00;

	if(level>7)
	return 0;

	SENSOR_REG_T_PTR sensor_reg_ptr=(SENSOR_REG_T_PTR)HI255_saturation_tab[level];

	//    SCI_ASSERT(PNULL!=sensor_reg_ptr);

	for(i=0x00; (0xff!=sensor_reg_ptr[i].reg_addr)||(0xff != sensor_reg_ptr[i].reg_value); i++)
	{
		Sensor_WriteReg_8bits(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}
	SENSOR_PRINT("sensor: terry HI255_set_saturation = 0x%02x.\n", level);
	return 0;
}

/******************************************************************************/
// Description:
// Global resource dependence:
// Author: Tim.zhu
// Note:
//
/******************************************************************************/
LOCAL const SENSOR_REG_T HI255_image_effect_tab[][7]=
{
	// effect normal
	{
		{0x03,0x10},
		{0x11,0x03},
		{0x12,0xf0},
		{0x13,0x08},
		{0x44,0x80},
		{0x45,0x80},
		{0xff,0xff},
	},
	// effect mono
	{
		{0x03,0x10},
		{0x11,0x03},
		{0x12,0x33},
		{0x13,0x08},
		{0x44,0x80},
		{0x45,0x80},
		{0xff,0xff},
	},
	// effect RED
	{
		{0xff,0xff},
	},
	// effect GREEN
	{
		{0xff,0xff},
	},
	// effect  BLUE
	{
		{0xff,0xff},
	},
	// effect  YELLOW
	{
		{0xff,0xff},
	},
	// effect NEGATIVE
	{
		{0x03,0x10},	// colorinv
		{0x11,0x03},//embossing effect off
		{0x12,0x38},//auto bright on/nagative effect on
		{0x13,0x02},//binary effect off
		{0x44,0x80},//ucon
		{0x45,0x80},//vcon
		{0xff,0xff},
	},
	//effect sepia
	{
		{0x03,0x10},//sepia
		{0x11,0x03},//embossing effect off
		{0x12,0x33},//auto bright on
		{0x13,0x22},//binary effect off/solarization effect on
		{0x44,0x40},//ucon	40
		{0x45,0xa8},//vcon
		{0xff,0xff},
	},
};
LOCAL unsigned long _HI255_set_image_effect(unsigned long effect_type)
{
	uint16_t i=0x00;
    if(effect_type > 7)
    	return 0;
	SENSOR_REG_T_PTR sensor_reg_ptr=(SENSOR_REG_T_PTR)HI255_image_effect_tab[effect_type];

	//  SCI_ASSERT(PNULL!=sensor_reg_ptr);

	for(i=0x00; (0xff!=sensor_reg_ptr[i].reg_addr)||(0xff != sensor_reg_ptr[i].reg_value); i++)
	{
		Sensor_WriteReg_8bits(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	SENSOR_PRINT("sensor: terry HI255_set_image_effect = 0x%02x.\n", effect_type);
	return 0;
}

/******************************************************************************/
// Description: anti 50/60 hz banding flicker
// Global resource dependence:
// Author: Tim.zhu
// Note:
//
/******************************************************************************/
LOCAL const SENSOR_REG_T HI255_anti_banding_flicker_tab[][3]=
{

    {//50hz
	{0x03, 0x20},
	{0x10, 0x9c},
  	  {0xff, 0xff},
    },
    {//60hz

	{0x03, 0x20},
	{0x10, 0x8c},
	{0xff, 0xff},
    }
};

LOCAL unsigned long _HI255_set_anti_flicker(unsigned long mode)
{
	SENSOR_REG_T_PTR sensor_reg_ptr=(SENSOR_REG_T_PTR)HI255_anti_banding_flicker_tab[mode];
	uint16_t i=0x00;

         if(mode>1)
		 return 0;

	for(i=0x00; (0xff!=sensor_reg_ptr[i].reg_addr)||(0xff!=sensor_reg_ptr[i].reg_value); i++)
	{
		Sensor_WriteReg_8bits(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	SENSOR_PRINT("sensor: terry _HI255_set_anti_flicker = 0x%02x", mode);

	return 0;
}

/******************************************************************************/
// Description: set video mode
// Global resource dependence:
// Author: Tim.zhu
// Note:
//
/******************************************************************************/
LOCAL const SENSOR_REG_T HI255_video_mode_tab[][44]=
{
	{//video preview
		{0x03, 0x00},
		{0x01, 0xf9},
		{0x0e, 0x03},
		{0x11, 0x90},

		{0x42, 0x00},
		{0x43, 0x14},

		{0x03, 0x20},
		{0x10, 0x1C},
		{0x03, 0x22},
		{0x10, 0x69},

		{0x03, 0x20},
		{0x2a, 0x03},
		{0x2b, 0xf5},

		{0x83, 0x01}, //EXP Normal 33.33 fps
		{0x84, 0x5f},
		{0x85, 0x90},

		{0x88, 0x02}, //EXP Max 16.67 fps
		{0x89, 0xbf},
		{0x8a, 0x20},

		{0x91, 0x03}, //EXP Fix 15.00 fps
		{0x92, 0x0d},
		{0x93, 0x40},

		{0x03, 0x20},
		{0x10, 0x9C},
		{0x03, 0x22},
		{0x10, 0xe9},

		{0x03, 0x00},
		{0x11, 0x94},

		{0x03, 0x00},
		{0x0e, 0x03},
		{0x0e, 0x73},

		{0x03, 0x00},
		{0x03, 0x00},
		{0x03, 0x00},
		{0x03, 0x00},
		{0x03, 0x00},
		{0x03, 0x00},
		{0x03, 0x00},
		{0x03, 0x00},
		{0x03, 0x00},
		{0x03, 0x00},
		{0x03, 0x00},

		{0x01, 0xf8},
		{0xff, 0xff} ,
	},
	//video encode
	{
		{0x03, 0x00},
		{0x01, 0xf9},
		{0x0e, 0x03},
		{0x11, 0x90},

		{0x42, 0x00},
		{0x43, 0x14},

		{0x03, 0x20},
		{0x10, 0x1C},
		{0x03, 0x22},
		{0x10, 0x69},

		{0x03, 0x20},
		{0x2a, 0x03},
		{0x2b, 0xf5},

		{0x83, 0x01}, //EXP Normal 33.33 fps
		{0x84, 0x5f},
		{0x85, 0x90},

		{0x88, 0x02}, //EXP Max 16.67 fps
		{0x89, 0xbf},
		{0x8a, 0x20},

		{0x91, 0x03}, //EXP Fix 15.00 fps
		{0x92, 0x0d},
		{0x93, 0x40},

		{0x03, 0x20},
		{0x10, 0x9C},
		{0x03, 0x22},
		{0x10, 0xe9},

		{0x03, 0x00},
		{0x11, 0x94},

		{0x03, 0x00},
		{0x0e, 0x03},
		{0x0e, 0x73},

		{0x03, 0x00},
		{0x03, 0x00},
		{0x03, 0x00},
		{0x03, 0x00},
		{0x03, 0x00},
		{0x03, 0x00},
		{0x03, 0x00},
		{0x03, 0x00},
		{0x03, 0x00},
		{0x03, 0x00},
		{0x03, 0x00},

		{0x01, 0xf8},
		{0xff, 0xff},
	},
	{//upcc mode
		{0xff, 0xff},
	}
};
#if 1
LOCAL unsigned long _HI255_set_video_mode(unsigned long mode)
{//65.6 us

	if(mode>2)
	return 0;

	SENSOR_REG_T_PTR sensor_reg_ptr=(SENSOR_REG_T_PTR)HI255_video_mode_tab[mode];
	uint16_t i=0x00;

	//  SCI_ASSERT(PNULL!=sensor_reg_ptr);   /*assert verified*/
	if( setmode == SENSOR_MODE_PREVIEW_ONE )

	for(i=0x00; (0xff!=sensor_reg_ptr[i].reg_addr)||(0xff!=sensor_reg_ptr[i].reg_value); i++)
	{
		Sensor_WriteReg_8bits(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	SENSOR_PRINT("sensor: terry _HI255_set_video_mode = 0x%02x", mode);

	return 0;
}
#endif
/******************************************************************************/
// Description: set wb mode
// Global resource dependence:
// Author: Tim.zhu
// Note:
//
/******************************************************************************/
LOCAL const SENSOR_REG_T HI255_awb_tab[][10] =
{
	{//auto
		{0x03, 0x22},
		{0x11, 0x2e},  	//awbctrl2
		{0x80, 0x38},
		{0x81, 0x20},
		{0x82, 0x36}, //3a
		{0x83, 0x56}, //5e
		{0x84, 0x21}, //24
		{0x85, 0x4f}, //54
		{0x86, 0x20}, //24 //22
		{0xff, 0xff},
	},
	{//incandescence
		{0x03, 0x22},
		{0x11, 0x2c},
		{0x80, 0x20},
		{0x81, 0x20},
		{0x82, 0x58},
		{0x83, 0x23},
		{0x84, 0x1f},
		{0x85, 0x58},
		{0x86, 0x52},
		{0xff, 0xff},
	},
	{//u30 not used
		{0xff, 0xff}
	},
	{//CWF  not used
		{0xff, 0xff}
	},
	//FLUORESCENT:
	{//fluorescent
		{0x03, 0x22},
		{0x11, 0x2c},
		{0x80, 0x3c},//R
		{0x81, 0x20},//G
		{0x82, 0x49},//B
		{0x83, 0x3e},// Rmax
		{0x84, 0x38},// Rmin
		{0x85, 0x4d},// Bmax
		{0x86, 0x44},// Bmin
		{0xff, 0xff},
	},
	{//daylight
		{0x03, 0x22},
		{0x11, 0x2c},
		{0x80, 0x52},
		{0x81, 0x20},
		{0x82, 0x27},
		{0x83, 0x58},
		{0x84, 0x4d},
		{0x85, 0x2c},
		{0x86, 0x22},
		{0xff, 0xff},
	},
	{//cloudy
		{0x03, 0x22},
		{0x11, 0x2c},
		{0x80, 0x6f},
		{0x81, 0x1e},
		{0x82, 0x1d},
		{0x83, 0x70},
		{0x84, 0x6e},
		{0x85, 0x1e},
		{0x86, 0x1c},
		{0xff, 0xff},
	}
};

LOCAL unsigned long _HI255_set_awb(unsigned long mode)
{

	uint16_t i=0x00;
  	if(mode>6)
  		return 0;

	SENSOR_REG_T_PTR sensor_reg_ptr=(SENSOR_REG_T_PTR)HI255_awb_tab[mode];

	//  SCI_ASSERT(PNULL!=sensor_reg_ptr);

	for(i=0; (0xff!=sensor_reg_ptr[i].reg_addr)||(0xff != sensor_reg_ptr[i].reg_value); i++)
	{
		Sensor_WriteReg_8bits(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_WHITEBALANCE, (uint32_t)mode);

	SENSOR_PRINT("sensor: terry HI255_set_awb = 0x%02x", mode);
	return 0;
}

/******************************************************************************/
// Description:
// Global resource dependence:
// Author: Tim.zhu
// Note:
//        mode 0:normal;   1:night
/******************************************************************************/
  LOCAL const SENSOR_REG_T HI255_work_mode_tab[][175]=
{
	{//auto
		{0x03, 0x00},
		{0xa0, 0x00},
		{0xa2, 0x00},
		{0xa4, 0x00},
		{0xa6, 0x00},

		{0x03, 0x16},
		{0x70, 0x00},
		{0x71, 0x0e},
		{0x72, 0x1f},
		{0x73, 0x3f},
		{0x74, 0x5d},
		{0x75, 0x75},
		{0x76, 0x8a},
		{0x77, 0x9c},
		{0x78, 0xad},
		{0x79, 0xbb},
		{0x7a, 0xc6},
		{0x7b, 0xd1},
		{0x7c, 0xda},
		{0x7d, 0xe3},
		{0x7e, 0xea},
		{0x7f, 0xf1},
		{0x80, 0xf6},
		{0x81, 0xfb},
		{0x82, 0xff},

		{0x03, 0x10},
		{0x41, 0x00},
		{0x60, 0x61}, //Saturation
		{0x63, 0x50}, //090728 for W_gain 50->30
		{0x64, 0x80},

		{0x03, 0x20}, //Page 20
		{0x10, 0x1c},// AE Off

		{0x88, 0x05}, //EXP Max 8.33 fps
		{0x89, 0x7e},
		{0x8a, 0x40},

		{0xb2, 0xe0},

		{0x10, 0x9C},
			{0xff, 0xff},
	},
	{//nightmode normal
		{0x03, 0x00},
		{0xa0, 0x01},
		{0xa2, 0x01},
		{0xa4, 0x01},
		{0xa6, 0x01},

		{0x03, 0x16},
		{0x70, 0x00},
		{0x71, 0x06},
		{0x72, 0x19},
		{0x73, 0x34},
		{0x74, 0x5f},
		{0x75, 0x7b},
		{0x76, 0x8c},
		{0x77, 0x9d},
		{0x78, 0xae},
		{0x79, 0xbb},
		{0x7a, 0xc6},
		{0x7b, 0xd1},
		{0x7c, 0xda},
		{0x7d, 0xe3},
		{0x7e, 0xea},
		{0x7f, 0xf1},
		{0x80, 0xf6},
		{0x81, 0xfb},
		{0x82, 0xff},

		{0x03, 0x10},
		{0x41, 0x14},
		{0x60, 0x61},
		{0x63, 0xf0},
		{0x64, 0xf0},

		{0x03, 0x20}, //Page 20, Normal condition
		{0x10, 0x1c}, // AE Off

		{0x88, 0x0f}, //EXP Max 3.03 fps
		{0x89, 0x1b},
		{0x8a, 0x30},

		{0xb2, 0xff},

		{0x10, 0x9c}, // AE On

			{0xff, 0xff},
	},
	{//nightmode dark
		{0x03, 0x00},
		{0xa0, 0x01},
		{0xa2, 0x01},
		{0xa4, 0x01},
		{0xa6, 0x01},

		{0x03, 0x16},
		{0x70, 0x00},
		{0x71, 0x06},
		{0x72, 0x19},
		{0x73, 0x34},
		{0x74, 0x5f},
		{0x75, 0x7b},
		{0x76, 0x8c},
		{0x77, 0x9d},
		{0x78, 0xae},
		{0x79, 0xbb},
		{0x7a, 0xc6},
		{0x7b, 0xd1},
		{0x7c, 0xda},
		{0x7d, 0xe3},
		{0x7e, 0xea},
		{0x7f, 0xf1},
		{0x80, 0xf6},
		{0x81, 0xfb},
		{0x82, 0xff},

		{0x03, 0x10},
		{0x41, 0x14},
		{0x60, 0x61},
		{0x63, 0xf0},
		{0x64, 0xf0},

		{0x03, 0x20}, //Page 20, Normal condition
		{0x10, 0x1c}, // AE Off

		{0x83, 0x0f}, // Expnormal 3.03fps
		{0x84, 0x1b},
		{0x85, 0x30},

		{0x88, 0x0f}, //EXP Max 3.03 fps
		{0x89, 0x1b},
		{0x8a, 0x30},

		{0xb2, 0xff},

		{0x10, 0x9c}, // AE On
		{0xff, 0xff},
	}
};

LOCAL unsigned long _HI255_set_work_mode(unsigned long mode)
{
	uint16_t i=0x00;

	if(mode>2)
	return 0;

	SENSOR_REG_T_PTR sensor_reg_ptr=(SENSOR_REG_T_PTR)HI255_work_mode_tab[mode];

//	SCI_ASSERT(PNULL != sensor_reg_ptr);

	for(i=0; (0xff!=sensor_reg_ptr[i].reg_addr)||(0xff != sensor_reg_ptr[i].reg_value); i++)
	{
		Sensor_WriteReg_8bits(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	//   Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_SCENECAPTURETYPE, (uint32)mode);

	CMR_LOGI("sensor: terry set_work_mode: mode = %d.\n", mode);
	return 0;
}

/******************************************************************************/
// Description:
// Global resource dependence:
// Author: Tim.zhu
// Note:
//
/******************************************************************************/
LOCAL unsigned long _HI255_BeforeSnapshot(unsigned long param)
{
	uint32_t cap_mode = (param>>CAP_MODE_BITS);

	param = param&0xffff;
	CMR_LOGE("%d,%ld.",cap_mode,param);

	CMR_LOGE("HI255: HI255_before_snapshot\n");
	if(SENSOR_MODE_PREVIEW_ONE>=param)
	{
	 	return SENSOR_SUCCESS;
	}
	CMR_LOGE("terry  _HI255_BeforeSnapshot =%ld.\n",param);

	  _HI255_SetExifInfo_ISO((uint32_t)param);

	Sensor_SetMode((uint32_t)param);
	Sensor_SetMode_WaitDone();
	return SENSOR_SUCCESS;
}

/******************************************************************************/
// Description:
// Global resource dependence:
// Author: Tim.zhu
// Note:
//
/******************************************************************************/
LOCAL unsigned long _HI255_check_image_format_support(unsigned long param)
{
	uint32_t ret_val = SENSOR_FAIL;

	switch(param)
	{
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

/******************************************************************************/
// Description:
// Global resource dependence:
// Author: Tim.zhu
// Note:
//
/******************************************************************************/
LOCAL unsigned long _HI255_pick_out_jpeg_stream(unsigned long param)
{
	return 0;
}
/******************************************************************************/
// Description:
// Global resource dependence:
// Author: Tim.zhu
// Note:
//
/******************************************************************************/
LOCAL unsigned long _HI255_after_snapshot(unsigned long param)
{
	//s_HI255_resolution_Tab_YUV[SENSOR_MODE_PREVIEW_ONE].sensor_reg_tab_ptr = (SENSOR_REG_T*)HI255_preview;
	//s_HI255_resolution_Tab_YUV[SENSOR_MODE_PREVIEW_ONE].reg_count = NUMBER_OF_ARRAY(HI255_preview);
       uint32_t exposure;

	SENSOR_PRINT("terry  _HI255_after_snapshot =%ld.\n",param);

    	Sensor_WriteReg(0x03, 0x20);
	exposure=Sensor_ReadReg(0x80);
	exposure=( exposure<<8 ) | Sensor_ReadReg(0x81);
	exposure=( exposure<<8 ) | Sensor_ReadReg(0x82);
	_HI255_SetExifInfo_exposure(exposure);

	Sensor_SetMode((uint32_t)param);
	return SENSOR_SUCCESS;
}
#if 0
LOCAL uint32_t _ov540_flash(uint32_t param)
{
	printk("_ov540_flash:param=%d .\n",param);

	if(param)
	{
		/* enable flash, disable in _HI255_BeforeSnapshot */
		g_flash_mode_en = param;
	}
	Sensor_SetFlash(param);

	printk("_ov540_flash:end .\n");
}
#endif
#if 0
LOCAL uint32_t HI255_InitExt(uint32_t param)
{
	uint32_t            rtn = SENSOR_SUCCESS;
	int 				ret = 0;
	uint32_t            i = 0;
	uint32_t            written_num = 0;
	uint16_t            wr_reg = 0;
	uint16_t            wr_val = 0;
	uint32_t            wr_num_once = 0;
	uint32_t            wr_num_once_ret = 0;
	uint32_t		   	alloc_size = 0;
	uint32_t            init_table_size = s_HI255_resolution_Tab_YUV[SENSOR_MODE_COMMON_INIT].reg_count;
	SENSOR_REG_T_PTR    p_reg_table = s_HI255_resolution_Tab_YUV[SENSOR_MODE_COMMON_INIT].sensor_reg_tab_ptr;
	uint8_t             *p_reg_val_tmp = 0;
	nsecs_t 			timestamp_old;
	nsecs_t				timestamp_new;

	timestamp_old = systemTime(CLOCK_MONOTONIC);

	SENSOR_PRINT("SENSOR: HI255_InitExt, init_table_size = %d \n", init_table_size);

	alloc_size = init_table_size*sizeof(uint8_t) + 16;
	p_reg_val_tmp = (uint8_t*)malloc(alloc_size);

	SENSOR_PRINT("_s5k5ccgx_InitExt: alloc size = %d \n", alloc_size);
	if(0 == p_reg_val_tmp)
	{
		SENSOR_PRINT("_s5k5ccgx_InitExt: alloc failed, size = %d \n", alloc_size);
		return 1;
	}

	while(written_num < init_table_size)
	{
		wr_num_once = 2;

		wr_reg = p_reg_table[written_num].reg_addr;
		wr_val = p_reg_table[written_num].reg_value;
		if(SENSOR_WRITE_DELAY == wr_reg){
			SENSOR_Sleep(wr_val);
		}else{
			p_reg_val_tmp[0] = (uint8_t)(wr_reg & 0xFF);
			p_reg_val_tmp[1] = (uint8_t)(wr_val & 0xFF);

			if ((0x0e == wr_reg) && (0x01 == wr_val)){
				for(i = written_num + 1; i< init_table_size; i++)
				{
					if((0x0e == wr_reg) && (0x00 == wr_val))
					{
						break;
					}
					else
					{
						wr_val = p_reg_table[i].reg_value;
						p_reg_val_tmp[wr_num_once] = (uint8_t)(wr_val & 0xFF);
						wr_num_once ++;
					}
				}
			}

			for (i = 0; i < 4; i++) {
				ret = Sensor_WriteData(p_reg_val_tmp, wr_num_once);
				if(ret!=0){
					SENSOR_PRINT("SENSOR: HI255_InitExt, i2c write error, ret=%d \n", ret);
					continue;
				}else{
					break;
				}
			}
		}
		written_num += wr_num_once-1;
	}

    free(p_reg_val_tmp);

	timestamp_new = systemTime(CLOCK_MONOTONIC);
    SENSOR_PRINT("SENSOR: HI255_InitExt time=%d us\n",(timestamp_new-timestamp_old)/1000);

    SENSOR_PRINT("SENSOR: HI255_InitExt, done: ret=%d \n", ret);

    return rtn;
}
#endif


LOCAL uint8_t  convert_ascii2num(uint8_t v)
{
	if (v >= '0'&& v <='9')
		return v - '0';
	else if (v >= 'a' && v <='f')
		return v - 'a' + 0xa;
	else if (v >= 'A' && v <='F')
		return v - 'A' + 0xa;
	else
		return 0xff;
}

LOCAL uint32_t sensor_tflash_debug(char* filename)
{
#define STATE_LINE_BEGIN 0
#define STATE_LINE_IGNORE 1
#define STATE_LEFT 2
#define STATE_ARRAY_0_0 3
#define STATE_ARRAY_0_X 4
#define STATE_ARRAY_0_DATA0 5
#define STATE_ARRAY_0_DATA1 6
#define STATE_ARRAY_0_DATA2 7
#define STATE_ARRAY_0_DATA3 8
#define STATE_ARRAY_0_END 9
#define STATE_ARRAY_1_0 10
#define STATE_ARRAY_1_X 11
#define STATE_ARRAY_1_DATA0 12
#define STATE_ARRAY_1_DATA1 13
#define STATE_ARRAY_1_DATA2 14
#define STATE_ARRAY_1_DATA3 15
#define STATE_LINE_END 16

#define HEAD 0
#define TAIL 2

	uint32_t i;
	uint8_t v = 0;
	FILE *file = NULL;
	uint32_t file_len = 0;
	int state = STATE_LINE_BEGIN;
	uint16_t reg;
	uint16_t value;
       uint8_t reg_addr_value_bits =  g_hi255_yuv_info.reg_addr_value_bits;
	uint32_t		   	alloc_size = 0;
	uint32_t            init_table_size = s_HI255_resolution_Tab_YUV[SENSOR_MODE_COMMON_INIT].reg_count;
	SENSOR_REG_T_PTR    p_reg_table = s_HI255_resolution_Tab_YUV[SENSOR_MODE_COMMON_INIT].sensor_reg_tab_ptr;
	uint16_t             *p_reg_val_tmp = 0;
	uint32_t            wr_num_once = 0;
       uint32_t            written_num = 0;
       uint8_t  group =0;
	alloc_size = 2*init_table_size*sizeof(uint16_t) + 1024;
	p_reg_val_tmp = (uint16_t*)malloc(alloc_size);
	if(0 == p_reg_val_tmp)
              return file_len;
    if (( file = fopen(filename, "rb") ))
    {

        SENSOR_PRINT("%s file=%p.\n",__func__, file);
       fseek(file, 0, TAIL);
	file_len = ftell(file);
	if (0 > ((int)file_len)) {
		SENSOR_PRINT("ftell file length negative %d", (int)file_len);
		fclose(file);
		return file_len;
	}
       fseek(file, 0, HEAD);
	SENSOR_PRINT("load sensor setting from file: %s ;file_len=%d;\n", filename,file_len);

	for (i = 0 ; i < file_len ; i++) {
		if (fread((char *)&v, 1, 1,file))
			continue;

		if (' ' == v || '\t' == v)
			continue;
		if ('\n' == v) {
			state = STATE_LINE_BEGIN;
			continue;
		}

		if (STATE_LINE_BEGIN == state) {
			if ('{' == v)
				state = STATE_LEFT;
			if ('/' == v)
				state = STATE_LINE_IGNORE;
		} else if (STATE_LINE_IGNORE == state) {
		} else if (STATE_LEFT == state) {
			if ('0' == v)
				state = STATE_ARRAY_0_0;
			else
				state = STATE_LINE_IGNORE;
		} else if (STATE_ARRAY_0_0 == state) {
			if ('x' == v || 'X' == v)
				state = STATE_ARRAY_0_X;
			else
				state = STATE_LINE_IGNORE;
		} else if (STATE_ARRAY_0_X == state) {
			v = convert_ascii2num(v);
			if (v <= 0xf) {
			  reg = ((unsigned int)v);
				state = STATE_ARRAY_0_DATA0;
			} else
				state = STATE_LINE_IGNORE;
		} else if (STATE_ARRAY_0_DATA0 == state) {
			v = convert_ascii2num(v);
			if (v <= 0xf) {
                       reg =reg <<4;
			  reg |= ((unsigned int)v);
                       if( (reg_addr_value_bits & SENSOR_I2C_REG_16BIT) != SENSOR_I2C_REG_16BIT)
                        {
                            //if(reg == 0xff)
				//state = STATE_ARRAY_0_DATA1;
                            //else
				state = STATE_ARRAY_0_DATA3;
                        }
                        else
				state = STATE_ARRAY_0_DATA1;
			} else
				state = STATE_LINE_IGNORE;
		} else if (STATE_ARRAY_0_DATA1 == state) {
			v = convert_ascii2num(v);
			if (v <= 0xf) {
                       reg =reg <<4;
			  reg |= ((unsigned int)v);
				state = STATE_ARRAY_0_DATA2;
			} else
				state = STATE_LINE_IGNORE;
		} else if (STATE_ARRAY_0_DATA2 == state) {
			v = convert_ascii2num(v);
			if (v <= 0xf) {
                       reg =reg <<4;
			  reg |= ((unsigned int)v);
				state = STATE_ARRAY_0_DATA3;
			} else
				state = STATE_LINE_IGNORE;
		} else if (STATE_ARRAY_0_DATA3 == state) {
			if (',' == v)
				state = STATE_ARRAY_0_END;
		} else if (STATE_ARRAY_0_END == state) {
			if ('0' == v)
				state = STATE_ARRAY_1_0;
			else
				state = STATE_LINE_IGNORE;
		} else if (STATE_ARRAY_1_0 == state) {
			if ('x' == v || 'X' == v)
				state = STATE_ARRAY_1_X;
			else
				state = STATE_LINE_IGNORE;
		} else if (STATE_ARRAY_1_X == state) {
			v = convert_ascii2num(v);
			if (v <= 0xf) {
			       value = ((unsigned int)v);
				state = STATE_ARRAY_1_DATA0;
			} else
				state = STATE_LINE_IGNORE;
		} else if (STATE_ARRAY_1_DATA0 == state) {
			v = convert_ascii2num(v);
			if (v <= 0xf) {
                            value =value <<4;
				value |= v;
                       if( (reg_addr_value_bits &SENSOR_I2C_VAL_16BIT) != SENSOR_I2C_VAL_16BIT)
				state = STATE_ARRAY_1_DATA3;
                        else
				state = STATE_ARRAY_1_DATA1;
			}else
				state = STATE_LINE_IGNORE;
		}else if (STATE_ARRAY_1_DATA1 == state) {
			v = convert_ascii2num(v);
			if (v <= 0xf) {
                            value =value <<4;
				value |= v;
				state = STATE_ARRAY_1_DATA2;
			} else
				state = STATE_LINE_IGNORE;
		}else if (STATE_ARRAY_1_DATA2 == state) {
			v = convert_ascii2num(v);
			if (v <= 0xf) {
                            value =value <<4;
				value |= v;
				state = STATE_ARRAY_1_DATA3;
			}else
				state = STATE_LINE_IGNORE;
		}else if (STATE_ARRAY_1_DATA3 == state) {
			if (',' == v){

		      p_reg_val_tmp[written_num++]=reg;
		      p_reg_val_tmp[written_num++]=value;


                 #if 0
                         if(group)
                          {
                              if((0x0e == reg) && (0x00 == value))
                               {
                                        group = 0;
                                        Sensor_WriteData(p_reg_val_tmp, wr_num_once);
//				            SENSOR_PRINT(" *%d* \n", wr_num_once);
                                }
                               else
                               {
                                        wr_num_once++;
                                        p_reg_val_tmp[wr_num_once] = value;
                                }
                         }
                        else if( reg == SENSOR_WRITE_DELAY )
                          {

	SENSOR_Sleep(value);
	                            SENSOR_PRINT(" SENSOR_WRITE_DELAY =%d; \n",value);
                          }
                        else
                         {
                                wr_num_once = 2;
			           p_reg_val_tmp[0] = reg;
			           p_reg_val_tmp[1] = value;
			            if ((0x0e == reg) && (0x01 == value))
                                     group =1 ;
                                 else{

                                     Sensor_WriteData(p_reg_val_tmp, wr_num_once);
//				         SENSOR_PRINT(" *%d* \n", wr_num_once);
                                    }

                         }
                        #endif
#if 1
                       if( (reg_addr_value_bits &SENSOR_I2C_REG_16BIT) != SENSOR_I2C_REG_16BIT
                              && (reg_addr_value_bits &SENSOR_I2C_VAL_16BIT) != SENSOR_I2C_VAL_16BIT
                             )
                        {
				SENSOR_PRINT("{0x%02x, 0x%02x}, \n", reg, value);
                        }
                       else if( (reg_addr_value_bits &SENSOR_I2C_REG_16BIT) == SENSOR_I2C_REG_16BIT
                              && (reg_addr_value_bits &SENSOR_I2C_VAL_16BIT) != SENSOR_I2C_VAL_16BIT
                             )
                        {
				SENSOR_PRINT("{0x%04x, 0x%02x}, \n", reg, value);
                        }
                       else if( (reg_addr_value_bits &SENSOR_I2C_REG_16BIT) == SENSOR_I2C_REG_16BIT
                              && (reg_addr_value_bits &SENSOR_I2C_VAL_16BIT) == SENSOR_I2C_VAL_16BIT
                             )
                        {
				SENSOR_PRINT("{0x%04x, 0x%04x}, \n", reg, value);
                        }
                       else
                        {
				SENSOR_PRINT("{0x%02x, 0x%04x}, \n", reg, value);
                        }
#endif
				state = STATE_LINE_IGNORE;
                     }
		}
	}
	{
		SENSOR_REG_TAB_T regTab;
		regTab.reg_count			= written_num >>1;
		regTab.reg_bits				= SENSOR_I2C_REG_8BIT | SENSOR_I2C_VAL_8BIT;
		regTab.burst_mode			= 7;
		regTab.sensor_reg_tab_ptr 	= (SENSOR_REG_T_PTR)p_reg_val_tmp;

		Sensor_Device_WriteRegTab(&regTab);
	}

	fclose(file);
     }
	free(p_reg_val_tmp);

  return file_len;
}

LOCAL uint32_t sensor_InitTflash(uint32_t param)

{
	int32_t ret =0;
	nsecs_t 			timestamp_old;
	nsecs_t				timestamp_new;
       SENSOR_IOCTL_FUNC_TAB_T_PTR set_ioctl_func_tab_ptr =&s_HI255_ioctl_func_tab;


	SENSOR_PRINT("SENSOR: %s, mode=%d; \n", __func__,param);

	timestamp_old = systemTime(CLOCK_MONOTONIC);
    if ( param == SENSOR_MODE_COMMON_INIT )
          {
              ret =sensor_tflash_debug("/sdcard/DCIM/Camera/cinit.txt");
              if(ret)
                {
                    set_ioctl_func_tab_ptr -> set_brightness = PNULL;
                    set_ioctl_func_tab_ptr -> set_contrast = PNULL;
                    set_ioctl_func_tab_ptr -> set_sharpness = PNULL;
                    set_ioctl_func_tab_ptr -> set_saturation = PNULL;
                    set_ioctl_func_tab_ptr -> set_preview_mode = PNULL;
                    set_ioctl_func_tab_ptr -> set_image_effect = PNULL;
                    set_ioctl_func_tab_ptr -> set_wb_mode = PNULL;
                    set_ioctl_func_tab_ptr -> set_iso = PNULL;
                    set_ioctl_func_tab_ptr -> set_exposure_compensation = PNULL;
                    set_ioctl_func_tab_ptr -> set_anti_banding_flicker = PNULL;
                    set_ioctl_func_tab_ptr -> set_video_mode =PNULL;
                }

           }
       else if ( param == SENSOR_MODE_PREVIEW_ONE )
            ret =sensor_tflash_debug("/sdcard/DCIM/Camera/c640480.txt");
       else if ( param == SENSOR_MODE_SNAPSHOT_ONE_FIRST )
            ret =sensor_tflash_debug("/sdcard/DCIM/Camera/c1280720.txt");
       else if ( param == 3 )
            ret =sensor_tflash_debug("/sdcard/DCIM/Camera/c1280960.txt");
       else if ( param == 4 )
            ret =sensor_tflash_debug("/sdcard/DCIM/Camera/c16001200.txt");
       else if ( param == 5 )
            ret =sensor_tflash_debug("/sdcard/DCIM/Camera/c20481536.txt");

	timestamp_new = systemTime(CLOCK_MONOTONIC);
	SENSOR_PRINT("SENSOR: sensor_InitTflash end, ret=%d, time=%d us\n", ret, (uint32_t)((timestamp_new-timestamp_old)/1000));


       if(ret >0)

	return  SENSOR_SUCCESS;
       else

	return  SENSOR_FAIL;
}
/**---------------------------------------------------------------------------*
 **                         Compiler Flag                                     *
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif

