LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    show_info.c
LOCAL_MODULE := libshowinfo
include $(BUILD_STATIC_LIBRARY)
