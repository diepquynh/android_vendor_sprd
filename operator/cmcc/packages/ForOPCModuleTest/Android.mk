LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES     := \
		test.c
LOCAL_MODULE := ForOPCModuleTest
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)


