LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
        SPRDMPEG4Encoder.cpp

LOCAL_C_INCLUDES := \
        frameworks/av/media/libstagefright/include \
	frameworks/native/include/media/hardware \
	$(TOP)/vendor/sprd/modules/media/libstagefrighthw/include \
	$(TOP)/vendor/sprd/modules/media/libstagefrighthw/include/openmax \
	$(TOP)/vendor/sprd/modules/libmemion \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video

LOCAL_C_INCLUDES += $(GPU_GRALLOC_INCLUDES)

LOCAL_CFLAGS := -DOSCL_EXPORT_REF= -DOSCL_IMPORT_REF=

LOCAL_ARM_MODE := arm

LOCAL_SHARED_LIBRARIES := \
        libstagefright libstagefright_omx libstagefright_foundation libstagefrighthw libutils  libui libmemion libdl liblog

LOCAL_MODULE := libstagefright_sprd_mpeg4enc
LOCAL_MODULE_TAGS := optional

ifeq ($(strip $(TARGET_BOARD_CAMERA_ANTI_SHAKE)),true)
LOCAL_CFLAGS += -DANTI_SHAKE
endif

include $(BUILD_SHARED_LIBRARY)
