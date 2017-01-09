LOCAL_PATH := vendor/sprd/UniverseUI/ThemeRes/SimpleStyle

ifeq ($(strip $(OUTPUT_PREVIEW_PATH)),)
OUTPUT_PREVIEW_PATH := system/etc/theme/default/preview
endif

PRODUCT_COPY_FILES += \
$(LOCAL_PATH)/previews/preview_deskclock_0.png:$(OUTPUT_PREVIEW_PATH)/preview_deskclock_0.png \
$(LOCAL_PATH)/previews/preview_icons_0.jpg:$(OUTPUT_PREVIEW_PATH)/preview_icons_0.jpg \
$(LOCAL_PATH)/previews/preview_lockscreen_0.jpg:$(OUTPUT_PREVIEW_PATH)/preview_lockscreen_0.jpg \
$(LOCAL_PATH)/previews/preview_mms_0.png:$(OUTPUT_PREVIEW_PATH)/preview_mms_0.png \
$(LOCAL_PATH)/previews/preview_settings_0.png:$(OUTPUT_PREVIEW_PATH)/preview_mms_0.png \
$(LOCAL_PATH)/previews/preview_dialer_0.png:$(OUTPUT_PREVIEW_PATH)/preview_dialer_0.png \
$(LOCAL_PATH)/previews/preview_statusbar_0.jpg:$(OUTPUT_PREVIEW_PATH)/preview_statusbar_0.jpg \
$(LOCAL_PATH)/wallpaper.jpg:$(OUTPUT_PREVIEW_PATH)/../wallpaper.jpg \
$(LOCAL_PATH)/lockscreen.jpg:$(OUTPUT_PREVIEW_PATH)/../lockscreen.jpg

ifneq ($(TARGET_LOWCOST_SUPPORT), true)
PRODUCT_COPY_FILES += \
$(LOCAL_PATH)/previews/preview_deskclock_0.png:$(OUTPUT_PREVIEW_PATH)/preview_deskclock_0.png \
$(LOCAL_PATH)/previews/preview_contacts_0.png:$(OUTPUT_PREVIEW_PATH)/preview_contacts_0.png \
$(LOCAL_PATH)/previews/preview_filexplorer_0.png:$(OUTPUT_PREVIEW_PATH)/preview_fileexplorer_0.png

endif
