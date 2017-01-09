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
#include "random_unpack.h"

#define RANDOM_SUCCESS 0
#define RANDOM_ERROR 0xff

int32_t random_unpack(void *random_data, uint32_t random_size, struct random_info *info)
{
	struct random_header *header = NULL;
	struct random_block_info *block_info = NULL;
	uint8_t *random_end = NULL;
	uint32_t i = 0;
	uint32_t awb_index = 0xff;
	uint32_t awb_offset = 0;
	uint32_t lsc_index = 0xff;
	uint32_t lsc_offset = 0;

	if (NULL == random_data || 0 == random_size || NULL == info) {
		return RANDOM_ERROR;
	}

	random_end = (uint8_t *)random_data + random_size;

	header = (struct random_header *)random_data;

	if (RANDOM_START != header->verify)
		return RANDOM_ERROR;

	if ((uint8_t *)header + sizeof(*header) > random_end)
		return RANDOM_ERROR;

	block_info = (struct random_block_info *)((uint8_t *)header + sizeof(*header));

	if ((uint8_t *)block_info + sizeof(*block_info) * header->block_num > random_end)
		return RANDOM_ERROR;

	for (i=0; i<header->block_num; i++) {

		switch (block_info[i].id) {
		case RANDOM_AWB_BLOCK_ID:
			awb_index = i;
			break;

		case RANDOM_LSC_BLOCK_ID:
			lsc_index = i;
			break;
		}
	}

	if (awb_index >= header->block_num || lsc_index >= header->block_num) {
		return RANDOM_ERROR;
	}

	if (awb_index < header->block_num && lsc_index < header->block_num) {
		info->awb = (void *)((uint8_t *)header + block_info[awb_index].offset);
		info->awb_size = block_info[awb_index].size;

		info->lsc = (void *)((uint8_t *)header + block_info[lsc_index].offset);
		info->lsc_size = block_info[lsc_index].size;
	}

	return RANDOM_SUCCESS;
}

int32_t random_lsc_unpack(void *random_lsc, uint32_t random_lsc_size, struct random_lsc_info *lsc_info)
{
	struct random_map_lsc *basic = NULL;
	uint16_t *gain_area = NULL;
	uint32_t chn_gain_num = 0;
	uint32_t i = 0;

	if (NULL == random_lsc || 0 == random_lsc_size || NULL == lsc_info) {
		return RANDOM_ERROR;
	}

	basic = (struct random_map_lsc *)random_lsc;
	gain_area = (uint8_t *)((uint8_t *)random_lsc + sizeof(struct random_map_lsc));
	chn_gain_num = basic->gain_num;

	lsc_info->gain_width = basic->gain_width;
	lsc_info->gain_height = basic->gain_height;
	lsc_info->chn_gain_size = basic->gain_num * sizeof(uint16_t);

	for (i=0; i<4; i++) {
		lsc_info->chn_gain[i] = gain_area + i * lsc_info->gain_width * lsc_info->gain_height;
	}

	return RANDOM_SUCCESS;
}

int32_t random_awb_unpack(void *random_awb, uint32_t random_awb_size, struct random_awb_info *awb_info)
{
	struct random_map_awb *awb_map = NULL;

	if (NULL == random_awb || 0 == random_awb_size || NULL == awb_info) {
		return RANDOM_ERROR;
	}

	awb_map = (struct random_map_awb *)random_awb;

	awb_info->avg_r = awb_map->avg_r;
	awb_info->avg_g = awb_map->avg_g;
	awb_info->avg_b = awb_map->avg_b;

	return RANDOM_SUCCESS;
}

