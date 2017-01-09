LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CMCC_VENDOR_RELATIVE_PATH :=

m := $(subst /, ,$(LOCAL_PATH))
m := $(patsubst %,..,$(m))
$(foreach n,$(m),\
   $(eval LOCAL_CMCC_VENDOR_RELATIVE_PATH := ../$(LOCAL_CMCC_VENDOR_RELATIVE_PATH)) \
)
cmcc_plugins := \
    $(LOCAL_CMCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/Dialer/InCallUI/addons/cmcc \
    $(LOCAL_CMCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/frameworks/opt/telephony/addons/CMCCPolicy \
    $(LOCAL_CMCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/Launcher3/addons/UnreadInfo \
    $(LOCAL_CMCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/Dialer/InCallUI/addons/CallerAddress \
    $(LOCAL_CMCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/Launcher3/addons/CustomizeCmccApp \
    $(LOCAL_CMCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/Dialer/addons/cmcc \
    $(LOCAL_CMCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/services/Telephony/addons/ShowCallingNumberPlugin \
    $(LOCAL_CMCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/services/Telecomm/addon/LogEmergencyNumbers \
    $(LOCAL_CMCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/Settings/addons/WifiSettingsAddon \
    $(LOCAL_CMCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/DeskClock/DeskClockStreamMediaAddons \
    $(LOCAL_CMCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/Settings/addons/DisableDataUsageForCMCC \
    $(LOCAL_CMCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/DeskClock/TimezoneAddons \
    $(LOCAL_CMCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/CallSettings/addons/cmcc \
    $(LOCAL_CMCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/Settings/addons/GPSPlugin \
    $(LOCAL_CMCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/SprdEmail/addons/JiEmailCmcc \
    $(LOCAL_CMCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/frameworks/base/packages/SettingsProvider/addons/SettingsProviderCmcc \
    $(LOCAL_CMCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/frameworks/base/packages/SystemUI/addons/SystemuiUtil \
    $(LOCAL_CMCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/DreamGallery2/addons/VideoCmcc \
    $(LOCAL_CMCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/SprdBrowser/addons/CMCCAddon \
    $(LOCAL_CMCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/services/Telecomm/addon/LogRejectedCalls\
    $(LOCAL_CMCC_VENDOR_RELATIVE_PATH)vendor/sprd/platform/packages/providers/SprdContactsProvider/plugins/addons/SegmentSearchSupportAddon \
    $(LOCAL_CMCC_VENDOR_RELATIVE_PATH)vendor/sprd/platform/packages/apps/SprdContacts/plugins/addons/SegmentSearchAddon \
    $(LOCAL_CMCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/Settings/addons/SettingsOperatorPlugin \
    $(LOCAL_CMCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/services/Telephony/addons/TeleServiceSupportCMCC \
    $(LOCAL_CMCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/frameworks/opt/telephony/addons/CMCCTelephonyPlugin

cmcc_plugins_src := $(foreach n,$(cmcc_plugins),$(n)/src)
cmcc_plugins_res := $(foreach n,$(cmcc_plugins), $(wildcard $(LOCAL_PATH)/$(n)/res))

LOCAL_MODULE_TAGS := optional

LOCAL_CERTIFICATE := platform

LOCAL_PACKAGE_NAME := CmccPlugins

LOCAL_JAVA_LIBRARIES += telephony-common radio_interactor_common sprd-support-addon

LOCAL_RESOURCE_DIR := $(cmcc_plugins_res)

LOCAL_OVERRIDES_PACKAGES := \
    InCallUICMCCPlugin \
    CMCCPolicy \
    UnreadInfo \
    CallerAddressPlugin \
    CustomizeCmccApp \
    DialerCMCCPlugin \
    ShowCallingNumberPlugin \
    LogEmergencyNumbers \
    WifiSettingsAddon \
    AddonDeskClockStreamMedia \
    DisableDataUsage \
    AddonDeskClockTimezone \
    CallSettingsCMCCPlugin \
    GPSPlugin \
    SprdJiEmailCMCCAddon \
    SettingsProviderCmcc \
    SystemuiUtil \
    VideoCmcc \
    SprdCmccAddon \
    LogRejectedCalls \
    SegmentSearchSupportAddon \
    SegmentSearchAddon \
    SettingsOperatorPlugin \
    TeleServiceSupportCMCC \
    CMCCTelephonyPlugin

LOCAL_SRC_FILES := $(call all-java-files-under, $(cmcc_plugins_src))

LOCAL_SRC_FILES += $(LOCAL_CMCC_VENDOR_RELATIVE_PATH)vendor/sprd/plugins/packages/apps/Settings/addons/WifiSettingsAddon/src/plugin/sprd/supportcmcc/IWifiSettings.aidl

LOCAL_DEX_PREOPT := false

LOCAL_APK_LIBRARIES += \
    Dialer \
    Telecom \
    ims \
    Launcher3 \
    Settings \
    TeleService \
    DeskClock \
    CallSettings \
    Email2 \
    SettingsProvider \
    SystemUI \
    DreamGallery2 \
    SprdBrowser \
    SprdContactsProvider \
    SprdContacts

LOCAL_AAPT_FLAGS += \
    --auto-add-overlay \
    --extra-packages com.sprd.incallui.inCallUICmccPlugin \
    --extra-packages plugin.sprd.operatorpolicy \
    --extra-packages plugins.sprd.unreadinfo \
    --extra-packages com.sprd.incallui.callerAddressPlugin \
    --extra-packages addon.sprd.launcher3.cmcc \
    --extra-packages com.sprd.dialer.dialerCmccPlugin \
    --extra-packages plugin.sprd.showcallingnumber \
    --extra-packages com.sprd.telecom.logEmergencyNumberPlugin \
    --extra-packages plugin.sprd.supportcmcc \
    --extra-packages addon.sprd.AddonDeskClockStreamMedia \
    --extra-packages plugin.sprd.disabledatausage \
    --extra-packages addon.sprd.AddonDeskClock \
    --extra-packages com.sprd.callsettings.callsettingscmccplugin \
    --extra-packages plugin.sprd.location \
    --extra-packages plugin.sprd.jiemail \
    --extra-packages addon.sprd.providers.settings.cmcc \
    --extra-packages plugin.sprd.systemuiaddon \
    --extra-packages com.sprd.cmccvideoplugin \
    --extra-packages addon.sprd.browser \
    --extra-packages com.sprd.telecom.logRejectedCallsPlugin \
    --extra-packages com.sprd.settings \
    --extra-packages plugin.sprd.teleservicesupportoperator \
    --extra-packages plugin.sprd.telephony.uicc

LOCAL_PROGUARD_ENABLED := disabled

include $(BUILD_ADDON_PACKAGE)

