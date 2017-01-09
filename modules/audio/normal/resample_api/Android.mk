LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES += vendor/sprd/modules/resampler\
			system/media/audio_utils/include

LOCAL_SHARED_LIBRARIES := liblog libc libcutils liblog libutils

LOCAL_32_BIT_ONLY := true
LOCAL_LDFLAGS += -Wl,--no-warn-shared-textrel
LOCAL_STATIC_LIBRARIES := libaudioresamplersprd

LOCAL_SRC_FILES += sprd_resample_48kto44k.c

LOCAL_MODULE := libresample48kto44k

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

