#
#
#autotest makefile
#
ifneq ($(strip $(PLATFORM_VERSION)), 6.0.1)

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(AUTOTST_DEVLP_DBG),true)
LOCAL_CPPFLAGS += -DAUTOTST_DEVLP_DBG
endif

ifeq ($(PLATFORM_VERSION), 6.0)
LOCAL_CFLAGS += -DGOOGLE_FM_INCLUDED
endif

ifeq ($(BOARD_SPRD_WCNBT_MARLIN),true)
	LOCAL_CFLAGS  += -DSPRD_WCNBT_MARLIN
endif

ifeq ($(BOARD_SPRD_WCNBT_SR2351),true)
	LOCAL_CFLAGS += -DSPRD_WCNBT_SR2351
endif

ifeq ($(PLATFORM_VERSION), 7.0)
LOCAL_CFLAGS += -DGOOGLE_FM_INCLUDED
endif

LOCAL_MODULE_TAGS:= optional

LOCAL_MODULE:= autotest

LOCAL_INIT_RC := autotest.rc

LOCAL_32_BIT_ONLY := true

#LOCAL_SRC_FILES:= \
    atv.cpp \
    audio.cpp \
    bt_fm_mixed.cpp \
    fm.cpp \
    perm.cpp \
    sensor.cpp

LOCAL_SRC_FILES:= \
    otg.cpp \
    sensor.cpp \
    battery.cpp  \
    camera.cpp   \
    channel.cpp  \
    cmmb.cpp     \
    debug.cpp  \
    diag.cpp   \
    driver.cpp \
    gps.cpp    \
    input.cpp  \
    tp.cpp    \
    lcd.cpp    \
    light.cpp  \
    sim.cpp    \
    tcard.cpp  \
    fm.cpp \
    tester.cpp \
    tester_main.cpp \
    tester_dbg.cpp  \
    util.cpp        \
    ver.cpp         \
    vibrator.cpp    \
    bt.cpp          \
    wifi.cpp        \
    power.cpp       \
    ui.c  \
    mipi_lcd.c   \
	key_common.cpp	\
	events.c	\
    gsensor.cpp   \
    lsensor.cpp   \
    testcomm.c  \
    main.cpp

LOCAL_C_INCLUDES:= \
    external/bluetooth/bluez/lib \
    external/bluetooth/bluez/src \
    frameworks/base/include \
    frameworks/av/include \
    system/bluetooth/bluedroid/include \
    system/core/include \
    hardware/libhardware/include \
    hardware/libhardware_legacy/include \
    external/bluetooth/bluedroid/btif/include \
    external/bluetooth/bluedroid/gki/ulinux \
    external/bluetooth/bluedroid/stack/include \
    external/bluetooth/bluedroid/stack/btm \
    $(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video
LOCAL_C_INCLUDES    +=  vendor/sprd/proprietories-source/autotest/minui \
			vendor/sprd/proprietories-source/autotest/res \
          system/core/libpixelflinger/include

LOCAL_C_INCLUDES += \
    vendor/sprd/modules/libatci


#    libbluedroid \
#    libbluetooth \
#    libbluetoothd\

LOCAL_SHARED_LIBRARIES:= \
    libbinder \
    liblog \
    libcamera_client \
    libcutils    \
    libdl        \
    libgui       \
    libhardware  \
    libhardware_legacy \
    libmedia \
    libui    \
    libutils \
    libatci \
    libpixelflinger

LOCAL_STATIC_LIBRARIES:= \
    libbt-utils\
    libbt-hci
LOCAL_STATIC_LIBRARIES += libatminui
#LOCAL_STATIC_LIBRARIES += libpixelflinger_static
include $(BUILD_EXECUTABLE)
include $(call all-makefiles-under,$(LOCAL_PATH))

endif
