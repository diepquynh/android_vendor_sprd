
LOCAL_PATH:= vendor/sprd/open-source/res/productinfo

PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/j3_3g_connectivity_configure.ini:system/etc/connectivity_configure.ini \
	$(LOCAL_PATH)/j3_3g_connectivity_configure.ini:prodnv/connectivity_configure.ini \
        $(LOCAL_PATH)/j3_3g_connectivity_calibration.ini:system/etc/connectivity_calibration.ini \
        $(LOCAL_PATH)/j3_3g_connectivity_calibration.ini:prodnv/connectivity_calibration.ini

