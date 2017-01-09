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
    SprdHWLayerList()
        : mFBInfo(NULL),
          mLayerList(0),
          mOSDLayerList(0), mVideoLayerList(0),
          mGXPLayerList(0),
          mFBTargetLayer(0),
          mAccerlator(NULL),
          mLayerCount(0),
          mOSDLayerCount(0), mVideoLayerCount(0),
          mDispCLayerCount(0), mGXPLayerCount(0),
          mYUVLayerCount(0),
          mFBLayerCount(0),
          mList(NULL),
          mAcceleratorMode(ACCELERATOR_NON),
          mGXPSupport(false),
          mDisableHWCFlag(false),
          mSkipLayerFlag(false),
          mGlobalProtectedFlag(false),
          mForceDisableHWC(false),
          mDebugFlag(0), mDumpFlag(0)
    {
#ifdef FORCE_DISABLE_HWC_OVERLAY
        mForceDisableHWC = true;
#else
        mForceDisableHWC = false;
#endif
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
    int revisitGeometry(int& DisplayFlag, SprdPrimaryDisplayDevice *mPrimary);

    inline void updateFBInfo(FrameBufferInfo* fbInfo)
    {
        mFBInfo = fbInfo;
    }

    int checkHWLayerList(hwc_display_contents_1_t* list);

    inline void setAccerlator(SprdUtil *acc)
    {
        mAccerlator = acc;
    }

    inline SprdHWLayer *getSprdLayer(unsigned int index)
    {
        return &(mLayerList[index]);
    }

    inline SprdHWLayer **getOSDLayerList()
    {
        return mOSDLayerList;
    }

    inline SprdHWLayer **getVideoLayerList()
    {
        return mVideoLayerList;
    }

    inline SprdHWLayer **getSprdGXPLayerList()
    {
        return mGXPLayerList;
    }

    inline int getGXPLayerCount()
    {
        return mGXPLayerCount;
    }

    inline int getOSDLayerCount()
    {
        return mOSDLayerCount;
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

    inline bool& getDisableHWCFlag()
    {
        return mDisableHWCFlag;
    }

private:
    FrameBufferInfo* mFBInfo;
    SprdHWLayer *mLayerList;
    SprdHWLayer **mOSDLayerList;
    SprdHWLayer **mVideoLayerList;
    SprdHWLayer **mGXPLayerList;
    /*
     *  mFBTargetLayer:it's the dst buffer, but in sprd hwc,
     *  we have independant overlay buffer, so just leave it alone.
     * */
    hwc_layer_1_t *mFBTargetLayer;
    SprdUtil    *mAccerlator;
    /*
     *  mLayerCount:total layer cnt of this composition, including fb target layer.
     * */
    unsigned int mLayerCount;
    unsigned int mOSDLayerCount;
    unsigned int mVideoLayerCount;
    unsigned int mDispCLayerCount;
    unsigned int mGXPLayerCount;
    unsigned int mYUVLayerCount;
    /*
     *  mFBLayerCount:layer cnt that should be composited by GPU in SF.
     * */
    unsigned int mFBLayerCount;
    hwc_display_contents_1_t *mList;
    /*
     *  mAcceleratorMode:available accelerator.
     * */
    int mAcceleratorMode;
    bool mGXPSupport;
    bool mDisableHWCFlag;
    bool mSkipLayerFlag;
    uint32_t mPrivateFlag[2];
    bool mGlobalProtectedFlag;
    bool mForceDisableHWC;
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
     *  Prepare for Display Controller.
     *  return value:
     *      0: use DispC to accelerate.
     *      1: use OverlayComposer to accelerate.
     *      -1: cannot find available accelerator.
     * */
    int prepareForDispC(SprdHWLayer *l);

    int prepareOverlayComposerLayer(SprdHWLayer *l);

    int revisitOverlayComposerLayer(int& DisplayFlag);

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
