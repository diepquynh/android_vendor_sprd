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


LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS := -D_POSIX_SOURCE -Wno-multichar -g

LOCAL_C_INCLUDES += vendor/sprd/open-source/apps/engmode \
			external/tinyalsa/include \
			external/expat/lib

LOCAL_SRC_FILES :=  audio_pga.c vb_pga.c tinyalsa_util.c

LOCAL_SHARED_LIBRARIES := liblog libc libcutils liblog libtinyalsa libaudioutils \
	libexpat libdl libhardware_legacy

LOCAL_MODULE := libvbpga

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)




