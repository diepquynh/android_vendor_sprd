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

#include "isp_type.h"
#include "isp_smart.h"
#include "isp_log.h"
#ifdef WIN32
#include "memory.h"
#include "malloc.h"
#endif

#define ISP_SMART_ALG_MAGIC_FLAG 0xffeeaabb
#define UNUSED(param)  (void)(param)

struct isp_smart_alg_context {
	uint32_t magic_flag;
};

static int32_t _calc_piecewise_func0(struct isp_smart_interplate_piecewise_func *func, int32_t x, int32_t value[2],
				     uint32_t weight[2])
{
	uint32_t num = func->num;
	struct isp_smart_interplate_sample *samples = func->samples;
	int16_t y = 0;
	uint32_t i = 0;

	if (0 == num)
		return y;

	if (x <= samples[0].x) {

		y = samples[0].y;
		value[0] = y;
		value[1] = y;
		weight[0] = ISP_SMART_WEIGHT_UNIT;
		weight[1] = 0;
	} else if (x >= samples[num - 1].x) {

		y = samples[num - 1].y;
		value[0] = y;
		value[1] = y;
		weight[0] = ISP_SMART_WEIGHT_UNIT;
		weight[1] = 0;
	} else {

		for (i = 0; i < num - 1; i++) {

			if (x >= samples[i].x && x < samples[i + 1].x) {

				if (0 != samples[i + 1].x - samples[i].x) {
					value[0] = samples[i].y;
					value[1] = samples[i + 1].y;

					weight[0] =
					    (samples[i + 1].x - x) * ISP_SMART_WEIGHT_UNIT / (samples[i + 1].x -
											      samples[i].x);
					weight[1] = ISP_SMART_WEIGHT_UNIT - weight[0];
				} else {
					value[0] = samples[i].y;
					value[1] = samples[i].y;
					weight[0] = ISP_SMART_WEIGHT_UNIT;
					weight[1] = 0;
				}

				break;
			}
		}
	}

	return y;
}

static int16_t _calc_piecewise_func1(struct isp_smart_interplate_piecewise_func *func, isp_s32 x)
{

	uint32_t num = func->num;
	struct isp_smart_interplate_sample *samples = func->samples;
	int16_t y = 0;
	uint32_t i = 0;

	ISP_LOGI("num=%d", num);

	for (i = 0; i < num; i++)
		ISP_LOGI("sample[%d]=(%d, %d)", i, samples[i].x, samples[i].y);

	if (0 == num)
		return y;

	if (x <= samples[0].x) {

		y = samples[0].y;
	} else if (x >= samples[num - 1].x) {
		y = samples[num - 1].y;
	} else {

		for (i = 0; i < num - 1; i++) {

			if (x >= samples[i].x && x < samples[i + 1].x) {

				if (0 != samples[i + 1].x - samples[i].x)
					y = samples[i].y + (x - samples[i].x) * (samples[i + 1].y - samples[i].y)
					    / (samples[i + 1].x - samples[i].x);
				else
					y = samples[i].y;

				break;
			}
		}
	}

	ISP_LOGI("result=%d", y);

	return y;
}

static struct isp_smart_alg_context *_getcontext_from_hanlde(isp_smart_handle_t handle)
{
	return (struct isp_smart_alg_context *)handle;
}

static int32_t _check_handle_validate(isp_smart_handle_t handle)
{
	int32_t ret = ISP_SUCCESS;
	struct isp_smart_alg_context *cxt_ptr = (struct isp_smart_alg_context *)handle;

	if (ISP_SMART_ALG_MAGIC_FLAG != cxt_ptr->magic_flag) {
		ISP_LOGE("handle is invalidated\n");
		ret = ISP_ERROR;
	}

	return ret;
}

isp_smart_handle_t isp_smart_init(void *in_ptr, void *out_ptr)
{
	UNUSED(out_ptr);
	struct isp_smart_alg_context *alg_cxt_ptr = NULL;

	if (NULL == in_ptr) {
		ISP_LOGE("isp_smart_init: input pointer is invalidated\n");
		goto _smart_init_error_exit;
	}

	alg_cxt_ptr = (struct isp_smart_alg_context *)malloc(sizeof(struct isp_smart_alg_context));
	if (NULL == alg_cxt_ptr) {
		ISP_LOGE("isp_smart_init: malloc0 failed\n");
		goto _smart_init_error_exit;
	}

	memset((void *)alg_cxt_ptr, 0x00, sizeof(struct isp_smart_alg_context));
	alg_cxt_ptr->magic_flag = ISP_SMART_ALG_MAGIC_FLAG;

_smart_init_error_exit:

	return (isp_smart_handle_t) alg_cxt_ptr;
}

int32_t isp_smart_calculation(isp_u32 func_type,
			      struct isp_smart_interplate_piecewise_func * cur_func,
			      void *smart_cur_info_in, void *smart_calc_param_out)
{
	int32_t rtn = ISP_SUCCESS;
	enum isp_smart_interplate_func_type inter_func_type;
	struct isp_smart_interplate_piecewise_func *func = NULL;
	uint32_t i = 0;

/*
	rtn = _check_handle_validate(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("isp smart deinit: check handle faild, rtn= %d\n", rtn);
		rtn = ISP_ERROR;
		goto _smart_calc_error_exit;
	}
*/
	if ((NULL == cur_func)
	    || (NULL == smart_cur_info_in)
	    || (NULL == smart_calc_param_out)) {
		ISP_LOGE("input param pointer is NULL: func:%p, input:%p, output:%p\n", cur_func, smart_cur_info_in,
			 smart_calc_param_out);

		rtn = ISP_ERROR;
		goto _smart_calc_error_exit;
	}

	inter_func_type = func_type;
	func = cur_func;

	ISP_LOGI("isp_smart_adjust_interplate:%d\n", inter_func_type);

	for (i = 0; i < func->num; i++) {
		ISP_LOGI("ISP_SMART: sample[%d]=[%d, %d]", i, func->samples[i].x, func->samples[i].y);
	}

	switch (inter_func_type) {
	case ISP_SMART_INTERPLATE_FUNC0:
		{
			struct isp_smart_interplate_input0 *input_ptr =
			    (struct isp_smart_interplate_input0 *)smart_cur_info_in;
			struct isp_smart_interplate_output0 *output_ptr =
			    (struct isp_smart_interplate_output0 *)smart_calc_param_out;

			rtn = _calc_piecewise_func0(func, input_ptr->x, output_ptr->index, output_ptr->weight);

			ISP_LOGI("ISP_SMART: func=%d, input=%d, output=(%d, %d), (%d, %d)", inter_func_type,
				 input_ptr->x, output_ptr->index[0], output_ptr->index[1], output_ptr->weight[0],
				 output_ptr->weight[1]);
		}
		break;

	case ISP_SMART_INTERPLATE_FUNC1:
		{
			struct isp_smart_interplate_input1 *input_ptr =
			    (struct isp_smart_interplate_input1 *)smart_cur_info_in;
			struct isp_smart_interplate_output0 *output_ptr =
			    (struct isp_smart_interplate_output0 *)smart_calc_param_out;

			output_ptr->index[0] = _calc_piecewise_func1(func, input_ptr->x);
			output_ptr->index[1] = output_ptr->index[0];
			output_ptr->weight[0] = ISP_SMART_WEIGHT_UNIT;
			output_ptr->weight[1] = 0;
			ISP_LOGI("ISP_SMART: func=%d, input=%d, output=(%d, %d), (%d, %d)", inter_func_type,
				 input_ptr->x, output_ptr->index[0], output_ptr->index[1], output_ptr->weight[0],
				 output_ptr->weight[1]);
		}
		break;

	case ISP_SMART_INTERPLATE_FUNC2:
		{
			struct isp_smart_interplate_input1 *input_ptr =
			    (struct isp_smart_interplate_input1 *)smart_cur_info_in;
			struct isp_smart_interplate_output0 *output_ptr =
			    (struct isp_smart_interplate_output0 *)smart_calc_param_out;

			output_ptr->index[0] = _calc_piecewise_func1(func, input_ptr->x);
			output_ptr->index[1] = output_ptr->index[0];
			output_ptr->weight[0] = ISP_SMART_WEIGHT_UNIT;
			output_ptr->weight[1] = 0;
			ISP_LOGI("ISP_SMART: func=%d, input=%d, output=(%d, %d), (%d, %d)", inter_func_type,
				 input_ptr->x, output_ptr->index[0], output_ptr->index[1], output_ptr->weight[0],
				 output_ptr->weight[1]);
		}
		break;

	default:
		ISP_LOGE("func type is invalidated, func type: 0x%x\n", inter_func_type);
		break;
	}

	ISP_LOGI("isp_smart_adjust_interplate: X\n");

_smart_calc_error_exit:

	return rtn;

}

int32_t isp_smart_deinit(isp_smart_handle_t handle)
{
	int32_t rtn = ISP_SUCCESS;

	rtn = _check_handle_validate(handle);
	if (ISP_SUCCESS != rtn) {
		ISP_LOGE("isp smart deinit: check handle faild, rtn= %d\n", rtn);
		return ISP_ERROR;
	}

	ISP_LOGI("isp_smart_deinit: E\n");

	ISP_LOGI("isp_smart_deinit: X\n");

	return rtn;
}
