ifneq ($(TARGET_SIMULATOR),true)
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES    +=  $(LOCAL_PATH) \
			$(LOCAL_PATH)/common \
			$(TARGET_OUT_INTERMEDIATES)/KERNEL/source/include/uapi/mtd \
			vendor/sprd/open-source/apps/engineeringmodel/engcs
LOCAL_SRC_FILES:= \
	nvitem_buf.c \
	nvitem_fs.c \
	nvitem_os.c \
	nvitem_packet.c \
	nvitem_server.c \
	nvitem_sync.c \
	nvitem_channe_spipe.c

LOCAL_CFLAGS += -Wall

LOCAL_SHARED_LIBRARIES := \
    libhardware_legacy \
    libc \
    libutils \
    liblog \
    libcutils
    
#ifeq ($(strip $(TARGET_USERIMAGES_USE_EXT4)),true)
#LOCAL_CFLAGS := -DCONFIG_EMMC
#endif

ifeq ($(strip $(TARGET_USERIMAGES_USE_UBIFS)),true)
LOCAL_CFLAGS := -DCONFIG_NAND_UBI_VOL
endif

LOCAL_MODULE := cp_diskserver
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)
endif
