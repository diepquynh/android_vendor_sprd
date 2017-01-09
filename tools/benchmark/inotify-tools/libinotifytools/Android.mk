LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := inotifytools.c redblack.c

LOCAL_MODULE := libinotifytools
LOCAL_MODULE_TAGS := optional
iLOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_STATIC_LIBRARIES := libc libcutils


include $(BUILD_STATIC_LIBRARY)
