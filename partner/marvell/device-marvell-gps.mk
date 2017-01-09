PRODUCT_COPY_FILES += \
	vendor/sprd/partner/marvell/L2000/gps.default.so:system/lib/hw/gps.default.so \
	vendor/sprd/partner/marvell/L2000/libagps_hal.so:system/lib/libagps_hal.so \
	vendor/sprd/partner/marvell/L2000/rom.bin:system/etc/rom.bin \
	vendor/sprd/partner/marvell/L2000/config/pxa_testcfg.ini:system/etc/pxa_testcfg.ini \
	vendor/sprd/partner/marvell/L2000/config/mrvl_agps_default.conf:system/etc/mrvl_agps_default.conf \
	vendor/sprd/partner/marvell/L2000/AGPS_CA.pem:system/etc/AGPS_CA.pem \
	vendor/sprd/partner/marvell/L2000/RXNdata/license.key:system/etc/license.key \
	vendor/sprd/partner/marvell/L2000/RXNdata/security.key:system/etc/security.key \
	vendor/sprd/partner/marvell/L2000/RXNdata/MSLConfig.txt:system/etc/MSLConfig.txt \
	vendor/sprd/partner/marvell/L2000/RXNdata/rxn_config_data:system/etc/rxn_config_data

ifeq (uart2,$(PRODUCT_MARVELL_GPS_UART))
PRODUCT_COPY_FILES += \
	vendor/sprd/partner/marvell/L2000/config/mrvl_gps_platform.conf:system/etc/mrvl_gps_platform.conf
endif

ifeq (uart3,$(PRODUCT_MARVELL_GPS_UART))
PRODUCT_COPY_FILES += \
	vendor/sprd/partner/marvell/L2000/config/mrvl_gps_platform_uart3.conf:system/etc/mrvl_gps_platform.conf
endif
