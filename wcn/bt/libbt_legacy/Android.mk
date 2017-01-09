#include $(call all-subdir-makefiles)

LOCAL_PATH:= $(call my-dir)
ifeq ($(BOARD_SPRD_WCNBT_MARLIN),true)
    include $(LOCAL_PATH)/4.4_5.1/marlin/Android.mk
endif

ifeq ($(BOARD_SPRD_WCNBT_SR2351),true)
    include $(LOCAL_PATH)/4.4_5.1/sr2351/Android.mk
endif
