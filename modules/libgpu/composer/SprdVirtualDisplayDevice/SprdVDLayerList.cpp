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
 ** File:SprdVDLayerList.cpp          DESCRIPTION                             *
 **                                   Responsible for traverse Virtual Display*
 **                                   Layer list and mark the layers as       *
 **                                   Overlay which                           *
 **                                   comply with Sprd Virtual Display spec.  *
 ******************************************************************************
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/

#include "SprdVDLayerList.h"


using namespace android;

SprdVDLayerList:: ~SprdVDLayerList()
{
    reclaimSprdHWLayer();
}

int SprdVDLayerList:: updateGeometry(hwc_display_contents_1_t *list)
{
    mOSDLayerCount = 0;
    mVideoLayerCount = 0;
    mLayerCount = 0;

    if (list == NULL)
    {
        ALOGI_IF(mDebugFlag, "SprdVirtualDisplayDevice:: updateGeometry Virtual Display Device maybe closed");
        return 0;
    }

    queryDebugFlag(&mDebugFlag);
    queryDumpFlag(&mDumpFlag);

    /*
     *  Should we reclaim it here?
     * */
    reclaimSprdHWLayer();

    if (HWCOMPOSER_DUMP_ORIGINAL_VD_LAYERS & mDumpFlag)
    {
        dumpImage(list);
    }

    mLayerCount = list->numHwLayers;
    if (mLayerCount <= 0)
    {
        ALOGI_IF(mDebugFlag, "SprdVirtualDisplayDevice:: updateGeometry mLayerCount < 0");
        return 0;
    }

    mLayerList = new SprdHWLayer[mLayerCount];
    if (mLayerList == NULL)
    {
        ALOGE("SprdVirtualDisplayDevice:: updateGeometry Cannot create Layer list");
        return -1;
    }

    /*
     *  mOSDLayerList and mVideoLayerList should not include
     *  FramebufferTarget layer.
     * */
    mOSDLayerList = new SprdHWLayer*[mLayerCount - 1];
    if (mOSDLayerList == NULL)
    {
        ALOGE("SprdVirtualDisplayDevice:: updateGeometry Cannot create OSD Layer list");
        return -1;
    }

    mVideoLayerList = new SprdHWLayer*[mLayerCount - 1];
    if (mVideoLayerList == NULL)
    {
        ALOGE("SprdVirtualDisplayDevice:: updateGeometry Cannot create Video Layer list");
        return -1;
    }

    mFBLayerCount = mLayerCount - 1;

    for (unsigned int i = 0; i < mLayerCount; i++)
    {
         hwc_layer_1_t *layer = &list->hwLayers[i];

         ALOGI_IF(mDebugFlag, "VirtualDisplay process LayerList[%d/%d]", i , mLayerCount);

         dump_layer(layer);

         mLayerList[i].setAndroidLayer(layer);

         if (!IsHWCLayer(layer))
         {
             ALOGI_IF(mDebugFlag, "NOT HWC layer");
             mSkipMode = true;
             continue;
         }

         if (layer->compositionType == HWC_FRAMEBUFFER_TARGET)
         {
             mFBTargetLayer = &mLayerList[i];

             struct private_handle_t *privateH = (struct private_handle_t *)(layer->handle);
             if (privateH == NULL)
             {
                 ALOGI_IF(mDebugFlag, "VirtualDisplay FBT layer privateH is NULL, privateH addr: %p", (void *)privateH);
                 continue;
             }
             ALOGI_IF(mDebugFlag, "VirtualDisplay HWC_FBT layer, ignore it, format: 0x%x", privateH->format);

             if (mFBTargetLayer)
             {
                 mFBTargetLayer->setLayerFormat(privateH->format);
             }
             else
             {
                 ALOGE("Sprd FBT layer is NULL");
             }
             continue;
         }

         resetOverlayFlag(&(mLayerList[i]));

         prepareOSDLayer(&(mLayerList[i]));

         prepareVideoLayer(&(mLayerList[i]));

         setOverlayFlag(&(mLayerList[i]), i);
    }

    return 0;
}

int SprdVDLayerList:: revistGeometry(hwc_display_contents_1_t *list)
{
#ifdef FORCE_HWC_COPY_FOR_VIRTUAL_DISPLAYS
    SprdHWLayer *l = &(mLayerList[0]);
    hwc_layer_1_t *layer = l->getAndroidLayer();
    if (layer == NULL)
    {
        return 0;
    }

    if (!IsHWCLayer(layer))
    {
        ALOGI_IF(mDebugFlag, "SprdVDLayerList:: revistGeometry NOT HWC layer");
        mSkipMode = true;
        return 0;
    }

    struct private_handle_t *privateH = (struct private_handle_t *)(layer->handle);
    if (privateH == NULL)
    {
        ALOGI_IF(mDebugFlag, "SprdVDLayerList:: revistGeometry privateH is NULL");
        return 0;
    }

    if ((privateH->format != HAL_PIXEL_FORMAT_YCbCr_420_SP)
        && (privateH->format != HAL_PIXEL_FORMAT_RGBA_8888)
        && (privateH->format != HAL_PIXEL_FORMAT_RGBX_8888)
        && (privateH->format != HAL_PIXEL_FORMAT_BGRA_8888)
        && (privateH->format != HAL_PIXEL_FORMAT_RGB_888)
        && (privateH->format != HAL_PIXEL_FORMAT_RGB_565))
    {
        ALOGI_IF(mDebugFlag, "SprdVDLayerList:: revistGeometry not support format");
        return 0;
    }

    if ((mLayerCount - 1 == 1)
        && (layer->compositionType != HWC_FRAMEBUFFER_TARGET))
    {
        mLayerList[0].setLayerType(LAYER_OSD);
        setOverlayFlag(&(mLayerList[0]), 0);
        ALOGI_IF(mDebugFlag, "SprdVDLayerList:: revistGeometry find single layer, force goto Overlay");
    }
#endif

    return 0;
}

int SprdVDLayerList:: prepareOSDLayer(SprdHWLayer *l)
{

    return 0;
}

int SprdVDLayerList:: prepareVideoLayer(SprdHWLayer *l)
{

    return 0;
}

void SprdVDLayerList:: reclaimSprdHWLayer()
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

void SprdVDLayerList:: ClearFrameBuffer(hwc_layer_1_t *l, unsigned int index)
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

void SprdVDLayerList:: setOverlayFlag(SprdHWLayer *l, unsigned int index)
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
            mOSDLayerList[index] = NULL;
            mVideoLayerList[index] = NULL;
            break;
    }

    l->setLayerIndex(index);
}

void SprdVDLayerList:: forceOverlay(SprdHWLayer *l)
{
    if (l == NULL)
    {
        ALOGE("Input parameters SprdHWLayer is NULL");
        return;
    }

    hwc_layer_1_t *layer = l->getAndroidLayer();
    layer->compositionType = HWC_OVERLAY;
}

void SprdVDLayerList:: resetOverlayFlag(SprdHWLayer *l)
{
    if (l == NULL)
    {
        ALOGE("SprdHWLayer is NULL");
        return;
    }

    hwc_layer_1_t *layer = l->getAndroidLayer();
    layer->compositionType = HWC_FRAMEBUFFER;
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

void SprdVDLayerList:: dump_layer(hwc_layer_1_t const* l)
{
    ALOGI_IF(mDebugFlag , "\tVirtualDisplay type=%d, flags=%08x, handle=%p, tr=%02x, blend=%04x, {%f,%f,%f,%f}, {%d,%d,%d,%d}",
             l->compositionType, l->flags, l->handle, l->transform, l->blending,
             l->sourceCropf.left,
             l->sourceCropf.top,
             l->sourceCropf.right,
             l->sourceCropf.bottom,
             l->displayFrame.left,
             l->displayFrame.top,
             l->displayFrame.right,
             l->displayFrame.bottom);
}

bool SprdVDLayerList::IsHWCLayer(hwc_layer_1_t *AndroidLayer)
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
