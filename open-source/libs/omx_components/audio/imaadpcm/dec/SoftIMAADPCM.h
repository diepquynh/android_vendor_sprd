/*
 * Copyright (C) 2011 The Android Open Source Project
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

#ifndef SOFT_IMAADPCM_H_

#define SOFT_IMAADPCM_H_

#include "SimpleSoftOMXComponent.h"

namespace android {

struct SoftIMAADPCM : public SimpleSoftOMXComponent {
    SoftIMAADPCM(const char *name,
            const OMX_CALLBACKTYPE *callbacks,
            OMX_PTR appData,
            OMX_COMPONENTTYPE **component);

protected:
    virtual ~SoftIMAADPCM();

    virtual OMX_ERRORTYPE internalGetParameter(
            OMX_INDEXTYPE index, OMX_PTR params);

    virtual OMX_ERRORTYPE internalSetParameter(
            OMX_INDEXTYPE index, const OMX_PTR params);

    virtual void onQueueFilled(OMX_U32 portIndex);

private:
    enum {
        kNumBuffers = 4,
        kMaxNumSamplesPerFrame = 131072*4
    };

    OMX_U32 mNumChannels;
    OMX_U32 mSamplingRate;
    OMX_U32 mBlockAlign;
    bool mSignalledError;

    void initPorts();

    static void DecodeIMAADPCM(int16_t *out, const uint8_t *in, int channels, size_t inSize);

    DISALLOW_EVIL_CONSTRUCTORS(SoftIMAADPCM);
};

}  // namespace android

#endif  // SOFT_IMAADPCM_H_

