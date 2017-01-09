# The Spreadtrum Communications Inc. 2015

# This is the top of this features, this feature may
# contains much sub-features, and they may divided
# and take effects by properties / addons / overlays or
# prebuilts

# You can gather them by build vars as follows:

# Now, let's get it on as follows

#Plugins of the same operator had been integrated into a single application for easy management.
#So please do not modify this configuration any more.
#Instead, you should read the Operator Plug-in Configuration Manual carefully before you plan to add an operator plugin.
FEATURES.PRODUCT_PACKAGES += \
    VodafonePlugins

#Vodafone feature for RTSP Streaming
ENABLE_VODAFONE_FEATURE := true

FEATURES.PRODUCT_PACKAGE_OVERLAYS += \
    $(PRODUCT_REVISION_COMMON_CONFIG_PATH)/vodafone/overlay
