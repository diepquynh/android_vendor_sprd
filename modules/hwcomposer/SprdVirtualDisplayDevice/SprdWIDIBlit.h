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
 ** 16/06/2014    Hardware Composer   Responsible for processing some         *
 **                                   Hardware layers. These layers comply    *
 **                                   with Virtual Display specification,     *
 **                                   can be displayed directly, bypass       *
 **                                   SurfaceFligner composition. It will     *
 **                                   improve system performance.             *
 ******************************************************************************
 ** File:SprdWIDIBlit.h               DESCRIPTION                             *
 **                                   WIDIBLIT: Wireless Display Blit         *
 **                                   Responsible for blit image data to      *
 **                                   Virtual Display.                        *
 ******************************************************************************
 ******************************************************************************
 ** Author:         zhongjun.chen@spreadtrum.com                              *
 *****************************************************************************/


#ifndef _SPRD_WIDI_BLIT_H_
#define _SPRD_WIDI_BLIT_H_

#include <semaphore.h>
#include <utils/RefBase.h>
#include <hardware/hardware.h>
#include <hardware/gralloc.h>
#include <hardware/hwcomposer.h>
#include <cutils/log.h>
#include "SprdVirtualPlane.h"
#include "../SprdUtil.h"
#include <arm_neon.h>
#include "../dump.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <ui/GraphicBuffer.h>

namespace android
{

class SprdWIDIBlit: public Thread
{
public:
    SprdWIDIBlit(SprdVirtualPlane *plane);
    virtual ~SprdWIDIBlit();

    /*
     *  Start Blit command 
     * */
    void onStart();

    void onDisplay();

    /*
     *  Query Blit status
     * */
    int queryBlit();


private:
    SprdVirtualPlane *mDisplayPlane;
    SprdUtil         *mAccelerator;
    FrameBufferInfo  *mFBInfo;
    int              mDebugFlag;
    sem_t            startSem;
    sem_t            doneSem;

    virtual status_t readyToRun();
    virtual void onFirstRef();
    virtual bool threadLoop();

    /*
     *  Blit with NEON from RGBA8888 to YUV420SP.
     * */
    int NEONBlit(uint8_t *inrgb, uint8_t *outy, uint8_t *outuv, int32_t width_org, int32_t height_org, int32_t width_dst, int32_t height_dst);


    /*
     *  The following interfaces are implemented by GPU.
     *  Use GPU to do the blit and format conversion.
     * */
    void printGLString(const char *name, GLenum s);
    void printEGLConfiguration(EGLDisplay dpy, EGLConfig config);
    void checkEglError(const char* op, EGLBoolean returnVal = EGL_TRUE);
    void checkGlError(const char* op);

    GLuint loadShader(GLenum shaderType, const char* pSource);
    GLuint createProgram(const char* pVertexSource, const char* pFragmentSource);

    sp<GraphicBuffer> wrapGraphicsBuffer(private_handle_t *handle);
    int setupGraphics();
    void destoryGraphics();
    int setupYuvTexSurface(hwc_layer_1_t *AndroidLayer, private_handle_t *TragetHandle, sp<GraphicBuffer>& Source, sp<GraphicBuffer>& Target);
    int renderImage(sp<GraphicBuffer> Source, sp<GraphicBuffer> Target);
};

}

#endif
