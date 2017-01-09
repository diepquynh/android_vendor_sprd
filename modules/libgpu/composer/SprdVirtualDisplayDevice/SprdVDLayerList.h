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
 ** File:SprdVDLayerList.h            DESCRIPTION                             *
 **                                   Responsible for traverse Virtual Display*
 **                                   Layer list and mark the layers as       *
 **                                   Overlay which                           *
 **                                   comply with Sprd Virtual Display spec.  *
 ******************************************************************************
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/


#ifndef _SPRD_VD_LAYER_LIST_H_
#define _SPRD_VD_LAYER_LIST_H_

#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>
#include <utils/RefBase.h>
#include <cutils/atomic.h>
#include <cutils/log.h>
#include "gralloc_priv.h"

#include "../SprdHWLayer.h"
#include "../dump.h"

using namespace android;

class SprdVDLayerList
{
public:
    SprdVDLayerList()
        : mLayerList(0),
          mOSDLayerList(0),
          mVideoLayerList(0),
          mFBTargetLayer(0),
          mLayerCount(0),
          mOSDLayerCount(0),
          mVideoLayerCount(0),
          mFBLayerCount(0),
          mSkipMode(false),
          mDebugFlag(0),
          mDumpFlag(0)
    {

    }
    virtual ~SprdVDLayerList();

    int updateGeometry(hwc_display_contents_1_t *list);
    int revistGeometry(hwc_display_contents_1_t *list);

    inline SprdHWLayer *getSprdLayer(unsigned int index)
    {
        return &(mLayerList[index]);
    }

    inline unsigned int getSprdLayerCount()
    {
        return mLayerCount;
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

    inline SprdHWLayer *getFBTargetLayer()
    {
        return mFBTargetLayer;
    }

    inline unsigned int getFBLayerCount()
    {
        return mFBLayerCount;
    }

    inline bool getSkipMode() const
    {
        return mSkipMode;
    }

private:
    SprdHWLayer *mLayerList;
    SprdHWLayer **mOSDLayerList;
    SprdHWLayer **mVideoLayerList;
    SprdHWLayer *mFBTargetLayer;
    unsigned int mLayerCount;
    int mOSDLayerCount;
    int mVideoLayerCount;
    int mFBLayerCount;
    bool mSkipMode;
    int mDebugFlag;
    int mDumpFlag;

    int prepareOSDLayer(SprdHWLayer *l);
    int prepareVideoLayer(SprdHWLayer *l);

    void reclaimSprdHWLayer();

    void ClearFrameBuffer(hwc_layer_1_t *l, unsigned int index) ;
    void setOverlayFlag(SprdHWLayer *l, unsigned int index);
    void forceOverlay(SprdHWLayer *l);
    void resetOverlayFlag(SprdHWLayer *l);

    void dump_layer(hwc_layer_1_t const* l);
    bool IsHWCLayer(hwc_layer_1_t *AndroidLayer);
};

#endif
