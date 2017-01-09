ifneq ($(TARGET_SIMULATOR),true)
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
BOARD_PRODUCT_NAME:=$(TARGET_BOARD)
$(warning BOARD_PRODUCT_NAME = $(BOARD_PRODUCT_NAME))
commands_local_path := $(LOCAL_PATH)
ifeq ($(strip $(BOARD_USE_EMMC)),true)
LOCAL_CFLAGS += -DCONFIG_EMMC
endif

ifeq ($(strip $(TARGET_USERIMAGES_USE_UBIFS)),true)
LOCAL_CFLAGS += -DCONFIG_NAND
endif
ifeq ($(PRODUCT_WIFI_DEVICE),bcm)
LOCAL_CFLAGS += -DBCM
endif
ifeq ($(BOARD_SPRD_WCNBT_MARLIN),true)
LOCAL_CFLAGS += -DSPRD_WCNBT_MARLIN
endif

ifeq ($(PLATFORM_VERSION), 6.0)
    LOCAL_CFLAGS += -DPLATFORM_VERSION6
endif

ifeq ($(PLATFORM_VERSION), 7.0)
    LOCAL_CFLAGS += -DPLATFORM_VERSION7
endif

ifeq ($(BOARD_HAVE_FM_BCM),true)
	LOCAL_CFLAGS += -DBOARD_HAVE_FM_BCM
endif

ifneq (,$(filter sp9860% sp9850%,$(TARGET_PRODUCT)))
ifeq (,$(filter sp9850ka% sp9833ka%,$(TARGET_PRODUCT)))
	LOCAL_CFLAGS += -DHAL_VERSION_1_3
	LOCAL_CFLAGS += -DAUDIO_DRIVER_2
endif
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

LOCAL_C_INCLUDES    +=  $(LOCAL_PATH) \
			$(LOCAL_PATH)/minui \
			$(LOCAL_PATH)/common	\
			$(LOCAL_PATH)/testitem  \
			$(LOCAL_PATH)/res \
			$(LOCAL_PATH)/sensor

LOCAL_C_INCLUDES    +=  external/tinyalsa/include \
			hardware/libhardware/include/hardware
#			vendor/sprd/partner/silead/prebuiltlibs/libs/inc


LOCAL_STATIC_LIBRARIES += libftminui libcutils liblog libpng libz
LOCAL_SHARED_LIBRARIES := libpixelflinger libcutils libtinyalsa libhardware libdl libhardware_legacy libnetutils

#libasound libeng_audio_mode
LOCAL_SRC_FILES := factorytest.c  \
		eng_tok.c \
		ui.c \
		ui_touch.c \
		parse_conf.c \


LOCAL_SRC_FILES += $(call all-c-files-under, testitem)
ifeq ($(BOARD_SPRD_WCNBT_SR2351),true)
  LOCAL_CFLAGS += -DSPRD_WCNBT_SR2351
endif

ifdef BOARD_HAVE_ACC
LOCAL_CFLAGS        += -DBOARD_HAVE_ACC=\"$(BOARD_HAVE_ACC)\"
endif

ifdef BOARD_HAVE_PLS
LOCAL_CFLAGS        += -DBOARD_HAVE_PLS=\"$(BOARD_HAVE_PLS)\"
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

LOCAL_MODULE := factorytest
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := 32

include $(BUILD_EXECUTABLE)
#include $(call all-makefiles-under,$(LOCAL_PATH))
include $(commands_local_path)/minui/Android.mk
endif

