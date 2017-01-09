LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(strip $(TARGET_ARCH)), arm)
include $(CLEAR_VARS)
LOCAL_PREBUILT_LIBS :=libSprdRecordNrProcess.a
include $(BUILD_MULTI_PREBUILT)
endif

ifeq ($(strip $(TARGET_ARCH)), arm64)
LOCAL_MODULE := libSprdRecordNrProcess
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
LOCAL_MULTILIB := 32
LOCAL_MODULE_STEM_32 := libSprdRecordNrProcess.a
LOCAL_SRC_FILES_32 := libSprdRecordNrProcess.a
LOCAL_MODULE_TAGS := optional
include $(BUILD_PREBUILT)
endif


