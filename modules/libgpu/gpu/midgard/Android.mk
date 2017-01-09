ifneq ($(TARGET_SIMULATOR),true)
LOCAL_PATH:= $(call my-dir)

ifeq ($(strip $(CONFIG_64KERNEL_32FRAMEWORK)),true)
MIDGARD_ARCH_ := arm64
else
MIDGARD_ARCH_ := $(TARGET_ARCH)
endif

ifeq ($(strip $(MIDGARD_ARCH_)),arm64)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libGLES_mali.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_RELATIVE_PATH := egl
LOCAL_MULTILIB := both
ifneq ($(strip $(TARGET_BUILD_VARIANT)), user)
LOCAL_SRC_FILES_32 :=  debug/libGLES_mali.so
LOCAL_SRC_FILES_64 :=  debug/libGLES_mali_64.so
else
LOCAL_SRC_FILES_32 :=  usr/libGLES_mali.so
LOCAL_SRC_FILES_64 :=  usr/libGLES_mali_64.so
endif

include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libGLES_mali.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_RELATIVE_PATH := egl
ifneq ($(strip $(TARGET_BUILD_VARIANT)), user)
LOCAL_SRC_FILES := debug/libGLES_mali.so
else
LOCAL_SRC_FILES := usr/libGLES_mali.so
endif

include $(BUILD_PREBUILT)
endif


include $(LOCAL_PATH)/driver/Android.mk

endif
