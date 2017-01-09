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
#ifndef _AWB_ALG_PARAM_H_
#define _AWB_ALG_PARAM_H_

/*------------------------------------------------------------------------------*
*				Dependencies					*
*-------------------------------------------------------------------------------*/
#ifndef WIN32
#include <sys/types.h>
#else
#include "sci_types.h"
#endif

#define AWB_ALG_PACKET_EN

#ifndef AWB_ALG_PACKET_EN
#include "sensor_raw.h"
#endif
/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif
/*------------------------------------------------------------------------------*
				Micro Define					*
*-------------------------------------------------------------------------------*/
#define AWB_ALG_ENVI_NUM	8
#define AWB_ALG_SCENE_NUM	4
#define AWB_ALG_PIECEWISE_SAMPLE_NUM	16
#define AWB_ALG_CT_INFO_NUM	8
#define AWB_ALG_WIN_COORD_NUM	20
#define AWB_CTRL_ENVI_NUM 8
#define AWB_CTRL_ENVI_NUM 8
/*------------------------------------------------------------------------------*
*				Data Structures					*
*-------------------------------------------------------------------------------*/
struct awb_alg_gain{
	uint16_t r;
	uint16_t g;
	uint16_t b;
	uint16_t padding;
};

struct awb_alg_range {
	short min;
	short max;
};

struct awb_alg_bv {
	uint32_t enable;
	uint32_t num;
	struct awb_alg_range bv_range[AWB_CTRL_ENVI_NUM];
};
struct awb_alg_ref_param {
	struct awb_alg_gain gain;
	uint32_t ct;
	uint32_t enable;
};

struct awb_alg_coord {
	uint16_t x;
	uint16_t yb;
	uint16_t yt;
	uint16_t padding;
};

struct awb_alg_sample {
	int16_t		x;
	int16_t		y;
};

struct awb_alg_piecewise_func {
	uint32_t num;
	struct awb_alg_sample samples[AWB_ALG_PIECEWISE_SAMPLE_NUM];
};

struct awb_alg_weight_of_ct_func {
	struct awb_alg_piecewise_func weight_func;
	uint32_t base;
};

struct awb_alg_ct_info {
	int32_t data[AWB_ALG_CT_INFO_NUM];
};

struct awb_alg_value_range {
	int16_t min;
	int16_t max;
};

struct awb_alg_param_common {

	/*common parameters*/
	struct awb_alg_gain init_gain;
	uint32_t init_ct;
	uint32_t quick_mode;
	uint32_t alg_id;

	/*parameters for alg 0*/
	/*white window*/
	struct awb_alg_coord win[AWB_ALG_WIN_COORD_NUM];
	uint8_t target_zone;

	/*parameters for alg 1*/
	struct awb_alg_ct_info ct_info;
	struct awb_alg_weight_of_ct_func weight_of_ct_func[AWB_ALG_ENVI_NUM];
	struct awb_alg_weight_of_ct_func weight_of_count_func;
	struct awb_alg_value_range value_range[AWB_ALG_ENVI_NUM];
	struct awb_alg_ref_param ref_param[AWB_ALG_ENVI_NUM];
	uint32_t scene_factor[AWB_ALG_SCENE_NUM];
	uint32_t steady_speed;
	uint32_t debug_level;
};

struct awb_alg_pos_weight{
	uint8_t *data;
	uint16_t width;
	uint16_t height;
};

struct awb_alg_map {
	uint16_t *data;
	uint32_t size;
};

struct awb_alg_tuning_param {
	struct awb_alg_param_common common;
	struct awb_alg_pos_weight pos_weight;
	struct awb_alg_map win_map;
};

#ifndef AWB_ALG_PACKET_EN
struct awb_param {
	struct sensor_awb_param sen_awb_param;
	struct sensor_awb_map awb_map;
	struct sensor_awb_weight awb_weight;
};
#endif

struct awb_alg_weight {
	uint16_t *data;
	uint32_t size;
};
/*------------------------------------------------------------------------------*
*				Functions					*
*-------------------------------------------------------------------------------*/

#ifndef AWB_ALG_PACKET_EN
int32_t awb_alg_param_packet(struct awb_param *awb_param,  uint32_t pack_buf_size, void *pack_buf);
#endif
int32_t awb_alg_param_unpacket(void *pack_data, uint32_t data_size, struct awb_alg_tuning_param *alg_param);

/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/
#endif
// End

