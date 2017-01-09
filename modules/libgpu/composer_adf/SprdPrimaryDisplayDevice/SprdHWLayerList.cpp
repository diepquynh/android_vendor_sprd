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

    if (mGXPLayerList)
    {
        delete [] mGXPLayerList;
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


/*
 *  function:updateGeometry
 *	check the list whether can be process by these accelerator.
 *	this function will init local layer object mLayerList from hwc_display_contents_1_t.
 *	it focus mainly on single layer check.
 *  accelerator:available accelerator for this display.
 *  list:the app layer list that will be composited and show out.
 * */
int SprdHWLayerList:: updateGeometry(hwc_display_contents_1_t *list, int accelerator)
{
    int ret = -1;
    mLayerCount = 0;
    mOSDLayerCount = 0;
    mVideoLayerCount = 0;
    mDispCLayerCount = 0;
    mGXPLayerCount = 0;
    mYUVLayerCount = 0;
    mSkipLayerFlag = false;
    mAcceleratorMode = accelerator;
    mGXPSupport = false;

    if (list == NULL)
    {
        ALOGE("updateGeometry input parameter list is NULL");
        return -1;
    }

    /*
     *  store the list to local.
     * */
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

    if (mGXPLayerList)
    {
        delete [] mGXPLayerList;
        mGXPLayerList = NULL;
    }

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

    /*
     *  list->numHwLayers includes the FramebufferTarget layer.
     * */
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

    mGXPLayerList = new SprdHWLayer*[mLayerCount - 1];
    if (mGXPLayerList == NULL)
    {
        ALOGE("Cannot create GXPLayerList");
        return -1;
    }

    mFBLayerCount = mLayerCount - 1;

    for (unsigned int i = 0; i < mLayerCount; i++)
    {
        hwc_layer_1_t *layer = &list->hwLayers[i];

        ALOGI_IF(mDebugFlag,"process LayerList[%d/%d]", i, (mLayerCount - 1));
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

        resetOverlayFlag(&(mLayerList[i]));

        prepareOSDLayer(&(mLayerList[i]));

        prepareVideoLayer(&(mLayerList[i]));

        setOverlayFlag(&(mLayerList[i]), i);

        /*
         *  If Display Controller cannot handle >= 2 layers, we should disable
         *  DispC.
         *  TODO:
         * */
        if (mAcceleratorMode & ACCELERATOR_DISPC)
        {

        }

        if ((mLayerList[i].getAccelerator() != ACCELERATOR_DISPC) && (mSkipLayerFlag == false))
        {
            unsigned int count = mGXPLayerCount;
            mGXPLayerList[count] = &(mLayerList[i]);
            mGXPLayerCount++;
        }
    }

    /*
     *  Prepare Layer geometry for Sprd Own accerlator: GSP/GPP
     * */
    if (mAcceleratorMode & ACCELERATOR_GSP)
    {
        if (((mGXPLayerCount > 0) && mAccerlator)&& (mSkipLayerFlag == false))
        {
            ret = mAccerlator->Prepare(mGXPLayerList, mGXPLayerCount, mGXPSupport);
            ALOGI_IF(mDebugFlag,"updateGeometry() Prepare return mGXPSupport:%d", mGXPSupport);
            if(mGXPSupport)
            {
                int dis_gsp = 0;
                queryIntFlag("dis.hwc.gsp",&dis_gsp);
                ALOGI_IF(dis_gsp,"updateGeometry() force mGXPSupport from true to false.");
                mGXPSupport = (dis_gsp>0)?false:mGXPSupport;
            }
            if (ret != 0)
            {
                ALOGI_IF(mDebugFlag,"SprdHWLayerList:: updateGeometry SprdUtil Prepare failed ret: %d", ret);
                mGXPSupport = false;
            }
        }
    }

    return 0;
}

/*
 *  function:revisitGeometry
 *	check the list whether can be process by these accelerator.
 *	it checks at a global view on all layers of this frame.
 * */
int SprdHWLayerList:: revisitGeometry(int& DisplayFlag, SprdPrimaryDisplayDevice *mPrimary)
{
    uint32_t i = 0;
    int LayerCount = mLayerCount;
    bool accelerateByDPC = false; // DPC: Display Controller
    bool accelerateByGXP = false; // GXP: GSP/GPP
    bool accelerateByOVC = false; // OVC: OverlayComposer

    if (mDisableHWCFlag)
    {
        return 0;
    }

    if (mPrimary == NULL)
    {
        ALOGE("prdHWLayerList:: revisitGeometry input parameters error");
        return -1;
    }

    /*
     *  revisit Overlay layer geometry.
     *  TODO: if Display Controller support composition job,
     *  We should change some condition.
     * */
    if (mDispCLayerCount > 0)
    {
        accelerateByDPC = true;
    }

    if ((mGXPLayerCount > 0) && mGXPSupport)
    {
        accelerateByGXP = true;
    }

    if (mDispCLayerCount + mGXPLayerCount < (mLayerCount -1))
    {
     //ALOGI_IF(mDebugFlag, "(FILE:%s, line:%d, func:%s) revisitGeometry accelerateByGXP :%d, mGXPLayerCount = %d, mDispCLayerCount = %d, mLayerCount = %d",
     //         __FILE__, __LINE__, __func__, accelerateByGXP, mGXPLayerCount, mDispCLayerCount, mLayerCount);
        /*
         *  DispC and GXP can not handle part of layers.
         *  Should Disable accelerateByDPC and accelerateByGXP
         * */
         //hl changed 0417
       accelerateByDPC = false;
       accelerateByGXP = false;
       mGXPLayerCount = 0;
       mDispCLayerCount = 0;
    }

    if ((accelerateByDPC == false) && (accelerateByGXP == false))
    {
        accelerateByOVC = true;
    }

    if (accelerateByOVC)
    {
        revisitOverlayComposerLayer(DisplayFlag);
    }

#ifdef DYNAMIC_RELEASE_PLANEBUFFER
    int ret = -1;
    bool holdCond = false;

    if (accelerateByDPC || accelerateByGXP || accelerateByOVC)
    {
        holdCond = true;
    }

    ret = mPrimary->reclaimPlaneBuffer(holdCond);
    if (ret == 1)
    {
        for (int i = 0; i < LayerCount; i++)
        {
            SprdHWLayer *l = &(mLayerList[i]);
            resetOverlayFlag(l);
            mFBLayerCount++;
            ALOGI_IF(mDebugFlag, "alloc plane buffer failed, goto FB");
        }
    }
#endif

    ALOGI_IF(mDebugFlag, "Total layer: %d, FB layer: %d, OSD layer: %d, video layer: %d",
            (mLayerCount - 1), mFBLayerCount, mOSDLayerCount, mVideoLayerCount);

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

    if (l->InitCheck())
    {
        ALOGI_IF(mDebugFlag, "setOverlayFlag, Overlay has been marked");
        return;
    }

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

/*
 *  resetOverlayFlag
 *  set hwc_layer_1_t::compositionType to HWC_FRAMEBUFFER,
 *  means the default composition should execute in SF.
 * */
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

/*
 *  prepareOSDLayer
 *  if it's rgb format,init SprdHWLayer obj from hwc_layer_1_t
 *  and check whether these accelerator can process or not.
 * */
int SprdHWLayerList:: prepareOSDLayer(SprdHWLayer *l)
{
    hwc_layer_1_t *layer = l->getAndroidLayer();
    const native_handle_t *pNativeHandle = layer->handle;
    struct private_handle_t *privateH = (struct private_handle_t *)pNativeHandle;
    struct sprdRect *srcRect = l->getSprdSRCRect();
    struct sprdRect *FBRect  = l->getSprdFBRect();

    unsigned int mFBWidth  = mFBInfo->fb_width;
    unsigned int mFBHeight = mFBInfo->fb_height;

    int sourceLeft   = (int)(layer->sourceCropf.left + 0.5);
    int sourceTop    = (int)(layer->sourceCropf.top + 0.5);
    int sourceRight  = (int)(layer->sourceCropf.right + 0.5);
    int sourceBottom = (int)(layer->sourceCropf.bottom + 0.5);

    if (privateH == NULL)
    {
        ALOGI_IF(mDebugFlag, "layer handle is NULL");
        return -1;
    }

    /*
     *  if it's not rgb format,leave it to prepareVideoLayer().
     * */
    if (!(l->checkRGBLayerFormat()))
    {
        ALOGI_IF(mDebugFlag, "prepareOSDLayer NOT RGB format layer Line:%d", __LINE__);
        return 0;
    }

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
    /*
     *  first we set SprdHWLayer::mAccelerator to overlay-GPU,
     *  then we will check DISPC&GSP capability.
     * */
    if (mAcceleratorMode & ACCELERATOR_OVERLAYCOMPOSER)
    {
        l->setLayerAccelerator(ACCELERATOR_OVERLAYCOMPOSER);
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

    if (mAcceleratorMode & ACCELERATOR_DISPC)
    {
        int retValue = prepareForDispC(l);
        if ((retValue == 1) &&
            (mAcceleratorMode & ACCELERATOR_OVERLAYCOMPOSER))
        {
            ALOGI_IF(mDebugFlag, "prepareOSDLayer L%d, cannot use DISPC, switch to OVC", __LINE__);
        }
        else if (retValue < 0)
        {
            ALOGI_IF(mDebugFlag, "prepareOSDLayer L%d, NO matched Accelerator", __LINE__);
            return 0;
        }
        else
        {
            /*
             *  if DISPC can accelerate , set SprdHWLayer::mAccelerator to DISPC
             * */
            l->setLayerAccelerator(ACCELERATOR_DISPC);
            mDispCLayerCount++;
            ALOGI_IF(mDebugFlag, "prepareOSDLayer Use DiscpC to accelerate L:%d", __LINE__);
        }
    }

    /*
     *  if DISPC can't accelerate , check GPU limit.
     * */
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

    /*
     *  if it's rgb format and can be accelerated by DISPC/GSP/GPU,
     *  set it to LAYER_OSD,means it can be process in hwc and is rgb.
     * */
    l->setLayerType(LAYER_OSD);
    ALOGI_IF(mDebugFlag, "prepareOSDLayer[L%d],set type OSD, accelerator: 0x%x", __LINE__, l->getAccelerator());

    mFBLayerCount--;

    return 0;
}

/*
 *  prepareVideoLayer
 *  if it's yuv format,init SprdHWLayer obj from hwc_layer_1_t
 *  and check whether these accelerator can process or not.
 * */
int SprdHWLayerList:: prepareVideoLayer(SprdHWLayer *l)
{
    hwc_layer_1_t *layer = l->getAndroidLayer();
    const native_handle_t *pNativeHandle = layer->handle;
    struct private_handle_t *privateH = (struct private_handle_t *)pNativeHandle;
    struct sprdRect *srcRect = l->getSprdSRCRect();
    struct sprdRect *FBRect  = l->getSprdFBRect();

    unsigned int mFBWidth  = mFBInfo->fb_width;
    unsigned int mFBHeight = mFBInfo->fb_height;

    int sourceLeft   = (int)(layer->sourceCropf.left + 0.5);
    int sourceTop    = (int)(layer->sourceCropf.top + 0.5);
    int sourceRight  = (int)(layer->sourceCropf.right + 0.5);
    int sourceBottom = (int)(layer->sourceCropf.bottom + 0.5);

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

    if (mAcceleratorMode & ACCELERATOR_OVERLAYCOMPOSER)
    {
        l->setLayerAccelerator(ACCELERATOR_OVERLAYCOMPOSER);
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

    if (mAcceleratorMode & ACCELERATOR_DISPC)
    {
        int retValue = prepareForDispC(l);
        if ((retValue == 1) &&
            (mAcceleratorMode & ACCELERATOR_OVERLAYCOMPOSER))
        {
            ALOGI_IF(mDebugFlag, "prepareVideoLayer L%d, cannot use DispC, switch to OVC", __LINE__);
        }
        else if (retValue != 0)
        {
            ALOGI_IF(mDebugFlag, "prepareVideoLayer L%d, NO matched Accelerator", __LINE__);
            return 0;
        }
        else
        {
            l->setLayerAccelerator(ACCELERATOR_DISPC);
            mDispCLayerCount++;
            ALOGI_IF(mDebugFlag, "prepareOverlayLayer L%d, Use DispC to accelerate", __LINE__);
        }
    }
    else if (mAcceleratorMode & ACCELERATOR_OVERLAYCOMPOSER)
    {
        l->setLayerAccelerator(ACCELERATOR_OVERLAYCOMPOSER);
        ALOGI_IF(mDebugFlag, "prepareOverlayLayer L%d, Use OVC to accelerate", __LINE__);
    }
    else if (mAcceleratorMode & ACCELERATOR_NON)
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

    /*
     *  if it's yuv format and can be accelerated by DISPC/GSP/GPU,
     *  set it to LAYER_OSD,means it can be process in hwc and is yuv.
     * */
    l->setLayerType(LAYER_OVERLAY);
    ALOGI_IF(mDebugFlag, "prepareVideoLayer[L%d],set type Video, accelerator: 0x%x", __LINE__, l->getAccelerator());

    mFBLayerCount--;

    return 0;
}

int SprdHWLayerList::prepareForDispC(SprdHWLayer *l)
{
    uint32_t srcWidth;
    uint32_t srcHeight;
    uint32_t destWidth;
    uint32_t destHeight;

    if (l == NULL)
    {
        ALOGI_IF(mDebugFlag, "prepareForDispC input SprdHWLayer is NULL L:%d", __LINE__);
        return -1;
    }

    hwc_layer_1_t *layer = l->getAndroidLayer();
    if (layer == NULL)
    {
        ALOGI_IF(mDebugFlag, "prepareForDispC input AndroidLayer is NULL L:%d", __LINE__);
        return -1;
    }

    const native_handle_t *pNativeHandle = layer->handle;
    struct private_handle_t *privateH = (struct private_handle_t *)pNativeHandle;
    if (privateH == NULL)
    {
        ALOGI_IF(mDebugFlag, "prepareForDispC input handle is NULL L:%d", __LINE__);
        return -1;
    }

#ifdef ACC_BY_DISPC
    struct sprdRect *srcRect = l->getSprdSRCRect();
    struct sprdRect *FBRect  = l->getSprdFBRect();

    unsigned int mFBWidth  = mFBInfo->fb_width;
    unsigned int mFBHeight = mFBInfo->fb_height;

    return 0;
#else
    return 1;
#endif
}

/*
 *  prepareOverlayComposerLayer
 *  check GPU overlay limited.
 * */
int SprdHWLayerList::prepareOverlayComposerLayer(SprdHWLayer *l)
{
#ifndef OVERLAY_COMPOSER_GPU
        mSkipLayerFlag = true;
        return -1;
#endif
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
        layer->displayFrame.right - layer->displayFrame.left > mFBInfo->fb_width ||
        layer->displayFrame.bottom - layer->displayFrame.top > mFBInfo->fb_height)
    {
        mSkipLayerFlag = true;
        return -1;
    }

    if (sourceLeft < 0 ||
        sourceTop < 0 ||
        sourceBottom < 0 ||
        sourceRight < 0 ||
        sourceBottom - sourceTop > mFBInfo->fb_height ||
        sourceRight - sourceLeft > mFBInfo->fb_height)
    {
        mSkipLayerFlag = true;
        return -1;
    }

    return 0;
}

int SprdHWLayerList:: revisitOverlayComposerLayer(int& DisplayFlag)
{
    int LayerCount = mLayerCount;
    int displayType = HWC_DISPLAY_MASK;

#ifndef OVERLAY_COMPOSER_GPU
    mSkipLayerFlag = true;
#endif

    /*
     *  At present, OverlayComposer cannot handle 2 or more than 2 YUV layers.
     *  And OverlayComposer do not handle cropped RGB layer except DRM video.
     *  DRM video must go into Overlay.
     * */
    if (mVideoLayerCount <= 0)
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

                mFBLayerCount--;
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
                mFBLayerCount++;
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
         displayType &= HWC_DISPLAY_MASK;
         displayType |= HWC_DISPLAY_FRAMEBUFFER_TARGET;
     }

     DisplayFlag |= displayType;

      mSkipLayerFlag = false;

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
