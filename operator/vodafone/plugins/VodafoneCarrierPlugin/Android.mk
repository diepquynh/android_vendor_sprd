LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_VODAFONE_VENDOR_RELATIVE_PATH :=

m := $(subst /, ,$(LOCAL_PATH))
m := $(patsubst %,..,$(m))
$(foreach n,$(m),\
   $(eval LOCAL_VODAFONE_VENDOR_RELATIVE_PATH := ../$(LOCAL_VODAFONE_VENDOR_RELATIVE_PATH)) \
)
vodafone_plugins := \
    $(LOCAL_VODAFONE_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/SprdStk/addons/StkSetupMenu \
    $(LOCAL_VODAFONE_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/Dialer/InCallUI/addons/explicitCallTransfer \
    $(LOCAL_VODAFONE_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/Settings/addons/VodafoneApnPlugin \

vodafone_plugins_src := $(foreach n,$(vodafone_plugins),$(n)/src)
vodafone_plugins_res := $(foreach n,$(vodafone_plugins), $(wildcard $(LOCAL_PATH)/$(n)/res))

LOCAL_MODULE_TAGS := optional

LOCAL_CERTIFICATE := platform

LOCAL_PACKAGE_NAME := VodafonePlugins

LOCAL_JAVA_LIBRARIES += radio_interactor_common

LOCAL_RESOURCE_DIR := $(vodafone_plugins_res)

LOCAL_OVERRIDES_PACKAGES := \
    StkSetupMenu \
    ExplicitCallTransferPlugin \
    VodafoneApnPlugin

LOCAL_SRC_FILES := $(call all-java-files-under, $(vodafone_plugins_src))

LOCAL_DEX_PREOPT := false

LOCAL_APK_LIBRARIES += \
    SprdStk \
    Dialer \
    Settings

LOCAL_AAPT_FLAGS += \
    --auto-add-overlay \
    --extra-packages plugin.sprd.stksetupmenu \
    --extra-packages com.sprd.incallui.explicitCallTransferPlugin \
    --extra-packages plugin.sprd.apneditor

LOCAL_PROGUARD_ENABLED := disabled

include $(BUILD_ADDON_PACKAGE)

