# Common features.
PRODUCT_PROPERTY_OVERRIDES += \
	universe_ui_support=true

PRODUCT_PACKAGES += \
    lockscreen \
    Draglockscreen \
    S2LockScreen \
    theme_init.sh \
    ThemeSettings

# Common control, which theme you want to set to default one.
ifeq ($(strip $(PRODUCT_DEFAULT_THEME)),)
PRODUCT_DEFAULT_THEME := SimpleStyle
endif

# Common control, which build solution you can choose, build with
# overlay can shrink ROM size but may raise risk of building and features.
ifeq ($(strip $(PRODUCT_BUILD_DEFAULT_THEME_BY_OVERLAY)),)
PRODUCT_BUILD_DEFAULT_THEME_BY_OVERLAY := true
endif

# 
ifeq ($(strip $(PRODUCT_THEME_PACKAGES)),)
PRODUCT_THEME_PACKAGES := HelloColor SimpleStyle
endif

ifneq ($(strip $(PRODUCT_THEME_PACKAGES)),none)

# Build default theme packages list
PRODUCT_PACKAGES += \
    defaultframework-res \
    defaultcom.android.dialer \
    defaultcom.android.incallui \
    defaultcom.android.contacts \
    defaultcom.android.settings \
    defaultcom.android.mms \
    defaultcom.thunderst.radio \
    defaultcom.android.calculator2 \
    defaultcom.android.music \
    defaultcom.android.email \
    defaultcom.android.systemui \
    defaultcom.android.cellbroadcastreceiver \
    defaultcom.android.soundrecorder \
    defaultcom.android.deskclock \
    defaultcom.android.browser \
    defaultcom.android.documentsui \
    defaultcom.android.calendar \
    defaultcom.android.quicksearchbox \
    defaultcom.sprd.audioprofile \
    defaultres.sprd.icons \
    defaultcom.sprd.fileexplorer \
    defaultcom.android.deskclock \
    defaultcom.android.launcher3

endif

ifneq ($(strip $(PRODUCT_THEME_PACKAGES)),none)
ifeq ($(filter SimpleStyle, $(PRODUCT_THEME_PACKAGES)),SimpleStyle)

include  vendor/sprd/UniverseUI/ThemeRes/SimpleStyle/build.mk

endif # filter SimpleStyle
endif # PRODUCT_THEME_PACKAGES is not none

ifneq ($(strip $(PRODUCT_THEME_PACKAGES)),none)
ifeq ($(filter HelloColor, $(PRODUCT_THEME_PACKAGES)),HelloColor)

include  vendor/sprd/UniverseUI/ThemeRes/HelloColor/build.mk

endif # filter SimpleStyle
endif # PRODUCT_THEME_PACKAGES is not none


#ifneq ($(strip $(PRODUCT_THEME_PACKAGES)),none)
#ifeq ($(filter Snowy, $(PRODUCT_THEME_PACKAGES)),Snowy)

#$(call inherit-product-if-exists, vendor/sprd/UniverseUI/ThemeRes/Snowy/copy_previews.mk)

# Snowy
# Build Snowy theme package
#PRODUCT_PACKAGES += \
#    Snowycom.android.contacts \
#    Snowycom.android.dialer \
#    Snowycom.sprd.fileexplorer \
#    Snowycom.android.deskclock \
#    Snowycom.android.launcher3 \
#    Snowyframework-res \
#    Snowycom.android.mms \
#    Snowycom.android.settings \
#    Snowycom.android.systemui \
#    Snowycom.sprd.audioprofile
#endif # filter Snowy
#endif # PRODUCT_THEME_PACKAGES is not none

