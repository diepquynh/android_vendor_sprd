/*
 * Copyright (C) 2012 The Android Open Source Project
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

#define LOG_NDEBUG 0
#define LOG_TAG "SPRDMPEG4Encoder"
#include <utils/Log.h>

#include "m4v_h263_enc_api.h"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/Utils.h>

#include <MetadataBufferType.h>
#include <HardwareAPI.h>

#include <ui/Rect.h>
#include <ui/GraphicBufferMapper.h>
#include <dlfcn.h>

#include <linux/ion.h>

#include "MemoryHeapIon.h"

#include "SPRDMPEG4Encoder.h"
#include "ion_sprd.h"
#include "gralloc_priv.h"

#define VIDEOENC_CURRENT_OPT

namespace android {

template<class T>
static void InitOMXParams(T *params) {
    params->nSize = sizeof(T);
    params->nVersion.s.nVersionMajor = 1;
    params->nVersion.s.nVersionMinor = 0;
    params->nVersion.s.nRevision = 0;
    params->nVersion.s.nStep = 0;
}
void dump_yuv( uint8 * pBuffer,uint32 aInBufSize)
{
    FILE *fp = fopen("/data/encoder_in.yuv","ab");
    fwrite(pBuffer,1,aInBufSize,fp);
    fclose(fp);
}

/*
 * In case of orginal input height_org is not 16 aligned, we shoud copy original data to a larger space
 * Example: width_org = 640, height_org = 426,
 * We have to copy this data to a width_dst = 640 height_dst = 432 buffer which is 16 aligned.
 * Be careful, when doing this convert we MUST keep UV in their right position.
 *
 * FIXME: If width_org is not 16 aligned also, this would be much complicate
 *
 */
inline static void ConvertYUV420PlanarToYVU420SemiPlanar(
    uint8_t *inyuv, uint8_t* outyuv,
    int32_t width_org, int32_t height_org,
    int32_t width_dst, int32_t height_dst) {

    int32_t inYsize = width_org * height_org;
    uint32_t *outy = (uint32_t *) outyuv;
    uint16_t *incb = (uint16_t *) (inyuv + inYsize);
    uint16_t *incr = (uint16_t *) (inyuv + inYsize + (inYsize >> 2));

    /* Y copying */
    memcpy(outy, inyuv, inYsize);

    /* U & V copying */
    uint32_t *outUV = (uint32_t *) (outyuv + width_dst * height_dst);
    for (int32_t i = height_org >> 1; i > 0; --i) {
        for (int32_t j = width_org >> 2; j > 0; --j) {
            uint32_t tempU = *incb++;
            uint32_t tempV = *incr++;

            tempU = (tempU & 0xFF) | ((tempU & 0xFF00) << 8);
            tempV = (tempV & 0xFF) | ((tempV & 0xFF00) << 8);
            uint32_t temp = tempV | (tempU << 8);

            // Flip U and V
            *outUV++ = temp;
        }
    }
}

static int RGB_r_y[256];
static int RGB_r_cb[256];
static int RGB_r_cr_b_cb[256];
static int RGB_g_y[256];
static int RGB_g_cb[256];
static int RGB_g_cr[256];
static int RGB_b_y[256];
static int RGB_b_cr[256];
static  bool mConventFlag = false;

//init the convert table, the Transformation matrix is as:
// Y  =  ((66 * (_r)  + 129 * (_g)  + 25    * (_b)) >> 8) + 16
// Cb = ((-38 * (_r) - 74   * (_g)  + 112  * (_b)) >> 8) + 128
// Cr =  ((112 * (_r) - 94   * (_g)  - 18    * (_b)) >> 8) + 128
inline static void inittable()
{
    ALOGI("init table");
    int i = 0;
    for(i = 0; i < 256; i++) {
        RGB_r_y[i] =  ((66 * i) >> 8);
        RGB_r_cb[i] = ((38 * i) >> 8);
        RGB_r_cr_b_cb[i] = ((112 * i) >> 8 );
        RGB_g_y[i] = ((129 * i) >> 8) + 16 ;
        RGB_g_cb[i] = ((74 * i) >> 8) + 128 ;
        RGB_g_cr[i] = ((94 * i) >> 8) + 128;
        RGB_b_y[i] =  ((25 * i) >> 8);
        RGB_b_cr[i] = ((18 * i) >> 8);
    }
}
inline static void ConvertARGB888ToYVU420SemiPlanar(uint8_t *inrgb, uint8_t* outyuv,
        int32_t width_org, int32_t height_org, int32_t width_dst, int32_t height_dst) {
#define RGB2Y(_r, _g, _b)    (  *(RGB_r_y +_r)      +   *(RGB_g_y+_g)   +    *(RGB_b_y+_b))
#define RGB2CB(_r, _g, _b)   ( -*(RGB_r_cb +_r)     -   *(RGB_g_cb+_g)  +    *(RGB_r_cr_b_cb+_b))
#define RGB2CR(_r, _g, _b)   (  *(RGB_r_cr_b_cb +_r)-   *(RGB_g_cr+_g)  -    *(RGB_b_cr+_b))
    uint8_t *argb_ptr = inrgb;
    uint8_t *y_p = outyuv;
    uint8_t *vu_p = outyuv + width_dst * height_dst;

    if (NULL == inrgb || NULL ==  outyuv)
        return;
    if (0 != (width_org & 1) || 0 != (height_org & 1))
        return;
    if(!mConventFlag) {
        mConventFlag = true;
        inittable();
    }
    ALOGI("rgb2yuv start");
    uint8_t *y_ptr;
    uint8_t *vu_ptr;
    int64_t start_encode = systemTime();
    uint32 i ;
    uint32 j = height_org + 1;
    while(--j) {
        //the width_dst may be bigger than width_org,
        //make start byte in every line of Y and CbCr align
        y_ptr = y_p;
        y_p += width_dst;
        if (!(j & 1))  {
            vu_ptr = vu_p;
            vu_p += width_dst;
            i  = width_org / 2 + 1;
            while(--i) {
                //format abgr, litter endian
                *y_ptr++    = RGB2Y(*argb_ptr, *(argb_ptr+1), *(argb_ptr+2));
                *vu_ptr++ =  RGB2CR(*argb_ptr, *(argb_ptr+1), *(argb_ptr+2));
                *vu_ptr++  = RGB2CB(*argb_ptr, *(argb_ptr+1), *(argb_ptr+2));
                *y_ptr++    = RGB2Y(*(argb_ptr + 4), *(argb_ptr+5), *(argb_ptr+6));
                argb_ptr += 8;
            }
        } else {
            i  = width_org + 1;
            while(--i) {
                //format abgr, litter endian
                *y_ptr++ = RGB2Y(*argb_ptr, *(argb_ptr+1), *(argb_ptr+2));
                argb_ptr += 4;
            }
        }
    }
    int64_t end_encode = systemTime();
    ALOGI("rgb2yuv time: %d",(unsigned int)((end_encode-start_encode) / 1000000L));
}

#ifdef VIDEOENC_CURRENT_OPT
inline static void set_ddr_freq(const char* freq_in_khz)
{
    const char* const set_freq = "/sys/devices/platform/scxx30-dmcfreq.0/devfreq/scxx30-dmcfreq.0/ondemand/set_freq";

    FILE* fp = fopen(set_freq, "w");
    if (fp != NULL) {
        fprintf(fp, "%s", freq_in_khz);
        ALOGE("set ddr freq to %skhz", freq_in_khz);
        fclose(fp);
    } else {
        ALOGE("Failed to open %s", set_freq);
    }
}
#endif

SPRDMPEG4Encoder::SPRDMPEG4Encoder(
    const char *name,
    const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData,
    OMX_COMPONENTTYPE **component)
    : SprdSimpleOMXComponent(name, callbacks, appData, component),
      mHandle(new tagMP4Handle),
      mEncConfig(new MMEncConfig),
      mNumInputFrames(-1),
      mSetFreqCount(0),
      mVideoWidth(176),
      mVideoHeight(144),
      mVideoFrameRate(30),
      mVideoBitRate(192000),
      mVideoColorFormat(OMX_SPRD_COLOR_FormatYVU420SemiPlanar),
      mPFrames(29),
      mStoreMetaData(OMX_FALSE),
      mIOMMUEnabled(false),
      mIOMMUID(-1),
      mStarted(false),
      mSawInputEOS(false),
      mSignalledError(false),
      mKeyFrameRequested(false),
      mIsH263(0),
      mPbuf_inter(NULL),
      mPbuf_yuv_v(NULL),
      mPbuf_yuv_p(0),
      mPbuf_yuv_size(0),
      mPbuf_stream_v(NULL),
      mPbuf_stream_p(0),
      mPbuf_stream_size(0),
      mPbuf_extra_v(NULL),
      mPbuf_extra_p(0),
      mPbuf_extra_size(0),
      mLibHandle(NULL),
      mMP4EncGetCodecCapability(NULL),
      mMP4EncPreInit(NULL),
      mMP4EncInit(NULL),
      mMP4EncSetConf(NULL),
      mMP4EncGetConf(NULL),
      mMP4EncStrmEncode(NULL),
      mMP4EncGenHeader(NULL),
      mMP4EncRelease(NULL) {

    ALOGI("Construct SPRDMPEG4Encoder, this: 0x%p", (void *)this);

    CHECK(mHandle != NULL);
    memset(mHandle, 0, sizeof(tagMP4Handle));

    mHandle->videoEncoderData = NULL;
    mHandle->userData = this;

    memset(&mEncInfo, 0, sizeof(mEncInfo));

    CHECK_EQ(openEncoder("libomx_m4vh263enc_hw_sprd.so"), true);

    if (!strcmp(name, "OMX.sprd.h263.encoder")) {
        mIsH263 = 1;
    } else {
        mIsH263 = 0;
        CHECK(!strcmp(name, "OMX.sprd.mpeg4.encoder"));
    }

    initPorts();
    ALOGI("Construct SPRDMPEG4Encoder");

    if (MemoryHeapIon::IOMMU_is_enabled(ION_MM)) {
        mIOMMUEnabled = true;
        mIOMMUID = ION_MM;
    } else if (MemoryHeapIon::IOMMU_is_enabled(ION_VSP)) {
        mIOMMUEnabled = true;
        mIOMMUID = ION_VSP;
    }
    ALOGI("%s, is IOMMU enabled: %d, ID: %d", __FUNCTION__, mIOMMUEnabled, mIOMMUID);

    MMCodecBuffer InterMemBfr;
    uint32_t size_inter = MP4ENC_INTERNAL_BUFFER_SIZE;

    mPbuf_inter = (uint8_t *)malloc(size_inter);
	CHECK(mPbuf_inter != NULL);
    InterMemBfr.common_buffer_ptr = mPbuf_inter;
    InterMemBfr.common_buffer_ptr_phy = 0;
    InterMemBfr.size = size_inter;

    CHECK_EQ((*mMP4EncPreInit)(mHandle, &InterMemBfr), MMENC_OK);

    CHECK_EQ ((*mMP4EncGetCodecCapability)(mHandle, &mCapability), MMENC_OK);

#ifdef SPRD_DUMP_YUV
    mFile_yuv = fopen("/data/video.yuv", "wb");
#endif

#ifdef SPRD_DUMP_BS
    mFile_bs = fopen("/data/video.m4v", "wb");
#endif
}

SPRDMPEG4Encoder::~SPRDMPEG4Encoder() {
    ALOGI("Destruct SPRDMPEG4Encoder, this: 0x%p", (void *)this);

    releaseEncoder();

    List<BufferInfo *> &outQueue = getPortQueue(1);
    List<BufferInfo *> &inQueue = getPortQueue(0);
    CHECK(outQueue.empty());
    CHECK(inQueue.empty());

    if(mLibHandle)
    {
        dlclose(mLibHandle);
        mLibHandle = NULL;
    }

#ifdef SPRD_DUMP_YUV
    if (mFile_yuv) {
        fclose(mFile_yuv);
        mFile_yuv = NULL;
    }
#endif

#ifdef SPRD_DUMP_BS
    if (mFile_bs) {
        fclose(mFile_bs);
        mFile_bs = NULL;
    }
#endif
}

OMX_ERRORTYPE SPRDMPEG4Encoder::initEncParams() {

    CHECK(mEncConfig != NULL);
    memset(mEncConfig, 0, sizeof(MMEncConfig));

#ifdef VIDEOENC_CURRENT_OPT
    if (((mVideoWidth <= 720) && (mVideoHeight <= 480)) || ((mVideoWidth <= 480) && (mVideoHeight <= 720))) {
        set_ddr_freq("200000");
        mSetFreqCount ++;
    }
#endif

    MMCodecBuffer ExtraMemBfr;
    MMCodecBuffer StreamMemBfr;
    unsigned long phy_addr = 0;
    size_t size = 0;
    size_t size_of_yuv = ((mVideoWidth+15)&(~15)) * ((mVideoHeight+15)&(~15)) * 3/2;

    size_t size_extra = size_of_yuv << 1;
    size_extra += 320*2*sizeof(uint32);
    size_extra += 10*1024;
    if (mIOMMUEnabled) {
        mPmem_extra = new MemoryHeapIon("/dev/ion", size_extra, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
    } else {
        mPmem_extra = new MemoryHeapIon("/dev/ion", size_extra, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
    }
    if (mPmem_extra->getHeapID() < 0) {
        ALOGE("Failed to alloc extra buffer (%zd), getHeapID failed", size_extra);
        return OMX_ErrorInsufficientResources;
    } else
    {
        int ret;
        if(mIOMMUEnabled) {
            ret = mPmem_extra->get_iova(mIOMMUID, &phy_addr, &size);
        } else {
            ret = mPmem_extra->get_phy_addr_from_ion(&phy_addr, &size);
        }
        if (ret < 0)
        {
            ALOGE("Failed to alloc extra buffer, get phy addr failed");
            return OMX_ErrorInsufficientResources;
        } else
        {
            mPbuf_extra_v = (uint8_t*)mPmem_extra->getBase();
            mPbuf_extra_p = phy_addr;
            mPbuf_extra_size = size;
        }
    }

    size_t size_stream = size_of_yuv >> 1;
    if(mIOMMUEnabled) {
        mPmem_stream = new MemoryHeapIon("/dev/ion", size_stream, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
    } else {
        mPmem_stream = new MemoryHeapIon("/dev/ion", size_stream, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
    }
    if (mPmem_stream->getHeapID() < 0) {
        ALOGE("Failed to alloc stream buffer (%zd), getHeapID failed", size_stream);
        return OMX_ErrorInsufficientResources;
    } else
    {
        int ret;
        if(mIOMMUEnabled) {
            ret = mPmem_stream->get_iova(mIOMMUID, &phy_addr, &size);
        } else {
            ret = mPmem_stream->get_phy_addr_from_ion(&phy_addr, &size);
        }
        if (ret < 0)
        {
            ALOGE("Failed to alloc stream buffer, get phy addr failed");
            return OMX_ErrorInsufficientResources;
        } else
        {
            mPbuf_stream_v = (uint8_t*)mPmem_stream->getBase();
            mPbuf_stream_p = phy_addr;
            mPbuf_stream_size = size;
        }
    }

    ExtraMemBfr.common_buffer_ptr = mPbuf_extra_v;
    ExtraMemBfr.common_buffer_ptr_phy = mPbuf_extra_p;
    ExtraMemBfr.size = size_extra;

    StreamMemBfr.common_buffer_ptr = mPbuf_stream_v;
    StreamMemBfr.common_buffer_ptr_phy = mPbuf_stream_p;
    StreamMemBfr.size	= size_stream;

    mEncInfo.is_h263 = mIsH263;
    mEncInfo.frame_width = mVideoWidth;
    mEncInfo.frame_height = mVideoHeight;
    if (mVideoColorFormat == OMX_COLOR_FormatYUV420SemiPlanar) {
        mEncInfo.yuv_format = MMENC_YUV420SP_NV12;
    } else {
        mEncInfo.yuv_format = MMENC_YUV420SP_NV21;
    }
    mEncInfo.time_scale = 1000;
#ifdef ANTI_SHAKE
    mEncInfo.b_anti_shake = 1;
#else
    mEncInfo.b_anti_shake = 0;
#endif

    if ((*mMP4EncInit)(mHandle, &ExtraMemBfr,&StreamMemBfr, &mEncInfo) != MMENC_OK) {
        ALOGE("Failed to init mp4enc");
        return OMX_ErrorUndefined;
    }

    if ((*mMP4EncGetConf)(mHandle, mEncConfig)) {
        ALOGE("Failed to get default encoding parameters");
        return OMX_ErrorUndefined;
    }

    mEncConfig->h263En = mIsH263;
    mEncConfig->RateCtrlEnable = 1;
    mEncConfig->targetBitRate = mVideoBitRate;
    mEncConfig->FrameRate = mVideoFrameRate;
    mEncConfig->PFrames = mPFrames;
    mEncConfig->QP_IVOP = 4;
    mEncConfig->QP_PVOP = 4;
    mEncConfig->vbv_buf_size = mVideoBitRate/2;
    mEncConfig->profileAndLevel = 1;

    if ((*mMP4EncSetConf)(mHandle, mEncConfig)) {
        ALOGE("Failed to set default encoding parameters");
        return OMX_ErrorUndefined;
    }

    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDMPEG4Encoder::initEncoder() {
    CHECK(!mStarted);

    OMX_ERRORTYPE errType = OMX_ErrorNone;
    if (OMX_ErrorNone != (errType = initEncParams())) {
        ALOGE("Failed to initialized encoder params");
        mSignalledError = true;
        notify(OMX_EventError, OMX_ErrorUndefined, 0, 0);
        return errType;
    }

    mNumInputFrames = -1;  // 1st buffer for codec specific data
    mStarted = true;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE SPRDMPEG4Encoder::releaseEncoder() {

    (*mMP4EncRelease)(mHandle);

    if (mPbuf_inter != NULL)
    {
        free(mPbuf_inter);
        mPbuf_inter = NULL;
    }

    if (mPbuf_extra_v != NULL)
    {
        if(mIOMMUEnabled) {
            mPmem_extra->free_iova(mIOMMUID, mPbuf_extra_p, mPbuf_extra_size);
        }
        mPmem_extra.clear();
        mPbuf_extra_v = NULL;
        mPbuf_extra_p = 0;
        mPbuf_extra_size = 0;
    }

    if (mPbuf_stream_v != NULL)
    {
        if(mIOMMUEnabled) {
            mPmem_stream->free_iova(mIOMMUID, mPbuf_stream_p, mPbuf_stream_size);
        }
        mPmem_stream.clear();
        mPbuf_stream_v = NULL;
        mPbuf_stream_p = 0;
        mPbuf_stream_size = 0;
    }

    if (mPbuf_yuv_v != NULL)
    {
        if(mIOMMUEnabled) {
            mYUVInPmemHeap->free_iova(mIOMMUID, mPbuf_yuv_p, mPbuf_yuv_size);
        }
        mYUVInPmemHeap.clear();
        mPbuf_yuv_v = NULL;
        mPbuf_yuv_p = 0;
        mPbuf_yuv_size = 0;
    }

#ifdef VIDEOENC_CURRENT_OPT
    while (mSetFreqCount > 0) {
        set_ddr_freq("0");
        mSetFreqCount --;
    }
#endif

    delete mEncConfig;
    mEncConfig = NULL;

    delete mHandle;
    mHandle = NULL;

    mStarted = false;

    return OMX_ErrorNone;
}

void SPRDMPEG4Encoder::initPorts() {
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);

    const size_t kInputBufferSize = (((mVideoWidth+15)&(~15))  * ((mVideoHeight+15)&(~15))  * 3) >> 1;

    // 256 * 1024 is a magic number for PV's encoder, not sure why
    const size_t kOutputBufferSize =
        (kInputBufferSize > 256 * 1024)
        ? kInputBufferSize: 256 * 1024;

    def.nPortIndex = 0;
    def.eDir = OMX_DirInput;
    def.nBufferCountMin = kNumBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.nBufferSize = kInputBufferSize;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainVideo;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 1;

    def.format.video.cMIMEType = const_cast<char *>("video/raw");
    def.format.video.eCompressionFormat = OMX_VIDEO_CodingUnused;
    def.format.video.eColorFormat = OMX_SPRD_COLOR_FormatYVU420SemiPlanar;
    def.format.video.xFramerate = (mVideoFrameRate << 16);  // Q16 format
    def.format.video.nBitrate = mVideoBitRate;
    def.format.video.nFrameWidth = mVideoWidth;
    def.format.video.nFrameHeight = mVideoHeight;
    def.format.video.nStride = mVideoWidth;
    def.format.video.nSliceHeight = mVideoHeight;

    addPort(def);

    def.nPortIndex = 1;
    def.eDir = OMX_DirOutput;
    def.nBufferCountMin = kNumBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.nBufferSize = kOutputBufferSize;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainVideo;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 2;

    def.format.video.cMIMEType =
        (mIsH263 == 0)
        ? const_cast<char *>(MEDIA_MIMETYPE_VIDEO_MPEG4)
        : const_cast<char *>(MEDIA_MIMETYPE_VIDEO_H263);

    def.format.video.eCompressionFormat =
        (mIsH263 == 0)
        ? OMX_VIDEO_CodingMPEG4
        : OMX_VIDEO_CodingH263;

    def.format.video.eColorFormat = OMX_COLOR_FormatUnused;
    def.format.video.xFramerate = (0 << 16);  // Q16 format
    def.format.video.nBitrate = mVideoBitRate;
    def.format.video.nFrameWidth = mVideoWidth;
    def.format.video.nFrameHeight = mVideoHeight;
    def.format.video.nStride = mVideoWidth;
    def.format.video.nSliceHeight = mVideoHeight;

    addPort(def);
}

OMX_ERRORTYPE SPRDMPEG4Encoder::internalGetParameter(
    OMX_INDEXTYPE index, OMX_PTR params) {
    switch (index) {
    case OMX_IndexParamVideoErrorCorrection:
    {
        return OMX_ErrorNotImplemented;
    }

    case OMX_IndexParamVideoBitrate:
    {
        OMX_VIDEO_PARAM_BITRATETYPE *bitRate =
            (OMX_VIDEO_PARAM_BITRATETYPE *) params;

        if (bitRate->nPortIndex != 1) {
            return OMX_ErrorUndefined;
        }

        bitRate->eControlRate = OMX_Video_ControlRateVariable;
        bitRate->nTargetBitrate = mVideoBitRate;
        return OMX_ErrorNone;
    }

    case OMX_IndexParamVideoPortFormat:
    {
        OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams =
            (OMX_VIDEO_PARAM_PORTFORMATTYPE *)params;

        if (formatParams->nPortIndex > 1) {
            return OMX_ErrorUndefined;
        }

        if (formatParams->nIndex > 3) {
            return OMX_ErrorNoMore;
        }

        if (formatParams->nPortIndex == 0) {
            formatParams->eCompressionFormat = OMX_VIDEO_CodingUnused;
            if (formatParams->nIndex == 0) {
                formatParams->eColorFormat = OMX_SPRD_COLOR_FormatYVU420SemiPlanar;
            } else if (formatParams->nIndex == 1) {
                formatParams->eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
            } else if(formatParams->nIndex == 2) {
                formatParams->eColorFormat = OMX_COLOR_FormatYUV420Planar;
            } else {
                formatParams->eColorFormat = OMX_COLOR_FormatAndroidOpaque;
            }
        } else {
            formatParams->eCompressionFormat =
                (mIsH263 == 0)
                ? OMX_VIDEO_CodingMPEG4
                : OMX_VIDEO_CodingH263;

            formatParams->eColorFormat = OMX_COLOR_FormatUnused;
        }

        return OMX_ErrorNone;
    }

    case OMX_IndexParamVideoH263:
    {
        OMX_VIDEO_PARAM_H263TYPE *h263type =
            (OMX_VIDEO_PARAM_H263TYPE *)params;

        if (h263type->nPortIndex != 1) {
            return OMX_ErrorUndefined;
        }

        h263type->nAllowedPictureTypes =
            (OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP);
        h263type->eProfile = OMX_VIDEO_H263ProfileBaseline;
        h263type->eLevel = OMX_VIDEO_H263Level45;
        h263type->bPLUSPTYPEAllowed = OMX_FALSE;
        h263type->bForceRoundingTypeToZero = OMX_FALSE;
        h263type->nPictureHeaderRepetition = 0;
        h263type->nGOBHeaderInterval = 0;

        return OMX_ErrorNone;
    }

    case OMX_IndexParamVideoMpeg4:
    {
        OMX_VIDEO_PARAM_MPEG4TYPE *mpeg4type =
            (OMX_VIDEO_PARAM_MPEG4TYPE *)params;

        if (mpeg4type->nPortIndex != 1) {
            return OMX_ErrorUndefined;
        }

        mpeg4type->eProfile = OMX_VIDEO_MPEG4ProfileCore;
        mpeg4type->eLevel = OMX_VIDEO_MPEG4Level2;
        mpeg4type->nAllowedPictureTypes =
            (OMX_VIDEO_PictureTypeI | OMX_VIDEO_PictureTypeP);
        mpeg4type->nBFrames = 0;
        mpeg4type->nIDCVLCThreshold = 0;
        mpeg4type->bACPred = OMX_TRUE;
        mpeg4type->nMaxPacketSize = 256;
        mpeg4type->nTimeIncRes = 1000;
        mpeg4type->nHeaderExtension = 0;
        mpeg4type->bReversibleVLC = OMX_FALSE;

        return OMX_ErrorNone;
    }

    case OMX_IndexParamVideoProfileLevelQuerySupported:
    {
        OMX_VIDEO_PARAM_PROFILELEVELTYPE *profileLevel =
            (OMX_VIDEO_PARAM_PROFILELEVELTYPE *)params;

        if (profileLevel->nPortIndex != 1) {
            return OMX_ErrorUndefined;
        }

        if (profileLevel->nProfileIndex > 0) {
            return OMX_ErrorNoMore;
        }

        if (mIsH263 == 1)  {
            profileLevel->eProfile = OMX_VIDEO_H263ProfileBaseline;
            profileLevel->eLevel = OMX_VIDEO_H263Level45;
        } else {
            profileLevel->eProfile = OMX_VIDEO_MPEG4ProfileCore;
            profileLevel->eLevel = OMX_VIDEO_MPEG4Level2;
        }

        return OMX_ErrorNone;
    }

    case OMX_IndexParamStoreMetaDataBuffer:
    {
        StoreMetaDataInBuffersParams *pStoreMetaData = (StoreMetaDataInBuffersParams *)params;
        if (pStoreMetaData->nPortIndex != 0) {
            ALOGE("%s: StoreMetadataInBuffersParams.nPortIndex not zero!",
                    __FUNCTION__);
            return OMX_ErrorUndefined;
        }
        pStoreMetaData->bStoreMetaData = mStoreMetaData;
        return OMX_ErrorNone;
    }

    case OMX_IndexParamDescribeColorFormat:
    {
        DescribeColorFormatParams *pDescribeColorFormat = (DescribeColorFormatParams *)params;

        MediaImage &image = pDescribeColorFormat->sMediaImage;
        memset(&image, 0, sizeof(image));

        image.mType = MediaImage::MEDIA_IMAGE_TYPE_UNKNOWN;
        image.mNumPlanes = 0;

        const OMX_COLOR_FORMATTYPE fmt = pDescribeColorFormat->eColorFormat;
        image.mWidth = pDescribeColorFormat->nFrameWidth;
        image.mHeight = pDescribeColorFormat->nFrameHeight;

        ALOGI("%s, DescribeColorFormat: 0x%x, w h = %d %d", __FUNCTION__,
                pDescribeColorFormat->eColorFormat,
                pDescribeColorFormat->nFrameWidth, pDescribeColorFormat->nFrameHeight);

        if (fmt != OMX_SPRD_COLOR_FormatYVU420SemiPlanar &&
            fmt != OMX_COLOR_FormatYUV420SemiPlanar &&
            fmt != OMX_COLOR_FormatYUV420Planar) {
            ALOGW("do not know color format 0x%x = %d", fmt, fmt);
            return OMX_ErrorUnsupportedSetting;
        }

        // TEMPORARY FIX for some vendors that advertise sliceHeight as 0
        if (pDescribeColorFormat->nStride != 0 && pDescribeColorFormat->nSliceHeight == 0) {
            ALOGW("using sliceHeight=%u instead of what codec advertised (=0)",
                    pDescribeColorFormat->nFrameHeight);
            pDescribeColorFormat->nSliceHeight = pDescribeColorFormat->nFrameHeight;
        }

        // we need stride and slice-height to be non-zero
        if (pDescribeColorFormat->nStride == 0 || pDescribeColorFormat->nSliceHeight == 0) {
            ALOGW("cannot describe color format 0x%x = %d with stride=%u and sliceHeight=%u",
                    fmt, fmt, pDescribeColorFormat->nStride, pDescribeColorFormat->nSliceHeight);
            return OMX_ErrorBadParameter;
        }

        // set-up YUV format
        image.mType = MediaImage::MEDIA_IMAGE_TYPE_YUV;
        image.mNumPlanes = 3;
        image.mBitDepth = 8;
        image.mPlane[image.Y].mOffset = 0;
        image.mPlane[image.Y].mColInc = 1;
        image.mPlane[image.Y].mRowInc = pDescribeColorFormat->nStride;
        image.mPlane[image.Y].mHorizSubsampling = 1;
        image.mPlane[image.Y].mVertSubsampling = 1;

        switch (fmt) {
            case OMX_SPRD_COLOR_FormatYVU420SemiPlanar:
            // NV21
            image.mPlane[image.V].mOffset = pDescribeColorFormat->nStride*pDescribeColorFormat->nSliceHeight;
            image.mPlane[image.V].mColInc = 2;
            image.mPlane[image.V].mRowInc = pDescribeColorFormat->nStride;
            image.mPlane[image.V].mHorizSubsampling = 2;
            image.mPlane[image.V].mVertSubsampling = 2;

            image.mPlane[image.U].mOffset = image.mPlane[image.V].mOffset + 1;
            image.mPlane[image.U].mColInc = 2;
            image.mPlane[image.U].mRowInc = pDescribeColorFormat->nStride;
            image.mPlane[image.U].mHorizSubsampling = 2;
            image.mPlane[image.U].mVertSubsampling = 2;
            break;

            case OMX_COLOR_FormatYUV420SemiPlanar:
                // FIXME: NV21 for sw-encoder, NV12 for decoder and hw-encoder
                // NV12
                image.mPlane[image.U].mOffset = pDescribeColorFormat->nStride*pDescribeColorFormat->nSliceHeight;
                image.mPlane[image.U].mColInc = 2;
                image.mPlane[image.U].mRowInc = pDescribeColorFormat->nStride;
                image.mPlane[image.U].mHorizSubsampling = 2;
                image.mPlane[image.U].mVertSubsampling = 2;

                image.mPlane[image.V].mOffset = image.mPlane[image.U].mOffset + 1;
                image.mPlane[image.V].mColInc = 2;
                image.mPlane[image.V].mRowInc = pDescribeColorFormat->nStride;
                image.mPlane[image.V].mHorizSubsampling = 2;
                image.mPlane[image.V].mVertSubsampling = 2;
                break;

            case OMX_COLOR_FormatYUV420Planar: // used for YV12
                image.mPlane[image.U].mOffset = pDescribeColorFormat->nStride*pDescribeColorFormat->nSliceHeight;
                image.mPlane[image.U].mColInc = 1;
                image.mPlane[image.U].mRowInc = pDescribeColorFormat->nStride / 2;
                image.mPlane[image.U].mHorizSubsampling = 2;
                image.mPlane[image.U].mVertSubsampling = 2;

                image.mPlane[image.V].mOffset = image.mPlane[image.U].mOffset
                        + (pDescribeColorFormat->nStride * pDescribeColorFormat->nSliceHeight / 4);
                image.mPlane[image.V].mColInc = 1;
                image.mPlane[image.V].mRowInc = pDescribeColorFormat->nStride / 2;
                image.mPlane[image.V].mHorizSubsampling = 2;
                image.mPlane[image.V].mVertSubsampling = 2;
                break;

            default:
                TRESPASS();
        }

        return OMX_ErrorNone;
    }

    default:
        return SprdSimpleOMXComponent::internalGetParameter(index, params);
    }
}

OMX_ERRORTYPE SPRDMPEG4Encoder::internalSetParameter(
    OMX_INDEXTYPE index, const OMX_PTR params) {
    switch (index) {
    case OMX_IndexParamVideoErrorCorrection:
    {
        return OMX_ErrorNotImplemented;
    }

    case OMX_IndexParamVideoBitrate:
    {
        OMX_VIDEO_PARAM_BITRATETYPE *bitRate =
            (OMX_VIDEO_PARAM_BITRATETYPE *) params;

        if (bitRate->nPortIndex != 1 ||
                bitRate->eControlRate != OMX_Video_ControlRateVariable) {
            return OMX_ErrorUndefined;
        }

        mVideoBitRate = bitRate->nTargetBitrate;
        return OMX_ErrorNone;
    }

    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE *def =
            (OMX_PARAM_PORTDEFINITIONTYPE *)params;
        if (def->nPortIndex > 1) {
            return OMX_ErrorUndefined;
        }

        if (def->nPortIndex == 0) {
            if (def->format.video.eCompressionFormat != OMX_VIDEO_CodingUnused ||
                    (def->format.video.eColorFormat != OMX_COLOR_FormatYUV420Flexible &&
                     def->format.video.eColorFormat != OMX_SPRD_COLOR_FormatYVU420SemiPlanar &&
                     def->format.video.eColorFormat != OMX_COLOR_FormatYUV420SemiPlanar &&
                     def->format.video.eColorFormat != OMX_COLOR_FormatYUV420Planar &&
                     def->format.video.eColorFormat != OMX_COLOR_FormatAndroidOpaque)) {
                return OMX_ErrorUndefined;
            }
        } else {
            if (((mIsH263 == 0)  &&
                    def->format.video.eCompressionFormat != OMX_VIDEO_CodingMPEG4) ||
                    ((mIsH263 == 1)  &&
                     def->format.video.eCompressionFormat != OMX_VIDEO_CodingH263) ||
                    (def->format.video.eColorFormat != OMX_COLOR_FormatUnused)) {
                return OMX_ErrorUndefined;
            }
        }

        // Enlarge the buffer size for both input and output port
        if(def->nPortIndex <= 1) {
            uint32_t bufferSize = ((def->format.video.nFrameWidth+15)&(~15))*((def->format.video.nFrameHeight+15)&(~15))*3/2;
            if(bufferSize > def->nBufferSize) {
                def->nBufferSize = bufferSize;
            }
        }

        //translate Flexible 8-bit YUV format to our default YUV format
        if (def->format.video.eColorFormat == OMX_COLOR_FormatYUV420Flexible) {
            ALOGI("internalSetParameter, translate Flexible 8-bit YUV format to SPRD YVU420SemiPlanar");
            def->format.video.eColorFormat = OMX_SPRD_COLOR_FormatYVU420SemiPlanar;
        }

        OMX_ERRORTYPE err = SprdSimpleOMXComponent::internalSetParameter(index, params);
        if (OMX_ErrorNone != err) {
            return err;
        }

        if (def->nPortIndex == 0) {
            mVideoWidth = def->format.video.nFrameWidth;
            mVideoHeight = def->format.video.nFrameHeight;
            mVideoFrameRate = def->format.video.xFramerate >> 16;
            mVideoColorFormat = def->format.video.eColorFormat;
        } else {
            mVideoBitRate = def->format.video.nBitrate;
        }

        return OMX_ErrorNone;
    }

    case OMX_IndexParamStandardComponentRole:
    {
        const OMX_PARAM_COMPONENTROLETYPE *roleParams =
            (const OMX_PARAM_COMPONENTROLETYPE *)params;

        if (strncmp((const char *)roleParams->cRole,
                    (mIsH263 == 1)
                    ? "video_encoder.h263": "video_encoder.mpeg4",
                    OMX_MAX_STRINGNAME_SIZE - 1)) {
            return OMX_ErrorUndefined;
        }

        return OMX_ErrorNone;
    }

    case OMX_IndexParamVideoPortFormat:
    {
        const OMX_VIDEO_PARAM_PORTFORMATTYPE *formatParams =
            (const OMX_VIDEO_PARAM_PORTFORMATTYPE *)params;

        if (formatParams->nPortIndex > 1) {
            return OMX_ErrorUndefined;
        }

        if (formatParams->nIndex > 3) {
            return OMX_ErrorNoMore;
        }

        if (formatParams->nPortIndex == 0) {
            if (formatParams->eCompressionFormat != OMX_VIDEO_CodingUnused ||
                    ((formatParams->nIndex == 0 &&
                      formatParams->eColorFormat != OMX_SPRD_COLOR_FormatYVU420SemiPlanar) ||
                     (formatParams->nIndex == 1 &&
                      formatParams->eColorFormat != OMX_COLOR_FormatYUV420SemiPlanar) ||
                      (formatParams->nIndex == 2 &&
                      formatParams->eColorFormat != OMX_COLOR_FormatYUV420Planar) ||
                      (formatParams->nIndex == 3 &&
                      formatParams->eColorFormat != OMX_COLOR_FormatAndroidOpaque))) {
                return OMX_ErrorUndefined;
            }
            mVideoColorFormat = formatParams->eColorFormat;
        } else {
            if (((mIsH263 == 1)  &&
                    formatParams->eCompressionFormat != OMX_VIDEO_CodingH263) ||
                    ((mIsH263 == 0)  &&
                     formatParams->eCompressionFormat != OMX_VIDEO_CodingMPEG4) ||
                    formatParams->eColorFormat != OMX_COLOR_FormatUnused) {
                return OMX_ErrorUndefined;
            }
        }

        return OMX_ErrorNone;
    }

    case OMX_IndexParamVideoH263:
    {
        OMX_VIDEO_PARAM_H263TYPE *h263type =
            (OMX_VIDEO_PARAM_H263TYPE *)params;

        if (h263type->nPortIndex != 1) {
            return OMX_ErrorUndefined;
        }

        mPFrames = h263type->nPFrames;
        ALOGI("%s, H263 mPFrames: %d",__FUNCTION__,mPFrames);

        if (h263type->eProfile != OMX_VIDEO_H263ProfileBaseline ||
                h263type->eLevel != OMX_VIDEO_H263Level45 ||
                (h263type->nAllowedPictureTypes & OMX_VIDEO_PictureTypeB) ||
                h263type->bPLUSPTYPEAllowed != OMX_FALSE ||
                h263type->bForceRoundingTypeToZero != OMX_FALSE ||
                h263type->nPictureHeaderRepetition != 0 ||
                h263type->nGOBHeaderInterval != 0) {
            return OMX_ErrorUndefined;
        }

        return OMX_ErrorNone;
    }

    case OMX_IndexParamVideoMpeg4:
    {
        OMX_VIDEO_PARAM_MPEG4TYPE *mpeg4type =
            (OMX_VIDEO_PARAM_MPEG4TYPE *)params;

        if (mpeg4type->nPortIndex != 1) {
            return OMX_ErrorUndefined;
        }

        mPFrames = mpeg4type->nPFrames;
        ALOGI("%s, Mpeg4 mPFrames: %d",__FUNCTION__,mPFrames);

        if (mpeg4type->eProfile != OMX_VIDEO_MPEG4ProfileCore ||
                mpeg4type->eLevel != OMX_VIDEO_MPEG4Level2 ||
                (mpeg4type->nAllowedPictureTypes & OMX_VIDEO_PictureTypeB) ||
                mpeg4type->nBFrames != 0 ||
                mpeg4type->nIDCVLCThreshold != 0 ||
                mpeg4type->bACPred != OMX_TRUE ||
                mpeg4type->nMaxPacketSize != 256 ||
                mpeg4type->nTimeIncRes != 1000 ||
                mpeg4type->nHeaderExtension != 0 ||
                mpeg4type->bReversibleVLC != OMX_FALSE) {
            return OMX_ErrorUndefined;
        }

        return OMX_ErrorNone;
    }

    case OMX_IndexParamStoreMetaDataBuffer:
    {
        StoreMetaDataInBuffersParams *pStoreMetaData = (StoreMetaDataInBuffersParams *)params;
        if (pStoreMetaData->nPortIndex != 0) {
            ALOGE("%s: StoreMetadataInBuffersParams.nPortIndex not zero!",
                    __FUNCTION__);
            return OMX_ErrorUndefined;
        }

        mStoreMetaData = pStoreMetaData->bStoreMetaData;
        ALOGV("StoreMetaDataInBuffers set to: %s",
                mStoreMetaData ? " true" : "false");

        return OMX_ErrorNone;
    }

    default:
        return SprdSimpleOMXComponent::internalSetParameter(index, params);
    }
}

OMX_ERRORTYPE SPRDMPEG4Encoder::setConfig(
    OMX_INDEXTYPE index, const OMX_PTR params) {
    switch (index) {
        case OMX_IndexConfigVideoIntraVOPRefresh:
        {
            OMX_CONFIG_INTRAREFRESHVOPTYPE *pConfigIntraRefreshVOP =
                (OMX_CONFIG_INTRAREFRESHVOPTYPE *)params;

            if (pConfigIntraRefreshVOP->nPortIndex != kOutputPortIndex) {
                return OMX_ErrorBadPortIndex;
            }

            mKeyFrameRequested = pConfigIntraRefreshVOP->IntraRefreshVOP;
            return OMX_ErrorNone;
        }

        default:
            return SprdSimpleOMXComponent::setConfig(index, params);
    }
}

OMX_ERRORTYPE SPRDMPEG4Encoder::getExtensionIndex(
    const char *name, OMX_INDEXTYPE *index)
{
    if(strcmp(name, "OMX.google.android.index.storeMetaDataInBuffers") == 0) {
        *index = (OMX_INDEXTYPE) OMX_IndexParamStoreMetaDataBuffer;
        return OMX_ErrorNone;
    } else if (strcmp(name, "OMX.google.android.index.describeColorFormat") == 0) {
        *index = (OMX_INDEXTYPE) OMX_IndexParamDescribeColorFormat;
        return OMX_ErrorNone;
    }

    return SprdSimpleOMXComponent::getExtensionIndex(name, index);
}

void SPRDMPEG4Encoder::onQueueFilled(OMX_U32 portIndex) {
    if (mSignalledError || mSawInputEOS) {
        return;
    }
    if (!mStarted) {
        if (OMX_ErrorNone != initEncoder()) {
            return;
        }
    }
    List<BufferInfo *> &inQueue = getPortQueue(0);
    List<BufferInfo *> &outQueue = getPortQueue(1);

    while (!mSawInputEOS && !inQueue.empty() && !outQueue.empty()) {
        BufferInfo *inInfo = *inQueue.begin();
        OMX_BUFFERHEADERTYPE *inHeader = inInfo->mHeader;
        BufferInfo *outInfo = *outQueue.begin();
        OMX_BUFFERHEADERTYPE *outHeader = outInfo->mHeader;

        outHeader->nTimeStamp = 0;
        outHeader->nFlags = 0;
        outHeader->nOffset = 0;
        outHeader->nFilledLen = 0;
        outHeader->nOffset = 0;

        uint8_t *outPtr = (uint8_t *) outHeader->pBuffer;
        uint32_t dataLength = outHeader->nAllocLen;

        if (mNumInputFrames < 0) {
            MMEncOut encOut;
            if ((*mMP4EncGenHeader)(mHandle, &encOut) != MMENC_OK) {
                ALOGE("Failed to generate VOL header");
                mSignalledError = true;
                notify(OMX_EventError, OMX_ErrorUndefined, 0, 0);
                return;
            }

#ifdef SPRD_DUMP_BS
            if (mFile_bs != NULL) {
                fwrite(encOut.pOutBuf, 1, encOut.strmSize, mFile_bs);
            }
#endif

            dataLength = encOut.strmSize;
            memcpy(outPtr, encOut.pOutBuf, dataLength);

            ALOGV("Output VOL header: %d bytes", dataLength);
            ++mNumInputFrames;
            outHeader->nFlags |= OMX_BUFFERFLAG_CODECCONFIG;
            outHeader->nFilledLen = dataLength;
            outQueue.erase(outQueue.begin());
            outInfo->mOwnedByUs = false;
            notifyFillBufferDone(outHeader);
            return;
        }

        // Save the input buffer info so that it can be
        // passed to an output buffer
        InputBufferInfo info;
        info.mTimeUs = inHeader->nTimeStamp;
        info.mFlags = inHeader->nFlags;
        mInputBufferInfoVec.push(info);

        if (inHeader->nFlags & OMX_BUFFERFLAG_EOS) {
            mSawInputEOS = true;
        }

        if (inHeader->nFilledLen > 0) {
            const void *inData = inHeader->pBuffer + inHeader->nOffset;
            uint8_t *inputData = (uint8_t *) inData;
            CHECK(inputData != NULL);


            MMEncIn vid_in;
            MMEncOut vid_out;
            memset(&vid_in, 0, sizeof(vid_in));
            memset(&vid_out, 0, sizeof(vid_out));
            uint8_t* py = NULL;
            uint8_t* py_phy = NULL;
            uint32_t width = 0;
            uint32_t height = 0;
            uint32_t x = 0;
            uint32_t y = 0;
            bool needUnmap = false;
            int bufFd = -1;
            unsigned long iova = 0;
            size_t iovaLen = 0;

            if (mStoreMetaData) {
                unsigned int *mataData = (unsigned int *)inputData;
                unsigned int type = *mataData++;
                if (type == kMetadataBufferTypeCameraSource) {
                    py_phy = (uint8_t*)(*(unsigned long *)mataData);
                    mataData += sizeof(unsigned long)/sizeof(unsigned int);
                    py = (uint8_t*)(*(unsigned long *)mataData);
                    mataData += sizeof(unsigned long)/sizeof(unsigned int);
                    width = (uint32_t)(*((uint32_t *) mataData++));
                    height = (uint32_t)(*((uint32_t *) mataData++));
                    x = (uint32_t)(*((uint32_t *) mataData++));
                    y = (uint32_t)(*((uint32_t *) mataData));
                } else if (type == kMetadataBufferTypeGrallocSource) {
                    buffer_handle_t buf = *((buffer_handle_t *)(inputData + 4));
                    struct private_handle_t *private_h = (struct private_handle_t*)buf;

                    ALOGI("format:0x%x, usage:0x%x", private_h->format, private_h->usage);

                    if ((mPbuf_yuv_v == NULL) &&
                        !((mVideoColorFormat == OMX_SPRD_COLOR_FormatYVU420SemiPlanar||
                        mVideoColorFormat == OMX_COLOR_FormatYUV420SemiPlanar)&&
                        (private_h->usage == GRALLOC_USAGE_HW_VIDEO_ENCODER))) {

                        size_t yuv_size = ((mVideoWidth+15)&(~15)) * ((mVideoHeight+15)&(~15)) *3/2;
                        if(mIOMMUEnabled) {
                            mYUVInPmemHeap = new MemoryHeapIon("/dev/ion", yuv_size, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
                        } else {
                            mYUVInPmemHeap = new MemoryHeapIon("/dev/ion", yuv_size, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
                        }
                        if (mYUVInPmemHeap->getHeapID() < 0) {
                            ALOGE("Failed to alloc yuv buffer");
                            return;
                        }
                        int ret;
                        unsigned long phy_addr;
                        size_t buffer_size;

                        if(mIOMMUEnabled) {
                            ret = mYUVInPmemHeap->get_iova(mIOMMUID, &phy_addr, &buffer_size);
                        } else {
                            ret = mYUVInPmemHeap->get_phy_addr_from_ion(&phy_addr, &buffer_size);
                        }
                        if(ret) {
                            ALOGE("Failed to get_phy_addr_from_ion %d", ret);
                            return;
                        }
                        mPbuf_yuv_v =(uint8_t *) mYUVInPmemHeap->getBase();
                        mPbuf_yuv_p = phy_addr;
                        mPbuf_yuv_size = buffer_size;
                    }


                    py = mPbuf_yuv_v;
                    py_phy = (uint8_t *)mPbuf_yuv_p;

                    GraphicBufferMapper &mapper = GraphicBufferMapper::get();
                    Rect bounds((mVideoWidth+15)&(~15), (mVideoHeight+15)&(~15));

                    void* vaddr;
                    if (mapper.lock(buf, GRALLOC_USAGE_SW_READ_OFTEN|GRALLOC_USAGE_SW_WRITE_NEVER, bounds, &vaddr)) {
                        return;
                    }

                    if (mVideoColorFormat == OMX_COLOR_FormatYUV420Planar) {
                        ConvertYUV420PlanarToYVU420SemiPlanar((uint8_t*)vaddr, py, mVideoWidth, mVideoHeight,
                                                             (mVideoWidth + 15) & (~15), (mVideoHeight + 15) & (~15));
                    } else if(mVideoColorFormat == OMX_COLOR_FormatAndroidOpaque) {
                        if(private_h->format == HAL_PIXEL_FORMAT_YCrCb_420_SP) {
                            if (private_h->usage == GRALLOC_USAGE_HW_VIDEO_ENCODER) {
                                unsigned long py_addr=0;
                                size_t buf_size=0;
                                int fd = private_h->share_fd;
                                int ret = 0;

                                if (mIOMMUEnabled) {
                                    ret = MemoryHeapIon::Get_iova(ION_MM, fd,&py_addr,&buf_size);
                                } else {
                                    ret = MemoryHeapIon::Get_phy_addr_from_ion(fd,&py_addr,&buf_size);
                                }
                                if(ret) {
                                    ALOGE("Failed to Get_iova or Get_phy_addr_from_ion %d", ret);
                                    return;
                                }
                                if (mIOMMUEnabled) {
                                    needUnmap = true;
                                    bufFd = fd;
                                    iova = py_addr;
                                    iovaLen = buf_size;
                                }

                                py = (uint8_t*)vaddr;
                                py_phy = (uint8_t*)py_addr;
                            } else {
                                memcpy(py, vaddr, ((mVideoWidth+15)&(~15)) * ((mVideoHeight+15)&(~15)) * 3/2);
                            }
                        }
                        else {
                            ConvertARGB888ToYVU420SemiPlanar((uint8_t*)vaddr, py, mVideoWidth, mVideoHeight, (mVideoWidth+15)&(~15), (mVideoHeight+15)&(~15));
                        }
                    } else {
                        if(private_h->usage == GRALLOC_USAGE_HW_VIDEO_ENCODER) {
                            unsigned long ion_addr=0;
                            size_t ion_size=0;
                            int fd = private_h->share_fd;

                            if (0  == mIOMMUEnabled) {
                                if (0 != MemoryHeapIon::Get_phy_addr_from_ion(fd,&ion_addr,&ion_size)) {
                                    return;
                                }
                            } else {
                                if (MemoryHeapIon::Get_iova(mIOMMUID, fd,&ion_addr,&ion_size)) {
                                    return;
                                }
                            }

                        if (mIOMMUEnabled) {
                            needUnmap = true;
                            bufFd = fd;
                            iova = ion_addr;
                            iovaLen = ion_size;
                        }

                            py = (uint8_t*)vaddr;
                            py_phy = (uint8_t*)ion_addr;
                            //ALOGD("%s, mIOMMUEnabled = %d, fd = 0x%lx, vaddr = 0x%lx, ion_addr = 0x%lx",__FUNCTION__, mIOMMUEnabled, fd, vaddr, ion_addr);
                        } else {
                            memcpy(py, vaddr, ((mVideoWidth+15)&(~15)) * ((mVideoHeight+15)&(~15)) * 3/2);
                        }
                    }

                    if (mapper.unlock(buf)) {
                        return;
                    }
                } else {
                    ALOGE("Error MetadataBufferType %d", type);
                    return;
                }
            } else {
                if (mPbuf_yuv_v == NULL) {
                    size_t yuv_size = ((mVideoWidth+15)&(~15))*((mVideoHeight+15)&(~15))*3/2;
                    if(mIOMMUEnabled) {
                        mYUVInPmemHeap = new MemoryHeapIon("/dev/ion", yuv_size, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_SYSTEM);
                    } else {
                        mYUVInPmemHeap = new MemoryHeapIon("/dev/ion", yuv_size, MemoryHeapIon::NO_CACHING, ION_HEAP_ID_MASK_MM);
                    }
                    if (mYUVInPmemHeap->getHeapID() < 0) {
                        ALOGE("Failed to alloc yuv buffer");
                        return;
                    }
                    int ret;
                    unsigned long phy_addr;
                    size_t buffer_size;

                    if(mIOMMUEnabled) {
                        ret = mYUVInPmemHeap->get_iova(mIOMMUID, &phy_addr, &buffer_size);
                    } else {
                        ret = mYUVInPmemHeap->get_phy_addr_from_ion(&phy_addr, &buffer_size);
                    }
                    if(ret) {
                        ALOGE("Failed to get_phy_addr_from_ion %d", ret);
                        return;
                    }
                    mPbuf_yuv_v =(uint8_t *) mYUVInPmemHeap->getBase();
                    mPbuf_yuv_p = phy_addr;
                    mPbuf_yuv_size = buffer_size;
                }


                py = mPbuf_yuv_v;
                py_phy = (uint8_t*)mPbuf_yuv_p;

                if (mVideoColorFormat == OMX_COLOR_FormatYUV420Planar) {
                    ConvertYUV420PlanarToYVU420SemiPlanar(inputData, py, mVideoWidth, mVideoHeight,
                                                         (mVideoWidth + 15) & (~15), (mVideoHeight + 15) & (~15));
                } else if(mVideoColorFormat == OMX_COLOR_FormatAndroidOpaque) {
                    ConvertARGB888ToYVU420SemiPlanar(inputData, py, mVideoWidth, mVideoHeight, (mVideoWidth+15)&(~15), (mVideoHeight+15)&(~15));
                } else {
                    memcpy(py, inputData, ((mVideoWidth+15)&(~15)) * ((mVideoHeight+15)&(~15)) * 3/2);
                }
            }

            vid_in.time_stamp = (inHeader->nTimeStamp + 500) / 1000;  // in ms;
            vid_in.channel_quality = 1;

            vid_in.needIVOP = false;    // default P frame
            if (mKeyFrameRequested || (mNumInputFrames == 0)) {
                vid_in.needIVOP = true;    // I frame
                ALOGI("Request an IDR frame");
            }

            vid_in.p_src_y = py;
            vid_in.p_src_v = 0;
            vid_in.p_src_y_phy = py_phy;
            vid_in.p_src_v_phy = 0;
            if(width != 0 && height != 0) {
                vid_in.p_src_u = py + width*height;
                vid_in.p_src_u_phy = py_phy + width*height;
            } else {
                vid_in.p_src_u = py + mVideoWidth*mVideoHeight;
                vid_in.p_src_u_phy = py_phy + mVideoWidth*mVideoHeight;
            }
            vid_in.org_img_width = (int32_t)width;
            vid_in.org_img_height = (int32_t)height;
            vid_in.crop_x = (int32_t)x;
            vid_in.crop_y = (int32_t)y;
#ifdef SPRD_DUMP_YUV
            if (mFile_yuv != NULL) {
                fwrite(py, 1, ((mVideoWidth+15)&(~15)) * ((mVideoHeight+15)&(~15))*3/2, mFile_yuv);
            }
#endif
            //dump_yuv(py, width*height*3/2);
            int64_t start_encode = systemTime();
            int ret = (*mMP4EncStrmEncode)(mHandle, &vid_in, &vid_out);
            int64_t end_encode = systemTime();
            ALOGI("MP4EncStrmEncode[%lld] %dms, in {%p-%p, %dx%d}, out {%p-%d, %d}, wh{%d, %d}, xy{%d, %d}",
                  mNumInputFrames, (unsigned int)((end_encode-start_encode) / 1000000L), py, py_phy,
                  mVideoWidth, mVideoHeight, vid_out.pOutBuf, vid_out.strmSize, vid_out.vopType, width, height, x, y);

            if (needUnmap) {
                ALOGV("Free_iova, fd: %d, iova: 0x%lx, size: %zd", bufFd, iova, iovaLen);
                MemoryHeapIon::Free_iova(mIOMMUID, bufFd, iova, iovaLen);
            }

            if ((vid_out.strmSize < 0) || (ret != MMENC_OK)) {
                ALOGE("Failed to encode frame %lld, ret=%d", mNumInputFrames, ret);
                mSignalledError = true;
                notify(OMX_EventError, OMX_ErrorUndefined, 0, 0);
            }

#ifdef SPRD_DUMP_BS
            if (mFile_bs != NULL) {
                fwrite(vid_out.pOutBuf, 1, vid_out.strmSize, mFile_bs);
            }
#endif

            if(vid_out.strmSize > 0) {
                dataLength = vid_out.strmSize;
                memcpy(outPtr, vid_out.pOutBuf, dataLength);

                if (vid_out.vopType == 0) { //I VOP
                    mKeyFrameRequested = false;
                    outHeader->nFlags |= OMX_BUFFERFLAG_SYNCFRAME;
                }
                ++mNumInputFrames;
            } else {
                dataLength = 0;
            }
        } else {
            dataLength = 0;
        }

        if ((inHeader->nFlags & OMX_BUFFERFLAG_EOS) && (inHeader->nFilledLen == 0)) {
            // We also tag this output buffer with EOS if it corresponds
            // to the final input buffer.
            outHeader->nFlags = OMX_BUFFERFLAG_EOS;
        }

        inQueue.erase(inQueue.begin());
        inInfo->mOwnedByUs = false;
        notifyEmptyBufferDone(inHeader);

        outQueue.erase(outQueue.begin());
        CHECK(!mInputBufferInfoVec.empty());
        InputBufferInfo *inputBufInfo = mInputBufferInfoVec.begin();
        outHeader->nTimeStamp = inputBufInfo->mTimeUs;
        outHeader->nFlags |= (inputBufInfo->mFlags | OMX_BUFFERFLAG_ENDOFFRAME);
        outHeader->nFilledLen = dataLength;
        mInputBufferInfoVec.erase(mInputBufferInfoVec.begin());
        outInfo->mOwnedByUs = false;
        notifyFillBufferDone(outHeader);
    }
}

bool SPRDMPEG4Encoder::openEncoder(const char* libName)
{
    if(mLibHandle) {
        dlclose(mLibHandle);
    }

    ALOGI("openEncoder, lib: %s",libName);

    mLibHandle = dlopen(libName, RTLD_NOW);
    if(mLibHandle == NULL) {
        ALOGE("openEncoder, can't open lib: %s",libName);
        return false;
    }

    mMP4EncPreInit = (FT_MP4EncPreInit)dlsym(mLibHandle, "MP4EncPreInit");
    if(mMP4EncPreInit == NULL) {
        ALOGE("Can't find MP4EncPreInit in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mMP4EncInit = (FT_MP4EncInit)dlsym(mLibHandle, "MP4EncInit");
    if(mMP4EncInit == NULL) {
        ALOGE("Can't find MP4EncInit in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mMP4EncSetConf = (FT_MP4EncSetConf)dlsym(mLibHandle, "MP4EncSetConf");
    if(mMP4EncSetConf == NULL) {
        ALOGE("Can't find MP4EncSetConf in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mMP4EncGetConf = (FT_MP4EncGetConf)dlsym(mLibHandle, "MP4EncGetConf");
    if(mMP4EncGetConf == NULL) {
        ALOGE("Can't find MP4EncGetConf in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mMP4EncStrmEncode = (FT_MP4EncStrmEncode)dlsym(mLibHandle, "MP4EncStrmEncode");
    if(mMP4EncStrmEncode == NULL) {
        ALOGE("Can't find MP4EncStrmEncode in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mMP4EncGenHeader = (FT_MP4EncGenHeader)dlsym(mLibHandle, "MP4EncGenHeader");
    if(mMP4EncGenHeader == NULL) {
        ALOGE("Can't find MP4EncGenHeader in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mMP4EncRelease = (FT_MP4EncRelease)dlsym(mLibHandle, "MP4EncRelease");
    if(mMP4EncRelease == NULL) {
        ALOGE("Can't find MP4EncRelease in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mMP4EncGetCodecCapability = (FT_MP4EncGetCodecCapability)dlsym(mLibHandle, "MP4EncGetCodecCapability");
    if(mMP4EncGetCodecCapability == NULL) {
        ALOGE("Can't find MP4EncGetCodecCapability in %s",libName);
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
    return new android::SPRDMPEG4Encoder(name, callbacks, appData, component);
}
