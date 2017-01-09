# Copyright (C) 2011 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

ifeq ($(strip $(USE_AUDIO_WHALE_HAL)),true)
ifeq ($(strip $(BOARD_USES_TINYALSA_AUDIO)),true)

LOCAL_PATH := $(call my-dir)

#TinyAlsa audio

include $(CLEAR_VARS)


LOCAL_MODULE := audio.primary.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_32_BIT_ONLY := true
LOCAL_CFLAGS := -D_POSIX_SOURCE -Wno-multichar -g

ifeq ($(TARGET_BUILD_VARIANT),userdebug)
LOCAL_CFLAGS += -DAUDIO_DEBUG
endif

ifeq ($(strip $(BOARD_USES_SS_VOIP)), true)
# Default case, Nothing to do.
else
LOCAL_CFLAGS += -DVOIP_DSP_PROCESS
endif

ifneq ($(filter $(strip $(PLATFORM_VERSION)),5.0 5.1),)
 LOCAL_CFLAGS += -DANDROID_VERSIO_5_X
endif

LOCAL_CFLAGS += -DLOCAL_SOCKET_SERVER

LOCAL_C_INCLUDES += \
	external/tinyalsa/include \
	external/expat/lib \
	system/media/audio_utils/include \
	system/media/audio_effects/include \
	$(LOCAL_PATH)/record_process \
	$(LOCAL_PATH)/debug \
	$(LOCAL_PATH)/record_nr \
	$(LOCAL_PATH)/audio_pram \
	external/tinyxml \
	external/tinycompress/include


LOCAL_SRC_FILES := audio_hw.c audio_monitor.c audio_control.cpp tinyalsa_util.cpp debug/audio_debug.cpp \
				audio_xml_utils.cpp audio_param/audio_param.cpp audio_param/dsp_control.c \
				agdsp.c \
				record_process/aud_proc_config.c.arm record_process/aud_filter_calc.c.arm \
				fm.c debug/ext_control.c debug/audio_hw_system_test.cpp debug/audio_hw_dev_test.c \
				audio_parse.cpp voice_call.c  audio_offload.c audio_register.c ring_buffer.c \
				audiotester/audiotester_server.c audiotester/audio_param_handler.cpp debug/dsp_loop.c \
                                record_process/record_nr_api.c audiotester/local_socket.c pipe.c

LOCAL_SHARED_LIBRARIES := \
	liblog libcutils libtinyalsa libaudioutils \
	libexpat libdl \
	libmedia libutils \
	libhardware_legacy libtinyxml libtinycompress
LOCAL_STATIC_LIBRARIES := libSprdRecordNrProcess

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= pipe.c audiotester/local_socket.c

LOCAL_C_INCLUDES += debug audiotester

LOCAL_CFLAGS += -DLOCAL_SOCKET_CLIENT

LOCAL_MODULE:= libsprd_audio

LOCAL_SHARED_LIBRARIES := \
	liblog

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
endif
endif

