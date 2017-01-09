#spreadtrum
# Currently, specs are different with their build packages.

# Prebuilt apps of Jio
PRODUCT_PACKAGES += \
    Net_Velocity \
    Jio_Drive \
    Jio_Setup_Wizard \
    Jio_Settings \
    Jio_Chat \
    Jio_Mags \
    Jio_News_Express \
    Jio_Beat \
    Jio_Vod


PRODUCT_PACKAGE_OVERLAYS += \
 vendor/sprd/operator/reliance/spec/overlay

PRODUCT_PACKAGES += startingguideinterface



