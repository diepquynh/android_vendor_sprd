# Copyright (C) 2012 The Android Open Source Project
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

ifeq ($(strip $(AUDIO_SMART_PA_TYPE)), NXP)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS := -D_POSIX_SOURCE -Wno-multichar -g

LOCAL_C_INCLUDES += external/tinyalsa/include \
			external/expat/lib	\
			system/media/audio_utils/include

LOCAL_SRC_FILES :=  Sprd_NxpTfa.c

LOCAL_SHARED_LIBRARIES := liblog libc libcutils liblog libtinyalsa libaudioutils \
	libexpat libdl	\
	libvbeffect	\
	libtfa9890_interface \
	libaudioutils

LOCAL_MODULE := libnxppa

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)

LOCAL_C_INCLUDES:= external/tinyalsa/include	\
		   frameworks/base/include	\
		   system/core/libcutils 	\
		   frameworks/av/include \
		   frameworks/native/include \
		   system/core/include \
		   hardware/libhardware/include \
		   hardware/libhardware_legacy/include \


LOCAL_SRC_FILES := sprdFmTrack.cpp

LOCAL_SHARED_LIBRARIES := liblog libc libcutils liblog libtinyalsa libaudioutils \
			  libexpat libdl	\
			  libvbeffect	\
			  libmedia	\
			  libui liblog libcutils libutils libbinder libsonivox libicuuc libicui18n libexpat \
			  libcamera_client libstagefright_foundation \
			  libgui libdl libaudioutils libnbaio

LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false

LOCAL_MODULE:= libpaFmTrack

include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)

LOCAL_C_INCLUDES:= external/tinyalsa/include	\
			  $(LOCAL_PATH)/../libFmRecordHal \
			  $(LOCAL_PATH)/../include

LOCAL_SRC_FILES := Sprd_Spk_Fm.c

LOCAL_SHARED_LIBRARIES := liblog libc libcutils liblog libtinyalsa libaudioutils \
			  libexpat libdl	\
			  libvbeffect	\
			  libtfa9890_interface	\
			  libpaFmTrack	\
			  libFMHalSource

LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false


LOCAL_MODULE:= libfminterface

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))

endif
