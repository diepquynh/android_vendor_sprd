LOCAL_PATH:= $(call my-dir)

#inotifywatch
include $(CLEAR_VARS)
LOCAL_SRC_FILES := inotifywatch.c common.c
LOCAL_MODULE := inotifywatch
LOCAL_MODULE_TAGS := optional
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_STATIC_LIBRARIES := libinotifytools libcutils libc
include $(BUILD_EXECUTABLE)

#inotifywait
include $(CLEAR_VARS)
LOCAL_SRC_FILES := inotifywait.c common.c

LOCAL_MODULE := inotifywait
LOCAL_MODULE_TAGS := optional
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_STATIC_LIBRARIES := libinotifytools libcutils libc
include $(BUILD_EXECUTABLE)

CUSTOM_MODULES += inotifywatch
CUSTOM_MODULES += inotifywait
