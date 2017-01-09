# Copyright 2011 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)

#$(shell cp -rf $(LOCAL_PATH)/sprd/src/* $(LOCAL_PATH)/src/)
#$(shell cp -rf $(LOCAL_PATH)/sprd/res/* $(LOCAL_PATH)/res)
#$(shell cp -rf $(LOCAL_PATH)/sprd/AndroidManifest.xml $(LOCAL_PATH)/)


include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_RESOURCE_DIR := $(LOCAL_PATH)/res
LOCAL_JAVA_LIBRARIES := telephony-common
#LOCAL_JAVA_LIBRARIES += CellBroadcastReceiver
LOCAL_STATIC_JAVA_LIBRARIES += android-support-v4
LOCAL_PACKAGE_NAME := CbCustomSetting
LOCAL_CERTIFICATE := platform
LOCAL_PRIVILEGED_MODULE := true

include $(BUILD_PACKAGE)

# This finds and builds the test apk as well, so a single make does both.
include $(call all-makefiles-under,$(LOCAL_PATH))
