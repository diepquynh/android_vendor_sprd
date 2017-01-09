ifneq ($(BOARD_WLAN_DEVICE),bcmdhd)
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := sprdwl.ko
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/modules
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

ifeq ($(TARGET_BUILD_VARIANT),user)
  DEBUGMODE := BUILD=no
else
  DEBUGMODE := $(DEBUGMODE)
endif

ifeq ($(KERNEL_UBOOT_USE_ARCH_ARM64), true)
EXT_FLAGS := ARCH=arm64 CROSS_COMPILE=$(shell pwd)/prebuilts/gcc/linux-x86/arm64/gcc-linaro-aarch64-linux-gnu-4.8/bin/aarch64-linux-gnu-
endif

$(LOCAL_PATH)/sprdwl.ko: bootimage
	$(MAKE) -C $(shell dirname $@) $(EXT_FLAGS) $(DEBUGMODE) KDIR=$(ANDROID_PRODUCT_OUT)/obj/KERNEL clean
	$(MAKE) -C $(shell dirname $@) $(EXT_FLAGS) $(DEBUGMODE) KDIR=$(ANDROID_PRODUCT_OUT)/obj/KERNEL
endif
