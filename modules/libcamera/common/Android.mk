LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -fno-strict-aliasing -Wno-unused-parameter -Werror

LOCAL_C_INCLUDES := $(LOCAL_PATH)/inc

LOCAL_SRC_FILES += $(shell find $(LOCAL_PATH) -name '*.c' | sed s:^$(LOCAL_PATH)/::g )

include $(LOCAL_PATH)/../SprdCtrl.mk

LOCAL_MODULE := libcamcommon

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := libcutils

include $(BUILD_SHARED_LIBRARY)
