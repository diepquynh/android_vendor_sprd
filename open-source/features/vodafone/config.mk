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
    NetWorkSupport3GOnly2GOnly \
    SupportApnNull \
    ExplicitCallTransferPlugin \
    StkSetupMenu \
    ApnEditablePlugin \
    FdnService

# Fdn Control , 0: stand for mms and short messaging isn't need to be filter , 1: need to flilter
FEATURES.PRODUCT_PROPERTY_OVERRIDES += \
    ro.messag.fdnfilter=0 \
