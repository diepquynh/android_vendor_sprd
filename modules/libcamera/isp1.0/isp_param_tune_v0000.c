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
**			Dependencies						*
**---------------------------------------------------------------------------*/
#include "isp_param_tune_com.h"
#include "isp_param_tune_v0000.h"
#include "isp_app.h"
#include "cmr_common.h"
#include "sensor_raw.h"

/**---------------------------------------------------------------------------*
**			Compiler Flag						*
**---------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif

/**---------------------------------------------------------------------------*
**			Micro Define						**
**----------------------------------------------------------------------------*/
#define ISP_SUCCESS 0x00

/**---------------------------------------------------------------------------*
**			Data Prototype						**
**----------------------------------------------------------------------------*/
static int32_t _ispParamIDCheck(char* str1, char* str2)
{
	int32_t rtn = ISP_SUCCESS;

	rtn = strcmp(str1, str2);

	return rtn;
}

static int32_t _ispSetAllParamV0000(void* in_param_ptr)
{
	int32_t rtn=0x00;
	// version_id||module_id||packet_len||module_info||data
	uint32_t* param_ptr=(uint32_t*)in_param_ptr;
	uint32_t version_id=param_ptr[0];
	uint32_t module_id=param_ptr[1];
	uint32_t packet_len=param_ptr[2];
	uint32_t module_bypass=param_ptr[3];
	void* data_addr=(void*)&param_ptr[4];
	uint32_t data_len =packet_len-0x10;
	uint32_t copy_size = 0;
	SENSOR_EXP_INFO_T* sensor_info_ptr=Sensor_GetInfo();
	void* raw_tune_ptr=(void*)sensor_info_ptr->raw_info_ptr->tune_ptr;

	struct sensor_raw_tune_info* raw_ptr=sensor_info_ptr->raw_info_ptr->tune_ptr;
	struct sensor_raw_tune_info* tune_ptr=(struct sensor_raw_tune_info*)data_addr;

	//CMR_LOGE("ISP_TOOL:--raw_ptr-- detail_thr:0x%x,smooth_thr: 0x%x, strength:0x%x",raw_ptr->edge.info[0].detail_thr,raw_ptr->edge.info[0].smooth_thr,raw_ptr->edge.info[0].strength);
	//CMR_LOGE("ISP_TOOL:--tune_ptr-- detail_thr:0x%x,smooth_thr: 0x%x, strength:0x%x",tune_ptr->edge.info[0].detail_thr,tune_ptr->edge.info[0].smooth_thr,tune_ptr->edge.info[0].strength);

	copy_size = sizeof(*sensor_info_ptr->raw_info_ptr->tune_ptr);
	if (copy_size > data_len) {
		copy_size = data_len;
	}

	if((NULL!=raw_tune_ptr)&&(NULL!=data_addr)&&(0x00!=copy_size)) {
		//CMR_LOGE("ISP_TOOL:_ispSetAllParamV0000:raw_tune_ptr:0x%x, data_addr:0x%x, data_len:0x%x tune_len:0x%x\n", raw_tune_ptr, data_addr, data_len, sizeof(struct sensor_raw_tune_info));
		memcpy(raw_tune_ptr, data_addr, copy_size);
		isp_ioctl(NULL, ISP_CTRL_PARAM_UPDATE|ISP_TOOL_CMD_ID, NULL);
	} else {
		rtn = 0x01;
	}

	return rtn;
}

static int32_t _ispSetLncParamV0000(void* in_param_ptr)
{
	int32_t rtn=0x00;
	// version_id||module_id||packet_len||module_info||data
	uint32_t* param_ptr=(uint32_t*)in_param_ptr;
	uint32_t version_id=param_ptr[0];
	uint32_t module_id=param_ptr[1];
	uint32_t packet_len=param_ptr[2];
	uint32_t module_info=param_ptr[3];
	void* data_addr=(void*)&param_ptr[4];
	uint32_t data_len =packet_len-0x10;
	uint32_t size_id=(module_info>>0x04)&0x0f;
	uint32_t awb_id=module_info&0x0f;
	SENSOR_EXP_INFO_T* sensor_info_ptr=Sensor_GetInfo();
	void* lnc_ptr=(void*)sensor_info_ptr->raw_info_ptr->fix_ptr->lnc.map[size_id][awb_id].param_addr;
	uint32_t lnc_param_len=sensor_info_ptr->raw_info_ptr->fix_ptr->lnc.map[size_id][awb_id].len;
	uint16_t* lnc_param_ptr=data_addr;
	uint16_t* lnc_src_ptr=(uint16_t*)lnc_ptr;

	if((NULL!=lnc_ptr)&&(NULL!=data_addr)&&(0x00!=data_len)&&(lnc_param_len==data_len)){
		memcpy(lnc_ptr, data_addr, data_len);
	} else {
		rtn = 0x01;
	}

	return rtn;
}

static int32_t _ispSetAwbParamV0000(void* in_param_ptr)
{
	int32_t rtn=0x00;
	// version_id||module_id||packet_len||module_info||data
	uint32_t* param_ptr=(uint32_t*)in_param_ptr;
	uint32_t version_id=param_ptr[0];
	uint32_t module_id=param_ptr[1];
	uint32_t packet_len=param_ptr[2];
	uint32_t module_info=param_ptr[3];
	void* data_addr=(void*)&param_ptr[4];
	uint32_t data_len =packet_len-0x10;
	uint32_t size_id=(module_info>>0x04)&0x0f;
	uint32_t awb_id=module_info&0x0f;

	SENSOR_EXP_INFO_T* sensor_info_ptr=Sensor_GetInfo();
	void* map_addr=(void*)sensor_info_ptr->raw_info_ptr->fix_ptr->awb.addr;
	uint32_t map_len=sensor_info_ptr->raw_info_ptr->fix_ptr->awb.len;

	if (NULL != map_addr && NULL != data_addr && data_len <= map_len){
		memcpy(map_addr, data_addr, data_len);
	} else {
		return 0x01;
	}

	return rtn;
}

struct isp_param_fun s_isp_fun_tab_v0000[]=
{
	{ISP_PACKET_ALL,            _ispSetAllParamV0000},
	{ISP_PACKET_BLC,            PNULL},
	{ISP_PACKET_NLC,            PNULL},
	{ISP_PACKET_LNC,            PNULL},
	{ISP_PACKET_AE,             PNULL},
	{ISP_PACKET_AWB,            PNULL},
	{ISP_PACKET_BPC,            PNULL},
	{ISP_PACKET_DENOISE,        PNULL},
	{ISP_PACKET_GRGB,           PNULL},
	{ISP_PACKET_CFA,            PNULL},
	{ISP_PACKET_CMC,            PNULL},
	{ISP_PACKET_GAMMA,          PNULL},
	{ISP_PACKET_UV_DIV,         PNULL},
	{ISP_PACKET_PREF,           PNULL},
	{ISP_PACKET_BRIGHT,         PNULL},
	{ISP_PACKET_CONTRAST,       PNULL},
	{ISP_PACKET_HIST,           PNULL},
	{ISP_PACKET_AUTO_CONTRAST,  PNULL},
	{ISP_PACKET_SATURATION,     PNULL},
	{ISP_PACKET_CSS,            PNULL},
	{ISP_PACKET_AF,             PNULL},
	{ISP_PACKET_EDGE,           PNULL},
	{ISP_PACKET_SPECIAL_EFFECT, PNULL},
	{ISP_PACKET_HDR,            PNULL},
	{ISP_PACKET_GLOBAL,         PNULL},
	{ISP_PACKET_CHN,            PNULL},
	{ISP_PACKET_LNC_PARAM,      _ispSetLncParamV0000},
	{ISP_PACKET_AWB_MAP,        _ispSetAwbParamV0000},
	{ISP_PACKET_MAX,            PNULL}
};

isp_fun ispGetDownParamFunV0000(uint32_t cmd)
{
	struct isp_param_fun* isp_fun_ptr=(struct isp_param_fun*)&s_isp_fun_tab_v0000;
	isp_fun fun_ptr=PNULL;
	uint32_t i=0x00;

	for(i=0x00; i<(sizeof(s_isp_fun_tab_v0000)/sizeof(s_isp_fun_tab_v0000[0])); i++)
	{
		if(ISP_PACKET_MAX==isp_fun_ptr[i].cmd)
		{
			break ;
		}

		if(cmd==isp_fun_ptr[i].cmd)
		{
			fun_ptr=isp_fun_ptr[i].param_fun;
			break ;
		}
	}

	return fun_ptr;
}

int32_t ispGetUpParamV0000(void*param_ptr, void* rtn_param_ptr)
{
	int32_t rtn=0x00;
	uint32_t* offset_ptr=(uint32_t*)param_ptr;
	struct isp_parser_buf_rtn* rtn_ptr=(struct isp_parser_buf_rtn*)rtn_param_ptr;
	uint32_t* data_addr=NULL;
	uint32_t data_len=0x00;
	SENSOR_EXP_INFO_T_PTR sensor_info_ptr=Sensor_GetInfo();
	struct sensor_raw_info* sensor_raw_info_ptr=(struct sensor_raw_info*)sensor_info_ptr->raw_info_ptr;
	struct sensor_version_info* raw_version_info_ptr=sensor_raw_info_ptr->version_info;
	struct sensor_raw_tune_info* raw_tune_ptr=sensor_raw_info_ptr->tune_ptr;

	CMR_LOGE("ISP_TOOL:sensor_raw_info_ptr:0x%lx:0x%lx:0x%lx",(unsigned long)sensor_raw_info_ptr, (unsigned long)raw_version_info_ptr, (unsigned long)raw_tune_ptr);

	ispGetParamSizeV0000(&data_len);
	rtn_ptr->buf_len=data_len+0x10;
	data_addr=ispParserAlloc(rtn_ptr->buf_len);
	rtn_ptr->buf_addr=(unsigned long)data_addr;

	CMR_LOGE("ISP_TOOL:sensor_raw_info_ptr:0x%lx:0x%lx:0x%x:0x%x",(unsigned long)data_addr, rtn_ptr->buf_addr, rtn_ptr->buf_len, data_len);

	if(NULL!=data_addr)
	{
		// packet cfg
		// version_id||module_id||packet_len||module_info||data
		data_addr[0]=raw_version_info_ptr->version_id;
		data_addr[1]=ISP_PACKET_ALL;
		data_addr[2]=data_len;
		data_addr[3]=0x00;
		// tab
		memcpy((void*)&data_addr[4], (void*)raw_tune_ptr, data_len);
	}
	else
	{
		rtn_ptr->buf_addr = (unsigned long)NULL;
		rtn_ptr->buf_len = 0x00;
	}
	return rtn;
}

int32_t ispGetParamSizeV0000(uint32_t* param_len)
{
	int32_t rtn=0x00;
	uint32_t data_len=0x10;

	data_len+=sizeof(struct sensor_raw_tune_info);
	*param_len=data_len;

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
