/*
 * Copyright (C) 2007 The Android Open Source Project
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

#ifndef _SPRD_AF_CTRL_V1_H
#define _SPRD_AF_CTRL_V1_H

#include "isp_type.h"
#include "isp_com.h"
#include "lib_ctrl.h"

#define ISP_CALLBACK_EVT 0x00040000    // FIXME: should be defined in isp_app.h
#define ISP_PROC_AF_IMG_DATA_UPDATE (1 << 3) // FIXME
#define AF_SAVE_MLOG_STR     "persist.sys.isp.af.mlog" /*save/no*/

int32_t sprd_afv1_module_init(struct af_lib_fun *ops);

#endif /* _SPRD_AF_CTRL_V1_H */
