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
#ifndef _ISP_AWB_H_
#define _ISP_AWB_H_
/*------------------------------------------------------------------------------*
*				Dependencies					*
*-------------------------------------------------------------------------------*/
#ifndef WIN32
#include <linux/types.h>
#else
#include "sci_types.h"

struct isp_size{
	uint32_t w;
	uint32_t h;
};

struct isp_awb_statistic_info{
	uint32_t r_info[1024];
	uint32_t g_info[1024];
	uint32_t b_info[1024];
};

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
#define ISP_AWB_WEIGHT_TAB_NUM 0x03
#define ISP_AWB_TEMPERATRE_NUM 0x14

#define ISP_AWB_SKIP_FOREVER 0xFFFFFFFF
#define ISP_AWB_TRIM_CONTER 0x0C
#define ISP_AWB_WINDOW_CONTER 0x03
#define ISP_AWB_SETTING_NUM 	0x04

#define ISP_AWB_ENVI_NUM 0x8
#define ISP_AWB_SCENE_NUM 0x4
#define ISP_AWB_PIECEWISE_SAMPLE_NUM 0x10
#define ISP_AWB_CT_INFO_NUM 0x8

#define ISP_AWB_GAIN_UNIT	256
/*------------------------------------------------------------------------------*
*				Data Structures					*
*-------------------------------------------------------------------------------*/
struct isp_awb_stat{
	uint32_t* r_ptr;
	uint32_t* g_ptr;
	uint32_t* b_ptr;
};

struct isp_awb_coord{
	uint16_t x;
	uint16_t yb;
	uint16_t yt;
};

struct isp_awb_gain{
	uint16_t r;
	uint16_t g;
	uint16_t b;
};

struct isp_awb_weight_lut {
	uint8_t *weight;
	uint16_t w;
	uint16_t h;
};

struct isp_awb_sample {
	int16_t x;
	int16_t y;
};

struct isp_awb_piecewise_func {
	uint32_t num;
	struct isp_awb_sample samples[ISP_AWB_PIECEWISE_SAMPLE_NUM];
};

struct isp_awb_range {
	int16_t	min;
	int16_t	max;
};

struct isp_awb_weight_of_ct_func {
	struct isp_awb_piecewise_func weight_func;
};

struct isp_awb_weight_of_count_func {
	struct isp_awb_piecewise_func weight_func;
	uint16_t base;
};

struct isp_awb_map {
	uint16_t *addr;
	uint32_t len;		//by bytes
};

struct isp_awb_ct_info {
	int32_t data[ISP_AWB_CT_INFO_NUM];
};

enum isp_awb_envi_id {
	ISP_AWB_ENVI_COMMON = 0,
	ISP_AWB_ENVI_LOW_LIGHT = 1,
	ISP_AWB_ENVI_INDOOR = 2,
	ISP_AWB_ENVI_OUTDOOR_LOW = 3,
	ISP_AWB_ENVI_OUTDOOR_MIDDLE = 4,
	ISP_AWB_ENVI_OUTDOOR_HIGH = 5,
	/*mixed low light and normal indoor*/
	ISP_AWB_ENVI_MIX_LOW_LIGHT_INDOOR = 6,
	/*mixed indoor and normal outdoor*/
	ISP_AWB_ENVI_MIX_INDOOR_OUTDOOR = 7,
	/*mixed noram outdoor and outdoor middle light*/
	ISP_AWB_ENVI_MIX_OUTDOOR_NORMAL_MIDDLE = 8,
	/*mixed out door middle light and outdoor high light*/
	ISP_AWB_ENVI_MIX_OUTDOOR_MIDDLE_HIGH = 9
};

enum isp_awb_scene_id {
	ISP_AWB_SCENE_GREEN = 1,
	ISP_AWB_SCENE_SKIN = 2
};

enum isp_awb_cmd {
	ISP_AWB_CMD_SET_BASE 			= 0x1000,
	ISP_AWB_CMD_GET_BASE			= 0x2000,
	ISP_AWB_CMD_GET_CALC_DETAIL		= (ISP_AWB_CMD_GET_BASE + 1)
};

struct isp_awb_scene_info {
	uint32_t ratio;
	struct isp_awb_gain gain;
};

struct isp_awb_ref_param {
	struct isp_awb_gain gain;
	uint32_t ct;
	uint32_t enable;
};

struct isp_awb_init_param {
	/*common parameters*/
	/*awb alg id*/
	uint32_t alg_id;
	/*statistic image window size*/
	struct isp_size win_size;
	/*statistic image size*/
	struct isp_size img_size;
	uint32_t base_gain;

	struct isp_awb_gain init_gain;
	uint32_t init_ct;

	uint32_t quick_mode;

	/*parameters for alg 0*/
	/*white window*/
	struct isp_awb_coord win[ISP_AWB_TEMPERATRE_NUM];
	uint8_t target_zone;

	/*parameters for alg 1*/
	/*window for alg 1*/
	struct isp_awb_map map_data;
	struct isp_awb_range value_range[ISP_AWB_ENVI_NUM];
	struct isp_awb_weight_of_count_func weight_of_count_func;
	struct isp_awb_weight_of_ct_func weight_of_ct_func[ISP_AWB_ENVI_NUM];
	struct isp_awb_ct_info ct_info;
	uint32_t steady_speed;
	uint32_t debug_level;
	struct isp_awb_weight_lut weight_of_pos_lut;
	uint32_t scene_factor[ISP_AWB_SCENE_NUM];
#if 1 // ref_gain
	struct isp_awb_ref_param ref_param[ISP_AWB_ENVI_NUM];
#endif
};

struct isp_awb_calc_param {
	struct isp_awb_statistic_info *awb_stat;
	enum isp_awb_envi_id envi_id[2];
	uint16_t envi_weight[2];
	uint32_t quick_mode;
};

struct isp_awb_calc_result {
	struct isp_awb_gain gain;
	uint32_t ct;
};

struct isp_awb_detail_info {
	uint16_t *coord_img;
	uint8_t *mask_img[2];
};


/*------------------------------------------------------------------------------*
*				Data Prototype					*
*-------------------------------------------------------------------------------*/

uint32_t isp_awb_init(uint32_t handler_id, void *in_param, void *out_param);
uint32_t isp_awb_deinit(uint32_t handler_id, void *in_param, void *out_param);
uint32_t isp_awb_calculation(uint32_t handler_id, void *in_param, void *out_param);
uint32_t isp_awb_set(uint32_t handler_id, uint32_t cmd, void *in_param, void *out_param);
uint32_t isp_awb_get(uint32_t handler_id, uint32_t cmd, void *in_param, void *out_param);
/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/
#endif
// End


