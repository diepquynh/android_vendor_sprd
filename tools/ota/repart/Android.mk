LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES    +=  $(LOCAL_PATH) \
        $(LOCAL_PATH)/$(BOARD_PRODUCT_NAME)/ \
        $(TARGET_OTA_EXTENSIONS_DIR) \
        system/extras/ext4_utils \
        system/vold \
        bootable/recovery/mtdutils

LOCAL_SRC_FILES:= \
        crc32.c \
        gptdata.c \
        main.c \
        roots.cpp

LOCAL_STATIC_LIBRARIES += \
        libcutils \
        libstdc++ \
        libc \
        libmtdutils \
        libfs_mgr \
        libext4_utils_static \
        libcutils \
        libselinux \
        libext4_utils_static \
        libsparse_static \
        libz \
        libshowinfo

LOCAL_MODULE := repart
LOCAL_MODULE_TAGS := optional
LOCAL_FORCE_STATIC_EXECUTABLE := true
include $(BUILD_EXECUTABLE)

