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
#ifndef _ISP_AWB_QUEUE_H_
#define _ISP_AWB_QUEUE_H_
/*------------------------------------------------------------------------------*
*				Dependencies					*
*-------------------------------------------------------------------------------*/
#include "isp_awb_types.h"
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

///////////////////////////////////////////////////////////////////////////////////
#define AWBL_MAX_QUEUE_SIZE	64

/*------------------------------------------------------------------------------*
*				Data Structures					*
*-------------------------------------------------------------------------------*/
typedef void* queue_handle_t;

struct awbl_cyc_queue
{
	uint32_t				q[AWBL_MAX_QUEUE_SIZE];
	uint32_t				size;
	uint32_t				cur_index;
	int32_t					gradient;
};

/*------------------------------------------------------------------------------*
*				Functions						*
*-------------------------------------------------------------------------------*/

//queue functions
int32_t _initQueue(struct awbl_cyc_queue *queue, uint32_t size);

void _addToCycQueue(struct awbl_cyc_queue *queue, uint32_t value);

int32_t _isQueueFull(struct awbl_cyc_queue *queue);

uint32_t _calcAvgValueOfQueue(struct awbl_cyc_queue *queue);

int32_t _calcDeltaValueOfQueue(struct awbl_cyc_queue *queue);

queue_handle_t queue_init(uint32_t size);
void queue_add(queue_handle_t queue, uint32_t value);
uint32_t queue_average(queue_handle_t queue);
uint32_t queue_max(queue_handle_t queue);
uint32_t queue_min(queue_handle_t queue);
uint32_t queue_delta(queue_handle_t queue);
uint32_t queue_statis(queue_handle_t queue, uint32_t statis_value);
uint32_t queue_weighted_average(queue_handle_t queue_value, queue_handle_t queue_weight);
void queue_deinit(queue_handle_t queue);
void queue_clear(queue_handle_t queue);

/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/
#endif
// End
