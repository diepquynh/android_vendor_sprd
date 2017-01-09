/*
 * Copyright (C) 2012 The Android Open Source Project
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
#ifndef SPRD_HAL2_SIGNAL_BASE_THREAD_H
#define SPRD_HAL2_SIGNAL_BASE_THREAD_H

#include <utils/threads.h>

namespace android {

#define SIGNAL_THREAD_TERMINATE     (1<<0)
#define SIGNAL_THREAD_PAUSE         (1<<1)

#define SIGNAL_THREAD_COMMON_LAST   (1<<3)

class SprdBaseThread:public Thread {
public:
                        SprdBaseThread();
                        SprdBaseThread(const char *name,
                            int32_t priority, size_t stack);
    virtual             ~SprdBaseThread();

            status_t    SetSignal(uint32_t signal);

            uint32_t    GetProcessingSignal();
            //void        ClearProcessingSignal(uint32_t signal);
            void        Start(const char *name,
                            int32_t priority, size_t stack);
            bool        IsTerminated();

private:
            status_t    readyToRun();
            status_t    readyToRunInternal();

            bool        threadLoop();
    virtual void        threadDealWithSiganl() = 0;

            void        ClearSignal();

            uint32_t    m_receivedSignal;
            uint32_t    m_processingSignal;
            uint32_t    m_pendingSignal;

            Mutex       m_signalMutex;
            Condition   m_threadCondition;
            bool	    m_isTerminated;
};

}; // namespace android

#endif
