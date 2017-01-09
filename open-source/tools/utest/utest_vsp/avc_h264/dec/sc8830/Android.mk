LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := utest_vsp_avch264dec
LOCAL_MODULE_TAGS := debug
LOCAL_CFLAGS := -fno-strict-aliasing -D_VSP_LINUX_ -D_VSP_
LOCAL_PRELINK_MODULE := false
LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES := avch264dec.cpp \
		../../../util/util.c

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
		$(LOCAL_PATH)/../../../util\
		$(TOP)/vendor/sprd/open-source/libs/libmemoryheapion \
		$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video

LOCAL_SHARED_LIBRARIES := libutils libmemoryheapion libdl

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)

include $(BUILD_EXECUTABLE)

