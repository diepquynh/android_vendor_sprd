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
 ** File: SprdVsyncEvent.h            DESCRIPTION                             *
 **                                   Handle vsync event, receive vsync event *
 **                                   and send it to EventThread for GUI.     *
 ******************************************************************************
 ******************************************************************************
 *****************************************************************************/

#ifndef _SPRD_HW_VSYNC_H_
#define _SPRD_HW_VSYNC_H_
#include <sys/types.h>

#include <utils/threads.h>
#include <hardware/hwcomposer.h>

namespace android
{

class SprdVsyncEvent: public Thread
{
    const hwc_procs_t *mProcs;
    mutable Mutex mLock;
    Condition mCondition;
    bool mEnabled;
    virtual void onFirstRef();
    virtual bool threadLoop();
    int getVSyncPeriod();
    int mFbFd;
    nsecs_t mVSyncPeriod;
public:
    SprdVsyncEvent();
    ~SprdVsyncEvent();
    void setEnabled(bool enabled);
    void setVsyncEventProcs(const hwc_procs_t *procs);
};

}
#endif
