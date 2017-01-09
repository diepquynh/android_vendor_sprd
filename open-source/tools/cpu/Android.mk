LOCAL_PATH:= $(call my-dir)

#cpu
include $(CLEAR_VARS)
LOCAL_SRC_FILES := cpu.c
LOCAL_MODULE := cpu
LOCAL_STATIC_LIBRARIES := libcutils
LOCAL_MODULE_TAGS := optional
#LOCAL_LDLIBS += -lpthread
include $(BUILD_EXECUTABLE)

CUSTOM_MODULES += cpu
