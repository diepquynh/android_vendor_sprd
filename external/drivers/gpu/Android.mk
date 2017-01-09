ifneq ($(TARGET_SIMULATOR),true)
LOCAL_PATH:= $(call my-dir)

ifeq ($(strip $(TARGET_GPU_PLATFORM)),midgard)
include $(LOCAL_PATH)/midgard/Android.mk
else ifeq ($(strip $(TARGET_GPU_PLATFORM)),rogue)
include $(LOCAL_PATH)/rogue/Android.mk
else ifeq ($(strip $(TARGET_GPU_PLATFORM)),soft)
include $(LOCAL_PATH)/soft/Android.mk
else
include $(LOCAL_PATH)/utgard/Android.mk
endif


endif


