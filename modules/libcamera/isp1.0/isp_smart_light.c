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
#include "isp_smart_light.h"
#include "isp_awb_debug.h"
#include "isp_awb_queue.h"

#ifdef WIN32
#define ALOGI
#else
#include <utils/Log.h>
#endif
/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------*
*					Micro Define				*
*-------------------------------------------------------------------------------*/
#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "isp_smart_light"
#endif

#ifdef WIN32
#define SMART_LOGE
#define SMART_LOGW
#define SMART_LOGI
#define SMART_LOGD
#define SMART_LOGV
#else
#define SMART_DEBUG_STR     "SMART: %d: "
#define SMART_DEBUG_ARGS    __LINE__

#define SMART_LOGE(format,...) ALOGE(SMART_DEBUG_STR format, SMART_DEBUG_ARGS, ##__VA_ARGS__)
#define SMART_LOGW(format,...) ALOGW(SMART_DEBUG_STR format, SMART_DEBUG_ARGS, ##__VA_ARGS__)
#define SMART_LOGI(format,...) ALOGI(SMART_DEBUG_STR format, SMART_DEBUG_ARGS, ##__VA_ARGS__)
#define SMART_LOGD(format,...) ALOGD(SMART_DEBUG_STR format, SMART_DEBUG_ARGS, ##__VA_ARGS__)
#define SMART_LOGV(format,...) ALOGV(SMART_DEBUG_STR format, SMART_DEBUG_ARGS, ##__VA_ARGS__)
#endif

#define SMART_LIGHT_MAX_HANDLER 		4
#define SMART_LIGHT_INIT_DONE			1
#define SMART_LIGHT_MAX_QUEUE_SIZE		16
#define SMART_LIGHT_BV_INIT_VALUE		100
#define SMART_LIGHT_BVQUEUE_INIT_VALUE 		0
#define SMART_BV_ENABLE	1
#define SMART_BV_UENABLE	0
/*------------------------------------------------------------------------------*
*					local structures			*
*-------------------------------------------------------------------------------*/
struct smart_light_gain_l {
	int16_t r;
	int16_t g;
	int16_t b;
};

struct smart_rgb_queue {
	queue_handle_t qr;
	queue_handle_t qg;
	queue_handle_t qb;
	uint32_t size;
};

struct smart_normal_queue {
	queue_handle_t qvalue;
	uint32_t size;
};

struct smart_bv_calc_param {
	uint32_t bv_enable;
	int32_t cur_bv_value;
	uint32_t bv_abs_thrd;
	uint32_t bv_avg_thrd;
	uint32_t bv_que_avg;
	uint32_t bv_delta_avg;
	uint32_t bv_delta_max;
	uint32_t num_indoor;
	uint32_t num_outdoor;
	struct smart_normal_queue bv_queue;
	struct smart_normal_queue bv_delta_queue;
	enum smart_light_envi_id last_envi_id;

};

struct smart_light_context {
	uint32_t init;
	struct smart_light_init_param init_param;
	struct smart_light_calc_param calc_param;
	struct smart_light_calc_result calc_result;
	debug_handle_t debug_handle;
	debug_handle_t bv_handle;
	char debug_str[1024];
	struct smart_rgb_queue gain_factor_queue;
	struct smart_rgb_queue hue_sat_gain_queue;
	struct smart_normal_queue bv_queue;
	struct smart_normal_queue bv_delta_queue;
	struct smart_rgb_queue envi_queue;
};

static struct smart_light_context s_smart_light_cxt[SMART_LIGHT_MAX_HANDLER] = {{0}};
static const char DEBUG_FILE[] = "/data/mlog/smart.txt";
static const char DEBUG_FILE_OP[] = "w+t";
/*------------------------------------------------------------------------------*
*					local functions				*
*-------------------------------------------------------------------------------*/
static struct smart_light_context *_get_context(uint32_t handler_id)
{
	if (handler_id >= SMART_LIGHT_MAX_HANDLER)
		return NULL;

	return &s_smart_light_cxt[handler_id];
}

static int32_t _adjust_sat_hue_by_gain(uint32_t *r_gain, uint32_t *b_gain, uint32_t clr_t, uint32_t r_coef, uint32_t b_coef)
{
	int32_t i, j;

	int32_t rgain = *r_gain;
	int32_t bgain = *b_gain;
	int32_t MAX_RATGAIN = ((r_coef)*2);
	int32_t MAX_BATGAIN =((b_coef)*2);
	//int32_t MAX_RATGAIN = ((128+18)*2);
	//int32_t MAX_BATGAIN =((128-12)*2);

	int32_t START_TC = 33;
	int32_t END_TC = 26;
	int32_t INTERNAL_TC=( END_TC-START_TC);

	int32_t MAX_RK = (256*(MAX_RATGAIN - 256)/INTERNAL_TC);
	int32_t MAX_BK = (256*(MAX_BATGAIN - 256)/INTERNAL_TC);

	clr_t = clr_t / 100;

	if ((int32_t)clr_t >= START_TC) {
		return 0;
	}

	if((int32_t)clr_t <= END_TC) {
		clr_t = END_TC;
	}

	*r_gain = rgain * ((clr_t-START_TC)* MAX_RK + 256*256)/256/256;
	*b_gain = bgain * ((clr_t-START_TC)* MAX_BK + 256*256)/256/256;

	return 0;
}

static void _deinit_rgb_queue(struct smart_rgb_queue *queue)
{
	if (0 != queue->qr) {
		queue_deinit(queue->qr);
		queue->qr = 0;
	}

	if (0 != queue->qg) {
		queue_deinit(queue->qg);
		queue->qg = 0;
	}

	if (0 != queue->qb) {
		queue_deinit(queue->qb);
		queue->qb = 0;
	}
}



static void _rgb_queue_add(struct smart_rgb_queue *queue, struct smart_light_gain *gain)
{
	if (0 != queue->qr)
		queue_add(queue->qr, gain->r);

	if (0 != queue->qg)
		queue_add(queue->qg, gain->g);

	if (0 != queue->qb)
		queue_add(queue->qb, gain->b);
}

static void _deinit_normal_queue(struct smart_normal_queue *queue)
{
	if (0 != queue->qvalue) {
		queue_deinit(queue->qvalue);
		queue->qvalue= 0;
	}
}

static void _normal_queue_add(struct smart_normal_queue *queue, uint32_t *gain)
{
	uint32_t value = 0;
	value = *gain;
	if (0 != queue->qvalue)
		queue_add(queue->qvalue, value);
}

static int32_t _init_normal_queue(struct smart_normal_queue *queue, uint32_t size, uint16_t *init_value)
{
	uint32_t value = 0;
	value = *init_value;
	queue->qvalue= queue_init(size);
	if (0 == queue->qvalue)
		goto ERROR_EXIT;

	if (value > 0 && 0 != queue->qvalue)
		queue_add(queue->qvalue, value);

	return SMART_LIGHT_SUCCESS;

ERROR_EXIT:
	_deinit_normal_queue(queue);
	return SMART_LIGHT_ERROR;
}

static void _normal_queue_clear(struct smart_normal_queue *queue)
{
	if (0 != queue->qvalue)
		queue_clear(queue->qvalue);
}

static void _normal_queue_average(struct smart_normal_queue *queue, uint32_t *gain)
{
	if (0 != queue->qvalue)
		*gain = queue_average(queue->qvalue);

}

static void _normal_queue_max(struct smart_normal_queue *queue, uint32_t *gain)
{
	if (0 != queue->qvalue)
		*gain = queue_max(queue->qvalue);
}

static void _normal_queue_delta(struct smart_normal_queue *queue, uint32_t *gain)
{
	if (0 != queue->qvalue)
		*gain = queue_delta(queue->qvalue);

}

static int32_t _init_rgb_queue(struct smart_rgb_queue *queue, uint32_t size, struct smart_light_gain *init_gain)
{
	queue->qr = queue_init(size);
	if (0 == queue->qr)
		goto ERROR_EXIT;

	queue->qg = queue_init(size);
	if (0 == queue->qg)
		goto ERROR_EXIT;

	queue->qb = queue_init(size);
	if (0 == queue->qb)
		goto ERROR_EXIT;

	if (init_gain->r > 0 && init_gain->g > 0 && init_gain->b > 0)
		_rgb_queue_add(queue, init_gain);

	return SMART_LIGHT_SUCCESS;

ERROR_EXIT:
	_deinit_rgb_queue(queue);
	return SMART_LIGHT_ERROR;
}



static void _envi_queue_statis(struct smart_bv_calc_param *bv_param, struct smart_rgb_queue *envi_queue)
{
	if (0 != envi_queue->qr)
		bv_param->num_indoor = queue_statis(envi_queue->qr, (uint32_t)SMART_ENVI_INDOOR_NORMAL) + queue_statis(envi_queue->qr, (uint32_t)SMART_ENVI_LOW_LIGHT);

	if (0 != envi_queue->qg)
		bv_param->num_outdoor = queue_statis(envi_queue->qg, (uint32_t)SMART_ENVI_OUTDOOR_NORMAL) + queue_statis(envi_queue->qg, (uint32_t)SMART_ENVI_OUTDOOR_MIDDLE) + queue_statis(envi_queue->qg, (uint32_t)SMART_ENVI_OUTDOOR_HIGH);
}
static void _rgb_queue_clear(struct smart_rgb_queue *queue)
{
	if (0 != queue->qr)
		queue_clear(queue->qr);

	if (0 != queue->qg)
		queue_clear(queue->qg);

	if (0 != queue->qb)
		queue_clear(queue->qb);
}



static void _rgb_queue_average(struct smart_rgb_queue *queue, struct smart_light_gain *gain)
{
	if (0 != queue->qr)
		gain->r = queue_average(queue->qr);

	if (0 != queue->qg)
		gain->g = queue_average(queue->qg);

	if (0 != queue->qb)
		gain->b = queue_average(queue->qb);

}
static void _rgb_queue_max(struct smart_rgb_queue *queue, struct smart_light_gain *gain)
{
	if (0 != queue->qr)
		gain->r = queue_max(queue->qr);

	if (0 != queue->qg)
		gain->g = queue_max(queue->qg);

	if (0 != queue->qb)
		gain->b = queue_max(queue->qb);
}




static void _rgb_queue_delta(struct smart_rgb_queue *queue, struct smart_light_gain *gain)
{
	if (0 != queue->qr)
		gain->r = queue_delta(queue->qr);

	if (0 != queue->qg)
		gain->g = queue_delta(queue->qg);

	if (0 != queue->qb)
		gain->b = queue_delta(queue->qb);
}



static int16_t _calc_piecewise_func_no_interp(struct smart_light_piecewise_func *func, int16_t x,
							int16_t value[2], uint16_t weight[2])
{
	uint32_t num = func->num;
	struct smart_light_sample *samples = func->samples;
	int16_t y = 0;
	uint32_t i = 0;

	if (0 == num)
		return y;

	if (x <= samples[0].x) {

		y = samples[0].y;
		value[0] = y;
		value[1] = y;
		weight[0] = SMART_WEIGHT_UNIT;
		weight[1] = 0;
	} else if (x >= samples[num - 1].x) {

		y = samples[num - 1].y;
		value[0] = y;
		value[1] = y;
		weight[0] = SMART_WEIGHT_UNIT;
		weight[1] = 0;
	} else {

		for (i=0; i<num-1; i++) {

			if (x >= samples[i].x && x < samples[i+1].x)  {

				if (0 != samples[i+1].x - samples[i].x) {
					value[0] = samples[i].y;
					value[1] = samples[i+1].y;

					weight[0] = (samples[i+1].x - x) * SMART_WEIGHT_UNIT / (samples[i+1].x - samples[i].x);
					weight[1] = SMART_WEIGHT_UNIT - weight[0];
				} else {
					value[0] = samples[i].y;
					value[1] = samples[i].y;
					weight[0] = SMART_WEIGHT_UNIT;
					weight[1] = 0;
				}

				break;
			}
		}
	}

	return y;
}

static int16_t _calc_piecewise_func_interp(struct smart_light_piecewise_func *func, int16_t x)
{

	uint32_t num = func->num;
	struct smart_light_sample *samples = func->samples;
	int16_t y = 0;
	uint32_t i = 0;

	SMART_LOGV("num=%d", num);

	for (i=0; i<num; i++)
		SMART_LOGV("	sample[%d]=(%d, %d)", i, samples[i].x, samples[i].y);

	if (0 == num)
		return y;

	if (x <= samples[0].x) {

		y = samples[0].y;
	} else if (x >= samples[num - 1].x) {
		y = samples[num - 1].y;
	} else {

		for (i=0; i<num-1; i++) {

			if (x >= samples[i].x && x < samples[i+1].x)  {

				if (0 != samples[i+1].x - samples[i].x)
					y = samples[i].y + (x - samples[i].x) * (samples[i+1].y - samples[i].y)
							/ (samples[i+1].x - samples[i].x);
				else
					y = samples[i].y;

				break;
			}
		}
	}

	SMART_LOGV("result=%d", y);

	return y;
}

static enum smart_light_envi_id _gen_mixed_envi_id(enum smart_light_envi_id envi_id0,
							enum smart_light_envi_id envi_id1)
{
	if ((uint32_t)envi_id0 < (uint32_t)envi_id1)
		return (enum smart_light_envi_id)((uint32_t)envi_id0 << 4) | ((uint32_t)envi_id1);
	else
		return (enum smart_light_envi_id)((uint32_t)envi_id1 << 4) | ((uint32_t)envi_id0);
}

static void _parser_mixed_envi_id(enum smart_light_envi_id envi_id, enum smart_light_envi_id result[2])
{
	result[0] = (envi_id >> 4) & 0xf;
	result[1] = (envi_id) & 0xf;
}


static int32_t _smart_bv_adjust(struct smart_bv_calc_param * bv_calc_param, uint32_t quickmode)
{
	uint32_t bv = 0;
	int32_t init_bv = 0;
	init_bv = bv_calc_param->cur_bv_value;

	if (0 > init_bv)
		bv = 0;
	else
		bv = init_bv;

	if (0 != quickmode) {
		_normal_queue_clear(&bv_calc_param->bv_queue);
		_normal_queue_clear(&bv_calc_param->bv_delta_queue);
		SMART_LOGI("gain use quick mode for the first time");
	}

	_normal_queue_add(&bv_calc_param->bv_queue, &bv);
	_normal_queue_average(&bv_calc_param->bv_queue, &bv);
	bv_calc_param->bv_que_avg = bv;

	_normal_queue_delta(&bv_calc_param->bv_queue, &bv);
	_normal_queue_add(&bv_calc_param->bv_delta_queue, &bv);
	_normal_queue_max(&bv_calc_param->bv_delta_queue, &bv);
	bv_calc_param->bv_delta_max = bv;

	_normal_queue_average(&bv_calc_param->bv_delta_queue, &bv);
	bv_calc_param->bv_delta_avg = bv;

	SMART_LOGV("bv_que_avg = %d, bv_delta_avg = %d, bv_delta_max = %d", bv_calc_param->bv_que_avg, bv_calc_param->bv_delta_avg, bv_calc_param->bv_delta_max);

	return SMART_LIGHT_SUCCESS;
}

static int32_t _smart_bv_range(struct smart_bv_calc_param * bv_calc_param, struct smart_light_envi_result *result)
{
	uint32_t delta_max = 0;
	uint32_t delta_avg = 0;
	delta_max = 10;
	delta_avg = 3;
	if (SMART_ENVI_INDOOR_NORMAL == result ->envi_id[0] && SMART_ENVI_OUTDOOR_NORMAL == result ->envi_id[1]) {

		if (bv_calc_param->bv_delta_max < delta_max && bv_calc_param->bv_delta_avg <delta_avg){
			if(bv_calc_param->num_outdoor > bv_calc_param->num_indoor){
				result->envi_id[0] = SMART_ENVI_OUTDOOR_NORMAL;
				result->envi_id[1] = 0;
				result->weight[0] = SMART_WEIGHT_UNIT;
				result->weight[1] = 0;
			}
			if(bv_calc_param->num_outdoor < bv_calc_param->num_indoor){

				result->envi_id[0] = SMART_ENVI_INDOOR_NORMAL;
				result->envi_id[1] = 0;
				result->weight[0] = SMART_WEIGHT_UNIT;
				result->weight[1] = 0;
			}

		}
		SMART_LOGI("smart bv range called!");

	}

	return SMART_LIGHT_SUCCESS;
}
static int32_t _smart_envi_detect(struct smart_light_envi_param *param, int32_t bv, struct smart_light_envi_result *result, struct smart_bv_calc_param *bv_param)
{
	struct smart_light_range_l *envi_bv = param->bv_range;
	enum smart_light_envi_id envi_id[SMART_WEIGHT_NUM] = {SMART_ENVI_COMMON};
	struct smart_light_envi_result envi_result = *result;
	uint16_t bv_distance[2] ={0, 1};
	int32_t bv_max = 0;
	int32_t bv_min = 0;
	uint32_t i = 0;
	int32_t bv_cur = 0;

	for (i=SMART_ENVI_LOW_LIGHT; i<SMART_ENVI_OUTDOOR_HIGH; i++) {

		/*invalid bv range*/
		if (0 == envi_bv[i].min && 0 == envi_bv[i].max)
			continue;

		if ((int32_t)bv >= envi_bv[i].min && (int32_t)bv <= envi_bv[i].max) {
			envi_id[0] = (enum smart_light_envi_id)i;
			break;

		} else if ((int32_t)bv > envi_bv[i].max && (int32_t)bv < envi_bv[i+1].min) {

			/*mixed environment*/
			bv_distance[0] = bv - envi_bv[i].max;
			bv_distance[1] = envi_bv[i+1].min - bv;
			envi_id[0] = i;
			envi_id[1] = i + 1;
		}
	}

	i = SMART_ENVI_OUTDOOR_HIGH;
	if (envi_bv[i].min > 0 && envi_bv[i].max > 0
		&& (int32_t)bv >= envi_bv[i].min && (int32_t)bv <= envi_bv[i].max)
			envi_id[0] = (enum smart_light_envi_id)i;

	/*calc weight value for mixed environment*/
	envi_result.weight[0] = bv_distance[1] * SMART_WEIGHT_UNIT
					/ (bv_distance[0] + bv_distance[1]);
	envi_result.weight[1] = SMART_WEIGHT_UNIT - envi_result.weight[0];

	envi_result.envi_id[0] = envi_id[0];
	envi_result.envi_id[1] = envi_id[1];

	if (SMART_BV_ENABLE == bv_param->bv_enable) {
		SMART_LOGV("smart light --smart bv range enable!");
		_smart_bv_range(bv_param, &envi_result);
	}

	if (envi_result.envi_id[0] != result->envi_id[0] || envi_result.envi_id[1] != result->envi_id[1]
		|| envi_result.weight[0] != result->weight[0] || envi_result.weight[1] != result->weight[1])
		envi_result.update = 1;
	else
		envi_result.update = 0;

	*result = envi_result;

	SMART_LOGV("envi_id=(%d, %d), weight = (%d, %d)", result->envi_id[0], result->envi_id[1], result->weight[0], result->weight[1]);

	return SMART_LIGHT_SUCCESS;
}

static int32_t _smart_gain_adjust(struct smart_light_gain_param *param, struct smart_light_envi_result *envi_result,
					uint32_t ct, struct smart_light_gain *gain, struct smart_rgb_queue *gain_factor_queue,
					uint32_t quick_mode,
					struct smart_light_gain_result *result)
{
	struct smart_light_gain gain_factor = {SMART_WEIGHT_UNIT, SMART_WEIGHT_UNIT, SMART_WEIGHT_UNIT};
	struct smart_light_gain result_gain = *gain;
	enum smart_light_envi_id *envi_id = envi_result->envi_id;
	uint16_t *weight = envi_result->weight;

	SMART_LOGV("ct = %d, envi_id=(%d, %d)", ct, (uint32_t)envi_id[0], (uint32_t)envi_id[1]);

	if ((weight[0] > 0 && 0 == weight[1]) || (weight[1] > 0 &&  0 == weight[0])) {

		enum smart_light_envi_id cur_envi_id = weight[0] > 0 ? envi_id[0] : envi_id[1];

		SMART_LOGV("r func num=%d", param->r_gain_func[cur_envi_id].num);
		SMART_LOGV("g func num=%d", param->g_gain_func[cur_envi_id].num);
		SMART_LOGV("b func num=%d", param->b_gain_func[cur_envi_id].num);

		if (param->r_gain_func[cur_envi_id].num > 0)
			gain_factor.r = _calc_piecewise_func_interp(&param->r_gain_func[cur_envi_id], ct);

		if (param->g_gain_func[cur_envi_id].num > 0)
			gain_factor.g = _calc_piecewise_func_interp(&param->g_gain_func[cur_envi_id], ct);

		if (param->b_gain_func[cur_envi_id].num > 0)
			gain_factor.b = _calc_piecewise_func_interp(&param->b_gain_func[cur_envi_id], ct);

	} else if (weight[0] > 0 && weight[1] > 0) {

		enum smart_light_envi_id cur_envi_id = SMART_ENVI_COMMON;
		uint16_t *weight = envi_result->weight;
		struct smart_light_gain mixed_gain_factor[2] = {
				{SMART_WEIGHT_UNIT, SMART_WEIGHT_UNIT, SMART_WEIGHT_UNIT},
				{SMART_WEIGHT_UNIT, SMART_WEIGHT_UNIT, SMART_WEIGHT_UNIT}
				};
		uint32_t index = 0;

		index = 0;
		cur_envi_id = envi_id[index];
		if (param->r_gain_func[cur_envi_id].num > 0)
			mixed_gain_factor[index].r = _calc_piecewise_func_interp(&param->r_gain_func[cur_envi_id], ct);

		if (param->g_gain_func[cur_envi_id].num > 0)
			mixed_gain_factor[index].g = _calc_piecewise_func_interp(&param->g_gain_func[cur_envi_id], ct);

		if (param->b_gain_func[cur_envi_id].num > 0)
			mixed_gain_factor[index].b = _calc_piecewise_func_interp(&param->b_gain_func[cur_envi_id], ct);

		index = 1;
		cur_envi_id = envi_id[index];
		if (param->r_gain_func[cur_envi_id].num > 0)
			mixed_gain_factor[index].r = _calc_piecewise_func_interp(&param->r_gain_func[cur_envi_id], ct);

		if (param->g_gain_func[cur_envi_id].num > 0)
			mixed_gain_factor[index].g = _calc_piecewise_func_interp(&param->g_gain_func[cur_envi_id], ct);

		if (param->b_gain_func[cur_envi_id].num > 0)
			mixed_gain_factor[index].b = _calc_piecewise_func_interp(&param->b_gain_func[cur_envi_id], ct);

		gain_factor.r = (mixed_gain_factor[0].r * weight[0] + mixed_gain_factor[1].r * weight[1]) / SMART_WEIGHT_UNIT;
		gain_factor.g = (mixed_gain_factor[0].g * weight[0] + mixed_gain_factor[1].g * weight[1]) / SMART_WEIGHT_UNIT;
		gain_factor.b = (mixed_gain_factor[0].b * weight[0] + mixed_gain_factor[1].b * weight[1]) / SMART_WEIGHT_UNIT;

	} else {
		SMART_LOGE("envi detect failed, weight[0] = weight[1] = 0");
		return SMART_LIGHT_ERROR;
	}

	/*clear the initialized value for the first time at quick mode*/
	if (0 != quick_mode) {
		_rgb_queue_clear(gain_factor_queue);
		SMART_LOGI("gain use quick mode for the first time");
	}

	_rgb_queue_add(gain_factor_queue, &gain_factor);
	_rgb_queue_average(gain_factor_queue, &gain_factor);

	result_gain.r = gain_factor.r * result_gain.r / SMART_WEIGHT_UNIT;
	result_gain.g = gain_factor.g * result_gain.g / SMART_WEIGHT_UNIT;
	result_gain.b = gain_factor.b * result_gain.b / SMART_WEIGHT_UNIT;

	if (result_gain.r != gain->r || result_gain.g != gain->g || result_gain.b != gain->b)
		result->update = 1;
	else
		result->update = 0;

	result->gain = result_gain;
	result->factor = gain_factor;

	SMART_LOGV("result gain = (%d, %d, %d), gain factor = (%d, %d, %d)",
			result_gain.r, result_gain.g, result_gain.b, gain_factor.r,
			gain_factor.g, gain_factor.b);

	return SMART_LIGHT_SUCCESS;
}

static int32_t _smart_lsc_adjust(struct smart_light_lsc_param *param, struct smart_light_envi_result *envi_info,
					uint32_t ct, struct smart_light_lsc_result *result)
{
	int32_t rtn = SMART_LIGHT_SUCCESS;
	enum smart_light_envi_id *envi_id = envi_info->envi_id;
	uint16_t *envi_weight = envi_info->weight;
	struct smart_light_piecewise_func *adjust_func = NULL;
	int16_t index[2];
	uint16_t weight[2];
	uint16_t cur_index = 0;

	SMART_LOGV("ct = %d, envi=(%d, %d)", ct, (uint32_t)envi_id[0], (uint32_t)envi_id[1]);

	if (0 == ct) {
		result->update = 0;
		return SMART_LIGHT_ERROR;
	}

	index[0] = result->index[0];
	index[1] = result->index[1];
	weight[0] = result->weight[0];
	weight[1] = result->weight[1];

	if ((envi_weight[0] > 0 && 0 == envi_weight[1]) || (envi_weight[1] > 0 &&  0 == envi_weight[0])) {

		enum smart_light_envi_id cur_envi_id = envi_weight[0] > 0 ? envi_id[0] : envi_id[1];

		adjust_func = &param->adjust_func[cur_envi_id];
		SMART_LOGV("num=%d, [%d, %d]", adjust_func->num, adjust_func->samples[0].x,
				adjust_func->samples[0].y);
		if (adjust_func->num > 0)
			_calc_piecewise_func_no_interp(adjust_func, ct, index, weight);

	} else if (envi_weight[0] > 0 && envi_weight[1] > 0) {

		enum smart_light_envi_id cur_envi_id = SMART_ENVI_COMMON;

		cur_envi_id = envi_id[0];
		adjust_func = &param->adjust_func[cur_envi_id];
		if (adjust_func->num > 0) {
			int16_t cur_value[2] = {0};
			uint16_t cur_weight[2] = {SMART_WEIGHT_UNIT, 0};
			_calc_piecewise_func_no_interp(adjust_func, ct,
								cur_value, cur_weight);

			index[0] = (cur_weight[0] > cur_weight[1]) ? cur_value[0] : cur_value[1];
		}

		cur_envi_id = envi_id[1];
		adjust_func = &param->adjust_func[cur_envi_id];
		if (adjust_func->num > 0) {
			int16_t cur_value[2] = {0};
			uint16_t cur_weight[2] = {SMART_WEIGHT_UNIT, 0};
			_calc_piecewise_func_no_interp(adjust_func, ct,
								cur_value, cur_weight);

			index[1] = (cur_weight[0] > cur_weight[1]) ? cur_value[0] : cur_value[1];
		}

		weight[0] = envi_weight[0];
		weight[1] = envi_weight[1];
	} else {
		SMART_LOGE("envi weight error, weight[0] = weight[1] = 0");
		return SMART_LIGHT_ERROR;
	}

	/*decrease the precision to avoid update lsc parameter too freqency*/
	weight[0] = weight[0] / 16 * 16;
	weight[1] = SMART_WEIGHT_UNIT - weight[0];

	if (index[0] == index[1]) {
		weight[0] = SMART_WEIGHT_UNIT;
		weight[1] = 0;
	}

	if (weight[0] != result->weight[0] || weight[1] != result->weight[1]
		|| index[0] != result->index[0] || index[1] != result->index[1]) {
		result->update = 1;
		result->weight[0] = weight[0];
		result->weight[1] = weight[1];
		result->index[0] = index[0];
		result->index[1] = index[1];
	} else {
		result->update = 0;
	}

	SMART_LOGV("index=(%d, %d), weight=(%d, %d)", result->index[0], result->index[1],
		result->weight[0], result->weight[1]);

	return rtn;
}

static int32_t _smart_cmc_adjust(struct smart_light_cmc_param *param, struct smart_light_envi_result *envi_info,
					uint32_t ct, struct smart_light_cmc_result *result)
{
	int32_t rtn = SMART_LIGHT_SUCCESS;
	enum smart_light_envi_id *envi_id = envi_info->envi_id;
	uint16_t *envi_weight = envi_info->weight;
	struct smart_light_piecewise_func *adjust_func = NULL;
	int16_t index[2];
	uint16_t weight[2];
	uint16_t cur_index = 0;

	SMART_LOGV("ct = %d, envi=(%d, %d)", ct, (uint32_t)envi_id[0], (uint32_t)envi_id[1]);

	if (0 == ct) {
		result->update = 0;
		return SMART_LIGHT_ERROR;
	}

	index[0] = result->index[0];
	index[1] = result->index[1];
	weight[0] = result->weight[0];
	weight[1] = result->weight[1];

	if ((envi_weight[0] > 0 && 0 == envi_weight[1]) || (envi_weight[1] > 0 &&  0 == envi_weight[0])) {

		enum smart_light_envi_id cur_envi_id = envi_weight[0] > 0 ? envi_id[0] : envi_id[1];

		adjust_func = &param->adjust_func[cur_envi_id];
		if (adjust_func->num > 0)
			_calc_piecewise_func_no_interp(adjust_func, ct, index, weight);
	} else if (envi_weight[0] > 0 && envi_weight[1] > 0) {

		enum smart_light_envi_id cur_envi_id = SMART_ENVI_COMMON;

		cur_envi_id = envi_id[0];
		adjust_func = &param->adjust_func[cur_envi_id];
		if (adjust_func->num > 0) {
			int16_t cur_value[2] = {0};
			uint16_t cur_weight[2] = {SMART_WEIGHT_UNIT, 0};
			_calc_piecewise_func_no_interp(adjust_func, ct,
								cur_value, cur_weight);

			index[0] = (cur_weight[0] > cur_weight[1]) ? cur_value[0] : cur_value[1];
		}

		cur_envi_id = envi_id[1];
		adjust_func = &param->adjust_func[cur_envi_id];
		if (adjust_func->num > 0) {
			int16_t cur_value[2] = {0};
			uint16_t cur_weight[2] = {SMART_WEIGHT_UNIT, 0};
			_calc_piecewise_func_no_interp(adjust_func, ct,
								cur_value, cur_weight);

			index[1] = (cur_weight[0] > cur_weight[1]) ? cur_value[0] : cur_value[1];
		}

		weight[0] = envi_weight[0];
		weight[1] = envi_weight[1];
	} else {
		SMART_LOGE("envi weight error, weight[0] = weight[1] = 0");
		return SMART_LIGHT_ERROR;
	}

	/*decrease the precision to avoid update lsc parameter too freqency*/
	weight[0] = weight[0] / 16 * 16;
	weight[1] = SMART_WEIGHT_UNIT - weight[0];

	if (index[0] == index[1]) {
		weight[0] = SMART_WEIGHT_UNIT;
		weight[1] = 0;
	}

	if (weight[0] != result->weight[0] || weight[1] != result->weight[1]
		|| index[0] != result->index[0] || index[1] != result->index[1]) {
		result->update = 1;
		result->weight[0] = weight[0];
		result->weight[1] = weight[1];
		result->index[0] = index[0];
		result->index[1] = index[1];
	} else {
		result->update = 0;
	}

	SMART_LOGV("index=(%d, %d), weight=(%d, %d)", result->index[0], result->index[1],
		result->weight[0], result->weight[1]);

	return rtn;
}

uint32_t _smart_saturation(struct smart_light_saturation_param *sat_param,
				struct smart_light_envi_result *envi_info, uint32_t ct,
				struct smart_light_saturation_result *result)
{
	int32_t rtn = SMART_LIGHT_SUCCESS;
	enum smart_light_envi_id *envi_id = envi_info->envi_id;
	uint16_t *envi_weight = envi_info->weight;
	struct smart_light_piecewise_func *adjust_func = NULL;
	uint16_t weight[2] = {SMART_WEIGHT_UNIT, SMART_WEIGHT_UNIT};
	uint32_t value = 0;
	uint32_t update = 1;

	value = result->value;

	if ((envi_weight[0] > 0 && 0 == envi_weight[1]) || (envi_weight[1] > 0 &&  0 == envi_weight[0])) {

		enum smart_light_envi_id cur_envi_id = envi_weight[0] > 0 ? envi_id[0] : envi_id[1];

		adjust_func = &sat_param->adjust_func[cur_envi_id];
		if (adjust_func->num > 0)
			value = _calc_piecewise_func_interp(adjust_func, ct);
		else
			value = 0;
	} else if (envi_weight[0] > 0 && envi_weight[1] > 0) {

		enum smart_light_envi_id cur_envi_id = SMART_ENVI_COMMON;
		int32_t cur_value[2];

		cur_value[0] = value;
		cur_value[1] = value;

		cur_envi_id = envi_id[0];
		adjust_func = &sat_param->adjust_func[cur_envi_id];
		if (adjust_func->num > 0)
			cur_value[0] = _calc_piecewise_func_interp(adjust_func, ct);
		else
			cur_value[0] = 0;

		cur_envi_id = envi_id[1];
		adjust_func = &sat_param->adjust_func[cur_envi_id];
		if (adjust_func->num > 0)
			cur_value[1] = _calc_piecewise_func_interp(adjust_func, ct);
		else
			cur_value[1] = 0;

		weight[0] = envi_weight[0];
		weight[1] = envi_weight[1];

		value = (weight[0] * cur_value[0] + weight[1] * cur_value[1]) / SMART_WEIGHT_UNIT;
	} else {
		SMART_LOGE("envi weight error, weight[0] = weight[1] = 0");
		value = 0;
		return SMART_LIGHT_ERROR;
	}

	result->value = value;
	result->update = update;

	SMART_LOGV("value = %d, update=%d, ct=%d", result->value, result->update, ct);

	return rtn;
}

uint32_t _smart_hue(struct smart_light_hue_param *hue_param,
				struct smart_light_envi_result *envi_info, uint32_t ct,
				struct smart_light_hue_result *result)
{
	int32_t rtn = SMART_LIGHT_SUCCESS;
	enum smart_light_envi_id *envi_id = envi_info->envi_id;
	uint16_t *envi_weight = envi_info->weight;
	struct smart_light_piecewise_func *adjust_func = NULL;
	uint16_t weight[2] = {SMART_WEIGHT_UNIT, SMART_WEIGHT_UNIT};
	int16_t value = 0;
	uint32_t update = 1;

	value = result->value;

	if ((envi_weight[0] > 0 && 0 == envi_weight[1]) || (envi_weight[1] > 0 &&  0 == envi_weight[0])) {

		enum smart_light_envi_id cur_envi_id = envi_weight[0] > 0 ? envi_id[0] : envi_id[1];

		adjust_func = &hue_param->adjust_func[cur_envi_id];
		if (adjust_func->num > 0)
			value = _calc_piecewise_func_interp(adjust_func, ct);
		else
			value = 0;

	} else if (envi_weight[0] > 0 && envi_weight[1] > 0) {

		enum smart_light_envi_id cur_envi_id = SMART_ENVI_COMMON;
		int32_t cur_value[2];

		cur_value[0] = value;
		cur_value[1] = value;

		cur_envi_id = envi_id[0];
		adjust_func = &hue_param->adjust_func[cur_envi_id];
		if (adjust_func->num > 0)
			cur_value[0] = _calc_piecewise_func_interp(adjust_func, ct);
		else
			cur_value[0] = 0;

		cur_envi_id = envi_id[1];
		adjust_func = &hue_param->adjust_func[cur_envi_id];
		if (adjust_func->num > 0)
			cur_value[1] = _calc_piecewise_func_interp(adjust_func, ct);
		else
			cur_value[1] = 0;

		weight[0] = envi_weight[0];
		weight[1] = envi_weight[1];

		/*hue should not be change if parameter of one environment is invalid */
		cur_value[0] = (0 == cur_value[0]) ? cur_value[1] : cur_value[0];
		cur_value[1] = (0 == cur_value[1]) ? cur_value[0] : cur_value[1];

		value = (weight[0] * cur_value[0] + weight[1] * cur_value[1]) / SMART_WEIGHT_UNIT;
	} else {
		SMART_LOGE("envi weight error, weight[0] = weight[1] = 0");
		value = 0;
		return SMART_LIGHT_ERROR;
	}

	result->value = value;
	result->update = update;

	SMART_LOGV("value = %d, update=%d, ct=%d", result->value, result->update, ct);

	return rtn;
}

static int32_t _convert_hue_saturation_to_gain(uint16_t hue, uint16_t saturation,
							struct smart_light_gain *gain)
{
	int32_t rtn = SMART_LIGHT_SUCCESS;

	if (hue < 1 || hue > 60 || saturation > 100 || saturation < 1) {
		gain->r = SMART_HUE_SAT_GAIN_UNIT;
		gain->g = SMART_HUE_SAT_GAIN_UNIT;
		gain->b = SMART_HUE_SAT_GAIN_UNIT;
		return rtn;
	}

	gain->b = (6000 - 60 * saturation) * SMART_HUE_SAT_GAIN_UNIT
			/ (6000 + hue * saturation - 60 * saturation);
	gain->g = SMART_HUE_SAT_GAIN_UNIT;
	gain->r = ((hue - 60) * gain->b + 60 * SMART_HUE_SAT_GAIN_UNIT) / hue;

	SMART_LOGV("gain = (%d, %d, %d)", gain->r, gain->g, gain->b);

	return rtn;
}

static int32_t _smart_denoise_adjust(struct smart_light_denoise_param *param, int32_t bv,
					struct smart_light_denoise_result *result)
{
	int32_t rtn = SMART_LIGHT_SUCCESS;
	uint32_t lsc_dec_ratio = result->lsc_dec_ratio;

	result->update = 0;

	/*disabel denoise adjust*/
	if (0 == param->lsc_dec_ratio_range.min && 0 == param->lsc_dec_ratio_range.max)
		return SMART_LIGHT_SUCCESS;

	/*for low light*/
	if (bv <= param->bv_range.min) {
		lsc_dec_ratio = param->lsc_dec_ratio_range.min;
	} else if (bv >= param->bv_range.max) {
		lsc_dec_ratio = param->lsc_dec_ratio_range.max;
	} else {
		uint32_t ratio_range = param->lsc_dec_ratio_range.max - param->lsc_dec_ratio_range.min;
		uint32_t bv_range = param->bv_range.max - param->bv_range.min;
		uint32_t ratio_min = param->lsc_dec_ratio_range.min;

		lsc_dec_ratio = ratio_min + ((bv - param->bv_range.min) * ratio_range + bv_range / 2) / bv_range;
	}

	if (lsc_dec_ratio != result->lsc_dec_ratio) {
		result->lsc_dec_ratio = lsc_dec_ratio;
		result->update = 1;
	}

	return rtn;
}

static void _init_debug_file(struct smart_light_context *cxt)
{
	cxt->debug_handle = debug_file_init(DEBUG_FILE, DEBUG_FILE_OP);
}

static void _deinit_debug_file(struct smart_light_context *cxt)
{
	debug_file_deinit(cxt->debug_handle);
}

static int32_t _init_smart_bv(struct smart_light_context *cxt)
{
	int32_t rtn = SMART_LIGHT_SUCCESS;
	uint16_t init_value = 0;
	struct smart_light_gain init_gain = {SMART_WEIGHT_UNIT, SMART_WEIGHT_UNIT, SMART_WEIGHT_UNIT};

	init_value = SMART_LIGHT_BV_INIT_VALUE;
	rtn = _init_normal_queue(&cxt->bv_queue, AWBL_MAX_QUEUE_SIZE, &init_value);
	if (SMART_LIGHT_SUCCESS != rtn) {
		SMART_LOGE("init bv queue failed");
		return SMART_LIGHT_ERROR;
	}

	init_value = SMART_LIGHT_BVQUEUE_INIT_VALUE;
	rtn = _init_normal_queue(&cxt->bv_delta_queue, SMART_LIGHT_MAX_QUEUE_SIZE, &init_value);
	if (SMART_LIGHT_SUCCESS != rtn) {
		SMART_LOGE("init bv delta queue failed");
		return SMART_LIGHT_ERROR;
	}

	init_gain.r = SMART_LIGHT_BVQUEUE_INIT_VALUE;
	init_gain.g = SMART_LIGHT_BVQUEUE_INIT_VALUE;
	init_gain.b = SMART_LIGHT_BVQUEUE_INIT_VALUE;

	rtn = _init_rgb_queue(&cxt->envi_queue, SMART_LIGHT_MAX_QUEUE_SIZE, &init_gain);
	if (SMART_LIGHT_SUCCESS != rtn) {
		SMART_LOGE("init envi_id queue failed");
		return SMART_LIGHT_ERROR;
	}

	return SMART_LIGHT_SUCCESS;
}
static void _smart_envi_calc(struct smart_light_context *calc_cxt, struct smart_bv_calc_param * bv_calc_param, uint32_t quickmode)
{
	struct smart_light_gain envi_factor = {0};

	if (0 != quickmode) {
		_rgb_queue_clear(&calc_cxt->envi_queue);
		SMART_LOGI("only use quick mode for the first time!");
	}

	envi_factor.r = calc_cxt->calc_result.envi.envi_id[0];
	envi_factor.g = calc_cxt->calc_result.envi.envi_id[0];
	envi_factor.b = calc_cxt->calc_result.envi.envi_id[0];

	if (envi_factor.r < SMART_ENVI_SIMPLE_MAX_NUM) {

		_rgb_queue_add(&calc_cxt->envi_queue, &envi_factor);
		_rgb_queue_average(&calc_cxt->envi_queue, &envi_factor);
		_envi_queue_statis(bv_calc_param, &calc_cxt->envi_queue);
		bv_calc_param->last_envi_id = envi_factor.r;
	}

}
static void _print_debug_info(struct smart_light_context *cxt)
{
	struct smart_light_calc_result *result = &cxt->calc_result;
	struct smart_light_calc_param*calc_param = &cxt->calc_param;
	uint32_t debug_level = 1;
	uint32_t debug_thres = 0;

	SMART_LOGI("debug_handle=%p", cxt->debug_handle);
	debug_file_open(cxt->debug_handle, debug_level, debug_thres);

	sprintf(cxt->debug_str, "smart=%d, bv=%d, ct=%d, quick=%d", calc_param->smart,
		calc_param->bv, calc_param->ct, calc_param->quick_mode);
	debug_file_print(cxt->debug_handle, cxt->debug_str);

	sprintf(cxt->debug_str, "envi: update=%d, id=(%d, %d), (%d, %d)", result->envi.update,
		result->envi.envi_id[0], result->envi.envi_id[1], result->envi.weight[0], result->envi.weight[1]);
	debug_file_print(cxt->debug_handle, cxt->debug_str);

	sprintf(cxt->debug_str, "lsc: update=%d, idx=(%d, %d), (%d, %d)", result->lsc.update,
		result->lsc.index[0], result->lsc.index[1], result->lsc.weight[0], result->lsc.weight[1]);
	debug_file_print(cxt->debug_handle, cxt->debug_str);

	sprintf(cxt->debug_str, "cmc: update=%d, idx=(%d, %d), (%d, %d)", result->cmc.update,
		result->cmc.index[0], result->cmc.index[1], result->cmc.weight[0], result->cmc.weight[1]);
	debug_file_print(cxt->debug_handle, cxt->debug_str);

	sprintf(cxt->debug_str, "gain: update=%d, (%d, %d, %d)", result->gain.update,
		result->gain.gain.r, result->gain.gain.g, result->gain.gain.b);
	debug_file_print(cxt->debug_handle, cxt->debug_str);

	sprintf(cxt->debug_str, "gain factor=(%d, %d, %d)", result->gain.factor.r,
		result->gain.factor.g, result->gain.factor.b);
	debug_file_print(cxt->debug_handle, cxt->debug_str);

	/*
	sprintf(cxt->debug_str, "input gain=(%d, %d, %d)", calc_param->gain.r,
		calc_param->gain.g, calc_param->gain.b);
	debug_file_print(cxt->debug_handle, cxt->debug_str);
	*/

	sprintf(cxt->debug_str, "hue: update=%d, val=%d", result->hue.update, result->hue.value);
	debug_file_print(cxt->debug_handle, cxt->debug_str);

	sprintf(cxt->debug_str, "sat: update=%d, val=%d", result->saturation.update, result->saturation.value);
	debug_file_print(cxt->debug_handle, cxt->debug_str);

	sprintf(cxt->debug_str, "hue_sat: update=%d, (%d, %d, %d)", result->hue_saturation.update,
		result->hue_saturation.r_gain, result->hue_saturation.g_gain, result->hue_saturation.b_gain);
	debug_file_print(cxt->debug_handle, cxt->debug_str);

	sprintf(cxt->debug_str, "denoise: update=%d, ratio=%d", result->denoise.update, result->denoise.lsc_dec_ratio);
	debug_file_print(cxt->debug_handle, cxt->debug_str);

	debug_file_close(cxt->debug_handle);
}

static void _init_smartlight_parameter(struct smart_light_context *smartlight_cxt)
{
	smartlight_cxt->calc_result.lsc.index[0] = smartlight_cxt->init_param.lsc_init.index[0];
	smartlight_cxt->calc_result.lsc.index[1] = smartlight_cxt->init_param.lsc_init.index[1];
	smartlight_cxt->calc_result.lsc.weight[0] = smartlight_cxt->init_param.lsc_init.weight[0];
	smartlight_cxt->calc_result.lsc.weight[1] = smartlight_cxt->init_param.lsc_init.weight[1];

	smartlight_cxt->calc_result.cmc.index[0] = smartlight_cxt->init_param.cmc_init.index[0];
	smartlight_cxt->calc_result.cmc.index[1] = smartlight_cxt->init_param.cmc_init.index[1];
	smartlight_cxt->calc_result.cmc.weight[0] = smartlight_cxt->init_param.cmc_init.weight[0];
	smartlight_cxt->calc_result.cmc.weight[1] = smartlight_cxt->init_param.cmc_init.weight[1];

	smartlight_cxt->calc_result.hue_saturation.r_gain = smartlight_cxt->init_param.init_hue_sat.r_gain;
	smartlight_cxt->calc_result.hue_saturation.g_gain = smartlight_cxt->init_param.init_hue_sat.g_gain;
	smartlight_cxt->calc_result.hue_saturation.b_gain = smartlight_cxt->init_param.init_hue_sat.b_gain;
}
void _print_init_param(struct smart_light_init_param *init_param)
{
	uint32_t i = 0;

	SMART_LOGI("envi info:");
	for (i=0; i<SMART_ENVI_SIMPLE_NUM; i++) {
		SMART_LOGI("	[%d]=(%d, %d)", i, init_param->envi.bv_range[i].min,
				init_param->envi.bv_range[i].max);
	}

	SMART_LOGI("lsc info:");
	for (i=0; i<SMART_ENVI_SIMPLE_NUM; i++) {
		struct smart_light_piecewise_func *func = &init_param->lsc.adjust_func[i];
		uint32_t j = 0;

		SMART_LOGI("envi_id=%d, num=%d", i, func->num);
		for (j=0; j<func->num; j++)
			SMART_LOGI("	[%d]=(%d, %d)", j, func->samples[j].x, func->samples[j].y);
	}

	SMART_LOGI("cmc info:");
	for (i=0; i<SMART_ENVI_SIMPLE_NUM; i++) {
		struct smart_light_piecewise_func *func = &init_param->cmc.adjust_func[i];
		uint32_t j = 0;

		SMART_LOGI("envi_id=%d, num=%d", i, func->num);
		for (j=0; j<func->num; j++)
			SMART_LOGI("	[%d]=(%d, %d)", j, func->samples[j].x, func->samples[j].y);
	}

	SMART_LOGI("gain info:");
	for (i=0; i<SMART_ENVI_SIMPLE_NUM; i++) {

		struct smart_light_piecewise_func *r_func = &init_param->gain.r_gain_func[i];
		struct smart_light_piecewise_func *g_func = &init_param->gain.g_gain_func[i];
		struct smart_light_piecewise_func *b_func = &init_param->gain.b_gain_func[i];
		uint32_t j = 0;

		SMART_LOGI("envi_id=%d, num=(%d, %d, %d)", i, r_func->num, g_func->num, b_func->num);
		for (j=0; j<r_func->num; j++) {
			SMART_LOGI("	r: [%d]=(%d, %d)", j, r_func->samples[j].x, r_func->samples[j].y);
			SMART_LOGI("	g: [%d]=(%d, %d)", j, g_func->samples[j].x, g_func->samples[j].y);
			SMART_LOGI("	b: [%d]=(%d, %d)", j, b_func->samples[j].x, b_func->samples[j].y);
		}
	}

	SMART_LOGI("hue info:");
	for (i=0; i<SMART_ENVI_SIMPLE_NUM; i++) {
		struct smart_light_piecewise_func *func = &init_param->hue.adjust_func[i];
		uint32_t j = 0;

		SMART_LOGI("envi_id=%d, num=%d", i, func->num);
		for (j=0; j<func->num; j++)
			SMART_LOGI("	[%d]=(%d, %d)", j, func->samples[j].x, func->samples[j].y);
	}

	SMART_LOGI("saturation info:");
	for (i=0; i<SMART_ENVI_SIMPLE_NUM; i++) {
		struct smart_light_piecewise_func *func = &init_param->saturation.adjust_func[i];
		uint32_t j = 0;

		SMART_LOGI("envi_id=%d, num=%d", i, func->num);
		for (j=0; j<func->num; j++)
			SMART_LOGI("	[%d]=(%d, %d)", j, func->samples[j].x, func->samples[j].y);
	}

	SMART_LOGI("denoise info:");
	SMART_LOGI("	bv_range=(%d, %d), lsc_dec_ratio=(%d, %d)", init_param->denoise.bv_range.min,
					init_param->denoise.bv_range.max,
					init_param->denoise.lsc_dec_ratio_range.min,
					init_param->denoise.lsc_dec_ratio_range.max);

	SMART_LOGI("steady_speed=%d", init_param->steady_speed);
}


static int32_t _check_init_param(struct smart_light_init_param *init_param)
{
	int32_t rtn = SMART_LIGHT_SUCCESS;
	struct smart_light_range_l *envi_bv = init_param->envi.bv_range;
	struct smart_light_denoise_param *denoise_param = &init_param->denoise;
	uint32_t i = 0;
	int32_t max_bv = -256;

	/*check envi param*/
	for (i=SMART_ENVI_LOW_LIGHT; i<=SMART_ENVI_OUTDOOR_HIGH; i++) {

		if (0 == envi_bv[i].min && 0 == envi_bv[i].max)
			continue;

		if (envi_bv[i].min < (int32_t)max_bv || envi_bv[i].min > envi_bv[i].max) {
			SMART_LOGE("invalid param: [%d]=(%d, %d)", i, envi_bv[i].min, envi_bv[i].max);
			return SMART_LIGHT_ERROR;
		}

		max_bv = envi_bv[i].max;
	}

	if (denoise_param->bv_range.min > denoise_param->bv_range.max) {
		int32_t temp = denoise_param->bv_range.min;
		denoise_param->bv_range.min = denoise_param->bv_range.max;
		denoise_param->bv_range.max = temp;
		SMART_LOGE("exchange bv range for denoise (%d, %d)", denoise_param->bv_range.min,
				denoise_param->bv_range.max);
	}

	if (denoise_param->lsc_dec_ratio_range.min > SMART_MAX_LSC_DEC_RATIO)
		denoise_param->lsc_dec_ratio_range.min = SMART_MAX_LSC_DEC_RATIO;

	if (denoise_param->lsc_dec_ratio_range.max > SMART_MAX_LSC_DEC_RATIO)
		denoise_param->lsc_dec_ratio_range.max = SMART_MAX_LSC_DEC_RATIO;

	if (denoise_param->lsc_dec_ratio_range.min > denoise_param->lsc_dec_ratio_range.max) {
		uint32_t temp = denoise_param->lsc_dec_ratio_range.min;
		denoise_param->lsc_dec_ratio_range.min = denoise_param->lsc_dec_ratio_range.max;
		denoise_param->lsc_dec_ratio_range.max = temp;
		SMART_LOGE("exchange lsc dec ratio range for denoise (%d, %d)", denoise_param->lsc_dec_ratio_range.min,
				denoise_param->lsc_dec_ratio_range.max);
	}

	if (init_param->steady_speed > SMART_LIGHT_MAX_QUEUE_SIZE)
		init_param->steady_speed = SMART_LIGHT_MAX_QUEUE_SIZE;

	return rtn;
}

/*------------------------------------------------------------------------------*
*					public functions			*
*-------------------------------------------------------------------------------*/
int32_t smart_light_init(uint32_t handler_id, void *in_param, void *out_param)
{
	int32_t rtn = SMART_LIGHT_SUCCESS;
	struct smart_light_context *cxt = _get_context(handler_id);
	struct smart_light_init_param *init_param = (struct smart_light_init_param *)in_param;
	struct smart_light_gain init_gain = {SMART_WEIGHT_UNIT, SMART_WEIGHT_UNIT, SMART_WEIGHT_UNIT};
	uint32_t i = 0;

	if (NULL == cxt || NULL == init_param) {
		SMART_LOGE("invalide init param");
		return SMART_LIGHT_ERROR;
	}

	_print_init_param(init_param);

	rtn = _check_init_param(init_param);
	if (SMART_LIGHT_SUCCESS != rtn) {
		SMART_LOGE("_check_init_param failed");
		return SMART_LIGHT_ERROR;
	}

	memcpy(&cxt->init_param, init_param, sizeof(cxt->init_param));
	init_gain.r = cxt->init_param.init_gain.gain.r;
	init_gain.g = cxt->init_param.init_gain.gain.g;
	init_gain.b = cxt->init_param.init_gain.gain.b;
	rtn = _init_rgb_queue(&cxt->hue_sat_gain_queue, init_param->steady_speed, &init_gain);
	if (SMART_LIGHT_SUCCESS != rtn) {
		SMART_LOGE("init hue sat gain queue failed");
		return SMART_LIGHT_ERROR;
	}

	init_gain.r = cxt->init_param.init_hue_sat.r_gain;
	init_gain.g = cxt->init_param.init_hue_sat.g_gain;
	init_gain.b = cxt->init_param.init_hue_sat.b_gain;
	rtn = _init_rgb_queue(&cxt->gain_factor_queue, init_param->steady_speed, &init_gain);
	if (SMART_LIGHT_SUCCESS != rtn) {
		SMART_LOGE("init gain factor queue failed");
		return SMART_LIGHT_ERROR;
	}

	rtn = _init_smart_bv(cxt);
	if (SMART_LIGHT_SUCCESS != rtn) {
		SMART_LOGE("init smart bv queue failed");
		return SMART_LIGHT_ERROR;
	}

	_init_smartlight_parameter(cxt);
	cxt->init = SMART_LIGHT_INIT_DONE;

	_init_debug_file(cxt);

	return SMART_LIGHT_SUCCESS;
}

int32_t smart_light_calculation(uint32_t handler_id, void *in_param, void *out_param)
{
	struct smart_light_context *cxt = _get_context(handler_id);
	struct smart_light_calc_param *calc_param = (struct smart_light_calc_param *)in_param;
	struct smart_light_calc_result *calc_result = (struct smart_light_calc_result *)out_param;
	struct smart_light_envi_result envi_info = {{0}, {0}, 0};
	struct smart_light_lsc_result *lsc_result = NULL;
	struct smart_light_cmc_result *cmc_result = NULL;
	struct smart_light_saturation_result *saturation_result = NULL;
	struct smart_light_gain_result *gain_result = NULL;
	struct smart_light_hue_result *hue_result = NULL;
	struct smart_light_hue_saturation_result *hue_sat_result = NULL;
	struct smart_light_denoise_result *denoise_result = NULL;
	struct smart_bv_calc_param bv_param = {0};
	uint32_t quick_mode = 0;
	uint32_t ct = 0;
	uint32_t smart = 0;

	if (NULL == cxt || NULL == calc_param || NULL == calc_result) {
		SMART_LOGE("invalid parameters");
		return SMART_LIGHT_ERROR;
	}

	if (SMART_LIGHT_INIT_DONE != cxt->init) {
		SMART_LOGE("do not init");
		return SMART_LIGHT_ERROR;
	}

	if (0 == calc_param->smart) {
		SMART_LOGE("smart disable");
		return SMART_LIGHT_ERROR;
	}

	SMART_LOGE("smart=%d, bv=%d, ct=%d, gain=(%d, %d, %d)",
		calc_param->smart, calc_param->bv, calc_param->ct,
		calc_param->gain.r, calc_param->gain.g, calc_param->gain.b);

	cxt->calc_param = *calc_param;

	//set the last value as the default value
	lsc_result = &cxt->calc_result.lsc;
	cmc_result = &cxt->calc_result.cmc;
	saturation_result = &cxt->calc_result.saturation;
	gain_result = &cxt->calc_result.gain;
	hue_result = &cxt->calc_result.hue;
	hue_sat_result = &cxt->calc_result.hue_saturation;
	denoise_result = &cxt->calc_result.denoise;
	ct = calc_param->ct;
	smart = calc_param->smart;
	/*only use quick mode at the first time*/
	quick_mode = cxt->calc_param.quick_mode;
	cxt->calc_param.quick_mode = 0;

	if (0 != (smart & SMART_ENVI)) {
		envi_info = cxt->calc_result.envi;
		bv_param.cur_bv_value = calc_param->bv;
		bv_param.bv_queue = cxt->bv_queue;
		bv_param.bv_delta_queue = cxt->bv_delta_queue;
		bv_param.bv_enable = SMART_BV_UENABLE;
		_smart_bv_adjust(&bv_param, quick_mode);

		_smart_envi_calc(cxt, &bv_param, quick_mode);

		_smart_envi_detect(&cxt->init_param.envi, calc_param->bv, &envi_info, &bv_param);
	} else {
		envi_info.envi_id[0] = SMART_ENVI_COMMON;
		envi_info.envi_id[1] = SMART_ENVI_COMMON;
		envi_info.weight[0] = SMART_WEIGHT_UNIT;
		envi_info.weight[1] = 0;

		if (envi_info.envi_id[0] != cxt->calc_result.envi.envi_id[0]
			|| envi_info.envi_id[1] != cxt->calc_result.envi.envi_id[1]
			|| envi_info.weight[0] != cxt->calc_result.envi.weight[0]
			|| envi_info.weight[1] != cxt->calc_result.envi.weight[1])
			envi_info.update = 1;
		else
			envi_info.update = 0;
	}

	cxt->calc_result.envi = envi_info;

	if (0 != (smart & SMART_LNC))
		_smart_lsc_adjust(&cxt->init_param.lsc, &envi_info, ct, lsc_result);
	else
		lsc_result->update = 0;

	if (0 != (smart & SMART_CMC))
		_smart_cmc_adjust(&cxt->init_param.cmc, &envi_info, ct, cmc_result);
	else
		cmc_result->update = 0;

	if (0 != (smart & SMART_GAIN) || 0 != (smart & SMART_POST_GAIN))
		_smart_gain_adjust(&cxt->init_param.gain, &envi_info, ct, &calc_param->gain,
					&cxt->gain_factor_queue, quick_mode, gain_result);
	else
		gain_result->update = 0;

	if (0 != (smart & SMART_SATURATION))
		_smart_saturation(&cxt->init_param.saturation, &envi_info, ct, saturation_result);
	else
		saturation_result->update = 0;

	if (0 != (smart & SMART_HUE))
		_smart_hue(&cxt->init_param.hue, &envi_info, ct, hue_result);
	else
		hue_result->update = 0;

	if (0 != (smart & SMART_SATURATION) || 0 != (smart & SMART_HUE)) {

		struct smart_light_gain gain = {0};

		_convert_hue_saturation_to_gain(hue_result->value, saturation_result->value, &gain);

		/*clear the initialized value for the first time at quick mode*/
		if (0 != quick_mode) {
			_rgb_queue_clear(&cxt->hue_sat_gain_queue);
			SMART_LOGI("hue sat gain use quick mode for the first time");
		}

		_rgb_queue_add(&cxt->hue_sat_gain_queue, &gain);
		_rgb_queue_average(&cxt->hue_sat_gain_queue, &gain);

		hue_sat_result->r_gain = gain.r;
		hue_sat_result->g_gain = gain.g;
		hue_sat_result->b_gain = gain.b;
		hue_sat_result->update = 1;

		SMART_LOGV("hue/saturation: update=%d, gain = (%d, %d, %d)",
			calc_result->hue_saturation.update, calc_result->hue_saturation.r_gain, calc_result->hue_saturation.g_gain,
			calc_result->hue_saturation.b_gain);
	}
	else {
		hue_sat_result->r_gain = SMART_HUE_SAT_GAIN_UNIT;
		hue_sat_result->g_gain = SMART_HUE_SAT_GAIN_UNIT;
		hue_sat_result->b_gain = SMART_HUE_SAT_GAIN_UNIT;
		hue_sat_result->update = 1;
	}

	if (0 != (smart & SMART_POST_GAIN)) {

		hue_sat_result->r_gain = hue_sat_result->r_gain * gain_result->factor.r/ SMART_WEIGHT_UNIT;
		hue_sat_result->g_gain = hue_sat_result->g_gain * gain_result->factor.g / SMART_WEIGHT_UNIT;
		hue_sat_result->b_gain = hue_sat_result->b_gain * gain_result->factor.b / SMART_WEIGHT_UNIT;

		gain_result->factor.r = SMART_WEIGHT_UNIT;
		gain_result->factor.g = SMART_WEIGHT_UNIT;
		gain_result->factor.b = SMART_WEIGHT_UNIT;
		gain_result->gain.r = calc_param->gain.r;
		gain_result->gain.g = calc_param->gain.g;
		gain_result->gain.b = calc_param->gain.b;
		hue_sat_result->update = 1;

		SMART_LOGV("post gain: update=%d, gain = (%d, %d, %d)",
			hue_sat_result->update, hue_sat_result->r_gain, hue_sat_result->g_gain,
			hue_sat_result->b_gain);
	}

	if (0 != (smart & SMART_LNC_DENOISE)) {

		_smart_denoise_adjust(&cxt->init_param.denoise, calc_param->bv, denoise_result);
	}

	*calc_result = cxt->calc_result;

	_print_debug_info(cxt);

	SMART_LOGV("bv=%d, ct=%d, quick_mode=%d", calc_param->bv, calc_param->ct, calc_param->quick_mode);

	SMART_LOGV("envi: update=%d, envi_id = (%d, %d), weight=(%d, %d)",
		calc_result->envi.update, calc_result->envi.envi_id[0], calc_result->envi.envi_id[1], calc_result->envi.weight[0],
		calc_result->envi.weight[1]);

	SMART_LOGV("lsc: update=%d, index = (%d, %d), weight=(%d, %d)",
		calc_result->lsc.update, calc_result->lsc.index[0], calc_result->lsc.index[1],
		calc_result->lsc.weight[0], calc_result->lsc.weight[1]);

	SMART_LOGV("cmc: update=%d, index = (%d, %d), weight=(%d, %d)",
		calc_result->cmc.update, calc_result->cmc.index[0], calc_result->cmc.index[1],
		calc_result->cmc.weight[0], calc_result->cmc.weight[1]);

	SMART_LOGV("gain: update=%d, gain = (%d, %d, %d)",
		calc_result->gain.update, calc_result->gain.gain.r, calc_result->gain.gain.g,
		calc_result->gain.gain.b);

	SMART_LOGV("sat: update=%d, value=%d)",
		calc_result->saturation.update, calc_result->saturation.value);

	SMART_LOGV("hue: update=%d, value=%d)",
		calc_result->hue.update, calc_result->hue.value);

	SMART_LOGV("sat_lsc gain: update=%d, gain = (%d, %d, %d)",
		calc_result->hue_saturation.update, calc_result->hue_saturation.r_gain, calc_result->hue_saturation.g_gain,
		calc_result->hue_saturation.b_gain);

	SMART_LOGV("denoise: update=%d, ratio=%d", denoise_result->update, denoise_result->lsc_dec_ratio);

	return SMART_LIGHT_SUCCESS;
}

int32_t smart_light_deinit(uint32_t handler_id, void *in_param, void *out_param)
{
	struct smart_light_context *cxt = _get_context(handler_id);

	if (NULL == cxt)
		return SMART_LIGHT_ERROR;

	cxt->init = 0;
	_deinit_rgb_queue(&cxt->gain_factor_queue);
	_deinit_rgb_queue(&cxt->hue_sat_gain_queue);
	_deinit_normal_queue(&cxt->bv_queue);
	_deinit_normal_queue(&cxt->bv_delta_queue);
	_deinit_rgb_queue(&cxt->envi_queue);
	_deinit_debug_file(cxt);

	return SMART_LIGHT_SUCCESS;
}
/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------*/
