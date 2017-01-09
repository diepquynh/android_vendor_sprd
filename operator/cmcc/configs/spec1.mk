# Spreadtrum
# Currently, specs are different with their build packages.

include vendor/sprd/operator/cmcc/configs/spec_common.mk

# Prebuilt apps of 3rd-party
PRODUCT_THIRDPARTY_PACKAGES += \
    BaiduSearch_CMCC \
    IFlyIME_CMCC \
    PeopleDaily_CMCC \
    TengXunShiPin_CMCC \
    MobileOffice_CMCC

# vendor(CMCC) apks
PRODUCT_VENDOR_PACKAGES += \
    CMCCFetion \
    AndLife_CMCC \
    139Mail_CMCC \
    miguVideo_CMCC \
    miguGame_CMCC \
    CMCCNavigation \
    HeNews_CMCC \
    jiEmail_for139_CMCC \
    popular_online_release_CMCC \
    Zhangting_CMCC

# Add the vendor(CMCC) apks into build list
ifneq ($(strip $(TARGET_DISABLE_VENDOR_PRELOADAPP)),true)
PRODUCT_PACKAGES += $(PRODUCT_VENDOR_PACKAGES)
endif

# Add the 3rd-party apks into build list
ifneq ($(strip $(TARGET_DISABLE_THIRDPATY_PRELOADAPP)),true)
PRODUCT_PACKAGES += $(PRODUCT_THIRDPARTY_PACKAGES)
endif


PRODUCT_PROPERTY_OVERRIDES += \
	ro.operator.version=spec1

