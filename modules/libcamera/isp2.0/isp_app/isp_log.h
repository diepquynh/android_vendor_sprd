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

#ifndef _ISP_LOG_H_
#define _ISP_LOG_H_

#ifndef WIN32
#include <sys/types.h>
#include <utils/Log.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>
#else
#include "stdio.h"
#endif
#ifdef   __cplusplus
extern   "C"
{
#endif

#define ISP_DEBUG_STR       "ISP: L %d, %s: "
#define ISP_DEBUG_ARGS    __LINE__,__FUNCTION__

#ifndef WIN32
#define ISP_LOGE(format,...) ALOGE(ISP_DEBUG_STR format, ISP_DEBUG_ARGS, ##__VA_ARGS__)
#define ISP_LOGW(format,...) ALOGW(ISP_DEBUG_STR format, ISP_DEBUG_ARGS, ##__VA_ARGS__)
#define ISP_LOGI(format,...) ALOGI(ISP_DEBUG_STR format, ISP_DEBUG_ARGS, ##__VA_ARGS__)
#define ISP_LOGD(format,...) ALOGD(ISP_DEBUG_STR format, ISP_DEBUG_ARGS, ##__VA_ARGS__)
#define ISP_LOGV(format,...) ALOGV(ISP_DEBUG_STR format, ISP_DEBUG_ARGS, ##__VA_ARGS__)
#else
#define ISP_LOGE printf
#define ISP_LOGW printf
#define ISP_LOGI printf
#define ISP_LOGD printf
#define ISP_LOGV printf
#endif

#ifdef   __cplusplus
}
#endif

#endif
