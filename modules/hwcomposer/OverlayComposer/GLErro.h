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


#ifndef _GLERRO_H_
#define _GLERRO_H_

#include <GLES/gl.h>

static __attribute__((noinline))
void checkEGLErrors(const char* token)
{
    struct EGLUtils {
        static const char *strerror(EGLint err) {
            switch (err){
                case EGL_SUCCESS:           return "EGL_SUCCESS";
                case EGL_NOT_INITIALIZED:   return "EGL_NOT_INITIALIZED";
                case EGL_BAD_ACCESS:        return "EGL_BAD_ACCESS";
                case EGL_BAD_ALLOC:         return "EGL_BAD_ALLOC";
                case EGL_BAD_ATTRIBUTE:     return "EGL_BAD_ATTRIBUTE";
                case EGL_BAD_CONFIG:        return "EGL_BAD_CONFIG";
                case EGL_BAD_CONTEXT:       return "EGL_BAD_CONTEXT";
                case EGL_BAD_CURRENT_SURFACE: return "EGL_BAD_CURRENT_SURFACE";
                case EGL_BAD_DISPLAY:       return "EGL_BAD_DISPLAY";
                case EGL_BAD_MATCH:         return "EGL_BAD_MATCH";
                case EGL_BAD_NATIVE_PIXMAP: return "EGL_BAD_NATIVE_PIXMAP";
                case EGL_BAD_NATIVE_WINDOW: return "EGL_BAD_NATIVE_WINDOW";
                case EGL_BAD_PARAMETER:     return "EGL_BAD_PARAMETER";
                case EGL_BAD_SURFACE:       return "EGL_BAD_SURFACE";
                case EGL_CONTEXT_LOST:      return "EGL_CONTEXT_LOST";
                default: return "UNKNOWN";
            }
        }
    };

    EGLint error = eglGetError();
    if (error && error != EGL_SUCCESS) {
        ALOGE("%s: EGL error 0x%04x (%s)",
                token, int(error), EGLUtils::strerror(error));
    }
}

static __attribute__((noinline))
void checkGLErrors()
{
    do {
        // there could be more than one error flag
        GLenum error = glGetError();
        if (error == GL_NO_ERROR)
            break;
        ALOGE("GL error 0x%04x", int(error));
    } while(true);
}


#endif
