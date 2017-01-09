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
#ifndef _AWB_AL_CTRL_H_
#define _AWB_AL_CTRL_H_
#include "AlAwbInterface.h"
/*------------------------------------------------------------------------------*
*				Data Prototype					*
*-------------------------------------------------------------------------------*/
struct al_awb_thirdlib_fun {
	void    (*AlAwbInterfaceInit)(TT_AlAwbInterface*        pptInterface);
	void    (*AlAwbInterfaceDestroy)(TT_AlAwbInterface*     pptInterface);
	void    (*AlAwbInterfaceReset)(TT_AlAwbInterface*       pptInterface);
	void    (*AlAwbInterfaceMain)(TT_AlAwbInterface*        pptInterface);
	UI_16   (*AlAwbInterfaceSendCommand)(TT_AlAwbInterface* pptInterface, TT_AlAisCmd* pptCmd);
	void    (*AlAwbInterfaceShowVersion)(TT_AlAwbInterface* pptInterface);
};

awb_ctrl_handle_t awb_al_ctrl_init(struct awb_ctrl_init_param *param,
				struct awb_ctrl_init_result *result);
uint32_t awb_al_ctrl_deinit(awb_ctrl_handle_t handle, void *param, void *result);
uint32_t awb_al_ctrl_calculation(awb_ctrl_handle_t handle,
				struct awb_ctrl_calc_param *param,
				struct awb_ctrl_calc_result *result);
uint32_t awb_al_ctrl_ioctrl(awb_ctrl_handle_t handle, enum awb_ctrl_cmd cmd,
				void *param0, void *param1);
uint32_t al_awb_lib_open(uint32_t version_id);
char* al_libversion_choice(uint32_t version_id);
#endif
