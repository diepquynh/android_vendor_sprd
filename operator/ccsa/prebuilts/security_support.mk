# Security Feature support config
USE_PROJECT_SEC :=true

ifeq ($(USE_PROJECT_SEC),true)
PRODUCT_PROPERTY_OVERRIDES += persist.support.securetest=1 \
                              persist.sys.usb.security_lock=true
# prebuild files
PRODUCT_PACKAGES += \
        Permission \
        framework-se-res.apk \
	ASA \
	RDC \
	PracticalTools \
	choose_secure \
        FileEncrypt

PRODUCT_COPY_FILES += frameworks/base/core/java/com/sprd/telephonesec.db:/system/etc/telephonesec.db
endif
