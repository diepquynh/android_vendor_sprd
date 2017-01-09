
#ifeq ($(strip $(NXP_SMARTPA_SUPPORT)),tfa9890)

LOCAL_PATH:= $(call my-dir)


############################## libtfa9890_interface
include $(CLEAR_VARS)
LOCAL_CFLAGS += -DNXP_TFA9890_SUPPORT
LOCAL_C_INCLUDES:=  $(LOCAL_PATH)/tfa9890 \
                    $(LOCAL_PATH)/tfa9890/hal/inc \
                    $(LOCAL_PATH)/tfa9890/tfa/inc \
                    $(LOCAL_PATH)/tfa9890/app/exTfa98xx/inc \
                    $(LOCAL_PATH)/tfa9890/srv/inc

LOCAL_SRC_FILES := 	tfa9890/tfa9890_interface.c \
                    tfa9890/app/exTfa98xx/src/common.c

LOCAL_MODULE := libtfa9890_interface
LOCAL_SHARED_LIBRARIES:= libcutils libutils
LOCAL_STATIC_LIBRARIES:= libtfasrv libtfa98xx libtfahal
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)

###############################################

include $(call all-makefiles-under,$(LOCAL_PATH))

#endif

ifeq ($(NXP_SMARTPA_SUPPORT),yes)

LOCAL_PATH:= $(call my-dir)

############################## libtfa9887_interface
include $(CLEAR_VARS)
LOCAL_CFLAGS += -DNXP_TFA9887_SUPPORT
LOCAL_C_INCLUDES:= \
    $(LOCAL_PATH)/tfa9887/interface \
    $(LOCAL_PATH)/tfa9887/climax/inc \
    $(LOCAL_PATH)/tfa9887/climax/src/lxScribo \
    $(LOCAL_PATH)/tfa9887/climax/src/lxScribo/scribosrv \
    $(LOCAL_PATH)/tfa9887/Tfa98xxhal/inc \
    $(LOCAL_PATH)/tfa9887/Tfa98xxhal/src/lxScribo \
    $(LOCAL_PATH)/tfa9887/Tfa98xxhal/inc \
    $(LOCAL_PATH)/tfa9887/Tfa98xx/inc \
    $(LOCAL_PATH)/tfa9887/Tfa98xx/src

LOCAL_SRC_FILES := 	tfa9887/interface/tfa9887_interface.c

LOCAL_MODULE := libtfa9887_interface
LOCAL_SHARED_LIBRARIES:= libcutils libutils
LOCAL_STATIC_LIBRARIES:= libtfa9887 liblxScribo9887
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)

###############################################

include $(call all-makefiles-under,$(LOCAL_PATH))

endif

