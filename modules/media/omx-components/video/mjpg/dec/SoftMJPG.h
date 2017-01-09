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

#ifndef SOFT_MJPG_H_

#define SOFT_MJPG_H_

#include "SprdSimpleOMXComponent.h"

#include "jpeglib.h"

#include <setjmp.h>


/* function pointer to call jpeg decoder lib in libjpeg.so */
typedef struct jpeg_error_mgr* jpeg_error_mgr_ptr;
typedef jpeg_error_mgr_ptr (*jpeg_std_error_ptr)(struct jpeg_error_mgr * err);
typedef void (*jpeg_create_decompress_ptr)(j_decompress_ptr cinfo, int version, size_t structsize);
typedef void (*jpeg_mem_src_ptr)(j_decompress_ptr cinfo, unsigned char * inbuffer, unsigned long insize);
typedef int (*jpeg_read_header_ptr)(j_decompress_ptr cinfo, boolean require_image);
typedef boolean (*jpeg_start_decompress_ptr)(j_decompress_ptr cinfo);
typedef JDIMENSION (*jpeg_read_scanlines_ptr)(j_decompress_ptr cinfo, JSAMPARRAY scanlines, JDIMENSION max_lines);
typedef boolean (*jpeg_finish_decompress_ptr)(j_decompress_ptr cinfo);
typedef void (*jpeg_destroy_decompress_ptr)(j_decompress_ptr cinfo);



namespace android {

struct SoftMJPG : public SprdSimpleOMXComponent {
    SoftMJPG(const char *name,
             const OMX_CALLBACKTYPE *callbacks,
             OMX_PTR appData,
             OMX_COMPONENTTYPE **component);

protected:
    virtual ~SoftMJPG();

    virtual OMX_ERRORTYPE internalGetParameter(
        OMX_INDEXTYPE index, OMX_PTR params);

    virtual OMX_ERRORTYPE internalSetParameter(
        OMX_INDEXTYPE index, const OMX_PTR params);

    virtual OMX_ERRORTYPE getConfig(OMX_INDEXTYPE index, OMX_PTR params);

    virtual void onQueueFilled(OMX_U32 portIndex);
    virtual void onPortEnableCompleted(OMX_U32 portIndex, bool enabled);

    void decode_jpeg_frame(unsigned char* yuv_buffer);

    static void notify_jpeg_error(j_common_ptr cinfo);
private:
    enum {
        kNumInputBuffers  = 4,
        kNumOutputBuffers = 4,
    };

    size_t mInputBufferCount;

    int32_t mWidth, mHeight;
    int32_t mCropWidth, mCropHeight;

    bool mSignalledError;
    bool mFramesConfigured;

    int32_t mNumSamplesOutput;

    enum {
        NONE,
        AWAITING_DISABLED,
        AWAITING_ENABLED
    } mOutputPortSettingsChange;



    struct jpeg_decompress_struct cinfo;
    struct my_jpeg_error_mgr
    {
        struct jpeg_error_mgr pub;
        jmp_buf setjmp_buffer;
    } jerr;

    void* mLibHandle;
    jpeg_std_error_ptr       mjpeg_std_error;
    jpeg_create_decompress_ptr mjpeg_create_decompress;
    jpeg_mem_src_ptr mjpeg_mem_src;
    jpeg_read_header_ptr mjpeg_read_header;
    jpeg_start_decompress_ptr mjpeg_start_decompress;
    jpeg_read_scanlines_ptr mjpeg_read_scanlines;
    jpeg_finish_decompress_ptr mjpeg_finish_decompress;
    jpeg_destroy_decompress_ptr mjpeg_destroy_decompress;


    void initPorts();
    status_t initDecoder();

    void updatePortDefinitions();
    bool portSettingsChanged();

    bool openDecoder(const char* libName);

    DISALLOW_EVIL_CONSTRUCTORS(SoftMJPG);
};

}  // namespace android

#endif  // SOFT_MJPG_H_


