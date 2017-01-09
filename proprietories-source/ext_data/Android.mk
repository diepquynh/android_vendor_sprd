LOCAL_PATH := $(call my-dir)
include $(call all-subdir-makefiles)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := ext_data.c utils.c auto_test.c

LOCAL_MODULE := ext_data
LOCAL_INIT_RC := ext_data.rc

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := -O2 -g -W -Wall -D__ANDROID__ -DNO_SCRIPT
LOCAL_SHARED_LIBRARIES := libnetutils liblog libcutils

include $(BUILD_EXECUTABLE)
