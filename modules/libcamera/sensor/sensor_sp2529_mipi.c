
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
#include <assert.h>
#define SCI_TRUE                1
#define SCI_FALSE               0
#define SCI_ASSERT assert
#define SCI_SUCCESS 0

//#define  SP2529_TCARD_TEST
#ifdef SP2529_TCARD_TEST
#include <fcntl.h>              /* low-level i/o */
#include <errno.h>
#include <sys/ioctl.h>

#include "sensor_cfg.h"
#include "sensor_drv_u.h"
#include "cmr_msg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
 //void    _sp5408_sdcard(char *);
//#define SENSOR_TRACE printk
int num_sd;
#endif

#define SP2529_I2C_ADDR_W 0x30
#define SP2529_I2C_ADDR_R 0x31
#define SENSOR_GAIN_SCALE 16

 typedef enum
{
	DCAMERA_FLICKER_50HZ = 0,
	DCAMERA_FLICKER_60HZ,
	FLICKER_MAX
}DCAMERA_FLICKER;
static BOOLEAN video_mode = SCI_FALSE;

static unsigned long set_SP2529_ae_enable(unsigned long enable);
static unsigned long SP2529_PowerOn(unsigned long power_on);
static unsigned long set_preview_mode(unsigned long preview_mode);
static unsigned long SP2529_Identify(unsigned long param);
static unsigned long SP2529_BeforeSnapshot(unsigned long param);
static unsigned long SP2529_After_Snapshot(unsigned long param);
static unsigned long set_brightness(unsigned long level);
static unsigned long set_contrast(unsigned long level);
static unsigned long set_sharpness(unsigned long level);
static unsigned long set_saturation(unsigned long level);
static unsigned long set_image_effect(unsigned long effect_type);
static unsigned long read_ev_value(unsigned long value);
static unsigned long write_ev_value(unsigned long exposure_value);
static unsigned long read_gain_value(unsigned long value);
static unsigned long write_gain_value(unsigned long gain_value);
static unsigned long read_gain_scale(unsigned long value);
static unsigned long set_frame_rate(unsigned long param);
static unsigned long set_SP2529_ev(unsigned long level);
static unsigned long set_SP2529_awb(unsigned long mode);
static unsigned long set_SP2529_anti_flicker(unsigned long mode);
static unsigned long set_SP2529_video_mode(unsigned long mode);
static void SP2529_set_shutter();
static unsigned long set_sensor_flip(unsigned long param);
static unsigned long SP2529_StreamOn(unsigned long param);
static unsigned long SP2529_StreamOff(unsigned long param);
LOCAL unsigned long _SP2529_GetResolutionTrimTab(unsigned long param);
static uint32_t SP2529_PRE_MODE = 0;

uint16_t	G_shutter;
uint8_t G_Gain;
uint8_t SP2529_Cap_Flag=0;

//AE target
#define  SP2529_P1_0xeb  0x95//78//80//8c fs
#define  SP2529_P1_0xec  0x8c//6e//74//8c fs

//sharpeness
#define  SP2529_P2_0xe9  0x68//78//80
#define  SP2529_P2_0xed  0x7e//6e//74//88 fs


//sat
#define  SP2529_P1_0xd3  0xaa//78//80
#define  SP2529_P1_0xd4  0xaa//78//80
#define  SP2529_P1_0xd5  0x9a//78//80
#define  SP2529_P1_0xd6  0x90//78//80
#define  SP2529_P1_0xd7  0xaa//78//80
#define  SP2529_P1_0xd8  0xaa//6e//74
#define  SP2529_P1_0xd9  0x9a//78//80
#define  SP2529_P1_0xda  0x90//78//80


//HEQ
#define  SP2529_P1_0x10  0x8a ////ku_outdoor  equal 0xdd of SP2529 //0x80
#define  SP2529_P1_0x11  0x8a //ku indoor  //0x80
#define  SP2529_P1_0x12  0x80 //ku dummy //0x80
#define  SP2529_P1_0x13  0x80 //ku low  //0x80

#define  SP2529_P1_0x14  0xa0 //kl outdoor //0xa0
#define  SP2529_P1_0x15  0xa0 //kl indoor  //0xa0
#define  SP2529_P1_0x16  0xa0// kl dummy //0xa0
#define  SP2529_P1_0x17  0xa0 //kl lowlight //0xa0//a8 fs


LOCAL uint32_t			Antiflicker		 = DCAMERA_FLICKER_50HZ;

#ifdef SP2529_TCARD_TEST
SENSOR_REG_T SP2529_YUV_COMMON[1000]=
#else
static SENSOR_REG_T SP2529_YUV_COMMON[]=
#endif
{
{0xfd,0x01},
{0x36,0x00},
{0xfd,0x00},
{0xe7,0x03},
{0xe7,0x00},
{0xfd,0x00},
{0x31,0x00},
{0x33,0x00},
{0x95,0x06},
{0x94,0x40},
{0x97,0x04},
{0x96,0xb0},
{0x98,0x3a},
{0x1d,0x00},
{0x1b,0x37},
{0x0c,0x66},
{0x27,0xa5},
{0x28,0x08},
{0x1e,0x13},
{0x21,0x10},
{0x15,0x16},
{0x71,0x18},
{0x7c,0x18},
{0x76,0x19},
{0xfd,0x02},
{0x85,0x00},
{0xfd,0x00},
{0x11,0x40},
{0x18,0x00},
{0x1a,0x49},
{0x1e,0x01},
{0x0c,0x55},
{0x21,0x10},
{0x22,0x2a},
{0x25,0xad},
{0x27,0xa1},
{0x1c,0x1f},
{0x1f,0xe0},
{0x28,0x0b},
{0x2b,0x8c},
{0x26,0x09},
{0x2c,0x45},
{0x37,0x00},
{0x16,0x01},
{0x17,0x2f},
{0x69,0x01},
{0x6a,0x2d},
{0x13,0x4f},
{0x6b,0x50},
{0x6c,0x50},
{0x6f,0x50},
{0x73,0x51},
{0x7a,0x41},
{0x70,0x41},
{0x7d,0x40},
{0x74,0x40},
{0x75,0x40},
{0x14,0x01},
{0x15,0x20},
{0x71,0x22},
{0x76,0x22},
{0x7c,0x22},
{0x7e,0x21},
{0x72,0x21},
{0x77,0x20},
{0xfd,0x00},
{0xfd,0x01},
{0xf2,0x09},
{0x32,0x00},
{0xfb,0x25},
{0xfd,0x02},
{0x85,0x00},
{0x00,0x82},
{0x01,0x82},
{0xfd,0x00},
{0x2e,0x85},
{0x1e,0x01},
{0xfd,0x00},
{0x2f,0x11},
{0x03,0x03},
{0x04,0x30},
{0x05,0x00},
{0x06,0x00},
{0x07,0x00},
{0x08,0x00},
{0x09,0x00},
{0x0a,0xb5},
{0xfd,0x01},
{0xf0,0x00},
{0xf7,0x88},
{0xf8,0x71},
{0x02,0x0b},
{0x03,0x01},
{0x06,0x88},
{0x07,0x00},
{0x08,0x01},
{0x09,0x00},
{0xfd,0x02},
{0x3d,0x0d},
{0x3e,0x71},
{0x3f,0x00},
{0x88,0xc3},
{0x89,0x87},
{0x8a,0x43},
{0xfd,0x02},
{0xbe,0xd0},
{0xbf,0x04},
{0xd0,0xd0},
{0xd1,0x04},
{0xc9,0xd0},
{0xca,0x04},
{0xb8,0x90},
{0xb9,0x85},
{0xba,0x30},
{0xbb,0x45},
{0xbc,0xc0},
{0xbd,0x60},
{0xfd,0x03},
{0x77,0x48},
{0xfd,0x01},
{0xe0,0x48},
{0xe1,0x38},
{0xe2,0x30},
{0xe3,0x2c},
{0xe4,0x2c},
{0xe5,0x2a},
{0xe6,0x2a},
{0xe7,0x28},
{0xe8,0x28},
{0xe9,0x28},
{0xea,0x26},
{0xf3,0x26},
{0xf4,0x26},
{0xfd,0x01},
{0x04,0xc8},
{0x05,0x27},
{0x0a,0x48},
{0x0b,0x26},
{0xfd,0x01},
{0xf2,0x09},
{0xeb,0x95},
{0xec,0x8C},
{0xed,0x06},
{0xee,0x0a},
{0xfd,0x03},
{0x52,0xff},
{0x53,0x60},
{0x94,0x20},
{0x54,0x00},
{0x55,0x00},
{0x56,0x80},
{0x57,0x80},
{0x95,0x80},
{0x58,0x00},
{0x59,0x00},
{0x5a,0xf6},
{0x5b,0x00},
{0x5c,0x88},
{0x5d,0x00},
{0x96,0x68},
{0xfd,0x03},
{0x8a,0x00},
{0x8b,0x00},
{0x8c,0x00},
{0x22,0xff},
{0x23,0xff},
{0x24,0xff},
{0x25,0xff},
{0x5e,0xff},
{0x5f,0xff},
{0x60,0xff},
{0x61,0xff},
{0x62,0x00},
{0x63,0x00},
{0x64,0x00},
{0x65,0x00},
{0xfd,0x01},
{0x21,0x00},
{0x22,0x00},
{0x26,0xa0},
{0x27,0x14},
{0x28,0x05},
{0x29,0x20},
{0x2a,0x01},
{0xfd,0x01},
{0xa1,0x25},
{0xa2,0x2c},
{0xa3,0x29},
{0xa4,0x31},
{0xa5,0x21},
{0xa6,0x27},
{0xa7,0x23},
{0xa8,0x30},
{0xa9,0x1e},
{0xaa,0x26},
{0xab,0x20},
{0xac,0x2d},
{0xad,0x07},
{0xae,0x05},
{0xaf,0x03},
{0xb0,0x05},
{0xb1,0x06},
{0xb2,0x05},
{0xb3,0x05},
{0xb4,0x07},
{0xb5,0x0a},
{0xb6,0x08},
{0xb7,0x06},
{0xb8,0x07},
{0xfd,0x02},
{0x26,0xa0},
{0x27,0x96},
{0x28,0xcc},
{0x29,0x01},
{0x2a,0x00},
{0x2b,0x00},
{0x2c,0x20},
{0x2d,0xdc},
{0x2e,0x20},
{0x2f,0x96},
{0x1b,0x80},
{0x1a,0x80},
{0x18,0x16},
{0x19,0x26},
{0x66,0x27},
{0x67,0x52},
{0x68,0xbd},
{0x69,0xe9},
{0x6a,0xa5},
{0x7c,0x26},
{0x7d,0x4A},
{0x7e,0xe0},
{0x7f,0x05},
{0x80,0xa6},
{0x70,0x21},
{0x71,0x41},
{0x72,0x05},
{0x73,0x25},
{0x74,0xaa},
{0x6b,0x00},
{0x6c,0x20},
{0x6d,0x0e},
{0x6e,0x2a},
{0x6f,0xaa},
{0x61,0xdb},
{0x62,0xfe},
{0x63,0x37},
{0x64,0x56},
{0x65,0x5a},
{0x75,0x00},
{0x76,0x09},
{0x77,0x02},
{0x0e,0x16},
{0x3b,0x09},
{0xfd,0x02},
{0x02,0x00},
{0x03,0x10},
{0x04,0xf0},
{0xf5,0xb3},
{0xf6,0x80},
{0xf7,0xe0},
{0xf8,0x89},
{0xfd,0x02},
{0x08,0x00},
{0x09,0x04},
{0xfd,0x02},
{0xdd,0x0f},
{0xde,0x0f},
{0xfd,0x02},
{0x57,0x30},
{0x58,0x10},
{0x59,0xe0},
{0x5a,0x00},
{0x5b,0x12},
{0xcb,0x04},
{0xcc,0x07},
{0xcd,0x10},
{0xce,0x1a},
{0xfd,0x03},
{0x87,0x04},
{0x88,0x08},
{0x89,0x10},
{0xfd,0x02},
{0xe8,0x68},
{0xec,0x7a},
{0xe9,0x68},
{0xed,0x7e},
{0xea,0x58},
{0xee,0x60},
{0xeb,0x48},
{0xef,0x40},
{0xfd,0x02},
{0xdc,0x04},
{0x05,0x6f},
{0xfd,0x02},
{0xf4,0x30},
{0xfd,0x03},
{0x97,0x98},
{0x98,0x88},
{0x99,0x88},
{0x9a,0x80},
{0xfd,0x02},
{0xe4,0xff},
{0xe5,0xff},
{0xe6,0xff},
{0xe7,0xff},
{0xfd,0x03},
{0x72,0x18},
{0x73,0x28},
{0x74,0x28},
{0x75,0x30},
{0xfd,0x02},
{0x78,0x20},
{0x79,0x20},
{0x7a,0x14},
{0x7b,0x08},
{0x81,0x02},
{0x82,0x20},
{0x83,0x20},
{0x84,0x08},
{0xfd,0x03},
{0x7e,0x06},
{0x7f,0x0d},
{0x80,0x10},
{0x81,0x16},
{0x7c,0xff},
{0x82,0x54},
{0x83,0x43},
{0x84,0x00},
{0x85,0x20},
{0x86,0x40},
{0xfd,0x03},
{0x66,0x18},
{0x67,0x28},
{0x68,0x20},
{0x69,0x88},
{0x9b,0x18},
{0x9c,0x28},
{0x9d,0x20},
{0xfd,0x01},
{0x8b,0x00},
{0x8c,0x13},
{0x8d,0x21},
{0x8e,0x32},
{0x8f,0x43},
{0x90,0x5c},
{0x91,0x6c},
{0x92,0x7a},
{0x93,0x87},
{0x94,0x9c},
{0x95,0xaa},
{0x96,0xba},
{0x97,0xc5},
{0x98,0xce},
{0x99,0xd4},
{0x9a,0xdc},
{0x9b,0xe3},
{0x9c,0xea},
{0x9d,0xef},
{0x9e,0xf5},
{0x9f,0xfb},
{0xa0,0xff},
{0xfd,0x02},
{0x15,0xa9},
{0x16,0x84},
{0xa0,0x74},
{0xa1,0x06},
{0xa2,0x06},
{0xa3,0xfa},
{0xa4,0x8c},
{0xa5,0xfa},
{0xa6,0xfa},
{0xa7,0xe9},
{0xa8,0x9d},
{0xa9,0x00},
{0xaa,0x33},
{0xab,0x0f},
{0xac,0x54},
{0xad,0x34},
{0xae,0xf8},
{0xaf,0xd4},
{0xb0,0xd9},
{0xb1,0xd4},
{0xb2,0xc0},
{0xb3,0xe6},
{0xb4,0xda},
{0xb5,0x30},
{0xb6,0x33},
{0xb7,0x0f},
{0xfd,0x01},
{0xd2,0x2d},
{0xd1,0x38},
{0xdd,0x3f},
{0xde,0x37},
{0xfd,0x02},
{0xc1,0x40},
{0xc2,0x40},
{0xc3,0x40},
{0xc4,0x40},
{0xc5,0x80},
{0xc6,0x60},
{0xc7,0x00},
{0xc8,0x00},
{0xfd,0x01},
{0xd3,0xaa},
{0xd4,0xaa},
{0xd5,0x9a},
{0xd6,0x90},
{0xd7,0xaa},
{0xd8,0xaa},
{0xd9,0x9a},
{0xda,0x90},
{0xfd,0x03},
{0x76,0x0a},
{0x7a,0x40},
{0x7b,0x40},
{0xfd,0x01},
{0xc2,0xaa},
{0xc3,0xaa},
{0xc4,0x66},
{0xc5,0x66},
{0xfd,0x01},
{0xcd,0x08},
{0xce,0x18},
{0xfd,0x02},
{0x32,0x78},
{0x35,0x60},
{0x37,0x13},
{0xfd,0x01},
{0xdb,0x00},
{0x10,0x8A},
{0x11,0x8A},
{0x12,0x80},
{0x13,0x80},
{0x14,0xa0},
{0x15,0xa0},
{0x16,0xa0},
{0x17,0xa0},
{0xfd,0x03},
{0x00,0x80},
{0x03,0x68},
{0x06,0xd8},
{0x07,0x28},
{0x0a,0xfd},
{0x01,0x16},
{0x02,0x16},
{0x04,0x16},
{0x05,0x16},
{0x0b,0x40},
{0x0c,0x40},
{0x0d,0x40},
{0x0e,0x40},
{0x08,0x0c},
{0x09,0x0c},
{0xfd,0x02},
{0x8e,0x0a},
{0x8f,0x03},
{0x90,0x40},
{0x91,0x40},
{0x92,0x60},
{0x93,0x80},
{0x94,0x80},
{0x95,0x80},
{0x96,0x80},
{0x97,0x80},
{0x98,0x80},
{0x99,0x80},
{0x9a,0x80},
{0x9b,0x80},
{0x9e,0x44},
{0x9f,0x44},
{0xfd,0x02},
{0x85,0x00},
{0xfd,0x01},
{0x00,0x00},
{0xfb,0x25},
{0x32,0x15},
{0x33,0xef},
{0x34,0xef},
{0x35,0x40},
{0xfd,0x00},
{0x3f,0x00},
{0xfd,0x01},
{0x50,0x00},
{0x66,0x00},
{0xfd,0x02},
{0xd6,0x0f},
};

#ifdef SP2529_TCARD_TEST
	#define SP2528_OP_CODE_INI		0x00		/* Initial value. */
	#define SP2528_OP_CODE_REG		0x01		/* Register */
	#define SP2528_OP_CODE_DLY		0x02		/* Delay */
	#define SP2528_OP_CODE_END		0x03		/* End of initial setting. */


	#define u16  unsigned short
	#define u8   unsigned char
	#define u32  unsigned int
		typedef struct
	{
		u16 init_reg;
		u16 init_val;	/* Save the register value and delay tick */
		u8 op_code;		/* 0 - Initial value, 1 - Register, 2 - Delay, 3 - End of setting. */
	} SP2528_initial_set_struct;

	SP2528_initial_set_struct SP2528_Init_Reg[500];




unsigned char data_buff[10*1024];
unsigned char *curr_ptr ;
FILE*fp;
int file_size;
int flag_sd =1;



 u32 sp_strtol(const char *nptr, u8 base)
{
	u32 ret;

	if(!nptr || (base!=16 && base!=10 && base!=8))
	{
		SENSOR_TRACE("%s(): NULL pointer input\n", __FUNCTION__);
		return -1;
	}
	for(ret=0; *nptr; nptr++)
	{
		if((base==16 && *nptr>='A' && *nptr<='F') ||
			(base==16 && *nptr>='a' && *nptr<='f') ||
			(base>=10 && *nptr>='0' && *nptr<='9') ||
			(base>=8 && *nptr>='0' && *nptr<='7') )
		{
			ret *= base;
			if(base==16 && *nptr>='A' && *nptr<='F')
				ret += *nptr-'A'+10;
			else if(base==16 && *nptr>='a' && *nptr<='f')
				ret += *nptr-'a'+10;
			else if(base>=10 && *nptr>='0' && *nptr<='9')
				ret += *nptr-'0';
			else if(base>=8 && *nptr>='0' && *nptr<='7')
				ret += *nptr-'0';
		}
		else
			return ret;
	}

	return ret;
}



#if 1
unsigned char SP2528_Initialize_from_T_Flash()
{
	u8 func_ind[4] = {0};
	int i=0;

	SENSOR_TRACE("hello:SP2528_Initialize_from_T_Flash(start)\n");

	curr_ptr = data_buff;
	while (curr_ptr < (data_buff + file_size))
	{
		while ((*curr_ptr == ' ') || (*curr_ptr == '\t'))/* Skip the Space & TAB */
			curr_ptr++;

		if (((*curr_ptr) == '/') && ((*(curr_ptr + 1)) == '*'))
		{
			while (!(((*curr_ptr) == '*') && ((*(curr_ptr + 1)) == '/')))
			{
				curr_ptr++;		/* Skip block comment code. */
			}

			while (!((*curr_ptr == 0x0D) && (*(curr_ptr+1) == 0x0A)))
			{
				curr_ptr++;
			}

			curr_ptr += 2;						/* Skip the enter line */

			continue ;
		}

		if (((*curr_ptr) == '/') || ((*curr_ptr) == '{') || ((*curr_ptr) == '}'))		/* Comment line, skip it. */
		{
			while (!((*curr_ptr == 0x0D) && (*(curr_ptr+1) == 0x0A)))
			{
				curr_ptr++;
			}

			curr_ptr += 2;						/* Skip the enter line */

			continue ;
		}
		/* This just content one enter line. */
		if (((*curr_ptr) == 0x0D) && ((*(curr_ptr + 1)) == 0x0A))
		{
			curr_ptr += 2;
			continue ;
		}
		//printk(" curr_ptr1 = %s\n",curr_ptr);
		memcpy(func_ind, curr_ptr, 3);
	        func_ind[3] = '\0';

		if (strcmp((const char *)func_ind, "REG") == 0)		/* REG */
		{
			curr_ptr += 6;				/* Skip "REG(0x" or "DLY(" */
			SP2528_Init_Reg[i].op_code = SP2528_OP_CODE_REG;

			SP2528_Init_Reg[i].init_reg = sp_strtol((const char *)curr_ptr, 16);
			curr_ptr += 5;	/* Skip "00, 0x" */

			SP2528_Init_Reg[i].init_val = sp_strtol((const char *)curr_ptr, 16);
			curr_ptr += 4;	/* Skip "00);" */

		}
		else									/* DLY */
		{
			#if 0 //lj_test
			/* Need add delay for this setting. */
			curr_ptr += 4;
			SP2529_Init_Reg[i].op_code = SP2529_OP_CODE_DLY;

			SP2529_Init_Reg[i].init_reg = 0xFF;
			SP2529_Init_Reg[i].init_val = sp2529_strtol((const char *)curr_ptr,  10);	/* Get the delay ticks, the delay should less then 50 */
			#endif
		}
		i++;
		num_sd =i;
#if 0 //lj_test
		/* Skip to next line directly. */
		while (!((*curr_ptr == 0x0D) && (*(curr_ptr+1) == 0x0A)))
		{
			curr_ptr++;
		}
		curr_ptr += 2;
#endif
	}

	SENSOR_TRACE("hello:SP2528_Initialize_from_T_Flash(end)\n");

return 1;

}
#endif


  void    _sp5408_sdcard_reg(void)
{

	int i;

	SENSOR_TRACE("hello:_sp5408_sdcard_reg(start)\n");

	for(i = 0;i<num_sd;i++)
	{
		SP2529_YUV_COMMON[i].reg_addr = SP2528_Init_Reg[i].init_reg;
		SP2529_YUV_COMMON[i].reg_value=SP2528_Init_Reg[i].init_val;
		//SENSOR_TRACE("gpwreg11 %x = %x\n",SP2529_YUV_COMMON[i].reg_addr ,SP2529_YUV_COMMON[i].reg_value);
	}

	if(num_sd  != 0)
	{
		for(i =num_sd;i<500;i++)
		{
			SP2529_YUV_COMMON[i].reg_addr = 0xfd;
			SP2529_YUV_COMMON[i].reg_value= 0x00;
			//SENSOR_TRACE("gpwreg %x = %x\n",SP2529_YUV_COMMON[i].reg_addr ,SP2529_YUV_COMMON[i].reg_value);
		}
	}

	SENSOR_TRACE("hello:_sp5408_sdcard_reg(end)\n");
}


 void    _sp5408_sdcard(void)
{

	 int i;
	 int cnt;

	SENSOR_TRACE("hello:_sp5408_sdcard(start)\n");

	memset(data_buff,0,sizeof(data_buff));//before use the buffer,clean
	fp = fopen("/system/lib/sp2529_sd", "r");

	if(NULL == fp){
		SENSOR_TRACE("open file error\n");
		//_sp5408_no_sdcard();//if no sdcard ,of open file error,use the origianl para
	}
	else
	{

	fseek(fp, 0, SEEK_END);

	file_size = ftell(fp);

	rewind(fp);

	cnt = (int)fread(data_buff, 1, file_size , fp);

	SENSOR_TRACE("open file ok %d\n" ,file_size);
	fclose(fp);

	SP2528_Initialize_from_T_Flash();//Analysis parameters

	_sp5408_sdcard_reg();//copy para and fill para

	}

	SENSOR_TRACE("hello:_sp5408_sdcard(end)\n");
}

#endif

static SENSOR_REG_T SP2529_YUV_320x240[]=
{
//scale 320*240
{0xfd,0x02},
{0x40,0x00},
{0x41,0xa0},
{0x42,0x00},
{0x43,0xa0},
{0x44,0x00},
{0x45,0xf0},
{0x46,0x01},
{0x47,0x40},
{0x0f,0x01},
{0x8f,0x02},

{0xfd,0x00},
{0x95,0x01},
{0x94,0x40},
{0x97,0x00},
{0x96,0xf0},
 {0xFF , 0xFF},
};

static SENSOR_REG_T SP2529_YUV_800x600[]=
{
#if 0
//scale 800*600
{0xfd,0x02},
{0x40,0x00},
{0x41,0x40},
{0x42,0x00},
{0x43,0x40},
{0x44,0x02},
{0x45,0x58},
{0x46,0x03},
{0x47,0x20},
{0x0f,0x01},
{0x8f,0x02},

{0xfd,0x00},
{0x95,0x03},
{0x94,0x20},
{0x97,0x02},
{0x96,0x58},
#endif
//MIPI
	{0xfd,0x00},
	{0x19,0x03},
	{0x31,0x04},
	{0x33,0x01},

	{0xfd,0x00},
	{0x95,0x03},
	{0x94,0x20},
	{0x97,0x02},
	{0x96,0x58},
       {0xFF,0xFF},
};
static SENSOR_REG_T SP2529_YUV_640x480[]=
{

  {0xFF , 0xFF},
};

static SENSOR_REG_T SP2529_YUV_1280x960[]=
{

    {0xFF , 0xFF},
};

static SENSOR_REG_T SP2529_YUV_1600x1200[]=
{
	{0xfd,0x00},
	{0x19,0x00},
	{0x31,0x00},
	{0x33,0x00},

	{0xfd,0x00},
	{0x95,0x06},
	{0x94,0x40},
	{0x97,0x04},
	{0x96,0xb0},
       {0xFF,0xFF},
};

LOCAL const SENSOR_REG_T s_SP2529_640x480_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

LOCAL const SENSOR_REG_T s_SP2529_800x600_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

LOCAL const SENSOR_REG_T s_sp2529_1600x1200_video_tab[SENSOR_VIDEO_MODE_MAX][1] = {
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

static SENSOR_REG_TAB_INFO_T s_SP2529_resolution_Tab_YUV[]=
{
	// COMMON INIT
	{ADDR_AND_LEN_OF_ARRAY(SP2529_YUV_COMMON), 1600, 1200, 24, SENSOR_IMAGE_FORMAT_YUV422},
	{ADDR_AND_LEN_OF_ARRAY(SP2529_YUV_1600x1200), 1600, 1200, 24, SENSOR_IMAGE_FORMAT_YUV422},
	//{ADDR_AND_LEN_OF_ARRAY(SP2529_YUV_800x600), 800, 600, 24, SENSOR_IMAGE_FORMAT_YUV422},	// YUV422 PREVIEW 1
	{ADDR_AND_LEN_OF_ARRAY(SP2529_YUV_1600x1200), 1600, 1200, 24, SENSOR_IMAGE_FORMAT_YUV422},
	{PNULL, 0, 0, 0, 0, 0},
	//{ADDR_AND_LEN_OF_ARRAY(SP2529_YUV_320x240), 320, 240, 24, SENSOR_IMAGE_FORMAT_YUV422},
	//{ADDR_AND_LEN_OF_ARRAY(SP2529_YUV_800x600), 800, 600, 24, SENSOR_IMAGE_FORMAT_YUV422},
	//{ADDR_AND_LEN_OF_ARRAY(SP2529_YUV_800x600), 800, 600, 24, SENSOR_IMAGE_FORMAT_YUV422},	// YUV422 PREVIEW 1
	//{ADDR_AND_LEN_OF_ARRAY(SP2529_YUV_800x600), 800, 600, 24, SENSOR_IMAGE_FORMAT_YUV422},	// YUV422 PREVIEW 1
	//{ADDR_AND_LEN_OF_ARRAY(SP2529_YUV_1280x960), 1280, 960, 24, SENSOR_IMAGE_FORMAT_YUV422},

	{PNULL, 0, 0, 0, 0, 0},

	// YUV422 PREVIEW 2
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0},
	{PNULL, 0, 0, 0, 0, 0}
};

LOCAL SENSOR_TRIM_T s_SP2529_Resolution_Trim_Tab[] = {
	{0, 0, 1600, 1200, 0, 0, 0, {0, 0, 1600, 1200}},
	{0, 0, 1600, 1200, 219, 240, 1208, {0, 0, 1600, 1200}}, //{0, 0, 800, 600, 68, 500, 608, {0, 0, 800, 600}},
	{0, 0, 1600, 1200, 219, 240, 1208, {0, 0, 1600, 1200}},  //第5个是Linetime，第6个Mipi_pclk,第7个frame line
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}},
	{0, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0}}
};
LOCAL SENSOR_VIDEO_INFO_T s_SP2529_video_info[] = {
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {30, 30, 219, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}}, (SENSOR_REG_T**)s_sp2529_1600x1200_video_tab},
	{{{0, 0, 0, 0}, {15, 30, 219, 100}, {0, 0, 0, 0}, {0, 0, 0, 0}}, (SENSOR_REG_T**)s_sp2529_1600x1200_video_tab},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL},
	{{{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}}, PNULL}
};

static SENSOR_IOCTL_FUNC_TAB_T s_SP2529_ioctl_func_tab =
{
#ifdef SP2529_TCARD_TEST
	// Internal
	PNULL,
	SP2529_PowerOn,
	PNULL,
	SP2529_Identify,

	PNULL,// write register
	PNULL,// read  register
	PNULL, //set_sensor_flip
	PNULL,//_SP2529_GetResolutionTrimTab,//_SP2529_GetResolutionTrimTab, //PNULL,

	// External
	PNULL,//set_SP2529_ae_enable,
	PNULL,
	PNULL,

	PNULL,//set_brightness,
	PNULL,//set_contrast,
	PNULL,//set_sharpness,//set_sharpness,
	PNULL,//set_saturation,//set_saturation,

	PNULL,//set_preview_mode,//set_preview_mode,
	PNULL,//set_image_effect,

	PNULL,//SP2529_BeforeSnapshot,//
	PNULL,//SP2529_After_Snapshot,

	PNULL,

	PNULL,//read_ev_value,
	PNULL,//write_ev_value,
	PNULL,//read_gain_value,
	PNULL,//write_gain_value,
	PNULL,//read_gain_scale,
	PNULL,//set_frame_rate,
	PNULL,
	PNULL,
	PNULL,//set_SP2529_awb,
	PNULL,
	PNULL,
	PNULL,//set_SP2529_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,//
       PNULL,//set_SP2529_anti_flicker,
	PNULL,//set_SP2529_video_mode,
	PNULL,
	PNULL,
	PNULL,
	SP2529_StreamOn,
	SP2529_StreamOff,
	PNULL,
	#else
	// Internal
	PNULL,
	SP2529_PowerOn,
	PNULL,
	SP2529_Identify,

	PNULL,// write register
	PNULL,// read  register
	PNULL, //set_sensor_flip
	_SP2529_GetResolutionTrimTab, //PNULL,

	// External
	PNULL,//set_SP2529_ae_enable,
	PNULL,
	PNULL,

	set_brightness,
	set_contrast,
	set_sharpness,//set_sharpness,
	set_saturation,//set_saturation,

	set_preview_mode,//set_preview_mode,
	set_image_effect,

	PNULL,//SP2529_BeforeSnapshot,//
	PNULL,//SP2529_After_Snapshot,

	PNULL,

	PNULL,//read_ev_value,
	PNULL,//write_ev_value,
	PNULL,//read_gain_value,
	PNULL,//write_gain_value,
	PNULL,//read_gain_scale,
	PNULL,//set_frame_rate,
	PNULL,
	PNULL,
	set_SP2529_awb,
	PNULL,
	PNULL,
	set_SP2529_ev,
	PNULL,
	PNULL,
	PNULL,
	PNULL,
	PNULL,//
       set_SP2529_anti_flicker,
	set_SP2529_video_mode,
	PNULL,
	PNULL,
	PNULL,
	SP2529_StreamOn,
	SP2529_StreamOff,
	PNULL,
	#endif
};

SENSOR_INFO_T g_SP2529_MIPI_yuv_info =
{
	SP2529_I2C_ADDR_W,				//salve i2c write address
	SP2529_I2C_ADDR_R,				//salve i2c read address

	SENSOR_I2C_FREQ_200,						//bit0: 0: i2c register value is 8 bit, 1: i2c register value is 16 bit
							//bit2: 0: i2c register addr  is 8 bit, 1: i2c register addr  is 16 bit
							//other bit: reseved
	SENSOR_HW_SIGNAL_PCLK_N|\
	SENSOR_HW_SIGNAL_VSYNC_N|\
	SENSOR_HW_SIGNAL_HSYNC_P,			//bit0: 0:negative; 1:positive -> polarily of pixel clock
							//bit2: 0:negative; 1:positive -> polarily of horizontal synchronization signal
							//bit4: 0:negative; 1:positive -> polarily of vertical synchronization signal
							//other bit: reseved

	// preview mode
	SENSOR_ENVIROMENT_NORMAL|\
	SENSOR_ENVIROMENT_NIGHT|\
	SENSOR_ENVIROMENT_SUNNY,

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

	7,						//bit[0:7]: count of step in brightness, contrast, sharpness, saturation
							//bit[8:31] reseved

	SENSOR_LOW_PULSE_RESET,				//reset pulse level
	20,						//reset pulse width(ms)

	SENSOR_HIGH_LEVEL_PWDN,				//power donw pulse level

	2,						//count of identify code
	{{0x02, 0x25},					//supply two code to identify sensor.
	{0xa0, 0x29}},					//for Example: index = 0-> Device id, index = 1 -> version id

	SENSOR_AVDD_2800MV,				//voltage of avdd

	1600,						//max width of source image
	1200,						//max height of source image
	"SP2529",					//name of sensor

	SENSOR_IMAGE_FORMAT_YUV422,			//define in SENSOR_IMAGE_FORMAT_E enum,
							//if set to SENSOR_IMAGE_FORMAT_MAX here, image format depent on SENSOR_REG_TAB_INFO_T
	SENSOR_IMAGE_PATTERN_YUV422_YUYV,		//pattern of input image form sensor;

	s_SP2529_resolution_Tab_YUV,			//point to resolution table information structure
	&s_SP2529_ioctl_func_tab,				//point to ioctl function table

	PNULL,						//information and table about Rawrgb sensor
	PNULL,						//extend information about sensor
	SENSOR_AVDD_1800MV,				//iovdd
	SENSOR_AVDD_1800MV,				//dvdd
	3,						//skip frame num before preview
	2,						//skip frame num before capture
	0,						//deci frame num during preview
	0,						//deci frame num during video preview
	0,						//threshold enable(only analog TV)
	0,						//atv output mode 0 fix mode 1 auto mode
	0,						//atv output start postion
	0,						//atv output end postion
	0,
	{SENSOR_INTERFACE_TYPE_CSI2, 1, 8, 1},
	s_SP2529_video_info,
	4,						//skip frame num while change setting
};

static void SP2529_WriteReg( uint8_t  subaddr, uint8_t data )
{
	Sensor_WriteReg_8bits(subaddr, data);
}

static uint8_t SP2529_ReadReg( uint8_t subaddr)
{
	uint8_t value = 0;

	value = Sensor_ReadReg( subaddr);

	return value;
}

LOCAL unsigned long _SP2529_GetResolutionTrimTab(unsigned long param)
{
	return (unsigned long) s_SP2529_Resolution_Trim_Tab;
}
static unsigned long SP2529_PowerOn(unsigned long power_on)
{
	SENSOR_AVDD_VAL_E dvdd_val = g_SP2529_MIPI_yuv_info          .dvdd_val;
	SENSOR_AVDD_VAL_E avdd_val = g_SP2529_MIPI_yuv_info          .avdd_val;
	SENSOR_AVDD_VAL_E iovdd_val = g_SP2529_MIPI_yuv_info          .iovdd_val;
	BOOLEAN power_down = g_SP2529_MIPI_yuv_info          .power_down_level;
	BOOLEAN reset_level = g_SP2529_MIPI_yuv_info          .reset_pulse_level;

	if (SENSOR_TRUE == power_on) {
		//Sensor_PowerDown(!power_down);
		//usleep(10*1000);
		//Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		//usleep(10*1000);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED);
		Sensor_PowerDown(power_down);
		Sensor_SetVoltage(dvdd_val, SENSOR_AVDD_2800MV,SENSOR_AVDD_1200MV );
		usleep(10*1000);
		Sensor_SetVoltage(dvdd_val, SENSOR_AVDD_2800MV,SENSOR_AVDD_1500MV );
		usleep(10*1000);
		Sensor_SetVoltage(dvdd_val, SENSOR_AVDD_2800MV,SENSOR_AVDD_1800MV );
		usleep(10*1000);
		Sensor_SetMCLK(SENSOR_DEFALUT_MCLK);
		usleep(10*1000);
		Sensor_PowerDown(!power_down);
		usleep(10*1000);
		Sensor_PowerDown(power_down);
		usleep(10*1000);
		Sensor_PowerDown(!power_down);
		usleep(10*1000);
		Sensor_Reset(reset_level);

	} else {
		Sensor_PowerDown(power_down);
		usleep(5*1000);
		Sensor_SetResetLevel(reset_level);
		usleep(5*1000);
		Sensor_SetVoltage(SENSOR_AVDD_CLOSED, SENSOR_AVDD_CLOSED,
				SENSOR_AVDD_CLOSED);
		usleep(5*1000);
		Sensor_SetMCLK(SENSOR_DISABLE_MCLK);
	}
	SENSOR_PRINT("(1:on, 0:off): %ld", power_on);
	return (uint32_t)SENSOR_SUCCESS;
}

static unsigned long SP2529_Identify(unsigned long param)
{
#define SP2529_PID_ADDR1 0x02
#define SP2529_PID_ADDR2 0xa0
#define SP2529_SENSOR_ID 0x2529

	uint16_t sensor_id = 0;
	uint8_t pid_value = 0;
	uint8_t ver_value = 0;
	int i;
	BOOLEAN ret_value = SENSOR_FAIL;

	for (i=0; i<3; i++) {
		SP2529_WriteReg(0xfd,0x00);
		sensor_id = Sensor_ReadReg(SP2529_PID_ADDR1) << 8;

		sensor_id |= Sensor_ReadReg(SP2529_PID_ADDR2);
		printf("li_%s sensor_id is %x\n", __func__, sensor_id);
		usleep(10*1000);
		//CMR_LOGV("liyj_CMR_LOGV\n");
		//CMR_LOGE("liyj_CMR_LOGE\n");
		SENSOR_PRINT("%s sensor_id is %x\n", __func__, sensor_id);

		if (sensor_id == SP2529_SENSOR_ID) {
			SENSOR_PRINT("the main sensor is SP2529\n");
			ret_value = SENSOR_SUCCESS;
			break;
		}
	}
#ifdef SP2529_TCARD_TEST
	_sp5408_sdcard();
#endif

	return ret_value;
}

static unsigned long set_SP2529_ae_enable(unsigned long enable)
{
#if 0
	uint32_t temp_AE_reg =0;

	if (enable == 1) {
		temp_AE_reg = Sensor_ReadReg(0xb6);
		Sensor_WriteReg(0xb6, temp_AE_reg| 0x01);
	} else {
		temp_AE_reg = Sensor_ReadReg(0xb6);
		Sensor_WriteReg(0xb6, temp_AE_reg&~0x01);
	}
#endif
	return 0;
}


static void SP2529_set_shutter()
{
#if 0
	uint32_t shutter = 0 ;

	Sensor_WriteReg(0xfe,0x00);
	Sensor_WriteReg(0xb6,0x00);
	shutter = (Sensor_ReadReg(0x03)<<8 )|( Sensor_ReadReg(0x04));

	shutter = shutter / 2 ;

	if  (shutter < 1)
		shutter = 1;

	Sensor_WriteReg(0x03, (shutter >> 8)&0xff);
	Sensor_WriteReg(0x04, shutter&0xff);
#endif
}

static SENSOR_REG_T SP2529_brightness_tab[][4]=
{
	{{0xfd , 0x01},{0xdb , 0xd0},{0xff , 0xff}},
	{{0xfd , 0x01},{0xdb , 0xe0},{0xff , 0xff}},
	{{0xfd , 0x01},{0xdb , 0xf0},{0xff , 0xff}},
	{{0xfd , 0x01},{0xdb , 0x00},{0xff , 0xff}},
	{{0xfd , 0x01},{0xdb , 0x10},{0xff , 0xff}},
	{{0xfd , 0x01},{0xdb , 0x20},{0xff , 0xff}},
	{{0xfd , 0x01},{0xdb , 0x30},{0xff , 0xff}}
};

static unsigned long set_brightness(unsigned long level)
{
	uint16_t i;
	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)SP2529_brightness_tab[level];

	if (level > 6)
		return 0;

	for (i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) && (0xFF != sensor_reg_ptr[i].reg_value); i++)
		SP2529_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);

	return 0;
}


static SENSOR_REG_T SP2529_ev_tab[][4]=
{
	{
        {0xfd,0x01},
        {0xeb,SP2529_P1_0xeb-0x18},//level -3
        {0xec,SP2529_P1_0xec-0x18},
        {0xff,0xff},
    },
    {
        {0xfd,0x01},
        {0xeb,SP2529_P1_0xeb-0x10},
        {0xec,SP2529_P1_0xec-0x10},
        {0xff,0xff},
    },
    {
        {0xfd,0x01},
        {0xeb,SP2529_P1_0xeb-0x08},
        {0xec,SP2529_P1_0xec-0x08},
        {0xff,0xff},
    },
    {
        {0xfd,0x01},
        {0xeb,SP2529_P1_0xeb},
        {0xec,SP2529_P1_0xec},
        {0xff,0xff},
    },
    {
        {0xfd,0x01},
        {0xeb,SP2529_P1_0xeb+0x08},
        {0xec,SP2529_P1_0xec+0x08},
        {0xff,0xff},
    },
    {
        {0xfd,0x01},
        {0xeb,SP2529_P1_0xeb+0x10},
        {0xec,SP2529_P1_0xec+0x10},
        {0xff,0xff},
    },
    {
        {0xfd,0x01},
        {0xeb,SP2529_P1_0xeb+0x18}, // level +3
        {0xec,SP2529_P1_0xec+0x18},
        {0xff,0xff},
    }
};

static unsigned long set_SP2529_ev(unsigned long level)
{
	uint16_t i;
	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)SP2529_ev_tab[level];

	if (level > 6)
		return 0;

	for (i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) ||(0xFF != sensor_reg_ptr[i].reg_value) ; i++)
		SP2529_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);

	return 0;
}

static unsigned long set_SP2529_anti_flicker(unsigned long mode )
{
	if(video_mode == SCI_TRUE)
		return 0;
	switch (mode) {
	case DCAMERA_FLICKER_50HZ:
		Antiflicker = DCAMERA_FLICKER_50HZ;
			// 2.5pll 9-11fps 50hz uxga
			SP2529_WriteReg(0xfd,0x00);
			SP2529_WriteReg(0x03,0x03);
			SP2529_WriteReg(0x04,0x30);
			SP2529_WriteReg(0x05,0x00);
			SP2529_WriteReg(0x06,0x00);
			SP2529_WriteReg(0x07,0x00);
			SP2529_WriteReg(0x08,0x00);
			SP2529_WriteReg(0x09,0x00);
			SP2529_WriteReg(0x0a,0xb5);
			SP2529_WriteReg(0xfd,0x01);
			SP2529_WriteReg(0xf0,0x00);
			SP2529_WriteReg(0xf7,0x88);
			SP2529_WriteReg(0xf8,0x71);
			SP2529_WriteReg(0x02,0x0b);
			SP2529_WriteReg(0x03,0x01);
			SP2529_WriteReg(0x06,0x88);
			SP2529_WriteReg(0x07,0x00);
			SP2529_WriteReg(0x08,0x01);
			SP2529_WriteReg(0x09,0x00);
			SP2529_WriteReg(0xfd,0x02);
			SP2529_WriteReg(0x3d,0x0d);
			SP2529_WriteReg(0x3e,0x71);
			SP2529_WriteReg(0x3f,0x00);
			SP2529_WriteReg(0x88,0xc3);
			SP2529_WriteReg(0x89,0x87);
			SP2529_WriteReg(0x8a,0x43);
			SP2529_WriteReg(0xfd,0x02);
			SP2529_WriteReg(0xbe,0xd8);
			SP2529_WriteReg(0xbf,0x05);
			SP2529_WriteReg(0xd0,0xd8);
			SP2529_WriteReg(0xd1,0x05);
			SP2529_WriteReg(0xc9,0xd8);
			SP2529_WriteReg(0xca,0x05);
		break;
	case DCAMERA_FLICKER_60HZ:
		Antiflicker = DCAMERA_FLICKER_60HZ;
		     // 2.5pll 9-11fps 60hz uxga
			SP2529_WriteReg(0xfd,0x00);
			SP2529_WriteReg(0x03,0x02);
			SP2529_WriteReg(0x04,0xac);
			SP2529_WriteReg(0x05,0x00);
			SP2529_WriteReg(0x06,0x00);
			SP2529_WriteReg(0x07,0x00);
			SP2529_WriteReg(0x08,0x00);
			SP2529_WriteReg(0x09,0x00);
			SP2529_WriteReg(0x0a,0xae);
			SP2529_WriteReg(0xfd,0x01);
			SP2529_WriteReg(0xf0,0x00);
			SP2529_WriteReg(0xf7,0x72);
			SP2529_WriteReg(0xf8,0x72);
			SP2529_WriteReg(0x02,0x0d);
			SP2529_WriteReg(0x03,0x01);
			SP2529_WriteReg(0x06,0x72);
			SP2529_WriteReg(0x07,0x00);
			SP2529_WriteReg(0x08,0x01);
			SP2529_WriteReg(0x09,0x00);
			SP2529_WriteReg(0xfd,0x02);
			SP2529_WriteReg(0x3d,0x0d);
			SP2529_WriteReg(0x3e,0x72);
			SP2529_WriteReg(0x3f,0x00);
			SP2529_WriteReg(0x88,0x7d);
			SP2529_WriteReg(0x89,0x7d);
			SP2529_WriteReg(0x8a,0x44);
			SP2529_WriteReg(0xfd,0x02);
			SP2529_WriteReg(0xbe,0xca);
			SP2529_WriteReg(0xbf,0x05);
			SP2529_WriteReg(0xd0,0xca);
			SP2529_WriteReg(0xd1,0x05);
			SP2529_WriteReg(0xc9,0xca);
			SP2529_WriteReg(0xca,0x05);
		break;
	default:
		break;

	}

	return 0;
}

static unsigned long set_SP2529_video_mode(unsigned long mode)
{

      if(mode == 0)
      video_mode = SCI_FALSE;
      else
      video_mode = SCI_TRUE;

	if(0 == mode)
	{
	       set_preview_mode(SP2529_PRE_MODE);
		CMR_LOGV("SP2529 SENSOR: set_video_mode000000");
	}
  	else
	{
		// 24M 2.5pll  fix 11fps 50hz
		SP2529_WriteReg(0xfd,0x00);
		SP2529_WriteReg(0x03,0x03);
		SP2529_WriteReg(0x04,0x30);
		SP2529_WriteReg(0x05,0x00);
		SP2529_WriteReg(0x06,0x00);
		SP2529_WriteReg(0x07,0x00);
		SP2529_WriteReg(0x08,0x00);
		SP2529_WriteReg(0x09,0x00);
		SP2529_WriteReg(0x0a,0xb5);
		SP2529_WriteReg(0xfd,0x01);
		SP2529_WriteReg(0xf0,0x00);
		SP2529_WriteReg(0xf7,0x88);
		SP2529_WriteReg(0xf8,0x71);
		SP2529_WriteReg(0x02,0x09);
		SP2529_WriteReg(0x03,0x01);
		SP2529_WriteReg(0x06,0x88);
		SP2529_WriteReg(0x07,0x00);
		SP2529_WriteReg(0x08,0x01);
		SP2529_WriteReg(0x09,0x00);
		SP2529_WriteReg(0xfd,0x02);
		SP2529_WriteReg(0x3d,0x0a);
		SP2529_WriteReg(0x3e,0x71);
		SP2529_WriteReg(0x3f,0x00);
		SP2529_WriteReg(0x88,0xc3);
		SP2529_WriteReg(0x89,0x87);
		SP2529_WriteReg(0x8a,0x43);
		SP2529_WriteReg(0xfd,0x02);
		SP2529_WriteReg(0xbe,0xc8);
		SP2529_WriteReg(0xbf,0x04);
		SP2529_WriteReg(0xd0,0xc8);
		SP2529_WriteReg(0xd1,0x04);
		SP2529_WriteReg(0xc9,0xc8);
		SP2529_WriteReg(0xca,0x04);
		CMR_LOGV("SP2529 SENSOR: set_video_mode1");
	}
    return 0;
}

static SENSOR_REG_T SP2529_awb_tab[][7]=
{
	 //AUTO
    {
			{0xfd,0x02},
			{0x26,0xa0},
			{0x27,0x96},
			{0xfd,0x01},
			{0x32,0x15},
			{0xff, 0xff}
    },
    //OFFICE:  2800K~3000K
    {
			 {0xfd,0x01},
			 {0x32,0x05},
			 {0xfd,0x02},
			 {0x26,0x7b},
			 {0x27,0xd3},
			 {0xfd,0x00},
			 {0xff, 0xff}
    },
        //U30    3000K
    {
        {0xff, 0xff}
    },
    //CWF  4150K
    {
        {0xff, 0xff}
    },
    //HOME FLUORESCENT 4200K~5000K
    {
			 {0xfd,0x01},
			 {0x32,0x05},
			 {0xfd,0x02},
			 {0x26,0xae},
			 {0x27,0xcc},
			 {0xfd,0x00},
			 {0xff,0xff}
    },

    //SUN:  DAYLIGHT//6500K
    {
			 {0xfd,0x01},
			 {0x32,0x05},
			 {0xfd,0x02},
			 {0x26,0xc1},
			 {0x27,0x88},
			 {0xfd,0x00},
			 {0xff,0xff}
    },
    //CLOUD:  7000K
    {
			 {0xfd,0x01},
			 {0x32,0x05},
			 {0xfd,0x02},
			 {0x26,0xe2},
			 {0x27,0x65},//65 fs
			 {0xfd,0x00},
			 {0xff,0xff}
     }
};

static unsigned long set_SP2529_awb(unsigned long mode)
{
	uint16_t i;
	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)SP2529_awb_tab[mode];

	if(mode >= DCAMERA_WB_MODE_MAX)
		return 0;

	SCI_ASSERT(PNULL != sensor_reg_ptr);

	for(i = 0;(0xFF != sensor_reg_ptr[i].reg_addr) || (0xFF != sensor_reg_ptr[i].reg_value) ; i++)
	{
		Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}

	//Sensor_SetSensorExifInfo(SENSOR_EXIF_CTRL_LIGHTSOURCE, (uint32_t)mode);
	usleep(100);
	SENSOR_TRACE("SENSOR: set_awb_mode: mode = %ld", mode);
	return 0;
	/*uint8_t awb_en_value;
	uint16_t i;

	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)SP2529_awb_tab[mode];

	if (mode > 6)
		return 0;

	for (i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) || (0xFF != sensor_reg_ptr[i].reg_value); i++)
		SP2529_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);

	return 0;*/
}

static SENSOR_REG_T SP2529_contrast_tab[][10]=
{
	 { //level -3
	        {0xfd, 0x01},
	        {0x10,  SP2529_P1_0x10-0x30},
	        {0x11,  SP2529_P1_0x11-0x30},
	        {0x12,  SP2529_P1_0x12-0x30},
	        {0x13,  SP2529_P1_0x13-0x30},
	        {0x14,  SP2529_P1_0x14-0x30},
	        {0x15,  SP2529_P1_0x15-0x30},
	        {0x16,  SP2529_P1_0x16-0x30},
	        {0x17,  SP2529_P1_0x17-0x30},
	        {0xff, 0xff}
	    },
	    { //level -2
	        {0xfd, 0x01},
	        {0x10,  SP2529_P1_0x10-0x20},
	        {0x11,  SP2529_P1_0x11-0x20},
	        {0x12,  SP2529_P1_0x12-0x20},
	        {0x13,  SP2529_P1_0x13-0x20},
	        {0x14,  SP2529_P1_0x14-0x20},
	        {0x15,  SP2529_P1_0x15-0x20},
	        {0x16,  SP2529_P1_0x16-0x20},
	        {0x17,  SP2529_P1_0x17-0x20},
	        {0xff, 0xff}
	    },
	    {//level -1
	        {0xfd, 0x01},
	        {0x10,  SP2529_P1_0x10-0x10},
	        {0x11,  SP2529_P1_0x11-0x10},
	        {0x12,  SP2529_P1_0x12-0x10},
	        {0x13,  SP2529_P1_0x13-0x10},
	        {0x14,  SP2529_P1_0x14-0x10},
	        {0x15,  SP2529_P1_0x15-0x10},
	        {0x16,  SP2529_P1_0x16-0x10},
	        {0x17,  SP2529_P1_0x17-0x10},
	        {0xff, 0xff}
	    },
	    {//level 0
	        {0xfd, 0x01},
	        {0x10,  SP2529_P1_0x10},
	        {0x11,  SP2529_P1_0x11},
	        {0x12,  SP2529_P1_0x12},
	        {0x13,  SP2529_P1_0x13},
	        {0x14,  SP2529_P1_0x14},
	        {0x15,  SP2529_P1_0x15},
	        {0x16,  SP2529_P1_0x16},
	        {0x17,  SP2529_P1_0x17},
	        {0xff, 0xff}
	    },
	    {//level +1
	        {0xfd, 0x01},
	        {0x10,  SP2529_P1_0x10+0x10},
	        {0x11,  SP2529_P1_0x11+0x10},
	        {0x12,  SP2529_P1_0x12+0x10},
	        {0x13,  SP2529_P1_0x13+0x10},
	        {0x14,  SP2529_P1_0x14+0x10},
	        {0x15,  SP2529_P1_0x15+0x10},
	        {0x16,  SP2529_P1_0x16+0x10},
	        {0x17,  SP2529_P1_0x17+0x10},
	        {0xff, 0xff}
	    },
	    {//level +2
	        {0xfd, 0x01},
	        {0x10,  SP2529_P1_0x10+0x20},
	        {0x11,  SP2529_P1_0x11+0x20},
	        {0x12,  SP2529_P1_0x12+0x20},
	        {0x13,  SP2529_P1_0x13+0x20},
	        {0x14,  SP2529_P1_0x14+0x20},
	        {0x15,  SP2529_P1_0x15+0x20},
	        {0x16,  SP2529_P1_0x16+0x20},
	        {0x17,  SP2529_P1_0x17+0x20},
	        {0xff, 0xff}
	    },
	    {//level +3
	        {0xfd, 0x01},
	        {0x10,  SP2529_P1_0x10+0x30},
	        {0x11,  SP2529_P1_0x11+0x30},
	        {0x12,  SP2529_P1_0x12+0x30},
	        {0x13,  SP2529_P1_0x13+0x30},
	        {0x14,  SP2529_P1_0x14+0x30},
	        {0x15,  SP2529_P1_0x15+0x30},
	        {0x16,  SP2529_P1_0x16+0x30},
	        {0x17,  SP2529_P1_0x17+0x30},
	        {0xff, 0xff}
	    }
};

static unsigned long set_contrast(unsigned long level)
{
	uint16_t i;
	SENSOR_REG_T* sensor_reg_ptr;

	sensor_reg_ptr = (SENSOR_REG_T*)SP2529_contrast_tab[level];

	if (level > 6)
		return 0;

	for (i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) && (0xFF != sensor_reg_ptr[i].reg_value); i++)
		SP2529_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);

	return 0;
}
static SENSOR_REG_T SP2529_sharpness_tab[][4]=
{
	#if 1
	//weakest
	{{0xfd, 0x02},{0xe9, SP2529_P2_0xe9-0x08},{0xed, SP2529_P2_0xed-0x08}},
	{{0xfd, 0x02},{0xe9, SP2529_P2_0xe9-0x04},{0xed, SP2529_P2_0xed-0x04}},
	{{0xfd, 0x02},{0xe9, SP2529_P2_0xe9},     {0xed, SP2529_P2_0xed}},            // 0
	{{0xfd, 0x02},{0xe9, SP2529_P2_0xe9+0x04},{0xed, SP2529_P2_0xed+0x04}},
	//strongest
	{{0xfd, 0x02},{0xe9, SP2529_P2_0xe9+0x08},{0xed, SP2529_P2_0xed+0x08}}
#endif
};
static unsigned long set_sharpness(unsigned long level)
{

#if 1
	uint16_t i;
	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)SP2529_sharpness_tab[level];

	//SCI_ASSERT(level <= 5 );
	//SCI_ASSERT(PNULL != sensor_reg_ptr);

	for(i = 0; i < 3/*(0xFF != sensor_reg_ptr[i].reg_addr) && (0xFF != sensor_reg_ptr[i].reg_value)*/ ; i++)
	{
		Sensor_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);
	}
	//SCI_TRACE_LOW("set_sharpness: level = %d", level);

#endif
	return 0;
}

static SENSOR_REG_T SP2529_saturation_tab[][10]=
{
// level 0
{
//sat u
{0xfd,0x01},
{0xd3,SP2529_P1_0xd3-0x30},
{0xd4,SP2529_P1_0xd4-0x30},
{0xd5,SP2529_P1_0xd5-0x30},
{0xd6,SP2529_P1_0xd6-0x30},
//sat v
{0xd7,SP2529_P1_0xd7-0x30},
{0xd8,SP2529_P1_0xd8-0x30},
{0xd9,SP2529_P1_0xd9-0x30},
{0xda,SP2529_P1_0xda-0x30},
{0xff,0xff},
},

// level 1
{
//sat u
{0xfd,0x01},
{0xd3,SP2529_P1_0xd3-0x20},
{0xd4,SP2529_P1_0xd4-0x20},
{0xd5,SP2529_P1_0xd5-0x20},
{0xd6,SP2529_P1_0xd6-0x20},
//sat v
{0xd7,SP2529_P1_0xd7-0x20},
{0xd8,SP2529_P1_0xd8-0x20},
{0xd9,SP2529_P1_0xd9-0x20},
{0xda,SP2529_P1_0xda-0x20},
{0xff,0xff},
},

// level 2
{
//sat u
{0xfd,0x01},
{0xd3,SP2529_P1_0xd3-0x10},
{0xd4,SP2529_P1_0xd4-0x10},
{0xd5,SP2529_P1_0xd5-0x10},
{0xd6,SP2529_P1_0xd6-0x10},
//sat v
{0xd7,SP2529_P1_0xd7-0x10},
{0xd8,SP2529_P1_0xd8-0x10},
{0xd9,SP2529_P1_0xd9-0x10},
{0xda,SP2529_P1_0xda-0x10},
{0xff,0xff},
},

// level 3
{
//sat u
{0xfd,0x01},
{0xd3,SP2529_P1_0xd3},
{0xd4,SP2529_P1_0xd4},
{0xd5,SP2529_P1_0xd5},
{0xd6,SP2529_P1_0xd6},
//sat v
{0xd7,SP2529_P1_0xd7},
{0xd8,SP2529_P1_0xd8},
{0xd9,SP2529_P1_0xd9},
{0xda,SP2529_P1_0xda},
{0xff,0xff},
},

// level 4
{
//sat u
{0xfd,0x01},
{0xd3,SP2529_P1_0xd3+0x10},
{0xd4,SP2529_P1_0xd4+0x10},
{0xd5,SP2529_P1_0xd5+0x10},
{0xd6,SP2529_P1_0xd6+0x10},
//sat v
{0xd7,SP2529_P1_0xd7+0x10},
{0xd8,SP2529_P1_0xd8+0x10},
{0xd9,SP2529_P1_0xd9+0x10},
{0xda,SP2529_P1_0xda+0x10},
{0xff,0xff},
},
// level 5
{
//sat u
{0xfd,0x01},
{0xd3,SP2529_P1_0xd3+0x20},
{0xd4,SP2529_P1_0xd4+0x20},
{0xd5,SP2529_P1_0xd5+0x20},
{0xd6,SP2529_P1_0xd6+0x20},
//sat v
{0xd7,SP2529_P1_0xd7+0x20},
{0xd8,SP2529_P1_0xd8+0x20},
{0xd9,SP2529_P1_0xd9+0x20},
{0xda,SP2529_P1_0xda+0x20},
{0xff,0xff},
},
// level 6
{
//sat u
{0xfd,0x01},
{0xd3,SP2529_P1_0xd3+0x30},
{0xd4,SP2529_P1_0xd4+0x30},
{0xd5,SP2529_P1_0xd5+0x30},
{0xd6,SP2529_P1_0xd6+0x30},
//sat v
{0xd7,SP2529_P1_0xd7+0x30},
{0xd8,SP2529_P1_0xd8+0x30},
{0xd9,SP2529_P1_0xd9+0x30},
{0xda,SP2529_P1_0xda+0x30},
{0xff,0xff},
},
};
static unsigned long set_saturation(unsigned long level)
{

#if 1
	uint16_t i;
	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)SP2529_saturation_tab[level];

	//SCI_ASSERT(level <= 8 );
	//SCI_ASSERT(PNULL != sensor_reg_ptr);
	for (i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) && (0xFF != sensor_reg_ptr[i].reg_value); i++)
		SP2529_WriteReg(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);

	//SCI_TRACE_LOW("set_saturation: level = %d", level);
#endif
	return 0;
}


static unsigned long set_preview_mode(unsigned long preview_mode)
{
	 if(video_mode == SCI_TRUE)
      return 0;
	SP2529_PRE_MODE= preview_mode;

	switch (preview_mode)
	{
		case DCAMERA_ENVIRONMENT_NORMAL:
		case DCAMERA_ENVIRONMENT_SUNNY:
		{
		if(Antiflicker== DCAMERA_FLICKER_50HZ)
		{
			// 2.5pll 9-11fps 50hz uxga
			SP2529_WriteReg(0xfd,0x00);
			SP2529_WriteReg(0x03,0x03);
			SP2529_WriteReg(0x04,0x30);
			SP2529_WriteReg(0x05,0x00);
			SP2529_WriteReg(0x06,0x00);
			SP2529_WriteReg(0x07,0x00);
			SP2529_WriteReg(0x08,0x00);
			SP2529_WriteReg(0x09,0x00);
			SP2529_WriteReg(0x0a,0xb5);
			SP2529_WriteReg(0xfd,0x01);
			SP2529_WriteReg(0xf0,0x00);
			SP2529_WriteReg(0xf7,0x88);
			SP2529_WriteReg(0xf8,0x71);
			SP2529_WriteReg(0x02,0x0b);
			SP2529_WriteReg(0x03,0x01);
			SP2529_WriteReg(0x06,0x88);
			SP2529_WriteReg(0x07,0x00);
			SP2529_WriteReg(0x08,0x01);
			SP2529_WriteReg(0x09,0x00);
			SP2529_WriteReg(0xfd,0x02);
			SP2529_WriteReg(0x3d,0x0d);
			SP2529_WriteReg(0x3e,0x71);
			SP2529_WriteReg(0x3f,0x00);
			SP2529_WriteReg(0x88,0xc3);
			SP2529_WriteReg(0x89,0x87);
			SP2529_WriteReg(0x8a,0x43);
			SP2529_WriteReg(0xfd,0x02);
			SP2529_WriteReg(0xbe,0xd8);
			SP2529_WriteReg(0xbf,0x05);
			SP2529_WriteReg(0xd0,0xd8);
			SP2529_WriteReg(0xd1,0x05);
			SP2529_WriteReg(0xc9,0xd8);
			SP2529_WriteReg(0xca,0x05);

		}
		else
		{
			// 2.5pll 9-11fps 60hz uxga
			SP2529_WriteReg(0xfd,0x00);
			SP2529_WriteReg(0x03,0x02);
			SP2529_WriteReg(0x04,0xac);
			SP2529_WriteReg(0x05,0x00);
			SP2529_WriteReg(0x06,0x00);
			SP2529_WriteReg(0x07,0x00);
			SP2529_WriteReg(0x08,0x00);
			SP2529_WriteReg(0x09,0x00);
			SP2529_WriteReg(0x0a,0xae);
			SP2529_WriteReg(0xfd,0x01);
			SP2529_WriteReg(0xf0,0x00);
			SP2529_WriteReg(0xf7,0x72);
			SP2529_WriteReg(0xf8,0x72);
			SP2529_WriteReg(0x02,0x0d);
			SP2529_WriteReg(0x03,0x01);
			SP2529_WriteReg(0x06,0x72);
			SP2529_WriteReg(0x07,0x00);
			SP2529_WriteReg(0x08,0x01);
			SP2529_WriteReg(0x09,0x00);
			SP2529_WriteReg(0xfd,0x02);
			SP2529_WriteReg(0x3d,0x0d);
			SP2529_WriteReg(0x3e,0x72);
			SP2529_WriteReg(0x3f,0x00);
			SP2529_WriteReg(0x88,0x7d);
			SP2529_WriteReg(0x89,0x7d);
			SP2529_WriteReg(0x8a,0x44);
			SP2529_WriteReg(0xfd,0x02);
			SP2529_WriteReg(0xbe,0xca);
			SP2529_WriteReg(0xbf,0x05);
			SP2529_WriteReg(0xd0,0xca);
			SP2529_WriteReg(0xd1,0x05);
			SP2529_WriteReg(0xc9,0xca);
			SP2529_WriteReg(0xca,0x05);
		}
		break;
		}
		case DCAMERA_ENVIRONMENT_NIGHT:
		{
		if(Antiflicker== DCAMERA_FLICKER_50HZ)
		{
			// 2.5 PLL 6-11fps 50hz uxga
			SP2529_WriteReg(0xfd,0x00);
			SP2529_WriteReg(0x03,0x03);
			SP2529_WriteReg(0x04,0x30);
			SP2529_WriteReg(0x05,0x00);
			SP2529_WriteReg(0x06,0x00);
			SP2529_WriteReg(0x07,0x00);
			SP2529_WriteReg(0x08,0x00);
			SP2529_WriteReg(0x09,0x00);
			SP2529_WriteReg(0x0a,0xb5);
			SP2529_WriteReg(0xfd,0x01);
			SP2529_WriteReg(0xf0,0x00);
			SP2529_WriteReg(0xf7,0x88);
			SP2529_WriteReg(0xf8,0x71);
			SP2529_WriteReg(0x02,0x10);
			SP2529_WriteReg(0x03,0x01);
			SP2529_WriteReg(0x06,0x88);
			SP2529_WriteReg(0x07,0x00);
			SP2529_WriteReg(0x08,0x01);
			SP2529_WriteReg(0x09,0x00);
			SP2529_WriteReg(0xfd,0x02);
			SP2529_WriteReg(0x3d,0x14);
			SP2529_WriteReg(0x3e,0x71);
			SP2529_WriteReg(0x3f,0x00);
			SP2529_WriteReg(0x88,0xc3);
			SP2529_WriteReg(0x89,0x87);
			SP2529_WriteReg(0x8a,0x43);
			SP2529_WriteReg(0xfd,0x02);
			SP2529_WriteReg(0xbe,0x80);
			SP2529_WriteReg(0xbf,0x08);
			SP2529_WriteReg(0xd0,0x80);
			SP2529_WriteReg(0xd1,0x08);
			SP2529_WriteReg(0xc9,0x80);
			SP2529_WriteReg(0xca,0x08);
		}
		else
		{
			// 2.5pll 6-11fps 60hz uxga
			SP2529_WriteReg(0xfd,0x00);
			SP2529_WriteReg(0x03,0x02);
			SP2529_WriteReg(0x04,0xac);
			SP2529_WriteReg(0x05,0x00);
			SP2529_WriteReg(0x06,0x00);
			SP2529_WriteReg(0x07,0x00);
			SP2529_WriteReg(0x08,0x00);
			SP2529_WriteReg(0x09,0x00);
			SP2529_WriteReg(0x0a,0xae);
			SP2529_WriteReg(0xfd,0x01);
			SP2529_WriteReg(0xf0,0x00);
			SP2529_WriteReg(0xf7,0x72);
			SP2529_WriteReg(0xf8,0x72);
			SP2529_WriteReg(0x02,0x14);
			SP2529_WriteReg(0x03,0x01);
			SP2529_WriteReg(0x06,0x72);
			SP2529_WriteReg(0x07,0x00);
			SP2529_WriteReg(0x08,0x01);
			SP2529_WriteReg(0x09,0x00);
			SP2529_WriteReg(0xfd,0x02);
			SP2529_WriteReg(0x3d,0x14);
			SP2529_WriteReg(0x3e,0x72);
			SP2529_WriteReg(0x3f,0x00);
			SP2529_WriteReg(0x88,0x7d);
			SP2529_WriteReg(0x89,0x7d);
			SP2529_WriteReg(0x8a,0x44);
			SP2529_WriteReg(0xfd,0x02);
			SP2529_WriteReg(0xbe,0xe8);
			SP2529_WriteReg(0xbf,0x08);
			SP2529_WriteReg(0xd0,0xe8);
			SP2529_WriteReg(0xd1,0x08);
			SP2529_WriteReg(0xc9,0xe8);
			SP2529_WriteReg(0xca,0x08);
		}
		break;
		}

		default:
		{
		break;
		}
		  //usleep(100);
		  return 0;

	}

		return 0;
}

static SENSOR_REG_T SP2529_image_effect_tab[][9] =
{
	// effect normal
	{
        		{0xfd,0x01},
			{0x66,0x00},
			{0x67,0x80},
			{0x68,0x80},
			//{0xdb,0x00},
			{0x34,0xc7},
			{0xfd,0x02},
			{0x14,0x00},
			{0xff,0xff}
    },
	//effect BLACKWHITE
	{
			{0xfd,0x01},
			{0x66,0x20},
			{0x67,0x80},
			{0x68,0x80},
			//{0xdb,0x00},
			{0x34,0xc7},
			{0xfd,0x02},
			{0x14,0x00},
	    {0xff,0xff}
    },
        // effect RED
    {
      {0xff,0xff}
    },
    // effect GREEN
    {
			{0xff,0xff}
    },
    // effect  BLUE
    {
      {0xfd,0x01},
			{0x66,0x10},
			{0x67,0x80},
			{0x68,0xb0},
			//{0xdb,0x00},
			{0x34,0xc7},
			{0xfd,0x02},
			{0x14,0x00},
			{0xff,0xff}
    },
    //effect YELLOW//huaijiu
    {
			{0xfd,0x01},
			{0x66,0x10},
			{0x67,0x90},
			{0x68,0x68},
			//{0xdb,0x00},
			{0x34,0xc7},
			{0xfd,0x02},
			{0x14,0x00},
      {0xff,0xff}
    },
	// effect NEGATIVE
	{
      {0xfd,0x01},
			{0x66,0x04},
			{0x67,0x80},
			{0x68,0x80},
			//{0xdb,0x00},
			{0x34,0xc7},
			{0xfd,0x02},
			{0x14,0x00},
			{0xff,0xff}
    },
	// effect sepia
	{
      {0xfd,0x01},
			{0x66,0x10},
			{0x67,0x90},
			{0x68,0x58},
			//{0xdb,0x00},
			{0x34,0xc7},
			{0xfd,0x02},
			{0x14,0x00},
			{0xff,0xff}
    }
};

static unsigned long set_image_effect(unsigned long effect_type)
{
	uint16_t i;

	SENSOR_REG_T* sensor_reg_ptr = (SENSOR_REG_T*)SP2529_image_effect_tab[effect_type];
	CMR_LOGE("li SP2529_After_Snapshot effect_type = %x \n",effect_type);//liyj
	if (effect_type > 7)
		return 0;

	for (i = 0; (0xFF != sensor_reg_ptr[i].reg_addr) || (0xFF != sensor_reg_ptr[i].reg_value) ; i++)
		Sensor_WriteReg_8bits(sensor_reg_ptr[i].reg_addr, sensor_reg_ptr[i].reg_value);

	return 0;
}

static void sp2529_Set_Shutter(uint16_t iShutter)
{
	uint8_t temp_reg_L, temp_reg_H;
	temp_reg_L = iShutter & 0xff;
	temp_reg_H = (iShutter >>8) & 0xff;
	SP2529_WriteReg(0xfd,0x00);
	SP2529_WriteReg(0x03,temp_reg_H);
	SP2529_WriteReg(0x04,temp_reg_L);

} /* Set_sp2529_Shutter */

static uint16_t sp2529_Read_Shutter(void)
{
	uint8_t temp_reg1, temp_reg2;
	uint16_t shutter;
           SP2529_WriteReg(0xfd,0x00);
	temp_reg1 = SP2529_ReadReg(0x04);
	temp_reg2 = SP2529_ReadReg(0x03);

	shutter = (temp_reg1 & 0xFF) | (temp_reg2 << 8);

	return shutter;
} /* sp2529_read_shutter */

static void SP2529_AfterCapture(void)
{
       uint8_t  	AE_tmp;
	#if 1
	uint8_t   tmp1,tmp2,tmp3;
	#endif

	SP2529_WriteReg(0xfd,0x01);
	AE_tmp=SP2529_ReadReg(0x32);
	AE_tmp=AE_tmp&0xfa;

	SP2529_WriteReg(0x32,AE_tmp);
	SP2529_WriteReg(0xfd,0x00);
	SP2529_WriteReg(0xe7,0x03);
	SP2529_WriteReg(0xe7,0x00);

	sp2529_Set_Shutter(G_shutter);

	SP2529_WriteReg(0xfd,0x00);
	SP2529_WriteReg(0x24,G_Gain);

	SP2529_WriteReg(0xfd,0x00);
	SP2529_WriteReg(0xe7,0x03);
	SP2529_WriteReg(0xe7,0x00);

	SP2529_WriteReg(0xfd,0x01);
	AE_tmp=SP2529_ReadReg(0x32);
	AE_tmp=AE_tmp|0x05;
	SP2529_WriteReg(0x32,AE_tmp);
	SP2529_WriteReg(0xfd,0x00);
	SP2529_WriteReg(0xe7,0x03);
	SP2529_WriteReg(0xe7,0x00);

	//usleep(350*1000);
//	return;
}


static void SP2529_BeforeCapture(void)
{
	uint32_t  Shutter_temp;
	uint16_t  Shutter, HB;
	uint8_t  	HB_L,HB_H,Gain,AE_tmp;

	SP2529_WriteReg(0xfd,0x01);
	AE_tmp=SP2529_ReadReg(0x32);
	SP2529_WriteReg(0xfd,0x00);
	Gain=SP2529_ReadReg(0x23);

	HB_H=SP2529_ReadReg(0x09);
	HB_L=SP2529_ReadReg(0x0a);

	G_Gain=Gain;
	HB = (HB_L & 0xFF) | (HB_H << 8);
	AE_tmp=AE_tmp&0xfa;
	Shutter = sp2529_Read_Shutter();
	G_shutter = Shutter;

	Shutter_temp = Shutter;
	Shutter_temp = Shutter_temp*(517+HB)/2/(922+HB);
	Shutter = Shutter_temp&0xffff;

      // Shutter = Shutter*24/60;

	if(Shutter<1)
	{
		Shutter=1;
	}
	SP2529_WriteReg(0xfd,0x01);
	SP2529_WriteReg(0x32,AE_tmp);
	SP2529_WriteReg(0xfd,0x00);
	SP2529_WriteReg(0xe7,0x03);
	SP2529_WriteReg(0xe7,0x00);

	sp2529_Set_Shutter(Shutter);
	SP2529_WriteReg(0xfd,0x00);
	SP2529_WriteReg(0x24,Gain);

	SP2529_WriteReg(0xfd,0x00);
	SP2529_WriteReg(0xe7,0x03);
	SP2529_WriteReg(0xe7,0x00);
       //usleep(300*1000);
//return 0;
}

static unsigned long SP2529_After_Snapshot(unsigned long param)
{

	SENSOR_PRINT("SP2529_After_Snapshot param = %lx \n",param);

	Sensor_SetMode((uint32_t)param);

       if( SP2529_Cap_Flag==1)// hill 0626
       {
           SP2529_Cap_Flag=0;
	    return 0;
	 }
       SP2529_AfterCapture();
	return 0;
}

static unsigned long SP2529_BeforeSnapshot(unsigned long sensor_snapshot_mode)
{
#if 1
	sensor_snapshot_mode &= 0xffff;
	Sensor_SetMode((uint32_t)sensor_snapshot_mode);
	Sensor_SetMode_WaitDone();

	switch (sensor_snapshot_mode) {
	case SENSOR_MODE_PREVIEW_ONE:
		SENSOR_PRINT("Capture VGA Size");
	       SP2529_Cap_Flag=1;
		return 0; // hill 0626
		break;
	case SENSOR_MODE_SNAPSHOT_ONE_FIRST:
	case SENSOR_MODE_SNAPSHOT_ONE_SECOND:
		SENSOR_PRINT("Capture 1.3M&2M Size");
		break;
	default:
		break;
	}

	SENSOR_PRINT("SENSOR_SP2529: Before Snapshot");
#endif

       SP2529_BeforeCapture();
	return 0;

}

static unsigned long SP2529_StreamOn(unsigned long param)
{
	SENSOR_PRINT("Start");

	Sensor_WriteReg(0xfd , 0x00);
	Sensor_WriteReg(0x92 , 0x81);
	Sensor_WriteReg(0xfd , 0x01);
	Sensor_WriteReg(0x36 , 0x00);
       usleep(10*1000);
	return 0;
}

static unsigned long SP2529_StreamOff(unsigned long param)
{
	SENSOR_PRINT("Stop");

	Sensor_WriteReg(0xfd , 0x00);
	Sensor_WriteReg(0x92 , 0x00);
	usleep(10*1000);

	return 0;
}


static unsigned long read_ev_value(unsigned long value)
{
	return 0;
}

static unsigned long write_ev_value(unsigned long exposure_value)
{
	return 0;
}

static unsigned long read_gain_value(unsigned long value)
{
	return 0;
}

static unsigned long write_gain_value(unsigned long gain_value)
{
	return 0;
}

static unsigned long read_gain_scale(unsigned long value)
{
	return SENSOR_GAIN_SCALE;
}


static unsigned long set_frame_rate(unsigned long param)
{
	return 0;
}
static unsigned long set_sensor_flip(unsigned long param)
{
	return 0;
}
