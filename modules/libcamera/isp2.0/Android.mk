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

ifeq ($(strip $(ISP_HW_VER)),2.0)
OEM_DIR := oem2v0

LOCAL_C_INCLUDES := \
	system/media/camera/include \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video \
	$(LOCAL_PATH)/../common/inc \
	$(LOCAL_PATH)/../vsp/inc \
	$(LOCAL_PATH)/../$(OEM_DIR)/inc \
	$(LOCAL_PATH)/../$(OEM_DIR)/isp_calibration/inc \
	$(LOCAL_PATH)/../tool/mtrace \
	$(LOCAL_PATH)/isp_app \
	$(LOCAL_PATH)/isp_tune \
	$(LOCAL_PATH)/calibration \
	$(LOCAL_PATH)/driver/inc \
	$(LOCAL_PATH)/param_manager \
	$(LOCAL_PATH)/sft_ae/inc \
	$(LOCAL_PATH)/awb/inc \
	$(LOCAL_PATH)/awb/ \
	$(LOCAL_PATH)/af/inc \
	$(LOCAL_PATH)/lsc/inc \
	$(LOCAL_PATH)/lib_ctrl/ \
	$(LOCAL_PATH)/anti_flicker/inc \
	$(LOCAL_PATH)/smart \
	$(LOCAL_PATH)/calibration/inc \
	$(LOCAL_PATH)/sft_af/inc \
  $(LOCAL_PATH)/afv1/inc \
  $(LOCAL_PATH)/aft/inc

LOCAL_SRC_DIR := $(LOCAL_PATH)

$(warning $(TARGET_BOARD_CAMERA_ISP_AE_VERSION))
$(warning $(LOCAL_PATH))


ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_AE_VERSION)),1)
LOCAL_C_INCLUDES += \
				$(LOCAL_PATH)/ae/ae1/inc
else
LOCAL_C_INCLUDES += \
				$(LOCAL_PATH)/ae/ae0/inc
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_AE_VERSION)),1)
LOCAL_SRC_FILES += \
				./ae/ae1/ae_sprd_ctrl.c \
				./ae/ae1/sensor_exposure_queue.c
else
LOCAL_SRC_FILES += \
				./ae/ae0/ae_sprd_ctrl.c \
				./ae/ae0/ae_dummy.c
endif

RM_FILE := ae_sprd_ctrl.c|sensor_exposure_queue.c|ae_dummy.c

ifeq ($(strip $(TARGET_BOARD_USE_THRID_LIB)),true)

ifeq ($(strip $(TARGET_BOARD_USE_THIRD_AWB_LIB_A)),true)
ifeq ($(strip $(TARGET_BOARD_USE_ALC_AE_AWB)),true)
	LOCAL_C_INCLUDES += \
                $(LOCAL_PATH)/$(CUR_DIR)/third_lib/alc_ip/inc \
                $(LOCAL_PATH)/$(CUR_DIR)/third_lib/alc_ip/ais/inc

	LOCAL_SRC_FILES += $(shell find $(LOCAL_SRC_DIR) -name 'ae' -prune -o -name 'backup' -prune -o -name 'alc_awb' -prune -o -name '*.c' | sed s:^$(LOCAL_PATH)/::g  )

#	example for rm some files
#	RM_FILE = "awb_al_ctrl.c awb_dummy.c ae_dummy.c af_dummy.c"
#	LOCAL_SRC_FILES += $(shell find $(LOCAL_SRC_DIR) -name 'ae' -prune -o -name 'backup' -prune -o -name '*.c' | sed s:^$(LOCAL_PATH)/::g | grep -v $(RM_FILE) )
else
	LOCAL_C_INCLUDES += \
                $(LOCAL_PATH)/$(CUR_DIR)/third_lib/alc_awb \
                $(LOCAL_PATH)/$(CUR_DIR)/third_lib/alc_awb/inc

	LOCAL_SRC_FILES += $(shell find $(LOCAL_SRC_DIR) -name 'ae' -prune -o -name 'backup' -prune -o -name 'alc_ip' -prune -o -name '*.c' | sed s:^$(LOCAL_PATH)/::g  )

endif
else
	LOCAL_SRC_FILES += $(shell find $(LOCAL_SRC_DIR) -name 'ae' -prune -o -name 'backup' -prune -o -name 'alc_awb' -prune -o -name 'alc_ip' -prune -o -name '*.c' | sed s:^$(LOCAL_PATH)/::g )
			
endif

ifeq ($(strip $(TARGET_BOARD_USE_THIRD_AF_LIB_A)),false)
	LOCAL_SRC_FILES += $(shell find $(LOCAL_SRC_DIR) -name 'ae' -prune -o -name 'backup' -prune -o -name 'alc_af' -prune -o -name '*.c' | sed s:^$(LOCAL_PATH)/::g )

else
	LOCAL_C_INCLUDES += \
               $(LOCAL_PATH)/$(CUR_DIR)/third_lib/alc_af/inc/
endif

else
	LOCAL_SRC_FILES += $(shell find $(LOCAL_SRC_DIR) -name 'ae' -prune -o -name 'backup' -prune -o -name 'third_lib' -prune -o -name '*.c' | sed s:^$(LOCAL_PATH)/::g  )

endif

LOCAL_CFLAGS += -DAE_WORK_MOD_V0 #AE_WORK_MOD_V0: Old ae algorithm + slow converge
				#AE_WORK_MOD_V1: new ae algorithm + slow converge
				#AE_WORK_MOD_V2: new ae algorithm + fast converge
				
$(warning $(LOCAL_SRC_FILES))

include $(LOCAL_PATH)/../SprdCtrl.mk

$(warning $(LOCAL_SRC_FILES))


LOCAL_MODULE := libcamisp$(ISP_HW_VER)

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := libcutils

LOCAL_SHARED_LIBRARIES += libutils libhardware libcamsensor libcamcommon

include $(LOCAL_PATH)/SprdIspSharedLib.mk
include $(BUILD_SHARED_LIBRARY)
include $(LOCAL_PATH)/SprdIspPreLib.mk

endif
