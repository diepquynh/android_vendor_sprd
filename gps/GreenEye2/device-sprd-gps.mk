PRODUCT_COPY_FILES += \
	vendor/sprd/gps/GreenEye2/gnssmodem.bin:/system/etc/gnssmodem.bin \
	vendor/sprd/gps/GreenEye2/gnssbdmodem.bin:/system/etc/gnssbdmodem.bin \
	vendor/sprd/gps/GreenEye2/gnssfdl.bin:/system/etc/gnssfdl.bin \
	vendor/sprd/gps/GreenEye2/jpleph.405:/system/etc/jpleph.405 \
	vendor/sprd/gps/GreenEye2/spirentroot.cer:/system/etc/spirentroot.cer \
	vendor/sprd/gps/GreenEye2/supl.xml:/system/etc/supl.xml \
	vendor/sprd/gps/GreenEye2/config.xml:/system/etc/config.xml \
	vendor/sprd/gps/GreenEye2/lib32/gps.default.so:/system/lib/hw/gps.default.so \
	vendor/sprd/gps/GreenEye2/lib32/libsupl.so:/system/lib/libsupl.so \
	vendor/sprd/gps/GreenEye2/lib32/liblcsagent.so:/system/lib/liblcsagent.so \
	vendor/sprd/gps/GreenEye2/lib32/liblcscp.so:/system/lib/liblcscp.so \
	vendor/sprd/gps/GreenEye2/lib32/liblcswbxml2.so:/system/lib/liblcswbxml2.so \
	vendor/sprd/gps/GreenEye2/lib32/liblte.so:/system/lib/liblte.so

ifeq ($(SPRD_GNSS_ARCH), arm64)
PRODUCT_COPY_FILES += \
	vendor/sprd/gps/GreenEye2/lib64/gps.default.so:/system/lib64/hw/gps.default.so \
	vendor/sprd/gps/GreenEye2/lib64/libsupl.so:/system/lib64/libsupl.so \
	vendor/sprd/gps/GreenEye2/lib64/liblcsagent.so:/system/lib64/liblcsagent.so \
	vendor/sprd/gps/GreenEye2/lib64/liblcscp.so:/system/lib64/liblcscp.so \
	vendor/sprd/gps/GreenEye2/lib64/liblcswbxml2.so:/system/lib64/liblcswbxml2.so \
	vendor/sprd/gps/GreenEye2/lib64/liblte.so:/system/lib64/liblte.so
endif
