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
#ifndef _AWB_CTRL_TYPES_H_
#define _AWB_CTRL_TYPES_H_
/*------------------------------------------------------------------------------*
*				Dependencies					*
*-------------------------------------------------------------------------------*/
#ifndef WIN32
#include <sys/types.h>
#include <utils/Log.h>
#else
#include "sci_types.h"
#endif
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
#ifdef WIN32
#define AWB_CTRL_LOGE
#define AWB_CTRL_LOGW
#define AWB_CTRL_LOGI
#define AWB_CTRL_LOGD
#define AWB_CTRL_LOGV
#else
#define AWB_CTRL_DEBUG_STR     "ISP_AWB: %d, %s: "
#define AWB_CTRL_DEBUG_ARGS    __LINE__,__FUNCTION__

#define AWB_CTRL_LOGE(format,...) ALOGE(AWB_CTRL_DEBUG_STR format, AWB_CTRL_DEBUG_ARGS, ##__VA_ARGS__)
#define AWB_CTRL_LOGW(format,...) ALOGW(AWB_CTRL_DEBUG_STR format, AWB_CTRL_DEBUG_ARGS, ##__VA_ARGS__)
#define AWB_CTRL_LOGI(format,...) ALOGI(AWB_CTRL_DEBUG_STR format, AWB_CTRL_DEBUG_ARGS, ##__VA_ARGS__)
#define AWB_CTRL_LOGD(format,...) ALOGD(AWB_CTRL_DEBUG_STR format, AWB_CTRL_DEBUG_ARGS, ##__VA_ARGS__)
#define AWB_CTRL_LOGV(format,...) ALOGV(AWB_CTRL_DEBUG_STR format, AWB_CTRL_DEBUG_ARGS, ##__VA_ARGS__)
#endif


/*------------------------------------------------------------------------------*
*				Data Structures					*
*-------------------------------------------------------------------------------*/


/*------------------------------------------------------------------------------*
*				Data Prototype					*
*-------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------*
*				Compiler Flag					*
*-------------------------------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
/*------------------------------------------------------------------------------*/
#endif
// End

