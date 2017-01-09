LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := udtsid3.c
LOCAL_STATIC_LIBRARIES :=  cpufeatures
LOCAL_C_INCLUDES = \
		$(JNI_H_INCLUDE) \
		udtsid3.h \
		sensorytypes.h \
		trulyhandsfree.h

LOCAL_CFLAGS := -Wall
#LOCAL_SHARED_LIBRARIES := liblog
LOCAL_SHARED_LIBRARIES := libdl libcutils liblog
LOCAL_LDFLAGS := -llog \
                $(LOCAL_PATH)/libthf_armeabi-v7a.a

#ifeq ($(strip $(TARGET_ARCH)), arm)
#	LOCAL_LDFLAGS += $(LOCAL_PATH)/libthf_armeabi-v7a.a
#endif
#ifeq ($(strip $(TARGET_ARCH)), arm64)
#	LOCAL_LDFLAGS += $(LOCAL_PATH)/libthf_arm64-v8a.a
#endif

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE    := libjni_udtsid

include $(BUILD_SHARED_LIBRARY)
