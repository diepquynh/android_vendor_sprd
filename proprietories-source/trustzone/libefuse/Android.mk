# Spreadtrum efuse hardware layer

LOCAL_PATH:= $(call my-dir)

SPECIAL_EFUSE_LIST := 9838 7720 9832a 7731

TEE_EFUSE_LIST := 9850 9860

include $(CLEAR_VARS)

ifneq ($(strip $(foreach var, $(SPECIAL_EFUSE_LIST), $(strip $(findstring $(var), $(TARGET_BOARD))))),)
LOCAL_SRC_FILES:= sprd_efuse_hw.c
else ifneq ($(strip $(foreach var, $(TEE_EFUSE_LIST), $(strip $(findstring $(var), $(TARGET_BOARD))))),)
LOCAL_SRC_FILES:= sprd_whalex_efuse_hw.c
else
LOCAL_SRC_FILES:= sprd_tshark_efuse_hw.c
endif

LOCAL_MODULE:= libefuse

LOCAL_SHARED_LIBRARIES:= liblog libc libcutils

LOCAL_C_INCLUDES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_MODULE_TAGS:= optional

include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)

ifneq ($(strip $(foreach var, $(SPECIAL_EFUSE_LIST), $(strip $(findstring $(var), $(TARGET_BOARD))))),)
LOCAL_SRC_FILES:= sprd_efuse_hw.c
else ifneq ($(strip $(foreach var, $(TEE_EFUSE_LIST), $(strip $(findstring $(var), $(TARGET_BOARD))))),)
LOCAL_SRC_FILES:= sprd_whalex_efuse_hw.c
else
LOCAL_SRC_FILES:= sprd_tshark_efuse_hw.c
endif

LOCAL_MODULE:= libefuse

LOCAL_SHARED_LIBRARIES:= liblog libc libcutils

LOCAL_C_INCLUDES := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ/usr/include

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)

LOCAL_MODULE_TAGS:= optional

include $(BUILD_STATIC_LIBRARY)


