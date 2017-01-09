LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := boot_complete.c
LOCAL_MODULE := boot_complete
LOCAL_STATIC_LIBRARIES := libcutils
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

CUSTOM_MODULES += boot_complete
