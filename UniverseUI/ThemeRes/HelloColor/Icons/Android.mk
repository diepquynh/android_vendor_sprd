LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

# LOCAL_AAPT_FLAGS := -o 

LOCAL_THEME_OVERLAY_PACKAGE := res.sprd.icons

# Specail package, its resources can't overlay any packages
LOCAL_PACKAGE_SUPPORT_OVERLAY := false

LOCAL_THEME_NAME := HelloColor
#LOCAL_THEME_VALUES := theme_values.xml
LOCAL_THEME_DUMMY_MANIFEST := AndroidManifest.xml

include $(BUILD_THEME_PACKAGE)
