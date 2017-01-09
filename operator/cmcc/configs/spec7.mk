
include vendor/sprd/operator/cmcc/configs/spec_common.mk

# Prebuilt apps of 3rd-party
PRODUCT_THIRDPARTY_PACKAGES += \
    wangyi_news_CMCC \
    BaiduSearch_CMCC \
    MobileOffice_CMCC \
    IFlyIME_CMCC \
    Ctrip_CMCC \
    Meituan_CMCC \
    360browser_CMCC

# vendor(CMCC) apks
PRODUCT_VENDOR_PACKAGES += \
    CMCCMM \
    CMCCMM_Plugin_SafeCente \
    CMCCNavigation \
    miguMusic_CMCC \
    miguRead_CMCC \
    Mobile_business_mail_CMCC \
    MobileVallet_CMCC \
    Lingxi_CMCC \
    jiEmail_for139_CMCC \
    popular1.1_online_release_CMCC


# Add the vendor(CMCC) apks into build list
ifneq ($(strip $(TARGET_DISABLE_VENDOR_PRELOADAPP)),true)
PRODUCT_PACKAGES += $(PRODUCT_VENDOR_PACKAGES)
endif

# Add the 3rd-party apks into build list
ifneq ($(strip $(TARGET_DISABLE_THIRDPATY_PRELOADAPP)),true)
PRODUCT_PACKAGES += $(PRODUCT_THIRDPARTY_PACKAGES)
endif

PRODUCT_PROPERTY_OVERRIDES += \
        ro.operator.version=spec7
