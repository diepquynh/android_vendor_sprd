LOCAL_PATH := vendor/sprd/UniverseUI/ThemeRes/Snowy

OUTPUT_PREVIEW_PATH := system/etc/theme/additional/Snowy/preview

PRODUCT_COPY_FILES += \
$(LOCAL_PATH)/previews/preview_contacts_0.png:$(OUTPUT_PREVIEW_PATH)/preview_contacts_0.png \
$(LOCAL_PATH)/previews/preview_icons_0.png:$(OUTPUT_PREVIEW_PATH)/preview_icons_0.png \
$(LOCAL_PATH)/previews/preview_icons_1.png:$(OUTPUT_PREVIEW_PATH)/preview_icons_1.png \
$(LOCAL_PATH)/previews/preview_lockscreen_0.png:$(OUTPUT_PREVIEW_PATH)/preview_lockscreen_0.png \
$(LOCAL_PATH)/previews/preview_mms_0.png:$(OUTPUT_PREVIEW_PATH)/preview_mms_0.png \
$(LOCAL_PATH)/previews/preview_statusbar_0.png:$(OUTPUT_PREVIEW_PATH)/preview_statusbar_0.png \
$(LOCAL_PATH)/previews/preview_statusbar_1.png:$(OUTPUT_PREVIEW_PATH)/preview_statusbar_1.png \
$(LOCAL_PATH)/wallpaper.jpg:$(OUTPUT_PREVIEW_PATH)/wallpaper.jpg

##$(LOCAL_PATH)/previews/preview_dialer_0.png:$(OUTPUT_PREVIEW_PATH)/preview_dialer_0.png \
##$(LOCAL_PATH)/previews/preview_fileexplorer_0.png:$(OUTPUT_PREVIEW_PATH)/preview_fileexplorer_0.png \
