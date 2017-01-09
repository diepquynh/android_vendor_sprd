#BRCM 47522
PRODUCT_COPY_FILES += \
	vendor/sprd/modules/gps/bcm47520/gps_sprd:system/bin/gpsd \
	vendor/sprd/modules/gps/bcm47520/gps_sprd.so:system/lib/hw/gps.default.so \
	vendor/sprd/modules/gps/bcm47520/gps.sc7730.xml:system/etc/gps.xml
