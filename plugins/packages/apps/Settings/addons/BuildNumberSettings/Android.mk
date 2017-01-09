LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_CERTIFICATE := platform

LOCAL_PACKAGE_NAME := BuildNumberSettings

LOCAL_APK_LIBRARIES += Settings

LOCAL_PROGUARD_ENABLED := disabled

include $(BUILD_ADDON_PACKAGE)
