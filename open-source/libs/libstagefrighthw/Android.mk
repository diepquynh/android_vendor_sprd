LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    SprdOMXPlugin.cpp \
	SprdOMXComponent.cpp \
	SprdSimpleOMXComponent.cpp

LOCAL_CFLAGS := $(PV_CFLAGS_MINUS_VISIBILITY)

LOCAL_C_INCLUDES:= \
	$(TOP)/frameworks/native/include/media/hardware \
	$(TOP)/vendor/sprd/open-source/libs/libstagefrighthw/include	\
	$(TOP)/vendor/sprd/open-source/libs/libstagefrighthw/include/openmax	\
	$(TOP)/vendor/sprd/open-source/libs/libmemoryheapion \

ifeq ($(strip $(TARGET_GPU_PLATFORM)),midgard)
    LOCAL_C_INCLUDES += $(TOP)/vendor/sprd/modules/libgpu/gralloc/midgard
else
    LOCAL_C_INCLUDES += $(TOP)/vendor/sprd/modules/libgpu/gralloc/utgard
endif

LOCAL_SHARED_LIBRARIES :=       \
        libmemoryheapion        \
        libutils                \
        libcutils               \
        libui                   \
        libdl			\
	libstagefright_foundation
LOCAL_MODULE := libstagefrighthw

LOCAL_CFLAGS:= -DLOG_TAG=\"$(TARGET_BOARD_PLATFORM).libstagefright\"

include $(BUILD_SHARED_LIBRARY)

