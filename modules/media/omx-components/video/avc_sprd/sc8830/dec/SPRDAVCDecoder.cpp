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
#define LOG_TAG "SPRDAVCDecoder"
#include <utils/Log.h>

#include "SPRDAVCDecoder.h"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AUtils.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/IOMX.h>

#include <dlfcn.h>
#include <media/hardware/HardwareAPI.h>
#include <ui/GraphicBufferMapper.h>
#include <cutils/properties.h>

#include "gralloc_priv.h"
#include "ion_sprd.h"
#include "avc_dec_api.h"

//#define VIDEODEC_CURRENT_OPT  /*only open for SAMSUNG currently*/


namespace android {

#define MAX_INSTANCES 8

static int instances = 0;

const static int64_t kConditionEventTimeOutNs = 3000000000LL;

static const CodecProfileLevel kProfileLevels[] = {
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1  },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel1b },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel11 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel12 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel13 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel2  },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel21 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel22 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel3  },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel31 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel32 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel4  },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel41 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel42 },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel5  },
    { OMX_VIDEO_AVCProfileBaseline, OMX_VIDEO_AVCLevel51 },

    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1  },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel1b },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel11 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel12 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel13 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel2  },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel21 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel22 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel3  },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel31 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel32 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel4  },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel41 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel42 },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel5  },
    { OMX_VIDEO_AVCProfileMain, OMX_VIDEO_AVCLevel51 },

    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1  },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel1b },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel11 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel12 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel13 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel2  },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel21 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel22 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel3  },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel31 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel32 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel4  },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel41 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel42 },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel5  },
    { OMX_VIDEO_AVCProfileHigh, OMX_VIDEO_AVCLevel51 },
};

template<class T>
static void InitOMXParams(T *params) {
    params->nSize = sizeof(T);
    params->nVersion.s.nVersionMajor = 1;
    params->nVersion.s.nVersionMinor = 0;
    params->nVersion.s.nRevision = 0;
    params->nVersion.s.nStep = 0;
}

typedef struct LevelConversion {
    OMX_U32 omxLevel;
    AVCLevel avcLevel;
} LevelConcersion;

static LevelConversion ConversionTable[] = {
    { OMX_VIDEO_AVCLevel1,  AVC_LEVEL1_B },
    { OMX_VIDEO_AVCLevel1b, AVC_LEVEL1   },
    { OMX_VIDEO_AVCLevel11, AVC_LEVEL1_1 },
    { OMX_VIDEO_AVCLevel12, AVC_LEVEL1_2 },
    { OMX_VIDEO_AVCLevel13, AVC_LEVEL1_3 },
    { OMX_VIDEO_AVCLevel2,  AVC_LEVEL2 },
#if 1
    // encoding speed is very poor if video
    // resolution is higher than CIF
    { OMX_VIDEO_AVCLevel21, AVC_LEVEL2_1 },
    { OMX_VIDEO_AVCLevel22, AVC_LEVEL2_2 },
    { OMX_VIDEO_AVCLevel3,  AVC_LEVEL3   },
    { OMX_VIDEO_AVCLevel31, AVC_LEVEL3_1 },
    { OMX_VIDEO_AVCLevel32, AVC_LEVEL3_2 },
    { OMX_VIDEO_AVCLevel4,  AVC_LEVEL4   },
    { OMX_VIDEO_AVCLevel41, AVC_LEVEL4_1 },
    { OMX_VIDEO_AVCLevel42, AVC_LEVEL4_2 },
    { OMX_VIDEO_AVCLevel5,  AVC_LEVEL5   },
    { OMX_VIDEO_AVCLevel51, AVC_LEVEL5_1 },
#endif
};

static bool outputBuffersNotEnough(const H264SwDecInfo *info, OMX_U32 bufferCountMin,
                                   OMX_U32 bufferCountActual, OMX_BOOL useNativeBuffer)
{
    if(useNativeBuffer) {
        if (info->numRefFrames + info->has_b_frames + 1 + 1 > bufferCountMin) {
            return true;
        }
    } else {
        if (info->numRefFrames + info->has_b_frames + 1 + 1 + 4 > bufferCountActual) {
            return true;
        }
    }

    return false;
}

SPRDAVCDecoder::SPRDAVCDecoder(
    const char *name,
    const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData,
    OMX_COMPONENTTYPE **component)
    : SprdSimpleOMXComponent(name, callbacks, appData, component),
      mHandle(new tagAVCHandle),
      mInputBufferCount(0),
      mPicId(0),
      mSetFreqCount(0),
      mMinCompressionRatio(2),
      mFrameWidth(320),
      mFrameHeight(240),
      mStride(mFrameWidth),
      mSliceHeight(mFrameHeight),
      mPictureSize(mStride * mSliceHeight * 3 / 2),
      mCropWidth(mFrameWidth),
      mCropHeight(mFrameHeight),
      mGettingPortFormat(OMX_FALSE),
      mEOSStatus(INPUT_DATA_AVAILABLE),
      mOutputPortSettingsChange(NONE),
      mHeadersDecoded(false),
      mSignalledError(false),
      mDecoderSwFlag(false),
      mChangeToSwDec(false),
      mAllocateBuffers(false),
      mNeedIVOP(true),
      mIOMMUEnabled(false),
      mIOMMUID(-1),
      mDumpYUVEnabled(false),
      mDumpStrmEnabled(false),
      mStopDecode(false),
      mThumbnailMode(OMX_FALSE),
      mCodecInterBuffer(NULL),
      mCodecExtraBuffer(NULL),
      mPmem_stream(NULL),
      mPbuf_stream_v(NULL),
      mPbuf_stream_p(0),
      mPbuf_stream_size(0),
      mPmem_extra(NULL),
      mPbuf_extra_v(NULL),
      mPbuf_extra_p(0),
      mPbuf_extra_size(0),
      mPbuf_mbinfo_idx(0),
      mDecoderSawSPS(false),
      mDecoderSawPPS(false),
      mSPSData(NULL),
      mSPSDataSize(0),
      mPPSData(NULL),
      mPPSDataSize(0),
      mIsResume(false),
      mLibHandle(NULL),
      mH264DecInit(NULL),
      mH264DecGetInfo(NULL),
      mH264DecDecode(NULL),
      mH264DecRelease(NULL),
      mH264Dec_SetCurRecPic(NULL),
      mH264Dec_GetLastDspFrm(NULL),
      mH264Dec_ReleaseRefBuffers(NULL),
      mH264DecMemInit(NULL),
      mH264GetCodecCapability(NULL),
      mH264DecGetNALType(NULL),
      mH264DecSetparam(NULL),
      mFrameDecoded(false){

    ALOGI("Construct SPRDAVCDecoder, this: %p, instances: %d", (void *)this, instances);

    mInitCheck = OMX_ErrorNone;

    //read config flag
#define USE_SW_DECODER	0x01
#define USE_HW_DECODER	0x00

    uint8_t video_cfg = USE_HW_DECODER;
    FILE *fp = fopen("/data/data/com.sprd.test.videoplayer/app_decode/flag", "rb");
    if (fp != NULL) {
        fread(&video_cfg, sizeof(uint8_t), 1, fp);
        fclose(fp);
    }
    ALOGI("%s, video_cfg: %d", __FUNCTION__, video_cfg);

    bool ret = false;
    if (USE_HW_DECODER == video_cfg) {
        ret = openDecoder("libomx_avcdec_hw_sprd.so");
    }

    if(ret == false) {
        ret = openDecoder("libomx_avcdec_sw_sprd.so");
        mDecoderSwFlag = true;
    }

    CHECK_EQ(ret, true);

    char value_dump[PROPERTY_VALUE_MAX];

    property_get("h264dec.yuv.dump", value_dump, "false");
    mDumpYUVEnabled = !strcmp(value_dump, "true");

    property_get("h264dec.strm.dump", value_dump, "false");
    mDumpStrmEnabled = !strcmp(value_dump, "true");
    ALOGI("%s, mDumpYUVEnabled: %d, mDumpStrmEnabled: %d", __FUNCTION__, mDumpYUVEnabled, mDumpStrmEnabled);

    if (MemoryHeapIon::IOMMU_is_enabled(ION_MM)) {
        mIOMMUEnabled = true;
        mIOMMUID = ION_MM;
    } else if (MemoryHeapIon::IOMMU_is_enabled(ION_VSP)) {
        mIOMMUEnabled = true;
        mIOMMUID = ION_VSP;
    }
    ALOGI("%s, is IOMMU enabled: %d, ID: %d", __FUNCTION__, mIOMMUEnabled, mIOMMUID);

    if(mDecoderSwFlag) {
        CHECK_EQ(initDecoder(), (status_t)OK);
    } else {
        if (initDecoder() != OK) {
            if (openDecoder("libomx_avcdec_sw_sprd.so")) {
                mDecoderSwFlag = true;
                if(initDecoder() != OK) {
                    mInitCheck = OMX_ErrorInsufficientResources;
                }
            } else {
                mInitCheck = OMX_ErrorInsufficientResources;
            }
        }
    }

    mSPSData = (uint8_t *)malloc(H264_HEADER_SIZE);
    mPPSData = (uint8_t *)malloc(H264_HEADER_SIZE);
    if (mSPSData == NULL || mPPSData == NULL) {
        mInitCheck = OMX_ErrorInsufficientResources;
    }

    for (int i = 0; i < 17; i++) {
        mPmem_mbinfo[i] = NULL;
        mPbuf_mbinfo_v[i] = NULL;
        mPbuf_mbinfo_p[i] = 0;
        mPbuf_mbinfo_size[i] = 0;
    }

    initPorts();

    iUseAndroidNativeBuffer[OMX_DirInput] = OMX_FALSE;
    iUseAndroidNativeBuffer[OMX_DirOutput] = OMX_FALSE;

    instances++;
    if (instances > MAX_INSTANCES) {
        ALOGE("instances(%d) are too much, return OMX_ErrorInsufficientResources", instances);
        mInitCheck = OMX_ErrorInsufficientResources;
    }
}

SPRDAVCDecoder::~SPRDAVCDecoder() {
    ALOGI("Destruct SPRDAVCDecoder, this: %p, instances: %d", (void *)this, instances);

    releaseDecoder();

    while (mSetFreqCount > 0)
    {
        set_ddr_freq("0");
        mSetFreqCount--;
    }

    if (mSPSData != NULL) {
        free(mSPSData);
        mSPSData = NULL;
    }

    if (mPPSData != NULL) {
        free(mPPSData);
        mPPSData = NULL;
    }

    delete mHandle;
    mHandle = NULL;

    List<BufferInfo *> &outQueue = getPortQueue(kOutputPortIndex);
    List<BufferInfo *> &inQueue = getPortQueue(kInputPortIndex);
    CHECK(outQueue.empty());
    CHECK(inQueue.empty());

    instances--;
}

OMX_ERRORTYPE SPRDAVCDecoder::initCheck() const{
    ALOGI("%s, mInitCheck: 0x%x", __FUNCTION__, mInitCheck);
    return mInitCheck;
}

void SPRDAVCDecoder::initPorts() {
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);

    def.nPortIndex = kInputPortIndex;
    def.eDir = OMX_DirInput;
    def.nBufferCountMin = kNumInputBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.nBufferSize = 1920*1088*3/2/2;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainVideo;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 1;

    def.format.video.cMIMEType = const_cast<char *>(MEDIA_MIMETYPE_VIDEO_AVC);
    def.format.video.pNativeRender = NULL;
    def.format.video.nFrameWidth = mFrameWidth;
    def.format.video.nFrameHeight = mFrameHeight;
    def.format.video.nStride = def.format.video.nFrameWidth;
    def.format.video.nSliceHeight = def.format.video.nFrameHeight;
    def.format.video.nBitrate = 0;
    def.format.video.xFramerate = 0;
    def.format.video.bFlagErrorConcealment = OMX_FALSE;
    def.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
    def.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    def.format.video.pNativeWindow = NULL;

    addPort(def);

    def.nPortIndex = kOutputPortIndex;
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
    def.format.video.nFrameWidth = mFrameWidth;
    def.format.video.nFrameHeight = mFrameHeight;
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

void SPRDAVCDecoder::set_ddr_freq(const char* freq_in_khz)
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

void SPRDAVCDecoder::change_ddr_freq()
{
    if(!mDecoderSwFlag)
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
}

status_t SPRDAVCDecoder::initDecoder() {

    memset(mHandle, 0, sizeof(tagAVCHandle));

    mHandle->userdata = (void *)this;
    mHandle->VSP_bindCb = BindFrameWrapper;
    mHandle->VSP_unbindCb = UnbindFrameWrapper;
    mHandle->VSP_extMemCb = ExtMemAllocWrapper;
    mHandle->VSP_mbinfoMemCb = MbinfoMemAllocWrapper;

    unsigned long phy_addr = 0;
    size_t size = 0, size_stream;

    size_stream = H264_DECODER_STREAM_BUFFER_SIZE;
    if (mDecoderSwFlag) {
        mPbuf_stream_v = (uint8_t*)malloc(size_stream * sizeof(unsigned char));
        mPbuf_stream_p = 0;
        mPbuf_stream_size = size_stream;
    } else {
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
                mPbuf_stream_v = (uint8_t*)mPmem_stream->getBase();
                mPbuf_stream_p = phy_addr;
                mPbuf_stream_size = size;
                ALOGI("pmem 0x%lx - %p - %zd", mPbuf_stream_p, mPbuf_stream_v, mPbuf_stream_size);
            }
        }
    }

    uint32_t size_inter = H264_DECODER_INTERNAL_BUFFER_SIZE;
    mCodecInterBuffer = (uint8_t *)malloc(size_inter);
    CHECK(mCodecInterBuffer != NULL);

    MMCodecBuffer codec_buf;
    MMDecVideoFormat video_format;

    codec_buf.common_buffer_ptr = mCodecInterBuffer;
    codec_buf.common_buffer_ptr_phy = 0;
    codec_buf.size = size_inter;
    codec_buf.int_buffer_ptr = NULL;
    codec_buf.int_size = 0;

    video_format.video_std = H264;
    video_format.frame_width = 0;
    video_format.frame_height = 0;
    video_format.p_extra = NULL;
    video_format.p_extra_phy = 0;
    video_format.i_extra = 0;
    //video_format.uv_interleaved = 1;
    video_format.yuv_format = YUV420SP_NV12;

    if ((*mH264DecInit)(mHandle, &codec_buf,&video_format) != MMDEC_OK) {
        ALOGE("Failed to init AVCDEC");
        return OMX_ErrorUndefined;
    }

    //int32 codec_capabilty;
    if ((*mH264GetCodecCapability)(mHandle, &mCapability) != MMDEC_OK) {
        ALOGE("Failed to mH264GetCodecCapability");
    }

    ALOGI("initDecoder, Capability: profile %d, level %d, max wh=%d %d",
          mCapability.profile, mCapability.level, mCapability.max_width, mCapability.max_height);

    return OMX_ErrorNone;
}

void SPRDAVCDecoder::releaseDecoder() {
    if( mH264DecRelease!=NULL )
        (*mH264DecRelease)(mHandle);

    if (mCodecInterBuffer != NULL) {
        free(mCodecInterBuffer);
        mCodecInterBuffer = NULL;
    }

    if (mCodecExtraBuffer != NULL) {
        free(mCodecExtraBuffer);
        mCodecExtraBuffer = NULL;
    }

    if (mPbuf_stream_v != NULL) {
        if (mDecoderSwFlag) {
            free(mPbuf_stream_v);
            mPbuf_stream_v = NULL;
        } else {
            if (mIOMMUEnabled) {
                mPmem_stream->free_iova(mIOMMUID, mPbuf_stream_p, mPbuf_stream_size);
            }
            mPmem_stream.clear();
            mPbuf_stream_v = NULL;
            mPbuf_stream_p = 0;
            mPbuf_stream_size = 0;
        }
    }
    if (mPbuf_extra_v != NULL) {
        if (mIOMMUEnabled) {
            mPmem_extra->free_iova(mIOMMUID, mPbuf_extra_p, mPbuf_extra_size);
        }
        mPmem_extra.clear();
        mPbuf_extra_v = NULL;
        mPbuf_extra_p = 0;
        mPbuf_extra_size = 0;
    }

    for (int i = 0; i < 17; i++) {
        if (mPbuf_mbinfo_v[i]) {
            if (mIOMMUEnabled) {
                mPmem_mbinfo[i]->free_iova(ION_MM, mPbuf_mbinfo_p[i], mPbuf_mbinfo_size[i]);
            }
            mPmem_mbinfo[i].clear();
            mPbuf_mbinfo_v[i] = NULL;
            mPbuf_mbinfo_p[i] = 0;
            mPbuf_mbinfo_size[i] = 0;
        }
    }
    mPbuf_mbinfo_idx = 0;

    if(mLibHandle) {
        dlclose(mLibHandle);
        mLibHandle = NULL;
        mH264Dec_ReleaseRefBuffers = NULL;
        mH264DecRelease = NULL;
    }
}

OMX_ERRORTYPE SPRDAVCDecoder::internalGetParameter(
    OMX_INDEXTYPE index, OMX_PTR params) {
    switch (index) {
    case OMX_IndexParamVideoPortFormat:
    {
        OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams =
            (OMX_VIDEO_PARAM_PORTFORMATTYPE *)params;

        if (formatParams->nPortIndex > kOutputPortIndex) {
            return OMX_ErrorUndefined;
        }

        if (formatParams->nIndex != 0) {
            return OMX_ErrorNoMore;
        }

        if (formatParams->nPortIndex == kInputPortIndex) {
            formatParams->eCompressionFormat = OMX_VIDEO_CodingAVC;
            formatParams->eColorFormat = OMX_COLOR_FormatUnused;
            formatParams->xFramerate = 0;
        } else {
            CHECK(formatParams->nPortIndex == kOutputPortIndex);

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

        if (profileLevel->nPortIndex != kInputPortIndex) {
            ALOGE("Invalid port index: %d", profileLevel->nPortIndex);
            return OMX_ErrorUnsupportedIndex;
        }

        size_t index = profileLevel->nProfileIndex;
        size_t nProfileLevels =
            sizeof(kProfileLevels) / sizeof(kProfileLevels[0]);
        if (index >= nProfileLevels) {
            return OMX_ErrorNoMore;
        }

        profileLevel->eProfile = kProfileLevels[index].mProfile;
        profileLevel->eLevel = kProfileLevels[index].mLevel;

        if (profileLevel->eProfile == OMX_VIDEO_AVCProfileHigh) {
            if (mCapability.profile < AVC_HIGH) {
                profileLevel->eProfile = OMX_VIDEO_AVCProfileMain;
            }
        }

        if (profileLevel->eProfile == OMX_VIDEO_AVCProfileMain) {
            if (mCapability.profile < AVC_MAIN) {
                profileLevel->eProfile = OMX_VIDEO_AVCProfileBaseline;
            }
        }

        const size_t size =
            sizeof(ConversionTable) / sizeof(ConversionTable[0]);

        for (index = 1; index < (size-1); index++) {
            if (ConversionTable[index].avcLevel > mCapability.level) {
                index--;
                break;
            }
        }

        if (profileLevel->eLevel > ConversionTable[index].omxLevel) {
            profileLevel->eLevel = ConversionTable[index].omxLevel;
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
        if(mDecoderSwFlag || mIOMMUEnabled) {
            pganbp->nUsage = GRALLOC_USAGE_SW_READ_OFTEN |GRALLOC_USAGE_SW_WRITE_OFTEN;
        } else {
            pganbp->nUsage = GRALLOC_USAGE_VIDEO_BUFFER | GRALLOC_USAGE_SW_READ_OFTEN |GRALLOC_USAGE_SW_WRITE_OFTEN;
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

OMX_ERRORTYPE SPRDAVCDecoder::internalSetParameter(
    OMX_INDEXTYPE index, const OMX_PTR params) {
    switch (index) {
    case OMX_IndexParamStandardComponentRole:
    {
        const OMX_PARAM_COMPONENTROLETYPE *roleParams =
            (const OMX_PARAM_COMPONENTROLETYPE *)params;

        if (strncmp((const char *)roleParams->cRole,
                    "video_decoder.avc",
                    OMX_MAX_STRINGNAME_SIZE - 1)) {
            return OMX_ErrorUndefined;
        }

        return OMX_ErrorNone;
    }

    case OMX_IndexParamVideoPortFormat:
    {
        OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams =
            (OMX_VIDEO_PARAM_PORTFORMATTYPE *)params;

        if (formatParams->nPortIndex > kOutputPortIndex) {
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
            /*FIXME: when NativeWindow is null, we can't use nBufferCountMin to calculate to
            * nBufferCountActual in Acodec.cpp&OMXCodec.cpp. So we need set nBufferCountActual
            * manually.
            * 4: reserved buffers by SurfaceFlinger(according to Acodec.cpp&OMXCodec.cpp)*/
            pOutPort->mDef.nBufferCountActual = pOutPort->mDef.nBufferCountMin + 4;
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

        ALOGI("%s, port:%d, old wh:%d %d, new wh:%d %d", __FUNCTION__, defParams->nPortIndex,
              oldWidth, oldHeight, newWidth, newHeight);

        memcpy(&port->mDef.format.video, &defParams->format.video, sizeof(OMX_VIDEO_PORTDEFINITIONTYPE));

        if((oldWidth != newWidth || oldHeight != newHeight)) {
            if (defParams->nPortIndex == kOutputPortIndex) {
                mFrameWidth = newWidth;
                mFrameHeight = newHeight;
                mStride = ((newWidth + 15) & -16);
                mSliceHeight = ((newHeight + 15) & -16);
                mPictureSize = mStride* mSliceHeight * 3 / 2;

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

OMX_ERRORTYPE SPRDAVCDecoder::internalUseBuffer(
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
        if(mAllocateBuffers) {
            if(bufferPrivate != NULL) {
                pBufCtrl->pMem = ((BufferPrivateStruct*)bufferPrivate)->pMem;
                pBufCtrl->phyAddr = ((BufferPrivateStruct*)bufferPrivate)->phyAddr;
                pBufCtrl->bufferSize = ((BufferPrivateStruct*)bufferPrivate)->bufferSize;
                pBufCtrl->bufferFd = 0;
            } else {
                pBufCtrl->pMem = NULL;
                pBufCtrl->phyAddr = 0;
                pBufCtrl->bufferSize = 0;
                pBufCtrl->bufferFd = 0;
            }
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

OMX_ERRORTYPE SPRDAVCDecoder::allocateBuffer(
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
        mAllocateBuffers = true;
        if(mDecoderSwFlag  || mChangeToSwDec) {
            return SprdSimpleOMXComponent::allocateBuffer(header, portIndex, appPrivate, size);
        } else {
            MemoryHeapIon* pMem = NULL;
            unsigned long phyAddr = 0;
            size_t bufferSize = 0;//don't use OMX_U32
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
    }

    default:
        return OMX_ErrorUnsupportedIndex;

    }
}

OMX_ERRORTYPE SPRDAVCDecoder::freeBuffer(
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

OMX_ERRORTYPE SPRDAVCDecoder::getConfig(
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

OMX_ERRORTYPE SPRDAVCDecoder::setConfig(
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
            if(!mDecoderSwFlag) {
                mChangeToSwDec = true;
            }
        }
        return OMX_ErrorNone;
    }

    default:
        return SprdSimpleOMXComponent::setConfig(index, params);
    }
}

void SPRDAVCDecoder::dump_strm(uint8 *pBuffer, int32 aInBufSize) {
    if(mDumpStrmEnabled) {
        FILE *fp = fopen("/data/misc/media/video_es.m4v","ab");
        fwrite(pBuffer,1,aInBufSize,fp);
        fclose(fp);
    }
}

void SPRDAVCDecoder::dump_yuv(uint8 *pBuffer, int32 aInBufSize) {
    if(mDumpYUVEnabled) {
        FILE *fp = fopen("/data/misc/media/video_out.yuv","ab");
        fwrite(pBuffer,1,aInBufSize,fp);
        fclose(fp);
    }
}

void SPRDAVCDecoder::onQueueFilled(OMX_U32 portIndex) {
    if (mSignalledError || mOutputPortSettingsChange != NONE) {
        return;
    }

    if (mEOSStatus == OUTPUT_FRAMES_FLUSHED) {
        return;
    }

    if(mChangeToSwDec) {

        mChangeToSwDec = false;

        ALOGI("%s, %d, change to sw decoder, mThumbnailMode: %d",
              __FUNCTION__, __LINE__, mThumbnailMode);

        releaseDecoder();

        if(!openDecoder("libomx_avcdec_sw_sprd.so")) {
            ALOGE("onQueueFilled, open  libomx_avcdec_sw_sprd.so failed.");
            notify(OMX_EventError, OMX_ErrorDynamicResourcesUnavailable, 0, NULL);
            mSignalledError = true;
            mDecoderSwFlag = false;
            return;
        }

        mDecoderSwFlag = true;

        if(initDecoder() != OK) {
            ALOGE("onQueueFilled, init sw decoder failed.");
            notify(OMX_EventError, OMX_ErrorDynamicResourcesUnavailable, 0, NULL);
            mSignalledError = true;
            return;
        }

        if (mThumbnailMode) {
            MMDecVideoFormat video_format;
            video_format.yuv_format = YUV420P_YU12;
            (*mH264DecSetparam)(mHandle, &video_format);
        }

        freeOutputBufferIOVA();
    }

    List<BufferInfo *> &inQueue = getPortQueue(kInputPortIndex);
    List<BufferInfo *> &outQueue = getPortQueue(kOutputPortIndex);

    while (!mStopDecode && (mEOSStatus != INPUT_DATA_AVAILABLE || !inQueue.empty())
            && outQueue.size() != 0) {

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
                ALOGV("onQueueFilled, get outQueue buffer, return, count=%zd, queue_size=%d",count, outQueue.size());
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

//        ALOGI("%s, %d, mBuffer=0x%x, outHeader=0x%x, iRefCount=%d", __FUNCTION__, __LINE__, *itBuffer, outHeader, pBufCtrl->iRefCount);
        ALOGV("%s, %d, outHeader:%p, inHeader: %p, len: %d, nOffset: %d, time: %lld, EOS: %d",
              __FUNCTION__, __LINE__,outHeader,inHeader, inHeader->nFilledLen,inHeader->nOffset, inHeader->nTimeStamp,inHeader->nFlags & OMX_BUFFERFLAG_EOS);

        ++mPicId;
        mFrameDecoded = false;
        if (inHeader->nFlags & OMX_BUFFERFLAG_EOS) {
//bug253058 , the last frame size may be not zero, it need to be decoded.
//            inQueue.erase(inQueue.begin());
//           inInfo->mOwnedByUs = false;
//            notifyEmptyBufferDone(inHeader);
            mEOSStatus = INPUT_EOS_SEEN;
//            continue;
        }

        if(inHeader->nFilledLen == 0 || inHeader->nFilledLen > H264_DECODER_STREAM_BUFFER_SIZE - 4) {
            mFrameDecoded = true;
            inInfo->mOwnedByUs = false;
            inQueue.erase(inQueue.begin());
            inInfo = NULL;
            notifyEmptyBufferDone(inHeader);
            inHeader = NULL;
            continue;
        }

        MMDecInput dec_in;
        MMDecOutput dec_out;
        uint32_t add_startcode_len = 0;

        uint8_t *bitstream = inHeader->pBuffer + inHeader->nOffset;
        uint32_t bufferSize = inHeader->nFilledLen;

        if (!mDecoderSawSPS || !mDecoderSawPPS) {
            if (inHeader->nFlags & OMX_BUFFERFLAG_CODECCONFIG) {
                if ((mSPSDataSize == 0) || (mPPSDataSize == 0)) {
                    findCodecConfigData();
                }
            } else {
                if (!mDecoderSawSPS) {
                    if (mSPSDataSize > 0) {
                        ALOGI("%s, drain SPSData", __FUNCTION__);
                        bitstream = mSPSData;
                        bufferSize = mSPSDataSize;
                        mIsResume = true;
                    }
                } else if (!mDecoderSawPPS) {
                    if (mPPSDataSize > 0) {
                        ALOGI("%s, drain PPSData", __FUNCTION__);
                        bitstream = mPPSData;
                        bufferSize = mPPSDataSize;
                        mIsResume = true;
                    }
                }
            }
        }

        dec_in.pStream = mPbuf_stream_v;
        dec_in.pStream_phy = mPbuf_stream_p;
        dec_in.dataLen = bufferSize;
        dec_in.beLastFrm = 0;
        dec_in.expected_IVOP = mNeedIVOP;
        dec_in.beDisplayed = 1;
        dec_in.err_pkt_num = 0;
        dec_in.nTimeStamp = (uint64)(inHeader->nTimeStamp);

        dec_out.frameEffective = 0;

        if(mThumbnailMode) {
            uint8_t *p = bitstream;

            if((p[0] != 0x0) || (p[1] != 0x0) || (p[2] != 0x0) || (p[3] != 0x1))
            {
                ALOGI("%s, %d, p[0]: %x, p[1]: %x, p[2]: %x, p[3]: %x", __FUNCTION__, __LINE__, p[0], p[1], p[2], p[3]);

                ((uint8_t *) mPbuf_stream_v)[0] = 0x0;
                ((uint8_t *) mPbuf_stream_v)[1] = 0x0;
                ((uint8_t *) mPbuf_stream_v)[2] = 0x0;
                ((uint8_t *) mPbuf_stream_v)[3] = 0x1;

                add_startcode_len = 4;
                dec_in.dataLen += add_startcode_len;
            }
            memcpy(mPbuf_stream_v+add_startcode_len, bitstream, bufferSize);
        } else {
            if (mPbuf_stream_v != NULL) {
                memcpy(mPbuf_stream_v, bitstream, bufferSize);
            }
        }

        ALOGV("%s, %d, dec_in.dataLen: %d, mPicId: %d", __FUNCTION__, __LINE__, dec_in.dataLen, mPicId);

        outHeader->nTimeStamp = inHeader->nTimeStamp;
        outHeader->nFlags = inHeader->nFlags;

        unsigned long picPhyAddr = 0;
        if(!mDecoderSwFlag) {
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
        }

        ALOGV("%s, %d, outHeader: %p, pBuffer: %p, phyAddr: 0x%lx",__FUNCTION__, __LINE__, outHeader, outHeader->pBuffer, picPhyAddr);
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
                ALOGE("onQueueFilled, mapper.lock fail %p",outHeader->pBuffer);
                return ;
            }
            ALOGV("%s, %d, pBuffer: 0x%p, vaddr: %p", __FUNCTION__, __LINE__, outHeader->pBuffer,vaddr);
            uint8_t *yuv = (uint8_t *)((uint8_t *)vaddr + outHeader->nOffset);
            ALOGV("%s, %d, yuv: %p, mPicId: %d, outHeader: %p, outHeader->pBuffer: %p, outHeader->nTimeStamp: %lld",
                  __FUNCTION__, __LINE__, yuv, mPicId,outHeader, outHeader->pBuffer, outHeader->nTimeStamp);
            (*mH264Dec_SetCurRecPic)(mHandle, yuv, (uint8 *)picPhyAddr, (void *)outHeader, mPicId);
        } else {
            uint8 *yuv = (uint8 *)(outHeader->pBuffer + outHeader->nOffset);
            (*mH264Dec_SetCurRecPic)(mHandle, yuv, (uint8 *)picPhyAddr, (void *)outHeader, mPicId);
        }

        dump_strm(mPbuf_stream_v, dec_in.dataLen);

        int64_t start_decode = systemTime();
        MMDecRet decRet = (*mH264DecDecode)(mHandle, &dec_in,&dec_out);
        int64_t end_decode = systemTime();
        ALOGI("%s, %d, decRet: %d, %dms, dec_out.frameEffective: %d, needIVOP: %d, consume byte: %u, flag:0x%x, SPS:%d, PPS:%d, pts:%lld",
              __FUNCTION__, __LINE__, decRet, (unsigned int)((end_decode-start_decode) / 1000000L),
              dec_out.frameEffective, mNeedIVOP, dec_in.dataLen, inHeader->nFlags,dec_out.sawSPS,dec_out.sawPPS, dec_out.pts);

        mDecoderSawSPS = dec_out.sawSPS;
        mDecoderSawPPS = dec_out.sawPPS;

        if(iUseAndroidNativeBuffer[OMX_DirOutput]) {
            if(mapper.unlock((const native_handle_t*)outHeader->pBuffer)) {
                ALOGE("onQueueFilled, mapper.unlock fail %p",outHeader->pBuffer);
            }
        }

        if( decRet == MMDEC_OK) {
            mNeedIVOP = false;
        } else {
            mNeedIVOP = true;
            if (decRet == MMDEC_MEMORY_ERROR) {
                ALOGE("failed to allocate memory.");
                if (mDecoderSwFlag) {
                    notify(OMX_EventError, OMX_ErrorInsufficientResources, 0, NULL);
                    mSignalledError = true;
                } else {
                    ALOGI("change to sw decoder.");
                    mChangeToSwDec = true;
                    mDecoderSawSPS = false;
                    mDecoderSawPPS = false;
                }
                return;
            } else if (decRet == MMDEC_NOT_SUPPORTED) {
                ALOGE("failed to support this format.");
                notify(OMX_EventError, OMX_ErrorFormatNotDetected, 0, NULL);
                mSignalledError = true;
                return;
            } else if (decRet == MMDEC_STREAM_ERROR) {
                ALOGE("failed to decode video frame, stream error");
            } else if (decRet == MMDEC_HW_ERROR) {
                ALOGE("failed to decode video frame, hardware error");
            } else {
                ALOGI("now, we don't take care of the decoder return: %d", decRet);
            }
        }

        H264SwDecInfo decoderInfo;
        MMDecRet ret;
        ret = (*mH264DecGetInfo)(mHandle, &decoderInfo);
        if (ret == MMDEC_OK) {
            if (!((decoderInfo.picWidth<= mCapability.max_width&& decoderInfo.picHeight<= mCapability.max_height)
                    || (decoderInfo.picWidth <= mCapability.max_height && decoderInfo.picHeight <= mCapability.max_width))) {
                ALOGE("[%d,%d] is out of range [%d, %d], failed to support this format.",
                      decoderInfo.picWidth, decoderInfo.picHeight, mCapability.max_width, mCapability.max_height);
                notify(OMX_EventError, OMX_ErrorFormatNotDetected, 0, NULL);
                mSignalledError = true;
                return;
            }

            if (handlePortSettingChangeEvent(&decoderInfo)) {
                return;
            } else if(mChangeToSwDec == true) {
                return;
            }
        } else {
            ALOGE("failed to get decoder information.");
        }

        if (mIsResume) {
            mIsResume = false;
        } else {
            CHECK_LE(dec_in.dataLen, inHeader->nFilledLen + add_startcode_len);

            bufferSize = dec_in.dataLen;
            inHeader->nOffset += bufferSize;
            inHeader->nFilledLen -= bufferSize;
//            mFrameDecoded = false;
            if (inHeader->nFilledLen <= 0) {
                mFrameDecoded = true;
                inHeader->nOffset = 0;
                inInfo->mOwnedByUs = false;
                inQueue.erase(inQueue.begin());
                inInfo = NULL;
                notifyEmptyBufferDone(inHeader);
                inHeader = NULL;
            }
        }

        while (!outQueue.empty() &&
                mHeadersDecoded &&
                dec_out.frameEffective) {
            ALOGV("%s, %d, dec_out.pBufferHeader: %p, dec_out.mPicId: %d, dec_out.pts: %lld", __FUNCTION__, __LINE__, dec_out.pBufferHeader, dec_out.mPicId, dec_out.pts);
            drainOneOutputBuffer(dec_out.mPicId, dec_out.pBufferHeader, dec_out.pts);
            dump_yuv(dec_out.pOutFrameY, mPictureSize);

            dec_out.frameEffective = false;
            if(mThumbnailMode) {
                mStopDecode = true;
            }
        }
    }
}

bool SPRDAVCDecoder::handlePortSettingChangeEvent(const H264SwDecInfo *info) {
    OMX_PARAM_PORTDEFINITIONTYPE *def = &editPortInfo(kOutputPortIndex)->mDef;
    OMX_BOOL useNativeBuffer = iUseAndroidNativeBuffer[OMX_DirOutput];
    bool needFlushBuffer = outputBuffersNotEnough(info, def->nBufferCountMin, def->nBufferCountActual, useNativeBuffer);
    bool cropChanged = handleCropRectEvent(&info->cropParams);

    if ((mStride != info->picWidth) || (mSliceHeight != info->picHeight) || cropChanged || (!mThumbnailMode && needFlushBuffer)) {
        Mutex::Autolock autoLock(mLock);
        int32_t picId;
        void* pBufferHeader;
        uint64 pts;

        while (MMDEC_OK == (*mH264Dec_GetLastDspFrm)(mHandle, &pBufferHeader, &picId, &pts)) {
            drainOneOutputBuffer(picId, pBufferHeader, pts);
        }
        if (mGettingPortFormat == OMX_TRUE) {
            ALOGI("%s, waiting for get crop parameter done", __FUNCTION__);
            status_t err = mCondition.waitRelative(mLock, kConditionEventTimeOutNs);
            if (err != OK) {
                ALOGE("Timed out waiting for mCondition signal!");
            }
        }
        ALOGI("%s, %d, mStride: %d, mSliceHeight: %d, info->picWidth: %d, info->picHeight: %d, mGettingPortFormat:%d",
              __FUNCTION__, __LINE__,mStride, mSliceHeight, info->picWidth, info->picHeight, mGettingPortFormat);

        mFrameWidth = info->cropParams.cropOutWidth;
        mFrameHeight = info->cropParams.cropOutHeight;
        mStride  = info->picWidth;
        mSliceHeight = info->picHeight;
        mPictureSize = mStride * mSliceHeight * 3 / 2;
        change_ddr_freq();

        if (!mThumbnailMode && needFlushBuffer) {
            if (useNativeBuffer) {
                ALOGI("%s, %d, info->numRefFrames: %d, info->has_b_frames: %d, def->nBufferCountMin: %d",
                      __FUNCTION__, __LINE__, info->numRefFrames, info->has_b_frames, def->nBufferCountMin);

                /*FIXME:plus additional one buffer for avoiding timed out,
                *because the number of native window reserved buffer is not sure.*/
                def->nBufferCountMin = info->numRefFrames + info->has_b_frames + 1 + 1;
            } else {
                ALOGI("%s, %d, info->numRefFrames: %d, info->has_b_frames: %d, def->nBufferCountActual: %d",
                      __FUNCTION__, __LINE__, info->numRefFrames, info->has_b_frames, def->nBufferCountActual);

                /*FIXME: When NativeWindow is null, We need calc actual buffer count manually.
                * 1: avoiding timed out, 1:reconstructed frame, 4:reserved buffers by SurfaceFlinger.*/
                def->nBufferCountActual = info->numRefFrames + info->has_b_frames + 1 + 1 + 4;

                /*fix Bug 375771 testCodecResetsH264WithSurface fail*/
                def->bPopulated = OMX_FALSE;
            }
        }

        updatePortDefinitions(true, true);
        (*mH264Dec_ReleaseRefBuffers)(mHandle);
        notify(OMX_EventPortSettingsChanged, 1, 0, NULL);
        mOutputPortSettingsChange = AWAITING_DISABLED;
        return true;
    }

    return false;
}

bool SPRDAVCDecoder::handleCropRectEvent(const CropParams *crop) {
    if (mCropWidth != crop->cropOutWidth ||
            mCropHeight != crop->cropOutHeight) {
        ALOGI("%s, crop w h: %d %d", __FUNCTION__, crop->cropOutWidth, crop->cropOutHeight);
        return true;
    }
    return false;
}

void SPRDAVCDecoder::drainOneOutputBuffer(int32_t picId, void* pBufferHeader, uint64 pts) {

    List<BufferInfo *> &outQueue = getPortQueue(kOutputPortIndex);

    List<BufferInfo *>::iterator it = outQueue.begin();
    while ((*it)->mHeader != (OMX_BUFFERHEADERTYPE*)pBufferHeader && it != outQueue.end()) {
        ++it;
    }
    CHECK((*it)->mHeader == (OMX_BUFFERHEADERTYPE*)pBufferHeader);

    BufferInfo *outInfo = *it;
    OMX_BUFFERHEADERTYPE *outHeader = outInfo->mHeader;

    outHeader->nFilledLen = mPictureSize;
    outHeader->nTimeStamp = (OMX_TICKS)pts;

    ALOGV("%s, %d, outHeader: %p, outHeader->pBuffer: %p, outHeader->nOffset: %d, outHeader->nFlags: %d, outHeader->nTimeStamp: %lld",
          __FUNCTION__, __LINE__, outHeader , outHeader->pBuffer, outHeader->nOffset, outHeader->nFlags, outHeader->nTimeStamp);

//    LOGI("%s, %d, outHeader->nTimeStamp: %d, outHeader->nFlags: %d, mPictureSize: %d", __FUNCTION__, __LINE__, outHeader->nTimeStamp, outHeader->nFlags, mPictureSize);
//   LOGI("%s, %d, out: %0x", __FUNCTION__, __LINE__, outHeader->pBuffer + outHeader->nOffset);

    outInfo->mOwnedByUs = false;
    outQueue.erase(it);
    outInfo = NULL;

    BufferCtrlStruct* pOutBufCtrl= (BufferCtrlStruct*)(outHeader->pOutputPortPrivate);
    pOutBufCtrl->iRefCount++;
    notifyFillBufferDone(outHeader);
}

bool SPRDAVCDecoder::drainAllOutputBuffers() {
    ALOGI("%s, %d", __FUNCTION__, __LINE__);

    List<BufferInfo *> &outQueue = getPortQueue(kOutputPortIndex);
    BufferInfo *outInfo;
    OMX_BUFFERHEADERTYPE *outHeader;

    int32_t picId;
    void* pBufferHeader;
    uint64 pts;

    while (!outQueue.empty() && mEOSStatus != OUTPUT_FRAMES_FLUSHED) {

        if (mHeadersDecoded &&
                MMDEC_OK == (*mH264Dec_GetLastDspFrm)(mHandle, &pBufferHeader, &picId, &pts) ) {
            List<BufferInfo *>::iterator it = outQueue.begin();
            while ((*it)->mHeader != (OMX_BUFFERHEADERTYPE*)pBufferHeader && it != outQueue.end()) {
                ++it;
            }
            CHECK((*it)->mHeader == (OMX_BUFFERHEADERTYPE*)pBufferHeader);
            outInfo = *it;
            outQueue.erase(it);
            outHeader = outInfo->mHeader;
            outHeader->nFilledLen = mPictureSize;
        } else {
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

void SPRDAVCDecoder::onPortFlushCompleted(OMX_U32 portIndex) {
    if (portIndex == kInputPortIndex) {
        mEOSStatus = INPUT_DATA_AVAILABLE;
        mNeedIVOP = true;
    }
}

void SPRDAVCDecoder::onPortEnableCompleted(OMX_U32 portIndex, bool enabled) {
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

void SPRDAVCDecoder::onPortFlushPrepare(OMX_U32 portIndex) {
    if(portIndex == OMX_DirOutput) {
        if( NULL!=mH264Dec_ReleaseRefBuffers )
            (*mH264Dec_ReleaseRefBuffers)(mHandle);
    }
}

void SPRDAVCDecoder::onReset() {
    mGettingPortFormat = OMX_FALSE;
    mSignalledError = false;

    //avoid process error after stop codec and restart codec when port settings changing.
    mOutputPortSettingsChange = NONE;
    mDecoderSawSPS = false;
    mDecoderSawPPS = false;
    mSPSDataSize = 0;
    mPPSDataSize = 0;
}

void SPRDAVCDecoder::updatePortDefinitions(bool updateCrop, bool updateInputSize) {
    OMX_PARAM_PORTDEFINITIONTYPE *outDef = &editPortInfo(kOutputPortIndex)->mDef;

    if (updateCrop) {
        mCropWidth = mFrameWidth;
        mCropHeight = mFrameHeight;
    }
    outDef->format.video.nFrameWidth = mStride;
    outDef->format.video.nFrameHeight = mSliceHeight;
    outDef->format.video.nStride = mStride;
    outDef->format.video.nSliceHeight = mSliceHeight;
    outDef->nBufferSize = mPictureSize;

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

    // when output format changes, input buffer size does not actually change
    if (updateInputSize) {
        inDef->nBufferSize = max(
                                 outDef->nBufferSize / mMinCompressionRatio,
                                 inDef->nBufferSize);
    }
}


// static
int32_t SPRDAVCDecoder::ExtMemAllocWrapper(
    void* aUserData, unsigned int size_extra) {
    return static_cast<SPRDAVCDecoder *>(aUserData)->VSP_malloc_cb(size_extra);
}

// static
int32_t SPRDAVCDecoder::MbinfoMemAllocWrapper(
    void* aUserData, unsigned int size_mbinfo, unsigned long *pPhyAddr) {

    return static_cast<SPRDAVCDecoder *>(aUserData)->VSP_malloc_mbinfo_cb(size_mbinfo, pPhyAddr);
}

// static
int32_t SPRDAVCDecoder::BindFrameWrapper(void *aUserData, void *pHeader) {
    return static_cast<SPRDAVCDecoder *>(aUserData)->VSP_bind_cb(pHeader);
}

// static
int32_t SPRDAVCDecoder::UnbindFrameWrapper(void *aUserData, void *pHeader) {
    return static_cast<SPRDAVCDecoder *>(aUserData)->VSP_unbind_cb(pHeader);
}

int SPRDAVCDecoder::VSP_malloc_cb(unsigned int size_extra) {

    ALOGI("%s, %d, mDecoderSwFlag: %d, mPictureSize: %d, size_extra: %d", __FUNCTION__, __LINE__, mDecoderSwFlag, mPictureSize, size_extra);

    int32_t picId;
    void* pBufferHeader;
    uint64 pts;

    /*fix Bug 381332 Whatsapp Recorded Video Getting Not Forward*/
    while (MMDEC_OK == (*mH264Dec_GetLastDspFrm)(mHandle, &pBufferHeader, &picId, &pts)) {
        drainOneOutputBuffer(picId, pBufferHeader, pts);
    }

    MMCodecBuffer extra_mem[MAX_MEM_TYPE];

    if (mDecoderSwFlag) {
        if (mCodecExtraBuffer != NULL) {
            free(mCodecExtraBuffer);
            mCodecExtraBuffer = NULL;
        }
        mCodecExtraBuffer = (uint8_t *)malloc(size_extra);
        if (mCodecExtraBuffer == NULL) {
            return -1;
        }
        extra_mem[SW_CACHABLE].common_buffer_ptr = mCodecExtraBuffer;
        extra_mem[SW_CACHABLE].common_buffer_ptr_phy = 0;
        extra_mem[SW_CACHABLE].size = size_extra;
    } else {
        mPbuf_mbinfo_idx = 0;

        if (mPbuf_extra_v != NULL) {
            if (mIOMMUEnabled) {
                mPmem_extra->free_iova(mIOMMUID, mPbuf_extra_p, mPbuf_extra_size);
            }
            mPmem_extra.clear();
            mPbuf_extra_v = NULL;
            mPbuf_extra_p = 0;
            mPbuf_extra_size = 0;
        }

        if (mIOMMUEnabled) {
            mPmem_extra = new MemoryHeapIon(SPRD_ION_DEV, size_extra, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
        } else {
            mPmem_extra = new MemoryHeapIon(SPRD_ION_DEV, size_extra, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
        }
        int fd = mPmem_extra->getHeapID();
        if(fd >= 0) {
            int ret;
            unsigned long phy_addr;
            size_t buffer_size;

            if (mIOMMUEnabled) {
                ret = mPmem_extra->get_iova(mIOMMUID, &phy_addr, &buffer_size);
            } else {
                ret = mPmem_extra->get_phy_addr_from_ion(&phy_addr, &buffer_size);
            }
            if(ret < 0) {
                ALOGE ("mPmem_extra: get phy addr fail %d",ret);
                return -1;
            }

            mPbuf_extra_p =phy_addr;
            mPbuf_extra_size = buffer_size;
            mPbuf_extra_v = (uint8_t *)mPmem_extra->getBase();
            ALOGI("pmem 0x%lx - %p - %zd", mPbuf_extra_p, mPbuf_extra_v, mPbuf_extra_size);

            extra_mem[HW_NO_CACHABLE].common_buffer_ptr = mPbuf_extra_v;
            extra_mem[HW_NO_CACHABLE].common_buffer_ptr_phy = mPbuf_extra_p;
            extra_mem[HW_NO_CACHABLE].size = size_extra;
        } else {
            ALOGE ("mPmem_extra: getHeapID fail %d", fd);
            return -1;
        }
    }

    (*mH264DecMemInit)(((SPRDAVCDecoder *)this)->mHandle, extra_mem);

    mHeadersDecoded = true;

    return 0;
}


int SPRDAVCDecoder::VSP_malloc_mbinfo_cb(unsigned int size_mbinfo, unsigned long *pPhyAddr) {

    int idx = mPbuf_mbinfo_idx;

    ALOGI("%s, %d, idx: %d, size_mbinfo: %d", __FUNCTION__, __LINE__, idx, size_mbinfo);

    if (mPbuf_mbinfo_v[idx] != NULL) {
        if (mIOMMUEnabled) {
            mPmem_mbinfo[idx]->free_iova(ION_MM, mPbuf_mbinfo_p[idx], mPbuf_mbinfo_size[idx]);
        }
        mPmem_mbinfo[idx].clear();
        mPbuf_mbinfo_v[idx] = NULL;
        mPbuf_mbinfo_p[idx] = 0;
        mPbuf_mbinfo_size[idx] = 0;
    }

    if (mIOMMUEnabled) {
        mPmem_mbinfo[idx] = new MemoryHeapIon(SPRD_ION_DEV, size_mbinfo, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
    } else {
        mPmem_mbinfo[idx] = new MemoryHeapIon(SPRD_ION_DEV, size_mbinfo, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
    }
    int fd = mPmem_mbinfo[idx]->getHeapID();
    if(fd >= 0) {
        int ret;
        unsigned long phy_addr;
        size_t buffer_size;

        if (mIOMMUEnabled) {
            ret = mPmem_mbinfo[idx]->get_iova(ION_MM, &phy_addr, &buffer_size);
        } else {
            ret = mPmem_mbinfo[idx]->get_phy_addr_from_ion(&phy_addr, &buffer_size);
        }
        if(ret < 0) {
            ALOGE ("mPmem_mbinfo[%d]: get phy addr fail %d", idx, ret);
            return -1;
        }

        mPbuf_mbinfo_p[idx] =phy_addr;
        mPbuf_mbinfo_size[idx] = buffer_size;
        mPbuf_mbinfo_v[idx] = (uint8_t *)mPmem_mbinfo[idx]->getBase();
        ALOGI("pmem 0x%lx - %p - %zd", mPbuf_mbinfo_p[idx], mPbuf_mbinfo_v[idx], mPbuf_mbinfo_size[idx]);

        *pPhyAddr = phy_addr;
    } else {
        ALOGE ("mPmem_mbinfo[%d]: getHeapID fail %d", idx, fd);
        return -1;
    }

    mPbuf_mbinfo_idx++;

    return 0;
}

int SPRDAVCDecoder::VSP_bind_cb(void *pHeader) {
    BufferCtrlStruct *pBufCtrl = (BufferCtrlStruct *)(((OMX_BUFFERHEADERTYPE *)pHeader)->pOutputPortPrivate);

    ALOGV("VSP_bind_cb, pBuffer: %p, pHeader: %p; iRefCount=%d",
          ((OMX_BUFFERHEADERTYPE *)pHeader)->pBuffer, pHeader,pBufCtrl->iRefCount);

    pBufCtrl->iRefCount++;
    return 0;
}

int SPRDAVCDecoder::VSP_unbind_cb(void *pHeader) {
    BufferCtrlStruct *pBufCtrl = (BufferCtrlStruct *)(((OMX_BUFFERHEADERTYPE *)pHeader)->pOutputPortPrivate);

    ALOGV("VSP_unbind_cb, pBuffer: %p, pHeader: %p; iRefCount=%d",
          ((OMX_BUFFERHEADERTYPE *)pHeader)->pBuffer, pHeader,pBufCtrl->iRefCount);

    if (pBufCtrl->iRefCount  > 0) {
        pBufCtrl->iRefCount--;
    }

    return 0;
}

OMX_ERRORTYPE SPRDAVCDecoder::getExtensionIndex(
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

bool SPRDAVCDecoder::openDecoder(const char* libName) {
    if(mLibHandle) {
        dlclose(mLibHandle);
    }

    ALOGI("openDecoder, lib: %s", libName);

    mLibHandle = dlopen(libName, RTLD_NOW);
    if(mLibHandle == NULL) {
        ALOGE("openDecoder, can't open lib: %s",libName);
        return false;
    }

    mH264DecInit = (FT_H264DecInit)dlsym(mLibHandle, "H264DecInit");
    if(mH264DecInit == NULL) {
        ALOGE("Can't find H264DecInit in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264DecGetInfo = (FT_H264DecGetInfo)dlsym(mLibHandle, "H264DecGetInfo");
    if(mH264DecGetInfo == NULL) {
        ALOGE("Can't find H264DecGetInfo in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264DecDecode = (FT_H264DecDecode)dlsym(mLibHandle, "H264DecDecode");
    if(mH264DecDecode == NULL) {
        ALOGE("Can't find H264DecDecode in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264DecRelease = (FT_H264DecRelease)dlsym(mLibHandle, "H264DecRelease");
    if(mH264DecRelease == NULL) {
        ALOGE("Can't find H264DecRelease in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264Dec_SetCurRecPic = (FT_H264Dec_SetCurRecPic)dlsym(mLibHandle, "H264Dec_SetCurRecPic");
    if(mH264Dec_SetCurRecPic == NULL) {
        ALOGE("Can't find H264Dec_SetCurRecPic in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264Dec_GetLastDspFrm = (FT_H264Dec_GetLastDspFrm)dlsym(mLibHandle, "H264Dec_GetLastDspFrm");
    if(mH264Dec_GetLastDspFrm == NULL) {
        ALOGE("Can't find H264Dec_GetLastDspFrm in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264Dec_ReleaseRefBuffers = (FT_H264Dec_ReleaseRefBuffers)dlsym(mLibHandle, "H264Dec_ReleaseRefBuffers");
    if(mH264Dec_ReleaseRefBuffers == NULL) {
        ALOGE("Can't find H264Dec_ReleaseRefBuffers in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264DecMemInit = (FT_H264DecMemInit)dlsym(mLibHandle, "H264DecMemInit");
    if(mH264DecMemInit == NULL) {
        ALOGE("Can't find H264DecMemInit in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264GetCodecCapability = (FT_H264GetCodecCapability)dlsym(mLibHandle, "H264GetCodecCapability");
    if(mH264GetCodecCapability == NULL) {
        ALOGE("Can't find H264GetCodecCapability in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264DecGetNALType = (FT_H264DecGetNALType)dlsym(mLibHandle, "H264DecGetNALType");
    if(mH264DecGetNALType == NULL) {
        ALOGE("Can't find H264DecGetNALType in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mH264DecSetparam = (FT_H264DecSetparam)dlsym(mLibHandle, "H264DecSetParameter");
    if(mH264DecSetparam == NULL) {
        ALOGE("Can't find H264DecSetParameter in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    return true;
}

void SPRDAVCDecoder::findCodecConfigData() {
    List<BufferInfo *> &inQueue = getPortQueue(kInputPortIndex);
    int queueSize = inQueue.size();
    List<BufferInfo *>::iterator itBuffer = inQueue.begin();
    OMX_BUFFERHEADERTYPE *bufferHeader;
    for (int i = 0; i < queueSize; i++) {
        bufferHeader = (*itBuffer)->mHeader;

        if (bufferHeader->nFlags & OMX_BUFFERFLAG_CODECCONFIG) {
            int nal_type, nal_ref_idc;
            uint8 *p = bufferHeader->pBuffer + bufferHeader->nOffset;
            //ALOGI("%s, %d, check Nal type: [%0x, %0x, %0x, %0x, %0x, %0x, %0x]",  __FUNCTION__, __LINE__, p[0], p[1], p[2], p[3], p[4], p[5]);

            MMDecRet decRet = (*mH264DecGetNALType)(mHandle, p, bufferHeader->nFilledLen, &nal_type, &nal_ref_idc);

            ALOGI("%s, queueSize:%d, bufferHeader:%p, nal_type:%d, nal_ref_idc:%d",
                  __FUNCTION__, queueSize, bufferHeader, nal_type, nal_ref_idc);

            if (decRet == MMDEC_OK) {
                if (!mDecoderSawSPS &&
                        nal_type == 0x7/*SPS*/ &&
                        bufferHeader->nFilledLen <= H264_HEADER_SIZE) {
                    mSPSDataSize = bufferHeader->nFilledLen;
                    memcpy(mSPSData, p, mSPSDataSize);
                } else if (!mDecoderSawPPS &&
                           nal_type == 0x8/*PPS*/ &&
                           bufferHeader->nFilledLen <= H264_HEADER_SIZE) {
                    mPPSDataSize = bufferHeader->nFilledLen;
                    memcpy(mPPSData, p, mPPSDataSize);
                }
            }
        }
        itBuffer++;
    }
}

}  // namespace android

android::SprdOMXComponent *createSprdOMXComponent(
    const char *name, const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData, OMX_COMPONENTTYPE **component) {
    return new android::SPRDAVCDecoder(name, callbacks, appData, component);
}
