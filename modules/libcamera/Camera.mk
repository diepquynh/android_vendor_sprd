LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ISP_HW_VER := 1.0
OEM_DIR := oem

TARGET_BOARD_CAMERA_READOTP_METHOD?=0

LOCAL_CFLAGS :=

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_SOFTWARE_VERSION)),2)
ISP_HW_VER := 2.0
OEM_DIR := oem2v0
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_SOFTWARE_VERSION)),3)
ISP_HW_VER := 2.0
OEM_DIR := oem2v0
endif

ISP_LIB_PATH := libs/isp$(ISP_HW_VER)

#face detect lib
TARGET_BOARD_CAMERA_FD_LIB := sprd

LOCAL_C_INCLUDES := \
                external/skia/include/images \
                external/skia/include/core\
                external/jhead \
                external/sqlite/dist \
                system/media/camera/include \
                $(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video \
                $(TOP)/vendor/sprd/modules/libmemion

LOCAL_C_INCLUDES += $(GPU_GRALLOC_INCLUDES)

include $(shell find $(LOCAL_PATH) -name 'Sprdroid.mk')

LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_CFLAGS += -fno-strict-aliasing -Wno-unused-parameter

include $(LOCAL_PATH)/SprdCtrl.mk

LOCAL_MODULE := camera.$(TARGET_BOARD_PLATFORM)

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := libutils libmemion libcamera_client libcutils libhardware libcamera_metadata libdl
LOCAL_SHARED_LIBRARIES += libui libbinder
LOCAL_SHARED_LIBRARIES += libcamsensor libcamcommon

LOCAL_SHARED_LIBRARIES += libcamisp$(ISP_HW_VER)

include $(LOCAL_PATH)/SprdLib.mk


# [Vendor_40/40]
  include $(CLEAR_VARS)
  LOCAL_SRC_FILES:= isp_guard/set_proc_priority.c
  LOCAL_MODULE := ispguard
  LOCAL_SHARED_LIBRARIES := \
      liblog
  TARGET_SYSTEM_OUT_BIN := $(PRODUCT_OUT)/system/bin
  LOCAL_MODULE_PATH := $(TARGET_SYSTEM_OUT_BIN)
  LOCAL_MODULE_TAGS := optional
  include $(BUILD_EXECUTABLE)
