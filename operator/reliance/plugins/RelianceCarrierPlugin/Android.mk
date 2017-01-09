LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_RELIANCE_VENDOR_RELATIVE_PATH :=

m := $(subst /, ,$(LOCAL_PATH))
m := $(patsubst %,..,$(m))
$(foreach n,$(m),\
   $(eval LOCAL_RELIANCE_VENDOR_RELATIVE_PATH := ../$(LOCAL_RELIANCE_VENDOR_RELATIVE_PATH)) \
)
reliance_plugins := \
    $(LOCAL_RELIANCE_VENDOR_RELATIVE_PATH)vendor/sprd/operator/reliance/plugins/LowBatteryUtils \
    $(LOCAL_RELIANCE_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/Settings/addons/SettingsReliancePlugin \
    $(LOCAL_RELIANCE_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/CallSettings/addons/reliance \
    $(LOCAL_RELIANCE_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/Dialer/addons/reliance \
    $(LOCAL_RELIANCE_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/frameworks/opt/telephony/addons/ReliancePolicy

reliance_plugins_src := $(foreach n,$(reliance_plugins),$(n)/src)
reliance_plugins_res := $(foreach n,$(reliance_plugins), $(wildcard $(LOCAL_PATH)/$(n)/res))

LOCAL_MODULE_TAGS := optional

LOCAL_CERTIFICATE := platform

LOCAL_PACKAGE_NAME := ReliancePlugins

LOCAL_JAVA_LIBRARIES += telephony-common sprd-framework

LOCAL_RESOURCE_DIR := $(reliance_plugins_res)

LOCAL_OVERRIDES_PACKAGES := \
    LowBatteryCallUtils_plugin \
    SettingsReliancePlugin \
    CallSettingsReliancePlugin \
    DialerReliancePlugin \
    ReliancePolicy

LOCAL_SRC_FILES := $(call all-java-files-under, $(reliance_plugins_src))

LOCAL_DEX_PREOPT := false

LOCAL_APK_LIBRARIES += \
    Settings \
    CallSettings \
    Dialer \
    SprdContacts

LOCAL_AAPT_FLAGS += \
    --auto-add-overlay \
    --extra-packages plugin.sprd.CallUtilsPlugin \
    --extra-packages com.sprd.settings \
    --extra-packages com.sprd.callsettings.callsettingsrelianceplugin\
    --extra-packages com.sprd.dialer \
    --extra-packages plugin.sprd.operatorpolicy \

LOCAL_PROGUARD_ENABLED := disabled

include $(BUILD_ADDON_PACKAGE)

