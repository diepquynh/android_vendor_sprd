LOCAL_PATH:= $(call my-dir)

ifneq ($(strip $(SPRD_WCNBT_CHISET)),)
# down also use ttyS0,which conflict with rdabt 
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	download.c packet.c crc16.c connectivity_rf_parameters.c

LOCAL_SHARED_LIBRARIES := \
	libcutils


ifeq ($(PLATFORM_VERSION),4.4.4)
LOCAL_CFLAGS += -DPARTITION_PATH_4.4
LOCAL_CFLAGS += -DWCN_EXTEN_15C
endif

ifeq ($(strip $(WCN_EXTENSION)),true)
LOCAL_CFLAGS += -DGET_MARLIN_CHIPID
LOCAL_CFLAGS += -DWCN_EXTEN_15C
#LOCAL_CFLAGS += -DENABLE_POWER_CTL
endif

ifeq ($(strip $(EXTERNAL_WCN_DOWNLOAD_POWER_CTL)),true)
#LOCAL_CFLAGS += -DENABLE_POWER_CTL
endif

LOCAL_MODULE := download

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)

endif

#build for gnss
ifeq ($(strip $(BOARD_USE_SPRD_GNSS)),ge2)
include $(CLEAR_VARS)
LOCAL_SRC_FILES:= \
	gnss_download.c packet.c crc16.c connectivity_rf_parameters.c

LOCAL_SHARED_LIBRARIES := \
	libcutils

LOCAL_CFLAGS += -DGNSS_DOWNLOAD

ifeq ($(PLATFORM_VERSION),4.4.4)
LOCAL_CFLAGS += -DPARTITION_PATH_4.4
endif

LOCAL_MODULE := gnss_download

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
endif
