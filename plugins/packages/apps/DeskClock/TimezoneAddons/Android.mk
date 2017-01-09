LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_MODULE_TAGS := optional

#LOCAL_CERTIFICATE := platform
LOCAL_AAPT_FLAGS := --auto-add-overlay

LOCAL_PACKAGE_NAME := AddonDeskClockTimezone

LOCAL_APK_LIBRARIES += DeskClock

#LOCAL_JAVA_LIBRARIES += telephony-common

LOCAL_DEX_PREOPT := false

LOCAL_PROGUARD_ENABLED := disabled

include $(BUILD_ADDON_PACKAGE)
include $(call all-makefiles-under,$(LOCAL_PATH))
