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

#include <utils/Log.h>
#include "SprdDisplayDevice.h"

using namespace android;


static inline int getSPRDFBFormat(ANativeWindow* const window, const sp<IGraphicBufferProducer>& surface, android::DisplayDevice::DisplayType type)

{
    int format = HAL_PIXEL_FORMAT_RGBA_8888;
#ifdef SPRD_ENABLE_FRAMEBUFFER_AFBC
    if (type == HWC_DISPLAY_PRIMARY)
        format = HAL_PIXEL_FORMAT_RGB_888;
#endif
    if (type >= HWC_DISPLAY_VIRTUAL)
        surface->query(NATIVE_WINDOW_FORMAT, &format);
    native_window_set_buffers_format(window, format);
    return format;
}

static inline bool getSPRDWindowSurfaceConfig(EGLDisplay display, android::DisplayDevice::DisplayType type, EGLConfig* config)
{
    if (type >= HWC_DISPLAY_VIRTUAL)
    {
        EGLConfig* nv12Config = config;
        EGLint numConfigs = 0;
	static EGLint sDefaultConfigAttribs[] = {
		    EGL_CONFIG_ID,
	#ifndef GPU_IS_MIDGARD
		    43,
	#else
		    23,
	#endif
		    EGL_NONE };
	    eglChooseConfig(display, sDefaultConfigAttribs, nv12Config, 1, &numConfigs);
        ALOGD("SPRD------------------- HWC_DISPLAY_VIRTUAL \n");
	    return true;
    }
    else
    {
        ALOGD("SPRD------------------- HWC_DISPLAY_PRIMARY \n");
	    return true;
    }
}

