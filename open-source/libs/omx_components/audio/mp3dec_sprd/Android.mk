LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
        SPRDMP3Decoder.cpp

LOCAL_C_INCLUDES := \
          frameworks/av/media/libstagefright/include \
	  frameworks/av/media/libstagefright/codecs/mp3dec_sprd   \
	  frameworks/av/include/media/stagefright \
	  $(TOP)/vendor/sprd/open-source/libs/libstagefrighthw/include \
	  $(TOP)/vendor/sprd/open-source/libs/libmemoryheapion \
	  $(TOP)/vendor/sprd/open-source/libs/libstagefrighthw/include/openmax

LOCAL_CFLAGS := -DOSCL_EXPORT_REF= -DOSCL_IMPORT_REF=

LOCAL_LDFLAGS += -Wl,--no-warn-shared-textrel

LOCAL_SHARED_LIBRARIES := \
          libstagefright libstagefright_omx libstagefright_foundation libstagefrighthw libutils libui libmemoryheapion libdl libcutils liblog 


LOCAL_MODULE := libstagefright_sprd_mp3dec
LOCAL_MODULE_TAGS := optional
LOCAL_32_BIT_ONLY := true

include $(BUILD_SHARED_LIBRARY)
