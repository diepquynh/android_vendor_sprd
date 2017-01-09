LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := theme_init.sh
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_SRC_FILES := theme_init.sh
include $(BUILD_PREBUILT)
