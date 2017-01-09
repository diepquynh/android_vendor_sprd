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

ifeq ($(strip $(ISP_HW_VER)),2.0)

LOCAL_C_INCLUDES := \
	$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video \
	$(LOCAL_PATH)/../common/inc \
	$(LOCAL_PATH)/../jpeg/inc \
	$(LOCAL_PATH)/../vsp/inc \
	$(LOCAL_PATH)/../isp$(ISP_HW_VER)/isp_app \
	$(LOCAL_PATH)/../isp$(ISP_HW_VER)/isp_tune \
	$(LOCAL_PATH)/../isp$(ISP_HW_VER)/ae/inc \
	$(LOCAL_PATH)/../isp$(ISP_HW_VER)/calibration/inc \
	$(LOCAL_PATH)/../isp$(ISP_HW_VER)/awb/inc \
	$(LOCAL_PATH)/../tool/mtrace \
	$(LOCAL_PATH)/../arithmetic/inc \
	$(LOCAL_PATH)/../arithmetic/facebeauty/inc \
	$(LOCAL_PATH)/../arithmetic/sprdface/inc \
	$(LOCAL_PATH)/inc \
	$(LOCAL_PATH)/isp_calibration/inc \
	$(LOCAL_PATH)/inc/ydenoise_paten

	LOCAL_SRC_FILES += $(shell find $(LOCAL_PATH) -name '*.c' | sed s:^$(LOCAL_PATH)/::g )

	LOCAL_SRC_FILES += $(shell find $(LOCAL_PATH)/../vsp/ -name '*.c' | sed s:^$(LOCAL_PATH)/::g )

	LOCAL_SRC_FILES += $(shell find $(LOCAL_PATH)/../jpeg/ -name '*.c' | sed s:^$(LOCAL_PATH)/::g )

	LOCAL_CFLAGS += -D_VSP_LINUX_ -D_VSP_

	LOCAL_CFLAGS += -DCHIP_ENDIAN_LITTLE -DJPEG_ENC

	include $(LOCAL_PATH)/../SprdCtrl.mk

	LOCAL_MODULE := libcamoem

	LOCAL_MODULE_TAGS := optional

	LOCAL_SHARED_LIBRARIES += libutils libcutils libcamsensor libcamcommon
	LOCAL_SHARED_LIBRARIES += libcamisp$(ISP_HW_VER)

	include $(LOCAL_PATH)/../SprdLib.mk

	ifeq ($(strip $(TARGET_BOARD_CAMERA_UV_DENOISE)),true)
		include $(CLEAR_VARS)
		LOCAL_PREBUILT_LIBS := ../libs/libuvdenoise.so
		LOCAL_MODULE_TAGS := optional
		include $(BUILD_MULTI_PREBUILT)
	endif

	ifeq ($(strip $(TARGET_BOARD_CAMERA_Y_DENOISE)),true)
		include $(CLEAR_VARS)
		LOCAL_PREBUILT_LIBS := ../libs/libynoise.so
		LOCAL_MODULE_TAGS := optional
		include $(BUILD_MULTI_PREBUILT)
	endif

	ifeq ($(strip $(TARGET_BOARD_CAMERA_SNR_UV_DENOISE)),true)
		include $(CLEAR_VARS)
		LOCAL_PREBUILT_LIBS := ../libs/libcnr.so
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
			LOCAL_SRC_FILES_32 :=  ../arithmetic/facebeauty/libts_face_beautify_hal.so
			LOCAL_SRC_FILES_64 :=  ../arithmetic/facebeauty/libts_face_beautify_hal_64.so
			include $(BUILD_PREBUILT)
		else
			include $(CLEAR_VARS)
			LOCAL_PREBUILT_LIBS := ../arithmetic/facebeauty/libts_face_beautify_hal.so
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
            LOCAL_SRC_FILES_32 :=  ../arithmetic/libsprd_easy_hdr.so
            LOCAL_SRC_FILES_64 :=  ../arithmetic/lib64/libsprd_easy_hdr.so
            include $(BUILD_PREBUILT)
		else
            include $(CLEAR_VARS)
	        LOCAL_PREBUILT_LIBS := ../arithmetic/libsprd_easy_hdr.so
	        LOCAL_MODULE_TAGS := optional
	        include $(BUILD_MULTI_PREBUILT)
		endif #end SPRD_LIB
	endif

# SPRD face detection
ifeq ($(strip $(TARGET_BOARD_CAMERA_FACE_DETECT)),true)
	ifeq ($(strip $(TARGET_BOARD_CAMERA_FD_LIB)),sprd)
	ifeq ($(strip $(TARGET_ARCH)),arm64)
		include $(CLEAR_VARS)
		LOCAL_MODULE := libsprdfd
		LOCAL_MODULE_CLASS := STATIC_LIBRARIES
		LOCAL_MULTILIB := both
		LOCAL_MODULE_STEM_32 := libsprdfd.a
		LOCAL_MODULE_STEM_64 := libsprdfd.a
		LOCAL_SRC_FILES_32 := ../arithmetic/sprdface/lib/armeabi-v7a/libsprdfd.a
		LOCAL_SRC_FILES_64 := ../arithmetic/sprdface/lib/arm64-v8a/libsprdfd.a
		LOCAL_MODULE_TAGS := optional
		include $(BUILD_PREBUILT)

		# SPRD face alignment library
		include $(CLEAR_VARS)
		LOCAL_MODULE := libsprdfa
		LOCAL_MODULE_CLASS := STATIC_LIBRARIES
		LOCAL_MULTILIB := both
		LOCAL_MODULE_STEM_32 := libsprdfa.a
		LOCAL_MODULE_STEM_64 := libsprdfa.a
		LOCAL_SRC_FILES_32 := ../arithmetic/sprdface/lib/armeabi-v7a/libsprdfa.a
		LOCAL_SRC_FILES_64 := ../arithmetic/sprdface/lib/arm64-v8a/libsprdfa.a
		LOCAL_MODULE_TAGS := optional
		include $(BUILD_PREBUILT)

		# SPRD face attribute recognition (smile detection) library
		include $(CLEAR_VARS)
		LOCAL_MODULE := libsprdfar
		LOCAL_MODULE_CLASS := STATIC_LIBRARIES
		LOCAL_MULTILIB := both
		LOCAL_MODULE_STEM_32 := libsprdfar.a
		LOCAL_MODULE_STEM_64 := libsprdfar.a
		LOCAL_SRC_FILES_32 := ../arithmetic/sprdface/lib/armeabi-v7a/libsprdfar.a
		LOCAL_SRC_FILES_64 := ../arithmetic/sprdface/lib/arm64-v8a/libsprdfar.a
		LOCAL_MODULE_TAGS := optional
		include $(BUILD_PREBUILT)
	else
		include $(CLEAR_VARS)
		LOCAL_MODULE := libsprdfd
		LOCAL_SRC_FILES := ../arithmetic/sprdface/lib/armeabi-v7a/libsprdfd.a
		LOCAL_MODULE_TAGS := optional
		include $(PREBUILT_STATIC_LIBRARY)

		include $(CLEAR_VARS)
		LOCAL_PREBUILT_LIBS := ../arithmetic/sprdface/lib/armeabi-v7a/libsprdfd.a
		LOCAL_MODULE_TAGS := optional
		include $(BUILD_MULTI_PREBUILT)

		# SPRD face alignment library
		include $(CLEAR_VARS)
		LOCAL_MODULE := libsprdfa
		LOCAL_SRC_FILES := ../arithmetic/sprdface/lib/armeabi-v7a/libsprdfa.a
		LOCAL_MODULE_TAGS := optional
		include $(PREBUILT_STATIC_LIBRARY)

		include $(CLEAR_VARS)
		LOCAL_PREBUILT_LIBS := ../arithmetic/sprdface/lib/armeabi-v7a/libsprdfa.a
		LOCAL_MODULE_TAGS := optional
		include $(BUILD_MULTI_PREBUILT)

		# SPRD face attribute recognition (smile detection) library
		include $(CLEAR_VARS)
		LOCAL_MODULE := libsprdfar
		LOCAL_SRC_FILES := ../arithmetic/sprdface/lib/armeabi-v7a/libsprdfar.a
		LOCAL_MODULE_TAGS := optional
		include $(PREBUILT_STATIC_LIBRARY)

		include $(CLEAR_VARS)
		LOCAL_PREBUILT_LIBS := ../arithmetic/sprdface/lib/armeabi-v7a/libsprdfar.a
		LOCAL_MODULE_TAGS := optional
		include $(BUILD_MULTI_PREBUILT)
	endif # end TARGET_ARCH
	endif # end TARGET_BOARD_CAMERA_FD_LIB
endif
endif
