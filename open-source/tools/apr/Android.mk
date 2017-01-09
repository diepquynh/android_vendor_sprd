LOCAL_PATH:= $(call my-dir)

#apr
include $(CLEAR_VARS)

LOCAL_MODULE := collect_apr
LOCAL_MODULE_TAGS := optional

LOCAL_STATIC_LIBRARIES := libxml2
LOCAL_SHARED_LIBRARIES := libcutils libhardware
LOCAL_SHARED_LIBRARIES += libicuuc
LOCAL_SHARED_LIBRARIES += libc

LOCAL_SRC_FILES := main.cpp
LOCAL_SRC_FILES += Observable.cpp
LOCAL_SRC_FILES += Observer.cpp
LOCAL_SRC_FILES += AprData.cpp
LOCAL_SRC_FILES += XmlStorage.cpp
LOCAL_SRC_FILES += Thread.cpp
LOCAL_SRC_FILES += InotifyThread.cpp
LOCAL_SRC_FILES += NativeCrash.cpp
LOCAL_SRC_FILES += Anr.cpp
LOCAL_SRC_FILES += ModemThread.cpp
LOCAL_SRC_FILES += JavaCrashThread.cpp
LOCAL_SRC_FILES += common.c
LOCAL_SRC_FILES += confile.c

LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog

LOCAL_CFLAGS := -D_STLP_USE_NO_IOSTREAMS
LOCAL_CFLAGS += -D_STLP_USE_MALLOC

LOCAL_C_INCLUDES := $(LOCAL_PATH)/inc
LOCAL_C_INCLUDES += prebuilts/ndk/current/sources/cxx-stl/stlport/stlport
LOCAL_C_INCLUDES += external/libxml2/include
LOCAL_C_INCLUDES += external/icu/icu4c/source/common

include $(BUILD_EXECUTABLE)

#apr.conf
include $(CLEAR_VARS)
LOCAL_MODULE := apr.conf
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := DATA
LOCAL_MODULE_PATH := $(TARGET_OUT_DATA)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

CUSTOM_MODULES += collect_apr
CUSTOM_MODULES += apr.conf
