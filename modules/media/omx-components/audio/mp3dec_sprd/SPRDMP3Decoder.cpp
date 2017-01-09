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
#define LOG_TAG "SPRDMP3"
#include <utils/Log.h>

#include "SPRDMP3Decoder.h"

#include "mp3_dec_api.h"

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

SPRDMP3Decoder::SPRDMP3Decoder(
    const char *name,
    const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData,
    OMX_COMPONENTTYPE **component)
    : SprdSimpleOMXComponent(name, callbacks, appData, component),
      mNumChannels(2),
      mSamplingRate(44100),
      mBitRate(0),
      mNextMdBegin(0),
      mPreFilledLen(0),
      mMaxFrameBuf(NULL),
      mLastInTimeUs(0),
      mAnchorTimeUs(0),
      mNumFramesOutput(0),
      mEOSFlag(false),
      mSignalledError(false),
      mInputEosUnDecode(false),
      mLibHandle(NULL),
      mOutputPortSettingsChange(NONE),
      mMP3_ARM_DEC_Construct(NULL),
      mMP3_ARM_DEC_Deconstruct(NULL),
      mMP3_ARM_DEC_InitDecoder(NULL),
      mMP3_ARM_DEC_DecodeFrame(NULL) {
    bool ret = false;
    ret = openDecoder("libomx_mp3dec_sprd.so");
    CHECK_EQ(ret, true);
    initPorts();
    initDecoder();
}

SPRDMP3Decoder::~SPRDMP3Decoder() {
    delete mLeftBuf;
    mLeftBuf = NULL;

    delete mRightBuf;
    mRightBuf = NULL;

    delete mMaxFrameBuf;
    mMaxFrameBuf = NULL;

    mMP3_ARM_DEC_Deconstruct((void const **)&mMP3DecHandle);

    if(mLibHandle) {
        dlclose(mLibHandle);
        mLibHandle = NULL;
    }
}

void SPRDMP3Decoder::initPorts() {
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);

    def.nPortIndex = 0;
    def.eDir = OMX_DirInput;
    def.nBufferCountMin = kNumBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.nBufferSize = 8192;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainAudio;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 1;

    def.format.audio.cMIMEType =
        const_cast<char *>(MEDIA_MIMETYPE_AUDIO_MPEG);

    def.format.audio.pNativeRender = NULL;
    def.format.audio.bFlagErrorConcealment = OMX_FALSE;
    def.format.audio.eEncoding = OMX_AUDIO_CodingMP3;

    addPort(def);

    def.nPortIndex = 1;
    def.eDir = OMX_DirOutput;
    def.nBufferCountMin = kNumBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.nBufferSize = kOutputBufferSize;
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

void SPRDMP3Decoder::initDecoder() {
    //Output buffer
    mLeftBuf = new uint16_t[MP3_DEC_FRAME_LEN];
    mRightBuf = new uint16_t[MP3_DEC_FRAME_LEN];

    //Temporary source data frame buffer.
    mMaxFrameBuf = new uint8_t[MP3_MAX_DATA_FRAME_LEN];

    int32_t ret = mMP3_ARM_DEC_Construct(&mMP3DecHandle);
    ALOGI("MP3_ARM_DEC_Construct=%d", ret);

    //Init sprd mp3 decoder
    mMP3_ARM_DEC_InitDecoder(mMP3DecHandle);

    mIsFirst = true;
    mFirstFrame = true;
}

OMX_ERRORTYPE SPRDMP3Decoder::internalGetParameter(
    OMX_INDEXTYPE index, OMX_PTR params) {
    switch (index) {
    case OMX_IndexParamAudioPcm:
    {
        OMX_AUDIO_PARAM_PCMMODETYPE *pcmParams =
            (OMX_AUDIO_PARAM_PCMMODETYPE *)params;

        if (pcmParams->nPortIndex > 1) {
            return OMX_ErrorUndefined;
        }

        pcmParams->eNumData = OMX_NumericalDataSigned;
        pcmParams->eEndian = OMX_EndianBig;
        pcmParams->bInterleaved = OMX_TRUE;
        pcmParams->nBitPerSample = 16;
        pcmParams->ePCMMode = OMX_AUDIO_PCMModeLinear;
        pcmParams->eChannelMapping[0] = OMX_AUDIO_ChannelLF;
        pcmParams->eChannelMapping[1] = OMX_AUDIO_ChannelRF;

        pcmParams->nChannels = mNumChannels;
        pcmParams->nSamplingRate = mSamplingRate;

        return OMX_ErrorNone;
    }
    case OMX_IndexParamAudioMp3:
    {
	    OMX_AUDIO_PARAM_MP3TYPE *mp3Params =
		    (OMX_AUDIO_PARAM_MP3TYPE *)params;

	    if (mp3Params->nPortIndex > 1) {
		    return OMX_ErrorUndefined;
	    }

	    mp3Params->nChannels = mNumChannels;
	    mp3Params->nBitRate = 0 /* unknown */;
	    mp3Params->nSampleRate = mSamplingRate;
	    // other fields are encoder-only

	    return OMX_ErrorNone;
    }
    default:
        return SprdSimpleOMXComponent::internalGetParameter(index, params);
    }
}

OMX_ERRORTYPE SPRDMP3Decoder::internalSetParameter(
    OMX_INDEXTYPE index, const OMX_PTR params) {
    switch (index) {
    case OMX_IndexParamStandardComponentRole:
    {
        const OMX_PARAM_COMPONENTROLETYPE *roleParams =
            (const OMX_PARAM_COMPONENTROLETYPE *)params;

        if (strncmp((const char *)roleParams->cRole,
                "audio_decoder.mp3", OMX_MAX_STRINGNAME_SIZE - 1) &&
                strncmp((const char *)roleParams->cRole,
                "audio_decoder.mp1", OMX_MAX_STRINGNAME_SIZE - 1) &&
                strncmp((const char *)roleParams->cRole,
                "audio_decoder.mp2", OMX_MAX_STRINGNAME_SIZE - 1)) {
            return OMX_ErrorUndefined;
        }

        return OMX_ErrorNone;
    }

    case OMX_IndexParamAudioPcm:
    {
        const OMX_AUDIO_PARAM_PCMMODETYPE *pcmParams =
            (const OMX_AUDIO_PARAM_PCMMODETYPE *)params;

        if (pcmParams->nPortIndex != 1) {
            return OMX_ErrorUndefined;
        }

        mNumChannels = pcmParams->nChannels;
        mSamplingRate = pcmParams->nSamplingRate;

        ALOGI("OMX_IndexParamAudioPcm, chanel:%d, sampleRate:%d", mNumChannels, mSamplingRate);

        return OMX_ErrorNone;
    }

    case OMX_IndexParamAudioMp3:
    {
        const OMX_AUDIO_PARAM_MP3TYPE *mp3Params =
            (const OMX_AUDIO_PARAM_MP3TYPE *)params;

        mNumChannels = mp3Params->nChannels;
        mSamplingRate = mp3Params->nSampleRate;

        ALOGI("mp3 decoder params chanel:%d, sampleRate:%d", mNumChannels, mSamplingRate);

        return OMX_ErrorNone;
    }

    default:
        return SprdSimpleOMXComponent::internalSetParameter(index, params);
    }
}

uint32_t SPRDMP3Decoder::getCurFrameBitRate(uint8_t *frameBuf)
{
    uint32_t header = 0;
    uint32_t bitrate = 0;

    if (frameBuf) {
        header = (frameBuf[0]<<24)|(frameBuf[1]<<16)|(frameBuf[2]<<8)|frameBuf[3];

        unsigned layer = (header >> 17) & 3;
        unsigned bitrate_index = (header >> 12) & 0x0f;
        unsigned version = (header >> 19) & 3;

        if (layer == 3) {
            // layer I
            static const int kBitrateV1[] = {
                32, 64, 96, 128, 160, 192, 224, 256,
                288, 320, 352, 384, 416, 448
            };

            static const int kBitrateV2[] = {
                32, 48, 56, 64, 80, 96, 112, 128,
                144, 160, 176, 192, 224, 256
            };

            bitrate =
                (version == 3 /* V1 */)
                ? kBitrateV1[bitrate_index - 1]
                : kBitrateV2[bitrate_index - 1];

        } else {
            // layer II or III
            static const int kBitrateV1L2[] = {
                32, 48, 56, 64, 80, 96, 112, 128,
                160, 192, 224, 256, 320, 384
            };

            static const int kBitrateV1L3[] = {
                32, 40, 48, 56, 64, 80, 96, 112,
                128, 160, 192, 224, 256, 320
            };

            static const int kBitrateV2[] = {
                8, 16, 24, 32, 40, 48, 56, 64,
                80, 96, 112, 128, 144, 160
            };

            if (version == 3 /* V1 */) {
                bitrate = (layer == 2 /* L2 */)
                          ? kBitrateV1L2[bitrate_index - 1]
                          : kBitrateV1L3[bitrate_index - 1];
            } else {
                // V2 (or 2.5)
                bitrate = kBitrateV2[bitrate_index - 1];
            }
        }
        //ALOGW("header=0x%x, bitrate=%d", header, bitrate);
        //ALOGV("layer(%d), bitrate_index(%d), version(%d)", layer, bitrate_index, version);
    }
    return bitrate;
}

uint32_t SPRDMP3Decoder::getNextMdBegin(uint8_t *frameBuf)
{
    uint32_t header = 0;
    uint32_t result = 0;
    uint32_t offset = 0;

    if (frameBuf) {
        header = (frameBuf[0]<<24)|(frameBuf[1]<<16)|(frameBuf[2]<<8)|frameBuf[3];
        offset += 4;

        unsigned layer = (header >> 17) & 3;

        if (layer == 1) {
            //only for layer3, else next_md_begin = 0.

            if ((header & 0xFFE60000L) == 0xFFE20000L)
            {
                if (!(header & 0x00010000L))
                {
                    offset += 2;
                    if (header & 0x00080000L)
                    {
                        result = ((uint32_t)frameBuf[7]>>7)|((uint32_t)frameBuf[6]<<1);
                    }
                    else
                    {
                        result = frameBuf[6];
                    }
                }
                else
                {
                    if (header & 0x00080000L)
                    {
                        result = ((uint32_t)frameBuf[5]>>7)|((uint32_t)frameBuf[4]<<1);
                    }
                    else
                    {
                        result = frameBuf[4];
                    }
                }
            }
        }
    }
    return result;
}
#if 0
void dump_mp3(int port, void *buffer, size_t size)
{
    char indebugflag[10];
    char outdebugflag[10];
    static FILE *fpin = NULL;
    static FILE *fpout = NULL;
    static FILE *fpinbak = NULL;
    char *pathin = "/data/sprdmp3input.mp3";
    char *pathout = "/data/sprdmp3output.wav";

    property_get("debug.sprd.mp3.enable.input", indebugflag, "");
    property_get("debug.sprd.mp3.enable.output", outdebugflag, "");

    if(!strncmp("1", indebugflag, 1) && (fpin == NULL)) {
        fpin = fopen(pathin, "wb+");
    } else if(strncmp("1", indebugflag, 1) && (fpin != NULL)) {
        fclose(fpin);
        fpin = NULL;
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
            if(ftell(fpin) >= dumpmaxsize) {
                fclose(fpin);
                fpin = fopen(pathin, "wb+");
                // sync output to header
                if(!strncmp("1", indebugflag, 1) && !strncmp("1", outdebugflag, 1)) {
                    fclose(fpout);
                    fpout = fopen(pathout, "wb+");
                }
            }

            fwrite(buffer, 1 , size, fpin);
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
void SPRDMP3Decoder::onQueueFilled(OMX_U32 portIndex) {
    if (mSignalledError || mOutputPortSettingsChange != NONE) {
        return;
    }

    List<BufferInfo *> &inQueue = getPortQueue(0);
    List<BufferInfo *> &outQueue = getPortQueue(1);

    while ((!inQueue.empty() || mInputEosUnDecode) && !outQueue.empty()) {
        BufferInfo *inInfo = NULL;;
        OMX_BUFFERHEADERTYPE *inHeader = NULL;
        if (!inQueue.empty()) {
            inInfo = *inQueue.begin();
            inHeader = inInfo->mHeader;
        }

        BufferInfo *outInfo = *outQueue.begin();
        OMX_BUFFERHEADERTYPE *outHeader = outInfo->mHeader;

        if (mEOSFlag) {
            if (inHeader) {
                inQueue.erase(inQueue.begin());
                inInfo->mOwnedByUs = false;
                notifyEmptyBufferDone(inHeader);
            }
            mInputEosUnDecode = false;
            if (!mIsFirst) {
                // pad the end of the stream with 529 samples, since that many samples
                // were trimmed off the beginning when decoding started
                outHeader->nFilledLen = kPVMP3DecoderDelay * mNumChannels * sizeof(int16_t);
                memset(outHeader->pBuffer, 0, outHeader->nFilledLen);
            } else {
                // Since we never discarded frames from the start, we won't have
                // to add any padding at the end either.
                outHeader->nFilledLen = 0;
            }
            outHeader->nTimeStamp = mLastInTimeUs;
            outHeader->nFlags = OMX_BUFFERFLAG_EOS;

            outQueue.erase(outQueue.begin());
            outInfo->mOwnedByUs = false;
            notifyFillBufferDone(outHeader);
            return;
        }
        if (inHeader && inHeader->nOffset == 0) {
            mAnchorTimeUs = inHeader->nTimeStamp;
            mNumFramesOutput = 0;
        } else if (!inHeader && mInputEosUnDecode){
            mNumFramesOutput = 0;
        }

        FRAME_DEC_T inputParam ;
        OUTPUT_FRAME_T outputFrame ;
        uint32_t decoderRet = 0;
        size_t numOutBytes = 0;

        memset(&inputParam, 0, sizeof(FRAME_DEC_T));
        memset(&outputFrame, 0, sizeof(OUTPUT_FRAME_T));

        if (mIsFirst) {
            mIsFirst = false;
            mFirstFrame = true;
            mPreFilledLen = inHeader->nFilledLen;
            if (mMaxFrameBuf && mPreFilledLen <= MP3_MAX_DATA_FRAME_LEN) {
                memcpy(mMaxFrameBuf, inHeader->pBuffer + inHeader->nOffset, mPreFilledLen);
                mLastInTimeUs = mAnchorTimeUs;
            }

            if (inHeader->nFlags & OMX_BUFFERFLAG_EOS) {
                mEOSFlag = true;
            } else {
                mEOSFlag = false;
                inInfo->mOwnedByUs = false;
                inQueue.erase(inQueue.begin());
                inInfo = NULL;
                notifyEmptyBufferDone(inHeader);
                inHeader = NULL;
                continue;
            }
        }

        inputParam.frame_buf_ptr = mMaxFrameBuf;
        inputParam.frame_len = mPreFilledLen;
        //Get current frame bitrate.
        mBitRate = getCurFrameBitRate(inputParam.frame_buf_ptr);
        if ((!inHeader && mInputEosUnDecode)
                || (inHeader && (inHeader->nFlags & OMX_BUFFERFLAG_EOS) && inHeader->nFilledLen == 0)
                || (inHeader && (inHeader->nFlags & OMX_BUFFERFLAG_EOS) && mFirstFrame)) {
            mEOSFlag = true;
            mNextMdBegin = 0;
        } else {
            if (inHeader->nFlags & OMX_BUFFERFLAG_EOS) {
                mInputEosUnDecode = true;
            }
            mEOSFlag = false;
            mNextMdBegin = getNextMdBegin(inHeader->pBuffer + inHeader->nOffset);
        }
        inputParam.next_begin = mNextMdBegin;
        inputParam.bitrate = mBitRate; //kbps

        //Config decoded output frame params.
        outputFrame.pcm_data_l_ptr = mLeftBuf;
        outputFrame.pcm_data_r_ptr = mRightBuf;
        mMP3_ARM_DEC_DecodeFrame(mMP3DecHandle, &inputParam,&outputFrame, &decoderRet);

        if(decoderRet != MP3_ARM_DEC_ERROR_NONE) { //decoder error
            ALOGE("MP3 decoder returned error %d, substituting silence", decoderRet);
            outputFrame.pcm_bytes = 1152; //samples number
        }
        //ALOGW("decoderRet=%d,pcm_samples=%d", decoderRet,outputFrame.pcm_bytes);

        uint16_t * pOutputBuffer = reinterpret_cast<uint16_t *>(outHeader->pBuffer);

        if(decoderRet != MP3_ARM_DEC_ERROR_NONE) {
            numOutBytes = outputFrame.pcm_bytes * sizeof(int16_t) *2;
            memset(outHeader->pBuffer, 0, numOutBytes);
        } else {
            for(int i=0; i<outputFrame.pcm_bytes; i++) {
                if(2 == mNumChannels)
                {
                    numOutBytes = outputFrame.pcm_bytes * sizeof(int16_t) *2;
                    pOutputBuffer[2*i] = mLeftBuf[i];
                    pOutputBuffer[2*i+1] = mRightBuf[i];
                } else {
                    numOutBytes = outputFrame.pcm_bytes * sizeof(int16_t);
                    pOutputBuffer[i] = mLeftBuf[i];
                }
            }
        }

        //dump_mp3(1, (void *)pOutputBuffer, (size_t)numOutBytes); //dump output data

        if (mFirstFrame) {
            mFirstFrame = false;
            // The decoder delay is 529 samples, so trim that many samples off
            // the start of the first output buffer. This essentially makes this
            // decoder have zero delay, which the rest of the pipeline assumes.
            outHeader->nOffset = kPVMP3DecoderDelay * mNumChannels * sizeof(int16_t);
            outHeader->nFilledLen = numOutBytes - outHeader->nOffset;
        } else {
            outHeader->nOffset = 0;
            outHeader->nFilledLen = numOutBytes;
        }
        //ALOGI("outHeader, numOutBytes:%d, offset:%d, filledlen:%d",numOutBytes,outHeader->nOffset,outHeader->nFilledLen);
        outHeader->nTimeStamp =
            mLastInTimeUs
            + (mNumFramesOutput * 1000000ll) / mSamplingRate;

        outHeader->nFlags = 0;

        mNumFramesOutput += outputFrame.pcm_bytes;

        if(mEOSFlag == false) {
            if (inHeader) {
                mPreFilledLen = inHeader->nFilledLen;
                if (mMaxFrameBuf && mPreFilledLen <= MP3_MAX_DATA_FRAME_LEN) {
                    memcpy(mMaxFrameBuf, inHeader->pBuffer + inHeader->nOffset, mPreFilledLen);
                    mLastInTimeUs = mAnchorTimeUs;
                }

                inInfo->mOwnedByUs = false;
                inQueue.erase(inQueue.begin());
                inInfo = NULL;
                notifyEmptyBufferDone(inHeader);
                inHeader = NULL;
            }
        }

        if (numOutBytes <= outHeader->nOffset) {
            ALOGI("onQueueFilled, numOutBytes:%d <= outHeader->nOffset:%ld, continue", numOutBytes, outHeader->nOffset);
            continue;
        }

        outInfo->mOwnedByUs = false;
        outQueue.erase(outQueue.begin());
        outInfo = NULL;
        notifyFillBufferDone(outHeader);
        outHeader = NULL;

    }
}

void SPRDMP3Decoder::onPortFlushPrepare(OMX_U32 portIndex) {
    if (portIndex == 0) {
        ALOGI("onPortFlushPrepare.");
        // Make sure that the next buffer output does not still
        // depend on fragments from the last one decoded.
        mNextMdBegin = 0;
        mPreFilledLen = 0;
        if (mLeftBuf) memset(mLeftBuf, 0, MP3_DEC_FRAME_LEN<<1);
        if (mRightBuf) memset(mRightBuf, 0, MP3_DEC_FRAME_LEN<<1);
        mMP3_ARM_DEC_InitDecoder(mMP3DecHandle);
        mIsFirst = true;
        mFirstFrame = true;
    }
}
void SPRDMP3Decoder::onPortFlushCompleted(OMX_U32 portIndex) {
    if (portIndex == 0) {
        mInputEosUnDecode = false;
        mEOSFlag = false;
    }
    // TODO
}

void SPRDMP3Decoder::onPortEnableCompleted(OMX_U32 portIndex, bool enabled) {
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

bool SPRDMP3Decoder::openDecoder(const char* libName)
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

    mMP3_ARM_DEC_Construct = (FT_MP3_ARM_DEC_Construct)dlsym(mLibHandle, "MP3_ARM_DEC_Construct");
    if(mMP3_ARM_DEC_Construct == NULL) {
        ALOGE("Can't find MP3_ARM_DEC_Construct in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mMP3_ARM_DEC_Deconstruct = (FT_MP3_ARM_DEC_Deconstruct)dlsym(mLibHandle, "MP3_ARM_DEC_Deconstruct");
    if(mMP3_ARM_DEC_Deconstruct == NULL) {
        ALOGE("Can't find MP3_ARM_DEC_Deconstruct in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mMP3_ARM_DEC_InitDecoder = (FT_MP3_ARM_DEC_InitDecoder)dlsym(mLibHandle, "MP3_ARM_DEC_InitDecoder");
    if(mMP3_ARM_DEC_InitDecoder == NULL) {
        ALOGE("Can't find MP3_ARM_DEC_InitDecoder in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mMP3_ARM_DEC_DecodeFrame = (FT_MP3_ARM_DEC_DecodeFrame)dlsym(mLibHandle, "MP3_ARM_DEC_DecodeFrame");
    if(mMP3_ARM_DEC_DecodeFrame == NULL) {
        ALOGE("Can't find MP3_ARM_DEC_DecodeFrame in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    return true;
}

void SPRDMP3Decoder::onReset() {
    // TODO.
    mInputEosUnDecode = false;
    mEOSFlag = false;
    ALOGI("onReset.");
}

}  // namespace android

android::SprdOMXComponent *createSprdOMXComponent(
    const char *name, const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData, OMX_COMPONENTTYPE **component) {
    return new android::SPRDMP3Decoder(name, callbacks, appData, component);
}

