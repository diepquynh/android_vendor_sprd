LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := eng debug

LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_JAVA_LIBRARIES := telephony-common
//LOCAL_JAVA_LIBRARIES += telephony-common2

LOCAL_PACKAGE_NAME := SprdSlt
LOCAL_CERTIFICATE := platform


include $(BUILD_PACKAGE)

# Use the folloing include to make our test apk.
include $(call all-makefiles-under,$(LOCAL_PATH))
