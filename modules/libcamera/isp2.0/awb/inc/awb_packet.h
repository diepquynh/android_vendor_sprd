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
#ifndef _AWB_PACKET_H_
#define _AWB_PACKET_H_

/*------------------------------------------------------------------------------*
*				Dependencies					*
*-------------------------------------------------------------------------------*/
#ifndef WIN32
#include <linux/types.h>
#include <sys/types.h>
#else
#include "sci_types.h"
#endif

#include "awb_alg_param.h"

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
#define AWB_PACKET_ERROR	255
#define AWB_PACKET_SUCCESS	0

#define AWB_ALG_RESOLUTION_NUM 	8
#define AWB_ALG_MWB_NUM		20
#define AWB_CTRL_SCENEMODE_NUM 10
/*------------------------------------------------------------------------------*
*				Data Structures					*
*-------------------------------------------------------------------------------*/
struct awb_alg_size {
	uint16_t w;
	uint16_t h;
};

struct awb_alg_pos {
	int16_t x;
	int16_t y;
};

struct awb_param_ctrl {
	/*window size of statistic image*/
	struct awb_alg_size stat_win_size;
	/*start position of statistic area*/
	struct awb_alg_pos stat_start_pos;
	/*compensate gain for each resolution*/
	struct awb_alg_gain compensate_gain[AWB_ALG_RESOLUTION_NUM];
	/*gain for each manual white balance*/
	struct awb_alg_gain mwb_gain[AWB_ALG_MWB_NUM];
	/*gain for each scenemode gain*/
	struct awb_alg_gain scene_gain[AWB_CTRL_SCENEMODE_NUM];
	/*bv value range for awb*/
	struct awb_alg_bv bv_range;
	/*init gain and ct*/
	struct awb_alg_gain init_gain;
	uint32_t init_ct;
};

struct awb_param_tuning {
	struct awb_param_ctrl common;
	/*algorithm param*/
	void *alg_param;
	/*algorithm param size*/
	uint32_t alg_param_size;
};
/*------------------------------------------------------------------------------*
*				Functions					*
*-------------------------------------------------------------------------------*/

//int32_t awb_param_pack(struct awb_param *awb_param, uint32_t pack_buf_size, void *pack_buf, uint32_t *pack_data_size);
int32_t awb_param_unpack(void *pack_data, uint32_t data_size, struct awb_param_tuning *tuning_param);

/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/
#endif
// End

