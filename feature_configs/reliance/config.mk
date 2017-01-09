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

FEATURES.PRODUCT_LOCALES := en_US bn_BD hi_IN te_IN ta_IN ur_PK gu_IN pa_IN ml_IN kn_IN mr_IN

#Plugins of the same operator had been integrated into a single application for easy management.
#So please do not modify this configuration any more.
#Instead, you should read the Operator Plug-in Configuration Manual carefully before you plan to add an operator plugin.
FEATURES.PRODUCT_PACKAGES += \
    LogEmergencyCalls \
    Folderplugins \
    ReliancePlugins

FEATURES.PRODUCT_PACKAGE_OVERLAYS += \
    $(PRODUCT_REVISION_COMMON_CONFIG_PATH)/reliance/overlay
