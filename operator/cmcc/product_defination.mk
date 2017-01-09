ifndef PRODUCT_VENDOR_LEVEL
PRODUCT_VENDOR_LEVEL := 3
endif


if ($(strip $(PRODUCT_VENDOR_LEVEL)),1)
# vendor(CMCC) apks
PRODUCT_VENDOR_PACKAGES +=

endif # 1


if ($(strip $(PRODUCT_VENDOR_LEVEL)),2)
# vendor(CMCC) apks
PRODUCT_VENDOR_PACKAGES +=

endif # 2


if ($(strip $(PRODUCT_VENDOR_LEVEL)),3)
# vendor(CMCC) apks
PRODUCT_VENDOR_PACKAGES +=

endif # 3


if ($(strip $(PRODUCT_VENDOR_LEVEL)),4)
# vendor(CMCC) apks
PRODUCT_VENDOR_PACKAGES +=

endif # 4
