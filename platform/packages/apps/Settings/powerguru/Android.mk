LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_CERTIFICATE := platform

LOCAL_SRC_FILES := $(call all-java-files-under, src compat_src/$(PLATFORM_SDK_VERSION))

LOCAL_PACKAGE_NAME := HeartbeatSyncSettings

# LOCAL_OVERRIDES_PACKAGES := HeartbeatSynchronization

include $(BUILD_PACKAGE)#
