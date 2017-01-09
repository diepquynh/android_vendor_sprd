LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    download.c \
    packet.c \
    crc16.c \
    connectivity_rf_parameters.c

LOCAL_SHARED_LIBRARIES := \
    libcutils

ifeq ($(PLATFORM_VERSION),4.4.4)
LOCAL_CFLAGS += -DPARTITION_PATH_4.4
LOCAL_CFLAGS += -DWCN_EXTEN_15C
endif

ifeq ($(strip $(WCN_EXTENSION)),true)
LOCAL_CFLAGS += -DGET_MARLIN_CHIPID
LOCAL_CFLAGS += -DWCN_EXTEN_15C
endif

LOCAL_MODULE := download

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

#build for gnss
ifeq ($(strip $(BOARD_USE_SPRD_GNSS)),ge2)
include $(CLEAR_VARS)
LOCAL_SRC_FILES:= \
    gnss_download.c \
    packet.c \
    crc16.c \
    connectivity_rf_parameters.c

LOCAL_SHARED_LIBRARIES := \
    libcutils libpower

LOCAL_CFLAGS += -DGNSS_DOWNLOAD

ifeq ($(PLATFORM_VERSION),4.4.4)
LOCAL_CFLAGS += -DPARTITION_PATH_4.4
endif

LOCAL_MODULE := gnss_download

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
endif
