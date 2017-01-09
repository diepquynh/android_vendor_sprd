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
    ril.sim.phone_ex.start=true

FEATURES.PRODUCT_PACKAGE_OVERLAYS += \
    $(PRODUCT_REVISION_COMMON_CONFIG_PATH)/base/overlay

FEATURES.PRODUCT_PACKAGES += \
    SystemUITelephonyAddon \
    PressBrightnessPlugin \
    BuildNumberSettings \
    SupportCPVersionDisplay \
    LimitOwnerInfoLength \
    MultiPartCallPlugin \
    LauncherDrmAddon \
    DynamicCalendarIcon \
    DynamicDeskClockIcon \
    SystemUIAudioProfile \
    SystemClockPlugin \
    SimLockSupport \
    KeyguardSupportSimlock \
    KeyguardSupportAmPm \
    FdnService \
    MusicDRMPlugin \
    AddonClearAllPlaylist \
    AddMusicForCMCCPlugin \
    OtaUpdate \
    SupportDefaultAppSettings \
    FileExplorerDRMAddon \
    ApnSettingsPlugin \
    SettingsForDynaNavigationBar \
    SystemUIDynaNavigationBar \
    SystemUILockApp \
    CbCustomSetting \
    SmilPlayer \
    ProtectedApp \
    SprdQuickSearchBox \
    NoteBook \
    CellBroadcastReceiver \
    Omacp
