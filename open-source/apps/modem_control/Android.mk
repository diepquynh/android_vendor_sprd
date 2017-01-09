LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(BOARD_SECURE_BOOT_ENABLE), true)
    LOCAL_C_INCLUDES := \
        vendor/sprd/proprietories-source/sprd_verify
endif

LOCAL_SRC_FILES:= \
	main.c modem_boot.c packet.c crc16.c modem_control.c nv_read.c

ifeq ($(BOARD_SECURE_BOOT_ENABLE), true)
    LOCAL_SRC_FILES += modem_verify.c
    LOCAL_STATIC_LIBRARIES += libsprd_verify
endif

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libhardware_legacy

ifeq ($(strip $(TARGET_USERIMAGES_USE_EXT4)),true)
LOCAL_CFLAGS := -DCONFIG_EMMC
endif

ifeq ($(BOARD_SECURE_BOOT_ENABLE), true)
  LOCAL_CFLAGS += -DSECURE_BOOT_ENABLE
endif

LOCAL_MODULE := modem_control

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)
