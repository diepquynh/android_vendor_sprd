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
#define LOG_TAG "android_drm_DrmManagerClientEx"
#include <utils/Log.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <jni.h>
#include <JNIHelp.h>
#include <ScopedLocalRef.h>
#include <android_runtime/AndroidRuntime.h>

#include <drm/DrmInfo.h>
#include <drm/DrmRights.h>
#include <drm/DrmInfoEvent.h>
#include <drm/DrmInfoStatus.h>
#include <drm/DrmInfoRequest.h>
#include <drm/DrmSupportInfo.h>
#include <drm/DrmConstraints.h>
#include <drm/DrmMetadata.h>
#include <drm/DrmConvertedStatus.h>
#include <drm/drm_framework_common.h>

#include <DrmManagerClientImpl.h>

using namespace android;

sp<DrmManagerClientImpl> getDrmManagerClientImpl(JNIEnv* env, jobject thiz);

jbyteArray android_drm_DrmManagerClient_pread (JNIEnv* env, jobject thiz, jint uniqueId, jobject handle, jint size, jint offset) {
    jclass clazz = env->FindClass("android/drm/DecryptHandle");
    jfieldID fieldId = env->GetFieldID(clazz, "mNativeContext", "J");
    DecryptHandleWrapper* wrapper = (DecryptHandleWrapper*)env->GetLongField(handle, fieldId);
    ALOGE("sunway: pread: decryptHandle id: %d", wrapper->handle->decryptId);
    char buffer[size];
    bzero(buffer, size);

    int count = getDrmManagerClientImpl(env, thiz)->pread(uniqueId, wrapper->handle, buffer, size, offset);
    if (count <=0) {
        return NULL;
    }
    jbyteArray ret = env->NewByteArray(count);
    if (ret == NULL) {
        return NULL;
    }
    env->SetByteArrayRegion(ret, 0, count, (jbyte*)buffer);
    return ret;
}

int android_drm_DrmManagerClient_setPlaybackStatus (JNIEnv* env, jobject thiz, jint uniqueId, jobject handle, jint status) {
    jclass clazz = env->FindClass("android/drm/DecryptHandle");
    jfieldID fieldId = env->GetFieldID(clazz, "mNativeContext", "J");
    DecryptHandleWrapper* wrapper = (DecryptHandleWrapper*)env->GetLongField(handle, fieldId);
    ALOGE("sunway: setPlaybackStatus: decryptHandle id: %d", wrapper->handle->decryptId);
    return getDrmManagerClientImpl(env, thiz)->setPlaybackStatus(uniqueId, wrapper->handle, status, 0);
}

static jint android_drm_DrmManagerClient_closeDecryptSession(
    JNIEnv* env, jobject thiz, jint uniqueId, jobject handler) {

    jclass clazz = env->FindClass("android/drm/DecryptHandle");
    jfieldID fieldId = env->GetFieldID(clazz, "mNativeContext", "J");
    DecryptHandleWrapper* wrapper = (DecryptHandleWrapper*)env->GetLongField(handler, fieldId);
    return getDrmManagerClientImpl(env, thiz)->closeDecryptSession(uniqueId, wrapper->handle);
}

static jobject android_drm_DrmManagerClient_openDecryptSession (
    JNIEnv* env, jobject thiz, jint uniqueId, jstring jpath, jstring jmimetype) {
    char* mimeType = const_cast< char* > (env->GetStringUTFChars(jmimetype, NULL));
    char* path = const_cast< char* > (env->GetStringUTFChars(jpath, NULL));

    int fd = ::open(path, O_RDONLY);
    env->ReleaseStringUTFChars(jpath, path);
    if (fd == -1) {
        return NULL;
    }

    sp<DecryptHandle> nativeHandle = getDrmManagerClientImpl(env, thiz)->openDecryptSession(uniqueId, fd, 0, 0, mimeType);
    env->ReleaseStringUTFChars(jmimetype, mimeType);
    ::close(fd);

    if (nativeHandle == NULL) {
        return NULL;
    }

    jclass clazz = env->FindClass("android/drm/DecryptHandle");
    jmethodID ctorId = env->GetMethodID(clazz, "<init>", "()V");
    jobject ret = env->NewObject(clazz, ctorId);
    jfieldID fieldId = env->GetFieldID(clazz, "mNativeContext", "J");
    sp<DecryptHandleWrapper> wrapper = new DecryptHandleWrapper();
    if (wrapper.get()) {
        wrapper->handle = nativeHandle;
        wrapper->incStrong(ret);
    }
    env->SetLongField(ret, fieldId, reinterpret_cast<jlong>(wrapper.get()));
    ALOGE("sunway: openDecryptSession: decryptHandle id: %d", nativeHandle->decryptId);
    return ret;
}

static JNINativeMethod nativeMethods[] = {

    {"_openDecryptSession", "(ILjava/lang/String;Ljava/lang/String;)Landroid/drm/DecryptHandle;",
                                    (void*)android_drm_DrmManagerClient_openDecryptSession},
    {"_closeDecryptSession", "(ILandroid/drm/DecryptHandle;)I",
                                    (void*)android_drm_DrmManagerClient_closeDecryptSession},
    {"_setPlaybackStatus", "(ILandroid/drm/DecryptHandle;I)I",
                                    (void*)android_drm_DrmManagerClient_setPlaybackStatus},
    {"_pread", "(ILandroid/drm/DecryptHandle;II)[B",
                                    (void*)android_drm_DrmManagerClient_pread}
};

int registerNativeMethodsEx(JNIEnv* env) {
    int result = -1;

    /* look up the class */
    jclass clazz = env->FindClass("android/drm/DrmManagerClientEx");

    if (NULL != clazz) {
        if (env->RegisterNatives(clazz, nativeMethods, sizeof(nativeMethods)
                / sizeof(nativeMethods[0])) == JNI_OK) {
            result = 0;
        }
    }
    return result;
}
