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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "isp_param_file_update.h"
/**---------------------------------------------------------------------------*
 **				Compiler Flag					*
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif

/**---------------------------------------------------------------------------*
**				Micro Define					*
**----------------------------------------------------------------------------*/
#define LNC_LOOKUP_STR "s_%s_lnc_"
#define AES_LOOKUP_STR "s_%s_aes_"
#define AEG_LOOKUP_STR "s_%s_aeg_"
#define AE_WEIGHT_LOOKUP_STR "s_%s_ae_weight_customer"
#define TUNE_INFO_LOOKUP_STR "s_%s_tune_info"
#define SENSOR_RAW_FIX_INFO_LOOKUP_STR "s_%s_fix_info"
#define AWB_MAP_LOOKUP_STR "s_%s_awb_map"

struct isp_raw_info_update_status
{
	unsigned long updata_file_time;
	struct sensor_raw_fix_info *fix_info_table_addr;
	uint8_t *tune_info_table_addr;
	uint8_t *ae_weight_table_addr;
	uint16_t *lnc_data_table_addr[SENSOR_MAP_NUM][SENSOR_AWB_CALI_NUM];
	uint32_t *aes_data_table_addr[SENSOR_AE_TAB_NUM];
	uint16_t *aeg_data_table_addr[SENSOR_AE_TAB_NUM];
	uint8_t *tune_info_table_org_addr;
	uint16_t *awb_map_table_addr;
	struct sensor_raw_fix_info *fix_info_table_org_addr;
};

static struct isp_raw_info_update_status raw_info_update_status[2] = {
	{
		0,
		PNULL,
		PNULL,
		PNULL,
		{{PNULL}},
		{PNULL},
		{PNULL},
		PNULL,
		PNULL,
		PNULL,
	},
	{
		0,
		PNULL,
		PNULL,
		PNULL,
		{{PNULL}},
		{PNULL},
		{PNULL},
		PNULL,
		PNULL,
		PNULL,
	}

};


/**---------------------------------------------------------------------------*
*				Data Prototype					*
**----------------------------------------------------------------------------*/

void _isp_raw_para_recover (SENSOR_INFO_T *sensor_info_ptr,SENSOR_ID_E sensor_id)
{
	uint32_t i,j;
	CMR_LOGE("Sensor id:%d",sensor_id);

	for(i=0;i<SENSOR_MAP_NUM;i++){
		for(j=0;j<SENSOR_AWB_CALI_NUM;j++){
			if(raw_info_update_status[sensor_id].lnc_data_table_addr[i][j]){
				free(raw_info_update_status[sensor_id].lnc_data_table_addr[i][j]);
			}
			raw_info_update_status[sensor_id].lnc_data_table_addr[i][j] = 0;
		}
	}

	for(i=0;i<SENSOR_AE_TAB_NUM;i++){
		if(raw_info_update_status[sensor_id].aes_data_table_addr[i]){
			free(raw_info_update_status[sensor_id].aes_data_table_addr[i]);
		}
		raw_info_update_status[sensor_id].aes_data_table_addr[i] = 0;
		if(raw_info_update_status[sensor_id].aeg_data_table_addr[i]){
			free(raw_info_update_status[sensor_id].aeg_data_table_addr[i]);
		}
		raw_info_update_status[sensor_id].aeg_data_table_addr[i] = 0;
	}

	if(raw_info_update_status[sensor_id].ae_weight_table_addr){
		free(raw_info_update_status[sensor_id].ae_weight_table_addr);
	}
	raw_info_update_status[sensor_id].ae_weight_table_addr = 0;

	if(raw_info_update_status[sensor_id].tune_info_table_addr){
		free(raw_info_update_status[sensor_id].tune_info_table_addr);
	}
	raw_info_update_status[sensor_id].tune_info_table_addr = 0;

	if(raw_info_update_status[sensor_id].fix_info_table_addr){
		free(raw_info_update_status[sensor_id].tune_info_table_addr);
	}
	raw_info_update_status[sensor_id].tune_info_table_addr = 0;

	if(raw_info_update_status[sensor_id].awb_map_table_addr){
		free(raw_info_update_status[sensor_id].awb_map_table_addr);
	}
	raw_info_update_status[sensor_id].awb_map_table_addr = 0;

	if(raw_info_update_status[sensor_id].tune_info_table_org_addr){
		(*sensor_info_ptr->raw_info_ptr)->tune_ptr = (struct sensor_raw_tune_info* )raw_info_update_status[sensor_id].tune_info_table_org_addr;
	}
	raw_info_update_status[sensor_id].tune_info_table_org_addr = 0;

	if(raw_info_update_status[sensor_id].fix_info_table_org_addr){
		(*sensor_info_ptr->raw_info_ptr)->fix_ptr = raw_info_update_status[sensor_id].fix_info_table_org_addr;
	}
	raw_info_update_status[sensor_id].fix_info_table_org_addr = 0;

	raw_info_update_status[sensor_id].updata_file_time = 0;
}


uint32_t isp_raw_para_update_from_file(SENSOR_INFO_T *sensor_info_ptr,SENSOR_ID_E sensor_id)
{
	const char *sensor_name = sensor_info_ptr->name;
	FILE *fp = PNULL;
	char file_name[80] = {0};
	int file_size = 0;
	int file_read_pos = 0;
	char *file_tmp_buf = PNULL;
	char lookup_str[60] = {0};
	char *search_ptr;
	struct sensor_raw_fix_info *raw_fix_info_ptr = PNULL;
	struct sensor_raw_tune_info *raw_tune_info_ptr = PNULL;
	uint16_t *awb_map_ptr = PNULL;
	int i,j, ret_val = 0;
	uint8_t *temp_buf_8 = PNULL;
	uint16_t *temp_buf_16 = PNULL;
	uint32_t *temp_buf_32 = PNULL;
	int tune_info_need_param_num = 0;
	struct stat file_status;

	if(SENSOR_IMAGE_FORMAT_RAW != sensor_info_ptr->image_format){
		CMR_LOGE("sensor is not raw farmat!!");
		return SENSOR_FAIL;
	}

	if(PNULL == (*sensor_info_ptr->raw_info_ptr)){
		CMR_LOGE("raw info is PNULL!!");
		return SENSOR_FAIL;
	}

	strcpy(file_name, "/data/misc/media/");
	strcat(file_name, "sensor_");
	strcat(file_name, sensor_name);
	strcat(file_name, "_raw_param.c");
	fp = (FILE *)fopen(file_name,"rb");

	if(!fp){
		CMR_LOGE("file:%s not exist",file_name);
		_isp_raw_para_recover(sensor_info_ptr,sensor_id);
		return SENSOR_FAIL;
	}

	ret_val = stat(file_name,&file_status);
	if (ret_val < 0) {
		CMR_LOGE("file: %s get stat failed\n", file_name);
		if (fp) {
			fclose(fp);
		}
		return SENSOR_FAIL;
	}

	if(0 == raw_info_update_status[sensor_id].updata_file_time){
		CMR_LOGE("file first load");
		raw_info_update_status[sensor_id].tune_info_table_org_addr = (uint8_t*)(*sensor_info_ptr->raw_info_ptr)->tune_ptr;
		raw_info_update_status[sensor_id].fix_info_table_org_addr = (*sensor_info_ptr->raw_info_ptr)->fix_ptr;
	}else if(raw_info_update_status[sensor_id].updata_file_time == file_status.st_mtime){
		CMR_LOGE("file has been loaded");
		if(fp){
			fclose(fp);
		}
		return SENSOR_SUCCESS;
	}else{
		CMR_LOGE("new file load");
		_isp_raw_para_recover(sensor_info_ptr,sensor_id);
		raw_info_update_status[sensor_id].tune_info_table_org_addr = (uint8_t*)(*sensor_info_ptr->raw_info_ptr)->tune_ptr;
		raw_info_update_status[sensor_id].fix_info_table_org_addr = (*sensor_info_ptr->raw_info_ptr)->fix_ptr;
	}

	fseek(fp, 0L, SEEK_END);
	file_size = ftell(fp);
	if (file_size < 0) {
		CMR_LOGE("file: %s ftell faild\n", file_name);
		if (fp) {
			fclose(fp);
		}
		return SENSOR_FAIL;
	}
	fseek(fp, 0L, SEEK_SET);
	file_tmp_buf = (char*)malloc(file_size + 32);
	if(!file_tmp_buf){
		if(fp){
			fclose(fp);
		}
		return SENSOR_FAIL;
	}
	ret_val = fread(file_tmp_buf,1,file_size,fp);
	if (ferror(fp)) {
		CMR_LOGE("file: %s read faild\n", file_name);
		if (fp) {
			fclose(fp);
		}
		goto update_error;
	}
	file_tmp_buf[file_size] = 0;
	fclose(fp);

	raw_info_update_status[sensor_id].updata_file_time = file_status.st_mtime;

	raw_fix_info_ptr = (struct sensor_raw_fix_info *)malloc(sizeof(struct sensor_raw_fix_info));
	if(!raw_fix_info_ptr){
		goto update_error;
	}
	raw_info_update_status[sensor_id].fix_info_table_addr = raw_fix_info_ptr;
	raw_fix_info_ptr = (struct sensor_raw_fix_info *)((unsigned long)raw_fix_info_ptr>>2<<2);

	memcpy(raw_fix_info_ptr,(*sensor_info_ptr->raw_info_ptr)->fix_ptr,sizeof(struct sensor_raw_fix_info));


	search_ptr = file_tmp_buf;
	while(*search_ptr){
		if((*search_ptr) == '/' ){
			if((*(search_ptr+1)) == '*'){
				*search_ptr++ = 0x20;
				*search_ptr++ = 0x20;
				while(((*search_ptr) != '*') || (*(search_ptr+1) != '/')){
					*search_ptr++ = 0x20;
				}
				*search_ptr++ = 0x20;
				*search_ptr++ = 0x20;

			}else if((*(search_ptr+1)) == '/'){
				*search_ptr++ = 0x20;
				*search_ptr++ = 0x20;
				while(((*search_ptr) != 0x0d) && ((*search_ptr) != 0x0a)){
					*search_ptr++ = 0x20;
				}
			}else{
				search_ptr++;
			}
		}else{
			search_ptr++;
		}
	}

/*LNC DATA Table*/
	temp_buf_16 = (uint16_t*)malloc(64*1024*2);
	if(!temp_buf_16){
		CMR_LOGE("LNC table malloc temp_buf_16 mem fail!!!");
		goto update_error;
	}
	search_ptr = file_tmp_buf;
	for(i=0;i<SENSOR_MAP_NUM;i++){
		for(j=0;j<SENSOR_AWB_CALI_NUM;j++){
			char temp_string[3] = {i+'0',j+'0',0};
			uint32_t lnc_data_cnt = 0;
			uint16_t *lnc_buf_alloc = PNULL;
			char* tmp_ptr;

			sprintf(lookup_str,LNC_LOOKUP_STR,sensor_name);
			strcat(lookup_str,temp_string);
			tmp_ptr = strstr(search_ptr,lookup_str);
			if(PNULL == tmp_ptr){
				raw_fix_info_ptr->lnc.map[i][j].len = 0;
				raw_fix_info_ptr->lnc.map[i][j].param_addr = PNULL;
				CMR_LOGE("LNC table %x%x is not exist!!",i,j);
				continue;
			}
			search_ptr = tmp_ptr + strlen(lookup_str);
			while((*search_ptr++) != '}'){
				if((*search_ptr >= '0')&&(*search_ptr <= '9')){
					if((*(search_ptr+1)=='x') || (*(search_ptr+1)=='X')){
						temp_buf_16[lnc_data_cnt++] = (uint16_t)strtol(search_ptr,&search_ptr,16);
					}else{
						temp_buf_16[lnc_data_cnt++] = (uint16_t)strtol(search_ptr,&search_ptr,10);
					}
				}
			}
			if(lnc_data_cnt > 1){
				lnc_buf_alloc = (uint16_t*)malloc(lnc_data_cnt*2);
				if(!lnc_buf_alloc){
					free(temp_buf_16);
					temp_buf_16 = PNULL;
					CMR_LOGE("LNC table %x%x malloc lnc_buf_alloc mem fail!!!",i,j);
					goto update_error;
				}
				raw_info_update_status[sensor_id].lnc_data_table_addr[i][j] = lnc_buf_alloc;
				lnc_buf_alloc = (uint16_t*)((unsigned long)lnc_buf_alloc>>2<<2);
				memcpy(lnc_buf_alloc,temp_buf_16,lnc_data_cnt*2);
				raw_fix_info_ptr->lnc.map[i][j].len = lnc_data_cnt*2;
				raw_fix_info_ptr->lnc.map[i][j].param_addr = lnc_buf_alloc;
			}else{
				raw_fix_info_ptr->lnc.map[i][j].len = 0;
				raw_fix_info_ptr->lnc.map[i][j].param_addr = PNULL;
				CMR_LOGE("LNC table %x%x is empty!!",i,j);
			}
		}
	}
	free(temp_buf_16);
	temp_buf_16 = PNULL;


/*AES DATA Table*/
	search_ptr = file_tmp_buf;
	temp_buf_32 = (uint32_t*)malloc((SENSOR_ISO_NUM*256)*4+32);
	if(!temp_buf_32){
		CMR_LOGE("AES DATA Table malloc temp_buf_32 mem fail!!!");
		goto update_error;
	}
	for(i=0;i<SENSOR_AE_TAB_NUM;i++){
		char temp_string[3] = {(i%2)+'0',(i/2)+'0',0};
		uint32_t aes_data_cnt = 0;
		uint32_t *aes_buf_alloc = PNULL;
		char* tmp_ptr;

		sprintf(lookup_str,AES_LOOKUP_STR,sensor_name);
		strcat(lookup_str,temp_string);
		tmp_ptr = strstr(search_ptr,lookup_str);
		if(PNULL == tmp_ptr){
			CMR_LOGE("AES DATA Table %x is not exit!!!",i);
			goto update_error;
		}
		search_ptr = tmp_ptr + strlen(lookup_str);
		while((*search_ptr++) != '}'){
			if((*search_ptr >= '0')&&(*search_ptr <= '9')){
				if((*(search_ptr+1)=='x') || (*(search_ptr+1)=='X')){
					temp_buf_32[aes_data_cnt++] = (uint32_t)strtol(search_ptr,&search_ptr,16);
				}else{
					temp_buf_32[aes_data_cnt++] = (uint32_t)strtol(search_ptr,&search_ptr,10);
				}
			}
		}
		if(aes_data_cnt > 1){
			aes_buf_alloc = (uint32_t*)malloc((SENSOR_ISO_NUM*256)*4+4);
			if(!aes_buf_alloc){
				free(temp_buf_32);
				temp_buf_32 = PNULL;
				CMR_LOGE("AES DATA Table %x malloc aes_buf_alloc mem fail!!!",i);
				goto update_error;
			}
			raw_info_update_status[sensor_id].aes_data_table_addr[i] = aes_buf_alloc;
			aes_buf_alloc = (uint32_t*)((unsigned long)aes_buf_alloc>>2<<2);
			memcpy(aes_buf_alloc,temp_buf_32,(SENSOR_ISO_NUM*256)*4);
			raw_fix_info_ptr->ae.tab[i].e_ptr= aes_buf_alloc;
		}else{
			CMR_LOGE("AES DATA Table %x is empty!!!",i);
			goto update_error;
		}
	}
	free(temp_buf_32);
	temp_buf_32 = PNULL;

/*AEG DATA Table*/
	search_ptr = file_tmp_buf;
	temp_buf_16 = (uint16_t*)malloc((SENSOR_ISO_NUM*256)*2+32);
	if(!temp_buf_16){
		CMR_LOGE("AEG DATA Table malloc temp_buf_16 mem fail!!!");
		goto update_error;
	}
	for(i=0;i<SENSOR_AE_TAB_NUM;i++){
		char temp_string[3] = {i%2+'0',i/2+'0',0};
		uint32_t aeg_data_cnt = 0;
		uint16_t *aeg_buf_alloc = PNULL;
		char* tmp_ptr;

		sprintf(lookup_str,AEG_LOOKUP_STR,sensor_name);
		strcat(lookup_str,temp_string);
		tmp_ptr = strstr(search_ptr,lookup_str);
		if(PNULL == tmp_ptr){
			CMR_LOGE("AEG DATA Table %x is not exit!!!",i);
			goto update_error;
		}
		search_ptr = tmp_ptr + strlen(lookup_str);
		while((*search_ptr++) != '}'){
			if((*search_ptr >= '0')&&(*search_ptr <= '9')){
				if((*(search_ptr+1)=='x') || (*(search_ptr+1)=='X')){
					temp_buf_16[aeg_data_cnt++] = (uint16_t)strtol(search_ptr,&search_ptr,16);
				}else{
					temp_buf_16[aeg_data_cnt++] = (uint16_t)strtol(search_ptr,&search_ptr,10);
				}
			}
		}
		if(aeg_data_cnt > 1){
			aeg_buf_alloc = (uint16_t*)malloc((SENSOR_ISO_NUM*256)*2+4);
			if(!aeg_buf_alloc){
				free(temp_buf_16);
				temp_buf_16 = PNULL;
				CMR_LOGE("AEG DATA Table %x malloc aeg_buf_alloc mem fail!!!",i);
				goto update_error;
			}
			raw_info_update_status[sensor_id].aeg_data_table_addr[i] = aeg_buf_alloc;
			aeg_buf_alloc = (uint16_t*)((unsigned long)aeg_buf_alloc>>2<<2);
			memcpy(aeg_buf_alloc,temp_buf_16,(SENSOR_ISO_NUM*256)*2);
			raw_fix_info_ptr->ae.tab[i].g_ptr= aeg_buf_alloc;
		}else{
			CMR_LOGE("AEG DATA Table %x is empty!!!",i);
			goto update_error;
		}
	}
	free(temp_buf_16);
	temp_buf_16 = PNULL;

/*AEG Weight Table*/
	search_ptr = file_tmp_buf;
	temp_buf_8 = (uint8_t*)malloc(32*33+32);
	if(!temp_buf_8){
		CMR_LOGE("AEG Weight Table malloc temp_buf_8 mem fail!!!");
		goto update_error;
	}
	{
		uint32_t ae_weight_data_cnt = 0;
		uint8_t *ae_weight_buf_alloc = PNULL;
		char* tmp_ptr;

		sprintf(lookup_str,AE_WEIGHT_LOOKUP_STR,sensor_name);
		tmp_ptr = strstr(search_ptr,lookup_str);
		if(PNULL == tmp_ptr){
			CMR_LOGE("AEG Weight Table is not exit!!!");
			goto update_error;
		}
		search_ptr = tmp_ptr + strlen(lookup_str);
		while((*search_ptr++) != '}'){
			if((*search_ptr >= '0')&&(*search_ptr <= '9')){
				if((*(search_ptr+1)=='x') || (*(search_ptr+1)=='X')){
					temp_buf_8[ae_weight_data_cnt++] = (uint8_t)strtol(search_ptr,&search_ptr,16);
				}else{
					temp_buf_8[ae_weight_data_cnt++] = (uint8_t)strtol(search_ptr,&search_ptr,10);
				}
			}
		}
		if(ae_weight_data_cnt > 1){
			ae_weight_buf_alloc = (uint8_t*)malloc(ae_weight_data_cnt + 4);
			if(!ae_weight_buf_alloc){
				free(temp_buf_8);
				temp_buf_8 = PNULL;
				CMR_LOGE("AEG Weight Table malloc ae_weight_data_cnt mem fail!!!");
				goto update_error;
			}
			raw_info_update_status[sensor_id].ae_weight_table_addr = ae_weight_buf_alloc;
			ae_weight_buf_alloc = (uint8_t*)((unsigned long)ae_weight_buf_alloc>>2<<2);
			memcpy(ae_weight_buf_alloc,temp_buf_8,ae_weight_data_cnt);
			raw_fix_info_ptr->ae.weight_tab = ae_weight_buf_alloc;
		}else{
			CMR_LOGE("AEG Weight Table is empty!!!");
			goto update_error;
		}
	}
	free(temp_buf_8);
	temp_buf_8 = PNULL;

/*Tune Info Table*/
	search_ptr = file_tmp_buf;
	tune_info_need_param_num = sizeof(struct sensor_raw_tune_info);
	temp_buf_8 = (uint8_t*)malloc(tune_info_need_param_num + 32);
	if(!temp_buf_8){
		CMR_LOGE("Tune Info Table malloc temp_buf_8 mem fail!!!");
		goto update_error;
	}
	{
		uint32_t tune_info_data_cnt = 0;
		char* tmp_ptr;

		sprintf(lookup_str,TUNE_INFO_LOOKUP_STR,sensor_name);
		tmp_ptr = strstr(search_ptr,lookup_str);
		if(PNULL == tmp_ptr){
			CMR_LOGE("Tune Info Table is not exist!!!");
			goto update_error;
		}
		search_ptr = tmp_ptr + strlen(lookup_str);
		while(((*search_ptr++) != '}')&&(tune_info_need_param_num >= tune_info_data_cnt)){
			if((*search_ptr >= '0')&&(*search_ptr <= '9')){
				if((*(search_ptr+1)=='x') || (*(search_ptr+1)=='X')){
					temp_buf_8[tune_info_data_cnt++] = (uint8_t)strtol(search_ptr,&search_ptr,16);
				}else{
					temp_buf_8[tune_info_data_cnt++] = (uint8_t)strtol(search_ptr,&search_ptr,10);
				}
			}
		}
		if(tune_info_data_cnt > 1){
			raw_tune_info_ptr = (struct sensor_raw_tune_info*)malloc(sizeof(struct sensor_raw_tune_info)+4);
			if(!raw_tune_info_ptr){
				free(temp_buf_8);
				temp_buf_8 = PNULL;
				CMR_LOGE("Tune Info Table malloc sensor_raw_tune_info mem fail!!!");
				goto update_error;
			}
			raw_info_update_status[sensor_id].tune_info_table_addr = (uint8_t*)raw_tune_info_ptr;
			raw_tune_info_ptr = (struct sensor_raw_tune_info*)((unsigned long)raw_tune_info_ptr>>2<<2);
			memcpy(raw_tune_info_ptr,temp_buf_8,sizeof(struct sensor_raw_tune_info));
		}else{
			CMR_LOGE("Tune Info Table is empty!!!");
			goto update_error;
		}
	}
	free(temp_buf_8);
	temp_buf_8 = PNULL;

/*AWB MAP Table*/
	search_ptr = file_tmp_buf;
	temp_buf_16 = (uint16_t*)malloc(1024*6*2 + 32);
	if(!temp_buf_16){
		CMR_LOGE("AWB MAP Table malloc temp_buf_16 mem fail!!!");
		goto update_error;
	}
	{
		uint32_t awb_map_data_cnt = 0;
		char* tmp_ptr;

		sprintf(lookup_str,AWB_MAP_LOOKUP_STR,sensor_name);
		tmp_ptr = strstr(search_ptr,lookup_str);
		if(PNULL == tmp_ptr){
			CMR_LOGE("AWB MAP Table is not exist!!!");
			goto update_error;
		}
		search_ptr = tmp_ptr + strlen(lookup_str) + 8;
		while((*search_ptr++) != '}'){
			if((*search_ptr >= '0')&&(*search_ptr <= '9')){
				if((*(search_ptr+1)=='x') || (*(search_ptr+1)=='X')){
					temp_buf_16[awb_map_data_cnt++] = (uint16_t)strtol(search_ptr,&search_ptr,16);
				}else{
					temp_buf_16[awb_map_data_cnt++] = (uint16_t)strtol(search_ptr,&search_ptr,10);
				}
			}
			if(*search_ptr == ';'){
				break;
			}
		}
		if(awb_map_data_cnt > 1){
			awb_map_ptr = (uint16_t*)malloc(1024*6*2 + 32);
			if(!awb_map_ptr){
				free(temp_buf_16);
				temp_buf_16 = PNULL;
				CMR_LOGE("AWB MAP Table malloc awb_map_ptr mem fail!!!");
				goto update_error;
			}
			raw_info_update_status[sensor_id].awb_map_table_addr = (uint16_t*)awb_map_ptr;
			awb_map_ptr = (uint16_t*)((unsigned long)awb_map_ptr>>2<<2);
			memcpy(awb_map_ptr,temp_buf_16,1024*6*2);
			raw_fix_info_ptr->awb.addr = awb_map_ptr;
			raw_fix_info_ptr->awb.len = awb_map_data_cnt;
		}else{
			CMR_LOGE("AWB MAP Table is empty!!!");
			awb_map_ptr = PNULL;
		}
	}
	free(temp_buf_16);
	temp_buf_16 = PNULL;



/*load	AE Fix info*/
	search_ptr = file_tmp_buf;
	sprintf(lookup_str,SENSOR_RAW_FIX_INFO_LOOKUP_STR,sensor_name);
	if(!(search_ptr = strstr(search_ptr,lookup_str))){
		CMR_LOGE("AE Fix info is not exist!!!");
		goto update_error;
	}


	sprintf(lookup_str,AEG_LOOKUP_STR,sensor_name);
	for(i=0;i<SENSOR_AE_TAB_NUM;i++){
		char* tmp_ptr;

		tmp_ptr = strstr(search_ptr,lookup_str);
		if(PNULL == tmp_ptr){
			CMR_LOGE("AE table %x info is not exist!!!",i);
			goto update_error;
		}
		search_ptr = tmp_ptr + strlen(lookup_str) + 4;
		for(j=0;j<SENSOR_ISO_NUM;j++){
			while((*search_ptr < '0')||(*search_ptr > '9'))
				search_ptr++;
			if((*(search_ptr+1)=='x') || (*(search_ptr+1)=='X')){
				raw_fix_info_ptr->ae.tab[i].index[j].start = (uint16_t)strtol(search_ptr,&search_ptr,16);
			}else{
				raw_fix_info_ptr->ae.tab[i].index[j].start = (uint16_t)strtol(search_ptr,&search_ptr,10);
			}
			while((*search_ptr < '0')||(*search_ptr > '9'))
				search_ptr++;
			if((*(search_ptr+1)=='x') || (*(search_ptr+1)=='X')){
				raw_fix_info_ptr->ae.tab[i].index[j].max = (uint16_t)strtol(search_ptr,&search_ptr,16);
			}else{
				raw_fix_info_ptr->ae.tab[i].index[j].max = (uint16_t)strtol(search_ptr,&search_ptr,10);
			}
		}
	}


/*load	LNC Fix info*/
	for(i=0;i<SENSOR_MAP_NUM;i++){
		for(j=0;j<SENSOR_AWB_CALI_NUM;j++){
			search_ptr = strstr(search_ptr,"0x");
			raw_fix_info_ptr->lnc.map[i][j].grid = (uint32_t)strtol(search_ptr,&search_ptr,16);
			search_ptr += 4;
		}
	}

/*load	AWB MAP Table Len*/

	search_ptr = file_tmp_buf;
	sprintf(lookup_str,SENSOR_RAW_FIX_INFO_LOOKUP_STR,sensor_name);
	if(!(search_ptr = strstr(search_ptr,lookup_str))){
		CMR_LOGE("AE Fix info is not exist!!!");
		goto update_error;
	}
	{
		char* tmp_ptr;
		sprintf(lookup_str,AWB_MAP_LOOKUP_STR,sensor_name);
		tmp_ptr = strstr(search_ptr,lookup_str);
		if(PNULL == tmp_ptr){
			CMR_LOGE("WB MAP Table Len is not exist!!!");
			goto update_error;
		}
		search_ptr = tmp_ptr + strlen(lookup_str);
		while((*search_ptr++) != '}'){
			while((*search_ptr < '0')||(*search_ptr > '9'))
				search_ptr++;
			if((*(search_ptr+1)=='x') || (*(search_ptr+1)=='X')){
				raw_fix_info_ptr->awb.len = (uint16_t)strtol(search_ptr,&search_ptr,16);
			}else{
				raw_fix_info_ptr->awb.len = (uint16_t)strtol(search_ptr,&search_ptr,10);
			}
			break;
		}
	}
	(*sensor_info_ptr->raw_info_ptr)->tune_ptr = raw_tune_info_ptr;
	(*sensor_info_ptr->raw_info_ptr)->fix_ptr = raw_fix_info_ptr;
	CMR_LOGE("file update success!!!");

	free(file_tmp_buf);
	file_tmp_buf = PNULL;
	return SENSOR_SUCCESS;
update_error:

	_isp_raw_para_recover(sensor_info_ptr,sensor_id);
	if(file_tmp_buf){
		free(file_tmp_buf);
		file_tmp_buf = PNULL;
	}
	if (temp_buf_8) {
		free(temp_buf_8);
		temp_buf_8 = PNULL;
	}
	if (temp_buf_16) {
		free(temp_buf_16);
		temp_buf_16 = PNULL;
	}
	if (temp_buf_32) {
		free(temp_buf_32);
		temp_buf_32 = PNULL;
	}
	return SENSOR_FAIL;
}


/**----------------------------------------------------------------------------*
**				Compiler Flag					*
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/

// End

