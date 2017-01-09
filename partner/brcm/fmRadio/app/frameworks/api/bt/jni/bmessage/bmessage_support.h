/*
 *  Copyright (C) 2009-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef BTA_MA_SUPPORT_H
#define BTA_MA_SUPPORT_H


#include "bmessage/bmessage_data_types.h"
#include "utils/Log.h"

#define BTA_MSE_DEBUG TRUE

//Make BTA_API empty so it compiles
#define BTA_API
//End BTA_API

//Hack to avoid including a bunch of goep and fs dependencies
#ifndef BTA_FS_CO_H
#define BTA_FS_CO_H
#define BTA_FS_O_RDONLY         0x0000
#define BTA_FS_O_WRONLY         0x0001
#define BTA_FS_O_RDWR           0x0002

#define BTA_FS_O_CREAT          0x0100
#define BTA_FS_O_EXCL           0x0200
#define BTA_FS_O_TRUNC          0x1000
#define BTA_FS_INVALID_FD (-1)
#endif
//End hack

//Map logging to that provided by Android utils/Log.h
#ifndef BT_TRACE_H
#define BT_TRACE_H
#define APPL_TRACE_ERROR0(m)    ALOGE(m)
#define APPL_TRACE_ERROR1(m,p1) ALOGE(m,p1)
#define APPL_TRACE_ERROR2(m,p1,p2) ALOGE(m,p1,p2)
#define APPL_TRACE_ERROR3(m,p1,p2,p3) ALOGE(m,p1,p2,p3)
#define APPL_TRACE_EVENT0(m)    ALOGI(m)
#define APPL_TRACE_EVENT1(m,p1) ALOGI(m,p1)
#define APPL_TRACE_EVENT2(m,p1,p2) ALOGI(m,p1,p2)
#define APPL_TRACE_EVENT3(m,p1,p2,p3) ALOGI(m,p1,p2,p3)
#define APPL_TRACE_DEBUG0(m) ALOGD(m)
#define APPL_TRACE_DEBUG1(m,p1) ALOGD(m,p1)
#define APPL_TRACE_DEBUG2(m,p1,p2) ALOGD(m,p1,p2)
#define APPL_TRACE_DEBUG4(m,p1,p2,p3,p4) ALOGD(m,p1,p2,p3,4)
#endif

//End log mapping

//Map GKI freebuf and getbuf to free/malloc
#define GKI_freebuf(m) free(m)
#define GKI_getbuf(m) malloc(m)
//End GKI mapping
#endif
