LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_THEME_OVERLAY_PACKAGE := com.android.calculator2

LOCAL_THEME_VALUES := theme_values.xml

include $(BUILD_THEME_PACKAGE)
