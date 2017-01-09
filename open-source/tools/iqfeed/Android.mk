# CP log
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := iqfeed
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := sd_state_a6.cpp \
		   iqfeed.cpp \
		   conf_ext_sd_a6.cpp

LOCAL_SHARED_LIBRARIES := libc \
			  libcutils \
			  liblog \
			  libutils
LOCAL_CPPFLAGS += -std=c++11
include $(BUILD_EXECUTABLE)

CUSTOM_MODULES += iqfeed
