LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	refnotify.c

LOCAL_SHARED_LIBRARIES := \
    libhardware_legacy \
    libc \
    libutils \
    libcutils

LOCAL_MODULE := refnotify
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)
