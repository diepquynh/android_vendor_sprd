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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdint.h>
#include <sys/types.h>
#include <semaphore.h>
#include <pthread.h>
#define LOG_TAG "isp-video"
#include <cutils/log.h>
#include <hardware/camera.h>

#include "isp_param_tune_com.h"
#include "isp_log.h"
#include "isp_video.h"
#include "isp_otp.h"
#include "sensor_raw.h"
#define DATA_CMD_LENGTH 32
#define PACKET_MSG_HEADER 8
#define ISP_READ_MODE_ID_MAX 13

#define ISP_NR_BLOCK_MIN 1
#define ISP_ISO_NUM_MAX 7
#define ISP_ISO_NUM_MIN 0
#define ISP_AE_WEIGHT_TYPE_MAX 2
#define ISP_AE_WEIGHT_TYPE_MIN 0
#define ISP_SCENE_NUM_MAX 7
#define ISP_SCENE_NUM_MIN 0
#define ISP_ANTIFLICKER_MIN 0
#define ISP_ANTIFLICKER_MAX 1
#define ISP_LNC_TAB_MAX 8
#define ISP_LNC_TAB_MIN 0
#define ISP_SIMULATOR_MAX 0xffff
#define ISP_SIMULATOR_MIN 0x0000
extern isp_u16 multi_mode[ISP_BLK_TYPE_MAX][MAX_MODE_NUM];
enum {
	CMD_START_PREVIEW = 1,
	CMD_STOP_PREVIEW,
	CMD_READ_ISP_PARAM,
	CMD_WRITE_ISP_PARAM,
	CMD_GET_PREVIEW_PICTURE,
	CMD_AUTO_UPLOAD,
	CMD_UPLOAD_MAIN_INFO,
	CMD_TAKE_PICTURE,
	CMD_ISP_LEVEL,
	CMD_READ_SENSOR_REG,
	CMD_WRITE_SENSOR_REG,
	CMD_GET_INFO,
	CMD_OTP_WRITE,
	CMD_OTP_READ,
	CMD_READ_ISP_PARAM_V1,//15
	CMD_WRITE_ISP_PARAM_V1,//16
	CMD_DOWNLOAD_RAW_PIC,  //17
	CMD_WRTIE_SCENE_PARAM, //18
	CMD_START_SIMULATION,  //19

	CMD_SFT_READ = 200,
	CMD_SFT_WRITE,
	CMD_SFT_TRIG,
	CMD_SFT_SET_POS,
	CMD_SFT_GET_POS,
	CMD_SFT_TAKE_PICTURE,
	CMD_SFT_TAKE_PICTURE_NEW,
	CMD_SFT_OPEN_FILTER,
	CMD_SFT_GET_AF_VALUE,

	CMD_ASIC_TAKE_PICTURE,
	CMD_ASIC_TAKE_PICTURE_NEW

};

typedef enum {
	BDN = 0x01,
	BPC,
	CCE_UV,
	CFAE,
	EDGE,
	FLAT_OFFSET, //0x06
	GRGB,
	IIR_YRANDOM,
	IIRCNR,
	IVST,
	NLM,
	PRFY, //0x0c
	PWD,
	RGB_PRECDN,
	VST,
	Y_AFM,
	YUV_CDN,
	YUV_POSTCDN, //0x12
	YUV_PRECDN,
	FILE_NAME_MAX
}DENOISE_DATA_NAME;

typedef enum {
	NORMAL = 0x00,
	NIGHT,
	SPORT,
	PORTRAIT,
	LANDSPACE,
	PANORAMA, //0x05
	USER0,
	USER1,
	USER2,
	USER3,
	USER4,
	USER5,
	USER6,
	USER7,
	USER8,
	USER9,
	SCENE_MODE_MAX
}DENOISE_SCENE_NAME;

enum{
	MODE_DUMP_DATA,
	MODE_NR_DATA,
	MODE_ISP_ID = 0x02,
	MODE_TUNE_INFO = 0x03,
	MODE_AE_TABLE,
	MODE_AE_WEIGHT_TABLE,
	MODE_AE_SCENE_TABLE,
	MODE_AE_AUTO_ISO_TABLE,
	MODE_LNC_DATA,
	MODE_AWB_DATA,
	MODE_NOTE_DATA,
	MODE_LIB_INFO_DATA,
	MODE_MAX
};

enum{
	AE_TABLE_FIVE_AUTO = 0x00,
	AE_TABLE_FIVE_ONE,
	AE_TABLE_FIVE_TWO,//'FIVE' is 50Hz,'SIX' is 60Hz;'ONE/TWO/FOUR....' is ISO100/ISO200/ISO400....
	AE_TABLE_FIVE_FOUR,
	AE_TABLE_FIVE_AEIGHT,
	AE_TABLE_FIVE_SIXTEEN,
	AE_TABLE_SIX_AUTO = 0x10,
	AE_TABLE_SIX_ONE,
	AE_TABLE_SIX_TWO,//'FIVE' is 50Hz,'SIX' is 60Hz;'ONE/TWO/FOUR....' is ISO100/ISO200/ISO400....
	AE_TABLE_SIX_FOUR,
	AE_TABLE_SIX_AEIGHT,
	AE_TABLE_SIX_SIXTEEN,
	AE_TABLE_MAX
};

enum{
	AE_WEIGHT_AVG = 0X00,
	AE_WEIGHT_CENTER,
	AE_WEIGHT_SPOT,
	AE_WEIGHT_MAX
};

enum{
	AE_SCENE_ZERO_FIVE = 0x00,
	AE_SCENE_ZERO_SIX,
	AE_SCENE_ONE_FIVE = 0x10,
	AE_SCENE_ONE_SIX,
	AE_SCENE_TWO_FIVE = 0x20,
	AE_SCENE_TWO_SIX,
	AE_SCENE_THREE_FIVE = 0x30,
	AE_SCENE_THREE_SIX,
	AE_SCENE_FOUR_FIVE = 0x40,
	AE_SCENE_FOUR_SIX,
	AE_SCENE_FIVE_FIVE = 0x50,
	AE_SCENE_FIVE_SIX,
	AE_SCENE_SIX_FIVE = 0x60,
	AE_SCENE_SIX_SIX,
	AE_SCENE_SEVEN_FIVE = 0x70,
	AE_SCENE_SEVEN_SIX ,
	AE_SCENE_MAX
};

enum{
	AE_AUTO_ISO_ONE = 0x00,
	AE_AUTO_ISO_TWO,
	AE_AUTO_ISO_MAX
};

enum{
	LNC_CT_ZERO = 0x00,
	LNC_CT_ONE,
	LNC_CT_TWO,
	LNC_CT_THREE,
	LNC_CT_FOUR,
	LNC_CT_FIVE,
	LNC_CT_SIX,
	LNC_CT_SEVEN,
	LNC_CT_EIGHT,
	LNC_CT_MAX
};

enum{
	AWB_WIN_MAP = 0x00,
	AWB_WEIGHT,
	AWB_MAX
};

// This is the communication frame head
typedef struct msg_head_tag
{
	unsigned int  seq_num;      // Message sequence number, used for flow control
	unsigned short  len;          // The totoal size of the packet "sizeof(MSG_HEAD_T)
	                      // + packet size"
	unsigned char   type;         // Main command type
	unsigned char   subtype;      // Sub command type
}__attribute__((packed)) MSG_HEAD_T;

typedef struct {
	uint32_t headlen;
	uint32_t img_format;
	uint32_t img_size;
	uint32_t totalpacket;
	uint32_t packetsn;
}ISP_IMAGE_HEADER_T;
//add
typedef struct {
	uint32_t totalpacket;
	uint32_t packetsn;
	uint32_t reserve[3];
}ISP_DATA_HEADER_T;

struct isp_data_header_read
{
	uint16_t main_type;
	uint16_t sub_type;
	uint8_t isp_mode;
	uint8_t nr_mode;
	uint8_t reserved[26];
}__attribute__((packed));

struct isp_data_header_normal
{
	uint32_t data_total_len;
	uint16_t main_type;
	uint16_t sub_type;
	uint8_t isp_mode;
	uint8_t packet_status;
	uint8_t nr_mode;
	uint8_t reserved[21];
}__attribute__((packed));

struct isp_check_cmd_valid
{
	uint16_t main_type;
	uint16_t sub_type;
	uint8_t isp_mode;
}__attribute__((packed));


struct camera_func{
	int32_t(*start_preview) (uint32_t param1, uint32_t param2);
	int32_t(*stop_preview) (uint32_t param1, uint32_t param2);
	int32_t(*take_picture) (uint32_t param1, uint32_t param2);
	int32_t(*set_capture_size) (uint32_t width, uint32_t height);
};

#define PREVIEW_MAX_WIDTH 640
#define PREVIEW_MAX_HEIGHT 480

#define CMD_BUF_SIZE  65536 // 64k
#define SEND_IMAGE_SIZE 64512 // 63k
#define SEND_DATA_SIZE 64512 //63k
#define DATA_BUF_SIZE 65536 //64k
#define PORT_NUM 16666        /* Port number for server */
#define BACKLOG 5
#define ISP_CMD_SUCCESS             0x0000
#define ISP_CMD_FAIL                0x0001
#define IMAGE_RAW_TYPE 0
#define IMAGE_YUV_TYPE 1

#define CLIENT_DEBUG
#ifdef CLIENT_DEBUG
#define DBG ISP_LOGE
#endif
static uint8_t* preview_buf_ptr = 0;
static unsigned char diag_rx_buf[CMD_BUF_SIZE];
static unsigned char temp_rx_buf[CMD_BUF_SIZE];
static unsigned char diag_cmd_buf[CMD_BUF_SIZE];
static unsigned char eng_rsp_diag[DATA_BUF_SIZE];
static int preview_flag = 0; // 1: start preview
static int preview_img_end_flag = 1; // 1: preview image send complete
static int capture_img_end_flag = 1; // 1: capture image send complete
static int capture_format = 1; // 1: start preview
static int capture_flag = 0; // 1: call get pic
char raw_filename[200] = {0};
uint8_t nr_tool_flag[17] = {0};
uint32_t tool_fmt_pattern = INVALID_FORMAT_PATTERN;
static FILE* raw_fp = NULL;
static struct isptool_scene_param scene_param = {0,0,0,0,0,0,0,0};

static sem_t preview_sem_lock;
static sem_t capture_sem_lock;
static pthread_mutex_t ispstream_lock;
static int wire_connected = 0;
static int sockfd = 0;
static int sock_fd=0;
static int rx_packet_len=0;
static int rx_packet_total_len=0;
int sequence_num = 0;
struct camera_func s_camera_fun={PNULL, PNULL, PNULL, PNULL};
struct camera_func* s_camera_fun_ptr=&s_camera_fun;
static void* isp_handler;

static uint32_t g_af_pos=0;// the af position
static uint32_t g_type = 8;// the mipi
static uint32_t g_command=CMD_TAKE_PICTURE;
struct denoise_param_update nr_update_param;

struct camera_func* ispvideo_GetCameraFunc(void)
{
	return s_camera_fun_ptr;
}

int ispvideo_SetreTurnValue(uint8_t* rtn_ptr, uint32_t rtn)
{
	uint32_t* rtn_value=(uint32_t*)rtn_ptr;

	*rtn_value=rtn;

	return 0x04;
}

uint32_t ispvideo_GetIspParamLenFromSt(unsigned char* dig_ptr)
{
	uint32_t data_len=0x00;
	uint16_t* data_ptr=(uint16_t*)(dig_ptr+0x05);

	if(NULL!=dig_ptr)
	{
		data_len=data_ptr[0];
		data_len-=0x08;
	}

	return data_len;
}


uint32_t ispvideo_GetImgDataLen(unsigned char* dig_ptr)
{
	uint32_t data_len=0x00;
	uint16_t* data_ptr=(uint16_t*)(dig_ptr+0x05);

	if(NULL!=dig_ptr)
	{
		data_len=data_ptr[0];
		data_len-=0x08;
		data_len-=sizeof(ISP_IMAGE_HEADER_T);
	}

	return data_len;
}


unsigned char* ispvideo_GetImgDataInfo(unsigned char* dig_ptr, uint32_t* packet_sn, uint32_t* total_pack,
											uint32_t* img_width, uint32_t* img_height, uint32_t* img_headlen)
{
	unsigned char* data_ptr = NULL;
	unsigned char *tmp_ptr;
	ISP_IMAGE_HEADER_T img_info;

	if (!dig_ptr || !packet_sn || !total_pack || !img_width || !img_height || !img_headlen) {
		ISP_LOGI("invalid param");
		return data_ptr;
	}

	tmp_ptr = dig_ptr + 1 + sizeof(MSG_HEAD_T);

	memcpy(&img_info, tmp_ptr, sizeof(ISP_IMAGE_HEADER_T));
	*img_headlen = img_info.headlen;
	*packet_sn = img_info.packetsn;
	*total_pack = img_info.totalpacket;
	*img_width = (img_info.img_size>>0x10)&0xFFFF;
	*img_height = img_info.img_size&0xFFFF;
	ISP_LOGI("ImageInfo headlen %d, packetsn %d, total_packet %d, img_width %d, img_height %d",
		*img_headlen, *packet_sn, *total_pack, *img_width, *img_height);

	data_ptr = dig_ptr + 1 + sizeof(MSG_HEAD_T) + sizeof(ISP_IMAGE_HEADER_T);
	return data_ptr;
}

int ispvideo_GetSceneInfo(unsigned char* dig_ptr, struct isptool_scene_param *scene_parm)
{
	unsigned char* data_ptr = NULL;
	int rtn = 0;

	if (!dig_ptr || !scene_parm) {
		return -1;
	}

	data_ptr = dig_ptr + 1 + sizeof(MSG_HEAD_T);
	//little endian
	memcpy(((void*)scene_parm)+8, data_ptr, sizeof(struct isptool_scene_param) -8);

	return rtn;
}

int ispvideo_GetIspParamFromSt(unsigned char* dig_ptr, struct isp_parser_buf_rtn* isp_ptr)
{
	int rtn=0x00;

	if((NULL != dig_ptr)
		&& (NULL != (void *)(isp_ptr->buf_addr))
		&& (0x00 != isp_ptr->buf_len))
	{
		memcpy((void*)isp_ptr->buf_addr, (void*)dig_ptr, isp_ptr->buf_len);
	}

	return rtn;
}

void*  ispvideo_GetIspHandle(void)
{
	return isp_handler;
}

uint32_t ispvideo_SetIspParamToSt(unsigned char* dig_ptr, struct isp_parser_buf_in* isp_ptr)
{
	uint32_t buf_len=0x00;

	if((NULL != dig_ptr)
		&& (NULL != (void *)(isp_ptr->buf_addr))
		&& (0x00!=isp_ptr->buf_len))
	{
		memcpy((void*)dig_ptr, (void*)isp_ptr->buf_addr, isp_ptr->buf_len);

		buf_len=isp_ptr->buf_len;
	}

	return buf_len;
}

static int handle_img_data(uint32_t format, uint32_t width,uint32_t height, char *ch0_ptr, int ch0_len,char *ch1_ptr, int ch1_len,char *ch2_ptr, int ch2_len)
{
	int i,  res, total_number;
	int send_number=0;
	int chn0_number, chn1_number, chn2_number;
	int len = 0, rlen = 0, rsp_len = 0, extra_len = 0;
	MSG_HEAD_T *msg_ret;
	ISP_IMAGE_HEADER_T isp_msg;

	chn0_number = (ch0_len + SEND_IMAGE_SIZE - 1) /SEND_IMAGE_SIZE;
	chn1_number = (ch1_len + SEND_IMAGE_SIZE - 1) /SEND_IMAGE_SIZE;
	chn2_number = (ch2_len + SEND_IMAGE_SIZE - 1) /SEND_IMAGE_SIZE;

	total_number = chn0_number + chn1_number + chn2_number;
	//total_number = (ch0_len + SEND_IMAGE_SIZE - 1) /SEND_IMAGE_SIZE;

	msg_ret = (MSG_HEAD_T *)(eng_rsp_diag+1);

	for (i=0; i<chn0_number; i++, send_number++)
	{// chn0
		if (i < chn0_number-1)
			len = SEND_IMAGE_SIZE;
		else
			len = ch0_len-SEND_IMAGE_SIZE*i;
		rsp_len = sizeof(MSG_HEAD_T)+1;

		// combine data
		isp_msg.headlen = 20;
		isp_msg.img_format = format;
		isp_msg.img_size = (width<<0x10)|height;
		isp_msg.totalpacket = total_number;
		isp_msg.packetsn = send_number+1;

		memcpy(eng_rsp_diag+rsp_len, (char *)&isp_msg, sizeof(ISP_IMAGE_HEADER_T));
		rsp_len += sizeof(ISP_IMAGE_HEADER_T);

		//DBG("%s:ISP_TOOL: request rsp_len[%d]\n",__FUNCTION__, rsp_len);
		memcpy(eng_rsp_diag+rsp_len, (char *)ch0_ptr+i*SEND_IMAGE_SIZE, len);
		rsp_len += len;

		eng_rsp_diag[rsp_len] = 0x7e;
		msg_ret->len = rsp_len-1;
		msg_ret->seq_num = sequence_num++;
		//DBG("%s:ISP_TOOL: request rsp_len[%d]\n",__FUNCTION__, rsp_len);
		res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);

	}

	for (i=0; i<chn1_number; i++, send_number++)
	{// chn1
		if (i < chn1_number-1)
			len = SEND_IMAGE_SIZE;
		else
			len = ch1_len-SEND_IMAGE_SIZE*i;
		rsp_len = sizeof(MSG_HEAD_T)+1;

		// combine data
		isp_msg.headlen = 20;
		isp_msg.img_format = format;
		isp_msg.img_size = (width<<0x10)|height;
		isp_msg.totalpacket = total_number;
		isp_msg.packetsn = send_number+1;

		memcpy(eng_rsp_diag+rsp_len, (char *)&isp_msg, sizeof(ISP_IMAGE_HEADER_T));
		rsp_len += sizeof(ISP_IMAGE_HEADER_T);

		//DBG("%s:ISP_TOOL: request rsp_len[%d]\n",__FUNCTION__, rsp_len);
		memcpy(eng_rsp_diag+rsp_len, (char *)ch1_ptr+i*SEND_IMAGE_SIZE, len);
		rsp_len += len;

		eng_rsp_diag[rsp_len] = 0x7e;
		msg_ret->len = rsp_len-1;
		msg_ret->seq_num = sequence_num++;
		//DBG("%s:ISP_TOOL: request rsp_len[%d]\n",__FUNCTION__, rsp_len);
		res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);

	}

	for (i=0; i<chn2_number; i++, send_number++)
	{// chn2
		if (i < chn2_number-1)
			len = SEND_IMAGE_SIZE;
		else
			len = ch2_len-SEND_IMAGE_SIZE*i;
		rsp_len = sizeof(MSG_HEAD_T)+1;

		// combine data
		isp_msg.headlen = 20;
		isp_msg.img_format = format;
		isp_msg.img_size = (width<<0x10)|height;
		isp_msg.totalpacket = total_number;
		isp_msg.packetsn = send_number+1;

		memcpy(eng_rsp_diag+rsp_len, (char *)&isp_msg, sizeof(ISP_IMAGE_HEADER_T));
		rsp_len += sizeof(ISP_IMAGE_HEADER_T);

		//DBG("%s:ISP_TOOL: request rsp_len[%d]\n",__FUNCTION__, rsp_len);
		memcpy(eng_rsp_diag+rsp_len, (char *)ch2_ptr+i*SEND_IMAGE_SIZE, len);
		rsp_len += len;

		eng_rsp_diag[rsp_len] = 0x7e;
		msg_ret->len = rsp_len-1;
		msg_ret->seq_num = sequence_num++;
		//DBG("%s:ISP_TOOL: request rsp_len[%d]\n",__FUNCTION__, rsp_len);
		res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);

	}

	return 0;
}

static int handle_img_data_asic(uint32_t format, uint32_t width,uint32_t height, char *ch0_ptr, int ch0_len,char *ch1_ptr, int ch1_len,char *ch2_ptr, int ch2_len)
{
	UNUSED(format);
	UNUSED(width);
	UNUSED(height);
	char name[100] = {'\0'};
	FILE* fp = NULL;
	switch( capture_format ){
	case 8:// the mipi
		sprintf(name,"/data/internal-asec/the_pic_%u.mipi_raw",g_af_pos);
		break;
	case 16://jpeg
		sprintf(name,"/data/internal-asec/the_pic_%u.jpg",g_af_pos);
		break;
	}
	fp = fopen(name,"wb");

	if( fp==NULL ){
		ISP_LOGI("Chj--create file fails");
		return 0;
	}

	fwrite(ch0_ptr, 1,ch0_len,fp);

	fwrite(ch1_ptr, 1,ch1_len,fp);

	fwrite(ch2_ptr, 1,ch2_len,fp);
	fclose(fp);
	ISP_LOGI("Chj--writing one pic succeeds");
	return 0;
}

static int handle_img_data_sft(uint32_t format, uint32_t width,uint32_t height, char *ch0_ptr, int ch0_len,char *ch1_ptr, int ch1_len,char *ch2_ptr, int ch2_len)
{
	UNUSED(format);
	UNUSED(width);
	UNUSED(height);
	char name[100] = {'\0'};
	FILE* fp = NULL;
	switch( capture_format ){
	case 8:// the mipi
		sprintf(name,"/data/misc/media/the_pic_%u.mipi_raw",g_af_pos);
		break;
	case 16://jpeg
		sprintf(name,"/data/misc/media/the_pic_%u.jpg",g_af_pos);
		break;
	}
	fp = fopen(name,"wb");

	if( fp==NULL ){
		ISP_LOGI("Chj--create file fails");
		return 0;
	}

	fwrite(ch0_ptr, 1,ch0_len,fp);

	fwrite(ch1_ptr, 1,ch1_len,fp);

	fwrite(ch2_ptr, 1,ch2_len,fp);
	fclose(fp);
	ISP_LOGI("Chj--writing one pic succeeds");
	return 0;
}

int isp_sft_read(isp_handle handler, uint8_t *data_buf, uint32_t* data_size){
	int ret = 0;
	struct isp_parser_buf_rtn rtn_param = {0x00, 0x00};

	rtn_param.buf_addr = (unsigned long)data_buf;

	ret = isp_ioctl(handler, ISP_CTRL_SFT_READ, (void*)&rtn_param);// get af info after auto focus

	*data_size = rtn_param.buf_len;

	return ret;
}
int isp_sft_write(isp_handle handler, uint8_t *data_buf, uint32_t *data_size){//DATA
	int ret = 0;
	struct isp_parser_buf_rtn rtn_param = {0x00, 0x00};
	rtn_param.buf_addr = (unsigned long)data_buf;
	rtn_param.buf_len = *data_size;
	ret = isp_ioctl(handler, ISP_CTRL_SFT_WRITE, (void*)&rtn_param);// set af info after auto focus

	return ret;
}

static int isp_tool_calc_nr_addr_offset(uint8_t isp_mode, uint8_t nr_mode, isp_u16 * one_multi_mode_ptr, isp_u32 *offset_units_ptr)
{
	int rtn = ISP_SUCCESS;
	isp_u32  offset_units = 0;
	isp_u32 i = 0, j = 0;

	if(isp_mode > (MAX_MODE_NUM - 1))
		return ISP_ERROR;
	if(nr_mode > (MAX_SCENEMODE_NUM- 1))
		return ISP_ERROR;
	if(!one_multi_mode_ptr)
		return ISP_ERROR;

	for( i = 0; i < isp_mode; i++) {
		if(one_multi_mode_ptr[i]) {
			for( j = 0; j < MAX_SCENEMODE_NUM; j++ ) {
				offset_units += (one_multi_mode_ptr[i] >> j) & 0x01;
			}
		}
	}

	for( j = 0; j < nr_mode; j++ ) {
		offset_units += (one_multi_mode_ptr[isp_mode] >> j) & 0x01;
	}
	*offset_units_ptr = offset_units;
	return rtn;
}

int isp_denoise_write(uint8_t *data_buf, uint32_t *data_size)
{
	int ret = ISP_SUCCESS;
	uint8_t *data_actual_ptr;
	uint32_t data_actual_len;
	uint8_t isp_mode = 0;
	uint8_t nr_mode = 0;
	isp_u32 offset_units = 0;
	uint32_t nr_offset_addr = 0;
	if ((NULL == data_buf) || (NULL == data_size)) {
		ISP_LOGE("param error");
		return ISP_PARAM_NULL;
	}
	struct isp_data_header_normal *data_head = (struct isp_data_header_normal *)data_buf;

	data_actual_ptr = data_buf + sizeof(struct isp_data_header_normal);
	data_actual_len = *data_size - sizeof(struct isp_data_header_normal);

	if(nr_update_param.multi_nr_flag != SENSOR_MULTI_MODE_FLAG) {
		isp_mode = 0;
		nr_mode = 0;
	} else {
		isp_mode = data_head->isp_mode;
		nr_mode = data_head->nr_mode;
	}
		switch(data_head->sub_type) {
			case BDN :
			{
				static uint32_t bdn_ptr_offset;
				isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_BL_NR_TYPE][0], &offset_units);
				nr_offset_addr = offset_units * sizeof(struct sensor_bdn_level) * SENSOR_SMART_LEVEL_NUM;
				memcpy(((uint8_t*)(nr_update_param.bdn_level_ptr)) + nr_offset_addr + bdn_ptr_offset, (uint8_t*)data_actual_ptr, data_actual_len);
				if (0x01 != data_head->packet_status)
					bdn_ptr_offset += data_actual_len;
				else
					bdn_ptr_offset = 0;
				nr_tool_flag[2] = 1;
				break;
			}
			case BPC :
			{
				static uint32_t bpc_ptr_offset;
				isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_BPC_TYPE][0], &offset_units);
				nr_offset_addr = offset_units * sizeof(struct sensor_bpc_level) * SENSOR_SMART_LEVEL_NUM;
				memcpy(((uint8_t*)(nr_update_param.bpc_level_ptr))  + nr_offset_addr + bpc_ptr_offset, (uint8_t*)data_actual_ptr, data_actual_len);
				if (0x01 != data_head->packet_status)
					bpc_ptr_offset += data_actual_len;
				else
					bpc_ptr_offset = 0;
				nr_tool_flag[1] = 1;
				break;
			}
			case CCE_UV :
			{
				static uint32_t cce_ptr_offset;
				isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_UVDIV_TYPE][0], &offset_units);
				nr_offset_addr = offset_units * sizeof(struct sensor_cce_uvdiv_level) * SENSOR_SMART_LEVEL_NUM;
				memcpy(((uint8_t*)(nr_update_param.cce_uvdiv_level_ptr)) + nr_offset_addr + cce_ptr_offset, (uint8_t*)data_actual_ptr, data_actual_len);
				if (0x01 != data_head->packet_status)
					cce_ptr_offset += data_actual_len;
				else
					cce_ptr_offset = 0;
				nr_tool_flag[16] = 1;
				break;
			}
			case CFAE :
			{
				static uint32_t cfae_ptr_offset;
				isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_CFA_TYPE][0], &offset_units);
				nr_offset_addr = offset_units * sizeof(struct sensor_cfae_level) * SENSOR_SMART_LEVEL_NUM;
				memcpy(((uint8_t*)(nr_update_param.cfae_level_ptr)) + nr_offset_addr + cfae_ptr_offset, (uint8_t*)data_actual_ptr, data_actual_len);
				if (0x01 != data_head->packet_status)
					cfae_ptr_offset += data_actual_len;
				else
					cfae_ptr_offset = 0;
				nr_tool_flag[6] = 1;
				break;
			}
			case EDGE :
			{
				static uint32_t edge_ptr_offset;
				isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_EDGE_TYPE][0], &offset_units);
				nr_offset_addr = offset_units * sizeof(struct sensor_ee_level) * SENSOR_SMART_LEVEL_NUM;
				memcpy(((uint8_t*)(nr_update_param.ee_level_ptr)) + nr_offset_addr  + edge_ptr_offset, (uint8_t*)data_actual_ptr, data_actual_len);
				if (0x01 != data_head->packet_status)
					edge_ptr_offset += data_actual_len;
				else
					edge_ptr_offset = 0;
				nr_tool_flag[13] = 1;
				break;
			}
			case FLAT_OFFSET :
			{
				static uint32_t flat_ptr_offset;
				isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_NLM_TYPE][0], &offset_units);
				nr_offset_addr = offset_units * sizeof(struct sensor_flat_offset_level) * SENSOR_SMART_LEVEL_NUM;
				memcpy(((uint8_t*)(nr_update_param.flat_offset_level_ptr)) + nr_offset_addr  + flat_ptr_offset, (uint8_t*)data_actual_ptr, data_actual_len);
				if (0x01 != data_head->packet_status)
					flat_ptr_offset += data_actual_len;
				else
					flat_ptr_offset = 0;
				nr_tool_flag[4] = 1;
				nr_tool_flag[5] = 1;
				break;
			}
			case GRGB :
			{
				static uint32_t grgb_ptr_offset;
				isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_GRGB_TYPE][0], &offset_units);
				nr_offset_addr = offset_units * sizeof(struct sensor_grgb_v1_level) * SENSOR_SMART_LEVEL_NUM;
				memcpy(((uint8_t*)(nr_update_param.grgb_v1_level_ptr)) + nr_offset_addr + grgb_ptr_offset, (uint8_t*)data_actual_ptr, data_actual_len);
				if (0x01 != data_head->packet_status)
					grgb_ptr_offset += data_actual_len;
				else
					grgb_ptr_offset = 0;
				nr_tool_flag[3] = 1;
				break;
			}
			case IIR_YRANDOM :
			{
				static uint32_t iir_ptr_offset;
				isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_IIRCNR_YRANDOM_TYPE][0], &offset_units);
				nr_offset_addr = offset_units * sizeof(struct sensor_iircnr_yrandom_level) * SENSOR_SMART_LEVEL_NUM;
				memcpy(((uint8_t*)(nr_update_param.iircnr_yrandom_level_ptr))  + nr_offset_addr + iir_ptr_offset, (uint8_t*)data_actual_ptr, data_actual_len);
				if (0x01 != data_head->packet_status)
					iir_ptr_offset += data_actual_len;
				else
					iir_ptr_offset = 0;
				nr_tool_flag[15] = 1;
				break;
			}
			case IIRCNR :
			{
				static uint32_t iircnr_ptr_offset;
				isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_IIRCNR_IIR_TYPE][0], &offset_units);
				nr_offset_addr = offset_units * sizeof(struct sensor_iircnr_level) * SENSOR_SMART_LEVEL_NUM;
				memcpy(((uint8_t*)(nr_update_param.iircnr_level_ptr)) + nr_offset_addr + iircnr_ptr_offset, (uint8_t*)data_actual_ptr, data_actual_len);
				if (0x01 != data_head->packet_status)
					iircnr_ptr_offset += data_actual_len;
				else
					iircnr_ptr_offset = 0;
				nr_tool_flag[14] = 1;
				break;
			}
			case IVST :
			{
				static uint32_t ivst_ptr_offset;
				isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_NLM_TYPE][0], &offset_units);
				nr_offset_addr = offset_units * sizeof(struct sensor_ivst_level) * SENSOR_SMART_LEVEL_NUM;
				memcpy(((uint8_t*)(nr_update_param.ivst_level_ptr)) + nr_offset_addr + ivst_ptr_offset, (uint8_t*)data_actual_ptr, data_actual_len);
				if (0x01 != data_head->packet_status)
					ivst_ptr_offset += data_actual_len;
				else
					ivst_ptr_offset = 0;
				nr_tool_flag[4] = 1;
				nr_tool_flag[5] = 1;
				break;
			}
			case NLM :
			{
				static uint32_t nlm_ptr_offset;
				isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_NLM_TYPE][0], &offset_units);
				nr_offset_addr = offset_units * sizeof(struct sensor_nlm_level) * SENSOR_SMART_LEVEL_NUM;
				memcpy(((uint8_t*)(nr_update_param.nlm_level_ptr)) + nr_offset_addr + nlm_ptr_offset, (uint8_t*)data_actual_ptr, data_actual_len);
				if (0x01 != data_head->packet_status)
					nlm_ptr_offset += data_actual_len;
				else
					nlm_ptr_offset = 0;
				nr_tool_flag[4] = 1;
				nr_tool_flag[5] = 1;
				break;
			}
			case PRFY :
			{
				static uint32_t prey_ptr_offset;
				isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_PREF_TYPE][0], &offset_units);
				nr_offset_addr = offset_units * sizeof(struct sensor_prfy_level) * SENSOR_SMART_LEVEL_NUM;
				memcpy((uint8_t*)(nr_update_param.prfy_level_ptr) + nr_offset_addr + prey_ptr_offset, (uint8_t*)data_actual_ptr, data_actual_len);
				if (0x01 != data_head->packet_status)
					prey_ptr_offset += data_actual_len;
				else
					prey_ptr_offset = 0;
				nr_tool_flag[10] = 1;
				break;
			}
			case PWD :
			{
				static uint32_t pwd_ptr_offset;
				isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_PRE_WAVELET_TYPE][0], &offset_units);
				nr_offset_addr = offset_units * sizeof(struct sensor_pwd_level) * SENSOR_SMART_LEVEL_NUM;
				memcpy(((uint8_t*)(nr_update_param.pwd_level_ptr)) + nr_offset_addr + pwd_ptr_offset, (uint8_t*)data_actual_ptr, data_actual_len);
				if (0x01 != data_head->packet_status)
					pwd_ptr_offset += data_actual_len;
				else
					pwd_ptr_offset = 0;
				nr_tool_flag[0] = 1;
				break;
			}
			case RGB_PRECDN :
			{
				static uint32_t rgb_ptr_offset;
				isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_RGB_PRECDN_TYPE][0], &offset_units);
				nr_offset_addr = offset_units * sizeof(struct sensor_rgb_precdn_level) * SENSOR_SMART_LEVEL_NUM;
				memcpy(((uint8_t*)(nr_update_param.rgb_precdn_level_ptr)) + nr_offset_addr + rgb_ptr_offset, (uint8_t*)data_actual_ptr, data_actual_len);
				if (0x01 != data_head->packet_status)
					rgb_ptr_offset += data_actual_len;
				else
					rgb_ptr_offset = 0;
				nr_tool_flag[7] = 1;
				break;
			}
			case VST :
			{
				static uint32_t vst_ptr_offset;
				isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_NLM_TYPE][0], &offset_units);
				nr_offset_addr = offset_units * sizeof(struct sensor_vst_level) * SENSOR_SMART_LEVEL_NUM;
				memcpy((uint8_t*)(nr_update_param.vst_level_ptr) + nr_offset_addr + vst_ptr_offset, (uint8_t*)data_actual_ptr, data_actual_len);
				if (0x01 != data_head->packet_status)
					vst_ptr_offset += data_actual_len;
				else
					vst_ptr_offset = 0;
				nr_tool_flag[4] = 1;
				nr_tool_flag[5] = 1;
				break;
			}
			case Y_AFM :
			{
				static uint32_t y_ptr_offset;
				isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_YIQ_AFM_TYPE][0], &offset_units);
				nr_offset_addr = offset_units * sizeof(struct sensor_y_afm_level) * SENSOR_SMART_LEVEL_NUM;
				memcpy(((uint8_t*)(nr_update_param.y_afm_level_ptr)) + nr_offset_addr + y_ptr_offset, (uint8_t*)data_actual_ptr, data_actual_len);
				if (0x01 != data_head->packet_status)
					y_ptr_offset += data_actual_len;
				else
					y_ptr_offset = 0;
				nr_tool_flag[8] = 1;
				break;
			}
			case YUV_CDN :
			{
				static uint32_t cdn_ptr_offset;
				isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_UV_CDN_TYPE][0], &offset_units);
				nr_offset_addr = offset_units * sizeof(struct sensor_uv_cdn_level) * SENSOR_SMART_LEVEL_NUM;
				memcpy(((uint8_t*)(nr_update_param.uv_cdn_level_ptr)) + nr_offset_addr + cdn_ptr_offset, (uint8_t*)data_actual_ptr, data_actual_len);
				if (0x01 != data_head->packet_status)
					cdn_ptr_offset += data_actual_len;
				else
					cdn_ptr_offset = 0;
				nr_tool_flag[11] = 1;
				break;
			}
			case YUV_POSTCDN :
			{
				static uint32_t postcdn_ptr_offset;
				isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_UV_POSTCDN_TYPE][0], &offset_units);
				nr_offset_addr = offset_units * sizeof(struct sensor_uv_postcdn_level) * SENSOR_SMART_LEVEL_NUM;
				memcpy(((uint8_t*)(nr_update_param.uv_postcdn_level_ptr)) + nr_offset_addr + postcdn_ptr_offset, (uint8_t*)data_actual_ptr, data_actual_len);
				if (0x01 != data_head->packet_status)
					postcdn_ptr_offset += data_actual_len;
				else
					postcdn_ptr_offset = 0;
				nr_tool_flag[12] = 1;
				break;
			}
			case YUV_PRECDN :
			{
				static uint32_t precdn_ptr_offset;
				isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_YUV_PRECDN_TYPE][0], &offset_units);
				nr_offset_addr = offset_units * sizeof(struct sensor_yuv_precdn_level) * SENSOR_SMART_LEVEL_NUM;
				memcpy(((uint8_t*)(nr_update_param.yuv_precdn_level_ptr)) + nr_offset_addr + precdn_ptr_offset, (uint8_t*)data_actual_ptr, data_actual_len);
				if (0x01 != data_head->packet_status)
					precdn_ptr_offset += data_actual_len;
				else
					precdn_ptr_offset = 0;
				nr_tool_flag[9] = 1;
				break;
			}
			default :
				break;
	}
	return ret;
}

int denoise_param_send(uint8_t *tx_buf, uint32_t valid_len, void *src_ptr, uint32_t src_size,
								uint8_t *data_ptr, unsigned short *actual_len, unsigned char *data_status)
{
	int ret = ISP_SUCCESS;
	int i, num;
	uint32_t tail_len;
	if (NULL == tx_buf || NULL == src_ptr || NULL == data_ptr || NULL == actual_len) {
		ISP_LOGE("param error:tx_buf:%p, src_ptr:%p, data_ptr:%p, actual_len:%p", tx_buf, src_ptr, data_ptr, actual_len);
		return ISP_PARAM_ERROR;
	}

	num = src_size / valid_len;
	tail_len = src_size % valid_len;

	if (0 != num) {
		for (i = 0; i < num; i++) {
			memcpy(data_ptr, (uint8_t*)src_ptr + i * valid_len, valid_len);
			*actual_len = sizeof(MSG_HEAD_T) + sizeof(struct isp_data_header_normal) + valid_len;
			*(tx_buf + (*actual_len) + 1) = 0x7e;
			if ((i == (num - 1)) && (0 == tail_len))
				*data_status = 0x01;
			else
				*data_status = 0x00;

			ret = send(sockfd, tx_buf, (*actual_len) + 2, 0);
		}
	}
	if (0 != tail_len) {
		memcpy(data_ptr, (uint8_t*)src_ptr + num * valid_len, tail_len);
		*actual_len = sizeof(MSG_HEAD_T) + sizeof(struct isp_data_header_normal) + tail_len;
		*(tx_buf + (*actual_len) + 1) = 0x7e;
		*data_status = 0x01;
		ret = send(sockfd, tx_buf, (*actual_len) + 2, 0);
	}
	ISP_LOGI("denoise send over! ret = %d", ret);
	return ret;
}

int isp_denoise_read(uint8_t *tx_buf, uint32_t len, struct isp_data_header_read *data_head)
{
	int ret = ISP_SUCCESS;
	int num;
	MSG_HEAD_T *msg_ret;
	struct isp_data_header_normal *data_head_ptr;
	uint8_t *data_ptr;
	uint32_t data_valid_len;
	uint32_t src_size = 0;
	int file_num = 0;
	uint8_t isp_mode = 0;
	uint8_t nr_mode = 0;

	if (NULL == tx_buf) {
		ISP_LOGE("param error:tx_buf:%p",tx_buf);
		return ISP_PARAM_ERROR;
	}

	msg_ret = (MSG_HEAD_T *)(tx_buf + 1);
	data_head_ptr = (struct isp_data_header_normal *)(tx_buf + sizeof(MSG_HEAD_T) + 1);
	data_head_ptr->main_type= 0x01;     //denoise param
	data_ptr = ((uint8_t*)data_head_ptr) + sizeof(struct isp_data_header_normal);
	data_valid_len = len - 2 - sizeof(MSG_HEAD_T) - sizeof(struct isp_data_header_normal);
	file_num = data_head->sub_type;
	isp_u32 offset_units = 0;
	void * nr_offset_addr = PNULL;
	//ISP_LOGE("nr_update_param.multi_nr_flag:%x, isp_mode=%d,nr_mode=%d,msg_ret->len=%d",nr_update_param.multi_nr_flag,
	//							data_head->isp_mode, data_head->nr_mode,msg_ret->len);
	if(nr_update_param.multi_nr_flag != SENSOR_MULTI_MODE_FLAG) {
		isp_mode = 0;
		nr_mode = 0;
	} else {
		isp_mode = data_head->isp_mode;
		nr_mode = data_head->nr_mode;
	}
		switch(file_num) {
			case BDN :
			{
				data_head_ptr->sub_type = BDN;
				src_size = sizeof(struct sensor_bdn_level) * SENSOR_SMART_LEVEL_NUM;
			isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_BL_NR_TYPE][0], &offset_units);
			nr_offset_addr = (uint8_t *)nr_update_param.bdn_level_ptr + offset_units * src_size;
				break;
			}
			case BPC :
			{
				data_head_ptr->sub_type = BPC;
				src_size = sizeof(struct sensor_bpc_level) * SENSOR_SMART_LEVEL_NUM;
			isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_BPC_TYPE][0], &offset_units);
			nr_offset_addr = (uint8_t *)nr_update_param.bpc_level_ptr + offset_units * src_size;
				break;
			}
			case CCE_UV :
			{
				data_head_ptr->sub_type = CCE_UV;
				src_size = sizeof(struct sensor_cce_uvdiv_level) * SENSOR_SMART_LEVEL_NUM;
			isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_UVDIV_TYPE][0], &offset_units);
			nr_offset_addr = (uint8_t *)nr_update_param.cce_uvdiv_level_ptr + offset_units * src_size;
				break;
			}
			case CFAE :
			{
				data_head_ptr->sub_type = CFAE;
				src_size = sizeof(struct sensor_cfae_level) * SENSOR_SMART_LEVEL_NUM;
			isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_CFA_TYPE][0], &offset_units);
			nr_offset_addr = (uint8_t *)nr_update_param.cfae_level_ptr + offset_units * src_size;
				break;
			}
			case EDGE :
			{
				data_head_ptr->sub_type = EDGE;
				src_size = sizeof(struct sensor_ee_level) * SENSOR_SMART_LEVEL_NUM;
			isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_EDGE_TYPE][0], &offset_units);
			nr_offset_addr = (uint8_t *)nr_update_param.ee_level_ptr+ offset_units * src_size;
				break;

			}
			case FLAT_OFFSET :
			{
				data_head_ptr->sub_type = FLAT_OFFSET;
				src_size = sizeof(struct sensor_flat_offset_level) * SENSOR_SMART_LEVEL_NUM;
			isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_NLM_TYPE][0], &offset_units);
			nr_offset_addr = (uint8_t *)nr_update_param.flat_offset_level_ptr + offset_units * src_size;
				break;
			}
			case GRGB :
			{
				data_head_ptr->sub_type = GRGB;
				src_size = sizeof(struct sensor_grgb_v1_level) * SENSOR_SMART_LEVEL_NUM;
			isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_GRGB_TYPE][0], &offset_units);
			nr_offset_addr = (uint8_t *)nr_update_param.grgb_v1_level_ptr + offset_units * src_size;
				break;
			}
			case IIR_YRANDOM :
			{
				data_head_ptr->sub_type = IIR_YRANDOM;
				src_size = sizeof(struct sensor_iircnr_yrandom_level) * SENSOR_SMART_LEVEL_NUM;
			isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_IIRCNR_YRANDOM_TYPE][0], &offset_units);
			nr_offset_addr = (uint8_t *)nr_update_param.iircnr_yrandom_level_ptr + offset_units * src_size;
				break;
			}
			case IIRCNR :
			{
				data_head_ptr->sub_type = IIRCNR;
				src_size = sizeof(struct sensor_iircnr_level) * SENSOR_SMART_LEVEL_NUM;
			isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_IIRCNR_IIR_TYPE][0], &offset_units);
			nr_offset_addr = (uint8_t *)nr_update_param.iircnr_level_ptr + offset_units * src_size;
				break;
			}
			case IVST :
			{
				data_head_ptr->sub_type = IVST;
				src_size = sizeof(struct sensor_ivst_level) * SENSOR_SMART_LEVEL_NUM;
			isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_NLM_TYPE][0], &offset_units);
			nr_offset_addr = (uint8_t *)nr_update_param.ivst_level_ptr + offset_units * src_size;
				break;
			}
			case NLM :
			{
				data_head_ptr->sub_type = NLM;
				src_size = sizeof(struct sensor_nlm_level) * SENSOR_SMART_LEVEL_NUM;
			isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_NLM_TYPE][0], &offset_units);
			nr_offset_addr = (uint8_t *)nr_update_param.nlm_level_ptr + offset_units * src_size;
				break;
			}
			case PRFY :
			{
				data_head_ptr->sub_type = PRFY;
				src_size = sizeof(struct sensor_prfy_level) * SENSOR_SMART_LEVEL_NUM;
			isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_PREF_TYPE][0], &offset_units);
			nr_offset_addr = (uint8_t *)nr_update_param.prfy_level_ptr + offset_units * src_size;
				break;
			}
			case PWD :
			{
				data_head_ptr->sub_type = PWD;
				src_size = sizeof(struct sensor_pwd_level) * SENSOR_SMART_LEVEL_NUM;
			isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_PRE_WAVELET_TYPE][0], &offset_units);
			nr_offset_addr = (uint8_t *)nr_update_param.pwd_level_ptr + offset_units * src_size;
				break;
			}
			case RGB_PRECDN :
			{
				data_head_ptr->sub_type = RGB_PRECDN;
				src_size = sizeof(struct sensor_rgb_precdn_level) * SENSOR_SMART_LEVEL_NUM;
			isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_RGB_PRECDN_TYPE][0], &offset_units);
			nr_offset_addr = (uint8_t *)nr_update_param.rgb_precdn_level_ptr + offset_units * src_size;
				break;
			}
			case VST :
			{
				data_head_ptr->sub_type = VST;
				src_size = sizeof(struct sensor_vst_level) * SENSOR_SMART_LEVEL_NUM;
			isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_NLM_TYPE][0], &offset_units);
			nr_offset_addr = (uint8_t *)nr_update_param.vst_level_ptr + offset_units * src_size;
				break;
			}
			case Y_AFM :
			{
				data_head_ptr->sub_type = Y_AFM;
				src_size = sizeof(struct sensor_y_afm_level) * SENSOR_SMART_LEVEL_NUM;
			isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_YIQ_AFM_TYPE][0], &offset_units);
			nr_offset_addr = (uint8_t *)nr_update_param.y_afm_level_ptr + offset_units * src_size;
				break;
			}
			case YUV_CDN :
			{
				data_head_ptr->sub_type = YUV_CDN;
				src_size = sizeof(struct sensor_uv_cdn_level) * SENSOR_SMART_LEVEL_NUM;
			isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_UV_CDN_TYPE][0], &offset_units);
			nr_offset_addr = (uint8_t *)nr_update_param.uv_cdn_level_ptr + offset_units * src_size;

				break;
			}
			case YUV_POSTCDN :
			{
				data_head_ptr->sub_type = YUV_POSTCDN;
				src_size = sizeof(struct sensor_uv_postcdn_level) * SENSOR_SMART_LEVEL_NUM;
			isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_UV_POSTCDN_TYPE][0], &offset_units);
			nr_offset_addr = (uint8_t *)nr_update_param.uv_postcdn_level_ptr + offset_units * src_size;
				break;
			}
			case YUV_PRECDN :
			{
				data_head_ptr->sub_type = YUV_PRECDN;
				src_size = sizeof(struct sensor_yuv_precdn_level) * SENSOR_SMART_LEVEL_NUM;
			isp_tool_calc_nr_addr_offset(isp_mode, nr_mode, (isp_u16 *)&multi_mode[ISP_BLK_YUV_PRECDN_TYPE][0], &offset_units);
			nr_offset_addr = (uint8_t *)nr_update_param.yuv_precdn_level_ptr + offset_units * src_size;
				break;
			}
			default :
				break;
		}
		denoise_param_send(tx_buf, data_valid_len, (void*)nr_offset_addr, src_size, data_ptr,
							&msg_ret->len, &data_head_ptr->packet_status);
	return ret;
}

int isp_dump_reg(isp_handle handler, uint8_t *tx_buf, uint32_t len)
{
	int ret = ISP_SUCCESS;
	struct isp_info param_info;
	MSG_HEAD_T *msg_ret;
	struct isp_data_header_normal *data_head_ptr;
	uint8_t *data_ptr;
	uint32_t data_valid_len;
	uint32_t src_size;
	void *src_ptr;
	uint32_t num, i;
	uint32_t tail_len;

	if (NULL == tx_buf || NULL == handler) {
			ISP_LOGE("param error");
			return ISP_PARAM_ERROR;
	}

	msg_ret = (MSG_HEAD_T *)(tx_buf + 1);
	data_head_ptr = (struct isp_data_header_normal *)(tx_buf + sizeof(MSG_HEAD_T) + 1);
	data_head_ptr->main_type = 0x00;     //dump isp reg
	data_ptr = ((uint8_t *)data_head_ptr) + sizeof(struct isp_data_header_normal);
	data_valid_len = len - 2 - sizeof(MSG_HEAD_T) - sizeof(struct isp_data_header_normal);

	ret = isp_ioctl(handler, ISP_CTRL_DUMP_REG,(void*)&param_info);

	src_ptr = param_info.addr;
	src_size = param_info.size;

	num = src_size / data_valid_len;
	tail_len = src_size % data_valid_len;

	for (i = 0; i < num; i++) {
		memcpy(data_ptr, (uint8_t*)src_ptr + i * data_valid_len, data_valid_len);
		msg_ret->len = sizeof(MSG_HEAD_T) + sizeof(struct isp_data_header_normal) + data_valid_len;
		*(tx_buf + (msg_ret->len) + 1) = 0x7e;
		if (0 == tail_len && i == (num-1))
			data_head_ptr->packet_status= 0x01; //last one
		else
			data_head_ptr->packet_status = 0x00;

		ret = send(sockfd, tx_buf, (msg_ret->len) + 2, 0);
	}
	if (0 != tail_len) {
		memcpy(data_ptr, (uint8_t*)src_ptr + num * data_valid_len, tail_len);
		msg_ret->len = sizeof(MSG_HEAD_T) + sizeof(struct isp_data_header_normal) + tail_len;
		*(tx_buf + (msg_ret->len) + 1) = 0x7e;
		data_head_ptr->packet_status = 0x01;  //last one
		ret = send(sockfd, tx_buf, (msg_ret->len) + 2, 0);
	}
	if(NULL != src_ptr) {
		free(src_ptr);
		src_ptr = NULL;
	}
	ISP_LOGI("dump over! ret=%d", ret);
	return ret;
}

static int isp_nr_reg_read(isp_handle handler, uint8_t *tx_buf, uint32_t len, uint8_t *rx_buf)
{
	int ret = ISP_SUCCESS;
	struct isp_data_header_read *data_head = (struct isp_data_header_read *)(rx_buf + sizeof(MSG_HEAD_T) + 1);
	if (0x01 == data_head->main_type) {
		ret = isp_denoise_read(tx_buf, len, data_head);
		ISP_LOGI("denoise_read over");
		return ret;
	}
	if (0x00 == data_head->main_type) {
		ret = isp_dump_reg(handler, tx_buf, len);
		ISP_LOGI("isp_dump_reg  over");
		return ret;
	}
	return ret;
}

static int isp_nr_write(isp_handle handler, uint8_t *tx_buf, uint8_t *rx_buf)
{
	int ret = ISP_SUCCESS;
	MSG_HEAD_T *msg, *msg_ret;
	int rsp_len = 0;

	msg = (MSG_HEAD_T *)(rx_buf + 1);
	msg_ret = (MSG_HEAD_T *)(tx_buf + 1);
	unsigned int data_len = msg->len - 8;
	struct isp_data_header_normal *data_head = (struct isp_data_header_normal *)(rx_buf + sizeof(MSG_HEAD_T) + 1);
	rsp_len = sizeof(MSG_HEAD_T) + 1;

	if (0x01 == data_head->main_type) {
		ret = isp_denoise_write((uint8_t*)data_head, &data_len);
		if (0x01 == data_head->packet_status) {
			if (ISP_SUCCESS == ret) {
				rsp_len += ispvideo_SetreTurnValue(tx_buf + rsp_len, ISP_SUCCESS);
				*(tx_buf + rsp_len) = 0x7e;
				msg_ret->len = rsp_len - 1;
				send(sockfd, tx_buf, rsp_len + 1, 0);
			}else {
				rsp_len+=ispvideo_SetreTurnValue(tx_buf + rsp_len, 0x01);
				*(tx_buf + rsp_len) = 0x7e;
				msg_ret->len = rsp_len - 1;
				send(sockfd, tx_buf, rsp_len + 1, 0);
			}
		}
	}
	ISP_LOGE("NR write over! ret = %d", ret);
	return ret;
}


uint32_t get_tune_packet_num(uint32_t packet_len)
{
	uint32_t packet_num = 0;
	uint32_t len_tmp = 0;
	len_tmp = sizeof(struct sensor_fix_param_mode_info);
	if (0 != ((packet_len - len_tmp)%(DATA_BUF_SIZE - PACKET_MSG_HEADER - DATA_CMD_LENGTH -len_tmp - 2))) {
		packet_num = (packet_len - len_tmp)/(DATA_BUF_SIZE - PACKET_MSG_HEADER - DATA_CMD_LENGTH - len_tmp - 2) + 1;
	}else {
		packet_num = (packet_len - len_tmp)/(DATA_BUF_SIZE - PACKET_MSG_HEADER - DATA_CMD_LENGTH - len_tmp - 2);
	}
	return packet_num;
}

uint32_t get_fix_packet_num(uint32_t packet_len)
{
	uint32_t packet_num = 0;
	uint32_t len_tmp = sizeof(struct sensor_fix_param_mode_info) + sizeof(struct sensor_fix_param_block_info);
	if (0 != ((packet_len - len_tmp)%(DATA_BUF_SIZE - PACKET_MSG_HEADER - DATA_CMD_LENGTH - len_tmp - 2))) {
		packet_num = (packet_len - len_tmp)/(DATA_BUF_SIZE - PACKET_MSG_HEADER - DATA_CMD_LENGTH -len_tmp - 2) + 1;
	}else {
		packet_num = (packet_len - len_tmp)/(DATA_BUF_SIZE - PACKET_MSG_HEADER - DATA_CMD_LENGTH -len_tmp - 2);
	}
	return packet_num;
}

uint32_t get_note_packet_num(uint32_t packet_len)
{
	uint32_t packet_num = 0;
	if (0 != (packet_len%(DATA_BUF_SIZE - PACKET_MSG_HEADER - DATA_CMD_LENGTH - 2))) {
		packet_num = packet_len/(DATA_BUF_SIZE - PACKET_MSG_HEADER - DATA_CMD_LENGTH - 2) + 1;
	}else {
		packet_num = packet_len/(DATA_BUF_SIZE - PACKET_MSG_HEADER - DATA_CMD_LENGTH - 2);
	}
	return packet_num;
}

uint32_t get_libuse_info_packet_num(uint32_t packet_len)
{
	uint32_t packet_num = 0;
	if (0 != (packet_len%(DATA_BUF_SIZE - PACKET_MSG_HEADER - DATA_CMD_LENGTH - 2))) {
		packet_num = packet_len/(DATA_BUF_SIZE - PACKET_MSG_HEADER - DATA_CMD_LENGTH - 2) + 1;
	}else {
		packet_num = packet_len/(DATA_BUF_SIZE - PACKET_MSG_HEADER - DATA_CMD_LENGTH - 2);
	}
	return packet_num;
}

int send_isp_mode_id_param(struct isp_data_header_read *read_cmd,struct msg_head_tag *msg,uint32_t *data_addr,uint32_t data_len)
{
	int rtn = 0x00;

	uint32_t i ;
	int res;
	uint32_t len = 0;
	uint32_t rsp_len = 0;

	uint32_t len_msg = sizeof(struct msg_head_tag);
	uint32_t len_data_header = sizeof(struct isp_data_header_normal);
	uint32_t len_mode_info = sizeof(struct sensor_fix_param_mode_info);

	uint8_t *data_ptr = NULL;
	struct sensor_fix_param_mode_info mode_param = {0};
	struct isp_data_header_normal data_header = {0};
	struct msg_head_tag msg_tag = {0};

	data_header.isp_mode = read_cmd->isp_mode;
	data_header.main_type = read_cmd->main_type;
	data_header.sub_type = read_cmd->sub_type;
	data_header.data_total_len = data_len;
	msg_tag.seq_num = msg->seq_num;
	msg_tag.subtype = msg->subtype;
	msg_tag.type = msg->type;

	len = data_len+len_msg + len_data_header ;
	msg_tag.len = len;
	data_header.packet_status = 0x01;

	memcpy(eng_rsp_diag+1,(uint8_t *)&msg_tag,len_msg);
	rsp_len = len_msg+1;
	memcpy(eng_rsp_diag + rsp_len,(uint8_t *)&data_header,len_data_header);
	rsp_len = rsp_len + len_data_header;

	memcpy(eng_rsp_diag + rsp_len,(uint8_t *)data_addr,data_len);
	rsp_len = len + 1;
	eng_rsp_diag[rsp_len] = 0x7e;
	res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
	return rtn;
}


int send_tune_info_param(struct isp_data_header_read *read_cmd,struct msg_head_tag *msg,uint32_t *data_addr,uint32_t data_len)
{
	int rtn = 0x00;

	uint32_t packet_num = 0;
	uint32_t i ;
	int res;
	uint32_t len = 0;

	uint32_t len_msg = sizeof(struct msg_head_tag);
	uint32_t len_data_header = sizeof(struct isp_data_header_normal);
	uint32_t len_mode_info = sizeof(struct sensor_fix_param_mode_info);

	uint8_t *data_ptr = NULL;
	struct sensor_fix_param_mode_info mode_param = {0};
	struct isp_data_header_normal data_header = {0};
	struct msg_head_tag msg_tag = {0};

	data_header.isp_mode = read_cmd->isp_mode;
	data_header.main_type = read_cmd->main_type;
	data_header.sub_type = read_cmd->sub_type;
	data_header.data_total_len = data_len;
	msg_tag.seq_num = msg->seq_num;
	msg_tag.subtype = msg->subtype;
	msg_tag.type = msg->type;

	packet_num = get_tune_packet_num(data_len);
	memcpy((void *)&mode_param,(void *)data_addr,len_mode_info);
	data_ptr = (uint8_t *)((uint8_t *)data_addr + len_mode_info);

	for (i = 0; i < packet_num; i++) {
		uint32_t rsp_len = 0;
		if (i < (packet_num-1)) {
			len = DATA_BUF_SIZE -2 ;
			msg_tag.len = len;
			data_header.packet_status = 0x00;
		}else {
			if (0 != i) {
				len = data_len + packet_num*(len_msg + len_data_header + 2) -DATA_BUF_SIZE*i-2;
			}else {
				len = (data_len - len_mode_info) + packet_num*(len_msg + len_data_header + len_mode_info + 2) -DATA_BUF_SIZE*i-2;
			}
			msg_tag.len = len;
			data_header.packet_status = 0x01;
		}
		memcpy(eng_rsp_diag+1,(uint8_t *)&msg_tag,len_msg);
		rsp_len = len_msg+1;
		memcpy(eng_rsp_diag + rsp_len,(uint8_t *)&data_header,len_data_header);
		rsp_len = rsp_len + len_data_header;
		if (0 == i) {
			memcpy(eng_rsp_diag + rsp_len,(uint8_t *)&mode_param,len_mode_info);
			rsp_len = rsp_len + len_mode_info;
			memcpy(eng_rsp_diag + rsp_len,(uint8_t *)data_ptr,len - len_msg - len_data_header - len_mode_info);
			data_ptr = (uint8_t *)data_ptr + len - len_msg - len_data_header - len_mode_info;
		}else {
			memcpy(eng_rsp_diag + rsp_len,(uint8_t *)data_ptr,len - len_msg - len_data_header);
			data_ptr = (uint8_t *)data_ptr + len - len_msg - len_data_header ;
		}
		rsp_len = len + 1;
		eng_rsp_diag[rsp_len] = 0x7e;
		res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
	}
	return rtn;
}

int send_fix_param(struct isp_data_header_read *read_cmd,struct msg_head_tag *msg,uint32_t *data_addr,uint32_t data_len)
{
	int rtn = 0x00;

	uint32_t packet_num = 0;
	uint32_t i ;
	int res;
	uint32_t len = 0;
	uint32_t rsp_len = 0;
	uint8_t *data_ptr = NULL;

	uint32_t len_msg = sizeof(struct msg_head_tag);
	uint32_t len_data_header = sizeof(struct isp_data_header_normal);
	uint32_t len_mode_info = sizeof(struct sensor_fix_param_mode_info);
	uint32_t len_block_info = sizeof(struct sensor_fix_param_block_info);

	struct sensor_fix_param_mode_info mode_param = {0};
	struct sensor_fix_param_block_info block_param = {0};
	struct isp_data_header_normal data_header = {0};
	struct msg_head_tag msg_tag = {0};

	data_header.isp_mode = read_cmd->isp_mode;
	data_header.main_type = read_cmd->main_type;
	data_header.sub_type = read_cmd->sub_type;
	data_header.data_total_len = data_len;
	msg_tag.seq_num = msg->seq_num;
	msg_tag.subtype = msg->subtype;
	msg_tag.type = msg->type;

	packet_num = get_fix_packet_num(data_len);
	memcpy((void *)&mode_param,(void *)data_addr,len_mode_info);
	data_ptr = (uint8_t *)((uint8_t *)data_addr + len_mode_info);
	memcpy((void *)&block_param,(void *)data_ptr,len_block_info);
	data_ptr = data_ptr + len_block_info;

	for (i=0; i<packet_num; i++) {
		uint32_t tmp = 0;
		if (i < (packet_num-1)) {
			len = DATA_BUF_SIZE -2 ;
			msg_tag.len = len;
			data_header.packet_status = 0x00;
		}else {
			if (0 != i) {
				len = data_len + packet_num*(len_msg + len_data_header + 2) -DATA_BUF_SIZE*i -2;
			}else {
				len = data_len + len_msg + len_data_header;
			}
			msg_tag.len = len;
			data_header.packet_status = 0x01;
		}
		memcpy(eng_rsp_diag+1,(uint8_t *)&msg_tag,len_msg);
		rsp_len = len_msg+1;
		memcpy(eng_rsp_diag + rsp_len,(uint8_t *)&data_header,len_data_header);
		rsp_len = rsp_len + len_data_header;
		if (0 == i) {
			memcpy(eng_rsp_diag + rsp_len,(uint8_t *)&mode_param,len_mode_info);
			rsp_len = rsp_len + len_mode_info;
			memcpy(eng_rsp_diag + rsp_len,(uint8_t *)&block_param,len_block_info);
			rsp_len = rsp_len + len_block_info;
			memcpy(eng_rsp_diag + rsp_len,(uint8_t *)data_ptr,len - len_msg - len_data_header - len_mode_info -len_block_info);
			data_ptr = (uint8_t *)data_ptr + len - len_msg - len_data_header - len_mode_info - len_block_info;
		}else {
			memcpy(eng_rsp_diag + rsp_len,(uint8_t *)data_ptr,len - len_msg - len_data_header);
			data_ptr = (uint8_t *)data_ptr + len - len_msg - len_data_header ;
		}
		rsp_len = len + 1;
		eng_rsp_diag[rsp_len] = 0x7e;
		res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
	}
	return rtn;
}

int send_note_param(struct isp_data_header_read *read_cmd,struct msg_head_tag *msg,uint32_t *data_addr,uint32_t data_len)
{
	int rtn = 0x00;

	uint32_t packet_num = 0;
	uint32_t i ;
	int res;
	uint32_t len = 0;

	uint32_t len_msg = sizeof(struct msg_head_tag);
	uint32_t len_data_header = sizeof(struct isp_data_header_normal);

	uint8_t *data_ptr = (uint8_t *)data_addr;
	struct isp_data_header_normal data_header = {0};
	struct msg_head_tag msg_tag = {0};

	data_header.isp_mode = read_cmd->isp_mode;
	data_header.main_type = read_cmd->main_type;
	data_header.sub_type = read_cmd->sub_type;
	msg_tag.seq_num = msg->seq_num;
	msg_tag.subtype = msg->subtype;
	msg_tag.type = msg->type;

	packet_num = get_note_packet_num(data_len);
	for (i=0; i<packet_num; i++) {
		uint32_t rsp_len = 0;
		if (i < (packet_num-1)) {
			len = DATA_BUF_SIZE -2 ;
			msg_tag.len = len;
			data_header.packet_status = 0x00;
		}else {
			len = data_len + packet_num*(len_msg + len_data_header + 2) -DATA_BUF_SIZE*i-2;
			msg_tag.len = len;
			data_header.packet_status = 0x01;
		}
		memcpy(eng_rsp_diag+1,(uint8_t *)&msg_tag,len_msg);
		rsp_len = len_msg+1;
		memcpy(eng_rsp_diag + rsp_len,(uint8_t *)&data_header,len_data_header);
		rsp_len = rsp_len + len_data_header;
		memcpy(eng_rsp_diag + rsp_len,(uint8_t *)data_ptr,len - len_msg - len_data_header);
		data_ptr = (uint8_t *)data_ptr + len - len_msg - len_data_header ;
		rsp_len = len + 1;
		eng_rsp_diag[rsp_len] = 0x7e;
		res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
	}
	return rtn;
}

int send_libuse_info_param(struct isp_data_header_read *read_cmd, struct msg_head_tag *msg,
		uint32_t *data_addr, uint32_t data_len)
{
	int rtn = 0x0;

	uint32_t packet_num = 0;
	uint32_t i ;
	int res;
	uint32_t len = 0;

	uint32_t len_msg = sizeof(struct msg_head_tag);
	uint32_t len_data_header = sizeof(struct isp_data_header_normal);

	uint8_t *data_ptr = (uint8_t *)data_addr;
	struct isp_data_header_normal data_header = {0};
	struct msg_head_tag msg_tag = {0};

	data_header.isp_mode = read_cmd->isp_mode;
	data_header.main_type = read_cmd->main_type;
	data_header.sub_type = read_cmd->sub_type;
	msg_tag.seq_num = msg->seq_num;
	msg_tag.subtype = msg->subtype;
	msg_tag.type = msg->type;

	packet_num = get_libuse_info_packet_num(data_len);
	for (i=0; i<packet_num; i++) {
		uint32_t rsp_len = 0;
		if (i < (packet_num-1)) {
			len = DATA_BUF_SIZE -2 ;
			msg_tag.len = len;
			data_header.packet_status = 0x00;
		}else {
			len = data_len + packet_num*(len_msg + len_data_header + 2) -DATA_BUF_SIZE*i-2;
			msg_tag.len = len;
			data_header.packet_status = 0x01;
		}
		memcpy(eng_rsp_diag+1,(uint8_t *)&msg_tag,len_msg);
		rsp_len = len_msg+1;
		memcpy(eng_rsp_diag + rsp_len,(uint8_t *)&data_header,len_data_header);
		rsp_len = rsp_len + len_data_header;
		memcpy(eng_rsp_diag + rsp_len,(uint8_t *)data_ptr,len - len_msg - len_data_header);
		data_ptr = (uint8_t *)data_ptr + len - len_msg - len_data_header ;
		rsp_len = len + 1;
		eng_rsp_diag[rsp_len] = 0x7e;
		res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
	}
	return rtn;
}

int get_ae_table_param_length(struct sensor_raw_fix_info *sensor_raw_fix,uint16_t sub_type,uint32_t *data_len)
{
	int rtn = 0x00;

	uint8_t flicker = (sub_type >>4)& 0x0f;
	uint8_t iso = sub_type & 0x0f;

	if(NULL != data_len) {
		*data_len = *data_len + sensor_raw_fix->ae.ae_tab[flicker][iso].index_len;
		*data_len = *data_len + sensor_raw_fix->ae.ae_tab[flicker][iso].exposure_len;
		*data_len = *data_len + sensor_raw_fix->ae.ae_tab[flicker][iso].dummy_len;
		*data_len = *data_len + sensor_raw_fix->ae.ae_tab[flicker][iso].again_len;
		*data_len = *data_len + sensor_raw_fix->ae.ae_tab[flicker][iso].dgain_len;
	}
	return rtn;
}

int get_ae_table_param(struct sensor_raw_fix_info *sensor_raw_fix,uint16_t sub_type,uint32_t *data_addr)
{
	int rtn =0x00;

	uint32_t *tmp_ptr = NULL;
	uint8_t flicker = (sub_type >>4) & 0x0f;
	uint8_t iso = sub_type & 0x0f;

	if (NULL != data_addr ) {
		memcpy((void *)data_addr,(void *)sensor_raw_fix->ae.ae_tab[flicker][iso].index,sensor_raw_fix->ae.ae_tab[flicker][iso].index_len);
		tmp_ptr = (uint32_t *)((uint8_t *)data_addr + sensor_raw_fix->ae.ae_tab[flicker][iso].index_len);
		memcpy((void *)tmp_ptr,(void *)sensor_raw_fix->ae.ae_tab[flicker][iso].exposure,sensor_raw_fix->ae.ae_tab[flicker][iso].exposure_len);
		tmp_ptr = (uint32_t *)((uint8_t *)tmp_ptr + sensor_raw_fix->ae.ae_tab[flicker][iso].exposure_len);
		memcpy((void *)tmp_ptr,(void *)sensor_raw_fix->ae.ae_tab[flicker][iso].dummy,sensor_raw_fix->ae.ae_tab[flicker][iso].dummy_len);
		tmp_ptr = (uint32_t *)((uint8_t *)tmp_ptr + sensor_raw_fix->ae.ae_tab[flicker][iso].dummy_len);
		memcpy((void *)tmp_ptr,(void *)sensor_raw_fix->ae.ae_tab[flicker][iso].again,sensor_raw_fix->ae.ae_tab[flicker][iso].again_len);
		tmp_ptr = (uint32_t *)((uint8_t *)tmp_ptr + sensor_raw_fix->ae.ae_tab[flicker][iso].again_len);
		memcpy((void *)tmp_ptr,(void *)sensor_raw_fix->ae.ae_tab[flicker][iso].dgain,sensor_raw_fix->ae.ae_tab[flicker][iso].dgain_len);
		tmp_ptr = (uint32_t *)((uint8_t *)tmp_ptr + sensor_raw_fix->ae.ae_tab[flicker][iso].dgain_len);
	}
	return rtn;
}

int get_ae_weight_param_length(struct sensor_raw_fix_info *sensor_raw_fix,uint16_t sub_type,uint32_t *data_len)
{
	int rtn = 0x00;
	uint16_t weight = sub_type;
	*data_len = *data_len + sensor_raw_fix->ae.weight_tab[weight].len;
	return rtn;
}

int get_ae_weight_param(struct sensor_raw_fix_info *sensor_raw_fix,uint16_t sub_type,uint32_t *data_addr)
{
	int rtn = 0x00;
	uint16_t weight = sub_type;
	if (NULL != data_addr)
		memcpy((void *)data_addr,(void *)sensor_raw_fix->ae.weight_tab[weight].weight_table,sensor_raw_fix->ae.weight_tab[weight].len);
	return rtn;
}

int get_ae_scene_param_length(struct sensor_raw_fix_info *sensor_raw_fix,uint16_t sub_type,uint32_t *data_len)
{
	int rtn = 0x00;
	uint8_t scene = (sub_type>>4) & 0x0f;
	uint8_t flicker = sub_type & 0x0f;

	if (NULL != data_len) {
		*data_len = *data_len + sensor_raw_fix->ae.scene_tab[scene][flicker].scene_info_len;
		*data_len = *data_len + sensor_raw_fix->ae.scene_tab[scene][flicker].index_len;
		*data_len = *data_len + sensor_raw_fix->ae.scene_tab[scene][flicker].exposure_len;
		*data_len = *data_len + sensor_raw_fix->ae.scene_tab[scene][flicker].dummy_len;
		*data_len = *data_len + sensor_raw_fix->ae.scene_tab[scene][flicker].again_len;
		*data_len = *data_len + sensor_raw_fix->ae.scene_tab[scene][flicker].dgain_len;
	}
	return rtn;
}

int get_ae_scene_param(struct sensor_raw_fix_info *sensor_raw_fix,uint16_t sub_type,uint32_t *data_addr)
{
	int rtn =0x00;

	uint32_t *tmp_ptr = NULL;
	uint8_t scene = (sub_type>>4) & 0x0f;
	uint8_t flicker = sub_type & 0x0f;

	if (NULL != data_addr && flicker < 2) {
		memcpy(data_addr,sensor_raw_fix->ae.scene_tab[scene][flicker].scene_info,sensor_raw_fix->ae.scene_tab[scene][flicker].scene_info_len);
		tmp_ptr = (uint32_t *)((uint8_t *)data_addr + sensor_raw_fix->ae.scene_tab[scene][flicker].scene_info_len);
		memcpy(tmp_ptr,sensor_raw_fix->ae.scene_tab[scene][flicker].index,sensor_raw_fix->ae.scene_tab[scene][flicker].index_len);
		tmp_ptr = (uint32_t *)((uint8_t *)tmp_ptr + sensor_raw_fix->ae.scene_tab[scene][flicker].index_len);
		memcpy(tmp_ptr,sensor_raw_fix->ae.scene_tab[scene][flicker].exposure,sensor_raw_fix->ae.scene_tab[scene][flicker].exposure_len);
		tmp_ptr = (uint32_t *)((uint8_t *)tmp_ptr + sensor_raw_fix->ae.scene_tab[scene][flicker].exposure_len);
		memcpy(tmp_ptr,sensor_raw_fix->ae.scene_tab[scene][flicker].dummy,sensor_raw_fix->ae.scene_tab[scene][flicker].dummy_len);
		tmp_ptr = (uint32_t *)((uint8_t *)tmp_ptr + sensor_raw_fix->ae.scene_tab[scene][flicker].dummy_len);
		memcpy((void *)tmp_ptr,(void *)sensor_raw_fix->ae.scene_tab[scene][flicker].again,sensor_raw_fix->ae.scene_tab[scene][flicker].again_len);
		tmp_ptr = (uint32_t *)((uint8_t *)tmp_ptr + sensor_raw_fix->ae.scene_tab[scene][flicker].again_len);
		memcpy((void *)tmp_ptr,(void *)sensor_raw_fix->ae.scene_tab[scene][flicker].dgain,sensor_raw_fix->ae.scene_tab[scene][flicker].dgain_len);
		tmp_ptr = (uint32_t *)((uint8_t *)tmp_ptr + sensor_raw_fix->ae.scene_tab[scene][flicker].dgain_len);
	}
	return rtn;
}

int get_ae_auto_iso_param_length(struct sensor_raw_fix_info *sensor_raw_fix,uint16_t sub_type,uint32_t *data_len)
{
	int rtn = 0x00;
	uint8_t iso = sub_type;
	*data_len = *data_len + sensor_raw_fix->ae.auto_iso_tab[iso].len;
	return rtn;
}

int get_ae_auto_iso_param(struct sensor_raw_fix_info *sensor_raw_fix,uint16_t sub_type,uint32_t *data_addr)
{
	int rtn =0x00;

	uint8_t iso = sub_type;
	if (NULL != data_addr)
		memcpy((void *)data_addr,(void *)sensor_raw_fix->ae.auto_iso_tab[iso].auto_iso_tab,sensor_raw_fix->ae.auto_iso_tab[iso].len);

	return rtn;
}

int get_lnc_param_length(struct sensor_raw_fix_info *sensor_raw_fix,uint16_t sub_type,uint32_t *data_len)
{
	int rtn = 0x00;
	uint16_t lnc_ct =sub_type;
	*data_len = *data_len + sensor_raw_fix->lnc.map[lnc_ct].map_info_len;
	*data_len = *data_len + sensor_raw_fix->lnc.map[lnc_ct].lnc_len;
	return rtn;
}

int get_lnc_param(struct sensor_raw_fix_info *sensor_raw_fix,uint16_t sub_type,uint32_t *data_addr)
{
	int rtn =0x00;

	uint32_t *tmp_ptr = NULL;
	uint16_t lnc_ct = sub_type;
	if(NULL != data_addr) {
		memcpy((void *)data_addr,(void *)sensor_raw_fix->lnc.map[lnc_ct].map_info,sensor_raw_fix->lnc.map[lnc_ct].map_info_len);
		tmp_ptr = (uint32_t *)((uint8_t *)data_addr + sensor_raw_fix->lnc.map[lnc_ct].map_info_len);
		memcpy((void *)tmp_ptr,(void *)sensor_raw_fix->lnc.map[lnc_ct].lnc_addr,sensor_raw_fix->lnc.map[lnc_ct].lnc_len);
	}
	return rtn;
}

int get_awb_param_length(struct sensor_raw_fix_info *sensor_raw_fix,uint16_t sub_type,uint32_t *data_len)
{
	int rtn = 0x00;
	switch(sub_type) {
	case AWB_WIN_MAP:
		*data_len = *data_len + sensor_raw_fix->awb.awb_map.len;
		break;
	case AWB_WEIGHT:
		*data_len = *data_len + sensor_raw_fix->awb.awb_weight.weight_len + sensor_raw_fix->awb.awb_weight.size_param_len;
		break;
	default:
		break;
	}

	return rtn;
}

int get_awb_param(struct sensor_raw_fix_info *sensor_raw_fix,uint16_t sub_type,uint32_t *data_addr)
{
	int rtn =0x00;

	uint32_t *tmp_ptr = NULL;
	if (NULL != data_addr) {
		switch (sub_type) {
		case AWB_WIN_MAP:
		{
			memcpy((void *)data_addr,(void *)sensor_raw_fix->awb.awb_map.addr,sensor_raw_fix->awb.awb_map.len);
		}
		break;
		case AWB_WEIGHT:
		{
			memcpy((void *)data_addr,(void *)sensor_raw_fix->awb.awb_weight.addr,sensor_raw_fix->awb.awb_weight.weight_len);
			tmp_ptr = (uint32_t *)((uint8_t *)data_addr + sensor_raw_fix->awb.awb_weight.weight_len);
			memcpy((void *)tmp_ptr,(void *)sensor_raw_fix->awb.awb_weight.size,sensor_raw_fix->awb.awb_weight.size_param_len);
		}
		break;
		default:
			break;
		}
	}
	return rtn;
}


static int save_param_to_file(int32_t sn, uint32_t size, uint8_t *addr)
{
	int ret = 0;
	char file_name[40];
	char tmp_str[30];
	FILE *fp = NULL;
	uint32_t i = 0;
	int tmp_size = 0;
	int count = 0;

	ISP_LOGI("size %d", size);

	memset(file_name,0,sizeof(file_name));
	sprintf(file_name, "/data/misc/media/read_lnc_param_%d.txt", sn);

	ISP_LOGI("file name %s", file_name);
	fp = fopen(file_name, "wb");

	if (NULL == fp) {
		ISP_LOGI("can not open file: %s \n", file_name);
		return 0;
	}
	for (i = 0; i < size;++i) {
		count++;
		memset(tmp_str, 0, sizeof(tmp_str));
		sprintf(tmp_str,"0x%02x,", *addr++);
		if (16 == count) {
			strcat(tmp_str,"\n");
			count = 0;
		}

		tmp_size = strlen(tmp_str);
		fwrite((void*)tmp_str, 1, tmp_size, fp);
	}
	fclose(fp);
	return 0;
}


int send_isp_param(struct isp_data_header_read *read_cmd,struct msg_head_tag *msg)
{
	int rtn = 0x00;

	uint32_t data_len = 0;
	uint32_t *data_addr = NULL;
	uint32_t packet_num = 0;
	uint8_t mode_id = 0;

	struct sensor_raw_fix_info *sensor_raw_fix = NULL;
	struct isp_mode_param_info mode_param_info = {0};
	struct sensor_raw_note_info sensor_note_param ={0};

	SENSOR_EXP_INFO_T_PTR sensor_info_ptr = Sensor_GetInfo();
	struct sensor_raw_info* sensor_raw_info_ptr=(struct sensor_raw_info*)sensor_info_ptr->raw_info_ptr;

	if (0xff == read_cmd->isp_mode) {
		mode_id = 0;
	}else {
		mode_id = read_cmd->isp_mode;
	}
	if (NULL != sensor_raw_info_ptr->fix_ptr[mode_id] ) {
		sensor_raw_fix = sensor_raw_info_ptr->fix_ptr[mode_id];
	}
	if (NULL != sensor_raw_info_ptr->mode_ptr[mode_id].addr) {
		mode_param_info = sensor_raw_info_ptr->mode_ptr[mode_id];
	}
	if (NULL != sensor_raw_info_ptr->note_ptr[mode_id].note) {
		sensor_note_param = sensor_raw_info_ptr->note_ptr[mode_id];
	}

	switch (read_cmd->main_type) {
	case MODE_ISP_ID:
	{
		uint8_t i;
		uint8_t isp_mode_num = 0;
		uint8_t data_mode_id[ISP_READ_MODE_ID_MAX] = {0};

		struct isp_mode_param *mode_param = (struct isp_mode_param *)mode_param_info.addr;
		for (i = 0;i < ISP_READ_MODE_ID_MAX;i++) {
			if(NULL != sensor_raw_info_ptr->mode_ptr[i].addr) {
				data_mode_id[isp_mode_num] = i;
				isp_mode_num ++;
			}
		}
		data_len = isp_mode_num;
		data_addr = (uint32_t *)ispParserAlloc(data_len);
		memset((uint8_t *)data_addr,0x00,data_len);
		if (0 != data_len && NULL != data_addr) {
			memcpy((uint8_t *)data_addr,data_mode_id,isp_mode_num);
			rtn = send_isp_mode_id_param(read_cmd,msg,data_addr,data_len);
		}
	}
	break;
	case MODE_TUNE_INFO:
	{
		struct isp_mode_param *mode_param = (struct isp_mode_param *)mode_param_info.addr;
		data_len = mode_param_info.len + sizeof(struct sensor_fix_param_mode_info);
		data_addr = (uint32_t *)ispParserAlloc(data_len);
		memset((uint8_t *)data_addr,0x00,data_len);
		if (0 != data_len && NULL != data_addr && NULL != mode_param) {
			data_addr[0] = mode_param->version_id;
			data_addr[1] = mode_id;
			data_addr[2] = mode_param->width;
			data_addr[3] = mode_param->height;
			memcpy((void *)((uint8_t *)data_addr+sizeof(struct sensor_fix_param_mode_info)),(void *)mode_param_info.addr,mode_param_info.len);
			rtn = send_tune_info_param(read_cmd,msg,data_addr,data_len);
			if (0x00 == rtn) {
				DBG("ISP_TOOL : send tune_info is error");
			}
		}
	}
	break;
	case MODE_AE_TABLE:
	{
		uint32_t *data_addr_tmp = NULL;
		uint32_t *data_addr_tmp1 = NULL;
		int i;
		struct isp_mode_param *mode_param = (struct isp_mode_param *)mode_param_info.addr;
		if (NULL == sensor_raw_fix) {
			return rtn;
		}
		data_len = sizeof(struct sensor_fix_param_mode_info) + sensor_raw_fix->ae.block.len;
		rtn = get_ae_table_param_length(sensor_raw_fix,read_cmd->sub_type,&data_len);
		data_addr = (uint32_t *)ispParserAlloc(data_len);
		memset((uint8_t *)data_addr,0x00,data_len);
		if (0 != data_len && NULL != data_addr && NULL != mode_param) {
			data_addr[0] = mode_param->version_id;
			data_addr[1] = mode_id;
			data_addr[2] = mode_param->width;
			data_addr[3] = mode_param->height;
			data_addr_tmp = (uint32_t *)((uint8_t *)data_addr + sizeof(struct sensor_fix_param_mode_info));
			memcpy(data_addr_tmp, sensor_raw_fix->ae.block.block_info, sensor_raw_fix->ae.block.len);
			data_addr_tmp1 = (uint32_t *)((uint8_t *)data_addr_tmp + sizeof(struct sensor_fix_param_block_info));
			rtn = get_ae_table_param(sensor_raw_fix,read_cmd->sub_type,data_addr_tmp1);
			rtn = send_fix_param(read_cmd,msg,data_addr,data_len);
		}
	}
	break;
	case MODE_AE_WEIGHT_TABLE:
	{
		uint32_t *data_addr_tmp = NULL;
		uint32_t *data_addr_tmp1 = NULL;

		struct isp_mode_param *mode_param = (struct isp_mode_param *)mode_param_info.addr;
		if (NULL == sensor_raw_fix) {
			return rtn;
		}
		data_len = sizeof(struct sensor_fix_param_mode_info) + sensor_raw_fix->ae.block.len;
		rtn = get_ae_weight_param_length(sensor_raw_fix,read_cmd->sub_type,&data_len);
		data_addr = (uint32_t *)ispParserAlloc(data_len);
		memset((uint8_t *)data_addr,0x00,data_len);
		if (0 != data_len && NULL != data_addr && NULL != mode_param) {
			data_addr[0] = mode_param->version_id;
			data_addr[1] = mode_id;
			data_addr[2] = mode_param->width;
			data_addr[3] = mode_param->height;
			data_addr_tmp = (uint32_t *)((uint8_t *)data_addr + sizeof(struct sensor_fix_param_mode_info));
			memcpy(data_addr_tmp, sensor_raw_fix->ae.block.block_info, sensor_raw_fix->ae.block.len);
			data_addr_tmp1 = (uint32_t *)((uint8_t *)data_addr_tmp + sensor_raw_fix->ae.block.len);
			rtn = get_ae_weight_param(sensor_raw_fix,read_cmd->sub_type,data_addr_tmp1);
			rtn = send_fix_param(read_cmd,msg,data_addr,data_len);
		}
	}
	break;
	case MODE_AE_SCENE_TABLE:
	{
		uint32_t *data_addr_tmp = NULL;
		uint32_t *data_addr_tmp1 = NULL;

		struct isp_mode_param *mode_param = (struct isp_mode_param *)mode_param_info.addr;
		if (NULL == sensor_raw_fix) {
			return rtn;
		}
		data_len = sizeof(struct sensor_fix_param_mode_info) + sensor_raw_fix->ae.block.len;
		rtn = get_ae_scene_param_length(sensor_raw_fix,read_cmd->sub_type,&data_len);
		data_addr = (uint32_t *)ispParserAlloc(data_len);
		memset((uint8_t *)data_addr,0x00,data_len);
		if (0 != data_len && NULL != data_addr && NULL != mode_param) {
			data_addr[0] = mode_param->version_id;
			data_addr[1] = mode_id;
			data_addr[2] = mode_param->width;
			data_addr[3] = mode_param->height;
			data_addr_tmp = (uint32_t *)((uint8_t *)data_addr + sizeof(struct sensor_fix_param_mode_info));
			memcpy(data_addr_tmp, sensor_raw_fix->ae.block.block_info, sensor_raw_fix->ae.block.len);
			data_addr_tmp1 = (uint32_t *)((uint8_t *)data_addr_tmp + sensor_raw_fix->ae.block.len);
			rtn = get_ae_scene_param(sensor_raw_fix,read_cmd->sub_type,data_addr_tmp1);
			//rtn = save_param_to_file(mode_id,data_len -sizeof(struct sensor_fix_param_mode_info) - sensor_raw_fix->ae.block.len,data_addr_tmp1 );
			rtn = send_fix_param(read_cmd,msg,data_addr,data_len);
		}
	}
	break;
	case MODE_AE_AUTO_ISO_TABLE:
	{
		uint32_t *data_addr_tmp = NULL;
		uint32_t *data_addr_tmp1 = NULL;
		struct isp_mode_param *mode_param = (struct isp_mode_param *)mode_param_info.addr;
		if (NULL == sensor_raw_fix) {
			return rtn;
		}
		data_len = sizeof(struct sensor_fix_param_mode_info) + sensor_raw_fix->ae.block.len;
		rtn = get_ae_auto_iso_param_length(sensor_raw_fix,read_cmd->sub_type,&data_len);
		data_addr = (uint32_t *)ispParserAlloc(data_len);
		memset((uint8_t *)data_addr,0x00,data_len);
		if (0 != data_len && NULL != data_addr && NULL != mode_param) {
			data_addr[0] = mode_param->version_id;
			data_addr[1] = mode_id;
			data_addr[2] = mode_param->width;
			data_addr[3] = mode_param->height;
			data_addr_tmp = (uint32_t *)((uint8_t *)data_addr + sizeof(struct sensor_fix_param_mode_info));
			memcpy(data_addr_tmp, sensor_raw_fix->ae.block.block_info, sensor_raw_fix->ae.block.len);
			data_addr_tmp1 = (uint32_t *)((uint8_t *)data_addr_tmp + sensor_raw_fix->ae.block.len);
			rtn = get_ae_auto_iso_param(sensor_raw_fix,read_cmd->sub_type,data_addr_tmp1);
			rtn = send_fix_param(read_cmd,msg,data_addr,data_len);
		}
	}
	break;
	case MODE_LNC_DATA:
	{
		uint32_t *data_addr_tmp = NULL;
		uint32_t *data_addr_tmp1 = NULL;
		uint32_t value = 1;
		uint32_t value1 = 2;
		struct isp_mode_param *mode_param = (struct isp_mode_param *)mode_param_info.addr;
		if (NULL == sensor_raw_fix) {
			return rtn;
		}
		data_len = sizeof(struct sensor_fix_param_mode_info) + sensor_raw_fix->lnc.block.len;
		rtn = get_lnc_param_length(sensor_raw_fix,read_cmd->sub_type,&data_len);
		data_addr = (uint32_t *)ispParserAlloc(data_len);
		memset((uint8_t *)data_addr,0x00,data_len);
		if (0 != data_len && NULL != data_addr && NULL != mode_param) {
			data_addr[0] = mode_param->version_id;
			data_addr[1] = mode_id;
			data_addr[2] = mode_param->width;
			data_addr[3] = mode_param->height;
			data_addr_tmp = (uint32_t *)((uint8_t *)data_addr + sizeof(struct sensor_fix_param_mode_info));
			memcpy(data_addr_tmp, sensor_raw_fix->lnc.block.block_info, sensor_raw_fix->lnc.block.len);
			data_addr_tmp1 = (uint32_t *)((uint8_t *)data_addr_tmp + sensor_raw_fix->lnc.block.len);
			rtn = get_lnc_param(sensor_raw_fix,read_cmd->sub_type,data_addr_tmp1);
			rtn = send_fix_param(read_cmd,msg,data_addr,data_len);
		}
	}
	break;
	case MODE_AWB_DATA:
	{
		uint32_t *data_addr_tmp = NULL;
		uint32_t *data_addr_tmp1 = NULL;
		struct isp_mode_param *mode_param = (struct isp_mode_param *)mode_param_info.addr;
		if (NULL == sensor_raw_fix) {
			return rtn;
		}
		data_len = sizeof(struct sensor_fix_param_mode_info) + sensor_raw_fix->awb.block.len;
		rtn = get_awb_param_length(sensor_raw_fix,read_cmd->sub_type,&data_len);
		data_addr = (uint32_t *)ispParserAlloc(data_len);
		memset((uint8_t *)data_addr,0x00,data_len);
		if (0 != data_len && NULL != data_addr && NULL != mode_param) {
			data_addr[0] = mode_param->version_id;
			data_addr[1] = mode_id;
			data_addr[2] = mode_param->width;
			data_addr[3] = mode_param->height;
			data_addr_tmp = (uint32_t *)((uint8_t *)data_addr + sizeof(struct sensor_fix_param_mode_info));
			memcpy(data_addr_tmp, sensor_raw_fix->awb.block.block_info, sensor_raw_fix->awb.block.len);
			data_addr_tmp1 = (uint32_t *)((uint8_t *)data_addr_tmp + sensor_raw_fix->awb.block.len);
			rtn = get_awb_param(sensor_raw_fix,read_cmd->sub_type,data_addr_tmp1);
			rtn = send_fix_param(read_cmd,msg,data_addr,data_len);
		}
	}
	break;
	case MODE_NOTE_DATA:
	{
		data_len = sensor_note_param.node_len;
		data_addr = (uint32_t *)ispParserAlloc(data_len);
		memset((uint8_t *)data_addr,0x00,data_len);
		if (0 != data_len && NULL != data_addr) {
			memcpy((uint8_t *)data_addr,sensor_note_param.note,data_len);
			rtn = send_note_param(read_cmd,msg,data_addr,data_len);
		}
	}
	break;
	case MODE_LIB_INFO_DATA:
	{
		data_len = sizeof(struct sensor_libuse_info);
		data_addr = (uint32_t *)ispParserAlloc(data_len);
		memset((uint8_t *)data_addr,0x00,data_len);
		if (0 != data_len && NULL != data_addr) {
			memcpy((uint8_t *)data_addr, sensor_raw_info_ptr->libuse_info, data_len);
			rtn = send_libuse_info_param(read_cmd,msg,data_addr,data_len);
		}
	}
	break;
	default:
		break;
	}

	ispParserFree(data_addr);
	return rtn;
}

int down_ae_table_param(struct sensor_raw_fix_info *sensor_raw_fix,uint16_t sub_type,uint8_t *data_addr)
{
	int rtn = 0x00;
	uint8_t flicker = (sub_type>>4) & 0x0f;
	uint8_t iso = sub_type & 0x0f;
	uint32_t offset_tmp = 0;

	uint32_t len_mode_info = sizeof(struct sensor_fix_param_mode_info);
	uint32_t len_block_info = sizeof(struct sensor_fix_param_block_info);

	if (NULL == sensor_raw_fix || NULL == data_addr) {
		DBG("ISP_TOOL : get ae_table param error");
		rtn = 0x01;
		return rtn;
	}

	memcpy((void *)sensor_raw_fix->mode.mode_info,(void *)(data_addr + offset_tmp),len_mode_info);
	offset_tmp +=  len_mode_info;
	memcpy((void *)sensor_raw_fix->ae.block.block_info,(void *)(data_addr + offset_tmp),len_block_info);
	offset_tmp += len_block_info;
	memcpy((void *)sensor_raw_fix->ae.ae_tab[flicker][iso].index,(void *)(data_addr + offset_tmp),sensor_raw_fix->ae.ae_tab[flicker][iso].index_len);
	offset_tmp += sensor_raw_fix->ae.ae_tab[flicker][iso].index_len;
	memcpy((void *)sensor_raw_fix->ae.ae_tab[flicker][iso].exposure,(void *)(data_addr + offset_tmp),sensor_raw_fix->ae.ae_tab[flicker][iso].exposure_len);
	offset_tmp += sensor_raw_fix->ae.ae_tab[flicker][iso].exposure_len;
	memcpy((void *)sensor_raw_fix->ae.ae_tab[flicker][iso].dummy,(void *)(data_addr + offset_tmp),sensor_raw_fix->ae.ae_tab[flicker][iso].dummy_len);
	offset_tmp += sensor_raw_fix->ae.ae_tab[flicker][iso].dummy_len;
	memcpy((void *)sensor_raw_fix->ae.ae_tab[flicker][iso].again,(void *)(data_addr + offset_tmp),sensor_raw_fix->ae.ae_tab[flicker][iso].again_len);
	offset_tmp += sensor_raw_fix->ae.ae_tab[flicker][iso].again_len;
	memcpy((void *)sensor_raw_fix->ae.ae_tab[flicker][iso].dgain,(void *)(data_addr + offset_tmp),sensor_raw_fix->ae.ae_tab[flicker][iso].dgain_len);
	return rtn;
}

int down_ae_weight_param(struct sensor_raw_fix_info *sensor_raw_fix,uint16_t sub_type,uint8_t *data_addr)
{
	int rtn = 0x00;
	uint8_t weight = sub_type;
	uint32_t offset_tmp = 0;

	uint32_t len_mode_info = sizeof(struct sensor_fix_param_mode_info);
	uint32_t len_block_info = sizeof(struct sensor_fix_param_block_info);

	if (NULL == sensor_raw_fix || NULL == data_addr) {
		DBG("ISP_TOOL : get ae_weight param error");
		rtn = 0x01;
		return rtn;
	}

	memcpy((void *)sensor_raw_fix->mode.mode_info,(void *)(data_addr + offset_tmp),len_mode_info);
	offset_tmp +=  len_mode_info;
	memcpy((void *)sensor_raw_fix->ae.block.block_info,(void *)(data_addr + offset_tmp),len_block_info);
	offset_tmp += len_block_info;
	memcpy((void *)sensor_raw_fix->ae.weight_tab[weight].weight_table,(void *)(data_addr + offset_tmp),sensor_raw_fix->ae.weight_tab[weight].len);
	return rtn;
}

int down_ae_scene_param(struct sensor_raw_fix_info *sensor_raw_fix,uint16_t sub_type,uint8_t *data_addr)
{
	int rtn = 0x00;
	uint8_t scene = (sub_type>>4) & 0x0f;
	uint8_t flicker = sub_type & 0x0f;
	uint32_t offset_tmp = 0;

	uint32_t len_mode_info = sizeof(struct sensor_fix_param_mode_info);
	uint32_t len_block_info = sizeof(struct sensor_fix_param_block_info);

	if (NULL == sensor_raw_fix || NULL == data_addr) {
		DBG("ISP_TOOL : get ae_scene param error");
		rtn = 0x01;
		return rtn;
	}

	memcpy((void *)sensor_raw_fix->mode.mode_info,(void *)(data_addr + offset_tmp),len_mode_info);
	offset_tmp +=  len_mode_info;
	memcpy((void *)sensor_raw_fix->ae.block.block_info,(void *)(data_addr + offset_tmp),len_block_info);
	offset_tmp += len_block_info;
	memcpy((void *)sensor_raw_fix->ae.scene_tab[scene][flicker].scene_info,(void *)(data_addr + offset_tmp),sensor_raw_fix->ae.scene_tab[scene][flicker].scene_info_len);
	offset_tmp += sensor_raw_fix->ae.scene_tab[scene][flicker].scene_info_len;
	memcpy((void *)sensor_raw_fix->ae.scene_tab[scene][flicker].index,(void *)(data_addr + offset_tmp),sensor_raw_fix->ae.scene_tab[scene][flicker].index_len);
	offset_tmp += sensor_raw_fix->ae.scene_tab[scene][flicker].index_len;
	memcpy((void *)sensor_raw_fix->ae.scene_tab[scene][flicker].exposure,(void *)(data_addr + offset_tmp),sensor_raw_fix->ae.scene_tab[scene][flicker].exposure_len);
	offset_tmp += sensor_raw_fix->ae.scene_tab[scene][flicker].exposure_len;
	memcpy((void *)sensor_raw_fix->ae.scene_tab[scene][flicker].dummy,(void *)(data_addr + offset_tmp),sensor_raw_fix->ae.scene_tab[scene][flicker].dummy_len);
	offset_tmp += sensor_raw_fix->ae.scene_tab[scene][flicker].dummy_len;
	memcpy((void *)sensor_raw_fix->ae.scene_tab[scene][flicker].again,(void *)(data_addr + offset_tmp),sensor_raw_fix->ae.scene_tab[scene][flicker].again_len);
	offset_tmp += sensor_raw_fix->ae.scene_tab[scene][flicker].again_len;
	memcpy((void *)sensor_raw_fix->ae.scene_tab[scene][flicker].dgain,(void *)(data_addr + offset_tmp),sensor_raw_fix->ae.scene_tab[scene][flicker].dgain_len);
	return rtn;
}

int down_ae_auto_iso_param(struct sensor_raw_fix_info *sensor_raw_fix,uint16_t sub_type,uint8_t *data_addr)
{
	int rtn = 0x00;
	uint8_t iso = sub_type;
	uint32_t offset_tmp = 0;

	uint32_t len_mode_info = sizeof(struct sensor_fix_param_mode_info);
	uint32_t len_block_info = sizeof(struct sensor_fix_param_block_info);

	if (NULL == sensor_raw_fix || NULL == data_addr) {
		DBG("ISP_TOOL : get ae_table param error");
		rtn = 0x01;
		return rtn;
	}

	memcpy((void *)sensor_raw_fix->mode.mode_info,(void *)(data_addr + offset_tmp),len_mode_info);
	offset_tmp +=  len_mode_info;
	memcpy((void *)sensor_raw_fix->ae.block.block_info,(void *)(data_addr + offset_tmp),len_block_info);
	offset_tmp += len_block_info;
	memcpy((void *)sensor_raw_fix->ae.auto_iso_tab[iso].auto_iso_tab,(void *)(data_addr + offset_tmp),sensor_raw_fix->ae.auto_iso_tab[iso].len);
	return rtn;
}

int down_lnc_param(struct sensor_raw_fix_info *sensor_raw_fix,uint16_t sub_type,uint8_t *data_addr,uint32_t data_len)
{
	int rtn =0x00;

	uint32_t offset_tmp = (uint32_t)PNULL;
	uint16_t lnc_ct = sub_type;

	uint32_t len_mode_info = sizeof(struct sensor_fix_param_mode_info);
	uint32_t len_block_info = sizeof(struct sensor_fix_param_block_info);

	if (NULL == sensor_raw_fix || NULL == data_addr) {
		DBG("ISP_TOOL : get lnc param error");
		rtn = 0x01;
		return rtn;
	}

	memcpy((void *)sensor_raw_fix->mode.mode_info,(void *)(data_addr + offset_tmp),len_mode_info);
	offset_tmp +=  len_mode_info;
	memcpy((void *)sensor_raw_fix->lnc.block.block_info,(void *)(data_addr + offset_tmp),len_block_info);
	offset_tmp += len_block_info;
	memcpy((void *)sensor_raw_fix->lnc.map[lnc_ct].map_info,(void *)(data_addr + offset_tmp),sensor_raw_fix->lnc.map[lnc_ct].map_info_len);
	offset_tmp += sensor_raw_fix->lnc.map[lnc_ct].map_info_len;
	sensor_raw_fix->lnc.map[lnc_ct].lnc_addr = (uint16_t *)(data_addr +offset_tmp);
	sensor_raw_fix->lnc.map[lnc_ct].lnc_len = data_len - len_mode_info - len_block_info - sensor_raw_fix->lnc.map[lnc_ct].map_info_len;

	return rtn;
}

int down_awb_param(struct sensor_raw_fix_info *sensor_raw_fix,uint16_t sub_type,uint8_t *data_addr,uint32_t data_len)
{
	int rtn =0x00;

	uint32_t offset_tmp = (uint32_t)PNULL;

	uint32_t len_mode_info = sizeof(struct sensor_fix_param_mode_info);
	uint32_t len_block_info = sizeof(struct sensor_fix_param_block_info);

	if (NULL == sensor_raw_fix || NULL == data_addr) {
		DBG("ISP_TOOL : get lnc param error");
		rtn = 0x01;
		return rtn;
	}

	switch (sub_type) {
	case AWB_WIN_MAP:
	{
		memcpy((void *)sensor_raw_fix->mode.mode_info,(void *)((uint8_t *)data_addr + offset_tmp),len_mode_info);
		offset_tmp +=  len_mode_info;
		memcpy((void *)sensor_raw_fix->awb.block.block_info,(void *)((uint8_t *)data_addr + offset_tmp),len_block_info);
		offset_tmp += len_block_info;
		sensor_raw_fix->awb.awb_map.addr = (uint16_t *)(data_addr + offset_tmp);
		sensor_raw_fix->awb.awb_map.len = data_len -len_mode_info -len_block_info;
	}
	break;
	case AWB_WEIGHT:
	{
		memcpy((void *)sensor_raw_fix->mode.mode_info,(void *)((uint8_t *)data_addr + offset_tmp),len_mode_info);
		offset_tmp +=  len_mode_info;
		memcpy((void *)sensor_raw_fix->awb.block.block_info,(void *)((uint8_t *)data_addr + offset_tmp),len_block_info);
		offset_tmp += len_block_info;
		memcpy((void *)sensor_raw_fix->awb.awb_weight.addr,(void *)((uint8_t *)data_addr + offset_tmp),sensor_raw_fix->awb.awb_weight.weight_len);
		offset_tmp += sensor_raw_fix->awb.awb_weight.weight_len;
		memcpy((void *)sensor_raw_fix->awb.awb_weight.size,(void *)((uint8_t *)data_addr + offset_tmp),sensor_raw_fix->awb.awb_weight.size_param_len);
	}
	break;
	default:
		break;
	}
	return rtn;
}

int down_isp_param(isp_handle isp_handler,struct isp_data_header_normal *write_cmd,struct msg_head_tag *msg,uint8_t *isp_data_ptr,int value)
{
	int rtn = 0x00;

	static uint8_t *data_addr = NULL;
	static uint8_t *isp_data_addr = NULL;
	static uint32_t offset = 0;
	static uint32_t flag = 0;
	uint32_t data_len = 0;

	uint8_t mode_id = write_cmd->isp_mode;
	struct sensor_raw_fix_info *sensor_raw_fix = NULL;
	struct isp_mode_param_info mode_param_info = {0};
	struct sensor_raw_note_info sensor_note_param ={0};
	struct msg_head_tag *msg_ret = (struct msg_head_tag *)(eng_rsp_diag + 1);

	uint32_t len_msg = sizeof(struct msg_head_tag);
	uint32_t len_data_header = sizeof(struct isp_data_header_normal);
	uint32_t len_mode_info = sizeof(struct sensor_fix_param_mode_info);
	uint32_t len_block_info = sizeof(struct sensor_fix_param_block_info);

	SENSOR_EXP_INFO_T_PTR sensor_info_ptr = Sensor_GetInfo();
	struct sensor_raw_info* sensor_raw_info_ptr=(struct sensor_raw_info*)sensor_info_ptr->raw_info_ptr;
	if (NULL != sensor_raw_info_ptr->fix_ptr[mode_id] ) {
		sensor_raw_fix = sensor_raw_info_ptr->fix_ptr[mode_id];
	}
	if (NULL != sensor_raw_info_ptr->mode_ptr[mode_id].addr) {
		mode_param_info = sensor_raw_info_ptr->mode_ptr[mode_id];
	}
	if (NULL != sensor_raw_info_ptr->note_ptr[mode_id].note) {
		sensor_note_param = sensor_raw_info_ptr->note_ptr[mode_id];
	}
	DBG("ISP_TOOL:down_isp_param write_cmd->main_type %d\n",write_cmd->main_type);

	switch (write_cmd->main_type) {
	case MODE_TUNE_INFO:
	{
		if (0 == flag) {
			data_len = mode_param_info.len;
			data_addr = (uint8_t *)ispParserAlloc(data_len);
			data_len = msg->len - len_msg - len_data_header -len_mode_info;
			memcpy(data_addr + offset,isp_data_ptr + len_mode_info,data_len);
			offset += data_len;
			flag++;
		} else {
			data_len = msg->len - len_msg - len_data_header;
			memcpy(data_addr + offset,isp_data_ptr,data_len);
			offset += data_len;
		}
		if (0x01 == write_cmd->packet_status) {
			offset = 0;
			flag = 0;
			memcpy(sensor_raw_info_ptr->mode_ptr[mode_id].addr,data_addr,sensor_raw_info_ptr->mode_ptr[mode_id].len);
			rtn = isp_ioctl(isp_handler, ISP_CTRL_IFX_PARAM_UPDATE|ISP_TOOL_CMD_ID, data_addr);
			if(NULL != data_addr) {
				free(data_addr);
				data_addr = NULL;
			}

			eng_rsp_diag[value] =  0x00;
			value = value + 0x04;
			msg_ret->len = value -1;
			eng_rsp_diag[value] = 0x7e;
			DBG("ISP_TOOL:CMD_WRITE_ISP_PARAM3 \n");
			rtn = send(sockfd, eng_rsp_diag, value+1, 0);
		}
	}
	break;
	case MODE_AE_TABLE:
	{
		if (0 == flag) {
			if (NULL == sensor_raw_fix) {
				ISP_LOGE("sensor_raw_fix null pointer !");
				rtn = 0x01;
				return rtn;
			}
			rtn = get_ae_table_param_length(sensor_raw_fix,write_cmd->sub_type,&data_len);
			data_len = data_len + len_mode_info + len_block_info;
			data_addr = (uint8_t *)ispParserAlloc(data_len);
			if (NULL == data_addr) {
				ISP_LOGE("malloc mem error !");
				rtn = 0x01;
				return rtn;
			}
			flag ++;
		}
		data_len = msg->len -len_msg - len_data_header ;
		memcpy(data_addr + offset,isp_data_ptr,data_len);
		offset += data_len;
		if (0x01 == write_cmd->packet_status) {
			flag = 0;
			offset = 0;
			rtn = down_ae_table_param(sensor_raw_fix,write_cmd->sub_type,data_addr);
			rtn = isp_ioctl(isp_handler, ISP_CTRL_IFX_PARAM_UPDATE|ISP_TOOL_CMD_ID, data_addr);
			if (NULL != data_addr) {
				free(data_addr);
				data_addr = NULL;
			}

			eng_rsp_diag[value] =  0x00;
			value = value + 0x04;
			msg_ret->len = value -1;
			eng_rsp_diag[value] = 0x7e;
			DBG("ISP_TOOL:CMD_WRITE_ISP_PARAM3 \n");
			rtn = send(sockfd, eng_rsp_diag, value+1, 0);
		}
	}
	break;
	case MODE_AE_WEIGHT_TABLE:
	{
		DBG("ISP_TOOL:MODE_AE_WEIGHT_TABLE0 \n");
		if (0 == flag) {
			if (NULL == sensor_raw_fix) {
				ISP_LOGE("sensor_raw_fix null pointer !");
				rtn = 0x01;
				return rtn;
			}
			rtn = get_ae_weight_param_length(sensor_raw_fix,write_cmd->sub_type,&data_len);
			data_len = data_len + len_mode_info + len_block_info;
			data_addr = (uint8_t *)ispParserAlloc(data_len);
			if (NULL == data_addr) {
				ISP_LOGE("malloc mem error !");
				rtn = 0x01;
				return rtn;
			}
			flag ++;
		}
		data_len = msg->len -len_msg - len_data_header ;
		memcpy(data_addr + offset,isp_data_ptr,data_len);
		offset += data_len;

		DBG("ISP_TOOL:MODE_AE_WEIGHT_TABLE1 \n");
		if (0x01 == write_cmd->packet_status) {
			DBG("ISP_TOOL:MODE_AE_WEIGHT_TABLE2 \n");
			flag = 0;
			offset = 0;
			rtn = down_ae_weight_param(sensor_raw_fix,write_cmd->sub_type,data_addr);
			rtn = isp_ioctl(isp_handler, ISP_CTRL_IFX_PARAM_UPDATE|ISP_TOOL_CMD_ID, data_addr);
			if (NULL != data_addr) {
				free(data_addr);
				data_addr = NULL;
			}

			DBG("ISP_TOOL:MODE_AE_WEIGHT_TABLE3 \n");
			eng_rsp_diag[value] =  0x00;
			value = value + 0x04;
			msg_ret->len = value -1;
			eng_rsp_diag[value] = 0x7e;
			DBG("ISP_TOOL:MODE_AE_WEIGHT_TABLE4\n");
			rtn = send(sockfd, eng_rsp_diag, value+1, 0);
		}
	}
	break;
	case MODE_AE_SCENE_TABLE:
	{
		if (0 == flag) {
			rtn = get_ae_scene_param_length(sensor_raw_fix,write_cmd->sub_type,&data_len);
			data_len = data_len + len_mode_info + len_block_info;
			data_addr = (uint8_t *)ispParserAlloc(data_len);
			if (NULL == data_addr) {
				ISP_LOGE("malloc mem error !");
				rtn = 0x01;
				return rtn;
			}
			flag ++;
		}
		data_len = msg->len -len_msg - len_data_header;
		memcpy(data_addr + offset,isp_data_ptr,data_len);
		offset += data_len;
		if (0x01 == write_cmd->packet_status) {
			flag = 0;
			offset = 0;
			rtn = down_ae_scene_param(sensor_raw_fix,write_cmd->sub_type,data_addr);
			rtn = isp_ioctl(isp_handler, ISP_CTRL_IFX_PARAM_UPDATE|ISP_TOOL_CMD_ID, data_addr);
			if (NULL != data_addr) {
				free(data_addr);
				data_addr = NULL;
			}

			eng_rsp_diag[value] =  0x00;
			value = value + 0x04;
			msg_ret->len = value -1;
			eng_rsp_diag[value] = 0x7e;
			DBG("ISP_TOOL:MODE_AE_SCENE_TABLE \n");
			rtn = send(sockfd, eng_rsp_diag, value+1, 0);
		}
	}
	break;
	case MODE_AE_AUTO_ISO_TABLE:
	{
		if (0 == flag) {
			rtn = get_ae_auto_iso_param_length(sensor_raw_fix,write_cmd->sub_type,&data_len);
			data_len = data_len + len_mode_info + len_block_info;
			data_addr = (uint8_t *)ispParserAlloc(data_len);
			if (NULL == data_addr) {
				ISP_LOGE("malloc mem error !");
				rtn = 0x01;
				return rtn;
			}
			flag ++;
		}
		data_len = msg->len -len_msg - len_data_header;
		memcpy(data_addr + offset,isp_data_ptr,data_len);
		offset += data_len;
		if (0x01 == write_cmd->packet_status) {
			flag = 0;
			offset = 0;
			rtn = down_ae_auto_iso_param(sensor_raw_fix,write_cmd->sub_type,data_addr);
			rtn = isp_ioctl(isp_handler, ISP_CTRL_IFX_PARAM_UPDATE|ISP_TOOL_CMD_ID, data_addr);
			if (NULL != data_addr) {
				free(data_addr);
				data_addr = NULL;
			}

			eng_rsp_diag[value] =  0x00;
			value = value + 0x04;
			msg_ret->len = value -1;
			eng_rsp_diag[value] = 0x7e;
			DBG("ISP_TOOL:MODE_AE_AUTO_ISO_TABLE \n");
			rtn = send(sockfd, eng_rsp_diag, value+1, 0);
		}
	}
	break;
	case MODE_LNC_DATA:
	{
		if (0 == flag) {
			data_addr = (uint8_t *)ispParserAlloc(write_cmd->data_total_len);
			if (NULL == data_addr) {
				ISP_LOGE("malloc mem error !");
				rtn = 0x01;
				return rtn;
			}
			flag ++;
		}
		data_len = msg->len -len_msg - len_data_header;
		memcpy(data_addr + offset,isp_data_ptr,data_len);
		offset += data_len;
		if (0x01 == write_cmd->packet_status) {
			flag = 0;
			offset = 0;
			rtn = down_lnc_param(sensor_raw_fix,write_cmd->sub_type,data_addr,write_cmd->data_total_len);
			rtn = isp_ioctl(isp_handler, ISP_CTRL_IFX_PARAM_UPDATE|ISP_TOOL_CMD_ID, data_addr);

			eng_rsp_diag[value] =  0x00;
			value = value + 0x04;
			msg_ret->len = value -1;
			eng_rsp_diag[value] = 0x7e;
			DBG("ISP_TOOL:MODE_LNC_DATA \n");
			rtn = send(sockfd, eng_rsp_diag, value+1, 0);
		}
	}
	break;
	case MODE_AWB_DATA:
	{
		if (0 == flag) {
			data_addr = (uint8_t *)ispParserAlloc(write_cmd->data_total_len);
			if (NULL == data_addr) {
				ISP_LOGE("malloc mem error !");
				rtn = 0x01;
				return rtn;
			}
			flag ++;
		}
		data_len = msg->len -len_msg - len_data_header;
		memcpy(data_addr + offset,isp_data_ptr,data_len);
		offset += data_len;
		if (0x01 == write_cmd->packet_status) {
			flag = 0;
			offset = 0;
			rtn = down_awb_param(sensor_raw_fix,write_cmd->sub_type,data_addr,write_cmd->data_total_len);
			rtn = isp_ioctl(isp_handler, ISP_CTRL_IFX_PARAM_UPDATE|ISP_TOOL_CMD_ID, data_addr);

			eng_rsp_diag[value] =  0x00;
			value = value + 0x04;
			msg_ret->len = value -1;
			eng_rsp_diag[value] = 0x7e;
			DBG("ISP_TOOL:MODE_AWB_DATA \n");
			rtn = send(sockfd, eng_rsp_diag, value+1, 0);
		}
	}
	break;
	case MODE_NOTE_DATA:
	{
		if (0 == flag) {
			data_len = sensor_note_param.node_len;
			data_addr = (uint8_t *)ispParserAlloc(data_len);
			if (NULL == data_addr) {
				ISP_LOGE("malloc mem error !");
				rtn = 0x01;
				return rtn;
			}
			flag++;
		}
		data_len = msg->len - len_msg - len_data_header;
		memcpy(data_addr + offset,isp_data_ptr,data_len);
		offset += data_len;
		if (0x01 == write_cmd->packet_status) {
			offset = 0;
			flag = 0;
			memcpy(sensor_raw_info_ptr->note_ptr[mode_id].note,data_addr,sensor_raw_info_ptr->note_ptr[mode_id].node_len);
			if (NULL != data_addr) {
				free(data_addr);
				data_addr = NULL;
			}

			eng_rsp_diag[value] =  0x00;
			value = value + 0x04;
			msg_ret->len = value -1;
			eng_rsp_diag[value] = 0x7e;
			DBG("ISP_TOOL:CMD_WRITE_ISP_PARAM3 \n");
			rtn = send(sockfd, eng_rsp_diag, value+1, 0);
		}
	}
	break;
	case MODE_LIB_INFO_DATA:
	{
		if (0 == flag) {
			data_addr = (uint8_t *)ispParserAlloc(write_cmd->data_total_len);
			if (NULL == data_addr) {
				ISP_LOGE("malloc mem error !");
				rtn = 0x01;
				return rtn;
			}
			flag ++;
		}
		data_len = msg->len -len_msg - len_data_header;
		memcpy(data_addr + offset,isp_data_ptr,data_len);
		offset += data_len;
		if (0x01 == write_cmd->packet_status) {
			flag = 0;
			offset = 0;
			memcpy(sensor_raw_info_ptr->libuse_info, data_addr, sizeof(struct sensor_libuse_info));

			eng_rsp_diag[value] =  0x00;
			value = value + 0x04;
			msg_ret->len = value -1;
			eng_rsp_diag[value] = 0x7e;
			DBG("ISP_TOOL:MODE_LIB_INFO_DATA: \n");
			rtn = send(sockfd, eng_rsp_diag, value+1, 0);
		}
	}
	break;

	default:
		break;
	}
	return rtn;
}

int check_cmd_valid(struct isp_check_cmd_valid *cmd,struct msg_head_tag *msg)
{
	int rtn = 0x00;

	uint32_t len = 0;
	uint32_t rsp_len = 0;

	uint32_t len_msg = sizeof(struct msg_head_tag);
	uint32_t len_data_header = sizeof(struct isp_data_header_normal);
	struct isp_data_header_normal data_header = {0};
	struct msg_head_tag msg_tag = {0};

	struct isp_check_cmd_valid *cmd_ptr = (struct isp_data_header_read *)cmd;
	uint32_t unvalid_flag = 0;
	if (MODE_MAX >= cmd_ptr->main_type  ) {
		switch( cmd_ptr->main_type) {
		case MODE_NR_DATA:
		{
			if (ISP_NR_BLOCK_MIN > cmd_ptr->sub_type || FILE_NAME_MAX < cmd_ptr->sub_type) {
				unvalid_flag = 1;
			}
		}
		break;
		case MODE_AE_TABLE:
		{
			uint8_t flicker = (cmd_ptr->sub_type >>4)& 0x0f;
			uint8_t iso = cmd_ptr->sub_type & 0x0f;
			if (ISP_ANTIFLICKER_MAX < flicker || ISP_ANTIFLICKER_MIN > flicker) {
				unvalid_flag = 1;
			} else {
				if (ISP_ISO_NUM_MIN > iso || ISP_ISO_NUM_MAX < iso) {
					unvalid_flag = 1;
				}
			}
		}
		break;
		case MODE_AE_WEIGHT_TABLE:
		{
			if (ISP_AE_WEIGHT_TYPE_MAX < cmd_ptr->sub_type ||
				ISP_AE_WEIGHT_TYPE_MIN > cmd_ptr->sub_type) {
				unvalid_flag = 1;
			}
		}
		break;
		case MODE_AE_SCENE_TABLE:
		{
			uint8_t scene = (cmd_ptr->sub_type>>4) & 0x0f;
			uint8_t flicker = cmd_ptr->sub_type & 0x0f;
			if (ISP_ANTIFLICKER_MAX < flicker || ISP_ANTIFLICKER_MIN > flicker) {
				unvalid_flag = 1;
			}  else {
				if (ISP_SCENE_NUM_MIN > scene || ISP_SCENE_NUM_MAX < scene) {
					unvalid_flag = 1;
				}
			}
		}
		break;
		case MODE_AE_AUTO_ISO_TABLE:
		{
			if (ISP_ANTIFLICKER_MAX < cmd_ptr->sub_type ||
				ISP_ANTIFLICKER_MIN > cmd_ptr->sub_type) {
				unvalid_flag = 1;
			}
		}
		break;
		case MODE_LNC_DATA:
		{
			if (ISP_LNC_TAB_MAX < cmd_ptr->sub_type ||
				ISP_LNC_TAB_MIN > cmd_ptr->sub_type) {
				unvalid_flag = 1;
			}
		}
		break;
		case MODE_AWB_DATA:
		{
			if (1 < cmd_ptr->sub_type ||
				0 > cmd_ptr->sub_type) {
				unvalid_flag = 1;
			}
		}
		break;
		default:
			break;
		}
	} else {
		DBG("main type is out of range!");
		unvalid_flag = 1;;
	}
	if (0 != unvalid_flag) {
		DBG("sub type out of range!");
		rtn = 0x01;
		goto CHECK_CMD_UNVALID;
	} else {
		return rtn;
	}

CHECK_CMD_UNVALID:
	data_header.isp_mode = cmd_ptr->isp_mode;
	data_header.main_type = cmd_ptr->main_type;
	data_header.sub_type = cmd_ptr->sub_type;
	data_header.data_total_len = 0;

	msg_tag.seq_num = msg->seq_num;
	msg_tag.subtype = msg->subtype;
	msg_tag.type = msg->type;
	msg_tag.len = len_msg + len_data_header ;
	data_header.packet_status = 0x02;

	memcpy(eng_rsp_diag+1,(uint8_t *)&msg_tag,len_msg);
	rsp_len = len_msg+1;
	memcpy(eng_rsp_diag + rsp_len,(uint8_t *)&data_header,len_data_header);
	rsp_len = rsp_len + len_data_header + 1;
	eng_rsp_diag[rsp_len] = 0x7e;
	rtn = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
	rtn = 0x01;
	return rtn;
}

static int handle_isp_data(unsigned char *buf, unsigned int len)
{
	uint32_t handler_id=0x00;
	int rlen = 0, rsp_len = 0, extra_len = 0;
	int ret = 1, res = 0;
	int image_type = 0;
	MSG_HEAD_T *msg, *msg_ret;
	//add
	ISP_DATA_HEADER_T isp_msg = {0};
	int preview_tmpflag = 0;
	//
	struct camera_func* fun_ptr=ispvideo_GetCameraFunc();
	int is_stop_preview = 0;

	if (len < sizeof(MSG_HEAD_T)+2){
		DBG("ISP_TOOL:the formal cmd is 0x7e + diag + 0x7e,which is 10Bytes,but the cmd has less than 10 bytes\n");
		return -1;
	}

	msg = (MSG_HEAD_T *)(buf+1);
	if(msg->type != 0xfe)
		return -1;

	rsp_len = sizeof(MSG_HEAD_T)+1;
	memset(eng_rsp_diag,0,sizeof(eng_rsp_diag));
	memcpy(eng_rsp_diag,buf,rsp_len);
	msg_ret = (MSG_HEAD_T *)(eng_rsp_diag+1);

	if(CMD_GET_PREVIEW_PICTURE != msg->subtype) {
		msg_ret->seq_num = sequence_num++;
	}

	switch ( msg->subtype ) {
		case CMD_SFT_READ:
		{
			ISP_LOGI("Chj--get af info");
			ret = isp_sft_read(isp_handler, &eng_rsp_diag[rsp_len], &len);
			DBG("ISP_SFT:CMD_SFT_READ rsp_len %d len %d\n",rsp_len,len);
			rsp_len += len;
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			break;
		}
		case CMD_SFT_WRITE:
		{
			ISP_LOGI("Chj--set af info");
			len = msg_ret->len - 8;
			ret = isp_sft_write(isp_handler,buf+rsp_len, &len);
			DBG("ISP_SFT:CMD_SFT_WRITE rsp_len %d len %d\n",rsp_len,len);
			rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			break;
		}
		case CMD_SFT_TRIG:
		{
			ISP_LOGI("Chj--af trig");
			ret = isp_ioctl(isp_handler, ISP_CTRL_AF, NULL);// set af info after auto focus
			if(!ret){
				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
			}else{
				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_FAIL);
			}
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			break;
		}
		case CMD_SFT_SET_POS:
		{
			ISP_LOGI("Chj--set pos");
			uint32_t bypass = 0;
			g_af_pos = *(uint32_t*)(buf+9);
			ret = isp_ioctl(isp_handler, ISP_CTRL_SET_AF_POS, buf+9);
			ret = isp_ioctl(isp_handler, ISP_CTRL_SFT_SET_PASS, NULL);// open the filter
			usleep(1000*100);
			if(!ret){
				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
			}else{
				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_FAIL);
			}
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			break;
		}
		case CMD_SFT_GET_POS:
		{
			ISP_LOGI("Chj--get pos");
			uint32_t pos;
			ret = isp_ioctl(isp_handler, ISP_CTRL_GET_AF_POS, &pos);// set af info after auto focus
			if(!ret){
				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
			}else{
				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_FAIL);
			}
			*(uint32_t*)(eng_rsp_diag+rsp_len) = pos;
			rsp_len += sizeof(uint32_t);
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			break;
		}
		case CMD_SFT_OPEN_FILTER:
		{
			ISP_LOGI("Chj--open the filter");
			ret = isp_ioctl(isp_handler, ISP_CTRL_SFT_SET_PASS, NULL);// open the filter
			usleep(1000*100);
			if(!ret){
				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
			}else{
				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_FAIL);
			}
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			break;
		}
		case CMD_SFT_GET_AF_VALUE:// value back to PC
		{
			uint32_t statistic[50]={0};
			ISP_LOGI("Chj-- get af value 1");
			ret = isp_ioctl(isp_handler, ISP_CTRL_SFT_GET_AF_VALUE,statistic);// set af info after auto focus
			ISP_LOGI("Chj-- get af value 2 ret=%d,af_value=%d,af_value=%d",ret,statistic[0],statistic[25]);
			//ret = isp_ioctl(isp_handler, ISP_CTRL_SFT_SET_BYPASS, NULL);// close the filter
			if(!ret){
				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
				memmove(eng_rsp_diag+rsp_len,statistic,sizeof(statistic));
				rsp_len+=sizeof(statistic);
			}else{
				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_FAIL);
			}
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			break;
		}
		case CMD_SFT_TAKE_PICTURE:// pic back to PC
		{
			ISP_LOGI("Chj--sft take picture");
			uint8_t* isp_ptr=buf+sizeof(MSG_HEAD_T)+1;
			uint32_t width,height,interval;

			g_command = CMD_SFT_TAKE_PICTURE;
			if( NULL!=fun_ptr->take_picture ) {
				capture_img_end_flag=0;
				capture_flag=1;
				capture_format=*(uint32_t*)isp_ptr;// capture format
				width = (*(uint32_t*)(isp_ptr+4))>>16;
				height = (*(uint32_t*)(isp_ptr+4))&0x0000ffff;
				interval = *(uint32_t*)(isp_ptr+8);
				usleep(1000*interval);
				if(NULL!=fun_ptr->set_capture_size)
				{
					fun_ptr->set_capture_size(width,height);
				}
				fun_ptr->take_picture(0, capture_format);
				ISP_LOGI("Chj--1 width=%d,height=%d,capture_format=%d",width,height,capture_format);
				sem_wait(&capture_sem_lock);
				ISP_LOGI("Chj--2 width=%d,height=%d,capture_format=%d",width,height,capture_format);
				if(NULL!=fun_ptr->stop_preview){
					fun_ptr->stop_preview(0, 0);
				}
				usleep(1000*10);

				if(NULL!=fun_ptr->start_preview){
					fun_ptr->start_preview(0, 0);
				}

				capture_flag = 0;
				g_command = CMD_TAKE_PICTURE;
			}
			break;
		}
		case CMD_SFT_TAKE_PICTURE_NEW:// save pic to file system
		{
			ISP_LOGI("Chj--sft take picture new");
			uint8_t* isp_ptr=buf+sizeof(MSG_HEAD_T)+1;
			uint32_t startpos,endpos,step,width,height,interval;
			uint32_t pos;
			uint32_t statistic[50]={0};
			g_command = CMD_SFT_TAKE_PICTURE_NEW;
			if( NULL!=fun_ptr->take_picture ) {
				ret = isp_ioctl(isp_handler, ISP_CTRL_STOP_3A, NULL);
				////////////////////////////
				startpos = *(uint32_t*)isp_ptr;
				endpos = *(uint32_t*)(isp_ptr+4);
				step = *(uint32_t*)(isp_ptr+8);
				capture_format=*(uint32_t*)(isp_ptr+12);// capture format
				width = (*(uint32_t*)(isp_ptr+16))>>16;
				height = (*(uint32_t*)(isp_ptr+16))&0x0000ffff;
				interval = *(uint32_t*)(isp_ptr+20);
				for( pos=startpos;pos<=endpos;pos+=step ){
					g_af_pos = pos;
					ret = isp_ioctl(isp_handler, ISP_CTRL_SET_AF_POS, &pos);// set af pos after auto focus
					usleep(1000*10);
					capture_img_end_flag=0;
					capture_flag=1;
					usleep(1000*interval);
					if(NULL!=fun_ptr->set_capture_size)
					{
						fun_ptr->set_capture_size(width,height);
					}
					fun_ptr->take_picture(0, capture_format);
					ISP_LOGI("Chj--1 width=%d,height=%d,capture_format=%d",width,height,capture_format);
					sem_wait(&capture_sem_lock);
					ISP_LOGI("Chj--2 width=%d,height=%d,capture_format=%d",width,height,capture_format);
					if(NULL!=fun_ptr->stop_preview){
						fun_ptr->stop_preview(0, 0);
					}
					usleep(1000*10);

					if(NULL!=fun_ptr->start_preview){
						fun_ptr->start_preview(0, 0);
					}
					capture_flag = 0;

				}
				g_command = CMD_TAKE_PICTURE;
				ret = isp_ioctl(isp_handler, ISP_CTRL_START_3A, NULL);
			}
			break;
		}
		case CMD_ASIC_TAKE_PICTURE:// save pic to file system
		{
			ISP_LOGI("Chj--asic take picture");
			uint8_t* isp_ptr=buf+sizeof(MSG_HEAD_T)+1;
			uint32_t width,height;
			g_command = CMD_ASIC_TAKE_PICTURE;
			if( NULL!=fun_ptr->take_picture ) {
				capture_img_end_flag=0;
				capture_flag=1;
				capture_format=*(uint32_t*)isp_ptr;// capture format
				width = (*(uint32_t*)(isp_ptr+4))>>16;
				height = (*(uint32_t*)(isp_ptr+4))&0x0000ffff;
				if(NULL!=fun_ptr->set_capture_size)
				{
					fun_ptr->set_capture_size(width,height);
				}
				fun_ptr->take_picture(0, capture_format);
				ISP_LOGI("Chj--1 width=%d,height=%d,capture_format=%d",width,height,capture_format);
				sem_wait(&capture_sem_lock);
				ISP_LOGI("Chj--2 width=%d,height=%d,capture_format=%d",width,height,capture_format);
				if(NULL!=fun_ptr->stop_preview){
					fun_ptr->stop_preview(0, 0);
				}
				usleep(1000*10);

				if(NULL!=fun_ptr->start_preview){
					fun_ptr->start_preview(0, 0);
				}

				capture_flag = 0;
				g_command = CMD_TAKE_PICTURE;

				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
				eng_rsp_diag[rsp_len] = 0x7e;
				msg_ret->len = rsp_len-1;
				res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			}
			break;
		}
		case CMD_ASIC_TAKE_PICTURE_NEW:// save pic to file system
		{
			ISP_LOGI("Chj--asic take picture new");
			uint8_t* isp_ptr=buf+sizeof(MSG_HEAD_T)+1;;
			uint32_t startpos,endpos,step,width,height;
			uint32_t pos;
			g_command = CMD_ASIC_TAKE_PICTURE_NEW;
			if( NULL!=fun_ptr->take_picture ) {
				////////////////////////////
				startpos = *(uint32_t*)isp_ptr;
				endpos = *(uint32_t*)(isp_ptr+4);
				step = *(uint32_t*)(isp_ptr+8);
				capture_format=*(uint32_t*)(isp_ptr+12);// capture format
				width = (*(uint32_t*)(isp_ptr+16))>>16;
				height = (*(uint32_t*)(isp_ptr+16))&0x0000ffff;

				for( pos=startpos;pos<=endpos;pos+=step ){
					g_af_pos = pos;
					ret = isp_ioctl(isp_handler, ISP_CTRL_SET_AF_POS, &pos);// set af pos after auto focus
					usleep(1000*10);
					capture_img_end_flag=0;
					capture_flag=1;
					if(NULL!=fun_ptr->set_capture_size)
					{
						fun_ptr->set_capture_size(width,height);
					}
					fun_ptr->take_picture(0, capture_format);
					ISP_LOGI("Chj--1 width=%d,height=%d,capture_format=%d",width,height,capture_format);
					sem_wait(&capture_sem_lock);
					ISP_LOGI("Chj--2 width=%d,height=%d,capture_format=%d",width,height,capture_format);
					if(NULL!=fun_ptr->stop_preview){
						fun_ptr->stop_preview(0, 0);
					}
					usleep(1000*10);

					if(NULL!=fun_ptr->start_preview){
						fun_ptr->start_preview(0, 0);
					}
					capture_flag = 0;
				}
				g_command = CMD_TAKE_PICTURE;
			}
			break;
		}
		case CMD_START_PREVIEW:
		{// ok
			DBG("ISP_TOOL:CMD_START_PREVIEW \n");
			if(NULL!=fun_ptr->start_preview){
				ret = fun_ptr->start_preview(0, 0);
			}
			if(!ret){
				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
				preview_flag = 1;
			}else{
				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_FAIL);
			}
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			break;
		}
		case CMD_STOP_PREVIEW:
		{// ok
			DBG("ISP_TOOL:CMD_STOP_PREVIEW \n");
			preview_flag = 0;
			rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			break;
		}
		case CMD_GET_PREVIEW_PICTURE:
		{ // ok
			//DBG("ISP_TOOL:CMD_GET_PREVIEW_PICTURE \n");
			if(1==preview_flag)
			{
				preview_img_end_flag = 0;
				sem_wait(&preview_sem_lock);
				preview_img_end_flag = 1;
			} else {
				DBG("ISP_TOOL:CMD_GET_PREVIEW_PICTURE \n");
				rsp_len += rlen;
				eng_rsp_diag[rsp_len] = 0x7e;
				msg_ret->len = rsp_len-1;
				res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			}
			break;
		}
		case CMD_READ_ISP_PARAM:
		{
			DBG("ISP_TOOL:CMD_READ_ISP_PARAM \n");
			struct isp_parser_buf_in in_param = {0x00, 0x00};
			struct isp_parser_buf_rtn rtn_param = {0x00, 0x00};
			struct isp_parser_cmd_param rtn_cmd;
			uint32_t packet_num = 0;
			uint32_t send_number = 0;
			uint8_t* dig_ptr=buf;
			uint32_t i = 0;
			uint8_t* isp_ptr=buf+sizeof(MSG_HEAD_T)+1;

			memset(&rtn_cmd, 0, sizeof(rtn_cmd));
			in_param.buf_len=ispvideo_GetIspParamLenFromSt(dig_ptr);
			in_param.buf_addr=(unsigned long)ispParserAlloc(in_param.buf_len);

			if((0x00!=in_param.buf_len)
				&&(0x00!=in_param.buf_addr))
			{
				ret=ispvideo_GetIspParamFromSt(isp_ptr, (struct isp_parser_buf_rtn*)&in_param);
				if (ret) {
					DBG("ISP_TOOL:ispvideo_GetIspParamFromSt failed \n");
				}
				ret=ispParser(isp_handler, ISP_PARSER_DOWN, (void*)in_param.buf_addr, (void*)&rtn_cmd);
				if(ret) {
					DBG("ISP_TOOL:ispParser failed \n");
				}
				ret=ispParserFree((void*)in_param.buf_addr);
				if(ret) {
					DBG("ISP_TOOL:ispParserFree failed \n");
				}

			if(ISP_UP_PARAM==rtn_cmd.cmd)
			{
				ret=ispParser(isp_handler, ISP_PARSER_UP_PARAM, (void*)in_param.buf_addr, (void*)&rtn_param);

				if(0x00==ret)
				{

						packet_num = (rtn_param.buf_len+SEND_DATA_SIZE-1)/SEND_DATA_SIZE;
					for (i=0; i<packet_num; i++, send_number++)
					{
						if (i < (packet_num-1))
								len = SEND_DATA_SIZE;
						else
								len = rtn_param.buf_len -SEND_DATA_SIZE*i;

						rsp_len = sizeof(MSG_HEAD_T)+1;

						isp_msg.totalpacket = packet_num;
						isp_msg.packetsn = send_number;

						memcpy(eng_rsp_diag+rsp_len, (char *)&isp_msg, sizeof(ISP_DATA_HEADER_T));
						rsp_len += sizeof(ISP_DATA_HEADER_T);


						//DBG("%s:ISP_TOOL: request rsp_len[%d]\n",__FUNCTION__, rsp_len);
							memcpy(eng_rsp_diag+rsp_len, (char *)rtn_param.buf_addr+i*SEND_DATA_SIZE, len);


						rsp_len += len;
						eng_rsp_diag[rsp_len] = 0x7e;
						msg_ret->len = rsp_len-1;
						msg_ret->seq_num = sequence_num++;
						res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);

					}

				}
			}
		}


		break;
	}
		case CMD_WRITE_ISP_PARAM:
		{
			DBG("ISP_TOOL:CMD_WRITE_ISP_PARAM \n");
			struct isp_parser_buf_in in_param = {0x00, 0x00};
			static struct isp_parser_buf_in packet_param = {0x00, 0x00};
			uint8_t* dig_ptr=buf;
			uint8_t* isp_ptr=buf+sizeof(MSG_HEAD_T)+sizeof(ISP_DATA_HEADER_T)+1;
			uint32_t* packet = (uint32_t*)(buf+sizeof(MSG_HEAD_T)+1);
			uint32_t packet_num = 0;
			static uint32_t offset = 0;
			uint32_t param_len = 0;
			uint32_t packet_total = 0;
			packet_total = packet[0];
			packet_num = packet[1];
			if(0==packet_num) {
				packet_param.buf_addr = (unsigned long)ispParserAlloc(packet_total*SEND_DATA_SIZE);
				packet_param.buf_len = packet_total*SEND_DATA_SIZE;
			}

			in_param.buf_len=ispvideo_GetIspParamLenFromSt(dig_ptr)-sizeof(ISP_DATA_HEADER_T);
			in_param.buf_addr=(unsigned long)ispParserAlloc(in_param.buf_len);

			if((0x00!=in_param.buf_len)
				&&(0x00!=in_param.buf_addr))
			{
				if(NULL!=fun_ptr->stop_preview){
//					fun_ptr->stop_preview(0, 0);
				}

				ret=ispvideo_GetIspParamFromSt(isp_ptr, (struct isp_parser_buf_rtn*)&in_param);
				if (ret) {
					DBG("ISP_TOOL:ispvideo_GetIspParamFromSt failed \n");
				}
				memcpy((char*)packet_param.buf_addr+offset,(void*)in_param.buf_addr,in_param.buf_len);
				offset += in_param.buf_len;

				if(in_param.buf_addr) {
					ret=ispParserFree((void*)in_param.buf_addr);
				}
				if(NULL!=fun_ptr->start_preview){
//					fun_ptr->start_preview(0, 0);
				}
			}

			if(packet_num==(packet_total-1)) {
				offset = 0;
				DBG("ISP_TOOL:CMD_WRITE_ISP_PARAM1 \n");
				ret=ispParser(isp_handler, ISP_PARSER_DOWN, (void*)packet_param.buf_addr, NULL);
				DBG("ISP_TOOL:CMD_WRITE_ISP_PARAM2  ret  %d\n",ret);
				ret=ispParserFree((void*)packet_param.buf_addr);
				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ret);
				eng_rsp_diag[rsp_len] = 0x7e;
				msg_ret->len = rsp_len-1;
				DBG("ISP_TOOL:CMD_WRITE_ISP_PARAM3 \n");
				res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
				DBG("ISP_TOOL:CMD_WRITE_ISP_PARAM4 \n");

			}

			break;
		}
		case CMD_READ_ISP_PARAM_V1:
		{
			DBG("ISP_TOOL:CMD_READ_ISP_PARAM_V1 \n");
			struct isp_data_header_read *read_cmd = NULL;
			uint32_t data_len = 0;
			uint8_t* dig_ptr = buf;
			uint8_t* isp_ptr = buf+sizeof(MSG_HEAD_T)+1;
			struct isp_check_cmd_valid cmd = {0};

			data_len = ispvideo_GetIspParamLenFromSt(dig_ptr);
			DBG("ISP_TOOL : data_len = %d",data_len);
			if (DATA_CMD_LENGTH == data_len) {
				DBG("ISP_TOOL : data is read cmd");
				read_cmd = (struct isp_data_header_read *)isp_ptr;
			}else {
				DBG("ISP_TOOL : cmd was error");
				break;
			}
			cmd.isp_mode = read_cmd->isp_mode;
			cmd.main_type = read_cmd->main_type;
			cmd.sub_type = read_cmd->sub_type;
			ret = check_cmd_valid(&cmd,msg);
			if (0 == ret) {
				if (0x02 > read_cmd->main_type) {
					ret = isp_nr_reg_read(isp_handler, eng_rsp_diag, DATA_BUF_SIZE, buf);
				} else {
					ret = send_isp_param(read_cmd,msg);
				}
				if (0x00 != ret)
					DBG("ISP_TOOL : read param error");
			}
		}
		break;
		case CMD_WRITE_ISP_PARAM_V1:
		{
			DBG("ISP_TOOL:CMD_WRITE_ISP_PARAM_V1 \n");
			struct isp_data_header_normal *write_cmd = NULL;
			uint8_t *dig_ptr = buf;
			uint8_t *isp_ptr = buf +sizeof(MSG_HEAD_T) +1;
			uint8_t *isp_data_ptr = isp_ptr + sizeof(struct isp_data_header_normal);
			struct isp_check_cmd_valid cmd = {0};

			write_cmd = (struct isp_data_header_normal *)isp_ptr;
			cmd.isp_mode = write_cmd->isp_mode;
			cmd.main_type = write_cmd->main_type;
			cmd.sub_type = write_cmd->sub_type;
			ret = check_cmd_valid(&cmd,msg);
			if (0 == ret) {
				if (0x02 > write_cmd->main_type) {
					ret = isp_nr_write(isp_handler, eng_rsp_diag, buf);
				} else {
					ret = down_isp_param(isp_handler,write_cmd,msg,isp_data_ptr,rsp_len);
				}
			}
		}
		break;
		case CMD_UPLOAD_MAIN_INFO:
		{ // ok
			DBG("ISP_TOOL:CMD_UPLOAD_MAIN_INFO \n");
			/* TODO:read isp param operation */
			// rlen is the size of isp_param
			// pass eng_rsp_diag+rsp_len

			struct isp_parser_buf_in in_param = {0x00, 0x00};
			struct isp_parser_buf_rtn rtn_param = {0x00, 0x00};
			struct isp_parser_cmd_param rtn_cmd;
			uint8_t* dig_ptr=buf;
			uint8_t* isp_ptr=buf+sizeof(MSG_HEAD_T)+1;
			uint8_t i=0x00;
			uint32_t* addr=(uint32_t*)isp_ptr;

			memset(&rtn_cmd, 0, sizeof(rtn_cmd));
			in_param.buf_len=ispvideo_GetIspParamLenFromSt(dig_ptr);
			in_param.buf_addr=(unsigned long)ispParserAlloc(in_param.buf_len);

			if((0x00!=in_param.buf_len)
				&&(0x00!=in_param.buf_addr))
			{
				ret=ispvideo_GetIspParamFromSt(isp_ptr, (struct isp_parser_buf_rtn*)&in_param);
				if (ret) {
					DBG("ISP_TOOL:ispvideo_GetIspParamFromSt failed\n");
				}
				ret=ispParser(isp_handler, ISP_PARSER_DOWN, (void*)in_param.buf_addr, (void*)&rtn_cmd);
				if (ret) {
					DBG("ISP_TOOL:ispParser failed\n");
				}
				ret=ispParserFree((void*)in_param.buf_addr);
				if (ret) {
					DBG("ISP_TOOL:ispParserFree failed\n");
				}

				if(ISP_MAIN_INFO==rtn_cmd.cmd)
				{
					ret=ispParser(isp_handler, ISP_PARSER_UP_MAIN_INFO, (void*)in_param.buf_addr, (void*)&rtn_param);
					if(0x00==ret)
					{
						isp_ptr=eng_rsp_diag+sizeof(MSG_HEAD_T)+1;
						rlen=ispvideo_SetIspParamToSt(isp_ptr, (struct isp_parser_buf_in*)&rtn_param);
						ret=ispParserFree((void*)rtn_param.buf_addr);
					}
				}
			}

			rsp_len += rlen;
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			break ;
		}
		case CMD_TAKE_PICTURE:
		{
			DBG("ISP_TOOL:CMD_TAKE_PICTURE\n");
			struct isp_parser_buf_in in_param = {0x00, 0x00};
			struct isp_parser_buf_rtn rtn_param = {0x00, 0x00};
			struct isp_parser_cmd_param rtn_cmd;
			uint8_t* dig_ptr=buf;
			uint8_t* isp_ptr=buf+sizeof(MSG_HEAD_T)+1;

			memset(&rtn_cmd, 0, sizeof(rtn_cmd));
			in_param.buf_len=ispvideo_GetIspParamLenFromSt(dig_ptr);
			in_param.buf_addr=(unsigned long)ispParserAlloc(in_param.buf_len);

			if((0x00!=in_param.buf_len)
				&&(0x00!=in_param.buf_addr))
			{
				ret=ispvideo_GetIspParamFromSt(isp_ptr, (struct isp_parser_buf_rtn*)&in_param);
				if (ret) {
					DBG("ISP_TOOL:ispvideo_GetIspParamFromSt failed \n");
				}
				ret=ispParser(isp_handler, ISP_PARSER_DOWN, (void*)in_param.buf_addr, (void*)&rtn_cmd);
				if (ret) {
					DBG("ISP_TOOL:ispParser failed \n");
				}
				ret=ispParserFree((void*)in_param.buf_addr);
				if (ret) {
					DBG("ISP_TOOL:ispParserFree failed \n");
				}

				if((ISP_CAPTURE==rtn_cmd.cmd)
					&&(NULL!=fun_ptr->take_picture)) {

					capture_img_end_flag=0;
					capture_flag=1;

					capture_format=rtn_cmd.param[0];// capture format

					if(NULL!=fun_ptr->set_capture_size)
					{
						fun_ptr->set_capture_size(rtn_cmd.param[1], rtn_cmd.param[2]);
					}

					fun_ptr->take_picture(0, capture_format);
					sem_wait(&capture_sem_lock);

					if(NULL!=fun_ptr->stop_preview){
						fun_ptr->stop_preview(0, 0);
					}

					usleep(1000*10);

					if(NULL!=fun_ptr->start_preview){
						fun_ptr->start_preview(0, 0);
					}

				}
			}

			capture_flag = 0;
			break;
		}
		case CMD_ISP_LEVEL:
		{ // ok need test
			DBG("ISP_TOOL:CMD_ISP_LEVEL \n");
			struct isp_parser_buf_in in_param = {0x00, 0x00};
			struct isp_parser_cmd_param rtn_cmd;
			uint8_t* dig_ptr=buf;
			uint8_t* isp_ptr=buf+sizeof(MSG_HEAD_T)+1;

			memset(&rtn_cmd, 0, sizeof(rtn_cmd));
			in_param.buf_len=ispvideo_GetIspParamLenFromSt(dig_ptr);
			in_param.buf_addr=(unsigned long)ispParserAlloc(in_param.buf_len);

			if((0x00!=in_param.buf_len)
				&&(0x00!=in_param.buf_addr))
			{
				ret=ispvideo_GetIspParamFromSt(isp_ptr, (struct isp_parser_buf_rtn*)&in_param);
				if (ret) {
					DBG("ISP_TOOL:ispvideo_GetIspParamFromSt failed \n");
				}
				ret=ispParser(isp_handler, ISP_PARSER_DOWN, (void*)in_param.buf_addr, (void*)&rtn_cmd);
				if (ret) {
					DBG("ISP_TOOL:ispParser failed \n");
				}
				ispParserFree((void*)in_param.buf_addr);
			}

			rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ret);
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			break;
		}

		case CMD_READ_SENSOR_REG:
		{// ok need test
			DBG("ISP_TOOL:CMD_READ_SENSOR_REG \n");
			struct isp_parser_buf_in in_param = {0x00, 0x00};
			struct isp_parser_cmd_param rtn_cmd;
			struct isp_parser_buf_rtn rtn_param = {0x00, 0x00};
			uint8_t* dig_ptr=buf;
			uint8_t* isp_ptr=buf+sizeof(MSG_HEAD_T)+1;

			memset(&rtn_cmd, 0, sizeof(rtn_cmd));
			in_param.buf_len=ispvideo_GetIspParamLenFromSt(dig_ptr);
			in_param.buf_addr=(unsigned long)ispParserAlloc(in_param.buf_len);

			if((0x00!=in_param.buf_len)
				&&(0x00!=in_param.buf_addr))
			{
				ret=ispvideo_GetIspParamFromSt(isp_ptr, (struct isp_parser_buf_rtn*)&in_param);
				if (ret) {
					DBG("ISP_TOOL:ispvideo_GetIspParamFromSt failed \n");
				}
				ret=ispParser(isp_handler, ISP_PARSER_DOWN, (void*)in_param.buf_addr, (void*)&rtn_cmd);
				if (ret) {
					DBG("ISP_TOOL:ispParser failed \n");
				}
				ret=ispParserFree((void*)in_param.buf_addr);
				if (ret) {
					DBG("ISP_TOOL:ispParserFree failed \n");
				}

				if(ISP_READ_SENSOR_REG==rtn_cmd.cmd)
				{
					ret=ispParser(isp_handler, ISP_READ_SENSOR_REG, (void*)&rtn_cmd, (void*)&rtn_param);
					if(0x00==ret)
					{
						isp_ptr=eng_rsp_diag+sizeof(MSG_HEAD_T)+1;
						rlen=ispvideo_SetIspParamToSt(isp_ptr, (struct isp_parser_buf_in*)&rtn_param);
						ret=ispParserFree((void*)rtn_param.buf_addr);
					}
				}
			}

			rsp_len += rlen;
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			break;
		}
		case CMD_WRITE_SENSOR_REG:
		{ // ok need test
			DBG("ISP_TOOL:CMD_WRITE_SENSOR_REG \n");
			struct isp_parser_buf_in in_param = {0x00, 0x00};
			struct isp_parser_cmd_param rtn_cmd;
			uint8_t* dig_ptr=buf;
			uint8_t* isp_ptr=buf+sizeof(MSG_HEAD_T)+1;

			memset(&rtn_cmd, 0, sizeof(rtn_cmd));
			in_param.buf_len=ispvideo_GetIspParamLenFromSt(dig_ptr);
			in_param.buf_addr=(unsigned long)ispParserAlloc(in_param.buf_len);

			if((0x00!=in_param.buf_len)
				&&(0x00!=in_param.buf_addr))
			{
				ret=ispvideo_GetIspParamFromSt(isp_ptr, (struct isp_parser_buf_rtn*)&in_param);
				if (ret) {
					DBG("ISP_TOOL:ispvideo_GetIspParamFromSt failed \n");
				}
				ret=ispParser(isp_handler, ISP_PARSER_DOWN, (void*)in_param.buf_addr, (void*)&rtn_cmd);
				if (ret) {
					DBG("ISP_TOOL:ispParser failed \n");
				}
				ret=ispParserFree((void*)in_param.buf_addr);
				if (ret) {
					DBG("ISP_TOOL:ispParserFree failed \n");
				}
			}

			rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			break;
		}
		case CMD_GET_INFO:
		{ // ok
			DBG("ISP_TOOL:CMD_GET_INFO \n");
			/* TODO:read isp param operation */
			// rlen is the size of isp_param
			// pass eng_rsp_diag+rsp_len

			struct isp_parser_buf_in in_param = {0x00, 0x00};
			struct isp_parser_buf_rtn rtn_param = {0x00, 0x00};
			struct isp_parser_cmd_param rtn_cmd;
			uint8_t* dig_ptr=buf;
			uint8_t* isp_ptr=buf+sizeof(MSG_HEAD_T)+1;
			uint8_t i=0x00;
			uint32_t* addr=(uint32_t*)isp_ptr;
			uint32_t* ptr=NULL;

			memset(&rtn_cmd, 0, sizeof(rtn_cmd));
			in_param.buf_len=ispvideo_GetIspParamLenFromSt(dig_ptr);
			in_param.buf_addr=(unsigned long)ispParserAlloc(in_param.buf_len);

			if((0x00!=in_param.buf_len)
				&&(0x00!=in_param.buf_addr))
			{
				ret=ispvideo_GetIspParamFromSt(isp_ptr, (struct isp_parser_buf_rtn*)&in_param);
				if (ret) {
					DBG("ISP_OTP:ispvideo_GetIspParamFromSt failed \n");
				}
				ret=ispParser(isp_handler, ISP_PARSER_DOWN, (void*)in_param.buf_addr, (void*)&rtn_cmd);
				if (ret) {
					DBG("ISP_OTP:ispParser failed \n");
				}
				ret=ispParserFree((void*)in_param.buf_addr);
				if (ret) {
					DBG("ISP_OTP:ispParserFree failed \n");
				}

				DBG("ISP_TOOL:CMD_GET_INFO rtn cmd:%d \n", rtn_cmd.cmd);

				if(ISP_INFO==rtn_cmd.cmd)
				{
					ret=ispParser(isp_handler, ISP_PARSER_UP_INFO, (void*)&rtn_cmd, (void*)&rtn_param);
					if(0x00==ret)
					{
						isp_ptr=eng_rsp_diag+sizeof(MSG_HEAD_T)+1;
						rlen=ispvideo_SetIspParamToSt(isp_ptr, (struct isp_parser_buf_in*)&rtn_param);
						ptr=(uint32_t*)rtn_param.buf_addr;
						for(i=0x00; i<rtn_param.buf_len; i+=0x04){

							DBG("ISP_TOOL:CMD_GET_INFO param:0x%08x \n", *ptr++);

						}
						ret=ispParserFree((void*)rtn_param.buf_addr);
					}
				}
			}

			rsp_len += rlen;
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			break ;
		}
		case CMD_OTP_WRITE:
			DBG("ISP_OTP:CMD_OTP_WRITE \n");
			preview_tmpflag = preview_flag;
			len -= rsp_len;
			is_stop_preview = isp_otp_needstopprev(&buf[rsp_len], &len);
			if(is_stop_preview && preview_tmpflag)
			{
				if(NULL!=fun_ptr->stop_preview){
					fun_ptr->stop_preview(0, 0);
				}
			}

			ret = isp_otp_write(isp_handler, &buf[rsp_len], &len);
			if (!ret) {
				memset(&eng_rsp_diag[rsp_len],0,len);
			} else {
				memset(&eng_rsp_diag[rsp_len],0,len);
				eng_rsp_diag[rsp_len] = 0x01;
			}
			rsp_len += len;
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;

			if(is_stop_preview && preview_tmpflag)
			{
				if(NULL!=fun_ptr->start_preview){
					fun_ptr->start_preview(0, 0);
				}
				preview_flag = preview_tmpflag;
			}

			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			DBG("ISP_OTP:CMD_OTP_WRITE done\n");
			break;

		case CMD_OTP_READ:
			DBG("ISP_OTP:CMD_OTP_READ \n");
			preview_tmpflag = preview_flag;
			len -= rsp_len;
			is_stop_preview = isp_otp_needstopprev(&buf[rsp_len], &len);
			if(is_stop_preview && preview_tmpflag)
			{
				if(NULL!=fun_ptr->stop_preview){
					fun_ptr->stop_preview(0, 0);
				}
			}

			memcpy(eng_rsp_diag,buf,(msg_ret->len+1));
			ret = isp_otp_read(isp_handler, &eng_rsp_diag[rsp_len], &len);
			DBG("ISP_OTP:CMD_OTP_READ rsp_len %d len %d\n",rsp_len,len);
			rsp_len += len;
			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;

			if(is_stop_preview && preview_tmpflag)
			{
				if(NULL!=fun_ptr->start_preview){
					fun_ptr->start_preview(0, 0);
				}
				preview_flag = preview_tmpflag;
			}
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			DBG("ISP_OTP:CMD_OTP_READ  done\n");

			break;

		case CMD_DOWNLOAD_RAW_PIC:
		{
			uint32_t img_len, pack_sn, total_pack, img_width, img_height, img_headlen;
			unsigned char* img_addr;
			img_len = ispvideo_GetImgDataLen(buf);
			img_addr = ispvideo_GetImgDataInfo(buf, &pack_sn, &total_pack, &img_width, &img_height, &img_headlen);
			if (NULL != img_addr) {

				if (1 == pack_sn) {
					//4208X3120_gain_123_awbgain_r_1659_g_1024_b_1757_ct_4901_bv_64.mipi_raw
					scene_param.width = img_width;
					scene_param.height = img_height;

					bzero(raw_filename, sizeof(raw_filename));
					sprintf(raw_filename+1, "/data/misc/cameraserver/%dX%d_gain_%d_awbgain_r_%d_g_%d_b_%d_ct_%d_bv_%d.mipi_raw",
					scene_param.width,scene_param.height,scene_param.gain, scene_param.awb_gain_r,
					scene_param.awb_gain_g,scene_param.awb_gain_b,scene_param.smart_ct, scene_param.smart_bv);

					ISP_LOGI("simulation raw filename %s", raw_filename+1);
					raw_filename[0] = 1;
					raw_fp = fopen(raw_filename+1, "wb+");
				}
				if (raw_fp) {
					fwrite((void*)img_addr, 1, img_len, raw_fp);
				}

				if (pack_sn == total_pack) {
					if (raw_fp){
						fclose(raw_fp);
						raw_fp = NULL;
					}
					tool_fmt_pattern = (img_headlen >> 0x10) & 0xFFFF;
					ISP_LOGI("image pattern %d", tool_fmt_pattern);
					//send response packet
					rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
					eng_rsp_diag[rsp_len] = 0x7e;
					msg_ret->len = rsp_len-1;
					res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
				}
			}
			break;
		}
		case CMD_WRTIE_SCENE_PARAM:
		{
			struct isptool_scene_param scene_info;
			ret = ispvideo_GetSceneInfo(buf, &scene_info);

			//send response packet
			if (0x00 == ret) {
				ret = isp_ioctl(isp_handler, ISP_CTRL_TOOL_SET_SCENE_PARAM, (void*)&scene_info);
				if (ret) {
					CMR_LOGE("failed isp ioctl for scene parameter %ld", ret);
				}
				memcpy(&(scene_param.gain), &(scene_info.gain), sizeof(struct isptool_scene_param)-8);
				ISP_LOGI("width/height %d/%d, gain 0x%x, awb r/g/b  0x%x, 0x%x, 0x%x, ct 0x%x, bv 0x%x",
					scene_param.width, scene_param.height, scene_param.gain, scene_param.awb_gain_r,
					scene_param.awb_gain_g, scene_param.awb_gain_b, scene_param.smart_ct, scene_param.smart_bv);

				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_SUCCESS);
			} else {
				rsp_len+=ispvideo_SetreTurnValue((uint8_t*)&eng_rsp_diag[rsp_len], ISP_CMD_FAIL);
			}

			eng_rsp_diag[rsp_len] = 0x7e;
			msg_ret->len = rsp_len-1;
			res = send(sockfd, eng_rsp_diag, rsp_len+1, 0);
			break;
		}

		case CMD_START_SIMULATION:
		{
			ISP_LOGI("simulation start");
			if((NULL!=fun_ptr->take_picture)) {
				capture_img_end_flag=0;
				capture_flag=1;

				capture_format=0x10;// capture format:JPEG
				if(NULL!=fun_ptr->set_capture_size) {
					fun_ptr->set_capture_size(scene_param.width, scene_param.height);
				}

				fun_ptr->take_picture(1, capture_format);  //take picture for simulation
				sem_wait(&capture_sem_lock);
			}
			capture_flag = 0;
			break;
		}
		default:
			break;
	}				/* -----  end switch  ----- */
	return EXIT_SUCCESS;
}				/* ----------  end of function main  ---------- */

/*

#define ISP_DATA_YUV422_2FRAME (1<<0)
#define ISP_DATA_YUV420_2FRAME (1<<1)
#define ISP_DATA_NORMAL_RAW10 (1<<2)
#define ISP_DATA_MIPI_RAW10 (1<<3)
#define ISP_DATA_JPG (1<<4)

*/

void ispvideo_Scale(uint32_t format, uint32_t in_w, uint32_t in_h, char *in_imgptr, int in_imglen, uint32_t* out_w, uint32_t* out_h, char **out_imgptr, int* out_imglen)
{
	int ret;
	uint32_t x=0x00;
	uint32_t y=0x00;
	uint32_t src_img_w=in_w;
	uint32_t src_img_h=in_h;
	uint32_t img_w=in_w;
	uint32_t img_h=in_h;
	uint8_t* src_buf_ptr=(uint8_t*)in_imgptr;
	uint8_t* dst_buf_ptr=(uint8_t*)*out_imgptr;
	uint32_t shift_num=0x01;

	if((PREVIEW_MAX_WIDTH>=img_w)
		&&(PREVIEW_MAX_HEIGHT>=img_h))	{
		*out_w=img_w;
		*out_h=img_h;
		*out_imgptr=in_imgptr;
		*out_imglen=in_imglen;
		return ;
	}

	for(x=0x00; ; x++){
		if((PREVIEW_MAX_WIDTH>=img_w)
			&&(PREVIEW_MAX_HEIGHT>=img_h))	{
			break ;
		}
		img_w>>=0x01;
		img_h>>=0x01;
		shift_num<<=0x01;
	}

	*out_w=img_w;
	*out_h=img_h;

	if ((ISP_TOOL_YUV420_2FRAME == format)
		||(ISP_TOOL_YVU420_2FRAME == format)) {
		*out_imglen=img_w*img_h+(img_w*img_h)/2;
		shift_num-=0x01;
		shift_num<<=0x01;

		for(y=0x00; y<img_h; y+=0x02){/*y*/
			for(x=0x00; x<img_w; x+=0x02){
				*dst_buf_ptr++=*src_buf_ptr++;
				*dst_buf_ptr++=*src_buf_ptr++;
				src_buf_ptr+=shift_num;
			}
			for(x=0x00; x<img_w; x+=0x02){
				*dst_buf_ptr++=*src_buf_ptr++;
				*dst_buf_ptr++=*src_buf_ptr++;
				src_buf_ptr+=shift_num;
			}
			src_buf_ptr+=(src_img_w*shift_num);
		}

		src_buf_ptr=(uint8_t*)in_imgptr;
		dst_buf_ptr=(uint8_t*)*out_imgptr;
		src_buf_ptr=src_buf_ptr+src_img_w*src_img_h;
		dst_buf_ptr=dst_buf_ptr+img_w*img_h;


		img_h>>=0x01;
		for(y=0x00; y<img_h; y++){/*uv*/
			for(x=0x00; x<img_w; x+=0x02){
				*dst_buf_ptr++=*src_buf_ptr++;
				*dst_buf_ptr++=*src_buf_ptr++;
				src_buf_ptr+=shift_num;
			}
			src_buf_ptr+=(src_img_w*(shift_num/2));
		}

	}

}

void send_img_data(uint32_t format, uint32_t width, uint32_t height, char *imgptr, int imagelen)
{
	uint32_t handler_id=0x00;
	int ret;

	if (0==preview_img_end_flag)
	{
		pthread_mutex_lock(&ispstream_lock);

		char *img_ptr = (char *)preview_buf_ptr;
		int img_len;
		uint32_t img_w;
		uint32_t img_h;
		/*if preview size more than vga that is subsample to less than vga for preview frame ratio*/
		ispvideo_Scale(format, width, height, imgptr, imagelen, &img_w, &img_h, &img_ptr, &img_len);

		ret = handle_img_data(format, img_w, img_h, img_ptr, img_len, 0, 0, 0, 0);

		sem_post(&preview_sem_lock);
		if (ret != 0) {
			DBG("ISP_TOOL:handle_img_data().error ret = %d.", ret);
		}

		pthread_mutex_unlock(&ispstream_lock);
	}

	return;
}

void send_capture_complete_msg()
{
	if ((capture_flag == 1) && (1 == capture_img_end_flag) )
	{
		usleep(1000*1000);
		sem_post(&capture_sem_lock);
		capture_flag = 0;
	}
}

void send_capture_data(uint32_t format, uint32_t width, uint32_t height, char *ch0_ptr, int ch0_len,char *ch1_ptr, int ch1_len,char *ch2_ptr, int ch2_len)
{
	uint32_t handler_id=0x00;
	int ret;

	if ((0 == capture_img_end_flag)&&(format == (uint32_t)capture_format))
	{
		pthread_mutex_lock(&ispstream_lock);

		DBG("ISP_TOOL: capture format: %d, width: %d, height: %d.\n", format, width, height);
		switch( g_command ){
			case CMD_ASIC_TAKE_PICTURE:
			case CMD_ASIC_TAKE_PICTURE_NEW:
				ret = handle_img_data_asic(format, width, height, ch0_ptr, ch0_len, ch1_ptr, ch1_len, ch2_ptr, ch2_len);
				break;
			//case CMD_SFT_TAKE_PICTURE_NEW:
				//ret = handle_img_data_sft(format, width, height, ch0_ptr, ch0_len, ch1_ptr, ch1_len, ch2_ptr, ch2_len);
				//break;
			default:
				ret = handle_img_data(format, width, height, ch0_ptr, ch0_len, ch1_ptr, ch1_len, ch2_ptr, ch2_len);
				break;
		}
		capture_img_end_flag=1;
		if (ret != 0) {
			DBG("ISP_TOOL: handle_img_data().error  ret = %d.", ret);
		}

		pthread_mutex_unlock(&ispstream_lock);
	}
}

int isp_RecDataCheck(uint8_t* rx_buf_ptr, int rx_bug_len, uint8_t* cmd_buf_ptr,  int* cmd_len)
{
	int rtn;
	uint8_t* rx_ptr=rx_buf_ptr;
	uint8_t* cmd_ptr=cmd_buf_ptr;
	uint8_t packet_header=rx_buf_ptr[0];
	uint16_t packet_len=(rx_buf_ptr[6]<<0x08)|(rx_buf_ptr[5]);
	uint8_t packet_end=rx_buf_ptr[rx_bug_len-1];
	int rx_len=rx_bug_len;
	int cmd_buf_offset=rx_packet_len;

	rtn=0;
	*cmd_len=rx_bug_len;

	if ((0x7e == packet_header)
		&&(0x7e == packet_end) && (packet_len+2 == rx_bug_len)) {
	/* one packet */

	} else { /* mul packet */
		rx_packet_len+=rx_bug_len;

		if ((0x7e == packet_end)
			&&(rx_packet_len == rx_packet_total_len)) {
			*cmd_len=rx_packet_len;
		} else {
			rtn=1;
		}
	}

	memcpy((void*)&cmd_buf_ptr[cmd_buf_offset], rx_buf_ptr, rx_bug_len);

	if (0 == rtn) {
		rx_packet_len=0x00;
		rx_packet_total_len=0x00;
	}

	return rtn;
}

static void * isp_diag_handler(void *args)
{
	uint32_t handler_id=0x00;
	int from = *((int *)args);
	int i, cnt, res, cmd_len, rtn, last_len;
	static char *code = "diag channel exit";
	fd_set rfds;
	struct timeval tv;

	FD_ZERO(&rfds);
	FD_SET(from, &rfds);

	sockfd = from;
	rx_packet_len=0x00;
	rx_packet_total_len=0x00;
	bzero(nr_tool_flag, sizeof(nr_tool_flag));

	/* Read client request, send sequence number back */
	while (wire_connected) {

		/* Wait up to  two seconds. */
		tv.tv_sec = 2;
		tv.tv_usec = 0;
		FD_SET(from, &rfds);
		res = select(from + 1, &rfds, NULL, NULL, &tv);
		if (res <= 0) { //timeout or other error
			DBG("ISP_TOOL:No data within five seconds. res:%d\n", res);
			continue;
		}

		cnt = recv(from, diag_rx_buf, CMD_BUF_SIZE,MSG_DONTWAIT);

		if (cnt <= 0) {
			DBG("ISP_TOOL:read socket error %s\n", strerror(errno));
			break;
		}

		do {
			if ((0x7e == diag_rx_buf[0])
			&&(0x00 == rx_packet_total_len)
			&&(0x00 == rx_packet_len)) {
				rx_packet_total_len= ((diag_rx_buf[6] <<0x08) | diag_rx_buf[5]) + 2;
			}

			if (cnt+rx_packet_len > rx_packet_total_len) {
				last_len = rx_packet_total_len - rx_packet_len;
				cnt = cnt - last_len;
				memcpy(temp_rx_buf, &diag_rx_buf[last_len], cnt);
				rtn = isp_RecDataCheck(diag_rx_buf, last_len, diag_cmd_buf, &cmd_len);
				if (0 == rtn) {
					handle_isp_data(diag_cmd_buf, cmd_len);
				} else {
					DBG("ISP_TOOL: rx packet comboine \n");
				}
				memcpy(diag_rx_buf, temp_rx_buf, cnt);
			} else {
				rtn = isp_RecDataCheck(diag_rx_buf, cnt, diag_cmd_buf, &cmd_len);
				if (0 == rtn) {
					handle_isp_data(diag_cmd_buf, cmd_len);
				} else {
					DBG("ISP_TOOL: rx packet comboine \n");
				}
				cnt = 0;
			}
		} while(cnt>0);
	}

	bzero(raw_filename, sizeof(raw_filename));
	if (raw_fp){
		fclose(raw_fp);
		raw_fp = NULL;
	}
	DBG("exit %s\n", strerror(errno));
	return code;
}

static void * ispserver_thread(void *args)
{
	UNUSED(args);
	uint32_t handler_id=0x00;
	struct sockaddr claddr;
	int lfd, cfd, optval;
	int log_fd;
	struct sockaddr_in sock_addr;
	socklen_t addrlen;
#ifdef CLIENT_DEBUG
#define ADDRSTRLEN (128)
	char addrStr[ADDRSTRLEN];
	char host[50];
	char service[30];
#endif
	pthread_t tdiag;
	pthread_attr_t attr;

	DBG("ISP_TOOL:isp-video server version 1.0\n");

	memset(&sock_addr, 0, sizeof (struct sockaddr_in));
	sock_addr.sin_family = AF_INET;        /* Allows IPv4*/
	sock_addr.sin_addr.s_addr = INADDR_ANY;/* Wildcard IP address;*/
	sock_addr.sin_port = htons(PORT_NUM);

	lfd = socket(sock_addr.sin_family, SOCK_STREAM, 0);
	if (lfd == -1) {
		 DBG("ISP_TOOL:socket error\n");
		 return NULL;
	}
	sock_fd=lfd;
	optval = 1;
	if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) != 0) {
		 DBG("ISP_TOOL:setsockopt error\n");
		 return NULL;
	}

	if (bind(lfd, (struct sockaddr *)&sock_addr, sizeof (struct sockaddr_in)) != 0) {
		DBG("ISP_TOOL:bind error %s\n", strerror(errno));
		 return NULL;
	}

	if (listen(lfd, BACKLOG) == -1){
		DBG("ISP_TOOL:listen error\n");
		 return NULL;
	}

	sem_init(&preview_sem_lock, 0, 0);
	sem_init(&capture_sem_lock, 0, 0);
	pthread_mutex_init(&ispstream_lock, NULL);

	pthread_attr_init(&attr);
	for (;;) {                  /* Handle clients iteratively */
		void * res;
		int ret;

		DBG("ISP_TOOL:log server waiting client dail in...\n");
		/* Accept a client connection, obtaining client's address */
		wire_connected = 0;
		addrlen = sizeof(struct sockaddr);
		cfd = accept(lfd, &claddr, &addrlen);
		if (cfd == -1) {
			DBG("ISP_TOOL:accept error %s\n", strerror(errno));
			break;
		}
		DBG("ISP_TOOL:log server connected with client\n");
		wire_connected = 1;
		sequence_num = 0;
		/* Ignore the SIGPIPE signal, so that we find out about broken
		 * connection errors via a failure from write().
		 */
		if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
			DBG("ISP_TOOL:signal error\n");
#ifdef CLIENT_DEBUG
		addrlen = sizeof(struct sockaddr);
		if (getnameinfo(&claddr, addrlen, host, 50, service,
			 30, NI_NUMERICHOST) == 0)
			snprintf(addrStr, ADDRSTRLEN, "(%s, %s)", host, service);
		else
			snprintf(addrStr, ADDRSTRLEN, "(?UNKNOWN?)");
		DBG("ISP_TOOL:Connection from %s\n", addrStr);
#endif

		//create a thread for recv cmd
		ret = pthread_create(&tdiag, &attr, isp_diag_handler, &cfd);
		if (ret != 0) {
			DBG("ISP_TOOL:diag thread create success\n");
			break;
		}

		pthread_join(tdiag, &res);
		DBG("ISP_TOOL:diag thread exit success %s\n", (char *)res);
		if (close(cfd) == -1)           /* Close connection */
			DBG("ISP_TOOL:close socket cfd error\n");
	}
	pthread_attr_destroy(&attr);
	if (close(lfd) == -1)           /* Close connection */
		DBG("ISP_TOOL:close socket lfd error\n");

	sock_fd=0x00;

	pthread_mutex_destroy(&ispstream_lock);

	 return NULL;
}

int ispvideo_RegCameraFunc(uint32_t cmd, int(*func)(uint32_t, uint32_t))
{
	struct camera_func* fun_ptr=ispvideo_GetCameraFunc();

	switch(cmd)
	{
		case REG_START_PREVIEW:
		{
			fun_ptr->start_preview = func;
			break;
		}
		case REG_STOP_PREVIEW:
		{
			fun_ptr->stop_preview = func;
			break;
		}
		case REG_TAKE_PICTURE:
		{
			fun_ptr->take_picture = func;
			break;
		}
		case REG_SET_PARAM:
		{
			fun_ptr->set_capture_size = func;
			break;
		}
		default :
		{
			break;
		}
	}

	return 0x00;
}


void stopispserver()
{
	uint32_t handler_id=0x00;

	DBG("ISP_TOOL:stopispserver \n");
	wire_connected = 0;

	if((1 == preview_flag) && (0 == preview_img_end_flag)) {
		sem_post(&preview_sem_lock);
	}
#ifndef MINICAMERA
	if(0x00!=sock_fd)
	{
		shutdown(sock_fd, 0);
		shutdown(sock_fd, 1);
	}

	ispParserFree((void*)preview_buf_ptr);

	preview_flag = 0;
	capture_flag = 0;
	preview_img_end_flag=1;
	capture_img_end_flag=1;
	preview_buf_ptr=0;
#endif
}

void startispserver()
{
	uint32_t handler_id=0x00;
	pthread_t tdiag;
	pthread_attr_t attr;
#ifdef MINICAMERA
    static int ret=-1;
#else
    int ret=-1;
#endif

	DBG("ISP_TOOL:startispserver\n");

	preview_flag = 0;
	capture_flag = 0;
	preview_img_end_flag=1;
	capture_img_end_flag=1;

	if(ret!=0){
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		ret = pthread_create(&tdiag, &attr, ispserver_thread, NULL);
		pthread_attr_destroy(&attr);
		if (ret < 0) {
			DBG("ISP_TOOL:pthread_create fail\n");
			return;
		}
		preview_buf_ptr=(uint8_t*)ispParserAlloc(PREVIEW_MAX_WIDTH*PREVIEW_MAX_HEIGHT*2);
	}else{
		DBG("ISP_TOOL:pthread already create now!\n");
	}
}

void setispserver(void* handle)
{
	isp_handler = handle;
	isp_ioctl(handle, ISP_CTRL_DENOISE_PARAM_READ,(void*)&nr_update_param);
	DBG("ISP_TOOL:setispserver %p\n", handle);
}
