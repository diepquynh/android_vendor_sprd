LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_TELCEL_VENDOR_RELATIVE_PATH :=

m := $(subst /, ,$(LOCAL_PATH))
m := $(patsubst %,..,$(m))
$(foreach n,$(m),\
   $(eval LOCAL_TELCEL_VENDOR_RELATIVE_PATH := ../$(LOCAL_TELCEL_VENDOR_RELATIVE_PATH)) \
)
telcel_plugins := \
    $(LOCAL_TELCEL_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/SprdStk/addons/StkTelcelOperator \
    $(LOCAL_TELCEL_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/services/Telephony/addons/telcel \
    $(LOCAL_TELCEL_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/Dialer/InCallUI/addons/telcel \
    $(LOCAL_TELCEL_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/Dialer/addons/telcel \
    $(LOCAL_TELCEL_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/frameworks/opt/telephony/addons/TelcelDataConnection \
    $(LOCAL_TELCEL_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/frameworks/opt/telephony/addons/TelcelTelephonyPlugin \
    $(LOCAL_TELCEL_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/services/Telecomm/addon/CallWaitingTone \
    $(LOCAL_TELCEL_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/services/Telephony/addons/DTMFToneSupport

telcel_plugins_src := $(foreach n,$(telcel_plugins),$(n)/src)
telcel_plugins_res := $(foreach n,$(telcel_plugins), $(wildcard $(LOCAL_PATH)/$(n)/res))

LOCAL_MODULE_TAGS := optional

LOCAL_CERTIFICATE := platform

LOCAL_PACKAGE_NAME := TelcelPlugins

LOCAL_JAVA_LIBRARIES += telephony-common radio_interactor_common

LOCAL_RESOURCE_DIR := $(telcel_plugins_res)

LOCAL_OVERRIDES_PACKAGES := \
    StkTelcelOperator \
    TelcelVoicemail \
    InCallUITelcelPlugin \
    DialerTelcelPlugin \
    TelcelDataConnection \
    MultiPartCallPlugin \
    TelcelTelephonyPlugin \
    DTMFToneSupport \
    CallWaitingTonePlugin

LOCAL_SRC_FILES := $(call all-java-files-under, $(telcel_plugins_src))

LOCAL_DEX_PREOPT := false

LOCAL_APK_LIBRARIES += \
    SprdStk \
    Dialer \
    TeleService \
    Telecom

LOCAL_AAPT_FLAGS += \
    --auto-add-overlay \
    --extra-packages plugin.sprd.stktelceloperator \
    --extra-packages com.sprd.telephonyPlugin \
    --extra-packages com.sprd.incallui.inCallUITelcelPlugin \
    --extra-packages com.sprd.dialer \
    --extra-packages plugin.sprd.dataconnection \
    --extra-packages plugin.sprd.callwaitingtone \
    --extra-packages plugin.sprd.dtmftoneSupport

LOCAL_PROGUARD_ENABLED := disabled

include $(BUILD_ADDON_PACKAGE)

