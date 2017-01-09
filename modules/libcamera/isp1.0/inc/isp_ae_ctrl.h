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

#ifndef _ISP_AE_CTRL_H_
#define _ISP_AE_CTRL_H_
/*----------------------------------------------------------------------------*
 **				Dependencies				*
 **---------------------------------------------------------------------------*/
#include <sys/types.h>
#include "isp_ae_alg_v00.h"
#include "isp_drv.h"

/**---------------------------------------------------------------------------*
**				Compiler Flag				*
**---------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C"
{
#endif

/**---------------------------------------------------------------------------*
**				Micro Define				**
**----------------------------------------------------------------------------*/
enum isp_ae_proc_mode{
	ISP_AE_DIR=0x00,
	ISP_AE_SOF,
	ISP_AE_PROC_MAX
};

/**---------------------------------------------------------------------------*
**					Data Prototype				**
**----------------------------------------------------------------------------*/
int32_t isp_ae_init_context(uint32_t handler_id, void *cxt);
uint32_t isp_ae_init(uint32_t handler_id);
uint32_t isp_ae_deinit(uint32_t handler_id);
uint32_t isp_ae_calculation(uint32_t handler_id);
uint32_t isp_ae_update_expos_gain(uint32_t handler_id);
uint32_t isp_ae_set_exposure_gain(uint32_t handler_id);
uint32_t isp_exp_gain_proc(uint32_t handler_id, uint32_t proc_mode, uint64_t system);
int32_t isp_ae_set_monitor_size(uint32_t handler_id, void* param_ptr);
int32_t isp_ae_set_alg(uint32_t handler_id, uint32_t mode);
int32_t isp_ae_set_ev(uint32_t handler_id, int32_t ev);
uint32_t isp_ae_set_denoise(uint32_t handler_id, uint32_t level);
int32_t isp_ae_set_denosie_level(uint32_t handler_id, uint32_t level);
int32_t isp_ae_get_denosie_level(uint32_t handler_id, uint32_t* level);
int32_t isp_ae_set_denosie_diswei_level(uint32_t handler_id, uint32_t level);
int32_t isp_ae_get_denosie_diswei_level(uint32_t handler_id, void* isp_context_ptr,uint32_t* level);
int32_t isp_ae_set_denosie_ranwei_level(uint32_t handler_id, uint32_t level);
int32_t isp_ae_get_denosie_ranwei_level(uint32_t handler_id, void* isp_context_ptr,uint32_t *level);
int32_t isp_ae_set_index(uint32_t handler_id, uint32_t index);
int32_t isp_ae_save_iso(uint32_t handler_id, uint32_t iso);
uint32_t isp_ae_get_save_iso(uint32_t handler_id);
int32_t isp_ae_set_iso(uint32_t handler_id, uint32_t iso);
int32_t isp_ae_get_iso(uint32_t handler_id, uint32_t* iso);
int32_t isp_ae_set_fast_stab(uint32_t handler_id, uint32_t eb);
int32_t isp_ae_set_stab(uint32_t handler_id, uint32_t eb);
int32_t isp_ae_get_stab(uint32_t handler_id, uint32_t* stab);
int32_t isp_ae_set_stab_ext_ctrl(uint32_t handler_id, uint32_t eb);
int32_t isp_ae_set_change(uint32_t handler_id, uint32_t eb);
int32_t isp_ae_set_param_index(uint32_t handler_id, uint32_t index);
int32_t isp_ae_ctrl_set(uint32_t handler_id, void* param_ptr);
int32_t isp_ae_ctrl_get(uint32_t handler_id, void* param_ptr);
int32_t isp_ae_get_ev_lum(uint32_t handler_id);
int32_t isp_ae_stop_callback_handler(uint32_t handler_id);
int32_t isp_ae_get_denosie_info(uint32_t handler_id, uint32_t* param_ptr);
int32_t _isp_ae_set_frame_info(uint32_t handler_id);
uint32_t _isp_ae_set_exposure_gain(uint32_t handler_id);
int32_t isp_ae_lock_init(uint32_t handler_id);
int32_t isp_ae_lock(uint32_t handler_id, void* fun);
int32_t isp_ae_unlock(uint32_t handler_id, void* fun);
int32_t isp_ae_set_skipnum_ext(uint32_t handler_id, uint32_t skipnum);
int32_t _isp_ae_monitor(uint32_t handler_id, uint32_t skip_num, const char *fun);
uint32_t isp_get_cur_lum(uint32_t handler_id, uint32_t* cur_lum, uint32_t rgb);
int32_t isp_ae_set_stab_ext(uint32_t handler_id, uint32_t eb);

/**----------------------------------------------------------------------------*
**					Compiler Flag				**
**----------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/**---------------------------------------------------------------------------*/
#endif
// End

