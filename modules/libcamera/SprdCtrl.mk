# add ctrl macro

ifeq ($(strip $(TARGET_BOARD_CAMERA_HAL_VERSION)),1.0)
	LOCAL_CFLAGS += -DCONFIG_CAMERA_HAL_VERSION_1
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_READOTP_TO_ISP)),true)
LOCAL_CFLAGS += -DCONFIG_READOTP_TO_ISP
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_READOTP_TO_ISP)),true)
LOCAL_CFLAGS += -DCONFIG_READOTP_TO_ISP
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_SOFTWARE_VERSION)),0)
	LOCAL_CFLAGS += -DCONFIG_SP7731GEA_BOARD
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_SOFTWARE_VERSION)),1)
	LOCAL_CFLAGS += -DCONFIG_SP9630EA_BOARD
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_SOFTWARE_VERSION)),2)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ISP_VERSION_V3
ifeq ($(strip $(TARGET_BOARD_CAMERA_FLASH_HIGH_AE_MEASURE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FLASH_HIGH_AE_MEASURE
endif
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_SOFTWARE_VERSION)),3)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ISP_VERSION_V4
ifeq ($(strip $(TARGET_BOARD_CAMERA_FLASH_HIGH_AE_MEASURE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FLASH_HIGH_AE_MEASURE
endif
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_AE_VERSION)),1)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ISP_AE_VERSION_V1
else
LOCAL_CFLAGS += -DCONFIG_CAMERA_ISP_AE_VERSION_V0
endif

ifeq ($(strip $(TARGET_BOARD_NO_FRONT_SENSOR)),true)
	LOCAL_CFLAGS += -DCONFIG_DCAM_SENSOR_NO_FRONT_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_SENSOR_DEV_2)),true)
LOCAL_CFLAGS += -DCONFIG_DCAM_SENSOR_DEV_2_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_NO_USE_ISP)),true)
else
LOCAL_CFLAGS += -DCONFIG_CAMERA_ISP
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_CAPTURE_MODE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_ZSL_CAPTURE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_BIG_PREVIEW_AND_RECORD_SIZE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_BIG_PREVIEW_RECORD_SIZE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FORCE_ZSL_MODE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FORCE_ZSL_CAPTURE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ANTI_FLICKER)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_AFL_AUTO_DETECTION
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_AF_ALG_SPRD)),true)
LOCAL_CFLAGS += -DCONFIG_NEW_AF
else
LOCAL_CFLAGS += -DCONFIG_SFT_AF
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

ifeq ($(strip $(TARGET_BOARD_PRE_REC_MAXSIZE_1280X960)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_1280X960
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_PRE_REC_MAXSIZE_1280X960)),true)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_SUPPORT_1280X960
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_SUPPORT_720P)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_720P
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_SUPPORT_720P)),true)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_SUPPORT_720P
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_SUPPORT_1080P)),true)
LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_SUPPORT_1080P
endif

ifeq ($(strip $(TARGET_BOARD_DISABLE_1080P_RECORDING_ZOOM)),true)
LOCAL_CFLAGS += -DCONFIG_DISABLE_1080P_RECORDING_ZOOM
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FACE_DETECT
ifeq ($(strip $(TARGET_BOARD_CAMERA_FD_LIB)),sprd)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FACE_DETECT_SPRD
endif
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_HDR_CAPTURE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_HDR_CAPTURE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_UV_DENOISE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_UV_DENOISE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_Y_DENOISE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_Y_DENOISE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_SNR_UV_DENOISE)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_SNR_UV_DENOISE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_NO_FLASH_DEV)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FLASH_NOT_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_FRONT_CAMERA_FLASH_DEV)),true)
LOCAL_CFLAGS += -DCONFIG_FRONT_FLASH_SUPPORT
endif

ifneq ($(strip $(TARGET_BOARD_CAMERA_AUTOFOCUS)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_AUTOFOCUS_NOT_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_NO_720P_PREVIEW)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_NO_720P_PREVIEW
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ADAPTER_IMAGE)),180)
LOCAL_CFLAGS += -DCONFIG_CAMERA_IMAGE_180
endif

ifeq ($(strip $(TARGET_VCM_BU64241GWZ)),true)
LOCAL_CFLAGS += -DCONFIG_VCM_BU64241GWZ
endif

$(info we wanna del -DCONFIG_CAMERA_IMAGE_180 and -DCONFIG_VCM_BU64241GWZ)

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

ifeq ($(strip $(TARGET_BOARD_CAMERA_CAPTURE_DENOISE)),true)
LOCAL_CFLAGS += -DCONFIG_CAPTURE_DENOISE
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_NO_EXPOSURE_METERING)),true)
LOCAL_CFLAGS += -DCONFIG_EXPOSURE_METERING_NOT_SUPPORT
endif

ifeq ($(strip $(TARGET_BOARD_LOW_CAPTURE_MEM)),true)
LOCAL_CFLAGS += -DCONFIG_LOW_CAPTURE_MEM
endif

ifeq ($(strip $(TARGET_BOARD_MULTI_CAP_MEM)),true)
LOCAL_CFLAGS += -DCONFIG_MULTI_CAP_MEM
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FULL_SCREEN_DISPLAY)),true)
LOCAL_CFLAGS += -DCONFIG_CAMERA_FULL_SCREEN_DISPLAY
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
ifdef CAMERA_SENSOR_TYPE_DEV_2
LOCAL_CFLAGS += -DCAMERA_SENSOR_TYPE_DEV_2=\"$(CAMERA_SENSOR_TYPE_DEV_2)\"
else
LOCAL_CFLAGS += -DCAMERA_SENSOR_TYPE_DEV_2=\"\\0\"
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
ifdef AT_CAMERA_SENSOR_TYPE_DEV_2
LOCAL_CFLAGS += -DAT_CAMERA_SENSOR_TYPE_DEV_2=\"$(AT_CAMERA_SENSOR_TYPE_DEV_2)\"
else
LOCAL_CFLAGS += -DAT_CAMERA_SENSOR_TYPE_DEV_2=\"\\0\"
endif

ifeq ($(strip $(TARGET_GPU_PLATFORM)),midgard)
LOCAL_CFLAGS += -DCONFIG_GPU_MIDGARD
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_SPRD_PRIVATE_ZSL)),true)
LOCAL_CFLAGS += -DCONFIG_SPRD_PRIVATE_ZSL
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_BEAUTY)),false)
else
LOCAL_CFLAGS += -DCONFIG_FACE_BEAUTY
endif

ifeq ($(strip $(TARGET_FORCE_PASS_FLEXIBLEYUV_IN_DARK)),true)
LOCAL_CFLAGS += -DFORCE_PASS_FLEXIBLEYUV_IN_DARK
endif

LOCAL_CFLAGS += -DCONFIG_READOTP_METHOD=$(TARGET_BOARD_CAMERA_READOTP_METHOD)

LOCAL_CFLAGS += -DCONFIG_CAMERA_SUPPORT_$(CAMERA_SUPPORT_SIZE)

LOCAL_CFLAGS += -DCONFIG_FRONT_CAMERA_SUPPORT_$(FRONT_CAMERA_SUPPORT_SIZE)

LOCAL_CFLAGS += -DCONFIG_CAMERA_DEV_2_SUPPORT_$(CAMERA_DEV_2_SUPPORT_SIZE)
