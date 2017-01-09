# Spreadtrum
# Currently, specs are different with their build packages.

include vendor/sprd/operator/cmcc/spec_common.mk

# Prebuilt apps of 3rd-party
PRODUCT_THIRDPARTY_PACKAGES += \
    MobileOffice_CMCC \
    IFlyIME_CMCC \
    BaiduSearch_CMCC \
    JinRiTouTiao_CMCC \
    youdianyisi_CMCC \
    letv_CMCC \
    baidu_tieba_CMCC

# vendor(CMCC) apks
PRODUCT_VENDOR_PACKAGES += \
    139Mail_CMCC \
    miguRead_CMCC \
    miguMusic_CMCC \
    AndCloud_CMCC \
    Lingxi_CMCC

# Add the vendor(CMCC) apks into build list
ifneq ($(strip $(TARGET_DISABLE_VENDOR_PRELOADAPP)),true)
PRODUCT_PACKAGES += $(PRODUCT_VENDOR_PACKAGES)
endif

# Add the 3rd-party apks into build list
ifneq ($(strip $(TARGET_DISABLE_THIRDPATY_PRELOADAPP)),true)
PRODUCT_PACKAGES += $(PRODUCT_THIRDPARTY_PACKAGES)
endif

PRODUCT_PROPERTY_OVERRIDES += \
	ro.operator.version=spec2

