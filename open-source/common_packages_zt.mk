# The default product packages treated as base.mk in sprdroid4.1
PRODUCT_PACKAGES += \
	FMPlayer \
	SprdRamOptimizer \
	FileExplorer \
	NoteBook \
	EngineerMode \
	ValidationTools \
	DrmProvider \
	CellBroadcastReceiver \
	SprdQuickSearchBox \
        Carddav-Sync \
        Caldav-Sync.apk\
        libsprd_agps_agent
#	libsprddm \

ifneq (none,$(strip $(PRODUCT_VIDEO_WALLPAPERS)))
PRODUCT_VIDEO_WALLPAPERS := Sunny Cloud Rain Cartoon
$(call inherit-product-if-exists, frameworks/base/data/videos/VideoPackageForUUI.mk)
PRODUCT_PACKAGES += VideoWallpaper
endif

ifeq ($(TARGET_LOWCOST_SUPPORT),true)
    ifneq ($(MULTILANGUAGE_SUPPORT),true)
        PRODUCT_PACKAGES += PinyinIME
		PRODUCT_PACKAGES += OpenWnn
    endif
else
    PRODUCT_PACKAGES += PinyinIME
endif
