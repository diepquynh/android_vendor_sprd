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
 ** File: SprdDisplayPlane.h          DESCRIPTION                             *
 **                                   Abstract class, father class of         *
 **                                   SprdPrimaryPlane and SprdOverlayPlane,  *
 **                                   provide some public methods and         *
 **                                   interface.                              *
 ******************************************************************************
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/

#ifndef _SPRD_DISPLAY_PLANE_H_
#define _SPRD_DISPLAY_PLANE_H_

#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <cutils/log.h>
#include <utils/Vector.h>
#include <utils/Condition.h>
#include <utils/Mutex.h>
#include "MemoryHeapIon.h"
#include <semaphore.h>

#ifdef DYNAMIC_RELEASE_PLANEBUFFER
#include <utils/threads.h>
#endif

#include "sprd_fb.h"
#include "gralloc_priv.h"
#include "dump.h"

using namespace android;

#define PLANE_BUFFER_NUMBER 2

/*
 *  Manage DisplayPlane buffer, including
 *  ION buffer info, buffer status.
 *  Buffer status indicates whether buffer is
 *  displayed by DisplayPlane and whether is
 *  re-write for next frame.
 * */
struct BufferSlot
{
    BufferSlot()
        : mBufferState(BufferSlot::FREE),
          mTransform(0),
          mIonBuffer(NULL)
    {

    }

    enum BufferState {
        FREE = 0,
        DEQUEUEED = 1,
        QUEUEED = 2,
        RELEASE = 4,
    };

    BufferState mBufferState;
    uint32_t mTransform;
    private_handle_t* mIonBuffer;
};

enum PlaneFormat {
    PLANE_FORMAT_RGB888 = 1,
    PLANE_FORMAT_RGB565 = 2,
    PLANE_FORMAT_YUV420 = 3,
    PLANE_FORMAT_YUV422 = 4,
    PLANE_FORMAT_NONE = 5,
};

enum PlaneRunStatus {
    PLANE_OPENED = 1,
    PLANE_CLOSED = 2,
    PLANE_SHOULD_CLOSED = 3,
    PLANE_STATUS_INVALID = 4,
};

typedef struct DisplayPlaneContext{
    struct overlay_setting BaseContext;
    //struct overlay_display_setting DisplayContext;

} PlaneContext;

#ifdef DYNAMIC_RELEASE_PLANEBUFFER
#define TIME_SPEC_NSEC_MAX_VALUE  1000000000UL
class SprdDisplayPlane;
class AllocHelper: public Thread
{
public:
    AllocHelper(SprdDisplayPlane *plane)
        : mAllocSuccess(false),
          mStopFlag(true),
          mPlane(plane)
    {

    }
    ~AllocHelper()
    {

    }

    int requestAllocBuffer();

private:
    bool mAllocSuccess;
    bool mStopFlag;
    SprdDisplayPlane *mPlane;
    mutable Mutex mLock;
    sem_t         doneSem;
    Condition mCondition;

    virtual void onFirstRef();
    virtual status_t readyToRun();
    virtual bool threadLoop();
};
#endif

/* SprdDisplayPlane is a abstract class, responsible for manage
 * display plane
 * */
class SprdDisplayPlane
{
public:
    SprdDisplayPlane();
    virtual ~SprdDisplayPlane();

    /*
     *  Gain a available buffer for SprdDisplayPlane.
     * */
    virtual private_handle_t* dequeueBuffer();

    /*
     *  Display a buffer filled with content to SprdDisplayPlane.
     * */
    virtual int queueBuffer();

    unsigned int getWidth()  { return mWidth; }
    unsigned int getHeight() { return mHeight; }

    inline int getPlaneRunThreshold()
    {
        return mPlaneRunThreshold;
    }

    inline void recordPlaneIdleCount()
    {
        mPlaneIdleCount++;
    }

    inline void resetPlaneIdleCount()
    {
        mPlaneIdleCount = 0;
    }

    enum PlaneRunStatus queryPlaneRunStatus();

    int getPlaneUsage()
    {
        return mPlaneUsage;
    }

protected:
    virtual bool open();
    virtual bool close();

    /*
     *  Update SprdDisplayPlane display registers.
     * */
    virtual private_handle_t* flush();
    //virtual bool display();

    inline PlaneContext *getPlaneContext()
    {
        return mContext;
    }

    virtual private_handle_t* getPlaneBuffer();
    virtual void getPlaneGeometry(unsigned int *width, unsigned int *height, int *format);
    inline int getPlaneBufferIndex()
    {
        return mDisplayBufferIndex;
    }

    void setGeometry(unsigned int width, unsigned int height, int format);

    inline void setPlaneRunThreshold(int threshold)
    {
        mPlaneRunThreshold = threshold;
    }

private:
    unsigned int mWidth;
    unsigned int mHeight;
    int mFormat;
    bool InitFlag;
    PlaneContext *mContext;
    int mBufferCount;
    int mPlaneUsage;
    BufferSlot mSlots[PLANE_BUFFER_NUMBER];
    int mDisplayBufferIndex;
    int mFlushingBufferIndex;
    int mPlaneRunThreshold;
    int mPlaneIdleCount;
    typedef Vector<int> FIFO;
    FIFO mQueue;
    mutable Condition mCondition;
    mutable Mutex mLock;
    bool mWaitingBuffer;
#ifdef DYNAMIC_RELEASE_PLANEBUFFER
    sp<AllocHelper> mAlloc;
#endif
    int mDebugFlag;


    inline bool InitCheck()
    {
        return InitFlag;
    }

    private_handle_t* createPlaneBuffer(int index);

    bool openBase();

#ifdef DYNAMIC_RELEASE_PLANEBUFFER
    friend AllocHelper;
#endif
};


#endif
