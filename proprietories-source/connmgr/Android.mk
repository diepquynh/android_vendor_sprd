LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_32_BIT_ONLY := true

WCND_LOCAL_CFLAGS := -DUSE_MARLIN

ifeq ($(BOARD_WLAN_DEVICE), sc2351)
WCND_LOCAL_CFLAGS :=
endif

LOCAL_CFLAGS += $(WCND_LOCAL_CFLAGS)

LOCAL_SRC_FILES:= \
    wcnd.c \
    wcnd_cmd.c \
    wcnd_worker.c \
    wcnd_sm.c \
    wcnd_eng_cmd_executer.c \
    wcnd_download.c \
    wcnd_util.c

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libnetutils

ifeq ($(USE_SPRD_ENG), true)
LOCAL_SHARED_LIBRARIES += \
    libiwnpi \
    libengbt \
    libengfm
LOCAL_CFLAGS += -DUSE_ENG_MODE
endif


ifeq ($(BOARD_WLAN_DEVICE), bcmdhd)
LOCAL_CFLAGS += -DHAVE_SLEEPMODE_CONFIG
endif

ifeq ($(PLATFORM_VERSION), 6.0)
LOCAL_CFLAGS += -DFM_ENG_DEBUG
endif

LOCAL_SRC_FILES += wcnd_eng_wifi_priv.c


LOCAL_MODULE := connmgr
LOCAL_INIT_RC := wcnd.rc
LOCAL_MODULE_TAGS := optional

LOCAL_REQUIRED_MODULES := connmgr_cli

include $(BUILD_EXECUTABLE)


#build connmgr_cli
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    wcnd_cli.c


LOCAL_SHARED_LIBRARIES := \
    libcutils

LOCAL_MODULE := connmgr_cli

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
