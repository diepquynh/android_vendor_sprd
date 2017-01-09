LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libawb1
LOCAL_ARM_MODE := arm

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../inc

LOCAL_SRC_FILES := awb.c


ifneq ($(strip $(TARGET_ARCH)),arm64)
LOCAL_MODULE_PATH := $(LOCAL_PATH)/../../../libs/isp2.0/lib
LOCAL_UNSTRIPPED_PATH := $(LOCAL_PATH)/../../../libs/isp2.0/lib/symbols
else
LOCAL_MODULE_PATH_32 := $(LOCAL_PATH)/../../../libs/isp2.0/lib
LOCAL_MODULE_PATH_64 := $(LOCAL_PATH)/../../../libs/isp2.0/lib64
endif

LOCAL_SHARED_LIBRARIES := \
      libutils \
      libcutils \
      liblog

include $(BUILD_SHARED_LIBRARY)

