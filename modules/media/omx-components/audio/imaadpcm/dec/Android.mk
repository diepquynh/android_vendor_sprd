LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
        SoftIMAADPCM.cpp

LOCAL_C_INCLUDES := \
        frameworks/av/media/libstagefright/include \
        $(LOCAL_PATH)/../../../../libstagefrighthw/include \
        $(LOCAL_PATH)/../../../../libstagefrighthw/include/openmax \
        $(LOCAL_PATH)/../../../../../libmemion
LOCAL_SHARED_LIBRARIES := \
        libstagefright libstagefright_omx libstagefright_foundation libutils liblog libstagefrighthw

LOCAL_MODULE := libstagefright_soft_imaadpcmdec
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
