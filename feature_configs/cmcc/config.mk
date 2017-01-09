# The Spreadtrum Communications Inc. 2015

# This is the top of these features, this feature may
# contain many sub-features, and they may be divided
# and take effects by properties / addons / overlays or
# prebuilts

# You can gather them by build vars as follows:

# properties -> FEATURES.PRODUCT_PROPERTY_OVERRIDES +=
# addons/packages/prebuilts -> FEATURES.PRODUCT_PACKAGES +=
# overlay -> FEATURES.PRODUCT_PACKAGE_OVERLAYS +=
#

# Now, let's get it on as follows

FEATURES.PRODUCT_PACKAGE_OVERLAYS += \
    vendor/sprd/feature_configs/cmcc/overlay

FEATURES.PRODUCT_PROPERTY_OVERRIDES += \
    ro.messaging.operator=cmcc

#Plugins of the same operator had been integrated into a single application for easy management.
#So please do not modify this configuration any more.
#Instead, you should read the Operator Plug-in Configuration Manual carefully before you plan to add an operator plugin.
FEATURES.PRODUCT_PACKAGES += \
    SmartcardService \
    UiccTerminal \
    org.simalliance.openmobileapi \
    org.simalliance.openmobileapi.xml \
    SprdAppBackup \
    ManageApplicationsCMCC \
    10086cn \
    MyFavorites \
    SprdGeneralSecurity \
    CmccPlugins

DEFAULT_RUNTIME_PERMISSION_FILES := vendor/sprd/operator/cmcc/configs/sprd-app-perms.xml
ifneq ($(wildcard $(DEFAULT_RUNTIME_PERMISSION_FILES)),)
FEATURES.PRODUCT_COPY_FILES += \
    $(DEFAULT_RUNTIME_PERMISSION_FILES):system/etc/sprd-app-perms.xml
endif
