#Created by Spreadst

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    vfat_format.c \
    newfs_msdos.c
LOCAL_MODULE := libvfat_format
include $(BUILD_STATIC_LIBRARY)

