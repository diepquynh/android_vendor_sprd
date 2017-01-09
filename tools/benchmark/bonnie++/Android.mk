#build executable

LOCAL_PATH:= $(call my-dir)
vold_cflags :=-O2  -DNDEBUG $(WFLAGS) $(MORECFLAGS)

include $(CLEAR_VARS)
LOCAL_MODULE:= bonnie++
LOCAL_MODULE_TAGS:= debug
LOCAL_MODULE_PATH:= $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_SRC_FILES := \
	bonnie++.cpp \
	bon_io.cpp \
	bon_file.cpp \
	bon_time.cpp \
	semaphore.cpp \
	forkit.cpp \
	bon_suid.cpp \
LOCAL_CFLAGS := $(vold_cflags)
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE:= zcav
LOCAL_MODULE_TAGS:= debug
LOCAL_MODULE_PATH:= $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_SRC_FILES := \
	zcav.cpp \
	bon_suid.cpp
include $(BUILD_EXECUTABLE)

