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

#ifndef _AF_LOG_H_
#define _AF_LOG_H_
/*------------------------------------------------------------------------------*
*					Dependencies				*
*-------------------------------------------------------------------------------*/
#include <sys/types.h>
#include <utils/Log.h>

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

#ifdef WIN32
#define AF_LOGE
#define AF_LOGW
#define AF_LOGI
#define AF_LOGD
#define AF_LOGV
#else
#define AF_DEBUG_STR     "ISP_AF: %d, %s: "
#define AF_DEBUG_ARGS    __LINE__,__FUNCTION__

#define AF_LOG(format,...) ALOGE(AF_DEBUG_STR format, AF_DEBUG_ARGS, ##__VA_ARGS__)
#define AF_LOGE(format,...) ALOGE(AF_DEBUG_STR format, AF_DEBUG_ARGS, ##__VA_ARGS__)
#define AF_LOGW(format,...) ALOGW(AF_DEBUG_STR format, AF_DEBUG_ARGS, ##__VA_ARGS__)
#define AF_LOGI(format,...) ALOGI(AF_DEBUG_STR format, AF_DEBUG_ARGS, ##__VA_ARGS__)
#define AF_LOGD(format,...) ALOGD(AF_DEBUG_STR format, AF_DEBUG_ARGS, ##__VA_ARGS__)
#define AF_LOGV(format,...) ALOGV(AF_DEBUG_STR format, AF_DEBUG_ARGS, ##__VA_ARGS__)
#endif



/*------------------------------------------------------------------------------*
*					Data Structures				*
*-------------------------------------------------------------------------------*/




/*------------------------------------------------------------------------------*
*					Data Prototype				*
*-------------------------------------------------------------------------------*/



/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/
#ifdef	 __cplusplus
}
#endif
/*-----------------------------------------------------------------------------*/
#endif
// End
