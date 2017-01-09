# Copyright (C) 2008 The Android Open Source Project
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
ifeq ($(USE_SPRD_SENSOR_LIB),true)
ifneq ($(TARGET_SIMULATOR),true)
ifneq ($(USE_INVENSENSE_LIB),true)

ifeq ($(BOARD_HAVE_ORI),ST480)
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS := libst480.a
include $(BUILD_MULTI_PREBUILT)
endif
# HAL module implemenation, not prelinked, and stored in
# hw/<SENSORS_HARDWARE_MODULE_ID>.<ro.product.board>.so
include $(CLEAR_VARS)

LOCAL_MODULE := sensors.$(TARGET_BOARD_PLATFORM)

LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := -DLOG_TAG=\"Sensors\" \
				-Wall

LOCAL_SRC_FILES := \
			SensorBase.cpp \
			SensorCoordinate.cpp \
			InputEventReader.cpp \
			sensors.cpp

#################################################################
ifeq ($(BOARD_HAVE_ORI),ST480)
LOCAL_STATIC_LIBRARIES := libst480
LOCAL_LDFLAGS = $(LOCAL_PATH)/libst480.a
endif
#ACCELERATION
ifneq ($(BOARD_HAVE_ACC),NULL)
LOCAL_SRC_FILES += Acc_$(BOARD_HAVE_ACC).cpp
LOCAL_CFLAGS += -DACC_INSTALL_$(BOARD_ACC_INSTALL)

#MAGNETIC_FIELD&ORIENTATION
ifneq ($(BOARD_HAVE_ORI),NULL)
LOCAL_SRC_FILES += Ori_$(BOARD_HAVE_ORI).cpp
LOCAL_CFLAGS += -DORI_INSTALL_$(BOARD_ORI_INSTALL)
else
LOCAL_CFLAGS += -DORI_NULL
endif

else
LOCAL_CFLAGS += -DACC_NULL
LOCAL_CFLAGS += -DORI_NULL
endif

#################################################################
#LIGHT&PROXIMITY
ifneq ($(BOARD_HAVE_PLS),NULL)
ifeq ($(BOARD_PLS_COMPATIBLE),true)
LOCAL_SRC_FILES += PlsSensor.cpp
LOCAL_CFLAGS += -DPLS_COMPATIBLE
else
LOCAL_SRC_FILES += Pls_$(BOARD_HAVE_PLS).cpp
endif
else
LOCAL_CFLAGS += -DPLS_NULL
endif


LOCAL_SHARED_LIBRARIES := liblog libcutils libdl
LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)

endif # !USE_INVENSENSE_LIB
endif # !TARGET_SIMULATOR
endif # USE_SPRD_SENSOR_LIB
