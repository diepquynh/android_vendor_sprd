LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ORANGE_VENDOR_RELATIVE_PATH :=

m := $(subst /, ,$(LOCAL_PATH))
m := $(patsubst %,..,$(m))
$(foreach n,$(m),\
   $(eval LOCAL_ORANGE_VENDOR_RELATIVE_PATH := ../$(LOCAL_ORANGE_VENDOR_RELATIVE_PATH)) \
)
orange_plugins := \
    $(LOCAL_ORANGE_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/SprdStk/addons/StkRefreshNoToast \
    $(LOCAL_ORANGE_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/SprdStk/addons/StkSessionEnd \
    $(LOCAL_ORANGE_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/SprdStk/addons/StkSetupCall \
    $(LOCAL_ORANGE_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/Dialer/InCallUI/addons/explicitCallTransfer \
    $(LOCAL_ORANGE_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/services/Telephony/addons/TelephonyOrangePlugin \
    $(LOCAL_ORANGE_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/Settings/addons/OrangeApnPlugin \
    $(LOCAL_ORANGE_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/frameworks/opt/telephony/addons/OrangeDataConnection \
    $(LOCAL_ORANGE_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/frameworks/base/packages/Keyguard/addons/KeyguardSupportOrange \
    $(LOCAL_ORANGE_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/frameworks/opt/telephony/addons/OrangeEccList \
    $(LOCAL_ORANGE_VENDOR_RELATIVE_PATH)vendor/sprd/platform/packages/apps/SprdContacts/plugins/addons/ContactsEFDisplayAddon

orange_plugins_src := $(foreach n,$(orange_plugins),$(n)/src)
orange_plugins_res := $(foreach n,$(orange_plugins), $(wildcard $(LOCAL_PATH)/$(n)/res))

LOCAL_MODULE_TAGS := optional

LOCAL_CERTIFICATE := platform

LOCAL_PACKAGE_NAME := OrangePlugins

LOCAL_JAVA_LIBRARIES += radio_interactor_common telephony-common sprd-support-addon

LOCAL_RESOURCE_DIR := $(orange_plugins_res)

LOCAL_OVERRIDES_PACKAGES := \
    StkRefreshNoToast \
    StkSessionEnd \
    StkSetupCall \
    ExplicitCallTransferPlugin \
    TelephonyOrangePlugin \
    OrangeApnPlugin \
    OrangeDataConnection \
    KeyguardSupportOrange \
    OrangeEccList \
    ContactsEFDisplayAddon

LOCAL_SRC_FILES := $(call all-java-files-under, $(orange_plugins_src))

LOCAL_DEX_PREOPT := false

$(info "$(LOCAL_SRC_FILES) ============")
$(info "$(LOCAL_RESOURCE_DIR) ============")

LOCAL_APK_LIBRARIES += \
    SprdStk \
    Dialer \
    TeleService \
    Settings \
    SystemUI \
    SprdContacts

LOCAL_AAPT_FLAGS += \
    --auto-add-overlay \
    --extra-packages plugin.sprd.stknotoast \
    --extra-packages plugin.sprd.stksessionend \
    --extra-packages plugin.sprd.stksetupcall \
    --extra-packages com.sprd.incallui.explicitCallTransferPlugin \
    --extra-packages plugin.sprd.telephonyOrangePlugin \
    --extra-packages plugin.sprd.apneditor \
    --extra-packages plugin.sprd.dataconnection \
    --extra-packages plugin.sprd.keyguardTelephonyAddon \
    --extra-packages plugin.sprd.operatorfeatures

LOCAL_PROGUARD_ENABLED := disabled

include $(BUILD_ADDON_PACKAGE)

