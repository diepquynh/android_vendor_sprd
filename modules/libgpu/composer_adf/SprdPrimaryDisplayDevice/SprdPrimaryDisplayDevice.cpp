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
     mDispCore(NULL),
     mPrimaryDisplayContext(0),
     mLayerList(0),
     mOverlayPlane(0),
     mPrimaryPlane(0),
#ifdef OVERLAY_COMPOSER_GPU
     mWindow(NULL),
     mOverlayComposer(NULL),
#endif
     mVsyncEvent(0),
     mComposedLayer(NULL),
     mUtil(0),
     mUtilSource(NULL),
     mUtilTarget(NULL),
     mDisplayFBTarget(false),
     mDisplayPrimaryPlane(false),
     mDisplayOverlayPlane(false),
     mDisplayOVC(false),
     mSchedualUtil(false),
     mHWCDisplayFlag(HWC_DISPLAY_MASK),
     mAcceleratorMode(ACCELERATOR_NON),
     mDebugFlag(0),
     mDumpFlag(0)
    {
        mBlank = false;
    }

bool SprdPrimaryDisplayDevice:: Init(void *adfData)
{
    int GXPAddrType = 0;
    void *pData = NULL;

#ifdef HWC_SUPPORT_ADF_DISPLAY
    if (adfData == NULL)
    {
        ALOGE("SprdPrimaryDisplayDevice:: Init adfData is NULL");
        return false;
    }

    mDispCore = static_cast<SprdADFWrapper *>(adfData);

#endif
    loadFrameBufferHAL(&mFBInfo);
    if (mFBInfo == NULL) {
        ALOGE("Can NOT get FrameBuffer info");
        return false;
    }

#ifndef HWC_SUPPORT_ADF_DISPLAY
    mPrimaryDisplayContext = (sprdfb_overlay_setting *)malloc(sizeof(sprdfb_overlay_setting));
    if (mPrimaryDisplayContext == NULL)
    {
        ALOGE("malloc sprdfb_overlay_setting failed");
        return false;
    }
#endif

    mUtil = new SprdUtil();
    if (mUtil == NULL) {
        ALOGE("new SprdUtil failed");
        return false;
    }

     mUtilSource = (SprdUtilSource *)malloc(sizeof(SprdUtilSource));
    if (mUtilSource == NULL)
    {
       ALOGE("malloc SprdUtilSource failed");
       return false;
    }

    mUtilTarget = (SprdUtilTarget *)malloc(sizeof(SprdUtilTarget));
    if (mUtilTarget == NULL)
    {
        ALOGE("malloc SprdUtilTarget failed");
        return false;
    }

    AcceleratorProbe();

    mLayerList = new SprdHWLayerList();
    if (mLayerList == NULL)
    {
        ALOGE("new SprdHWLayerList failed");
        return false;
    }

    mLayerList->setAccerlator(mUtil);

    mPrimaryPlane = new SprdPrimaryPlane();
    if (mPrimaryPlane == NULL)
    {
        ALOGE("new SprdPrimaryPlane failed");
        return false;
    }

#ifndef HWC_SUPPORT_ADF_DISPLAY
    mPrimaryPlane->setPlaneContext(static_cast<void *>(&(mPrimaryDisplayContext->layer_info[SPRD_LAYERS_OSD])));
#endif

#ifdef BORROW_PRIMARYPLANE_BUFFER
    mOverlayPlane = new SprdOverlayPlane(mPrimaryPlane);
#else
    mOverlayPlane = new SprdOverlayPlane();
#endif
    if (mOverlayPlane == NULL)
    {
        ALOGE("new SprdOverlayPlane failed");
        return false;
    }

#ifndef HWC_SUPPORT_ADF_DISPLAY
    mOverlayPlane->setPlaneContext(static_cast<void *>(&(mPrimaryDisplayContext->layer_info[SPRD_LAYERS_IMG])));
#endif


#ifndef HWC_SUPPORT_ADF_DISPLAY
    mVsyncEvent = new SprdVsyncEvent();
    if (mVsyncEvent == NULL)
    {
        ALOGE("new SprdVsyncEvent failed");
        return false;
    }
#endif

    return true;
}

SprdPrimaryDisplayDevice:: ~SprdPrimaryDisplayDevice()
{
#ifndef HWC_SUPPORT_ADF_DISPLAY
    eventControl(0);
#endif

    if (mUtil != NULL)
    {
        delete mUtil;
        mUtil = NULL;
    }

    if (mUtilTarget != NULL)
    {
        free(mUtilTarget);
        mUtilTarget = NULL;
    }

    if (mUtilSource != NULL)
    {
        free(mUtilSource);
        mUtilSource = NULL;
    }

#ifndef HWC_SUPPORT_ADF_DISPLAY
    if (mVsyncEvent != NULL)
    {
        mVsyncEvent->requestExitAndWait();
    }
#endif

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

    if (mLayerList)
    {
        delete mLayerList;
        mLayerList = NULL;
    }

    if (mPrimaryDisplayContext != NULL)
    {
        free(mPrimaryDisplayContext);
    }

    if (mFBInfo != NULL)
    {
        closeFrameBufferHAL(mFBInfo);
    }
}

int SprdPrimaryDisplayDevice:: AcceleratorProbe()
{
    int accelerator = ACCELERATOR_NON;

#ifdef ACC_BY_DISPC
    accelerator |= ACCELERATOR_DISPC;
#endif


#ifdef PROCESS_VIDEO_USE_GSP
    if (mUtil->probeGSPDevice() != 0)
    {
        ALOGE("cannot find GXP devices");
        goto Step2;
    }
    accelerator |= ACCELERATOR_GSP;
#endif

Step2:
#ifdef OVERLAY_COMPOSER_GPU
    accelerator |= ACCELERATOR_OVERLAYCOMPOSER;
#endif
    mAcceleratorMode |= accelerator;

    ALOGI_IF(mDebugFlag, "%s[%d] mAcceleratorMode:%x",__func__,__LINE__,mAcceleratorMode);
    return 0;
}

/*
 *  function:AcceleratorAdapt
 *  SprdPrimaryDisplayDevice::mAcceleratorMode record the actually probed available Accelerator type.
 *  DisplayDeviceAccelerator: is the Accelerator type that some special display have.
 *							  like primary have dispc-type, virtual do't have dispc-type.
 *  this function is used to get the intersection of these two set.
 * */
int SprdPrimaryDisplayDevice:: AcceleratorAdapt(int DisplayDeviceAccelerator)
{
    int value = ACCELERATOR_NON;
    HWC_IGNORE(DisplayDeviceAccelerator);
#ifdef FORCE_ADJUST_ACCELERATOR
    if (DisplayDeviceAccelerator & ACCELERATOR_DISPC)
    {
        if (mAcceleratorMode & ACCELERATOR_DISPC)
        {
            value |= ACCELERATOR_DISPC;
        }
    }

    if (DisplayDeviceAccelerator & ACCELERATOR_GSP)
    {
        if (mAcceleratorMode & ACCELERATOR_GSP)
        {
            value |= ACCELERATOR_GSP;
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

int SprdPrimaryDisplayDevice:: syncAttributes(AttributesSet *dpyAttributes)
{
    int index = 0;
    float refreshRate = 60.0;
    //framebuffer_device_t *fbDev = mFBInfo->fbDev;

    if (dpyAttributes == NULL)
    {
        ALOGE("Input parameter is NULL");
        return -1;
    }

#if 0
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
#endif

    mFBInfo->fb_width    = dpyAttributes->xres;
    mFBInfo->fb_height   = dpyAttributes->yres;
    //mFBInfo->format      =
    mFBInfo->stride      = dpyAttributes->xres;
    mFBInfo->xdpi        = dpyAttributes->xdpi;
    mFBInfo->ydpi        = dpyAttributes->ydpi;

    mLayerList->updateFBInfo(mFBInfo);
    mPrimaryPlane->updateFBInfo(mFBInfo);
    mOverlayPlane->updateFBInfo(mFBInfo);
    mUtil->UpdateFBInfo(mFBInfo);

#ifdef OVERLAY_COMPOSER_GPU
    static bool OVCInit = false;
    if (OVCInit == false)
    {
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
		OVCInit = true;
    }
#endif

    return 0;
}

int SprdPrimaryDisplayDevice:: ActiveConfig(DisplayAttributes *dpyAttributes)
{
    if (dpyAttributes == NULL)
    {
        ALOGE("SprdPrimaryDisplayDevice:: ActiveConfig input para is NULL");
        return -1;
    }

    AttributesSet *attr = &(dpyAttributes->sets[dpyAttributes->configsIndex]);
    dpyAttributes->connected = true;
    syncAttributes(attr);

    return 0;
}

int SprdPrimaryDisplayDevice:: setPowerMode(int mode)
{
    int ret = 0;
    int sprdMode = -1;

#if 0
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
#else
    Mutex::Autolock _l(mLock);
    mBlank = (mode == HWC_POWER_MODE_OFF ? 1 : 0);
    mDispCore->Blank(DISPLAY_PRIMARY, mBlank);
#endif

    return ret;
}

int SprdPrimaryDisplayDevice:: setCursorPositionAsync(int x_pos, int y_pos)
{
    HWC_IGNORE(x_pos);
    HWC_IGNORE(y_pos);
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
    SprdHWLayer **OSDLayerList = mLayerList->getOSDLayerList();
    SprdHWLayer **VideoLayerList = mLayerList->getVideoLayerList();
    hwc_layer_1_t *FBTargetLayer = mLayerList->getFBTargetLayer();
    bool& disableHWC = mLayerList->getDisableHWCFlag();

    if (disableHWC)
    {
        ALOGI_IF(mDebugFlag, "SprdPrimaryDisplayDevice:: attachToDisplayPlane HWC is disabled now");
        return 0;
    }

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

int SprdPrimaryDisplayDevice:: WrapFBTargetLayer(hwc_display_contents_1_t *list)
{
    hwc_layer_1_t *FBTargetLayer = NULL;
    struct sprdRect *src = NULL;
    struct sprdRect *fb  = NULL;
    FBTargetLayer = &(list->hwLayers[list->numHwLayers - 1]);
    if (FBTargetLayer == NULL)
    {
        ALOGE("WrapFBTargetLayer FBTargetLayer is NULL");
        return -1;
    }

    const native_handle_t *pNativeHandle = FBTargetLayer->handle;
    struct private_handle_t *privateH = (struct private_handle_t *)pNativeHandle;
    if (privateH == NULL)
    {
        ALOGE("WrapFBTargetLayer FBT handle is NULL");
        return -1;
    }

    mComposedLayer = new SprdHWLayer(FBTargetLayer, privateH->format);
    if (mComposedLayer == NULL)
    {
        ALOGE("WrapFBTargetLayer new mComposedLayer failed");
        return -1;
    }

    src     = mComposedLayer->getSprdSRCRect();
    fb      = mComposedLayer->getSprdFBRect();
    src->x  = FBTargetLayer->sourceCropf.left;
    src->y  = FBTargetLayer->sourceCropf.top;
    src->w  = FBTargetLayer->sourceCropf.right  - src->x;
    src->h  = FBTargetLayer->sourceCropf.bottom - src->y;

    fb->x   = FBTargetLayer->displayFrame.left;
    fb->y   = FBTargetLayer->displayFrame.top;
    fb->w   = FBTargetLayer->displayFrame.right  - fb->x;
    fb->h   = FBTargetLayer->displayFrame.bottom - fb->y;

    return 0;
}

int SprdPrimaryDisplayDevice:: WrapOverlayLayer(private_handle_t* buf, int format, int fenceFd)
{
    struct sprdRect *src = NULL;
    struct sprdRect *fb  = NULL;

    if (buf == NULL)
    {
        ALOGE("WrapOverlayLayer buf is NULL");
        return -1;
    }

    mComposedLayer = new SprdHWLayer(buf, format, 255, HWC_BLENDING_NONE, 0x00,
                                     fenceFd);

    src     = mComposedLayer->getSprdSRCRect();
    fb      = mComposedLayer->getSprdFBRect();
    src->x  = 0;
    src->y  = 0;
    src->w  = buf->width;
    src->h  = buf->height;

    fb->x   = 0;
    fb->y   = 0;
    fb->w   = buf->width;
    fb->h   = buf->height;

    return 0;
}

int SprdPrimaryDisplayDevice:: SprdUtilScheldule(SprdUtilSource *Source, SprdUtilTarget *Target)
{
    if (Source == NULL || Target == NULL)
    {
        ALOGE("SprdUtilScheldule input source/target is NULL");
        return -1;
    }

#ifdef TRANSFORM_USE_DCAM
    mUtil->transformLayer(Source, Target);
#endif

#ifdef PROCESS_VIDEO_USE_GSP
    if (mOverlayPlane->online())
    {
        Target->format = mOverlayPlane->getPlaneFormat();
    }
    else if (mPrimaryPlane->online())
    {
        Target->format = mPrimaryPlane->getPlaneFormat();
    }

    //if(mUtil->composerLayers(Source, Target))
    if(mUtil->composeLayerList(Source, Target))
    {
        ALOGE("%s[%d],composerLayers ret err!!",__func__,__LINE__);
    }
    else
    {
        ALOGI_IF(mDebugFlag, "%s[%d],composerLayers success",__func__,__LINE__);
    }
#endif

     return 0;
}

#if OVERLAY_COMPOSER_GPU
int SprdPrimaryDisplayDevice:: OverlayComposerScheldule(hwc_display_contents_1_t *list, SprdDisplayPlane *DisplayPlane)
{
    int acquireFenceFd = -1;
    int format = -1;
    private_handle_t* buf = NULL;

    if (list == NULL || DisplayPlane == NULL)
    {
        ALOGE("OverlayComposerScheldule input para is NULL");
        return -1;
    }

    ALOGI_IF(mDebugFlag, "Start OverlayComposer composition misson");

    mOverlayComposer->onComposer(list);
    buf = DisplayPlane->flush(&acquireFenceFd);
    format = DisplayPlane->getPlaneFormat();

    if (WrapOverlayLayer(buf, format, acquireFenceFd))
    {
        ALOGE("OverlayComposerScheldule WrapOverlayLayer failed");
        return -1;
    }

    return 0;
}
#endif

int SprdPrimaryDisplayDevice:: prepare(hwc_display_contents_1_t *list, unsigned int accelerator)
{
    int ret = -1;
    int displayFlag = HWC_DISPLAY_MASK;
    int acceleratorLocal = ACCELERATOR_NON;

    queryDebugFlag(&mDebugFlag);

    ALOGI_IF(mDebugFlag, "HWC start prepare");

    Mutex::Autolock _l(mLock);
    if (mBlank)
    {
        ALOGE("we don't do prepare action when in blanke state");
        return 0;
    }

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

    ret = mLayerList->revisitGeometry(displayFlag, this);
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
    int OverlayBufferFenceFd = -1;
    int PrimaryBufferFenceFd = -1;
    bool PrimaryPlane_Online = false;
    PlaneContext *PrimaryContext = NULL;
    PlaneContext *OverlayContext = NULL;
    SprdDisplayPlane *DisplayPlane = NULL;

    int GXPLayerCount = mLayerList->getGXPLayerCount();
    SprdHWLayer **GXPLayerList = mLayerList->getSprdGXPLayerList();

    mDisplayFBTarget = false;
    mDisplayPrimaryPlane = false;
    mDisplayOverlayPlane = false;
    mDisplayOVC = false;
    mSchedualUtil = false;
    mUtilSource->LayerList = NULL;
    mUtilSource->LayerCount = 0;
    mUtilSource->releaseFenceFd = -1;
    mUtilTarget->buffer = NULL;
    mUtilTarget->acquireFenceFd = -1;
    mUtilTarget->releaseFenceFd = -1;


    Mutex::Autolock _l(mLock);
    if (mBlank)
    {
        ALOGE("we don't do commit action when in blanke state");
        return 0;
    }

    if (list == NULL)
    {
        /*
         * release our resources, the screen is turning off
         * in our case, there is nothing to do.
         * */
         return 0;
    }

    ALOGI_IF(mDebugFlag, "HWC start commit");

    switch ((mHWCDisplayFlag & ~HWC_DISPLAY_MASK))
    {
        case (HWC_DISPLAY_FRAMEBUFFER_TARGET):
            mDisplayFBTarget = true;
            break;
        case (HWC_DISPLAY_PRIMARY_PLANE):
            mDisplayPrimaryPlane = true;
            break;
        case (HWC_DISPLAY_OVERLAY_PLANE):
            mDisplayOverlayPlane = true;
            break;
        case (HWC_DISPLAY_PRIMARY_PLANE |
              HWC_DISPLAY_OVERLAY_PLANE):
            mDisplayPrimaryPlane = true;
            mDisplayOverlayPlane = true;
            break;
        case (HWC_DISPLAY_OVERLAY_COMPOSER_GPU):
            mDisplayOVC = true;
            break;
        case (HWC_DISPLAY_FRAMEBUFFER_TARGET |
              HWC_DISPLAY_OVERLAY_PLANE):
            mDisplayFBTarget = true;
            mDisplayOverlayPlane = true;
            break;
        default:
            ALOGI_IF(mDebugFlag, "Display type: %d, use FBTarget", (mHWCDisplayFlag & ~HWC_DISPLAY_MASK));
            mDisplayFBTarget = true;
            break;
    }

    if (mComposedLayer)
    {
        delete mComposedLayer;
        mComposedLayer = NULL;
    }

    /*
     *  This is temporary methods for displaying Framebuffer target layer, has some bug in FB HAL.
     *  ====     start   ================
     * */
    if (mDisplayFBTarget)
    {
        WrapFBTargetLayer(list);
        goto DisplayDone;
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

    OverlayContext = mOverlayPlane->getPlaneContext();
    PrimaryContext = mPrimaryPlane->getPlaneContext();

#ifdef OVERLAY_COMPOSER_GPU
    if (mDisplayOVC)
    {
        DisplayPlane = mOverlayComposer->getDisplayPlane();
        OverlayComposerScheldule(list, DisplayPlane);
        goto DisplayDone;
    }
#endif

    if (mDisplayOverlayPlane && (!OverlayContext->DirectDisplay))
    {
        mUtilTarget->buffer = mOverlayPlane->dequeueBuffer(&OverlayBufferFenceFd);
        mUtilTarget->releaseFenceFd = OverlayBufferFenceFd;

        mSchedualUtil = true;
    }
    else
    {
        mOverlayPlane->disable();
    }

#ifdef PROCESS_VIDEO_USE_GSP
    PrimaryPlane_Online = (mDisplayPrimaryPlane && (!mDisplayOverlayPlane ));
#else
    PrimaryPlane_Online = mDisplayPrimaryPlane;
#endif

    if (PrimaryPlane_Online)
    {
        mPrimaryPlane->dequeueBuffer(&PrimaryBufferFenceFd);

        if (PrimaryContext->DirectDisplay == false)
        {
            mUtilTarget->buffer2 = mPrimaryPlane->getPlaneBuffer();
            mUtilTarget->releaseFenceFd = PrimaryBufferFenceFd;

            mSchedualUtil = true;
        }
    }
    else
    {
        /*
         *  Use GSP to do 2 layer blending, so if PrimaryLayer is not NULL,
         *  disable DisplayPrimaryPlane.
         * */
        mPrimaryPlane->disable();
        mDisplayPrimaryPlane = false;
    }


    if (mSchedualUtil)
    {
        mUtilSource->LayerList = GXPLayerList;
        mUtilSource->LayerCount = GXPLayerCount;

        SprdUtilScheldule(mUtilSource, mUtilTarget);
        ALOGI_IF(mDebugFlag,"<02-1> SprdUtilScheldule() return, src rlsFd:%d, dst acqFd:%d,dst rlsFd:%d",
                mUtilSource->releaseFenceFd,mUtilTarget->acquireFenceFd,mUtilTarget->releaseFenceFd);

#ifdef HWC_DUMP_CAMERA_SHAKE_TEST
        dumpCameraShakeTest(list);
#endif

        mWindow->notifyDirtyTarget(true);
    }

    if (mOverlayPlane->online())
    {
        int acquireFenceFd = -1;
        private_handle_t* buf = NULL;
        ret = mOverlayPlane->queueBuffer(mUtilTarget->acquireFenceFd);
        buf = mOverlayPlane->flush(&acquireFenceFd);

        if (ret != 0)
        {
            ALOGE("OverlayPlane::queueBuffer failed");
            return -1;
        }

        WrapOverlayLayer(buf, mOverlayPlane->getPlaneFormat(), acquireFenceFd);
        if (mComposedLayer == NULL)
        {
            ALOGE("OverlayPlane Wrap ComposedLayer failed");
            return -1;
        }
    }
    else if (mPrimaryPlane->online())
    {
        int acquireFenceFd = -1;
        private_handle_t* buf = NULL;
        ret = mPrimaryPlane->queueBuffer(mUtilTarget->acquireFenceFd);

        buf = mPrimaryPlane->flush(&acquireFenceFd);

        if (ret != 0)
        {
            ALOGE("PrimaryPlane::queueBuffer failed");
            return -1;
        }

        WrapOverlayLayer(buf, mPrimaryPlane->getPlaneFormat(), acquireFenceFd);
        if (mComposedLayer == NULL)
        {
            ALOGE("OverlayPlane Wrap ComposedLayer failed");
            return -1;
        }
    }

DisplayDone:

    if (mDisplayOVC || mSchedualUtil || mDisplayFBTarget)
    {
        mDispCore->AddFlushData(DISPLAY_PRIMARY, &mComposedLayer, 1);
    }
    else if (PrimaryContext->DirectDisplay || OverlayContext->DirectDisplay)
    {
        /*
         *  Direct Post Android layer to Display.
         *  TODO: implement layter
         * */
        //mDispCore->AddFlushData(DISPLAY_PRIMARY, , );
    }

    return 0;
}

int SprdPrimaryDisplayDevice:: buildSyncData(hwc_display_contents_1_t *list, DisplayTrack *tracker)
{
    int AndroidReleaseFenceFd = -1;//src rel
    SprdDisplayPlane *DisplayPlane = NULL;
    PlaneContext *PrimaryContext = NULL;
    PlaneContext *OverlayContext = NULL;
    OverlayContext = mOverlayPlane->getPlaneContext();
    PrimaryContext = mPrimaryPlane->getPlaneContext();
    DisplayPlane = mOverlayComposer->getDisplayPlane();

    if (list == NULL || tracker == NULL)
    {
        ALOGE("SprdPrimaryDisplayDevice:: buildSyncData input para is NULL");
        return -1;
    }

    if (tracker->releaseFenceFd == -1 || tracker->retiredFenceFd == -1)
    {
        ALOGE("SprdPrimaryDisplayDevice:: buildSyncData input fencefd illegal");
        return -1;
    }

    if (mDisplayFBTarget)//SF-GPU
    {
        AndroidReleaseFenceFd = tracker->releaseFenceFd;
        goto FBTPath;
    }

    if (mSchedualUtil == false)
    {
        goto OVERLAYPath;
    }

    if (mUtilSource->releaseFenceFd >= 0)
    {
       AndroidReleaseFenceFd = mUtilSource->releaseFenceFd;//OV-GSP/GPP
    }

    if (mDisplayOverlayPlane)
    {
        mOverlayPlane->InvalidatePlane();
        mOverlayPlane->addFlushReleaseFence(tracker->releaseFenceFd);
    }

    if (mDisplayPrimaryPlane)
    {
        mPrimaryPlane->InvalidatePlane();
        mPrimaryPlane->addFlushReleaseFence(tracker->releaseFenceFd);
    }

OVERLAYPath:
    if (mDisplayOVC && DisplayPlane != NULL)
    {
        DisplayPlane->InvalidatePlane();
        DisplayPlane->addFlushReleaseFence(tracker->releaseFenceFd);
        AndroidReleaseFenceFd = mOverlayComposer->getReleaseFence();//OV-GPU
    }

    /*
     *  Here, we use Display driver fence as Android layer release
     *  fence, because PrimaryPlane and OverlayPlane can be displayed
     *  directly.
     * */
    if (PrimaryContext->DirectDisplay || OverlayContext->DirectDisplay)
    {
        //AndroidReleaseFenceFd = tracker->releaseFenceFd;
        //tracker->releaseFenceFd = -1;
    }

FBTPath:

    if (BufferSyncBuild(list, AndroidReleaseFenceFd, tracker->retiredFenceFd))
    {
        ALOGE("BufferSyncBuild failed");
    }
    closeAcquireFDs(list,mDebugFlag);
    if (!mDisplayFBTarget)
    {
        ALOGI_IF(mDebugFlag, "<10> close src rel parent:%d.",AndroidReleaseFenceFd);
        closeFence(&AndroidReleaseFenceFd);
    }
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
