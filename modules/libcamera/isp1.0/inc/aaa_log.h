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
#ifndef _AAAB_LOG_H_
#define _AAAB_LOG_H_

#include <sys/types.h>
#include <utils/Log.h>
#include <stdlib.h>
#include <fcntl.h>/* low-level i/o */
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/
#ifdef   __cplusplus
extern   "C"
{
#endif
/*------------------------------------------------------------------------------*
*					Micro Define				*
*-------------------------------------------------------------------------------*/
#if 0
#define ISP_DEBUG_STR      "%s, %s, %d line,: "
#define ISP_DEBUG_ARGS    __FILE__,__FUNCTION__,__LINE__
#else
#define AAA_DEBUG_STR      "ISP_AAA: id:0x%02x, %05d line, %s : "
#define AAA_DEBUG_ARGS    handler_id,__LINE__,__FUNCTION__
#endif

#ifdef ANDROID_4003
/*android 4.0.0.3*/
#define AAA_LOG(format,...) LOGE(AAA_DEBUG_STR format, AAA_DEBUG_ARGS, ##__VA_ARGS__)
#else
/*android 4.1.0.0*/
#define AAA_LOG(format,...) ALOGE(AAA_DEBUG_STR format, AAA_DEBUG_ARGS, ##__VA_ARGS__)
#endif

/*------------------------------------------------------------------------------*
*					Data Structures				*
*-------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------*
*					Compiler Flag				*
*-------------------------------------------------------------------------------*/
#ifdef   __cplusplus
}
#endif
/*-----------------------------------------------------------------------------*/
#endif
// End

