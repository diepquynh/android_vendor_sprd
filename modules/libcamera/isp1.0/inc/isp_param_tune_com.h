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
#ifndef _ISP_PARAM_TUNE_COM_H_
#define _ISP_PARAM_TUNE_COM_H_
/*----------------------------------------------------------------------------*
 **				Dependencies					*
 **---------------------------------------------------------------------------*/
#include <sys/types.h>
#include "sensor_drv_u.h"
#include "sensor_raw.h"
#include "isp_raw.h"

/**---------------------------------------------------------------------------*
 **				Compiler Flag					*
 **---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

/**---------------------------------------------------------------------------*
**				Micro Define					*
**----------------------------------------------------------------------------*/
typedef int32_t (*isp_fun)(void* param_ptr);

#define ISP_AE_TAB_NUM 0x04
#define ISP_CMC_NUM 0x09
#define ISP_AWB_NUM 0x09
#define ISP_MAP_NUM 0x08

#define ISP_END_ID 0xffff
#define ISP_VERSION_0000_ID 0x00000000
#define ISP_VERSION_0001_ID 0x00010000
#define ISP_VERSION_00010001_ID 0x00010001
#define ISP_VERSION_00020000_ID 0x00020000

#define ISP_PACKET_VERIFY_ID 0x71717567
#define ISP_PACKET_END_ID 0x69656e64

#define ISP_TOOL_CMD_ID 0x80000000

//parser cmd
#define ISP_PARSER_DOWN 0x0000
#define ISP_PARSER_UP_PARAM   0x0001
#define ISP_PARSER_UP_PRV_DATA 0x0003
#define ISP_PARSER_UP_CAP_DATA 0x0004
#define ISP_PARSER_UP_CAP_SIZE 0x0005
#define ISP_PARSER_UP_MAIN_INFO 0x0006
#define ISP_PARSER_UP_SENSOR_REG 0x0007
#define ISP_PARSER_UP_INFO 0x0008

//packet type
#define ISP_TYPE_CMD   0x0000
#define ISP_TYPE_PARAM 0x0001
#define ISP_TYPE_LEVEL 0x0002
#define ISP_TYPE_PRV_DATA  0x0003
#define ISP_TYPE_CAP_DATA  0x0004
#define ISP_TYPE_MAIN_INFO 0x0005
#define ISP_TYPE_SENSOR_REG 0x0006
#define ISP_TYPE_INFO 0x0007

#define ISP_PACKET_ALL 0x0000
#define ISP_PACKET_BLC 0x0001
#define ISP_PACKET_NLC 0x0002
#define ISP_PACKET_LNC 0x0003
#define ISP_PACKET_AE 0x0004
#define ISP_PACKET_AWB 0x0005
#define ISP_PACKET_BPC 0x0006
#define ISP_PACKET_DENOISE 0x0007
#define ISP_PACKET_GRGB 0x0008
#define ISP_PACKET_CFA 0x0009
#define ISP_PACKET_CMC 0x000A
#define ISP_PACKET_GAMMA 0x000B
#define ISP_PACKET_UV_DIV 0x000C
#define ISP_PACKET_PREF 0x000D
#define ISP_PACKET_BRIGHT 0x000E
#define ISP_PACKET_CONTRAST 0x000F
#define ISP_PACKET_HIST 0x0010
#define ISP_PACKET_AUTO_CONTRAST 0x0011
#define ISP_PACKET_SATURATION 0x0012
#define ISP_PACKET_CSS 0x0013
#define ISP_PACKET_AF 0x0014
#define ISP_PACKET_EDGE 0x0015
#define ISP_PACKET_SPECIAL_EFFECT 0x0016
#define ISP_PACKET_HDR 0x0017
#define ISP_PACKET_GLOBAL 0x0018
#define ISP_PACKET_CHN 0x0019
#define ISP_PACKET_LNC_PARAM 0x001a
#define ISP_PACKET_AWB_MAP 0x001b
#define ISP_PACKET_MAX 0xFFFF

#define ISP_VIDEO_YUV422_2FRAME (1<<0)
#define ISP_VIDEO_YUV420_2FRAME (1<<1)
#define ISP_VIDEO_NORMAL_RAW10 (1<<2)
#define ISP_VIDEO_MIPI_RAW10 (1<<3)
#define ISP_VIDEO_JPG (1<<4)
#define ISP_VIDEO_YVU420_2FRAME (1<<6)

#define ISP_UINT8 0x01
#define ISP_UINT16 0x02
#define ISP_UINT32 0x04
#define ISP_INT8 0x01
#define ISP_INT16 0x02
#define ISP_INT32 0x04

/**---------------------------------------------------------------------------*
**				Data Prototype					*
**----------------------------------------------------------------------------*/

enum isp_parser_cmd{
	ISP_PREVIEW=0x00,
	ISP_STOP_PREVIEW,
	ISP_CAPTURE,
	ISP_UP_PREVIEW_DATA,
	ISP_UP_PARAM,
	ISP_TAKE_PICTURE_SIZE,
	ISP_MAIN_INFO,
	ISP_READ_SENSOR_REG,
	ISP_WRITE_SENSOR_REG,
	ISP_BIN_TO_PACKET,
	ISP_PACKET_TO_BIN,
	ISP_INFO,
	ISP_PARSER_CMD_MAX
};

struct isp_main_info{
	char sensor_id[32];
	uint32_t version_id;
	uint32_t preview_format;
	uint32_t preview_size;
	uint32_t capture_format;
	uint32_t capture_num;
	uint32_t capture_size[8];
};

struct isp_version_info{
	uint32_t version_id;
	uint32_t srtuct_size;
	uint32_t reserve;
};

struct isp_param_info{
	char main_type[32];
	char sub_type[32];
	char third_type[32];
	uint32_t data_type;
	uint32_t data_num;
	uint32_t addr_offset;
};

struct isp_parser_up_data{
	uint32_t format;
	uint32_t pattern;
	uint32_t width;
	uint32_t height;
	uint32_t* buf_addr;
	uint32_t buf_len;
};

struct isp_parser_buf_in{
	unsigned long buf_addr;
	uint32_t buf_len;
};

struct isp_parser_buf_rtn{
	unsigned long buf_addr;
	uint32_t buf_len;
};

struct isp_parser_cmd_param{
	enum isp_parser_cmd cmd;
	uint32_t param[48]; // capture param format/width/height
};

struct isp_param_fun{
	uint32_t cmd;
	int32_t (*param_fun)(void* param_ptr);
};

uint32_t ispParserGetSizeID(uint32_t width, uint32_t height);
int32_t ispParser(uint32_t cmd, void* in_param_ptr, void* rtn_param_ptr);
uint32_t* ispParserAlloc(uint32_t size);
int32_t ispParserFree(void* addr);

/**----------------------------------------------------------------------------*
**				Compiler Flag					*
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif
// End

