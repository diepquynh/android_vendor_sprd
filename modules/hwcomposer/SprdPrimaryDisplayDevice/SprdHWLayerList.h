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
 ** File: SprdHWLayerList.h           DESCRIPTION                             *
 **                                   Mainly responsible for filtering HWLayer*
 **                                   list, find layers that meet OverlayPlane*
 **                                   and PrimaryPlane specifications and then*
 **                                   mark them as HWC_OVERLAY.               *
 ******************************************************************************
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/


#ifndef _SPRD_HWLAYER_LIST_H_
#define _SPRD_HWLAYER_LIST_H_

#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>
#include <utils/RefBase.h>
#include <cutils/atomic.h>
#include <cutils/log.h>
#include "gralloc_priv.h"
#include "sc8825/dcam_hal.h"

#include "../SprdHWLayer.h"
#include "SprdFrameBufferHAL.h"
#include "SprdPrimaryDisplayDevice.h"
#include "../SprdUtil.h"


using namespace android;

class SprdPrimaryDisplayDevice;

/*
 *  Mainly responsible for traversaling HWLayer list,
 *  find layers that meet SprdDisplayPlane specification
 *  and then mark them as HWC_OVERLAY.
 * */
class SprdHWLayerList
{
public:
    SprdHWLayerList(FrameBufferInfo* fbInfo)
        : mFBInfo(fbInfo),
          mLayerList(0),
          mOSDLayerList(0),
          mVideoLayerList(0),
          mFBTargetLayer(0),
          mLayerCount(0),
          mRGBLayerCount(0), mYUVLayerCount(0),
          mOSDLayerCount(0), mVideoLayerCount(0),
          mFBLayerCount(0),
          mRGBLayerFullScreenFlag(false),
          mList(NULL),
          mAcceleratorMode(ACCELERATOR_NON),
          mDisableHWCFlag(false),
          mSkipLayerFlag(false),
          mPData(NULL),
	  mGlobalProtectedFlag(false),
          mDebugFlag(0), mDumpFlag(0)
    {
    }
    ~SprdHWLayerList();

    /*
     *  traversal HWLayer list
     *  and change some geometry.
     * */
    int updateGeometry(hwc_display_contents_1_t *list, int accelerator);

    /*
     *  traversal HWLayer list again,
     *  mainly judge whether upper layer and bottom layer
     *  is consistent with SprdDisplayPlane Hardware requirements.
     * */
    int revisitGeometry(bool mGspSupport, int *DisplayFlag, SprdPrimaryDisplayDevice *mPrimary);

    int checkHWLayerList(hwc_display_contents_1_t* list);

    inline SprdHWLayer *getSprdLayer(unsigned int index)
    {
        return &(mLayerList[index]);
    }

    inline SprdHWLayer **getSprdOSDLayerList()
    {
        return mOSDLayerList;
    }

    inline int getOSDLayerCount()
    {
        return mOSDLayerCount;
    }

    inline SprdHWLayer **getSprdVideoLayerList()
    {
        return mVideoLayerList;
    }

    inline int getVideoLayerCount()
    {
        return mVideoLayerCount;
    }

    inline int getYuvLayerCount()
    {
        return mYUVLayerCount;
    }

    inline hwc_layer_1_t *getFBTargetLayer()
    {
        return mFBTargetLayer;
    }

    inline unsigned int getSprdLayerCount()
    {
        return mLayerCount;
    }

    inline unsigned int getFBLayerCount()
    {
        return mFBLayerCount;
    }

#ifdef PROCESS_VIDEO_USE_GSP
    inline void transforGXPCapParameters(GSP_CAPABILITY_T *GXPCap)
    {
        mPData = static_cast<void *>(GXPCap);
    }
#endif
private:
    FrameBufferInfo* mFBInfo;
    SprdHWLayer *mLayerList;
    SprdHWLayer **mOSDLayerList;
    SprdHWLayer **mVideoLayerList;
    hwc_layer_1_t *mFBTargetLayer;
    unsigned int mLayerCount;
    unsigned int mRGBLayerCount;
    unsigned int mYUVLayerCount;
    int mOSDLayerCount;
    int mVideoLayerCount;
    int mFBLayerCount;
    bool mRGBLayerFullScreenFlag;
    hwc_display_contents_1_t *mList;
    int mAcceleratorMode;
    bool mDisableHWCFlag;
    bool mSkipLayerFlag;
    void *mPData;
    uint32_t mPrivateFlag[2];
    bool mGlobalProtectedFlag;
    int mDebugFlag;
    int mDumpFlag;

    /*
     *  Filter OSD layer
     * */
    int prepareOSDLayer(SprdHWLayer *l);

    /*
     *  Filter video layer
     * */
    int prepareVideoLayer(SprdHWLayer *l);

    /*
     *  Prepare for GSP/GPP HW device.
     *  return value:
     *      0: use GSP/GPP to accelerate.
     *      1: use OverlayComposer to accelerate.
     *      -1: cannot find available accelerator.
     * */
    int prepareForGXP(SprdHWLayer *l);

//#ifdef OVERLAY_COMPOSER_GPU
    int prepareOverlayComposerLayer(SprdHWLayer *l);

    int revisitOverlayComposerLayer(SprdHWLayer *YUVLayer, SprdHWLayer *RGBLayer,
                            int LayerCount, int *FBLayerCount, int *DisplayFlag);
//#endif

#ifdef TRANSFORM_USE_DCAM
    int DCAMTransformPrepare(hwc_layer_1_t *layer, struct sprdRect *srcRect, struct sprdRect *FBRect);
#endif

    bool IsHWCLayer(hwc_layer_1_t *AndroidLayer);

    /*
     * set a HW layer as Overlay flag.
     * */
    void setOverlayFlag(SprdHWLayer *l, unsigned int index);

    /*
     *  reset a HW layer as normal framebuffer flag
     * */
    void resetOverlayFlag(SprdHWLayer *l);

    /*
     *  Force to set a layer to Overlay flag.
     * */
    void forceOverlay(SprdHWLayer *l);

    /*
     *  Clear framebuffer content to black color.
     * */
    void ClearFrameBuffer(hwc_layer_1_t *l, unsigned int index);

    void HWCLayerPreCheck();

    void dump_layer(hwc_layer_1_t const* l);
    void dump_yuv(uint8_t* pBuffer, uint32_t aInBufSize);

    inline int MIN(int x, int y)
    {
        return ((x < y) ? x : y);
    }

    inline int MAX(int x, int y)
    {
        return ((x > y) ? x : y);
    }
};
#endif
