LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES:= $(LOCAL_PATH)/include \
                   external/tinycompress/include
LOCAL_SRC_FILES:= compress_util.c
LOCAL_MODULE := libtinycompress_util
LOCAL_SHARED_LIBRARIES:= libcutils libutils libtinycompress
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

