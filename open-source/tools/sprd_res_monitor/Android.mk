LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= \
	sprd_res_monitor.c \
	res-monitor/res_mon.c \
	res-monitor/lsof.c
LOCAL_SHARED_LIBRARIES := \
    libcutils \
    libc \
    liboprofiledaemon

LOCAL_MODULE := sprd_res_monitor
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := sprd_monitor-userdebug.conf
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE := sprd_monitor-user.conf
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

CUSTOM_MODULES += sprd_monitor-userdebug.conf
CUSTOM_MODULES += sprd_monitor-user.conf
CUSTOM_MODULES += sprd_res_monitor

include $(call all-makefiles-under,$(LOCAL_PATH))
