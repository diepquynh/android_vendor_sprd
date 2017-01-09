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


/*****************************************************************************
 ************************  How to use OverlayComposer ?  *********************
 ***** Enable/Disable Macro: USE_OVERLAY_COMPOSER_GPU    *********************
 *****                                                   *********************
 *****************************************************************************
 ***** Using steps:                                      *********************
 *****             1. Initialize the struct overlayDevice_t;******************
 *****                member, including:                    ******************
 *****                    int fbfd;                         ******************
 *****                    unsigned int fb_width;            ******************
 *****                    unsigned int fb_height;           ******************
 *****                                                      ******************
 *****                    uint32_t overlay_phy_addr;        ******************
 *****                    void *overlay_v_addr;             ******************
 *****                    uint32_t overlay_buf_size;        ******************
 *****                                                      ******************
 *****                                                      ******************
 *****             2. Instantiate OverlayComposer class;    ******************
 *****                                                      ******************
 *****             3. call init();                          ******************
 *****                                                      ******************
 *****             4. When you prepare Hardware Layer list, ******************
 *****                just call onComposer();               ******************
 *****                                                      ******************
 *****             5. When you want to display the overlay  ******************
 *****                just call onDisplay();                ******************
 *****                                                      ******************
 *****   The OverlayComposer is Android smart poiner,       ******************
 *****   will be automatically destroyed by HWComposer      ******************
 *****              Everything is done !!                   ******************
 * */

#ifndef _OVERLAY_COMPOSER_H_
#define _OVERLAY_COMPOSER_H_

#include <stdint.h>
#include <errno.h>
#include <fcntl.h>

#include <GLES/gl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <hardware/hardware.h>
#include <hardware/hwcomposer.h>
#include <utils/Thread.h>
#include <semaphore.h>

#include <cutils/log.h>
#include <utils/RefBase.h>
#include <utils/List.h>

#include <ui/ANativeObjectBase.h>

#include "gralloc_priv.h"

#include "Utility.h"
#include "../SprdPrimaryDisplayDevice/SprdFrameBufferHAL.h"

#include "OverlayNativeWindow.h"
#include "Layer.h"


namespace android
{


class OverlayComposer: public Thread
{
public:
    OverlayComposer(SprdPrimaryPlane *displayPlane, sp<OverlayNativeWindow> NativeWindow);
    ~OverlayComposer();

    SprdDisplayPlane* getDisplayPlane() { return mDisplayPlane; }

    /* Start the HWLayer composer command */
    bool onComposer(hwc_display_contents_1_t* l);

    /* Start display the composered Overlay Buffer */
    void onDisplay();

private:

    /* Overlay composer Info */
    SprdPrimaryPlane *mDisplayPlane;

    /* Hardware Layer Info */
    hwc_display_contents_1_t* mList;
    unsigned int     mNumLayer;

   /* Graphics Info */
    int InitFlag;
    sp<OverlayNativeWindow> mWindow;
    EGLDisplay      mDisplay;
    EGLSurface      mSurface;
    EGLContext      mContext;
    EGLConfig       mConfig;

    uint32_t        mFlags;
    GLint           mMaxViewportDims[2];
    GLint           mMaxTextureSize;

    GLuint                      mWormholeTexName;
    GLuint                      mProtectedTexName;


    /* define a List to store Layer object */
    typedef List<Layer * > DrawLayerList;
    DrawLayerList mDrawLayerList;


    static status_t selectConfigForPixelFormat(
                                 EGLDisplay dpy,
                                 EGLint const* attrs,
                                 PixelFormat format,
                                 EGLConfig* outConfig);
    bool initOpenGLES();
    void deInitOpenGLES();
    bool initEGL();
    void deInitEGL();

    void ClearOverlayComposerBuffer();
    void caculateLayerRect(hwc_layer_1_t  *l, struct LayerRect *rect, struct LayerRect *rV);

    bool swapBuffers();

    /*
     *  A new thread is needed to do the composer work.
     *  It will save the context switch back to SurfaceFlinger
     *  context.
     * */
    sem_t         cmdSem;
    sem_t         doneSem;
    sem_t         displaySem;
    int composerHWLayers();
    virtual bool        threadLoop();
    virtual status_t    readyToRun();
    virtual void        onFirstRef();
    /***** work thread  done *****/


    static inline uint16_t pack565(int r, int g, int b)
    {
        return (r<<11)|(g<<5)|b;
    }

    inline int MIN(int x, int y)
    {
        return ((x < y) ? x: y);
    }

    inline int MAX(int x, int y)
    {
        return ((x > y) ? x : y);
    }
};


};

#endif
