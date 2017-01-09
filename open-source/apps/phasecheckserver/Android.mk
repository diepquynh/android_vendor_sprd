LOCAL_PATH:=$(call my-dir)
include $(CLEAR_VARS)
LOCAL_LDLIBS := -llog
LOCAL_SRC_FILES:=phasecheckserver.cpp
LOCAL_SHARED_LIBRARIES:=libutils libbinder liblog
LOCAL_MODULE_TAGS:=optional
LOCAL_MODULE:=phasecheckserver
TARGET_SYSTEM_OUT_BIN := $(PRODUCT_OUT)/system/bin
LOCAL_MODULE_PATH := $(TARGET_SYSTEM_OUT_BIN)
include $(BUILD_EXECUTABLE)
