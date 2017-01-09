LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
# Include res dir from chips
chips_dir := ../../../../../frameworks/ex/chips/res
res_dirs := $(chips_dir) res

$(shell rm -f $(LOCAL_PATH)/chips)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := $(call all-java-files-under,src)

# TODO: Remove dependency of application on the test runner (android.test.runner)
# library.

LOCAL_JAVA_LIBRARIES += telephony-common mms-common

LOCAL_STATIC_JAVA_LIBRARIES += android-common jsr305
LOCAL_STATIC_JAVA_LIBRARIES += android-common-chips

LOCAL_RESOURCE_DIR := $(addprefix $(LOCAL_PATH)/, $(res_dirs))
LOCAL_AAPT_FLAGS := --auto-add-overlay
LOCAL_AAPT_FLAGS += --extra-packages com.android.ex.chips

LOCAL_PACKAGE_NAME := InsertData

include $(BUILD_PACKAGE)

# Use the following include to make our test apk.
include $(call all-makefiles-under,$(LOCAL_PATH))
