# Copyright 2005 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

define _add-w-alarm-image
include $$(CLEAR_VARS)
LOCAL_MODULE := alarm_w_$(notdir $(1))
LOCAL_MODULE_STEM := $(notdir $(1))
_img_modules += $$(LOCAL_MODULE)
LOCAL_SRC_FILES := $1
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $$(TARGET_OUT)/res/images/wvga
include $$(BUILD_PREBUILT)
endef

define _add-h-alarm-image
include $$(CLEAR_VARS)
LOCAL_MODULE := alarm_h_$(notdir $(1))
LOCAL_MODULE_STEM := $(notdir $(1))
_img_modules += $$(LOCAL_MODULE)
LOCAL_SRC_FILES := $1
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $$(TARGET_OUT)/res/images/hvga
include $$(BUILD_PREBUILT)
endef

_w_img_modules :=
_h_img_modules :=
_img_modules :=
_images :=
$(foreach _img, $(call find-subdir-subdir-files, "res/images/wvga", "*.png"), \
  $(eval $(call _add-w-alarm-image,$(_img))))
_images :=
$(foreach _img, $(call find-subdir-subdir-files, "res/images/hvga", "*.png"), \
  $(eval $(call _add-h-alarm-image,$(_img))))

include $(CLEAR_VARS)
LOCAL_MODULE := default_ring.mp3
LOCAL_MODULE_STEM := default_ring.mp3
_img_modules += default_ring.mp3
LOCAL_SRC_FILES := res/sounds/default_ring.mp3
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT)/res/sounds/
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := alarm_res
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := $(_w_img_modules) $(_h_img_modules) $(_img_modules)
include $(BUILD_PHONY_PACKAGE)

_img_modules :=
_w_img_modules :=
_h_img_modules :=
_add-alarm-image :=

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	graphics.c \
	load_bootimage.c \
	resources.c \
	alarm.c \
	log.c \
	events.c \
	battery.c

LOCAL_C_INCLUDES += \
	external/zlib \
	external/libpng

LOCAL_MODULE:= poweroff_alarm

LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_MODULE_PATH := $(TARGET_OUT_EXECUTABLES)
LOCAL_UNSTRIPPED_PATH := $(TARGET_OUT_EXECUTABLES_UNSTRIPPED)

LOCAL_STATIC_LIBRARIES := libcutils libc

LOCAL_STATIC_LIBRARIES += libcutils libpixelflinger_static
LOCAL_STATIC_LIBRARIES += libunz  libpng libcutils libz liblog
LOCAL_STATIC_LIBRARIES += libstdc++ libc
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := alarm_res

include $(BUILD_EXECUTABLE)

