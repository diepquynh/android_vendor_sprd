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
 ** File: SprdPrimaryPlane.h          DESCRIPTION                             *
 **                                   display RGBA format Hardware layer      *
 ******************************************************************************
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/



#include "SprdPrimaryPlane.h"
#include "dump.h"


using namespace android;

SprdPrimaryPlane::SprdPrimaryPlane(FrameBufferInfo *fbInfo)
    : SprdDisplayPlane(),
      mFBInfo(fbInfo),
      mHWLayer(NULL),
      mPrimaryPlaneCount(1),
      mFreePlaneCount(1),
      mContext(NULL),
      mBuffer(NULL),
      mBufferIndex(-1),
      mDefaultDisplayFormat(-1),
      mDisplayFormat(-1),
      mPlaneDisable(false),
      mDisplayFBTargetLayerFlag(false),
      mDirectDisplayFlag(false),
      mPlaneBufferPhyAddr(NULL),
      mDisplayFBTargetPhyAddr(NULL),
      mDirectDisplayPhyAddr(NULL),
      mThreadID(-1),
      mDebugFlag(0),
      mDumpFlag(0)
{
#ifdef PRIMARYPLANE_USE_RGB565
    mDefaultDisplayFormat = HAL_PIXEL_FORMAT_RGB_565;
#else
    mDefaultDisplayFormat = HAL_PIXEL_FORMAT_RGBA_8888;
    //mDefaultDisplayFormat = HAL_PIXEL_FORMAT_RGBX_8888;
#endif

    SprdDisplayPlane::setGeometry(mFBInfo->fb_width, mFBInfo->fb_height, mDefaultDisplayFormat);

    SprdDisplayPlane::setPlaneRunThreshold(100);

    mContext = SprdDisplayPlane::getPlaneContext();

    mDisplayFormat = mDefaultDisplayFormat;

#ifndef DYNAMIC_RELEASE_PLANEBUFFER
    open();
#endif

    mThreadID = gettid();
}

SprdPrimaryPlane::~SprdPrimaryPlane()
{
    close();
}

private_handle_t* SprdPrimaryPlane::dequeueBuffer()
{
    bool ret = false;
    int localThreadID = -1;

    queryDebugFlag(&mDebugFlag);
    queryDumpFlag(&mDumpFlag);

    mFreePlaneCount = 1;

    enable();

    if (mDisplayFBTargetLayerFlag)
    {
        mPlaneBufferPhyAddr = mDisplayFBTargetPhyAddr;
    }
    else if (GetDirectDisplay())
    {
        mPlaneBufferPhyAddr = mDirectDisplayPhyAddr;
        ALOGI_IF(mDebugFlag, "SprdPrimaryPlane is in DirectDisplay Mode");
    }
    else
    {
        mBuffer = SprdDisplayPlane::dequeueBuffer();
        if (mBuffer == NULL)
        {
            ALOGE("SprdPrimaryPlane cannot get ION buffer");
            return NULL;
        }
        mPlaneBufferPhyAddr = (unsigned char *)(mBuffer->phyaddr);
    }

    mBufferIndex = SprdDisplayPlane:: getPlaneBufferIndex();

    ALOGI_IF(mDebugFlag, "SprdPrimaryPlane::dequeueBuffer phy addr:%p, index: %d", (void *)mPlaneBufferPhyAddr, mBufferIndex);

    return mBuffer;
}

int SprdPrimaryPlane::queueBuffer()
{
    if ((mDisplayFBTargetLayerFlag || GetDirectDisplay()) == false)
    {
        SprdDisplayPlane::queueBuffer();
    }

    flush();

    mFreePlaneCount = 0;

    return 0;
}

void SprdPrimaryPlane::AttachPrimaryLayer(SprdHWLayer *l, bool DirectDisplayFlag)
{
    int ret = checkHWLayer(l);

    if (ret != 0)
    {
        ALOGE("Check Sprd PrimaryLayer failed");
        return;
    }

    mHWLayer = l;

    if (DirectDisplayFlag)
    {
        hwc_layer_1_t *AndroidLayer = mHWLayer->getAndroidLayer();
        SetDisplayParameters(AndroidLayer);
    }
}

void SprdPrimaryPlane::AttachFramebufferTargetLayer(hwc_layer_1_t *FBTargetLayer)
{
    const native_handle_t *pNativeHandle = FBTargetLayer->handle;
    struct private_handle_t *privateH = (struct private_handle_t *)pNativeHandle;
    unsigned long phy_addr = 0;
    size_t size = 0;

    mDisplayFormat = privateH->format;

    MemoryHeapIon::Get_phy_addr_from_ion(privateH->share_fd, &phy_addr, &size);

    mDisplayFBTargetPhyAddr = (unsigned char *)phy_addr;

    mDisplayFBTargetLayerFlag = true;

    ALOGI_IF(mDebugFlag, "FBTargetLayer FB addr:%p", (void *)mDisplayFBTargetPhyAddr);
}

bool SprdPrimaryPlane::SetDisplayParameters(hwc_layer_1_t *AndroidLayer)
{
    if (AndroidLayer == NULL)
    {
        ALOGI_IF(mDebugFlag, "SprdHWLayer is NULL");
        mDirectDisplayFlag = false;
        return false;
    }

    const native_handle_t *pNativeHandle = AndroidLayer->handle;
    struct private_handle_t *privateH = (struct private_handle_t *)pNativeHandle;
    unsigned long phy_addr = 0;
    size_t size = 0;

    if (privateH == NULL)
    {
        ALOGE("SetDisplayParameters privateH is NULL");
        mDirectDisplayFlag = false;
        return false;
    }

    if ((privateH->format == HAL_PIXEL_FORMAT_YCbCr_420_SP) ||
        (privateH->format == HAL_PIXEL_FORMAT_YCrCb_420_SP) ||
        (privateH->format == HAL_PIXEL_FORMAT_YV12))
    {
        ALOGI("Current transform device and display device cannot support virtual adress");
        mDirectDisplayFlag = false;
        return false;
    }

    if (!((privateH->flags) & (private_handle_t::PRIV_FLAGS_USES_PHY)))
    {
        ALOGI_IF(mDebugFlag, "Current device cannot support virtual adress");
        mDirectDisplayFlag = false;
        return false;
    }

    if (AndroidLayer->transform != 0)
    {
        ALOGI_IF(mDebugFlag, "This layer need to be transformed");
        mDirectDisplayFlag = false;
        return false;
    }

    mDisplayFormat = privateH->format;

    mDirectDisplayFlag = true;

    MemoryHeapIon::Get_phy_addr_from_ion(privateH->share_fd, &phy_addr, &size);

    mDirectDisplayPhyAddr = (unsigned char *)phy_addr;

    return mDirectDisplayFlag;
}

void SprdPrimaryPlane::display(bool DisplayOverlayPlane, bool DisplayPrimaryPlane, bool DisplayFBTarget)
{
    int PlaneType = 0;
    struct overlay_display_setting displayContext;

    displayContext.display_mode = SPRD_DISPLAY_OVERLAY_ASYNC;

    if (DisplayOverlayPlane)
    {
        PlaneType |= SPRD_LAYERS_IMG;
    }

    if (mPlaneDisable == false &&
        (DisplayPrimaryPlane || DisplayFBTarget))
    {
        PlaneType |= SPRD_LAYERS_OSD;
        if (GetDirectDisplay())
        {
            displayContext.display_mode = SPRD_DISPLAY_OVERLAY_SYNC;
        }
    }

    displayContext.layer_index = PlaneType;
    displayContext.rect.x = 0;
    displayContext.rect.y = 0;
    displayContext.rect.w = mFBInfo->fb_width;
    displayContext.rect.h = mFBInfo->fb_height;

    ALOGI_IF(mDebugFlag, "SPRD_FB_DISPLAY_OVERLAY %d", PlaneType);

    ioctl(mFBInfo->fbfd, SPRD_FB_DISPLAY_OVERLAY, &displayContext);

    /*
     *  Restore some status.
     * */
    mDisplayFBTargetLayerFlag = false;

    mDirectDisplayFlag = false;

    mDisplayFormat = mDefaultDisplayFormat;
}

void SprdPrimaryPlane::disable()
{
    mPlaneDisable = true;
}

void SprdPrimaryPlane::enable()
{
    mPlaneDisable = false;
}

bool SprdPrimaryPlane::online()
{

    if (mPlaneDisable)
    {
        ALOGI_IF(mDebugFlag, "SprdPrimaryPlane has been disabled");
        return false;
    }

    if (mFreePlaneCount == 0)
    {
        ALOGI_IF(mDebugFlag, "SprdPrimaryPlanens is not avaiable");
        return false;
    }

    //if (mHWLayer == NULL || mDisplayFBTargetLayerFlag == false)
    //{
    //    ALOGI("SprdPrimaryPlane SprdHWLayer or FramebufferTarget layer is NULL");
    //    return false;
    //}

    return true;
}

int SprdPrimaryPlane::checkHWLayer(SprdHWLayer *l)
{
    if (l == NULL)
    {
        ALOGE("SprdPrimaryPlane Failed to check the list, SprdHWLayer is NULL");
        return -1;
    }

    return 0;
}

enum PlaneFormat SprdPrimaryPlane::getPlaneFormat()
{
    enum PlaneFormat format;
    int displayFormat = mDisplayFormat;

    switch(displayFormat)
    {
        case HAL_PIXEL_FORMAT_RGBX_8888:
        case HAL_PIXEL_FORMAT_RGBA_8888:
            format = PLANE_FORMAT_RGB888;
            break;
        case HAL_PIXEL_FORMAT_RGB_565:
            format = PLANE_FORMAT_RGB565;
            break;
        default:
            format = PLANE_FORMAT_NONE;
            break;
    }

    return format;
}

private_handle_t* SprdPrimaryPlane::flush()
{
    enum PlaneFormat format;
    struct overlay_setting *BaseContext = &(mContext->BaseContext);
    private_handle_t* flushingBuffer = NULL;

    queryDebugFlag(&mDebugFlag);
    queryDumpFlag(&mDumpFlag);

    InvalidatePlaneContext();

    if ((mDisplayFBTargetLayerFlag || GetDirectDisplay()) == false)
    {
        flushingBuffer = SprdDisplayPlane::flush();
    }

    BaseContext->layer_index = SPRD_LAYERS_OSD;

    format = getPlaneFormat();
    if (format == PLANE_FORMAT_RGB888)
    {
        BaseContext->data_type = SPRD_DATA_FORMAT_RGB888;
        BaseContext->y_endian = SPRD_DATA_ENDIAN_B0B1B2B3;
        BaseContext->uv_endian = SPRD_DATA_ENDIAN_B0B1B2B3;
        BaseContext->rb_switch = 1;
    }
    else if (format ==  PLANE_FORMAT_RGB565)
    {
        BaseContext->data_type = SPRD_DATA_FORMAT_RGB565;
        BaseContext->y_endian = SPRD_DATA_ENDIAN_B2B3B0B1;
        BaseContext->uv_endian = SPRD_DATA_ENDIAN_B0B1B2B3;
        BaseContext->rb_switch = 0;
    }

    BaseContext->rect.x = 0;
    BaseContext->rect.y = 0;
    BaseContext->rect.w = mFBInfo->fb_width;
    BaseContext->rect.h = mFBInfo->fb_height;

    if (GetDirectDisplay() || mDisplayFBTargetLayerFlag)
    {
        if (mPlaneBufferPhyAddr == NULL)
        {
            ALOGE("mPlaneBufferPhyAddr is NULL");
            return NULL;
        }
        BaseContext->buffer = mPlaneBufferPhyAddr;
    }
    else
    {
        if (flushingBuffer == NULL)
        {
            ALOGE("SprdPrimaryPlane::flush flushingBuffer error");
            return NULL;
        }

        BaseContext->buffer = (unsigned char *)(flushingBuffer->phyaddr);
    }

    ALOGI_IF(mDebugFlag, "SprdPrimaryPlane::flush  osd overlay parameter datatype = %d, x = %d, y = %d, w = %d, h = %d, buffer = 0x%p",
             BaseContext->data_type,
             BaseContext->rect.x,
             BaseContext->rect.y,
             BaseContext->rect.w,
             BaseContext->rect.h,
             BaseContext->buffer);

    if ((HWCOMPOSER_DUMP_OSD_OVERLAY_FLAG & mDumpFlag)
        && (flushingBuffer != NULL))
    {
        const char *name = "OverlayOSD";

        dumpOverlayImage(flushingBuffer, name);
    }

    if (ioctl(mFBInfo->fbfd, SPRD_FB_SET_OVERLAY, BaseContext) == -1)
    {
        ALOGE("fail osd SPRD_FB_SET_OVERLAY");
        ioctl(mFBInfo->fbfd, SPRD_FB_SET_OVERLAY, BaseContext);//Fix ME later
    }

    return flushingBuffer;
}

bool SprdPrimaryPlane::open()
{
    if (SprdDisplayPlane::open() == false)
    {
        ALOGE("SprdPrimaryPlane::open failed");
        return false;
    };

    mPrimaryPlaneCount = 1;
    mFreePlaneCount = 1;

    return true;
}

bool SprdPrimaryPlane::close()
{
    SprdDisplayPlane::close();

    mFreePlaneCount = 0;

    return true;
}

void SprdPrimaryPlane::InvalidatePlaneContext()
{
    memset(&(mContext->BaseContext), 0x00, sizeof(struct overlay_setting));
}

SprdHWLayer *SprdPrimaryPlane::getPrimaryLayer()
{
    return mHWLayer;
}

int SprdPrimaryPlane::getPlaneBufferIndex()
{
    return mBufferIndex;
}

private_handle_t* SprdPrimaryPlane::getPlaneBuffer()
{
    if (mBuffer == NULL)
    {
        ALOGE("Failed to get hte SprdPrimaryPlane buffer");
        return NULL;
    }

    return mBuffer;
}


void SprdPrimaryPlane::getPlaneGeometry(unsigned int *width, unsigned int *height, int *format)
{
    if (width == NULL || height == NULL || format == NULL)
    {
        ALOGE("getPlaneGeometry, input parameters are NULL");
        return;
    }

    *width = mFBInfo->fb_width;
    *height = mFBInfo->fb_height;
    *format = mDefaultDisplayFormat;
}

#ifdef BORROW_PRIMARYPLANE_BUFFER
private_handle_t* SprdPrimaryPlane:: dequeueFriendBuffer()
{
    return SprdDisplayPlane::dequeueBuffer();
}

int SprdPrimaryPlane:: queueFriendBuffer()
{
    return SprdDisplayPlane::queueBuffer();
}

private_handle_t* SprdPrimaryPlane:: flushFriend()
{
    return SprdDisplayPlane::flush();
}
#endif
