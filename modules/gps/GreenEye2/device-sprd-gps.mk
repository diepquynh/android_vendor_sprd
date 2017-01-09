TARGET_VARIANTS := x86 x86_64
ifeq ($(TARGET_ARCH), $(filter $(TARGET_ARCH),$(TARGET_VARIANTS)))
PRODUCT_COPY_FILES += \
	vendor/sprd/modules/gps/GreenEye2/gnssmodem.bin:/system/etc/gnssmodem.bin \
	vendor/sprd/modules/gps/GreenEye2/gnssbdmodem.bin:/system/etc/gnssbdmodem.bin \
	vendor/sprd/modules/gps/GreenEye2/gnssfdl.bin:/system/etc/gnssfdl.bin \
	vendor/sprd/modules/gps/GreenEye2/spirentroot.cer:/system/etc/spirentroot.cer \
	vendor/sprd/modules/gps/GreenEye2/supl.xml:/system/etc/supl.xml \
	vendor/sprd/modules/gps/GreenEye2/config.xml:/system/etc/config.xml \
	vendor/sprd/modules/gps/GreenEye2/lib32_x86/gps.default.so:/system/lib/hw/gps.default.so \
    vendor/sprd/modules/gps/GreenEye2/lib32_x86/liblte.so:/system/lib/liblte.so \
	vendor/sprd/modules/gps/GreenEye2/lib32_x86/libsupl.so:/system/lib/libsupl.so \
	vendor/sprd/modules/gps/GreenEye2/lib32_x86/liblcsagent.so:/system/lib/liblcsagent.so \
	vendor/sprd/modules/gps/GreenEye2/lib32_x86/liblcscp.so:/system/lib/liblcscp.so \
	vendor/sprd/modules/gps/GreenEye2/lib32_x86/liblcswbxml2.so:/system/lib/liblcswbxml2.so \
    vendor/sprd/modules/gps/GreenEye2/lib32_x86/liblcsmgt.so:/system/lib/liblcsmgt.so \
	vendor/sprd/modules/gps/GreenEye2/lib64_x86/gps.default.so:/system/lib64/hw/gps.default.so \
    vendor/sprd/modules/gps/GreenEye2/lib64_x86/liblte.so:/system/lib64/liblte.so \
    vendor/sprd/modules/gps/GreenEye2/lib64_x86/libsupl.so:/system/lib64/libsupl.so \
	vendor/sprd/modules/gps/GreenEye2/lib64_x86/liblcsagent.so:/system/lib64/liblcsagent.so \
	vendor/sprd/modules/gps/GreenEye2/lib64_x86/liblcscp.so:/system/lib64/liblcscp.so \
	vendor/sprd/modules/gps/GreenEye2/lib64_x86/liblcswbxml2.so:/system/lib64/liblcswbxml2.so \
	vendor/sprd/modules/gps/GreenEye2/lib64_x86/liblcsmgt.so:/system/lib64/liblcsmgt.so 

ifeq ($(strip $(TARGET_ARCH)),x86_64)
 PRODUCT_COPY_FILES += \
        vendor/sprd/modules/gps/GreenEye2/lib64_x86/gpsd:/system/bin/gpsd
else 
        vendor/sprd/modules/gps/GreenEye2/lib32_x86/gpsd:/system/bin/gpsd
endif

else #arm arch 
PRODUCT_COPY_FILES += \
	vendor/sprd/modules/gps/GreenEye2/gnssmodem.bin:/system/etc/gnssmodem.bin \
	vendor/sprd/modules/gps/GreenEye2/gnssbdmodem.bin:/system/etc/gnssbdmodem.bin \
	vendor/sprd/modules/gps/GreenEye2/gnssfdl.bin:/system/etc/gnssfdl.bin \
	vendor/sprd/modules/gps/GreenEye2/spirentroot.cer:/system/etc/spirentroot.cer \
	vendor/sprd/modules/gps/GreenEye2/supl.xml:/system/etc/supl.xml \
	vendor/sprd/modules/gps/GreenEye2/config.xml:/system/etc/config.xml \
	vendor/sprd/modules/gps/GreenEye2/lib32/gps.default.so:/system/lib/hw/gps.default.so \
	vendor/sprd/modules/gps/GreenEye2/lib32/libsupl.so:/system/lib/libsupl.so \
	vendor/sprd/modules/gps/GreenEye2/lib32/liblcsagent.so:/system/lib/liblcsagent.so \
	vendor/sprd/modules/gps/GreenEye2/lib32/liblcscp.so:/system/lib/liblcscp.so \
	vendor/sprd/modules/gps/GreenEye2/lib32/liblcswbxml2.so:/system/lib/liblcswbxml2.so \
    vendor/sprd/modules/gps/GreenEye2/lib32/liblcsmgt.so:/system/lib/liblcsmgt.so \
	vendor/sprd/modules/gps/GreenEye2/lib32/liblte.so:/system/lib/liblte.so \
	vendor/sprd/modules/gps/GreenEye2/lib32/gpsd:/system/bin/gpsd

ifeq ($(SPRD_GNSS_ARCH), arm64)
PRODUCT_COPY_FILES += \
	vendor/sprd/modules/gps/GreenEye2/lib64/gps.default.so:/system/lib64/hw/gps.default.so \
	vendor/sprd/modules/gps/GreenEye2/lib64/libsupl.so:/system/lib64/libsupl.so \
	vendor/sprd/modules/gps/GreenEye2/lib64/liblcsagent.so:/system/lib64/liblcsagent.so \
	vendor/sprd/modules/gps/GreenEye2/lib64/liblcscp.so:/system/lib64/liblcscp.so \
	vendor/sprd/modules/gps/GreenEye2/lib64/liblcswbxml2.so:/system/lib64/liblcswbxml2.so \
	vendor/sprd/modules/gps/GreenEye2/lib64/liblte.so:/system/lib64/liblte.so \
	vendor/sprd/modules/gps/GreenEye2/lib64/liblcsmgt.so:/system/lib64/liblcsmgt.so \
	vendor/sprd/modules/gps/GreenEye2/lib64/gpsd:/system/bin/gpsd
endif #arm64

endif
