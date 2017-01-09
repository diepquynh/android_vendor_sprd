LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

sc8830like:=0
isp_use2.0:=0
TARGET_BOARD_CAMERA_HAL_VERSION:=HAL1.0
MINICAMERA?=2

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sc8830)
sc8830like=1
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),scx15)
sc8830like=1
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_SOFTWARE_VERSION)),2)
isp_use2.0=1
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_SOFTWARE_VERSION)),3)
isp_use2.0=1
endif

ifeq ($(strip $(sc8830like)),1)
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/vsp/inc	\
	$(LOCAL_PATH)/vsp/src \
	$(LOCAL_PATH)/jpeg/inc \
	$(LOCAL_PATH)/jpeg/src \
	$(LOCAL_PATH)/common/inc \
	$(LOCAL_PATH)/hal1.0/inc \
	$(LOCAL_PATH)/tool/auto_test/inc \
	$(LOCAL_PATH)/tool/mtrace \
	$(LOCAL_PATH)/oem/inc \
	$(LOCAL_PATH)/oem/isp_calibration/inc \
	$(LOCAL_PATH)/mtrace \
	external/skia/include/images \
	external/skia/include/core\
        external/jhead \
        external/sqlite/dist \
	system/media/camera/include \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/source/include/video \
	$(TOP)/vendor/sprd/open-source/libs/gralloc \
	$(TOP)/vendor/sprd/open-source/libs/mali/src/ump/include \
	$(TOP)/vendor/sprd/open-source/libs/libmemoryheapion

LOCAL_SRC_FILES:= \
	oem/src/SprdOEMCamera.c \
	hal1.0/src/SprdCameraParameters.cpp \
	oem/src/cmr_common.c \
	oem/src/cmr_oem.c \
	oem/src/cmr_setting.c \
	oem/src/cmr_mem.c \
	common/src/cmr_msg.c \
	oem/src/cmr_scale.c \
	oem/src/cmr_rotate.c \
	oem/src/cmr_grab.c \
	oem/src/jpeg_codec.c \
	oem/src/cmr_exif.c \
	oem/src/sensor_cfg.c \
	oem/src/cmr_preview.c \
	oem/src/cmr_snapshot.c \
	oem/src/cmr_sensor.c \
	oem/src/cmr_ipm.c \
	oem/src/cmr_focus.c \
	oem/src/sensor_drv_u.c \
	vsp/src/jpg_drv_sc8830.c \
	jpeg/src/jpegcodec_bufmgr.c \
	jpeg/src/jpegcodec_global.c \
	jpeg/src/jpegcodec_table.c \
	jpeg/src/jpegenc_bitstream.c \
	jpeg/src/jpegenc_frame.c \
	jpeg/src/jpegenc_header.c \
	jpeg/src/jpegenc_init.c \
	jpeg/src/jpegenc_interface.c \
	jpeg/src/jpegenc_malloc.c \
	jpeg/src/jpegenc_api.c \
	jpeg/src/jpegdec_bitstream.c \
	jpeg/src/jpegdec_frame.c \
	jpeg/src/jpegdec_init.c \
	jpeg/src/jpegdec_interface.c \
	jpeg/src/jpegdec_malloc.c \
	jpeg/src/jpegdec_dequant.c	\
	jpeg/src/jpegdec_out.c \
	jpeg/src/jpegdec_parse.c \
	jpeg/src/jpegdec_pvld.c \
	jpeg/src/jpegdec_vld.c \
	jpeg/src/jpegdec_api.c  \
	jpeg/src/exif_writer.c  \
	jpeg/src/jpeg_stream.c \
	tool/mtrace/mtrace.c \
	oem/isp_calibration/src/isp_calibration.c \
	oem/isp_calibration/src/isp_cali_interface.c

ifeq ($(strip $(isp_use2.0)),1)
include $(LOCAL_PATH)/isp2.0/isp2_0.mk
LOCAL_SRC_FILES+= \
	sensor/ov5640/sensor_ov5640_mipi.c \
	sensor/ov5640/sensor_ov5640_mipi_raw.c \
	sensor/ov5670/sensor_ov5670_mipi_raw.c \
	sensor/gc2155/sensor_gc2155_mipi.c \
	sensor/ov8825/sensor_ov8825_mipi_raw.c \
	sensor/hi544/sensor_hi544_mipi_raw.c \
	sensor/hi255/sensor_hi255.c \
	sensor/sensor_gc0310_mipi.c \
	sensor/s5k4h5yc/sensor_s5k4h5yc_mipi_raw.c \
	sensor/s5k4h5yc/sensor_s5k4h5yc_mipi_raw_jsl.c \
	sensor/s5k4h5yc/packet_convert.c \
	sensor/s5k5e3yx/sensor_s5k5e3yx_mipi_raw.c \
	oem/isp_calibration/src/port_isp2.0.c

else
include $(LOCAL_PATH)/isp1.0/isp1_0.mk
LOCAL_SRC_FILES += \
	tool/auto_test/src/SprdCameraHardware_autest_Interface.cpp \
	sensor/sensor_ov8825_mipi_raw.c \
	sensor/sensor_autotest_ov8825_mipi_raw.c\
	sensor/sensor_ov13850_mipi_raw.c \
	sensor/sensor_ov5648_mipi_raw.c \
	sensor/sensor_ov5670_mipi_raw.c \
	sensor/sensor_ov2680_mipi_raw.c \
	sensor/sensor_imx179_mipi_raw.c \
	sensor/sensor_imx219_mipi_raw.c \
	sensor/sensor_hi544_mipi_raw.c \
	sensor/sensor_ov5640_mipi.c \
	sensor/sensor_autotest_ov5640_mipi_yuv.c \
	sensor/sensor_ov5640.c \
	sensor/sensor_autotest_ov5640_ccir_yuv.c \
	sensor/sensor_autotest_ccir_yuv.c \
	sensor/sensor_JX205_mipi_raw.c \
	sensor/sensor_gc2035.c \
	sensor/sensor_gc2155.c \
	sensor/sensor_gc2155_mipi.c \
	sensor/sensor_gc0308.c \
	sensor/sensor_gc0310_mipi.c \
	sensor/sensor_hm2058.c \
	sensor/sensor_ov8865_mipi_raw.c \
	sensor/sensor_gt2005.c \
	sensor/sensor_hi702_ccir.c \
	sensor/sensor_pattern.c \
	sensor/sensor_ov7675.c\
	sensor/sensor_hi253.c\
	sensor/sensor_hi255.c\
	sensor/sensor_s5k4ecgx_mipi.c \
	sensor/sensor_sp2529_mipi.c \
	sensor/sensor_s5k4ecgx.c \
	sensor/sensor_sr352.c \
	sensor/sensor_sr352_mipi.c \
	sensor/sensor_sr030pc50_mipi.c \
	sensor/sensor_s5k4h5yb_mipi_raw.c \
	sensor/sensor_s5k5e3yx_mipi_raw.c \
	oem/isp_calibration/src/port_isp1.0.c

endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
LOCAL_SRC_FILES+= \
	oem/src/cmr_fd.c
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_HDR_CAPTURE)),true)
LOCAL_SRC_FILES+= \
	oem/src/cmr_hdr.c
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_UV_DENOISE)),true)
LOCAL_SRC_FILES+= \
	oem/src/cmr_uvdenoise.c
endif

ifneq ($(findstring HAL3,$(strip $(TARGET_BOARD_CAMERA_HAL_VERSION))),)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/hal3/inc
LOCAL_SRC_FILES+= \
        hal3/SprdCamera3Factory.cpp \
        hal3/SprdCamera3Hal.cpp \
        hal3/SprdCamera3HWI.cpp \
        hal3/SprdCamera3Channel.cpp \
	hal3/SprdCamera3Mem.cpp \
	hal3/SprdCamera3OEMIf.cpp \
	hal3/SprdCamera3Setting.cpp \
	hal3/SprdCamera3Stream.cpp \
	oem/isp_calibration/src/testdcam3.cpp
LOCAL_CFLAGS +=-DCONFIG_CAMERA_HAL_VERSION=3
else
LOCAL_C_INCLUDES += $(LOCAL_PATH)/hal1.0/inc
LOCAL_SRC_FILES += hal1.0/src/SprdCameraHardwareInterface.cpp \
	oem/isp_calibration/src/testdcam1.cpp
LOCAL_CFLAGS +=-DCONFIG_CAMERA_HAL_VERSION=1
endif


LOCAL_CFLAGS += -fno-strict-aliasing -D_VSP_ -DJPEG_ENC -D_VSP_LINUX_ -DCHIP_ENDIAN_LITTLE -DCONFIG_CAMERA_2M -DANDROID_4100

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_SOFTWARE_VERSION)),0)
LOCAL_CFLAGS += -DCONFIG_SP7731GEA_BOARD
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_SOFTWARE_VERSION)),1)
LOCAL_CFLAGS += -DCONFIG_SP9630EA_BOARD
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_SOFTWARE_VERSION)),2)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ISP_VERSION_V3
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),scx15)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SMALL_PREVSIZE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FLASH_CTRL)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FLASH_CTRL
endif

ifeq ($(strip $(CAMERA_SUPPORT_SIZE)),13M)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_13M
endif

ifeq ($(strip $(TARGET_BOARD_DISABLE_1080P_RECORDING_ZOOM)),true)
LOCAL_CFLAGS += -DCONFIG_DISABLE_1080P_RECORDING_ZOOM
endif


ifeq ($(strip $(CAMERA_SUPPORT_SIZE)),8M)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_8M
endif

ifeq ($(strip $(CAMERA_SUPPORT_SIZE)),5M)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_5M
endif

ifeq ($(strip $(CAMERA_SUPPORT_SIZE)),3M)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_3M
endif

ifeq ($(strip $(CAMERA_SUPPORT_SIZE)),2M)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_2M
endif

ifeq ($(strip $(FRONT_CAMERA_SUPPORT_SIZE)),3M)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_SUPPORT_3M
endif

ifeq ($(strip $(TARGET_BOARD_NO_FRONT_SENSOR)),true)
LOCAL_CFLAGS += -DCONFIG_DCAM_SENSOR_NO_FRONT_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),sc8830)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ISP
endif

LOCAL_CFLAGS += -DCONFIG_CAMERA_PREVIEW_YV12

ifeq ($(strip $(TARGET_BOARD_CAMERA_CAPTURE_MODE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ZSL_CAPTURE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ANDROID_ZSL_MODE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ANDROID_ZSL_CAPTURE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_CAF)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_CAF
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ROTATION_CAPTURE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ROTATION_CAPTURE
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_ROTATION)),true)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_ROTATION
endif

ifeq ($(strip $(TARGET_BOARD_BACK_CAMERA_ROTATION)),true)
LOCAL_CFLAGS += -DCONFIG_BACK_CAMERA_ROTATION
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ROTATION)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ROTATION
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ANTI_SHAKE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ANTI_SHAKE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_DMA_COPY)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_DMA_COPY
endif

ifeq ($(strip $(TARGET_BOARD_BACK_CAMERA_INTERFACE)),mipi)
LOCAL_CFLAGS += -DCONFIG_BACK_CAMERA_MIPI
endif

ifeq ($(strip $(TARGET_BOARD_BACK_CAMERA_INTERFACE)),ccir)
LOCAL_CFLAGS += -DCONFIG_BACK_CAMERA_CCIR
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_INTERFACE)),mipi)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_MIPI
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_INTERFACE)),ccir)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_CCIR
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_SUPPORT_720P)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_720P
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_SUPPORT_CIF)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_CIF
endif

ifeq ($(strip $(CAMERA_DISP_ION)),true)
LOCAL_CFLAGS += -DUSE_ION_MEM
endif

ifeq ($(strip $(CAMERA_SENSOR_OUTPUT_ONLY)),true)
LOCAL_CFLAGS += -DCONFIG_SENSOR_OUTPUT_ONLY
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FACE_DETECT
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_HDR_CAPTURE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_HDR_CAPTURE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_UV_DENOISE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_UV_DENOISE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_NO_FLASH_DEV)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FLASH_NOT_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_NO_AUTOFOCUS_DEV)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_NO_720P_PREVIEW)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_NO_720P_PREVIEW
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ADAPTER_IMAGE)),180)
LOCAL_CFLAGS += -DCONFIG_CAMERA_IMAGE_180
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_PRE_ALLOC_CAPTURE_MEM)),true)
LOCAL_CFLAGS += -DCONFIG_PRE_ALLOC_CAPTURE_MEM
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_MIPI)),phya)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_MIPI_PHYA
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_MIPI)),phyb)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_MIPI_PHYB
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_MIPI)),phyc)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_MIPI_PHYC
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_MIPI)),phyab)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_MIPI_PHYAB
endif

ifeq ($(strip $(TARGET_BOARD_BACK_CAMERA_MIPI)),phya)
LOCAL_CFLAGS += -DCONFIG_BACK_CAMERA_MIPI_PHYA
endif

ifeq ($(strip $(TARGET_BOARD_BACK_CAMERA_MIPI)),phyb)
LOCAL_CFLAGS += -DCONFIG_BACK_CAMERA_MIPI_PHYB
endif

ifeq ($(strip $(TARGET_BOARD_BACK_CAMERA_MIPI)),phyab)
LOCAL_CFLAGS += -DCONFIG_BACK_CAMERA_MIPI_PHYAB
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_CCIR_PCLK)),source0)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_CCIR_PCLK_SOURCE0
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_CCIR_PCLK)),source1)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_CCIR_PCLK_SOURCE1
endif

ifeq ($(strip $(TARGET_BOARD_BACK_CAMERA_CCIR_PCLK)),source0)
LOCAL_CFLAGS += -DCONFIG_BACK_CAMERA_CCIR_PCLK_SOURCE0
endif

ifeq ($(strip $(TARGET_BOARD_BACK_CAMERA_CCIR_PCLK)),source1)
LOCAL_CFLAGS += -DCONFIG_BACK_CAMERA_CCIR_PCLK_SOURCE1
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_CAPTURE_DENOISE)),true)
LOCAL_CFLAGS += -DCONFIG_CAPTURE_DENOISE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_NO_EXPOSURE_METERING)),true)
LOCAL_CFLAGS += -DCONFIG_EXPOSURE_METERING_NOT_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISO_NOT_SUPPORT)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ISO_NOT_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_LOW_CAPTURE_MEM)),true)
LOCAL_CFLAGS += -DCONFIG_LOW_CAPTURE_MEM
endif

ifeq ($(strip $(TARGET_BOARD_IS_SC_FPGA)),true)
LOCAL_CFLAGS += -DSC_FPGA=1
else
LOCAL_CFLAGS += -DSC_FPGA=0
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_HAL_VERSION)),HAL3.0)
LOCAL_CFLAGS += -DCONFIG_CAMERA_DIGIT_ZOOM_INFO
LOCAL_CFLAGS += -DCONFIG_CAMERA_HAL3_FEATURE
endif

ifdef CAMERA_SENSOR_TYPE_BACK
LOCAL_CFLAGS += -DCAMERA_SENSOR_TYPE_BACK=\"$(CAMERA_SENSOR_TYPE_BACK)\"
else
LOCAL_CFLAGS += -DCAMERA_SENSOR_TYPE_BACK=\"\\0\"
endif
ifdef CAMERA_SENSOR_TYPE_FRONT
LOCAL_CFLAGS += -DCAMERA_SENSOR_TYPE_FRONT=\"$(CAMERA_SENSOR_TYPE_FRONT)\"
else
LOCAL_CFLAGS += -DCAMERA_SENSOR_TYPE_FRONT=\"\\0\"
endif
ifdef AT_CAMERA_SENSOR_TYPE_BACK
LOCAL_CFLAGS += -DAT_CAMERA_SENSOR_TYPE_BACK=\"$(AT_CAMERA_SENSOR_TYPE_BACK)\"
else
LOCAL_CFLAGS += -DAT_CAMERA_SENSOR_TYPE_BACK=\"\\0\"
endif
ifdef AT_CAMERA_SENSOR_TYPE_FRONT
LOCAL_CFLAGS += -DAT_CAMERA_SENSOR_TYPE_FRONT=\"$(AT_CAMERA_SENSOR_TYPE_FRONT)\"
else
LOCAL_CFLAGS += -DAT_CAMERA_SENSOR_TYPE_FRONT=\"\\0\"
endif


ifeq ($(strip $(MINICAMERA)),2)
LOCAL_CFLAGS += -DMINICAMERA=2
else
LOCAL_CFLAGS += -DMINICAMERA=1
endif

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE := testdcam
LOCAL_MODULE_TAGS := optional

#LOCAL_SHARED_LIBRARIES := libandroidfw libexif libutils libbinder libcamera_client libskia libcutils libhardware libisp libuvdenoise libcamera_metadata
LOCAL_SHARED_LIBRARIES :=  libutils libmemoryheapion libbinder libcamera_client libcutils libhardware libcamera_metadata
LOCAL_SHARED_LIBRARIES += libui

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
LOCAL_SHARED_LIBRARIES += libface_finder
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_HDR_CAPTURE)),true)
	#default use sprd hdr
	LOCAL_CFLAGS += -DCONFIG_SPRD_HDR_LIB
	LOCAL_SHARED_LIBRARIES += libsprd_easy_hdr
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_UV_DENOISE)),true)
LOCAL_SHARED_LIBRARIES += libuvdenoise
endif

ifeq ($(strip $(isp_use2.0)),1)
LOCAL_SHARED_LIBRARIES += libae libAF libsft_af_ctrl libaf_tune libspaf libawb liblsc libcalibration
else
LOCAL_SHARED_LIBRARIES += libisp
endif


include $(BUILD_EXECUTABLE)

endif  #sc8830like  end
