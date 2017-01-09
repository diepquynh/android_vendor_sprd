LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
        src/com/sprd/firewall/db/BlackColumns.java

LOCAL_MODULE := com.sprd.firewall
include $(BUILD_STATIC_JAVA_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-java-files-under,src)

LOCAL_JAVA_LIBRARIES := telephony-common

LOCAL_STATIC_JAVA_LIBRARIES := \
    android-support-v13 \
    android-support-v4 \

LOCAL_MODULE_PATH := $(TARGET_OUT)/vital-app
LOCAL_MODULE_PATH := $(TARGET_OUT)/app
LOCAL_PACKAGE_NAME := CallFireWall

LOCAL_CERTIFICATE := platform

include $(BUILD_PACKAGE)

#Use the following include to make our test apk.
include $(call all-makefiles-under,$(LOCAL_PATH))

