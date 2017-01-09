ifneq (,$(filter true, $(BOARD_SPRD_WCNBT_MARLIN) $(BOARD_HAVE_BLUETOOTH_SPRD)))

LOCAL_PATH := $(call my-dir)
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

ifeq ($(BOARD_SPRD_WCNBT_MARLIN),true)
LOCAL_CFLAGS := -DSPRD_WCNBT_MARLIN \
                -DHAS_NO_BDROID_BUILDCFG
endif

ifeq ($(strip $(WCN_EXTENSION)),true)
LOCAL_CFLAGS += -DGET_MARLIN_CHIPID
endif

ifeq ($(BOARD_HAVE_BLUETOOTH_SPRD),true)
      LOCAL_CFLAGS += -DHAS_BLUETOOTH_SPRD
      LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
endif

LOCAL_MODULE := libbt-vendor
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_OWNER := sprd

include $(LOCAL_PATH)/vnd_buildcfg.mk
include $(BUILD_SHARED_LIBRARY)

endif
