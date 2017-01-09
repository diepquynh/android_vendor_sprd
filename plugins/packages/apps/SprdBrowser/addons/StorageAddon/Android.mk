# Build addon module

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optinal

LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_RESOURCE_DIR := $(LOCAL_PATH)/res

LOCAL_AAPT_FLAGS := --auto-add-overlay

LOCAL_PACKAGE_NAME := SprdBrowserStorageAddon

LOCAL_APK_LIBRARIES += SprdBrowser

LOCAL_PROGUARD_ENABLED := disabled

include $(BUILD_ADDON_PACKAGE)
include $(call all-makefiles-under,$(LOCAL_PATH))
