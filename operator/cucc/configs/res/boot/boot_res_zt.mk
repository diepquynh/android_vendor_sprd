
LOCAL_PATH:= vendor/sprd/open-source/res/boot

PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/bootanimation_zt.zip:system/media/bootanimation.zip \
	$(LOCAL_PATH)/bootsound_zt.mp3:system/media/bootsound.mp3 \
	$(LOCAL_PATH)/shutdownanimation_zt.zip:system/media/shutdownanimation.zip \
	$(LOCAL_PATH)/shutdownsound_zt.mp3:system/media/shutdownsound.mp3
