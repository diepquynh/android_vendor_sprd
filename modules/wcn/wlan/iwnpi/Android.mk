#Build libiwnpi
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifdef WIFI_DRIVER_MODULE_PATH
LOCAL_CFLAGS        += -DWIFI_DRIVER_MODULE_PATH=\"$(WIFI_DRIVER_MODULE_PATH)\"
endif
ifdef WIFI_DRIVER_MODULE_NAME
LOCAL_CFLAGS        += -DWIFI_DRIVER_MODULE_NAME=\"$(WIFI_DRIVER_MODULE_NAME)\"
endif

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES    += iwnpi_cmd_executer.c \
                      util.c \
                      cmd.c
#cflags
LOCAL_CFLAGS ?= -O2 -g
LOCAL_CFLAGS += -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -DCONFIG_LIBNL20
#include dirs
LOCAL_C_INCLUDES += external/libnl/include

LOCAL_SHARED_LIBRARIES += libcutils   \
                          libutils

LOCAL_MODULE := libiwnpi
LOCAL_MODULE_TAGS := optional
LOCAL_STATIC_LIBRARIES += libnl
include $(BUILD_SHARED_LIBRARY)

#Android makefile for libsprdftms

ifeq ($(strip $(BOARD_WLAN_DEVICE)),sc2332)
#Build libss
include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
#source files
LIBSS_OBJS	  += ops.c netlink.c wrap.c nl80211.c
#cflags
LIBSS_CFLAGS ?= -O2 -g
LIBSS_CFLAGS += -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs \
				-fno-strict-aliasing -fno-common \
				-Werror-implicit-function-declaration \
				-DCONFIG_LIBNL20 -DCONFIG_SAMSUNG_EXT
#include dirs
INCLUDES += external/libnl/include

LOCAL_MODULE = libsprdftms
LOCAL_MODULE_TAGS = debug
LOCAL_CFLAGS = $(LIBSS_CFLAGS)
LOCAL_SRC_FILES = $(LIBSS_OBJS)
LOCAL_C_INCLUDES = $(INCLUDES)
LOCAL_SHARED_LIBRARIES += libcutils
LOCAL_SHARED_LIBRARIES += libnl
include $(BUILD_SHARED_LIBRARY)
endif

#Compile iwnpi
include $(call all-makefiles-under,$(LOCAL_PATH))
