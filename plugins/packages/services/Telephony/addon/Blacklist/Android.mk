LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)


LOCAL_SRC_FILES := $(call all-java-files-under, src)


LOCAL_MODULE_TAGS := optional


LOCAL_PACKAGE_NAME := CallFirewallPlugin_phone

LOCAL_JAVA_LIBRARIES += telephony-common voip-common

#LOCAL_STATIC_JAVA_LIBRARIES := com.android.phone.shared \
#      guava

LOCAL_APK_LIBRARIES += TeleService

LOCAL_PROGUARD_FLAG_FILES := proguard.flags


include $(BUILD_ADDON_PACKAGE)





