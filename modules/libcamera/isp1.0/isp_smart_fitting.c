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

#include"isp_smart_fitting.h"

#define ISP_SUCCESS 0x00
#define ISP_INPUT_ERROR 0x01

struct region_info{
	uint32_t region;
	uint32_t scene1;
	uint32_t scene2;
};

enum isp_smart_region_id{
	LOWLUX = 0x00,
	NON_LOWLUX= 0x01,
	TRANS_REGION = 0x02,
	NON_TRANS_REGION = 0x03,
	MAX
};

static int32_t _isp_get_region_v0(struct isp_smart_in_param *in_param_ptr,struct region_info *region_ptr);
static int32_t _isp_get_region_v1(struct isp_smart_in_param *in_param_ptr,struct region_info *region_ptr);
static uint32_t _isp_smart_linear_interpolate(struct isp_smart_scene *scene, uint32_t value);

/*  _isp_denoise_get_region --
*@
*@
*@ return:
*/
static int32_t _isp_get_region_v0(struct isp_smart_in_param *in_param_ptr,struct region_info *region_ptr)
{
	uint32_t i;
	int32_t rtn = 0;
	struct isp_smart_scene_info *scene_info = &in_param_ptr->scene_info;
	uint32_t scene_count = scene_info->scene_count;
	uint32_t bv_count = in_param_ptr->bv_info.bv_count;
	int32_t cur_bv = in_param_ptr->cur_bv;
	uint32_t is_low_lux = in_param_ptr->low_lux;
	struct isp_smart_bv_thr *bv = in_param_ptr->bv_info.bv;

	//low_lux is the first scene
	if(1==is_low_lux){
		region_ptr->region = LOWLUX;
		region_ptr->scene1 = 0;
		goto GET_REGION_EXIT;
	}

	//not low_lux scene begins
	if(1==scene_count || 2==scene_count){
		region_ptr->region = NON_LOWLUX;
		region_ptr->scene1 = scene_count-1;
		goto GET_REGION_EXIT;
	}

	//scene_count>=3, and bv is from high to low from the sceond scene
	if(bv_count+2 > scene_count){
		bv_count = scene_count - 2;
	}

	for(i=0; i<bv_count; i++){
		if(bv[i].bv_low<=bv[i].bv_high){
			if(cur_bv<bv[i].bv_low){
				region_ptr->region = NON_LOWLUX;
				region_ptr->scene1 = i+1;
				break;
			}
			if(cur_bv<bv[i].bv_high){
				region_ptr->region = TRANS_REGION;
				region_ptr->scene1 = i+1;
				region_ptr->scene2 = i+2;
				break;
			}
		}else{
			rtn = ISP_INPUT_ERROR;
		}
	}

	//if cur_bv<=bv[bv_count-1].bv_low
	if(i==bv_count){
		region_ptr->region = NON_LOWLUX;
		region_ptr->scene1 = bv_count+1;
	}

	GET_REGION_EXIT:
	return rtn;
}

/* _isp_smart_linear_interpolate --
*@
*@
*@ return:
*/
static uint32_t _isp_smart_linear_interpolate(struct isp_smart_scene *scene, uint32_t value)
{
	uint32_t level;
	uint32_t count = scene->sample_count;
	struct isp_smart_sample *sample = scene->sample_group;
	uint32_t i = 0;
	int32_t level1;
	int32_t level2;
	int32_t gain1;
	int32_t gain2;
	int32_t temp;

	if(0==count || 1==count){
		level = 0;
		return level;
	}

	for(i=0;i<count-1;i++){
		if(value<sample[i+1].gain && value>=sample[i].gain){
			break;
		}
	}

	if(value<sample[0].gain){
		i = 0;
	}
	if(value>sample[count-1].gain){
		i = count - 2;
	}

	level1 = sample[i].level;
	level2 = sample[i+1].level;
	gain1 = sample[i].gain;
	gain2 = sample[i+1].gain;

	if(gain2!=gain1){
		temp = (int32_t)(level1+(level2-level1)*(value-gain1)*1.0/(gain2-gain1)+0.5);
	}else{
		temp = (int32_t)((level1+level2)/2+0.5);
	}
	level = temp>0?(uint32_t)temp:0;

	return level;
}

/* _isp_gamma_get_region --
*@
*@
*@ return:
*/
static int32_t _isp_get_region_v1(struct isp_smart_in_param *in_param_ptr,struct region_info *region_ptr)
{
	uint32_t i;
	int32_t rtn = 0;
	struct isp_smart_scene_info *scene_info = &in_param_ptr->scene_info;
	struct isp_smart_bv_thr *bv = in_param_ptr->bv_info.bv;
	uint32_t scene_count = scene_info->scene_count;
	uint32_t bv_count = in_param_ptr->bv_info.bv_count;
	int32_t cur_bv = in_param_ptr->cur_bv;
	uint32_t is_low_lux = in_param_ptr->low_lux;

	if(1==scene_count){
		region_ptr->region = NON_TRANS_REGION;
		region_ptr->scene1 = 0;
		goto GET_REGION_EXIT;
	}

	if(bv_count+1 > scene_count){
		bv_count = scene_count - 1;
	}

	for(i=0; i<bv_count; i++){
		if(bv[i].bv_low<=bv[i].bv_high){
			if(cur_bv<bv[i].bv_low){
				region_ptr->region = NON_TRANS_REGION;
				region_ptr->scene1 = i;
				break;
			}
			if(cur_bv<bv[i].bv_high){
				region_ptr->region = TRANS_REGION;
				region_ptr->scene1 = i;
				region_ptr->scene2 = i+1;
				break;
			}
		}else{
			rtn = ISP_INPUT_ERROR;
		}
	}

	//if cur_bv<=bv[bv_count-1].bv_low
	if(i==bv_count){
		region_ptr->region = NON_TRANS_REGION;
		region_ptr->scene1 = bv_count;
	}

	GET_REGION_EXIT:
	return rtn;
}

/* isp_smart_get_level --
*@
*@
*@ return:
*/
int32_t isp_smart_get_level(struct isp_smart_in_param *in_param_ptr, uint32_t *level)
{
	int32_t rtn = ISP_SUCCESS;
	struct isp_smart_scene *scene_group = in_param_ptr->scene_info.scene_group;
	struct isp_smart_bv_thr *bv = in_param_ptr->bv_info.bv;
	struct region_info region;
	int32_t cur_bv = in_param_ptr->cur_bv;
	uint32_t cur_value;
	uint32_t level1;
	uint32_t level2;
	uint32_t scene1;
	uint32_t scene2;

	if(0==in_param_ptr->scene_info.scene_count
		|| in_param_ptr->scene_info.scene_count > MAX_SCENE_COUNT
		|| in_param_ptr->bv_info.bv_count > MAX_BV_THR_COUNT){
		*level = 0;
		goto GET_LEVEL_EXIT;
	}

	region.region = 0;
	region.scene1 = 0;
	region.scene2 = 0;
	if(ISP_INPUT_ERROR == _isp_get_region_v0(in_param_ptr,&region)){
		*level = 0;
		goto GET_LEVEL_EXIT;
	}

	switch(region.region){
	case NON_LOWLUX:
		cur_value = in_param_ptr->cur_gain;
		scene1 = region.scene1;
		*level=_isp_smart_linear_interpolate(&scene_group[scene1],cur_value);
		break;
	case LOWLUX:
		cur_value = in_param_ptr->cur_luma;
		scene1 = region.scene1;
		*level=_isp_smart_linear_interpolate(&scene_group[scene1],cur_value);
		break;
	case TRANS_REGION:
		cur_value = in_param_ptr->cur_gain;
		scene1 = region.scene1;
		scene2 = region.scene2;
		level1 = _isp_smart_linear_interpolate(&scene_group[scene1],cur_value);
		level2 = _isp_smart_linear_interpolate(&scene_group[scene2],cur_value);
		if(bv[scene1-1].bv_high != bv[scene1-1].bv_low){
			*level=(uint32_t)(((bv[scene1-1].bv_high-cur_bv)*level1 +(cur_bv-bv[scene1-1].bv_low)*level2)*1.0/(bv[scene1-1].bv_high-bv[scene1-1].bv_low)+0.5);
		}else{
			*level=(uint32_t)((level1+level2)/2+0.5);
		}
		break;
	default:
		break;
	}

	GET_LEVEL_EXIT:
	return rtn;
}

/* isp_smart_get_index --
*@
*@
*@ return:
*/
int32_t isp_smart_get_gamma_index(struct isp_smart_in_param *in_param_ptr, uint32_t *index1, uint32_t *index2, uint32_t *bv_index)
{
	int32_t rtn = ISP_SUCCESS;
	struct isp_smart_scene *scene_group = in_param_ptr->scene_info.scene_group;
	struct isp_smart_bv_thr *bv = in_param_ptr->bv_info.bv;
	struct region_info region;
	int32_t cur_bv = in_param_ptr->cur_bv;
	uint32_t cur_value;
	uint32_t scene1;
	uint32_t scene2;

	if(0==in_param_ptr->scene_info.scene_count
		|| in_param_ptr->scene_info.scene_count > MAX_SCENE_COUNT
		|| in_param_ptr->bv_info.bv_count > MAX_BV_THR_COUNT){
		*index1 = 0;
		*index2 = 0;
		*bv_index = 0;
		goto GET_INDEX_EXIT;
	}

	if(ISP_INPUT_ERROR == _isp_get_region_v1(in_param_ptr,&region)){
		*index1 = 0;
		*index2 = 0;
		*bv_index = 0;
		goto GET_INDEX_EXIT;
	}

	switch(region.region){
	case NON_TRANS_REGION:
		scene1 = region.scene1;
		*index1 = scene_group[scene1].sample_group[0].index;
		*index2 = scene_group[scene1].sample_group[0].index;
		*bv_index = scene1;
		break;
	case TRANS_REGION:
		scene1 = region.scene1;
		*index1 = scene_group[scene1].sample_group[0].index;
		scene2 = region.scene2;
		*index2 = scene_group[scene2].sample_group[0].index;
		*bv_index = scene1;
		break;
	default:
		break;
	}

	GET_INDEX_EXIT:
	return rtn;
}

/* isp_smart_get_index --
*@
*@
*@ return:
*/
int32_t isp_smart_get_index(struct isp_smart_in_param *in_param_ptr, uint32_t *index)
{
	int32_t rtn = ISP_SUCCESS;
	struct isp_smart_scene *scene_group = in_param_ptr->scene_info.scene_group;
	struct isp_smart_bv_thr *bv = in_param_ptr->bv_info.bv;
	struct region_info region;
	int32_t cur_bv = in_param_ptr->cur_bv;
	uint32_t cur_value;
	uint32_t index1;
	uint32_t index2;
	uint32_t scene1;
	uint32_t scene2;

	if(0==in_param_ptr->scene_info.scene_count
		|| in_param_ptr->scene_info.scene_count > MAX_SCENE_COUNT
		|| in_param_ptr->bv_info.bv_count > MAX_BV_THR_COUNT){
		*index = 0;
		goto GET_INDEX_EXIT;
	}

	if(ISP_INPUT_ERROR == _isp_get_region_v1(in_param_ptr,&region)){
		*index = 0;
		goto GET_INDEX_EXIT;
	}

	switch(region.region){
	case NON_TRANS_REGION:
		scene1 = region.scene1;
		*index = scene_group[scene1].sample_group[0].index;
		break;
	case TRANS_REGION:
		scene1 = region.scene1;
		index1 = scene_group[scene1].sample_group[0].index;
		scene2 = region.scene2;
		index2 = scene_group[scene2].sample_group[0].index;
		if(bv[scene1].bv_high != bv[scene1].bv_low){
			*index = (uint32_t)(((bv[scene1].bv_high-cur_bv)*index1 +(cur_bv-bv[scene1].bv_low)*index2)*1.0/(bv[scene1].bv_high-bv[scene1].bv_low)+0.5);
		}else{
			*index = (uint32_t)((index1+index2)/2 + 0.5);
		}
		break;
	default:
		break;
	}

	GET_INDEX_EXIT:
	return rtn;
}

