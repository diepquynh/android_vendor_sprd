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
#ifndef _AE_UTILS_H_
#define _AE_UTILS_H_
/*----------------------------------------------------------------------------*
 **				 Dependencies				*
 **---------------------------------------------------------------------------*/ 
#include "ae_types.h"
#include "ae_ctrl_types.h"
/**---------------------------------------------------------------------------*
 **				 Compiler Flag				*
 **---------------------------------------------------------------------------*/ 
#ifdef __cplusplus
extern "C" {
	
#endif	/*  */
/**---------------------------------------------------------------------------*
**				Macro Define				*
**----------------------------------------------------------------------------*/
#define AE_FRAME_INFO_NUM 	8
/**---------------------------------------------------------------------------*
**				Data Structures				*
**---------------------------------------------------------------------------*/
	struct ae_ctrl_time {
		uint32_t sec;
		uint32_t usec;
	};

	struct ae_ctrl_stats_info {
		struct ae_stat_img_info info;
		struct ae_ctrl_time eof_time;
		struct ae_ctrl_time write_sensor_time;
		struct ae_ctrl_time delay_time;
		struct ae_ctrl_time frame_time;
		uint32_t skip_frame;
		uint32_t delay_frame;
		uint32_t near_stab;
		uint32_t frame_id;
	};

	struct ae_history_info {
		struct ae_ctrl_stats_info stats_info[AE_FRAME_INFO_NUM];	//save the latest 8 exposure time and gain;
		//0: latest one; 
		//list will be better
		uint32_t cur_stat_index;
		struct ae_ctrl_stats_info last_info;
		struct ae_ctrl_stats_info cur_info;
		struct ae_ctrl_stats_info effect_info;
		uint32_t sensor_effect_delay_num;
	};


/**---------------------------------------------------------------------------*
** 				Function Defination			*
**---------------------------------------------------------------------------*/
	int32_t ae_utils_calc_func(struct ae_piecewise_func *func, uint32_t y_type, int32_t x, struct ae_weight_value *result);
	int32_t ae_utils_get_effect_index(struct ae_history_info *history, uint32_t frame_id, int32_t * effect_index, int32_t * uneffect_index, uint32_t * is_only_calc_lum_flag);
	int32_t ae_utils_save_stat_info(struct ae_history_info *history, struct ae_ctrl_stats_info *cur_info);
	int32_t ae_utils_get_stat_info(struct ae_history_info *history, struct ae_ctrl_stats_info *stat_info, uint32_t cur_stat_index, uint32_t frame_id);
	int32_t ae_utils_get_delay_frame_num(struct ae_history_info *history, struct ae_ctrl_time *eof, struct ae_ctrl_time *write_sensor, uint32_t frame_time);

/**----------------------------------------------------------------------------*
**					Compiler Flag			*
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
} 
#endif	/*  */

#endif
