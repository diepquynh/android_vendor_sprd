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

#ifndef _ISP_CAF_H_
#define _ISP_CAF_H_
/*------------------------------------------------------------------------------*
*					Dependencies				*
*-------------------------------------------------------------------------------*/
#include <linux/types.h>

/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/
#ifdef  __cplusplus
extern "C"
{
#endif

/*------------------------------------------------------------------------------*
*					Micro Define				*
*-------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------*
*					Data Structures				*
*-------------------------------------------------------------------------------*/

struct isp_caf_in_param{
	uint32_t data_type;
	uint32_t *data;
};

enum isp_caf_cal_type {
	CAMERA_CTN_AF_DATA_AE,
	CAMERA_CTN_AF_DATA_AWB,
	CAMERA_CTN_AF_DATA_AF,
	CAMERA_CTN_AF_DATA_MAX

};


struct isp_caf_cal_cfg{
	uint32_t awb_cal_value_threshold;
	uint32_t awb_cal_num_threshold;
	uint32_t awb_cal_value_stab_threshold;
	uint32_t awb_cal_num_stab_threshold;
	uint32_t awb_cal_cnt_stab_threshold;
	uint32_t af_cal_threshold;
	uint32_t af_cal_stab_threshold;
	uint32_t af_cal_cnt_stab_threshold;
	uint32_t awb_cal_skip_cnt;
	uint32_t af_cal_skip_cnt;
	uint32_t caf_work_lum_thr;  //percent of ae target
};


struct isp_caf_cal_param{
	uint32_t awb_r_base[1024];
	uint32_t awb_g_base[1024];
	uint32_t awb_b_base[1024];
	uint32_t awb_r_diff[1024];
	uint32_t awb_g_diff[1024];
	uint32_t awb_b_diff[1024];
	uint32_t af_base[9];
	uint32_t af_pre[9];
	uint32_t af_diff[9];
	uint32_t af_diff2[9];
	uint32_t awb_r_base_avg;
	uint32_t awb_g_base_avg;
	uint32_t awb_b_base_avg;
	uint32_t awb_r_base_sum;
	uint32_t awb_g_base_sum;
	uint32_t awb_b_base_sum;
	uint32_t awb_cal_count;
	uint32_t awb_stab_cal_count;
	uint32_t awb_stab_cnt;
	uint32_t awb_is_stab;
	uint32_t af_cal_count;
	uint32_t af_stab_cal_count;
	uint32_t af_cal_need_af;
	uint32_t af_stab_cnt;
	uint32_t af_is_stab;
	uint32_t cal_state;

};





/*------------------------------------------------------------------------------*
*					Data Prototype				*
*-------------------------------------------------------------------------------*/
uint32_t isp_continue_af_calc(uint32_t handler_id, void* param);
void isp_caf_init(uint32_t handler_id);
void isp_caf_deinit(uint32_t handler_id);
void isp_caf_reset(uint32_t handler_id);

/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/
#ifdef	 __cplusplus
}
#endif
/*-----------------------------------------------------------------------------*/
#endif
// End
