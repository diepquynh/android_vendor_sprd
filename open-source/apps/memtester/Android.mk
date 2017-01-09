#build executable

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(TARGET_ARCH),x86_64)
LOCAL_CFLAGS+= -DSUPPORT_X86
endif

LOCAL_MODULE_TAGS:= debug
LOCAL_32_BIT_ONLY := true
LOCAL_CFLAGS+=-O2 -DPOSIX -D_POSIX_C_SOURCE=200809L -D_FILE_OFFSET_BITS=64 -DTEST_NARROW_WRITES -c -static

LOCAL_SHARED_LIBRARIES := \
    libc             \
    libcutils             \
    libhardware_legacy

LOCAL_SRC_FILES:= memtester.c tests.c log.c cpu.cpp

LOCAL_MODULE:= memtester

include $(BUILD_EXECUTABLE)




