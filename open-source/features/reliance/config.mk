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

# Please do not modify this configuration easily unless you make sure
# the Plug-in you add is a general one which means it will be used for
# at least two operators. If not, please just add your Plug-in into the
# 'Reliance-plugins' directly!
FEATURES.PRODUCT_PACKAGES += \
    Reliance-plugins \
    LogEmergencyCalls \
    Folderplugins

#Add reliance prebuild apks
include vendor/sprd/operator/reliance/spec.mk
