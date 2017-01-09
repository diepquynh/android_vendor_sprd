LOCAL_PATH:= $(call my-dir)
ifeq ($(strip $(FOTA_UPDATE_SUPPORT)), true)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := FotaUpdate
LOCAL_MODULE_CLASS := APPS
LOCAL_MODULE_SUFFIX := $(COMMON_ANDROID_PACKAGE_SUFFIX)
LOCAL_CERTIFICATE := PRESIGNED
ifeq ($(strip $(FOTA_UPDATE_WITHOUT_MENU)), true)
LOCAL_SRC_FILES := app/noMenu/FotaUpdate.apk
else
ifeq ($(strip $(FOTA_UPDATE_WITH_ICON)), true)
LOCAL_SRC_FILES := app/withIcon/FotaUpdate.apk
else
LOCAL_SRC_FILES := app/noIcon/FotaUpdate.apk
endif
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := UpdateProvider
LOCAL_MODULE_CLASS := APPS
LOCAL_MODULE_SUFFIX := $(COMMON_ANDROID_PACKAGE_SUFFIX)
LOCAL_CERTIFICATE := platform
LOCAL_SRC_FILES := app/UpdateProvider.apk
include $(BUILD_PREBUILT)
endif
