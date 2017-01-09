# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


LOCAL_PATH := $(call my-dir)

# HAL module implemenation stored in
# hw/<OVERLAY_HARDWARE_MODULE_ID>.<ro.product.board>.so
ifneq ($(strip $(TARGET_SUPPORT_ADF_DISPLAY)),true)
ifeq ($(strip $(USE_SPRD_HWCOMPOSER)),true)

include $(CLEAR_VARS)

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_SHARED_LIBRARIES := liblog libEGL libmemoryheapion libutils libcutils libGLESv1_CM libGLESv2 libhardware libui libsync
LOCAL_SRC_FILES := SprdHWComposer.cpp \
		   SprdPrimaryDisplayDevice/SprdFrameBufferHAL.cpp \
		   AndroidFence.cpp \
		   SprdDisplayPlane.cpp \
		   SprdHWLayer.cpp \
		   SprdPrimaryDisplayDevice/SprdPrimaryDisplayDevice.cpp \
		   SprdPrimaryDisplayDevice/SprdVsyncEvent.cpp \
		   SprdPrimaryDisplayDevice/SprdHWLayerList.cpp \
		   SprdPrimaryDisplayDevice/SprdOverlayPlane.cpp \
		   SprdPrimaryDisplayDevice/SprdPrimaryPlane.cpp \
		   SprdVirtualDisplayDevice/SprdVirtualDisplayDevice.cpp \
		   SprdVirtualDisplayDevice/SprdVDLayerList.cpp \
		   SprdVirtualDisplayDevice/SprdVirtualPlane.cpp \
		   SprdVirtualDisplayDevice/SprdWIDIBlit.cpp \
		   SprdExternalDisplayDevice/SprdExternalDisplayDevice.cpp \
		   SprdUtil.cpp \
                   dump.cpp
LOCAL_C_INCLUDES := \
	$(TOP)/vendor/sprd/open-source/libs/libmemoryheapion \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video \

ifeq ($(strip $(TARGET_GPU_PLATFORM)),midgard)
LOCAL_C_INCLUDES += $(TOP)/vendor/sprd/modules/libgpu/gralloc/midgard
else
LOCAL_C_INCLUDES += $(TOP)/vendor/sprd/modules/libgpu/gralloc/utgard
endif

LOCAL_MODULE := hwcomposer.$(TARGET_BOARD_PLATFORM)
LOCAL_CFLAGS:= -DLOG_TAG=\"SPRDHWComposer\"
LOCAL_CFLAGS += -D_USE_SPRD_HWCOMPOSER -DGL_GLEXT_PROTOTYPES -DEGL_EGLEXT_PROTOTYPES

#DEVICE_OVERLAYPLANE_BORROW_PRIMARYPLANE_BUFFER can make SprdPrimaryPlane
#share the plane buffer to SprdOverlayPlane,
#save 3 screen size YUV420 buffer memory.
#DEVICE_PRIMARYPLANE_USE_RGB565 make the SprdPrimaryPlane use
#RGB565 format buffer to display, also can save 4 screen size
#buffer memory, but it will reduce the image quality.

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sc8830)
DEVICE_WITH_GSP := true
DEVICE_OVERLAYPLANE_BORROW_PRIMARYPLANE_BUFFER := true
DEVICE_USE_FB_HW_VSYNC := true
DEVICE_DIRECT_DISPLAY_SINGLE_OSD_LAYER := true
endif
ifeq ($(strip $(TARGET_BOARD_PLATFORM)),scx15)
DEVICE_WITH_GSP := true
DEVICE_OVERLAYPLANE_BORROW_PRIMARYPLANE_BUFFER := true
#DEVICE_PRIMARYPLANE_USE_RGB565 := true
#DEVICE_DYNAMIC_RELEASE_PLANEBUFFER := true
endif

ifeq ($(strip $(DEVICE_USE_FB_HW_VSYNC)),true)
	LOCAL_CFLAGS += -DUSE_FB_HW_VSYNC
endif

#SPRD_HWC_DEBUG_TRACE := false

ifeq ($(strip $(DEVICE_WITH_GSP)),true)
	LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libcamera/sc8830/inc	
	#LOCAL_CFLAGS += -DVIDEO_LAYER_USE_RGB
	# PROCESS_VIDEO_USE_GSP : protecting sc8830 code
	LOCAL_CFLAGS += -DPROCESS_VIDEO_USE_GSP
	LOCAL_CFLAGS += -DGSP_OUTPUT_USE_YUV420

	# LOCAL_CFLAGS += -D_DMA_COPY_OSD_LAYER

#
# if GSP has not IOMMU, DIRECT_DISPLAY_SINGLE_OSD_LAYER need contiguous physcial address;
# if GSP has IOMMU, we can open DIRECT_DISPLAY_SINGLE_OSD_LAYER.
ifeq ($(strip $(DEVICE_DIRECT_DISPLAY_SINGLE_OSD_LAYER)),true)
        LOCAL_CFLAGS += -DDIRECT_DISPLAY_SINGLE_OSD_LAYER
endif

endif

ifeq ($(strip $(DEVICE_PRIMARYPLANE_USE_RGB565)), true)
	LOCAL_CFLAGS += -DPRIMARYPLANE_USE_RGB565
endif

ifeq ($(strip $(DEVICE_OVERLAYPLANE_BORROW_PRIMARYPLANE_BUFFER)), true)
	LOCAL_CFLAGS += -DBORROW_PRIMARYPLANE_BUFFER
endif

ifeq ($(strip $(DEVICE_DYNAMIC_RELEASE_PLANEBUFFER)), true)
	LOCAL_CFLAGS += -DDYNAMIC_RELEASE_PLANEBUFFER
endif

# For Virtual Display
# HWC need do the Hardware copy and format convertion
# FORCE_ADJUST_ACCELERATOR: for a better performance of Virtual Display,
# we forcibly make sure the GSP/GPP device to be used by Virtual Display.
# and disable the GSP/GPP on Primary Display.
ifeq ($(TARGET_FORCE_HWC_FOR_VIRTUAL_DISPLAYS),true)
	LOCAL_CFLAGS += -DFORCE_HWC_COPY_FOR_VIRTUAL_DISPLAYS
	#LOCAL_CFLAGS += -DFORCE_ADJUST_ACCELERATOR
endif

# OVERLAY_COMPOSER_GPU_CONFIG: Enable or disable OVERLAY_COMPOSER_GPU
# Macro, OVERLAY_COMPOSER will do Hardware layer blending and then
# post the overlay buffer to OSD display plane.
# If you want to know how OVERLAY_COMPOSER use and work,
# Please see the OverlayComposer/OverlayComposer.h for more details.
ifeq ($(strip $(USE_OVERLAY_COMPOSER_GPU)),true)

ifeq ($(strip $(TARGET_GPU_PLATFORM)),midgard)
	LOCAL_CFLAGS += -DINVALIDATE_WINDOW_TARGET
endif
	LOCAL_CFLAGS += -DOVERLAY_COMPOSER_GPU

	LOCAL_SRC_FILES += OverlayComposer/OverlayComposer.cpp

	LOCAL_SRC_FILES += OverlayComposer/OverlayNativeWindow.cpp

	LOCAL_SRC_FILES += OverlayComposer/Layer.cpp

	LOCAL_SRC_FILES += OverlayComposer/Utility.cpp

	LOCAL_SRC_FILES += OverlayComposer/SyncThread.cpp

endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sc8825)
	#LOCAL_CFLAGS += -DTRANSFORM_USE_GPU
	LOCAL_CFLAGS += -DSCAL_ROT_TMP_BUF

	LOCAL_C_INCLUDES += $(LOCAL_PATH)/../libcamera/sc8825/inc

	LOCAL_SRC_FILES += sc8825/scale_rotate.c
	LOCAL_CFLAGS += -D_PROC_OSD_WITH_THREAD

	LOCAL_CFLAGS += -D_DMA_COPY_OSD_LAYER
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sc8810)
	LOCAL_CFLAGS += -DSCAL_ROT_TMP_BUF
	LOCAL_SRC_FILES += sc8810/scale_rotate.c
	LOCAL_CFLAGS += -D_VSYNC_USE_SOFT_TIMER
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sc7710)
	LOCAL_CFLAGS += -DSCAL_ROT_TMP_BUF
	LOCAL_SRC_FILES += sc8810/scale_rotate.c
	LOCAL_CFLAGS += -D_VSYNC_USE_SOFT_TIMER
endif

ifeq ($(strip $(USE_GPU_PROCESS_VIDEO)) , true)
	LOCAL_CFLAGS += -DTRANSFORM_USE_GPU
	LOCAL_SRC_FILES += gpu_transform.cpp
endif

ifeq ($(strip $(USE_RGB_VIDEO_LAYER)) , true)
	LOCAL_CFLAGS += -DVIDEO_LAYER_USE_RGB
endif

ifeq ($(strip $(USE_SPRD_DITHER)) , true)
	LOCAL_CFLAGS += -DSPRD_DITHER_ENABLE
endif

ifeq ($(strip $(SPRD_HWC_DEBUG_TRACE)), true)
	LOCAL_CFLAGS += -DHWC_DEBUG_TRACE
endif

ifeq ($(strip $(TARGET_DEBUG_CAMERA_SHAKE_TEST)), true)
	LOCAL_CFLAGS += -DHWC_DUMP_CAMERA_SHAKE_TEST
endif

LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)

endif

include $(call all-makefiles-under,$(LOCAL_PATH))
endif
