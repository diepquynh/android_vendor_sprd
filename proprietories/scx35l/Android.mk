LOCAL_PATH := $(call my-dir)

define proprietary-prebuild
$(if $(wildcard $(LOCAL_PATH)/$(LOCAL_SRC_FILES)), \
    $(eval include $(BUILD_PREBUILT)), \
    $(eval include $(CLEAR_VARS)) \
)
endef

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := ims
LOCAL_MODULE_STEM := ims.apk
LOCAL_MODULE_CLASS := APPS
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_SRC_FILES := system/app/ims/ims.apk
$(call proprietary-prebuild)
