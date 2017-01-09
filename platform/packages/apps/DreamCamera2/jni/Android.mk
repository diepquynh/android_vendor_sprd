LOCAL_PATH:= $(call my-dir)

WIDEANGLE_NENABLE:=true

ifeq ($(strip $(TARGET_TS_UCAM_MAKEUP_WIDEANGLE_NENABLE)),false)
    WIDEANGLE_NENABLE := false
endif

ifeq ($(strip $(WIDEANGLE_NENABLE)),true)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/feature_stab/db_vlvm \
        $(LOCAL_PATH)/feature_stab/src \
        $(LOCAL_PATH)/feature_stab/src/dbreg \
        $(LOCAL_PATH)/feature_mos/src \
        $(LOCAL_PATH)/feature_mos/src/mosaic \
	$(TOPDIR)system/core/include \
	$(TOPDIR)hardware/libhardware/include \
	$(TOPDIR)frameworks/native/include \
	$(TOPDIR)frameworks/native/opengl/include \

LOCAL_C_INCLUDES += $(GPU_GRALLOC_INCLUDES)
LOCAL_CFLAGS := -O3 -DNDEBUG -D__STDC_FORMAT_MACROS -fstrict-aliasing -DEGL_EGLEXT_PROTOTYPES -DGL_GLEXT_PROTOTYPES #-DOPTIMIZE_OPENGLES_CALL

LOCAL_SRC_FILES := \
        feature_mos_jni.cpp \
        mosaic_renderer_jni.cpp \
        feature_mos/src/mosaic/trsMatrix.cpp \
        feature_mos/src/mosaic/AlignFeatures.cpp \
        feature_mos/src/mosaic/Blend.cpp \
        feature_mos/src/mosaic/Delaunay.cpp \
        feature_mos/src/mosaic/ImageUtils.cpp \
        feature_mos/src/mosaic/Mosaic.cpp \
        feature_mos/src/mosaic/Pyramid.cpp \
        feature_mos/src/mosaic_renderer/Renderer.cpp \
        feature_mos/src/mosaic_renderer/WarpRenderer.cpp \
        feature_mos/src/mosaic_renderer/SurfaceTextureRenderer.cpp \
        feature_mos/src/mosaic_renderer/YVURenderer.cpp \
        feature_mos/src/mosaic_renderer/FrameBuffer.cpp \
        feature_stab/db_vlvm/db_feature_detection.cpp \
        feature_stab/db_vlvm/db_feature_matching.cpp \
        feature_stab/db_vlvm/db_framestitching.cpp \
        feature_stab/db_vlvm/db_image_homography.cpp \
        feature_stab/db_vlvm/db_rob_image_homography.cpp \
        feature_stab/db_vlvm/db_utilities.cpp \
        feature_stab/db_vlvm/db_utilities_camera.cpp \
        feature_stab/db_vlvm/db_utilities_indexing.cpp \
        feature_stab/db_vlvm/db_utilities_linalg.cpp \
        feature_stab/db_vlvm/db_utilities_poly.cpp \
        feature_stab/src/dbreg/dbreg.cpp \
        feature_stab/src/dbreg/dbstabsmooth.cpp \
        feature_stab/src/dbreg/vp_motionmodel.c

ifeq ($(TARGET_ARCH), arm)
        LOCAL_SDK_VERSION := 9
endif

ifeq ($(TARGET_ARCH), x86)
        LOCAL_SDK_VERSION := 9
endif

ifeq ($(TARGET_ARCH), mips)
        LOCAL_SDK_VERSION := 9
endif

LOCAL_LDFLAGS := -llog -lGLESv2

LOCAL_MODULE_TAGS := optional
LOCAL_SHARED_LIBRARIES := libcutils \
                          libutils \
                          libhardware \
                          libEGL \
                          libGLESv1_CM \
                          libGLESv2 \
                          libbinder \
                          libui \
                          libgui
LOCAL_MODULE    := libjni_mosaic_dream
include $(BUILD_SHARED_LIBRARY)
endif

# TinyPlanet
include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cc
LOCAL_LDFLAGS   := -llog -ljnigraphics
LOCAL_SDK_VERSION := 9
LOCAL_MODULE    := libjni_tinyplanet_dream
LOCAL_SRC_FILES := tinyplanet.cc

LOCAL_CFLAGS    += -ffast-math -O3 -funroll-loops
LOCAL_ARM_MODE := arm

include $(BUILD_SHARED_LIBRARY)

# JpegUtil
include $(CLEAR_VARS)

LOCAL_CFLAGS := -std=c++11
LOCAL_NDK_STL_VARIANT := c++_static
LOCAL_LDFLAGS   := -llog -ldl -ljnigraphics
LOCAL_SDK_VERSION := 9
LOCAL_MODULE    := libjni_jpegutil_dream
LOCAL_SRC_FILES := jpegutil.cpp jpegutilnative.cpp

LOCAL_C_INCLUDES += external/jpeg

LOCAL_SHARED_LIBRARIES := libjpeg

LOCAL_CFLAGS    += -ffast-math -O3 -funroll-loops
LOCAL_ARM_MODE := arm

include $(BUILD_SHARED_LIBRARY)
