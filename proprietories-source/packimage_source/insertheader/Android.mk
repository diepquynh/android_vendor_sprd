LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := imgheaderinsert

LOCAL_SRC_FILES := imgheaderinsert.c
LOCAL_STATIC_LIBRARIES := libmincrypt

LOCAL_C_INCLUDES := $(LOCAL_PATH)

LOCAL_MODULE_PATH := $(HOST_OUT_EXECUTABLES)

include $(BUILD_HOST_EXECUTABLE)

