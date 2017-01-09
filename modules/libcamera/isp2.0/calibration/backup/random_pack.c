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

#include "random_map.h"
#include "random_pack.h"

#define RANDOM_SUCCESS 	0
#define RANDOM_ERROR     0XFF

struct data_info {
	void *data;
	uint32_t size;
};

static int32_t _lsc_pack(struct random_lsc_info *lsc_info, struct data_info *target, uint32_t *real_size)
{
	struct random_map_lsc *lsc_map = target->data;
	uint32_t gain_size = lsc_info->chn_gain_size * 4;
	uint32_t crc_len = 4;
	uint8_t *data_start = NULL;
	uint16_t *chn_r = NULL;
	uint16_t *chn_b = NULL;
	uint16_t *chn_gr = NULL;
	uint16_t *chn_gb = NULL;

	if (NULL == target->data || target->size < sizeof(*lsc_map) + gain_size + crc_len) {
		return RANDOM_ERROR;
	}

	lsc_map->version = RANDOM_LSC_BLOCK_VERSION;
	lsc_map->algorithm_version = lsc_info->alg_version;
	lsc_map->bayer_pattern = lsc_info->bayer_pattern;
	lsc_map->compress_flag = lsc_info->compress;
	lsc_map->gain_width = lsc_info->gain_width;
	lsc_map->gain_height = lsc_info->gain_height;
	lsc_map->grid_width = lsc_info->grid_width;
	lsc_map->grid_height = lsc_info->grid_height;
	lsc_map->image_width = lsc_info->img_width;
	lsc_map->image_height = lsc_info->img_height;
	lsc_map->optical_x = lsc_info->center_x;
	lsc_map->optical_y = lsc_info->center_y;
	lsc_map->percent = lsc_info->percent;
	lsc_map->gain_num = lsc_info->gain_width * lsc_info->gain_height * 4;
	lsc_map->data_length = sizeof(*lsc_map) + gain_size;

	data_start = (uint8_t *)lsc_map + sizeof(*lsc_map);

	memcpy(data_start, lsc_info->chn_gain[0], lsc_info->chn_gain_size);
	data_start += lsc_info->chn_gain_size;
	memcpy(data_start, lsc_info->chn_gain[1], lsc_info->chn_gain_size);
	data_start += lsc_info->chn_gain_size;
	memcpy(data_start, lsc_info->chn_gain[2], lsc_info->chn_gain_size);
	data_start += lsc_info->chn_gain_size;
	memcpy(data_start, lsc_info->chn_gain[3], lsc_info->chn_gain_size);
	data_start += lsc_info->chn_gain_size;

	//CRC, 4 bytes
	*(uint32_t *)data_start = 0;
	data_start += crc_len;

	*real_size = data_start - (uint8_t *)lsc_map;

	return RANDOM_SUCCESS;
}


static int32_t _awb_pack(struct random_awb_info *awb_info, struct data_info *target, uint32_t *real_size)
{
	struct random_map_awb *awb_map = (struct random_map_awb *)target->data;
	uint8_t *data_start = (uint8_t *)awb_map;
	uint32_t crc_len = 4;

	if (NULL == target->data || target->size < sizeof(*awb_map) + crc_len) {
		return RANDOM_ERROR;
	}

	awb_map->version = RANDOM_AWB_BLOCK_VERSION;
	awb_map->avg_r = awb_info->avg_r;
	awb_map->avg_g = awb_info->avg_g;
	awb_map->avg_b = awb_info->avg_b;
	awb_map->reserved = 0;

	data_start += sizeof(*awb_map);

	//CRC, 4 bytes
	*(uint32_t *)data_start = 0;
	data_start += crc_len;

	*real_size = data_start - (uint8_t *)awb_map;

	return RANDOM_SUCCESS;
}

int32_t get_random_pack_size(uint32_t chn_gain_size, uint32_t *size)
{
	uint32_t crc_len = 4;
	uint32_t gain_size = chn_gain_size * 4;
	uint32_t awb_size = sizeof(struct random_map_awb) + crc_len;
	uint32_t lsc_size = sizeof(struct random_map_lsc) + gain_size + crc_len;
	uint32_t header_size = sizeof(struct random_header);
	uint32_t block_info_size = sizeof(struct random_block_info) * 2;
	uint32_t end_len = 4;

	uint32_t total_size = header_size + block_info_size + awb_size + lsc_size + end_len;

	if (NULL == size || 0 == chn_gain_size)
		return RANDOM_ERROR;

	*size = total_size;

	return RANDOM_SUCCESS;
}

int32_t random_pack(struct random_pack_param *param, struct random_pack_result *result)
{
	int32_t rtn = RANDOM_ERROR;
	struct data_info awb = {0};
	struct data_info lsc = {0};
	uint8_t *target_cur = NULL;
	uint8_t *data_cur = NULL;
	uint8_t *target_end = NULL;
	uint32_t real_size = 0;
	struct random_header *header = NULL;
	struct random_block_info *block_info = NULL;
	uint32_t end_len = 4;

	if (NULL == param || NULL == result)
		return RANDOM_ERROR;

	if (NULL == param->target_buf || 0 == param->target_buf_size)
		return RANDOM_ERROR;

	memset(param->target_buf, 0, param->target_buf_size);

	target_cur = (uint8_t *)param->target_buf;
	target_end = target_cur + param->target_buf_size;

	//write random header
	header = (struct random_header *)target_cur;
	target_cur += sizeof(*header);

	if (target_cur >= target_end)
		return RANDOM_ERROR;

	header->block_num = 2;
	header->verify = RANDOM_START;
	header->version = RANDOM_VERSION;

	//write block info
	block_info = (struct random_block_info *)target_cur;
	target_cur += header->block_num * sizeof(*block_info);

	if (target_cur >= target_end)
		return RANDOM_ERROR;

	//awb pack
	awb.data = target_cur;
	awb.size = target_end - target_cur;
	rtn = _awb_pack(&param->awb_info, &awb, &real_size);
	if (RANDOM_SUCCESS != rtn)
		return RANDOM_ERROR;

	awb.size = real_size;
	block_info[0].id = RANDOM_AWB_BLOCK_ID;
	block_info[0].offset = target_cur - (uint8_t *)header;
	block_info[0].size = awb.size;
	target_cur += awb.size;

	//lsc pack
	lsc.data = target_cur;
	lsc.size = target_end - target_cur;
	rtn = _lsc_pack(&param->lsc_info, &lsc, &real_size);
	if (RANDOM_SUCCESS != rtn)
		return RANDOM_ERROR;

	lsc.size = real_size;
	block_info[1].id = RANDOM_LSC_BLOCK_ID;
	block_info[1].offset = target_cur - (uint8_t *)header;
	block_info[1].size = lsc.size;
	target_cur += lsc.size;

	if (target_cur + end_len > target_end)
		return RANDOM_ERROR;

	/*write end*/
	*(uint32_t *)target_cur = RANDOM_END;
	target_cur += end_len;

	result->real_size = target_cur - (uint8_t *)header;

	rtn = RANDOM_SUCCESS;

	return rtn;
}
