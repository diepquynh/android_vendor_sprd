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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "isp_log.h"

#include "isp_param_file_update.h"

/**---------------------------------------------------------------------------*
 **				Compiler Flag					*
 **---------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C"
{
#endif

#define LNC_MAP_NUM 9
#define SCENE_INFO_NUM 10
#define ISP_PARAM_VERSION_OLD 0x00030002

static int32_t count = 8;

int read_mode(FILE* fp, uint8_t * buf, int* plen)
{
	int rtn = 0x00;
	uint8_t* param_buf = buf;
	int i;
	unsigned char line_buff[1024];

	while (!feof(fp)) {
		uint8_t c[16];
		int n = 0;

		if (fgets(line_buff, 1024, fp) == NULL) {
			break;
		}

		if (strstr(line_buff, "{") != NULL) {
			continue;
		}

		if (strstr(line_buff, "/*") != NULL) {
			continue;
		}

		if (strstr(line_buff, "};") != NULL) {
			break;
		}


		n = sscanf(line_buff, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", &c[0], &c[1], &c[2], &c[3], &c[4], &c[5], &c[6], &c[7], &c[8], &c[9], &c[10], &c[11], &c[12], &c[13], &c[14], &c[15]);
		for (i=0; i<n; i++) {
			*param_buf ++ = c[i];
		}
	}

	*plen = (long)param_buf - (long)(buf);
	return rtn;
}

int update_param_v1(struct sensor_raw_info* sensor_raw_info_ptr,unsigned char *sensor_name)
{
	int rtn = 0x00;

	FILE* fp = NULL;
	unsigned char filename[80];
	unsigned char line_buff[512];
	int mode_flag[16] = {0};
	unsigned char mode_name_prefix[80];
	unsigned char mode_name[16][80];
	unsigned char mode_info_array[80];
	unsigned char mode_info[16][80];
	int i;

	sprintf(filename, "/data/misc/media/sensor_%s_raw_param_v3.c", sensor_name);

	sprintf(mode_name_prefix, "static uint8_t s_%s_tune_info_", sensor_name);

	sprintf(mode_name[0], "static uint8_t s_%s_tune_info_common[]=", sensor_name);
	sprintf(mode_name[1], "static uint8_t s_%s_tune_info_prv_0[]=", sensor_name);
	sprintf(mode_name[2], "static uint8_t s_%s_tune_info_prv_1[]=", sensor_name);
	sprintf(mode_name[3], "static uint8_t s_%s_tune_info_prv_2[]=", sensor_name);
	sprintf(mode_name[4], "static uint8_t s_%s_tune_info_prv_3[]=", sensor_name);
	sprintf(mode_name[5], "static uint8_t s_%s_tune_info_cap_0[]=", sensor_name);
	sprintf(mode_name[6], "static uint8_t s_%s_tune_info_cap_1[]=", sensor_name);
	sprintf(mode_name[7], "static uint8_t s_%s_tune_info_cap_2[]=", sensor_name);
	sprintf(mode_name[8], "static uint8_t s_%s_tune_info_cap_3[]=", sensor_name);
	sprintf(mode_name[9], "static uint8_t s_%s_tune_info_video_0[]=", sensor_name);
	sprintf(mode_name[10], "static uint8_t s_%s_tune_info_video_1[]=", sensor_name);
	sprintf(mode_name[11], "static uint8_t s_%s_tune_info_video_2[]=", sensor_name);
	sprintf(mode_name[12], "static uint8_t s_%s_tune_info_video_3[]=", sensor_name);


	sprintf(mode_info_array, "static struct sensor_raw_info s_%s_mipi_raw_info=", sensor_name);

	sprintf(mode_info[0], "{s_%s_tune_info_common, sizeof(s_%s_tune_info_common)},", sensor_name, sensor_name);
	sprintf(mode_info[1], "{s_%s_tune_info_prv_0, sizeof(s_%s_tune_info_prv_0)},", sensor_name, sensor_name);
	sprintf(mode_info[2], "{s_%s_tune_info_prv_1, sizeof(s_%s_tune_info_prv_1)},", sensor_name, sensor_name);
	sprintf(mode_info[3], "{s_%s_tune_info_prv_2, sizeof(s_%s_tune_info_prv_2)},", sensor_name, sensor_name);
	sprintf(mode_info[4], "{s_%s_tune_info_prv_3, sizeof(s_%s_tune_info_prv_3)},", sensor_name, sensor_name);
	sprintf(mode_info[5], "{s_%s_tune_info_cap_0, sizeof(s_%s_tune_info_cap_0)},", sensor_name, sensor_name);
	sprintf(mode_info[6], "{s_%s_tune_info_cap_1, sizeof(s_%s_tune_info_cap_1)},", sensor_name, sensor_name);
	sprintf(mode_info[7], "{s_%s_tune_info_cap_2, sizeof(s_%s_tune_info_cap_2)},", sensor_name, sensor_name);
	sprintf(mode_info[8], "{s_%s_tune_info_cap_3, sizeof(s_%s_tune_info_cap_3)},", sensor_name, sensor_name);
	sprintf(mode_info[9], "{s_%s_tune_info_video_0, sizeof(s_%s_tune_info_video_0)},", sensor_name, sensor_name);
	sprintf(mode_info[10], "{s_%s_tune_info_video_1, sizeof(s_%s_tune_info_video_1)},", sensor_name, sensor_name);
	sprintf(mode_info[11], "{s_%s_tune_info_video_2, sizeof(s_%s_tune_info_video_2)},", sensor_name, sensor_name);
	sprintf(mode_info[12], "{s_%s_tune_info_video_3, sizeof(s_%s_tune_info_video_3)},", sensor_name, sensor_name);


	ISP_LOGI("ISP try to read %s", filename);
	fp = fopen(filename, "r");
	if (fp == NULL) {
		return -1;
	}


	while (!feof(fp)) {
		if (fgets(line_buff, 512, fp) == NULL) {
			break;
		}

		if (strstr(line_buff, mode_name_prefix) != NULL) {
			for (i=0; i<16; i++) {
				if (strstr(line_buff, mode_name[i]) != NULL) {
					read_mode(fp, sensor_raw_info_ptr->mode_ptr[i].addr, &(sensor_raw_info_ptr->mode_ptr[i].len));
					break;
				}
			}
		} else if (strstr(line_buff, mode_info_array) != NULL) {
			while (!feof(fp)) {
				if (fgets(line_buff, 512, fp) == NULL) {
					break;
				}

				for (i=0; i<16; i++) {
					if (strstr(line_buff, mode_info[i]) != NULL) {
						mode_flag[i] = 1;
						break;
					}
				}
			}
		}
	}


	for (i=0; i<16; i++) {
		if (mode_flag[i] == 0) {
			sensor_raw_info_ptr->mode_ptr[i].addr = NULL;
			sensor_raw_info_ptr->mode_ptr[i].len = 0;
		}
	}

	fclose(fp);
	return rtn;
}

int read_tune_info(FILE *fp,uint8_t *data_ptr,uint32_t *data_len)
{
	int rtn =0x00;

	uint8_t *param_buf = data_ptr;
	unsigned char line_buff[256];
	int i;

	while (!feof(fp)) {
		uint8_t c[16];
		int n = 0;
		if (fgets(line_buff, 256, fp) == NULL) {
			break;
		}
		if (strstr(line_buff, "{") != NULL) {
			continue;
		}

		if (strstr(line_buff, "/*") != NULL) {
			continue;
		}

		if (strstr(line_buff, "};") != NULL) {
			break;
		}

		n = sscanf(line_buff, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", &c[0], &c[1], &c[2], &c[3], &c[4], &c[5], &c[6], &c[7], &c[8], &c[9], &c[10], &c[11], &c[12], &c[13], &c[14], &c[15]);
		for (i=0; i<n; i++) {
			*param_buf ++ = c[i];
		}
	}
	*data_len = (long)param_buf - (long)(data_ptr);
	return rtn;
}

int read_ae_table_32bit(FILE *fp,uint32_t *data_ptr,uint32_t *data_len)
{
	int rtn = 0x00;

	int i;
	uint32_t *param_buf = data_ptr;
	unsigned char line_buf[512];
	while (!feof(fp)) {
		uint32_t c1[16];
		int n = 0;

		if (fgets(line_buf, 512, fp) == NULL) {
			break;
		}

		if (strstr(line_buf, "{") != NULL) {
			continue;
		}

		if (strstr(line_buf, "/*") != NULL) {
			continue;
		}

		if (strstr(line_buf, "};") != NULL) {
			break;
		}

		n = sscanf(line_buf, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", &c1[0], &c1[1], &c1[2], &c1[3], &c1[4], &c1[5], &c1[6], &c1[7], &c1[8], &c1[9], &c1[10], &c1[11], &c1[12], &c1[13], &c1[14], &c1[15]);

		for (i=0; i<n; i++) {
			*param_buf ++ = c1[i];
		}
	}

	*data_len = (long)param_buf - (long)data_ptr;

	return rtn;
}

int read_ae_table_16bit(FILE *fp,uint16_t *data_ptr,uint32_t *data_len)
{
	int rtn = 0x00;

	int i;
	uint16_t *param_buf = data_ptr;
	unsigned char line_buf[512];

	while (!feof(fp)) {
		uint16_t c2[16];
		int n = 0;

		if (fgets(line_buf, 512, fp) == NULL) {
			break;
		}

		if (strstr(line_buf, "{") != NULL) {
			continue;
		}

		if (strstr(line_buf, "/*") != NULL) {
			continue;
		}

		if (strstr(line_buf, "};") != NULL) {
			break;
		}

		n = sscanf(line_buf, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", &c2[0], &c2[1], &c2[2], &c2[3], &c2[4], &c2[5], &c2[6], &c2[7], &c2[8], &c2[9], &c2[10], &c2[11], &c2[12], &c2[13], &c2[14], &c2[15]);

		for (i=0; i<n; i++) {
			*param_buf ++ = c2[i];
		}
	}

	*data_len = (long)param_buf - (long)data_ptr;

	return rtn;
}

int read_ae_weight(FILE *fp,struct ae_weight_tab *weight_ptr)
{
	int rtn = 0x00;

	uint8_t *param_buf = weight_ptr->weight_table;
	int i;
	unsigned char line_buff[512];

	while (!feof(fp)) {
		uint8_t c[16];
		int n = 0;

		if (fgets(line_buff, 512, fp) == NULL) {
			break;
		}
		if (strstr(line_buff, "{") != NULL) {
			continue;
		}
		if (strstr(line_buff, "/*") != NULL) {
			continue;
		}
		if (strstr(line_buff, "};") != NULL) {
			break;
		}
		n = sscanf(line_buff, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", &c[0], &c[1], &c[2], &c[3], &c[4], &c[5], &c[6], &c[7], &c[8], &c[9], &c[10], &c[11], &c[12], &c[13], &c[14], &c[15]);
		for (i=0; i<n; i++) {
			*param_buf ++ = c[i];
		}
	}
	weight_ptr->len = (long)param_buf - (long)(weight_ptr->weight_table);

	return 0;
}

int read_ae_scene_info(FILE *fp,struct sensor_ae_tab *ae_ptr,int scene_mode)
{
	int rtn = 0x00;

	uint32_t *param_buf = ae_ptr->scene_tab[scene_mode][0].scene_info;
	uint32_t *param_buf1 = ae_ptr->scene_tab[scene_mode][1].scene_info;
	int i,j;
	unsigned char line_buff[512];

	while (!feof(fp)) {
		uint32_t c[16];
		int n = 0;

		if (fgets(line_buff, 512, fp) == NULL) {
			break;
		}
		if (strstr(line_buff, "{") != NULL) {
			continue;
		}
		if (strstr(line_buff, "/*") != NULL) {
			continue;
		}
		if (strstr(line_buff, "};") != NULL) {
			break;
		}
		n = sscanf(line_buff, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", &c[0], &c[1], &c[2], &c[3], &c[4], &c[5], &c[6], &c[7], &c[8], &c[9], &c[10], &c[11], &c[12], &c[13], &c[14], &c[15]);

		for (i=0; i<n; i++) {
			*param_buf ++ = c[i];
			*param_buf1 ++ = c[i];
		}
	}
	ae_ptr->scene_tab[scene_mode][0].scene_info_len = (long)param_buf - (long)(ae_ptr->scene_tab[scene_mode][0].scene_info);
	ae_ptr->scene_tab[scene_mode][1].scene_info_len = (long)param_buf - (long)(ae_ptr->scene_tab[scene_mode][1].scene_info);

	return rtn;
}

int read_ae_auto_iso(FILE *fp,struct ae_auto_iso_tab_v1 *auto_iso_ptr)
{
	int rtn = 0x00;

	uint16_t *param_buf = auto_iso_ptr->auto_iso_tab;
	int i;
	unsigned char line_buff[512];

	while (!feof(fp)) {
		uint16_t c[16];
		int n = 0;

		if (fgets(line_buff, 512, fp) == NULL) {
			break;
		}

		if (strstr(line_buff, "{") != NULL) {
			continue;
		}

		if (strstr(line_buff, "/*") != NULL) {
			continue;
		}

		if (strstr(line_buff, "};") != NULL) {
			break;
		}


		n = sscanf(line_buff, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", &c[0], &c[1], &c[2], &c[3], &c[4], &c[5], &c[6], &c[7], &c[8], &c[9], &c[10], &c[11], &c[12], &c[13], &c[14], &c[15]);
		for (i=0; i<n; i++) {
			*param_buf ++ = c[i];
		}
	}

	auto_iso_ptr->len = (long)param_buf - (long)(auto_iso_ptr->auto_iso_tab);

	return 0;
}

int read_fix_ae_info(FILE *fp,struct sensor_ae_tab *ae_ptr)
{
	int rtn = 0x00;

	int flag_end = 0;
	int i,j;
	unsigned char *ae_tab_info[5] = {NULL};
	unsigned char *ae_scene_info[6] = {NULL};
	unsigned char ae_auto_iso_info[50];
	unsigned char *ae_weight_info[50];

	unsigned char *line_buf = (unsigned char *)malloc(512*sizeof(unsigned char));
	if ( NULL == line_buf) {
		ISP_LOGE("malloc mem was error!");
		rtn = 0x01;
		return rtn;
	}

	for(i = 0;i < 5;i++) {
		ae_tab_info[i] = (unsigned char *)malloc(50*sizeof(unsigned char));
		if ( NULL == ae_tab_info[i]) {
			ISP_LOGE("malloc mem was error!");
			rtn = 0x01;
			goto exit;
		}
	}
	for(i = 0;i < 6;i++) {
		ae_scene_info[i] = (unsigned char *)malloc(50*sizeof(unsigned char));
		if ( NULL == ae_scene_info[i]) {
			ISP_LOGE("malloc mem was error!");
			rtn = 0x01;
			goto exit;
		}
	}
	while (!feof(fp)) {
		if (NULL == fgets(line_buf,512,fp)) {
			break;
		}
		if (strstr(line_buf,"_ae_tab_") != NULL && strstr(line_buf,"_scene_ae_tab_") == NULL) {
			for (i = 0;i < AE_FLICKER_NUM;i++) {
				int break_flag = 0;
				for (j = 0;j < AE_ISO_NUM_NEW;j++) {
					sprintf(ae_tab_info[0],"_ae_tab_index_%d%d",i,j);
					sprintf(ae_tab_info[1],"_ae_tab_exposure_%d%d",i,j);
					sprintf(ae_tab_info[2],"_ae_tab_dummy_%d%d",i,j);
					sprintf(ae_tab_info[3],"_ae_tab_again_%d%d",i,j);
					sprintf(ae_tab_info[4],"_ae_tab_dgain_%d%d",i,j);
					if ((strstr(line_buf,"static uint32_t") != NULL)&&(strstr(line_buf,ae_tab_info[0]) != NULL)) {
						rtn = read_ae_table_32bit(fp,ae_ptr->ae_tab[i][j].index,&ae_ptr->ae_tab[i][j].index_len);
						if (0x00 != rtn) {
							goto exit;
						}
						break_flag =1;
						break;
					}
					if ((strstr(line_buf,"static uint32_t") != NULL)&&(strstr(line_buf,ae_tab_info[1]) != NULL)) {
						rtn = read_ae_table_32bit(fp,ae_ptr->ae_tab[i][j].exposure,&ae_ptr->ae_tab[i][j].exposure_len);
						if (0x00 != rtn) {
							goto exit;
						}
						break_flag =1;
						break;
					}
					if ((strstr(line_buf,"static uint32_t") != NULL)&&(strstr(line_buf,ae_tab_info[2]) != NULL)) {
						rtn = read_ae_table_32bit(fp,ae_ptr->ae_tab[i][j].dummy,&ae_ptr->ae_tab[i][j].dummy_len);
						if(0x00 != rtn) {
							goto exit;
						}
						break_flag =1;
						break;
					}
					if ((strstr(line_buf,"static uint16_t") != NULL)&&(strstr(line_buf,ae_tab_info[3]) != NULL)) {
						rtn = read_ae_table_16bit(fp,ae_ptr->ae_tab[i][j].again,&ae_ptr->ae_tab[i][j].again_len);
						if (0x00 != rtn) {
							goto exit;
						}
						break_flag =1;
						break;
					}
					if ((strstr(line_buf,"static uint16_t") != NULL)&&(strstr(line_buf,ae_tab_info[4]) != NULL)) {
						rtn = read_ae_table_16bit(fp,ae_ptr->ae_tab[i][j].dgain,&ae_ptr->ae_tab[i][j].dgain_len);
						if (0x00 != rtn) {
							goto exit;
						}
						break_flag =1;
						break;
					}
				}
				if (0 != break_flag)
					break;
			}
		}
		if (strstr(line_buf,"_ae_weight_") != NULL ) {
			for (i = 0;i < AE_WEIGHT_TABLE_NUM;i++) {
				sprintf(ae_weight_info,"_ae_weight_%d",i);
				if ((strstr(line_buf,"static uint8_t") != NULL ) && (strstr(line_buf,ae_weight_info) != NULL)) {
					rtn = read_ae_weight(fp,&ae_ptr->weight_tab[i]);
					break;
				}
			}
		}
		if (strstr(line_buf,"_scene_") != NULL && (strstr(line_buf,"static uint32_t") != NULL ||strstr(line_buf,"static uint16_t") != NULL)) {
			for (i = 0;i < AE_SCENE_NUM;i++) {
				int break_flag = 0;
				sprintf(ae_scene_info[0],"_ae_scene_info_%d",i);
				if (strstr(line_buf,ae_scene_info[0]) != NULL){
					rtn = read_ae_scene_info(fp,ae_ptr,i);
					if (rtn != 0x00){
						rtn = 0x01;
						goto exit;
					}
					break;
				}
				for (j = 0;j < AE_FLICKER_NUM;j++) {
					sprintf(ae_scene_info[1],"_scene_ae_tab_index_%d%d",i,j);
					sprintf(ae_scene_info[2],"_scene_ae_tab_exposure_%d%d",i,j);
					sprintf(ae_scene_info[3],"_scene_ae_tab_dummy_%d%d",i,j);
					sprintf(ae_scene_info[4],"_scene_ae_tab_again_%d%d",i,j);
					sprintf(ae_scene_info[5],"_scene_ae_tab_dgain_%d%d",i,j);
					if ((strstr(line_buf,"static uint32_t") != NULL)&&(strstr(line_buf,ae_scene_info[1]) != NULL)) {
						rtn = read_ae_table_32bit(fp,ae_ptr->scene_tab[i][j].index,&ae_ptr->scene_tab[i][j].index_len);
						if (0x00 != rtn) {
							goto exit;
						}
						break_flag = 1;
						break;
					}
					if ((strstr(line_buf,"static uint32_t") != NULL)&&(strstr(line_buf,ae_scene_info[2]) != NULL)) {
						rtn = read_ae_table_32bit(fp,ae_ptr->scene_tab[i][j].exposure,&ae_ptr->scene_tab[i][j].exposure_len);
						if (0x00 != rtn) {
							goto exit;
						}
						break_flag = 1;
						break;
					}
					if ((strstr(line_buf,"static uint32_t") != NULL)&&(strstr(line_buf,ae_scene_info[3]) != NULL)) {
						rtn = read_ae_table_32bit(fp,ae_ptr->scene_tab[i][j].dummy,&ae_ptr->scene_tab[i][j].dummy_len);
						if (0x00 != rtn) {
							goto exit;
						}
						break_flag = 1;
						break;
					}
					if ((strstr(line_buf,"static uint16_t") != NULL)&&(strstr(line_buf,ae_scene_info[4]) != NULL)) {
						rtn = read_ae_table_16bit(fp,ae_ptr->scene_tab[i][j].again,&ae_ptr->scene_tab[i][j].again_len);
						if (0x00 != rtn) {
							goto exit;
						}
						break_flag = 1;
						break;
					}
					if ((strstr(line_buf,"static uint16_t") != NULL)&&(strstr(line_buf,ae_scene_info[5]) != NULL)) {
						rtn = read_ae_table_16bit(fp,ae_ptr->scene_tab[i][j].dgain,&ae_ptr->scene_tab[i][j].dgain_len);
						if (0x00 != rtn) {
							goto exit;
						}
						break_flag = 1;
						break;
					}
				}
				if (0 != break_flag)
					break;
			}
		}
		if (strstr(line_buf,"_ae_auto_iso_") != NULL ) {
			for (i = 0;i < AE_FLICKER_NUM;i++) {
				sprintf(ae_auto_iso_info,"_ae_auto_iso_%d",i);
				if ((strstr(line_buf,"static uint16_t") != NULL )&& (strstr(line_buf,ae_auto_iso_info) != NULL)) {
					rtn = read_ae_auto_iso(fp,&ae_ptr->auto_iso_tab[i]);
					if (0x00 != rtn) {
						goto exit;
					}
					if (1 == i)
						flag_end = 1;
					break;
				}
			}
		}
		if (0 != flag_end)
			break;
	}

exit:
	if (NULL != line_buf){
		free(line_buf);
	}
	for (i = 0; i < 5; i++) {
		if (NULL != ae_tab_info[i]) {
			free(ae_tab_info[i]);
		}
	}
	for (i = 0; i < 6; i++) {
		if (NULL != ae_scene_info[i]) {
			free(ae_scene_info[i]);
		}
	}
	return rtn;
}

int read_awb_win_map(FILE *fp,struct sensor_awb_map *awb_map_ptr)
{
	int rtn = 0x00;

	uint16_t *param_buf = awb_map_ptr->addr;
	int i;
	unsigned char line_buff[512];

	while (!feof(fp)) {
		uint16_t c[16];
		int n = 0;

		if (fgets(line_buff, 512, fp) == NULL) {
			break;
		}

		if (strstr(line_buff, "{") != NULL) {
			continue;
		}

		if (strstr(line_buff, "/*") != NULL) {
			continue;
		}

		if (strstr(line_buff, "};") != NULL) {
			break;
		}


		n = sscanf(line_buff, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", &c[0], &c[1], &c[2], &c[3], &c[4], &c[5], &c[6], &c[7], &c[8], &c[9], &c[10], &c[11], &c[12], &c[13], &c[14], &c[15]);
		for (i = 0; i < n; i++) {
			*param_buf ++ = c[i];
		}
	}

	awb_map_ptr->len = (long)param_buf - (long)(awb_map_ptr->addr);

	return 0;
}

int read_awb_pos_weight(FILE *fp,struct sensor_awb_weight *awb_weight_ptr)
{
	int rtn = 0x00;

	uint8_t *param_buf = awb_weight_ptr->addr;
	int i;
	unsigned char line_buff[512];

	while (!feof(fp)) {
		uint8_t c[16];
		int n = 0;

		if (fgets(line_buff, 512, fp) == NULL) {
			break;
		}

		if (strstr(line_buff, "{") != NULL) {
			continue;
		}

		if (strstr(line_buff, "/*") != NULL) {
			continue;
		}

		if (strstr(line_buff, "};") != NULL) {
			break;
		}


		n = sscanf(line_buff, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d", &c[0], &c[1], &c[2], &c[3], &c[4], &c[5], &c[6], &c[7], &c[8], &c[9], &c[10], &c[11], &c[12], &c[13], &c[14], &c[15]);
		for (i=0; i<n; i++) {
			*param_buf ++ = c[i];
		}
	}

	awb_weight_ptr->weight_len = (long)param_buf - (long)(awb_weight_ptr->addr);

	return rtn;
}

int	read_awb_width_height(FILE *fp,struct sensor_awb_weight *awb_weight_ptr)
{
	int rtn = 0x00;

	uint16_t *param_buf = awb_weight_ptr->size;
	int i;
	unsigned char line_buff[512];

	while (!feof(fp)) {
		uint16_t c[16];
		int n = 0;

		if (fgets(line_buff, 512, fp) == NULL) {
			break;
		}

		if (strstr(line_buff, "{") != NULL) {
			continue;
		}

		if (strstr(line_buff, "/*") != NULL) {
			continue;
		}

		if (strstr(line_buff, "};") != NULL) {
			break;
		}


		n = sscanf(line_buff, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", &c[0], &c[1], &c[2], &c[3], &c[4], &c[5], &c[6], &c[7], &c[8], &c[9], &c[10], &c[11], &c[12], &c[13], &c[14], &c[15]);
		for (i=0; i<n; i++) {
			*param_buf ++ = c[i];
		}
	}

	awb_weight_ptr->size_param_len = (long)param_buf - (long)(awb_weight_ptr->size);

	return 0;
}

int read_fix_awb_info(FILE *fp,struct sensor_awb_map_weight_param *awb_ptr)
{
	int rtn = 0x00;

	int flag_end = 0;
	unsigned char line_buf[512];

	while (!feof(fp)) {
		if (NULL == fgets(line_buf,512,fp)) {
			break;
		}
		if ((strstr(line_buf,"static uint16_t") != NULL) && (strstr(line_buf,"_awb_win_map") != NULL)) {
			rtn = read_awb_win_map(fp,&awb_ptr->awb_map);
			if (0x00 != rtn) {
				return rtn;
			}
		}
		if ((strstr(line_buf,"static uint8_t") != NULL) && (strstr(line_buf,"_awb_pos_weight") != NULL)) {
			rtn = read_awb_pos_weight(fp,&awb_ptr->awb_weight);
			if (0x00 != rtn) {
				return rtn;
			}
		}
		if ((strstr(line_buf,"static uint16_t") != NULL) && (strstr(line_buf,"_awb_pos_weight_width_height") != NULL)) {
			rtn = read_awb_width_height(fp,&awb_ptr->awb_weight);
			if (0x00 != rtn) {
				return rtn;
			}
			flag_end = 1;
		}
		if (0 != flag_end)
			break;
	}

	return rtn;
}

int read_lnc_map(FILE *fp,struct sensor_lens_map *lnc_map_ptr)
{
	int rtn = 0x00;

	uint32_t *param_buf = lnc_map_ptr->map_info;
	int i;
	unsigned char line_buff[512];

	while (!feof(fp)) {
		uint32_t c[16];
		int n = 0;

		if (fgets(line_buff, 512, fp) == NULL)
		{
			break;
		}

		if (strstr(line_buff, "{") != NULL) {
			continue;
		}

		if (strstr(line_buff, "/*") != NULL) {
			continue;
		}

		if (strstr(line_buff, "};") != NULL) {
			break;
		}

		n = sscanf(line_buff, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", &c[0], &c[1], &c[2], &c[3], &c[4], &c[5], &c[6], &c[7], &c[8], &c[9], &c[10], &c[11], &c[12], &c[13], &c[14], &c[15]);
		for (i=0; i<n; i++) {
			*param_buf ++ = c[i];
		}
	}

	lnc_map_ptr->map_info_len = (long)param_buf - (long)(lnc_map_ptr->map_info);

	return rtn;
}

int read_lnc_info(FILE *fp,struct sensor_lens_map *lnc_map_ptr)
{
	int rtn = 0x00;

	uint16_t *param_buf = lnc_map_ptr->lnc_addr;
	int i;
	unsigned char line_buff[512];

	while (!feof(fp)) {
		uint16_t c[16];
		int n = 0;

		if (fgets(line_buff, 512, fp) == NULL) {
			break;
		}

		if (strstr(line_buff, "{") != NULL) {
			continue;
		}

		if (strstr(line_buff, "/*") != NULL) {
			continue;
		}

		if (strstr(line_buff, "};") != NULL) {
			break;
		}

		n = sscanf(line_buff, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", &c[0], &c[1], &c[2], &c[3], &c[4], &c[5], &c[6], &c[7], &c[8], &c[9], &c[10], &c[11], &c[12], &c[13], &c[14], &c[15]);
		for (i=0; i<n; i++) {
			*param_buf ++ = c[i];
		}
	}

	lnc_map_ptr->lnc_len = (long)param_buf - (long)(lnc_map_ptr->lnc_addr);
	return rtn;
}

int read_fix_lnc_info(FILE *fp,struct sensor_lsc_map *lnc_ptr)
{
	int rtn = 0x00;

	int i;
	int flag_end = 0;

	unsigned char lnc_map_info[50];
	unsigned char lnc_info[20];
	//unsigned char line_buf[512];

	unsigned char *line_buf = (unsigned char *)malloc(512*sizeof(unsigned char));
	if (NULL == line_buf) {
		ISP_LOGE("malloc mem was error!");
		rtn = 0x01;
		return rtn;
	}
	while (!feof(fp)) {
		if (NULL == fgets(line_buf,512,fp)) {
			break;
		}
		if (NULL != strstr(line_buf,"_lnc_map_info")) {
			for(i = 0;i < LNC_MAP_NUM;i++) {
				sprintf(lnc_map_info,"_lnc_map_info_0%d",i);
				if ((strstr(line_buf,"static uint32_t") != NULL) && (strstr(line_buf,lnc_map_info) != NULL)) {
					rtn = read_lnc_map(fp,&lnc_ptr->map[i]);
					if (0x00 != rtn) {
						return rtn;
					}
					break;
				}
			}
		}
		if (NULL == strstr(line_buf,"_lnc_map_info") && NULL != strstr(line_buf,"_lnc_")) {
			for (i = 0;i < LNC_MAP_NUM;i++) {
				sprintf(lnc_info,"_lnc_0%d",i);
				if ((strstr(line_buf,"static uint16_t") != NULL) && (strstr(line_buf,lnc_info) != NULL)) {
					rtn = read_lnc_info(fp,&lnc_ptr->map[i]);
					if (0x00 != rtn) {
						return rtn;
					}
					if ((LNC_MAP_NUM - 1) == i)
						flag_end = 1;
					break;
				}
			}
		}
		if(0 != flag_end)
			break;
	}

	if (NULL != line_buf) {
		free(line_buf);
		line_buf = NULL;
	}
	return rtn;
}

int read_libuse_info(FILE *fp,struct sensor_raw_info *sensor_raw_ptr)
{
	int rtn = 0x00;

	uint32_t *param_buf = sensor_raw_ptr->libuse_info;
	int i;
	unsigned char line_buff[512];

	while (!feof(fp)) {
		uint32_t c[16];
		int n = 0;

		if (fgets(line_buff, 512, fp) == NULL) {
			break;
		}

		if (strstr(line_buff, "{") != NULL) {
			continue;
		}

		if (strstr(line_buff, "/*") != NULL) {
			continue;
		}

		if (strstr(line_buff, "};") != NULL) {
			break;
		}


		n = sscanf(line_buff, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x", &c[0], &c[1], &c[2], &c[3], &c[4], &c[5], &c[6], &c[7], &c[8], &c[9], &c[10], &c[11], &c[12], &c[13], &c[14], &c[15]);
		for (i = 0; i < n; i++) {
			*param_buf ++ = c[i];
		}
	}

	return 0;
}

int update_param_v2(struct sensor_raw_info *sensor_raw_ptr,unsigned char *sensor_name)
{
	int rtn = 0x00;

	int i,j = 0;

	unsigned char tune_info[80];
	unsigned char fixname[50];
	unsigned char note_name[50];
	unsigned char libuse_info[50];

	//unsigned char line_buf[512];
	//unsigned char filename[13][80];

	unsigned char *line_buf = (unsigned char *)malloc(512*sizeof(unsigned char));
	unsigned char *filename[14] = {NULL};

	if (NULL == line_buf) {
		ISP_LOGI("malloc mem was error!");
		rtn = 0x01;
		return rtn;
	}

	for (i = 0;i < 14;i++) {
		filename[i] = (unsigned char *)malloc(80*sizeof(unsigned char));
		if (NULL == filename[i]) {
			ISP_LOGI("malloc mem was error!");
			rtn = 0x01;
			goto exit;
		}
	}
	sprintf(filename[0],"/data/misc/media/sensor_%s_raw_param_common.c",sensor_name);
	sprintf(filename[1],"/data/misc/media/sensor_%s_raw_param_prv_0.c",sensor_name);
	sprintf(filename[2],"/data/misc/media/sensor_%s_raw_param_prv_1.c",sensor_name);
	sprintf(filename[3],"/data/misc/media/sensor_%s_raw_param_prv_2.c",sensor_name);
	sprintf(filename[4],"/data/misc/media/sensor_%s_raw_param_prv_3.c",sensor_name);
	sprintf(filename[5],"/data/misc/media/sensor_%s_raw_param_cap_0.c",sensor_name);
	sprintf(filename[6],"/data/misc/media/sensor_%s_raw_param_cap_1.c",sensor_name);
	sprintf(filename[7],"/data/misc/media/sensor_%s_raw_param_cap_2.c",sensor_name);
	sprintf(filename[8],"/data/misc/media/sensor_%s_raw_param_cap_3.c",sensor_name);
	sprintf(filename[9],"/data/misc/media/sensor_%s_raw_param_video_0.c",sensor_name);
	sprintf(filename[10],"/data/misc/media/sensor_%s_raw_param_video_1.c",sensor_name);
	sprintf(filename[11],"/data/misc/media/sensor_%s_raw_param_video_2.c",sensor_name);
	sprintf(filename[12],"/data/misc/media/sensor_%s_raw_param_video_3.c",sensor_name);
	sprintf(filename[13],"/data/misc/media/sensor_%s_raw_param_main.c",sensor_name);

	sprintf(tune_info,"static uint8_t s_%s_tune_info",sensor_name);
	sprintf(fixname,"static uint32_t s_%s_",sensor_name);
	sprintf(note_name,"static uint8_t s_%s_",sensor_name);
	sprintf(libuse_info,"static uint32_t s_%s_libuse_info",sensor_name);

	for (i = 0;i < 14;i++) {
		FILE *fp = fopen(filename[i],"r");
		if (NULL == fp) {
			printf("The param file %s is not exit!\n",filename[i]);
			continue;
		}
		ISP_LOGI("i = %d,filename = %s",i,filename[i]);
		while (!feof(fp))
		{
			if (fgets(line_buf, 512, fp) == NULL) {
				break;
			}
			if (strstr(line_buf,libuse_info) != NULL) {
				rtn = read_libuse_info(fp,sensor_raw_ptr);
				if (0x00 != rtn) {
					ISP_LOGE("libuse_info was error!");
					return rtn;
				}
				break;
			}
			if (strstr(line_buf,tune_info) != NULL) {
				rtn = read_tune_info(fp,sensor_raw_ptr->mode_ptr[i].addr,&sensor_raw_ptr->mode_ptr[i].len);
				if (0x00 != rtn) {
					ISP_LOGE("read_tune_info was error!");
					fclose(fp);
					goto exit;
				}
				continue;
			}
			if (strstr(line_buf,fixname) != NULL && strstr(line_buf,"ae_param_info") != NULL) {
				rtn = read_fix_ae_info(fp,&sensor_raw_ptr->fix_ptr[i]->ae);
				if(0x00 != rtn){
					ISP_LOGE("read_ae_info was error!");
					fclose(fp);
					goto exit;
				}
				continue;
			}
			if (strstr(line_buf,fixname) != NULL && strstr(line_buf,"awb_param_info") != NULL) {
				rtn = read_fix_awb_info(fp,&sensor_raw_ptr->fix_ptr[i]->awb);
				if (0x00 != rtn) {
					ISP_LOGE("read_awb_info was error!");
					fclose(fp);
					goto exit;
				}
				continue;
			}
			if (strstr(line_buf,fixname) != NULL && strstr(line_buf,"lnc_param_info") != NULL) {
				rtn = read_fix_lnc_info(fp,&sensor_raw_ptr->fix_ptr[i]->lnc);
				if (0x00 != rtn) {
					ISP_LOGE("read_lnc_info was error!");
					fclose(fp);
					goto exit;
				}
			}
			if (NULL != strstr(line_buf,note_name) && NULL != strstr(line_buf,"_tool_ui_input")) {
				break;
			}
		}
		fclose(fp);
	}

exit:
	if (NULL != line_buf) {
		free(line_buf);
	}
	for (i = 0; i < 13; i++) {
		if (NULL != filename[i]) {
			free(filename[i]);
		}
	}
	return rtn;
}

uint32_t isp_raw_para_update_from_file(SENSOR_INFO_T *sensor_info_ptr,SENSOR_ID_E sensor_id)
{
	int rtn = 0x00;

	const char *sensor_name = sensor_info_ptr->name;
	struct sensor_raw_info* sensor_raw_info_ptr = *(sensor_info_ptr->raw_info_ptr);
	struct sensor_ae_tab *ae_ptr_tmp = &sensor_raw_info_ptr->fix_ptr[0]->ae;

	int version = 0;
	unsigned char *filename = NULL;
	unsigned char filename0[80];
	unsigned char filename1[80];
	unsigned char filename2[80];

	sprintf(filename0,"/data/misc/media/sensor_%s_raw_param.c",sensor_name);
	sprintf(filename1,"/data/misc/media/sensor_%s_raw_param_v3.c",sensor_name);
	sprintf(filename2,"/data/misc/media/sensor_%s_raw_param_common.c",sensor_name);

	if (-1 != access(filename0,0)) {
		ISP_LOGI("%s is exit!\n",filename0);
		filename = filename0;
		version = 1;
	}
	if (NULL == filename) {
		if(-1 != access(filename1,0)) {
			ISP_LOGI("%s is exit!\n",filename1);
			filename = filename1;
			version = 2;
		}
		if(NULL == filename) {
			if(-1 != access(filename2,0)) {
				ISP_LOGI("%s is exit!\n",filename2);
				filename = filename2;
				version = 3;
			}
		}
	}
	if (NULL == filename && 0 == version) {
		ISP_LOGI("there is no param file!\n");
		return rtn;
	} else {
		ISP_LOGI("the param file is %s,version = %d",filename,version);
	}

	if ( ISP_PARAM_VERSION_OLD < sensor_raw_info_ptr->version_info->version_id) {
		rtn = update_param_v2(sensor_raw_info_ptr,sensor_name);
		if(0x00 != rtn) {
			ISP_LOGI("update param error!");
			return rtn;
		}
	} else {
		rtn = update_param_v1(sensor_raw_info_ptr,sensor_name);
		if(0x00 != rtn){
			ISP_LOGI("update param error!");
			return rtn;
		}
	}
	return rtn;
}
