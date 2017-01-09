# CMCC SpecA
# $(call inherit-product, vendor/sprd/operator/cmcc/specA.mk)
# CMCC Prebuild packages
# Attention the packages name is treat as LOCAL_MODULE when building,
# the items should be unique.
PRODUCT_PACKAGE_OVERLAYS += \
 vendor/sprd/operator/cmcc/specA/overlay

#PRODUCT_PACKAGES += \
#    SprdDM \
#    libsprddm \

# packages files
PRODUCT_PACKAGES += \
	ForOPCModuleTest

# own copyright packages files
PRODUCT_PACKAGES += \
    10086cn \
    Monternet \
    MyFavorites \
    CallFireWall \
    SprdAppBackup

################################################################ REMOVE NEEDED
# Prebuilt apps of 3rd-party
PRODUCT_THIRDPARTY_PACKAGES += \
    MobileOffice_CMCC

# vendor(CMCC) apks
PRODUCT_VENDOR_PACKAGES += \
    CMCCGameHall \
    CMCCFetion \
    CMCCMM \
    CMCCMM_Plugin_Cartoon \
    CMCCMM_Plugin_Comic \
    CMCCMM_Plugin_Music \
    CMCCMM_Plugin_Video \
    miguRead_CMCC \
    CMCCWifi \
    CMCCNavigation \
    IFlyIME_CMCC

# Add the vendor(CMCC) apks into build list
ifneq ($(strip $(TARGET_DISABLE_VENDOR_PRELOADAPP)),true)
PRODUCT_PACKAGES += $(PRODUCT_VENDOR_PACKAGES)
endif

# Add the 3rd-party apks into build list
ifneq ($(strip $(TARGET_DISABLE_THIRDPATY_PRELOADAPP)),true)
PRODUCT_PACKAGES += $(PRODUCT_THIRDPARTY_PACKAGES)
endif

# add  system properties
PRODUCT_PROPERTY_OVERRIDES += \
	ro.operator=cmcc \
	ro.operator.version=specA

#$(call inherit-product, device/sprd/common/res/boot/boot_res.mk)
#$(call inherit-product, vendor/sprd/UniverseUI/universeui.mk)
