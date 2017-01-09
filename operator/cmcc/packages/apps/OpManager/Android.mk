LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_STATIC_JAVA_LIBRARIES := libdm_sdk_test
LOCAL_STATIC_JAVA_LIBRARIES += android-support-v4

LOCAL_REQUIRED_MODULES := libaes-jni

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := OpManager

LOCAL_CERTIFICATE := platform

include $(BUILD_PACKAGE)

#########################
include $(CLEAR_VARS)

LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES :=libdm_sdk_test:libs/dm_sdk_test_v1.0.53.2.jar

ifeq ($(strip $(TARGET_ARCH)),arm64)
LOCAL_PREBUILT_LIBS :=libaes-jni:libs/arm64-v8a/libaes-jni.so
else
LOCAL_PREBUILT_LIBS :=libaes-jni:libs/armeabi-v7a/libaes-jni.so
endif

LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)

##################
include $(call all-makefiles-under,$(LOCAL_PATH))
