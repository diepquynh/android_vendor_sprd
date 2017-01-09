# Spreadtrum efuse hardware layer

LOCAL_PATH:= $(call my-dir)

SPECIAL_EFUSE_LIST := 9838 7720 7731c 7727se 9832a

include $(CLEAR_VARS)

#LOCAL_CFLAGS := -DSCX15 -I$(LOCAL_PATH)

ifneq ($(strip $(foreach var, $(SPECIAL_EFUSE_LIST), $(strip $(findstring $(var), $(TARGET_BOARD))))),)
LOCAL_SRC_FILES:= sprd_efuse_hw.c
else
LOCAL_SRC_FILES:= sprd_tshark_efuse_hw.c
endif

LOCAL_MODULE:= libefuse

LOCAL_SHARED_LIBRARIES:= liblog libc libcutils

LOCAL_C_INCLUDES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_MODULE_TAGS:= optional

include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)

#LOCAL_CFLAGS := -DSCX15 -I$(LOCAL_PATH)

ifneq ($(strip $(foreach var, $(SPECIAL_EFUSE_LIST), $(strip $(findstring $(var), $(TARGET_BOARD))))),)
LOCAL_SRC_FILES:= sprd_efuse_hw.c
else
LOCAL_SRC_FILES:= sprd_tshark_efuse_hw.c
endif


LOCAL_MODULE:= libefuse

LOCAL_SHARED_LIBRARIES:= liblog libc libcutils

LOCAL_C_INCLUDES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)

LOCAL_MODULE_TAGS:= optional

include $(BUILD_STATIC_LIBRARY)


