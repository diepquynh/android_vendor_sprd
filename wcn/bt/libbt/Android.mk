LOCAL_PATH := $(call my-dir)

ifneq ($(SPRD_WCNBT_CHISET),)

include $(CLEAR_VARS)

BDROID_DIR := $(TOP_DIR)system/bt

LOCAL_SRC_FILES := \
        src/bt_vendor_sprd.c \
        src/hardware.c \
        src/userial_vendor.c \
        src/upio.c \
        src/conf.c \
        src/sitm.c \
        conf/sprd/$(SPRD_WCNBT_CHISET)/src/$(SPRD_WCNBT_CHISET).c

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/include \
        $(BDROID_DIR)/hci/include \
        $(LOCAL_PATH)/conf/sprd/$(SPRD_WCNBT_CHISET)/include

LOCAL_SHARED_LIBRARIES := \
        libcutils \
        liblog


## Special configuration ##
ifeq ($(BOARD_SPRD_WCNBT_MARLIN), true)
    ifneq ($(strip $(WCN_EXTENSION)),true)
        LIBBT_CFLAGS += -DSPRD_WCNBT_MARLIN_15A
    else
        LIBBT_CFLAGS += -DGET_MARLIN_CHIPID
    endif
endif
LOCAL_CFLAGS := $(LIBBT_CFLAGS)


LOCAL_MODULE := libbt-vendor
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_OWNER := sprd
LOCAL_PROPRIETARY_MODULE := true

include $(LOCAL_PATH)/vnd_buildcfg.mk

include $(BUILD_SHARED_LIBRARY)

endif
