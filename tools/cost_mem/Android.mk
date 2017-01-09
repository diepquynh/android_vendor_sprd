LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := cost_mem.c
LOCAL_MODULE := costmem
LOCAL_STATIC_LIBRARIES := libcutils liblog
LOCAL_MODULE_TAGS := debug

include $(BUILD_EXECUTABLE)
