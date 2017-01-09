LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
    proto/ril_oem.proto

LOCAL_C_INCLUDES += \
    external/protobuf/src \

LOCAL_PROTOC_OPTIMIZE_TYPE := lite

LOCAL_CFLAGS :=

LOCAL_MODULE:= libril_oem

#LOCAL_LDLIBS += -lpthread

include $(BUILD_SHARED_LIBRARY)
# Create java protobuf code
include $(CLEAR_VARS)

src_proto := $(LOCAL_PATH)
LOCAL_MODULE := ril-oem-java-static
LOCAL_SRC_FILES := proto/ril_oem.proto
LOCAL_PROTOC_OPTIMIZE_TYPE := micro
#LOCAL_LDLIBS += -lpthread

include $(BUILD_STATIC_JAVA_LIBRARY)
