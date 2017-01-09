LOCAL_PATH := $(call my-dir)
# Building the omxil-vpu-component
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/library_entry_point.c \
	src/omx_vpudec_component.c \
	src/android_support.cpp \
	vpp_deinterlace/vpp_deint_api.c \
	vpp_deinterlace/vpp_drv_interface.c

LOCAL_CFLAGS := -DCONFIG_DEBUG_LEVEL=15 -DANDROID_PLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)
#LOCAL_CFLAGS    += -g
#LOCAL_CFLAGS += -DCNM_FPGA_PLATFORM
#LOCAL_CFLAGS += -DCNM_FPGA_CTS_TEST

LOCAL_MODULE_TAGS := eng
LOCAL_MODULE := libomxvpu

#LOCAL_STATIC_LIBRARIES := libyuv_static

#LOCAL_LDLIBS += -lpthread

LOCAL_SHARED_LIBRARIES :=       \
	libvpu                  \
	libomxil-bellagio       \
	libutils                \
	libcutils               \
	libdl                   \
	libsync	                \
	libsync	                \
	libmemion        \
	libui

LOCAL_C_INCLUDES := $(LOCAL_PATH)/src		\
                    $(LOCAL_PATH)/vpp_deinterlace \
			$(TOP)/vendor/sprd/modules/media/vpu/mmf/openmax/libomxil-bellagio-0.9.3/src \
			$(TOP)/vendor/sprd/modules/media/vpu/mmf/openmax/libomxil-bellagio-0.9.3/src/base \
			$(TOP)/vendor/sprd/modules/media/vpu \
			$(TOP)/vendor/sprd/modules/media/vpu/vpuapi \
			$(TOP)/vendor/sprd/modules/media/libstagefrighthw/include/openmax \
			$(TOP)/frameworks/av/media/libstagefright \
			$(TOP)/frameworks/native/include/media/hardware \
			$(TOP)/frameworks/base/include/media/stagefright/openmax \
			$(TOP)/frameworks/base/include/media/stagefright \
			$(TOP)/vendor/sprd/modules/libmemion \
			$(TOP)/vendor/sprd/external/drivers/gpu/midgard/include/ \
			$(TOP)/vendor/sprd/external/drivers/gpu/utgard/include/ \
			$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video

ifeq ($(strip $(TARGET_GPU_PLATFORM)),midgard)
LOCAL_C_INCLUDES += $(TOP)/vendor/sprd/proprietories-source/libgpu/gralloc/midgard
else 
LOCAL_C_INCLUDES += $(TOP)/vendor/sprd/proprietories-source/libgpu/gralloc/utgard
endif 

# needed for #include <libyuv.h> to use ConvertRgbToYuvbySW in android_support.h
LOCAL_C_INCLUDES += $(TOP)/external/libyuv/files/include
# needed for #include <gralloc_priv.h> to use getAndroidNativeBufferHandleInfo in android_support.h
# SHOULD change your libgralloc path
LOCAL_C_INCLUDES += $(TOP)/hardware/qcom/display/msm8994/libgralloc

LOCAL_COPY_HEADERS_TO := ./bellagio
LOCAL_COPY_HEADERS := src/OMX_VPU_Index.h			\
			src/OMX_VPU_Video.h

include $(BUILD_SHARED_LIBRARY)
