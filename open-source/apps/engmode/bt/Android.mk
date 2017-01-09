ifeq ($(HUAWEI_BT_ENGMODE),true)

ifneq (,$(filter true,$(BOARD_SPRD_WCNBT_MARLIN) $(BOARD_SPRD_WCNBT_SR2351)))
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_32_BIT_ONLY := true

# resolve compile error in .h file
LOCAL_CFLAGS += -DHAS_NO_BDROID_BUILDCFG

# 2351 and marlin should not have different, but ...
ifeq ($(BOARD_SPRD_WCNBT_SR2351),true)
LOCAL_CFLAGS += -DSPRD_WCNBT_SR2351
endif

ifeq ($(BOARD_SPRD_WCNBT_MARLIN),true)
LOCAL_CFLAGS += -DSPRD_WCNBT_MARLIN
endif

BLUEDROID_PATH := system/bt/
LOCAL_SRC_FILES := bt_cmd_executer.c \
                   bt_engpc_sprd.c
LOCAL_C_INCLUDES += $(BLUEDROID_PATH)stack/include \
                    $(BLUEDROID_PATH)include

LOCAL_MODULE := libengbt
LOCAL_MODULE_TAGS := debug

LOCAL_SHARED_LIBRARIES += libcutils   \
                          libutils    \
                          libhardware \
                          libhardware_legacy \
                          libdl

include $(BUILD_SHARED_LIBRARY)
endif

else

#Build libengbt
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES := bt_cmd_executer_origin.c 

ifeq ($(BOARD_SPRD_WCNBT_SR2351),true)
  LOCAL_CFLAGS += -DSPRD_WCNBT_SR2351
endif

ifeq ($(BOARD_SPRD_WCNBT_MARLIN),true)
  LOCAL_CFLAGS += -DSPRD_WCNBT_MARLIN
endif


LOCAL_MODULE := libengbt
LOCAL_MODULE_TAGS := debug

LOCAL_SHARED_LIBRARIES += libcutils   \
                          libutils    \
                          libhardware \
                          libhardware_legacy \
			  libcutils
include $(BUILD_SHARED_LIBRARY)

endif


