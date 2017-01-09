# Spreadtrum busmonitor hardware layer

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

#LOCAL_CFLAGS := -DSCX15 -I$(LOCAL_PATH)
LOCAL_SRC_FILES:= sprd_bm.c

LOCAL_MODULE:= libbm

LOCAL_SHARED_LIBRARIES:= liblog libc libcutils

LOCAL_C_INCLUDES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_MODULE_TAGS:= optional

include $(BUILD_SHARED_LIBRARY)
