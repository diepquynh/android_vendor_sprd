LOCAL_PATH:= $(call my-dir)

ifeq ($(PLATFORM_VERSION),4.4.4)
include $(LOCAL_PATH)/4.4/4_4.mk
else ifeq ($(PLATFORM_VERSION),5.1)
include $(LOCAL_PATH)/5.1/5_1.mk
else ifeq ($(PLATFORM_VERSION),$(filter $(PLATFORM_VERSION),6.0 6.0.1))
include $(LOCAL_PATH)/6.0/6_0.mk
endif
