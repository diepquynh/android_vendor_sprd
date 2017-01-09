# Spreadtrum Communications Inc.
# Suggest to use this Android.mk like Annotation

# The LOCAL_PATH must be defined, build error if not, my-dir is a definition
# help us to find where we are easily.
LOCAL_PATH:= $(call my-dir)

# Secure Authentication start
include $(CLEAR_VARS)
LOCAL_MODULE := GeneralSecure
LOCAL_SRC_FILES := GeneralSecure/$(LOCAL_MODULE).apk
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := APPS
LOCAL_MODULE_SUFFIX := $(COMMON_ANDROID_PACKAGE_SUFFIX)
LOCAL_CERTIFICATE := platform
LOCAL_PRIVILEGED_MODULE := true
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := tsiptableserver
LOCAL_SRC_FILES := GeneralSecure/$(LOCAL_MODULE)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_PATH := $(PRODUCT_OUT)/system/bin
include $(BUILD_PREBUILT)
# Secure Authentication end

