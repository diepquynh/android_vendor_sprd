LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_JAVA_LIBRARIES := radio_interactor_common

LOCAL_SRC_FILES := $(call all-java-files-under, $(src_dirs))

LOCAL_PACKAGE_NAME := radio_interactor_service

LOCAL_MODULE_TAGS := optional
LOCAL_CERTIFICATE := platform
LOCAL_PRIVILEGED_MODULE := true

LOCAL_PROGUARD_ENABLED := disabled

include $(BUILD_PACKAGE)
include $(call all-makefiles-under,$(LOCAL_PATH))
