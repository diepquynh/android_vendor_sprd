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
 ** File: SprdVsyncEvent.cpp          DESCRIPTION                             *
 **                                   Handle vsync event, receive vsync event *
 **                                   and send it to EventThread for GUI.     *
 ******************************************************************************
 ******************************************************************************
 *****************************************************************************/

#include "SprdVsyncEvent.h"
#include <hardware/hwcomposer.h>
#include <cutils/log.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>

#include "SprdFrameBufferHAL.h"
#include "../SprdDisplayDevice.h"
#include "../SprdTrace.h"


namespace android
{
extern "C" int clock_nanosleep(clockid_t clock_id, int flags,
                           const struct timespec *request,
                           struct timespec *remain);


SprdVsyncEvent::SprdVsyncEvent()
    : mProcs(NULL), mEnabled(false)
{
    char const * const device_template[] =
    {
        "/dev/graphics/fb%u",
        "/dev/fb%u",
        NULL
    };
    int fd = -1;
    int i = 0;
    char name[64];
    while ((fd == -1) && device_template[i])
    {
        snprintf(name, 64, device_template[i], 0);
        fd = open(name, O_RDWR, 0);
        i++;
    }
    if (fd < 0)
    {
        ALOGE("fail to open fb");
    }
    mFbFd = fd;
    getVSyncPeriod();
}
SprdVsyncEvent::~SprdVsyncEvent()
{
    if (mFbFd >= 0)
        close(mFbFd);
}

void SprdVsyncEvent::onFirstRef() {
    run("SprdVSyncThread", PRIORITY_URGENT_DISPLAY + PRIORITY_MORE_FAVORABLE);
}
void SprdVsyncEvent::setEnabled(bool enabled) {
    HWC_TRACE_CALL;
    Mutex::Autolock _l(mLock);
    mEnabled = enabled;
    mCondition.signal();
}

void SprdVsyncEvent::setVsyncEventProcs(const hwc_procs_t *procs)
{
    if (procs == NULL)
    {
        ALOGE("The procs is NULL");
        return;
    }

    mProcs = procs;
}

bool SprdVsyncEvent::threadLoop() {
    { // scope for lock
        Mutex::Autolock _l(mLock);
        while (!mEnabled) {
            mCondition.wait(mLock);
        }
    }

    //8810 use sleep mode
#ifdef _VSYNC_USE_SOFT_TIMER
    static nsecs_t netxfakevsync = 0;
    const nsecs_t period = mVSyncPeriod;;
    const nsecs_t now = systemTime(CLOCK_MONOTONIC);
    nsecs_t next_vsync = netxfakevsync;
    nsecs_t sleep = next_vsync - now;
    if (sleep < 0) {
        // we missed, find where the next vsync should be
        sleep = (period - ((now - next_vsync) % period));
        next_vsync = now + sleep;
    }
    netxfakevsync = next_vsync + period;

    struct timespec spec;
    spec.tv_sec  = next_vsync / 1000000000;
    spec.tv_nsec = next_vsync % 1000000000;

    int err;
    do {
        err = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &spec, NULL);
    } while (err<0 && errno == EINTR);

    if (err == 0) {
    if(!mProcs || !mProcs->vsync)
    {
        return true;
    }
        mProcs->vsync(mProcs, DISPLAY_PRIMARY, next_vsync);
    }
#else //8825 use driver vsync mode now use sleep for temporaryly
#ifndef USE_FB_HW_VSYNC
    static nsecs_t netxfakevsync = 0;
    const nsecs_t period = mVSyncPeriod;
    const nsecs_t now = systemTime(CLOCK_MONOTONIC);
    nsecs_t next_vsync = netxfakevsync;
    nsecs_t sleep = next_vsync - now;
    if (sleep < 0) {
        // we missed, find where the next vsync should be
        sleep = (period - ((now - next_vsync) % period));
        next_vsync = now + sleep;
    }
    netxfakevsync = next_vsync + period;

    struct timespec spec;
    spec.tv_sec  = next_vsync / 1000000000;
    spec.tv_nsec = next_vsync % 1000000000;

    int err;
    do {
        err = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &spec, NULL);
    } while (err<0 && errno == EINTR);

    if (err == 0)
    {
        if(!mProcs || !mProcs->vsync)
        {
            return true;
        }

        mProcs->vsync(mProcs, DISPLAY_PRIMARY, next_vsync);
    }
    //may open when driver ready
#else
    HWC_TRACE_BEGIN_VSYNC;

    if (ioctl(mFbFd, FBIO_WAITFORVSYNC, NULL) == -1)
    {
        ALOGE("fail to wait vsync , mFbFd:%d" , mFbFd);
    }
    else
    {
        if(!mProcs || !mProcs->vsync)
        {
            ALOGW("device procs or vsync is null procs:%p , vsync:%p",
                  (void *)mProcs , mProcs ? (void *)(mProcs->vsync):NULL);
            return true;
        }
        mProcs->vsync(mProcs, 0, systemTime(CLOCK_MONOTONIC));
    }

    HWC_TRACE_END;
#endif
#endif

    return true;
}
int SprdVsyncEvent::getVSyncPeriod()
{
    struct fb_var_screeninfo info;
    if (ioctl(mFbFd, FBIOGET_VSCREENINFO, &info) == -1)
    {
        return -errno;
    }

    int refreshRate = 0;
    if ( info.pixclock > 0 )
    {
        refreshRate = 1000000000000000LLU /
            (
             uint64_t(info.upper_margin + info.lower_margin + info.yres)
             * (info.left_margin  + info.right_margin + info.xres)
                 * info.pixclock);
    }
    else
    {
        ALOGW( "fbdev pixclock is zero for fd: %d", mFbFd );
    }

    if (refreshRate == 0)
    {
        ALOGW("getVsyncPeriod refreshRate use fake rate, 60HZ");
        refreshRate = 60*1000;  // 60 Hz
    }
    float fps  = refreshRate / 1000.0f;
    mVSyncPeriod = nsecs_t(1e9 / fps);
    return 0;
}

}
