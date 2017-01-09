LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(strip $(PRODUCT_SECURE_BOOT)),SANSA)
include $(LOCAL_PATH)/sansa/Android.mk
endif

ifeq ($(strip $(PRODUCT_SECURE_BOOT)),SPRD)
include $(LOCAL_PATH)/sprd/Android.mk
endif
