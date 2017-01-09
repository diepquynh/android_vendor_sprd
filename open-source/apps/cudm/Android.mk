LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
#DMHis

# Only compile source java files in this apk.
LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := sprdcudm

#LOCAL_SDK_VERSION := current

LOCAL_CERTIFICATE := platform
LOCAL_PROGUARD_FLAG_FILES := proguard.flags

#LOCAL_JNI_SHARED_LIBRARIES := libdmjni
LOCAL_JAVA_LIBRARIES += telephony-common

include $(BUILD_PACKAGE)

# Use the following include to make our test apk.
include $(call all-makefiles-under,$(LOCAL_PATH))

#################################################################
####### copy the library to /system/lib #########################
#################################################################

#include $(CLEAR_VARS)
#LOCAL_MODULE := libdmjni.so
#LOCAL_MODULE_CLASS := SHARED_LIBRARIES
 
#LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
#LOCAL_SRC_FILES := jni/$(LOCAL_MODULE)
#OVERRIDE_BUILD_MODULE_PATH := $(TARGET_OUT_INTERMEDIATE_LIBRARIES)
 
#include $(BUILD_PREBUILT)
