LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

phone_common_dir := ../../../../../../packages/apps/PhoneCommon
telephony_dir := ../../../../../../packages/services/Telephony
telephony_overlay := ../../../../feature_configs/base/overlay/packages/services/Telephony

res_dirs := res $(phone_common_dir)/res $(telephony_dir)/res $(telephony_dir)/sip/res $(telephony_overlay)/res
LOCAL_RESOURCE_DIR := $(addprefix $(LOCAL_PATH)/, $(res_dirs))

LOCAL_AAPT_FLAGS := \
    --auto-add-overlay \
    --extra-packages com.android.phone.common

LOCAL_STATIC_JAVA_LIBRARIES := \
        guava \

LOCAL_JAVA_LIBRARIES := telephony-common ims-common
LOCAL_SRC_FILES := $(call all-subdir-java-files)

LOCAL_PACKAGE_NAME := CallSettings
LOCAL_CERTIFICATE := platform

include $(BUILD_PACKAGE)
