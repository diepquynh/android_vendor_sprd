LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../include
LOCAL_C_COMMON_LIB_SRC := $(shell cd $(LOCAL_PATH); ls ../lib/*.c)
LOCAL_SRC_FILES := $(LOCAL_C_COMMON_LIB_SRC)
LOCAL_SHARED_LIBRARIES := liblog
LOCAL_MODULE_TAGS := optional
#-----------------------------------
LOCAL_SRC_FILES += sprd_agps_agent.c
LOCAL_MODULE := libsprd_agps_agent
include $(BUILD_SHARED_LIBRARY)
