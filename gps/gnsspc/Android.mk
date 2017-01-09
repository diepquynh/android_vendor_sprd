
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := gps_pc_mode.c fft.c

LOCAL_MODULE := libgpspc
LOCAL_MODULE_TAGS := debug

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)\
	hardware/libhardware_legacy \
	hardware/libhardware_legacy/include/hardware_legacy \
	hardware/libhardware/include/hardware

LOCAL_SHARED_LIBRARIES := \
	libc \
	libutils \
	libcutils \
	liblog \
	libdl


LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)


