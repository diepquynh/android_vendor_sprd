LOCAL_PATH := $(call my-dir)

ifeq ($(BOARD_SPRD_WCNBT_SR2351),true)

include $(CLEAR_VARS)

BDROID_DIR := $(TOP_DIR)external/bluetooth/bluedroid

LOCAL_SRC_FILES := \
        src/bt_vendor_sprd.c \
        src/hardware.c \
        src/userial_vendor.c \
        src/pskey_get.c

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/include \
        $(BDROID_DIR)/hci/include

LOCAL_SHARED_LIBRARIES := \
        libcutils \
        liblog
ifneq ($(findstring 8830gea,$(TARGET_BOOTLOADER_BOARD_NAME)),8830gea)
ifeq ($(findstring 8830,$(TARGET_BOOTLOADER_BOARD_NAME)),8830)
      LOCAL_CFLAGS += -DHW_ADC_ADAPT_SUPPORT
endif
endif

ifeq ($(findstring 7715,$(TARGET_BOOTLOADER_BOARD_NAME)),7715)
      LOCAL_CFLAGS += -DHW_ADC_ADAPT_SUPPORT
endif

ifeq ($(findstring 8815,$(TARGET_BOOTLOADER_BOARD_NAME)),8815)
      LOCAL_CFLAGS += -DHW_ADC_ADAPT_SUPPORT
endif

ifeq ($(findstring 5735,$(TARGET_BOOTLOADER_BOARD_NAME)),5735)
      LOCAL_CFLAGS += -DHW_ADC_ADAPT_SUPPORT
endif

LOCAL_MODULE := libbt-vendor
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_OWNER := sprd

include $(LOCAL_PATH)/vnd_buildcfg.mk

include $(BUILD_SHARED_LIBRARY)

endif
