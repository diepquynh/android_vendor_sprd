# Copyright 2007-2008 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

#for normal case,used normal AndroidManifest
#for cucc and mono sim version,used base AndroidManifest
#for cucc and dual sim version,used plus AndroidManifest
#ifneq (,$(findstring cucc, $(TARGET_PRODUCT)))
ifneq (,$(findstring cucc, $(TARGET_PRODUCT)))
    ifneq (,$(findstring base, $(TARGET_PRODUCT)))
        LOCAL_MANIFEST_FILE := cucc/base/AndroidManifest.xml
    else
        LOCAL_MANIFEST_FILE := cucc/plus/AndroidManifest.xml
    endif
else
    LOCAL_MANIFEST_FILE := AndroidManifest.xml
endif

$(info LOCAL_MANIFEST_FILE: $(LOCAL_MANIFEST_FILE))

LOCAL_MODULE_TAGS := optional

LOCAL_JAVA_LIBRARIES := telephony-common
LOCAL_JAVA_LIBRARIES += radio_interactor_common

LOCAL_SRC_FILES := $(call all-subdir-java-files)

LOCAL_PACKAGE_NAME := SprdStk
LOCAL_CERTIFICATE := platform

include $(BUILD_PACKAGE)
