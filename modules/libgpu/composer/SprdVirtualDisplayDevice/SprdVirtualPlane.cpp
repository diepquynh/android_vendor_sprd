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
 ** 16/06/2014    Hardware Composer   Responsible for processing some         *
 **                                   Hardware layers. These layers comply    *
 **                                   with Virtual Display specification,     *
 **                                   can be displayed directly, bypass       *
 **                                   SurfaceFligner composition. It will     *
 **                                   improve system performance.             *
 ******************************************************************************
 ** File:SprdVirtualPlane.cpp         DESCRIPTION                             *
 **                                   Responsible for Post display data to    *
 **                                   Virtual Display.                        *
 ******************************************************************************
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/


#include "SprdVirtualPlane.h"
#include "dump.h"
#include "AndroidFence.h"


using namespace android;


SprdVirtualPlane:: SprdVirtualPlane()
    : SprdDisplayPlane(),
      mPlaneWidth(1),
      mPlaneHeight(1),
      mPlaneFormat(-1),
      mDefaultPlaneFormat(-1),
      mVideoLayerCount(0),
      mOSDLayerCount(0),
      mVideoLayerList(0),
      mOSDLayerList(0),
      mAndroidLayerList(0),
      mFBTLayer(0),
      mDisplayBuffer(0),
      mDebugFlag(0),
      mDumpFlag(0)
{
     mDefaultPlaneFormat = HAL_PIXEL_FORMAT_YCbCr_420_SP;
}

SprdVirtualPlane:: ~SprdVirtualPlane()
{

}

bool SprdVirtualPlane:: open()
{
    return true;
}

bool SprdVirtualPlane:: close()
{
    return true;
}

private_handle_t *SprdVirtualPlane:: dequeueBuffer()
{
    struct private_handle_t *outHandle = ((struct private_handle_t *)mAndroidLayerList->outbuf);
    if (outHandle == NULL)
    {
        ALOGE("SprdVirtualPlane:: dequeueBuffer outHandle is NULL");
        return NULL;
    }

    AttachDisplayBuffer((struct private_handle_t *)(mAndroidLayerList->outbuf));

    if (mDisplayBuffer == NULL)
    {
        ALOGE("SprdVirtualPlane:: dequeueBuffer failed");
        return NULL;
    }
    struct private_handle_t *privateH = (struct private_handle_t *)mDisplayBuffer;
    if (privateH == NULL)
    {
        ALOGE("SprdVirtualPlane:: dequeueBuffer Display Buffer handle is NULL");
        return NULL;
    }

    bool phyType = (privateH->flags & private_handle_t::PRIV_FLAGS_USES_PHY);

    queryDebugFlag(&mDebugFlag);

    mPlaneWidth = privateH->width;
    mPlaneHeight = privateH->height;
    mPlaneFormat = mDefaultPlaneFormat;

    ALOGI_IF(mDebugFlag, "SprdVirtualPlane::dequeueBuffer width:%d, height: %d, vir addr: %p, phyAddr: %d, handle format: %d",
             mPlaneWidth, mPlaneHeight, (void *)(privateH->base),
             phyType, privateH->format);

    return mDisplayBuffer;
}

int SprdVirtualPlane:: queueBuffer()
{
    ALOGI_IF(mDebugFlag, "SprdVirtualPlane:: queueBuffer HWC has finished the blit operation");

    if (HWCBufferSyncReleaseForVirtualDisplay(mAndroidLayerList) < 0)
    {
        ALOGE("SprdVirtualPlane:: queueBuffer HWCBufferSyncReleaseForVirtualDisplay failed");
    }

    queryDumpFlag(&mDumpFlag);

    if (HWCOMPOSER_DUMP_VD_OVERLAY_FLAG & mDumpFlag)
    {
        const char *name = "VirtualDisplayYUV";

        dumpOverlayImage(mDisplayBuffer, name);
    }

    resetPlaneGeometry();

    return 0;
}

int SprdVirtualPlane:: UpdateAndroidLayerList(hwc_display_contents_1_t *list)
{
    if (list == NULL)
    {
        ALOGE("SprdVirtualPlane:: UpdateAndroidLayerList input list is NULL");
        return -1;
    }

    mAndroidLayerList = list;

    return 0;
}

void SprdVirtualPlane:: resetPlaneGeometry()
{
    mPlaneWidth = 1;
    mPlaneHeight = 1;
    mPlaneFormat = -1;
}

int SprdVirtualPlane:: AttachVDFramebufferTargetLayer(SprdHWLayer *SprdFBTLayer)
{
    if (SprdFBTLayer == NULL)
    {
        ALOGE("SprdVirtualPlane:: AttachVDFramebufferTargetLayer input is NULL");
        return -1;
    }

    mFBTLayer = SprdFBTLayer;

    return 0;
}

void SprdVirtualPlane:: AttachDisplayBuffer(private_handle_t *outputBuffer)
{
    mDisplayBuffer = outputBuffer;
}

void SprdVirtualPlane:: getPlaneGeometry(unsigned int *width, unsigned int *height, int *format)
{
    if (width == NULL || height == NULL || format == NULL)
    {
        ALOGE("SprdVirtualPlane:: getPlaneGeometry input para is NULL");
        return;
    }

    *width = mPlaneWidth;
    *height = mPlaneHeight;
    *format = mPlaneFormat;
}

void SprdVirtualPlane:: AttachVDLayer(SprdHWLayer **videoLayerList, int videoLayerCount, SprdHWLayer **osdLayerList, int osdLayerCount)
{
    if (videoLayerList == NULL || osdLayerList == NULL)
    {
        ALOGE("SprdVirtualPlane:: AttachVDLayer Sprd Layer List is NULL");
        mVideoLayerList = NULL;
        mOSDLayerList = NULL;
        mVideoLayerCount = 0;
        mOSDLayerCount = 0;
        return;
    }

    mVideoLayerList = videoLayerList;
    mVideoLayerCount = videoLayerCount;

    mOSDLayerList = osdLayerList;
    mOSDLayerCount = osdLayerCount;
}

private_handle_t* SprdVirtualPlane:: getPlaneBuffer()
{
    return mDisplayBuffer;
}
