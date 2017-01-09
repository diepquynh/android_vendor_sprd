LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_MODULE_TAGS := optional

#The LOCAL_SDK_VERSION build variable will make your
#application can not use the new @hide API(@link AddonManager)
#So you need to add the sprd-support-addon java library
LOCAL_SDK_VERSION := current
LOCAL_JAVA_LIBRARIES := sprd-support-addon

LOCAL_DEX_PREOPT := false

#If the PROGUARD of host application is enable like
#"LOCAL_PROGUARD_FLAG_FILES := proguard.flags", you
#need to keep the unused method by proguard.flags
#LOCAL_PROGUARD_ENABLED := disabled
LOCAL_PROGUARD_FLAG_FILES := proguard.flags

LOCAL_PACKAGE_NAME := HelloWorld

#Set the Same certificate key and userid to make the
#two app process(host app and plugin) share data.
#The default certificate is testkey.
#LOCAL_CERTIFICATE := testkey

include $(BUILD_PACKAGE)
