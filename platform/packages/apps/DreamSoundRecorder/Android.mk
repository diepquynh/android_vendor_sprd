LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-subdir-java-files)

LOCAL_PACKAGE_NAME := DreamSoundRecorder
LOCAL_OVERRIDES_PACKAGES := SoundRecorder
LOCAL_CERTIFICATE := platform
LOCAL_JAVA_LIBRARIES += sprd-framework

include $(BUILD_PACKAGE)
