
LOCAL_PATH:= vendor/sprd/open-source/res/boot

PRODUCT_COPY_FILES += \
        $(LOCAL_PATH)/bootanimation_8830.zip:system/media/bootanimation.zip \
        $(LOCAL_PATH)/bootsound.mp3:system/media/bootsound.mp3 \
        $(LOCAL_PATH)/shutdownanimation_8830.zip:system/media/shutdownanimation.zip \
        $(LOCAL_PATH)/shutdownsound.mp3:system/media/shutdownsound.mp3
