# Spreadtrum Communication Inc. 2016

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, core/java drm/java graphics/java media/java telephony/java/android/telephony ex-interface/telephony/java/android/telephony/) \
                   $(call all-java-files-under, ../opt/telephony/src/java/com/android/internal/telephony/fdnsvr/) \
                   ../opt/telephony/src/java/android/telephony/SmsManagerEx.java

LOCAL_MODULE_TAGS := optional

LOCAL_JAVA_LIBRARIES += telephony-common

LOCAL_MODULE := sprd-framework

include $(BUILD_JAVA_LIBRARY)


##### LEGACY STATIC JAVA LIBRARIES #####

# Add for plugin mechanism support
include $(CLEAR_VARS)

LOCAL_SRC_FILES := core/java/android/app/AddonManager.java

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := sprd-support-addon

include $(BUILD_JAVA_LIBRARY)


# Add for kill stop front app api support
include $(CLEAR_VARS)

LOCAL_SRC_FILES := core/java/android/app/LowmemoryUtils.java

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := sprd-support-lowmemory

include $(BUILD_JAVA_LIBRARY)

##### IMS STATIC JAVA LIBRARIES #####
include $(CLEAR_VARS)

LOCAL_AIDL_INCLUDES := $(LOCAL_PATH)/telephony/java
LOCAL_SRC_FILES := \
    $(call all-java-files-under, telephony/java)

LOCAL_SRC_FILES += \
    telephony/java/com/android/ims/internal/IImsServiceListenerEx.aidl \
    telephony/java/com/android/ims/internal/IImsRegisterListener.aidl \
    telephony/java/com/android/ims/internal/IImsUtListenerEx.aidl \
    telephony/java/com/android/ims/internal/IImsUtEx.aidl \
    telephony/java/com/android/ims/internal/IImsServiceEx.aidl

LOCAL_AIDL_INCLUDES += \
    telephony/java/com/android/ims/internal/IImsServiceListenerEx.aidl \
    telephony/java/com/android/ims/internal/IImsRegisterListener.aidl \
    telephony/java/com/android/ims/internal/IImsUtListenerEx.aidl \
    telephony/java/com/android/ims/internal/IImsUtEx.aidl \
    telephony/java/com/android/ims/internal/IImsServiceEx.aidl

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := ims-api

include $(BUILD_JAVA_LIBRARY)

# Add for app clone api support
include $(CLEAR_VARS)

LOCAL_SRC_FILES := core/java/android/content/pm/AppCloneUserInfo.java

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := app-clone

include $(BUILD_JAVA_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := core/java/android/print/PrintManagerHelper.java

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := sprd-support-print

include $(BUILD_STATIC_JAVA_LIBRARY)

include $(CLEAR_VARS)
include $(call all-makefiles-under, $(LOCAL_PATH))
