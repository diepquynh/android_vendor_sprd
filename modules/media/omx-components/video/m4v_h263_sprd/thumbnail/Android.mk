LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
        SoftSPRDMPEG4.cpp	\

LOCAL_C_INCLUDES := \
        frameworks/av/media/libstagefright/include \
	frameworks/native/include/media/hardware \
	frameworks/native/include/ui \
	frameworks/native/include/utils \
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


LOCAL_MODULE := libstagefright_sprd_soft_mpeg4dec
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

