
LOCAL_PATH:= vendor/sprd/operator/cmcc/configs/res/boot

PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/bootanimation_cmcc_4g_fwvga.zip:system/media/bootanimation.zip \
	$(LOCAL_PATH)/bootsound_4g.mp3:system/media/bootsound.mp3 \
	$(LOCAL_PATH)/shutdownanimation_cmcc_4g_fwvga.zip:system/media/shutdownanimation.zip \
	$(LOCAL_PATH)/shutdownsound_4g.mp3:system/media/shutdownsound.mp3
