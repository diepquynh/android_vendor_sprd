# add lib

ISP_LIB_PATH := ../libs/isp2.0

ifeq ($(strip $(TARGET_BOARD_CAMERA_ISP_AE_VERSION)),1)
ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := libae_v1
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libae_v1.so
LOCAL_MODULE_STEM_64 := libae_v1.so
LOCAL_SRC_FILES_32 :=  $(ISP_LIB_PATH)/lib/libae_v1.so
LOCAL_SRC_FILES_64 :=  $(ISP_LIB_PATH)/lib64/libae_v1.so
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := $(ISP_LIB_PATH)/lib/libae_v1.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif
else
ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := libae
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libae.so
LOCAL_MODULE_STEM_64 := libae.so
LOCAL_SRC_FILES_32 :=  $(ISP_LIB_PATH)/lib/libae.so
LOCAL_SRC_FILES_64 :=  $(ISP_LIB_PATH)/lib64/libae.so
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := $(ISP_LIB_PATH)/lib/libae.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif
endif

ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := libawb libawb1
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libawb.so libawb1.so 
LOCAL_MODULE_STEM_64 := libawb.so libawb1.so
LOCAL_SRC_FILES_32 :=  $(ISP_LIB_PATH)/lib/libawb.so $(ISP_LIB_PATH)/lib/libawb1.so
LOCAL_SRC_FILES_64 :=  $(ISP_LIB_PATH)/lib64/libawb_64.so $(ISP_LIB_PATH)/lib64/libawb1_64.so
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := $(ISP_LIB_PATH)/lib/libawb.so $(ISP_LIB_PATH)/lib/libawb1.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := libcalibration
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libcalibration.so
LOCAL_MODULE_STEM_64 := libcalibration.so
LOCAL_SRC_FILES_32 :=  $(ISP_LIB_PATH)/lib/libcalibration.so
LOCAL_SRC_FILES_64 :=  $(ISP_LIB_PATH)/lib64/libcalibration_64.so
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := $(ISP_LIB_PATH)/lib/libcalibration.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := libAF
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libAF.so
LOCAL_MODULE_STEM_64 := libAF.so
LOCAL_SRC_FILES_32 :=  sft_af/lib/libAF.so
LOCAL_SRC_FILES_64 :=  sft_af/lib/libAF_64.so
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := sft_af/lib/libAF.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := libsft_af_ctrl
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libsft_af_ctrl.so
LOCAL_MODULE_STEM_64 := libsft_af_ctrl.so
LOCAL_SRC_FILES_32 :=  sft_af/lib/libsft_af_ctrl.so
LOCAL_SRC_FILES_64 :=  sft_af/lib/libsft_af_ctrl_64.so
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := sft_af/lib/libsft_af_ctrl.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := libaf_tune
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libaf_tune.so
LOCAL_MODULE_STEM_64 := libaf_tune.so
LOCAL_SRC_FILES_32 :=  sft_af/lib/libaf_tune.so
LOCAL_SRC_FILES_64 :=  sft_af/lib/libaf_tune_64.so
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := sft_af/lib/libaf_tune.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_BOARD_CAMERA_ANTI_FLICKER)),true)
ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := libdeflicker
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libdeflicker.so
LOCAL_MODULE_STEM_64 := libdeflicker.so
LOCAL_SRC_FILES_32 := $(ISP_LIB_PATH)/lib/libdeflicker.so
LOCAL_SRC_FILES_64 := $(ISP_LIB_PATH)/lib64/libdeflicker_64.so
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := $(ISP_LIB_PATH)/lib/libdeflicker.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif
endif

ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := libspaf
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libspaf.so
LOCAL_MODULE_STEM_64 := libspaf.so
LOCAL_SRC_FILES_32 := $(ISP_LIB_PATH)/lib/libspaf.so
LOCAL_SRC_FILES_64 := $(ISP_LIB_PATH)/lib64/libspaf_64.so
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := $(ISP_LIB_PATH)/lib/libspaf.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := libspafv1
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libspafv1.so
LOCAL_MODULE_STEM_64 := libspafv1.so
LOCAL_SRC_FILES_32 := $(ISP_LIB_PATH)/lib/libspafv1.so
LOCAL_SRC_FILES_64 := $(ISP_LIB_PATH)/lib64/libspafv1_64.so
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := $(ISP_LIB_PATH)/lib/libspafv1.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := libspcaftrigger
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := libspcaftrigger.so
LOCAL_MODULE_STEM_64 := libspcaftrigger.so
LOCAL_SRC_FILES_32 := $(ISP_LIB_PATH)/lib/libspcaftrigger.so
LOCAL_SRC_FILES_64 := $(ISP_LIB_PATH)/lib64/libspcaftrigger_64.so
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := $(ISP_LIB_PATH)/lib/libspcaftrigger.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)
LOCAL_MODULE := liblsc
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := both
LOCAL_MODULE_STEM_32 := liblsc.so
LOCAL_MODULE_STEM_64 := liblsc.so
LOCAL_SRC_FILES_32 := $(ISP_LIB_PATH)/lib/liblsc.so
LOCAL_SRC_FILES_64 := $(ISP_LIB_PATH)/lib64/liblsc_64.so
include $(BUILD_PREBUILT)
else
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := $(ISP_LIB_PATH)/lib/liblsc.so
LOCAL_MODULE_TAGS := optional
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_BOARD_USE_THRID_LIB)),true)
ifeq ($(strip $(TARGET_BOARD_USE_THIRD_AWB_LIB_A)),true)
ifeq ($(strip $(TARGET_BOARD_USE_ALC_AE_AWB)),false)
ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)		
LOCAL_MODULE := libAl_Awb		
LOCAL_MODULE_CLASS := SHARED_LIBRARIES		
LOCAL_MODULE_TAGS := optional		
LOCAL_MULTILIB := both		
LOCAL_MODULE_STEM_32 := libAl_Awb.so		
LOCAL_MODULE_STEM_64 := libAl_Awb_64.so		
LOCAL_SRC_FILES_32 := isp2.0/third_lib/alc_awb/libAl_Awb.so		
LOCAL_SRC_FILES_64 := isp2.0/third_lib/alc_awb/libAl_Awb_64.so		
include $(BUILD_PREBUILT)		
else		
include $(CLEAR_VARS)		
LOCAL_PREBUILT_LIBS := isp2.0/third_lib/alc_awb/libAl_Awb.so
LOCAL_MODULE_TAGS := optional		
include $(BUILD_MULTI_PREBUILT)
endif
endif
endif
endif


ifeq ($(strip $(TARGET_BOARD_USE_THRID_LIB)),true)
ifeq ($(strip $(TARGET_BOARD_USE_THIRD_AWB_LIB_A)),true)
ifeq ($(strip $(TARGET_BOARD_USE_ALC_AE_AWB)),false)
ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)		
LOCAL_MODULE := libAl_Awb_Sp		
LOCAL_MODULE_CLASS := SHARED_LIBRARIES		
LOCAL_MODULE_TAGS := optional		
LOCAL_MULTILIB := both		
LOCAL_MODULE_STEM_32 := libAl_Awb_Sp.so		
LOCAL_MODULE_STEM_64 := libAl_Awb_Sp_64.so		
LOCAL_SRC_FILES_32 := isp2.0/third_lib/alc_awb/libAl_Awb_Sp.so		
LOCAL_SRC_FILES_64 := isp2.0/third_lib/alc_awb/libAl_Awb_Sp_64.so		
include $(BUILD_PREBUILT)		
else		
include $(CLEAR_VARS)		
LOCAL_PREBUILT_LIBS := isp2.0/third_lib/alc_awb/libAl_Awb_Sp.so		
LOCAL_MODULE_TAGS := optional		
include $(BUILD_MULTI_PREBUILT)
endif
endif
endif
endif

ifeq ($(strip $(TARGET_BOARD_USE_THRID_LIB)),true)
ifeq ($(strip $(TARGET_BOARD_USE_THIRD_AF_LIB_A)),true)
ifeq ($(strip $(TARGET_ARCH)),arm64)
include $(CLEAR_VARS)		
LOCAL_MODULE := libaf_running		
LOCAL_MODULE_CLASS := SHARED_LIBRARIES		
LOCAL_MODULE_TAGS := optional		
LOCAL_MULTILIB := both		
LOCAL_MODULE_STEM_32 := libaf_running.so		
LOCAL_MODULE_STEM_64 := libaf_running.so		
LOCAL_SRC_FILES_32 := isp2.0/third_lib/alc_af/lib/libaf_running.so		
LOCAL_SRC_FILES_64 := isp2.0/third_lib/alc_af/lib/libaf_running_64.so		
include $(BUILD_PREBUILT)		
else		
include $(CLEAR_VARS)		
LOCAL_PREBUILT_LIBS := isp2.0/third_lib/alc_af/lib/libaf_running.so		
LOCAL_MODULE_TAGS := optional	
include $(BUILD_MULTI_PREBUILT)
endif
endif
endif


ifeq ($(strip $(TARGET_BOARD_USE_THRID_LIB)),true)
ifeq ($(strip $(TARGET_BOARD_USE_THIRD_AF_LIB_A)),true)
ifeq ($(strip $(TARGET_BOARD_USE_ALC_AE_AWB)),true)
	ifeq ($(strip $(TARGET_ARCH)),arm)
		include $(CLEAR_VARS)
		LOCAL_PREBUILT_LIBS := alc_ip/ais/libAl_Ais.so
		LOCAL_MODULE_TAGS := optional
		include $(BUILD_MULTI_PREBUILT)

		include $(CLEAR_VARS)
		LOCAL_PREBUILT_LIBS := alc_ip/ais/libAl_Ais_Sp.so
		LOCAL_MODULE_TAGS := optional
		include $(BUILD_MULTI_PREBUILT)
	endif

	ifeq ($(strip $(TARGET_ARCH)),arm64)
		include $(CLEAR_VARS)
		LOCAL_MODULE := libAl_Ais
		LOCAL_MODULE_CLASS := SHARED_LIBRARIES
		LOCAL_MODULE_TAGS := optional
		LOCAL_MULTILIB := both
		LOCAL_MODULE_STEM_32 := libAl_Ais.so
		LOCAL_MODULE_STEM_64 := libAl_Ais.so
		LOCAL_SRC_FILES_32 :=  alc_ip/ais/libAl_Ais.so
		LOCAL_SRC_FILES_64 :=  alc_ip/ais/libAl_Ais_64.so
		include $(BUILD_PREBUILT)

		include $(CLEAR_VARS)
		LOCAL_MODULE := libAl_Ais_Sp
		LOCAL_MODULE_CLASS := SHARED_LIBRARIES
		LOCAL_MODULE_TAGS := optional
		LOCAL_MULTILIB := both
		LOCAL_MODULE_STEM_32 := libAl_Ais_Sp.so
		LOCAL_MODULE_STEM_64 := libAl_Ais_Sp.so
		LOCAL_SRC_FILES_32 :=  alc_ip/ais/libAl_Ais_Sp.so
		LOCAL_SRC_FILES_64 :=  alc_ip/ais/libAl_Ais_Sp_64.so
		include $(BUILD_PREBUILT)
	endif
endif

endif
endif

