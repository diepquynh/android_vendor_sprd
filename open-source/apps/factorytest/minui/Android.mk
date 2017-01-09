LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := graphics.c events.c

LOCAL_C_INCLUDES +=\
    external/libpng\
    external/zlib \
    system/core/libpixelflinger/include

LOCAL_MODULE := libftminui

include $(BUILD_STATIC_LIBRARY)
