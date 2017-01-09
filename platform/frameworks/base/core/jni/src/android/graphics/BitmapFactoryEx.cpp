#define LOG_TAG "BitmapFactoryEx"

#include "BitmapFactory.h"
#include "NinePatchPeeker.h"
#include "SkFrontBufferedStream.h"
#include "SkImageDecoder.h"
#include "SkMath.h"
#include "SkPixelRef.h"
#include "SkStream.h"
#include "SkTemplates.h"
#include "SkUtils.h"
#include "CreateJavaOutputStreamAdaptor.h"
#include "Utils.h"
#include "JNIHelp.h"
#include "GraphicsJNI.h"

#include "core_jni_helpers.h"
#include <androidfw/Asset.h>
#include <androidfw/ResourceTypes.h>
#include <cutils/compiler.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <drm/DrmManagerClient.h>

using namespace android;

jobject doDecode(JNIEnv* env, SkStreamRewindable* stream, jobject padding, jobject options);

static jobject nativeDecodeDrmStream(JNIEnv* env, jobject clazz, jobject client, jobject handle,
                            jobject opts, jboolean applyScale, jfloat scale) {
    jobject bitmap = NULL;

    jclass tmpClazz = env->FindClass("android/drm/DrmManagerClientEx");
    jfieldID fieldId = env->GetFieldID(tmpClazz, "mNativeContext", "J");
    DrmManagerClientImpl* nativeClient = (DrmManagerClientImpl*)env->GetLongField(client, fieldId);
    fieldId = env->GetFieldID(tmpClazz, "mUniqueId", "I");
    int uniqueId = env->GetIntField(client, fieldId);

    tmpClazz = env->FindClass("android/drm/DecryptHandle");
    fieldId = env->GetFieldID(tmpClazz, "mNativeContext", "J");
    DecryptHandleWrapper* nativeHandleWrapper = (DecryptHandleWrapper*)env->GetLongField(handle, fieldId);

    SkStreamRewindable* stream = new SkDrmStream(uniqueId, nativeClient, nativeHandleWrapper);

    if (stream) {
        bitmap = doDecode(env, stream, NULL, opts);
        // On Android-N, 'stream' is no longer need to be free here,
        // because of the function 'doDecode' in class BitmapFactory
        // will help to free 'stream'.
        // delete stream;
        stream = NULL;
    }
    return bitmap;
}

static JNINativeMethod gMethodsEx[] = {
    {   "nativeDecodeDrmStream",
        "(Landroid/drm/DrmManagerClientEx;Landroid/drm/DecryptHandle;Landroid/graphics/BitmapFactory$Options;ZF)Landroid/graphics/Bitmap;",
        (void*)nativeDecodeDrmStream
    },
};

int register_android_graphics_BitmapFactoryEx(JNIEnv* env) {
    return android::RegisterMethodsOrDie(env, "android/graphics/BitmapFactoryEx",
                                         gMethodsEx, NELEM(gMethodsEx));
}
