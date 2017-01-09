#here is some commmon things for all spec*.mk
PRODUCT_PACKAGES += \
    OpManager \
    Provision2

PRODUCT_PROPERTY_OVERRIDES += \
	ro.usb.support_cta=true \
        ro.operator=cmcc
