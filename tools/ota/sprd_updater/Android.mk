LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := sprd_install.cpp
LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../ \
    bootable/recovery \
# SPRD: add for support format vfat @{
LOCAL_STATIC_LIBRARIES += \
    libvfat_format
# @}

# SPRD: add for ubi support
LOCAL_STATIC_LIBRARIES += \
    libubiutils

# SPRD: add for spl update
LOCAL_STATIC_LIBRARIES += \
    libsplmerge

LOCAL_STATIC_LIBRARIES += \
    libfs_mgr \
    libselinux \
    libapplypatch \
    libcrypto_static \

ifeq ($(TARGET_USERIMAGES_USE_EXT4), true)
    LOCAL_CFLAGS += -DUSE_EXT4
    LOCAL_C_INCLUDES += system/extras/ext4_utils
    LOCAL_STATIC_LIBRARIES += libext4_utils_static libz
endif


TARGET_RECOVERY_UPDATER_EXTRA_LIBS := $(LOCAL_STATIC_LIBRARIES)

LOCAL_MODULE := libsprd_updater
include $(BUILD_STATIC_LIBRARY)
