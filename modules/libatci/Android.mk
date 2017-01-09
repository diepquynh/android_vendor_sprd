# Copyright (C) 2016 Spreadtrum Communications Inc.
#

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    atci.cpp

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \
    libbinder \
    libutils \
    librilutils

LOCAL_MODULE := libatci

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
