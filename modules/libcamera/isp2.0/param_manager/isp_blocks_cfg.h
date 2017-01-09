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
 #ifndef _ISP_BLOCKS_CFG_H_
 #define _ISP_BLOCKS_CFG_H_

#include "isp_type.h"

#ifdef	 __cplusplus
extern	 "C"
{
#endif

#define ISP_PM_CMC_MAX_INDEX 9
#define ISP_PM_CMC_SHIFT 18

struct isp_block_operations {
	int32_t (*init)(void *blk_ptr, void* param_ptr0, void* param_ptr1, void* param_ptr2);
	int32_t (*set)(void *blk_ptr, uint32_t cmd, void* param_ptr0, void*param_ptr1);
	int32_t (*get)(void *blk_ptr, uint32_t cmd, void* param_ptr0, void*param_ptr1);
	int32_t (*reset)(void *blk_ptr, uint32_t size);
	int32_t (*deinit)(void *blk_ptr);
};

struct isp_block_cfg {
	uint32_t id;
	uint32_t offset;
	uint32_t param_size;
	struct isp_block_operations *ops;
};

struct isp_block_cfg* isp_pm_get_block_cfg(uint32_t id);

#ifdef	 __cplusplus
}
#endif
 #endif //
