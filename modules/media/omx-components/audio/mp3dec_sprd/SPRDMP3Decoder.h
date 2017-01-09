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

#ifndef SPRD_MP3_DECODER_H_

#define SPRD_MP3_DECODER_H_

#include "SprdSimpleOMXComponent.h"
#include "mp3_dec_api.h"

namespace android {

#define MP3_MAX_DATA_FRAME_LEN  (1536)  //unit by bytes
#define MP3_DEC_FRAME_LEN       (1152)  //pcm samples number

struct SPRDMP3Decoder : public SprdSimpleOMXComponent {
    SPRDMP3Decoder(const char *name,
                   const OMX_CALLBACKTYPE *callbacks,
                   OMX_PTR appData,
                   OMX_COMPONENTTYPE **component);

protected:
    virtual ~SPRDMP3Decoder();

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
        kNumBuffers = 4,
        kOutputBufferSize = 4608 * 2,
        kPVMP3DecoderDelay = 529 // frames
    };

    int32_t mNumChannels;
    int32_t mSamplingRate;
    int32_t mBitRate;
    uint16_t mNextMdBegin;
    int32_t mPreFilledLen;

    void *mMP3DecHandle;
    uint16_t *mLeftBuf;  //output pcm buffer
    uint16_t *mRightBuf;
    uint8_t *mMaxFrameBuf;

    int64_t mLastInTimeUs;
    int64_t mAnchorTimeUs;
    int64_t mNumFramesOutput;

    bool mIsFirst;
    bool mEOSFlag;
    bool mFirstFrame;
    bool mSignalledError;
    bool mInputEosUnDecode;
    void* mLibHandle;

    FT_MP3_ARM_DEC_Construct mMP3_ARM_DEC_Construct;
    FT_MP3_ARM_DEC_Deconstruct mMP3_ARM_DEC_Deconstruct;
    FT_MP3_ARM_DEC_InitDecoder mMP3_ARM_DEC_InitDecoder;
    FT_MP3_ARM_DEC_DecodeFrame mMP3_ARM_DEC_DecodeFrame;

    enum {
        NONE,
        AWAITING_DISABLED,
        AWAITING_ENABLED
    } mOutputPortSettingsChange;

    void initPorts();
    void initDecoder();

    uint32_t getNextMdBegin(uint8_t *frameBuf);
    uint32_t getCurFrameBitRate(uint8_t *frameBuf);
    bool openDecoder(const char* libName);

    DISALLOW_EVIL_CONSTRUCTORS(SPRDMP3Decoder);
};

}  // namespace android

#endif  // SPRD_MP3_DECODER_H_


