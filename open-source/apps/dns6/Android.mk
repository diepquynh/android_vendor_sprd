# Copyright 2006 The Android Open Source Project
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := dns6.c
LOCAL_MODULE := dns6
LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libc libcutils libnetutils
LOCAL_CFLAGS := -DMODULE \
                -DREDIRECT_SYSLOG_TO_ANDROID_LOGCAT \
                -DANDROID_CHANGES
include $(BUILD_EXECUTABLE)
