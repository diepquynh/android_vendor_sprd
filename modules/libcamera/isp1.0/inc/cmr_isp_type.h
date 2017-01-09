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
#ifndef _CMR_ISP_TYPE_H_
#define _CMR_ISP_TYPE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <sys/types.h>
#ifdef MINICAMERA
#include "port.h"
#endif

typedef unsigned long   cmr_uint;
typedef long            cmr_int;
typedef uint64_t        cmr_u64;
typedef int64_t         cmr_s64;
typedef unsigned int    cmr_u32;
typedef int             cmr_s32;
typedef unsigned short  cmr_u16;
typedef short           cmr_s16;
typedef unsigned char   cmr_u8;
typedef char            cmr_s8;
typedef void*           cmr_handle;


#ifdef __cplusplus
}
#endif

#endif

