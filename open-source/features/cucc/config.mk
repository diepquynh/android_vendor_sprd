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

FEATURES.PRODUCT_PACKAGE_OVERLAYS += \
    vendor/sprd/operator/cucc/specA/overlay

FEATURES.PRODUCT_PACKAGES += \
         MmsPriorityAddon \
         SystemUISupportCUCC \
         SimSupportCUCC \
         PolicySupportCUCC \
         SprdCuccBookmarkAddon \
         SprdCuccUserAgentAddon \
         SignalStrengthUtils \
         TeleServiceSupportCUCC \
         KeyguardOperatorCUCC \
         SprdCuccAppSortAddon \
         UsimNotifier \
         CallFireWall \
         CallFirewallPlugin_phone \
         CallFirewallPlugin_sms \
         DialerBlackListAddon \
         CallerAddressPlugin \
         CustomizeCuccApp \
         StkCuccOperator \
         InCallUIHdVoiceCUCC \
         ApnUtilsPlugin

# Display the geocode of the caller, for cucc currently
FEATURES.PRODUCT_PROPERTY_OVERRIDES += \
    ro.device.support.sprd_geocode=1 \
    ro.wifi.support_cucc=true \
    ro.browser.enginecust=cucc \
    ro.browser.homepagecust=cucc \
    ro.stk.support_cucc=true

