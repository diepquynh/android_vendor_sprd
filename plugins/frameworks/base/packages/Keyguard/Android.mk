LOCAL_PATH := $(call my-dir)

# TODO:SPRD_CHECK The SystemUI have not porting complete yet, disable building
include $(call all-makefiles-under,$(LOCAL_PATH))

# open SystemuiUtil plugin
#include $(LOCAL_PATH)/addons/SystemuiUtil/Android.mk
