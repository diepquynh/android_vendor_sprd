LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_32_BIT_ONLY := true

# resolve compile error in .h file

BLUEDROID_PATH := system/bt
LOCAL_SRC_FILES := bt_eng.c \
                   bt_hal.c \
                   bt_engpc_sprd.c

LOCAL_C_INCLUDES += $(BLUEDROID_PATH)/stack/include \
                    $(BLUEDROID_PATH)/include \
                    $(BLUEDROID_PATH)/hci/include

ifneq ($(SPRD_WCNBT_CHISET),)
    LOCAL_C_INCLUDES += $(BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR)
    LOCAL_CFLAGS += -DHAS_SPRD_BUILDCFG -DHAS_BDROID_BUILDCFG
else
    LOCAL_CFLAGS += -DHAS_NO_BDROID_BUILDCFG
endif

LOCAL_MODULE := libengbt
LOCAL_MODULE_TAGS := debug

LOCAL_SHARED_LIBRARIES += libcutils   \
                          libutils    \
                          libhardware \
                          libhardware_legacy \
                          libdl

include $(BUILD_SHARED_LIBRARY)
