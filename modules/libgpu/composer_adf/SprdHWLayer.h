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
 ** File: SprdHWLayer.h               DESCRIPTION                             *
 **                                   Mainly responsible for filtering HWLayer*
 **                                   list, find layers that meet OverlayPlane*
 **                                   and PrimaryPlane specifications and then*
 **                                   mark them as HWC_OVERLAY.               *
 ******************************************************************************
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/


#ifndef _SPRD_HWLAYER_H_
#define _SPRD_HWLAYER_H_

#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>
#include <utils/RefBase.h>
#include <cutils/atomic.h>
#include <cutils/log.h>
#include "gralloc_priv.h"
#include "dump.h"

#include "SprdPrimaryDisplayDevice/SprdFrameBufferHAL.h"

using namespace android;

/*
 *  YUV format layer info.
 * */
struct sprdYUV {
    uint32_t w;
    uint32_t h;
    uint32_t format;
    uint32_t y_addr;
    uint32_t u_addr;
    uint32_t v_addr;
};

/*
 *  Available layer rectangle.
 * */
struct sprdRect {
    uint32_t x;
    uint32_t y;
    uint32_t w;
    uint32_t h;
};


struct sprdPoint {
    uint32_t x;
    uint32_t y;
};

enum layerType {
    LAYER_OSD = 1,// means rgb format, should bind to OSD layer of dispc
    LAYER_OVERLAY,// means yuv format, should bind to IMG layer of dispc
    LAYER_INVALIDE
};



/*
 *  SprdHWLayer wrapped the hwc_layer_1_t which come from SF.
 *  SprdHWLayer is a local layer abstract, include src-rect,
 *  dst-rect, overlay buffer layer's handle,etc.
 * */
class SprdHWLayer
{
public:
    /*
     *  SprdHWLayer
     *  default constractor used to wrap src layer.
     * */
    SprdHWLayer()
        : mInit(false),
          mAndroidLayer(0), mLayerType(LAYER_INVALIDE),
          mPrivateH(NULL),
          mFormat(-1),
          mLayerIndex(-1),
          mSprdLayerIndex(-1),
          mAccelerator(-1),
          mProtectedFlag(false),
          mPlaneAlpha(255),
          mBlending(HWC_BLENDING_NONE),
          mTransform(0x0),
          mAcquireFenceFd(-1),
          mDebugFlag(0)
    {

    }

    SprdHWLayer(hwc_layer_1_t *androidLayer, int format);
    SprdHWLayer(struct private_handle_t *handle, int format, int32_t planeAlpha,
                int32_t blending, int32_t transform, int32_t fenceFd);

    ~SprdHWLayer()
    {

    }

    inline bool InitCheck()
    {
        return (mAndroidLayer->compositionType == HWC_OVERLAY);
    }

    inline int getLayerIndex()
    {
        return mLayerIndex;
    }

    inline int getSprdLayerIndex()
    {
        return mSprdLayerIndex;
    }

    inline hwc_layer_1_t *getAndroidLayer()
    {
        return mAndroidLayer;
    }

    inline enum layerType getLayerType()
    {
        return mLayerType;
    }


    inline int getLayerFormat()
    {
        return mFormat;
    }

    inline struct sprdRect *getSprdSRCRect()
    {
        return &srcRect;
    }

    inline struct sprdRect *getSprdFBRect()
    {
        return &FBRect;
    }

    inline bool checkContiguousPhysicalAddress(struct private_handle_t *privateH)
    {
        return (privateH->flags & private_handle_t::PRIV_FLAGS_USES_PHY);
    }

    inline bool checkNotSupportOverlay(struct private_handle_t *privateH)
    {
        HWC_IGNORE(privateH);
        return false;
        //return (privateH->flags & private_handle_t::PRIV_FLAGS_NOT_OVERLAY);
    }

    inline int getAccelerator()
    {
        return mAccelerator;
    }

    inline void resetAccelerator()
    {
        mAccelerator = 0;
    }

    inline void updateAndroidLayer(hwc_layer_1_t *l)
    {
        mAndroidLayer = l;
        mInit = false;
    }

    inline bool getProtectedFlag() const
    {
        return mProtectedFlag;
    }

    inline int getPlaneAlpha()
    {
        return mPlaneAlpha;
    }

    inline int32_t getBlendMode()
    {
        return  mBlending;
    }

    inline uint32_t getTransform()
    {
        return  mTransform;
    }

    inline struct private_handle_t *getBufferHandle()
    {
        return mPrivateH;
    }

    inline int getAcquireFence()
    {
        return mAcquireFenceFd;
    }

    bool checkRGBLayerFormat();
    bool checkYUVLayerFormat();

private:
    friend class SprdHWLayerList;
    friend class SprdVDLayerList;

    bool mInit;
    hwc_layer_1_t *mAndroidLayer;
    enum layerType mLayerType;// indicate this layer should bind to OSD/IMG layer of dispc
    struct private_handle_t *mPrivateH;
    int mFormat;
    int mLayerIndex;
    int mSprdLayerIndex;
    struct sprdRect srcRect;
    struct sprdRect FBRect;
    int mAccelerator;//default is ACCELERATOR_OVERLAYCOMPOSER, then check dispc&gsp can process or not.
    bool mProtectedFlag;
    int mPlaneAlpha;
    int32_t mBlending;
    int32_t mTransform;
    int32_t mAcquireFenceFd;
    int mDebugFlag;

    inline void setAndroidLayer(hwc_layer_1_t *l)
    {
        mAndroidLayer = l;
    }

    inline void setLayerIndex(unsigned int index)
    {
        mLayerIndex = index;
    }

    inline void setSprdLayerIndex(unsigned int index)
    {
        mSprdLayerIndex = index;
    }

    inline void setLayerType(enum layerType t)
    {
        mLayerType = t;
    }

    inline void setLayerFormat(int f)
    {
        mFormat = f;
    }

    inline void setLayerAccelerator(int flag)
    {
        mAccelerator = flag;
    }

    inline void setProtectedFlag(bool flag)
    {
        mProtectedFlag = flag;
    }

    inline void setPlaneAlpha(int32_t alpha)
    {
        mPlaneAlpha = alpha;
    }

    inline void setBlending(int32_t blend)
    {
        mBlending = blend;
    }

    inline void setTransform(int32_t transform)
    {
        mTransform = transform;
    }

    inline void setAcquireFenceFd(int fd)
    {
        mAcquireFenceFd = fd;
    }
};

#endif
