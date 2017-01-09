
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
 ** File: SprdDisplayPlane.cpp        DESCRIPTION                             *
 **                                   Abstract class, father class of         *
 **                                   SprdPrimaryPlane and SprdOverlayPlane,  *
 **                                   provide some public methods and         *
 **                                   interface.                              *
 ******************************************************************************
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/

#include <ui/GraphicBufferAllocator.h>
#include "SprdDisplayPlane.h"

using namespace android;

SprdDisplayPlane::SprdDisplayPlane()
    : mWidth(1), mHeight(1), mFormat(-1),
      InitFlag(false), mContext(NULL),
      mBufferCount(PLANE_BUFFER_NUMBER),
      mPlaneUsage(GRALLOC_USAGE_OVERLAY_BUFFER),
      mDisplayBufferIndex(-1),
      mFlushingBufferIndex(-1),
      mPlaneRunThreshold(100),
      mPlaneIdleCount(0),
      mWaitingBuffer(false),
#ifdef DYNAMIC_RELEASE_PLANEBUFFER
      mAlloc(NULL),
#endif
      mDebugFlag(0)
{
    mContext = (PlaneContext *)malloc(sizeof(PlaneContext));
    if (mContext == NULL)
    {
        ALOGE("Failed to malloc overlay_setting");
        exit(-1);
    }

#ifdef DYNAMIC_RELEASE_PLANEBUFFER
    mAlloc = new AllocHelper(this);
    if (mAlloc == NULL)
    {
        ALOGE("Failed to new AllocHelper");
        exit(-1);
    }
#endif
}

SprdDisplayPlane::~SprdDisplayPlane()
{
    if (mContext)
    {
        free(mContext);
        mContext = NULL;
    }
}

void SprdDisplayPlane::setGeometry(unsigned int width, unsigned int height, int format)
{
    mWidth = width;
    mHeight = height;
    mFormat = format;
}

private_handle_t* SprdDisplayPlane:: createPlaneBuffer(int index)
{
    private_handle_t* BufHandle = NULL;
    uint32_t stride;
    size_t size;

    if (index < 0)
    {
        ALOGE("plane buffer index < 0");
        return NULL;
    }

    GraphicBufferAllocator::get().alloc(mWidth, mHeight, mFormat, mPlaneUsage, (buffer_handle_t*)&BufHandle, &stride);
    if (BufHandle == NULL)
    {
        ALOGE("SprdDisplayPlane cannot alloc buffer");
        return NULL;
    }

    MemoryHeapIon::Get_phy_addr_from_ion(BufHandle->share_fd, &(BufHandle->phyaddr), &size);

    mSlots[index].mIonBuffer = static_cast<private_handle_t* >(BufHandle);

    ALOGI("DisplayPlane createPlaneBuffer phy addr:%p, size:%zd",
          (void *)(BufHandle->phyaddr), size);

    return BufHandle;
}

private_handle_t* SprdDisplayPlane::dequeueBuffer()
{
    bool repeat = true;
    int found = -1;
    mWaitingBuffer = false;

    queryDebugFlag(&mDebugFlag);

    Mutex::Autolock _l(mLock);
    do
    {
        for(int i = 0; i < mBufferCount; i++)
        {
            const int state = mSlots[i].mBufferState;

            if (state == BufferSlot::FREE)
            {
                found = i;
                break;
            }
        }

        if (found < 0)
        {
            /*
             *  Wait availeable buffer
             **/
            mWaitingBuffer = true;
            nsecs_t timeout = ms2ns(3000);
            if (mCondition.waitRelative(mLock, timeout) == TIMED_OUT)
            {
                ALOGE("SprdDisplayPlane::dequeueBuffer has waited 3s, give up");
                repeat = false;
            }
        }
        else
        {
            break;
        }

    } while (repeat);

    if (found < 0)
    {
        ALOGE("get invalid buffer index");
        return NULL;
    }

    mSlots[found].mBufferState = BufferSlot::DEQUEUEED;
    private_handle_t* buffer = mSlots[found].mIonBuffer;

    if (buffer == NULL && mWidth > 1 && mHeight > 1)
    {
        buffer = createPlaneBuffer(found);
    }

    if (buffer == NULL) {
        ALOGE("buffer is NULL");
    }

    /*
     *  eglMakeCurrent will call dequeueBuffer first.
     *  If GPU do not use this buffer, just release the
     *  buffer status.
     *  It is a workaround method to make dequeueBuffer
     *  and queueBuffer invocation in serial way.
     * */
    static int dequeueFirstFlag = 0;
    if (dequeueFirstFlag == 0 && mDisplayBufferIndex >= 0)
    {
        mSlots[mDisplayBufferIndex].mBufferState = BufferSlot::FREE;
        dequeueFirstFlag = 1;
    }

    mDisplayBufferIndex = found;

    return (mSlots[found].mIonBuffer);
}

int SprdDisplayPlane::queueBuffer()
{
    int bufferIndex = mDisplayBufferIndex;

    Mutex::Autolock _l(mLock);
    mSlots[bufferIndex].mBufferState = BufferSlot::QUEUEED;

    mQueue.push_back(bufferIndex);

    return 0;
}

private_handle_t* SprdDisplayPlane::flush()
{
    if (mQueue.empty())
    {
        ALOGE("SprdDisplayPlane::flush no avaialbe buffer for flushing");
        return NULL;
    }

    FIFO::iterator front(mQueue.begin());
    int index(*front);
    private_handle_t* flushingBuffer = mSlots[index].mIonBuffer;

    mQueue.erase(front);

    /*
     *  Two tasks need to be done.
     *
     *  1. The avaiable buffer has been found, restore the prevous flushing
     *     buffer state to BufferSlot::FREE.
     * */
    /*
     *  2. Restore the available buffer status from the error status.
     *     Sometimes, display plane dequeueBuffer and queueBuffer can
     *     not been invocated in serial way, this will result in a buffer
     *     can not be used.
     *     So here, check the buffer status in error.
     * */
    if (mFlushingBufferIndex == index)
    {
        ALOGE("SprdDisplayPlane buffer: %d has been in error status", index);

        for (int i = 0; i < mBufferCount; i++)
        {
            if (index == i)
            {
                continue;
            }

            mSlots[i].mBufferState = BufferSlot::FREE;
        }
    }
    else if (mFlushingBufferIndex >= 0)
    {
        mSlots[mFlushingBufferIndex].mBufferState = BufferSlot::FREE;
    }

    /*
     *  Update the flushing buffer index
     * */
    mFlushingBufferIndex = index;

    if (mWaitingBuffer)
    {
        mCondition.broadcast();
    }

    return flushingBuffer;
}

//bool SprdDisplayPlane::display()
//{
//    return true;
//}

bool SprdDisplayPlane:: openBase()
{
    bool success = false;
    private_handle_t* buffer = NULL;

    for (int i = 0; i < mBufferCount; i++)
    {
        buffer = mSlots[i].mIonBuffer;
        mSlots[i].mBufferState = BufferSlot::FREE;
        if (buffer)
        {
            success = true;
            continue;
        }
        buffer = createPlaneBuffer(i);
        if (buffer == NULL)
        {
            ALOGE("SprdDisplayPlane::open createPlaneBuffer failed");
            success = false;
            return false;
        }

        success = true;
    }

    return success;
}

bool SprdDisplayPlane::open()
{
    private_handle_t* buffer = NULL;
#ifdef DYNAMIC_RELEASE_PLANEBUFFER
    int ret = -1;
    ret = mAlloc->requestAllocBuffer();
    if (ret < 0)
    {
        ALOGE("SprdDisplayPlane::open requestAllocBuffer failed");
        return false;
    }
#else
    bool retValue = false;
    retValue = openBase();
    if (retValue == false)
    {
        ALOGE("SprdDisplayPlane openBase failed");
        return false;
    }
#endif

    InitFlag = true;

    return true;
}

bool SprdDisplayPlane::close()
{
    queryDebugFlag(&mDebugFlag);

    for (int i = 0; i < mBufferCount; i++)
    {
        private_handle_t* bufferHandle = mSlots[i].mIonBuffer;
        ALOGI_IF(mDebugFlag, "SprdDisplayPlane::close free buffer phy: %p", (void *)(bufferHandle->phyaddr));
        GraphicBufferAllocator::get().free((buffer_handle_t)bufferHandle);
        bufferHandle = NULL;
        mSlots[i].mIonBuffer = NULL;
        mSlots[i].mBufferState = BufferSlot::RELEASE;
    }

    mFlushingBufferIndex = -1;

    InitFlag = false;

    return true;
}

private_handle_t* SprdDisplayPlane::getPlaneBuffer()
{
    return NULL;
}

void SprdDisplayPlane::getPlaneGeometry(unsigned int *width, unsigned int *height, int *format)
{

}

enum PlaneRunStatus SprdDisplayPlane:: queryPlaneRunStatus()
{
    enum PlaneRunStatus status = PLANE_STATUS_INVALID;

    if (InitCheck() == true)
    {
        status = PLANE_OPENED;
    }
    else
    {
        status = PLANE_CLOSED;
    }

    if (status == PLANE_OPENED)
    {
        bool flag = ((mPlaneIdleCount < mPlaneRunThreshold) ?  true : false);
        if (flag == false)
        {
            status = PLANE_SHOULD_CLOSED;
        }
    }

    return status;
}

#ifdef DYNAMIC_RELEASE_PLANEBUFFER
void AllocHelper:: onFirstRef()
{
    run("DisplayAllocHelper", PRIORITY_URGENT_DISPLAY + PRIORITY_MORE_FAVORABLE);
}

status_t AllocHelper:: readyToRun()
{
    if (mPlane == NULL)
    {
        ALOGE("AllocHelper:: readyToRun check parameters error");
        return -1;
    }

    sem_init(&doneSem, 0, 0);

    return NO_ERROR;
}

bool AllocHelper:: threadLoop()
{
    {
        Mutex::Autolock _l(mLock);
        mCondition.wait(mLock);
    }

    bool success = false;
    mStopFlag = false;

    do {
        success = mPlane->openBase();
        if (success || mStopFlag)
        {
            break;
        }
    } while (1);

    if (mStopFlag == false)
    {
        mAllocSuccess = success;

        sem_post(&doneSem);
    }

    return true;
}

int AllocHelper:: requestAllocBuffer()
{
    int ret = -1;

    {
        Mutex::Autolock _l(mLock);
        mCondition.broadcast();
    }

    unsigned int timeout = 1;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeout;
    //ts.tv_nsec += 0;
    if ((unsigned int)(ts.tv_nsec) >= TIME_SPEC_NSEC_MAX_VALUE)
    {
        ts.tv_sec += 1;
        ts.tv_nsec -= TIME_SPEC_NSEC_MAX_VALUE;
    }

    ret = sem_timedwait(&doneSem, &ts);

    if (ret == -1)
    {
        ALOGE("Wait alloc buffer timedwait error: %d", errno);
        mStopFlag = true;
        return ret;
    }

    if (mAllocSuccess)
    {
        ret = 0;
    }
    else
    {
        ret = -1;
        ALOGE("AllocHelper:: requestAllocBuffer failed");
    }

    return ret;
}
#endif
