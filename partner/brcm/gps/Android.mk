ifneq ($(TARGET_SIMULATOR),true)
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SHARED_LIBRARIES  := libcutils libbluedroid

LOCAL_C_INCLUDES := 

LOCAL_SRC_FILES     := Factory.c

LOCAL_MODULE := glgps_factory
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)
endif
