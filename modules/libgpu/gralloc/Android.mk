ifneq ($(TARGET_SIMULATOR),true)
LOCAL_PATH:= $(call my-dir)

ifeq ($(strip $(TARGET_GPU_PLATFORM)),midgard)
include $(LOCAL_PATH)/midgard/Android.mk
else
include $(LOCAL_PATH)/utgard/Android.mk
endif

endif

