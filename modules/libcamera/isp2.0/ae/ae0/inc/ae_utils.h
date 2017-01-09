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
/**---------------------------------------------------------------------------*
 **				 Compiler Flag				*
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif
/**---------------------------------------------------------------------------*
**				Macro Define				*
**----------------------------------------------------------------------------*/
#define AE_HISTORY_NUM 	8
/**---------------------------------------------------------------------------*
**				Data Structures				*
**---------------------------------------------------------------------------*/

enum ae_frame_status {
	AE_FRAME_SKIP = 0,
	AE_FRAME_WRITE_SENSOR = 1
};

struct ae_frame_info {
	uint32_t frame_id;	//frame id
	uint32_t status;		//0: do nothing; 1: write exp/gain; 
	uint32_t index;
	uint32_t exp_line;
	uint32_t again;
	uint32_t dgain;
	uint32_t luma;
	uint32_t delay_num;	//frames delay after exp/gain writting
	uint32_t skip_num;	//frames need skip after written
};

struct ae_time {
	uint32_t sec;
	uint32_t usec;
};

struct ae_history {
	struct ae_frame_info queue[AE_HISTORY_NUM];
	int32_t index;		//current index for queue
	struct ae_frame_info write_sensor_queue[AE_HISTORY_NUM];
	int32_t write_sensor_index;
};
/**---------------------------------------------------------------------------*
** 				Function Defination			*
**---------------------------------------------------------------------------*/
int32_t save_frame_info(struct ae_history *history, struct ae_frame_info *frame_info);
int32_t get_frame_info(struct ae_history *history, uint32_t frame_id,  struct ae_frame_info *frame_info);
int32_t get_write_sensor_frame_info(struct ae_history *history, struct ae_frame_info *frame_info, uint32_t before_frame); 
int32_t get_write_delay(struct ae_time *eof, struct ae_time *sensor_write, uint32_t frame_time);
int32_t get_last_frame_info(struct ae_history *history, struct ae_frame_info *frame_info);
/**----------------------------------------------------------------------------*
**					Compiler Flag			*
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif

#endif