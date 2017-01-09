LOCAL_PATH:= $(call my-dir)


include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := mxdcmmbplayer.apk
LOCAL_MODULE_CLASS := APPS
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_SRC_FILES := app/mxdcmmbplayer.apk

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := mxdcmmbtest.apk
LOCAL_MODULE_CLASS := APPS
LOCAL_CERTIFICATE := PRESIGNED
LOCAL_SRC_FILES := app/mxdcmmbtest.apk

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libiwcmmb_jni_demon.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES := lib/libiwcmmb_jni_demon.so

include $(BUILD_PREBUILT)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libiwcmmbdev_demon.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES := lib/libiwcmmbdev_demon.so

include $(BUILD_PREBUILT)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libiwcmmb_jni.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES := lib/libiwcmmb_jni.so

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libiwcmmbdev.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES := lib/libiwcmmbdev.so

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libiwmbbms.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES := lib/libiwmbbms.so

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libiwmbbms_jni.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES := lib/libiwmbbms_jni.so

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libiwmbbms_tel.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES := lib/libiwmbbms_tel.so

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libiwuam.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES := lib/libiwuam.so

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libmxdcmmb.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES := lib/libmxdcmmb.so

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libplayEngine.so
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_SRC_FILES := lib/libplayEngine.so

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := mxdcmmbserver
LOCAL_MODULE_CLASS := EXECUTABLES 
LOCAL_SRC_FILES := bin/mxdcmmbserver

include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := mxdid.id
LOCAL_MODULE_CLASS := ETC 
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)
LOCAL_SRC_FILES := bin/$(LOCAL_MODULE) 
ifeq ($(CMMB_MXDID),0265)
LOCAL_SRC_FILES := bin/0265/$(LOCAL_MODULE) 
endif
ifeq ($(CMMB_MXDID),0266)
LOCAL_SRC_FILES := bin/0266/$(LOCAL_MODULE) 
endif
$(warning $(LOCAL_SRC_FILES))
include $(BUILD_PREBUILT)



