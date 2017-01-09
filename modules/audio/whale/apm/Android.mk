# apm/Android.mk
#
# Copyright 2012 Spreadtrum
#

# This is the sprd audio policy manager

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

  LOCAL_CFLAGS := -D_POSIX_SOURCE

  LOCAL_SRC_FILES := AudioPolicyManagerSPRD.cpp

  LOCAL_MODULE := libaudiopolicy
  LOCAL_MODULE_TAGS := optional

  LOCAL_STATIC_LIBRARIES := \
    libmedia_helper

  LOCAL_WHOLE_STATIC_LIBRARIES += libaudiopolicy_legacy

  LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libmedia \
    libbinder

  LOCAL_C_INCLUDES += \
    frameworks/av/services/volumemanager

  include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libutils \
    libmedia \
    libbinder

LOCAL_STATIC_LIBRARIES := \
    libmedia_helper

LOCAL_WHOLE_STATIC_LIBRARIES := \
    libaudiopolicy_legacy

LOCAL_SHARED_LIBRARIES += libaudiopolicy

LOCAL_MODULE := audio_policy.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
