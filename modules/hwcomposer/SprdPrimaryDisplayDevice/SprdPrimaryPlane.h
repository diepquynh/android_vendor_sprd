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


#ifndef _SPRD_PRIMARY_PLANE_H_
#define _SPRD_PRIMARY_PLANE_H_

#include <cutils/log.h>
#include "SprdDisplayPlane.h"
#include "../SprdHWLayer.h"
#include "SprdFrameBufferHAL.h"

#ifdef BORROW_PRIMARYPLANE_BUFFER
#include "SprdOverlayPlane.h"
class SprdOverlayPlane;
#endif


using namespace android;


class SprdPrimaryPlane: public SprdDisplayPlane
{
public:
    SprdPrimaryPlane(FrameBufferInfo *fbInfo);
    virtual ~SprdPrimaryPlane();

    /*
     *  These interfaces are from the father class SprdDisplayPlane.
     *  dequeueBuffer: gain a available buffer for SprdPrimaryPlane.
     *  queueBuffer: display a buffer.
     * */
    virtual private_handle_t* dequeueBuffer();
    virtual int queueBuffer();
    virtual private_handle_t* getPlaneBuffer();
    virtual void getPlaneGeometry(unsigned int *width, unsigned int *height, int *format);
    /**************************************************************************************/

    virtual bool open();
    virtual bool close();

    /*
     *  Bind OSD layer to SprdPrimaryPlane.
     * */
    void AttachPrimaryLayer(SprdHWLayer *l, bool DirectDisplayFlag);

    /*
     *  Attach HWC_FRAMEBUFFER_TARGET layer to SprdPrimaryPlane.
     * */
    void AttachFramebufferTargetLayer(hwc_layer_1_t *FBTargetLayer);

    /*
     *  Finally display Overlay plane and Primary plane buffer.
     * */
    void display(bool DisplayOverlayPlane, bool DisplayPrimaryPlane, bool DisplayFBTarget);

    /*
     * Check whether SprdPrimaryPlane is available.
     * */
    bool online();

    /*
     *  Force to disable SprdPrimaryPlane.
     * */
    void disable();

    /*
     *  enable SprdPrimaryPlane.
     * */
    void enable();

    /*
     *  Some HW layers use contiguous physcial address,
     *  not need to be transfomed.
     *  Here, let these layers displayed directly by SprdPrimaryPlane.
     * */
    bool SetDisplayParameters(hwc_layer_1_t *AndroidLayer);
    SprdHWLayer *getPrimaryLayer();

    int getPlaneImageFormat();
    int getPlaneBufferIndex();

    inline bool GetDirectDisplay()
    {
        return mDirectDisplayFlag;
    }

    enum PlaneFormat getPlaneFormat();

private:
    FrameBufferInfo *mFBInfo;
    SprdHWLayer *mHWLayer;
    unsigned int mPrimaryPlaneCount;
    unsigned int mFreePlaneCount;
    PlaneContext *mContext;
    private_handle_t* mBuffer;
    int mBufferIndex;
    int mDefaultDisplayFormat;
    int mDisplayFormat;
    bool mPlaneDisable;
    bool mDisplayFBTargetLayerFlag;
    bool mDirectDisplayFlag;
    unsigned char *mPlaneBufferPhyAddr;
    unsigned char *mDisplayFBTargetPhyAddr;
    unsigned char *mDirectDisplayPhyAddr;
    int mThreadID;
    int mDebugFlag;
    int mDumpFlag;

    virtual private_handle_t* flush();

    void InvalidatePlaneContext();

    int checkHWLayer(SprdHWLayer *l);

    inline unsigned int getFreePlane()
    {
        return mFreePlaneCount;
    }

    inline void addFreePlane()
    {
        mFreePlaneCount++;
    }

    inline void subFreePlane()
    {
        mFreePlaneCount--;
    }

#ifdef BORROW_PRIMARYPLANE_BUFFER
    /*
     *  Friend function for SprdOverlayPlane
     * */
    friend SprdOverlayPlane;

    private_handle_t* dequeueFriendBuffer();

    int queueFriendBuffer();

    private_handle_t* flushFriend();
#endif

};


#endif
