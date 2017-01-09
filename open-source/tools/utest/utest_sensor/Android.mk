
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SHARED_LIBRARIES := libcutils libhardware
LOCAL_MODULE:= utest_sensor
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE_PATH:= $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_SRC_FILES:= utest_sensor.cpp
include $(BUILD_EXECUTABLE)
