
# 480*800
LOCAL_PATH:= vendor/sprd/operator/cucc/specA/res/boot/wvga

PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/bootanimation.zip:system/media/bootanimation.zip \
	$(LOCAL_PATH)/bootsound.mp3:system/media/bootsound.mp3 \
	$(LOCAL_PATH)/shutdownanimation.zip:system/media/shutdownanimation.zip \
	$(LOCAL_PATH)/shutdownsound.mp3:system/media/shutdownsound.mp3
