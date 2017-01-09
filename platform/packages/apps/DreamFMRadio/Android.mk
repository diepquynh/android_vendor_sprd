# Copyright (C) 2014 The Android Open Source Project
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

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := fm.conf
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/priv-app/FMRadio
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_CERTIFICATE := platform

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := DreamFMRadio
LOCAL_JNI_SHARED_LIBRARIES := libfmjni
LOCAL_REQUIRED_MODULES := fm.conf

LOCAL_PROGUARD_ENABLED := disabled
LOCAL_PRIVILEGED_MODULE := true

LOCAL_STATIC_JAVA_LIBRARIES += android-support-v7-cardview \
                               com.broadcom.bt
LOCAL_RESOURCE_DIR = $(LOCAL_PATH)/res frameworks/support/v7/cardview/res

#LOCAL_JAVA_LIBRARIES := com.broadcom.bt

LOCAL_AAPT_FLAGS := --auto-add-overlay --extra-packages android.support.v7.cardview

ifneq ($(strip $(PRODUCT_DISABLED_FMRADIO)),true)
include $(BUILD_PACKAGE)
endif

include $(call all-makefiles-under,$(LOCAL_PATH))



