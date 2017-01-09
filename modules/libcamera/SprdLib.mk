# add lib

# include $(shell find $(LOCAL_PATH) -name 'SprdSharedLib.mk')

ifeq ($(strip $(TARGET_BOARD_CAMERA_UV_DENOISE)),true)
LOCAL_SHARED_LIBRARIES += libuvdenoise
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_Y_DENOISE)),true)
LOCAL_SHARED_LIBRARIES += libynoise
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_SNR_UV_DENOISE)),true)
LOCAL_SHARED_LIBRARIES += libcnr
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_BEAUTY)),false)
else
LOCAL_SHARED_LIBRARIES += libts_face_beautify_hal
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_HDR_CAPTURE)),true)
    #default use sprd hdr
	LOCAL_CFLAGS += -DCONFIG_SPRD_HDR_LIB
	LOCAL_SHARED_LIBRARIES += libsprd_easy_hdr

endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
	ifeq ($(strip $(TARGET_BOARD_CAMERA_FD_LIB)),sprd)
		LOCAL_STATIC_LIBRARIES += libsprdfd libsprdfa libsprdfar
	endif
endif

######################################################
include $(BUILD_SHARED_LIBRARY)
######################################################
#include $(shell find $(LOCAL_PATH) -name 'SprdPreLib.mk')
