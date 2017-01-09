LOCAL_PATH:= $(call my-dir)

# UASetting //modify LOCAL_MODULE_PATH to make the apk cannot be uninstall
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := UASetting
LOCAL_MODULE_STEM := UASetting.apk
LOCAL_MODULE_CLASS := APPS
LOCAL_CERTIFICATE := platform
LOCAL_MODULE_PATH := $(TARGET_OUT)/app
LOCAL_SRC_FILES := UASetting.apk
include $(BUILD_PREBUILT)

include $(call all-makefiles-under,$(LOCAL_PATH))
