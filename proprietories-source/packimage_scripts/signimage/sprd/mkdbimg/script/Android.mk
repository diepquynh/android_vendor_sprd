LOCAL_PATH:=$(call my-dir)
    include $(CLEAR_VARS)

LOCAL_SRC_FILES:=sprd_mkdbimg.sh

LOCAL_MODULE:=sprd_mkdbimg.sh

LOCAL_MODULE_CLASS:=EXECUTABLES

LOCAL_IS_HOST_MODULE:=true

include $(BUILD_PREBUILT)

#include $(LOCAL_PATH)/source/Android.mk
#include $(call all-makefiles-under,$(LOCAL_PATH))
