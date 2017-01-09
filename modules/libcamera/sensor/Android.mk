#
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
#

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -fno-strict-aliasing -Wno-unused-parameter #-Werror

ISP_HW_VER := 1.0
OEM_DIR = oem

TARGET_BOARD_CAMERA_READOTP_METHOD?=0

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_SOFTWARE_VERSION)),2)
ISP_HW_VER := 2.0
OEM_DIR = oem2v0
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_SOFTWARE_VERSION)),3)
ISP_HW_VER := 2.0
OEM_DIR = oem2v0
endif

LOCAL_C_INCLUDES := \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video \
	$(LOCAL_PATH)/../common/inc \
	$(LOCAL_PATH)/../jpeg/inc \
	$(LOCAL_PATH)/../vsp/inc \
	$(LOCAL_PATH)/../tool/mtrace \
	$(LOCAL_PATH)/utility \
	$(LOCAL_PATH)/../isp$(ISP_HW_VER)/isp_app \
	$(LOCAL_PATH)/../isp$(ISP_HW_VER)/isp_tune \
	$(LOCAL_PATH)/../isp$(ISP_HW_VER)/calibration/inc \
	$(LOCAL_PATH)/../isp$(ISP_HW_VER)/awb/inc \
	$(LOCAL_PATH)/../isp$(ISP_HW_VER)/inc \
	$(LOCAL_PATH)/../$(OEM_DIR)/isp_calibration/inc \
	$(LOCAL_PATH)/../isp$(ISP_HW_VER)/uv_denoise/inc

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../$(OEM_DIR)/inc \
	$(LOCAL_PATH)/../$(OEM_DIR)/isp_calibration/inc \
	$(LOCAL_PATH)/../$(OEM_DIR)/ydenoise_paten/inc
	
	
	                
ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_AE_VERSION)),1)
LOCAL_C_INCLUDES += \
				$(LOCAL_PATH)/../isp$(ISP_HW_VER)/ae/ae1/inc \
				$(LOCAL_PATH)/../isp$(ISP_HW_VER)/lib_ctrl

else
LOCAL_C_INCLUDES += \
				$(LOCAL_PATH)/../isp$(ISP_HW_VER)/ae/ae0/inc
				
endif   


include $(LOCAL_PATH)/../SprdCtrl.mk

ifeq ($(strip $(ISP_HW_VER)),2.0)
LOCAL_SRC_FILES += \
		af_et9714.c \
                vcm_dw9714a.c \
                af_dw9718s.c \
                ov5640/sensor_ov5640_mipi.c \
                ov5640/sensor_ov5640_mipi_raw.c \
                ov5670/sensor_ov5670_mipi_raw.c \
		ov5675/sensor_ov5675_mipi_raw.c \
                c2590/sensor_c2590_mipi_raw.c \
                gc2155/sensor_gc2155_mipi.c \
                ov8825/sensor_ov8825_mipi_raw.c \
                hi544/sensor_hi544_mipi_raw.c \
                hi255/sensor_hi255.c \
                sensor_gc0310_mipi.c \
                ov5648/sensor_ov5648_mipi_raw.c \
                ov5648_darling/sensor_ov5648_darling_mipi_raw.c \
                ov13850r2a/sensor_ov13850r2a_mipi_raw.c \
                ov8858/sensor_ov8858_mipi_raw.c \
                ov2680/sensor_ov2680_mipi_raw.c \
                s5k4h5yc/sensor_s5k4h5yc_mipi_raw.c \
                s5k5e3yx/sensor_s5k5e3yx_mipi_raw.c \
                s5k4h5yc/sensor_s5k4h5yc_mipi_raw_jsl.c \
                s5k3l2xx/sensor_s5k3l2xx_mipi_raw.c \
                imx132/sensor_imx132_mipi_raw.c \
                s5k3m2xxm3/sensor_s5k3m2xxm3_mipi_raw.c \
                imx214/sensor_imx214_mipi_raw.c \
                s5k4h5yc/packet_convert.c \
                sensor_autotest_ov5648_mipi_raw.c \
                sensor_autotest_ov5670_mipi_raw.c \
                sensor_autotest_ov13850_mipi_raw.c
else
LOCAL_SRC_FILES += \
                sensor_ov8825_mipi_raw.c \
                sensor_autotest_ov8825_mipi_raw.c\
                sensor_ov5648_mipi_raw.c \
                sensor_ov5670_mipi_raw.c \
                sensor_ov2680_mipi_raw.c \
                sensor_ov8858_mipi_raw.c \
                sensor_imx179_mipi_raw.c \
                sensor_imx219_mipi_raw.c \
                sensor_hi544_mipi_raw.c \
                sensor_ov5640_mipi.c \
                sensor_autotest_ov5640_mipi_yuv.c \
                sensor_ov5640.c \
                sensor_autotest_ov5640_ccir_yuv.c \
                sensor_autotest_ccir_yuv.c \
                sensor_JX205_mipi_raw.c \
                sensor_gc2035.c \
                sensor_gc2155.c \
                sensor_gc2155_mipi.c \
                sensor_gc0308.c \
                sensor_gc0310_mipi.c \
                sensor_hm2058.c \
                sensor_ov8865_mipi_raw.c \
                sensor_gt2005.c \
                sensor_hi702_ccir.c \
                sensor_pattern.c \
                sensor_ov7675.c\
                sensor_hi253.c\
                sensor_hi255.c\
                sensor_s5k4ecgx_mipi.c \
                sensor_sp2529_mipi.c \
                sensor_s5k4ecgx.c \
                sensor_sr352.c \
                sensor_sr352_mipi.c \
                sensor_sr030pc50_mipi.c \
                sensor_s5k4h5yb_mipi_raw.c \
                sensor_s5k5e3yx_mipi_raw.c \
                sensor_autotest_ov5670_mipi_raw.c \
                sensor_autotest_ov8858_mipi_raw.c \
                sensor_autotest_ov2680_mipi_raw.c \
                sensor_autotest_gc0310_mipi.c
endif


LOCAL_SRC_FILES += 	\
    sensor_drv_u.c \
	sensor_cfg.c \
	cmr_sensor.c

ifeq ($(strip $(ISP_HW_VER)),2.0)
LOCAL_SRC_FILES += 	\
	sensor_isp_param_awb_pac.c \
	sensor_isp_param_merge.c \
	utility/isp_param_file_update.c
endif

LOCAL_MODULE := libcamsensor

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := libcutils libcamcommon

include $(BUILD_SHARED_LIBRARY)
