LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_JAVA_LIBRARIES := radio_interactor_common

LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_SRC_FILES += \ $(call all-java-files-under, ../SmartCardService/common/src)
LOCAL_SRC_FILES += \ $(call all-Iaidl-files-under, ../SmartCardService/common/src)
LOCAL_AIDL_INCLUDES := $(LOCAL_PATH)/../SmartCardService/common/src/

LOCAL_PACKAGE_NAME := UiccTerminal
LOCAL_CERTIFICATE := platform

LOCAL_PROGUARD_ENABLED := disabled
LOCAL_DEX_PREOPT := false

include $(BUILD_PACKAGE)
