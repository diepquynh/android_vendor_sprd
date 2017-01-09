LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    modemd.c      \
    modemd_vlx.c  \
    modemd_ext.c  \
    modemd_sipc.c

LOCAL_SHARED_LIBRARIES := \
    libcutils             \
    libhardware_legacy

ifeq ($(strip $(TARGET_USERIMAGES_USE_EXT4)),true)
LOCAL_CFLAGS := -DCONFIG_EMMC
endif

ifeq ($(BOARD_SECURE_BOOT_ENABLE), true)
LOCAL_CFLAGS += -DSECURE_BOOT_ENABLE
endif

LOCAL_MODULE := modemd

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
