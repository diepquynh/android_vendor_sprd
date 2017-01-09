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

#ifndef SPRD_APE_DECODER_H_

#define SPRD_AAC_DECODER_H_

#include "MONKEY_APIs.h"
#include "SprdSimpleOMXComponent.h"

namespace android {

#define AAC_STREAM_BUF_SIZE 1024*64+256
#define AAC_PCM_OUT_SIZE  2048

struct SPRDAPEDecoder : public SprdSimpleOMXComponent {
    SPRDAPEDecoder(const char *name,
            const OMX_CALLBACKTYPE *callbacks,
            OMX_PTR appData,
            OMX_COMPONENTTYPE **component);

protected:
    virtual ~SPRDAPEDecoder();

    virtual OMX_ERRORTYPE internalGetParameter(
            OMX_INDEXTYPE index, OMX_PTR params);

    virtual OMX_ERRORTYPE internalSetParameter(
            OMX_INDEXTYPE index, const OMX_PTR params);

    virtual void onQueueFilled(OMX_U32 portIndex);
    virtual void onPortFlushCompleted(OMX_U32 portIndex);
    virtual void onPortEnableCompleted(OMX_U32 portIndex, bool enabled);

private:
    enum {
        kNumInputBuffers        = 4,
        kNumOutputBuffers       = 4,
    };

    void *mDecoderBuf;
    uint32_t mProfile;
    uint32_t mFrameSize;
	uint32_t mOutputSize;
    int32_t mSamplingRate;
    char *mPcm_out;
    APE__StreamDecoder *mDecoder;

    void *mLibHandle;

    FT_APEDecompress_Init mAPEDecompress_Init;
    FT_APEDecompress_Dec mAPEDecompress_Dec;
    FT_APEDecompress_Term mAPEDecompress_Term;
 
    size_t mInputBufferCount;
    int64_t mAnchorTimeUs;
    int64_t mNumSamplesOutput;

    bool mSignalledError;
    enum {
        NONE,
        AWAITING_DISABLED,
        AWAITING_ENABLED
    } mOutputPortSettingsChange;

    void initPorts();
    status_t initDecoder();
    bool isConfigured() const;
    bool openDecoder(const char* libName);

    DISALLOW_EVIL_CONSTRUCTORS(SPRDAPEDecoder);
};

}  // namespace android

#endif  // SPRD_APE_DECODER_H_
