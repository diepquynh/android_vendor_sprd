LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
BDROID_DIR := $(TOP_DIR)system/bt

LOCAL_SRC_FILES := \
        src/bqb.c \
        src/bqb_vendor.c

LOCAL_C_INCLUDES += \
        $(LOCAL_PATH)/include \
        $(BDROID_DIR)/hci/include \
        $(BDROID_DIR)/stack/include \
        $(BDROID_DIR)/include \
        $(BDROID_DIR)/btcore/include \
        $(BDROID_DIR)/gki/common \
        $(BDROID_DIR)/gki/ulinux \
        $(BDROID_DIR)/osi/include \
        $(BDROID_DIR)/stack/include \
        $(BDROID_DIR)/utils/include \
        $(BDROID_DIR)/

LOCAL_SHARED_LIBRARIES := \
        libcutils \
        liblog

LOCAL_CFLAGS := -fstack-protector
LOCAL_MODULE := libbqbbt
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_OWNER := sprd
LOCAL_MULTILIB := 32
include $(BUILD_SHARED_LIBRARY)
