#LIBDUMPDATA
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_LDLIBS        += -Idl

#LOCAL_CFLAGS += -DENG_API_LOG

LOCAL_C_INCLUDES  := external/expat/lib 

LOCAL_SRC_FILES     :=   dumpdata.c


LOCAL_SHARED_LIBRARIES :=  liblog libcutils libexpat libdl 




LOCAL_MODULE:= libdumpdata
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)



