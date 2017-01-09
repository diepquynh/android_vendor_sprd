ifeq ($(strip $(LOCAL_MODULE)),telephony-common)
    LOCAL_SRC_FILES += $(call all-java-files-under, $(LOCAL_VENDOR_RELATIVE_PATH)/ex-interface)
    LOCAL_SRC_FILES += $(LOCAL_VENDOR_RELATIVE_PATH)/ex-interface/com/android/internal/telephony/IIccPhoneBookEx.aidl
    LOCAL_SRC_FILES += $(LOCAL_VENDOR_RELATIVE_PATH)/ex-interface/com/android/internal/telephony/ISmsEx.aidl
    LOCAL_AIDL_INCLUDES += $(LOCAL_VENDOR_PATH)/ex-interface
    LOCAL_JAVA_LIBRARIES += radio_interactor_common
endif
