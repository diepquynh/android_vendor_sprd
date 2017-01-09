# Spreadtrum Communication Inc. 2016

# Ensure the proguard.flags is exists
ifneq (,$(wildcard $(LOCAL_PATH)/$(LOCAL_VENDOR_RELATIVE_PATH)/proguard.flags))
LOCAL_PROGUARD_FLAG_FILES += $(LOCAL_VENDOR_RELATIVE_PATH)/proguard.flags
endif

ifdef localDebug
$(info LOCAL_PROGUARD_FLAG_FILES := $(LOCAL_PROGUARD_FLAG_FILES))
endif
