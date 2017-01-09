/*
 * Copyright (C) 2013 The Android Open Source Project
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

#define LOG_TAG "sprdsltjni"
#include "utils/Log.h"

#include <stdint.h>
#include <jni.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <malloc.h>


#define ROOT_MAGIC 0x524F4F54 //"ROOT"
#define ROOT_OFFSET 512
#define MAX_COMMAND_BYTES               (8 * 1024)

extern "C" {
#include "atci.h"
}

typedef struct {
  uint32_t magic;
  uint32_t root_flag;
} root_stat_t;

static jstring SprdSlt_sendATCmd(JNIEnv* env, jobject thiz, jint phoneId, jstring cmd) {
   char result[MAX_COMMAND_BYTES] = {0};
   const char* atCmd = env->GetStringUTFChars(cmd,0);
   ALOGD("send AT cmd: %s",atCmd);
   int resultValue = sendATCmd(phoneId,atCmd,result,MAX_COMMAND_BYTES);
   env->ReleaseStringUTFChars(cmd, atCmd);
   if (resultValue != 0) {
        return env->NewStringUTF("ERROR");
      }
   ALOGI("the return value is: %s",result);
   return env->NewStringUTF(result);
}

        
static JNINativeMethod getMethods[] = {
        {"native_sendATCmd","(ILjava/lang/String;)Ljava/lang/String;", (void*) SprdSlt_sendATCmd },
};

static int registerNativeMethods(JNIEnv* env, const char* className,
        JNINativeMethod* gMethods, int numMethods) {
    jclass clazz;
    clazz = env->FindClass(className);
    if (clazz == NULL) {
        ALOGE("Native registration unable to find class '%s'", className);
        return JNI_FALSE;
    }
    if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
        ALOGE("RegisterNatives failed for '%s'", className);
        return JNI_FALSE;
    }
    return JNI_TRUE;
}

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env;
    //use JNI1.6
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        ALOGE("Error: GetEnv failed in JNI_OnLoad");
        return -1;
    }
      return JNI_VERSION_1_6;
}
