//--=========================================================================--
//  This file is a part of VPU OpenMAX API project
//-----------------------------------------------------------------------------
//
//       This confidential and proprietary software may be used only
//     as authorized by a licensing agreement from Chips&Media Inc.
//     In the event of publication, the following notice is applicable:
//
//            (C) COPYRIGHT 2006 - 2011  CHIPS&MEDIA INC.
//                      ALL RIGHTS RESERVED
//
//       The entire notice above must be reproduced on all authorized
//       copies.
//
//--=========================================================================--

//#define LOG_NDEBUG 0
#define LOG_TAG "VPUOMXPlugin"
#include <utils/Log.h>

#include "VPUOMXPlugin.h"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AString.h>

#include <dlfcn.h>

#define OMX_CORE_LIBRARY "libomxil-bellagio.so"

namespace android {

static const struct {
    const char *mName;
    const char *mRole;

} kComponents[] = {
    { "OMX.vpu.video_decoder.avc", "video_decoder.avc" },
    { "OMX.vpu.video_decoder.mpeg2", "video_decoder.mpeg2" },
};

static const size_t kNumComponents =
    sizeof(kComponents) / sizeof(kComponents[0]);

OMXPluginBase *createOMXPlugin() {
    return new VPUOMXPlugin;
}

VPUOMXPlugin::VPUOMXPlugin()
: mLibHandle(dlopen(OMX_CORE_LIBRARY, RTLD_NOW)),
mInit(NULL),
mDeinit(NULL),
mComponentNameEnum(NULL),
mGetHandle(NULL),
mFreeHandle(NULL),
mGetRolesOfComponentHandle(NULL) {

    if (mLibHandle != NULL) {
        mInit = (InitFunc)dlsym(mLibHandle, "OMX_Init");
        mDeinit = (DeinitFunc)dlsym(mLibHandle, "OMX_Deinit");

        mComponentNameEnum =
            (ComponentNameEnumFunc)dlsym(mLibHandle, "OMX_ComponentNameEnum");

        mGetHandle = (GetHandleFunc)dlsym(mLibHandle, "OMX_GetHandle");
        mFreeHandle = (FreeHandleFunc)dlsym(mLibHandle, "OMX_FreeHandle");

        mGetRolesOfComponentHandle =
            (GetRolesOfComponentFunc)dlsym(
			mLibHandle, "OMX_GetRolesOfComponent");

        //(*mInit)();
    }
	else {
		ALOGE("not found libomx-bellagio.so\n");
	}
}

VPUOMXPlugin::~VPUOMXPlugin() {
    if (mLibHandle != NULL) {
        (*mDeinit)();

        dlclose(mLibHandle);
        mLibHandle = NULL;
    }
}

OMX_ERRORTYPE VPUOMXPlugin::makeComponentInstance(
        const char *name,
        const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData,
        OMX_COMPONENTTYPE **component) {

    ALOGI("makeComponentInstance: %s", name);

    if (mLibHandle == NULL) {
        return OMX_ErrorUndefined;
    }

    if ((*mInit)()) {
        ALOGE("makeComponentInstance, init error");
        return OMX_ErrorInsufficientResources;
    }

    return (*mGetHandle)(
		reinterpret_cast<OMX_HANDLETYPE *>(component),
		const_cast<char *>(name),
		appData, const_cast<OMX_CALLBACKTYPE *>(callbacks));
}

OMX_ERRORTYPE VPUOMXPlugin::destroyComponentInstance(
        OMX_COMPONENTTYPE *component) {

    if (mLibHandle == NULL) {
        return OMX_ErrorUndefined;
    }

    return (*mFreeHandle)(reinterpret_cast<OMX_HANDLETYPE *>(component));
}

OMX_ERRORTYPE VPUOMXPlugin::enumerateComponents(
        OMX_STRING name,
        size_t size,
        OMX_U32 index) {
    if (index >= kNumComponents) {
        return OMX_ErrorNoMore;
    }

    strcpy(name, kComponents[index].mName);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE VPUOMXPlugin::getRolesOfComponent(
        const char *name,
        Vector<String8> *roles) {
    for (size_t i = 0; i < kNumComponents; ++i) {
        if (strcmp(name, kComponents[i].mName)) {
            continue;
        }

        roles->clear();
        roles->push(String8(kComponents[i].mRole));

        return OMX_ErrorNone;
    }

    return OMX_ErrorInvalidComponentName;
}

}  // namespace android
