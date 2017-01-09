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
 ** File:SprdPrimaryDisplayDevice.cpp DESCRIPTION                             *
 **                                   Manage the PrimaryDisplayDevice         *
 **                                   including prepare and commit            *
 ******************************************************************************
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/

#include "SprdPrimaryDisplayDevice.h"
#include <utils/String8.h>
#include "../SprdTrace.h"

using namespace android;

SprdPrimaryDisplayDevice:: SprdPrimaryDisplayDevice()
   : mFBInfo(0),
     mLayerList(0),
     mOverlayPlane(0),
     mPrimaryPlane(0),
#ifdef OVERLAY_COMPOSER_GPU
     mWindow(NULL),
     mOverlayComposer(NULL),
#endif
     mVsyncEvent(0),
     mUtil(0),
     mPostFrameBuffer(true),
     mHWCDisplayFlag(HWC_DISPLAY_MASK),
     mAcceleratorMode(ACCELERATOR_NON),
#ifdef PROCESS_VIDEO_USE_GSP
     mGXPCap(NULL),
#endif
     mDebugFlag(0),
     mDumpFlag(0),
     mGspSupport(true)
    {

    }

bool SprdPrimaryDisplayDevice:: Init(FrameBufferInfo **fbInfo)
{
    int GXPAddrType = 0;
    void *pData = NULL;

    loadFrameBufferHAL(&mFBInfo);
    if (mFBInfo == NULL) {
        ALOGE("Can NOT get FrameBuffer info");
        return false;
    }

    mUtil = new SprdUtil(mFBInfo);
    if (mUtil == NULL) {
        ALOGE("new SprdUtil failed");
        return false;
    }
#ifdef PROCESS_VIDEO_USE_GSP
    if(GXPAddrType == 0) {
        //GXPAddrType = mUtil->getGSPAddrType();
    }
    mGXPCap = (GSP_CAPABILITY_T *)malloc(sizeof(GSP_CAPABILITY_T));
    if (mGXPCap == NULL)
    {
        ALOGE("SprdPrimaryDisplayDevice:: Init malloc GSP_CAPABILITY_T failed");
        return false;
    }
    if (mUtil->getGSPCapability(mGXPCap))
    {
        ALOGE("get gsp capability failed");
    }

    if (mGXPCap->magic != CAPABILITY_MAGIC_NUMBER)
    {
        ALOGE("SprdPrimaryDisplayDevice:: Init GXP device init failed");
    }
    pData = static_cast<void *>(mGXPCap);
#endif

    AcceleratorProbe(pData);

    mLayerList = new SprdHWLayerList(mFBInfo);
    if (mLayerList == NULL)
    {
        ALOGE("new SprdHWLayerList failed");
        return false;
    }
#ifdef PROCESS_VIDEO_USE_GSP
    mLayerList->transforGXPCapParameters(mGXPCap);
#endif

    mPrimaryPlane = new SprdPrimaryPlane(mFBInfo);
    if (mPrimaryPlane == NULL)
    {
        ALOGE("new SprdPrimaryPlane failed");
        return false;
    }

#ifdef BORROW_PRIMARYPLANE_BUFFER
    mOverlayPlane = new SprdOverlayPlane(mFBInfo, mPrimaryPlane);
#else
    mOverlayPlane = new SprdOverlayPlane(mFBInfo);
#endif
    if (mOverlayPlane == NULL)
    {
        ALOGE("new SprdOverlayPlane failed");
        return false;
    }

#ifdef OVERLAY_COMPOSER_GPU
    mWindow = new OverlayNativeWindow(mPrimaryPlane);
    if (mWindow == NULL)
    {
        ALOGE("Create Native Window failed, NO mem");
        return false;
    }

    if (!(mWindow->Init()))
    {
        ALOGE("Init Native Window failed");
        return false;
    }

    mOverlayComposer = new OverlayComposer(mPrimaryPlane, mWindow);
    if (mOverlayComposer == NULL)
    {
        ALOGE("new OverlayComposer failed");
        return false;
    }
#endif

    mVsyncEvent = new SprdVsyncEvent();
    if (mVsyncEvent == NULL)
    {
        ALOGE("new SprdVsyncEvent failed");
        return false;
    }



    *fbInfo = mFBInfo;

    return true;
}

SprdPrimaryDisplayDevice:: ~SprdPrimaryDisplayDevice()
{
    eventControl(0);

    if (mUtil != NULL)
    {
        delete mUtil;
        mUtil = NULL;
    }

    if (mVsyncEvent != NULL)
    {
        mVsyncEvent->requestExitAndWait();
    }

    if (mPrimaryPlane)
    {
        delete mPrimaryPlane;
        mPrimaryPlane = NULL;
    }

    if (mOverlayPlane)
    {
        delete mOverlayPlane;
        mOverlayPlane = NULL;
    }
#ifdef PROCESS_VIDEO_USE_GSP
    if (mGXPCap != NULL)
    {
        free(mGXPCap);
        mGXPCap = NULL;
    }
#endif

    if (mLayerList)
    {
        delete mLayerList;
        mLayerList = NULL;
    }
}

int SprdPrimaryDisplayDevice:: AcceleratorProbe(void *pData)
{
    int accelerator = ACCELERATOR_NON;

#ifdef PROCESS_VIDEO_USE_GSP
    if (pData == NULL)
    {
        ALOGD("SprdPrimaryDisplayDevice:: AcceleratorProbe GXP device error");
        return 0;
    }
    GSP_CAPABILITY_T *GXPCap = static_cast<GSP_CAPABILITY_T *>(pData);
    if (GXPCap->buf_type_support == GSP_ADDR_TYPE_PHYSICAL)
    {
        accelerator |= ACCELERATOR_GSP;
    }
    else if (GXPCap->buf_type_support == GSP_ADDR_TYPE_IOVIRTUAL)
    {
        //accelerator &= ~ACCELERATOR_GSP;
        accelerator |= ACCELERATOR_GSP_IOMMU;
    }
#endif

#ifdef OVERLAY_COMPOSER_GPU
    accelerator |= ACCELERATOR_OVERLAYCOMPOSER;
#endif

    mAcceleratorMode |= accelerator;

    return 0;
}

int SprdPrimaryDisplayDevice:: AcceleratorAdapt(int DisplayDeviceAccelerator)
{
    int value = ACCELERATOR_NON;

#ifdef FORCE_ADJUST_ACCELERATOR
    if (DisplayDeviceAccelerator & ACCELERATOR_GSP_IOMMU)
    {
        if (mAcceleratorMode & ACCELERATOR_GSP_IOMMU)
        {
            value |= ACCELERATOR_GSP_IOMMU;
            value |= ACCELERATOR_GSP;
        }
        else if (mAcceleratorMode & ACCELERATOR_GSP)
        {
            value |= ACCELERATOR_GSP;
            value &= ~ACCELERATOR_GSP_IOMMU;
        }
    }
    else if (DisplayDeviceAccelerator & ACCELERATOR_GSP)
    {
        if (mAcceleratorMode & ACCELERATOR_GSP)
        {
            value |= ACCELERATOR_GSP;
            value &= ~ACCELERATOR_GSP_IOMMU;
        }
    }

    if (DisplayDeviceAccelerator & ACCELERATOR_OVERLAYCOMPOSER)
    {
        if (mAcceleratorMode & ACCELERATOR_OVERLAYCOMPOSER)
        {
            value |= ACCELERATOR_OVERLAYCOMPOSER;
        }
    }
#else
    value |= mAcceleratorMode;
#endif

    ALOGI_IF(mDebugFlag, "SprdPrimaryDisplayDevice:: AcceleratorAdapt accelerator: 0x%x", value);
    return value;
}

#ifdef HWC_DUMP_CAMERA_SHAKE_TEST
void SprdPrimaryDisplayDevice:: dumpCameraShakeTest(hwc_display_contents_1_t* list)
{
    char value[PROPERTY_VALUE_MAX];
    if ((0 != property_get("persist.sys.performance_camera", value, "0")) &&
         (atoi(value) == 1))
    {
        for(unsigned int i = 0; i < list->numHwLayers; i++)
        {
            hwc_layer_1_t *l = &(list->hwLayers[i]);

            if (l && ((l->flags & HWC_DEBUG_CAMERA_SHAKE_TEST) == HWC_DEBUG_CAMERA_SHAKE_TEST))
            {
                struct private_handle_t *privateH = (struct private_handle_t *)(l->handle);
                if (privateH == NULL)
                {
                    continue;
                }

                void *cpuAddr = NULL;
                int offset = 0;
                int format = -1;
                int width = privateH->width;
                int height = privateH->stride;
                cpuAddr = (void *)(privateH->base);
                format = privateH->format;
                if (format == HAL_PIXEL_FORMAT_RGBA_8888)
                {
                    int r = -1;
                    int g = -1;
                    int b = -1;
                    int r2 = -1;
                    int g2 = -1;
                    int b2 = -1;
                    int colorNumber = -1;

                    /*
                     *  read the pixel in the 1/4 of the layer
                     * */
                    offset = ((width>>1) * (height>>1))<<2;
                    uint8_t *inrgb = (uint8_t *)((int *)cpuAddr + offset);

                    r = *(inrgb++); // for r;
                    g = *(inrgb++); // for g;
                    b = *(inrgb++);
                    inrgb++; // for a;
                    r2 = *(inrgb++); // for r;
                    g2 = *(inrgb++); // for g;
                    b2 = *(inrgb++);

                    if ((r == 205) && (g == 0) && (b == 252))
                    {
                        colorNumber = 0;
                    }
                    else if ((r == 15) && (g == 121) && (b == 0))
                    {
                        colorNumber = 1;
                    }
                    else if ((r == 31) && (g == 238) && (b == 0))
                    {
                        colorNumber = 2;
                    }

                    ALOGD("[HWComposer] will post camera shake test color:%d to LCD, 1st pixel in the middle of screen [r=%d, g=%d, b=%d], 2st pixel[r=%d, g=%d, b=%d]",
                           colorNumber, r, g, b, r2, g2, b2);
                }
            }
        }
    }
}
#endif

int SprdPrimaryDisplayDevice:: getDisplayAttributes(DisplayAttributes *dpyAttributes)
{
    int index = 0;
    float refreshRate = 60.0;
    framebuffer_device_t *fbDev = mFBInfo->fbDev;

    if (dpyAttributes == NULL)
    {
        ALOGE("Input parameter is NULL");
        return -1;
    }

    if (fbDev->fps > 0)
    {
        refreshRate = fbDev->fps;
    }

    index = dpyAttributes->configsIndex;
    if (index < 0)
    {
        ALOGE("SprdPrimaryDisplayDevice:: getDisplayAttributes invalid index");
        return -1;
    }

    dpyAttributes->sets[index].vsync_period = 1000000000l / refreshRate;
    dpyAttributes->sets[index].xres = mFBInfo->fb_width;
    dpyAttributes->sets[index].yres = mFBInfo->fb_height;
    dpyAttributes->sets[index].stride = mFBInfo->stride;
    dpyAttributes->sets[index].xdpi = mFBInfo->xdpi * 1000.0;
    dpyAttributes->sets[index].ydpi = mFBInfo->ydpi * 1000.0;
    dpyAttributes->connected = true;

    return 0;
}

int SprdPrimaryDisplayDevice:: ActiveConfig(DisplayAttributes *dpyAttributes)
{

    return 0;
}

int SprdPrimaryDisplayDevice:: setPowerMode(int mode)
{
    int ret = -1;
    int sprdMode = -1;

    switch(mode)
    {
        case POWER_MODE_NORMAL:
        {

            /*
             *  Turn on the display (if it was previously off),
             *  and take it out of low power mode.
             * */
            sprdMode = SPRD_FB_POWER_NORMAL;
            if (ioctl(mFBInfo->fbfd, SPRD_FB_SET_POWER_MODE, &sprdMode) < 0)
            {
                ALOGE("SprdPrimaryDisplayDevice:: setPowerMode: %d err", mode);
                ret = -1;
            }
            else
            {
                ret = 0;
            }
            break;
        }
        case POWER_MODE_DOZE:
        {
            /*
             *  Turn on the display (if it was previously off),
             *  and put the display in a low power mode.
             * */
            sprdMode = SPRD_FB_POWER_DOZE;
            if (ioctl(mFBInfo->fbfd, SPRD_FB_SET_POWER_MODE, &sprdMode) < 0)
            {
                ALOGE("SprdPrimaryDisplayDevice:: setPowerMode: %d err", mode);
                ret = -1;
            }
            else
            {
                ret = 0;
            }
            break;
        }
        case POWER_MODE_OFF:
        {
            /*
             *  Turn the display off.
             * */
            sprdMode = SPRD_FB_POWER_OFF;
            if (ioctl(mFBInfo->fbfd, SPRD_FB_SET_POWER_MODE, &sprdMode) < 0)
            {
                ALOGE("SprdPrimaryDisplayDevice:: setPowerMode: %d err", mode);
                ret = -1;
            }
            else
            {
                ret = 0;
            }
            break;
        }
#ifdef __LP64__
        case POWER_MODE_DOZE_SUSPEND:
        {
            /*
             *  Turn the display off.
             * */
            sprdMode = SPRD_FB_POWER_SUSPEND;
            if (ioctl(mFBInfo->fbfd, SPRD_FB_SET_POWER_MODE, &sprdMode) < 0)
            {
                ALOGE("SprdPrimaryDisplayDevice:: setPowerMode: %d err", mode);
                ret = -1;
            }
            else
            {
                ret = 0;
            }
            break;
        }
#endif
        default:
            return 0;
    }

    return ret;
}

int SprdPrimaryDisplayDevice:: setCursorPositionAsync(int x_pos, int y_pos)
{

    return 0;
}

int SprdPrimaryDisplayDevice:: getBuiltInDisplayNum(uint32_t *number)
{
    /*
     * At present, Sprd just support one built-in physical screen.
     * If support two later, should change here.
     * */
    *number = 1;

    return 0;
}

int SprdPrimaryDisplayDevice:: reclaimPlaneBuffer(bool condition)
{
    static int ret = -1;
    enum PlaneRunStatus status = PLANE_STATUS_INVALID;

    if (condition == false)
    {
        mPrimaryPlane->recordPlaneIdleCount();

        status = mPrimaryPlane->queryPlaneRunStatus();
        if (status == PLANE_SHOULD_CLOSED)
        {
            mPrimaryPlane->close();
#ifdef OVERLAY_COMPOSER_GPU
            mWindow->releaseNativeBuffer();
#endif
        }

        ret = 0;
    }
    else
    {
        mPrimaryPlane->resetPlaneIdleCount();

        status = mPrimaryPlane->queryPlaneRunStatus();
        if (status == PLANE_CLOSED)
        {
            bool value = false;
            value = mPrimaryPlane->open();
            if (value == false)
            {
                ALOGE("open PrimaryPlane failed");
                ret = 1;
            }
            else
            {
                ret = 0;
            }
        }
    }

    return ret;
}

int SprdPrimaryDisplayDevice:: attachToDisplayPlane(int DisplayFlag)
{
    int displayType = HWC_DISPLAY_MASK;
    mHWCDisplayFlag = HWC_DISPLAY_MASK;
    int OSDLayerCount = mLayerList->getOSDLayerCount();
    int VideoLayerCount = mLayerList->getVideoLayerCount();
    int FBLayerCount = mLayerList->getFBLayerCount();
    SprdHWLayer **OSDLayerList = mLayerList->getSprdOSDLayerList();
    SprdHWLayer **VideoLayerList = mLayerList->getSprdVideoLayerList();
    hwc_layer_1_t *FBTargetLayer = mLayerList->getFBTargetLayer();
    if (OSDLayerCount < 0 || VideoLayerCount < 0 ||
        OSDLayerList == NULL || VideoLayerList == NULL ||
        FBTargetLayer == NULL)
    {
        ALOGE("SprdPrimaryDisplayDevice:: attachToDisplayPlane get LayerList parameters error");
        return -1;
    }

    /*
     *  At present, each SprdDisplayPlane only only can handle one
     *  HWC layer.
     *  According to Android Framework definition, the smaller z-order
     *  layer is in the bottom layer list.
     *  The application layer is in the bottom layer list.
     *  Here, we forcibly attach the bottom layer to SprdDisplayPlane.
     * */
#define DEFAULT_ATTACH_LAYER 0

    bool cond = false;
#ifdef DIRECT_DISPLAY_SINGLE_OSD_LAYER
    cond = OSDLayerCount > 0;
#else
    cond = OSDLayerCount > 0 && VideoLayerCount > 0;
#endif
    if (cond)
    {
        bool DirectDisplay = false;
#ifdef DIRECT_DISPLAY_SINGLE_OSD_LAYER
        DirectDisplay = ((OSDLayerCount == 1) && (VideoLayerCount == 0));
#endif
        /*
         *  At present, we disable the Direct Display OSD layer first
         * */
        SprdHWLayer *sprdLayer = OSDLayerList[DEFAULT_ATTACH_LAYER];
        if (sprdLayer && sprdLayer->InitCheck())
        {
            mPrimaryPlane->AttachPrimaryLayer(sprdLayer, DirectDisplay);
            ALOGI_IF(mDebugFlag, "Attach Format:%d layer to SprdPrimaryDisplayPlane",
                     sprdLayer->getLayerFormat());

            displayType |= HWC_DISPLAY_PRIMARY_PLANE;
        }
        else
        {
            ALOGI_IF(mDebugFlag, "Attach layer to SprdPrimaryPlane failed");
            displayType &= ~HWC_DISPLAY_PRIMARY_PLANE;
        }
    }

    if (VideoLayerCount > 0)
    {
        SprdHWLayer *sprdLayer = VideoLayerList[DEFAULT_ATTACH_LAYER];

        if (sprdLayer && sprdLayer->InitCheck())
        {
            mOverlayPlane->AttachOverlayLayer(sprdLayer);
            ALOGI_IF(mDebugFlag, "Attach Format:%d layer to SprdOverlayPlane",
                     sprdLayer->getLayerFormat());

            displayType |= HWC_DISPLAY_OVERLAY_PLANE;
        }
        else
        {
            ALOGI_IF(mDebugFlag, "Attach layer to SprdOverlayPlane failed");

            displayType &= ~HWC_DISPLAY_OVERLAY_PLANE;
        }
    }

    if (DisplayFlag & HWC_DISPLAY_OVERLAY_COMPOSER_GPU)
    {
        displayType &= ~(HWC_DISPLAY_PRIMARY_PLANE | HWC_DISPLAY_OVERLAY_PLANE);
        displayType |= DisplayFlag;
    }
    else if (FBTargetLayer &&
             FBLayerCount > 0)
    {
        //mPrimary->AttachFrameBufferTargetLayer(mFBTargetLayer);
        ALOGI_IF(mDebugFlag, "Attach Framebuffer Target layer");

        displayType |= (0x1) & HWC_DISPLAY_FRAMEBUFFER_TARGET;
    }
    else
    {
        displayType &= ~HWC_DISPLAY_FRAMEBUFFER_TARGET;
    }

    mHWCDisplayFlag |= displayType;

    return 0;
}

int SprdPrimaryDisplayDevice:: prepare(hwc_display_contents_1_t *list, unsigned int accelerator)
{
    int ret = false;
    int displayFlag = HWC_DISPLAY_MASK;
    int acceleratorLocal = ACCELERATOR_NON;

    queryDebugFlag(&mDebugFlag);

    ALOGI_IF(mDebugFlag, "HWC start prepare");

    if (list == NULL)
    {
        ALOGE("The input parameters list is NULl");
        return -1;
    }

    acceleratorLocal = AcceleratorAdapt(accelerator);

    ret = mLayerList->updateGeometry(list, acceleratorLocal);
    if (ret != 0)
    {
        ALOGE("(FILE:%s, line:%d, func:%s) updateGeometry failed",
              __FILE__, __LINE__, __func__);
        return -1;
    }

    ret = mLayerList->revisitGeometry(mGspSupport, &displayFlag, this);
    if (ret !=0)
    {
        ALOGE("(FILE:%s, line:%d, func:%s) revisitGeometry failed",
              __FILE__, __LINE__, __func__);
        return -1;
    }

    ret = attachToDisplayPlane(displayFlag);
    if (ret != 0)
    {
        ALOGE("SprdPrimaryDisplayDevice:: attachToDisplayPlane failed");
        return -1;
    }

    return 0;
}

int SprdPrimaryDisplayDevice:: commit(hwc_display_contents_1_t* list)
{
    HWC_TRACE_CALL;
    int ret = -1;
    bool DisplayFBTarget = false;
    bool DisplayPrimaryPlane = false;
    bool DisplayOverlayPlane = false;
    bool DisplayOverlayComposerGPU = false;
    bool DisplayOverlayComposerGSP = false;
    bool DirectDisplayFlag = false;
    bool PrimaryPlane_Online_cond = false;
    private_handle_t* buffer1 = NULL;
    private_handle_t* buffer2 = NULL;

    hwc_layer_1_t *FBTargetLayer = NULL;
    int mGspReturn = 0;


    int OSDLayerCount = mLayerList->getOSDLayerCount();
    int VideoLayerCount = mLayerList->getVideoLayerCount();
    SprdHWLayer **OSDLayerList = mLayerList->getSprdOSDLayerList();
    SprdHWLayer **VideoLayerList = mLayerList->getSprdVideoLayerList();


    if (list == NULL)
    {
        /*
         * release our resources, the screen is turning off
         * in our case, there is nothing to do.
         * */
         return 0;
    }

    ALOGI_IF(mDebugFlag, "HWC start commit");

    waitAcquireFence(list);

    HWCBufferSyncBuild(list, DISPLAY_PRIMARY);

    switch ((mHWCDisplayFlag & ~HWC_DISPLAY_MASK))
    {
        case (HWC_DISPLAY_FRAMEBUFFER_TARGET):
            DisplayFBTarget = true;
            break;
        case (HWC_DISPLAY_PRIMARY_PLANE):
            DisplayPrimaryPlane = true;
            break;
        case (HWC_DISPLAY_OVERLAY_PLANE):
            DisplayOverlayPlane = true;
            break;
        case (HWC_DISPLAY_PRIMARY_PLANE |
              HWC_DISPLAY_OVERLAY_PLANE):
            DisplayPrimaryPlane = true;
            DisplayOverlayPlane = true;
            break;
        case (HWC_DISPLAY_OVERLAY_COMPOSER_GPU):
            DisplayOverlayComposerGPU = true;
            break;
        case (HWC_DISPLAY_FRAMEBUFFER_TARGET |
              HWC_DISPLAY_OVERLAY_PLANE):
            DisplayFBTarget = true;
            DisplayOverlayPlane = true;
            break;
        case (HWC_DISPLAY_OVERLAY_COMPOSER_GSP):
            DisplayOverlayComposerGSP = true;
            break;
        default:
            ALOGI_IF(mDebugFlag, "Display type: %d, use FBTarget", (mHWCDisplayFlag & ~HWC_DISPLAY_MASK));
            DisplayFBTarget = true;
            break;
    }


    /*
     *  This is temporary methods for displaying Framebuffer target layer, has some bug in FB HAL.
     *  ====     start   ================
     * */
    if (DisplayFBTarget)
    {
        FBTargetLayer = mLayerList->getFBTargetLayer();
        if (FBTargetLayer == NULL)
        {
            ALOGE("FBTargetLayer is NULL");
            return -1;
        }

        const native_handle_t *pNativeHandle = FBTargetLayer->handle;
        struct private_handle_t *privateH = (struct private_handle_t *)pNativeHandle;

        ALOGI_IF(mDebugFlag, "Start Displaying FramebufferTarget layer");

        if (FBTargetLayer->acquireFenceFd >= 0)
        {
            String8 name("HWCFBT::Post");

            FenceWaitForever(name, FBTargetLayer->acquireFenceFd);
            if (FBTargetLayer->acquireFenceFd >= 0)
            {
                close(FBTargetLayer->acquireFenceFd);
                FBTargetLayer->acquireFenceFd = -1;
            }
        }

#ifdef HWC_DUMP_CAMERA_SHAKE_TEST
        dumpCameraShakeTest(list);
#endif

#ifdef SPRD_DITHER_ENABLE
        if(!((mLayerList->getVideoLayerCount() != 0) || (mLayerList->getYuvLayerCount() != 0))) {
            privateH->flags |= private_handle_t::PRIV_FLAGS_SPRD_DITHER;
        }
        else {
            privateH->flags &= ~(private_handle_t::PRIV_FLAGS_SPRD_DITHER);
        }
#endif
        mFBInfo->fbDev->post(mFBInfo->fbDev, privateH);

        goto displayDone;
    }
    /*
     *  ==== end ========================
     * */

    /*
    static int64_t now = 0, last = 0;
    static int flip_count = 0;
    flip_count++;
    now = systemTime();
    if ((now - last) >= 1000000000LL)
    {
        float fps = flip_count*1000000000.0f/(now-last);
        ALOGI("HWC post FPS: %f", fps);
        flip_count = 0;
        last = now;
    }
    */

#ifdef OVERLAY_COMPOSER_GPU
    if (DisplayOverlayComposerGPU)
    {
        ALOGI_IF(mDebugFlag, "Start OverlayComposer composition misson");
        mOverlayComposer->onComposer(list);

        ALOGI_IF(mDebugFlag, "Start OverlayComposer display misson");
        mOverlayComposer->onDisplay();

        goto displayDone;
    }
#endif

    if (DisplayOverlayPlane)
    {
        mOverlayPlane->dequeueBuffer();

        buffer1 = mOverlayPlane->getPlaneBuffer();
    }
    else
    {
        mOverlayPlane->disable();
    }

#ifdef PROCESS_VIDEO_USE_GSP
    PrimaryPlane_Online_cond = (DisplayPrimaryPlane && (DisplayOverlayPlane == false));
#else
    PrimaryPlane_Online_cond = DisplayPrimaryPlane;
#endif

    if (PrimaryPlane_Online_cond)
    {
        mPrimaryPlane->dequeueBuffer();

        DirectDisplayFlag = mPrimaryPlane->GetDirectDisplay();
        if (DirectDisplayFlag == false)
        {
            buffer2 = mPrimaryPlane->getPlaneBuffer();
        }
    }
    else
    {
       mPrimaryPlane->disable();
    }


    if (DisplayOverlayPlane ||
        (DisplayPrimaryPlane && DirectDisplayFlag == false))
    {
        SprdHWLayer *OverlayLayer = NULL;
        SprdHWLayer *PrimaryLayer = NULL;

        if (DisplayOverlayPlane)
        {
            OverlayLayer = mOverlayPlane->getOverlayLayer();
        }

        if (DisplayPrimaryPlane && DirectDisplayFlag == false)
        {
            PrimaryLayer = mPrimaryPlane->getPrimaryLayer();
        }

#ifdef TRANSFORM_USE_DCAM
        mUtil->transformLayer(OverlayLayer, PrimaryLayer, buffer1, buffer2);
#endif

#ifdef PROCESS_VIDEO_USE_GSP
        if (OverlayLayer)
        {
            if (mOverlayPlane->getPlaneFormat() == PLANE_FORMAT_RGB888)
            {
                      mUtil->UpdateOutputFormat(GSP_DST_FMT_RGB888);
            }
            else if (mOverlayPlane->getPlaneFormat() == PLANE_FORMAT_YUV422)
            {
                      mUtil->UpdateOutputFormat(GSP_DST_FMT_YUV422_2P);
            }
            else if (mOverlayPlane->getPlaneFormat() == PLANE_FORMAT_YUV420)
            {
                      mUtil->UpdateOutputFormat(GSP_DST_FMT_YUV420_2P);
            }
        }
        else if (OverlayLayer == NULL && PrimaryLayer != NULL)
        {
            if (mPrimaryPlane->getPlaneFormat() == PLANE_FORMAT_RGB888)
            {
                      mUtil->UpdateOutputFormat(GSP_DST_FMT_RGB888);
            }
            else if (mPrimaryPlane->getPlaneFormat() == PLANE_FORMAT_RGB565)
            {
                      mUtil->UpdateOutputFormat(GSP_DST_FMT_RGB565);
            }
        }

        //if(mUtil->composerLayers(OverlayLayer, PrimaryLayer, buffer1, buffer2))
        mGspReturn = mUtil->composerLayerList(VideoLayerList, VideoLayerCount, OSDLayerList, OSDLayerCount, buffer1, buffer2);
        if(mGspReturn)
        {
            if(mGspReturn == GSP_KERNEL_WAITDONE_TIMEOUT)
                mGspSupport = false;
            ALOGE("%s[%d],composerLayers ret err!!",__func__,__LINE__);
        }
        else
        {
            ALOGI_IF(mDebugFlag, "%s[%d],composerLayers success",__func__,__LINE__);

#ifdef HWC_DUMP_CAMERA_SHAKE_TEST
            dumpCameraShakeTest(list);
#endif
        }
        /*
         *  Use GSP to do 2 layer blending, so if PrimaryLayer is not NULL,
         *  disable DisplayPrimaryPlane.
         * */
        if (DisplayOverlayPlane)
        {
            DisplayPrimaryPlane = false;
        }
#endif
    }

   buffer1 = NULL;
   buffer2 = NULL;

   if (mOverlayPlane->online())
   {
       ret = mOverlayPlane->queueBuffer();
       if (ret != 0)
       {
           ALOGE("OverlayPlane::queueBuffer failed");
           return -1;
       }
   }

   if (mPrimaryPlane->online())
   {
       ret = mPrimaryPlane->queueBuffer();
       if (ret != 0)
       {
           ALOGE("PrimaryPlane::queueBuffer failed");
           return -1;
       }
   }

#ifdef OVERLAY_COMPOSER_GPU
    mWindow->notifyDirtyTarget(true);
#endif

   if (DisplayOverlayPlane || DisplayPrimaryPlane || DisplayFBTarget)
   {
       mPrimaryPlane->display(DisplayOverlayPlane, DisplayPrimaryPlane, DisplayFBTarget);
   }


displayDone:
   queryDumpFlag(&mDumpFlag);
   if (DisplayFBTarget &&
       (mDumpFlag & HWCOMPOSER_DUMP_FRAMEBUFFER_FLAG))
   {
       dumpFrameBuffer(mFBInfo->pFrontAddr, "FrameBuffer", mFBInfo->fb_width, mFBInfo->fb_height, mFBInfo->format);
   }

    closeAcquireFDs(list);

    return 0;
}

void SprdPrimaryDisplayDevice:: setVsyncEventProcs(const hwc_procs_t *procs)
{
    sp<SprdVsyncEvent> VE = getVsyncEventHandle();
    if (VE == NULL)
    {
        ALOGE("getVsyncEventHandle failed");
        return;
    }

    VE->setVsyncEventProcs(procs);
}

void SprdPrimaryDisplayDevice:: eventControl(int enabled)
{
    sp<SprdVsyncEvent> VE = getVsyncEventHandle();
    if (VE == NULL)
    {
        ALOGE("getVsyncEventHandle failed");
        return;
    }

    VE->setEnabled(enabled);
}
