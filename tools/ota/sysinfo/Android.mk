LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES += \
    bootable/recovery

LOCAL_SRC_FILES := \
    check.c
LOCAL_STATIC_LIBRARIES += libcrypto_static
LOCAL_MODULE := libsysinfo
include $(BUILD_STATIC_LIBRARY)
