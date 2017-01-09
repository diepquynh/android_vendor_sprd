ifeq ($(strip $(BOARD_USES_ALSA_AUDIO)),true)

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CFLAGS := \
	-fPIC -D_POSIX_SOURCE \
	-DALSA_CONFIG_DIR=\"/system/usr/share/alsa\" \
	-DALSA_PLUGIN_DIR=\"/system/usr/lib/alsa-lib\" \
	-DALSA_DEVICE_DIRECTORY=\"/dev/snd/\"

LOCAL_MODULE:= utest_audio
LOCAL_MODULE_TAGS:= debug
LOCAL_MODULE_PATH:= $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_SRC_FILES:= utest_audio.c
LOCAL_C_INCLUDES:= vendor/sprd/open-source/libs/audio/vbc/alsa-lib/include

LOCAL_SHARED_LIBRARIES:= libasound libaudio
include $(BUILD_EXECUTABLE)

endif
