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
 ** 16/08/2013    Hardware Composer   Add a new feature to Harware composer,  *
 **                                   verlayComposer use GPU to do the        *
 **                                   Hardware layer blending on Overlay      *
 **                                   buffer, and then post the OVerlay       *
 **                                   buffer to Display                       *
 ******************************************************************************
 ** Author:         fushou.yang@spreadtrum.com                                *
 **                 zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/


#ifndef _OVERLAY_NATIVEWINDOW_H_
#define _OVERLAY_NATIVEWINDOW_H_

#include <ui/ANativeObjectBase.h>
#include <ui/Fence.h>
#include "gralloc_priv.h"
#include "Utility.h"

#include "../SprdPrimaryDisplayDevice/SprdPrimaryPlane.h"
#include "../SprdPrimaryDisplayDevice/SprdFrameBufferHAL.h"

class SprdPrimaryPlane;

namespace android {
// ----------------------------------------------------------------------------

//class Surface;
class NativeBuffer
    : public ANativeObjectBase<
        ANativeWindowBuffer,
        NativeBuffer,
        LightRefBase<NativeBuffer> >
{
public:
    NativeBuffer(int w, int h, int f, int u) : BASE() {
        ANativeWindowBuffer::width  = w;
        ANativeWindowBuffer::height = h;
        ANativeWindowBuffer::format = f;
        ANativeWindowBuffer::usage  = u;
    }
private:
    friend class LightRefBase<NativeBuffer>;
    ~NativeBuffer() { }; // this class cannot be overloaded
};


#define NUM_FRAME_BUFFERS  2


class OverlayNativeWindow   //: public overlayNativeWindow
    : public ANativeObjectBase<
        ANativeWindow,
        OverlayNativeWindow,
        LightRefBase<OverlayNativeWindow> >
{
public:
    OverlayNativeWindow(SprdPrimaryPlane *displayPlane);
    ~OverlayNativeWindow();

    bool Init();

    int releaseNativeBuffer();

    void notifyDirtyTarget(bool flag);

private:
    SprdPrimaryPlane *mDisplayPlane;
    unsigned int mWidth;
    unsigned int mHeight;
    int mFormat;
    int mWindowUsage;
    int32_t mNumBuffers;
    int32_t mNumFreeBuffers;
    int32_t mBufferHead;
    int32_t mInvalidateCount;
    sp<NativeBuffer> buffers[NUM_FRAME_BUFFERS];
    sp<NativeBuffer> front;

    mutable Mutex mutex;
    Condition mCondition;
    int32_t mCurrentBufferIndex;
    bool mUpdateOnDemand;
    bool mDirtyTargetFlag;


    friend class LightRefBase<OverlayNativeWindow>;

    static int setSwapInterval(ANativeWindow* window, int interval);
    static int cancelBuffer(ANativeWindow* window, ANativeWindowBuffer* buffer, int fenceFd);
    static int dequeueBuffer(ANativeWindow* window, ANativeWindowBuffer** buffer, int* fenceFd);
    //static int lockBuffer(ANativeWindow* window, ANativeWindowBuffer* buffer);
    static int queueBuffer(ANativeWindow* window, ANativeWindowBuffer* buffer, int fenceFd);
    static int query(const ANativeWindow* window, int what, int* value);
    static int perform(ANativeWindow* window, int operation, ...);

    static sp<NativeBuffer> CreateGraphicBuffer(private_handle_t* buffer);

    inline unsigned int round_up_to_page_size(unsigned int x)
    {
         return (x + (PAGE_SIZE-1)) & ~(PAGE_SIZE-1);
    }

};
}; //android space

#endif
