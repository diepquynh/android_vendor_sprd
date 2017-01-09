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
#ifndef _ISP_SMART_LIGHT_H_
#define _ISP_SMART_LIGHT_H_
/*------------------------------------------------------------------------------*
*				Dependencies					*
*-------------------------------------------------------------------------------*/
#ifdef WIN32
#include "sci_types.h"
#else
#include <sys/types.h>
#endif
/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif
/*------------------------------------------------------------------------------*
*				Micro Define					*
*-------------------------------------------------------------------------------*/
#define SMART_NONE 		0x00		//none
#define SMART_LNC 		(1 << 0)	//bit0: smart lsc
#define SMART_CMC 		(1 << 1)	//bit1: smart cmc
#define SMART_ENVI 		(1 << 2)	//bit2: multi white windows
#define SMART_GAIN 		(1 << 3)	//bit3: adjust gain
#define SMART_HUE 		(1 << 4)	//bit4: smart hue
#define SMART_SATURATION 	(1 << 5)	//bit 5: smart saturation
#define SMART_POST_GAIN		(1 << 6)	//bit6: adjust gain after gamma, in cce
#define SMART_LNC_DENOISE	(1 << 7)	//bit7: adjust denoise according to ae index

#define SMART_CT_SECTION_NUM		9
#define SMART_LSC_NUM			9
#define SMART_CMC_NUM			9
#define SMART_HUE_NUM 			7
#define SMART_SAT_NUM 			7
#define SMART_WEIGHT_NUM		2

#define SMART_WEIGHT_UNIT		256
#define SMART_INVALID_INDEX 		(-1)
#define SMART_LIGHT_SUCCESS		0
#define SMART_LIGHT_ERROR		255

#define SMART_ENVI_RANGE_NUM		5
#define SMART_GAIN_FACTOR_NUM		5

#define SMART_PIECEWISE_MAX_NUM		16
#define SMART_ENVI_SIMPLE_MAX_NUM	6

#define SMART_HUE_SAT_GAIN_UNIT		1024
#define SMART_MAX_LSC_DEC_RATIO		100

#define SMART_MAX_BV			0x7fff
/*------------------------------------------------------------------------------*
*				Data Structures					*
*-------------------------------------------------------------------------------*/
enum smart_light_envi_id {
	/*unknown environment*/
	SMART_ENVI_COMMON = 0,
	/*low light*/
	SMART_ENVI_LOW_LIGHT = 0x01,
	/*normal indoor*/
	SMART_ENVI_INDOOR_NORMAL = 0x02,
	/*normal outdoor*/
	SMART_ENVI_OUTDOOR_NORMAL = 0x03,
	/*outdoor middle light*/
	SMART_ENVI_OUTDOOR_MIDDLE = 0x04,
	/*outdoor high light*/
	SMART_ENVI_OUTDOOR_HIGH = 0x05,
	/*number of simple environment*/
	SMART_ENVI_SIMPLE_NUM = 0x06,

	/*mixed low light and normal indoor*/
	SMART_ENVI_MIX_LOW_LIGHT_INDOOR = ((SMART_ENVI_LOW_LIGHT << 4) & 0xf0) | (SMART_ENVI_INDOOR_NORMAL & 0xf),
	/*mixed indoor and normal outdoor*/
	SMART_ENVI_MIX_INDOOR_OUTDOOR = ((SMART_ENVI_INDOOR_NORMAL << 4) & 0xf0) | (SMART_ENVI_OUTDOOR_NORMAL & 0xf),
	/*mixed noram outdoor and outdoor middle light*/
	SMART_ENVI_MIX_OUTDOOR_NORMAL_MIDDLE = ((SMART_ENVI_OUTDOOR_NORMAL << 4) & 0xf0) | (SMART_ENVI_OUTDOOR_MIDDLE & 0xf),
	/*mixed indoor and normal outdoor*/
	/*mixed out door middle light and outdoor high light*/
	SMART_ENVI_MIX_OUTDOOR_MIDDLE_HIGH = ((SMART_ENVI_OUTDOOR_MIDDLE << 4) & 0xf0) | (SMART_ENVI_OUTDOOR_HIGH & 0xf)
};

struct smart_light_sample {
	int16_t		x;
	int16_t		y;
};

struct smart_light_piecewise_func {
	uint32_t num;
	struct smart_light_sample samples[SMART_PIECEWISE_MAX_NUM];
};

struct smart_light_gain {
	uint16_t r;
	uint16_t g;
	uint16_t b;
};

struct smart_light_range {
	int16_t min;
	int16_t max;
};

struct smart_light_range_l {
	int32_t min;
	int32_t max;
};

struct smart_light_envi_result {
	enum smart_light_envi_id envi_id[SMART_WEIGHT_NUM];
	uint16_t weight[SMART_WEIGHT_NUM];
	uint32_t update;
};

struct smart_light_hue_result {
	int16_t value;
	uint32_t update;
};

struct smart_light_saturation_result {
	uint16_t value;
	uint32_t update;
};

struct smart_light_hue_saturation_result {
	uint16_t r_gain;
	uint16_t g_gain;
	uint16_t b_gain;
	uint32_t update;
};

struct smart_light_lsc_result {
	uint16_t index[SMART_WEIGHT_NUM];
	uint16_t weight[SMART_WEIGHT_NUM];
	uint32_t update;
};

struct smart_light_cmc_result {
	uint16_t index[SMART_WEIGHT_NUM];
	uint16_t weight[SMART_WEIGHT_NUM];
	uint32_t update;
};

struct smart_light_gain_result {
	struct smart_light_gain gain;
	struct smart_light_gain factor;
	uint32_t update;
};

struct smart_light_envi_param {
	struct smart_light_range_l bv_range[SMART_ENVI_SIMPLE_NUM];
};

struct smart_light_hue_param
{
	/*adjust function for each enviroment, x=ct, y=hue*/
	struct smart_light_piecewise_func adjust_func[SMART_ENVI_SIMPLE_MAX_NUM];
};

struct smart_light_saturation_param
{
	/*adjust function for each enviroment, x=ct, y=saturation value*/
	struct smart_light_piecewise_func adjust_func[SMART_ENVI_SIMPLE_MAX_NUM];
};

struct smart_light_lsc_param {
	/*adjust function for each enviroment, x=ct, y=index*/
	struct smart_light_piecewise_func adjust_func[SMART_ENVI_SIMPLE_MAX_NUM];
};

struct smart_light_cmc_param {
	/*adjust function for each enviroment, x=ct, y=index*/
	struct smart_light_piecewise_func adjust_func[SMART_ENVI_SIMPLE_MAX_NUM];
};

struct smart_light_gain_param {
	/*adjust function for each enviroment, x=ct, y=gain factor*/
	struct smart_light_piecewise_func r_gain_func[SMART_ENVI_SIMPLE_MAX_NUM];
	struct smart_light_piecewise_func g_gain_func[SMART_ENVI_SIMPLE_MAX_NUM];
	struct smart_light_piecewise_func b_gain_func[SMART_ENVI_SIMPLE_MAX_NUM];
};

struct smart_light_denoise_param {
	struct smart_light_range_l bv_range;
	/*value range: 0-100*/
	struct smart_light_range lsc_dec_ratio_range;
};

struct smart_light_denoise_result {
	uint32_t lsc_dec_ratio;
	uint32_t update;
};

struct smart_light_init_param {
	struct smart_light_envi_param envi;
	struct smart_light_lsc_param lsc;
	struct smart_light_cmc_param cmc;
	struct smart_light_gain_param gain;
	struct smart_light_hue_param hue;
	struct smart_light_saturation_param saturation;
	struct smart_light_denoise_param denoise;
	struct smart_light_gain_result init_gain;
	struct smart_light_hue_saturation_result init_hue_sat;
	struct smart_light_lsc_result lsc_init;
	struct smart_light_cmc_result cmc_init;
	uint32_t steady_speed;
};

struct smart_light_calc_param {
	uint32_t smart;
	int32_t bv;
	uint32_t ct;
	struct smart_light_gain gain;
	uint32_t quick_mode;
};


struct smart_light_calc_result {
	struct smart_light_envi_result envi;
	struct smart_light_lsc_result lsc;
	struct smart_light_cmc_result cmc;
	struct smart_light_gain_result gain;
	struct smart_light_hue_result hue;
	struct smart_light_saturation_result saturation;
	struct smart_light_hue_saturation_result hue_saturation;
	struct smart_light_denoise_result denoise;
};
/*------------------------------------------------------------------------------*
*				Data Prototype					*
*-------------------------------------------------------------------------------*/
int32_t smart_light_init(uint32_t handler_id, void *in_param, void *out_param);
int32_t smart_light_calculation(uint32_t handler_id, void *in_param, void *out_param);
int32_t smart_light_deinit(uint32_t handler_id, void *in_param, void *out_param);
/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/
#endif
// End


