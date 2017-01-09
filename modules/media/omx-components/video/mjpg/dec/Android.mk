LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)


LOCAL_SRC_FILES := \
        SoftMJPG.cpp

LOCAL_C_INCLUDES := \
        frameworks/av/media/libstagefright/include \
        $(LOCAL_PATH)/../../../../libstagefrighthw/include/ \
        $(LOCAL_PATH)/../../../../libstagefrighthw/include/openmax \
        $(LOCAL_PATH)/../../../../../libmemion

ifeq ($(USE_EXTERNAL_JPEG),true)
LOCAL_C_INCLUDES += external/jpeg
else
LOCAL_C_INCLUDES += external/libjpeg-turbo
endif

LOCAL_CFLAGS := -DOSCL_EXPORT_REF= -DOSCL_IMPORT_REF=

LOCAL_SHARED_LIBRARIES := \
        libstagefright libstagefright_omx libstagefright_foundation libutils libdl liblog libstagefrighthw
LOCAL_MODULE := libstagefright_soft_mjpgdec
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
