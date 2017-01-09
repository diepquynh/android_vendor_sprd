# add lib


include $(shell find $(LOCAL_PATH) -name 'SprdSharedLib.mk')

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
	ifeq ($(strip $(TARGET_BOARD_CAMERA_FD_LIB)),omron)
		LOCAL_STATIC_LIBRARIES +=libeUdnDt libeUdnCo
	endif
endif






######################################################
include $(BUILD_SHARED_LIBRARY)
######################################################

include $(shell find $(LOCAL_PATH) -name 'SprdPreLib.mk')


ifeq ($(strip $(TARGET_BOARD_CAMERA_UV_DENOISE)),true)
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := libs/libuvdenoise.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif


ifeq ($(strip $(TARGET_BOARD_CAMERA_Y_DENOISE)),true)
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := libs/libynoise.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif


ifeq ($(strip $(TARGET_BOARD_CAMERA_SNR_UV_DENOISE)),true)
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := libs/libcnr.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif


ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_BEAUTY)),false)
else
ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := libts_face_beautify_hal
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libts_face_beautify_hal.so
LOCAL_MODULE_STEM_64 := libts_face_beautify_hal.so
LOCAL_SRC_FILES_32 :=  arithmetic/facebeauty/libts_face_beautify_hal.so
LOCAL_SRC_FILES_64 :=  arithmetic/facebeauty/libts_face_beautify_hal_64.so
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := arithmetic/facebeauty/libts_face_beautify_hal.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif
endif


ifeq ($(strip $(TARGET_BOARD_CAMERA_HDR_CAPTURE)),true)
	ifeq ($(strip $(TARGET_ARCH)),arm64)
		include $(CLEAR_VARS)
		LOCAL_MODULE := libsprd_easy_hdr
		LOCAL_MODULE_CLASS := SHARED_LIBRARIES
		LOCAL_MODULE_TAGS := optional
		LOCAL_MULTILIB := both
		LOCAL_MODULE_STEM_32 := libsprd_easy_hdr.so
		LOCAL_MODULE_STEM_64 := libsprd_easy_hdr.so
		LOCAL_SRC_FILES_32 :=  arithmetic/libsprd_easy_hdr.so
		LOCAL_SRC_FILES_64 :=  arithmetic/lib64/libsprd_easy_hdr.so
		include $(BUILD_PREBUILT)
	else
		include $(CLEAR_VARS)
		LOCAL_PREBUILT_LIBS := arithmetic/libsprd_easy_hdr.so
		LOCAL_MODULE_TAGS := optional
		include $(BUILD_MULTI_PREBUILT)
	endif #end SPRD_LIB
endif


ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
	ifeq ($(strip $(TARGET_BOARD_CAMERA_FD_LIB)),omron)
	ifeq ($(strip $(TARGET_ARCH)),arm64)
		include $(CLEAR_VARS)
		LOCAL_MODULE := libeUdnDt
		LOCAL_MODULE_CLASS := STATIC_LIBRARIES
		LOCAL_MULTILIB := both
		LOCAL_MODULE_STEM_32 := libeUdnDt.a
		LOCAL_MODULE_STEM_64 := libeUdnDt.a
		LOCAL_SRC_FILES_32 := arithmetic/omron/lib32/libeUdnDt.a
		LOCAL_SRC_FILES_64 := arithmetic/omron/lib64/libeUdnDt.a
		LOCAL_MODULE_TAGS := optional
		include $(BUILD_PREBUILT)

		include $(CLEAR_VARS)
		LOCAL_MODULE := libeUdnCo
		LOCAL_MODULE_CLASS := STATIC_LIBRARIES
		LOCAL_MULTILIB := both
		LOCAL_MODULE_STEM_32 := libeUdnCo.a
		LOCAL_MODULE_STEM_64 := libeUdnCo.a
		LOCAL_SRC_FILES_32 := arithmetic/omron/lib32/libeUdnCo.a
		LOCAL_SRC_FILES_64 := arithmetic/omron/lib64/libeUdnCo.a
		LOCAL_MODULE_TAGS := optional
		include $(BUILD_PREBUILT)
	else
		include $(CLEAR_VARS)
		LOCAL_MODULE := libeUdnDt
		LOCAL_SRC_FILES := arithmetic/omron/lib32/libeUdnDt.a
		LOCAL_MODULE_TAGS := optional
		include $(PREBUILT_STATIC_LIBRARY)

		include $(CLEAR_VARS)
		LOCAL_MODULE := libeUdnCo
		LOCAL_SRC_FILES := arithmetic/omron/lib32/libeUdnCo.a
		LOCAL_MODULE_TAGS := optional
		include $(PREBUILT_STATIC_LIBRARY)

		include $(CLEAR_VARS)
		LOCAL_PREBUILT_LIBS := arithmetic/omron/lib32/libeUdnDt.a
		LOCAL_MODULE_TAGS := optional
		include $(BUILD_MULTI_PREBUILT)

		include $(CLEAR_VARS)
		LOCAL_PREBUILT_LIBS := arithmetic/omron/lib32/libeUdnCo.a
		LOCAL_MODULE_TAGS := optional
		include $(BUILD_MULTI_PREBUILT)
	endif # end TARGET_ARCH
	endif # fd_lib
endif



######################################################

