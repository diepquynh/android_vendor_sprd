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

#define LOG_TAG "isp_pm"

#ifdef WIN32
#include <malloc.h>
#include <memory.h>
#include <string.h>
#endif
#include "isp_log.h"
#include "isp_com.h"
#include "isp_pm.h"
#include "isp_blocks_cfg.h"
#include "isp_pm_com_type.h"
#include "isp_otp_calibration.h"
#include "isp_drv.h"

#define ISP_PM_BUF_NUM     10
#define ISP_PM_MAGIC_FLAG  0xFFEE5511
#define UNUSED(param) (void)(param)

struct isp_pm_buffer {
	isp_u32 count;
	struct isp_data_info buf_array[ISP_PM_BUF_NUM];
};

struct isp_pm_blk_status {
	isp_u32 update_flag;/*0: not update 1: update*/
	isp_u32 block_id;/*need update block_id*/
};

struct isp_pm_mode_update_status {
	isp_u32 blk_num;/*need update total numbers*/
	struct isp_pm_blk_status pm_blk_status[ISP_TUNE_BLOCK_MAX];
};

struct isp_pm_tune_merge_param {
	isp_u32 mode_num;
	struct isp_pm_mode_param **tune_merge_para;
};

struct isp_pm_context {
	isp_u32 magic_flag;
	pthread_mutex_t pm_mutex;
	isp_u32 mode_num;
	isp_u32 cxt_num;
	struct isp_pm_buffer buffer;
	struct isp_pm_param_data temp_param_data[ISP_TUNE_BLOCK_MAX];
	struct isp_pm_param_data *tmp_param_data_ptr;
	struct isp_context *active_cxt_ptr;
	struct isp_context cxt_array[ISP_TUNE_MODE_MAX];/*preview,capture,video and so on*/
	struct isp_pm_mode_param *active_mode;
	struct isp_pm_mode_param *merged_mode_array[ISP_TUNE_MODE_MAX];/*new preview/capture/video mode param*/
	struct isp_pm_mode_param *tune_mode_array[ISP_TUNE_MODE_MAX];/*bakup isp tuning parameter, come frome sensor tuning file*/
	uint32_t param_source;
	isp_ctrl_context *isp_ctrl_cxt_handle;
};

struct isp_pm_set_param {
	isp_u32 id;
	isp_u32 cmd;
	void *data;
	isp_u32 size;
};

struct isp_pm_get_param {
	isp_u32 id;
	isp_u32 cmd;
};

struct isp_pm_get_result {
	void *data_ptr;
	isp_u32 size;
};

struct isp_pm_write_param {
	isp_u32 id;
	void *data;
	isp_u32 size;
};


static isp_s32 isp_pm_handle_check(isp_pm_handle_t handle)
{
	struct isp_pm_context *cxt_ptr = (struct isp_pm_context *)handle;

	if (PNULL == cxt_ptr) {
		ISP_LOGE("isp pm handle is null error.");
		return ISP_ERROR;
	}

	if (ISP_PM_MAGIC_FLAG != cxt_ptr->magic_flag) {
		ISP_LOGE("isp pm magic error.");
		return ISP_ERROR;
	}

	return ISP_SUCCESS;
}

static isp_pm_handle_t isp_pm_context_create(void)
{
	struct isp_pm_context *cxt_ptr = PNULL;

	cxt_ptr = (struct isp_pm_context *)malloc(sizeof(struct isp_pm_context));
	if (PNULL == cxt_ptr) {
		ISP_LOGE("no memory for context error");
		return cxt_ptr;
	}

	memset((void *)cxt_ptr, 0x00, sizeof(struct isp_pm_context));

	cxt_ptr->magic_flag = ISP_PM_MAGIC_FLAG;

	pthread_mutex_init(&cxt_ptr->pm_mutex, NULL);

	return (isp_pm_handle_t)cxt_ptr;
}

static void isp_pm_check_param(uint32_t id,uint32_t *update_flag)
{
	switch (id) {
	case ISP_BLK_PRE_WAVELET_V1:
	case ISP_BLK_BPC_V1:
	case ISP_BLK_BL_NR_V1:
	case ISP_BLK_GRGB_V1:
	case ISP_BLK_NLM:
	case ISP_BLK_CFA_V1:
	case ISP_BLK_RGB_PRECDN:
	case ISP_BLK_YUV_PRECDN:
	case ISP_BLK_PREF_V1:
	case ISP_BLK_UV_CDN:
	case ISP_BLK_EDGE_V1:
	case ISP_BLK_UV_POSTCDN:
	case ISP_BLK_IIRCNR_IIR:
	case ISP_BLK_IIRCNR_YRANDOM:
	case ISP_BLK_UVDIV_V1:
	case ISP_BLK_YIQ_AFM:

	case ISP_BLK_BINNING4AWB_V1:
	{
		*update_flag = ISP_PARAM_FROM_TOOL;
		break;
	}
	default:
		break;
	}
}


isp_u16 multi_mode[ISP_BLK_TYPE_MAX][MAX_MODE_NUM] = {{0}};
static isp_s32 isp_pm_context_init(isp_pm_handle_t handle)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i = 0, blk_num = 0, id = 0, offset = 0;
	uint32_t update_flag = 0;
	void *blk_ptr = PNULL;
	void *param_data_ptr = PNULL;
	intptr_t isp_cxt_start_addr = 0;
	struct isp_block_operations *ops = PNULL;
	struct isp_block_cfg *blk_cfg_ptr = PNULL;
	struct isp_context *isp_cxt_ptr = PNULL;
	struct isp_pm_context *pm_cxt_ptr = PNULL;
	struct isp_pm_block_header *blk_header_array = PNULL;
	struct isp_pm_block_header *blk_header_ptr = PNULL;
	struct isp_pm_mode_param *mode_param_ptr = PNULL;

	pm_cxt_ptr = (struct isp_pm_context *)handle;
	isp_cxt_ptr = (struct isp_context *)pm_cxt_ptr->active_cxt_ptr;
	mode_param_ptr = (struct isp_pm_mode_param *)pm_cxt_ptr->active_mode;
	blk_header_array =(struct isp_pm_block_header *)mode_param_ptr->header;

	blk_num = mode_param_ptr->block_num;
	for (i = 0; i < blk_num; i++) {
		id = blk_header_array[i].block_id;
		blk_cfg_ptr = isp_pm_get_block_cfg(id);
		blk_header_ptr = &blk_header_array[i];
		if (pm_cxt_ptr->param_source) {
			isp_pm_check_param(id,&update_flag);
			if (0 != update_flag) {
				update_flag = 0;
				continue;
			}
		}
		if ((PNULL != blk_cfg_ptr) && (PNULL != blk_header_ptr)) {
			if (blk_cfg_ptr->ops) {
				ops = blk_cfg_ptr->ops;
				if (ops->init) {
					if (blk_header_ptr->size > 0) {
						isp_cxt_start_addr = (intptr_t)isp_cxt_ptr;
						offset = blk_cfg_ptr->offset;
						blk_ptr = (void *)(isp_cxt_start_addr + offset);
						param_data_ptr = (void *)blk_header_ptr->absolute_addr;
						if ((PNULL == blk_ptr) || (PNULL == param_data_ptr)
							|| (PNULL == blk_header_ptr)
							|| (PNULL == mode_param_ptr)) {
							ISP_LOGE ("param is null error: blk_addr:%p, param:%p, header:%p, resolution:%p",
								blk_ptr, param_data_ptr, blk_header_ptr, &mode_param_ptr->resolution);
							rtn = ISP_ERROR;
						} else {
							ops->init(blk_ptr, param_data_ptr, blk_header_ptr, &mode_param_ptr->resolution);
						}
					} else {
						ISP_LOGI("param size is warning: size:%d, id:0x%x, i:%d\n", blk_header_ptr->size, id, i);
					}
				}
			}
		}
	}

	isp_cxt_ptr->is_validate = ISP_ONE;

	return rtn;
}

static isp_s32 isp_pm_context_update(isp_pm_handle_t handle, struct isp_pm_mode_param *org_mode_param)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i = 0, j =0, blk_num = 0, id = 0, offset = 0;
	void *blk_ptr = PNULL;
	void *param_data_ptr = PNULL;
	intptr_t isp_cxt_start_addr = 0;
	struct isp_block_operations *ops = PNULL;
	struct isp_block_cfg *blk_cfg_ptr = PNULL;
	struct isp_context *isp_cxt_ptr = PNULL;
	struct isp_pm_context *pm_cxt_ptr = PNULL;
	struct isp_pm_block_header *blk_header_array = PNULL;
	struct isp_pm_block_header *blk_header_ptr = PNULL;
	struct isp_pm_mode_param *mode_param_ptr = PNULL;

	pm_cxt_ptr = (struct isp_pm_context *)handle;
	isp_cxt_ptr = (struct isp_context *)pm_cxt_ptr->active_cxt_ptr;
	mode_param_ptr = (struct isp_pm_mode_param *)pm_cxt_ptr->active_mode;
	blk_header_array =(struct isp_pm_block_header *)mode_param_ptr->header;

	blk_num = mode_param_ptr->block_num;
	for (i = 0; i < blk_num; i++) {
		id = blk_header_array[i].block_id;
		blk_cfg_ptr = isp_pm_get_block_cfg(id);
		blk_header_ptr = &blk_header_array[i];
		if ((PNULL != blk_cfg_ptr) && (PNULL != blk_header_ptr)) {
			if (blk_cfg_ptr->ops) {
				ops = blk_cfg_ptr->ops;
				if (ops->init) {
					if (blk_header_ptr->size > 0) {
						isp_cxt_start_addr = (intptr_t)isp_cxt_ptr;
						offset = blk_cfg_ptr->offset;
						blk_ptr = (void *)(isp_cxt_start_addr + offset);
						param_data_ptr = (void *)blk_header_ptr->absolute_addr;
						if ((PNULL == blk_ptr) || (PNULL == param_data_ptr)
							|| (PNULL == blk_header_ptr)
							|| (PNULL == mode_param_ptr)) {
							ISP_LOGE ("param is null error: blk_addr:%p, param:%p, header:%p, resolution:%p",
								blk_ptr, param_data_ptr, blk_header_ptr, &mode_param_ptr->resolution);
							rtn = ISP_ERROR;
						} else {
							for (j = 0; j < org_mode_param->block_num; j++) {
								if (org_mode_param->header[j].block_id == blk_header_ptr->block_id) {
									break;
								}
							}
							if (j < org_mode_param->block_num) {
								if (blk_header_ptr->source_flag != org_mode_param->header[j].source_flag) {
									ops->init(blk_ptr, param_data_ptr, blk_header_ptr, &mode_param_ptr->resolution);
								}
							} else {
								ops->init(blk_ptr, param_data_ptr, blk_header_ptr, &mode_param_ptr->resolution);
							}
						}
					} else {
						ISP_LOGI("param size is warning: size:%d, id:0x%x, i:%d\n", blk_header_ptr->size, id, i);
					}
				}
			}
		}
	}

	isp_cxt_ptr->is_validate = ISP_ONE;

	return rtn;
}


static isp_s32 isp_pm_context_deinit(isp_pm_handle_t handle)
{
	isp_s32 rtn = ISP_SUCCESS;
	void *blk_ptr = PNULL;
	void *param_data_ptr = PNULL;
	isp_u32 i = 0, j = 0, offset = 0;
	isp_u32 blk_num = 0, id = 0;
	intptr_t isp_cxt_start_addr = 0;
	struct isp_block_operations *ops = PNULL;
	struct isp_block_cfg *blk_cfg_ptr = PNULL;
	struct isp_context *isp_cxt_ptr = PNULL;
	struct isp_pm_context *pm_cxt_ptr = PNULL;
	struct isp_pm_block_header *blk_header_array = PNULL;
	struct isp_pm_block_header *blk_header_ptr = PNULL;
	struct isp_pm_mode_param *mode_param_ptr = PNULL;

	pm_cxt_ptr = (struct isp_pm_context *)handle;
	isp_cxt_ptr = (struct isp_context *)pm_cxt_ptr->active_cxt_ptr;
	isp_cxt_start_addr = (intptr_t)isp_cxt_ptr;
	mode_param_ptr = (struct isp_pm_mode_param *)pm_cxt_ptr->active_mode;
	blk_header_array =(struct isp_pm_block_header *)mode_param_ptr->header;
	blk_num = mode_param_ptr->block_num;
	for (i = 0; i < blk_num; i++) {
		id = blk_header_array[i].block_id;
		blk_cfg_ptr = isp_pm_get_block_cfg(id);
		blk_header_ptr = &blk_header_array[i];
		if ((PNULL != blk_cfg_ptr) && (PNULL != blk_header_ptr)) {
			if (blk_cfg_ptr->ops) {
				ops = blk_cfg_ptr->ops;
				if (ops->deinit) {
					offset = blk_cfg_ptr->offset;
					blk_ptr = (void*)(isp_cxt_start_addr + offset);
					if (PNULL != blk_ptr) {
						ops->deinit(blk_ptr);
					} else {
						ISP_LOGE("blk_addr is null erro");
					}
				}
			}
		}
	}

	for (j = 0; j < ISP_TUNE_MODE_MAX; j++) {
		if (PNULL == pm_cxt_ptr->tune_mode_array[j]) {
			continue;
		}
		mode_param_ptr = (struct isp_pm_mode_param *)pm_cxt_ptr->tune_mode_array[j];
		blk_header_array =(struct isp_pm_block_header *)mode_param_ptr->header;
		isp_cxt_ptr = (struct isp_context *)&pm_cxt_ptr->cxt_array[j];
		isp_cxt_start_addr = (intptr_t)isp_cxt_ptr;
		blk_num = mode_param_ptr->block_num;
		for (i = 0; i < blk_num; i++) {
			id = blk_header_array[i].block_id;
			blk_cfg_ptr = isp_pm_get_block_cfg(id);
			blk_header_ptr = &blk_header_array[i];
			if ((PNULL != blk_cfg_ptr) && (PNULL != blk_header_ptr)) {
				if (blk_cfg_ptr->ops) {
					ops = blk_cfg_ptr->ops;
					if (ops->deinit) {
						offset = blk_cfg_ptr->offset;
						blk_ptr = (void*)(isp_cxt_start_addr + offset);
						if (PNULL != blk_ptr) {
							ops->deinit(blk_ptr);
						} else {
							ISP_LOGE("blk_addr is null erro");
						}
					}
				}
			}
		}
	}

	return rtn;
}

static struct isp_pm_block_header *isp_pm_get_block_header(struct isp_pm_mode_param *pm_mode_ptr, isp_u32 id, int32_t *index)
{
	isp_u32 i = 0, blk_num = 0;
	struct isp_pm_block_header *header_ptr = PNULL;
	struct isp_pm_block_header *blk_header = (struct isp_pm_block_header *)pm_mode_ptr->header;

	blk_num = pm_mode_ptr->block_num;

	*index = ISP_TUNE_BLOCK_MAX;

	for (i = 0; i < blk_num; i++) {
		if (id == blk_header[i].block_id) {
			break;
		}
	}

	if (i < blk_num) {
		header_ptr = (struct isp_pm_block_header *)&blk_header[i];
		*index = i;
	}

	return header_ptr;
}

static struct isp_pm_mode_param *isp_pm_get_mode(struct isp_pm_tune_merge_param tune_merge_para, isp_u32 id)
{
	isp_u32 i = 0, mode_num = 0;
	struct isp_pm_mode_param *mode_param_ptr = PNULL;

	mode_num = tune_merge_para.mode_num;
	for (i = 0; i < mode_num; i++) {
		if (PNULL == tune_merge_para.tune_merge_para[i]) {
			continue;
		}
		if (id == tune_merge_para.tune_merge_para[i]->mode_id) {
			break;
		}
	}

	if (i < mode_num) {
		mode_param_ptr = (struct isp_pm_mode_param *)(tune_merge_para.tune_merge_para[i]);
	}

	return mode_param_ptr;
}

static struct isp_context *isp_pm_get_context(struct isp_pm_context *pm_cxt_ptr, isp_u32 id)
{
	isp_u32 i = 0, num = 0;
	struct isp_context *isp_cxt_ptr = PNULL;

#if 0
	num = pm_cxt_ptr->cxt_num;

	for (i = 0; i < num; i++) {
		if (id == pm_cxt_ptr->cxt_array[i].mode_id) {
			break;
		}
	}

	if (i < num) {
		isp_cxt_ptr = (struct isp_context *)&pm_cxt_ptr->cxt_array[i];
	}
#else
	isp_cxt_ptr = (struct isp_context *)&pm_cxt_ptr->cxt_array[ISP_MODE_ID_PRV_0];
	isp_cxt_ptr->mode_id = id;

#endif
	return isp_cxt_ptr;
}

static isp_s32 isp_pm_active_mode_init(isp_pm_handle_t handle, isp_u32 mode_id)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;
	struct isp_pm_tune_merge_param merged_mode_param;

	memset((void *)&merged_mode_param, 0x00, sizeof(struct isp_pm_tune_merge_param));
	merged_mode_param.tune_merge_para = pm_cxt_ptr->merged_mode_array;
	merged_mode_param.mode_num = pm_cxt_ptr->mode_num;

	pm_cxt_ptr->active_mode = isp_pm_get_mode(merged_mode_param, mode_id);
	if (PNULL == pm_cxt_ptr->active_mode) {
		ISP_LOGE("get active mode error");
		rtn = ISP_ERROR;
		return rtn;
	}

	pm_cxt_ptr->active_cxt_ptr = isp_pm_get_context(pm_cxt_ptr, mode_id);
	if (PNULL == pm_cxt_ptr->active_cxt_ptr) {
		ISP_LOGE("get active cxt error");
		rtn = ISP_ERROR;
		return rtn;
	}

	return rtn;
}

static void isp_pm_mode_list_deinit(isp_pm_handle_t handle)
{
	isp_u32 i = 0;
	struct isp_pm_context *cxt_ptr = (struct isp_pm_context *)handle;

	for (i = 0; i < ISP_TUNE_MODE_MAX; i++) {
		if (cxt_ptr->tune_mode_array[i]) {
			free(cxt_ptr->tune_mode_array[i]);
			cxt_ptr->tune_mode_array[i] = PNULL;
		}
		if (cxt_ptr->merged_mode_array[i]) {
			free(cxt_ptr->merged_mode_array[i]);
			cxt_ptr->merged_mode_array[i] = PNULL;
		}
	}

	if (PNULL != cxt_ptr->tmp_param_data_ptr) {
		free(cxt_ptr->tmp_param_data_ptr);
		cxt_ptr->tmp_param_data_ptr = PNULL;
	}
}

isp_s8 nr_param_name[ISP_BLK_TYPE_MAX][16] = {
	"pwd",
	"bpc",
	"bdn",
	"grgb",
	"nlm",
	"vst",
	"ivst",
	"flat_offset",
	"cfae",
	"rgb_precdn",
	"yuv_precdn",
	"prfy",
	"yuv_cdn",
	"edge",
	"yuv_postcdn",
	"iircnr",
	"iir_yrandom",
	"cce_uv",
	"y_afm"
};
isp_s8 nr_mode_name[MAX_MODE_NUM][8] = {
	"common",
	"prv_0",
	"prv_1",
	"prv_2",
	"prv_3",
	"cap_0",
	"cap_1",
	"cap_2",
	"cap_3",
	"video_0",
	"video_1",
	"video_2",
	"video_3",
};
isp_s8 nr_scene_name[MAX_SCENEMODE_NUM][16] = {
	"normal",
	"night",
	"sport",
	"portrait",
	"landspace",
	"panorama",
};
#if 0
isp_u32 nr_param_size[ISP_BLK_TYPE_MAX] = {
	sizeof( struct sensor_pwd_level) * SENSOR_SMART_LEVEL_NUM,
	sizeof( struct sensor_bpc_level) * SENSOR_SMART_LEVEL_NUM,
	sizeof( struct sensor_bdn_level) * SENSOR_SMART_LEVEL_NUM,
	sizeof( struct sensor_grgb_v1_level) * SENSOR_SMART_LEVEL_NUM,
	sizeof( struct sensor_nlm_level) * SENSOR_SMART_LEVEL_NUM,
	sizeof( struct sensor_vst_level) * SENSOR_SMART_LEVEL_NUM,
	sizeof( struct sensor_ivst_level) * SENSOR_SMART_LEVEL_NUM,
	sizeof( struct sensor_flat_offset_level) * SENSOR_SMART_LEVEL_NUM,
	sizeof( struct sensor_cfae_level) * SENSOR_SMART_LEVEL_NUM,
	sizeof( struct sensor_rgb_precdn_level) * SENSOR_SMART_LEVEL_NUM,
	sizeof( struct sensor_yuv_precdn_level) * SENSOR_SMART_LEVEL_NUM,
	sizeof( struct sensor_prfy_level) * SENSOR_SMART_LEVEL_NUM,
	sizeof( struct sensor_uv_cdn_level) * SENSOR_SMART_LEVEL_NUM,
	sizeof( struct sensor_ee_level) * SENSOR_SMART_LEVEL_NUM,
	sizeof( struct sensor_uv_postcdn_level) * SENSOR_SMART_LEVEL_NUM,
	sizeof( struct sensor_iircnr_level) * SENSOR_SMART_LEVEL_NUM,
	sizeof( struct sensor_iircnr_yrandom_level) * SENSOR_SMART_LEVEL_NUM,
	sizeof( struct sensor_cce_uvdiv_level) * SENSOR_SMART_LEVEL_NUM,
	sizeof( struct sensor_y_afm_level) * SENSOR_SMART_LEVEL_NUM,
};
#endif
static isp_u32 isp_calc_nr_addr_offset(isp_u32 mode_flag, isp_u16 * one_multi_mode_ptr, isp_u32 *basic_offset_units_ptr)
{

	isp_u32 rtn = ISP_SUCCESS;

	isp_u32  basic_offset_units = 0;
	isp_u32 i = 0, j = 0;

	if(mode_flag > (MAX_MODE_NUM - 1))
		return ISP_ERROR;
	if(!one_multi_mode_ptr)
		return ISP_ERROR;
	for( i = 0; i < mode_flag; i++) {
		//ISP_LOGI("nr_mode_flag[%d]=%x\n",i,one_multi_mode_ptr[i]);
		if(one_multi_mode_ptr[i]) {
			for( j = 0; j < MAX_SCENEMODE_NUM; j++ ) {
				basic_offset_units += (one_multi_mode_ptr[i] >> j) & 0x01;
			}
		}
	}
	*basic_offset_units_ptr = basic_offset_units;

	return rtn;
}
struct isp_nr_param_update_info {
	isp_s8 *sensor_name;
	isp_u32 sensor_mode;
	isp_u32 param_type;
	isp_uint *nr_param_ptr;
	isp_u32 size_of_per_unit;
	isp_uint *multi_nr_map_ptr;
};
static isp_s32 isp_nr_param_update(struct isp_nr_param_update_info * nr_param_update_ptr )
{
	isp_u32 rtn = ISP_SUCCESS;
	isp_u32 basic_offset_units = 0;
	isp_u16 * one_multi_mode_ptr = PNULL;
	isp_u8 * nr_param_ptr = PNULL;
	isp_u8 * nr_param_start_ptr = PNULL;
	isp_u32 size_of_per_unit = 0;

	isp_u32 scene_number = 0;
	isp_u32 sensor_mode = 0;
	isp_s8 *sensor_name = PNULL;
	isp_u32 param_type =0;
	FILE* fp = NULL;
	isp_s8 filename[80];

	if( nr_param_update_ptr->sensor_mode > (MAX_MODE_NUM - 1))
		return ISP_ERROR;
	if(nr_param_update_ptr->param_type > (ISP_BLK_TYPE_MAX - 1))
		return ISP_ERROR;

	one_multi_mode_ptr = (isp_u16 *)nr_param_update_ptr->multi_nr_map_ptr;
	nr_param_ptr = (isp_u8 *)nr_param_update_ptr->nr_param_ptr;
	size_of_per_unit = nr_param_update_ptr->size_of_per_unit;
	param_type =nr_param_update_ptr->param_type;
	sensor_mode =nr_param_update_ptr->sensor_mode;
	sensor_name = nr_param_update_ptr->sensor_name;

	rtn = isp_calc_nr_addr_offset(sensor_mode, one_multi_mode_ptr, &basic_offset_units);

	nr_param_ptr += basic_offset_units * size_of_per_unit;

	/* calc current block */
	if(one_multi_mode_ptr[sensor_mode]) {
		for( scene_number = 0; scene_number <  MAX_SCENEMODE_NUM; scene_number++ ) {

			if( (one_multi_mode_ptr[sensor_mode] >> scene_number) & 0x01 ) {
				if((sensor_mode == ISP_MODE_ID_COMMON) && (scene_number ==  ISP_SCENEMODE_AUTO)) {
					sprintf(filename, "/data/misc/media/%s_%s_param.bin", sensor_name, nr_param_name[param_type]);
				} else {
					sprintf(filename, "/data/misc/media/%s_%s_%s_%s_param.bin", sensor_name, nr_mode_name[sensor_mode],
														nr_scene_name[scene_number], nr_param_name[param_type]);
				}

				if(0 != access(filename,R_OK)) {
					ISP_LOGE("param access %s failure",filename);
				} else {
					ISP_LOGI("param access %s exist",filename);
					if( NULL!=(fp=fopen(filename,"rb")) ) {
						fread((void*)nr_param_ptr,1, size_of_per_unit, fp);
						fclose(fp);
					}else{
						ISP_LOGV("param open %s failure",filename);\
					}
				}
				nr_param_ptr += size_of_per_unit;
			}
		}
	}
	return rtn;
}
static isp_s32 isp_pm_mode_list_init(isp_pm_handle_t handle, struct isp_pm_init_input *input)
{
	isp_u32 rtn = ISP_SUCCESS;
	isp_u32 max_num = 0;
	isp_u32 data_area_size = 0;
	isp_u32 i = 0, j = 0, size = 0;
	isp_u8 *src_data_ptr = PNULL;
	isp_u8 *dst_data_ptr = PNULL;
	struct isp_mode_param *src_mod_ptr = PNULL;
	struct isp_pm_mode_param *dst_mod_ptr = PNULL;
	struct isp_block_header *src_header = PNULL;
	struct isp_pm_block_header *dst_header = PNULL;
	struct isp_pm_context *cxt_ptr = (struct isp_pm_context *)handle;
	isp_ctrl_context *isp_handle;
	if (NULL == cxt_ptr->isp_ctrl_cxt_handle)
		cxt_ptr->isp_ctrl_cxt_handle = input->isp_ctrl_cxt_handle;
	isp_handle = cxt_ptr->isp_ctrl_cxt_handle;
	cxt_ptr->mode_num = input->num;
	struct isp_nr_param_update_info nr_param_update_info;

	memset(multi_mode, 0x00, ISP_BLK_TYPE_MAX * MAX_MODE_NUM * sizeof(multi_mode[0][0]));
	isp_uint * temp_param[ISP_BLK_TYPE_MAX] = { PNULL };

	if( strlen( input->sensor_name )) {
		isp_handle->multi_nr_flag = SENSOR_MULTI_MODE_FLAG;
		strcpy(isp_handle->sensor_name, input->sensor_name);
	} else {
		isp_handle->multi_nr_flag = 0x0;
		memset(isp_handle->sensor_name, 0, sizeof(isp_handle->sensor_name));
	}
	nr_param_update_info.sensor_name = isp_handle->sensor_name;
	for (i = 0; i < input->num; i++) {
		nr_param_update_info.sensor_mode = i;
		src_mod_ptr = (struct isp_mode_param *)input->tuning_data[i].data_ptr;
		if (PNULL == src_mod_ptr)
			continue;

		data_area_size = src_mod_ptr->size - sizeof(struct isp_mode_param);
		size = data_area_size + sizeof(struct isp_pm_mode_param);
		if (PNULL != cxt_ptr->tune_mode_array[i]) {
			free(cxt_ptr->tune_mode_array[i]);
			cxt_ptr->tune_mode_array[i] = PNULL;
		}
		cxt_ptr->tune_mode_array[i] = (struct isp_pm_mode_param *)malloc(size);
		if (PNULL == cxt_ptr->tune_mode_array[i]) {
			ISP_LOGE("tune mode malloc error, i=%d", i);
			rtn = ISP_ERROR;
			goto _mode_list_init_error_exit;
		}
		memset((void *)cxt_ptr->tune_mode_array[i], 0x00, size);

		dst_mod_ptr = (struct isp_pm_mode_param *)cxt_ptr->tune_mode_array[i];
		src_header = (struct isp_block_header *)src_mod_ptr->block_header;
		dst_header = (struct isp_pm_block_header *)dst_mod_ptr->header;
		for (j = 0; j < src_mod_ptr->block_num; j++) {
			dst_header[j].is_update = 0;
			dst_header[j].source_flag = i;/*data source is common,preview,capture or video*/
			dst_header[j].bypass = src_header[j].bypass;
			dst_header[j].block_id = src_header[j].block_id;
			dst_header[j].param_id = src_header[j].param_id;
			dst_header[j].version_id = src_header[j].version_id;
			dst_header[j].size = src_header[j].size;

			size = src_header[j].offset - sizeof(struct isp_mode_param);
			size = size + sizeof(struct isp_pm_mode_param);
			src_data_ptr = (isp_u8 *)((intptr_t)src_mod_ptr + src_header[j].offset);
			dst_data_ptr = (isp_u8 *)((intptr_t)dst_mod_ptr + size);
			dst_header[j].absolute_addr = (void *)dst_data_ptr;
			if(isp_handle->multi_nr_flag == SENSOR_MULTI_MODE_FLAG) {
				switch (src_header[j].block_id) {
				case ISP_BLK_PRE_WAVELET_V1: {
						struct sensor_pwd_param *block = (struct sensor_pwd_param *)src_data_ptr;
						multi_mode[ISP_BLK_PRE_WAVELET_TYPE][i] = block->nr_scene_flag;
						block->multi_nr_map_ptr = (isp_uint *)&multi_mode[ISP_BLK_PRE_WAVELET_TYPE][0];
						if(ISP_MODE_ID_COMMON == i) {
							temp_param[ISP_BLK_PRE_WAVELET_TYPE] = block->param_ptr;
						} else {
							block->param_ptr = temp_param[ISP_BLK_PRE_WAVELET_TYPE];
						}
						nr_param_update_info.param_type= ISP_BLK_PRE_WAVELET_TYPE;
						nr_param_update_info.nr_param_ptr = block->param_ptr;
						nr_param_update_info.size_of_per_unit= sizeof(struct sensor_pwd_level) * SENSOR_SMART_LEVEL_NUM;
						nr_param_update_info.multi_nr_map_ptr = block->multi_nr_map_ptr;
						isp_nr_param_update(&nr_param_update_info);
					}
					break;
				case ISP_BLK_BPC_V1: {
						struct sensor_bpc_param_v1 *block = (struct sensor_bpc_param_v1 *)src_data_ptr;
						multi_mode[ISP_BLK_BPC_TYPE][i] = block->nr_scene_flag;
						block->multi_nr_map_ptr = (isp_uint *)&multi_mode[ISP_BLK_BPC_TYPE][0];
						if(ISP_MODE_ID_COMMON == i) {
							temp_param[ISP_BLK_BPC_TYPE] = block->param_ptr;
						} else {
							block->param_ptr = temp_param[ISP_BLK_BPC_TYPE];
						}
						nr_param_update_info.param_type= ISP_BLK_BPC_TYPE;
						nr_param_update_info.nr_param_ptr = block->param_ptr;
						nr_param_update_info.size_of_per_unit= sizeof(struct sensor_bpc_level) * SENSOR_SMART_LEVEL_NUM;
						nr_param_update_info.multi_nr_map_ptr = block->multi_nr_map_ptr;
						isp_nr_param_update(&nr_param_update_info);
					}
					break;
				case ISP_BLK_BL_NR_V1: {
						struct sensor_bdn_param *block = (struct sensor_bdn_param *)src_data_ptr;
						multi_mode[ISP_BLK_BL_NR_TYPE][i] = block->nr_scene_flag;
						block->multi_nr_map_ptr = (isp_uint *)&multi_mode[ISP_BLK_BL_NR_TYPE][0];
						if(ISP_MODE_ID_COMMON == i) {
							temp_param[ISP_BLK_BL_NR_TYPE] = block->param_ptr;
						} else {
							block->param_ptr = temp_param[ISP_BLK_BL_NR_TYPE];
						}
						nr_param_update_info.param_type= ISP_BLK_BL_NR_TYPE;
						nr_param_update_info.nr_param_ptr = block->param_ptr;
						nr_param_update_info.size_of_per_unit= sizeof(struct sensor_bdn_level) * SENSOR_SMART_LEVEL_NUM;
						nr_param_update_info.multi_nr_map_ptr = block->multi_nr_map_ptr;
						isp_nr_param_update(&nr_param_update_info);
					}
					break;
				case ISP_BLK_GRGB_V1: {
						struct sensor_grgb_v1_param *block = (struct sensor_grgb_v1_param *)src_data_ptr;
						multi_mode[ISP_BLK_GRGB_TYPE][i] = block->nr_scene_flag;
						block->multi_nr_map_ptr = (isp_uint *)&multi_mode[ISP_BLK_GRGB_TYPE][0];
						if(ISP_MODE_ID_COMMON == i) {
							temp_param[ISP_BLK_GRGB_TYPE] = block->param_ptr;
						} else {
							block->param_ptr = temp_param[ISP_BLK_GRGB_TYPE];
						}
						nr_param_update_info.param_type= ISP_BLK_GRGB_TYPE;
						nr_param_update_info.nr_param_ptr = block->param_ptr;
						nr_param_update_info.size_of_per_unit= sizeof(struct sensor_grgb_v1_level) * SENSOR_SMART_LEVEL_NUM;
						nr_param_update_info.multi_nr_map_ptr = block->multi_nr_map_ptr;
						isp_nr_param_update(&nr_param_update_info);
					}
					break;
				case ISP_BLK_NLM: {
						struct sensor_nlm_param *block = (struct sensor_nlm_param *)src_data_ptr;
						multi_mode[ISP_BLK_NLM_TYPE][i] = block->nr_scene_flag;
						//multi_mode[ISP_BLK_NLM_VST_TYPE][i] = block->nr_scene_flag;
						//multi_mode[ISP_BLK_NLM_IVST_TYPE][i] = block->nr_scene_flag;
						//multi_mode[ISP_BLK_NLM_FLAT_OFFSET_TYPE][i] = block->nr_scene_flag;
						block->multi_nr_map_ptr = (isp_uint *)&multi_mode[ISP_BLK_NLM_TYPE][0];
						if(ISP_MODE_ID_COMMON == i) {
							temp_param[ISP_BLK_NLM_TYPE] = block->param_nlm_ptr;
							temp_param[ISP_BLK_NLM_VST_TYPE] = block->param_vst_ptr;
							temp_param[ISP_BLK_NLM_IVST_TYPE] = block->param_ivst_ptr;
							temp_param[ISP_BLK_NLM_FLAT_OFFSET_TYPE] = block->param_flat_offset_ptr;
						} else {
							block->param_nlm_ptr = temp_param[ISP_BLK_NLM_TYPE];
							block->param_vst_ptr = temp_param[ISP_BLK_NLM_VST_TYPE];
							block->param_ivst_ptr = temp_param[ISP_BLK_NLM_IVST_TYPE];
							block->param_flat_offset_ptr = temp_param[ISP_BLK_NLM_FLAT_OFFSET_TYPE];
						}
						nr_param_update_info.param_type= ISP_BLK_NLM_TYPE;
						nr_param_update_info.nr_param_ptr = block->param_nlm_ptr;
						nr_param_update_info.size_of_per_unit= sizeof(struct sensor_nlm_level) * SENSOR_SMART_LEVEL_NUM;
						nr_param_update_info.multi_nr_map_ptr = block->multi_nr_map_ptr;
						isp_nr_param_update(&nr_param_update_info);
						nr_param_update_info.param_type= ISP_BLK_NLM_VST_TYPE;
						nr_param_update_info.nr_param_ptr = block->param_vst_ptr;
						nr_param_update_info.size_of_per_unit= sizeof(struct sensor_vst_level) * SENSOR_SMART_LEVEL_NUM;
						nr_param_update_info.multi_nr_map_ptr = block->multi_nr_map_ptr;
						isp_nr_param_update(&nr_param_update_info);
						nr_param_update_info.param_type= ISP_BLK_NLM_IVST_TYPE;
						nr_param_update_info.nr_param_ptr = block->param_ivst_ptr;
						nr_param_update_info.size_of_per_unit= sizeof(struct sensor_ivst_level) * SENSOR_SMART_LEVEL_NUM;
						nr_param_update_info.multi_nr_map_ptr = block->multi_nr_map_ptr;
						isp_nr_param_update(&nr_param_update_info);
						nr_param_update_info.param_type= ISP_BLK_NLM_FLAT_OFFSET_TYPE;
						nr_param_update_info.nr_param_ptr = block->param_flat_offset_ptr;
						nr_param_update_info.size_of_per_unit= sizeof(struct sensor_flat_offset_level) * SENSOR_SMART_LEVEL_NUM;
						nr_param_update_info.multi_nr_map_ptr = block->multi_nr_map_ptr;
						isp_nr_param_update(&nr_param_update_info);
					}
					break;
				case ISP_BLK_CFA_V1: {
						struct sensor_cfa_param_v1 *block = (struct sensor_cfa_param_v1*)src_data_ptr;
						multi_mode[ISP_BLK_CFA_TYPE][i] = block->nr_scene_flag;
						block->multi_nr_map_ptr = (isp_uint *)&multi_mode[ISP_BLK_CFA_TYPE][0];
						if(ISP_MODE_ID_COMMON == i) {
							temp_param[ISP_BLK_CFA_TYPE] = block->param_ptr;
						} else {
							block->param_ptr = temp_param[ISP_BLK_CFA_TYPE];
						}
						nr_param_update_info.param_type= ISP_BLK_CFA_V1;
						nr_param_update_info.nr_param_ptr = block->param_ptr;
						nr_param_update_info.size_of_per_unit= sizeof(struct sensor_cfae_level) * SENSOR_SMART_LEVEL_NUM;
						nr_param_update_info.multi_nr_map_ptr = block->multi_nr_map_ptr;
						isp_nr_param_update(&nr_param_update_info);
					}
					break;
				case ISP_BLK_RGB_PRECDN: {
						struct sensor_rgb_precdn_param *block = (struct sensor_rgb_precdn_param*)src_data_ptr;
						multi_mode[ISP_BLK_RGB_PRECDN_TYPE][i] = block->nr_scene_flag;
						block->multi_nr_map_ptr = (isp_uint *)&multi_mode[ISP_BLK_RGB_PRECDN_TYPE][0];
						if(ISP_MODE_ID_COMMON == i) {
							temp_param[ISP_BLK_RGB_PRECDN_TYPE] = block->param_ptr;
						} else {

							block->param_ptr = temp_param[ISP_BLK_RGB_PRECDN_TYPE];
						}
						nr_param_update_info.param_type= ISP_BLK_RGB_PRECDN_TYPE;
						nr_param_update_info.nr_param_ptr = block->param_ptr;
						nr_param_update_info.size_of_per_unit= sizeof(struct sensor_rgb_precdn_level) * SENSOR_SMART_LEVEL_NUM;
						nr_param_update_info.multi_nr_map_ptr = block->multi_nr_map_ptr;
						isp_nr_param_update(&nr_param_update_info);
					}
					break;
				case ISP_BLK_YUV_PRECDN: {
						struct sensor_yuv_precdn_param *block = (struct sensor_yuv_precdn_param*)src_data_ptr;
						multi_mode[ISP_BLK_YUV_PRECDN_TYPE][i] = block->nr_scene_flag;
						block->multi_nr_map_ptr = (isp_uint *)&multi_mode[ISP_BLK_YUV_PRECDN_TYPE][0];
						if(ISP_MODE_ID_COMMON == i) {
							temp_param[ISP_BLK_YUV_PRECDN_TYPE] = block->param_ptr;
						} else {
							block->param_ptr = temp_param[ISP_BLK_YUV_PRECDN_TYPE];
						}
						nr_param_update_info.param_type= ISP_BLK_YUV_PRECDN_TYPE;
						nr_param_update_info.nr_param_ptr = block->param_ptr;
						nr_param_update_info.size_of_per_unit= sizeof(struct sensor_yuv_precdn_level) * SENSOR_SMART_LEVEL_NUM;
						nr_param_update_info.multi_nr_map_ptr = block->multi_nr_map_ptr;
						isp_nr_param_update(&nr_param_update_info);
					}
					break;
				case ISP_BLK_PREF_V1: {
						struct sensor_prfy_param *block = (struct sensor_prfy_param*)src_data_ptr;
						multi_mode[ISP_BLK_PREF_TYPE][i] = block->nr_scene_flag;
						block->multi_nr_map_ptr = (isp_uint *)&multi_mode[ISP_BLK_PREF_TYPE][0];
						if(ISP_MODE_ID_COMMON == i) {
							temp_param[ISP_BLK_PREF_TYPE] = block->param_ptr;
						} else {
							block->param_ptr = temp_param[ISP_BLK_PREF_TYPE];
						}
						nr_param_update_info.param_type= ISP_BLK_PREF_TYPE;
						nr_param_update_info.nr_param_ptr = block->param_ptr;
						nr_param_update_info.size_of_per_unit= sizeof(struct sensor_prfy_level) * SENSOR_SMART_LEVEL_NUM;
						nr_param_update_info.multi_nr_map_ptr = block->multi_nr_map_ptr;
						isp_nr_param_update(&nr_param_update_info);
					}
					break;
				case ISP_BLK_UV_CDN: {
						struct sensor_uv_cdn_param *block = (struct sensor_uv_cdn_param*)src_data_ptr;
						multi_mode[ISP_BLK_UV_CDN_TYPE][i] = block->nr_scene_flag;
						block->multi_nr_map_ptr = (isp_uint *)&multi_mode[ISP_BLK_UV_CDN_TYPE][0];
						if(ISP_MODE_ID_COMMON == i) {
							temp_param[ISP_BLK_UV_CDN_TYPE] = block->param_ptr;
						} else {
							block->param_ptr = temp_param[ISP_BLK_UV_CDN_TYPE];
						}
						nr_param_update_info.param_type= ISP_BLK_UV_CDN_TYPE;
						nr_param_update_info.nr_param_ptr = block->param_ptr;
						nr_param_update_info.size_of_per_unit= sizeof(struct sensor_uv_cdn_level) * SENSOR_SMART_LEVEL_NUM;
						nr_param_update_info.multi_nr_map_ptr = block->multi_nr_map_ptr;
						isp_nr_param_update(&nr_param_update_info);
					}
					break;
				case ISP_BLK_EDGE_V1: {
						struct sensor_ee_param *block = (struct sensor_ee_param*)src_data_ptr;
						multi_mode[ISP_BLK_EDGE_TYPE][i] = block->nr_scene_flag;
						block->multi_nr_map_ptr = (isp_uint *)&multi_mode[ISP_BLK_EDGE_TYPE][0];
						if(ISP_MODE_ID_COMMON == i) {
							temp_param[ISP_BLK_EDGE_TYPE] = block->param_ptr;
						} else {
							block->param_ptr = temp_param[ISP_BLK_EDGE_TYPE];
						}
						nr_param_update_info.param_type = ISP_BLK_EDGE_TYPE;
						nr_param_update_info.nr_param_ptr = block->param_ptr;
						nr_param_update_info.size_of_per_unit= sizeof(struct sensor_ee_level) * SENSOR_SMART_LEVEL_NUM;
						nr_param_update_info.multi_nr_map_ptr = block->multi_nr_map_ptr;
						isp_nr_param_update(&nr_param_update_info);
					}
					break;
				case ISP_BLK_UV_POSTCDN: {
						struct sensor_uv_postcdn_param *block = (struct sensor_uv_postcdn_param*)src_data_ptr;
						multi_mode[ISP_BLK_UV_POSTCDN_TYPE][i] = block->nr_scene_flag;
						block->multi_nr_map_ptr = (isp_uint *)&multi_mode[ISP_BLK_UV_POSTCDN_TYPE][0];
						if(ISP_MODE_ID_COMMON == i) {
							temp_param[ISP_BLK_UV_POSTCDN_TYPE] = block->param_ptr;
						} else {
							block->param_ptr = temp_param[ISP_BLK_UV_POSTCDN_TYPE];
						}
						nr_param_update_info.param_type= ISP_BLK_UV_POSTCDN_TYPE;
						nr_param_update_info.nr_param_ptr = block->param_ptr;
						nr_param_update_info.size_of_per_unit= sizeof(struct sensor_uv_postcdn_level) * SENSOR_SMART_LEVEL_NUM;
						nr_param_update_info.multi_nr_map_ptr = block->multi_nr_map_ptr;
						isp_nr_param_update(&nr_param_update_info);
					}
					break;
				case ISP_BLK_IIRCNR_IIR: {
						struct sensor_iircnr_param *block = (struct sensor_iircnr_param*)src_data_ptr;
						multi_mode[ISP_BLK_IIRCNR_IIR_TYPE][i] = block->nr_scene_flag;
						block->multi_nr_map_ptr = (isp_uint *)&multi_mode[ISP_BLK_IIRCNR_IIR_TYPE][0];
						if(ISP_MODE_ID_COMMON == i) {
							temp_param[ISP_BLK_IIRCNR_IIR_TYPE] = block->param_ptr;
						} else {
							block->param_ptr = temp_param[ISP_BLK_IIRCNR_IIR_TYPE];
						}
						nr_param_update_info.param_type= ISP_BLK_IIRCNR_IIR_TYPE;
						nr_param_update_info.nr_param_ptr = block->param_ptr;
						nr_param_update_info.size_of_per_unit= sizeof(struct sensor_iircnr_level) * SENSOR_SMART_LEVEL_NUM;
						nr_param_update_info.multi_nr_map_ptr = block->multi_nr_map_ptr;
						isp_nr_param_update(&nr_param_update_info);
					}
					break;
				case ISP_BLK_IIRCNR_YRANDOM: {
						struct sensor_iircnr_yrandom_param *block = (struct sensor_iircnr_yrandom_param*)src_data_ptr;
						multi_mode[ISP_BLK_IIRCNR_YRANDOM_TYPE][i] = block->nr_scene_flag;
						block->multi_nr_map_ptr = (isp_uint *)&multi_mode[ISP_BLK_IIRCNR_YRANDOM_TYPE][0];
						if(ISP_MODE_ID_COMMON == i) {
								temp_param[ISP_BLK_IIRCNR_YRANDOM_TYPE] = block->param_ptr;
						} else {
							block->param_ptr = temp_param[ISP_BLK_IIRCNR_YRANDOM_TYPE];
						}
						nr_param_update_info.param_type= ISP_BLK_IIRCNR_YRANDOM_TYPE;
						nr_param_update_info.nr_param_ptr = block->param_ptr;
						nr_param_update_info.size_of_per_unit= sizeof(struct sensor_iircnr_yrandom_level) * SENSOR_SMART_LEVEL_NUM;
						nr_param_update_info.multi_nr_map_ptr = block->multi_nr_map_ptr;
						isp_nr_param_update(&nr_param_update_info);
					}
					break;
				case ISP_BLK_UVDIV_V1: {
						struct sensor_cce_uvdiv_param_v1 *block = (struct sensor_cce_uvdiv_param_v1*)src_data_ptr;
						multi_mode[ISP_BLK_UVDIV_TYPE][i] = block->nr_scene_flag;
						block->multi_nr_map_ptr = (isp_uint *)&multi_mode[ISP_BLK_UVDIV_TYPE][0];
						if(ISP_MODE_ID_COMMON == i) {
							temp_param[ISP_BLK_UVDIV_TYPE] = block->param_ptr;
						} else {
							block->param_ptr = temp_param[ISP_BLK_UVDIV_TYPE];
						}
						nr_param_update_info.param_type= ISP_BLK_UVDIV_TYPE;
						nr_param_update_info.nr_param_ptr = block->param_ptr;
						nr_param_update_info.size_of_per_unit= sizeof(struct sensor_cce_uvdiv_level) * SENSOR_SMART_LEVEL_NUM;
						nr_param_update_info.multi_nr_map_ptr = block->multi_nr_map_ptr;
						isp_nr_param_update(&nr_param_update_info);
					}
					break;
				case ISP_BLK_YIQ_AFM: {
						struct sensor_y_afm_param *block = (struct sensor_y_afm_param*)src_data_ptr;
						multi_mode[ISP_BLK_YIQ_AFM_TYPE][i] = block->nr_scene_flag;
						block->multi_nr_map_ptr = (isp_uint *)&multi_mode[ISP_BLK_YIQ_AFM_TYPE][0];
						if(ISP_MODE_ID_COMMON == i) {
							temp_param[ISP_BLK_YIQ_AFM_TYPE] = block->param_ptr;
						} else {
							block->param_ptr = temp_param[ISP_BLK_YIQ_AFM_TYPE];
						}
						nr_param_update_info.param_type= ISP_BLK_YIQ_AFM_TYPE;
						nr_param_update_info.nr_param_ptr = block->param_ptr;
						nr_param_update_info.size_of_per_unit= sizeof(struct sensor_y_afm_level) * SENSOR_SMART_LEVEL_NUM;
						nr_param_update_info.multi_nr_map_ptr = block->multi_nr_map_ptr;
						isp_nr_param_update(&nr_param_update_info);
					}
					break;
				default:
					break;
				}
			}

			memcpy((void *)dst_data_ptr, (void *)src_data_ptr, src_header[j].size);
			memcpy((void *)dst_header[j].name, (void *)src_header[j].block_name, sizeof(dst_header[j].name));
		}

		if (max_num < src_mod_ptr->block_num)
			max_num = src_mod_ptr->block_num;

		dst_mod_ptr->block_num = src_mod_ptr->block_num;
		dst_mod_ptr->mode_id = src_mod_ptr->mode_id;
		dst_mod_ptr->resolution.w = src_mod_ptr->width;
		dst_mod_ptr->resolution.h = src_mod_ptr->height;
		dst_mod_ptr->fps = src_mod_ptr->fps;
		memcpy((void *)dst_mod_ptr->mode_name, (void *)src_mod_ptr->mode_name, sizeof(src_mod_ptr->mode_name));
		ISP_LOGI("mode[%d]: param modify_time :%d", i, src_mod_ptr->reserved[0]);

		if (PNULL != cxt_ptr->merged_mode_array[i]) {
			free(cxt_ptr->merged_mode_array[i]);
			cxt_ptr->merged_mode_array[i] = PNULL;
		}

		cxt_ptr->merged_mode_array[i] = (struct isp_pm_mode_param *)malloc(sizeof(struct isp_pm_mode_param));
		if (PNULL == cxt_ptr->merged_mode_array[i]) {
			ISP_LOGE("merged mode malloc error, i=%d", i);
			rtn = ISP_ERROR;
			goto _mode_list_init_error_exit;
		}
		memset((void *)cxt_ptr->merged_mode_array[i], 0x00, sizeof(struct isp_pm_mode_param));
	}

	size = max_num * cxt_ptr->mode_num * sizeof(struct isp_pm_param_data);
	cxt_ptr->tmp_param_data_ptr = (struct isp_pm_param_data *)malloc(size);
	if (PNULL == cxt_ptr->tmp_param_data_ptr) {
		ISP_LOGE("tmp param malloc error");
		rtn = ISP_ERROR;
		goto _mode_list_init_error_exit;
	}
	memset((void *)cxt_ptr->tmp_param_data_ptr, 0x00, size);

	return rtn;

_mode_list_init_error_exit:

	isp_pm_mode_list_deinit(handle);

	return rtn;
}

static isp_s32 isp_pm_mode_param_convert(struct isp_pm_mode_param *next_mode_in, struct isp_pm_mode_param *active_mode_in, struct isp_pm_mode_update_status *mode_convert_status)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 block_num_active = 0;
	isp_u32 block_num_next = 0;
	isp_u32 i = 0, j = 0, count = 0;
	isp_s32 smart_index = -1;
	void *smart_absolute_addr = NULL;

	if( NULL == next_mode_in || NULL == active_mode_in) {
		rtn = ISP_ERROR;
		ISP_LOGE("param address is null error:%p %p", next_mode_in, active_mode_in);
		return rtn;
	}

	block_num_active = active_mode_in->block_num;
	block_num_next = next_mode_in->block_num;

	memset(mode_convert_status,0x00,sizeof(struct isp_pm_mode_update_status));

	for (i = 0; i < block_num_active; i++) {
		for (j = 0; j < block_num_next; j++) {
			if (active_mode_in->header[i].block_id == next_mode_in->header[j].block_id) {
				if ( next_mode_in->header[j].source_flag != active_mode_in->header[i].source_flag ) {
					mode_convert_status->pm_blk_status[count].block_id = active_mode_in->header[i].block_id;
					mode_convert_status->pm_blk_status[count].update_flag = 1;
					if (ISP_BLK_SMART == active_mode_in->header[i].block_id) {
						smart_index = count;
						smart_absolute_addr = active_mode_in->header[i].absolute_addr;
					}
					count++;
				}
				break;
			} else {
				continue;
			}
		}
	}

	if (smart_index >= 0 && NULL != smart_absolute_addr) {
		struct isp_smart_param *smart_param = (struct isp_smart_param *)smart_absolute_addr;

		ISP_LOGI("update smart related blocks");

		for (i=0; i<smart_param->block_num; i++) {
			isp_u32 block_id = smart_param->block[i].block_id;

			if (block_id >= ISP_BLK_EXT || 0 == block_id || 0 == smart_param->block[i].enable)
				continue;

			mode_convert_status->pm_blk_status[count].block_id = block_id;
			mode_convert_status->pm_blk_status[count].update_flag = 1;
			count++;

			ISP_LOGI("[%d], id=0x%x update", i, block_id);
		}
	}

	mode_convert_status->blk_num = count;

	return rtn;
}

static isp_s32 isp_pm_mode_common_to_other(struct isp_pm_mode_param *mode_common_in, struct isp_pm_mode_param *mode_other_list_out)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i = 0, j = 0, temp_block_num = 0;
	isp_u32 common_block_num = 0, other_block_num = 0;

	if ( NULL == mode_common_in || NULL == mode_other_list_out) {
		rtn = ISP_ERROR;
		ISP_LOGE("param is null error:%p %p", mode_common_in, mode_other_list_out);
		return rtn;
	}

	common_block_num = mode_common_in->block_num;
	other_block_num  = mode_other_list_out->block_num;

	if (common_block_num < other_block_num) {
		rtn = ISP_ERROR;
		ISP_LOGE("block num error: %d %d",common_block_num, other_block_num);
		return rtn;
	}

	temp_block_num = other_block_num;
	for (i = 0; i < common_block_num; i++) {
		for (j = 0; j < other_block_num; j++) {
			if (mode_common_in->header[i].block_id == mode_other_list_out->header[j].block_id) {
				mode_other_list_out->header[j].source_flag = mode_other_list_out->mode_id;
				break;
			}
		}
		if (j == other_block_num) {
			memcpy((isp_u8 *)&mode_other_list_out->header[temp_block_num], (isp_u8 *)&mode_common_in->header[i], sizeof(struct isp_pm_block_header));
			mode_other_list_out->header[temp_block_num].source_flag = mode_common_in->mode_id;
			temp_block_num++;
		}
	}

	mode_other_list_out->block_num = temp_block_num;

	return rtn;
}

static isp_s32 isp_pm_layout_param_and_init(isp_pm_handle_t handle)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i = 0, counts = 0, mode_count = 0;
	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;

	if (NULL == pm_cxt_ptr) {
		ISP_LOGE("cxt ptr is null error!\n");
		rtn = ISP_ERROR;
		return rtn;
	}

	mode_count = ISP_TUNE_MODE_MAX;

	for (i = ISP_MODE_ID_COMMON; i < mode_count; i++) {
		if (PNULL == (isp_u8 *)pm_cxt_ptr->tune_mode_array[i]) {
			continue;
		}
		memcpy((isp_u8 *)pm_cxt_ptr->merged_mode_array[i], (isp_u8 *)pm_cxt_ptr->tune_mode_array[i],
				sizeof(struct isp_pm_mode_param));
	}

	for (i = ISP_MODE_ID_PRV_0; i < mode_count; i++) {
		if (PNULL == pm_cxt_ptr->merged_mode_array[i]) {
			continue;
		}
		rtn = isp_pm_mode_common_to_other(pm_cxt_ptr->merged_mode_array[ISP_MODE_ID_COMMON],
											pm_cxt_ptr->merged_mode_array[i]);
		if (ISP_SUCCESS != rtn) {
			ISP_LOGE("mode change error");
			rtn = ISP_ERROR;
			break;
		}

		if (0 == pm_cxt_ptr->param_source) {
			memset((void *)&pm_cxt_ptr->cxt_array[counts], 0x00, sizeof(pm_cxt_ptr->cxt_array[counts]));
		}
		pm_cxt_ptr->cxt_array[counts].mode_id = pm_cxt_ptr->merged_mode_array[i]->mode_id;
		counts++;
	}

	pm_cxt_ptr->cxt_num = counts;

	return rtn;
}

static isp_u32 isp_pm_active_mode_param_update(
		struct isp_context *isp_cxt_ptr,
		struct isp_pm_mode_param *mode_param,
		struct isp_pm_mode_update_status* mode_convert_status)
{
	isp_u32 rtn = ISP_SUCCESS;
	isp_u32 i = 0, id = 0, offset = 0;
	isp_u32 blcok_num_active = 0;
	isp_u32 blcok_num_next = 0;
	intptr_t isp_cxt_start_addr = 0;
	uint32_t tmp_idx = 0;
	void* blk_ptr = PNULL;
	void* param_data_ptr = PNULL;
	struct isp_block_cfg *blk_cfg_ptr = PNULL;
	struct isp_block_operations *ops = PNULL;
	struct isp_pm_block_header *blk_header_ptr = PNULL;

	if ((PNULL == isp_cxt_ptr)
		|| (PNULL == mode_param)
		|| (PNULL == mode_convert_status)) {
		ISP_LOGE("param ptr is null error:%p %p %p",
				isp_cxt_ptr, mode_param, mode_convert_status);
		rtn = ISP_ERROR;
		return rtn;
	}

	for (i = 0; i < mode_convert_status->blk_num; i++) {
		if (mode_convert_status->pm_blk_status[i].update_flag){
			id = mode_convert_status->pm_blk_status[i].block_id;
			blk_cfg_ptr = isp_pm_get_block_cfg(id);
			blk_header_ptr = isp_pm_get_block_header(mode_param, id, &tmp_idx);
			if ((PNULL != blk_cfg_ptr) && (PNULL != blk_header_ptr)) {
				if (blk_cfg_ptr->ops) {
					ops = blk_cfg_ptr->ops;
					isp_cxt_start_addr = (intptr_t)isp_cxt_ptr;/*get isp context start address*/
					offset = blk_cfg_ptr->offset;
					blk_ptr = (void *)(isp_cxt_start_addr + offset);/*current block struct address;*/
					param_data_ptr = (void *)blk_header_ptr->absolute_addr;/*current block struct address;*/
					if (ops->reset) {
						ops->reset(blk_ptr, blk_cfg_ptr->param_size);
					}
					if (ops->init) {
						ops->init(blk_ptr, param_data_ptr, blk_header_ptr, &mode_param->resolution);
					}
				}
			}
		}

	}

	return rtn;
}

static struct isp_data_info *isp_pm_buffer_alloc(struct isp_pm_buffer *buff_ptr, isp_u32 size)
{
	isp_u32 idx = 0;
	struct isp_data_info *data_ptr = PNULL;

	if ((size > 0) && (buff_ptr->count < ISP_PM_BUF_NUM)) {
		idx = buff_ptr->count;
		data_ptr = &buff_ptr->buf_array[idx];
		data_ptr->data_ptr = (void*)malloc(size);
		if (PNULL == data_ptr->data_ptr) {
			ISP_LOGE("alloc buffer error, size: %d", size);
			data_ptr = PNULL;
			return data_ptr;
		}
		memset((void *)data_ptr->data_ptr, 0x00, size);
		data_ptr->size = size;
		buff_ptr->count++;
	}

	return data_ptr;
}

static isp_s32 isp_pm_buffer_free(struct isp_pm_buffer *buffer_ptr)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i = 0;

	for (i = 0; i < buffer_ptr->count; i++) {
		if (PNULL != buffer_ptr->buf_array[i].data_ptr) {
			free(buffer_ptr->buf_array[i].data_ptr);
			buffer_ptr->buf_array[i].data_ptr = PNULL;
			buffer_ptr->buf_array[i].size = 0;
		}
	}

	return rtn;
}

static isp_s32 isp_pm_set_block_param (struct isp_pm_context *pm_cxt_ptr, struct isp_pm_param_data *param_data_ptr)
{
	isp_s32 rtn = ISP_SUCCESS;
	void *blk_ptr = PNULL;
	intptr_t cxt_start_addr = 0;
	isp_u32 id = 0, cmd = 0, offset = 0, size = 0;
	uint32_t tmp_idx = 0;
	struct isp_block_operations *ops = PNULL;
	struct isp_block_cfg *blk_cfg_ptr = PNULL;
	struct isp_pm_block_header *blk_header_ptr = PNULL;

	id = param_data_ptr->id;
	blk_cfg_ptr = isp_pm_get_block_cfg(id);
	blk_header_ptr = isp_pm_get_block_header(pm_cxt_ptr->active_mode, id, &tmp_idx);
	if ((PNULL != blk_cfg_ptr) &&(PNULL != blk_header_ptr)) {
		if (blk_cfg_ptr->ops) {
			cmd = param_data_ptr->cmd;
			cxt_start_addr = (intptr_t)pm_cxt_ptr->active_cxt_ptr;
			offset = blk_cfg_ptr->offset;
			ops = blk_cfg_ptr->ops;
			blk_ptr = (void*)(cxt_start_addr + offset);
			if ((PNULL == blk_ptr)\
				|| (PNULL == blk_header_ptr)) {
				rtn = ISP_ERROR;
				ISP_LOGE ("param is null error: blk_addr:%p, param data_ptr:%p, header:%p",
							blk_ptr, param_data_ptr, blk_header_ptr);
			} else {
				ops->set(blk_ptr, cmd, param_data_ptr->data_ptr, blk_header_ptr);
			}
		}
	} else {
		ISP_LOGE("id is invalidated, id=%d", id);
	}

	return rtn;
}

static isp_s32 isp_pm_set_mode(isp_pm_handle_t handle, isp_u32 mode_id)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_context *isp_cxt_ptr = PNULL;
	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;
	struct isp_pm_mode_param *next_mode_param = PNULL;
	struct isp_pm_tune_merge_param merged_mode_param;

	if (ISP_TUNE_MODE_MAX < mode_id) {
		ISP_LOGE("mode id is error, id=%d\n", mode_id);
		rtn = ISP_ERROR;
		return rtn;
	}

	merged_mode_param.tune_merge_para = pm_cxt_ptr->merged_mode_array;
	merged_mode_param.mode_num = pm_cxt_ptr->mode_num;

	next_mode_param = isp_pm_get_mode(merged_mode_param, mode_id);
	if (PNULL == next_mode_param) {
		ISP_LOGE("mode pointer is null error");
		rtn = ISP_ERROR;
		goto _pm_set_mode_error_exit;
	}
	pm_cxt_ptr->active_mode = next_mode_param;

	isp_cxt_ptr = isp_pm_get_context(pm_cxt_ptr, mode_id);
	if (PNULL == isp_cxt_ptr) {
		ISP_LOGE("cxt pointer is null error");
		rtn = ISP_ERROR;
		goto _pm_set_mode_error_exit;
	}
	pm_cxt_ptr->active_cxt_ptr = isp_cxt_ptr;

	isp_pm_context_init(handle);

_pm_set_mode_error_exit:

	return rtn;
}


static isp_s32 isp_pm_change_mode(isp_pm_handle_t handle, isp_u32 mode_id)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_context *isp_cxt_ptr = PNULL;
	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;
	struct isp_pm_mode_param *next_mode_param = PNULL;
	struct isp_pm_tune_merge_param merged_mode_param;
	struct isp_pm_mode_param *org_mode_param = PNULL;

	if (ISP_TUNE_MODE_MAX < mode_id) {
		ISP_LOGE("mode id is error, id=%d\n", mode_id);
		rtn = ISP_ERROR;
		return rtn;
	}

	org_mode_param = pm_cxt_ptr->active_mode;
	merged_mode_param.tune_merge_para = pm_cxt_ptr->merged_mode_array;
	merged_mode_param.mode_num = pm_cxt_ptr->mode_num;

	next_mode_param = isp_pm_get_mode(merged_mode_param, mode_id);
	if (PNULL == next_mode_param) {
		ISP_LOGE("mode pointer is null error");
		rtn = ISP_ERROR;
		goto _pm_set_mode_error_exit;
	}
	pm_cxt_ptr->active_mode = next_mode_param;

	isp_cxt_ptr = isp_pm_get_context(pm_cxt_ptr, mode_id);
	if (PNULL == isp_cxt_ptr) {
		ISP_LOGE("cxt pointer is null error");
		rtn = ISP_ERROR;
		goto _pm_set_mode_error_exit;
	}
	pm_cxt_ptr->active_cxt_ptr = isp_cxt_ptr;

	isp_pm_context_update(handle, org_mode_param);

_pm_set_mode_error_exit:

	return rtn;
}



static isp_s32 isp_pm_set_param(isp_pm_handle_t handle, enum isp_pm_cmd cmd, void *param_ptr)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pm_context *pm_cxt_ptr = PNULL;

	if ((PNULL == handle) ||(PNULL == param_ptr)) {
		ISP_LOGE("param is null error: %p %p", handle, param_ptr);
		return ISP_ERROR;
	}

	pm_cxt_ptr = (struct isp_pm_context *)handle;

	switch (cmd) {
	case ISP_PM_CMD_SET_MODE:
	{
		uint32_t mode_id = *((isp_u32 *)param_ptr);
		rtn = isp_pm_change_mode(handle, mode_id);
	}
		break;
	case ISP_PM_CMD_SET_AWB:
	case ISP_PM_CMD_SET_AE:
	case ISP_PM_CMD_SET_AF:
	case ISP_PM_CMD_SET_SMART:
	case ISP_PM_CMD_SET_OTHERS:
	case ISP_PM_CMD_SET_SCENE_MODE:
	case ISP_PM_CMD_SET_SPECIAL_EFFECT:
	{
		struct isp_pm_ioctl_input *ioctrl_input_ptr = (struct isp_pm_ioctl_input *)param_ptr;
		struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)ioctrl_input_ptr->param_data_ptr;

		if (PNULL == param_data_ptr) {
			rtn = ISP_ERROR;
			ISP_LOGE("param is invalidated error");
			return rtn;
		}

		rtn = isp_pm_set_block_param(pm_cxt_ptr, param_data_ptr);
	}
		break;
	case ISP_PM_CMD_ALLOC_BUF_MEMORY:
	{
		isp_u32 id = 0, size = 0;
		struct isp_data_info *data_ptr = PNULL;
		struct isp_buffer_size_info *buffer_size_ptr = PNULL;
		struct isp_pm_param_data param_data;
		struct isp_pm_memory_init_param memory_param;
		struct isp_pm_ioctl_input *ioctrl_input_ptr = (struct isp_pm_ioctl_input *)param_ptr;
		struct isp_pm_param_data *param_data_ptr = (struct isp_pm_param_data *)ioctrl_input_ptr->param_data_ptr;

		if (PNULL == param_data_ptr) {
			rtn = ISP_ERROR;
			ISP_LOGE("param data 1 error, cmd: 0x%x\n", cmd);
			return rtn;
		}
		buffer_size_ptr = (struct isp_buffer_size_info *)param_data_ptr->data_ptr;
		if (PNULL == buffer_size_ptr) {
			rtn = ISP_ERROR;
			ISP_LOGE("param data 2 error, cmd:0x%x\n", cmd);
			return rtn;
		}

		size = buffer_size_ptr->count_lines * buffer_size_ptr->pitch;
		data_ptr = (struct isp_data_info *)isp_pm_buffer_alloc(&pm_cxt_ptr->buffer, size);
		if (PNULL == data_ptr) {
			ISP_LOGE("buffer memory alloc error");
			return rtn;
		}
		memory_param.buffer = *data_ptr;
		memory_param.size_info = *buffer_size_ptr;
		param_data = *param_data_ptr;
		param_data.data_ptr = (void *)&memory_param;
		param_data.data_size = sizeof(struct isp_pm_memory_init_param);
		param_data.cmd = ISP_PM_BLK_MEMORY_INIT;
		rtn = isp_pm_set_block_param(pm_cxt_ptr, &param_data);
	}
		break;
	case ISP_PM_CMD_SET_PARAM_SOURCE:
	{
		uint32_t *param_source = (uint32_t *)param_ptr;
		pm_cxt_ptr->param_source = *param_source;
	}
		break;
	default:
		break;
	}

	return rtn;
}

static isp_s32 isp_pm_get_single_block_param(
		struct isp_pm_mode_param *mode_param_in,
		struct isp_context *isp_cxt_ptr,
		struct isp_pm_ioctl_input *blk_info_in,
		isp_u32 *param_count,
		struct isp_pm_param_data *data_param_ptr,
		isp_u32 *rtn_idx)
{
	isp_s32 rtn = ISP_SUCCESS;
	intptr_t isp_cxt_start_addr  =0;
	void* blk_ptr = NULL;
	isp_u32 tm_idx = 0;
	isp_u32 i = 0, id =0, cmd = 0, counts = 0, offset = 0, blk_num = 0;
	struct isp_pm_block_header *blk_header_ptr = PNULL;
	struct isp_pm_param_data *block_param_data_ptr = NULL;
	struct isp_block_operations *ops = PNULL;
	struct isp_block_cfg *blk_cfg_ptr = PNULL;

	if (blk_info_in->param_num > 1) {
		ISP_LOGE("only support block num is one, num: %d\n", blk_info_in->param_num);
		rtn = ISP_ERROR;
		return rtn;
	}

	blk_num = blk_info_in->param_num;
	block_param_data_ptr = blk_info_in->param_data_ptr;
	for( i = 0; i < 1; i++) {//just support get only one block info
		id = block_param_data_ptr[i].id;
		cmd = block_param_data_ptr[i].cmd;
		blk_header_ptr = isp_pm_get_block_header(mode_param_in,id, &tm_idx);
		blk_cfg_ptr = isp_pm_get_block_cfg(id);
		if ((NULL != blk_cfg_ptr) && (NULL != blk_header_ptr)) {
			isp_cxt_start_addr = (intptr_t)isp_cxt_ptr;
			offset = blk_cfg_ptr->offset;
			ops = blk_cfg_ptr->ops;
			blk_ptr = (void*)(isp_cxt_start_addr + offset);
			if( NULL != ops && NULL != ops->get ) {
				ops->get(blk_ptr,cmd, &data_param_ptr[tm_idx], &blk_header_ptr->is_update);
				counts++;
			}
		}
		*rtn_idx = tm_idx;
	}
	*param_count = counts;

	return rtn;
}

static isp_s32 isp_pm_get_param(isp_pm_handle_t handle, enum isp_pm_cmd cmd, void *in_ptr, void *out_ptr)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 blk_num = 0, id = 0, i = 0, j = 0;
	struct isp_pm_context *pm_cxt_ptr = PNULL;
	struct isp_pm_ioctl_output *result_ptr = PNULL;
	struct isp_pm_param_data *param_data_ptr = PNULL;
	struct isp_pm_mode_param *mode_param_ptr = PNULL;
	struct isp_pm_block_header *blk_header_array = PNULL;

	if (PNULL == out_ptr) {
		ISP_LOGE("output ptr is null error");
		return ISP_PARAM_NULL;
	}

	if (ISP_PM_CMD_GET_MODEID_BY_FPS == cmd) {
		struct isp_pm_context *cxt_ptr = (struct isp_pm_context *)handle;
		*((int*)out_ptr)  = ISP_MODE_ID_VIDEO_0;
		for (i=ISP_MODE_ID_VIDEO_0; i<ISP_MODE_ID_VIDEO_3; i++) {
			if (cxt_ptr->tune_mode_array[i] != NULL) {
				if (cxt_ptr->tune_mode_array[i]->fps ==(uint32_t)(*(int*)in_ptr)) {
					*((int*)out_ptr) = cxt_ptr->tune_mode_array[i]->mode_id;
					break;
				}
			}
		}
	} else if (ISP_PM_CMD_GET_MODEID_BY_RESOLUTION == cmd) {
		struct isp_pm_context *cxt_ptr = (struct isp_pm_context *)handle;
		struct isp_video_start* param_ptr = in_ptr;
		if (param_ptr->work_mode == 0) {
			*((int*)out_ptr)  = ISP_MODE_ID_PRV_0;
			for (i=ISP_MODE_ID_PRV_0; i<ISP_MODE_ID_PRV_3; i++) {
				if (cxt_ptr->tune_mode_array[i] != NULL) {
					if (cxt_ptr->tune_mode_array[i]->resolution.w == param_ptr->size.w) {
						*((int*)out_ptr) = cxt_ptr->tune_mode_array[i]->mode_id;
						break;
					}
				}
			}
		} else {
			*((int*)out_ptr)  = ISP_MODE_ID_CAP_0;
			for (i=ISP_MODE_ID_CAP_0; i<ISP_MODE_ID_CAP_3; i++) {
				if (cxt_ptr->tune_mode_array[i] != NULL) {
					if (cxt_ptr->tune_mode_array[i]->resolution.w == param_ptr->size.w) {
						*((int*)out_ptr) = cxt_ptr->tune_mode_array[i]->mode_id;
						break;
					}
				}
			}
		}
	}  else if (ISP_PM_CMD_GET_DV_MODEID_BY_RESOLUTION == cmd) {
		struct isp_pm_context *cxt_ptr = (struct isp_pm_context *)handle;
		struct isp_video_start* param_ptr = in_ptr;
		if (param_ptr->dv_mode == 0) {
			ISP_LOGE ("param is error, not dv mode\n");
		} else {
			*((int*)out_ptr)  = ISP_MODE_ID_VIDEO_0;
			for (i=ISP_MODE_ID_VIDEO_0; i<ISP_MODE_ID_VIDEO_3; i++) {
				if (cxt_ptr->tune_mode_array[i] != NULL) {
					if (cxt_ptr->tune_mode_array[i]->resolution.w == param_ptr->size.w) {
						*((int*)out_ptr) = cxt_ptr->tune_mode_array[i]->mode_id;
						break;
					}
				}
			}
		}
	} else if ((ISP_PM_CMD_GET_ISP_SETTING == cmd)|| (ISP_PM_CMD_GET_ISP_ALL_SETTING == cmd)) {
			void *blk_ptr = PNULL;
			isp_u32 flag = 0, is_update = 0;
			isp_u32 update_flag = 0;
			isp_u32 offset = 0, counts = 0;
			intptr_t isp_cxt_start_addr  =0;
			struct isp_block_operations *ops = PNULL;
			struct isp_block_cfg *blk_cfg_ptr = PNULL;
			struct isp_context *isp_cxt_ptr = PNULL;
			struct isp_pm_block_header *blk_header_ptr = PNULL;

			if (ISP_PM_CMD_GET_ISP_SETTING == cmd) {
				flag = 0;
			} else if (ISP_PM_CMD_GET_ISP_ALL_SETTING == cmd) {
				flag = 1;
			}

			pm_cxt_ptr = (struct isp_pm_context *)handle;
			isp_cxt_ptr = (struct isp_context *)pm_cxt_ptr->active_cxt_ptr;
			mode_param_ptr = (struct isp_pm_mode_param *)pm_cxt_ptr->active_mode;
			blk_header_array  =(struct isp_pm_block_header *)mode_param_ptr->header;
			param_data_ptr= (struct isp_pm_param_data *)pm_cxt_ptr->tmp_param_data_ptr;
			result_ptr = (struct isp_pm_ioctl_output *)out_ptr;

			blk_num = mode_param_ptr->block_num;
			for (i = 0; i < blk_num; i++) {
				id = blk_header_array[i].block_id;
				blk_cfg_ptr = isp_pm_get_block_cfg(id);
				blk_header_ptr = &blk_header_array[i];
				if ((PNULL != blk_cfg_ptr) && (PNULL != blk_header_ptr)) {
					is_update = 0;
					if (ISP_CHIP_ID_PIKE == isp_dev_get_chip_id(pm_cxt_ptr->isp_ctrl_cxt_handle->handle_device)) {
						if (((ISP_ZERO != blk_header_ptr->is_update) && (ISP_BLK_RGB_GAMC != id))
							|| ((ISP_ZERO != blk_header_ptr->is_update) && (ISP_BLK_RGB_GAMC == id) && pm_cxt_ptr->isp_ctrl_cxt_handle->update_gamma_eb)
							|| (ISP_ZERO != flag))
							is_update = 1;
					} else {
						if ((ISP_ZERO != blk_header_ptr->is_update) || (ISP_ZERO != flag)) {
							is_update = 1;
						}
					}

					if (is_update) {
						isp_cxt_start_addr = (intptr_t)isp_cxt_ptr;
						offset = blk_cfg_ptr->offset;
						ops = blk_cfg_ptr->ops;
						blk_ptr = (void*)(isp_cxt_start_addr + offset);
						if (ops != NULL) {
							if ((PNULL == blk_ptr) || (PNULL == param_data_ptr) ) {
								ISP_LOGE ("param is null error: blk_addr:%p, param_ptr:%p, isp_update:%d\n",\
											blk_ptr, param_data_ptr, blk_header_ptr->is_update);
							} else {
								ops->get(blk_ptr, ISP_PM_BLK_ISP_SETTING, param_data_ptr, &blk_header_ptr->is_update);
							}
						}
						if (param_data_ptr)
							param_data_ptr++;
						counts++;
					}

				} else {
					ISP_LOGE("no operation function:%d", id);
				}
			}
			result_ptr->param_data = pm_cxt_ptr->tmp_param_data_ptr;
			result_ptr->param_num = counts;
	} else if (ISP_PM_CMD_GET_SINGLE_SETTING == cmd) {
		isp_u32 single_param_counts = 0;
		isp_u32 blk_idx = 0;
		struct isp_pm_param_data* single_data_ptr = PNULL;

		pm_cxt_ptr = (struct isp_pm_context *)handle;
		single_data_ptr = pm_cxt_ptr->temp_param_data;
		rtn = isp_pm_get_single_block_param(pm_cxt_ptr->active_mode,
				pm_cxt_ptr->active_cxt_ptr,
				(struct isp_pm_ioctl_input *)in_ptr,
				&single_param_counts,
				single_data_ptr,
				&blk_idx);
		if ( ISP_SUCCESS != rtn ) {
			rtn = ISP_ERROR;
			return rtn;
		}
		result_ptr = (struct isp_pm_ioctl_output *)out_ptr;
		result_ptr->param_num = 0;
		result_ptr->param_data = PNULL;
		result_ptr->param_data = &pm_cxt_ptr->temp_param_data[blk_idx];
		result_ptr->param_num = single_param_counts;//always is one
	}else if (ISP_PM_CMD_GET_AE_VERSION_ID == cmd) {
		pm_cxt_ptr = (struct isp_pm_context *)handle;
		isp_u32 mod_num = 0;
		int *version_id = (int *)out_ptr;
		mod_num = pm_cxt_ptr->mode_num;
		for (j = 0; j < mod_num; j++) {
			mode_param_ptr = (struct isp_pm_mode_param *)pm_cxt_ptr->tune_mode_array[j];
			if (PNULL != mode_param_ptr) {
				blk_header_array  =(struct isp_pm_block_header *)mode_param_ptr->header;
				blk_num = mode_param_ptr->block_num;
				for (i = 0; i < blk_num; i++) {
					if (ISP_BLK_AE_V1  == blk_header_array[i].block_id) {
						*version_id  = blk_header_array[i].version_id;
					}
					if (-1 != *version_id)
						break;
				}
			} else {
				continue;
			}
			if (-1 != *version_id)
				break;
		}
	}else {
		void *awb_data_ptr = PNULL;
		isp_u32 block_id = 0;
		isp_u32 mod_num = 0;
		switch (cmd) {
		case ISP_PM_CMD_GET_INIT_AE:
			block_id = ISP_BLK_AE_V1;
			break;
		case ISP_PM_CMD_GET_INIT_ALSC:
			block_id = ISP_BLK_ALSC;
			break;
		case ISP_PM_CMD_GET_INIT_AWB:
			block_id = ISP_BLK_AWB_V1;
			break;
		case ISP_PM_CMD_GET_INIT_AWB_NEW:
			block_id = ISP_BLK_AWB_NEW;
			break;
		case ISP_PM_CMD_GET_INIT_AF:
			block_id = ISP_BLK_AF_V1;
			break;
		case ISP_PM_CMD_GET_INIT_AF_NEW:
			block_id = ISP_BLK_AF_NEW;
			break;
		case ISP_PM_CMD_GET_INIT_SMART:
			block_id = ISP_BLK_SMART;
			break;
		case ISP_PM_CMD_GET_INIT_AFT:
			block_id = ISP_BLK_AFT;
			break;
		case ISP_PM_CMD_GET_THIRD_PART_INIT_SFT_AF:
			block_id = ISP_BLK_SFT_AF;
			break;
		default:
			break;
		}
		pm_cxt_ptr = (struct isp_pm_context *)handle;
		result_ptr = (struct isp_pm_ioctl_output *)out_ptr;
		result_ptr->param_num = 0;
		result_ptr->param_data = PNULL;
		mod_num = pm_cxt_ptr->mode_num;
		param_data_ptr= (struct isp_pm_param_data *)pm_cxt_ptr->tmp_param_data_ptr;
		for (j = 0; j < mod_num; j++) {
			mode_param_ptr = (struct isp_pm_mode_param *)pm_cxt_ptr->tune_mode_array[j];
			if (PNULL != mode_param_ptr) {
				blk_header_array  =(struct isp_pm_block_header *)mode_param_ptr->header;
				blk_num = mode_param_ptr->block_num;
				for (i = 0; i < blk_num; i++) {
					id = blk_header_array[i].block_id;
					if (block_id == id) {
						break;
					}
				}
				if (i < blk_num) {
					param_data_ptr->data_ptr  =(void *)blk_header_array[i].absolute_addr;
					param_data_ptr->data_size = blk_header_array[i].size;
					param_data_ptr->user_data[0] = blk_header_array[i].bypass;
				} else {
					param_data_ptr->data_ptr  = PNULL;
					param_data_ptr->data_size = 0;
				}
			} else {
				param_data_ptr->data_ptr  = PNULL;
				param_data_ptr->data_size = 0;
			}

			param_data_ptr->cmd = cmd;
			param_data_ptr->mod_id = j;
			param_data_ptr->id = (j << 16) | block_id;/*H-16: mode id L-16: block id*/
			param_data_ptr++;
			result_ptr->param_num++;
		}
		result_ptr->param_data = (void *)pm_cxt_ptr->tmp_param_data_ptr;
	}

	return rtn;
}

static isp_s32 isp_pm_param_init_and_update(isp_pm_handle_t handle, struct isp_pm_init_input *input, isp_u32 mod_id)
{
	isp_s32 rtn = ISP_SUCCESS;

	rtn = isp_pm_mode_list_init(handle, input);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("mode list init error");
		return rtn;
	}

	rtn = isp_pm_layout_param_and_init(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("layout and init error");
		return rtn;
	}

	rtn = isp_pm_active_mode_init(handle, mod_id);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("active mode init error");
		return rtn;
	}

	rtn = isp_pm_set_mode(handle, mod_id);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("set mode error");
		return rtn;
	}

	return rtn;
}

static void isp_pm_free(isp_pm_handle_t handle)
{
	if (PNULL != handle) {
		struct isp_pm_context *cxt_ptr = (struct isp_pm_context *)handle;
		isp_pm_context_deinit(handle);
		isp_pm_mode_list_deinit(handle);
		isp_pm_buffer_free(&cxt_ptr->buffer);

		free(handle);
	}
}

static isp_s32 isp_pm_lsc_otp_param_update(isp_pm_handle_t handle, struct isp_pm_param_data *param_data)
{
	isp_s32 rtn = ISP_SUCCESS;
	void *blk_ptr = PNULL;
	isp_u32 i = 0, offset = 0;
	isp_u32 mod_id = 0, id = 0;
	isp_u32 tmp_idx = 0;
	intptr_t isp_cxt_start_addr = 0;
	struct isp_context *isp_cxt_ptr = PNULL;
	struct isp_block_cfg *blk_cfg = PNULL;
	struct isp_pm_block_header *blk_header_ptr = PNULL;
	struct isp_pm_context *pm_cxt_ptr = (struct isp_pm_context *)handle;

	for (i = ISP_MODE_ID_PRV_0; i < ISP_TUNE_MODE_MAX; ++i) {
		if (pm_cxt_ptr->merged_mode_array[i]) {
			mod_id = pm_cxt_ptr->merged_mode_array[i]->mode_id;
			isp_cxt_ptr = isp_pm_get_context(pm_cxt_ptr, mod_id);
			isp_cxt_start_addr = (intptr_t)isp_cxt_ptr;
			id = param_data->id;
			blk_cfg = isp_pm_get_block_cfg(id);
			blk_header_ptr = isp_pm_get_block_header(pm_cxt_ptr->merged_mode_array[i], id, &tmp_idx);
			if ((PNULL != blk_cfg)
				&&(PNULL !=blk_cfg->ops)
				&& (PNULL != blk_cfg->ops->set)
				&& (PNULL != blk_header_ptr)) {
					offset = blk_cfg->offset;
					blk_ptr = (void *)(isp_cxt_start_addr + offset);
					blk_cfg->ops->set(blk_ptr, ISP_PM_BLK_LSC_OTP, param_data->data_ptr, blk_header_ptr);
			} else {
				ISP_LOGE("cfg, ops, set, header is NULL error");
			}
		}
	}

	return rtn;
}

isp_pm_handle_t isp_pm_init(struct isp_pm_init_input *input, void *output)
{
	UNUSED(output);
	isp_s32 rtn = ISP_SUCCESS;
	isp_pm_handle_t handle = PNULL;

	if (PNULL == input) {
		ISP_LOGE("input param is null error");
		return PNULL;
	}

	handle = isp_pm_context_create();
	rtn = isp_pm_handle_check(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("handle is null error");
		goto init_error_exit;
	}

	rtn = isp_pm_param_init_and_update(handle, input, ISP_MODE_ID_PRV_0);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("init & update error\n");
		goto init_error_exit;
	}

	return handle;

init_error_exit:

	if (handle) {
		struct isp_pm_context *cxt_ptr = handle;
		pthread_mutex_destroy(&cxt_ptr->pm_mutex);

		isp_pm_free(handle);
		handle = PNULL;
	}

	return handle;
}

isp_s32 isp_pm_ioctl(isp_pm_handle_t handle, enum isp_pm_cmd cmd, void *input, void *output)
{
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pm_context *cxt_ptr = handle;

	rtn = isp_pm_handle_check(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("isp pm handel is invalidate\n");
		rtn = ISP_ERROR;
		goto _ioctl_error_exit;
	}

	switch ((cmd & isp_pm_cmd_mask)) {
	case ISP_PM_CMD_SET_BASE:
		pthread_mutex_lock(&cxt_ptr->pm_mutex);
		isp_pm_set_param(handle, cmd, input);
		pthread_mutex_unlock(&cxt_ptr->pm_mutex);
		break;
	case ISP_PM_CMD_GET_BASE:
	case ISP_PM_CMD_GET_THIRD_PART_BASE:
		pthread_mutex_lock(&cxt_ptr->pm_mutex);
		isp_pm_get_param(handle, cmd, input, output);
		pthread_mutex_unlock(&cxt_ptr->pm_mutex);
		break;
	default:
		break;
	}

_ioctl_error_exit:

	return rtn;
}

isp_s32 isp_pm_update(isp_pm_handle_t handle, enum isp_pm_cmd cmd, void *input, void *output)
{
	UNUSED(output);
	isp_s32 rtn = ISP_SUCCESS;

	rtn = isp_pm_handle_check(handle);
	if (ISP_SUCCESS != rtn) {
		rtn = ISP_ERROR;
		ISP_LOGE("handle is invalidated\n");
		goto isp_pm_update_error_exit;
	}

	switch (cmd) {
	case ISP_PM_CMD_UPDATE_ALL_PARAMS:
	{
		struct isp_pm_update_input *update_input_ptr  =(struct isp_pm_update_input *)input;
		rtn = isp_pm_param_init_and_update(handle, (struct isp_pm_init_input *)update_input_ptr, ISP_MODE_ID_PRV_0);
	}
		break;

	case ISP_PM_CMD_UPDATE_LSC_OTP:
	{
		struct isp_pm_param_data *param_data = (struct isp_pm_param_data *)input;
		rtn = isp_pm_lsc_otp_param_update(handle, param_data);
	}
		break;
	default:
		break;
	}

	return rtn;

isp_pm_update_error_exit:
	return rtn;
}

isp_s32 isp_pm_deinit(isp_pm_handle_t handle, void *input, void* output)
{
	UNUSED(input);
	UNUSED(output);
	isp_s32 rtn = ISP_SUCCESS;
	struct isp_pm_context *cxt_ptr = handle;

	rtn = isp_pm_handle_check(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("isp_pm_deinit: handle is invalidated, rtn: %d\n", rtn);

		return ISP_ERROR;
	}

	pthread_mutex_destroy(&cxt_ptr->pm_mutex);

	 isp_pm_free(handle);

	 handle = PNULL;

	return rtn;
}
