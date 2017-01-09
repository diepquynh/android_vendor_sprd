/*
 * Copyright (C) 2009 The Android Open Source Project
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

#ifndef ANDROID_INCLUDE_APM_SPRD_H
#define ANDROID_INCLUDE_APM_SPRD_H

#include <stdint.h>
#include <sys/types.h>
#include <utils/Timers.h>
#include <utils/Errors.h>
#include <utils/KeyedVector.h>
#include <hardware_legacy/AudioPolicyManagerBase.h>

using namespace android;

namespace android_audio_legacy {

class AudioPolicyManagerSPRD : public AudioPolicyManagerBase
{
public:
    AudioPolicyManagerSPRD(AudioPolicyClientInterface *clientInterface);

    virtual ~AudioPolicyManagerSPRD();
    status_t startOutput(audio_io_handle_t output,
                                             AudioSystem::stream_type stream,
                                             int session);
    status_t stopOutput(audio_io_handle_t output,
                                            AudioSystem::stream_type stream,
                                            int session);
    void releaseOutput(audio_io_handle_t output);

private:
    int is_voip_set;
    void handleNotificationRoutingForStream(AudioSystem::stream_type stream);
};

};

#endif // ANDROID_INCLUDE_APM_SPRD_H

