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
#define LOG_TAG "SPRDAPEDecoder"
#include <utils/Log.h>

#include "SPRDAPEDecoder.h"

#include "MONKEY_APIs.h"
#include "MONKEY_APEDecompress.h"

#include <cutils/properties.h>
#include <dlfcn.h>

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/hexdump.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>

namespace android {

template<class T>
static void InitOMXParams(T *params) {
    params->nSize = sizeof(T);
    params->nVersion.s.nVersionMajor = 1;
    params->nVersion.s.nVersionMinor = 0;
    params->nVersion.s.nRevision = 0;
    params->nVersion.s.nStep = 0;
}

SPRDAPEDecoder::SPRDAPEDecoder(
        const char *name,
        const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData,
        OMX_COMPONENTTYPE **component)
    : SprdSimpleOMXComponent(name, callbacks, appData, component),
      mDecoderBuf(NULL),
      mProfile (0),
      mFrameSize(0),
      mOutputSize(0),
      mSamplingRate(44100),
      mPcm_out(NULL),
      mInputBufferCount(0),
      mAnchorTimeUs(0),
      mNumSamplesOutput(0),
      mLibHandle(NULL),
      mSignalledError(false),
      mInputCurrentLength(0),
      mAPEDecompress_Init(NULL),
      mAPEDecompress_Dec(NULL),
      mAPEDecompress_Term(NULL),
      mOutputPortSettingsChange(NONE) {
    bool ret = false;
    ret = openDecoder("libomx_apedec_sprd.so");
    CHECK_EQ(ret, true);
    initPorts();
    CHECK_EQ(initDecoder(), (status_t)OK);
}

SPRDAPEDecoder::~SPRDAPEDecoder() {
    mAPEDecompress_Term(&mDecoder->HANDLE, &mDecoder->CONFIG);
    if(mDecoder != NULL) {
        free(mDecoder);
        mDecoder = NULL;
    }

    if(mLibHandle) {
        dlclose(mLibHandle);
        mLibHandle = NULL;
    }

    delete []mPcm_out;
    mPcm_out = NULL;
}

void SPRDAPEDecoder::initPorts() {
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);

    def.nPortIndex = 0;
    def.eDir = OMX_DirInput;
    def.nBufferCountMin = kNumInputBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.nBufferSize = 8192;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainAudio;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 1;

    def.format.audio.cMIMEType = const_cast<char *>(MEDIA_MIMETYPE_AUDIO_APE);
    def.format.audio.pNativeRender = NULL;
    def.format.audio.bFlagErrorConcealment = OMX_FALSE;
    def.format.audio.eEncoding = OMX_AUDIO_CodingAPE;

    addPort(def);

    def.nPortIndex = 1;
    def.eDir = OMX_DirOutput;
    def.nBufferCountMin = kNumOutputBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.nBufferSize = 8192;
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

status_t SPRDAPEDecoder::initDecoder() {
    mDecoder = (APE__StreamDecoder *)malloc(sizeof(APE__StreamDecoder));
    if(mDecoder == NULL) {
        ALOGE("Failed to alloc mDecoder");
        return UNKNOWN_ERROR;
    } else {
        memset(mDecoder, 0, sizeof(APE__StreamDecoder));
    }

    mPcm_out = new char[BUFFER_PCMOUTPUT_BYTES];
    if(mPcm_out == NULL) {
        ALOGE("Failed to alloc mDecoder");
        return UNKNOWN_ERROR;
    } else {
        memset(mPcm_out, 0, sizeof(APE__StreamDecoder));
    }        
    return OK;
}

OMX_ERRORTYPE SPRDAPEDecoder::internalGetParameter(
        OMX_INDEXTYPE index, OMX_PTR params) {
    switch (index) {
        case OMX_IndexParamAudioApe:
        {
            OMX_AUDIO_PARAM_APETYPE *apeParams =
                (OMX_AUDIO_PARAM_APETYPE *)params;

            if (apeParams->nPortIndex != 0) {
                return OMX_ErrorUndefined;
            }

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
            pcmParams->eChannelMapping[0] = OMX_AUDIO_ChannelLF;
            pcmParams->eChannelMapping[1] = OMX_AUDIO_ChannelRF;

            if (!isConfigured()) {
                pcmParams->nChannels = 1;
                pcmParams->nSamplingRate = 44100;
            } else {
                pcmParams->nChannels = 2;
                pcmParams->nSamplingRate = mSamplingRate;
            }

            return OMX_ErrorNone;
        }

        default:
            return SprdSimpleOMXComponent::internalGetParameter(index, params);
    }
}

OMX_ERRORTYPE SPRDAPEDecoder::internalSetParameter(
        OMX_INDEXTYPE index, const OMX_PTR params) {
    switch (index) {
        case OMX_IndexParamStandardComponentRole:
        {
            const OMX_PARAM_COMPONENTROLETYPE *roleParams =
                (const OMX_PARAM_COMPONENTROLETYPE *)params;

            if (strncmp((const char *)roleParams->cRole,
                        "audio_decoder.ape",
                        OMX_MAX_STRINGNAME_SIZE - 1)) {
                return OMX_ErrorUndefined;
            }

            return OMX_ErrorNone;
        }

        case OMX_IndexParamAudioApe:
        {
            const OMX_AUDIO_PARAM_AACPROFILETYPE *apeParams =
                (const OMX_AUDIO_PARAM_AACPROFILETYPE *)params;

            if (apeParams->nPortIndex != 0) {
                return OMX_ErrorUndefined;
            }

            return OMX_ErrorNone;
        }

        case OMX_IndexParamAudioPcm:
        {
            const OMX_AUDIO_PARAM_PCMMODETYPE *pcmParams =
                (OMX_AUDIO_PARAM_PCMMODETYPE *)params;

            if (pcmParams->nPortIndex != 1) {
                return OMX_ErrorUndefined;
            }

            return OMX_ErrorNone;
        }

        default:
            return SprdSimpleOMXComponent::internalSetParameter(index, params);
    }
}

bool SPRDAPEDecoder::isConfigured() const {
    return mInputBufferCount > 0;
}

void SPRDAPEDecoder::onQueueFilled(OMX_U32 portIndex) {
    if (mSignalledError || mOutputPortSettingsChange != NONE) {
        return;
    }
    uint64_t currentTime=0;
    uint64_t stepTime=(uint64_t)((double)1024*1000000/mDecoder->CONFIG.nSampleRate);
    ALOGV("onQueueFilled enter");

    List<BufferInfo *> &inQueue = getPortQueue(0);
    List<BufferInfo *> &outQueue = getPortQueue(1);

    if (portIndex == 0 && mInputBufferCount == 0) {
        ++mInputBufferCount;

        BufferInfo *info = *inQueue.begin();
        OMX_BUFFERHEADERTYPE *header = info->mHeader;
        int initRet;
        uint8_t *pInputBuffer =  header->pBuffer + header->nOffset;
        uint32_t inputBufferCurrentLength =  header->nFilledLen;

        if (header->nFlags & OMX_BUFFERFLAG_CODECCONFIG) {
            if(inputBufferCurrentLength > sizeof(MONKEY_APE_DEC_CONFIG)) {
                ALOGE("APE Configure info is too large");
            } else {
                memcpy(&mDecoder->CONFIG, pInputBuffer, inputBufferCurrentLength);
            }
            initRet = mAPEDecompress_Init(&mDecoder->HANDLE, &mDecoder->CONFIG);
            mDecoder->INPUT.pchData = NULL;
            mDecoder->INPUT.nLenAlloc = 0;
            mDecoder->INPUT.nLen = 0;
            mDecoder->INPUT.counter = 0;

            mDecoder->OUTPUT.pchData = NULL;
            mDecoder->OUTPUT.nLenAlloc = 0;
            mDecoder->OUTPUT.nLen = 0;
            mDecoder->OUTPUT.counter = 0;

            if(initRet != ERROR_SUCCESS) {
                ALOGE("APE decoder init returned error %d", initRet);
                mSignalledError = true;
                notify(OMX_EventError, OMX_ErrorUndefined, initRet, NULL);
                return;
            } else {
                ALOGE("APE decoder init returned ok %d", initRet);
            }
        } else {
            ALOGE("APE decoder don't have APEInfo data");
        }

        inQueue.erase(inQueue.begin());
        info->mOwnedByUs = false;
        notifyEmptyBufferDone(header);

        notify(OMX_EventPortSettingsChanged, 1, 0, NULL);
        mOutputPortSettingsChange = AWAITING_DISABLED;
        return;
    }

    while (!inQueue.empty() && !outQueue.empty()) {
        BufferInfo *inInfo = *inQueue.begin();
        OMX_BUFFERHEADERTYPE *inHeader = inInfo->mHeader;
        uint8_t *pInputBuffer;
        char remainder = 0;
        
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

        if ((inHeader->nOffset == 0)||( mAnchorTimeUs != inHeader->nTimeStamp)) {
            ALOGV("SPRDAPEDecoder:: decode begin-------------------------------init decoder");
            mAnchorTimeUs = inHeader->nTimeStamp;
            mNumSamplesOutput = 0;

            pInputBuffer = inHeader->pBuffer + inHeader->nOffset;
            mInputCurrentLength = inHeader->nFilledLen;
            remainder = *pInputBuffer;

            mDecoder->INPUT.pchData = (char *)pInputBuffer + 1;
            mDecoder->INPUT.nLen = mInputCurrentLength;
            mDecoder->INPUT.counter = 0;

            mDecoder->OUTPUT.pchData = mPcm_out;
            mDecoder->OUTPUT.nLen = 0;
            mDecoder->OUTPUT.nLenAlloc = BUFFER_PCMOUTPUT_BYTES;
            mDecoder->OUTPUT.counter = 0;
        }
        currentTime = inHeader->nTimeStamp;
        int decoderRet = 0;
        if(mInputCurrentLength > 0) {
            if(mDecoder->HANDLE.m_nCurrentTimeUs != mAnchorTimeUs) {
                int nBlockOffset=1024*mAnchorTimeUs/((int)(1024.0*1000000.0/(float)mDecoder->CONFIG.nSampleRate));

                int nBaseFrame=nBlockOffset/mDecoder->CONFIG.nBlocksPerFrame;
                int nBlocksToSkip=nBlockOffset%mDecoder->CONFIG.nBlocksPerFrame;
                int nBytesToSkip=nBlocksToSkip*mDecoder->CONFIG.nBytesPerSample*mDecoder->CONFIG.nChannels;
                ALOGV("APEDecompress ----------------------------------begin");
                ALOGV("APEDecompress currentTime = %llu ", currentTime);
                ALOGV("APEDecompress mAnchorTimeUs = %llu ", mAnchorTimeUs);
                ALOGV("APEDecompress nBlockOffset = %d ", nBlockOffset);
                ALOGV("APEDecompress nBaseFrame = %d ", nBaseFrame);
                ALOGV("APEDecompress nBlocksToSkip = %d ", nBlocksToSkip);
                mDecoder->HANDLE.m_nCurrentBlock=nBaseFrame*mDecoder->CONFIG.nBlocksPerFrame;
                mDecoder->HANDLE.m_nCurrentFrameBufferBlock=nBaseFrame*mDecoder->CONFIG.nBlocksPerFrame;
                mDecoder->HANDLE.m_nCurrentFrame=nBaseFrame;
                mDecoder->HANDLE.nBlocksToOutput=mDecoder->HANDLE.m_nFinishBlock-nBaseFrame*mDecoder->CONFIG.nBlocksPerFrame;
                mDecoder->HANDLE.m_cbFrameBufferX.m_nHead=0;
                mDecoder->HANDLE.m_cbFrameBufferX.m_nTail=0;
                mDecoder->HANDLE.m_cbFrameBufferX.m_nEndCap=mDecoder->HANDLE.m_cbFrameBufferX.m_nTotal;
                mDecoder->HANDLE.m_cbFrameBufferY.m_nHead=0;
                mDecoder->HANDLE.m_cbFrameBufferY.m_nTail=0;
                mDecoder->HANDLE.m_cbFrameBufferY.m_nEndCap=mDecoder->HANDLE.m_cbFrameBufferY.m_nTotal;
                mDecoder->HANDLE.m_spUnBitArray.m_nBytes=0;
                mDecoder->HANDLE.m_spUnBitArray.m_nCurrentBitIndex=0;
                mDecoder->HANDLE.m_spUnBitArray.m_nCurrentBitIndex=remainder<<3;
                mOutputSize=0;
                mNumSamplesOutput=0;
                while(nBlocksToSkip>0){
                    ALOGV("APEDecompress ----------------------------------ing");
                    mAPEDecompress_Dec(&mDecoder->HANDLE, &mDecoder->CONFIG, &mDecoder->INPUT, &mDecoder->OUTPUT);
                    nBlocksToSkip-=1024;
                    mDecoder->OUTPUT.nLen=0;
                    mDecoder->OUTPUT.counter=0;
                    mOutputSize+=1024;
                    mNumSamplesOutput+=1024;
                }
                ALOGV("APEDecompress ----------------------------------end");
                mDecoder->HANDLE.m_nCurrentTimeUs = mAnchorTimeUs;
            }

            decoderRet = mAPEDecompress_Dec(&mDecoder->HANDLE, &mDecoder->CONFIG, &mDecoder->INPUT, &mDecoder->OUTPUT);
        } else {
            ALOGW("APE decoder stream buf size error %d", mInputCurrentLength);
            decoderRet = 2;
        }

        int compare;
        int64_t tmp =(int64_t)((float)mDecoder->CONFIG.nBlocksPerFrame * (mDecoder->CONFIG.nTotalFrames - 1)/mDecoder->CONFIG.nSampleRate*1000000);
        if (mAnchorTimeUs < tmp) {
            compare = mDecoder->CONFIG.nBlocksPerFrame;
        } else {
            compare = mDecoder->CONFIG.nFinalFrameBlocks;
        }

        mOutputSize += mDecoder->OUTPUT.counter;
        if (mOutputSize >= compare) {
            inHeader->nFilledLen = 0;
            mOutputSize = 0;
        }

        mFrameSize=mDecoder->OUTPUT.counter;
        size_t numOutBytes =  mFrameSize * sizeof(int16_t) * mDecoder->CONFIG.nChannels;
        uint16_t * pOutputBuffer = reinterpret_cast<uint16_t *>(outHeader->pBuffer + outHeader->nOffset);


        if((decoderRet != ERROR_SUCCESS)) {
            memset(pOutputBuffer, 0, numOutBytes);
        } else {
            for(int i = 0; i < mFrameSize; i++) {
                pOutputBuffer[2 * i] = *((int16_t*)mPcm_out+i);
                pOutputBuffer[2 * i + 1] =*((int16_t *)mPcm_out +numOutBytes /4 + i);
            }
        }

        {
            ALOGV("outHeader outQueue erase");
            // We'll only output data if we successfully decoded it or
            // we've previously decoded valid data, in the latter case
            // (decode failed) we'll output a silent frame.
            outHeader->nFilledLen = numOutBytes;
            outHeader->nFlags = 0;

            outHeader->nTimeStamp = mAnchorTimeUs
                    + (mNumSamplesOutput * 1000000ll) / mSamplingRate;

            mNumSamplesOutput += mFrameSize;

            outInfo->mOwnedByUs = false;
            outQueue.erase(outQueue.begin());
            outInfo = NULL;
            notifyFillBufferDone(outHeader);
            outHeader = NULL;
            mDecoder->OUTPUT.counter=0;
        }

        if (inHeader->nFilledLen == 0) {
            ALOGV("inHeader inQueue erase");
            inInfo->mOwnedByUs = false;
            inQueue.erase(inQueue.begin());
            inInfo = NULL;
            notifyEmptyBufferDone(inHeader);
            inHeader = NULL;
        } else {
            inHeader->nOffset = mDecoder->INPUT.counter;
        }

        if (decoderRet == 0) {
            ++mInputBufferCount;
        }
    }
}

void SPRDAPEDecoder::onPortFlushCompleted(OMX_U32 portIndex) {
    if (portIndex == 0) {
        // Make sure that the next buffer output does not still
        // depend on fragments from the last one decoded.
    }
}

void SPRDAPEDecoder::onPortEnableCompleted(OMX_U32 portIndex, bool enabled) {
    if (portIndex != 1) {
        return;
    }

    switch (mOutputPortSettingsChange) {
        case NONE:
            break;

        case AWAITING_DISABLED:
        {
            CHECK(!enabled);
            mOutputPortSettingsChange = AWAITING_ENABLED;
            break;
        }

        default:
        {
            CHECK_EQ((int)mOutputPortSettingsChange, (int)AWAITING_ENABLED);
            CHECK(enabled);
            mOutputPortSettingsChange = NONE;
            break;
        }
    }
}

bool SPRDAPEDecoder::openDecoder(const char* libName)
{
    if(mLibHandle) {
        dlclose(mLibHandle);
        mLibHandle = NULL;
    }

    ALOGI("openDecoder, lib: %s", libName);

    mLibHandle = dlopen(libName, RTLD_NOW);
    if(mLibHandle == NULL) {
        ALOGE("openDecoder, can't open lib: %s",libName);
        return false;
    }

    mAPEDecompress_Init= (FT_APEDecompress_Init)dlsym(mLibHandle, "MONKEY_APEDecompress_Init");
    if(mAPEDecompress_Init == NULL) {
        ALOGE("Can't find MONKEY_APEDecompress_Init in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mAPEDecompress_Dec = (FT_APEDecompress_Dec)dlsym(mLibHandle, "MONKEY_APEDecompress_Dec");
    if(mAPEDecompress_Dec == NULL) {
        ALOGE("Can't find MONKEY_APEDecompress_Dec in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mAPEDecompress_Term = (FT_APEDecompress_Term)dlsym(mLibHandle, "MONKEY_APEDecompress_Term");
    if(mAPEDecompress_Term == NULL) {
        ALOGE("Can't find MONKEY_APEDecompress_Term in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    return true;
}

}  // namespace android

android::SprdOMXComponent *createSprdOMXComponent(
        const char *name, const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData, OMX_COMPONENTTYPE **component) {
    return new android::SPRDAPEDecoder(name, callbacks, appData, component);
}
