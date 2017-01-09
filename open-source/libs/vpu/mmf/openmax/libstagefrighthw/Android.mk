LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    VPUOMXPlugin.cpp \

LOCAL_C_INCLUDES:= \
        $(TOP)/frameworks/av/media/libstagefright \
        $(TOP)/frameworks/native/include/media/hardware \
        $(TOP)/frameworks/base/include/media/stagefright/openmax \
	$(TOP)/frameworks/base/include/media/stagefright \
	$(TOP)/vendor/sprd/open-source/libs/libstagefrighthw/include/openmax

LOCAL_SHARED_LIBRARIES :=       \
        libbinder               \
        libmedia                        \
        libutils                \
        libui                           \
        libcutils               \
        libstagefright_foundation       \
	liblog \
        libdl


LOCAL_MODULE := libstagefrighthw_cm


include $(BUILD_SHARED_LIBRARY)

