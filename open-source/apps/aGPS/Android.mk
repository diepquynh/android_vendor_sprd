LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include
LOCAL_C_COMMON_LIB_SRC := $(shell cd $(LOCAL_PATH); ls lib/*.c)
LOCAL_SRC_FILES := $(LOCAL_C_COMMON_LIB_SRC)
LOCAL_SHARED_LIBRARIES := liblog
LOCAL_MODULE_TAGS := optional
#-----------------------------------
LOCAL_SRC_FILES += main.c
LOCAL_MODULE := atd
include $(BUILD_EXECUTABLE)
include $(LOCAL_PATH)/generic/Android.mk
