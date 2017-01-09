ifeq ($(strip $(LOCAL_PACKAGE_NAME)),TeleService)
LOCAL_SRC_FILES +=$(call all-java-files-under, $(LOCAL_VENDOR_RELATIVE_PATH)/ex-interface)
endif