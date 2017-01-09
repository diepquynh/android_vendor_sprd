LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
    ubiutils.c

LOCAL_C_INCLUDES += \
    external/kernel-headers/original/uapi/mtd \
    bootable/recovery

LOCAL_MODULE := libubiutils
LOCAL_STATIC_LIBRARIES := libmtdutils
include $(BUILD_STATIC_LIBRARY)

