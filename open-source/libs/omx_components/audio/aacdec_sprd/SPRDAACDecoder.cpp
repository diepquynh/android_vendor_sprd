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
#define LOG_TAG "SPRDAACDecoder"
#include <utils/Log.h>

#include "SPRDAACDecoder.h"

#include "aac_dec_api.h"

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

SPRDAACDecoder::SPRDAACDecoder(
        const char *name,
        const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData,
        OMX_COMPONENTTYPE **component)
    : SprdSimpleOMXComponent(name, callbacks, appData, component),
       mIsADTS(false),
       mIsLATM(false),
       mDecoderBuf(NULL),
       mProfile (0),
       mFrameSize(0),
       mChannels(1),
       mSamplingRate(44100),
       mSeekFlag(false),
       mSpecialData(NULL),
       mSpecialDataLen(0),
      mPcm_out_l(NULL),
      mPcm_out_r(NULL),
      mInputBufferCount(0),
      mAnchorTimeUs(0),
      mNumSamplesOutput(0),
      mLibHandle(NULL),
      mSignalledError(false),
      mAAC_MemoryFree(NULL),
      mAAC_MemoryAlloc(NULL),
      mAAC_DecInit(NULL),
      mAAC_RetrieveSampleRate(NULL),
      mAAC_FrameDecode(NULL),
      mAAC_DecStreamBufferUpdate(NULL),
      mOutputPortSettingsChange(NONE) {
    bool ret = false;
    ret = openDecoder("libomx_aacdec_sprd.so");
    CHECK_EQ(ret, true);
    initPorts();
    CHECK_EQ(initDecoder(), (status_t)OK);
}

SPRDAACDecoder::~SPRDAACDecoder() {
    mAAC_MemoryFree(&mDecoderBuf);
    mDecoderBuf = NULL;

    ALOGI("~SPRDAACDecoder.");
    delete []mSpecialData;
    mSpecialData = NULL;
    delete []mPcm_out_l;
    mPcm_out_l = NULL;
    delete []mPcm_out_r;
    mPcm_out_r = NULL;

    if(mLibHandle) {
        dlclose(mLibHandle);
        mLibHandle = NULL;
    }
}

void SPRDAACDecoder::initPorts() {
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

    def.format.audio.cMIMEType = const_cast<char *>(MEDIA_MIMETYPE_AUDIO_AAC);
    def.format.audio.pNativeRender = NULL;
    def.format.audio.bFlagErrorConcealment = OMX_FALSE;
    def.format.audio.eEncoding = OMX_AUDIO_CodingAAC;

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

    def.format.audio.cMIMEType = const_cast<char *>("audio/raw");
    def.format.audio.pNativeRender = NULL;
    def.format.audio.bFlagErrorConcealment = OMX_FALSE;
    def.format.audio.eEncoding = OMX_AUDIO_CodingPCM;

    addPort(def);
}

status_t SPRDAACDecoder::initDecoder() {
    if(mAAC_MemoryAlloc(&mDecoderBuf))
    {
        ALOGE("Failed to initialize AAC audio decoder");
        return UNKNOWN_ERROR;
    }

    mPcm_out_l = new uint16_t[AAC_PCM_OUT_SIZE*2];
    mPcm_out_r = new uint16_t[AAC_PCM_OUT_SIZE*2];

    return OK;
}

OMX_ERRORTYPE SPRDAACDecoder::internalGetParameter(
        OMX_INDEXTYPE index, OMX_PTR params) {
    switch (index) {
        case OMX_IndexParamAudioAac:
        {
            OMX_AUDIO_PARAM_AACPROFILETYPE *aacParams =
                (OMX_AUDIO_PARAM_AACPROFILETYPE *)params;

            if (aacParams->nPortIndex != 0) {
                return OMX_ErrorUndefined;
            }

            aacParams->nBitRate = 0;
            aacParams->nAudioBandWidth = 0;
            aacParams->nAACtools = 0;
            aacParams->nAACERtools = 0;
            aacParams->eAACProfile = OMX_AUDIO_AACObjectMain;

            aacParams->eAACStreamFormat =
                mIsADTS
                    ? OMX_AUDIO_AACStreamFormatMP4ADTS
                    : OMX_AUDIO_AACStreamFormatMP4FF;

            aacParams->eAACStreamFormat =
                mIsLATM
                    ? OMX_AUDIO_AACStreamFormatMP4LATM
                    : aacParams->eAACStreamFormat;

            aacParams->eChannelMode = OMX_AUDIO_ChannelModeStereo;

            aacParams->nChannels = mChannels;
            aacParams->nSampleRate = mSamplingRate;

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

            pcmParams->nChannels = mChannels;
            pcmParams->nSamplingRate = mSamplingRate;

            return OMX_ErrorNone;
        }

        default:
            return SprdSimpleOMXComponent::internalGetParameter(index, params);
    }
}

OMX_ERRORTYPE SPRDAACDecoder::internalSetParameter(
        OMX_INDEXTYPE index, const OMX_PTR params) {
    switch (index) {
        case OMX_IndexParamStandardComponentRole:
        {
            const OMX_PARAM_COMPONENTROLETYPE *roleParams =
                (const OMX_PARAM_COMPONENTROLETYPE *)params;

            if (strncmp((const char *)roleParams->cRole,
                        "audio_decoder.aac",
                        OMX_MAX_STRINGNAME_SIZE - 1)) {
                return OMX_ErrorUndefined;
            }

            return OMX_ErrorNone;
        }

        case OMX_IndexParamAudioAac:
        {
            const OMX_AUDIO_PARAM_AACPROFILETYPE *aacParams =
                (const OMX_AUDIO_PARAM_AACPROFILETYPE *)params;

            if (aacParams->nPortIndex != 0) {
                return OMX_ErrorUndefined;
            }

            mProfile = aacParams->eAACProfile;

            mChannels = aacParams->nChannels;
            mSamplingRate = aacParams->nSampleRate;

            ALOGI("sampleRate : %d, channels : %d", mSamplingRate, mChannels);

            if (aacParams->eAACStreamFormat == OMX_AUDIO_AACStreamFormatMP4FF) {
                mIsADTS = false;
                mIsLATM = false;
            } else if (aacParams->eAACStreamFormat
                        == OMX_AUDIO_AACStreamFormatMP4LATM) {
                mIsADTS = false;
                mIsLATM = true;
            } else if (aacParams->eAACStreamFormat
                        == OMX_AUDIO_AACStreamFormatMP4ADTS) {
                mIsADTS = true;
                mIsLATM = false;
            } else {
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

bool SPRDAACDecoder::isConfigured() const {
    return mInputBufferCount > 0;
}
#if 0
void dump_aac(int port, void *buffer, size_t size)
{
    char indebugflag[256];
    char outdebugflag[256];
    static FILE *fpin = NULL;
    static FILE *fpout = NULL;
    static FILE *fpinbak = NULL;
    char *pathin = "/data/local/media/sprdaacinput.aac";
    char *pathout = "/data/local/media/sprdaacoutput.wav";
    char *pathinbak = "/data/local/media/sprdaacinputbak.aac";

    property_get("debug.sprd.aac.enable.input", indebugflag, "");
    property_get("debug.sprd.aac.enable.output", outdebugflag, "");

    if(!strncmp("1", indebugflag, 1) && (fpin == NULL)) {
        fpin = fopen(pathin, "wb+");
        fpinbak = fopen(pathinbak, "wb+");
    } else if(strncmp("1", indebugflag, 1) && (fpin != NULL)) {
        fclose(fpin);
        fpin = NULL;
        fclose(fpinbak);
        fpinbak = NULL;
    }

    if(!strncmp("1", outdebugflag, 1) && (fpout == NULL)) {
        fpout = fopen(pathout, "wb+");
    } else if(strncmp("1", outdebugflag, 1) && (fpout != NULL)) {
        fclose(fpout);
        fpout = NULL;
    }

    if(!strncmp("1", indebugflag, 1) || !strncmp("1", outdebugflag, 1)) {
        long dumpmaxsize = 20 * 1024 * 1024;
        if(!strncmp("1", indebugflag, 1) && !strncmp("1", outdebugflag, 1)) {
            //dump input and output at the same time
            dumpmaxsize = 2 * 1024 * 1024;
        } else {
            //only dump input or output stream
            dumpmaxsize = 20 * 1024 * 1024;
        }

        if((port == 0) && !strncmp("1", indebugflag, 1)) { //input
            fseek(fpin, 0L, SEEK_END);
            fseek(fpinbak, 0L, SEEK_END);
            if(ftell(fpin) >= dumpmaxsize) {
                fclose(fpin);
                fpin = fopen(pathin, "wb+");
                fclose(fpinbak);
                fpinbak = fopen(pathinbak, "wb+");
                // sync output to header
                if(!strncmp("1", indebugflag, 1) && !strncmp("1", outdebugflag, 1)) {
                    fclose(fpout);
                    fpout = fopen(pathout, "wb+");
                }
            }

            fwrite(buffer, 1 , size, fpin);
            uint8_t length[4] = {0};
            length[0] = (uint8_t)((size >> 24) & 0xff);
            length[1] = (uint8_t)((size >> 16) & 0xff);
            length[2] = (uint8_t)((size >> 8) & 0xff);
            length[3] = (uint8_t)(size & 0xff);
            fwrite(length, 1 , sizeof(length), fpinbak);
            fwrite(buffer, 1 , size, fpinbak);
        } else if((port == 1) && !strncmp("1", outdebugflag, 1)) { //output
            if(strncmp("1", indebugflag, 1) && !strncmp("1", outdebugflag, 1)) {
                fseek(fpout, 0L, SEEK_END);
                if(ftell(fpout) >= dumpmaxsize) {
                    fclose(fpout);
                    fpout = fopen(pathout, "wb+");
                }
            }

            fwrite(buffer, 1 , size, fpout);
        }
    }
}
#endif
void SPRDAACDecoder::onQueueFilled(OMX_U32 portIndex) {
    if (mSignalledError || mOutputPortSettingsChange != NONE) {
        return;
    }

    List<BufferInfo *> &inQueue = getPortQueue(0);
    List<BufferInfo *> &outQueue = getPortQueue(1);

    if (portIndex == 0 && mInputBufferCount == 0) {
        ++mInputBufferCount;

        if (mSeekFlag && mDecoderBuf) {
            mSeekFlag = false;
            mAAC_MemoryFree(&mDecoderBuf);
            if(mAAC_MemoryAlloc(&mDecoderBuf)) {
                ALOGE("Failed to initialize AAC audio decoder when Seek");
            }
        }

        BufferInfo *info = *inQueue.begin();
        OMX_BUFFERHEADERTYPE *header = info->mHeader;
        if (!(header->nFlags & OMX_BUFFERFLAG_CODECCONFIG)) {
            ALOGI("not codec config.");
        } else {
            if(!mSpecialData) {
                delete []mSpecialData;
                mSpecialData = NULL;
            }
            mSpecialDataLen = header->nFilledLen;
            mSpecialData = new uint8_t[mSpecialDataLen];
            memcpy(mSpecialData, header->pBuffer + header->nOffset, mSpecialDataLen);
        }
        uint8_t latm[] = {'L', 'A', 'T', 'M', 0};
        int16_t initRet;
        int16_t sign = 0;
        uint8_t *pInputBuffer =  header->pBuffer + header->nOffset;
        uint32_t inputBufferCurrentLength =  header->nFilledLen;

        if(mIsLATM) {
            if(mProfile == OMX_AUDIO_AACObjectLC) {
                latm[4] = 0;
            } else {
                latm[4] = 1;
            }

            sign = 1;
            pInputBuffer = latm;
            inputBufferCurrentLength = sizeof(latm);
            ALOGW("AAC Frame is LATM");
        } else {
            sign = 0;
            pInputBuffer = mSpecialData;
            inputBufferCurrentLength = mSpecialDataLen;
            ALOGW("AAC Frame is ADTS");
        }

        initRet = mAAC_DecInit((int8_t *)pInputBuffer,inputBufferCurrentLength,mSamplingRate,sign,mDecoderBuf);
        if(initRet){
            ALOGW("AAC decoder init returned error %d", initRet);
            mSignalledError = true;
            notify(OMX_EventError, OMX_ErrorUndefined, initRet, NULL);
            return;
        }

        // Check on the sampling rate to see whether it is changed.
        int32_t sampleRate = mAAC_RetrieveSampleRate(mDecoderBuf);
        if (mSamplingRate != sampleRate) {
            ALOGI("aac sampleRate is %d, but now is %d", mSamplingRate, sampleRate);
            mSamplingRate = sampleRate;
            initRet = mAAC_DecInit((int8_t *)pInputBuffer,inputBufferCurrentLength,mSamplingRate,sign,mDecoderBuf);
            if(initRet){
                ALOGW("AAC decoder init returned error %d", initRet);
                mSignalledError = true;
                notify(OMX_EventError, OMX_ErrorUndefined, initRet, NULL);
                return;
            }
            inQueue.erase(inQueue.begin());
            info->mOwnedByUs = false;
            notifyEmptyBufferDone(header);
            notify(OMX_EventPortSettingsChanged, 1, 0, NULL);
            mOutputPortSettingsChange = AWAITING_DISABLED;
            return;
        }

        if (header->nFlags & OMX_BUFFERFLAG_CODECCONFIG) {
            inQueue.erase(inQueue.begin());
            info->mOwnedByUs = false;
            notifyEmptyBufferDone(header);
        }
    }

decoding:
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

        if (inHeader->nOffset == 0) {
            mAnchorTimeUs = inHeader->nTimeStamp;
            mNumSamplesOutput = 0;
        }

        uint8_t *pInputBuffer;
        uint32_t inputBufferCurrentLength;

        if (mIsADTS) {
            // skip 30 bits, aac_frame_length follows.

            const uint8_t *adtsHeader = inHeader->pBuffer + inHeader->nOffset;

            bool signalError = false;
            if (inHeader->nFilledLen < 7) {
                ALOGE("Audio data too short to contain even the ADTS header. "
                      "Got %ld bytes.", inHeader->nFilledLen);
                hexdump(adtsHeader, inHeader->nFilledLen);
                signalError = true;
            } else {
                bool protectionAbsent = (adtsHeader[1] & 1);

                unsigned aac_frame_length =
                    ((adtsHeader[3] & 3) << 11)
                    | (adtsHeader[4] << 3)
                    | (adtsHeader[5] >> 5);

                if (inHeader->nFilledLen < aac_frame_length) {
                    ALOGE("Not enough audio data for the complete frame. "
                          "Got %ld bytes, frame size according to the ADTS "
                          "header is %u bytes.",
                          inHeader->nFilledLen, aac_frame_length);
                    hexdump(adtsHeader, inHeader->nFilledLen);
                    signalError = true;
                } else {
                    size_t adtsHeaderSize = (protectionAbsent ? 7 : 9);

                    pInputBuffer = (uint8_t *)adtsHeader + adtsHeaderSize;
                    inputBufferCurrentLength = aac_frame_length - adtsHeaderSize;

                    inHeader->nOffset += adtsHeaderSize;
                    inHeader->nFilledLen -= adtsHeaderSize;
                }
            }

            if (signalError) {
                mSignalledError = true;

                notify(OMX_EventError,
                       OMX_ErrorStreamCorrupt,
                       ERROR_MALFORMED,
                       NULL);

                return;
            }
        } else {
            pInputBuffer =  inHeader->pBuffer + inHeader->nOffset;
            inputBufferCurrentLength =  inHeader->nFilledLen;
        }

        uint16_t frm_pcm_len;
        int16_t  decoderRet = 0;
        int16_t  decodedBytes = 0;
        if(inputBufferCurrentLength > 0) {

            decoderRet = mAAC_FrameDecode(pInputBuffer,inputBufferCurrentLength,mPcm_out_l,mPcm_out_r,&frm_pcm_len,mDecoderBuf,1, &decodedBytes);

            //dump_aac(0, (void *)pInputBuffer, (size_t)inputBufferCurrentLength);

        } else {
            ALOGW("AAC decoder stream buf size error %d",inputBufferCurrentLength);
            decoderRet = 2;
            frm_pcm_len = 2048;
        }

        if (mIsLATM) {
            if(decoderRet == 0) {
                // LATM decode sucess
                inHeader->nFilledLen = 0;
                mFrameSize = frm_pcm_len;
            } else if(decoderRet == 1) {
                // LATM need decode continue
                mFrameSize = frm_pcm_len;
            } else {
                // LATM decode fail
                inHeader->nFilledLen = 0;
                ALOGW("AAC LATM current buffer decode fail %d", decoderRet);
            }
        } else {
            if((decoderRet == 0) && ((int32_t)decodedBytes <= (int32_t)inputBufferCurrentLength)) {
                // ADTS decode success
                inHeader->nFilledLen -= decodedBytes;
                inHeader->nOffset += decodedBytes;
                mFrameSize = frm_pcm_len;
            } else {
                // ADTS decode fail
                inHeader->nFilledLen = 0;
                ALOGW("AAC ADTS current buffer decode fail %d", decoderRet);
            }
        }

        size_t numOutBytes =  mFrameSize * sizeof(int16_t);
        if (mChannels == 2) {
            numOutBytes *=  2;
        }
        uint16_t * pOutputBuffer = reinterpret_cast<uint16_t *>(outHeader->pBuffer + outHeader->nOffset);

        if((!mIsLATM && (decoderRet != 0)) || (mIsLATM && (decoderRet != 0) && (decoderRet != 1))) {
            memset(pOutputBuffer, 0, numOutBytes);
        } else {
            for(uint32_t i = 0; i < mFrameSize; i++) {
                if (mChannels == 2) {
                    pOutputBuffer[2 * i] = mPcm_out_l[i];
                    pOutputBuffer[2 * i + 1] = mPcm_out_r[i];
                } else {
                    pOutputBuffer[i] = mPcm_out_l[i];
                }
            }
        }

        //dump_aac(1, (void *)pOutputBuffer, (size_t)numOutBytes);

        {
            // We'll only output data if we successfully decoded it or
            // we've previously decoded valid data, in the latter case
            // (decode failed) we'll output a silent frame.
            outHeader->nFilledLen = numOutBytes;
            outHeader->nFlags = 0;

            outHeader->nTimeStamp =
                mAnchorTimeUs
                    + (mNumSamplesOutput * 1000000ll) / mSamplingRate;

            mNumSamplesOutput += mFrameSize;

            outInfo->mOwnedByUs = false;
            outQueue.erase(outQueue.begin());
            outInfo = NULL;
            notifyFillBufferDone(outHeader);
            outHeader = NULL;
        }

        if (inHeader->nFilledLen == 0) {
            inInfo->mOwnedByUs = false;
            inQueue.erase(inQueue.begin());
            inInfo = NULL;
            notifyEmptyBufferDone(inHeader);
            inHeader = NULL;
        }

        if (decoderRet == 0) {
            ++mInputBufferCount;
        }
    }
}

void SPRDAACDecoder::onPortFlushPrepare(OMX_U32 portIndex) {
    ALOGI("onPortFlushPrepare.");
    if (portIndex == 0) {
        mInputBufferCount = 0;
        mSeekFlag = true;
    }
}

void SPRDAACDecoder::onPortFlushCompleted(OMX_U32 portIndex) {
    if (portIndex == 0) {
        ALOGI("onPortFlushCompleted.");
        // Make sure that the next buffer output does not still
        // depend on fragments from the last one decoded.
        mAAC_DecStreamBufferUpdate(1,mDecoderBuf);
    }
}

void SPRDAACDecoder::onPortEnableCompleted(OMX_U32 portIndex, bool enabled) {
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

bool SPRDAACDecoder::openDecoder(const char* libName)
{
    if(mLibHandle) {
        dlclose(mLibHandle);
    }

    ALOGI("openDecoder, lib: %s", libName);

    mLibHandle = dlopen(libName, RTLD_NOW);
    if(mLibHandle == NULL) {
        ALOGE("openDecoder, can't open lib: %s",libName);
        return false;
    }

    mAAC_MemoryFree = (FT_AAC_MemoryFree)dlsym(mLibHandle, "AAC_MemoryFree");
    if(mAAC_MemoryFree == NULL) {
        ALOGE("Can't find AAC_MemoryFree in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mAAC_MemoryAlloc = (FT_AAC_MemoryAlloc)dlsym(mLibHandle, "AAC_MemoryAlloc");
    if(mAAC_MemoryAlloc == NULL) {
        ALOGE("Can't find AAC_MemoryAlloc in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mAAC_DecInit = (FT_AAC_DecInit)dlsym(mLibHandle, "AAC_DecInit");
    if(mAAC_DecInit == NULL) {
        ALOGE("Can't find AAC_DecInit in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mAAC_RetrieveSampleRate = (FT_AAC_RetrieveSampleRate)dlsym(mLibHandle, "AAC_RetrieveSampleRate");
    if(mAAC_RetrieveSampleRate == NULL) {
        ALOGE("Can't find AAC_RetrieveSampleRate in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mAAC_FrameDecode = (FT_AAC_FrameDecode)dlsym(mLibHandle, "AAC_FrameDecode");
    if(mAAC_FrameDecode == NULL) {
        ALOGE("Can't find AAC_FrameDecode in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mAAC_DecStreamBufferUpdate = (FT_AAC_DecStreamBufferUpdate)dlsym(mLibHandle, "AAC_DecStreamBufferUpdate");
    if(mAAC_DecStreamBufferUpdate == NULL) {
        ALOGE("Can't find AAC_DecStreamBufferUpdate in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    return true;
}

void SPRDAACDecoder::onReset() {
    // TODO.
    ALOGI("onReset.");
}

}  // namespace android

android::SprdOMXComponent *createSprdOMXComponent(
        const char *name, const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData, OMX_COMPONENTTYPE **component) {
    return new android::SPRDAACDecoder(name, callbacks, appData, component);
}
