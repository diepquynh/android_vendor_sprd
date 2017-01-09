LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_RES_FILES := $(LOCAL_PATH)/res
LOCAL_SRC_FILES := \
        $(call all-java-files-under, src) \
        src/com/sprd/voicetrigger/aidl/VoiceTriggerAidlService.aidl \
        src/com/sprd/voicetrigger/aidl/IVoiceTriggerUnlock.aidl
LOCAL_ASSET_DIR := $(LOCAL_PATH)/assets

LOCAL_PACKAGE_NAME := VoiceTrigger
LOCAL_CERTIFICATE := platform
LOCAL_MODULE_TAGS := optional
#LOCAL_MODULE_TAGS := tests
LOCAL_JNI_SHARED_LIBRARIES := libjni_udtsid
LOCAL_PROGUARD_ENABLED := disabled

LOCAL_MULTILIB := 32

ifneq ($(strip $(PRODUCT_DISABLED_VOICETRIGGER)),true)
include $(BUILD_PACKAGE)
endif

include $(call all-makefiles-under,$(LOCAL_PATH))
