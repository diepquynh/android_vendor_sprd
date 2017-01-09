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



#include <hardware/hwcomposer.h>
#include <hardware/hardware.h>
#include <sys/ioctl.h>
#include "sprd_fb.h"
#include "SyncThread.h"

#include "OverlayNativeWindow.h"
#include "dump.h"


namespace android {

static int mDebugFlag = 0;

OverlayNativeWindow::OverlayNativeWindow(SprdPrimaryPlane *displayPlane)
    : mDisplayPlane(displayPlane),
      mWidth(1), mHeight(1), mFormat(-1),
      mWindowUsage(-1),
      mNumBuffers(NUM_FRAME_BUFFERS),
      mNumFreeBuffers(NUM_FRAME_BUFFERS), mBufferHead(0),
      mInvalidateCount(NUM_FRAME_BUFFERS),
      mCurrentBufferIndex(0),
      mUpdateOnDemand(false),
      mDirtyTargetFlag(false)
{

}

bool OverlayNativeWindow::Init()
{
    mDisplayPlane->getPlaneGeometry(&mWidth, &mHeight, &mFormat);
    mWindowUsage = mDisplayPlane->getPlaneUsage();

    ANativeWindow::setSwapInterval = setSwapInterval;
    ANativeWindow::cancelBuffer    = cancelBuffer;
    ANativeWindow::dequeueBuffer   = dequeueBuffer;
    ANativeWindow::queueBuffer     = queueBuffer;
    ANativeWindow::query           = query;
    ANativeWindow::perform         = perform;

    const_cast<int&>(ANativeWindow::minSwapInterval) = 0;
    const_cast<int&>(ANativeWindow::maxSwapInterval) = 1;

    return true;
}

OverlayNativeWindow::~OverlayNativeWindow()
{
}

int OverlayNativeWindow:: releaseNativeBuffer()
{
    for (int i = 0; i < NUM_FRAME_BUFFERS; i++)
    {
        if (buffers[i] != NULL)
        {
            buffers[i] = 0;
        }
    }

    return 0;
}

void OverlayNativeWindow:: notifyDirtyTarget(bool flag)
{
    OverlayNativeWindow* self = getSelf(this);
    if (!(self->mDirtyTargetFlag))
    {
        Mutex::Autolock _l(self->mutex);
        self->mDirtyTargetFlag = true;
        self->mInvalidateCount = mNumBuffers - 1;
    }
}

sp<NativeBuffer> OverlayNativeWindow::CreateGraphicBuffer(private_handle_t* buffer)
{
    sp<NativeBuffer> nativeBuffer = NULL;

    nativeBuffer = new NativeBuffer(buffer->width,
                                    buffer->height,
                                    buffer->format, GRALLOC_USAGE_HW_FB);
    nativeBuffer->handle = buffer;
    nativeBuffer->stride = buffer->width;

    return nativeBuffer;
}


int OverlayNativeWindow::dequeueBuffer(ANativeWindow* window,
        ANativeWindowBuffer** buffer, int* fenceFd)
{
    OverlayNativeWindow* self = getSelf(window);
    Mutex::Autolock _l(self->mutex);
    int index = -1;

    // wait for a free buffer
    while (!self->mNumFreeBuffers) {
        self->mCondition.wait(self->mutex);
    }

    private_handle_t* IONBuffer = self->mDisplayPlane->dequeueBuffer();
    if (buffer == NULL)
    {
        ALOGE("Failed to get the Display plane buffer");
        return -1;
    }

    index = self->mDisplayPlane->getPlaneBufferIndex();
    if (index < 0)
    {
        ALOGE("OverlayNativeWindow get invalid buffer index");
        return -1;
    }
    // get this buffer
    self->mNumFreeBuffers--;
    self->mCurrentBufferIndex = index;

    if (self->buffers[index] == NULL)
    {
        self->buffers[index] = CreateGraphicBuffer(IONBuffer);
        if (self->buffers[index] == NULL)
        {
            ALOGE("Failed to CreateGraphicBuffer");
            return -1;
        }
    }
    *buffer = self->buffers[index].get();
#ifdef INVALIDATE_WINDOW_TARGET
    if (self->mDirtyTargetFlag)
    {
        IONBuffer->buf_idx = 0x100;
    }
#endif

    *fenceFd = -1;

    queryDebugFlag(&mDebugFlag);
    ALOGI_IF(mDebugFlag, "OverlayNativeWindow::dequeueBuffer phy addr:%p", (void *)(((private_handle_t*)(*buffer)->handle)->phyaddr));
    return 0;
}


int OverlayNativeWindow::queueBuffer(ANativeWindow* window,
        ANativeWindowBuffer* buffer, int fenceFd)
{
    private_handle_t *hnd = (private_handle_t *)buffer->handle;
    OverlayNativeWindow* self = getSelf(window);
    Mutex::Autolock _l(self->mutex);

    sp<Fence> fence(new Fence(fenceFd));
    fence->wait(Fence::TIMEOUT_NEVER);

    self->mDisplayPlane->queueBuffer();

    self->mDisplayPlane->display(false, true, false);

    postSem();

    const int index = self->mCurrentBufferIndex;
    self->front = static_cast<NativeBuffer*>(buffer);
    self->mNumFreeBuffers++;

    if ((self->mInvalidateCount)-- <= 0)
    {
        self->mDirtyTargetFlag = false;
    }
#ifdef INVALIDATE_WINDOW_TARGET
    hnd->buf_idx = 0;
#endif
    self->mCondition.broadcast();

    queryDebugFlag(&mDebugFlag);
    ALOGI_IF(mDebugFlag, "OverlayNativeWindow::queueBuffer phy addr:%p", (void *)(((private_handle_t*)buffer->handle)->phyaddr));

    return 0;
}

#if 0
int OverlayNativeWindow::lockBuffer(ANativeWindow* window,
        ANativeWindowBuffer* buffer)
{
    OverlayNativeWindow* self = getSelf(window);
    Mutex::Autolock _l(self->mutex);

    const int index = self->mCurrentBufferIndex;

    // wait that the buffer we're locking is not front anymore
    while (self->front == buffer) {
        self->mCondition.wait(self->mutex);
    }

    return 0;
}
#endif

int OverlayNativeWindow::cancelBuffer(ANativeWindow* window, ANativeWindowBuffer* buffer, int fenceFd)
{
    const OverlayNativeWindow* self = getSelf(window);
    Mutex::Autolock _l(self->mutex);

    return 0;
}

int OverlayNativeWindow::query(const ANativeWindow* window,
        int what, int* value)
{
    const OverlayNativeWindow* self = getSelf(window);
    Mutex::Autolock _l(self->mutex);


    //ALOGI("%s %d",__func__,__LINE__);

    switch (what) {
        case NATIVE_WINDOW_FORMAT:
            *value = self->mFormat;
            return NO_ERROR;
        case NATIVE_WINDOW_QUEUES_TO_WINDOW_COMPOSER:
            *value = 0;
            return NO_ERROR;
        case NATIVE_WINDOW_CONCRETE_TYPE:
            *value = NATIVE_WINDOW_FRAMEBUFFER;
            return NO_ERROR;
        case NATIVE_WINDOW_DEFAULT_WIDTH:
            *value = self->mWidth;
            return NO_ERROR;
        case NATIVE_WINDOW_DEFAULT_HEIGHT:
            *value = self->mHeight;
            return NO_ERROR;
        case NATIVE_WINDOW_TRANSFORM_HINT:
            *value = 0;
            return NO_ERROR;
        case NATIVE_WINDOW_CONSUMER_RUNNING_BEHIND:
            *value = 0;
            return NO_ERROR;
        case NATIVE_WINDOW_CONSUMER_USAGE_BITS:
            *value = GRALLOC_USAGE_HW_FB | GRALLOC_USAGE_HW_RENDER |
                     GRALLOC_USAGE_HW_COMPOSER | self->mWindowUsage;
            return NO_ERROR;
        case NATIVE_WINDOW_MIN_UNDEQUEUED_BUFFERS:
            *value = 1;
            return NO_ERROR;
    }
    return BAD_VALUE;

}

int OverlayNativeWindow::perform(ANativeWindow* window,
        int operation, ...)
{
    switch (operation) {
        case NATIVE_WINDOW_CONNECT:
        case NATIVE_WINDOW_DISCONNECT:
        case NATIVE_WINDOW_SET_USAGE:
        case NATIVE_WINDOW_SET_BUFFERS_GEOMETRY:
        case NATIVE_WINDOW_SET_BUFFERS_DIMENSIONS:
        case NATIVE_WINDOW_SET_BUFFERS_FORMAT:
        case NATIVE_WINDOW_SET_BUFFERS_TRANSFORM:
        case NATIVE_WINDOW_API_CONNECT:
        case NATIVE_WINDOW_API_DISCONNECT:
            // TODO: we should implement these
            return NO_ERROR;

        case NATIVE_WINDOW_LOCK:
        case NATIVE_WINDOW_UNLOCK_AND_POST:
        case NATIVE_WINDOW_SET_CROP:
        case NATIVE_WINDOW_SET_BUFFER_COUNT:
        case NATIVE_WINDOW_SET_BUFFERS_TIMESTAMP:
        case NATIVE_WINDOW_SET_SCALING_MODE:
            return INVALID_OPERATION;
    }
    return NAME_NOT_FOUND;
}


int OverlayNativeWindow::setSwapInterval(
        ANativeWindow* window, int interval)
{
    //hwc_composer_device_t* fb = getSelf(window)->hwc_dev;
  //  return fb->setSwapInterval(fb, interval);
    return 0;
}

};
