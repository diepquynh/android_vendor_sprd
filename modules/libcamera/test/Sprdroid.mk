#
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
#
ifneq ($(strip $(TARGET_BOARD_CAMERA_HAL_VERSION)),1.0)
CUR_DIR := test

LOCAL_SRC_DIR := $(LOCAL_PATH)/$(CUR_DIR)

LOCAL_SRC_FILES += $(shell find $(LOCAL_SRC_DIR) -name '*.cpp' | sed s:^$(LOCAL_PATH)/::g )
endif

#set camera preview size
ifneq (,$(filter sp9860% sp9850%,$(TARGET_PRODUCT)))
	LOCAL_CFLAGS += -DPREVIEW_WIDTH=1440
	LOCAL_CFLAGS += -DPREVIEW_HIGHT=1080
else ifneq (,$(filter sp7731%,$(TARGET_PRODUCT)))
	LOCAL_CFLAGS += -DPREVIEW_WIDTH=640
	LOCAL_CFLAGS += -DPREVIEW_HIGHT=480
else ifneq (,$(filter sp9832%,$(TARGET_PRODUCT)))
	LOCAL_CFLAGS += -DPREVIEW_WIDTH=960
	LOCAL_CFLAGS += -DPREVIEW_HIGHT=720
else
	LOCAL_CFLAGS += -DPREVIEW_WIDTH=640
	LOCAL_CFLAGS += -DPREVIEW_HIGHT=480
endif