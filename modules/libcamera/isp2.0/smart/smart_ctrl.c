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

#include "smart_ctrl.h"
#include "isp_log.h"
#include "debug_file.h"
#include <stdio.h>
#include <cutils/properties.h>

#define ISP_SMART_MAGIC_FLAG 0xf7758521
#define array_size(array) (sizeof(array) / sizeof(array[0]))

#define DEBUG_BUF_SIZE (5 * 1024)
#define DEBUG_FILE_NAME "/data/mlog/smart.txt"
#define UNUSED(param) (void)(param)

struct block_name_map {
	uint32_t block_id;
	char name[8];
};

struct tuning_param {
	uint32_t version;
	uint32_t bypass;
	struct isp_smart_param param;
};

static const char *s_smart_block_name[] = {
	"lsc",
	"hue_sat",
	"cmc",
	"sat_depr",
	"hsv",
	"ctm",
	"edge",
	"pref",
	"uv_cdn",
	"gamma",
	"gain",
	"wavelet",
	"bpc",
	"nlm",
	"rgb_precdn",
	"yuv_precdn",
	"uv_postcdn",
	"iir",
	"bdn",
	"uvdiv",
	"af",
	"cfae",
	"grgb",
	"iir_yrandom",
	"y_afm",
	"unknown"
};

struct smart_context {
	isp_u32 magic_flag;
	pthread_mutex_t status_lock;
	struct tuning_param tuning_param[SMART_MAX_WORK_MODE];
	struct tuning_param tuning_param_org[SMART_MAX_WORK_MODE];
	struct tuning_param *cur_param;
	uint32_t work_mode;
	enum smart_ctrl_flash_mode flash_mode;
	struct smart_block_result calc_result[ISP_SMART_MAX_BLOCK_NUM];
	struct smart_block_result block_result;
	uint8_t debug_buf[DEBUG_BUF_SIZE];
	debug_handle_t debug_file;
};

static int32_t is_print_log(void)
{
	char value[PROPERTY_VALUE_MAX] = { 0 };
	uint32_t is_print = 0;

	property_get("debug.camera.isp.smart", value, "0");

	if (!strcmp(value, "1")) {
		is_print = 1;
	}

	return is_print;
}

static int32_t check_handle_validate(smart_handle_t handle)
{
	int32_t ret = ISP_SUCCESS;
	struct smart_context *cxt_ptr = (struct smart_context *)handle;

	if (NULL == handle) {
		ISP_LOGE("handle is invalidated\n");
		return ISP_ERROR;
	}

	if (ISP_SMART_MAGIC_FLAG != cxt_ptr->magic_flag) {
		ISP_LOGE("handle is invalidated\n");
		return ISP_ERROR;
	}

	return ret;
}

static int32_t smart_ctl_set_workmode(struct smart_context *cxt, void *in_param)
{
	int32_t rtn = ISP_SUCCESS;
	uint32_t work_mode = *(uint32_t *) in_param;

	if (cxt->tuning_param[work_mode].param.block_num > 0) {
		cxt->work_mode = work_mode;
	} else {
		cxt->work_mode = ISP_MODE_ID_COMMON;
	}

	return rtn;
}

static int32_t smart_ctl_set_flash(struct smart_context *cxt, void *in_param)
{
	int32_t rtn = ISP_SUCCESS;
	enum smart_ctrl_flash_mode flash_mode = 0x0;

	if (NULL == in_param) {
		ISP_LOGE("invalid in param!");
		return ISP_ERROR;
	}

	flash_mode = *(enum smart_ctrl_flash_mode *)in_param;

	if (flash_mode >= SMART_CTRL_FLASH_END) {
		ISP_LOGE("invalid flash mode!");
		return ISP_ERROR;
	}
	cxt->flash_mode = flash_mode;

	return rtn;
}

static int32_t smart_ctl_check_block_param(struct isp_smart_block_cfg *blk_cfg)
{
	int32_t rtn = ISP_SUCCESS;
	struct isp_smart_component_cfg *comp_cfg = NULL;
	struct isp_piecewise_func *func = NULL;
	struct isp_range *bv_range = NULL;
	uint32_t i = 0;
	uint32_t j = 0;
	uint32_t k = 0;

	for (i = 0; i < blk_cfg->component_num; i++) {
		comp_cfg = &blk_cfg->component[i];

		if (blk_cfg->smart_id >= ISP_SMART_MAX) {
			ISP_LOGI("block[%d]: smart_id is invalid.\n", i);
			rtn = ISP_ERROR;
			return rtn;
		}

		if (blk_cfg->block_id >= ISP_BLK_ID_MAX) {
			ISP_LOGI("block[%d]: block_id is invalid.\n", i);
			rtn = ISP_ERROR;
			return rtn;
		}

		for (k = 0; k < comp_cfg->section_num; k++) {
			func = &comp_cfg->func[k];
			bv_range = &comp_cfg->bv_range[k];

			if (bv_range->min > bv_range->max) {
				ISP_LOGV("section[%d]: bv_range is invalid.\n", k);
				rtn = ISP_ERROR;
				return rtn;
			}

			if ((k < comp_cfg->section_num - 1) && (bv_range->max > comp_cfg->bv_range[k + 1].min)) {
				ISP_LOGI("section[%d]: bv_range.max=%d, section[%d]: bv_range.min=%d, bv_range is invalid.",
				     k, bv_range->max, k + 1, comp_cfg->bv_range[k + 1].min);
				rtn = ISP_ERROR;
				return rtn;
			}
		}
	}

	return rtn;
}

static int32_t smart_ctl_check_param(struct isp_smart_param *param)
{
	int32_t rtn = ISP_SUCCESS;
	uint32_t i = 0;
	uint32_t j = 0;
	struct isp_smart_block_cfg *blk_cfg = NULL;
	struct isp_smart_component_cfg *comp_cfg = NULL;
	struct isp_piecewise_func *func = NULL;
	struct isp_range *bv_range = NULL;
	char value = 0;

	ISP_LOGV("block_num:%d.", param->block_num);

	if (0 == is_print_log())
		return rtn;

	for (i = 0; i < param->block_num; i++) {
		blk_cfg = &param->block[i];

		ISP_LOGI("block[%d], smart id=%x, block id=%x, enable=%d, comp num=%d",
			 i, blk_cfg->smart_id, blk_cfg->block_id, blk_cfg->enable, blk_cfg->component_num);

		if (blk_cfg->smart_id > ISP_SMART_MAX) {
			ISP_LOGI("block[%d]: smart_id is invalid.\n", i);
		}

		if (blk_cfg->block_id > ISP_BLK_ID_MAX) {
			ISP_LOGI("block[%d]: block_id is invalid.\n", i);
		}

		for (j = 0; j < blk_cfg->component_num; j++) {
			uint32_t k = 0;
			uint32_t m = 0;

			comp_cfg = &blk_cfg->component[j];

			ISP_LOGI(" component[%d], section num=%d", j, comp_cfg->section_num);

			if (ISP_SMART_X_TYPE_BV == comp_cfg->x_type) {
				ISP_LOGI(" x_type: bv");
			} else if (ISP_SMART_X_TYPE_BV_GAIN == comp_cfg->x_type) {
				ISP_LOGI(" x_type: bv gain");
			} else if (ISP_SMART_X_TYPE_CT == comp_cfg->x_type) {
				ISP_LOGI(" x_type: ct");
			} else if (ISP_SMART_X_TYPE_BV_CT == comp_cfg->x_type) {
				ISP_LOGI(" x_type: bv ct");
			}

			if (ISP_SMART_Y_TYPE_VALUE == comp_cfg->y_type) {
				ISP_LOGI(" y_type: value");
			} else if (ISP_SMART_Y_TYPE_WEIGHT_VALUE == comp_cfg->y_type) {
				ISP_LOGI(" y_type: weight value");
			}

			ISP_LOGI(" use_flash_value = %d", comp_cfg->use_flash_val);
			ISP_LOGI(" flash_value = %d", comp_cfg->flash_val);

			for (k = 0; k < comp_cfg->section_num; k++) {
				func = &comp_cfg->func[k];
				bv_range = &comp_cfg->bv_range[k];

				ISP_LOGI("  section[%d], bv=[%d, %d], func num=%d",
					 k, comp_cfg->bv_range[k].min, comp_cfg->bv_range[k].max, func->num);

				if (bv_range->min > bv_range->max) {
					ISP_LOGI("  section[%d]: bv_range is invalid.\n", k);
				}

				if ((k + 1 < comp_cfg->section_num) && (bv_range->max > comp_cfg->bv_range[k + 1].min)) {
					ISP_LOGI("  section[%d]: bv_range.max=%d, section[%d]: bv_range.min=%d, bv_range is invalid.",
					     k, bv_range->max, k + 1, comp_cfg->bv_range[k + 1].min);
				}

			}

			if (0 == comp_cfg->section_num) {
				func = &comp_cfg->func[0];
				ISP_LOGI("func num=%d", func->num);
				for (m = 0; m < func->num; m++) {
					ISP_LOGI("    [%d]=(%4d, %4d)", m, func->samples[m].x, func->samples[m].y);
				}
			}
		}
	}

	return rtn;
}

static int32_t smart_ctl_parse_tuning_param(struct smart_tuning_param src[], struct tuning_param dst[], uint32_t num)
{
	int32_t rtn = ISP_SUCCESS;
	uint32_t i = 0;

	for (i = 0; i < num; i++) {
		if (NULL != src[i].data.data_ptr && src[i].data.size > 0) {
			if (0 == src[i].version) {
				rtn = smart_ctl_check_param((struct isp_smart_param *)src[i].data.data_ptr);

				if (ISP_SUCCESS == rtn) {
					dst[i].bypass = src[i].bypass;
					dst[i].version = src[i].version;
					memcpy(&dst[i].param, src[i].data.data_ptr, src[i].data.size);
				}
			}
		}
	}

	return ISP_SUCCESS;
}

static int32_t smart_ctl_get_update_param(struct smart_context *cxt, void *in_param)
{
	int32_t rtn = ISP_SUCCESS;
	uint32_t i = 0;
	struct smart_init_param *param = NULL;

	if (NULL == in_param) {
		ISP_LOGE("input param is validated, in: %p\n", param);
		goto ERROR_EXIT;
	}

	param = (struct smart_init_param *)in_param;

	rtn = smart_ctl_parse_tuning_param(param->tuning_param, cxt->tuning_param, SMART_MAX_WORK_MODE);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("parse tuning param failed: rtn = %d", rtn);
		goto ERROR_EXIT;
	}

	cxt->cur_param = &cxt->tuning_param[cxt->work_mode];

ERROR_EXIT:

	return rtn;
}

static int32_t smart_ctl_piecewise_func_v1(struct isp_piecewise_func *func, int32_t x,
				  uint32_t weight_unit, struct isp_weight_value *result)
{
	int32_t rtn = ISP_SUCCESS;
	uint32_t num = func->num;
	struct isp_sample *samples = func->samples;
	int16_t y = 0;
	uint32_t i = 0;

	if (0 == num)
		return ISP_ERROR;

	if (x <= samples[0].x) {
		y = samples[0].y;
		result->value[0] = y;
		result->value[1] = y;
		result->weight[0] = weight_unit;
		result->weight[1] = 0;
	} else if (x >= samples[num - 1].x) {
		y = samples[num - 1].y;
		result->value[0] = y;
		result->value[1] = y;
		result->weight[0] = weight_unit;
		result->weight[1] = 0;
	} else {
		rtn = ISP_ERROR;

		for (i = 0; i < num - 1; i++) {
			if (x >= samples[i].x && x < samples[i + 1].x) {
				if (0 != samples[i + 1].x - samples[i].x) {
					result->value[0] = samples[i].y;
					result->value[1] = samples[i + 1].y;

					result->weight[0] =
					    (samples[i + 1].x - x) * weight_unit / (samples[i + 1].x - samples[i].x);
					result->weight[1] = weight_unit - result->weight[0];
				} else {
					result->value[0] = samples[i].y;
					result->value[1] = samples[i].y;
					result->weight[0] = weight_unit;
					result->weight[1] = 0;
				}

				rtn = ISP_SUCCESS;
				break;
			}
		}
	}

	return rtn;
}

static int32_t smart_ctl_piecewise_func_v0(struct isp_piecewise_func *func, int32_t x, int32_t * result)
{
	int32_t rtn = ISP_SUCCESS;
	uint32_t num = func->num;
	struct isp_sample *samples = func->samples;
	int32_t y = 0;
	uint32_t i = 0;

	if (0 == num)
		return ISP_ERROR;

	if (x <= samples[0].x) {
		y = samples[0].y;
	} else if (x >= samples[num - 1].x) {
		y = samples[num - 1].y;
	} else {
		rtn = ISP_ERROR;

		for (i = 0; i < num - 1; i++) {
			if (x >= samples[i].x && x < samples[i + 1].x) {
				if (0 != samples[i + 1].x - samples[i].x)
					y = samples[i].y + (x - samples[i].x) * (samples[i + 1].y - samples[i].y)
					    / (samples[i + 1].x - samples[i].x);
				else
					y = samples[i].y;

				rtn = ISP_SUCCESS;
				break;
			}
		}
	}

	*result = y;

	return rtn;
}

static int32_t smart_crl_calc_func(struct isp_piecewise_func *func, uint32_t y_type, isp_s32 x,
					struct isp_weight_value *result)
{
	int32_t rtn = ISP_SUCCESS;
	struct isp_weight_value weight_value = { {0}, {0} };
	int32_t value = 0;

	switch (y_type) {
	case ISP_SMART_Y_TYPE_VALUE:
		rtn = smart_ctl_piecewise_func_v0(func, x, &value);
		result->value[0] = (int16_t) value;
		result->weight[0] = SMART_WEIGHT_UNIT;
		break;

	case ISP_SMART_Y_TYPE_WEIGHT_VALUE:
		rtn = smart_ctl_piecewise_func_v1(func, x, SMART_WEIGHT_UNIT, result);
		break;

	default:
		rtn = ISP_ERROR;
		break;
	}

	return rtn;
}

static int32_t smart_ctl_calc_bv_section(struct isp_range bv_range[], uint32_t num, int32_t bv,
						struct isp_weight_value *result)
{
	int32_t rtn = ISP_SUCCESS;
	uint16_t bv_distance[2] = { 0, 1 };
	int32_t bv_max = 0;
	int32_t bv_min = 0;
	uint32_t i = 0;
	int32_t bv_cur = 0;

	if (num < 1)
		return ISP_ERROR;

	if (bv <= bv_range[0].max) {
		result->value[0] = 0;
		result->value[1] = 0;
	} else if (bv >= bv_range[num - 1].min) {
		result->value[0] = num - 1;
		result->value[1] = num - 1;
	} else {
		rtn = ISP_ERROR;

		for (i = 0; i < num - 1; i++) {

			if (bv >= bv_range[i].min && bv <= bv_range[i].max) {
				result->value[0] = i;
				result->value[1] = i;
				rtn = ISP_SUCCESS;
				break;
			} else if (bv > bv_range[i].max && bv < bv_range[i + 1].min) {

				/*mixed environment */
				bv_distance[0] = bv - bv_range[i].max;
				bv_distance[1] = bv_range[i + 1].min - bv;
				result->value[0] = i;
				result->value[1] = i + 1;
				rtn = ISP_SUCCESS;
				break;
			}
		}
	}

	if (ISP_SUCCESS == rtn) {
		/*calc weight value for mixed environment */
		result->weight[0] = bv_distance[1] * SMART_WEIGHT_UNIT / (bv_distance[0] + bv_distance[1]);
		result->weight[1] = SMART_WEIGHT_UNIT - result->weight[0];
	}

	return rtn;
}

static int32_t smart_ctl_calc_component(struct isp_smart_component_cfg * cfg, int32_t bv, int32_t bv_gain, uint32_t ct,
			struct smart_component_result * result)
{
	int32_t rtn = ISP_SUCCESS;
	uint32_t i = 0;
	uint32_t section_num = cfg->section_num;
	struct isp_range *bv_range = cfg->bv_range;
	struct isp_weight_value func_result = { {0}, {0} };
	struct isp_weight_value *fix_data = (struct isp_weight_value *)result->fix_data;

	switch (cfg->x_type) {
	case ISP_SMART_X_TYPE_BV:
		rtn = smart_crl_calc_func(&cfg->func[0], cfg->y_type, bv, &func_result);
		break;

	case ISP_SMART_X_TYPE_BV_GAIN:
		rtn = smart_crl_calc_func(&cfg->func[0], cfg->y_type, bv_gain, &func_result);
		break;

	case ISP_SMART_X_TYPE_CT:
		rtn = smart_crl_calc_func(&cfg->func[0], cfg->y_type, ct, &func_result);
		break;

	case ISP_SMART_X_TYPE_BV_CT:
		{
			struct isp_weight_value bv_result = { {0}, {0} };
			struct isp_weight_value tmp_result[2] = { {{0}, {0}}, {{0}, {0}} };

			rtn = smart_ctl_calc_bv_section(cfg->bv_range, cfg->section_num, bv, &bv_result);
			if (ISP_SUCCESS != rtn) {
				return rtn;
			}

			for (i = 0; i < 2; i++) {
				/*bv result return the index of the bv section */
				uint32_t bv_idx = (uint32_t) bv_result.value[i];
				uint32_t bv_weight = bv_result.weight[i];

				if (bv_weight > 0 && bv_idx < cfg->section_num) {
					rtn = smart_crl_calc_func(&cfg->func[bv_idx], cfg->y_type, ct, &tmp_result[i]);
					if (ISP_SUCCESS != rtn)
						return rtn;
				}
			}

			if (ISP_SMART_Y_TYPE_VALUE == cfg->y_type) {
				int32_t sum = tmp_result[0].value[0] * bv_result.weight[0]
				    + tmp_result[1].value[0] * bv_result.weight[1];
				int32_t weight = bv_result.weight[0] + bv_result.weight[1];

				if (weight > 0)
					func_result.value[0] = sum / weight;
				else
					rtn = ISP_ERROR;
			} else if (ISP_SMART_Y_TYPE_WEIGHT_VALUE == cfg->y_type) {
				if (0 == bv_result.weight[1]) {
					func_result = tmp_result[0];
				} else if (0 == bv_result.weight[0]) {
					func_result = tmp_result[1];
				} else {
					for (i = 0; i < 2; i++) {
						if (tmp_result[i].weight[0] > tmp_result[i].weight[1])
							func_result.value[i] = tmp_result[i].value[0];
						else
							func_result.value[i] = tmp_result[i].value[1];
					}

					func_result.weight[0] = bv_result.weight[0];
					func_result.weight[1] = bv_result.weight[1];
				}
			} else {
				rtn = ISP_ERROR;
			}
		}
		break;

	default:
		rtn = ISP_ERROR;
		break;

	}

	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("calc component faile, x type=%d", cfg->x_type);
		return ISP_ERROR;
	}

	switch (cfg->y_type) {
	case ISP_SMART_Y_TYPE_VALUE:
		result->size = sizeof(int32_t);
		result->fix_data[0] = func_result.value[0];
		ISP_LOGV("value = %d", result->fix_data[0]);
		break;

	case ISP_SMART_Y_TYPE_WEIGHT_VALUE:
		result->size = sizeof(func_result);
		fix_data->weight[0] = func_result.weight[0];
		fix_data->weight[1] = func_result.weight[1];
		fix_data->value[0] = func_result.value[0];
		fix_data->value[1] = func_result.value[1];
		if (1 == is_print_log()) {
			ISP_LOGI("value=(%d, %d), weight=(%d, %d)", fix_data->value[0],
				 fix_data->value[1], fix_data->weight[0], fix_data->weight[1]);
		}
		break;

	default:
		rtn = ISP_ERROR;
		break;
	}

	result->type = cfg->y_type;

	return rtn;
}

static int32_t smart_ctl_calc_component_flash(struct isp_smart_component_cfg * cfg, struct smart_component_result * result)
{
	int32_t rtn = ISP_SUCCESS;
	struct isp_weight_value func_result = { {0}, {0} };
	struct isp_weight_value *fix_data = (struct isp_weight_value *)result->fix_data;

	switch (cfg->y_type) {
	case ISP_SMART_Y_TYPE_VALUE:
		result->size = sizeof(int32_t);
		result->fix_data[0] = cfg->flash_val;
		ISP_LOGI("value = %d", result->fix_data[0]);
		break;

	case ISP_SMART_Y_TYPE_WEIGHT_VALUE:
		result->size = sizeof(func_result);
		fix_data->weight[0] = 256;
		fix_data->weight[1] = 0;
		fix_data->value[0] = cfg->flash_val;
		fix_data->value[1] = 0;
		ISP_LOGI("value=(%d, %d), weight=(%d, %d)", fix_data->value[0],
			 fix_data->value[1], fix_data->weight[0], fix_data->weight[1]);
		break;

	default:
		rtn = ISP_ERROR;
		break;

	}

	result->type = cfg->y_type;

	return rtn;
}

static int32_t smart_ctl_calc_block(struct isp_smart_block_cfg * cfg, int32_t bv, uint32_t bv_gain, uint32_t ct,
		    struct smart_block_result * result, enum smart_ctrl_flash_mode flash_mode)
{
	int32_t rtn = ISP_SUCCESS;
	uint32_t i = 0;
	struct smart_component_result component_result = { 0 };
	uint32_t component_num = cfg->component_num;
	uint32_t update_num = 0;

	if (0 == cfg->enable)
		return ISP_SUCCESS;

	rtn = smart_ctl_check_block_param(cfg);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGV("ISP_TAG: block param check failed.\n");
		return rtn;
	}
	component_num = component_num > ISP_SMART_MAX_VALUE_NUM ? ISP_SMART_MAX_VALUE_NUM : component_num;

	if (1 == is_print_log()) {
		ISP_LOGI("ISP_TAG: use_flash_val = %d. block_id = %d. smart_id = %d.\n",
			 cfg->component[i].use_flash_val, cfg->block_id, cfg->smart_id);
	}

	for (i = 0; i < component_num; i++) {
		if (SMART_CTRL_FLASH_MAIN != flash_mode || 1 != cfg->component[i].use_flash_val) {
			if (1 == is_print_log()) {
				ISP_LOGI("ISP_TAG: flash_mode = %d, use_flash_val = %d. bv = %d, ct = %d\n", flash_mode,
					 cfg->component[i].use_flash_val, bv, ct);
			}
			rtn = smart_ctl_calc_component(&cfg->component[i], bv, bv_gain, ct, &component_result);
		} else {
			rtn = smart_ctl_calc_component_flash(&cfg->component[i], &component_result);
		}

		if (ISP_SUCCESS == rtn) {
			result->component[i] = component_result;
			result->update = 1;
			update_num++;
		}
	}

	if (update_num > 0) {
		result->block_id = cfg->block_id;
		result->component_num = update_num;
		result->update = 1;
		result->smart_id = cfg->smart_id;
	}

	/*always return success for the update will be 0 if any error occured */
	return ISP_SUCCESS;
}

static const char *smart_ctl_find_block_name(uint32_t smart_id)
{
	if (smart_id >= ISP_SMART_MAX)
		smart_id = ISP_SMART_MAX;

	return s_smart_block_name[smart_id];
}

static void smart_ctl_print_debug_file(debug_handle_t debug_file, struct smart_calc_param *calc_param,
			      uint32_t mode, struct smart_calc_result *result, char *debug_buf)
{
	struct smart_block_result *blk = NULL;
	struct smart_component_result *comp = NULL;
	struct isp_weight_value *weight_value = NULL;
	uint32_t i = 0, j = 0, k = 0;
	const char *block_name = NULL;
	int32_t fd = 0;
	int32_t rtn = ISP_SUCCESS;
	char value[PROPERTY_VALUE_MAX] = { 0 };

	property_get("persist.sys.isp.smartdebug", value, "0");

	if (!strcmp(value, "0"))
		return;

	rtn = smart_debug_file_open(debug_file);
	if (ISP_SUCCESS != rtn)
		return;

	sprintf(debug_buf, "bv=%d, ct=%d, bv_gain=%d", calc_param->bv, calc_param->ct, calc_param->bv_gain);
	smart_debug_file_print(debug_file, debug_buf);

	for (i = 0; i < result->counts; i++) {
		blk = &result->block_result[i];

		if (!blk->update)
			continue;

		block_name = smart_ctl_find_block_name(blk->smart_id);
		sprintf(debug_buf, "%s: id=%d, m=%d", block_name, blk->block_id, mode);
		smart_debug_file_print(debug_file, debug_buf);

		for (j = 0; j < blk->component_num; j++) {
			comp = &blk->component[j];

			switch (comp->type) {
			case ISP_SMART_Y_TYPE_VALUE:
				sprintf(debug_buf, "[%d]:val=%d", j, comp->fix_data[0]);
				break;

			case ISP_SMART_Y_TYPE_WEIGHT_VALUE:
				weight_value = (struct isp_weight_value *)comp->fix_data;
				sprintf(debug_buf, "[%d]:val=(%d, %d), w=(%d, %d)", j,
					weight_value->value[0], weight_value->value[1],
					weight_value->weight[0], weight_value->weight[1]);
				break;

			default:
				sprintf(debug_buf, "unknown y type");
				break;
			}

			smart_debug_file_print(debug_file, debug_buf);
		}
	}

	smart_debug_file_close(debug_file);
}

static void smart_ctl_print_smart_result(uint32_t mode, struct smart_calc_result *result)
{
	struct smart_block_result *blk = NULL;
	struct smart_component_result *comp = NULL;
	struct isp_weight_value *weight_value = NULL;
	struct smart_context *cxt = NULL;
	uint32_t i = 0, j = 0, k = 0;
	const char *block_name = NULL;

	ISP_LOGV("block num = %d", result->counts);

	for (i = 0; i < result->counts; i++) {
		blk = &result->block_result[i];
		block_name = smart_ctl_find_block_name(blk->smart_id);

		ISP_LOGV("block[%d]: %s, block_id=0x%x, smart_id=%d, update=%d, mode=%d",
			 i, block_name, blk->block_id, blk->smart_id, blk->update, mode);

		if (!blk->update)
			continue;

		for (j = 0; j < blk->component_num; j++) {
			comp = &blk->component[j];

			switch (comp->type) {
			case ISP_SMART_Y_TYPE_VALUE:
				ISP_LOGV(" component[%d]: value=%d", j, comp->fix_data[0]);
				break;

			case ISP_SMART_Y_TYPE_WEIGHT_VALUE:
				weight_value = (struct isp_weight_value *)comp->fix_data;
				ISP_LOGV(" component[%d]: value=(%d, %d), weight=(%d, %d)", j,
					 weight_value->value[0], weight_value->value[1],
					 weight_value->weight[0], weight_value->weight[1]);
				break;
			}
		}
	}
}

smart_handle_t smart_ctl_init(struct smart_init_param *param, void *result)
{
	int32_t rtn = ISP_SUCCESS;
	uint32_t i = 0;
	smart_handle_t handle = NULL;
	struct smart_context *cxt = NULL;

	if (NULL == param) {
		ISP_LOGE("input is validated, in: %p, out: %p\n", param, result);
		goto param_failed;
	}

	/* create isp_smart_context handle. */
	cxt = (struct smart_context *)malloc(sizeof(struct smart_context));
	if (NULL == cxt) {
		ISP_LOGE("malloc failed, size: %d\n", sizeof(struct smart_context));
		goto malloc_failed;
	}

	/* initial isp_smart_contex is set zeros. */
	memset((void *)cxt, 0x00, sizeof(struct smart_context));

	rtn = smart_ctl_parse_tuning_param(param->tuning_param, cxt->tuning_param, SMART_MAX_WORK_MODE);
	memcpy(cxt->tuning_param_org, cxt->tuning_param, sizeof(cxt->tuning_param_org));
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("parse tuning param failed: rtn = %d", rtn);
		goto parse_tuning_failed;
	}

	cxt->magic_flag = ISP_SMART_MAGIC_FLAG;
	cxt->work_mode = 0;
	cxt->flash_mode = SMART_CTRL_FLASH_CLOSE;
	cxt->cur_param = &cxt->tuning_param[cxt->work_mode];
	cxt->debug_file = smart_debug_file_init(DEBUG_FILE_NAME, "wt");
	handle = (smart_handle_t) cxt;

	return handle;

parse_tuning_failed:
	free(cxt);
	cxt = NULL;
malloc_failed:
param_failed:
	return handle;
}

int32_t smart_ctl_calculation(smart_handle_t handle, struct smart_calc_param * param, struct smart_calc_result * result)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i = 0, mod_num = 0, func_num = 0;
	struct smart_context *cxt = NULL;
	struct tuning_param *cur_param = NULL;
	struct isp_smart_param *smart_param = NULL;
	uint32_t update_block_num = 0;
	enum smart_ctrl_flash_mode flash_mode = SMART_CTRL_FLASH_CLOSE;
	uint32_t cmd = ISP_SMART_IOCTL_SET_FLASH_MODE;

	rtn = check_handle_validate(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("input handle is invalidate,rtn:%d\n", rtn);
		rtn = ISP_ERROR;

		return rtn;
	}

	cxt = (struct smart_context *)handle;
	pthread_mutex_lock(&cxt->status_lock);
	ISP_LOGV("SMART_TAG: smart work mode = %d cxt->cur_param = %p", cxt->work_mode, cxt->cur_param);
	if (ISP_MODE_ID_PRV_0 == cxt->work_mode)
		cxt->cur_param = &cxt->tuning_param[ISP_MODE_ID_COMMON];
	else
		cxt->cur_param = &cxt->tuning_param[cxt->work_mode];

	cur_param = cxt->cur_param;
	flash_mode = cxt->flash_mode;

	if (1 == cur_param->bypass) {
		rtn = ISP_SUCCESS;
		goto EXIT;
	}

	smart_param = &cur_param->param;

	for (i = 0; i < smart_param->block_num; i++) {
		cxt->block_result.update = 0;
		rtn = smart_ctl_calc_block(&smart_param->block[i],
				param->bv, param->bv_gain, param->ct,
				&cxt->block_result,
				flash_mode);

		if (1 == cxt->block_result.update) {
			cxt->calc_result[update_block_num] = cxt->block_result;
			update_block_num++;
		}
	}

	result->counts = update_block_num;
	result->block_result = cxt->calc_result;

	ISP_LOGV("bv=%d, ct=%d, flash=%d", param->bv, param->ct, flash_mode);
	smart_ctl_print_smart_result(cxt->flash_mode, result);
	smart_ctl_print_debug_file(cxt->debug_file, param, cxt->flash_mode, result, (char *)cxt->debug_buf);

EXIT:

	pthread_mutex_unlock(&cxt->status_lock);
	return rtn;
}

int32_t smart_ctl_deinit(smart_handle_t handle, void *param, void *result)
{
	UNUSED(param);
	UNUSED(result);
	isp_s32 rtn = ISP_SUCCESS;
	struct smart_context *cxt = NULL;

	rtn = check_handle_validate(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("deinit check handle failed, rtn = %d\n", rtn);
		rtn = ISP_ERROR;
		goto ERROR_EXIT;
	}

	cxt = (struct smart_context *)handle;
	if (NULL != cxt->debug_file) {
		smart_debug_file_deinit(cxt->debug_file);
		cxt->debug_file = NULL;
	}

	pthread_mutex_destroy(&cxt->status_lock);
	memset(cxt, 0, sizeof(*cxt));
	free(cxt);
	cxt = NULL;

ERROR_EXIT:
	return rtn;
}

int32_t smart_ctl_ioctl(smart_handle_t handle, uint32_t cmd, void *param, void *result)
{
	UNUSED(result);
	isp_s32 rtn = ISP_SUCCESS;
	struct smart_context *cxt_ptr = NULL;

	rtn = check_handle_validate(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("deinit check handle failed, rtn = %d\n", rtn);
		return rtn;
	}

	cxt_ptr = (struct smart_context *)handle;
	pthread_mutex_lock(&cxt_ptr->status_lock);

	switch (cmd) {
	case ISP_SMART_IOCTL_SET_WORK_MODE:
		rtn = smart_ctl_set_workmode(cxt_ptr, param);
		break;

	case ISP_SMART_IOCTL_SET_FLASH_MODE:
		rtn = smart_ctl_set_flash(cxt_ptr, param);
		break;

	case ISP_SMART_IOCTL_GET_UPDATE_PARAM:
		rtn = smart_ctl_get_update_param(cxt_ptr, param);
		break;

	default:
		ISP_LOGE("isp_smart_ctl_ioctl:invalid cmd = %d", cmd);
		rtn = ISP_ERROR;
		break;
	}

ERROR_EXIT:

	pthread_mutex_unlock(&cxt_ptr->status_lock);
	return rtn;
}

int32_t smart_ctl_block_eb(smart_handle_t handle, void *block_eb, uint32_t is_eb)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i = 0;
	int16_t *block_eb_ptr = (int16_t *) block_eb;
	struct smart_context *cxt = NULL;
	struct tuning_param *cur_param = NULL;
	struct isp_smart_param *smart_param = NULL;

	rtn = check_handle_validate(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGI("input handle is null");
		return ISP_SUCCESS;
	}

	cxt = (struct smart_context *)handle;

	if (ISP_MODE_ID_PRV_0 == cxt->work_mode) {
		cxt->cur_param = &cxt->tuning_param[ISP_MODE_ID_COMMON];
	} else {
		cxt->cur_param = &cxt->tuning_param[cxt->work_mode];
	}

	cur_param = cxt->cur_param;

	if (1 == cur_param->bypass) {
		ISP_LOGI("current paramter is bypass");
		return ISP_SUCCESS;
	}

	smart_param = &cur_param->param;

	if (ISP_SMART_MAX < smart_param->block_num) {
		ISP_LOGI("smart block number error:%d", smart_param->block_num);
		return ISP_SUCCESS;
	}

	for (i = 0; i < smart_param->block_num; i++) {
		if (ISP_SMART_LNC == smart_param->block[i].smart_id
		    || ISP_SMART_CMC == smart_param->block[i].smart_id
		    || ISP_SMART_GAMMA == smart_param->block[i].smart_id) {
			if (is_eb) {
				smart_param->block[i].enable = *block_eb_ptr;
			} else {
				*block_eb_ptr = smart_param->block[i].enable;
				smart_param->block[i].enable = 0;
			}
		}
		block_eb_ptr++;
	}

	return rtn;
}

int32_t smart_ctl_block_enable_recover(smart_handle_t handle, uint32_t smart_id)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i = 0;
	struct smart_context *cxt = NULL;
	struct tuning_param *cur_param = NULL;
	struct tuning_param *org_param = NULL;
	struct isp_smart_param *smart_param = NULL;

	rtn = check_handle_validate(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGI("input handle is null");
		return ISP_SUCCESS;
	}

	cxt = (struct smart_context *)handle;

	if (ISP_MODE_ID_PRV_0 == cxt->work_mode) {
		cxt->cur_param = &cxt->tuning_param[ISP_MODE_ID_COMMON];
		org_param = &cxt->tuning_param_org[ISP_MODE_ID_COMMON];
	} else {
		cxt->cur_param = &cxt->tuning_param[cxt->work_mode];
		org_param = &cxt->tuning_param_org[cxt->work_mode];
	}

	cur_param = cxt->cur_param;

	if (1 == cur_param->bypass) {
		ISP_LOGI("current paramter is bypass");
		return ISP_SUCCESS;
	}

	smart_param = &cur_param->param;

	if (ISP_SMART_MAX < smart_param->block_num) {
		ISP_LOGI("smart block number error:%d", smart_param->block_num);
		return ISP_SUCCESS;
	}

	for (i = 0; i < smart_param->block_num; i++) {
		if (smart_id == smart_param->block[i].smart_id) {
			smart_param->block[i].enable = org_param->param.block[i].enable;
			break;
		}
	}

	return rtn;
}

int32_t smart_ctl_block_disable(smart_handle_t handle, uint32_t smart_id)
{
	isp_s32 rtn = ISP_SUCCESS;
	isp_u32 i = 0;
	struct smart_context *cxt = NULL;
	struct tuning_param *cur_param = NULL;
	struct isp_smart_param *smart_param = NULL;

	rtn = check_handle_validate(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGI("input handle is null");
		return ISP_SUCCESS;
	}

	cxt = (struct smart_context *)handle;

	if (ISP_MODE_ID_PRV_0 == cxt->work_mode) {
		cxt->cur_param = &cxt->tuning_param[ISP_MODE_ID_COMMON];
	} else {
		cxt->cur_param = &cxt->tuning_param[cxt->work_mode];
	}

	cur_param = cxt->cur_param;

	if (1 == cur_param->bypass) {
		ISP_LOGI("current paramter is bypass");
		return ISP_SUCCESS;
	}

	smart_param = &cur_param->param;

	if (ISP_SMART_MAX < smart_param->block_num) {
		ISP_LOGI("smart block number error:%d", smart_param->block_num);
		return ISP_SUCCESS;
	}

	for (i = 0; i < smart_param->block_num; i++) {
		if (smart_id == smart_param->block[i].smart_id) {
			smart_param->block[i].enable = 0;
			break;
		}
	}

	return rtn;
}
