LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := packimage.sh
LOCAL_SRC_FILES := packimage.sh
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_IS_HOST_MODULE := true
include $(BUILD_PREBUILT)
include $(call all-makefiles-under,$(LOCAL_PATH))
