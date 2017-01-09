/*
 * Copyright (C) 2010 The Android Open Source Project
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


/******************************************************************************
 **                   Edit    History                                         *
 **---------------------------------------------------------------------------*
 ** DATE          Module              DESCRIPTION                             *
 ** 16/08/2013    Hardware Composer   Add a new feature to Harware composer,  *
 **                                   verlayComposer use GPU to do the        *
 **                                   Hardware layer blending on Overlay      *
 **                                   buffer, and then post the OVerlay       *
 **                                   buffer to Display                       *
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/


#ifndef _LAYER_H_
#define _LAYER_H_

#include <stdint.h>
#include <errno.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <ui/GraphicBuffer.h>

#include "gralloc_priv.h"
#include <hardware/gralloc.h>

#include "Utility.h"

#include <cutils/log.h>


namespace android
{

class OverlayComposer;

class Layer
{
public:
    Layer(OverlayComposer* composer, struct private_handle_t *h);
    ~Layer();

    /* Hardware Layer draw function */
    int draw();

    /* Gain some info from external class */
    bool setLayerTransform(uint32_t transform);
    bool setPlaneTransform(uint8_t orientation);
    bool setLayerRect(struct LayerRect *rect, struct LayerRect *rV);
    void setLayerAlpha(float alpha);
    void setBlendFlag(int32_t blendFlag);

private:
    OverlayComposer* mComposer;
    struct private_handle_t *mPrivH;
    EGLImageKHR mImage;
    GLenum mTexTarget;
    GLuint mTexName;
    uint32_t mTransform;
    float    mAlpha;
    bool mSkipFlag;
    bool mFilteringEnabled;


    struct LayerRect *mRect;
    struct LayerRect *mRV;
    bool mPremultipliedAlpha;
    int mNumVertices;

    float mCurrentTransformMatrix[16];

    sp<GraphicBuffer> mGFXBuffer;

    bool init();
    bool wrapGraphicBuffer();
    void unWrapGraphicBuffer();
    bool createTextureImage();
    void destroyTextureImage();

    bool prepareDrawData();


    void computeTransformMatrix();
    inline void mtxMul(float out[16], const float a[16], const float b[16]);
};


};


#endif
