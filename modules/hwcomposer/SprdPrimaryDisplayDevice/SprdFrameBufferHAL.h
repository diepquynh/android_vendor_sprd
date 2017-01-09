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
 ** File: SprdFrameBufferHal.h        DESCRIPTION                             *
 **                                   Open FrameBuffer device.                *
 ******************************************************************************
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/

#ifndef _SPRD_FRAME_BUFFER_HAL_H_
#define _SPRD_FRAME_BUFFER_HAL_H_

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include <linux/fb.h>
#include <hardware/hardware.h>
#include <hardware/gralloc.h>
#include <utils/RefBase.h>
#include <cutils/log.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "sprd_fb.h"
#include "gralloc_priv.h"


using namespace android;

#define HWC_DISPLAY_MASK                  (0x00000000)
#define HWC_DISPLAY_FRAMEBUFFER_TARGET    (0x00000001)
#define HWC_DISPLAY_PRIMARY_PLANE         (0x00000010)
#define HWC_DISPLAY_OVERLAY_PLANE         (0x00000100)
#define HWC_DISPLAY_OVERLAY_COMPOSER_GPU  (0x00001000)
#define HWC_DISPLAY_OVERLAY_COMPOSER_GSP  (0x00010000)

/*
 * FrameBuffer information.
 * */
typedef struct _FrameBufferInfo {
    int fbfd;
    int fb_width;
    int fb_height;
    float xdpi;
    float ydpi;
    int stride;
    void *fb_virt_addr;
    char *pFrontAddr;
    char *pBackAddr;
    int format;
    framebuffer_device_t* fbDev;
} FrameBufferInfo;

extern int loadFrameBufferHAL(FrameBufferInfo **fbInfo);
extern void closeFrameBufferHAL(FrameBufferInfo *fbInfo);

#endif
