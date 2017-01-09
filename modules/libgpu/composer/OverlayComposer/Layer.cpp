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


#include "Layer.h"
#include "GLErro.h"
#include "OverlayComposer.h"
#include <hardware/hwcomposer.h>


namespace android
{


//#define _DEBUG
#ifdef _DEBUG
#define GL_CHECK(x) \
    x; \
    { \
        GLenum err = glGetError(); \
        if(err != GL_NO_ERROR) { \
            ALOGE("glGetError() = %i (0x%.8x) at line %i\n", err, err, __LINE__); \
        } \
    }
#else
#define GL_CHECK(x) x
#endif

static GLfloat texcoords[] = {
    0.0f, 0.0f,
    1.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 1.0f
};

static float mtxFlipH[16] = {
    -1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    1, 0, 0, 1,
};

static float mtxFlipV[16] = {
    1, 0, 0, 0,
    0, -1, 0, 0,
    0, 0, 1, 0,
    0, 1, 0, 1,
};

static float mtxRot90[16] = {
    0, 1, 0, 0,
    -1, 0, 0, 0,
    0, 0, 1, 0,
    1, 0, 0, 1,
};
static float mtxRot180[16] = {
    -1, 0, 0, 0,
    0, -1, 0, 0,
    0, 0, 1, 0,
    1, 1, 0, 1,
};
static float mtxRot270[16] = {
    0, -1, 0, 0,
    1, 0, 0, 0,
    0, 0, 1, 0,
    0, 1, 0, 1,
};

static float mtxIdentity[16] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1,
};


struct TexCoords {
    GLfloat u;
    GLfloat v;
};

GLfloat mVertices[4][2];
struct TexCoords texCoord[4];
struct TexCoords vertices[4];

Layer::Layer(OverlayComposer* composer, struct private_handle_t *h)
    : mComposer(composer), mPrivH(h),
      mImage(EGL_NO_IMAGE_KHR),
      mTexTarget(GL_TEXTURE_EXTERNAL_OES),
      mTexName(-1U), mTransform(0),
      mAlpha(0.0),
      mSkipFlag(false)
{
    bool ret = init();
    if (!ret)
    {
        ALOGE("Layer Init failed");
        return;
    }

}

bool Layer::init()
{
    if (!wrapGraphicBuffer())
    {
        ALOGE("wrap GraphicBuffer failed");
        return false;
    }

    if (!createTextureImage())
    {
        ALOGE("createEGLImage failed");
        return false;
    }

    /* Initialize Premultiplied Alpha */
    mPremultipliedAlpha = true;

    mNumVertices = 4;

    mFilteringEnabled = true;

    return true;
}

Layer::~Layer()
{
    unWrapGraphicBuffer();
    destroyTextureImage();
}

bool Layer::wrapGraphicBuffer()
{
    uint32_t size;
    uint32_t stride;

    getSizeStride(mPrivH->width, mPrivH->height, mPrivH->format, size, stride);

    mGFXBuffer = new GraphicBuffer(mPrivH->width, mPrivH->height,
                                   mPrivH->format, GraphicBuffer::USAGE_HW_TEXTURE,
                                   stride,
                                   (native_handle_t*)mPrivH, false);
    if (mGFXBuffer->initCheck() != NO_ERROR)
    {
        ALOGE("buf_src create fail");
        return false;
    }

    return true;
}

void Layer::unWrapGraphicBuffer()
{
    return;
}

bool Layer::createTextureImage()
{
    GLint error;
    static EGLint attribs[] = {
        EGL_IMAGE_PRESERVED_KHR,    EGL_TRUE,
        EGL_NONE
    };

    EGLDisplay mDisplay = eglGetCurrentDisplay();

    mImage = eglCreateImageKHR(mDisplay, EGL_NO_CONTEXT,
                               EGL_NATIVE_BUFFER_ANDROID,
                               (EGLClientBuffer)mGFXBuffer->getNativeBuffer(),
                               attribs);

    checkEGLErrors("eglCreateImageKHR");

    if (mImage == EGL_NO_IMAGE_KHR)
    {
        ALOGE("Create EGL Image failed, error = %x", eglGetError());
        return false;
    }

    glGenTextures(1, &mTexName);
    checkGLErrors();
    glBindTexture(mTexTarget, mTexName);
    checkGLErrors();
    glEGLImageTargetTexture2DOES(mTexTarget, (GLeglImageOES)mImage);
    checkGLErrors();
    while ((error = glGetError()) != GL_NO_ERROR)
    {
        ALOGE("createTextureImage error binding external texture image %p, (slot %p): %#04x",
              mImage, mGFXBuffer.get(), error);
        return false;
    }

    return true;
}

void Layer::destroyTextureImage()
{
    EGLDisplay mDisplay = eglGetCurrentDisplay();

    eglDestroyImageKHR(mDisplay, mImage);
    checkEGLErrors("eglDestroyImageKHR");
    glDeleteTextures(1, &mTexName);
}


void Layer::setLayerAlpha(float alpha)
{
    mAlpha = alpha;
}

void Layer::setBlendFlag(int32_t blendFlag)
{
    if (blendFlag == HWC_BLENDING_PREMULT)
    {
        mPremultipliedAlpha = true;
    }
    else
    {
        mPremultipliedAlpha = false;
    }
}

bool Layer::setLayerTransform(uint32_t transform)
{
    mTransform = transform;

    return true;
}

bool Layer::setLayerRect(struct LayerRect *rect, struct LayerRect *rV)
{
    if (rect == NULL && rV == NULL)
    {
        ALOGE("The rectangle is NULL");
        return false;
    }

    mRect = rect;
    mRV = rV;

    return true;
}


void Layer::mtxMul(float out[16], const float a[16], const float b[16])
{
    out[0] = a[0]*b[0] + a[4]*b[1] + a[8]*b[2] + a[12]*b[3];
    out[1] = a[1]*b[0] + a[5]*b[1] + a[9]*b[2] + a[13]*b[3];
    out[2] = a[2]*b[0] + a[6]*b[1] + a[10]*b[2] + a[14]*b[3];
    out[3] = a[3]*b[0] + a[7]*b[1] + a[11]*b[2] + a[15]*b[3];

    out[4] = a[0]*b[4] + a[4]*b[5] + a[8]*b[6] + a[12]*b[7];
    out[5] = a[1]*b[4] + a[5]*b[5] + a[9]*b[6] + a[13]*b[7];
    out[6] = a[2]*b[4] + a[6]*b[5] + a[10]*b[6] + a[14]*b[7];
    out[7] = a[3]*b[4] + a[7]*b[5] + a[11]*b[6] + a[15]*b[7];

    out[8] = a[0]*b[8] + a[4]*b[9] + a[8]*b[10] + a[12]*b[11];
    out[9] = a[1]*b[8] + a[5]*b[9] + a[9]*b[10] + a[13]*b[11];
    out[10] = a[2]*b[8] + a[6]*b[9] + a[10]*b[10] + a[14]*b[11];
    out[11] = a[3]*b[8] + a[7]*b[9] + a[11]*b[10] + a[15]*b[11];

    out[12] = a[0]*b[12] + a[4]*b[13] + a[8]*b[14] + a[12]*b[15];
    out[13] = a[1]*b[12] + a[5]*b[13] + a[9]*b[14] + a[13]*b[15];
    out[14] = a[2]*b[12] + a[6]*b[13] + a[10]*b[14] + a[14]*b[15];
    out[15] = a[3]*b[12] + a[7]*b[13] + a[11]*b[14] + a[15]*b[15];
}

void Layer::computeTransformMatrix()
{
    float xform[16];
    bool mFilteringEnabled = true;
    float tx = 0.0f, ty = 0.0f, sx = 1.0f, sy = 1.0f;

    sp<GraphicBuffer>& buf(mGFXBuffer);
    float bufferWidth = buf->getWidth();
    float bufferHeight = buf->getHeight();

    memcpy(xform, mtxIdentity, sizeof(xform));
    if (mTransform & NATIVE_WINDOW_TRANSFORM_FLIP_H) {
        float result[16];
        mtxMul(result, xform, mtxFlipH);
        memcpy(xform, result, sizeof(xform));
    }
    if (mTransform & NATIVE_WINDOW_TRANSFORM_FLIP_V) {
        float result[16];
        mtxMul(result, xform, mtxFlipV);
        memcpy(xform, result, sizeof(xform));
    }
    if (mTransform & NATIVE_WINDOW_TRANSFORM_ROT_90) {
        float result[16];
        mtxMul(result, xform, mtxRot90);
        memcpy(xform, result, sizeof(xform));
    }

    if (mRect) {
        float shrinkAmount = 0.0f;
        if (mFilteringEnabled) {
            /* In order to prevent bilinear sampling beyond the edge of the
             * crop rectangle we may need to shrink it by 2 texels in each
             * dimension.  Normally this would just need to take 1/2 a texel
             * off each end, but because the chroma channels of YUV420 images
             * are subsampled we may need to shrink the crop region by a whole
             * texel on each side.
             * */
            switch (buf->getPixelFormat()) {
                case PIXEL_FORMAT_RGBA_8888:
                case PIXEL_FORMAT_RGBX_8888:
                case PIXEL_FORMAT_RGB_888:
                case PIXEL_FORMAT_RGB_565:
                case PIXEL_FORMAT_BGRA_8888:
                case PIXEL_FORMAT_RGBA_5551:
                case PIXEL_FORMAT_RGBA_4444:
                    // We know there's no subsampling of any channels, so we
                    // only need to shrink by a half a pixel.
                    shrinkAmount = 0.5;
                    break;

                default:
                    // If we don't recognize the format, we must assume the
                    // worst case (that we care about), which is YUV420.
                    shrinkAmount = 0.0;
                    break;
            }
        }

        // Only shrink the dimensions that are not the size of the buffer.
        int width = mRect->right - mRect->left;
        int height = mRect->bottom - mRect->top;
        if ( width < bufferWidth) {
            tx = ((float)(mRect->left) + shrinkAmount) / bufferWidth;
            sx = ((float)(width) - (2.0f * shrinkAmount)) / bufferWidth;
        }
        if (height < bufferHeight) {
            ty = ((float)(bufferHeight - mRect->bottom) + shrinkAmount) /
                    bufferHeight;
            sy = ((float)(height) - (2.0f * shrinkAmount)) /
                    bufferHeight;
        }
    }

    float crop[16] = {
        sx, 0, 0, 0,
        0, sy, 0, 0,
        0, 0, 1, 0,
        tx, ty, 0, 1,
    };

    float mtxBeforeFlipV[16];
    mtxMul(mtxBeforeFlipV, crop, xform);

    // We expects the top of its window textures to be at a Y
    // coordinate of 0, so SurfaceTexture must behave the same way.  We don't
    // want to expose this to applications, however, so we must add an
    // additional vertical flip to the transform after all the other transforms.
    mtxMul(mCurrentTransformMatrix, mtxFlipV, mtxBeforeFlipV);
}

bool Layer::prepareDrawData()
{
    sp<GraphicBuffer>& buf(mGFXBuffer);

    GLfloat left = GLfloat(mRect->left) / GLfloat(mPrivH->width);
    GLfloat top = GLfloat(mRect->top) / GLfloat(mPrivH->height);
    GLfloat right = GLfloat(mRect->right) / GLfloat(mPrivH->width);
    GLfloat bottom = GLfloat(mRect->bottom) / GLfloat(mPrivH->height);

    texCoord[0].u = texCoord[1].u = left;
    texCoord[0].v = texCoord[3].v = top;
    texCoord[1].v = texCoord[2].v = bottom;
    texCoord[2].u = texCoord[3].u = right;

    /*
     *  Caculate the vertex coordinate
     * */
    /*Overlay play video is consistent  wtih GSP play video.
     * Please refer to the SprdUtil.cpp
     */
    left   = (GLfloat)(mRV->left);
    top = (GLfloat)(mRV->top);
    right  = (GLfloat)(mRV->right);
    bottom = (GLfloat)(mRV->bottom);

    if (mTransform & NATIVE_WINDOW_TRANSFORM_FLIP_H) {
        GLfloat temp = left;
        left = right;
        right = temp;
    }

    if (mTransform & NATIVE_WINDOW_TRANSFORM_FLIP_V) {
        GLfloat temp = top;
        top = bottom;
        bottom = temp;
    }

    vertices[0].u = vertices[1].u = left;
    vertices[0].v = vertices[3].v = top;
    vertices[1].v = vertices[2].v = bottom;
    vertices[2].u = vertices[3].u = right;

    /*
     * Rotate 90 degrees clockwise
     * */
    if (mTransform & NATIVE_WINDOW_TRANSFORM_ROT_90) {
        int left_top = 0, left_bottom = 0, right_top = 0, right_bottom = 0;
        struct TexCoords    center,temp;

        center.u = (left + right)/2;
        center.v = (top + bottom)/2;

        for (int i = 0; i < 4; i++) {
            if (vertices[i].u > center.u ) {
                if (vertices[i].v > center.v) {
                    right_bottom = i;
                } else {
                    right_top = i;
                }
            } else {
                if (vertices[i].v > center.v) {
                    left_bottom = i;
                } else {
                    left_top = i;
                }
            }
        }

        temp                   = vertices[left_top];
        vertices[left_top]     = vertices[right_top];
        vertices[right_top]    = vertices[right_bottom];
        vertices[right_bottom] = vertices[left_bottom];
        vertices[left_bottom]  = temp;
    }

    unsigned int fb_height = mComposer->getDisplayPlane()->getHeight();
    for (int i = 0; i < 4; i++) {
        vertices[i].v = (GLfloat)fb_height - vertices[i].v;
    }

   return true;
}


int Layer::draw()
{
    int status = -1;

    /*
    mSkipFlag = false;
    */
    prepareDrawData();
    /*
    if (mSkipFlag)
    {
        ALOGD("Skip this frame");
        mSkipFlag = false;
        return 0;
    }*/

    glTexParameterx(GL_TEXTURE_EXTERNAL_OES,
                    GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterx(GL_TEXTURE_EXTERNAL_OES,
                    GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterx(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterx(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    //computeTransformMatrix();
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    //glLoadMatrixf(mCurrentTransformMatrix);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_EXTERNAL_OES);
#ifdef PRIMARYPLANE_USE_RGB565
    glEnable(GL_DITHER);
#endif


    /*
     *  Start call openGLES Draw list here
     *  By default, we use Premultiplied Alpha
     * */
    GLenum src = mPremultipliedAlpha ? GL_ONE : GL_SRC_ALPHA;
    if (mAlpha < 0xFF)
    {
        const GLfloat alpha = (GLfloat)mAlpha * (1.0f/255.0f);
        if (mPremultipliedAlpha)
        {
            glColor4f(alpha, alpha, alpha, alpha);
        }
        else
        {
            glColor4f(1.0f, 1.0f, 1.0f, alpha);
        }
        glTexEnvx(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glEnable(GL_BLEND);
        glBlendFunc(src, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
    {
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glTexEnvx(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        glEnable(GL_BLEND);
        glBlendFunc(src, GL_ONE_MINUS_SRC_ALPHA);
    }

    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, vertices);
    glTexCoordPointer(2, GL_FLOAT, 0, texCoord);
    GL_CHECK(glDrawArrays(GL_TRIANGLE_FAN, 0, mNumVertices));


    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable(GL_BLEND);


#ifdef PRIMARYPLANE_USE_RGB565
    glDisable(GL_DITHER);
#endif
    glDisable(GL_TEXTURE_EXTERNAL_OES);
    glDisable(GL_TEXTURE_2D);

    status = 0;
    return status;
}

};

