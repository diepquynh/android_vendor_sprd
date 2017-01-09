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


#ifndef _SMART_PARAM_CONTAINER_H_
#define _SMART_PARAM_CONTAINER_H_

/* smart parameter container*/
/*------------------------------------------------------------------------------*
*				Dependencies					*
*-------------------------------------------------------------------------------*/
#ifndef WIN32
#include <linux/types.h>
#include <sys/types.h>
#else
#include "sci_types.h"
#endif
#include "awb_packet.h"

/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif

#define SPC_ERROR					255
#define SPC_SUCCESS					0
#define SPC_HEADER_NOT_INIT			1
#define SPC_VERIFY_NOT_MATCH		2
#define SPC_MODULE_NOT_EXIST		3
#define SPC_WRITE_NOT_INORDER		4
#define SPC_ID_NOT_FOUND			5
#define SPC_NUMBER_TOO_BIG			6
#define SPC_MEM_NOT_ENOUGH		7
#define SPC_WRITE_NULL_DATA		8

#define AWB_ALG_CT_INFO_NUM	8
#define AWB_ALG_RESOLUTION_NUM 	8
#define AWB_ALG_MWB_NUM		20
#define AWB_CTRL_SCENEMODE_NUM 10
#define AWB_ALG_ENVI_NUM	8
#define AWB_ALG_SCENE_NUM	4
#define AWB_ALG_PIECEWISE_SAMPLE_NUM	16

#define AWB_ALG_WIN_COORD_NUM	20
#define AWB_CTRL_ENVI_NUM	8

#define AWB_PACKET_VERIFY		0x87101003
#define AWB_PACKET_VERSION		0x00010001
#define AWB_PACKET_FIX_SIZE		(16 * 2)
#define AWB_CTRL_PACKET_VERIFY		0x87101003
#define AWB_CTRL_PACKET_VERSION		0x00010001
#define AWB_CTRL_PACKET_FIX_SIZE		(16 * 1)
#define AWB_MODULE_ID_CTRL_PARAM		0x1001
#define AWB_MODULE_ID_ALG_PARAM		0x1002

#define AWB_PACKET_ERROR	255
#define AWB_PACKET_SUCCESS	0

#define AWB_ALG_PACKET_VERIFY		0x87101003
#define AWB_ALG_PACKET_VERSION		0x00010001
#define AWB_ALG_PACKET_FIX_SIZE		(16 * 4)

#define AWB_ALG_MODULE_ID_COMMON		0x4001
#define AWB_ALG_MODULE_ID_WIN_MAP		0x4002
#define AWB_ALG_MODULE_ID_POS_WEIGHT		0x4003

struct spc_data {
	void *data;
	uint32_t data_size;
	uint32_t max_size;
};

struct spc_module_data {
	uint32_t id;
	void  *data;
	uint32_t data_size;
};

struct spc_init_param {
	uint32_t verify;
	uint32_t version;
	uint32_t fixed_size;
	uint32_t user_data[4];
};

struct spc_info {
	uint32_t verify;
	uint32_t version;
	uint32_t fixed_size;
	uint32_t size;
	uint32_t module_num;
	uint32_t user_data[4];
};

struct spc_write_param {
	struct spc_module_data *module_data;
	uint32_t module_num;
	uint32_t verify;
};

struct spc_header {
	uint32_t verify;
	uint32_t version;
	uint32_t fixed_size;
	uint32_t size;
	uint32_t module_num;
	uint32_t user_data[4];
	uint32_t reserved[3];
};

struct spc_module_header {
	uint32_t id;
	uint32_t size;
	uint32_t offset;
	uint32_t reserved;
};

int32_t _packet_ctrl_param_v1(struct spc_data *spc, struct awb_param_ctrl *ctrl_param,void *pack_buf, uint32_t pack_buf_size);
int32_t _packet_alg_param_v1(struct spc_data *spc, struct awb_alg_param_common *alg_param,struct awb_alg_map *alg_map,
								struct awb_alg_weight *alg_weight,void *pack_buf, uint32_t pack_buf_size);
 int32_t _ctrl_alg_param_pack_v1(struct spc_data *spc, void *ctrl_pack_buf,uint32_t ctrl_pack_buf_size, void * alg_pack_buf,
								uint32_t  alg_pack_buf_size, void *pack_buf,uint32_t pack_buf_size);



/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/
#endif
// End

