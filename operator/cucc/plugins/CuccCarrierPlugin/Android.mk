LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CUCC_VENDOR_RELATIVE_PATH :=

m := $(subst /, ,$(LOCAL_PATH))
m := $(patsubst %,..,$(m))
$(foreach n,$(m),\
   $(eval LOCAL_CUCC_VENDOR_RELATIVE_PATH := ../$(LOCAL_CUCC_VENDOR_RELATIVE_PATH)) \
)
cucc_plugins := \
    $(LOCAL_CUCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/frameworks/opt/telephony/addons/CUCCPolicy \
    $(LOCAL_CUCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/SprdStk/addons/StkCuccOperator \
    $(LOCAL_CUCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/Launcher3/addons/CustomizeAppSort \
    $(LOCAL_CUCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/Dialer/InCallUI/addons/CallerAddress \
    $(LOCAL_CUCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/Dialer/InCallUI/addons/cucc \
    $(LOCAL_CUCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/Launcher3/addons/CustomizeCuccApp \
    $(LOCAL_CUCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/services/Telephony/addons/TeleServiceSupportCUCC \
    $(LOCAL_CUCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/Settings/addons/CUCCApnPlugin \
    $(LOCAL_CUCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/frameworks/base/packages/Keyguard/addons/KeyguardSupportCUCC \
    $(LOCAL_CUCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/SprdBrowser/addons/CUCCAddon \
    $(LOCAL_CUCC_VENDOR_RELATIVE_PATH)vendor/sprd/operator/cucc/plugins/CuccUsimNotifier \
    $(LOCAL_CMCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/services/Telecomm/addon/LogRejectedCalls

cucc_plugins_src := $(foreach n,$(cucc_plugins),$(n)/src)
cucc_plugins_res := $(foreach n,$(cucc_plugins), $(wildcard $(LOCAL_PATH)/$(n)/res))

LOCAL_MODULE_TAGS := optional

LOCAL_CERTIFICATE := platform

LOCAL_PACKAGE_NAME := CuccPlugins

LOCAL_JAVA_LIBRARIES += telephony-common radio_interactor_common sprd-support-addon

LOCAL_RESOURCE_DIR := $(cucc_plugins_res)

LOCAL_OVERRIDES_PACKAGES := \
    CUCCPolicy \
    StkCuccOperator \
    SprdCuccAppSortAddon \
    CallerAddressPlugin \
    InCallUICUCCPlugin \
    CustomizeCuccApp \
    TeleServiceSupportCUCC \
    CUCCApnPlugin \
    KeyguardSupportCUCC \
    SprdCuccAddon \
    CuccUsimNotifier \
    LogRejectedCalls

LOCAL_SRC_FILES := $(call all-java-files-under, $(cucc_plugins_src))

LOCAL_DEX_PREOPT := false

LOCAL_APK_LIBRARIES += \
    SprdStk \
    Launcher3 \
    Dialer \
    TeleService \
    Settings \
    SystemUI \
    SprdBrowser \
    Telecom

LOCAL_AAPT_FLAGS += \
    --auto-add-overlay \
    --extra-packages plugin.sprd.operatorpolicy \
    --extra-packages plugin.sprd.stkcuccoperator \
    --extra-packages addon.sprd.launcher3.appsort \
    --extra-packages com.sprd.incallui.callerAddressPlugin \
    --extra-packages com.sprd.incallui.inCallUICuccPlugin \
    --extra-packages addon.sprd.launcher3.cucc \
    --extra-packages plugin.sprd.teleservicesupportoperator \
    --extra-packages plugin.sprd.apneditor \
    --extra-packages plugin.sprd.keyguardTelephonyAddon \
    --extra-packages addon.sprd.browser \
    --extra-packages plugin.sprd.usim \
    --extra-packages com.sprd.telecom.logRejectedCallsPlugin

LOCAL_PROGUARD_ENABLED := disabled

include $(BUILD_ADDON_PACKAGE)

