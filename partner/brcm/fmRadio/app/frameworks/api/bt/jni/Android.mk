LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)

LOCAL_CFLAGS += -DPLATFORM_ANDROID  -DBRCM_BT_USE_BTL_IF -DBT_USE_BTL_IF

# Relative path from current dir to vendor brcm
BRCM_BT_SRC_ROOT_PATH := ../../../../hardware/broadcom/bt

# Relative path from <mydroid> to brcm base
BRCM_BT_INC_ROOT_PATH := $(LOCAL_PATH)/../../../../hardware/broadcom/bt

#Add all JNI files in the below path
LOCAL_SRC_FILES := \
    bt_api_loader.cpp\
    com_broadcom_bt_util_bmsg_BMessageManager.cpp \
    bmessage/bmessage_api.c \
    bmessage/bmessage_support.c\
    bmessage/bmessage_util.c  \
    bmessage/smstpdu.cpp \
    bmessage/smsaddr.cpp \
    bmessage/smscodec.cpp

LOCAL_C_INCLUDES :=  \
			$(LOCAL_PATH) \
            $(JNI_H_INCLUDE) \
			$(BRCM_BT_INC_ROOT_PATH)/adaptation/btl-if/client \
			$(BRCM_BT_INC_ROOT_PATH)/adaptation/btl-if/include \
			$(BRCM_BT_INC_ROOT_PATH)/adaptation/dtun/include \
			$(BRCM_BT_INC_ROOT_PATH)/adaptation/include \
			$(LOCAL_PATH)/../../../../frameworks/base/include \
			$(LOCAL_PATH)/../../../../system/core/include

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE    := libbt-client-api

LOCAL_PRELINK_MODULE := false


include $(BUILD_SHARED_LIBRARY)
