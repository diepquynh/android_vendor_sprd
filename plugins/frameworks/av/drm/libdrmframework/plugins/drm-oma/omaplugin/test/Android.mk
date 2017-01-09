#
# Copyright (C) 2010 The Android Open Source Project
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
#
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    TestDrm.cpp \
    ../src/DrmOmaPlugIn.cpp \
    ../src/DcfParser.cpp \
    ../src/RightsParser.cpp \
    ../src/DmParser.cpp \
    ../src/DcfCreator.cpp \
    ../src/RightsManager.cpp \
    ../src/RightsConsumer.cpp \
    ../src/WbXmlConverter.cpp \
    ../src/UUID.cpp

LOCAL_MODULE := test_drm

LOCAL_STATIC_LIBRARIES := libdrmframeworkcommon \
    libgtest

LOCAL_SHARED_LIBRARIES := \
    libutils \
    liblog \
    libdl \
    libtinyxml \
    libsqlite \
    libcrypto

LOCAL_C_INCLUDES += \
    $(TOP)/frameworks/av/drm/libdrmframework/include \
    $(TOP)/vendor/sprd/plugins/frameworks/av/drm/libdrmframework/plugins/drm-oma/omaplugin/include \
    $(TOP)/frameworks/av/drm/libdrmframework/plugins/common/include \
    $(TOP)/frameworks/av/drm/libdrmframework/plugins/common/util/include \
    $(TOP)/frameworks/av/include \
    $(TOP)/external/tinyxml \
    $(TOP)/external/sqlite/dist \
    $(TOP)/external/openssl/include \
    $(TOP)/external/gtest/include

# Set the following flag to enable the decryption oma flow
#LOCAL_CFLAGS += -DENABLE_OMA_DECRYPTION
LOCAL_MODULE_TAGS := debug
#LOCAL_C_INCLUDES += external/stlport/stlport bionic/ bionic/libstdc++/include
#LOCAL_SHARED_LIBRARIES += libstlport

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := test.dm
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE_CLASS := drm
LOCAL_SRC_FILES := files/test.dm
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/drm/test
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := test.drc
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE_CLASS := drm
LOCAL_SRC_FILES := files/test.drc
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/drm/test
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := test.dr
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE_CLASS := drm
LOCAL_SRC_FILES := files/test.dr
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/drm/test
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := test.dcf
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE_CLASS := drm
LOCAL_SRC_FILES := files/test.dcf
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/drm/test
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := sd_5coun_midi_11k_zhendong_Blue_Ice.dcf
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE_CLASS := drm
LOCAL_SRC_FILES := files/sd_5coun_midi_11k_zhendong_Blue_Ice.dcf
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/drm/test
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := sd_14_3_5_15_3_5_avi_1666k_MP4V_mp3.dcf
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE_CLASS := drm
LOCAL_SRC_FILES := files/sd_14_3_5_15_3_5_avi_1666k_MP4V_mp3.dcf
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)/drm/test
include $(BUILD_PREBUILT)
