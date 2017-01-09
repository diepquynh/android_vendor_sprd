ifneq ($(TARGET_SIMULATOR),true)
ifeq ($(TARGET_ARCH),arm)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false
LOCAL_LDLIBS += -Idl

LOCAL_SRC_FILES := \
	battery.c

LOCAL_MODULE := batterysrv
LOCAL_MODULE_TAGS := optional
LOCAL_FORCE_STATIC_EXECUTABLE := true
TARGET_SYSTEM_OUT_BIN := $(PRODUCT_OUT)/system/bin
LOCAL_MODULE_PATH := $(TARGET_SYSTEM_OUT_BIN)

LOCAL_STATIC_LIBRARIES := libcutils libc liblog

include $(BUILD_EXECUTABLE)

endif   # TARGET_ARCH == arm
endif    # !TARGET_SIMULATOR

