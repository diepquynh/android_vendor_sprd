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
    src/DrmOmaPlugIn.cpp \
    src/DcfParser.cpp \
    src/RightsParser.cpp \
    src/DmParser.cpp \
    src/DcfCreator.cpp \
    src/RightsManager.cpp \
    src/RightsConsumer.cpp \
    src/WbXmlConverter.cpp \
    src/UUID.cpp

LOCAL_MODULE := libdrmomaplugin

LOCAL_STATIC_LIBRARIES := libdrmframeworkcommon

LOCAL_SHARED_LIBRARIES := \
    libutils \
    liblog \
    libdl \
    libtinyxml \
    libsqlite \
    libcrypto \
    libandroid_servers

LOCAL_C_INCLUDES += \
    $(TOP)/frameworks/av/drm/libdrmframework/include \
    $(TOP)/vendor/sprd/plugins/frameworks/av/drm/libdrmframework/plugins/drm-oma/omaplugin/include \
    $(TOP)/frameworks/av/drm/libdrmframework/plugins/common/include \
    $(TOP)/frameworks/av/drm/libdrmframework/plugins/common/util/include \
    $(TOP)/frameworks/av/include \
    $(TOP)/external/tinyxml \
    $(TOP)/external/sqlite/dist \
    $(TOP)/external/boringssl/include

# Set the following flag to enable the decryption oma flow
#LOCAL_CFLAGS += -DENABLE_OMA_DECRYPTION
#LOCAL_MODULE_PATH := $(TARGET_OUT)/lib/drm
LOCAL_MODULE_RELATIVE_PATH := drm

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

include $(call all-makefiles-under,$(LOCAL_PATH))
