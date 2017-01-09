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

//#define LOG_NDEBUG 0
#define LOG_TAG "SprdOMXPlugin"
#include <utils/Log.h>

#include "SprdOMXPlugin.h"
#include "SprdOMXComponent.h"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AString.h>

#include <dlfcn.h>

namespace android {

static const struct {
    const char *mName;
    const char *mLibNameSuffix;
    const char *mRole;

} kComponents[] = {

    { "OMX.sprd.h263.decoder", "sprd_mpeg4dec", "video_decoder.h263" },
    { "OMX.sprd.mpeg4.decoder", "sprd_mpeg4dec", "video_decoder.mpeg4" },
    { "OMX.sprd.h264.decoder", "sprd_h264dec", "video_decoder.h264" },
    { "OMX.sprd.vpx.decoder", "sprd_vpxdec", "video_decoder.vpx" },
    { "OMX.sprd.aac.decoder", "sprd_aacdec", "audio_decoder.aac" },
    { "OMX.sprd.mp3.decoder", "sprd_mp3dec", "audio_decoder.mp3" },
    { "OMX.sprd.mp3l1.decoder", "sprd_mp3dec", "audio_decoder.mp1" },
    { "OMX.sprd.mp3l2.decoder", "sprd_mp3dec", "audio_decoder.mp2" },
    { "OMX.sprd.ape.decoder", "sprd_apedec", "audio_decoder.ape" },

    { "OMX.sprd.h263.encoder", "sprd_mpeg4enc", "video_encoder.h263" },
    { "OMX.sprd.mpeg4.encoder", "sprd_mpeg4enc", "video_encoder.mpeg4" },
    { "OMX.sprd.h264.encoder", "sprd_h264enc", "video_encoder.avc" },

    { "OMX.sprd.soft.h264.decoder", "sprd_soft_h264dec", "video_decoder.avc" },
    { "OMX.sprd.soft.h263.decoder", "sprd_soft_mpeg4dec", "video_decoder.h263" },
    { "OMX.sprd.soft.mpeg4.decoder", "sprd_soft_mpeg4dec", "video_decoder.mpeg4" },
};

static const size_t kNumComponents =
    sizeof(kComponents) / sizeof(kComponents[0]);

OMXPluginBase *createOMXPlugin() {
    return new SprdOMXPlugin;
}

SprdOMXPlugin::SprdOMXPlugin() {
}

OMX_ERRORTYPE SprdOMXPlugin::makeComponentInstance(
        const char *name,
        const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData,
        OMX_COMPONENTTYPE **component) {
    ALOGI("makeComponentInstance '%s'", name);

    for (size_t i = 0; i < kNumComponents; ++i) {
        if (strcmp(name, kComponents[i].mName)) {
            continue;
        }

        AString libName = "libstagefright_";
        libName.append(kComponents[i].mLibNameSuffix);
        libName.append(".so");

        void *libHandle = dlopen(libName.c_str(), RTLD_NOW);

        if (libHandle == NULL) {
            ALOGE("unable to dlopen %s, err:%s", libName.c_str(), dlerror());

            return OMX_ErrorComponentNotFound;
        }

        typedef SprdOMXComponent *(*CreateSprdOMXComponentFunc)(
                const char *, const OMX_CALLBACKTYPE *,
                OMX_PTR, OMX_COMPONENTTYPE **);

        CreateSprdOMXComponentFunc createSprdOMXComponent =
            (CreateSprdOMXComponentFunc)dlsym(
                    libHandle,
                    "_Z22createSprdOMXComponentPKcPK16OMX_CALLBACKTYPE"
                    "PvPP17OMX_COMPONENTTYPE");

        if (createSprdOMXComponent == NULL) {
            dlclose(libHandle);
            libHandle = NULL;

            return OMX_ErrorComponentNotFound;
        }

        sp<SprdOMXComponent> codec =
            (*createSprdOMXComponent)(name, callbacks, appData, component);

        if (codec == NULL) {
            dlclose(libHandle);
            libHandle = NULL;

            return OMX_ErrorInsufficientResources;
        }

        OMX_ERRORTYPE err = codec->initCheck();
        if (err != OMX_ErrorNone) {
            dlclose(libHandle);
            libHandle = NULL;

            return err;
        }

        codec->incStrong(this);
        codec->setLibHandle(libHandle);

        return OMX_ErrorNone;
    }

    return OMX_ErrorInvalidComponentName;
}

OMX_ERRORTYPE SprdOMXPlugin::destroyComponentInstance(
        OMX_COMPONENTTYPE *component) {
    SprdOMXComponent *me =
        (SprdOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;

    me->prepareForDestruction();

    void *libHandle = me->libHandle();

    CHECK_EQ(me->getStrongCount(), 1);
    me->decStrong(this);
    me = NULL;

    dlclose(libHandle);
    libHandle = NULL;

    return OMX_ErrorNone;
}

OMX_ERRORTYPE SprdOMXPlugin::enumerateComponents(
        OMX_STRING name,
        size_t size,
        OMX_U32 index) {
    if (index >= kNumComponents) {
        return OMX_ErrorNoMore;
    }

    strcpy(name, kComponents[index].mName);

    return OMX_ErrorNone;
}

OMX_ERRORTYPE SprdOMXPlugin::getRolesOfComponent(
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
