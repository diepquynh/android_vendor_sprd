# Created by Spreadst

LOCAL_PATH := $(call my-dir)

pack_src_files := \
	pack.c \
	pack_main.c

#
# Build a statically-linked binary to include in OTA packages
#
include $(CLEAR_VARS)

# Build only in eng, so we don't end up with a copy of this in /system
# on user builds.  (TODO: find a better way to build device binaries
# needed only for OTA packages.)
LOCAL_MODULE_TAGS := eng

LOCAL_SRC_FILES := $(pack_src_files)

ifeq ($(TARGET_USERIMAGES_USE_EXT4), true)
LOCAL_CFLAGS += -DUSE_EXT4
LOCAL_CFLAGS += -Wno-unused-parameter
LOCAL_C_INCLUDES += system/extras/ext4_utils
LOCAL_STATIC_LIBRARIES += \
    libext4_utils_static \
    libsparse_static \
    libz
endif

# SPRD: add for remove selinux opration
ifeq ($(NOT_HAVE_SELINUX), true)
$(warning not have selinux)
LOCAL_CFLAGS += -DNOT_HAVE_SELINUX
endif # NOT_HAVE_SELINUX

# SPRD: add for ubi support
LOCAL_STATIC_LIBRARIES += \
    libubiutils

# SPRD: add for spl update
LOCAL_STATIC_LIBRARIES += \
    libsplmerge

LOCAL_STATIC_LIBRARIES += \
    libfs_mgr \

ifeq ($(TARGET_USERIMAGES_USE_EXT4), true)
    LOCAL_CFLAGS += -DUSE_EXT4
    LOCAL_C_INCLUDES += system/extras/ext4_utils
    LOCAL_STATIC_LIBRARIES += libext4_utils_static libz
endif

#@}

LOCAL_STATIC_LIBRARIES += $(TARGET_RECOVERY_UPDATER_LIBS) $(TARGET_RECOVERY_UPDATER_EXTRA_LIBS)
LOCAL_STATIC_LIBRARIES += libapplypatch libedify libmtdutils libminzip libz
LOCAL_STATIC_LIBRARIES += libcrypto_static libbz
LOCAL_STATIC_LIBRARIES += libcutils liblog libstdc++ libc
LOCAL_STATIC_LIBRARIES += libselinux
LOCAL_C_INCLUDES += $(LOCAL_PATH)/..


LOCAL_MODULE := pack

LOCAL_FORCE_STATIC_EXECUTABLE := true

include $(BUILD_EXECUTABLE)
