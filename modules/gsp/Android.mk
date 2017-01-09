LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := gsp.$(TARGET_BOARD_PLATFORM)

LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_MODULE_TAGS := optional

LOCAL_SHARED_LIBRARIES := liblog libutils libcutils libhardware libui libsync libdl

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../external/kernel-headers \
                    $(LOCAL_PATH)/../libmemion \
                    $(LOCAL_PATH)/../hwcomposer \
                    $(LOCAL_PATH)/../

LOCAL_C_INCLUDES += $(GPU_GRALLOC_INCLUDES)

LOCAL_CFLAGS := -DLOG_TAG=\"GSP\"



LOCAL_SRC_FILES := GspPlane.cpp \
                   GspDevice.cpp \
                   GspModule.cpp \
                   GspR1P1Plane/GspR1P1Plane.cpp \
                   GspR1P1Plane/GspR1P1PlaneArray.cpp \
                   GspR1P1Plane/GspR1P1PlaneDst.cpp \
                   GspR1P1Plane/GspR1P1PlaneImage.cpp \
                   GspR1P1Plane/GspR1P1PlaneOsd.cpp \
                   GspR3P0Plane/GspR3P0Plane.cpp \
                   GspR3P0Plane/GspR3P0PlaneArray.cpp \
                   GspR3P0Plane/GspR3P0PlaneDst.cpp \
                   GspR3P0Plane/GspR3P0PlaneImage.cpp \
                   GspR3P0Plane/GspR3P0PlaneOsd.cpp \
                   GspLiteR1P0Plane/GspLiteR1P0Plane.cpp \
                   GspLiteR1P0Plane/GspLiteR1P0PlaneArray.cpp \
                   GspLiteR1P0Plane/GspLiteR1P0PlaneDst.cpp \
                   GspLiteR1P0Plane/GspLiteR1P0PlaneImage.cpp \
                   GspLiteR1P0Plane/GspLiteR1P0PlaneOsd.cpp

include $(BUILD_SHARED_LIBRARY)
