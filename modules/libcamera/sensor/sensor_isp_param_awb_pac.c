/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*smart parameter container*/
#include "sensor_isp_param_awb_pac.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/**---------------------------------------------------------------------------*
**				Micro Define					*
**----------------------------------------------------------------------------*/
#define AWB_PACKET_VERIFY		0x87101003
#define AWB_PACKET_VERSION		0x00010001
#define AWB_PACKET_FIX_SIZE		(16 * 2)
#define AWB_CTRL_PACKET_VERIFY		0x87101003
#define AWB_CTRL_PACKET_VERSION		0x00010001
#define AWB_CTRL_PACKET_FIX_SIZE		(16 * 1)
#define AWB_MODULE_ID_CTRL_PARAM		0x1001
#define AWB_MODULE_ID_ALG_PARAM		0x1002


/* _relayout_spc_data --read the module data from spc data by index
*@
*@
*@ return:
*/
static int32_t _relayout_spc_data(struct spc_data *spcdata, uint32_t relayout_module_index, struct spc_module_data *relayout_module_data)
{
	int32_t rtn = SPC_SUCCESS;
	uint32_t i = 0;
	struct spc_module_header *tmp_module_header = NULL;
	struct spc_module_header *dst_module_header_last = NULL;
	struct spc_module_header *dst_module_header = NULL;
	struct spc_header *dst_header = NULL,*tmp_header = NULL;
	uint32_t spc_data_tmp_size = 0;
	uint32_t spc_data_dst_size = 0;
	uint32_t spc_data_max_size = 0;
	struct spc_data tmp_buf;
	uint8_t *dst_addr = NULL;
	uint8_t *tmp_addr = NULL;

	spc_data_max_size = spcdata->max_size;
	spc_data_tmp_size = spcdata->data_size;
	tmp_buf.data= (void *)malloc(spc_data_tmp_size);
	if (NULL == tmp_buf.data)
		return SPC_MEM_NOT_ENOUGH;

	memset(tmp_buf.data, 0, spc_data_tmp_size);
	memcpy(tmp_buf.data, spcdata->data, spc_data_tmp_size);

	dst_header = (struct spc_header *)spcdata->data;
	tmp_header = (struct spc_header *)tmp_buf.data;

	for (i = 0; i < dst_header->module_num; i ++) {

		tmp_module_header = (struct spc_module_header *)((int8_t *)tmp_buf.data +
								sizeof(struct spc_header) + i * sizeof(struct spc_module_header));
		dst_module_header = (struct spc_module_header *)((int8_t *)spcdata->data +
								sizeof(struct spc_header) + i * sizeof(struct spc_module_header));

		if (0 == i) {
			dst_module_header->offset = dst_header->fixed_size + sizeof(struct spc_header);
		} else {
			dst_module_header_last = (struct spc_module_header *)((uint8_t *)spcdata->data +
								sizeof(struct spc_header) + (i-1) * sizeof(struct spc_module_header));

			dst_module_header->offset = dst_module_header_last->offset + dst_module_header_last->size;
		}

		dst_addr = (uint8_t *)dst_header + dst_module_header->offset;
		tmp_addr = (uint8_t *)tmp_header + tmp_module_header->offset;

		if (i == relayout_module_index) {

			if (tmp_module_header->size < relayout_module_data->data_size) {
				spc_data_dst_size = spc_data_tmp_size + (relayout_module_data->data_size - tmp_module_header->size);
			} else {
				spc_data_dst_size = spc_data_tmp_size - (tmp_module_header->size - relayout_module_data->data_size);
			}

			if (spc_data_dst_size > spcdata->max_size) {
				rtn = SPC_MEM_NOT_ENOUGH;
				break;
			}

			memset(dst_addr,0,dst_module_header->size);
			dst_module_header->size = relayout_module_data->data_size;
			if (0 < dst_module_header->size)
				memcpy(dst_addr,(int8_t *)relayout_module_data->data,dst_module_header->size);

		} else
			memcpy(dst_addr,tmp_addr,tmp_module_header->size);

	}

	free(tmp_buf.data);
	dst_header->size = spc_data_dst_size;
	spcdata->data_size = spc_data_dst_size;

	return rtn;
}

/* _delete_by_id --read the module data from spc data by module id
*@
*@
*@ return:
*/
static int32_t _delete_module(struct spc_data *spcdata, uint32_t module_index,struct spc_module_data *delete_module_data)
{
	int32_t rtn = SPC_SUCCESS;
	uint32_t i,j;
	struct spc_header *spcheader = NULL;
	uint8_t *module_header_base = NULL;
	uint8_t *module_header_tmp_base = NULL;
	uint8_t *module_header_addr = NULL;
	uint8_t *module_header_tmp_addr = NULL;

	spcheader = (struct spc_header *)spcdata->data;
	module_header_base = (uint8_t *)((uint8_t *)spcdata->data + sizeof(struct spc_header));

	if (0 < spcheader->fixed_size)
		module_header_tmp_base = (uint8_t *)malloc(spcheader->fixed_size);
	else
		return SPC_ERROR;

	_relayout_spc_data(spcdata, module_index, delete_module_data);
	memcpy(module_header_tmp_base, module_header_base,spcheader->fixed_size);

	for (i = 0,j = 0; i < spcheader->module_num; i ++) {
		module_header_tmp_addr = module_header_tmp_base + i * (sizeof(struct spc_module_header));
		module_header_addr = module_header_base + j * (sizeof(struct spc_module_header));

		if (i != module_index) {
			memcpy(module_header_addr,module_header_tmp_addr,sizeof(struct spc_module_header));
			j ++;
		}
	}

	if (1 > spcheader->module_num)
		return SPC_ERROR;

	spcheader->module_num -= 1;

	if (NULL != module_header_tmp_base) {
		free(module_header_tmp_base);
		module_header_tmp_base = NULL;
	}

	return rtn;
}
/**---------------------------------------------------------------------------*
** 				Public Functions			     *
**---------------------------------------------------------------------------*/
/* spc_init --init the spc data area: write the haeder and init the fixed area
*@
*@
*@ return:
*/
static int32_t spc_init(struct spc_data *spcdata,struct spc_init_param *init_param)
{
	int32_t rtn = SPC_SUCCESS;
	struct spc_header *spcheader = NULL;
	uint32_t spc_size = 0;
	uint32_t spc_fixe_size = 0;

	if (NULL == init_param || NULL == spcdata)
		return SPC_ERROR;

	spc_fixe_size = init_param->fixed_size;
	spc_size = spc_fixe_size + sizeof(struct spc_header);
	if (NULL == spcdata->data|| spcdata->max_size < spc_size)
		return SPC_ERROR;

	memset(spcdata->data, 0, spcdata->max_size);

	spcheader = (struct spc_header *)spcdata->data;

	//init the spc data
	spcheader->verify = init_param->verify;
	spcheader->version = init_param->version;
	spcheader->fixed_size = spc_fixe_size;
	spcheader->module_num = 0;
	spcheader->size = spc_size;
	spcheader->user_data[0] = init_param->user_data[0];
	spcheader->user_data[1] = init_param->user_data[1];
	spcheader->user_data[2] = init_param->user_data[2];
	spcheader->user_data[3] = init_param->user_data[3];

	spcdata->data_size = spc_size;

	return rtn;
}


/* spc_write --write modules to spc data, must be called after spc_init
*@
*@
*@ return:
*/
static int32_t spc_write_module(struct spc_data *spcdata,struct spc_write_param *write_param)
{
	int32_t rtn = SPC_SUCCESS;
	uint32_t i,j;
	struct spc_header *spcheader = NULL;
	struct spc_module_header *spc_module_header_ptr = NULL;
	struct spc_module_data *w_module_data = NULL;

	uint32_t w_module_num = 0;//processing module number
	int8_t *spc_module_data_addr = NULL;
	int32_t spc_size = 0;
	int32_t w_size = 0;
	uint32_t spc_max_module_num = 0;// according to fixed_sixe in spc header

	if (NULL == spcdata || NULL == write_param)
		return SPC_ERROR;

	//check whether the spc data has been initialized
	spcheader = (struct spc_header *)spcdata->data;

	if (0 == spcheader->fixed_size)
		return SPC_HEADER_NOT_INIT;

	spc_max_module_num = spcheader->fixed_size/(sizeof(struct spc_module_header));
	w_module_num = write_param->module_num;

	for (i = 0; i < w_module_num; i ++) {//i : index of w_module

		w_module_data = &write_param->module_data[i];

		for(j = 0; j < spcheader->module_num; j ++) {// j : index of src spc module data
			 spc_module_header_ptr = (struct spc_module_header *)((int8_t *)spcheader +
						sizeof(struct spc_header) + j * sizeof(struct spc_module_header));

			if (w_module_data->id == spc_module_header_ptr->id)//update
				break;
		}

		if (j >= spcheader->module_num) {//not found same id
			if (spcheader->module_num >= spc_max_module_num) {
				rtn = SPC_MEM_NOT_ENOUGH;
				break;
			}

			spc_module_header_ptr = (struct spc_module_header *)((int8_t *)spcheader +
						sizeof(struct spc_header) + j * sizeof(struct spc_module_header));

			if ((0 == w_module_data->data_size) || (NULL == w_module_data->data))
				return SPC_WRITE_NULL_DATA;

			if (w_module_data->data_size + spcheader->size > spcdata->max_size) {
				rtn = SPC_MEM_NOT_ENOUGH;
				break;
			}

			spc_module_header_ptr->size = w_module_data->data_size;
			spc_module_header_ptr->id = w_module_data->id;
			spc_module_header_ptr->offset = spcheader->size;
			spc_module_data_addr = (int8_t *)(spcheader) + spc_module_header_ptr->offset;
			memcpy(spc_module_data_addr, (int8_t *)w_module_data->data, w_module_data->data_size);

			spcheader->size += w_module_data->data_size;//spc data size in spc header
			spcheader->module_num += 1;
			spcdata->data_size = spcheader->size;
		} else {//find same id

			spc_size = spc_module_header_ptr->size;
			w_size = w_module_data->data_size;

			if ((0 == w_size) || (w_module_data->data == NULL)) {
				rtn = _delete_module(spcdata, j, w_module_data);
				if (rtn != SPC_SUCCESS)
					return rtn;
			}
			else if (w_size == spc_size) {
				spc_module_data_addr = (int8_t *)(spcheader) + spc_module_header_ptr->offset;
				memcpy(spc_module_data_addr,(int8_t *)w_module_data->data,w_size);
			} else {
				rtn = _relayout_spc_data(spcdata, j, w_module_data);
				if (SPC_SUCCESS != rtn)
					break;
			}
		}

	}

	return rtn;
}

int32_t _packet_ctrl_param_v1(struct spc_data *spc, struct awb_param_ctrl *ctrl_param,void *pack_buf, uint32_t pack_buf_size)
{
	int32_t rtn = AWB_PACKET_SUCCESS;

	struct spc_write_param write_param = {0};
	struct spc_init_param init_param = {0};
	struct spc_module_data module_data = {0};

	init_param.verify = AWB_CTRL_PACKET_VERIFY;
	init_param.version = AWB_CTRL_PACKET_VERSION;
	init_param.fixed_size = AWB_CTRL_PACKET_FIX_SIZE;

	spc->data = (void *)pack_buf;
	spc->data_size = 0;
	spc->max_size = pack_buf_size;

	rtn = spc_init(spc,&init_param);
	if (0 != rtn) {
		rtn = AWB_PACKET_ERROR;
		return rtn;
	}

	write_param.module_num = 1;

	write_param.module_data = &module_data;
	write_param.module_data[0].id = AWB_MODULE_ID_CTRL_PARAM;
	write_param.module_data[0].data = ctrl_param;
	write_param.module_data[0].data_size = sizeof(struct awb_param_ctrl);

	rtn = spc_write_module(spc, &write_param);
	if (AWB_PACKET_SUCCESS != rtn	){
		rtn =  AWB_PACKET_ERROR;
		return rtn;
	}

	return rtn;
}

int32_t _packet_alg_param_v1(struct spc_data *spc, struct awb_alg_param_common *alg_param,
							struct awb_alg_map *alg_map, struct awb_alg_weight *alg_weight,
							void *pack_buf, uint32_t pack_buf_size)
{
	int32_t rtn = 0;

	struct spc_write_param write_param = {0};
	struct spc_init_param init_param = {0};
	struct spc_module_data module_data[3] = {0};

	init_param.verify = AWB_ALG_PACKET_VERIFY;
	init_param.version = AWB_ALG_PACKET_VERSION;
	init_param.fixed_size = AWB_ALG_PACKET_FIX_SIZE;

	spc->data = (void *)pack_buf;
	spc->data_size = 0;
	spc->max_size = pack_buf_size;

	rtn = spc_init(spc,&init_param);
	if (0 != rtn) {
		rtn = AWB_PACKET_ERROR;
		return rtn;
	}

	write_param.module_num = 3;

	write_param.module_data = module_data;
	write_param.module_data[0].id = AWB_ALG_MODULE_ID_COMMON;
	write_param.module_data[0].data = alg_param;
	write_param.module_data[0].data_size = sizeof(struct awb_alg_param_common);

	write_param.module_data[1].id = AWB_ALG_MODULE_ID_WIN_MAP;
	write_param.module_data[1].data = alg_map->data;
	write_param.module_data[1].data_size = alg_map->size;

	write_param.module_data[2].id = AWB_ALG_MODULE_ID_POS_WEIGHT;
	write_param.module_data[2].data = alg_weight->data;
	write_param.module_data[2].data_size = alg_weight->size;

	rtn = spc_write_module(spc, &write_param);
	if (0 != rtn){
		rtn = AWB_PACKET_ERROR;
		return rtn;
	}

	return rtn;
}

int32_t _ctrl_alg_param_pack_v1(struct spc_data *spc, void *ctrl_pack_buf,
								uint32_t ctrl_pack_buf_size, void * alg_pack_buf,
								uint32_t  alg_pack_buf_size, void *pack_buf,
								uint32_t pack_buf_size)
{
	int32_t rtn = AWB_PACKET_SUCCESS;

	struct spc_write_param write_param = {0};
	struct spc_init_param init_param = {0};
	struct spc_module_data module_data[2] = {0};
	struct spc_module_data *module_data_ptr = NULL;

	spc->data = (void *)pack_buf;
	spc->data_size = 0;
	spc->max_size = pack_buf_size;

	init_param.verify = AWB_PACKET_VERIFY;
	init_param.version = AWB_PACKET_VERSION;
	init_param.fixed_size = AWB_PACKET_FIX_SIZE;

	spc->data_size = 0;

	rtn = spc_init(spc,&init_param);
	if (0 != rtn) {
		rtn = AWB_PACKET_ERROR;
		return rtn;
	}

	write_param.module_num = 2;

	write_param.module_data = module_data;
	write_param.module_data[0].id = AWB_MODULE_ID_CTRL_PARAM;
	write_param.module_data[0].data = ctrl_pack_buf;
	write_param.module_data[0].data_size = ctrl_pack_buf_size;

	write_param.module_data[1].id = AWB_MODULE_ID_ALG_PARAM;
	write_param.module_data[1].data = alg_pack_buf;
	write_param.module_data[1].data_size = alg_pack_buf_size;

	rtn = spc_write_module(spc, &write_param);
	if (0 != rtn){
		rtn = AWB_PACKET_ERROR;
		return rtn;
	}

	return rtn;
}



