#add 2341 npi app
LOCAL_PATH := $(call my-dir)
ifeq ($(strip $(BOARD_WLAN_DEVICE)),sc2341)
#source files
WLNPI_OBJS	  += wlnpi.c cmd.c
#cflags
IWNPI_CFLAGS ?= -O2 -g
IWNPI_CFLAGS += -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -DCONFIG_LIBNL20
#include dirs
INCLUDES += external/libnl/include

#Build wlnpi tool
include $(CLEAR_VARS)
LOCAL_MODULE := iwnpi
LOCAL_SHARED_LIBRARIES += libcutils   \
                          libutils

LOCAL_STATIC_LIBRARIES += libcutils   \
                          libutils
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS = $(IWNPI_CFLAGS)
LOCAL_SRC_FILES = $(WLNPI_OBJS)
LOCAL_C_INCLUDES = $(INCLUDES)
LOCAL_STATIC_LIBRARIES += libnl
include $(BUILD_EXECUTABLE)

endif
