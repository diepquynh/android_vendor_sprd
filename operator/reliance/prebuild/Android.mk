# Spreadtrum Communications Inc.
# Suggest to use this Android.mk like Annotation

# The LOCAL_PATH must be defined, build error if not, my-dir is a definition
# help us to find where we are easily.
LOCAL_PATH:= $(call my-dir)

##############################################################################
# Annotation: Suggest dimming new module like this:
#
# Firstly, using CLEAR_VARS to clean vars in last build. MUST Call
#-------- include $(CLEAR_VARS)
#
# Whether this module in different product, eng: only build in eng product
# user: only build in user product; optional: eng/user
#-------- LOCAL_MODULE_TAGS := optional
#
# The identity of the module, treated as id and MUST be unique, build error
# if not, and put the module into PRODUCT_PACKAGES in specA.mk if you want to
# build it into version. Suggest not using suffix as .apk
#-------- LOCAL_MODULE := AnApplicationCMCC
#
# The instatlled package name finally in out, if you not define this one, it
# will be as same as LOCAL_MODULE.
#-------- LOCAL_MODULE_STEM := AnApplication.apk
# It will be Install: out/target/product/xxx/system/app/AnApplication.apk
#
# The Certificate to sign the apk.
#-------- LOCAL_CERTIFICATE := PRESIGNED
#
# The path of the apk generated.
#-------- LOCAL_MODULE_PATH := $(TARGET_OUT)/preloadapp
#
# Where the apk copy from. NOTICE: Use suggestion, if the third-party apk
# updated, change it here only.
#-------- LOCAL_SRC_FILES := SOURCEAPK_v1.0.1.apk
#
# Finally call BUILD_PREBUILT to build this apk.
# include $(BUILD_PREBUILT)
#########################################################################

# Prepare archs assuming
# TODO not support multi archs
my_archs := arm x86 arm64 x86_64
my_src_arch := $(call get-prebuilt-src-arch, $(my_archs))
ifeq ($(my_src_arch),arm)
    my_src_abi := armeabi*
else ifeq ($(my_src_arch),x86)
    my_src_abi := x86
else ifeq ($(my_src_arch),arm64)
    my_src_abi := arm64-v8a
else ifeq ($(my_src_arch),x86_64)
    my_src_abi := x86_64
endif

# TODO asumme linux or macos, the unzip command may perform different
# TODO can not compatibility with LOCAL_MUTILIB
define get-prebuilt-jni
    $(strip $(if $(wildcard $(strip $(LOCAL_PATH))/$(strip $(1))), \
        $(foreach arch,$(2), \
            $(if $(shell unzip -l $(strip $(LOCAL_PATH))/$(strip $(1)) | grep $(arch)), \
                $(eval LOCAL_PREBUILT_JNI_LIBS += @lib/$(arch)/*) \
            ,) \
        ) \
    ,))
endef

#
# Add or remove prebuild item in specA.mk through LOCAL_MODULE
# If prebuild apk need upate, only change LOCAL_SRC_FILES

#Net Velocity
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := Net_Velocity
LOCAL_MODULE_STEM := Net_Velocity.apk
LOCAL_MODULE_CLASS := APPS
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_MODULE_PATH := $(TARGET_OUT)/preloadapp
LOCAL_SRC_FILES := app/NVConsumer_prod_v2.5.5.apk
LOCAL_DEX_PREOPT := false

include $(BUILD_PREBUILT)

#Jio Drive
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := Jio_Drive
LOCAL_MODULE_STEM := Jio_Drive.apk
LOCAL_MODULE_CLASS := APPS
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_MODULE_PATH := $(TARGET_OUT)/app
LOCAL_SRC_FILES := app/15.1.43-reliance-prod-release.apk
$(call get-prebuilt-jni, $(LOCAL_SRC_FILES), $(my_src_abi))
include $(BUILD_PREBUILT)

#Jio Setup Wizard
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := Jio_Setup_Wizard
LOCAL_MODULE_STEM := Jio_Setup_Wizard.apk
LOCAL_MODULE_CLASS := APPS
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_MODULE_PATH := $(TARGET_OUT)/priv-app
LOCAL_SRC_FILES := app/JioDrive_Setup-signed-14-vendor.apk
include $(BUILD_PREBUILT)

#Jio Settings
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := Jio_Settings
LOCAL_MODULE_STEM := Jio_Settings.apk
LOCAL_MODULE_CLASS := APPS
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_MODULE_PATH := $(TARGET_OUT)/app
LOCAL_SRC_FILES := app/jio-mdk-services-prod-release-1.5.22.apk

include $(BUILD_PREBUILT)

#Jio Chat
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := Jio_Chat
LOCAL_MODULE_STEM := Jio_Chat.apk
LOCAL_MODULE_CLASS := APPS
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_MODULE_PATH := $(TARGET_OUT)/preloadapp
LOCAL_SRC_FILES := app/JioChat_Pro_V1.0.3.0710_1631_100002.apk
#$(call get-prebuilt-jni, $(LOCAL_SRC_FILES), $(my_src_abi))
include $(BUILD_PREBUILT)

#Jio Mags
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := Jio_Mags
LOCAL_MODULE_STEM := Jio_Mags.apk
LOCAL_MODULE_CLASS := APPS
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_MODULE_PATH := $(TARGET_OUT)/app
LOCAL_SRC_FILES := app/JioMagsProduction_09_09_2015.apk
$(call get-prebuilt-jni, $(LOCAL_SRC_FILES), $(my_src_abi))
include $(BUILD_PREBUILT)

#Jio News Express
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := Jio_News_Express
LOCAL_MODULE_STEM := Jio_News_Express.apk
LOCAL_MODULE_CLASS := APPS
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_MODULE_PATH := $(TARGET_OUT)/app
LOCAL_SRC_FILES := app/JIO_XPNN_DEV_MAPP_ANDROID_008.apk
include $(BUILD_PREBUILT)

#Jio Beat
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := Jio_Beat
LOCAL_MODULE_STEM := Jio_Beat.apk
LOCAL_MODULE_CLASS := APPS
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_MODULE_PATH := $(TARGET_OUT)/preloadapp
LOCAL_SRC_FILES := app/jiobeats_production_build.apk
#$(call get-prebuilt-jni, $(LOCAL_SRC_FILES), $(my_src_abi))
include $(BUILD_PREBUILT)

#Jio VOD
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := Jio_Vod
LOCAL_MODULE_STEM := Jio_Vod.apk
LOCAL_MODULE_CLASS := APPS
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_MODULE_PATH := $(TARGET_OUT)/app
LOCAL_SRC_FILES := app/JioVod_1.87.apk
$(call get-prebuilt-jni, $(LOCAL_SRC_FILES), $(my_src_abi))
include $(BUILD_PREBUILT)

#Sales Tracker
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := Sales_Tracker
LOCAL_MODULE_STEM := Sales_Tracker.apk
LOCAL_MODULE_CLASS := APPS
LOCAL_CERTIFICATE := platform
LOCAL_MODULE_PATH := $(TARGET_OUT)/app
LOCAL_SRC_FILES := app/Sales_Tracker_Reliancejio_0831_test_4V1.0.7.apk
include $(BUILD_PREBUILT)

#FOTA
#include $(CLEAR_VARS)
#LOCAL_MODULE_TAGS := optional
#LOCAL_MODULE :=
#LOCAL_MODULE_STEM :=
#LOCAL_MODULE_CLASS :=
#LOCAL_CERTIFICATE :=
#LOCAL_MODULE_PATH :=
#LOCAL_SRC_FILES := app/FotaUpdate_standard_APK.apk
#include $(BUILD_PREBUILT)

# library of the PDF Viewer
#include $(CLEAR_VARS)
#LOCAL_MODULE_TAGS := optional
#LOCAL_MODULE := libpdfview2.so
#LOCAL_MODULE_CLASS := SHARED_LIBRARIES
#LOCAL_SRC_FILES := lib/libpdfview2.so
#include $(BUILD_PREBUILT)
