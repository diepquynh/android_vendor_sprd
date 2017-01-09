# Spreadtrum
# Currently, specs are different with their build packages.

include vendor/sprd/operator/cmcc/configs/spec_common.mk

# Prebuilt apps of 3rd-party
PRODUCT_THIRDPARTY_PACKAGES += \
    MobileOffice_CMCC \
    IFlyIME_CMCC \
    BaiduSearch_CMCC \
    JinRiTouTiao_CMCC \
    ShuQiXiaoShuo_CMCC \
    360browser_CMCC \
    TengXunShiPin_CMCC

# vendor(CMCC) apks
PRODUCT_VENDOR_PACKAGES += \
    miguMusic_CMCC \
    miguRead_CMCC \
    CMCCMM \
    AndCloud_CMCC \
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
	ro.operator.version=spec3

