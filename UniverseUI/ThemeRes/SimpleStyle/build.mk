LOCAL_THEME_NAME := SimpleStyle

ifeq ($(strip $(PRODUCT_DEFAULT_THEME)),$(strip $(LOCAL_THEME_NAME)))
LOCAL_IS_DEFAULT_THEME := true
endif

ifeq ($(strip $(LOCAL_IS_DEFAULT_THEME)),true)
OUTPUT_PREVIEW_PATH := system/etc/theme/default/preview
else
OUTPUT_PREVIEW_PATH := system/etc/theme/additional/$(LOCAL_THEME_NAME)/preview
endif

include vendor/sprd/UniverseUI/ThemeRes/SimpleStyle/copy_previews.mk

# SimpleStyle
# Build theme package named SimpleStyle
PRODUCT_PACKAGES += \
    $(LOCAL_THEME_NAME)com.android.contacts \
    $(LOCAL_THEME_NAME)com.android.dialer \
    $(LOCAL_THEME_NAME)com.sprd.fileexplorer \
    $(LOCAL_THEME_NAME)framework-res \
    $(LOCAL_THEME_NAME)com.android.mms \
    $(LOCAL_THEME_NAME)com.android.settings \
    $(LOCAL_THEME_NAME)com.android.systemui \
    $(LOCAL_THEME_NAME)com.android.deskclock \
    $(LOCAL_THEME_NAME)com.android.launcher3 \
    $(LOCAL_THEME_NAME)com.sprd.audioprofile \
    $(LOCAL_THEME_NAME)res.sprd.icons

PRODUCT_COPY_FILES += \
    vendor/sprd/UniverseUI/ThemeRes/SimpleStyle/description.xml:$(OUTPUT_PREVIEW_PATH)/../description.xml

ifeq ($(strip $(PRODUCT_BUILD_DEFAULT_THEME_BY_OVERLAY)),true)
ifeq ($(strip $(LOCAL_IS_DEFAULT_THEME)),true)
DEVICE_PACKAGE_OVERLAYS += \
    vendor/sprd/UniverseUI/ThemeRes/SimpleStyle/overlays
endif
endif

# CLEAR_VARS
LOCAL_IS_DEFAULT_THEME :=
LOCAL_THEME_NAME :=
OUTPUT_PREVIEW_PATH :=
