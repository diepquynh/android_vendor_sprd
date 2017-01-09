# Copyright (C) 2015 Spreadtrum Communications Inc.
#

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
#fff
#LOCAL_C_INCLUDES :=$(LOCAL_PATH)/../include

LOCAL_SRC_FILES := \
    sprd_atci.cpp

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \
    libbinder \
    libutils \
    librilutils

LOCAL_MODULE := libatci

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

