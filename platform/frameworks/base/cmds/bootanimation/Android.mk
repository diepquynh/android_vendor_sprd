LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES += \
    BootAnimationExt.cpp

LOCAL_C_INCLUDES += vendor/sprd/platform/frameworks/base/cmds/bootanimation

LOCAL_MODULE:= libbootanim

ifdef TARGET_32_BIT_SURFACEFLINGER
LOCAL_32_BIT_ONLY := true
endif

include $(BUILD_STATIC_LIBRARY)
