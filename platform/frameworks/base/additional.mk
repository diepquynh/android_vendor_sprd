ifeq ($(strip $(LOCAL_MODULE)),framework)

LOCAL_AIDL_INCLUDES += $(LOCAL_VENDOR_PATH)/telephony/java \
                       $(LOCAL_VENDOR_PATH)/core/java \
                       $(LOCAL_VENDOR_PATH)/ex-interface/core/java

LOCAL_SRC_FILES += \
    $(LOCAL_VENDOR_RELATIVE_PATH)/telephony/java/com/android/internal/telephony/ITelephonyEx.aidl \
    $(LOCAL_VENDOR_RELATIVE_PATH)/telephony/java/com/android/internal/telephony/ICarrierConfigLoaderEx.aidl \
    $(LOCAL_VENDOR_RELATIVE_PATH)/telephony/java/com/android/internal/telephony/IFdnService.aidl \
    $(LOCAL_VENDOR_RELATIVE_PATH)/telephony/java/com/android/internal/telephony/IPhoneSubInfoEx.aidl \
    $(LOCAL_VENDOR_RELATIVE_PATH)/telephony/java/com/android/ims/internal/ImsCallForwardInfoEx.java \
    $(LOCAL_VENDOR_RELATIVE_PATH)/telephony/java/com/android/ims/internal/IImsServiceListenerEx.aidl \
    $(LOCAL_VENDOR_RELATIVE_PATH)/telephony/java/com/android/ims/internal/IImsRegisterListener.aidl \
    $(LOCAL_VENDOR_RELATIVE_PATH)/telephony/java/com/android/ims/internal/IImsUtListenerEx.aidl \
    $(LOCAL_VENDOR_RELATIVE_PATH)/telephony/java/com/android/ims/internal/IImsUtEx.aidl \
    $(LOCAL_VENDOR_RELATIVE_PATH)/telephony/java/com/android/ims/internal/IImsServiceEx.aidl \
    $(LOCAL_VENDOR_RELATIVE_PATH)/core/java/android/os/IPowerManagerEx.aidl \
    $(LOCAL_VENDOR_RELATIVE_PATH)/ex-interface/core/java/android/app/IPowerGuru.aidl

    LOCAL_SRC_FILES += $(call all-java-files-under, $(LOCAL_VENDOR_RELATIVE_PATH)/ex-interface)
endif
