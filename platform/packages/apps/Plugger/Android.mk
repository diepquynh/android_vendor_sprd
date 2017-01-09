LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(call all-java-files-under, app/src)
LOCAL_RESOURCE_DIR := $(addprefix $(LOCAL_PATH)/, app/src/main/res)
LOCAL_PACKAGE_NAME := Pluggersprd
LOCAL_JAVA_LIBRARIES += telephony-common
LOCAL_JAVA_LIBRARIES += android.test.runner
LOCAL_STATIC_JAVA_LIBRARIES += android-support-v4

LOCAL_CERTIFICATE := platform
LOCAL_PRIVILEGED_MODULE := true
include $(BUILD_PACKAGE)

include $(call all-makefiles-under,$(LOCAL_PATH))
