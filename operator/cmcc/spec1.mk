# Spreadtrum
# Currently, specs are different with their build packages.

# Prebuilt apps of 3rd-party
PRODUCT_THIRDPARTY_PACKAGES += \
    QQBrowser_CMCC \
    QQBuy_CMCC \
    MobileOffice_CMCC

# vendor(CMCC) apks
PRODUCT_VENDOR_PACKAGES += \
    CMCCGameHall \
    miguMusic_CMCC \
    CMCCFetion \
    CMCCMM \
    AndLife_CMCC \
    IFlyIME_CMCC

include vendor/sprd/operator/cmcc/spec_common.mk

PRODUCT_PROPERTY_OVERRIDES += \
	ro.operator.version=spec1

