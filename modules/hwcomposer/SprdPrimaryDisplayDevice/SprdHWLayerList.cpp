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
 ** File: SprdHWLayerList.cpp         DESCRIPTION                             *
 **                                   Mainly responsible for filtering HWLayer*
 **                                   list, find layers that meet OverlayPlane*
 **                                   and PrimaryPlane specifications and then*
 **                                   mark them as HWC_OVERLAY.               *
 ******************************************************************************
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/

#include "SprdFrameBufferHAL.h"
#include "SprdHWLayerList.h"
#include "dump.h"
#include "SprdUtil.h"

using namespace android;


SprdHWLayerList::~SprdHWLayerList()
{
    if (mLayerList)
    {
        delete [] mLayerList;
        mLayerList = NULL;
    }

    if (mOSDLayerList)
    {
        delete [] mOSDLayerList;
        mOSDLayerList = NULL;
    }
    if (mVideoLayerList)
    {
        delete [] mVideoLayerList;
        mVideoLayerList = NULL;
    }
}

void SprdHWLayerList::dump_yuv(uint8_t* pBuffer,uint32_t aInBufSize)
{
    FILE *fp = fopen("/data/video.data","ab");
    if (fp) {
        fwrite(pBuffer,1,aInBufSize,fp);
        fclose(fp);
    }
}

void SprdHWLayerList::dump_layer(hwc_layer_1_t const* l) {
    ALOGI_IF(mDebugFlag , "\ttype=%d, flags=%08x, handle=%p, tr=%02x, blend=%04x, {%f,%f,%f,%f}, {%d,%d,%d,%d}, planeAlpha: %d",
             l->compositionType, l->flags, l->handle, l->transform, l->blending,
             l->sourceCropf.left,
             l->sourceCropf.top,
             l->sourceCropf.right,
             l->sourceCropf.bottom,
             l->displayFrame.left,
             l->displayFrame.top,
             l->displayFrame.right,
             l->displayFrame.bottom,
             l->planeAlpha);
}

void SprdHWLayerList:: HWCLayerPreCheck()
{
    char value[PROPERTY_VALUE_MAX];

    property_get("debug.hwc.disable", value, "0");

    if (atoi(value) == 1)
    {
        mDisableHWCFlag = true;
    }
    else
    {
        mDisableHWCFlag = false;;
    }
}

bool SprdHWLayerList::IsHWCLayer(hwc_layer_1_t *AndroidLayer)
{
    if (AndroidLayer->flags & HWC_SKIP_LAYER)
    {
        ALOGI_IF(mDebugFlag, "Skip layer");
        return false;
    }

    /*
     *  Here should check buffer usage
     * */

    return true;
}

int SprdHWLayerList:: updateGeometry(hwc_display_contents_1_t *list, int accelerator)
{
    mLayerCount = 0;
    mRGBLayerCount = 0;
    mYUVLayerCount = 0;
    mOSDLayerCount = 0;
    mVideoLayerCount = 0;
    mRGBLayerFullScreenFlag = false;
    mSkipLayerFlag = false;
    mGlobalProtectedFlag = false;
    mAcceleratorMode = ACCELERATOR_NON;
    mAcceleratorMode |= accelerator;

    if (list == NULL)
    {
        ALOGE("updateGeometry input parameter list is NULL");
        return -1;
    }
    mList = list;

    if (mLayerList)
    {
        delete [] mLayerList;
        mLayerList = NULL;
    }

    if (mOSDLayerList)
    {
       delete [] mOSDLayerList;
       mOSDLayerList = NULL;
    }

    if (mVideoLayerList)
    {
        delete [] mVideoLayerList;
        mVideoLayerList = NULL;
    }


    //SprdUtil::test_color_for_prepare(mList);

    queryDebugFlag(&mDebugFlag);
    queryDumpFlag(&mDumpFlag);
    if (HWCOMPOSER_DUMP_ORIGINAL_LAYERS & mDumpFlag)
    {
        dumpImage(mList);
    }

    HWCLayerPreCheck();

    if (mDisableHWCFlag)
    {
        ALOGI_IF(mDebugFlag, "HWComposer is disabled now ...");
        return 0;
    }

    mLayerCount = list->numHwLayers;

    mLayerList = new SprdHWLayer[mLayerCount];
    if (mLayerList == NULL)
    {
        ALOGE("Cannot create Layer list");
        return -1;
    }

    /*
     *  mOSDLayerList and mVideoLayerList should not include
     *  FramebufferTarget layer.
     * */
    mOSDLayerList = new SprdHWLayer*[mLayerCount - 1];
    if (mOSDLayerList == NULL)
    {
        ALOGE("Cannot create OSD Layer list");
        return -1;
    }

    mVideoLayerList = new SprdHWLayer*[mLayerCount - 1];
    if (mVideoLayerList == NULL)
    {
        ALOGE("Cannot create Video Layer list");
        return -1;
    }

    mFBLayerCount = mLayerCount - 1;

    for (unsigned int i = 0; i < mLayerCount; i++)
    {
        hwc_layer_1_t *layer = &list->hwLayers[i];

        ALOGI_IF(mDebugFlag,"process LayerList[%d/%d]", i, mLayerCount - 1);
        dump_layer(layer);

        mLayerList[i].setAndroidLayer(layer);

        if (!IsHWCLayer(layer))
        {
            ALOGI_IF(mDebugFlag, "NOT HWC layer");
            mSkipLayerFlag = true;
            continue;
        }

        if (layer->compositionType == HWC_FRAMEBUFFER_TARGET)
        {
            ALOGI_IF(mDebugFlag, "HWC_FRAMEBUFFER_TARGET layer, ignore it");
            mFBTargetLayer = layer;
            continue;
        }

        //layer->compositionType = HWC_FRAMEBUFFER;
        resetOverlayFlag(&(mLayerList[i]));

        prepareOSDLayer(&(mLayerList[i]));

        prepareVideoLayer(&(mLayerList[i]));

        setOverlayFlag(&(mLayerList[i]), i);
    }

    return 0;
}

int SprdHWLayerList:: revisitGeometry(bool mGspSupport, int *DisplayFlag, SprdPrimaryDisplayDevice *mPrimary)
{
    SprdHWLayer *YUVLayer = NULL;
    SprdHWLayer *RGBLayer = NULL;
    int YUVIndex = 0;
    int RGBIndex = 0;
    uint32_t i = 0;
    bool postProcessVideoCond = false;
    bool singleRGBLayerCond = false;
    int LayerCount = mLayerCount;
    bool accelerateVideoByGSP = false;
    bool accelerateOSDByOVC = false; // OVC: OverlayComposer
    uint32_t GXPMaxComposeOSDLayerCount = 1;
    uint32_t GXPMaxComposeWithVideoLayerCount = GXPMaxComposeOSDLayerCount;
    uint32_t GXPMaxComposeVideoLayerCount = 0;
    bool GXPSupportVideoAndOSDBlending = false;

    if (mDisableHWCFlag)
    {
        return 0;
    }

    if (mPrimary == NULL)
    {
        ALOGE("prdHWLayerList:: revisitGeometry input parameters error");
        return -1;
    }

#ifdef PROCESS_VIDEO_USE_GSP
    GSP_CAPABILITY_T *pGXPPara = static_cast<GSP_CAPABILITY_T *>(mPData);
    GXPMaxComposeOSDLayerCount = pGXPPara->max_layer_cnt;
    GXPMaxComposeWithVideoLayerCount = pGXPPara->max_layer_cnt_with_video;
    GXPMaxComposeVideoLayerCount = pGXPPara->max_videoLayer_cnt;
    GXPSupportVideoAndOSDBlending = pGXPPara->blend_video_with_OSD;
#endif
    /*
     *  revist Overlay layer geometry.
     * */
    uint32_t VideoLayerCount = mVideoLayerCount;

    for (i = 0; i < VideoLayerCount; i++)
    {
        if (!(mVideoLayerList[i]->InitCheck()))
        {
            continue;
        }

        if (mSkipLayerFlag)
        {
            resetOverlayFlag(mVideoLayerList[i]);
            mFBLayerCount++;
            ALOGI_IF(mDebugFlag, "revisit video layer list, find Skip layer L: %d", __LINE__);
            continue;
        }

        YUVLayer = mVideoLayerList[i];
        YUVIndex = YUVLayer->getLayerIndex();

        if ((mFBLayerCount > 0)
            || ((GXPSupportVideoAndOSDBlending == false) && (mOSDLayerCount > 0))
            || (VideoLayerCount + mOSDLayerCount > GXPMaxComposeWithVideoLayerCount)
            || (VideoLayerCount > GXPMaxComposeVideoLayerCount)
            || (mGspSupport == false))
        {
            accelerateVideoByGSP = false;
            YUVLayer->setLayerAccelerator(ACCELERATOR_OVERLAYCOMPOSER);
            ALOGI_IF(mDebugFlag, "GXP cannot process video cnt:%d(max:%d), change accelerator to OVC",
                     VideoLayerCount, GXPMaxComposeVideoLayerCount);
        }
        else
        {
            accelerateVideoByGSP = ((YUVLayer->getAccelerator() == ACCELERATOR_GSP) ||
                               (YUVLayer->getAccelerator() == ACCELERATOR_GSP_IOMMU));
        }
    }

    /*
     *  revist OSD layer geometry.
     * */
    uint32_t OSDLayerCount = mOSDLayerCount;

    for (i = 0; i < OSDLayerCount; i++)
    {
        if (!(mOSDLayerList[i]->InitCheck()))
        {
            continue;
        }

        RGBLayer = mOSDLayerList[i];
        RGBIndex = RGBLayer->getLayerIndex();
        if (RGBLayer == NULL)
        {
            continue;
        }

        /*
         *  If the video layer do not exist, and one layer can be processed by GXP,
         *  another layer cannot be processed by GXP,
         *  we should disable all OSD Overlay.
         * */
        if ((mPrivateFlag[0] == 1)
            && (mVideoLayerCount == 0)
            && (mOSDLayerCount > 0))
        {
            resetOverlayFlag(mOSDLayerList[i]);
            mFBLayerCount++;
            ALOGI_IF(mDebugFlag, "No video, one OSD cannot be accerlated by GXP, also should disable other OSD");
            continue;
        }

        accelerateOSDByOVC = (RGBLayer->getAccelerator() == ACCELERATOR_OVERLAYCOMPOSER) ? true : false;
        if(!mGspSupport)
            accelerateOSDByOVC = true;
#ifdef DIRECT_DISPLAY_SINGLE_OSD_LAYER
        /*
         *  if the RGB layer is bottom layer and there is no other layer,
         *  go overlay.
         * */
        singleRGBLayerCond = ((RGBIndex == 0) && (LayerCount == 2));
        if (singleRGBLayerCond)
        {
            ALOGI_IF(mDebugFlag, "Force single OSD layer go to Overlay");
            RGBLayer = mOSDLayerList[i];
            break;
        }
#endif

        /*
         *  Make sure the OSD layer is the top layer and the layer below it
         *  is video layer. If so, go overlay.
         * */
        bool supportYUVLayerCond = false;
        if (YUVLayer)
        {
            supportYUVLayerCond = ((YUVLayer->getLayerIndex())
                                   < RGBLayer->getLayerIndex()) ? true : false;

            /*
             *  If the OSD layer is the bottom layer, video layer is the top layer.
             *  We should disable GXP.
             *  GXP just accept video layer is bottom layer.
             * */
            if (supportYUVLayerCond == false)
            {
                accelerateVideoByGSP = false;
                YUVLayer->setLayerAccelerator(ACCELERATOR_OVERLAYCOMPOSER);
                ALOGI_IF(mDebugFlag, "revisitGeometry GXP cannot handle top video(bottom OSD), switch to OVC");
            }
        }

        /*
         *  At present, the HWComposer cannot handle 2 or more than 2 RGB layer.
         *  So, when found RGB layer count > 1, just switch back to SurfaceFlinger.
         * */
         bool resetOSDLayerCond =
             (supportYUVLayerCond == true) ? ((uint32_t)(mOSDLayerCount + mVideoLayerCount) >
                                               GXPMaxComposeWithVideoLayerCount)
                                             : (((uint32_t)mOSDLayerCount >
                                                 GXPMaxComposeOSDLayerCount)
                                                || (mOSDLayerCount > 0 && mFBLayerCount > 0)
                                                || accelerateOSDByOVC
                                                || mSkipLayerFlag);
        if (resetOSDLayerCond)
        {
            resetOverlayFlag(mOSDLayerList[i]);
            mFBLayerCount++;
            RGBLayer = NULL;
            RGBIndex = 0;
            ALOGI_IF(mDebugFlag , "not support video layer, abandon osd overlay");
            continue;
        }

        /*
         *  At present, some OSD layer cannot not be accelerated by GXP,
         *  So we need change video to OVC.
         * */
        if (YUVLayer && accelerateOSDByOVC)
        {
            accelerateVideoByGSP = false;
            YUVLayer->setLayerAccelerator(ACCELERATOR_OVERLAYCOMPOSER);
            ALOGI_IF(mDebugFlag, "revisitGeometry OSD layer cannot be accerlated by GXP, video switch to OVC");
        }

        RGBLayer = mOSDLayerList[i];
        RGBIndex = RGBLayer->getLayerIndex();
    }


#ifdef DYNAMIC_RELEASE_PLANEBUFFER
    int ret = -1;
    bool holdCond = false;

#ifdef DIRECT_DISPLAY_SINGLE_OSD_LAYER
    if (YUVLayer != NULL || singleRGBLayerCond)
    {
        holdCond = true;
    }
#else
    if (YUVLayer != NULL)
    {
        holdCond = true;
    }
#endif

    ret = mPrimary->reclaimPlaneBuffer(holdCond);
    if (ret == 1)
    {
        resetOverlayFlag(YUVLayer);
        mFBLayerCount++;
        YUVLayer = NULL;
        ALOGI_IF(mDebugFlag, "alloc plane buffer failed, reset video layer");

        if (RGBLayer)
        {
            resetOverlayFlag(RGBLayer);
            mFBLayerCount++;
            RGBLayer = NULL;
            ALOGI_IF(mDebugFlag, "alloc plane buffer failed, reset OSD layer");
        }
    }
#endif


    if ((YUVLayer != NULL) && (accelerateVideoByGSP == false))
    {
        postProcessVideoCond = true;
    }
    else if (singleRGBLayerCond && (accelerateOSDByOVC == true))
    {
        ALOGI_IF(mDebugFlag, "revisitGeometry no accelerator found, disable single OSD Overlay");
        if (RGBLayer)
        {
            RGBLayer->resetAccelerator();
            resetOverlayFlag(RGBLayer);
            mFBLayerCount++;
        }
    }

    if (postProcessVideoCond)
    {
#ifndef OVERLAY_COMPOSER_GPU
        for (int i = 0; i < LayerCount; i++)
        {
            SprdHWLayer *SprdLayer = &(mLayerList[i]);
            if (SprdLayer == NULL)
            {
                continue;
            }

            if (SprdLayer->InitCheck())
            {
                resetOverlayFlag(SprdLayer);
                mFBLayerCount++;
            }
        }
#else
        revisitOverlayComposerLayer(YUVLayer, RGBLayer, LayerCount, &mFBLayerCount, DisplayFlag);
#endif
    }

    ALOGI_IF(mDebugFlag, "Total layer: %d, FB layer: %d, OSD layer: %d, video layer: %d",
            (mLayerCount - 1), mFBLayerCount, mOSDLayerCount, mVideoLayerCount);

    YUVLayer = NULL;
    RGBLayer = NULL;


    return 0;
}

void SprdHWLayerList:: ClearFrameBuffer(hwc_layer_1_t *l, unsigned int index)
{
    if (index != 0)
    {
        l->hints = HWC_HINT_CLEAR_FB;
    }
    else
    {
        l->hints &= ~HWC_HINT_CLEAR_FB;
    }
}

void SprdHWLayerList:: setOverlayFlag(SprdHWLayer *l, unsigned int index)
{
    hwc_layer_1_t *layer = l->getAndroidLayer();

    switch (l->getLayerType())
    {
        case LAYER_OSD:
            l->setSprdLayerIndex(mOSDLayerCount);
            mOSDLayerList[mOSDLayerCount] = l;
            mOSDLayerCount++;
            forceOverlay(l);
            ClearFrameBuffer(layer, index);
            break;
        case LAYER_OVERLAY:
            l->setSprdLayerIndex(mVideoLayerCount);
            mVideoLayerList[mVideoLayerCount] = l;
            mVideoLayerCount++;
            forceOverlay(l);
            ClearFrameBuffer(layer, index);
            break;
        default:
            break;
    }

    l->setLayerIndex(index);
}

void SprdHWLayerList:: forceOverlay(SprdHWLayer *l)
{
    if (l == NULL)
    {
        ALOGE("Input parameters SprdHWLayer is NULL");
        return;
    }

    hwc_layer_1_t *layer = l->getAndroidLayer();
    layer->compositionType = HWC_OVERLAY;

}

void SprdHWLayerList:: resetOverlayFlag(SprdHWLayer *l)
{
    if (l == NULL)
    {
        ALOGI_IF(mDebugFlag, "SprdHWLayer is NULL");
        return;
    }

    hwc_layer_1_t *layer = l->getAndroidLayer();
    if (layer)
    {
        layer->compositionType = HWC_FRAMEBUFFER;
    }
    int index = l->getSprdLayerIndex();

    if (index < 0)
    {
        return;
    }

    switch (l->getLayerType())
    {
        case LAYER_OSD:
            mOSDLayerList[index] = NULL;
            mOSDLayerCount--;
            break;
        case LAYER_OVERLAY:
            mVideoLayerList[index] = NULL;
            mVideoLayerCount--;
            break;
        default:
            return;
    }
}

int SprdHWLayerList:: prepareOSDLayer(SprdHWLayer *l)
{
    hwc_layer_1_t *layer = l->getAndroidLayer();
    const native_handle_t *pNativeHandle = layer->handle;
    struct private_handle_t *privateH = (struct private_handle_t *)pNativeHandle;
    struct sprdRect *srcRect = l->getSprdSRCRect();
    struct sprdRect *FBRect  = l->getSprdFBRect();

    unsigned int mFBWidth  = mFBInfo->fb_width;
    unsigned int mFBHeight = mFBInfo->fb_height;

    int sourceLeft   = (int)(layer->sourceCropf.left);
    int sourceTop    = (int)(layer->sourceCropf.top);
    int sourceRight  = (int)(layer->sourceCropf.right);
    int sourceBottom = (int)(layer->sourceCropf.bottom);

    if (privateH == NULL)
    {
        ALOGI_IF(mDebugFlag, "layer handle is NULL");
        return -1;
    }

    if (!(l->checkRGBLayerFormat()))
    {
        ALOGI_IF(mDebugFlag, "prepareOSDLayer NOT RGB format layer Line:%d", __LINE__);
        return 0;
    }

    mRGBLayerCount++;

    if ((privateH->usage & GRALLOC_USAGE_PROTECTED) == GRALLOC_USAGE_PROTECTED)
    {
        ALOGI_IF(mDebugFlag, "prepareOSDLayer do not process RGB DRM Line:%d", __LINE__);
        return 0;
    }

    if (privateH->usage & GRALLOC_USAGE_HW_TILE_ALIGN)
    {
        ALOGI_IF(mDebugFlag, "prepareOSDLayer do not support Tile align layer Line:%d", __LINE__);
        return 0;
    }

    l->setLayerFormat(privateH->format);
    l->resetAccelerator();

    if (mAcceleratorMode & ACCELERATOR_OVERLAYCOMPOSER)
    {
        l->setLayerAccelerator(ACCELERATOR_OVERLAYCOMPOSER);
    }

    if ((mAcceleratorMode & ACCELERATOR_GSP) && (!(mAcceleratorMode & ACCELERATOR_GSP_IOMMU)))
    {
        if (!(l->checkContiguousPhysicalAddress(privateH)))
        {
            if ((l->getAccelerator() == ACCELERATOR_NON))
            {
                ALOGI_IF(mDebugFlag, "prepareOSDLayer find virtual address Line:%d", __LINE__);
                l->resetAccelerator();
                return 0;
            }
            else
            {
                ALOGI_IF(mDebugFlag, "prepareOSDLayer Use GPU to accelerate L:%d", __LINE__);
            }
        } 
        else
        {
            l->setLayerAccelerator(ACCELERATOR_GSP);
            ALOGI_IF(mDebugFlag, "prepareOSDLayer Use GSP to accelerate L:%d", __LINE__);
        }
    }
    else if (mAcceleratorMode & ACCELERATOR_GSP_IOMMU)
    {
        l->setLayerAccelerator(ACCELERATOR_GSP_IOMMU);
        ALOGI_IF(mDebugFlag, "prepareOSDLayer Use GSPIOMMU to accelerate L:%d", __LINE__);
    }
    else if ((l->getAccelerator() == ACCELERATOR_NON))
    {
        if ((layer->transform != 0) ||
            !(l->checkContiguousPhysicalAddress(privateH)))
        {
            ALOGI_IF(mDebugFlag, "prepareOSDLayer L%d, no accelerator, not PHY, need transform", __LINE__);
            return 0;
        }
    }


    if ((layer->transform != 0) &&
        ((layer->transform & HAL_TRANSFORM_ROT_90) != HAL_TRANSFORM_ROT_90))
    {
        ALOGI_IF(mDebugFlag, "prepareOSDLayer not support the kind of rotation L%d", __LINE__);
        l->resetAccelerator();
        return 0;
    }

    srcRect->x = MAX(sourceLeft, 0);
    srcRect->y = MAX(sourceTop, 0);
    srcRect->w = MIN(sourceRight - sourceLeft, privateH->width);
    srcRect->h = MIN(sourceBottom - sourceTop, privateH->height);

    FBRect->x = MAX(layer->displayFrame.left, 0);
    FBRect->y = MAX(layer->displayFrame.top, 0);
    FBRect->w = MIN(layer->displayFrame.right - layer->displayFrame.left, mFBWidth);
    FBRect->h = MIN(layer->displayFrame.bottom - layer->displayFrame.top, mFBHeight);

    ALOGI_IF(mDebugFlag, "displayFrame[l%d,t%d,r%d,b%d] mFBWidth:%d mFBHeight:%d FBRect[x%d,y%d,w%d,h%d] ",
        layer->displayFrame.left,layer->displayFrame.top,layer->displayFrame.right,layer->displayFrame.bottom,
        mFBWidth,mFBHeight,
        FBRect->x,FBRect->y,FBRect->w,FBRect->h);


    /*
     * Mark full screen layer flag.
     * */
    if ((FBRect->w != mFBWidth) || (FBRect->h != mFBHeight) ||
        (FBRect->x != 0) || (FBRect->y != 0))
    {
        ALOGI_IF(mDebugFlag, "prepareOSDLayer find not full screen layer, L%d", __LINE__);
    }
    else
    {
        mRGBLayerFullScreenFlag = true;
    }


    if ((l->getAccelerator() == ACCELERATOR_GSP_IOMMU) ||
        (l->getAccelerator() == ACCELERATOR_GSP))
    {
        int retValue = prepareForGXP(l);
        if ((retValue == 1) &&
            (mAcceleratorMode & ACCELERATOR_OVERLAYCOMPOSER))
        {
            ALOGI_IF(mDebugFlag, "prepareOSDLayer L%d, cannot use GXP, switch to OVC", __LINE__);
        }
        else if (retValue < 0)
        {
            ALOGI_IF(mDebugFlag, "prepareOSDLayer L%d, NO matched Accelerator", __LINE__);
            return 0;
        }
    }

#if OVERLAY_COMPOSER_GPU
    if (l->getAccelerator() == ACCELERATOR_OVERLAYCOMPOSER)
    {
        int ret = prepareOverlayComposerLayer(l);
        if (ret != 0)
        {
            ALOGI_IF(mDebugFlag, "prepareOverlayComposerLayer find irregular layer, give up OverlayComposerGPU,ret 0, L%d", __LINE__);
            l->resetAccelerator();
            return 0;
        }
    }
#endif

    /*
     *  If OSD layer cannot be accerlated by GXP,
     *  can be accerlated by OverlayComposer,
     *  Should use a flag to record this thing.
     * */
    if (l->getAccelerator() == ACCELERATOR_OVERLAYCOMPOSER)
    {
        mPrivateFlag[0] = 1;
    }
    else
    {
         mPrivateFlag[0] = 0;
    }

    l->setLayerType(LAYER_OSD);
    ALOGI_IF(mDebugFlag, "prepareOSDLayer[L%d],set type OSD, accelerator: 0x%x", __LINE__, l->getAccelerator());

    mFBLayerCount--;
    return 0;
}

int SprdHWLayerList:: prepareVideoLayer(SprdHWLayer *l)
{
    hwc_layer_1_t *layer = l->getAndroidLayer();
    const native_handle_t *pNativeHandle = layer->handle;
    struct private_handle_t *privateH = (struct private_handle_t *)pNativeHandle;
    struct sprdRect *srcRect = l->getSprdSRCRect();
    struct sprdRect *FBRect  = l->getSprdFBRect();

    unsigned int mFBWidth  = mFBInfo->fb_width;
    unsigned int mFBHeight = mFBInfo->fb_height;

    int sourceLeft   = (int)(layer->sourceCropf.left);
    int sourceTop    = (int)(layer->sourceCropf.top);
    int sourceRight  = (int)(layer->sourceCropf.right);
    int sourceBottom = (int)(layer->sourceCropf.bottom);

    if (privateH == NULL)
    {
        ALOGI_IF(mDebugFlag, "layer handle is NULL");
        return -1;
    }

    if ((privateH->usage & GRALLOC_USAGE_PROTECTED) == GRALLOC_USAGE_PROTECTED)
    {
        l->setProtectedFlag(true);
        mGlobalProtectedFlag = true;
        ALOGI_IF(mDebugFlag, "prepareVideoLayer L: %d, find protected video",
                 __LINE__);
    }
    else
    {
        l->setProtectedFlag(false);
    }

    /*
     *  Some RGB DRM video should also be considered as video layer
     *  which must be processed by HWC.
     * */
    if ((!(l->checkYUVLayerFormat()))
        && (l->getProtectedFlag() == false))
    {
        ALOGI_IF(mDebugFlag, "prepareVideoLayer L%d,color format:0x%08x,ret 0", __LINE__, privateH->format);
        return 0;
    }

    l->setLayerFormat(privateH->format);

    mYUVLayerCount++;

    l->resetAccelerator();

    if (privateH->usage & GRALLOC_USAGE_HW_TILE_ALIGN)
    {
        ALOGI_IF(mDebugFlag, "prepareVideoLayer do not support Tile align layer Line:%d", __LINE__);
        return 0;
    }

    if (mAcceleratorMode & ACCELERATOR_GSP_IOMMU)
    {
        l->setLayerAccelerator(ACCELERATOR_GSP_IOMMU);
        ALOGI_IF(mDebugFlag, "prepareOverlayLayer L%d, Use GSP_IOMMU to accelerate", __LINE__);
    }
    else if (mAcceleratorMode & ACCELERATOR_GSP)
    {
        if (!(l->checkContiguousPhysicalAddress(privateH)))
        {
            if (mAcceleratorMode & ACCELERATOR_OVERLAYCOMPOSER)
            {
                l->setLayerAccelerator(ACCELERATOR_OVERLAYCOMPOSER);
                ALOGI_IF(mDebugFlag, "prepareOverlayLayer L%d, Use GPU to accelerate", __LINE__);
            }
            else
            {
                ALOGI_IF(mDebugFlag, "prepareOverlayLayer L%d,no accelerator, flags: 0x%x, ret 0 \n", __LINE__, privateH->flags);
                return 0;
            }
        }
       else
       {
            l->setLayerAccelerator(ACCELERATOR_GSP);
            ALOGI_IF(mDebugFlag, "prepareOverlayLayer L%d, Use GSP to accelerate", __LINE__);
       }
    }
    else if (mAcceleratorMode & ACCELERATOR_OVERLAYCOMPOSER)
    {
        l->setLayerAccelerator(ACCELERATOR_OVERLAYCOMPOSER);
        ALOGI_IF(mDebugFlag, "prepareOverlayLayer L%d, Use GPU to accelerate", __LINE__);
    }

    if(l->checkNotSupportOverlay(privateH))
    {
        ALOGI_IF(mDebugFlag, "prepareOverlayLayer L%d, not support Ovelray, flags: 0x%x, ret 0 \n", __LINE__, privateH->flags);
        l->resetAccelerator();
        return 0;
    }


    if(layer->blending != HWC_BLENDING_NONE)
    {
       ALOGI_IF(mDebugFlag, "prepareVideoLayer L%d,blend:0x%08x,ret 0", __LINE__, layer->blending);
        l->resetAccelerator();
        return 0;
    }

    srcRect->x = MAX(sourceLeft, 0);
    srcRect->y = MAX(sourceTop, 0);
    srcRect->w = MIN(sourceRight - sourceLeft, privateH->width);
    srcRect->h = MIN(sourceBottom - sourceTop, privateH->height);

    FBRect->x = MAX(layer->displayFrame.left, 0);
    FBRect->y = MAX(layer->displayFrame.top, 0);
    FBRect->w = MIN(layer->displayFrame.right - layer->displayFrame.left, mFBWidth);
    FBRect->h = MIN(layer->displayFrame.bottom - layer->displayFrame.top, mFBHeight);

#ifdef TRANSFORM_USE_DCAM
    int ret = DCAMTransformPrepare(layer, srcRect, FBRect);
    if (ret != 0)
    {
        return 0;
    }
#endif

    ALOGV("rects {%d,%d,%d,%d}, {%d,%d,%d,%d}", srcRect->x, srcRect->y, srcRect->w, srcRect->h,
          FBRect->x, FBRect->y, FBRect->w, FBRect->h);

    if (privateH->format == HAL_PIXEL_FORMAT_YV12
        ||privateH->yuv_info == MALI_YUV_BT709_NARROW)
    {
         if (mAcceleratorMode & ACCELERATOR_OVERLAYCOMPOSER)
         {
                l->setLayerAccelerator(ACCELERATOR_OVERLAYCOMPOSER);
                ALOGI_IF(mDebugFlag, "prepareOverlayLayer L%d, GSP do not support YV12 and BT709, Use GPU to accelerate", __LINE__);
         }
         else
         {
                ALOGI_IF(mDebugFlag, "prepareVideoLayer L%d, NO matched Accelerator", __LINE__);
                return 0;
         }
    }

    if ((l->getAccelerator() == ACCELERATOR_GSP_IOMMU) ||
        (l->getAccelerator() == ACCELERATOR_GSP))
    {
        int retValue = prepareForGXP(l);
        if ((retValue == 1) &&
            (mAcceleratorMode & ACCELERATOR_OVERLAYCOMPOSER))
        { //gsp support [1/16-gsp_scaling_up_limit] scaling
            ALOGI_IF(mDebugFlag, "prepareVideoLayer L%d, cannot use GXP, switch to OVC", __LINE__);
        }
        else if (retValue != 0)
        {
            ALOGI_IF(mDebugFlag, "prepareVideoLayer L%d, NO matched Accelerator", __LINE__);
            return 0;
        }
    }
    else if (l->getAccelerator() != ACCELERATOR_OVERLAYCOMPOSER)
    {
        if(layer->transform == HAL_TRANSFORM_FLIP_V)
        {
           ALOGI_IF(mDebugFlag, "prepareVideoLayer L%d,transform:0x%08x,ret 0", __LINE__, layer->transform);
            l->resetAccelerator();
            return 0;
        }

        if((layer->transform == (HAL_TRANSFORM_ROT_90 | HAL_TRANSFORM_FLIP_H))
            || (layer->transform == (HAL_TRANSFORM_ROT_90 | HAL_TRANSFORM_FLIP_V)))
        {
            ALOGI_IF(mDebugFlag, "prepareVideoLayer L%d,transform:0x%08x,ret 0", __LINE__, layer->transform);
            l->resetAccelerator();
            return 0;
        }
    }

    l->setLayerType(LAYER_OVERLAY);
    ALOGI_IF(mDebugFlag, "prepareVideoLayer[L%d],set type Video, accelerator: 0x%x", __LINE__, l->getAccelerator());

    mFBLayerCount--;

    return 0;
}

int SprdHWLayerList::prepareForGXP(SprdHWLayer *l)
{
    uint32_t srcWidth;
    uint32_t srcHeight;
    uint32_t destWidth;
    uint32_t destHeight;

    if (l == NULL)
    {
        ALOGI_IF(mDebugFlag, "prepareForGXP input SprdHWLayer is NULL L:%d", __LINE__);
        return -1;
    }

    hwc_layer_1_t *layer = l->getAndroidLayer();
    if (layer == NULL)
    {
        ALOGI_IF(mDebugFlag, "prepareForGXP input AndroidLayer is NULL L:%d", __LINE__);
        return -1;
    }

    const native_handle_t *pNativeHandle = layer->handle;
    struct private_handle_t *privateH = (struct private_handle_t *)pNativeHandle;
    if (privateH == NULL)
    {
        ALOGI_IF(mDebugFlag, "prepareForGXP input handle is NULL L:%d", __LINE__);
        return -1;
    }

#ifdef PROCESS_VIDEO_USE_GSP
    struct sprdRect *srcRect = l->getSprdSRCRect();
    struct sprdRect *FBRect  = l->getSprdFBRect();

    unsigned int mFBWidth  = mFBInfo->fb_width;
    unsigned int mFBHeight = mFBInfo->fb_height;
    int gxp_scaling_up_limit = 4;
    int gxp_scaling_down_limit = 0;

    destWidth = FBRect->w;
    destHeight = FBRect->h;
    if (((layer->transform&HAL_TRANSFORM_ROT_90) == HAL_TRANSFORM_ROT_90)
        || ((layer->transform&HAL_TRANSFORM_ROT_270) == HAL_TRANSFORM_ROT_270))
    {
        srcWidth = srcRect->h;
        srcHeight = srcRect->w;
    }
    else
    {
        srcWidth = srcRect->w;
        srcHeight = srcRect->h;
    }

    if (mPData == NULL)
    {
        ALOGI_IF(mDebugFlag,"prepareForGXP, GXP not enable, L%d, switch to OVC", __LINE__);
        l->setLayerAccelerator(ACCELERATOR_OVERLAYCOMPOSER);
        return 1;
    }

    GSP_CAPABILITY_T *pGXPPara = static_cast<GSP_CAPABILITY_T *>(mPData);

    /*
     *  If yuv_xywh_even == 1, GXP do not support odd source layer.
     * */

    if ((pGXPPara->yuv_xywh_even == 1) && (l->getLayerType() == LAYER_OVERLAY))
    {
        if ((srcRect->x % 2)
            || (srcRect->y % 2)
            || (srcRect->w % 2)
            || (srcRect->h % 2))
        {
            ALOGI_IF(mDebugFlag, "prepareForGXP, GXP do not support odd source layer L%d, switch to OVC", __LINE__);
            l->setLayerAccelerator(ACCELERATOR_OVERLAYCOMPOSER);
            return 1;
        }
    }

    if(srcWidth < (uint32_t)(pGXPPara->crop_min.w)
       || srcHeight < (uint32_t)(pGXPPara->crop_min.h)
       || srcWidth > (uint32_t)(pGXPPara->crop_max.w)
       || srcHeight > (uint32_t)(pGXPPara->crop_max.h)
       || FBRect->w < (uint32_t)(pGXPPara->out_min.w)
       || FBRect->h < (uint32_t)(pGXPPara->out_min.h)
       || FBRect->w > (uint32_t)(pGXPPara->out_max.w)
       || FBRect->h > (uint32_t)(pGXPPara->out_max.h)
       || FBRect->w > mFBWidth // when HWC do blending by GSP, the output can't larger than LCD width and height
       || FBRect->h > mFBHeight) {
       ALOGI_IF(mDebugFlag,"prepareForGXP, out of scailing limit, L%d, switch to OVC",__LINE__);
        l->setLayerAccelerator(ACCELERATOR_OVERLAYCOMPOSER);
        return 1;
    }

    gxp_scaling_up_limit = pGXPPara->scale_range_up / 16;
    gxp_scaling_down_limit = 16 * pGXPPara->scale_range_down;

    if(gxp_scaling_up_limit * srcWidth < destWidth || srcWidth > gxp_scaling_down_limit * destWidth ||
    gxp_scaling_up_limit * srcHeight < destHeight || srcHeight > gxp_scaling_down_limit * destHeight)
    { //gsp support [1/16-gsp_scaling_up_limit] scaling
        ALOGI_IF(mDebugFlag,"prepareForGXP[%d], GXP only support 1/16-%d scaling! switch to OVC",__LINE__,gxp_scaling_up_limit);
        l->setLayerAccelerator(ACCELERATOR_OVERLAYCOMPOSER);
        return 1;
    }

    if((destHeight != srcHeight) || (destWidth != srcWidth))
    {
        //for scaler input > 4x4
        if((srcWidth <= 4) || (srcHeight <= 4))
        {
            ALOGI_IF(mDebugFlag,"prepareForGXP[%d], GXP do not support scaling input <= 4x4 ! %d",__LINE__);
            l->setLayerAccelerator(ACCELERATOR_OVERLAYCOMPOSER);
            return 1;
        }
    }

    //for blend require the rect's height >= 32
    if((destHeight < 32) || (srcWidth < 32) || (srcHeight < 32))
    {
        ALOGI_IF(mDebugFlag,"prepareForGXP[%d], GXP do not support blend with rect height < 32 ! %d",__LINE__);
        l->setLayerAccelerator(ACCELERATOR_OVERLAYCOMPOSER);
        return 1;
    }

    if(destHeight<srcHeight)//scaling down
    {
        uint32_t div = 1;

        if(destHeight*4 >= srcHeight)//
        {
            div = 32;
        }else if(destHeight*8 >= srcHeight)
        {
            div = 64;
        }else if(destHeight*16 >= srcHeight)
        {
            div = 128;
        }

        if(srcHeight/div*div != srcHeight)
        {
            if((srcHeight/div*div*destHeight) > (srcHeight*(destHeight-1)+1))
            {
                ALOGI_IF(mDebugFlag,"prepareForGXP[%d], GXP can't support %dx%d->%dx%d scaling!",__LINE__,
                srcWidth,srcHeight,destWidth,destHeight);
                l->setLayerAccelerator(ACCELERATOR_OVERLAYCOMPOSER);
                return 1;
            }
        }
    }


    /*
     *  The GXP do not support scailing up and down at the same time.
     * */
    if (pGXPPara->scale_updown_sametime == 0)
    {
        if(((srcWidth < destWidth) && (srcHeight > destHeight))
        || ((srcWidth > destWidth) && (srcHeight < destHeight)))
        {
            ALOGI_IF(mDebugFlag,"prepareForGXP[%d], GXP not support one direction scaling down while the other scaling up! ret 0",__LINE__);
            l->setLayerAccelerator(ACCELERATOR_OVERLAYCOMPOSER);
            return 1;
        }
    }

    if ((pGXPPara->OSD_scaling == 0) &&
        (l->checkRGBLayerFormat()))
    {
        if ((srcWidth != (uint32_t)destWidth)
            || (srcHeight != (uint32_t)destHeight))
        {
            ALOGI_IF(mDebugFlag, "prepareForGXP[%d] GXP do not support RGB scailing now, src(w:%d, h:%d), des(w:%d, h:%d)", __LINE__, srcWidth, srcHeight, destWidth, destHeight);
            l->setLayerAccelerator(ACCELERATOR_OVERLAYCOMPOSER);
            return 1;
        }
    }

    if (l->checkYUVLayerFormat())
    {
        if ((pGXPPara->max_video_size == 0)
             && (srcWidth > 1920 && srcHeight > 1080))
        {
            ALOGI_IF(mDebugFlag,"prepareForGXP[%d], GXP not support > 1080P video",__LINE__);
            l->setLayerAccelerator(ACCELERATOR_OVERLAYCOMPOSER);
            return 1;
        }
        else if ((pGXPPara->max_video_size == 1)
             && (srcWidth > 1280 && srcHeight > 720))
        {
         ALOGI_IF(mDebugFlag,"prepareForGXP[%d], GXP not support > 720P video",__LINE__);
            l->setLayerAccelerator(ACCELERATOR_OVERLAYCOMPOSER);
            return 1;
        }
    }

    return 0;
#else
    return 1;
#endif
}

int SprdHWLayerList::prepareOverlayComposerLayer(SprdHWLayer *l)
{
#ifdef OVERLAY_COMPOSER_GPU
    hwc_layer_1_t *layer = l->getAndroidLayer();
    if (layer == NULL)
    {
        ALOGE("prepareOverlayComposerLayer input layer is NULL");
        return -1;
    }

    int sourceLeft   = (int)(layer->sourceCropf.left);
    int sourceTop    = (int)(layer->sourceCropf.top);
    int sourceRight  = (int)(layer->sourceCropf.right);
    int sourceBottom = (int)(layer->sourceCropf.bottom);

    if (layer->displayFrame.left < 0 ||
        layer->displayFrame.top < 0 ||
        layer->displayFrame.left > mFBInfo->fb_width ||
        layer->displayFrame.top > mFBInfo->fb_height ||
        layer->displayFrame.right > mFBInfo->fb_width ||
        layer->displayFrame.bottom > mFBInfo->fb_height){
        mSkipLayerFlag = true;
        return -1;
    }

    if (sourceLeft < 0 ||
        sourceTop < 0 ||
        sourceBottom < 0 ||
        sourceRight < 0 ||
        sourceLeft > mFBInfo->fb_width ||
        sourceTop > mFBInfo->fb_height ||
        sourceBottom-sourceTop > mFBInfo->fb_height ||
        sourceRight -sourceLeft > mFBInfo->fb_width)
    {
        mSkipLayerFlag = true;
        return -1;
    }
#endif

    return 0;
}

int SprdHWLayerList:: revisitOverlayComposerLayer(SprdHWLayer *YUVLayer, SprdHWLayer *RGBLayer, int LayerCount, int *FBLayerCount, int *DisplayFlag)
{
#ifdef OVERLAY_COMPOSER_GPU
    int displayType = HWC_DISPLAY_MASK;

    /*
     *  At present, OverlayComposer cannot handle 2 or more than 2 YUV layers.
     *  And OverlayComposer do not handle cropped RGB layer except DRM video.
     *  DRM video must go into Overlay.
     * */
    if (YUVLayer == NULL)
    {
        ALOGI_IF(mDebugFlag, "revisitOverlayComposerLayer must include video layer");
        mSkipLayerFlag = true;
    }

    if (mSkipLayerFlag == false)
    {
        for (int j = 0; j < LayerCount; j++)
        {
            SprdHWLayer *SprdLayer = &(mLayerList[j]);
            if (SprdLayer == NULL)
            {
                continue;
            }

            hwc_layer_1_t *l = SprdLayer->getAndroidLayer();

            if (l == NULL)
            {
                continue;
            }

            if (!IsHWCLayer(l))
            {
                continue;
            }

            if (l->compositionType == HWC_FRAMEBUFFER_TARGET)
            {
                continue;
            }

            if (mGlobalProtectedFlag)
            {
                ALOGI_IF(mDebugFlag, "Find Protected Video layer, force Overlay");
                mSkipLayerFlag = false;
            }
            else if (mYUVLayerCount > 0)
            {
                ALOGI_IF(mDebugFlag, "Not find protected video, switch to SF");
                mSkipLayerFlag = true;
                break;
            }

            if (l->compositionType == HWC_FRAMEBUFFER)
            {
                int format = SprdLayer->getLayerFormat();
                if (SprdLayer->checkRGBLayerFormat())
                {
                    SprdLayer->setLayerType(LAYER_OSD);
                }
                else if (SprdLayer->checkYUVLayerFormat())
                {
                    SprdLayer->setLayerType(LAYER_OVERLAY);
                }
                ALOGI_IF(mDebugFlag, "Force layer format:%d go into OVC", format);
                setOverlayFlag(SprdLayer, j);

                (*FBLayerCount)--;
            }
        }
        displayType |= HWC_DISPLAY_OVERLAY_COMPOSER_GPU;
    }

    if (mSkipLayerFlag)
    {
        for (int i = 0; i < LayerCount; i++)
        {
            SprdHWLayer *SprdLayer = &(mLayerList[i]);
            if (SprdLayer == NULL)
            {
                continue;
            }

            if (SprdLayer->InitCheck())
            {
                resetOverlayFlag(SprdLayer);
                (*FBLayerCount)++;
            }
        }
    }


     /*
      *  When Skip layer is found, SurfaceFlinger maybe want to do the Animation,
      *  or other thing, here just disable OverlayComposer.
      *  Switch back to SurfaceFlinger for composition.
      *  At present, it is just a workaround method.
      * */
     if (mSkipLayerFlag)
     {
         displayType &= ~HWC_DISPLAY_OVERLAY_COMPOSER_GPU;
     }

     *DisplayFlag |= displayType;

      mSkipLayerFlag = false;
#endif

      return 0;
}

#ifdef TRANSFORM_USE_DCAM
int SprdHWLayerList:: DCAMTransformPrepare(hwc_layer_1_t *layer, struct sprdRect *srcRect, struct sprdRect *FBRect)
{
    hwc_layer_1_t *layer = l->getAndroidLayer();
    const native_handle_t *pNativeHandle = layer->handle;
    struct private_handle_t *privateH = (struct private_handle_t *)pNativeHandle;
    struct sprdYUV srcImg;
    unsigned int mFBWidth  = mFBInfo->fb_width;
    unsigned int mFBHeight = mFBInfo->fb_height;

    srcImg.format = privateH->format;
    srcImg.w = privateH->width;
    srcImg.h = privateH->height;

    int rot_90_270 = (layer->transform & HAL_TRANSFORM_ROT_90) == HAL_TRANSFORM_ROT_90;

    srcRect->x = MAX(layer->sourceCrop.left, 0);
    srcRect->x = (srcRect->x + SRCRECT_X_ALLIGNED) & (~SRCRECT_X_ALLIGNED);//dcam 8 pixel crop
    srcRect->x = MIN(srcRect->x, srcImg.w);
    srcRect->y = MAX(layer->sourceCrop.top, 0);
    srcRect->y = (srcRect->y + SRCRECT_Y_ALLIGNED) & (~SRCRECT_Y_ALLIGNED);//dcam 8 pixel crop
    srcRect->y = MIN(srcRect->y, srcImg.h);

    srcRect->w = MIN(layer->sourceCrop.right - layer->sourceCrop.left, srcImg.w - srcRect->x);
    srcRect->h = MIN(layer->sourceCrop.bottom - layer->sourceCrop.top, srcImg.h - srcRect->y);

    if((srcRect->w - (srcRect->w & (~SRCRECT_WIDTH_ALLIGNED)))> ((SRCRECT_WIDTH_ALLIGNED+1)>>1))
    {
        srcRect->w = (srcRect->w + SRCRECT_WIDTH_ALLIGNED) & (~SRCRECT_WIDTH_ALLIGNED);//dcam 8 pixel crop
    } else
    {
        srcRect->w = (srcRect->w) & (~SRCRECT_WIDTH_ALLIGNED);//dcam 8 pixel crop
    }

    if((srcRect->h - (srcRect->h & (~SRCRECT_HEIGHT_ALLIGNED)))> ((SRCRECT_HEIGHT_ALLIGNED+1)>>1))
    {
        srcRect->h = (srcRect->h + SRCRECT_HEIGHT_ALLIGNED) & (~SRCRECT_HEIGHT_ALLIGNED);//dcam 8 pixel crop
    }
    else
    {
        srcRect->h = (srcRect->h) & (~SRCRECT_HEIGHT_ALLIGNED);//dcam 8 pixel crop
    }

    srcRect->w = MIN(srcRect->w, srcImg.w - srcRect->x);
    srcRect->h = MIN(srcRect->h, srcImg.h - srcRect->y);
    //--------------------------------------------------
    FBRect->x = MAX(layer->displayFrame.left, 0);
    FBRect->y = MAX(layer->displayFrame.top, 0);
    FBRect->x = MIN(FBRect->x, mFBWidth);
    FBRect->y = MIN(FBRect->y, mFBHeight);

    FBRect->w = MIN(layer->displayFrame.right - layer->displayFrame.left, mFBWidth - FBRect->x);
    FBRect->h = MIN(layer->displayFrame.bottom - layer->displayFrame.top, mFBHeight - FBRect->y);
    if((FBRect->w - (FBRect->w & (~FB_WIDTH_ALLIGNED)))> ((FB_WIDTH_ALLIGNED+1)>>1))
    {
        FBRect->w = (FBRect->w + FB_WIDTH_ALLIGNED) & (~FB_WIDTH_ALLIGNED);//dcam 8 pixel and lcdc must 4 pixel for yuv420
    }
    else
    {
        FBRect->w = (FBRect->w) & (~FB_WIDTH_ALLIGNED);//dcam 8 pixel and lcdc must 4 pixel for yuv420
    }

    if((FBRect->h - (FBRect->h & (~FB_HEIGHT_ALLIGNED)))> ((FB_HEIGHT_ALLIGNED+1)>>1))
    {
        FBRect->h = (FBRect->h + FB_HEIGHT_ALLIGNED) & (~FB_HEIGHT_ALLIGNED);//dcam 8 pixel and lcdc must 4 pixel for yuv420
    }
    else
    {
        FBRect->h = (FBRect->h) & (~FB_HEIGHT_ALLIGNED);//dcam 8 pixel and lcdc must 4 pixel for yuv420
    }


    FBRect->w = MIN(FBRect->w, mFBWidth - ((FBRect->x + FB_WIDTH_ALLIGNED) & (~FB_WIDTH_ALLIGNED)));
    FBRect->h = MIN(FBRect->h, mFBHeight - ((FBRect->y + FB_HEIGHT_ALLIGNED) & (~FB_HEIGHT_ALLIGNED)));

    if(srcRect->w < 4 || srcRect->h < 4 ||
       FBRect->w < 4 || FBRect->h < 4 ||
       FBRect->w > 960 || FBRect->h > 960)
    { //dcam scaling > 960 should use slice mode
        ALOGI_IF(mDebugFlag,"prepareVideoLayer, dcam scaling > 960 should use slice mode! L%d",__LINE__);
        return -1;
    }

    if(4 * srcWidth < destWidth || srcWidth > 4 * destWidth ||
       4 * srcHeight < destHeight || srcHeight > 4 * destHeight)
    { //dcam support 1/4-4 scaling
        ALOGI_IF(mDebugFlag,"prepareVideoLayer, dcam support 1/4-4 scaling! L%d",__LINE__);
        return -1;
    }

    return 0;
}
#endif

int SprdHWLayerList::checkHWLayerList(hwc_display_contents_1_t* list)
{
    if (list == NULL)
    {
        ALOGE("input list is NULL");
        return -1;
    }

    if (list != mList)
    {
        ALOGE("input list has been changed");
        return -1;
    }

    if (list->numHwLayers != mLayerCount)
    {
        ALOGE("The input layer count have been changed");
        return -1;
    }

    return 0;
}
