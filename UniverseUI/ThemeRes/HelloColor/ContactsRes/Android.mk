LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

# LOCAL_AAPT_FLAGS := -o 

LOCAL_THEME_OVERLAY_PACKAGE := com.android.contacts

LOCAL_THEME_NAME := HelloColor
# LOCAL_THEME_RESOURCES := res
# LOCAL_THEME_DUMMY_MANIFEST := AndroidManifest.xml

##LOCAL_PACKAGE_NAME := ContactsRes
##LOCAL_MODULE_PATH := $(TARGET_OUT)/app
#LOCAL_THEME_NAME := UniverseUIClassic
LOCAL_THEME_VALUES := theme_values.xml

include $(BUILD_THEME_PACKAGE)
