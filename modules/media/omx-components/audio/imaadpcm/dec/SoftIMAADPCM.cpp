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

//#define LOG_NDEBUG 0
#define LOG_TAG "SoftIMAADPCM"
#include <utils/Log.h>

#include "SoftIMAADPCM.h"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>

namespace android {

template<class T>
static void InitOMXParams(T *params) {
    params->nSize = sizeof(T);
    params->nVersion.s.nVersionMajor = 1;
    params->nVersion.s.nVersionMinor = 0;
    params->nVersion.s.nRevision = 0;
    params->nVersion.s.nStep = 0;
}

SoftIMAADPCM::SoftIMAADPCM(
        const char *name,
        const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData,
        OMX_COMPONENTTYPE **component)
    : SprdSimpleOMXComponent(name, callbacks, appData, component),
      mNumChannels(1),
      mSamplingRate(8000),
      mBlockAlign(0x400),
      mSignalledError(false) {
    CHECK(!strcmp(name, "OMX.google.imaadpcm.decoder"));

    initPorts();
}

SoftIMAADPCM::~SoftIMAADPCM() {
}

void SoftIMAADPCM::initPorts() {
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);

    def.nPortIndex = 0;
    def.eDir = OMX_DirInput;
    def.nBufferCountMin = kNumBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.nBufferSize = kMaxNumSamplesPerFrame/4;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainAudio;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 1;

    def.format.audio.cMIMEType = "audio/ima-adpcm";

    def.format.audio.pNativeRender = NULL;
    def.format.audio.bFlagErrorConcealment = OMX_FALSE;
    def.format.audio.eEncoding = (OMX_AUDIO_CODINGTYPE)OMX_AUDIO_CodingIMAADPCM;

    addPort(def);

    def.nPortIndex = 1;
    def.eDir = OMX_DirOutput;
    def.nBufferCountMin = kNumBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.nBufferSize = kMaxNumSamplesPerFrame * 2 * sizeof(int16_t);
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainAudio;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 2;

    def.format.audio.cMIMEType = const_cast<char *>(MEDIA_MIMETYPE_AUDIO_RAW);
    def.format.audio.pNativeRender = NULL;
    def.format.audio.bFlagErrorConcealment = OMX_FALSE;
    def.format.audio.eEncoding = OMX_AUDIO_CodingPCM;

    addPort(def);
}

OMX_ERRORTYPE SoftIMAADPCM::getExtensionIndex(
        const char *name, OMX_INDEXTYPE *index)
{
    if(strcmp(name, "OMX.sprd.index.AudioImaadpam") == 0) {
        *index = (OMX_INDEXTYPE) OMX_IndexParamAudioImaAdpcm;
        return OMX_ErrorNone;
    }
    return SprdSimpleOMXComponent::getExtensionIndex(name, index);
}

OMX_ERRORTYPE SoftIMAADPCM::internalGetParameter(
        OMX_INDEXTYPE index, OMX_PTR params) {
    switch (index) {
        case OMX_IndexParamAudioImaAdpcm:
        {
            int32_t *para = (int32_t *)params;
            para[0] = mNumChannels;
            para[1] = 4;
            para[2] = mSamplingRate;
            para[3] = mBlockAlign;

            return OMX_ErrorNone;
        }

        case OMX_IndexParamAudioPcm:
        {
            OMX_AUDIO_PARAM_PCMMODETYPE *pcmParams =
                (OMX_AUDIO_PARAM_PCMMODETYPE *)params;

            if (pcmParams->nPortIndex != 1) {
                return OMX_ErrorUndefined;
            }

            pcmParams->eNumData = OMX_NumericalDataSigned;
            pcmParams->eEndian = OMX_EndianBig;
            pcmParams->bInterleaved = OMX_TRUE;
            pcmParams->nBitPerSample = 16;
            pcmParams->ePCMMode = OMX_AUDIO_PCMModeLinear;

            if (mNumChannels == 1) {
                pcmParams->eChannelMapping[0] = OMX_AUDIO_ChannelCF;
            } else {
                CHECK_EQ(mNumChannels, 2);

                pcmParams->eChannelMapping[0] = OMX_AUDIO_ChannelLF;
                pcmParams->eChannelMapping[1] = OMX_AUDIO_ChannelRF;
            }

            pcmParams->nChannels = mNumChannels;
            pcmParams->nSamplingRate = mSamplingRate;

            return OMX_ErrorNone;
        }

        default:
            return SprdSimpleOMXComponent::internalGetParameter(index, params);
    }
}

OMX_ERRORTYPE SoftIMAADPCM::internalSetParameter(
        OMX_INDEXTYPE index, const OMX_PTR params) {
    switch (index) {
        case OMX_IndexParamAudioImaAdpcm:
        {
            int32_t *para = (int32_t *)params;
            if (para[0] < 1 || para[0] > 2) {
                return OMX_ErrorUndefined;
            }
            mNumChannels = para[0];
            mSamplingRate = para[2];
            mBlockAlign = para[3];

            return OMX_ErrorNone;
        }

        case OMX_IndexParamStandardComponentRole:
        {
            const OMX_PARAM_COMPONENTROLETYPE *roleParams =
                (const OMX_PARAM_COMPONENTROLETYPE *)params;

            if (strncmp((const char *)roleParams->cRole,
                       "audio_decoder.imaadpcm",
                       OMX_MAX_STRINGNAME_SIZE - 1)) {
                return OMX_ErrorUndefined;
            }

            return OMX_ErrorNone;
        }

        default:
            return SprdSimpleOMXComponent::internalSetParameter(index, params);
    }
}

void SoftIMAADPCM::onQueueFilled(OMX_U32 portIndex) {
    if (mSignalledError) {
        return;
    }

    List<BufferInfo *> &inQueue = getPortQueue(0);
    List<BufferInfo *> &outQueue = getPortQueue(1);

    while (!inQueue.empty() && !outQueue.empty()) {
        BufferInfo *inInfo = *inQueue.begin();
        OMX_BUFFERHEADERTYPE *inHeader = inInfo->mHeader;

        BufferInfo *outInfo = *outQueue.begin();
        OMX_BUFFERHEADERTYPE *outHeader = outInfo->mHeader;

        if (inHeader->nFlags & OMX_BUFFERFLAG_EOS) {
            inQueue.erase(inQueue.begin());
            inInfo->mOwnedByUs = false;
            notifyEmptyBufferDone(inHeader);

            outHeader->nFilledLen = 0;
            outHeader->nFlags = OMX_BUFFERFLAG_EOS;

            outQueue.erase(outQueue.begin());
            outInfo->mOwnedByUs = false;
            notifyFillBufferDone(outHeader);
            return;
        }

        int samples_per_frame;
        samples_per_frame = ((mBlockAlign / mNumChannels - 4) << 1) + 1;
        if (inHeader->nFilledLen%mBlockAlign != 0) {
            ALOGW("WARNING! input buffer corrupt, len=%d, ba=%d", inHeader->nFilledLen, mBlockAlign);
        }

        if (inHeader->nFilledLen/mBlockAlign*samples_per_frame > kMaxNumSamplesPerFrame) {
            ALOGE("input buffer too large (%ld).", inHeader->nFilledLen);

            mSignalledError = true;
        }

        const uint8_t *inputPtr = inHeader->pBuffer + inHeader->nOffset;
        int inputLength = inHeader->nFilledLen;

        int16_t *outputPtr = reinterpret_cast<int16_t *>(outHeader->pBuffer);
        int frames = 0;
        while (inputLength >= mBlockAlign) {
            DecodeIMAADPCM(
                outputPtr,
                inputPtr, mNumChannels, mBlockAlign);
            inputLength -= mBlockAlign;
            inputPtr += mBlockAlign;
            outputPtr += samples_per_frame * mNumChannels;
            frames += samples_per_frame;
        }

        outHeader->nTimeStamp = inHeader->nTimeStamp;
        outHeader->nOffset = 0;
        outHeader->nFilledLen = frames * mNumChannels * sizeof(int16_t);
        outHeader->nFlags = 0;

        inInfo->mOwnedByUs = false;
        inQueue.erase(inQueue.begin());
        inInfo = NULL;
        notifyEmptyBufferDone(inHeader);
        inHeader = NULL;

        outInfo->mOwnedByUs = false;
        outQueue.erase(outQueue.begin());
        outInfo = NULL;
        notifyFillBufferDone(outHeader);
        outHeader = NULL;
    }
}

static void _adpcm_decode_frame(int16_t **dst_ptrs,
                          int dst_step,
                          const uint8_t *src_frame_ptr,
                          int frames,
                          int channels
                          );

// static
void SoftIMAADPCM::DecodeIMAADPCM(
        int16_t *out, const uint8_t *in, int channels, size_t inSize) {
        int frames = ((inSize / channels - 4) << 1) + 1;
        int16_t *dst[2];
        dst[0] = out;
        dst[1] = out+1;
        _adpcm_decode_frame(dst, channels, in, frames, channels);
}

/* First table lookup for Ima-ADPCM quantizer */
static const int8_t IndexAdjust[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };

/* Second table lookup for Ima-ADPCM quantizer */
static const short StepSize[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

typedef struct _ima_adpcm_state {
    int pred_val;                /* Calculated predicted value */
    int step_idx;                /* Previous StepSize lookup index */
} ima_adpcm_state_t;

static int adpcm_decoder(unsigned char code, ima_adpcm_state_t * state) {
    short pred_diff;    /* Predicted difference to next sample */
    short step;           /* holds previous StepSize value */
    char sign;

    int i;

    /* Separate sign and magnitude */
    sign = code & 0x8;
    code &= 0x7;

    /*
     * Computes pred_diff = (code + 0.5) * step / 4,
     * but see comment in adpcm_coder.
     */

    step = StepSize[state->step_idx];

    /* Compute difference and new predicted value */
    pred_diff = step >> 3;
    for (i = 0x4; i; i >>= 1, step >>= 1) {
        if (code & i) {
            pred_diff += step;
        }
    }
    state->pred_val += (sign) ? -pred_diff : pred_diff;

    /* Clamp output value */
    if (state->pred_val > 32767) {
        state->pred_val = 32767;
    } else if (state->pred_val < -32768) {
        state->pred_val = -32768;
    }

    /* Find new StepSize index value */
    state->step_idx += IndexAdjust[code];

    if (state->step_idx < 0) {
        state->step_idx = 0;
    } else if (state->step_idx > 88) {
    state->step_idx = 88;
    }
    return (state->pred_val);
}

static void _adpcm_decode_mono(int16_t *dst_ptr,
                                int dst_step,
                                const uint8_t *src_ptr,
                                unsigned int frames,
                                ima_adpcm_state_t *states) {
    int srcbit = 0;
    while (frames-- > 0) {
        unsigned char v;
        if (!srcbit)
            v = *src_ptr & 0x0f;
        else
            v = (*src_ptr >> 4) & 0x0f;

            *dst_ptr = adpcm_decoder(v, states);
            srcbit ++;
            if (srcbit == 2) {
                src_ptr++;
                srcbit = 0;
            }
            dst_ptr += dst_step;
    }
}

static void _adpcm_decode_frame(int16_t **dst_ptrs,
                                int dst_step,
                                const uint8_t *src_frame_ptr,
                                int frames,
                                int channels) {
    ima_adpcm_state_t state[2];
    int16_t *dst_ptr[2];
    int i;
    assert(channels==1 || channels==2);
    // parse headers
    for (i = 0; i < channels; i++) {
        state[i].pred_val = ((int16_t)(src_frame_ptr[0] | (src_frame_ptr[1]<<8)));
        src_frame_ptr+=2;
        state[i].step_idx = *src_frame_ptr;
        src_frame_ptr+=2;
        dst_ptr[i] = dst_ptrs[i];
        *dst_ptr[i] = state[i].pred_val;
        dst_ptr[i] += dst_step;
    }
    frames --;
    // decode samples
    while (frames > 0) {
        for (i = 0; i < channels; i++) {
            int decoded_fremes = frames > 8 ? 8 : frames;
            _adpcm_decode_mono(dst_ptr[i],
                      dst_step,
                      src_frame_ptr,
                      decoded_fremes, // max 8 samples per group
                      &state[i]);
            src_frame_ptr += 4;
            dst_ptr[i] += 8*dst_step;
        }
        frames -= 8;
    }
}

}  // namespace android

android::SprdOMXComponent *createSprdOMXComponent(
        const char *name, const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData, OMX_COMPONENTTYPE **component) {
    return new android::SoftIMAADPCM(name, callbacks, appData, component);
}

