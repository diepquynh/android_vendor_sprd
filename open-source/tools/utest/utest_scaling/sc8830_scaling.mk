
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)


LOCAL_MODULE:= utest_scaling_$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_TAGS:= debug
LOCAL_MODULE_PATH:= $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_SHARED_LIBRARIES := liblog libEGL libbinder libutils

LOCAL_SRC_FILES:= utest_scaling.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../../../../kernel/include/video
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../../libs/libcamera/sc8830/inc

include $(BUILD_EXECUTABLE)

