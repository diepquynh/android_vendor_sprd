ifneq ($(TARGET_SIMULATOR),true)
LOCAL_PATH := $(call my-dir)

ifeq ($(strip $(TARGET_ARCH)),x86_64)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := gralloc.$(TARGET_BOARD_PLATFORM).so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MULTILIB := both
LOCAL_SRC_FILES_32 :=  x86/lib/gralloc.soft.so
LOCAL_SRC_FILES_64 :=  x86/lib64/gralloc.soft.so
include $(BUILD_PREBUILT)

else
include $(CLEAR_VARS)

include $(BUILD_PREBUILT)
endif

endif



