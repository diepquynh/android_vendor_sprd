
LOCAL_PATH:= vendor/sprd/open-source/res/boot

PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/bootanimation_cu_hvga.zip:system/media/bootanimation.zip \
	$(LOCAL_PATH)/bootsound_cu.mp3:system/media/bootsound.mp3 \
	$(LOCAL_PATH)/shutdownanimation_cu_hvga.zip:system/media/shutdownanimation.zip \
	$(LOCAL_PATH)/shutdownsound_cu.mp3:system/media/shutdownsound.mp3
