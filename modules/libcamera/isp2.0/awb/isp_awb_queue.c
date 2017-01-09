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


#include "isp_awb_queue.h"
/**---------------------------------------------------------------------------*
 ** 				Compiler Flag					*
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif
/**---------------------------------------------------------------------------*
**				Micro Define					*
**----------------------------------------------------------------------------*/
#define ABS(_x)   ((_x) < 0 ? -(_x) : (_x))
/**---------------------------------------------------------------------------*
**				Data Structures					*
**---------------------------------------------------------------------------*/


/**---------------------------------------------------------------------------*
**				extend Variables and function			*
**---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**				Local Variables					*
**---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**				Constant Variables					*
**---------------------------------------------------------------------------*/

/**---------------------------------------------------------------------------*
**				Constant Variables				*
**---------------------------------------------------------------------------*/


/**---------------------------------------------------------------------------*
** 				Public Function Prototypes				*
**---------------------------------------------------------------------------*/
int32_t _initQueue(struct awbl_cyc_queue *queue, uint32_t size)
{
	if (NULL == queue || size > AWBL_MAX_QUEUE_SIZE)
	{
		return -1;
	}

	memset(queue->q, 0, sizeof(uint32_t) * AWBL_MAX_QUEUE_SIZE);

	queue->size = size;
	queue->cur_index = 0;
	queue->gradient = 0;

	return 0;
}

void _addToCycQueue(struct awbl_cyc_queue *queue, uint32_t value)
{
	uint32_t cur_index = queue->cur_index;
	uint32_t *q = queue->q;

	if (NULL == queue)
		return;

	//the queue is full, drop the first one
	if (cur_index == queue->size)
	{
		uint32_t i = 0;

		for (i=1; i<queue->size; i++)
		{
			q[i-1] = q[i];
		}

		cur_index = queue->size - 1;
		q[cur_index] = value;
	}
	else
	{
		q[cur_index++] = value;
		queue->cur_index = cur_index;
	}
}

int32_t _isQueueFull(struct awbl_cyc_queue *queue)
{
	if (NULL == queue)
		return -1;

	if (queue->cur_index < queue->size)
		return 0;
	else
		return 1;
}

uint32_t _calcAvgValueOfQueue(struct awbl_cyc_queue *queue)
{
	uint32_t avg = 0;
	uint32_t sum = 0;
	int32_t i = 0;
	uint32_t *q = NULL;
	int32_t size = 0;

	if (NULL == queue)
		return 0;

	q = queue->q;

	size = (queue->cur_index < queue->size)
		? queue->cur_index : queue->size;

	if (0 == size)
		return 0;

	for (i=0; i<size; i++)
	{
		sum += q[i];
	}

	avg = sum / size;

	return avg;
}

uint32_t _calcMaxValueOfQueue(struct awbl_cyc_queue *queue)
{
	int32_t size = 0;
	int32_t i = 0;
	uint32_t max = 0;
	uint32_t *q = NULL;


	if (NULL == queue)
		return 0 ;

	q = queue->q;

	size = (queue->cur_index < queue->size) ? queue->cur_index : queue->size;

	if ( 0 == size)
		return 0;

	for (i=0; i<size; i++)
	{
		if (q[i] > max){
			max = q[i];
		}
	}

	return max;
}
uint32_t _calcMinValueOfQueue(struct awbl_cyc_queue *queue)
{
	int32_t size = 0;
	int32_t i = 0;
	uint32_t min = 0;
	uint32_t *q = NULL;


	if (NULL == queue)
		return 0 ;

	q = queue->q;

	size = (queue->cur_index < queue->size) ? queue->cur_index : queue->size;

	if ( 0 == size)
		return 0;

	for (i=0; i<size; i++)
	{
		if (q[i] < min){
			min = q[i];
		}
	}

	return min;
}
int32_t _calcDeltaValueOfQueue(struct awbl_cyc_queue *queue)
{
	int32_t i = 0;
	uint32_t *q = NULL;
	int32_t size = 0;
	int32_t delta = 0;

	if (NULL == queue)
		return 0;

	q = queue->q;

	size = (queue->cur_index < queue->size)
		? queue->cur_index : queue->size;

	if (size < 2)
		return 0;

	for (i=1; i<size; i++)
	{
		delta += (int32_t)q[i] - (int32_t)q[i-1];
	}

	return delta;
}

int32_t _calcDeltaOfQueue(struct awbl_cyc_queue *queue)
{
	int32_t size = 0;
	uint32_t delta = 0;
	uint32_t *q = NULL;
	uint32_t index = 0;
	if (NULL == queue)
		return 0;

	q = queue->q;

	size = (queue->cur_index < queue->size) ? queue->cur_index : queue->size;

	if (size < 2)
		return 0;

	//index = queue->cur_index;

	delta = ABS((int32_t)q[size-1] - (int32_t)q[size -2]);
	//delta_abs = ABS(delta);

	return delta;
}

uint32_t _calcNumOfQueue(struct awbl_cyc_queue *queue, uint32_t statis_value)
{
	int32_t size = 0;
	int32_t i = 0;
	uint32_t *q = NULL;
	uint32_t num = 0;

	if (NULL == queue)
		return 0;
	q = queue->q;

	size = (queue->cur_index < queue->size) ? queue->cur_index : queue->size;
	if (0 == size)
		return 0;

	for (i=0; i<size; i++)
	{
		if (statis_value == q[i]){
			num++;
		}
	}

	return num;
}
uint32_t _calc_weighted_average(struct awbl_cyc_queue *queue_value,
					struct awbl_cyc_queue *queue_weight)
{
	uint32_t avg = 0;
	uint32_t sum = 0;
	int32_t i = 0;
	uint32_t *qv = NULL;
	uint32_t *qw = NULL;
	int32_t size_v = 0;
	int32_t size_w = 0;
	uint32_t weight_sum = 0;

	if (NULL == queue_value || NULL == queue_weight)
		return 0;

	qv = queue_value->q;
	qw = queue_weight->q;

	size_v = (queue_value->cur_index < queue_value->size)
		? queue_value->cur_index : queue_value->size;

	size_w = (queue_weight->cur_index < queue_weight->size)
		? queue_weight->cur_index : queue_weight->size;

	if (size_v != size_w || 0 == size_w)
		return 0;

	for (i=0; i<size_w; i++)
	{
		qw[i] = 256;
	}

        if(size_w>1)
        {

                qw[0] = qw[0] - 60;
		qw[size_w-1] = qw[size_w -1] + 60;

        }

	for (i=0; i<size_v; i++)
	{
		//AWB_LOGI("gid: [%d]: value=%d, weight=%d", i, qv[i], qw[i]);
		sum += qv[i] * qw[i];
		weight_sum += qw[i];
	}

	if (weight_sum > 0)
		avg = sum / weight_sum;


	return avg;
}

void _clear(struct awbl_cyc_queue *queue)
{
	if (NULL == queue)
		return;

	queue->cur_index = 0;
}

queue_handle_t queue_init(uint32_t size)
{
	struct awbl_cyc_queue *queue = NULL;

	if (0 == size || size > AWBL_MAX_QUEUE_SIZE)
		return 0;

	queue = (struct awbl_cyc_queue *)malloc(sizeof(*queue));
	if (NULL == queue)
		return 0;

	_initQueue(queue, size);

	return (queue_handle_t)queue;
}

void queue_add(queue_handle_t queue, uint32_t value)
{
	_addToCycQueue((struct awbl_cyc_queue *)queue, value);
}

uint32_t queue_average(queue_handle_t queue)
{
	return _calcAvgValueOfQueue((struct awbl_cyc_queue *)queue);
}

uint32_t queue_max(queue_handle_t queue)
{
	return _calcMaxValueOfQueue((struct awbl_cyc_queue *)queue);
}

uint32_t queue_min(queue_handle_t queue)
{
	return _calcMinValueOfQueue((struct awbl_cyc_queue *)queue);
}

uint32_t queue_delta(queue_handle_t queue)
{
	return _calcDeltaOfQueue((struct awbl_cyc_queue *)queue);
}
uint32_t queue_weighted_average(queue_handle_t queue_value, queue_handle_t queue_weight)
{
	return _calc_weighted_average((struct awbl_cyc_queue *)queue_value,
					(struct awbl_cyc_queue *)queue_weight);
}

uint32_t queue_statis(queue_handle_t queue, uint32_t statis_value)
{
	return _calcNumOfQueue((struct awbl_cyc_queue *)queue, statis_value);
}

void queue_clear(queue_handle_t queue)
{
	_clear((struct awbl_cyc_queue *)queue);
}

void queue_deinit(queue_handle_t queue)
{
	if (NULL != (struct awbl_cyc_queue *)queue) {
		free((struct awbl_cyc_queue *)queue);
		queue = 0;
	}
}

/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef	__cplusplus
}
#endif
/**---------------------------------------------------------------------------*/

