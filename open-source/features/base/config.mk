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

FEATURES.PRODUCT_PACKAGES += \
    SpecialChars \
    SimLockSupport \
    CallFireWall \
    CallFirewallPlugin_phone \
    CallFirewallPlugin_sms \
    DialerBlackListAddon \
    ContactsBlackListAddon \
    ContactsDefaultContactAddon \
    ApnSettingsPlugin \
    BrowserXposed \
    EmailDrmAddon \
    DocumentsUIXposed \
    MmsDrmAddon \
    LauncherDrmAddon \
    VideoDrm \
    FdnDialSupport \
    GalleryDrm \
    FileExplorerDRMAddon \
    MusicDRMPlugin \
    DownloadProviderDrmAddon \
    MultiPartCallPlugin

FEATURES.PRODUCT_PACKAGE_OVERLAYS += \
    vendor/sprd/open-source/features/base/overlay

FEATURES.PRODUCT_PACKAGE_OVERLAYS += \
    vendor/sprd/operator/operator_res/operatorname_overlay

FEATURES.PRODUCT_PROPERTY_OVERRIDES += \
        persist.netmon.linger=5000
