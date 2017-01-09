LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := AudioProfile
LOCAL_CERTIFICATE := platform
#LOCAL_JAVA_LIBRARIES := telephony-common2
LOCAL_PRIVILEGED_MODULE := true

#myfile=$(TOP)/vendor/ts/proprietary-bin/app/$(LOCAL_PACKAGE_NAME).apk
#file := $(TARGET_OUT)/app/$(LOCAL_PACKAGE_NAME).apk

include $(BUILD_PACKAGE)

#$(myfile) : $(file) | $(ACP)
#	$(transform-prebuilt-to-target)
#ALL_PREBUILT += $(myfile)

# Use the folloing include to make our test apk.
include $(call all-makefiles-under,$(LOCAL_PATH))
