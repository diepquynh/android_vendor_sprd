LOCAL_PATH:= $(call my-dir)

$(shell cp -rf $(LOCAL_PATH)/sprd/res/* $(LOCAL_PATH)/res)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := Omacp
LOCAL_JAVA_LIBRARIES += telephony-common

LOCAL_CERTIFICATE := platform
LOCAL_PRIVILEGED_MODULE := true
include $(BUILD_PACKAGE)

include $(call all-makefiles-under,$(LOCAL_PATH))
