ifeq ($(strip $(FOTA_REDSTONE_SUPPORT)), yes)
#com.redstone.ota.ui
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := com.redstone.ota.ui
LOCAL_MODULE_TAGS := optional
ifeq ($(strip $(FOTA_APK_ICON)), yes)
LOCAL_SRC_FILES := ./ICON/com.redstone.ota.ui.apk
else
ifeq ($(strip $(FOTA_APK_NOICON)), yes)
LOCAL_SRC_FILES := ./NOICON/com.redstone.ota.ui.apk
endif
endif
LOCAL_MODULE_CLASS := APPS
LOCAL_MODULE_SUFFIX := $(COMMON_ANDROID_PACKAGE_SUFFIX)
LOCAL_CERTIFICATE := platform
include $(BUILD_PREBUILT)
endif
