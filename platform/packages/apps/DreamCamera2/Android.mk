LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
MY_FILTER_SCENERY := true
MY_EDIT_PUZZLE:=true
MY_TIME_STAMP:=true
MY_GIF_CAM:=true
MY_SCENERY_CAM:=true
MY_FILTER_CAM:=true
MY_VGESTURE_CAM:=true
MY_FILTER_SCENERY_GIF:=true
MY_EDIT_PUZZLE_GIF:=true

DREAM_UI:=true

ifeq ($(strip $(TARGET_TS_UCAM_EDIT_PUZZLE_NENABLE)),false)
    MY_EDIT_PUZZLE := false
endif

ifeq ($(strip $(TARGET_TS_UCAM_MAKEUP_TIMESTAMP_NENABLE)),false)
    MY_TIME_STAMP:=false
endif

ifeq ($(strip $(TARGET_TS_UCAM_MAKEUP_GIF_NENABLE)),false)
    MY_GIF_CAM := false
endif

ifeq ($(strip $(TARGET_TS_UCAM_MAKEUP_SCENERY_NENABLE)),false)
    MY_SCENERY_CAM := false
endif

ifeq ($(strip $(TARGET_TS_UCAM_MAKEUP_VGESTURE_NENABLE)),false)
    MY_VGESTURE_CAM := false
endif

ifeq ($(strip $(TARGET_TS_UCAM_MAKEUP_FILTER_NENABLE)),false)
    MY_FILTER_CAM := false
endif

ifeq ($(strip $(TARGET_TS_UCAM_MAKEUP_FILTER_NENABLE)),false)
ifeq ($(strip $(TARGET_TS_UCAM_MAKEUP_SCENERY_NENABLE)),false)
	MY_FILTER_SCENERY := false
endif
endif

ifeq ($(strip $(TARGET_TS_UCAM_EDIT_PUZZLE_NENABLE)),false)
ifeq ($(strip $(TARGET_TS_UCAM_MAKEUP_GIF_NENABLE)),false)
    MY_EDIT_PUZZLE_GIF:=false
endif
endif


ifeq ($(strip $(TARGET_TS_UCAM_MAKEUP_FILTER_NENABLE)),false)
ifeq ($(strip $(TARGET_TS_UCAM_MAKEUP_SCENERY_NENABLE)),false)
ifeq ($(strip $(TARGET_TS_UCAM_MAKEUP_GIF_NENABLE)),false)
    MY_FILTER_SCENERY_GIF:=false
endif
endif
endif

ifeq ($(strip $(TARGET_TS_DRAM_UI_NENABLE)),false)
    DREAM_UI := false
endif

LOCAL_STATIC_JAVA_LIBRARIES := android-support-v13
LOCAL_STATIC_JAVA_LIBRARIES := android-support-v4
LOCAL_STATIC_JAVA_LIBRARIES += android-ex-camera2-portability
LOCAL_STATIC_JAVA_LIBRARIES += xmp_toolkit
LOCAL_STATIC_JAVA_LIBRARIES += glide
LOCAL_STATIC_JAVA_LIBRARIES += guava
LOCAL_STATIC_JAVA_LIBRARIES += jsr305
LOCAL_JAVA_LIBRARIES += sprd-framework
LOCAL_STATIC_JAVA_LIBRARIES += zxing

ifeq ($(strip $(MY_EDIT_PUZZLE_GIF)),true)
LOCAL_STATIC_JAVA_LIBRARIES += libemail-uphoto
LOCAL_JAVA_LIBRARIES := org.apache.http.legacy
endif

LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_SRC_FILES += $(call all-java-files-under, src_pd)
LOCAL_SRC_FILES += $(call all-java-files-under, src_pd_gcam)
LOCAL_SRC_FILES += src/com/sprd/gallery3d/aidl/IFloatWindowController.aidl

LOCAL_SRC_FILES += $(call all-java-files-under, src_ucam/UCamera/src/com/ucamera/ucam/jni)

ifeq ($(strip $(MY_EDIT_PUZZLE_GIF)),true)
LOCAL_SRC_FILES += $(call all-java-files-under, src_ucam/UCommon/puzzle)
LOCAL_SRC_FILES += $(call all-java-files-under, src_ucam/UCommon/downloadcenter)
LOCAL_SRC_FILES += $(call all-java-files-under, src_ucam/UGallery)
LOCAL_SRC_FILES += $(call all-java-files-under, src_ucam/UPhoto)
else
LOCAL_SRC_FILES += $(call all-java-files-under, src_fake/com/ucamera/ugallery)
LOCAL_SRC_FILES += $(call all-java-files-under, src_fake/com/ucamera/ucomm/puzzle)
endif

ifeq ($(MY_FILTER_SCENERY_GIF),true)
LOCAL_SRC_FILES += $(call all-java-files-under, src_ucam/UCamera/src/com/ucamera/ucam/modules/ui)
LOCAL_SRC_FILES += $(call all-java-files-under, src_ucam/UCamera/src/com/ucamera/ucam/modules/utils)
LOCAL_SRC_FILES += $(call all-java-files-under, src_ucam/UCamera/src/com/ucamera/ucam/modules/BasicModule.java)
LOCAL_SRC_FILES += $(call all-java-files-under, src_ucam/UCamera/src/com/ucamera/ucam/modules/compatible)
LOCAL_SRC_FILES += $(call all-java-files-under, src_ucam/UCamera/src/com/ucamera/ucam/sound)

ifeq ($(strip $(MY_GIF_CAM)),true)
LOCAL_SRC_FILES += $(call all-java-files-under, src_ucam/UCamera/src/com/ucamera/ucam/modules/ugif)
else
LOCAL_SRC_FILES += $(call all-java-files-under, src_fake/com/ucamera/ucam/modules/ugif)
endif

ifeq ($(strip $(MY_SCENERY_CAM)),true)
LOCAL_SRC_FILES += $(call all-java-files-under, src_ucam/UCamera/src/com/ucamera/ucam/modules/uscenery)
else
LOCAL_SRC_FILES += $(call all-java-files-under, src_fake/com/ucamera/ucam/modules/uscenery)
endif

ifeq ($(strip $(MY_FILTER_CAM)),true)
LOCAL_SRC_FILES += $(call all-java-files-under, src_ucam/UCamera/src/com/ucamera/ucam/modules/ufilter)
LOCAL_SRC_FILES += $(call all-java-files-under, src_ucam/UCamera/src/com/thundersoft)
else
LOCAL_SRC_FILES += $(call all-java-files-under, src_fake/com/ucamera/ucam/modules/ufilter)
LOCAL_SRC_FILES += $(call all-java-files-under, src_fake/com/thundersoft)
endif

else
LOCAL_SRC_FILES += $(call all-java-files-under, src_fake/com/ucamera/ucam)
LOCAL_SRC_FILES += $(call all-java-files-under, src_fake/com/thundersoft)
endif

LOCAL_RESOURCE_DIR += \
    $(LOCAL_PATH)/res \
    $(LOCAL_PATH)/res_p \
    $(LOCAL_PATH)/res_ucam

ifeq ($(strip $(MY_EDIT_PUZZLE_GIF)),true)
LOCAL_RESOURCE_DIR += $(LOCAL_PATH)/src_ucam/UCommon/puzzle/res
LOCAL_RESOURCE_DIR += $(LOCAL_PATH)/src_ucam/UCommon/downloadcenter/res
LOCAL_RESOURCE_DIR += $(LOCAL_PATH)/src_ucam/UGallery/res
LOCAL_RESOURCE_DIR += $(LOCAL_PATH)/src_ucam/UPhoto/res
endif

ifeq ($(MY_FILTER_SCENERY_GIF),true)
LOCAL_RESOURCE_DIR += $(LOCAL_PATH)/src_ucam/UCamera/res
ifeq ($(strip $(MY_GIF_CAM)),true)
LOCAL_RESOURCE_DIR += $(LOCAL_PATH)/src_ucam/UCamera/res_gif
endif
ifeq ($(strip $(MY_SCENERY_CAM)),true)
LOCAL_RESOURCE_DIR += $(LOCAL_PATH)/src_ucam/UCamera/res_scene
endif
ifeq ($(strip $(MY_FILTER_CAM)),true)
LOCAL_RESOURCE_DIR += $(LOCAL_PATH)/src_ucam/UCamera/res_filter
endif
endif

ifeq ($(strip $(DREAM_UI)),true)
ifneq ($(wildcard $(LOCAL_PATH)/Dream.mk),)
-include $(LOCAL_PATH)/Dream.mk
endif
endif

include $(LOCAL_PATH)/version.mk
LOCAL_AAPT_FLAGS := \
        --auto-add-overlay \
        --extra-packages com.ucamera.uphoto \
        --extra-packages com.ucamera.ugallery:com.ucamera.ucomm.puzzle:com.ucamera.ucomm.sns:com.ucamera.ucomm.downloadcenter \
        --version-name "$(version_name_package)" \
        --version-code $(version_code_package) \

LOCAL_PACKAGE_NAME := DreamCamera2
LOCAL_OVERRIDES_PACKAGES := Camera2
LOCAL_CERTIFICATE := platform

#LOCAL_SDK_VERSION := current

LOCAL_PROGUARD_FLAG_FILES := proguard.flags

ifeq ($(strip $(MY_EDIT_PUZZLE_GIF)),true)
LOCAL_PROGUARD_FLAGS += -include $(LOCAL_PATH)/src_ucam/UCamera/proguard.flags
LOCAL_PROGUARD_FLAGS += -include $(LOCAL_PATH)/src_ucam/UPhoto/proguard.flags
LOCAL_PROGUARD_FLAGS += -include $(LOCAL_PATH)/src_ucam/UGallery/proguard.flags
LOCAL_PROGUARD_FLAGS += -include $(LOCAL_PATH)/src_ucam/UCommon/puzzle/proguard.flags
endif

ifeq ($(strip $(MY_EDIT_PUZZLE_GIF)),true)
LOCAL_ASSET_DIR := $(call intermediates-dir-for, APPS, $(LOCAL_PACKAGE_NAME),,COMMON)/assets
ifeq ($(UPHOTO_ASSET_COMMON_DIR),)
UPHOTO_ASSET_COMMON_DIR := $(LOCAL_ASSET_DIR)
$(info Coping UPhoto common assets to: $(UPHOTO_ASSET_COMMON_DIR))
$(shell rm -rf $(UPHOTO_ASSET_COMMON_DIR))
define my-copy-assets
$(foreach f, $(call find-subdir-assets, $(1)), \
  $(shell mkdir -p $(dir $(2)/$(f)); $(LOCAL_PATH)/acp -fp $(1)/$(f) $(2)/$(f)) \
)
endef
$(shell rm -rf $(LOCAL_ASSET_DIR))
$(call my-copy-assets, $(LOCAL_PATH)/assets, $(LOCAL_ASSET_DIR))
$(call my-copy-assets, $(LOCAL_PATH)/src_ucam/UPhoto/assets, $(LOCAL_ASSET_DIR))
$(call my-copy-assets, $(LOCAL_PATH)/src_ucam/UCommon/puzzle/assets, $(LOCAL_ASSET_DIR))
endif
endif

LOCAL_JNI_SHARED_LIBRARIES := libjni_tinyplanet_dream libjni_jpegutil_dream

LOCAL_REQUIRED_MODULES := libmakeupengine libImageProcessJni libGpuProcessJni libjni_mosaic_dream libself_portrait_jni libtsadvancedfilterJni

include $(BUILD_PACKAGE)

include $(CLEAR_VARS)
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES := zxing:libs/core.jar

ifeq ($(strip $(MY_EDIT_PUZZLE_GIF)),true)
LOCAL_PREBUILT_STATIC_JAVA_LIBRARIES += \
                    libemail-uphoto:src_ucam/UGallery/libs/email.jar

ifeq ($(strip $(TARGET_ARCH)),arm64)
LOCAL_PREBUILT_LIBS += libGpuProcessJni:src_ucam/UPhoto/libs/arm64-v8a/libGpuProcessJni.so
LOCAL_PREBUILT_LIBS += libmakeupengine:src_ucam/UPhoto/libs/arm64-v8a/libmakeupengine.so
else
LOCAL_PREBUILT_LIBS += libGpuProcessJni:src_ucam/UPhoto/libs/armeabi-v7a/libGpuProcessJni.so
LOCAL_PREBUILT_LIBS += libmakeupengine:src_ucam/UPhoto/libs/armeabi-v7a/libmakeupengine.so
endif
endif

ifeq ($(strip $(TARGET_ARCH)),arm64)
LOCAL_PREBUILT_LIBS += libImageProcessJni:src_ucam/UPhoto/libs/arm64-v8a/libImageProcessJni.so
else
LOCAL_PREBUILT_LIBS += libImageProcessJni:src_ucam/UPhoto/libs/armeabi-v7a/libImageProcessJni.so
endif

ifeq ($(strip $(MY_VGESTURE_CAM)),true)

ifeq ($(strip $(TARGET_ARCH)),arm64)
LOCAL_PREBUILT_LIBS += libself_portrait_jni:libs/arm64-v8a/libself_portrait_jni.so
else
LOCAL_PREBUILT_LIBS += libself_portrait_jni:libs/armeabi-v7a/libself_portrait_jni.so
endif

endif

ifeq ($(strip $(MY_FILTER_CAM)),true)
ifeq ($(strip $(TARGET_ARCH)),arm64)
LOCAL_PREBUILT_LIBS += libtsadvancedfilterJni:src_ucam/UPhoto/libs/arm64-v8a/libtsadvancedfilterJni.so
else
LOCAL_PREBUILT_LIBS += libtsadvancedfilterJni:src_ucam/UPhoto/libs/armeabi-v7a/libtsadvancedfilterJni.so
endif
endif

include $(BUILD_MULTI_PREBUILT)
                    
include $(call all-makefiles-under, $(LOCAL_PATH))
