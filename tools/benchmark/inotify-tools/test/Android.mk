LOCAL_PATH:= $(call my-dir)

#test inotifytools
include $(CLEAR_VARS)
LOCAL_SRC_FILES := test.c
LOCAL_MODULE := inotifytools_test
LOCAL_MODULE_TAGS := optional
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_STATIC_LIBRARIES := libinotifytools libcutils libc
include $(BUILD_EXECUTABLE)

CUSTOM_MODULES += inotifytools_test
