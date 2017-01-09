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
/*----------------------------------------------------------------------------*
 **				Dependencies					*
 **---------------------------------------------------------------------------*/
#include "isp_param_tune_com.h"
#include "isp_param_size.h"
#include "isp_app.h"
#include "isp_video.h"

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

/**---------------------------------------------------------------------------*
*				Data Prototype					*
**----------------------------------------------------------------------------*/

static int32_t _ispParamVerify(void* in_param_ptr)
{
	int32_t rtn=0x00;
	uint32_t* param_ptr=(uint32_t*)in_param_ptr;
	uint32_t verify_id=param_ptr[0];
	uint32_t packet_size=param_ptr[2];
	uint32_t end_id=param_ptr[(packet_size/4)-1];

	if(ISP_PACKET_VERIFY_ID!=verify_id)
	{
		rtn=0x01;
	}

	if(ISP_PACKET_END_ID!=end_id)
	{
		rtn=0x01;
	}

	return rtn;
}

static uint32_t _ispParserGetType(void* in_param_ptr)
{
	uint32_t* param_ptr=(uint32_t*)in_param_ptr;
	uint32_t type=param_ptr[2];

	return type;
}

static int32_t _ispParserDownParam(isp_handle isp_handler, void* in_param_ptr)
{
	uint32_t rtn=0x00;
	uint32_t* param_ptr=(uint32_t*)((uint8_t*)in_param_ptr+0x0c);// packet data

	// version_id||module_id||packet_len||module_info||data
	uint32_t version_id=param_ptr[0];
	uint32_t module_id=param_ptr[1];
	uint32_t packet_len=param_ptr[2];
	uint32_t module_bypass=param_ptr[3];
	void* data_addr=(void*)&param_ptr[4];
	uint32_t data_len =packet_len-0x10;
	uint32_t mode_offset = 0;
	SENSOR_EXP_INFO_T* sensor_info_ptr=Sensor_GetInfo();
	CMR_LOGE(" _ispParserDownParam");

	while(mode_offset<data_len) {
		struct isp_mode_param* mode_param_ptr = (struct isp_mode_param*)((char*)data_addr+mode_offset);
		CMR_LOGE(" _ispParserDownParam mode_param_ptr->mode_id =%d",mode_param_ptr->mode_id);

		if(0!=mode_param_ptr->size) {
			memcpy(sensor_info_ptr->raw_info_ptr->mode_ptr[mode_param_ptr->mode_id].addr,(char*)data_addr+mode_offset,mode_param_ptr->size);
			sensor_info_ptr->raw_info_ptr->mode_ptr[mode_param_ptr->mode_id].len = mode_param_ptr->size;
			mode_offset += mode_param_ptr->size;
		}
	}

	rtn = isp_ioctl(isp_handler, ISP_CTRL_PARAM_UPDATE|ISP_TOOL_CMD_ID, data_addr);

	return rtn;
}

static int32_t _ispParserDownLevel(isp_handle isp_handler, void* in_param_ptr)
{
	int32_t rtn=0x00;
	uint32_t* param_ptr=(uint32_t*)((uint8_t*)in_param_ptr+0x0c);// packet data
	uint32_t module_id=param_ptr[0];
	enum isp_ctrl_cmd cmd=ISP_CTRL_MAX;
	void* ioctl_param_ptr=NULL;

	cmd=module_id;

	//CMR_LOGE("ISP_TOOL:Level cmd :%d level :%d \n", cmd, level);
	//CMR_LOGE("ISP_TOOL:0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x \n", param_ptr[0], param_ptr[1], param_ptr[2], param_ptr[3], param_ptr[4],param_ptr[5]);

	if(ISP_CTRL_AF==cmd)
	{
		SENSOR_EXP_INFO_T_PTR sensor_info_ptr=Sensor_GetInfo();
		struct isp_af_win af_param;
		struct isp_af_win* in_af_ptr=(struct isp_af_win*)&param_ptr[3];
		uint16_t img_width=(param_ptr[2]>>0x10)&0xffff;
		uint16_t img_height=param_ptr[2]&0xffff;
		uint16_t prv_width=sensor_info_ptr->sensor_mode_info[1].width;
		uint16_t prv_height=sensor_info_ptr->sensor_mode_info[1].height;
		uint32_t i=0x00;

		CMR_LOGE("zone prv_width=%d prv_height=%d",prv_width,prv_height);

		af_param.valid_win=in_af_ptr->valid_win;
		af_param.mode=in_af_ptr->mode;
		for(i=0x00; i<af_param.valid_win; i++)
		{
			af_param.win[i].start_x=(in_af_ptr->win[i].start_x*prv_width)/img_width;
			af_param.win[i].start_y=(in_af_ptr->win[i].start_y*prv_height)/img_height;
			af_param.win[i].end_x=(in_af_ptr->win[i].end_x*prv_width)/img_width;
			af_param.win[i].end_y=(in_af_ptr->win[i].end_y*prv_height)/img_height;
		}
		ioctl_param_ptr=(void*)&af_param;
	} else {
		ioctl_param_ptr=(void*)&param_ptr[2];
	}

	cmd|=ISP_TOOL_CMD_ID;

	rtn=isp_ioctl(isp_handler, cmd, ioctl_param_ptr);

	return rtn;
}

static int32_t _ispParserUpMainInfo(void* rtn_param_ptr)
{
	uint32_t rtn=0x00;
	SENSOR_EXP_INFO_T_PTR sensor_info_ptr=Sensor_GetInfo();
	struct isp_parser_buf_rtn* rtn_ptr=(struct isp_parser_buf_rtn*)rtn_param_ptr;
	uint32_t i=0x00;
	uint32_t j=0x00;
	uint32_t* data_addr=NULL;
	uint32_t data_len =0x10;
	struct isp_main_info* param_ptr=NULL;

	data_len=sizeof(struct isp_main_info);
	data_addr=ispParserAlloc(data_len);
	if (!data_addr) {
		CMR_LOGE("ispParserAlloc fail");
		return -1;
	}
	param_ptr=(struct isp_main_info*)data_addr;

	memset((void*)data_addr, 0x00, sizeof(struct isp_main_info));

	rtn_ptr->buf_addr=(unsigned long)data_addr;
	rtn_ptr->buf_len=sizeof(struct isp_main_info);

	if((NULL!=sensor_info_ptr)
		/*&&(NULL!=sensor_info_ptr->raw_info_ptr)*/)
	{
		/*get sensor name*/
		strcpy((char*)&param_ptr->sensor_id, sensor_info_ptr->name);
		if(NULL != sensor_info_ptr->raw_info_ptr)
			param_ptr->version_id = sensor_info_ptr->raw_info_ptr->version_info->version_id;
		else
			param_ptr->version_id = TOOL_DEFAULT_VER;

		/*set preview param*/
		param_ptr->preview_size=(sensor_info_ptr->sensor_mode_info[1].width<<0x10)&0xffff0000;
		param_ptr->preview_size|=sensor_info_ptr->sensor_mode_info[1].height&0xffff;

		for(i=SENSOR_MODE_PREVIEW_ONE; i<SENSOR_MODE_MAX; i++)
		{
			if((0x00!=sensor_info_ptr->sensor_mode_info[i].width)
				&&(0x00!=sensor_info_ptr->sensor_mode_info[i].height))
			{
				param_ptr->capture_size[param_ptr->capture_num]=(sensor_info_ptr->sensor_mode_info[i].trim_width<<0x10)&0xffff0000;
				param_ptr->capture_size[param_ptr->capture_num]|=sensor_info_ptr->sensor_mode_info[i].trim_height&0xffff;
				param_ptr->capture_num++;
			} else {
				break ;
			}
		}
		/* new format hsb no equal to zero,*/
		param_ptr->capture_num|=0x00010000;

		param_ptr->preview_format = ISP_VIDEO_YVU420_2FRAME;
		param_ptr->capture_format = ISP_VIDEO_YVU420_2FRAME|ISP_VIDEO_JPG;

		if(NULL!=sensor_info_ptr->raw_info_ptr) {
			if(0x00==sensor_info_ptr->sensor_interface.is_loose){
				param_ptr->capture_format |= ISP_VIDEO_MIPI_RAW10;
			} else {
				param_ptr->capture_format |= ISP_VIDEO_NORMAL_RAW10;
			}
		} else {
			strcat((char*)&param_ptr->sensor_id, "--YUV");
		}
	} else {
#if  1
		strcpy((char*)&param_ptr->sensor_id, "no sensor identified");
		param_ptr->version_id = TOOL_DEFAULT_VER;
#else
		rtn_ptr->buf_addr = (uint32_t)NULL;
		rtn_ptr->buf_len = 0x00;
		ispParserFree(data_addr);
		rtn = 0x01;
#endif
	}

	return rtn;
}

static int32_t _ispParserUpParam(void* rtn_param_ptr)
{
	int32_t rtn=0x00;
	int32_t i =0;
	SENSOR_EXP_INFO_T_PTR sensor_info_ptr=Sensor_GetInfo();
	uint32_t version_id=sensor_info_ptr->raw_info_ptr->version_info->version_id;
	struct sensor_raw_info* sensor_raw_info_ptr=(struct sensor_raw_info*)sensor_info_ptr->raw_info_ptr;
	struct isp_parser_buf_rtn* rtn_ptr=(struct isp_parser_buf_rtn*)rtn_param_ptr;
	uint32_t* data_addr=NULL;
	uint32_t data_len=0;
	uint32_t data_offset=0;

	for(i =0; i<MAX_MODE_NUM;i++){
		data_len += sensor_raw_info_ptr->mode_ptr[i].len;
	}

	rtn_ptr->buf_len=data_len+0x10;
	data_addr=ispParserAlloc(rtn_ptr->buf_len);
	rtn_ptr->buf_addr=(unsigned long)data_addr;

	if(NULL!=data_addr)
	{
		// packet cfg
		// version_id||module_id||packet_len||module_info||data
		data_addr[0]=version_id;
		data_addr[1]=ISP_PACKET_ALL;
		data_addr[2]=data_len;
		data_addr[3]=0x00;

		for(i =0; i<MAX_MODE_NUM;i++){
			if(NULL!=sensor_raw_info_ptr->mode_ptr[i].addr){
				memcpy((char*)&data_addr[4]+data_offset, (char*)sensor_raw_info_ptr->mode_ptr[i].addr, sensor_raw_info_ptr->mode_ptr[i].len);
				data_offset += sensor_raw_info_ptr->mode_ptr[i].len;
				CMR_LOGI("ISP_TOOL:_sensor_raw_info_ptr->mode_ptr[i].len: %d   i=  %d\n",sensor_raw_info_ptr->mode_ptr[i].len,i);


			}
		}
		CMR_LOGI("ISP_TOOL:_ispParserUpParam data_offset: %d\n",data_offset);

	}
	else
	{
		rtn_ptr->buf_addr = (unsigned long)NULL;
		rtn_ptr->buf_len = 0x00;
	}

	return rtn;
}

static int32_t _ispParserUpdata(void* in_param_ptr, void* rtn_param_ptr)
{
	int32_t rtn=0x00;
	struct isp_parser_up_data* in_ptr=(struct isp_parser_up_data*)in_param_ptr;
	struct isp_parser_buf_rtn* rtn_ptr=(struct isp_parser_buf_rtn*)rtn_param_ptr;
	uint32_t* data_addr=NULL;
	uint32_t data_len=0x10;

	data_len+=in_ptr->buf_len;
	data_addr=ispParserAlloc(data_len);

	if(NULL!=data_addr)
	{
		data_addr[0]=data_len;
		data_addr[1]=in_ptr->format;
		data_addr[2]=in_ptr->pattern;
		data_addr[3]=in_ptr->width;
		data_addr[4]=in_ptr->height;

		memcpy((void*)&data_addr[5], (void*)in_ptr->buf_addr, in_ptr->buf_len);

		// rtn sesult
		rtn_ptr->buf_addr = (unsigned long)data_addr;
		rtn_ptr->buf_len = data_len;
	}

	return rtn;
}

static int32_t _ispParserCapturesize(void* in_param_ptr, void* rtn_param_ptr)
{
	int32_t rtn=0x00;
	struct isp_parser_cmd_param* in_ptr=(struct isp_parser_cmd_param*)in_param_ptr;
	struct isp_parser_buf_rtn* rtn_ptr=(struct isp_parser_buf_rtn*)rtn_param_ptr;
	uint32_t* data_addr=NULL;
	uint32_t data_len=0x0c;

	data_addr=ispParserAlloc(data_len);

	if(NULL!=data_addr)
	{
		data_addr[0]=in_ptr->cmd;
		data_addr[1]=data_len;
		data_addr[2]=in_ptr->param[0];

		// rtn sesult
		rtn_ptr->buf_addr = (unsigned long)data_addr;
		rtn_ptr->buf_len = data_len;
	}

	return rtn;
}

static int32_t _ispParserReadSensorReg(void* in_param_ptr, void* rtn_param_ptr)
{
	int32_t rtn=0x00;
	struct isp_parser_cmd_param* in_ptr=(struct isp_parser_cmd_param*)in_param_ptr;
	struct isp_parser_buf_rtn* rtn_ptr=(struct isp_parser_buf_rtn*)rtn_param_ptr;
	uint32_t* data_addr=NULL;
	uint32_t data_len=0x0c;
	uint32_t reg_num=in_ptr->param[0];
	uint16_t reg_addr=(uint16_t)in_ptr->param[1];
	uint16_t reg_data=0x00;
	uint32_t i=0x00;

	rtn_ptr->buf_addr = (unsigned long)NULL;
	rtn_ptr->buf_len = 0x00;

	data_len+=reg_num*0x08;
	data_addr=ispParserAlloc(data_len);

	if(NULL!=data_addr)
	{
		data_addr[0]=in_ptr->cmd;
		data_addr[1]=data_len;
		data_addr[2]=reg_num;

		for(i=0x00; i<reg_num; i++)
		{
			reg_data=Sensor_ReadReg(reg_addr);
			data_addr[3+i*2]=(uint32_t)reg_addr;
			data_addr[4+i*2]=(uint32_t)reg_data;
			reg_addr++;
		}

		// rtn sesult
		rtn_ptr->buf_addr = (unsigned long)data_addr;
		rtn_ptr->buf_len = data_len;
	}

	return rtn;
}

static int32_t _ispParserWriteSensorReg(void* in_param_ptr)
{
	int32_t rtn=0x00;
	// cmd|packet_len|reg_num|reg_addr|reg_data|...
	struct isp_parser_cmd_param* in_ptr=(struct isp_parser_cmd_param*)in_param_ptr;
	uint32_t reg_num=in_ptr->param[1];
	uint16_t reg_addr=0x00;
	uint16_t reg_data=0x00;
	uint32_t i=0x00;

	for(i=0x00; i<reg_num; i++)
	{
		reg_addr=(uint16_t)in_ptr->param[2+i*2];
		reg_data=(uint16_t)in_ptr->param[3+i*2];
		Sensor_WriteReg(reg_addr, reg_data);
	}

	return rtn;
}

static int32_t _ispParserGetInfo(isp_handle isp_handler, void* in_param_ptr, void* rtn_param_ptr)
{
	int32_t rtn=0x00;
	uint32_t* param_ptr=(uint32_t*)in_param_ptr;
	struct isp_parser_buf_rtn* rtn_ptr=(struct isp_parser_buf_rtn*)rtn_param_ptr;
	uint32_t cmd_id=param_ptr[0];
	enum isp_ctrl_cmd cmd=param_ptr[1];
	void* ioctl_param_ptr=NULL;
	uint32_t* data_addr=NULL;
	uint32_t data_len=0x14+param_ptr[2];

	rtn_ptr->buf_addr = 0;
	rtn_ptr->buf_len = 0x00;

	CMR_LOGI("ISP_TOOL:_ispParserGetInfo %d\n", (uint32_t)cmd);

	data_addr=ispParserAlloc(data_len);

	if(NULL!=data_addr)
	{
		data_addr[0]=data_len;
		data_addr[1]=0x14;
		data_addr[2]=cmd;
		data_addr[3]=0x00;
		data_addr[4]=0x00;
		ioctl_param_ptr=(void*)&data_addr[5];
		memcpy((void*)&data_addr[5], (void*)&param_ptr[3], param_ptr[2]);
	}

	cmd|=ISP_TOOL_CMD_ID;

	rtn=isp_ioctl(isp_handler, cmd, ioctl_param_ptr);

	rtn_ptr->buf_addr = (unsigned long)data_addr;
	rtn_ptr->buf_len = data_len;

	return rtn;
}

static int32_t _ispParserDownCmd(void* in_param_ptr, void* rtn_param_ptr)
{
	int32_t rtn=0x00;
	uint32_t* param_ptr=(uint32_t*)in_param_ptr+0x03;
	struct isp_parser_cmd_param* rtn_ptr=(struct isp_parser_cmd_param*)rtn_param_ptr;
	uint32_t cmd=param_ptr[0];
	uint32_t i=0x00;
	struct isp_size_info* size_info_ptr=ISP_ParamGetSizeInfo();

	rtn_ptr->cmd=cmd;

	CMR_LOGI("ISP_TOOL:_ispParserDownCmd type: 0x%x, 0x%x, 0x%x\n", param_ptr[0], param_ptr[1], param_ptr[2]);

	switch(cmd)
	{
		case ISP_CAPTURE:
		{
			rtn_ptr->param[0]=param_ptr[2];/*format*/

			if (0x00 !=(param_ptr[3]&0xffff0000)) {
				rtn_ptr->param[1]=(param_ptr[3]>>0x10)&0xffff; /*width*/
				rtn_ptr->param[2]=param_ptr[3]&0xffff; /*height*/
			} else {
				for(i=0x00; ISP_SIZE_END!=size_info_ptr[i].size_id; i++) {
					if(size_info_ptr[i].size_id==param_ptr[3]) {
						rtn_ptr->param[1]=size_info_ptr[i].width;//width
						rtn_ptr->param[2]=size_info_ptr[i].height;//height
						break ;
					}
				}
			}
			break;
		}
		case ISP_READ_SENSOR_REG:
		{
			rtn_ptr->param[0]=param_ptr[2];//addr num
			rtn_ptr->param[1]=param_ptr[3];//start addr
			break;
		}
		case ISP_WRITE_SENSOR_REG:
		{
			_ispParserWriteSensorReg((void*)param_ptr);
			break;
		}
		case ISP_PREVIEW:
		case ISP_STOP_PREVIEW:
		case ISP_UP_PREVIEW_DATA:
		case ISP_UP_PARAM:
		case ISP_TAKE_PICTURE_SIZE:
		case ISP_MAIN_INFO:
		{
			break;
		}
		case ISP_INFO:
		{
			rtn_ptr->param[0]=param_ptr[2];//thrd cmd
			memcpy((void*)&rtn_ptr->param[1], (void*)&param_ptr[3], param_ptr[3]+0x04);
			CMR_LOGI("ISP_TOOL:_ispParserDownCmd thrd cmd: 0x%x\n", rtn_ptr->param[0]);
			break;
		}
		default :
		{
			break;
		}
	}

	return rtn;
}

static int32_t _ispParserDownHandle(isp_handle isp_handler, void* in_param_ptr, void* rtn_param_ptr)
{
	int32_t rtn=0x00;
	uint32_t* param_ptr=(uint32_t*)in_param_ptr; //packet
	uint32_t type=param_ptr[1];

	rtn=_ispParamVerify(in_param_ptr);

	CMR_LOGI("ISP_TOOL:_ispParserDownHandle param: 0x%x, 0x%x, 0x%x\n", param_ptr[0], param_ptr[1], param_ptr[2]);

	CMR_LOGI("ISP_TOOL:_ispParserDownHandle type: 0x%x\n", type);

	switch(type)
	{
		case ISP_TYPE_CMD:
		{
			rtn=_ispParserDownCmd(in_param_ptr, rtn_param_ptr);
			break;
		}
		case ISP_TYPE_PARAM:
		{
			rtn=_ispParserDownParam(isp_handler, in_param_ptr);
			break;
		}
		case ISP_TYPE_LEVEL:
		{
			rtn=_ispParserDownLevel(isp_handler, in_param_ptr);
			break;
		}
		default :
		{
			break;
		}
	}

	return rtn;
}

static int32_t _ispParserUpHandle(isp_handle isp_handler, uint32_t cmd, void* in_param_ptr, void* rtn_param_ptr)
{
	int32_t rtn=0x00;
	struct isp_parser_up_data* in_ptr=(struct isp_parser_up_data*)in_param_ptr;
	struct isp_parser_buf_rtn* rtn_ptr=(struct isp_parser_buf_rtn*)rtn_param_ptr;
	uint32_t* data_addr=NULL;
	uint32_t data_len=0x10;

	CMR_LOGI("ISP_TOOL:_ispParserUpHandle %d\n", cmd);

	switch(cmd)
	{
		case ISP_PARSER_UP_MAIN_INFO:
		{
			rtn=_ispParserUpMainInfo(rtn_param_ptr);
			break;
		}
		case ISP_PARSER_UP_PARAM:
		{
			rtn=_ispParserUpParam(rtn_param_ptr);
			break;
		}
		case ISP_PARSER_UP_PRV_DATA:
		case ISP_PARSER_UP_CAP_DATA:
		{
			rtn=_ispParserUpdata(in_param_ptr, rtn_param_ptr);
			break;
		}
		case ISP_PARSER_UP_CAP_SIZE:
		{
			rtn=_ispParserCapturesize(in_param_ptr, rtn_param_ptr);
			break;
		}
		case ISP_PARSER_UP_SENSOR_REG:
		{
			rtn=_ispParserReadSensorReg(in_param_ptr, rtn_param_ptr);
			break;
		}
		case ISP_PARSER_UP_INFO:
		{
			CMR_LOGI("ISP_TOOL:ISP_PARSER_UP_INFO %d\n", cmd);

			rtn=_ispParserGetInfo(isp_handler, in_param_ptr, rtn_param_ptr);
			break;
		}
		default :
		{
			break;
		}
	}

	if(0x00==rtn)
	{
		data_len+=rtn_ptr->buf_len;
		data_addr=ispParserAlloc(data_len);

		if(NULL!=data_addr)
		{
			data_addr[0]=ISP_PACKET_VERIFY_ID;

			switch(cmd)
			{
				case ISP_PARSER_UP_PARAM:
				{
					data_addr[1]=ISP_TYPE_PARAM;
					break;
				}
				case ISP_PARSER_UP_PRV_DATA:
				{
					data_addr[1]=ISP_TYPE_PRV_DATA;
					break;
				}
				case ISP_PARSER_UP_CAP_DATA:
				{
					data_addr[1]=ISP_TYPE_CAP_DATA;
					break;
				}
				case ISP_PARSER_UP_CAP_SIZE:
				{
					data_addr[1]=ISP_TYPE_CMD;
					break;
				}
				case ISP_PARSER_UP_MAIN_INFO:
				{
					data_addr[1]=ISP_TYPE_MAIN_INFO;
					break;
				}
				case ISP_PARSER_UP_SENSOR_REG:
				{
					data_addr[1]=ISP_TYPE_SENSOR_REG;
					break;
				}
				case ISP_PARSER_UP_INFO:
				{
					data_addr[1]=ISP_TYPE_INFO;
					break;
				}
				default :
				{
					break;
				}
			}

			data_addr[2]=data_len;

			memcpy((void*)&data_addr[3], (void*)rtn_ptr->buf_addr, rtn_ptr->buf_len);

			data_addr[(data_len>>0x02)-0x01]=ISP_PACKET_END_ID;

			ispParserFree((void*)rtn_ptr->buf_addr);

			// rtn sesult
			rtn_ptr->buf_addr = (unsigned long)data_addr;
			rtn_ptr->buf_len = data_len;
		}
	}

	return rtn;
}

uint32_t ispParserGetSizeID(uint32_t width, uint32_t height)
{
	uint32_t i=0x00;
	uint32_t size_id=0x00;
	struct isp_size_info* size_info_ptr=ISP_ParamGetSizeInfo();

	for(i=0x00; ISP_SIZE_END!=size_info_ptr[i].size_id; i++)
	{
		if((size_info_ptr[i].width==width)
			&&(size_info_ptr[i].height==height))
		{
			size_id=size_info_ptr[i].size_id;
			break ;
		}
	}

	return size_id;
}

int32_t ispParser(isp_handle isp_handler, uint32_t cmd, void* in_param_ptr, void* rtn_param_ptr)
{
	int32_t rtn=0x00;

	CMR_LOGI("ISP_TOOL:cmd = %d\n", cmd);

	switch(cmd)
	{
		case ISP_PARSER_UP_MAIN_INFO:
		case ISP_PARSER_UP_PARAM:
		case ISP_PARSER_UP_PRV_DATA:
		case ISP_PARSER_UP_CAP_DATA:
		case ISP_PARSER_UP_CAP_SIZE:
		case ISP_PARSER_UP_SENSOR_REG:
		case ISP_PARSER_UP_INFO:
		{
			rtn=_ispParserUpHandle(isp_handler, cmd, in_param_ptr, rtn_param_ptr);
			break;
		}
		case ISP_PARSER_DOWN:
		{
			rtn=_ispParserDownHandle(isp_handler, in_param_ptr, rtn_param_ptr);
			break;
		}

		default :
		{
			break;
		}
	}

	return rtn;
}

uint32_t* ispParserAlloc(uint32_t size)
{
	uint32_t* addr=0x00;

	if(0x00!=size)
	{
		addr=(uint32_t*)malloc(size);
	}

	return addr;
}

int32_t ispParserFree(void* addr)
{
	int32_t rtn=0x00;
	void* temp_addr=addr;

	if(NULL!=temp_addr)
	{
		free(temp_addr);
	}

	return rtn;
}
/**----------------------------------------------------------------------------*
**				Compiler Flag					*
**----------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/

// End

