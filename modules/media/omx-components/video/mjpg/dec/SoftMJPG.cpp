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

#define LOG_NDEBUG 0
#define LOG_TAG "SoftMJPG"
#include <utils/Log.h>

#include "SoftMJPG.h"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/MediaErrors.h>
#include <media/IOMX.h>

#include <dlfcn.h>

namespace android {


template<class T>
static void InitOMXParams(T *params) {
    params->nSize = sizeof(T);
    params->nVersion.s.nVersionMajor = 1;
    params->nVersion.s.nVersionMinor = 0;
    params->nVersion.s.nRevision = 0;
    params->nVersion.s.nStep = 0;
}

SoftMJPG::SoftMJPG(
    const char *name,
    const OMX_CALLBACKTYPE *callbacks,
    OMX_PTR appData,
    OMX_COMPONENTTYPE **component)
    : SprdSimpleOMXComponent(name, callbacks, appData, component),
      mInputBufferCount(0),
      mWidth(174),
      mHeight(144),
      mCropWidth(mWidth),
      mCropHeight(mHeight),
      mSignalledError(false),
      mFramesConfigured(false),
      mNumSamplesOutput(0),
      mOutputPortSettingsChange(NONE),
      mLibHandle(NULL),
      mjpeg_std_error(NULL),
      mjpeg_create_decompress(NULL),
      mjpeg_mem_src(NULL),
      mjpeg_read_header(NULL),
      mjpeg_start_decompress(NULL),
      mjpeg_read_scanlines(NULL),
      mjpeg_finish_decompress(NULL),
      mjpeg_destroy_decompress(NULL) {
    CHECK(!strcmp(name, "OMX.google.mjpg.decoder"));

    CHECK_EQ(openDecoder("libjpeg.so"), true);

    initPorts();
    CHECK_EQ(initDecoder(), (status_t)OK);
}

SoftMJPG::~SoftMJPG() {
    if(mLibHandle)
    {
        dlclose(mLibHandle);
        mLibHandle = NULL;
    }
}

void SoftMJPG::initPorts() {
    OMX_PARAM_PORTDEFINITIONTYPE def;
    InitOMXParams(&def);

    def.nPortIndex = 0;
    def.eDir = OMX_DirInput;
    def.nBufferCountMin = kNumInputBuffers;
    def.nBufferCountActual = def.nBufferCountMin;
    def.nBufferSize = 8192;
    def.bEnabled = OMX_TRUE;
    def.bPopulated = OMX_FALSE;
    def.eDomain = OMX_PortDomainVideo;
    def.bBuffersContiguous = OMX_FALSE;
    def.nBufferAlignment = 1;

    def.format.video.cMIMEType = "video/mjpg";

    def.format.video.pNativeRender = NULL;
    def.format.video.nFrameWidth = mWidth;
    def.format.video.nFrameHeight = mHeight;
    def.format.video.nStride = def.format.video.nFrameWidth;
    def.format.video.nSliceHeight = def.format.video.nFrameHeight;
    def.format.video.nBitrate = 0;
    def.format.video.xFramerate = 0;
    def.format.video.bFlagErrorConcealment = OMX_FALSE;

    def.format.video.eCompressionFormat = OMX_VIDEO_CodingMJPEG;

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

status_t SoftMJPG::initDecoder() {
    memset(&cinfo, 0, sizeof(cinfo));
    memset(&jerr, 0, sizeof(jerr));

    return OK;
}

OMX_ERRORTYPE SoftMJPG::internalGetParameter(
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
            formatParams->eCompressionFormat = OMX_VIDEO_CodingMJPEG;

            formatParams->eColorFormat = OMX_COLOR_FormatUnused;
            formatParams->xFramerate = 0;
        } else {
            CHECK_EQ(formatParams->nPortIndex, 1u);

            formatParams->eCompressionFormat = OMX_VIDEO_CodingUnused;
            formatParams->eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
            formatParams->xFramerate = 0;
        }

        return OMX_ErrorNone;
    }

    default:
        return SprdSimpleOMXComponent::internalGetParameter(index, params);
    }
}

OMX_ERRORTYPE SoftMJPG::internalSetParameter(
    OMX_INDEXTYPE index, const OMX_PTR params) {
    switch (index) {
    case OMX_IndexParamStandardComponentRole:
    {
        const OMX_PARAM_COMPONENTROLETYPE *roleParams =
            (const OMX_PARAM_COMPONENTROLETYPE *)params;

        if (strncmp((const char *)roleParams->cRole,
                    "video_decoder.mjpg",
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

    case OMX_IndexParamPortDefinition:
    {
        OMX_PARAM_PORTDEFINITIONTYPE *defParams =
            (OMX_PARAM_PORTDEFINITIONTYPE *)params;

        if (defParams->nPortIndex > 1) {
            return OMX_ErrorBadPortIndex;
        }

        if (defParams->nSize != sizeof(OMX_PARAM_PORTDEFINITIONTYPE)) {
            return OMX_ErrorUnsupportedSetting;
        }

        PortInfo *port = editPortInfo(defParams->nPortIndex);

        // default behavior is that we only allow buffer size to increase
        if (defParams->nBufferSize > port->mDef.nBufferSize) {
            port->mDef.nBufferSize = defParams->nBufferSize;
        }

        if (defParams->nBufferCountActual < port->mDef.nBufferCountMin) {
            ALOGW("component requires at least %u buffers (%u requested)",
                    port->mDef.nBufferCountMin, defParams->nBufferCountActual);
            return OMX_ErrorUnsupportedSetting;
        }

        port->mDef.nBufferCountActual = defParams->nBufferCountActual;

        memcpy(&port->mDef.format.video, &defParams->format.video, sizeof(OMX_VIDEO_PORTDEFINITIONTYPE));

        if (defParams->nPortIndex == kOutputPortIndex) {
            mWidth = mCropWidth = defParams->format.video.nFrameWidth;
            mHeight = mCropHeight = defParams->format.video.nFrameHeight;
            updatePortDefinitions();
        }

        return OMX_ErrorNone;
    }

    default:
        return SprdSimpleOMXComponent::internalSetParameter(index, params);
    }
}

OMX_ERRORTYPE SoftMJPG::getConfig(
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
        rectParams->nWidth = mCropWidth;
        rectParams->nHeight = mCropHeight;

        return OMX_ErrorNone;
    }

    default:
        return OMX_ErrorUnsupportedIndex;
    }
}

void SoftMJPG::onQueueFilled(OMX_U32 portIndex) {
    if (mSignalledError || mOutputPortSettingsChange != NONE) {
        return;
    }

    List<BufferInfo *> &inQueue = getPortQueue(0);
    List<BufferInfo *> &outQueue = getPortQueue(1);

    while (!inQueue.empty() && outQueue.size() == kNumOutputBuffers) {
        BufferInfo *inInfo = *inQueue.begin();
        OMX_BUFFERHEADERTYPE *inHeader = inInfo->mHeader;

        PortInfo *port = editPortInfo(1);

        OMX_BUFFERHEADERTYPE *outHeader =
            port->mBuffers.editItemAt(mNumSamplesOutput & 1).mHeader;

        if ((inHeader->nFlags & OMX_BUFFERFLAG_EOS) || (inHeader->nFilledLen == 0)) {
            inQueue.erase(inQueue.begin());
            inInfo->mOwnedByUs = false;
            notifyEmptyBufferDone(inHeader);

            ++mInputBufferCount;

            outHeader->nFilledLen = 0;
            if (inHeader->nFlags & OMX_BUFFERFLAG_EOS) {
                outHeader->nFlags = OMX_BUFFERFLAG_EOS;
            }

            List<BufferInfo *>::iterator it = outQueue.begin();
            while ((*it)->mHeader != outHeader) {
                ++it;
            }

            BufferInfo *outInfo = *it;
            outInfo->mOwnedByUs = false;
            outQueue.erase(it);
            outInfo = NULL;

            notifyFillBufferDone(outHeader);
            outHeader = NULL;
            return;
        }

        uint8_t *bitstream = inHeader->pBuffer + inHeader->nOffset;

        if (!mFramesConfigured) {
            PortInfo *port = editPortInfo(1);
            OMX_BUFFERHEADERTYPE *outHeader = port->mBuffers.editItemAt(1).mHeader;

            mFramesConfigured = true;
        }

        // decoder deals in ms, OMX in us.
        uint32_t timestamp = ((inHeader->nOffset == 0)) ? (inHeader->nTimeStamp + 500) / 1000 : 0xFFFFFFFF;
        int32_t bufferSize = inHeader->nFilledLen;


        /* allocate and initialize JPEG decompression object */
        cinfo.err = mjpeg_std_error(&jerr.pub);
        jerr.pub.error_exit = notify_jpeg_error;
        if (setjmp(jerr.setjmp_buffer)) {
            mjpeg_destroy_decompress(&cinfo);

            notify(OMX_EventError, OMX_ErrorUndefined, 0, NULL);
            mSignalledError = true;
            return;
        }

        mjpeg_create_decompress(&cinfo, JPEG_LIB_VERSION, sizeof(cinfo));

        if (mjpeg_mem_src == NULL) {
            notify(OMX_EventError, OMX_ErrorUndefined, 0, NULL);
            mSignalledError = true;
            return;
        }
        /* specify data source */
        mjpeg_mem_src(&cinfo, bitstream, bufferSize);

        /* read parameters with jpeg_read_header() */
        mjpeg_read_header(&cinfo, TRUE);

        cinfo.dct_method = JDCT_IFAST;
        cinfo.do_fancy_upsampling = FALSE;
        cinfo.do_block_smoothing = FALSE;
        cinfo.dither_mode = JDITHER_NONE;
        cinfo.two_pass_quantize = FALSE;

        ALOGI("jpeg %dx%d", cinfo.image_width, cinfo.image_height);

        if (((cinfo.image_width+15)&(~15)) != mWidth || ((cinfo.image_height+15)&(~15)) != mHeight) {
            mWidth = (cinfo.image_width+15)&(~15);
            mHeight = (cinfo.image_height+15)&(~15);

            mCropWidth = (int32_t)cinfo.image_width;
			mCropHeight = (int32_t)cinfo.image_height;

            updatePortDefinitions();

            mFramesConfigured = false;

            notify(OMX_EventPortSettingsChanged, 1, 0, NULL);
            mOutputPortSettingsChange = AWAITING_DISABLED;

            mjpeg_destroy_decompress(&cinfo);

            return;
        }

        decode_jpeg_frame(outHeader->pBuffer);

        // decoder deals in ms, OMX in us.
        outHeader->nTimeStamp = timestamp * 1000;

        inHeader->nOffset += bufferSize;
        inHeader->nFilledLen = 0;

        if (inHeader->nFilledLen == 0) {
            inInfo->mOwnedByUs = false;
            inQueue.erase(inQueue.begin());
            inInfo = NULL;
            notifyEmptyBufferDone(inHeader);
            inHeader = NULL;
        }

        ++mInputBufferCount;

        outHeader->nOffset = 0;
        outHeader->nFilledLen = (mWidth * mHeight * 3) / 2;
        outHeader->nFlags = 0;

        List<BufferInfo *>::iterator it = outQueue.begin();
        while ((*it)->mHeader != outHeader) {
            ++it;
        }

        BufferInfo *outInfo = *it;
        outInfo->mOwnedByUs = false;
        outQueue.erase(it);
        outInfo = NULL;

        notifyFillBufferDone(outHeader);
        outHeader = NULL;

        ++mNumSamplesOutput;
    }
}


void SoftMJPG::onPortEnableCompleted(OMX_U32 portIndex, bool enabled) {
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

void SoftMJPG::updatePortDefinitions() {
    OMX_PARAM_PORTDEFINITIONTYPE *def = &editPortInfo(0)->mDef;
    def->format.video.nFrameWidth = mWidth;
    def->format.video.nFrameHeight = mHeight;
    def->format.video.nStride = def->format.video.nFrameWidth;
    def->format.video.nSliceHeight = def->format.video.nFrameHeight;

    def = &editPortInfo(1)->mDef;
    def->format.video.nFrameWidth = mWidth;
    def->format.video.nFrameHeight = mHeight;
    def->format.video.nStride = def->format.video.nFrameWidth;
    def->format.video.nSliceHeight = def->format.video.nFrameHeight;

    def->nBufferSize =
        (((def->format.video.nFrameWidth + 15) & -16)
         * ((def->format.video.nFrameHeight + 15) & -16) * 3) / 2;
}

void SoftMJPG::notify_jpeg_error(j_common_ptr cinfo)
{
    char buffer[JMSG_LENGTH_MAX];
    (*cinfo->err->format_message) (cinfo, buffer);

    my_jpeg_error_mgr* myerr = (my_jpeg_error_mgr*) cinfo->err;

    ALOGE("jpeg error: %s", buffer);

    longjmp(myerr->setjmp_buffer, 1);
}

void SoftMJPG::decode_jpeg_frame(unsigned char * yuv_buffer)
{
    cinfo.out_color_space = cinfo.jpeg_color_space;

    /* Start decompressor */
    mjpeg_start_decompress(&cinfo);

    unsigned char* py = yuv_buffer;
    unsigned char* puv = yuv_buffer +mWidth *mHeight;
    JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo, JPOOL_IMAGE, mWidth * cinfo.output_components, 1);
    while (cinfo.output_scanline < cinfo.output_height) {
        /* jpeg_read_scanlines expects an array of pointers to scanlines.
         * Here the array is only one element long, but you could ask for
         * more than one scanline at a time if that's more convenient.
         */
        mjpeg_read_scanlines(&cinfo, buffer, 1);

        /* Assume put_scanline_someplace wants a pointer and sample count. */
        unsigned char* pbuf = buffer[0];
        if (cinfo.out_color_space == JCS_YCbCr) {
            if (cinfo.output_scanline & 1) {
                for (int i=mWidth/2; i>0; i--) {
                    *py ++ = * pbuf ++;
                    *puv ++ = * pbuf ++;
                    *puv ++ = * pbuf ++;

                    *py ++ = * pbuf ++;
                    pbuf ++;
                    pbuf ++;
                }
            } else {
                for (int i=mWidth/2; i>0; i--) {
                    *py ++ = * pbuf ++;
                    pbuf ++;
                    pbuf ++;

                    *py ++ = * pbuf ++;
                    pbuf ++;
                    pbuf ++;
                }
            }
        } else { // JCS_GRAYSCALE
            memcpy(py, pbuf, mWidth);
            py += mWidth;
            memset(puv, 0x80, mWidth/2);
            puv += mWidth/2;
        }
    }


    /* Finish decompression */
    mjpeg_finish_decompress(&cinfo);

    /* Release JPEG decompression object */
    mjpeg_destroy_decompress(&cinfo);
}



bool SoftMJPG::openDecoder(const char* libName)
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

    mjpeg_std_error = (jpeg_std_error_ptr)dlsym(mLibHandle, "jpeg_std_error");
    if(mjpeg_std_error == NULL) {
        ALOGE("Can't find jpeg_std_error in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mjpeg_create_decompress = (jpeg_create_decompress_ptr)dlsym(mLibHandle, "jpeg_CreateDecompress");
    if(mjpeg_create_decompress == NULL) {
        ALOGE("Can't find jpeg_CreateDecompress in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mjpeg_mem_src = (jpeg_mem_src_ptr)dlsym(mLibHandle, "jpeg_mem_src");
    if(mjpeg_mem_src == NULL) {
        ALOGE("Can't find jpeg_mem_src in %s, and cant't play mjpg video clips",libName);
    }

    mjpeg_read_header = (jpeg_read_header_ptr)dlsym(mLibHandle, "jpeg_read_header");
    if(mjpeg_read_header == NULL) {
        ALOGE("Can't find jpeg_read_header in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mjpeg_start_decompress = (jpeg_start_decompress_ptr)dlsym(mLibHandle, "jpeg_start_decompress");
    if(mjpeg_start_decompress == NULL) {
        ALOGE("Can't find jpeg_start_decompress in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mjpeg_read_scanlines = (jpeg_read_scanlines_ptr)dlsym(mLibHandle, "jpeg_read_scanlines");
    if(mjpeg_read_scanlines == NULL) {
        ALOGE("Can't find jpeg_read_scanlines in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mjpeg_finish_decompress = (jpeg_finish_decompress_ptr)dlsym(mLibHandle, "jpeg_finish_decompress");
    if(mjpeg_finish_decompress == NULL) {
        ALOGE("Can't find jpeg_finish_decompress in %s",libName);
        dlclose(mLibHandle);
        mLibHandle = NULL;
        return false;
    }

    mjpeg_destroy_decompress = (jpeg_destroy_decompress_ptr)dlsym(mLibHandle, "jpeg_destroy_decompress");
    if(mjpeg_destroy_decompress == NULL) {
        ALOGE("Can't find jpeg_destroy_decompress in %s",libName);
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
    return new android::SoftMJPG(name, callbacks, appData, component);
}

