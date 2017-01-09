/*
 * Copyright (C) 2010 The Android Open Source Project
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


/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          Module              DESCRIPTION                             *
 ** 22/09/2013    Hardware Composer   Responsible for processing some         *
 **                                   Hardware layers. These layers comply    *
 **                                   with display controller specification,  *
 **                                   can be displayed directly, bypass       *
 **                                   SurfaceFligner composition. It will     *
 **                                   improve system performance.             *
 ******************************************************************************
 ** File: AndroidFence.h              DESCRIPTION                             *
 **                                   Handle Android Framework fence          *
 ******************************************************************************
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/

#ifndef _ANDROID_FENCE_H_
#define _ANDROID_FENCE_H_

#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>
#include <utils/RefBase.h>
#include <ui/Fence.h>
#include <sync/sync.h>
#include <utils/String8.h>


using namespace android;

extern int openSprdFence();

extern void closeSprdFence();

extern int waitAcquireFence(hwc_display_contents_1_t *list);

extern void closeAcquireFDs(hwc_display_contents_1_t *list);

extern int HWCBufferSyncBuild(hwc_display_contents_1_t *list, int display);

extern int FenceWaitForever(const String8& name, int fenceFd);

extern int HWCBufferSyncBuildForVirtualDisplay(hwc_display_contents_1_t *list);

extern int HWCBufferSyncReleaseForVirtualDisplay(hwc_display_contents_1_t *list);

#endif
