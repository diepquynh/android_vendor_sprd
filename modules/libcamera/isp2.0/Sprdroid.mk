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
ifeq ($(strip $(ISP_HW_VER)),2.0)
CUR_DIR := isp2.0

LOCAL_C_INCLUDES += \
                $(LOCAL_PATH)/$(CUR_DIR)/isp_app \
                $(LOCAL_PATH)/$(CUR_DIR)/isp_tune \
                $(LOCAL_PATH)/$(CUR_DIR)/calibration \
                $(LOCAL_PATH)/$(CUR_DIR)/driver/inc \
                $(LOCAL_PATH)/$(CUR_DIR)/param_manager \
                $(LOCAL_PATH)/$(CUR_DIR)/sft_ae/inc \
                $(LOCAL_PATH)/$(CUR_DIR)/awb/inc \
                $(LOCAL_PATH)/$(CUR_DIR)/awb/ \
                $(LOCAL_PATH)/$(CUR_DIR)/af/inc \
                $(LOCAL_PATH)/$(CUR_DIR)/lsc/inc \
                $(LOCAL_PATH)/$(CUR_DIR)/lib_ctrl/ \
                $(LOCAL_PATH)/$(CUR_DIR)/anti_flicker/inc \
                $(LOCAL_PATH)/$(CUR_DIR)/smart \
                $(LOCAL_PATH)/$(CUR_DIR)/utility \
                $(LOCAL_PATH)/$(CUR_DIR)/calibration/inc \
                $(LOCAL_PATH)/$(CUR_DIR)/sft_af/inc \
                $(LOCAL_PATH)/$(CUR_DIR)/afv1/inc \
                $(LOCAL_PATH)/$(CUR_DIR)/aft/inc
                
         

ifeq ($(strip $(TARGET_BOARD_USE_THRID_LIB)),true)
ifeq ($(strip $(TARGET_BOARD_USE_THIRD_AWB_LIB_A)),true)
ifeq ($(strip $(TARGET_BOARD_USE_ALC_AE_AWB)),true)
	LOCAL_C_INCLUDES += \
                $(LOCAL_PATH)/$(CUR_DIR)/third_lib/alc_ip/inc \
                $(LOCAL_PATH)/$(CUR_DIR)/third_lib/alc_ip/ais/inc

#	example for rm some files
#	RM_FILE = "awb_al_ctrl.c awb_dummy.c ae_dummy.c af_dummy.c"
#	LOCAL_SRC_FILES += $(shell find $(LOCAL_SRC_DIR) -name 'backup' -prune -o -name '*.c' | sed s:^$(LOCAL_PATH)/::g | grep -v $(RM_FILE) )
else	
	LOCAL_C_INCLUDES += \
                $(LOCAL_PATH)/$(CUR_DIR)/third_lib/alc_awb \
                $(LOCAL_PATH)/$(CUR_DIR)/third_lib/alc_awb/inc

endif
endif

ifeq ($(strip $(TARGET_BOARD_USE_THIRD_AF_LIB_A)),true)
	LOCAL_C_INCLUDES += \
               $(LOCAL_PATH)/$(CUR_DIR)/third_lib/alc_af/inc/
endif
endif

LOCAL_CFLAGS += -DAE_WORK_MOD_V0 #AE_WORK_MOD_V0: Old ae algorithm + slow converge
				#AE_WORK_MOD_V1: new ae algorithm + slow converge
				#AE_WORK_MOD_V2: new ae algorithm + fast converge
endif
