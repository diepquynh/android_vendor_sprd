/*
 * Copyright (C) 2015 The Android Open Source Project
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

#ifndef _SENSOR_EXPOSURE_QUEUE_H_
#define _SENSOR_EXPOSURE_QUEUE_H_
/*----------------------------------------------------------------------------*
 **				 Dependencies				*
 **---------------------------------------------------------------------------*/
#include "isp_type.h"
#include "ae_log.h"
#define cmr_int int32_t
#define cmr_u32 int32_t
/**---------------------------------------------------------------------------*
 **				 Compiler Flag				*
 **---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

	enum seq_work_mode {
		SEQ_WORK_PREVIEW,
		SEQ_WORK_CAPTURE,
		SEQ_WORK_MAX
	};

	struct seq_cell {
		cmr_u32 frame_id;//must start from 0
		cmr_u32 exp_line;//it is invalid value while 0
		cmr_u32 gain;//it is invalid value while 0
		cmr_u32 exp_time;
		cmr_u32 dummy;
	};

	struct seq_item {
		cmr_u32 work_mode;
		struct seq_cell cell;
	};

	struct seq_init_in {
		cmr_u32 preview_skip_num;
		cmr_u32 capture_skip_num;
		cmr_u32 exp_valid_num;
		cmr_u32 gain_valid_num;
		cmr_u32 idx_start_from;
	};

	cmr_int seq_init(cmr_u32 queue_num, struct seq_init_in *in_ptr, void **handle);
	cmr_int seq_deinit(void *handle);
	cmr_int seq_reset(void *handle);
	cmr_int seq_put(void *handle, struct seq_item *in_est_ptr, struct seq_cell *out_actual_ptr, struct seq_cell *out_write_ptr);
/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif
