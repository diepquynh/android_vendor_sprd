ifneq ($(TARGET_SIMULATOR),true)
LOCAL_PATH:= $(call my-dir)
# SPRD: add for secure boot @{
ifeq ($(BOARD_SECURE_BOOT_ENABLE), true)
  RECOVERY_COMMON_CGLAGS = -DSECURE_BOOT_ENABLE
endif
include $(CLEAR_VARS)
LOCAL_C_INCLUDES = \
    $(LOCAL_PATH) \
    $(LOCAL_PATH)/$(BOARD_PRODUCT_NAME)/
LOCAL_SRC_FILES:= \
    splmerge.c
LOCAL_STATIC_LIBRARIES += libcutils libstdc++ libc
LOCAL_MODULE := splmerge
LOCAL_MODULE_TAGS := optional
LOCAL_FORCE_STATIC_EXECUTABLE := true

LOCAL_CFLAGS := -DMAKE_EXECUTABLE
# SPRD: add for secure boot
LOCAL_CFLAGS += $(RECOVERY_COMMON_CGLAGS)

ifeq ($(strip $(TARGET_USERIMAGES_USE_UBIFS)),true)
LOCAL_CFLAGS += -DCONFIG_NAND
endif
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= := \
    splmerge.c
LOCAL_STATIC_LIBRARIES += libcutils libstdc++ libc
LOCAL_MODULE := libsplmerge
LOCAL_MODULE_TAGS := optional
# SPRD: add for secure boot
LOCAL_CFLAGS += $(RECOVERY_COMMON_CGLAGS)
include $(BUILD_STATIC_LIBRARY)
endif
