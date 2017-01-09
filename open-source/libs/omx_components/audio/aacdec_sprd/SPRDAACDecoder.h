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

#ifndef SPRD_AAC_DECODER_H_

#define SPRD_AAC_DECODER_H_

#include "SprdSimpleOMXComponent.h"
#include "aac_dec_api.h"
//struct tPVMP4AudioDecoderExternal;

namespace android {

#define AAC_STREAM_BUF_SIZE 1024*64+256
#define AAC_PCM_OUT_SIZE  2048

struct SPRDAACDecoder : public SprdSimpleOMXComponent {
    SPRDAACDecoder(const char *name,
            const OMX_CALLBACKTYPE *callbacks,
            OMX_PTR appData,
            OMX_COMPONENTTYPE **component);

protected:
    virtual ~SPRDAACDecoder();

    virtual OMX_ERRORTYPE internalGetParameter(
            OMX_INDEXTYPE index, OMX_PTR params);

    virtual OMX_ERRORTYPE internalSetParameter(
            OMX_INDEXTYPE index, const OMX_PTR params);

    virtual void onQueueFilled(OMX_U32 portIndex);
    virtual void onPortFlushCompleted(OMX_U32 portIndex);
    virtual void onPortEnableCompleted(OMX_U32 portIndex, bool enabled);
    virtual void onReset();
    virtual void onPortFlushPrepare(OMX_U32 portIndex);

private:
    enum {
        kNumInputBuffers        = 4,
        kNumOutputBuffers       = 4,
    };

    bool mIsADTS;
    bool mIsLATM;
    void *mDecoderBuf;
    uint32_t mProfile;
    uint32_t mFrameSize;
    int32_t mChannels;
    int32_t mSamplingRate;
    uint16_t *mPcm_out_l;
    uint16_t *mPcm_out_r;
    bool mSeekFlag;
    uint8_t *mSpecialData;
    uint32_t mSpecialDataLen;
 
    size_t mInputBufferCount;
//    size_t mUpsamplingFactor;
    int64_t mAnchorTimeUs;
    int64_t mNumSamplesOutput;
    void* mLibHandle;

    FT_AAC_MemoryFree mAAC_MemoryFree;
    FT_AAC_MemoryAlloc mAAC_MemoryAlloc;
    FT_AAC_DecInit mAAC_DecInit;
    FT_AAC_RetrieveSampleRate mAAC_RetrieveSampleRate;
    FT_AAC_FrameDecode mAAC_FrameDecode;
    FT_AAC_DecStreamBufferUpdate  mAAC_DecStreamBufferUpdate;

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

    DISALLOW_EVIL_CONSTRUCTORS(SPRDAACDecoder);
};

}  // namespace android

#endif  // SPRD_AAC_DECODER_H_
