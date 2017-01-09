LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

src_dirs :=  \
            ../Gallery2/addons/GalleryDrm/src \
            ../Gallery2/addons/VideoDrm/src \
            ../Mms/addons/Drm/src \
            ../Launcher3/addons/CustomizeAppSort/src \
            ../Launcher3/WallpaperPicker/addons/DRM/src \
            ../Browser/addons/DrmAddon/BrowserXposed/src \
            ../Email/addons/Drm/src \
            ../Music/addons/DrmAddon/src \
            ../../providers/DownloadProvider/addons/Drm/src

res_dirs := \
            ../Gallery2/addons/GalleryDrm/res \
            ../Gallery2/addons/VideoDrm/res \
            ../Launcher3/addons/CustomizeAppSort/res \
            ../Mms/addons/Drm/res \
            ../Browser/addons/DrmAddon/BrowserXposed/res \
            ../Email/addons/Drm/res \
            ../Music/addons/DrmAddon/res \
            ../../providers/DownloadProvider/addons/Drm/res

LOCAL_AAPT_FLAGS := \
    --auto-add-overlay \
    --extra-packages com.sprd.fileexploreraddon \
    --extra-packages com.sprd.drmgalleryplugin \
    --extra-packages com.sprd.drmvideoplugin \
    --extra-packages com.sprd.mmsaddon \
    --extra-packages addon.sprd.launcher3.drm \
    --extra-packages addon.sprd.launcher3.appsort \
    --extra-packages addon.sprd.browser.plugindrm \
    --extra-packages plugin.sprd.drmxposed \
    --extra-packages plugin.sprd.emailxposed \
    --extra-packages addon.sprd.downloadprovider

LOCAL_APK_LIBRARIES := Gallery2 Launcher3 Music Email Mms Browser \
    DownloadProvider DownloadProviderUi

LOCAL_JAVA_LIBRARIES := telephony-common

#Gallery2

LOCAL_SRC_FILES := $(call all-java-files-under,$(src_dirs))

LOCAL_RESOURCE_DIR := $(addprefix $(LOCAL_PATH)/, $(res_dirs))

LOCAL_PACKAGE_NAME := DrmAddon

LOCAL_PROGUARD_FLAG_FILES := proguard.flags
#LOCAL_PROGUARD_ENABLED := disabled

# Warning, please use the BUILD_ADDON_PACKAGE instead of BUILD_PACKAGE when building the addon
#include $(BUILD_ADDON_PACKAGE)
include $(call all-makefiles-under,$(LOCAL_PATH))
