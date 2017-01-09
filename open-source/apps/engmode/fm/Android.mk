#Build libengfm
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES := fm_cmd_executer.cpp

LOCAL_C_INCLUDES    += packages/apps/FMRadio/jni/fmr

LOCAL_SHARED_LIBRARIES += libcutils   \
                          libfmjni

LOCAL_MODULE := libengfm
LOCAL_MODULE_TAGS := debug

include $(BUILD_SHARED_LIBRARY)
