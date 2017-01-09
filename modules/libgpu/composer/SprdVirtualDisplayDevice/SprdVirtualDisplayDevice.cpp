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
 ** File:SprdVirtualDisplayDevice.cpp DESCRIPTION                             *
 **                                   Manager Virtual Display device.         *
 ******************************************************************************
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/

#include "SprdVirtualDisplayDevice.h"
#include "../SprdTrace.h"


using namespace android;

SprdVirtualDisplayDevice:: SprdVirtualDisplayDevice()
    : mLayerList(0),
      mDisplayPlane(0),
      mBlit(NULL),
      mHWCCopy(false),
      mDebugFlag(0),
      mDumpFlag(0)
{

}

SprdVirtualDisplayDevice:: ~SprdVirtualDisplayDevice()
{
    if (mDisplayPlane)
    {
        delete mDisplayPlane;
        mDisplayPlane = NULL;
    }

    if (mLayerList)
    {
        delete mLayerList;
        mLayerList = NULL;
    }
}

int SprdVirtualDisplayDevice:: Init()
{
    mLayerList = new SprdVDLayerList();
    if (mLayerList == NULL)
    {
        ALOGE("SprdVirtualDisplayDevice:: Init allocate mLayerList failed");
        return -1;
    }
    mDisplayPlane = new SprdVirtualPlane();
    if (mDisplayPlane == NULL)
    {
        ALOGE("SprdVirtualDisplayDevice:: Init allocate SprdVirtualPlane failed");
        return -1;
    }

    mBlit = new SprdWIDIBlit(mDisplayPlane);
    if (mBlit == NULL)
    {
        ALOGE("SprdVirtualDisplayDevice:: Init allocate SprdWIDIBlit failed");
        return -1;
    }

#ifdef FORCE_HWC_COPY_FOR_VIRTUAL_DISPLAYS
    mHWCCopy = true;
#else
    mHWCCopy = false;
#endif

    return 0;
}

int SprdVirtualDisplayDevice:: getDisplayAttributes(DisplayAttributes *dpyAttributes)
{
    return 0;
}

int SprdVirtualDisplayDevice:: setCursorPositionAsync(int x_pos, int y_pos)
{

    return 0;
}

int SprdVirtualDisplayDevice:: prepare(hwc_display_contents_1_t *list, unsigned int accelerator)
{
    queryDebugFlag(&mDebugFlag);
    queryDumpFlag(&mDumpFlag);

    if (list == NULL)
    {
        ALOGI_IF(mDebugFlag, "commit: Virtual Display Device maybe closed");
        return 0;
    }

    if (mLayerList->updateGeometry(list) != 0)
    {
        ALOGE("SprdVirtualDisplayDevice:: prepare updateGeometry failed");
        return -1;
    }

    if (mLayerList->revistGeometry(list) != 0)
    {
        ALOGE("SprdVirtualDisplayDevice:: prepare revistGeometry failed");
        return -1;
    }

    return 0;
}

int SprdVirtualDisplayDevice:: commit(hwc_display_contents_1_t *list)
{
    HWC_TRACE_CALL;

    bool BlitCond = false;
    int releaseFenceFd = -1;
    int AndroidLayerCount = mLayerList->getSprdLayerCount();
    SprdHWLayer *SprdFBTLayer = NULL;
    SprdHWLayer *SprdLayer = NULL;
    SprdHWLayer **OSDLayerList = NULL;
    hwc_layer_1_t *FBTargetLayer = NULL;
    int OSDLayerCount = mLayerList->getOSDLayerCount();

    queryDebugFlag(&mDebugFlag);

    if (list == NULL)
    {
        ALOGI_IF(mDebugFlag, "commit: Virtual Display Device maybe closed");
        return 0;
    }

    mDisplayPlane->UpdateAndroidLayerList(list);

    SprdFBTLayer = mLayerList->getFBTargetLayer();
    if (SprdFBTLayer == NULL)
    {
        ALOGE("SprdVirtualDisplayDevice:: commit cannot get SprdFBTLayer");
        return -1;
    }

    FBTargetLayer = &(list->hwLayers[list->numHwLayers - 1]);
    if (FBTargetLayer == NULL)
    {
        ALOGE("VirtualDisplay FBTLayer is NULL");
        return -1;
    }
    SprdFBTLayer->updateAndroidLayer(FBTargetLayer);

    const native_handle_t *pNativeHandle = FBTargetLayer->handle;
    struct private_handle_t *privateH = (struct private_handle_t *)pNativeHandle;

    ALOGI_IF(mDebugFlag, "Start Display VirtualDisplay FBT layer");

    if (FBTargetLayer->acquireFenceFd >= 0)
    {
        String8 name("HWCFBTVirtual::Post");

        FenceWaitForever(name, FBTargetLayer->acquireFenceFd);

        if (FBTargetLayer->acquireFenceFd >= 0)
        {
            close(FBTargetLayer->acquireFenceFd);
            FBTargetLayer->acquireFenceFd = -1;
        }
    }

    closeAcquireFDs(list);

    if (mHWCCopy)
    {
        struct private_handle_t *outHandle = ((struct private_handle_t *)list->outbuf);
        bool TargetIsRGBFormat = false;
        if (outHandle
            && (outHandle->format != HAL_PIXEL_FORMAT_YCbCr_420_SP)
            && (outHandle->format != HAL_PIXEL_FORMAT_YCrCb_420_SP))
        {
            TargetIsRGBFormat = true;
        }

        if (TargetIsRGBFormat)
        {
            BlitCond = mLayerList->getSkipMode() ? false : true;
        }
        else
        {
            BlitCond = true;
        }
    }

    if ((mHWCCopy == false)
        || (BlitCond == false))
    {
        /*
         *  Virtual display just have outbufAcquireFenceFd.
         *  We do not touch this outbuf, and do not need
         *  wait this fence, so just send this acquireFence
         *  back to SurfaceFlinger as retireFence.
         * */
        ALOGI_IF(mDebugFlag, "SprdVirtualDisplayDevice:: commit do not need COPY");
        list->retireFenceFd = list->outbufAcquireFenceFd;
    }
    else if (mHWCCopy && BlitCond)
    {
        OSDLayerList = mLayerList->getSprdOSDLayerList();
        SprdLayer = OSDLayerList[0];
        if ((AndroidLayerCount - 1 == 1) && (OSDLayerCount == 1)
            && SprdLayer
            && (SprdLayer->getAndroidLayer()))
        {
            ALOGI_IF(mDebugFlag, "SprdVirtualDisplayDevice:: commit attach Overlay layer[OSD]");
            mDisplayPlane->AttachVDFramebufferTargetLayer(SprdLayer);
        }
        else
        {
            ALOGI_IF(mDebugFlag, "SprdVirtualDisplayDevice:: commit attach FBT layer");
            mDisplayPlane->AttachVDFramebufferTargetLayer(SprdFBTLayer);
        }

        if (list->outbufAcquireFenceFd >= 0)
        {
            String8 name("HWCFBTVirtual::outbuf");

            FenceWaitForever(name, list->outbufAcquireFenceFd);

            if (list->outbufAcquireFenceFd >= 0)
            {
                close(list->outbufAcquireFenceFd);
                list->outbufAcquireFenceFd = -1;
            }
        }

        HWCBufferSyncBuildForVirtualDisplay(list);

        /*
         *  Blit buffer for Virtual Display
         * */
        mBlit->onStart();

        mBlit->onDisplay();
    }

    return 0;
}
