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

#include "sensor_isp_param_merge.h"

#define NEW_PARAM_RESERVE_LEN 256
#define OLD_PARAM_RESERVE_LEN 2314
#define PAC_AWB_MALLOC_MEM_LEN 1024
#define LNC_MAP_COUNT 9
#define ISP_PARAM_OLD_VERSION 0x00030002

#define ISP_SUCCESS 0x00
#define ISP_ERROR 0x01

static int sensor_isp_get_param_len(struct sensor_raw_info* sensor_info_ptr,uint32_t *all_param_len,
									uint32_t *ae_len,uint32_t *awb_len,uint32_t *lnc_len,int mode)
{
	int rtn = ISP_SUCCESS;

	struct sensor_raw_fix_info *sensor_fix_info = NULL;

	struct sensor_ae_tab* ae_table_ptr = NULL;
	struct sensor_awb_map_weight_param* awb_map_weight_ptr = NULL;
	struct sensor_lsc_map* lnc_map_ptr = NULL;

	struct ae_exp_gain_tab* ae_tab = NULL;
	struct ae_weight_tab* weight_tab = NULL;
	struct ae_scene_exp_gain_tab* scene_tab = NULL;
	struct ae_auto_iso_tab_v1*  auto_iso_tab = NULL;

	struct sensor_awb_weight* awb_weight = NULL;
	struct sensor_awb_map* awb_map = NULL;

	struct sensor_lens_map* lnc_map = NULL;

	struct isp_mode_param* src_mode_ptr = NULL;
	struct isp_block_header* src_header = NULL;

	uint32_t i = 0;
	uint32_t j = 0;
	int ae_flag = -1;
	int awb_flag = -1;
	int lnc_flag = -1;

	uint32_t num_gain1 = 0;
	uint32_t param_len = 0;
	uint32_t add_ae_len = 0;
	uint32_t add_awb_len = 0;
	uint32_t add_lnc_len = 0;

	uint32_t again_len = 0;
	uint32_t dgain_len = 0;
	uint32_t dummy_len = 0;
	uint32_t exposure_len = 0;
	uint32_t index_len = 0;
	uint32_t scene_info_len = 0;

	sensor_fix_info = (struct sensor_raw_fix_info *)sensor_info_ptr->fix_ptr[mode];

	src_mode_ptr = (struct isp_mode_param *)sensor_info_ptr->mode_ptr[mode].addr;
	src_header = src_mode_ptr->block_header;
	for (i = 0;i < src_mode_ptr->block_num; i++) {
		if (src_header[i].block_id == ISP_BLK_AE_V1)
			ae_flag = i;
		if (src_header[i].block_id == ISP_BLK_AWB_V1)
			awb_flag = i;
		if (src_header[i].block_id == ISP_BLK_2D_LSC)
			lnc_flag = i;
	}
	param_len = sensor_info_ptr->mode_ptr[mode].len;
	CMR_LOGI("sensor_info_ptr->mode_ptr[mode].len =%d",sensor_info_ptr->mode_ptr[mode].len);

	/*  calculate AE data length*/
	if (-1 != ae_flag) {
		ae_table_ptr = (struct sensor_ae_tab*)&sensor_fix_info->ae;
		ae_tab = (struct ae_exp_gain_tab*)ae_table_ptr->ae_tab;
		weight_tab = (struct ae_weight_tab*)ae_table_ptr->weight_tab;
		scene_tab = (struct ae_scene_exp_gain_tab*)&ae_table_ptr->scene_tab;
		auto_iso_tab = (struct ae_auto_iso_tab_v1*)ae_table_ptr->auto_iso_tab;
		/*ae_gain table*/
		for (i = 0 ;i < AE_FLICKER_NUM;i++) {
			for (j = 0;j < AE_ISO_NUM;j++) {
				again_len = ae_tab[i*AE_ISO_NUM_NEW+j].again_len;
				dgain_len = ae_tab[i*AE_ISO_NUM_NEW+j].dgain_len;
				dummy_len = ae_tab[i*AE_ISO_NUM_NEW+j].dummy_len;
				exposure_len = ae_tab[i*AE_ISO_NUM_NEW+j].exposure_len;
				index_len = ae_tab[i*AE_ISO_NUM_NEW+j].index_len;
				param_len = param_len + again_len + dgain_len + dummy_len+ exposure_len + index_len;
				add_ae_len = add_ae_len + again_len + dgain_len + dummy_len+ exposure_len + index_len;
			}
		}
		/*ae_flash table*/
		param_len = param_len + add_ae_len;
		add_ae_len = add_ae_len*2;
		/*ae_weight table*/
		for (i = 0; i < AE_WEIGHT_TABLE_NUM;i++) {
			param_len = param_len + weight_tab[i].len;
			add_ae_len = add_ae_len + weight_tab[i].len;
		}
		/*ae_scene table*/
		for (i = 0 ;i < AE_SCENE_NUM;i++) {
			for (j = 0;j < AE_FLICKER_NUM;j++) {
				again_len = scene_tab[i*AE_FLICKER_NUM+j].again_len;
				dgain_len = scene_tab[i*AE_FLICKER_NUM+j].dgain_len;
				dummy_len = scene_tab[i*AE_FLICKER_NUM+j].dummy_len;
				exposure_len = scene_tab[i*AE_FLICKER_NUM+j].exposure_len;
				index_len = scene_tab[i*AE_FLICKER_NUM+j].index_len;
				if(0 == j) {
					scene_info_len = scene_tab[i*AE_FLICKER_NUM+j].scene_info_len;
				}else {
					scene_info_len = 0;
				}
				param_len = param_len + again_len + dgain_len + dummy_len+ exposure_len + index_len + scene_info_len;
				add_ae_len = add_ae_len + again_len + dgain_len + dummy_len+ exposure_len + index_len + scene_info_len;
			}
		}
		/*ae_auto_iso table*/
		for (i = 0;i < AE_FLICKER_NUM;i++) {
			param_len = param_len+auto_iso_tab[i].len;
			add_ae_len = add_ae_len+auto_iso_tab[i].len;
		}
		CMR_LOGI("param_len = %d , add_ae_len = %d",param_len,add_ae_len);
	}

	/* calculate AWB data length*/
	if (-1 != awb_flag) {
		awb_map_weight_ptr = (struct sensor_awb_map_weight_param*)&sensor_fix_info->awb;
		awb_weight = (struct sensor_awb_weight*)&awb_map_weight_ptr->awb_weight;
		awb_map = (struct sensor_awb_map*)&awb_map_weight_ptr->awb_map;
		/*awb_map*/
		param_len = param_len + awb_map->len;
		add_awb_len = add_awb_len + awb_map->len;
		/*wab_weight*/
		param_len = param_len+awb_weight->weight_len + awb_weight->size_param_len;
		add_awb_len = add_awb_len+awb_weight->weight_len + awb_weight->size_param_len;
		CMR_LOGI("param_len = %d , add_awb_len = %d",param_len,add_awb_len);
	}

	/*  calculate lnc data length*/
	if (-1 != lnc_flag) {
		lnc_map_ptr = (struct sensor_lsc_map*)&sensor_fix_info->lnc;
		lnc_map= (struct sensor_lens_map*)lnc_map_ptr->map;
	/*lnc_map*/
		for (i = 0;i < LNC_MAP_COUNT;i++) {
			param_len = param_len + lnc_map[i].lnc_len + lnc_map[i].map_info_len;
			add_lnc_len = add_lnc_len + lnc_map[i].lnc_len + lnc_map[i].map_info_len;
			CMR_LOGI("lnc_map[i].lnc_len = %d, lnc_map[i].map_info_len = %d",lnc_map[i].lnc_len,lnc_map[i].map_info_len);
		}
	}
	CMR_LOGI("data_len = %d,AE_data_len = %d,AWB_data_len = %d,LNC_data_len = %d",
		param_len,add_ae_len,add_awb_len,add_lnc_len);

	*all_param_len = param_len;
	*ae_len = add_ae_len;
	*awb_len = add_awb_len;
	*lnc_len = add_lnc_len;
	return rtn;
}

static int sensor_isp_get_awb_packet( void* awb_ptr,struct sensor_awb_map* awb_map,struct sensor_awb_weight* awb_weight,
										void* pack_buf,uint32_t* pack_buf_size)
{
	int32_t rtn = AWB_PACKET_SUCCESS;

	struct spc_data spc_data_ctrl = {0};
	struct spc_data spc_data_alg = {0};
	struct spc_data spc_data_ctrl_alg = {0};
	struct awb_param_ctrl* ctrl_param = PNULL;
	struct awb_alg_param_common* alg_param_common = NULL;
	struct awb_alg_map  alg_param_map = {0};
	struct awb_alg_weight  alg_param_weight ={0};

	uint8_t* awb_ctrl_param = NULL;
	uint8_t* awb_alg_common_param = NULL;
	uint8_t* awb_alg_weight_param =NULL;
	uint8_t* awb_alg_weight_param_tmp = NULL;

	void* ctrl_pack_buf = NULL;
	void* alg_pack_buf = NULL;
	uint32_t ctrl_pack_buf_size =0;
	uint32_t alg_pack_buf_size = 0;

	/*get awb ctrl param from awb_ptr*/
	awb_ctrl_param = (uint8_t*)malloc(sizeof(struct awb_param_ctrl));
	if (PNULL == awb_ctrl_param) {
		CMR_LOGE("alloc awb_ctrl_param mem error");
		rtn = ISP_ERROR;
		goto EXIT;
	}
	memset(awb_ctrl_param,0x00,sizeof(struct awb_param_ctrl));

	memcpy(awb_ctrl_param,(uint8_t*)awb_ptr,sizeof(struct awb_param_ctrl));
	CMR_LOGI("awb_ctrl_param = %d",sizeof(struct awb_param_ctrl));
	ctrl_param = (struct awb_param_ctrl*)awb_ctrl_param;
	/*get awb alg common param from awb_ptr*/

	awb_alg_common_param = (void*)malloc(sizeof(struct awb_alg_param_common));
	if (PNULL == awb_alg_common_param) {
		CMR_LOGE("alloc awb_alg_common_param mem error");
		rtn = ISP_ERROR;
		goto EXIT;
	}
	memset(awb_alg_common_param,0x00,sizeof(struct awb_alg_param_common));
	CMR_LOGI("awb_alg_common_param = %d",sizeof(struct awb_alg_param_common));
	memcpy(awb_alg_common_param,(void*)((uint8_t*)awb_ptr+sizeof(struct awb_param_ctrl)),sizeof(struct awb_alg_param_common));
	alg_param_common = (struct awb_alg_param_common*)awb_alg_common_param;

	/*get awb alg map param from awb_map*/
	alg_param_map.data = (uint16_t*)awb_map->addr;
	alg_param_map.size = awb_map->len;
	/*get awb weight param*/
	awb_alg_weight_param = (uint8_t*)malloc(awb_weight->weight_len+awb_weight->size_param_len);
	if (PNULL == awb_alg_weight_param) {
		CMR_LOGE("alloc awb_alg_common_param mem error");
		free(awb_ctrl_param);
		awb_ctrl_param = PNULL;
		rtn = ISP_ERROR;
		goto EXIT;
	}
	memset(awb_alg_weight_param,0x00,awb_weight->weight_len+awb_weight->size_param_len);
	memcpy((void*)awb_alg_weight_param,(void*)awb_weight->size,awb_weight->size_param_len);
	awb_alg_weight_param_tmp = (uint8_t*)awb_alg_weight_param + awb_weight->size_param_len;
	memcpy(awb_alg_weight_param_tmp,(uint8_t*)awb_weight->addr,awb_weight->weight_len);
	alg_param_weight.data = (int16_t*)awb_alg_weight_param;
	alg_param_weight.size = awb_weight->weight_len+awb_weight->size_param_len;

	ctrl_pack_buf_size = 100 + sizeof(struct awb_param_ctrl);
	ctrl_pack_buf = (void *)malloc(ctrl_pack_buf_size * sizeof(uint8_t));
	if (PNULL == ctrl_pack_buf) {
		CMR_LOGE("alloc ctrl_pack_buf mem error");
		rtn = ISP_ERROR;
		goto EXIT;
	}
	memset(ctrl_pack_buf,0,ctrl_pack_buf_size);

	alg_pack_buf_size = PAC_AWB_MALLOC_MEM_LEN * 50;
	alg_pack_buf = (void *)malloc(alg_pack_buf_size * sizeof(uint8_t));
	if (PNULL == alg_pack_buf) {
		CMR_LOGE("alloc alg_pack_buf mem error");
		rtn = ISP_ERROR;
		goto EXIT;
	}
	memset(alg_pack_buf,0,alg_pack_buf_size);

	rtn = _packet_ctrl_param_v1(&spc_data_ctrl, ctrl_param, ctrl_pack_buf, ctrl_pack_buf_size);
	if (0!= rtn) {
		rtn = AWB_PACKET_ERROR;
		goto EXIT;
	}
	rtn = _packet_alg_param_v1(&spc_data_alg, alg_param_common, &alg_param_map, &alg_param_weight,
							alg_pack_buf, alg_pack_buf_size);
	if (0 != rtn) {
		rtn = AWB_PACKET_ERROR;
		goto EXIT;
	}
	rtn = _ctrl_alg_param_pack_v1(&spc_data_ctrl_alg, ctrl_pack_buf, ctrl_pack_buf_size,
							alg_pack_buf, alg_pack_buf_size, pack_buf, *pack_buf_size);
	if (0!= rtn) {
		rtn = AWB_PACKET_ERROR;
		goto EXIT;
	}
	*pack_buf_size = spc_data_ctrl_alg.data_size;


EXIT:
	if (PNULL != awb_ctrl_param){
		free(awb_ctrl_param);
		awb_ctrl_param = NULL;
	}
	if (PNULL !=awb_alg_common_param) {
		free(awb_alg_common_param);
		awb_alg_common_param = NULL;
	}
	if (PNULL != awb_alg_weight_param) {
		free(awb_alg_weight_param);
		awb_alg_weight_param = NULL;
	}
	if (NULL != ctrl_pack_buf) {
		free(ctrl_pack_buf);
		ctrl_pack_buf = NULL;
	}
	if (NULL != alg_pack_buf) {
		free(alg_pack_buf);
		alg_pack_buf = NULL;
	}

	return rtn;
}

int  sensor_isp_param_merge(struct sensor_raw_info* sensor_info_ptr,struct isp_data_info* out_param ,int mode)
{
	int32_t rtn = ISP_SUCCESS;

	uint32_t i = 0 ;
	uint32_t j = 0;

	/*the param length and fix param length */
	uint32_t param_len = 0;
	uint32_t add_ae_len= 0;
	uint32_t add_awb_len = 0;
	uint32_t add_lnc_len = 0;

	uint8_t* param_ptr = NULL;
	uint8_t* tmp_ptr = NULL;
	uint8_t* param_ptr_awb_pac = NULL;

	uint8_t* ae_ptr = NULL;
	uint8_t* awb_ptr = NULL;
	uint8_t* lnc_ptr = NULL;

	int ae_index = -1;
	int awb_index = -1;
	int lnc_index = -1;
	uint8_t* ae_tmp_ptr = NULL;

	void* pack_buf = NULL;
	uint32_t pack_buf_size = 0;

	struct sensor_raw_fix_info *sensor_fix_info = NULL;
	struct isp_mode_param *src_mode_ptr = NULL;
	struct isp_mode_param *src_mode_ptr_new = NULL;
	struct isp_mode_param *src_mode_ptr_awb_pac = NULL;
	struct isp_block_header *src_header = NULL;
	struct isp_block_header *src_header_new = NULL;
	struct isp_block_header *src_header_awb_pac = NULL;

	struct ae_scene_exp_gain_tab* scene_tab = NULL;
	struct ae_exp_gain_tab* ae_tab = NULL;
	struct ae_weight_tab* weight_tab = NULL;
	struct ae_auto_iso_tab_v1* auto_iso_tab = NULL;
	struct sensor_awb_map* awb_map = NULL;
	struct sensor_awb_weight* awb_weight = NULL;
	struct sensor_lens_map* map = NULL;

	sensor_fix_info = (struct sensor_raw_fix_info *)sensor_info_ptr->fix_ptr[mode];
	src_mode_ptr = (struct isp_mode_param *)sensor_info_ptr->mode_ptr[mode].addr;
	if (NULL == src_mode_ptr) {
		out_param->size = 0;
		out_param->data_ptr = NULL;
		return ISP_SUCCESS;
	}

	rtn = sensor_isp_get_param_len(sensor_info_ptr,&param_len,&add_ae_len,&add_awb_len,&add_lnc_len,mode);
	if (ISP_SUCCESS != rtn) {
		CMR_LOGE("get param error");
		return ISP_ERROR;
	}

	if ((0 == add_ae_len) && (0 == add_awb_len) && (0 == add_lnc_len)) {
		out_param->size = sensor_info_ptr->mode_ptr[mode].len;
		out_param->data_ptr = (void *)sensor_info_ptr->mode_ptr[mode].addr;
		return ISP_SUCCESS;
	}

	/*get the total mem of new param that match with the struct of ae_tuning_param */
	param_len = param_len + (OLD_PARAM_RESERVE_LEN - NEW_PARAM_RESERVE_LEN)*sizeof(uint32_t) + PAC_AWB_MALLOC_MEM_LEN;
	param_ptr = (uint8_t*)malloc(param_len);
	if (PNULL == param_ptr) {
		CMR_LOGE("ISP alloc mem error ");
		return ISP_ERROR;
	}
	memset(param_ptr,0x00,param_len);
	CMR_LOGI("param_ptr = %p ,param_len = %d",param_ptr,param_len);

	/*copy header_data from common to new mem*/
	memcpy((void*)param_ptr,(void*)src_mode_ptr,sizeof(struct isp_mode_param));
	tmp_ptr = param_ptr + sizeof(struct isp_mode_param);

	/*copy the data except AE/AWB/LNC .Meanwhile ,record the info of AE/AWB/LNC*/
	src_header = (struct isp_block_header*)src_mode_ptr->block_header;
	src_mode_ptr_new = (struct isp_mode_param*)param_ptr;
	src_header_new = (struct isp_block_header *)src_mode_ptr_new->block_header;
	for (i = 0;i < src_mode_ptr->block_num;i++) {
		uint32_t offset = 0;
		if (src_header[i].block_id == ISP_BLK_AE_V1) {
			ae_index = i;
			ae_ptr = (uint8_t*)malloc(src_header[i].size);
			if (PNULL == ae_ptr){
				CMR_LOGE("ISP alloc mem error ");
				free(param_ptr);
				param_ptr = PNULL;
				goto EXIT;
			}
			offset = src_header[i].offset;
			memcpy((void*)ae_ptr,(void*)((uint8_t*)src_mode_ptr+offset),src_header[i].size);
		} else if (src_header[i].block_id == ISP_BLK_AWB_V1) {
			awb_index = i;
			awb_ptr = (uint8_t*)malloc(src_header[i].size);
			if (PNULL == awb_ptr){
				CMR_LOGE("ISP alloc mem error ");
				free(param_ptr);
				param_ptr = NULL;
				goto EXIT;
			}
			offset = src_header[i].offset;
			memcpy((void*)awb_ptr,(void*)((uint8_t*)src_mode_ptr+offset),src_header[i].size);
		} else if (src_header[i].block_id == ISP_BLK_2D_LSC) {
			lnc_index = i;
			lnc_ptr = (uint8_t*)malloc(src_header[i].size);
			if (NULL == lnc_ptr) {
				CMR_LOGE("ISP alloc mem error ");
				free(param_ptr);
				param_ptr = NULL;
				goto EXIT;
			}
			offset = src_mode_ptr->block_header[i].offset;
			memcpy((void*)lnc_ptr,(void*)((int8_t*)src_mode_ptr+offset),src_header[i].size);
		} else {
			offset = src_header[i].offset;
			src_header_new[i].offset = (uint32_t)(tmp_ptr -param_ptr);
			memcpy((void*)tmp_ptr,(void*)((uint8_t*)src_mode_ptr+offset),src_header[i].size);
			tmp_ptr = tmp_ptr+src_header[i].size;
		}
	}

	/*copy ae_data to new mem*/
	if (ae_index >= 0 ) {
		src_header_new[ae_index].size = add_ae_len + src_header[ae_index].size + (2314 -256)*sizeof(uint32_t);
		src_header_new[ae_index].offset = (uint32_t)(tmp_ptr - param_ptr);
		memcpy((void*)tmp_ptr,(void*)ae_ptr,sizeof(struct ae_param_tmp_001));
		tmp_ptr = tmp_ptr+sizeof(struct ae_param_tmp_001);
		ae_tmp_ptr = ae_ptr+sizeof(struct ae_param_tmp_001);

		/*get param from fix struct*/
		scene_tab = (struct ae_scene_exp_gain_tab*)sensor_fix_info->ae.scene_tab;
		ae_tab = (struct ae_exp_gain_tab*)sensor_fix_info->ae.ae_tab;
		weight_tab = (struct ae_weight_tab*)sensor_fix_info->ae.weight_tab;
		auto_iso_tab = (struct ae_auto_iso_tab_v1*)sensor_fix_info->ae.auto_iso_tab;

		/*copy ae gain table data*/
		for (i = 0;i < AE_FLICKER_NUM; i++) {
			for (j = 0;j < AE_ISO_NUM;j++) {
				memcpy((void*)tmp_ptr,(void*)ae_tab[i*AE_ISO_NUM_NEW+j].index,ae_tab[i*AE_ISO_NUM_NEW+j].index_len);
				tmp_ptr = tmp_ptr + ae_tab[i*AE_ISO_NUM_NEW+j].index_len;
				memcpy((void*)tmp_ptr,(void*)ae_tab[i*AE_ISO_NUM_NEW+j].exposure,ae_tab[i*AE_ISO_NUM_NEW+j].exposure_len);
				tmp_ptr = tmp_ptr + ae_tab[i*AE_ISO_NUM_NEW+j].exposure_len;
				memcpy((void*)tmp_ptr,(void*)ae_tab[i*AE_ISO_NUM_NEW+j].dummy,ae_tab[i*AE_ISO_NUM_NEW+j].dummy_len);
				tmp_ptr = tmp_ptr + ae_tab[i*AE_ISO_NUM_NEW+j].dummy_len;
				memcpy((void*)tmp_ptr,(void*)ae_tab[i*AE_ISO_NUM_NEW+j].again,ae_tab[i*AE_ISO_NUM_NEW+j].again_len);
				tmp_ptr = tmp_ptr + ae_tab[i*AE_ISO_NUM_NEW+j].again_len;
				memcpy((void*)tmp_ptr,(void*)ae_tab[i*AE_ISO_NUM_NEW+j].dgain,ae_tab[i*AE_ISO_NUM_NEW+j].dgain_len);
				tmp_ptr = tmp_ptr + ae_tab[i*AE_ISO_NUM_NEW+j].dgain_len;
			}
		}

		/*for flash_table*/
		for (i = 0;i < AE_FLICKER_NUM; i++) {
			for (j = 0;j < AE_ISO_NUM;j++) {
				tmp_ptr = tmp_ptr + ae_tab[i*AE_ISO_NUM_NEW+j].index_len;
				tmp_ptr = tmp_ptr + ae_tab[i*AE_ISO_NUM_NEW+j].exposure_len;
				tmp_ptr = tmp_ptr + ae_tab[i*AE_ISO_NUM_NEW+j].dummy_len;
				tmp_ptr = tmp_ptr + ae_tab[i*AE_ISO_NUM_NEW+j].again_len;
				tmp_ptr = tmp_ptr + ae_tab[i*AE_ISO_NUM_NEW+j].dgain_len;
			}
		}

		/*copy ae weight table data*/
		for ( i = 0;i < AE_WEIGHT_TABLE_NUM;i++) {
			memcpy((void*)tmp_ptr,(void*)weight_tab[i].weight_table,weight_tab[i].len);
			tmp_ptr = tmp_ptr + weight_tab[i].len;
		}

		/*copy scene_info data*/
		for (i = 0;i<AE_SCENE_NUM;i++) {
			memcpy((void*)tmp_ptr,(void*)scene_tab[i*AE_FLICKER_NUM].scene_info,scene_tab[i*AE_FLICKER_NUM].scene_info_len);
			tmp_ptr = tmp_ptr + scene_tab[i*AE_FLICKER_NUM].scene_info_len;
			for (j = 0;j<AE_FLICKER_NUM;j++) {
				memcpy((void*)tmp_ptr,(void*)scene_tab[i*AE_FLICKER_NUM+j].index,scene_tab[i*AE_FLICKER_NUM+j].index_len);
				tmp_ptr = tmp_ptr + scene_tab[i*AE_FLICKER_NUM+j].index_len;
				memcpy((void*)tmp_ptr,(void*)scene_tab[i*AE_FLICKER_NUM+j].exposure,scene_tab[i*AE_FLICKER_NUM+j].exposure_len);
				tmp_ptr = tmp_ptr + scene_tab[i*AE_FLICKER_NUM+j].exposure_len;
				memcpy((void*)tmp_ptr,(void*)scene_tab[i*AE_FLICKER_NUM+j].dummy,scene_tab[i*AE_FLICKER_NUM+j].dummy_len);
				tmp_ptr = tmp_ptr + scene_tab[i*AE_FLICKER_NUM+j].dummy_len;
				memcpy((void*)tmp_ptr,(void*)scene_tab[i*AE_FLICKER_NUM+j].again,scene_tab[i*AE_FLICKER_NUM+j].again_len);
				tmp_ptr = tmp_ptr + scene_tab[i*AE_FLICKER_NUM+j].again_len;
				memcpy((void*)tmp_ptr,(void*)scene_tab[i*AE_FLICKER_NUM+j].dgain,scene_tab[i*AE_FLICKER_NUM+j].dgain_len);
				tmp_ptr = tmp_ptr + scene_tab[i*AE_FLICKER_NUM+j].dgain_len;
			}
		}

		/*copy auto iso table data*/
		for ( i = 0;i < AE_FLICKER_NUM;i++) {
			memcpy((void*)tmp_ptr,(void*)auto_iso_tab[i].auto_iso_tab,auto_iso_tab[i].len);
			tmp_ptr = tmp_ptr + auto_iso_tab[i].len;
		}

		memcpy((void*)tmp_ptr,(void*)ae_tmp_ptr,sizeof(struct ae_param_tmp_002));
		tmp_ptr = tmp_ptr + sizeof(struct ae_param_tmp_002);
		ae_tmp_ptr = ae_tmp_ptr + sizeof(struct ae_param_tmp_002);

		/*for reserved*/
		memcpy((void*)tmp_ptr,(void*)ae_tmp_ptr,NEW_PARAM_RESERVE_LEN*sizeof(uint32_t));
		tmp_ptr = tmp_ptr + OLD_PARAM_RESERVE_LEN*sizeof(uint32_t);
		src_mode_ptr_new->size = tmp_ptr -param_ptr;
		param_len = src_mode_ptr_new->size;
	}

	/*copy lnc_data to new mem*/
	if (lnc_index >= 0){
		/* 2*9*sizeof(uint32_t) is calc offset_len*/
		src_header_new[lnc_index].size = add_lnc_len + src_header[lnc_index].size + 2*LNC_MAP_COUNT*sizeof(uint32_t);
		CMR_LOGI("src_header_new[lnc_index].size = %d",src_header_new[lnc_index].size);
		src_header_new[lnc_index].offset = (uint32_t)(tmp_ptr - param_ptr);
		memcpy((void*)tmp_ptr,(void*)lnc_ptr,sizeof(struct sensor_2d_lsc_param_v1));
		tmp_ptr = tmp_ptr + sizeof(struct sensor_2d_lsc_param_v1);
		map = sensor_fix_info->lnc.map;
		for (i = 0;i < LNC_MAP_COUNT;i++) {
			uint32_t lnc_offset = 0;
			memcpy((void*)tmp_ptr,(void*)map[i].map_info,map[i].map_info_len);
			tmp_ptr = tmp_ptr + map[i].map_info_len;

			*(uint32_t *)tmp_ptr = map[i].lnc_len;
			tmp_ptr = tmp_ptr + sizeof(uint32_t);
			for (j = 0;j < i;j++)
				lnc_offset = lnc_offset + map[j].lnc_len;
			*(uint32_t *)tmp_ptr = lnc_offset;
			tmp_ptr = tmp_ptr + sizeof(uint32_t);
		}
		for (i = 0;i < LNC_MAP_COUNT;i++) {
			memcpy((void*)tmp_ptr,(void*)map[i].lnc_addr,map[i].lnc_len);
			tmp_ptr = tmp_ptr + map[i].lnc_len;
		}
		src_mode_ptr_new->size = tmp_ptr -param_ptr;
		param_len = src_mode_ptr_new->size;
	}

	/*copy awb_data to new mem*/
	if (awb_index >= 0) {
		int num = 0;
		src_header_new[awb_index].offset = (uint32_t)(tmp_ptr - param_ptr);
		pack_buf_size = PAC_AWB_MALLOC_MEM_LEN * PAC_AWB_MALLOC_MEM_LEN * sizeof(uint32_t);
		pack_buf = (void*)malloc(pack_buf_size);
		if (NULL == pack_buf) {
			CMR_LOGE("ISP alloc mem error ");
			free(param_ptr);
			param_ptr = NULL;
			goto EXIT;
		}
		memset(pack_buf,0x00,pack_buf_size);

		awb_map = (struct sensor_awb_map*)&sensor_fix_info->awb.awb_map;
		awb_weight = (struct sensor_awb_weight*)&sensor_fix_info->awb.awb_weight;

		/*pack the awb param*/
		rtn = sensor_isp_get_awb_packet((void*)awb_ptr,awb_map,awb_weight,pack_buf,&pack_buf_size);

		param_ptr_awb_pac  = (uint8_t *)malloc(src_header_new[awb_index].offset + pack_buf_size);
		if (NULL == param_ptr_awb_pac) {
			CMR_LOGE("ISP alloc mem error ");
			free(param_ptr);
			param_ptr = NULL;
			goto EXIT;
		}
		memset(param_ptr_awb_pac,0x00,src_header_new[awb_index].offset + pack_buf_size);

		memcpy(param_ptr_awb_pac,param_ptr,src_header_new[awb_index].offset);
		tmp_ptr = param_ptr_awb_pac + src_header_new[awb_index].offset;
		src_mode_ptr_awb_pac = (struct isp_mode_param *)param_ptr_awb_pac;
		src_header_awb_pac = (struct isp_block_header *)src_mode_ptr_awb_pac->block_header;

		src_header_awb_pac[awb_index].size = pack_buf_size;
		src_header_awb_pac[awb_index].offset = (uint32_t)(tmp_ptr - param_ptr_awb_pac);
		memcpy((void*)tmp_ptr,(void*)pack_buf,pack_buf_size);
		tmp_ptr = tmp_ptr + pack_buf_size;
		if (NULL != param_ptr) {
			free(param_ptr);
			param_ptr = NULL;
		}
		src_mode_ptr_awb_pac->size = tmp_ptr - param_ptr_awb_pac;
		param_ptr = param_ptr_awb_pac;
		param_len = src_mode_ptr_awb_pac->size;
	}

	out_param->size = param_len;
	out_param->data_ptr = (void *)param_ptr;
	CMR_LOGI("param_ptr = %p,param_len = %d",param_ptr,param_len);

EXIT:
	if (NULL != pack_buf) {
		free(pack_buf);
		pack_buf = NULL;
	}
	if (NULL != ae_ptr) {
		free(ae_ptr);
		ae_ptr = NULL;
	}
	if (NULL != awb_ptr) {
		free(awb_ptr);
		awb_ptr =NULL;
	}
	if (NULL != lnc_ptr) {
		free(lnc_ptr);
		lnc_ptr = NULL;
	}
	return rtn;
}

int sensor_merge_isp_param(struct isp_init_param *init_param_ptr,struct sensor_raw_info *sensor_info_ptr)
{
	int rtn  = ISP_SUCCESS;
	struct sensor_raw_info *sensor_raw_info_ptr = sensor_info_ptr;
	struct sensor_version_info *version_info = NULL;
	//struct isp_data_info *data_info = PNULL;
	int mode;

	if (NULL == sensor_raw_info_ptr || NULL == sensor_raw_info_ptr->version_info) {
		CMR_LOGE("X. sensor_info_ptr is null.");
		rtn = ISP_ERROR;
		return rtn;
	}

	version_info =  (struct sensor_version_info *)sensor_raw_info_ptr->version_info;
	if (ISP_PARAM_OLD_VERSION < version_info->version_id) {
		for (mode = 0; mode < MAX_MODE_NUM; mode++) {
			rtn = sensor_isp_param_merge(sensor_raw_info_ptr,&init_param_ptr->mode_ptr[mode],mode);
			if (0 != rtn ) {
				CMR_LOGE("isp get param error");
				return rtn;
			}
			CMR_LOGI("data_info.data_ptr = %p,data_info.size = %d",init_param_ptr->mode_ptr[mode].data_ptr,init_param_ptr->mode_ptr[mode].size);
		}
	}
	return rtn;
}

int sensor_merge_isp_param_free(isp_ctrl_context *isp_cxt )
{

	int rtn = ISP_SUCCESS;

	int i;
	struct sensor_raw_info *sensor_raw_info_ptr = NULL;
	struct sensor_version_info *sensor_version = NULL;
	struct isp_data_info *init_data_info = NULL;
	struct isp_data_info *update_data_info = NULL;

	struct sensor_ae_tab *ae = NULL;
	struct sensor_lsc_map *lnc = NULL;
	struct sensor_awb_map_weight_param *awb = NULL;

	if ((NULL == isp_cxt) || (NULL == isp_cxt->sn_raw_info)) {
		CMR_LOGE("get isp param error");
		rtn = ISP_ERROR;
		return rtn;
	}
	sensor_raw_info_ptr = (struct sensor_raw_info *)isp_cxt->sn_raw_info;

	if ((NULL == sensor_raw_info_ptr) || (NULL == sensor_raw_info_ptr->version_info)) {
		CMR_LOGE("get isp param error");
		rtn = ISP_ERROR;
		return rtn;
	}
	sensor_version = (struct sensor_version_info *)sensor_raw_info_ptr->version_info;
	init_data_info = (struct isp_data_info *)isp_cxt->isp_init_data;
	update_data_info = (struct isp_data_info *)isp_cxt->isp_update_data;

	if (ISP_PARAM_OLD_VERSION < sensor_version->version_id) {
		for (i = 0; i < MAX_MODE_NUM; i++) {
			if (NULL != sensor_raw_info_ptr->mode_ptr[i].addr) {
				ae = &sensor_raw_info_ptr->fix_ptr[i]->ae;
				lnc = &sensor_raw_info_ptr->fix_ptr[i]->lnc;
				awb = &sensor_raw_info_ptr->fix_ptr[i]->awb;
				if (NULL != ae->block.block_info || NULL != awb->block.block_info
							|| NULL != lnc->block.block_info ) {
					CMR_LOGV("mode = %d,init_addr = %p,len =%d",i,init_data_info[i].data_ptr,init_data_info[i].size);
					if (NULL != init_data_info[i].data_ptr) {
						free(init_data_info[i].data_ptr);
						init_data_info[i].data_ptr = NULL;
						init_data_info[i].size = 0;
					} else {
						CMR_LOGE("free isp param mem error.");
						rtn = ISP_ERROR;
						return rtn;
					}
					/*for isp tool*/
					CMR_LOGV("mode = %d,update_addr = %p,len =%d",i,update_data_info[i].data_ptr,update_data_info[i].size);
					if (NULL != update_data_info[i].data_ptr) {
						free(update_data_info[i].data_ptr);
						update_data_info[i].data_ptr = NULL;
						update_data_info[i].size = 0;
					}
				}
			} else {
				continue;
			}
		}
	}

	return rtn;
}


