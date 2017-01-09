
LOCAL_PATH:= vendor/sprd/open-source/res/boot

ifeq ($(strip $(PRODUCT_LCD_TYPE)),hvga)
    -include $(LOCAL_PATH)/boot_res_cu_hvga.mk
else
PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/bootanimation.zip:system/media/bootanimation.zip \
	$(LOCAL_PATH)/bootsound.mp3:system/media/bootsound.mp3 \
	$(LOCAL_PATH)/shutdownanimation.zip:system/media/shutdownanimation.zip \
	$(LOCAL_PATH)/shutdownsound.mp3:system/media/shutdownsound.mp3
endif
