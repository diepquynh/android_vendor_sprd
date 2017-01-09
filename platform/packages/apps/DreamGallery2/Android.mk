LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_STATIC_JAVA_LIBRARIES := android-support-v13
LOCAL_STATIC_JAVA_LIBRARIES += com.sprd.gallery3d.common2
LOCAL_STATIC_JAVA_LIBRARIES += xmp_toolkit
LOCAL_STATIC_JAVA_LIBRARIES += mp4parser
LOCAL_STATIC_JAVA_LIBRARIES += glide
LOCAL_STATIC_JAVA_LIBRARIES += MoviePlayerVideoView
LOCAL_STATIC_JAVA_LIBRARIES += com.sprd.appbackup.service

LOCAL_SRC_FILES := \
    $(call all-java-files-under, src) \
    $(call all-renderscript-files-under, src)
LOCAL_SRC_FILES += $(call all-java-files-under, src_pd)
LOCAL_SRC_FILES += src/com/sprd/gallery3d/aidl/IFloatWindowController.aidl

LOCAL_RESOURCE_DIR += $(LOCAL_PATH)/res

LOCAL_AAPT_FLAGS := --auto-add-overlay

LOCAL_PACKAGE_NAME := DreamGallery2

LOCAL_OVERRIDES_PACKAGES := Gallery Gallery3D GalleryNew3D Gallery2

#LOCAL_SDK_VERSION := current

#LOCAL_CERTIFICATE := platform
LOCAL_PRIVILEGED_MODULE := true

LOCAL_JAVA_LIBRARIES := sprd-support-addon
LOCAL_JAVA_LIBRARIES += sprd-framework
LOCAL_JAVA_LIBRARIES += sprd-support-windowdecoractionbar-disableanimation

LOCAL_JNI_SHARED_LIBRARIES := libsprdjni_eglfence libsprdjni_filtershow_filters libsprdjni_jpegstream

LOCAL_PROGUARD_FLAG_FILES := proguard.flags

LOCAL_JAVA_LIBRARIES += org.apache.http.legacy

LOCAL_DEX_PREOPT := false

LOCAL_PROGUARD_ENABLED := disabled

include $(BUILD_PACKAGE)

include $(CLEAR_VARS)
LOCAL_MODULE := MoviePlayerVideoView
LOCAL_SRC_FILES := sprd/src/com/sprd/gallery3d/app/MoviePlayerVideoView.java
LOCAL_PROGUARD_ENABLED := disabled
include $(BUILD_STATIC_JAVA_LIBRARY)
ifeq ($(strip $(LOCAL_PACKAGE_OVERRIDES)),)

# Use the following include to make gallery test apk
include $(call all-makefiles-under, $(LOCAL_PATH))

endif
