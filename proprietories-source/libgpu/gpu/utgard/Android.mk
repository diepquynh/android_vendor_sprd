ifneq ($(TARGET_SIMULATOR),true)
LOCAL_PATH:= $(call my-dir)


ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libGLES_mali.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_RELATIVE_PATH := egl
LOCAL_MULTILIB := both
#LOCAL_MODULE_STEM_32 := libGLES_mali.so
#LOCAL_MODULE_STEM_64 := libGLES_mali.so
ifneq ($(strip $(TARGET_BUILD_VARIANT)), user)
LOCAL_SRC_FILES_32 :=  debug/libGLES_mali.so
LOCAL_SRC_FILES_64 :=  debug/libGLES_mali_64.so
else
LOCAL_SRC_FILES_32 :=  usr/libGLES_mali.so
LOCAL_SRC_FILES_64 :=  usr/libGLES_mali_64.so
endif

include $(BUILD_PREBUILT)

#hw
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),)
LOCAL_MODULE := gralloc.default.so
else
LOCAL_MODULE := gralloc.$(TARGET_BOARD_PLATFORM).so
endif

LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MULTILIB := both
ifeq ($(strip $(TARGET_BUILD_VARIANT)), user)
LOCAL_SRC_FILES_32 :=  usr/gralloc.utgard.so
LOCAL_SRC_FILES_64 :=  usr/gralloc.utgard_64.so
else
LOCAL_SRC_FILES_32 :=  debug/gralloc.utgard.so
LOCAL_SRC_FILES_64 :=  debug/gralloc.utgard_64.so
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

#hw
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),)
LOCAL_MODULE := gralloc.default.so
else
LOCAL_MODULE := gralloc.$(TARGET_BOARD_PLATFORM).so
endif

LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MULTILIB := both
ifeq ($(strip $(TARGET_BUILD_VARIANT)), user)
LOCAL_SRC_FILES :=  usr/gralloc.utgard.so
else
LOCAL_SRC_FILES :=  debug/gralloc.utgard.so
endif

include $(BUILD_PREBUILT)
endif

endif
