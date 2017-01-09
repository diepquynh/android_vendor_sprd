LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)


LOCAL_SRC_FILES := $(call all-java-files-under, src)


LOCAL_MODULE_TAGS := optional


LOCAL_PACKAGE_NAME := TelephonyOrangePlugin

LOCAL_JAVA_LIBRARIES += telephony-common sprd-support-addon

LOCAL_APK_LIBRARIES += TeleService

LOCAL_PROGUARD_FLAG_FILES := proguard.flags

res_dirs := res

include $(BUILD_ADDON_PACKAGE)
include $(call all-makefiles-under,$(LOCAL_PATH))

