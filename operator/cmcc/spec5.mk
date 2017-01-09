# Spreadtrum
# Currently, specs are different with their build packages.

include vendor/sprd/operator/cmcc/spec_common.mk

# Prebuilt apps of 3rd-party
PRODUCT_THIRDPARTY_PACKAGES += \
    BaiduSearch_CMCC \
    IFlyIME_CMCC \
    JinRiTouTiao_CMCC \
    MobileOffice_CMCC

# vendor(CMCC) apks
PRODUCT_VENDOR_PACKAGES += \
    CMCCNavigation \
    CMCCGameHall \
    AndCloud_CMCC \
    miguMusic_CMCC  \
    CMCCFetion \
    miguRead_CMCC \
    CMCCMM \
    139Mail_CMCC \
    AndLife_CMCC \
    CMCCFetion \
    IFlyIME_CMCC

# Add the vendor(CMCC) apks into build list
ifneq ($(strip $(TARGET_DISABLE_VENDOR_PRELOADAPP)),true)
PRODUCT_PACKAGES += $(PRODUCT_VENDOR_PACKAGES)
endif

# Add the 3rd-party apks into build list
ifneq ($(strip $(TARGET_DISABLE_THIRDPATY_PRELOADAPP)),true)
PRODUCT_PACKAGES += $(PRODUCT_THIRDPARTY_PACKAGES)
endif

PRODUCT_PROPERTY_OVERRIDES += \
	ro.operator.version=spec5

