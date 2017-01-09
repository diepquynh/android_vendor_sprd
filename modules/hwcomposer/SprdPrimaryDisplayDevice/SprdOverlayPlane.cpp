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


SprdOverlayPlane::SprdOverlayPlane(FrameBufferInfo *fbInfo)
    : SprdDisplayPlane(),
      mFBInfo(fbInfo),
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
    SprdDisplayPlane::setGeometry(mFBInfo->fb_width, mFBInfo->fb_height, mDisplayFormat);

    SprdDisplayPlane::setPlaneRunThreshold(300);

    mContext = SprdDisplayPlane::getPlaneContext();

    open();
}

#ifdef BORROW_PRIMARYPLANE_BUFFER
SprdOverlayPlane::SprdOverlayPlane(FrameBufferInfo *fbInfo, SprdPrimaryPlane *PrimaryPlane)
    : SprdDisplayPlane(),
      mFBInfo(fbInfo),
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

    mContext = SprdDisplayPlane::getPlaneContext();

    open();
}
#endif

SprdOverlayPlane::~SprdOverlayPlane()
{
    close();
}

private_handle_t* SprdOverlayPlane::dequeueBuffer()
{
    bool ret = false;

    queryDebugFlag(&mDebugFlag);
    queryDumpFlag(&mDumpFlag);

#ifdef BORROW_PRIMARYPLANE_BUFFER
    mBuffer = mPrimaryPlane->dequeueFriendBuffer();
#else
    mBuffer = SprdDisplayPlane::dequeueBuffer();
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

int SprdOverlayPlane::queueBuffer()
{
#ifdef BORROW_PRIMARYPLANE_BUFFER
    mPrimaryPlane->queueFriendBuffer();
#else
    SprdDisplayPlane::queueBuffer();
#endif

    if(flush() == NULL)
    {
        return -1;
    }

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

enum PlaneFormat SprdOverlayPlane::getPlaneFormat()
{
    enum PlaneFormat format;
    int displayFormat = mDisplayFormat;

    switch(displayFormat)
    {
        case HAL_PIXEL_FORMAT_RGBA_8888:
            format = PLANE_FORMAT_RGB888;
            break;
        case HAL_PIXEL_FORMAT_RGB_565:
            format = PLANE_FORMAT_RGB565;
            break;
        case HAL_PIXEL_FORMAT_YCbCr_422_SP:
            format = PLANE_FORMAT_YUV422;
            break;
        case HAL_PIXEL_FORMAT_YCbCr_420_SP:
            format = PLANE_FORMAT_YUV420;
            break;
        default:
            format = PLANE_FORMAT_NONE;
            break;
    }

    return format;
}

private_handle_t* SprdOverlayPlane::flush()
{
    enum PlaneFormat format;
    struct overlay_setting *BaseContext = &(mContext->BaseContext);

    InvalidatePlaneContext();

    private_handle_t* flushingBuffer = NULL;
#ifdef BORROW_PRIMARYPLANE_BUFFER
    flushingBuffer = mPrimaryPlane->flushFriend();
#else
    flushingBuffer = SprdDisplayPlane::flush();
#endif

    BaseContext->layer_index = SPRD_LAYERS_IMG;

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

    ALOGI_IF(mDebugFlag, "SprdOverlayPlane::flush SET_OVERLAY parameter datatype = %d, x = %d, y = %d, w = %d, h = %d, buffer = 0x%p",
             BaseContext->data_type,
             BaseContext->rect.x,
             BaseContext->rect.y,
             BaseContext->rect.w,
             BaseContext->rect.h,
             BaseContext->buffer);

    if (HWCOMPOSER_DUMP_VIDEO_OVERLAY_FLAG & mDumpFlag)
    {
        const char *name = "OverlayVideo";

        dumpOverlayImage(flushingBuffer, name);
    }

    if (ioctl(mFBInfo->fbfd, SPRD_FB_SET_OVERLAY, BaseContext) == -1)
    {
        ALOGE("fail video SPRD_FB_SET_OVERLAY");
        ioctl(mFBInfo->fbfd, SPRD_FB_SET_OVERLAY, BaseContext);//Fix ME later
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

void SprdOverlayPlane::InvalidatePlaneContext()
{
    memset(&(mContext->BaseContext), 0x00, sizeof(struct overlay_setting));
}

SprdHWLayer *SprdOverlayPlane::getOverlayLayer()
{
    if (mHWLayer == NULL)
    {
        ALOGE("mHWLayer is NULL");
        return NULL;
    }

    return mHWLayer;
}

private_handle_t* SprdOverlayPlane::getPlaneBuffer()
{
    if (mBuffer == NULL)
    {
        ALOGE("OverlayPlane buffer is NULL, dequeueBuffer failed");
        return NULL;
    }

    return mBuffer;
}
