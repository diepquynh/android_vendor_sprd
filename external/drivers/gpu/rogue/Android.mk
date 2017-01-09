ifneq ($(TARGET_SIMULATOR),true)
LOCAL_PATH := $(call my-dir)

ifeq ($(strip $(TARGET_ARCH)),x86_64)
#bin

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := pvrdebug
LOCAL_MODULE_CLASS := EXECUTABLES
ifeq ($(strip $(TARGET_BUILD_VARIANT)), user)
LOCAL_SRC_FILES :=  usr/bin/pvrdebug
else
LOCAL_SRC_FILES :=  debug/bin/pvrdebug
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := pvrhwperf
LOCAL_MODULE_CLASS := EXECUTABLES
ifeq ($(strip $(TARGET_BUILD_VARIANT)), user)
LOCAL_SRC_FILES :=  usr/bin/pvrhwperf
else
LOCAL_SRC_FILES :=  debug/bin/pvrhwperf
endif
include $(BUILD_PREBUILT)


#egl
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libEGL_POWERVR_ROGUE.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_RELATIVE_PATH := egl
LOCAL_MULTILIB := both
ifeq ($(strip $(TARGET_BUILD_VARIANT)), user)
LOCAL_SRC_FILES_32 :=  usr/lib/egl/libEGL_POWERVR_ROGUE.so
LOCAL_SRC_FILES_64 :=  usr/lib64/egl/libEGL_POWERVR_ROGUE.so
else
LOCAL_SRC_FILES_32 :=  debug/lib/egl/libEGL_POWERVR_ROGUE.so
LOCAL_SRC_FILES_64 :=  debug/lib64/egl/libEGL_POWERVR_ROGUE.so
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libGLESv1_CM_POWERVR_ROGUE.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_RELATIVE_PATH := egl
LOCAL_MULTILIB := both
ifeq ($(strip $(TARGET_BUILD_VARIANT)), user)
LOCAL_SRC_FILES_32 :=  usr/lib/egl/libGLESv1_CM_POWERVR_ROGUE.so
LOCAL_SRC_FILES_64 :=  usr/lib64/egl/libGLESv1_CM_POWERVR_ROGUE.so
else
LOCAL_SRC_FILES_32 :=  debug/lib/egl/libGLESv1_CM_POWERVR_ROGUE.so
LOCAL_SRC_FILES_64 :=  debug/lib64/egl/libGLESv1_CM_POWERVR_ROGUE.so
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libGLESv2_POWERVR_ROGUE.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_RELATIVE_PATH := egl
LOCAL_MULTILIB := both
ifeq ($(strip $(TARGET_BUILD_VARIANT)), user)
LOCAL_SRC_FILES_32 :=  usr/lib/egl/libGLESv2_POWERVR_ROGUE.so
LOCAL_SRC_FILES_64 :=  usr/lib64/egl/libGLESv2_POWERVR_ROGUE.so
else
LOCAL_SRC_FILES_32 :=  debug/lib/egl/libGLESv2_POWERVR_ROGUE.so
LOCAL_SRC_FILES_64 :=  debug/lib64/egl/libGLESv2_POWERVR_ROGUE.so
endif
include $(BUILD_PREBUILT)

#hw
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional

ifeq ($(strip $(TARGET_BOARD_PLATFORM)),)
LOCAL_MODULE := gralloc.default.so
else
LOCAL_MODULE := gralloc.$(TARGET_BOARD_PLATFORM).so
endif

LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MULTILIB := both
ifeq ($(strip $(TARGET_BUILD_VARIANT)), user)
LOCAL_SRC_FILES_32 :=  usr/lib/hw/gralloc.generic.so
LOCAL_SRC_FILES_64 :=  usr/lib64/hw/gralloc.generic.so
else
LOCAL_SRC_FILES_32 :=  debug/lib/hw/gralloc.generic.so
LOCAL_SRC_FILES_64 :=  debug/lib64/hw/gralloc.generic.so
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := memtrack.generic.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MULTILIB := both
ifeq ($(strip $(TARGET_BUILD_VARIANT)), user)
LOCAL_SRC_FILES_32 :=  usr/lib/hw/memtrack.generic.so
LOCAL_SRC_FILES_64 :=  usr/lib64/hw/memtrack.generic.so
else
LOCAL_SRC_FILES_32 :=  debug/lib/hw/memtrack.generic.so
LOCAL_SRC_FILES_64 :=  debug/lib64/hw/memtrack.generic.so
endif
include $(BUILD_PREBUILT)

#lib/lib64
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libcreatesurface.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
ifeq ($(strip $(TARGET_BUILD_VARIANT)), user)
LOCAL_SRC_FILES_32 :=  usr/lib/libcreatesurface.so
LOCAL_SRC_FILES_64 :=  usr/lib64/libcreatesurface.so
else
LOCAL_SRC_FILES_32 :=  debug/lib/libcreatesurface.so
LOCAL_SRC_FILES_64 :=  debug/lib64/libcreatesurface.so
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libglslcompiler.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
ifeq ($(strip $(TARGET_BUILD_VARIANT)), user)
LOCAL_SRC_FILES_32 :=  usr/lib/libglslcompiler.so
LOCAL_SRC_FILES_64 :=  usr/lib64/libglslcompiler.so
else
LOCAL_SRC_FILES_32 :=  debug/lib/libglslcompiler.so
LOCAL_SRC_FILES_64 :=  debug/lib64/libglslcompiler.so
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libIMGegl.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
ifeq ($(strip $(TARGET_BUILD_VARIANT)), user)
LOCAL_SRC_FILES_32 :=  usr/lib/libIMGegl.so
LOCAL_SRC_FILES_64 :=  usr/lib64/libIMGegl.so
else
LOCAL_SRC_FILES_32 :=  debug/lib/libIMGegl.so
LOCAL_SRC_FILES_64 :=  debug/lib64/libIMGegl.so
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libpvrANDROID_WSEGL.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
ifeq ($(strip $(TARGET_BUILD_VARIANT)), user)
LOCAL_SRC_FILES_32 :=  usr/lib/libpvrANDROID_WSEGL.so
LOCAL_SRC_FILES_64 :=  usr/lib64/libpvrANDROID_WSEGL.so
else
LOCAL_SRC_FILES_32 :=  debug/lib/libpvrANDROID_WSEGL.so
LOCAL_SRC_FILES_64 :=  debug/lib64/libpvrANDROID_WSEGL.so
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libPVRScopeServices.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
ifeq ($(strip $(TARGET_BUILD_VARIANT)), user)
LOCAL_SRC_FILES_32 :=  usr/lib/libPVRScopeServices.so
LOCAL_SRC_FILES_64 :=  usr/lib64/libPVRScopeServices.so
else
LOCAL_SRC_FILES_32 :=  debug/lib/libPVRScopeServices.so
LOCAL_SRC_FILES_64 :=  debug/lib64/libPVRScopeServices.so
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libsrv_um.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
ifeq ($(strip $(TARGET_BUILD_VARIANT)), user)
LOCAL_SRC_FILES_32 :=  usr/lib/libsrv_um.so
LOCAL_SRC_FILES_64 :=  usr/lib64/libsrv_um.so
else
LOCAL_SRC_FILES_32 :=  debug/lib/libsrv_um.so
LOCAL_SRC_FILES_64 :=  debug/lib64/libsrv_um.so
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libsutu_display.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
ifeq ($(strip $(TARGET_BUILD_VARIANT)), user)
LOCAL_SRC_FILES_32 :=  usr/lib/libsutu_display.so
LOCAL_SRC_FILES_64 :=  usr/lib64/libsutu_display.so
else
LOCAL_SRC_FILES_32 :=  debug/lib/libsutu_display.so
LOCAL_SRC_FILES_64 :=  debug/lib64/libsutu_display.so
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libusc.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MULTILIB := both
ifeq ($(strip $(TARGET_BUILD_VARIANT)), user)
LOCAL_SRC_FILES_32 :=  usr/lib/libusc.so
LOCAL_SRC_FILES_64 :=  usr/lib64/libusc.so
else
LOCAL_SRC_FILES_32 :=  debug/lib/libusc.so
LOCAL_SRC_FILES_64 :=  debug/lib64/libusc.so
endif
include $(BUILD_PREBUILT)

#special files
include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := rgx.fw.signed
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_PATH := $(TARGET_OUT_VENDOR)/firmware
ifeq ($(strip $(TARGET_BUILD_VARIANT)), user)
LOCAL_SRC_FILES :=  usr/firmware/rgx.fw.signed
else
LOCAL_SRC_FILES :=  debug/firmware/rgx.fw.signed
endif
include $(BUILD_PREBUILT)

include $(LOCAL_PATH)/driver/build/linux/sprd_android/Android.mk

else
include $(CLEAR_VARS)

include $(BUILD_PREBUILT)
endif

endif
