ifneq ($(TARGET_SIMULATOR),true)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE    := false

CONFIG_MINIENGPC := false

ifeq ($(strip $(BOARD_SECURE_BOOT_ENABLE)), true)
LOCAL_CFLAGS += -DSECURE_BOOT_ENABLE
endif

ifeq ($(CONFIG_MINIENGPC), true)
LOCAL_CFLAGS += -DCONFIG_MINIENGPC
ifeq ($(USE_AUDIO_WHALE_HAL), true)
LOCAL_CFLAGS += -DUSE_AUDIO_WHALE_HAL
endif
LOCAL_SHARED_LIBRARIES  := libcutils libsqlite libhardware libhardware_legacy
else
ifeq ($(USE_AUDIO_WHALE_HAL), true)
LOCAL_CFLAGS += -DUSE_AUDIO_WHALE_HAL
LOCAL_SHARED_LIBRARIES  := libcutils libsqlite libhardware libhardware_legacy libatci libefuse libgpspc libbm \
                           libeng-audio
else
LOCAL_SHARED_LIBRARIES  := libcutils libsqlite libhardware libhardware_legacy libvbeffect libvbpga libnvexchange libatci libefuse libgpspc libbm \
                           libeng-audio
endif
ifeq ($(strip $(BOARD_TEE_CONFIG)), watchdata)
LOCAL_CFLAGS += -DTEE_PRODUCTION_CONFIG
LOCAL_SHARED_LIBRARIES  += libteeproduction
else ifeq ($(strip $(BOARD_TEE_CONFIG)), beanpod)
LOCAL_CFLAGS += -DTEE_PRODUCTION_CONFIG
LOCAL_SHARED_LIBRARIES  += libteeproduction
endif
endif

LOCAL_STATIC_LIBRARIES  := libbt-utils
LOCAL_LDLIBS        += -Idl
ifeq ($(strip $(BOARD_USE_EMMC)),true)
LOCAL_CFLAGS += -DCONFIG_EMMC
endif

ifeq ($(strip $(TARGET_USERIMAGES_USE_UBIFS)),true)
LOCAL_CFLAGS := -DCONFIG_NAND
endif

ifeq ($(USE_BOOT_AT_DIAG),true)
LOCAL_CFLAGS += -DUSE_BOOT_AT_DIAG
endif

LOCAL_C_INCLUDES    += hardware/libhardware/include \
                       hardware/libhardware_legacy/include \
                       system/core/include

ifneq ($(CONFIG_MINIENGPC), true)
LOCAL_C_INCLUDES    +=  vendor/sprd/modules/audio/voiceband/effect/include \
                        vendor/sprd/modules/audio/voiceband/gaincontrol/include \
                        vendor/sprd/modules/audio/nv_exchange/include \
                        vendor/sprd/proprietories-source/trustzone/libefuse/ \
                        vendor/sprd/modules/libatci/ \
                        vendor/sprd/modules/gps/gnsspc/ \
                        vendor/sprd/proprietories-source/libbm/ \
                        hardware/marvell/bt/libbt-vendor/
endif
LOCAL_C_INCLUDES    +=  external/sqlite/dist/ \
                        $(TARGET_OUT_INTERMEDIATES)/KERNEL/source/include/uapi/mtd/


LOCAL_SRC_FILES     := eng_pcclient.c  \
                       eng_diag.c \
                       vlog.c \
                       vdiag.c \
                       eng_productdata.c \
                       adc_calibration.c\
                       crc16.c \
                       eng_attok.c \
                       engopt.c \
                       eng_at.c \
                       eng_sqlite.c \
                       eng_btwifiaddr.c \
                       eng_cmd4linuxhdlr.c \
                       eng_testhardware.c \
                       power.c \
                       backlight.c \
                       eng_util.c \
                       eng_autotest.c \
                       eng_uevent.c \
                       eng_debug.c \
                       eng_ap_modem_time_sync.c

ifneq ($(CONFIG_MINIENGPC), true)
LOCAL_SRC_FILES += eng_busmonitor.c
endif

ifneq ($(CONFIG_MINIENGPC), true)

#### brcm BT BQB test
ifeq ($(BOARD_HAVE_BLUETOOTH_BCM),true)
LOCAL_C_INCLUDES += LOCAL_C_INCLUDES += vendor/sprd/modules/wcn/bt/libbqbbt/include/
LOCAL_CFLAGS += -DCONFIG_BRCM_BQBTEST
LOCAL_SHARED_LIBRARIES += libbt-vendor
LOCAL_SRC_FILES += eng_brcm_controllerbqbtest.c
endif #brcm BT BQB test

###GPS
ifeq ($(strip $(BOARD_USE_SPRD_4IN1_GPS)),true)
#LOCAL_SRC_FILES     += sprd_gps_eut.c
else
LOCAL_SRC_FILES     += gps_eut.c
endif

ifeq ($(strip $(BOARD_WLAN_DEVICE)),bcmdhd)
#brcm wifi
ifdef WIFI_DRIVER_FW_PATH_PARAM
LOCAL_CFLAGS += -DWIFI_DRIVER_FW_PATH_PARAM=\"$(WIFI_DRIVER_FW_PATH_PARAM)\"
endif
ifdef WIFI_DRIVER_FW_PATH_MFG
LOCAL_CFLAGS += -DWIFI_DRIVER_FW_PATH_MFG=\"$(WIFI_DRIVER_FW_PATH_MFG)\"
endif

ifeq ($(BOARD_HAVE_BLUETOOTH_BCM),true)
# bcm connectivity
LOCAL_CFLAGS += -DENGMODE_EUT_BCM
LOCAL_SRC_FILES     += bt_eut_pandora.c \
		       wifi_eut.c \
                       bt_eut.c
endif
else

ifneq ($(strip $(BOARD_HAVE_SPRD_WCN_COMBO)),)
# sprd connectivity
LOCAL_CFLAGS += -DENGMODE_EUT_SPRD

## wifi
### 2351
ifeq ($(BOARD_SPRD_WCNBT_SR2351),true)
LOCAL_CFLAGS += -DSPRD_WCN_SR2351
endif

### marlin
ifeq ($(BOARD_SPRD_WCNBT_MARLIN),true)
LOCAL_CFLAGS += -DSPRD_WCN_MARLIN
endif

LOCAL_SRC_FILES += wifi_eut_sprd.c
LOCAL_CFLAGS += -DWIFI_DRIVER_MODULE_PATH=\"$(WIFI_DRIVER_MODULE_PATH)\"
LOCAL_CFLAGS += -DWIFI_DRIVER_MODULE_NAME=\"$(WIFI_DRIVER_MODULE_NAME)\"

## bt
LOCAL_CFLAGS += -DHAVE_EUT_BLUETOOTH_SPRD
LOCAL_SHARED_LIBRARIES += libengbt
LOCAL_C_INCLUDES +=  vendor/sprd/proprietories-source/bt/libengbt
### bt bqb


LOCAL_C_INCLUDES += vendor/sprd/proprietories-source/engmode/bt/ \
                    vendor/sprd/modules/wcn/bt/libbqbbt/include/
LOCAL_CFLAGS += \
        -DCONFIG_BQBTEST \
        -DHAS_BLUETOOTH_SPRD=\"$(BOARD_HAVE_SPRD_WCN_COMBO)\"

endif
endif
endif

LOCAL_MODULE := engpc
LOCAL_INIT_RC := engpc.rc
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := 32
include $(BUILD_EXECUTABLE)
#include $(call all-makefiles-under,$(LOCAL_PATH))
#ENGHARDWARETEST
include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
LOCAL_LDLIBS        += -Idl
#LOCAL_CFLAGS += -DENG_API_LOG

LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_C_INCLUDES  += eng_hardware_test.h

LOCAL_SRC_FILES     :=   eng_hardware_test.c \
                         wifi_eut.c

LOCAL_MODULE:= enghardwaretest
LOCAL_MODULE_TAGS := optional
LOCAL_MULTILIB := 32
include $(BUILD_EXECUTABLE)
include $(call all-makefiles-under,$(LOCAL_PATH))
endif
