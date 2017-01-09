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
 ** File: SprdOverlayPlane.cpp        DESCRIPTION                             *
 **                                   display YUV format Hardware layer, also *
 **                                   support RGBA format layer.              *
 ******************************************************************************
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/



#include "SprdOverlayPlane.h"
#include "dump.h"


using namespace android;


SprdOverlayPlane::SprdOverlayPlane()
    : SprdDisplayPlane(),
      mFBInfo(NULL),
      mHWLayer(NULL),
      mOverlayPlaneCount(1),
      mFreePlaneCount(1),
      mContext(NULL),
      mBuffer(NULL),
      mDisplayFormat(-1),
      mPlaneDisable(false),
      mDebugFlag(0),
      mDumpFlag(0)
{
#ifdef VIDEO_LAYER_USE_RGB
    mDisplayFormat = HAL_PIXEL_FORMAT_RGBA_8888;
#else
#ifdef GSP_OUTPUT_USE_YUV420
    mDisplayFormat = HAL_PIXEL_FORMAT_YCbCr_420_SP;
#else
    mDisplayFormat = HAL_PIXEL_FORMAT_YCbCr_422_SP;
#endif
#endif

    SprdDisplayPlane::setPlaneRunThreshold(300);
    mContext = SprdDisplayPlane::getPlaneBaseContext();
}

#ifdef BORROW_PRIMARYPLANE_BUFFER
SprdOverlayPlane::SprdOverlayPlane(SprdPrimaryPlane *PrimaryPlane)
    : SprdDisplayPlane(),
      mFBInfo(NULL),
      mPrimaryPlane(PrimaryPlane),
      mHWLayer(NULL),
      mOverlayPlaneCount(1),
      mFreePlaneCount(1),
      mContext(NULL),
      mBuffer(NULL),
      mDisplayFormat(-1),
      mPlaneDisable(false),
      mDebugFlag(0),
      mDumpFlag(0)
{
#ifdef VIDEO_LAYER_USE_RGB
    mDisplayFormat = HAL_PIXEL_FORMAT_RGBA_8888;
#else
#ifdef GSP_OUTPUT_USE_YUV420
    mDisplayFormat = HAL_PIXEL_FORMAT_YCbCr_420_SP;
#else
    mDisplayFormat = HAL_PIXEL_FORMAT_YCbCr_422_SP;
#endif
#endif

    mContext = SprdDisplayPlane::getPlaneBaseContext();
}
#endif

SprdOverlayPlane::~SprdOverlayPlane()
{
    close();
}

void SprdOverlayPlane:: updateFBInfo(FrameBufferInfo* fbInfo)
{
    if (fbInfo == NULL)
    {
        ALOGE("SprdOverlayPlane:: updateFBInfo input para is NULL");
        return;
    }

    mFBInfo = fbInfo;

    SprdDisplayPlane::setGeometry(mFBInfo->fb_width, mFBInfo->fb_height,
        mDisplayFormat, GRALLOC_USAGE_OVERLAY_BUFFER);

    open();
}

int SprdOverlayPlane::setPlaneContext(void *context)
{
    if (mContext == NULL)
    {
        ALOGE("SprdOverlayPlane::setPlaneContext mContext is NULL");
        return -1;
    }

    if (context == NULL)
    {
        ALOGE("SprdOverlayPlane::setPlaneContext input contxt is NULL");
        return -1;
    }

    mContext->BaseContext = context;

    return 0;
}

private_handle_t* SprdOverlayPlane::dequeueBuffer(int *fenceFd)
{
    bool ret = false;

    queryDebugFlag(&mDebugFlag);
    queryDumpFlag(&mDumpFlag);

#ifdef BORROW_PRIMARYPLANE_BUFFER
    mBuffer = mPrimaryPlane->dequeueFriendBuffer(fenceFd);
#else
    mBuffer = SprdDisplayPlane::dequeueBuffer(fenceFd);
#endif
    if (mBuffer == NULL)
    {
        ALOGE("SprdOverlayPlane cannot get ION buffer");
        return NULL;
    }

    enable();

    mFreePlaneCount = 1;

    ALOGI_IF(mDebugFlag, "SprdOverlayPlane::dequeueBuffer phy addr:%p", (void *)mBuffer->phyaddr);

    return mBuffer;
}

int SprdOverlayPlane::queueBuffer(int fenceFd)
{
#ifdef BORROW_PRIMARYPLANE_BUFFER
    mPrimaryPlane->queueFriendBuffer(fenceFd);
#else
    SprdDisplayPlane::queueBuffer(fenceFd);
#endif

    mFreePlaneCount = 0;

    return 0;
}

void SprdOverlayPlane::AttachOverlayLayer(SprdHWLayer *l)
{
    int ret = checkHWLayer(l);

    if (ret != 0)
    {
        ALOGE("Check OverlayLayer failed");
        return;
    }

    mContext->DirectDisplay = false;
    mHWLayer = l;
}

bool SprdOverlayPlane::online()
{
    if (mFreePlaneCount == 0)
    {
        ALOGI_IF(mDebugFlag, "SprdOverlayPlane is not avaiable");
        return false;
    }

    if (mPlaneDisable == false)
    {
        ALOGI_IF(mDebugFlag, "SprdOverlayPlane is disabled now.");
        return false;
    }
    return true;
}

int SprdOverlayPlane::checkHWLayer(SprdHWLayer *l)
{
    if (l == NULL)
    {
        ALOGE("SprdOverlayPlane Failed to check the list, SprdHWLayer is NULL");
        return -1;
    }

    return 0;
}

int SprdOverlayPlane::getPlaneFormat() const
{
    return mDisplayFormat;
}

private_handle_t* SprdOverlayPlane::flush(int *fenceFd)
{

    if (mContext == NULL/* || mContext->BaseContext == NULL*/)
    {
        ALOGE("SprdOverlayPlane::flush get display context failed");
        return NULL;
    }

#ifndef HWC_SUPPORT_ADF_DISPLAY
    sprdfb_layer_setting *BaseContext = static_cast<sprdfb_layer_setting *>(mContext->BaseContext);
#endif

    private_handle_t* flushingBuffer = NULL;
#ifdef BORROW_PRIMARYPLANE_BUFFER
    flushingBuffer = mPrimaryPlane->flushFriend(fenceFd);
#else
    flushingBuffer = SprdDisplayPlane::flush(fenceFd);
#endif

#ifndef HWC_SUPPORT_ADF_DISPLAY
    enum PlaneFormat format;
    BaseContext->layer_enable = true;

    format = getPlaneFormat();
    if (format == PLANE_FORMAT_RGB888 ||
        format == PLANE_FORMAT_RGB565)
    {
        BaseContext->data_type = SPRD_DATA_FORMAT_RGB888;
        BaseContext->y_endian = SPRD_DATA_ENDIAN_B0B1B2B3;
        BaseContext->uv_endian = SPRD_DATA_ENDIAN_B0B1B2B3;
        BaseContext->rb_switch = 1;
    }
    else if (format == PLANE_FORMAT_YUV422)
    {
        BaseContext->data_type = SPRD_DATA_FORMAT_YUV422;
        BaseContext->y_endian = SPRD_DATA_ENDIAN_B3B2B1B0;
        //BaseContext->uv_endian = SPRD_DATA_ENDIAN_B3B2B1B0;
        BaseContext->uv_endian = SPRD_DATA_ENDIAN_B0B1B2B3;
        BaseContext->rb_switch = 0;
    }
    else if (format == PLANE_FORMAT_YUV420)
    {
        BaseContext->data_type = SPRD_DATA_FORMAT_YUV420;
        BaseContext->y_endian = SPRD_DATA_ENDIAN_B3B2B1B0;
        BaseContext->rb_switch = 0;
        BaseContext->uv_endian = SPRD_DATA_ENDIAN_B3B2B1B0;
    }

    BaseContext->blending_mode = 0;

    BaseContext->rect.x = 0;
    BaseContext->rect.y = 0;
    BaseContext->rect.w = mFBInfo->fb_width;
    BaseContext->rect.h = mFBInfo->fb_height;

    if (flushingBuffer == NULL)
    {
        ALOGE("SprdOverlayPlane: Cannot get the display buffer");
        return NULL;
    }
    BaseContext->buffer = (unsigned char *)(flushingBuffer->phyaddr);

    ALOGI_IF(mDebugFlag, "SprdOverlayPlane::flush SET_OVERLAY parameter datatype = %d, x = %d, y = %d, w = %d, h = %d, buffer = 0x%08x",
             BaseContext->data_type,
             BaseContext->rect.x,
             BaseContext->rect.y,
             BaseContext->rect.w,
             BaseContext->rect.h,
             BaseContext->buffer);
#endif
    if ((HWCOMPOSER_DUMP_VIDEO_OVERLAY_FLAG & mDumpFlag)
        && flushingBuffer != NULL)
    {
        const char *name = "OverlayVideo";

        dumpOverlayImage(flushingBuffer, name);
    }

    return flushingBuffer;
}


bool SprdOverlayPlane::open()
{
#ifdef BORROW_PRIMARYPLANE_BUFFER
    if (mPrimaryPlane == NULL)
    {
        ALOGE("PrimaryPlane is NULL");
        return false;
    }
#else
    if (SprdDisplayPlane::open() == false)
    {
        ALOGE("SprdOverlayPlane::open failed");
        return false;
    }
#endif

    mOverlayPlaneCount = 1;
    mFreePlaneCount = 1;

    return true;
}

bool SprdOverlayPlane::close()
{
#ifndef BORROW_PRIMARYPLANE_BUFFER
    SprdDisplayPlane::close();
#endif

    mFreePlaneCount = 0;

    return true;
}

void SprdOverlayPlane::enable()
{
    mPlaneDisable = true;
}

void SprdOverlayPlane::disable()
{
    mPlaneDisable = false;
}

void SprdOverlayPlane::InvalidatePlane()
{
    if (mContext == NULL/* || mContext->BaseContext == NULL*/)
    {
        ALOGE("SprdOverlayPlane::InvalidatePlane display context is NULL");
        return;
    }

#ifndef HWC_SUPPORT_ADF_DISPLAY
    sprdfb_layer_setting *Context = static_cast<sprdfb_layer_setting *>(mContext->BaseContext);

    memset(Context, 0x00, sizeof(sprdfb_layer_setting));
    Context->layer_enable = false;
#endif
    mContext->DirectDisplay = false;
}

SprdHWLayer *SprdOverlayPlane::getOverlayLayer() const
{
    if (mHWLayer == NULL)
    {
        ALOGE("mHWLayer is NULL");
        return NULL;
    }

    return mHWLayer;
}

private_handle_t* SprdOverlayPlane::getPlaneBuffer() const
{
    if (mBuffer == NULL)
    {
        ALOGE("OverlayPlane buffer is NULL, dequeueBuffer failed");
        return NULL;
    }

    return mBuffer;
}

PlaneContext *SprdOverlayPlane::getPlaneContext() const
{
    return mContext;
}

int SprdOverlayPlane::addFlushReleaseFence(int fenceFd)
{
#ifndef BORROW_PRIMARYPLANE_BUFFER
    SprdDisplayPlane::addFlushReleaseFence(fenceFd);
#else
    mPrimaryPlane->addFriendFlushReleaseFence(fenceFd);
#endif

    return 0;
}
