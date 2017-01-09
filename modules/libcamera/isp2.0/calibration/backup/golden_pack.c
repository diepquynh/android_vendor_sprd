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
/*------------------------------------------------------------------------------*
*				Dependencies													*
*-------------------------------------------------------------------------------*/
#include "golden_map.h"
#include "golden_pack.h"
#include "otp_pack.h"
/*------------------------------------------------------------------------------*
				Micro Define													*
*-------------------------------------------------------------------------------*/
#define GOLDEN_SUCCESS 	0
#define GOLDEN_ERROR     0XFF

/*------------------------------------------------------------------------------*
*				Data Structures													*
*-------------------------------------------------------------------------------*/
struct data_info {
	void *data;
	uint32_t size;
};

struct golden_gain_info {
	uint32_t ct;
	uint32_t chn_gain_num;
	uint16_t *chn_gain[4];
};

/*------------------------------------------------------------------------------*
*				Local functions													*
*-------------------------------------------------------------------------------*/
static int32_t _module_pack_size(struct golden_module_info *module_info, uint32_t *real_size)
{
	*real_size = sizeof(struct golden_map_module);

	return GOLDEN_SUCCESS;
}

static int32_t _module_pack(struct golden_module_info *module_info, struct data_info *target, uint32_t *real_size)
{
	struct golden_map_module *module_map = NULL;
	uint8_t *data_start = NULL;
	uint32_t reserved_size = 32;

	if (NULL == module_info || NULL == target || NULL == real_size) {
		return GOLDEN_ERROR;
	}

	if (NULL == target->data || target->size < sizeof(*module_map) + reserved_size) {
		return GOLDEN_ERROR;
	}

	data_start = (uint8_t *)target->data;
	module_map = (struct golden_map_module *)data_start;
	module_map->core_version = module_info->core_version;
	module_map->sensor_maker = module_info->sensor_maker;
	module_map->year = module_info->year;
	module_map->month = module_info->month;
	module_map->module_version = module_info->module_version;
	module_map->release_number = module_info->release_number;
	module_map->cal_dll_version = module_info->cal_dll_version;
	module_map->cal_map_version = module_info->cal_map_version;

	memset(module_map->reserved, 0, sizeof(module_map->reserved));

	*real_size = sizeof(*module_map);

	return GOLDEN_SUCCESS;
}

static int32_t _lsc_basic_pack(struct golden_lsc_info *lsc_info, struct data_info *target, uint32_t *real_size)
{
	struct golden_map_lsc_basic *basic_map = NULL;
	uint8_t *data_start = NULL;

	if (NULL == lsc_info || NULL == target || NULL == real_size) {
		return GOLDEN_ERROR;
	}

	if (NULL == target->data || target->size < sizeof(*basic_map)) {
		return GOLDEN_ERROR;
	}

	basic_map = (struct golden_map_lsc_basic *)target->data;
	basic_map->base_gain = (uint16_t)lsc_info->base_gain;
	basic_map->algorithm_version = (uint16_t)((uint8_t)lsc_info->alg_version << 8)
									+ (uint8_t)lsc_info->alg_type;
	basic_map->compress_flag = (uint16_t)lsc_info->compress;
	basic_map->image_width = (uint16_t)lsc_info->img_width;
	basic_map->image_height = (uint16_t)lsc_info->img_height;
	basic_map->gain_width = (uint16_t)lsc_info->gain_width;
	basic_map->gain_height = (uint16_t)lsc_info->gain_height;
	basic_map->optical_x = (uint16_t)lsc_info->center_x;
	basic_map->optical_y = (uint16_t)lsc_info->center_y;
	basic_map->grid_width = (uint16_t)lsc_info->grid_width;
	basic_map->grid_height = (uint16_t)lsc_info->grid_height;
	basic_map->percent = (uint16_t)lsc_info->percent;
	basic_map->bayer_pattern = (uint16_t)lsc_info->bayer_pattern;
	basic_map->reserved = 0;

	*real_size = sizeof(*basic_map);

	return GOLDEN_SUCCESS;
}

static int32_t _lsc_gain_pack(struct golden_gain_info *gain_info, struct data_info *target, uint32_t *real_size)
{
	uint8_t *data_cur = NULL;
	uint32_t chn_gain_size = 0;
	uint32_t i = 0;

	chn_gain_size = gain_info->chn_gain_num * sizeof(uint16_t);

	if (NULL == target->data || target->size < chn_gain_size * LSC_CHN_NUM + 2 * sizeof(uint32_t)) {
		return GOLDEN_ERROR;
	}

	data_cur = (uint8_t *)target->data;

	*(uint32_t *)data_cur = gain_info->ct;
	data_cur += 4;
	*(uint32_t *)data_cur = gain_info->chn_gain_num;
	data_cur += 4;

	for (i=0; i<LSC_CHN_NUM; i++) {
		memcpy(data_cur, gain_info->chn_gain[i], chn_gain_size);
		data_cur += chn_gain_size;
	}

	*real_size = data_cur - (uint8_t *)target->data;

	return GOLDEN_SUCCESS;
}

static int32_t _lsc_pack_size(struct golden_lsc_info *lsc_info, uint32_t *real_size)
{
	uint32_t header_size = 0;
	uint32_t block_info_size = 0;
	uint32_t basic_data_size = 0;
	uint32_t std_data_size = 0;
	uint32_t nonstd_data_size = 0;
	uint32_t data_size = 0;
	uint32_t block_num = 0;
	uint32_t std_header_size = 2 * sizeof(uint32_t);
	uint32_t nonstd_header_size = 2 * sizeof(uint32_t);
	uint32_t i = 0;

	header_size = sizeof(struct golden_block_info);
	data_size += header_size;

	block_num = lsc_info->nonstd_num + 2;
	block_info_size = block_num * sizeof(struct golden_block_info);
	data_size += block_info_size;

	basic_data_size = sizeof(struct golden_map_lsc_basic);
	data_size += basic_data_size;

	std_data_size = lsc_info->std_gain.gain_width * lsc_info->std_gain.gain_height
						* sizeof(uint16_t) * LSC_CHN_NUM + std_header_size;
	data_size += std_data_size;

	for (i=0; i<lsc_info->nonstd_num; i++) {
		nonstd_data_size = lsc_info->nonstd_diff[i].chn_num * sizeof(uint16_t) * LSC_CHN_NUM
												+ nonstd_header_size;
		data_size += nonstd_data_size;
	}

	*real_size = data_size;

	return GOLDEN_SUCCESS;
}

static int32_t _lsc_pack(struct golden_lsc_info *lsc_info, struct data_info *target, uint32_t *real_size)
{
	int32_t rtn = GOLDEN_ERROR;
	struct data_info basic_pack = {0};
	struct data_info gain_pack = {0};
	uint8_t *target_cur = NULL;
	uint8_t *data_cur = NULL;
	uint8_t *target_end = NULL;
	struct golden_lsc_header *header = NULL;
	struct golden_block_info *block_info = NULL;
	struct golden_gain_info gain_info = {0};
	uint32_t lsc_pack_size = 0;
	uint32_t block_real_size = 0;
	uint32_t block_index = 0;
	uint32_t i = 0;

	rtn = _lsc_pack_size(lsc_info, &lsc_pack_size);
	if (GOLDEN_SUCCESS != rtn) {
		return rtn;
	}

	memset(target->data, 0, target->size);

	target_cur = (uint8_t *)target->data;
	target_end = target_cur + target->size;

	//write header
	header = (struct golden_lsc_header *)target_cur;
	target_cur += sizeof(*header);

	if (target_cur >= target_end)
		return GOLDEN_ERROR;

	header->block_num = lsc_info->nonstd_num + 2;
	header->length = 0;
	header->version = GOLDEN_LSC_BLOCK_VERSION;

	//write block info
	block_info = (struct golden_block_info *)target_cur;
	target_cur += header->block_num * sizeof(*block_info);

	if (target_cur >= target_end)
		return GOLDEN_ERROR;

	//block 0: basic info pack
	basic_pack.data = target_cur;
	basic_pack.size = target_end - target_cur;
	rtn = _lsc_basic_pack(lsc_info, &basic_pack, &block_real_size);
	if (GOLDEN_SUCCESS != rtn)
		return GOLDEN_ERROR;

	basic_pack.size = block_real_size;
	block_info[block_index].id = GOLDEN_BLOCK_ID_LSC_BASIC;
	block_info[block_index].offset = target_cur - (uint8_t *)header;
	block_info[block_index].size = basic_pack.size;
	block_index++;
	target_cur += basic_pack.size;

	//block 2: std gain pack
	for (i=0; i<LSC_CHN_NUM; i++)
		gain_info.chn_gain[i] = lsc_info->std_gain.chn_gain[i];

	gain_info.chn_gain_num = lsc_info->std_gain.gain_width * lsc_info->std_gain.gain_height;
	gain_info.ct = lsc_info->std_ct;

	gain_pack.data = target_cur;
	gain_pack.size = target_end - target_cur;
	rtn = _lsc_gain_pack(&gain_info, &gain_pack, &block_real_size);
	if (GOLDEN_SUCCESS != rtn)
		return GOLDEN_ERROR;

	gain_pack.size = block_real_size;
	block_info[block_index].id = GOLDEN_BLOCK_ID_LSC_STD_GAIN;
	block_info[block_index].offset = target_cur - (uint8_t *)header;
	block_info[block_index].size = gain_pack.size;
	block_index++;
	target_cur += gain_pack.size;

	for (i=0; i<lsc_info->nonstd_num; i++) {

		uint32_t j = 0;

		//block 2 + i: diff gain pack
		for (j=0; j<LSC_CHN_NUM; j++)
			gain_info.chn_gain[j] = lsc_info->nonstd_diff[i].diff[j];

		gain_info.chn_gain_num = lsc_info->nonstd_diff[i].chn_num;
		gain_info.ct = lsc_info->nonstd_ct[i];

		gain_pack.data = target_cur;
		gain_pack.size = target_end - target_cur;
		rtn = _lsc_gain_pack(&gain_info, &gain_pack, &block_real_size);
		if (GOLDEN_SUCCESS != rtn)
			return GOLDEN_ERROR;

		gain_pack.size = block_real_size;
		block_info[block_index].id = GOLDEN_BLOCK_ID_LSC_DIFF_GAIN;
		block_info[block_index].offset = target_cur - (uint8_t *)header;
		block_info[block_index].size = gain_pack.size;
		block_index++;
		target_cur += gain_pack.size;
	}

	header->length = target_cur - (uint8_t *)header;
	*real_size = header->length;

	return GOLDEN_SUCCESS;
}

static int32_t _awb_pack_size(struct golden_awb_info *awb_info, uint32_t *real_size)
{
	*real_size = sizeof(struct golden_map_awb);

	return GOLDEN_SUCCESS;
}


static int32_t _awb_pack(struct golden_awb_info *awb_info, struct data_info *target, uint32_t *real_size)
{

	struct golden_map_awb *awb_map = (struct golden_map_awb *)target->data;

	if (NULL == target->data || target->size < sizeof(*awb_map)) {
		return GOLDEN_ERROR;
	}

	awb_map->version = GOLDEN_AWB_BLOCK_VERSION;
	awb_map->avg_r = awb_info->avg_r;
	awb_map->avg_g = awb_info->avg_g;
	awb_map->avg_b = awb_info->avg_b;

	*real_size = sizeof(*awb_map);

	return GOLDEN_SUCCESS;
}

/*------------------------------------------------------------------------------*
*				Public functions												*
*-------------------------------------------------------------------------------*/
int32_t get_golden_pack_size(struct golden_pack_param *param, uint32_t *size)
{
	int32_t rtn = GOLDEN_ERROR;
	uint32_t module_size = 0;
	uint32_t awb_size = 0;
	uint32_t lsc_size = 0;
	uint32_t header_size = 0;
	uint32_t block_info_size = 0;
	uint32_t block_num = 3;
	uint32_t total_size = 0;
	uint32_t crc_size = 4;
	uint32_t end_size = 4;

	header_size = sizeof(struct golden_header);
	total_size += header_size;
	block_info_size = sizeof(struct golden_block_info) * block_num;
	total_size += block_info_size;

	rtn = _module_pack_size(&param->module_info, &module_size);
	if (GOLDEN_SUCCESS != rtn)
		return GOLDEN_ERROR;

	total_size += module_size;

	rtn = _lsc_pack_size(&param->lsc_info, &lsc_size);
	if (GOLDEN_SUCCESS != rtn)
		return GOLDEN_ERROR;

	total_size += lsc_size;

	rtn = _awb_pack_size(&param->awb_info, &awb_size);
	if (GOLDEN_SUCCESS != rtn)
		return GOLDEN_ERROR;

	total_size += awb_size;
	total_size += crc_size;
	total_size += end_size;

	*size = total_size;

	return GOLDEN_SUCCESS;
}

int32_t golden_pack(struct golden_pack_param *param, struct golden_pack_result *result)
{
	int32_t rtn = GOLDEN_ERROR;
	struct data_info awb = {0};
	struct data_info lsc = {0};
	struct data_info module = {0};
	uint8_t *target_cur = NULL;
	uint8_t *data_cur = NULL;
	uint8_t *target_end = NULL;
	uint32_t real_size = 0;
	struct golden_header *header = NULL;
	struct golden_block_info *block_info = NULL;
	uint32_t total_size = 0;
	uint32_t block_index = 0;
	uint32_t crc_value = 0;
	uint32_t crc_size = 4;
	uint32_t end_size = 4;

	if (NULL == param || NULL == result)
		return GOLDEN_ERROR;

	if (NULL == param->target_buf || 0 == param->target_buf_size)
		return GOLDEN_ERROR;

	rtn = get_golden_pack_size(param, &total_size);
	if (GOLDEN_SUCCESS != rtn)
		return rtn;

	memset(param->target_buf, 0, param->target_buf_size);

	target_cur = (uint8_t *)param->target_buf;
	target_end = target_cur + param->target_buf_size;

	//write random header
	header = (struct golden_header *)target_cur;
	target_cur += sizeof(*header);

	if (target_cur >= target_end)
		return GOLDEN_ERROR;

	header->block_num = 3;
	header->start = GOLDEN_START;
	header->version = GOLDEN_VERSION;

	//write block info
	block_info = (struct golden_block_info *)target_cur;
	target_cur += header->block_num * sizeof(*block_info);

	if (target_cur >= target_end)
		return GOLDEN_ERROR;

	//block 0: module info pack
	module.data = target_cur;
	module.size = target_end - target_cur;
	rtn = _module_pack(&param->module_info, &module, &real_size);
	if (GOLDEN_SUCCESS != rtn)
		return GOLDEN_ERROR;

	module.size = real_size;
	block_info[block_index].id = GOLDEN_MODULE_BLOCK_ID;
	block_info[block_index].offset = target_cur - (uint8_t *)header;
	block_info[block_index].size = module.size;
	block_index++;
	target_cur += module.size;

	//block 1: lsc pack
	lsc.data = target_cur;
	lsc.size = target_end - target_cur;
	rtn = _lsc_pack(&param->lsc_info, &lsc, &real_size);
	if (GOLDEN_SUCCESS != rtn)
		return rtn;

	lsc.size = real_size;
	block_info[block_index].id = GOLDEN_LSC_BLOCK_ID;
	block_info[block_index].offset = target_cur - (uint8_t *)header;
	block_info[block_index].size = lsc.size;
	block_index++;
	target_cur += lsc.size;

	//block 2: awb pack
	awb.data = target_cur;
	awb.size = target_end - target_cur;
	rtn = _awb_pack(&param->awb_info, &awb, &real_size);
	if (GOLDEN_SUCCESS != rtn)
		return rtn;

	awb.size = real_size;
	block_info[block_index].id = GOLDEN_AWB_BLOCK_ID;
	block_info[block_index].offset = target_cur - (uint8_t *)header;
	block_info[block_index].size = awb.size;
	block_index++;
	target_cur += awb.size;

	*(uint32_t *)target_cur = crc_value;
	target_cur += crc_size;

	*(uint32_t *)target_cur = GOLDEN_END;
	target_cur += end_size;

	result->real_size = target_cur - (uint8_t *)header;
	header->length = result->real_size;

	rtn = GOLDEN_SUCCESS;

	return rtn;
}