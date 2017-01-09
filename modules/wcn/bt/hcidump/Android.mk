LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := hcidump.c
LOCAL_MODULE := hcidump
LOCAL_STATIC_LIBRARIES := libcutils
LOCAL_MODULE_TAGS := optional
#LOCAL_LDLIBS += -lpthread
LOCAL_SHARED_LIBRARIES := liblog libz
include $(BUILD_EXECUTABLE)

CUSTOM_MODULES += hcidump
