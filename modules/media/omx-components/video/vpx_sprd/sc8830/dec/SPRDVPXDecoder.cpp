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
#define LOG_TAG "SPRDVPXDecoder"
#include <utils/Log.h>

#include "SPRDVPXDecoder.h"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/IOMX.h>
#include <media/hardware/HardwareAPI.h>
#include <ui/GraphicBufferMapper.h>

#include "gralloc_priv.h"
#include "vpx_dec_api.h"
#include <dlfcn.h>
#include "ion_sprd.h"

//#define VIDEODEC_CURRENT_OPT  /*only open for SAMSUNG currently*/


namespace android {

#define MAX_INSTANCES 8

static int instances = 0;

const static int64_t kConditionEventTimeOutNs = 3000000000LL;

template<class T>
static void InitOMXParams(T *params) {
    params->nSize = sizeof(T);
    params->nVersion.s.nVersionMajor = 1;
    params->nVersion.s.nVersionMinor = 0;
    params->nVersion.s.nRevision = 0;
    params->nVersion.s.nStep = 0;
}

SPRDVPXDecoder::SPRDVPXDecoder(
    const char *name,
    const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData,
    OMX_COMPONENTTYPE **component)
    : SprdSimpleOMXComponent(name, callbacks, appData, component),
      mHandle(new tagVPXHandle),
      mInputBufferCount(0),
      mSetFreqCount(0),
      mFrameWidth(320),
      mFrameHeight(240),
      mStride(mFrameWidth),
      mSliceHeight(mFrameHeight),
      mMaxWidth(352),
      mMaxHeight(288),
      mCropWidth(mFrameWidth),
      mCropHeight(mFrameHeight),
      mGettingPortFormat(OMX_FALSE),
      mEOSStatus(INPUT_DATA_AVAILABLE),
      mOutputPortSettingsChange(NONE),
      mSignalledError(false),
      mIOMMUEnabled(false),
      mIOMMUID(-1),
      mPbuf_inter(NULL),
      mPmem_stream(NULL),
      mPbuf_stream_v(NULL),
      mPbuf_stream_p(0),
      mPbuf_stream_size(0),
      mPmem_extra(NULL),
      mPbuf_extra_v(NULL),
      mPbuf_extra_p(0),
      mPbuf_extra_size(0),
      mLibHandle(NULL),
      mVPXDecSetCurRecPic(NULL),
      mVPXDecInit(NULL),
      mVPXDecDecode(NULL),
      mVPXDecRelease(NULL),
      mVPXGetVideoDimensions(NULL),
      mVPXGetBufferDimensions(NULL),
      mVPXDecReleaseRefBuffers(NULL),
      mVPXDecGetLastDspFrm(NULL),
      mVPXGetCodecCapability(NULL),
      mFrameDecoded(false) {

    ALOGI("Construct SPRDVPXDecoder, this: %p, instances: %d", (void *)this, instances);

    mInitCheck = OMX_ErrorNone;

    initPorts();
    if (!openDecoder("libomx_vpxdec_hw_sprd.so")){
        mInitCheck = OMX_ErrorInsufficientResources;
    }

    if (MemoryHeapIon::IOMMU_is_enabled(ION_MM)) {
        mIOMMUEnabled = true;
        mIOMMUID = ION_MM;
    } else if (MemoryHeapIon::IOMMU_is_enabled(ION_VSP)) {
        mIOMMUEnabled = true;
        mIOMMUID = ION_VSP;
    }
    ALOGI("%s, is IOMMU enabled: %d, ID: %d", __FUNCTION__, mIOMMUEnabled, mIOMMUID);

    if(initDecoder() != OK) {
        mInitCheck = OMX_ErrorInsufficientResources;
    }

    iUseAndroidNativeBuffer[OMX_DirInput] = OMX_FALSE;
    iUseAndroidNativeBuffer[OMX_DirOutput] = OMX_FALSE;

    instances++;
    if (instances > MAX_INSTANCES) {
        ALOGE("instances(%d) are too much, return OMX_ErrorInsufficientResources", instances);
        mInitCheck = OMX_ErrorInsufficientResources;
    }
}

SPRDVPXDecoder::~SPRDVPXDecoder() {

    ALOGI("Destruct SPRDVPXDecoder, this: %p, instances: %d", (void *)this, instances);

    releaseDecoder();

    delete mHandle;
    mHandle = NULL;

    if(mLibHandle) {
        dlclose(mLibHandle);
        mLibHandle = NULL;
    }

    while (mSetFreqCount > 0)
    {
        set_ddr_freq("0");
        mSetFreqCount--;
    }
    instances--;
}

OMX_ERRORTYPE SPRDVPXDecoder::initCheck() const{
    ALOGI("%s, mInitCheck: 0x%x", __FUNCTION__, mInitCheck);
    return mInitCheck;
}


void SPRDVPXDecoder::initPorts() {
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);

    def.nPortIndex = 0;
    def.eDir = OMX_DirInput;
    def.nBufferCountMin = kNumBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.nBufferSize = 1920*1088*3/2/2;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainVideo;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 1;

    def.format.video.cMIMEType = const_cast<char *>(MEDIA_MIMETYPE_VIDEO_VP8);
    def.format.video.pNativeRender = NULL;
    def.format.video.nFrameWidth = mFrameWidth;
    def.format.video.nFrameHeight = mFrameHeight;
    def.format.video.nStride = def.format.video.nFrameWidth;
    def.format.video.nSliceHeight = def.format.video.nFrameHeight;
    def.format.video.nBitrate = 0;
    def.format.video.xFramerate = 0;
    def.format.video.bFlagErrorConcealment = OMX_FALSE;
    def.format.video.eCompressionFormat = OMX_VIDEO_CodingVP8;
    def.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    def.format.video.pNativeWindow = NULL;

    addPort(def);

    def.nPortIndex = 1;
    def.eDir = OMX_DirOutput;
    def.nBufferCountMin = kNumBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainVideo;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 2;

    def.format.video.cMIMEType = const_cast<char *>(MEDIA_MIMETYPE_VIDEO_RAW);
    def.format.video.pNativeRender = NULL;
    def.format.video.nFrameWidth = mFrameWidth;
    def.format.video.nFrameHeight = mFrameHeight;
    def.format.video.nStride = def.format.video.nFrameWidth;
    def.format.video.nSliceHeight = def.format.video.nFrameHeight;
    def.format.video.nBitrate = 0;
    def.format.video.xFramerate = 0;
    def.format.video.bFlagErrorConcealment = OMX_FALSE;
    def.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    def.format.video.eColorFormat = OMX_COLOR_FormatYUV420Planar;
    def.format.video.pNativeWindow = NULL;

    def.nBufferSize =
        (def.format.video.nFrameWidth * def.format.video.nFrameHeight * 3) / 2;

    addPort(def);

    ALOGI("%s, %d, def.nBufferCountMin: %d,def.nBufferCountActual : %d ", __FUNCTION__, __LINE__, def.nBufferCountMin, def.nBufferCountActual );
}

void SPRDVPXDecoder::set_ddr_freq(const char* freq_in_khz)
{
    const char* const set_freq = "/sys/devices/platform/scxx30-dmcfreq.0/devfreq/scxx30-dmcfreq.0/ondemand/set_freq";
    FILE* fp = fopen(set_freq, "w");
    if (fp != NULL)
    {
        fprintf(fp, "%s", freq_in_khz);
        ALOGE("set ddr freq to %skhz", freq_in_khz);
        fclose(fp);
    }
    else
    {
        ALOGE("Failed to open %s", set_freq);
    }
}

void SPRDVPXDecoder::change_ddr_freq()
{
    uint32_t frame_size = mFrameWidth * mFrameHeight;
    const char* ddr_freq;

    if(frame_size > 1280*720)
    {
        ddr_freq = "500000";
    }
#ifdef VIDEODEC_CURRENT_OPT
    else if(frame_size > 864*480)
    {
        ddr_freq = "300000";
    }
#else
    else if(frame_size > 720*576)
    {
        ddr_freq = "400000";
    }
    else if(frame_size > 320*240)
    {
        ddr_freq = "300000";
    }
#endif
    else
    {
        ddr_freq = "200000";
    }
    set_ddr_freq(ddr_freq);
    mSetFreqCount ++;
}

status_t SPRDVPXDecoder::initDecoder() {
    memset(mHandle, 0, sizeof(tagVPXHandle));

    mHandle->userdata = (void *)this;
    mHandle->VSP_bindCb = BindFrameWrapper;
    mHandle->VSP_unbindCb = UnbindFrameWrapper;

    MMCodecBuffer InterMemBfr;
    MMCodecBuffer ExtraMemBfr;
    MMDecVideoFormat VideoFormat;
    unsigned long phy_addr = 0;
    size_t size = 0, size_stream;

    size_stream = ONEFRAME_BITSTREAM_BFR_SIZE;
    if (mIOMMUEnabled) {
        mPmem_stream = new MemoryHeapIon(SPRD_ION_DEV, size_stream, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
    } else {
        mPmem_stream = new MemoryHeapIon(SPRD_ION_DEV, size_stream, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
    }
    if (mPmem_stream->getHeapID() < 0) {
        ALOGE("Failed to alloc bitstream pmem buffer, getHeapID failed");
        return OMX_ErrorInsufficientResources;
    } else {
        int ret;
        if (mIOMMUEnabled) {
            ret = mPmem_stream->get_iova(mIOMMUID, &phy_addr, &size);
        } else {
            ret = mPmem_stream->get_phy_addr_from_ion(&phy_addr, &size);
        }
        if (ret < 0) {
            ALOGE("Failed to alloc bitstream pmem buffer, get phy addr failed");
            return OMX_ErrorInsufficientResources;
        } else {
            mPbuf_stream_v = (uint8_t *)mPmem_stream->getBase();
            mPbuf_stream_p = phy_addr;
            mPbuf_stream_size = size;
        }
    }

    uint32_t size_inter = VP8_DECODER_INTERNAL_BUFFER_SIZE;
    mPbuf_inter = (uint8_t *)malloc(size_inter);
	CHECK(mPbuf_inter != NULL);

    uint32_t size_extra = 8160*9*8+64;
    size_extra += 10*1024;
    if (mIOMMUEnabled) {
        mPmem_extra = new MemoryHeapIon("/dev/ion", size_extra, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
    } else {
        mPmem_extra = new MemoryHeapIon("/dev/ion", size_extra, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
    }
    if (mPmem_extra->getHeapID() < 0) {
        ALOGE("Failed to alloc extra pmem (%d), getHeapID failed", size_extra);
        return OMX_ErrorInsufficientResources;
    } else {
        int ret;
        if (mIOMMUEnabled) {
            ret = mPmem_extra->get_iova(mIOMMUID, &phy_addr, &size);
        } else {
            ret = mPmem_extra->get_phy_addr_from_ion(&phy_addr, &size);
        }
        if (ret < 0) {
            ALOGE("Failed to alloc extra pmem, get phy addr failed");
            return OMX_ErrorInsufficientResources;
        } else {
            mPbuf_extra_v = (uint8_t *)mPmem_extra->getBase();
            mPbuf_extra_p = phy_addr;
        }
    }

    InterMemBfr.common_buffer_ptr = mPbuf_inter;
    InterMemBfr.common_buffer_ptr_phy= 0;
    InterMemBfr.size = size_inter;

    ExtraMemBfr.common_buffer_ptr = mPbuf_extra_v;
    ExtraMemBfr.common_buffer_ptr_phy = mPbuf_extra_p;
    ExtraMemBfr.size = size_extra;

    VideoFormat.yuv_format = YUV420SP_NV12;

    if((*mVPXDecInit)( mHandle, &InterMemBfr, &ExtraMemBfr, &VideoFormat) != MMDEC_OK) {
        ALOGE("Failed to init VPXDEC");
        return OMX_ErrorUndefined;
    }

    if ((*mVPXGetCodecCapability)(mHandle, &mMaxWidth, &mMaxHeight) != MMDEC_OK) {
        ALOGE("Failed to mVPXGetCodecCapability\n");
    }

    return OMX_ErrorNone;
}

void SPRDVPXDecoder::releaseDecoder()
{
	    (*mVPXDecRelease)(mHandle);

	    if (mPbuf_inter != NULL) {
	        free(mPbuf_inter);
	        mPbuf_inter = NULL;
	    }

	    if (mPbuf_stream_v != NULL) {
	        if (mIOMMUEnabled) {
	            mPmem_stream->free_iova(mIOMMUID, mPbuf_stream_p, mPbuf_stream_size);
	        }
	        mPmem_stream.clear();
	        mPbuf_stream_v = NULL;
	        mPbuf_stream_p = 0;
	        mPbuf_stream_size = 0;
	    }

	    if(mPbuf_extra_v != NULL) {
	        if (mIOMMUEnabled) {
	            mPmem_extra->free_iova(mIOMMUID, mPbuf_extra_p, mPbuf_extra_size);
	        }
	        mPmem_extra.clear();
	        mPbuf_extra_v = NULL;
	        mPbuf_extra_p = 0;
	        mPbuf_extra_size = 0;
	    }
}

OMX_ERRORTYPE SPRDVPXDecoder::internalGetParameter(
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
            formatParams->eCompressionFormat = OMX_VIDEO_CodingVP8;
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
        if(mIOMMUEnabled) {
            pganbp->nUsage = GRALLOC_USAGE_SW_READ_OFTEN|GRALLOC_USAGE_SW_WRITE_OFTEN;
        } else {
            pganbp->nUsage = GRALLOC_USAGE_VIDEO_BUFFER|GRALLOC_USAGE_SW_READ_OFTEN|GRALLOC_USAGE_SW_WRITE_OFTEN;
        }
        ALOGI("internalGetParameter, OMX_IndexParamGetAndroidNativeBuffer 0x%x",pganbp->nUsage);
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

        {
            Mutex::Autolock autoLock(mLock);
            memcpy(defParams, &port->mDef, sizeof(port->mDef));
            if (isExecuting() && mOutputPortSettingsChange == NONE) {
                mGettingPortFormat = OMX_TRUE;
            } else {
                mGettingPortFormat = OMX_FALSE;
            }
        }
        return OMX_ErrorNone;
    }

    default:
        return OMX_ErrorUnsupportedIndex;
    }
}

OMX_ERRORTYPE SPRDVPXDecoder::internalSetParameter(
    OMX_INDEXTYPE index, const OMX_PTR params) {
    switch (index) {
    case OMX_IndexParamStandardComponentRole:
    {
        const OMX_PARAM_COMPONENTROLETYPE *roleParams =
            (const OMX_PARAM_COMPONENTROLETYPE *)params;

        if (strncmp((const char *)roleParams->cRole,
                    "video_decoder.vp8",
                    OMX_MAX_STRINGNAME_SIZE - 1)) {
            return OMX_ErrorUndefined;
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
        PortInfo *pOutPort = editPortInfo(kOutputPortIndex);
        if (peanbp->enable == OMX_FALSE) {
            ALOGI("internalSetParameter, disable AndroidNativeBuffer");
            iUseAndroidNativeBuffer[OMX_DirOutput] = OMX_FALSE;

            pOutPort->mDef.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
        } else {
            ALOGI("internalSetParameter, enable AndroidNativeBuffer");
            iUseAndroidNativeBuffer[OMX_DirOutput] = OMX_TRUE;

            pOutPort->mDef.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
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

        uint32_t oldWidth = port->mDef.format.video.nFrameWidth;
        uint32_t oldHeight = port->mDef.format.video.nFrameHeight;
        uint32_t newWidth = defParams->format.video.nFrameWidth;
        uint32_t newHeight = defParams->format.video.nFrameHeight;

        ALOGI("%s, old wh:%d %d, new wh:%d %d", __FUNCTION__, oldWidth, oldHeight, newWidth, newHeight);

        memcpy(&port->mDef.format.video, &defParams->format.video, sizeof(OMX_VIDEO_PORTDEFINITIONTYPE));

        if((oldWidth != newWidth || oldHeight != newHeight)) {
            if (defParams->nPortIndex == kOutputPortIndex) {
                mFrameWidth = newWidth;
                mFrameHeight = newHeight;
                mStride = ((newWidth + 15) & -16);
                mSliceHeight = ((newHeight + 15) & -16);

                ALOGI("%s, mFrameWidth %d, mFrameHeight %d, mStride %d, mSliceHeight %d", __FUNCTION__,
                      mFrameWidth, mFrameHeight, mStride, mSliceHeight);

                updatePortDefinitions(true, true);
                change_ddr_freq();
            } else {
                port->mDef.format.video.nFrameWidth = newWidth;
                port->mDef.format.video.nFrameHeight = newHeight;
            }
        }

        return OMX_ErrorNone;
    }

    default:
        return SprdSimpleOMXComponent::internalSetParameter(index, params);
    }
}

OMX_ERRORTYPE SPRDVPXDecoder::internalUseBuffer(
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
        pBufCtrl->id = mIOMMUID;
        if(bufferPrivate != NULL) {
            pBufCtrl->pMem = ((BufferPrivateStruct*)bufferPrivate)->pMem;
            pBufCtrl->phyAddr = ((BufferPrivateStruct*)bufferPrivate)->phyAddr;
            pBufCtrl->bufferSize = ((BufferPrivateStruct*)bufferPrivate)->bufferSize;
            pBufCtrl->bufferFd = 0;
        } else {
            if (mIOMMUEnabled) {
                unsigned long picPhyAddr = 0;
                size_t bufferSize = 0;
                native_handle_t *pNativeHandle = (native_handle_t *)((*header)->pBuffer);
                struct private_handle_t *private_h = (struct private_handle_t *)pNativeHandle;
                MemoryHeapIon::Get_iova(mIOMMUID, private_h->share_fd, &picPhyAddr, &bufferSize);

                pBufCtrl->pMem = NULL;
                pBufCtrl->bufferFd = private_h->share_fd;
                pBufCtrl->phyAddr = picPhyAddr;
                pBufCtrl->bufferSize = bufferSize;
            } else {
                pBufCtrl->pMem = NULL;
                pBufCtrl->bufferFd = 0;
                pBufCtrl->phyAddr = 0;
                pBufCtrl->bufferSize = 0;
            }
        }
    }

    PortInfo *port = editPortInfo(portIndex);

    port->mBuffers.push();

    BufferInfo *buffer =
        &port->mBuffers.editItemAt(port->mBuffers.size() - 1);
    ALOGI("internalUseBuffer, header=%p, pBuffer=%p, size=%d",*header, ptr, size);
    buffer->mHeader = *header;
    buffer->mOwnedByUs = false;

    if (port->mBuffers.size() == port->mDef.nBufferCountActual) {
        port->mDef.bPopulated = OMX_TRUE;
        checkTransitions();
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDVPXDecoder::allocateBuffer(
    OMX_BUFFERHEADERTYPE **header,
    OMX_U32 portIndex,
    OMX_PTR appPrivate,
    OMX_U32 size) {
    switch(portIndex)
    {
    case OMX_DirInput:
        return SprdSimpleOMXComponent::allocateBuffer(header, portIndex, appPrivate, size);

    case OMX_DirOutput:
    {

        MemoryHeapIon* pMem = NULL;
        unsigned long phyAddr = 0;
        size_t bufferSize = 0;
        OMX_U8* pBuffer = NULL;
        size_t size64word = (size + 1024*4 - 1) & ~(1024*4 - 1);

        if (mIOMMUEnabled) {
            pMem = new MemoryHeapIon(SPRD_ION_DEV, size64word, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
        } else {
            pMem = new MemoryHeapIon(SPRD_ION_DEV, size64word, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
        }

        if(pMem->getHeapID() < 0) {
            ALOGE("Failed to alloc outport pmem buffer");
            return OMX_ErrorInsufficientResources;
        }

        if (mIOMMUEnabled) {
            if(pMem->get_iova(mIOMMUID, &phyAddr, &bufferSize)) {
                ALOGE("get_mm_iova fail");
                return OMX_ErrorInsufficientResources;
            }
        } else {
            if(pMem->get_phy_addr_from_ion(&phyAddr, &bufferSize)) {
                ALOGE("get_phy_addr_from_ion fail");
                return OMX_ErrorInsufficientResources;
            }
        }

        pBuffer = (OMX_U8 *)(pMem->getBase());
        BufferPrivateStruct* bufferPrivate = new BufferPrivateStruct();
        bufferPrivate->pMem = pMem;
        bufferPrivate->phyAddr = phyAddr;
        bufferPrivate->bufferSize = bufferSize;
        ALOGI("allocateBuffer, allocate buffer from pmem, pBuffer: %p, phyAddr: 0x%lx, size: %zd", pBuffer, phyAddr, bufferSize);

        SprdSimpleOMXComponent::useBuffer(header, portIndex, appPrivate, (OMX_U32)bufferSize, pBuffer, bufferPrivate);
        delete bufferPrivate;

        return OMX_ErrorNone;
    }

    default:
        return OMX_ErrorUnsupportedIndex;

    }
}

OMX_ERRORTYPE SPRDVPXDecoder::freeBuffer(
    OMX_U32 portIndex,
    OMX_BUFFERHEADERTYPE *header) {
    switch(portIndex)
    {
    case OMX_DirInput:
        return SprdSimpleOMXComponent::freeBuffer(portIndex, header);

    case OMX_DirOutput:
    {
        BufferCtrlStruct* pBufCtrl= (BufferCtrlStruct*)(header->pOutputPortPrivate);
        if(pBufCtrl != NULL) {
            if(pBufCtrl->pMem != NULL) {
                ALOGI("freeBuffer, phyAddr: 0x%lx", pBufCtrl->phyAddr);
                if (mIOMMUEnabled) {
                    pBufCtrl->pMem->free_iova(mIOMMUID, pBufCtrl->phyAddr, pBufCtrl->bufferSize);
                }
                pBufCtrl->pMem.clear();
            }
            return SprdSimpleOMXComponent::freeBuffer(portIndex, header);
        } else {
            ALOGE("freeBuffer, pBufCtrl==NULL");
            return OMX_ErrorUndefined;
        }
    }

    default:
        return OMX_ErrorUnsupportedIndex;
    }
}

void SPRDVPXDecoder::onQueueFilled(OMX_U32 portIndex) {
    if (mSignalledError || mOutputPortSettingsChange != NONE) {
        return;
    }

    if (mEOSStatus == OUTPUT_FRAMES_FLUSHED) {
        return;
    }

    List<BufferInfo *> &inQueue = getPortQueue(0);
    List<BufferInfo *> &outQueue = getPortQueue(1);

    while ((mEOSStatus != INPUT_DATA_AVAILABLE || !inQueue.empty())
            && !outQueue.empty()) {

        if (mEOSStatus == INPUT_EOS_SEEN && mFrameDecoded) {
            drainAllOutputBuffers();
            return;
        }

        BufferInfo *inInfo = *inQueue.begin();
        OMX_BUFFERHEADERTYPE *inHeader = inInfo->mHeader;

        List<BufferInfo *>::iterator itBuffer = outQueue.begin();
        OMX_BUFFERHEADERTYPE *outHeader = NULL;
        BufferCtrlStruct *pBufCtrl = NULL;
        size_t count = 0;
        do {
            if(count >= outQueue.size()) {
                ALOGV("%s, %d, get outQueue buffer fail, return, count=%zd, queue_size=%d",__FUNCTION__, __LINE__, count, outQueue.size());
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

        ALOGV("%s, %d, outHeader:%p, inHeader: %p, len: %d, nOffset: %d, time: %lld, EOS: %d",
              __FUNCTION__, __LINE__,outHeader,inHeader, inHeader->nFilledLen,inHeader->nOffset, inHeader->nTimeStamp,inHeader->nFlags & OMX_BUFFERFLAG_EOS);

        mFrameDecoded = false;
        if (inHeader->nFlags & OMX_BUFFERFLAG_EOS) {
            mEOSStatus = INPUT_EOS_SEEN; //the last frame size may be not zero, it need to be decoded.
        }

        if(inHeader->nFilledLen == 0) {
            mFrameDecoded = true;
            inInfo->mOwnedByUs = false;
            inQueue.erase(inQueue.begin());
            inInfo = NULL;
            notifyEmptyBufferDone(inHeader);
            inHeader = NULL;
            continue;
        }

        outHeader->nTimeStamp = inHeader->nTimeStamp;

        MMDecInput dec_in;
        MMDecOutput dec_out;

        uint8_t *bitstream = inHeader->pBuffer + inHeader->nOffset;
        uint32_t bufferSize = inHeader->nFilledLen;

        if (mPbuf_stream_v != NULL) {
            memcpy(mPbuf_stream_v, bitstream, bufferSize);
        }
        dec_in.pStream=  mPbuf_stream_v;
        dec_in.pStream_phy= mPbuf_stream_p;
        dec_in.dataLen = bufferSize;
        dec_in.beLastFrm = 0;
        dec_in.expected_IVOP = 0;
        dec_in.beDisplayed = 1;
        dec_in.err_pkt_num = 0;

        dec_out.frameEffective = 0;

        unsigned long picPhyAddr = 0;

        pBufCtrl= (BufferCtrlStruct*)(outHeader->pOutputPortPrivate);
        if(pBufCtrl->phyAddr != 0) {
            picPhyAddr = pBufCtrl->phyAddr;
        } else {
            if (mIOMMUEnabled) {
                ALOGE("onQueueFilled, pBufCtrl->phyAddr == 0, fail");
                notify(OMX_EventError, OMX_ErrorUndefined, 0, NULL);
                mSignalledError = true;
                return;
            } else {
                native_handle_t *pNativeHandle = (native_handle_t *)outHeader->pBuffer;
                struct private_handle_t *private_h = (struct private_handle_t *)pNativeHandle;
                size_t bufferSize = 0;
                MemoryHeapIon::Get_phy_addr_from_ion(private_h->share_fd, &picPhyAddr, &bufferSize);
                pBufCtrl->phyAddr = picPhyAddr;
            }
        }

//    ALOGI("%s, %d, header: %0x, mPictureSize: %d", __FUNCTION__, __LINE__, header_tmp, mPictureSize);
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
                ALOGI("onQueueFilled, mapper.lock fail %p",outHeader->pBuffer);
                return ;
            }
//    	ALOGI("%s, %d, pBuffer: 0x%x, vaddr: 0x%x", __FUNCTION__, __LINE__, outHeader->pBuffer,vaddr);
            uint8_t *yuv = (uint8_t *)((uint8_t *)vaddr + outHeader->nOffset);
            ALOGV("%s, %d, yuv: %p,outHeader->pBuffer: %p, outHeader->nOffset: %d, outHeader->nFlags: %d, outHeader->nTimeStamp: %lld",
                  __FUNCTION__, __LINE__, yuv, outHeader->pBuffer, outHeader->nOffset, outHeader->nFlags, outHeader->nTimeStamp);
            (*mVPXDecSetCurRecPic)(mHandle, yuv, (uint8_t *)picPhyAddr, (void *)outHeader);
        } else {
            (*mVPXDecSetCurRecPic)(mHandle, outHeader->pBuffer, (uint8_t *)picPhyAddr, (void *)outHeader);
        }

//        dump_bs( dec_in.pStream, dec_in.dataLen);

        MMDecRet decRet = (*mVPXDecDecode)(mHandle, &dec_in,&dec_out);
        ALOGI("%s, %d, decRet: %d, dec_out.frameEffective: %d", __FUNCTION__, __LINE__, decRet, dec_out.frameEffective);

        if(iUseAndroidNativeBuffer[OMX_DirOutput]) {
            if(mapper.unlock((const native_handle_t*)outHeader->pBuffer)) {
                ALOGI("onQueueFilled, mapper.unlock fail %p",outHeader->pBuffer);
            }
        }

        if (decRet == MMDEC_OK || decRet == MMDEC_MEMORY_ALLOCED) {
            int32_t disp_width, disp_height;
            int32_t buf_width, buf_height;

            (*mVPXGetBufferDimensions)(mHandle, &buf_width, &buf_height);

            if (!((buf_width<= mMaxWidth&& buf_height<= mMaxHeight)
                    || (buf_width <= mMaxHeight && buf_height <= mMaxWidth))) {
                ALOGE("[%d,%d] is out of range [%d, %d], failed to support this format.",
                      buf_width, buf_height, mMaxWidth, mMaxHeight);
                notify(OMX_EventError, OMX_ErrorFormatNotDetected, 0, NULL);
                mSignalledError = true;
                return;
            }

            (*mVPXGetVideoDimensions)(mHandle, &disp_width, &disp_height);

            if (disp_width != mFrameWidth ||
                    disp_height != mFrameHeight ||
                    buf_width != mStride ||
                    buf_height != mSliceHeight) {
                Mutex::Autolock autoLock(mLock);
                if (mGettingPortFormat == OMX_TRUE) {
                    ALOGI("%s, waiting for get crop parameter done", __FUNCTION__);
                    status_t err = mCondition.waitRelative(mLock, kConditionEventTimeOutNs);
                    if (err != OK) {
                        ALOGE("Timed out waiting for mCondition signal!");
                    }
                }
                ALOGI("%s, OutputPortSettingsChange, disp wh %dx%d -> %dx%d, buf wh %dx%d -> %dx%d",
                    __FUNCTION__, mFrameWidth, mFrameHeight, disp_width, disp_height,
                    mStride, mSliceHeight, buf_width, buf_height);
                mFrameWidth = disp_width;
                mFrameHeight = disp_height;
                mStride = buf_width;
                mSliceHeight = buf_height;
                change_ddr_freq();
                updatePortDefinitions(true, true);
                (*mVPXDecReleaseRefBuffers)(mHandle);

                notify(OMX_EventPortSettingsChanged, 1, 0, NULL);
                mOutputPortSettingsChange = AWAITING_DISABLED;
                return;
            } else if(decRet == MMDEC_MEMORY_ALLOCED) {
                continue;
            }
        } else if (decRet == MMDEC_MEMORY_ERROR) {
            ALOGE("failed to allocate memory, signal error!");
            notify(OMX_EventError, OMX_ErrorInsufficientResources, 0, NULL);
            mSignalledError = true;
            return;
        } else {
            ALOGE("failed to decode video frame.");
        }

//        ALOGI("%s, %d, bufferSize: %d, inHeader->nFilledLen: %d", __FUNCTION__, __LINE__, bufferSize, inHeader->nFilledLen);
        CHECK_LE(bufferSize, inHeader->nFilledLen);
        inHeader->nOffset += inHeader->nFilledLen - bufferSize;
        inHeader->nFilledLen -= bufferSize;

//        ALOGI("%s, %d, inHeader->nOffset: %d,inHeader->nFilledLen: %d , in->timestamp: %lld, timestamp: %d, out->timestamp: %lld",
//            __FUNCTION__, __LINE__, inHeader->nOffset, inHeader->nFilledLen, inHeader->nTimeStamp, timestamp,outHeader->nTimeStamp);
//        mFrameDecoded = false;
        if (inHeader->nFilledLen == 0) {
            mFrameDecoded = true;
            inInfo->mOwnedByUs = false;
            inQueue.erase(inQueue.begin());
            inInfo = NULL;
            notifyEmptyBufferDone(inHeader);
            inHeader = NULL;
        }
        ++mInputBufferCount;

        if (dec_out.frameEffective) {
            outHeader = (OMX_BUFFERHEADERTYPE*)(dec_out.pBufferHeader);
            outHeader->nOffset = 0;
            outHeader->nFilledLen = (mStride * mSliceHeight * 3) / 2;
            outHeader->nFlags = 0;
            ALOGV("%s, %d, drainOneOutputBuffer, dec_out.pBufferHeader: %p, nTimeStamp: %lld", __FUNCTION__, __LINE__,
                    outHeader, outHeader->nTimeStamp);
//           dump_yuv(outHeader->pBuffer, outHeader->nFilledLen);
        } else {
            continue;
        }

        List<BufferInfo *>::iterator it = outQueue.begin();
//        ALOGI("%s, %d,mHeader=0x%x, outHeader=0x%x", __FUNCTION__, __LINE__,(*it)->mHeader,outHeader );
        while ((*it)->mHeader != outHeader) {
//        ALOGI("%s, %d, while,mHeader=0x%x, outHeader=0x%x", __FUNCTION__, __LINE__,(*it)->mHeader,outHeader );
            ++it;
        }

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

bool SPRDVPXDecoder::drainAllOutputBuffers() {
    ALOGI("%s, %d", __FUNCTION__, __LINE__);

    List<BufferInfo *> &outQueue = getPortQueue(1);
    BufferInfo *outInfo;
    OMX_BUFFERHEADERTYPE *outHeader;
    void *pBufferHeader;

    while (!outQueue.empty() && mEOSStatus != OUTPUT_FRAMES_FLUSHED) {

        if ((*mVPXDecGetLastDspFrm)(mHandle, &pBufferHeader) ) {
            ALOGI("%s, %d, VPXDecGetLastDspFrm, pBufferHeader: %p", __FUNCTION__, __LINE__, pBufferHeader);
            List<BufferInfo *>::iterator it = outQueue.begin();
            while ((*it)->mHeader != (OMX_BUFFERHEADERTYPE*)pBufferHeader && it != outQueue.end()) {
                ++it;
            }
            CHECK((*it)->mHeader == (OMX_BUFFERHEADERTYPE*)pBufferHeader);
            outInfo = *it;
            outQueue.erase(it);
            outHeader = outInfo->mHeader;
            outHeader->nFilledLen = (mStride * mSliceHeight * 3) / 2;
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

void SPRDVPXDecoder::onPortFlushCompleted(OMX_U32 portIndex) {
    ALOGI("onPortFlushCompleted, portIndex:%d",portIndex);

    if (portIndex == OMX_DirInput) {
        mEOSStatus = INPUT_DATA_AVAILABLE;
    }
}

void SPRDVPXDecoder::onPortEnableCompleted(OMX_U32 portIndex, bool enabled) {
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

OMX_ERRORTYPE SPRDVPXDecoder::getConfig(
    OMX_INDEXTYPE index, OMX_PTR params) {
    switch (index) {
    case OMX_IndexConfigCommonOutputCrop:
    {
        OMX_CONFIG_RECTTYPE *rectParams = (OMX_CONFIG_RECTTYPE *)params;

        if (rectParams->nPortIndex != 1) {
            return OMX_ErrorUndefined;
        }

        rectParams->nLeft = 0;
        rectParams->nTop = 0;
        {
            ALOGI("%s, mCropWidth:%d, mCropHeight:%d, mGettingPortFormat:%d",
                __FUNCTION__, mCropWidth, mCropHeight, mGettingPortFormat);
            Mutex::Autolock autoLock(mLock);
            rectParams->nWidth = mCropWidth;
            rectParams->nHeight = mCropHeight;
            mGettingPortFormat = OMX_FALSE;
            mCondition.signal();
        }
        return OMX_ErrorNone;
    }

    default:
        return OMX_ErrorUnsupportedIndex;
    }
}

void SPRDVPXDecoder::onPortFlushPrepare(OMX_U32 portIndex) {
    ALOGI("onPortFlushPrepare, portIndex:%d",portIndex);

    if (portIndex == OMX_DirInput) {
        ALOGI("onPortFlushPrepare, reset codec!");
        releaseDecoder();
        initDecoder();
    }
}

void SPRDVPXDecoder::onReset() {
    mGettingPortFormat = OMX_FALSE;
    mSignalledError = false;

    //avoid process error after stop codec and restart codec when port settings changing.
    mOutputPortSettingsChange = NONE;
}

void SPRDVPXDecoder::updatePortDefinitions(bool updateCrop, bool updateInputSize) {
    OMX_PARAM_PORTDEFINITIONTYPE *outDef = &editPortInfo(kOutputPortIndex)->mDef;

    if (updateCrop) {
        mCropWidth = mFrameWidth;
        mCropHeight = mFrameHeight;
    }
    outDef->format.video.nFrameWidth = mStride;
    outDef->format.video.nFrameHeight = mSliceHeight;
    outDef->format.video.nStride = mStride;
    outDef->format.video.nSliceHeight = mSliceHeight;
    outDef->nBufferSize = mStride * mSliceHeight * 3 / 2;

    ALOGI("%s, %d %d %d %d", __FUNCTION__, outDef->format.video.nFrameWidth,
          outDef->format.video.nFrameHeight,
          outDef->format.video.nStride,
          outDef->format.video.nSliceHeight);

    OMX_PARAM_PORTDEFINITIONTYPE *inDef = &editPortInfo(kInputPortIndex)->mDef;

    inDef->format.video.nFrameWidth = mFrameWidth;
    inDef->format.video.nFrameHeight = mFrameHeight;
    // input port is compressed, hence it has no stride
    inDef->format.video.nStride = 0;
    inDef->format.video.nSliceHeight = 0;
}

OMX_ERRORTYPE SPRDVPXDecoder::getExtensionIndex(
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
    }

    return OMX_ErrorNotImplemented;
}

// static
int32_t SPRDVPXDecoder::BindFrameWrapper(
    void *aUserData, void *pHeader, int flag) {
    return static_cast<SPRDVPXDecoder *>(aUserData)->VSP_bind_cb(pHeader, flag);
}

// static
int32_t SPRDVPXDecoder::UnbindFrameWrapper(
    void *aUserData, void *pHeader, int flag) {
    return static_cast<SPRDVPXDecoder *>(aUserData)->VSP_unbind_cb(pHeader, flag);
}

int SPRDVPXDecoder::VSP_bind_cb(void *pHeader,int flag) {
    BufferCtrlStruct *pBufCtrl = (BufferCtrlStruct *)(((OMX_BUFFERHEADERTYPE *)pHeader)->pOutputPortPrivate);

    ALOGV("VSP_bind_cb, pBuffer: %p, pHeader: %p; iRefCount=%d",
          ((OMX_BUFFERHEADERTYPE *)pHeader)->pBuffer, pHeader,pBufCtrl->iRefCount);

    pBufCtrl->iRefCount++;
    return 0;
}

int SPRDVPXDecoder::VSP_unbind_cb(void *pHeader,int flag) {
    BufferCtrlStruct *pBufCtrl = (BufferCtrlStruct *)(((OMX_BUFFERHEADERTYPE *)pHeader)->pOutputPortPrivate);

    ALOGV("VSP_unbind_cb, pBuffer: %p, pHeader: %p; iRefCount=%d",
          ((OMX_BUFFERHEADERTYPE *)pHeader)->pBuffer, pHeader,pBufCtrl->iRefCount);

    if (pBufCtrl->iRefCount  > 0) {
        pBufCtrl->iRefCount--;
    }

    return 0;
}

bool SPRDVPXDecoder::openDecoder(const char* libName) {
    if(mLibHandle) {
        dlclose(mLibHandle);
    }

    ALOGI("openDecoder, lib: %s", libName);

    mLibHandle = dlopen(libName, RTLD_NOW);
    if(mLibHandle == NULL) {
        ALOGE("openDecoder, can't open lib: %s",libName);
        return false;
    }

    mVPXDecSetCurRecPic = (FT_VPXDecSetCurRecPic)dlsym(mLibHandle, "VP8DecSetCurRecPic");
    if(mVPXDecSetCurRecPic == NULL) {
        ALOGE("Can't find VPXDecSetCurRecPic in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mVPXDecInit = (FT_VPXDecInit)dlsym(mLibHandle, "VP8DecInit");
    if(mVPXDecInit == NULL) {
        ALOGE("Can't find VP8DecInit in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mVPXDecDecode = (FT_VPXDecDecode)dlsym(mLibHandle, "VP8DecDecode");
    if(mVPXDecDecode == NULL) {
        ALOGE("Can't find VP8DecDecode in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mVPXDecRelease = (FT_VPXDecRelease)dlsym(mLibHandle, "VP8DecRelease");
    if(mVPXDecRelease == NULL) {
        ALOGE("Can't find VP8DecRelease in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mVPXGetVideoDimensions = (FT_VPXGetVideoDimensions)dlsym(mLibHandle, "VP8GetVideoDimensions");
    if(mVPXGetVideoDimensions == NULL) {
        ALOGE("Can't find VP8GetVideoDimensions in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mVPXGetBufferDimensions = (FT_VPXGetBufferDimensions)dlsym(mLibHandle, "VP8GetBufferDimensions");
    if(mVPXGetBufferDimensions == NULL) {
        ALOGE("Can't find VP8GetBufferDimensions in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mVPXDecReleaseRefBuffers = (FT_VPXDecReleaseRefBuffers)dlsym(mLibHandle, "VP8DecReleaseRefBuffers");
    if(mVPXDecReleaseRefBuffers == NULL) {
        ALOGE("Can't find VP8DecReleaseRefBuffers in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mVPXDecGetLastDspFrm = (FT_VPXDecGetLastDspFrm)dlsym(mLibHandle, "VP8DecGetLastDspFrm");
    if(mVPXDecGetLastDspFrm == NULL) {
        ALOGE("Can't find VPXDecGetLastDspFrm in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mVPXGetCodecCapability = (FT_VPXGetCodecCapability)dlsym(mLibHandle, "VP8GetCodecCapability");
    if(mVPXGetCodecCapability == NULL) {
        ALOGE("Can't find VP8GetCodecCapability in %s",libName);
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
    return new android::SPRDVPXDecoder(name, callbacks, appData, component);
}

