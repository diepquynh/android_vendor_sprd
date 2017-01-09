ifeq ($(strip $(LOCAL_PACKAGE_NAME)), SystemUITests)
    LOCAL_SRC_FILES += $(call all-java-files-under, $(LOCAL_VENDOR_RELATIVE_PATH)/../ex-interface)
endif