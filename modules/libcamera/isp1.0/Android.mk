LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -fno-strict-aliasing -Wno-unused-parameter #-Werror

ISP_HW_VER := 1.0
OEM_DIR = oem
ISP_LIB_PATH := ../libs/isp1.0

TARGET_BOARD_CAMERA_READOTP_METHOD?=0

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_SOFTWARE_VERSION)),2)
ISP_HW_VER := 2.0
OEM_DIR = oem2v0
ISP_LIB_PATH := ../libs/isp2.0
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_SOFTWARE_VERSION)),3)
ISP_HW_VER := 2.0
OEM_DIR = oem2v0
ISP_LIB_PATH := ../libs/isp2.0
endif

ifeq ($(strip $(ISP_HW_VER)),1.0)
OEM_DIR := oem
CUR_DIR := isp1.0

LOCAL_C_INCLUDES := \
	system/media/camera/include \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video \
	$(LOCAL_PATH)/../common/inc \
	$(LOCAL_PATH)/../vsp/inc \
	$(LOCAL_PATH)/../$(OEM_DIR)/inc \
	$(LOCAL_PATH)/../$(OEM_DIR)/isp_calibration/inc \
	$(LOCAL_PATH)/../tool/mtrace \
	$(LOCAL_PATH)/inc \
    $(LOCAL_PATH)/uv_denoise/inc

LOCAL_SRC_DIR := $(LOCAL_PATH)

LOCAL_SRC_FILES += $(shell find $(LOCAL_PATH) -name '*.c' | sed s:^$(LOCAL_PATH)/::g )

include $(LOCAL_PATH)/../SprdCtrl.mk
LOCAL_MODULE := libcamisp$(ISP_HW_VER)

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := libcutils libutils libhardware libcamsensor libcamcommon

include $(LOCAL_PATH)/SprdIspSharedLib.mk
include $(BUILD_SHARED_LIBRARY)
include $(LOCAL_PATH)/SprdIspPreLib.mk

endif
