# add shared libs
#

ifeq ($(strip $(ISP_HW_VER)),2.0)
	
LOCAL_SHARED_LIBRARIES += libspaf libawb libawb1 liblsc libcalibration
LOCAL_SHARED_LIBRARIES += libAF libsft_af_ctrl libaf_tune

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_AE_VERSION)),1)
LOCAL_SHARED_LIBRARIES += libae_v1
else
LOCAL_SHARED_LIBRARIES += libae
endif

LOCAL_SHARED_LIBRARIES += libspafv1
LOCAL_SHARED_LIBRARIES += libspcaftrigger

ifeq ($(strip $(TARGET_BOARD_USE_THRID_LIB)),true)

ifeq ($(strip $(TARGET_BOARD_USE_THIRD_AF_LIB_A)),true)
LOCAL_SHARED_LIBRARIES += libaf_running
endif

ifeq ($(strip $(TARGET_BOARD_USE_THIRD_AWB_LIB_A)),true)

ifeq ($(strip $(TARGET_BOARD_USE_ALC_AE_AWB)),true)
LOCAL_CFLAGS += -DCONFIG_USE_ALC_AE
LOCAL_CFLAGS += -DCONFIG_USE_ALC_AWB
LOCAL_SHARED_LIBRARIES += libAl_Ais libAl_Ais_Sp
else
LOCAL_CFLAGS += -DCONFIG_USE_ALC_AWB
endif

endif

endif

ifeq ($(strip $(TARGET_ARCH)),arm)
LOCAL_SHARED_LIBRARIES += libdeflicker
endif

ifeq ($(strip $(TARGET_ARCH)),arm64)
ifeq ($(strip $(TARGET_BOARD_CAMERA_ANTI_FLICKER)),true)
LOCAL_SHARED_LIBRARIES += libdeflicker
endif
endif

endif
