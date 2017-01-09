#Android makefile for 2351
LOCAL_PATH := $(call my-dir)
ifeq ($(strip $(BOARD_WLAN_DEVICE)),sc2351)
#source files
IWNPI_OBJS	  += iwnpi.c util.c cmd.c
#cflags
IWNPI_CFLAGS ?= -O2 -g
IWNPI_CFLAGS += -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -DCONFIG_LIBNL20
#include dirs
INCLUDES += external/libnl/include

#Build iwnpi tool
include $(CLEAR_VARS)
LOCAL_MODULE := iwnpi
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS = $(IWNPI_CFLAGS)
LOCAL_SRC_FILES = $(IWNPI_OBJS)
LOCAL_C_INCLUDES = $(INCLUDES)
LOCAL_STATIC_LIBRARIES += libnl
LOCAL_SHARED_LIBRARIES += libcutils   \
                          libutils
include $(BUILD_EXECUTABLE)
endif

#Build libiwnpi

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LIBIWNPI_OBJS	  += iwnpi_cmd_executer.c util.c cmd.c
#cflags
LIBIWNPI_CFLAGS ?= -O2 -g
LIBIWNPI_CFLAGS += -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -DCONFIG_LIBNL20
#include dirs
INCLUDES += external/libnl/include

LOCAL_SHARED_LIBRARIES += libcutils   \
                          libutils

LOCAL_MODULE := libiwnpi
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS = $(LIBIWNPI_CFLAGS)
LOCAL_SRC_FILES = $(LIBIWNPI_OBJS)
LOCAL_C_INCLUDES = $(INCLUDES)
LOCAL_STATIC_LIBRARIES += libnl
include $(BUILD_SHARED_LIBRARY)
