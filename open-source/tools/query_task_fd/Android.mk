LOCAL_PATH:= $(call my-dir)

#query_stask_fd
include $(CLEAR_VARS)
LOCAL_SRC_FILES := query_stask_fd.c
LOCAL_MODULE := query_task_fd
LOCAL_STATIC_LIBRARIES := libcutils
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

CUSTOM_MODULES += query_task_fd
