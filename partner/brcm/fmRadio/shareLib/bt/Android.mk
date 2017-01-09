LOCAL_PATH := $(call my-dir)

# the library
# ============================================================
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	$(call all-subdir-java-files) \
	src/com/broadcom/fm/fmreceiver/IFmReceiverService.aidl\
	src/com/broadcom/fm/fmreceiver/IFmReceiverCallback.aidl

LOCAL_MODULE_TAGS := optional

# This is the target being built.
LOCAL_MODULE:= com.broadcom.bt

include $(BUILD_JAVA_LIBRARY)

# Install permissions for this shared jar
# ====================================================================
include $(CLEAR_VARS)

LOCAL_MODULE := com.broadcom.bt.xml

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_CLASS := ETC

LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/permissions

LOCAL_SRC_FILES := $(LOCAL_MODULE)

include $(BUILD_PREBUILT)

include $(call all-makefiles-under,$(LOCAL_PATH))

