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
#define LOG_TAG "SoftSPRDMPEG4"
#include <utils/Log.h>

#include "SoftSPRDMPEG4.h"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/IOMX.h>
#include <media/hardware/HardwareAPI.h>
#include <ui/GraphicBufferMapper.h>

#include "m4v_h263_dec_api.h"
#include <dlfcn.h>

namespace android {

typedef enum {
    H263_MODE = 0,MPEG4_MODE,
    FLV_MODE,
    UNKNOWN_MODE
} MP4DecodingMode;

static const CodecProfileLevel kM4VProfileLevels[] = {
    { OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level0 },
    { OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level0b },
    { OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level1 },
    { OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level2 },
    { OMX_VIDEO_MPEG4ProfileSimple, OMX_VIDEO_MPEG4Level3 },

    { OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level0 },
    { OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level0b },
    { OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level1 },
    { OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level2 },
    { OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level3 },
    { OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level4 },
    { OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level4a },
    { OMX_VIDEO_MPEG4ProfileAdvancedSimple, OMX_VIDEO_MPEG4Level5 },
};

static const CodecProfileLevel kH263ProfileLevels[] = {
    { OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level10 },
    { OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level20 },
    { OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level30 },
    { OMX_VIDEO_H263ProfileBaseline, OMX_VIDEO_H263Level45 },
    { OMX_VIDEO_H263ProfileISWV2,    OMX_VIDEO_H263Level10 },
    { OMX_VIDEO_H263ProfileISWV2,    OMX_VIDEO_H263Level20 },
    { OMX_VIDEO_H263ProfileISWV2,    OMX_VIDEO_H263Level30 },
    { OMX_VIDEO_H263ProfileISWV2,    OMX_VIDEO_H263Level45 },
};

template<class T>
static void InitOMXParams(T *params) {
    params->nSize = sizeof(T);
    params->nVersion.s.nVersionMajor = 1;
    params->nVersion.s.nVersionMinor = 0;
    params->nVersion.s.nRevision = 0;
    params->nVersion.s.nStep = 0;
}

void dump_bs( uint8 * pBuffer,uint32 aInBufSize) {
    FILE *fp = fopen("/data/video_es.m4v","ab");
    fwrite(pBuffer,1,aInBufSize,fp);
    fclose(fp);
}

void dump_yuv( uint8 * pBuffer,uint32 aInBufSize) {
    FILE *fp = fopen("/data/dump/video_out.yuv","ab");
    fwrite(pBuffer,1,aInBufSize,fp);
    fclose(fp);
}

SoftSPRDMPEG4::SoftSPRDMPEG4(
    const char *name,
    const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData,
    OMX_COMPONENTTYPE **component)
    : SprdSimpleOMXComponent(name, callbacks, appData, component),
      mMode(MODE_MPEG4),
      mHandle(new tagMP4Handle),
      mInputBufferCount(0),
      mWidth(352),
      mHeight(288),
      mCropLeft(0),
      mCropTop(0),
      mCropRight(mWidth - 1),
      mCropBottom(mHeight - 1),
      mMaxWidth(352),
      mMaxHeight(288),
      mEOSStatus(INPUT_DATA_AVAILABLE),
      mOutputPortSettingsChange(NONE),
      mHeadersDecoded(false),
      mSignalledError(false),
      mNeedIVOP(true),
      mInitialized(false),
      mFramesConfigured(false),
      mStopDecode(false),
      mThumbnailMode(OMX_FALSE),
      mCodecInterBuffer(NULL),
      mCodecExtraBuffer(NULL),
      mPVolHeader(NULL),
      mPVolHeaderSize(0),
      mLibHandle(NULL),
      mMP4DecSetCurRecPic(NULL),
      mMP4DecInit(NULL),
      mMP4DecVolHeader(NULL),
      mMP4DecMemInit(NULL),
      mMP4DecDecode(NULL),
      mMP4DecRelease(NULL),
      mMp4GetVideoDimensions(NULL),
      mMp4GetBufferDimensions(NULL),
      mMP4DecReleaseRefBuffers(NULL),
      mMP4DecSetReferenceYUV(NULL),
      mMP4DecGetLastDspFrm(NULL),
      mMP4GetCodecCapability(NULL) {

    ALOGI("Construct SoftSPRDMPEG4, this: 0x%p", (void *)this);

    if (!strcmp(name, "OMX.sprd.soft.h263.decoder")) {
        mMode = MODE_H263;
    } else {
        CHECK(!strcmp(name, "OMX.sprd.soft.mpeg4.decoder"));
    }

    CHECK_EQ(openDecoder("libomx_m4vh263dec_sw_sprd.so"), true);
    CHECK_EQ(initDecoder(), (status_t)OK);

    mPVolHeader = (uint8_t *)malloc(MPEG4_VOL_HEADER_SIZE);
    CHECK(mPVolHeader);

    initPorts();

    iUseAndroidNativeBuffer[OMX_DirInput] = OMX_FALSE;
    iUseAndroidNativeBuffer[OMX_DirOutput] = OMX_FALSE;
}

SoftSPRDMPEG4::~SoftSPRDMPEG4() {
    ALOGI("Destruct SoftSPRDMPEG4, this: %p", (void *)this);

    releaseDecoder();

    if (mPVolHeader != NULL) {
        free(mPVolHeader);
        mPVolHeader = NULL;
    }

    delete mHandle;
    mHandle = NULL;
}

void SoftSPRDMPEG4::initPorts() {
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);

    def.nPortIndex = 0;
    def.eDir = OMX_DirInput;
    def.nBufferCountMin = kNumInputBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.nBufferSize = 160*1024; ///8192;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainVideo;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 1;

    def.format.video.cMIMEType =
        (mMode == MODE_MPEG4)
        ? const_cast<char *>(MEDIA_MIMETYPE_VIDEO_MPEG4)
        : const_cast<char *>(MEDIA_MIMETYPE_VIDEO_H263);

    def.format.video.pNativeRender = NULL;
    def.format.video.nFrameWidth = mWidth;
    def.format.video.nFrameHeight = mHeight;
    def.format.video.nStride = def.format.video.nFrameWidth;
    def.format.video.nSliceHeight = def.format.video.nFrameHeight;
    def.format.video.nBitrate = 0;
    def.format.video.xFramerate = 0;
    def.format.video.bFlagErrorConcealment = OMX_FALSE;

    def.format.video.eCompressionFormat =
        mMode == MODE_MPEG4 ? OMX_VIDEO_CodingMPEG4 : OMX_VIDEO_CodingH263;

    def.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    def.format.video.pNativeWindow = NULL;

    addPort(def);

    def.nPortIndex = 1;
    def.eDir = OMX_DirOutput;
    def.nBufferCountMin = kNumOutputBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainVideo;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 2;

    def.format.video.cMIMEType = const_cast<char *>(MEDIA_MIMETYPE_VIDEO_RAW);
    def.format.video.pNativeRender = NULL;
    def.format.video.nFrameWidth = mWidth;
    def.format.video.nFrameHeight = mHeight;
    def.format.video.nStride = def.format.video.nFrameWidth;
    def.format.video.nSliceHeight = def.format.video.nFrameHeight;
    def.format.video.nBitrate = 0;
    def.format.video.xFramerate = 0;
    def.format.video.bFlagErrorConcealment = OMX_FALSE;
    def.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    def.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
    def.format.video.pNativeWindow = NULL;

    def.nBufferSize =
        (def.format.video.nFrameWidth * def.format.video.nFrameHeight * 3) / 2;

    addPort(def);
}

status_t SoftSPRDMPEG4::initDecoder() {
    memset(mHandle, 0, sizeof(tagMP4Handle));

    mHandle->userdata = (void *)this;
    mHandle->VSP_extMemCb = extMemoryAllocWrapper;
    mHandle->VSP_bindCb = BindFrameWrapper;
    mHandle->VSP_unbindCb = UnbindFrameWrapper;

    uint32_t size_inter = MP4DEC_INTERNAL_BUFFER_SIZE;
    mCodecInterBuffer = (uint8_t *)malloc(size_inter);
    CHECK(mCodecInterBuffer != NULL);

    MMCodecBuffer codec_buf;

    codec_buf.common_buffer_ptr = mCodecInterBuffer;
    codec_buf.common_buffer_ptr_phy= 0;
    codec_buf.size = size_inter;
    codec_buf.int_buffer_ptr = NULL;
    codec_buf.int_size = 0;

    if ((*mMP4DecInit)( mHandle, &codec_buf) != MMDEC_OK) {
        ALOGE("Failed to init MPEG4DEC");
        return OMX_ErrorUndefined;
    }

    if ((*mMP4GetCodecCapability)(mHandle, &mMaxWidth, &mMaxHeight) != MMDEC_OK) {
        ALOGE("Failed to mMP4GetCodecCapability");
    }
    return OMX_ErrorNone;
}

void SoftSPRDMPEG4::releaseDecoder() {
    if( mMP4DecRelease!=NULL )
        (*mMP4DecRelease)(mHandle);

    if (mInitialized) {
        mInitialized = false;
    }

    if (mCodecInterBuffer != NULL) {
        free(mCodecInterBuffer);
        mCodecInterBuffer = NULL;
    }

    if (mCodecExtraBuffer != NULL) {
        free(mCodecExtraBuffer);
        mCodecExtraBuffer = NULL;
    }

    if(mLibHandle) {
        dlclose(mLibHandle);
        mLibHandle = NULL;
        mMP4DecReleaseRefBuffers = NULL;
        mMP4DecRelease = NULL;
    }
}

OMX_ERRORTYPE SoftSPRDMPEG4::internalGetParameter(
    OMX_INDEXTYPE index, OMX_PTR params) {
    switch (index) {
    case OMX_IndexParamVideoPortFormat:
    {
        OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams =
            (OMX_VIDEO_PARAM_PORTFORMATTYPE *)params;

        if (formatParams->nPortIndex > 1) {
            return OMX_ErrorUndefined;
        }

        if (formatParams->nIndex != 0) {
            return OMX_ErrorNoMore;
        }

        if (formatParams->nPortIndex == 0) {
            formatParams->eCompressionFormat =
                (mMode == MODE_MPEG4)
                ? OMX_VIDEO_CodingMPEG4 : OMX_VIDEO_CodingH263;

            formatParams->eColorFormat = OMX_COLOR_FormatUnused;
            formatParams->xFramerate = 0;
        } else {
            CHECK_EQ(formatParams->nPortIndex, 1u);

            PortInfo *pOutPort = editPortInfo(kOutputPortIndex);
            ALOGI("internalGetParameter, OMX_IndexParamVideoPortFormat, eColorFormat: 0x%x",pOutPort->mDef.format.video.eColorFormat);
            formatParams->eCompressionFormat = OMX_VIDEO_CodingUnused;
            formatParams->eColorFormat = pOutPort->mDef.format.video.eColorFormat;
            formatParams->xFramerate = 0;
        }

        return OMX_ErrorNone;
    }

    case OMX_IndexParamVideoProfileLevelQuerySupported:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *profileLevel =
            (OMX_VIDEO_PARAM_PROFILELEVELTYPE *) params;

        if (profileLevel->nPortIndex != 0) {  // Input port only
            ALOGE("Invalid port index: %d", profileLevel->nPortIndex);
            return OMX_ErrorUnsupportedIndex;
        }

        size_t index = profileLevel->nProfileIndex;
        if (mMode == MODE_H263) {
            size_t nProfileLevels =
                sizeof(kH263ProfileLevels) / sizeof(kH263ProfileLevels[0]);
            if (index >= nProfileLevels) {
                return OMX_ErrorNoMore;
            }

            profileLevel->eProfile = kH263ProfileLevels[index].mProfile;
            profileLevel->eLevel = kH263ProfileLevels[index].mLevel;
        } else {
            size_t nProfileLevels =
                sizeof(kM4VProfileLevels) / sizeof(kM4VProfileLevels[0]);
            if (index >= nProfileLevels) {
                return OMX_ErrorNoMore;
            }

            profileLevel->eProfile = kM4VProfileLevels[index].mProfile;
            profileLevel->eLevel = kM4VProfileLevels[index].mLevel;
        }
        return OMX_ErrorNone;
    }

    case OMX_IndexParamEnableAndroidBuffers:
    {
        EnableAndroidNativeBuffersParams *peanbp = (EnableAndroidNativeBuffersParams *)params;
        peanbp->enable = iUseAndroidNativeBuffer[OMX_DirOutput];
        ALOGI("internalGetParameter, OMX_IndexParamEnableAndroidBuffers %d",peanbp->enable);
        return OMX_ErrorNone;
    }

    case OMX_IndexParamGetAndroidNativeBuffer:
    {
        GetAndroidNativeBufferUsageParams *pganbp;

        pganbp = (GetAndroidNativeBufferUsageParams *)params;
        pganbp->nUsage = GRALLOC_USAGE_SW_READ_OFTEN |GRALLOC_USAGE_SW_WRITE_OFTEN;
        ALOGI("internalGetParameter, OMX_IndexParamGetAndroidNativeBuffer 0x%x",pganbp->nUsage);
        return OMX_ErrorNone;
    }

    default:
        return SprdSimpleOMXComponent::internalGetParameter(index, params);
    }
}

OMX_ERRORTYPE SoftSPRDMPEG4::internalSetParameter(
    OMX_INDEXTYPE index, const OMX_PTR params) {
    switch (index) {
    case OMX_IndexParamStandardComponentRole:
    {
        const OMX_PARAM_COMPONENTROLETYPE *roleParams =
            (const OMX_PARAM_COMPONENTROLETYPE *)params;

        if (mMode == MODE_MPEG4) {
            if (strncmp((const char *)roleParams->cRole,
                        "video_decoder.mpeg4",
                        OMX_MAX_STRINGNAME_SIZE - 1)) {
                return OMX_ErrorUndefined;
            }
        } else {
            if (strncmp((const char *)roleParams->cRole,
                        "video_decoder.h263",
                        OMX_MAX_STRINGNAME_SIZE - 1)) {
                return OMX_ErrorUndefined;
            }
        }

        return OMX_ErrorNone;
    }

    case OMX_IndexParamVideoPortFormat:
    {
        OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams =
            (OMX_VIDEO_PARAM_PORTFORMATTYPE *)params;

        if (formatParams->nPortIndex > 1) {
            return OMX_ErrorUndefined;
        }

        if (formatParams->nIndex != 0) {
            return OMX_ErrorNoMore;
        }

        return OMX_ErrorNone;
    }

    case OMX_IndexParamEnableAndroidBuffers:
    {
        EnableAndroidNativeBuffersParams *peanbp = (EnableAndroidNativeBuffersParams *)params;
//        PortInfo *pOutPort = editPortInfo(kOutputPortIndex);
        if (peanbp->enable == OMX_FALSE) {
            ALOGI("internalSetParameter, disable AndroidNativeBuffer");
            iUseAndroidNativeBuffer[OMX_DirOutput] = OMX_FALSE;

//            pOutPort->mDef.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
        } else {
            ALOGI("internalSetParameter, enable AndroidNativeBuffer");
            iUseAndroidNativeBuffer[OMX_DirOutput] = OMX_TRUE;

//            pOutPort->mDef.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
        }
        return OMX_ErrorNone;
    }

    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE *defParams =
            (OMX_PARAM_PORTDEFINITIONTYPE *)params;

        if (defParams->nPortIndex > 1
                || defParams->nSize
                != sizeof(OMX_PARAM_PORTDEFINITIONTYPE)) {
            return OMX_ErrorUndefined;
        }

        PortInfo *port = editPortInfo(defParams->nPortIndex);

        if (defParams->nBufferSize != port->mDef.nBufferSize) {
            CHECK_GE(defParams->nBufferSize, port->mDef.nBufferSize);
            port->mDef.nBufferSize = defParams->nBufferSize;
        }

        if (defParams->nBufferCountActual
                != port->mDef.nBufferCountActual) {
            CHECK_GE(defParams->nBufferCountActual,
                     port->mDef.nBufferCountMin);

            port->mDef.nBufferCountActual = defParams->nBufferCountActual;
        }

        memcpy(&port->mDef.format.video, &defParams->format.video, sizeof(OMX_VIDEO_PORTDEFINITIONTYPE));

        if(defParams->nPortIndex == 1) {
            port->mDef.format.video.nStride = port->mDef.format.video.nFrameWidth;
            port->mDef.format.video.nSliceHeight = port->mDef.format.video.nFrameHeight;
            mWidth = port->mDef.format.video.nFrameWidth;
            mHeight = port->mDef.format.video.nFrameHeight;
            mCropRight = mWidth - 1;
            mCropBottom = mHeight - 1;
            port->mDef.nBufferSize =(((mWidth + 15) & -16)* ((mHeight + 15) & -16) * 3) / 2;
        }

        if (!((mWidth < 1280 && mHeight < 720) || (mWidth < 720 && mHeight < 1280))) {
            PortInfo *port = editPortInfo(kInputPortIndex);
            if(port->mDef.nBufferSize < 384*1024)
                port->mDef.nBufferSize = 384*1024;
        } else if (!((mWidth < 720 && mHeight < 480) || (mWidth < 480 && mHeight < 720))) {
            PortInfo *port = editPortInfo(kInputPortIndex);
            if(port->mDef.nBufferSize < 256*1024)
                port->mDef.nBufferSize = 256*1024;
        }

        return OMX_ErrorNone;
    }

    default:
        return SprdSimpleOMXComponent::internalSetParameter(index, params);
    }
}

OMX_ERRORTYPE SoftSPRDMPEG4::internalUseBuffer(
    OMX_BUFFERHEADERTYPE **header,
    OMX_U32 portIndex,
    OMX_PTR appPrivate,
    OMX_U32 size,
    OMX_U8 *ptr,
    BufferPrivateStruct* bufferPrivate) {

    *header = new OMX_BUFFERHEADERTYPE;
    (*header)->nSize = sizeof(OMX_BUFFERHEADERTYPE);
    (*header)->nVersion.s.nVersionMajor = 1;
    (*header)->nVersion.s.nVersionMinor = 0;
    (*header)->nVersion.s.nRevision = 0;
    (*header)->nVersion.s.nStep = 0;
    (*header)->pBuffer = ptr;
    (*header)->nAllocLen = size;
    (*header)->nFilledLen = 0;
    (*header)->nOffset = 0;
    (*header)->pAppPrivate = appPrivate;
    (*header)->pPlatformPrivate = NULL;
    (*header)->pInputPortPrivate = NULL;
    (*header)->pOutputPortPrivate = NULL;
    (*header)->hMarkTargetComponent = NULL;
    (*header)->pMarkData = NULL;
    (*header)->nTickCount = 0;
    (*header)->nTimeStamp = 0;
    (*header)->nFlags = 0;
    (*header)->nOutputPortIndex = portIndex;
    (*header)->nInputPortIndex = portIndex;

    if(portIndex == OMX_DirOutput) {
        (*header)->pOutputPortPrivate = new BufferCtrlStruct;
        CHECK((*header)->pOutputPortPrivate != NULL);
        BufferCtrlStruct* pBufCtrl= (BufferCtrlStruct*)((*header)->pOutputPortPrivate);
        pBufCtrl->iRefCount = 1; //init by1
        pBufCtrl->pMem = NULL;
        pBufCtrl->bufferFd = 0;
        pBufCtrl->phyAddr = 0;
        pBufCtrl->bufferSize = 0;
        pBufCtrl->id = -1;
    }

    PortInfo *port = editPortInfo(portIndex);

    port->mBuffers.push();

    BufferInfo *buffer =
        &port->mBuffers.editItemAt(port->mBuffers.size() - 1);
    ALOGI("internalUseBuffer, header=0x%p, pBuffer=0x%p, size=%d",*header, ptr, size);
    buffer->mHeader = *header;
    buffer->mOwnedByUs = false;

    if (port->mBuffers.size() == port->mDef.nBufferCountActual) {
        port->mDef.bPopulated = OMX_TRUE;
        checkTransitions();
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE SoftSPRDMPEG4::getConfig(
    OMX_INDEXTYPE index, OMX_PTR params) {
    switch (index) {
    case OMX_IndexConfigCommonOutputCrop:
    {
        OMX_CONFIG_RECTTYPE *rectParams = (OMX_CONFIG_RECTTYPE *)params;

        if (rectParams->nPortIndex != 1) {
            return OMX_ErrorUndefined;
        }

        rectParams->nLeft = mCropLeft;
        rectParams->nTop = mCropTop;
        rectParams->nWidth = mCropRight - mCropLeft + 1;
        rectParams->nHeight = mCropBottom - mCropTop + 1;

        return OMX_ErrorNone;
    }

    default:
        return OMX_ErrorUnsupportedIndex;
    }
}

OMX_ERRORTYPE SoftSPRDMPEG4::setConfig(
    OMX_INDEXTYPE index, const OMX_PTR params) {
    switch (index) {
    case OMX_IndexConfigThumbnailMode:
    {
        OMX_BOOL *pEnable = (OMX_BOOL *)params;

        if (*pEnable == OMX_TRUE) {
            mThumbnailMode = OMX_TRUE;
        }

        ALOGI("setConfig, mThumbnailMode = %d", mThumbnailMode);

        if (mThumbnailMode) {
            PortInfo *pInPort = editPortInfo(OMX_DirInput);
            PortInfo *pOutPort = editPortInfo(OMX_DirOutput);
            pInPort->mDef.nBufferCountActual = 2;
            pOutPort->mDef.nBufferCountActual = 2;
            pOutPort->mDef.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;
        }
        return OMX_ErrorNone;
    }

    default:
        return SprdSimpleOMXComponent::setConfig(index, params);
    }
}

void SoftSPRDMPEG4::onQueueFilled(OMX_U32 portIndex) {
    if (mSignalledError || mOutputPortSettingsChange != NONE) {
        return;
    }

    if (mEOSStatus == OUTPUT_FRAMES_FLUSHED) {
        return;
    }

    List<BufferInfo *> &inQueue = getPortQueue(0);
    List<BufferInfo *> &outQueue = getPortQueue(1);

    while (!mStopDecode && (mEOSStatus != INPUT_DATA_AVAILABLE || !inQueue.empty())
            && outQueue.size() != 0) {

        if (mEOSStatus == INPUT_EOS_SEEN) {
            drainAllOutputBuffers();
            return;
        }

        BufferInfo *inInfo = *inQueue.begin();
        OMX_BUFFERHEADERTYPE *inHeader = inInfo->mHeader;

//        PortInfo *port = editPortInfo(1);
        List<BufferInfo *>::iterator itBuffer = outQueue.begin();
        OMX_BUFFERHEADERTYPE *outHeader = NULL;
        BufferCtrlStruct *pBufCtrl = NULL;
        size_t count = 0;
        do {
            if(count >= outQueue.size()) {
                ALOGI("onQueueFilled, get outQueue buffer, return, count=%zd, queue_size=%d",count, outQueue.size());
                return;
            }

            outHeader = (*itBuffer)->mHeader;
            pBufCtrl= (BufferCtrlStruct*)(outHeader->pOutputPortPrivate);
            if(pBufCtrl == NULL) {
                ALOGE("onQueueFilled, pBufCtrl == NULL, fail");
                notify(OMX_EventError, OMX_ErrorUndefined, 0, NULL);
                mSignalledError = true;
                return;
            }

            itBuffer++;
            count++;
        }
        while(pBufCtrl->iRefCount > 0);

        ALOGI("%s, %d, outHeader:0x%p, inHeader: 0x%p, len: %d, time: %lld, EOS: %d, cfg:%d", __FUNCTION__, __LINE__,outHeader,
            inHeader, inHeader->nFilledLen,inHeader->nTimeStamp,inHeader->nFlags & OMX_BUFFERFLAG_EOS,inHeader->nFlags & OMX_BUFFERFLAG_CODECCONFIG);

        if (inHeader->nFlags & OMX_BUFFERFLAG_EOS) {
            mEOSStatus = INPUT_EOS_SEEN; //the last frame size may be not zero, it need to be decoded.
        }

        uint8_t *bitstream = inHeader->pBuffer + inHeader->nOffset;

        if (!mInitialized) {
            uint8_t *vol_data[1];
            int32_t vol_size = 0;

            vol_data[0] = NULL;

            if (inHeader->nFlags & OMX_BUFFERFLAG_CODECCONFIG) {
                vol_data[0] = bitstream;
                vol_size = inHeader->nFilledLen;
                if (vol_size <= MPEG4_VOL_HEADER_SIZE && mPVolHeaderSize == 0) {
                    memcpy(mPVolHeader, bitstream, vol_size);
                    mPVolHeaderSize = vol_size;
                }
            } else if (mPVolHeaderSize > 0) {
                vol_data[0] = mPVolHeader;
                vol_size = mPVolHeaderSize;
            }

            MP4DecodingMode mode =
                (mMode == MODE_MPEG4) ? MPEG4_MODE : H263_MODE;

            MMDecVideoFormat video_format;

            video_format.i_extra = vol_size;
            if(video_format.i_extra > 0) {
                video_format.p_extra =(uint8 *)(vol_data[0]);
#if 0
                dump_bs((uint8_t *)video_format.p_extra,  video_format.i_extra);
#endif
            } else {
                video_format.i_extra = 0;
                video_format.p_extra = NULL;
                video_format.p_extra_phy = 0;
            }

            if(mode == H263_MODE) {
                video_format.video_std = ITU_H263;
            } else {
                video_format.video_std = MPEG4;
            }

            video_format.frame_width = mWidth;
            video_format.frame_height = mHeight;

            PortInfo *pOutPort = editPortInfo(OMX_DirOutput);
            if (pOutPort->mDef.format.video.eColorFormat == OMX_COLOR_FormatYUV420Planar) {
                video_format.yuv_format = YUV420P_YU12;
            } else {
                video_format.yuv_format = YUV420SP_NV12;
            }

            MMDecRet ret = (*mMP4DecVolHeader)(mHandle, &video_format);

            ALOGI("%s, %d, MP4DecVolHeader, ret: %d, width: %d, height: %d, yuv_format: 0x%x", __FUNCTION__, __LINE__,
                ret, video_format.frame_width, video_format.frame_height, video_format.yuv_format);

            if (ret != MMDEC_OK) {
                ALOGW("MP4DecVolHeader failed. Unsupported content?");

                notify(OMX_EventError, OMX_ErrorUndefined, 0, NULL);
                mSignalledError = true;
                return;
            }

            mInitialized = true;

            if (mode == MPEG4_MODE) {
                int32_t buf_width, buf_height;

                (*mMp4GetBufferDimensions)(mHandle, &buf_width, &buf_height);
                if (!((buf_width <= mMaxWidth&& buf_height <= mMaxHeight) || (buf_width <= mMaxHeight && buf_height <= mMaxWidth))) {
                    ALOGE("[%d,%d] is out of range [%d, %d], failed to support this format.",buf_width, buf_height, mMaxWidth, mMaxHeight);
                    notify(OMX_EventError, OMX_ErrorFormatNotDetected, 0, NULL);
                    mSignalledError = true;
                    return;
                }

                if (portSettingsChanged()) {
                    return;
                }
            }

            if (inHeader->nFlags & OMX_BUFFERFLAG_CODECCONFIG) {
                inInfo->mOwnedByUs = false;
                inQueue.erase(inQueue.begin());
                inInfo = NULL;
                notifyEmptyBufferDone(inHeader);
                inHeader = NULL;
            }

            continue;
        }

        if(inHeader->nFilledLen == 0) {
            inInfo->mOwnedByUs = false;
            inQueue.erase(inQueue.begin());
            inInfo = NULL;
            notifyEmptyBufferDone(inHeader);
            inHeader = NULL;

            continue;
        }

        outHeader->nTimeStamp = inHeader->nTimeStamp;

        if (mThumbnailMode && !mFramesConfigured) {
            (*mMP4DecSetReferenceYUV)(mHandle, outHeader->pBuffer);
            mFramesConfigured = true;
        }

        uint32_t bufferSize = inHeader->nFilledLen;

        MMDecInput dec_in;
        MMDecOutput dec_out;

        GraphicBufferMapper &mapper = GraphicBufferMapper::get();
        if(iUseAndroidNativeBuffer[OMX_DirOutput]) {
            OMX_PARAM_PORTDEFINITIONTYPE *def = &editPortInfo(kOutputPortIndex)->mDef;
            int width = def->format.video.nStride;
            int height = def->format.video.nSliceHeight;
            Rect bounds(width, height);
            void *vaddr;
            int usage;

            usage = GRALLOC_USAGE_SW_READ_OFTEN|GRALLOC_USAGE_SW_WRITE_OFTEN;

            if(mapper.lock((const native_handle_t*)outHeader->pBuffer, usage, bounds, &vaddr)) {
                ALOGE("onQueueFilled, mapper.lock fail 0x%p",outHeader->pBuffer);
                return ;
            }
            ALOGV("%s, %d, pBuffer: 0x%p, vaddr: 0x%p", __FUNCTION__, __LINE__, outHeader->pBuffer,vaddr);
            (*mMP4DecSetCurRecPic)(mHandle,(uint8*)(vaddr), NULL, (void *)(outHeader));
        } else {
            (*mMP4DecSetCurRecPic)(mHandle,outHeader->pBuffer, NULL, (void *)(outHeader));
        }

        dec_in.pStream= bitstream;
        dec_in.pStream_phy= 0;
        dec_in.dataLen = bufferSize;
        dec_in.beLastFrm = 0;
        dec_in.expected_IVOP = mNeedIVOP;
        dec_in.beDisplayed = 1;
        dec_in.err_pkt_num = 0;
        dec_in.nTimeStamp = (uint64)(inHeader->nTimeStamp);

        dec_out.VopPredType = -1;
        dec_out.frameEffective = 0;

#if 0
        dump_bs((uint8_t *)dec_in.pStream,  dec_in.dataLen);
#endif

        int64_t start_decode = systemTime();
        MMDecRet decRet =	(*mMP4DecDecode)( mHandle, &dec_in,&dec_out);
        int64_t end_decode = systemTime();
        ALOGI("%s, %d, decRet: %d, %dms, frameEffective: %d, pOutFrameY: %p, pBufferHeader: %p, needIVOP: %d, error_flag: %0x",
              __FUNCTION__, __LINE__, decRet, (unsigned int)((end_decode-start_decode) / 1000000L),dec_out.frameEffective, dec_out.pOutFrameY, dec_out.pBufferHeader,mNeedIVOP, mHandle->g_mpeg4_dec_err_flag);

        if(iUseAndroidNativeBuffer[OMX_DirOutput]) {
            if(mapper.unlock((const native_handle_t*)outHeader->pBuffer)) {
                ALOGE("onQueueFilled, mapper.unlock fail %p",outHeader->pBuffer);
            }
        }

        if (decRet == MMDEC_OK || decRet == MMDEC_MEMORY_ALLOCED) {
            int32_t buf_width, buf_height;

            (*mMp4GetBufferDimensions)(mHandle, &buf_width, &buf_height);
            if (!((buf_width <= mMaxWidth&& buf_height <= mMaxHeight) || (buf_width <= mMaxHeight && buf_height <= mMaxWidth))) {
                ALOGE("[%d,%d] is out of range [%d, %d], failed to support this format.",buf_width, buf_height, mMaxWidth, mMaxHeight);
                notify(OMX_EventError, OMX_ErrorFormatNotDetected, 0, NULL);
                mSignalledError = true;
                return;
            }

            if (portSettingsChanged()) {
                return;
            } else if(decRet == MMDEC_MEMORY_ALLOCED) {
                continue;
            }
            mNeedIVOP = false;
        } else if (decRet == MMDEC_FRAME_SEEK_IVOP) {
            inInfo->mOwnedByUs = false;
            inQueue.erase(inQueue.begin());
            inInfo = NULL;
            notifyEmptyBufferDone(inHeader);
            inHeader = NULL;

            continue;
        } else if (decRet == MMDEC_MEMORY_ERROR) {
            ALOGE("failed to allocate memory.");
            notify(OMX_EventError, OMX_ErrorInsufficientResources, 0, NULL);
            mSignalledError = true;
            return;
        } else if (decRet == MMDEC_STREAM_ERROR) {
            ALOGE("failed to decode video frame, stream error");
        } else if (decRet == MMDEC_HW_ERROR)
        {
            ALOGE("failed to decode video frame, hardware error");
        } else if (decRet == MMDEC_NOT_SUPPORTED)
        {
            ALOGE("failed to decode video frame, unsupported");
            notify(OMX_EventError, OMX_ErrorUnsupportedSetting, 0, NULL);
            mSignalledError = true;
            return;
        } else
        {
            ALOGE("now, we don't take care of the decoder return: %d", decRet);
        }

        CHECK_LE(bufferSize, inHeader->nFilledLen);
        inHeader->nOffset += inHeader->nFilledLen - bufferSize;
        inHeader->nFilledLen -= bufferSize;

        if (inHeader->nFilledLen == 0) {
            inInfo->mOwnedByUs = false;
            inQueue.erase(inQueue.begin());
            inInfo = NULL;
            notifyEmptyBufferDone(inHeader);
            inHeader = NULL;
        }

        if (dec_out.frameEffective) {
            outHeader = (OMX_BUFFERHEADERTYPE*)(dec_out.pBufferHeader);
            outHeader->nOffset = 0;
            outHeader->nFilledLen = (mWidth * mHeight * 3) / 2;
            outHeader->nFlags = 0;
            outHeader->nTimeStamp = (OMX_TICKS)(dec_out.pts);
            if(mThumbnailMode) {
                mStopDecode = true;
            }
            ALOGI("%s, %d, dec_out.pBufferHeader: 0x%p, time: %lld, mStopDecode :%d",
                    __FUNCTION__, __LINE__, outHeader, outHeader->nTimeStamp, mStopDecode);
//           dump_yuv(dec_out.pOutFrameY, outHeader->nFilledLen);
        } else {
            continue;
        }


        List<BufferInfo *>::iterator it = outQueue.begin();
//        ALOGI("%s, %d,mHeader=0x%x, outHeader=0x%x", __FUNCTION__, __LINE__,(*it)->mHeader,outHeader );
        while ((*it)->mHeader != outHeader && it != outQueue.end()) {
            ++it;
        }
        CHECK((*it)->mHeader == outHeader);

        BufferInfo *outInfo = *it;
        outInfo->mOwnedByUs = false;
        outQueue.erase(it);
        outInfo = NULL;

        BufferCtrlStruct* pOutBufCtrl= (BufferCtrlStruct*)(outHeader->pOutputPortPrivate);
        pOutBufCtrl->iRefCount++;

        notifyFillBufferDone(outHeader);
        outHeader = NULL;
    }
}

bool SoftSPRDMPEG4::drainAllOutputBuffers() {
    ALOGI("%s, %d", __FUNCTION__, __LINE__);

    List<BufferInfo *> &outQueue = getPortQueue(1);
    BufferInfo *outInfo;
    OMX_BUFFERHEADERTYPE *outHeader;
    void *pBufferHeader;

    while (!outQueue.empty() && mEOSStatus != OUTPUT_FRAMES_FLUSHED) {
        if (mHeadersDecoded &&(*mMP4DecGetLastDspFrm)(mHandle, &pBufferHeader) ) {
            ALOGI("%s, %d, MP4DecGetLastDspFrm, pBufferHeader: %p", __FUNCTION__, __LINE__, pBufferHeader);
            List<BufferInfo *>::iterator it = outQueue.begin();
            while ((*it)->mHeader != (OMX_BUFFERHEADERTYPE*)pBufferHeader && it != outQueue.end()) {
                ++it;
            }
            CHECK((*it)->mHeader == (OMX_BUFFERHEADERTYPE*)pBufferHeader);
            outInfo = *it;
            outQueue.erase(it);
            outHeader = outInfo->mHeader;
            outHeader->nFilledLen = (mWidth * mHeight * 3) / 2;
        } else {
            ALOGI("%s, %d, output EOS", __FUNCTION__, __LINE__);
            outInfo = *outQueue.begin();
            outQueue.erase(outQueue.begin());
            outHeader = outInfo->mHeader;
            outHeader->nTimeStamp = 0;
            outHeader->nFilledLen = 0;
            outHeader->nFlags = OMX_BUFFERFLAG_EOS;
            mEOSStatus = OUTPUT_FRAMES_FLUSHED;
        }

        outInfo->mOwnedByUs = false;
        BufferCtrlStruct* pOutBufCtrl= (BufferCtrlStruct*)(outHeader->pOutputPortPrivate);
        pOutBufCtrl->iRefCount++;
        notifyFillBufferDone(outHeader);
    }

    return true;
}

bool SoftSPRDMPEG4::portSettingsChanged() {
    int32_t disp_width, disp_height;

    (*mMp4GetVideoDimensions)(mHandle, &disp_width, &disp_height);

    int32_t buf_width, buf_height;

    (*mMp4GetBufferDimensions)(mHandle, &buf_width, &buf_height);

    if(disp_width <= 0 || disp_height <= 0 || buf_width <= 0 || buf_height <= 0) {
        ALOGE("%s, %d, erro disp w h: %d %d, buf w h: %d %d", __FUNCTION__, __LINE__,
                disp_width, disp_height, buf_width, buf_height);
        return false;
    }

    CHECK_LE(disp_width, buf_width);
    CHECK_LE(disp_height, buf_height);

    if (mCropRight != disp_width - 1
            || mCropBottom != disp_height - 1) {
        ALOGI("%s, %d, mCropLeft: %d, mCropTop: %d, mCropRight: %d, mCropBottom: %d", __FUNCTION__, __LINE__, mCropLeft, mCropTop, mCropRight, mCropBottom);
        mCropLeft = 0;
        mCropTop = 0;
        mCropRight = disp_width - 1;
        mCropBottom = disp_height - 1;

        notify(OMX_EventPortSettingsChanged,
               1,
               OMX_IndexConfigCommonOutputCrop,
               NULL);
    }

    if (buf_width != mWidth || buf_height != mHeight) {
        ALOGI("%s, %d, disp w h: %d %d, buf w h: %d %d, org w h: %d %d", __FUNCTION__, __LINE__,
                disp_width, disp_height, buf_width, buf_height, mWidth, mHeight);
        mWidth = buf_width;
        mHeight = buf_height;

        updatePortDefinitions();

        (*mMP4DecReleaseRefBuffers)(mHandle);

        mFramesConfigured = false;

        notify(OMX_EventPortSettingsChanged, 1, 0, NULL);
        mOutputPortSettingsChange = AWAITING_DISABLED;
        return true;
    }

    return false;
}

void SoftSPRDMPEG4::onPortFlushCompleted(OMX_U32 portIndex) {
    if (portIndex == 0 && mInitialized) {
        mEOSStatus = INPUT_DATA_AVAILABLE;
        mNeedIVOP = true;
    }
}

void SoftSPRDMPEG4::onPortEnableCompleted(OMX_U32 portIndex, bool enabled) {
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

void SoftSPRDMPEG4::onPortFlushPrepare(OMX_U32 portIndex) {
    if(portIndex == OMX_DirOutput) {
        if( mMP4DecReleaseRefBuffers!=NULL )
            (*mMP4DecReleaseRefBuffers)(mHandle);
    }
}

void SoftSPRDMPEG4::onReset() {
    mSignalledError = false;
    mInitialized = false;
    //avoid process error after stop codec and restart codec when port settings changing.
    mOutputPortSettingsChange = NONE;
}

void SoftSPRDMPEG4::updatePortDefinitions() {
    OMX_PARAM_PORTDEFINITIONTYPE *def = &editPortInfo(kInputPortIndex)->mDef;
    def->format.video.nFrameWidth = mWidth;
    def->format.video.nFrameHeight = mHeight;
    def->format.video.nStride = def->format.video.nFrameWidth;
    def->format.video.nSliceHeight = def->format.video.nFrameHeight;

    def = &editPortInfo(kOutputPortIndex)->mDef;
    def->format.video.nFrameWidth = mWidth;
    def->format.video.nFrameHeight = mHeight;
    def->format.video.nStride = def->format.video.nFrameWidth;
    def->format.video.nSliceHeight = def->format.video.nFrameHeight;

    def->nBufferSize =
        (((def->format.video.nFrameWidth + 15) & -16)
         * ((def->format.video.nFrameHeight + 15) & -16) * 3) / 2;
}

// static
int32_t SoftSPRDMPEG4::extMemoryAllocWrapper(
    void *aUserData, unsigned int extra_mem_size) {
    return static_cast<SoftSPRDMPEG4 *>(aUserData)->extMemoryAlloc(extra_mem_size);
}

int32_t SoftSPRDMPEG4::BindFrameWrapper(
    void *aUserData, void *pHeader, int flag) {
    return static_cast<SoftSPRDMPEG4 *>(aUserData)->VSP_bind_cb(pHeader, flag);
}

int32_t SoftSPRDMPEG4::UnbindFrameWrapper(
    void *aUserData, void *pHeader, int flag) {
    return static_cast<SoftSPRDMPEG4 *>(aUserData)->VSP_unbind_cb(pHeader, flag);
}

int SoftSPRDMPEG4::extMemoryAlloc(unsigned int extra_mem_size) {
    if (mCodecExtraBuffer != NULL)
    {
        free(mCodecExtraBuffer);
        mCodecExtraBuffer = NULL;
    }
    mCodecExtraBuffer = (uint8_t *)malloc(extra_mem_size);

    MMCodecBuffer extra_mem[MAX_MEM_TYPE];

    extra_mem[SW_CACHABLE].common_buffer_ptr = mCodecExtraBuffer;
    extra_mem[SW_CACHABLE].size = extra_mem_size;
    ALOGI("%s, %d, ext_mem: 0x%p, ext_mem_size: %d",
          __FUNCTION__, __LINE__, mCodecExtraBuffer, extra_mem_size);

    (*mMP4DecMemInit)( ((SoftSPRDMPEG4 *)this)->mHandle, extra_mem);

    mHeadersDecoded = true;

    return 1;
}

int SoftSPRDMPEG4::VSP_bind_cb(void *pHeader,int flag) {
    BufferCtrlStruct *pBufCtrl = (BufferCtrlStruct *)(((OMX_BUFFERHEADERTYPE *)pHeader)->pOutputPortPrivate);

    ALOGI("VSP_bind_cb, pBuffer: 0x%p, pHeader: 0x%p; iRefCount=%d",
          ((OMX_BUFFERHEADERTYPE *)pHeader)->pBuffer, pHeader,pBufCtrl->iRefCount);

    pBufCtrl->iRefCount++;
    return 0;
}

int SoftSPRDMPEG4::VSP_unbind_cb(void *pHeader,int flag) {
    BufferCtrlStruct *pBufCtrl = (BufferCtrlStruct *)(((OMX_BUFFERHEADERTYPE *)pHeader)->pOutputPortPrivate);

    ALOGI("VSP_unbind_cb, pBuffer: 0x%p, pHeader: 0x%p; iRefCount=%d",
          ((OMX_BUFFERHEADERTYPE *)pHeader)->pBuffer, pHeader,pBufCtrl->iRefCount);

    if (pBufCtrl->iRefCount  > 0) {
        pBufCtrl->iRefCount--;
    }

    return 0;
}

OMX_ERRORTYPE SoftSPRDMPEG4::getExtensionIndex(
    const char *name, OMX_INDEXTYPE *index) {

    ALOGI("getExtensionIndex, name: %s",name);
    if(strcmp(name, SPRD_INDEX_PARAM_ENABLE_ANB) == 0) {
        ALOGI("getExtensionIndex:%s",SPRD_INDEX_PARAM_ENABLE_ANB);
        *index = (OMX_INDEXTYPE) OMX_IndexParamEnableAndroidBuffers;
        return OMX_ErrorNone;
    } else if (strcmp(name, SPRD_INDEX_PARAM_GET_ANB) == 0) {
        ALOGI("getExtensionIndex:%s",SPRD_INDEX_PARAM_GET_ANB);
        *index = (OMX_INDEXTYPE) OMX_IndexParamGetAndroidNativeBuffer;
        return OMX_ErrorNone;
    }	else if (strcmp(name, SPRD_INDEX_PARAM_USE_ANB) == 0) {
        ALOGI("getExtensionIndex:%s",SPRD_INDEX_PARAM_USE_ANB);
        *index = OMX_IndexParamUseAndroidNativeBuffer2;
        return OMX_ErrorNone;
    }  else if (strcmp(name, SPRD_INDEX_CONFIG_THUMBNAIL_MODE) == 0) {
        ALOGI("getExtensionIndex:%s",SPRD_INDEX_CONFIG_THUMBNAIL_MODE);
        *index = OMX_IndexConfigThumbnailMode;
        return OMX_ErrorNone;
    }

    return OMX_ErrorNotImplemented;
}

bool SoftSPRDMPEG4::openDecoder(const char* libName)
{
    if(mLibHandle) {
        dlclose(mLibHandle);
    }

    ALOGI("openDecoder, lib: %s",libName);

    mLibHandle = dlopen(libName, RTLD_NOW);
    if(mLibHandle == NULL) {
        ALOGE("openDecoder, can't open lib: %s",libName);
        return false;
    }

    mMP4DecSetCurRecPic = (FT_MP4DecSetCurRecPic)dlsym(mLibHandle, "MP4DecSetCurRecPic");
    if(mMP4DecSetCurRecPic == NULL) {
        ALOGE("Can't find MP4DecSetCurRecPic in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }


    mMP4DecInit = (FT_MP4DecInit)dlsym(mLibHandle, "MP4DecInit");
    if(mMP4DecInit == NULL) {
        ALOGE("Can't find MP4DecInit in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mMP4DecVolHeader = (FT_MP4DecVolHeader)dlsym(mLibHandle, "MP4DecVolHeader");
    if(mMP4DecVolHeader == NULL) {
        ALOGE("Can't find MP4DecVolHeader in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mMP4DecMemInit = (FT_MP4DecMemInit)dlsym(mLibHandle, "MP4DecMemInit");
    if(mMP4DecMemInit == NULL) {
        ALOGE("Can't find MP4DecMemInit in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mMP4DecDecode = (FT_MP4DecDecode)dlsym(mLibHandle, "MP4DecDecode");
    if(mMP4DecDecode == NULL) {
        ALOGE("Can't find MP4DecDecode in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mMP4DecRelease = (FT_MP4DecRelease)dlsym(mLibHandle, "MP4DecRelease");
    if(mMP4DecRelease == NULL) {
        ALOGE("Can't find MP4DecRelease in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mMp4GetVideoDimensions = (FT_Mp4GetVideoDimensions)dlsym(mLibHandle, "Mp4GetVideoDimensions");
    if(mMp4GetVideoDimensions == NULL) {
        ALOGE("Can't find Mp4GetVideoDimensions in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mMp4GetBufferDimensions = (FT_Mp4GetBufferDimensions)dlsym(mLibHandle, "Mp4GetBufferDimensions");
    if(mMp4GetBufferDimensions == NULL) {
        ALOGE("Can't find Mp4GetBufferDimensions in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mMP4DecReleaseRefBuffers = (FT_MP4DecReleaseRefBuffers)dlsym(mLibHandle, "MP4DecReleaseRefBuffers");
    if(mMP4DecReleaseRefBuffers == NULL) {
        ALOGE("Can't find MP4DecReleaseRefBuffers in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mMP4DecSetReferenceYUV = (FT_MP4DecSetReferenceYUV)dlsym(mLibHandle, "MP4DecSetReferenceYUV");
    if(mMP4DecSetReferenceYUV == NULL) {
        ALOGE("Can't find MP4DecSetReferenceYUV in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mMP4DecGetLastDspFrm = (FT_MP4DecGetLastDspFrm)dlsym(mLibHandle, "MP4DecGetLastDspFrm");
    if(mMP4DecGetLastDspFrm == NULL) {
        ALOGE("Can't find MP4DecGetLastDspFrm in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mMP4GetCodecCapability = (FT_MP4GetCodecCapability)dlsym(mLibHandle, "MP4GetCodecCapability");
    if(mMP4GetCodecCapability == NULL) {
        ALOGE("Can't find MP4GetCodecCapability in %s",libName);
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
    return new android::SoftSPRDMPEG4(name, callbacks, appData, component);
}

