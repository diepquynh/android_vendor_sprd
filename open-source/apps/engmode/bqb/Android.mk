LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
BDROID_DIR := $(TOP_DIR)system/bt

LOCAL_SRC_FILES := \
        main.c \
        vendor.c

LOCAL_C_INCLUDES += \
        $(BDROID_DIR)/hci/include \
        $(BDROID_DIR)/stack/include \
        $(BDROID_DIR)/include \
        $(BDROID_DIR)/btcore/include \
        $(BDROID_DIR)/gki/common \
        $(BDROID_DIR)/gki/ulinux \
        $(BDROID_DIR)/osi/include \
        $(BDROID_DIR)/stack/include \
        $(BDROID_DIR)/utils/include

LOCAL_SHARED_LIBRARIES := \
        libcutils \
        liblog

ifeq ($(BOARD_SPRD_WCNBT_MARLIN),true)
LOCAL_CFLAGS := -DSPRD_WCNBT_MARLIN \
        -DHAS_NO_BDROID_BUILDCFG
endif


ifeq ($(BOARD_HAVE_BLUETOOTH_SPRD),true)
      LOCAL_CFLAGS += -DHAS_BLUETOOTH_SPRD
endif

LOCAL_MODULE := libbt-bqb
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MULTILIB := 32
include $(BUILD_STATIC_LIBRARY)


