LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)


include $(LOCAL_PATH)/Camera.mk
#include $(LOCAL_PATH)/Camera_test.mk
#include $(LOCAL_PATH)/Camera_Utest.mk
#include $(LOCAL_PATH)/Utest_jpeg.mk
include $(wildcard $(LOCAL_PATH)/*/Android.mk)
