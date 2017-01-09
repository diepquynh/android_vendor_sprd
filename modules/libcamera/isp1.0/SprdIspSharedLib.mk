# add shared libs
#

ifeq ($(strip $(ISP_HW_VER)),1.0)

LOCAL_SHARED_LIBRARIES += libae libawb libaf liblsc libev

ifeq ($(strip $(TARGET_BOARD_CAMERA_UV_DENOISE)),true)
LOCAL_SHARED_LIBRARIES += libuvdenoise
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_Y_DENOISE)),true)
LOCAL_SHARED_LIBRARIES += libynoise
endif

endif
