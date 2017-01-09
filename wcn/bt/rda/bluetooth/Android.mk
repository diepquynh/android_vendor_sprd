
LOCAL_PATH := $(call my-dir)

#ifeq ($(BOARD_BLUETOOTH_CHIP),rda)
#$(error "hardware/rda/bluetooth/Android.mk  test complie!")
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  rda.c \
  bt_em.c \
  rda5876_init.c \
  rdabt_interface.c \
  rdabt_poweron.c



LOCAL_C_INCLUDES := \
  $(LOCAL_PATH)/../../../../../../system/bt/hci/include \
  $(LOCAL_PATH)/../../../../../../system/bt/stack/include

$(warning "TOP=$(TOP)")
LOCAL_CFLAGS := \
  -DBLUETOOTH_RDA587X

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libbluetooth_rda
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SHARED_LIBRARIES := liblog
include $(BUILD_SHARED_LIBRARY)



include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  bt_drv.c


LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../../../../../../system/bt/hci/include


LOCAL_CFLAGS := 

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libbt-vendor
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SHARED_LIBRARIES := liblog libbluetooth_rda
include $(BUILD_SHARED_LIBRARY)

#
## RDA bt_test
#
#
include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
  rda_bt_test.c

LOCAL_C_INCLUDES := \
  $(LOCAL_PATH)/../../../../../../system/bt/hci/include


LOCAL_CFLAGS :=

LOCAL_MODULE := bt_test
LOCAL_MODULE_TAGS := debug
LOCAL_SHARED_LIBRARIES := liblog libbluetooth_rda
include $(BUILD_EXECUTABLE)

#
# RDA bt fcc ce
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES := rda_bt_cert.c\
                   rdabt_fcc_ce_init.c

LOCAL_C_INCLUDES := \
   $(LOCAL_PATH)/../../../../../../system/bt/hci/include


LOCAL_SHARED_LIBRARIES := liblog libbluetooth_rda
LOCAL_LDLIBS:= -llog
LOCAL_CFLAGS :=

LOCAL_MODULE := bt_cert
LOCAL_MODULE_TAGS := debug

include $(BUILD_EXECUTABLE)
#endif
