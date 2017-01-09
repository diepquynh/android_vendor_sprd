ifneq ($(TARGET_SIMULATOR),true)
LOCAL_PATH:= $(call my-dir)

include $(LOCAL_PATH)/$(TARGET_GPU_PLATFORM)/Android.mk

endif



