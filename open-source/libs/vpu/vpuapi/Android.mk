LOCAL_PATH := $(call my-dir)

# Building the vpuapi
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
 	vpuapi.c \
	vpuapifunc.c \
	vpuhelper.c \
	vpuio.c \

 LOCAL_SRC_FILES += \
 	../vdi/vdi.c \
   	../vdi/vdi_osal.c \
	../vdi/android_memory_alloc.cpp \


# LOCAL_SRC_FILES += \
#  		../vdi/socket/vdi.c \
#  		../vdi/socket/vdi_osal.c \
# 		../vdi/socket/hpi_client.c \
# 		../vdi/mm.c


LOCAL_MODULE_TAGS := eng
LOCAL_MODULE := libvpu

LOCAL_CFLAGS := -DCONFIG_DEBUG_LEVEL=255
# LOCAL_CFLAGS += -DCNM_FPGA_PLATFORM

LOCAL_STATIC_LIBRARIES := 

LOCAL_SHARED_LIBRARIES :=         \
		libutils          \
		libdl             \
		liblog            \
		libmemoryheapion
		
LOCAL_C_INCLUDES := $(LOCAL_PATH)/src		\
		$(TOP)/vendor/sprd/open-source/libs/vpu	\
		$(TOP)/vendor/sprd/open-source/libs/vpu/include \
		$(TOP)/vendor/sprd/open-source/libs/vpu/vdi \
		$(TOP)/vendor/sprd/open-source/libs/libmemoryheapion \
		$(TARGET_OUT_INTERMEDIATES)/KERNEL/usr/include/video \

include $(BUILD_SHARED_LIBRARY)

