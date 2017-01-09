# The Spreadtrum Communications Inc. 2015

# This is the top of this features, this feature may
# contains much sub-features, and they may divided
# and take effects by properties / addons / overlays or
# prebuilts

# You can gather them by build vars as follows:

# properties -> FEATURES.PRODUCT_PROPERTY_OVERRIDES +=
# addons/packages/prebuilts -> FEATURES.PRODUCT_PACKAGES +=
# overlay -> FEATURES.PRODUCT_PACKAGE_OVERLAYS +=
#

# Now, let's get it on as follows


FEATURES.PRODUCT_PROPERTY_OVERRIDES += \
    ro.operator=cmcc \
    sys.data.ipv6.on=0 \
    ro.device.support.sprd_geocode=1

$(warning The property ** ro.operator=cmcc ** will be removed recently)

FEATURES.PRODUCT_PACKAGE_OVERLAYS += \
    vendor/sprd/operator/cmcc/specA/overlay \
    vendor/sprd/open-source/features/cmcc/overlay

FEATURES.PRODUCT_PACKAGES += \
    MmsBlackListAddon \
    TelephonyPlugin_CMCC \
    SystemUISupportCMCC \
    KeyguardOperatorCMCC \
    WifiSettingsAddon \
    SprdCmccBookmarkAddon \
    ContactsBlackListAddon \
    PolicySupportCMCC \
    SprdCmccUserAgentAddon \
    CallFireWall \
    SprdAppBackup \
    SystemuiUtil \
    InCallUIAddon \
    DialerAddon \
    SignalStrengthUtils \
    AddonDeskClockTimezone \
    LogRejectedCalls \
    LogEmergencyCalls \
    UpdateAdnUidSupportCMCC \
    VideoCmcc \
    CallerAddressPlugin \
    AddonDeskClockStreamMedia \
    CustomizeCmccApp\
    TeleServiceSupportCMCC \
    GeneralSecure \
    UnreadInfo \
    tsiptableserver \
    ManageApplicationsCMCC \
    AddMusicForCMCCPlugin \
    MergeCallButton \
    AddCameraForCMCCPlugin \
    SettingsProviderCmcc \
    SendSmsButtonPlugin \
    SmilPlayer

FEATURES.PRODUCT_PACKAGES += \
    CallFireWall \
    CallFirewallPlugin_sms \
    CallFirewallPlugin_phone \
    DialerBlackListAddon

# Open-source Open Mobile API components for cmcc requirement
FEATURES.PRODUCT_PACKAGES += \
    SmartcardService \
    UiccTerminal \
    org.simalliance.openmobileapi \
    org.simalliance.openmobileapi.xml

FEATURES.PRODUCT_PROPERTY_OVERRIDES += \
    persist.radio.ssda.mode=csfb \
    persist.radio.ssda.testmode=9 \
    persist.radio.ssda.testmode1=10 \
    ro.browser.enginecust=cmcc \
    ro.browser.homepagecust=cmcc

DEFAULT_RUNTIME_PERMISSION_FILES := vendor/sprd/operator/cmcc/permission/sprd-app-perms.xml
ifneq ($(wildcard $(DEFAULT_RUNTIME_PERMISSION_FILES)),)
FEATURES.PRODUCT_COPY_FILES += \
    $(DEFAULT_RUNTIME_PERMISSION_FILES):system/etc/sprd-app-perms.xml
endif
