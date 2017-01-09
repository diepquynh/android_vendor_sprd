
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES += \
	external/tinyalsa/include \
	frameworks/base/include

LOCAL_SHARED_LIBRARIES:= libcutils libutils libtinyalsa libaudioutils

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := libFMHalSource

LOCAL_SRC_FILES := FMHalSource.cpp RingBuffer.cpp tinyalsa_util.c

include $(BUILD_SHARED_LIBRARY)

