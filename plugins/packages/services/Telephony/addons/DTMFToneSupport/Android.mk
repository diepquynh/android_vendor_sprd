LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := DTMFToneSupport

LOCAL_APK_LIBRARIES += TeleService

LOCAL_JAVA_LIBRARIES += telephony-common sprd-support-addon

LOCAL_DEX_PREOPT := false

LOCAL_PROGUARD_ENABLED := disabled

include $(BUILD_ADDON_PACKAGE)