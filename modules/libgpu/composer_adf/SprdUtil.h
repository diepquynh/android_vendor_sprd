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
 ** File: SprdUtil.h                  DESCRIPTION                             *
 **                                   Transform or composer Hardware layers   *
 **                                   when display controller cannot deal     *
 **                                   with these function                     *
 ******************************************************************************
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/


#ifndef _SPRD_UTIL_H_
#define _SPRD_UTIL_H_

#include <utils/Thread.h>
#include <utils/RefBase.h>
#include <cutils/log.h>
#include <semaphore.h>
//#include "MemoryHeapIon.h"
#include <ui/Rect.h>
#include <ui/GraphicBufferAllocator.h>
#include <ui/GraphicBufferMapper.h>

#include "SprdHWLayer.h"
#include "gralloc_priv.h"
#include "SprdPrimaryDisplayDevice/SprdFrameBufferHAL.h"
#include "SprdDisplayPlane.h"
#include "dump.h"

#ifdef PROCESS_VIDEO_USE_GSP
#include "sc8830/gsp_hal.h"
#endif

//#ifdef TRANSFORM_USE_DCAM
#include "sc8825/dcam_hal.h"
//#endif

#ifdef TRANSFORM_USE_GPU
#include "sc8810/gpu_transform.h"
#endif

using namespace android;


/*
 *  Accelerator mode
 * */
#define ACCELERATOR_NON              (0x00000000)
#define ACCELERATOR_DISPC            (0x00000001)
#define ACCELERATOR_GSP              (0x00000010)
#define ACCELERATOR_OVERLAYCOMPOSER  (0x00000100)// GPU
#define ACCELERATOR_DCAM             (0x00010000)

#ifndef ALIGN
#define ALIGN(value, base) (((value) + ((base) - 1)) & ~((base) - 1))
#endif


typedef struct _SprdUtilSource {
    SprdHWLayer **LayerList;
    int LayerCount;
    int releaseFenceFd;
} SprdUtilSource;

typedef struct _SprdUtilTarget {
    private_handle_t* buffer; /* This is Target buffer. */
    private_handle_t* buffer2; /* This is Target buffer2. */
    int acquireFenceFd;       /* acquire fence fd of Target buffer */;
    int releaseFenceFd;       /* release fence fd of Target buffer */
    int format;
} SprdUtilTarget;

#ifdef TRANSFORM_USE_DCAM
/*
 *  Transform OSD layer.
 * */
class OSDTransform: public Thread
{
public:
    OSDTransform();
    ~OSDTransform();

    void onStart(SprdHWLayer *l, private_handle_t* buffer);
    void onWait();

private:
    SprdHWLayer *mL;
    //SceenSize *mFBInfo;
    private_handle_t* mBuffer;
    bool mInitFLag;
    int mDebugFlag;


    /*
     * OSDTransform thread info.
     * In order to accerate the OSD transform speed,
     * need start a new thread to the transform work,
     * Parallel with video transform work.
     * */
#ifdef _PROC_OSD_WITH_THREAD
    sem_t startSem;
    sem_t doneSem;

    virtual status_t readyToRun();
    virtual void onFirstRef();
    virtual bool threadLoop();
#endif

    int transformOSD();
};
#endif


/*
 *  SprdUtil is responsible for transform or composer HW layers with
 *  hardware devices, such as DCAM, GPU or GSP.
 * */
class SprdUtil
{
public:
	SprdUtil();
    ~SprdUtil();

    bool transformLayer(SprdUtilSource *Source, SprdUtilTarget *Target);

#ifdef PROCESS_VIDEO_USE_GSP
	int probeGSPDevice();

    int Prepare(SprdHWLayer **LayerList, int LayerCount, bool& Support);

    int composeLayerList(SprdUtilSource *Source, SprdUtilTarget *Target);

    inline void UpdateFBInfo(FrameBufferInfo *FBInfo)
    {
		mFBInfo = FBInfo;
		if(mGspDev && mGspDev->GSP_SetFBInfo) {
			mGspDev->GSP_SetFBInfo(FBInfo->fb_width, FBInfo->fb_height);
		}
    }

#endif

private:
    FrameBufferInfo *mFBInfo;
#ifdef TRANSFORM_USE_DCAM
    private_handle_t* tmpDCAMBuffer;
    sp<OSDTransform>  mOSDTransform;
#endif
#ifdef PROCESS_VIDEO_USE_GSP
    gsp_device_t *mGspDev;
#endif
    int mInitFlag;
    int mDebugFlag;

#ifdef TRANSFORM_USE_GPU
    int getTransformInfo(SprdHWLayer *l1, SprdHWLayer *l2,
                         private_handle_t* buffer1, private_handle_t* buffer2,
                         gpu_transform_info_t *transformInfo);
#endif
#ifdef PROCESS_VIDEO_USE_GSP
    int openGSPDevice();
#endif
};


#endif
