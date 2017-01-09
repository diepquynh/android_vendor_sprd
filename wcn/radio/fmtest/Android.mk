LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	fmtest.c

LOCAL_SHARED_LIBRARIES := \
		libutils \
		libcutils

LOCAL_MODULE := fmtest
LOCAL_MODULE_TAGS := debug

include $(BUILD_EXECUTABLE)
