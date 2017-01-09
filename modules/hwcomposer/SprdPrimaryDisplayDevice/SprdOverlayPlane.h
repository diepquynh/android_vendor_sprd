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
 ** File: SprdOverlayPlane.h          DESCRIPTION                             *
 **                                   display YUV format Hardware layer, also *
 **                                   support RGBA format layer.              *
 ******************************************************************************
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/


#ifndef _SPRD_OVERLAY_PLANE_H_
#define _SPRD_OVERLAY_PLANE_H_

#include <cutils/log.h>
#include "SprdDisplayPlane.h"
#include "../SprdHWLayer.h"
#include "SprdFrameBufferHAL.h"

#ifdef BORROW_PRIMARYPLANE_BUFFER
#include "SprdPrimaryPlane.h"
class SprdPrimaryPlane;
#endif

using namespace android;


class SprdOverlayPlane: public SprdDisplayPlane
{
public:
    SprdOverlayPlane(FrameBufferInfo *fbInfo);
#ifdef BORROW_PRIMARYPLANE_BUFFER
    SprdOverlayPlane(FrameBufferInfo *fbInfo, SprdPrimaryPlane *PrimaryPlane);
#endif
    virtual ~SprdOverlayPlane();

    /*
     *  These interfaces are from the father class.
     *  dequeueBuffer: gain a buffer for SprdOverlayPlane;
     *  queueBuffer: display a buffer.
     * */
    virtual private_handle_t* dequeueBuffer();
    virtual int queueBuffer();
    virtual private_handle_t* getPlaneBuffer();
    /****************************************************/

    virtual bool open();
    virtual bool close();

    /*
     *  Setup a Overlay layer to SprdOverlayPlane
     * */
    void AttachOverlayLayer(SprdHWLayer *l);

    /*
     *  Check whether SprdOverlayPlane is available.
     * */
    bool online();

    /*
     *  Force to disable SprdOverlayPlane
     * */
    void disable();

    /*
     *  enable SprdOverlayPlane
     * */
    void enable();

    SprdHWLayer *getOverlayLayer();

    enum PlaneFormat getPlaneFormat();

private:
    FrameBufferInfo *mFBInfo;
#ifdef BORROW_PRIMARYPLANE_BUFFER
    SprdPrimaryPlane *mPrimaryPlane;
#endif
    SprdHWLayer *mHWLayer;
    unsigned int mOverlayPlaneCount;
    unsigned int mFreePlaneCount;
    PlaneContext *mContext;
    private_handle_t* mBuffer;
    int mDisplayFormat;
    bool mPlaneDisable;
    int mDebugFlag;
    int mDumpFlag;

    /*
     * father classe SprdDisplayPlane interface.
     * flush: update the plane registers.
     * */
    virtual private_handle_t* flush();
    //virtual void display();
    /********************************************/

    /*
     *  reset the displayplane context.
     * */
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
};



#endif
