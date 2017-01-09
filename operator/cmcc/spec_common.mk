# CMCC SpecA
# $(call inherit-product, vendor/sprd/operator/cmcc/specA.mk)
# CMCC Prebuild packages
# Attention the packages name is treat as LOCAL_MODULE when building,
# the items should be unique.

# Overlay build files
PRODUCT_PACKAGE_OVERLAYS += \
 vendor/sprd/operator/cmcc/specA/overlay

# Overlay system properties
PRODUCT_PROPERTY_OVERRIDES += \
	ro.operator=cmcc \
	ro.usb.support_cta=true

# Open-source CMCC recommond apks
PRODUCT_PACKAGES += \
    10086cn \
    Monternet \
    MyFavorites \
    CallFireWall \
    SprdAppBackup
# \
#   remove DM follow cmcc requirement
#    SprdDM \
#    libsprddm

# Prebuilt apps of 3rd-party
PRODUCT_THIRDPARTY_PACKAGES += \
#    MobileOffice_CMCC

# vendor(CMCC) apks
PRODUCT_VENDOR_PACKAGES += \
#    CMCCMM \
#    CMCCFetion \
#    AndLife_CMCC \
#    IFlyIME_CMCC

#include vendor/sprd/operator/cmcc/product_defination.mk

# Add the vendor(CMCC) apks into build list
ifneq ($(strip $(TARGET_DISABLE_VENDOR_PRELOADAPP)),true)
PRODUCT_PACKAGES += $(PRODUCT_VENDOR_PACKAGES)
endif

# Add the 3rd-party apks into build list
ifneq ($(strip $(TARGET_DISABLE_THIRDPATY_PRELOADAPP)),true)
PRODUCT_PACKAGES += $(PRODUCT_THIRDPARTY_PACKAGES)
endif

# Remove PinyinIME because cmcc has IFlyIME
PRODUCT_PACKAGES := $(filter-out PinyinIME, $(PRODUCT_PACKAGES))

BOARD_SEPOLICY_DIRS += vendor/sprd/operator/cmcc/sepolicy \
                       vendor/sprd/operator/cmcc/prebuild_release/GeneralSecure/sepolicy

