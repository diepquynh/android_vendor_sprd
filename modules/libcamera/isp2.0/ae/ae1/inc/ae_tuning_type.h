/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _AE_TUNING_TYPE_H_
#define _AE_TUNING_TYPE_H_

/*----------------------------------------------------------------------------*
 **				 Dependencies					*
 **---------------------------------------------------------------------------*/
#include "ae_types.h"
#include "mulaes.h"
#include "flat.h"
#include "touch_ae.h"
#include "ae1_face.h"
#include "region.h"
/**---------------------------------------------------------------------------*
 **				 Compiler Flag					*
 **---------------------------------------------------------------------------*/

struct ae_tuning_param {//total bytes must be 263480
	uint32_t version;
	uint32_t verify;
	uint32_t alg_id;
	uint32_t target_lum;
	uint32_t target_lum_zone;	// x16
	uint32_t convergence_speed;// x16
	uint32_t flicker_index;
	uint32_t min_line;
	uint32_t start_index;
	uint32_t exp_skip_num;
	uint32_t gain_skip_num;
	struct ae_stat_req stat_req;
	struct ae_flash_tuning flash_tuning;
	struct touch_zone touch_param;
	struct ae_ev_table ev_table;
	struct ae_exp_gain_table ae_table[AE_FLICKER_NUM][AE_ISO_NUM];
	struct ae_exp_gain_table backup_ae_table[AE_FLICKER_NUM][AE_ISO_NUM];
	struct ae_weight_table weight_table[AE_WEIGHT_TABLE_NUM];
	struct ae_scene_info scene_info[AE_SCENE_NUM];
	struct ae_auto_iso_tab auto_iso_tab;
	struct ae_exp_anti exp_anti;
	struct ae_ev_cali ev_cali;
	struct ae_convergence_parm cvgn_param[AE_CVGN_NUM];
	struct ae_touch_param touch_info;/*it is in here,just for compatible; 3 * 4bytes */
	struct ae_face_tune_param face_info;

	/*13 * 4bytes */
	uint8_t monitor_mode;	/*0: single, 1: continue */
	uint8_t ae_tbl_exp_mode;	/*0: ae table exposure is exposure time; 1: ae table exposure is exposure line */
	uint8_t enter_skip_num;	/*AE alg skip frame as entering camera */
	uint8_t cnvg_stride_ev_num;
	int8_t cnvg_stride_ev[32];
	int8_t stable_zone_ev[16];

	struct ae_sensor_cfg sensor_cfg;	/*sensor cfg information: 2 * 4bytes */

	struct ae_lv_calibration lv_cali;	/*1 * 4bytes */
	/*scene detect and others alg */
	/*for touch info */

	struct flat_tuning_param flat_param;	/*51 * 4bytes */
	struct ae_flash_tuning_param flash_param;	/*1 * 4bytes */
	struct region_tuning_param region_param;	/*180 * 4bytes */
	struct mulaes_tuning_param mulaes_param;	/*9 * 4bytes */
    struct face_tuning_param face_param;

	uint32_t reserved[2046];
};

/**---------------------------------------------------------------------------*/
#endif //_AE_TUNING_TYPE_H_
