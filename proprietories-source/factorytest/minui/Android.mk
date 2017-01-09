LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := graphics.c graphics_adf.c graphics_fbdev.c events.c \
	resources.c

LOCAL_C_INCLUDES +=\
    external/libpng\
    external/zlib \
    system/core/libpixelflinger/include
    
LOCAL_WHOLE_STATIC_LIBRARIES += libadf

LOCAL_MODULE := libftminui

# This used to compare against values in double-quotes (which are just
# ordinary characters in this context).  Strip double-quotes from the
# value so that either will work.

ifeq ($(subst ",,$(TARGET_RECOVERY_PIXEL_FORMAT)),RGBX_8888)
  LOCAL_CFLAGS += -DRECOVERY_RGBX
endif
ifeq ($(subst ",,$(TARGET_RECOVERY_PIXEL_FORMAT)),BGRA_8888)
  LOCAL_CFLAGS += -DRECOVERY_BGRA
endif

ifneq ($(TARGET_RECOVERY_OVERSCAN_PERCENT),)
  LOCAL_CFLAGS += -DOVERSCAN_PERCENT=$(TARGET_RECOVERY_OVERSCAN_PERCENT)
else
  LOCAL_CFLAGS += -DOVERSCAN_PERCENT=0
endif

ifneq (,$(filter sp9860% sp9850%,$(TARGET_PRODUCT)))
	LOCAL_CFLAGS += -DFONT_SIZE=37
else ifneq (,$(filter sp7731%,$(TARGET_PRODUCT)))
	LOCAL_CFLAGS += -DFONT_SIZE=18
else ifneq (,$(filter sp9832%,$(TARGET_PRODUCT)))
	LOCAL_CFLAGS += -DFONT_SIZE=37
else
	LOCAL_CFLAGS += -DFONT_SIZE=18
endif

include $(BUILD_STATIC_LIBRARY)
