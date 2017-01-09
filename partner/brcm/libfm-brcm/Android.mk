ifeq ($(BOARD_HAVE_BLUETOOTH_BCM),true)
ifeq ($(BOARD_HAVE_FM_BCM),true)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -DPLATFORM_ANDROID  -DBRCM_BT_USE_BTL_IF -DBT_USE_BTL_IF

# Relative path from current dir to vendor brcm
#BRCM_BT_SRC_ROOT_PATH := ../../hardware/broadcom/bt
BRCM_BT_SRC_ROOT_PATH := .
#BRCM_BT_SRC_ROOT_PATH := $(LOCAL_PATH)

# Relative path from <mydroid> to brcm base
#BRCM_BT_INC_ROOT_PATH := $(LOCAL_PATH)/../../hardware/broadcom/bt
BRCM_BT_INC_ROOT_PATH := $(LOCAL_PATH)
#BRCM_BT_INC_ROOT_PATH := $(LOCAL_PATH)/

#Add all JNI files in the below path
LOCAL_SRC_FILES := \
    $(BRCM_BT_SRC_ROOT_PATH)/adaptation/btl-if/client/btl_ifc_wrapper.c \
    $(BRCM_BT_SRC_ROOT_PATH)/adaptation/btl-if/client/btl_ifc.c\
    com_broadcom_bt_service_fm_FmReceiverService.cpp \
    CMutex.cpp \
    libfm-brcm_hw.cpp

# FmReceiverLoader.cpp \

LOCAL_C_INCLUDES :=  \
			$(LOCAL_PATH) \
			$(JNI_H_INCLUDE) \
			$(BRCM_BT_INC_ROOT_PATH)/adaptation/btl-if/client \
			$(BRCM_BT_INC_ROOT_PATH)/adaptation/btl-if/include \
			$(BRCM_BT_INC_ROOT_PATH)/adaptation/dtun/include \
			$(BRCM_BT_INC_ROOT_PATH)/adaptation/include \
			$(LOCAL_PATH)/../../../../frameworks/base/include \
			$(LOCAL_PATH)/../../../../system/core/include \
			$(LOCAL_PATH)/../../hardware/libhardware/include/hardware/

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
	liblog

LOCAL_MODULE_TAGS := optional

#LOCAL_MODULE    := libfmservice
LOCAL_MODULE    := fm.$(TARGET_BOARD_PLATFORM)

LOCAL_PROGUARD_ENABLED:=disabled

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_RELATIVE_PATH := hw


include $(BUILD_SHARED_LIBRARY)

#
# FmDaemon
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	  FmDaemon.cpp

ifneq ($(TARGET_BUILD_VARIANT),user)
LOCAL_CFLAGS:=    -DFM_UDP_SERVER
endif

LOCAL_C_INCLUDES:=\
        $(call include-path-for, bluedroid ) \
        system/bluetooth/bluedroid/include  \
        external/bluetooth/bluez/lib

LOCAL_SHARED_LIBRARIES := \
          libbluedroid \
          libcutils    \
          libutils

#LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE:=FmDaemon

include $(BUILD_EXECUTABLE)


#
# Fm
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	  FmDaemon.cpp

LOCAL_CFLAGS:= \


LOCAL_C_INCLUDES:=\
          $(call include-path-for, bluedroid ) \
          system/bluetooth/bluedroid/include  \
          external/bluetooth/bluez/lib

LOCAL_SHARED_LIBRARIES := \
          libbluedroid   \
          libcutils      \
          libutils

#LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE:=FmTest

include $(BUILD_EXECUTABLE)

endif
endif
